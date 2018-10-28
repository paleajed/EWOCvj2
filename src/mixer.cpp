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

Layer* Mixer::add_layer(std::vector<Layer*> &layers, int pos) {
	bool comp;
	if (layers == this->layersA or layers == this->layersB) comp = false;
	else comp = true;1;
	Layer *layer = new Layer(comp);
	if (layers == this->layersA or layers == this->layersAcomp) layer->deck = 0;
	else layer->deck = 1;
	Clip *clip = new Clip;
	layer->clips.push_back(clip);
	clip->type = ELEM_LAYER;
 	layer->node = mainprogram->nodesmain->currpage->add_videonode(comp);
	layer->node->layer = layer;
	layer->lasteffnode = layer->node;
	if (layers == this->layersA or layers == this->layersAcomp) {
		layer->genmidibut->value = this->genmidi[0]->value;
	}
	else {
		layer->genmidibut->value = this->genmidi[1]->value;
	}
	
	Layer *testlay = NULL;
	
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
		if (layer->pos == 1) bnode->firstlayer = prevlay;
		Node *node;
		if (prevlay->pos > 0) {
			node = prevlay->blendnode;
		}
		else {
			node = prevlay->lasteffnode;
		}
		mainprogram->nodesmain->currpage->connect_nodes(node, layer->lasteffnode, layer->blendnode);
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
		Layer *nextlay = NULL;
		if (layers.size() > 1) nextlay = layers[1];
		if (nextlay) {
			mainprogram->nodesmain->currpage->connect_nodes(bnode, nextlay->lasteffnode->out[0]);
			nextlay->lasteffnode->out.clear();
			nextlay->blendnode = bnode;
			mainprogram->nodesmain->currpage->connect_nodes(layer->node, nextlay->lasteffnode, bnode);
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
	Layer *nextlay = NULL;
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

		if (testlay->pos > 0 and testlay->blendnode) {
			mainprogram->nodesmain->currpage->connect_nodes(testlay->blendnode->in, testlay->blendnode->out[0]);
			mainprogram->nodesmain->currpage->delete_node(testlay->blendnode);
			testlay->blendnode = 0;
		}
		else {
			if (nextlay) {
				nextlay->lasteffnode->out.clear();
				mainprogram->nodesmain->currpage->connect_nodes(nextlay->lasteffnode, nextlay->blendnode->out[0]);
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

	delete testlay;
	
	if (mainmix->clonemap.find(testlay) != mainmix->clonemap.end()) {
		mainmix->clonemap[testlay]->erase(testlay);
		if (mainmix->clonemap[testlay]->size() == 0) {
			delete mainmix->clonemap[testlay];
		}
		mainmix->clonemap.erase(testlay);
	}
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
				if ((BlendNode*)node == sourcelayersA[m]->lasteffnode) destlayersA[m]->lasteffnode = cnode;
				if ((BlendNode*)node == sourcelayersA[m]->blendnode) destlayersA[m]->blendnode = (BlendNode*)cnode;
			}
			for (int m = 0; m < sourcelayersB.size(); m++) {
				if ((BlendNode*)node == sourcelayersB[m]->lasteffnode) destlayersB[m]->lasteffnode = cnode;
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
	if (lay->lasteffnode->out.size()) nextnode = lay->lasteffnode->out[0];
	if (lay->node) {
		if (lay->node != lay->lasteffnode) {
			if (lay->pos > 0) {
				((BlendNode*)nextnode)->in2 = nullptr;
				mainprogram->nodesmain->currpage->connect_in2(lay->node, (BlendNode*)nextnode);
			}
			else {
				nextnode->in = nullptr;
				mainprogram->nodesmain->currpage->connect_nodes(lay->node, nextnode);
			}
		}
		lay->lasteffnode = lay->node;
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
						glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (int)(glob->w * 0.3f), (int)(glob->h * 0.3f), 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
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
			int type;
			Effect *eff = nullptr;
			while (istring != "ENDOFEFFECTS") {
				getline(rfile, istring);
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
			}
		}
	}
	return jpegcount;
}
	
void Mixer::record_video() {
    const AVCodec *codec;
    AVCodecContext *c = NULL;
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
    ret = avcodec_open2(c, codec, NULL);
    
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
        NULL,
        NULL,
        NULL);
    
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
        encode_frame(nullptr, c, this->yuvframe, pkt, f, 0);
    }
    /* flush the encoder */
    encode_frame(nullptr, c, NULL, pkt, f, 0);
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
		#define BUFFER_OFFSET(i) ((char *)NULL + (i))
		glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, this->ioBuf);
		glBufferData(GL_PIXEL_PACK_BUFFER_ARB, (int)(mainprogram->ow * mainprogram->oh) * 4, NULL, GL_DYNAMIC_READ);
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
