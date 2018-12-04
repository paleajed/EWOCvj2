#include <boost/filesystem.hpp>

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
#include "GL/glut.h"

#include <ostream>
#include <istream>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ios>

#include "node.h"
#include "box.h"
#include "effect.h"
#include "layer.h"
#include "window.h"
#include "program.h"
#include "loopstation.h"

Mixer::Mixer() {
	for (int j = 0; j < 2; j++) {
		for (int i = 0; i < 5; i++) {
			Scene *scene = new Scene;
			scene->deck = j;
			this->scenes[j].push_back(scene);
			Box *box = new Box;
			Button *button = new Button(false);
			this->scenes[j][i]->box = box;
			this->scenes[j][i]->button = button;
			if (i == 0) {
				box->lcolor[0] = 1.0;
				box->lcolor[1] = 0.5;
				box->lcolor[2] = 0.5;
				box->lcolor[3] = 1.0;
			}
			else {
				box->tooltiptitle = "Scene toggle deck A";
				box->tooltip = "Leftclick activates one of the four scenes (separate layer stacks) for deck A. ";
				box->lcolor[0] = 1.0;
				box->lcolor[1] = 1.0;
				box->lcolor[2] = 1.0;
				box->lcolor[3] = 1.0;
			}
			box->vtxcoords->x1 = -1 + j;
			box->vtxcoords->y1 = 1 - (i + 1) * mainprogram->numh;
			box->vtxcoords->w = mainprogram->numw;
			box->vtxcoords->h = mainprogram->numh;
			box->upvtxtoscr();
		}
	}

	this->modebox = new Box;
	this->modebox->vtxcoords->x1 = 0.85f;
	this->modebox->vtxcoords->y1 = -1.0f;
	this->modebox->vtxcoords->w = 0.15f;
	this->modebox->vtxcoords->h = 0.1f;
	this->modebox->upvtxtoscr();
	this->modebox->tooltiptitle = "Operation mode toggle ";
	this->modebox->tooltip = "Leftclick toggles between Basic and Node mode. ";
	
	this->genmidi[0] = new Button(false);
	this->genmidi[0]->value = 1;
	this->genmidi[1] = new Button(false);
	this->genmidi[1]->value = 2;
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
	this->genmidi[0]->box->tooltip = "Leftclick toggles between MIDI presets for deck A (A, B, C, D or off).  Rightclick shows menu allowing settings for the current MIDI preset. ";
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
	this->genmidi[1]->box->tooltip = "Leftclick toggles between MIDI presets for deck B (A, B, C, D or off).  Rightclick shows menu allowing settings for the current MIDI preset. ";
	
	this->crossfade = new Param;
	this->crossfade->name = "Crossfade"; 
	this->crossfade->value = 0.5f;
	this->crossfade->range[0] = 0.0f;
	this->crossfade->range[1] = 1.0f;
	this->crossfade->sliding = true;
	this->crossfade->shadervar = "cf";
	this->crossfade->box->vtxcoords->x1 = -0.15f;
	this->crossfade->box->vtxcoords->y1 = -1.0f + mainprogram->monh * 2.0f;
	this->crossfade->box->vtxcoords->w = 0.3f;
	this->crossfade->box->vtxcoords->h = tf(0.05f);
	this->crossfade->box->upvtxtoscr();
	this->crossfade->box->tooltiptitle = "Crossfade ";
	this->crossfade->box->tooltip = "Leftdrag crossfades between deck A and deck B streams. Doubleclick allows numeric entry. ";
	this->crossfade->box->acolor[3] = 1.0f;
	lpc->allparams.push_back(this->crossfade);
	this->crossfadecomp = new Param;
	this->crossfadecomp->name = "Crossfadecomp"; 
	this->crossfadecomp->value = 0.5f;
	this->crossfadecomp->range[0] = 0.0f;
	this->crossfadecomp->range[1] = 1.0f;
	this->crossfadecomp->sliding = true;
	this->crossfadecomp->shadervar = "cf";
	this->crossfadecomp->box->vtxcoords->x1 = -0.15f;
	this->crossfadecomp->box->vtxcoords->y1 = -1.0f + mainprogram->monh * 2.0f;
	this->crossfadecomp->box->vtxcoords->w = 0.3f;
	this->crossfadecomp->box->vtxcoords->h = tf(0.05f);
	this->crossfadecomp->box->upvtxtoscr();
	this->crossfadecomp->box->tooltiptitle = "Crossfade ";
	this->crossfadecomp->box->tooltip = "Leftdrag crossfades between deck A and deck B streams. Doubleclick allows numeric entry. ";
	this->crossfadecomp->box->acolor[3] = 1.0f;
	lpc->allparams.push_back(this->crossfadecomp);
	
	this->recbut = new Button(false);
	this->recbut->box->vtxcoords->x1 = 0.15f;
	this->recbut->box->vtxcoords->y1 = -1.0f + mainprogram->monh * 2.0f;
	this->recbut->box->vtxcoords->w = tf(0.031f);
	this->recbut->box->vtxcoords->h = tf(0.05f);
	this->recbut->box->upvtxtoscr();
	this->recbut->box->tooltiptitle = "Record output to video file ";
	this->recbut->box->tooltip = "Start/stop recording the output stream to an MJPEG video file in the recordings directory (set in preferences). ";
}


Param::Param() {
	this->box = new Box;
	this->box->acolor[0] = 0.2;
	this->box->acolor[1] = 0.2;
	this->box->acolor[2] = 0.2;
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
	if (std::find(loopstation->allparams.begin(), loopstation->allparams.end(), this) != loopstation->allparams.end()) {
		loopstation->allparams.erase(std::find(loopstation->allparams.begin(), loopstation->allparams.end(), this));
	}
}

void Param::handle() {
	float green[4] = {0.0, 1.0, 0.2, 1.0};
	float white[4] = {1.0, 1.0, 1.0, 1.0};
	std::string parstr;
	draw_box(this->box, -1);
	int val;
	if (!this->powertwo) val = round(this->value * 1000.0f);
	else val = round(this->value * this->value * 1000.0f);
	int val2 = val;
	val += 1000000;
	int firstdigit = 7 - std::to_string(val2).length();
	if (firstdigit > 3) firstdigit = 3;
	if (mainmix->learnparam == this and mainmix->learn) {
		parstr = "MIDI";
	}
	else if (this != mainmix->adaptparam) parstr = this->name;
	else if (this->sliding) parstr = std::to_string(val).substr(firstdigit, 1) + "." + std::to_string(val).substr(std::to_string(val).length() - 3, std::string::npos); 
	else parstr = std::to_string((int)(this->value + 0.5f));
	if (this != mainmix->adaptnumparam) {
		render_text(parstr, white, this->box->vtxcoords->x1 + tf(0.01f), this->box->vtxcoords->y1 + tf(0.05f) - tf(0.030f), tf(0.0003f), tf(0.0005f));
		if (this->box->in()) {
			if (mainprogram->leftmousedown) {
				mainprogram->leftmousedown = false;
				mainmix->adaptparam = this;
				mainmix->prevx = mainprogram->mx;
			}
			if (mainprogram->doubleleftmouse) {
				mainprogram->renaming = EDIT_PARAM;
				mainmix->adaptnumparam = this;
				mainprogram->inputtext = "";
				mainprogram->cursorpos = mainprogram->inputtext.length();
				SDL_StartTextInput();
				mainprogram->doubleleftmouse = false;
			}
			if (mainprogram->menuactivation and !mainprogram->menuondisplay) {
				if (loopstation->elemmap.find(this) != loopstation->elemmap.end()) mainprogram->parammenu2->state = 2;
				else mainprogram->parammenu1->state = 2;
				mainmix->learnbutton = nullptr;
				mainmix->learnparam = this;
				mainprogram->menuactivation = false;
			}
		}
		if (this->sliding) {
			draw_box(green, green, this->box->vtxcoords->x1 + this->box->vtxcoords->w * ((this->value - this->range[0]) / (this->range[1] - this->range[0])) - tf(0.001f), this->box->vtxcoords->y1, 0.002f, this->box->vtxcoords->h, -1);
		}
		else {
			draw_box(green, green, this->box->vtxcoords->x1 + this->box->vtxcoords->w * (((int)(this->value + 0.5f) - this->range[0]) / (this->range[1] - this->range[0])) - tf(0.001f), this->box->vtxcoords->y1, 0.002f, this->box->vtxcoords->h, -1);
		}
	}
	if (this == mainmix->adaptnumparam) {
		std::string part = mainprogram->inputtext.substr(0, mainprogram->cursorpos);
		float textw = render_text(part, white, box->vtxcoords->x1 + tf(0.01f), box->vtxcoords->y1 + tf(0.05f) - tf(0.03f), tf(0.0003f), tf(0.0005f), 1) * 0.5f;
		part = mainprogram->inputtext.substr(mainprogram->cursorpos, mainprogram->inputtext.length() - mainprogram->cursorpos);
		render_text(part, white, box->vtxcoords->x1 + tf(0.01f) + textw * 4, box->vtxcoords->y1 + tf(0.05f) - tf(0.03f), tf(0.0003f), tf(0.0005f), 1);
		draw_line(white, box->vtxcoords->x1 + tf(0.01f) + textw * 4, box->vtxcoords->y1 + tf(0.04f) - tf(0.03f), box->vtxcoords->x1 + tf(0.01f) + textw * 4, box->vtxcoords->y1 + tf(0.04f));
	}
}


Effect::Effect() {
	Box *box = new Box;
	this->box = box;
	box->vtxcoords->w = tf(0.2f);
	box->vtxcoords->h = tf(0.05f);
	box->upvtxtoscr();
	this->onoffbutton = new Button(true);
	box = this->onoffbutton->box;
	box->vtxcoords->x1 = -1.0f + mainprogram->numw;
	box->vtxcoords->w = tf(0.025f);
	box->vtxcoords->h = tf(0.05f);
	box->upvtxtoscr();
	box->tooltiptitle = "Effect on/off ";
	box->tooltip = "Leftclick toggles effect on/off ";
	
	// sets the dry/wet (mix of no-effect with effect) amount of the effect as a parameter
	// read comment at BlurEffect::BlurEffect()
	this->drywet = new Param;
	this->drywet->name = ""; 
	this->drywet->value = 1.0f;
	this->drywet->range[0] = 0.0f;
	this->drywet->range[1] = 1.0f;
	this->drywet->sliding = true;
	this->drywet->shadervar = "drywet";
	this->drywet->box->vtxcoords->w = tf(0.025f);
	this->drywet->box->vtxcoords->h = tf(0.05f);
	this->drywet->box->upvtxtoscr();
	this->drywet->box->tooltiptitle = "Effect dry/wet ";
	this->drywet->box->tooltip = "Sets the dry/wet value of the effect - Leftdrag sets value. Doubleclick allows numeric input. ";
}

Effect::~Effect() {
	delete this->box;
	delete this->onoffbutton;
	glDeleteTextures(1, &this->fbotex);
	glDeleteFramebuffers(1, &this->fbo);
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
	Param *param = new Param;
	param->name = "Amount"; 
	param->value = 10.0;
	param->range[0] = 2;
	param->range[1] = 132;
	param->sliding = true;
	param->shadervar = "glowblur";
	param->effect = this;
	param->box->tooltiptitle = "Blur amount ";
	param->box->tooltip = "Amount of image blurring - between 2.0 and 132.0 ";
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
	param->value = -0.5;
	param->range[0] = -0.5;
	param->range[1] = 0.5;
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
	param->box->tooltip = "Factor that multiplies image saturation - between 0.0 and 8.0";
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
	param->value = 64.0f;
	param->range[0] = 1.0f;
	param->range[1] = 512.0f;
	param->sliding = true;
	param->shadervar = "pixel_w";
	param->effect = this;
	param->box->tooltiptitle = "Pixel width ";
	param->box->tooltip = "Width of pixels of pixelization - between 1.0 and 512.0 ";
	this->params.push_back(param);
	param = new Param;
	param->name = "Height"; 
	param->value = 128.0f;
	param->range[0] = 1.0f;
	param->range[1] = 512.0f;
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
	param->range[0] = 0.0f;
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
	param->range[0] = 20.0f;
	param->range[1] = 200.0f;
	param->sliding = true;
	param->shadervar = "asciisize";
	param->effect = this;
	param->box->tooltiptitle = "ASCII size ";
	param->box->tooltip = "Size of ascii symbols - between 20.0 and 200.0 ";
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
	param->value = 0.0f;
	param->range[0] = 0.0f;
	param->range[1] = 40.0f;
	param->sliding = true;
	param->shadervar = "edthickness";  //dummy
	param->effect = this;
	param->box->tooltiptitle = "Edgedetect edge thickness ";
	param->box->tooltip = "Thickness of detected image edges - between 0.0 and 40.0 ";
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
	param->value = 30.0f;
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
	param->value = 0.5f;
	param->range[0] = 0.4f;
	param->range[1] = 0.55f;
	param->sliding = true;
	param->shadervar = "dither_size";
	param->effect = this;
	param->box->tooltiptitle = "Dithering size ";
	param->box->tooltip = "Size of the image dithering - between 0.4 and 0.55 ";
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


Effect *new_effect(Effect *effect) {
	switch (effect->type) {
		case BLUR:
			return new BlurEffect(*(BlurEffect *)effect);
			break;
		case BRIGHTNESS:
			return new BrightnessEffect(*(BrightnessEffect *)effect);
			break;
		case CHROMAROTATE:
			return new ChromarotateEffect(*(ChromarotateEffect *)effect);
			break;
		case CONTRAST:
			return new ContrastEffect(*(ContrastEffect *)effect);
			break;
		case DOT:
			return new DotEffect(*(DotEffect *)effect);
			break;
		case GLOW:
			return new GlowEffect(*(GlowEffect *)effect);
			break;
		case RADIALBLUR:
			return new RadialblurEffect(*(RadialblurEffect *)effect);
			break;
		case SATURATION:
			return new SaturationEffect(*(SaturationEffect *)effect);
			break;
		case SCALE:
			return new ScaleEffect(*(ScaleEffect *)effect);
			break;
		case SWIRL:
			return new SwirlEffect(*(SwirlEffect *)effect);
			break;
		case OLDFILM:
			return new OldFilmEffect(*(OldFilmEffect *)effect);
			break;
		case RIPPLE:
			return new RippleEffect(*(RippleEffect *)effect);
			break;
		case FISHEYE:
			return new FishEyeEffect(*(FishEyeEffect *)effect);
			break;
		case TRESHOLD:
			return new TresholdEffect(*(TresholdEffect *)effect);
			break;
		case STROBE:
			return new StrobeEffect(*(StrobeEffect *)effect);
			break;
		case POSTERIZE:
			return new PosterizeEffect(*(PosterizeEffect *)effect);
			break;
		case PIXELATE:
			return new PixelateEffect(*(PixelateEffect *)effect);
			break;
		case CROSSHATCH:
			return new CrosshatchEffect(*(CrosshatchEffect *)effect);
			break;
		case INVERT:
			return new InvertEffect(*(InvertEffect *)effect);
			break;
		case ROTATE:
			return new RotateEffect(*(RotateEffect *)effect);
			break;
		case EMBOSS:
			return new EmbossEffect(*(EmbossEffect *)effect);
			break;
		case ASCII:
			return new AsciiEffect(*(AsciiEffect *)effect);
			break;
		case SOLARIZE:
			return new SolarizeEffect(*(SolarizeEffect *)effect);
			break;
		case VARDOT:
			return new VarDotEffect(*(VarDotEffect *)effect);
			break;
		case CRT:
			return new CRTEffect(*(CRTEffect *)effect);
			break;
		case EDGEDETECT:
			return new EdgeDetectEffect(*(EdgeDetectEffect *)effect);
			break;
		case KALEIDOSCOPE:
			return new KaleidoScopeEffect(*(KaleidoScopeEffect *)effect);
			break;
		case HTONE:
			return new HalfToneEffect(*(HalfToneEffect *)effect);
			break;
		case CARTOON:
			return new CartoonEffect(*(CartoonEffect *)effect);
			break;
		case CUTOFF:
			return new CutoffEffect(*(CutoffEffect *)effect);
			break;
		case GLITCH:
			return new GlitchEffect(*(GlitchEffect *)effect);
			break;
		case COLORIZE:
			return new ColorizeEffect(*(ColorizeEffect *)effect);
			break;
		case NOISE:
			return new NoiseEffect(*(NoiseEffect *)effect);
			break;
		case GAMMA:
			return new GammaEffect(*(GammaEffect *)effect);
			break;
		case THERMAL:
			return new ThermalEffect(*(ThermalEffect *)effect);
			break;
		case BOKEH:
			return new BokehEffect(*(BokehEffect *)effect);
			break;
		case SHARPEN:
			return new SharpenEffect(*(SharpenEffect *)effect);
			break;
		case DITHER:
			return new DitherEffect(*(DitherEffect *)effect);
			break;
		case FLIP:
			return new FlipEffect(*(FlipEffect *)effect);
			break;
		case MIRROR:
			return new MirrorEffect(*(MirrorEffect *)effect);
			break;
	}
	return nullptr;
}

// adds an effect of a certain type to a layer at a certain position in its effect list
// comp is set when the layer belongs to the output layer stacks
Effect *do_add_effect(Layer *lay, EFFECT_TYPE type, int pos, bool comp) {
	Effect *effect;
	switch (type) {
		case BLUR:
			effect = new BlurEffect();
			break;
		case BRIGHTNESS:
			effect = new BrightnessEffect();
			break;
		case CHROMAROTATE:
			effect = new ChromarotateEffect();
			break;
		case CONTRAST:
			effect = new ContrastEffect();
			break;
		case DOT:
			effect = new DotEffect();
			break;
		case GLOW:
			effect = new GlowEffect();
			break;
		case RADIALBLUR:
			effect = new RadialblurEffect();
			break;
		case SATURATION:
			effect = new SaturationEffect();
			break;
		case SCALE:
			effect = new ScaleEffect();
			break;
		case SWIRL:
			effect = new SwirlEffect();
			break;
		case OLDFILM:
			effect = new OldFilmEffect();
			break;
		case RIPPLE:
			effect = new RippleEffect();
			((RippleEffect*)(effect))->speed = ((RippleEffect*)(effect))->params[0]->value;			
			break;
		case FISHEYE:
			effect = new FishEyeEffect();
			break;
		case TRESHOLD:
			effect = new TresholdEffect();
			break;
		case STROBE:
			effect = new StrobeEffect();
			break;
		case POSTERIZE:
			effect = new PosterizeEffect();
			break;
		case PIXELATE:
			effect = new PixelateEffect();
			break;
		case CROSSHATCH:
			effect = new CrosshatchEffect();
			break;
		case INVERT:
			effect = new InvertEffect();
			break;
		case ROTATE:
			effect = new RotateEffect();
			break;
		case EMBOSS:
			effect = new EmbossEffect();
			break;
		case ASCII:
			effect = new AsciiEffect();
			break;
		case SOLARIZE:
			effect = new SolarizeEffect();
			break;
		case VARDOT:
			effect = new VarDotEffect();
			break;
		case CRT:
			effect = new CRTEffect();
			break;
		case EDGEDETECT:
			effect = new EdgeDetectEffect();
			break;
		case KALEIDOSCOPE:
			effect = new KaleidoScopeEffect();
			break;
		case HTONE:
			effect = new HalfToneEffect();
			break;
		case CARTOON:
			effect = new CartoonEffect();
			break;
		case CUTOFF:
			effect = new CutoffEffect();
			break;
		case GLITCH:
			effect = new GlitchEffect();
			break;
		case COLORIZE:
			effect = new ColorizeEffect();
			break;
		case NOISE:
			effect = new NoiseEffect();
			break;
		case GAMMA:
			effect = new GammaEffect();
			break;
		case THERMAL:
			effect = new ThermalEffect();
			break;
		case BOKEH:
			effect = new BokehEffect();
			break;
		case SHARPEN:
			effect = new SharpenEffect();
			break;
		case DITHER:
			effect = new DitherEffect();
			break;
		case FLIP:
			effect = new FlipEffect();
			break;
		case MIRROR:
			effect = new MirrorEffect();
			break;
	}

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
		if (lay->pos > 0 and lay->blendnode and !cat) {
			lay->blendnode->in2 = nullptr;
			mainprogram->nodesmain->currpage->connect_in2(effnode1, lay->blendnode);
		}
		else if (lay->lasteffnode[cat]->out.size()) {
			lay->lasteffnode[cat]->out[0]->in = nullptr;
			mainprogram->nodesmain->currpage->connect_nodes(effnode1, lay->lasteffnode[cat]->out[0]);
		}
		lay->lasteffnode[cat]->out.clear();
		lay->lasteffnode[cat] = effnode1;
		if (lay->pos == 0 and !cat and lay->effects[1].size() == 0) {
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
				printf("lasteffnode0 %d\n", lay->lasteffnode[0]);
				lay->lasteffnode[0]->out.clear();
				mainprogram->nodesmain->currpage->connect_nodes(lay->lasteffnode[0], effnode1);
			}
			else {
				lay->blendnode->out.clear();
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
	
	// add parameters to OSC ssystem
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
		mainprogram->st->add_method(path, "f", osc_param, effect->params[i]);
	}
	
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
	if (this->type == ELEM_FILE) this->type = ELEM_LAYER;
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
			lay->lasteffnode[cat]->out = evec[pos]->node->out;
			if (cat) evec[pos]->node->out[0]->in = lay->lasteffnode[1];
		}
		if (lay->pos == 0) {
			lay->lasteffnode[1] = lay->lasteffnode[0];
		}
		if (!cat) {
			if (lay->pos == 0) {
				mainprogram->nodesmain->currpage->connect_nodes(lay->lasteffnode[0], evec[pos]->node->out[0]);
			}
			else {
				mainprogram->nodesmain->currpage->connect_in2(lay->lasteffnode[0], ((BlendNode*)(evec[pos]->node->out[0])));
			}
		}
	}
	else {
		evec[pos + 1]->node->in = evec[pos]->node->in;
		if (pos != 0) evec[pos - 1]->node->out.push_back(evec[pos + 1]->node);
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

	evec.erase(evec.begin() + pos);
	delete effect;
	
	make_layboxes();
}

void Layer::delete_effect(int pos) {
	do_delete_effect(this, pos);
	this->type = ELEM_LAYER;
}		


Layer* Mixer::add_layer(std::vector<Layer*> &layers, int pos) {
	bool comp;
	if (mainprogram->prevmodus) comp = false;
	else comp = true;
	Layer *layer = new Layer(comp);
	printf("comp %d\n", comp);
	if (layers == this->layersA or layers == this->layersAcomp) layer->deck = 0;
	else layer->deck = 1;
	Clip *clip = new Clip;
	layer->clips.push_back(clip);
	clip->type = ELEM_LAYER;
 	layer->node = mainprogram->nodesmain->currpage->add_videonode(comp);
	layer->node->layer = layer;
	layer->lasteffnode[0] = layer->node;
	if (layers == this->layersA or layers == this->layersAcomp) {
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
		if (pos == layers.size() - 1 and mainprogram->nodesmain->mixnodes.size()) {
			if (layers == mainmix->layersA) {
				mainprogram->nodesmain->currpage->connect_nodes(layer->blendnode, mainprogram->nodesmain->mixnodes[0]);
			}
			else if (layers == mainmix->layersB) {
				mainprogram->nodesmain->currpage->connect_nodes(layer->blendnode, mainprogram->nodesmain->mixnodes[1]);
			}
			else if (layers == mainmix->layersAcomp) {
				mainprogram->nodesmain->currpage->connect_nodes(layer->blendnode, mainprogram->nodesmain->mixnodescomp[0]);
			}
			else if (layers == mainmix->layersBcomp) {
				mainprogram->nodesmain->currpage->connect_nodes(layer->blendnode, mainprogram->nodesmain->mixnodescomp[1]);
			}
		}
		else if (pos < layers.size() - 1) {
			mainprogram->nodesmain->currpage->connect_nodes(layer->blendnode, layers[pos + 1]->blendnode);
		}	
	}
	else {
		layer->blendnode = new BlendNode;
		BlendNode *bnode = mainprogram->nodesmain->currpage->add_blendnode(MIXING, false);
		Layer *nextlay = nullptr;
		if (layers.size() > 1) nextlay = layers[1];
		if (nextlay) {
			mainprogram->nodesmain->currpage->connect_nodes(bnode, nextlay->lasteffnode[0]->out[0]);
			nextlay->lasteffnode[0]->out.clear();
			nextlay->blendnode = bnode;
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
	if (testlay == mainmix->currlay) {
		if (testlay->pos == layers.size() - 1) mainmix->currlay = layers[testlay->pos - 1];
		else mainmix->currlay = layers[testlay->pos + 1];
		if (layers.size() == 1) mainmix->currlay = nullptr;
	}

	if (layers.size() == 1 and add) {
		Layer *lay = mainmix->add_layer(layers, 1);
		if (!mainmix->currlay) mainmix->currlay = lay;
	}
	
	BLEND_TYPE nextbtype;
	Layer *nextlay = nullptr;
	if (layers.size() > testlay->pos + 1) {
		nextlay = layers[testlay->pos + 1];
		nextbtype = nextlay->blendnode->blendtype;
	}
	
	int size = layers.size();
	for (int i = 0; i < size; i++) {
		if (layers[i] == testlay) {
			layers.erase(layers.begin() + i);
		}
	}
		
    if (mainprogram->nodesmain->linked) {
		while (!testlay->clips.empty()) {
			delete testlay->clips.back();
			testlay->clips.pop_back();
		}
			
		while (!testlay->effects[0].empty()) {
			mainprogram->nodesmain->currpage->delete_node(testlay->effects[0].back()->node);
			for (int j = 0; j < testlay->effects[0].back()->params.size(); j++) {
				delete testlay->effects[0].back()->params[j];
			}
			delete testlay->effects[0].back();
			testlay->effects[0].pop_back();
		}
		while (!testlay->effects[1].empty()) {
			mainprogram->nodesmain->currpage->delete_node(testlay->effects[1].back()->node);
			for (int j = 0; j < testlay->effects[1].back()->params.size(); j++) {
				delete testlay->effects[1].back()->params[j];
			}
			delete testlay->effects[1].back();
			testlay->effects[1].pop_back();
		}

		if (testlay->pos > 0 and testlay->blendnode) {
			mainprogram->nodesmain->currpage->connect_nodes(testlay->blendnode->in, testlay->lasteffnode[1]->out[0]);
			mainprogram->nodesmain->currpage->delete_node(testlay->blendnode);
			testlay->blendnode = 0;
		}
		else {
			if (nextlay) {
				nextlay->lasteffnode[0]->out.clear();
				mainprogram->nodesmain->currpage->connect_nodes(nextlay->lasteffnode[0], nextlay->lasteffnode[1]->out[0]);
				mainprogram->nodesmain->currpage->delete_node(nextlay->blendnode);
				nextlay->blendnode = new BlendNode;
				nextlay->blendnode->blendtype = nextbtype;
			}
		}
		
		for(int i = testlay->pos; i < layers.size(); i++) {
			layers[i]->pos = i;
		}
	}
	
	if (std::find(mainprogram->busylayers.begin(), mainprogram->busylayers.end(), testlay) != mainprogram->busylayers.end()) {
		mainprogram->busylayers.erase(std::find(mainprogram->busylayers.begin(), mainprogram->busylayers.end(), testlay));
		mainprogram->busylist.erase(std::find(mainprogram->busylist.begin(), mainprogram->busylist.end(), testlay->filename));
	}
	avformat_close_input(&testlay->video);
	
	if (testlay->node) mainprogram->nodesmain->currpage->delete_node(testlay->node);
	
	if (testlay->clonesetnr != -1) {
		mainmix->clonesets[testlay->clonesetnr]->erase(testlay);
		if (mainmix->clonesets[testlay->clonesetnr]->size() == 0) {
			mainmix->clonesets.erase(mainmix->clonesets.begin() + testlay->clonesetnr);
			testlay->clonesetnr = -1;
		}
	}

	delete testlay;
}

void Mixer::delete_layer(std::vector<Layer*> &layers, Layer *testlay, bool add) {
	testlay->closethread = true;
	while (testlay->closethread) {
		testlay->ready = true;
		testlay->startdecode.notify_one();
	}
	
	testlay->audioplaying = false;
	
	this->do_deletelay(testlay, layers, add);
}


Layer::Layer() {
	Layer(true);
}

Layer::Layer(bool comp) {
	this->comp = comp;

    glClearColor(0, 0, 0, 0);

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
	}
	else {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, mainprogram->ow3, mainprogram->oh3, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glGenFramebuffers(1, &this->fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, this->fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->fbotex, 0);

    this->mutebut = new Button(false);
    this->mutebut->box->vtxcoords->y1 = 1.0f - mainprogram->layh - 0.05f;
	this->mutebut->box->vtxcoords->w = 0.03f;
   	this->mutebut->box->vtxcoords->h = 0.05f;
    this->mutebut->box->tooltiptitle = "Layer mute ";
    this->mutebut->box->tooltip = "Leftclick temporarily mutes/unmutes this layer. ";
    this->solobut = new Button(false);
    this->solobut->box->vtxcoords->y1 = 1.0f - mainprogram->layh - 0.05f;
	this->solobut->box->vtxcoords->w = 0.03f;
   	this->solobut->box->vtxcoords->h = 0.05f;
    this->solobut->box->tooltiptitle = "Layer solo ";
    this->solobut->box->tooltip = "Leftclick temporarily soloes/unsoloes this layer (all other layers in deck are muted). ";
    this->mixbox = new Box;
    this->mixbox->tooltiptitle = "Set layer mix mode ";
    this->mixbox->tooltip = "Left or rightclick for choosing how to mix this layer with the previous ones: blendmode or local wipe.  Also accesses colorkeying. ";
    this->colorbox = new Box;
    this->colorbox->acolor[3] = 1.0f;
    this->colorbox->tooltiptitle = "Set colorkey color ";
    this->colorbox->tooltip = "Leftclick to set colorkey color.  Either use colorwheel or leftclick anywhere on screen.  Hovering mouse shows color that will be selected. ";
    this->chtol = new Param;
    this->chtol->name = "Tolerance";
    this->chtol->value = 0.8f;
    this->chtol->range[0] = -0.1f;
    this->chtol->range[1] = 3.3f;
    this->chtol->shadervar = "colortol";
    this->chtol->box->tooltiptitle = "Set colorkey tolerance ";
    this->chtol->box->tooltip = "Leftdrag to set color tolerance (\"spread\") around colorkey color.  Doubleclicking allows numeric entry. ";
	this->speed = new Param;
	this->speed->name = "Speed"; 
	this->speed->value = 1.0f;
	this->speed->range[0] = 0.0f;
	this->speed->range[1] = 3.33f;
	this->speed->sliding = true;
	this->speed->powertwo = true;
	this->speed->box->acolor[0] = 0.5;
	this->speed->box->acolor[1] = 0.2;
	this->speed->box->acolor[2] = 0.5;
	this->speed->box->acolor[3] = 1.0;
    this->speed->box->tooltiptitle = "Set layer speed ";
	this->speed->box->tooltip = "Leftdrag sets current layer video playing speed factor. Doubleclicking allows numeric entry. ";
	this->opacity = new Param;
	this->opacity->name = "Opacity"; 
	this->opacity->value = 1.0f;
	this->opacity->range[0] = 0.0f;
	this->opacity->range[1] = 1.0f;
	this->opacity->sliding = true;
	this->opacity->box->acolor[0] = 0.5;
	this->opacity->box->acolor[1] = 0.2;
	this->opacity->box->acolor[2] = 0.5;
	this->opacity->box->acolor[3] = 1.0;
    this->opacity->box->tooltiptitle = "Set layer opacity ";
	this->opacity->box->tooltip = "Leftdrag sets current layer opacity. Doubleclicking allows numeric entry. ";
	this->volume = new Param;
	this->volume->name = "Volume"; 
	this->volume->value = 0.0f;
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
    this->playbut->box->tooltiptitle = "Toggle play ";
	this->playbut->box->tooltip = "Leftclick toggles current layer video normal play on/off. ";
	this->frameforward = new Button(false);
    this->frameforward->box->tooltiptitle = "Frame forward ";
	this->frameforward->box->tooltip = "Leftclick advances the current layer video by one frame. ";
	this->framebackward = new Button(false);
    this->framebackward->box->tooltiptitle = "Frame backward ";
	this->framebackward->box->tooltip = "Leftclick seeks back in the current layer video by one frame. ";
	this->revbut = new Button(false);
    this->revbut->box->tooltiptitle = "Toggle reverse play ";
	this->revbut->box->tooltip = "Leftclick toggles current layer video reverse play on/off. ";
	this->bouncebut = new Button(false);
    this->bouncebut->box->tooltiptitle = "Toggle bounce play ";
	this->bouncebut->box->tooltip = "Leftclick toggles current layer video bounce play on/off.  Bounce play plays the video first forward than backward. ";
	this->genmidibut = new Button(false);
    this->genmidibut->box->tooltiptitle = "Set layer MIDI preset ";
	this->genmidibut->box->tooltip = "Selects (leftclick advances) for this layer which MIDI preset (A, B, C, D or off) is used to control this layers common controls. ";
	this->loopbox = new Box;
    this->loopbox->tooltiptitle = "Loop bar ";
	this->loopbox->tooltip = "Loop bar for current layer video.  Green area is looped area, white vertical line is video position. .  Leftdrag on bar scrubs video.  When hovering over green area edges, it turns blue; when this happens middledrag will drag the areas edge.  If area is green, middledrag will drag the looparea left/right.  Rightclicking starts a menu allowing to set loop start or end to the current play position. ";
	this->chdir = new Button(false);
    this->chdir->box->tooltiptitle = "Toggle colorkey direction ";
	this->chdir->box->tooltip = "Leftclick toggles colorkey direction: does the previous layer stream image fill up the current layer's color or does the current layer fill a color in the previous layer stream image. ";
	this->chinv = new Button(false);
    this->chinv->box->tooltiptitle = "Toggle colorkey inverse modus ";
	this->chinv->box->tooltip = "Leftclick toggles colorkey inverse modus: either the selected color is exchanged or all colors but the selected color are exchanged. ";
	this->chdir->box->acolor[3] = 1.0f;
	this->chinv->box->acolor[3] = 1.0f;

    GLfloat vcoords[8];
 	GLfloat *p = vcoords;
	GLfloat tcoords[] = {0.0f, 1.0f,
						1.0f, 1.0f,
						0.0f, 0.0f,
						1.0f, 0.0f};

  GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        printf("%X\n", err);
    }

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

 	//glDisableVertexAttribArray(0);
 	//glDisableVertexAttribArray(1);

	this->decresult = new frame_result;
	this->decresult->data = nullptr;

    this->decoding = std::thread{&Layer::get_frame, this};
    this->decoding.detach();

	glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
}

Layer::~Layer() {
   	glDeleteTextures(1, &this->fbotex);
    glDeleteBuffers(1, &(this->vbuf));
    glDeleteBuffers(1, &(this->tbuf));
    glDeleteFramebuffers(1, &(this->fbo));
	glDeleteVertexArrays(1, &(this->vao));
}

void Layer::initialize(int w, int h) {
	this->iw = w;
	this->ih = h;
	std::vector<int> emptydata(this->iw * this->ih);
	std::fill(emptydata.begin(), emptydata.end(), 0x00000000);
	glBindTexture(GL_TEXTURE_2D, this->texture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
	int count = 0;
	while (1) {
		if (this->vidformat == 188 or this->vidformat == 187) {
			if (this->decresult->compression == 187) {
				glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, this->iw, this->ih, 0, GL_BGRA, GL_UNSIGNED_BYTE, &emptydata[0]);
			}
			else if (this->decresult->compression == 190) {
				glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, this->iw, this->ih, 0, GL_BGRA, GL_UNSIGNED_BYTE, &emptydata[0]);
			}
		}
		else {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, this->iw, this->ih, 0, GL_BGRA, GL_UNSIGNED_BYTE, &emptydata[0]);
		}
		if (count == this->effects[0].size()) break;
		glBindTexture(GL_TEXTURE_2D, this->effects[0][count]->fbotex);
		count++;
	}
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
	glBindTexture(GL_TEXTURE_2D, this->fbotex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
	for (int i = 0; i < this->effects[0].size(); i++) {
		glBindTexture(GL_TEXTURE_2D, this->effects[0][i]->fbotex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
	}
}

void Layer::open_image(const std::string &path) {
	ILboolean ret = ilLoadImage(path.c_str());
	if (ret == IL_FALSE) return;
	this->filename = path;
	this->vidformat = -1;
	int w = ilGetInteger(IL_IMAGE_WIDTH);
	int h = ilGetInteger(IL_IMAGE_HEIGHT);
	int bpp = ilGetInteger(IL_IMAGE_BPP);
	this->initialize(w, h);
	this->type = ELEM_IMAGE;
	this->decresult->width = w;
	this->decresult->height = h;
		
	glBindTexture(GL_TEXTURE_2D, this->texture);
	if (bpp == 3) {
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, (char*)ilGetData());
	}
	else if (bpp == 4) {
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_BGRA, GL_UNSIGNED_BYTE, (char*)ilGetData());
	}
}

void Layer::set_clones() {
	if (this->clonesetnr != -1) {
		std::unordered_set<Layer*>::iterator it;
		for (it = mainmix->clonesets[this->clonesetnr]->begin(); it != mainmix->clonesets[this->clonesetnr]->end(); it++) {
			Layer *lay = *it;
			lay->speed->value = this->speed->value;
			lay->opacity->value = this->opacity->value;
			lay->playbut->value = this->playbut->value;
			lay->revbut->value = this->revbut->value;
			lay->bouncebut->value = this->bouncebut->value;
			lay->genmidibut->value = this->genmidibut->value;
			lay->frame = this->frame;
			lay->startframe = this->startframe;
			lay->endframe = this->endframe;
		}
	}
}

Layer* Layer::next() {
	if (std::find(mainmix->layersA.begin(), mainmix->layersA.end(), this) != mainmix->layersA.end()) {
		if (this->pos < mainmix->layersA.size() - 1) return mainmix->layersA[this->pos + 1];
	}
	else if (std::find(mainmix->layersB.begin(), mainmix->layersB.end(), this) != mainmix->layersB.end()) {
		 if (this->pos < mainmix->layersB.size() - 1) return mainmix->layersB[this->pos + 1];
	}
	else if (std::find(mainmix->layersAcomp.begin(), mainmix->layersAcomp.end(), this) != mainmix->layersAcomp.end()) {
		 if (this->pos < mainmix->layersAcomp.size() - 1) return mainmix->layersAcomp[this->pos + 1];
	}
	else if (std::find(mainmix->layersBcomp.begin(), mainmix->layersBcomp.end(), this) != mainmix->layersBcomp.end()) {
		 if (this->pos < mainmix->layersBcomp.size() - 1) return mainmix->layersBcomp[this->pos + 1];
	}
	return this;
}	

Layer* Layer::prev() {
	if (this->pos > 0) {
		if (std::find(mainmix->layersA.begin(), mainmix->layersA.end(), this) != mainmix->layersA.end()) return mainmix->layersA[this->pos - 1];
		else if (std::find(mainmix->layersB.begin(), mainmix->layersB.end(), this) != mainmix->layersB.end()) return mainmix->layersB[this->pos - 1];
		else if (std::find(mainmix->layersAcomp.begin(), mainmix->layersAcomp.end(), this) != mainmix->layersAcomp.end()) return mainmix->layersAcomp[this->pos - 1];
		else if (std::find(mainmix->layersBcomp.begin(), mainmix->layersBcomp.end(), this) != mainmix->layersBcomp.end()) return mainmix->layersBcomp[this->pos - 1];
	}
	return this;
}	

void Mixer::set_values(Layer *clay, Layer *lay) {
	clay->speed->value = lay->speed->value;
	clay->playbut->value = lay->playbut->value;
	clay->revbut->value = lay->revbut->value;
	clay->bouncebut->value = lay->bouncebut->value;
	clay->playkind = lay->playkind;
	clay->genmidibut->value = lay->genmidibut->value;
	clay->pos = lay->pos;
	clay->deck = lay->deck;
	clay->aspectratio = lay->aspectratio;
	clay->shiftx = lay->shiftx;
	clay->shifty = lay->shifty;
	clay->scale = lay->scale;
	clay->opacity->value = lay->opacity->value;
	clay->volume->value = lay->volume->value;
	clay->chtol->value = lay->chtol->value;
	clay->chdir->value = lay->chdir->value;
	clay->chinv->value = lay->chinv->value;
	if (lay->type == ELEM_LIVE) {
		set_live_base(clay, lay->filename);
	}
	else if (lay->filename != "") open_video(lay->frame, clay, lay->filename, false);
	clay->millif = lay->millif;
	clay->prevtime = lay->prevtime;
	clay->frame = lay->frame;
	clay->prevframe = lay->frame - 1;
	clay->startframe = lay->startframe;
	clay->endframe = lay->endframe;
	clay->numf = lay->numf;
}

void Mixer::loopstation_copy(bool comp) {
	LoopStation *lp1;
	LoopStation *lp2;
	if (comp) {
		lp1 = lp;
		lp2 = lpc;
	}
	else {
		lp1 = lpc;
		lp2 = lp;
	}
	for (int i = 0; i < lp1->elems.size(); i++) {
		lp2->elems[i]->eventlist.clear();
		lp2->elems[i]->params.clear();
		lp2->elems[i]->layers.clear();
		for (int j = 0; j < lp1->elems[i]->eventlist.size(); j++) {
			std::tuple<long long, Param*, float> event1 = lp1->elems[i]->eventlist[j];
			std::tuple<long long, Param*, float> event2;
			event2 = std::make_tuple(std::get<0>(event1), lp1->parmap[std::get<1>(event1)], std::get<2>(event1));
			Param *par = std::get<1>(event2);
			if (par) {	
				lp2->elems[i]->eventlist.push_back(event2);
				lp2->elems[i]->params.emplace(par);
				if (par->effect) lp2->elems[i]->layers.emplace(par->effect->layer);
				lp2->elemmap[par] = lp2->elems[i];
				par->box->acolor[0] = lp2->elems[i]->colbox->acolor[0];
				par->box->acolor[1] = lp2->elems[i]->colbox->acolor[1];
				par->box->acolor[2] = lp2->elems[i]->colbox->acolor[2];
				par->box->acolor[3] = lp2->elems[i]->colbox->acolor[3];
			}
		}
		lp2->elems[i]->speed->value = lp1->elems[i]->speed->value;
		lp2->elems[i]->starttime = lp1->elems[i]->starttime;
		lp2->elems[i]->interimtime = lp1->elems[i]->interimtime;
		lp2->elems[i]->speedadaptedtime = lp1->elems[i]->speedadaptedtime;
		lp2->elems[i]->eventpos = lp1->elems[i]->eventpos;
		lp2->elems[i]->loopbut->value = lp1->elems[i]->loopbut->value;
		lp2->elems[i]->playbut->value = lp1->elems[i]->playbut->value;
	}
}

Layer* Mixer::clone_layer(std::vector<Layer*> &lvec, Layer* slay) {
	Layer *dlay = mainmix->add_layer(lvec, slay->pos + 1);
	this->set_values(dlay, slay);
	dlay->pos = slay->pos + 1;
	dlay->blendnode->blendtype = slay->blendnode->blendtype;
	dlay->blendnode->mixfac->value = slay->blendnode->mixfac->value;
	dlay->blendnode->chred = slay->blendnode->chred;
	dlay->blendnode->chgreen = slay->blendnode->chgreen;
	dlay->blendnode->chblue = slay->blendnode->chblue;
	dlay->blendnode->wipetype = slay->blendnode->wipetype;
	dlay->blendnode->wipedir = slay->blendnode->wipedir;
	dlay->blendnode->wipex = slay->blendnode->wipex;
	dlay->blendnode->wipey = slay->blendnode->wipey;
	
	int buval = mainprogram->effcat[dlay->deck]->value;
	mainprogram->effcat[dlay->deck]->value = 0;
	for (int i = 0; i < slay->effects[0].size(); i++) {
		Effect *eff = dlay->add_effect(slay->effects[0][i]->type, i);
		for (int j = 0; j < slay->effects[0][i]->params.size(); j++) {
			Param *par = slay->effects[0][i]->params[j];
			Param *cpar = eff->params[j];
			cpar->value = par->value;
			cpar->midi[0] = par->midi[0];
			cpar->midi[1] = par->midi[1];
			cpar->effect = eff;
			if (loopstation->elemmap.find(par) != loopstation->elemmap.end()) {
				for (int k = 0; k < loopstation->elemmap[par]->eventlist.size(); k++) {
					LoopStationElement *elem = loopstation->elemmap[par];
					std::tuple<long long, Param*, float> event1 = elem->eventlist[k];
					if (par == std::get<1>(event1)) {
						std::tuple<long long, Param*, float> event2;
						event2 = std::make_tuple(std::get<0>(event1), cpar, std::get<2>(event1));
						elem->eventlist.insert(elem->eventlist.begin() + k + 1, event2);
						elem->params.emplace(cpar);
						elem->layers.emplace(cpar->effect->layer);
						loopstation->elemmap[par] = elem;
						cpar->box->acolor[0] = elem->colbox->acolor[0];
						cpar->box->acolor[1] = elem->colbox->acolor[1];
						cpar->box->acolor[2] = elem->colbox->acolor[2];
						cpar->box->acolor[3] = elem->colbox->acolor[3];
					}
				}
			}
		}
	}
	mainprogram->effcat[dlay->deck]->value = buval;
	
	return dlay;
}

void Mixer::clonesets_copy(bool comp) {
	bool bupm = mainprogram->prevmodus;
	mainprogram->prevmodus = comp;
	std::vector<Layer*> &lvecA1 = choose_layers(0);
	std::vector<Layer*> &lvecB1 = choose_layers(1);
	mainprogram->prevmodus = !comp;
	std::vector<std::unordered_set<Layer*>*> tempclonesets;
	for (int i = 0; i < mainmix->clonesets.size(); i++) {
		if (std::find(lvecA1.begin(), lvecA1.end(), *mainmix->clonesets[i]->begin()) != lvecA1.end() or std::find(lvecB1.begin(), lvecB1.end(), *mainmix->clonesets[i]->begin()) != lvecB1.end()) {
			std::unordered_set<Layer*> *uset1 = new std::unordered_set<Layer*>;
			tempclonesets.push_back(uset1);
			std::unordered_set<Layer*>::iterator it;
			for (it = mainmix->clonesets[i]->begin(); it != mainmix->clonesets[i]->end(); it++) {
				Layer *lay = *it;
				uset1->emplace(lay);
				lay->clonesetnr = tempclonesets.size() - 1;
			}
			std::unordered_set<Layer*> *uset2 = new std::unordered_set<Layer*>;
			tempclonesets.push_back(uset2);
			for (it = mainmix->clonesets[i]->begin(); it != mainmix->clonesets[i]->end(); it++) {
				Layer *lay = *it;
				std::vector<Layer*> &lvec2 = choose_layers(lay->deck);
				uset2->emplace(lvec2[lay->pos]);
				lvec2[lay->pos]->clonesetnr = tempclonesets.size() - 1;
			}
		}
		delete mainmix->clonesets[i];
	}
		
	mainmix->clonesets = tempclonesets;
	mainprogram->prevmodus = bupm;
}			

void Mixer::lay_copy(std::vector<Layer*> &slayers, std::vector<Layer*> &dlayers, bool comp) {
	LoopStation *lp1;
	LoopStation *lp2;
	if (comp) {
		lp1 = lp;
		lp2 = lpc;
		lp1->parmap[mainmix->crossfade] = mainmix->crossfadecomp;
	}
	else {
		lp1 = lpc;
		lp2 = lp;
		lp1->parmap[mainmix->crossfadecomp] = mainmix->crossfade;
	}
	int pos = std::find(dlayers.begin(), dlayers.end(), mainmix->currlay) - dlayers.begin();
	while (!dlayers.empty()) {
		dlayers.back()->node = nullptr;
		dlayers.back()->blendnode = nullptr;
		mainmix->delete_layer(dlayers, dlayers.back(), false);
	}
	for (int i = 0; i < slayers.size(); i++) {
		Layer *lay = slayers[i];
		Layer *clay;
		if (&dlayers == &mainmix->layersA or &dlayers == &mainmix->layersB) {
			clay = new Layer(false);
			if (i == pos) mainmix->currlay = clay;
		}
		else {
			clay = new Layer(true);
		}
		this->set_values(clay, lay);
		dlayers.push_back(clay);
		clay->pos = dlayers.size() - 1;
		if (i == 0) { 	  
			clay->blendnode = new BlendNode;
			clay->blendnode->blendtype = lay->blendnode->blendtype;
			clay->blendnode->mixfac->value = lay->blendnode->mixfac->value;
			clay->blendnode->chred = lay->blendnode->chred;
			clay->blendnode->chgreen = lay->blendnode->chgreen;
			clay->blendnode->chblue = lay->blendnode->chblue;
			clay->blendnode->wipetype = lay->blendnode->wipetype;
			clay->blendnode->wipedir = lay->blendnode->wipedir;
			clay->blendnode->wipex = lay->blendnode->wipex;
			clay->blendnode->wipey = lay->blendnode->wipey;
		}

		dlayers[i]->effects[0].clear();
		for (int m = 0; m < 2; m++) {
			for (int j = 0; j < slayers[i]->effects[m].size(); j++) {
				Effect *eff = slayers[i]->effects[m][j];
				Effect *ceff;
				switch (eff->type) {
					case BLUR:
						ceff = new BlurEffect();
						break;
					case BRIGHTNESS:
						ceff = new BrightnessEffect();
						break;
					case CHROMAROTATE:
						ceff = new ChromarotateEffect();
						break;
					case CONTRAST:
						ceff = new ContrastEffect();
						break;
					case DOT:
						ceff = new DotEffect();
						break;
					case GLOW:
						ceff = new GlowEffect();
						break;
					case RADIALBLUR:
						ceff = new RadialblurEffect();
						break;
					case SATURATION:
						ceff = new SaturationEffect();
						break;
					case SCALE:
						ceff = new ScaleEffect();
						break;
					case SWIRL:
						ceff = new SwirlEffect();
						break;
					case OLDFILM:
						ceff = new OldFilmEffect();
						break;
					case RIPPLE:
						ceff = new RippleEffect();
						break;
					case FISHEYE:
						ceff = new FishEyeEffect();
						break;
					case TRESHOLD:
						ceff = new TresholdEffect();
						break;
					case STROBE:
						ceff = new StrobeEffect();
						break;
					case POSTERIZE:
						ceff = new PosterizeEffect();
						break;
					case PIXELATE:
						ceff = new PixelateEffect();
						break;
					case CROSSHATCH:
						ceff = new CrosshatchEffect();
						break;
					case INVERT:
						ceff = new InvertEffect();
						break;
					case ROTATE:
						ceff = new RotateEffect();
						break;
					case EMBOSS:
						ceff = new EmbossEffect();
						break;
					case ASCII:
						ceff = new AsciiEffect();
						break;
					case SOLARIZE:
						ceff = new SolarizeEffect();
						break;
					case VARDOT:
						ceff = new VarDotEffect();
						break;
					case CRT:
						ceff = new CRTEffect();
						break;
					case EDGEDETECT:
						ceff = new EdgeDetectEffect();
						break;
					case KALEIDOSCOPE:
						ceff = new KaleidoScopeEffect();
						break;
					case HTONE:
						ceff = new HalfToneEffect();
						break;
					case CARTOON:
						ceff = new CartoonEffect();
						break;
					case CUTOFF:
						ceff = new CutoffEffect();
						break;
					case GLITCH:
						ceff = new GlitchEffect();
						break;
					case COLORIZE:
						ceff = new ColorizeEffect();
						break;
					case NOISE:
						ceff = new NoiseEffect();
						break;
					case GAMMA:
						ceff = new GammaEffect();
						break;
					case THERMAL:
						ceff = new ThermalEffect();
						break;
					case BOKEH:
						ceff = new BokehEffect();
						break;
					case SHARPEN:
						ceff = new SharpenEffect();
						break;
					case DITHER:
						ceff = new DitherEffect();
						break;
				}
				ceff->type = eff->type;
				ceff->layer = dlayers[i];
				ceff->onoffbutton->value = eff->onoffbutton->value;
				ceff->drywet->value = eff->drywet->value;
				if (ceff->type == RIPPLE) ((RippleEffect*)ceff)->speed = ((RippleEffect*)eff)->speed;
				if (ceff->type == RIPPLE) ((RippleEffect*)ceff)->ripplecount = ((RippleEffect*)eff)->ripplecount;
				dlayers[i]->effects[m].push_back(ceff);
				for (int k = 0; k < slayers[i]->effects[m][j]->params.size(); k++) {
					Param *par = slayers[i]->effects[m][j]->params[k];
					Param *cpar = dlayers[i]->effects[m][j]->params[k];
					lp1->parmap[par] = cpar;
					cpar->value = par->value;
					cpar->midi[0] = par->midi[0];
					cpar->midi[1] = par->midi[1];
					cpar->effect = ceff;
					lp2->allparams.push_back(cpar);
				}
			}
		}
		clay->aspectratio = lay->aspectratio;
		if (lay->video_dec_ctx) clay->set_aspectratio(lay->video_dec_ctx->width, lay->video_dec_ctx->height);
	}
}

void Mixer::copy_to_comp(std::vector<Layer*> &sourcelayersA, std::vector<Layer*> &destlayersA, std::vector<Layer*> &sourcelayersB, std::vector<Layer*> &destlayersB, std::vector<Node*> &sourcenodes, std::vector<Node*> &destnodes, std::vector<MixNode*> &destmixnodes, bool comp) {
	if (sourcelayersA == mainmix->layersA) {
		mainmix->crossfadecomp->value = mainmix->crossfade->value;
		mainmix->wipe[1] = mainmix->wipe[0];
		mainmix->wipedir[1] = mainmix->wipedir[0];
		mainmix->wipex[1] = mainmix->wipex[0];
		mainmix->wipey[1] = mainmix->wipey[0];
	}
	else {
		mainmix->crossfade->value = mainmix->crossfadecomp->value;
		mainmix->wipe[0] = mainmix->wipe[1];
		mainmix->wipedir[0] = mainmix->wipedir[1];
		mainmix->wipex[0] = mainmix->wipex[1];
		mainmix->wipey[0] = mainmix->wipey[1];
	}
	
	this->lay_copy(sourcelayersA, destlayersA, comp);
	this->lay_copy(sourcelayersB, destlayersB, comp);
	this->loopstation_copy(comp);
	this->clonesets_copy(comp);
	
	for (int i = 0; i < destnodes.size(); i++) {
		delete destnodes[i];
	}
	destmixnodes.clear();
	
	destnodes.clear();
	for (int i = 0; i < 1; i++) { //mainprogram->nodesmain->pages.size()
		for (int j = 0; j < sourcenodes.size(); j++) {
			Node *node = sourcenodes[j];
			if (node->type == VIDEO) {
				VideoNode *cnode = mainprogram->nodesmain->currpage->add_videonode(comp);
				for (int k = 0; k < sourcelayersA.size(); k++) {
					if (sourcelayersA[k] == ((VideoNode*)(node))->layer) {
						cnode->layer = destlayersA[k];
						cnode->layer->node = cnode;
					}
				}
				for (int k = 0; k < sourcelayersB.size(); k++) {
					if (sourcelayersB[k] == ((VideoNode*)(node))->layer) {
						cnode->layer = destlayersB[k];
						cnode->layer->node = cnode;
					}
				}
			}
			else if (node->type == MIX) {
				MixNode *cnode = mainprogram->nodesmain->currpage->add_mixnode(0, comp);
			}
			else if (node->type == EFFECT) {
				EffectNode *cnode = new EffectNode();
				for (int k = 0; k < sourcelayersA.size(); k++) {
					for (int m = 0; m < sourcelayersA[k]->effects[0].size(); m++) {
						Effect *eff = sourcelayersA[k]->effects[0][m];
						if (eff == ((EffectNode*)node)->effect) {
							cnode->effect = destlayersA[k]->effects[0][m];
							destlayersA[k]->effects[0][m]->node = cnode;
							break;
						}
					}
				}
				for (int k = 0; k < sourcelayersB.size(); k++) {
					for (int m = 0; m < sourcelayersB[k]->effects[0].size(); m++) {
						Effect *eff = sourcelayersB[k]->effects[0][m];
						if (eff == ((EffectNode*)node)->effect) {
							cnode->effect = destlayersB[k]->effects[0][m];
							destlayersB[k]->effects[0][m]->node = cnode;
							break;
						}
					}
				}
				for (int k = 0; k < sourcelayersA.size(); k++) {
					for (int m = 0; m < sourcelayersA[k]->effects[1].size(); m++) {
						Effect *eff = sourcelayersA[k]->effects[1][m];
						if (eff == ((EffectNode*)node)->effect) {
							cnode->effect = destlayersA[k]->effects[1][m];
							destlayersA[k]->effects[1][m]->node = cnode;
							break;
						}
					}
				}
				for (int k = 0; k < sourcelayersB.size(); k++) {
					for (int m = 0; m < sourcelayersB[k]->effects[1].size(); m++) {
						Effect *eff = sourcelayersB[k]->effects[1][m];
						if (eff == ((EffectNode*)node)->effect) {
							cnode->effect = destlayersB[k]->effects[1][m];
							destlayersB[k]->effects[1][m]->node = cnode;
							break;
						}
					}
				}
				destnodes.push_back(cnode);
			}
			else if (node->type == BLEND) {
				BlendNode *cnode = new BlendNode();
				if (node == mainprogram->bnodeend) mainprogram->bnodeendcomp = cnode;
				else if (node == mainprogram->bnodeendcomp) mainprogram->bnodeend = cnode;
				for (int i = 0; i < sourcelayersA.size(); i++) {
					if (sourcelayersA[i]->blendnode == node) destlayersA[i]->blendnode = cnode;
				}
				for (int i = 0; i < sourcelayersB.size(); i++) {
					if (sourcelayersB[i]->blendnode == node) destlayersB[i]->blendnode = cnode;
				}
				cnode->blendtype = ((BlendNode*)node)->blendtype;
				cnode->mixfac->value = ((BlendNode*)node)->mixfac->value;
				cnode->chred = ((BlendNode*)node)->chred;
				cnode->chblue = ((BlendNode*)node)->chblue;
				cnode->chgreen = ((BlendNode*)node)->chgreen;
				cnode->wipetype = ((BlendNode*)node)->wipetype;
				cnode->wipedir = ((BlendNode*)node)->wipedir;
				cnode->wipex = ((BlendNode*)node)->wipex;
				cnode->wipey = ((BlendNode*)node)->wipey;
				destnodes.push_back(cnode);
			}
			else if (node->type == MIDI) {
				MidiNode *mnode = new MidiNode();
				destnodes.push_back(mnode);
			}
		}
		for (int j = 0; j < sourcenodes.size(); j++) {
			Node *node = sourcenodes[j];
			Node *cnode = destnodes[j];
			if (node->in) {
				for (int k = 0; k < sourcenodes.size(); k++) {
					Node *tnode = sourcenodes[k];
					if (node->in == tnode) {
						Node *innode = destnodes[k];
						cnode->in = innode;
						innode->out.push_back(cnode);
					}
				}
			}
			if (node->type == BLEND) {
				if (((BlendNode*)node)->in2) {
					for (int k = 0; k < sourcenodes.size(); k++) {
						Node *tnode = sourcenodes[k];
						if (((BlendNode*)node)->in2 == tnode) {
							Node *innode = destnodes[k];
							mainprogram->nodesmain->pages[i]->connect_in2(innode, ((BlendNode*)cnode));
						}
					}
				}
			}
			for (int m = 0; m < sourcelayersA.size(); m++) {
				if ((BlendNode*)node == sourcelayersA[m]->lasteffnode[0]) destlayersA[m]->lasteffnode[0] = cnode;
				if ((BlendNode*)node == sourcelayersA[m]->blendnode) destlayersA[m]->blendnode = (BlendNode*)cnode;
			}
			for (int m = 0; m < sourcelayersB.size(); m++) {
				if ((BlendNode*)node == sourcelayersB[m]->lasteffnode[0]) destlayersB[m]->lasteffnode[0] = cnode;
				if ((BlendNode*)node == sourcelayersB[m]->blendnode) destlayersB[m]->blendnode = (BlendNode*)cnode;
			}
		}
	}
	
	// setting lasteffnode[1]s
	for (int i = 0; i < destlayersA.size(); i++) {
		if (destlayersA[i]->pos == 0) {
			destlayersA[i]->lasteffnode[1] = destlayersA[i]->lasteffnode[0];
		}
		else {
			if (destlayersA[i]->effects[1].size()) {
				destlayersA[i]->lasteffnode[1] = destlayersA[i]->effects[1].back()->node;
			}
			else {
				destlayersA[i]->lasteffnode[1] = destlayersA[i]->blendnode;
			}
		}
	}
	for (int i = 0; i < destlayersB.size(); i++) {
		if (destlayersB[i]->pos == 0) {
			destlayersB[i]->lasteffnode[1] = destlayersB[i]->lasteffnode[0];
		}
		else {
			if (destlayersB[i]->effects[1].size()) {
				destlayersB[i]->lasteffnode[1] = destlayersB[i]->effects[1].back()->node;
			}
			else {
				destlayersB[i]->lasteffnode[1] = destlayersB[i]->blendnode;
			}
		}
	}
	
	make_layboxes();
	
}


std::vector<std::string> Mixer::write_layer(Layer *lay, std::ostream& wfile, bool doclips) {
	wfile << "POS\n";
	wfile << std::to_string(lay->pos);
	wfile << "\n";
	wfile << "TYPE\n";
	wfile << std::to_string(lay->type);
	wfile << "\n";
	wfile << "FILENAME\n";
	wfile << lay->filename;
	wfile << "\n";
	wfile << "RELPATH\n";
	wfile << "./" + boost::filesystem::relative(lay->filename, "./").string();
	wfile << "\n";
	if (lay->filename != "") {
		wfile << "WIDTH\n";
		wfile << std::to_string(lay->video_dec_ctx->width);
		wfile << "\n";
		wfile << "HEIGHT\n";
		wfile << std::to_string(lay->video_dec_ctx->height);
		wfile << "\n";
	}
	wfile << "ASPECTRATIO\n";
	wfile << std::to_string(lay->aspectratio);
	wfile << "\n";
	wfile << "MUTE\n";
	wfile << std::to_string(lay->mutebut->value);
	wfile << "\n";
	wfile << "SOLO\n";
	wfile << std::to_string(lay->solobut->value);
	wfile << "\n";
	wfile << "CLONESETNR\n";
	wfile << std::to_string(lay->clonesetnr);
	wfile << "\n";
	if (lay->type != ELEM_LIVE and lay->type != ELEM_IMAGE) {
		wfile << "SPEEDVAL\n";
		wfile << std::to_string(lay->speed->value);
		wfile << "\n";
		wfile << "PLAYBUTVAL\n";
		wfile << std::to_string(lay->playbut->value);;
		wfile << "\n";
		wfile << "REVBUTVAL\n";
		wfile << std::to_string(lay->revbut->value);;
		wfile << "\n";
		wfile << "BOUNCEBUTVAL\n";
		wfile << std::to_string(lay->bouncebut->value);;
		wfile << "\n";
		wfile << "PLAYKIND\n";
		wfile << std::to_string(lay->playkind);;
		wfile << "\n";
		wfile << "GENMIDIBUTVAL\n";
		wfile << std::to_string(lay->genmidibut->value);;
		wfile << "\n";
	}
	wfile << "SHIFTX\n";
	wfile << std::to_string(lay->shiftx);
	wfile << "\n";
	wfile << "SHIFTY\n";
	wfile << std::to_string(lay->shifty);
	wfile << "\n";
	wfile << "SCALE\n";
	wfile << std::to_string(lay->scale);
	wfile << "\n";
	wfile << "OPACITYVAL\n";
	wfile << std::to_string(lay->opacity->value);
	wfile << "\n";
	wfile << "VOLUMEVAL\n";
	wfile << std::to_string(lay->volume->value);
	wfile << "\n";
	wfile << "CHTOLVAL\n";
	wfile << std::to_string(lay->chtol->value);
	wfile << "\n";
	wfile << "CHDIRVAL\n";
	wfile << std::to_string(lay->chdir->value);
	wfile << "\n";
	wfile << "CHINVVAL\n";
	wfile << std::to_string(lay->chinv->value);
	wfile << "\n";
	if (lay->type != ELEM_LIVE and lay->type != ELEM_IMAGE) {
		wfile << "FRAME\n";
		wfile << std::to_string(lay->frame);
		wfile << "\n";
		wfile << "STARTFRAME\n";
		wfile << std::to_string(lay->startframe);
		wfile << "\n";
		wfile << "ENDFRAME\n";
		wfile << std::to_string(lay->endframe);
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
	wfile << std::to_string(lay->blendnode->wipex);
	wfile << "\n";
	wfile << "WIPEY\n";
	wfile << std::to_string(lay->blendnode->wipey);
	wfile << "\n";
	
	std::vector<std::string> jpegpaths;
	if (doclips) {
		wfile << "CLIPS\n";
		int size = std::clamp((int)(lay->clips.size() - 1), 0, (int)(lay->clips.size() - 1));
		for (int j = 0; j < size; j++) {
			Clip *clip = lay->clips[j];
			wfile << "TYPE\n";
			wfile << std::to_string(clip->type);
			wfile << "\n";
			wfile << "FILENAME\n";
			wfile << clip->path;
			wfile << "\n";
			if (clip->type == ELEM_LAYER) {
				wfile << "CLIPLAYER\n";
				std::ifstream rfile;
				std::string result = deconcat_files(clip->path);
				if (result != "") rfile.open(result);
				else rfile.open(clip->path);
				std::string istring;				
				while (getline(rfile, istring)) {
					wfile << istring;
					wfile << "\n";
					if (istring == "ENDOFFILE") break;
				}
				wfile << "ENDOFCLIPLAYER\n";
			}
			std::string clipjpegpath;
			std::string name = remove_extension(basename(clip->path));
			int count = 0;
			while (1) {
				clipjpegpath = mainprogram->temppath + name + ".jpg";
				if (!exists(clipjpegpath)) {
					break;
				}
				count++;
				name = remove_version(name) + "_" + std::to_string(count);
			}
			save_thumb(clipjpegpath, clip->tex);
			jpegpaths.push_back(clipjpegpath);
			wfile << "JPEGPATH\n";
			wfile << clipjpegpath;
			wfile << "\n";
		}
		wfile << "ENDOFCLIPS\n";
	}
	
	wfile << "EFFECTS\n";
	for (int j = 0; j < lay->effects[0].size(); j++) {
		Effect *eff = lay->effects[0][j];
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
			Param *par = eff->params[k];
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
			mainmix->event_write(wfile, par);
		}
		wfile << "ENDOFPARAMS\n";
	}
	wfile << "ENDOFEFFECTS\n";
	
	wfile << "STREAMEFFECTS\n";
	for (int j = 0; j < lay->effects[1].size(); j++) {
		Effect *eff = lay->effects[1][j];
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
			Param *par = eff->params[k];
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
			mainmix->event_write(wfile, par);
		}
		wfile << "ENDOFPARAMS\n";
	}
	wfile << "ENDOFSTREAMEFFECTS\n";
	
	wfile << "EFFCAT\n";
	wfile << std::to_string(mainprogram->effcat[lay->deck]->value);
	wfile << "\n";
	
	return jpegpaths;
}

void Mixer::save_layerfile(const std::string &path, Layer *lay, bool doclips) {
	std::string ext = path.substr(path.length() - 6, std::string::npos);
	std::string str;
	if (ext != ".layer") str = path + ".layer";
	else str = path;
	std::ofstream wfile;
	wfile.open(str);
	wfile << "EWOCvj LAYERFILE V0.2\n";
		
	std::vector<std::vector<std::string>> jpegpaths;
	jpegpaths.push_back(mainmix->write_layer(lay, wfile, doclips));
	
	wfile << "ENDOFFILE\n";
	wfile.close();
	
    std::ofstream outputfile;
	outputfile.open(mainprogram->temppath + "tempconcat", std::ios::out | std::ios::binary);
	concat_files(outputfile, str, jpegpaths);
	outputfile.close();
	boost::filesystem::rename(mainprogram->temppath + "tempconcat", str);
}

void Mixer::save_state(const std::string &path) {
	std::vector<std::string> filestoadd;
	std::string ext = path.substr(path.length() - 6, std::string::npos);
	std::string str;
	if (ext != ".state") str = path + ".state";
	else str = path;
	std::ofstream wfile;
	wfile.open(str);
	wfile << "EWOCvj SAVESTATE V0.2\n";
	wfile << "PREVMODUS\n";
	wfile << std::to_string(mainprogram->prevmodus);
	wfile << "\n";
	wfile << "TOSCREENMIDI0\n";
	wfile << std::to_string(mainprogram->toscreen->midi[0]);
	wfile << "\n";
	wfile << "TOSCREENMIDI1\n";
	wfile << std::to_string(mainprogram->toscreen->midi[1]);
	wfile << "\n";
	wfile << "TOSCREENMIDIPORT\n";
	wfile << mainprogram->toscreen->midiport;
	wfile << "\n";
	wfile << "BACKTOPREMIDI0\n";
	wfile << std::to_string(mainprogram->backtopre->midi[0]);
	wfile << "\n";
	wfile << "BACKTOPREMIDI1\n";
	wfile << std::to_string(mainprogram->backtopre->midi[1]);
	wfile << "\n";
	wfile << "BACKTOPREMIDIPORT\n";
	wfile << mainprogram->backtopre->midiport;
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
	wfile << "ENDOFFILE\n";
	wfile.close();
	
	mainprogram->shelves[0]->save(mainprogram->temppath + "tempA.shelf");
	filestoadd.push_back(mainprogram->temppath + "tempA.shelf");
	mainprogram->shelves[1]->save(mainprogram->temppath + "tempB.shelf");
	filestoadd.push_back(mainprogram->temppath + "tempB.shelf");
	bool save = mainprogram->prevmodus;
	mainprogram->prevmodus = true;
	mainmix->save_mix(mainprogram->temppath + "temp.state1");
	filestoadd.push_back(mainprogram->temppath + "temp.state1.mix");
	mainprogram->prevmodus = false;
	mainmix->save_mix(mainprogram->temppath + "temp.state2");
	filestoadd.push_back(mainprogram->temppath + "temp.state2.mix");
	//save_genmidis(remove_extension(path) + ".midi");
	mainprogram->prevmodus = save;
	
    std::ofstream outputfile;
	outputfile.open(mainprogram->temppath + "tempconcatstate", std::ios::out | std::ios::binary);
	std::vector<std::vector<std::string>> filestoadd2;
	filestoadd2.push_back(filestoadd);
	concat_files(outputfile, str, filestoadd2);
	outputfile.close();
	boost::filesystem::rename(mainprogram->temppath + "tempconcatstate", str);
}

void Mixer::event_write(std::ostream &wfile, Param *par) {
	for (int i = 0; i < loopstation->elems.size(); i++) {
		LoopStationElement *elem = loopstation->elems[i];
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
					wfile << std::to_string(std::get<2>(elem->eventlist[j]));
					wfile << "\n";
				}
			}
			wfile << "ENDOFEVENT\n";
		}
	}
}

void Mixer::event_read(std::istream &rfile, Param *par, Layer *lay) {
	// load loopstation events for this parameter
	std::string istring;
	LoopStationElement *loop = nullptr;
	getline(rfile, istring);
	int elemnr = std::stoi(istring);
	LoopStation *ls;
	if (mainprogram->prevmodus) ls = lp;
	else ls = lpc;
	if (std::find(ls->readelemnrs.begin(), ls->readelemnrs.end(), elemnr) == ls->readelemnrs.end()) {
		// new loopstation line taken in use
		loop = ls->free_element();
		ls->readelemnrs.push_back(elemnr);
		ls->readelems.push_back(loop);
		printf("loop %d\n", loop);
	}
	else {
		// other parameter(s) of this rfile using this loopstation line already exist
		loop = ls->readelems[std::find(ls->readelemnrs.begin(), ls->readelemnrs.end(), elemnr) - ls->readelemnrs.begin()];
	}
	getline(rfile, istring);
	if (loop) {
		loop->loopbut->value = std::stoi(istring);
		loop->loopbut->oldvalue = std::stoi(istring);
		if (loop->loopbut->value) {
			loop->eventpos = 0;
			loop->starttime = std::chrono::high_resolution_clock::now();
		}
	}
	getline(rfile, istring);
	if (loop) {
		loop->playbut->value = std::stoi(istring);
		loop->playbut->oldvalue = std::stoi(istring);
		if (loop->playbut->value) {
			loop->eventpos = 0;
			loop->starttime = std::chrono::high_resolution_clock::now();
		}
	}
	getline(rfile, istring);
	if (loop) {
		loop->speed->value = std::stof(istring);
		loop->layers.emplace(lay);
	}
	while (getline(rfile, istring)) {
		if (istring == "ENDOFEVENT") break;
		int time = std::stoi(istring);
		getline(rfile, istring);
		float value = std::stof(istring);
		bool inserted = false;
		if (loop) {
			for (int i = 0; i < loop->eventlist.size(); i++) {
				if (time < std::get<0>(loop->eventlist[i])) {
					loop->eventlist.insert(loop->eventlist.begin() + i, std::make_tuple(time, par, value));
					inserted = true;
					break;
				}
			}
			if (!inserted) {
				loop->eventlist.push_back(std::make_tuple(time, par, value));
			}
			loop->params.emplace(par);
			ls->elemmap[par] = loop;
			if (par->effect) {
				loop->layers.emplace(par->effect->layer);
			}
			par->box->acolor[0] = loop->colbox->acolor[0];
			par->box->acolor[1] = loop->colbox->acolor[1];
			par->box->acolor[2] = loop->colbox->acolor[2];
			par->box->acolor[3] = loop->colbox->acolor[3];
		}
	}
}

void Mixer::save_mix(const std::string &path) {
	std::string ext = path.substr(path.length() - 4, std::string::npos);
	std::string str;
	if (ext != ".mix") str = path + ".mix";
	else str = path;
	std::ofstream wfile;
	wfile.open(str);
	wfile << "EWOCvj SAVEMIX V0.2\n";
	
	wfile << "CURRLAY\n";
	wfile << std::to_string(mainmix->currlay->pos);
	wfile << "\n";
	wfile << "CURRDECK\n";
	wfile << std::to_string(mainmix->currlay->deck);
	wfile << "\n";
	wfile << "CROSSFADE\n";
	wfile << std::to_string(mainmix->crossfade->value);
	wfile << "\n";
	wfile << "CROSSFADEEVENT\n";
	Param *par = mainmix->crossfade;
	mainmix->event_write(wfile, par);
	wfile << "CROSSFADECOMP\n";
	wfile << std::to_string(mainmix->crossfadecomp->value);
	wfile << "\n";
	wfile << "CROSSFADECOMPEVENT\n";
	par = mainmix->crossfadecomp;
	mainmix->event_write(wfile, par);
	wfile << "DECKSPEEDA\n";
	wfile << std::to_string(mainprogram->deckspeed[0]->value);
	wfile << "\n";
	wfile << "DECKSPEEDAMIDI0\n";
	wfile << std::to_string(mainprogram->deckspeed[0]->midi[0]);
	wfile << "\n";
	wfile << "DECKSPEEDAMIDI1\n";
	wfile << std::to_string(mainprogram->deckspeed[0]->midi[1]);
	wfile << "\n";
	wfile << "DECKSPEEDAMIDIPORT\n";
	wfile << mainprogram->deckspeed[0]->midiport;
	wfile << "\n";
	wfile << "DECKSPEEDB\n";
	wfile << std::to_string(mainprogram->deckspeed[1]->value);
	wfile << "\n";
	wfile << "DECKSPEEDBMIDI0\n";
	wfile << std::to_string(mainprogram->deckspeed[1]->midi[0]);
	wfile << "\n";
	wfile << "DECKSPEEDBMIDI1\n";
	wfile << std::to_string(mainprogram->deckspeed[1]->midi[1]);
	wfile << "\n";
	wfile << "DECKSPEEDBMIDIPORT\n";
	wfile << mainprogram->deckspeed[1]->midiport;
	wfile << "\n";
	wfile << "WIPE\n";
	wfile << std::to_string(mainmix->wipe[0]);
	wfile << "\n";
	wfile << "WIPEDIR\n";
	wfile << std::to_string(mainmix->wipedir[0]);
	wfile << "\n";
	wfile << "WIPECOMP\n";
	wfile << std::to_string(mainmix->wipe[1]);
	wfile << "\n";
	wfile << "WIPEDIRCOMP\n";
	wfile << std::to_string(mainmix->wipedir[1]);
	wfile << "\n";
	
	std::vector<std::vector<std::string>> jpegpaths;
	wfile << "LAYERSA\n";
	std::vector<Layer*> &lvec1 = choose_layers(0);
	for (int i = 0; i < lvec1.size(); i++) {
		Layer *lay = lvec1[i];
		jpegpaths.push_back(mainmix->write_layer(lay, wfile, 1));
	}
	
	wfile << "LAYERSB\n";
	std::vector<Layer*> &lvec2 = choose_layers(1);
	for (int i = 0; i < lvec2.size(); i++) {
		Layer *lay = lvec2[i];
		jpegpaths.push_back(mainmix->write_layer(lay, wfile, 1));
	}
	
	wfile << "ENDOFFILE\n";
	wfile.close();
	
    std::ofstream outputfile;
	outputfile.open(mainprogram->temppath + "/tempconcat", std::ios::out | std::ios::binary);
	concat_files(outputfile, str, jpegpaths);
	outputfile.close();
	boost::filesystem::rename(mainprogram->temppath + "/tempconcat", str);
}

void Mixer::save_deck(const std::string &path) {
	std::string ext = path.substr(path.length() - 5, std::string::npos);
	std::string str;
	if (ext != ".deck") str = path + ".deck";
	else str = path;
	std::ofstream wfile;
	wfile.open(str);
	wfile << "EWOCvj DECKFILE V0.2\n";
	
	wfile << "DECKSPEED\n";
	wfile << std::to_string(mainprogram->deckspeed[mainmix->mousedeck]->value);
	wfile << "\n";
	wfile << "DECKSPEEDMIDI0\n";
	wfile << std::to_string(mainprogram->deckspeed[mainmix->mousedeck]->midi[0]);
	wfile << "\n";
	wfile << "DECKSPEEDMIDI1\n";
	wfile << std::to_string(mainprogram->deckspeed[mainmix->mousedeck]->midi[1]);
	wfile << "\n";
	wfile << "DECKSPEEDMIDIPORT\n";
	wfile << mainprogram->deckspeed[mainmix->mousedeck]->midiport;
	wfile << "\n";
	
	std::vector<std::vector<std::string>> jpegpaths;
	std::vector<Layer*> &lvec = choose_layers(mainmix->mousedeck);
	for (int i = 0; i < lvec.size(); i++) {
		Layer *lay = lvec[i];
		jpegpaths.push_back(mainmix->write_layer(lay, wfile, 1));
	}
	
	wfile << "ENDOFFILE\n";
	wfile.close();
	
    std::ofstream outputfile;
	outputfile.open(mainprogram->temppath + "/tempconcat", std::ios::out | std::ios::binary);
	concat_files(outputfile, str, jpegpaths);
	outputfile.close();
	boost::filesystem::rename(mainprogram->temppath + "/tempconcat", str);
}

void Mixer::open_layerfile(const std::string &path, Layer *lay, bool loadevents, bool doclips) {
	std::string result = deconcat_files(path);
	bool concat = (result != "");
	std::ifstream rfile;
	if (concat) rfile.open(result);
	else rfile.open(path);
	
	std::string istring;
	
	Node *nextnode;
	if (lay->lasteffnode[0]) {
		if (lay->lasteffnode[0]->out.size()) nextnode = lay->lasteffnode[0]->out[0];
	}
	if (lay->node) {
		if (lay->node != lay->lasteffnode[0]) {
			if (lay->pos > 0) {
				((BlendNode*)nextnode)->in2 = nullptr;
				mainprogram->nodesmain->currpage->connect_in2(lay->node, (BlendNode*)nextnode);
			}
			else {
				nextnode->in = nullptr;
				mainprogram->nodesmain->currpage->connect_nodes(lay->node, nextnode);
			}
		}
		lay->lasteffnode[0] = lay->node;
	}
	while (!lay->effects[0].empty()) {
		mainprogram->nodesmain->currpage->delete_node(lay->effects[0].back()->node);
		for (int j = 0; j < lay->effects[0].back()->params.size(); j++) {
			delete lay->effects[0].back()->params[j];
		}
		delete lay->effects[0].back();
		lay->effects[0].pop_back();
	}
	loopstation->readelems.clear();
	loopstation->readelemnrs.clear();
	std::vector<Layer*> layers;
	layers.push_back(lay);
	mainmix->read_layers(rfile, result, layers, lay->deck, 0, doclips, concat, 1, loadevents);
	
	rfile.close();
}

void Mixer::open_state(const std::string &path) {
	std::string result = deconcat_files(path);
	bool concat = (result != "");
	std::ifstream rfile;
	if (concat) rfile.open(result);
	else rfile.open(path);
	
	std::string istring;
	getline(rfile, istring);
	
	mainprogram->set_ow3oh3();
    glBindTexture(GL_TEXTURE_2D, mainprogram->fbotex[2]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, mainprogram->ow, mainprogram->oh, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, mainprogram->fbotex[3]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, mainprogram->ow, mainprogram->oh, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, mainprogram->bnodeend->fbotex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, mainprogram->ow, mainprogram->oh, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, mainprogram->bnodeend->fbotex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, mainprogram->ow, mainprogram->oh, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
	for (int i = 0; i < mainprogram->nodesmain->mixnodes.size(); i++) {
		glBindTexture(GL_TEXTURE_2D, mainprogram->nodesmain->mixnodes[i]->mixtex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, mainprogram->ow3, mainprogram->oh3, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
	}
	for (int i = 0; i < mainprogram->nodesmain->mixnodescomp.size(); i++) {
		glDeleteTextures(1, &mainprogram->nodesmain->mixnodescomp[i]->mixtex);
		glDeleteFramebuffers(1, &mainprogram->nodesmain->mixnodescomp[i]->mixfbo);
		glGenTextures(1, &mainprogram->nodesmain->mixnodescomp[i]->mixtex);
		glBindTexture(GL_TEXTURE_2D, mainprogram->nodesmain->mixnodescomp[i]->mixtex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, mainprogram->ow, mainprogram->oh, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glGenFramebuffers(1, &mainprogram->nodesmain->mixnodescomp[i]->mixfbo);
		glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->nodesmain->mixnodescomp[i]->mixfbo);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mainprogram->nodesmain->mixnodescomp[i]->mixtex, 0);
	}
	
	mainprogram->shelves[0]->open(result + "_0.file");
	mainprogram->shelves[1]->open(result + "_1.file");
	mainprogram->prevmodus = true;
	mainmix->open_mix(result + "_2.file");
	Layer *bulay = mainmix->currlay;
	if (exists(result + "_3.file")) {
		mainprogram->prevmodus = false;
		mainmix->open_mix(result + "_3.file");
	}
	//open_genmidis(remove_extension(path) + ".midi");
	
	while (getline(rfile, istring)) {
		if (istring == "ENDOFFILE") break;
		if (istring == "PREVMODUS") {
			getline(rfile, istring);
			mainprogram->prevmodus = std::stoi(istring);
			if (mainprogram->prevmodus) mainmix->currlay = bulay;
		}
		if (istring == "TOSCREENMIDI0") {
			getline(rfile, istring);
			mainprogram->toscreen->midi[0] = std::stoi(istring);
		}
		if (istring == "TOSCREENMIDI1") {
			getline(rfile, istring);
			mainprogram->toscreen->midi[1] = std::stoi(istring);
		}
		if (istring == "TOSCREENMIDIPORT") {
			getline(rfile, istring);
			mainprogram->toscreen->midiport = istring;
		}
		if (istring == "BACKTOPREMIDI0") {
			getline(rfile, istring);
			mainprogram->backtopre->midi[0] = std::stoi(istring);
		}
		if (istring == "BACKTOPREMIDI1") {
			getline(rfile, istring);
			mainprogram->backtopre->midi[1] = std::stoi(istring);
		}
		if (istring == "BACKTOPREMIDIPORT") {
			getline(rfile, istring);
			mainprogram->backtopre->midiport = istring;
		}
		if (istring == "MODUSBUTMIDI0") {
			getline(rfile, istring);
			mainprogram->modusbut->midi[0] = std::stoi(istring);
		}
		if (istring == "MODUSBUTMIDI1") {
			getline(rfile, istring);
			mainprogram->modusbut->midi[1] = std::stoi(istring);
		}
		if (istring == "MODUSBUTMIDIPORT") {
			getline(rfile, istring);
			mainprogram->modusbut->midiport = istring;
		}
	}
	rfile.close();
}

void Mixer::open_mix(const std::string &path) {
	std::string result = deconcat_files(path);
	bool concat = (result != "");
	std::ifstream rfile;
	if (concat) rfile.open(result);
	else rfile.open(path);
	std::string istring;
	
	this->new_file(2, 1);
	for (int i = 0; i < loopstation->elems.size(); i++) {
		loopstation->elems[i]->erase_elem();
	}
	loopstation->parmap.clear();
	loopstation->elemmap.clear();
	loopstation->readelems.clear();
	loopstation->readelemnrs.clear();
	
	int clpos, cldeck;
	Layer *lay;
	while (getline(rfile, istring)) {
		if (istring == "CURRLAY") {
			getline(rfile, istring);
			clpos = std::stoi(istring);
		}
		if (istring == "CURRDECK") {
			getline(rfile, istring);
			cldeck = std::stoi(istring);
		}
		if (istring == "CROSSFADE") {
			getline(rfile, istring); 
			mainmix->crossfade->value = std::stof(istring);
			if (mainprogram->prevmodus) {
				GLfloat cf = glGetUniformLocation(mainprogram->ShaderProgram, "cf");
				glUniform1f(cf, mainmix->crossfade->value);
			}
		}
		if (istring == "CROSSFADEEVENT") {
			Param *par = mainmix->crossfade;
			getline(rfile, istring);
			if (istring == "EVENTELEM") {
				mainmix->event_read(rfile, par, lay);
			}
		}
		if (istring == "CROSSFADECOMP") {
			getline(rfile, istring); 
			mainmix->crossfadecomp->value = std::stof(istring);
			if (!mainprogram->prevmodus) {
				GLfloat cf = glGetUniformLocation(mainprogram->ShaderProgram, "cf");
				glUniform1f(cf, mainmix->crossfadecomp->value);
			}
		}
		if (istring == "CROSSFADECOMPEVENT") {
			Param *par = mainmix->crossfadecomp;
			getline(rfile, istring);
			if (istring == "EVENTELEM") {
				mainmix->event_read(rfile, par, lay);
			}
		}
		if (istring == "DECKSPEEDA") {
			getline(rfile, istring);
			mainprogram->deckspeed[0]->value = std::stof(istring);
		}
		if (istring == "DECKSPEEDAMIDI0") {
			getline(rfile, istring);
			mainprogram->deckspeed[0]->midi[0] = std::stoi(istring);
		}
		if (istring == "DECKSPEEDAMIDI1") {
			getline(rfile, istring);
			mainprogram->deckspeed[0]->midi[1] = std::stoi(istring);
		}
		if (istring == "DECKSPEEDAMIDIPORT") {
			getline(rfile, istring);
			mainprogram->deckspeed[0]->midiport = istring;
		}
		if (istring == "DECKSPEEDB") {
			getline(rfile, istring);
			mainprogram->deckspeed[1]->value = std::stof(istring);
		}
		if (istring == "DECKSPEEDBMIDI0") {
			getline(rfile, istring);
			mainprogram->deckspeed[1]->midi[0] = std::stoi(istring);
		}
		if (istring == "DECKSPEEDBMIDI1") {
			getline(rfile, istring);
			mainprogram->deckspeed[1]->midi[1] = std::stoi(istring);
		}
		if (istring == "DECKSPEEDBMIDIPORT") {
			getline(rfile, istring);
			mainprogram->deckspeed[1]->midiport = istring;
		}
		if (istring == "WIPE") {
			getline(rfile, istring); 
			mainmix->wipe[0] = std::stoi(istring);
		}
		if (istring == "WIPEDIR") {
			getline(rfile, istring); 
			mainmix->wipedir[0] = std::stoi(istring);
		}
		if (istring == "WIPECOMP") {
			getline(rfile, istring); 
			mainmix->wipe[1] = std::stoi(istring);
		}
		if (istring == "WIPEDIRCOMP") {
			getline(rfile, istring); 
			mainmix->wipedir[1] = std::stoi(istring);
		}
		int deck;
		if (istring == "LAYERSA") {
			std::vector<Layer*> &layersA = choose_layers(0);
			int jpegcount = mainmix->read_layers(rfile, result, layersA, 0, 2, concat, 1, 1, 1);
			mainprogram->filecount = jpegcount;
			std::vector<Layer*> &layersB = choose_layers(1);
			mainmix->read_layers(rfile, result, layersB, 1, 2, concat, 1, 1, 1);
			mainprogram->filecount = 0;
		}
		std::vector<int> map;
		for (int m = 0; m < 2; m++) {
			std::vector<Layer*> &layers = choose_layers(m);
			for (int i = 0; i < layers.size(); i++) {
				if (layers[i]->clonesetnr != -1) {
					int pos = std::find(map.begin(), map.end(), layers[i]->clonesetnr) - map.begin();
					if (pos == map.size()) {
						std::unordered_set<Layer*> *uset = new std::unordered_set<Layer*>;
						mainmix->clonesets.push_back(uset);
						map.push_back(layers[i]->clonesetnr);
						layers[i]->clonesetnr = mainmix->clonesets.size() - 1;
					}
					else layers[i]->clonesetnr = pos;
					mainmix->clonesets[layers[i]->clonesetnr]->emplace(layers[i]);
				}
			}
		}
	}
	
	std::vector<Layer*> &lvec = choose_layers(cldeck);
	for (int i = 0; i < lvec.size(); i++) {
		if (lvec[i]->pos == clpos) {
			mainmix->currlay = lvec[i];
			break;
		}
	}
	
	rfile.close();
}

void Mixer::open_deck(const std::string &path, bool alive) {
	std::string result = deconcat_files(path);
	bool concat = (result != "");
	std::ifstream rfile;
	if (concat) rfile.open(result);
	else rfile.open(path);
	std::string istring;
	
	this->new_file(mainmix->mousedeck, alive);
	
	for (int i = 0; i < loopstation->elems.size(); i++) {
		bool alsootherdeck = false;
		std::unordered_set<Layer*>::iterator it;
		for (it = loopstation->elems[i]->layers.begin(); it != loopstation->elems[i]->layers.end(); it++) {
			Layer *lay = *it;
			if (lay->deck == !mainmix->mousedeck) {
				alsootherdeck = true;
				break;
			}
		}
		if (alsootherdeck) {
			std::unordered_set<Param*>::iterator it;
			for (it = loopstation->elems[i]->params.begin(); it != loopstation->elems[i]->params.end(); it++) {
				loopstation->elemmap.erase(*it);
				loopstation->parmap.erase(*it);
			}
			loopstation->elems[i]->erase_elem();
		}
	}
	loopstation->readelems.clear();
	loopstation->readelemnrs.clear();
	std::vector<Layer*> &layers = choose_layers(mainmix->mousedeck);
	mainmix->read_layers(rfile, result, layers, mainmix->mousedeck, 1, 1, concat, 1, 1);
	std::vector<int> map;
	for (int i = 0; i < mainmix->clonesets.size(); i++) {
		map.push_back(i);
	}
	for (int i = 0; i < layers.size(); i++) {
		if (layers[i]->clonesetnr != -1) {
			int pos = std::find(map.begin(), map.end(), layers[i]->clonesetnr) - map.begin();
			if (pos == map.size()) {
				std::unordered_set<Layer*> *uset = new std::unordered_set<Layer*>;
				mainmix->clonesets.push_back(uset);
				map.push_back(layers[i]->clonesetnr);
				layers[i]->clonesetnr = mainmix->clonesets.size() - 1;
			}
			else layers[i]->clonesetnr = pos;
			mainmix->clonesets[layers[i]->clonesetnr]->emplace(layers[i]);
		}
	}
	rfile.close();
}

int Mixer::read_layers(std::istream &rfile, const std::string &result, std::vector<Layer*> &layers, bool deck, int type, bool doclips, bool concat, bool load, bool loadevents) {
	Layer *lay = nullptr;
	std::string istring;
	int jpegcount = 0;
	if (mainprogram->filecount) jpegcount = mainprogram->filecount;
	int lw = 0;
	int lh = 0;
	while (getline(rfile, istring)) {
		if (istring == "LAYERSB" or istring == "ENDOFCLIPLAYER" or istring == "ENDOFFILE") {
			return jpegcount;	
		}
		if (istring == "DECKSPEED") {
			getline(rfile, istring);
			mainprogram->deckspeed[deck]->value = std::stof(istring);
		}
		if (istring == "DECKSPEEDMIDI0") {
			getline(rfile, istring);
			mainprogram->deckspeed[deck]->midi[0] = std::stoi(istring);
		}
		if (istring == "DECKSPEEDMIDI1") {
			getline(rfile, istring);
			mainprogram->deckspeed[deck]->midi[1] = std::stoi(istring);
		}
		if (istring == "DECKSPEEDMIDIPORT") {
			getline(rfile, istring);
			mainprogram->deckspeed[deck]->midiport = istring;
		}
		if (istring == "POS") {
			getline(rfile, istring);
			int pos = std::stoi(istring);
			if (pos == 0 or type == 0) {
				lay = layers[0];
				if (type == 1) mainmix->currlay = lay;
			}
			else {
				lay = mainmix->add_layer(layers, pos);
			}
			lay->deck = deck;
		}
		if (istring == "TYPE") {
			getline(rfile, istring);
			lay->type = (ELEM_TYPE)std::stoi(istring);
		}
		if (istring == "FILENAME") {
			getline(rfile, istring);
			lay->filename = istring;
			if (load) {
				lay->timeinit = false;
				if (lay->filename != "") {
					if (lay->type == ELEM_LIVE) {
						avdevice_register_all();
						ptrdiff_t pos = std::find(mainprogram->busylist.begin(), mainprogram->busylist.end(), lay->filename) - mainprogram->busylist.begin();
						if (pos >= mainprogram->busylist.size()) {
							mainprogram->busylist.push_back(lay->filename);
							mainprogram->busylayers.push_back(lay);
							AVInputFormat *ifmt = av_find_input_format("dshow");
							thread_vidopen(lay, ifmt, false);
						}
						else {
							lay->liveinput = mainprogram->busylayers[pos];
							mainprogram->mimiclayers.push_back(lay);
						}
					}
					else if (lay->type == ELEM_FILE or lay->type == ELEM_LAYER) {
						open_video(lay->frame, lay, lay->filename, false);
					}
					else if (lay->type == ELEM_IMAGE) {
						lay->open_image(lay->filename);
					}
				}
				if (type > 0) lay->prevframe = -1;
			}
		}
		if (istring == "RELPATH") {
			getline(rfile, istring);
			if (load) {
				if (lay->filename == "") {
					lay->filename = boost::filesystem::absolute(istring).string();
					lay->timeinit = false;
					if (lay->type == ELEM_FILE or lay->type == ELEM_LAYER) {
						open_video(lay->frame, lay, lay->filename, false);
					}
					else if (lay->type == ELEM_IMAGE) {
						lay->open_image(lay->filename);
					}
				}
			}
		}
		if (istring == "WIDTH") {
			getline(rfile, istring); 
			lw = std::stoi(istring);
		}
		if (istring == "HEIGHT") {
			getline(rfile, istring); 
			lh = std::stoi(istring);
		}
		if (istring == "ASPECTRATIO") {
			getline(rfile, istring); 
			lay->aspectratio = (RATIO_TYPE)std::stoi(istring);
		}
		if (istring == "MUTE") {
			getline(rfile, istring); 
			lay->mutebut->value = std::stoi(istring);
		}
		if (istring == "SOLO") {
			getline(rfile, istring); 
			lay->solobut->value = std::stoi(istring);
		}
		if (istring == "CLONESETNR") {
			getline(rfile, istring); 
			lay->clonesetnr = std::stoi(istring);
		}
		if (istring == "SPEEDVAL") {
			getline(rfile, istring); 
			lay->speed->value = std::stof(istring);
		}
		if (istring == "PLAYBUTVAL") {
			getline(rfile, istring); 
			lay->playbut->value = std::stoi(istring);
		}
		if (istring == "REVBUTVAL") {
			getline(rfile, istring); 
			lay->revbut->value = std::stoi(istring);
		}
		if (istring == "BOUNCEBUTVAL") {
			getline(rfile, istring); 
			lay->bouncebut->value = std::stoi(istring);
		}
		if (istring == "PLAYKIND") {
			getline(rfile, istring); 
			lay->playkind = std::stoi(istring);
		}
		if (istring == "GENMIDIBUTVAL") {
			getline(rfile, istring); 
			lay->genmidibut->value = std::stoi(istring);
		}
		if (istring == "SHIFTX") {
			getline(rfile, istring); 
			lay->shiftx = std::stoi(istring);
		}
		if (istring == "SHIFTY") {
			getline(rfile, istring); 
			lay->shifty = std::stoi(istring);
		}
		if (istring == "SCALE") {
			getline(rfile, istring);
			lay->scale = std::stof(istring);
		}
		if (istring == "OPACITYVAL") {
			getline(rfile, istring); 
			lay->opacity->value = std::stof(istring);
		}
		if (istring == "VOLUMEVAL") {
			getline(rfile, istring); 
			lay->volume->value = std::stof(istring);
		}
		if (istring == "CHTOLVAL") {
			getline(rfile, istring); 
			lay->chtol->value = std::stof(istring);
		}
		if (istring == "CHDIRVAL") {
			getline(rfile, istring); 
			lay->chdir->value = std::stoi(istring);
		}
		if (istring == "CHINVVAL") {
			getline(rfile, istring); 
			lay->chinv->value = std::stoi(istring);
		}
		if (istring == "FRAME") {
			getline(rfile, istring); 
			lay->frame = std::stof(istring);
		}
		if (istring == "STARTFRAME") {
			getline(rfile, istring); 
			lay->startframe = std::stoi(istring);
		}
		if (istring == "ENDFRAME") {
			getline(rfile, istring); 
			lay->endframe = std::stoi(istring);
		}
		if (lay) {
			if (!lay->dummy) {
				if (istring == "MIXMODE") {
					getline(rfile, istring); 
					lay->blendnode->blendtype = (BLEND_TYPE)std::stoi(istring);
				}
				if (istring == "MIXFACVAL") {
					getline(rfile, istring); 
					lay->blendnode->mixfac->value = std::stof(istring);
				}
				if (istring == "CHRED") {
					getline(rfile, istring); 
					lay->blendnode->chred = std::stof(istring);
					lay->colorbox->acolor[0] = lay->blendnode->chred;
				}
				if (istring == "CHGREEN") {
					getline(rfile, istring); 
					lay->blendnode->chgreen = std::stof(istring);
					lay->colorbox->acolor[1] = lay->blendnode->chgreen;
				}
				if (istring == "CHBLUE") {
					getline(rfile, istring); 
					lay->blendnode->chblue = std::stof(istring);
					lay->colorbox->acolor[2] = lay->blendnode->chblue;
				}
				if (istring == "WIPETYPE") {
					getline(rfile, istring); 
					lay->blendnode->wipetype = std::stoi(istring);
				}
				if (istring == "WIPEDIR") {
					getline(rfile, istring); 
					lay->blendnode->wipedir = std::stoi(istring);
				}
				if (istring == "WIPEX") {
					getline(rfile, istring); 
					lay->blendnode->wipex = std::stof(istring);
				}
				if (istring == "WIPEY") {
					getline(rfile, istring); 
					lay->blendnode->wipey = std::stof(istring);
				}
			}
		}
		
		if (istring == "CLIPS") {
			Clip *clip = nullptr;
			while (getline(rfile, istring)) {
				if (istring == "ENDOFCLIPS") break;
				if (doclips) {
					if (istring == "TYPE") {
						clip = new Clip;
						getline(rfile, istring);
						clip->type = (ELEM_TYPE)std::stoi(istring);
						lay->clips.insert(lay->clips.begin() + lay->clips.size() - 1, clip);
					}
					if (istring == "FILENAME") {
						getline(rfile, istring);
						clip->path = istring;					
					}
					if (istring == "CLIPLAYER") {
						std::vector<Layer*> cliplayers;
						Layer *cliplay = mainmix->add_layer(cliplayers, 0);
						mainmix->read_layers(rfile, result, cliplayers, 0, 0, 0, 1, 0, 0);
						std::string name = remove_extension(remove_extension(basename(result)));
						int count = 0;
						while (1) {
							clip->path = mainprogram->temppath + "cliptemp_" + name + ".layer";
							if (!exists(clip->path)) {
								mainmix->save_layerfile(clip->path, cliplay, 0);
								break;
							}
							count++;
							name = remove_version(name) + "_" + std::to_string(count);
						}
						mainmix->delete_layer(cliplayers, cliplay, false);
					}
					if (istring == "JPEGPATH") {
						getline(rfile, istring);
						std::string jpegpath = istring;
						glGenTextures(1, &clip->tex);
						glBindTexture(GL_TEXTURE_2D, clip->tex);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
						glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (int)(glob->w * 0.3f), (int)(glob->h * 0.3f), 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
						if (concat) {
							boost::filesystem::rename(result + "_" + std::to_string(jpegcount) + ".file", result + "_" + std::to_string(jpegcount) + ".jpeg");
							open_thumb(result + "_" + std::to_string(jpegcount) + ".jpeg", clip->tex);
							jpegcount++;
						}
						else open_thumb(jpegpath, clip->tex);
					}
				}
			}
		}
		if (istring == "EFFECTS") {
			mainprogram->effcat[lay->deck]->value = 0;
			int type;
			Effect *eff = nullptr;
			getline(rfile, istring);
			while (istring != "ENDOFEFFECTS") {
				if (istring == "TYPE") {
					getline(rfile, istring);
					type = std::stoi(istring);
				}
				if (istring == "POS") {
					getline(rfile, istring);
					int pos = std::stoi(istring);
					eff = lay->add_effect((EFFECT_TYPE)type, pos);
				}
				if (istring == "ONOFFVAL") {
					getline(rfile, istring);
					eff->onoffbutton->value = std::stoi(istring);
				}
				if (istring == "ONOFFMIDI0") {
					getline(rfile, istring);
					eff->onoffbutton->midi[0] = std::stoi(istring);
				}
				if (istring == "ONOFFMIDI1") {
					getline(rfile, istring);
					eff->onoffbutton->midi[1] = std::stoi(istring);
				}
				if (istring == "ONOFFMIDIPORT") {
					getline(rfile, istring);
					eff->onoffbutton->midiport = istring;
				}
				if (istring == "DRYWETVAL") {
					getline(rfile, istring);
					eff->drywet->value = std::stof(istring);
				}
				if (istring == "DRYWETMIDI0") {
					getline(rfile, istring);
					eff->drywet->midi[0] = std::stoi(istring);
				}
				if (istring == "DRYWETMIDI1") {
					getline(rfile, istring);
					eff->drywet->midi[1] = std::stoi(istring);
				}
				if (istring == "DRYWETMIDIPORT") {
					getline(rfile, istring);
					eff->drywet->midiport = istring;
				}
				if (eff) {
					if (eff->type == RIPPLE) {
						if (istring == "RIPPLESPEED") {
							getline(rfile, istring);
							((RippleEffect*)eff)->speed = std::stof(istring);
						}
						if (istring == "RIPPLECOUNT") {
							getline(rfile, istring);
							((RippleEffect*)eff)->ripplecount = std::stof(istring);
						}
					}
				}
				
				if (istring == "PARAMS") {
					int pos = 0;
					Param *par;
					while (istring != "ENDOFPARAMS") {
						getline(rfile, istring);
						if (istring == "VAL") {
							par = eff->params[pos];
							pos++;
							getline(rfile, istring);
							par->value = std::stof(istring);
							par->effect = eff;
						}
						if (istring == "MIDI0") {
							getline(rfile, istring);
							par->midi[0] = std::stoi(istring);
						}
						if (istring == "MIDI1") {
							getline(rfile, istring);
							par->midi[1] = std::stoi(istring);
						}
						if (istring == "MIDIPORT") {
							getline(rfile, istring);
							par->midiport = istring;
						}
						if (istring == "EVENTELEM") {
							if (loadevents) {
								mainmix->event_read(rfile, par, lay);
							}
							else {
								while (getline(rfile, istring)) {
									if (istring == "ENDOFEVENT") break;
								}
							}
						}
					}
				}
				getline(rfile, istring);
			}
			if (lw) lay->set_aspectratio(lw, lh);
		}
		if (istring == "STREAMEFFECTS") {
			mainprogram->effcat[lay->deck]->value = 1;
			int type;
			Effect *eff = nullptr;
			getline(rfile, istring);
			while (istring != "ENDOFSTREAMEFFECTS") {
				if (istring == "TYPE") {
					getline(rfile, istring);
					type = std::stoi(istring);
				}
				if (istring == "POS") {
					getline(rfile, istring);
					int pos = std::stoi(istring);
					eff = lay->add_effect((EFFECT_TYPE)type, pos);
				}
				if (istring == "ONOFFVAL") {
					getline(rfile, istring);
					eff->onoffbutton->value = std::stoi(istring);
				}
				if (istring == "DRYWETVAL") {
					getline(rfile, istring);
					eff->drywet->value = std::stof(istring);
				}
				if (eff) {
					if (eff->type == RIPPLE) {
						if (istring == "RIPPLESPEED") {
							getline(rfile, istring);
							((RippleEffect*)eff)->speed = std::stof(istring);
						}
						if (istring == "RIPPLECOUNT") {
							getline(rfile, istring);
							((RippleEffect*)eff)->ripplecount = std::stof(istring);
						}
					}
				}
				
				if (istring == "PARAMS") {
					int pos = 0;
					Param *par;
					while (istring != "ENDOFPARAMS") {
						getline(rfile, istring);
						if (istring == "VAL") {
							par = eff->params[pos];
							pos++;
							getline(rfile, istring);
							par->value = std::stof(istring);
							par->effect = eff;
						}
						if (istring == "MIDI0") {
							getline(rfile, istring);
							par->midi[0] = std::stoi(istring);
						}
						if (istring == "MIDI1") {
							getline(rfile, istring);
							par->midi[1] = std::stoi(istring);
						}
						if (istring == "MIDIPORT") {
							getline(rfile, istring);
							par->midiport = istring;
						}
						if (istring == "EVENTELEM") {
							if (loadevents) {
								mainmix->event_read(rfile, par, lay);
							}
							else {
								while (getline(rfile, istring)) {
									if (istring == "ENDOFEVENT") break;
								}
							}
						}
					}
				}
				getline(rfile, istring);
			}
		}
		if (istring == "EFFCAT") {
			getline(rfile, istring);
			mainprogram->effcat[lay->deck]->value = std::stoi(istring);
		}
	}
	
	std::vector<Layer*> &lvec = choose_layers(deck);
	for (int i = 0; i < lvec.size(); i++) {
		if (lvec[i]->mutebut->value) lvec[i]->mute_handle();
	}
	
	return jpegcount;
}
	
void Mixer::new_file(int decks, bool alive) {
	// kill mixnodes
	//mainprogram->nodesmain->mixnodes.clear();
	//mainprogram->nodesmain->mixnodescomp.clear();
	
	// reset layers
	bool currdeck = 0;
	if (mainmix->currlay) currdeck = mainmix->currlay->deck;
	if (decks == 0 or decks == 2) {
		std::vector<Layer*> &lvec = choose_layers(0);
		this->delete_layers(lvec, alive);
		Layer *lay = mainmix->add_layer(lvec, 0);
		if (mainprogram->prevmodus) {
			mainprogram->nodesmain->currpage->connect_nodes(lvec[lvec.size() - 1]->lasteffnode[0], mainprogram->nodesmain->mixnodes[0]);
		}
		else {
			mainprogram->nodesmain->currpage->connect_nodes(lvec[lvec.size() - 1]->lasteffnode[0], mainprogram->nodesmain->mixnodescomp[0]);
		}
		if (currdeck == 0) mainmix->currlay = lay;
	}
	if (decks == 1 or decks == 2) {
		std::vector<Layer*> &lvec = choose_layers(1);
		this->delete_layers(lvec, alive);
		Layer *lay = mainmix->add_layer(lvec, 0);
		if (mainprogram->prevmodus) {
			mainprogram->nodesmain->currpage->connect_nodes(lvec[lvec.size() - 1]->lasteffnode[0], mainprogram->nodesmain->mixnodes[1]);
		}
		else {
			mainprogram->nodesmain->currpage->connect_nodes(lvec[lvec.size() - 1]->lasteffnode[0], mainprogram->nodesmain->mixnodescomp[1]);
		}
		if (currdeck == 1) mainmix->currlay = lay;
	}
		
	// set comp layers
	//mainmix->copy_to_comp(mainmix->layersA, mainmix->layersAcomp, mainmix->layersB, mainmix->layersBcomp, mainprogram->nodesmain->currpage->nodes, mainprogram->nodesmain->currpage->nodescomp, mainprogram->nodesmain->mixnodescomp, true);
}

void Mixer::delete_layers(std::vector<Layer*> &layers, bool alive) {
	while (!layers.empty()) {
		//layers.back()->node = nullptr;
		//layers.back()->blendnode = nullptr;
		Layer *lay = layers.back();
		if (alive and std::find(mainprogram->busylayers.begin(), mainprogram->busylayers.end(), lay) == mainprogram->busylayers.end()) {
			mainmix->delete_layer(layers, lay, false);
		}
		else {
			int size = layers.size();
			for (int i = 0; i < size; i++) {
				if (layers[i] == lay) {
					layers.erase(layers.begin() + i);
				}
			}
			while (!lay->effects[0].empty()) {
				mainprogram->nodesmain->currpage->delete_node(lay->effects[0].back()->node);
				for (int j = 0; j < lay->effects[0].back()->params.size(); j++) {
					delete lay->effects[0].back()->params[j];
				}
				delete lay->effects[0].back();
				lay->effects[0].pop_back();
			}
			mainprogram->nodesmain->currpage->delete_node(lay->node);
		}
	}
}


void Mixer::record_video() {
    const AVCodec *codec;
    AVCodecContext *c = nullptr;
    int i, ret, x, y;
    FILE *f;
    AVFrame *frame;
    AVPacket *pkt;
    uint8_t endcode[] = { 0, 0, 1, 0xb7 };
    codec = avcodec_find_encoder_by_name("mjpeg");
    c = avcodec_alloc_context3(codec);
    pkt = av_packet_alloc();
    c->global_quality = 0;
    c->width = mainprogram->ow;
    c->height = (int)mainprogram->oh;
    /* frames per second */
    c->time_base = (AVRational){1, 25};
    c->framerate = (AVRational){25, 1};
    c->pix_fmt = AV_PIX_FMT_YUVJ420P;
    /* open it */
    ret = avcodec_open2(c, codec, nullptr);
    
	std::string name = "recording_0";
	int count = 0;
	while (1) {
		std::string path = mainprogram->recdir + name + ".mov";
		if (!exists(path)) {
			f = fopen(path.c_str(), "wb");
			if (!f) {
				fprintf(stderr, "Could not open %s\n", path);
				exit(1);
			}
			break;
		}
		count++;
		name = remove_version(name) + "_" + std::to_string(count);
	}
    
	this->yuvframe = av_frame_alloc();
    this->yuvframe->format = c->pix_fmt;
    this->yuvframe->width  = c->width;
    this->yuvframe->height = c->height;
    //ret = av_frame_get_buffer(frame, 32);
    //if (ret < 0) {
    //    fprintf(stderr, "Could not allocate the video frame data\n");
    //    exit(1);
    //}

	// Determine required buffer size and allocate buffer
	this->yuvframe = av_frame_alloc();
    this->yuvframe->format = c->pix_fmt;
    this->yuvframe->width  = c->width;
    this->yuvframe->height = c->height;
 
  	this->sws_ctx = sws_getContext
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
    while (mainmix->recording) {
		std::unique_lock<std::mutex> lock(this->recordlock);
		this->startrecord.wait(lock, [&]{return this->recordnow;});
		lock.unlock();
		this->recordnow = false;
		
		AVFrame *rgbaframe;
		rgbaframe = av_frame_alloc();
		rgbaframe->data[0] = (uint8_t *)mainmix->rgbdata;
		rgbaframe->format = c->pix_fmt;
		rgbaframe->width  = c->width;
		rgbaframe->height = c->height;
	 
		//av_image_fill_arrays(picture->data, picture->linesize,
                               // ptr, pix_fmt, width, height, 1);		avpicture_fill(&rgbaframe, (uint8_t *)mainmix->rgbdata, AV_PIX_FMT_BGRA, c->width, c->height);
		rgbaframe->linesize[0] = mainprogram->ow * 4;
		int storage = av_image_alloc(this->yuvframe->data, this->yuvframe->linesize, this->yuvframe->width, this->yuvframe->height, c->pix_fmt, 32);
		sws_scale
		(
			this->sws_ctx,
			rgbaframe->data,
			rgbaframe->linesize,
			0,
			c->height,
			this->yuvframe->data,
			this->yuvframe->linesize
		);
		
 		glUnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB);
 		glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0);
        /* make sure the frame data is writable */
        ret = av_frame_make_writable(this->yuvframe);
        /* prepare a dummy image */
        /* Y */
        //for (y = 0; y < c->height; y++) {
        //    for (x = 0; x < c->width; x++) {
        //        this->yuvframe->data[0][y * this->yuvframe->linesize[0] + x] = x + y + i * 3;
        //    }
        //}
        /* Cb and Cr */
        //for (y = 0; y < c->height/2; y++) {
        //    for (x = 0; x < c->width/2; x++) {
        //        this->yuvframe->data[1][y * this->yuvframe->linesize[1] + x] = 128 + y + i * 2;
        //        this->yuvframe->data[2][y * this->yuvframe->linesize[2] + x] = 64 + x + i * 5;
        //    }
        //}
        //frame->pts = i;
        /* encode the image */
        encode_frame(nullptr, nullptr, c, this->yuvframe, pkt, f, 0);
    }
    /* flush the encoder */
    encode_frame(nullptr, nullptr, c, nullptr, pkt, f, 0);
    /* add sequence end code to have a real MPEG file */
    fwrite(endcode, 1, sizeof(endcode), f);
    fclose(f);
    avcodec_free_context(&c);
    av_frame_free(&this->yuvframe);
    av_packet_free(&pkt);
    mainmix->donerec = true;
}

void Mixer::start_recording() {
	// start recording main output
	this->donerec = false;
	this->recording = true;
	// recording is done in separate low priority thread
	this->recording_video = std::thread{&Mixer::record_video, this};
	#ifdef _WIN64
	SetThreadPriority((void*)this->recording_video.native_handle(), THREAD_PRIORITY_LOWEST);
	#else
	struct sched_param params;
	params.sched_priority = sched_get_priority_min(SCHED_FIFO);		pthread_setschedparam(this->recording_video.native_handle(), SCHED_FIFO, &params);
	#endif
	this->recording_video.detach();
	#define BUFFER_OFFSET(i) ((char *)nullptr + (i))
	glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, this->ioBuf);
	glBufferData(GL_PIXEL_PACK_BUFFER_ARB, (int)(mainprogram->ow * mainprogram->oh) * 4, nullptr, GL_DYNAMIC_READ);
	glBindFramebuffer(GL_FRAMEBUFFER, ((MixNode*)mainprogram->nodesmain->mixnodescomp[2])->mixfbo);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glReadPixels(0, 0, mainprogram->ow, (int)mainprogram->oh, GL_BGRA, GL_UNSIGNED_BYTE, BUFFER_OFFSET(0));
	this->rgbdata = glMapBuffer(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY);
	//assert(this->rgbdata);
	this->recordnow = true;
	this->startrecord.notify_one();
	glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
}
