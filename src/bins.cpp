#if defined(WIN32) && !defined(__linux__)
#define WINDOWS
#elif defined(__linux__) && !defined(WIN32)
#define POSIX
#endif


#include <boost/filesystem.hpp>


#include "GL/glew.h"
#include "GL/gl.h"
#include "GL/glut.h"

#ifdef POSIX
#include <X11/Xos.h>
#endif

#include "SDL2/SDL.h"
#include "SDL2/SDL_syswm.h"

#include <ostream>
#include <ios>
#include <arpa/inet.h>

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
	// boxes of bins in binslist
	this->box = new Box;
	this->box->vtxcoords->x1 = 0.50f;
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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
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

void BinElement::erase() {
	this->select = false;
	this->path = "";
	this->oldpath = "";
	this->name = "";
	this->oldname = "";
	this->jpegpath = "";
	this->oldjpegpath = "";
	this->type = ELEM_FILE;
	this->oldtype = ELEM_FILE;
	GLuint tex = this->tex;
	this->tex = copy_tex(tex);
	blacken(this->tex);
	blacken(this->oldtex);
}

BinsMain::BinsMain() {
	for (int i = 0; i < 12; i++) {
		for (int j = 0; j < 12; j++) {
			// initialize all 144 bin elements
			Box *box = new Box;
			this->elemboxes.push_back(box);
			box->vtxcoords->x1 = -0.95f + j * 0.12f;
			box->vtxcoords->y1 = 0.95f - ((i % 12) + 1) * 0.15f;
			box->vtxcoords->w = 0.1f;
			box->vtxcoords->h = 0.1f;
			box->upvtxtoscr();
			box->lcolor[0] = 0.4f;
			box->lcolor[1] = 0.4f;
			box->lcolor[2] = 0.4f;
			box->lcolor[3] = 1.0f;
			box->acolor[3] = 1.0f;
			box->tooltiptitle = "Media bin element ";
			box->tooltip = "Shows thumbnail of media bin element, either being a video file (grey border) or an image (pink border) or a layer file (orange border) or a deck file (purple border) or mix file (green border).  Hovering over this element shows video resolution and video compression method (CPU or HAP).  Mousewheel skips through the element contents (previewed in larger monitor topmiddle).  Leftdrag allows dragging to mix screen via wormgate.  Leftclick allows moving inside the media bin. Rightclickmenu allows among other things, HAP encoding and also loading of a yellowbordered grid-block of mediabin elements into one of the shelves. ";
		}
	}

	// box to click to load a bin into the bins list
	this->loadbinbox = new Box;
	this->loadbinbox->vtxcoords->x1 = 0.50f;
	this->loadbinbox->vtxcoords->y1 = -0.95f;
	this->loadbinbox->vtxcoords->w = 0.3f;
	this->loadbinbox->vtxcoords->h = 0.05f;
	this->loadbinbox->upvtxtoscr();
	this->loadbinbox->tooltiptitle = "Load bin ";
	this->loadbinbox->tooltip = "Leftclick to browse for a bin to be loaded. ";
	// box to click to add another bin to bins list
	this->newbinbox = new Box;
	this->newbinbox->vtxcoords->x1 = 0.50f;
	this->newbinbox->vtxcoords->y1 = -1.0f;
	this->newbinbox->vtxcoords->w = 0.3f;
	this->newbinbox->vtxcoords->h = 0.05f;
	this->newbinbox->upvtxtoscr();
	this->newbinbox->tooltiptitle = "Add new bin ";
	this->newbinbox->tooltip = "Leftclick to add a new bin and make it current. ";

	this->renamingbox = new Box;
	this->renamingbox->vtxcoords->x1 = -0.5f;
	this->renamingbox->vtxcoords->y1 = -0.2f;
	this->renamingbox->vtxcoords->w = 1.0f;
	this->renamingbox->vtxcoords->h = 0.2f;
	this->renamingbox->upvtxtoscr();
	this->renamingbox->tooltiptitle = "Rename bin element ";
	this->renamingbox->tooltip = "Use keyboard to edit media element display name. ";

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
	this->binsscrollup->vtxcoords->x1 = 0.475f;
	this->binsscrollup->vtxcoords->y1 = -0.05f;
	this->binsscrollup->vtxcoords->w = 0.025f;
	this->binsscrollup->vtxcoords->h = 0.05f;
	this->binsscrollup->upvtxtoscr();
	this->binsscrollup->tooltiptitle = "Scroll bins list up ";
	this->binsscrollup->tooltip = "Leftclicking scrolls the bins list up ";

	// arrow box to scroll bins list down
    this->binsscrolldown = new Box;
    this->binsscrolldown->vtxcoords->x1 = 0.475f;
    this->binsscrolldown->vtxcoords->y1 = -1.0f;
    this->binsscrolldown->vtxcoords->w = 0.025f;
    this->binsscrolldown->vtxcoords->h = 0.05f;
    this->binsscrolldown->upvtxtoscr();
    this->binsscrolldown->tooltiptitle = "Scroll bins list down ";
    this->binsscrolldown->tooltip = "Leftclicking scrolls the bins list down ";

    this->floatbox = new Box;
    this->floatbox->vtxcoords->x1 = 0.8f;
    this->floatbox->vtxcoords->y1 = 0.95f;
    this->floatbox->vtxcoords->w = 0.15f;
    this->floatbox->vtxcoords->h = 0.05f;
    this->floatbox->upvtxtoscr();
    this->floatbox->tooltiptitle = "Float/dock bins screen ";
    this->floatbox->tooltip = "Leftclick toggles between a docked bins screen (swapped with mix screen) or a floating bins screen (shown on a separate screen). ";

}

void BinsMain::handle(bool draw) {
	GLint inverted = glGetUniformLocation(mainprogram->ShaderProgram, "inverted");

    int numd = SDL_GetNumVideoDisplays();
    if (numd > 1) {
        draw_box(this->floatbox, -1);
        if (this->floating) render_text("DOCK", white, this->floatbox->vtxcoords->x1 + 0.02f, this->floatbox->vtxcoords->y1 + 0.01f, 0.00045f, 0.00075f);
        else render_text("FLOAT", white, this->floatbox->vtxcoords->x1 + 0.02f, this->floatbox->vtxcoords->y1 + 0.01f, 0.00045f, 0.00075f);
        if (this->floatbox->in()) {
            if (mainprogram->leftmouse || mainprogram->rightmouse) {
                if (!this->floating) {
                    std::vector<std::string> bintargets;
                    for (int i = 1; i < numd; i++) {
                        bintargets.push_back(SDL_GetDisplayName(i));
                    }
                    mainprogram->make_menu("bintargetmenu", mainprogram->bintargetmenu, bintargets);
                    mainprogram->bintargetmenu->state = 2;
                    mainprogram->bintargetmenu->menux = mainprogram->mx;
                    mainprogram->bintargetmenu->menuy = mainprogram->my;
                }
                else this->floating = false;
            }
        }
    }

    mainprogram->handle_bintargetmenu();

	if (this->renamingelem) {
		if (mainprogram->renaming == EDIT_NONE) {
			this->renamingelem = nullptr;
		}
		if (mainprogram->leftmousedown && !this->renamingbox->in()) {
			mainprogram->renaming = EDIT_NONE;
			this->renamingelem = nullptr;
			SDL_StopTextInput();
			mainprogram->leftmousedown = false;
		}
		if (mainprogram->rightmouse) {
			mainprogram->renaming = EDIT_NONE;
			SDL_StopTextInput();
			this->renamingelem->name = this->renamingelem->oldname;
			this->renamingelem = nullptr;
			mainprogram->rightmouse = false;
			mainprogram->menuactivation = false;
		}
	}
	if (mainprogram->renaming == EDIT_BINNAME) {
		if (mainprogram->rightmouse) {
			mainprogram->renaming = EDIT_NONE;
			SDL_StopTextInput();
			this->menubin->name = mainprogram->backupname;
			this->menubin = nullptr;
			mainprogram->rightmouse = false;
			mainprogram->menuactivation = false;
		}
	}

	//draw binelements
	if (!mainprogram->menuondisplay) this->menubinel = nullptr;
	if (mainprogram->menuactivation) this->menubinel = nullptr;
	if (draw) {
		for (int j = 0; j < 12; j++) {
			for (int i = 0; i < 12; i++) {
				Box* box = this->elemboxes[i * 12 + j];
				BinElement* binel = this->currbin->elements[i * 12 + j];
				if (box->in()) {
					this->menubinel = binel;
					if (mainprogram->menuactivation) {
						this->menuactbinel = binel;
						if (binel->name != "") {
							// Set menu when over non-empty element
							this->binelmenuoptions.clear();
							std::vector<std::string> bnlm;
							if (!binel->encoding) {
								bnlm.push_back("Delete element");
								binelmenuoptions.push_back(BET_DELETE);
								bnlm.push_back("Rename element");
								binelmenuoptions.push_back(BET_RENAME);
								bnlm.push_back("Open file(s) from disk");
								binelmenuoptions.push_back(BET_OPENFILES);
								bnlm.push_back("Insert deck A");
								binelmenuoptions.push_back(BET_INSDECKA);
								bnlm.push_back("Insert deck B");
								binelmenuoptions.push_back(BET_INSDECKB);
								bnlm.push_back("Insert full mix");
								binelmenuoptions.push_back(BET_INSMIX);
								bnlm.push_back("Load block in shelf A");
								binelmenuoptions.push_back(BET_LOADSHELFA);
								bnlm.push_back("Load block in shelf B");
								binelmenuoptions.push_back(BET_LOADSHELFB);
								bnlm.push_back("HAP encode element");
								binelmenuoptions.push_back(BET_HAPELEM);
								bnlm.push_back("Quit");
								binelmenuoptions.push_back(BET_QUIT);
							}
							else {
								bnlm.push_back("Cancel element HAP encoding");
								binelmenuoptions.push_back(BET_HAPELEM);
							}
							mainprogram->make_menu("binelmenu", mainprogram->binelmenu, bnlm);
						}
						if (binel->type == ELEM_DECK) {
							// set binelmenu entries when mouse over deck
							this->binelmenuoptions.clear();
							std::vector<std::string> bnlm;
							if (!binel->encoding) {
								bnlm.push_back("Load in deck A");
								binelmenuoptions.push_back(BET_LOADDECKA);
								bnlm.push_back("Load in deck B");
								binelmenuoptions.push_back(BET_LOADDECKB);
								bnlm.push_back("Delete deck");
								binelmenuoptions.push_back(BET_DELETE);
								bnlm.push_back("Rename deck");
								binelmenuoptions.push_back(BET_RENAME);
								bnlm.push_back("Open file(s) from disk");
								binelmenuoptions.push_back(BET_OPENFILES);
								bnlm.push_back("Insert deck A");
								binelmenuoptions.push_back(BET_INSDECKA);
								bnlm.push_back("Insert deck B");
								binelmenuoptions.push_back(BET_INSDECKB);
								bnlm.push_back("Insert full mix");
								binelmenuoptions.push_back(BET_INSMIX);
								bnlm.push_back("Load block in shelf A");
								binelmenuoptions.push_back(BET_LOADSHELFA);
								bnlm.push_back("Load block in shelf B");
								binelmenuoptions.push_back(BET_LOADSHELFB);
								bnlm.push_back("HAP encode deck");
								binelmenuoptions.push_back(BET_HAPELEM);
								bnlm.push_back("Quit");
								binelmenuoptions.push_back(BET_QUIT);
							}
							else {
								bnlm.push_back("Cancel deck HAP encoding");
								binelmenuoptions.push_back(BET_HAPELEM);
							}
							mainprogram->make_menu("binelmenu", mainprogram->binelmenu, bnlm);
						}
						else if (binel->type == ELEM_MIX) {
							// set binelmenu entries when mouse over mix
							this->binelmenuoptions.clear();
							std::vector<std::string> bnlm;
							if (!binel->encoding) {
								bnlm.push_back("Load in mix");
								binelmenuoptions.push_back(BET_LOADMIX);
								bnlm.push_back("Delete mix");
								binelmenuoptions.push_back(BET_DELETE);
								bnlm.push_back("Rename mix");
								binelmenuoptions.push_back(BET_RENAME);
								bnlm.push_back("Open file(s) from disk");
								binelmenuoptions.push_back(BET_OPENFILES);
								bnlm.push_back("Insert deck A");
								binelmenuoptions.push_back(BET_INSDECKA);
								bnlm.push_back("Insert deck B");
								binelmenuoptions.push_back(BET_INSDECKB);
								bnlm.push_back("Insert full mix");
								binelmenuoptions.push_back(BET_INSMIX);
								bnlm.push_back("Load block in shelf A");
								binelmenuoptions.push_back(BET_LOADSHELFA);
								bnlm.push_back("Load block in shelf B");
								binelmenuoptions.push_back(BET_LOADSHELFB);
								bnlm.push_back("HAP encode mix");
								binelmenuoptions.push_back(BET_HAPELEM);
								bnlm.push_back("Quit");
								binelmenuoptions.push_back(BET_QUIT);
							}
							else {
								bnlm.push_back("Cancel mix HAP encoding");
								binelmenuoptions.push_back(BET_HAPELEM);
							}
							mainprogram->make_menu("binelmenu", mainprogram->binelmenu, bnlm);
						}
						if (binel->select) {
                            // set binelmenu entries when mouse over selected element
                            this->binelmenuoptions.clear();
                            std::vector<std::string> bnlm;
                            if (!binel->encoding) {
                                bnlm.push_back("Move selected elements");
                                binelmenuoptions.push_back(BET_MOVSEL);
                                bnlm.push_back("Delete selected elements");
                                binelmenuoptions.push_back(BET_DELSEL);
                                bnlm.push_back("HAP encode selected elements");
                                binelmenuoptions.push_back(BET_HAPSEL);
                                bnlm.push_back("Quit");
                                binelmenuoptions.push_back(BET_QUIT);
                            }
                            else {
                                bnlm.push_back("Cancel element HAP encoding");
                                binelmenuoptions.push_back(BET_HAPELEM);
                            }
                            mainprogram->make_menu("binelmenu", mainprogram->binelmenu, bnlm);
						}
					}
				}

				// color code elements
				float color[4];
				if (binel->select) {
					color[0] = 1.0f;
					color[1] = 1.0f;
					color[2] = 1.0f;
					color[3] = 1.0f;
				}
				else if (binel->type == ELEM_LAYER) {
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
				if (binel->name != "") {
					if (binel->name != "") render_text(binel->name.substr(0, 20), white, box->vtxcoords->x1, box->vtxcoords->y1 - 0.02f, 0.00045f, 0.00075f);
				}
			}
			// draw big grey areas next to each element column to cut off element titles
			Box* box = this->elemboxes[j];
			draw_box(nullptr, darkgrey, box->vtxcoords->x1 + box->vtxcoords->w, -1.0f, 0.12f, 2.0f, -1);
		}

		bool cond1 = false;
		if (this->menubinel) cond1 = (this->menubinel->name == "");
		if (!mainprogram->menuondisplay) this->mouseshelfnum = -1;
        Box *insertbox = nullptr;
		mainprogram->frontbatch = true;
		for (int j = 0; j < 3; j++) {
			for (int i = 0; i < 3; i++) {
			    Box dumbox;
			    Box *box = &dumbox;
 				box->vtxcoords->x1 = -0.965f + j * 0.48f;
				box->vtxcoords->y1 = 0.925f - ((i % 3) + 1) * 0.6f;
				box->vtxcoords->w = 0.48f;
				box->vtxcoords->h = 0.6f;
				box->upvtxtoscr();
				draw_box(yellow, nullptr, box, -1);
				if (box->in()) {
					this->mouseshelfnum = i * 3 + j;
					if (this->insertshelf) {
						insertbox = box;
						mainprogram->leftmousedown = false;
						// inserting a shelf into one of the bin shelf blocks
						if (this->oldmouseshelfnum != -1) {
							if (this->oldmouseshelfnum != this->mouseshelfnum) {
								for (int i = 0; i < 16; i++) {
									// reset elements of previously hovered shelf block
									BinElement* binel = this->currbin->elements[this->oldmouseshelfnum / 3 * 48 + (this->oldmouseshelfnum % 3) * 4 + i / 4 + (i % 4) * 12];
									binel->tex = binel->oldtex;
									binel->type = binel->oldtype;
									binel->name = binel->oldname;
								}
							}
						}
						if (this->oldmouseshelfnum != this->mouseshelfnum) {
							// temporary change when hovering
							for (int i = 0; i < 16; i++) {
								BinElement* binel = this->currbin->elements[this->mouseshelfnum / 3 * 48 + (this->mouseshelfnum % 3) * 4 + i / 4 + (i % 4) * 12];
								ShelfElement* elem = this->insertshelf->elements[i];
								binel->oldtex = binel->tex;
								binel->oldtype = binel->type;
								binel->oldname = binel->name;
								binel->tex = elem->tex;
								binel->type = elem->type;
								binel->name = remove_extension(basename(elem->path));
							}
						}
						if (mainprogram->leftmouse) {
							// confirm insert
							for (int i = 0; i < 16; i++) {
								BinElement* binel = this->currbin->elements[this->mouseshelfnum / 3 * 48 + (this->mouseshelfnum % 3) * 4 + i / 4 + (i % 4) * 12];
								ShelfElement* elem = this->insertshelf->elements[i];
                                GLuint butex = elem->tex;
                                elem->tex = copy_tex(binel->tex);
                                if (butex != -1) glDeleteTextures(1, &butex);
								binel->tex = copy_tex(elem->tex);
								binel->path = elem->path;
								binel->jpegpath = elem->jpegpath;
							}
							this->insertshelf = nullptr;
                            this->mouseshelfnum = -1;
                            this->oldmouseshelfnum = -1;
							//mainprogram->binsscreen = false;
						}
						if (mainprogram->rightmouse) {
							// cancel insert
							for (int i = 0; i < 16; i++) {
								// reset elements of previously hovered shelf block
								BinElement* binel = this->currbin->elements[this->mouseshelfnum / 3 * 48 + (this->mouseshelfnum % 3) * 4 + i / 4 + (i % 4) * 12];
								binel->tex = binel->oldtex;
								binel->type = binel->oldtype;
								binel->name = binel->oldname;
							}
							this->insertshelf = nullptr;
							mainprogram->rightmouse = false;
							mainprogram->menuactivation = false;
							mainprogram->binsscreen = false;
						}
						this->oldmouseshelfnum = this->mouseshelfnum;
					}
					else {
						mainprogram->tooltipbox = nullptr;
					}
					if ((!this->menubinel || cond1) && mainprogram->menuactivation) {
						// set binelmenu entries when mouse in shelf block box but not on full bin element
						std::vector<std::string> binel;
						this->binelmenuoptions.clear();
						binel.push_back("Open file(s) from disk");
						binelmenuoptions.push_back(BET_OPENFILES);
						binel.push_back("Insert deck A");
						binelmenuoptions.push_back(BET_INSDECKA);
						binel.push_back("Insert deck B");
						binelmenuoptions.push_back(BET_INSDECKB);
						binel.push_back("Insert full mix");
						binelmenuoptions.push_back(BET_INSMIX);
						binel.push_back("Load block in shelf A");
						binelmenuoptions.push_back(BET_LOADSHELFA);
						binel.push_back("Load block in shelf B");
						binelmenuoptions.push_back(BET_LOADSHELFB);
						binel.push_back("HAP encode entire bin");
						binelmenuoptions.push_back(BET_HAPBIN);
						binel.push_back("Quit");
						binelmenuoptions.push_back(BET_QUIT);
						mainprogram->make_menu("binelmenu", mainprogram->binelmenu, binel);
					}
				}
			}
		}
		if (insertbox) {
			draw_box(red, nullptr, insertbox, -1);
			delete insertbox;
			mainprogram->tooltipbox = nullptr;
		}
		mainprogram->frontbatch = false;


		if (this->mouseshelfnum == -1 && !this->menubinel && mainprogram->menuactivation) {
			// set binelmenu entries when mouse over nothing
			std::vector<std::string> binel;
			this->binelmenuoptions.clear();
			binel.push_back("Save project");
			binelmenuoptions.push_back(BET_SAVPROJ);
			binel.push_back("HAP encode entire bin");
			binelmenuoptions.push_back(BET_HAPBIN);
			binel.push_back("Quit");
			binelmenuoptions.push_back(BET_QUIT);
			mainprogram->make_menu("binelmenu", mainprogram->binelmenu, binel);
		}

        std::unique_ptr <Box> box = std::make_unique <Box> ();
		bool found = false;
		bool cond2 = (mainprogram->mx < mainprogram->xvtxtoscr(1.475f));
		if ((!this->menubinel || cond1) && cond2) {
			// not in bin element -> leftmouse draws box select
			bool full = false;
			if (this->menubinel) {
				if (this->menubinel->name != "") full = true;
			}
			if (mainprogram->leftmousedown && !full && !this->inputtexes.size() && !mainprogram->intopmenu && !this->renamingelem && mainprogram->quitting == "") {
				mainprogram->leftmousedown = false;
				if (!this->selboxing) {
					this->selboxing = true;
					this->selboxx = mainprogram->mx;
					this->selboxy = mainprogram->my;
					if (!mainprogram->ctrl && !mainprogram->shift) {
                        for (int i = 0; i < 12; i++) {
                            for (int j = 0; j < 12; j++) {
                                BinElement *binel = this->currbin->elements[i * 12 + j];
                                binel->select = false;
                            }
                        }
                    }
				}
			}
		}
		if (this->selboxing) {
			box->vtxcoords->x1 = -1.0f + mainprogram->xscrtovtx(this->selboxx);
			box->vtxcoords->h = mainprogram->yscrtovtx(mainprogram->my - this->selboxy);
			box->vtxcoords->y1 = 1.0f - mainprogram->yscrtovtx(this->selboxy);
			if (box->vtxcoords->h > 0) box->vtxcoords->y1 -= box->vtxcoords->h;
			else box->vtxcoords->h = -box->vtxcoords->h;
			box->vtxcoords->w = mainprogram->xscrtovtx(mainprogram->mx - this->selboxx);
			if (box->vtxcoords->w < 0) {
				box->vtxcoords->x1 += box->vtxcoords->w;
				box->vtxcoords->w = -box->vtxcoords->w;
			}
			box->upvtxtoscr();
			mainprogram->frontbatch = true;
			draw_box(white, nullptr, box, -1);
			mainprogram->frontbatch = false;
			// select bin elements inside selection box
			for (int i = 0; i < 12; i++) {
				for (int j = 0; j < 12; j++) {
					Box* ebox = this->elemboxes[i * 12 + j];
					BinElement* binel = this->currbin->elements[i * 12 + j];
					if (binel->name == "") continue;
					if (binel->boxselect) binel->select = false;
					if (box->in(ebox->scrcoords->x1, ebox->scrcoords->y1) || box->in(ebox->scrcoords->x1 + ebox->scrcoords->w, ebox->scrcoords->y1) || box->in(ebox->scrcoords->x1, ebox->scrcoords->y1 - ebox->scrcoords->h) || box->in(ebox->scrcoords->x1 + ebox->scrcoords->w, ebox->scrcoords->y1 - ebox->scrcoords->h)) {
                        binel->select = true;
                        binel->boxselect = true;
						if (!found) {
						    this->firsti = i;
						    this->firstj = j;
						}
						found = true;
					}
				}
			}
			if (mainprogram->leftmouse) {
			    for (int i = 0; i < 144; i++) {
			       this->currbin->elements[i]->boxselect = false;
			    }
			    this->selboxing = false;
			    mainprogram->leftmouse = false;
			}
		}
	}


	// manage SEND button
    auto put_in_buffer = [](const char* str, char* walk) {
	    // buffer utility
        for (int i = 0; i < strlen(str); i++) {
            *walk++ = str[i];
        }
        char *nll = "\0";
        *walk++ = *nll;
        return walk;
    };

    Box box;
    box.vtxcoords->x1 = -0.83f;
    box.vtxcoords->y1 = -0.98f;
    box.vtxcoords->w = 0.16f;
    box.vtxcoords->h = 0.085f;
    box.upvtxtoscr();
    Box ipbox;
    ipbox.vtxcoords->x1 = -0.67f;
    ipbox.vtxcoords->y1 = -0.98f;
    ipbox.vtxcoords->w = 0.15f;
    ipbox.vtxcoords->h = 0.085f;
    ipbox.upvtxtoscr();
    Box connbox;
    connbox.vtxcoords->x1 = -0.47f;
    connbox.vtxcoords->y1 = -0.98f;
    connbox.vtxcoords->w = 0.15f;
    connbox.vtxcoords->h = 0.085f;
    connbox.upvtxtoscr();
    Box seatbox;
    seatbox.vtxcoords->x1 = 0.04f;
    seatbox.vtxcoords->y1 = -0.98f;
    seatbox.vtxcoords->w = 0.3f;
    seatbox.vtxcoords->h = 0.085f;
    seatbox.upvtxtoscr();
    draw_box(&seatbox, -1);
    render_text("Seatname:", white, -0.05f, -0.95f, 0.00075f, 0.0012f);
    if (seatbox.in()) {
        if (mainprogram->leftmouse) {
            mainprogram->renamingseat = true;
            mainprogram->renaming = EDIT_STRING;
            mainprogram->inputtext = mainprogram->seatname;
            mainprogram->cursorpos0 = mainprogram->inputtext.length();
            SDL_StartTextInput();
        }
    }
    if (mainprogram->renamingseat == false) {
        render_text(mainprogram->seatname, white, 0.05f, -0.95f, 0.00075f, 0.0012f);
    } else {
        if (mainprogram->renaming == EDIT_NONE) {
            mainprogram->renamingseat = false;
            mainprogram->seatname = mainprogram->inputtext;
        } else if (mainprogram->renaming == EDIT_CANCEL) {
            mainprogram->renamingseat = false;
        } else {
            do_text_input(seatbox.vtxcoords->x1 + 0.02f,
                          seatbox.vtxcoords->y1 + 0.03f, 0.00075f, 0.0012f, mainprogram->mx, mainprogram->my,
                          mainprogram->xvtxtoscr(0.3f), 0, nullptr, false);
        }
    }
    if (!mainprogram->server) {
         draw_box(&box, -1);
         if (mainprogram->connected == 0) {
            render_text("START SERVER", white, -0.805f, -0.95f, 0.00075f, 0.0012f);
            if (inet_pton(AF_INET, mainprogram->serverip.c_str(), &mainprogram->serv_addr_server.sin_addr) <= 0) {
                printf("\nInvalid server address/ Address not supported \n");
            }
            else {
                if (mainprogram->connfailed) {
                    if (mainprogram->connfailedmilli > 1000) mainprogram->connfailed = false;
                    draw_box(white, darkred1, &connbox, -1);
                    render_text("FAILED", white, -0.445f, -0.95f, 0.00075f, 0.0012f);
                }
                else {
                    draw_box(&connbox, -1);
                    render_text("TRY CONNECT", white, -0.450f, -0.95f, 0.00075f, 0.0012f);
                    if (connbox.in() && mainprogram->leftmouse) {
                        int opt = 1;
                        std::thread sockclient(&Program::socket_client, mainprogram, mainprogram->serv_addr_client,
                                               opt);
                        sockclient.detach();
                    }
                }
            }
        }
        else if (mainprogram->connected == 1) {
            draw_box(white, darkgreen1, &box, -1);
            render_text("CONNECTED", white, -0.805f, -0.95f, 0.00075f, 0.0012f);
        }
        draw_box(&ipbox, -1);
        if (ipbox.in()) {
            if (mainprogram->leftmouse) {
                mainprogram->renamingip = true;
                mainprogram->renaming = EDIT_STRING;
                mainprogram->inputtext = mainprogram->serverip;
                mainprogram->cursorpos0 = mainprogram->inputtext.length();
                SDL_StartTextInput();
            }
        }
        if (mainprogram->renamingip == false) {
            render_text(mainprogram->serverip, white, -0.65f, -0.95f, 0.00075f, 0.0012f);
        } else {
            if (mainprogram->renaming == EDIT_NONE) {
                mainprogram->renamingip = false;
                mainprogram->serverip = mainprogram->inputtext;
            } else if (mainprogram->renaming == EDIT_CANCEL) {
                mainprogram->renamingip = false;
            } else {
                do_text_input(ipbox.vtxcoords->x1 + 0.02f,
                              ipbox.vtxcoords->y1 + 0.03f, 0.00075f, 0.0012f, mainprogram->mx, mainprogram->my,
                              mainprogram->xvtxtoscr(0.15f), 0, nullptr, false);
            }
        }
    }
    else {
        draw_box(white, darkgreen1, &ipbox, -1);
        render_text("SERVER @", white, -0.645f, -0.95f, 0.00075f, 0.0012f);
        render_text(mainprogram->serverip, white, -0.45f, -0.95f, 0.00075f, 0.0012f);
    }
    if (box.in() && mainprogram->connected == 0) {
        if (mainprogram->leftmouse) {
            // start server
            mainprogram->serverip = mainprogram->localip; // local ip is server ip
            if (inet_pton(AF_INET, mainprogram->localip.c_str(), &mainprogram->serv_addr_server.sin_addr) <= 0) {
                printf("\nInvalid server address/ Address not supported \n");
            } else {
                int opt = 1;
                mainprogram->server = true;
                std::thread sockserver(&Program::socket_server, mainprogram, mainprogram->serv_addr_server,
                                       opt);
                sockserver.detach();
            }
        }
    }

    if (mainprogram->connsocknames.size()) {
        box.vtxcoords->x1 = -0.28f;
        box.vtxcoords->y1 = -0.98f;
        box.vtxcoords->w = 0.15f;
        box.vtxcoords->h = 0.085f;
        box.upvtxtoscr();
        draw_box(&box, -1);
        render_text("SHARE BIN", white, -0.255f, -0.95f, 0.00075f, 0.0012f);
        if (box.in()) {
            if (mainprogram->leftmouse) {
                mainprogram->make_menu("sendmenu", mainprogram->sendmenu, mainprogram->connsocknames);
                mainprogram->sendmenu->state = 2;
                mainprogram->sendmenu->menux = mainprogram->mx;
                mainprogram->sendmenu->menuy = mainprogram->my;
            }
        }
    }

    // handle sendmenu
    int k;
    if (mainprogram->sendmenu) {
        k = mainprogram->handle_menu(mainprogram->sendmenu);
        if (k > -1) {
            binsmain->currbin->sendtonames.push_back(mainprogram->connsocknames[k]);
            int sock;
            if (mainprogram->server) sock = mainprogram->connsockets[k];
            else sock = mainprogram->sock;
            char buf[148480] = {0};
            char *walk = buf;
            walk = put_in_buffer(mainprogram->seatname.c_str(), walk);
            if (this->currbin->shared) {
                walk = put_in_buffer(this->currbin->name.c_str(), walk);
            }
            else  {
                binsmain->do_save_bin(mainprogram->temppath + "bin_to_copy");
                binsmain->new_bin(this->currbin->name + " (SHARED)");
                binsmain->open_bin(mainprogram->temppath + "bin_to_copy", binsmain->currbin);
                walk = put_in_buffer((this->currbin->name + " (SHARED)").c_str(), walk);
            }
            for (int i = 0; i < 12; i++) {
                for (int j = 0; j < 12; j++) {
                    BinElement *binel = this->currbin->elements[j * 12 + i];
                    walk = put_in_buffer(binel->name.c_str(), walk);
                    walk = put_in_buffer(binel->path.c_str(), walk);
                }
            }
            char buf2[148495] = {0};
            char *walk2 = buf2;
            walk2 = put_in_buffer("BIN_SENT", walk2);
            walk2 = put_in_buffer(
                    (std::to_string(walk - &buf[0] + (std::to_string(walk - &buf[0])).size() + 10)).c_str(),
                    walk2);
            for (int i = 0; i < walk - &buf[0]; i++) {
                *walk2 = buf[i];  // append buf to buf2
                walk2++;
            }
            send(sock, buf2, walk2 - &buf2[0], 0);
        }
        if (mainprogram->menuchosen) {
            mainprogram->menuchosen = false;
            mainprogram->menuactivation = 0;
            mainprogram->menuresults.clear();
        }
    }

    // recieve sent bins
    for (int i = 0; i < binsmain->messages.size(); i++) {
        if (mainprogram->server) {
            // send recieved bins through from server to destination clients
            for (int j = 0; j < mainprogram->connsockets.size(); j++) {
                if (mainprogram->connsockets[j] != mainprogram->connmap[binsmain->messagesocknames[i]]) {
                    send(mainprogram->connsockets[j], binsmain->rawmessages[i],
                         binsmain->messagelengths[i], 0);
                }
            }
        }
        // process messages
        char *walk = binsmain->messages[i];
        std::string str(walk);
        walk += strlen(walk) + 1;

        Bin *binis = nullptr;
        for (Bin *bin : binsmain->bins) {
            if (bin->name == str) {
                binis = bin;
                break;
            }
        }
        if (!binis) binis = new_bin(str);
        make_currbin(binis->pos);
        binis->shared = true;

        for (int i = 0; i < 12; i++) {
            for (int j = 0; j < 12; j++) {
                BinElement *binel = this->currbin->elements[j * 12 + i];
                std::string name(walk);
                walk += strlen(walk) + 1;
                std::string path(walk);
                walk += strlen(walk) + 1;
                binel->name = name;
                mainprogram->paths.push_back(path);
            }
        }

        this->openfilesbin = true;
        this->menuactbinel = binsmain->currbin->elements[0];  // loading starts from first bin element
    }
    binsmain->messages.clear();
    binsmain->rawmessages.clear();
    binsmain->messagelengths.clear();
    binsmain->messagesocknames.clear();



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
		}
	}

	// set lay to current layer or start layer
	Layer *lay = nullptr;		
	if (mainmix->currlay[!mainprogram->prevmodus]) lay = mainmix->currlay[!mainprogram->prevmodus];
	else {
		if (mainprogram->prevmodus) lay = mainmix->layersA[0];
		else lay = mainmix->layersAcomp[0];
	}
		
	if (mainprogram->rightmouse && this->currbinel && !mainprogram->dragbinel) {
		// rightmouse cancelling of file opening
		bool cont = false;
		if (this->prevbinel) {
			// 
			if (this->inputtexes.size()) {
                int ii = this->previ - this->firsti;
                int jj = this->prevj - this->firstj;
                for (int k = 0; k < this->inputtexes.size(); k++) {
                    int epos = 0;
                    epos = ii * 12 + jj + k;
                    BinElement *dirbinel;
                    if (0 <= epos && epos < 144) {
                        dirbinel = this->currbin->elements[epos];
                    }
                    else continue;
                    dirbinel->tex = dirbinel->oldtex;
				}

				this->currbinel = nullptr;
				this->inputtexes.clear();
                mainprogram->rightmouse = false;
                mainprogram->menuactivation = false;
			}
		}
		for (int i = 0; i < this->movebinels.size(); i++) {
			this->movebinels[i]->select = false;
		}
		this->movebinels.clear();
	}
	
	if (draw) {
		//handle binslist scroll
		Box binsbox;
		binsbox.vtxcoords->x1 = 0.50f;
		binsbox.vtxcoords->y1 = -1.0f;
		binsbox.vtxcoords->w = 0.3f;
		binsbox.vtxcoords->h = 1.0f;
		binsbox.upvtxtoscr();
		draw_box(red, black, &binsbox, -1);
		if (binsbox.in()) {
			// mousewheel scroll
			this->binsscroll -= mainprogram->mousewheel;
			if (this->binsscroll < 0) this->binsscroll = 0;
			if (this->bins.size() > 20 && this->bins.size() - this->binsscroll < 20) this->binsscroll = this->bins.size() - 19;
		}

		// draw and handle binslist scrollboxes
		this->binsscroll = mainprogram->handle_scrollboxes(this->binsscrollup, this->binsscrolldown, this->bins.size(), this->binsscroll, 19);

		//draw and handle binslist
		this->indragbox = false;
		for (int i = 0; i < this->bins.size() - this->binsscroll; i++) {
			Bin* bin = this->bins[i + this->binsscroll];
			bin->pos = i;
			bin->box->vtxcoords->y1 = (i + 1) * -0.05f;
			bin->box->upvtxtoscr();
			if (bin->box->in() && !this->dragbin) {
				if (mainprogram->renaming == EDIT_NONE) {
					if (mainprogram->leftmousedown && !this->dragbinsense) {
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
			if (mainprogram->renaming != EDIT_NONE && bin == this->menubin) {
				// bin renaming with keyboard
				do_text_input(bin->box->vtxcoords->x1 + 0.015f, bin->box->vtxcoords->y1 + 0.018f, 0.00045f, 0.00075f, mainprogram->mx, mainprogram->my, mainprogram->xvtxtoscr(0.3f - 0.03f), 0, nullptr);
			}
			else render_text(bin->name, white, bin->box->vtxcoords->x1 + 0.015f, bin->box->vtxcoords->y1 + 0.018f, 0.00045f, 0.00075f);
		}
		if (!this->indragbox && this->dragbinsense) {
			// dragging has moved (!this->draginbox) so start doing it
			this->dragbin = this->bins[this->dragbinpos];
			this->dragbinsense = false;
		}

		// draw and handle box that allows adding a new bin to the end of the list0
		Box* box = this->newbinbox;
		if (box->in()) {
			if (mainprogram->leftmouse && !this->dragbin) {
			    new_bin(find_unused_filename("new bin", mainprogram->project->binsdir, ".bin"));
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
		render_text("+ NEW BIN", red, 0.62f, -1.0f + 0.018f, 0.00045f, 0.00075f);

		// draw and handle box that allows loading a bin at the end of the list
		box = this->loadbinbox;
		if (box->in()) {
			if (mainprogram->leftmouse && !this->dragbin) {
				mainprogram->pathto = "OPENBIN";
				std::thread filereq(&Program::get_multinname, mainprogram, "Open file(s)", "application/ewocvj2-bin", boost::filesystem::canonical(mainprogram->currbinsdir).generic_string());
				filereq.detach();
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
		render_text("+ LOAD BIN(S)", red, 0.62f, -0.95f + 0.018f, 0.00045f, 0.00075f);

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
				if (mainprogram->my > under2 && mainprogram->my < upper) {
					// draw dragged box
					draw_box(white, black, this->dragbin->box->vtxcoords->x1, 1.0f - mainprogram->yscrtovtx(under1), this->dragbin->box->vtxcoords->w, this->dragbin->box->vtxcoords->h, -1);
					render_text(this->dragbin->name, white, this->dragbin->box->vtxcoords->x1 + 0.015f, 1.0f - mainprogram->yscrtovtx(under1) + 0.018f, 0.00045f, 0.00075f);
					pos = i;
					break;
				}
			}
			if (mainprogram->lmover) {
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

		// bin element renaming
		if (mainprogram->renaming != EDIT_NONE && this->renamingelem) {
			// bin element renaming with keyboard
			mainprogram->frontbatch = true;
			draw_box(white, black, this->renamingbox, -1);
			do_text_input(-0.5f + 0.1f, -0.2f + 0.05f, 0.0009f, 0.0015f, mainprogram->mx, mainprogram->my,
                 mainprogram->xvtxtoscr(0.8f), 0, nullptr);
            mainprogram->frontbatch = false;
		}

		// render hap encoding text on elems
		for (int j = 0; j < 12; j++) {
			for (int i = 0; i < 12; i++) {
				// handle elements, row per row
				Box* box = this->elemboxes[i * 12 + j];
				box->upvtxtoscr();
				BinElement* binel = this->currbin->elements[i * 12 + j];
				if (draw) {
					// show if element encoding/awaiting encoding
					if (binel->encwaiting) {
						render_text("Waiting...", white, box->vtxcoords->x1 + 0.0075f, box->vtxcoords->y1 + box->vtxcoords->h - 0.0225f, 0.0005f, 0.0008f);
					}
					else if (binel->encoding) {
						float progress = binel->encodeprogress;
						if (binel->type == ELEM_DECK || binel->type == ELEM_MIX) {
							if (binel->allhaps) {
								progress /= binel->allhaps;
							}
							else progress = 0.0f;
						}
						render_text("Encoding...", white, box->vtxcoords->x1 + 0.0075f, box->vtxcoords->y1 + box->vtxcoords->h - 0.0225f, 0.0005f, 0.0008f);
						draw_box(black, white, box->vtxcoords->x1, box->vtxcoords->y1 + box->vtxcoords->h - 0.0675f, progress * 0.1f, 0.02f, -1);
					}
				}
			}
		}




		// Draw and handle binmenu and binelmenu	
		if (this->bins.size() > 1) {
			int k = mainprogram->handle_menu(mainprogram->binmenu);
			if (k == 0) {
				// delete bin
				boost::filesystem::remove(this->menubin->path);
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
			}
			else if (k == 1) {
				// start renaming bin
				mainprogram->backupname = this->menubin->name;
				mainprogram->inputtext = this->menubin->name;
				mainprogram->cursorpos0 = mainprogram->inputtext.length();
				SDL_StartTextInput();
				mainprogram->renaming = EDIT_BINNAME;
			}
			else if (k == 2) {
				// import bin
				mainprogram->pathto = "IMPORTBIN";
				std::thread filereq(&Program::get_inname, mainprogram, "Import bin(s)", "application/ewocvj2-bin", boost::filesystem::canonical(mainprogram->currbinfilesdir).generic_string());
				filereq.detach();
			}
		}
		else {
			int k = mainprogram->handle_menu(mainprogram->bin2menu);  // rightclick on bin when there's only one (can't delete)
			if (k == 0) {
				// start renaming bin
				mainprogram->backupname = this->menubin->name;
				mainprogram->inputtext = this->menubin->name;
				mainprogram->cursorpos0 = mainprogram->inputtext.length();
				SDL_StartTextInput();
				mainprogram->renaming = EDIT_BINNAME;
			}
		}
	}

	if (mainprogram->menuchosen) {
		// menu cleanup
		mainprogram->menuchosen = false;
		mainprogram->menuactivation = 0;
		mainprogram->menuresults.clear();
		mainprogram->menuondisplay = false;
	}


	// handle binelmenu thats been populated above, menuset controls which options sets are used
	k = mainprogram->handle_menu(mainprogram->binelmenu);
	//if (k > -1) this->currbinel = nullptr;
	if (binelmenuoptions.size() && k > -1) {
		if (binelmenuoptions[k] != BET_OPENFILES) this->menuactbinel = nullptr;
        if (binelmenuoptions[k] == BET_DELETE) {
            // delete hovered bin element
            this->delbinels.push_back(this->menubinel);
            mainprogram->del = true;
        }
        else if (binelmenuoptions[k] == BET_DELSEL) {
            // delete selected bin elements
            for (int i = 0; i < 144; i++) {
                BinElement *elem = this->currbin->elements[i];
                if (elem->select) this->delbinels.push_back(elem);
            }
            mainprogram->del = true;
        }
        else if (binelmenuoptions[k] == BET_MOVSEL) {
            // move selected bin elements
            for (int i = 0; i < 12; i++) {
                for (int j = 0; j < 12; j++) {
                    BinElement* binel = this->currbin->elements[i * 12 + j];
                    if (binel->select) {
                        this->movebinels.push_back(binel);
                        this->addpaths.push_back(binel->path);
                        this->inputtexes.push_back(binel->tex);
                        this->inputtypes.push_back(binel->type);
                        this->inputjpegpaths.push_back(binel->jpegpath);
                    }
                    else {
                        this->addpaths.push_back("");
                        this->inputtexes.push_back(-1);
                        this->inputtypes.push_back(ELEM_FILE);
                        this->inputjpegpaths.push_back("");
                    }
                }
            }
        }
		else if (binelmenuoptions[k] == BET_RENAME) {
			// start renaming bin element
			this->renamingelem = this->menubinel;
			this->renamingelem->oldname = this->renamingelem->name;
			std::string name = this->menubinel->name;
			mainprogram->backupname = name;
			mainprogram->inputtext = name;
			mainprogram->cursorpos0 = mainprogram->inputtext.length();
			SDL_StartTextInput();
			mainprogram->renaming = EDIT_BINELEMNAME;
		}
		else if (binelmenuoptions[k] == BET_LOADDECKA) {
			// load hovered deck in deck A
			mainprogram->binsscreen = false;
			mainmix->mousedeck = 0;
			mainmix->open_deck(this->menubinel->path, 1);
		}
		else if (binelmenuoptions[k] == BET_LOADDECKB) {
			// load hovered deck in deck B
			mainprogram->binsscreen = false;
			mainmix->mousedeck = 1;
			mainmix->open_deck(this->menubinel->path, 1);
		}
		else if (binelmenuoptions[k] == BET_LOADMIX) {
			// load hovered mix into decks
			mainprogram->binsscreen = false;
			mainmix->open_mix(this->menubinel->path.c_str(), true);
		}
		else if (binelmenuoptions[k] == BET_OPENFILES) {
			// open videos/images/layer files into bin
			mainprogram->pathto = "OPENFILESBIN";
			std::thread filereq(&Program::get_multinname, mainprogram, "Open file(s)", "", boost::filesystem::canonical(mainprogram->currbinfilesdir).generic_string());
			filereq.detach();
		}
		else if (binelmenuoptions[k] == BET_INSDECKA) {
			// insert deck A into bin
			mainprogram->paths.clear();
			mainmix->mousedeck = 0;
			std::string path = find_unused_filename("deckA", mainprogram->binsdir, ".deck");
			mainmix->do_save_deck(path, true, true);
			open_handlefile(path);
			this->menubinel->tex = this->inputtexes[0];
			this->menubinel->type = this->inputtypes[0];
			this->menubinel->path = this->addpaths[0];
			this->menubinel->name = remove_extension(this->menubinel->path);
			this->menubinel->oldjpegpath = this->menubinel->jpegpath;
			this->menubinel->jpegpath = this->inputjpegpaths[0];
			// clean up: maybe too much cleared here, doesn't really matter
			this->inputtexes.clear();
			this->inputtypes.clear();
			this->inputjpegpaths.clear();
			this->menuactbinel = nullptr;
			this->addpaths.clear();
			this->prevbinel = nullptr;
		}
		else if (binelmenuoptions[k] == BET_INSDECKB) {
			// insert deck B into bin
			mainprogram->paths.clear();
			mainmix->mousedeck = 1;

            std::string path = find_unused_filename("deckB", mainprogram->binsdir, ".deck");
			mainmix->do_save_deck(path, true, true);
			open_handlefile(path);
			this->menubinel->tex = this->inputtexes[0];
			this->menubinel->type = this->inputtypes[0];
			this->menubinel->path = this->addpaths[0];
			this->menubinel->name = remove_extension(this->menubinel->path);
			this->menubinel->oldjpegpath = this->menubinel->jpegpath;
			this->menubinel->jpegpath = this->inputjpegpaths[0];
			// clean up: maybe too much cleared here, doesn't really matter
			this->inputtexes.clear();
			this->inputtypes.clear();
			this->inputjpegpaths.clear();
			this->menuactbinel = nullptr;
			this->addpaths.clear();
			this->prevbinel = nullptr;
		}
		else if (binelmenuoptions[k] == BET_INSMIX) {
			// insert live mix into bin
			mainprogram->paths.clear();

            std::string path = find_unused_filename("mix", mainprogram->binsdir, ".mix");
            mainmix->do_save_mix(path, mainprogram->prevmodus, true);
			open_handlefile(path);
			this->menubinel->tex = this->inputtexes[0];
			this->menubinel->type = this->inputtypes[0];
			this->menubinel->path = this->addpaths[0];
			this->menubinel->name = remove_extension(this->menubinel->path);
			this->menubinel->oldjpegpath = this->menubinel->jpegpath;
			this->menubinel->jpegpath = this->inputjpegpaths[0];
			// clean up: maybe too much cleared here, doesn't really matter
			this->inputtexes.clear();
			this->inputtypes.clear();
			this->inputjpegpaths.clear();
			this->menuactbinel = nullptr;
			this->addpaths.clear();
			this->prevbinel = nullptr;
		}
		else if (binelmenuoptions[k] == BET_LOADSHELFA) {
			// load hovered block into shelf A
			Shelf* shelf = mainprogram->shelves[0];
			for (int i = 0; i < 16; i++) {
				BinElement* binel = this->currbin->elements[this->mouseshelfnum / 3 * 48 + (this->mouseshelfnum % 3) * 4 + i / 4 + (i % 4) * 12];
				ShelfElement* elem = shelf->elements[i];
				elem->path = binel->path;
				elem->jpegpath = binel->jpegpath;
				elem->type = binel->type;
                GLuint butex = elem->tex;
                elem->tex = copy_tex(binel->tex);
                if (butex != -1) glDeleteTextures(1, &butex);
			}
		}
		else if (binelmenuoptions[k] == BET_LOADSHELFB) {
			// load hovered block into shelf B
			Shelf* shelf = mainprogram->shelves[1];
			for (int i = 0; i < 16; i++) {
				BinElement* binel = this->currbin->elements[this->mouseshelfnum / 3 * 48 + (this->mouseshelfnum % 3) * 4 + i / 4 + (i % 4) * 12];
				ShelfElement* elem = shelf->elements[i];
				elem->path = binel->path;
				elem->jpegpath = binel->jpegpath;
				elem->type = binel->type;
				GLuint butex = elem->tex;
				elem->tex = copy_tex(binel->tex);
                if (butex != -1) glDeleteTextures(1, &butex);
			}
		}
		else if (binelmenuoptions[k] == BET_HAPELEM) {
			if (!this->menubinel->encoding) {
				// hap encode hovered bin element
				if (this->menubinel->type == ELEM_DECK) {
					this->hap_mix(this->menubinel);
				}
				else if (this->menubinel->type == ELEM_MIX) {
					this->hap_deck(this->menubinel);
				}
				else {
					this->hap_binel(this->menubinel, nullptr);
				}
			}
			else this->menubinel->encoding = false; // signal stopping the encoding of this elem
		}
        else if (binelmenuoptions[k] == BET_HAPBIN) {
            // hap encode entire bin
            for (int i = 0; i < 12; i++) {
                for (int j = 0; j < 12; j++) {
                    // elements
                    BinElement* binel = this->currbin->elements[i * 12 + j];
                    if (binel->name != "" && (binel->type == ELEM_FILE || binel->type == ELEM_LAYER)) {
                        this->hap_binel(binel, nullptr);
                    }
                    else if (binel->type == ELEM_DECK) {
                        this->hap_deck(binel);
                    }
                    else if (binel->type == ELEM_MIX) {
                        this->hap_mix(binel);
                    }
                }
            }
        }
        else if (binelmenuoptions[k] == BET_HAPSEL) {
            // hap encode entire bin
            for (int i = 0; i < 12; i++) {
                for (int j = 0; j < 12; j++) {
                    // elements
                    BinElement* binel = this->currbin->elements[i * 12 + j];
                    if (binel->select) {
                        if (binel->name != "" && (binel->type == ELEM_FILE || binel->type == ELEM_LAYER)) {
                            this->hap_binel(binel, nullptr);
                        } else if (binel->type == ELEM_DECK) {
                            this->hap_deck(binel);
                        } else if (binel->type == ELEM_MIX) {
                            this->hap_mix(binel);
                        }
                    }
                }
            }
        }
        else if (binelmenuoptions[k] == BET_QUIT) {
            // quit program
            mainprogram->quitting = "quitted";
        }
        else if (binelmenuoptions[k] == BET_SAVPROJ) {
            // save project
            mainprogram->project->save(mainprogram->project->path);
        }
	}

	if (mainprogram->menuchosen) {
		// menu cleanup
		mainprogram->menuchosen = false;
		mainprogram->menuactivation = 0;
		mainprogram->menuresults.clear();
		mainprogram->menuondisplay = false;
	}


	if (mainprogram->menuactivation && !this->inputtexes.size() && !lay->vidmoving && this->movingtex == -1) {
		// activate binslist or bin menu
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
				if (binel->encoding && binel->encthreads == 0 && (binel->type == ELEM_DECK || binel->type == ELEM_MIX)) {
					binel->encoding = false;
					boost::filesystem::rename(remove_extension(binel->path) + ".temp", binel->path);
				}
				if ((box->in() || mainprogram->rightmouse || binel == menuactbinel) && !this->openfilesbin && !binel->encoding) {
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
						if (!binel->encwaiting && !binel->encoding && binel->name != "" && !this->inputtexes.size() && !lay->vidmoving && !this->insertshelf) {
							if (this->previewbinel != binel) {
								// reset when new element hovered
								this->previewimage = "";
								this->previewbinel = nullptr;
							}
							mainprogram->frontbatch = true;
							if ((this->previewimage != "" || binel->type == ELEM_IMAGE) && !this->binpreview) {
								// do first entry preview preperation/visualisation when image hovered
								this->binpreview = true;  // just entering preview, or already done preparation (different if clauses)
								if (mainprogram->prelay) {
								    mainprogram->prelay->del();
									// close old preview layer
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
								render_text("IMAGE", white, box->vtxcoords->x1 + 0.0075f, box->vtxcoords->y1 + box->vtxcoords->h - 0.0225f, 0.0005f, 0.0008f);
								render_text(std::to_string(w) + "x" + std::to_string(h), white, box->vtxcoords->x1 + 0.0075f, box->vtxcoords->y1 + box->vtxcoords->h - 0.0675f, 0.0005f, 0.0008f);
							}
							else if (this->previewimage != "" || binel->type == ELEM_IMAGE) {
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
								render_text("IMAGE", white, box->vtxcoords->x1 + 0.0075f, box->vtxcoords->y1 + box->vtxcoords->h - 0.0225f, 0.0005f, 0.0008f);
								render_text(std::to_string(w) + "x" + std::to_string(h), white, box->vtxcoords->x1 + 0.0075f, box->vtxcoords->y1 + box->vtxcoords->h - 0.0675f, 0.0005f, 0.0008f);
							}
							else if ((binel->type == ELEM_LAYER) && !this->binpreview) {
								// do first entry preview preperation/visualisation when layer file hovered
								if (remove_extension(basename(binel->path)) != "") {
                                    if (mainprogram->prelay) {
                                        mainprogram->prelay->del();
                                        // close old preview layer
                                    }
									this->binpreview = true;
									draw_box(red, black, 0.52f, 0.5f, 0.4f, 0.4f, -1);
									mainprogram->prelay = new Layer(false);
									mainprogram->prelay->dummy = true;
									mainprogram->prelay->pos = 0;
									mainprogram->prelay->blendnode = mainprogram->nodesmain->currpage->add_blendnode(MIXING, !mainprogram->prevmodus);
									mainprogram->prelay->node = mainprogram->nodesmain->currpage->add_videonode(2);
									mainprogram->prelay->node->layer = mainprogram->prelay;
                                    mainprogram->prelay = mainmix->open_layerfile(binel->path, mainprogram->prelay, true, 0);
                                    mainprogram->prelay->lasteffnode[0] = mainprogram->prelay->node;
                                    mainprogram->prelay->lasteffnode[1] = mainprogram->prelay->node;
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
										mainprogram->prelay->startdecode.notify_all();
									}
									// wait for decode end
									std::unique_lock<std::mutex> lock2(mainprogram->prelay->enddecodelock);
									mainprogram->prelay->enddecodevar.wait(lock2, [&] {return mainprogram->prelay->processed; });
									mainprogram->prelay->processed = false;
									lock2.unlock();
									glActiveTexture(GL_TEXTURE0);
									glBindTexture(GL_TEXTURE_2D, mainprogram->prelay->texture);
									if (mainprogram->prelay->vidformat == 188 || mainprogram->prelay->vidformat == 187) {
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
									GLuint butex = mainprogram->prelay->fbotex;
									mainprogram->prelay->fbotex = copy_tex(mainprogram->prelay->texture);
                                    if (butex != -1) glDeleteTextures(1, &butex);
									// calculate effects
									onestepfrom(0, mainprogram->prelay->node, nullptr, -1, -1);
									if (mainprogram->prelay->effects[0].size()) {
										draw_box(red, black, 0.52f, 0.5f, 0.4f, 0.4f, mainprogram->prelay->effects[0][mainprogram->prelay->effects[0].size() - 1]->fbotex);
									}
									else {
									    draw_box(red, black, 0.52f, 0.5f, 0.4f, 0.4f, mainprogram->prelay->fbotex);
									}
									if (!binel->encoding) {
										// show video format
										if (mainprogram->prelay->vidformat == 188 || mainprogram->prelay->vidformat == 187) {
											render_text("HAP", white, box->vtxcoords->x1 + 0.0075f, box->vtxcoords->y1 + box->vtxcoords->h - 0.0225f, 0.0005f, 0.0008f);
											render_text(std::to_string(mainprogram->prelay->decresult->width) + "x" + std::to_string(mainprogram->prelay->decresult->height), white, box->vtxcoords->x1 + 0.0075f, box->vtxcoords->y1 + box->vtxcoords->h - 0.0675f, 0.0005f, 0.0008f);
										}
										else if (mainprogram->prelay->type != ELEM_IMAGE) {
											render_text("CPU", white, box->vtxcoords->x1 + 0.0075f, box->vtxcoords->y1 + box->vtxcoords->h - 0.0225f, 0.0005f, 0.0008f);
											render_text(std::to_string(mainprogram->prelay->decresult->width) + "x" + std::to_string(mainprogram->prelay->decresult->height), white, box->vtxcoords->x1 + 0.0075f, box->vtxcoords->y1 + box->vtxcoords->h - 0.0675f, 0.0005f, 0.0008f);
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
										mainprogram->prelay->startdecode.notify_all();
									}
									// wait for decode finished
									std::unique_lock<std::mutex> lock(mainprogram->prelay->enddecodelock);
									mainprogram->prelay->enddecodevar.wait(lock, [&] {return mainprogram->prelay->processed; });
									mainprogram->prelay->processed = false;
									lock.unlock();
									glBindTexture(GL_TEXTURE_2D, mainprogram->prelay->texture);
									if (mainprogram->prelay->vidformat == 188 || mainprogram->prelay->vidformat == 187) {
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
									GLuint butex = mainprogram->prelay->fbotex;
									mainprogram->prelay->fbotex = copy_tex(mainprogram->prelay->texture);
                                    if (butex != -1) glDeleteTextures(1, &butex);
									// calculate effects
									onestepfrom(0, mainprogram->prelay->node, nullptr, -1, -1);
									if (mainprogram->prelay->effects[0].size()) {
										draw_box(red, black, 0.52f, 0.5f, 0.4f, 0.4f, mainprogram->prelay->effects[0][mainprogram->prelay->effects[0].size() - 1]->fbotex);
									}
									else {
										draw_box(red, black, 0.52f, 0.5f, 0.4f, 0.4f, mainprogram->prelay->fbotex);
									}
								}
								else {
									// show old image if not mousewheeled through file
									if (mainprogram->prelay->effects[0].size()) {
										draw_box(red, black, 0.52f, 0.5f, 0.4f, 0.4f, mainprogram->prelay->effects[0][mainprogram->prelay->effects[0].size() - 1]->fbotex);
									}
									else {
										draw_box(red, black, 0.52f, 0.5f, 0.4f, 0.4f, mainprogram->prelay->fbotex);
									}
								}
								if (!binel->encoding && remove_extension(basename(binel->path)) != "") {
									// show video format
									if (mainprogram->prelay->vidformat == 188 || mainprogram->prelay->vidformat == 187) {
										render_text("HAP", white, box->vtxcoords->x1 + 0.0075f, box->vtxcoords->y1 + box->vtxcoords->h - 0.0225f, 0.0005f, 0.0008f);
										render_text(std::to_string(mainprogram->prelay->decresult->width) + "x" + std::to_string(mainprogram->prelay->decresult->height), white, box->vtxcoords->x1 + 0.0075f, box->vtxcoords->y1 + box->vtxcoords->h - 0.0675f, 0.0005f, 0.0008f);
									}
									else if (mainprogram->prelay->type != ELEM_IMAGE) {
										render_text("CPU", white, box->vtxcoords->x1 + 0.0075f, box->vtxcoords->y1 + box->vtxcoords->h - 0.0225f, 0.0005f, 0.0008f);
										render_text(std::to_string(mainprogram->prelay->decresult->width) + "x" + std::to_string(mainprogram->prelay->decresult->height), white, box->vtxcoords->x1 + 0.0075f, box->vtxcoords->y1 + box->vtxcoords->h - 0.0675f, 0.0005f, 0.0008f);
									}
								}
							}
							else if ((binel->type == ELEM_FILE) && !this->binpreview) {
								// do first entry preview preparation/visualisation when video hovered
								if (remove_extension(basename(binel->path)) != "") {
                                    if (mainprogram->prelay) {
                                        mainprogram->prelay->del();
                                        // close old preview layer
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
										mainprogram->prelay->startdecode.notify_all();
									}
									// wait for decode finished
									std::unique_lock<std::mutex> lock2(mainprogram->prelay->enddecodelock);
									mainprogram->prelay->enddecodevar.wait(lock2, [&] {return mainprogram->prelay->processed; });
									mainprogram->prelay->processed = false;
									lock2.unlock();
									glBindTexture(GL_TEXTURE_2D, this->binelpreviewtex);
									if (mainprogram->prelay->vidformat == 188 || mainprogram->prelay->vidformat == 187) {
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
										if (mainprogram->prelay->vidformat == 188 || mainprogram->prelay->vidformat == 187) {
											render_text("HAP", white, box->vtxcoords->x1 + 0.0075f, box->vtxcoords->y1 + box->vtxcoords->h - 0.0225f, 0.0005f, 0.0008f);
											render_text(std::to_string(mainprogram->prelay->video_dec_ctx->width) + "x" + std::to_string(mainprogram->prelay->video_dec_ctx->height), white, box->vtxcoords->x1 + 0.0075f, box->vtxcoords->y1 + box->vtxcoords->h - 0.0675f, 0.0005f, 0.0008f);
										}
										else {
											if (mainprogram->prelay->type == ELEM_FILE) {
												render_text("CPU", white, box->vtxcoords->x1 + 0.0075f, box->vtxcoords->y1 + box->vtxcoords->h - 0.0225f, 0.0005f, 0.0008f);
												render_text(std::to_string(mainprogram->prelay->video_dec_ctx->width) + "x" + std::to_string(mainprogram->prelay->video_dec_ctx->height), white, box->vtxcoords->x1 + 0.0075f, box->vtxcoords->y1 + box->vtxcoords->h - 0.0675f, 0.0005f, 0.0008f);
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
											mainprogram->prelay->startdecode.notify_all();
										}
										// wait for decode finished
										std::unique_lock<std::mutex> lock(mainprogram->prelay->enddecodelock);
										mainprogram->prelay->enddecodevar.wait(lock, [&] {return mainprogram->prelay->processed; });
										mainprogram->prelay->processed = false;
										lock.unlock();
										glBindTexture(GL_TEXTURE_2D, this->binelpreviewtex);
										if (mainprogram->prelay->vidformat == 188 || mainprogram->prelay->vidformat == 187) {
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
								if (!binel->encoding && remove_extension(basename(binel->path)) != "") {
									// show video format
									if (mainprogram->prelay->vidformat == 188 || mainprogram->prelay->vidformat == 187) {
										render_text("HAP", white, box->vtxcoords->x1 + 0.0075f, box->vtxcoords->y1 + box->vtxcoords->h - 0.0225f, 0.0005f, 0.0008f);
										render_text(std::to_string(mainprogram->prelay->video_dec_ctx->width) + "x" + std::to_string(mainprogram->prelay->video_dec_ctx->height), white, box->vtxcoords->x1 + 0.0075f, box->vtxcoords->y1 + box->vtxcoords->h - 0.0675f, 0.0005f, 0.0008f);
									}
									else {
										if (mainprogram->prelay->type == ELEM_FILE) {
											render_text("CPU", white, box->vtxcoords->x1 + 0.0075f, box->vtxcoords->y1 + box->vtxcoords->h - 0.0225f, 0.0005f, 0.0008f);
											render_text(std::to_string(mainprogram->prelay->video_dec_ctx->width) + "x" + std::to_string(mainprogram->prelay->video_dec_ctx->height), white, box->vtxcoords->x1 + 0.0075f, box->vtxcoords->y1 + box->vtxcoords->h - 0.0675f, 0.0005f, 0.0008f);
										}
									}
								}
							}
							mainprogram->frontbatch = false;
						}

						if (binel->name != "") {
							if (!this->inputtexes.size()) {
                                if (mainprogram->leftmousedown && !mainprogram->dragbinel && !mainprogram->ctrl && !mainprogram->shift) {
                                    // dragging single bin element
                                    if (binel->name != "") {
                                        mainprogram->dragbinel = new BinElement;
                                        mainprogram->dragbinel->tex = binel->tex;
                                        mainprogram->dragbinel->path = binel->path;
                                        mainprogram->dragbinel->name = binel->name;
                                        mainprogram->dragbinel->type = binel->type;
                                        mainprogram->leftmousedown = false;
                                        // start drag
                                        this->movingtex = binel->tex;
                                        this->movingbinel = binel;
                                        this->currbinel = binel;
                                    }
                                }
                                else if (mainprogram->leftmouse) {
                                    if (mainprogram->ctrl) {
                                        binel->select = true;
                                        mainprogram->leftmouse = false;
                                    }
                                }
							}
						}
					}

                    if (this->movebinels.size() && mainprogram->leftmouse) {
                        // confirm elements move, set elements and clean up
                        int ii = i - this->firsti;
                        int jj = j - this->firstj;
                        for (int k = 0; k < this->inputtexes.size(); k++) {
                            int epos = 0;
                            epos = ii * 12 + jj + k;
                            BinElement *dirbinel;
                            if (0 <= epos && epos < 144) {
                                dirbinel = this->currbin->elements[epos];
                            }
                            else continue;
                            if (this->inputtexes[k] != -1) {
                                dirbinel->type = this->inputtypes[k];
                                dirbinel->path = this->addpaths[k];
                                dirbinel->name = remove_extension(basename(dirbinel->path));
                                dirbinel->oldjpegpath = dirbinel->jpegpath;
                                dirbinel->jpegpath = this->inputjpegpaths[k];
                                int pos =
                                        std::find(this->movebinels.begin(), this->movebinels.end(), dirbinel) -
                                        this->movebinels.begin();
                                if (pos < this->movebinels.size()) {
                                    this->movebinels.erase(this->movebinels.begin() + pos);
                                }
                                if (this->movebinels.size()) {
                                    GLuint butex = dirbinel->tex;
                                    dirbinel->tex = copy_tex(dirbinel->tex);
                                    if (butex != -1) glDeleteTextures(1, &butex);
                                }
                            }
                        }

                        for (int i = 0; i < this->movebinels.size(); i++) {
                            if (this->movebinels[i]->tex != -1) {
                                this->movebinels[i]->erase();
                            }
                        }
                        this->movebinels.clear();
                        // clean up
                        this->inputtexes.clear();
                        if (this->currbinel == this->menuactbinel) this->menuactbinel = nullptr;
                        this->currbinel = nullptr;
                        this->prevbinel = nullptr;
                        this->inputtypes.clear();
                        this->addpaths.clear();
                        mainprogram->leftmouse = false;
                    }

                    if (binel != this->currbinel) {
						if (this->currbinel) this->binpreview = false;
                        bool cond1 = false;
                        bool cond2 = false;
						if (mainprogram->dragbinel) {
						    cond1 = (mainprogram->shelfdragelem);
							cond2 = (mainprogram->dragbinel->type == ELEM_DECK || mainprogram->dragbinel->type == ELEM_MIX);
						}
						if (lay->vidmoving || cond1 || cond2) {
							// when dragging layer/mix/deck in from mix view
							if (this->currbinel) {
								//reset old currbinel
								this->currbinel->tex = this->currbinel->oldtex;
								// set new layer drag textures in this bin element
								binel->oldtex = binel->tex;
								binel->tex = mainprogram->dragbinel->tex;
							}
							else {
								// set new layer drag textures in this bin element
								binel->oldtex = binel->tex;
								binel->tex = mainprogram->dragbinel->tex;
							}
							this->currbinel = binel;
						}	

						if (this->inputtexes.size() && !this->menuactbinel) {
							if (this->prevbinel) {
								// when inputting files, set old texes back
                                int ii = this->previ - this->firsti;
                                int jj = this->prevj - this->firstj;
 								for (int k = 0; k < this->inputtexes.size(); k++) {
                                    int epos = 0;
                                    epos = ii * 12 + jj + k;
                                    BinElement *dirbinel;
                                    if (0 <= epos && epos < 144) {
                                        dirbinel = this->currbin->elements[epos];
                                    }
                                    else continue;
                                    if (this->inputtexes[k] != -1) {
                                        dirbinel->select = dirbinel->oldselect;
                                        dirbinel->tex = dirbinel->oldtex;
                                    }
								}
							}

							// set new texes of inputted files
                            int ii = i - this->firsti;
                            int jj = j - this->firstj;
                            for (int k = 0; k < this->inputtexes.size(); k++) {
                                int epos = 0;
                                epos = ii * 12 + jj + k;
                                BinElement *dirbinel;
                                if (0 <= epos && epos < 144) {
                                    dirbinel = this->currbin->elements[epos];
                                }
                                else continue;
                                if (this->inputtexes[k] != -1) {
                                    dirbinel->oldselect = dirbinel->select;
                                    dirbinel->select = false;
                                    dirbinel->oldtex = dirbinel->tex;
                                    dirbinel->tex = this->inputtexes[k];
                                }
							}

							// set values to allow for resetting old texes when inputfiles are moved around
							this->prevbinel = binel;
							this->previ = i;
							this->prevj = j;
						}
					}

					if (this->inputtexes.size() && binel == this->menuactbinel && !this->openfilesbin) {
						bool cont = false;
						// set values of elems of opened files
						for (int k = 0; k < this->inputtexes.size(); k++) {
                            int cnt = 0;
                            int epos = -1;
                            bool firstdone = false;
                            for (int o = (i / 4) * 3 + j / 4; o < 9; o++) {
                                for (int n = (i % 4) * !firstdone; n < 4; n++) {
                                    for (int m = (j % 4) * !firstdone; m < 4; m++) {
                                        if (cnt == k) {
                                            epos = (int)(o / 3) * 48 + (int)(o % 3) * 4 + n * 12 + m;
                                            break;
                                        }
                                        cnt++;
                                    }
                                    firstdone = true;
                                    if (epos >= 0) break;
                                }
                                if(epos >= 0) break;
                            }
                            if (epos < 144) {
                                BinElement *dirbinel = this->currbin->elements[epos];
                                if (this->addpaths[k] == dirbinel->path) continue;
                                dirbinel->tex = this->inputtexes[k];
                                dirbinel->type = this->inputtypes[k];
                                dirbinel->path = this->addpaths[k];
                                dirbinel->name = remove_extension(basename(dirbinel->path));
                                dirbinel->oldjpegpath = dirbinel->jpegpath;
                                dirbinel->jpegpath = this->inputjpegpaths[k];
                            }
						}
						// clean up
						this->inputtexes.clear();
						this->inputtypes.clear();
						this->menuactbinel = nullptr;
						this->addpaths.clear();
					}


					BinElement *tempbinel = this->currbinel;
					if (this->currbinel && this->movingtex != -1) {
						if (binel != this->currbinel) {
							this->currbinel->tex = this->currbinel->oldtex;
							this->movingbinel->tex = binel->tex;
							binel->oldtex = binel->tex;
							binel->tex = this->movingtex;
							this->currbinel = binel;
						}
					}
					else this->currbinel = binel;
				}
				

				if (mainprogram->del) {
					// handle element deleting
					for (int k = 0; k < this->delbinels.size(); k++) {
						// deleting single bin element
						blacken(this->delbinels[k]->oldtex);
						this->delbinels[k]->tex = this->delbinels[k]->oldtex;
						std::string name = remove_extension(basename(this->delbinels[k]->path));
						this->delbinels[k]->path = this->delbinels[k]->oldpath;
						this->delbinels[k]->name = this->delbinels[k]->oldname;
						this->delbinels[k]->jpegpath = this->delbinels[k]->oldjpegpath;
						this->delbinels[k]->type = this->delbinels[k]->oldtype;
						this->delbinels[k]->select = false;
						boost::filesystem::remove(this->delbinels[k]->jpegpath);
					}
					this->delbinels.clear();
					mainprogram->del = false;
				}
			}
		}

		if (!inbinel) this->binpreview = false;

		if (inbinel && !mainprogram->rightmouse && (lay->vidmoving || mainprogram->shelfdragelem) && mainprogram->lmover) {
			// confirm layer dragging from main view and set influenced bin element to the right values
			this->currbinel->type = mainprogram->dragbinel->type;
			this->currbinel->path = mainprogram->dragbinel->path;
			if (this->currbinel->type == ELEM_LAYER) {
			    std::string p1;
			    if (lay->vidmoving) p1 = lay->filename;
			    else p1 = mainprogram->shelfdragelem->path;
                this->currbinel->path = find_unused_filename( basename(p1),
                                                    mainprogram->project->binsdir + this->currbin->name + "/", ""
                                                                                                               ".layer");
                this->currbinel->name = remove_extension(basename(this->currbinel->path));
                mainmix->save_layerfile(this->currbinel->path, lay, 1, 0);
			}
			this->currbinel->name = remove_extension(basename(this->currbinel->path));
			this->currbinel->full = true;
			this->currbinel = nullptr;
			enddrag();
			lay->vidmoving = false;
			mainmix->moving = false;
		}
		else if ((mainprogram->lmover || mainprogram->rightmouse) && mainprogram->dragbinel) {
			//when dropping on grey area
			bool cond = false;
			if (mainprogram->dragbinel) {
				cond = (mainprogram->dragbinel->type == ELEM_DECK || mainprogram->dragbinel->type == ELEM_MIX);
			}
			if (lay->vidmoving) {
				// when layer/mix/deck dragging from mix view
				if (this->currbinel) {
					this->currbinel->tex = this->currbinel->oldtex;
					this->currbinel = nullptr;
				}
			}
			else if (this->movingtex != -1) {
				// drop: when element dragging inside bin
				bool found = false;
				BinElement* foundbinel;
				for (int j = 0; j < 12; j++) {
					for (int i = 0; i < 12; i++) {
						Box* box = this->elemboxes[i * 12 + j];
						foundbinel = this->currbin->elements[i * 12 + j];
						if (box->in() && this->currbinel == foundbinel) {
							found = true;
							break;
						}
					}
				}
				if (!found) {
					this->currbinel->tex = this->movingbinel->oldtex;
					this->movingbinel->tex = this->movingtex;
					this->currbinel = nullptr;
					this->movingbinel = nullptr;
					this->movingtex = -1;
				}
				else {
					std::swap(this->currbinel->type, this->movingbinel->type);
					std::swap(this->currbinel->path, this->movingbinel->path);
					std::swap(this->currbinel->name, this->movingbinel->name);
					std::swap(this->currbinel->jpegpath, this->movingbinel->jpegpath);  // one way?
					this->currbinel = nullptr;
					this->movingbinel = nullptr;
					this->movingtex = -1; }
			}
			enddrag();
		}
	}

	// load one file into bin each loop, at end to allow drawing ordering dialog on top
	if (this->openfilesbin) {
		open_files_bin();
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
	int pos;
	std::string istring;
	safegetline(rfile, istring);
	//check if binfile
	while (safegetline(rfile, istring)) {
		if (istring == "ENDOFFILE") {
			break;
		}
		else if (istring == "ELEMS") {
			// open bin elements
			while (safegetline(rfile, istring)) {
				if (istring == "ENDOFELEMS") break;
				if (istring == "POS") {
					safegetline(rfile, istring);
					pos = std::stoi(istring);
				}
                if (istring == "ABSPATH") {
                    safegetline(rfile, istring);
                    bin->elements[pos]->path = istring;
                    if (!exists(bin->elements[pos]->path)) bin->elements[pos]->path = "";
                }
                if (istring == "RELPATH") {
                    safegetline(rfile, istring);
                    if (istring == "") continue;
                    if (bin->elements[pos]->path == "") {
                        boost::filesystem::current_path(mainprogram->project->binsdir);
                        bin->elements[pos]->path = pathtoplatform(boost::filesystem::absolute(istring).string());
                        boost::filesystem::current_path(mainprogram->contentpath);
                        if (!exists(bin->elements[pos]->path)) {
                            mainmix->retargeting = true;
                            mainmix->newbinelpaths.push_back(bin->elements[pos]->path);
                            mainmix->newpathbinels.push_back(bin->elements[pos]);
                        }
                    }
                }
				if (istring == "NAME") {
					safegetline(rfile, istring);
					bin->elements[pos]->name = istring;
				}
				if (istring == "TYPE") {
					safegetline(rfile, istring);
					bin->elements[pos]->type = (ELEM_TYPE)std::stoi(istring);
					ELEM_TYPE type = bin->elements[pos]->type;
					if ((type == ELEM_LAYER || type == ELEM_DECK || type == ELEM_MIX) && bin->elements[pos]->name != "") {
						if (concat) {
							boost::filesystem::rename(result + "_" + std::to_string(filecount) + ".file", bin->elements[pos]->path);
							filecount++;
						}
					}
				}
				if (istring == "JPEGPATH") {
					safegetline(rfile, istring);
					bin->elements[pos]->jpegpath = istring;
					if (bin->elements[pos]->name != "") {
						if (bin->elements[pos]->jpegpath != "") {
							if (concat) {
								bin->elements[pos]->jpegpath = find_unused_filename(this->currbin->name + "_" +
								        basename(bin->elements[pos]->jpegpath), mainprogram->temppath, ".jpg");
								boost::filesystem::rename(result + "_" + std::to_string(filecount) + ".file", bin->elements[pos]->jpegpath);
								open_thumb(bin->elements[pos]->jpegpath, bin->elements[pos]->tex);
								filecount++;
							}
							else open_thumb(bin->elements[pos]->jpegpath, bin->elements[pos]->tex);
						}
					}
				}
			}
		}
        else if (istring == "SHARED") {
            // shared state
            safegetline(rfile, istring);
            bin->shared = std::stoi(istring);
        }
	}

	rfile.close();
}

void BinsMain::save_bin(const std::string& path) {
	//std::thread binsav(&BinsMain::do_save_bin, this, path);
	//binsav.detach();
	this->do_save_bin(path);
}

void BinsMain::do_save_bin(const std::string& path) {
	// save bin file
	std::vector<std::string> filestoadd;
	std::ofstream wfile;
	wfile.open(path.c_str());
	wfile << "EWOCvj BINFILE\n";
	
	wfile << "ELEMS\n";
	// save elements
	for (int i = 0; i < 12; i++) {
		for (int j = 0; j < 12; j++) {
			wfile << "POS\n";
			wfile << i * 12 + j;
			wfile << "\n";
			wfile << "ABSPATH\n";
			wfile << this->currbin->elements[i * 12 + j]->path;
			wfile << "\n";
            wfile << "RELPATH\n";
            wfile << boost::filesystem::relative(this->currbin->elements[i * 12 + j]->path, mainprogram->project->binsdir).string();
            wfile << "\n";
			wfile << "NAME\n";
			wfile << this->currbin->elements[i * 12 + j]->name;
			wfile << "\n";
			wfile << "TYPE\n";
			wfile << std::to_string(this->currbin->elements[i * 12 + j]->type);
			wfile << "\n";
			ELEM_TYPE type = this->currbin->elements[i * 12 + j]->type;
			if ((type == ELEM_LAYER || type == ELEM_DECK || type == ELEM_MIX) && this->currbin->elements[i * 12 + j]->name != "") {
				filestoadd.push_back(this->currbin->elements[i * 12 + j]->path);
			}
			if (this->currbin->elements[i * 12 + j]->name != "") {
				if (!exists(this->currbin->elements[i * 12 + j]->jpegpath)) {
                    this->currbin->elements[i * 12 + j]->jpegpath = find_unused_filename(this->currbin->name, mainprogram->temppath, ".jpg");
					save_thumb(this->currbin->elements[i * 12 + j]->jpegpath, this->currbin->elements[i * 12 + j]->tex);
				}
				filestoadd.push_back(this->currbin->elements[i * 12 + j]->jpegpath);
			}			
			wfile << "JPEGPATH\n";
			wfile << this->currbin->elements[i * 12 + j]->jpegpath;
			wfile << "\n";
		}
	}
	wfile << "ENDOFELEMS\n";

    wfile << "SHARED\n";
    wfile << this->currbin->shared;
    wfile << "\n";

	wfile << "ENDOFFILE\n";
	wfile.close();
	
	std::string tcbpath = find_unused_filename("tempconcatbin", mainprogram->temppath, "");
	std::ofstream outputfile;
	outputfile.open(tcbpath, std::ios::out | std::ios::binary);
	std::vector<std::vector<std::string>> filestoadd2;
	filestoadd2.push_back(filestoadd);
	concat_files(outputfile, path, filestoadd2);
	outputfile.close();
	boost::filesystem::remove(path);
	boost::filesystem::rename(tcbpath, path);
}

Bin *BinsMain::new_bin(const std::string &name) {
	Bin *bin = new Bin(this->bins.size());
	bin->name = name;
	this->bins.push_back(bin);
	bin->pos = this->bins.size() - 1;
	//this->currbin->elements.clear();
	
	for (int i = 0; i < 12; i++) {
		for (int j = 0; j < 12; j++) {
			BinElement *binel = new BinElement;
			bin->elements.push_back(binel);
		}
	}
	make_currbin(this->bins.size() - 1);
	std::string path;
	bin->path = mainprogram->project->binsdir + name + ".bin";
	boost::filesystem::path p1{mainprogram->project->binsdir + name};
	boost::filesystem::create_directory(p1);
	return bin;
}

void BinsMain::make_currbin(int pos) {
	// make bin on position pos current
	this->currbin = this->bins[pos];
	this->currbin->name = this->bins[pos]->name;
	this->currbin->path = this->bins[pos]->path;
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
	safegetline(rfile, istring);
	//check if is binslistfile
	safegetline(rfile, istring);
	int currbin = std::stoi(istring);
	for (int i = 0; i < this->bins.size(); i++) {
		delete this->bins[i];
	}
	this->bins.clear();
	while (safegetline(rfile, istring)) {
		Bin *newbin;
		if (istring == "ENDOFFILE") break;
		if (istring == "BINS") {
			while (safegetline(rfile, istring)) {
				if (istring == "ENDOFBINS") break;
				newbin = new_bin(istring);
			}
		}
		if (istring == "BINSSCROLL") {
			safegetline(rfile, istring);
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

void BinsMain::import_bins() {
    auto next_bin = []()
    {
        mainprogram->shelffileselem++;
        mainprogram->shelffilescount++;
        binsmain->binscount++;
        if (binsmain->binscount == mainprogram->paths.size()) {
            binsmain->importbins = false;
            mainprogram->paths.clear();
        }
    };

    std::string result = deconcat_files(mainprogram->paths[binsmain->binscount]);
    bool concat = (result != "");
    std::ifstream rfile;
    if (concat) rfile.open(result);
    else rfile.open(mainprogram->paths[binsmain->binscount]);
    std::string istring;
    safegetline(rfile, istring);
    if (istring != "EWOCvj BINFILE") {
        next_bin();
        return;
    }

	Bin* bin = binsmain->new_bin(remove_extension(basename(mainprogram->paths[binsmain->binscount])));
	binsmain->open_bin(mainprogram->paths[binsmain->binscount], bin);
	std::string path = mainprogram->project->binsdir + bin->name + ".bin";
	if (binsmain->bins.size() >= 20) binsmain->binsscroll++;
    next_bin();
}

void BinsMain::open_files_bin() {
    // open videos/images/layer files into bin

    if (!currbin->shared) {
        // order elements
        if (mainprogram->paths.size() == 0) {
            binsmain->openfilesbin = false;
            mainprogram->multistage = 0;
            return;
        }
        bool cont = mainprogram->order_paths(false);
        if (!cont) return;


        mainprogram->blocking = true;
        if (SDL_GetMouseFocus() != mainprogram->mainwindow) {
            SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW));
        } else {
            SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT));
        }
    }

	if (mainprogram->counting == mainprogram->paths.size()) {
		this->currbin->path = mainprogram->project->binsdir + this->currbin->name + ".bin";
		this->openfilesbin = false;
		mainprogram->paths.clear();
		mainprogram->multistage = 0;
		mainprogram->blocking = false;
		SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW));
		return;
	}
	std::string str = mainprogram->paths[mainprogram->counting];
	open_handlefile(str, mainprogram->pathtexes[mainprogram->counting]);
	mainprogram->counting++;
}

void BinsMain::open_handlefile(const std::string &path, GLuint tex) {
    if (path != "") {
        // prepare value lists for inputting videos/images/layer files from disk
        ELEM_TYPE endtype;
        GLuint endtex;

        // determine file type
        std::string istring = "";
        std::string result = deconcat_files(path);
        if (!mainprogram->openerr) {
            bool concat = (result != "");
            std::ifstream rfile;
            if (concat) rfile.open(result);
            else rfile.open(path);
            safegetline(rfile, istring);
        } else mainprogram->openerr = false;
        if (istring == "EWOCvj LAYERFILE") {
            // prepare layer file for bin entry
            endtype = ELEM_LAYER;
            if (tex == -1) endtex = get_layertex(path);
            else endtex = tex;
        } else if (istring == "EWOCvj DECKFILE") {
            // prepare layer file for bin entry
            endtype = ELEM_DECK;
            if (tex == -1) endtex = get_deckmixtex(path);
            else endtex = tex;
        } else if (istring == "EWOCvj MIXFILE") {
            // prepare layer file for bin entry
            endtype = ELEM_MIX;
            if (tex == -1) endtex = get_deckmixtex(path);
            else endtex = tex;
        } else if (isimage(path)) {
            // prepare image file for bin entry
            endtype = ELEM_IMAGE;
            if (tex == -1) endtex = get_imagetex(path);
            else endtex = tex;
        } else if (isvideo(path)) {
            // prepare video file for bin entry
            endtype = ELEM_FILE;
            if (tex == -1) endtex = get_videotex(path);
            else endtex = tex;

        } else if (mainprogram->openerr) {
            return;
        }

        if (endtex == -1) return;
        this->inputtexes.push_back(endtex);
        this->inputtypes.push_back(endtype);
        std::string jpath = find_unused_filename(basename(path), mainprogram->temppath, ".jpg");
        save_thumb(jpath, copy_tex(endtex));
        this->inputjpegpaths.push_back(jpath);
    }
    else {
        this->inputtexes.push_back(-1);
        this->inputtypes.push_back(ELEM_FILE);
        this->inputjpegpaths.push_back("");
    }
    this->addpaths.push_back(path);
}


std::tuple<std::string, std::string> BinsMain::hap_binel(BinElement *binel, BinElement *bdm) {
	// encode single bin element, possibly contained in a deck or a mix
	this->binpreview = false;
   	std::string apath = "";
	std::string rpath = "";
	if (binel->type == ELEM_FILE) {
		std::thread hap (&BinsMain::hap_encode, this, binel->path, binel, bdm);
		if (!mainprogram->threadmode || binel->bin == nullptr) {
			#ifdef WINDOWS
			SetThreadPriority((void*)hap.native_handle(), THREAD_PRIORITY_LOWEST);
			#else
			struct sched_param params;
			params.sched_priority = sched_get_priority_min(SCHED_FIFO);
			pthread_setschedparam(hap.native_handle(), SCHED_FIFO, &params);
			#endif
		}
		hap.detach();
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
		while (safegetline(rfile, istring)) {
			if (istring == "FILENAME") {
				safegetline(rfile, istring);
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
				safegetline(rfile, istring);
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
            find_stream_index(&idx, video, AVMEDIA_TYPE_VIDEO);
			cpm = video->streams[idx]->codecpar;
			if (cpm->codec_id == 188 || cpm->codec_id == 187) {
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
				#ifdef WINDOWS
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

	while (safegetline(rfile, istring)) {
		if (istring == "FILENAME") {
			safegetline(rfile, istring);
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
			safegetline(rfile, istring);
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
		while (safegetline(rfile, istring)) {
			if (istring == "FILENAME") {
				safegetline(rfile, istring);
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
				safegetline(rfile, istring);
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
    find_stream_index(&source_stream_idx, source, AVMEDIA_TYPE_VIDEO);
	source_stream = source->streams[source_stream_idx];
	source_dec_cpm = source_stream->codecpar;
	const AVCodec *scodec = avcodec_find_decoder(source_stream->codecpar->codec_id);
	source_dec_ctx = avcodec_alloc_context3(scodec);
	r = avcodec_parameters_to_context(source_dec_ctx, source_dec_cpm);
	source_dec_ctx->time_base = { 1, 1000 };
	source_dec_ctx->framerate = { source_stream->r_frame_rate.num, source_stream->r_frame_rate.den };
	r = avcodec_open2(source_dec_ctx, scodec, nullptr);
	if (source_dec_cpm->codec_id == 188 || source_dec_cpm->codec_id == 187) {
		binel->encwaiting = false;
		binel->encoding = false;
		if (bdm) {
			bdm->encthreads--;
		}
		return;
	}

	if (binel->bin) {
        while (mainprogram->encthreads >= mainprogram->maxthreads) {
            mainprogram->hapnow = false;
            std::unique_lock<std::mutex> lock(mainprogram->hapmutex);
            mainprogram->hap.wait(lock, [&] { return mainprogram->hapnow; });
            lock.unlock();
        }
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
    //c->framerate = (AVRational){source_stream->r_frame_rate.num, source_stream->r_frame_rate.den};
	c->sample_aspect_ratio = source_dec_cpm->sample_aspect_ratio;
    c->pix_fmt = codec->pix_fmts[0];  
    int rem = source_dec_cpm->width % 32;
    c->width = source_dec_cpm->width + (32 - rem) * (rem > 0);
    rem = source_dec_cpm->height % 4;
    c->height = source_dec_cpm->height + (4 - rem) * (rem > 0);
    //c->global_quality = 0;
   	r = avcodec_open2(c, codec, nullptr);
       
    std::string destpath = remove_extension(srcpath) + "_temp.mov";
    avformat_alloc_output_context2(&dest, av_guess_format("mov", nullptr, "video/mov"), nullptr, destpath.c_str());
    dest_stream = avformat_new_stream(dest, codec);
    dest_stream->time_base = source_stream->time_base;
	r = avcodec_parameters_from_context(dest_stream->codecpar, c);
    if (r < 0) {
		av_log(nullptr, AV_LOG_ERROR, "Copying stream context failed\n");
		av_log(nullptr, AV_LOG_ERROR, "Copying parameters for stream #%u failed\n");
    }   
    dest_stream->r_frame_rate = source_stream->r_frame_rate;
    dest->oformat->flags = AVFMT_NOFILE;
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
		bool cond = false;
		if (bdm) cond = (bdm->encoding == false);
		if (binel->encoding == false || cond) {
			if (bdm) {
				bdm->encthreads--;
				delete binel;  // temp bin elements populate bdm binelements
			}
			mainprogram->encthreads--;
			avio_close(dest->pb);
			boost::filesystem::remove(destpath); // delete the hap file under construction
			return;
		}
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
    dest_stream->duration = (count + 1) * av_rescale_q(1, c->time_base, dest_stream->time_base);
    av_write_trailer(dest);
    avio_close(dest->pb);
    avcodec_free_context(&c);
    av_frame_free(&nv12frame);
    av_packet_unref(&pkt);
    binel->path = remove_extension(binel->path) + "_hap.mov";
	boost::filesystem::rename(destpath, binel->path);
    binel->encoding = false;
    if (binel->otflay) {
        binel->otflay->encodeload = true;
        binel->otflay->open_video(binel->otflay->frame, binel->path, false);
        binel->otflay->encodeload = false;
        binel->otflay->hapbinel = nullptr;
    }
    if (bdm) {
   		bdm->encthreads--;
		delete binel;  // temp bin elements populate bdm binelements
    }
	mainprogram->encthreads--;
	mainprogram->hapnow = true;
	mainprogram->hap.notify_all();
}

