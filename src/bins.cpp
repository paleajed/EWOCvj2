#include <boost/filesystem.hpp>


#include "GL/glew.h"
#include "GL/gl.h"
#include "GL/glut.h"

#ifdef __linux__
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/cursorfont.h>
#endif

#include "SDL2/SDL.h"
#include "SDL2/SDL_syswm.h"

#include <algorithm>
#include <ostream>
#include <istream>
#include <iostream>
#include <ios>

#include "tinyfiledialogs.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
#include "libswscale/swscale.h"
#include "libavutil/avutil.h"
#include "libavutil/pixfmt.h"
#include "libavutil/opt.h"
#include "libavutil/imgutils.h"
}

// my own headers
#include "box.h"
#include "effect.h"
#include "node.h"
#include "layer.h"
#include "window.h"
#include "program.h"
#include "loopstation.h"
#include "bins.h"


Bin::Bin(int pos) {
	this->box = new Box;
	this->box->vtxcoords->x1 = -0.15f;
	this->box->vtxcoords->y1 = (pos + 1) * -0.05f;
	this->box->vtxcoords->w = 0.3f;
	this->box->vtxcoords->h = 0.05f;
	this->box->upvtxtoscr();
	this->box->tooltiptitle = "List of media bins ";
	this->box->tooltip = "Media bins listed by name. Leftclick to make bin current. Leftdrag to move bin inside bin list. Rightclick to access bin renaming and bin deleting. ";
}

Bin::~Bin() {
	delete this->box;
}

BinElement::BinElement() {
	glGenTextures(1, &this->tex);
	glBindTexture(GL_TEXTURE_2D, this->tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

BinElement::~BinElement() {
	glDeleteTextures(1, &this->tex);
	glDeleteTextures(1, &this->oldtex);
}

BinDeck::BinDeck() {
	this->box = new Box;
	this->box->vtxcoords->w = 0.36f;
	this->box->upvtxtoscr();
	this->box->lcolor[0] = 0.5f;
	this->box->lcolor[1] = 0.5f;
	this->box->lcolor[2] = 1.0f;
	this->box->lcolor[3] = 1.0f;
}

BinDeck::~BinDeck() {
	delete this->box;
}

BinMix::BinMix() {
	this->box = new Box;
	this->box->vtxcoords->w = 0.72f;
	this->box->upvtxtoscr();
	this->box->lcolor[0] = 0.5f;
	this->box->lcolor[1] = 1.0f;
	this->box->lcolor[2] = 0.5f;
	this->box->lcolor[3] = 1.0f;
}

BinMix::~BinMix() {
	delete this->box;
}

BinsMain::BinsMain() {
	for (int i = 0; i < 6; i++) {
		for (int j = 0; j < 24; j++) {
			Box *box = new Box;
			this->elemboxes.push_back(box);
			box->vtxcoords->x1 = -0.95f + i * 0.12f + (1.2f * (j > 11));
			box->vtxcoords->y1 = 0.95f - ((j % 12) + 1) * 0.15f;
			box->vtxcoords->w = 0.1f;
			box->vtxcoords->h = 0.1f;
			box->upvtxtoscr();
			box->lcolor[0] = 0.4f;
			box->lcolor[1] = 0.4f;
			box->lcolor[2] = 0.4f;
			box->lcolor[3] = 1.0f;
			box->acolor[3] = 1.0f;
			box->tooltiptitle = "Media bin element ";
			box->tooltip = "Shows thumbnail of media bin element, either being a video file (grey border) or a layer file (orange border) or belonging to a deck file (purple border) or mix file (green border).  Hovering over this element shows video resolution and video compression method (CPU or HAP).  Mousewheel skips through the element contents (previewed in larger monitor topmiddle).  Leftdrag allows dragging to mix screen via wormhole.  Leftclick allows moving inside the media bin. Rightclickmenu allows among other things, HAP encoding. ";
		}
	}
	
	this->newbinbox = new Box;
	this->newbinbox->vtxcoords->x1 = -0.15f;
	this->newbinbox->vtxcoords->w = 0.3f;
	this->newbinbox->vtxcoords->h = 0.05f;
	this->newbinbox->upvtxtoscr();
	this->newbinbox->tooltiptitle = "Add new bin ";
	this->newbinbox->tooltip = "Leftclick to add a new bin and make it current. ";
	
	this->inputbinel = new BinElement;
	
	this->currbin = new Bin(-1);
	
	this->hapmodebox = new Box;
	this->hapmodebox->vtxcoords->x1 = -0.05f;
	this->hapmodebox->vtxcoords->y1 = 0.65f;
	this->hapmodebox->vtxcoords->w = 0.1f;
	this->hapmodebox->vtxcoords->h = 0.075f;
	this->hapmodebox->upvtxtoscr();
	this->hapmodebox->tooltiptitle = "HAP encode modus ";
	this->hapmodebox->tooltip = "Toggles between hap encoding for during live situations (only using one core, not to slow down the realtime video mix, and hap encoding at full power (using 'number of system cores + 1' threads). ";
}

void BinsMain::handle() {
	float white[] = {1.0f, 1.0f, 1.0f, 1.0f};
	float black[] = {0.0f, 0.0f, 0.0f, 1.0f};
	float darkgrey[] = {0.2f, 0.2f, 0.2f, 1.0f};
	float lightblue[] = {0.5f, 0.5f, 1.0f, 1.0f};
	float red[] = {1.0f, 0.5f, 0.5f, 1.0f};
	float lightgreen[] = {0.5f, 1.0f, 0.5f, 1.0f};
	
	//draw binelements
	if (mainprogram->menuactivation) mainprogram->menuset = 0;
	for (int i = 0; i < 6; i++) {
		for (int j = 0; j < 24; j++) {
			Box *box = this->elemboxes[i * 24 + j];
			BinElement *binel = this->currbin->elements[i * 24 + j];
			if (box->in()) {
				if (binel->path != "" and mainprogram->menuactivation) {
					this->menubinel = binel;
					std::vector<std::string> binel;
					binel.push_back("Delete element");
					binel.push_back("Insert file from disk");
					binel.push_back("Insert dir from disk");
					binel.push_back("Insert deck A");
					binel.push_back("Insert deck B");
					binel.push_back("Insert full mix");
					binel.push_back("HAP encode element");
					binel.push_back("Quit");
					mainprogram->make_menu("binelmenu", mainprogram->binelmenu, binel);
					mainprogram->menuset = 1;
				}
			}
			float color[4];
			if (binel->type == ELEM_LAYER) {
				color[0] = 1.0f;
				color[1] = 0.5f;
				color[2] = 0.0f;
				color[3] = 1.0f;
			}
			else if (binel->type == ELEM_DECK) {
				color[0] = 0.5f;
				color[1] = 0.5f;
				color[2] = 1.0f;
				color[3] = 1.0f;
			}
			else if (binel->type == ELEM_MIX) {
				color[0] = 0.5f;
				color[1] = 1.0f;
				color[2] = 0.5f;
				color[3] = 1.0f;
			}
			else {
				color[0] = 0.4f;
				color[1] = 0.4f;
				color[2] = 0.4f;
				color[3] = 1.0f;
			}
			draw_box(nullptr, color, box->vtxcoords->x1 - 0.01f, box->vtxcoords->y1 - 0.01f, box->vtxcoords->w + 0.02f, box->vtxcoords->h + 0.02f, -1);
			draw_box(box, -1);  //in case of alpha thumbnail
			draw_box(box, binel->tex);
			if (remove_extension(basename(binel->path)) != "") render_text(basename(binel->path).substr(0, 20), white, box->vtxcoords->x1, box->vtxcoords->y1 - 0.02f, 0.00045f, 0.00075f);
		}
		Box *box = this->elemboxes[i * 24];
		draw_box(nullptr, darkgrey, box->vtxcoords->x1 + box->vtxcoords->w, -1.0f, 0.12f, 2.0f, -1);
		box = this->elemboxes[i * 24 + 12];
		draw_box(nullptr, darkgrey, box->vtxcoords->x1 + box->vtxcoords->w, -1.0f, 0.12f, 2.0f, -1);
	}
	
	// set threadmode for hap encoding
	render_text("HAP Encoding Mode", white, -0.125f, 0.8f, 0.00075f, 0.0012f);
	draw_box(white, black, binsmain->hapmodebox, -1);
	draw_box(white, lightblue, -0.049f + 0.048f * mainprogram->threadmode, 0.6575f, 0.048f, 0.06f, -1);
	render_text("Live mode", white, -0.15f, 0.6f, 0.00075f, 0.0012f);
	render_text("Max mode", white, 0.01f, 0.6f, 0.00075f, 0.0012f);
	render_text("1 thread", white, -0.15f, 0.55f, 0.00075f, 0.0012f);
	mainprogram->maxthreads = mainprogram->numcores * mainprogram->threadmode + 1;
	render_text(std::to_string(mainprogram->numcores + 1) + " threads", white, 0.01f, 0.55f, 0.00075f, 0.0012f);
	if (binsmain->hapmodebox->in()) {
		if (mainprogram->leftmouse) {
			mainprogram->threadmode = !mainprogram->threadmode;
			mainprogram->leftmouse = false;
		}
	}
	
	Layer *lay = nullptr;		
	if (mainmix->currlay) lay = mainmix->currlay;
	else {
		if (mainprogram->prevmodus) lay = mainmix->layersA[0];
		else lay = mainmix->layersAcomp[0];
	}
	int deck = this->inserting;
		
	if (mainprogram->rightmouse and this->currbinel) {
		bool cont = false;
		if (this->prevbinel) {
			if (this->templayers.size()) {
				int offset = 0;
				for (int k = 0; k < this->templayers.size(); k++) {
					int offj = (int)((this->previ + offset + k) / 6);
					int el = ((this->previ + offset + k + 144) % 6) * 24 + this->prevj + offj;
					if (this->prevj + offj > 23) {
						offset = k - this->templayers.size();
						cont = true;
						break;
					}
					BinElement *dirbinel = this->currbin->elements[el];
					while (dirbinel->type == ELEM_DECK or dirbinel->type == ELEM_MIX) {
						offset++;
						offj = (int)((this->previ + offset + k) / 6);
						el = ((this->previ + offset + k + 144) % 6) * 24 + this->prevj + offj;
						if (this->prevj + offj > 23) {
							offset = k - this->templayers.size();
							cont = true;
							break;
						}
						dirbinel = this->currbin->elements[el];
					}
					if (cont) break;
				}
				offset = 0;
				if (!cont) {
					for (int k = 0; k < this->templayers.size(); k++) {
						int offj = (int)((this->previ + offset + k) / 6);
						int el = ((this->previ + offset + k + 144) % 6) * 24 + this->prevj + offj;
						BinElement *dirbinel = this->currbin->elements[el];
						while (dirbinel->type == ELEM_DECK or dirbinel->type == ELEM_MIX) {
							offset++;
							offj = (int)((this->previ + offset + k) / 6);
							el = ((this->previ + offset + k + 144) % 6) * 24 + this->prevj + offj;
							dirbinel = this->currbin->elements[el];
						}
						dirbinel->tex = dirbinel->oldtex;
						glDeleteTextures(1, &this->inputtexes[k]);
						Layer *lay = this->templayers[k];
						delete lay;
					}
				}
				else {
					for (int k = 0; k < this->templayers.size(); k++) {
						int offj = (int)((143 - k + offset) / 6) - (143 - k + offset < 0);
						int el = ((143 - k + offset) % 6) * 24 + offj;
						BinElement *dirbinel = this->currbin->elements[el];
						while (dirbinel->type == ELEM_DECK or dirbinel->type == ELEM_MIX) {
							offset--;
							offj = (int)((143 - k + offset) / 6) - (143 - k + offset < 0);
							el = ((143 - k + offset) % 6) * 24 + offj;
							dirbinel = this->currbin->elements[el];
						}
						dirbinel->tex = dirbinel->oldtex;
						glDeleteTextures(1, &this->inputtexes[k]);
						Layer *lay = this->templayers[k];
						delete lay;
					}
				}
				this->currbinel = nullptr;
				this->inputtexes.clear();
				this->templayers.clear();
				mainprogram->rightmouse = false;
			}
		}
		if (lay->vidmoving) {
			this->currbinel->tex = this->inputbinel->tex;
			this->currbinel = nullptr;
			mainprogram->rightmouse = false;
		}
		else if (this->movingtex != -1) {
			bool temp = this->currbinel->full;
			this->currbinel->full = this->movingbinel->full;
			this->movingbinel->full = temp;
			this->currbinel->tex = this->movingbinel->tex;
			this->movingbinel->tex = this->movingtex;
			this->currbinel = nullptr;
			this->movingbinel = nullptr;
			this->movingtex = -1;
			mainprogram->rightmouse = false;
		}
	}
			
	//draw and handle binslist
	draw_box(red, black, -0.15f, -1.0f, 0.3f, 1.0f, -1);
	for (int i = 0; i < this->bins.size(); i++) {
		Bin *bin = this->bins[i];
		if (bin->box->in()) {
			if (mainprogram->leftmouse) {
				make_currbin(i);
			}
			bin->box->acolor[0] = 0.5f;
			bin->box->acolor[1] = 0.5f;
			bin->box->acolor[2] = 1.0f;
			bin->box->acolor[3] = 1.0f;
		}
		else if (i == this->currbin->pos) {
			bin->box->acolor[0] = 0.4f;
			bin->box->acolor[1] = 0.2f;
			bin->box->acolor[2] = 0.2f;
			bin->box->acolor[3] = 1.0f;
		}
		else {
			bin->box->acolor[0] = 0.0f;
			bin->box->acolor[1] = 0.0f;
			bin->box->acolor[2] = 0.0f;
			bin->box->acolor[3] = 1.0f;
		}
		draw_box(bin->box, -1);
		if (mainprogram->renaming != EDIT_NONE and bin == this->menubin) {
			std::string part = mainprogram->inputtext.substr(0, mainprogram->cursorpos);
			float textw = render_text(part, white, bin->box->vtxcoords->x1 + tf(0.01f), bin->box->vtxcoords->y1 + tf(0.012f), tf(0.0003f), tf(0.0005f));
			part = mainprogram->inputtext.substr(mainprogram->cursorpos, mainprogram->inputtext.length() - mainprogram->cursorpos);
			render_text(part, white, bin->box->vtxcoords->x1 + tf(0.01f) + textw * 2, bin->box->vtxcoords->y1 + tf(0.012f), tf(0.0003f), tf(0.0005f));
			draw_line(white, bin->box->vtxcoords->x1 + tf(0.011f) + textw * 2, bin->box->vtxcoords->y1 + tf(0.01f), bin->box->vtxcoords->x1 + tf(0.011f) + textw * 2, bin->box->vtxcoords->y1 + tf(0.028f)); 
		}
		else {
			render_text(bin->name, white, bin->box->vtxcoords->x1 + tf(0.01f), bin->box->vtxcoords->y1 + tf(0.012f), tf(0.0003f), tf(0.0005f));
		}
	}

	if (mainprogram->leftmouse and mainprogram->dragbinel) {
		mainprogram->leftmouse;
		enddrag();  //when dropping on grey area
	}

	Box *box = this->newbinbox;
	float newy = (this->bins.size() + 1) * -0.05f;
	box->vtxcoords->y1 = newy;
	box->upvtxtoscr();
	if (box->in()) {
		if (mainprogram->leftmouse) {
			std::string name = "new bin";
			std::string path;
			int count = 0;
			while (1) {
				path = mainprogram->binsdir + name + ".bin";
				if (!exists(path)) {
					new_bin(name);
					break;
				}
				count++;
				name = remove_version(name) + "_" + std::to_string(count);
			}
		}
		box->acolor[0] = 0.5f;
		box->acolor[1] = 0.5f;
		box->acolor[2] = 1.0f;
		box->acolor[3] = 1.0f;
	}
	else {
		box->acolor[0] = 0.0f;
		box->acolor[1] = 0.0f;
		box->acolor[2] = 0.0f;
		box->acolor[3] = 1.0f;
	}
	draw_box(box, -1);
	render_text("+ NEW BIN", red, 0.05f, newy + 0.018f, tf(0.0003f), tf(0.0005f));
	
	// Draw and handle binmenu and binelmenu	
	if (this->bins.size() > 1) {
		int k = handle_menu(mainprogram->binmenu);
		if (k == 0) {
			std::string path = mainprogram->binsdir + this->menubin->name + ".bin";
			boost::filesystem::remove(path);			
			if (this->currbin->name == this->menubin->name) {
				if (this->currbin->pos == 0) make_currbin(1);
				else make_currbin(this->currbin->pos - 1);
			}
			this->bins.erase(std::find(this->bins.begin(), this->bins.end(), this->menubin));
			delete this->menubin; //delete textures in destructor
			for (int i = 0; i < this->bins.size(); i++) {
				if (this->bins[i] == this->currbin) this->currbin->pos = i;
				this->bins[i]->box->vtxcoords->y1 = (i + 1) * -0.05f;
				this->bins[i]->box->upvtxtoscr();
			}
			this->save_binslist();
		}
		else if (k == 1) {
			mainprogram->backupname = this->menubin->name;
			mainprogram->inputtext = this->menubin->name;
			mainprogram->cursorpos = mainprogram->inputtext.length();
			SDL_StartTextInput();
			mainprogram->renaming = EDIT_BINNAME;
		}
	}
	else {
		int k = handle_menu(mainprogram->bin2menu);
		if (k == 0) {
			mainprogram->backupname = this->menubin->name;
			mainprogram->inputtext = this->menubin->name;
			mainprogram->cursorpos = mainprogram->inputtext.length();
			SDL_StartTextInput();
			mainprogram->renaming = EDIT_BINNAME;
		}
	}
	
	for (int i = 0; i < this->currbin->decks.size(); i++) {
		BinDeck *bd = this->currbin->decks[i];
		bd->box->vtxcoords->x1 = -0.965f + bd->i * 0.12f + (1.2f * (bd->j > 11));
		bd->box->vtxcoords->y1 = 0.92f - ((bd->j % 12) + bd->height) * 0.15f;
		bd->box->vtxcoords->h = 0.15f * bd->height;
		bd->box->upvtxtoscr();
		draw_box(bd->box, -1);
		if (!this->movingstruct) {
			if (bd->box->in()) {
				this->movingdeck = bd;
				if (mainprogram->menuactivation) {
					std::vector<std::string> binel;
					binel.push_back("Load in deck A");
					binel.push_back("Load in deck B");
					binel.push_back("Delete deck");
					binel.push_back("Insert file from disk");
					binel.push_back("Insert dir from disk");
					binel.push_back("Insert deck A");
					binel.push_back("Insert deck B");
					binel.push_back("Insert full mix");
					binel.push_back("HAP encode deck");
					binel.push_back("Quit");
					mainprogram->make_menu("binelmenu", mainprogram->binelmenu, binel);
					mainprogram->menuset = 3;
				}
				if (mainprogram->leftmouse) {
					if (this->dragdeck and this->dragdeck != bd) {
						this->dragdeck = nullptr;
					}
					else {
						this->movingstruct = true;
						this->inserting = 0;
						this->currbinel = this->currbin->elements[bd->j + bd->i * 24];
						this->previ = bd->i;
						this->prevj = bd->j;
						for (int j = 0; j < bd->height; j++) {
							for (int k = 0; k < 3; k++) {
								BinElement *deckbinel = this->currbin->elements[(bd->j + j) + ((bd->i + k) * 24)];
								glGenTextures(1, &deckbinel->oldtex);
								glBindTexture(GL_TEXTURE_2D, deckbinel->oldtex);
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
								glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB, (int)(mainprogram->ow3), (int)(mainprogram->oh3));
								this->inserttexes[0].push_back(deckbinel->tex);
								this->inserttypes[0].push_back(ELEM_DECK);
								this->insertpaths[0].push_back(deckbinel->path);
								this->insertjpegpaths[0].push_back(deckbinel->jpegpath);
							}
						}
					}
					mainprogram->leftmouse = false;
				}
				if (this->movingdeck) {
					if (this->movingdeck->encthreads) continue;
				}
				if (mainprogram->leftmousedown) {
					mainprogram->leftmousedown = false;
					this->dragdeck = bd;
					for (int j = 0; j < bd->height; j++) {
						for (int m = bd->i; m < bd->i + 3; m++) {
							BinElement *mixbinel = this->currbin->elements[(bd->j + j) + (m * 24)];
							this->dragtexes[0].push_back(mixbinel->tex);
						}
					}
				}
			}
		}
	}
	for (int i = 0; i < this->currbin->mixes.size(); i++) {
		BinMix *bm = this->currbin->mixes[i];
		bm->box->vtxcoords->x1 = -0.965f + (1.2f * (bm->j > 11));
		bm->box->vtxcoords->y1 = 0.92f - ((bm->j % 12) + bm->height) * 0.15f;
		bm->box->vtxcoords->h = 0.15f * bm->height;
		bm->box->upvtxtoscr();
		draw_box(bm->box, -1);
		if (!this->movingstruct) {
			if (bm->box->in()) {
				this->movingmix = bm;
				if (mainprogram->menuactivation) {
					std::vector<std::string> binel;
					binel.push_back("Load in mix");
					binel.push_back("Delete mix");
					binel.push_back("Insert file from disk");
					binel.push_back("Insert dir from disk");
					binel.push_back("Insert deck A");
					binel.push_back("Insert deck B");
					binel.push_back("Insert full mix");
					binel.push_back("HAP encode mix");
					binel.push_back("Quit");
					mainprogram->make_menu("binelmenu", mainprogram->binelmenu, binel);
					mainprogram->menuset = 2;
				}
				if (mainprogram->leftmouse) {
					if (this->dragmix and this->dragmix != bm) {
						this->dragmix = nullptr;
					}
					else {
						this->movingstruct = true;
						this->inserting = 2;
						this->currbinel = this->currbin->elements[bm->j];
						this->previ = 0;
						this->prevj = bm->j;
						this->prevbinel = this->currbinel;
						for (int j = 0; j < bm->height; j++) {
							for (int k = 0; k < 6; k++) {
								BinElement *mixbinel = this->currbin->elements[(bm->j + j) + (k * 24)];
								glGenTextures(1, &mixbinel->oldtex);
								glBindTexture(GL_TEXTURE_2D, mixbinel->oldtex);
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
								glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB, (int)(mainprogram->ow3), (int)(mainprogram->oh3));
								if (k < 3) this->inserttexes[0].push_back(mixbinel->tex);
								else this->inserttexes[1].push_back(mixbinel->tex);
								if (k < 3) this->inserttypes[0].push_back(ELEM_MIX);
								else this->inserttypes[1].push_back(mixbinel->type);
								if (k < 3) this->insertpaths[0].push_back(mixbinel->path);
								else this->insertpaths[1].push_back(mixbinel->path);
								if (k < 3) this->insertjpegpaths[0].push_back(mixbinel->jpegpath);
								else this->insertjpegpaths[1].push_back(mixbinel->jpegpath);
							}
						}
					}
					mainprogram->leftmouse = false;
				}
				if (this->movingmix) {
					if (this->movingmix->encthreads) continue;
				}
				if (mainprogram->leftmousedown) {
					mainprogram->leftmousedown = false;
					this->dragmix = bm;
					for (int j = 0; j < bm->height; j++) {
						for (int m = 0; m < 6; m++) {
							BinElement *mixbinel = this->currbin->elements[(bm->j + j) + (m * 24)];
							if (m < 3) this->dragtexes[0].push_back(mixbinel->tex);
							else this->dragtexes[1].push_back(mixbinel->tex);
						}
					}
				}
			}		
		}
	}
	if (mainprogram->menuset == 0 and mainprogram->menuactivation) {		
		std::vector<std::string> binel;
		binel.push_back("Insert file(s) from disk");
		binel.push_back("Insert dir from disk");
		binel.push_back("Insert deck A");
		binel.push_back("Insert deck B");
		binel.push_back("Insert full mix");
		binel.push_back("HAP encode entire bin");
		binel.push_back("Quit");
		mainprogram->make_menu("binelmenu", mainprogram->binelmenu, binel);
	}
	
	int k = handle_menu(mainprogram->binelmenu);
	//if (k > -1) this->currbinel = nullptr;
	if (k == 0 and mainprogram->menuset == 1) {
		this->movingtex = this->menubinel->tex;
		this->movingbinel = this->menubinel;
		mainprogram->del = true;
	}
	else if (k == 0 and mainprogram->menuset == 3) {
		mainprogram->binsscreen = false;
		mainmix->mousedeck = 0;
		mainmix->open_deck(this->movingdeck->path, 1);
	}
	else if (k == 1 and mainprogram->menuset == 3) {
		mainprogram->binsscreen = false;
		mainmix->mousedeck = 1;
		mainmix->open_deck(this->movingdeck->path, 1);
	}
	else if (k == 2 and mainprogram->menuset == 3) {
		for (int j = 0; j < this->movingdeck->height; j++) {
			for (int m = 0; m < 3; m++) {
				BinElement *deckbinel = this->currbin->elements[(this->movingdeck->j + j) + ((this->movingdeck->i + m) * 24)];
				this->inserttexes[0].push_back(deckbinel->tex);
				this->insertpaths[0].push_back(deckbinel->path);
				this->insertjpegpaths[0].push_back(deckbinel->jpegpath);
			}
		}
		this->movingstruct = true;
		this->inserting = 0;
		mainprogram->del = true;
	}
	else if (k == 0 and mainprogram->menuset == 2) {
		mainprogram->binsscreen = false;
		mainmix->open_mix(this->movingmix->path.c_str());
	}
	else if (k == 1 and mainprogram->menuset == 2) {
		for (int j = 0; j < this->movingmix->height; j++) {
			for (int m = 0; m < 6; m++) {
				BinElement *mixbinel = this->currbin->elements[(this->movingmix->j + j) + (m * 24)];
				glGenTextures(1, &mixbinel->oldtex);
				glBindTexture(GL_TEXTURE_2D, mixbinel->oldtex);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB, (int)(mainprogram->ow3), (int)(mainprogram->oh3));
				if (m < 3) this->inserttexes[0].push_back(mixbinel->tex);
				else this->inserttexes[1].push_back(mixbinel->tex);
				if (m < 3) this->inserttypes[0].push_back(mixbinel->type);
				else this->inserttypes[1].push_back(mixbinel->type);
				if (m < 3) this->insertpaths[0].push_back(mixbinel->path);
				else this->insertpaths[1].push_back(mixbinel->path);
				if (m < 3) this->insertjpegpaths[0].push_back(mixbinel->jpegpath);
				else this->insertjpegpaths[1].push_back(mixbinel->jpegpath);
			}
		}
		this->movingstruct = true;
		this->inserting = 2;
		mainprogram->del = true;
	}
	else if (k == mainprogram->menuset) {
		mainprogram->pathto = "OPENBINFILES";
		std::thread filereq (&Program::get_multinname, mainprogram, "Open video and/or image files", boost::filesystem::canonical(mainprogram->currvideodir).generic_string());
		filereq.detach();
	}
	else if (k == mainprogram->menuset + 1) {
		mainprogram->pathto = "OPENBINDIR";
		mainprogram->blocking = true;
		std::thread filereq (&Program::get_dir, mainprogram, "Open video/image file directory", boost::filesystem::canonical(mainprogram->currbindirdir).generic_string());
		filereq.detach();
	}
	else if (k == mainprogram->menuset + 2) {
		this->inserting = 0;
		this->inserttexes[0].clear();
		get_texes(0);
	}
	else if (k == mainprogram->menuset + 3) {
		this->inserting = 1;
		this->inserttexes[1].clear();
		get_texes(1);
	}
	else if (k == mainprogram->menuset + 4) {
		this->inserting = 2;
		this->inserttexes[0].clear();
		get_texes(0);
		this->inserttexes[1].clear();
		get_texes(1);
	}
	else if (k == 6 and mainprogram->menuset == 1) {
		this->hap_binel(this->menubinel, nullptr, nullptr);
	}
	else if (k == 8 and mainprogram->menuset == 3) {
		this->hap_deck(this->movingdeck);
	}	
	else if (k == 7 and mainprogram->menuset == 2) {
		this->hap_mix(this->movingmix);
	}	
	else if (k == 5 and mainprogram->menuset == 0) {
		for (int i = 0; i < 6; i++) {
			for (int j = 0; j < 24; j++) {
				BinElement *binel = this->currbin->elements[i * 24 + j];
				if (binel->full and (binel->type == ELEM_FILE or binel->type == ELEM_LAYER)) {
					this->hap_binel(binel, nullptr, nullptr);
				}
			}
		}
		for (int i = 0; i < this->currbin->decks.size(); i++) {
			BinDeck *bd = this->currbin->decks[i];
			this->hap_deck(bd);
		}
		for (int i = 0; i < this->currbin->mixes.size(); i++) {
			BinMix *bm = this->currbin->mixes[i];
			this->hap_mix(bm);
		}
	}
	else if (k == mainprogram->binelmenu->entries.size() - 1) {
		mainprogram->quit("quitted");
	}
	
	if (mainprogram->menuchosen) {
		mainprogram->menuchosen = false;
		mainprogram->leftmouse = 0;
		mainprogram->menuactivation = 0;
		mainprogram->menuresults.clear();
		mainprogram->menuondisplay = false;
	}
	
	if (mainprogram->menuactivation and !this->templayers.size() and this->inserting == -1 and !lay->vidmoving and this->movingtex == -1) {
		int dt = 0;
		int mt = 0;
		if (this->movingdeck and mainprogram->menuset == 3) {
			dt = this->movingdeck->encthreads;
		}
		else if (this->movingmix and mainprogram->menuset == 2) {
			mt = this->movingmix->encthreads;
		}
		if (dt == 0 and mt == 0) {
			this->menubin = nullptr;
			for (int i = 0; i < this->bins.size(); i++) {
				if (this->bins[i]->box->in()) {
					this->menubin = this->bins[i];
					break;
				}
			}
			mainprogram->menuondisplay = true;
			if (this->menubin) {
				mainprogram->binmenu->state = 2;
				mainprogram->bin2menu->state = 2;
			}
			else {
				mainprogram->binelmenu->state = 2;
				mainprogram->leftmousedown = false;
			}
		}
		mainprogram->menuactivation = false;
		mainprogram->rightmouse = false;
	}
	
	//handle binelements
	if (this->openbindir) {
		open_bindir();
	}
	else if (this->openbinfile) {
		open_binfiles();
	}
	else if (!mainprogram->menuondisplay) {
		for (int j = 0; j < 24; j++) {
			for (int i = 0; i < 6; i++) {
				Box *box = this->elemboxes[i * 24 + j];
				box->upvtxtoscr();
				BinElement *binel = this->currbin->elements[i * 24 + j];
				if (binel->encwaiting) {
					render_text("Waiting...", white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.015f), 0.0005f, 0.0008f);
				}
				else if (binel->encoding) {
					render_text("Encoding...", white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.015f), 0.0005f, 0.0008f);
					draw_box(black, white, box->vtxcoords->x1, box->vtxcoords->y1 + box->vtxcoords->h - tf(0.045f), binel->encodeprogress * 0.1f, 0.02f, -1);
				}
				if ((box->in() or mainprogram->rightmouse) and !binel->encoding) {
					if (this->currbin->encthreads) continue;
					if (this->movingdeck) {
						if (this->movingdeck->encthreads) continue;
					}
					else if (this->movingmix) {
						if (this->movingmix->encthreads) continue;
					}
					if (!binel->encwaiting and !binel->encoding and binel->full and !this->templayers.size() and this->inserting == -1 and !lay->vidmoving) {
						if ((binel->type == ELEM_LAYER or binel->type == ELEM_DECK or binel->type == ELEM_MIX) and !this->binpreview) {
							if (remove_extension(basename(binel->path)) != "") {
								if (mainprogram->prelay) {
									mainprogram->prelay->closethread = true;
									while (mainprogram->prelay->closethread) {
										mainprogram->prelay->ready = true;
										mainprogram->prelay->startdecode.notify_one();
									}
									//delete mainprogram->prelay;
								}
								this->binpreview = true;
								draw_box(red, black, -0.2f, 0.5f, 0.4f, 0.4f, -1);
								mainprogram->prelay = new Layer(false);
								mainprogram->prelay->dummy = true;
								mainprogram->prelay->pos = 0;
								mainprogram->prelay->node = mainprogram->nodesmain->currpage->add_videonode(2);
								mainprogram->prelay->node->layer = mainprogram->prelay;
								mainprogram->prelay->lasteffnode[0] = mainprogram->prelay->node;
								mainprogram->prelay->lasteffnode[1] = mainprogram->prelay->node;
								mainmix->open_layerfile(binel->path, mainprogram->prelay, true, 0);
								mainprogram->prelay->node->calc = true;
								mainprogram->prelay->node->walked = false;
								mainprogram->prelay->playbut->value = false;
								mainprogram->prelay->revbut->value = false;
								mainprogram->prelay->bouncebut->value = false;
								for (int k = 0; k < mainprogram->prelay->effects[0].size(); k++) {
									mainprogram->prelay->effects[0][k]->node->calc = true;
									mainprogram->prelay->effects[0][k]->node->walked = false;
								}
								if (mainprogram->prelay->type != ELEM_IMAGE) {
									mainprogram->prelay->frame = 0.0f;
									mainprogram->prelay->prevframe = -1;
									mainprogram->prelay->ready = true;
									while (mainprogram->prelay->ready) {
										mainprogram->prelay->startdecode.notify_one();
									}
									std::unique_lock<std::mutex> lock(mainprogram->prelay->enddecodelock);
									mainprogram->prelay->enddecodevar.wait(lock, [&]{return mainprogram->prelay->processed;});
									mainprogram->prelay->processed = false;
									lock.unlock();
									mainprogram->prelay->initialized = true;
									glActiveTexture(GL_TEXTURE0);
									glBindTexture(GL_TEXTURE_2D, mainprogram->prelay->texture);
									if (mainprogram->prelay->vidformat == 188 or mainprogram->prelay->vidformat == 187) {
										if (mainprogram->prelay->decresult->compression == 187) {
											glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, mainprogram->prelay->decresult->width, mainprogram->prelay->decresult->height, 0, mainprogram->prelay->decresult->size, mainprogram->prelay->decresult->data);
										}
										else if (mainprogram->prelay->decresult->compression == 190) {
											glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, mainprogram->prelay->decresult->width, mainprogram->prelay->decresult->height, 0, mainprogram->prelay->decresult->size, mainprogram->prelay->decresult->data);
										}
									}
									else {
										glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, mainprogram->prelay->decresult->width, mainprogram->prelay->decresult->height, 0, GL_BGRA, GL_UNSIGNED_BYTE, mainprogram->prelay->decresult->data);
									}
								}
								mainprogram->prelay->initialized = true;
								mainprogram->prelay->fbotex = copy_tex(mainprogram->prelay->texture);
								onestepfrom(0, mainprogram->prelay->node, nullptr, -1, -1);
								if (mainprogram->prelay->effects[0].size()) {
									draw_box(red, black, -0.2f, 0.5f, 0.4f, 0.4f, mainprogram->prelay->effects[0][mainprogram->prelay->effects[0].size() - 1]->fbotex);
								}
								else {
									draw_box(red, black, -0.2f, 0.5f, 0.4f, 0.4f, mainprogram->prelay->fbotex);
								}
								if (!binel->encoding) {
									if (mainprogram->prelay->vidformat == 188 or mainprogram->prelay->vidformat == 187) {
										render_text("HAP", white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.015f), 0.0005f, 0.0008f);
									}
									else if (mainprogram->prelay->type != ELEM_IMAGE) {
										render_text("CPU", white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.015f), 0.0005f, 0.0008f);
									}
									else {
										render_text("IMAGE", white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.015f), 0.0005f, 0.0008f);
									}
									render_text(std::to_string(mainprogram->prelay->decresult->width) + "x" + std::to_string(mainprogram->prelay->decresult->height), white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.045f), 0.0005f, 0.0008f);
								}
							}
						}
						else if (binel->type == ELEM_LAYER or binel->type == ELEM_DECK or binel->type == ELEM_MIX) {
							draw_box(red, black, -0.2f, 0.5f, 0.4f, 0.4f, -1);
							if (mainprogram->mousewheel) {
								mainprogram->prelay->frame += mainprogram->mousewheel * (mainprogram->prelay->numf / 32.0f);
								mainprogram->mousewheel = 0.0f;
								if (mainprogram->prelay->frame < 0.0f) {
									mainprogram->prelay->frame = mainprogram->prelay->numf - 1.0f;
								}
								else if (mainprogram->prelay->frame > mainprogram->prelay->numf - 1) {
									mainprogram->prelay->frame = 0.0f;
								}
								//mainprogram->prelay->prevframe = -1;
								mainprogram->prelay->node->calc = true;
								mainprogram->prelay->node->walked = false;
								for (int k = 0; k < mainprogram->prelay->effects[0].size(); k++) {
									mainprogram->prelay->effects[0][k]->node->calc = true;
									mainprogram->prelay->effects[0][k]->node->walked = false;
								}
								if (mainprogram->prelay->type != ELEM_IMAGE) {
									mainprogram->prelay->ready = true;
									while (mainprogram->prelay->ready) {
										mainprogram->prelay->startdecode.notify_one();
									}
									std::unique_lock<std::mutex> lock(mainprogram->prelay->enddecodelock);
									mainprogram->prelay->enddecodevar.wait(lock, [&]{return mainprogram->prelay->processed;});
									mainprogram->prelay->processed = false;
									lock.unlock();
									glBindTexture(GL_TEXTURE_2D, mainprogram->prelay->texture);
									if (mainprogram->prelay->vidformat == 188 or mainprogram->prelay->vidformat == 187) {
										if (mainprogram->prelay->decresult->compression == 187) {
											glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, mainprogram->prelay->decresult->width, mainprogram->prelay->decresult->height, 0, mainprogram->prelay->decresult->size, mainprogram->prelay->decresult->data);
										}
										else if (mainprogram->prelay->decresult->compression == 190) {
											glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, mainprogram->prelay->decresult->width, mainprogram->prelay->decresult->height, 0, mainprogram->prelay->decresult->size, mainprogram->prelay->decresult->data);
										}
									}
									else {
										glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, mainprogram->prelay->decresult->width, mainprogram->prelay->decresult->height, 0, GL_BGRA, GL_UNSIGNED_BYTE, mainprogram->prelay->decresult->data);
									}
								}
								mainprogram->prelay->fbotex = copy_tex(mainprogram->prelay->texture);
								onestepfrom(0, mainprogram->prelay->node, nullptr, -1, -1);
								if (mainprogram->prelay->effects[0].size()) {
									draw_box(red, black, -0.2f, 0.5f, 0.4f, 0.4f, mainprogram->prelay->effects[0][mainprogram->prelay->effects[0].size() - 1]->fbotex);
								}
								else {
									draw_box(red, black, -0.2f, 0.5f, 0.4f, 0.4f, mainprogram->prelay->fbotex);
								}
							}
							else {
								if (mainprogram->prelay->effects[0].size()) {
									draw_box(red, black, -0.2f, 0.5f, 0.4f, 0.4f, mainprogram->prelay->effects[0][mainprogram->prelay->effects[0].size() - 1]->fbotex);
								}
								else {
									draw_box(red, black, -0.2f, 0.5f, 0.4f, 0.4f, mainprogram->prelay->fbotex);
								}
							}
							if (!binel->encoding and remove_extension(basename(binel->path)) != "") {
								if (mainprogram->prelay->vidformat == 188 or mainprogram->prelay->vidformat == 187) {
									render_text("HAP", white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.015f), 0.0005f, 0.0008f);
								}
								else if (mainprogram->prelay->type != ELEM_IMAGE) {
									render_text("CPU", white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.015f), 0.0005f, 0.0008f);
								}
								else {
									render_text("IMAGE", white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.015f), 0.0005f, 0.0008f);
								}
								render_text(std::to_string(mainprogram->prelay->decresult->width) + "x" + std::to_string(mainprogram->prelay->decresult->height), white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.045f), 0.0005f, 0.0008f);
							}
						}
						else if ((binel->type == ELEM_FILE or binel->type == ELEM_IMAGE) and !this->binpreview) {
							if (remove_extension(basename(binel->path)) != "") {
								if (mainprogram->prelay) {
									mainprogram->prelay->closethread = true;
									while (mainprogram->prelay->closethread) {
										mainprogram->prelay->ready = true;
										mainprogram->prelay->startdecode.notify_one();
									}
									//delete mainprogram->prelay;
								}
								this->binpreview = true;
								draw_box(red, black, -0.2f, 0.5f, 0.4f, 0.4f, -1);
								mainprogram->prelay = new Layer(true);
								mainprogram->prelay->dummy = true;
								if (binel->type == ELEM_FILE) {
									open_video(1, mainprogram->prelay, binel->path, true);
									std::unique_lock<std::mutex> olock(mainprogram->prelay->endopenlock);
									mainprogram->prelay->endopenvar.wait(olock, [&]{return mainprogram->prelay->opened;});
									mainprogram->prelay->opened = false;
									olock.unlock();
									mainprogram->prelay->initialized = true;
									mainprogram->prelay->frame = 0.0f;
									mainprogram->prelay->prevframe = -1;
									mainprogram->prelay->ready = true;
									while (mainprogram->prelay->ready) {
										mainprogram->prelay->startdecode.notify_one();
									}
									std::unique_lock<std::mutex> lock(mainprogram->prelay->enddecodelock);
									mainprogram->prelay->enddecodevar.wait(lock, [&]{return mainprogram->prelay->processed;});
									mainprogram->prelay->processed = false;
									lock.unlock();
									glBindTexture(GL_TEXTURE_2D, this->binelpreviewtex);
									if (mainprogram->prelay->vidformat == 188 or mainprogram->prelay->vidformat == 187) {
										if (mainprogram->prelay->decresult->compression == 187) {
											glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, mainprogram->prelay->decresult->width, mainprogram->prelay->decresult->height, 0, mainprogram->prelay->decresult->size, mainprogram->prelay->decresult->data);
										}
										else if (mainprogram->prelay->decresult->compression == 190) {
											glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, mainprogram->prelay->decresult->width, mainprogram->prelay->decresult->height, 0, mainprogram->prelay->decresult->size, mainprogram->prelay->decresult->data);
										}
									}
									else {
										glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, mainprogram->prelay->decresult->width, mainprogram->prelay->decresult->height, 0, GL_BGRA, GL_UNSIGNED_BYTE, mainprogram->prelay->decresult->data);
									}
									draw_box(red, black, -0.2f, 0.5f, 0.4f, 0.4f, this->binelpreviewtex);
								}
								else if (binel->type == ELEM_IMAGE) {
									mainprogram->prelay->open_image(binel->path);
									draw_box(red, black, -0.2f, 0.5f, 0.4f, 0.4f, lay->texture);
								}
								if (!binel->encoding) {
									if (mainprogram->prelay->vidformat == 188 or mainprogram->prelay->vidformat == 187) {
										render_text("HAP", white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.015f), 0.0005f, 0.0008f);
									}
									else {
										if (mainprogram->prelay->type == ELEM_FILE) {
											render_text("CPU", white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.015f), 0.0005f, 0.0008f);
										}
										if (mainprogram->prelay->type == ELEM_IMAGE) {
											render_text("IMAGE", white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.015f), 0.0005f, 0.0008f);
										}
									}
									render_text(std::to_string(mainprogram->prelay->decresult->width) + "x" + std::to_string(mainprogram->prelay->decresult->height), white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.045f), 0.0005f, 0.0008f);
								}
							}
						}
						else if (binel->type == ELEM_FILE or binel->type == ELEM_IMAGE) {
							draw_box(red, black, -0.2f, 0.5f, 0.4f, 0.4f, -1);
							if (mainprogram->prelay->type != ELEM_IMAGE) {
								if (mainprogram->mousewheel) {
									mainprogram->prelay->frame += mainprogram->mousewheel * (mainprogram->prelay->numf / 32.0f);
									mainprogram->mousewheel = 0.0f;
									if (mainprogram->prelay->frame < 0.0f) {
										mainprogram->prelay->frame = mainprogram->prelay->numf - 1.0f;
									}
									else if (mainprogram->prelay->frame > mainprogram->prelay->numf - 1) {
										mainprogram->prelay->frame = 0.0f;
									}
									//mainprogram->prelay->prevframe = -1;
									mainprogram->prelay->ready = true;
									while (mainprogram->prelay->ready) {
										mainprogram->prelay->startdecode.notify_one();
									}
									std::unique_lock<std::mutex> lock(mainprogram->prelay->enddecodelock);
									mainprogram->prelay->enddecodevar.wait(lock, [&]{return mainprogram->prelay->processed;});
									mainprogram->prelay->processed = false;
									lock.unlock();
									glBindTexture(GL_TEXTURE_2D, this->binelpreviewtex);
									if (mainprogram->prelay->vidformat == 188 or mainprogram->prelay->vidformat == 187) {
										if (mainprogram->prelay->decresult->compression == 187) {
											glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, mainprogram->prelay->decresult->width, mainprogram->prelay->decresult->height, 0, mainprogram->prelay->decresult->size, mainprogram->prelay->decresult->data);
										}
										else if (mainprogram->prelay->decresult->compression == 190) {
											glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, mainprogram->prelay->decresult->width, mainprogram->prelay->decresult->height, 0, mainprogram->prelay->decresult->size, mainprogram->prelay->decresult->data);
										}
									}
									else {
										glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, mainprogram->prelay->decresult->width, mainprogram->prelay->decresult->height, 0, GL_BGRA, GL_UNSIGNED_BYTE, mainprogram->prelay->decresult->data);
									}
									draw_box(red, black, -0.2f, 0.5f, 0.4f, 0.4f, this->binelpreviewtex);
								}
								else {
									draw_box(red, black, -0.2f, 0.5f, 0.4f, 0.4f, this->binelpreviewtex);
								}
							}
							else {
								draw_box(red, black, -0.2f, 0.5f, 0.4f, 0.4f, mainprogram->prelay->texture);
							}
							if (!binel->encoding and remove_extension(basename(binel->path)) != "") {
								if (mainprogram->prelay->vidformat == 188 or mainprogram->prelay->vidformat == 187) {
									render_text("HAP", white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.015f), 0.0005f, 0.0008f);
								}
								else {
									if (mainprogram->prelay->type == ELEM_FILE) {
										render_text("CPU", white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.015f), 0.0005f, 0.0008f);
									}
									if (mainprogram->prelay->type == ELEM_IMAGE) {
										render_text("IMAGE", white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.015f), 0.0005f, 0.0008f);
									}
								}
								render_text(std::to_string(mainprogram->prelay->decresult->width) + "x" + std::to_string(mainprogram->prelay->decresult->height), white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.045f), 0.0005f, 0.0008f);
							}
						}
					}
					if (binel->type != ELEM_DECK and binel->type != ELEM_MIX and binel->path != "") {
						if (this->inserting == -1 and !this->templayers.size() and mainprogram->leftmousedown and !mainprogram->dragbinel) {
							mainprogram->dragbinel = binel;
							this->dragtex = binel->tex;
							mainprogram->leftmousedown = false;
						}
					}
					if (binel != this->currbinel or mainprogram->rightmouse) {
						if (this->currbinel) this->binpreview = false;
						if (this->inserting == 0 or this->inserting == 1) {
							if (this->prevbinel) {
								for (int k = 0; k < this->inserttexes[deck].size(); k++) {
									BinElement *deckbinel = find_element(this->inserttexes[deck].size(), k, this->previ, this->prevj, 1);
									if (!deckbinel) {
										this->inserting = -1;
										break;
									}
									if (!this->movingstruct) {
										std::vector<Layer*> &lvec = choose_layers(deck);
										if (lvec[k]->filename != "") {
											deckbinel->tex = deckbinel->oldtex;
										}
									}
									else {
										deckbinel->tex = deckbinel->oldtex;
									}
									if (this->movingstruct) {
										deckbinel->path = deckbinel->oldpath;
										deckbinel->jpegpath = deckbinel->oldjpegpath;
										deckbinel->type = deckbinel->oldtype;
									}
								}
							}
							for (int k = 0; k < this->inserttexes[deck].size(); k++) {
								int newi = i;
								if (mainprogram->rightmouse) newi = this->movingdeck->i;
								int newj = j;
								if (mainprogram->rightmouse) newj = this->movingdeck->j;
								BinElement *deckbinel = find_element(this->inserttexes[deck].size(), k, newi, newj, 1);
								if (!deckbinel) {
									this->inserting = -1;
									break;
								}
								if (!this->movingstruct) {
									std::vector<Layer*> &lvec = choose_layers(deck);
									if (lvec[k]->filename != "") {
										deckbinel->oldtex = deckbinel->tex;
										deckbinel->tex = this->inserttexes[deck][k];
									}
								}
								else {
									deckbinel->oldtex = deckbinel->tex;
									deckbinel->tex = this->inserttexes[deck][k];
								}
								//if (mainprogram->rightmouse) glDeleteTextures(1, &deckbinel->oldtex);
								if (this->movingstruct) {
									deckbinel->oldtype = deckbinel->type;
									deckbinel->type = this->inserttypes[deck][k];
									deckbinel->oldpath = deckbinel->path;
									deckbinel->path = this->insertpaths[deck][k];
									deckbinel->oldjpegpath = deckbinel->jpegpath;
									deckbinel->jpegpath = this->insertjpegpaths[deck][k];
									int pos = std::distance(this->currbin->elements.begin(), std::find(this->currbin->elements.begin(), this->currbin->elements.end(), deckbinel));
									int ii = (int)((int)(pos / 24) / 3) * 3;
									int jj = pos % 24;
									BinDeck *bd = this->movingdeck;
									if (jj < bd->j or jj > bd->j + bd->height or ii != bd->i) {
										int ii = bd->i + k % 3;
										int jj = bd->j + (int)(k / 3);
										BinElement *elem = this->currbin->elements[ii * 24 + jj];
										elem->tex = deckbinel->oldtex;
										elem->path = deckbinel->oldpath;
										elem->jpegpath = deckbinel->oldjpegpath;
										elem->type = deckbinel->oldtype;
									}
								}
							}
							if (mainprogram->rightmouse) {
								this->movingstruct = false;
								this->inserttexes[0].clear();
								this->inserttexes[1].clear();
								this->newpaths.clear();
								this->inserting = -1;
								this->currbinel = nullptr;
								mainprogram->rightmouse = false;
								break;
							}
							this->prevbinel = binel;
							this->previ = i;
							this->prevj = j;
						}
						if (this->inserting == 2) {
							int size = std::max(((int)((this->inserttexes[0].size() - 1) / 3) * 3 + 3), ((int)((this->inserttexes[1].size() - 1) / 3) * 3 + 3));
							if (this->prevbinel) {
								for (int k = 0; k < this->inserttexes[0].size(); k++) {
									int newj = this->prevj;
									if (this->prevj > 23 - (int)((size - 1) / 3)) newj = this->prevj - (int)((size - 1) / 3);
									BinElement *deckbinel = find_element(size, k, 0, newj, 1);
									if (!deckbinel) {
										this->inserting = -1;
										break;
									}
									if (!this->movingstruct) {
										std::vector<Layer*> &lvec = choose_layers(0);
										if (lvec[k]->filename != "") {
											deckbinel->tex = deckbinel->oldtex;
										}
									}
									else {
										deckbinel->tex = deckbinel->oldtex;
									}
									if (this->movingstruct) {
										deckbinel->path = deckbinel->oldpath;
										deckbinel->jpegpath = deckbinel->oldjpegpath;
										deckbinel->type = deckbinel->oldtype;
									}
								}
								for (int k = 0; k < this->inserttexes[1].size(); k++) {
									BinElement *deckbinel = find_element(size, k, 3, this->prevj, 1);
									if (!deckbinel) {
										this->inserting = -1;
										break;
									}
									if (!this->movingstruct) {
										std::vector<Layer*> &lvec = choose_layers(1);
										if (lvec[k]->filename != "") {
											deckbinel->tex = deckbinel->oldtex;
										}
									}
									else {
										deckbinel->tex = deckbinel->oldtex;
									}
									if (this->movingstruct) {
										deckbinel->path = deckbinel->oldpath;
										deckbinel->jpegpath = deckbinel->oldjpegpath;
										deckbinel->type = deckbinel->oldtype;
									}
								}
							}
							
							for (int m = 0; m < 2; m++) {
								for (int k = 0; k < this->inserttexes[m].size(); k++) {
									int newj = j;
									if (j > 23 - (int)((size - 1) / 3)) newj = j - (int)((size - 1) / 3);
									if (mainprogram->rightmouse) newj = this->movingmix->j;
									BinElement *deckbinel = find_element(size, k, m * 3, newj, 1);
									if (!deckbinel) {
										this->inserting = -1;
										break;
									}
									if (!this->movingstruct) {
										std::vector<Layer*> &lvec = choose_layers(m);
										if (lvec[k]->filename != "") {
											deckbinel->oldtex = deckbinel->tex;
											deckbinel->tex = this->inserttexes[m][k];
										}
									}
									else {
										deckbinel->oldtex = deckbinel->tex;
										deckbinel->tex = this->inserttexes[m][k];
									}
									//if (mainprogram->rightmouse) glDeleteTextures(1, &deckbinel->oldtex);
									if (this->movingstruct) {
										deckbinel->oldtype = deckbinel->type;
										deckbinel->type = this->inserttypes[m][k];
										deckbinel->oldpath = deckbinel->path;
										deckbinel->path = this->insertpaths[m][k];
										deckbinel->oldjpegpath = deckbinel->jpegpath;
										deckbinel->jpegpath = this->insertjpegpaths[m][k];
										int pos = std::distance(this->currbin->elements.begin(), std::find(this->currbin->elements.begin(), this->currbin->elements.end(), deckbinel));
										int jj = pos % 24;
										BinMix *bm = this->movingmix;
										if (jj < bm->j and jj >= bm->j + bm->height) {
											int ii = k % 3 + m * 3;
											int jj = bm->j + (int)(k / 3);
											BinElement *elem = this->currbin->elements[ii * 24 + jj];
											elem->tex = deckbinel->oldtex;
											elem->path = deckbinel->oldpath;
											elem->jpegpath = deckbinel->oldjpegpath;
											elem->type = deckbinel->oldtype;
										}
									}
								}
							}
							if (mainprogram->rightmouse) {
								this->movingstruct = false;
								this->inserttexes[0].clear();
								this->inserttexes[1].clear();
								this->newpaths.clear();
								this->inserting = -1;
								this->currbinel = nullptr;
								mainprogram->rightmouse = false;
								break;
							}
							this->prevbinel = binel;
							this->previ = i;
							this->prevj = j;
						}
						
						if (lay->vidmoving) {
							if (this->currbinel) {
								if (binel->type != ELEM_DECK and binel->type != ELEM_MIX) {
									GLuint temp;
									temp = this->currbinel->tex;
									this->currbinel->tex = this->inputbinel->tex;
									this->inputbinel->tex = binel->tex;
									binel->tex = temp;
									std::string path = mainprogram->binsdir + this->currbin->name + ".bin";
									save_bin(path);
								}
								else break;
							}
							else {
								if (lay->vidmoving) {
									binel->tex = this->dragtex;
								}
							}
							binel->full = true;
							this->currbinel = binel;
						}						
						if (this->templayers.size()) {
							bool cont = false;
							if (this->prevbinel) {
								int offset = 0;
								for (int k = 0; k < this->templayers.size(); k++) {
									int offj = (int)((this->previ + offset + k) / 6);
									int el = ((this->previ + offset + k + 144) % 6) * 24 + this->prevj + offj;
									if (this->prevj + offj > 23) {
										offset = k - this->templayers.size();
										cont = true;
										break;
									}
									BinElement *dirbinel = this->currbin->elements[el];
									while (dirbinel->type == ELEM_DECK or dirbinel->type == ELEM_MIX) {
										offset++;
										offj = (int)((this->previ + offset + k) / 6);
										el = ((this->previ + offset + k + 144) % 6) * 24 + this->prevj + offj;
										if (this->prevj + offj > 23) {
											offset = k - this->templayers.size();
											cont = true;
											break;
										}
										dirbinel = this->currbin->elements[el];
									}
									if (cont) break;
								}
								offset = 0;
								if (!cont) {
									for (int k = 0; k < this->templayers.size(); k++) {
										int offj = (int)((this->previ + offset + k) / 6);
										int el = ((this->previ + offset + k + 144) % 6) * 24 + this->prevj + offj;
										BinElement *dirbinel = this->currbin->elements[el];
										while (dirbinel->type == ELEM_DECK or dirbinel->type == ELEM_MIX) {
											offset++;
											offj = (int)((this->previ + offset + k) / 6);
											el = ((this->previ + offset + k + 144) % 6) * 24 + this->prevj + offj;
											dirbinel = this->currbin->elements[el];
										}
										dirbinel->tex = dirbinel->oldtex;
									}
								}
								else {
									for (int k = 0; k < this->templayers.size(); k++) {
										int offj = (int)((143 - k + offset) / 6) - (143 - k + offset < 0);
										int el = ((143 - k + offset) % 6) * 24 + offj;
										BinElement *dirbinel = this->currbin->elements[el];
										while (dirbinel->type == ELEM_DECK or dirbinel->type == ELEM_MIX) {
											offset--;
											offj = (int)((143 - k + offset) / 6) - (143 - k + offset < 0);
											el = ((143 - k + offset) % 6) * 24 + offj;
											dirbinel = this->currbin->elements[el];
										}
										dirbinel->tex = dirbinel->oldtex;
									}
								}
							}
							int offset = 0;
							cont = false;
							for (int k = 0; k < this->templayers.size(); k++) {
								int offj = (int)((i + offset + k) / 6) - (i + offset + k < 0);
								int el = ((i + offset + k + 144) % 6) * 24 + j + offj;
								if (j + offj > 23) {
									offset = k - this->templayers.size();
									cont = true;
									break;
								}
								BinElement *dirbinel = this->currbin->elements[el];
								while (dirbinel->type == ELEM_DECK or dirbinel->type == ELEM_MIX) {
									offset++;
									offj = (int)((i + offset + k) / 6) - (i + offset + k < 0);
									el = ((i + offset + k + 144) % 6) * 24 + j + offj;
									if (j + offj > 23) {
										offset = k - this->templayers.size();
										cont = true;
										break;
									}
									dirbinel = this->currbin->elements[el];
								}
								if (cont) break;
							}
							offset = 0;
							if (!cont) {
								for (int k = 0; k < this->templayers.size(); k++) {
									int offj = (int)((i + offset + k) / 6) - (i + offset + k < 0);
									int el = ((i + offset + k + 144) % 6) * 24 + j + offj;
									BinElement *dirbinel = this->currbin->elements[el];
									while (dirbinel->type == ELEM_DECK or dirbinel->type == ELEM_MIX) {
										offset++;
										offj = (int)((i + offset + k) / 6) - (i + offset + k < 0);
										el = ((i + offset + k + 144) % 6) * 24 + j + offj;
										dirbinel = this->currbin->elements[el];
									}
									dirbinel->oldtex = dirbinel->tex;
									dirbinel->tex = this->inputtexes[k];
								}
							}
							else {
								for (int k = 0; k < this->templayers.size(); k++) {
									int offj = (int)((143 - k + offset) / 6) - (143 - k + offset < 0);
									int el = ((143 - k + offset) % 6) * 24 + offj;
									BinElement *dirbinel = this->currbin->elements[el];
									while (dirbinel->type == ELEM_DECK or dirbinel->type == ELEM_MIX) {
										offset--;
										offj = (int)((143 - k + offset) / 6) - (143 - k + offset < 0);
										el = ((143 - k + offset) % 6) * 24 + offj;
										dirbinel = this->currbin->elements[el];
									}
									dirbinel->oldtex = dirbinel->tex;
									dirbinel->tex = this->inputtexes[this->templayers.size() - k - 1];
								}
							}
							this->prevbinel = binel;
							this->previ = i;
							this->prevj = j;
						}
					}
					if (mainprogram->leftmouse) {
						enddrag();
						if (this->movingtex != -1) {
							std::string temp1 = this->currbinel->path;
							std::string temp3 = this->currbinel->jpegpath;
							ELEM_TYPE temptype = this->currbinel->type;
							this->currbinel->type = this->movingbinel->type;
							this->currbinel->path = this->movingbinel->path;
							this->currbinel->jpegpath = this->movingbinel->jpegpath;
							this->movingbinel->type = temptype;
							this->movingbinel->path = temp1;
							this->movingbinel->jpegpath = temp3;
							std::string path = mainprogram->binsdir + this->currbin->name + ".bin";
							save_bin(path);
							this->currbinel = nullptr;
							this->movingbinel = nullptr;
							this->movingtex = -1;
							mainprogram->leftmouse = false;
						}
						else if (binel->full) {
							this->movingtex = binel->tex;
							this->movingbinel = binel;
							this->currbinel = binel;
							mainprogram->leftmouse = false;
						}
					}
					else if (this->currbinel and this->movingtex != -1) {
						if (binel->type != ELEM_DECK and binel->type != ELEM_MIX) {
							bool temp = this->currbinel->full;
							this->currbinel->full = this->movingbinel->full;
							this->movingbinel->full = binel->full;
							this->currbinel->tex = this->movingbinel->tex;
							this->movingbinel->tex = binel->tex;
							binel->tex = this->movingtex;
							binel->full = temp;
							this->currbinel = binel;
						}
					}
					this->currbinel = binel;
				}
				

				if (mainprogram->del) {
					if (this->movingtex != -1) {
						this->movingtex = -1;
						this->movingbinel->tex = this->movingbinel->oldtex;
						std::string name = remove_extension(basename(this->movingbinel->path));
						if (this->movingbinel->type == ELEM_LAYER) boost::filesystem::remove(this->movingbinel->path);
						this->movingbinel->path = this->movingbinel->oldpath;
						this->movingbinel->jpegpath = this->movingbinel->oldjpegpath;
						this->movingbinel->type = this->movingbinel->oldtype;
						this->movingbinel->full = false;
						if (this->currbinel) this->currbinel->tex = this->currbinel->oldtex;
						boost::filesystem::remove(this->movingbinel->jpegpath);
						std::string path = mainprogram->binsdir + this->currbin->name + ".bin";
						save_bin(path);
					}
					if (this->movingstruct) {
						if (this->inserting == 0) {
							for (int k = 0; k < this->inserttexes[0].size(); k++) {
								glDeleteTextures(1, &this->inserttexes[0][k]);
								std::string path = this->insertpaths[0][k];
								boost::filesystem::remove(path);
								path = this->insertjpegpaths[0][k];
								boost::filesystem::remove(path);
								BinElement *deckbinel = find_element(this->inserttexes[deck].size(), k, this->movingdeck->i, this->movingdeck->j, 1);
								glBindTexture(GL_TEXTURE_2D, deckbinel->oldtex);
								glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB, (int)(mainprogram->ow3), (int)(mainprogram->oh3));
								deckbinel->tex = deckbinel->oldtex;
								deckbinel->path = deckbinel->oldpath;
								deckbinel->jpegpath = deckbinel->oldjpegpath;
								deckbinel->type = deckbinel->oldtype;
							}
							boost::filesystem::remove(this->movingdeck->path);
							this->currbin->decks.erase(std::find(this->currbin->decks.begin(), this->currbin->decks.end(), this->movingdeck));
							delete this->movingdeck;
							std::string path = mainprogram->binsdir + this->currbin->name + ".bin";
							save_bin(path);
						}
						else if (this->inserting == 2) {
							for (int m = 0; m < 2; m++) {
								for (int k = 0; k < this->inserttexes[m].size(); k++) {
									glDeleteTextures(1, &this->inserttexes[m][k]);
									std::string path = this->insertpaths[m][k];
									boost::filesystem::remove(path);
									path = this->insertjpegpaths[0][k];
									boost::filesystem::remove(path);
									int size = std::max(((int)((this->inserttexes[0].size() - 1) / 3) * 3 + 3), ((int)((this->inserttexes[1].size() - 1) / 3) * 3 + 3));
									int newj = this->prevj;
									if (this->prevj > 23 - (int)((size - 1) / 3)) newj = this->prevj - (int)((size - 1) / 3);
									BinElement *deckbinel = find_element(size, k, m * 3, newj, 1);
									glBindTexture(GL_TEXTURE_2D, deckbinel->oldtex);
									glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB, (int)(mainprogram->ow3), (int)(mainprogram->oh3));
									deckbinel->tex = deckbinel->oldtex;
									if (this->movingstruct) {
										deckbinel->path = deckbinel->oldpath;
										deckbinel->jpegpath = deckbinel->oldjpegpath;
										deckbinel->type = deckbinel->oldtype;
									}
								}
							}
							boost::filesystem::remove(this->movingmix->path);
							this->currbin->mixes.erase(std::find(this->currbin->mixes.begin(), this->currbin->mixes.end(), this->movingmix));
							delete this->movingmix;
							std::string path = mainprogram->binsdir + this->currbin->name + ".bin";
							save_bin(path);
						}
						this->movingstruct = false;
						this->inserting = -1;
					}
					mainprogram->del = false;
				}
				
				if (this->currbinel and lay->vidmoving and mainprogram->leftmouse) {
					glBindTexture(GL_TEXTURE_2D, this->inputbinel->tex);
					glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 8, 8);
					this->currbinel->full = true;
					if (lay->vidmoving) {
						this->currbinel->type = ELEM_LAYER;
						std::string name = remove_extension(basename(lay->filename));
						int count = 0;
						while (1) {
							this->currbinel->path = mainprogram->binsdir + this->currbin->name + "/" + name + ".layer";
							if (!exists(this->currbinel->path)) {
								mainmix->save_layerfile(this->currbinel->path, lay, 1);
								break;
							}
							count++;
							name = remove_version(name) + "_" + std::to_string(count);
						}
					}
					std::string path = mainprogram->binsdir + this->currbin->name + ".bin";
					save_bin(path);
					this->currbinel = nullptr;
					enddrag();
					mainprogram->leftmouse = false;
				}
				if (this->currbinel and this->templayers.size() and mainprogram->leftmouse) {
					for (int k = 0; k < this->templayers.size(); k++) {
						int intm = (144 - (this->previ + this->prevj * 6) - this->templayers.size());
						intm = (intm < 0) * intm;
						int jj = this->prevj + (int)((k + this->previ + intm) / 6) - ((k + this->previ + intm) < 0);
						int ii = ((k + intm + 144) % 6 + this->previ + 144) % 6;
						Layer *lay = this->templayers[k];
						BinElement *dirbinel = this->currbin->elements[ii * 24 + jj];
						dirbinel->full = true;
						dirbinel->type = lay->type;
						dirbinel->path = this->newpaths[k];
						dirbinel->jpegpath = "";
						dirbinel->oldjpegpath = "";
						//glDeleteTextures(1, &dirbinel->oldtex);
						lay->closethread = true;
						while (lay->closethread) {
							lay->ready = true;
							lay->startdecode.notify_one();
						}
						//delete lay;
					}
					std::string path = mainprogram->binsdir + this->currbin->name + ".bin";
					save_bin(path);
					this->inputtexes.clear();
					this->currbinel = nullptr;
					this->templayers.clear();
					this->newpaths.clear();
					mainprogram->leftmouse = false;
				}
				if (this->currbinel and (this->inserting > -1) and mainprogram->leftmouse) {
					
					this->prevbinel = nullptr;
					std::string dirname = this->currbin->name;
					std::string name = this->currbin->name;
					std::string binname = name;
					std::string path;
					BinDeck *bd = nullptr;
					BinMix *bm = nullptr;
					if (!this->movingstruct) {
						int startdeck, enddeck;
						if (this->inserting == 0) {
							startdeck = 0;
							enddeck = 1;
						}
						else if (this->inserting == 1) {
							startdeck = 1;
							enddeck = 2;
						}
						else if (this->inserting == 2) {
							startdeck = 0;
							enddeck = 2;
						}
						mainmix->mousedeck = deck;
						int count1 = 0;
						while (1) {
							if (this->inserting == 2) path = mainprogram->binsdir + dirname + "/" + name + ".mix";
							else path = mainprogram->binsdir + dirname + "/" + name + ".deck";
							if (!exists(path)) {
								int size = std::max(((int)((this->inserttexes[0].size() - 1) / 3) * 3 + 3), ((int)((this->inserttexes[1].size() - 1) / 3) * 3 + 3));
								for (int d = startdeck; d < enddeck; d++) {
									std::vector<Layer*> &lvec = choose_layers(d);
									for (int k = 0; k < this->inserttexes[d].size(); k++) {
										BinElement *deckbinel;
										if (this->inserting == 2) {
											int newj = this->prevj;
											if (this->prevj > 23 - (int)((size - 1) / 3)) newj = this->prevj - (int)((size - 1) / 3);
											deckbinel = find_element(size, k, d * 3, newj, 1);
										}
										else deckbinel = find_element(this->inserttexes[d].size(), k, this->previ, this->prevj, 1);
										deckbinel->full = true;
										if (this->inserting == 2) deckbinel->type = ELEM_MIX;
										else deckbinel->type = ELEM_DECK;
										glDeleteTextures(1, &deckbinel->oldtex);
										std::string name = remove_extension(basename(lvec[k]->filename));
										int count = 0;
										while (1) {
											if (lvec[k]->filename == "") {
												deckbinel->path = mainprogram->binsdir + this->currbin->name + "/" + ".layer";
												mainmix->save_layerfile(deckbinel->path, lvec[k], 1);
												break;
											}
											else {
												deckbinel->path = mainprogram->binsdir + this->currbin->name + "/" + name + ".layer";
												if (!exists(deckbinel->path)) {
													mainmix->save_layerfile(deckbinel->path, lvec[k], 1);
													break;
												}
											}
											count++;
											name = remove_version(name) + "_" + std::to_string(count);
										}
									}
								}
								if (this->inserting == 2) {
									bm = new BinMix;
									this->currbin->mixes.push_back(bm);
									bm->path = path;
									bm->j = this->prevj;
									bm->height = std::max((int)((this->inserttexes[0].size() - 1) / 3), (int)((this->inserttexes[1].size() - 1) / 3)) + 1;
									mainmix->save_mix(path);
								}
								else {
									bd = new BinDeck;
									this->currbin->decks.push_back(bd);
									bd->path = path;
									bd->i = (int)(this->previ / 3) * 3;
									bd->j = this->prevj;
									bd->height = (int)((this->inserttexes[deck].size() - 1) / 3) + 1;
									mainmix->save_deck(path);
								}
								break;
							}
							count1++;
							name = remove_version(name) + "_" + std::to_string(count1);
						}
					}
					else {
						if (this->inserting == 0) {
							bd = this->movingdeck;
							bd->i = (int)(this->previ / 3) * 3;
							bd->j = this->prevj;
							bd->height = (int)((this->inserttexes[0].size() - 1) / 3) + 1;
						}
						else {
							bm = this->movingmix;
							bm->j = this->prevj;
							bm->height = std::max((int)((this->inserttexes[0].size() - 1) / 3), (int)((this->inserttexes[1].size() - 1) / 3)) + 1;
						}
						this->movingstruct = false;
					}
					//fill blanks with type
					if (this->inserting != 2) {
						for (int i = bd->i; i < bd->i + 3; i++) {
							for (int j = bd->j; j < bd->j + bd->height; j++) {
								this->currbin->elements[i * 24 + j]->type = ELEM_DECK;
							}
						}
					}
					else {
						for (int i = 0; i < 6; i++) {
							for (int j = bm->j; j < bm->j + bm->height; j++) {
								this->currbin->elements[i * 24 + j]->type = ELEM_MIX;
							}
						}
					}
					path = mainprogram->binsdir + this->currbin->name + ".bin";
					save_bin(path);
					this->inserttexes[0].clear();
					this->inserttexes[1].clear();
					this->inserttypes[0].clear();
					this->inserttypes[1].clear();
					this->insertpaths[0].clear();
					this->insertpaths[1].clear();
					this->insertjpegpaths[0].clear();
					this->insertjpegpaths[1].clear();
					this->inserting = -1;
					this->currbinel = nullptr;
					this->newpaths.clear();
					mainprogram->leftmouse = false;
				}
			}
		}
	}
}

void BinsMain::open_bin(const std::string &path, Bin *bin) {
	std::string result = deconcat_files(path);
	bool concat = (result != "");
	std::ifstream rfile;
	if (concat) rfile.open(result);
	else rfile.open(path);
	
	int filecount = 0;
	std::string istring;
	getline(rfile, istring);
	//check if binfile
	int count = 0;
	while (getline(rfile, istring)) {
		if (istring == "ENDOFFILE") {
			break;
		}
		else if (istring == "ELEMS") {
			while (getline(rfile, istring)) {
				if (istring == "ENDOFELEMS") break;
				bin->elements[count]->path = istring;
				if (istring != "") bin->elements[count]->full = true;
				else bin->elements[count]->full = false;
				getline(rfile, istring);
				bin->elements[count]->type = (ELEM_TYPE)std::stoi(istring);
				ELEM_TYPE type = bin->elements[count]->type;
				if ((type == ELEM_LAYER or type == ELEM_DECK or type == ELEM_MIX) and bin->elements[count]->path != "") {
					if (concat) {
						boost::filesystem::rename(result + "_" + std::to_string(filecount) + ".file", bin->elements[count]->path);
						filecount++;
					}
				}
				getline(rfile, istring);
				bin->elements[count]->jpegpath = istring;
				if (bin->elements[count]->path != "") {
					if (bin->elements[count]->jpegpath != "") {
						if (concat) {
							boost::filesystem::rename(result + "_" + std::to_string(filecount) + ".file", result + "_" + std::to_string(filecount) + ".jpeg");
							open_thumb(result + "_" + std::to_string(filecount) + ".jpeg", bin->elements[count]->tex);
							bin->elements[count]->jpegpath = result + "_" + std::to_string(filecount) + ".jpeg";
							filecount++;
						}
						else open_thumb(bin->elements[count]->jpegpath, bin->elements[count]->tex);
					}
				}
				count++;
			}
		}
		else if (istring == "DECKS") {
			while (getline(rfile, istring)) {
				if (istring == "ENDOFDECKS") break;
				BinDeck *bd = new BinDeck;
				bin->decks.push_back(bd);
				if (concat) {
					bd->path = result + "_" + std::to_string(filecount) + ".deck";
					boost::filesystem::rename(result + "_" + std::to_string(filecount) + ".file", bd->path);
				}
				else bd->path = istring;
				filecount++;
				getline(rfile, istring);
				bd->i = std::stoi(istring);
				getline(rfile, istring);
				bd->j = std::stoi(istring);
				getline(rfile, istring);
				bd->height = std::stoi(istring);
			}
		}
		else if (istring == "MIXES") {
			while (getline(rfile, istring)) {
				if (istring == "ENDOFMIXES") break;
				BinMix *bm = new BinMix;
				bin->mixes.push_back(bm);
				if (concat) {
					bm->path = result + "_" + std::to_string(filecount) + ".mix";
					boost::filesystem::rename(result + "_" + std::to_string(filecount) + ".file", bm->path);
				}
				else bm->path = istring;
				filecount++;
				getline(rfile, istring);
				bm->j = std::stoi(istring);
				getline(rfile, istring);
				bm->height = std::stoi(istring);
			}
		}
	}

	rfile.close();
}

void BinsMain::save_bin(const std::string &path) {
	std::vector<std::string> filestoadd;
	std::ofstream wfile;
	wfile.open(path.c_str());
	wfile << "EWOCvj BINFILE V0.2\n";
	
	wfile << "ELEMS\n";
	for (int i = 0; i < 6; i++) {
		for (int j = 0; j < 24; j++) {
			wfile << this->currbin->elements[i * 24 + j]->path;
			wfile << "\n";
			wfile << std::to_string(this->currbin->elements[i * 24 + j]->type);
			wfile << "\n";
			ELEM_TYPE type = this->currbin->elements[i * 24 + j]->type;
			if ((type == ELEM_LAYER or type == ELEM_DECK or type == ELEM_MIX) and this->currbin->elements[i * 24 + j]->path != "") {
				filestoadd.push_back(this->currbin->elements[i * 24 + j]->path);
			}
			if (this->currbin->elements[i * 24 + j]->path != "") {
				if (this->currbin->elements[i * 24 + j]->jpegpath == "") {
					std::string name = remove_extension(basename(this->currbin->elements[i * 24 + j]->path));
					int count = 0;
					while (1) {
						this->currbin->elements[i * 24 + j]->jpegpath = mainprogram->temppath + this->currbin->name + "_" + name + ".jpg";
						if (!exists(this->currbin->elements[i * 24 + j]->jpegpath)) {
							break;
						}
						count++;
						name = remove_version(name) + "_" + std::to_string(count);
					}
					save_thumb(this->currbin->elements[i * 24 + j]->jpegpath, this->currbin->elements[i * 24 + j]->tex);
				}
				filestoadd.push_back(this->currbin->elements[i * 24 + j]->jpegpath);
			}				
			wfile << this->currbin->elements[i * 24 + j]->jpegpath;
			wfile << "\n";
		}
	}
	wfile << "ENDOFELEMS\n";
	
	wfile << "DECKS\n";
	for (int i = 0; i < this->currbin->decks.size(); i++) {
		wfile << this->currbin->decks[i]->path;
		wfile << "\n";
		if (this->currbin->decks[i]->path != "") filestoadd.push_back(this->currbin->decks[i]->path);
		wfile << std::to_string(this->currbin->decks[i]->i);
		wfile << "\n";
		wfile << std::to_string(this->currbin->decks[i]->j);
		wfile << "\n";
		wfile << std::to_string(this->currbin->decks[i]->height);
		wfile << "\n";
	}
	wfile << "ENDOFDECKS\n";
	wfile << "MIXES\n";
	for (int i = 0; i < this->currbin->mixes.size(); i++) {
		wfile << this->currbin->mixes[i]->path;
		wfile << "\n";
		if (this->currbin->mixes[i]->path != "") filestoadd.push_back(this->currbin->mixes[i]->path);
		wfile << std::to_string(this->currbin->mixes[i]->j);
		wfile << "\n";
		wfile << std::to_string(this->currbin->mixes[i]->height);
		wfile << "\n";
	}
	wfile << "ENDOFMIXES\n";
		
	wfile << "ENDOFFILE\n";
	wfile.close();
	
    std::ofstream outputfile;
	outputfile.open(mainprogram->temppath + "tempconcatbin", std::ios::out | std::ios::binary);
	std::vector<std::vector<std::string>> filestoadd2;
	filestoadd2.push_back(filestoadd);
	concat_files(outputfile, path, filestoadd2);
	outputfile.close();
	boost::filesystem::rename(mainprogram->temppath + "tempconcatbin", path);
}

Bin *BinsMain::new_bin(const std::string &name) {
	Bin *bin = new Bin(this->bins.size());
	bin->name = name;
	this->bins.push_back(bin);
	//this->currbin->elements.clear();
	
	for (int i = 0; i < 6; i++) {
		for (int j = 0; j < 24; j++) {
			BinElement *binel = new BinElement;
			bin->elements.push_back(binel);
		}
	}
	make_currbin(this->bins.size() - 1);
	std::string path;
	path = mainprogram->binsdir + name + ".bin";
	if (!exists(path)) {
		save_bin(path);
		this->save_binslist();
	}
	boost::filesystem::path p1{mainprogram->binsdir + name};
	boost::filesystem::create_directory(p1);
	return bin;
}

void BinsMain::make_currbin(int pos) {
	this->currbin->pos = pos;
	this->currbin->name = this->bins[pos]->name;
	this->currbin->elements = this->bins[pos]->elements;
	this->currbin->decks = this->bins[pos]->decks;
	this->currbin->mixes = this->bins[pos]->mixes;
	this->prevbinel = nullptr;
}

int BinsMain::read_binslist() {
	std::ifstream rfile;
	rfile.open(mainprogram->binsdir + "bins.list");
	std::string istring;
	getline(rfile, istring);
	//check if is binslistfile
	getline(rfile, istring);
	int currbin = std::stoi(istring);
	for (int i = 0; i < this->bins.size(); i++) {
		delete this->bins[i];
	}
	this->bins.clear();
	while (getline(rfile, istring)) {
		Bin *newbin;
		if (istring == "ENDOFFILE") break;
		if (istring == "BINS") {
			while (getline(rfile, istring)) {
				if (istring == "ENDOFBINS") break;
				newbin = new_bin(istring);
			}
		}
	}
	rfile.close();
	return currbin;
}

void BinsMain::save_binslist() {
	std::ofstream wfile;
	wfile.open(mainprogram->binsdir + "bins.list");
	wfile << "EWOC BINSLIST v0.1\n";
	wfile << std::to_string(this->currbin->pos);
	wfile << "\n";
	wfile << "BINS\n";
	for (int i = 0; i < this->bins.size(); i++) {
		std::string path = mainprogram->binsdir + this->bins[i]->name + ".bin";
		wfile << this->bins[i]->name;
		wfile << "\n";
	}
	wfile << "ENDOFBINS\n";
	wfile << "ENDOFFILE\n";
	wfile.close();
}

void BinsMain::get_texes(int deck) {
	std::vector<Layer*> &lvec = choose_layers(deck);
	if (lvec.size() == 1 and lvec[0]->filename == "") {
		binsmain->inserting = -1;
		return;
	}
	for (int i = 0; i < lvec.size(); i++) {
		GLuint tex, fbo;
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, mainprogram->ow3, mainprogram->oh3);
		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glViewport(0, 0, mainprogram->ow3, mainprogram->oh3);
		GLint down = glGetUniformLocation(mainprogram->ShaderProgram, "down");
		glUniform1i(down, 1);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, lvec[i]->node->vidbox->tex);
		glBindVertexArray(mainprogram->fbovao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glUniform1i(down, 0);
		this->inserttexes[deck].push_back(tex);
		glDeleteFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glViewport(0, 0, glob->w, glob->h);
		glDeleteFramebuffers(1, &fbo);
	}
}

void BinsMain::open_binfiles() {
	mainprogram->blocking = true;
	if (SDL_GetMouseFocus() != mainprogram->mainwindow) {
		SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW));
	}
	else {
		SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT));
	} 
	if (mainprogram->counting == mainprogram->paths.size()) {
		this->openbinfile = false;
		mainprogram->paths.clear();
		mainprogram->blocking = false;
		SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW));
		return;
	}
	std::string str = mainprogram->paths[mainprogram->counting];
	open_handlefile(str);
	mainprogram->counting++;
}

void BinsMain::open_bindir() {
	if (SDL_GetMouseFocus() != mainprogram->mainwindow) {
		SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW));
	}
	else {
		SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT));
	} 
	struct dirent *ent;
	if ((ent = readdir(mainprogram->opendir)) != nullptr) {
		char *filepath = (char*)malloc(1024);
		strcpy(filepath, this->binpath.c_str());
		strcat(filepath, "/");
		strcat(filepath, ent->d_name);
		std::string str(filepath);
		
		open_handlefile(str);
	}
	else {
		this->openbindir = false;
		mainprogram->blocking = false;
		SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW));
	} 
}

void BinsMain::open_handlefile(const std::string &path) {
	Layer *lay = new Layer(true);
	lay->dummy = 1;
	if (isimage(path)) {
		lay->open_image(path);
	}
	else {
		open_video(1, lay, path, true);
		std::unique_lock<std::mutex> olock(lay->endopenlock);
		lay->endopenvar.wait(olock, [&]{return lay->opened;});
		lay->opened = false;
		olock.unlock();
		if (lay->openerr) {
			printf("error!\n");
			lay->openerr = false;
			lay->closethread = true;
			while (lay->closethread) {
				lay->ready = true;
				lay->startdecode.notify_one();
			}
			//delete lay;
			return;
		}
		lay->frame = lay->numf / 2.0f;
		lay->prevframe = -1;
		lay->ready = true;
		while (lay->ready) {
			lay->startdecode.notify_one();
		}
		std::unique_lock<std::mutex> lock2(lay->enddecodelock);
		lay->enddecodevar.wait(lock2, [&]{return lay->processed;});
		lay->processed = false;
		lock2.unlock();
		lay->initialized = true;
		glBindTexture(GL_TEXTURE_2D, lay->texture);
		if (lay->vidformat == 188 or lay->vidformat == 187) {
			if (lay->decresult->compression == 187) {
				glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, lay->decresult->width, lay->decresult->height, 0, lay->decresult->size, lay->decresult->data);
			}
			else if (lay->decresult->compression == 190) {
				glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, lay->decresult->width, lay->decresult->height, 0, lay->decresult->size, lay->decresult->data);
			}
		}
		else {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, lay->decresult->width, lay->decresult->height, 0, GL_BGRA, GL_UNSIGNED_BYTE, lay->decresult->data);
		}
	}
	GLuint endtex;
	endtex = copy_tex(lay->texture);
	
	this->newpaths.push_back(path);
	this->inputtexes.push_back(endtex);
	this->templayers.push_back(lay);
	this->prevbinel = nullptr;
}

std::tuple<std::string, std::string> BinsMain::hap_binel(BinElement *binel, BinDeck *bd, BinMix *bm) {
	this->binpreview = false;
   	std::string apath = "";
	std::string rpath = "";
	if (binel->type == ELEM_FILE) {
		std::thread hap (&BinsMain::hap_encode, this, binel->path, binel, nullptr, nullptr);
		if (!mainprogram->threadmode) {
			#ifdef _WIN64
			SetThreadPriority((void*)hap.native_handle(), THREAD_PRIORITY_LOWEST);
			#else
			struct sched_param params;
			params.sched_priority = sched_get_priority_min(SCHED_FIFO);		pthread_setschedparam(hap.native_handle(), SCHED_FIFO, &params);
			#endif
		}
		hap.detach();
		binel->path = remove_extension(binel->path) + "_hap.mov";
	}
	else {
		std::ifstream rfile;
		rfile.open(binel->path);
		std::ofstream wfile;
		wfile.open(remove_extension(binel->path) + ".temp");
		std::string istring;
		std::string path = "";
		while (getline(rfile, istring)) {
			if (istring == "FILENAME") {
				getline(rfile, istring);
				apath = istring;
				if (exists(istring)) {
					path = istring;
					apath = remove_extension(apath) + "_hap.mov";
					wfile << "FILENAME\n";
					wfile << apath;
					wfile << "\n";
					wfile << "RELPATH\n";
					wfile << mainprogram->docpath + boost::filesystem::relative(apath, mainprogram->docpath).string();
					rpath = mainprogram->docpath + boost::filesystem::relative(apath, mainprogram->docpath).string();
					wfile << "\n";
				}
			}
			else if (istring == "RELPATH") {
				getline(rfile, istring);
				if (path == "") {
					rpath = istring;
					if (exists(istring)) {
						path = boost::filesystem::canonical(istring).string();
						rpath = remove_extension(rpath) + "_hap.mov";
						wfile << "FILENAME\n";
						wfile << boost::filesystem::canonical(rpath).string();
						wfile << "\n";
						wfile << "RELPATH\n";
						wfile << path;
						wfile << "\n";
						apath = boost::filesystem::canonical(rpath).string();
					}
					else {
						wfile << "FILENAME\n";
						wfile << path;
						wfile << "\n";
						wfile << "RELPATH\n";
						wfile << mainprogram->docpath + boost::filesystem::relative(path, mainprogram->docpath).string();
						wfile << "\n";
					}
				}
			}
			else {
				wfile << istring;
				wfile << "\n";
			}
		}
		if (path != "") {
			AVFormatContext *video = nullptr;
			AVCodecParameters *cpm = nullptr;
			int idx;
			printf("pathname %s\n", path.c_str());
			fflush(stdout);
			avformat_open_input(&video, path.c_str(), nullptr, nullptr);
			avformat_find_stream_info(video, nullptr);
			open_codec_context(&idx, video, AVMEDIA_TYPE_VIDEO);
			cpm = video->streams[idx]->codecpar;
			if (cpm->codec_id == 188 or cpm->codec_id == 187) {
    			apath = path;
    			rpath = mainprogram->docpath + boost::filesystem::relative(path, mainprogram->docpath).string();
				wfile.close();
				rfile.close();
 				boost::filesystem::remove(remove_extension(binel->path) + ".temp");
				binel->encoding = false;
				if (bd) {
					bd->encthreads--;
				}
				else if (bm) {
					bm->encthreads--;
				}
				return {apath, rpath};
 			}
			std::thread hap (&BinsMain::hap_encode, this, path, binel, bd, bm);
			if (!mainprogram->threadmode) {
				#ifdef _WIN64
				SetThreadPriority((void*)hap.native_handle(), THREAD_PRIORITY_LOWEST);
				#else
				struct sched_param params;
				params.sched_priority = sched_get_priority_min(SCHED_FIFO);		pthread_setschedparam(hap.native_handle(), SCHED_FIFO, &params);
				#endif
			}
			hap.detach();
		}
		else printf("Can't find file to encode");
		wfile.close();
		rfile.close();
		boost::filesystem::rename(remove_extension(binel->path) + ".temp", binel->path);
	}
	std::string bpath = mainprogram->binsdir + this->currbin->name + ".bin";
	this->save_bin(bpath);
	
	return {apath, rpath};
}				
	
void BinsMain::hap_deck(BinDeck * bd) {			
	std::ifstream rfile;
	rfile.open(bd->path);
	std::ofstream wfile;
	wfile.open(remove_extension(bd->path) + ".temp");
	std::string istring;
	bd->encthreads = 0;
	for (int j = 0; j < bd->height; j++) {
		for (int m = 0; m < 3; m++) {
			BinElement *deckbinel = this->currbin->elements[(bd->j + j) + ((bd->i + m) * 24)];
			std::string apath;
			std::string rpath;
			if (deckbinel->path != "") {
				bd->encthreads++;
				std::tuple<std::string, std::string> output = this->hap_binel(deckbinel, bd, nullptr);
				apath = std::get<0>(output);
				rpath = std::get<1>(output);
			}
			else {
				apath = "";
				rpath = "";
			}
			while (getline(rfile, istring)) {
				if (istring == "FILENAME") {
					getline(rfile, istring);
					wfile << "FILENAME\n";
					wfile << apath;
					wfile << "\n";
					wfile << "RELPATH\n";
					wfile << rpath;
					wfile << "\n";
				}
				else if (istring == "RELPATH") {
					getline(rfile, istring);
					break;
				}
				else {
					wfile << istring;
					wfile << "\n";
				}
			}
		}
	}
	wfile.close();
	rfile.close();
	boost::filesystem::rename(remove_extension(bd->path) + ".temp", bd->path);
	this->save_bin(mainprogram->binsdir + this->currbin->name + ".bin");
	this->open_bin(mainprogram->binsdir + this->currbin->name + ".bin", this->currbin);
}

void BinsMain::hap_mix(BinMix * bm) {
	std::ifstream rfile;
	rfile.open(bm->path);
	std::ofstream wfile;
	wfile.open(remove_extension(bm->path) + ".temp");
	std::string istring;
	bm->encthreads = 0;
	for (int n = 0; n < 2; n++) {
		for (int j = 0; j < bm->height; j++) {
			for (int m = 3 * n; m < 3 * (n + 1); m++) {
				BinElement *mixbinel = this->currbin->elements[(bm->j + j) + (m * 24)];
				std::string apath;
				std::string rpath;
				if (mixbinel->path != "") {
					bm->encthreads++;
					std::tuple<std::string, std::string> output = this->hap_binel(mixbinel, nullptr, bm);
					apath = std::get<0>(output);
					rpath = std::get<1>(output);
					while (getline(rfile, istring)) {
						if (istring == "FILENAME") {
							getline(rfile, istring);
							wfile << "FILENAME\n";
							wfile << apath;
							wfile << "\n";
							wfile << "RELPATH\n";
							wfile << rpath;
							wfile << "\n";
						}
						else if (istring == "RELPATH") {
							getline(rfile, istring);
							break;
						}
						else {
							wfile << istring;
							wfile << "\n";
						}
					}
				}
			}
		}
	}
	wfile.close();
	rfile.close();
	boost::filesystem::rename(remove_extension(bm->path) + ".temp", bm->path);
	this->save_bin(mainprogram->binsdir + this->currbin->name + ".bin");
	this->open_bin(mainprogram->binsdir + this->currbin->name + ".bin", this->currbin);
}	

void BinsMain::hap_encode(const std::string srcpath, BinElement *binel, BinDeck *bd, BinMix *bm) {
	binel->encwaiting = true;
	while (mainprogram->encthreads >= mainprogram->maxthreads) {
		mainprogram->hapnow = false;
		std::unique_lock<std::mutex> lock(mainprogram->hapmutex);
		mainprogram->hap.wait(lock, [&]{return mainprogram->hapnow;});
		lock.unlock();
	}	
	mainprogram->encthreads++;
	binel->encwaiting = false;
	binel->encoding = true;
	binel->encodeprogress = 0.0f;
	
	// opening the source vid
	AVFormatContext *source = nullptr;
	AVCodecContext *source_dec_ctx;
	AVCodecParameters *source_dec_cpm;
	AVStream *source_stream;
	int source_stream_idx;
	int numf;
	int r = avformat_open_input(&source, srcpath.c_str(), nullptr, nullptr);
	if (r < 0) {
		printf("%s\n", "Couldnt open video to encode");
		return;
	}
	if (avformat_find_stream_info(source, nullptr) < 0) {
        printf("Could not find stream information\n");
        return;
    }
	open_codec_context(&source_stream_idx, source, AVMEDIA_TYPE_VIDEO);
    source_stream = source->streams[source_stream_idx];
    source_dec_cpm = source_stream->codecpar;
	AVCodec *scodec = avcodec_find_decoder(source_stream->codecpar->codec_id);	
	source_dec_ctx = avcodec_alloc_context3(scodec);
	r = avcodec_parameters_to_context(source_dec_ctx, source_dec_cpm);
    source_dec_ctx->time_base = {1, 1000};
    source_dec_ctx->framerate = {source_stream->r_frame_rate.num, source_stream->r_frame_rate.den};
	r = avcodec_open2(source_dec_ctx, scodec, nullptr);
	if (source_dec_cpm->codec_id == 188 or source_dec_cpm->codec_id == 187) {
		binel->encoding = false;
		if (bd) {
			bd->encthreads--;
		}
		else if (bm) {
			bm->encthreads--;
		}
		return;
	}
	numf = source_stream->nb_frames;
	
	// setting up destination vid
	AVFormatContext *dest = avformat_alloc_context();
	AVStream *dest_stream;
    const AVCodec *codec = nullptr;
    AVCodecContext *c = nullptr;
    AVFrame *nv12frame;
    AVPacket pkt;
    //uint8_t endcode[] = { 0, 0, 1, 0xb7 };
    codec = avcodec_find_encoder_by_name("hap");
    c = avcodec_alloc_context3(codec);
	av_init_packet(&pkt);
	pkt.data = nullptr;
	pkt.size = 0;
	/* open it */
    c->time_base = source_dec_ctx->time_base;
    //c->framerate = (AVRational){source_stream->r_frame_rate.num, source_stream->r_frame_rate.den};
	c->sample_aspect_ratio = source_dec_cpm->sample_aspect_ratio;
    c->pix_fmt = codec->pix_fmts[0];  
    int rem = source_dec_cpm->width % 32;
    c->width = source_dec_cpm->width + (32 - rem) * (rem > 0);
    rem = source_dec_cpm->height % 4;
    c->height = source_dec_cpm->height + (4 - rem) * (rem > 0);
    //c->global_quality = 0;
   	r = avcodec_open2(c, codec, nullptr);
       
    std::string destpath = remove_extension(srcpath) + "_hap.mov";
    avformat_alloc_output_context2(&dest, av_guess_format("mov", nullptr, "video/mov"), nullptr, destpath.c_str());
    dest_stream = avformat_new_stream(dest, codec);
    dest_stream->time_base = source_stream->time_base;
	r = avcodec_parameters_from_context(dest_stream->codecpar, c);
    if (r < 0) {
		av_log(nullptr, AV_LOG_ERROR, "Copying stream context failed\n");
		av_log(nullptr, AV_LOG_ERROR, "Copying parameters for stream #%u failed\n");
    }   
    dest_stream->r_frame_rate = source_stream->r_frame_rate;
    dest->oformat->flags |= AVFMT_NOFILE;
    //avformat_init_output(dest, nullptr);
    r = avio_open(&dest->pb, destpath.c_str(), AVIO_FLAG_WRITE);
  	r = avformat_write_header(dest, nullptr);
  
	nv12frame = av_frame_alloc();
    nv12frame->format = c->pix_fmt;
    nv12frame->width  = c->width;
    nv12frame->height = c->height;
 
	// Determine required buffer size and allocate buffer
  	struct SwsContext *sws_ctx = sws_getContext
    (
        c->width,
       	c->height,
        (AVPixelFormat)source_dec_cpm->format,
        c->width,
        c->height,
        c->pix_fmt,
        SWS_BILINEAR,
        nullptr,
        nullptr,
        nullptr);
	/* transcode */
	AVFrame *decframe = nullptr;
	decframe = av_frame_alloc();
	int frame = 0;
	int count = 0;
    while (count < numf) {
    	binel->encodeprogress = (float)count / (float)numf;
    	// decode a frame
		av_init_packet(&pkt);
		pkt.data = nullptr;
		pkt.size = 0;
		av_read_frame(source, &pkt);
		if (pkt.stream_index != source_stream_idx) continue;
    	count++;
		int got_frame;
    	while (1) {
			avcodec_send_packet(source_dec_ctx, &pkt);
 			int r = avcodec_receive_frame(source_dec_ctx, decframe);
			if (r == AVERROR(EAGAIN)) {
				av_packet_unref(&pkt);
				av_read_frame(source, &pkt);
			}
			else break;
			if (r == AVERROR(EINVAL)) printf("EINVAL\n");
		}
	   	
	   	// do pixel format conversion
		int storage = av_image_alloc(nv12frame->data, nv12frame->linesize, nv12frame->width, nv12frame->height, c->pix_fmt, 32);
		sws_scale
		(
			sws_ctx,
			decframe->data,
			decframe->linesize,
			0,
			nv12frame->height,
			nv12frame->data,
			nv12frame->linesize
		);
		nv12frame->pts = decframe->pts;

		encode_frame(dest, source, c, nv12frame, &pkt, nullptr, frame);
		
        av_freep(nv12frame->data);
		av_packet_unref(&pkt);
		//av_frame_unref(decframe);
		//av_frame_unref(nv12frame);
        frame++;
    }
    /* flush the encoder */
    // int got_frame;
    // while (1) {
    	// got_frame = encode_frame(dest, c, nullptr, &pkt, nullptr);
    	// if (!got_frame) break;
    // }
    av_write_trailer(dest);
    avio_close(dest->pb);
    avcodec_free_context(&c);
    av_frame_free(&nv12frame);
    av_packet_unref(&pkt);
    binel->encoding = false;
    if (bd) {
    	bd->encthreads--;
    }
    else if (bm) {
    	bm->encthreads--;
   	}
	mainprogram->encthreads--;
	mainprogram->hapnow = true;
	mainprogram->hap.notify_all();
}

