#if defined(WIN32) && !defined(__linux__)
#define WINDOWS
#elif defined(__linux__) && !defined(WIN32)
#define POSIX
#endif

#include <thread>

#include "GL/glew.h"
#include "GL/gl.h"
#define FREEGLUT_STATIC
#define _LIB
#define FREEGLUT_LIB_PRAGMAS 0
#include "GL/freeglut.h"

#ifdef POSIX
#include <Xos_fixindexmacro.h>
#include <arpa/inet.h>
#endif

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#ifdef WINDOWS
#include <windows.h>
#endif

#include "SDL2/SDL.h"
#include "SDL2/SDL_syswm.h"

#include <ostream>
#include <fstream>
#include <ios>
#include <filesystem>

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

// my own header
#include "program.h"
#include "UPnPPortMapper.h"




Bin::Bin(int pos) {
	// boxes of bins in binslist
	this->box = new Boxx;
	this->box->vtxcoords->x1 = 0.51f;
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
    blacken(this->tex);
    glGenTextures(1, &this->oldtex);
    glBindTexture(GL_TEXTURE_2D, this->oldtex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    blacken(this->oldtex);
}

BinElement::~BinElement() {
	glDeleteTextures(1, &this->tex);
	if (this->oldtex != -1) glDeleteTextures(1, &this->oldtex);
}

void BinElement::erase(bool deletetex) {
	this->select = false;
	this->path = "";
    this->oldpath = "";
    this->relpath = "";
	this->name = "";
	this->oldname = "";
    this->jpegpath = "";
    this->absjpath = "";
    this->reljpath = "";
	this->oldjpegpath = "";
	this->type = ELEM_FILE;
	this->oldtype = ELEM_FILE;
    GLuint tex = this->tex;
    this->tex = copy_tex(tex, 192, 108);
    if (deletetex) {
        //glDeleteTextures(1, &tex);      for undo
    }
	blacken(this->tex);
	blacken(this->oldtex);
}

BinsMain::BinsMain() {
	for (int i = 0; i < 12; i++) {
		for (int j = 0; j < 12; j++) {
			// initialize all 144 bin elements
			Boxx *box = new Boxx;
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
			box->tooltip = "Shows thumbnail of media bin element, either being a video file ("
                           "grey border) or an image (white border) or a layer file (orange border) or a deck file (purple border) or mix file (green border).  Hovering over this element shows video resolution and video compression method (CPU or HAP).  Mousewheel skips through the element contents (previewed in larger monitor topright).  Leftdrag allows dragging to mix screen via wormgate, going past the right screen border.  Leftclickdrag allows moving elements inside the media bin. You can also box select elements.  Rightclickmenu allows among other things, HAP encoding and also loading of a yellowbordered grid-block of mediabin elements into one of the shelves. ";
		}
	}

	this->binsbox = new Boxx;
	this->binsbox->vtxcoords->x1 = 0.51f;
	this->binsbox->vtxcoords->y1 = -1.0f;
	this->binsbox->vtxcoords->w = 0.3f;
	this->binsbox->vtxcoords->h = 1.0f;
	this->binsbox->upvtxtoscr();

	this->hapmodebox = new Boxx;
	this->hapmodebox->vtxcoords->x1 = 0.67f;
	this->hapmodebox->vtxcoords->y1 = 0.65f;
	this->hapmodebox->vtxcoords->w = 0.1f;
	this->hapmodebox->vtxcoords->h = 0.075f;
	this->hapmodebox->upvtxtoscr();
	this->hapmodebox->tooltiptitle = "HAP encode modus ";
	this->hapmodebox->tooltip = "Toggles between hap encoding for during live situations (only using one core, not to slow down the realtime video mix, and hap encoding at full power (using 'number of system cores + 1' threads). ";

	// arrow box to scroll bins list up
	this->binsscrollup = new Boxx;
	this->binsscrollup->vtxcoords->x1 = 0.485f;
	this->binsscrollup->vtxcoords->y1 = -0.05f;
	this->binsscrollup->vtxcoords->w = 0.025f;
	this->binsscrollup->vtxcoords->h = 0.05f;
	this->binsscrollup->upvtxtoscr();
	this->binsscrollup->tooltiptitle = "Scroll bins list up ";
	this->binsscrollup->tooltip = "Leftclicking scrolls the bins list up ";

	// arrow box to scroll bins list down
    this->binsscrolldown = new Boxx;
    this->binsscrolldown->vtxcoords->x1 = 0.485f;
    this->binsscrolldown->vtxcoords->y1 = -1.0f;
    this->binsscrolldown->vtxcoords->w = 0.025f;
    this->binsscrolldown->vtxcoords->h = 0.05f;
    this->binsscrolldown->upvtxtoscr();
    this->binsscrolldown->tooltiptitle = "Scroll bins list down ";
    this->binsscrolldown->tooltip = "Leftclicking scrolls the bins list down ";

    this->floatbox = new Boxx;
    this->floatbox->vtxcoords->x1 = 0.8f;
    this->floatbox->vtxcoords->y1 = 0.95f;
    this->floatbox->vtxcoords->w = 0.15f;
    this->floatbox->vtxcoords->h = 0.05f;
    this->floatbox->upvtxtoscr();
    this->floatbox->tooltiptitle = "Float/dock bins screen ";
    this->floatbox->tooltip = "Leftclick toggles between a docked bins screen (swapped with mix screen) or a floating bins screen (shown on a separate screen). ";
}

void BinsMain::solve_nameclashes() {
	for (int i = 0; i < this->bins.size(); i++) {
		// solve bin name clashes
		for (int j = i + 1; j < this->bins.size(); j++) {
			if (this->bins[j]->name == this->bins[i]->name) {
				std::string oldname = this->bins[i]->name;
				this->bins[j]->path = find_unused_filename(this->bins[j]->name, mainprogram->project->binsdir, ".bin");
				this->bins[j]->name = basename(remove_extension(this->bins[j]->path));
				// this map solves name clashes between normal bins and incoming shared bins
				if (this->bins[i]->shared) {
					this->sharedbinnamesmap[this->bins[j]->name] = oldname;
				}
				// If bin[j] is a shared bin with an idstr, update the idtonamemap
				if (this->bins[j]->shared && !this->bins[j]->idstr.empty()) {
					this->idtonamemap[this->bins[j]->idstr] = this->bins[j]->name;
				}
				break;
			}
		}
	}
}

void BinsMain::handle(bool draw) {
	this->solve_nameclashes();

    // correct loaded path when using LOAD BIN
    for (int i = 0; i < this->bins.size(); i++) {
        this->bins[i]->path = mainprogram->project->binsdir + this->bins[i]->name + ".bin";
    }

    int numd = SDL_GetNumVideoDisplays();
    if (numd > 1) {
        draw_box(this->floatbox, -1);
        if (this->floating) render_text("DOCK", white, this->floatbox->vtxcoords->x1 + 0.02f, this->floatbox->vtxcoords->y1 + 0.01f, 0.00045f, 0.00075f);
        else render_text("CHOOSE MONITOR", white, this->floatbox->vtxcoords->x1 + 0.02f, this->floatbox->vtxcoords->y1 + 0.01f, 0.00045f, 0.00075f);
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
                else {
                    this->floating = false;
                }
            }
        }
    }

    mainprogram->handle_bintargetmenu();

	if (this->renamingelem) {
		if (mainprogram->renaming == EDIT_NONE) {
			this->renamingelem = nullptr;
		}
		if (mainprogram->leftmousedown && !mainprogram->renamingbox->in()) {
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
			binsmain->binrenamemap.erase(this->currbin->name);
			this->currbin->name = this->backupname;
			this->currbin = nullptr;
			mainprogram->rightmouse = false;
			mainprogram->menuactivation = false;
		}
	}

	//draw binelements
	if (!mainprogram->menuondisplay) this->menubinel = nullptr;
	if (mainprogram->menuactivation) this->menubinel = nullptr;
	if (draw) {
		if (this->selboxing) mainprogram->menuactivation = false;
		for (int j = 0; j < 12; j++) {
			for (int i = 0; i < 12; i++) {
				Boxx* box = this->elemboxes[i * 12 + j];
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
					color[1] = 1.0f;
					color[2] = 1.0f;
					color[3] = 1.0f;
				}
				else {
					color[0] = 0.4f;
					color[1] = 0.4f;
					color[2] = 0.4f;
					color[3] = 1.0f;
				}
				// visualize elements
				//draw_box(nullptr, color, box->vtxcoords->x1 - 0.01f, box->vtxcoords->y1 - 0.01f, box->vtxcoords->w + 0.02f, box->vtxcoords->h + 0.02f, -1);
				//draw_box(box, -1);  //in case of alpha thumbnail
				if (binel->select) {
					mainprogram->uniformCache->setBool("inverteff", true);
				}
				draw_box(box, binel->tex);
				mainprogram->uniformCache->setBool("inverteff", false);
                if (!this->inbin) {
                    //mainprogram->frontbatch = true;
                }
                // grey areas next to each element column to cut off element titles
                draw_box(nullptr, color, box->vtxcoords->x1 - 0.01f, box->vtxcoords->y1 - 0.035f, box->vtxcoords->w + 0.02f, 0.028f, -1);
                if (!this->inbin) {
                    //mainprogram->frontbatch = false;
                }
                if (binel->name != "") {
                    if (binel->name != "") render_text(binel->name.substr(0, 20), white, box->vtxcoords->x1, box->vtxcoords->y1 - 0.03f, 0.00045f, 0.00075f);
				}
     		}
		}

		bool cond1 = false;
		if (this->menubinel) cond1 = (this->menubinel->name == "");
		if (!mainprogram->menuondisplay) this->mouseshelfnum = -1;
        Boxx *insertbox = nullptr;
		mainprogram->frontbatch = true;
		for (int j = 0; j < 3; j++) {
			for (int i = 0; i < 3; i++) {
			    Boxx dumbox;
			    Boxx *box = &dumbox;
 				box->vtxcoords->x1 = -0.96f + j * 0.48f;
				box->vtxcoords->y1 = 0.91f - ((i % 3) + 1) * 0.6f;
				box->vtxcoords->w = 0.48f;
				box->vtxcoords->h = 0.6f;
				box->upvtxtoscr();
				draw_box(yellow, nullptr, box, -1);
				if (box->in()) {
					this->mouseshelfnum = i * 3 + j;
					if (this->insertshelf) {
						insertbox = box;
                        if (insertbox) {
                            draw_box(red, nullptr, insertbox, -1);
                            //delete insertbox;
                            delete mainprogram->tooltipbox;
                            mainprogram->tooltipbox = nullptr;
                        }
						mainprogram->leftmousedown = false;
						// inserting a shelf into one of the bin shelf blocks
						if (this->oldmouseshelfnum != -1) {
							if (this->oldmouseshelfnum != this->mouseshelfnum) {
								for (int i = 0; i < 16; i++) {
									// reset elements of previously hovered shelf block
									BinElement* binel = this->currbin->elements[this->oldmouseshelfnum / 3 * 48 + (this->oldmouseshelfnum % 3) * 4 + (i / 4) * 12 + (i % 4)];
									binel->tex = binel->oldtex;
									binel->type = binel->oldtype;
									binel->name = binel->oldname;
								}
							}
						}
						if (this->oldmouseshelfnum != this->mouseshelfnum) {
							// temporary change when hovering
							for (int i = 0; i < 16; i++) {
								BinElement* binel = this->currbin->elements[this->mouseshelfnum / 3 * 48 + (this->mouseshelfnum % 3) * 4 + (i / 4) * 12 + (i % 4)];
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
								BinElement* binel = this->currbin->elements[this->mouseshelfnum / 3 * 48 + (this->mouseshelfnum % 3) * 4 + (i / 4) * 12 + (i % 4)];
								ShelfElement* elem = this->insertshelf->elements[i];
                                GLuint butex = elem->tex;
                                elem->tex = copy_tex(binel->tex);
                                if (butex != -1) glDeleteTextures(1, &butex);
								binel->tex = copy_tex(elem->tex, this->elemboxes[0]->scrcoords->w, this->elemboxes[0]->scrcoords->h);
                                binel->type = elem->type;
								binel->path = elem->path;
                                binel->name = remove_extension(basename(binel->path));
                                if (binel->path != "") {
                                    binel->jpegpath = find_unused_filename(binel->name, mainprogram->project->binsdir +
                                                                                        this->currbin->name + "/",
                                                                           ".jpg");
                                    save_thumb(binel->jpegpath, binel->tex);
                                    binel->oldjpegpath = binel->jpegpath;
                                    binel->absjpath = binel->jpegpath;
                                    if (binel->absjpath != "") {
                                        binel->reljpath = std::filesystem::relative(binel->absjpath,
                                                                                    mainprogram->project->binsdir).generic_string();
                                    }
                                }
 							}
							this->insertshelf = nullptr;
                            this->mouseshelfnum = -1;
                            this->oldmouseshelfnum = -1;
							//mainprogram->binsscreen = false;
						}
						if (mainprogram->rightmouse) {
							// cancel shelf insert
							for (int i = 0; i < 16; i++) {
								// reset elements of previously hovered shelf block
								BinElement* binel = this->currbin->elements[this->mouseshelfnum / 3 * 48 + (this->mouseshelfnum % 3) * 4 + (i / 4) * 12 + (i % 4)];
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
                        delete mainprogram->tooltipbox;
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

        std::unique_ptr <Boxx> box = std::make_unique <Boxx> ();
		bool found = false;
		bool cond2 = (mainprogram->mx < mainprogram->xvtxtoscr(1.475f));
		cond2 = true;
        if ((!this->menubinel || cond1) && cond2) {
			// not in bin element -> leftmouse draws box select
			bool full = false;
			if (this->menubinel) {
				if (this->menubinel->name != "") full = true;
			}
			if (mainprogram->leftmousedown && !full && !this->inputtexes.size() && !mainprogram->intopmenu && !this->renamingelem && mainprogram->quitting == "") {
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
					//mainprogram->rightmouse = false;
					//mainprogram->rightmousedown = false;
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
					Boxx* ebox = this->elemboxes[i * 12 + j];
					BinElement* binel = this->currbin->elements[i * 12 + j];
					if (binel->name == "") continue;
					if (binel->boxselect) binel->select = false;
					if (box->in(ebox->scrcoords->x1, ebox->scrcoords->y1) || box->in(ebox->scrcoords->x1 + ebox->scrcoords->w, ebox->scrcoords->y1) || box->in(ebox->scrcoords->x1, ebox->scrcoords->y1 - ebox->scrcoords->h) || box->in(ebox->scrcoords->x1 + ebox->scrcoords->w, ebox->scrcoords->y1 - ebox->scrcoords->h)) {
                        if (!binel->encoding && !binel->encwaiting) {
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
			}
			if (mainprogram->lmover) {
			    for (int i = 0; i < 144; i++) {
			       this->currbin->elements[i]->boxselect = false;
			    }
			    this->selboxing = false;
			    //mainprogram->leftmouse = false;
			}
		}
	}


	// manage SEND button
    Boxx box;
    box.vtxcoords->x1 = -0.64f;
    box.vtxcoords->y1 = -0.98f;
    box.vtxcoords->w = 0.16f;
    box.vtxcoords->h = 0.085f;
    box.upvtxtoscr();
    Boxx ipbox;
    ipbox.vtxcoords->x1 = -0.67f;
    ipbox.vtxcoords->y1 = -0.98f;
    ipbox.vtxcoords->w = 0.15f;
    ipbox.vtxcoords->h = 0.085f;
    ipbox.upvtxtoscr();
    Boxx connbox;
    connbox.vtxcoords->x1 = -0.64f;
    connbox.vtxcoords->y1 = -0.98f;
    connbox.vtxcoords->w = 0.15f;
    connbox.vtxcoords->h = 0.085f;
    connbox.upvtxtoscr();
    Boxx seatbox;
    seatbox.vtxcoords->x1 = 0.18f;
    seatbox.vtxcoords->y1 = -0.98f;
    seatbox.vtxcoords->w = 0.3f;
    seatbox.vtxcoords->h = 0.085f;
    seatbox.upvtxtoscr();
    draw_box(&seatbox, -1);
    render_text("Seatname:", white, 0.08f, -0.95f, 0.00075f, 0.0012f);
    if (seatbox.in()) {
        this->selboxing = false;
        if (mainprogram->leftmouse) {
			mainprogram->leftmouse = false;
            mainprogram->renamingseat = true;
            mainprogram->renaming = EDIT_STRING;
            mainprogram->inputtext = mainprogram->seatname;
            mainprogram->cursorpos0 = mainprogram->inputtext.length();
            SDL_StartTextInput();
        }
    }
    if (mainprogram->renamingseat == false) {
        render_text(mainprogram->seatname, white, 0.19f, -0.95f, 0.00075f, 0.0012f);
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
        if (mainprogram->connected == 0) {
            // Display status
            draw_box(&box, -1);
            bool hasSeats;
            {
                std::lock_guard<std::mutex> lock(mainprogram->discoveryMutex);
                hasSeats = !mainprogram->discoveredSeats.empty();
            }
            if (!hasSeats) {
                render_text("SEARCHING...", white, -0.615f, -0.95f, 0.00075f, 0.0012f);
            } else {
                render_text("CONNECTING...", white, -0.615f, -0.95f, 0.00075f, 0.0012f);
            }

            /*if (mainprogram->connfailed) {
                if (mainprogram->connfailedmilli > 1000) mainprogram->connfailed = false;
                draw_box(white, darkred1, &connbox, -1);
                render_text("FAILED", white, -0.445f, -0.95f, 0.00075f, 0.0012f);
            }*/
        }
        else if (mainprogram->connected == 1) {
            draw_box(white, darkgreen1, &box, -1);
            render_text("CONNECTED", white, -0.615f, -0.95f, 0.00075f, 0.0012f);
        }
    }
    else {
        // Server: show public IP (moved right to make room for IP input)
		Boxx servipbox;
		servipbox.vtxcoords->x1 = -0.64f;
		servipbox.vtxcoords->y1 = -0.98f;
		servipbox.vtxcoords->w = 0.18f;
		servipbox.vtxcoords->h = 0.085f;
		servipbox.upvtxtoscr();
		draw_box(white, darkgreen1, &servipbox, -1);
		render_text("SERVER @", white, -0.62f, -0.95f, 0.00075f, 0.0012f);
		render_text(mainprogram->publicip, white, -0.4f, -0.95f, 0.00075f, 0.0012f);
		render_text(mainprogram->localip, white, -0.25f, -0.95f, 0.00075f, 0.0012f);
    }

    // Manual server IP input box - ALWAYS AVAILABLE
    Boxx serveripbox;
    serveripbox.vtxcoords->x1 = -0.95f;
    serveripbox.vtxcoords->y1 = -0.98f;
    serveripbox.vtxcoords->w = 0.28f;
    serveripbox.vtxcoords->h = 0.085f;
    serveripbox.upvtxtoscr();

    if (!mainprogram->enteringserverip) {
        draw_box(white, darkblue, &serveripbox, -1);
        std::string displayText = mainprogram->manualserverip.empty() ? "LOCAL SERVER" : mainprogram->manualserverip;
        render_text(displayText, white, -0.93f, -0.95f, 0.00075f, 0.0012f);

        // Click to start entering server IP
        if (serveripbox.in()) {
            this->selboxing = false;
            if (mainprogram->leftmouse) {
				mainprogram->leftmouse = false;
                mainprogram->enteringserverip = true;
                mainprogram->renaming = EDIT_STRING;
                mainprogram->inputtext = mainprogram->manualserverip;
                mainprogram->cursorpos0 = mainprogram->inputtext.length();
                SDL_StartTextInput();
            }
        }
    } else {
        // Currently entering server IP
        draw_box(white, darkblue, &serveripbox, -1);
        if (mainprogram->renaming == EDIT_NONE) {
            // User finished entering
            mainprogram->enteringserverip = false;
            mainprogram->manualserverip = mainprogram->inputtext;

            // If valid IP entered, attempt to connect
            if (!mainprogram->manualserverip.empty() && mainprogram->manualserverip != "LOCAL SERVER") {
                // Validate IP before connecting
                struct sockaddr_in test_addr;
                if (inet_pton(AF_INET, mainprogram->manualserverip.c_str(), &test_addr.sin_addr) > 0) {
                    std::cout << "Connecting to remote server IP: " << mainprogram->manualserverip << std::endl;

                    // If we're currently a server, stop being server
                    if (mainprogram->server) {
                        std::cout << "Quitting server role to connect to remote server" << std::endl;
                        mainprogram->server = false;
                        mainprogram->stop_discovery();

                        // Clean up UPnP port mapping
                        if (mainprogram->upnpMapper) {
                            std::cout << "Removing UPnP port mapping..." << std::endl;
                            mainprogram->upnpMapper->removePortMapping(8000, "TCP");
                            delete mainprogram->upnpMapper;
                            mainprogram->upnpMapper = nullptr;
                        }
                    }

                    // Mark as disconnected so socket_client will create fresh socket
                    mainprogram->connected = 0;
                    mainprogram->serverip = mainprogram->manualserverip;
                    mainprogram->autoConnectAttempted = true;

                    if (inet_pton(AF_INET, mainprogram->serverip.c_str(), &mainprogram->serv_addr_client.sin_addr) > 0) {
                        int opt = 1;
                        std::thread sockclient(&Program::socket_client, mainprogram, mainprogram->serv_addr_client, opt);
                        sockclient.detach();
                    }
                } else {
                    std::cout << "Invalid IP address entered: " << mainprogram->manualserverip << std::endl;
                }
            }
        } else {
            // Show input text with cursor - validate and color accordingly
            struct sockaddr_in test_addr;
            bool validIP = (inet_pton(AF_INET, mainprogram->inputtext.c_str(), &test_addr.sin_addr) > 0);
            float* textColor = validIP ? white : red;
            render_text(mainprogram->inputtext, textColor, -0.86f, -0.95f, 0.00075f, 0.0012f);
        }
    }

    if (mainprogram->connsocknames.size()) {
        box.vtxcoords->x1 = -0.1f;
        box.vtxcoords->y1 = -0.98f;
        box.vtxcoords->w = 0.15f;
        // Build list of clients not already in sendtonames
        std::vector<std::string> available_clients;
        for (const auto& client : mainprogram->connsocknames) {
            if (std::find(this->currbin->sendtonames.begin(), this->currbin->sendtonames.end(), client) == this->currbin->sendtonames.end()) {
                available_clients.push_back(client);
            }
        }

        // Only show SHARE BIN box if there are clients we're not already sharing with
        if (!available_clients.empty()) {
            box.vtxcoords->h = 0.085f;
            box.upvtxtoscr();
            draw_box(&box, -1);
            render_text("SHARE BIN", white, -0.075f, -0.95f, 0.00075f, 0.0012f);
            if (box.in()) {
                this->selboxing = false;
                if (mainprogram->leftmouse) {
                    mainprogram->leftmouse = false;
                    mainprogram->make_menu("sendmenu", mainprogram->sendmenu, available_clients);
                    mainprogram->sendmenu->state = 2;
                    mainprogram->sendmenu->menux = mainprogram->mx;
                    mainprogram->sendmenu->menuy = mainprogram->my;
                }
            }
        }
    }

    // handle sendmenu
    int k = -1;
    if (mainprogram->sendmenu) {
        k = mainprogram->handle_menu(mainprogram->sendmenu);
        if (k > -1) {
            // Build list of available clients again to get the correct name
            std::vector<std::string> available_clients;
            for (const auto& client : mainprogram->connsocknames) {
                if (std::find(this->currbin->sendtonames.begin(), this->currbin->sendtonames.end(), client) == this->currbin->sendtonames.end()) {
                    available_clients.push_back(client);
                }
            }

            std::string selected_name;
            if (k < available_clients.size()) {
                selected_name = available_clients[k];
                this->currbin->sendtonames.push_back(selected_name);
            }
			this->currbin->shared = true;
			srand(std::time(0));
			this->currbin->idstr = std::to_string(rand());
			this->currbin->prevnames.clear();  // Clear to force full texture send
			this->currbin->prevtexes.clear();  // Clear to force full texture send

            // If we're a client, send subscription request to server
            // If we're the server, register it locally
			auto put_in_buffer = [](const char* str, char* walk, char* buffer_end) -> char* {
				// buffer utility with bounds checking
				size_t len = strlen(str);
				if (walk + len + 1 > buffer_end) {
					return nullptr;  // Buffer overflow would occur
				}
				for (size_t i = 0; i < len; i++) {
					*walk++ = str[i];
				}
				*walk++ = '\0';
				return walk;
			};
            if (!mainprogram->server && mainprogram->connected) {
                // Send SUBSCRIBE message to server
                char subscribe_msg[1024];
                char* walk = subscribe_msg;
                char* end = subscribe_msg + sizeof(subscribe_msg);

                walk = put_in_buffer("SUBSCRIBE", walk, end);
                if (walk) walk = put_in_buffer(mainprogram->seatname.c_str(), walk, end);
                if (walk) walk = put_in_buffer(this->currbin->idstr.c_str(), walk, end);
                if (walk) walk = put_in_buffer(selected_name.c_str(), walk, end);

                if (walk) {
                    int sent = mainprogram->bl_send(mainprogram->sock, subscribe_msg, walk - subscribe_msg, 0);
                    if (sent != walk - subscribe_msg) {
                        std::cout << "DEBUG: Failed to send subscription request, sent " << sent << " of " << (walk - subscribe_msg) << " bytes" << std::endl;
                    }
                }
            }
            else if (mainprogram->server) {
                // Register subscription locally on server
                std::lock_guard<std::mutex> lock(mainprogram->subscriptionMutex);
                auto key = std::make_pair(mainprogram->seatname, this->currbin->idstr);
                mainprogram->subscriptionMap[key].insert(selected_name);
            }


			this->send_shared_bins();
        }
        if (mainprogram->menuchosen) {
            mainprogram->menuchosen = false;
            mainprogram->menuactivation = 0;
            mainprogram->menuresults.clear();
            mainprogram->recundo = false;
        }
    }

    // intermediate update sends for shared bins
    this->send_shared_bins();

    // set threadmode for hap encoding
    if (!this->binpreview) {
        render_text("HAP Encoding Mode", white, 0.62f, 0.8f, 0.00075f, 0.0012f);
        draw_box(white, black, this->hapmodebox, -1);
        draw_box(white, lightblue, 0.67f + 0.048f * mainprogram->threadmode, 0.6575f, 0.048f, 0.06f, -1);
        render_text("Live mode", white, 0.59f, 0.6f, 0.00075f, 0.0012f);
        render_text("Max mode", white, 0.75f, 0.6f, 0.00075f, 0.0012f);
        render_text("1 thread", white, 0.59f, 0.55f, 0.00075f, 0.0012f);
        mainprogram->maxthreads = mainprogram->numcores * mainprogram->threadmode + 1;
        render_text(std::to_string(mainprogram->numcores + 1) + " threads", white, 0.75f, 0.55f, 0.00075f, 0.0012f);
        if (this->hapmodebox->in()) {
            this->selboxing = false;
            if (mainprogram->leftmouse) {
                mainprogram->threadmode = !mainprogram->threadmode;
            }
        }
    }

	// set lay to current layer or start layer
	Layer *lay = nullptr;
	if (mainmix->currlay[!mainprogram->prevmodus]) lay = mainmix->currlay[!mainprogram->prevmodus];
	else {
		if (mainprogram->prevmodus) lay = mainmix->layers[0][0];
		else lay = mainmix->layers[2][0];
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
                    if (this->inputtexes[k] != -1) dirbinel->tex = dirbinel->oldtex;
				}

				this->currbinel = nullptr;
                this->inputtexes.clear();
                this->inputtypes.clear();
                this->menuactbinel = nullptr;
                this->addpaths.clear();
                this->inputjpegpaths.clear();
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
		draw_box(red, black, this->binsbox, -1);
		if (this->binsbox->in()) {
			// mousewheel scroll
			this->binsscroll -= mainprogram->mousewheel;
			if (this->binsscroll < 0) this->binsscroll = 0;
			if (this->bins.size() > 21 && this->bins.size() - this->binsscroll < 21) this->binsscroll = this->bins.size() - 20;
		}

		// draw and handle binslist scrollboxes
		this->binsscroll = mainprogram->handle_scrollboxes(*this->binsscrollup, *this->binsscrolldown, this->bins.size(), this->binsscroll, 20);

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
                        this->selboxing = false;
						this->dragbinsense = true;
						this->dragbox = bin->box;
						this->dragbinpos = i + this->binsscroll;
						mainprogram->leftmousedown = false;
					}
					if (mainprogram->leftmouse) {
						// click to choose current bin
                        if (this->inputtexes.size() && !this->menuactbinel) {
                            if (this->prevbinel) {
                                // when moving files, set old texes back before switching bin (move)
                                int ii = this->previ - this->firsti;
                                int jj = this->prevj - this->firstj;
                                for (int k = 0; k < this->inputtexes.size(); k++) {
                                    int epos = 0;
                                    epos = ii * 12 + jj + k;
                                    BinElement *dirbinel;
                                    if (0 <= epos && epos < 144) {
                                        dirbinel = this->currbin->elements[epos];
                                    } else {
                                        continue;
                                    }
                                    if (this->inputtexes[k] != -1) {
                                        dirbinel->select = dirbinel->oldselect;
                                        dirbinel->tex = dirbinel->oldtex;
                                    }
                                }
                            }
                            this->prevbinel = nullptr;
                        }
						make_currbin(i);
						this->dragbox = nullptr;
						this->dragbinsense = false;
					}
					if (mainprogram->doubleleftmouse) {
						// start renaming bin
						this->backupname = this->currbin->name;
						mainprogram->inputtext = this->currbin->name;
						mainprogram->cursorpos0 = mainprogram->inputtext.length();
						SDL_StartTextInput();
						mainprogram->renaming = EDIT_BINNAME;
						this->dragbin = nullptr;
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
			std::string namedisplay = bin->name;
			if (bin->shared) {
				namedisplay += " (SHARED)";
			}
			if (mainprogram->renaming != EDIT_NONE && bin == this->currbin) {
				// bin renaming with keyboard
				do_text_input(bin->box->vtxcoords->x1 + 0.025f, bin->box->vtxcoords->y1 + 0.018f, 0.00045f, 0.00075f, mainprogram->mx, mainprogram->my, mainprogram->xvtxtoscr(0.3f - 0.03f), 0, nullptr);
			}
			else render_text(namedisplay, white, bin->box->vtxcoords->x1 + 0.025f, bin->box->vtxcoords->y1 + 0.018f, 0.00045f, 0.00075f);
		}
		if (!this->indragbox && this->dragbinsense) {
			// dragging has moved (!this->draginbox) so start doing it
			this->dragbin = this->bins[this->dragbinpos];
			this->dragbinsense = false;
		}
        if (this->selboxing) {
            mainprogram->leftmousedown = false;
        }

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
					std::string namedisplay = this->dragbin->name;
					if (this->dragbin->shared) {
						namedisplay += " (SHARED)";
					}
					render_text(namedisplay, white, this->dragbin->box->vtxcoords->x1 + 0.025f, 1.0f - mainprogram->yscrtovtx(under1) + 0.018f, 0.00045f, 0.00075f);
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
			draw_box(white, black, mainprogram->renamingbox, -1);
			do_text_input(-0.5f + 0.1f, -0.2f + 0.05f, 0.0009f, 0.0015f, mainprogram->mx, mainprogram->my,
                 mainprogram->xvtxtoscr(0.8f), 0, nullptr);
            mainprogram->frontbatch = false;
		}

		// render hap encoding text on elements
		for (int j = 0; j < 12; j++) {
			for (int i = 0; i < 12; i++) {
				// handle elements, row per row
				Boxx* box = this->elemboxes[i * 12 + j];
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

        if (mainprogram->del) {
            // delete selected bin elements if Delete key pressed
            for (int i = 0; i < 144; i++) {
                BinElement *elem = this->currbin->elements[i];
                if (elem->select) this->delbinels.push_back(elem);
            }
            mainprogram->del = true;
        }

        // Draw and handle binmenu and binelmenu
		bool inbin = false;
		for (int i = 0; i < this->bins.size(); i++) {
			if (this->bins[i]->box->in()) {
				inbin = true;
				break;
			}
		}
		if (this->bins.size() > 1) {
			int k = mainprogram->handle_menu(mainprogram->binmenu);
			if (k == 0) {
				if (!this->dragbin) {
					std::string name = "new bin";
					int count = 0;
					for (Bin* bin : this->bins) {
						if (name == bin->name) {
							count++;
							name = remove_version(name) + "_" + std::to_string(count);
							continue;
						}
					}
					new_bin(name);
					if (this->bins.size() > 20) this->binsscroll++;
				}
			}
			else if (k == 1) {
				if (!this->dragbin) {
					mainprogram->pathto = "OPENBIN";
					std::thread filereq(&Program::get_multinname, mainprogram, "Open file(s)", "application/ewocvj2-bin", std::filesystem::canonical(mainprogram->currbinsdir).generic_string());
					filereq.detach();
				}
			}
			else if (k == 2) {
				if (!this->dragbin) {
					mainprogram->pathto = "SAVEBIN";
					std::thread filereq(&Program::get_outname, mainprogram, "Open file(s)", "application/ewocvj2-bin", std::filesystem::canonical(mainprogram->currbinsdir).generic_string());
					filereq.detach();
				}
			}
			else if (k == 3) {
				// delete bin
				if (this->currbin->shared) {
					this->sharedbinnamesmap.erase(this->currbin->name);
				}
				mainprogram->remove(this->currbin->path);
				this->bins.erase(std::find(this->bins.begin(), this->bins.end(), this->currbin));
				delete this->currbin; //delete textures in destructor
				if (this->currbin->pos == 0) make_currbin(1);
				else make_currbin(this->currbin->pos - 1);
				for (int i = 0; i < this->bins.size(); i++) {
					if (this->bins[i] == this->currbin) this->currbin->pos = i;
					this->bins[i]->box->vtxcoords->y1 = (i + 1) * -0.05f;
					this->bins[i]->box->upvtxtoscr();
				}
			}
			else if (k == 4) {
				// start renaming bin
				this->backupname = this->currbin->name;
				mainprogram->inputtext = this->currbin->name;
				mainprogram->cursorpos0 = mainprogram->inputtext.length();
				SDL_StartTextInput();
				mainprogram->renaming = EDIT_BINNAME;
			}
		}
		else {
			int k = mainprogram->handle_menu(mainprogram->bin2menu);  // rightclick on bin when there's only one (can't delete)
			if (k == 0) {
				if (!this->dragbin) {
					std::string name = "new bin";
					int count = 0;
					for (Bin* bin : this->bins) {
						if (name == bin->name) {
							count++;
							name = remove_version(name) + "_" + std::to_string(count);
							continue;
						}
					}
					new_bin(name);
					if (this->bins.size() > 20) this->binsscroll++;
				}
			}
			else if (k == 1) {
				if (!this->dragbin) {
					mainprogram->pathto = "OPENBIN";
					std::thread filereq(&Program::get_multinname, mainprogram, "Open file(s)", "application/ewocvj2-bin", std::filesystem::canonical(mainprogram->currbinsdir).generic_string());
					filereq.detach();
				}
			}
			else if (k == 2) {
				if (!this->dragbin) {
					mainprogram->pathto = "SAVEBIN";
					std::thread filereq(&Program::get_outname, mainprogram, "Open file(s)", "application/ewocvj2-bin", std::filesystem::canonical(mainprogram->currbinsdir).generic_string());
					filereq.detach();
				}
			}
			else if (k == 3) {
				// start renaming bin
				this->backupname = this->currbin->name;
				mainprogram->inputtext = this->currbin->name;
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
        mainprogram->recundo = true;
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
            // start move selected bin elements
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
			this->backupname = name;
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
			std::thread filereq(&Program::get_multinname, mainprogram, "Open file(s)", "", std::filesystem::canonical(mainprogram->currfilesdir).generic_string());
			filereq.detach();
 		}
		else if (binelmenuoptions[k] == BET_INSDECKA) {
			// insert deck A into bin
			mainprogram->paths.clear();
			mainmix->mousedeck = 0;
			std::string path = find_unused_filename("deckA", mainprogram->project->binsdir + this->currbin->name + "/", ".deck");
            if (mainprogram->prevmodus) {
                this->menubinel->tex = copy_tex(mainprogram->nodesmain->mixnodes[0][mainmix->mousedeck]->mixtex, 192, 108);
            }
            else {
                this->menubinel->tex = copy_tex(mainprogram->nodesmain->mixnodes[1][mainmix->mousedeck]->mixtex, 192, 108);
            }
			mainmix->save_deck(path, true, true);
			this->menubinel->type = ELEM_DECK;
			this->menubinel->path = path;
			this->menubinel->name = remove_extension(basename(this->menubinel->path));
			//this->menubinel->oldjpegpath = this->menubinel->jpegpath;
            std::string jpegpath = path + ".jpeg";
            save_thumb(jpegpath, this->menubinel->tex);
            this->menubinel->jpegpath = jpegpath;
            this->menubinel->absjpath = this->menubinel->jpegpath;
            this->menubinel->reljpath = std::filesystem::relative(this->menubinel->absjpath, mainprogram->project->binsdir).generic_string();
            this->menubinel->temp = true;
			// clean up: maybe too much cleared here, doesn't really matter
			this->menuactbinel = nullptr;
			this->prevbinel = nullptr;
		}
		else if (binelmenuoptions[k] == BET_INSDECKB) {
			// insert deck B into bin
			mainprogram->paths.clear();
			mainmix->mousedeck = 1;

            std::string path = find_unused_filename("deckB", mainprogram->project->binsdir + this->currbin->name + "/", ".deck");
            if (mainprogram->prevmodus) {
                this->menubinel->tex = copy_tex(mainprogram->nodesmain->mixnodes[0][mainmix->mousedeck]->mixtex);
            }
            else {
                this->menubinel->tex = copy_tex(mainprogram->nodesmain->mixnodes[1][mainmix->mousedeck]->mixtex);
            }
            mainmix->save_deck(path, true, true);
            this->menubinel->type = ELEM_DECK;
            this->menubinel->path = path;
            this->menubinel->name = remove_extension(basename(this->menubinel->path));
            //this->menubinel->oldjpegpath = this->menubinel->jpegpath;
            std::string jpegpath = path + ".jpeg";
            save_thumb(jpegpath, this->menubinel->tex);
            this->menubinel->jpegpath = jpegpath;
            this->menubinel->absjpath = this->menubinel->jpegpath;
            this->menubinel->reljpath = std::filesystem::relative(this->menubinel->absjpath, mainprogram->project->binsdir).generic_string();
            this->menubinel->temp = true;
            // clean up: maybe too much cleared here, doesn't really matter
            this->menuactbinel = nullptr;
            this->prevbinel = nullptr;
        }
		else if (binelmenuoptions[k] == BET_INSMIX) {
			// insert current mix into bin
			mainprogram->paths.clear();

            std::string path = find_unused_filename("mix", mainprogram->project->binsdir + this->currbin->name + "/", ".mix");
            if (mainprogram->prevmodus) {
                this->menubinel->tex = copy_tex(mainprogram->nodesmain->mixnodes[0][2]->mixtex, 192, 108);
            }
            else {
                this->menubinel->tex = copy_tex(mainprogram->nodesmain->mixnodes[1][2]->mixtex, 192, 108);
            }
            mainmix->save_mix(path, mainprogram->prevmodus, true);
            this->menubinel->type = ELEM_MIX;
            this->menubinel->path = path;
            this->menubinel->name = remove_extension(basename(this->menubinel->path));
            //this->menubinel->oldjpegpath = this->menubinel->jpegpath;
            std::string jpegpath = path + ".jpeg";
            save_thumb(jpegpath, this->menubinel->tex);
            this->menubinel->jpegpath = jpegpath;
            this->menubinel->absjpath = this->menubinel->jpegpath;
            this->menubinel->reljpath = std::filesystem::relative(this->menubinel->absjpath, mainprogram->project->binsdir).generic_string();
            this->menubinel->temp = true;
            // clean up: maybe too much cleared here, doesn't really matter
            this->menuactbinel = nullptr;
            this->prevbinel = nullptr;
		}
		else if (binelmenuoptions[k] == BET_LOADSHELFA) {
			// load hovered block into shelf A
			Shelf* shelf = mainprogram->shelves[0];
			for (int i = 0; i < 16; i++) {
				BinElement* binel = this->currbin->elements[this->mouseshelfnum / 3 * 48 + (this->mouseshelfnum % 3) * 4 + (i / 4) * 12 + (i % 4)];
				ShelfElement* elem = shelf->elements[i];
                elem->path = binel->path;
                elem->name = binel->name;
				elem->jpegpath = binel->jpegpath;
				elem->type = binel->type;
                GLuint butex = elem->tex;
                elem->tex = copy_tex(binel->tex, 192, 108);
                if (butex != -1) glDeleteTextures(1, &butex);
			}
		}
		else if (binelmenuoptions[k] == BET_LOADSHELFB) {
			// load hovered block into shelf B
			Shelf* shelf = mainprogram->shelves[1];
			for (int i = 0; i < 16; i++) {
				BinElement* binel = this->currbin->elements[this->mouseshelfnum / 3 * 48 + (this->mouseshelfnum % 3) * 4 + (i / 4) * 12 + (i % 4)];
				ShelfElement* elem = shelf->elements[i];
				elem->path = binel->path;
                elem->name = binel->name;
				elem->jpegpath = binel->jpegpath;
				elem->type = binel->type;
				GLuint butex = elem->tex;
				elem->tex = copy_tex(binel->tex, 192, 108);
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
                    if (binel->encoding) continue;
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
        if (mainprogram->pathto == "") {
            mainprogram->recundo = true;
        }
	}


	if (mainprogram->menuactivation && !this->inputtexes.size() && !lay->vidmoving && this->movingtex == -1) {
		// activate binslist or bin menu
		if (this->binsbox->in()) {
		    mainprogram->binmenu->state = 2;
		    mainprogram->binmenu->menux = mainprogram->mx;
		    mainprogram->binmenu->menuy = mainprogram->my;
			mainprogram->bin2menu->state = 2;
			mainprogram->bin2menu->menux = mainprogram->mx;
			mainprogram->bin2menu->menuy = mainprogram->my;
		}
		else {
			mainprogram->binelmenu->state = 2;
			mainprogram->binelmenu->menux = mainprogram->mx;
			mainprogram->binelmenu->menuy = mainprogram->my;
			mainprogram->leftmousedown = false;
		}
		mainprogram->menuondisplay = true;
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
				Boxx* box = this->elemboxes[i * 12 + j];
				box->upvtxtoscr();
				BinElement* binel = this->currbin->elements[i * 12 + j];
                if (binel->encoding && binel->encthreads == 0 && (binel->type == ELEM_DECK || binel->type == ELEM_MIX)) {
					binel->encoding = false;
					rename(remove_extension(binel->path) + ".temp", binel->path);
				}
				if ((box->in() || mainprogram->rightmouse || binel == menuactbinel) && !this->openfilesbin && !binel->encoding) {
					if (draw) {
						inbinel = true;
                        if (mainprogram->dropfiles.size()) {
                            // SDL drag'n'drop
                            mainprogram->path = mainprogram->dropfiles[0];
                            for (char *df : mainprogram->dropfiles) {
                                mainprogram->paths.push_back(df);
                            }
                            binsmain->menuactbinel = binel;
                            mainprogram->counting = 0;
                            mainprogram->pathto = "OPENFILESBIN";
                        }
						// don't preview when encoding
						if (this->currbin->encthreads) continue;
						if (this->menubinel) {
							if (this->menubinel->encthreads) continue;
						}
						if (!binel->encwaiting && !binel->encoding && binel->name != "" && !this->inputtexes.size() && !lay->vidmoving && !this->insertshelf && !this->selboxing && !mainprogram->dragbinel) {
							if (this->previewbinel != binel) {
								// reset when new element hovered
								this->previewimage = "";
								this->previewbinel = nullptr;
							}
							mainprogram->frontbatch = true;
							float yfac = (mainprogram->ow[0] / mainprogram->oh[0]) / (1920.0f / 1080.0f);
							if ((this->previewimage != "" || binel->type == ELEM_IMAGE) && !this->binpreview) {
								// do first entry preview preperation/visualisation when image hovered
								this->binpreview = true;  // just entering preview, or already done preparation (different if clauses)
								if (mainprogram->prelay) {
                                    mainprogram->prelay->close();
									// close old preview layer
								}
								mainprogram->prelay = new Layer(false);
								mainprogram->prelay->dummy = true;
                                mainprogram->prelay->transfered = true;
								if (this->previewimage != "") mainprogram->prelay->open_image(this->previewimage);
								else mainprogram->prelay->open_image(binel->path);
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
                                glBindTexture(GL_TEXTURE_2D, mainprogram->prelay->texture);
                                if (bpp == 3) {
									glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, mode, GL_UNSIGNED_BYTE, ilGetData());
								}
								else if (bpp == 4) {
									glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, mode, GL_UNSIGNED_BYTE, ilGetData());
								}
								auto svec = mainprogram->prelay->get_inside_offsets();
								draw_box(red, black, 0.52f + 0.4f * svec[0], 0.5f + 0.4f * svec[1] * yfac, 0.4f - 0.8f * svec[0], (0.4f - 0.8f * svec[1]) * yfac, mainprogram->prelay->texture);
                                render_text("MOUSEWHEEL searches through file", white, 0.62f, 0.45f, 0.0005f, 0.0008f);
								render_text("IMAGE", white, box->vtxcoords->x1 + 0.0075f, box->vtxcoords->y1 + box->vtxcoords->h - 0.0225f, 0.0005f, 0.0008f);
								render_text(std::to_string(w) + "x" + std::to_string(h), white, box->vtxcoords->x1 + 0.0075f, box->vtxcoords->y1 + box->vtxcoords->h - 0.0675f, 0.0005f, 0.0008f);
							}
							else if (this->previewimage != "" || binel->type == ELEM_IMAGE) {
								// do second entry preview visualisation when image hovered
								if (mainprogram->mousewheel) {
									// mousewheel leaps through animated gifs
									mainprogram->prelay->frame += mainprogram->mousewheel * (mainprogram->prelay->numf / 32.0f);
								}

                                mainprogram->mousewheel = 0.0f;
                                if (mainprogram->prelay->frame < 0.0f) {
                                    mainprogram->prelay->frame = mainprogram->prelay->numf - 1.0f;
                                }
                                else if (mainprogram->prelay->frame > mainprogram->prelay->numf - 1) {
                                    mainprogram->prelay->frame = 0.0f;
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
								auto svec = mainprogram->prelay->get_inside_offsets();
								draw_box(red, black, 0.52f + 0.4f * svec[0], 0.5f + 0.4f * svec[1] * yfac, 0.4f - 0.8f * svec[0], (0.4f - 0.8f * svec[1]) * yfac, mainprogram->prelay->texture);
                                render_text("MOUSEWHEEL searches through file", white, 0.62f, 0.45f, 0.0005f, 0.0008f);
								render_text("IMAGE", white, box->vtxcoords->x1 + 0.0075f, box->vtxcoords->y1 + box->vtxcoords->h - 0.0225f, 0.0005f, 0.0008f);
								render_text(std::to_string(w) + "x" + std::to_string(h), white, box->vtxcoords->x1 + 0.0075f, box->vtxcoords->y1 + box->vtxcoords->h - 0.0675f, 0.0005f, 0.0008f);
							}
							else if ((binel->type == ELEM_LAYER) && !this->binpreview) {
								// do first entry preview preperation/visualisation when layer file hovered
								if (binel->name != "") {
                                    if (mainprogram->prelay) {
                                        mainprogram->prelay->close();
                                        // close old preview layer
                                    }
									this->binpreview = true;
									Layer *prelay = new Layer(false);
                                    prelay->dummy = true;
                                    prelay->deck = 0;
                                    prelay->pos = 0;
                                    prelay->keepeffbut->value = 0;
                                    mainprogram->prelay = mainmix->open_layerfile(binel->path, prelay, false, false);
                                    prelay->close();
                                    mainprogram->prelay->dummy = true;
                                    mainprogram->prelay->pos = 0;
                                    mainprogram->prelay->deck = 1;
                                    mainprogram->prelay->blendnode = mainprogram->nodesmain->currpage->add_blendnode(MIXING, !mainprogram->prevmodus);
                                    mainprogram->prelay->node->layer = mainprogram->prelay;
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
                                    mainprogram->prelay->initialize(lay->decresult->width, lay->decresult->height);
									glActiveTexture(GL_TEXTURE0);
									glBindTexture(GL_TEXTURE_2D, mainprogram->prelay->texture);
                                    if (mainprogram->prelay->vidformat == 188 || mainprogram->prelay->vidformat == 187) {
                                        if (mainprogram->prelay->decresult->compression == 187) {
                                            glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mainprogram->prelay->decresult->width, mainprogram->prelay->decresult->height, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, mainprogram->prelay->decresult->size, mainprogram->prelay->decresult->data);
                                        }
                                        else if (mainprogram->prelay->decresult->compression == 190) {
                                            glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mainprogram->prelay->decresult->width, mainprogram->prelay->decresult->height, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, mainprogram->prelay->decresult->size, mainprogram->prelay->decresult->data);
                                        }
                                    }
                                    else {
                                        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mainprogram->prelay->decresult->width, mainprogram->prelay->decresult->height, GL_BGRA, GL_UNSIGNED_BYTE, mainprogram->prelay->decresult->data);
                                    }
                                    std::vector<Layer*> layers;
                                    layers.push_back(mainprogram->prelay);
                                    mainprogram->prelay->layers = &layers;
                                    mainmix->reconnect_all(layers);
									// calculate effects
                                    mainprogram->directmode = true;
									onestepfrom(0, mainprogram->prelay->node, nullptr, -1, -1);
                                    mainprogram->directmode = false;
									auto svec = mainprogram->prelay->get_inside_offsets();
									if (mainprogram->prelay->effects[0].size()) {
										draw_box(red, black, 0.52f + 0.4f * svec[0], 0.5f + 0.4f * svec[1] * yfac, 0.4f - 0.8f * svec[0], (0.4f - 0.8f * svec[1]) * yfac, mainprogram->prelay->effects[0][mainprogram->prelay->effects[0].size() - 1]->fbotex);
									}
									else {
									    draw_box(red, black, 0.52f + 0.4f * svec[0], 0.5f + 0.4f * svec[1] * yfac, 0.4f - 0.8f * svec[0], (0.4f - 0.8f * svec[1]) * yfac, mainprogram->prelay->fbotex);
									}
 									if (!binel->encoding) {
										// show video format
					                   render_text("MOUSEWHEEL searches through file", white, 0.62f, 0.45f, 0.0005f, 0.0008f);
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
								auto svec = mainprogram->prelay->get_inside_offsets();
								draw_box(red, black, 0.52f + 0.4f * svec[0], 0.5f + 0.4f * svec[1] * yfac, 0.4f - 0.8f * svec[0], (0.4f - 0.8f * svec[1]) * yfac, -1);
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
                                    mainprogram->prelay->prevframe = mainprogram->prelay->frame - 1.0f;
									mainprogram->prelay->node->calc = true;
									mainprogram->prelay->node->walked = false;
									for (int k = 0; k < mainprogram->prelay->effects[0].size(); k++) {
										mainprogram->prelay->effects[0][k]->node->calc = true;
										mainprogram->prelay->effects[0][k]->node->walked = false;
									}
                                    mainprogram->prelay->decresult->newdata = false;
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
                                            glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mainprogram->prelay->decresult->width, mainprogram->prelay->decresult->height, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, mainprogram->prelay->decresult->size, mainprogram->prelay->decresult->data);
                                        }
                                        else if (mainprogram->prelay->decresult->compression == 190) {
                                            glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mainprogram->prelay->decresult->width, mainprogram->prelay->decresult->height, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, mainprogram->prelay->decresult->size, mainprogram->prelay->decresult->data);
                                        }
                                    }
                                    else {
                                        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mainprogram->prelay->decresult->width, mainprogram->prelay->decresult->height, GL_BGRA, GL_UNSIGNED_BYTE, mainprogram->prelay->decresult->data);
                                    }

                                    // calculate effects
                                    mainprogram->directmode = true;
									onestepfrom(0, mainprogram->prelay->node, nullptr, -1, -1);
                                    mainprogram->directmode = false;
									auto svec = mainprogram->prelay->get_inside_offsets();
									if (mainprogram->prelay->effects[0].size()) {
										draw_box(red, black, 0.52f + 0.4f * svec[0], 0.5f + 0.4f * svec[1] * yfac, 0.4f - 0.8f * svec[0], (0.4f - 0.8f * svec[1]) * yfac, mainprogram->prelay->effects[0][mainprogram->prelay->effects[0].size() - 1]->fbotex);
									}
									else {
										draw_box(red, black, 0.52f + 0.4f * svec[0], 0.5f + 0.4f * svec[1] * yfac, 0.4f - 0.8f * svec[0], (0.4f - 0.8f * svec[1]) * yfac, mainprogram->prelay->fbotex);
									}
								}
								else {
									// show old image if not mousewheeled through file
									auto svec = mainprogram->prelay->get_inside_offsets();
									if (mainprogram->prelay->effects[0].size()) {
										draw_box(red, black, 0.52f + 0.4f * svec[0], 0.5f + 0.4f * svec[1] * yfac, 0.4f - 0.8f * svec[0], (0.4f - 0.8f * svec[1]) * yfac, mainprogram->prelay->effects[0][mainprogram->prelay->effects[0].size() - 1]->fbotex);
									}
									else {
										draw_box(red, black, 0.52f + 0.4f * svec[0], 0.5f + 0.4f * svec[1] * yfac, 0.4f - 0.8f * svec[0], (0.4f - 0.8f * svec[1]) * yfac, mainprogram->prelay->fbotex);
									}
								}
								if (!binel->encoding && remove_extension(basename(binel->path)) != "") {
									// show video format
                                    render_text("MOUSEWHEEL searches through file", white, 0.62f, 0.45f, 0.0005f, 0.0008f);
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
                                        mainprogram->prelay->close();
                                        // close old preview layer
                                    }
									this->binpreview = true;
									mainprogram->prelay = new Layer(true);
                                    mainprogram->prelay->dummy = true;
                                    mainprogram->prelay->transfered = true;
                                    mainprogram->prelay->open_video(0, binel->path, true);
                                    // wait for video open
									std::unique_lock<std::mutex> olock(mainprogram->prelay->endopenlock);
									mainprogram->prelay->endopenvar.wait(olock, [&] {return mainprogram->prelay->opened; });
									mainprogram->prelay->opened = false;
									olock.unlock();
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
                                    mainprogram->prelay->initialize(lay->decresult->width, lay->decresult->height);
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
									auto svec = mainprogram->prelay->get_inside_offsets();
									draw_box(red, black, 0.52f + 0.4f * svec[0], 0.5f + 0.4f * svec[1] * yfac, 0.4f - 0.8f * svec[0], (0.4f - 0.8f * svec[1]) * yfac, this->binelpreviewtex);
									if (!binel->encoding) {
										// show video format
                                        render_text("MOUSEWHEEL searches through file", white, 0.62f, 0.45f, 0.0005f, 0.0008f);
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
								auto svec = mainprogram->prelay->get_inside_offsets();
								draw_box(red, black, 0.52f + 0.4f * svec[0], 0.5f + 0.4f * svec[1] * yfac, 0.4f - 0.8f * svec[0], (0.4f - 0.8f * svec[1]) * yfac, -1);
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
										mainprogram->prelay->prevframe = mainprogram->prelay->frame - 1.0f;
                                        mainprogram->prelay->decresult->newdata = false;
                                        mainprogram->prelay->ready = true;
										while (mainprogram->prelay->ready) {
											// start decode frame
											mainprogram->prelay->startdecode.notify_all();
										}
										// wait for decode finished
										std::unique_lock<std::mutex> lock(mainprogram->prelay->enddecodelock);
										mainprogram->prelay->enddecodevar.wait(lock, [&] {return mainprogram->prelay->processed; });
										lock.unlock();
                                        mainprogram->prelay->processed = false;
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
										auto svec = mainprogram->prelay->get_inside_offsets();
										draw_box(red, black, 0.52f + 0.4f * svec[0], 0.5f + 0.4f * svec[1] * yfac, 0.4f - 0.8f * svec[0], (0.4f - 0.8f * svec[1]) * yfac, this->binelpreviewtex);
									}
									else {
										auto svec = mainprogram->prelay->get_inside_offsets();
										draw_box(red, black, 0.52f + 0.4f * svec[0], 0.5f + 0.4f * svec[1] * yfac, 0.4f - 0.8f * svec[0], (0.4f - 0.8f * svec[1]) * yfac, this->binelpreviewtex);
									}
								}
								else {
									auto svec = mainprogram->prelay->get_inside_offsets();
									draw_box(red, black, 0.52f + 0.4f * svec[0], 0.5f + 0.4f * svec[1] * yfac, 0.4f - 0.8f * svec[0], (0.4f - 0.8f * svec[1]) * yfac, mainprogram->prelay->texture);
								}
								if (!binel->encoding && remove_extension(basename(binel->path)) != "") {
									// show video format
                                    render_text("MOUSEWHEEL searches through file", white, 0.62f, 0.45f, 0.0005f, 0.0008f);
									if (mainprogram->prelay->vidformat == 188 || mainprogram->prelay->vidformat == 187) {
										render_text("HAP", white, box->vtxcoords->x1 + 0.0075f, box->vtxcoords->y1 + box->vtxcoords->h - 0.0225f, 0.0005f, 0.0008f);
										render_text(std::to_string(mainprogram->prelay->video_dec_ctx->width) + "x" + std::to_string(mainprogram->prelay->video_dec_ctx->height), white, box->vtxcoords->x1 + 0.0075f, box->vtxcoords->y1 + box->vtxcoords->h - 0.0675f, 0.0005f, 0.0008f);
									}
									else {
										if (mainprogram->prelay->type == ELEM_FILE) {
											render_text("CPU", white, box->vtxcoords->x1 + 0.0075f, box->vtxcoords->y1 + box->vtxcoords->h - 0.0225f, 0.0005f, 0.0008f);
											render_text(std::to_string(mainprogram->prelay->video_dec_ctx->width) + "x" + std::to_string(mainprogram->prelay->video_dec_ctx->height), white, box->vtxcoords->x1 + 0.0075f, box->vtxcoords->y1 + box->vtxcoords->h - 0.0675f, 0.0005f, 0.0008);
										}
									}
								}
							}
							mainprogram->frontbatch = false;
						}

                        /*if (mainprogram->prelay) {
                            mainprogram->prelay->close();
                        }*/

						if (binel->name != "") {
                            if (binel->select && mainprogram->leftmousedown && this->movebinels.empty()) {
                                // start dragging selection around (move)
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
                                bool dummy = false;
                            }
							if (!this->inputtexes.size()) {
                                if (mainprogram->leftmousedown && !mainprogram->dragbinel && !mainprogram->ctrl && !mainprogram->shift) {
                                    // dragging single bin element
                                    if (binel->name != "" && !binel->encoding && !binel->encwaiting) {
                                        mainprogram->dragbinel = new BinElement;
                                        mainprogram->dragbinel->tex = binel->tex;
                                        mainprogram->dragbinel->path = binel->path;
                                        mainprogram->dragbinel->name = binel->name;
                                        mainprogram->dragbinel->type = binel->type;
                                        // start drag
                                        this->movingtex = binel->tex;
                                        this->movingbinel = binel;
                                        this->currbinel = binel;
                                        mainprogram->shelves[0]->prevnum = -1;
                                        mainprogram->shelves[1]->prevnum = -1;
                                    }
									mainprogram->leftmousedown = false;
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

                    if (binel != this->currbinel && mainprogram->binsscreen) {
						if (this->currbinel) this->binpreview = false;
                        bool cond1 = false;
                        bool cond2 = false;
						if (mainprogram->dragbinel) {
                            cond1 = (mainprogram->shelfdragelem);
                            cond2 = (mainprogram->draggingrec);
                        }
						if (lay->vidmoving || cond1 || cond2) {
							// when dragging layer/mix/deck in from mix view
							if (this->currbinel) {
								//reset old currbinel
								this->currbinel->tex = this->currbinel->oldtex;
								// set new layer drag textures in this bin element
								binel->oldtex = binel->tex;
                                binel->tex = mainprogram->dragbinel->tex;
                                binel->tex = mainprogram->dragbinel->tex;
                            }
							else {
								// set new layer drag textures in this bin element
								binel->oldtex = binel->tex;
								binel->tex = mainprogram->dragbinel->tex;
							}
						}

						if (this->inputtexes.size() && !this->menuactbinel && !mainprogram->leftmouse) {
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
                                    else {
                                        continue;
                                    }
                                    if (!dirbinel->select || this->movebinels.empty()) {
                                        if (this->inputtexes[k] != -1) {
                                            dirbinel->select = dirbinel->oldselect;
                                            dirbinel->tex = dirbinel->oldtex;
                                        }
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
                                else {
                                    continue;
                                }
                                if (std::find(this->inputtexes.begin(), this->inputtexes.end(), dirbinel->tex) == this->inputtexes.end()) {
                                    if (this->inputtexes[k] != -1) {
                                        dirbinel->oldselect = dirbinel->select;
                                        dirbinel->select = false;
                                        dirbinel->oldtex = dirbinel->tex;
                                        dirbinel->tex = this->inputtexes[k];
                                    }
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
						// set values of elements of opened files
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
                            if (epos >= 0 and epos < 144) {
                                BinElement *dirbinel = this->currbin->elements[epos];
                                if (dirbinel->tex != -1) {
                                    //glDeleteTextures(1, &dirbinel->tex);
                                }
                                //mainprogram->delete_text(dirbinel->name.substr(0, 20));
                                dirbinel->tex = this->inputtexes[k];
                                dirbinel->type = this->inputtypes[k];
                                dirbinel->path = this->addpaths[k];
								dirbinel->name = remove_extension(basename(dirbinel->path));
                                dirbinel->oldjpegpath = dirbinel->jpegpath;
                                dirbinel->jpegpath = this->inputjpegpaths[k];
                                dirbinel->absjpath = dirbinel->jpegpath;
                                if (dirbinel->absjpath != "") {
                                    dirbinel->reljpath = std::filesystem::relative(dirbinel->absjpath,mainprogram->project->binsdir).generic_string();
                                }
                            }
						}
						// clean up
						this->inputtexes.clear();
						this->inputtypes.clear();
						this->menuactbinel = nullptr;
						this->addpaths.clear();
                        this->inputjpegpaths.clear();
                        mainprogram->recundo = true;
                    }


                    if (this->currbinel && this->movingtex != -1) {
                        // intermediate swapping when moving bin elements in bin
						if (binel != this->currbinel) {
							if (this->movingtex != this->movingbinel->tex) {
                                this->currbinel->tex = this->movingbinel->tex;
                            }
							this->movingbinel->tex = binel->tex;
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
                        this->delbinels[k]->erase();
					}
					this->delbinels.clear();
					mainprogram->del = false;
				}
			}
		}

		if (!inbinel) this->binpreview = false;

        if (inbinel && !mainprogram->rightmouse && (lay->vidmoving || mainprogram->shelfdragelem || mainprogram->draggingrec) &&
            mainprogram->lmover) {
            // confirm layer dragging from main view and set influenced bin element to the right values
            this->currbinel->type = mainprogram->dragbinel->type;
            this->currbinel->path = mainprogram->dragbinel->path;
            this->currbinel->tex = copy_tex(this->currbinel->tex);
            if (this->currbinel->type == ELEM_DECK || this->currbinel->type == ELEM_MIX) {
                if (exists(this->currbinel->path) && mainprogram->draglay == nullptr) {
                    // a duplicate of the content will be made, if the content file already exists
                    std::string ext = this->currbinel->path.substr(this->currbinel->path.rfind("."));
                    std::string newpath = find_unused_filename(remove_extension(basename(this->currbinel->path)),
                                                               mainprogram->project->binsdir, ext);
                    copy_file(this->currbinel->path, newpath);
                    this->currbinel->path = newpath;
                }
            }
            this->currbinel->name = mainprogram->dragbinel->name;
            if (this->currbinel->type == ELEM_LAYER) {
            	std::string p1;
            	if (lay->vidmoving) p1 = lay->filename;
            	else p1 = mainprogram->shelfdragelem->path;
            	this->currbinel->path = find_unused_filename(basename(p1),
															 mainprogram->project->binsdir + this->currbin->name +
															 "/", ""
																  ".layer");
                if (mainprogram->dragbinel->name == "") {
                    this->currbinel->name = remove_extension(basename(this->currbinel->path));
                }
            	this->currbinel->temp = true;
                mainmix->save_layerfile(this->currbinel->path, lay, 1, 0);
            }
            if (mainprogram->dragbinel->name == "") {
                this->currbinel->name = remove_extension(basename(this->currbinel->path));
            }
            this->currbinel->full = true;
            this->currbinel = nullptr;
            enddrag();
            lay->vidmoving = false;
            mainmix->moving = false;
        }
        if (!draw) {
            if (mainprogram->lmover) {
                mainprogram->recundo = false;
            }
        }
        if ((mainprogram->lmover || mainprogram->rightmouse) && mainprogram->dragbinel) {
            //when dropping on grey area
            // or rightmouse canceling from mix view
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
            } else if (this->movingtex != -1) {
            	// drop: when element dragging inside bin
            	bool found = false;
            	BinElement *foundbinel;
            	for (int j = 0; j < 12; j++) {
            		for (int i = 0; i < 12; i++) {
            			Boxx *box = this->elemboxes[i * 12 + j];
            			foundbinel = this->currbin->elements[i * 12 + j];
            			if (box->in() && this->currbinel == foundbinel) {
            				found = true;
            				break;
            			}
            		}
            	}
            	if (!found) {
            		this->currbinel->tex = this->movingbinel->tex;
            		this->movingbinel->tex = this->movingtex;
            		this->currbinel = nullptr;
            		this->movingbinel = nullptr;
            		this->movingtex = -1;
            	} else {
            		std::swap(this->currbinel->type, this->movingbinel->type);
            		std::swap(this->currbinel->path, this->movingbinel->path);
            		std::swap(this->currbinel->name, this->movingbinel->name);
                    std::swap(this->currbinel->jpegpath, this->movingbinel->jpegpath);  // one way?
                    std::swap(this->currbinel->absjpath, this->movingbinel->absjpath);  // one way?
                    std::swap(this->currbinel->reljpath, this->movingbinel->reljpath);  // one way?
            		this->currbinel = nullptr;
            		this->movingbinel = nullptr;
            		this->movingtex = -1;
            	}
            }
            enddrag();
        }

        if (this->movebinels.size() && mainprogram->leftmouse) {
            // confirm elements move, set elements and clean up
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
                    dirbinel->type = this->inputtypes[k];
                    dirbinel->path = this->addpaths[k];
                    dirbinel->tex = this->inputtexes[k];
                    dirbinel->name = remove_extension(basename(dirbinel->path));
                    dirbinel->oldjpegpath = dirbinel->jpegpath;
                    dirbinel->jpegpath = this->inputjpegpaths[k];
                    dirbinel->absjpath = dirbinel->jpegpath;
                    dirbinel->reljpath = std::filesystem::relative(dirbinel->absjpath, mainprogram->project->binsdir).generic_string();
                    int pos =
                            std::find(this->movebinels.begin(), this->movebinels.end(), dirbinel) -
                            this->movebinels.begin();
                    if (pos < this->movebinels.size()) {
                        this->movebinels.erase(this->movebinels.begin() + pos);
                    }
                }
            }

            for (int k = 0; k < this->movebinels.size(); k++) {
                if (this->movebinels[k]->tex != -1) {
                    this->movebinels[k]->erase(false);
                }
            }
            for (auto elem : this->currbin->elements) {
                elem->select = false;
            }
            this->movebinels.clear();
            // clean up
            this->inputtexes.clear();
            if (this->currbinel == this->menuactbinel) this->menuactbinel = nullptr;
            this->currbinel = nullptr;
            this->prevbinel = nullptr;
            this->inputtypes.clear();
            this->addpaths.clear();
            this->inputjpegpaths.clear();
            mainprogram->leftmouse = false;
        }
	}

	// load one file into bin each loop, at end to allow drawing ordering dialog on top
	if (this->openfilesbin) {
		open_files_bin();
	}
}




void BinsMain::send_shared_bins() {
    auto put_in_buffer = [](const char* str, char* walk, char* buffer_end) -> char* {
        // buffer utility with bounds checking
        size_t len = strlen(str);
        if (walk + len + 1 > buffer_end) {
            return nullptr;  // Buffer overflow would occur
        }
        for (size_t i = 0; i < len; i++) {
            *walk++ = str[i];
        }
        *walk++ = '\0';
        return walk;
    };

    // Helper to calculate string length including null terminator
    auto str_len = [](const std::string& s) -> size_t {
        return s.length() + 1;
    };

    // Calculate correct message length including the length field itself
    // This handles the edge case where msglen goes from 9999->10000 etc.
    auto calculate_message_length = [&str_len](size_t payload_size) -> size_t {
        size_t msglen = payload_size;
        size_t msglen_str_len = str_len(std::to_string(msglen));

        // Check if adding the length field changes the number of digits
        size_t new_msglen = payload_size + msglen_str_len;
        size_t new_msglen_str_len = str_len(std::to_string(new_msglen));

        // Iterate until stable (usually 1-2 iterations max)
        while (new_msglen_str_len != msglen_str_len) {
            msglen_str_len = new_msglen_str_len;
            new_msglen = payload_size + msglen_str_len;
            new_msglen_str_len = str_len(std::to_string(new_msglen));
        }

        return new_msglen;
    };

    for (auto name : this->currbin->sendtonames) {
        int sock = -1;  // Initialize to invalid socket
        bool brk = false;

        if (mainprogram->server) {
            // Don't send bin back to the server itself
            if (name == mainprogram->seatname) continue;

            // Don't echo the message back to whoever just sent it
            if (!this->currbin->last_message_sender.empty() && name == this->currbin->last_message_sender) {
                std::cout << "DEBUG: Skipping echo to message sender '" << name << "' for bin '" << this->currbin->name << "'" << std::endl;
                continue;
            }
            for (auto &elem: mainprogram->connmap) {
                if (elem.first == name) {
                    sock = elem.second;  // Get the actual socket for this name
                    brk = true;
                    break;
                }
            }
            if (!brk) {
                // Socket not found for this name, skip
                continue;
            }
        }
        else {
            sock = mainprogram->sock;
        }

        // Only send if currbin is shared AND all textures have been received
        if (this->currbin->shared && this->currbin->pending_textures == 0 && this->currbin->pending_files == 0) {
            std::vector<std::string> tempfiles;
            std::vector<std::string> temptexes;
            std::vector<std::string> tempnames;
            for (auto elem : this->currbin->elements) {
                temptexes.push_back(elem->absjpath);
                tempnames.push_back(elem->name);
                if (elem->type == ELEM_LAYER || elem->type == ELEM_DECK || elem->type == ELEM_MIX) {
                    tempfiles.push_back(elem->path);
                }
                else {
                    tempfiles.push_back("");
                }
            }

            // Check if names or textures changed
            bool names_changed = (tempnames != this->currbin->prevnames);
            bool textures_changed = (temptexes != this->currbin->prevtexes);
            bool files_changed = (tempfiles != this->currbin->prevfiles);
            bool send_bin_metadata = names_changed || this->currbin->prevnames.empty();
            bool send_textures = textures_changed || this->currbin->prevtexes.empty();
            bool send_files = files_changed || this->currbin->prevfiles.empty();

            // Skip if nothing changed
            if (!send_bin_metadata && !send_textures && !send_files) {
                continue;
            }

            std::cout << "DEBUG: Sending bin '" << this->currbin->name << "' to '" << name
                     << "' (names_changed=" << names_changed << ", textures_changed=" << textures_changed << ", files_changed=" << files_changed << ")" << std::endl;

            // First, count how many textures will be sent (only if textures changed)
            int texture_count = 0;
            if (send_textures) {
                for (int i = 0; i < 12; i++) {
                    for (int j = 0; j < 12; j++) {
                        bool cond = false;
                        if (this->currbin->prevtexes.empty()) {
                            cond = true;
                        }
                        else {
                            cond = (this->currbin->prevtexes[i * 12 + j] != temptexes[i * 12 + j]);
                        }
                        if (cond) {
                            texture_count++;
                        }
                    }
                }
            }

            // Then, count how many LAYER/DECK/MIX files will be sent (only if files changed)
            int file_count = 0;
            if (send_files) {
                for (int i = 0; i < 12; i++) {
                    for (int j = 0; j < 12; j++) {
                        BinElement *binel = this->currbin->elements[i * 12 + j];
                        bool cond = false;
                        if (this->currbin->prevtexes.empty()) {
                            cond = true;
                        }
                        else {
                            cond = (this->currbin->prevfiles[i * 12 + j] != tempfiles[i * 12 + j]);
                        }
                        if (binel->type == ELEM_LAYER || binel->type == ELEM_DECK || binel->type == ELEM_MIX) {
                            if (cond) {
                                file_count++;
                            }
                        }
                    }
                }
            }

			// get sharename
			std::string currbinsharename = this->currbin->name;
			if (this->sharedbinnamesmap.count(this->currbin->name)) {
				currbinsharename = this->sharedbinnamesmap[this->currbin->name];
			}

            // Only send BIN_SENT if names or paths changed
            if (send_bin_metadata) {
                char buf[148480] = {0};
                char *buf_end = buf + sizeof(buf);
                char *walk = buf;

                walk = put_in_buffer(mainprogram->seatname.c_str(), walk, buf_end);
                if (!walk) continue;  // Buffer overflow

				walk = put_in_buffer((currbinsharename).c_str(), walk, buf_end);
				if (!walk) continue;

				walk = put_in_buffer(this->currbin->idstr.c_str(), walk, buf_end);
				if (!walk) continue;

				// Send texture count so receiver knows how many TEX_SENT messages to expect
                walk = put_in_buffer(std::to_string(texture_count).c_str(), walk, buf_end);
                if (!walk) continue;

                // Send file count so receiver knows how many FILE_SENT messages to expect
                walk = put_in_buffer(std::to_string(file_count).c_str(), walk, buf_end);
                if (!walk) continue;

                for (int i = 0; i < 12; i++) {
                    for (int j = 0; j < 12; j++) {
                        BinElement *binel = this->currbin->elements[i * 12 + j];
                        walk = put_in_buffer(binel->name.c_str(), walk, buf_end);
                        if (!walk) break;
                        walk = put_in_buffer(binel->path.c_str(), walk, buf_end);
                        if (!walk) break;
                    }
                    if (!walk) break;
                }
                if (!walk) continue;  // Buffer overflow occurred

                // Calculate actual message size
                size_t msg_size = walk - buf;

                char buf2[148495] = {0};
                char *buf2_end = buf2 + sizeof(buf2);
                char *walk2 = buf2;

                walk2 = put_in_buffer("BIN_SENT", walk2, buf2_end);
                if (!walk2) continue;

                walk2 = put_in_buffer(std::to_string(msg_size).c_str(), walk2, buf2_end);
                if (!walk2) continue;

                // Append buf to buf2
                if (walk2 + msg_size > buf2_end) continue;  // Check bounds
                memcpy(walk2, buf, msg_size);
                walk2 += msg_size;

                int sent = mainprogram->bl_send(sock, buf2, walk2 - buf2, 0);
                if (sent != walk2 - buf2) {
                    std::cout << "DEBUG: BIN_SENT send failed - sent " << sent << " of " << (walk2 - buf2) << " bytes" << std::endl;
                }
                std::cout << "DEBUG: Sent BIN_SENT for '" << this->currbin->name << "' with " << texture_count << " textures to follow" << std::endl;
            }

            // Send LAYER/DECK/MIX files for each BinElement
            if (send_files) {
                for (int i = 0; i < 12; i++) {
                    for (int j = 0; j < 12; j++) {
                        BinElement *binel = this->currbin->elements[i * 12 + j];

                        if (binel->type != ELEM_LAYER && binel->type != ELEM_DECK && binel->type != ELEM_MIX) {
                            continue;
                        }

                        bool cond = false;
                        if (this->currbin->prevfiles.empty()) {
                            cond = true;
                        }
                        else {
                            cond = (this->currbin->prevfiles[i * 12 + j] != tempfiles[i * 12 + j]);
                        }

                        if (cond) {
                            // Try to open file
                            std::ifstream infile;
                            size_t filesize = 0;

                            infile.open(binel->path, std::ios::binary | std::ios::ate);
                            if (infile.is_open()) {
                                filesize = infile.tellg();
                                infile.seekg(0, std::ios::beg);
                            }

                            std::string suffix;
                            if (binel->type == ELEM_LAYER) {
                                suffix = ".layer";
                            }
                            else if (binel->type == ELEM_DECK) {
                                suffix = ".deck";
                            }
                            else if (binel->type == ELEM_MIX) {
                                suffix = ".mix";
                            }

                            // Send actual LAYER/DECK/MIX file
                            // Calculate payload size FIRST (without building buffer yet)
                            size_t payload_base =
                                    //str_len(mainprogram->seatname) +
                                    str_len(currbinsharename) +
									str_len(this->currbin->idstr) +
									str_len(std::to_string(i * 12 + j)) +
                                    str_len(suffix) +
                                    str_len(std::to_string(filesize)) +
                                    filesize;

                            // Calculate total message length including msglen field itself
                            size_t total_msglen = calculate_message_length(payload_base);
                            std::string msglen_str = std::to_string(total_msglen);

                            // Now build the actual buffer with correct sizes
                            size_t header_estimate = 256;
                            size_t buffer_size = header_estimate + filesize;
                            std::unique_ptr<char[]> completemsg(new char[buffer_size]());
                            char *completemsg_end = completemsg.get() + buffer_size;
                            char *filewalk = completemsg.get();

                            filewalk = put_in_buffer("FILE_SENT", filewalk, completemsg_end);
                            if (!filewalk) {
                                infile.close();
                                continue;
                            }

                            filewalk = put_in_buffer(msglen_str.c_str(), filewalk, completemsg_end);
                            if (!filewalk) {
                                infile.close();
                                continue;
                            }

                            filewalk = put_in_buffer(mainprogram->seatname.c_str(), filewalk, completemsg_end);
                            if (!filewalk) {
                                infile.close();
                                continue;
                            }

                            filewalk = put_in_buffer((currbinsharename).c_str(), filewalk, completemsg_end);
                            if (!filewalk) {
                                infile.close();
                                continue;
                            }

							filewalk = put_in_buffer(this->currbin->idstr.c_str(), filewalk, completemsg_end);
							if (!filewalk) {
								infile.close();
								continue;
							}

							filewalk = put_in_buffer(std::to_string(i * 12 + j).c_str(), filewalk, completemsg_end);
							if (!filewalk) {
								infile.close();
								continue;
							}

							filewalk = put_in_buffer(suffix.c_str(), filewalk, completemsg_end);
                            if (!filewalk) {
                                infile.close();
                                continue;
                            }

                            filewalk = put_in_buffer(std::to_string(filesize).c_str(), filewalk, completemsg_end);
                            if (!filewalk) {
                                infile.close();
                                continue;
                            }

                            // Read file data directly into message buffer
                            infile.read(filewalk, filesize);
                            infile.close();

                            // Send complete message
                            size_t total_size = filewalk - completemsg.get() + filesize;
                            int sent = mainprogram->bl_send(sock, completemsg.get(), total_size, 0);
                            if (sent != total_size) {
                                std::cout << "DEBUG: FILE_SENT send failed - sent " << sent << " of " << total_size << " bytes" << std::endl;
                            }
                        }
                    }
                }
            }

            // Send texture files for each BinElement (only if textures need to be sent)
            if (send_textures) {
                for (int i = 0; i < 12; i++) {
                    for (int j = 0; j < 12; j++) {
                        BinElement *binel = this->currbin->elements[i * 12 + j];

                        bool cond = false;
                        if (this->currbin->prevtexes.empty()) {
                            cond = true;
                        }
                        else {
                            cond = (this->currbin->prevtexes[i * 12 + j] != temptexes[i * 12 + j]);
                        }

                        if (cond) {
                            // Try to open file if texture exists
                            std::ifstream infile;
                            size_t filesize = 0;
                            bool has_texture = false;

                            if (binel->tex != -1 && !binel->absjpath.empty()) {
                                infile.open(binel->absjpath, std::ios::binary | std::ios::ate);
                                if (infile.is_open()) {
                                    filesize = infile.tellg();
                                    infile.seekg(0, std::ios::beg);
                                    has_texture = true;
                                }
                            }

                            if (has_texture) {
                                // Send actual texture file
                                // Calculate payload size FIRST (without building buffer yet)
                                size_t payload_base =
                                        //str_len(mainprogram->seatname) +
                                        str_len(currbinsharename) +
										str_len(this->currbin->idstr) +
                                        str_len(std::to_string(i * 12 + j)) +
                                        str_len(std::to_string(filesize)) +
                                        filesize;

                                // Calculate total message length including msglen field itself
                                size_t total_msglen = calculate_message_length(payload_base);
                                std::string msglen_str = std::to_string(total_msglen);

                                // Now build the actual buffer with correct sizes
                                size_t header_estimate = 256;
                                size_t buffer_size = header_estimate + filesize;
                                std::unique_ptr<char[]> completemsg(new char[buffer_size]());
                                char *completemsg_end = completemsg.get() + buffer_size;
                                char *texwalk = completemsg.get();

                                texwalk = put_in_buffer("TEX_SENT", texwalk, completemsg_end);
                                if (!texwalk) {
                                    infile.close();
                                    continue;
                                }

                                texwalk = put_in_buffer(msglen_str.c_str(), texwalk, completemsg_end);
                                if (!texwalk) {
                                    infile.close();
                                    continue;
                                }

                                texwalk = put_in_buffer(mainprogram->seatname.c_str(), texwalk, completemsg_end);
                                if (!texwalk) {
                                    infile.close();
                                    continue;
                                }

                                texwalk = put_in_buffer((currbinsharename).c_str(), texwalk, completemsg_end);
                                if (!texwalk) {
                                    infile.close();
                                    continue;
                                }

								texwalk = put_in_buffer(this->currbin->idstr.c_str(), texwalk, completemsg_end);
								if (!texwalk) {
									infile.close();
									continue;
								}

								texwalk = put_in_buffer(std::to_string(i * 12 + j).c_str(), texwalk, completemsg_end);
								if (!texwalk) {
									infile.close();
									continue;
								}

								texwalk = put_in_buffer(std::to_string(filesize).c_str(), texwalk, completemsg_end);
                                if (!texwalk) {
                                    infile.close();
                                    continue;
                                }

                                // Read file data directly into message buffer
                                infile.read(texwalk, filesize);
                                infile.close();

                                // Send complete message
                                size_t total_size = texwalk - completemsg.get() + filesize;
                                int sent = mainprogram->bl_send(sock, completemsg.get(), total_size, 0);
                                if (sent != total_size) {
                                    std::cout << "DEBUG: TEX_SENT send failed - sent " << sent << " of " << total_size << " bytes" << std::endl;
                                }
                            } else {
                                // No texture or file couldn't be opened - send placeholder
                                // Calculate payload size FIRST
                                size_t payload_base =
                                        //str_len(mainprogram->seatname) +
                                        str_len(currbinsharename) +
										str_len(this->currbin->idstr) +
                                        str_len(std::to_string(i * 12 + j)) +
                                        str_len("0");  // filesize = 0

                                // Calculate total message length including msglen field itself
                                size_t total_msglen = calculate_message_length(payload_base);
                                std::string msglen_str = std::to_string(total_msglen);

                                // Now build the buffer
                                char texheader[256] = {0};
                                char *texheader_end = texheader + sizeof(texheader);
                                char *texwalk = texheader;

                                texwalk = put_in_buffer("TEX_SENT", texwalk, texheader_end);
                                if (!texwalk) continue;

                                texwalk = put_in_buffer(msglen_str.c_str(), texwalk, texheader_end);
                                if (!texwalk) continue;

                                texwalk = put_in_buffer(mainprogram->seatname.c_str(), texwalk, texheader_end);
                                if (!texwalk) continue;

                                texwalk = put_in_buffer((currbinsharename).c_str(), texwalk, texheader_end);
                                if (!texwalk) continue;

								texwalk = put_in_buffer(this->currbin->idstr.c_str(), texwalk, texheader_end);
								if (!texwalk) continue;

								texwalk = put_in_buffer(std::to_string(i * 12 + j).c_str(), texwalk, texheader_end);
								if (!texwalk) continue;

								texwalk = put_in_buffer("0", texwalk, texheader_end);  // file size = 0
                                if (!texwalk) continue;

                                int sent = mainprogram->bl_send(sock, texheader, texwalk - texheader, 0);
                                if (sent != texwalk - texheader) {
                                    std::cout << "DEBUG: TEX_SENT (empty) send failed - sent " << sent << " of " << (texwalk - texheader) << " bytes" << std::endl;
                                }
                            }
                        }
                    }
                }
            }
            
            // Update tracking variables
            this->currbin->prevfiles = tempfiles;
            this->currbin->prevtexes = temptexes;
            this->currbin->prevnames = tempnames;
        }

        if (!mainprogram->server) {
            // only send to server
            break;
        }
    }

    // Clear last_message_sender after processing this recipient
    // This happens whether or not we actually sent (could be skipped due to pending_textures)
    // Must clear so future messages can be processed
    if (mainprogram->server) {
        this->currbin->last_message_sender = "";
    }
}

void BinsMain::receive_shared_bins() {
	// receive sent bins
	if (!this->messages.empty()) {
        // Use proper synchronization for message access
        std::lock_guard<std::mutex> lock(this->syncmutex);
        
        // Process only one message at a time to avoid iterator invalidation
        if (this->messages.empty()) return;
        
        char* message = this->messages[0];
        char* rawmessage = nullptr;
        if (mainprogram->server && !this->rawmessages.empty()) {
            rawmessage = this->rawmessages[0];
        }
        std::string messagesockname;
        if (!this->messagesocknames.empty()) {
            messagesockname = this->messagesocknames[0];
        }
        int messagelength = 0;
        if (!this->messagelengths.empty()) {
            messagelength = this->messagelengths[0];
        }
        
        // Process message safely
        char *walk = message;
        if (!walk) {
            // Clean up and return if null message
            this->messages.erase(this->messages.begin());
            if (mainprogram->server && !this->rawmessages.empty()) {
                this->rawmessages.erase(this->rawmessages.begin());
            }
            if (!this->messagelengths.empty()) {
                this->messagelengths.erase(this->messagelengths.begin());
            }
            if (!this->messagesocknames.empty()) {
                this->messagesocknames.erase(this->messagesocknames.begin());
            }
            return;
        }

        walk += strlen(walk) + 1;
		std::string sharename(walk);
		walk += strlen(walk) + 1;
		std::string idstr(walk);

		walk += strlen(walk) + 1;

        // Read texture count
        std::string texture_count_str(walk);
        int expected_textures = 0;
        try {
            expected_textures = std::stoi(texture_count_str);
        } catch (...) {
            expected_textures = 0;
        }
        walk += strlen(walk) + 1;

        // Read LAYER/DECK/MIX file count
        std::string file_count_str(walk);
        int expected_files = 0;
        try {
            expected_files = std::stoi(file_count_str);
        } catch (...) {
            expected_files = 0;
        }
        walk += strlen(walk) + 1;

        Bin *binis = nullptr;
        for (Bin *bin: binsmain->bins) {
			if (this->idtonamemap.count(idstr)) {
				if (bin->name == this->idtonamemap[idstr] && bin->idstr == idstr) {
					binis = bin;
					break;
				}
			}
        }
        if (!binis) {
	        binis = new_bin(sharename, true);  // Pass shared=true
			binis->idstr = idstr;
			// Now set the maps with the (potentially renamed) bin name
			this->sharedbinnamesmap[binis->name] = sharename;
			this->idtonamemap[idstr] = binis->name;
        }

        // Check if there's a pending send for this bin (last_message_sender is set)
        // OR if we're still receiving textures from the previous message
        // If so, postpone processing this message until the pending operations complete
        if (!binis->last_message_sender.empty() || binis->pending_textures > 0 || binis->pending_files > 0) {
            std::cout << "DEBUG: Postponing BIN_SENT for '" << sharename << "' - pending operations "
                     << "(last_sender='" << binis->last_message_sender << "', pending_textures="
                                         << binis->pending_textures << "', pending_files="
                                         << binis->pending_files << ")" << std::endl;
            // Don't remove from queue - will be processed next time
            return;
        }

        make_currbin(binis->pos);
        binis->pending_textures = expected_textures;  // Store how many textures to expect
        binis->pending_files = expected_files;  // Store how many textures to expect

        // Set the owner to the sender if not already set
        if (binis->owner.empty() && !messagesockname.empty()) {
            binis->owner = messagesockname;
            std::cout << "DEBUG: Setting bin '" << sharename << "' owner to '" << messagesockname << "'" << std::endl;
        }

        // Track who sent this message to avoid echoing it back
        binis->last_message_sender = messagesockname;

        std::cout << "DEBUG: Received bin '" << sharename << "' from '" << messagesockname << "' expecting " << expected_textures << " textures" << std::endl;

        // SERVER: When receiving a bin from a client, add client to sendtonames for bidirectional sync
        if (mainprogram->server && !messagesockname.empty()) {
            // Check if this client is already in sendtonames
            auto it = std::find(binis->sendtonames.begin(), binis->sendtonames.end(), messagesockname);
            if (it == binis->sendtonames.end()) {
                std::cout << "DEBUG: Server adding client '" << messagesockname << "' to bin '" << sharename << "' sendtonames for bidirectional sync" << std::endl;
                binis->sendtonames.push_back(messagesockname);
            }
        }

        // Safely parse bin elements with bounds checking
        for (int i2 = 0; i2 < 12; i2++) {
            for (int j = 0; j < 12; j++) {
                BinElement *binel = this->currbin->elements[i2 * 12 + j];
                
                // Check if we have enough data left to read
                if (walk >= message + messagelength - 2) break;
                
                std::string name(walk);
                walk += strlen(walk) + 1;
                if (walk >= message + messagelength - 1) break;
                
                std::string path(walk);
                walk += strlen(walk) + 1;
                if (walk > message + messagelength) break;
                
                std::string teststr;
                if (exists(path)) {
                    teststr = path;
                } else {
                    teststr = test_driveletters(path);
                }
                binel->name = name;
                bool islayer = false;
                bool isdeck = false;
                bool ismix = false;
                if (path.size() > 6) {
                    islayer = (path.substr(path.size() - 6, 6) == ".layer");
                }
                if (path.size() > 5) {
                    isdeck = (path.substr(path.size() - 5, 5) == ".deck");
                }
                if (path.size() > 4) {
                    ismix = (path.substr(path.size() - 4, 4) == ".mix");
                }
                binel->path = teststr;
                if (teststr == "" && !islayer && !isdeck && !ismix) {
                    binel->name = "";
				}
                if (islayer || isdeck || ismix) {
                    binel->path = path;
                }
                binel->type = ELEM_FILE;
            }
        }

        this->menuactbinel = this->currbin->elements[0];  // loading starts from first bin element

        // SERVER: Forward message to all subscribed clients (except sender)
        if (mainprogram->server && rawmessage && messagelength > 0) {
            // Parse sender name and bin name and bin id from message
            char* parse_ptr = message;
            std::string sender_name(parse_ptr);
			parse_ptr += sender_name.length() + 1;
			std::string bin_name(parse_ptr);
			parse_ptr += bin_name.length() + 1;
			std::string idstr(parse_ptr);

            // Find subscribers for this bin
            std::lock_guard<std::mutex> sub_lock(mainprogram->subscriptionMutex);
            auto key = std::make_pair(sender_name, idstr);
            auto it = mainprogram->subscriptionMap.find(key);

            if (it != mainprogram->subscriptionMap.end()) {
                for (const auto& subscriber_name : it->second) {
                    // Don't send back to the sender
                    if (subscriber_name == messagesockname) continue;
                    if (subscriber_name == mainprogram->seatname) continue;

                    // Look up subscriber socket
                    auto sock_it = mainprogram->connmap.find(subscriber_name);
                    if (sock_it != mainprogram->connmap.end()) {
                        #ifdef WINDOWS
                        SOCKET subscriber_socket = sock_it->second;
                        #endif
                        #ifdef POSIX
                        int subscriber_socket = sock_it->second;
                        #endif

                        // Forward the raw message
                        int sent = mainprogram->bl_send(subscriber_socket, rawmessage, messagelength, 0);
                        if (sent != messagelength) {
                            std::cout << "DEBUG: Failed to forward BIN_SENT to subscriber, sent " << sent << " of " << messagelength << " bytes" << std::endl;
                        }
                    }
                }
            }
        }

        // Clean up processed message and free memory
        if (rawmessage) free(rawmessage);
        free(message);  // Free the allocated message memory
        
        // Remove from vectors
        this->messages.erase(this->messages.begin());
        if (mainprogram->server && !this->rawmessages.empty()) {
            this->rawmessages.erase(this->rawmessages.begin());
        }
        if (!this->messagelengths.empty()) {
            this->messagelengths.erase(this->messagelengths.begin());
        }
        if (!this->messagesocknames.empty()) {
            this->messagesocknames.erase(this->messagesocknames.begin());
        }

		this->currbin->prevnames.clear();
		for (auto elem : this->currbin->elements) {
			this->currbin->prevnames.push_back(elem->name);
		}
	}
}

void BinsMain::receive_shared_textures() {
    // receive sent texture files
    if (!this->texmessages.empty()) {
        // Use proper synchronization for texture message access
        std::lock_guard<std::mutex> lock(this->syncmutex);

        // Process only one texture message at a time to avoid iterator invalidation
        if (this->texmessages.empty()) return;

        char* texmessage = this->texmessages[0];
        char* rawtexmessage = nullptr;
        int rawtexmessagelength = 0;
        if (mainprogram->server && !this->rawtexmessages.empty()) {
            rawtexmessage = this->rawtexmessages[0];
        }
        if (mainprogram->server && !this->rawtexmessagelengths.empty()) {
            rawtexmessagelength = this->rawtexmessagelengths[0];
        }
        std::string texmessagesockname;
        if (!this->texmessagesocknames.empty()) {
            texmessagesockname = this->texmessagesocknames[0];
        }
        int texmessagelength = 0;
        if (!this->texmessagelengths.empty()) {
            texmessagelength = this->texmessagelengths[0];
        }

        // Process texture message safely
        if (!texmessage) {
            // Clean up and return if null message
            this->texmessages.erase(this->texmessages.begin());
            if (mainprogram->server && !this->rawtexmessages.empty()) {
                this->rawtexmessages.erase(this->rawtexmessages.begin());
            }
            if (mainprogram->server && !this->rawtexmessagelengths.empty()) {
                this->rawtexmessagelengths.erase(this->rawtexmessagelengths.begin());
            }
            if (!this->texmessagelengths.empty()) {
                this->texmessagelengths.erase(this->texmessagelengths.begin());
            }
            if (!this->texmessagesocknames.empty()) {
                this->texmessagesocknames.erase(this->texmessagesocknames.begin());
            }
            return;
        }

        char *walk = texmessage;
        char *message_end = texmessage + texmessagelength;

        // Initialize all variables before any goto statements to avoid crossing initialization
        int pos = 0, filesize = 0, binid = 0;
        Bin *targetbin = nullptr;
        std::string binname, posstr, filesizestr, idstr;

        // Safely parse texture message with bounds checking
        // Note: seatname was already parsed by socket_client/socket_server_receive
        // The message here contains only: binname, position, filesize, [binary data]

        if (walk >= message_end) {
            goto cleanup;
        }
		binname = std::string(walk, strnlen(walk, message_end - walk));
		walk += binname.length() + 1;
		idstr = std::string(walk, strnlen(walk, message_end - walk));
		walk += idstr.length() + 1;

        if (walk >= message_end) {
            goto cleanup;
        }
        posstr = std::string(walk, strnlen(walk, message_end - walk));
        walk += posstr.length() + 1;

        if (walk >= message_end) {
            goto cleanup;
        }
        filesizestr = std::string(walk, strnlen(walk, message_end - walk));
        walk += filesizestr.length() + 1;

        try {
            pos = std::stoi(posstr);
            filesize = std::stoi(filesizestr);
        } catch (const std::exception& e) {
            std::cout << "DEBUG: Invalid texture message format" << std::endl;
            goto cleanup;
        }

		for (Bin *bin: binsmain->bins) {
			if (this->idtonamemap.count(idstr)) {
				if (bin->name == this->idtonamemap[idstr] && bin->idstr == idstr) {
					targetbin = bin;
					break;
				}
			}
		}
		if (!targetbin) {
			targetbin = new_bin(binname, true);  // Pass shared=true
			targetbin->idstr = idstr;
			// Now set the maps with the (potentially renamed) bin name
			this->sharedbinnamesmap[targetbin->name] = binname;
			this->idtonamemap[idstr] = targetbin->name;
		}

		if (targetbin && pos >= 0 && pos < 144) {
            BinElement *binel = targetbin->elements[pos];

            if (filesize == 0) {
                // No texture - set to placeholder
                blacken(binel->tex);
            } else if (filesize > 0 && filesize <= 50*1024*1024 && walk + filesize <= message_end) {  // Size limit and bounds check
                // Receive texture file data
                char *texturedata = walk;

                std::string jpath = find_unused_filename(remove_extension(basename(binel->path)), mainprogram->project->binsdir + targetbin->name + "/", ".jpeg");

                // Write received data to temporary file
                std::ofstream tempfile(jpath, std::ios::binary);
                if (tempfile.is_open()) {
                    tempfile.write(texturedata, filesize);
                    tempfile.close();

                    // Set jpeg paths for the received texture
                    binel->jpegpath = pathtoplatform(jpath);
                    binel->absjpath = pathtoplatform(jpath);
                    binel->reljpath = std::filesystem::relative(binel->absjpath, mainprogram->project->binsdir).generic_string();

                    std::cout << "DEBUG: Loading texture from: " << binel->absjpath << std::endl;
                    std::cout << "DEBUG: File exists: " << (exists(binel->absjpath) ? "YES" : "NO") << std::endl;
                    std::cout << "DEBUG: Old texture ID: " << binel->tex << std::endl;

                    // Delete old texture and create a new one to ensure clean state
                    if (binel->tex != -1) {
                        glDeleteTextures(1, &binel->tex);
                    }
                    glGenTextures(1, &binel->tex);
                    glBindTexture(GL_TEXTURE_2D, binel->tex);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

                    std::cout << "DEBUG: New texture ID: " << binel->tex << std::endl;
                    open_thumb(binel->absjpath, binel->tex);

                    std::cout << "DEBUG: Texture loaded for bin '" << binname << "' position " << pos << std::endl;
                } else {
                    std::cout << "DEBUG: Failed to create temporary file for texture at: " << jpath << std::endl;
                }
            }

            // Decrement pending texture count
            if (targetbin->pending_textures > 0) {
                targetbin->pending_textures--;
                std::cout << "DEBUG: Received texture for bin '" << binname << "', "
                          << targetbin->pending_textures << " textures remaining" << std::endl;
            }
        }

        // SERVER: Forward texture message to all subscribed clients (except sender)
        if (mainprogram->server && rawtexmessage && rawtexmessagelength > 0 && targetbin) {
            // Parse sender name from the beginning of texmessage
            // Note: The sender name should have been stored in texmessagesockname

            // Find subscribers for this bin
            // We need to identify the bin owner - for now assume it's from messagesockname
            std::lock_guard<std::mutex> sub_lock(mainprogram->subscriptionMutex);

            // Try to find matching subscription - search through all keys
            for (auto& [key, subscribers] : mainprogram->subscriptionMap) {
                const auto& [owner, id_str] = key;
                if (id_str == idstr) {
                    // Forward to all subscribers except the sender
                    for (const auto& subscriber_name : subscribers) {
                        // Don't send back to the sender
                        if (subscriber_name == texmessagesockname) continue;
                        if (subscriber_name == mainprogram->seatname) continue;

                        // Look up subscriber socket
                        auto sock_it = mainprogram->connmap.find(subscriber_name);
                        if (sock_it != mainprogram->connmap.end()) {
#ifdef WINDOWS
                            SOCKET subscriber_socket = sock_it->second;
#endif
#ifdef POSIX
                            int subscriber_socket = sock_it->second;
#endif

                            // Forward the raw texture message
                            int sent = mainprogram->bl_send(subscriber_socket, rawtexmessage, rawtexmessagelength, 0);
                            if (sent != rawtexmessagelength) {
                                std::cout << "DEBUG: Failed to forward TEX_SENT to subscriber, sent " << sent << " of " << rawtexmessagelength << " bytes" << std::endl;
                            }
                        }
                    }
                    break;  // Found the bin, no need to continue
                }
            }
        }

        cleanup:
        // Clean up processed message and free memory
        if (rawtexmessage && rawtexmessagelength > 0) {
            free(rawtexmessage);
        }
        if (texmessage && texmessagelength > 0) {
            free(texmessage);
        }

        // Remove from vectors
        this->texmessages.erase(this->texmessages.begin());
        if (mainprogram->server && !this->rawtexmessages.empty()) {
            this->rawtexmessages.erase(this->rawtexmessages.begin());
        }
        if (mainprogram->server && !this->rawtexmessagelengths.empty()) {
            this->rawtexmessagelengths.erase(this->rawtexmessagelengths.begin());
        }
        if (!this->texmessagelengths.empty()) {
            this->texmessagelengths.erase(this->texmessagelengths.begin());
        }
        if (!this->texmessagesocknames.empty()) {
            this->texmessagesocknames.erase(this->texmessagesocknames.begin());
        }

        this->currbin->prevtexes.clear();
        for (auto elem : this->currbin->elements) {
            this->currbin->prevtexes.push_back(elem->absjpath);
        }
    }
}

void BinsMain::receive_shared_files() {
    // receive sent LAYER/DECK/MIX files
    if (!this->filemessages.empty()) {
        // Use proper synchronization for file message access
        std::lock_guard<std::mutex> lock(this->syncmutex);

        // Process only one file message at a time to avoid iterator invalidation
        if (this->filemessages.empty()) return;

        char* filemessage = this->filemessages[0];
        char* rawfilemessage = nullptr;
        int rawfilemessagelength = 0;
        if (mainprogram->server && !this->rawfilemessages.empty()) {
            rawfilemessage = this->rawfilemessages[0];
        }
        if (mainprogram->server && !this->rawfilemessagelengths.empty()) {
            rawfilemessagelength = this->rawfilemessagelengths[0];
        }
        std::string filemessagesockname;
        if (!this->filemessagesocknames.empty()) {
            filemessagesockname = this->filemessagesocknames[0];
        }
        int filemessagelength = 0;
        if (!this->filemessagelengths.empty()) {
            filemessagelength = this->filemessagelengths[0];
        }

        // Process file message safely
        if (!filemessage) {
            // Clean up and return if null message
            this->filemessages.erase(this->filemessages.begin());
            if (mainprogram->server && !this->rawfilemessages.empty()) {
                this->rawfilemessages.erase(this->rawfilemessages.begin());
            }
            if (mainprogram->server && !this->rawfilemessagelengths.empty()) {
                this->rawfilemessagelengths.erase(this->rawfilemessagelengths.begin());
            }
            if (!this->filemessagelengths.empty()) {
                this->filemessagelengths.erase(this->filemessagelengths.begin());
            }
            if (!this->filemessagesocknames.empty()) {
                this->filemessagesocknames.erase(this->filemessagesocknames.begin());
            }
            return;
        }

        char *walk = filemessage;
        char *message_end = filemessage + filemessagelength;

        // Initialize all variables before any goto statements to avoid crossing initialization
        int pos = 0, filesize = 0;
        Bin *targetbin = nullptr;
        std::string binname, idstr, posstr, typestr, filesizestr;

        // Safely parse file message with bounds checking
        // Note: seatname was already parsed by socket_client/socket_server_receive
        // The message here contains only: binname, position, filetype, filesize, [binary data]

        if (walk >= message_end) {
            goto cleanup;
        }
		binname = std::string(walk, strnlen(walk, message_end - walk));
		walk += binname.length() + 1;
		idstr = std::string(walk, strnlen(walk, message_end - walk));
		walk += idstr.length() + 1;

        if (walk >= message_end) {
            goto cleanup;
        }
        posstr = std::string(walk, strnlen(walk, message_end - walk));
        walk += posstr.length() + 1;

        if (walk >= message_end) {
            goto cleanup;
        }
        typestr = std::string(walk, strnlen(walk, message_end - walk));
        walk += typestr.length() + 1;

        if (walk >= message_end) {
            goto cleanup;
        }
        filesizestr = std::string(walk, strnlen(walk, message_end - walk));
        walk += filesizestr.length() + 1;

        try {
            pos = std::stoi(posstr);
            filesize = std::stoi(filesizestr);
        } catch (const std::exception& e) {
            std::cout << "DEBUG: Invalid file message format" << std::endl;
            goto cleanup;
        }

		for (Bin *bin: binsmain->bins) {
			if (this->idtonamemap.count(idstr)) {
				if (bin->name == this->idtonamemap[idstr] && bin->idstr == idstr) {
					targetbin = bin;
					break;
				}
			}
		}
		if (!targetbin) {
			targetbin = new_bin(binname, true);  // Pass shared=true
			targetbin->idstr = idstr;
			// Now set the maps with the (potentially renamed) bin name
			this->sharedbinnamesmap[targetbin->name] = binname;
			this->idtonamemap[idstr] = targetbin->name;
		}

		if (targetbin && pos >= 0 && pos < 144) {
            BinElement *binel = targetbin->elements[pos];

            if (filesize > 0 && filesize <= 50*1024*1024 && walk + filesize <= message_end) {  // Size limit and bounds check
                // Receive file data
                char *filedata = walk;

                std::string jpath = find_unused_filename(remove_extension(basename(binel->path)), mainprogram->project->binsdir + targetbin->name + "/", typestr);

                // Write received data to file
                std::ofstream outfile(jpath, std::ios::binary);
                if (outfile.is_open()) {
                    outfile.write(filedata, filesize);
                    outfile.close();
                    
                    binel->path = jpath;

                    if (typestr == ".layer") {
                        binel->type = ELEM_LAYER;
                    }
                    else if (typestr == ".deck") {
                        binel->type = ELEM_DECK;
                    }
                    else if (typestr == ".mix") {
                        binel->type = ELEM_MIX;
                    }

                    std::cout << "DEBUG: Loading " << typestr << " file from: " << binel->path << std::endl;
                    std::cout << "DEBUG: File exists: " << (exists(binel->absjpath) ? "YES" : "NO") << std::endl;
                    std::cout << "DEBUG: Old texture ID: " << binel->tex << std::endl;
                } else {
                    std::cout << "DEBUG: Failed to create file in bin directory for " + typestr + " file at: " << jpath << std::endl;
                }
            }

            // Decrement pending file count
            if (targetbin->pending_files > 0) {
                targetbin->pending_files--;
                std::cout << "DEBUG: Received file for bin '" << binname << "', "
                          << targetbin->pending_files << " files remaining" << std::endl;
            }
        }

        // SERVER: Forward file message to all subscribed clients (except sender)
        if (mainprogram->server && rawfilemessage && rawfilemessagelength > 0 && targetbin) {
            // Parse sender name from the beginning of filemessage
            // Note: The sender name should have been stored in filemessagesockname

            // Find subscribers for this bin
            // We need to identify the bin owner - for now assume it's from messagesockname
            std::lock_guard<std::mutex> sub_lock(mainprogram->subscriptionMutex);

            // Try to find matching subscription - search through all keys
            for (auto& [key, subscribers] : mainprogram->subscriptionMap) {
                const auto& [owner, id_str] = key;
                if (id_str == idstr) {
                    // Forward to all subscribers except the sender
                    for (const auto& subscriber_name : subscribers) {
                        // Don't send back to the sender
                        if (subscriber_name == filemessagesockname) continue;
                        if (subscriber_name == mainprogram->seatname) continue;

                        // Look up subscriber socket
                        auto sock_it = mainprogram->connmap.find(subscriber_name);
                        if (sock_it != mainprogram->connmap.end()) {
#ifdef WINDOWS
                            SOCKET subscriber_socket = sock_it->second;
#endif
#ifdef POSIX
                            int subscriber_socket = sock_it->second;
#endif

                            // Forward the raw file message
                            int sent = mainprogram->bl_send(subscriber_socket, rawfilemessage, rawfilemessagelength, 0);
                            if (sent != rawfilemessagelength) {
                                std::cout << "DEBUG: Failed to forward FILE_SENT to subscriber, sent " << sent << " of " << rawfilemessagelength << " bytes" << std::endl;
                            }
                        }
                    }
                    break;  // Found the bin, no need to continue
                }
            }
        }

        cleanup:
        // Clean up processed message and free memory
        if (rawfilemessage && rawfilemessagelength > 0) {
            free(rawfilemessage);
        }
        if (filemessage && filemessagelength > 0) {
            free(filemessage);
        }

        // Remove from vectors
        this->filemessages.erase(this->filemessages.begin());
        if (mainprogram->server && !this->rawfilemessages.empty()) {
            this->rawfilemessages.erase(this->rawfilemessages.begin());
        }
        if (mainprogram->server && !this->rawfilemessagelengths.empty()) {
            this->rawfilemessagelengths.erase(this->rawfilemessagelengths.begin());
        }
        if (!this->filemessagelengths.empty()) {
            this->filemessagelengths.erase(this->filemessagelengths.begin());
        }
        if (!this->filemessagesocknames.empty()) {
            this->filemessagesocknames.erase(this->filemessagesocknames.begin());
        }

        this->currbin->prevfiles.clear();
        for (auto elem : this->currbin->elements) {
            if (elem->type == ELEM_LAYER || elem->type == ELEM_DECK || elem->type == ELEM_MIX) {
                this->currbin->prevfiles.push_back(elem->path);
            }
            else {
                this->currbin->prevfiles.push_back("");
            }
        }
    }
}


void BinsMain::open_bin(std::string path, Bin *bin, bool newbin) {
	// open a bin file
	std::string result = mainprogram->deconcat_files(path);
	bool concat = (result != "");
	std::ifstream rfile;
	if (concat) rfile.open(result);
	else rfile.open(path);

	int filecount = 0;
	int pos;
	std::string abspath;
	std::string istring;
	safegetline(rfile, istring);
	//check if binfile
	while (safegetline(rfile, istring)) {
		if (istring == "ENDOFFILE") {
			break;
		}
		else if (istring == "ELEMS") {
			// open bin elements
            bool reta = false;
			while (safegetline(rfile, istring)) {
				if (istring == "ENDOFELEMS") break;
				if (istring == "POS") {
					safegetline(rfile, istring);
					pos = std::stoi(istring);
                    bin->elements[pos]->pos = pos;
                    bin->elements[pos]->bin = bin;
                    bin->elements[pos]->autosavejpegsaved = false;
                }
                if (istring == "ABSPATH") {
                    reta = false;
                    safegetline(rfile, istring);
					abspath = istring;
                    bin->elements[pos]->path = istring;
                    bin->elements[pos]->relpath = std::filesystem::relative(istring, mainprogram->project->binsdir).generic_string();
                }
                if (istring == "RELPATH") {
                    safegetline(rfile, istring);
                    if (istring == "" && abspath == "") {
						continue;
					}
                    std::filesystem::current_path(mainprogram->project->binsdir);
                    if (istring != "" && bin->elements[pos]->path == "" && exists(istring)) {
                        bin->elements[pos]->path = pathtoplatform(std::filesystem::absolute(istring).generic_string());
                        bin->elements[pos]->relpath = std::filesystem::relative(istring, mainprogram->project->binsdir).generic_string();
                    }
                    std::filesystem::current_path(mainprogram->contentpath);
                    if (!exists(bin->elements[pos]->path)) {
                        auto teststr = test_driveletters(abspath);
                        if (teststr == "") {
                            reta = true;
                            mainmix->retargeting = true;
                            mainmix->newbinelpaths.push_back(bin->elements[pos]->path);
                            mainmix->newpathbinels.push_back(bin->elements[pos]);
                        }
                        else {
                            bin->elements[pos]->path = teststr;
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
				}
				if (istring == "ABSJPEGPATH") {
					safegetline(rfile, istring);
                    if (exists(istring)) {
                        bin->elements[pos]->absjpath = istring;
                        bin->elements[pos]->jpegpath = istring;
                        if (bin->elements[pos]->name != "") {
                            if (bin->elements[pos]->absjpath != "") {
                                bin->open_positions.emplace(pos);
                            }
                            else {
                                bool dummy = false;
                            }
                        }
                    }
                    if (reta) {
                        mainmix->newbineljpegpaths.push_back(istring);
                    }
				}
                if (istring == "RELJPEGPATH") {
                    safegetline(rfile, istring);
                    bin->elements[pos]->reljpath = istring;
                    if (bin->elements[pos]->absjpath == "" && istring != "") {
                        std::filesystem::current_path(mainprogram->project->binsdir);
                        bin->elements[pos]->absjpath = std::filesystem::absolute(istring).generic_string();
                        bin->elements[pos]->jpegpath = bin->elements[pos]->absjpath;
                        if (bin->elements[pos]->name != "") {
                            if (bin->elements[pos]->absjpath != "") {
                                bin->open_positions.emplace(pos);
                            } else {
                                bool dummy = false;
                            }
                        }
                    }
                    else if (bin->elements[pos]->absjpath == "" && istring == "") {
                        bin->elements[pos]->erase(true);
                    }
                }
                if (istring == "FILESIZE") {
                    safegetline(rfile, istring);
                    bin->elements[pos]->filesize = std::stoll(istring);
                }
            }
		}
	}

    rfile.close();

    if (!newbin) {
        bin->saved = true;
    }
}


void BinsMain::save_bin(std::string path) {
	// save bin file
	std::vector<std::string> filestoadd;
	std::ofstream wfile;
	wfile.open(path.c_str());
	wfile << "EWOCvj BINFILE\n";

	wfile << "ELEMS\n";
	// save elements
	for (int i = 0; i < 12; i++) {
		for (int j = 0; j < 12; j++) {
            BinElement *binel = this->currbin->elements[i * 12 + j];
			wfile << "POS\n";
			wfile << i * 12 + j;
			wfile << "\n";
            std::string p = binel->path;
            std::string rp = binel->relpath;
            std::string jpap = binel->absjpath;
            std::string jprp = binel->reljpath;
			wfile << "ABSPATH\n";
			wfile << p;
            wfile << "\n";
            wfile << "RELPATH\n";
            wfile << rp;
            wfile << "\n";
			wfile << "NAME\n";
			wfile << binel->name;
			wfile << "\n";
			wfile << "TYPE\n";
			wfile << std::to_string(binel->type);
			wfile << "\n";
			ELEM_TYPE type = binel->type;
            wfile << "ABSJPEGPATH\n";
            wfile << jpap;
            wfile << "\n";
            wfile << "RELJPEGPATH\n";
            wfile << jprp;
            wfile << "\n";
            if (p != "") {
                wfile << "FILESIZE\n";
                wfile << std::to_string(std::filesystem::file_size(p));
                wfile << "\n";
            }
            else {}
		}
	}
	wfile << "ENDOFELEMS\n";

	wfile << "ENDOFFILE\n";
	wfile.close();

    if (!exists(mainprogram->project->binsdir + this->currbin->name)) {
        std::filesystem::path p1{mainprogram->project->binsdir + this->currbin->name};
        std::filesystem::create_directory(p1);
    }
	std::string tpath = mainprogram->temppath + "tempconcatbin" + "_" + std::to_string(mainprogram->concatsuffix++);
    std::string ttpath = tpath;
    std::vector<std::vector<std::string>> filestoadd2;
	filestoadd2.push_back(filestoadd);

    std::thread concat(&Program::concat_files, mainprogram, ttpath, path, filestoadd2, 0, true);
    concat.detach();

    if (dirname(path) == mainprogram->project->binsdir) {
        binsmain->currbin->saved = true;
    }
}

Bin *BinsMain::new_bin(std::string name, bool shared) {
	Bin *bin = new Bin(this->bins.size());
	this->bins.push_back(bin);
	bin->pos = this->bins.size() - 1;

	for (int i = 0; i < 12; i++) {
		for (int j = 0; j < 12; j++) {
			BinElement *binel = new BinElement;
            binel->pos = j * 12 + i;
            binel->bin = bin;
			bin->elements.push_back(binel);
		}
	}
	make_currbin(this->bins.size() - 1);
	std::string path;
	bin->path = mainprogram->project->binsdir + name + ".bin";
    bin->name = name;
	bin->shared = shared;
	// no two bins can have the same name
	this->solve_nameclashes();
	std::filesystem::path p1{ mainprogram->project->binsdir + bin->name };
	std::filesystem::create_directory(p1);
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
        this->save_bin(this->bins[0]->path);
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
		/*if (istring == "BINSSCROLL") {
			safegetline(rfile, istring);
			this->binsscroll = std::stoi(istring);
		}*/
	}
	rfile.close();

	return currbin;
}

void BinsMain::save_binslist() {
	// save the list of bins in this project
	std::ofstream wfile;
	wfile.open(mainprogram->project->binsdir + "bins.list");
	wfile << "EWOC BINSLIST v0.2\n";
    if (this->currbin->saved) {
        wfile << std::to_string(this->currbin->pos);
    }
    else {
        wfile << std::to_string(0);
    }
	wfile << "\n";
	wfile << "BINS\n";
	for (int i = 0; i < this->bins.size(); i++) {
		std::string path = mainprogram->project->binsdir + this->bins[i]->name + ".bin";
		wfile << this->bins[i]->name;
		wfile << "\n";
	}
	wfile << "ENDOFBINS\n";
/*	wfile << "BINSSCROLL\n";
	wfile << std::to_string(this->binsscroll);
	wfile << "\n";/\*/
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

    std::string result = mainprogram->deconcat_files(mainprogram->paths[binsmain->binscount]);
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
	binsmain->open_bin(mainprogram->paths[binsmain->binscount], bin, true);
	std::string path = mainprogram->project->binsdir + bin->name + ".bin";
	if (binsmain->bins.size() > 20) binsmain->binsscroll++;
    next_bin();
}

void BinsMain::open_files_bin() {
    // open videos/images/layer/deck/mix files into bin

    if (mainprogram->multistage < 5) {
        // order elements
        /*if (mainprogram->paths.size() == 0) {
            binsmain->openfilesbin = false;
            mainprogram->multistage = 0;
            return;
        }*/
        bool cont = mainprogram->order_paths(true);
        if (!cont) return;


        mainprogram->blocking = true;
        if (SDL_GetMouseFocus() != mainprogram->mainwindow) {  // reminder : remove?
            SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW));
        } else {
            SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT));
        }
    }

	if (mainprogram->counting == mainprogram->paths.size()) {
		this->currbin->path = mainprogram->project->binsdir + this->currbin->name + ".bin";
		this->openfilesbin = false;
        mainprogram->pathtexes.clear();
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

void BinsMain::open_handlefile(std::string path, GLuint tex) {
    if (path != "") {
        // prepare value lists for inputting videos/images/layer files from disk
        ELEM_TYPE endtype;
        GLuint endtex;

        // determine file type
        std::string istring = mainprogram->get_typestring(path);

        if (istring.find("EWOCvj LAYERFILE") != std::string::npos) {
            endtype = ELEM_LAYER;
        } else if (istring.find("EWOCvj DECKFILE") != std::string::npos) {
            endtype = ELEM_DECK;
         } else if (istring.find("EWOCvj MIXFILE") != std::string::npos) {
            endtype = ELEM_MIX;
        } else if (isimage(path)) {
           endtype = ELEM_IMAGE;
        } else if (isvideo(path)) {
            endtype = ELEM_FILE;
        } else {
            return;
        }

        if (tex != -1) {
            if (istring.find("EWOCvj LAYERFILE") != std::string::npos) {
                // prepare layer file for bin entry
                if (tex == -1) {
                    Layer *lay = new Layer(true);
                    get_layertex(lay, path);
                    std::unique_lock<std::mutex> lock2(lay->enddecodelock);
                    lay->enddecodevar.wait(lock2, [&] {return lay->processed; });
                    lay->processed = false;
                    lock2.unlock();
                    GLuint butex = lay->fbotex;
                    lay->fbotex = copy_tex(lay->texture, 192, 108);
                    glDeleteTextures(1, &butex);
                    onestepfrom(0, lay->node, nullptr, -1, -1);
                    if (lay->effects[0].size()) {
                        endtex = copy_tex(lay->effects[0][lay->effects[0].size() - 1]->fbotex, 192, 108);
                    } else {
                        endtex = copy_tex(lay->fbotex, 192, 108);
                    }
                    lay->close();
                }
                else endtex = tex;
            } else if (istring.find("EWOCvj DECKFILE") != std::string::npos) {
                // prepare deck file for bin entry
                if (tex == -1) {
                    get_deckmixtex(nullptr, path);
                    glGenTextures(1, &endtex);
                    //glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, endtex);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
                    open_thumb(mainprogram->result + "_" + std::to_string(mainprogram->resnum - 2) + ".file", endtex);
                }
                else endtex = tex;
            } else if (istring.find("EWOCvj MIXFILE") != std::string::npos) {
                // prepare layer file for bin entry
                if (tex == -1) {
                    get_deckmixtex(nullptr, path);
                    glGenTextures(1, &endtex);
                    //glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, endtex);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
                    open_thumb(mainprogram->result + "_" + std::to_string(mainprogram->resnum - 2) + ".file", endtex);
                }
                else endtex = tex;
            } else if (isimage(path)) {
                // prepare image file for bin entry
                if (tex == -1) {
                    Layer *lay = new Layer(true);
                    get_imagetex(lay, path);
                    std::unique_lock<std::mutex> lock2(lay->enddecodelock);
                    lay->enddecodevar.wait(lock2, [&] {return lay->processed; });
                    lay->processed = false;
                    lock2.unlock();
                    endtex = mainprogram->get_tex(lay);
                    lay->close();
                }
                else endtex = tex;
            } else if (isvideo(path)) {
                // prepare video file for bin entry
                if (tex == -1) {
                    Layer *lay = new Layer(true);
                    get_videotex(lay, path);
                    std::unique_lock<std::mutex> lock2(lay->enddecodelock);
                    lay->enddecodevar.wait(lock2, [&] {return lay->processed; });
                    lay->processed = false;
                    lock2.unlock();
                    endtex = mainprogram->get_tex(lay);
                    lay->close();
                }
                else endtex = tex;

            } else {
                return;
            }
        }
        else {
            endtex = mainprogram->pathtexes[mainprogram->counting];
        }

        if (endtex == -1) return;
        this->inputtexes.push_back(endtex);
        this->inputtypes.push_back(endtype);
        std::string jpath = find_unused_filename(basename(path), mainprogram->project->binsdir + this->currbin->name + "/", ".jpg");
        save_thumb(jpath, endtex);
        this->inputjpegpaths.push_back(jpath);
    }
    else {
        this->inputtexes.push_back(-1);
        this->inputtypes.push_back(ELEM_FILE);
        this->inputjpegpaths.push_back("");
    }
    this->addpaths.push_back(path);
}

void BinsMain::save_binjpegs() {
    // every loop iteration, load one bin element jpeg if there are any unloaded ones
    if (!mainmix->retargeting) {
        for (Bin *bin: this->bins) {
            if (bin->open_positions.size() != 0) {
                int pos = *(bin->open_positions.begin());
                if (bin->elements[pos]->name != "") {
                    if (exists(bin->elements[pos]->jpegpath)) {
                        std::filesystem::current_path(mainprogram->project->binsdir);
                        std::string path = pathtoplatform(
                                std::filesystem::absolute(bin->elements[pos]->jpegpath).generic_string());
                        open_thumb(path, bin->elements[pos]->tex);
                        std::filesystem::current_path(mainprogram->contentpath);
                    } else {
                        blacken(bin->elements[pos]->tex);
                    }
                } else {
                    bool dummy = false;
                }
                bin->open_positions.erase(pos);
            } else {
                // each loop iteration, save ten bin elements / element jpegs to prepare for autosave
				if (mainprogram->renaming != EDIT_BINNAME) {
					std::string str = mainprogram->project->autosavedir + "temp/bins/" + bin->name;
					if (!exists(str)) {
						std::filesystem::create_directories(std::filesystem::path(str));
					}
					int cnt = 0;
					bool brk = false;
					for (Bin *bin: this->bins) {
						for (BinElement *elem: bin->elements) {
							if (elem->path != "") {
								std::string elempath = str + "/" + basename(elem->path);
								if (!exists(elempath)) {
									if (elem->type == ELEM_LAYER || elem->type == ELEM_DECK || elem->type == ELEM_MIX) {
										copy_file(elem->path, elempath);
										cnt++;
									}
								}
								if (cnt == 10) {
									brk = true;
									break;
								}
							}
						}
						if (brk) break;
					}
					for (BinElement *binel: bin->elements) {
						if (binel->path != "") {
							if (!binel->autosavejpegsaved) {
								std::string jpgpath = str + "/" + basename(binel->jpegpath);
								binel->autosavejpegsaved = true;
								if (binel->jpegpath != "") save_thumb(jpgpath, binel->tex);
								cnt++;
								if (cnt == 10) break;
							}
						}
					}
				}
            }
        }
    }
}

std::tuple<std::string, std::string> BinsMain::hap_binel(BinElement *binel, BinElement *bdm) {
	// encode single bin element, possibly contained in a deck or a mix
    if (!check_permission(dirname(binel->path))) {
        return {"",""};
    }
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
		rpath = mainprogram->docpath + std::filesystem::relative(apath, mainprogram->docpath).generic_string();
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
					wfile << mainprogram->docpath + std::filesystem::relative(apath, mainprogram->docpath).generic_string();
					rpath = mainprogram->docpath + std::filesystem::relative(apath, mainprogram->docpath).generic_string();
					wfile << "\n";
				}
			}
			else if (istring == "RELPATH") {
				safegetline(rfile, istring);
				if (path == "") {
					rpath = istring;
					if (exists(istring)) {
						path = std::filesystem::canonical(istring).generic_string();
						rpath = remove_extension(rpath) + "_hap.mov";
						wfile << "FILENAME\n";
						wfile << std::filesystem::canonical(rpath).generic_string();
						wfile << "\n";
						wfile << "RELPATH\n";
						wfile << path;
						wfile << "\n";
						apath = std::filesystem::canonical(rpath).generic_string();
					}
					else {
						wfile << "FILENAME\n";
						wfile << path;
						wfile << "\n";
						wfile << "RELPATH\n";
						wfile << mainprogram->docpath + std::filesystem::relative(path, mainprogram->docpath).generic_string();
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
    				rpath = mainprogram->docpath + std::filesystem::relative(path, mainprogram->docpath).generic_string();
				wfile.close();
				rfile.close();
 				mainprogram->remove(remove_extension(binel->path) + ".temp");
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
				params.sched_priority = sched_get_priority_min(SCHED_FIFO);
                pthread_setschedparam(hap.native_handle(), SCHED_FIFO, &params);
				#endif
			}
			hap.detach();
		}
		else printf("Can't find file to encode");
		wfile.close();
		rfile.close();

		rename(remove_extension(binel->path) + ".temp", binel->path);
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
			BinElement binel;
			binel.path = istring;
			binel.type = ELEM_FILE;
			bd->encthreads++;
			std::tuple<std::string, std::string> output = this->hap_binel(&binel, bd);
            apath = std::get<0>(output);
			rpath = std::get<1>(output);
            if (apath == "" && rpath == "") {
                rfile.close();
                wfile.close();
                std::filesystem::remove(remove_extension(bd->path) + ".temp");
                return;
            }
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
				BinElement binel;
				binel.path = istring;
				binel.type = ELEM_FILE;
				bm->encthreads++;
				std::tuple<std::string, std::string> output = this->hap_binel(&binel, bm);
				apath = std::get<0>(output);
				rpath = std::get<1>(output);
                if (apath == "" && rpath == "") {
                    rfile.close();
                    wfile.close();
                    std::filesystem::remove(remove_extension(bm->path) + ".temp");
                    return;
                }
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

void BinsMain::hap_encode(std::string srcpath, BinElement *binel, BinElement *bdm) {
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

	// Audio stream setup for WAV extraction
	int audio_stream_idx = -1;
	AVStream *audio_stream = nullptr;
	AVCodecContext *audio_dec_ctx = nullptr;
	AVCodecParameters *audio_dec_cpm = nullptr;
	FILE *wav_file = nullptr;
	std::string wav_temp_path;
	int64_t audio_samples_written = 0;

	// Find and setup audio stream if available
	if (find_stream_index(&audio_stream_idx, source, AVMEDIA_TYPE_AUDIO) >= 0) {
		audio_stream = source->streams[audio_stream_idx];
		audio_dec_cpm = audio_stream->codecpar;
		const AVCodec *audio_codec = avcodec_find_decoder(audio_stream->codecpar->codec_id);
		if (audio_codec) {
			audio_dec_ctx = avcodec_alloc_context3(audio_codec);
			if (avcodec_parameters_to_context(audio_dec_ctx, audio_dec_cpm) >= 0) {
				audio_dec_ctx->time_base = audio_stream->time_base;
				if (avcodec_open2(audio_dec_ctx, audio_codec, nullptr) >= 0) {
					// Setup WAV file output
					wav_temp_path = remove_extension(srcpath) + "_temp.wav";
					wav_file = fopen(wav_temp_path.c_str(), "wb");
					if (wav_file) {
						// Write temporary WAV header (will be updated at the end)
						uint8_t wav_header[44] = {0};
						memcpy(wav_header, "RIFF", 4);
						memcpy(wav_header + 8, "WAVE", 4);
						memcpy(wav_header + 12, "fmt ", 4);
						*(uint32_t*)(wav_header + 16) = 16; // PCM format chunk size
						*(uint16_t*)(wav_header + 20) = 1;  // PCM format
						*(uint16_t*)(wav_header + 22) = audio_dec_cpm->ch_layout.nb_channels;
						*(uint32_t*)(wav_header + 24) = audio_dec_cpm->sample_rate;
						*(uint32_t*)(wav_header + 28) = audio_dec_cpm->sample_rate * audio_dec_cpm->ch_layout.nb_channels * 2; // byte rate (assuming 16-bit)
						*(uint16_t*)(wav_header + 32) = audio_dec_cpm->ch_layout.nb_channels * 2; // block align
						*(uint16_t*)(wav_header + 34) = 16; // bits per sample
						memcpy(wav_header + 36, "data", 4);
						fwrite(wav_header, 1, 44, wav_file);
					}
				}
			}
		}
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
    binel->encodingend = srcpath;

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
    if (codec == nullptr) {
        mainprogram->infostr = "Your ffmpeg library is compiled without snappy support";
        return;
    }


    c = avcodec_alloc_context3(codec);
	pkt = *av_packet_alloc();
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
    //((AVOutputFormat*)(dest->oformat))->flags |= AVFMT_NOFILE;
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
			// Clean up audio resources if encoding is cancelled
			if (wav_file) {
				fclose(wav_file);
				mainprogram->remove(wav_temp_path); // delete the wav file under construction
			}
			if (audio_dec_ctx) {
				avcodec_free_context(&audio_dec_ctx);
			}

			if (bdm) {
				bdm->encthreads--;
				delete binel;  // temp bin elements populate bdm binelements
			}
			mainprogram->encthreads--;
			avio_close(dest->pb);
			mainprogram->remove(destpath); // delete the hap file under construction
			return;
		}
		binel->encodeprogress = (float)count / (float)numf;
		if (bdm) bdm->encodeprogress += binel->encodeprogress - oldprogress;
		oldprogress = binel->encodeprogress;
		// decode a frame
		pkt = *av_packet_alloc();;
		pkt.data = nullptr;
		pkt.size = 0;
		av_read_frame(source, &pkt);

		// Handle audio packets
		if (audio_dec_ctx && pkt.stream_index == audio_stream_idx) {
			AVFrame *audio_frame = av_frame_alloc();
			if (avcodec_send_packet(audio_dec_ctx, &pkt) >= 0) {
				while (avcodec_receive_frame(audio_dec_ctx, audio_frame) >= 0) {
					if (wav_file && audio_frame->nb_samples > 0) {
						// Convert audio to 16-bit PCM and write to WAV file
						int samples_per_channel = audio_frame->nb_samples;
						int num_channels = audio_frame->ch_layout.nb_channels;

						// Convert audio samples to 16-bit PCM
						for (int i = 0; i < samples_per_channel; i++) {
							for (int ch = 0; ch < num_channels; ch++) {
								int16_t sample = 0;

								// Handle different audio formats
								switch (audio_frame->format) {
									case AV_SAMPLE_FMT_S16:
									case AV_SAMPLE_FMT_S16P:
									{
										int16_t *data = (int16_t*)audio_frame->data[audio_frame->format == AV_SAMPLE_FMT_S16P ? ch : 0];
										sample = data[audio_frame->format == AV_SAMPLE_FMT_S16P ? i : i * num_channels + ch];
										break;
									}
									case AV_SAMPLE_FMT_FLT:
									case AV_SAMPLE_FMT_FLTP:
									{
										float *data = (float*)audio_frame->data[audio_frame->format == AV_SAMPLE_FMT_FLTP ? ch : 0];
										float fval = data[audio_frame->format == AV_SAMPLE_FMT_FLTP ? i : i * num_channels + ch];
										sample = (int16_t)(fval * 32767.0f);
										break;
									}
									case AV_SAMPLE_FMT_S32:
									case AV_SAMPLE_FMT_S32P:
									{
										int32_t *data = (int32_t*)audio_frame->data[audio_frame->format == AV_SAMPLE_FMT_S32P ? ch : 0];
										int32_t ival = data[audio_frame->format == AV_SAMPLE_FMT_S32P ? i : i * num_channels + ch];
										sample = (int16_t)(ival >> 16);
										break;
									}
									default:
										sample = 0; // Unsupported format, write silence
								}

								fwrite(&sample, sizeof(int16_t), 1, wav_file);
								audio_samples_written++;
							}
						}
					}
				}
			}
			av_frame_free(&audio_frame);
			av_packet_unref(&pkt);
			continue;
		}

		if (pkt.stream_index != source_stream_idx) {
			av_packet_unref(&pkt);
			continue;
		}
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

        av_freep(&nv12frame->data[0]);
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

    // Finalize WAV file if audio was processed
    if (wav_file) {
    	// Update WAV header with correct file size
    	fseek(wav_file, 0, SEEK_SET);
    	uint32_t file_size = (uint32_t)(44 + audio_samples_written * 2 - 8); // Total file size minus 8
    	uint32_t data_size = (uint32_t)(audio_samples_written * 2); // Data chunk size

    	fseek(wav_file, 4, SEEK_SET);
    	fwrite(&file_size, sizeof(uint32_t), 1, wav_file);
    	fseek(wav_file, 40, SEEK_SET);
    	fwrite(&data_size, sizeof(uint32_t), 1, wav_file);

    	fclose(wav_file);

    	// Rename WAV file to final name
    	std::string final_wav_path = remove_extension(binel->path) + "_hap.wav";
    	rename(wav_temp_path.c_str(), final_wav_path.c_str());
    }

    // Clean up audio resources
    if (audio_dec_ctx) {
    	avcodec_free_context(&audio_dec_ctx);
    }

    avcodec_free_context(&c);
    av_frame_free(&nv12frame);
    av_frame_free(&decframe);
    av_packet_unref(&pkt);
    avcodec_free_context(&source_dec_ctx);
    avformat_close_input(&source);
    binel->path = remove_extension(binel->path) + "_hap.mov";
	rename(destpath, binel->path);
    if (mainprogram->stashvideos) {
        if (!exists(mainprogram->contentpath + "EWOCvj2_CPU_vid_backups")) {
            std::filesystem::path d{mainprogram->contentpath + "EWOCvj2_CPU_vid_backups"};
            std::filesystem::create_directory(d);
        }
        std::filesystem::path d2{mainprogram->contentpath + "EWOCvj2_CPU_vid_backups/" + basename(dirname(srcpath))};
        std::filesystem::create_directory(d2);
        copy_file(srcpath,
                                   mainprogram->contentpath + "EWOCvj2_CPU_vid_backups/" + basename(dirname(srcpath)) +
                                   "/" + basename(srcpath),
                                   std::filesystem::copy_options::overwrite_existing);
        mainprogram->remove(srcpath);
    }
    binel->encoding = false;
    if (binel->otflay) {
        bool bukeb = binel->otflay->keepeffbut->value;
        binel->otflay->keepeffbut->value = 1;
        binel->otflay->transfered = true;
        binel->otflay = binel->otflay->open_video(binel->otflay->frame, binel->path, false);
        binel->otflay->keepeffbut->value = bukeb;
        // wait for video open
        std::unique_lock<std::mutex> olock(binel->otflay->endopenlock);
        binel->otflay->endopenvar.wait(olock, [&] {return binel->otflay->opened; });
        binel->otflay->opened = false;
        olock.unlock();
        binel->otflay->set_clones();
        binel->otflay->scritched = true;
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

void BinsMain::clear_undo() {
    for (int i = 0; i < binsmain->undobins.size(); i++) {
        auto bin = std::get<0>(binsmain->undobins[i]);
        for (auto binel : bin->elements) {
            delete binel;
        }
        delete bin;
    }
    binsmain->undobins.clear();
    binsmain->undopos = 0;
    bool bubs = mainprogram->binsscreen;
    mainprogram->binsscreen = true;
    mainprogram->undo_redo_save();
    mainprogram->binsscreen = bubs;
}

void BinsMain::undo_redo(char offset) {
    for (int i = 0; i < binsmain->bins.size(); i++) {
        if (binsmain->bins[i]->name ==
            std::get<1>(binsmain->undobins[binsmain->undopos + offset])) {
            binsmain->make_currbin(i);
            Bin *bubin = binsmain->bins[i];
            binsmain->bins[i] = std::get<0>(binsmain->undobins[binsmain->undopos + offset]);
            binsmain->bins[i]->pos = i;
            binsmain->make_currbin(i);
            break;
        }
    }
}