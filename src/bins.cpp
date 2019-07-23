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
	// boxes of &bins in binslist
	this->box = new Box;
	this->box->vtxcoords->x1 = 0.57f;
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
	// return next bin element or nullptr (when at end)
	int pos = std::find(this->bin->elements.begin(), this->bin->elements.end(), this) - this->bin->elements.begin();
	int j = pos / 12;
	int i = pos / 12;
	if (i == 11) j++;
	else i++;
	int nxt = i * 12 + j;
	if (nxt == 144) return nullptr;
	else return this->bin->elements[nxt];
}

BinsMain::BinsMain() {
	for (int i = 0; i < 12; i++) {
		for (int j = 0; j < 12; j++) {
			// initialize all 144 bin elements
			Box *box = new Box;
			this->elemboxes.push_back(box);
			box->vtxcoords->x1 = -0.95f + i * 0.12f;
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
			box->tooltip = "Shows thumbnail of media bin element, either being a video file (grey border) or an image (pink border) or a layer file (orange border) or belonging to a deck file (purple border) or mix file (green border).  Hovering over this element shows video resolution and video compression method (CPU or HAP).  Mousewheel skips through the element contents (previewed in larger monitor topmiddle).  Leftdrag allows dragging to mix screen via wormhole.  Leftclick allows moving inside the media bin. Rightclickmenu allows among other things, HAP encoding. ";
		}
	}
	
	// box to click to add another bin to bins list
	this->newbinbox = new Box;
	this->newbinbox->vtxcoords->x1 = 0.57f;
	this->newbinbox->vtxcoords->w = 0.3f;
	this->newbinbox->vtxcoords->h = 0.05f;
	this->newbinbox->upvtxtoscr();
	this->newbinbox->tooltiptitle = "Add new bin ";
	this->newbinbox->tooltip = "Leftclick to add a new bin and make it current. ";
	
	this->currbin = new Bin(-1);
	
	this->hapmodebox = new Box;
	this->hapmodebox->vtxcoords->x1 = 0.67f;
	this->hapmodebox->vtxcoords->y1 = 0.65f;
	this->hapmodebox->vtxcoords->w = 0.1f;
	this->hapmodebox->vtxcoords->h = 0.075f;
	this->hapmodebox->upvtxtoscr();
	this->hapmodebox->tooltiptitle = "HAP encode modus ";
	this->hapmodebox->tooltip = "Toggles between hap encoding for during live situations (only using one core, not to slow down the realtime video mix, and hap encoding at full power (using 'number of system cores + 1' threads). ";

	// arrow box to scroll bins list up
	this->binsscrollup = new Box;
	this->binsscrollup->vtxcoords->x1 = 0.545f;
	this->binsscrollup->vtxcoords->y1 = -0.05f;
	this->binsscrollup->vtxcoords->w = 0.025f;
	this->binsscrollup->vtxcoords->h = 0.05f;
	this->binsscrollup->upvtxtoscr();
	this->binsscrollup->tooltiptitle = "Scroll bins list up ";
	this->binsscrollup->tooltip = "Leftclicking scrolls the bins list up ";

	// arrow box to scroll bins list down
	this->binsscrolldown = new Box;
	this->binsscrolldown->vtxcoords->x1 = 0.545f;
	this->binsscrolldown->vtxcoords->y1 = -1.0f;
	this->binsscrolldown->vtxcoords->w = 0.025f;
	this->binsscrolldown->vtxcoords->h = 0.05f;
	this->binsscrolldown->upvtxtoscr();
	this->binsscrolldown->tooltiptitle = "Scroll bins list down ";
	this->binsscrolldown->tooltip = "Leftclicking scrolls the bins list down ";
}

void BinsMain::handle(bool draw) {
	float white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	float black[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float darkgrey[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	float lightblue[] = { 0.5f, 0.5f, 1.0f, 1.0f };
	float red[] = { 1.0f, 0.5f, 0.5f, 1.0f };
	float lightgreen[] = { 0.5f, 1.0f, 0.5f, 1.0f };
	float yellow[] = { 1.0f, 1.0f, 0.0f, 1.0f };

	GLint inverted = glGetUniformLocation(mainprogram->ShaderProgram, "inverted");

	//draw binelements
	if (!mainprogram->menuondisplay) this->menubinel = nullptr;
	if (mainprogram->menuactivation) mainprogram->menuset = 0;
	if (draw) {
		for (int i = 0; i < 12; i++) {
			for (int j = 0; j < 12; j++) {
				Box* box = this->elemboxes[i * 12 + j];
				BinElement* binel = this->currbin->elements[i * 12 + j];
				if (box->in()) {
					this->menubinel = binel;
					if (mainprogram->menuactivation) {
						if (binel->path != "") {
							// Set menu when over non-empty element
							std::vector<std::string> binel;
							binel.push_back("Delete element");
							binel.push_back("Open file(s) from disk");
							binel.push_back("Insert deck A");
							binel.push_back("Insert deck B");
							binel.push_back("Insert full mix");
							binel.push_back("Load block in shelf A");
							binel.push_back("Load block in shelf B");
							binel.push_back("HAP encode element");
							binel.push_back("Quit");
							mainprogram->make_menu("binelmenu", mainprogram->binelmenu, binel);
							mainprogram->menuset = 1;
						}
						if (binel->type == ELEM_DECK) {
							// set binelmenu entries when mouse over deck
							std::vector<std::string> binel;
							binel.push_back("Load in deck A");
							binel.push_back("Load in deck B");
							binel.push_back("Delete deck");
							binel.push_back("Open file(s) from disk");
							binel.push_back("Insert deck A");
							binel.push_back("Insert deck B");
							binel.push_back("Insert full mix");
							binel.push_back("Load block in shelf A");
							binel.push_back("Load block in shelf B");
							binel.push_back("HAP encode deck");
							binel.push_back("Quit");
							mainprogram->make_menu("binelmenu", mainprogram->binelmenu, binel);
							mainprogram->menuset = 3;
						}
						else if (binel->type == ELEM_MIX) {
							// set binelmenu entries when mouse over mix
							std::vector<std::string> binel;
							binel.push_back("Load in mix");
							binel.push_back("Delete mix");
							binel.push_back("Open file(s) from disk");
							binel.push_back("Insert deck A");
							binel.push_back("Insert deck B");
							binel.push_back("Insert full mix");
							binel.push_back("Load block in shelf A");
							binel.push_back("Load block in shelf B");
							binel.push_back("HAP encode mix");
							binel.push_back("Quit");
							mainprogram->make_menu("binelmenu", mainprogram->binelmenu, binel);
							mainprogram->menuset = 2;
						}
					}
				}

				// color code elements
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
				else if (binel->type == ELEM_IMAGE) {
					color[0] = 1.0f;
					color[1] = 0.5f;
					color[2] = 0.5f;
					color[3] = 1.0f;
				}
				else {
					color[0] = 0.4f;
					color[1] = 0.4f;
					color[2] = 0.4f;
					color[3] = 1.0f;
				}
				// visualize elements
				draw_box(nullptr, color, box->vtxcoords->x1 - 0.01f, box->vtxcoords->y1 - 0.01f, box->vtxcoords->w + 0.02f, box->vtxcoords->h + 0.02f, -1);
				draw_box(box, -1);  //in case of alpha thumbnail
				if (binel->select) {
					glUniform1i(inverted, 1);
				}
				draw_box(box, binel->tex);
				glUniform1i(inverted, 0);
				if (binel->path != "") {
					if (remove_extension(basename(binel->path)) != "") render_text(basename(binel->path).substr(0, 20), white, box->vtxcoords->x1, box->vtxcoords->y1 - 0.02f, 0.00045f, 0.00075f);
				}
			}
			// draw big grey areas next to each element column to cut off element titles
			Box* box = this->elemboxes[i * 12];
			draw_box(nullptr, darkgrey, box->vtxcoords->x1 + box->vtxcoords->w, -1.0f, 0.12f, 2.0f, -1);
		}

		if (!mainprogram->menuondisplay) this->mouseshelfnum = -1;
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				Box box;
				box.vtxcoords->x1 = -0.965f + i * 0.48f;
				box.vtxcoords->y1 = 0.925f - ((j % 3) + 1) * 0.6f;
				box.vtxcoords->w = 0.48f;
				box.vtxcoords->h = 0.6f;
				box.upvtxtoscr();
				draw_box(yellow, nullptr, &box, -1);
				if (box.in()) {
					this->mouseshelfnum = i * 3 + j;
					if (mainprogram->menuset == 0 and mainprogram->menuactivation) {
						// set binelmenu entries when mouse in shelf blocks box but not on bin element
						std::vector<std::string> binel;
						binel.push_back("Open file(s) from disk");
						binel.push_back("Insert deck A");
						binel.push_back("Insert deck B");
						binel.push_back("Insert full mix");
						binel.push_back("Load block in shelf A");
						binel.push_back("Load block in shelf B");
						binel.push_back("HAP encode entire bin");
						binel.push_back("Quit");
						mainprogram->make_menu("binelmenu", mainprogram->binelmenu, binel);
					}
				}
			}
		}

		if (this->mouseshelfnum == -1 and mainprogram->menuset == 0 and mainprogram->menuactivation) {
			// set binelmenu entries when mouse over nothing
			std::vector<std::string> binel;
			binel.push_back("Open file(s) from disk");
			binel.push_back("Insert deck A");
			binel.push_back("Insert deck B");
			binel.push_back("Insert full mix");
			binel.push_back("HAP encode entire bin");
			binel.push_back("Quit");
			mainprogram->make_menu("binelmenu", mainprogram->binelmenu, binel);
		}

		if (mainprogram->menuset == 0) {
			// not in bin element -> leftmouse draws selection box
			bool full = false;
			if (this->menubinel) {
				if (this->menubinel->full) full = true;
			}
			if (mainprogram->leftmousedown and !full and !this->inputtexes.size() and !mainprogram->intopmenu) {
				mainprogram->leftmousedown = false;
				if (!this->selboxing) {
					this->selboxing = true;
					this->selboxx = mainprogram->mx;
					this->selboxy = mainprogram->my;
					for (int i = 0; i < 12; i++) {
						for (int j = 0; j < 12; j++) {
							BinElement* binel = this->currbin->elements[i * 12 + j];
							binel->select = false;
						}
					}
				}
			}
			if (this->selboxing) {
				Box box;
				box.vtxcoords->x1 = -1.0f + mainprogram->xscrtovtx(this->selboxx);
				box.vtxcoords->h = mainprogram->yscrtovtx(mainprogram->my - this->selboxy);
				box.vtxcoords->y1 = 1.0f - mainprogram->yscrtovtx(this->selboxy);
				box.vtxcoords->y1 -= box.vtxcoords->h;
				box.vtxcoords->w = mainprogram->xscrtovtx(mainprogram->mx - this->selboxx);
				box.upvtxtoscr();
				draw_box(white, nullptr, &box, -1);
				bool found = false;
				// select bin elements entirely inside selection box
				for (int i = 0; i < 12; i++) {
					for (int j = 0; j < 12; j++) {
						Box* ebox = this->elemboxes[i * 12 + j];
						BinElement* binel = this->currbin->elements[i * 12 + j];
						if (!binel->full) continue;
						binel->select = false;
						if (box.in(ebox->scrcoords->x1, ebox->scrcoords->y1)) {
							if (box.in(ebox->scrcoords->x1 + ebox->scrcoords->w, ebox->scrcoords->y1 - ebox->scrcoords->h)) {
								binel->select = true;
								found = true;
							}
						}
					}
				}
				if (mainprogram->leftmouse) {
					if (found) mainprogram->binselmenu->state = 2;
					this->selboxing = false;
					mainprogram->leftmouse = false;
				}
			}
		}
	}


	// set threadmode for hap encoding
	render_text("HAP Encoding Mode", white, 0.62f, 0.8f, 0.00075f, 0.0012f);
	draw_box(white, black, binsmain->hapmodebox, -1);
	draw_box(white, lightblue, 0.67f + 0.048f * mainprogram->threadmode, 0.6575f, 0.048f, 0.06f, -1);
	render_text("Live mode", white, 0.59f, 0.6f, 0.00075f, 0.0012f);
	render_text("Max mode", white, 0.75f, 0.6f, 0.00075f, 0.0012f);
	render_text("1 thread", white, 0.59f, 0.55f, 0.00075f, 0.0012f);
	mainprogram->maxthreads = mainprogram->numcores * mainprogram->threadmode + 1;
	render_text(std::to_string(mainprogram->numcores + 1) + " threads", white, 0.75f, 0.55f, 0.00075f, 0.0012f);
	if (binsmain->hapmodebox->in()) {
		if (mainprogram->leftmouse) {
			mainprogram->threadmode = !mainprogram->threadmode;
			mainprogram->leftmouse = false;
		}
	}

	// set lay to current layer or start layer
	Layer *lay = nullptr;		
	if (mainmix->currlay) lay = mainmix->currlay;
	else {
		if (mainprogram->prevmodus) lay = mainmix->layersA[0];
		else lay = mainmix->layersAcomp[0];
	}
		
	if (mainprogram->rightmouse and this->currbinel and !mainprogram->dragbinel) {
		// rightmouse cancelling of file opening
		bool cont = false;
		if (this->prevbinel) {
			// 
			if (this->inputtexes.size()) {
				int offset = 0;
				for (int k = 0; k < this->inputtexes.size(); k++) {
					// when opening elements, calculate offset when first element past bin start or end
					int offj = (int)((this->previ + offset + k) / 12);
					int el = ((this->previ + offset + k + 144) % 12) * 12 + this->prevj + offj;
					if (this->prevj + offj > 11) {
						offset = k - this->inputtexes.size();
						cont = true;
						break;
					}
					BinElement *dirbinel = this->currbin->elements[el];
					if (cont) break;
				}
				offset = 0;
				if (!cont) {
					for (int k = 0; k < this->inputtexes.size(); k++) {
						int offj = (int)((this->previ + offset + k) / 12);
						int el = ((this->previ + offset + k + 144) % 12) * 12 + this->prevj + offj;
						BinElement *dirbinel = this->currbin->elements[el];
						// reset texes to cancel input
						dirbinel->tex = dirbinel->oldtex;
						glDeleteTextures(1, &this->inputtexes[k]);
					}
				}
				else {
					for (int k = 0; k < this->inputtexes.size(); k++) {
						int offj = (int)((143 - k + offset) / 12) - (143 - k + offset < 0);
						int el = ((143 - k + offset) % 12) * 12 + offj;
						BinElement *dirbinel = this->currbin->elements[el];
						// reset texes to cancel input
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
		//handle binslist scroll
		Box binsbox;
		binsbox.vtxcoords->x1 = 0.57f;
		binsbox.vtxcoords->y1 = -1.0f;
		binsbox.vtxcoords->w = 0.3f;
		binsbox.vtxcoords->h = 1.0f;
		binsbox.upvtxtoscr();
		draw_box(red, black, &binsbox, -1);
		if (binsbox.in()) {
			// mousewheel scroll
			this->binsscroll -= mainprogram->mousewheel;
			if (this->binsscroll < 0) this->binsscroll = 0;
			if (this->bins.size() > 20 and this->bins.size() - this->binsscroll < 20) this->binsscroll = this->bins.size() - 19;
		}

		// draw and handle binslist scrollboxes
		if (this->binsscroll > 0) {
			if (this->binsscrollup->in()) {
				this->binsscrollup->acolor[0] = 0.5f;
				this->binsscrollup->acolor[1] = 0.5f;
				this->binsscrollup->acolor[2] = 1.0f;
				this->binsscrollup->acolor[3] = 1.0f;
				if (mainprogram->leftmouse) {
					this->binsscroll--;
					this->save_binslist();
				}
			}
			else {
				this->binsscrollup->acolor[0] = 0.0f;
				this->binsscrollup->acolor[1] = 0.0f;
				this->binsscrollup->acolor[2] = 0.0f;
				this->binsscrollup->acolor[3] = 1.0f;
			}
			draw_box(this->binsscrollup, -1);
			draw_triangle(white, white, this->binsscrollup->vtxcoords->x1 + 0.0074f, this->binsscrollup->vtxcoords->y1 + 0.0416f - 0.030f, 0.011f, 0.0208f, DOWN, CLOSED);
		}
		if (this->bins.size() - this->binsscroll > 19) {
			if (this->binsscrolldown->in()) {
				this->binsscrolldown->acolor[0] = 0.5f;
				this->binsscrolldown->acolor[1] = 0.5f;
				this->binsscrolldown->acolor[2] = 1.0f;
				this->binsscrolldown->acolor[3] = 1.0f;
				if (mainprogram->leftmouse) {
					this->binsscroll++;
					this->save_binslist();
				}
			}
			else {
				this->binsscrolldown->acolor[0] = 0.0f;
				this->binsscrolldown->acolor[1] = 0.0f;
				this->binsscrolldown->acolor[2] = 0.0f;
				this->binsscrolldown->acolor[3] = 1.0f;
			}
			draw_box(this->binsscrolldown, -1);
			draw_triangle(white, white, this->binsscrolldown->vtxcoords->x1 + 0.0074f, this->binsscrolldown->vtxcoords->y1 + 0.0416f - 0.030f, 0.011f, 0.0208f, UP, CLOSED);
		}

		//draw and handle binslist
		this->indragbox = false;
		for (int i = 0; i < this->bins.size() - this->binsscroll; i++) {
			Bin* bin = this->bins[i + this->binsscroll];
			bin->pos = i;
			bin->box->vtxcoords->y1 = (i + 1) * -0.05f;
			bin->box->upvtxtoscr();
			if (bin->box->in() and !this->dragbin) {
				if (mainprogram->leftmousedown and !this->dragbinsense) {
					this->dragbinsense = true;
					this->dragbox = bin->box;
					this->dragbinpos = i + this->binsscroll;
					mainprogram->leftmousedown = false;
				}
				if (mainprogram->leftmouse) {
					// click to choose current bin
					make_currbin(i);
					this->dragbox = nullptr;
					this->dragbinsense = false;
					mainprogram->leftmouse = false;
				}
				if (i + this->binsscroll == this->dragbinpos) {
					// mouse over box thats being dragged: first stage drag startup, allows user to just choose current bin without starting drag
					this->indragbox = true;
				}
				bin->box->acolor[0] = 0.5f;
				bin->box->acolor[1] = 0.5f;
				bin->box->acolor[2] = 1.0f;
				bin->box->acolor[3] = 1.0f;
			}
			else if (i == this->currbin->pos) {
				// curent bin colored differently
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
				// bin renaming with keyboard
				std::string part = mainprogram->inputtext.substr(0, mainprogram->cursorpos);
				float textw = render_text(part, white, bin->box->vtxcoords->x1 + tf(0.01f), bin->box->vtxcoords->y1 + tf(0.012f), tf(0.0003f), tf(0.0005f));
				part = mainprogram->inputtext.substr(mainprogram->cursorpos, mainprogram->inputtext.length() - mainprogram->cursorpos);
				render_text(part, white, bin->box->vtxcoords->x1 + tf(0.01f) + textw, bin->box->vtxcoords->y1 + tf(0.012f), tf(0.0003f), tf(0.0005f));
				draw_line(white, bin->box->vtxcoords->x1 + tf(0.011f) + textw, bin->box->vtxcoords->y1 + tf(0.01f), bin->box->vtxcoords->x1 + tf(0.011f) + textw, bin->box->vtxcoords->y1 + tf(0.028f));
			}
			else {
				render_text(bin->name, white, bin->box->vtxcoords->x1 + tf(0.01f), bin->box->vtxcoords->y1 + tf(0.012f), tf(0.0003f), tf(0.0005f));
			}
		}
		if (!this->indragbox and this->dragbinsense) {
			// dragging has moved (!this->draginbox) so start doing it
			this->dragbin = this->bins[this->dragbinpos];
			this->dragbinsense = false;
		}

		// draw and handle box that allows adding a new bin to the end of the list0
		Box* box = this->newbinbox;
		box->vtxcoords->y1 = -1.0f;
		box->upvtxtoscr();
		if (box->in()) {
			if (mainprogram->leftmouse and !this->dragbin) {
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
				if (this->bins.size() >= 20) this->binsscroll++;
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
		render_text("+ NEW BIN", red, 0.62f, -1.0f + 0.018f, tf(0.0003f), tf(0.0005f));

		// handle bin drag in binslist
		if (this->dragbin) {
			int pos;
			for (int i = this->binsscroll; i < this->bins.size() + 1; i++) {
				// calculate dragged bin temporary position pos when mouse between
				//limits under and upper
				int under1, under2, upper;
				if (i == this->binsscroll) {
					// mouse above first bin
					under2 = 0;
					under1 = this->bins[this->binsscroll]->box->scrcoords->y1 - this->bins[this->binsscroll]->box->scrcoords->h * 0.5f;
				}
				else {
					under1 = this->bins[i - 1]->box->scrcoords->y1 + this->bins[i - 1]->box->scrcoords->h * 0.5f;
					under2 = under1;
				}
				if (i == this->bins.size()) {
					// mouse under last bin
					upper = glob->h;
				}
				else upper = this->bins[i]->box->scrcoords->y1 + this->bins[i]->box->scrcoords->h * 0.5f;
				if (mainprogram->my > under2 and mainprogram->my < upper) {
					// draw dragged box
					draw_box(white, black, this->dragbin->box->vtxcoords->x1, 1.0f - mainprogram->yscrtovtx(under1), this->dragbin->box->vtxcoords->w, this->dragbin->box->vtxcoords->h, -1);
					render_text(this->dragbin->name, white, this->dragbin->box->vtxcoords->x1 + tf(0.01f), 1.0f - mainprogram->yscrtovtx(under1) + tf(0.012f), tf(0.0003f), tf(0.0005f));
					pos = i;
					break;
				}
			}
			if (mainprogram->leftmouse) {
				// do bin drag
				if (this->dragbinpos < pos) {
					std::rotate(this->bins.begin() + this->dragbinpos, this->bins.begin() + this->dragbinpos + 1, this->bins.begin() + pos);
				}
				else {
					std::rotate(this->bins.begin() + pos, this->bins.begin() + this->dragbinpos, this->bins.begin() + this->dragbinpos + 1);
				}
				for (int i = 0; i < this->bins.size(); i++) {
					// set pos and box y1 for all bins in new list
					this->bins[i]->pos = i;
					this->bins[i]->box->vtxcoords->y1 = (i + 1) * -0.05f;
					this->bins[i]->box->upvtxtoscr();
				}
				this->dragbin = nullptr;
				this->save_binslist();
				mainprogram->leftmouse = false;
			}
			if (mainprogram->rightmouse) {
				// cancel bin drag
				this->dragbin = nullptr;
				mainprogram->rightmouse = false;
				mainprogram->menuactivation = false;
				for (int i = 0; i < mainprogram->menulist.size(); i++) {
					mainprogram->menulist[i]->state = 0;
				}
			}
		}


		// Draw and handle binselmenu	
		int k = handle_menu(mainprogram->binselmenu);
		if (k == 0) {
			// delete selected bin elements
			for (int i = 0; i < 12; i++) {
				for (int j = 0; j < 12; j++) {
					BinElement* binel = this->currbin->elements[i * 12 + j];
					if (binel->select) {
						this->delbinels.push_back(binel);
					}
				}
			}
			mainprogram->del = true;
		}

		// Draw and handle binmenu and binelmenu	
		if (this->bins.size() > 1) {
			int k = handle_menu(mainprogram->binmenu);
			if (k == 0) {
				// delete bin
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
				// start renaming bin
				mainprogram->backupname = this->menubin->name;
				mainprogram->inputtext = this->menubin->name;
				mainprogram->cursorpos = mainprogram->inputtext.length();
				SDL_StartTextInput();
				mainprogram->renaming = EDIT_BINNAME;
			}
		}
		else {
			int k = handle_menu(mainprogram->bin2menu);  // rightclick on bin when there's only one (can't delete)
			if (k == 0) {
				// start renaming bin
				mainprogram->backupname = this->menubin->name;
				mainprogram->inputtext = this->menubin->name;
				mainprogram->cursorpos = mainprogram->inputtext.length();
				SDL_StartTextInput();
				mainprogram->renaming = EDIT_BINNAME;
			}
		}
	}


	// handle binelmenu thats been populated above, menuset controls which options sets are used
	int k = handle_menu(mainprogram->binelmenu);
	//if (k > -1) this->currbinel = nullptr;
	if (k == 0 and mainprogram->menuset == 1) {
		// delete hovered bin element
		this->delbinels.push_back(this->menubinel);
		mainprogram->del = true;
	}
	else if (k == 0 and mainprogram->menuset == 3) {
		// load hovered deck in deck A
		mainprogram->binsscreen = false;
		mainmix->mousedeck = 0;
		mainmix->open_deck(this->menubinel->path, 1);
	}
	else if (k == 1 and mainprogram->menuset == 3) {
		// load hovered deck in deck B
		mainprogram->binsscreen = false;
		mainmix->mousedeck = 1;
		mainmix->open_deck(this->menubinel->path, 1);
	}
	else if (k == 2 and mainprogram->menuset == 3) {
		// delete hovered deck
		this->delbinels.push_back(this->menubinel);
		mainprogram->del = true;
	}
	else if (k == 0 and mainprogram->menuset == 2) {
		// load hovered mix into decks
		mainprogram->binsscreen = false;
		mainmix->open_mix(this->menubinel->path.c_str());
	}
	else if (k == 1 and mainprogram->menuset == 2) {
		// delete hovered mix
		this->delbinels.push_back(this->menubinel);
		mainprogram->del = true;
	}
	else if (k == mainprogram->menuset) {
		// open videos/images/layer files into bin
		mainprogram->pathto = "OPENBINFILES";
		std::thread filereq(&Program::get_multinname, mainprogram, "Open file(s)", boost::filesystem::canonical(mainprogram->currvideodir).generic_string());
		filereq.detach();
	}
	else if (k == mainprogram->menuset + 1) {
		// insert deck A into bin
		mainprogram->paths.clear();
		mainmix->mousedeck = 0;
		std::string name = "deckA";
		std::string path;
		int count = 0;
		while (1) {
			path = mainprogram->temppath + name + ".deck";
			if (!exists(path)) {
				break;
			}
			count++;
			name = remove_version(name) + "_" + std::to_string(count);
		}
		mainmix->do_save_deck(path, true, true);
		mainprogram->paths.push_back(path);
		mainprogram->counting = 0;
		this->openbinfile = true;
		mainprogram->lmsave = false;
		this->prevbinel = nullptr;
	}
	else if (k == mainprogram->menuset + 2) {
		// insert deck B into bin
		mainprogram->paths.clear();
		mainmix->mousedeck = 1;
		std::string name = "deckB";
		std::string path;
		int count = 0;
		while (1) {
			path = mainprogram->temppath + name + ".deck";
			if (!exists(path)) {
				break;
			}
			count++;
			name = remove_version(name) + "_" + std::to_string(count);
		}
		mainmix->do_save_deck(path, true, true);
		mainprogram->paths.push_back(path);
		mainprogram->counting = 0;
		this->openbinfile = true;
		mainprogram->lmsave = false;
		this->prevbinel = nullptr;
	}
	else if (k == mainprogram->menuset + 3) {
		// insert live mix into bin
		mainprogram->paths.clear();
		std::string name = "mix";
		std::string path;
		int count = 0;
		while (1) {
			path = mainprogram->temppath + name + ".mix";
			if (!exists(path)) {
				break;
			}
			count++;
			name = remove_version(name) + "_" + std::to_string(count);
		}
		mainmix->do_save_mix(path, mainprogram->prevmodus, true);
		mainprogram->paths.push_back(path);
		mainprogram->counting = 0;
		this->openbinfile = true;
		mainprogram->lmsave = false;
		this->prevbinel = nullptr;
	}
	else if (k == mainprogram->menuset + 4 and this->mouseshelfnum != -1) {
	Shelf* shelf = mainprogram->shelves[0];
	for (int i = 0; i < 16; i++) {
		BinElement* binel = this->currbin->elements[this->mouseshelfnum / 3 * 48 + (this->mouseshelfnum % 3) * 4 + i / 4 + (i % 4) * 12];
		ShelfElement* elem = shelf->elements[i];
		elem->path = binel->path;
		elem->jpegpath = binel->jpegpath;
		elem->type = binel->type;
		elem->tex = copy_tex(binel->tex);
	}
	}
	else if (k == mainprogram->menuset + 5 and this->mouseshelfnum != -1) {
	Shelf* shelf = mainprogram->shelves[1];
	for (int i = 0; i < 16; i++) {
		BinElement* binel = this->currbin->elements[this->mouseshelfnum / 3 * 48 + (this->mouseshelfnum % 3) * 4 + i / 4 + (i % 4) * 12];
		ShelfElement* elem = shelf->elements[i];
		elem->path = binel->path;
		elem->jpegpath = binel->jpegpath;
		elem->type = binel->type;
		elem->tex = copy_tex(binel->tex);
	}
	}
	else if (k == 7 and mainprogram->menuset == 1) {
		// hap encode hovered bin element
		this->hap_binel(this->menubinel, nullptr);
	}
	else if (k == 9 and mainprogram->menuset == 3) {
		// hap encode hovered deck
		this->hap_deck(this->menubinel);
	}
	else if (k == 8 and mainprogram->menuset == 2) {
		// hap encode hovered mix
		this->hap_mix(this->menubinel);
	}
	else if (k == 4 + (this->mouseshelfnum != -1) * 2 and mainprogram->menuset == 0) {
		// hap encode entire bin
		for (int i = 0; i < 12; i++) {
			for (int j = 0; j < 12; j++) {
				// elements
				BinElement* binel = this->currbin->elements[i * 12 + j];
				if (binel->full and (binel->type == ELEM_FILE or binel->type == ELEM_LAYER)) {
					this->hap_binel(binel, nullptr);
				}
				else if (binel->type == ELEM_DECK) {
					this->hap_deck(this->menubinel);
				}
				else if (binel->type == ELEM_MIX) {
					this->hap_mix(this->menubinel);
				}
			}
		}
	}
	else if (k == mainprogram->binelmenu->entries.size() - 1) {
		// quit program
		mainprogram->quit("quitted");
	}

	if (mainprogram->menuchosen) {
		// menu cleanup
		mainprogram->menuchosen = false;
		mainprogram->leftmouse = 0;
		mainprogram->menuactivation = 0;
		mainprogram->menuresults.clear();
		mainprogram->menuondisplay = false;
	}


	if (mainprogram->menuactivation and !this->inputtexes.size() and !lay->vidmoving and this->movingtex == -1) {
		// activate binslist or bin menu
		int dt = 0;
		int mt = 0;
		if (this->menubinel and mainprogram->menuset == 3) {
			dt = this->menubinel->encthreads;
		}
		else if (this->menubinel and mainprogram->menuset == 2) {
			mt = this->menubinel->encthreads;
		}
		if (dt == 0 and mt == 0) {
			// only when not encoding
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
	
	//big handle binelements block
	if (!mainprogram->menuondisplay) {
		this->prevelems.clear();
		bool inbinel = false;
		for (int j = 0; j < 12; j++) {
			for (int i = 0; i < 12; i++) {
				// handle elements, row per row
				Box* box = this->elemboxes[i * 12 + j];
				box->upvtxtoscr();
				BinElement* binel = this->currbin->elements[i * 12 + j];
				if (draw) {
					// show if element encoding/awaiting encoding
					if (binel->encwaiting) {
						render_text("Waiting...", white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.015f), 0.0005f, 0.0008f);
					}
					else if (binel->encoding) {
						float progress = binel->encodeprogress;
						if (binel->type == ELEM_DECK or binel->type == ELEM_MIX) {
							if (binel->allhaps) {
								progress /= binel->allhaps;
							}
							else progress = 0.0f;
						}
						render_text("Encoding...", white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.015f), 0.0005f, 0.0008f);
						draw_box(black, white, box->vtxcoords->x1, box->vtxcoords->y1 + box->vtxcoords->h - tf(0.045f), progress * 0.1f, 0.02f, -1);
					}
				}
				if (binel->encoding and binel->encthreads == 0 and (binel->type == ELEM_DECK or binel->type == ELEM_MIX)) {
					binel->encoding = false;
					boost::filesystem::rename(remove_extension(binel->path) + ".temp", binel->path);
					this->do_save_bin(mainprogram->project->binsdir + this->currbin->name + ".bin");
				}
				if ((box->in() or mainprogram->rightmouse) and !binel->encoding) {
					if (draw) {
						inbinel = true;
						// don't preview when encoding
						if (this->currbin->encthreads) continue;
						if (this->menubinel) {
							if (this->menubinel->encthreads) continue;
						}
						else if (this->menubinel) {
							if (this->menubinel->encthreads) continue;
						}
						if (!binel->encwaiting and !binel->encoding and binel->full and !this->inputtexes.size() and !lay->vidmoving) {
							if (this->previewbinel != binel) {
								// reset when new element hovered
								this->previewimage = "";
								this->previewbinel = nullptr;
							}
							if ((this->previewimage != "" or binel->type == ELEM_IMAGE) and !this->binpreview) {
								// do first entry preview preperation/visualisation when image hovered
								this->binpreview = true;  // just entering preview, or already done preparation (different if clauses)
								if (mainprogram->prelay) {
									// close old preview layer
									mainprogram->prelay->closethread = true;
									while (mainprogram->prelay->closethread) {
										mainprogram->prelay->ready = true;
										mainprogram->prelay->startdecode.notify_one();
									}
									//delete mainprogram->prelay;
								}
								draw_box(red, black, 0.52f, 0.5f, 0.4f, 0.4f, -1);
								mainprogram->prelay = new Layer(false);
								mainprogram->prelay->dummy = true;
								if (this->previewimage != "") mainprogram->prelay->open_image(this->previewimage);
								else mainprogram->prelay->open_image(binel->path);
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
								draw_box(red, black, 0.52f, 0.5f, 0.4f, 0.4f, mainprogram->prelay->texture);
								render_text("IMAGE", white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.015f), 0.0005f, 0.0008f);
								render_text(std::to_string(w) + "x" + std::to_string(h), white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.045f), 0.0005f, 0.0008f);
							}
							else if (this->previewimage != "" or binel->type == ELEM_IMAGE) {
								// do second entry preview visualisation when image hovered
								if (mainprogram->mousewheel) {
									// mousewheel leaps through animated gifs
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
								draw_box(red, black, 0.52f, 0.5f, 0.4f, 0.4f, mainprogram->prelay->texture);
								render_text("IMAGE", white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.015f), 0.0005f, 0.0008f);
								render_text(std::to_string(w) + "x" + std::to_string(h), white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.045f), 0.0005f, 0.0008f);
							}
							else if ((binel->type == ELEM_LAYER) and !this->binpreview) {
								// do first entry preview preperation/visualisation when layer file hovered
								if (remove_extension(basename(binel->path)) != "") {
									if (mainprogram->prelay) {
										// close old preview layer
										mainprogram->prelay->closethread = true;
										while (mainprogram->prelay->closethread) {
											mainprogram->prelay->ready = true;
											mainprogram->prelay->startdecode.notify_one();
										}
										//delete mainprogram->prelay;
									}
									this->binpreview = true;
									draw_box(red, black, 0.52f, 0.5f, 0.4f, 0.4f, -1);
									mainprogram->prelay = new Layer(false);
									mainprogram->prelay->dummy = true;
									mainprogram->prelay->pos = 0;
									mainprogram->prelay->node = mainprogram->nodesmain->currpage->add_videonode(2);
									mainprogram->prelay->node->layer = mainprogram->prelay;
									mainprogram->prelay->lasteffnode[0] = mainprogram->prelay->node;
									mainprogram->prelay->lasteffnode[1] = mainprogram->prelay->node;
									mainmix->open_layerfile(binel->path, mainprogram->prelay, true, 0);
									if (isimage(mainprogram->prelay->filename)) {
										// if layer file contains an image, relay preview to image specific code above
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
									// wait until opened
									std::unique_lock<std::mutex> olock2(mainprogram->prelay->endopenlock);
									mainprogram->prelay->endopenvar.wait(olock2, [&] {return mainprogram->prelay->opened; });
									mainprogram->prelay->opened = false;
									olock2.unlock();
									mainprogram->prelay->initialized = true;
									mainprogram->prelay->ready = true;
									while (mainprogram->prelay->ready) {
										// start decode frame
										mainprogram->prelay->startdecode.notify_one();
									}
									// wait for decode end
									std::unique_lock<std::mutex> lock2(mainprogram->prelay->enddecodelock);
									mainprogram->prelay->enddecodevar.wait(lock2, [&] {return mainprogram->prelay->processed; });
									mainprogram->prelay->processed = false;
									lock2.unlock();
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
									// calculate effects
									onestepfrom(0, mainprogram->prelay->node, nullptr, -1, -1);
									if (mainprogram->prelay->effects[0].size()) {
										draw_box(red, black, 0.52f, 0.5f, 0.4f, 0.4f, mainprogram->prelay->effects[0][mainprogram->prelay->effects[0].size() - 1]->fbotex);
									}
									else {
										if (mainprogram->prelay->drawfbo2) {
											draw_box(red, black, 0.52f, 0.5f, 0.4f, 0.4f, mainprogram->prelay->fbotex);
										}
										else {
											draw_box(red, black, 0.52f, 0.5f, 0.4f, 0.4f, mainprogram->prelay->fbotexintm);
										}
									}
									if (!binel->encoding) {
										// show video format
										if (mainprogram->prelay->vidformat == 188 or mainprogram->prelay->vidformat == 187) {
											render_text("HAP", white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.015f), 0.0005f, 0.0008f);
											render_text(std::to_string(mainprogram->prelay->decresult->width) + "x" + std::to_string(mainprogram->prelay->decresult->height), white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.045f), 0.0005f, 0.0008f);
										}
										else if (mainprogram->prelay->type != ELEM_IMAGE) {
											render_text("CPU", white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.015f), 0.0005f, 0.0008f);
											render_text(std::to_string(mainprogram->prelay->decresult->width) + "x" + std::to_string(mainprogram->prelay->decresult->height), white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.045f), 0.0005f, 0.0008f);
										}
									}
								}
							}
							else if (binel->type == ELEM_LAYER) {
								// do second entry preview visualisation when layer file hovered
								draw_box(red, black, 0.52f, 0.5f, 0.4f, 0.4f, -1);
								if (mainprogram->mousewheel) {
									// mousewheel leaps through video
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
										// start decode frame
										mainprogram->prelay->startdecode.notify_one();
									}
									// wait for decode finished
									std::unique_lock<std::mutex> lock(mainprogram->prelay->enddecodelock);
									mainprogram->prelay->enddecodevar.wait(lock, [&] {return mainprogram->prelay->processed; });
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
									// calculate effects
									onestepfrom(0, mainprogram->prelay->node, nullptr, -1, -1);
									if (mainprogram->prelay->effects[0].size()) {
										draw_box(red, black, 0.52f, 0.5f, 0.4f, 0.4f, mainprogram->prelay->effects[0][mainprogram->prelay->effects[0].size() - 1]->fbotex);
									}
									else {
										if (mainprogram->prelay->drawfbo2) {
											draw_box(red, black, 0.52f, 0.5f, 0.4f, 0.4f, mainprogram->prelay->fbotex);
										}
										else {
											draw_box(red, black, 0.52f, 0.5f, 0.4f, 0.4f, mainprogram->prelay->fbotexintm);
										}
									}
								}
								else {
									// show old image if not mousewheeled through file
									if (mainprogram->prelay->effects[0].size()) {
										draw_box(red, black, 0.52f, 0.5f, 0.4f, 0.4f, mainprogram->prelay->effects[0][mainprogram->prelay->effects[0].size() - 1]->fbotex);
									}
									else {
										if (mainprogram->prelay->drawfbo2) {
											draw_box(red, black, 0.52f, 0.5f, 0.4f, 0.4f, mainprogram->prelay->fbotex);
										}
										else {
											draw_box(red, black, 0.52f, 0.5f, 0.4f, 0.4f, mainprogram->prelay->fbotexintm);
										}
									}
								}
								if (!binel->encoding and remove_extension(basename(binel->path)) != "") {
									// show video format
									if (mainprogram->prelay->vidformat == 188 or mainprogram->prelay->vidformat == 187) {
										render_text("HAP", white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.015f), 0.0005f, 0.0008f);
										render_text(std::to_string(mainprogram->prelay->decresult->width) + "x" + std::to_string(mainprogram->prelay->decresult->height), white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.045f), 0.0005f, 0.0008f);
									}
									else if (mainprogram->prelay->type != ELEM_IMAGE) {
										render_text("CPU", white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.015f), 0.0005f, 0.0008f);
										render_text(std::to_string(mainprogram->prelay->decresult->width) + "x" + std::to_string(mainprogram->prelay->decresult->height), white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.045f), 0.0005f, 0.0008f);
									}
								}
							}
							else if ((binel->type == ELEM_FILE) and !this->binpreview) {
								// do first entry preview preparation/visualisation when video hovered
								if (remove_extension(basename(binel->path)) != "") {
									if (mainprogram->prelay) {
										// close old preview layer
										mainprogram->prelay->closethread = true;
										while (mainprogram->prelay->closethread) {
											mainprogram->prelay->ready = true;
											mainprogram->prelay->startdecode.notify_one();
										}
										//delete mainprogram->prelay;
									}
									this->binpreview = true;
									draw_box(red, black, 0.52f, 0.5f, 0.4f, 0.4f, -1);
									mainprogram->prelay = new Layer(true);
									mainprogram->prelay->dummy = true;
									mainprogram->prelay->open_video(0, binel->path, true);
									// wait for video open
									std::unique_lock<std::mutex> olock(mainprogram->prelay->endopenlock);
									mainprogram->prelay->endopenvar.wait(olock, [&] {return mainprogram->prelay->opened; });
									mainprogram->prelay->opened = false;
									olock.unlock();
									mainprogram->prelay->initialized = true;
									mainprogram->prelay->frame = 0.0f;
									mainprogram->prelay->prevframe = -1;
									mainprogram->prelay->ready = true;
									while (mainprogram->prelay->ready) {
										// start decode frame
										mainprogram->prelay->startdecode.notify_one();
									}
									// wait for decode finished
									std::unique_lock<std::mutex> lock2(mainprogram->prelay->enddecodelock);
									mainprogram->prelay->enddecodevar.wait(lock2, [&] {return mainprogram->prelay->processed; });
									mainprogram->prelay->processed = false;
									lock2.unlock();
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
									draw_box(red, black, 0.52f, 0.5f, 0.4f, 0.4f, this->binelpreviewtex);
									if (!binel->encoding) {
										// show video format
										if (mainprogram->prelay->vidformat == 188 or mainprogram->prelay->vidformat == 187) {
											render_text("HAP", white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.015f), 0.0005f, 0.0008f);
											render_text(std::to_string(mainprogram->prelay->video_dec_ctx->width) + "x" + std::to_string(mainprogram->prelay->video_dec_ctx->height), white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.045f), 0.0005f, 0.0008f);
										}
										else {
											if (mainprogram->prelay->type == ELEM_FILE) {
												render_text("CPU", white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.015f), 0.0005f, 0.0008f);
												render_text(std::to_string(mainprogram->prelay->video_dec_ctx->width) + "x" + std::to_string(mainprogram->prelay->video_dec_ctx->height), white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.045f), 0.0005f, 0.0008f);
											}
										}
									}
								}
							}
							else if (binel->type == ELEM_FILE) {
								// do second entry preview visualisation when video hovered
								draw_box(red, black, 0.52f, 0.5f, 0.4f, 0.4f, -1);
								if (mainprogram->prelay->type != ELEM_IMAGE) {
									if (mainprogram->mousewheel) {
										// mousewheel leaps through video
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
											// start decode frame
											mainprogram->prelay->startdecode.notify_one();
										}
										// wait for decode finished
										std::unique_lock<std::mutex> lock(mainprogram->prelay->enddecodelock);
										mainprogram->prelay->enddecodevar.wait(lock, [&] {return mainprogram->prelay->processed; });
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
										draw_box(red, black, 0.52f, 0.5f, 0.4f, 0.4f, this->binelpreviewtex);
									}
									else {
										draw_box(red, black, 0.52f, 0.5f, 0.4f, 0.4f, this->binelpreviewtex);
									}
								}
								else {
									draw_box(red, black, 0.52f, 0.5f, 0.4f, 0.4f, mainprogram->prelay->texture);
								}
								if (!binel->encoding and remove_extension(basename(binel->path)) != "") {
									// show video format
									if (mainprogram->prelay->vidformat == 188 or mainprogram->prelay->vidformat == 187) {
										render_text("HAP", white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.015f), 0.0005f, 0.0008f);
										render_text(std::to_string(mainprogram->prelay->video_dec_ctx->width) + "x" + std::to_string(mainprogram->prelay->video_dec_ctx->height), white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.045f), 0.0005f, 0.0008f);
									}
									else {
										if (mainprogram->prelay->type == ELEM_FILE) {
											render_text("CPU", white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.015f), 0.0005f, 0.0008f);
											render_text(std::to_string(mainprogram->prelay->video_dec_ctx->width) + "x" + std::to_string(mainprogram->prelay->video_dec_ctx->height), white, box->vtxcoords->x1 + tf(0.005f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.045f), 0.0005f, 0.0008f);
										}
									}
								}
							}
						}

						if (binel->path != "") {
							if (!this->inputtexes.size() and mainprogram->leftmousedown and !mainprogram->dragbinel) {
								// dragging single bin element
								mainprogram->dragbinel = new BinElement;
								mainprogram->dragbinel->tex = binel->tex;
								mainprogram->dragbinel->path = binel->path;
								mainprogram->dragbinel->type = binel->type;
								mainprogram->leftmousedown = false;
								if (this->movingtex != -1) {
									// update drag
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
									// start drag
									this->movingtex = binel->tex;
									this->movingbinel = binel;
									this->currbinel = binel;
									mainprogram->leftmouse = false;
								}
							}
						}
					}

					if (binel != this->currbinel) {
						if (this->currbinel) this->binpreview = false;
						if (lay->vidmoving) {
							// when dragging layer in from mix view
							if (this->currbinel) {
								//reset old currbinel
								this->currbinel->tex = this->currbinel->oldtex;
								// set new layer drag textures in this bin element
								binel->oldtex = binel->tex;
								binel->tex = this->dragtex;
							}
							else {
								// set new layer drag textures in this bin element
								binel->oldtex = binel->tex;
								binel->tex = this->dragtex;
							}
							binel->full = true;
							this->currbinel = binel;
						}	

						if (this->inputtexes.size()) {
							bool cont = false;
							if (this->prevbinel) {
								// when inputting files, set old texes back
								int offset = 0;
								for (int k = 0; k < this->inputtexes.size(); k++) {
									// when opening elements, calculate offset when first element past bin start or end
									int offj = (int)((this->previ + offset + k) / 12);
									int el = ((this->previ + offset + k + 144) % 12) * 12 + this->prevj + offj;
									if (this->prevj + offj > 11) {
										offset = k - this->inputtexes.size();
										cont = true;
										break;
									}
									BinElement *dirbinel = this->currbin->elements[el];
									if (cont) break;
								}
								offset = 0;
								if (!cont) {
									for (int k = 0; k < this->inputtexes.size(); k++) {
										int offj = (int)((this->previ + offset + k) / 12);
										int el = ((this->previ + offset + k + 144) % 12) * 12 + this->prevj + offj;
										BinElement *dirbinel = this->currbin->elements[el];
										// reset old texes
										dirbinel->tex = dirbinel->oldtex;
									}
								}
								else {
									for (int k = 0; k < this->inputtexes.size(); k++) {
										int offj = (int)((143 - k + offset) / 12) - (143 - k + offset < 0);
										int el = ((143 - k + offset) % 12) * 12 + offj;
										BinElement *dirbinel = this->currbin->elements[el];
										// reset old texes
										dirbinel->tex = dirbinel->oldtex;
									}
								}
							}
							// set new texes of inputted files
							int offset = 0;
							cont = false;
							for (int k = 0; k < this->inputtexes.size(); k++) {
								int offj = (int)((i + offset + k) / 12) - (i + offset + k < 0);
								int el = ((i + offset + k + 144) % 12) * 12 + j + offj;
								if (j + offj > 11) {
									offset = k - this->inputtexes.size();
									cont = true;
									break;
								}
								BinElement *dirbinel = this->currbin->elements[el];
								if (cont) break;
							}
							offset = 0;
							if (!cont) {
								for (int k = 0; k < this->inputtexes.size(); k++) {
									int offj = (int)((i + offset + k) / 12) - (i + offset + k < 0);
									int el = ((i + offset + k + 144) % 12) * 12 + j + offj;
									BinElement *dirbinel = this->currbin->elements[el];
									// set input texes
									dirbinel->oldtex = dirbinel->tex;
									dirbinel->tex = this->inputtexes[k];
								}
							}
							else {
								for (int k = 0; k < this->inputtexes.size(); k++) {
									int offj = (int)((143 - k + offset) / 12) - (143 - k + offset < 0);
									int el = ((143 - k + offset) % 12) * 12 + offj;
									BinElement *dirbinel = this->currbin->elements[el];
									// set input texes
									dirbinel->oldtex = dirbinel->tex;
									dirbinel->tex = this->inputtexes[this->inputtexes.size() - k - 1];
								}
							}
							// set values to allow for resetting old texes when inputfiles are moved around
							this->prevbinel = binel;
							this->previ = i;
							this->prevj = j;
						}
					}

					BinElement *tempbinel = this->currbinel;
					if (this->currbinel and this->movingtex != -1) {
						if (binel != this->currbinel) {
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
						else this->currbinel = binel;
					}
					else this->currbinel = binel;
				}
				

				if (mainprogram->del) {
					// handle element deleting
					for (int k = 0; k < this->delbinels.size(); k++) {
						// deleting single bin element
						this->delbinels[k]->tex = this->delbinels[k]->oldtex;
						std::string name = remove_extension(basename(this->delbinels[k]->path));
						//if (this->delbinels[k]->type == ELEM_LAYER or this->delbinels[k]->type == ELEM_LAYERthis->movingbinel->type == ELEM_LAYER) boost::filesystem::remove(this->delbinels[k]->path);  reminder
						this->delbinels[k]->path = this->delbinels[k]->oldpath;
						this->delbinels[k]->jpegpath = this->delbinels[k]->oldjpegpath;
						this->delbinels[k]->type = this->delbinels[k]->oldtype;
						this->delbinels[k]->full = false;
						this->delbinels[k]->select = false;
						boost::filesystem::remove(this->delbinels[k]->jpegpath);
					}
					std::string path = mainprogram->project->binsdir + this->currbin->name + ".bin";
					save_bin(path);
					this->delbinels.clear();
					mainprogram->del = false;
				}
				
				if (this->currbinel and this->inputtexes.size() and mainprogram->leftmouse) {
					// confirm position of inputted files and set all influenced bin elements to the right values
					for (int k = 0; k < this->inputtexes.size(); k++) {
						int intm = (144 - (this->previ + this->prevj * 12) - this->inputtexes.size());
						intm = (intm < 0) * intm;
						int jj = this->prevj + (int)((k + this->previ + intm) / 12) - ((k + this->previ + intm) < 0);
						int ii = ((k + intm + 144) % 12 + this->previ + 144) % 12;
						BinElement *dirbinel = this->currbin->elements[ii * 12 + jj];
						dirbinel->full = true;
						dirbinel->type = this->inputtypes[k];
						dirbinel->path = this->newpaths[k];
						dirbinel->oldjpegpath = dirbinel->jpegpath;
						dirbinel->jpegpath = this->inputjpegpaths[k];
						//glDeleteTextures(1, &dirbinel->oldtex);
					}
					std::string path = mainprogram->project->binsdir + this->currbin->name + ".bin";
					save_bin(path);
					// clean up
					this->inputtexes.clear();
					this->currbinel = nullptr;
					this->prevbinel = nullptr;
					this->inputtypes.clear();
					this->newpaths.clear();
					mainprogram->leftmouse = false;
				}
			}
		}

		if (!inbinel) this->binpreview = false;

		if (inbinel and !mainprogram->rightmouse and lay->vidmoving and mainprogram->lmsave) {
			// confirm layer dragging from main view and set influenced bin element to the right values
			this->currbinel->full = true;
			this->currbinel->type = mainprogram->dragbinel->type;
			this->currbinel->path = mainprogram->dragbinel->path;
			if (this->currbinel->type == ELEM_LAYER) {
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
			}
			std::string path = mainprogram->project->binsdir + this->currbin->name + ".bin";
			save_bin(path);
			this->currbinel = nullptr;
			enddrag();
			lay->vidmoving = false;
			mainmix->moving = false;
		}
		else if ((mainprogram->lmsave or mainprogram->rightmouse) and mainprogram->dragbinel) {
			//when dropping on grey area
			if (lay->vidmoving) {
				// whn layer dragging from mix view
				if (this->currbinel) {
					this->currbinel->tex = this->currbinel->oldtex;
					this->currbinel = nullptr;
				}
			}
			else if (this->movingtex != -1) {
				// when element dragging inside bin
				bool found = false;
				BinElement* foundbinel;
				for (int j = 0; j < 12; j++) {
					for (int i = 0; i < 12; i++) {
						Box* box = this->elemboxes[i * 12 + j];
						foundbinel = this->currbin->elements[i * 12 + j];
						if (box->in() and this->currbinel == foundbinel) {
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

	// load one file into bin each loop, at end to allow drawing ordering dialog on top
	if (this->openbinfile) {
		open_binfiles();
	}
}

void BinsMain::open_bin(const std::string &path, Bin *bin) {
	// open a bin file
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
			// open bin elements
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
	}

	rfile.close();
}

void BinsMain::save_bin(const std::string& path) {
	std::thread binsav(&BinsMain::do_save_bin, this, path);
	binsav.detach();
}

void BinsMain::do_save_bin(const std::string& path) {
	// save bin file
	SDL_GL_MakeCurrent(mainprogram->dummywindow, glc_th);
	std::vector<std::string> filestoadd;
	std::ofstream wfile;
	wfile.open(path.c_str());
	wfile << "EWOCvj BINFILE V0.2\n";
	
	wfile << "ELEMS\n";
	// save elements
	for (int i = 0; i < 12; i++) {
		for (int j = 0; j < 12; j++) {
			wfile << this->currbin->elements[i * 12 + j]->path;
			wfile << "\n";
			wfile << std::to_string(this->currbin->elements[i * 12 + j]->type);
			wfile << "\n";
			ELEM_TYPE type = this->currbin->elements[i * 12 + j]->type;
			if ((type == ELEM_LAYER or type == ELEM_DECK or type == ELEM_MIX) and this->currbin->elements[i * 12 + j]->path != "") {
				filestoadd.push_back(this->currbin->elements[i * 12 + j]->path);
			}
			if (this->currbin->elements[i * 12 + j]->path != "") {
				if (this->currbin->elements[i * 12 + j]->jpegpath == "") {
					std::string name = remove_extension(basename(this->currbin->elements[i * 12 + j]->path));
					int count = 0;
					while (1) {
						this->currbin->elements[i * 12 + j]->jpegpath = mainprogram->temppath + this->currbin->name + "_" + name + ".jpg";
						if (!exists(this->currbin->elements[i * 12 + j]->jpegpath)) {
							break;
						}
						count++;
						name = remove_version(name) + "_" + std::to_string(count);
					}
					save_thumb(this->currbin->elements[i * 12 + j]->jpegpath, this->currbin->elements[i * 12 + j]->tex);
				}
				filestoadd.push_back(this->currbin->elements[i * 12 + j]->jpegpath);
			}			
			wfile << this->currbin->elements[i * 12 + j]->jpegpath;
			wfile << "\n";
		}
	}
	wfile << "ENDOFELEMS\n";
	
	SDL_GL_MakeCurrent(nullptr, nullptr);
		
	wfile << "ENDOFFILE\n";
	wfile.close();
	
	std::string tcbpath;
	std::string name = "tempconcatbin";
	int count = 0;
	while (1) {
		tcbpath = mainprogram->temppath + name;
		if (!exists(tcbpath)) {
			break;
		}
		count++;
		name = remove_version(name) + "_" + std::to_string(count);
	}
	std::ofstream outputfile;
	outputfile.open(tcbpath, std::ios::out | std::ios::binary);
	std::vector<std::vector<std::string>> filestoadd2;
	filestoadd2.push_back(filestoadd);
	concat_files(outputfile, path, filestoadd2);
	outputfile.close();
	boost::filesystem::rename(tcbpath, path);
}

Bin *BinsMain::new_bin(const std::string &name) {
	Bin *bin = new Bin(this->bins.size());
	bin->name = name;
	this->bins.push_back(bin);
	//this->currbin->elements.clear();
	
	for (int i = 0; i < 12; i++) {
		for (int j = 0; j < 12; j++) {
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
	// make bin on position pos current
	this->currbin->pos = pos;
	this->currbin->name = this->bins[pos]->name;
	this->currbin->elements = this->bins[pos]->elements;
	this->prevbinel = nullptr;
}

int BinsMain::read_binslist() {
	// load the list of bins in this project
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
		if (istring == "BINSSCROLL") {
			getline(rfile, istring);
			this->binsscroll = std::stoi(istring);
		}
	}
	rfile.close();
	return currbin;
}

void BinsMain::save_binslist() {
	// save the list of bins in this project
	std::ofstream wfile;
	wfile.open(mainprogram->project->binsdir + "bins.list");
	wfile << "EWOC BINSLIST v0.2\n";
	wfile << std::to_string(this->currbin->pos);
	wfile << "\n";
	wfile << "BINS\n";
	for (int i = 0; i < this->bins.size(); i++) {
		std::string path = mainprogram->project->binsdir + this->bins[i]->name + ".bin";
		wfile << this->bins[i]->name;
		wfile << "\n";
	}
	wfile << "ENDOFBINS\n";
	wfile << "BINSSCROLL\n";
	wfile << std::to_string(this->binsscroll);
	wfile << "\n";
	wfile << "ENDOFFILE\n";
	wfile.close();
}

void BinsMain::open_binfiles() {
	// open videos/images/layer files into bin

	// order elements
	bool cont = mainprogram->order_paths(false);
	if (!cont) return;

	mainprogram->blocking = true;
	if (SDL_GetMouseFocus() != mainprogram->mainwindow) {
		SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW));
	}
	else {
		SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT));
	} 
	if (mainprogram->counting == mainprogram->paths.size()) {
		std::string path = mainprogram->project->binsdir + this->currbin->name + ".bin";
		save_bin(path);
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

void BinsMain::open_handlefile(const std::string &path) {
	// prepare value lists for inputting videos/images/layer files from disk
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
	else if (path.substr(path.length() - 5, std::string::npos) == ".deck") {
		// prepare layer file for bin entry
		endtype = ELEM_DECK;
		endtex = get_deckmixtex(path);
	}
	else if (path.substr(path.length() - 4, std::string::npos) == ".mix") {
		// prepare layer file for bin entry
		endtype = ELEM_MIX;
		endtex = get_deckmixtex(path);
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
	std::string jpath;
	std::string name = remove_extension(basename(path));
	int count = 0;
	while (1) {
		jpath = mainprogram->temppath + name + ".jpg";
		if (!exists(jpath)) {
			break;
		}
		count++;
		name = remove_version(name) + "_" + std::to_string(count);
	}
	save_thumb(jpath, copy_tex(endtex));
	this->inputjpegpaths.push_back(jpath);
}


std::tuple<std::string, std::string> BinsMain::hap_binel(BinElement *binel, BinElement *bdm) {
	// encode single bin element, possibly contained in a deck or a mix
	this->binpreview = false;
   	std::string apath = "";
	std::string rpath = "";
	if (binel->type == ELEM_FILE) {
		std::thread hap (&BinsMain::hap_encode, this, binel->path, binel, bdm);
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
		apath = binel->path;
		rpath = mainprogram->docpath + boost::filesystem::relative(apath, mainprogram->docpath).string();
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
				if (bdm) {
					bdm->encthreads--;
				}
				mainprogram->encthreads--;
				return {apath, rpath};
 			}
			std::thread hap (&BinsMain::hap_encode, this, path, binel, bdm);
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
	if (!bdm) {
		std::string bpath = mainprogram->project->binsdir + this->currbin->name + ".bin";
		this->do_save_bin(bpath);
	}

	return {apath, rpath};
}				
	
void BinsMain::hap_deck(BinElement* bd) {
	bd->allhaps = 0;
	bd->encodeprogress = 0.0f;
	bd->encoding = true;
	// hap encode a bindeck
	std::ifstream rfile;
	rfile.open(bd->path);
	std::ofstream wfile;
	wfile.open(remove_extension(bd->path) + ".temp");
	std::string istring;
	bd->encthreads = 0;
	std::string apath = "";
	std::string rpath = "";

	while (getline(rfile, istring)) {
		if (istring == "FILENAME") {
			getline(rfile, istring);
			BinElement *binel = new BinElement;
			binel->path = istring;
			binel->type = ELEM_FILE;
			bd->encthreads++;
			std::tuple<std::string, std::string> output = this->hap_binel(binel, bd);
			apath = std::get<0>(output);
			rpath = std::get<1>(output);
			wfile << "FILENAME\n";
			wfile << apath;
			wfile << "\n";
			wfile << "RELPATH\n";
			wfile << rpath;
			wfile << "\n";
		}
		else if (istring == "RELPATH") {
			getline(rfile, istring);
		}
		else {
			wfile << istring;
			wfile << "\n";
		}
	}
	rfile.close();
	wfile.close();
	bd->allhaps = bd->encthreads;
}

void BinsMain::hap_mix(BinElement * bm) {
	bm->allhaps = 0;
	bm->encodeprogress = 0.0f;
	bm->encoding = true;
	// hap encode a binmix
	std::ifstream rfile;
	rfile.open(bm->path);
	std::ofstream wfile;
	wfile.open(remove_extension(bm->path) + ".temp");
	std::string istring;
	bm->encthreads = 0;
	std::string apath;
	std::string rpath;
	if (bm->path != "") {
		while (getline(rfile, istring)) {
			if (istring == "FILENAME") {
				getline(rfile, istring);
				BinElement *binel = new BinElement;
				binel->path = istring;
				binel->type = ELEM_FILE;
				bm->encthreads++;
				std::tuple<std::string, std::string> output = this->hap_binel(binel, bm);
				apath = std::get<0>(output);
				rpath = std::get<1>(output);
				wfile << "FILENAME\n";
				wfile << apath;
				wfile << "\n";
				wfile << "RELPATH\n";
				wfile << rpath;
				wfile << "\n";
			}
			else if (istring == "RELPATH") {
				getline(rfile, istring);
			}
			else {
				wfile << istring;
				wfile << "\n";
			}
		}
	}
	rfile.close();
	wfile.close();
	bm->allhaps = bm->encthreads;
}

void BinsMain::hap_encode(const std::string srcpath, BinElement *binel, BinElement *bdm) {
	// do the actual hap encoding
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
		if (bdm) {
			bdm->encthreads--;
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
	float oldprogress = 0.0f;

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
		if (bdm) bdm->encodeprogress += binel->encodeprogress - oldprogress;
		oldprogress = binel->encodeprogress;
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
    if (bdm) {
    	bdm->encthreads--;
		delete binel;  // temp bin elements populate bdm binelements
    }
 	mainprogram->encthreads--;
	mainprogram->hapnow = true;
	mainprogram->hap.notify_all();
}

