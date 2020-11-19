#include "GL/glew.h"
#include "GL/gl.h"
#include "GL/glut.h"

#include <chrono>
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
	this->init();
}

void LoopStation::init() {
	for (int i = 0; i < this->elems.size(); i++) {
		this->elems[i]->init();
		this->elems[i]->lpst = this;
	}
	this->parelemmap.clear();
	this->butelemmap.clear();
	this->currelem = this->elems[0];
}

LoopStationElement::LoopStationElement() {
	this->recbut = new Button(0);
	this->recbut->name[0] = "R";
	this->recbut->toggle = 1;
    this->recbut->box->lcolor[0] = 0.4f;
    this->recbut->box->lcolor[1] = 0.4f;
    this->recbut->box->lcolor[2] = 0.4f;
    this->recbut->box->lcolor[3] = 1.0f;
	this->recbut->box->tooltiptitle = "Record loopstation row ";
	this->recbut->box->tooltip = "Start recording non-automated parameter movements on this loopstation row.  ";
	this->loopbut = new Button(0);
    this->loopbut->name[0] = "L";
	this->loopbut->toggle = 1;
    this->loopbut->box->lcolor[0] = 0.4f;
    this->loopbut->box->lcolor[1] = 0.4f;
    this->loopbut->box->lcolor[2] = 0.4f;
    this->loopbut->box->lcolor[3] = 1.0f;
	this->loopbut->box->tooltiptitle = "Loop play loopstation row ";
	this->loopbut->box->tooltip = "Start loop-playing of recorded parameter movements for this loopstation row. Stops recording if it was still running. ";
	this->playbut = new Button(0);
    this->playbut->name[0] = "1";
	this->playbut->toggle = 1;
    this->playbut->box->lcolor[0] = 0.4f;
    this->playbut->box->lcolor[1] = 0.4f;
    this->playbut->box->lcolor[2] = 0.4f;
    this->playbut->box->lcolor[3] = 1.0f;
	this->playbut->box->tooltiptitle = "One-off play loopstation row ";
	this->playbut->box->tooltip = "Start one-off-play of recorded parameter movements for this loopstation row. Stops at end of recording. ";
	this->speed = new Param;
	this->speed->name = "Speed";
	this->speed->value = 1.0f;
	this->speed->deflt = 1.0f;
	this->speed->range[0] = 0.0f;
	this->speed->range[1] = 4.0f;
	this->speed->sliding = true;
    this->speed->box->lcolor[0] = 0.4f;
    this->speed->box->lcolor[1] = 0.4f;
    this->speed->box->lcolor[2] = 0.4f;
    this->speed->box->lcolor[3] = 1.0f;
	this->speed->box->vtxcoords->w = 0.15f;
	this->speed->box->vtxcoords->h = 0.75f;
	this->speed->box->upvtxtoscr();
	this->speed->box->tooltiptitle = "Loopstation row speed ";
	this->speed->box->tooltip = "Allows multiplying the recorded event's speed of this loopstation row.  Leftdrag sets value. Doubleclick allows numeric entry. ";
	this->colbox = new Box;
    this->colbox->lcolor[0] = 0.4f;
    this->colbox->lcolor[1] = 0.4f;
    this->colbox->lcolor[2] = 0.4f;
    this->colbox->lcolor[3] = 1.0f;
	this->colbox->vtxcoords->w = 0.0465f;
	this->colbox->vtxcoords->h = 0.75f;
	this->colbox->upvtxtoscr();
	this->colbox->tooltiptitle = "Loopstation row color code ";
	this->colbox->tooltip = "Leftclicking this box shows colored boxes on the layer stack scroll strips for layers that contain parameters automated by this loopstation row. A white circle is drawn here when this loopstation row contains data. ";
	this->box = new Box;
    this->box->lcolor[0] = 0.4f;
    this->box->lcolor[1] = 0.4f;
    this->box->lcolor[2] = 0.4f;
    this->box->lcolor[3] = 1.0f;
	this->box->vtxcoords->w = 0.02f;
	this->box->vtxcoords->h = 0.75f;
	this->box->upvtxtoscr();
	this->box->tooltiptitle = "Loopstation row select current ";
	this->box->tooltip = "Leftclicking this box selects this loopstation row for use of R(record), L(loop play this row) and S(one shot play this row) keyboard shortcuts. ";
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
	but->box->vtxcoords->w = 0.0465f;
	but->box->vtxcoords->h = 0.075f;
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
	for (int i = 0; i < this->elems.size(); i++) {
		elems[i]->handle();
	}
	render_text("Loopstation", white, elems[0]->recbut->box->vtxcoords->x1 + 0.015f,
             elems[0]->recbut->box->vtxcoords->y1 + elems[0]->recbut->box->vtxcoords->h * 2.0f - 0.045f, 0.0005f, 0.0008f);
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
						lvec[j]->scrollcol[0] = 0.4f;
						lvec[j]->scrollcol[1] = 0.4f;
						lvec[j]->scrollcol[2] = 0.4f;
						lvec[j]->scrollcol[3] = 0.0f;
					}
				}
			}
		}
	}
	
	if ((this->loopbut->value || this->playbut->value) && this->eventlist.size()) this->set_values();
}

void LoopStationElement::init() {
	this->loopbut->value = 0;
	this->playbut->value = 0;
	this->loopbut->oldvalue = 0;
	this->playbut->oldvalue = 0;
	this->speed->value = 1.0f;
	this->eventlist.clear();
	this->eventpos = 0;
	this->params.clear();
	this->layers.clear();
}

void LoopStationElement::visualize() {
	this->recbut->box->vtxcoords->x1 = -0.8f + 1.1f * !mainmix->currlay[!mainprogram->prevmodus]->deck;
	this->box->vtxcoords->x1 = this->recbut->box->vtxcoords->x1 - 0.02f;
	this->loopbut->box->vtxcoords->x1 = -0.8f + 1.1f * !mainmix->currlay[!mainprogram->prevmodus]->deck + this->loopbut->box->vtxcoords->w;
	this->playbut->box->vtxcoords->x1 = -0.8f + 1.1f * !mainmix->currlay[!mainprogram->prevmodus]->deck + this->playbut->box->vtxcoords->w * 2.0f;
	this->speed->box->vtxcoords->x1 = -0.8f + 1.1f * !mainmix->currlay[!mainprogram->prevmodus]->deck + this->recbut->box->vtxcoords->w * 3.0f;
	this->colbox->vtxcoords->x1 = -0.8f + 1.1f * !mainmix->currlay[!mainprogram->prevmodus]->deck + this->recbut->box->vtxcoords->w * 3.0f + this->speed->box->vtxcoords->w;
	this->recbut->box->vtxcoords->y1 = 0.4f - 0.075f * this->pos;
	this->loopbut->box->vtxcoords->y1 = 0.4f - 0.075f * this->pos;
	this->playbut->box->vtxcoords->y1 = 0.4f - 0.075f * this->pos;
	this->speed->box->vtxcoords->y1 = 0.4f - 0.075f * this->pos;
	this->colbox->vtxcoords->y1 = 0.4f - 0.075f * this->pos;
	this->box->vtxcoords->y1 = 0.4f - 0.075f * this->pos;
	this->recbut->box->upvtxtoscr();
	this->loopbut->box->upvtxtoscr();
	this->playbut->box->upvtxtoscr();
	this->speed->box->upvtxtoscr();
	this->colbox->upvtxtoscr();
	this->box->upvtxtoscr();
	this->speed->handle();
    draw_box(grey, this->colbox->acolor, this->colbox, -1);
	if (this->eventlist.size()) draw_box(this->colbox->lcolor, this->colbox->vtxcoords->x1 + 0.02325f ,
                                      this->colbox->vtxcoords->y1 + 0.0375f, 0.0225f, 1);
	if (this == loopstation->currelem) draw_box(grey, white, this->box, -1);
	else draw_box(grey, nullptr, this->box, -1);
	if (this->box->in() || this->recbut->box->in() || this->loopbut->box->in() || this->playbut->box->in() || this->speed->box->in() || this->colbox->in()) {
        if (!mainprogram->menuondisplay) {
            if (mainprogram->menuactivation) {
                mainprogram->lpstmenu->state = 2;
                mainmix->mouselpstelem = this;
                mainprogram->menuactivation = false;
            }
        }
	}
}
	
void LoopStationElement::erase_elem() {
	std::unordered_set<Param*>::iterator it1;
	for (it1 = this->params.begin(); it1 != this->params.end(); it1++) {
		Param* par = *it1;
		if (par->name.length()) {
			par->box->acolor[0] = 0.2f;
			par->box->acolor[01] = 0.2f;
			par->box->acolor[2] = 0.2f;
			par->box->acolor[3] = 1.0f;
		}
	}
	std::unordered_set<Button*>::iterator it2;
	for (it2 = this->buttons.begin(); it2 != this->buttons.end(); it2++) {
		Button* but = *it2;
		if (but) {
			but->box->acolor[0] = 0.2f;
			but->box->acolor[1] = 0.2f;
			but->box->acolor[2] = 0.2f;
			but->box->acolor[3] = 1.0f;
		}
	}
	this->init();
	this->eventlist.clear();
	this->params.clear();
	this->buttons.clear();
	this->layers.clear();
}

void LoopStationElement::mouse_handle() {
	this->recbut->handle(1, 0);
	if (this->recbut->toggled()) {
		if (this->recbut->value) {
			this->loopbut->value = false;
			this->playbut->value = false;
			this->erase_elem();
			this->starttime = std::chrono::high_resolution_clock::now();
		}
		else {
			if (this->eventlist.size()) {
                std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> elapsed;
                elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - this->starttime);
                this->totaltime = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
				this->loopbut->value = true;
				this->starttime = std::chrono::high_resolution_clock::now();
				this->interimtime = 0;
				this->speedadaptedtime = 0;
			}
		}
	}
	this->loopbut->handle(1, 0);
	if (this->loopbut->toggled()) {
		// start/stop loop play of recording
		if (this->eventlist.size()) {
			this->recbut->value = false;
			this->recbut->oldvalue = false;
			this->playbut->value = false;
			this->playbut->oldvalue = false;
			if (this->loopbut->value) {
				this->eventpos = 0;
				this->starttime = std::chrono::high_resolution_clock::now();
				this->interimtime = 0;
				this->speedadaptedtime = 0;
			}
		}
		else {
			this->loopbut->value = false;
			this->loopbut->oldvalue = false;
		}
	}
	this->playbut->handle(1, 0);
	if (this->playbut->toggled()) {
		// start/stop one-shot play of recording
		if (this->eventlist.size()) {
			this->recbut->value = false;
			this->recbut->oldvalue = false;
			this->loopbut->value = false;
			this->loopbut->oldvalue = false;
			if (this->playbut->value) {
				this->eventpos = 0;
				this->starttime = std::chrono::high_resolution_clock::now();
				this->interimtime = 0;
				this->speedadaptedtime = 0;
			}
		}
		else {
			this->playbut->value = false;
			this->playbut->oldvalue = false;
		}
	}
	//current loopstation element selection
	if (this->box->in() && mainprogram->leftmouse) loopstation->currelem = this;
}
	
void LoopStationElement::set_values() {
	// if current elapsed time in loop > eventtime of events starting from eventpos then set their params to stored values
	std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed;
	elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - this->starttime);
	long long millicount = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
	int passed = millicount - this->interimtime;
	this->interimtime = millicount;
	this->speedadaptedtime = this->speedadaptedtime + passed * this->speed->value;
	std::tuple<long long, Param*, Button*, float> event;
	event = this->eventlist[this->eventpos];
	while (this->speedadaptedtime > std::get<0>(event)) {
	    // play all recorded events upto now
		Param *par = std::get<1>(event);
		Button *but = std::get<2>(event);
		lpc = lpc;
		if (par) {
			if (par != mainmix->adaptparam) {
				if (std::find(this->lpst->allparams.begin(), this->lpst->allparams.end(), par) != this->lpst->allparams.end()) {
					par->value = std::get<3>(event);
				}
			}
		}
		else if (but) {
			if (std::find(this->lpst->allbuttons.begin(), this->lpst->allbuttons.end(), but) != this->lpst->allbuttons.end()) {
				but->value = (int)(std::get<3>(event) + 0.5f);
			}
		}
		this->eventpos++;
		event = this->eventlist[this->eventpos];
	}
    if (this->speedadaptedtime >= this->totaltime) {
        // reached end of loopstation element recording
        if (this->eventlist.size() == 0) {
            // end loop when one-shot playing or no of the params exist anymore
            this->playbut->value = false;
            this->loopbut->value = false;
        }
        else {
            if (this->loopbut->value) {
                //start loop again
                this->eventpos = 0;
                this->speedadaptedtime -= this->totaltime;
                this->interimtime = this->speedadaptedtime * this->speed->value;
                this->starttime = std::chrono::high_resolution_clock::now() - std::chrono::milliseconds((long long)this->interimtime);
            }
            else if (this->playbut->value) {
                //end of single shot eventlist play
                this->playbut->value = false;
                this->playbut->oldvalue = true;
            }
        }
    }
}
		
void LoopStationElement::add_param(Param* par) {
    if (loopstation->parelemmap[par]) return;  // each parameter can be automated only once to avoid chaos
	std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed;
	elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - this->starttime);
	long long millicount = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
	std::tuple<long long, Param*, Button*, float> event;
	event = std::make_tuple(millicount, par, nullptr, par->value);
	this->eventlist.push_back(event);
	this->params.emplace(par);
	loopstation->parelemmap[par] = this;
	if (par->effect) {
		this->layers.emplace(par->effect->layer);
	}
	par->box->acolor[0] = this->colbox->acolor[0];
	par->box->acolor[1] = this->colbox->acolor[1];
	par->box->acolor[2] = this->colbox->acolor[2];
	par->box->acolor[3] = this->colbox->acolor[3];
	for (int i = 0; i < 2; i++) {
		std::vector<Layer*>& lvec = choose_layers(i);
		for (int j = 0; j < lvec.size(); j++) {
			if (par == lvec[j]->speed) this->layers.emplace(lvec[j]);
			if (par == lvec[j]->opacity) this->layers.emplace(lvec[j]);
            if (par == lvec[j]->volume) this->layers.emplace(lvec[j]);
            if (par == lvec[j]->scritch) this->layers.emplace(lvec[j]);
            if (par == lvec[j]->blendnode->mixfac) this->layers.emplace(lvec[j]);
			if (par == lvec[j]->shiftx) {
				this->layers.emplace(lvec[j]);
				par = lvec[j]->shifty;
				this->add_param(par);
				par = nullptr;
				return;
			}
			if (par == lvec[j]->blendnode->wipex) {
				this->layers.emplace(lvec[j]);
				par = lvec[j]->blendnode->wipey;
				this->add_param(par);
				par = nullptr;
				return;
			}
			if (par == lvec[j]->scale) {
				this->layers.emplace(lvec[j]);
				par = nullptr;
				return;
			}
		}
	}
	if (par == mainmix->wipex[0]) {
		par = mainmix->wipey[0];
		this->add_param(par);
		par = nullptr;
		return;
	}
	if (par == mainmix->wipex[1]) {
		par = mainmix->wipey[1];
		this->add_param(par);
		par = nullptr;
		return;
	}
}

void LoopStationElement::add_button(Button* but) {
    if (loopstation->butelemmap[but]) return;  // each button can be automated only once to avoid chaos
	std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed;
	elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - this->starttime);
	long long millicount = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
	std::tuple<long long, Param*, Button*, float> event;
	event = std::make_tuple(millicount, nullptr, but, (float)but->value);
	this->eventlist.push_back(event);
	this->buttons.emplace(but);
	loopstation->butelemmap[but] = this;
	if (but->layer) {
		this->layers.emplace(but->layer);
	}
	but->box->acolor[0] = this->colbox->acolor[0];
	but->box->acolor[1] = this->colbox->acolor[1];
	but->box->acolor[2] = this->colbox->acolor[2];
	but->box->acolor[3] = this->colbox->acolor[3];
}


LoopStationElement* LoopStation::free_element() {
	LoopStationElement *loop = nullptr;
	for (int i = 0; i < this->elems.size(); i++) {
		LoopStationElement *elem = this->elems[i];
		if (!elem->recbut->value && elem->eventlist.size() == 0) {
			loop = elem;
			break;
		}
	}
	return loop;
}