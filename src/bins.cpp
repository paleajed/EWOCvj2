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

BinElement* BinElement::next() {
	int pos = std::find(this->bin->elements.begin(), this->bin->elements.end(), this) - this->bin->elements.begin();
	int j = pos / 6;
	int i = pos / 24;
	if (i == 5) j++;
	else i++;
	int nxt = i * 24 + j;
	if (nxt == 144) return nullptr;
	else return this->bin->elements[nxt];
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

void BinsMain::handle(bool draw) {
	float white[] = {1.0f, 1.0f, 1.0f, 1.0f};
	float black[] = {0.0f, 0.0f, 0.0f, 1.0f};
	float darkgrey[] = {0.2f, 0.2f, 0.2f, 1.0f};
	float lightblue[] = {0.5f, 0.5f, 1.0f, 1.0f};
	float red[] = {1.0f, 0.5f, 0.5f, 1.0f};
	float lightgreen[] = {0.5f, 1.0f, 0.5f, 1.0f};
	
	//draw binelements
	if (mainprogram->menuactivation) mainprogram->menuset = 0; if (draw) {
		for (int i = 0; i < 6; i++) {
			for (int j = 0; j < 24; j++) {
				Box* box = this->elemboxes[i * 24 + j];
				BinElement* binel = this->currbin->elements[i * 24 + j];
				if (box->in()) {
					if (binel->path != "" and mainprogram->menuactivation) {
						this->menubinel = binel;
						std::vector<std::string> binel;
						binel.push_back("Delete element");
						binel.push_back("Open file(s) from disk");
						binel.push_back("Open dir from disk");
						binel.push_back("Open deck");
						binel.push_back("Open mix");
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
			Box* box = this->elemboxes[i * 24];
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
	}

	Layer *lay = nullptr;		
	if (mainmix->currlay) lay = mainmix->currlay;
	else {
		if (mainprogram->prevmodus) lay = mainmix->layersA[0];
		else lay = mainmix->layersAcomp[0];
	}
		
	if (mainprogram->rightmouse and this->currbinel) {
		bool cont = false;
		if (this->prevbinel) {
			if (this->inputtexes.size()) {
				int offset = 0;
				for (int k = 0; k < this->inputtexes.size(); k++) {
					int offj = (int)((this->previ + offset + k) / 6);
					int el = ((this->previ + offset + k + 144) % 6) * 24 + this->prevj + offj;
					if (this->prevj + offj > 23) {
						offset = k - this->inputtexes.size();
						cont = true;
						break;
					}
					BinElement *dirbinel = this->currbin->elements[el];
					while (dirbinel->type == ELEM_DECK or dirbinel->type == ELEM_MIX) {
						offset++;
						offj = (int)((this->previ + offset + k) / 6);
						el = ((this->previ + offset + k + 144) % 6) * 24 + this->prevj + offj;
						if (this->prevj + offj > 23) {
							offset = k - this->inputtexes.size();
							cont = true;
							break;
						}
						dirbinel = this->currbin->elements[el];
					}
					if (cont) break;
				}
				offset = 0;
				if (!cont) {
					for (int k = 0; k < this->inputtexes.size(); k++) {
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
					}
				}
				else {
					for (int k = 0; k < this->inputtexes.size(); k++) {
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
					}
				}
				this->currbinel = nullptr;
				this->inputtexes.clear();
				mainprogram->rightmouse = false;
			}
		}
	}
	
	if (draw) {
		//draw and handle binslist
		draw_box(red, black, -0.15f, -1.0f, 0.3f, 1.0f, -1);
		for (int i = 0; i < this->bins.size(); i++) {
			Bin* bin = this->bins[i];
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
	}

	if (draw) {
		Box* box = this->newbinbox;
		float newy = (this->bins.size() + 1) * -0.05f;
		box->vtxcoords->y1 = newy;
		box->upvtxtoscr();
		if (box->in()) {
			if (mainprogram->leftmouse) {
				std::string name = "new bin";
				std::string path;
				int count = 0;
				while (1) {
					path = mainprogram->project->binsdir + name + ".bin";
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
				std::string path = mainprogram->project->binsdir + this->menubin->name + ".bin";
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
			BinDeck* bd = this->currbin->decks[i];
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
						binel.push_back("Open file(s) from disk");
						binel.push_back("Open dir from disk");
						binel.push_back("Open deck");
						binel.push_back("Open mix");
						binel.push_back("Insert deck A");
						binel.push_back("Insert deck B");
						binel.push_back("Insert full mix");
						binel.push_back("HAP encode deck");
						binel.push_back("Quit");
						mainprogram->make_menu("binelmenu", mainprogram->binelmenu, binel);
						mainprogram->menuset = 3;
					}
					if (mainprogram->lmsave) {
						if (this->dragdeck and this->dragdeck != bd) {
							this->dragdeck = nullptr;
						}
						mainprogram->leftmouse = false;
					}
					if (this->movingdeck) {
						if (this->movingdeck->encthreads) continue;
					}
					if (mainprogram->leftmousedown) {
						// dragging a deck from one place to the other in the bins view
						mainprogram->leftmousedown = false;
						mainprogram->dragmousedown = true;
						this->dragdeck = bd;
						for (int j = 0; j < bd->height; j++) {
							for (int m = bd->i; m < bd->i + 3; m++) {
								BinElement* mixbinel = this->currbin->elements[(bd->j + j) + (m * 24)];
								mixbinel->full = false;
								this->dragtexes[0].push_back(mixbinel->tex);
							}
						}
						this->movingstruct = true;
						this->inserting = 0;
						this->currbinel = this->currbin->elements[bd->j + bd->i * 24];
						this->prevbinel = this->currbinel;
						this->ii = bd->i;
						this->jj = bd->j;
						for (int j = 0; j < bd->height; j++) {
							for (int k = 0; k < 3; k++) {
								BinElement* deckbinel = this->currbin->elements[(bd->j + j) + ((bd->i + k) * 24)];
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
				}
			}
		}
		for (int i = 0; i < this->currbin->mixes.size(); i++) {
			BinMix* bm = this->currbin->mixes[i];
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
						binel.push_back("Open file(s) from disk");
						binel.push_back("Open dir from disk");
						binel.push_back("Open deck");
						binel.push_back("Open mix");
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
						mainprogram->leftmouse = false;
					}
					if (this->movingmix) {
						if (this->movingmix->encthreads) continue;
					}
					if (mainprogram->leftmousedown) {
						mainprogram->leftmousedown = false;
						// dragging a mix from one place to the other in the bins view
						this->dragmix = bm;
						for (int j = 0; j < bm->height; j++) {
							for (int m = 0; m < 6; m++) {
								BinElement* mixbinel = this->currbin->elements[(bm->j + j) + (m * 24)];
								mixbinel->full = false;
								if (m < 3) this->dragtexes[0].push_back(mixbinel->tex);
								else this->dragtexes[1].push_back(mixbinel->tex);
							}
						}
						this->movingstruct = true;
						this->inserting = 2;
						this->currbinel = this->currbin->elements[bm->j];
						this->ii = 0;
						this->jj = bm->j;
						this->prevbinel = this->currbinel;
						for (int j = 0; j < bm->height; j++) {
							for (int k = 0; k < 6; k++) {
								BinElement* mixbinel = this->currbin->elements[(bm->j + j) + (k * 24)];
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
				}
			}
		}
		if (mainprogram->menuset == 0 and mainprogram->menuactivation) {
			std::vector<std::string> binel;
			binel.push_back("Open file(s) from disk");
			binel.push_back("Open dir from disk");
			binel.push_back("Open deck");
			binel.push_back("Open mix");
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
					BinElement* deckbinel = this->currbin->elements[(this->movingdeck->j + j) + ((this->movingdeck->i + m) * 24)];
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
					BinElement* mixbinel = this->currbin->elements[(this->movingmix->j + j) + (m * 24)];
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
			std::thread filereq(&Program::get_multinname, mainprogram, "Open file(s)", boost::filesystem::canonical(mainprogram->currvideodir).generic_string());
			filereq.detach();
		}
		else if (k == mainprogram->menuset + 1) {
			mainprogram->pathto = "OPENBINDIR";
			mainprogram->blocking = true;
			std::thread filereq(&Program::get_dir, mainprogram, "Open files directory", boost::filesystem::canonical(mainprogram->currbindirdir).generic_string());
			filereq.detach();
		}
		else if (k == mainprogram->menuset + 2) {
			mainprogram->pathto = "OPENBINDECK";
			std::thread filereq(&Program::get_inname, mainprogram, "Open deck", "application/ewocvj2-deck", boost::filesystem::canonical(mainprogram->currvideodir).generic_string());
			filereq.detach();
		}
		else if (k == mainprogram->menuset + 3) {
			mainprogram->pathto = "OPENBINMIX";
			std::thread filereq(&Program::get_inname, mainprogram, "Open mix", "application/ewocvj2-mix", boost::filesystem::canonical(mainprogram->currvideodir).generic_string());
			filereq.detach();
		}
		else if (k == mainprogram->menuset + 4) {
			this->inserting = 0;
			this->inserttexes[0].clear();
			get_texes(0);
			this->prevbinel = nullptr;
		}
		else if (k == mainprogram->menuset + 5) {
			this->inserting = 1;
			this->inserttexes[1].clear();
			get_texes(1);
			this->prevbinel = nullptr;
		}
		else if (k == mainprogram->menuset + 6) {
			this->inserting = 2;
			this->inserttexes[0].clear();
			get_texes(0);
			this->inserttexes[1].clear();
			get_texes(1);
			this->prevbinel = nullptr;
		}
		else if (k == 8 and mainprogram->menuset == 1) {
			this->hap_binel(this->menubinel, nullptr, nullptr);
		}
		else if (k == 10 and mainprogram->menuset == 3) {
			this->hap_deck(this->movingdeck);
		}
		else if (k == 9 and mainprogram->menuset == 2) {
			this->hap_mix(this->movingmix);
		}
		else if (k == 7 and mainprogram->menuset == 0) {
			for (int i = 0; i < 6; i++) {
				for (int j = 0; j < 24; j++) {
					BinElement* binel = this->currbin->elements[i * 24 + j];
					if (binel->full and (binel->type == ELEM_FILE or binel->type == ELEM_LAYER)) {
						this->hap_binel(binel, nullptr, nullptr);
					}
				}
			}
			for (int i = 0; i < this->currbin->decks.size(); i++) {
				BinDeck* bd = this->currbin->decks[i];
				this->hap_deck(bd);
			}
			for (int i = 0; i < this->currbin->mixes.size(); i++) {
				BinMix* bm = this->currbin->mixes[i];
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
	}


	if (mainprogram->menuactivation and !this->inputtexes.size() and this->inserting == -1 and !lay->vidmoving and this->movingtex == -1) {
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
		this->prevelems.clear();
		bool inbinel = false;
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
					inbinel = true;
					if (this->currbin->encthreads) continue;
					if (this->movingdeck) {
						if (this->movingdeck->encthreads) continue;
					}
					else if (this->movingmix) {
						if (this->movingmix->encthreads) continue;
					}
					if (!binel->encwaiting and !binel->encoding and binel->full and !this->inputtexes.size() and this->inserting == -1 and !lay->vidmoving) {
						if (this->previewbinel != binel) {
							this->previewimage = "";
							this->previewbinel = nullptr;
						}
						if ((this->previewimage != "" or  binel->type == ELEM_IMAGE) and !this->binpreview) {
							this->binpreview = true;
							if (mainprogram->prelay) {
								mainprogram->prelay->closethread = true;
									while (mainprogram->prelay->closethread) {
										mainprogram->prelay->ready = true;
											mainprogram->prelay->startdecode.notify_one();
									}
								//delete mainprogram->prelay;
							}
							draw_box(red, black, -0.2f, 0.5f, 0.4f, 0.4f, -1);
							mainprogram->prelay = new Layer(false);
							mainprogram->prelay->dummy = true;
							mainprogram->prelay->open_image(this->previewimage);
							glBindTexture(GL_TEXTURE_2D, mainprogram->prelay->texture);
							ilBindImage(mainprogram->prelay->boundimage);
							ilActiveImage((int)mainprogram->prelay->frame);
							int imageformat = ilGetInteger(IL_IMAGE_FORMAT);
							int w = ilGetInteger(IL_IMAGE_WIDTH);
							int h = ilGetInteger(IL_IMAGE_HEIGHT);
							int bpp = ilGetInteger(IL_IMAGE_BPP);
							GLuint mode = GL_BGR;
							if (imageformat == IL_RGBA) mode = GL_RGBA;
							if (imageformat == IL_BGRA) mode = GL_BGRA;
							if (imageformat == IL_RGB) mode = GL_RGB;
							if (imageformat == IL_BGR) mode = GL_BGR;
							if (bpp == 3) {
								glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, mode, GL_UNSIGNED_BYTE, ilGetData());
							}
							else if (bpp == 4) {
								glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, mode, GL_UNSIGNED_BYTE, ilGetData());
							}
							draw_box(red, black, -0.2f, 0.5f, 0.4f, 0.4f, mainprogram->prelay->texture);
						}
						else if (this->previewimage != "" or binel->type == ELEM_IMAGE) {
							if (mainprogram->mousewheel) {
								mainprogram->prelay->frame += mainprogram->mousewheel * (mainprogram->prelay->numf / 32.0f);
							}
							glBindTexture(GL_TEXTURE_2D, mainprogram->prelay->texture);
							ilBindImage(mainprogram->prelay->boundimage);
							ilActiveImage((int)mainprogram->prelay->frame);
							int imageformat = ilGetInteger(IL_IMAGE_FORMAT);
							int w = ilGetInteger(IL_IMAGE_WIDTH);
							int h = ilGetInteger(IL_IMAGE_HEIGHT);
							int bpp = ilGetInteger(IL_IMAGE_BPP);
							GLuint mode = GL_BGR;
							if (imageformat == IL_RGBA) mode = GL_RGBA;
							if (imageformat == IL_BGRA) mode = GL_BGRA;
							if (imageformat == IL_RGB) mode = GL_RGB;
							if (imageformat == IL_BGR) mode = GL_BGR;
							if (bpp == 3) {
								glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, mode, GL_UNSIGNED_BYTE, ilGetData());
							}
							else if (bpp == 4) {
								glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, mode, GL_UNSIGNED_BYTE, ilGetData());
							}
							draw_box(red, black, -0.2f, 0.5f, 0.4f, 0.4f, mainprogram->prelay->texture);
						}
						else if ((binel->type == ELEM_LAYER or binel->type == ELEM_DECK or binel->type == ELEM_MIX) and !this->binpreview) {
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
								if (isimage(mainprogram->prelay->filename)) {
									this->previewimage = mainprogram->prelay->filename;
									this->previewbinel = binel;
									this->binpreview = false;
									continue;
								}
								mainprogram->prelay->node->calc = true;
								mainprogram->prelay->node->walked = false;
								mainprogram->prelay->playbut->value = false;
								mainprogram->prelay->revbut->value = false;
								mainprogram->prelay->bouncebut->value = false;
								for (int k = 0; k < mainprogram->prelay->effects[0].size(); k++) {
									mainprogram->prelay->effects[0][k]->node->calc = true;
									mainprogram->prelay->effects[0][k]->node->walked = false;
								}
								mainprogram->prelay->frame = 0.0f;
								mainprogram->prelay->prevframe = -1;
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
								mainprogram->prelay->initialized = true;
								mainprogram->prelay->fbotex = copy_tex(mainprogram->prelay->texture);
								onestepfrom(0, mainprogram->prelay->node, nullptr, -1, -1);
								if (mainprogram->prelay->effects[0].size()) {
									draw_box(red, black, -0.2f, 0.5f, 0.4f, 0.4f, mainprogram->prelay->effects[0][mainprogram->prelay->effects[0].size() - 1]->fbotex);
								}
								else {
									if (mainprogram->prelay->drawfbo2) {
										draw_box(red, black, -0.2f, 0.5f, 0.4f, 0.4f, mainprogram->prelay->fbotex2);
									}
									else {
										draw_box(red, black, -0.2f, 0.5f, 0.4f, 0.4f, mainprogram->prelay->fbotexintm);
									}
								}
								if (!binel->encoding) {
									if (mainprogram->prelay->vidformat == 188 or mainprogram->prelay->vidformat == 187) {
										render_text("HAP", white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.015f), 0.0005f, 0.0008f);
										render_text(std::to_string(mainprogram->prelay->decresult->width) + "x" + std::to_string(mainprogram->prelay->decresult->height), white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.045f), 0.0005f, 0.0008f);
									}
									else if (mainprogram->prelay->type != ELEM_IMAGE) {
										render_text("CPU", white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.015f), 0.0005f, 0.0008f);
										render_text(std::to_string(mainprogram->prelay->decresult->width) + "x" + std::to_string(mainprogram->prelay->decresult->height), white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.045f), 0.0005f, 0.0008f);
									}
									else {
										render_text("IMAGE", white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.015f), 0.0005f, 0.0008f);
									}
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
								mainprogram->prelay->fbotex = copy_tex(mainprogram->prelay->texture);
								onestepfrom(0, mainprogram->prelay->node, nullptr, -1, -1);
								if (mainprogram->prelay->effects[0].size()) {
									draw_box(red, black, -0.2f, 0.5f, 0.4f, 0.4f, mainprogram->prelay->effects[0][mainprogram->prelay->effects[0].size() - 1]->fbotex);
								}
								else {
									if (mainprogram->prelay->drawfbo2) {
										draw_box(red, black, -0.2f, 0.5f, 0.4f, 0.4f, mainprogram->prelay->fbotex2);
									}
									else {
										draw_box(red, black, -0.2f, 0.5f, 0.4f, 0.4f, mainprogram->prelay->fbotexintm);
									}
								}
							}
							else {
								if (mainprogram->prelay->effects[0].size()) {
									draw_box(red, black, -0.2f, 0.5f, 0.4f, 0.4f, mainprogram->prelay->effects[0][mainprogram->prelay->effects[0].size() - 1]->fbotex);
								}
								else {
									if (mainprogram->prelay->drawfbo2) {
										draw_box(red, black, -0.2f, 0.5f, 0.4f, 0.4f, mainprogram->prelay->fbotex2);
									}
									else {
										draw_box(red, black, -0.2f, 0.5f, 0.4f, 0.4f, mainprogram->prelay->fbotexintm);
									}
								}
							}
							if (!binel->encoding and remove_extension(basename(binel->path)) != "") {
								if (mainprogram->prelay->vidformat == 188 or mainprogram->prelay->vidformat == 187) {
									render_text("HAP", white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.015f), 0.0005f, 0.0008f);
									render_text(std::to_string(mainprogram->prelay->decresult->width) + "x" + std::to_string(mainprogram->prelay->decresult->height), white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.045f), 0.0005f, 0.0008f);
								}
								else if (mainprogram->prelay->type != ELEM_IMAGE) {
									render_text("CPU", white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.015f), 0.0005f, 0.0008f);
									render_text(std::to_string(mainprogram->prelay->decresult->width) + "x" + std::to_string(mainprogram->prelay->decresult->height), white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.045f), 0.0005f, 0.0008f);
								}
								else {
									render_text("IMAGE", white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.015f), 0.0005f, 0.0008f);
								}
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
									mainprogram->prelay->open_video(0, binel->path, true);
									std::unique_lock<std::mutex> olock(mainprogram->prelay->endopenlock);
									mainprogram->prelay->endopenvar.wait(olock, [&]{return mainprogram->prelay->opened;});
									mainprogram->prelay->opened = false;
									olock.unlock();
									mainprogram->prelay->initialized = true;
									mainprogram->prelay->frame = 0.0f;
									mainprogram->prelay->prevframe = -1;
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
										render_text(std::to_string(mainprogram->prelay->decresult->width) + "x" + std::to_string(mainprogram->prelay->decresult->height), white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.045f), 0.0005f, 0.0008f);
									}
									else {
										if (mainprogram->prelay->type == ELEM_FILE) {
											render_text("CPU", white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.015f), 0.0005f, 0.0008f);
											render_text(std::to_string(mainprogram->prelay->decresult->width) + "x" + std::to_string(mainprogram->prelay->decresult->height), white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.045f), 0.0005f, 0.0008f);
										}
										if (mainprogram->prelay->type == ELEM_IMAGE) {
											render_text("IMAGE", white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.015f), 0.0005f, 0.0008f);
										}
									}
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
									render_text(std::to_string(mainprogram->prelay->decresult->width) + "x" + std::to_string(mainprogram->prelay->decresult->height), white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.045f), 0.0005f, 0.0008f);
								}
								else {
									if (mainprogram->prelay->type == ELEM_FILE) {
										render_text("CPU", white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.015f), 0.0005f, 0.0008f);
										render_text(std::to_string(mainprogram->prelay->decresult->width) + "x" + std::to_string(mainprogram->prelay->decresult->height), white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.045f), 0.0005f, 0.0008f);
									}
									if (mainprogram->prelay->type == ELEM_IMAGE) {
										render_text("IMAGE", white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.015f), 0.0005f, 0.0008f);
									}
								}
							}
						}
					}
					if (binel->type != ELEM_DECK and binel->type != ELEM_MIX and binel->path != "") {
						if (this->inserting == -1 and !this->inputtexes.size() and mainprogram->leftmousedown and !mainprogram->dragbinel) {
							mainprogram->dragbinel = binel;
							this->dragtex = binel->tex;
							mainprogram->leftmousedown = false;
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
								std::string path = mainprogram->project->binsdir + this->currbin->name + ".bin";
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
					}
					if (binel != this->currbinel or mainprogram->rightmouse) {
						if (this->currbinel) this->binpreview = false;
						if (this->inserting == 0 or this->inserting == 1) {
							if (this->prevbinel) {
								// reset previous position of inserted deck
								for (int k = 0; k < this->inserttexes[this->inserting].size(); k++) {
									BinElement* deckbinel = find_element(this->inserttexes[this->inserting].size(), k, this->ii, this->jj, 1);
									if (!deckbinel) {
										this->inserting = -1;
										break;
									}
									if (!deckbinel->full or (deckbinel->oldtype != ELEM_DECK and deckbinel->oldtype != ELEM_MIX)) {
										if (!this->movingstruct) {
											std::vector<Layer*>& lvec = choose_layers(this->inserting);
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
							}
							for (int k = 0; k < this->inserttexes[this->inserting].size(); k++) {
								int newi = i;
								if (mainprogram->rightmouse) newi = this->movingdeck->i;
								int newj = j;
								if (mainprogram->rightmouse) newj = this->movingdeck->j;
								BinElement* deckbinel = find_element(this->inserttexes[this->inserting].size(), k, newi, newj, 1);
								deckbinel->oldtex = deckbinel->tex;
								deckbinel->tex = this->inserttexes[this->inserting][k];
								//if (mainprogram->rightmouse) glDeleteTextures(1, &deckbinel->oldtex);
								if (this->movingstruct) {
									deckbinel->oldtype = deckbinel->type;
									deckbinel->type = this->inserttypes[this->inserting][k];
									deckbinel->oldpath = deckbinel->path;
									deckbinel->path = this->insertpaths[this->inserting][k];
									deckbinel->oldjpegpath = deckbinel->jpegpath;
									deckbinel->jpegpath = this->insertjpegpaths[this->inserting][k];
								}
								int pos = std::distance(this->currbin->elements.begin(), std::find(this->currbin->elements.begin(), this->currbin->elements.end(), deckbinel));
								this->ii = (int)((int)(pos / 24) / 3) * 3;
								this->jj = pos % 24;
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
						}
						if (this->inserting == 2) {
							int size = std::max(((int)((this->inserttexes[0].size() - 1) / 3) * 3 + 3), ((int)((this->inserttexes[1].size() - 1) / 3) * 3 + 3));
							if (this->prevbinel) {
								// reset previous position of inserted mix
								for (int m = 0; m < 2; m++) {
									for (int k = 0; k < (this->inserttexes[m].size() + 2) / 3 * 3; k++) {
										int newj = this->jj;
										if (this->jj > 23 - (int)(((this->inserttexes[m].size() + 2) / 3 * 3 - 1) / 3)) newj = this->jj - (int)(((this->inserttexes[m].size() + 2) / 3 * 3 - 1) / 3);
										BinElement* deckbinel = find_element((this->inserttexes[m].size() + 2) / 3 * 3, k, 0 + m * 3, newj, 1);
										if (!deckbinel) {
											this->inserting = -1;
											break;
										}
										if (!deckbinel->full or (deckbinel->oldtype != ELEM_DECK and deckbinel->oldtype != ELEM_MIX)) {
											deckbinel->tex = deckbinel->oldtex;
											if (this->movingstruct) {
												deckbinel->path = deckbinel->oldpath;
												deckbinel->jpegpath = deckbinel->oldjpegpath;
												deckbinel->type = deckbinel->oldtype;
												deckbinel->full = false;
											}
										}
									}
								}
							}
							for (int m = 0; m < 2; m++) {
								for (int k = 0; k < this->inserttexes[m].size(); k++) {
									int newj = j;
									if (j > 23 - (int)((size - 1) / 3)) newj = j - (int)((size - 1) / 3);
									if (mainprogram->rightmouse) newj = this->movingmix->j;
									BinElement* deckbinel = find_element(size, k, m * 3, newj, 1);
									if (!deckbinel) {
										this->inserting = -1;
										break;
									}
									if (!this->movingstruct) {
										std::vector<Layer*>& lvec = choose_layers(m);
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
									}
									int pos = std::distance(this->currbin->elements.begin(), std::find(this->currbin->elements.begin(), this->currbin->elements.end(), deckbinel));
									this->jj = pos % 24;
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
						}

						if (lay->vidmoving) {
							if (this->currbinel) {
								if (binel->type != ELEM_DECK and binel->type != ELEM_MIX) {
									this->currbinel->tex = this->currbinel->oldtex;
									binel->oldtex = binel->tex;
									binel->tex = this->dragtex;
								}
								else break;
							}
							else {
								if (lay->vidmoving) {
									binel->oldtex = binel->tex;
									binel->tex = this->dragtex;
								}
							}
							binel->full = true;
							this->currbinel = binel;
						}						
						if (this->inputtexes.size()) {
							bool cont = false;
							if (this->prevbinel) {
								int offset = 0;
								for (int k = 0; k < this->inputtexes.size(); k++) {
									int offj = (int)((this->previ + offset + k) / 6);
									int el = ((this->previ + offset + k + 144) % 6) * 24 + this->prevj + offj;
									if (this->prevj + offj > 23) {
										offset = k - this->inputtexes.size();
										cont = true;
										break;
									}
									BinElement *dirbinel = this->currbin->elements[el];
									while (dirbinel->type == ELEM_DECK or dirbinel->type == ELEM_MIX) {
										offset++;
										offj = (int)((this->previ + offset + k) / 6);
										el = ((this->previ + offset + k + 144) % 6) * 24 + this->prevj + offj;
										if (this->prevj + offj > 23) {
											offset = k - this->inputtexes.size();
											cont = true;
											break;
										}
										dirbinel = this->currbin->elements[el];
									}
									if (cont) break;
								}
								offset = 0;
								if (!cont) {
									for (int k = 0; k < this->inputtexes.size(); k++) {
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
									for (int k = 0; k < this->inputtexes.size(); k++) {
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
							for (int k = 0; k < this->inputtexes.size(); k++) {
								int offj = (int)((i + offset + k) / 6) - (i + offset + k < 0);
								int el = ((i + offset + k + 144) % 6) * 24 + j + offj;
								if (j + offj > 23) {
									offset = k - this->inputtexes.size();
									cont = true;
									break;
								}
								BinElement *dirbinel = this->currbin->elements[el];
								while (dirbinel->type == ELEM_DECK or dirbinel->type == ELEM_MIX) {
									offset++;
									offj = (int)((i + offset + k) / 6) - (i + offset + k < 0);
									el = ((i + offset + k + 144) % 6) * 24 + j + offj;
									if (j + offj > 23) {
										offset = k - this->inputtexes.size();
										cont = true;
										break;
									}
									dirbinel = this->currbin->elements[el];
								}
								if (cont) break;
							}
							offset = 0;
							if (!cont) {
								for (int k = 0; k < this->inputtexes.size(); k++) {
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
								for (int k = 0; k < this->inputtexes.size(); k++) {
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
									dirbinel->tex = this->inputtexes[this->inputtexes.size() - k - 1];
								}
							}
							this->prevbinel = binel;
							this->previ = i;
							this->prevj = j;
						}
					}
					if (this->currbinel and this->movingtex != -1) {
						if (binel != this->currbinel) {
							if (binel->type != ELEM_DECK and binel->type != ELEM_MIX) {
								bool temp = this->currbinel->full;
								this->currbinel->full = this->movingbinel->full;
								this->movingbinel->full = binel->full;
								this->currbinel->tex = this->currbinel->oldtex;
								this->movingbinel->tex = binel->tex;
								binel->oldtex = binel->tex;
								binel->tex = this->movingtex;
								binel->full = temp;
								this->currbinel = binel;
							}
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
						std::string path = mainprogram->project->binsdir + this->currbin->name + ".bin";
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
								BinElement *deckbinel = find_element(this->inserttexes[this->inserting].size(), k, this->movingdeck->i, this->movingdeck->j, 1);
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
							std::string path = mainprogram->project->binsdir + this->currbin->name + ".bin";
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
										deckbinel->full = true;
									}
								}
							}
							boost::filesystem::remove(this->movingmix->path);
							this->currbin->mixes.erase(std::find(this->currbin->mixes.begin(), this->currbin->mixes.end(), this->movingmix));
							delete this->movingmix;
							std::string path = mainprogram->project->binsdir + this->currbin->name + ".bin";
							save_bin(path);
						}
						this->movingstruct = false;
						this->inserting = -1;
					}
					mainprogram->del = false;
				}
				
				if (this->currbinel and this->inputtexes.size() and mainprogram->leftmouse) {
					for (int k = 0; k < this->inputtexes.size(); k++) {
						int intm = (144 - (this->previ + this->prevj * 6) - this->inputtexes.size());
						intm = (intm < 0) * intm;
						int jj = this->prevj + (int)((k + this->previ + intm) / 6) - ((k + this->previ + intm) < 0);
						int ii = ((k + intm + 144) % 6 + this->previ + 144) % 6;
						BinElement *dirbinel = this->currbin->elements[ii * 24 + jj];
						dirbinel->full = true;
						dirbinel->type = this->inputtypes[k];
						dirbinel->path = this->newpaths[k];
						dirbinel->jpegpath = "";
						dirbinel->oldjpegpath = "";
						//glDeleteTextures(1, &dirbinel->oldtex);
					}
					std::string path = mainprogram->project->binsdir + this->currbin->name + ".bin";
					save_bin(path);
					this->inputtexes.clear();
					this->currbinel = nullptr;
					this->inputtypes.clear();
					this->newpaths.clear();
					mainprogram->leftmouse = false;
				}


				if (this->currbinel and (this->inserting > -1) and (mainprogram->leftmouse or (mainprogram->lmsave and this->movingstruct))) {
					this->prevbinel = nullptr;
					std::string dirname = this->currbin->name;
					std::string name = this->currbin->name;
					std::string binname = name;
					std::string path;
					BinDeck* bd = nullptr;
					BinMix* bm = nullptr;
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
						mainmix->mousedeck = this->inserting;
						int count = 0;
						while (1) {
							if (this->inserting == 2) path = mainprogram->project->binsdir + dirname + "/" + name + ".mix";
							else path = mainprogram->project->binsdir + dirname + "/" + name + ".deck";
							if (!exists(path)) {
								break;
							}
							count++;
							name = remove_version(name) + "_" + std::to_string(count);
						}

						int size = std::max(((int)((this->inserttexes[0].size() - 1) / 3) * 3 + 3), ((int)((this->inserttexes[1].size() - 1) / 3) * 3 + 3));
						for (int d = startdeck; d < enddeck; d++) {
							std::vector<Layer*>& lvec = choose_layers(d);
							for (int k = 0; k < this->inserttexes[d].size(); k++) {
								BinElement* deckbinel;
								if (this->inserting == 2) {
									int newj = this->jj;
									if (this->jj > 23 - (int)((size - 1) / 3)) newj = this->jj - (int)((size - 1) / 3);
									deckbinel = find_element(size, k, d * 3, newj, 1);
								}
								else deckbinel = find_element(this->inserttexes[d].size(), k, this->ii, this->jj, 1);
								deckbinel->full = true;
								if (this->inserting == 2) deckbinel->type = ELEM_MIX;
								else deckbinel->type = ELEM_DECK;
								glDeleteTextures(1, &deckbinel->oldtex);
								std::string name = remove_extension(basename(lvec[k]->filename));
								int count = 0;
								while (1) {
									if (lvec[k]->filename == "") {
										deckbinel->path = mainprogram->project->binsdir + this->currbin->name + "/" + ".layer";
										mainmix->save_layerfile(deckbinel->path, lvec[k], 1, 1);
										break;
									}
									else {
										deckbinel->path = mainprogram->project->binsdir + this->currbin->name + "/" + name + ".layer";
										if (!exists(deckbinel->path)) {
											mainmix->save_layerfile(deckbinel->path, lvec[k], 1, 1);
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
							bm->path = path;
							bm->j = this->jj;
							bm->height = std::max((int)((this->inserttexes[0].size() - 1) / 3), (int)((this->inserttexes[1].size() - 1) / 3)) + 1;
							if (mainprogram->prevmodus) bm->tex = copy_tex(mainprogram->nodesmain->mixnodes[2]->mixtex);
							else bm->tex = copy_tex(mainprogram->nodesmain->mixnodescomp[2]->mixtex);
							bm->jpegpath = path + ".jpeg";
							save_thumb(bm->jpegpath, bm->tex);
							mainmix->save_mix(path);
						}
						else {
							bd = new BinDeck;
							bd->path = path;
							bd->i = (int)(this->ii / 3) * 3;
							bd->j = this->jj;
							bd->height = (int)((this->inserttexes[this->inserting].size() - 1) / 3) + 1;
							if (mainprogram->prevmodus) bd->tex = copy_tex(mainprogram->nodesmain->mixnodes[this->inserting]->mixtex);
							else bd->tex = copy_tex(mainprogram->nodesmain->mixnodescomp[this->inserting]->mixtex);
							bd->jpegpath = path + ".jpeg";
							save_thumb(bd->jpegpath, bd->tex);
							mainmix->save_deck(path);
						}
					}
					else {
						if (this->inserting == 0) {
							bd = this->movingdeck;
							bd->i = (int)(this->ii / 3) * 3;
							bd->j = this->jj;
							bd->height = (int)((this->inserttexes[0].size() - 1) / 3) + 1;
						}
						else {
							bm = this->movingmix;
							bm->j = this->jj;
							bm->height = std::max((int)((this->inserttexes[0].size() - 1) / 3), (int)((this->inserttexes[1].size() - 1) / 3)) + 1;
						}
					}

					this->movingstruct = false;
					if (this->inserting == 2) {
						if (std::find(this->currbin->mixes.begin(), this->currbin->mixes.end(), bm) == this->currbin->mixes.end()) {
							this->currbin->mixes.push_back(bm);
						}
					}
					else {
						if (std::find(this->currbin->decks.begin(), this->currbin->decks.end(), bd) == this->currbin->decks.end()) {
							this->currbin->decks.push_back(bd);
						}
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
					path = mainprogram->project->binsdir + this->currbin->name + ".bin";
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
					enddrag();
					mainprogram->leftmouse = false;
				}
			}
		}

		if (inbinel and lay->vidmoving and mainprogram->lmsave) {
			this->currbinel->full = true;
			this->currbinel->type = ELEM_LAYER;
			std::string name = remove_extension(basename(lay->filename));
			int count = 0;
			while (1) {
				this->currbinel->path = mainprogram->project->binsdir + this->currbin->name + "/" + name + ".layer";
				if (!exists(this->currbinel->path)) {
					mainmix->save_layerfile(this->currbinel->path, lay, 1, 0);
					break;
				}
				count++;
				name = remove_version(name) + "_" + std::to_string(count);
			}
			std::string path = mainprogram->project->binsdir + this->currbin->name + ".bin";
			save_bin(path);
			this->currbinel = nullptr;
			enddrag();
			lay->vidmoving = false;
			mainmix->moving = false;
		}
		else if (mainprogram->lmsave and mainprogram->dragbinel) {
			//when dropping on grey area
			if (lay->vidmoving) {
				if (this->currbinel) {
					this->currbinel->tex = this->currbinel->oldtex;
					this->currbinel = nullptr;
				}
			}
			else if (this->movingtex != -1) {
				bool found = false;
				for (int j = 0; j < 24; j++) {
					for (int i = 0; i < 6; i++) {
						Box* box = this->elemboxes[i * 24 + j];
						BinElement* be = this->currbin->elements[i * 24 + j];
						if (box->in() and this->currbinel == be) {
							found = true;
							break;
						}
					}
				}
				if (!found) {
					bool temp = this->currbinel->full;
					this->currbinel->full = this->movingbinel->full;
					this->movingbinel->full = temp;
					this->currbinel->tex = this->movingbinel->oldtex;
					this->movingbinel->tex = this->movingtex;
					this->currbinel = nullptr;
					this->movingbinel = nullptr;
					this->movingtex = -1;
				}
				else {
					std::string temppath = this->currbinel->path;
					ELEM_TYPE temptype = this->currbinel->type;
					this->currbinel->type = this->movingbinel->type;
					this->currbinel->path = this->movingbinel->path;
					this->currbinel->jpegpath = this->movingbinel->jpegpath;
					this->movingbinel->full = this->currbinel->full;
					this->movingbinel->path = temppath;
					this->movingbinel->type = temptype;
					this->currbinel = nullptr;
					this->movingbinel = nullptr;
					this->movingtex = -1;
					this->save_bin(mainprogram->project->binsdir + this->currbin->name + ".bin");
				}
			}
			enddrag();
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
				getline(rfile, istring);
				if (istring == "ENDOFDECKS") break;
				bd->jpegpath = istring;
				open_thumb(istring, bd->tex);
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
				getline(rfile, istring);
				if (istring == "ENDOFMIXES") break;
				bm->jpegpath = istring;
				open_thumb(istring, bm->tex);
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
		wfile << this->currbin->decks[i]->jpegpath;
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
		wfile << this->currbin->mixes[i]->jpegpath;
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
	path = mainprogram->project->binsdir + name + ".bin";
	if (!exists(path)) {
		save_bin(path);
		this->save_binslist();
	}
	boost::filesystem::path p1{mainprogram->project->binsdir + name};
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
	rfile.open(mainprogram->project->binsdir + "bins.list");
	if (!rfile.is_open()) {
		this->new_bin("this is a bin");
		this->save_binslist();
		return 0;
	}
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
	wfile.open(mainprogram->project->binsdir + "bins.list");
	wfile << "EWOC BINSLIST v0.1\n";
	wfile << std::to_string(this->currbin->pos);
	wfile << "\n";
	wfile << "BINS\n";
	for (int i = 0; i < this->bins.size(); i++) {
		std::string path = mainprogram->project->binsdir + this->bins[i]->name + ".bin";
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
		
		this->open_handlefile(str);
	}
	else {
		this->openbindir = false;
		mainprogram->blocking = false;
		SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW));
	} 
}

void BinsMain::open_handlefile(const std::string &path) {
	ELEM_TYPE endtype;
	GLuint endtex;
	if (isimage(path)) {
		// prepare image file for bin entry
		endtype = ELEM_IMAGE;
		endtex = get_imagetex(path);
	}
	else if (path.substr(path.length() - 6, std::string::npos) == ".layer") {
		// prepare layer file for bin entry
		endtype = ELEM_LAYER;
		endtex = get_layertex(path);
	}
	else {
		// prepare video file for bin entry
		endtype = ELEM_FILE;
		endtex = get_videotex(path);
	}
	if (endtex == -1) return;
	this->newpaths.push_back(path);
	this->inputtexes.push_back(endtex);
	this->inputtypes.push_back(endtype);
	this->prevbinel = nullptr;
}

void BinsMain::open_bindeck(const std::string& path) {
	this->movingdeck = new BinDeck;
	this->movingdeck->path = path;
	this->dragdeck = this->movingdeck;
	std::string result = deconcat_files(path);
	bool concat = (result != "");
	std::ifstream rfile;
	if (concat) rfile.open(result);
	else rfile.open(path);

	// read layers from deck to get at layerfilepaths, but dont load vids for speed
	std::vector<Layer*> layers;
	mainmix->add_layer(layers, false);
	layers[0]->dummy = true;
	layers[0]->closethread = true;
	mainmix->read_layers(rfile, result, layers, 0, 1, 1, concat, false, false, true);
	rfile.close();

	if (concat) rfile.open(result);
	else rfile.open(path);
	std::string istring;

	int jpegcount = 0;
	while (getline(rfile, istring)) {
		if (istring == "JPEGPATH") {
			getline(rfile, istring);
			GLuint tex;
			glGenTextures(1, &tex);
			glBindTexture(GL_TEXTURE_2D, tex);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			open_thumb(result + "_" + std::to_string(jpegcount) + ".file", tex);
			this->inserttexes[0].push_back(tex);
			this->inserttypes[0].push_back(ELEM_DECK);
			this->dragtexes[0].push_back(tex);
			this->insertpaths[0].push_back(layers[jpegcount]->layerfilepath);
			std::string jpegpath = "";
			std::string name = remove_extension(basename(layers[jpegcount]->layerfilepath));
			int count = 0;
			while (1) {
				jpegpath = mainprogram->temppath + name + ".jpeg";
				if (!exists(jpegpath)) {
					break;
				}
				count++;
				name = remove_version(name) + "_" + std::to_string(count);
			}
			boost::filesystem::rename(result + "_" + std::to_string(jpegcount) + ".file", jpegpath);
			this->insertjpegpaths[0].push_back(jpegpath);
			jpegcount++;
		}
		if (istring == "CLIPS") {
			while (getline(rfile, istring)) {
				if (istring == "ENDOFCLIPS") break;
				if (istring == "TYPE") jpegcount++;
			}
		}
	}

	this->movingstruct = true;
	this->inserting = 0;
	this->prevbinel = nullptr;
}

void BinsMain::open_binmix(const std::string& path) {
	this->movingmix = new BinMix;
	this->movingmix->path = path;
	this->dragmix = this->movingmix;
	std::string result = deconcat_files(path);
	bool concat = (result != "");
	std::ifstream rfile;
	if (concat) rfile.open(result);
	else rfile.open(path);

	// read layers from deck to get at layerfilepaths, but dont load vids for speed
	std::vector<Layer*> layersA;
	mainmix->add_layer(layersA, false);
	layersA[0]->dummy = true;
	layersA[0]->closethread = true;
	mainmix->read_layers(rfile, result, layersA, 0, 1, 1, concat, false, false, true);
	std::vector<Layer*> layersB;
	mainmix->add_layer(layersB, false);
	layersB[0]->dummy = true;
	layersB[0]->closethread = true;
	mainmix->read_layers(rfile, result, layersB, 1, 1, 1, concat, false, false, true);
	rfile.close();

	if (concat) rfile.open(result);
	else rfile.open(path);
	std::string istring;

	int jpegcount = 0;
	int totaljpegcount = 0;
	std::vector<Layer*> &layers = layersA;
	bool deck = 0;
	while (getline(rfile, istring)) {
		if (istring == "LAYERSB") {
			layers = layersB;
			deck = 1;
			jpegcount = 0;
		}
		if (istring == "JPEGPATH") {
			getline(rfile, istring);
			GLuint tex;
			glGenTextures(1, &tex);
			glBindTexture(GL_TEXTURE_2D, tex);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			open_thumb(result + "_" + std::to_string(totaljpegcount) + ".file", tex);
			this->inserttexes[deck].push_back(tex);
			this->inserttypes[deck].push_back(ELEM_MIX);
			this->dragtexes[deck].push_back(tex);
			this->insertpaths[deck].push_back(layers[jpegcount]->layerfilepath);
			std::string jpegpath = "";
			std::string name = remove_extension(basename(layers[jpegcount]->layerfilepath));
			int count = 0;
			while (1) {
				jpegpath = mainprogram->temppath + name + ".jpeg";
				if (!exists(jpegpath)) {
					break;
				}
				count++;
				name = remove_version(name) + "_" + std::to_string(count);
			}
			boost::filesystem::rename(result + "_" + std::to_string(totaljpegcount) + ".file", jpegpath);
			this->insertjpegpaths[deck].push_back(jpegpath);
			jpegcount++;
			totaljpegcount++;
		}
		if (istring == "CLIPS") {
			while (getline(rfile, istring)) {
				if (istring == "ENDOFCLIPS") break;
				if (istring == "TYPE") jpegcount++;
			}
		}
	}

	this->movingstruct = true;
	this->inserting = 2;
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
				mainprogram->encthreads--;
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
	std::string bpath = mainprogram->project->binsdir + this->currbin->name + ".bin";
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
	this->save_bin(mainprogram->project->binsdir + this->currbin->name + ".bin");
	this->open_bin(mainprogram->project->binsdir + this->currbin->name + ".bin", this->currbin);
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
	this->save_bin(mainprogram->project->binsdir + this->currbin->name + ".bin");
	this->open_bin(mainprogram->project->binsdir + this->currbin->name + ".bin", this->currbin);
}	

void BinsMain::hap_encode(const std::string srcpath, BinElement *binel, BinDeck *bd, BinMix *bm) {
	binel->encwaiting = true;
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
	source_dec_ctx->time_base = { 1, 1000 };
	source_dec_ctx->framerate = { source_stream->r_frame_rate.num, source_stream->r_frame_rate.den };
	r = avcodec_open2(source_dec_ctx, scodec, nullptr);
	if (source_dec_cpm->codec_id == 188 or source_dec_cpm->codec_id == 187) {
		binel->encwaiting = false;
		binel->encoding = false;
		if (bd) {
			bd->encthreads--;
		}
		else if (bm) {
			bm->encthreads--;
		}
		return;
	}

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
	
	numf = source_stream->nb_frames;
	if (numf == 0) {
		numf = (double)source->duration * (double)source_stream->avg_frame_rate.num / (double)source_stream->avg_frame_rate.den / (double)1000000.0f;
	}

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
	printf("ctb %d\n", c->time_base.num);
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

