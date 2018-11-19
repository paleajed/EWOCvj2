#include <boost/filesystem.hpp>

#include "GL/glew.h"
#include "GL/gl.h"
#include "GL/glut.h"
#ifdef __linux__
#include "GL/glx.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include "paths.h"
#endif

#include "SDL2/SDL.h"
#include "SDL2/SDL_syswm.h"

#include "tinyfiledialogs.h"

#include <istream>
#include <ostream>
#include <iostream>
#include <string>

// my own headers
#include "box.h"
#include "effect.h"
#include "node.h"
#include "layer.h"
#include "window.h"
#include "program.h"
#include "loopstation.h"
#include "bins.h"

Program::Program() {
	#ifdef _WIN64
	this->temppath = "./temp/";
	#else
	#ifdef __linux__
	std::string homedir (getenv("HOME"));
	this->temppath = homedir + "/.ewocvj2/temp/";
	#endif
	#endif
	
	this->numh = this->numh * glob->w / glob->h;
	
	this->scrollboxes[0] = new Box;
	this->scrollboxes[0]->vtxcoords->x1 = -1.0f + this->numw;
	this->scrollboxes[0]->vtxcoords->y1 = 1.0f - this->layw - 0.05f;
	this->scrollboxes[0]->vtxcoords->w = this->layw * 3.0f;
	this->scrollboxes[0]->vtxcoords->h = 0.05f;
	this->scrollboxes[0]->upvtxtoscr();
	this->scrollboxes[0]->tooltiptitle = "Deck A Layer stack scroll bar ";
	this->scrollboxes[0]->tooltip = "Scroll bar for layer stack.  Divided in total number of layers parts for deck A. Lightgrey part is the visible layer monitors part.  Leftdragging the lightgrey part allows scrolling.  So does leftclicking the darkgrey parts. ";
	this->scrollboxes[1] = new Box;
	this->scrollboxes[1]->vtxcoords->x1 = this->numw;
	this->scrollboxes[1]->vtxcoords->y1 = 1.0f - this->layw - 0.05f;
	this->scrollboxes[1]->vtxcoords->w = this->layw * 3.0f;
	this->scrollboxes[1]->vtxcoords->h = 0.05f;
	this->scrollboxes[1]->upvtxtoscr();
	this->scrollboxes[1]->tooltiptitle = "Deck b Layer stack scroll bar ";
	this->scrollboxes[1]->tooltip = "Scroll bar for layer stack.  Divided in total number of layers parts for deck B. Lightgrey part is the visible layer monitors part.  Leftdragging the lightgrey part allows scrolling.  So does leftclicking the darkgrey parts. ";
	
	this->cwbox = new Box;

	this->toscreen = new Button(false);
	this->buttons.push_back(this->toscreen);
	this->toscreen->name[0] = "SEND";
	this->toscreen->name[1] = "SEND";
	this->toscreen->box->vtxcoords->x1 = -0.3;
	this->toscreen->box->vtxcoords->y1 = -0.3;
	this->toscreen->box->vtxcoords->w = 0.3 / 2.0;
	this->toscreen->box->vtxcoords->h = 0.3 / 3.0;
	this->toscreen->box->upvtxtoscr();
	this->toscreen->box->tooltiptitle = "Send preview streams to output ";
	this->toscreen->box->tooltip = "Leftclick sends/copies the streams being previewed to the output streams ";
	
	this->backtopre = new Button(false);
	this->buttons.push_back(this->backtopre);
	this->backtopre->name[0] = "SEND";
	this->backtopre->name[1] = "SEND";
	this->backtopre->box->vtxcoords->x1 = -0.3;
	this->backtopre->box->vtxcoords->y1 = -0.4;
	this->backtopre->box->vtxcoords->w = 0.15f;
	this->backtopre->box->vtxcoords->h = 0.1f;
	this->backtopre->box->upvtxtoscr();
	this->backtopre->box->tooltiptitle = "Send output streams to preview ";
	this->backtopre->box->tooltip = "Leftclick sends/copies the streams being output back into the preview streams ";
	
	this->modusbut = new Button(false);
	this->buttons.push_back(this->modusbut);
	this->modusbut->name[0] = "LIVE MODUS";
	this->modusbut->name[1] = "PREVIEW MODUS";
	this->modusbut->box->vtxcoords->x1 = -0.3 - (0.3 / 2.0);
	this->modusbut->box->vtxcoords->y1 = -0.7;
	this->modusbut->box->vtxcoords->w = 0.3 / 2.0;
	this->modusbut->box->vtxcoords->h = 0.3 / 3.0;
	this->modusbut->box->upvtxtoscr();
	this->modusbut->box->tooltiptitle = "LIVE/PREVIEW modus switch ";
	this->modusbut->box->tooltip = "Leftclicking toggles between preview modus (changes are previewed, then sent to/from output) and live modus (changes appear in output immediately) ";
	this->modusbut->tcol[0] = 0.0f;
	this->modusbut->tcol[1] = 0.0f;
	this->modusbut->tcol[2] = 0.0f;
	this->modusbut->tcol[3] = 1.0f;
	
	this->deckspeed[0] = new Param;
	this->deckspeed[0]->name = "Speed A"; 
	this->deckspeed[0]->value = 1.0f;
	this->deckspeed[0]->range[0] = 0.0f;
	this->deckspeed[0]->range[1] = 3.33f;
	this->deckspeed[0]->sliding = true;
	this->deckspeed[0]->powertwo = true;
	this->deckspeed[0]->box->vtxcoords->x1 = -0.3 - (0.3 / 2.0) - 0.15f;
	this->deckspeed[0]->box->vtxcoords->y1 = -0.7;
	this->deckspeed[0]->box->vtxcoords->w = 0.3 / 2.0;
	this->deckspeed[0]->box->vtxcoords->h = 0.3 / 3.0;
	this->deckspeed[0]->box->upvtxtoscr();
	this->deckspeed[0]->box->tooltiptitle = "Global deck A speed setting ";
	this->deckspeed[0]->box->tooltip = "Change global deck A speed factor. Leftdrag changes value. Doubleclick allows numeric entry. ";
	
	this->deckspeed[1] = new Param;
	this->deckspeed[1]->name = "Speed B"; 
	this->deckspeed[1]->value = 1.0f;
	this->deckspeed[1]->range[0] = 0.0f;
	this->deckspeed[1]->range[1] = 3.33f;
	this->deckspeed[1]->sliding = true;
	this->deckspeed[1]->powertwo = true;
	this->deckspeed[1]->box->vtxcoords->x1 = -0.3 - (0.3 / 2.0) + 0.9f;
	this->deckspeed[1]->box->vtxcoords->y1 = -0.7;
	this->deckspeed[1]->box->vtxcoords->w = 0.3 / 2.0;
	this->deckspeed[1]->box->vtxcoords->h = 0.3 / 3.0;
	this->deckspeed[1]->box->upvtxtoscr();
	this->deckspeed[1]->box->tooltiptitle = "Global deck B speed setting ";
	this->deckspeed[1]->box->tooltip = "Change global deck B speed factor. Leftdrag changes value. Doubleclick allows numeric entry. ";
	
	this->outputmonitor = new Box;
	this->outputmonitor->vtxcoords->x1 = -0.15f;
	this->outputmonitor->vtxcoords->y1 = -0.4f + tf(0.05f);
	this->outputmonitor->vtxcoords->w = 0.3f;
	this->outputmonitor->vtxcoords->h = 0.3f;
	this->outputmonitor->upvtxtoscr();
	this->outputmonitor->tooltiptitle = "Output monitor ";
	this->outputmonitor->tooltip = "Shows mix of output stream when in preview modus.  Rightclick menu allows sending monitor image fullscreen to another display device. ";

	this->mainmonitor = new Box;
	this->mainmonitor->vtxcoords->x1 = -0.3f;
	this->mainmonitor->vtxcoords->y1 = -1.0f;
	this->mainmonitor->vtxcoords->w = 0.6f;
	this->mainmonitor->vtxcoords->h = 0.6f;
	this->mainmonitor->upvtxtoscr();
	this->mainmonitor->tooltiptitle = "A+B monitor ";
	this->mainmonitor->tooltip = "Shows crossfaded or wiped A+B mix of preview stream (preview modus) or output stream (live modus).  Rightclick menu allows setting wipes and sending monitor image fullscreen to another display device. Leftclick/drag on image affects some wipe modes. ";

	this->deckmonitor[0] = new Box;
	this->deckmonitor[0]->vtxcoords->x1 = -0.6f;
	this->deckmonitor[0]->vtxcoords->y1 = -1.0f;
	this->deckmonitor[0]->vtxcoords->w = 0.3f;
	this->deckmonitor[0]->vtxcoords->h = 0.3f;
	this->deckmonitor[0]->upvtxtoscr();
	this->deckmonitor[0]->tooltiptitle = "Deck A monitor ";
	this->deckmonitor[0]->tooltip = "Shows deck A stream.  Rightclick menu allows sending monitor image fullscreen to another display device. Leftclick/drag on image affects some local layer wipe modes. ";

	this->deckmonitor[1] = new Box;
	this->deckmonitor[1]->vtxcoords->x1 = 0.3f;
	this->deckmonitor[1]->vtxcoords->y1 = -1.0f;
	this->deckmonitor[1]->vtxcoords->w = 0.3f;
	this->deckmonitor[1]->vtxcoords->h = 0.3f;
	this->deckmonitor[1]->upvtxtoscr();
	this->deckmonitor[1]->tooltiptitle = "Deck B monitor ";
	this->deckmonitor[1]->tooltip = "Shows deck B stream.  Rightclick menu allows sending monitor image fullscreen to another display device. Leftclick/drag on image affects some local layer wipe modes. ";

	this->effcat[0] = new Button(false);
	this->buttons.push_back(this->effcat[0]);
	this->effcat[0]->name[0] = "Layer effects";
	this->effcat[0]->name[1] = "Stream effects";
	this->effcat[0]->box->vtxcoords->x1 = -1.0f + this->numw - tf(0.025f);
	this->effcat[0]->box->vtxcoords->y1 = 1.0f - tf(this->layw) - tf(0.50f);
	this->effcat[0]->box->vtxcoords->w = tf(0.025f);
	this->effcat[0]->box->vtxcoords->h = tf(0.2f);
	this->effcat[0]->box->upvtxtoscr();
	this->effcat[0]->box->tooltiptitle = "Layer/stream effects toggle ";
	this->effcat[0]->box->tooltip = "Leftclick toggles between layer effects queue (these effects only affect the current layer) and stream effects queue (effects affect mix of all layers upto and including the current one). ";
	
	this->effcat[1] = new Button(false);
	this->buttons.push_back(this->effcat[1]);
	this->effcat[1]->name[0] = "Layer effects";
	this->effcat[1]->name[1] = "Stream effects";
	float xoffset = 1.0f + this->layw - 0.019f;
	this->effcat[1]->box->vtxcoords->x1 = -1.0f + this->numw - tf(0.025f) + xoffset;
	this->effcat[1]->box->vtxcoords->y1 = 1.0f - tf(this->layw) - tf(0.50f);
	this->effcat[1]->box->vtxcoords->w = tf(0.025f);
	this->effcat[1]->box->vtxcoords->h = tf(0.2f);
	this->effcat[1]->box->upvtxtoscr();
	this->effcat[1]->box->tooltiptitle = "Layer/stream effects toggle ";
	this->effcat[1]->box->tooltip = "Leftclick toggles between layer effects queue (these effects only affect the current layer) and stream effects queue (effects affect mix of all layers upto and including the current one). ";
	
	this->effscrollupA = new Box;
	this->effscrollupA->vtxcoords->x1 = -1.0;
	this->effscrollupA->vtxcoords->y1 = 1.0 - tf(this->layw) - tf(0.20f);
	this->effscrollupA->vtxcoords->w = tf(0.025f);
	this->effscrollupA->vtxcoords->h = tf(0.05f);
	this->effscrollupA->upvtxtoscr();
	this->effscrollupA->tooltiptitle = "Scroll effects queue up ";
	this->effscrollupA->tooltip = "Leftclicking scrolls the effect queue up ";
	
	this->effscrollupB = new Box;
	this->effscrollupB->vtxcoords->x1 = 1.0 - tf(0.05f);
	this->effscrollupB->vtxcoords->y1 = 1.0 - tf(this->layw) - tf(0.20f);
	this->effscrollupB->vtxcoords->w = tf(0.025f);
	this->effscrollupB->vtxcoords->h = tf(0.05f);     
	this->effscrollupB->upvtxtoscr();
	this->effscrollupB->tooltiptitle = "Scroll effects queue up ";
	this->effscrollupB->tooltip = "Leftclicking scrolls the effect queue up ";
	
	this->effscrolldownA = new Box;
	this->effscrolldownA->vtxcoords->x1 = -1.0;
	this->effscrolldownA->vtxcoords->y1 = 1.0 - tf(this->layw) - tf(0.20f) - tf(0.05f) * 10;
	this->effscrolldownA->vtxcoords->w = tf(0.025f);
	this->effscrolldownA->vtxcoords->h = tf(0.05f);
	this->effscrolldownA->upvtxtoscr();
	this->effscrolldownA->tooltiptitle = "Scroll effects queue down ";
	this->effscrolldownA->tooltip = "Leftclicking scrolls the effect queue down ";
	
	this->effscrolldownB = new Box;
	this->effscrolldownB->vtxcoords->x1 = 1.0 - tf(0.05f);
	this->effscrolldownB->vtxcoords->y1 = 1.0 - tf(this->layw) - tf(0.20f) - tf(0.05f) * 10;
	this->effscrolldownB->vtxcoords->w = tf(0.025f);
	this->effscrolldownB->vtxcoords->h = tf(0.05f);
	this->effscrolldownB->upvtxtoscr();
	this->effscrolldownB->tooltiptitle = "Scroll effects queue down ";
	this->effscrolldownB->tooltip = "Leftclicking scrolls the effect queue down ";
	
	this->addeffectbox = new Box;
	this->addeffectbox->vtxcoords->w = tf(this->layw);
	this->addeffectbox->vtxcoords->h = tf(0.038f);
	this->addeffectbox->tooltiptitle = "Add effect ";
	this->addeffectbox->tooltip = "Add effect to end of layer effect queue ";
	
	this->tmplay = new Box;
	this->tmplay->vtxcoords->x1 = 0.075;
	this->tmplay->vtxcoords->y1 = -0.9f;
	this->tmplay->vtxcoords->w = 0.15f;
	this->tmplay->vtxcoords->h = 0.26f;
	this->tmplay->tooltiptitle = "Set MIDI for play button ";
	this->tmplay->tooltip = "Leftclick to start waiting for a MIDI command that will trigger normal video play for this preset. ";
	this->tmbackw = new Box;
	this->tmbackw->vtxcoords->x1 = -0.225f;
	this->tmbackw->vtxcoords->y1 = -0.9f;
	this->tmbackw->vtxcoords->w = 0.15f;
	this->tmbackw->vtxcoords->h = 0.26f;
	this->tmbackw->tooltiptitle = "Set MIDI for reverse play button ";
	this->tmbackw->tooltip = "Leftclick to start waiting for a MIDI command that will trigger reverse video play for this preset. ";
	this->tmbounce = new Box;
	this->tmbounce->vtxcoords->x1 = -0.075f;
	this->tmbounce->vtxcoords->y1 = -0.9f;
	this->tmbounce->vtxcoords->w = 0.15f;
	this->tmbounce->vtxcoords->h = 0.26f;
	this->tmbounce->tooltiptitle = "Set MIDI for bounce play button ";
	this->tmbounce->tooltip = "Leftclick to start waiting for a MIDI command that will trigger bounce video play for this preset. ";
	this->tmfrforw = new Box;
	this->tmfrforw->vtxcoords->x1 = 0.225f;
	this->tmfrforw->vtxcoords->y1 = -0.9f;
	this->tmfrforw->vtxcoords->w = 0.15;
	this->tmfrforw->vtxcoords->h = 0.26f;
	this->tmfrforw->tooltiptitle = "Set MIDI for frame forward button ";
	this->tmfrforw->tooltip = "Leftclick to start waiting for a MIDI command that will trigger frame forward for this preset. ";
	this->tmfrbackw = new Box;
	this->tmfrbackw->vtxcoords->x1 = -0.375f;
	this->tmfrbackw->vtxcoords->y1 = -0.9f;
	this->tmfrbackw->vtxcoords->w = 0.15;
	this->tmfrbackw->vtxcoords->h = 0.26f;
	this->tmfrbackw->tooltiptitle = "Set MIDI for frame backward button ";
	this->tmfrbackw->tooltip = "Leftclick to start waiting for a MIDI command that will trigger frame backward for this preset. ";
	this->tmspeed = new Box;
	this->tmspeed->vtxcoords->x1 = -0.8f;
	this->tmspeed->vtxcoords->y1 = -0.5f;
	this->tmspeed->vtxcoords->w = 0.2f;
	this->tmspeed->vtxcoords->h = 1.0f;
	this->tmspeed->tooltiptitle = "Set MIDI for setting video play speed ";
	this->tmspeed->tooltip = "Leftclick to start waiting for a MIDI command that will set the play speed factor for this preset. ";
	this->tmspeedzero = new Box;
	this->tmspeedzero->vtxcoords->x1 = -0.775f;
	this->tmspeedzero->vtxcoords->y1 = -0.1f;
	this->tmspeedzero->vtxcoords->w = 0.15f;
	this->tmspeedzero->vtxcoords->h = 0.2f;
	this->tmspeedzero->tooltiptitle = "Set MIDI for setting video play speed to one";
	this->tmspeedzero->tooltip = "Leftclick to start waiting for a MIDI command that will set the play speed factor back to one for this preset. ";
	this->tmopacity = new Box;
	this->tmopacity->vtxcoords->x1 = 0.6f;
	this->tmopacity->vtxcoords->y1 = -0.5f;
	this->tmopacity->vtxcoords->w = 0.2f;
	this->tmopacity->vtxcoords->h = 1.0f;
	this->tmopacity->tooltiptitle = "Set MIDI for setting video opacity ";
	this->tmopacity->tooltip = "Leftclick to start waiting for a MIDI command that will set the opacity for this preset. ";
	this->tmfreeze = new Box;
	this->tmfreeze->vtxcoords->x1 = -0.1f;
	this->tmfreeze->vtxcoords->y1 = 0.1f;
	this->tmfreeze->vtxcoords->w = 0.2f;
	this->tmfreeze->vtxcoords->h = 0.2f;
	this->tmfreeze->tooltiptitle = "Set MIDI for scratch wheel freeze ";
	this->tmfreeze->tooltip = "Leftclick to start waiting for a MIDI command that will trigger scratch wheel freeze for this preset. ";
	this->tmscratch = new Box;
	this->tmscratch->vtxcoords->x1 = -1.0f;
	this->tmscratch->vtxcoords->y1 = -1.0f;
	this->tmscratch->vtxcoords->w = 2.0f;
	this->tmscratch->vtxcoords->h = 2.0f;
	this->tmscratch->tooltiptitle = "Set MIDI for scratch wheel ";
	this->tmscratch->tooltip = "Leftclick to start waiting for a MIDI command that will trigger the scratch wheel  for this preset. ";
	
	this->wormhole = new Button(false);
	this->wormhole->box->tooltiptitle = "Screen switching wormhole ";
	this->wormhole->box->tooltip = "Connects mixing screen and media bins screen.  Leftclick to switch screen.  Drag content inside wormhole to travel to the other screen. ";
	this->buttons.push_back(this->wormhole);
}

void Program::make_menu(const std::string &name, Menu *&menu, std::vector<std::string> &entries) {
	bool found = false;
	for (int i = 0; i < mainprogram->menulist.size(); i++) {
		if (mainprogram->menulist[i]->name == name) {
			found = true;
			break;
		}
	}
	if (!found) {
		menu = new Menu;
		mainprogram->menulist.push_back(menu);
		menu->name = name;
	}
	menu->entries = entries;
	Box *box = new Box;
	menu->box = box;
	menu->box->scrcoords->x1 = 0;
	menu->box->scrcoords->y1 = mainprogram->yvtxtoscr(tf(0.05f));
	menu->box->scrcoords->w = mainprogram->xvtxtoscr(tf(0.156f));
	menu->box->scrcoords->h = mainprogram->yvtxtoscr(tf(0.05f));
	menu->box->upscrtovtx();
}

std::string Program::mime_to_wildcard(std::string filters) {
	if (filters == "") return "";
	if (filters == "application/ewocvj2-layer") return "*.layer";
	if (filters == "application/ewocvj2-deck") return "*.deck";
	if (filters == "application/ewocvj2-mix") return "*.mix";
	if (filters == "application/ewocvj2-state") return "*.state";
	if (filters == "application/ewocvj2-shelf") return "*.shelf";
}

void Program::get_inname(const char *title, std::string filters, std::string defaultdir) {
	char const* const dd = (defaultdir == "") ? "" : defaultdir.c_str();
	#ifdef _WIN64
	filters = this->mime_to_wildcard(filters);
	#endif
	const char* fi[1];
	fi[0] = filters.c_str();
	if (fi[0] == "") {
		this->path = tinyfd_openFileDialog(title, dd, 0, nullptr, nullptr, 0);
	}
	else {
		this->path = tinyfd_openFileDialog(title, dd, 1, fi, nullptr, 0);
	}
}

void Program::get_outname(const char *title, std::string filters, std::string defaultdir) {
	char const* const dd = (defaultdir == "") ? "" : defaultdir.c_str();
	#ifdef _WIN64
	filters = this->mime_to_wildcard(filters);
	#endif
	const char* fi[1];
	fi[0] = filters.c_str();
	if (fi[0] == "") {
		this->path = tinyfd_saveFileDialog(title, dd, 0, nullptr, nullptr);
	}
	else {
		this->path = tinyfd_saveFileDialog(title, dd, 1, fi, nullptr);
	}
}

void Program::get_multinname(const char* title) {
	const char *outpaths;
	outpaths = tinyfd_openFileDialog(title, "", 0, nullptr, nullptr, 1);
	if (outpaths == nullptr) {
		binsmain->openbinfile = false;
		return;
	}
	std::string opaths(outpaths);
	std::string currstr = "";
	std::string charstr;
	for (int i = 0; i < opaths.length(); i++) {
		charstr = opaths[i];
		if (charstr == "|") {
			this->paths.push_back(currstr);
			std::string currstr = "";
			continue;
		}
		currstr += charstr;
		if (i == opaths.length() - 1) this->paths.push_back(currstr);
	}
	this->path = (char*)"ENTER";
	this->counting = 0;
}

void Program::get_dir(const char *title) {
	this->path = tinyfd_selectFolderDialog(title, "") ;
}



float Program::xscrtovtx(float scrcoord) {
	return (scrcoord * 2.0 / (float)glob->w);
}

float Program::yscrtovtx(float scrcoord) {
	return (scrcoord * 2.0 / (float)glob->h);
}

float Program::xvtxtoscr(float vtxcoord) {
	return (vtxcoord * (float)glob->w / 2.0);
}

float Program::yvtxtoscr(float vtxcoord) {
	return (vtxcoord * (float)glob->h / 2.0);
}

/* A simple function that prints a message, the error code returned by SDL,
 * and quits the application */
void Program::quit(std::string msg)
{
	//save midi map
	save_genmidis("./midiset.gm");
	//empty temp dir
	boost::filesystem::path path_to_remove(mainprogram->temppath);
	for (boost::filesystem::directory_iterator end_dir_it, it(path_to_remove); it!=end_dir_it; ++it) {
		boost::filesystem::remove_all(it->path());
	}
	printf("%s: %s\n", msg.c_str(), SDL_GetError());
    printf("stopped\n");

    SDL_Quit();
    exit(1);
}

void Program::preveff_init() {
	std::vector<Layer*> &lvec = choose_layers(mainmix->currlay->deck);
	if (!this->prevmodus) {
		// normally all comp layers are copied from normal layers
		// but when going into performance mode, its more coherent to open them in comp layers
		for (int i = 0; i < mainmix->layersA.size(); i++) {
			Layer *lay = mainmix->layersA[i];
			if (lay->filename != "") {
				Layer *laycomp = mainmix->layersAcomp[i];
				laycomp->audioplaying = false;
				open_video(lay->frame, laycomp, lay->filename, true);
			}
		}
		for (int i = 0; i < mainmix->layersB.size(); i++) {
			Layer *lay = mainmix->layersB[i];
			if (lay->filename != "") {
				Layer *laycomp = mainmix->layersBcomp[i];
				laycomp->audioplaying = false;
				open_video(lay->frame, laycomp, lay->filename, true);
			}
		}
	}
	int p = mainmix->currlay->pos;
	if (p > lvec.size() - 1) p = lvec.size() - 1;
	mainmix->currlay = lvec[p];
	GLint preff = glGetUniformLocation(this->ShaderProgram, "preff");
	glUniform1i(preff, this->prevmodus);
}

unsigned long getFileLength(std::ifstream& file)
{
    if(!file.good()) return 0;

    unsigned long pos=file.tellg();
    file.seekg(0,std::ios::end);
    unsigned long len = file.tellg();
    file.seekg(std::ios::beg);

    return len;
}

int Program::load_shader(char* filename, char** ShaderSource, unsigned long len)
{
   std::ifstream file;
   file.open(filename, std::ios::in); // opens as ASCII!
   if(!file) return -1;

   len = getFileLength(file);

   if (len==0) return -2;   // Error: Empty File

   *ShaderSource = (char*)malloc(len+1);
   if (*ShaderSource == 0) return -3;   // can't reserve memory

    // len isn't always strlen cause some characters are stripped in ascii read...
    // it is important to 0-terminate the real length later, len is just max possible value...
   (*ShaderSource)[len] = 0;

   unsigned int i=0;
   while (file.good())
   {
       (*ShaderSource)[i] = file.get();       // get character from file.
       if (!file.eof())
        i++;
   }

   (*ShaderSource)[i] = 0;  // 0-terminate it at the correct position

   file.close();

   return 0; // No Error
}

GLuint Program::set_shader() {
	GLuint program;
	GLuint vertexShaderObject = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderObject = glCreateShader(GL_FRAGMENT_SHADER);
	unsigned long vlen, flen;
	char *VShaderSource;
 	char *vshader = (char*)malloc(100);
 	#ifdef _WIN64
 	if (exists("./shader.vs")) strcpy (vshader, "./shader.vs");
 	else mainprogram->quit("Unable to find vertex shader \"shader.vs\" in current directory");
 	#else
 	#ifdef __linux__
 	std::string ddir (DATADIR);
 	if (exists("./shader.vs")) strcpy (vshader, "./shader.vs");
 	else if (exists(ddir + "/shader.vs")) strcpy (vshader, (ddir + "/shader.vs").c_str());
 	else mainprogram->quit("Unable to find vertex shader \"shader.vs\" in " + ddir);
 	#endif
 	#endif
 	load_shader(vshader, &VShaderSource, vlen);
	char *FShaderSource;
 	char *fshader = (char*)malloc(100);
 	#ifdef _WIN64
 	if (exists("./shader.fs")) strcpy (fshader, "./shader.fs");
 	else mainprogram->quit("Unable to find fragment shader \"shader.fs\" in current directory");
 	#else
 	#ifdef __linux__
 	if (exists("./shader.fs")) strcpy (fshader, "./shader.fs");
 	else if (exists(ddir + "/shader.fs")) strcpy (fshader, (ddir + "/shader.fs").c_str());
 	else mainprogram->quit("Unable to find fragment shader \"shader.fs\" in " + ddir);
 	#endif
 	#endif
	load_shader(fshader, &FShaderSource, flen);
	glShaderSource(vertexShaderObject, 1, &VShaderSource, nullptr);
	glShaderSource(fragmentShaderObject, 1, &FShaderSource, nullptr);
	glCompileShader(vertexShaderObject);
	glCompileShader(fragmentShaderObject);

	GLint maxLength = 0;
	glGetShaderiv(fragmentShaderObject, GL_INFO_LOG_LENGTH, &maxLength);
 	GLchar *infolog = (GLchar*)malloc(maxLength);
	glGetShaderInfoLog(fragmentShaderObject, maxLength, &maxLength, &(infolog[0]));
	printf("compile log %s\n", infolog);

	program = glCreateProgram();
	glBindAttribLocation(program, 0, "Position");
	glBindAttribLocation(program, 1, "TexCoord");
	glAttachShader(program, vertexShaderObject);
	glAttachShader(program, fragmentShaderObject);
	glLinkProgram(program);

	maxLength = 1024;
 	infolog = (GLchar*)malloc(maxLength);
	glGetProgramInfoLog(program, maxLength, &maxLength, &(infolog[0]));
	printf("linker log %s\n", infolog);

	GLint isLinked = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
	printf("log %d\n", isLinked);
	fflush(stdout);
	
	return program;
}



