#include "GL/glew.h"
#include "GL/gl.h"
#include "GL/glut.h"

#include <iostream>
#include <chrono>
#include <ctime>
#include <algorithm>

#include "node.h"
#include "box.h"
#include "effect.h"
#include "layer.h"
#include "window.h"
#include "program.h"
#include "loopstation.h"
#include "bins.h"

LoopStation::LoopStation() {
	for (int i = 0; i < this->numelems; i++) {
		LoopStationElement *elem = this->add_elem();
		elem->colbox->acolor[0] = this->colvals[i*3];
		elem->colbox->acolor[1] = this->colvals[i*3 + 1];
		elem->colbox->acolor[2] = this->colvals[i*3 + 2];
		elem->colbox->acolor[3] = 1.0f;
	}
}

void LoopStation::init() {
	for (int i = 0; i < this->elems.size(); i++) {
		this->elems[i]->init();
	}
	this->elemmap.clear();
}

LoopStationElement::LoopStationElement() {
	this->recbut = new Button(0);
	this->recbut->box->tooltiptitle = "Record loopstation row ";
	this->recbut->box->tooltip = "Start recording non-automated parameter movements on this loopstation row.  ";
	this->loopbut = new Button(0);
	this->loopbut->box->tooltiptitle = "Loop play loopstation row ";
	this->loopbut->box->tooltip = "Start loop-playing of recorded parameter movements for this loopstation row. Stops recording if it was still running. ";
	this->playbut = new Button(0);
	this->playbut->box->tooltiptitle = "One-off play loopstation row ";
	this->playbut->box->tooltip = "Start one-off-play of recorded parameter movements for this loopstation row. Stops at end of recording. ";
	this->speed = new Param;
	this->speed->name = "Speed";
	this->speed->value = 1.0f;
	this->speed->range[0] = 0.0f;
	this->speed->range[1] = 4.0f;
	this->speed->sliding = true;
	this->speed->box->vtxcoords->w = tf(0.1f);
	this->speed->box->vtxcoords->h = tf(0.05f);
	this->speed->box->upvtxtoscr();
	this->speed->box->tooltiptitle = "Loopstation row speed ";
	this->speed->box->tooltip = "Allows multiplying the recorded event's speed of this loopstation row.  Leftdrag sets value. Doubleclick allows numeric entry. ";
	this->colbox = new Box;
	this->colbox->vtxcoords->w = tf(0.031f);
	this->colbox->vtxcoords->h = tf(0.05f);
	this->colbox->upvtxtoscr();
	this->colbox->tooltiptitle = "Loopstation row color code ";
	this->colbox->tooltip = "Leftclicking this box shows colored boxes on both deck layer scroll strips for layers that contain parameters automated by this loopstation row. ";
}

LoopStationElement::~LoopStationElement() {
	delete this->recbut;
	delete this->loopbut;
	delete this->playbut;
}

void LoopStation::setbut(Button *but, float r, float g, float b) {
	but->ccol[0] = r;
	but->ccol[1] = g;
	but->ccol[2] = b;
	but->box->vtxcoords->w = tf(0.031f);
	but->box->vtxcoords->h = tf(0.05f);
	but->box->upvtxtoscr();
}

LoopStationElement* LoopStation::add_elem() {
	LoopStationElement *elem = new LoopStationElement;
	this->elems.push_back(elem);
	elem->pos = this->elems.size() - 1;
	// float values are colors for circles in boxes
	this->setbut(elem->recbut, 0.8f, 0.0f, 0.0f);
	this->setbut(elem->loopbut, 0.0f, 0.8f, 0.0f);
	this->setbut(elem->playbut, 0.0f, 0.0f, 0.8f);
	return elem;
}

void LoopStation::handle() {
	float white[] = {1.0f, 1.0f, 1.0f, 1.0f};
	for (int i = 0; i < this->elems.size(); i++) {
		elems[i]->handle();
	}
	render_text("Loopstation", white, elems[0]->recbut->box->vtxcoords->x1 + tf(0.01f), elems[0]->recbut->box->vtxcoords->y1 + elems[0]->recbut->box->vtxcoords->h * 2.0f - tf(0.03f), 0.0005f, 0.0008f);
}

void LoopStationElement::handle() {
	this->mouse_handle();
	this->visualize();
	if (this->colbox->in()) {
		if (mainprogram->leftmouse) {
			mainprogram->leftmouse = false;
			for (int i = 0; i < 2; i++) {
				std::vector<Layer*> &lvec = choose_layers(i);
				for (int j = 0; j < lvec.size(); j++) {
					if (std::find(this->layers.begin(), this->layers.end(), lvec[j]) != this->layers.end()) {
						lvec[j]->scrollcol[0] = this->colbox->acolor[0];
						lvec[j]->scrollcol[1] = this->colbox->acolor[1];
						lvec[j]->scrollcol[2] = this->colbox->acolor[2];
						lvec[j]->scrollcol[3] = this->colbox->acolor[3];
					}
					else {
						lvec[j]->scrollcol[0] = 0.5f;
						lvec[j]->scrollcol[1] = 0.5f;
						lvec[j]->scrollcol[2] = 0.5f;
						lvec[j]->scrollcol[3] = 0.0f;
					}
				}
			}
		}
	}
	
	if (this->loopbut->value or this->playbut->value) this->set_params();
}

void LoopStationElement::init() {
	this->recbut->value = 0;
	this->loopbut->value = 0;
	this->playbut->value = 0;
	this->speed->value = 1.0f;
	this->eventlist.clear();
	this->params.clear();
	this->layers.clear();
}

void LoopStationElement::visualize() {
	this->recbut->box->vtxcoords->x1 = -0.8f + 1.1f * !mainmix->currlay->deck;
	this->loopbut->box->vtxcoords->x1 = -0.8f + 1.1f * !mainmix->currlay->deck + this->loopbut->box->vtxcoords->w;
	this->playbut->box->vtxcoords->x1 = -0.8f + 1.1f * !mainmix->currlay->deck + this->playbut->box->vtxcoords->w * 2.0f;		
	this->speed->box->vtxcoords->x1 = -0.8f + 1.1f * !mainmix->currlay->deck + this->recbut->box->vtxcoords->w * 3.0f;
	this->colbox->vtxcoords->x1 = -0.8f + 1.1f * !mainmix->currlay->deck + this->recbut->box->vtxcoords->w * 3.0f + this->speed->box->vtxcoords->w;
	this->recbut->box->vtxcoords->y1 = 0.4f - tf(0.05f) * this->pos;
	this->loopbut->box->vtxcoords->y1 = 0.4f - tf(0.05f) * this->pos;
	this->playbut->box->vtxcoords->y1 = 0.4f - tf(0.05f) * this->pos;
	this->speed->box->vtxcoords->y1 = 0.4f - tf(0.05f) * this->pos;
	this->colbox->vtxcoords->y1 = 0.4f - tf(0.05f) * this->pos;
	this->recbut->box->upvtxtoscr();
	this->loopbut->box->upvtxtoscr();
	this->playbut->box->upvtxtoscr();
	this->speed->box->upvtxtoscr();
	this->colbox->upvtxtoscr();
	this->speed->handle();
	draw_box(this->colbox, -1);
}
	
void LoopStationElement::erase_elem() {
	this->eventlist.clear();
	std::unordered_set<Param*>::iterator it;
	for (it = this->params.begin(); it != this->params.end(); it++) {
		Param *par = *it;
		par->box->acolor[0] = 0.2f;
		par->box->acolor[1] = 0.2f;
		par->box->acolor[2] = 0.2f;
		par->box->acolor[3] = 1.0f;
	}
	this->params.clear();
	this->layers.clear();
}

void LoopStationElement::mouse_handle() {
	this->recbut->handle(1);
	if (this->recbut->toggled()) {
		if (this->recbut->value) {
			this->loopbut->value = false;
			this->playbut->value = false;
			this->erase_elem();
			this->starttime = std::chrono::high_resolution_clock::now();
		}
		else {
			if (this->eventlist.size()) {
				this->loopbut->value = true;
				this->starttime = std::chrono::high_resolution_clock::now();
				this->interimtime = 0;
				this->speedadaptedtime = 0;
			}
		}
	}
	this->loopbut->handle(1);
	if (this->loopbut->toggled()) {
		// start/stop loop play of recording
		if (this->eventlist.size()) {
			this->loopbut->value = !this->loopbut->value;
			this->recbut->value = false;
			this->playbut->value = false;
			if (this->loopbut->value) {
				this->eventpos = 0;
				this->starttime = std::chrono::high_resolution_clock::now();
				this->interimtime = 0;
				this->speedadaptedtime = 0;
			}
		}
	}
	this->playbut->handle(1);
	if (this->playbut->toggled()) {
		// start/stop one-shot play of recording
		if (this->eventlist.size()) {
			this->playbut->value = !this->playbut->value;
			this->recbut->value = false;
			this->loopbut->value = false;
			if (this->playbut->value) {
				this->eventpos = 0;
				this->starttime = std::chrono::high_resolution_clock::now();
				this->interimtime = 0;
				this->speedadaptedtime = 0;
			}
		}
	}
}
	
void LoopStationElement::set_params() {
	// if current elapsed time in loop > eventtime of events starting from eventpos then set their params to stored values
	std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed;
	elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - this->starttime);
	long long millicount = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
	int passed = millicount - this->interimtime;
	this->interimtime = millicount;
	this->speedadaptedtime = this->speedadaptedtime + passed * this->speed->value;
	std::tuple<long long, Param*, float> event;
	event = this->eventlist[this->eventpos];
	while (this->speedadaptedtime > std::get<0>(event)) {
		Param *par = std::get<1>(event);
		if (par != mainmix->adaptparam) {
			if (std::find(loopstation->allparams.begin(), loopstation->allparams.end(), par) != loopstation->allparams.end()) {
				par->value = std::get<2>(event);
			}
		}
		this->eventpos++;
		if (this->eventpos >= this->eventlist.size()) {
			if (this->eventlist.size() == 0) {
				// end loop when one-shot playing or no of the params exist anymore
				this->playbut->value = false;
				this->loopbut->value = false;
			}
			if (this->eventpos >= this->eventlist.size() and this->loopbut->value) {
				//start loop again
				this->eventpos = 0;
				this->starttime = std::chrono::high_resolution_clock::now();
				this->interimtime = 0;
				this->speedadaptedtime = 0;
			}
			break;
		}
		event = this->eventlist[this->eventpos];
	}
}		
		
void LoopStationElement::add_param() {
	std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed;
	elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - this->starttime);
	long long millicount = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
	std::tuple<long long, Param*, float> event;
	event = std::make_tuple(millicount, mainmix->adaptparam, mainmix->adaptparam->value);
	this->eventlist.push_back(event);
	this->params.emplace(mainmix->adaptparam);
	loopstation->elemmap[mainmix->adaptparam] = this;
	if (mainmix->adaptparam->effect) {
		this->layers.emplace(mainmix->adaptparam->effect->layer);
	}
	for (int i = 0; i < 2; i++) {
		std::vector<Layer*> &lvec = choose_layers(i);
		for (int j = 0; j < lvec.size(); j++) {
			if (mainmix->adaptparam == lvec[j]->speed) this->layers.emplace(lvec[j]);
			if (mainmix->adaptparam == lvec[j]->opacity) this->layers.emplace(lvec[j]);
			if (mainmix->adaptparam == lvec[j]->volume) this->layers.emplace(lvec[j]);
			if (mainmix->adaptparam == lvec[j]->shiftx) {
				this->layers.emplace(lvec[j]);
				mainmix->adaptparam = lvec[j]->shifty;
				this->add_param();
				mainmix->adaptparam = nullptr;
				return;
			}
			if (mainmix->adaptparam == lvec[j]->scale) {
				this->layers.emplace(lvec[j]);
				mainmix->adaptparam = nullptr;
				return;
			}
		}
	}
	mainmix->adaptparam->box->acolor[0] = this->colbox->acolor[0];
	mainmix->adaptparam->box->acolor[1] = this->colbox->acolor[1];
	mainmix->adaptparam->box->acolor[2] = this->colbox->acolor[2];
	mainmix->adaptparam->box->acolor[3] = this->colbox->acolor[3];
}

LoopStationElement* LoopStation::free_element() {
	LoopStationElement *loop = nullptr;
	for (int i = 0; i < this->elems.size(); i++) {
		LoopStationElement *elem = this->elems[i];
		if (!elem->recbut->value and elem->eventlist.size() == 0) {
			loop = elem;
			break;
		}
	}
	return loop;
}