#if defined(WIN32) && !defined(__linux__)
#define WINDOWS
#elif defined(__linux__) && !defined(WIN32)
#define POSIX
#endif

#include "snappy-c.h"

#include "IL/il.h"
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

#include "GL/glew.h"
#include "GL/gl.h"
#define FREEGLUT_STATIC
#define _LIB
#define FREEGLUT_LIB_PRAGMAS 0
#include "GL/freeglut.h"

#include <ostream>
#include <fstream>
#include <ios>
#include <list>
#include <map>
#include <filesystem>

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
#include "libswscale/swscale.h"
#include "libavutil/avutil.h"
#include "libavutil/pixfmt.h"
#include "libavutil/dict.h"
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
#include "retarget.h"



std::mutex databufmutex;



// BASIC MIXER METHODS

Mixer::Mixer() {
	this->decknamebox[0] = new Boxx;
	this->decknamebox[0]->lcolor[0] = 1.0;
	this->decknamebox[0]->lcolor[1] = 0.5;
	this->decknamebox[0]->lcolor[2] = 0.5;
	this->decknamebox[0]->lcolor[3] = 1.0;
	this->decknamebox[0]->vtxcoords->x1 = -1;
	this->decknamebox[0]->vtxcoords->y1 = 1 - mainprogram->numh;
	this->decknamebox[0]->vtxcoords->w = mainprogram->numw;
	this->decknamebox[0]->vtxcoords->h = mainprogram->numh;
	this->decknamebox[0]->upvtxtoscr();
	this->decknamebox[1] = new Boxx;
	this->decknamebox[1]->lcolor[0] = 1.0;
	this->decknamebox[1]->lcolor[1] = 0.5;
	this->decknamebox[1]->lcolor[2] = 0.5;
	this->decknamebox[1]->lcolor[3] = 1.0;
	this->decknamebox[1]->vtxcoords->x1 = 0;
	this->decknamebox[1]->vtxcoords->y1 = 1 - mainprogram->numh;
	this->decknamebox[1]->vtxcoords->w = mainprogram->numw;
	this->decknamebox[1]->vtxcoords->h = mainprogram->numh;
	this->decknamebox[1]->upvtxtoscr();
    for (int j = 0; j < 2; j++) {
        for (int i = 0; i < 4; i++) {
            Scene* scene = new Scene;
            scene->deck = j;
            this->scenes[j].push_back(scene);
            Boxx* box = new Boxx;
            Button* button = new Button(false);
            mainprogram->buttons[23 + i + j * 4] = button;
            button->toggle = 0;
            scene->box = box;
            scene->button = button;
            box->tooltiptitle = "Scene toggle deck A";
            box->tooltip = "Leftclick activates one of the four scenes (separate layer stacks) for deck A. ";
            box->lcolor[0] = 0.4f;
            box->lcolor[1] = 0.4f;
            box->lcolor[2] = 0.4f;
            box->lcolor[3] = 1.0f;
            box->vtxcoords->x1 = -1 + j;
            box->vtxcoords->y1 = 1 - (i + 2) * mainprogram->numh;
            box->vtxcoords->w = mainprogram->numw;
            box->vtxcoords->h = mainprogram->numh;
            box->upvtxtoscr();
        }
    }

	this->wipex[0] = new Param;
    this->wipex[0]->name = "wipex";
    this->wipex[0]->sliding = true;
    this->wipex[0]->value = 0.5f;
    this->wipex[0]->range[0] = 0.0f;
    this->wipex[0]->range[1] = 1.0f;
	this->wipex[0]->shadervar = "xpos";
	this->wipey[0] = new Param;
    this->wipey[0]->name = "wipey";
    this->wipey[0]->sliding = true;
    this->wipey[0]->value = 0.5f;
    this->wipey[0]->range[0] = 1.0f;
    this->wipey[0]->range[1] = 0.0f;
	this->wipey[0]->shadervar = "ypos";
	lp->allparams.push_back(this->wipex[0]);
	lp->allparams.push_back(this->wipey[0]);
	this->wipex[1] = new Param;
    this->wipex[1]->name = "wipex";
    this->wipex[1]->sliding = true;
    this->wipex[1]->value = 0.5f;
    this->wipex[1]->range[0] = 0.0f;
    this->wipex[1]->range[1] = 1.0f;
	this->wipex[1]->shadervar = "xpos";
	this->wipey[1] = new Param;
    this->wipey[1]->name = "wipey";
    this->wipey[1]->sliding = true;
    this->wipey[1]->value = 0.5f;
    this->wipey[1]->range[0] = 1.0f;
    this->wipey[1]->range[1] = 0.0f;
	this->wipey[1]->shadervar = "ypos";
	lpc->allparams.push_back(this->wipex[1]);
	lpc->allparams.push_back(this->wipey[1]);

	this->modebox = new Boxx;
	this->modebox->vtxcoords->x1 = 0.85f;
	this->modebox->vtxcoords->y1 = -1.0f;
	this->modebox->vtxcoords->w = 0.15f;
	this->modebox->vtxcoords->h = 0.1f;
	this->modebox->upvtxtoscr();
	this->modebox->tooltiptitle = "Operation mode toggle ";
	this->modebox->tooltip = "Leftclick toggles between Basic and Node mode. ";
	
	this->genmidi[0] = new Button(false);
	this->genmidi[0]->value = 1;
	this->genmidi[0]->toggle = 4;
	this->genmidi[1] = new Button(false);
    this->genmidi[1]->value = 2;
	this->genmidi[1]->toggle = 4;
	this->genmidi[0]->box->acolor[0] = 0.5;
	this->genmidi[0]->box->acolor[1] = 0.2;
	this->genmidi[0]->box->acolor[2] = 0.5;
	this->genmidi[0]->box->acolor[3] = 1.0;
	this->genmidi[0]->box->vtxcoords->x1 = -1.0f;
	this->genmidi[0]->box->vtxcoords->y1 =  1 - 6 * mainprogram->numh;
	this->genmidi[0]->box->vtxcoords->w = mainprogram->numw;
	this->genmidi[0]->box->vtxcoords->h = mainprogram->numh;
	this->genmidi[0]->box->upvtxtoscr();
	this->genmidi[0]->box->tooltiptitle = "Deck A global MIDI preset ";
	this->genmidi[0]->box->tooltip = "Leftclick toggles between MIDI presets for deck A (A, B, C, D or off). ";
	this->genmidi[1]->box->acolor[0] = 0.5;
	this->genmidi[1]->box->acolor[1] = 0.2;
	this->genmidi[1]->box->acolor[2] = 0.5;
	this->genmidi[1]->box->acolor[3] = 1.0;
	this->genmidi[1]->box->vtxcoords->x1 = 0.0f;
	this->genmidi[1]->box->vtxcoords->y1 =  1 - 6 * mainprogram->numh;
	this->genmidi[1]->box->vtxcoords->w = mainprogram->numw;
	this->genmidi[1]->box->vtxcoords->h = mainprogram->numh;
	this->genmidi[1]->box->upvtxtoscr();
	this->genmidi[1]->box->tooltiptitle = "Deck B global MIDI preset ";
	this->genmidi[1]->box->tooltip = "Leftclick toggles between MIDI presets for deck B (A, B, C, D or off). ";
	
	this->crossfade = new Param;
	this->crossfade->name = "Crossfade"; 
	this->crossfade->value = 0.5f;
	this->crossfade->deflt = 0.5f;
	this->crossfade->range[0] = 0.0f;
	this->crossfade->range[1] = 1.0f;
	this->crossfade->sliding = true;
	this->crossfade->shadervar = "cf";
	this->crossfade->box->vtxcoords->x1 = -0.15f;
	this->crossfade->box->vtxcoords->y1 = -1.0f + mainprogram->monh * 2.0f;
	this->crossfade->box->vtxcoords->w = 0.3f;
	this->crossfade->box->vtxcoords->h = 0.075f;
	this->crossfade->box->upvtxtoscr();
	this->crossfade->box->tooltiptitle = "Crossfade ";
	this->crossfade->box->tooltip = "Leftdrag crossfades between deck A and deck B streams. Doubleclick allows numeric entry. ";
	this->crossfade->box->acolor[3] = 1.0f;
	lp->allparams.push_back(this->crossfade);
	this->crossfadecomp = new Param;
	this->crossfadecomp->name = "Crossfade"; 
	this->crossfadecomp->value = 0.5f;
	this->crossfadecomp->deflt = 0.5f;
	this->crossfadecomp->range[0] = 0.0f;
	this->crossfadecomp->range[1] = 1.0f;
	this->crossfadecomp->sliding = true;
	this->crossfadecomp->shadervar = "cf";
	this->crossfadecomp->box->vtxcoords->x1 = -0.15f;
	this->crossfadecomp->box->vtxcoords->y1 = -1.0f + mainprogram->monh * 2.0f;
	this->crossfadecomp->box->vtxcoords->w = 0.3f;
	this->crossfadecomp->box->vtxcoords->h = 0.075f;
	this->crossfadecomp->box->upvtxtoscr();
	this->crossfadecomp->box->tooltiptitle = "Crossfade ";
	this->crossfadecomp->box->tooltip = "Leftdrag crossfades between deck A and deck B streams. Doubleclick allows numeric entry. ";
	this->crossfadecomp->box->acolor[3] = 1.0f;
	lpc->allparams.push_back(this->crossfadecomp);

    /*this->recbutS = new Button(false);
    this->recbutS->name[0] = "S";
    this->recbutS->toggle = 1;
    this->recbutS->box->vtxcoords->x1 = 0.15f;
    this->recbutS->box->vtxcoords->y1 = -1.0f + mainprogram->monh * 2.0f;
    this->recbutS->box->vtxcoords->w = 0.0465f;
    this->recbutS->box->vtxcoords->h = 0.075f;
    this->recbutS->box->upvtxtoscr();
    this->recbutS->box->tooltiptitle = "Record output to video file in xvid codec. ";
    this->recbutS->box->tooltip = "Start/stop recording the output stream to an XviD video file in the recordings directory of the project.  Favours file size over quality.  Mainly for archival purposes. ";
*/
    this->recbutQ = new Button(false);
    this->recbutQ->name[0] = " .";
    this->recbutQ->toggle = 1;
    this->recbutQ->box->vtxcoords->x1 = 0.15f;
    this->recbutQ->box->vtxcoords->y1 = -1.0f + mainprogram->monh * 2.0f;
    this->recbutQ->box->vtxcoords->w = 0.0465f;
    this->recbutQ->box->vtxcoords->h = 0.075f;
    this->recbutQ->box->upvtxtoscr();
    this->recbutQ->box->tooltiptitle = "Record output to video file in hap codec. ";
    this->recbutQ->box->tooltip = "Start/stop recording the output stream to a HAP video file in the recordings directory of the project.  Favours quality over file size.  Mainly for remixing the outputted videos. ";

    this->deckspeed[0][0] = new Param;
	this->deckspeed[0][0]->name = "Speed A";
	this->deckspeed[0][0]->value = 1.0f;
	this->deckspeed[0][0]->deflt = 1.0f;
	this->deckspeed[0][0]->range[0] = 0.0f;
	this->deckspeed[0][0]->range[1] = 3.33f;
	this->deckspeed[0][0]->sliding = true;
	this->deckspeed[0][0]->powertwo = true;
	this->deckspeed[0][0]->box->vtxcoords->x1 = -0.3 - (0.3 / 2.0) - 0.15f;
	this->deckspeed[0][0]->box->vtxcoords->y1 = -1.0f + mainprogram->monh;
	this->deckspeed[0][0]->box->vtxcoords->w = 0.3 / 2.0;
	this->deckspeed[0][0]->box->vtxcoords->h = 0.3 / 3.0;
	this->deckspeed[0][0]->box->upvtxtoscr();
	this->deckspeed[0][0]->box->tooltiptitle = "Global preview deck A speed setting ";
	this->deckspeed[0][0]->box->tooltip = "Change global deck A speed factor for preview streams. Leftdrag changes value. Doubleclick allows numeric entry. ";
    lp->allparams.push_back(this->deckspeed[0][0]);

	this->deckspeed[0][1] = new Param;
	this->deckspeed[0][1]->name = "Speed B";
	this->deckspeed[0][1]->value = 1.0f;
	this->deckspeed[0][1]->deflt = 1.0f;
	this->deckspeed[0][1]->range[0] = 0.0f;
	this->deckspeed[0][1]->range[1] = 3.33f;
	this->deckspeed[0][1]->sliding = true;
	this->deckspeed[0][1]->powertwo = true;
	this->deckspeed[0][1]->box->vtxcoords->x1 = -0.3 - (0.3 / 2.0) + 0.9f;
	this->deckspeed[0][1]->box->vtxcoords->y1 = -1.0f + mainprogram->monh;
	this->deckspeed[0][1]->box->vtxcoords->w = 0.3 / 2.0;
	this->deckspeed[0][1]->box->vtxcoords->h = 0.3 / 3.0;
	this->deckspeed[0][1]->box->upvtxtoscr();
	this->deckspeed[0][1]->box->tooltiptitle = "Global preview deck B speed setting ";
	this->deckspeed[0][1]->box->tooltip = "Change global deck B speed factor for preview streams. Leftdrag changes value. Doubleclick allows numeric entry. ";
    lp->allparams.push_back(this->deckspeed[0][1]);

	this->deckspeed[1][0] = new Param;
	this->deckspeed[1][0]->name = "Speed A";
	this->deckspeed[1][0]->value = 1.0f;
	this->deckspeed[1][0]->deflt = 1.0f;
	this->deckspeed[1][0]->range[0] = 0.0f;
	this->deckspeed[1][0]->range[1] = 3.33f;
	this->deckspeed[1][0]->sliding = true;
	this->deckspeed[1][0]->powertwo = true;
	this->deckspeed[1][0]->box->vtxcoords->x1 = -0.3 - (0.3 / 2.0) - 0.15f;
	this->deckspeed[1][0]->box->vtxcoords->y1 = -1.0f + mainprogram->monh;
	this->deckspeed[1][0]->box->vtxcoords->w = 0.3 / 2.0;
	this->deckspeed[1][0]->box->vtxcoords->h = 0.3 / 3.0;
	this->deckspeed[1][0]->box->upvtxtoscr();
	this->deckspeed[1][0]->box->tooltiptitle = "Global preview deck A speed setting ";
	this->deckspeed[1][0]->box->tooltip = "Change global deck A speed factor for preview streams. Leftdrag changes value. Doubleclick allows numeric entry. ";
    lpc->allparams.push_back(this->deckspeed[1][0]);

	this->deckspeed[1][1] = new Param;
	this->deckspeed[1][1]->name = "Speed B";
	this->deckspeed[1][1]->value = 1.0f;
	this->deckspeed[1][1]->deflt = 1.0f;
	this->deckspeed[1][1]->range[0] = 0.0f;
	this->deckspeed[1][1]->range[1] = 3.33f;
	this->deckspeed[1][1]->sliding = true;
	this->deckspeed[1][1]->powertwo = true;
	this->deckspeed[1][1]->box->vtxcoords->x1 = -0.3 - (0.3 / 2.0) + 0.9f;
	this->deckspeed[1][1]->box->vtxcoords->y1 = -1.0f + mainprogram->monh;
	this->deckspeed[1][1]->box->vtxcoords->w = 0.3 / 2.0;
	this->deckspeed[1][1]->box->vtxcoords->h = 0.3 / 3.0;
	this->deckspeed[1][1]->box->upvtxtoscr();
	this->deckspeed[1][1]->box->tooltiptitle = "Global preview deck B speed setting ";
	this->deckspeed[1][1]->box->tooltip = "Change global deck B speed factor for preview streams. Leftdrag changes value. Doubleclick allows numeric entry. ";
    lpc->allparams.push_back(this->deckspeed[1][1]);

    this->layers.push_back({}); // the old layersA
    this->layers.push_back({}); // the old layersB
    this->layers.push_back({}); // the old layersAcomp
    this->layers.push_back({}); // the old layersBcomp
}


void Mixer::change_currlay(Layer *oldcurr, Layer *newcurr) {
    int pos = std::find(mainmix->currlays[!mainprogram->prevmodus].begin(),
                        mainmix->currlays[!mainprogram->prevmodus].end(), oldcurr) -
              mainmix->currlays[!mainprogram->prevmodus].begin();
    if (pos == mainmix->currlays[!mainprogram->prevmodus].size()) {
        pos = -1;
    }
    if (pos != -1) {
        mainmix->currlays[!mainprogram->prevmodus][pos] = newcurr;
        mainmix->currlay[!mainprogram->prevmodus] = newcurr;
    }
}



// GENERAL MIDI PRESET BUTTONS

void Mixer::handle_genmidibuttons() {
	Button* but;
	for (int i = 0; i < 2; i++) {
		std::vector<Layer*>& lvec = choose_layers(i);
		if (i == 0) but = this->genmidi[0];
		else but = this->genmidi[1];
		bool ch = mainprogram->handle_button(but, 0, 0);
		if (ch) {
			for (int j = 0; j < lvec.size(); j++) {
				lvec[j]->genmidibut->value = but->value;
			}
		}
		std::string butstr;
		if (but->value == 0) butstr = "off";
		else if (but->value == 1) butstr = "A";
		else if (but->value == 2) butstr = "B";
		else if (but->value == 3) butstr = "C";
		else if (but->value == 4) butstr = "D";
		render_text(butstr, white, but->box->vtxcoords->x1 + 0.0078f, but->box->vtxcoords->y1 + 0.0416f - 0.030, 0.0006, 0.001);
	}
}




// BASIC PARAMETER METHODS

Param::Param() {
	this->box = new Boxx;
    this->box->lcolor[0] = 0.6f;
    this->box->lcolor[1] = 0.6f;
    this->box->lcolor[2] = 0.6f;
    this->box->lcolor[3] = 1.0f;
    this->box->acolor[0] = 0.15;
    this->box->acolor[1] = 0.3;
    this->box->acolor[2] = 0.15;
    this->box->acolor[3] = 1.0;
	if (mainprogram) {
		if (mainprogram->prevmodus) {
			if (lp) lp->allparams.push_back(this);
		}
		else {
			if (lpc) lpc->allparams.push_back(this);
		}
	}
}

Param::~Param() {
	delete this->box;
	std::mutex lock;
	lock.lock();
	this->deautomate();
	lock.unlock();
    if (mainprogram) {
        if (mainprogram->prevmodus) {
            int pos = std::find(lp->allparams.begin(), lp->allparams.end(), this) - lp->allparams.begin();
            if (pos != lp->allparams.size()) {
                lp->allparams.erase(lp->allparams.begin() + pos);
            }
        }
        else {
            int pos = std::find(lpc->allparams.begin(), lpc->allparams.end(), this) - lpc->allparams.begin();
            if (pos != lpc->allparams.size()) {
                lpc->allparams.erase(lpc->allparams.begin() + pos);
            }
        }
    }
}

void Param::handle(bool smallxpad) {
    float pad = 0.02f;
    if (smallxpad) pad = 0.005f;
	std::string thisstr;
	draw_box(this->box, -1);
    draw_box(grey, black, this->box->vtxcoords->x1 + pad, this->box->vtxcoords->y1 + this->box->vtxcoords->h / 8.0f, this->box->vtxcoords->w - pad * 2.0f, this->box->vtxcoords->h * 0.75f, -1);
	int val;
	if (!this->powertwo) val = round(this->value * 1000.0f);
	else val = round(this->value * this->value * 1000.0f);
	int val2 = val;
	val += 1000000;
	int firstdigit = 7 - std::to_string(val2).length();
	if (firstdigit > 3) firstdigit = 3;
	if (mainmix->learnparam == this && mainmix->learn) {
		thisstr = "MIDI";
	}
	else if (this != mainmix->adaptparam) thisstr = this->name;
	else if (this->sliding) thisstr = std::to_string(val).substr(firstdigit, 4 - firstdigit) + "." + std::to_string(val).substr(std::to_string(val).length() - 3, std::string::npos); 
	else thisstr = std::to_string((int)(this->value + (float)(0.5f * (this->effect->type == FLIP || this->effect->type == MIRROR))));
	if (this != mainmix->adaptnumparam) {
		render_text(thisstr, white, this->box->vtxcoords->x1 + 0.03f, this->box->vtxcoords->y1 + 0.075f - 0.045f, 0.00045f, 0.00075f);
		if (this->box->in()) {
			if (mainprogram->leftmousedown && !mainprogram->inserteffectbox->in()) {
				mainprogram->leftmousedown = false;
				mainmix->prepadaptparam = this;
				mainmix->prevx = mainprogram->mx;
			}
			if (mainprogram->doubleleftmouse) {
				mainprogram->renaming = EDIT_PARAM;
				mainmix->adaptnumparam = this;
				mainprogram->inputtext = "";
				mainprogram->cursorpos0 = mainprogram->inputtext.length();
				SDL_StartTextInput();
				mainprogram->doubleleftmouse = false;
			}
			if (mainprogram->menuactivation && !mainprogram->menuondisplay) {
                if (this->name == "Speed") {
                    if (loopstation->parelemmap.find(this) != loopstation->parelemmap.end()) mainprogram->parammenu2b->state = 2;
                    else {
                        mainprogram->parammenu1b->state = 2;
                    }
                    for (Layer *l : mainmix->layers[0]) {
                        if (l->speed == this) {
                            mainmix->menulayer = l;
                            break;
                        }
                    }
                    for (Layer *l : mainmix->layers[2]) {
                        if (l->speed == this) {
                            mainmix->menulayer = l;
                            break;
                        }
                    }
                    for (Layer *l : mainmix->layers[1]) {
                        if (l->speed == this) {
                            mainmix->menulayer = l;
                            break;
                        }
                    }
                    for (Layer *l : mainmix->layers[3]) {
                        if (l->speed == this) {
                            mainmix->menulayer = l;
                            break;
                        }
                    }
                }
                else {
                    if (loopstation->parelemmap.find(this) != loopstation->parelemmap.end())
                        mainprogram->parammenu2->state = 2;
                    else mainprogram->parammenu1->state = 2;
                }
				mainmix->learnbutton = nullptr;
				mainmix->learnparam = this;
				mainprogram->menuactivation = false;
			}
		}
		if (this->sliding) {
			draw_box(green, green, this->box->vtxcoords->x1 + pad + (this->box->vtxcoords->w - pad * 2.0f) * ((this->value - this->range[0]) / (this->range[1] - this->range[0])) - 0.002f, this->box->vtxcoords->y1 + this->box->vtxcoords->h / 8.0f, 0.004f, this->box->vtxcoords->h * 0.75f, -1);
		}
		else {
			draw_box(green, green, this->box->vtxcoords->x1 + pad + (this->box->vtxcoords->w - pad * 2.0f) * (((int)(this->value + 0.5f) - this->range[0]) / (this->range[1] - this->range[0])) - 0.002f, this->box->vtxcoords->y1 + this->box->vtxcoords->h / 8.0f, 0.004f, this->box->vtxcoords->h * 0.75f, -1);
		}
	}
	if (this == mainmix->adaptnumparam) {
		do_text_input(this->box->vtxcoords->x1 + 0.035f, this->box->vtxcoords->y1 + 0.03f, 0.00045f, 0.00075f, mainprogram->mx, mainprogram->my, mainprogram->xvtxtoscr(this->box->vtxcoords->w - 0.03f), 0, nullptr);
	}
}


void reset_par(Param *par, float val) {
// reset all speed settings
    LoopStationElement *elem = loopstation->parelemmap[par];
    if (elem) {
        if (!elem->eventlist.empty()) {
            for (int i = elem->eventlist.size() - 1; i >= 0; i--) {
                std::tuple<long long, Param *, Button *, float> event = elem->eventlist[i];
                if (std::get<1>(event) == par) {
                    elem->eventlist.erase(elem->eventlist.begin() + i);
                }
            }
        }
        loopstation->parelemmap[par]->params.erase(par);
        loopstation->parelemmap.erase(par);
    }
    par->value = val;
}


void Param::deautomate() {
    if (this) {
        LoopStationElement *elem = loopstation->parelemmap[this];
        if (!elem) return;
        elem->params.erase(this);
        if (this->effect) elem->layers.erase(this->effect->layer);
        for (int i = elem->eventlist.size() - 1; i >= 0; i--) {
            if (std::get<1>(elem->eventlist[i]) == this)
                elem->eventlist.erase(elem->eventlist.begin() + i);
        }
        loopstation->parelemmap.erase(this);
        this->box->acolor[0] = 0.2f;
        this->box->acolor[1] = 0.2f;
        this->box->acolor[2] = 0.2f;
        this->box->acolor[3] = 1.0f;
    }
}

void Param::register_midi() {
    if (this->midiport == "") return;
    registered_midi rm = mainmix->midi_registrations[!mainprogram->prevmodus][this->midi[0]][this->midi[1]][this->midiport];
    if (rm.but) {
        rm.but->midi[0] = -1;
        rm.but->midi[1] = -1;
        rm.but->midiport = "";
        rm.but = nullptr;
    }
    else if (rm.par && rm.par != this) {
        rm.par->midi[0] = -1;
        rm.par->midi[1] = -1;
        rm.par->midiport = "";
        rm.par = nullptr;
    }
    else if (rm.midielem) {
        rm.midielem->midi0 = -1;
        rm.midielem->midi1= -1;
        rm.midielem->midiport = "";
        rm.midielem = nullptr;
    }

    mainmix->midi_registrations[!mainprogram->prevmodus][this->midi[0]][this->midi[1]][this->midiport].par = this;
}

void Param::unregister_midi() {
    mainmix->midi_registrations[!mainprogram->prevmodus][this->midi[0]][this->midi[1]][this->midiport].par = nullptr;
}



// PARAMETER CHANGES

void Mixer::handle_adaptparam() {
	this->adaptparam->value += (mainprogram->mx - this->prevx) * (this->adaptparam->range[1] - this->adaptparam->range[0]) / mainprogram->xvtxtoscr(this->adaptparam->box->vtxcoords->w);
	if (this->adaptparam->value < this->adaptparam->range[0]) {
		this->adaptparam->value = this->adaptparam->range[0];
	}
	if (this->adaptparam->value > this->adaptparam->range[1]) {
		this->adaptparam->value = this->adaptparam->range[1];
	}
	this->prevx = mainprogram->mx;

	if (this->adaptparam == this->currlay[!mainprogram->prevmodus]->speed) {
	    for (int i = 0; i < this->currlays[!mainprogram->prevmodus].size(); i++) {
	        this->currlays[!mainprogram->prevmodus][i]->set_clones();
	    }
	}
	if (this->adaptparam == this->currlay[!mainprogram->prevmodus]->opacity) {
	    for (int i = 0; i < this->currlays[!mainprogram->prevmodus].size(); i++) {
            this->currlays[!mainprogram->prevmodus][i]->set_clones();
        }
	}

    if (this->adaptparam->shadervar == "ripple") {
		((RippleEffect*)this->adaptparam->effect)->speed = this->adaptparam->value;
	}
	else {
		GLint var = glGetUniformLocation(mainprogram->ShaderProgram, this->adaptparam->shadervar.c_str());
		if (this->adaptparam->sliding) {
			glUniform1f(var, this->adaptparam->value);
		}
		else {
			glUniform1f(var, (int)(this->adaptparam->value + 0.5f));
		}
		this->midiparam = nullptr;
	}

	for (int i = 0; i < loopstation->elems.size(); i++) {
		if (loopstation->elems[i]->recbut->value) {
			loopstation->elems[i]->add_param_automationentry(mainmix->adaptparam);
		}
	}

	if (mainprogram->lmover) {
		if (!this->adaptparam->sliding) {
			this->adaptparam->value = (int)(this->adaptparam->value + 0.5f);
		}
		this->adaptparam = nullptr;
	}
}



// BASIC EFFECT METHODS

Effect::Effect() {
	Boxx *box = new Boxx;
	this->box = box;
	box->vtxcoords->w = 0.3f;
	box->vtxcoords->h = 0.075f;
	box->tooltiptitle = "Effect type name ";
	box->tooltip = "Leftclick or rightclick effect name to change it in another effect class.  Leftdrag to move effect in the stack. ";
	box->upvtxtoscr();
	this->onoffbutton = new Button(true);
    this->onoffbutton->butid = 1;
	this->onoffbutton->toggle = 1;
	box = this->onoffbutton->box;
	box->vtxcoords->x1 = -1.0f + mainprogram->numw;
	box->vtxcoords->w = 0.0375f;
	box->vtxcoords->h = 0.075f;
	box->upvtxtoscr();
	box->tooltiptitle = "Effect on/off ";
	box->tooltip = "Leftclick toggles effect on/off ";
	
	// sets the dry/wet (mix of no-effect with effect) amount of the effect as a parameter
	// read comment at BlurEffect::BlurEffect()
	this->drywet = new Param;
	this->drywet->name = ""; 
	this->drywet->value = 1.0f;
	this->drywet->deflt = 1.0f;
	this->drywet->range[0] = 0.0f;
	this->drywet->range[1] = 1.0f;
	this->drywet->sliding = true;
	this->drywet->shadervar = "drywet";
	this->drywet->box->vtxcoords->w = 0.0375f;
	this->drywet->box->vtxcoords->h = 0.075f;
	this->drywet->box->upvtxtoscr();
	this->drywet->box->tooltiptitle = "Effect dry/wet ";
	this->drywet->box->tooltip = "Sets the dry/wet value of the effect - Leftdrag sets value. Doubleclick allows numeric input between 0.0 and 1.0 ";
}

Effect::~Effect() {
	delete this->box;
	delete this->drywet;
	delete this->onoffbutton;
    if (this->layer != mainmix->reclay) {
        glDeleteTextures(1, &this->fbotex);
        glDeleteFramebuffers(1, &this->fbo);
    }
}

std::string Effect::get_namestring() {
	std::string effstr;
	switch (this->type) {
		case 0:
			effstr = "BLUR";
			break;
		case 1:
			effstr = "BRIGHTNESS";
			break;
		case 2:
			effstr = "CHROMAROTATE";
			break;
		case 3:
			effstr = "CONTRAST";
			break;
		case 4:
			effstr = "DOT";
			break;
		case 5:
			effstr = "GLOW";
			break;
		case 6:
			effstr = "RADIALBLUR";
			break;
		case 7:
			effstr = "SATURATION";
			break;
		case 8:
			effstr = "SCALE";
			break;
		case 9:
			effstr = "SWIRL";
			break;
		case 10:
			effstr = "OLDFILM";
			break;
		case 11:
			effstr = "RIPPLE";
			break;
		case 12:
			effstr = "FISHEYE";
			break;
		case 13:
			effstr = "TRESHOLD";
			break;
		case 14:
			effstr = "STROBE";
			break;
		case 15:
			effstr = "POSTERIZE";
			break;
		case 16:
			effstr = "PIXELATE";
			break;
		case 17:
			effstr = "CROSSHATCH";
			break;
		case 18:
			effstr = "INVERT";
			break;
		case 19:
			effstr = "ROTATE";
			break;
		case 20:
			effstr = "EMBOSS";
			break;
		case 21:
			effstr = "ASCII";
			break;
		case 22:
			effstr = "SOLARIZE";
			break;
		case 23:
			effstr = "VARDOT";
			break;
		case 24:
			effstr = "CRT";
			break;
		case 25:
			effstr = "EDGEDETECT";
			break;
		case 26:
			effstr = "KALEIDOSCOPE";
			break;
		case 27:
			effstr = "HALFTONE";
			break;
		case 28:
			effstr = "CARTOON";
			break;
		case 29:
			effstr = "CUTOFF";
			break;
		case 30:
			effstr = "GLITCH";
			break;
		case 31:
			effstr = "COLORIZE";
			break;
		case 32:
			effstr = "NOISE";
			break;
		case 33:
			effstr = "GAMMA";
			break;
		case 34:
			effstr = "THERMAL";
			break;
		case 35:
			effstr = "BOKEH";
			break;
		case 36:
			effstr = "SHARPEN";
			break;
		case 37:
			effstr = "DITHER";
			break;
		case 38:
			effstr = "FLIP";
			break;
		case 39:
			effstr = "MIRROR";
			break;
        case 40:
            effstr = "BOXBLUR";
            break;
        case 41:
            effstr = "CHROMASTRETCH";
            break;
	}
	return effstr;
}

void Mixer::copy_effects(Layer* slay, Layer* dlay, bool comp) {
	LoopStation* lp1;
	LoopStation* lp2;
	if (comp) {
		lp1 = lp;
		lp2 = lpc;
	}
	else {
		lp1 = lpc;
		lp2 = lp;
	}
	for (int m = 0; m < 2; m++) {
		dlay->effects[m].clear();
		for (int j = 0; j < slay->effects[m].size(); j++) {
			Effect* eff = slay->effects[m][j];
			Effect* ceff = new_effect(eff->type);
			ceff->type = eff->type;
			ceff->layer = dlay;
			ceff->onoffbutton->layer = dlay;
			ceff->onoffbutton->value = eff->onoffbutton->value;
			ceff->drywet->value = eff->drywet->value;
			if (ceff->type == RIPPLE) ((RippleEffect*)ceff)->speed = ((RippleEffect*)eff)->speed;
			if (ceff->type == RIPPLE) ((RippleEffect*)ceff)->ripplecount = ((RippleEffect*)eff)->ripplecount;
			dlay->effects[m].push_back(ceff);
			for (int k = 0; k < slay->effects[m][j]->params.size(); k++) {
				Param* par = slay->effects[m][j]->params[k];
				Param* cpar = dlay->effects[m][j]->params[k];
				lp1->parmap[par] = cpar;
				cpar->value = par->value;
				cpar->midi[0] = par->midi[0];
				cpar->midi[1] = par->midi[1];
				cpar->register_midi();
				cpar->effect = ceff;
				lp2->allparams.push_back(cpar);
			}
		}
	}

}

// The following section defines the different effects and their respective parameters
// this->numrows defines the number of lines the effect+params takes up in the GUI
// param->name is the parameter name as it appears in the GUI
// param->value is the default parameter value
// param->range defines the boundaries of possible parameter values
// param->sliding: true when value is continuous, false when value is integer (not used at the moment, all params are continuous)
// param->shadervar is the uniform variable name used in the shader to represent the parameter
// param->effect: the effect the parameter belongs to

BlurEffect::BlurEffect() {
	this->numrows = 1;
	Param* param = new Param;
	param->name = "Amount";
	param->value = 10.0;
	param->range[0] = 1;
	param->range[1] = 60;
	param->sliding = true;
	param->shadervar = "glowblur";
	param->effect = this;
	param->box->tooltiptitle = "Blur amount ";
	param->box->tooltip = "Amount of image blurring - between 1 and 60 ";
	this->params.push_back(param);
}

BoxblurEffect::BoxblurEffect() {
    this->numrows = 1;
    Param* param = new Param;
    param->name = "Amount";
    param->value = 10.0;
    param->range[0] = 1;
    param->range[1] = 60;
    param->sliding = false;
    param->shadervar = "glowblur";
    param->effect = this;
    param->box->tooltiptitle = "Boxblur amount ";
    param->box->tooltip = "Amount of image blurring - between 1 and 60 ";
    this->params.push_back(param);
    param = new Param;
    param->name = "Precision";
    param->value = 20;
    param->range[0] = 2;
    param->range[1] = 40;
    param->sliding = false;
    param->shadervar = "jump";
    param->effect = this;
    param->box->tooltiptitle = "Boxblur precision ";
    param->box->tooltip = "Size of boxblur boxes - between 1 and 40 ";
    this->params.push_back(param);
}

ChromastretchEffect::ChromastretchEffect() {
    this->numrows = 1;
    Param* param = new Param;
    param->name = "Hue";
    param->value = 0.0f;
    param->range[0] = 0.0f;
    param->range[1] = 1.0f;
    param->sliding = true;
    param->shadervar = "cshue";
    param->effect = this;
    param->box->tooltiptitle = "Chromastretch hue ";
    param->box->tooltip = "Hue used as chromastretch center - between 0.0 and 1.0 ";
    this->params.push_back(param);
    param = new Param;
    param->name = "Factor";
    param->value = 1.0f;
    param->range[0] = 0.0f;
    param->range[1] = 1.0f;
    param->sliding = true;
    param->shadervar = "csfac";
    param->effect = this;
    param->box->tooltiptitle = "Chromastretch factor";
    param->box->tooltip = "Factor used for chromastretch - between 0.0 and 1.0 ";
    this->params.push_back(param);
}

BrightnessEffect::BrightnessEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Amount"; 
	param->value = 0.5f;
	param->range[0] = -1.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "brightamount";
	param->effect = this;
	param->box->tooltiptitle = "Brightness amount ";
	param->box->tooltip = "Amount of image brightness change - between -1.0 and 1.0 ";
	this->params.push_back(param);
}

ChromarotateEffect::ChromarotateEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Amount"; 
	param->value = 0.0f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "colorrot";
	param->effect = this;
	param->box->tooltiptitle = "Chromarotate amount ";
	param->box->tooltip = "Amount of image hue change - between -0.5 and 0.5 ";
	this->params.push_back(param);
}

ContrastEffect::ContrastEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Amount"; 
	param->value = 2.0;
	param->range[0] = 0;
	param->range[1] = 4;
	param->sliding = true;
	param->shadervar = "contrastamount";
	param->effect = this;
	param->box->tooltiptitle = "Contrast amount ";
	param->box->tooltip = "Image contrast multiplication factor - between 0.0 and 4.0";
	this->params.push_back(param);
}

DotEffect::DotEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Size"; 
	param->value = 300.0;
	param->range[0] = 7.5;
	param->range[1] = 2000;
	param->sliding = true;
	param->shadervar = "dotsize";
	param->effect = this;
	param->box->tooltiptitle = "Dot size ";
	param->box->tooltip = "Size of overlayed dots - between 7.5 and 2000.0 ";
	this->params.push_back(param);
}

GlowEffect::GlowEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Blur"; 
	param->value = 32.0;
	param->range[0] = 8;
	param->range[1] = 132;
	param->sliding = true;
	param->shadervar = "glowblur";
	param->box->tooltiptitle = "Blur amount ";
	param->box->tooltip = "Amount of image blurring - betwwen 8.0 and 132.0 ";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "Amount"; 
	param->value = 1.2f;
	param->range[0] = 1.0f;
	param->range[1] = 1.5f;
	param->sliding = true;
	param->shadervar = "glowfac";
	param->effect = this;
	param->box->tooltiptitle = "Glow amount ";
	param->box->tooltip = "Amount of image glow - between 1.0 and 1.5 ";
	this->params.push_back(param);
}

RadialblurEffect::RadialblurEffect() {
	this->numrows = 2;
	Param *param = new Param;
	param->name = "Strength"; 
	param->value = 0.1f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "radstrength";
	param->box->tooltiptitle = "Radialblur strength ";
	param->box->tooltip = "Strenght of radialblur - between 0.0 and 1.0 ";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "InnerRadius"; 
	param->value = 0.0f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "radinner";
	param->effect = this;
	param->box->tooltiptitle = "Inner radialblur radius ";
	param->box->tooltip = "Radius of untouched area inside blur ellipse - between 0.0 and 1.0 ";
	this->params.push_back(param);
	param = new Param;
	param->nextrow = true;
	param->name = "X"; 
	param->value = 0.5f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "radialX";
	param->effect = this;
	param->box->tooltiptitle = "Radialblur X position ";
	param->box->tooltip = "X position where radialblur rays start ";
	this->params.push_back(param);
	param = new Param;
	param->name = "Y"; 
	param->value = 0.5f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "radialY";
	param->effect = this;
	param->box->tooltiptitle = "Radialblur Y position ";
	param->box->tooltip = "Y position where radialblur rays start ";
	this->params.push_back(param);
}

SaturationEffect::SaturationEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Factor"; 
	param->value = 4.0f;
	param->range[0] = 0.0f;
	param->range[1] = 8.0f;
	param->sliding = true;
	param->shadervar = "satamount";
	param->effect = this;
	param->box->tooltiptitle = "Saturation amount factor";
	param->box->tooltip = "Factor that multiplies image saturation - between 0.0 and 8.0 ";
	this->params.push_back(param);
}

ScaleEffect::ScaleEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Factor"; 
	param->value = 1.1f;
	param->range[0] = 0.75f;
	param->range[1] = 1.33f;
	param->sliding = true;
	param->shadervar = "scalefactor";
	param->effect = this;
	param->box->tooltiptitle = "Scale factor ";
	param->box->tooltip = "Factor by which image is scaled - between 0.75 and 1.33 ";
	this->params.push_back(param);
}

SwirlEffect::SwirlEffect() {
	this->numrows = 2;
	Param *param = new Param;
	param->name = "Angle"; 
	param->value = 0.8f;
	param->range[0] = 0.0f;
	param->range[1] = 2.0f;
	param->sliding = true;
	param->shadervar = "swirlangle";
	param->effect = this;
	param->box->tooltiptitle = "Swirl angle ";
	param->box->tooltip = "Angle amount of image swirl - between 0.0 and 2.0 ";
	this->params.push_back(param);
	param = new Param;
	param->name = "Radius"; 
	param->value = 0.5;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "swradius";
	param->effect = this;
	param->box->tooltiptitle = "Swirl radius ";
	param->box->tooltip = "Radius size of image swirl - between 0.0 and 1.0 ";
	this->params.push_back(param);
	param = new Param;
	param->nextrow = true;
	param->name = "X"; 
	param->value = 0.5f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "swirlx";
	param->effect = this;
	param->box->tooltiptitle = "Swirl X position ";
	param->box->tooltip = "X position of swirl center - between 0.0 and 1.0 ";
	this->params.push_back(param);
	param = new Param;
	param->name = "Y"; 
	param->value = 0.5f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "swirly";
	param->effect = this;
	param->box->tooltiptitle = "Swirl Y position ";
	param->box->tooltip = "Y position of swirl center - between 0.0 and 1.0 ";
	this->params.push_back(param);
}

OldFilmEffect::OldFilmEffect() {
	this->numrows = 2;
	Param *param = new Param;
	param->name = "Noise"; 
	param->value = 2.0f;
	param->range[0] = 0.0f;
	param->range[1] = 4.0f;
	param->sliding = true;
	param->shadervar = "filmnoise";
	param->effect = this;
	param->box->tooltiptitle = "Oldfilm noise amount ";
	param->box->tooltip = "Amount of noise in image - between 0.0 and 4.0 ";
	this->params.push_back(param);
	param = new Param;
	param->name = "Sepia"; 
	param->value = 2.0f;
	param->range[0] = 0.0f;
	param->range[1] = 4.0f;
	param->sliding = true;
	param->shadervar = "SepiaValue";
	param->box->tooltiptitle = "Oldfilm sepia amount ";
	param->box->tooltip = "Sepia image colorization saturation amount - between 0.0 and 4.0 ";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "Scratch"; 
	param->value = 2.0f;
	param->range[0] = 0.0f;
	param->range[1] = 4.0f;
	param->sliding = true;
	param->shadervar = "ScratchValue";
	param->box->tooltiptitle = "Oldfilm scratches amount ";
	param->box->tooltip = "Amount of scratches in image - between 0.0 and 4.0 ";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->nextrow = true;
	param->name = "InVign"; 
	param->value = 0.7f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "InnerVignetting";
	param->box->tooltiptitle = "Oldfilm inner vignetting amount ";
	param->box->tooltip = "Amount of inner vignetting of image - between 0.0 and 1.0 ";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "OutVign"; 
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "OuterVignetting";
	param->value = 1.0f;
	param->box->tooltiptitle = "Oldfilm outer vignetting amount ";
	param->box->tooltip = "Amount of outer vignetting of image - between 0.0 and 1.0 ";
	param->effect = this;
	this->params.push_back(param);
}

RippleEffect::RippleEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Speed"; 
	param->value = 0.1f;
	param->range[0] = 0.0f;
	param->range[1] = 0.5f;
	param->sliding = true;
	param->shadervar = "ripple";
	param->effect = this;
	param->box->tooltiptitle = "Ripple speed ";
	param->box->tooltip = "Speed of image ripple distortion - between 0.0 and 0.5 ";
	this->params.push_back(param);
}

FishEyeEffect::FishEyeEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Amount"; 
	param->value = 0.7f;
	param->range[0] = 0.1f;
	param->range[1] = 2;
	param->sliding = true;
	param->shadervar = "fishamount";
	param->effect = this;
	param->box->tooltiptitle = "Fisheye amount ";
	param->box->tooltip = "Amount of image fisheye distortion -between 0.1 and 2.0  Low values pinch image. ";
	this->params.push_back(param);
}

TresholdEffect::TresholdEffect() {
	this->numrows = 3;
	Param *param = new Param;
	param->name = "Level"; 
	param->value = 0.5f;
	param->range[0] = 0.0f;
	param->range[1] = 2.0f;
	param->sliding = true;
	param->shadervar = "treshheight";
	param->effect = this;
	param->box->tooltiptitle = "Treshold cutoff level";
	param->box->tooltip = "Lightness level at which treshold switches between colors - between 0.0 and 2.0 ";
	this->params.push_back(param);
	param = new Param;
	param->nextrow = true;
	param->name = "R1"; 
	param->value = 1.0f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "treshred1";
	param->effect = this;
	param->box->tooltiptitle = "Treshold red1 ";
	param->box->tooltip = "Value of red component of color 1 - between 0.0 and 1.0 ";
	this->params.push_back(param);
	param = new Param;
	param->name = "G1"; 
	param->value = 1.0f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "treshgreen1";
	param->effect = this;
	param->box->tooltiptitle = "Treshold green1 ";
	param->box->tooltip = "Value of green component of color 1 - between 0.0 and 1.0 ";
	this->params.push_back(param);
	param = new Param;
	param->name = "B1"; 
	param->value = 1.0f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "treshblue1";
	param->effect = this;
	param->box->tooltiptitle = "Treshold blue1 ";
	param->box->tooltip = "Value of blue component of color 1 - between 0.0 and 1.0 ";
	this->params.push_back(param);
	param = new Param;
	param->nextrow = true;
	param->name = "R2"; 
	param->value = 0.0f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "treshred2";
	param->effect = this;
	param->box->tooltiptitle = "Treshold red2 ";
	param->box->tooltip = "Value of red component of color 2 - between 0.0 and 1.0 ";
	this->params.push_back(param);
	param = new Param;
	param->name = "G2"; 
	param->value = 0.0f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "treshgreen2";
	param->effect = this;
	param->box->tooltiptitle = "Treshold green2 ";
	param->box->tooltip = "Value of green component of color 2 - between 0.0 and 1.0 ";
	this->params.push_back(param);
	param = new Param;
	param->name = "B2"; 
	param->value = 0.0f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "treshblue2";
	param->effect = this;
	param->box->tooltiptitle = "Treshold blue2 ";
	param->box->tooltip = "Value of blue component of color 2 - between 0.0 and 1.0 ";
	this->params.push_back(param);
}

StrobeEffect::StrobeEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "R"; 
	param->value = 1.0f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "strobered";
	param->effect = this;
	param->box->tooltiptitle = "Strobe red ";
	param->box->tooltip = "Red component of strobe color - between 0.0 and 1.0 ";
	this->params.push_back(param);
	param = new Param;
	param->name = "G"; 
	param->value = 1.0f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "strobegreen";
	param->effect = this;
	param->box->tooltiptitle = "Strobe green ";
	param->box->tooltip = "Green component of strobe color - between 0.0 and 1.0 ";
	this->params.push_back(param);
	param = new Param;
	param->name = "B"; 
	param->value = 1.0f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "strobeblue";
	param->effect = this;
	param->box->tooltiptitle = "Strobe blue ";
	param->box->tooltip = "Blue component of strobe color - between 0.0 and 1.0 ";
	this->params.push_back(param);
}

PosterizeEffect::PosterizeEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Number"; 
	param->value = 8;
	param->range[0] = 0;
	param->range[1] = 16;
	param->sliding = false;
	param->shadervar = "numColors";
	param->effect = this;
	param->box->tooltiptitle = "Number of colors ";
	param->box->tooltip = "Number of colors for posterization - integer between 0 and 16 ";
	this->params.push_back(param);
	param = new Param;
	param->name = "Gamma"; 
	param->value = 0.6f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "gamma";
	param->effect = this;
	param->box->tooltiptitle = "Posterize gamma ";
	param->box->tooltip = "Gamma adaptation for posterization - between 0.0 and 1.0 ";
	this->params.push_back(param);
}

PixelateEffect::PixelateEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Width"; 
	param->value = 8.0f;
	param->range[0] = 1.0f;
	param->range[1] = 10.0f;
	param->sliding = true;
	param->shadervar = "pixel_w";
	param->effect = this;
	param->box->tooltiptitle = "Pixel width ";
	param->box->tooltip = "Width of pixels of pixelization - between 1.0 and 512.0 ";
	this->params.push_back(param);
	param = new Param;
	param->name = "Height"; 
	param->value = 8.0f;
	param->range[0] = 1.0f;
	param->range[1] = 10.0f;
	param->sliding = true;
	param->shadervar = "pixel_h";
	param->effect = this;
	param->box->tooltiptitle = "Pixel height ";
	param->box->tooltip = "Height of pixels of pixelization - between 1.0 and 512.0 ";
	this->params.push_back(param);
}

CrosshatchEffect::CrosshatchEffect() {
	this->numrows = 3;
	Param *param = new Param;
	param->name = "Size"; 
	param->value = 20.0f;
	param->range[0] = 2.0f;
	param->range[1] = 100.0f;
	param->sliding = true;
	param->shadervar = "hatchsize";
	param->effect = this;
	param->box->tooltiptitle = "Crosshatch size ";
	param->box->tooltip = "Size of iame crosshatching - between 0.0 and 100.0 ";
	this->params.push_back(param);
	param = new Param;
	param->name = "Y Offset"; 
	param->value = 5.0;
	param->range[0] = 0.0f;
	param->range[1] = 40.0f;
	param->sliding = true;
	param->shadervar = "hatch_y_offset";
	param->effect = this;
	param->box->tooltiptitle = "Crosshatch Y offset ";
	param->box->tooltip = "Y offset distance of image crosshatching - between 0.0 and 40.0 ";
	this->params.push_back(param);
	param = new Param;
	param->nextrow = true;
	param->name = "Lum Tresh 1"; 
	param->value = 1.0f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "lum_threshold_1";
	param->effect = this;
	param->box->tooltiptitle = "Crosshatch luminance treshold 1 ";
	param->box->tooltip = "Luminance treshold value for image crosshatching - between 0.0 and 1.0 ";
	this->params.push_back(param);
	param = new Param;
	param->name = "Lum Tresh 2"; 
	param->value = 0.7f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "lum_threshold_2";
	param->effect = this;
	param->box->tooltiptitle = "Crosshatch luminance treshold 2 ";
	param->box->tooltip = "Luminance treshold value for image crosshatching - between 0.0 and 1.0 ";
	this->params.push_back(param);
	param = new Param;
	param->nextrow = true;
	param->name = "Lum Tresh 3"; 
	param->value = 0.5f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "lum_threshold_3";
	param->effect = this;
	param->box->tooltiptitle = "Crosshatch luminance treshold 3 ";
	param->box->tooltip = "Luminance treshold value for image crosshatching - between 0.0 and 1.0 ";
	this->params.push_back(param);
	param = new Param;
	param->name = "Lum Tresh 4"; 
	param->value = 0.3f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "lum_threshold_4";
	param->effect = this;
	param->box->tooltiptitle = "Crosshatch luminance treshold 4 ";
	param->box->tooltip = "Luminance treshold value for image crosshatching - between 0.0 and 1.0 ";
	this->params.push_back(param);
}

InvertEffect::InvertEffect() {
	this->numrows = 1;
}

RotateEffect::RotateEffect() {
	this->numrows = 1;
    Param *param = new Param;
    param->name = "Angle";
    param->value = 45.0;
    param->range[0] = 0.0f;
    param->range[1] = 360.0f;
    param->sliding = true;
    param->shadervar = "rotamount";
    param->effect = this;
    param->box->tooltiptitle = "Rotation angle ";
    param->box->tooltip = "Angle of image rotation - between 0.0 and 360.0 ";
    this->params.push_back(param);
    param = new Param;
    param->name = "Mode";
    param->value = 0.0f;
    param->range[0] = 0.0f;
    param->range[1] = 1.0f;
    param->sliding = false;
    param->shadervar = "rotmode";
    param->effect = this;
    param->box->tooltiptitle = "Rotation mode ";
    param->box->tooltip = "Mode of image rotation - between 0 and 1 ";
    this->params.push_back(param);
}

EmbossEffect::EmbossEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Strength"; 
	param->value = 5.0f;
	param->range[0] = 0.0f;
	param->range[1] = 20.0f;
	param->sliding = true;
	param->shadervar = "embstrength";
	param->effect = this;
	param->box->tooltiptitle = "Emboss strength ";
	param->box->tooltip = "Strength of image embossing - between 0.0 and 20.0 ";
	this->params.push_back(param);
}

SolarizeEffect::SolarizeEffect() {
	this->numrows = 1;
}

AsciiEffect::AsciiEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Size"; 
	param->value = 50.0f;
	param->range[0] = 10.0f;
	param->range[1] = 200.0f;
	param->sliding = true;
	param->shadervar = "asciisize";
	param->effect = this;
	param->box->tooltiptitle = "ASCII size ";
	param->box->tooltip = "Size of ascii symbols - between 10.0 and 200.0 ";
	this->params.push_back(param);
}

VarDotEffect::VarDotEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Size"; 
	param->value = 0.1f;
	param->range[0] = 0.0f;
	param->range[1] = 0.2f;
	param->sliding = true;
	param->shadervar = "vardotsize";
	param->effect = this;
	param->box->tooltiptitle = "Vardot size ";
	param->box->tooltip = "Size of vardots - between 0.0 and 2.0 ";
	this->params.push_back(param);
	param = new Param;
	param->name = "Angle"; 
	param->value = 0.0f;
	param->range[0] = 0.0f;
	param->range[1] = 4.0f;
	param->sliding = true;
	param->shadervar = "vardotangle";
	param->effect = this;
	param->box->tooltiptitle = "Vardot angle ";
	param->box->tooltip = "Angle of image vardotting - between 0.0 and 4.0 ";
	this->params.push_back(param);
}

CRTEffect::CRTEffect() {
	this->numrows = 3;
	Param *param = new Param;
	param->name = "Curvature";
	param->value = 1.0f;
	param->range[0] = 0.0f;
	param->range[1] = 10.0f;
	param->sliding = true;
	param->shadervar = "crtcurvature";
	param->effect = this;
	param->box->tooltiptitle = "CRT linecurvature ";
	param->box->tooltip = "Curvature amount of crt lines - between 0.0 and 10.0 ";
	this->params.push_back(param);
	param = new Param;
	param->name = "LineWidth";
	param->value = 1.0f;
	param->range[0] = 0.0f;
	param->range[1] = 50.0f;
	param->sliding = true;
	param->shadervar = "crtlineWidth";
	param->effect = this;
	param->box->tooltiptitle = "CRT linewidth ";
	param->box->tooltip = "Width of crt lines - between 0.0 and 50.0 ";
	this->params.push_back(param);
	param = new Param;
	param->name = "LineContrast";
	param->value = 0.25f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "crtlineContrast";
	param->effect = this;
	param->box->tooltiptitle = "CRT linecontrast ";
	param->box->tooltip = "Contrast amount of crt lines - between 0.0 and 1.0 ";
	this->params.push_back(param);
	param = new Param;
	param->nextrow = true;
	param->name = "Noise";
	param->value = 0.3f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "crtnoise";
	param->effect = this;
	param->box->tooltiptitle = "CRT noise level ";
	param->box->tooltip = "Level of noise - between 0.0 and 1.0 ";
	this->params.push_back(param);
	param = new Param;
	param->name = "NoiseSize";
	param->value = 1.0f;
	param->range[0] = 0.0f;
	param->range[1] = 20.0f;
	param->sliding = true;
	param->shadervar = "crtnoiseSize";
	param->effect = this;
	param->box->tooltiptitle = "CRT noise size ";
	param->box->tooltip = "Size of noise - between 0.0 and 20.0 ";
	this->params.push_back(param);
	param = new Param;
	param->nextrow = true;
	param->name = "Vignet";
	param->value = 0.3f;
	param->range[0] = 0.0f;
	param->range[1] = 0.5f;
	param->sliding = true;
	param->shadervar = "crtvignetting";
	param->effect = this;
	param->box->tooltiptitle = "CRT vignetting size ";
	param->box->tooltip = "Size of vignetting - between 0.0 and 0.5 ";
	this->params.push_back(param);
	param = new Param;
	param->name = "VignetAlpha";
	param->value = 1.0f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "crtvignettingAlpha";
	param->effect = this;
	param->box->tooltiptitle = "CRT vignetting alpha ";
	param->box->tooltip = "Alpha value of vignetting - between 0.0 and 1.0 ";
	this->params.push_back(param);
	param = new Param;
	param->name = "VignetBlur";
	param->value = 0.3f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "crtvignettingBlur";
	param->effect = this;
	param->box->tooltiptitle = "CRT vignetting blur ";
	param->box->tooltip = "Blur amount of vignetting - between 0.0 and 1.0 ";
	this->params.push_back(param);
}

EdgeDetectEffect::EdgeDetectEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Threshold1";
	param->value = 1.1f;
	param->range[0] = 0.0f;
	param->range[1] = 1.1f;
	param->sliding = true;
	param->shadervar = "edge_thres";
	param->effect = this;
	param->box->tooltiptitle = "Edgedetect edge treshold 1 ";
	param->box->tooltip = "Edge treshold value for edge detection - between 0.0 and 1.1 ";
	this->params.push_back(param);
	param = new Param;
	param->name = "Threshold2";
	param->value = 5.0f;
	param->range[0] = 0.0f;
	param->range[1] = 20.0f;
	param->sliding = true;
	param->shadervar = "edge_thres2";
	param->effect = this;
	param->box->tooltiptitle = "Edgedetect edge treshold 2 ";
	param->box->tooltip = "Edge treshold value for edge detection - between 0.0 and 20.0 ";
	this->params.push_back(param);
	param = new Param;
	param->name = "Thickness";
	param->value = 1.0f;
	param->range[0] = 1.0f;
	param->range[1] = 20.0f;
	param->sliding = true;
	param->shadervar = "edthickness";  //dummy
	param->effect = this;
	param->box->tooltiptitle = "Edgedetect edge thickness ";
	param->box->tooltip = "Thickness of detected image edges - between 1.0 and 20.0 ";
	this->params.push_back(param);
}

KaleidoScopeEffect::KaleidoScopeEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Slices"; 
	param->value = 8;
	param->range[0] = 0;
	param->range[1] = 32;
	param->sliding = false;
	param->shadervar = "kalslices";
	param->effect = this;
	param->box->tooltiptitle = "Kaleidoscope slices ";
	param->box->tooltip = "Number of kaeidoscope slices - integer between 0 and 32 ";
	this->params.push_back(param);
}

HalfToneEffect::HalfToneEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Scale";
	param->value = 100.0f;
	param->range[0] = 1.0f;
	param->range[1] = 200.0f;
	param->sliding = true;
	param->shadervar = "hts";
	param->effect = this;
	param->box->tooltiptitle = "Halftone scale ";
	param->box->tooltip = "Scale of image halftoneing - between 1.0 and 200.0 ";
	this->params.push_back(param);
}

CartoonEffect::CartoonEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Threshold1";
	param->value = 0.2f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "edge_thres";
	param->effect = this;
	param->box->tooltiptitle = "Cartoonify edge treshold 1 ";
	param->box->tooltip = "Edge treshold value for image caroonization - between 0.0 and 1.0 ";
	this->params.push_back(param);
	param = new Param;
	param->name = "Threshold2";
	param->value = 5.0f;
	param->range[0] = 0.0f;
	param->range[1] = 10.0f;
	param->sliding = true;
	param->shadervar = "edge_thres2";
	param->effect = this;
	param->box->tooltiptitle = "Cartoonify edge treshold 2 ";
	param->box->tooltip = "Edge treshold value for image caroonization - between 0.0 and 10.0 ";
	this->params.push_back(param);
}

CutoffEffect::CutoffEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Treshold";
	param->value = 0.5f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "cut";
	param->effect = this;
	param->box->tooltiptitle = "Cutoff treshold ";
	param->box->tooltip = "Level under which dark image areas are turned black - between 0.0 and 1.0";
	this->params.push_back(param);
}

GlitchEffect::GlitchEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Speed";
	param->value = 1.0f;
	param->range[0] = 0.2f;
	param->range[1] = 4.0f;
	param->sliding = true;
	param->shadervar = "glitchspeed";
	param->effect = this;
	param->box->tooltiptitle = "Glitch speed ";
	param->box->tooltip = "Speed factor of image glitching - between 0.2 and 4.0 ";
	this->params.push_back(param);
}

ColorizeEffect::ColorizeEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Hue";
	param->value = 0.0f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "colhue";
	param->effect = this;
	param->box->tooltiptitle = "Colorize hue ";
	param->box->tooltip = "Hue of image colorizing - between 0.0 and 1.0 ";
	this->params.push_back(param);
}

NoiseEffect::NoiseEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Level";
	param->value = 0.5f;
	param->range[0] = 0.0f;
	param->range[1] = 8.0f;
	param->sliding = true;
	param->shadervar = "noiselevel";
	param->effect = this;
	param->box->tooltiptitle = "Noise level ";
	param->box->tooltip = "Amount of image noise - between 0.0 and 8.0 ";
	this->params.push_back(param);
}

GammaEffect::GammaEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Level";
	param->value = 2.2f;
	param->range[0] = 0.0f;
	param->range[1] = 4.0f;
	param->sliding = true;
	param->shadervar = "gammaval";
	param->effect = this;
	param->box->tooltiptitle = "Gamma ";
	param->box->tooltip = "Image gamma level - between 0.0 and 4.0 ";
	this->params.push_back(param);
}

ThermalEffect::ThermalEffect() {
	this->numrows = 1;
}

BokehEffect::BokehEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Radius";
	param->value = 1.0f;
	param->range[0] = 0.0f;
	param->range[1] = 4.0f;
	param->sliding = true;
	param->shadervar = "bokehrad";
	param->effect = this;
	param->box->tooltiptitle = "Bokeh radius ";
	param->box->tooltip = "Radius of bokeh circle spots - between 0.0 and 4.0 ";
	this->params.push_back(param);
}

SharpenEffect::SharpenEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Strength";
	param->value = 10.0f;
	param->range[0] = 0.0f;
	param->range[1] = 40.0f;
	param->sliding = true;
	param->shadervar = "curve_height";
	param->effect = this;
	param->box->tooltiptitle = "Sharpness strength ";
	param->box->tooltip = "Strenght of image sharpening - between 0.0 and 40.0 ";
	this->params.push_back(param);
}

DitherEffect::DitherEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Size";
	param->value = 0.71f;
	param->range[0] = 0.449f;
	param->range[1] = 0.982f;
	param->sliding = true;
	param->shadervar = "dither_size";
	param->effect = this;
	param->box->tooltiptitle = "Dithering size ";
	param->box->tooltip = "Size of the image dithering - between 0.1 and 0.97 ";
	this->params.push_back(param);
}

FlipEffect::FlipEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "X";
	param->value = 1.0f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = false;
	param->shadervar = "xflip";
	param->effect = this;
	param->box->tooltiptitle = "X Flip ";
	param->box->tooltip = "Toggles X image flipping: 0 = no flipping, 1 = flipping in X direction ";
	this->params.push_back(param);
	param = new Param;
	param->name = "Y";
	param->value = 0.0f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = false;
	param->shadervar = "yflip";
	param->effect = this;
	param->box->tooltiptitle = "Y Flip ";
	param->box->tooltip = "Toggles Y image flipping: 0 = no flipping, 1 = flipping in Y direction ";
	this->params.push_back(param);
}

MirrorEffect::MirrorEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Xdir";
	param->value = 0.0f;
	param->range[0] = 0.0f;
	param->range[1] = 2.0f;
	param->sliding = false;
	param->shadervar = "xmirror";
	param->effect = this;
	param->box->tooltiptitle = "X Mirror ";
	param->box->tooltip = "Toggles X image mirroring: 0 = left to right mirroring, 1 = no mirroring, 2 = right to left mirroring. ";
	this->params.push_back(param);
	param = new Param;
	param->name = "Ydir";
	param->value = 1.0f;
	param->range[0] = 0.0f;
	param->range[1] = 2.0f;
	param->sliding = false;
	param->shadervar = "ymirror";
	param->effect = this;
	param->box->tooltiptitle = "Y Mirror ";
	param->box->tooltip = "Toggles Y image mirroring: 0 = top to bottom mirroring, 1 = no mirroring, 2 = bottom to top mirroring. ";
	this->params.push_back(param);
}

float RippleEffect::get_speed() { return this->speed; }
float RippleEffect::get_ripplecount() { return this->ripplecount; }
void RippleEffect::set_ripplecount(float count) { this->ripplecount = count; }

int StrobeEffect::get_phase() { return this->phase; }
void StrobeEffect::set_phase(int phase) { this->phase = phase; }



// EFFECTS BELONGING TO A LAYER

Effect *do_add_effect(Layer *lay, EFFECT_TYPE type, int pos, bool comp) {
	// adds an effect of a certain type to a layer at a certain position in its effect list
	// comp is set when the layer belongs to the output layer stacks
	Effect *effect = new_effect(type);

	effect->type = type;
	effect->pos = pos;
	effect->layer = lay;
	
	std::vector<Effect*> &evec = lay->choose_effects();
	evec.insert(evec.begin() + pos, effect);
	bool cat = mainprogram->effcat[lay->deck]->value;
	
	// does scrolling when effect stack reaches bottom of GUI space
	lay->numefflines[cat] += effect->numrows;
	if (lay->numefflines[cat] > 11) {
		int further = (lay->numefflines[cat] - lay->effscroll[cat]) - 11;
		lay->effscroll[cat] = lay->effscroll[cat] + further * (further > 0);
		int linepos = 0;
		for (int i = 0; i < effect->pos; i++) {
			linepos += evec[i]->numrows;
		}
		if (lay->effscroll[cat] > linepos) lay->effscroll[cat] = linepos;
	}
	
	EffectNode *effnode1 = mainprogram->nodesmain->currpage->add_effectnode(effect, lay->node, pos, comp);
	effnode1->align = lay->node;
	effnode1->alignpos = pos;
	lay->node->aligned += 1;
	
	if (pos == evec.size() - 1) {
		if (lay->pos > 0 && lay->blendnode && !cat) {
			lay->blendnode->in2 = nullptr;
			mainprogram->nodesmain->currpage->connect_in2(effnode1, lay->blendnode);
		}
		else if (lay->lasteffnode[cat]->out.size()) {
			lay->lasteffnode[cat]->out[0]->in = nullptr;
			mainprogram->nodesmain->currpage->connect_nodes(effnode1, lay->lasteffnode[cat]->out[0]);
		}
		lay->lasteffnode[cat]->out.clear();
		lay->lasteffnode[cat] = effnode1;
		if (lay->pos == 0 && !cat && lay->effects[1].size() == 0) {
			lay->lasteffnode[1] = lay->lasteffnode[0];
		}
	}

	if (pos == 0) {
		if (evec.size() > 1) {
			EffectNode *effnode2 = evec[pos + 1]->node;
			mainprogram->nodesmain->currpage->connect_nodes(effnode1, effnode2);
		}
		if (!cat) {
			lay->node->out.clear();
			mainprogram->nodesmain->currpage->connect_nodes(lay->node, effnode1);
		}
		else {
			if (lay->pos == 0) {
				mainprogram->nodesmain->currpage->connect_nodes(lay->lasteffnode[0], effnode1);
			}
			else {
				mainprogram->nodesmain->currpage->connect_nodes(lay->blendnode, effnode1);
			}
		}
	}
	else {
		EffectNode *effnode2 = evec[pos - 1]->node;
		if (evec.size() > pos + 1) {
			EffectNode *effnode3 = evec[pos + 1]->node;
			mainprogram->nodesmain->currpage->connect_nodes(effnode1, effnode3);
		}
		effnode2->out.clear();
		mainprogram->nodesmain->currpage->connect_nodes(effnode2, effnode1);
	}
	for (int i = 0; i < effect->params.size(); i++) {
		effect->params[i]->box->tooltip += " - Leftdrag sets value. Doubleclicking allows numeric entry. ";
	}
	/* reminder
	// add parameters to OSC system
	if (lay->numoftypemap.find(effect->type) != lay->numoftypemap.end()) lay->numoftypemap[effect->type]++;
	else lay->numoftypemap[effect->type] = 1;
	for (int i = 0; i < effect->params.size(); i++) {
		std::string deckstr;
		if (lay->deck == 0) deckstr = "/deckA";
		else deckstr = "/deckB";
		std::string compstr;
		if (mainprogram->prevmodus) compstr = "preview";
		else compstr = "output";
		std::string path = "/mix/" + compstr + deckstr + "/layer/" + std::to_string(lay->pos + 1) + "/effect/" + mainprogram->effectsmap[effect->type] + "/" + std::to_string(lay->numoftypemap[effect->type]) + "/" + effect->params[i]->name;
		effect->params[i]->oscpaths.push_back(path);
		printf("osc %s\n", path.c_str());
		fflush(stdout);
		//mainprogram->st->add_method(path, "f", osc_param, effect->params[i]);
	}*/
	
	return effect;
}
Effect* Layer::add_effect(EFFECT_TYPE type, int pos) {
	Effect *eff;

	if (mainprogram->prevmodus) {
		eff = do_add_effect(this, type, pos, false);
	}
	else {
		eff = do_add_effect(this, type, pos, true);
	}
	eff->onoffbutton->layer = this;
	for (int i = 0; i < eff->params.size(); i++) {
		eff->params[i]->deflt = eff->params[i]->value;
	}
	if (this->type == ELEM_FILE) {
        this->type = ELEM_LAYER;
	}
	return eff;
}		

Effect* Layer::replace_effect(EFFECT_TYPE type, int pos) {
	Effect *effect;

	this->delete_effect(pos);
	effect = this->add_effect(type, pos);
	
	return effect;
}

void do_delete_effect(Layer *lay, int pos) {
	
	std::vector<Effect*> &evec = lay->choose_effects();
	bool cat = mainprogram->effcat[lay->deck]->value;
	Effect *effect = evec[pos];
	
	for(int i = pos; i < evec.size(); i++) {
		Effect *eff = evec[i];
		eff->node->alignpos -= 1;
	}
	lay->node->aligned -= 1;
	
	for (int i = 0; i < mainprogram->nodesmain->currpage->nodes.size(); i++) {
		if (evec[pos]->node == mainprogram->nodesmain->currpage->nodes[i]) {
			mainprogram->nodesmain->currpage->nodes.erase(mainprogram->nodesmain->currpage->nodes.begin() + i);
		}
	}
	for (int i = 0; i < mainprogram->nodesmain->currpage->nodescomp.size(); i++) {
		if (evec[pos]->node == mainprogram->nodesmain->currpage->nodescomp[i]) {
			mainprogram->nodesmain->currpage->nodescomp.erase(mainprogram->nodesmain->currpage->nodescomp.begin() + i);
		}
	}
	if (lay->lasteffnode[cat] == evec[pos]->node) {
		if (pos == 0) {
			if (!cat) lay->lasteffnode[cat] = lay->node;
			else {
				if (lay->pos == 0) {
					lay->lasteffnode[0]->out.clear();
					mainprogram->nodesmain->currpage->connect_nodes(lay->lasteffnode[0], lay->lasteffnode[1]->out[0]);
					lay->lasteffnode[1] = lay->lasteffnode[0];
				}
				else {	
					lay->blendnode->out.clear();
					mainprogram->nodesmain->currpage->connect_nodes(lay->blendnode, lay->lasteffnode[1]->out[0]);
					lay->lasteffnode[1] = lay->blendnode;
				}
			}
		}
		else {
			lay->lasteffnode[cat] = evec[pos - 1]->node;
			lay->lasteffnode[cat]->out[0] = evec[pos]->node->out[0];
			if (cat) evec[pos]->node->out[0]->in = lay->lasteffnode[1];
		}
		if (!cat) {
			if (lay->pos == 0) {
                lay->lasteffnode[1] = lay->lasteffnode[0];
				mainprogram->nodesmain->currpage->connect_nodes(lay->lasteffnode[0], evec[pos]->node->out[0]);
			}
            else if (lay->pos = lay->layers->size() - 1){
                lay->lasteffnode[1] = lay->lasteffnode[0];
                mainprogram->nodesmain->currpage->connect_nodes(lay->lasteffnode[0], evec[pos]->node->out[0]);
            }
            else {
                mainprogram->nodesmain->currpage->connect_in2(lay->lasteffnode[0], ((BlendNode*)(evec[pos]->node->out[0])));
            }
		}
	}
	else {
		evec[pos + 1]->node->in = evec[pos]->node->in;
		if (pos != 0) evec[pos - 1]->node->out[0] = evec[pos + 1]->node;
		else {
			if (!cat) lay->node->out.push_back(evec[pos + 1]->node);
			else {
				if (lay->pos == 0) {
					lay->lasteffnode[0]->out.push_back(evec[pos + 1]->node);
				}
				else {
					lay->blendnode->out.push_back(evec[pos + 1]->node);
				}
			}
		}
	}	
	
	lay->node->page->delete_node(evec[pos]->node);
	lay->node->upeffboxes();

	for (int i = 0; i < effect->params.size(); i++) {
	    Param *par = effect->params[i];
	    delete par;
	}


	evec.erase(evec.begin() + pos);
	delete effect;
	
	make_layboxes();
}

void Layer::delete_effect(int pos) {
	do_delete_effect(this, pos);
	if (this->type == ELEM_FILE) this->type = ELEM_LAYER;
}		



// LAYERS BELONGING TO A MIXER

Layer* Mixer::add_layer(std::vector<Layer*> &layers, int pos) {
	bool comp;
	if (mainprogram->prevmodus) comp = false;
	else comp = true;
	Layer *layer = new Layer(comp);
	layer->layers = &layers;
	if (layers == this->layers[0] || layers == this->layers[2]) layer->deck = 0;
	else layer->deck = 1;
	Clip *clip = new Clip;  // empty never-active clip at queue end for adding to queue from the GUI
	clip->insert(layer, layer->clips.end());
	clip->type = ELEM_LAYER;
 	layer->node = mainprogram->nodesmain->currpage->add_videonode(comp);
	layer->node->layer = layer;
	layer->lasteffnode[0] = layer->node;
	if (layers == this->layers[0] || layers == this->layers[2]) {
		layer->genmidibut->value = this->genmidi[0]->value;
	}
	else {
		layer->genmidibut->value = this->genmidi[1]->value;
	}
	
	Layer *testlay = nullptr;
	
	layer->pos = pos;

	layers.insert(layers.begin() + pos, layer);
	for (int i = pos; i < layers.size(); i++) {
		testlay = layers[i];
		testlay->pos = i;
	}

	if (pos > 0) {
		Layer *prevlay = layers[pos - 1];
		BlendNode *bnode = mainprogram->nodesmain->currpage->add_blendnode(MIXING, comp);
		layer->blendnode = bnode;
		Node *node;
		if (prevlay->effects[1].size()) {
			node = prevlay->lasteffnode[1];
		}
		else if (prevlay->pos > 0) {
			node = prevlay->blendnode;
		}
		else {
			node = prevlay->lasteffnode[0];
		}
		mainprogram->nodesmain->currpage->connect_nodes(node, layer->lasteffnode[0], layer->blendnode);
		if (pos == layers.size() - 1) {
            for (int i = 0; i < 4; i++) {
                if (layers == mainmix->layers[i] && mainprogram->nodesmain->mixnodes[0].size()) {
                    mainprogram->nodesmain->currpage->connect_nodes(layer->blendnode,
                                                                    mainprogram->nodesmain->mixnodes[i / 2][i % 2]);
                    break;
                }
            }
		}
		else if (pos < layers.size() - 1) {
			mainprogram->nodesmain->currpage->connect_nodes(layer->blendnode, layers[pos + 1]->blendnode);
		}
	}
	else {
		layer->blendnode = new BlendNode;
		BlendNode *bnode = mainprogram->nodesmain->currpage->add_blendnode(MIXING, comp);
		Layer *nextlay = nullptr;
		if (layers.size() > 1) nextlay = layers[1];
		if (nextlay) {
			mainprogram->nodesmain->currpage->connect_nodes(bnode, nextlay->lasteffnode[0]->out[0]);
			nextlay->lasteffnode[0]->out.clear();
			nextlay->blendnode = bnode;
			if (!nextlay->effects[1].size()) nextlay->lasteffnode[1] = bnode;
			mainprogram->nodesmain->currpage->connect_nodes(layer->node, nextlay->lasteffnode[0], bnode);
		}
	}
	
	if (layer->blendnode->in) {
		if (layer->blendnode->in->type == BLEND) {
			layer->blendnode->box->scrcoords->x1 = layer->blendnode->in->box->scrcoords->x1 + 192;
			layer->blendnode->box->scrcoords->y1 = 500;
		}
		else {
			layer->blendnode->box->scrcoords->x1 = 700;
			layer->blendnode->box->scrcoords->y1 = 500;
		}
	}
	layer->blendnode->box->upscrtovtx();
	
	if (pos == 0) layer->lasteffnode[1] = layer->lasteffnode[0];
	else layer->lasteffnode[1] = layer->blendnode;
	
	make_layboxes();

	return layer;
}

void Mixer::do_deletelay(Layer *testlay, std::vector<Layer*> &layers, bool add) {
    if (testlay == mainmix->currlay[!mainprogram->prevmodus]) {
        if (testlay->pos == 0 && layers.size() == 1) mainmix->currlay[!mainprogram->prevmodus] = nullptr;
        else if (testlay->pos == layers.size() - 1)
            mainmix->currlay[!mainprogram->prevmodus] = layers[testlay->pos - 1];
        else mainmix->currlay[!mainprogram->prevmodus] = layers[testlay->pos + 1];
    }
    if (layers.size() == 1 && add) {
        Layer *lay = mainmix->add_layer(layers, 1);
        if (!mainmix->currlay[!mainprogram->prevmodus]) {
            mainmix->currlay[!mainprogram->prevmodus] = lay;
        }
    }

    if (std::find(mainmix->currlays[!mainprogram->prevmodus].begin(), mainmix->currlays[!mainprogram->prevmodus].end(),
                  testlay) != mainmix->currlays[!mainprogram->prevmodus].end()) {
        mainmix->currlays[!mainprogram->prevmodus].erase(std::find(mainmix->currlays[!mainprogram->prevmodus].begin(),
                                                                   mainmix->currlays[!mainprogram->prevmodus].end(),
                                                                   testlay));
    }
    if (std::find(mainmix->currlays[!mainprogram->prevmodus].begin(), mainmix->currlays[!mainprogram->prevmodus].end(),
                  mainmix->currlay[!mainprogram->prevmodus]) == mainmix->currlays[!mainprogram->prevmodus].end()) {
        if (mainmix->currlay[!mainprogram->prevmodus]) {
            mainmix->currlays[!mainprogram->prevmodus].push_back(mainmix->currlay[!mainprogram->prevmodus]);
        }
    }

    BLEND_TYPE nextbtype;
    Layer *nextlay = nullptr;
    if (layers.size() > testlay->pos + 1) {
        nextlay = layers[testlay->pos + 1];
        nextbtype = nextlay->blendnode->blendtype;
    }
    Layer *prevlay = nullptr;
    if (testlay->pos > 0) {
        prevlay = layers[testlay->pos - 1];
    }

    int size = layers.size();
    for (int i = 0; i < size; i++) {
        if (layers[i] == testlay) {
            layers.erase(layers.begin() + i);
            break;
        }
    }

    if (mainprogram->nodesmain->linked) {
        while (!testlay->clips.empty()) {
            delete testlay->clips.back();
            testlay->clips.pop_back();
        }

        Node *bulasteffnode1out = nullptr;
        if (testlay->pos > 0 && testlay->lasteffnode[1]->out.size())
            bulasteffnode1out = testlay->lasteffnode[1]->out[0];
        while (!testlay->effects[0].empty()) {
            for (int j = 0; j < testlay->effects[0].back()->params.size(); j++) {
                delete testlay->effects[0].back()->params[j];
            }
            mainprogram->deleffects.push_back(testlay->effects[0].back());
            testlay->effects[0].pop_back();
        }
        while (!testlay->effects[1].empty()) {
            for (int j = 0; j < testlay->effects[1].back()->params.size(); j++) {
                delete testlay->effects[1].back()->params[j];
            }
            mainprogram->deleffects.push_back(testlay->effects[1].back());
            testlay->effects[1].pop_back();
        }

        if (testlay->pos > 0 && testlay->blendnode) {
            if (bulasteffnode1out)
                mainprogram->nodesmain->currpage->connect_nodes(prevlay->lasteffnode[1], bulasteffnode1out);
            mainprogram->nodesmain->currpage->delete_node(testlay->blendnode);
            testlay->blendnode = 0;
        } else {
            if (nextlay) {
                nextlay->lasteffnode[0]->out.clear();
                if (nextlay->lasteffnode[1]->out.size())
                    mainprogram->nodesmain->currpage->connect_nodes(nextlay->lasteffnode[0],
                                                                    nextlay->lasteffnode[1]->out[0]);
                nextlay->lasteffnode[1] = nextlay->lasteffnode[0];
                mainprogram->nodesmain->currpage->delete_node(nextlay->blendnode);
                nextlay->blendnode = new BlendNode;
                nextlay->blendnode->blendtype = nextbtype;
            }
        }

        for (int i = testlay->pos; i < layers.size(); i++) {
            layers[i]->pos = i;
        }
    }

    // if layer is active webcam connection: look to activate a mimiclayer
    int pos = std::find(mainprogram->busylayers.begin(), mainprogram->busylayers.end(), testlay) -
              mainprogram->busylayers.begin();
    if (pos != mainprogram->busylayers.size()) {
        bool found = testlay->find_new_live_base(pos);
        if (!found) {
            mainprogram->busylayers.erase(mainprogram->busylayers.begin() + pos);
            mainprogram->busylist.erase(
                    std::find(mainprogram->busylist.begin(), mainprogram->busylist.end(), testlay->filename));
        }
    }

    if (std::find(mainprogram->mimiclayers.begin(), mainprogram->mimiclayers.end(), testlay) != mainprogram->mimiclayers.end()) {
        testlay->texture = -1;
    }

	testlay->closethread = 1;
}

void Mixer::delete_layer(std::vector<Layer*> &layers, Layer *testlay, bool add) {
	/*if (!testlay->dummy) {
        ShelfElement *elem = nullptr;
        if (testlay->prevshelfdragelems.size()) {
            elem = testlay->prevshelfdragelems.back();
        }
		bool ret = this->set_prevshelfdragelem(testlay);
		if (!ret && elem->type == ELEM_DECK) {
			if (elem->launchtype == 2) {
				this->mousedeck = elem->nblayers[0]->deck;
				this->do_save_deck(mainprogram->temppath + "tempdeck_lnch.deck", false, false);
				this->open_deck(mainprogram->temppath + "tempdeck_lnch.deck", false);
			}
		}
		else if (!ret && elem->type == ELEM_MIX) {
			if (elem->launchtype == 2) {
				this->do_save_mix(mainprogram->temppath + "tempdeck_lnch.deck", mainprogram->prevmodus, false);
				this->open_mix(mainprogram->temppath + "tempdeck_lnch.deck", false);
			}
		}
	}*/

	testlay->audioplaying = false;

	if (testlay->mutebut->value) {
		testlay->mutebut->value = false;
		testlay->mute_handle();
	}

    if (testlay->clonesetnr != -1) {
        int clnr = testlay->clonesetnr;
        testlay->clonesetnr = -1;
        testlay->texture = -1;
        mainmix->clonesets[clnr]->erase(testlay);

        if (mainmix->clonesets[clnr]->size() == 1) {
            mainmix->cloneset_destroy(clnr);
        }
    }

    this->do_deletelay(testlay, layers, add);
}



// BASIC LAYER METHODS

Layer::Layer() {
	Layer(true);
}

Layer::Layer(bool comp) {
	this->comp = comp;

    glClearColor(0, 0, 0, 0);

    glGenTextures(1, &this->jpegtex);
    glBindTexture(GL_TEXTURE_2D, this->jpegtex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glGenTextures(1, &this->texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, this->texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	glGenTextures(1, &this->fbotex);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, this->fbotex);
	if (comp) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, mainprogram->ow, mainprogram->oh, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
		//glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, mainprogram->ow, mainprogram->oh);
	}
	else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, mainprogram->ow3, mainprogram->oh3, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
		//glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, mainprogram->ow3, mainprogram->oh3);
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glGenFramebuffers(1, &this->fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, this->fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->fbotex, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    glGenTextures(1, &this->minitex);
    glBindTexture(GL_TEXTURE_2D, this->minitex);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, mainprogram->ow3, mainprogram->oh3);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	this->panbox = new Boxx;
	this->panbox->reserved = true;
	this->panbox->tooltiptitle = "Pan box ";
	this->panbox->tooltip = "Leftclickdrag enables dragging/panning the image around. ";
	this->mutebut = new Button(false);
    this->mutebut->butid = 1;
	this->mutebut->toggle = 1;
	this->mutebut->box->vtxcoords->y1 = 1.0f - mainprogram->layh;
	this->mutebut->box->vtxcoords->w = 0.03f;
	this->mutebut->box->vtxcoords->h = 0.05f;
	this->mutebut->box->reserved = true;
	this->mutebut->layer = this;
	this->mutebut->box->tooltiptitle = "Layer mute ";
    this->mutebut->box->tooltip = "Leftclick temporarily mutes/unmutes this layer. ";
    this->solobut = new Button(false);
    this->solobut->butid = 2;
    this->solobut->toggle = 1;
    this->solobut->box->vtxcoords->y1 = 1.0f - mainprogram->layh;
    this->solobut->box->vtxcoords->w = 0.03f;
    this->solobut->box->vtxcoords->h = 0.05f;
    this->solobut->box->reserved = true;
    this->solobut->layer = this;
    this->solobut->box->tooltiptitle = "Layer solo ";
    this->solobut->box->tooltip = "Leftclick temporarily soloes/unsoloes this layer (all other layers in deck are muted). ";
    this->keepeffbut = new Button(false);
    this->keepeffbut->butid = 3;
    this->keepeffbut->name[0] = "keepeffbut";
    this->keepeffbut->value = mainprogram->keepeffpref;
    this->keepeffbut->toggle = 1;
    this->keepeffbut->box->vtxcoords->y1 = 1.0f - mainprogram->layh;
    this->keepeffbut->box->vtxcoords->w = 0.03f;
    this->keepeffbut->box->vtxcoords->h = 0.05f;
    this->keepeffbut->box->reserved = true;
    this->keepeffbut->layer = this;
    this->keepeffbut->box->tooltiptitle = "Keep effects ";
    this->keepeffbut->box->tooltip = "Leftclick temporarily keeps the effects of this layer (any new video/layerfile loaded will not change the current effect stack of this layer). ";
  	this->queuebut = new Button(false);
    this->queuebut->butid = 4;
    this->queuebut->name[0] = "queuebut";
	this->queuebut->box->vtxcoords->y1 = 1.0f - mainprogram->layh;
	this->queuebut->box->vtxcoords->w = 0.03f;
	this->queuebut->box->vtxcoords->h = 0.05f;
	this->queuebut->box->reserved = true;
	this->queuebut->layer = this;
	this->queuebut->box->tooltiptitle = "Layer clip queue ";
	this->queuebut->box->tooltip = "Leftclick folds/unfolds the layer clip queue. ";
	this->mixbox = new Boxx;
    this->mixbox->tooltiptitle = "Set layer mix mode ";
    this->mixbox->tooltip = "Left or rightclick for choosing how to mix this layer with the previous ones: blendmode or local wipe.  Also accesses colorkeying. ";
    this->colorbox = new Boxx;
    this->colorbox->acolor[3] = 1.0f;
    this->colorbox->tooltiptitle = "Set colorkey color ";
    this->colorbox->tooltip = "Leftclick to set colorkey color.  Either use colorwheel or leftclick anywhere on screen.  Hovering mouse shows color that will be selected. ";
    
    this->shiftx = new Param;
    this->shiftx->name = "shiftx";
    this->shiftx->sliding = true;
    this->shiftx->value = 0.0f;
    this->shiftx->range[0] = -mainprogram->monh / 1.5f;
    this->shiftx->range[1] = mainprogram->monh / 1.5f;
    this->shiftx->box->acolor[0] = 1.0f;
    this->shiftx->box->acolor[1] = 1.0f;
    this->shiftx->box->acolor[2] = 1.0f;
    this->shiftx->box->acolor[3] = 0.5f;
    this->shifty = new Param;
    this->shifty->name = "shifty";
    this->shifty->sliding = true;
    this->shifty->value = 0.0f;
    this->shifty->range[0] = mainprogram->monh / 1.5f;
    this->shifty->range[1] = -mainprogram->monh / 1.5f;
    this->scale = new Param;
    this->scale->value = 1.0f;
    this->scale->range[0] = 0.001f;
    this->scale->range[1] = 1000.0f;
    this->scritch = new Param;
    this->scritch->value = 0.0f;
    this->startframe = new Param;
    this->startframe->value = 0.0f;
    this->startframe->range[0] = 0.0f;
    this->startframe->range[1] = 9999999.0f;
    this->endframe = new Param;
    this->endframe->value = 0.0f;
    this->endframe->range[0] = 0.0f;
    this->endframe->range[1] = 9999999.0f;

    this->chtol = new Param;
    this->chtol->name = "Tolerance";
	this->chtol->value = 0.8f;
	this->chtol->deflt = 0.8f;
	this->chtol->range[0] = -0.1f;
    this->chtol->range[1] = 3.3f;
    this->chtol->shadervar = "colortol";
    this->chtol->box->tooltiptitle = "Set key tolerance ";
    this->chtol->box->tooltip = "Leftdrag to set tolerance (\"spread\") around key color/hue/grayscale.  Doubleclicking allows numeric entry. ";
	this->speed = new Param;
	this->speed->name = "Speed"; 
	this->speed->value = 1.0f;
	this->speed->deflt = 1.0f;
	this->speed->range[0] = 0.0f;
	this->speed->range[1] = 3.33f;
	this->speed->sliding = true;
	this->speed->powertwo = true;
	this->speed->box->acolor[0] = 0.1;
	this->speed->box->acolor[1] = 0.3;
	this->speed->box->acolor[2] = 0.2;
	this->speed->box->acolor[3] = 1.0;
    this->speed->box->tooltiptitle = "Set layer speed ";
	this->speed->box->tooltip = "Leftdrag sets current layer video playing speed factor. Doubleclicking allows numeric entry. ";
	this->opacity = new Param;
	this->opacity->name = "Opacity"; 
	this->opacity->value = 1.0f;
	this->opacity->deflt = 1.0f;
	this->opacity->range[0] = 0.0f;
	this->opacity->range[1] = 1.0f;
	this->opacity->sliding = true;
	this->opacity->box->acolor[0] = 0.1;
	this->opacity->box->acolor[1] = 0.3;
	this->opacity->box->acolor[2] = 0.2;
	this->opacity->box->acolor[3] = 1.0;
    this->opacity->box->tooltiptitle = "Set layer opacity ";
	this->opacity->box->tooltip = "Leftdrag sets current layer opacity. Doubleclicking allows numeric entry. ";
	this->volume = new Param;
	this->volume->name = "Volume"; 
	this->volume->value = 0.0f;
	this->volume->deflt = 0.0f;
	this->volume->range[0] = 0.0f;
	this->volume->range[1] = 1.0f;
	this->volume->sliding = true;
	this->volume->box->acolor[0] = 0.5;
	this->volume->box->acolor[1] = 0.2;
	this->volume->box->acolor[2] = 0.5;
	this->volume->box->acolor[3] = 1.0;
    this->volume->box->tooltiptitle = "Set audio volume ";
	this->volume->box->tooltip = "Leftdrag sets current layer audio volume.  Default is zero.  (Audio support is experimental and often causes strange effects.) Doubleclicking allows numeric entry. ";
	this->playbut = new Button(false);
    this->playbut->butid = 5;
	this->playbut->toggle = 1;
	this->playbut->layer = this;
	this->playbut->box->tooltiptitle = "Toggle play ";
	this->playbut->box->tooltip = "Leftclick toggles current layer video normal play on/off. ";
	this->frameforward = new Button(false);
    this->frameforward->butid = 6;
	this->frameforward->toggle = 0;
	this->frameforward->layer = this;
	this->frameforward->box->tooltiptitle = "Frame forward ";
	this->frameforward->box->tooltip = "Leftclick advances the current layer video by one frame. ";
	this->framebackward = new Button(false);
    this->framebackward->butid = 7;
	this->framebackward->toggle = 0;
	this->framebackward->layer = this;
	this->framebackward->box->tooltiptitle = "Frame backward ";
	this->framebackward->box->tooltip = "Leftclick seeks back in the current layer video by one frame. ";
	this->revbut = new Button(false);
    this->revbut->butid = 8;
	this->revbut->toggle = 1;
	this->revbut->layer = this;
	this->revbut->box->tooltiptitle = "Toggle reverse play ";
	this->revbut->box->tooltip = "Leftclick toggles current layer video reverse play on/off. ";
	this->bouncebut = new Button(false);
    this->bouncebut->butid = 9;
	this->bouncebut->toggle = 1;
	this->bouncebut->layer = this;
	this->bouncebut->box->tooltiptitle = "Toggle bounce play ";
	this->bouncebut->box->tooltip = "Leftclick toggles current layer video bounce play on/off.  Bounce play plays the video first forward than backward. ";
    this->stopbut = new Button(false);
    this->stopbut->butid = 10;
    this->stopbut->toggle = 1;
    this->stopbut->layer = this;
    this->stopbut->box->tooltiptitle = "Stop playback ";
    this->stopbut->box->tooltip = "Leftclick stops current layer playback. ";
    this->lpbut = new Button(false);
    this->lpbut->butid = 11;
    this->lpbut->toggle = 1;
    this->lpbut->value = mainprogram->repeatdefault;
    this->lpbut->layer = this;
    this->lpbut->box->tooltiptitle = "Toggle looped playback ";
    this->lpbut->box->tooltip = "Leftclick toggles current layer video looped playback on/off. ";
	this->genmidibut = new Button(false);
    this->genmidibut->butid = 12;
    this->genmidibut->toggle = 4;
	this->genmidibut->layer = this;
	this->genmidibut->box->tooltiptitle = "Set layer MIDI preset ";
	this->genmidibut->box->tooltip = "Selects (leftclick advances) for this layer which MIDI preset (A, B, C, D or off) is used to control this layers common controls. ";
    this->loopbox = new Boxx;
    this->loopbox->tooltiptitle = "Loop bar ";
    this->loopbox->tooltip = "Loop bar for current layer video.  Green area is looped area, white vertical line is video  .  Leftdrag on bar scrubs video.  When hovering over green area edges, the area turns blue; when this happens ctrl+leftdrag will drag the area edge.  If area is green, ctrl+leftdrag on the area will drag the looparea left/right.  Rightclicking starts a menu allowing to set loop start or end to the current play position. ";
    this->cliploopbox = new Boxx;
    this->cliploopbox->tooltiptitle = "Clip queue loop bar ";
    this->cliploopbox->tooltip = "Loop bar for current layer video when clip queue is shown.  Grey area is looped area, white vertical line is video  .  Leftdrag on bar scrubs video. ";
    this->cliploopbox->acolor[0] = 0.7f;
    this->cliploopbox->acolor[1] = 0.7f;
    this->cliploopbox->acolor[2] = 0.7f;
    this->cliploopbox->acolor[3] = 1.0f;
	this->chdir = new Button(false);
    this->chdir->butid = 13;
	this->chdir->toggle = 1;
	this->chdir->box->tooltiptitle = "Toggle key direction ";
	this->chdir->box->tooltip = "Leftclick toggles key direction: does the previous layer stream image fill up the current layer's color/hue/grayscale or does the current layer fill a color/hue/grayscale in the previous layer stream image. ";
	this->chinv = new Button(false);
    this->chinv->butid = 14;
    this->chinv->toggle = 1;
	this->chinv->box->tooltiptitle = "Toggle key inverse modus ";
	this->chinv->box->tooltip = "Leftclick toggles key inverse modus: either the selected color/hue/grayscale is exchanged or all colors/hues/grayscales but the selected color/hue/grayscale are exchanged. ";
	this->chdir->box->acolor[3] = 1.0f;
	this->chinv->box->acolor[3] = 1.0f;

	this->currclip = new Clip;
	this->currclip->type = ELEM_LAYER;

    GLfloat vcoords[8];
 	GLfloat *p = vcoords;
	GLfloat tcoords[] = {0.0f, 1.0f,
						1.0f, 1.0f,
						0.0f, 0.0f,
						1.0f, 0.0f};

  //GLenum err;
    //while ((err = glGetError()) != GL_NO_ERROR) {
     //   printf("%X\n", err);
    //}

    glGenBuffers(1, &this->vbuf);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbuf);
    glBufferData(GL_ARRAY_BUFFER, 32, vcoords, GL_STATIC_DRAW);
    GLuint tbuf;
    glGenBuffers(1, &tbuf);
    glBindBuffer(GL_ARRAY_BUFFER, tbuf);
    glBufferData(GL_ARRAY_BUFFER, 32, tcoords, GL_STATIC_DRAW);
    glGenVertexArrays(1, &(this->vao));
    glBindVertexArray(this->vao);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbuf);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
    glBindBuffer(GL_ARRAY_BUFFER, tbuf);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
    glBindVertexArray(this->vao);

    glDeleteBuffers(1, &tbuf);

    this->decresult = new frame_result;
    this->remfr[0] = new remaining_frames;
    this->remfr[1] = new remaining_frames;
    this->remfr[2] = new remaining_frames;

    this->currcliptexpath = find_unused_filename("currcliptex", mainprogram->temppath, ".jpg");

    this->triggering = std::thread{ &Layer::trigger, this };
    this->triggering.detach();
    this->decoding = std::thread{ &Layer::get_frame, this };
    this->decoding.detach();

    this->lpst = new LoopStation;
}

Layer::~Layer() {
    delete this->panbox;
    delete this->mutebut;
    delete this->solobut;
    delete this->keepeffbut;
    delete this->queuebut;
    delete this->mixbox;
    delete this->colorbox;
    delete this->shiftx;
    delete this->shifty;
    delete this->scale;
    delete this->scritch;
    delete this->startframe;
    delete this->endframe;
    delete this->chtol;
    delete this->speed;
    delete this->opacity;
    delete this->volume;
    delete this->playbut;
    delete this->frameforward;
    delete this->framebackward;
    delete this->revbut;
    delete this->bouncebut;
    delete this->stopbut;
    delete this->lpbut;
    delete this->genmidibut;
    delete this->loopbox;
    delete this->cliploopbox;
    delete this->chdir;
    delete this->chinv;
    //delete this->currclip;
    for (Clip *clip : this->clips) {
        //delete clip;  reminder
    }
    delete this->decresult;

    glDeleteTextures(1, &this->jpegtex);
    glDeleteTextures(1, &this->fbotex);
    glDeleteTextures(1, &this->minitex);
	glDeleteBuffers(1, &(this->vbuf));
    //glDeleteBuffers(1, &(this->tbuf));
	if (this->fbo != -1) glDeleteFramebuffers(1, &(this->fbo));
	if (this->vao != -1) glDeleteVertexArrays(1, &(this->vao));
    if (this->filename != "") {
        if ((this->type == ELEM_FILE || this->type == ELEM_LAYER)&& this->remfr[0]->data) {
            if ((this->vidformat == 188 || this->vidformat == 187)) {
                // free HAP databuf
                //databufmutex.lock();
                free(this->databuf[0]);
                free(this->databuf[1]);
                free(this->databuf[2]);
                //databufmutex.unlock();
            } else {
                //free this->rgbframe
                av_freep(&this->remfr[0]->data);
                av_freep(&this->remfr[1]->data);
                av_freep(&this->remfr[2]->data);
            }
        }
    }
    if (!this->nopbodel) {
        // dont delete when pbos are copied over
        glDeleteTextures(1, &this->texture);
        if (!this->dummy) {
            WaitBuffer(this->syncobj[this->pbofri]);
            glDeleteBuffers(3, this->pbo);
        }
        delete this->remfr[0];
        delete this->remfr[1];
        delete this->remfr[2];
        av_frame_free(&this->rgbframe[0]);
        av_frame_free(&this->rgbframe[1]);
        av_frame_free(&this->rgbframe[2]);
    }  // reminder

    if (this->video_dec_ctx) {
        avcodec_free_context(&this->video_dec_ctx);
    }
    if (this->video) {
        avformat_close_input(&this->video);
    }
    if (this->videoseek) {
        avformat_close_input(&this->videoseek);
    }

    if (this->node) {
        mainprogram->nodesmain->currpage->delete_node(this->node);
    }
}

Layer* Layer::next() {
	if (std::find(mainmix->layers[0].begin(), mainmix->layers[0].end(), this) != mainmix->layers[0].end()) {
		if (this->pos < mainmix->layers[0].size() - 1) return mainmix->layers[0][this->pos + 1];
	}
	else if (std::find(mainmix->layers[1].begin(), mainmix->layers[1].end(), this) != mainmix->layers[1].end()) {
		 if (this->pos < mainmix->layers[1].size() - 1) return mainmix->layers[1][this->pos + 1];
	}
	else if (std::find(mainmix->layers[2].begin(), mainmix->layers[2].end(), this) != mainmix->layers[2].end()) {
		 if (this->pos < mainmix->layers[2].size() - 1) return mainmix->layers[2][this->pos + 1];
	}
	else if (std::find(mainmix->layers[3].begin(), mainmix->layers[3].end(), this) != mainmix->layers[3].end()) {
		 if (this->pos < mainmix->layers[3].size() - 1) return mainmix->layers[3][this->pos + 1];
	}
	return this;
}	

Layer* Layer::prev() {
	if (this->pos > 0) {
		if (std::find(mainmix->layers[0].begin(), mainmix->layers[0].end(), this) != mainmix->layers[0].end()) return mainmix->layers[0][this->pos - 1];
		else if (std::find(mainmix->layers[1].begin(), mainmix->layers[1].end(), this) != mainmix->layers[1].end()) return mainmix->layers[1][this->pos - 1];
		else if (std::find(mainmix->layers[2].begin(), mainmix->layers[2].end(), this) != mainmix->layers[2].end()) return mainmix->layers[2][this->pos - 1];
		else if (std::find(mainmix->layers[3].begin(), mainmix->layers[3].end(), this) != mainmix->layers[3].end()) return mainmix->layers[3][this->pos - 1];
	}
	return this;
}



// LAYER PROPERTIES

void Layer::initialize(int w, int h) {
    this->initialize(w, h, this->decresult->compression);
}

void Layer::initialize(int w, int h, int compression) {
	this->iw = w;
	this->ih = h;
	if (this->texture != -1) glDeleteTextures(1, &this->texture);
	glGenTextures(1, &this->texture);
	glBindTexture(GL_TEXTURE_2D, this->texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
	int count = 0;
	if (this->vidformat == 188 || this->vidformat == 187) {
		if (compression == 187 || compression == 171) {
			glTexStorage2D(GL_TEXTURE_2D, 1, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, this->iw, this->ih);
		}
		else if (compression == 190) {
			glTexStorage2D(GL_TEXTURE_2D, 1, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, this->iw, this->ih);
		}
	}
	else {
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, this->iw, this->ih);
	}

	if (!this->nonewpbos) {
        // map three buffers persistently for pixel transfer from cpu to gpu
        // using double PBOs for DMA pixel transfer
        if (this->initialized) glDeleteBuffers(3, this->pbo);
        glGenBuffers(3, this->pbo);
        int bsize = w * h * this->bpp;
        int flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
        glBindBuffer(GL_ARRAY_BUFFER, this->pbo[0]);
        if (this->initialized) glUnmapBuffer(GL_ARRAY_BUFFER);
        glBufferStorage(GL_ARRAY_BUFFER, bsize, 0, flags);
        this->mapptr[0] = (GLubyte *) glMapBufferRange(GL_ARRAY_BUFFER, 0, bsize, flags);
        glBindBuffer(GL_ARRAY_BUFFER, this->pbo[1]);
        if (this->initialized) glUnmapBuffer(GL_ARRAY_BUFFER);
        glBufferStorage(GL_ARRAY_BUFFER, bsize, 0, flags);
        this->mapptr[1] = (GLubyte *) glMapBufferRange(GL_ARRAY_BUFFER, 0, bsize, flags);
        glBindBuffer(GL_ARRAY_BUFFER, this->pbo[2]);
        if (this->initialized) glUnmapBuffer(GL_ARRAY_BUFFER);
        glBufferStorage(GL_ARRAY_BUFFER, bsize, 0, flags);
        this->mapptr[2] = (GLubyte *) glMapBufferRange(GL_ARRAY_BUFFER, 0, bsize, flags);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    else this->nonewpbos = false;

    if (!this->initdeck) this->changeinit = -1;
    this->oldvidformat = this->vidformat;
	this->oldcompression = compression;
	this->oldtype = this->type;
	this->initialized = true;
}

void Layer::set_aspectratio(int lw, int lh) {
	float ow, oh;
	if (this->comp) {
		ow = mainprogram->ow;
		oh = mainprogram->oh;
	}
	else {
		ow = mainprogram->ow3;
		oh = mainprogram->oh3;
	}
	int w = ow;
	int h = oh;
	if (this->aspectratio == RATIO_ORIGINAL_INSIDE) {
		if (ow / oh > (float)lw / (float)lh) {
			w = ow * ((float)lw / (float)lh) / (ow / oh);
			h = oh;
		}
		else {
			h = oh * ((float)lh / (float)lw) / (oh / ow);
			w = ow;
		}
	}
	else if (this->aspectratio == RATIO_ORIGINAL_OUTSIDE) {
		if (ow / oh < (float)lw / (float)lh) {
			w = ow * ((float)lw / (float)lh) / (ow / oh);
			h = oh;
		}
		else {
			h = oh * ((float)lh / (float)lw) / (oh / ow);
			w = ow;
		}
	}
	GLuint tex;
	tex = set_texes(this->fbotex, &this->fbo, w, h);
	this->fbotex = tex;
	/*for (int i = 0; i < this->effects[0].size(); i++) {
		tex = set_texes(this->effects[0][i]->fbotex, &this->effects[0][i]->fbo, w, h);
		this->effects[0][i]->fbotex = tex;
	}
	for (int i = 0; i < this->effects[1].size(); i++) {
		tex = set_texes(this->effects[1][i]->fbotex, &this->effects[1][i]->fbo, w, h);
		this->effects[1][i]->fbotex = tex;
	}*/

	if (this->type == ELEM_IMAGE || this->type == ELEM_LIVE) {
		if (this->numf == 0) this->remfr[this->pbodi]->newdata = true;
	}
}

void Layer::inhibit() {
    this->deautomate();
    while (!this->effects[0].empty()) {
        mainprogram->nodesmain->currpage->delete_node(this->effects[0].back()->node);
        for (int j = 0; j < this->effects[0].back()->params.size(); j++) {
            delete this->effects[0].back()->params[j];
        }
        delete this->effects[0].back();
        this->effects[0].pop_back();
    }

    this->lasteffnode[1]->out.clear();
    mainprogram->nodesmain->currpage->delete_node(this->blendnode);
    mainprogram->nodesmain->currpage->delete_node(this->node);
    this->olddeckspeed = mainmix->deckspeed[this->comp][this->deck]->value;
}

void Layer::deautomate() {
    for (int m = 0; m < 2; m++) {
        for (int i = 0; i < this->effects[m].size(); i++) {
            for (int j = 0; j < this->effects[m][i]->params.size(); j++) {
                this->effects[m][i]->params[j]->deautomate();
            }
        }
    }
    this->speed->deautomate();
    this->opacity->deautomate();
    this->volume->deautomate();
    this->scritch->deautomate();
    this->shiftx->deautomate();
    this->shifty->deautomate();
    this->scale->deautomate();
    this->blendnode->mixfac->deautomate();
    this->blendnode->wipex->deautomate();
    this->blendnode->wipey->deautomate();

    for (int i = 0; i < loopstation->allbuttons.size(); i++) {
        if (this == loopstation->allbuttons[i]->layer) loopstation->allbuttons[i]->deautomate();
    }
}


// LAYER CLONING

void Mixer::cloneset_destroy(int clnr) {
    std::unordered_set<Layer*>::iterator it2;
    for (it2 = mainmix->clonesets[clnr]->begin(); it2 != mainmix->clonesets[clnr]->end(); it2++) {
        Layer *lay = *it2;
        lay->clonesetnr = -1;
        lay->isclone = false;
    }
    mainmix->firstlayers.erase(clnr);
    mainmix->clonesets.erase(clnr);
}

void Layer::set_clones() {
	if (this->clonesetnr != -1) {
        std::unordered_set<Layer *>::iterator it;
        for (it = mainmix->clonesets[this->clonesetnr]->begin();
            it != mainmix->clonesets[this->clonesetnr]->end(); it++) {
            Layer *lay = *it;
            if (lay == this) continue;
            lay->speed->value = this->speed->value;
            lay->opacity->value = this->opacity->value;
            lay->playbut->value = this->playbut->value;
            lay->revbut->value = this->revbut->value;
            lay->bouncebut->value = this->bouncebut->value;
            lay->genmidibut->value = this->genmidibut->value;
            lay->frame = this->frame;
            lay->startframe->value = this->startframe->value;
            lay->endframe->value = this->endframe->value;
            lay->vidformat = this->vidformat;
		}
	}
}

Layer* Layer::clone() {
	std::vector<Layer*>& lvec = choose_layers(this->deck);
	Layer* dlay = mainmix->add_layer(lvec, this->pos + 1);
	dlay->isclone = true;
	mainmix->set_values(dlay, this, true);
	dlay->pos = this->pos + 1;
	dlay->blendnode->blendtype = this->blendnode->blendtype;
	dlay->blendnode->mixfac->value = this->blendnode->mixfac->value;
	dlay->blendnode->chred = this->blendnode->chred;

	dlay->blendnode->chgreen = this->blendnode->chgreen;
	dlay->blendnode->chblue = this->blendnode->chblue;
	dlay->blendnode->wipetype = this->blendnode->wipetype;
	dlay->blendnode->wipedir = this->blendnode->wipedir;
	dlay->blendnode->wipex->value = this->blendnode->wipex->value;
	dlay->blendnode->wipey->value = this->blendnode->wipey->value;

	int buval = mainprogram->effcat[dlay->deck]->value;
	mainprogram->effcat[dlay->deck]->value = 0;
	for (int i = 0; i < this->effects[0].size(); i++) {
		Effect* eff = dlay->add_effect(this->effects[0][i]->type, i);
		for (int j = 0; j < this->effects[0][i]->params.size(); j++) {
			Param* par = this->effects[0][i]->params[j];
			Param* cpar = eff->params[j];
			cpar->value = par->value;
			cpar->midi[0] = par->midi[0];
			cpar->midi[1] = par->midi[1];
			cpar->effect = eff;
			if (loopstation->parelemmap.find(par) != loopstation->parelemmap.end()) {
			    if (loopstation->parelemmap[par]) {
                    for (int k = 0; k < loopstation->parelemmap[par]->eventlist.size(); k++) {
                        LoopStationElement *elem = loopstation->parelemmap[par];
                        std::tuple<long long, Param *, Button *, float> event1 = elem->eventlist[k];
                        if (par == std::get<1>(event1)) {
                            std::tuple<long long, Param *, Button *, float> event2;
                            event2 = std::make_tuple(std::get<0>(event1), cpar, nullptr, std::get<3>(event1));
                            elem->eventlist.insert(elem->eventlist.begin() + k + 1, event2);
                            elem->params.emplace(cpar);
                            elem->layers.emplace(cpar->effect->layer);
                            loopstation->parelemmap[par] = elem;
                            cpar->box->acolor[0] = elem->colbox->acolor[0];
                            cpar->box->acolor[1] = elem->colbox->acolor[1];
                            cpar->box->acolor[2] = elem->colbox->acolor[2];
                            cpar->box->acolor[3] = elem->colbox->acolor[3];
                        }
                    }
                }
			}
		}
        Button* but = this->effects[0][i]->onoffbutton;
        Button* cbut = eff->onoffbutton;
        cbut->value = but->value;
        if (loopstation->butelemmap.find(but) != loopstation->butelemmap.end()) {
            if (loopstation->butelemmap[but] != nullptr) {
                for (int k = 0; k < loopstation->butelemmap[but]->eventlist.size(); k++) {
                    LoopStationElement *elem = loopstation->butelemmap[but];
                    std::tuple<long long, Param *, Button *, float> event1 = elem->eventlist[k];
                    if (but == std::get<2>(event1)) {
                        std::tuple<long long, Param *, Button *, float> event2;
                        event2 = std::make_tuple(std::get<0>(event1), nullptr, cbut, std::get<3>(event1));
                        elem->eventlist.insert(elem->eventlist.begin() + k + 1, event2);
                        elem->buttons.emplace(cbut);
                        if (but->layer) elem->layers.emplace(cbut->layer);
                        loopstation->butelemmap[but] = elem;
                        cbut->box->acolor[0] = elem->colbox->acolor[0];
                        cbut->box->acolor[1] = elem->colbox->acolor[1];
                        cbut->box->acolor[2] = elem->colbox->acolor[2];
                        cbut->box->acolor[3] = elem->colbox->acolor[3];
                    }
                }
            }
        }
        Param* par = this->effects[0][i]->drywet;
        Param* cpar = eff->drywet;
        cpar->value = par->value;
        if (loopstation->parelemmap.find(par) != loopstation->parelemmap.end()) {
            for (int k = 0; k < loopstation->parelemmap[par]->eventlist.size(); k++) {
                LoopStationElement* elem = loopstation->parelemmap[par];
                std::tuple<long long, Param*, Button*, float> event1 = elem->eventlist[k];
                if (par == std::get<1>(event1)) {
                    std::tuple<long long, Param*, Button*, float> event2;
                    event2 = std::make_tuple(std::get<0>(event1), cpar, nullptr, std::get<3>(event1));
                    elem->eventlist.insert(elem->eventlist.begin() + k + 1, event2);
                    elem->params.emplace(cpar);
                    elem->layers.emplace(cpar->effect->layer);
                    loopstation->parelemmap[par] = elem;
                    cpar->box->acolor[0] = elem->colbox->acolor[0];
                    cpar->box->acolor[1] = elem->colbox->acolor[1];
                    cpar->box->acolor[2] = elem->colbox->acolor[2];
                    cpar->box->acolor[3] = elem->colbox->acolor[3];
                }
            }
        }
	}
	mainprogram->effcat[dlay->deck]->value = buval;

    Param* partransfers[12][2] = { {this->speed, dlay->speed}, {this->opacity, dlay->opacity}, {this->volume, dlay->volume}, {this->scritch, dlay->scritch}, {this->startframe, dlay->startframe}, {this->endframe, dlay->endframe}, {this->blendnode->mixfac, dlay->blendnode->mixfac}, {this->shiftx, dlay->shiftx}, {this->shifty, dlay->shifty}, {this->blendnode->wipex, dlay->blendnode->wipex}, {this->blendnode->wipey, dlay->blendnode->wipey}, {this->scale, dlay->scale} };
    for (int i = 0; i < 12; i++) {
        Param* par = partransfers[i][0];
        Param* cpar = partransfers[i][1];
        if (loopstation->parelemmap.find(par) != loopstation->parelemmap.end()) {
            if (loopstation->parelemmap[par]) {
                for (int k = 0; k < loopstation->parelemmap[par]->eventlist.size(); k++) {
                    LoopStationElement *elem = loopstation->parelemmap[par];
                    std::tuple<long long, Param *, Button *, float> event1 = elem->eventlist[k];
                    if (par == std::get<1>(event1)) {
                        std::tuple<long long, Param *, Button *, float> event2;
                        event2 = std::make_tuple(std::get<0>(event1), cpar, nullptr, std::get<3>(event1));
                        elem->eventlist.insert(elem->eventlist.begin() + k + 1, event2);
                        elem->params.emplace(cpar);
                        elem->layers.emplace(dlay);
                        loopstation->parelemmap[par] = elem;
                        cpar->box->acolor[0] = elem->colbox->acolor[0];
                        cpar->box->acolor[1] = elem->colbox->acolor[1];
                        cpar->box->acolor[2] = elem->colbox->acolor[2];
                        cpar->box->acolor[3] = elem->colbox->acolor[3];
                    }
                }
            }
        }
    }
    Button* buttransfers[12][2] = { {this->mutebut, dlay->mutebut}, {this->solobut, dlay->solobut}, {this->keepeffbut, dlay->keepeffbut}, {this->queuebut, dlay->queuebut}, {this->playbut, dlay->playbut}, {this->bouncebut, dlay->bouncebut}, {this->revbut, dlay->revbut}, {this->frameforward, dlay->frameforward}, {this->framebackward, dlay->framebackward}, {this->genmidibut, dlay->genmidibut}, {this->lpbut, dlay->lpbut}, {this->stopbut, dlay->stopbut} };
    for (int i = 0; i < 12; i++) {
        Button* but = buttransfers[i][0];
        Button* cbut = buttransfers[i][1];
        if (loopstation->butelemmap.find(but) != loopstation->butelemmap.end()) {
            if (loopstation->butelemmap[but]) {
                for (int k = 0; k < loopstation->butelemmap[but]->eventlist.size(); k++) {
                    LoopStationElement *elem = loopstation->butelemmap[but];
                    std::tuple<long long, Param *, Button *, float> event1 = elem->eventlist[k];
                    if (but == std::get<2>(event1)) {
                        std::tuple<long long, Param *, Button *, float> event2;
                        event2 = std::make_tuple(std::get<0>(event1), nullptr, cbut, std::get<3>(event1));
                        elem->eventlist.insert(elem->eventlist.begin() + k + 1, event2);
                        elem->buttons.emplace(cbut);
                        if (but->layer) elem->layers.emplace(cbut->layer);
                        loopstation->butelemmap[but] = elem;
                        cbut->box->acolor[0] = elem->colbox->acolor[0];
                        cbut->box->acolor[1] = elem->colbox->acolor[1];
                        cbut->box->acolor[2] = elem->colbox->acolor[2];
                        cbut->box->acolor[3] = elem->colbox->acolor[3];
                    }
                }
            }
        }
    }

	dlay->decresult->width = -1;

	return dlay;
}



// LAYER DISPLAY

void make_layboxes() {

	for (int i = 0; i < mainprogram->nodesmain->pages.size(); i++) {
		for (int j = 0; j < mainprogram->nodesmain->pages[i]->nodescomp.size(); j++) {
			Node *node = mainprogram->nodesmain->pages[i]->nodescomp[j];

			if (mainprogram->nodesmain->linked) {
				if (node->type == VIDEO) {
					VideoNode *vnode = (VideoNode*)node;
                    /*bool found = 0;
                    if (std::find(mainmix->layers[0].begin(), mainmix->layers[0].begin(), vnode->layer) != mainmix->layers[0].end())
                        found = true;
                    if (std::find(mainmix->layers[1].begin(), mainmix->layers[1].begin(), vnode->layer) != mainmix->layers[1].end())
                        found = true;
                    if (std::find(mainmix->layers[2].begin(), mainmix->layers[2].begin(), vnode->layer) != mainmix->layers[2].end())
                        found = true;
                    if (std::find(mainmix->layers[3].begin(), mainmix->layers[3].begin(), vnode->layer) != mainmix->layers[3].end())
                        found = true;
                    if (!found) continue;*/

                    if (vnode->layer->closethread > 0) continue;
					float xoffset;
					int deck;
					if (std::find(mainmix->layers[2].begin(), mainmix->layers[2].end(), vnode->layer) != mainmix->layers[2].end()) {
						xoffset = 0.0f;
						deck = 0;
					}
					else {
						xoffset = 1.0f;
						deck = 1;
					}
                    float pixelw = 2.0f / glob->w;
                    float pixelh = 2.0f / glob->h;
					int lpos = vnode->layer->pos - mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos;
					if (lpos < 0 || lpos > 2) vnode->vidbox->vtxcoords->x1 = 2.0f;
					else vnode->vidbox->vtxcoords->x1 = -1.0f + (float)(lpos % 3) * mainprogram->layw + mainprogram->numw + xoffset + pixelw;
					vnode->vidbox->vtxcoords->y1 = 1.0f - mainprogram->layh + pixelh;
					vnode->vidbox->vtxcoords->w = mainprogram->layw - 2.0f * pixelw;
					vnode->vidbox->vtxcoords->h = mainprogram->layh - 2.0f * pixelh;
					vnode->vidbox->upvtxtoscr();

					vnode->vidbox->lcolor[0] = 0.0;
					vnode->vidbox->lcolor[1] = 0.0;
					vnode->vidbox->lcolor[2] = 0.0;
					vnode->vidbox->lcolor[3] = 1.0;
                    if (!vnode->layer->initdeck) {
                        if (vnode->layer->effects[0].size() == 0) {
                            vnode->vidbox->tex = vnode->layer->fbotex;
                        } else {
                            vnode->vidbox->tex = vnode->layer->effects[0][vnode->layer->effects[0].size() - 1]->fbotex;
                        }
                    }
                    else {
                        bool dummy = false;
                    }
                    if (mainprogram->effcat[vnode->layer->deck]->value == 1) {
                        if ((vnode)->layer == mainmix->currlay[!mainprogram->prevmodus]) {
                            if ((vnode)->layer->effects[1].size() == 0) {
                                vnode->vidbox->tex = vnode->layer->blendnode->fbotex;
                            } else {
                                vnode->vidbox->tex = vnode->layer->effects[1][vnode->layer->effects[1].size() -
                                                                              1]->fbotex;
                            }
                        }
                    }
				}
				else continue;
			}
		}
	}
	for (int i = 0; i < mainprogram->nodesmain->pages.size(); i++) {
		for (int j = 0; j < mainprogram->nodesmain->pages[i]->nodes.size(); j++) {
			Node *node = mainprogram->nodesmain->pages[i]->nodes[j];
			if (mainprogram->nodesmain->linked) {
				if (node->type == VIDEO) {
					if (((VideoNode*)node)->layer) {  // reminder: trick, not solution...
						VideoNode* vnode = (VideoNode*)node;
                        if (vnode->layer->closethread > 0) continue;
						float xoffset;
						int deck;
						if (vnode->layer->deck == 0) {
							xoffset = 0.0f;
							deck = 0;
						}
						else {
							xoffset = 1.0f;
							deck = 1;
						}
                        float pixelw = 2.0f / glob->w;
                        float pixelh = 2.0f / glob->h;
						int lpos = vnode->layer->pos - mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos;
						if (lpos < 0 || lpos > 2) vnode->vidbox->vtxcoords->x1 = 2.0f;
						else vnode->vidbox->vtxcoords->x1 = -1.0f + (float)(lpos % 3) * mainprogram->layw + mainprogram->numw + xoffset + pixelw;
						vnode->vidbox->vtxcoords->y1 = 1.0f - mainprogram->layh + pixelh;
						vnode->vidbox->vtxcoords->w = mainprogram->layw - 2.0f * pixelw;
						vnode->vidbox->vtxcoords->h = mainprogram->layh - 2.0f * pixelh;
						vnode->vidbox->upvtxtoscr();

						vnode->vidbox->lcolor[0] = 0.0;
						vnode->vidbox->lcolor[1] = 0.0;
						vnode->vidbox->lcolor[2] = 0.0;
						vnode->vidbox->lcolor[3] = 1.0;
                        if (!vnode->layer->initdeck) {
                            if ((vnode)->layer->effects[0].size() == 0) {
                                vnode->vidbox->tex = vnode->layer->fbotex;
                            } else {
                                vnode->vidbox->tex = vnode->layer->effects[0][vnode->layer->effects[0].size() -
                                                                              1]->fbotex;
                            }
                        }
                        if (vnode->layer == mainmix->currlay[!mainprogram->prevmodus] && mainprogram->effcat[vnode->layer->deck]->value == 1) {
                            if ((vnode)->layer->effects[1].size() == 0) {
                                vnode->vidbox->tex = vnode->layer->blendnode->fbotex;
                            }
                            else {
                                vnode->vidbox->tex = vnode->layer->effects[1][vnode->layer->effects[1].size() - 1]->fbotex;
                            }
                        }
					}
				}
				else continue;
			}
			else {
				if (node->monitor != -1) {
					node->monbox->vtxcoords->x1 = -1.0f + (float)(node->monitor % 6) * mainprogram->layw + mainprogram->numw;
					if (node->type == VIDEO) {
						node->monbox->tex = ((VideoNode*)node)->layer->fbotex;
					}
					else if (node->type == EFFECT) {
						node->monbox->tex = ((EffectNode*)node)->effect->fbotex;
					}
					else if (node->type == MIX) {
						node->monbox->tex = ((MixNode*)node)->mixtex;
					}
					else if (node->type == BLEND) {
						node->monbox->tex = ((BlendNode*)node)->fbotex;
					}
				}
				else continue;
			}
			node->monbox->vtxcoords->y1 = 1.0f - mainprogram->layh;
			node->monbox->vtxcoords->w = mainprogram->layw;
			node->monbox->vtxcoords->h = mainprogram->layh;
			node->monbox->upvtxtoscr();

			node->monbox->lcolor[0] = 1.0;
			node->monbox->lcolor[1] = 1.0;
			node->monbox->lcolor[2] = 1.0;
			node->monbox->lcolor[3] = 1.0;
		}
	}

	float xoffset;
	if (mainprogram->nodesmain->linked) {
        int offs = 2;
        if (mainprogram->prevmodus) offs = 0;
		for (int j = offs; j < 2 + offs; j++) {
			std::vector<Layer*> lvec;
			if (j == 0) {
				lvec = mainmix->layers[0];
                //if (lvec == skip) continue;
				xoffset = 0.0f;
			}
			else if (j == 1) {
				lvec = mainmix->layers[1];
                //if (lvec == skip) continue;
				xoffset = 1.0f - 0.2f;
			}
			else if (j == 2) {
				lvec = mainmix->layers[2];
                //if (lvec == skip) continue;
				xoffset = 0.0f;
			}
			else if (j == 3) {
				lvec = mainmix->layers[3];
                //if (lvec == skip) continue;
				xoffset = 1.0f - 0.2f;
			}
			for (int i = 0; i < lvec.size(); i++) {
				Layer *testlay = lvec[i];
				//testlay->node->upeffboxes();
				// set mixbox position and dimensions
                testlay->mixbox->lcolor[0] = 0.7;
                testlay->mixbox->lcolor[1] = 0.7;
                testlay->mixbox->lcolor[2] = 0.7;
                testlay->mixbox->lcolor[3] = 1.0;
                int sp = mainmix->scenes[testlay->deck][mainmix->currscene[testlay->deck]]->scrollpos;
                if (testlay->pos - sp == 2 && testlay->deck == 1) xoffset -= 0.1f;
				testlay->mixbox->vtxcoords->x1 = -1.0f + mainprogram->numw + xoffset + ((testlay->pos - sp) % 3) * mainprogram->layw;
				testlay->mixbox->vtxcoords->y1 = 1.0f - mainprogram->layh - 0.135f;
				testlay->mixbox->vtxcoords->w = 0.075f;
				testlay->mixbox->vtxcoords->h = 0.075f;
				testlay->mixbox->upvtxtoscr();
                testlay->colorbox->lcolor[0] = 0.7;
                testlay->colorbox->lcolor[1] = 0.7;
                testlay->colorbox->lcolor[2] = 0.7;
                testlay->colorbox->lcolor[3] = 1.0;
				testlay->colorbox->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + 0.105f;
				testlay->colorbox->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1;
				testlay->colorbox->vtxcoords->w = 0.075f;
				testlay->colorbox->vtxcoords->h = 0.075f;
				testlay->colorbox->upvtxtoscr();

				// shift effectboxes and parameterboxes according to position in stack and scrollposition of stack
				std::vector<Effect*> &evec = testlay->choose_effects();
				Effect *prevmodus = nullptr;
				for (int j = 0; j < evec.size(); j++) {
					Effect *eff = evec[j];
					eff->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + 0.075f;
					eff->onoffbutton->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + 0.0375f;
					eff->drywet->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1;
					float dy;
					if (prevmodus) {
						if (prevmodus->params.size() < 4) {
							eff->box->vtxcoords->y1 = prevmodus->box->vtxcoords->y1 - 0.075f;
						}
						else if (prevmodus->params.size() > 5) {
							eff->box->vtxcoords->y1 = prevmodus->box->vtxcoords->y1 - 0.225f;
						}
						else {
							eff->box->vtxcoords->y1 = prevmodus->box->vtxcoords->y1 - 0.15f;
						}
					}
					else {
						eff->box->vtxcoords->y1 = 1.0 - mainprogram->layh - 0.435f + (0.075f *
						        testlay->effscroll[mainprogram->effcat[testlay->deck]->value]);
					}
					eff->box->upvtxtoscr();
					eff->onoffbutton->box->vtxcoords->y1 = eff->box->vtxcoords->y1;
					eff->onoffbutton->box->upvtxtoscr();
					eff->drywet->box->vtxcoords->y1 = eff->box->vtxcoords->y1;
					eff->drywet->box->upvtxtoscr();
					prevmodus = eff;
				}

				// Make GUI box of mixing factor slider
                testlay->blendnode->box->lcolor[0] = 0.7;
                testlay->blendnode->box->lcolor[1] = 0.7;
                testlay->blendnode->box->lcolor[2] = 0.7;
                testlay->blendnode->box->lcolor[3] = 1.0;
				testlay->blendnode->mixfac->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + 0.12f;
				testlay->blendnode->mixfac->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1;
				testlay->blendnode->mixfac->box->vtxcoords->w = mainprogram->layw * 1.5f * 0.25f;
				testlay->blendnode->mixfac->box->vtxcoords->h = 0.075f;
				testlay->blendnode->mixfac->box->upvtxtoscr();

				// Make GUI box of video speed slider
                testlay->speed->box->lcolor[0] = 0.7;
                testlay->speed->box->lcolor[1] = 0.7;
                testlay->speed->box->lcolor[2] = 0.7;
                testlay->speed->box->lcolor[3] = 1.0;
				testlay->speed->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1;
				testlay->speed->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - 0.075f;
				testlay->speed->box->vtxcoords->w = mainprogram->layw * 1.5f * 0.5f;
				testlay->speed->box->vtxcoords->h = 0.075f;
				testlay->speed->box->upvtxtoscr();

				// GUI box of layer opacity slider
                testlay->opacity->box->lcolor[0] = 0.7;
                testlay->opacity->box->lcolor[1] = 0.7;
                testlay->opacity->box->lcolor[2] = 0.7;
                testlay->opacity->box->lcolor[3] = 1.0;
				testlay->opacity->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1;
				testlay->opacity->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - 0.225f;
				testlay->opacity->box->vtxcoords->w = mainprogram->layw * 1.5f * 0.25f;
				testlay->opacity->box->vtxcoords->h = 0.075f;
				testlay->opacity->box->upvtxtoscr();

				// GUI box of layer audio volume slider
				testlay->volume->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + mainprogram->layw * 1.5f * 0.30f;
				testlay->volume->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - 0.225f;
				testlay->volume->box->vtxcoords->w = mainprogram->layw * 1.5f * 0.25f;
				testlay->volume->box->vtxcoords->h = 0.075f;
				testlay->volume->box->upvtxtoscr();

				// GUI box of play video button
                testlay->playbut->box->lcolor[0] = 0.7;
                testlay->playbut->box->lcolor[1] = 0.7;
                testlay->playbut->box->lcolor[2] = 0.7;
                testlay->playbut->box->lcolor[3] = 1.0;
				testlay->playbut->box->acolor[0] = 0.5;
				testlay->playbut->box->acolor[1] = 0.2;
				testlay->playbut->box->acolor[2] = 0.5;
				testlay->playbut->box->acolor[3] = 1.0;
				testlay->playbut->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + mainprogram->layw * 1.5f * 0.5f
				        + 0.132f;
				testlay->playbut->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - 0.075f;
				testlay->playbut->box->vtxcoords->w = 0.0375f;
				testlay->playbut->box->vtxcoords->h = 0.075f;
				testlay->playbut->box->upvtxtoscr();

				// GUI box of bounce play video button
                testlay->bouncebut->box->lcolor[0] = 0.7;
                testlay->bouncebut->box->lcolor[1] = 0.7;
                testlay->bouncebut->box->lcolor[2] = 0.7;
                testlay->bouncebut->box->lcolor[3] = 1.0;
				testlay->bouncebut->box->acolor[0] = 0.5;
				testlay->bouncebut->box->acolor[1] = 0.2;
				testlay->bouncebut->box->acolor[2] = 0.5;
				testlay->bouncebut->box->acolor[3] = 1.0;
				testlay->bouncebut->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + mainprogram->layw * 1.5f * 0.5f + 0.087f;
				testlay->bouncebut->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - 0.075f;
				testlay->bouncebut->box->vtxcoords->w = 0.045f;
				testlay->bouncebut->box->vtxcoords->h = 0.075f;
				testlay->bouncebut->box->upvtxtoscr();

				// GUI box of reverse play video button
                testlay->revbut->box->lcolor[0] = 0.7;
                testlay->revbut->box->lcolor[1] = 0.7;
                testlay->revbut->box->lcolor[2] = 0.7;
                testlay->revbut->box->lcolor[3] = 1.0;
				testlay->revbut->box->acolor[0] = 0.5;
				testlay->revbut->box->acolor[1] = 0.2;
				testlay->revbut->box->acolor[2] = 0.5;
				testlay->revbut->box->acolor[3] = 1.0;
				testlay->revbut->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + mainprogram->layw * 1.5f * 0.5f + 0.0495f;
				testlay->revbut->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - 0.075f;
				testlay->revbut->box->vtxcoords->w = 0.0375f;
				testlay->revbut->box->vtxcoords->h = 0.075f;
				testlay->revbut->box->upvtxtoscr();

				// GUI box of video frame forward button
                testlay->frameforward->box->lcolor[0] = 0.7;
                testlay->frameforward->box->lcolor[1] = 0.7;
                testlay->frameforward->box->lcolor[2] = 0.7;
                testlay->frameforward->box->lcolor[3] = 1.0;
				testlay->frameforward->box->acolor[0] = 0.3;
				testlay->frameforward->box->acolor[1] = 0.3;
				testlay->frameforward->box->acolor[2] = 0.6;
				testlay->frameforward->box->acolor[3] = 1.0;
				testlay->frameforward->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + mainprogram->layw * 1.5f *
				        0.5f + 0.1695f;
				testlay->frameforward->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - 0.075f;
				testlay->frameforward->box->vtxcoords->w = 0.0375f;
				testlay->frameforward->box->vtxcoords->h = 0.075f;
				testlay->frameforward->box->upvtxtoscr();

				// GUI box of video frame backward button
                testlay->framebackward->box->lcolor[0] = 0.7;
                testlay->framebackward->box->lcolor[1] = 0.7;
                testlay->framebackward->box->lcolor[2] = 0.7;
                testlay->framebackward->box->lcolor[3] = 1.0;
				testlay->framebackward->box->acolor[0] = 0.3;
				testlay->framebackward->box->acolor[1] = 0.3;
				testlay->framebackward->box->acolor[2] = 0.3;
				testlay->framebackward->box->acolor[3] = 1.0;
				testlay->framebackward->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + mainprogram->layw * 1.5f *
				        0.5f + 0.012f;
				testlay->framebackward->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - 0.075f;
				testlay->framebackward->box->vtxcoords->w = 0.0375f;
				testlay->framebackward->box->vtxcoords->h = 0.075f;
				testlay->framebackward->box->upvtxtoscr();

                // GUI box of reverse play video button
                testlay->lpbut->box->lcolor[0] = 0.7;
                testlay->lpbut->box->lcolor[1] = 0.7;
                testlay->lpbut->box->lcolor[2] = 0.7;
                testlay->lpbut->box->lcolor[3] = 1.0;
                testlay->lpbut->box->acolor[0] = 0.5;
                testlay->lpbut->box->acolor[1] = 0.2;
                testlay->lpbut->box->acolor[2] = 0.5;
                testlay->lpbut->box->acolor[3] = 1.0;
                testlay->lpbut->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + mainprogram->layw * 1.5f * 0.5f +
                                                     0.2445f;
                testlay->lpbut->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - 0.075f;
                testlay->lpbut->box->vtxcoords->w = 0.0375f;
                testlay->lpbut->box->vtxcoords->h = 0.075f;
                testlay->lpbut->box->upvtxtoscr();

                // GUI box of reverse play video button
                testlay->stopbut->box->lcolor[0] = 0.7;
                testlay->stopbut->box->lcolor[1] = 0.7;
                testlay->stopbut->box->lcolor[2] = 0.7;
                testlay->stopbut->box->lcolor[3] = 1.0;
                testlay->stopbut->box->acolor[0] = 0.3;
                testlay->stopbut->box->acolor[1] = 0.3;
                testlay->stopbut->box->acolor[2] = 0.3;
                testlay->stopbut->box->acolor[3] = 1.0;
                testlay->stopbut->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + mainprogram->layw * 1.5f * 0.5f +
                                                     0.207f;
                testlay->stopbut->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - 0.075f;
                testlay->stopbut->box->vtxcoords->w = 0.0375f;
                testlay->stopbut->box->vtxcoords->h = 0.075f;
                testlay->stopbut->box->upvtxtoscr();

                // GUI box of specific general midi set for layer switch
                testlay->genmidibut->box->lcolor[0] = 0.7;
                testlay->genmidibut->box->lcolor[1] = 0.7;
                testlay->genmidibut->box->lcolor[2] = 0.7;
                testlay->genmidibut->box->lcolor[3] = 1.0;
                testlay->genmidibut->box->acolor[0] = 0.5;
                testlay->genmidibut->box->acolor[1] = 0.2;
                testlay->genmidibut->box->acolor[2] = 0.5;
                testlay->genmidibut->box->acolor[3] = 1.0;
				testlay->genmidibut->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + mainprogram->layw * 1.5f *
				        0.5f + 0.282f;
				testlay->genmidibut->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - 0.075f;
				testlay->genmidibut->box->vtxcoords->w = 0.0375f;
				testlay->genmidibut->box->vtxcoords->h = 0.075f;
				testlay->genmidibut->box->upvtxtoscr();

				// GUI box of scratch video box
                testlay->loopbox->lcolor[0] = 0.7;
                testlay->loopbox->lcolor[1] = 0.7;
                testlay->loopbox->lcolor[2] = 0.7;
                testlay->loopbox->lcolor[3] = 1.0;
                bool found = false;
                for (int i = 0; i < loopstation->elems.size(); i++) {
                    if (loopstation->elems[i]->params.size()) {
                        if (std::find(loopstation->elems[i]->params.begin(), loopstation->elems[i]->params.end(),
                                      testlay->scritch) != loopstation->elems[i]->params.end()) {
                            found = true;
                            break;
                        }
                    }
                }
                if (!found) {
                    testlay->loopbox->acolor[0] = 0.3;
                    testlay->loopbox->acolor[1] = 0.3;
                    testlay->loopbox->acolor[2] = 0.3;
                    testlay->loopbox->acolor[3] = 1.0;
                }
				testlay->loopbox->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1;
				testlay->loopbox->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - 0.15f;
				testlay->loopbox->vtxcoords->w = mainprogram->layw * 1.5f;
				testlay->loopbox->vtxcoords->h = 0.075f;
				testlay->loopbox->upvtxtoscr();

				if (mainprogram->nodesmain->linked) {
					if (testlay->pos > 0) {
						testlay->node->box->scrcoords->x1 = testlay->blendnode->box->scrcoords->x1 -
						        mainprogram->xvtxtoscr(0.225f);
					}
					else {
						testlay->node->box->scrcoords->x1 = mainprogram->xvtxtoscr(0.132f);
					}
					testlay->node->box->upscrtovtx();
				}

				// GUI box of colourkey direction switch
                testlay->chdir->box->lcolor[0] = 0.7;
                testlay->chdir->box->lcolor[1] = 0.7;
                testlay->chdir->box->lcolor[2] = 0.7;
                testlay->chdir->box->lcolor[3] = 1.0;
				testlay->chdir->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + 0.36f;
				testlay->chdir->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1;
				testlay->chdir->box->vtxcoords->w = 0.0375f;
				testlay->chdir->box->vtxcoords->h = 0.075f;
				testlay->chdir->box->upvtxtoscr();

				// GUI box of colourkey inversion switch
                testlay->chinv->box->lcolor[0] = 0.7;
                testlay->chinv->box->lcolor[1] = 0.7;
                testlay->chinv->box->lcolor[2] = 0.7;
                testlay->chinv->box->lcolor[3] = 1.0;
				testlay->chinv->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + 0.4275f;
				testlay->chinv->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1;
				testlay->chinv->box->vtxcoords->w = 0.0375f;
				testlay->chinv->box->vtxcoords->h = 0.075f;
				testlay->chinv->box->upvtxtoscr();
			}
		}
	}
}

void Layer::mute_handle() {
    // change node connections to accommodate for a mute layer
    if (this->mutebut->value) {
        if (this->pos > 0) {
            if (this->blendnode->in) {
                this->blendnode->in->out.clear();
                mainprogram->nodesmain->currpage->connect_nodes(this->blendnode->in, this->lasteffnode[1]->out[0]);
            }
        }
        else if (this->next() != this) this->next()->blendnode->in = nullptr;
        //this->lasteffnode[1]->out.clear();
    }
    else {
        Layer *templay = this;
        while (1) {
            if (templay == templay->prev()) break;
            if ((templay->prev()->pos == 0) && !templay->prev()->mutebut->value) {
                //this->prev()->lasteffnode[1] = this->prev()->lasteffnode[0];
                this->prev()->lasteffnode[1]->out.clear();
                mainprogram->nodesmain->currpage->connect_nodes(this->prev()->lasteffnode[1], this->blendnode);
                break;
            }
            if (!templay->prev()->mutebut->value) {
                this->prev()->lasteffnode[1]->out.clear();
                mainprogram->nodesmain->currpage->connect_nodes(this->prev()->lasteffnode[1], this->blendnode);
                break;
            }
            templay = templay->prev();
        }
        this->lasteffnode[1]->out.clear();
        templay = this;
        while (1) {
            if (templay == templay->next()) {
                std::vector<Layer*> &lvec = choose_layers(this->deck);
                if (&lvec == &mainmix->layers[0]) {
                    mainprogram->nodesmain->currpage->connect_nodes(this->lasteffnode[1], mainprogram->nodesmain->mixnodes[0][0]);
                }
                else if (&lvec == &mainmix->layers[1]) {
                    mainprogram->nodesmain->currpage->connect_nodes(this->lasteffnode[1], mainprogram->nodesmain->mixnodes[0][1]);
                }
                else if (&lvec == &mainmix->layers[2]) {
                    mainprogram->nodesmain->currpage->connect_nodes(this->lasteffnode[1], mainprogram->nodesmain->mixnodes[1][0]);
                }
                else if (&lvec == &mainmix->layers[3]) {
                    mainprogram->nodesmain->currpage->connect_nodes(this->lasteffnode[1], mainprogram->nodesmain->mixnodes[1][1]);
                }
                break;
            }
            else if (!templay->next()->mutebut->value) {
                mainprogram->nodesmain->currpage->connect_nodes(this->lasteffnode[1], templay->next()->blendnode);
                break;
            }
            templay = templay->next();
        }
    }
}

int encode_frame(AVFormatContext *fmtctx, AVFormatContext *srcctx, AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt, FILE *outfile, int framenr) {
    int ret;
    /* send the frame to the encoder */
	AVPacket enc_pkt;
	enc_pkt.data = nullptr;
	enc_pkt.size = 0;
	av_init_packet(&enc_pkt);
	int got_frame = 0;
	if (outfile) {
		avcodec_send_frame(enc_ctx, frame);
 		int err = avcodec_receive_packet(enc_ctx, pkt);
 	}
	else {
		avcodec_send_frame(enc_ctx, frame);
		int err = avcodec_receive_packet(enc_ctx, &enc_pkt);
		if (srcctx) av_packet_rescale_ts(&enc_pkt, srcctx->streams[enc_pkt.stream_index]->time_base, fmtctx->streams[enc_pkt.stream_index]->time_base);
		else {
			// *2 still don't know why
			AVRational sttb = { fmtctx->streams[enc_pkt.stream_index]->time_base.num, fmtctx->streams[enc_pkt.stream_index]->time_base.den * 2};
			enc_pkt.pts = framenr;
			av_packet_rescale_ts(&enc_pkt, enc_ctx->time_base, sttb);
		}
	}
	if (outfile) fwrite(pkt->data, 1, pkt->size, outfile);
	else {
		/* prepare packet for muxing */
		enc_pkt.stream_index = pkt->stream_index;
		//enc_pkt.duration = pkt->duration;//(fmtctx->streams[enc_pkt.stream_index]->time_base.den * fmtctx->streams[enc_pkt.stream_index]->r_frame_rate.den) / (fmtctx->streams[enc_pkt.stream_index]->time_base.num * fmtctx->streams[enc_pkt.stream_index]->r_frame_rate.num);
		int err = av_interleaved_write_frame(fmtctx, &enc_pkt);
	}
   	av_packet_unref(pkt);
   	av_packet_unref(&enc_pkt);
   	return got_frame;
}

static int decode_packet(Layer *lay, bool show)
{
    int ret = 0;
    int decoded = lay->decpkt.size;
 	//av_frame_unref(lay->decframe);
	// lay->decpkt.dts = av_rescale_q_rnd(lay->decpkt.dts,
			// lay->video->streams[lay->decpkt.stream_index]->time_base,
			// lay->video->streams[lay->decpkt.stream_index]->codec->time_base,
			// AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
	// lay->decpkt.pts = av_rescale_q_rnd(lay->decpkt.pts,
			// lay->video->streams[lay->decpkt.stream_index]->time_base,
			// lay->video->streams[lay->decpkt.stream_index]->codec->time_base,
			// AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
	if (lay->decpkt.stream_index == lay->video_stream_idx) {
		/* decode video frame */
		int err2 = 0;
		if (!lay->vidopen) {
			while (1) {
				int err1 = avcodec_send_packet(lay->video_dec_ctx, &lay->decpkt);
				err2 = avcodec_receive_frame(lay->video_dec_ctx, lay->decframe);
				if (err2 == AVERROR(EAGAIN)) {
					av_packet_unref(&lay->decpkt);
					av_read_frame(lay->video, &lay->decpkt);
				}
				else break;
			}
			if (lay->vidformat == 187) {
				printf("");
			}
			//char buffer[1024];
			//av_strerror(err2, buffer, 1024);
			//printf(buffer);
			//if (err == AVERROR_EOF) lay->numf--;
			if (err2 == AVERROR(EINVAL)) {
				fprintf(stderr, "Error decoding video frame (%s)\n", 0);
				printf("codec %d", lay->decpkt);
				return ret;
			}
			if (err2 == AVERROR_EOF) {
				avcodec_flush_buffers(lay->video_dec_ctx);
			}
			if (show) {
				/* copy decoded frame to destination buffer:
				 * this is required since rawvideo expects non aligned data */
				int h = sws_scale
				(
					lay->sws_ctx,
					(uint8_t const* const*)lay->decframe->data,
					lay->decframe->linesize,
					0,
					lay->video_dec_ctx->height,
					lay->rgbframe[lay->pbofri]->data,
					lay->rgbframe[lay->pbofri]->linesize
				);
				if (h < 1) {
				    return 0;
				}
				std::mutex datalock;
				datalock.lock();
				lay->decresult->hap = false;
                lay->remfr[lay->pbofri]->data = (char*)*(lay->rgbframe[lay->pbofri]->extended_data);
 				lay->decresult->height = lay->video_dec_ctx->height;
				lay->decresult->width = lay->video_dec_ctx->width;
				lay->decresult->size = lay->decresult->width * lay->decresult->height * 4;
				lay->remfr[lay->pbofri]->newdata = true;
				if (lay->clonesetnr != -1) {
					std::unordered_set<Layer*>::iterator it;
					for (it = mainmix->clonesets[lay->clonesetnr]->begin(); it != mainmix->clonesets[lay->clonesetnr]->end(); it++) {
						Layer* clay = *it;
						if (clay == lay) continue;
						clay->remfr[lay->pbofri]->newdata = true;
					}
				}
				datalock.unlock();
			}
		}
	}
    else if (lay->decpkt.stream_index == lay->audio_stream_idx) {
        return 0;
        /* decode audio frame */
		int err2 = 0;
		int nsam = 0;
		while (1) {
			int err1 = avcodec_send_packet(lay->audio_dec_ctx, &lay->decpkt);
			err2 = avcodec_receive_frame(lay->audio_dec_ctx, lay->decframe);
			nsam += lay->decframe->nb_samples;
			if (err2 == AVERROR(EAGAIN)) {
				av_packet_unref(&lay->decpkt);
				av_read_frame(lay->video, &lay->decpkt);
			}
			else break;
		}
		if (err2 == AVERROR(EINVAL)) {
			fprintf(stderr, "Error decoding audio frame (%s)\n", 0);
			printf("codec %d", lay->decpkt);
			return ret;
		}
		/* Some audio decoders decode only part of the packet, and have to be
         * called again with the remainder of the packet data.
         * Sample: fate-suite/lossless-audio/luckynight-partial.shn
         * Also, some decoders might over-read the packet. */
        //decoded = FFMIN(ret, lay->decpkt.size);
		return nsam;
    }

 	//av_frame_unref(lay->decframe);

	return decoded;
}

void decode_audio(Layer *lay) {
    int ret = 0, got_frame;
    char *snippet = nullptr;
    size_t unpadded_linesize;
    av_packet_unref(&lay->decpkt);
    av_frame_unref(lay->decframe);
    av_read_frame(lay->video, &lay->decpkt);
    while (lay->decpkt.stream_index == lay->audio_stream_idx) {
        /*if (!lay->dummy && lay->volume->value > 0.0f) {
            ret = decode_packet(lay, &got_frame);
            // flush
            //lay->decpkt.data = nullptr;
            //lay->decpkt.size = 0;
            //decode_packet(lay, &got_frame);
            if (ret >= 0) {
                int ps;
                unpadded_linesize = lay->decframe->linesize[0];
                if (av_sample_fmt_is_planar(lay->audio_dec_ctx->sample_fmt)) {
                    snippet = new char[unpadded_linesize / 2];
                    ps = unpadded_linesize / 2;
                }
                else {
                    snippet = new char[unpadded_linesize / 2];
                    ps = unpadded_linesize / 2;
                }
                for (int pos = 0; pos < unpadded_linesize / 2; pos++) {
                    if (av_sample_fmt_is_planar(lay->audio_dec_ctx->sample_fmt)) {
                        snippet[pos] = lay->decframe->extended_data[0][pos];
                    }
                    else {
                        snippet[pos] = lay->decframe->extended_data[0][pos * 2 + 1];
                    }
                }
                //if ((lay->playbut->value && lay->speed->value < 0) || (lay->revbut->value && lay->speed->value > 0)) {
                //	snippet = (int*)malloc(unpadded_linesize);
                //	for (int add = 0; add < unpadded_linesize / av_get_bytes_per_sample((AVSampleFormat)lay->decframe->format); add++) {
                //		snippet[add] = snip[add];
                //	}
                //	free(snip);
                //}
                lay->pslens.push_back(ps);
                lay->snippets.push_back(snippet);
                //	if (ret < 0) break;
                //	lay->decpkt.data += ret;
                //	lay->decpkt.size -= ret;
                //} while (lay->decpkt.size > 0);
                //av_free_packet(&orig_pkt);
            }
        }*/

        av_packet_unref(&lay->decpkt);
        av_read_frame(lay->video, &lay->decpkt);
    }
    return;
    if (snippet) {
        lay->chready = true;
        while (lay->chready) {
            lay->newchunk.notify_all();
        }
    }
}

int find_stream_index(int *stream_idx,
                              AVFormatContext *video, enum AVMediaType type)
{
    int ret;
    AVDictionary *opts = nullptr;
    ret = av_find_best_stream(video, type, -1, -1, nullptr, 0);
    if (ret <0) {
    	printf("%s\n", " cant find stream");
    	return -1;
    }
	*stream_idx = ret;
    return ret;
}
static int get_format_from_sample_fmt(const char **fmt,
                                      enum AVSampleFormat sample_fmt)
{
    int i;
    struct sample_fmt_entry {
        enum AVSampleFormat sample_fmt; const char *fmt_be, *fmt_le;
    } sample_fmt_entries[] = {
        { AV_SAMPLE_FMT_U8,  "u8",    "u8"    },
        { AV_SAMPLE_FMT_S16, "s16be", "s16le" },
        { AV_SAMPLE_FMT_S32, "s32be", "s32le" },
        { AV_SAMPLE_FMT_FLT, "f32be", "f32le" },
        { AV_SAMPLE_FMT_DBL, "f64be", "f64le" },
    };
    *fmt = nullptr;
    for (i = 0; i < FF_ARRAY_ELEMS(sample_fmt_entries); i++) {
        struct sample_fmt_entry *entry = &sample_fmt_entries[i];
        if (sample_fmt == entry->sample_fmt) {
            *fmt = AV_NE(entry->fmt_be, entry->fmt_le);
            return 0;
        }
    }
    fprintf(stderr,
            "sample format %s is not supported as output format\n",
            av_get_sample_fmt_name(sample_fmt));
    return -1;
}

void Layer::get_cpu_frame(int framenr, int prevframe, int errcount)
{
    int ret = 0, got_frame;
    /* initialize packet, set data to nullptr, let the demuxer fill it */
    av_init_packet(&this->decpkt);
    av_init_packet(&this->decpktseek);
    /* flush cached frames */
    this->decpkt.data = nullptr;
    this->decpkt.size = 0;
    this->decpktseek.data = nullptr;
    this->decpktseek.size = 0;
    //do {
    //	decode_packet(&got_frame, 1);
    //} while (got_frame);


    if (this->type != ELEM_LIVE) {
        if (this->numf == 0) return;

        long long seekTarget = av_rescale(this->video_duration, framenr, this->numf) + this->videoseek_stream->first_dts;
        if (framenr != (int)this->startframe->value) {
            if (framenr != prevframe + 1) {
                // hop to not-next-frame
                av_seek_frame(this->videoseek, this->videoseek_stream->index, seekTarget, AVSEEK_FLAG_BACKWARD );
                //avcodec_flush_buffers(this->video_dec_ctx);
                //int r = av_read_frame(this->videoseek, &this->decpktseek);
            }
            else {
                //avformat_seek_file(this->video, this->video_stream->index, 0, seekTarget, seekTarget, AVSEEK_FLAG_ANY);
                //avformat_seek_file(this->video, this->video_stream->index, this->video_stream->first_dts,
                //                  seekTarget, seekTarget, 0);
            }
        }
        else {
            ret = avformat_seek_file(this->video, this->video_stream->index, seekTarget, seekTarget, this->video_duration + this->video_stream->first_dts, 0);
        }

        do {
            decode_audio(this);
        } while (this->decpkt.stream_index != this->video_stream_idx);
        if (framenr != prevframe + 1) {
            do {
                int r = av_read_frame(this->videoseek, &this->decpktseek);
            } while (this->decpktseek.stream_index != this->videoseek_stream_idx);
            int readpos = ((this->decpktseek.dts - this->videoseek_stream->first_dts) * this->numf) / this->video_duration;
            if (!this->keyframe) {
                if (readpos <= framenr) {
                    // readpos at keyframe after framenr
                    if (framenr > prevframe && prevframe > readpos) {
                        // starting from just past prevframe here is more efficient than decoding from readpos keyframe
                        readpos = prevframe + 1;
                    } else {
                        avformat_seek_file(this->video, this->video_stream->index, this->video_stream->first_dts,
                                           seekTarget, seekTarget, 0 );
                    }
                    for (int f = readpos; f < framenr; f = f + mainprogram->qualfr) {
                        // decode sequentially frames starting from keyframe readpos to current framenr
                        ret = decode_packet (this, false);
                        for (int q = 0; q <mainprogram->qualfr - 1; q++) {
                            int r = av_read_frame(this->video, &this->decpkt);
                        }
                        do {
                            decode_audio(this);
                        } while (this->decpkt.stream_index != this->video_stream_idx);
                    }
                }
            }
        }
        ret = decode_packet(this, true);
        if (ret == 0) {
            return;
        }
        if (this->scritching != 1 && ! (this->playbut->value == 0 && this->revbut->value == 0 && this->bouncebut->value == 0)) this->prevframe = framenr;
        if (this->decframe->width == 0) {
            if (!(this->playbut->value == 0 && this->revbut->value == 0 && this->bouncebut->value == 0)) this->prevframe = framenr;
            if ((this->speed->value > 0 && (this->playbut->value || this->bouncebut->value == 1)) || (this->speed->value < 0 && (this->revbut->value || this->bouncebut->value == 2))) {
                framenr++;
            }
            else if ((this->speed->value > 0 && (this->revbut->value || this->bouncebut->value == 2)) || (this->speed->value < 0 && (this->playbut->value || this->bouncebut->value == 1))) {
                framenr--;
            }
            else if (this->prevfbw) {
                //this->prevfbw = false;
                framenr--;
            }
            else framenr++;
            if (framenr > this->endframe->value) framenr = this->startframe->value;
            else if (framenr < this->startframe->value) framenr = this->endframe->value;
            //avcodec_flush_buffers(this->video_dec_ctx);
            errcount++;
            if (errcount == 1000) {
                mainprogram->openerr = true;
                return;
            }
            av_packet_unref(&this->decpkt);
            get_cpu_frame(framenr, this->prevframe, errcount);
            this->frame = framenr;
        }
        //decode_audio(this);
        av_packet_unref(&this->decpkt);
    }
    else {
        int r = av_read_frame(this->video, &this->decpkt);
        if (r < 0) printf("problem reading frame\n");
        else decode_packet(this, &got_frame);
        av_packet_unref(&this->decpkt);
    }


    if (this->audio_stream && 0) {
        enum AVSampleFormat sfmt = this->audio_dec_ctx->sample_fmt;
        int n_channels = this->audio_dec_ctx->channels;
        const char *fmt;
        if (av_sample_fmt_is_planar(this->audio_dec_ctx->sample_fmt)) {
            const char *packed = av_get_sample_fmt_name(sfmt);
            printf("Warning: the sample format the decoder produced is planar "
                   "(%s). This example will output the first channel only.\n",
                   packed ? packed : "?");
            sfmt = av_get_packed_sample_fmt(sfmt);
            n_channels = 1;
        }
        ret = get_format_from_sample_fmt(&fmt, sfmt);
    }

    //av_free_packet(&this->decpkt);
    //av_free_packet(&this->decpktseek);
}


bool Layer::get_hap_frame() {
    if (!this->video) return false;


    databufmutex.lock();
    if (!this->databuf[0]) {
        //	this->decresult->width = 0;
        for (int k = 0; k < 3; k++) {
            this->databuf[k] = (char *) malloc(this->video_dec_ctx->width * this->video_dec_ctx->height * 4);
            if (this->databuf[k] == nullptr) {
                return 0;
            }
        }
        this->databufready = true;
    }
    databufmutex.unlock();

    long long seekTarget = av_rescale(this->video_stream->duration, this->frame, this->numf) + this->video_stream->first_dts;
    int r = av_seek_frame(this->video, this->video_stream->index, seekTarget, 0);
    //av_new_packet(&this->decpkt, this->video_dec_ctx->width * this->video_dec_ctx->height * 4);
    av_init_packet(&this->decpkt);
    this->decpkt.data = NULL;
    this->decpkt.size = 0;
    AVPacket* pktpnt = &this->decpkt;
    av_frame_unref(this->decframe);
    r = av_read_frame(this->video, &this->decpkt);
    if (!this->dummy && ! (this->playbut->value == 0 && this->revbut->value == 0 && this->bouncebut->value == 0)) {
        this->prevframe = (int)this->frame;
    }

    char *bptrData = (char*)(&this->decpkt)->data;
    size_t size = (&this->decpkt)->size;
    if (size == 0) {
        return false;
    }
    size_t uncomp = this->video_dec_ctx->width * this->video_dec_ctx->height * 4;
    int headerl = 4;
    if (*bptrData == 0 && *(bptrData + 1) == 0 && *(bptrData + 2) == 0) {
        headerl = 8;
    }
    this->decresult->compression = *(bptrData + 3);

    //this->pboimutex.lock();
    int st = -1;
    databufmutex.lock();
    if (this->databufready) {
        st = snappy_uncompress(bptrData + headerl, size - headerl, this->databuf[this->pbofri], &uncomp);
    }
    databufmutex.unlock();
    av_packet_unref(&this->decpkt);

    if (!this->vidopen) {
        if (st != SNAPPY_OK) {
            this->decresult->width = 0;
            return false;
        }
        this->decresult->height = this->video_dec_ctx->height;
        this->decresult->width = this->video_dec_ctx->width;
        this->decresult->size = uncomp;
        this->decresult->hap = true;
        databufmutex.lock();
        this->remfr[this->pbofri]->data = this->databuf[this->pbofri];
        databufmutex.unlock();
        this->remfr[this->pbofri]->newdata = true;
        if (this->clonesetnr != -1) {
            std::unordered_set<Layer*>::iterator it;
            for (it = mainmix->clonesets[this->clonesetnr]->begin(); it != mainmix->clonesets[this->clonesetnr]->end(); it++) {
                Layer* clay = *it;
                if (clay == this) continue;
                clay->remfr[clay->pbofri]->newdata = true;
            }
        }
    }
    //this->pboimutex.unlock();
    return true;
}

void Layer::get_frame(){

    float speed = 1.0;
    int frnums[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

    while (1) {
        std::unique_lock<std::mutex> lock(this->startlock);
        this->startdecode.wait(lock, [&] {return this->ready; });
        lock.unlock();
        this->ready = false;
        //if (this->filename == "") continue;

        if (this->closethread == 1) {
            this->closethread = 2;
            dellayslock.lock();
            mainprogram->dellays.push_back(this);
            //if (std::find(this->layers->begin(), this->layers->end(), this) != this->layers->end()) {
            //    this->layers->erase(std::find(this->layers->begin(), this->layers->end(), this));
            //}
            dellayslock.unlock();
            return;
        }

        if (this->vidopen) {
            if (!this->dummy && this->video) {
                std::unique_lock<std::mutex> lock(this->enddecodelock);
                this->enddecodevar.wait(lock, [&] {return this->processed; });
                this->processed = false;
                lock.unlock();
                avformat_close_input(&this->video);
                if (this->videoseek) avformat_close_input(&this->videoseek);
            }
            bool r = this->thread_vidopen();
            this->vidopen = false;
            if (this->isduplay) {
                this->decresult->width = 0;
                this->frame = this->isduplay->frame;
                this->isduplay = nullptr;
            }
            if (r == 0) {
                printf("load error\n");
                mainprogram->openerr = true;
                this->opened = true;
                if (!this->isclone) this->initialized = false;
                this->startframe->value = 0;
                this->endframe->value = 0;
                this->opened = true;
            }
            else {
                this->opened = true;
            }
            continue;
        }
        if (!this->liveinput) {
            if (this->filename == "" || this->isclone) {
                continue;
            }
            if (this->vidformat != 188 && this->vidformat != 187) {
                if (this->video) {
                    if ((int)(this->frame) != this->prevframe || this->type == ELEM_LIVE) {
                        get_cpu_frame((int)this->frame, this->prevframe, 0);
                    }
                    else {
                        printf("");
                    }
                }
                this->processed = true;
                continue;
            }
            if ((int)(this->frame) != this->prevframe) {
                bool ret = this->get_hap_frame();
                if (!ret) {
                    this->get_hap_frame();
                }
            }
        }
        this->processed = true;
        continue;
    }
}

void Layer::trigger() {
    // trigger variable checks in mutexes

    while(this->closethread == 0) {
        if (mainprogram->shutdown) {
            // delete all layers on program quit
            this->closethread = 1;
            break;
        }
        if (mainprogram->openerr) {
            this->opened = true;
            mainprogram->openerr = false;  // reminder : requester
        }
        this->endopenvar.notify_all();  // use return variable as trigger
        this->enddecodevar.notify_all();  // use return variable as trigger
#ifdef POSIX
        sleep(0.1f);
#endif
#ifdef WINDOWS
        Sleep(10);
#endif
    }
    while(this->closethread == 1) {
        this->ready = true;
        this->startdecode.notify_all();
    }
}

void Mixer::add_del_bar() {
	//add/delete layer bar when right side of layer monitor hovered
	if (!mainprogram->menuondisplay && !mainprogram->dragbinel) {
		for (int j = 0; j < 2; j++) {
            std::vector<Layer*> &lvec = choose_layers(j);
            int sz = lvec.size();
			for (int i = 0; i < sz; i++) {
                Layer *lay = lvec[i];
                bool comp = !mainprogram->prevmodus;
                if (lay->pos < this->scenes[j][this->currscene[j]]->scrollpos ||
                    lay->pos > this->scenes[j][this->currscene[j]]->scrollpos + 2)
                    continue;
                Boxx *box = lay->node->vidbox;
                float thick = mainprogram->xvtxtoscr(0.009f);
                if (box->scrcoords->y1 - box->scrcoords->h < mainprogram->my && mainprogram->my < box->scrcoords->y1) {
                    if (box->scrcoords->x1 + box->scrcoords->w - thick -
                        (i - this->scenes[j][this->currscene[j]]->scrollpos == 2) * thick <
                        mainprogram->mx && mainprogram->mx < box->scrcoords->x1 + box->scrcoords->w + (i -
                                                                                                       this->scenes[j][this->currscene[j]]->scrollpos !=
                                                                                                       2) * thick) {
                        mainprogram->leftmousedown = false;
                        if (lay->pos == lvec.size() - 1 ||
                            lay->pos == this->scenes[j][this->currscene[j]]->scrollpos + 2) {
                            // handle vertical boxes at layer side for adding and deleting layer
                            // this block handles the rightmost box onscreen
                            mainprogram->addbox->vtxcoords->x1 =
                                    box->vtxcoords->x1 + box->vtxcoords->w - mainprogram->xscrtovtx(thick) * 2.0f +
                                    mainprogram->xscrtovtx(thick * ((i -
                                                                     this->scenes[j][this->currscene[j]]->scrollpos !=
                                                                     2)));
                            mainprogram->addbox->upvtxtoscr();
                            mainprogram->delbox->vtxcoords->x1 =
                                    box->vtxcoords->x1 + box->vtxcoords->w - mainprogram->xscrtovtx(thick) * 2.0f +
                                    mainprogram->xscrtovtx(thick * ((i -
                                                                     this->scenes[j][this->currscene[j]]->scrollpos !=
                                                                     2)));
                            mainprogram->delbox->upvtxtoscr();
                            mainprogram->frontbatch = true;
                            draw_box(lightblue, lightblue, mainprogram->addbox, -1);
                            render_text("ADD LAYER", white,
                                        mainprogram->addbox->vtxcoords->x1 - mainprogram->addbox->vtxcoords->w * 0.75f,
                                        mainprogram->addbox->vtxcoords->y1 + mainprogram->addbox->vtxcoords->h - 0.05f,
                                        0.00036f, 0.0006f, 0, 1);
                            draw_box(red, red, mainprogram->delbox, -1);
                            render_text("DEL", white,
                                        mainprogram->delbox->vtxcoords->x1 - mainprogram->delbox->vtxcoords->w * 0.75f,
                                        mainprogram->delbox->vtxcoords->y1 + mainprogram->delbox->vtxcoords->h + 0.05f,
                                        0.00036f, 0.0006f, 0, 1);
                            mainprogram->frontbatch = false;
                            bool cond1 = mainprogram->delbox->in();
                            bool cond2 = mainprogram->addbox->in();
                            if (mainprogram->leftmouse && !this->moving) {
                                // do add/delete layer
                                if (cond1) {
                                    this->delete_layer(lvec, lay, true);
                                } else if (cond2) {
                                    Layer *lay1 = this->add_layer(lvec, lay->pos + 1);
                                    make_layboxes();
                                }
                                mainprogram->leftmouse = false;
                                break;
                            }
                        }
                    } else if (
                            box->scrcoords->x1 - thick + (i - this->scenes[j][this->currscene[j]]->scrollpos
                                                          == 0) * thick < mainprogram->mx && mainprogram->mx <
                                                                                             box->scrcoords->x1 +
                                                                                             thick + (i -
                                                                                                      this->scenes[j][this->currscene[j]]->scrollpos ==
                                                                                                      0) * thick) {
                        // handle vertical boxes at layer side for adding and deleting layer
                        // this block handles the first boxes onscreen, just not the last
                        mainprogram->leftmousedown = false;
                        mainprogram->addbox->vtxcoords->x1 = box->vtxcoords->x1 - mainprogram->xscrtovtx(thick) + (i -
                                                                                                                   this->scenes[j][this->currscene[j]]->scrollpos ==
                                                                                                                   0) *
                                                                                                                  mainprogram->xscrtovtx(
                                                                                                                          thick);
                        mainprogram->addbox->upvtxtoscr();
                        mainprogram->delbox->vtxcoords->x1 = box->vtxcoords->x1 - mainprogram->xscrtovtx(thick) + (i -
                                                                                                                   this->scenes[j][this->currscene[j]]->scrollpos ==
                                                                                                                   0) *
                                                                                                                  mainprogram->xscrtovtx(
                                                                                                                          thick);
                        mainprogram->delbox->upvtxtoscr();
                        mainprogram->frontbatch = true;
                        draw_box(lightblue, lightblue, mainprogram->addbox, -1);
                        render_text("ADD LAYER", white,
                                    mainprogram->addbox->vtxcoords->x1 - mainprogram->addbox->vtxcoords->w * 0.75f,
                                    mainprogram->addbox->vtxcoords->y1 + mainprogram->addbox->vtxcoords->h - 0.05f,
                                    0.00036f, 0.0006f, 0, 1);
                        if (lay->pos > 0) {
                            draw_box(red, red, mainprogram->delbox, -1);
                            render_text("DEL", white,
                                        mainprogram->delbox->vtxcoords->x1 - mainprogram->delbox->vtxcoords->w * 0.75f,
                                        mainprogram->delbox->vtxcoords->y1 + mainprogram->delbox->vtxcoords->h + 0.05f,
                                        0.00036f, 0.0006f, 0, 1);
                        }
                        mainprogram->frontbatch = false;
                        bool cond1 = mainprogram->delbox->in();
                        bool cond2 = mainprogram->addbox->in();
                        if (mainprogram->leftmouse && !this->moving && !mainprogram->intopmenu) {
                            // do add/delete layer
                            if (lay->pos > 0 && cond1) {
                                this->delete_layer(lvec, lvec[lay->pos - 1], true);
                            } else if (cond2) {
                                Layer *lay1;
                                lay1 = this->add_layer(lvec, lay->pos);
                                make_layboxes();
                            }
                            break;
                        }
                    }
                }
            }
		}
	}
}

// HANDLING LAYER MONITOR HUD
void Mixer::vidbox_handle() {
    if (mainprogram->dragbinel) return;

	// handles layer monitor controls
	mainmix->add_del_bar();

	if (mainprogram->nodesmain->linked && mainmix->currlay[!mainprogram->prevmodus]) {
		// Handle vidbox
		for (int i = 0; i < this->currlays[!mainprogram->prevmodus].size(); i++) {
            Layer* lay = this->currlays[!mainprogram->prevmodus][i];
            std::vector<Layer*>& lvec = choose_layers(lay->deck);
            Boxx* box = lay->node->vidbox;
            if (box->in() && !lay->transforming) {
                mainprogram->frontbatch = true;

                lay->panbox->vtxcoords->x1 = box->vtxcoords->x1 + (box->vtxcoords->w / 2.0f) - 0.015f;
                lay->panbox->vtxcoords->y1 = box->vtxcoords->y1 + (box->vtxcoords->h / 2.0f) - 0.025f;
                lay->panbox->vtxcoords->w = 0.03f;
                lay->panbox->vtxcoords->h = 0.05f;
                lay->panbox->upvtxtoscr();
                draw_box(black, lay->shiftx->box->acolor, lay->panbox, -1);
                if (lay->panbox->in()) {
                    draw_box(black, lightblue, lay->panbox, -1);
                    if (mainprogram->leftmousedown) {
                        // layer view pan
                        mainprogram->leftmousedown = false;
                        mainprogram->transforming = true;
                        lay->transforming = 1;
                        lay->oldshx = lay->shiftx->value;
                        lay->oldshy = lay->shifty->value;
                        lay->oldmx = mainprogram->mx;
                        lay->oldmy = mainprogram->my;
                        lay->transmx = mainprogram->mx - (lay->shiftx->value * (float)glob->w / 2.0f);
                        lay->transmy = mainprogram->my - (lay->shifty->value * (float)glob->w / 2.0f);
                    }
                    if (mainprogram->menuactivation) {
                        if (loopstation->parelemmap.find(lay->shiftx) != loopstation->parelemmap.end()) mainprogram->parammenu6->state = 2;
                        else mainprogram->parammenu5->state = 2;
                        mainmix->menulayer = lay;
                        mainmix->learnparam = lay->shiftx;
                        mainmix->learnbutton = nullptr;
                        mainprogram->menuactivation = false;
                    }
                }
                else if (box->in()) {
                    // move layer     dragging
                    if (mainprogram->leftmousedown && !mainprogram->intopmenu) {
                        if (!lay->vidmoving && !mainmix->moving && !lay->cliploading) {
                            mainprogram->draglay = lay;
                            mainprogram->dragbinel = new BinElement;
                            mainprogram->dragbinel->tex = copy_tex(lay->node->vidbox->tex);
                            if (lay->type == ELEM_LIVE) {
                                mainprogram->dragbinel->path = lay->filename;
                                mainprogram->dragbinel->relpath = std::filesystem::relative(lay->filename, mainprogram->project->binsdir).generic_string();
                                mainprogram->dragbinel->type = ELEM_LIVE;
                            }
                            else {
                                mainprogram->dragpath = find_unused_filename("cliptemp_" + basename(mainprogram->draglay->filename), mainprogram->temppath, ".layer");
                                mainmix->save_layerfile(mainprogram->dragpath, mainprogram->draglay, 1, 0);
                                mainprogram->dragbinel->path = mainprogram->dragpath;
                                mainprogram->dragbinel->relpath = std::filesystem::relative(mainprogram->dragpath, mainprogram->project->binsdir).generic_string();
                                mainprogram->dragbinel->type = ELEM_LAYER;
                                mainprogram->draglay = lay;
                            }

                            mainprogram->shelves[0]->prevnum = -1;
                            mainprogram->shelves[1]->prevnum = -1;
                            mainprogram->leftmousedown = false;
                            lay->vidmoving = true;
                            mainmix->moving = true;
                            binsmain->currbinel = nullptr;
                        }
                    }
                }
                // display lock?
                if (lay->lockzoompan) {
                    draw_box(nullptr, nullptr, lay->panbox->vtxcoords->x1 + 0.003f, lay->panbox->vtxcoords->y1 + 0.005f, lay->panbox->vtxcoords->w * 0.8f, lay->panbox->vtxcoords->h * 0.9f, mainprogram->loktex);
                }
                mainprogram->frontbatch = false;
            }
            if (box->in()) {
                if (mainprogram->mousewheel) {
                    // scaling layer view
                    lay->scale->value -= mainprogram->mousewheel * lay->scale->value / 10.0f;
                    for (int i = 0; i < loopstation->elems.size(); i++) {
                        if (loopstation->elems[i]->recbut->value) {
                            loopstation->elems[i]->add_param_automationentry(lay->scale);
                        }
                    }
                    if (lay->type == ELEM_IMAGE || lay->type == ELEM_LIVE) {
                        lay->remfr[lay->pbodi]->newdata = true;
                    }
                }
            }

            if (lay->transforming == 1) {
                if (mainprogram->shift) {
                    // kick-in of straight line pan
                    if (abs((float)(mainprogram->mx - lay->oldmx)) > glob->h / 80.0f && (abs((float) (mainprogram->mx - lay->oldmx) / (float) (mainprogram->my - lay->oldmy)) > 1.2f) && !lay->straighty && !lay->straightx) {
                        lay->straightx = true;
                        lay->transmx = mainprogram->mx - (lay->shiftx->value * (float)glob->w / 2.0f);
                    } else if (abs((float)(mainprogram->my - lay->oldmy)) > glob->h / 80.0f && (abs((float) (mainprogram->my - lay->oldmy) / (float) (mainprogram->mx - lay->oldmx)) > 1.2f) && !lay->straightx && !lay->straighty) {
                        lay->straighty = true;
                        lay->transmy = mainprogram->my - (lay->shifty->value * (float)glob->w / 2.0f);
                    }
                }
                if (lay->straightx) {
                    // straight x pan
                    lay->shiftx->value = (float) (mainprogram->mx - lay->transmx) / ((float) glob->w / 2.0f);
                    lay->shifty->value = lay->oldshy;
                }
                else if (lay->straighty) {
                    // straight x pan
                    lay->shifty->value = (float) (mainprogram->my - lay->transmy) / ((float) glob->w / 2.0f);
                    lay->shiftx->value = lay->oldshx;
                }
                else if (!mainprogram->shift && !lay->straightx && !lay->straighty) {
                    // free pan
                    lay->shiftx->value = (float) (mainprogram->mx - lay->transmx) / ((float) glob->w / 2.0f);
                    lay->shifty->value = (float) (mainprogram->my - lay->transmy) / ((float) glob->w / 2.0f);
                }

                for (int i = 0; i < loopstation->elems.size(); i++) {
                    if (loopstation->elems[i]->recbut->value) {
                        loopstation->elems[i]->add_param_automationentry(lay->shiftx);
                    }
                }
                if (mainprogram->leftmouse) {
                    lay->straightx = false;
                    lay->straighty = false;
                    lay->transforming = 0;
                    mainprogram->transforming = false;
                }
                if (lay->type == ELEM_IMAGE || lay->type == ELEM_LIVE) {
                    lay->remfr[lay->pbodi]->newdata = true;
                }
            }
        }
	}
}

void Layer::display() {

	std::vector<Layer*>& lvec = choose_layers(this->deck);
	/*bool notyet = false;
	for (int i = 0; i < lvec.size(); i++) {
		if (!lvec[i]->newtexdata >= 2) {
			notyet = true;
			break;
		}
	}
	if (notyet && this->filename != "") {
		if (mainmix->bulrs[this->deck].size() > this->pos) {
			this->node->vidbox->tex = mainmix->butexes[this->deck][this->pos];
		}
	}*/
	if (mainmix->scenes[this->deck][mainmix->currscene[this->deck]]->scrollpos > lvec.size() - 2) mainmix->scenes[this->deck][mainmix->currscene[this->deck]]->scrollpos = lvec.size() - 2;
	if (mainmix->scenes[this->deck][mainmix->currscene[this->deck]]->scrollpos < 0) mainmix->scenes[this->deck][mainmix->currscene[this->deck]]->scrollpos = 0;
    make_layboxes();
    if (this->pos >= mainmix->scenes[this->deck][mainmix->currscene[this->deck]]->scrollpos && this->pos < mainmix->scenes[this->deck][mainmix->currscene[this->deck]]->scrollpos + 3) {
		Boxx* box = this->node->vidbox;
        mainprogram->frontbatch = true;  // allow alpha
		if (!this->mutebut->value) {
            draw_box(box, -1);
            draw_box(box, box->tex);
		}
		else {
            draw_box(box, -1);
			draw_box(box, box->tex);
			draw_box(white, darkred3, box, -1);
		}
		if (std::find(mainmix->currlays[!mainprogram->prevmodus].begin(), mainmix->currlays[!mainprogram->prevmodus].end(), this) != mainmix->currlays[!mainprogram->prevmodus].end()) {
            float pixelw = 2.0f / glob->w;
            float pixelh = 2.0f / glob->h;
            draw_box(white, nullptr, box->vtxcoords->x1 - pixelw * 2.0f, box->vtxcoords->y1 - pixelh * 2.0f, box->vtxcoords->w + 3.0f * pixelw, box->vtxcoords->h + 3.0f * pixelh, -1);
            draw_box(black, nullptr, box, -1);
		}

		if (mainmix->mode == 0 && mainprogram->nodesmain->linked) {
		    // set x1 for mute, solo and keepeff
            this->mutebut->box->vtxcoords->x1 = box->vtxcoords->x1 + mainprogram->layw - 0.06f;
            this->mutebut->box->upvtxtoscr();
            this->solobut->box->vtxcoords->x1 = this->mutebut->box->vtxcoords->x1 + 0.03f;
            this->solobut->box->upvtxtoscr();
            this->keepeffbut->box->vtxcoords->x1 = this->mutebut->box->vtxcoords->x1 - 0.03f;
            this->keepeffbut->box->upvtxtoscr();
            if (std::find(mainmix->currlays[!mainprogram->prevmodus].begin(), mainmix->currlays[!mainprogram->prevmodus].end(), this) != mainmix->currlays[!mainprogram->prevmodus].end()) {
                // print layer monitors overlay text
                std::string deckstr;
                if (this->deck == 0) deckstr = "A";
                else if (this->deck == 1) deckstr = "B";
                render_text("Layer " + deckstr + std::to_string(this->pos + 1), white, box->vtxcoords->x1 + 0.015f,
                            box->vtxcoords->y1 + box->vtxcoords->h - 0.045f, 0.0005f, 0.0008f);
                std::string name = remove_extension(basename(this->filename));
				render_text(name, white, box->vtxcoords->x1 + 0.015f,
                box->vtxcoords->y1 + box->vtxcoords->h - 0.09f, 0.0005f, 0.0008f);
				if (this->vidformat == -1) {
					if (this->type == ELEM_IMAGE) {
						render_text("IMAGE", white, box->vtxcoords->x1 + 0.015f, box->vtxcoords->y1 + box->vtxcoords->h - 0.135f, 0.0005f, 0.0008f);
					}
				}
				else if (this->vidformat == 188 || this->vidformat == 187) {
					render_text("HAP", white, box->vtxcoords->x1 + 0.015f, box->vtxcoords->y1 + box->vtxcoords->h - 0.135f, 0.0005f, 0.0008f);
				}
				else {
					render_text("CPU", red, box->vtxcoords->x1 + 0.015f, box->vtxcoords->y1 + box->vtxcoords->h - 0.135f, 0.0005f, 0.0008f);
				}
			}
            if (this == mainmix->reclay) {
                render_text("REC", red, box->vtxcoords->x1 + 0.22f,
                            box->vtxcoords->y1 + box->vtxcoords->h - 0.045f, 0.0005f, 0.0008f);
            }

            if (this->hapbinel) {
                // show that layer is hapencoding on-the-fly
                float progress = this->hapbinel->encodeprogress;
                render_text("Encoding...", white, box->vtxcoords->x1 + 0.0225f, box->vtxcoords->y1 + 0.105f, 0.001f, 0.0016f);
                draw_box(white, nullptr, box->vtxcoords->x1 + 0.0225f, box->vtxcoords->y1 + 0.045f, box->vtxcoords->w - 0.045f, 0.04f, -1);
                draw_box(white, white, box->vtxcoords->x1 + 0.0225f, box->vtxcoords->y1 + 0.045f, progress * (box->vtxcoords->w - 0.045f), 0.04f, -1);
            }

            mainprogram->frontbatch = false;

			if (box->in()) {
                if (mainprogram->dropfiles.size()) {
                    // SDL drag'n'drop
                    mainprogram->path = mainprogram->dropfiles[0];
                    for (char *df: mainprogram->dropfiles) {
                        bool wrong = false;
                        std::string path = df;
                        if (isdeckfile(path)) {
                            wrong = true;
                        }
                        if (ismixfile(path)) {
                            wrong = true;
                        }
                        if (!wrong && (isvideo(path) || islayerfile(path))) {
                            mainprogram->paths.push_back(df);
                        }
                    }
                    mainprogram->pathto = "OPENFILESLAYER";
                    mainprogram->loadlay = this;
                }
                if (!mainprogram->needsclick || mainprogram->leftmouse) {
                    if (box != mainprogram->prevbox) {
                        mainprogram->prevbox = box;
                        if (!mainmix->moving && !mainprogram->cwon) {
                            mainmix->currlay[!mainprogram->prevmodus] = this;
                            if (mainprogram->shift || mainprogram->ctrl) {
                                if (std::find(mainmix->currlays[!mainprogram->prevmodus].begin(), mainmix->currlays[!mainprogram->prevmodus].end(), this) ==
                                    mainmix->currlays[!mainprogram->prevmodus].end()) {
                                    mainmix->currlays[!mainprogram->prevmodus].push_back(this);
                                } else {
                                    if (mainmix->currlays[!mainprogram->prevmodus].size() > 1) {
                                        mainmix->currlays[!mainprogram->prevmodus].erase(std::find(mainmix->currlays[!mainprogram->prevmodus].begin(), mainmix->currlays[!mainprogram->prevmodus].end(), this));
                                        mainmix->currlay[!mainprogram->prevmodus] = mainmix->currlays[!mainprogram->prevmodus][mainmix->currlays[!mainprogram->prevmodus].size() - 1];
                                    }
                                }
                            } else {
                                mainmix->currlays[!mainprogram->prevmodus].clear();
                                mainmix->currlays[!mainprogram->prevmodus].push_back(this);
                            }
                            mainprogram->effcat[this->deck]->value = 0;
                        }
                    }
                }
                if (mainprogram->menuactivation && !this->panbox->in()) {
                    // Trigger mainprogram->laymenu#
                    if (this->type == ELEM_IMAGE || this->type == ELEM_LIVE)
                        mainprogram->laymenu2->state = 2;
                    else mainprogram->laymenu1->state = 2;
                    mainmix->mouselayer = this;
                    mainmix->mousedeck = this->deck;
                    mainprogram->menuactivation = false;
                }

				//draw and handle layer mute and solo buttons
				if (this->mutebut->box->in()) {
					render_text("M", alphablue, this->mutebut->box->vtxcoords->x1 + 0.0078f, this->mutebut->box->vtxcoords->y1 + 0.0078f, 0.0006, 0.001);
					if (mainprogram->leftmousedown) {
						this->muting = true;
						mainprogram->leftmousedown = false;
					}
					if (this->muting && mainprogram->leftmouse) {
						this->muting = false;
						this->mutebut->value = !this->mutebut->value;
						this->mutebut->oldvalue = !this->mutebut->oldvalue;
						this->mute_handle();
						for (int i = 0; i < loopstation->elems.size(); i++) {
							if (loopstation->elems[i]->recbut->value) {
								loopstation->elems[i]->add_button_automationentry(this->mutebut);
							}
						}
					}
					if (mainprogram->menuactivation && !mainprogram->menuondisplay) {
						mainprogram->parammenu3->state = 2;
						mainmix->learnparam = nullptr;
						mainmix->learnbutton = this->mutebut;
						mainprogram->menuactivation = false;
					}
				}
				else if (!this->mutebut->value) {
					render_text("M", alphawhite, this->mutebut->box->vtxcoords->x1 + 0.0078f, this->mutebut->box->vtxcoords->y1 + 0.0078f, 0.0006, 0.001);
				}
				this->solobut->box->vtxcoords->x1 = this->mutebut->box->vtxcoords->x1 + 0.03f;
				this->solobut->box->upvtxtoscr();
				if (this->solobut->box->in()) {
					render_text("S", alphablue, this->solobut->box->vtxcoords->x1 + 0.0078f, this->solobut->box->vtxcoords->y1 + 0.0078f, 0.0006, 0.001);
					if (mainprogram->leftmousedown) {
						this->soloing = true;
						mainprogram->leftmousedown = false;
					}
					if (this->soloing && mainprogram->leftmouse) {
						mainprogram->leftmouse = false;
						this->soloing = false;
						this->solobut->value = !this->solobut->value;
						this->solobut->oldvalue = !this->solobut->oldvalue;
						if (this->solobut->value) {
							if (this->mutebut->value) {
								this->mutebut->value = false;
								this->mutebut->oldvalue = false;
								this->mute_handle();
							}
							for (int k = 0; k < lvec.size(); k++) {
								if (k != this->pos) {
									if (lvec[k]->mutebut->value == false) {
										lvec[k]->mutebut->value = true;
										lvec[k]->mutebut->oldvalue = true;
										lvec[k]->mute_handle();
									}
									else {
										lvec[k]->mutebut->value = false;
										lvec[k]->mutebut->oldvalue = false;
										lvec[k]->mute_handle();
										lvec[k]->mutebut->value = true;
										lvec[k]->mutebut->oldvalue = true;
										lvec[k]->mute_handle();
									}
									lvec[k]->solobut->value = false;
									lvec[k]->solobut->oldvalue = false;
								}
							}
						}
						else {
							for (int k = 0; k < lvec.size(); k++) {
								if (k != this->pos) {
									lvec[k]->mutebut->value = false;
									lvec[k]->mutebut->oldvalue = false;
									lvec[k]->mute_handle();
								}
							}
						}
						for (int i = 0; i < loopstation->elems.size(); i++) {
							if (loopstation->elems[i]->recbut->value) {
								loopstation->elems[i]->add_button_automationentry(this->solobut);
							}
						}
					}
					if (mainprogram->menuactivation && !mainprogram->menuondisplay) {
						mainprogram->parammenu3->state = 2;
						mainmix->learnparam = nullptr;
						mainmix->learnbutton = this->solobut;
						mainprogram->menuactivation = false;
					}
				}
				else if (!this->solobut->value) {
					render_text("S", alphawhite, this->solobut->box->vtxcoords->x1 + 0.0078f, this->solobut->box->vtxcoords->y1 + 0.0078f, 0.0006, 0.001);
				}

				// draw and handle keepeff button
                this->keepeffbut->box->vtxcoords->x1 = this->mutebut->box->vtxcoords->x1 - 0.03f;
                this->keepeffbut->box->upvtxtoscr();
                if (this->keepeffbut->box->in()) {
                    render_text("E", alphablue, this->keepeffbut->box->vtxcoords->x1 + 0.0078f,
                                this->keepeffbut->box->vtxcoords->y1 + 0.0078f, 0.0006, 0.001);
                    if (mainprogram->leftmousedown) {
                        this->keepeffing = true;
                        mainprogram->leftmousedown = false;
                    }
                    if (this->keepeffing && mainprogram->leftmouse) {
                        this->keepeffbut->value = !this->keepeffbut->value;
                        mainprogram->leftmouse = false;
                        for (int i = 0; i < loopstation->elems.size(); i++) {
                            if (loopstation->elems[i]->recbut->value) {
                                loopstation->elems[i]->add_button_automationentry(this->keepeffbut);
                            }
                        }
                    }
                    if (mainprogram->menuactivation && !mainprogram->menuondisplay) {
                        mainprogram->parammenu3->state = 2;
                        mainmix->learnparam = nullptr;
                        mainmix->learnbutton = this->mutebut;
                        mainprogram->menuactivation = false;
                    }
                }
                else if (!this->keepeffbut->value) {
                    render_text("E", alphawhite, this->keepeffbut->box->vtxcoords->x1 + 0.0078f, this->keepeffbut->box->vtxcoords->y1 + 0.0078f, 0.0006, 0.001);
                }

				// queue fold/unfold button
                mainprogram->frontbatch = true;
				this->queuebut->box->vtxcoords->x1 = box->vtxcoords->x1 + 0.015f;
				this->queuebut->box->upvtxtoscr();
				if (this->queuebut->box->in()) {
					register_triangle_draw(black, alphablue, this->queuebut->box->vtxcoords->x1, this->queuebut->box->vtxcoords->y1, this->queuebut->box->vtxcoords->w, this->queuebut->box->vtxcoords->h, UP, CLOSED);
					register_triangle_draw(black, black, this->queuebut->box->vtxcoords->x1, this->queuebut->box->vtxcoords->y1, this->queuebut->box->vtxcoords->w, this->queuebut->box->vtxcoords->h, UP, OPEN);
					if (mainprogram->leftmousedown) {
						this->mousequeue = true;
						mainprogram->leftmousedown = false;
					}
					if (this->mousequeue && mainprogram->leftmouse) {
						this->mousequeue = false;
						mainprogram->leftmouse = false;
						this->queueing = !this->queueing;
						if (this->queueing) mainprogram->queueing = true;
						else {
							bool found = false;
							for (int i = 0; i < 2; i++) {
								std::vector<Layer*>& lvec = choose_layers(i);
								for (int j = 0; j < lvec.size(); j++) {
									if (lvec[j]->queueing) {
										found = true;
										break;
									}
								}
							}
							if (!found)	mainprogram->queueing = false;
						}
					}
				}
				else {
					register_triangle_draw(black, alphawhite, this->queuebut->box->vtxcoords->x1, this->queuebut->box->vtxcoords->y1, this->queuebut->box->vtxcoords->w, this->queuebut->box->vtxcoords->h, UP, CLOSED);
					register_triangle_draw(black, black, this->queuebut->box->vtxcoords->x1, this->queuebut->box->vtxcoords->y1, this->queuebut->box->vtxcoords->w, this->queuebut->box->vtxcoords->h, UP, OPEN);
				}
                mainprogram->frontbatch = false;
			}
			else {
                if (mainprogram->prevbox == box) mainprogram->prevbox = nullptr;
				int size = lvec.size();
				int numonscreen = size - mainmix->scenes[this->deck][mainmix->currscene[this->deck]]->scrollpos;
				if (0 <= numonscreen && numonscreen <= 2) {
					if (mainprogram->xvtxtoscr(mainprogram->numw + this->deck * 1.0f + numonscreen * mainprogram->layw) < mainprogram->mx && mainprogram->mx < this->deck * (glob->w / 2.0f) + glob->w / 2.0f) {
						if (0 < mainprogram->my && mainprogram->my < mainprogram->yvtxtoscr(mainprogram->layh)) {
                            if (mainprogram->dropfiles.size()) {
                                // SDL drag'n'drop
                                mainprogram->path = mainprogram->dropfiles[0];
                                for (char *df: mainprogram->dropfiles) {
                                    bool wrong = false;
                                    std::string path = df;
                                    if (isdeckfile(path)) {
                                        wrong = true;
                                    }
                                    if (ismixfile(path)) {
                                        wrong = true;
                                    }
                                    if (!wrong && (isvideo(path) || islayerfile(path))) {
                                        mainprogram->paths.push_back(df);
                                    }
                                    mainprogram->pathto = "OPENFILESSTACK";
                                    mainmix->addlay = true;
                                    mainmix->mouselayer = nullptr;
                                    mainmix->mousedeck = this->deck;
                                }
                            }
							if (mainprogram->menuactivation) {
								mainprogram->newlaymenu->state = 2;
								mainmix->mousedeck = this->deck;
								mainprogram->menuactivation = false;
							}
						}
					}
				}
			}
			if (this->mutebut->toggled()) {
				this->mute_handle();
			}
			if (this->solobut->toggled()) {
				if (this->solobut->value) {
					if (this->mutebut->value) {
						this->mutebut->value = false;
						this->mutebut->oldvalue = false;
						this->mute_handle();
					}
					for (int k = 0; k < lvec.size(); k++) {
						if (k != this->pos) {
							if (lvec[k]->mutebut->value == false) {
								lvec[k]->mutebut->value = true;
								lvec[k]->mutebut->oldvalue = true;
								lvec[k]->mute_handle();
							}
							else {
								lvec[k]->mutebut->value = false;
								lvec[k]->mutebut->oldvalue = false;
								lvec[k]->mute_handle();
								lvec[k]->mutebut->value = true;
								lvec[k]->mutebut->oldvalue = true;
								lvec[k]->mute_handle();
							}
							lvec[k]->solobut->value = false;
							lvec[k]->solobut->oldvalue = false;
						}
					}
				}
				else {
					for (int k = 0; k < lvec.size(); k++) {
						if (k != this->pos) {
							lvec[k]->mutebut->value = false;
							lvec[k]->mutebut->oldvalue = false;
							lvec[k]->mute_handle();
						}
					}
				}
			}
			if (this->mutebut->value) {
				render_text("M", alphagreen, this->mutebut->box->vtxcoords->x1 + 0.0078f, this->mutebut->box->vtxcoords->y1 + 0.0078f, 0.0006, 0.001);
			}
            if (this->solobut->value) {
                render_text("S", alphagreen, this->solobut->box->vtxcoords->x1 + 0.0078f, this->solobut->box->vtxcoords->y1 + 0.0078f, 0.0006, 0.001);
            }
            if (this->keepeffbut->value) {
                render_text("E", alphagreen, this->keepeffbut->box->vtxcoords->x1 + 0.0078f, this->keepeffbut->box->vtxcoords->y1 + 0.0078f, 0.0006, 0.001);
            }
		}
		mainprogram->frontbatch = false;


		// Show current layer controls
        if (this != mainmix->currlay[!mainprogram->prevmodus]) {
            return;
        }

        if (!this->queueing) {
            // Draw controls/effects background box
            draw_box(grey, darkgreygreen, this->mixbox->vtxcoords->x1, mainmix->crossfade->box->vtxcoords->y1 + mainmix->crossfade->box->vtxcoords->h, 0.6f, this->mixbox->vtxcoords->y1 - mainmix->crossfade->box->vtxcoords->y1, -1);
            // Draw mixbox
            std::string mixstr;
            box = this->mixbox;
            draw_box(box, -1);
            switch (this->blendnode->blendtype) {
                case 1:
                    mixstr = "Mix";
                    break;
                case 2:
                    mixstr = "Mult";
                    break;
                case 3:
                    mixstr = "Scrn";
                    break;
                case 4:
                    mixstr = "Ovrl";
                    break;
                case 5:
                    mixstr = "HaLi";
                    break;
                case 6:
                    mixstr = "SoLi";
                    break;
                case 7:
                    mixstr = "Divi";
                    break;
                case 8:
                    mixstr = "Add";
                    break;
                case 9:
                    mixstr = "Sub";
                    break;
                case 10:
                    mixstr = "Diff";
                    break;
                case 11:
                    mixstr = "Dodg";
                    break;
                case 12:
                    mixstr = "CoBu";
                    break;
                case 13:
                    mixstr = "LiBu";
                    break;
                case 14:
                    mixstr = "ViLi";
                    break;
                case 15:
                    mixstr = "LiLi";
                    break;
                case 16:
                    mixstr = "DaOn";
                    break;
                case 17:
                    mixstr = "LiOn";
                    break;
                case 18:
                    mixstr = "Wipe";
                    break;
                case 19:
                    mixstr = "CKey";
                    if (this->pos > 0) {
                        draw_box(this->colorbox, -1);
                        render_text("Color", white, this->mixbox->vtxcoords->x1 + 0.12f,
                                    1.0f - (mainprogram->layh + 0.135f) + 0.03f, 0.00045f, 0.00075f);
                    }
                    break;
                case 20:
                    mixstr = "ChrKey";
                    if (this->pos > 0) {
                        draw_box(this->colorbox, -1);
                        render_text("Color", white, this->mixbox->vtxcoords->x1 + 0.12f,
                                    1.0f - (mainprogram->layh + 0.135f) + 0.03f, 0.00045f, 0.00075f);
                    }
                    break;
                case 21:
                    mixstr = "LumKey";
                    if (this->pos > 0) {
                        draw_box(this->colorbox, -1);
                        render_text("Color", white, this->mixbox->vtxcoords->x1 + 0.12f,
                                    1.0f - (mainprogram->layh + 0.135f) + 0.03f, 0.00045f, 0.00075f);
                    }
                    break;
                case 22:
                    mixstr = "Disp";
                    break;
            }
            if (this->pos > 0) {
                render_text(mixstr, white, this->mixbox->vtxcoords->x1 + 0.015f,
                            1.0f - (mainprogram->layh + 0.135f) + 0.03f, 0.00045f, 0.00075f);
            } else {
                render_text(mixstr, darkred1, this->mixbox->vtxcoords->x1 + 0.015f,
                            1.0f - (mainprogram->layh + 0.135f) + 0.03f, 0.00045f, 0.00075f);
            }

            // Draw and handle effect category buttons
            float efx = this->mixbox->vtxcoords->x1 - mainprogram->numw;
            mainprogram->effscrollupA->vtxcoords->x1 = efx;
            mainprogram->effscrollupB->vtxcoords->x1 = efx;
            mainprogram->effscrolldownA->vtxcoords->x1 = efx;
            mainprogram->effscrolldownB->vtxcoords->x1 = efx;
            mainprogram->effscrollupA->upvtxtoscr();
            mainprogram->effscrollupB->upvtxtoscr();
            mainprogram->effscrolldownA->upvtxtoscr();
            mainprogram->effscrolldownB->upvtxtoscr();
            Boxx *box = mainprogram->effcat[this->deck]->box;
            box->vtxcoords->x1 = efx;
            box->upvtxtoscr();
            mainprogram->handle_button(mainprogram->effcat[this->deck]);
            if (this->effects[1].size() && mainprogram->effcat[this->deck]->value == 0) {
                box->acolor[0] = 0.5f;
                box->acolor[1] = 0.0f;
                box->acolor[2] = 0.0f;
                box->acolor[3] = 1.0f;
            }
            draw_box(box, -1);
            render_text(mainprogram->effcat[this->deck]->name[mainprogram->effcat[this->deck]->value], white,
                        box->vtxcoords->x1, box->vtxcoords->y1 + box->vtxcoords->h + 0.03f, 0.00045f, 0.00075f, 0,
                        1);
            std::vector<Effect *> &evec = this->choose_effects();
            bool cat = mainprogram->effcat[this->deck]->value;

            // Draw and handle effect stack scrollboxes
            if (mainmix->currlay[!mainprogram->prevmodus]->deck == 0) {
                this->effscroll[cat] = mainprogram->handle_scrollboxes(*mainprogram->effscrollupA,
                                                                       *mainprogram->effscrolldownA,
                                                                       this->numefflines[cat], this->effscroll[cat],
                                                                       mainprogram->efflines);
            } else {
                this->effscroll[cat] = mainprogram->handle_scrollboxes(*mainprogram->effscrollupB,
                                                                       *mainprogram->effscrolldownB,
                                                                       this->numefflines[cat], this->effscroll[cat],
                                                                       mainprogram->efflines);
            }
            if (this->effects[cat].size()) {
                if ((glob->w / 2.0f > mainprogram->mx && mainmix->currlay[!mainprogram->prevmodus]->deck == 0) ||
                    (glob->w / 2.0f < mainprogram->mx && mainmix->currlay[!mainprogram->prevmodus]->deck == 1)) {
                    if (mainprogram->my > mainprogram->yvtxtoscr(mainprogram->layh - 0.3f)) {
                        if (mainprogram->mousewheel && this->numefflines[cat] > mainprogram->efflines) {
                            this->effscroll[cat] -= mainprogram->mousewheel;
                            if (this->effscroll[cat] < 0) this->effscroll[cat] = 0;
                            if (this->numefflines[cat] - this->effscroll[cat] < mainprogram->efflines)
                                this->effscroll[cat] = this->numefflines[cat] - mainprogram->efflines;
                        }
                    }
                }
            }
            // Draw effectboxes and parameters
            std::string effstr;
            for (int i = 0; i < evec.size(); i++) {
                Effect *eff = evec[i];
                Boxx *box;
                float x1, y1, wi;
                x1 = eff->box->vtxcoords->x1 + 0.048f;
                wi = (0.7f - mainprogram->numw - 0.048f) / 4.0f;

                if (eff->box->vtxcoords->y1 <
                    1.0 - mainprogram->layh - 0.135f - 0.33f - 0.075f * (mainprogram->efflines - 1))
                    break;
                if (eff->box->vtxcoords->y1 <= 1.0 - mainprogram->layh - 0.135f - 0.27f) {
                    eff->drywet->handle(true);
                    mainprogram->handle_button(eff->onoffbutton);

                    box = eff->box;
                    if (mainprogram->effcat[this->deck]->value == 0) {
                        if (eff->onoffbutton->value) draw_box(lightgrey, darkred1, box, -1);
                        else draw_box(lightgrey, darkred2, box, -1);
                    } else {
                        if (eff->onoffbutton->value) draw_box(lightgrey, darkgreen1, box, -1);
                        else draw_box(lightgrey, darkgreen2, box, -1);
                    }
                    effstr = eff->get_namestring();
                    float textw = (textwvec_total(render_text(effstr, white, eff->box->vtxcoords->x1 + 0.015f,
                                                                eff->box->vtxcoords->y1 + 0.075f - 0.045f,
                                                                0.00045f, 0.00075f))) * 1.5f;
                    eff->box->vtxcoords->w = textw + 0.048f;
                    x1 = eff->box->vtxcoords->x1 + 0.048f + textw;
                    wi = (0.7f - mainprogram->numw - 0.048f - textw) / 4.0f;
                }
                y1 = eff->box->vtxcoords->y1;
                // draw parameters
                for (int j = 0; j < eff->params.size(); j++) {
                    Param *par = eff->params[j];
                    par->box->lcolor[0] = 0.6;
                    par->box->lcolor[1] = 0.6;
                    par->box->lcolor[2] = 0.6;
                    par->box->lcolor[3] = 1.0;
                    if (par->nextrow) {
                        x1 = eff->box->vtxcoords->x1 + 0.03f;
                        y1 -= 0.075f;
                        wi = (0.7f - mainprogram->numw - 0.03f) / 4.0;
                    }
                    par->box->vtxcoords->x1 = x1;
                    x1 += wi + 0.015f;
                    par->box->vtxcoords->y1 = y1;
                    par->box->vtxcoords->w = wi;
                    par->box->vtxcoords->h = eff->box->vtxcoords->h;
                    par->box->upvtxtoscr();

                    if (par->box->vtxcoords->y1 <
                        1.0 - mainprogram->layh - 0.135f - 0.33f - 0.075f * (mainprogram->efflines - 1))
                        break;
                    if (par->box->vtxcoords->y1 <= 1.0 - mainprogram->layh - 0.135f - 0.27f) {
                        par->handle();
                    }
                }
            }

            // Draw effectboxes
            // Bottom bar
            float sx1, sy1, vx1, vy1, vx2, vy2, sw;
            bool bottom = false;
            bool inbetween = false;
            Effect *eff = nullptr;
            if (!mainprogram->queueing) {
                if (evec.size()) {
                    eff = evec[evec.size() - 1];
                    box = eff->onoffbutton->box;
                    sx1 = box->scrcoords->x1;
                    sy1 = box->scrcoords->y1 + (eff->numrows - 1) * mainprogram->yvtxtoscr(0.075f);
                    if (1.0f - mainprogram->yscrtovtx(sy1) < -0.4f) {
                        sy1 = mainprogram->yvtxtoscr(1.4f);
                    }
                    vx1 = box->vtxcoords->x1;
                    vy1 = box->vtxcoords->y1 - (eff->numrows - 1) * 0.075f;
                    if (vy1 < -0.4f) vy1 = -0.4f;
                    sw = mainprogram->layw * 1.5f * glob->w / 2.0;
                } else {
                    box = this->mixbox;
                    sw = mainprogram->layw * 1.5f * glob->w / 2.0;
                    sx1 = box->scrcoords->x1 + mainprogram->xvtxtoscr(0.0375f);
                    sy1 = this->opacity->box->scrcoords->y1;
                    vx1 = box->vtxcoords->x1 + 0.0375f;
                    vy1 = 1 - mainprogram->layh - 0.375f;
                }
                if (!mainprogram->menuondisplay) {
                    if (sy1 - 7.5 <= mainprogram->my && mainprogram->my <= sy1 + 7.5) {
                        //inbetween = true;
                        vx2 = vx1;
                        vy2 = vy1 - 0.0165f;
                    }
                    if (mainprogram->addeffectbox->in()) {
                        if ((mainprogram->menuactivation || mainprogram->leftmouse) && !mainprogram->drageff) {
                            mainprogram->effectmenu->state = 2;
                            mainmix->insert = true;
                            mainmix->mouseeffect = evec.size();
                            mainmix->mouselayer = this;
                            mainprogram->effectmenu->menux = mainprogram->mx;
                            mainprogram->effectmenu->menuy = mainprogram->my;
                            mainprogram->menuactivation = false;
                            mainprogram->menuondisplay = true;
                            mainprogram->drageffsense = false;
                            mainprogram->drageff = nullptr;
                        }
                        bottom = true;
                    }
                }
                mainprogram->indragbox = false;
                for (int j = 0; j < evec.size(); j++) {
                    eff = evec[j];
                    box = eff->box;
                    eff->box->acolor[0] = 0.75f;
                    eff->box->acolor[1] = 0.0f;
                    eff->box->acolor[2] = 0.0f;
                    eff->box->acolor[3] = 1.0f;
                    if (!mainprogram->menuondisplay) {
                        if (box->scrcoords->x1 < mainprogram->mx &&
                            mainprogram->mx < box->scrcoords->x1 + box->scrcoords->w) {
                            if (box->scrcoords->y1 - box->scrcoords->h + 7 <= mainprogram->my &&
                                mainprogram->my <= box->scrcoords->y1 - 7) {
                                // mouse on effect box
                                if (mainprogram->del) {
                                    this->delete_effect(j);
                                    mainprogram->del = 0;
                                    break;
                                }
                                if ((mainprogram->menuactivation || mainprogram->leftmouse) && !mainprogram->drageff) {
                                    mainprogram->effectmenu->state = 2;
                                    mainmix->mouselayer = this;
                                    mainmix->mouseeffect = j;
                                    mainmix->insert = false;
                                    mainprogram->effectmenu->menux = mainprogram->mx;
                                    mainprogram->effectmenu->menuy = mainprogram->my;
                                    mainprogram->menuactivation = false;
                                    mainprogram->menuondisplay = true;
                                    mainprogram->dragbox = nullptr;
                                    mainprogram->drageffsense = false;
                                }
                                eff->box->acolor[0] = 0.5;
                                eff->box->acolor[1] = 0.5;
                                eff->box->acolor[2] = 1.0;
                                eff->box->acolor[3] = 1.0;

                                // prepare effect dragging
                                eff->pos = j;
                                if (!mainprogram->drageff) {
                                    if (mainprogram->leftmousedown && !mainprogram->drageffsense) {
                                        mainprogram->drageffsense = true;
                                        mainprogram->dragbox = eff->box;
                                        mainprogram->drageffpos = j;
                                        mainprogram->leftmousedown = false;
                                    }
                                    if (j == mainprogram->drageffpos) {
                                        mainprogram->indragbox = true;
                                    }
                                }
                            }
                        }
                        box = eff->onoffbutton->box;
                        if (box->scrcoords->x1 < mainprogram->mx &&
                            mainprogram->mx < box->scrcoords->x1 + mainprogram->layw * 1.5f * glob->w / 2.0) {
                            if (box->scrcoords->y1 - box->scrcoords->h - 7.5 < mainprogram->iemy &&
                                mainprogram->iemy < box->scrcoords->y1 - box->scrcoords->h + 7.5) {
                                // mouse inbetween effects
                                if ((mainprogram->menuactivation || mainprogram->leftmouse || mainprogram->lmover) &&
                                    !mainprogram->drageff) {
                                    mainprogram->effectmenu->state = 2;
                                    mainmix->insert = true;
                                    mainmix->mouseeffect = j;
                                    mainmix->mouselayer = this;
                                    mainprogram->effectmenu->menux = mainprogram->mx;
                                    mainprogram->effectmenu->menuy = mainprogram->iemy;
                                    mainprogram->menuactivation = false;
                                    mainprogram->menuondisplay = true;
                                }
                                if (!mainmix->adaptparam) inbetween = true;
                                vx2 = box->vtxcoords->x1;
                                vy2 = box->vtxcoords->y1 + box->vtxcoords->h - 0.0165f;
                            }
                        }
                    }
                }
                if (!mainprogram->indragbox && mainprogram->drageffsense) {
                    mainprogram->drageff = evec[mainprogram->drageffpos];
                    mainprogram->drageffsense = false;
                }
            }

            // Draw effectmenuhints
            mainprogram->inserteffectbox->vtxcoords->x1 = 1.0f; // shove inserteffectbox offscreen
            if (!mainprogram->queueing) {
                if (vy1 < 1.0 - mainprogram->layh - 0.135f - 0.33f - 0.075f * (mainprogram->efflines - 1)) {
                    vy1 = 1.0 - mainprogram->layh - 0.135f - 0.3f - 0.075f * (mainprogram->efflines - 1);
                }

                mainprogram->addeffectbox->vtxcoords->x1 = vx1;
                mainprogram->addeffectbox->vtxcoords->y1 = vy1 - 0.08f;
                mainprogram->addeffectbox->upvtxtoscr();
                if (mainprogram->addeffectbox->in()) {
                    bottom = false;
                    draw_box(lightgrey, lightblue, mainprogram->addeffectbox, -1);
                } else {
                    draw_box(lightgrey, black, mainprogram->addeffectbox, -1);
                }
                render_text("+ Add effect", white, vx1 + 0.015f, vy1 - 0.058f, 0.00045f, 0.00075f);
                mainprogram->addeffectbox->in();

                if (inbetween) { //true when mouse inbetween effect stack entries
                    inbetween = false;
                    draw_box(lightblue, lightblue, vx2, vy2, mainprogram->layw * 1.5f, 0.033f, -1);
                    mainprogram->inserteffectbox->vtxcoords->x1 = mainprogram->mx * 2.0f / glob->w - 1.0f - 0.12f;
                    mainprogram->inserteffectbox->vtxcoords->y1 =
                            1.0f - (mainprogram->my * 2.0f / glob->h) - 0.0285f;
                    mainprogram->inserteffectbox->upvtxtoscr();
                    draw_box(white, lightblue, mainprogram->inserteffectbox, -1);
                    render_text("+ Insert effect", white, mainprogram->mx * 2.0f / glob->w - 1.0f - 0.105f,
                                1.0f - (mainprogram->my * 2.0f / glob->h) - 0.0111f, 0.00045f, 0.00075f);
                }
            }

            // handle effect drag
            if (mainprogram->drageff) {
                int pos;
                for (int j = this->effscroll[cat]; j < evec.size() + 1; j++) {
                    // calculate dragged effect temporary position pos when mouse between
                    //limits under and upper
                    int under1, under2, upper;
                    if (j == this->effscroll[cat]) {
                        // mouse above first bin
                        under2 = 0;
                        under1 = evec[this->effscroll[cat]]->box->scrcoords->y1 -
                                 evec[this->effscroll[cat]]->box->scrcoords->h * 0.5f;
                    } else {
                        under1 = evec[j - 1]->box->scrcoords->y1 + evec[j - 1]->box->scrcoords->h * 0.5f;
                        under2 = under1;
                    }
                    if (j == evec.size()) {
                        // mouse under last bin
                        upper = glob->h;
                    } else upper = evec[j]->box->scrcoords->y1 + evec[j]->box->scrcoords->h * 0.5f;
                    if (mainprogram->my > under2 && mainprogram->my < upper) {
                        std::string name = mainprogram->drageff->get_namestring();
                        draw_box(white, darkred1, mainprogram->drageff->box->vtxcoords->x1,
                                 1.0f - mainprogram->yscrtovtx(under1), mainprogram->drageff->box->vtxcoords->w,
                                 mainprogram->drageff->box->vtxcoords->h, -1);
                        render_text(name, white, mainprogram->drageff->box->vtxcoords->x1 + 0.015f,
                                    1.0f - mainprogram->yscrtovtx(under1) + 0.075f - 0.045f, 0.00045f,
                                    0.00075f);
                        pos = j;
                        break;
                    }
                }
                if (mainprogram->lmover) {
                    // do effect drag
                    Node *bulastoutnode = this->lasteffnode[cat]->out[0];
                    if (mainprogram->drageffpos < pos) {
                        std::rotate(evec.begin() + mainprogram->drageffpos, evec.begin() + mainprogram->drageffpos + 1,
                                    evec.begin() + pos);
                    } else {
                        std::rotate(evec.begin() + pos, evec.begin() + mainprogram->drageffpos,
                                    evec.begin() + mainprogram->drageffpos + 1);
                    }
                    for (int k = 0; k < evec.size(); k++) {
                        // set pos and box y1 and connect nodes for all effects in new list
                        if (k == 0) {
                            if (cat) {
                                if (this->pos == 0) {
                                    mainprogram->nodesmain->currpage->connect_nodes(this->lasteffnode[0],
                                                                                    evec[0]->node);
                                } else {
                                    mainprogram->nodesmain->currpage->connect_nodes(this->blendnode, evec[0]->node);
                                }
                            } else {
                                mainprogram->nodesmain->currpage->connect_nodes(this->node, evec[0]->node);
                            }
                        } else if (k < evec.size()) {
                            mainprogram->nodesmain->currpage->connect_nodes(evec[k - 1]->node, evec[k]->node);
                        }
                        if (k == evec.size() - 1) {
                            this->lasteffnode[cat] = evec[k]->node;
                            mainprogram->nodesmain->currpage->connect_nodes(evec[k]->node, bulastoutnode);
                        }
                        if (this->pos == 0 && this->effects[1].size() == 0) {
                            this->lasteffnode[1] = this->lasteffnode[0];
                        }
                        evec[k]->pos = k;
                        //evec->box->vtxcoords->y1 = (i + 1) * -0.05f;
                        //evec->box->upvtxtoscr();
                    }
                    mainprogram->drageff = nullptr;
                }
                if (mainprogram->rightmouse) {
                    // cancel bin drag
                    mainprogram->drageff = nullptr;
                    mainprogram->rightmouse = false;
                    mainprogram->menuactivation = false;
                    for (int i = 0; i < mainprogram->menulist.size(); i++) {
                        mainprogram->menulist[i]->state = 0;
                    }
                }
            }


            // Handle color tolerance
            if (this->pos > 0) {
                if (this->blendnode->blendtype == COLOURKEY || this->blendnode->blendtype == CHROMAKEY || this->blendnode->blendtype == LUMAKEY) {
                    Param *par = this->chtol;
                    par->box->lcolor[0] = 0.6;
                    par->box->lcolor[1] = 0.6;
                    par->box->lcolor[2] = 0.6;
                    par->box->lcolor[3] = 0.6;
                    par->box->vtxcoords->x1 = this->colorbox->vtxcoords->x1 + 0.105f;
                    par->box->vtxcoords->y1 = this->colorbox->vtxcoords->y1;
                    par->box->vtxcoords->w = 0.12f;
                    par->box->vtxcoords->h = this->colorbox->vtxcoords->h;
                    par->box->upvtxtoscr();

                    par->handle();
                }
            }


            // Draw mixfac->box
            if (this->pos > 0 && (this->blendnode->blendtype == MIXING || this->blendnode->blendtype == WIPE)) {
                Param *par = this->blendnode->mixfac;
                par->handle();
            }

            // Draw speed->box
            Param *par = this->speed;
            if (this->filename == "" || this->type == ELEM_LIVE) {
                draw_box(lightgrey, darkgrey, this->speed->box->vtxcoords->x1, this->speed->box->vtxcoords->y1,
                         this->speed->box->vtxcoords->w * 0.30f, 0.075f, -1);
            } else par->handle();
            if (par == mainmix->adaptparam) {
                for (int i = 0; i < mainmix->currlays[!mainprogram->prevmodus].size(); i++) {
                    mainmix->currlays[!mainprogram->prevmodus][i]->speed->value = par->value;
                }
            }
            mainprogram->frontbatch = true;
            draw_box(lightgrey, nullptr, this->speed->box->vtxcoords->x1, this->speed->box->vtxcoords->y1,
                     this->speed->box->vtxcoords->w * 0.30f + 0.0085f, 0.075f, -1);
            // display lock?
            if (this->lockspeed) {
                draw_box(nullptr, nullptr, this->speed->box->vtxcoords->x1 + this->speed->box->vtxcoords->w / 2.1f, this->speed->box->vtxcoords->y1 + 0.015f, this->speed->box->vtxcoords->w / 12.0f, this->speed->box->vtxcoords->h * 0.55f, mainprogram->loktex);
             }
            mainprogram->frontbatch = false;

            // Draw opacity->box
            par = this->opacity;
            if (this->filename == "" || this->type == ELEM_LIVE) {
                draw_box(lightgrey, darkgrey, this->opacity->box, -1);
            } else par->handle();
            if (par == mainmix->adaptparam) {
                for (int i = 0; i < mainmix->currlays[!mainprogram->prevmodus].size(); i++) {
                    mainmix->currlays[!mainprogram->prevmodus][i]->opacity->value = par->value;
                }
            }

            // Draw volume->box   reminder implement audio
            //if (this->audioplaying) {
            //    par = this->volume;
            //    par->handle();
            //}

            // Draw and handle playbutton revbutton bouncebutton
            if (this->playbut->box->in()) {
                this->playbut->box->acolor[0] = 0.5;
                this->playbut->box->acolor[1] = 0.5;
                this->playbut->box->acolor[2] = 1.0;
                this->playbut->box->acolor[3] = 1.0;
                if (mainprogram->leftmouse) {
                    this->playbut->value = !this->playbut->value;
                    if (this->playbut->value) this->onhold = false;
                    for (int i = 0; i < mainmix->currlays[!mainprogram->prevmodus].size(); i++) {
                        mainmix->currlays[!mainprogram->prevmodus][i]->playbut->value = this->playbut->value;
                        mainmix->currlays[!mainprogram->prevmodus][i]->set_clones();
                        if (mainmix->currlays[!mainprogram->prevmodus][i]->playbut->value) {
                            mainmix->currlays[!mainprogram->prevmodus][i]->revbut->value = false;
                            mainmix->currlays[!mainprogram->prevmodus][i]->bouncebut->value = false;
                        }
                        for (int i = 0; i < loopstation->elems.size(); i++) {
                            if (loopstation->elems[i]->recbut->value) {
                                loopstation->elems[i]->add_button_automationentry(mainmix->currlays[!mainprogram->prevmodus][i]->playbut);
                            }
                        }
                    }
                }
            } else {
                if (this->filename == "" || this->type == ELEM_LIVE) {
                    this->playbut->box->acolor[0] = 0.3;
                    this->playbut->box->acolor[1] = 0.3;
                    this->playbut->box->acolor[2] = 0.3;
                    this->playbut->box->acolor[3] = 1.0;
                } else {
                    this->playbut->box->acolor[0] = 0.3;
                    this->playbut->box->acolor[1] = 0.6;
                    this->playbut->box->acolor[2] = 0.4;
                    this->playbut->box->acolor[3] = 1.0;
                    if (this->playbut->value) {
                        this->playbut->box->acolor[0] = 0.3;
                        this->playbut->box->acolor[1] = 0.4;
                        this->playbut->box->acolor[2] = 0.7;
                        this->playbut->box->acolor[3] = 1.0;
                    }
                }
            }
            draw_box(this->playbut->box, -1);
            register_triangle_draw(white, white, this->playbut->box->vtxcoords->x1 + 0.0117f,
                                   this->playbut->box->vtxcoords->y1 + 0.0624f - 0.045f, 0.0165f,
                                   0.0312f, RIGHT, CLOSED);
            if (this->revbut->box->in()) {
                this->revbut->box->acolor[0] = 0.5;
                this->revbut->box->acolor[1] = 0.5;
                this->revbut->box->acolor[2] = 1.0;
                this->revbut->box->acolor[3] = 1.0;
                if (mainprogram->leftmouse) {
                    this->revbut->value = !this->revbut->value;
                    if (this->revbut->value) this->onhold = false;
                    for (int i = 0; i < mainmix->currlays[!mainprogram->prevmodus].size(); i++) {
                        mainmix->currlays[!mainprogram->prevmodus][i]->revbut->value = this->revbut->value;
                        mainmix->currlays[!mainprogram->prevmodus][i]->set_clones();
                        if (mainmix->currlays[!mainprogram->prevmodus][i]->revbut->value) {
                            mainmix->currlays[!mainprogram->prevmodus][i]->playbut->value = false;
                            mainmix->currlays[!mainprogram->prevmodus][i]->bouncebut->value = false;
                        }
                        for (int i = 0; i < loopstation->elems.size(); i++) {
                            if (loopstation->elems[i]->recbut->value) {
                                loopstation->elems[i]->add_button_automationentry(mainmix->currlays[!mainprogram->prevmodus][i]->revbut);
                            }
                        }
                    }
                }
            } else {
                if (this->filename == "" || this->type == ELEM_LIVE) {
                    this->revbut->box->acolor[0] = 0.3;
                    this->revbut->box->acolor[1] = 0.3;
                    this->revbut->box->acolor[2] = 0.3;
                    this->revbut->box->acolor[3] = 1.0;
                } else {
                    this->revbut->box->acolor[0] = 0.3;
                    this->revbut->box->acolor[1] = 0.6;
                    this->revbut->box->acolor[2] = 0.4;
                    this->revbut->box->acolor[3] = 1.0;
                    if (this->revbut->value) {
                        this->revbut->box->acolor[0] = 0.3;
                        this->revbut->box->acolor[1] = 0.4;
                        this->revbut->box->acolor[2] = 0.7;
                        this->revbut->box->acolor[3] = 1.0;
                    }
                }
            }
            draw_box(this->revbut->box, -1);
            register_triangle_draw(white, white, this->revbut->box->vtxcoords->x1 + 0.009375f,
                                   this->revbut->box->vtxcoords->y1 + 0.0624f - 0.045f, 0.0165f, 0.0312f,
                                   LEFT, CLOSED);
            if (this->bouncebut->box->in()) {
                this->bouncebut->box->acolor[0] = 0.5;
                this->bouncebut->box->acolor[1] = 0.5;
                this->bouncebut->box->acolor[2] = 1.0;
                this->bouncebut->box->acolor[3] = 1.0;
                if (mainprogram->leftmouse) {
                    this->bouncebut->value = !this->bouncebut->value;
                    if (this->bouncebut->value) this->onhold = false;
                    for (int i = 0; i < mainmix->currlays[!mainprogram->prevmodus].size(); i++) {
                        mainmix->currlays[!mainprogram->prevmodus][i]->bouncebut->value = this->bouncebut->value;
                        mainmix->currlays[!mainprogram->prevmodus][i]->set_clones();
                        if (mainmix->currlays[!mainprogram->prevmodus][i]->bouncebut->value) {
                            if (mainmix->currlays[!mainprogram->prevmodus][i]->revbut->value) {
                                mainmix->currlays[!mainprogram->prevmodus][i]->bouncebut->value = 2;
                                mainmix->currlays[!mainprogram->prevmodus][i]->revbut->value = 0;
                            } else mainmix->currlays[!mainprogram->prevmodus][i]->playbut->value = 0;
                        }
                        for (int i = 0; i < loopstation->elems.size(); i++) {
                            if (loopstation->elems[i]->recbut->value) {
                                loopstation->elems[i]->add_button_automationentry(mainmix->currlays[!mainprogram->prevmodus][i]->bouncebut);
                            }
                        }
                    }
                }
            } else {
                if (this->filename == "" || this->type == ELEM_LIVE) {
                    this->bouncebut->box->acolor[0] = 0.3;
                    this->bouncebut->box->acolor[1] = 0.3;
                    this->bouncebut->box->acolor[2] = 0.3;
                    this->bouncebut->box->acolor[3] = 1.0;
                } else {
                    this->bouncebut->box->acolor[0] = 0.3;
                    this->bouncebut->box->acolor[1] = 0.6;
                    this->bouncebut->box->acolor[2] = 0.4;
                    this->bouncebut->box->acolor[3] = 1.0;
                    if (this->bouncebut->value) {
                        this->bouncebut->box->acolor[0] = 0.3;
                        this->bouncebut->box->acolor[1] = 0.4;
                        this->bouncebut->box->acolor[2] = 0.7;
                        this->bouncebut->box->acolor[3] = 1.0;
                    }
                }
            }
            draw_box(this->bouncebut->box, -1);
            register_triangle_draw(white, white, this->bouncebut->box->vtxcoords->x1 + 0.009375f,
                                   this->bouncebut->box->vtxcoords->y1 + 0.0624f - 0.045f, 0.0117f,
                                   0.0312f, LEFT, CLOSED);
            register_triangle_draw(white, white, this->bouncebut->box->vtxcoords->x1 + 0.0255f,
                                   this->bouncebut->box->vtxcoords->y1 + 0.0624f - 0.045f, 0.0117f,
                                   0.0312f, RIGHT, CLOSED);

            // Draw and handle frameforward framebackward
            if (this->frameforward->box->in()) {
                this->frameforward->box->acolor[0] = 0.5;
                this->frameforward->box->acolor[1] = 0.5;
                this->frameforward->box->acolor[2] = 1.0;
                this->frameforward->box->acolor[3] = 1.0;
                if (mainprogram->leftmouse) {
                    for (int i = 0; i < mainmix->currlays[!mainprogram->prevmodus].size(); i++) {
                        mainmix->currlays[!mainprogram->prevmodus][i]->frame += 1;
                        if (mainmix->currlays[!mainprogram->prevmodus][i]->frame >= mainmix->currlays[!mainprogram->prevmodus][i]->numf)
                            mainmix->currlays[!mainprogram->prevmodus][i]->frame = 0.0f;
                    }
                    this->frameforward->box->acolor[0] = 0.3;
                    this->frameforward->box->acolor[1] = 0.4;
                    this->frameforward->box->acolor[2] = 0.7;
                    this->frameforward->box->acolor[3] = 1.0;
                    this->prevffw = true;
                    this->prevfbw = false;
                    for (int i = 0; i < loopstation->elems.size(); i++) {
                        if (loopstation->elems[i]->recbut->value) {
                            loopstation->elems[i]->add_button_automationentry(this->frameforward);
                        }
                    }
                }
            } else {
                if (this->filename == "" || this->type == ELEM_LIVE) {
                    this->frameforward->box->acolor[0] = 0.3;
                    this->frameforward->box->acolor[1] = 0.3;
                    this->frameforward->box->acolor[2] = 0.3;
                    this->frameforward->box->acolor[3] = 1.0;
                } else {
                    this->frameforward->box->acolor[0] = 0.3;
                    this->frameforward->box->acolor[1] = 0.6;
                    this->frameforward->box->acolor[2] = 0.4;
                    this->frameforward->box->acolor[3] = 1.0;
                }
            }
            draw_box(this->frameforward->box, -1);
            register_triangle_draw(white, white, this->frameforward->box->vtxcoords->x1 + 0.01125f,
                                   this->frameforward->box->vtxcoords->y1 + 0.0624f - 0.045f, 0.0165f,
                                   0.0312f, RIGHT, OPEN);
            if (this->framebackward->box->in()) {
                this->framebackward->box->acolor[0] = 0.5;
                this->framebackward->box->acolor[1] = 0.5;
                this->framebackward->box->acolor[2] = 1.0;
                this->framebackward->box->acolor[3] = 1.0;
                if (mainprogram->leftmouse) {
                    for (int i = 0; i < mainmix->currlays[!mainprogram->prevmodus].size(); i++) {
                        mainmix->currlays[!mainprogram->prevmodus][i]->frame -= 1;
                        if (mainmix->currlays[!mainprogram->prevmodus][i]->frame < 0)
                            mainmix->currlays[!mainprogram->prevmodus][i]->frame = mainmix->currlays[!mainprogram->prevmodus][i]->numf - 1.0f;
                    }
                    this->framebackward->box->acolor[0] = 0.3;
                    this->framebackward->box->acolor[1] = 0.4;
                    this->framebackward->box->acolor[2] = 0.7;
                    this->framebackward->box->acolor[3] = 1.0;
                    this->prevfbw = true;
                    this->prevffw = false;
                    for (int i = 0; i < loopstation->elems.size(); i++) {
                        if (loopstation->elems[i]->recbut->value) {
                            loopstation->elems[i]->add_button_automationentry(this->framebackward);
                        }
                    }
                }
            } else {
                if (this->filename == "" || this->type == ELEM_LIVE) {
                    this->framebackward->box->acolor[0] = 0.3;
                    this->framebackward->box->acolor[1] = 0.3;
                    this->framebackward->box->acolor[2] = 0.3;
                    this->framebackward->box->acolor[3] = 1.0;
                } else {
                    this->framebackward->box->acolor[0] = 0.3;
                    this->framebackward->box->acolor[1] = 0.6;
                    this->framebackward->box->acolor[2] = 0.4;
                    this->framebackward->box->acolor[3] = 1.0;
                }
            }
            draw_box(this->framebackward->box, -1);
            register_triangle_draw(white, white, this->framebackward->box->vtxcoords->x1 + 0.009375f,
                                   this->framebackward->box->vtxcoords->y1 + 0.0624f - 0.045f, 0.0165f,
                                   0.0312f, LEFT, OPEN);


            if (this->stopbut->box->in()) {
                this->stopbut->box->acolor[0] = 0.5;
                this->stopbut->box->acolor[1] = 0.5;
                this->stopbut->box->acolor[2] = 1.0;
                this->stopbut->box->acolor[3] = 1.0;
                if (mainprogram->leftmouse) {
                   this->onhold = true;
                    for (int i = 0; i < mainmix->currlays[!mainprogram->prevmodus].size(); i++) {
                        mainmix->currlays[!mainprogram->prevmodus][i]->set_clones();
                        mainmix->currlays[!mainprogram->prevmodus][i]->playbut->value = false;
                        mainmix->currlays[!mainprogram->prevmodus][i]->revbut->value = false;
                        mainmix->currlays[!mainprogram->prevmodus][i]->bouncebut->value = false;
                        this->frame = 0.0f;
                        for (int i = 0; i < loopstation->elems.size(); i++) {
                            if (loopstation->elems[i]->recbut->value) {
                                loopstation->elems[i]->add_button_automationentry(mainmix->currlays[!mainprogram->prevmodus][i]->stopbut);
                            }
                        }
                    }
                }
            } else {
                if (this->filename == "" || this->type == ELEM_LIVE) {
                    this->stopbut->box->acolor[0] = 0.3;
                    this->stopbut->box->acolor[1] = 0.3;
                    this->stopbut->box->acolor[2] = 0.3;
                    this->stopbut->box->acolor[3] = 1.0;
                } else {
                    this->stopbut->box->acolor[0] = 0.3;
                    this->stopbut->box->acolor[1] = 0.6;
                    this->stopbut->box->acolor[2] = 0.4;
                    this->stopbut->box->acolor[3] = 1.0;
                }
            }
            draw_box(this->stopbut->box, -1);
            draw_box(white, white, this->stopbut->box->vtxcoords->x1 + 0.0117f,
                                   this->stopbut->box->vtxcoords->y1 + 0.0624f - 0.045f, 0.0165f,
                                   0.0312f, -1);
            
            
            if (this->lpbut->box->in()) {
                this->lpbut->box->acolor[0] = 0.5;
                this->lpbut->box->acolor[1] = 0.5;
                this->lpbut->box->acolor[2] = 1.0;
                this->lpbut->box->acolor[3] = 1.0;
                if (mainprogram->leftmouse) {
                    this->lpbut->value = !this->lpbut->value;
                }
            } else {
                if (this->type == ELEM_LIVE) {
                    this->lpbut->box->acolor[0] = 0.3;
                    this->lpbut->box->acolor[1] = 0.3;
                    this->lpbut->box->acolor[2] = 0.3;
                    this->lpbut->box->acolor[3] = 1.0;
                } else {
                    this->lpbut->box->acolor[0] = 0.3;
                    this->lpbut->box->acolor[1] = 0.6;
                    this->lpbut->box->acolor[2] = 0.4;
                    this->lpbut->box->acolor[3] = 1.0;
                    if (this->lpbut->value) {
                        this->lpbut->box->acolor[0] = 0.3;
                        this->lpbut->box->acolor[1] = 0.4;
                        this->lpbut->box->acolor[2] = 0.7;
                        this->lpbut->box->acolor[3] = 1.0;
                    }
                }
            }
            draw_box(this->lpbut->box, -1);
            render_text("LP", white, this->lpbut->box->vtxcoords->x1 + 0.009375f,
                                   this->lpbut->box->vtxcoords->y1 + 0.0624f - 0.045f, 0.00075f, 0.0012f);
            // Draw and handle genmidibutton
            mainprogram->handle_button(this->genmidibut, 0, 0);
            for (int i = 0; i < mainmix->currlays[!mainprogram->prevmodus].size(); i++) {
                mainmix->currlays[!mainprogram->prevmodus][i]->genmidibut->value = this->genmidibut->value;
                //mainmix->currlays[!mainprogram->prevmodus][i]->set_clones();  // reminder: only if changed
            }
            std::string butstr;
            if (this->genmidibut->value == 0) butstr = "off";
            else if (this->genmidibut->value == 1) butstr = "A";
            else if (this->genmidibut->value == 2) butstr = "B";
            else if (this->genmidibut->value == 3) butstr = "C";
            else if (this->genmidibut->value == 4) butstr = "D";
            render_text(butstr, white, this->genmidibut->box->vtxcoords->x1 + 0.0117f,
                        this->genmidibut->box->vtxcoords->y1 + 0.0624f - 0.045f, 0.00045f, 0.00075f);

            // Draw and handle loopbox
            bool ends = false;
            if (this->loopbox->in()) {
                //special cases
                if (pdistance(mainprogram->mx, mainprogram->my, this->loopbox->scrcoords->x1 + this->startframe->value *
                                                                                               (this->loopbox->scrcoords->w /
                                                                                                (this->numf - 1)),
                              this->loopbox->scrcoords->y1, this->loopbox->scrcoords->x1 + this->startframe->value *
                                                                                           (this->loopbox->scrcoords->w /
                                                                                            (this->numf - 1)),
                              this->loopbox->scrcoords->y1 - mainprogram->yvtxtoscr(0.045f)) < 6) {
                    ends = true;
                    if (mainprogram->ctrl && mainprogram->leftmousedown) {
                        this->scritching = 2;
                        mainprogram->leftmousedown = false;
                    }
                    if (mainprogram->ctrl && mainprogram->leftmousedown) {
                        this->scritching = 2;
                        mainprogram->leftmousedown = false;
                    }
                    if (mainprogram->menuactivation) {
                        mainprogram->parammenu3->state = 2;
                        mainmix->learnparam = this->startframe;
                        mainmix->learnbutton = nullptr;
                        this->startframe->range[1] = this->numf - 1.0f;
                        mainprogram->menuactivation = false;
                    }
                } else if (pdistance(mainprogram->mx, mainprogram->my, this->loopbox->scrcoords->x1 + this->startframe->value *
                                                                                                      (this->loopbox->scrcoords->w /
                                                                                                       (this->numf -
                                                                                                        1)) +
                                                                       (this->endframe->value - this->startframe->value) *
                                                                       (this->loopbox->scrcoords->w / (this->numf - 1)),
                                     this->loopbox->scrcoords->y1, this->loopbox->scrcoords->x1 + this->startframe->value *
                                                                                                  (this->loopbox->scrcoords->w /
                                                                                                   (this->numf - 1)) +
                                                                   (this->endframe->value - this->startframe->value) *
                                                                   (this->loopbox->scrcoords->w / (this->numf - 1)),
                                     this->loopbox->scrcoords->y1 - mainprogram->yvtxtoscr(0.045f)) < 6) {
                    ends = true;
                    if (mainprogram->ctrl && mainprogram->leftmousedown) {
                        this->scritching = 3;
                        mainprogram->leftmousedown = false;
                    }
                    if (mainprogram->menuactivation) {
                        mainprogram->parammenu3->state = 2;
                        mainmix->learnparam = this->endframe;
                        mainmix->learnbutton = nullptr;
                        this->endframe->range[1] = this->numf - 1.0f;
                        mainprogram->menuactivation = false;
                    }
                } else if (mainprogram->mx > this->loopbox->scrcoords->x1 +
                                             this->startframe->value * (this->loopbox->scrcoords->w / (this->numf - 1)) &&
                           mainprogram->mx < this->loopbox->scrcoords->x1 +
                                             this->startframe->value * (this->loopbox->scrcoords->w / (this->numf - 1)) +
                                             (this->endframe->value - this->startframe->value) *
                                             (this->loopbox->scrcoords->w / (this->numf - 1))) {
                    if (mainprogram->ctrl && mainprogram->leftmousedown) {
                        mainmix->prevx = mainprogram->mx;
                        this->scritching = 5;
                        mainprogram->leftmousedown = false;
                    }
                }
                if (mainprogram->menuactivation && !ends) {
                    mainprogram->loopmenu->state = 2;
                    mainmix->mouselayer = this;
                    mainprogram->menuactivation = false;
                }

            }
            this->oldframe = this->frame;
            this->oldstartframe = this->startframe->value;
            this->oldendframe = this->endframe->value;
            if (this->loopbox->in()) {
                if (mainprogram->leftmousedown) {
                    this->scritching = 1;
                    //this->frame = (this->numf - 1.0f) *
                                 // ((mainprogram->mx - this->loopbox->scrcoords->x1) / this->loopbox->scrcoords->w);
                    this->set_clones();
                }
            }
            if (this->scritching) mainprogram->leftmousedown = false;
            if (this->scritching == 1) {
                this->frame = (this->numf - 1) *
                              ((mainprogram->mx - this->loopbox->scrcoords->x1) / this->loopbox->scrcoords->w);
                if (this->frame < 0) this->frame = 0.0f;
                else if (this->frame >= this->numf) this->frame = this->numf - 1;
                this->set_clones();
                if (mainprogram->leftmouse && !mainprogram->menuondisplay) this->scritching = 4;
            } else if (this->scritching == 2) {
                // ctrl leftmouse dragging loop start
                this->startframe->value = (this->numf - 1) *
                                   ((mainprogram->mx - this->loopbox->scrcoords->x1) / this->loopbox->scrcoords->w);
                if (this->startframe->value < 0) this->startframe->value = 0.0f;
                else if (this->startframe->value >= this->numf) this->startframe->value = this->numf - 1;
                if (this->startframe->value > this->frame) this->frame = this->startframe->value;
                if (this->startframe->value > this->endframe->value) {
                    this->startframe->value = this->endframe->value;
                    this->frame = this->endframe->value;
                }
                this->set_clones();
                if (mainprogram->leftmouse) this->scritching = 0;
            } else if (this->scritching == 3) {
                // ctrl leftmouse dragging loop end
                this->endframe->value = (this->numf - 1) *
                                 ((mainprogram->mx - this->loopbox->scrcoords->x1) / this->loopbox->scrcoords->w);
                if (this->endframe->value < this->frame) this->frame = this->endframe->value;
                if (this->endframe->value < this->startframe->value) {
                    this->endframe->value = this->startframe->value;
                    this->frame = this->endframe->value;
                }
                else if (this->endframe->value >= this->numf) this->endframe->value = this->numf - 1;
                //if (this->endframe->value < this->frame) this->frame = this->endframe->value;
                //if (this->endframe->value < this->startframe->value) this->endframe->value = this->startframe->value;
                this->set_clones();
                if (mainprogram->leftmouse) this->scritching = 0;
            } else if (this->scritching == 5) {
                // ctrl leftmouse dragging loop
                //float start = 0.0f;
                //float end = 0.0f;
                float looplen = this->endframe->value - this->startframe->value;
                this->startframe->value +=
                        (mainprogram->mx - mainmix->prevx) / (this->loopbox->scrcoords->w / (this->numf - 1));
                if (this->startframe->value + looplen >= this->numf) this->startframe->value = this->oldstartframe;
                if (this->startframe->value < 0) {
                    //start = this->startframe->value - 1;
                    this->startframe->value = 0.0f;
                } else if (this->startframe->value >= this->numf) this->startframe->value = this->numf - 1;
                if (this->startframe->value > this->frame) this->frame = this->startframe->value;
                if (this->startframe->value > this->endframe->value) this->startframe->value = this->endframe->value;
                this->endframe->value +=
                        (mainprogram->mx - mainmix->prevx) / (this->loopbox->scrcoords->w / (this->numf - 1));
                if (this->endframe->value - looplen < 0.0f) this->endframe->value = this->oldendframe;
                if (this->endframe->value < this->frame) this->frame = this->endframe->value;
                if (this->endframe->value < 0) {
                    this->endframe->value = 0.0f;
                }
                else if (this->endframe->value >= this->numf) {
                    //end = this->endframe->value - (this->numf - 1);
                    this->endframe->value = this->numf - 1;
                }
                mainmix->prevx = mainprogram->mx;
                //if (this->endframe->value < this->frame) this->frame = this->endframe->value;
                //if (this->endframe->value < this->startframe->value) this->endframe->value = this->startframe->value;
                this->set_clones();
                if (mainprogram->leftmouse) this->scritching = 0;
            }
            if (this->frame != this->oldframe && this->scritching == 1) {
                // standard scritching is loopstationed
                this->scritch->value = this->frame;  // set scritching parameter for loopstation
                for (int i = 0; i < loopstation->elems.size(); i++) {
                    if (loopstation->elems[i]->recbut->value) {
                        loopstation->elems[i]->add_param_automationentry(this->scritch);
                    }
                }
                this->loopbox->acolor[0] = this->scritch->box->acolor[0];
                this->loopbox->acolor[1] = this->scritch->box->acolor[1];
                this->loopbox->acolor[2] = this->scritch->box->acolor[2];
                this->loopbox->acolor[3] = this->scritch->box->acolor[3];
            }
            if (this->startframe->value != this->oldstartframe) {
                // startframe is loopstationed
                for (int i = 0; i < loopstation->elems.size(); i++) {
                    if (loopstation->elems[i]->recbut->value) {
                        loopstation->elems[i]->add_param_automationentry(this->startframe);
                    }
                }
            }
            if (this->endframe->value != this->oldendframe) {
                // endframe is loopstationed
                for (int i = 0; i < loopstation->elems.size(); i++) {
                    if (loopstation->elems[i]->recbut->value) {
                        loopstation->elems[i]->add_param_automationentry(this->endframe);
                    }
                }
            }
            draw_box(this->loopbox, -1);
			if (ends) {
			    // mouse over looparea start or end
				draw_box(lightgrey, blue, this->loopbox->vtxcoords->x1 + this->startframe->value * (this->loopbox->vtxcoords->w / (this->numf - 1)), this->loopbox->vtxcoords->y1, (this->endframe->value - this->startframe->value) * (this->loopbox->vtxcoords->w / (this->numf - 1)), 0.075f, -1);
			}
			else {
				draw_box(lightgrey, green, this->loopbox->vtxcoords->x1 + this->startframe->value * (this->loopbox->vtxcoords->w / (this->numf - 1)), this->loopbox->vtxcoords->y1, (this->endframe->value - this->startframe->value) * (this->loopbox->vtxcoords->w / (this->numf - 1)), 0.075f, -1);
			}
			draw_box(white, white, this->loopbox->vtxcoords->x1 + this->frame * (this->loopbox->vtxcoords->w /
                                                                                 (this->numf - 1)) - 0.002f,
            this->loopbox->vtxcoords->y1, 0.004f, 0.075f, -1);

			if (this->speed->value == 0.0f || this->type == ELEM_IMAGE || this->type == ELEM_LIVE) {
                render_text("--:-- / --:--", white, this->loopbox->vtxcoords->x1 + this->loopbox->vtxcoords->w + 0.015f, this->loopbox->vtxcoords->y1 + 0.075f - 0.045f, 0.0006f, 0.001f);
			}
			else {
                std::string min1 = std::to_string(
                        (int) ((this->millif * (this->frame - this->startframe->value) / (this->speed->value * this->speed->value)) / 60000));
                std::string sec1 = std::to_string(
                        (int) ((this->millif * (this->frame - this->startframe->value) / (this->speed->value * this->speed->value)) / 1000) % 60);
                if (sec1.length() == 1) sec1 = "0" + sec1;
                std::string min2 = std::to_string(
                        (int) ((this->millif * (this->endframe->value - this->startframe->value) / (this->speed->value * this->speed->value)) / 60000));
                std::string sec2 = std::to_string(
                        (int) ((this->millif * (this->endframe->value - this->startframe->value) / (this->speed->value * this->speed->value)) / 1000) % 60);
                if (sec2.length() == 1) sec2 = "0" + sec2;
                render_text(min1 + ":" + sec1 + " / " + min2 + ":" + sec2, white,
                            this->loopbox->vtxcoords->x1 + this->loopbox->vtxcoords->w + 0.015f,
                            this->loopbox->vtxcoords->y1 + 0.075f - 0.045f, 0.0006f, 0.001f);
            }


			// Draw and handle chdir chinv
			if (this->pos > 0) {
                if (this->blendnode->blendtype == COLOURKEY || this->blendnode->blendtype == CHROMAKEY ||
                this->blendnode->blendtype == LUMAKEY) {
					if (this->chdir->box->in()) {
						if (mainprogram->leftmouse) {
							this->chdir->value = !this->chdir->value;
							GLint chdir = glGetUniformLocation(mainprogram->ShaderProgram, "chdir");
							glUniform1i(chdir, this->chdir->value);
						}
					}
					if (this->chdir->value) {
						this->chdir->box->acolor[1] = 0.7f;
					}
					else {
						this->chdir->box->acolor[1] = 0.0f;
					}
					draw_box(this->chdir->box, -1);
					render_text("D", white, this->chdir->box->vtxcoords->x1 + 0.0117f, this->chdir->box->vtxcoords->y1 + 0.0624f - 0.045f, 0.00045f, 0.00075f);
					if (this->chinv->box->in()) {
						if (mainprogram->leftmouse) {
							this->chinv->value = !this->chinv->value;
							GLint chinv = glGetUniformLocation(mainprogram->ShaderProgram, "chinv");
							glUniform1i(chinv, this->chinv->value);
						}
					}
					if (this->chinv->value) {
                        draw_box(white, green, this->chinv->box, -1);
					}
					else {
                        draw_box(white, black, this->chinv->box, -1);
					}
					render_text("I", white, this->chinv->box->vtxcoords->x1 + 0.0117f, this->chinv->box->vtxcoords->y1 + 0.0624f - 0.045f, 0.00045f, 0.00075f);
				}
			}
		}
	}

    //this->startframe->handle();
    this->startframe->range[1] = this->numf - 1.0f;
    //this->endframe->handle();
    this->endframe->range[1] = this->numf - 1.0f;
}


// MIXER OUTPUTS DISPLAY

void Mixer::outputmonitors_handle() {
	// Draw mix view
	if (this->mode == 0) {
		std::vector<Boxx*> boxes;
        boxes.push_back(mainprogram->deckmonitor[0]);
        boxes.push_back(mainprogram->deckmonitor[1]);
        boxes.push_back(mainprogram->mainmonitor);
        boxes.push_back(mainprogram->outputmonitor);
		// Handle mixtargets
		for (int i = 0; i < boxes.size(); i++) {
            Boxx *outputbox = boxes[i];
            if (outputbox->in()) {

                if (i == 0) {
                    render_text("Deck A Monitor", white, outputbox->vtxcoords->x1 + 0.015f,
                                outputbox->vtxcoords->y1 + outputbox->vtxcoords->h - 0.045f, 0.0005f, 0.0008f);
                } else if (i == 1) {
                    render_text("Deck B Monitor", white, outputbox->vtxcoords->x1 + 0.015f,
                                outputbox->vtxcoords->y1 + outputbox->vtxcoords->h - 0.045f, 0.0005f, 0.0008f);
                } else if (i == 2 && mainprogram->prevmodus) {
                    render_text("Preview Mix Monitor", white, outputbox->vtxcoords->x1 + 0.015f,
                                outputbox->vtxcoords->y1 + outputbox->vtxcoords->h - 0.045f, 0.0005f, 0.0008f);
                } else if (mainprogram->prevmodus && i == 3) {
                    render_text("Output Mix Monitor", white, outputbox->vtxcoords->x1 + 0.015f,
                                outputbox->vtxcoords->y1 + outputbox->vtxcoords->h - 0.045f, 0.0005f, 0.0008f);
                } else if (!mainprogram->prevmodus && i == 2) {
                    render_text("Output Mix Monitor", white, outputbox->vtxcoords->x1 + 0.015f,
                                outputbox->vtxcoords->y1 + outputbox->vtxcoords->h - 0.045f, 0.0005f, 0.0008f);
                }

                if (mainprogram->doubleleftmouse) {
                    mainprogram->fullscreen = i;
                }
                if (mainprogram->menuactivation) {
                    mainprogram->monitormenu->state = 2;
                    mainprogram->monitormenu->value = i;
                    if (!mainprogram->prevmodus && i == 2) {
                        mainprogram->monitormenu->value = 3;
                    }
                    mainprogram->menuactivation = false;
                }
            }
        }

        // Handle wipes
        int deck = -1;
        if (mainprogram->deckmonitor[0]->in()) deck = 0;
        else if (mainprogram->deckmonitor[1]->in()) deck = 1;
        if (deck > -1) {
            if (mainprogram->leftmousedown) {
                mainprogram->wiping = true;
                this->currlay[!mainprogram->prevmodus]->blendnode->wipex->value = -(((1.0f - ((mainprogram->xscrtovtx(mainprogram->mx) - 0.55f - deck * 0.9f) / 0.3f)) - 0.5f) * 2.0f - 1.5f);
                this->currlay[!mainprogram->prevmodus]->blendnode->wipey->value = -((((2.0f - mainprogram->yscrtovtx(mainprogram->my)) / 0.3f) - 0.5f) * 2.0f - 0.50f);
                for (int i = 0; i < loopstation->elems.size(); i++) {
                    if (loopstation->elems[i]->recbut->value) {
                        loopstation->elems[i]->add_param_automationentry(this->currlay[!mainprogram->prevmodus]->blendnode->wipex);
                    }
                }
            }
            else {
                mainprogram->wiping = false;
            }
        }

        for (int i = 0; i < 2; i++) {
            bool in = false;
            if (i == 0) {
                if (mainprogram->prevmodus) {
                    if (mainprogram->mainmonitor->in()) in = true;
                    else if (mainprogram->outputmonitor->in()) in = true;
                }
            }
            else if (i == 1) {
                if (!mainprogram->prevmodus) {
                    if (mainprogram->mainmonitor->in()) in = true;
                }
            }
            if (!in) continue;
			if (mainprogram->leftmousedown) {
				this->wipex[i]->value = -(((1.0f - ((mainprogram->xscrtovtx(mainprogram->mx) - 0.7f) / 0.6f)) - 0.5f) * 2.0f - 0.5f);
				this->wipey[i]->value = -((((2.0f - mainprogram->yscrtovtx(mainprogram->my)) / 0.6f) - 0.5f) * 2.0f - 0.5f);
				for (int i = 0; i < loopstation->elems.size(); i++) {
					if (loopstation->elems[i]->recbut->value) {
						loopstation->elems[i]->add_param_automationentry(this->wipex[!i]);
					}
				}
			}
			if (mainprogram->middlemouse) {
				GLint wipe = glGetUniformLocation(mainprogram->ShaderProgram, "wipe");
				glUniform1i(wipe, 0);
			}
		}

		// Handle vidboxes
		Layer* lay = this->currlay[!mainprogram->prevmodus];
		Effect* eff;

		Boxx *box;
		if (this->currlay[!mainprogram->prevmodus]) {
			// Handle mixbox
			box = lay->mixbox;
			box->acolor[0] = 0.0;
			box->acolor[1] = 0.0;
			box->acolor[2] = 0.0;
			box->acolor[3] = 1.0;
			if (!mainprogram->menuondisplay) {
				if (box->in()) {
					if (mainprogram->menuactivation || mainprogram->leftmouse) {
						mainprogram->mixenginemenu->state = 2;
						for (int i = 0; i < mainprogram->menulist.size(); i++) {
							mainprogram->menulist[i]->menux = mainprogram->mx;
							mainprogram->menulist[i]->menuy = mainprogram->my;
						}
						this->mousenode = lay->blendnode;
						mainprogram->menuactivation = false;
					}
					box->acolor[0] = 0.5;
					box->acolor[1] = 0.5;
					box->acolor[2] = 1.0;
					box->acolor[3] = 1.0;
				}
			}
		}
	}
}



// SWITCH LAYER TO LIVE INPUT (WEBCAM,...)
bool Layer::find_new_live_base(int pos) {
    avformat_close_input(&mainprogram->busylayers[pos]->video);
	int size = mainprogram->mimiclayers.size();
	for (int i = 0; i < size; i++) {
		Layer* mlay = mainprogram->mimiclayers[i];
		if (mlay->liveinput->filename == this->filename) {
			mlay->liveinput = nullptr;
			mlay->texture = -1;
			mlay->initialized = false;
			mainprogram->mimiclayers.erase(mainprogram->mimiclayers.begin() + i);
			mainprogram->busylist[pos] = mlay->filename;
			mainprogram->busylayers[pos] = mlay;
#ifdef WINDOWS
			mlay->ifmt = av_find_input_format("dshow");
#else
#ifdef POSIX
			mlay->ifmt = (AVInputFormat*)av_find_input_format("v4l2");
#endif
#endif
			mlay->skip = true;
			mlay->vidopen = true;
			mlay->ready = true;
			while (mlay->ready) {
				mlay->startdecode.notify_all();
			}
			for (int j = 0; j < mainprogram->mimiclayers.size(); j++) {
				if (mainprogram->mimiclayers[j]->liveinput == this) {
					mainprogram->mimiclayers[j]->liveinput = mlay;
				}
			}
			return true;
		}
	}
	return false;
}

void Layer::set_live_base(std::string livename) {
	this->decresult->width = 0;
	this->vidformat = 0;
	this->initialized = false;
	this->audioplaying = false;
	if (this->clonesetnr != -1) {
        int clnr = this->clonesetnr;
        this->clonesetnr = -1;
        this->texture = -1;
        mainmix->clonesets[clnr]->erase(this);

        if (mainmix->clonesets[clnr]->size() == 0) {
            mainmix->cloneset_destroy(clnr);
        }
		this->clonesetnr = -1;
	}

	this->filename = livename;
	if (this->video) {
		this->ready = true;
		while (this->ready) {
			this->startdecode.notify_all();
		}
		std::unique_lock<std::mutex> lock(this->enddecodelock);
		this->enddecodevar.wait(lock, [&] {return this->processed; });
		this->vidopen = true;
		lock.unlock();
		this->processed = false;
		avformat_close_input(&this->video);
	}
	else this->type = ELEM_LIVE;
	avdevice_register_all();
	ptrdiff_t pos = std::find(mainprogram->busylist.begin(), mainprogram->busylist.end(), livename) - mainprogram->busylist.begin();
	if (pos >= mainprogram->busylist.size()) {
		mainprogram->busylist.push_back(this->filename);
		mainprogram->busylayers.push_back(this);
#ifdef WINDOWS
		this->ifmt = av_find_input_format("dshow");
#else
#ifdef POSIX
		this->ifmt = (AVInputFormat*)av_find_input_format("v4l2");
#endif
#endif
		this->reset = false;
		this->frame = 0.0f;
		this->prevframe = -1;
		this->numf = 0;
		this->startframe->value = 0;
		this->endframe->value = 0;
		this->skip = true;
		this->vidopen = true;
		this->ready = true;
		while (this->ready) {
			this->startdecode.notify_all();
		}
	}
	else {
		this->liveinput = mainprogram->busylayers[pos];
		mainprogram->mimiclayers.push_back(this);
	}
}



// DRAGGING

void Mixer::layerdrag_handle() {
	// dragging layers around
	if (mainprogram->nodesmain->linked && this->currlay[!mainprogram->prevmodus]) {
		std::vector<Layer*>& lvec = choose_layers(this->currlay[!mainprogram->prevmodus]->deck);
		Layer* lay = this->currlay[!mainprogram->prevmodus];
		Boxx* box = lay->node->vidbox;
		if (lay->vidmoving) {
			if (mainprogram->binsscreen) {
				float boxwidth = 0.3f;
				float nmx = mainprogram->xscrtovtx(mainprogram->mx) + boxwidth / 2.0f;
				float nmy = 2.0 - mainprogram->yscrtovtx(mainprogram->my) - boxwidth / 2.0f;
				draw_box(nullptr, black, -1.0f - 0.5 * boxwidth + nmx, -1.0f - 0.5 * boxwidth + nmy, boxwidth, boxwidth, mainprogram->dragbinel->tex);
			}

			if (mainprogram->lmover) {
				mainprogram->rightmouse = true;
				binsmain->handle(0);
                enddrag();
                mainprogram->rightmouse = false;
				bool ret;
				if (!mainprogram->binsscreen) {
					if (mainprogram->prevmodus) {
						if (mainprogram->mx < glob->w / 2.0f) ret = lay->exchange(lvec, this->layers[0], 0);
						else ret = lay->exchange(lvec, this->layers[1], 1);
					}
					else {
						if (mainprogram->mx < glob->w / 2.0f) ret = lay->exchange(lvec, this->layers[2], 0);
						else ret = lay->exchange(lvec, this->layers[3], 1);
					}
				}
			}
		}
	}
}

void Mixer::deckmixdrag_handle() {
	//deck and mix dragging
    bool loadev = true;
    if (mainprogram->shelfdragelem) loadev = false;  // dont load loopstation events from shelf

    if (mainprogram->dropfiles.size()) {
        // SDL drag'n'drop of decks and mixes
        std::string df = mainprogram->dropfiles[0];
        if (isdeckfile(df)) {
            mainprogram->dragbinel = new BinElement;
            mainprogram->dragbinel->type = ELEM_DECK;
            mainprogram->dragbinel->path = df;
            mainprogram->lmover = true;
        }
        if (ismixfile(df)) {
            mainprogram->dragbinel = new BinElement;
            mainprogram->dragbinel->type = ELEM_MIX;
            mainprogram->dragbinel->path = df;
            mainprogram->lmover = true;
        }
    }

    if (mainprogram->dragbinel) {
        if (mainprogram->dragbinel->type == ELEM_DECK) {
            if (!mainprogram->binsscreen) {
                // check drop of dragged deck into mix
                std::unique_ptr <Boxx> boxA = std::make_unique <Boxx> ();;
                boxA->vtxcoords->x1 = -1.0f + mainprogram->numw;
                boxA->vtxcoords->y1 = 1.0f - mainprogram->layh;
                boxA->vtxcoords->w = mainprogram->layw * 3;
                boxA->vtxcoords->h = mainprogram->layh;
                boxA->upvtxtoscr();
                std::unique_ptr <Boxx> boxB = std::make_unique <Boxx> ();;
                boxB->vtxcoords->x1 = mainprogram->numw;
                boxB->vtxcoords->y1 = 1.0f - mainprogram->layh;
                boxB->vtxcoords->w = mainprogram->layw * 3;
                boxB->vtxcoords->h = mainprogram->layh;
                boxB->upvtxtoscr();
                if (boxA->in(mainprogram->mx, mainprogram->my)) {
                	mainmix->dropdeckblue = 1;
                    if (mainprogram->lmover) {
                        mainmix->mousedeck = 0;
                        mainmix->open_deck(mainprogram->dragbinel->path, 1, loadev);
                        std::vector<Layer*>& lvec = choose_layers(0);
                        for (Layer *lay : lvec) {
                            if (std::find(lay->prevshelfdragelems.begin(), lay->prevshelfdragelems.end(), mainprogram->shelfdragelem) != lay->prevshelfdragelems.end()) {
                                if (mainprogram->shelfdragelem->launchtype == 1) {
                                    if (mainprogram->shelfdragelem->clayers.size()) {
                                        lay->frame = mainprogram->shelfdragelem->clayers[0]->frame;
                                    }
                                } else if (mainprogram->shelfdragelem->launchtype == 2) {
                                    if (mainprogram->shelfdragelem->nblayers.size()) {
                                        lay->frame = mainprogram->shelfdragelem->nblayers[0]->frame;
                                    }
                                }
                            }
                        }
                        mainprogram->rightmouse = true;
                        binsmain->handle(0);
                        enddrag();
                        mainprogram->rightmouse = false;
                    }
                }
                if (boxB->in(mainprogram->mx, mainprogram->my)) {
                	mainmix->dropdeckblue = 2;
                    if (mainprogram->lmover) {
                        mainmix->mousedeck = 1;
                        mainmix->open_deck(mainprogram->dragbinel->path, 1, loadev);
                        mainprogram->rightmouse = true;
                        binsmain->handle(0);
                        enddrag();
                        mainprogram->rightmouse = false;
                    }
                }
            }
        } else if (mainprogram->dragbinel->type == ELEM_MIX) {
            if (!mainprogram->binsscreen) {
                // check drop of dragged mix into layer stacks
                std::unique_ptr <Boxx> box = std::make_unique <Boxx> ();;
                box->vtxcoords->x1 = -1.0f + mainprogram->numw;
                box->vtxcoords->y1 = 1.0f - mainprogram->layh;
                box->vtxcoords->w = mainprogram->layw * 6 + mainprogram->numw;
                box->vtxcoords->h = mainprogram->layh;
                box->upvtxtoscr();
                if (box->in(mainprogram->mx, mainprogram->my)) {
                	mainmix->dropmixblue = 1;
                    draw_box(nullptr, lightblue, -1.0f + mainprogram->numw, 1.0f - mainprogram->layh,
                             mainprogram->layw * 6 + mainprogram->numw, mainprogram->layh, -1);
                    if (mainprogram->lmover) {
                        mainmix->open_mix(mainprogram->dragbinel->path.c_str(), true, loadev);
                        mainprogram->rightmouse = true;
                        binsmain->handle(0);
                        enddrag();
                        mainprogram->rightmouse = false;
                    }
                }
            }
        }
        ShelfElement* elem = mainprogram->shelfdragelem;
    }
}

void ShelfElement::set_nbclayers(Layer *lay) {
    LoopStation *lpst;
    if (mainprogram->prevmodus) {
        lpst = lp;
    } else {
        lpst = lpc;
    }
    lay->lpst->init();
    int count = 0;
    for (LoopStationElement *elem: lpst->elems) {
        LoopStationElement *layelem = lay->lpst->elems[count++];
        layelem->effcatposns.clear();
        layelem->effposns.clear();
        layelem->parposns.clear();
        layelem->buteffcatposns.clear();
        layelem->buteffposns.clear();
        layelem->butposns.clear();
        layelem->compareelems.clear();
        for (int n = 0; n < 2; n++) {
            for (int i = 0; i < lay->effects[n].size(); i++) {
                Effect *eff = lay->effects[n][i];
                for (int j = 0; j < eff->params.size(); j++) {
                    Param *par = eff->params[j];
                    if (elem->params.find(par) != elem->params.end()) {
                        layelem->starttime = elem->starttime;
                        layelem->interimtime = elem->interimtime;
                        layelem->totaltime = elem->totaltime;
                        layelem->speedadaptedtime = elem->speedadaptedtime;
                        layelem->eventlist = elem->eventlist;
                        layelem->eventpos = elem->eventpos;
                        layelem->atend = elem->atend;
                        layelem->effcatposns.push_back(n);
                        layelem->effposns.push_back(i);
                        layelem->parposns.push_back(j);
                        //layelem->compareelems.push_back(layelem->pos);

                        layelem->recbut->value = elem->recbut->value;
                        layelem->loopbut->value = elem->loopbut->value;
                        layelem->playbut->value = elem->playbut->value;
                        layelem->speed->value = elem->speed->value;
                    }
                }
            }
        }
        layelem->butposns.clear();
        for (auto it = mainprogram->buttons.begin(); it != mainprogram->buttons.end(); ++it) {
            Button *but = it->second;
            if (std::find(elem->buttons.begin(), elem->buttons.end(), but) != elem->buttons.end()) {
                layelem->starttime = elem->starttime;
                layelem->interimtime = elem->interimtime;
                layelem->totaltime = elem->totaltime;
                layelem->speedadaptedtime = elem->speedadaptedtime;
                layelem->eventlist = elem->eventlist;
                layelem->eventpos = elem->eventpos;
                layelem->atend = elem->atend;
                layelem->buteffcatposns.push_back(-1);
                layelem->buteffposns.push_back(-1);
                for (int n = 0; n < 2; n++) {
                    for (int k = 0; k < lay->effects[n].size(); k++) {
                        if (but == lay->effects[n][k]->onoffbutton) {
                            layelem->buteffcatposns.push_back(n);
                            layelem->buteffposns.push_back(k);
                        }
                    }
                }
                layelem->butposns.push_back(but->butid);
                //layelem->compareelems.push_back(layelem->pos);
            }
        }
    }
}

bool Mixer::set_prevshelfdragelem(Layer* lay) {  // reminder : check if prevshelfdragelems function
    ShelfElement *elem = nullptr;
    if (lay->prevshelfdragelems.size() > lay->psde_size) {
        elem = lay->prevshelfdragelems.back();
        lay->psde_size++;
    }

    if (elem) {
		std::vector<Layer*>& lvec1 = choose_layers(lay->deck);
		bool cond = true;
		if (elem) {
			if (elem->type == ELEM_DECK) {
				cond = false;
				if (elem->launchtype == 1) {
                    elem->prevclayers = elem->clayers;
					elem->clayers.clear();
					for (int i = 0; i < lvec1.size(); i++) {
						elem->clayers.push_back(lvec1[i]);
                        elem->set_nbclayers(lvec1[i]);
                        //lvec1[i]->prevshelfdragelems.pop_back();
                    }
				}
				else if (elem->launchtype == 2) {
                    elem->prevnblayers = elem->nblayers;
					elem->nblayers.clear();
					for (int i = 0; i < lvec1.size(); i++) {
						lvec1[i]->olddeckspeed = mainmix->deckspeed[!mainprogram->prevmodus][lay->deck]->value;
                        elem->nblayers.push_back(lvec1[i]);
						elem->set_nbclayers(lvec1[i]);
                        //lvec1[i]->prevshelfdragelems.pop_back();
					}
				}
			}
			else if (elem->type == ELEM_MIX) {
				cond = false;
				if (elem->launchtype == 1) {
                    elem->prevclayers = elem->clayers;
					elem->clayers.clear();
					for (int m = 0; m < 2; m++) {
						std::vector<Layer*>& lvec2 = choose_layers(m);
						for (int i = 0; i < lvec2.size(); i++) {
							elem->clayers.push_back(lvec2[i]);  // reminder : possibly clayers einhalt is deleted somewhere?
                            elem->set_nbclayers(lvec2[i]);
                            //lvec2[i]->prevshelfdragelems.pop_back();
                        }
					}
				}
				else if (elem->launchtype == 2) {
                    elem->prevnblayers = elem->nblayers;
					elem->nblayers.clear();
					for (int m = 0; m < 2; m++) {
						std::vector<Layer*>& lvec2 = choose_layers(m);
						for (int i = 0; i < lvec2.size(); i++) {
							lvec2[i]->olddeckspeed = mainmix->deckspeed[!mainprogram->prevmodus][m]->value;
                            elem->nblayers.push_back(lvec2[i]);
                            elem->set_nbclayers(lvec2[i]);
                            //lvec2[i]->prevshelfdragelems.pop_back();
						}
					}
				}
			}
			else cond = true;
		}
		if (cond) {
            // when not ELEM_DECK or ELEM_MIX
			if (elem->launchtype == 1) {
                elem->prevclayers = elem->clayers;
				elem->clayers.clear();
				elem->clayers.push_back(lay);
                elem->set_nbclayers(lay);
			}
			if (elem->launchtype == 2) {
                elem->prevnblayers = elem->nblayers;
				elem->nblayers.clear();
                lay->olddeckspeed = mainmix->deckspeed[!mainprogram->prevmodus][lay->deck]->value;
                elem->nblayers.push_back(lay);
				elem->set_nbclayers(lay);
			}
            //lay->prevshelfdragelems.pop_back();
        }
		return false;
	}
	else return true;
}

void Mixer::open_dragbinel(Layer *thislay) {
    this->open_dragbinel(thislay, -1);
}

void Mixer::open_dragbinel(Layer *thislay, int i, bool uselayers) {
    // open element dragged to layer stack or double-clicked from shelf
    Layer *newlay = thislay;
    std::vector<Layer*> &lvec = choose_layers(newlay->deck);
    if (mainprogram->dragbinel->type == ELEM_LAYER) {
        //thislay->layers = &lvec;
        //mainprogram->gettinglayertex = true;
        //mainprogram->gettinglayertexlay = thislay;
        newlay = mainmix->open_layerfile(mainprogram->dragbinel->path, thislay, true, true, uselayers);
        //mainprogram->gettinglayertexlay = nullptr;
        //mainprogram->gettinglayertex = false;

        // when something new is dragged into layer, set frames from framecounters set in Mixer::set_prevshelfdragelem()
        if (mainprogram->shelfdragelem) {
            if (mainprogram->shelfdragelem->launchtype == 1) {
                newlay->prevshelfdragelems = thislay->prevshelfdragelems;
                newlay->prevshelfdragelems.push_back(mainprogram->shelfdragelem);
            } else if (mainprogram->shelfdragelem->launchtype == 2) {
                newlay->prevshelfdragelems = thislay->prevshelfdragelems;
                newlay->prevshelfdragelems.push_back(mainprogram->shelfdragelem);
            }
        }
        //thislay->layers = &lvec;
        thislay->set_inlayer(newlay, false);
        if (i != -1) {
            mainmix->currlays[!mainprogram->prevmodus][i] = newlay;
            mainmix->currlay[!mainprogram->prevmodus] = newlay;
        }

    } else if (mainprogram->dragbinel->type == ELEM_FILE) {
        newlay = thislay->open_video(0, mainprogram->dragbinel->path, true);
        /*std::unique_lock<std::mutex> olock(thislay->endopenlock);
        thislay->endopenvar.wait(olock, [&] {return thislay->opened; });
        thislay->opened = false;
        olock.unlock();*/
        if (i != -1) {
            mainmix->currlays[!mainprogram->prevmodus][i] = newlay;
            mainmix->currlay[!mainprogram->prevmodus] = newlay;
        }
        if (newlay != thislay) {
            //if (thislay->initialized) mainmix->copy_pbos(newlay, thislay);
            //mainmix->delete_layer(*thislay->layers, thislay, false);
        }
        //newlay->prevshelfdragelems = thislay->prevshelfdragelems;
        //newlay->prevshelfdragelems.push_back(mainprogram->shelfdragelem);
        // when something new is dragged into layer, set frames from framecounters set in Mixer::set_prevshelfdragelem()
        if (mainprogram->shelfdragelem) {
            if (mainprogram->shelfdragelem->launchtype == 1) {
                newlay->prevshelfdragelems = thislay->prevshelfdragelems;
                newlay->prevshelfdragelems.push_back(mainprogram->shelfdragelem);
            } else if (mainprogram->shelfdragelem->launchtype == 2) {
                newlay->prevshelfdragelems = thislay->prevshelfdragelems;
                newlay->prevshelfdragelems.push_back(mainprogram->shelfdragelem);
            }
        }
    } else if (mainprogram->dragbinel->type == ELEM_IMAGE) {
        newlay = thislay->open_image(mainprogram->dragbinel->path);
        if (i != -1) {
            mainmix->currlays[!mainprogram->prevmodus][i] = newlay;
            mainmix->currlay[!mainprogram->prevmodus] = newlay;
        }
        if (thislay->initialized) {
            mainmix->copy_pbos(newlay, thislay);
        }
        //newlay->prevshelfdragelems = thislay->prevshelfdragelems;
        //newlay->prevshelfdragelems.push_back(mainprogram->shelfdragelem);
        newlay->frame = 0.0f;
        // when something new is dragged into layer, set frames from framecounters set in Mixer::set_prevshelfdragelem()
        if (mainprogram->shelfdragelem) {
            if (mainprogram->shelfdragelem->launchtype == 1) {
                newlay->prevshelfdragelems = thislay->prevshelfdragelems;
                newlay->prevshelfdragelems.push_back(mainprogram->shelfdragelem);
            } else if (mainprogram->shelfdragelem->launchtype == 2) {
                newlay->prevshelfdragelems = thislay->prevshelfdragelems;
                newlay->prevshelfdragelems.push_back(mainprogram->shelfdragelem);
            }
        }
    }
    //std::vector<Layer*> &lvec = choose_layers(thislay->deck);
    mainmix->reconnect_all(lvec);

    if (newlay != thislay) {
        if (newlay->bufbotex == -1) {
            newlay->bufbotex = copy_tex(thislay->fbotex, glob->w, glob->h);
        }
        else newlay->node->vidbox->tex = newlay->bufbotex;
        //mainmix->nlaymap[newlay] = thislay;  reminder : nlaymap system redundant?
    }

    if (std::find(newlay->prevshelfdragelems.begin(), newlay->prevshelfdragelems.end(), mainprogram->shelfdragelem) != newlay->prevshelfdragelems.end()) {
        if (mainprogram->shelfdragelem->launchtype == 1) {
            if (mainprogram->shelfdragelem->clayers.size()) {
                newlay->frame = mainprogram->shelfdragelem->clayers[0]->frame;
                this->copy_lpst(newlay, mainprogram->shelfdragelem->clayers[0], true, false);
            }
        } else if (mainprogram->shelfdragelem->launchtype == 2) {
            if (mainprogram->shelfdragelem->nblayers.size()) {
                newlay->frame = mainprogram->shelfdragelem->nblayers[0]->frame;
                this->copy_lpst(newlay, mainprogram->shelfdragelem->nblayers[0], true, false);
            }
        }
    }
}

void Mixer::copy_lpst(Layer *destlay, Layer *srclay, bool global, bool back) {
    // copy lpst values saved in nblayers or clayers back to the newly created layers (shelf trigger launchtype 2 & 1)
    LoopStation *lpst;
    LoopStation *lpst2;
    if (global) {
        if (mainprogram->prevmodus) {
            lpst = lp;
        } else {
            lpst = lpc;
        }
    }
    else {
        lpst = destlay->lpst;
    }
    
    if (back) {
        if (mainprogram->prevmodus) {
            lpst2 = lp;
        } else {
            lpst2 = lpc;
        }
    }
    else {
        lpst2 = srclay->lpst;
    }
    
    //int count = 0;
    for (int i = 0; i < lpst2->elems.size(); i++) {
        LoopStationElement *layelem = lpst2->elems[i];
        if (layelem->eventlist.size() == 0) continue;

        LoopStationElement *elem = nullptr;

        /*for (LoopStationElement *selem: lpst->elems) {
            if (selem->comparepos == layelem->compareelems[count]) {
                elem = selem;
            }
        }
        count++;
        if (!elem) {*/
        for (LoopStationElement *selem: lpst->elems) {
            if (selem->eventlist.size() == 0) {
                elem = selem;
                break;
            }
        }
        if (elem == nullptr) continue;
    //}

        std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
        elem->starttime = now - std::chrono::milliseconds((long long)(layelem->speedadaptedtime));
        std::chrono::duration<double> elapsed = std::chrono::milliseconds((int)(layelem->interimtime));
        long long millicount = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
        elem->interimtime = layelem->interimtime;
        elem->totaltime = layelem->totaltime;
        elem->speedadaptedtime = layelem->speedadaptedtime;
        std::tuple<long long, Param*, Button*, float> event;
        for (int j = 0; elem->eventlist.size(); j++) {
            event = elem->eventlist[std::clamp(j, 0, (int) elem->eventlist.size() - 1)];
            if (elem->speedadaptedtime > std::get<0>(event)) {
                elem->eventpos = j;
                break;
            }
        }
        elem->atend = layelem->atend;

        /*if (writeevents) {
            Param *par = nullptr;
            Button *but = nullptr;
            for (int i = 0; i < layelem->parposns.size(); i++) {
                par = destlay->effects[layelem->effcatposns[i]][layelem->effposns[i]]->params[layelem->parposns[i]];
            }
            for (int i = 0; i < layelem->butposns.size(); i++) {
                if (layelem->buteffcatposns[i] != -1) {
                    but = destlay->effects[layelem->buteffcatposns[i]][layelem->buteffposns[i]]->onoffbutton;
                } else {
                    for (auto it = mainprogram->buttons.begin(); it != mainprogram->buttons.end(); ++it) {
                        if (it->second->butid == layelem->butposns[i]) {
                            if (it->second->layer == destlay) {
                                but = it->second;
                            }
                        }
                    }
                }
            }

            if (par) {
                elem->params.emplace(par);
                lpst->parelemmap[par] = elem;
                lpst->allparams.push_back(par);
                par->box->acolor[0] = elem->colbox->acolor[0];
                par->box->acolor[1] = elem->colbox->acolor[1];
                par->box->acolor[2] = elem->colbox->acolor[2];
                par->box->acolor[3] = elem->colbox->acolor[3];
            }
            if (but) {
                elem->buttons.emplace(but);
                lpst->butelemmap[but] = elem;
                lpst->allbuttons.push_back(but);
                but->box->acolor[0] = elem->colbox->acolor[0];
                but->box->acolor[1] = elem->colbox->acolor[1];
                but->box->acolor[2] = elem->colbox->acolor[2];
                but->box->acolor[3] = elem->colbox->acolor[3];
            }
            elem->recbut->value = layelem->recbut->value;
            elem->loopbut->value = layelem->loopbut->value;
            elem->playbut->value = layelem->playbut->value;
            elem->speed->value = layelem->speed->value;

            std::tuple<long long, Param *, Button *, float> event1;
            std::tuple<long long, Param *, Button *, float> event2;
            std::tuple<long long, Param *, Button *, float> event3;
            elem->eventlist.clear();
            for (int j = 0; j < layelem->eventlist.size(); j++) {
                event1 = layelem->eventlist[j];
                event3 = std::make_tuple(std::get<0>(event1), par, but, std::get<3>(event1));
                //if (elem->eventlist.size() == 0) {
                elem->eventlist.push_back(event3);
                //}
                //else {
                //    for (int k = 0; k < elem->eventlist.size(); k++) {
                //        event2 = elem->eventlist[k];
                //        if (std::get<0>(event1) > std::get<0>(event2)) {
                //            elem->eventlist.insert(elem->eventlist.begin() + k + 1, event3);
                //        }
                //    }
                //}
            }
        }*/
    }
}


void Mixer::reconnect_all(std::vector<Layer*> &layers) {
    // clean up deleted layers
    dellayslock.lock();
    for (Layer* lay : mainprogram->dellays) {
        if (std::find(layers.begin(), layers.end(), lay) != layers.end()) {
            layers.erase(std::find(layers.begin(), layers.end(), lay));
        }
    }
    dellayslock.unlock();
    // connect all nodes correctly if (layers.size() > 0) {
    for (int j = 0; j < layers.size(); j++) {
        // set lasteffnodes
        if (layers[j]->effects[0].size()) {
            layers[j]->lasteffnode[0] = layers[j]->effects[0].back()->node;
        }
        else layers[j]->lasteffnode[0] = layers[j]->node;
        if (j == 0) layers[j]->lasteffnode[1] = layers[j]->lasteffnode[0];
        if (layers[j]->effects[1].size()) {
            layers[j]->lasteffnode[1] = layers[j]->effects[1].back()->node;
        }
        else if (j > 0) {
            layers[j]->lasteffnode[1] = layers[j]->blendnode;
        }
    }
    for (int j = 0; j < layers.size(); j++) {
        // reconnect everything in the layer stack
        layers[j]->pos = j;
        if (j == 0) {
            if (j != layers.size() - 1)
                mainprogram->nodesmain->currpage->connect_nodes(layers[j]->lasteffnode[1],
                                                                layers[j + 1]->blendnode);
            if (layers[j]->effects[1].size()) {
                mainprogram->nodesmain->currpage->connect_nodes(layers[j]->lasteffnode[0],
                                                                layers[j]->lasteffnode[1]);
            }
        }
        else {
            mainprogram->nodesmain->currpage->connect_in2(layers[j]->lasteffnode[0], layers[j]->blendnode);
            if (layers[j]->effects[1].size()) {
                mainprogram->nodesmain->currpage->connect_nodes(layers[j]->blendnode,
                                                                layers[j]->effects[1][0]->node);
                if (j + 1 < layers.size()) {
                    mainprogram->nodesmain->currpage->connect_nodes(layers[j]->lasteffnode[1],
                                                                    layers[j + 1]->blendnode);
                }
            }
            else if (j != layers.size() - 1) mainprogram->nodesmain->currpage->connect_nodes(layers[j]->blendnode, layers[j + 1]->blendnode);
        }
    }

    int j = layers.size() - 1;

    // reconnect to end of layer stack MIX nodes
    if (j != -1) {
        for (int i = 0; i < 4; i++) {
            if (&layers == &mainmix->layers[i]) {
                mainprogram->nodesmain->mixnodes[i / 2][i % 2]->in = nullptr;
                mainprogram->nodesmain->currpage->connect_nodes(layers[j]->lasteffnode[1],
                                                                mainprogram->nodesmain->mixnodes[i / 2][i % 2]);
            }
        }
    }
    for (int j = 0; j < layers.size(); j++) {
        if (layers[j]->mutebut->value) {
            if (layers[j]->pos > 0) {
                if (layers[j]->blendnode->in) {
                    layers[j]->blendnode->in->out.clear();
                    mainprogram->nodesmain->currpage->connect_nodes(layers[j]->blendnode->in,
                                                                    layers[j]->lasteffnode[1]->out[0]);
                }
            } else if (layers[j]->next() != layers[j]) layers[j]->next()->blendnode->in = nullptr;
        }
    }
}


bool Layer::exchange(std::vector<Layer*>& slayers, std::vector<Layer*>& dlayers, bool deck) {
	// regulates the reordering of layers around the stacks by dragging
	// both exchanging two layers and moving one layer
	bool ret = false;
	int size = dlayers.size();
	Layer* inlay = nullptr;
	for (int i = 0; i < size; i++) {
		inlay = dlayers[i];
		bool comp = !mainprogram->prevmodus;
		if (inlay->pos < mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos || inlay->pos > mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos + 2) continue;
		Boxx* box = inlay->node->vidbox;
		int endx = false;
		if ((i == dlayers.size() - 1 || i == mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos + 2) && (box->scrcoords->x1 + box->scrcoords->w - mainprogram->xvtxtoscr(0.1125f) < mainprogram->mx && mainprogram->mx < box->scrcoords->x1 + box->scrcoords->w + mainprogram->xvtxtoscr(0.075f))) {
		    endx = true;
		}
		bool dropin = false;
		int numonscreen = size - mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos;
		if (0 <= numonscreen && numonscreen <= 2) {
			if (mainprogram->xvtxtoscr(mainprogram->numw + deck * 1.0f + numonscreen * mainprogram->layw) < mainprogram->mx && mainprogram->mx < deck * (glob->w / 2.0f) + glob->w / 2.0f) {
				if (0 < mainprogram->my && mainprogram->my < mainprogram->yvtxtoscr(mainprogram->layh)) {
					if (size == i + 1) {
						dropin = true;
						endx = 1;
					}
				}
			}
		}
		if (dropin || (box->scrcoords->y1 < mainprogram->my + box->scrcoords->h && mainprogram->my < box->scrcoords->y1)) {
			if ((box->scrcoords->x1 + mainprogram->xvtxtoscr(0.075f) < mainprogram->mx && mainprogram->mx < box->scrcoords->x1 + box->scrcoords->w - mainprogram->xvtxtoscr(0.075f))) {
				if (this == inlay) return false;
				ret = mainmix->set_prevshelfdragelem(this);  // reminder : of inlay
				//exchange two layers

				bool indeck = inlay->deck;
				BlendNode* bnode = inlay->blendnode;
				BLEND_TYPE btype = bnode->blendtype;
				VideoNode* node = inlay->node;
				Node* len1 = inlay->lasteffnode[1];

				slayers[this->pos] = inlay;
				slayers[this->pos]->pos = this->pos;
				slayers[this->pos]->deck = this->deck;
				slayers[this->pos]->node = this->node;
				slayers[this->pos]->node->layer = inlay;
				slayers[this->pos]->blendnode = this->blendnode;
				slayers[this->pos]->blendnode->blendtype = this->blendnode->blendtype;
				slayers[this->pos]->blendnode->mixfac->value = this->blendnode->mixfac->value;
				slayers[this->pos]->blendnode->wipetype = this->blendnode->wipetype;
				slayers[this->pos]->blendnode->wipedir = this->blendnode->wipedir;
				slayers[this->pos]->blendnode->wipex->value = this->blendnode->wipex->value;
				slayers[this->pos]->blendnode->wipey->value = this->blendnode->wipey->value;
				if (this->pos == 0) inlay->lasteffnode[1] = inlay->lasteffnode[0];
				else inlay->lasteffnode[1] = this->lasteffnode[1];

				dlayers[i] = this;
				dlayers[i]->pos = i;
				dlayers[i]->deck = indeck;
				dlayers[i]->node = node;
				dlayers[i]->node->layer = this;
				dlayers[i]->blendnode = bnode;
				dlayers[i]->blendnode->blendtype = btype;
				dlayers[i]->blendnode->mixfac->value = bnode->mixfac->value;
				dlayers[i]->blendnode->wipetype = bnode->wipetype;
				dlayers[i]->blendnode->wipedir = bnode->wipedir;
				dlayers[i]->blendnode->wipex->value = bnode->wipex->value;
				dlayers[i]->blendnode->wipey->value = bnode->wipey->value;
				if (i == 0) this->lasteffnode[1] = this->lasteffnode[0];
				else this->lasteffnode[1] = len1;

				Node* inlayout = inlay->lasteffnode[0]->out[0];
				Node* layout = this->lasteffnode[0]->out[0];
				if (inlay->effects[0].size()) {
					inlay->node->out.clear();
					mainprogram->nodesmain->currpage->connect_nodes(inlay->node, inlay->effects[0][0]->node);
					inlay->lasteffnode[0] = inlay->effects[0][inlay->effects[0].size() - 1]->node;
				}
				else inlay->lasteffnode[0] = inlay->node;
				inlay->lasteffnode[0]->out.clear();
				if (inlay->pos == 0) {
					layout->in = nullptr;
					mainprogram->nodesmain->currpage->connect_nodes(inlay->lasteffnode[0], layout);
				}
				else {
					((BlendNode*)(layout))->in2 = nullptr;
					mainprogram->nodesmain->currpage->connect_in2(inlay->lasteffnode[0], (BlendNode*)(layout));
				}
				for (int j = 0; j < inlay->effects[0].size(); j++) {
					inlay->effects[0][j]->node->align = inlay->node;
				}
				if (this->effects[0].size()) {
					this->node->out.clear();
					mainprogram->nodesmain->currpage->connect_nodes(this->node, this->effects[0][0]->node);
					this->lasteffnode[0] = this->effects[0][this->effects[0].size() - 1]->node;
				}
				else this->lasteffnode[0] = this->node;
				this->lasteffnode[0]->out.clear();
				if (this->pos == 0) {
					inlayout->in = nullptr;
					mainprogram->nodesmain->currpage->connect_nodes(this->lasteffnode[0], inlayout);
				}
				else {
					((BlendNode*)(inlayout))->in2 = nullptr;
					mainprogram->nodesmain->currpage->connect_in2(this->lasteffnode[0], (BlendNode*)inlayout);
				}
				for (int j = 0; j < this->effects[0].size(); j++) {
					this->effects[0][j]->node->align = this->node;
				}
				//if (inlay->filename == "") {
				//	mainmix->delete_layer(slayers, inlay, false);
				//}
				if (slayers.size() == 0) {
					Layer* newlay = mainmix->add_layer(slayers, 0);
					if (&slayers == &mainmix->layers[0]) {
						mainprogram->nodesmain->currpage->connect_nodes(newlay->lasteffnode[0], mainprogram->nodesmain->mixnodes[0][0]);
					}
					else if (&slayers == &mainmix->layers[1]) {
						mainprogram->nodesmain->currpage->connect_nodes(newlay->lasteffnode[0], mainprogram->nodesmain->mixnodes[0][1]);
					}
					else if (&slayers == &mainmix->layers[2]) {
						mainprogram->nodesmain->currpage->connect_nodes(newlay->lasteffnode[0], mainprogram->nodesmain->mixnodes[1][0]);
					}
					else if (&slayers == &mainmix->layers[3]) {
						mainprogram->nodesmain->currpage->connect_nodes(newlay->lasteffnode[0], mainprogram->nodesmain->mixnodes[1][1]);
					}
				}
				break;
			}
			else if (dropin || endx || (box->scrcoords->x1 - mainprogram->xvtxtoscr(0.075f) * (i - mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos != 0) < mainprogram->mx && mainprogram->mx < box->scrcoords->x1 + mainprogram->xvtxtoscr(0.075f))) {
				if (this == dlayers[i]) return false;
				//if (this == dlayers[i - (i > 0)]) return false;

				ret = true;
				int pos = i;
				//move one layer
				BLEND_TYPE nextbtype;
				float nextmfval = 0.0f;
				int nextwipetype = 0;
				int nextwipedir = 0;
				float nextwipex = 0.0f;
				float nextwipey = 0.0f;
				Layer* nextlay = nullptr;
				Layer* nextnextlay = nullptr;
				if (slayers.size() > this->pos + 1) {
					nextlay = slayers[this->pos + 1];
					nextbtype = nextlay->blendnode->blendtype;
					if (slayers.size() > nextlay->pos + 1) {
						nextnextlay = slayers[nextlay->pos + 1];
						//nextbtype = nextlay->blendnode->blendtype;
					}
					else nextnextlay = this;
				}
				this->deck = deck;
				BLEND_TYPE btype = this->blendnode->blendtype;
				BlendNode* firstbnode = (BlendNode*)dlayers[0]->lasteffnode[0]->out[0];
				Node* firstlasteffnode = dlayers[0]->lasteffnode[0];
				float mfval = this->blendnode->mixfac->value;
				int wipetype = this->blendnode->wipetype;
				int wipedir = this->blendnode->wipedir;
				float wipex = this->blendnode->wipex->value;
				float wipey = this->blendnode->wipey->value;
				if (this->pos > 0) {
					this->blendnode->in->out.clear();
					mainprogram->nodesmain->currpage->connect_nodes(this->blendnode->in, this->lasteffnode[1]->out[0]);
					mainprogram->nodesmain->currpage->delete_node(this->blendnode);
				}
				else if (pos + endx != 0) {
					if (nextlay) {
						nextlay->lasteffnode[0]->out.clear();
						if (nextlay->effects[1].size()) {
							mainprogram->nodesmain->currpage->connect_nodes(nextlay->lasteffnode[0], nextlay->effects[1][0]->node);
						}
						else {
							nextlay->lasteffnode[1] = nextlay->lasteffnode[0];
						}
						mainprogram->nodesmain->currpage->delete_node(nextlay->blendnode);
						nextlay->blendnode = new BlendNode;
						nextlay->blendnode->blendtype = nextbtype;
						nextlay->blendnode->mixfac->value = nextmfval;
						nextlay->blendnode->wipetype = nextwipetype;
						nextlay->blendnode->wipedir = nextwipedir;
						nextlay->blendnode->wipex->value = nextwipex;
						nextlay->blendnode->wipey->value = nextwipey;
					}
				}
				this->blendnode = nullptr;
				bool lastisvid = false;
				if (this->lasteffnode[0] == this->node) lastisvid = true;
				mainprogram->nodesmain->currpage->delete_node(this->node);

				// do layer move in layer vectors
				int slayerssize = slayers.size();
				for (int j = 0; j < slayerssize; j++) {
					if (slayers[j] == this) {
						slayers.erase(slayers.begin() + j);
						if (slayers == dlayers && pos > j) {
							pos--;
							nextnextlay = nullptr;
						}
						break;
					}
				}
				bool comp;
				if (dlayers == mainmix->layers[0] || dlayers == mainmix->layers[1]) comp = false;
				else comp = true;
				for (int j = 0; j < slayers.size(); j++) {
					slayers[j]->pos = j;
				}
				dlayers.insert(dlayers.begin() + pos + endx, this);
				for (int j = 0; j < dlayers.size(); j++) {
					dlayers[j]->pos = j;
				}
				//// end of layer move

				if (dlayers[pos + endx]->pos == mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos + 3) {
					// inserted at visual layer stack end
					mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos++;
				}
				dlayers[pos + endx]->node = mainprogram->nodesmain->currpage->add_videonode(comp);
				dlayers[pos + endx]->node->layer = dlayers[pos + endx];
				for (int j = 0; j < dlayers[pos + endx]->effects[0].size(); j++) {
					if (j == 0) mainprogram->nodesmain->currpage->connect_nodes(dlayers[pos + endx]->node, dlayers[pos + endx]->effects[0][j]->node);
					dlayers[pos + endx]->effects[0][j]->node->align = dlayers[pos + endx]->node;
				}
				if (lastisvid) dlayers[pos + endx]->lasteffnode[0] = dlayers[pos + endx]->node;
				if (dlayers[pos + endx]->pos > 0) {
					Layer* prevlay = dlayers[dlayers[pos + endx]->pos - 1];
					BlendNode* bnode = mainprogram->nodesmain->currpage->add_blendnode(MIXING, comp);
					bnode->blendtype = btype;
					dlayers[pos + endx]->blendnode = bnode;
					dlayers[pos + endx]->blendnode->mixfac->value = mfval;
					dlayers[pos + endx]->blendnode->wipetype = wipetype;
					dlayers[pos + endx]->blendnode->wipedir = wipedir;
					dlayers[pos + endx]->blendnode->wipex->value = wipex;
					dlayers[pos + endx]->blendnode->wipey->value = wipey;
					mainprogram->nodesmain->currpage->connect_nodes(prevlay->lasteffnode[1], dlayers[pos + endx]->lasteffnode[0], dlayers[pos + endx]->blendnode);
					if (dlayers[pos + endx]->effects[1].size()) {
						dlayers[pos + endx]->blendnode->out.clear();
						mainprogram->nodesmain->currpage->connect_nodes(dlayers[pos + endx]->blendnode, dlayers[pos + endx]->effects[1][0]->node);
					}
					else {
						dlayers[pos + endx]->lasteffnode[1] = dlayers[pos + endx]->blendnode;
					}
					if (dlayers[pos + endx]->pos == dlayers.size() - 1 && mainprogram->nodesmain->mixnodes[0].size()) {
						if (&dlayers == &mainmix->layers[0]) {
							mainprogram->nodesmain->currpage->connect_nodes(dlayers[pos + endx]->lasteffnode[1], mainprogram->nodesmain->mixnodes[0][0]);
						}
						else if (&dlayers == &mainmix->layers[1]) {
							mainprogram->nodesmain->currpage->connect_nodes(dlayers[pos + endx]->lasteffnode[1], mainprogram->nodesmain->mixnodes[0][1]);
						}
						else if (&dlayers == &mainmix->layers[2]) {
							mainprogram->nodesmain->currpage->connect_nodes(dlayers[pos + endx]->lasteffnode[1], mainprogram->nodesmain->mixnodes[1][0]);
						}
						else if (&dlayers == &mainmix->layers[3]) {
							mainprogram->nodesmain->currpage->connect_nodes(dlayers[pos + endx]->lasteffnode[1], mainprogram->nodesmain->mixnodes[1][1]);
						}
					}

					else if (dlayers[pos + endx]->pos < dlayers.size() - 1) {
						mainprogram->nodesmain->currpage->connect_nodes(dlayers[pos + endx]->lasteffnode[1], dlayers[dlayers[pos + endx]->pos + 1]->blendnode);
					}
				}
				else {
					dlayers[pos + endx]->blendnode = new BlendNode;
					//BlendNode *bnode = mainprogram->nodesmain->currpage->add_blendnode(MIXING, false);
					if (dlayers[pos + endx]->effects[1].size()) {
						dlayers[pos + endx]->lasteffnode[0]->out.clear();
						mainprogram->nodesmain->currpage->connect_nodes(dlayers[pos + endx]->lasteffnode[0], dlayers[pos + endx]->effects[1][0]->node);
					}
					else {
						dlayers[pos + endx]->lasteffnode[1] = dlayers[pos + endx]->lasteffnode[0];
					}
					Layer* nxlay = nullptr;
					if (dlayers.size() > 2) nxlay = dlayers[1];
					if (nxlay) {
						Node* cpn = nxlay->lasteffnode[1]->out[0];
						firstlasteffnode->out.clear();
						BlendNode* bnode = mainprogram->nodesmain->currpage->add_blendnode(MIXING, comp);
						nxlay->blendnode = bnode;
						dlayers[pos + endx]->lasteffnode[1]->out.clear();
						mainprogram->nodesmain->currpage->connect_nodes(dlayers[pos + endx]->lasteffnode[1], firstlasteffnode, bnode);
						if (nxlay->effects[1].size()) {
							mainprogram->nodesmain->currpage->connect_nodes(bnode, nxlay->effects[1][0]->node);
							nxlay->lasteffnode[1]->out.clear();
							mainprogram->nodesmain->currpage->connect_nodes(nxlay->lasteffnode[1], cpn);
						}
						else {
						    mainprogram->nodesmain->currpage->connect_nodes(bnode, cpn);
                            nxlay->lasteffnode[1] = bnode;
						}
					}
					else {
						BlendNode* bnode = mainprogram->nodesmain->currpage->add_blendnode(MIXING, comp);
						dlayers[pos + endx]->lasteffnode[1]->out.clear();
						dlayers[1]->blendnode = bnode;
						dlayers[1]->blendnode->blendtype = MIXING;
						mainprogram->nodesmain->currpage->connect_nodes(dlayers[pos + endx]->lasteffnode[1], dlayers[1]->lasteffnode[0], bnode);
						dlayers[1]->lasteffnode[1] = bnode;
						if (&dlayers == &mainmix->layers[0]) {
							mainprogram->nodesmain->currpage->connect_nodes(bnode, mainprogram->nodesmain->mixnodes[0][0]);
						}
						else if (&dlayers == &mainmix->layers[1]) {
							mainprogram->nodesmain->currpage->connect_nodes(bnode, mainprogram->nodesmain->mixnodes[0][1]);
						}
						else if (&dlayers == &mainmix->layers[2]) {
							mainprogram->nodesmain->currpage->connect_nodes(bnode, mainprogram->nodesmain->mixnodes[1][0]);
						}
						else if (&dlayers == &mainmix->layers[3]) {
							mainprogram->nodesmain->currpage->connect_nodes(bnode, mainprogram->nodesmain->mixnodes[1][1]);
						}
					}

				}
				dlayers[pos + endx]->blendnode->blendtype = btype;
				dlayers[pos + endx]->blendnode->mixfac->value = mfval;
				dlayers[pos + endx]->blendnode->wipetype = wipetype;
				dlayers[pos + endx]->blendnode->wipedir = wipedir;
				dlayers[pos + endx]->blendnode->wipex->value = wipex;
				dlayers[pos + endx]->blendnode->wipey->value = wipey;
				if (nextnextlay) {
					nextlay->lasteffnode[1]->out.clear();
					nextnextlay->blendnode->in = nullptr;
					mainprogram->nodesmain->currpage->connect_nodes(nextlay->lasteffnode[1], nextnextlay->blendnode);
				}

				if (slayers.empty()) {
					Layer* newlay = mainmix->add_layer(slayers, 0);
					if (&slayers == &mainmix->layers[0]) {
						mainprogram->nodesmain->currpage->connect_nodes(newlay->lasteffnode[0], mainprogram->nodesmain->mixnodes[0][0]);
					}
					else if (&slayers == &mainmix->layers[1]) {
						mainprogram->nodesmain->currpage->connect_nodes(newlay->lasteffnode[0], mainprogram->nodesmain->mixnodes[0][1]);
					}
					else if (&slayers == &mainmix->layers[2]) {
						mainprogram->nodesmain->currpage->connect_nodes(newlay->lasteffnode[0], mainprogram->nodesmain->mixnodes[1][0]);
					}
					else if (&slayers == &mainmix->layers[3]) {
						mainprogram->nodesmain->currpage->connect_nodes(newlay->lasteffnode[0], mainprogram->nodesmain->mixnodes[1][1]);
					}
				}

                mainmix->reconnect_all(slayers);
                mainmix->reconnect_all(dlayers);

				break;
			}
		}
    }

	if (ret && this->prevshelfdragelems.size()) {
		if (!this->dummy) {
            ShelfElement *elem = this->prevshelfdragelems.back();
            bool ret = mainmix->set_prevshelfdragelem(this);
            if (!ret && elem->type == ELEM_DECK) {
                if (elem->launchtype == 2) {
                    mainmix->mousedeck = elem->nblayers[0]->deck;
                    mainmix->do_save_deck(mainprogram->temppath + "tempdeck_lnch.deck", false, false);
                    mainmix->open_deck(mainprogram->temppath + "tempdeck_lnch.deck", false);
                }
            } else if (!ret && elem->type == ELEM_MIX) {
                if (elem->launchtype == 2) {
                    mainmix->do_save_mix(mainprogram->temppath + "tempdeck_lnch.deck", mainprogram->prevmodus,
                                         false);
                    mainmix->open_mix(mainprogram->temppath + "tempdeck_lnch.deck", false);
                }
            }
        }
		if (!inlay->dummy) {
			ShelfElement* elem = inlay->prevshelfdragelems.back();
			bool ret = mainmix->set_prevshelfdragelem(inlay);
			if (!ret && elem->type == ELEM_DECK) {
				if (elem->launchtype == 2) {
					mainmix->mousedeck = elem->nblayers[0]->deck;
					mainmix->do_save_deck(mainprogram->temppath + "tempdeck_lnch.deck", false, false);
					mainmix->open_deck(mainprogram->temppath + "tempdeck_lnch.deck", false);
				}
			}
			else if (!ret && elem->type == ELEM_MIX) {
				if (elem->launchtype == 2) {
					mainmix->do_save_mix(mainprogram->temppath + "tempdeck_lnch.deck", mainprogram->prevmodus, false);
					mainmix->open_mix(mainprogram->temppath + "tempdeck_lnch.deck", false);
				}
			}
		}
	}

	return ret;
}






// COPYING METHODS

void Mixer::copy_pbos(Layer *clay, Layer *lay) {
    // carry over pbos to new layer
    if (clay == lay) return;
    if (lay->filename == "") return;
    clay->nonewpbos = true;
    clay->pbo[0] = lay->pbo[0];
    clay->pbo[1] = lay->pbo[1];
    clay->pbo[2] = lay->pbo[2];
    clay->mapptr[0] = lay->mapptr[0];
    clay->mapptr[1] = lay->mapptr[1];
    clay->mapptr[2] = lay->mapptr[2];
    clay->remfr[0] = lay->remfr[0];
    clay->remfr[1] = lay->remfr[1];
    clay->remfr[2] = lay->remfr[2];
    // reminder : other endopenvars remove if lay->opened
    if (clay->vidopen) {
        std::unique_lock<std::mutex> olock(clay->endopenlock);
        clay->endopenvar.wait(olock, [&] { return clay->opened; });
        clay->opened = false;
        olock.unlock();
    }

    if (clay->type != ELEM_IMAGE) {
        if (clay->remfr[lay->pbofri]->width != clay->video_dec_ctx->width ||
            clay->remfr[lay->pbofri]->height != clay->video_dec_ctx->height ||
            clay->remfr[lay->pbofri]->bpp != clay->bpp) {
            clay->changeinit = 3;
            clay->initialized = true;
        } else clay->initialized = true;
    }
    else {
        clay->initialized = true;
    }
    clay->started = lay->started;
    clay->started2 = lay->started2;
    /*clay->databuf[0] = nullptr;
    clay->databuf[1] = nullptr;
    clay->databuf[2] = nullptr;*/
    clay->databufready = lay->databufready;
    clay->rgbframe[0] = lay->rgbframe[0];
    clay->rgbframe[1] = lay->rgbframe[1];
    clay->rgbframe[2] = lay->rgbframe[2];
    clay->syncobj[0] = lay->syncobj[0];
    clay->syncobj[1] = lay->syncobj[1];
    clay->syncobj[2] = lay->syncobj[2];
    clay->pbodi = lay->pbodi;
    clay->pboui = lay->pboui;
    clay->pbofri = lay->pbofri;

    clay->texture = lay->texture;    // for open_layerfile
    //clay->changeinit = 3;  // toggle start of 3 frame prerun in old resolution with already loaded mapptrs

    lay->nopbodel = true;  // so they dont get deleted
}

void Mixer::set_values(Layer *clay, Layer *lay, bool open) {
    clay->speed->value = lay->speed->value;
    clay->playbut->value = lay->playbut->value;
    clay->revbut->value = lay->revbut->value;
    clay->bouncebut->value = lay->bouncebut->value;
    clay->playkind = lay->playkind;
    clay->genmidibut->value = lay->genmidibut->value;
    clay->pos = lay->pos;
    clay->deck = lay->deck;
    clay->aspectratio = lay->aspectratio;
    clay->shiftx->value = lay->shiftx->value;
    clay->shifty->value = lay->shifty->value;
    clay->scale->value = lay->scale->value;
    clay->opacity->value = lay->opacity->value;
    clay->volume->value = lay->volume->value;
    clay->chtol->value = lay->chtol->value;
    clay->chdir->value = lay->chdir->value;
    clay->chinv->value = lay->chinv->value;
    if (open) {
        if (lay->type == ELEM_LIVE) {
            clay->set_live_base(lay->filename);
        } else if (lay->filename != "") {
            if (lay->type == ELEM_IMAGE) clay->open_image(lay->filename);
            else {
                clay->open_video(lay->frame, lay->filename, false);
                //std::unique_lock<std::mutex> lock(lay->endopenlock);
                //lay->endopenvar.wait(lock, [&] {return lay->opened; });
                //lay->opened = false;
                //lock.unlock();
            }
        }
    }
    clay->millif = lay->millif;
    clay->prevtime = lay->prevtime;
    clay->frame = lay->frame;
    clay->prevframe = lay->frame - 1;
    clay->startframe->value = lay->startframe->value;
    clay->endframe->value = lay->endframe->value;
    clay->numf = lay->numf;
    clay->clips.clear();
    for (Clip *clip : lay->clips) {
        clip->copy()->insert(clay, clay->clips.end());
    }
    clay->prevshelfdragelems = lay->prevshelfdragelems;
    clay->clonesetnr = lay->clonesetnr;
	if (clay->clonesetnr != -1) {
		bool dummy = false;
	}
}


void Mixer::copy_to_comp(bool deckA, bool deckB, bool comp) {
    if (this->loadinglays.size()) return;
    if (deckA && deckB) {
        mainmix->wipe[comp] = mainmix->wipe[!comp];
        mainmix->wipedir[comp] = mainmix->wipedir[!comp];
        mainmix->wipex[comp]->value = mainmix->wipex[!comp]->value;
        mainmix->wipey[comp]->value = mainmix->wipey[!comp]->value;
        if (comp) {
            mainmix->crossfadecomp->value = mainmix->crossfade->value;
        } else {
            mainmix->crossfade->value = mainmix->crossfadecomp->value;
        }
    }

    bool bupm = mainprogram->prevmodus;
    mainprogram->prevmodus = comp;
    if (comp) loopstation = lp;
    else loopstation = lpc;
    if (deckA) {
        mainmix->mousedeck = 0;
        mainmix->do_save_deck(mainprogram->temppath + "copytocompdeckA.deck", false, true);
    }
    if (deckB) {
        mainmix->mousedeck = 1;
        mainmix->do_save_deck(mainprogram->temppath + "copytocompdeckB.deck", false, true);
    }
    mainprogram->prevmodus = !comp;
    if (comp) loopstation = lpc;
    else loopstation = lp;
    if (deckA) {
        mainmix->mousedeck = 0;
        mainmix->open_deck(mainprogram->temppath + "copytocompdeckA.deck", true, true, true);
    }
    if (deckB) {
        mainmix->mousedeck = 1;
        mainmix->open_deck(mainprogram->temppath + "copytocompdeckB.deck", true, true, true);
    }
    mainprogram->prevmodus = bupm;
    if (bupm) loopstation = lp;
    else loopstation = lpc;

    std::vector<std::vector<Layer*>> *tempmap;
    for (int i = comp * 2; i < comp * 2 + 2; i++) {
        for (int i = 0; i < 4; i++) {
            int prevl;
            if (i == 0) {
                tempmap = &mainmix->swapmap[0];
                prevl = 2;
            } else if (i == 1) {
                tempmap = &mainmix->swapmap[1];
                prevl = 3;
            }
            if (i == 2) {
                tempmap = &mainmix->swapmap[2];
                prevl = 0;
            } else if (i == 3) {
                tempmap = &mainmix->swapmap[3];
                prevl = 1;
            }
            if (tempmap->size()) {
                for (std::vector<Layer *> lv: *tempmap) {
                    lv[1]->timeinit = mainmix->layers[prevl][lv[1]->pos]->timeinit;
                    lv[1]->prevtime = mainmix->layers[prevl][lv[1]->pos]->prevtime;
                }
            }
        }
    }
}






//

//   OPEN / SAVE  FUNCTIONALITY

//

			// STATE

void Mixer::new_state() {
	this->new_file(3, 1, true);
	//clear genmidis?
	mainprogram->shelves[0]->erase();
	mainprogram->shelves[1]->erase();
	lp->init();
	lpc->init();
	for (int i = 0; i < 3; i++) {
		glDeleteFramebuffers(1, &((MixNode*)mainprogram->nodesmain->mixnodes[0][i])->mixfbo);
		glDeleteTextures(1, &((MixNode*)mainprogram->nodesmain->mixnodes[0][i])->mixtex);
		((MixNode*)mainprogram->nodesmain->mixnodes[0][i])->mixfbo = -1;
		glDeleteFramebuffers(1, &((MixNode*)mainprogram->nodesmain->mixnodes[1][i])->mixfbo);
		glDeleteTextures(1, &((MixNode*)mainprogram->nodesmain->mixnodes[1][i])->mixtex);
		((MixNode*)mainprogram->nodesmain->mixnodes[1][i])->mixfbo = -1;
	}
	glDeleteFramebuffers(1, &mainprogram->bnodeend[0]->fbo);
	glDeleteTextures(1, &mainprogram->bnodeend[0]->fbotex);
	mainprogram->bnodeend[0]->fbo = -1;
	glDeleteFramebuffers(1, &mainprogram->bnodeend[1]->fbo);
	glDeleteTextures(1, &mainprogram->bnodeend[1]->fbotex);
	mainprogram->bnodeend[1]->fbo = -1;
}

void Mixer::open_state(std::string path) {
	std::string result = deconcat_files(path);
    if (mainprogram->openerr) return;
	bool concat = (result != "");
	std::ifstream rfile;
	if (concat) rfile.open(result);
	else rfile.open(path);
	std::string istring;
    safegetline(rfile, istring);
    if (istring != "EWOCvj STATEFILE") {
        mainprogram->openerr = true;
        return;
    }
	
	GLuint tex;
	tex = set_texes(mainprogram->fbotex[2], &mainprogram->frbuf[2], mainprogram->ow, mainprogram->oh);
	mainprogram->fbotex[2] = tex;
	tex = set_texes(mainprogram->fbotex[3], &mainprogram->frbuf[3], mainprogram->ow, mainprogram->oh);
	mainprogram->fbotex[3] = tex;
	if (mainprogram->bnodeend[0]->fbo == -1) {
		glGenTextures(1, &mainprogram->bnodeend[0]->fbotex);
 		glBindTexture(GL_TEXTURE_2D, mainprogram->bnodeend[0]->fbotex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glGenFramebuffers(1, &(mainprogram->bnodeend[0]->fbo));
		glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->bnodeend[0]->fbo);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mainprogram->bnodeend[0]->fbotex, 0);
	}
	if (mainprogram->bnodeend[1]->fbo == -1) {
		glGenTextures(1, &mainprogram->bnodeend[1]->fbotex);
 		glBindTexture(GL_TEXTURE_2D, mainprogram->bnodeend[1]->fbotex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glGenFramebuffers(1, &(mainprogram->bnodeend[1]->fbo));
		glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->bnodeend[1]->fbo);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mainprogram->bnodeend[1]->fbotex, 0);
	}
    glBindTexture(GL_TEXTURE_2D, mainprogram->bnodeend[0]->fbotex);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, mainprogram->ow3, mainprogram->oh3);
    glBindTexture(GL_TEXTURE_2D, mainprogram->bnodeend[1]->fbotex);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, mainprogram->ow, mainprogram->oh);
	for (int i = 0; i < mainprogram->nodesmain->mixnodes[0].size(); i++) {
		if (mainprogram->nodesmain->mixnodes[0][i]->mixfbo == -1) {
			glGenTextures(1, &mainprogram->nodesmain->mixnodes[0][i]->mixtex);
			glBindTexture(GL_TEXTURE_2D, mainprogram->nodesmain->mixnodes[0][i]->mixtex);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			glGenFramebuffers(1, &(mainprogram->nodesmain->mixnodes[0][i]->mixfbo));
			glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->nodesmain->mixnodes[0][i]->mixfbo);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mainprogram->nodesmain->mixnodes[0][i]->mixtex, 0);
		}
		glBindTexture(GL_TEXTURE_2D, mainprogram->nodesmain->mixnodes[0][i]->mixtex);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, mainprogram->ow3, mainprogram->oh3);
	}
	for (int i = 0; i < mainprogram->nodesmain->mixnodes[1].size(); i++) {
		if (mainprogram->nodesmain->mixnodes[1][i]->mixfbo == -1) {
			glGenTextures(1, &mainprogram->nodesmain->mixnodes[1][i]->mixtex);
			glBindTexture(GL_TEXTURE_2D, mainprogram->nodesmain->mixnodes[1][i]->mixtex);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			glGenFramebuffers(1, &(mainprogram->nodesmain->mixnodes[1][i]->mixfbo));
			glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->nodesmain->mixnodes[1][i]->mixfbo);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mainprogram->nodesmain->mixnodes[1][i]->mixtex, 0);
		}
		glBindTexture(GL_TEXTURE_2D, mainprogram->nodesmain->mixnodes[1][i]->mixtex);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, mainprogram->ow, mainprogram->oh);
	}
	//open_genmidis(remove_extension(path) + ".midi");

	int deckcnt = 4;
    int cs0, cs1;
	while (safegetline(rfile, istring)) {
		if (istring == "ENDOFFILE") break;
		Layer* bulay = nullptr;
		std::vector<Layer*> bulays;
        if (istring == "MAINSCENE0") {
            safegetline(rfile, istring);
            mainmix->currscene[0] = std::stoi(istring);
            safegetline(rfile, istring);
            mainmix->currscene[0] = std::stoi(istring);
        }
        if (istring == "MAINSCENE1") {
            safegetline(rfile, istring);
            mainmix->currscene[1] = std::stoi(istring);
            safegetline(rfile, istring);
            mainmix->currscene[1] = std::stoi(istring);
            bool bumodus = mainprogram->prevmodus;
            mainprogram->prevmodus = true;
            //if (exists(result + "_2.file")) {
                mainmix->open_mix(result + "_2.file", true);
            //}
            bulay = mainmix->currlay[!mainprogram->prevmodus];
            bulays = mainmix->currlays[!mainprogram->prevmodus];
            //if (exists(result + "_3.file")) {
                mainprogram->prevmodus = false;
                mainmix->open_mix(result + "_3.file", true);
            //}
            mainprogram->prevmodus = bumodus;
        }
        if (istring == "TOSCREENAMIDI0") {
            safegetline(rfile, istring);
            mainprogram->toscreenA->midi[0] = std::stoi(istring);
        }
        if (istring == "TOSCREENAMIDI1") {
            safegetline(rfile, istring);
            mainprogram->toscreenA->midi[1] = std::stoi(istring);
        }
        if (istring == "TOSCREENAMIDIPORT") {
            safegetline(rfile, istring);
            mainprogram->toscreenA->midiport = istring;
            mainprogram->toscreenA->register_midi();
        }
        if (istring == "TOSCREENBMIDI0") {
            safegetline(rfile, istring);
            mainprogram->toscreenB->midi[0] = std::stoi(istring);
        }
        if (istring == "TOSCREENBMIDI1") {
            safegetline(rfile, istring);
            mainprogram->toscreenB->midi[1] = std::stoi(istring);
        }
        if (istring == "TOSCREENBMIDIPORT") {
            safegetline(rfile, istring);
            mainprogram->toscreenB->midiport = istring;
            mainprogram->toscreenB->register_midi();
        }
        if (istring == "TOSCREENMMIDI0") {
            safegetline(rfile, istring);
            mainprogram->toscreenM->midi[0] = std::stoi(istring);
        }
        if (istring == "TOSCREENMMIDI1") {
            safegetline(rfile, istring);
            mainprogram->toscreenM->midi[1] = std::stoi(istring);
        }
        if (istring == "TOSCREENMMIDIPORT") {
            safegetline(rfile, istring);
            mainprogram->toscreenM->midiport = istring;
            mainprogram->toscreenM->register_midi();
        }
        if (istring == "TOSCENEA00MIDI0") {
            safegetline(rfile, istring);
            mainprogram->toscene[0][0][0]->midi[0] = std::stoi(istring);
        }
        if (istring == "TOSCENEA00MIDI1") {
            safegetline(rfile, istring);
            mainprogram->toscene[0][0][0]->midi[1] = std::stoi(istring);
        }
        if (istring == "TOSCENEA00MIDIPORT") {
            safegetline(rfile, istring);
            mainprogram->toscene[0][0][0]->midiport = istring;
            mainprogram->toscene[0][0][0]->register_midi();
        }
        if (istring == "TOSCENEA01MIDI0") {
            safegetline(rfile, istring);
            mainprogram->toscene[0][0][1]->midi[0] = std::stoi(istring);
        }
        if (istring == "TOSCENEA01MIDI1") {
            safegetline(rfile, istring);
            mainprogram->toscene[0][0][1]->midi[1] = std::stoi(istring);
        }
        if (istring == "TOSCENEA01MIDIPORT") {
            safegetline(rfile, istring);
            mainprogram->toscene[0][0][1]->midiport = istring;
            mainprogram->toscene[0][0][1]->register_midi();
        }
        if (istring == "TOSCENEA02MIDI0") {
            safegetline(rfile, istring);
            mainprogram->toscene[0][0][2]->midi[0] = std::stoi(istring);
        }
        if (istring == "TOSCENEA02MIDI1") {
            safegetline(rfile, istring);
            mainprogram->toscene[0][0][2]->midi[1] = std::stoi(istring);
        }
        if (istring == "TOSCENEA02MIDIPORT") {
            safegetline(rfile, istring);
            mainprogram->toscene[0][0][2]->midiport = istring;
            mainprogram->toscene[0][0][2]->register_midi();
        }
        if (istring == "TOSCENEA10MIDI0") {
            safegetline(rfile, istring);
            mainprogram->toscene[0][1][0]->midi[0] = std::stoi(istring);
        }
        if (istring == "TOSCENEA10MIDI1") {
            safegetline(rfile, istring);
            mainprogram->toscene[0][1][0]->midi[1] = std::stoi(istring);
        }
        if (istring == "TOSCENEA10MIDIPORT") {
            safegetline(rfile, istring);
            mainprogram->toscene[0][1][0]->midiport = istring;
            mainprogram->toscene[0][1][0]->register_midi();
        }
        if (istring == "TOSCENEA11MIDI0") {
            safegetline(rfile, istring);
            mainprogram->toscene[0][1][1]->midi[0] = std::stoi(istring);
        }
        if (istring == "TOSCENEA11MIDI1") {
            safegetline(rfile, istring);
            mainprogram->toscene[0][1][1]->midi[1] = std::stoi(istring);
        }
        if (istring == "TOSCENEA11MIDIPORT") {
            safegetline(rfile, istring);
            mainprogram->toscene[0][1][1]->midiport = istring;
            mainprogram->toscene[0][1][1]->register_midi();
        }
        if (istring == "TOSCENEA12MIDI0") {
            safegetline(rfile, istring);
            mainprogram->toscene[0][1][2]->midi[0] = std::stoi(istring);
        }
        if (istring == "TOSCENEA12MIDI1") {
            safegetline(rfile, istring);
            mainprogram->toscene[0][1][2]->midi[1] = std::stoi(istring);
        }
        if (istring == "TOSCENEA12MIDIPORT") {
            safegetline(rfile, istring);
            mainprogram->toscene[0][1][2]->midiport = istring;
            mainprogram->toscene[0][1][2]->register_midi();
        }
        if (istring == "TOSCENEB00MIDI0") {
            safegetline(rfile, istring);
            mainprogram->toscene[1][0][0]->midi[0] = std::stoi(istring);
        }
        if (istring == "TOSCENEB00MIDI1") {
            safegetline(rfile, istring);
            mainprogram->toscene[1][0][0]->midi[1] = std::stoi(istring);
        }
        if (istring == "TOSCENEB00MIDIPORT") {
            safegetline(rfile, istring);
            mainprogram->toscene[1][0][0]->midiport = istring;
            mainprogram->toscene[1][0][0]->register_midi();
        }
        if (istring == "TOSCENEB01MIDI0") {
            safegetline(rfile, istring);
            mainprogram->toscene[1][0][1]->midi[0] = std::stoi(istring);
        }
        if (istring == "TOSCENEB01MIDI1") {
            safegetline(rfile, istring);
            mainprogram->toscene[1][0][1]->midi[1] = std::stoi(istring);
        }
        if (istring == "TOSCENEB01MIDIPORT") {
            safegetline(rfile, istring);
            mainprogram->toscene[1][0][1]->midiport = istring;
            mainprogram->toscene[1][0][1]->register_midi();
        }
        if (istring == "TOSCENEB02MIDI0") {
            safegetline(rfile, istring);
            mainprogram->toscene[1][0][2]->midi[0] = std::stoi(istring);
        }
        if (istring == "TOSCENEB02MIDI1") {
            safegetline(rfile, istring);
            mainprogram->toscene[1][0][2]->midi[1] = std::stoi(istring);
        }
        if (istring == "TOSCENEB02MIDIPORT") {
            safegetline(rfile, istring);
            mainprogram->toscene[1][0][2]->midiport = istring;
            mainprogram->toscene[1][0][2]->register_midi();
        }
        if (istring == "TOSCENEB10MIDI0") {
            safegetline(rfile, istring);
            mainprogram->toscene[1][1][0]->midi[0] = std::stoi(istring);
        }
        if (istring == "TOSCENEB10MIDI1") {
            safegetline(rfile, istring);
            mainprogram->toscene[1][1][0]->midi[1] = std::stoi(istring);
        }
        if (istring == "TOSCENEB10MIDIPORT") {
            safegetline(rfile, istring);
            mainprogram->toscene[1][1][0]->midiport = istring;
            mainprogram->toscene[1][1][0]->register_midi();
        }
        if (istring == "TOSCENEB11MIDI0") {
            safegetline(rfile, istring);
            mainprogram->toscene[1][1][1]->midi[0] = std::stoi(istring);
        }
        if (istring == "TOSCENEB11MIDI1") {
            safegetline(rfile, istring);
            mainprogram->toscene[1][1][1]->midi[1] = std::stoi(istring);
        }
        if (istring == "TOSCENEB11MIDIPORT") {
            safegetline(rfile, istring);
            mainprogram->toscene[1][1][1]->midiport = istring;
            mainprogram->toscene[1][1][1]->register_midi();
        }
        if (istring == "TOSCENEB12MIDI0") {
            safegetline(rfile, istring);
            mainprogram->toscene[1][1][2]->midi[0] = std::stoi(istring);
        }
        if (istring == "TOSCENEB12MIDI1") {
            safegetline(rfile, istring);
            mainprogram->toscene[1][1][2]->midi[1] = std::stoi(istring);
        }
        if (istring == "TOSCENEB12MIDIPORT") {
            safegetline(rfile, istring);
            mainprogram->toscene[1][1][2]->midiport = istring;
            mainprogram->toscene[1][1][2]->register_midi();
        }
        if (istring == "BACKTOPREAMIDI0") {
            safegetline(rfile, istring);
            mainprogram->backtopreA->midi[0] = std::stoi(istring);
        }
        if (istring == "BACKTOPREAMIDI1") {
            safegetline(rfile, istring);
            mainprogram->backtopreA->midi[1] = std::stoi(istring);
        }
        if (istring == "BACKTOPREAMIDIPORT") {
            safegetline(rfile, istring);
            mainprogram->backtopreA->midiport = istring;
            mainprogram->backtopreA->register_midi();
        }
        if (istring == "BACKTOPREBMIDI0") {
            safegetline(rfile, istring);
            mainprogram->backtopreB->midi[0] = std::stoi(istring);
        }
        if (istring == "BACKTOPREBMIDI1") {
            safegetline(rfile, istring);
            mainprogram->backtopreB->midi[1] = std::stoi(istring);
        }
        if (istring == "BACKTOPREBMIDIPORT") {
            safegetline(rfile, istring);
            mainprogram->backtopreB->midiport = istring;
            mainprogram->backtopreB->register_midi();
        }
        if (istring == "BACKTOPREMMIDI0") {
            safegetline(rfile, istring);
            mainprogram->backtopreM->midi[0] = std::stoi(istring);
        }
        if (istring == "BACKTOPREMMIDI1") {
            safegetline(rfile, istring);
            mainprogram->backtopreM->midi[1] = std::stoi(istring);
        }
        if (istring == "BACKTOPREMMIDIPORT") {
            safegetline(rfile, istring);
            mainprogram->backtopreM->midiport = istring;
            mainprogram->backtopreM->register_midi();
        }
		if (istring == "MODUSBUTMIDI0") {
			safegetline(rfile, istring);
			mainprogram->modusbut->midi[0] = std::stoi(istring);
		}
		if (istring == "MODUSBUTMIDI1") {
			safegetline(rfile, istring);
			mainprogram->modusbut->midi[1] = std::stoi(istring);
		}
        if (istring == "MODUSBUTMIDIPORT") {
            safegetline(rfile, istring);
            mainprogram->modusbut->midiport = istring;
            mainprogram->modusbut->register_midi();
        }

		if (istring.substr(0, 4) == "DECK") {
			int scndeck = std::stoi(istring.substr(4, 1));
			int scnnum = std::stoi(istring.substr(5, 1));
			if (result != "") {
				std::filesystem::rename(result + "_" + std::to_string(deckcnt) + ".file", mainprogram->temppath + "tempdecksc_" + std::to_string(scndeck) + std::to_string(scnnum) + ".deck");
				deckcnt++;
				mainmix->scenes[scndeck][scnnum]->scnblayers.clear();
				while (safegetline(rfile, istring)) {
					if (istring == "FRAMES") {
						while (safegetline(rfile, istring)) {
							if (istring == "ENDFRAMES") {
								mainmix->scenes[scndeck][scnnum]->loaded = true;
								break;
							}
							mainmix->scenes[scndeck][scnnum]->nbframes.push_back(std::stof(istring));
						}
						break;
					}
				}
            }
		}
	}

    if (result != "") {
        //if (exists(result + "_0.file")) {
        mainprogram->shelves[0]->open(result + "_0.file");
        //
        //if (exists(result + "_1.file")) {
        mainprogram->shelves[1]->open(result + "_1.file");
        //}
    }

	rfile.close();
}

void Mixer::save_state(std::string path, bool autosave) {
	std::thread statesav(&Mixer::do_save_state, this, path, autosave);
	statesav.detach();
}

void Mixer::do_save_state(std::string path, bool autosave) {
	std::vector<std::string> filestoadd;
	std::string ext = path.substr(path.length() - 6, std::string::npos);
	std::string str;
	if (ext != ".state") str = path + ".state";
	else str = path;
	std::ofstream wfile;
	wfile.open(str);

	wfile << "EWOCvj STATEFILE\n";

	wfile << "MAINSCENE0\n";
	wfile << std::to_string(mainmix->currscene[0]);
	wfile << "\n";
	wfile << std::to_string(mainmix->currscene[0]);
	wfile << "\n";
	wfile << "MAINSCENE1\n";
	wfile << std::to_string(mainmix->currscene[1]);
	wfile << "\n";
	wfile << std::to_string(mainmix->currscene[1]);
	wfile << "\n";
	wfile << "PREVMODUS\n";
	wfile << std::to_string(mainprogram->prevmodus);
	wfile << "\n";
    wfile << "TOSCREENAMIDI0\n";
    wfile << std::to_string(mainprogram->toscreenA->midi[0]);
    wfile << "\n";
    wfile << "TOSCREENAMIDI1\n";
    wfile << std::to_string(mainprogram->toscreenA->midi[1]);
    wfile << "\n";
    wfile << "TOSCREENAMIDIPORT\n";
    wfile << mainprogram->toscreenA->midiport;
    wfile << "\n";
    wfile << "TOSCREENBMIDI0\n";
    wfile << std::to_string(mainprogram->toscreenB->midi[0]);
    wfile << "\n";
    wfile << "TOSCREENBMIDI1\n";
    wfile << std::to_string(mainprogram->toscreenB->midi[1]);
    wfile << "\n";
    wfile << "TOSCREENBMIDIPORT\n";
    wfile << mainprogram->toscreenB->midiport;
    wfile << "\n";
    wfile << "TOSCREENMMIDI0\n";
    wfile << std::to_string(mainprogram->toscreenM->midi[0]);
    wfile << "\n";
    wfile << "TOSCREENMMIDI1\n";
    wfile << std::to_string(mainprogram->toscreenM->midi[1]);
    wfile << "\n";
    wfile << "TOSCREENMMIDIPORT\n";
    wfile << mainprogram->toscreenM->midiport;
    wfile << "\n";
    wfile << "TOSCENEA00MIDI0\n";
    wfile << std::to_string(mainprogram->toscene[0][0][0]->midi[0]);
    wfile << "\n";
    wfile << "TOSCENEA00MIDI1\n";
    wfile << std::to_string(mainprogram->toscene[0][0][0]->midi[1]);
    wfile << "\n";
    wfile << "TOSCENEA00MIDIPORT\n";
    wfile << mainprogram->toscene[0][0][0]->midiport;
    wfile << "\n";
    wfile << "TOSCENEA01MIDI0\n";
    wfile << std::to_string(mainprogram->toscene[0][0][1]->midi[0]);
    wfile << "\n";
    wfile << "TOSCENEA01MIDI1\n";
    wfile << std::to_string(mainprogram->toscene[0][0][1]->midi[1]);
    wfile << "\n";
    wfile << "TOSCENEA01MIDIPORT\n";
    wfile << mainprogram->toscene[0][0][1]->midiport;
    wfile << "\n";
    wfile << "TOSCENEA02MIDI0\n";
    wfile << std::to_string(mainprogram->toscene[0][0][2]->midi[0]);
    wfile << "\n";
    wfile << "TOSCENEA02MIDI1\n";
    wfile << std::to_string(mainprogram->toscene[0][0][2]->midi[1]);
    wfile << "\n";
    wfile << "TOSCENEA02MIDIPORT\n";
    wfile << mainprogram->toscene[0][0][2]->midiport;
    wfile << "\n";
    wfile << "TOSCENEA10MIDI0\n";
    wfile << std::to_string(mainprogram->toscene[0][1][0]->midi[0]);
    wfile << "\n";
    wfile << "TOSCENEA10MIDI1\n";
    wfile << std::to_string(mainprogram->toscene[0][1][0]->midi[1]);
    wfile << "\n";
    wfile << "TOSCENEA10MIDIPORT\n";
    wfile << mainprogram->toscene[0][1][0]->midiport;
    wfile << "\n";
    wfile << "TOSCENEA11MIDI0\n";
    wfile << std::to_string(mainprogram->toscene[0][1][1]->midi[0]);
    wfile << "\n";
    wfile << "TOSCENEA11MIDI1\n";
    wfile << std::to_string(mainprogram->toscene[0][1][1]->midi[1]);
    wfile << "\n";
    wfile << "TOSCENEA11MIDIPORT\n";
    wfile << mainprogram->toscene[0][1][1]->midiport;
    wfile << "\n";
    wfile << "TOSCENEA12MIDI0\n";
    wfile << std::to_string(mainprogram->toscene[0][1][2]->midi[0]);
    wfile << "\n";
    wfile << "TOSCENEA12MIDI1\n";
    wfile << std::to_string(mainprogram->toscene[0][1][2]->midi[1]);
    wfile << "\n";
    wfile << "TOSCENEA12MIDIPORT\n";
    wfile << mainprogram->toscene[0][1][2]->midiport;
    wfile << "\n";
    wfile << "TOSCENEB00MIDI0\n";
    wfile << std::to_string(mainprogram->toscene[0][0][0]->midi[0]);
    wfile << "\n";
    wfile << "TOSCENEB00MIDI1\n";
    wfile << std::to_string(mainprogram->toscene[0][0][0]->midi[1]);
    wfile << "\n";
    wfile << "TOSCENEB00MIDIPORT\n";
    wfile << mainprogram->toscene[0][0][0]->midiport;
    wfile << "\n";
    wfile << "TOSCENEB01MIDI0\n";
    wfile << std::to_string(mainprogram->toscene[0][0][1]->midi[0]);
    wfile << "\n";
    wfile << "TOSCENEB01MIDI1\n";
    wfile << std::to_string(mainprogram->toscene[0][0][1]->midi[1]);
    wfile << "\n";
    wfile << "TOSCENEB01MIDIPORT\n";
    wfile << mainprogram->toscene[0][0][1]->midiport;
    wfile << "\n";
    wfile << "TOSCENEB02MIDI0\n";
    wfile << std::to_string(mainprogram->toscene[0][0][2]->midi[0]);
    wfile << "\n";
    wfile << "TOSCENEB02MIDI1\n";
    wfile << std::to_string(mainprogram->toscene[0][0][2]->midi[1]);
    wfile << "\n";
    wfile << "TOSCENEB02MIDIPORT\n";
    wfile << mainprogram->toscene[0][0][2]->midiport;
    wfile << "\n";
    wfile << "TOSCENEB10MIDI0\n";
    wfile << std::to_string(mainprogram->toscene[0][1][0]->midi[0]);
    wfile << "\n";
    wfile << "TOSCENEB10MIDI1\n";
    wfile << std::to_string(mainprogram->toscene[0][1][0]->midi[1]);
    wfile << "\n";
    wfile << "TOSCENEB10MIDIPORT\n";
    wfile << mainprogram->toscene[0][1][0]->midiport;
    wfile << "\n";
    wfile << "TOSCENEB11MIDI0\n";
    wfile << std::to_string(mainprogram->toscene[0][1][1]->midi[0]);
    wfile << "\n";
    wfile << "TOSCENEB11MIDI1\n";
    wfile << std::to_string(mainprogram->toscene[0][1][1]->midi[1]);
    wfile << "\n";
    wfile << "TOSCENEB11MIDIPORT\n";
    wfile << mainprogram->toscene[0][1][1]->midiport;
    wfile << "\n";
    wfile << "TOSCENEB12MIDI0\n";
    wfile << std::to_string(mainprogram->toscene[0][1][2]->midi[0]);
    wfile << "\n";
    wfile << "TOSCENEB12MIDI1\n";
    wfile << std::to_string(mainprogram->toscene[0][1][2]->midi[1]);
    wfile << "\n";
    wfile << "TOSCENEB12MIDIPORT\n";
    wfile << mainprogram->toscene[0][1][2]->midiport;
    wfile << "\n";
    wfile << "BACKTOPREAMIDI0\n";
    wfile << std::to_string(mainprogram->backtopreA->midi[0]);
    wfile << "\n";
    wfile << "BACKTOPREAMIDI1\n";
    wfile << std::to_string(mainprogram->backtopreA->midi[1]);
    wfile << "\n";
    wfile << "BACKTOPREAMIDIPORT\n";
    wfile << mainprogram->backtopreA->midiport;
    wfile << "\n";
    wfile << "BACKTOPREBMIDI0\n";
    wfile << std::to_string(mainprogram->backtopreB->midi[0]);
    wfile << "\n";
    wfile << "BACKTOPREBMIDI1\n";
    wfile << std::to_string(mainprogram->backtopreB->midi[1]);
    wfile << "\n";
    wfile << "BACKTOPREBMIDIPORT\n";
    wfile << mainprogram->backtopreB->midiport;
    wfile << "\n";
    wfile << "BACKTOPREMMIDI0\n";
    wfile << std::to_string(mainprogram->backtopreM->midi[0]);
    wfile << "\n";
    wfile << "BACKTOPREMMIDI1\n";
    wfile << std::to_string(mainprogram->backtopreM->midi[1]);
    wfile << "\n";
    wfile << "BACKTOPREMMIDIPORT\n";
    wfile << mainprogram->backtopreM->midiport;
    wfile << "\n";
	wfile << "MODUSBUTMIDI0\n";
	wfile << std::to_string(mainprogram->modusbut->midi[0]);
	wfile << "\n";
	wfile << "MODUSBUTMIDI1\n";
	wfile << std::to_string(mainprogram->modusbut->midi[1]);
	wfile << "\n";
    wfile << "MODUSBUTMIDIPORT\n";
    wfile << mainprogram->modusbut->midiport;
    wfile << "\n";

    mainprogram->shelves[0]->save(mainprogram->temppath + "tempA.shelf");
    filestoadd.push_back(mainprogram->temppath + "tempA.shelf");
    mainprogram->shelves[1]->save(mainprogram->temppath + "tempB.shelf");
    filestoadd.push_back(mainprogram->temppath + "tempB.shelf");
    std::string as = (autosave) ? "_autosave" : "";
    mainmix->do_save_mix(mainprogram->temppath + "temp.state1" + as, true, true);
    filestoadd.push_back(mainprogram->temppath + "temp.state1" + as + ".mix");
    mainmix->do_save_mix(mainprogram->temppath + "temp.state2" + as, false, true);
    filestoadd.push_back(mainprogram->temppath + "temp.state2" + as + ".mix");

    for (int j = 0; j < 2; j++) {
        for (int i = 0; i < 4; i++) {
            if (mainmix->scenes[j][mainmix->currscene[j]] == mainmix->scenes[j][i]) {
                continue;
            }
            wfile << "DECK" + std::to_string(j) + std::to_string(i) + "\n";
            wfile << "FRAMES\n";
            for (int k = 0; k < mainmix->scenes[j][i]->scnblayers.size(); k++) {
                wfile << std::to_string(mainmix->scenes[j][i]->scnblayers[k]->frame);
                wfile << "\n";
            }
            wfile << "ENDFRAMES\n";
            filestoadd.push_back(mainprogram->temppath + "tempdecksc_" + std::to_string(j) + std::to_string(i) + ".deck");
        }
    }
	//save_genmidis(remove_extension(path) + ".midi");

	wfile << "ENDOFFILE\n";
	wfile.close();

	std::vector<std::vector<std::string>> filestoadd2;
	filestoadd2.push_back(filestoadd);
    std::string ofpath = mainprogram->temppath + "tempconcatstate";
    std::thread concat(&Program::concat_files, mainprogram,  ofpath, str, filestoadd2);
    concat.detach();
}


			// MIX

void Mixer::open_mix(const std::string path, bool alive, bool loadevents) {
	std::string result = deconcat_files(path);
    if (mainprogram->openerr) {
        return;
    }
	bool concat = (result != "");
	std::ifstream rfile;
	if (concat) rfile.open(result);
	else rfile.open(path);
	std::string istring;
    safegetline(rfile, istring);
    if (istring != "EWOCvj MIXFILE") {
        mainprogram->openerr = true;
        return;
    }

    std::vector<Layer*> lvec0 = choose_layers(0);
    std::vector<Layer*> lvec1 = choose_layers(1);
	bool ret = this->set_prevshelfdragelem(lvec0[0]);
	if (!ret) alive = false;

    for (int i = 0; i < loopstation->elems.size(); i++) {
		loopstation->elems[i]->init();
		loopstation->elems[i]->eventlist.clear();
		loopstation->elems[i]->params.clear();
		loopstation->elems[i]->layers.clear();
	}
	loopstation->parmap.clear();
	loopstation->parelemmap.clear();
	loopstation->butmap.clear();
	loopstation->butelemmap.clear();
	loopstation->readelems.clear();
	loopstation->readelemnrs.clear();

	int clpos, cldeck;
    std::vector<int> cls1;
    std::vector<int> cls2;
	Layer *lay = nullptr;
	while (safegetline(rfile, istring)) {
        if (istring == "END") break;
        if (istring == "CURRLAY") {
            safegetline(rfile, istring);
            clpos = std::stoi(istring);
        }
        if (istring == "CURRLAYS0") {
            while (safegetline(rfile, istring)) {
                if (istring == "ENDCLS") break;
                cls1.push_back(std::stoi(istring));
            }
        }
        if (istring == "CURRLAYS1") {
            while (safegetline(rfile, istring)) {
                if (istring == "ENDCLS") break;
                cls2.push_back(std::stoi(istring));
            }
        }
    }

    this->new_file(2, alive, true, false);

    while (safegetline(rfile, istring)) {
        if (istring == "CROSSFADE") {
            safegetline(rfile, istring);
            mainmix->crossfade->value = std::stof(istring);
            if (mainprogram->prevmodus) {
                GLfloat cf = glGetUniformLocation(mainprogram->ShaderProgram, "cf");
                glUniform1f(cf, mainmix->crossfade->value);
            }
        }
        if (istring == "CROSSFADEEVENT") {
            Param *par = mainmix->crossfade;
            safegetline(rfile, istring);
            if (istring == "EVENTELEM") {
                mainmix->event_read(rfile, par, nullptr, lay);
            }
        }
        if (istring == "CROSSFADECOMP") {
            safegetline(rfile, istring);
            mainmix->crossfadecomp->value = std::stof(istring);
            if (!mainprogram->prevmodus) {
                GLfloat cf = glGetUniformLocation(mainprogram->ShaderProgram, "cf");
                glUniform1f(cf, mainmix->crossfadecomp->value);
            }
        }
        if (istring == "CROSSFADECOMPEVENT") {
            Param *par = mainmix->crossfadecomp;
            safegetline(rfile, istring);
            if (istring == "EVENTELEM") {
                mainmix->event_read(rfile, par, nullptr, lay);
            }
        }
        if (istring == "SCROLLPOSA") {
            safegetline(rfile, istring);
            mainmix->scenes[0][0]->scrollpos = std::stoi(istring);
            safegetline(rfile, istring);
            mainmix->scenes[0][1]->scrollpos = std::stoi(istring);
            safegetline(rfile, istring);
            mainmix->scenes[0][2]->scrollpos = std::stoi(istring);
            safegetline(rfile, istring);
            mainmix->scenes[0][3]->scrollpos = std::stoi(istring);
        }
        if (istring == "SCROLLPOSB") {
            safegetline(rfile, istring);
            mainmix->scenes[1][0]->scrollpos = std::stoi(istring);
            safegetline(rfile, istring);
            mainmix->scenes[1][1]->scrollpos = std::stoi(istring);
            safegetline(rfile, istring);
            mainmix->scenes[1][2]->scrollpos = std::stoi(istring);
            safegetline(rfile, istring);
            mainmix->scenes[1][3]->scrollpos = std::stoi(istring);
        }
        if (istring == "DECKSPEEDAEVENT") {
            Param *par = mainmix->deckspeed[!mainprogram->prevmodus][0];
            safegetline(rfile, istring);
            if (istring == "EVENTELEM") {
                mainmix->event_read(rfile, par, nullptr, nullptr);
            }
        }
        if (istring == "DECKSPEEDAMIDI0") {
            safegetline(rfile, istring);
            mainmix->deckspeed[!mainprogram->prevmodus][0]->midi[0] = std::stoi(istring);
        }
        if (istring == "DECKSPEEDAMIDI1") {
            safegetline(rfile, istring);
            mainmix->deckspeed[!mainprogram->prevmodus][0]->midi[1] = std::stoi(istring);
        }
        if (istring == "DECKSPEEDAMIDIPORT") {
            safegetline(rfile, istring);
            mainmix->deckspeed[!mainprogram->prevmodus][0]->midiport = istring;
            mainmix->deckspeed[!mainprogram->prevmodus][0]->register_midi();
        }
        if (istring == "DECKSPEEDB") {
            safegetline(rfile, istring);
            mainmix->deckspeed[!mainprogram->prevmodus][1]->value = std::stof(istring);
        }
        if (istring == "DECKSPEEDBEVENT") {
            Param *par = mainmix->deckspeed[!mainprogram->prevmodus][1];
            safegetline(rfile, istring);
            if (istring == "EVENTELEM") {
                mainmix->event_read(rfile, par, nullptr, nullptr);
            }
        }
        if (istring == "DECKSPEEDBMIDI0") {
            safegetline(rfile, istring);
            mainmix->deckspeed[!mainprogram->prevmodus][1]->midi[0] = std::stoi(istring);
        }
        if (istring == "DECKSPEEDBMIDI1") {
            safegetline(rfile, istring);
            mainmix->deckspeed[!mainprogram->prevmodus][1]->midi[1] = std::stoi(istring);
        }
        if (istring == "DECKSPEEDBMIDIPORT") {
            safegetline(rfile, istring);
            mainmix->deckspeed[!mainprogram->prevmodus][1]->midiport = istring;
            mainmix->deckspeed[!mainprogram->prevmodus][1]->register_midi();
        }
        if (istring == "WIPE") {
            safegetline(rfile, istring);
            mainmix->wipe[0] = std::stoi(istring);
        }
        if (istring == "WIPEDIR") {
            safegetline(rfile, istring);
            mainmix->wipedir[0] = std::stoi(istring);
        }
        if (istring == "WIPEX") {
            safegetline(rfile, istring);
            mainmix->wipex[0]->value = std::stof(istring);
        }
        if (istring == "WIPEXEVENT") {
            Param *par = mainmix->wipex[0];
            safegetline(rfile, istring);
            if (istring == "EVENTELEM") {
                mainmix->event_read(rfile, par, nullptr, lay);
            }
        }
        if (istring == "WIPEY") {
            safegetline(rfile, istring);
            mainmix->wipey[0]->value = std::stof(istring);
        }
        if (istring == "WIPEYEVENT") {
            Param *par = mainmix->wipey[0];
            safegetline(rfile, istring);
            if (istring == "EVENTELEM") {
                mainmix->event_read(rfile, par, nullptr, lay);
            }
        }
        if (istring == "WIPECOMP") {
            safegetline(rfile, istring);
            mainmix->wipe[1] = std::stoi(istring);
        }
        if (istring == "WIPEDIRCOMP") {
            safegetline(rfile, istring);
            mainmix->wipedir[1] = std::stoi(istring);
        }
        if (istring == "WIPEXCOMP") {
            safegetline(rfile, istring);
            mainmix->wipex[1]->value = std::stof(istring);
        }
        if (istring == "WIPEXCOMPEVENT") {
            Param *par = mainmix->wipex[1];
            safegetline(rfile, istring);
            if (istring == "EVENTELEM") {
                mainmix->event_read(rfile, par, nullptr, lay);
            }
        }
        if (istring == "WIPEYCOMP") {
            safegetline(rfile, istring);
            mainmix->wipey[1]->value = std::stof(istring);
        }
        if (istring == "WIPEYCOMPEVENT") {
            Param *par = mainmix->wipey[1];
            safegetline(rfile, istring);
            if (istring == "EVENTELEM") {
                mainmix->event_read(rfile, par, nullptr, lay);
            }
        }
        if (istring == "LPSTCURRELEM") {
            safegetline(rfile, istring);
            if (mainprogram->prevmodus) {
                lp->currelem = lp->elems[std::stoi(istring)];
            } else {
                lpc->currelem = lpc->elems[std::stoi(istring)];
            }
        }
        int deck;
        if (istring == "LAYERSA") {
            //std::vector<Layer *> &layers[0] = choose_layers(0);
            if (layers[0].empty()) {
                mainmix->add_layer(layers[0], 0);
            }
            mainmix->tempmapislayer = false;
            mainmix->currclonesize = mainmix->clonesets.size();
            mainprogram->filecount = 0;
            mainmix->read_layers(rfile, result, lvec0, 0, true, 2, concat, 1, loadevents, 1, 0);
            mainmix->reconnect_all(layers[0]);
            //std::vector<Layer *> &layers[1] = choose_layers(1);
            if (layers[1].empty()) {
                mainmix->add_layer(layers[1], 0);
            }
            //mainprogram->filecount = 0;
            mainmix->read_layers(rfile, result, lvec1, 1, true, 2, concat, 1, loadevents, 1, 0);
            mainmix->currclonesize = -1;
            mainmix->reconnect_all(layers[1]);
        }
    }


    mainprogram->filecount = 0;

    std::vector<Layer*> C_layers;
    if (mainprogram->prevmodus) {
        C_layers = mainmix->newlrs[0];
    }
    else {
        C_layers = mainmix->newlrs[2];
    }
    mainmix->currlay[!mainprogram->prevmodus] = C_layers[0];
    mainmix->currlays[!mainprogram->prevmodus].clear();
    mainmix->currlays[!mainprogram->prevmodus].push_back(C_layers[0]);
    for (int pos : cls1) {
        std::vector<Layer*> A_layers;
        if (mainprogram->prevmodus) {
            A_layers = mainmix->newlrs[0];
        }
        else {
            A_layers = mainmix->newlrs[2];
            //if (mainprogram->newproject) A_layers = mainmix->layers[2];
        }
    }

    for (int pos : cls2) {
        std::vector<Layer*> B_layers;
        if (mainprogram->prevmodus) {
            B_layers = mainmix->newlrs[1];
        }
        else {
            B_layers = mainmix->newlrs[3];
        }
        mainmix->currlay[!mainprogram->prevmodus] = B_layers[pos];
        mainmix->currlays[!mainprogram->prevmodus].clear();
        mainmix->currlays[!mainprogram->prevmodus].push_back(B_layers[pos]);
    }

    LoopStation *templpst = lpc;
    if (mainprogram->prevmodus) templpst = lp;
    for (LoopStationElement *elem : templpst->elems) {
        // make sure currelem isnt reset because of button toggling
        elem->loopbut->toggled();
        elem->playbut->toggled();
    }

     rfile.close();
}

void Mixer::save_mix(std::string path) {
	mainmix->do_save_mix(path, mainprogram->prevmodus, true);
	//std::thread mixsav(&Mixer::do_save_mix, this, path, mainprogram->prevmodus, true);
	//mixsav.detach();  reminder
}

void Mixer::do_save_mix(const std::string path, bool modus, bool save) {
    std::string ext = path.substr(path.length() - 4, std::string::npos);
	std::string str;
	if (ext != ".mix") str = path + ".mix";
	else str = path;
	std::ofstream wfile;
	wfile.open(str);
	wfile << "EWOCvj MIXFILE\n";

    wfile << "CURRLAY\n";
    wfile << std::to_string(mainmix->currlay[!modus]->pos);
    wfile << "\n";
    wfile << "CURRDECK\n";
	wfile << std::to_string(mainmix->currlay[!modus]->deck);
	wfile << "\n";
    wfile << "CURRLAYS0\n";
    for (int i = 0; i < mainmix->currlays[!modus].size(); i++) {
        if (mainmix->currlays[!modus][i]->deck == 0) {
            wfile << std::to_string(mainmix->currlays[!modus][i]->pos);
            wfile << "\n";
        }
    }
    wfile << "ENDCLS\n";
    wfile << "CURRLAYS1\n";
    for (int i = 0; i < mainmix->currlays[!modus].size(); i++) {
        if (mainmix->currlays[!modus][i]->deck == 1) {
            wfile << std::to_string(mainmix->currlays[!modus][i]->pos);
            wfile << "\n";
        }
    }
    wfile << "ENDCLS\n";
    wfile << "END\n";

    wfile << "CROSSFADE\n";
	wfile << std::to_string(mainmix->crossfade->value);
	wfile << "\n";
	wfile << "CROSSFADEEVENT\n";
	Param *par = mainmix->crossfade;
	mainmix->event_write(wfile, par, nullptr);
	wfile << "CROSSFADECOMP\n";
	wfile << std::to_string(mainmix->crossfadecomp->value);
	wfile << "\n";
	wfile << "CROSSFADECOMPEVENT\n";
	par = mainmix->crossfadecomp;
    mainmix->event_write(wfile, par, nullptr);
    wfile << "SCROLLPOSA\n";
    wfile << std::to_string( mainmix->scenes[0][0]->scrollpos);
    wfile << "\n";
    wfile << std::to_string( mainmix->scenes[0][1]->scrollpos);
    wfile << "\n";
    wfile << std::to_string( mainmix->scenes[0][2]->scrollpos);
    wfile << "\n";
    wfile << std::to_string( mainmix->scenes[0][3]->scrollpos);
    wfile << "\n";
    wfile << "SCROLLPOSB\n";
    wfile << std::to_string( mainmix->scenes[1][0]->scrollpos);
    wfile << "\n";
    wfile << std::to_string( mainmix->scenes[1][1]->scrollpos);
    wfile << "\n";
    wfile << std::to_string( mainmix->scenes[1][2]->scrollpos);
    wfile << "\n";
    wfile << std::to_string( mainmix->scenes[1][3]->scrollpos);
    wfile << "\n";
	wfile << "DECKSPEEDA\n";
	wfile << std::to_string(mainmix->deckspeed[!modus][0]->value);
	wfile << "\n";
	wfile << "DECKSPEEDAEVENT\n";
	par = mainmix->deckspeed[!modus][0];
	mainmix->event_write(wfile, par, nullptr);
	wfile << "DECKSPEEDAMIDI0\n";
	wfile << std::to_string(mainmix->deckspeed[!modus][0]->midi[0]);
	wfile << "\n";
	wfile << "DECKSPEEDAMIDI1\n";
	wfile << std::to_string(mainmix->deckspeed[!modus][0]->midi[1]);
	wfile << "\n";
	wfile << "DECKSPEEDAMIDIPORT\n";
	wfile << mainmix->deckspeed[!modus][0]->midiport;
	wfile << "\n";
	wfile << "DECKSPEEDB\n";
	wfile << std::to_string(mainmix->deckspeed[!modus][1]->value);
	wfile << "\n";
	wfile << "DECKSPEEDBEVENT\n";
	par = mainmix->deckspeed[!modus][1];
	mainmix->event_write(wfile, par, nullptr);
	wfile << "DECKSPEEDBMIDI0\n";
	wfile << std::to_string(mainmix->deckspeed[!modus][1]->midi[0]);
	wfile << "\n";
	wfile << "DECKSPEEDBMIDI1\n";
	wfile << std::to_string(mainmix->deckspeed[!modus][1]->midi[1]);
	wfile << "\n";
	wfile << "DECKSPEEDBMIDIPORT\n";
	wfile << mainmix->deckspeed[!modus][1]->midiport;
	wfile << "\n";
	wfile << "WIPE\n";
	wfile << std::to_string(mainmix->wipe[0]);
	wfile << "\n";
	wfile << "WIPEDIR\n";
	wfile << std::to_string(mainmix->wipedir[0]);
	wfile << "\n";
	wfile << "WIPEX\n";
	wfile << std::to_string(mainmix->wipex[0]->value);
	wfile << "\n";
    wfile << "WIPEXEVENT\n";
    mainmix->event_write(wfile, mainmix->wipex[0], nullptr);
	wfile << "WIPEY\n";
	wfile << std::to_string(mainmix->wipey[0]->value);
	wfile << "\n";
    wfile << "WIPEYEVENT\n";
    mainmix->event_write(wfile, mainmix->wipey[0], nullptr);
	wfile << "WIPECOMP\n";
	wfile << std::to_string(mainmix->wipe[1]);
	wfile << "\n";
	wfile << "WIPEDIRCOMP\n";
	wfile << std::to_string(mainmix->wipedir[1]);
	wfile << "\n";
	wfile << "WIPEXCOMP\n";
	wfile << std::to_string(mainmix->wipex[1]->value);
	wfile << "\n";
    wfile << "WIPEXCOMPEVENT\n";
    mainmix->event_write(wfile, mainmix->wipex[1], nullptr);
	wfile << "WIPEYCOMP\n";
	wfile << std::to_string(mainmix->wipey[1]->value);
	wfile << "\n";
    wfile << "WIPEYCOMPEVENT\n";
    mainmix->event_write(wfile, mainmix->wipey[1], nullptr);
    wfile << "LPSTCURRELEM\n";
    if (modus) {
        wfile << std::to_string(lp->currelem->pos);
    }
    else {
        wfile << std::to_string(lpc->currelem->pos);
    }
    wfile << "\n";

	std::vector<std::vector<std::string>> jpegpaths;
	wfile << "LAYERSA\n";
	std::vector<Layer*> lvec;
	if (modus) {
        lvec = mainmix->layers[0];
    }
	else {
        lvec = mainmix->layers[2];
    }
	for (int i = 0; i < lvec.size(); i++) {
		Layer* lay = lvec[i];
		jpegpaths.push_back(mainmix->write_layer(lay, wfile, true, save));
	}

	wfile << "LAYERSB\n";
	if (modus) lvec = mainmix->layers[1];
	else lvec = mainmix->layers[3];
	for (int i = 0; i < lvec.size(); i++) {
		Layer* lay = lvec[i];
		jpegpaths.push_back(mainmix->write_layer(lay, wfile, true, save));
	}

	wfile << "ENDOFFILE\n";
	wfile.close();
	
	std::vector<MixNode*> mns = mainprogram->nodesmain->mixnodes[1];
	if (modus) mns = mainprogram->nodesmain->mixnodes[0];
	if (save && ((MixNode*)(mns[2]))->mixtex != -1) {
		if (mns.size()) {
			std::vector<std::string> jpegpaths2;
			GLuint tex = ((MixNode*)(mns[2]))->mixtex;
            this->minitex = copy_tex(tex, 192, 108);
			save_thumb(mainprogram->temppath + "mix.jpg", this->minitex);
			jpegpaths2.push_back(mainprogram->temppath + "mix.jpg");
			jpegpaths.push_back(jpegpaths2);
		}
	}

	std::string tcpath = find_unused_filename("tempconcat_" + std::to_string(modus), mainprogram->temppath, "");
    std::thread concat = std::thread(&Program::concat_files, mainprogram, tcpath, str, jpegpaths);
    concat.detach();
}


			// DECK

void Mixer::open_deck(const std::string path, bool alive, bool loadevents, bool copycomp) {
		
	std::string result = deconcat_files(path);
	if (mainprogram->openerr) return;
	bool concat = (result != "");
	std::ifstream rfile;
	if (concat) rfile.open(result);
	else rfile.open(path);
	std::string istring;
    safegetline(rfile, istring);
    if (istring != "EWOCvj DECKFILE") {
        mainprogram->openerr = true;  // reminder : a requester here
        return;
    }

	std::vector<Layer*>& lvec = choose_layers(mainmix->mousedeck);
	if (lvec.size()) {
		bool ret = this->set_prevshelfdragelem(lvec[0]);
		if (!ret) alive = false;
	}
	this->new_file(mainmix->mousedeck, alive, false, false);

	for (int i = 0; i < loopstation->elems.size(); i++) {
		bool alsootherdeck = false;
		std::unordered_set<Layer*>::iterator it;
		for (it = loopstation->elems[i]->layers.begin(); it != loopstation->elems[i]->layers.end(); it++) {
			Layer* lay = *it;
			if (lay->deck == !mainmix->mousedeck) {
				alsootherdeck = true;
				break;
			}
		}
        std::unordered_set<Param*>::const_iterator got = loopstation->elems[i]->params.find(mainmix->crossfade);
        if (got != loopstation->elems[i]->params.end()) alsootherdeck = true;
        got = loopstation->elems[i]->params.find(mainmix->crossfadecomp);
        if (got != loopstation->elems[i]->params.end()) alsootherdeck = true;
        got = loopstation->elems[i]->params.find(mainmix->wipex[0]);
        if (got != loopstation->elems[i]->params.end()) alsootherdeck = true;
        got = loopstation->elems[i]->params.find(mainmix->wipey[0]);
        if (got != loopstation->elems[i]->params.end()) alsootherdeck = true;
        got = loopstation->elems[i]->params.find(mainmix->wipex[1]);
        if (got != loopstation->elems[i]->params.end()) alsootherdeck = true;
        got = loopstation->elems[i]->params.find(mainmix->wipey[1]);
        if (got != loopstation->elems[i]->params.end()) alsootherdeck = true;
		if (!alsootherdeck && loadevents) {
			std::unordered_set<Param*>::iterator it1;
			for (it1 = loopstation->elems[i]->params.begin(); it1 != loopstation->elems[i]->params.end(); it1++) {
				loopstation->parelemmap.erase(*it1);
				loopstation->parmap.erase(*it1);
			}
			std::unordered_set<Button*>::iterator it2;
			for (it2 = loopstation->elems[i]->buttons.begin(); it2 != loopstation->elems[i]->buttons.end(); it2++) {
				loopstation->butelemmap.erase(*it2);
				loopstation->butmap.erase(*it2);
			}
			loopstation->elems[i]->erase_elem();
		}
	}

    std::vector<Layer*> &layers = choose_layers(mainmix->mousedeck);
    while (safegetline(rfile, istring)) {
        if (istring == "SCROLLPOS") {
            safegetline(rfile, istring);
            mainmix->scenes[mainmix->mousedeck][0]->scrollpos = std::stoi(istring);
            safegetline(rfile, istring);
            mainmix->scenes[mainmix->mousedeck][1]->scrollpos = std::stoi(istring);
            safegetline(rfile, istring);
            mainmix->scenes[mainmix->mousedeck][2]->scrollpos = std::stoi(istring);
            safegetline(rfile, istring);
            mainmix->scenes[mainmix->mousedeck][3]->scrollpos = std::stoi(istring);
        }
        else if (istring == "DECKSPEED") {
            safegetline(rfile, istring);
            mainmix->deckspeed[!mainprogram->prevmodus][mainmix->mousedeck]->value = std::stof(istring);
        }
        else if (istring == "DECKSPEEDEVENT") {
            Param* par = mainmix->deckspeed[!mainprogram->prevmodus][mainmix->mousedeck];
            safegetline(rfile, istring);
            if (istring == "EVENTELEM") {
                mainmix->event_read(rfile, par, nullptr, nullptr);
            }
        }
        else if (istring == "DECKSPEEDMIDI0") {
            safegetline(rfile, istring);
            mainmix->deckspeed[!mainprogram->prevmodus][mainmix->mousedeck]->midi[0] = std::stoi(istring);
        }
        else if (istring == "DECKSPEEDMIDI1") {
            safegetline(rfile, istring);
            mainmix->deckspeed[!mainprogram->prevmodus][mainmix->mousedeck]->midi[1] = std::stoi(istring);
        }
        else if (istring == "DECKSPEEDMIDIPORT") {
            safegetline(rfile, istring);
            mainmix->deckspeed[!mainprogram->prevmodus][mainmix->mousedeck]->midiport = istring;
        }
        else if (istring == "LAYERS") {
            loopstation->readelems.clear();
            loopstation->readelemnrs.clear();

            mainprogram->filecount = 0;
            mainmix->copycomp_busy = true;
            mainmix->tempmapislayer = false;
            mainmix->currclonesize = mainmix->clonesets.size();
            mainmix->read_layers(rfile, result, layers, mainmix->mousedeck, true, 0, 1, concat, 1, loadevents, 0);
            mainmix->currclonesize = -1;
            mainmix->copycomp_busy = false;

            break;
        }
    }

    // if mousedeck == 2 then its a background load for bin entries
    std::vector<std::vector<Layer*>> tempmap;
    /*for (int i = 0; i < 4; i++) {
        tempmap = mainmix->swapmap[i];
	    std::map<int, int> map;
        for (std::vector<Layer*> lv : tempmap) {
            if (lv[1]->clonesetnr != -1) {
                if (map.count(lv[1]->clonesetnr) == 0) {
                    std::unordered_set<Layer *> *uset = new std::unordered_set<Layer *>;;
                    mainmix->clonesets[lv[1]->clonesetnr] = uset;
                    map[lv[1]->clonesetnr] = mainmix->clonesets.size() - 1;
                    lv[1]->clonesetnr = mainmix->clonesets.size() - 1;
                } else lv[1]->clonesetnr = map[lv[1]->clonesetnr];
                mainmix->clonesets[lv[1]->clonesetnr]->emplace(lv[1]);
            }
        }
	}*/

    mainprogram->filecount = 0;

    if (!copycomp) {
        for (int i = 0; i < 4; i++) {
            tempmap = mainmix->swapmap[i];
            for (std::vector<Layer *> lv: tempmap) {
                if (!lv[1]) continue;
                int sz = mainmix->currlays[!mainprogram->prevmodus].size();
                for (int i = sz - 1; i >= 0; i--) {
                    if (mainmix->currlays[!mainprogram->prevmodus][i]->deck == mainmix->mousedeck) {
                        if (lv[1]->pos == mainmix->currlays[!mainprogram->prevmodus][i]->pos) {
                            mainmix->currlays[!mainprogram->prevmodus].erase(
                                    mainmix->currlays[!mainprogram->prevmodus].begin() + i);
                        }
                    }
                }
                if (mainmix->currlay[!mainprogram->prevmodus]) {
                    if (mainmix->currlay[!mainprogram->prevmodus]->deck == mainmix->mousedeck) {
                        if (lv[1]->pos == mainmix->currlay[!mainprogram->prevmodus]->pos) {
                            mainmix->currlay[!mainprogram->prevmodus] = lv[1];
                            mainmix->currlays[!mainprogram->prevmodus].push_back(lv[1]);
                        }
                    }
                }
                if (!mainmix->currlay[!mainprogram->prevmodus]) {
                    mainmix->currlay[!mainprogram->prevmodus] = lv[1];
                    mainmix->currlays[!mainprogram->prevmodus].push_back(lv[1]);
                }
            }
        }
    }
    else {
        for (int i = 0; i < 4; i++) {
            tempmap = mainmix->swapmap[i];
            for (std::vector<Layer *> lv: tempmap) {
                bool clear = true;
                int sz = mainmix->currlays[!lv[1]->comp].size();
                for (int j = sz - 1; j >= 0; j--) {
                    if (mainmix->currlays[!lv[1]->comp][j]->deck == mainmix->mousedeck) {
                        if (lv[1]->pos == mainmix->currlays[!lv[1]->comp][j]->pos) {
                            if (clear) {
                                mainmix->currlays[lv[1]->comp].clear();
                            }
                            mainmix->currlays[lv[1]->comp].push_back(
                                    lv[1]);
                            clear = false;
                        }
                    }
                }
                for (int j = 0; j < mainmix->currlays[lv[1]->comp].size(); j++) {
                    Layer *clay = mainmix->currlays[lv[1]->comp][j];
                    if (clay->comp == mainprogram->prevmodus) {
                        mainmix->currlays[lv[1]->comp].erase(mainmix->currlays[lv[1]->comp].begin() + j);
                    }
                }
                if (mainmix->currlay[!lv[1]->comp]) {
                    if (mainmix->currlay[!lv[1]->comp]->deck == mainmix->mousedeck) {
                        if (lv[1]->pos == mainmix->currlay[!lv[1]->comp]->pos) {
                            mainmix->currlay[lv[1]->comp] = lv[1];
                        }
                    }
                }
            }
        }
    }

    LoopStation *templpst = lpc;
    if (mainprogram->prevmodus) templpst = lp;
    for (LoopStationElement *elem : templpst->elems) {
        // make sure interimtime isnt reset because of button toggling
        elem->loopbut->toggled();
        elem->playbut->toggled();
    }

    if (layers.empty()) {
        mainmix->add_layer(layers, 0);
    }

    rfile.close();

}

void Mixer::save_deck(const std::string path) {
	mainmix->do_save_deck(path, true, true);
	//std::thread decksav(&Mixer::do_save_deck, this, path, true, true);
	//decksav.detach();  // reminder
}

void Mixer::do_save_deck(const std::string path, bool save, bool doclips) {
	std::string ext = path.substr(path.length() - 5, std::string::npos);
	std::string str;
	if (ext != ".deck") str = path + ".deck";
	else str = path;
	std::ofstream wfile;
	wfile.open(str);
	wfile << "EWOCvj DECKFILE\n";

    wfile << "SCROLLPOS\n";
    wfile << std::to_string( mainmix->scenes[mainmix->mousedeck][0]->scrollpos);
    wfile << "\n";
    wfile << std::to_string( mainmix->scenes[mainmix->mousedeck][1]->scrollpos);
    wfile << "\n";
    wfile << std::to_string( mainmix->scenes[mainmix->mousedeck][2]->scrollpos);
    wfile << "\n";
    wfile << std::to_string( mainmix->scenes[mainmix->mousedeck][3]->scrollpos);
    wfile << "\n";
    wfile << "DECKSPEED\n";
    wfile << std::to_string(mainmix->deckspeed[!mainprogram->prevmodus][mainmix->mousedeck]->value);
    wfile << "\n";
	wfile << "DECKSPEEDEVENT\n";
	Param* par = mainmix->deckspeed[!mainprogram->prevmodus][mainmix->mousedeck];
	mainmix->event_write(wfile, par, nullptr);
	wfile << "DECKSPEEDMIDI0\n";
	wfile << std::to_string(mainmix->deckspeed[!mainprogram->prevmodus][mainmix->mousedeck]->midi[0]);
	wfile << "\n";
	wfile << "DECKSPEEDMIDI1\n";
	wfile << std::to_string(mainmix->deckspeed[!mainprogram->prevmodus][mainmix->mousedeck]->midi[1]);
	wfile << "\n";
	wfile << "DECKSPEEDMIDIPORT\n";
	wfile << mainmix->deckspeed[!mainprogram->prevmodus][mainmix->mousedeck]->midiport;
	wfile << "\n";

    wfile << "LAYERS\n";
	std::vector<std::vector<std::string>> jpegpaths;
	std::vector<Layer*>& lvec = choose_layers(mainmix->mousedeck);
	for (int i = 0; i < lvec.size(); i++) {
		Layer* lay = lvec[i];
		jpegpaths.push_back(mainmix->write_layer(lay, wfile, doclips, true));
	}
	if (save) {
		std::vector<MixNode*>& mns = mainprogram->nodesmain->mixnodes[1];
		if (mainprogram->prevmodus) mns = mainprogram->nodesmain->mixnodes[0];
		if (mns.size()) {
			std::vector<std::string> jpegpaths2;
			GLuint tex = ((MixNode*)(mns[mainmix->mousedeck]))->mixtex;
			save_thumb(mainprogram->temppath + "deck.jpg", tex);
			jpegpaths2.push_back(mainprogram->temppath + "deck.jpg");
			jpegpaths.push_back(jpegpaths2);
		}
	}

	wfile << "ENDOFFILE\n";
	wfile.close();

    std::thread concat = std::thread(&Program::concat_files, mainprogram, mainprogram->temppath + "tempconcatdeck", str, jpegpaths);
    concat.detach();
}


			// LAYER FILE

Layer* Layer::open_video(float frame, const std::string filename, int reset, bool dontdeleffs) {
    // do configuration and thread launch for opening a video into a layer
    this->databufready = false;
    this->vidopen = true;

    if (!this->dummy) {
        bool ret = mainmix->set_prevshelfdragelem(this);
        if (!ret && !this->encodeload) {
            Layer *lay = this->transfer();
            lay->prevshelfdragelems.pop_back();
            lay->open_video(0, filename, true);
            return lay;
        }
    }

    if (this->boundimage != -1) {
        glDeleteTextures(1, &this->boundimage);
        this->boundimage = -1;
    }
    this->audioplaying = false;
    if (!this->keepeffbut->value && !dontdeleffs && !this->dummy && this->filename != "") {
        std::vector<Layer*> &lvec = choose_layers(this->deck);
        mainmix->reconnect_all(lvec);
        Node *out = this->lasteffnode[0]->out[0];
        // remove effects if keep effects isnt on
        while (!this->effects[0].empty()) {
            // reminder : code duplication
            mainprogram->deleffects.push_back(this->effects[0].back());
            this->effects[0].pop_back();
        }
        this->lasteffnode[0] = this->node;
        if (this->pos == 0) {
            this->lasteffnode[1] = this->lasteffnode[0];
        }
        mainprogram->nodesmain->currpage->connect_nodes(this->lasteffnode[0], out);
    }
    if (!this->lockspeed) {
        reset_par(this->speed, 1.0f);
    }
    if (!this->lockzoompan) {
        reset_par(this->shiftx, 0.0f);
        reset_par(this->shifty, 0.0f);
        reset_par(this->scale, 1.0f);
    }
    if (this->effects[0].size() == 0) this->type = ELEM_FILE;
    else this->type = ELEM_LAYER;
    if (this->filename == "") this->newload = true;
    else this->newload = false;
    this->filename = filename;
    if (frame >= 0.0f) this->frame = frame;
    this->prevframe = this->frame - 1;
    this->reset = reset;
    this->skip = false;
    this->ifmt = nullptr;
    this->remfr[0]->newdata = false;
    this->remfr[1]->newdata = false;
    this->remfr[2]->newdata = false;
    this->decresult->width = 0;
    this->decresult->compression = 0;
    if (mainprogram->autoplay && this->revbut->value == 0 && this->bouncebut->value == 0) {
        this->playbut->value = 1;
    }
    this->ready = true;
    this->startdecode.notify_all();

    return this;
}

bool Layer::thread_vidopen() {
    if (this->video_dec_ctx) avcodec_free_context(&this->video_dec_ctx);
    if (this->video) avformat_close_input(&this->video);
    if (this->videoseek) avformat_close_input(&this->videoseek);
    this->video = nullptr;
    this->video_dec_ctx = nullptr;
    this->liveinput = nullptr;

    if (!this->skip) {
        bool foundnew = false;
        ptrdiff_t pos = std::find(mainprogram->busylayers.begin(), mainprogram->busylayers.end(), this) - mainprogram->busylayers.begin();
        if (pos < mainprogram->busylayers.size()) {
            foundnew = this->find_new_live_base(pos);
            if (!foundnew && this->filename != mainprogram->busylist[pos]) {
                avformat_close_input(&mainprogram->busylayers[pos]->video);
                mainprogram->busylayers.erase(mainprogram->busylayers.begin() + pos);
                mainprogram->busylist.erase(mainprogram->busylist.begin() + pos);
            }
        }
        pos = std::find(mainprogram->mimiclayers.begin(), mainprogram->mimiclayers.end(), this) - mainprogram->mimiclayers.begin();
        if (pos < mainprogram->mimiclayers.size()) {
            mainprogram->mimiclayers.erase(mainprogram->mimiclayers.begin() + pos);
        }
    }

    //av_opt_set_int(this->video, "probesize2", INT_MAX, 0);
    this->video = avformat_alloc_context();
    if (!this->ifmt) this->video->flags |= AVFMT_FLAG_NONBLOCK;
    if (this->ifmt) {
        this->type = ELEM_LIVE;
    }
    int r = avformat_open_input(&(this->video), this->filename.c_str(), this->ifmt, nullptr);
    printf("loading... %s\n", this->filename.c_str());
    if (r < 0) {
        this->filename = "";
        mainmix->addlay = false;
        mainprogram->openerr = true;
        printf("%s\n", "Couldnt open video");
        return 0;
    }

    //av_opt_set_int(this->video, "max_analyze_duration2", 100000000, 0);
    if (avformat_find_stream_info(this->video, nullptr) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        this->filename = "";
        mainmix->addlay = false;
        mainprogram->openerr = true;
        return 0;
    }
    //this->video->max_picture_buffer = 20000000;

    if (find_stream_index(&(this->video_stream_idx), this->video, AVMEDIA_TYPE_VIDEO) >= 0) {
        this->video_stream = this->video->streams[this->video_stream_idx];
        const AVCodec* dec = avcodec_find_decoder(this->video_stream->codecpar->codec_id);
        this->vidformat = this->video_stream->codecpar->codec_id;
        this->video_dec_ctx = avcodec_alloc_context3(dec);
        if (this->video_stream->codecpar->codec_id == AV_CODEC_ID_MPEG2VIDEO || this->video_stream->codecpar->codec_id == AV_CODEC_ID_H264 || this->video_stream->codecpar->codec_id == AV_CODEC_ID_H264) {
            //this->video_dec_ctx->ticks_per_frame = 2;
        }
        avcodec_parameters_to_context(this->video_dec_ctx, this->video_stream->codecpar);
        avcodec_open2(this->video_dec_ctx, dec, nullptr);
        this->bpp = 4;
        if (this->vidformat == 188 || this->vidformat == 187) {
            if (oldvidformat != -1) {
                if (this->oldvidformat != 188 && this->oldvidformat != 187) {
                    // hap cpu change needs new texstorage
                    this->initialized = false;
                }
            }
            this->numf = this->video_stream->nb_frames;
            if (this->numf == 0) {
                this->numf = (double)this->video->duration * (double)this->video_stream->avg_frame_rate.num / (double)this->video_stream->avg_frame_rate.den / (double)1000000.0f;
                this->video_duration = this->video->duration / (1000000.0f * this->video_stream->time_base.num / this->video_stream->time_base.den);
            }
            else this->video_duration = this->video_stream->duration;
            float tbperframe = (float)this->video_stream->duration / (float)this->numf;
            this->millif = tbperframe * (((float)this->video_stream->time_base.num * 1000.0) / (float)this->video_stream->time_base.den);

            if (!this->framesloaded) {
                this->startframe->value = 0;
                this->endframe->value = this->numf;
            }
            else {
                this->framesloaded = false;
            }
            if (0) { // this->reset?
                this->frame = 0.0f;
            }
            if (this->decframe) {
                //av_frame_free(&this->rgbframe);
                //av_frame_free(&this->decframe);
                if (this->decframe->data[0] != nullptr) {
                    av_freep(&this->decframe->data[0]);
                }
                sws_freeContext(this->sws_ctx);
                //avcodec_free_context(&this->video_dec_ctx);
                av_free(this->decframe);
            }
            this->decframe = av_frame_alloc();

            std::mutex flock;
            flock.lock();
            if (this->databuf[0]) {
                free(this->databuf[0]);
                free(this->databuf[1]);
                free(this->databuf[2]);
            }
            this->databuf[0] = nullptr;
            this->databuf[1] = nullptr;
            this->databuf[2] = nullptr;
            flock.unlock();

            return 1;
        }
        else if (this->type != ELEM_LIVE) {
            this->videoseek = avformat_alloc_context();
            this->videoseek->flags |= AVFMT_FLAG_NONBLOCK;
            avformat_open_input(&(this->videoseek), this->filename.c_str(), this->ifmt, nullptr);
            avformat_find_stream_info(this->videoseek, nullptr);
            if (find_stream_index(&(this->videoseek_stream_idx), this->videoseek, AVMEDIA_TYPE_VIDEO) >= 0) {
                this->videoseek_stream = this->videoseek->streams[this->videoseek_stream_idx];
            }
        }
        if (oldvidformat != -1) {
            if (this->oldvidformat == 188 || this->oldvidformat == 187) {
                // hap cpu change needs new texstorage
                this->initialized = false;
            }
        }
    }
    else {
        printf("Error2\n");
        this->filename = "";
        mainmix->addlay = false;
        mainprogram->openerr = true;
        return 0;
    }

    for (int k = 0; k < 3; k++) {
        if (this->rgbframe[k]) {
            //avcodec_close(this->video_dec_ctx);
            //avcodec_close(this->audio_dec_ctx);
            av_freep(&this->rgbframe[k]->data[0]);
            av_frame_free(&this->rgbframe[k]);
        }
    }
    if (this->decframe) {
        //avcodec_close(this->video_dec_ctx);
        //avcodec_close(this->audio_dec_ctx);
        av_frame_free(&this->decframe);
    }

    if (this->type != ELEM_LIVE) {
        this->numf = this->video_stream->nb_frames;
        if (this->numf == 0) {
            this->numf = (double)this->video->duration * (double)this->video_stream->avg_frame_rate.num / (double)this->video_stream->avg_frame_rate.den / (double)1000000.0f;
            this->video_duration = this->video->duration / (1000000.0f * this->video_stream->time_base.num / this->video_stream->time_base.den);
        }
        else this->video_duration = this->video_stream->duration;
        if (this->reset) {
            this->startframe->value = 0;
            this->endframe->value = this->numf - 1;
        }
        float tbperframe = (float)this->video_duration / (float)this->numf;
        this->millif = tbperframe * (((float)this->video_stream->time_base.num * 1000.0) / (float)this->video_stream->time_base.den);
    }

    /*if (find_stream_index(&(this->audio_stream_idx), this->video, AVMEDIA_TYPE_AUDIO) >= 0 && !this->dummy) {
        this->audio_stream = this->video->streams[this->audio_stream_idx];
        const AVCodec *dec = avcodec_find_decoder(this->audio_stream->codecpar->codec_id);
        this->audio_dec_ctx = avcodec_alloc_context3(dec);
        avcodec_parameters_to_context(this->audio_dec_ctx, this->audio_stream->codecpar);
        avcodec_open2(this->audio_dec_ctx, dec, nullptr);
        this->sample_rate = this->audio_dec_ctx->sample_rate;
        this->channels = this->audio_dec_ctx->channels;
        this->channels = 1;
        switch (this->audio_dec_ctx->sample_fmt) {
            case AV_SAMPLE_FMT_U8:
                if (this->channels == 1) this->sampleformat = AL_FORMAT_MONO8;
                else this->sampleformat = AL_FORMAT_STEREO8;
                break;
            case AV_SAMPLE_FMT_S16:
                if (this->channels == 1) this->sampleformat = AL_FORMAT_MONO16;
                else this->sampleformat = AL_FORMAT_STEREO16;
                break;
            case AV_SAMPLE_FMT_S32:
                if (this->channels == 1) this->sampleformat = AL_FORMAT_MONO_FLOAT32;
                else this->sampleformat = AL_FORMAT_STEREO_FLOAT32;
                break;
            case AV_SAMPLE_FMT_FLT:
                if (this->channels == 1) this->sampleformat = AL_FORMAT_MONO_FLOAT32;
                else this->sampleformat = AL_FORMAT_STEREO_FLOAT32;
                break;
            case AV_SAMPLE_FMT_U8P:
                if (this->channels == 1) this->sampleformat = AL_FORMAT_MONO8;
                else this->sampleformat = AL_FORMAT_STEREO8;
                break;
            case AV_SAMPLE_FMT_S16P:
                if (this->channels == 1) this->sampleformat = AL_FORMAT_MONO16;
                else this->sampleformat = AL_FORMAT_STEREO16;
                break;
            case AV_SAMPLE_FMT_S32P:
                if (this->channels == 1) this->sampleformat = AL_FORMAT_MONO_FLOAT32;
                else this->sampleformat = AL_FORMAT_STEREO_FLOAT32;
                break;
            case AV_SAMPLE_FMT_FLTP:
                if (this->channels == 1) this->sampleformat = AL_FORMAT_MONO_FLOAT32;
                else this->sampleformat = AL_FORMAT_STEREO_FLOAT32;
                break;
        }

        this->audioplaying = true;
        this->audiot = std::thread{&Layer::playaudio, this};
        this->audiot.detach();
    }*/

    for (int k = 0; k < 3; k++) {
        this->rgbframe[k] = av_frame_alloc();
        this->rgbframe[k]->format = AV_PIX_FMT_BGRA;
        this->rgbframe[k]->width = this->video_dec_ctx->width;
        this->rgbframe[k]->height = this->video_dec_ctx->height;
        int storage = av_image_alloc(this->rgbframe[k]->data, this->rgbframe[k]->linesize, this->rgbframe[k]->width,
                                     this->rgbframe[k]->height, AV_PIX_FMT_BGRA, 32);
        if (storage < 0) {
            // reminder
            mainprogram->openerr = true;
            return 0;
        }
    }

    this->sws_ctx = sws_getContext
            (
                    this->video_dec_ctx->width,
                    this->video_dec_ctx->height,
                    this->video_dec_ctx->pix_fmt,
                    this->video_dec_ctx->width,
                    this->video_dec_ctx->height,
                    AV_PIX_FMT_BGRA,
                    SWS_BILINEAR,
                    nullptr,
                    nullptr,
                    nullptr);

    this->decframe = av_frame_alloc();

    return 1;
}


void Layer::playaudio() {

    ALint availBuffers = 0; // Buffers to be recovered
    ALint buffqueued = 0;
    ALuint myBuff; // The buffer we're using
    ALuint buffHolder[128]; // An array to hold catch the unqueued buffers
    bool done = false;
    std::list<ALuint> bufferQueue; // A quick and dirty queue of buffer objects
    ALuint temp[128];
    alGenBuffers(128, &temp[0]);
    // Queue our buffers onto an STL list
    for (int ii = 0; ii < 128; ++ii) {
        bufferQueue.push_back(temp[ii]);
    }
    // Request a source name
    ALuint source[1];
    alGenSources((ALuint)1, &source[0]);
    // Set the default volume
    alSourcef(source[0], AL_GAIN, this->volume->value);
    // Set the default position of the sound
    alSource3f(source[0], AL_POSITION, 0, 0, 0);
    while (this->audioplaying) {
        std::unique_lock<std::mutex> lock(this->chunklock);
        this->newchunk.wait(lock, [&]{return this->chready;});
        lock.unlock();
        this->chready = false;

        while ((this->playbut->value || this->revbut->value) && this->snippets.size()) {
            // Poll for recoverable buffers
            alSourcef(source[0], AL_GAIN, this->volume->value);
            alGetSourcei(source[0], AL_BUFFERS_PROCESSED, &availBuffers);
            if (availBuffers > 0) {
                alSourceUnqueueBuffers(source[0], availBuffers, buffHolder);
                for (int ii=0; ii<availBuffers; ++ii) {
                    // Push the recovered buffers back on the queue
                    bufferQueue.push_back(buffHolder[ii]);
                }
            }
            alGetSourcei(source[0], AL_BUFFERS_QUEUED, &buffqueued);
            // Stuff the data in a buffer-object
            int count = 0;
            if (this->snippets.size()) {
                char *input = this->snippets.front(); this->snippets.pop_front();
                int pslen = this->pslens.front(); this->pslens.pop_front();
                if (!bufferQueue.empty()) { // We just drop the data if no buffers are available
                    myBuff = bufferQueue.front(); bufferQueue.pop_front();
                    alBufferData(myBuff, this->sampleformat, input, pslen, this->sample_rate);
                    // Queue the buffer
                    alSourceQueueBuffers(source[0], 1, &myBuff);
                }
                if (pslen) delete[] input;
            }

            // Restart the source if needed
            // (if we take too long and the queue dries up,
            //  the source stops playing).
            ALint sState = 0;
            alGetSourcei(source[0], AL_SOURCE_STATE, &sState);
            if (sState != AL_PLAYING) {
                alSourcePlay(source[0]);
                alSourcei(source[0], AL_LOOPING, AL_FALSE);
            }
            float fac = mainmix->deckspeed[!mainprogram->prevmodus][this->deck]->value;
            if (this->clonesetnr != -1) {
                std::unordered_set<Layer*>::iterator it;
                for (it = mainmix->clonesets[this->clonesetnr]->begin(); it != mainmix->clonesets[this->clonesetnr]->end(); it++) {
                    Layer *lay = *it;
                    if (lay->deck == !this->deck) {
                        fac *= mainmix->deckspeed[!mainprogram->prevmodus][!this->deck]->value;
                        break;
                    }
                }
            }
            alSourcef(source[0], AL_PITCH, this->speed->value * fac * fac);
        }
    }
    alDeleteBuffers(128, &temp[0]);
}


void Layer::set_inlayer(Layer* lay, bool pbos) {
    if (pbos) {
        int sw1, sh1;
        glBindTexture(GL_TEXTURE_2D, this->texture);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &sw1);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &sh1);
        int sw2 = 0;
        int sh2 = 0;
        if (lay->type == ELEM_IMAGE) {
            sw2 = ilGetInteger(IL_IMAGE_WIDTH);
            sh2 = ilGetInteger(IL_IMAGE_HEIGHT);
        } else if (lay->video_dec_ctx) {
            sw2 = lay->video_dec_ctx->width;
            sh2 = lay->video_dec_ctx->height;
        };
        if (sw1 == sw2 && sh1 == sh2) {
            mainmix->copy_pbos(lay, this);
        }
    }
    int pos = this->pos;
    std::vector<Layer *> *lrs = this->layers;
    //mainmix->delete_layer(lrs, this, false);
    if (this->pos < lrs->size()) {
        lrs->erase(lrs->begin() + this->pos);
    }
    lay->queueing = this->queueing;
    if (!lay->queueing) {
        bool dummy = false;
    }
    //this->closethread = 1;
    if (pos <= lrs->size()) lrs->insert(lrs->begin() + pos, lay);
    lay->pos = pos;
    lay->layers = lrs;
    lay->clips = this->clips;
    lay->currclip = this->currclip;
    this->node->in = nullptr;
    this->node->out.clear();
    mainmix->reconnect_all(*lrs);
}

void prepare_param_cont(Param *par) {
    mainmix->buparval[par] = par->value;
    mainmix->bupar[par] = par;
    if (loopstation->parelemmap[par]) {
        mainmix->buelembool[par] = true;
        mainmix->bulpstelem[par] = loopstation->parelemmap[par]->eventlist;
    }
    for (LoopStationElement *elem: loopstation->elems) {
        if (!elem->eventlist.empty()) {
            for (int i = elem->eventlist.size() - 1; i >= 0; i--) {
                std::tuple<long long, Param *, Button *, float> event = elem->eventlist[i];
                if (std::get<1>(event) == par) {
                    elem->eventlist.erase(elem->eventlist.begin() + i);
                    mainmix->buplaybut[par] = elem->playbut->value;
                    mainmix->buloopbut[par] = elem->loopbut->value;
                    mainmix->bustarttime[par] = elem->starttime;
                    mainmix->buinterimtime[par] = elem->interimtime;
                    mainmix->butotaltime[par] = elem->totaltime;
                    mainmix->buspeedadaptedtime[par] = elem->speedadaptedtime;
                    mainmix->bulpspeed[par] = elem->speed->value;
                }
            }
        }
    }
}

void execute_param_cont(Param *par, Param *newpar) {
    LoopStationElement *elem = loopstation->parelemmap[newpar];
    if (elem) {
        if (!elem->eventlist.empty()) {
            for (int i = elem->eventlist.size() - 1; i >= 0; i--) {
                std::tuple<long long, Param *, Button *, float> event = elem->eventlist[i];
                if (std::get<1>(event) == newpar) {
                    elem->eventlist.erase(elem->eventlist.begin() + i);
                }
            }
        }
        loopstation->parelemmap[par]->params.erase(newpar);
        loopstation->parelemmap.erase(newpar);
    }
    if (mainmix->buelembool[par]) {
        for (LoopStationElement *elem: loopstation->elems) {
            if (elem->eventlist.empty()) {
                for (std::tuple<long long, Param *, Button *, float> event: mainmix->bulpstelem[par]) {
                    if (std::get<1>(event) == mainmix->bupar[par]) {
                        newpar->value = std::get<3>(event);
                        elem->add_param_automationentry(newpar, std::get<0>(event));
                        elem->playbut->value = mainmix->buplaybut[par];
                        elem->loopbut->value = mainmix->buloopbut[par];
                        elem->playbut->oldvalue = mainmix->buplaybut[par];
                        elem->loopbut->oldvalue = mainmix->buloopbut[par];
                        elem->eventpos = mainmix->buevpos[par];
                        elem->starttime = mainmix->bustarttime[par];
                        elem->interimtime = mainmix->buinterimtime[par];
                        elem->totaltime = mainmix->butotaltime[par];
                        elem->speedadaptedtime = mainmix->buspeedadaptedtime[par];
                        elem->speed->value = mainmix->bulpspeed[par];
                    }
                }
                break;
            }
        }
    }
    if (mainmix->adaptparam == mainmix->bupar[par]) mainmix->adaptparam = par;
    newpar->value = mainmix->buparval[par];
}

Layer* Mixer::open_layerfile(const std::string path, Layer* lay, bool loadevents, bool doclips, bool uselayers) {
    std::string result = deconcat_files(path);
    if (mainprogram->openerr) {
        return nullptr;
    }
    bool concat = (result != "");
    std::ifstream rfile;
    if (concat) rfile.open(result);
    else rfile.open(path);
    std::string istring;
    safegetline(rfile, istring);
    if (istring != "EWOCvj LAYERFILE") {
        mainprogram->openerr = true;
        return nullptr;
    }

    Node *out = nullptr;
    if (!lay->dummy) {
        if (lay->lasteffnode[1]->out.size()) {
            out = lay->lasteffnode[1]->out[0];
        }
    }
    bool bulsp = lay->lockspeed;
    bool bulzp = lay->lockzoompan;

    prepare_param_cont(lay->speed);
    prepare_param_cont(lay->shiftx);
    prepare_param_cont(lay->shifty);
    prepare_param_cont(lay->scale);

    bool bumute = lay->mutebut->value;
    bool busolo = lay->solobut->value;
    bool keepeff = lay->keepeffbut->value;
    if (!keepeff) {
        for (int i = 0; i < lay->effects[0].size(); i++) {
            //delete(lay->effects[0][i]);       reminder : leak! but small
        }
    }
    //loopstation->parelemmap.erase(lay->speed);
    loopstation->readelems.clear();
    loopstation->readelemnrs.clear();
    bool switching = false;
    if (lay->comp == mainprogram->prevmodus) {
        switching = true;
        mainprogram->prevmodus = !mainprogram->prevmodus;
    }
    Layer *lay2 = nullptr;
    mainprogram->laypos = lay->pos;
    mainmix->currclonesize = -1;
    mainmix->tempmapislayer = true;
    if (uselayers) {
       std::vector<Layer *> &try_layers = choose_layers(lay->deck);
        std::vector<Layer *> &is = choose_layers(lay->deck);
        std::vector<Layer *> &is_not = choose_layers(!lay->deck);
        std::vector<Layer *> &layers =
                std::find(try_layers.begin(), try_layers.end(), lay) != try_layers.end() ? is : is_not;
        std::vector<Layer *> layers2;
        if (layers == choose_layers(!lay->deck)) layers = layers2;
        bulrs[mainprogram->prevmodus][lay->deck] = layers;
        lay2 = mainmix->read_layers(rfile, result, layers, lay->deck, false, 0, doclips, concat, 1, loadevents, 0, lay->keepeffbut->value);
   }
    else {
        std::vector<Layer *> layers;
        lay2 = mainmix->read_layers(rfile, result, layers, lay->deck, false, 0, doclips, concat, 1, loadevents, 0, lay->keepeffbut->value);
    }
    if (switching) {
        mainprogram->prevmodus = !mainprogram->prevmodus;
        //lay2->comp = !lay2->comp;
    }
    if (keepeff ) {
        lay2->effects[0] = lay->effects[0];
        lay->effects[0].clear();
        if (lay2->effects[0].size()) {
            mainprogram->nodesmain->currpage->connect_nodes(lay2->node, lay2->effects[0].front()->node);
            mainprogram->nodesmain->currpage->connect_nodes(lay2->effects[0].back()->node, lay->lasteffnode[0]->out[0]);
        }
    }

    if (out) {
        if (lay2->effects[0].size()) lay2->lasteffnode[0] = lay2->effects[0][lay2->effects[0].size() - 1]->node;
        else lay2->lasteffnode[0] = lay2->node;
        if (lay2->effects[1].size()) lay2->lasteffnode[1] = lay2->effects[1][lay2->effects[1].size() - 1]->node;
        else if (lay2->pos > 0) lay2->lasteffnode[1] = lay2->blendnode;
        else {
            lay2->lasteffnode[1] = lay2->lasteffnode[0];
        }
        mainprogram->nodesmain->currpage->connect_nodes(lay2->lasteffnode[1], out);
    }

    // don't set these values (use backup values of previous layer)
    lay2->lockspeed = bulsp;
    lay2->lockzoompan = bulzp;
    if (lay2->lockspeed) {
        execute_param_cont(lay->speed, lay2->speed);
    }
    if (lay2->lockzoompan) {
        execute_param_cont(lay->shiftx, lay2->shiftx);
        execute_param_cont(lay->shifty, lay2->shifty);
        execute_param_cont(lay->scale, lay2->scale);
    }
    lay2->keepeffbut->value = keepeff;
    lay2->mutebut->value = bumute;
    lay2->solobut->value = busolo;

	rfile.close();

    if (lay2->type != ELEM_IMAGE) lay2->type = ELEM_LAYER;
	return lay2;
}

void Mixer::save_layerfile(const std::string path, Layer* lay, bool doclips, bool dojpeg) {
	std::string ext = path.substr(path.length() - 6, std::string::npos);
	std::string str;
	if (ext != ".layer") str = path + ".layer";
	else str = path;
	std::ofstream wfile;
	wfile.open(str);
	wfile << "EWOCvj LAYERFILE\n";

	std::vector<std::vector<std::string>> jpegpaths;
    std::vector<std::string> jp = mainmix->write_layer(lay, wfile, doclips, dojpeg);
	jpegpaths.push_back(jp);

	wfile << "ENDOFFILE\n";
	wfile.close();

    std::thread concat = std::thread(&Program::concat_files, mainprogram, mainprogram->temppath + "tempconcatlayerfile", str, jpegpaths);
    concat.detach();
	lay->layerfilepath = str;
}



// LAYER PROGRESSING

void Layer::cnt_lpst() {
    std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
    for (LoopStationElement *elem : this->lpst->elems) {
        std::chrono::duration<double> elapsed;
        elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - elem->starttime);
        long long millicount = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
        int passed = millicount - elem->interimtime;
        elem->interimtime = millicount;
        elem->speedadaptedtime = elem->speedadaptedtime + passed * this->speed->value;
        if (elem->speedadaptedtime >= elem->totaltime) {
            // reached end of loopstation element recording
            if (elem->loopbut->value) {
                //start loop again
                elem->speedadaptedtime = 0;
                elem->interimtime = 0;
                elem->starttime = std::chrono::high_resolution_clock::now() - std::chrono::milliseconds((long long)elem->interimtime);
            }
            else if (elem->playbut->value) {
                //end of single shot eventlist play
                elem->playbut->value = false;
                elem->playbut->oldvalue = true;
            }
        }
    }
}

bool Layer::progress(bool comp, bool alive) {
    // calculates new frame numbers for a video layer
    //measure time since last loop
    std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed;
    if (this->timeinit) elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - this->prevtime);
    else {
        this->timeinit = true;
        this->prevtime = now;
        return true;
    }
    long long microcount = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
    this->prevtime = now;
    float thismilli = (float)microcount / 1000.0f;

    if (mainprogram->tooltipbox && this->deck == 0 && this->pos == 0) {
        mainprogram->tooltipmilli += thismilli;
    }
    if (mainprogram->dragbinel && mainprogram->onscenebutton && this->deck == 0 && this->pos == 0) {
        mainprogram->onscenemilli += thismilli;
    }
    if (mainprogram->connfailed) mainprogram->connfailedmilli += thismilli;

    if (this->type != ELEM_LIVE) {
        if (!this->vidopen) {
            this->frame = this->frame + this->scratch;
            // calculate new frame numbers
            float fac = 0.0f;
            if (1) fac = mainmix->deckspeed[comp][this->deck]->value;
            else fac = this->olddeckspeed;
            if (this->clonesetnr != -1) {
                std::unordered_set<Layer*>::iterator it;
                for (it = mainmix->clonesets[this->clonesetnr]->begin(); it != mainmix->clonesets[this->clonesetnr]->end(); it++) {
                    Layer* l = *it;
                    if (l->deck == !this->deck) {
                        if (1) fac *= mainmix->deckspeed[comp][!this->deck]->value;
                        else fac *= this->olddeckspeed;
                        break;
                    }
                }
            }
            if (this->type == ELEM_IMAGE) {
                if (this->boundimage != -1) {
                    ilBindImage(this->boundimage);
                }
                this->millif = ilGetInteger(IL_IMAGE_DURATION);
            }
            if ((this->speed->value > 0 && (this->playbut->value || this->bouncebut->value == 1)) || (this->speed->value < 0 && (this->revbut->value || this->bouncebut->value == 2))) {
                if (this->scratch) {
                    bool dummy = false;
                }
                this->frame += !(this->scratch != 0 || this->scratchtouch) * this->speed->value * fac * fac * this->speed->value * thismilli / this->millif;
            }
            else if ((this->speed->value > 0 && (this->revbut->value || this->bouncebut->value == 2)) || (this->speed->value < 0 && (this->playbut->value || this->bouncebut->value == 1))) {
                this->frame -= !(this->scratch != 0 || this->scratchtouch) * this->speed->value * fac * fac * this->speed->value * thismilli / this->millif;
            }
            if ((int)(this->frame) != this->prevframe && this->type == ELEM_IMAGE && this->numf > 0) {
                // set animated gif to update now
                this->remfr[this->pbofri]->newdata = true;
            }
            this->scratch = 0;
            bool found = false;
            for (int i = 0; i < loopstation->elems.size(); i++) {
                if (std::find(loopstation->elems[i]->params.begin(), loopstation->elems[i]->params.end(), this->scritch) != loopstation->elems[i]->params.end()) {
                    found = true;
                    break;
                }
            }
            if (found) {
                this->frame = this->scritch->value;
            }

            // on video end (or video beginning if reverse playing) switch to the next clip in the queue
            if (1) { //this->oldalive || !alive
                if (this->scritching == 0) {
                    if (this->bouncebut->value || this->playbut->value || this->revbut->value) {
                        if (this->frame > (this->endframe->value) && this->startframe->value != this->endframe->value) {
                            if (this->scritching != 4) {
                                if (this->bouncebut->value == 0) {
                                    if (this->lpbut->value == 0) {
                                        this->playbut->value = 0;
                                        this->onhold = true;
                                    }
                                    this->frame = this->startframe->value;
                                    if (this->checkre) {
                                        if (!this->recstarted) {
                                            this->recstarted = true;
                                        }
                                        else{
                                            this->recended = true;
                                        }
                                    }
                                    this->clip_display_next(0, alive);
                                } else {
                                    this->frame = this->endframe->value - (this->frame - this->endframe->value);
                                    this->bouncebut->value = 2;
                                }
                            }
                        } else if (this->frame < this->startframe->value && this->startframe->value != this->endframe->value) {
                            if (this->scritching != 4) {
                                if (this->bouncebut->value == 0) {
                                    if (this->lpbut->value == 0) {
                                        this->revbut->value = 0;
                                        this->onhold = true;
                                    }
                                    //if (mainmix->checkre) mainmix->rerun = true;
                                    this->frame = this->endframe->value;
                                    if (this->checkre) {
                                        if (!this->recstarted) {
                                            this->recstarted = true;
                                        }
                                        else{
                                            this->recended = true;
                                        }
                                    }
                                    this->clip_display_next(1, alive);
                                } else {
                                    this->frame = this->startframe->value + (this->startframe->value - this->frame);
                                    this->bouncebut->value = 1;
                                }
                            }
                        }
                    }
                }
                if (this->scritching == 4) this->scritching = 0;
            }
            else {
                if (this->type == ELEM_FILE) {
                    this->open_video(this->frame, this->filename, 0);
                }
                else if (this->type == ELEM_LAYER) {
                    Layer *l = mainmix->open_layerfile(this->layerfilepath, this, 1, 0);
                    this->set_inlayer(l, true);
                }
                else if (this->type == ELEM_IMAGE) {
                    this->open_image(this->filename);
                }
                else if (this->type == ELEM_LIVE) {
                    this->set_live_base(this->filename);
                }
                this->oldalive = true;
            }
        }
    }

    // advance ripple effects
    for (int m = 0; m < 2; m++) {
        for (int i = 0; i < this->effects[m].size(); i++) {
            if (this->effects[m][i]->type == RIPPLE) {
                RippleEffect* effect = (RippleEffect*)(this->effects[m][i]);
                effect->set_ripplecount(effect->get_ripplecount() + (effect->get_speed() * (thismilli / 50.0f)));
                if (effect->get_ripplecount() > 100) effect->set_ripplecount(0);
            }
        }
    }

    return true;
}

void Layer::load_frame() {
    // checks if new frame has been decompressed and loads it into layer
    if (this->filename == "") return;
    Layer *srclay = this;
    bool ret = false;
    //if (this->vidopen) return;
    if (this->liveinput || this->type == ELEM_IMAGE) {

    }
    else if (this->startframe->value != this->endframe->value || this->type == ELEM_LIVE) {
        bool found = false;
        for (auto &it : mainmix->firstlayers) {
            if (it.second == srclay) {
                found = true;
                break;
            }
        }
        if (found || mainmix->firstlayers.count(this->clonesetnr) == 0) {
            if (!this->remfr[this->pbofri]->newdata) {
                // promote first layer found in layer stack with this clonesetnr to element of firstlayers
                this->ready = true;
                if ((int) (this->frame) != this->prevframe) {
                    this->startdecode.notify_all();
                }
                /*if (this->clonesetnr != -1) {
                    mainmix->firstlayers[this->clonesetnr] = this;
                    this->isclone = false;
                }*/
            }
        } else {
            srclay = mainmix->firstlayers[this->clonesetnr];
            int sw, sh;
            glBindTexture(GL_TEXTURE_2D, srclay->texture);
            glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &sw);
            glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &sh);
            this->texture = srclay->texture;
            this->newtexdata = true;
            ret = true;
        }
    } else return;

    /*if (this->vidopen) {
        return;
    }*/

    if (srclay->pos == 1 && srclay->deck == 1) {
        bool dummy = false;
    }

    // initialize layer?
    if (this->isduplay) return;
    if (!this->liveinput && !srclay->video_dec_ctx && srclay->type != ELEM_IMAGE) return;
    if ((!srclay->initialized && srclay->initdeck) || (!srclay->mapptr[srclay->pbodi] && srclay->type != ELEM_IMAGE) || (!srclay->initialized && srclay->changeinit < 0) || srclay->changeinit == 4) {
        // reminder : test with different bpp
        if (!this->liveinput) {
            if (srclay->video_dec_ctx) {
                if (srclay->vidformat == 188 || srclay->vidformat == 187) {
                    if (srclay->decresult->compression) {
                        float w = srclay->video_dec_ctx->width;
                        float h = srclay->video_dec_ctx->height;
                        this->initialize(w, h, srclay->decresult->compression);
                    }
                } else {
                    float w = srclay->video_dec_ctx->width;
                    float h = srclay->video_dec_ctx->height;
                    this->initialize(w, h);
                }
            } else return;
        } else {
            if (this->liveinput->video_dec_ctx) {
                float w = this->liveinput->video_dec_ctx->width;
                float h = this->liveinput->video_dec_ctx->height;
                this->initialize(w, h);
            }
        }
    }
    if (ret) return;

    // set frame data to texture
    if (this->liveinput) {  // mimiclayers trick/// :(
        this->texture = this->liveinput->texture;
        return;
    }

    if (this->isclone) return;


    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "OpenGL error3: " << err << std::endl;
    }


    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, srclay->texture);

    if (mainprogram->encthreads == 0) {
        WaitBuffer(srclay->syncobj[srclay->pbofri]);
    }

    if (srclay->started2 || srclay->type == ELEM_IMAGE || srclay->type == ELEM_LIVE) {

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, srclay->pbo[srclay->pboui]);


        if ((!srclay->remfr[srclay->pboui]->liveinput && srclay->remfr[srclay->pboui]->initialized) || srclay->numf == 0) {
            if ((!srclay->remfr[srclay->pboui]->isclone)) {  // decresult contains new frame width, height, number of bitmaps && data
                //if (!srclay->decresult->width) {
                //    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
                //	return;
                //}
                if ((srclay->remfr[srclay->pboui]->vidformat == 188 || srclay->remfr[srclay->pboui]->vidformat == 187)) {
                    //if (!srclay->databufready) {
                    //    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
                    //	return;
                    //}
                    // HAP video layers

                    if (srclay->remfr[srclay->pboui]->compression == 187 ||
                        srclay->remfr[srclay->pboui]->compression == 171) {

                        glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, srclay->remfr[srclay->pboui]->width,
                                                  srclay->remfr[srclay->pboui]->height,
                                                  GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, srclay->remfr[srclay->pboui]->size, 0);
                    } else if (srclay->remfr[srclay->pboui]->compression == 190) {
                        glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, srclay->remfr[srclay->pboui]->width,
                                                  srclay->remfr[srclay->pboui]->height, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,
                                                  srclay->remfr[srclay->pboui]->size, 0);
                    }
                } else {
                    if (srclay->type == ELEM_IMAGE) {
                        int imageformat = ilGetInteger(IL_IMAGE_FORMAT);
                        int bpp = ilGetInteger(IL_IMAGE_BPP);
                        int w = ilGetInteger(IL_IMAGE_WIDTH);
                        int h = ilGetInteger(IL_IMAGE_HEIGHT);
                        GLuint mode = GL_BGR;
                        if (imageformat == IL_RGBA) mode = GL_RGBA;
                        if (imageformat == IL_BGRA) mode = GL_BGRA;
                        if (imageformat == IL_RGB) mode = GL_RGB;
                        if (imageformat == IL_BGR) mode = GL_BGR;
                        if (srclay->numf == 0) {
                            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
                            glBindTexture(GL_TEXTURE_2D, srclay->texture);
                            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, mode, GL_UNSIGNED_BYTE,
                                            srclay->remfr[srclay->pboui]->data);
                        } else {
                            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, mode, GL_UNSIGNED_BYTE,  0);
                        }
                    } else if (1) {
                        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, srclay->remfr[srclay->pboui]->width,
                                        srclay->remfr[srclay->pboui]->height, GL_BGRA, GL_UNSIGNED_BYTE, 0);
                    }
                }
            }
        }
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }

    if (srclay->type == ELEM_IMAGE) {
        ilBindImage(srclay->boundimage);
        ilActiveImage((int) srclay->frame);
        srclay->remfr[srclay->pbofri]->data = (char *) ilGetData();
        srclay->remfr[srclay->pbofri]->newdata = true;
        if (srclay->numf == 0) {
            srclay->changeinit = 4;
            return;
        }
        bool dummy = false;
    }
    if (!srclay->remfr[srclay->pbofri]->newdata) {
        return;
    }

    int w, h;
    if (srclay->type == ELEM_IMAGE) {
        w = ilGetInteger(IL_IMAGE_WIDTH);
        h = ilGetInteger(IL_IMAGE_HEIGHT);
    } else if (srclay->video_dec_ctx) {
        w = srclay->video_dec_ctx->width;
        h = srclay->video_dec_ctx->height;
    }
    else return;
    if ((srclay->changeinit == -1 && srclay->initdeck) || srclay->changeinit == 3 || (!srclay->newload && srclay->started2 && (srclay->remfr[srclay->pbofri]->width != w ||
                                                                                                                               srclay->remfr[srclay->pbofri]->height != h || srclay->remfr[srclay->pbofri]->bpp != srclay->bpp))) {
        // video (size) changed
        srclay->changeinit = srclay->pbodi;
    }
    if (!srclay->isclone && !srclay->liveinput) {
        // start transferring to pbou
        // bind PBO to update pixel values
        //glBindBuffer(GL_PIXEL_UNPACK_BUFFER, srclay->pbo[srclay->pboui]);
        if (srclay->mapptr[srclay->pbodi]) {
            if (srclay->changeinit > -1 && !srclay->initdeck && srclay->changeinit != 5) {
                // make new pbos one by one when switching layer
                printf("changepbo %d\n", srclay->pboui);
                glDeleteBuffers(1, &srclay->pbo[srclay->pboui]);
                glGenBuffers(1, &srclay->pbo[srclay->pboui]);
                int bsize = w * h * srclay->bpp;
                int flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
                glBindBuffer(GL_ARRAY_BUFFER, srclay->pbo[srclay->pboui]);
                glBufferStorage(GL_ARRAY_BUFFER, bsize, 0, flags);
                srclay->mapptr[srclay->pboui] = (GLubyte *) glMapBufferRange(GL_ARRAY_BUFFER, 0, bsize, flags);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
            }

            if (srclay->started && srclay->remfr[srclay->pbodi]->newdata) {
                // update data directly on the mapped buffer
                memcpy(srclay->mapptr[srclay->pbodi], srclay->remfr[srclay->pbodi]->data, srclay->remfr[srclay->pbodi]->size);
                //srclay->currclip->tex = copy_tex(srclay->node->vidbox->tex);
                if (mainprogram->encthreads == 0) LockBuffer(srclay->syncobj[srclay->pbodi]);
            }
            if (srclay->started) {
                srclay->started2 = true;
            }
            if (srclay->started == false) {
                srclay->started = true;
            }
        }
    } else srclay->initialized = true;  //reminder: trick, is set false somewhere

    // round robin triple pbos: currently activated
    srclay->remfr[srclay->pbofri]->compression = srclay->decresult->compression;
    srclay->remfr[srclay->pbofri]->vidformat = srclay->vidformat;
    srclay->remfr[srclay->pbofri]->initialized = srclay->initialized;
    srclay->remfr[srclay->pbofri]->liveinput = srclay->liveinput;
    srclay->remfr[srclay->pbofri]->isclone = srclay->isclone;
    if (srclay->type == ELEM_IMAGE) {
        srclay->remfr[srclay->pbofri]->width = ilGetInteger(IL_IMAGE_WIDTH);
        srclay->remfr[srclay->pbofri]->height = ilGetInteger(IL_IMAGE_HEIGHT);
    } else {
        srclay->remfr[srclay->pbofri]->width = srclay->video_dec_ctx->width;
        srclay->remfr[srclay->pbofri]->height = srclay->video_dec_ctx->height;
    };
    srclay->remfr[srclay->pbofri]->bpp = srclay->bpp;
    srclay->remfr[srclay->pbofri]->size = srclay->decresult->size;
    srclay->remfr[srclay->pbodi]->newdata = false;
    //srclay->pboimutex.lock();
    srclay->pbodi++;
    srclay->pboui++;
    srclay->pbofri++;
    if (srclay->pbodi == 3) srclay->pbodi = 0;
    if (srclay->pboui == 3) srclay->pboui = 0;
    if (srclay->pbofri == 3) srclay->pbofri = 0;
    if (srclay->changeinit == srclay->pbodi) srclay->changeinit = 4;
    //srclay->pboimutex.unlock();


    srclay->newtexdata = true;
}


// IMAGE

Layer* Layer::open_image(const std::string path, bool init) {
    Layer *lay = this;
    if (!this->dummy && !this->transfered) {
        Layer *lay = this->transfer();
        lay->transfered = true;
        lay->open_image(path);
        return lay;
    }
    this->transfered = false;

	this->vidopen = true;
	ilEnable(IL_CONV_PAL);
	if (this->boundimage != -1) {
		ilDeleteImages(1, &this->boundimage);
		this->boundimage = -1;
	}
	ilGenImages(1, &this->boundimage);
	ilBindImage(this->boundimage);
	ilActiveImage(0);
	ILboolean ret = ilLoadImage((const ILstring)path.c_str());
	if (ret == IL_FALSE) {
		printf("can't load image %s\n", path.c_str());
        mainprogram->openerr = true;
		return nullptr;
	}
	this->numf = ilGetInteger(IL_NUM_IMAGES);
	this->frame = 0.0f;
	this->startframe->value = 0;
	this->endframe->value = this->numf;
	int w = ilGetInteger(IL_IMAGE_WIDTH);
	int h = ilGetInteger(IL_IMAGE_HEIGHT);
	this->bpp = ilGetInteger(IL_IMAGE_BPP);
	this->filename = path;
	this->vidformat = -1;
	if (init) {
        this->initialize(w, h);
        this->remfr[this->pboui]->initialized = true;
    }
	this->type = ELEM_IMAGE;
	this->decresult->size = w * h * this->bpp;
	this->decresult->width = -1;
	this->decresult->hap = false;
    if (this->numf == 0) {
        this->remfr[this->pboui]->data = (char *) ilGetData();
        this->remfr[this->pboui]->newdata = true;
        this->remfr[this->pboui]->vidformat = -1;
    }
    else {
        this->remfr[this->pbofri]->data = (char *) ilGetData();
        this->remfr[this->pbofri]->newdata = true;
    }

    if (mainprogram->autoplay && this->revbut->value == 0 && this->bouncebut->value == 0) {
        this->playbut->value = 1;
    }
	this->vidopen = false;

    return lay;
}


			// VIDEO FILES

void Layer::open_files_layers() {
	// order elements

    if (mainprogram->prevlayer) {
        if (mainprogram->prevlayer->changeinit != 5) {
            return;
        }
    }

	if (mainprogram->paths.size() == 0) {
        mainprogram->openfilesshelf = false;
        mainprogram->multistage = 0;
        return;
    }
    if (mainprogram->paths.size() != 1) {
        bool cont = mainprogram->order_paths(false);
        if (!cont) return;
    }

	// load one element of ordered list each loop
    //std::vector<Layer*>& lvec = (mainprogram->clickednextto != -1) ? choose_layers(mainprogram->clickednextto) : choose_layers(mainprogram->fileslay->deck);
    std::vector<Layer*>& lvec = choose_layers(mainprogram->fileslay->deck);
    std::string str = mainprogram->paths[mainprogram->filescount];
	Layer* lay = mainprogram->fileslay;
    /*if (mainprogram->clickednextto != -1) {
        lay = mainmix->add_layer(lvec, lvec.size());
    }
    else*/ if (mainprogram->filescount > 0) {
        lay = mainmix->add_layer(lvec, mainprogram->fileslay->pos + mainprogram->filescount);
    }

    // get file type
    std::string istring = "";
    std::string result = deconcat_files(str);
    if (!mainprogram->openerr) {
        bool concat = (result != "");
        std::ifstream rfile;
        if (concat) rfile.open(result);
        else rfile.open(str);
        safegetline(rfile, istring);
        rfile.close();
    } else mainprogram->openerr = false;

	if (isimage(str)) {
		lay->open_image(str);
	}
	else if (istring == "EWOCvj LAYERFILE") {  //reminder
		Layer *l = mainmix->open_layerfile(str, lay, true, true);
        mainprogram->prevlayer = l;
        mainmix->change_currlay(lay, l);
        if (mainmix->mouselayer && mainmix->mouselayer->filename != "") lay->set_inlayer(l, true);
        else lay->set_inlayer(l, false);
	}
	else {
		lay->open_video(0, str, true);
		std::unique_lock<std::mutex> olock(lay->endopenlock);
		lay->endopenvar.wait(olock, [&] {return lay->opened; });
		lay->opened = false;
		olock.unlock();
	}
    if (mainprogram->openerr) {
        lay->filename = "";
        mainprogram->openerr = false;
    }

	mainprogram->filescount++;
	if (mainprogram->filescount == mainprogram->paths.size()) {
		mainprogram->openfileslayers = false;
		mainprogram->paths.clear();
		mainprogram->multistage = 0;
		mainmix->reconnect_all(lvec);
	}
}

void Layer::open_files_queue() {
    // order elements
    bool cont = mainprogram->order_paths(false);
    if (!cont) return;

    // load one element of ordered list each loop
    std::string str = mainprogram->paths[mainprogram->filescount];
    if (mainprogram->filescount == 0) {
        mainprogram->clipfilesclip = mainprogram->fileslay->clips[0];
    }
    int pos = std::find(mainprogram->fileslay->clips.begin(), mainprogram->fileslay->clips.end(),
                    mainprogram->clipfilesclip) - mainprogram->fileslay->clips.begin();
    if (pos == mainprogram->fileslay->clips.size() - 1) {
        Clip *clip = new Clip;
        mainprogram->clipfilesclip = clip;
        clip->insert(mainprogram->fileslay, mainprogram->fileslay->clips.end() - 1);
    }

    // get file type
    std::string istring = "";
    std::string result = deconcat_files(str);
    if (!mainprogram->openerr) {
        bool concat = (result != "");
        std::ifstream rfile;
        if (concat) rfile.open(result);
        else rfile.open(str);
        safegetline(rfile, istring);
        rfile.close();
    } else mainprogram->openerr = false;

    if (isimage(str)) {
        mainprogram->clipfilesclip->path = str;

        Layer *lay = new Layer(true);
        get_imagetex(lay, str);
        std::unique_lock<std::mutex> lock2(lay->enddecodelock);
        lay->enddecodevar.wait(lock2, [&] {return lay->processed; });
        lay->processed = false;
        lock2.unlock();
        mainprogram->clipfilesclip->tex = mainprogram->get_tex(lay);
        lay->closethread = 1;

        mainprogram->clipfilesclip->type = ELEM_IMAGE;
        mainprogram->clipfilesclip->get_imageframes();
    } else if (istring == "EWOCvj LAYERFILE") {
        mainprogram->clipfilesclip->path = str;

        Layer *lay = new Layer(true);
        get_layertex(lay, str);
        std::unique_lock<std::mutex> lock2(lay->enddecodelock);
        lay->enddecodevar.wait(lock2, [&] {return lay->processed; });
        lay->processed = false;
        lock2.unlock();
        GLuint butex = lay->fbotex;
        lay->fbotex = copy_tex(lay->texture);
        glDeleteTextures(1, &butex);
        onestepfrom(0, lay->node, nullptr, -1, -1);
        if (lay->effects[0].size()) {
            mainprogram->clipfilesclip->tex = copy_tex(lay->effects[0][lay->effects[0].size() - 1]->fbotex, binsmain->elemboxes[0]->scrcoords->w, binsmain->elemboxes[0]->scrcoords->h);
        } else {
            mainprogram->clipfilesclip->tex = copy_tex(lay->fbotex, binsmain->elemboxes[0]->scrcoords->w, binsmain->elemboxes[0]->scrcoords->h);
        }
        lay->closethread = 1;

        mainprogram->clipfilesclip->type = ELEM_LAYER;
        mainprogram->clipfilesclip->get_layerframes();
    } else {
        mainprogram->clipfilesclip->path = str;
        mainprogram->clipfilesclip->type = ELEM_FILE;

        Layer *lay = new Layer(true);
        get_videotex(lay, str);
        std::unique_lock<std::mutex> lock2(lay->enddecodelock);
        lay->enddecodevar.wait(lock2, [&] {return lay->processed; });
        lay->processed = false;
        lock2.unlock();

        mainprogram->clipfilesclip->tex = mainprogram->get_tex(lay);
        lay->closethread = 1;
        mainprogram->clipfilesclip->get_videoframes();
    }
    mainprogram->clipfilesclip = mainprogram->fileslay->clips[pos + 1];

	mainprogram->filescount++;
	if (mainprogram->filescount == mainprogram->paths.size()) {
		mainprogram->fileslay->cliploading = false;
		mainprogram->openfilesqueue = false;
        mainprogram->fileslay->queueing = true;
        mainprogram->queueing = true;
		mainprogram->paths.clear();
		mainprogram->multistage = 0;
	}
}



			// UTILITY

					// WORKING WITH LAYERS

Layer* Mixer::read_layers(std::istream &rfile, const std::string result, std::vector<Layer*> &to_layers, bool deck, bool isdeck, int type, bool doclips, bool concat, bool load, bool loadevents, bool save, bool keepeff) {
    Layer *lay = nullptr;
    Layer *layend = nullptr;
    std::string istring;
    std::string filename;
	bool newlay = false;
	bool notfound = false;
	int overridepos = 0;
	std::vector<Layer*> waitlayers;
    std::vector<Layer*> layers;
    /*if (to_layers.size()) {
        if (mainprogram->prevmodus) {
            if (to_layers[0] == bulrs[1][0][0]) {
                mainmix->swapmap[0].clear();
            } else if (to_layers[0] == bulrs[1][1][0]) {
                mainmix->swapmap[1].clear();
            }
        } else {
            if (to_layers[0] == bulrs[0][0][0]) {
                mainmix->swapmap[2].clear();
            } else if (to_layers[0] == bulrs[0][1][0]) {
                mainmix->swapmap[3].clear();
            }
        }
    }*/
    while (safegetline(rfile, istring)) {
		if (istring == "LAYERSB" || istring == "ENDOFFILE") {
			break;
		}
		if (istring == "DECKSPEED") {
			safegetline(rfile, istring);
			mainmix->deckspeed[!mainprogram->prevmodus][deck]->value = std::stof(istring);
		}
		if (istring == "DECKSPEEDEVENT") {
			Param *par = mainmix->deckspeed[!mainprogram->prevmodus][deck];
			safegetline(rfile, istring);
			if (istring == "EVENTELEM") {
				mainmix->event_read(rfile, par, nullptr, lay);
			}
		}
		if (istring == "DECKSPEEDMIDI0") {
			safegetline(rfile, istring);
			mainmix->deckspeed[!mainprogram->prevmodus][deck]->midi[0] = std::stoi(istring);
		}
		if (istring == "DECKSPEEDMIDI1") {
			safegetline(rfile, istring);
			mainmix->deckspeed[!mainprogram->prevmodus][deck]->midi[1] = std::stoi(istring);
		}
		if (istring == "DECKSPEEDMIDIPORT") {
			safegetline(rfile, istring);
			mainmix->deckspeed[!mainprogram->prevmodus][deck]->midiport = istring;
            mainmix->deckspeed[!mainprogram->prevmodus][deck]->register_midi();
		}
		if (istring == "POS") {
			safegetline(rfile, istring);
			int pos = std::stoi(istring);  // reminder : does this need to be saved?
			if (overridepos == 0 || type == 0) {
                if (mainprogram->gettinglayertex) {
                    lay = mainprogram->gettinglayertexlay;
                    layend = mainmix->add_layer(layers, overridepos);
                }
                else {
                    lay = mainmix->add_layer(layers, overridepos);
                    layend = lay;
                }
				lay->numefflines[0] = 0;
				if (type == 1) mainmix->currlay[!mainprogram->prevmodus] = lay;
				//if (lay->lasteffnode[0] != lay->lasteffnode[1]) lay->lasteffnode[1]->out.clear();
			}
			else {
				lay = mainmix->add_layer(layers, overridepos);
                layend = lay;
			}
            layend->lasteffnode[0] = lay->node;
 			layend->deck = deck;
            layend->pos = pos;
            if (mainprogram->laypos != -1) {
                layend->pos = mainprogram->laypos;
                mainprogram->laypos = -1;
            }
            //lay->layers = &layers;

            /*if (bulrs[deck].size() > lay->pos) {
                lay->oldwidth = bulrs[deck][lay->pos]->iw;
                lay->oldheight = bulrs[deck][lay->pos]->ih;
			}*/

			overridepos++;
		}
		if (istring == "TYPE") {
			safegetline(rfile, istring);
			lay->type = (ELEM_TYPE)std::stoi(istring);
		}
        if (istring == "CLONESETNR") {
            safegetline(rfile, istring);
            if (mainmix->currclonesize != -1) {
                layend->clonesetnr = std::stoi(istring);
                if (layend->clonesetnr != -1) {
                    layend->clonesetnr += mainmix->currclonesize;
                    if (mainmix->clonesets.size() == 0 ||
                        (mainmix->clonesets.size() && mainmix->clonesets[layend->clonesetnr] == nullptr)) {
                        mainmix->firstlayers[layend->clonesetnr] = layend;
                        std::unordered_set<Layer *> *uset = new std::unordered_set<Layer *>;;
                        mainmix->clonesets[layend->clonesetnr] = uset;
                    } else {
                        layend->isclone = true;
                    }
                    mainmix->clonesets[layend->clonesetnr]->emplace(layend);
                }
            }
        }
        if (istring == "FILENAME") {
			if (layend->type != 0) layend->framesloaded = true;
        	notfound = false;
			safegetline(rfile, istring);
			filename = istring;
			if (load) {
				layend->timeinit = false;
				if (filename != "") {
					if (layend->type == ELEM_LIVE) {
						layend->initialized = false;
						bool found = false;
						for (int i = 0; i < mainprogram->busylayers.size(); i++) {
							if (mainprogram->busylayers[i]->filename == istring) {
								layend->liveinput = mainprogram->busylayers[i];
								mainprogram->mimiclayers.push_back(layend);
                                layend->filename = istring;
								found = true;
								break;
							}
						}
						if (!found) {
							layend->set_live_base(filename);
							layend->liveinput = nullptr;
						}
					}
                    else if (layend->clonesetnr == -1 || mainmix->firstlayers.count(layend->clonesetnr) == 1) {
                        //if (exists(filename)) {
                            if (lay->type == ELEM_IMAGE || isimage(filename)) {
                                //Layer *kplay = lay;
                                layend->transfered = true;
                                layend->open_image(filename);
                                layend->transfered = false;
                                layend->type = ELEM_IMAGE;
                                mainmix->loadinglays.push_back(layend);
                                //layend->timeinit = kplay->timeinit;
                                //mainmix->loadinglays.push_back(layend);
                                //layend->initialized = kplay->initialized;
                            }
                            else if (lay->type == ELEM_FILE || lay->type == ELEM_LAYER) {
                                Layer *kplay = lay;
                                layend->transfered = true;
                                layend = lay->open_video(0, filename, false, keepeff);
                                mainmix->loadinglays.push_back(layend);
                                layend->numefflines[0] = 0;
                                if (type == 1) mainmix->currlay[!mainprogram->prevmodus] = layend;
                                //if (lay->lasteffnode[0] != lay->lasteffnode[1]) lay->lasteffnode[1]->out.clear();
                                layend->lasteffnode[0] = layend->node;
                                layend->type = kplay->type;
                                layend->timeinit = kplay->timeinit;
                                layend->initialized = kplay->initialized;
                            }
                        //}
                    }
                    else {
                        filename = "";
                    }
				}
				if (type > 0) lay->prevframe = -1;
			}
		}
		if (istring == "RELPATH") {
			safegetline(rfile, istring);
			if (filename == "") {
                if (istring != "") {
                    std::filesystem::current_path(mainprogram->contentpath);
                    filename = pathtoplatform(std::filesystem::absolute(istring).generic_string());
                    if (layend->clonesetnr == -1 || mainmix->firstlayers.count(layend->clonesetnr) == 1) {
                        if (load && !notfound) {
                            lay->timeinit = false;
                            if (lay->type == ELEM_IMAGE || isimage(filename)) {
                                layend->transfered = true;
                                layend->open_image(filename);
                                layend->transfered = false;
                                layend->type = ELEM_IMAGE;
                                mainmix->loadinglays.push_back(layend);
                            } else if (lay->type == ELEM_FILE || lay->type == ELEM_LAYER) {
                                Layer *kplay = lay;
                                layend = lay->open_video(-1, filename, false, keepeff);
                                mainmix->loadinglays.push_back(layend);
                                layend->numefflines[0] = 0;
                                if (type == 1) mainmix->currlay[!mainprogram->prevmodus] = layend;
                                //if (lay->lasteffnode[0] != lay->lasteffnode[1]) lay->lasteffnode[1]->out.clear();
                                layend->lasteffnode[0] = layend->node;
                                layend->type = kplay->type;
                                layend->timeinit = kplay->timeinit;
                                layend->initialized = kplay->initialized;
                            }
                        }
                    }
                }
            }
            if (filename != "") {
                if (!exists(filename)) {
                    notfound = true;
                    this->newlaypaths.push_back(filename);
                    filename = "";
                }
            }
            layend->filename = filename;  // for CLIPLAYER
			newlay = true;

			if (!isdeck && filename != "") {
                if (to_layers.size()) {
                    if (to_layers[0] == bulrs[mainprogram->prevmodus][0][0] ||
                        to_layers[0] == bulrs[mainprogram->prevmodus][1][0]) {
                        // copy old pbos to maintain constant stream of frames
                        if (!mainmix->copycomp_busy &&
                            bulrs[mainprogram->prevmodus][layend->deck].size() > layend->pos &&
                            layend->type != ELEM_IMAGE) {
                            mainmix->copy_pbos(layend, bulrs[mainprogram->prevmodus][layend->deck][layend->pos]);
                            //layend->fbo = bulrs[lay1->deck][lay1->pos]->fbo;
                            //layend->fbotex = bulrs[lay1->deck][lay1->pos]->fbotex;
                        }
                    } else {
                        // opening layerfile : keep effects?
                    }
                }
            }
            if (notfound) {
                if (concat) {
                    std::filesystem::rename(result + "_" + std::to_string(mainprogram->filecount) + ".file",
                                              result + "_" + std::to_string(mainprogram->filecount) + ".jpeg");
                    open_thumb(result + "_" + std::to_string(mainprogram->filecount) + ".jpeg", layend->jpegtex);
                }
                mainmix->retargeting = true;
                this->newpathlayers.push_back(layend);
                //mainprogram->filescount++;
            }
		}
		if (istring == "JPEGPATH") {
			safegetline(rfile, istring);
            mainprogram->filecount++;  // dojpeg? wherefor?
		}
        if (istring == "FILESIZE") {
            safegetline(rfile, istring);
            layend->filesize = std::stoll(istring);
        }
        if (istring == "WIDTH") {
            safegetline(rfile, istring);
            layend->iw = std::stoi(istring);
        }
		if (istring == "HEIGHT") {
			safegetline(rfile, istring);
			layend->ih = std::stoi(istring);
		}
		if (istring == "ASPECTRATIO") {
			safegetline(rfile, istring);
			layend->aspectratio = (RATIO_TYPE)std::stoi(istring);
		}
        if (istring == "QUEUEING") {
            safegetline(rfile, istring);
            layend->queueing = std::stoi(istring);
            if (layend->queueing) mainprogram->queueing = true;
        }
        if (istring == "MUTE") {
            safegetline(rfile, istring);
            layend->mutebut->value = std::stoi(istring);
        }
		if (istring == "MUTEEVENT") {
			Button* but = lay->mutebut;
			safegetline(rfile, istring);
			if (istring == "EVENTELEM") {
				mainmix->event_read(rfile, nullptr, but, layend);
			}
		}
        if (istring == "SOLO") {
            safegetline(rfile, istring);
            layend->solobut->value = std::stoi(istring);
        }
        if (istring == "SOLOEVENT") {
            Button* but = lay->solobut;
            safegetline(rfile, istring);
            if (istring == "EVENTELEM") {
                mainmix->event_read(rfile, nullptr, but, layend);
            }
        }
        if (istring == "KEEPEFF") {
            safegetline(rfile, istring);
            layend->keepeffbut->value = std::stoi(istring);
        }
        if (istring == "KEEPEFFEVENT") {
            Button* but = lay->keepeffbut;
            safegetline(rfile, istring);
            if (istring == "EVENTELEM") {
                mainmix->event_read(rfile, nullptr, but, layend);
            }
        }
		if (istring == "SPEEDVAL") {
			safegetline(rfile, istring);
			layend->speed->value = std::stof(istring);
		}
		if (istring == "SPEEDEVENT") {
			Param *par = lay->speed;
			safegetline(rfile, istring);
			if (istring == "EVENTELEM") {
				mainmix->event_read(rfile, par, nullptr, layend);
			}
		}
		if (istring == "PLAYBUTVAL") {
			safegetline(rfile, istring);
			layend->playbut->value = std::stoi(istring);
		}
		if (istring == "PLAYBUTEVENT") {
			Button* but = lay->playbut;
			safegetline(rfile, istring);
			if (istring == "EVENTELEM") {
				mainmix->event_read(rfile, nullptr, but, layend);
			}
		}
		if (istring == "REVBUTVAL") {
			safegetline(rfile, istring);
			layend->revbut->value = std::stoi(istring);
		}
		if (istring == "REVBUTEVENT") {
			Button* but = lay->revbut;
			safegetline(rfile, istring);
			if (istring == "EVENTELEM") {
				mainmix->event_read(rfile, nullptr, but, layend);
			}
		}
        if (istring == "BOUNCEBUTVAL") {
            safegetline(rfile, istring);
            layend->bouncebut->value = std::stoi(istring);
        }
        if (istring == "BOUNCEBUTEVENT") {
            Button* but = lay->bouncebut;
            safegetline(rfile, istring);
            if (istring == "EVENTELEM") {
                mainmix->event_read(rfile, nullptr, but, layend);
            }
        }
        if (istring == "LPBUTVAL") {
            safegetline(rfile, istring);
            layend->lpbut->value = std::stoi(istring);
        }
        if (istring == "LPBUTEVENT") {
            Button* but = lay->lpbut;
            safegetline(rfile, istring);
            if (istring == "EVENTELEM") {
                mainmix->event_read(rfile, nullptr, but, layend);
            }
        }
		if (istring == "PLAYKIND") {
			safegetline(rfile, istring);
			layend->playkind = std::stoi(istring);
		}
		if (istring == "GENMIDIBUTVAL") {
			safegetline(rfile, istring);
			layend->genmidibut->value = std::stoi(istring);
		}
		if (istring == "GENMIDIBUTEVENT") {
			Button* but = lay->genmidibut;
			safegetline(rfile, istring);
			if (istring == "EVENTELEM") {
				mainmix->event_read(rfile, nullptr, but, layend);
			}
		}
		if (istring == "FRAMEFORWARDEVENT") {
			Button* but = lay->frameforward;
			safegetline(rfile, istring);
			if (istring == "EVENTELEM") {
				mainmix->event_read(rfile, nullptr, but, layend);
			}
		}
		if (istring == "FRAMEBACKWARDEVENT") {
			Button* but = lay->framebackward;
			safegetline(rfile, istring);
			if (istring == "EVENTELEM") {
				mainmix->event_read(rfile, nullptr, but, layend);
			}
		}
        if (istring == "LOCKSPEED") {
            safegetline(rfile, istring);
            layend->lockspeed = std::stoi(istring);
        }
        if (istring == "LOCKZOOMPAN") {
            safegetline(rfile, istring);
            layend->lockzoompan = std::stoi(istring);
        }
        if (istring == "SHIFTX") {
            safegetline(rfile, istring);
            layend->shiftx->value = std::stof(istring);
        }
		if (istring == "SHIFTXEVENT") {
			Param *par = lay->shiftx;
			safegetline(rfile, istring);
			if (istring == "EVENTELEM") {
				mainmix->event_read(rfile, par, nullptr, layend);
			}
		}
		if (istring == "SHIFTY") {
			safegetline(rfile, istring);
			layend->shifty->value = std::stof(istring);
		}
		if (istring == "SHIFTYEVENT") {
			Param *par = lay->shifty;
			safegetline(rfile, istring);
			if (istring == "EVENTELEM") {
				mainmix->event_read(rfile, par, nullptr, layend);
			}
		}
		if (istring == "SCALE") {
			safegetline(rfile, istring);
			layend->scale->value = std::stof(istring);
		}
        if (istring == "SCALEEVENT") {
            Param *par = lay->scale;
            safegetline(rfile, istring);
            if (istring == "EVENTELEM") {
                mainmix->event_read(rfile, par, nullptr, layend);
            }
        }
        if (istring == "SCRITCHEVENT") {
            Param *par = lay->scritch;
            safegetline(rfile, istring);
            if (istring == "EVENTELEM") {
                mainmix->event_read(rfile, par, nullptr, layend);
            }
        }
		if (istring == "OPACITYVAL") {
			safegetline(rfile, istring);
			layend->opacity->value = std::stof(istring);
		}
		if (istring == "OPACITYEVENT") {
			Param *par = lay->opacity;
			safegetline(rfile, istring);
			if (istring == "EVENTELEM") {
				mainmix->event_read(rfile, par, nullptr, layend);
			}
		}
		if (istring == "VOLUMEVAL") {
			safegetline(rfile, istring);
			layend->volume->value = std::stof(istring);
		}
		if (istring == "VOLUMEEVENT") {
			Param *par = lay->volume;
			safegetline(rfile, istring);
			if (istring == "EVENTELEM") {
				mainmix->event_read(rfile, par, nullptr, layend);
			}
		}
        if (istring == "MIXFACEVENT") {
            Param *par = lay->blendnode->mixfac;
            safegetline(rfile, istring);
            if (istring == "EVENTELEM") {
                mainmix->event_read(rfile, par, nullptr, layend);
            }
        }
		if (istring == "CHTOLVAL") {
			safegetline(rfile, istring);
			layend->chtol->value = std::stof(istring);
		}
		if (istring == "CHDIRVAL") {
			safegetline(rfile, istring);
			layend->chdir->value = std::stoi(istring);
		}
		if (istring == "CHINVVAL") {
			safegetline(rfile, istring);
			layend->chinv->value = std::stoi(istring);
		}
		if (istring == "FRAME") {
			safegetline(rfile, istring);
			layend->frame = std::stof(istring);
		}
		if (istring == "STARTFRAME") {
			safegetline(rfile, istring);
			layend->startframe->value = std::stof(istring);
			bool dummy = false;
		}
        if (istring == "ENDFRAME") {
            safegetline(rfile, istring);
            layend->endframe->value = std::stof(istring);
        }
        if (istring == "MILLIF") {
            safegetline(rfile, istring);
            layend->millif = std::stof(istring);
        }
		if (layend) {
			if (!layend->dummy) {
				if (istring == "MIXMODE") {
					safegetline(rfile, istring);
					layend->blendnode->blendtype = (BLEND_TYPE)std::stoi(istring);
				}
				if (istring == "MIXFACVAL") {
					safegetline(rfile, istring);
					layend->blendnode->mixfac->value = std::stof(istring);
				}
				if (istring == "CHRED") {
					safegetline(rfile, istring);
					layend->blendnode->chred = std::stof(istring);
					layend->colorbox->acolor[0] = lay->blendnode->chred;
				}
				if (istring == "CHGREEN") {
					safegetline(rfile, istring);
					layend->blendnode->chgreen = std::stof(istring);
					layend->colorbox->acolor[1] = lay->blendnode->chgreen;
				}
				if (istring == "CHBLUE") {
					safegetline(rfile, istring);
					layend->blendnode->chblue = std::stof(istring);
					layend->colorbox->acolor[2] = lay->blendnode->chblue;
				}
				if (istring == "WIPETYPE") {
					safegetline(rfile, istring);
					layend->blendnode->wipetype = std::stoi(istring);
				}
				if (istring == "WIPEDIR") {
					safegetline(rfile, istring);
					layend->blendnode->wipedir = std::stoi(istring);
				}
				if (istring == "WIPEX") {
					safegetline(rfile, istring);
					layend->blendnode->wipex->value = std::stof(istring);
				}
                if (istring == "WIPEXEVENT") {
                    Param *par = layend->blendnode->wipex;
                    safegetline(rfile, istring);
                    if (istring == "EVENTELEM") {
                        mainmix->event_read(rfile, par, nullptr, layend);
                    }
                }
				if (istring == "WIPEY") {
					safegetline(rfile, istring);
					layend->blendnode->wipey->value = std::stof(istring);
				}
                if (istring == "WIPEYEVENT") {
                    Param *par = layend->blendnode->wipey;
                    safegetline(rfile, istring);
                    if (istring == "EVENTELEM") {
                        mainmix->event_read(rfile, par, nullptr, layend);
                    }
                }
			}
		}
		
		if (istring == "CLIPS") {
			Clip *clp = nullptr;
            bool isvid = false;
            //int clipfilecount = 0;
			while (safegetline(rfile, istring)) {
				if (istring == "ENDOFCLIPS") break;
				if (doclips) {
					if (istring == "TYPE") {
						clp = new Clip;
						safegetline(rfile, istring);
						clp->type = (ELEM_TYPE)std::stoi(istring);
					}
					if (istring == "FILENAME") {
                        safegetline(rfile, istring);
                        if (clp->type == ELEM_LAYER && !isvid) {
                            // pass control to CLIPLAYER
                        }
                        /**else if (exists(istring)) {
                            clp->path = istring;
                            clp->insert(layend, layend->clips.end() - 1);
                        }
                        else {
                            clp->path = "";
                        }*/
					}
                    if (istring == "RELPATH") {
                        safegetline(rfile, istring);
                        if (clp->type == ELEM_LAYER && !isvid) {
                            // pass control to CLIPLAYER
                        }
                        else if (clp->path == "" && istring != "") {
                            std::filesystem::current_path(mainprogram->contentpath);
                            clp->path = pathtoplatform(std::filesystem::absolute(istring).generic_string());
                            if (!exists(clp->path)) {
                                if (isvid) {
                                    mainmix->retargeting = true;
                                    this->newclippaths.push_back(clp->path);
                                    this->newpathclips.push_back(clp);
                                    clp->layer = layend;
                                }
                            }
                            else clp->insert(layend, layend->clips.end() - 1);
                        }
                        isvid = false;
                    }
                    if (istring == "FILESIZE") {
                        safegetline(rfile, istring);
                        clp->filesize = std::stoll(istring);
                    }
                    if (istring == "CLIPLAYER") {
						std::vector<Layer*> cliplayers;
                        Layer *cliplay = mainmix->read_layers(rfile, result, cliplayers, 0, false, 0, 0, 1, 0, 0, 0);
						clp->path = find_unused_filename("cliptemp_" + remove_extension(basename(result)), mainprogram->temppath, ".layer");
                        if (!exists(cliplay->filename)) {
                            mainmix->retargeting = true;
                            this->newclippaths.push_back(cliplay->filename);
                            this->newpathclips.push_back(clp);
                            clp->layer = layend;
                        }
                        else {
                            mainmix->save_layerfile(clp->path, cliplay, 0, 0);
                            clp->insert(layend, layend->clips.end() - 1);
                        }
						//mainmix->delete_layer(cliplayers, cliplay, false);  // reminder
					}
					if (istring == "JPEGPATH") {
						safegetline(rfile, istring);
						std::string jpegpath = istring;
						glGenTextures(1, &clp->tex);
						glBindTexture(GL_TEXTURE_2D, clp->tex);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
						if (concat) {
							std::filesystem::rename(result + "_" + std::to_string(mainprogram->filecount) + ".file", result + "_" + std::to_string(mainprogram->filecount) + ".jpeg");
							open_thumb(result + "_" + std::to_string(mainprogram->filecount) + ".jpeg", clp->tex);
                            clp->jpegpath = result + "_" + std::to_string(mainprogram->filecount) + ".jpeg";
                            mainprogram->filecount++;
						}
						else open_thumb(jpegpath, clp->tex);
					}
					if (istring == "FRAME") {
						safegetline(rfile, istring);
						clp->frame = std::stof(istring);
					}
					if (istring == "STARTFRAME") {
						safegetline(rfile, istring);
						clp->startframe->value = std::stoi(istring);
					}
					if (istring == "ENDFRAME") {
						safegetline(rfile, istring);
						clp->endframe->value = std::stoi(istring);
					}
				}
			}
		}
		if (istring == "EFFECTS") {
			mainprogram->effcat[layend->deck]->value = 0;
			int type;
			Effect *eff = nullptr;
			safegetline(rfile, istring);
			while (istring != "ENDOFEFFECTS") {
			    if (keepeff) {
			        safegetline(rfile, istring);
			        continue;
			    }
				if (istring == "TYPE") {
					safegetline(rfile, istring);
					type = std::stoi(istring);
				}
				if (istring == "POS") {
					safegetline(rfile, istring);
					int pos = std::stoi(istring);
					eff = layend->add_effect((EFFECT_TYPE)type, pos);
				}
				if (istring == "ONOFFVAL") {
					safegetline(rfile, istring);
					eff->onoffbutton->value = std::stoi(istring);
				}
				if (istring == "ONOFFMIDI0") {
					safegetline(rfile, istring);
					eff->onoffbutton->midi[0] = std::stoi(istring);
				}
				if (istring == "ONOFFMIDI1") {
					safegetline(rfile, istring);
					eff->onoffbutton->midi[1] = std::stoi(istring);
				}
				if (istring == "ONOFFMIDIPORT") {
					safegetline(rfile, istring);
					eff->onoffbutton->midiport = istring;
                    eff->onoffbutton->register_midi();
				}
				if (istring == "ONOFFEVENT") {
					Button* but = eff->onoffbutton;
					safegetline(rfile, istring);
					if (istring == "EVENTELEM") {
						mainmix->event_read(rfile, nullptr, but, layend);
					}
				}
				if (istring == "DRYWETVAL") {
					safegetline(rfile, istring);
					eff->drywet->value = std::stof(istring);
				}
				if (istring == "DRYWETMIDI0") {
					safegetline(rfile, istring);
					eff->drywet->midi[0] = std::stoi(istring);
				}
				if (istring == "DRYWETMIDI1") {
					safegetline(rfile, istring);
					eff->drywet->midi[1] = std::stoi(istring);
				}
				if (istring == "DRYWETMIDIPORT") {
					safegetline(rfile, istring);
					eff->drywet->midiport = istring;
                    eff->drywet->register_midi();
				}
				if (eff) {
					if (eff->type == RIPPLE) {
						if (istring == "RIPPLESPEED") {
							safegetline(rfile, istring);
							((RippleEffect*)eff)->speed = std::stof(istring);
						}
						if (istring == "RIPPLECOUNT") {
							safegetline(rfile, istring);
							((RippleEffect*)eff)->ripplecount = std::stof(istring);
						}
					}
				}
				
				if (istring == "PARAMS") {
					int pos = 0;
					Param *par = nullptr;
					while (istring != "ENDOFPARAMS") {
						safegetline(rfile, istring);
						if (istring == "VAL") {
							par = eff->params[pos];
							pos++;
							safegetline(rfile, istring);
							par->value = std::stof(istring);
							par->effect = eff;
						}
						if (istring == "MIDI0") {
							safegetline(rfile, istring);
							par->midi[0] = std::stoi(istring);
						}
						if (istring == "MIDI1") {
							safegetline(rfile, istring);
							par->midi[1] = std::stoi(istring);
						}
						if (istring == "MIDIPORT") {
							safegetline(rfile, istring);
							par->midiport = istring;
                            par->register_midi();
						}
						if (istring == "EVENTELEM") {
							if (loadevents) {
								mainmix->event_read(rfile, par, nullptr, layend);
							}
							else {
								while (safegetline(rfile, istring)) {
									if (istring == "ENDOFEVENT") break;
								}
							}
						}
					}
				}
				safegetline(rfile, istring);
			}
		}
		if (istring == "STREAMEFFECTS") {
			mainprogram->effcat[layend->deck]->value = 1;
			int type;
			Effect *eff = nullptr;
			safegetline(rfile, istring);
			while (istring != "ENDOFSTREAMEFFECTS") {
				if (istring == "TYPE") {
					safegetline(rfile, istring);
					type = std::stoi(istring);
				}
				if (istring == "POS") {
					safegetline(rfile, istring);
					int pos = std::stoi(istring);
					eff = layend->add_effect((EFFECT_TYPE)type, pos);
				}
				if (istring == "ONOFFVAL") {
					safegetline(rfile, istring);
					eff->onoffbutton->value = std::stoi(istring);
				}
				if (istring == "DRYWETVAL") {
					safegetline(rfile, istring);
					eff->drywet->value = std::stof(istring);
				}
				if (eff) {
					if (eff->type == RIPPLE) {
						if (istring == "RIPPLESPEED") {
							safegetline(rfile, istring);
							((RippleEffect*)eff)->speed = std::stof(istring);
						}
						if (istring == "RIPPLECOUNT") {
							safegetline(rfile, istring);
							((RippleEffect*)eff)->ripplecount = std::stof(istring);
						}
					}
				}
				
				if (istring == "PARAMS") {
					int pos = 0;
					Param *par = nullptr;
					while (istring != "ENDOFPARAMS") {
						safegetline(rfile, istring);
						if (istring == "VAL") {
							par = eff->params[pos];
							pos++;
							safegetline(rfile, istring);
							par->value = std::stof(istring);
							par->effect = eff;
						}
						if (istring == "MIDI0") {
							safegetline(rfile, istring);
							par->midi[0] = std::stoi(istring);
						}
						if (istring == "MIDI1") {
							safegetline(rfile, istring);
							par->midi[1] = std::stoi(istring);
						}
						if (istring == "MIDIPORT") {
							safegetline(rfile, istring);
							par->midiport = istring;
                            par->register_midi();
						}
						if (istring == "EVENTELEM") {
							if (loadevents) {
								mainmix->event_read(rfile, par, nullptr, layend);
							}
							else {
								while (safegetline(rfile, istring)) {
									if (istring == "ENDOFEVENT") break;
								}
							}
						}
					}
				}
				safegetline(rfile, istring);
			}
		}
        if (istring == "EFFCAT") {
            safegetline(rfile, istring);
            mainprogram->effcat[layend->deck]->value = std::atoi(istring.c_str());
        }

		if (newlay && save) {
			newlay = false;
			layend->layerfilepath = find_unused_filename(basename(layend->filename), mainprogram->temppath, ".layer");
			mainmix->save_layerfile(layend->layerfilepath, layend, false, false);
		}
	}

    if (to_layers.size()) {
        if (to_layers[0] == bulrs[mainprogram->prevmodus][0][0] ||
            to_layers[0] == bulrs[mainprogram->prevmodus][1][0]) {
            for (int i = 0; i < layers.size(); i++) {
                //if (layers[i]->mutebut->value) layers[i]->mute_handle();
                layers[i]->set_aspectratio(layers[i]->iw, layers[i]->ih);
            }
            //mainmix->reconnect_all(to_layers);
        }
    }

    /*for (Layer *lay2 : waitlayers) {
        std::unique_lock<std::mutex> olock(lay2->endopenlock);
        lay2->endopenvar.wait(olock, [&] { return lay2->open.ed; });
        lay2->opened = false;
        olock.unlock();
    }*/

    if (to_layers.size()) {
        std::vector<Layer*> lrs;
        std::vector<std::vector<Layer*>> *tempmap;
        if (mainprogram->prevmodus) {
            if (to_layers[0] == bulrs[1][0][0]) {
                lrs = bulrs[1][0];
                mainmix->newlrs[0] = layers;
                this->layers[0] = lrs;
                tempmap = &mainmix->swapmap[0];
            } else if (to_layers[0] == bulrs[1][1][0]) {
                lrs = bulrs[1][1];
                mainmix->newlrs[1] = layers;
                this->layers[1] = lrs;
                tempmap = &mainmix->swapmap[1];
            }
        }
        else {
            if (to_layers[0] == bulrs[0][0][0]) {
                lrs = bulrs[0][0];
                mainmix->newlrs[2] = layers;
                this->layers[2] = lrs;
                tempmap = &mainmix->swapmap[2];
            } else if (to_layers[0] == bulrs[0][1][0]) {
                lrs = bulrs[0][1];
                mainmix->newlrs[3] = layers;
                this->layers[3] = lrs;
                tempmap = &mainmix->swapmap[3];
            }
        }
        if (lrs.size()) {
            for (int i = 0; i < std::max(layers.size(), lrs.size()); i++) {
                Layer *first = nullptr;
                Layer *second = nullptr;
                if (i < lrs.size()) {
                    first = lrs[i];
                }
                for (Layer *l1 : layers) {
                    if (l1->pos == i) {
                        second = l1;
                        break;
                    }
                }
                (*tempmap).push_back({first, second});
                if (second) second->initdeck = true;
            }
        }
    }
    //}

    return layend;  // for when open_video or open_layerfile swaps for a new layer
}
	
std::vector<std::string> Mixer::write_layer(Layer* lay, std::ostream& wfile, bool doclips, bool dojpeg) {
	std::vector<std::string> jpegpaths;

	wfile << "POS\n";
	wfile << std::to_string(lay->pos);
	wfile << "\n";
	wfile << "TYPE\n";
	wfile << std::to_string(lay->type);
	wfile << "\n";
    wfile << "CLONESETNR\n";
    wfile << std::to_string(lay->clonesetnr);
    wfile << "\n";
	wfile << "FILENAME\n";
	wfile << lay->filename;
	wfile << "\n";
	if (lay->type != ELEM_LIVE) {
		wfile << "RELPATH\n";
		if (lay->filename != "") {
			wfile << std::filesystem::relative(lay->filename, mainprogram->contentpath).generic_string();
		}
		else {
			wfile << lay->filename;
		}
		wfile << "\n";
		if (lay->filename != "") {
			wfile << "FILESIZE\n";
			wfile << std::to_string(std::filesystem::file_size(lay->filename));
			wfile << "\n";
		}
	}
	if (lay->filename != "") {
        int sw2 = 0;
        int sh2 = 0;
        if (lay->type == ELEM_IMAGE) {
            sw2 = ilGetInteger(IL_IMAGE_WIDTH);
            sh2 = ilGetInteger(IL_IMAGE_HEIGHT);
        } else if (lay->video_dec_ctx) {
            sw2 = lay->video_dec_ctx->width;
            sh2 = lay->video_dec_ctx->height;
        };
        wfile << "WIDTH\n";
		wfile << std::to_string(sw2);
		wfile << "\n";
		wfile << "HEIGHT\n";
		wfile << std::to_string(sh2);
		wfile << "\n";
	}
	if (lay->node->vidbox && dojpeg && lay->filename != "") {
		std::string jpegpath = find_unused_filename(basename(lay->filename), mainprogram->temppath, ".jpg");
        lay->minitex = copy_tex(lay->node->vidbox->tex, 192, 108);
		save_thumb(jpegpath, lay->minitex);
		jpegpaths.push_back(jpegpath);
		wfile << "JPEGPATH\n";
		wfile << jpegpath;
		wfile << "\n";
	}
	wfile << "ASPECTRATIO\n";
	wfile << std::to_string(lay->aspectratio);
	wfile << "\n";
    wfile << "QUEUEING\n";
    wfile << std::to_string(lay->queueing);
    wfile << "\n";
    wfile << "MUTE\n";
    wfile << std::to_string(lay->mutebut->value);
    wfile << "\n";
	wfile << "MUTEEVENT\n";
	mainmix->event_write(wfile, nullptr, lay->mutebut);
    wfile << "SOLO\n";
    wfile << std::to_string(lay->solobut->value);
    wfile << "\n";
    wfile << "SOLOEVENT\n";
    mainmix->event_write(wfile, nullptr, lay->solobut);
    wfile << "KEEPEFF\n";
    wfile << std::to_string(lay->keepeffbut->value);
    wfile << "\n";
    wfile << "KEEPEFFEVENT\n";
    mainmix->event_write(wfile, nullptr, lay->keepeffbut);
	if (lay->type != ELEM_LIVE) {
		wfile << "SPEEDVAL\n";
		wfile << std::to_string(lay->speed->value);
		wfile << "\n";
		wfile << "SPEEDEVENT\n";
		mainmix->event_write(wfile, lay->speed, nullptr);
		wfile << "PLAYBUTVAL\n";
		wfile << std::to_string(lay->playbut->value);;
		wfile << "\n";
		wfile << "PLAYBUTEVENT\n";
		mainmix->event_write(wfile, nullptr, lay->playbut);
		wfile << "REVBUTVAL\n";
		wfile << std::to_string(lay->revbut->value);;
		wfile << "\n";
		wfile << "REVBUTEVENT\n";
		mainmix->event_write(wfile, nullptr, lay->revbut);
        wfile << "BOUNCEBUTVAL\n";
        wfile << std::to_string(lay->bouncebut->value);;
        wfile << "\n";
        wfile << "BOUNCEBUTEVENT\n";
        mainmix->event_write(wfile, nullptr, lay->bouncebut);
        wfile << "LPBUTVAL\n";
        wfile << std::to_string(lay->lpbut->value);;
        wfile << "\n";
        wfile << "LPBUTEVENT\n";
        mainmix->event_write(wfile, nullptr, lay->lpbut);
		wfile << "PLAYKIND\n";
		wfile << std::to_string(lay->playkind);;
		wfile << "\n";
		wfile << "GENMIDIBUTVAL\n";
		wfile << std::to_string(lay->genmidibut->value);;
		wfile << "\n";
		wfile << "GENMIDIBUTEVENT\n";
		mainmix->event_write(wfile, nullptr, lay->genmidibut);
		wfile << "FRAMEFORWARDEVENT\n";
		mainmix->event_write(wfile, nullptr, lay->frameforward);
		wfile << "FRAMEBACKWARDEVENT\n";
		mainmix->event_write(wfile, nullptr, lay->framebackward);
	}
    wfile << "LOCKSPEED\n";
    wfile << std::to_string(lay->lockspeed);
    wfile << "\n";
    wfile << "LOCKZOOMPAN\n";
    wfile << std::to_string(lay->lockzoompan);
    wfile << "\n";
    wfile << "SHIFTX\n";
    wfile << std::to_string(lay->shiftx->value);
    wfile << "\n";
	wfile << "SHIFTXEVENT\n";
	mainmix->event_write(wfile, lay->shiftx, nullptr);
	wfile << "SHIFTY\n";
	wfile << std::to_string(lay->shifty->value);
	wfile << "\n";
	wfile << "SHIFTYEVENT\n";
	mainmix->event_write(wfile, lay->shifty, nullptr);
	wfile << "SCALE\n";
	wfile << std::to_string(lay->scale->value);
	wfile << "\n";
    wfile << "SCALEEVENT\n";
    mainmix->event_write(wfile, lay->scale, nullptr);
    wfile << "SCRITCHEVENT\n";
    mainmix->event_write(wfile, lay->scritch, nullptr);
	wfile << "OPACITYVAL\n";
	wfile << std::to_string(lay->opacity->value);
	wfile << "\n";
	wfile << "OPACITYEVENT\n";
	mainmix->event_write(wfile, lay->opacity, nullptr);
	wfile << "VOLUMEVAL\n";
	wfile << std::to_string(lay->volume->value);
	wfile << "\n";
    wfile << "VOLUMEEVENT\n";
    mainmix->event_write(wfile, lay->volume, nullptr);
    wfile << "MIXFACEVENT\n";
    mainmix->event_write(wfile, lay->blendnode->mixfac, nullptr);
	wfile << "CHTOLVAL\n";
	wfile << std::to_string(lay->chtol->value);
	wfile << "\n";
	wfile << "CHDIRVAL\n";
	wfile << std::to_string(lay->chdir->value);
	wfile << "\n";
	wfile << "CHINVVAL\n";
	wfile << std::to_string(lay->chinv->value);
	wfile << "\n";
	if (lay->type != ELEM_LIVE) {
		wfile << "FRAME\n";
		wfile << std::to_string(lay->frame);
		wfile << "\n";
		wfile << "STARTFRAME\n";
		wfile << std::to_string(lay->startframe->value);
		wfile << "\n";
        wfile << "ENDFRAME\n";
        wfile << std::to_string(lay->endframe->value);
        wfile << "\n";
        wfile << "MILLIF\n";
        wfile << std::to_string(lay->millif);
        wfile << "\n";
	}
	wfile << "BLENDTYPE\n";
	wfile << std::to_string(lay->blendnode->blendtype);
	wfile << "\n";
	wfile << "MIXMODE\n";
	wfile << std::to_string(lay->blendnode->blendtype);
	wfile << "\n";
	wfile << "MIXFACVAL\n";
	wfile << std::to_string(lay->blendnode->mixfac->value);
	wfile << "\n";
	wfile << "CHRED\n";
	wfile << std::to_string(lay->blendnode->chred);
	wfile << "\n";
	wfile << "CHGREEN\n";
	wfile << std::to_string(lay->blendnode->chgreen);
	wfile << "\n";
	wfile << "CHBLUE\n";
	wfile << std::to_string(lay->blendnode->chblue);
	wfile << "\n";
	wfile << "WIPETYPE\n";
	wfile << std::to_string(lay->blendnode->wipetype);
	wfile << "\n";
	wfile << "WIPEDIR\n";
	wfile << std::to_string(lay->blendnode->wipedir);
	wfile << "\n";
	wfile << "WIPEX\n";
	wfile << std::to_string(lay->blendnode->wipex->value);
	wfile << "\n";
    wfile << "WIPEXEVENT\n";
    mainmix->event_write(wfile, lay->blendnode->wipex, nullptr);
	wfile << "WIPEY\n";
	wfile << std::to_string(lay->blendnode->wipey->value);
	wfile << "\n";
    wfile << "WIPEYEVENT\n";
    mainmix->event_write(wfile, lay->blendnode->wipey, nullptr);

	if (doclips && lay->clips.size()) {
		wfile << "CLIPS\n";
		int size = std::clamp((int)(lay->clips.size() - 1), 0, (int)(lay->clips.size() - 1));
		for (int j = 0; j < size; j++) {
			Clip* clip = lay->clips[j];
			wfile << "TYPE\n";
            wfile << std::to_string(clip->type);
			wfile << "\n";
			wfile << "FILENAME\n";
			wfile << clip->path;
			wfile << "\n";
            wfile << "RELPATH\n";
            if (clip->path != "") {
                wfile << std::filesystem::relative(clip->path, mainprogram->contentpath).generic_string();
            }
            else {
                wfile << clip->path;
            }
            wfile << "\n";
            if (clip->path != "") {
                wfile << "FILESIZE\n";
                wfile << std::to_string(std::filesystem::file_size(clip->path));
                wfile << "\n";
            }
			if (clip->type == ELEM_LAYER && !isvideo(clip->path)) {
				wfile << "CLIPLAYER\n";
				std::ifstream rfile;
				std::string result = deconcat_files(clip->path);
				if (result != "") rfile.open(result);
				else rfile.open(clip->path);
				std::string istring;
				while (safegetline(rfile, istring)) {
					wfile << istring;
					wfile << "\n";
					if (istring == "ENDOFFILE") break;
				}
				wfile << "ENDOFCLIPLAYER\n";
			}
            if (clip->jpegpath != "") {
                jpegpaths.push_back(clip->jpegpath);
            }
            else {
                clip->jpegpath = find_unused_filename(basename(clip->path), mainprogram->temppath, ".jpg");
                save_thumb(clip->jpegpath, clip->tex);
                jpegpaths.push_back(clip->jpegpath);
            }
			wfile << "JPEGPATH\n";
			wfile << clip->jpegpath;
			wfile << "\n";
			wfile << "FRAME\n";
			wfile << std::to_string(clip->frame);
			wfile << "\n";
			wfile << "STARTFRAME\n";
			wfile << std::to_string(clip->startframe->value);
			wfile << "\n";
			wfile << "ENDFRAME\n";
			wfile << std::to_string(clip->endframe->value);
			wfile << "\n";
		}
		wfile << "ENDOFCLIPS\n";
	}

	wfile << "EFFECTS\n";
	for (int j = 0; j < lay->effects[0].size(); j++) {
		Effect* eff = lay->effects[0][j];
		wfile << "TYPE\n";
		wfile << std::to_string(eff->type);
		wfile << "\n";
		wfile << "POS\n";
		wfile << std::to_string(j);
		wfile << "\n";
		wfile << "ONOFFVAL\n";
		wfile << std::to_string(eff->onoffbutton->value);
		wfile << "\n";
		wfile << "ONOFFMIDI0\n";
		wfile << std::to_string(eff->onoffbutton->midi[0]);
		wfile << "\n";
		wfile << "ONOFFMIDI1\n";
		wfile << std::to_string(eff->onoffbutton->midi[1]);
		wfile << "\n";
		wfile << "ONOFFMIDIPORT\n";
		wfile << eff->onoffbutton->midiport;
		wfile << "\n";
		wfile << "ONOFFEVENT\n";
		mainmix->event_write(wfile, nullptr, eff->onoffbutton);
		wfile << "DRYWETVAL\n";
		wfile << std::to_string(eff->drywet->value);
		wfile << "\n";
		wfile << "DRYWETMIDI0\n";
		wfile << std::to_string(eff->drywet->midi[0]);
		wfile << "\n";
		wfile << "DRYWETMIDI1\n";
		wfile << std::to_string(eff->drywet->midi[1]);
		wfile << "\n";
		wfile << "DRYWETMIDIPORT\n";
		wfile << eff->drywet->midiport;
		wfile << "\n";
		if (eff->type == RIPPLE) {
			wfile << "RIPPLESPEED\n";
			wfile << std::to_string(((RippleEffect*)eff)->speed);
			wfile << "\n";
		}
		if (eff->type == RIPPLE) {
			wfile << "RIPPLECOUNT\n";
			wfile << std::to_string(((RippleEffect*)eff)->ripplecount);
			wfile << "\n";
		}

		wfile << "PARAMS\n";
		for (int k = 0; k < eff->params.size(); k++) {
			Param* par = eff->params[k];
			wfile << "VAL\n";
			wfile << std::to_string(par->value);
			wfile << "\n";
			wfile << "MIDI0\n";
			wfile << std::to_string(par->midi[0]);
			wfile << "\n";
			wfile << "MIDI1\n";
			wfile << std::to_string(par->midi[1]);
			wfile << "\n";
			wfile << "MIDIPORT\n";
			wfile << par->midiport;
			wfile << "\n";
			mainmix->event_write(wfile, par, nullptr);
		}
		wfile << "ENDOFPARAMS\n";
	}
	wfile << "ENDOFEFFECTS\n";

	wfile << "STREAMEFFECTS\n";
	for (int j = 0; j < lay->effects[1].size(); j++) {
		Effect* eff = lay->effects[1][j];
		wfile << "TYPE\n";
		wfile << std::to_string(eff->type);
		wfile << "\n";
		wfile << "POS\n";
		wfile << std::to_string(j);
		wfile << "\n";
		wfile << "ONOFFVAL\n";
		wfile << std::to_string(eff->onoffbutton->value);
		wfile << "\n";
		wfile << "DRYWETVAL\n";
		wfile << std::to_string(eff->drywet->value);
		wfile << "\n";
		if (eff->type == RIPPLE) {
			wfile << "RIPPLESPEED\n";
			wfile << std::to_string(((RippleEffect*)eff)->speed);
			wfile << "\n";
		}
		if (eff->type == RIPPLE) {
			wfile << "RIPPLECOUNT\n";
			wfile << std::to_string(((RippleEffect*)eff)->ripplecount);
			wfile << "\n";
		}

		wfile << "PARAMS\n";
		for (int k = 0; k < eff->params.size(); k++) {
			Param* par = eff->params[k];
			wfile << "VAL\n";
			wfile << std::to_string(par->value);
			wfile << "\n";
			wfile << "MIDI0\n";
			wfile << std::to_string(par->midi[0]);
			wfile << "\n";
			wfile << "MIDI1\n";
			wfile << std::to_string(par->midi[1]);
			wfile << "\n";
			wfile << "MIDIPORT\n";
			wfile << par->midiport;
			wfile << "\n";
			mainmix->event_write(wfile, par, nullptr);
		}
		wfile << "ENDOFPARAMS\n";
	}
	wfile << "ENDOFSTREAMEFFECTS\n";

	wfile << "EFFCAT\n";
	wfile << std::to_string(mainprogram->effcat[lay->deck]->value);
	wfile << "\n";

	return jpegpaths;
}

void Mixer::do_delete_layers(std::vector<Layer*> lrs, bool alive) {
	for (int i = 0; i < lrs.size(); i++) {
		Layer* lay = lrs[lrs.size() - i - 1];
		if (alive && std::find(mainprogram->busylayers.begin(), mainprogram->busylayers.end(), lay) == mainprogram->busylayers.end()) {
			mainmix->delete_layer(lrs, lay, false);
		}
		else {
			lay->inhibit();
		}
	}
}

void Mixer::delete_layers(std::vector<Layer*>& layers, bool alive) {
	this->do_delete_layers(layers, alive);
	//std::thread lrsdel(&Mixer::do_delete_layers, this, layers, alive);
	//lrsdel.detach();
}


					// WORKING WITH EVENTS

void Mixer::event_write(std::ostream &wfile, Param* par, Button* but) {
	for (int i = 0; i < loopstation->elems.size(); i++) {
		LoopStationElement *elem = loopstation->elems[i];
		if (par) {
			if (std::find(elem->params.begin(), elem->params.end(), par) != elem->params.end()) {
				wfile << "EVENTELEM\n";
				wfile << std::to_string(i);
				wfile << "\n";
				wfile << std::to_string(elem->loopbut->value);
				wfile << "\n";
				wfile << std::to_string(elem->playbut->value);
				wfile << "\n";
				wfile << std::to_string(elem->speed->value);
				wfile << "\n";
				for (int j = 0; j < elem->eventlist.size(); j++) {
					if (par == std::get<1>(elem->eventlist[j])) {
						wfile << std::to_string(std::get<0>(elem->eventlist[j]));
						wfile << "\n";
						wfile << std::to_string(std::get<3>(elem->eventlist[j]));
						wfile << "\n";
					}
				}
				wfile << "ENDOFEVENT\n";
                wfile << "TOTALTIME\n";
                wfile << std::to_string(elem->totaltime);
                wfile << "\n";
                std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> elapsed;
                elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - elem->starttime);
                long long millicount = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
                int passed = millicount - elem->interimtime;
                elem->interimtime = millicount;
                elem->speedadaptedtime = elem->speedadaptedtime + passed * elem->speed->value;
                wfile << "INTERIMTIME\n";
                wfile << std::to_string(elem->interimtime);
                wfile << "\n";
                wfile << "SPEEDADAPTEDTIME\n";
                wfile << std::to_string(elem->speedadaptedtime);
                wfile << "\n";
            }
		}
		else if (but) {
			if (std::find(elem->buttons.begin(), elem->buttons.end(), but) != elem->buttons.end()) {
				wfile << "EVENTELEM\n";
				wfile << std::to_string(i);
				wfile << "\n";
				wfile << std::to_string(elem->loopbut->value);
				wfile << "\n";
				wfile << std::to_string(elem->playbut->value);
				wfile << "\n";
				wfile << std::to_string(elem->speed->value);
				wfile << "\n";
				for (int j = 0; j < elem->eventlist.size(); j++) {
					if (but == std::get<2>(elem->eventlist[j])) {
						wfile << std::to_string(std::get<0>(elem->eventlist[j]));
						wfile << "\n";
						wfile << std::to_string(std::get<3>(elem->eventlist[j]));
						wfile << "\n";
					}
				}
				wfile << "ENDOFEVENT\n";
                wfile << "TOTALTIME\n";
                wfile << std::to_string(elem->totaltime);
                wfile << "\n";
                std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> elapsed;
                elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - elem->starttime);
                long long millicount = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
                int passed = millicount - elem->interimtime;
                elem->interimtime = millicount;
                elem->speedadaptedtime = elem->speedadaptedtime + passed * elem->speed->value;
                wfile << "INTERIMTIME\n";
                wfile << std::to_string(elem->interimtime);
                wfile << "\n";
                wfile << "SPEEDADAPTEDTIME\n";
                wfile << std::to_string(elem->speedadaptedtime);
                wfile << "\n";
			}
		}
	}
}

void Mixer::event_read(std::istream &rfile, Param *par, Button* but, Layer *lay) {
	// load loopstation events for this parameter
	std::string istring;
	LoopStationElement *loop = nullptr;
	safegetline(rfile, istring);
	int elemnr = std::stoi(istring);
	LoopStation *ls;
	if (mainprogram->prevmodus) ls = lp;
	else ls = lpc;
	if (std::find(ls->readelemnrs.begin(), ls->readelemnrs.end(), elemnr) == ls->readelemnrs.end()) {
		// new loopstation line taken in use at location elemnr
		loop = ls->elems[elemnr];
		ls->readelemnrs.push_back(elemnr);
		ls->readelems.push_back(loop);
	}
	else {
		// other parameter(s) of this rfile using this loopstation line already exist
		loop = ls->free_element();
	}
    if (loop) {
        if (par){
            loop->params.emplace(par);
        }
        if (but){
            loop->buttons.emplace(but);
        }
    }
	safegetline(rfile, istring);
	if (loop) {
		loop->loopbut->value = std::stoi(istring);
		loop->loopbut->oldvalue = std::stoi(istring);
		if (loop->loopbut->value) {
			loop->eventpos = 0;
			loop->starttime = std::chrono::high_resolution_clock::now();
		}
	}
	safegetline(rfile, istring);
	if (loop) {
		loop->playbut->value = std::stoi(istring);
		loop->playbut->oldvalue = std::stoi(istring);
		if (loop->playbut->value) {
			loop->eventpos = 0;
			loop->starttime = std::chrono::high_resolution_clock::now();
		}
	}
	safegetline(rfile, istring);
	if (loop) {
		loop->speed->value = std::stof(istring);
		if (lay) loop->layers.emplace(lay);
	}
	while (safegetline(rfile, istring)) {
		if (istring == "ENDOFEVENT") break;
		int time = std::stoi(istring);
		safegetline(rfile, istring);
		float value = std::stof(istring);
		bool inserted = false;
		if (loop) {
			for (int i = 0; i < loop->eventlist.size(); i++) {
				if (time < std::get<0>(loop->eventlist[i])) {
					loop->eventlist.insert(loop->eventlist.begin() + i, std::make_tuple(time, par, but, value));
					inserted = true;
					break;
				}
			}

			if (!inserted) {
				loop->eventlist.push_back(std::make_tuple(time, par, but, value));
			}
			if (par) {
				loop->params.emplace(par);
				ls->parelemmap[par] = loop;
				if (par->effect) {
					loop->layers.emplace(par->effect->layer);
				}
				par->box->acolor[0] = loop->colbox->acolor[0];
				par->box->acolor[1] = loop->colbox->acolor[1];
				par->box->acolor[2] = loop->colbox->acolor[2];
				par->box->acolor[3] = loop->colbox->acolor[3];
			}
			else if (but) {
				loop->buttons.emplace(but);
				ls->butelemmap[but] = loop;
				if (but->layer) {
					loop->layers.emplace(but->layer);
				}
				but->box->acolor[0] = loop->colbox->acolor[0];
				but->box->acolor[1] = loop->colbox->acolor[1];
				but->box->acolor[2] = loop->colbox->acolor[2];
				but->box->acolor[3] = loop->colbox->acolor[3];
			}
		}
	}
	if (loop) {
        safegetline(rfile, istring);
        if (istring == "TOTALTIME") {
            safegetline(rfile, istring);
            loop->totaltime = std::stof(istring);
        }
        safegetline(rfile, istring);
        if (istring == "INTERIMTIME") {
            safegetline(rfile, istring);
            loop->interimtime = std::stof(istring);
            std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
            loop->starttime = now - std::chrono::milliseconds((long long)loop->interimtime);
        }
        safegetline(rfile, istring);
        if (istring == "SPEEDADAPTEDTIME") {
            safegetline(rfile, istring);
            loop->speedadaptedtime = std::stof(istring);
        }
    }
}


					// NEW

void Mixer::new_file(int decks, bool alive, bool add, bool empty) {
	// kill mixnodes[0]
	//mainprogram->nodesmain->mixnodes[0].clear();
	//mainprogram->nodesmain->mixnodes[1].clear();
	
	// reset layers if bool empty
    bool currdeck = 0;
	if (mainmix->currlay[!mainprogram->prevmodus]) currdeck = mainmix->currlay[!mainprogram->prevmodus]->deck;
	mainmix->bualive = alive;
	bool pm[2];
	int ns = 1;
	if (decks == 3) {
		ns = 2;
		pm[0] = !mainprogram->prevmodus;
		pm[1] = mainprogram->prevmodus;
	}
	else pm[0] = mainprogram->prevmodus;
	for (int m = 0; m < ns; m++) {
		// mimiclayers that will be overwritten should not be promoted to busylayer
		if (decks == 0 || decks >= 2) {
			std::vector<Layer*>& lvec = choose_layers(0);
			for (int i = 0; i < lvec.size(); i++) {
				ptrdiff_t pos = std::find(mainprogram->mimiclayers.begin(), mainprogram->mimiclayers.end(), lvec[i]) - mainprogram->mimiclayers.begin();
				if (pos < mainprogram->mimiclayers.size()) {
					mainprogram->mimiclayers.erase(mainprogram->mimiclayers.begin() + pos);
					lvec[i]->liveinput = nullptr;
				}
			}
		}
		if (decks == 1 || decks >= 2) {
			std::vector<Layer*>& lvec = choose_layers(1);
			for (int i = 0; i < lvec.size(); i++) {
				ptrdiff_t pos = std::find(mainprogram->mimiclayers.begin(), mainprogram->mimiclayers.end(), lvec[i]) - mainprogram->mimiclayers.begin();
				if (pos < mainprogram->mimiclayers.size()) {
					mainprogram->mimiclayers.erase(mainprogram->mimiclayers.begin() + pos);
					lvec[i]->liveinput = nullptr;
				}
			}
		}
	}
	for (int m = 0; m < ns; m++) {
		mainprogram->prevmodus = pm[m];
		if (decks == 0 || decks >= 2) {
			std::vector<Layer*> &lvec = choose_layers(0);
			//mainmix->butexes[0].clear();
			for (int i = 0; i < lvec.size(); i++) {
				ptrdiff_t pos = std::find(mainprogram->busylayers.begin(), mainprogram->busylayers.end(), lvec[i]) - mainprogram->busylayers.begin();
				if (pos < mainprogram->busylayers.size()) {
					avformat_close_input(&mainprogram->busylayers[pos]->video);
					bool found = mainprogram->busylayers[pos]->find_new_live_base(pos);
					if (!found) {
						mainprogram->busylist.erase(mainprogram->busylist.begin() + pos);
						mainprogram->busylayers.erase(mainprogram->busylayers.begin() + pos);
					}
				}
				lvec[i]->mutebut->value = false;
				lvec[i]->mute_handle();
				/*GLuint butex;
				if (lvec[i]->effects[0].size() == 0) {
					butex = lvec[i]->fbotex;
				}
				else {
					butex = lvec[i]->effects[0][lvec[i]->effects[0].size() - 1]->fbotex;
				}
				int sw, sh;
				glBindTexture(GL_TEXTURE_2D, butex);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &sw);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &sh);
				mainmix->butexes[0].push_back(copy_tex(butex, sw, sh));*/
			}
			if (alive) mainmix->bulrs[mainprogram->prevmodus][0] = lvec;
			else {
                mainmix->bulrs[mainprogram->prevmodus][0].clear();
            }
            if (empty) {
                lvec.clear();
                Layer *lay;
                if (add) {
                    lay = mainmix->add_layer(lvec, 0);
                    if (mainprogram->prevmodus) {
                        mainprogram->nodesmain->currpage->connect_nodes(lvec[lvec.size() - 1]->lasteffnode[0],
                                                                        mainprogram->nodesmain->mixnodes[0][0]);
                    } else {
                        mainprogram->nodesmain->currpage->connect_nodes(lvec[lvec.size() - 1]->lasteffnode[0],
                                                                        mainprogram->nodesmain->mixnodes[1][0]);
                    }
                    if (currdeck == 0) mainmix->currlay[!mainprogram->prevmodus] = lay;
                }
                if (mainprogram->prevmodus) {
                    mainprogram->nodesmain->mixnodes[0][0]->newmixfbo = true;
                } else {
                    mainprogram->nodesmain->mixnodes[1][0]->newmixfbo = true;
                }
                mainmix->scenes[0][mainmix->currscene[0]]->scrollpos = 0;
            }
		}
		if (decks == 1 || decks >= 2) {
            std::vector<Layer*> &lvec = choose_layers(1);
			//mainmix->butexes[1].clear();
			for (int i = 0; i < lvec.size(); i++) {
				ptrdiff_t pos = std::find(mainprogram->busylayers.begin(), mainprogram->busylayers.end(), lvec[i]) - mainprogram->busylayers.begin();
				if (pos < mainprogram->busylayers.size()) {
					avformat_close_input(&mainprogram->busylayers[pos]->video);
					bool found = mainprogram->busylayers[pos]->find_new_live_base(pos);
					if (!found) {
						mainprogram->busylist.erase(mainprogram->busylist.begin() + pos);
						mainprogram->busylayers.erase(mainprogram->busylayers.begin() + pos);
					}
				}
				lvec[i]->mutebut->value = false;
				lvec[i]->mute_handle();
				/*GLuint butex;
				if (lvec[i]->effects[0].size() == 0) {
					butex = lvec[i]->fbotex;
				}
				else {
					butex = lvec[i]->effects[0][lvec[i]->effects[0].size() - 1]->fbotex;
				}
				int sw, sh;
				glBindTexture(GL_TEXTURE_2D, butex);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &sw);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &sh);
				mainmix->butexes[1].push_back(copy_tex(butex, sw, sh));*/
			}
			if (alive) mainmix->bulrs[mainprogram->prevmodus][1] = lvec;
			else {
                mainmix->bulrs[mainprogram->prevmodus][1].clear();
            }
            if (empty) {
                lvec.clear();
                Layer *lay;
                if (add) {
                    lay = mainmix->add_layer(lvec, 0);
                    if (mainprogram->prevmodus) {
                        mainprogram->nodesmain->currpage->connect_nodes(lvec[lvec.size() - 1]->lasteffnode[0],
                                                                        mainprogram->nodesmain->mixnodes[0][1]);
                    } else {
                        mainprogram->nodesmain->currpage->connect_nodes(lvec[lvec.size() - 1]->lasteffnode[0],
                                                                        mainprogram->nodesmain->mixnodes[1][1]);
                    }
                    if (currdeck == 1) mainmix->currlay[!mainprogram->prevmodus] = lay;
                }

                if (mainprogram->prevmodus) {
                    mainprogram->nodesmain->mixnodes[0][1]->newmixfbo = true;
                } else {
                    mainprogram->nodesmain->mixnodes[1][1]->newmixfbo = true;
                }
                mainmix->scenes[1][mainmix->currscene[1]]->scrollpos = 0;
            }
		}
		if (decks >= 2) {
			if (mainprogram->prevmodus) {
				mainprogram->nodesmain->mixnodes[0][2]->newmixfbo = true;
			}
			else {
				mainprogram->nodesmain->mixnodes[1][2]->newmixfbo = true;
			}
		}
	}
}





// OUTPUT RECORDING TO VIDEO

void Mixer::record_video(std::string reccod) {
    bool cbool;
    if (this->reckind == 0) {
        if (!mainprogram->prevmodus) {
            cbool = 1;
        }
        else {
            cbool = 0;
        }
    }
    else {
        cbool = 1;
    }
	AVFormatContext* dest = avformat_alloc_context();
	AVStream* dest_stream;
	const AVCodec *codec = nullptr;
    AVCodecContext *c = nullptr;
    int i, ret, x, y;
    AVFrame *frame;
    AVPacket *pkt;
    uint8_t endcode[] = { 0, 0, 1, 0xb7 };
    codec = avcodec_find_encoder_by_name(reccodec.c_str());
    c = avcodec_alloc_context3(codec);
    pkt = av_packet_alloc();
    if (cbool) c->width = mainprogram->ow;
    else c->width = mainprogram->ow3;
	int rem = c->width % 32;
	c->width = c->width + (32 - rem) * (rem > 0);
    if (cbool) c->height = mainprogram->oh;
    else c->height = mainprogram->oh3;
	rem = c->height % 4;
	c->height = c->height + (4 - rem) * (rem > 0);
	/* frames per second */
    c->time_base = {1, 25 * (!this->reckind + 1)};
    c->framerate = {25, 1};
	c->pix_fmt = codec->pix_fmts[0];
    /* open it */
    ret = avcodec_open2(c, codec, nullptr);

    std::string path;
    if (this->reckind) {
        path = find_unused_filename("recording_0", mainprogram->project->recdir, "_hap.mov");
    }
    else {
        path = find_unused_filename(basename(this->reclay->filename), mainprogram->project->recdir, "_REC_hap.mov");
    }
    avformat_alloc_output_context2(&dest, av_guess_format("mov", nullptr, "video/mov"), nullptr, path.c_str());
    dest_stream = avformat_new_stream(dest, codec);
    dest_stream->time_base = {1, 25};
	int r = avcodec_parameters_from_context(dest_stream->codecpar, c);
	if (r < 0) {
		av_log(nullptr, AV_LOG_ERROR, "Copying stream context failed\n");
		av_log(nullptr, AV_LOG_ERROR, "Copying parameters for stream #%u failed\n");
	}
	dest_stream->r_frame_rate = c->framerate;
	dest_stream->avg_frame_rate = c->framerate;
	dest_stream->first_dts = 0;
    ((AVOutputFormat*)(dest->oformat))->flags |= AVFMT_NOFILE;
	//avformat_init_output(dest, nullptr);
	r = avio_open(&dest->pb, path.c_str(), AVIO_FLAG_WRITE);
	r = avformat_write_header(dest, nullptr);

	//ret = av_frame_get_buffer(frame, 32);
    //if (ret < 0) {
    //    fprintf(stderr, "Could not allocate the video frame data\n");
    //    exit(0);
    //}

	// Determine required buffer size and allocate buffer
    AVFrame *yuvframe = av_frame_alloc();
    yuvframe->format = c->pix_fmt;
    yuvframe->width  = c->width;
    yuvframe->height = c->height;

    struct SwsContext *sws_ctx = sws_getContext
    (
        c->width,
       	c->height,
        AV_PIX_FMT_BGRA,
        c->width,
        c->height,
        c->pix_fmt,
        SWS_BILINEAR,
        nullptr,
        nullptr,
        nullptr);
    
	/* record */
	int count = 0;
    this->reclay->checkre = true;
    bool started = false;
    while (this->recording[this->reckind]) {
        std::unique_lock<std::mutex> lock(this->recordlock[this->reckind]);
        this->startrecord[this->reckind].wait(lock, [&] { return this->recordnow[this->reckind]; });
        lock.unlock();
        this->recordnow[this->reckind] = false;
        if (this->recrep) {
            if (!started) {
                if (!this->reclay->recstarted) {
                    continue;
                }
                started = true;
            }
        }
		
		AVFrame *rgbaframe;
		rgbaframe = av_frame_alloc();
		rgbaframe->data[0] = (uint8_t *)mainmix->rgbdata;
		rgbaframe->format = c->pix_fmt;
		rgbaframe->width  = c->width;
		rgbaframe->height = c->height;
	 
		//av_image_fill_arrays(picture->data, picture->linesize,
                               // ptr, pix_fmt, width, height, 1);		avpicture_fill(&rgbaframe, (uint8_t *)mainmix->rgbdata, AV_PIX_FMT_BGRA, c->width, c->height);
        if (cbool) rgbaframe->linesize[0] = mainprogram->ow * 4;
        else rgbaframe->linesize[0] = mainprogram->ow3 * 4;
		int storage = av_image_alloc(yuvframe->data, yuvframe->linesize, yuvframe->width, yuvframe->height, c->pix_fmt, 32);
		sws_scale
		(
			sws_ctx,
			rgbaframe->data,
			rgbaframe->linesize,
			0,
			c->height,
			yuvframe->data,
			yuvframe->linesize
		);
		yuvframe->pts++;

        /* encode the image */
        encode_frame(dest, nullptr, c, yuvframe, pkt, nullptr, count);
		
		av_freep(rgbaframe->data);
		av_freep(yuvframe->data);
		av_packet_unref(pkt);
    	av_frame_free(&rgbaframe);

		count++;

        if (this->reclay->recended) {
            // break when recrepping at end of video
            this->reclay->recstarted = false;
            this->reclay->recended = false;
            this->reclay->checkre = false;
            break;
        }
	}
    /* flush the encoder */
 //   encode_frame(dest, nullptr, c, nullptr, pkt, nullptr, 0);
    /* add sequence end code to have a real MPEG file */
	dest_stream->duration = (count + 1) * av_rescale_q(1, c->time_base, dest_stream->time_base);
	av_write_trailer(dest);
	avio_close(dest->pb);
    avcodec_free_context(&c);
    av_frame_free(&yuvframe);
    av_packet_free(&pkt);

    this->donerec[this->reckind] = true;
    this->recordnow[this->reckind] = false;
    this->recswitch[this->reckind] = true;
    this->recpath[this->reckind] = path;

    if (this->recrep) {
        this->reclay->speed->value = 1.0f;
        this->reclay->playbut->value = 1;
        this->reclay->revbut->value = 0;
        this->reclay->bouncebut->value = 0;
        int bukebv = this->reclay->keepeffbut->value;
        this->reclay->keepeffbut->value = 0;
        this->reclay->open_video(0.0f, path, true, false);
        this->reclay->keepeffbut->value = bukebv;
        this->reclay->type = ELEM_FILE;
        this->reclay = nullptr;
        this->recrep = false;
    }
}

void Mixer::start_recording() {
	// start recording main output
    std::string reccod = this->reccodec;
    this->donerec[this->reckind] = false;
    this->recording[this->reckind] = true;
	// recording is done in separate low priority thread
	this->recording_video[this->reckind] = std::thread{&Mixer::record_video, this, reccod};
	#ifdef WINDOWS
	SetThreadPriority((void*)this->recording_video[this->reckind].native_handle(), THREAD_PRIORITY_LOWEST);
	#else
	struct sched_param params;
	params.sched_priority = sched_get_priority_min(SCHED_FIFO);
    pthread_setschedparam(this->recording_video[this->reckind].native_handle(), SCHED_FIFO, &params);
	#endif
	this->recording_video[this->reckind].detach();
	#define BUFFER_OFFSET(i) ((char *)nullptr + (i))
	glBindBuffer(GL_PIXEL_PACK_BUFFER, this->ioBuf);
    bool cbool;
    if (this->reckind == 0) {
        if (!mainprogram->prevmodus) {
            glBufferData(GL_PIXEL_PACK_BUFFER, (int)(mainprogram->ow * mainprogram->oh) * 4, nullptr, GL_DYNAMIC_READ);
            cbool = 1;
        }
        else {
            glBufferData(GL_PIXEL_PACK_BUFFER, (int)(mainprogram->ow3 * mainprogram->oh3) * 4, nullptr, GL_DYNAMIC_READ);
            cbool = 0;
        }
        if (this->reclay->effects[0].size() == 0) {
            glBindFramebuffer(GL_FRAMEBUFFER, this->reclay->fbo);
        }
        else {
            glBindFramebuffer(GL_FRAMEBUFFER, this->reclay->effects[0][this->reclay->effects[0].size() - 1]->fbo);
        }
    }
    else {
        cbool = 1;
        glBufferData(GL_PIXEL_PACK_BUFFER, (int)(mainprogram->ow * mainprogram->oh) * 4, nullptr, GL_DYNAMIC_READ);
        glBindFramebuffer(GL_FRAMEBUFFER, ((MixNode *) mainprogram->nodesmain->mixnodes[1][2])->mixfbo);
    }
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    if (cbool) glReadPixels(0, 0, mainprogram->ow, (int)mainprogram->oh, GL_BGRA, GL_UNSIGNED_BYTE, BUFFER_OFFSET(0));
    else glReadPixels(0, 0, mainprogram->ow3, (int)mainprogram->oh3, GL_BGRA, GL_UNSIGNED_BYTE, BUFFER_OFFSET(0));
	this->rgbdata = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
	//assert(this->rgbdata);
    if (this->reckind) {
        this->recordnow[this->reckind] = true;
        while (this->recordnow[this->reckind]) {
            this->startrecord[this->reckind].notify_all();
        }
    }

	// make a thumbnail for display afterwards
    if (this->reckind && !this->recordnow[this->reckind]) {
        if (this->recQthumb != -1) glDeleteTextures(1, &this->recQthumb);
        this->recQthumb = copy_tex(mainprogram->nodesmain->mixnodes[1][2]->mixtex);
    }
}


// CLIP QUEUE

void Mixer::clip_inside_test(std::vector<Layer*>& layers, bool deck) {
	// test if inside a clip
	Layer* lay;
	mainprogram->inclips = -1;
	for (int i = 0; i < layers.size(); i++) {
		lay = layers[i];
		if (lay->pos < mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos || lay->pos > mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos + 2) continue;
		Boxx* box = lay->node->vidbox;
		if (lay->queueing) {
			if (box->scrcoords->x1 < mainprogram->mx && mainprogram->mx < box->scrcoords->x1 + box->scrcoords->w) {
				int limit = lay->clips.size() - lay->queuescroll;
				if (limit > 4) limit = 4;
				for (int j = 0; j < limit; j++) {
					Clip* jc = lay->clips[j + lay->queuescroll];
					jc->box->vtxcoords->x1 = box->vtxcoords->x1;
					jc->box->vtxcoords->y1 = box->vtxcoords->y1 - (j + 1) * box->vtxcoords->h - 0.125f;
					jc->box->vtxcoords->w = box->vtxcoords->w;
					jc->box->vtxcoords->h = box->vtxcoords->h;
					jc->box->upvtxtoscr();
					if (jc->box->in()) {
						if (mainprogram->menuactivation || mainprogram->lpstmenuon) {
                            mainprogram->lpstmenu->state = 0;
                            mainprogram->parammenu3->state = 0;
                            mainprogram->parammenu4->state = 0;
                            mainprogram->lpstmenuon = false;
							mainprogram->clipmenu->state = 2;
							mainmix->mouseclip = jc;
							mainmix->mouselayer = lay;
							mainprogram->menuactivation = false;
						}
						else mainprogram->inclips = i;
						break;
					}
				}
			}
		}
	}
}

void Mixer::handle_clips() {
    Boxx borderclipbox;
    Boxx insideclipbox;
    Boxx clipbox;
    mainprogram->frontbatch = true;
    bool startdrag = false;
	for (int i = 0; i < 2; i++) {
		std::vector<Layer*>& lays = choose_layers(i);
		for (int j = 0; j < lays.size(); j++) {
			Layer* lay2 = lays[j];
			Boxx *vbox = lay2->node->vidbox;
			if (lay2->queueing) {
				// drawing clip queue
				int sc = this->scenes[lay2->deck][this->currscene[lay2->deck]]->scrollpos;
				int max = 4;
				if (mainprogram->dragclip) {
					if (mainprogram->inshelf == 0 && (lay2->pos == sc || lay2->pos == sc + 1) && lay2->deck == 0) {
						max = 3;
					}
					if (mainprogram->inshelf == 1 && (lay2->pos == sc + 1 || lay2->pos == sc + 2) && lay2->deck == 1) {
						max = 3;
					}
				}
				if (max > lay2->clips.size()) max = lay2->clips.size();
				int until = lay2->clips.size() - lay2->queuescroll;
				if (until > max) until = max;
				// draw cliploopbox for following layer play
                lay2->cliploopbox->vtxcoords->x1 = vbox->vtxcoords->x1;
                lay2->cliploopbox->vtxcoords->y1 = vbox->vtxcoords->y1 - 0.075f - 0.05f;
                lay2->cliploopbox->vtxcoords->w = mainprogram->layw;
                lay2->cliploopbox->vtxcoords->h = lay2->loopbox->vtxcoords->h;
                draw_box(lay2->cliploopbox, -1);
                draw_box(white, white, lay2->cliploopbox->vtxcoords->x1 + lay2->frame * (lay2->cliploopbox->vtxcoords->w /
                                                                                     (lay2->numf - 1)),
                         lay2->cliploopbox->vtxcoords->y1, 0.00117f, 0.075f, -1);
                if (lay2->cliploopbox->in()) {
                    if (mainprogram->leftmousedown) {
                        lay2->clipscritching = 1;
                        lay2->frame = (lay2->numf - 1.0f) *
                                      ((mainprogram->mx - lay2->cliploopbox->scrcoords->x1) / lay2->cliploopbox->scrcoords->w);
                        lay2->set_clones();
                    }
                }
                if (lay2->clipscritching) mainprogram->leftmousedown = false;
                if (lay2->clipscritching == 1) {
                    lay2->frame = (lay2->numf - 1) *
                                  ((mainprogram->mx - lay2->cliploopbox->scrcoords->x1) / lay2->cliploopbox->scrcoords->w);
                    if (lay2->frame < 0) lay2->frame = 0.0f;
                    else if (lay2->frame >= lay2->numf) lay2->frame = lay2->numf - 1;
                    lay2->set_clones();
                    if (mainprogram->leftmouse && !mainprogram->menuondisplay) lay2->clipscritching = 4;
                }
                for (int k = 0; k < until; k++) {
                    clipbox.vtxcoords->x1 = vbox->vtxcoords->x1;
                    clipbox.vtxcoords->y1 = vbox->vtxcoords->y1 - (k + 1) * vbox->vtxcoords->h - 0.125f;
                    clipbox.vtxcoords->w = vbox->vtxcoords->w;
                    clipbox.vtxcoords->h = vbox->vtxcoords->h;
                    clipbox.upvtxtoscr();
					draw_box(white, black, &clipbox, lay2->clips[k +
					lay2->queuescroll]->tex);
					render_text("Queued clip #" + std::to_string(k + lay2->queuescroll + 1), white,
                 clipbox.vtxcoords->x1 + 0.015f, clipbox.vtxcoords->y1 +0.045f + 0.05f
                 + 0.075f, 0.0005f, 0.0008f);
					if (lay2->clips[k + lay2->queuescroll]->type == ELEM_LIVE) {
						render_text(lay2->clips[k + lay2->queuescroll]->path, white, clipbox.vtxcoords->x1 + 0.015f
						, clipbox.vtxcoords->y1 - (clipbox.vtxcoords->h / 2.0f) - 0.075f, 0.0005f, 0.0008f);
					}
				}
				for (int k = 0; k < max; k++) {
                    insideclipbox.vtxcoords->x1 = vbox->vtxcoords->x1;
                    insideclipbox.vtxcoords->y1 = vbox->vtxcoords->y1 - (k + 0.75f) * vbox->vtxcoords->h - 0.125f;
                    insideclipbox.vtxcoords->w = vbox->vtxcoords->w;
                    insideclipbox.vtxcoords->h = vbox->vtxcoords->h / 2.0f;
                    insideclipbox.upvtxtoscr();
                    borderclipbox.vtxcoords->x1 = vbox->vtxcoords->x1;
                    borderclipbox.vtxcoords->y1 = vbox->vtxcoords->y1 - (k + 0.25f) * vbox->vtxcoords->h - 0.125f;
                    borderclipbox.vtxcoords->w = vbox->vtxcoords->w;
                    borderclipbox.vtxcoords->h = vbox->vtxcoords->h / 2.0f;
                    borderclipbox.upvtxtoscr();
					if (borderclipbox.in()) {
                        if (mainprogram->dragbinel) {
                            draw_box(lightblue, lightblue, borderclipbox.vtxcoords->x1 + (k == 0) * 0.075f,
                                     borderclipbox.vtxcoords->y1,
                                     borderclipbox.vtxcoords->w - ((k == 0) * 0.075) * 2.0f,
                                     borderclipbox.vtxcoords->h, -1);
                            if (mainprogram->lmover) {
                                // inserting
                                if (mainprogram->dragbinel->type != ELEM_DECK &&
                                    mainprogram->dragbinel->type != ELEM_MIX) {
                                    /*if (mainprogram->dragbinel) {
                                        if (mainprogram->draglay == lay2) { // && j == 0) {
                                            enddrag();
                                            return;
                                        }
                                    }*/
                                    if (mainprogram->dragbinel) {
                                        Clip *newclip = new Clip;
                                        GLuint butex = newclip->tex;
                                        newclip->tex = copy_tex(mainprogram->dragbinel->tex);
                                        if (butex != -1) glDeleteTextures(1, &butex);
                                        newclip->type = mainprogram->dragbinel->type;
                                        newclip->path = mainprogram->dragbinel->path;
                                        newclip->relpath = mainprogram->dragbinel->relpath;
                                        if (newclip->type == ELEM_IMAGE) {
                                            newclip->get_imageframes();
                                        } else if (newclip->type == ELEM_FILE) {
                                            newclip->get_videoframes();
                                        } else if (newclip->type == ELEM_LAYER) {
                                            newclip->get_layerframes();
                                        }
                                        newclip->insert(lay2, lay2->clips.begin() + k + lay2->queuescroll);
                                        enddrag();
                                        if (k + lay2->queuescroll == lay2->clips.size() - 1) {
                                            Clip *clip = new Clip;
                                            clip->insert(lay2, lay2->clips.end());
                                            if (lay2->clips.size() > 4) lay2->queuescroll++;
                                        }
                                        return;
                                    }
                                }
                            }
                        }
					}
                    if (insideclipbox.in()) {
                        if (mainprogram->dropfiles.size()) {
                            // SDL drag'n'drop
                            mainprogram->path = mainprogram->dropfiles[0];
                            for (char *df: mainprogram->dropfiles) {
                                mainprogram->paths.push_back(df);
                            }
                            mainprogram->pathto = "OPENFILESCLIP";
                            mainmix->mouseclip = lay2->clips[k + lay2->queuescroll];
                            mainmix->mouselayer = lay2;
                        }
                        if (mainprogram->dragbinel) {
                            // replacing
                            Clip* jc = lay2->clips[k + lay2->queuescroll];
                            if (mainprogram->lmover) {
                                if (mainprogram->dragbinel) {
                                    if (jc == mainprogram->dragclip) {
                                        enddrag();
                                        continue;
                                    }
                                    /*if (mainprogram->dragclip) {
                                        mainprogram->dragclip->tex = jc->tex;
                                        mainprogram->dragclip->type = jc->type;
                                        mainprogram->dragclip->path = jc->path;
                                        mainprogram->dragclip->insert(mainprogram->draglay, mainprogram->draglay->clips.begin() + mainprogram->dragpos);
                                        mainprogram->dragclip = nullptr;
                                    }*/
                                    GLuint butex = jc->tex;
                                    jc->tex = copy_tex(mainprogram->dragbinel->tex);
                                    if (butex != -1) glDeleteTextures(1, &butex);
                                    jc->type = mainprogram->dragbinel->type;
                                    jc->path = mainprogram->dragbinel->path;
                                    jc->relpath = mainprogram->dragbinel->relpath;
                                    if (jc->type == ELEM_IMAGE) {
                                        jc->get_imageframes();
                                    }
                                    else if (jc->type == ELEM_FILE) {
                                        jc->get_videoframes();
                                    }
                                    else if (jc->type == ELEM_LAYER) {
                                        jc->get_layerframes();
                                    }
                                    enddrag();
                                    if (k + lay2->queuescroll == lay2->clips.size() - 1) {
                                        Clip* clip = new Clip;
                                        clip->insert(lay2, lay2->clips.end());
                                        if (lay2->clips.size() > 4) lay2->queuescroll++;
                                    }
                                    return;
                                }
                            }
                        }
                    }
                }
                for (int k = 0; k < max; k++) {
                    insideclipbox.vtxcoords->x1 = vbox->vtxcoords->x1;
                    insideclipbox.vtxcoords->y1 = vbox->vtxcoords->y1 - (k + 0.75f) * vbox->vtxcoords->h - 0.125f;
                    insideclipbox.vtxcoords->w = vbox->vtxcoords->w;
                    insideclipbox.vtxcoords->h = vbox->vtxcoords->h / 2.0f;
                    insideclipbox.upvtxtoscr();
                    if (insideclipbox.in()) {
                        if (mainprogram->mousewheel) {
                            lay2->queuescroll -= mainprogram->mousewheel;
                            if (lay2->clips.size() >= 4) {
                                lay2->queuescroll = std::clamp(lay2->queuescroll, 0, (int) (lay2->clips.size() - 4));
                            }
                            break;
                        }
                        if (mainprogram->leftmousedown) {
                            // starting dragging a clip
                            if (k + lay2->queuescroll < lay2->clips.size() - 1) {
                                Clip *clip = lay2->clips[k + lay2->queuescroll];
                                mainprogram->dragbinel = new BinElement;
                                mainprogram->dragbinel->type = clip->type;
                                mainprogram->dragbinel->path = clip->path;
                                mainprogram->dragbinel->relpath = clip->relpath;
                                mainprogram->dragbinel->tex = clip->tex;
                                lay2->vidmoving = true;
                                mainprogram->dragclip = clip;
                                mainprogram->draglay = lay2;
                                mainprogram->draglaypos = lay2->pos;
                                mainprogram->draglaydeck = lay2->deck;
                                mainprogram->dragpos = k + lay2->queuescroll;
                            }
                            mainprogram->shelves[0]->prevnum = -1;
                            mainprogram->shelves[1]->prevnum = -1;
                            mainprogram->leftmousedown = false;
                            startdrag = true;
                        }
                    }
                }
			}
		}
	}

    for (int i = 0; i < 2; i++) {
        std::vector<Layer *> &lays = choose_layers(i);
        for (int j = 0; j < lays.size(); j++) {
            Layer *lay2 = lays[j];
            if (mainprogram->dragbinel && !startdrag) {
                if (!mainprogram->binsscreen) {
                    if (mainprogram->lmover) {
                        if (lay2->pos != mainprogram->draglaypos) continue;
                        if (lay2->deck != mainprogram->draglaydeck) continue;
                        // leftmouse dragging clip into nothing destroys the dragged clip
                        bool inlayers = false;
                        for (int deck = 0; deck < 2; deck++) {
                            if (mainprogram->xvtxtoscr(mainprogram->numw + deck * 1.0f) <
                                mainprogram->mx &&
                                mainprogram->mx < deck * (glob->w / 2.0f) + glob->w / 2.0f) {
                                if (0 < mainprogram->my &&
                                    mainprogram->my < mainprogram->yvtxtoscr(mainprogram->layh)) {
                                    // but not when dragged into layer stack field, even when there's no layer there
                                    inlayers = true;
                                    break;
                                }
                            }
                        }
                        if (!inlayers && mainprogram->dragclip) {
                            lay2->clips.erase(
                                    std::find(lay2->clips.begin(), lay2->clips.end(),
                                              mainprogram->dragclip));
                            delete mainprogram->dragclip;
                            mainprogram->dragclip = nullptr;
                            enddrag();
                        }
                    }
                }
            }
        }
    }

    //clip menu?
	if (mainprogram->prevmodus) {
		clip_inside_test(this->layers[0], 0);
		clip_inside_test(this->layers[1], 1);
	}
	else {
		clip_inside_test(this->layers[2], 0);
		clip_inside_test(this->layers[3], 1);
	}

    mainprogram->frontbatch = false;
}

void Layer::clip_display_next(bool startend, bool alive) {
	// cycle clips and load the first in queue
    if (this == nullptr) return;
    if (this->changeinit != 5 && this->changeinit != -1) return;
	if (this->clips.size() > 1) {
        Clip *oldclip = new Clip;
        oldclip->type = this->type;
        //if (!alive && this->currclip) {
        //oldclip->type = this->currclip->type;
        //}
        if (oldclip->type == ELEM_LAYER) oldclip->frame = this->frame;
        else oldclip->frame = 0.0f;
        if (startend) oldclip->frame = this->endframe->value;
        oldclip->startframe->value = this->startframe->value;
        oldclip->endframe->value = this->endframe->value;
        if (!alive && this->currclip) {
            //oldclip->tex = this->currclip->tex;
        }
        VideoNode *node = (VideoNode *) this->node;
        bool buec = mainprogram->effcat[this->deck]->value;
        bool renew = true;
        GLuint butex = oldclip->tex;
        if (this->currclip->type != ELEM_LIVE) {
            if (oldclip->path == "" || oldclip->type != ELEM_LAYER) {
                oldclip->path = find_unused_filename("cliptemp_" + basename(this->filename),
                                                     mainprogram->temppath,
                                                     ".layer");
            }
            //this->currclip->type = ELEM_LAYER;
            oldclip->type = ELEM_LAYER;
            if (this->currclipjpegpath == "") {
                this->currclipjpegpath = find_unused_filename(basename(oldclip->path), mainprogram->temppath, ".jpg");
            }
            oldclip->jpegpath = this->currclipjpegpath;
            if (this->comp == !mainprogram->bupm && !mainprogram->binsscreen) {
                oldclip->tex = copy_tex(node->vidbox->tex, 192, 108);
                save_thumb(oldclip->jpegpath, oldclip->tex);
            }
            else {
                renew = false;
                glGenTextures(1, &oldclip->tex);
                glBindTexture(GL_TEXTURE_2D, oldclip->tex);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                if (!this->compswitched) {
                    open_thumb(oldclip->jpegpath, oldclip->tex);
                } else {
                    open_thumb(this->currcliptexpath, oldclip->tex);
                    oldclip->jpegpath = find_unused_filename(basename(oldclip->path), mainprogram->temppath, ".jpg");
                    this->compswitched = false;
                    save_thumb(oldclip->jpegpath, oldclip->tex);
                }
            }
            //else oldclip->tex = this->currclip->tex;
            //oldclip->path = this->currclippath;
            mainprogram->effcat[this->deck]->value = 0;
            mainmix->save_layerfile(oldclip->path, this, 0, 0);
        } else if (this->type == ELEM_LIVE) {
            oldclip->tex = copy_tex(node->vidbox->tex);
            oldclip->path = this->filename;
            /*} else if (this->type == ELEM_IMAGE) {
                oldclip->tex = copy_tex(this->fbotex);
                oldclip->path = this->filename;
            } else {
                oldclip->tex = copy_tex(this->fbotex);
                oldclip->path = this->filename;
                oldclip->type = ELEM_FILE;*/
        }
        if (butex != -1 && renew) {
            glDeleteTextures(1, &butex);
        }


        this->currclip = this->clips[0];
        this->currclippath = this->currclip->path;
        this->currclipjpegpath = this->currclip->jpegpath;

        this->clips.erase(this->clips.begin());
        oldclip->insert(this, this->clips.end() - 1);

        this->frame = this->currclip->frame;
        this->startframe->value = this->currclip->startframe->value;
        this->endframe->value = this->currclip->endframe->value;
        this->type = this->currclip->type;
        this->layerfilepath = this->currclippath;
        this->filename = this->currclippath;

        Layer *lay = this;
        std::vector<Effect *> bueff1 = lay->effects[1];

        if (lay->currclip->type == ELEM_LAYER) {
            int currl1 = (std::find(mainmix->currlays[!mainprogram->prevmodus].begin(),
                                    mainmix->currlays[!mainprogram->prevmodus].end(), lay) -
                          mainmix->currlays[!mainprogram->prevmodus].begin());
            if (currl1 == mainmix->currlays[!mainprogram->prevmodus].size()) currl1 = -1;
            bool currl2 = (lay == mainmix->currlay[!mainprogram->prevmodus]);
            //std::vector<Clip *> savcl = lay->clips;
            //lay->clips.clear();
            lay = mainmix->open_layerfile(lay->currclippath, lay, true, false);
            if (currl1 != -1) mainmix->currlays[!mainprogram->prevmodus][currl1] = lay;  // reminder
            if (currl2) mainmix->currlay[!mainprogram->prevmodus] = lay;
            this->set_inlayer(lay, true);
            //this->closethread = 1;
            //lay->clips = savcl;
            lay->currclip->type = ELEM_LAYER;
        }
        else if (lay->currclip->type == ELEM_LIVE) {
            lay->set_live_base(lay->currclippath);
            lay->currclip->type = ELEM_LIVE;
        }
        else if (isvideo(lay->currclippath)) {
            lay = lay->open_video(0, lay->currclippath, 0);
            //if (lay->initialized) mainmix->copy_pbos(lay, lay);
            //lay->currclip->type = ELEM_FILE;
        }
        else if (isimage(lay->currclippath)) {
            lay->transfered = true;
            lay->open_image(lay->currclippath);
            //lay->currclip->type = ELEM_IMAGE;
        }

        delete this->currclip;

        if (this == mainmix->mouselayer) {
            mainmix->mouselayer = lay;
        }
        mainprogram->effcat[lay->deck]->value = buec;
        lay->effects[1] = bueff1;
        mainmix->reconnect_all(*lay->layers);

	}
}




// UTILITY

Effect* new_effect(EFFECT_TYPE type) {
	switch (type) {
	case BLUR:
		return new BlurEffect;
		break;
	case BRIGHTNESS:
		return new BrightnessEffect;
		break;
	case CHROMAROTATE:
		return new ChromarotateEffect;
		break;
	case CONTRAST:
		return new ContrastEffect;
		break;
	case DOT:
		return new DotEffect;
		break;
	case GLOW:
		return new GlowEffect;
		break;
	case RADIALBLUR:
		return new RadialblurEffect;
		break;
	case SATURATION:
		return new SaturationEffect;
		break;
	case SCALE:
		return new ScaleEffect;
		break;
	case SWIRL:
		return new SwirlEffect;
		break;
	case OLDFILM:
		return new OldFilmEffect;
		break;
	case RIPPLE:
		return new RippleEffect;
		break;
	case FISHEYE:
		return new FishEyeEffect;
		break;
	case TRESHOLD:
		return new TresholdEffect;
		break;
	case STROBE:
		return new StrobeEffect;
		break;
	case POSTERIZE:
		return new PosterizeEffect;
		break;
	case PIXELATE:
		return new PixelateEffect;
		break;
	case CROSSHATCH:
		return new CrosshatchEffect;
		break;
	case INVERT:
		return new InvertEffect;
		break;
	case ROTATE:
		return new RotateEffect;
		break;
	case EMBOSS:
		return new EmbossEffect;
		break;
	case ASCII:
		return new AsciiEffect;
		break;
	case SOLARIZE:
		return new SolarizeEffect;
		break;
	case VARDOT:
		return new VarDotEffect;
		break;
	case CRT:
		return new CRTEffect;
		break;
	case EDGEDETECT:
		return new EdgeDetectEffect;
		break;
	case KALEIDOSCOPE:
		return new KaleidoScopeEffect;
		break;
	case HTONE:
		return new HalfToneEffect;
		break;
	case CARTOON:
		return new CartoonEffect;
		break;
	case CUTOFF:
		return new CutoffEffect;
		break;
	case GLITCH:
		return new GlitchEffect;
		break;
	case COLORIZE:
		return new ColorizeEffect;
		break;
	case NOISE:
		return new NoiseEffect;
		break;
	case GAMMA:
		return new GammaEffect;
		break;
	case THERMAL:
		return new ThermalEffect;
		break;
	case BOKEH:
		return new BokehEffect;
		break;
	case SHARPEN:
		return new SharpenEffect;
		break;
	case DITHER:
		return new DitherEffect;
		break;
	case FLIP:
		return new FlipEffect;
		break;
	case MIRROR:
		return new MirrorEffect;
		break;
    case BOXBLUR:
        return new BoxblurEffect;
        break;
    case CHROMASTRETCH:
        return new ChromastretchEffect;
        break;
	}
	return nullptr;
}



Retarget::Retarget() {
    this->iconbox = new Boxx;
    this->iconbox->vtxcoords->x1 = 0.2f;
    this->iconbox->vtxcoords->y1 = 0.0f;
    this->iconbox->vtxcoords->w = 0.1f;
    this->iconbox->vtxcoords->h = 0.1f;
    this->iconbox->upvtxtoscr();
    this->iconbox->tooltiptitle = "Browse for file ";
    this->iconbox->tooltip = "Leftclick this icon to browse for the file. ";
    this->valuebox = new Boxx;
    this->valuebox->vtxcoords->x1 = -0.4f;
    this->valuebox->vtxcoords->y1 = 0.0f;
    this->valuebox->vtxcoords->w = 0.6f;
    this->valuebox->vtxcoords->h = 0.1f;
    this->valuebox->upvtxtoscr();
    this->valuebox->tooltiptitle = "Video/image files not found ";
    this->valuebox->tooltip = "Locate video/image files that weren't found on the saved location.  Use arrows/mousewheel to scroll list when its bigger then the screen.  Click DONE to continue. ";
    this->searchbox = new Boxx;
    this->searchbox->vtxcoords->x1 = -0.4f;
    this->searchbox->vtxcoords->y1 = -0.1f;
    this->searchbox->vtxcoords->w = 0.8f;
    this->searchbox->vtxcoords->h = 0.1f;
    this->searchbox->upvtxtoscr();
    this->searchbox->tooltiptitle = "Search video/image files ";
    this->searchbox->tooltip = "Add search location for video/image files that weren't found on their saved location.  Leftclick to browse. ";
    this->skipbox = new Boxx;
    this->skipbox->vtxcoords->x1 = 0.1f;
    this->skipbox->vtxcoords->y1 = 0.1f;
    this->skipbox->vtxcoords->w = 0.1f;
    this->skipbox->vtxcoords->h = 0.1f;
    this->skipbox->upvtxtoscr();
    this->skipbox->tooltiptitle = "Skip this file ";
    this->skipbox->tooltip = "Leftclick to skip the retargeting step for this file. ";
    this->skipallbox = new Boxx;
    this->skipallbox->vtxcoords->x1 = 0.2f;
    this->skipallbox->vtxcoords->y1 = 0.1f;
    this->skipallbox->vtxcoords->w = 0.1f;
    this->skipallbox->vtxcoords->h = 0.1f;
    this->skipallbox->upvtxtoscr();
    this->skipallbox->tooltiptitle = "Skip this file ";
    this->skipallbox->tooltip = "Leftclick to skip the retargeting step for this file. ";
}


Layer* Layer::transfer() {
    // why???
    LoopStation *l;
    if (!mainprogram->prevmodus) {
        l = lpc;
    } else {
        l = lp;
    }
    this->layers->erase(std::find(this->layers->begin(), this->layers->end(), this));
    Layer *lay = mainmix->add_layer(*this->layers, this->pos);
    //(*(this->layers))[this->pos] = lay;
    //mainprogram->nodesmain->currpage->connect_nodes(lay->node, this->lasteffnode[0]->out[0]);
    for (int m = 0; m < 2; m++) {
        for (int j = 0; j < this->effects[m].size(); j++) {
            Effect *eff = lay->add_effect(this->effects[m][j]->type, j);
            for (int k = 0; k < this->effects[m][j]->params.size(); k++) {
                Param *par = this->effects[m][j]->params[k];
                Param *cpar = lay->effects[m][j]->params[k];
                LoopStationElement *lpe = l->parelemmap[par];
                if (lpe) {
                    for (int i = 0; i < lpe->eventlist.size(); i++) {
                        if (std::get<1>(lpe->eventlist[i]) == par) {
                            std::get<1>(lpe->eventlist[i]) = cpar;
                        }
                    }
                    cpar->box->acolor[0] = lpe->colbox->acolor[0];
                    cpar->box->acolor[1] = lpe->colbox->acolor[1];
                    cpar->box->acolor[2] = lpe->colbox->acolor[2];
                    cpar->box->acolor[3] = lpe->colbox->acolor[3];
                    l->parelemmap[cpar] = lpe;
                    l->parelemmap.erase(par);
                    lpe->params.erase(par);
                    lpe->params.emplace(cpar);
                    lpe->params.erase(par);
                    lpe->layers.emplace(lay);
                    lpe->layers.erase(this);
                    l->parmap[par] = l->parmap[cpar];
                    l->allparams.erase(std::find(l->allparams.begin(), l->allparams.end(), par));
                    l->allparams.push_back(cpar);
                }
                cpar->value = par->value;
                cpar->midi[0] = par->midi[0];
                cpar->midi[1] = par->midi[1];
                cpar->register_midi();
                cpar->effect = eff;
            }
            eff->drywet->value = this->effects[m][j]->drywet->value;
            Button *but = this->effects[m][j]->onoffbutton;
            Button *cbut = eff->onoffbutton;
            LoopStationElement *lpe = l->butelemmap[but];
            if (lpe) {
                for (int i = 0; i < lpe->eventlist.size(); i++) {
                    if (std::get<2>(lpe->eventlist[i]) == but) {
                        std::get<2>(lpe->eventlist[i]) = cbut;
                    }
                }
//cbut->box->acolor[0] = lpe->colbox->acolor[0];
//cbut->box->acolor[1] = lpe->colbox->acolor[1];
//cbut->box->afcolor[2] = lpe->colbox->acolor[2];
//cbut->box->acolor[3] = lpe->colbox->acolor[3];
                l->butelemmap[cbut] = lpe;
                l->butelemmap.erase(but);
                lpe->buttons.erase(but);
                lpe->buttons.emplace(cbut);
                lpe->buttons.erase(but);
                lpe->layers.emplace(lay);
                lpe->layers.erase(this);
                l->butmap[but] = l->butmap[cbut];
                l->allbuttons.erase(std::find(l->allbuttons.begin(), l->allbuttons.end(), but));
                l->allbuttons.push_back(cbut);
            }
            cbut->value = but->value;
            cbut->midi[0] = but->midi[0];
            cbut->midi[1] = but->midi[1];
            cbut->register_midi();
        }
    }
    this->inhibit(); // lay is passed over into only framecounting
    mainmix->set_values(lay, this, false);
    if (this == mainmix->currlay[!mainprogram->prevmodus]) mainmix->currlay[!mainprogram->prevmodus] = lay;
    if (std::find(mainmix->currlays[!mainprogram->prevmodus].begin(), mainmix->currlays[!mainprogram->prevmodus].end(),
                  this) != mainmix->currlays[!mainprogram->prevmodus].end()) {
        mainmix->currlays[!mainprogram->prevmodus].erase(std::find(mainmix->currlays[!mainprogram->prevmodus].begin(),
                                                                   mainmix->currlays[!mainprogram->prevmodus].end(),
                                                                   this));
        mainmix->currlays[!mainprogram->prevmodus].push_back(lay);
    }

    mainmix->reconnect_all(*lay->layers);

    return lay;
}

std::vector<Effect*>& Layer::choose_effects() {
    if (mainprogram->effcat[this->deck]->value) {
        return this->effects[1];
    }
    else {
        return this->effects[0];
    }
}

std::vector<Layer*>& choose_layers(bool j) {
    if (mainprogram->prevmodus) {
        if (j == 0) return mainmix->layers[0];
        else return mainmix->layers[1];
    }
    else {
        if (j == 0) return mainmix->layers[2];
        else return mainmix->layers[3];
    }
}




// CLIPS

Clip::Clip() {
	this->box = new Boxx;
	this->box->tooltiptitle = "Clip queue ";
	this->box->tooltip = "Clip queue: clips (videos, images, layer files, live feeds) loaded here are played in order after the current clip.  Rightclick menu allows loading live feed / opening content into clip / deleting clip.  Clips can be dragged anywhere and anything can be dragged into or inserted between them. ";
	this->tex = -1;
    this->startframe = new Param;
    this->startframe->value = -1;
    this->endframe = new Param;
    this->endframe->value = -1;
}

Clip::~Clip() {
    delete this->box;
    delete this->startframe;
    delete this->endframe;
	if (this->tex != -1) glDeleteTextures(1, &this->tex);
	if (this->path.rfind(".layer") != std::string::npos) {
		if (this->path.find("cliptemp_") != std::string::npos) {
			std::filesystem::remove(this->path);
		}
	}
}

// reminder : check all currlay/currlays

Clip* Clip::copy() {
    Clip *clip = new Clip;
    clip->path = this->path;
    clip->type = this->type;
    clip->tex = copy_tex(this->tex);
    clip->frame = this->frame;
    clip->startframe->value = this->startframe->value;
    clip->endframe->value = this->endframe->value;
    clip->box = this->box->copy();
    return clip;
}

void Clip::insert(Layer* lay, std::vector<Clip*>::iterator it) {
    lay->clips.insert(it, this);
    this->layer = lay;
}

bool Clip::get_imageframes() {
	ILuint boundimage;
	ilGenImages(1, &boundimage);
	ilBindImage(boundimage);
	ILboolean ret = ilLoadImage((const ILstring)this->path.c_str());
	if (ret == IL_FALSE) {
		printf("can't load image %s\n", this->path.c_str());
		return false;
	}
	int numf = ilGetInteger(IL_NUM_IMAGES);
	this->startframe->value = 0;
	this->frame = 0.0f;
	this->endframe->value = numf;

	return true;
}

bool Clip::get_layerframes() {
	std::string result = deconcat_files(this->path);
	bool concat = (result != "");
	std::ifstream rfile;
	if (concat) rfile.open(result);
	else rfile.open(this->path);

	std::string istring;

	while (safegetline(rfile, istring)) {
		if (istring == "ENDOFFILE") break;
		if (istring == "startframe->value") {
			safegetline(rfile, istring);
			this->startframe->value = std::stoi(istring);
		}
		if (istring == "endframe->value") {
			safegetline(rfile, istring);
			this->endframe->value = std::stoi(istring);
		}
		if (istring == "FRAME") {
			safegetline(rfile, istring);
			this->frame = std::stof(istring);
		}
	}

	rfile.close();

	return 1;
}

bool Clip::get_videoframes() {
	AVFormatContext* video = nullptr;
	AVStream* video_stream = nullptr;
	int video_stream_idx = -1;
	int r = avformat_open_input(&video, this->path.c_str(), nullptr, nullptr);
	printf("loading... %s\n", this->path.c_str());
	if (r < 0) {
		printf("%s\n", "Couldnt open video");
		return false;
	}
	avformat_find_stream_info(video, nullptr);
    find_stream_index(&(video_stream_idx), video, AVMEDIA_TYPE_VIDEO);
	video_stream = video->streams[video_stream_idx];
	this->frame = 0.0f;
	this->startframe->value = 0;
	this->endframe->value = video_stream->nb_frames;
    avformat_close_input(&video);

	return true;
}

void Clip::open_clipfiles() {
	// order elements
    if (mainprogram->paths.size() == 0) {
        mainprogram->openclipfiles = false;
        mainprogram->multistage = 0;
        return;
    }
	bool cont = mainprogram->order_paths(false);
	if (!cont) return;

	const std::string str = mainprogram->paths[mainprogram->clipfilescount];
	int pos = std::find(mainprogram->clipfileslay->clips.begin(), mainprogram->clipfileslay->clips.end(), mainprogram->clipfilesclip) - mainprogram->clipfileslay->clips.begin();
	if (pos == mainprogram->clipfileslay->clips.size() - 1) {
		Clip* clip = new Clip;
		if (mainprogram->clipfileslay->clips.size() > 4) mainprogram->clipfileslay->queuescroll++;
		mainprogram->clipfilesclip = clip;
		clip->insert(mainprogram->clipfileslay, mainprogram->clipfileslay->clips.end() - 1);
	}
	mainprogram->clipfilesclip->path = str;

	if (isimage(str)) {
        Layer *lay = new Layer(true);
        get_imagetex(lay, str);
        std::unique_lock<std::mutex> lock2(lay->enddecodelock);
        lay->enddecodevar.wait(lock2, [&] {return lay->processed; });
        lay->processed = false;
        lock2.unlock();
        mainprogram->clipfilesclip->tex = mainprogram->get_tex(lay);
        lay->closethread = 1;

		mainprogram->clipfilesclip->type = ELEM_IMAGE;
		mainprogram->clipfilesclip->get_imageframes();
	}
	else if (str.substr(str.length() - 6, std::string::npos) == ".layer") {
        Layer *lay = new Layer(true);
        get_layertex(lay, str);
        std::unique_lock<std::mutex> lock2(lay->enddecodelock);
        lay->enddecodevar.wait(lock2, [&] {return lay->processed; });
        lay->processed = false;
        lock2.unlock();
        GLuint butex = lay->fbotex;
        lay->fbotex = copy_tex(lay->texture);
        glDeleteTextures(1, &butex);
        onestepfrom(0, lay->node, nullptr, -1, -1);
        if (lay->effects[0].size()) {
            mainprogram->clipfilesclip->tex = copy_tex(lay->effects[0][lay->effects[0].size() - 1]->fbotex, binsmain->elemboxes[0]->scrcoords->w, binsmain->elemboxes[0]->scrcoords->h);
        } else {
            mainprogram->clipfilesclip->tex = copy_tex(lay->fbotex, binsmain->elemboxes[0]->scrcoords->w, binsmain->elemboxes[0]->scrcoords->h);
        }
        lay->closethread = 1;

		mainprogram->clipfilesclip->type = ELEM_LAYER;
		mainprogram->clipfilesclip->get_layerframes();
	}
	else {
        Layer *lay = new Layer(true);
        get_videotex(lay, str);
        std::unique_lock<std::mutex> lock2(lay->enddecodelock);
        lay->enddecodevar.wait(lock2, [&] {return lay->processed; });
        lay->processed = false;
        lock2.unlock();

        mainprogram->clipfilesclip->tex = mainprogram->get_tex(lay);
        lay->closethread = 1;
		mainprogram->clipfilesclip->type = ELEM_FILE;
		mainprogram->clipfilesclip->get_videoframes();
	}
	GLuint butex = mainprogram->clipfilesclip->tex;
	mainprogram->clipfilesclip->tex = copy_tex(mainprogram->clipfilesclip->tex);
    mainprogram->clipfilesclip->jpegpath = find_unused_filename(basename(mainprogram->clipfilesclip->path), mainprogram->temppath, ".jpg");
    save_thumb(mainprogram->clipfilesclip->jpegpath, mainprogram->clipfilesclip->tex);
	if (butex != -1) glDeleteTextures(1, &butex);
	mainprogram->clipfilescount++;
	mainprogram->clipfilesclip = mainprogram->clipfileslay->clips[pos + 1];
	if (mainprogram->clipfilescount == mainprogram->paths.size()) {
		mainprogram->clipfileslay->cliploading = false;
		mainprogram->openclipfiles = false;
		mainprogram->paths.clear();
		mainprogram->multistage = 0;
	}
}




