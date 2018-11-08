#include <boost/filesystem.hpp>

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
	this->numh = this->numh * glob->w / glob->h;
	for (int j = 0; j < 2; j++) {
		for (int i = 0; i < 5; i++) {
			Box *box = new Box;
			if (j == 0) this->numboxesA.push_back(box);
			else this->numboxesB.push_back(box);
			if (i == 0) {
				box->lcolor[0] = 1.0;
				box->lcolor[1] = 0.5;
				box->lcolor[2] = 0.5;
				box->lcolor[3] = 1.0;
			}
			else {
				box->lcolor[0] = 1.0;
				box->lcolor[1] = 1.0;
				box->lcolor[2] = 1.0;
				box->lcolor[3] = 1.0;
			}
			box->vtxcoords->x1 = -1 + j;
			box->vtxcoords->y1 = 1 - (i + 1) * this->numh;
			box->vtxcoords->w = this->numw;
			box->vtxcoords->h = this->numh;
			box->upvtxtoscr();
		}
	}

	this->modebox = new Box;
	this->modebox->vtxcoords->x1 = 0.85f;
	this->modebox->vtxcoords->y1 = -1.0f;
	this->modebox->vtxcoords->w = 0.15f;
	this->modebox->vtxcoords->h = 0.1f;
	this->modebox->upvtxtoscr();
	
	this->genmidi[0] = new Button(false);
	this->genmidi[0]->value = 1;
	this->genmidi[1] = new Button(false);
	this->genmidi[1]->value = 2;
	this->genmidi[0]->box->acolor[0] = 0.5;
	this->genmidi[0]->box->acolor[1] = 0.2;
	this->genmidi[0]->box->acolor[2] = 0.5;
	this->genmidi[0]->box->acolor[3] = 1.0;
	this->genmidi[0]->box->vtxcoords->x1 = -1.0f;
	this->genmidi[0]->box->vtxcoords->y1 =  1 - 6 * this->numh;
	this->genmidi[0]->box->vtxcoords->w = this->numw;
	this->genmidi[0]->box->vtxcoords->h = this->numh;
	this->genmidi[0]->box->upvtxtoscr();
	this->genmidi[1]->box->acolor[0] = 0.5;
	this->genmidi[1]->box->acolor[1] = 0.2;
	this->genmidi[1]->box->acolor[2] = 0.5;
	this->genmidi[1]->box->acolor[3] = 1.0;
	this->genmidi[1]->box->vtxcoords->x1 = 0.0f;
	this->genmidi[1]->box->vtxcoords->y1 =  1 - 6 * this->numh;
	this->genmidi[1]->box->vtxcoords->w = this->numw;
	this->genmidi[1]->box->vtxcoords->h = this->numh;
	this->genmidi[1]->box->upvtxtoscr();
	
	this->crossfade = new Param;
	this->crossfade->name = "Crossfade"; 
	this->crossfade->value = 0.5f;
	this->crossfade->range[0] = 0.0f;
	this->crossfade->range[1] = 1.0f;
	this->crossfade->sliding = true;
	this->crossfade->shadervar = "cf";
	this->crossfade->box->vtxcoords->x1 = -0.15f;
	this->crossfade->box->vtxcoords->y1 = -0.4f;
	this->crossfade->box->vtxcoords->w = 0.3f;
	this->crossfade->box->vtxcoords->h = tf(0.05f);
	this->crossfade->box->upvtxtoscr();
	this->crossfade->box->acolor[3] = 1.0f;
	this->crossfadecomp = new Param;
	this->crossfadecomp->name = "Crossfadecomp"; 
	this->crossfadecomp->value = 0.5f;
	this->crossfadecomp->range[0] = 0.0f;
	this->crossfadecomp->range[1] = 1.0f;
	this->crossfadecomp->sliding = true;
	this->crossfadecomp->shadervar = "cf";
	this->crossfadecomp->box->vtxcoords->x1 = -0.15f;
	this->crossfadecomp->box->vtxcoords->y1 = -0.4f;
	this->crossfadecomp->box->vtxcoords->w = 0.3f;
	this->crossfadecomp->box->vtxcoords->h = tf(0.05f);
	this->crossfadecomp->box->upvtxtoscr();
	this->crossfadecomp->box->acolor[3] = 1.0f;
	lpc->allparams.push_back(this->crossfadecomp);
	
	this->recbut = new Button(false);
	this->recbut->box->vtxcoords->x1 = 0.15f;
	this->recbut->box->vtxcoords->y1 = -0.4f;
	this->recbut->box->vtxcoords->w = tf(0.031f);
	this->recbut->box->vtxcoords->h = tf(0.05f);
	this->recbut->box->upvtxtoscr();
}


Param::Param() {
	this->box = new Box;
	this->box->acolor[0] = 0.2;
	this->box->acolor[1] = 0.2;
	this->box->acolor[2] = 0.2;
	this->box->acolor[3] = 1.0;
	if (loopstation) loopstation->allparams.push_back(this);
}

Param::~Param() {
	delete this->box;
	loopstation->allparams.erase(std::find(loopstation->allparams.begin(), loopstation->allparams.end(), this));
}

void Param::handle() {
	float green[4] = {0.0, 1.0, 0.2, 1.0};
	float white[4] = {1.0, 1.0, 1.0, 1.0};
	std::string parstr;
	draw_box(this->box, -1);
	int val;
	if (!this->powertwo) val = round(this->value * 1000.0f);
	else val = round(this->value * this->value * 1000.0f);
	if (mainmix->learnparam == this and mainmix->learn) {
		parstr = "MIDI";
	}
	else if (this != mainmix->adaptparam) parstr = this->name;
	else if (this->sliding) parstr = std::to_string(val).substr(0, std::to_string(val).length() - 3) + "." + std::to_string(val).substr(std::to_string(val).length() - 3, std::string::npos); 
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
			if (mainprogram->menuactivation) {
				if (loopstation->elemmap.find(this) != loopstation->elemmap.end()) mainprogram->parammenu2->state = 2;
				else mainprogram->parammenu1->state = 2;
				mainmix->learnparam = this;
				mainprogram->menuactivation = false;
			}
		}
		if (this->sliding) {
			draw_box(green, green, this->box->vtxcoords->x1 + this->box->vtxcoords->w * ((this->value - this->range[0]) / (this->range[1] - this->range[0])) - tf(0.00078f), this->box->vtxcoords->y1, 2.0f / glob->w, this->box->vtxcoords->h, -1);
		}
		else {
			draw_box(green, green, this->box->vtxcoords->x1 + this->box->vtxcoords->w * (((int)(this->value + 0.5f) - this->range[0]) / (this->range[1] - this->range[0])) - tf(0.00078f), this->box->vtxcoords->y1, 2.0f / glob->w, this->box->vtxcoords->h, -1);
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
	box->vtxcoords->x1 = -1.0f + mainmix->numw;
	box->vtxcoords->w = tf(0.025f);
	box->vtxcoords->h = tf(0.05f);
	box->upvtxtoscr();
	box->acolor[3] = 1.0f;
	
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
	param->shadervar = "chromarot";
	param->effect = this;
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
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "Factor"; 
	param->value = 1.2f;
	param->range[0] = 1.0f;
	param->range[1] = 1.5f;
	param->sliding = true;
	param->shadervar = "glowfac";
	param->effect = this;
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
	this->params.push_back(param);
	param = new Param;
	param->name = "Y"; 
	param->value = 0.5f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "radialY";
	param->effect = this;
	this->params.push_back(param);
}

SaturationEffect::SaturationEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Amount"; 
	param->value = 4;
	param->range[0] = 0;
	param->range[1] = 8;
	param->sliding = true;
	param->shadervar = "satamount";
	param->effect = this;
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
	this->params.push_back(param);
}

SwirlEffect::SwirlEffect() {
	this->numrows = 2;
	Param *param = new Param;
	param->name = "Angle"; 
	param->value = 0.8;
	param->range[0] = 0;
	param->range[1] = 2;
	param->sliding = true;
	param->shadervar = "swirlangle";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "Radius"; 
	param->value = 0.5;
	param->range[0] = 0;
	param->range[1] = 1;
	param->sliding = true;
	param->shadervar = "swradius";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->nextrow = true;
	param->name = "X"; 
	param->value = 0.5;
	param->range[0] = 0;
	param->range[1] = 1;
	param->sliding = true;
	param->shadervar = "swirlx";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "Y"; 
	param->value = 0.5;
	param->range[0] = 0;
	param->range[1] = 1;
	param->sliding = true;
	param->shadervar = "swirly";
	param->effect = this;
	this->params.push_back(param);
}

OldFilmEffect::OldFilmEffect() {
	this->numrows = 2;
	Param *param = new Param;
	param->name = "Noise"; 
	param->value = 2.0;
	param->range[0] = 0;
	param->range[1] = 4;
	param->sliding = true;
	param->shadervar = "filmnoise";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "Sepia"; 
	param->value = 2.0;
	param->range[0] = 0;
	param->range[1] = 4;
	param->sliding = true;
	param->shadervar = "SepiaValue";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "Scratch"; 
	param->value = 2.0;
	param->range[0] = 0;
	param->range[1] = 4;
	param->sliding = true;
	param->shadervar = "ScratchValue";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->nextrow = true;
	param->name = "InVign"; 
	param->range[0] = 0;
	param->range[1] = 1;
	param->sliding = true;
	param->shadervar = "InnerVignetting";
	param->value = 0.7;
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "OutVign"; 
	param->range[0] = 0;
	param->range[1] = 1;
	param->sliding = true;
	param->shadervar = "OuterVignetting";
	param->value = 1.0;
	param->effect = this;
	this->params.push_back(param);
}

RippleEffect::RippleEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Speed"; 
	param->value = 0.1;
	param->range[0] = 0;
	param->range[1] = 0.5;
	param->sliding = true;
	param->shadervar = "ripple";
	param->effect = this;
	this->params.push_back(param);
}

FishEyeEffect::FishEyeEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Amount"; 
	param->value = 0.7;
	param->range[0] = 0.1f;
	param->range[1] = 2;
	param->sliding = true;
	param->shadervar = "fishamount";
	param->effect = this;
	this->params.push_back(param);
}

TresholdEffect::TresholdEffect() {
	this->numrows = 3;
	Param *param = new Param;
	param->name = "Level"; 
	param->value = 0.5;
	param->range[0] = 0;
	param->range[1] = 2;
	param->sliding = true;
	param->shadervar = "treshheight";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->nextrow = true;
	param->name = "R1"; 
	param->value = 1.0;
	param->range[0] = 0;
	param->range[1] = 1;
	param->sliding = true;
	param->shadervar = "treshred1";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "G1"; 
	param->value = 1.0;
	param->range[0] = 0;
	param->range[1] = 1;
	param->sliding = true;
	param->shadervar = "treshgreen1";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "B1"; 
	param->value = 1.0;
	param->range[0] = 0;
	param->range[1] = 1;
	param->sliding = true;
	param->shadervar = "treshblue1";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->nextrow = true;
	param->name = "R2"; 
	param->value = 0.0;
	param->range[0] = 0;
	param->range[1] = 1;
	param->sliding = true;
	param->shadervar = "treshred2";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "G2"; 
	param->value = 0.0;
	param->range[0] = 0;
	param->range[1] = 1;
	param->sliding = true;
	param->shadervar = "treshgreen2";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "B2"; 
	param->value = 0.0;
	param->range[0] = 0;
	param->range[1] = 1;
	param->sliding = true;
	param->shadervar = "treshblue2";
	param->effect = this;
	this->params.push_back(param);
}

StrobeEffect::StrobeEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "R"; 
	param->value = 1.0;
	param->range[0] = 0;
	param->range[1] = 1;
	param->sliding = true;
	param->shadervar = "strobered";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "G"; 
	param->value = 1.0;
	param->range[0] = 0;
	param->range[1] = 1;
	param->sliding = true;
	param->shadervar = "strobegreen";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "B"; 
	param->value = 1.0;
	param->range[0] = 0;
	param->range[1] = 1;
	param->sliding = true;
	param->shadervar = "strobeblue";
	param->effect = this;
	this->params.push_back(param);
}

PosterizeEffect::PosterizeEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Number"; 
	param->value = 8.0;
	param->range[0] = 0;
	param->range[1] = 16;
	param->sliding = false;
	param->shadervar = "numColors";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "Gamma"; 
	param->value = 0.6;
	param->range[0] = 0;
	param->range[1] = 1;
	param->sliding = true;
	param->shadervar = "gamma";
	param->effect = this;
	this->params.push_back(param);
}

PixelateEffect::PixelateEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Width"; 
	param->value = 64.0;
	param->range[0] = 1;
	param->range[1] = 512;
	param->sliding = true;
	param->shadervar = "pixel_w";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "Height"; 
	param->value = 128.0;
	param->range[0] = 1;
	param->range[1] = 1024;
	param->sliding = true;
	param->shadervar = "pixel_h";
	param->effect = this;
	this->params.push_back(param);
}

CrosshatchEffect::CrosshatchEffect() {
	this->numrows = 3;
	Param *param = new Param;
	param->name = "Size"; 
	param->value = 20.0;
	param->range[0] = 0;
	param->range[1] = 100;
	param->sliding = true;
	param->shadervar = "hatchsize";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "Y Offset"; 
	param->value = 5.0;
	param->range[0] = 0;
	param->range[1] = 40;
	param->sliding = true;
	param->shadervar = "hatch_y_offset";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->nextrow = true;
	param->name = "Lum Tresh 1"; 
	param->value = 1.0;
	param->range[0] = 0;
	param->range[1] = 1.0;
	param->sliding = true;
	param->shadervar = "lum_threshold_1";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "Lum Tresh 2"; 
	param->value = 0.7;
	param->range[0] = 0;
	param->range[1] = 1.0;
	param->sliding = true;
	param->shadervar = "lum_threshold_2";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->nextrow = true;
	param->name = "Lum Tresh 3"; 
	param->value = 0.5;
	param->range[0] = 0;
	param->range[1] = 1.0;
	param->sliding = true;
	param->shadervar = "lum_threshold_3";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "Lum Tresh 4"; 
	param->value = 0.3;
	param->range[0] = 0;
	param->range[1] = 1.0;
	param->sliding = true;
	param->shadervar = "lum_threshold_4";
	param->effect = this;
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
	param->range[0] = 0;
	param->range[1] = 360;
	param->sliding = true;
	param->shadervar = "rotamount";
	param->effect = this;
	this->params.push_back(param);
}

EmbossEffect::EmbossEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Strength"; 
	param->value = 5.0f;
	param->range[0] = 0;
	param->range[1] = 20.0f;
	param->sliding = true;
	param->shadervar = "embstrength";
	param->effect = this;
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
	param->range[0] = 20;
	param->range[1] = 200;
	param->sliding = true;
	param->shadervar = "asciisize";
	param->effect = this;
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
	this->params.push_back(param);
	param = new Param;
	param->name = "Angle"; 
	param->value = 0.0f;
	param->range[0] = 0.0f;
	param->range[1] = 4.0f;
	param->sliding = true;
	param->shadervar = "vardotangle";
	param->effect = this;
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
	this->params.push_back(param);
	param = new Param;
	param->name = "LineWidth";
	param->value = 1.0f;
	param->range[0] = 0.0f;
	param->range[1] = 50.0f;
	param->sliding = true;
	param->shadervar = "crtlineWidth";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "LineContrast";
	param->value = 0.25f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "crtlineContrast";
	param->effect = this;
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
	this->params.push_back(param);
	param = new Param;
	param->name = "NoiseSize";
	param->value = 1.0f;
	param->range[0] = 0.0f;
	param->range[1] = 20.0f;
	param->sliding = true;
	param->shadervar = "crtnoiseSize";
	param->effect = this;
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
	this->params.push_back(param);
	param = new Param;
	param->name = "VignetAlpha";
	param->value = 1.0f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "crtvignettingAlpha";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "VignetBlur";
	param->value = 0.3f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "crtvignettingBlur";
	param->effect = this;
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
	this->params.push_back(param);
	param = new Param;
	param->name = "Threshold2";
	param->value = 5.0f;
	param->range[0] = 0.0f;
	param->range[1] = 20.0f;
	param->sliding = true;
	param->shadervar = "edge_thres2";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "Thickness";
	param->value = 0.0f;
	param->range[0] = 0.0f;
	param->range[1] = 40.0f;
	param->sliding = true;
	param->shadervar = "edthickness";  //dummy
	param->effect = this;
	this->params.push_back(param);
}

KaleidoScopeEffect::KaleidoScopeEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Slices"; 
	param->value = 8.0;
	param->range[0] = 0;
	param->range[1] = 32.0f;
	param->sliding = false;
	param->shadervar = "kalslices";
	param->effect = this;
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
	this->params.push_back(param);
	param = new Param;
	param->name = "Threshold2";
	param->value = 5.0f;
	param->range[0] = 0.0f;
	param->range[1] = 10.0f;
	param->sliding = true;
	param->shadervar = "edge_thres2";
	param->effect = this;
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
	this->params.push_back(param);
}

GlitchEffect::GlitchEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Size";
	param->value = 1.0f;
	param->range[0] = 0.2f;
	param->range[1] = 4.0f;
	param->sliding = true;
	param->shadervar = "glitchstr";
	param->effect = this;
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
	this->params.push_back(param);
	param = new Param;
	param->name = "Y";
	param->value = 0.0f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = false;
	param->shadervar = "yflip";
	param->effect = this;
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
	this->params.push_back(param);
	param = new Param;
	param->name = "Ydir";
	param->value = 1.0f;
	param->range[0] = 0.0f;
	param->range[1] = 2.0f;
	param->sliding = false;
	param->shadervar = "ymirror";
	param->effect = this;
	this->params.push_back(param);
}

float RippleEffect::get_speed() { return this->speed; }
float RippleEffect::get_ripplecount() { return this->ripplecount; }
void RippleEffect::set_ripplecount(float count) { this->ripplecount = count; }

int StrobeEffect::get_phase() { return this->phase; }
void StrobeEffect::set_phase(int phase) { this->phase = phase; }


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
	lay->numefflines += effect->numrows;
	if (lay->numefflines > 11) {
		int further = (lay->numefflines - lay->effscroll[cat]) - 11;
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
	
	// add parameters to OSC ssystem
	if (lay->numoftypemap.find(effect->type) != lay->numoftypemap.end()) lay->numoftypemap[effect->type]++;
	else lay->numoftypemap[effect->type] = 1;
	for (int i = 0; i < effect->params.size(); i++) {
		std::string deckstr;
		if (lay->deck == 0) deckstr = "/deckA";
		else deckstr = "/deckB";
		std::string compstr;
		if (mainprogram->preveff) compstr = "preview";
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

	if (mainprogram->preveff) {
		eff = do_add_effect(this, type, pos, false);
	}
	else {
		eff = do_add_effect(this, type, pos, true);
	}
	this->currcliptype = ELEM_LAYER;
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
	
	for (int i = 0; i < lay->node->page->nodes.size(); i++) {
		if (evec[pos]->node == lay->node->page->nodes[i]) {
			lay->node->page->nodes.erase(lay->node->page->nodes.begin() + i);
		}
	}
	if (lay->lasteffnode[cat] == evec[pos]->node) {
		if (pos == 0) {
			if (!cat) lay->lasteffnode[cat] = lay->node;
			else {
				if (lay->pos == 0) {
					lay->lasteffnode[0]->out.clear();
					mainprogram->nodesmain->currpage->connect_nodes(lay->lasteffnode[0], lay->lasteffnode[1]->out[0]);
				}
				else {	
					lay->blendnode->out.clear();
					mainprogram->nodesmain->currpage->connect_nodes(lay->blendnode, lay->lasteffnode[1]->out[0]);
				}
				lay->lasteffnode[1] = lay->blendnode;
			}
		}
		else {
			lay->lasteffnode[cat] = evec[pos - 1]->node;
			lay->lasteffnode[cat]->out = evec[pos]->node->out;
			if (cat) evec[pos]->node->out[0]->in = lay->lasteffnode[1];
		}
		if (!cat) {
			if (lay->pos == 0) {
				lay->lasteffnode[cat]->out[0]->in = lay->lasteffnode[cat];
			}
			else {
				((BlendNode*)lay->lasteffnode[0]->out[0])->in2 = lay->lasteffnode[0];
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
	this->currcliptype = ELEM_LAYER;
}		


Layer* Mixer::add_layer(std::vector<Layer*> &layers, int pos) {
	bool comp;
	if (layers == this->layersA or layers == this->layersB) comp = false;
	else comp = true;
	Layer *layer = new Layer(comp);
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
		if (prevlay->streameffects.size()) {
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
			
		while (!testlay->effects.empty()) {
			mainprogram->nodesmain->currpage->delete_node(testlay->effects.back()->node);
			for (int j = 0; j < testlay->effects.back()->params.size(); j++) {
				delete testlay->effects.back()->params[j];
			}
			delete testlay->effects.back();
			testlay->effects.pop_back();
		}
		while (!testlay->streameffects.empty()) {
			mainprogram->nodesmain->currpage->delete_node(testlay->streameffects.back()->node);
			for (int j = 0; j < testlay->streameffects.back()->params.size(); j++) {
				delete testlay->streameffects.back()->params[j];
			}
			delete testlay->streameffects.back();
			testlay->streameffects.pop_back();
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
	clay->shiftx = lay->shiftx;
	clay->shifty = lay->shifty;
	clay->scale = lay->scale;
	clay->opacity->value = lay->opacity->value;
	clay->volume->value = lay->volume->value;
	clay->chtol->value = lay->chtol->value;
	clay->chdir->value = lay->chdir->value;
	clay->chinv->value = lay->chinv->value;
	if (lay->live) {
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
	}
			
	for (int i = 0; i < slayers.size(); i++) {
		dlayers[i]->effects.clear();
		for (int j = 0; j < slayers[i]->effects.size(); j++) {
			Effect *eff = slayers[i]->effects[j];
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
			dlayers[i]->effects.push_back(ceff);
			for (int k = 0; k < slayers[i]->effects[j]->params.size(); k++) {
				Param *par = slayers[i]->effects[j]->params[k];
				Param *cpar = dlayers[i]->effects[j]->params[k];
				lp1->parmap[par] = cpar;
				cpar->value = par->value;
				cpar->midi[0] = par->midi[0];
				cpar->midi[1] = par->midi[1];
				cpar->effect = ceff;
				lp2->allparams.push_back(cpar);
			}
		}
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
	for (int i = 0; i < slay->effects.size(); i++) {
		Effect *eff = dlay->add_effect(slay->effects[i]->type, i);
		for (int j = 0; j < slay->effects[i]->params.size(); j++) {
			Param *par = slay->effects[i]->params[j];
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
	return dlay;
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
					for (int m = 0; m < sourcelayersA[k]->effects.size(); m++) {
						Effect *eff = sourcelayersA[k]->effects[m];
						if (eff == ((EffectNode*)node)->effect) {
							cnode->effect = destlayersA[k]->effects[m];
							destlayersA[k]->effects[m]->node = cnode;
							break;
						}
					}
				}
				for (int k = 0; k < sourcelayersB.size(); k++) {
					for (int m = 0; m < sourcelayersB[k]->effects.size(); m++) {
						Effect *eff = sourcelayersB[k]->effects[m];
						if (eff == ((EffectNode*)node)->effect) {
							cnode->effect = destlayersB[k]->effects[m];
							destlayersB[k]->effects[m]->node = cnode;
							break;
						}
					}
				}
				for (int k = 0; k < sourcelayersA.size(); k++) {
					for (int m = 0; m < sourcelayersA[k]->streameffects.size(); m++) {
						Effect *eff = sourcelayersA[k]->streameffects[m];
						if (eff == ((EffectNode*)node)->effect) {
							cnode->effect = destlayersA[k]->streameffects[m];
							destlayersA[k]->streameffects[m]->node = cnode;
							break;
						}
					}
				}
				for (int k = 0; k < sourcelayersB.size(); k++) {
					for (int m = 0; m < sourcelayersB[k]->streameffects.size(); m++) {
						Effect *eff = sourcelayersB[k]->streameffects[m];
						if (eff == ((EffectNode*)node)->effect) {
							cnode->effect = destlayersB[k]->streameffects[m];
							destlayersB[k]->streameffects[m]->node = cnode;
							break;
						}
					}
				}
				destnodes.push_back(cnode);
			}
			else if (node->type == BLEND) {
				BlendNode *cnode = new BlendNode();
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
	
	//for (int i = 0; i < sourcelayers.size(); i++) {
	//	Layer *lay = sourcelayers[i];
	//	Layer *clay = destlayers[i];

	//	clay->prevtime = lay->prevtime;
	//	clay->frame = lay->frame;
	//	clay->prevframe = lay->frame - 1;
	//}
	
	make_layboxes();
	
}


std::vector<std::string> Mixer::write_layer(Layer *lay, std::ostream& wfile, bool doclips) {
	wfile << "POS\n";
	wfile << std::to_string(lay->pos);
	wfile << "\n";
	wfile << "LIVE\n";
	wfile << std::to_string(lay->live);
	wfile << "\n";
	wfile << "FILENAME\n";
	wfile << lay->filename;
	wfile << "\n";
	wfile << "RELPATH\n";
	wfile << "./" + boost::filesystem::relative(lay->filename, "./").string();
	wfile << "\n";
	wfile << "MUTE\n";
	wfile << std::to_string(lay->mute);
	wfile << "\n";
	wfile << "SOLO\n";
	wfile << std::to_string(lay->solo);
	wfile << "\n";
	wfile << "CLONESETNR\n";
	wfile << std::to_string(lay->clonesetnr);
	wfile << "\n";
	if (!lay->live) {
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
	if (!lay->live) {
		wfile << "MILLIF\n";
		wfile << std::to_string(lay->millif);
		wfile << "\n";
		//wfile << "PREVTIME";
		//wfile << std::to_string(lay->prevtime);
		wfile << "FRAME\n";
		wfile << std::to_string(lay->frame);
		wfile << "\n";
		//wfile << "PREVFRAME\n";
		//wfile << std::to_string(lay->prevframe);
		//wfile << "\n";
		wfile << "STARTFRAME\n";
		wfile << std::to_string(lay->startframe);
		wfile << "\n";
		wfile << "ENDFRAME\n";
		wfile << std::to_string(lay->endframe);
		wfile << "\n";
		wfile << "NUMF\n";
		wfile << std::to_string(lay->numf);
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
	for (int j = 0; j < lay->effects.size(); j++) {
		Effect *eff = lay->effects[j];
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
	wfile << "ENDOFEFFECTS\n";
	
	wfile << "STREAMEFFECTS\n";
	for (int j = 0; j < lay->streameffects.size(); j++) {
		Effect *eff = lay->streameffects[j];
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
	wfile << "PREVEFF\n";
	wfile << std::to_string(mainprogram->preveff);
	wfile << "\n";
	wfile << "PREVVID\n";
	wfile << std::to_string(mainprogram->prevvid);
	wfile << "\n";
	wfile << "COMPON\n";
	wfile << std::to_string(mainmix->compon);
	wfile << "\n";
	wfile << "ENDOFFILE\n";
	wfile.close();
	
	save_shelf(mainprogram->temppath + "temp.shelf", 2);
	filestoadd.push_back(mainprogram->temppath + "temp.shelf");
	bool save = mainprogram->preveff;
	mainprogram->preveff = true;
	mainmix->save_mix(mainprogram->temppath + "temp.state1");
	filestoadd.push_back(mainprogram->temppath + "temp.state1.mix");
	if (mainmix->compon) {
		mainprogram->preveff = false;
		mainmix->save_mix(mainprogram->temppath + "temp.state2");
		filestoadd.push_back(mainprogram->temppath + "temp.state2.mix");
	}
	//save_genmidis(remove_extension(path) + ".midi");
	mainprogram->preveff = save;
	
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
	if (std::find(loopstation->readelemnrs.begin(), loopstation->readelemnrs.end(), elemnr) == loopstation->readelemnrs.end()) {
		// new loopstation line taken in use
		loop = loopstation->free_element();
		loopstation->readelemnrs.push_back(elemnr);
		loopstation->readelems.push_back(loop);
	}
	else {
		// other parameter(s) of this rfile using this loopstation line already exist
		loop = loopstation->readelems[std::find(loopstation->readelemnrs.begin(), loopstation->readelemnrs.end(), elemnr) - loopstation->readelemnrs.begin()];
	}
	getline(rfile, istring);
	if (loop) {
		loop->loopbut->value = std::stoi(istring);
		if (loop->loopbut->value) {
			loop->eventpos = 0;
			loop->starttime = std::chrono::high_resolution_clock::now();
		}
	}
	getline(rfile, istring);
	if (loop) {
		loop->playbut->value = std::stoi(istring);
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
				loop->eventlist.insert(loop->eventlist.end(), std::make_tuple(time, par, value));
			}
			loop->params.emplace(par);
			if (mainprogram->preveff) lp->elemmap[par] = loop;
			else lpc->elemmap[par] = loop;
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
	wfile << "DECKSPEEDB\n";
	wfile << std::to_string(mainprogram->deckspeed[1]->value);
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
	while (!lay->effects.empty()) {
		mainprogram->nodesmain->currpage->delete_node(lay->effects.back()->node);
		for (int j = 0; j < lay->effects.back()->params.size(); j++) {
			delete lay->effects.back()->params[j];
		}
		delete lay->effects.back();
		lay->effects.pop_back();
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
	
	open_shelf(result + "_0.file", 2);
	mainprogram->preveff = true;
	mainmix->open_mix(result + "_1.file");
	if (exists(result + "_2.file")) {
		mainprogram->preveff = false;
		mainmix->open_mix(result + "_2.file");
	}
	//open_genmidis(remove_extension(path) + ".midi");
	
	while (getline(rfile, istring)) {
		if (istring == "ENDOFFILE") break;
		if (istring == "PREVEFF") {
			getline(rfile, istring);
			mainprogram->preveff = std::stoi(istring);
		}
		if (istring == "PREVVID") {
			getline(rfile, istring);
			mainprogram->prevvid = std::stoi(istring);
		}
		if (istring == "COMPON") {
			getline(rfile, istring);
			mainmix->compon = std::stoi(istring);
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
	
	new_file(2, 1);
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
			if (mainprogram->preveff) {
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
			if (!mainprogram->preveff) {
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
		if (istring == "DECKSPEEDB") {
			getline(rfile, istring);
			mainprogram->deckspeed[1]->value = std::stof(istring);
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
	
	new_file(mainmix->mousedeck, alive);
	
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
	while (getline(rfile, istring)) {
		if (istring == "LAYERSB" or istring == "ENDOFCLIPLAYER" or istring == "ENDOFFILE") {
			return jpegcount;	
		}
		if (istring == "DECKSPEED") {
			getline(rfile, istring);
			mainprogram->deckspeed[deck]->value = std::stof(istring);
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
		if (istring == "LIVE") {
			getline(rfile, istring); 
			lay->live = std::stoi(istring);
		}
		if (istring == "FILENAME") {
			getline(rfile, istring);
			lay->filename = istring;
			if (load) {
				lay->timeinit = false;
				if (lay->filename != "") {
					if (lay->live) {
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
					else {
						open_video(lay->frame, lay, lay->filename, false);
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
					open_video(lay->frame, lay, lay->filename, false);
				}
			}
		}	
		if (istring == "MUTE") {
			getline(rfile, istring); 
			lay->mute = std::stoi(istring);
		}
		if (istring == "SOLO") {
			getline(rfile, istring); 
			lay->solo = std::stoi(istring);
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
		if (istring == "MILLIF") {
			getline(rfile, istring); 
			lay->millif = std::stof(istring);
		}
		if (istring == "FRAME") {
			getline(rfile, istring); 
			lay->frame = std::stof(istring);
		}
		//if (istring == "PREVFRAME") {
		//	getline(rfile, istring); 
		//	lay->prevframe = std::stoi(istring);
		//}
		if (istring == "STARTFRAME") {
			getline(rfile, istring); 
			lay->startframe = std::stoi(istring);
		}
		if (istring == "ENDFRAME") {
			getline(rfile, istring); 
			lay->endframe = std::stoi(istring);
		}
		if (istring == "NUMF") {
			getline(rfile, istring); 
			lay->numf = std::stoi(istring);
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
					lay->chromabox->acolor[0] = lay->blendnode->chred;
				}
				if (istring == "CHGREEN") {
					getline(rfile, istring); 
					lay->blendnode->chgreen = std::stof(istring);
					lay->chromabox->acolor[1] = lay->blendnode->chgreen;
				}
				if (istring == "CHBLUE") {
					getline(rfile, istring); 
					lay->blendnode->chblue = std::stof(istring);
					lay->chromabox->acolor[2] = lay->blendnode->chblue;
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
						glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (int)(glob->w * 0.3f), (int)(glob->h * 0.3f), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
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
		if (lvec[i]->mute) lvec[i]->mute_handle();
	}
	
	return jpegcount;
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
        AV_PIX_FMT_RGBA,
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
		if (!mainmix->compon) continue;
		
		AVFrame *rgbaframe;
		rgbaframe = av_frame_alloc();
		rgbaframe->data[0] = (uint8_t *)mainmix->rgbdata;
		rgbaframe->format = c->pix_fmt;
		rgbaframe->width  = c->width;
		rgbaframe->height = c->height;
	 
		//av_image_fill_arrays(picture->data, picture->linesize,
                               // ptr, pix_fmt, width, height, 1);		avpicture_fill(&rgbaframe, (uint8_t *)mainmix->rgbdata, AV_PIX_FMT_RGBA, c->width, c->height);
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
	if (this->compon) {
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
		glReadPixels(0, 0, mainprogram->ow, (int)mainprogram->oh, GL_RGBA, GL_UNSIGNED_BYTE, BUFFER_OFFSET(0));
		this->rgbdata = glMapBuffer(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY);
		//assert(this->rgbdata);
		this->recordnow = true;
		this->startrecord.notify_one();
		glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
	}
}
