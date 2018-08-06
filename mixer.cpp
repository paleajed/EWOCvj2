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

#include "GL\glew.h"
#include "GL\gl.h"
#include "GL\glut.h"

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
	this->numh = this->numh * w / h;
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

Mixer::do_deletelay(Layer *testlay, std::vector<Layer*> &layers, bool add) {
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
	wfile << ".\\" + boost::filesystem::relative(lay->filename, ".\\").string();
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
				clipjpegpath = mainprogram->binsdir + name + ".jpg";
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

Mixer::save_layerfile(const std::string &path, Layer *lay, bool doclips) {
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

Mixer::save_state(const std::string &path) {
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
	
	save_shelf(mainprogram->temppath + "temp.shelf");
	filestoadd.push_back(mainprogram->temppath + "temp.shelf");
	bool save = mainprogram->preveff;
	mainprogram->preveff = true;
	mainmix->save_mix(mainprogram->temppath + "temp.state1");
	filestoadd.push_back(mainprogram->temppath + "temp.state1.ewoc");
	if (mainmix->compon) {
		mainprogram->preveff = false;
		mainmix->save_mix(mainprogram->temppath + "temp.state2");
		filestoadd.push_back(mainprogram->temppath + "temp.state2.ewoc");
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

Mixer::event_write(std::ostream &wfile, Param *par) {
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

Mixer::event_read(std::istream &rfile, Param *par, Layer *lay) {
	std::string istring;
	LoopStationElement *loop = nullptr;
	getline(rfile, istring);
	int elemnr = std::stoi(istring);
	if (std::find(loopstation->readelemnrs.begin(), loopstation->readelemnrs.end(), elemnr) == loopstation->readelemnrs.end()) {
		loop = loopstation->free_element();
		loopstation->readelemnrs.push_back(elemnr);
		loopstation->readelems.push_back(loop);
	}
	else loop = loopstation->readelems[std::find(loopstation->readelemnrs.begin(), loopstation->readelemnrs.end(), elemnr) - loopstation->readelemnrs.begin()];
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
		loop->layers.push_back(lay);
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
			if (!inserted) loop->eventlist.insert(loop->eventlist.end(), std::make_tuple(time, par, value));
			par->box->acolor[0] = loop->colbox->acolor[0];
			par->box->acolor[1] = loop->colbox->acolor[1];
			par->box->acolor[2] = loop->colbox->acolor[2];
			par->box->acolor[3] = loop->colbox->acolor[3];
		}
	}
}

Mixer::save_mix(const std::string &path) {
	std::string ext = path.substr(path.length() - 5, std::string::npos);
	std::string str;
	if (ext != ".ewoc") str = path + ".ewoc";
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
	outputfile.open("./tempconcat", std::ios::out | std::ios::binary);
	concat_files(outputfile, str, jpegpaths);
	outputfile.close();
	boost::filesystem::rename("./tempconcat", str);
}

Mixer::save_deck(const std::string &path) {
	std::string ext = path.substr(path.length() - 5, std::string::npos);
	std::string str;
	if (ext != ".deck") str = path + ".deck";
	else str = path;
	std::ofstream wfile;
	wfile.open(str.c_str());
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
	outputfile.open("./tempconcat", std::ios::out | std::ios::binary);
	concat_files(outputfile, str, jpegpaths);
	outputfile.close();
	boost::filesystem::rename("./tempconcat", str);
}

Mixer::open_layerfile(const std::string &path, Layer *lay, int reset, bool doclips) {
	std::string result = deconcat_files(path);
	bool concat = (result != "");
	std::ifstream rfile;
	if (concat) rfile.open(result);
	else rfile.open(path);
	
	std::string istring;
	
	if (lay->node) {
		if (lay->lasteffnode->out.size()) {
			lay->lasteffnode->out[0]->in = nullptr;
			mainprogram->nodesmain->currpage->connect_nodes(lay->node, lay->lasteffnode->out[0]);
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
	mainmix->read_layers(rfile, result, layers, lay->deck, 0, doclips, concat);
	
	rfile.close();
}

Mixer::open_state(const std::string &path) {
	std::string result = deconcat_files(path);
	bool concat = (result != "");
	std::ifstream rfile;
	if (concat) rfile.open(result);
	else rfile.open(path);
	
	std::string istring;
	getline(rfile, istring);
	
	open_shelf(result + "_0.file");
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

Mixer::open_mix(const std::string &path) {
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
			int jpegcount = mainmix->read_layers(rfile, result, layersA, 0, 2, concat, 1);
			mainprogram->filecount = jpegcount;
			std::vector<Layer*> &layersB = choose_layers(1);
			mainmix->read_layers(rfile, result, layersB, 1, 2, concat, 1);
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

Mixer::open_deck(const std::string &path, bool alive) {
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
	mainmix->read_layers(rfile, result, layers, mainmix->mousedeck, 1, 1, concat);
	
	rfile.close();
}

Mixer::read_layers(std::istream &rfile, const std::string &result, std::vector<Layer*> &layers, bool deck, int type, bool doclips, bool concat, bool load) {
	Layer *lay = nullptr;
	std::string istring;
	int jpegcount = 0;
	if (mainprogram->filecount) jpegcount = mainprogram->filecount;
	while (getline(rfile, istring)) {
		if (istring == "LAYERSB" or istring == "ENDOFCLIPLAYER") {
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
						mainmix->read_layers(rfile, result, cliplayers, 0, 0, 0, 1, 0);
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
						glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (int)(w * 0.3f), (int)(h * 0.3f), 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
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
							mainmix->event_read(rfile, par, lay);
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

