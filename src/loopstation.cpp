#include "GL/glew.h"
#include "GL/gl.h"
#define FREEGLUT_STATIC
#define FREEGLUT_LIB_PRAGMAS 0

#include <algorithm>
#include <set>

#include "node.h"
#include "box.h"
#include "effect.h"
#include "layer.h"
#include "window.h"
#include "program.h"
#include "loopstation.h"
#include "bins.h"

LoopStation::LoopStation() {
    this->upscrbox = new Boxx;
    this->upscrbox->vtxcoords->y1 = 0.4f - 0.075f * 0;
    this->upscrbox->vtxcoords->w = 0.04f;
    this->upscrbox->vtxcoords->h = 0.075f;
    this->upscrbox->upvtxtoscr();
    this->upscrbox->tooltiptitle = "Loopstation scroll up box ";
    this->upscrbox->tooltip = "Leftclicking this box scrolls the loopstation element list upwards. ";
    this->downscrbox = new Boxx;
    this->downscrbox->vtxcoords->y1 = 0.4f - 0.075f * 7;
    this->downscrbox->vtxcoords->w = 0.04f;
    this->downscrbox->vtxcoords->h = 0.075f;
    this->downscrbox->upvtxtoscr();
    this->downscrbox->tooltiptitle = "Loopstation scroll down box ";
    this->downscrbox->tooltip = "Leftclicking this box scrolls the loopstation element list downwards. ";
    this->confupscrbox = new Boxx;
    this->confupscrbox->vtxcoords->x1 = -0.33f;
    this->confupscrbox->vtxcoords->y1 = 0.45f - 0.15f * -1;
    this->confupscrbox->vtxcoords->w = 0.08f;
    this->confupscrbox->vtxcoords->h = 0.15f;
    this->confupscrbox->upvtxtoscr();
    this->confupscrbox->tooltiptitle = "Loopstation midi config scroll up box ";
    this->confupscrbox->tooltip = "Leftclicking this box scrolls the loopstation element list upwards. ";
    this->confdownscrbox = new Boxx;
    this->confdownscrbox->vtxcoords->x1 = -0.33f;
    this->confdownscrbox->vtxcoords->y1 = 0.45f - 0.15f * 8;
    this->confdownscrbox->vtxcoords->w = 0.08f;
    this->confdownscrbox->vtxcoords->h = 0.15f;
    this->confdownscrbox->upvtxtoscr();
    this->confdownscrbox->tooltiptitle = "Loopstation midi config scroll down box ";
    this->confdownscrbox->tooltip = "Leftclicking this box scrolls the loopstation element list downwards. ";

	for (int i = 0; i < this->numelems; i++) {
		this->add_elem();
	}
    this->currelem = this->elements[0];
	this->init();
}

LoopStation::~LoopStation() {
    for (LoopStationElement *elem : this->elements) {
        delete elem;
    }
    delete this->upscrbox;
    delete this->downscrbox;
    delete this->confupscrbox;
    delete this->confdownscrbox;
}

void LoopStation::init() {
	for (auto & element : this->elements) {
		element->init();
	}
	this->parelemmap.clear();
	this->butelemmap.clear();
	this->currelem = this->elements[0];
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
    this->loopbut->name[0] = "T";
	this->loopbut->toggle = 1;
    this->loopbut->box->lcolor[0] = 0.4f;
    this->loopbut->box->lcolor[1] = 0.4f;
    this->loopbut->box->lcolor[2] = 0.4f;
    this->loopbut->box->lcolor[3] = 1.0f;
	this->loopbut->box->tooltiptitle = "Loop play loopstation row ";
	this->loopbut->box->tooltip = "Start loop-playing of recorded parameter movements for this loopstation row. Stops recording if it was still running. ";
	this->playbut = new Button(0);
    this->playbut->name[0] = "Y";
	this->playbut->toggle = 1;
    this->playbut->box->lcolor[0] = 0.4f;
    this->playbut->box->lcolor[1] = 0.4f;
    this->playbut->box->lcolor[2] = 0.4f;
    this->playbut->box->lcolor[3] = 1.0f;
	this->playbut->box->tooltiptitle = "One-off play loopstation row ";
	this->playbut->box->tooltip = "Start one-off-play of recorded parameter movements for this loopstation row. Stops at end of recording. ";
	this->speed = new Param;
	this->speed->name = "LPST speed";
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
	this->speed->box->vtxcoords->h = 0.075f;
	this->speed->box->upvtxtoscr();
	this->speed->box->tooltiptitle = "Loopstation row speed ";
	this->speed->box->tooltip = "Allows multiplying the recorded event's speed of this loopstation row.  Leftdrag sets value. Doubleclick allows numeric entry. ";
	this->colbox = new Boxx;
    this->colbox->lcolor[0] = 0.4f;
    this->colbox->lcolor[1] = 0.4f;
    this->colbox->lcolor[2] = 0.4f;
    this->colbox->lcolor[3] = 1.0f;
	this->colbox->vtxcoords->w = 0.0465f;
	this->colbox->vtxcoords->h = 0.075f;
	this->colbox->upvtxtoscr();
	this->colbox->tooltiptitle = "Loopstation row color code ";
	this->colbox->tooltip = "Leftclicking this box shows colored boxes on the layer stack scroll strips for layers that contain parameters automated by this loopstation row. A white circle is drawn here when this loopstation row contains data. ";
    this->box = new Boxx;
    this->box->lcolor[0] = 0.4f;
    this->box->lcolor[1] = 0.4f;
    this->box->lcolor[2] = 0.4f;
    this->box->lcolor[3] = 1.0f;
    this->box->vtxcoords->w = 0.02f;
    this->box->vtxcoords->h = 0.075f;
    this->box->upvtxtoscr();
    this->box->tooltiptitle = "Loopstation row select current ";
    this->box->tooltip = "Leftclicking this box selects this loopstation row for use of R(record), L(loop play this row) and S(one shot play this row) keyboard shortcuts. ";
    this->scritch = new Param;
    this->scritch->box->lcolor[0] = 0.4f;
    this->scritch->box->lcolor[1] = 0.4f;
    this->scritch->box->lcolor[2] = 0.4f;
    this->scritch->box->lcolor[3] = 1.0f;
    this->scritch->box->vtxcoords->w = 0.15f;
    this->scritch->box->vtxcoords->h = 0.075f;
    this->scritch->box->upvtxtoscr();
    this->scritch->box->tooltiptitle = "Loopstation framecounter ";
    this->scritch->box->tooltip = "Leftclicking/dragging inside this box sets the position inside the loopstation recording. ";
 }

LoopStationElement::~LoopStationElement() {
	delete this->recbut;
	delete this->loopbut;
	delete this->playbut;
    delete this->box;
    delete this->colbox;
    delete this->speed;
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
	auto *elem = new LoopStationElement;
	this->elements.push_back(elem);
	elem->pos = this->elements.size() - 1;
	elem->lpst = this;
	// float values are colors for circles in boxes
	LoopStation::setbut(elem->recbut, 0.8f, 0.0f, 0.0f);
	LoopStation::setbut(elem->loopbut, 0.0f, 0.8f, 0.0f);
	LoopStation::setbut(elem->playbut, 0.0f, 0.0f, 0.8f);
    elem->colbox->acolor[0] = this->colvals[(elem->pos % 8) * 3];
    elem->colbox->acolor[1] = this->colvals[(elem->pos % 8) * 3 + 1];
    elem->colbox->acolor[2] = this->colvals[(elem->pos % 8) * 3 + 2];
    elem->colbox->acolor[3] = 1.0f;
	return elem;
}

void LoopStation::handle() {
    this->scrpos = mainprogram->handle_scrollboxes(*this->upscrbox, *this->downscrbox, this->elements.size(), this->scrpos, 8);
    int ce = std::clamp(this->currelem->pos, this->scrpos, this->scrpos + 7);
    this->currelem = this->elements[ce];
    this->foundrec = false;
    for (int i = 0; i < this->elements.size(); i++) {
		this->elements[i]->handle();
	}
    this->upscrbox->vtxcoords->x1 = this->elements[0]->colbox->vtxcoords->x1 + 0.0465f;
    this->downscrbox->vtxcoords->x1 = this->elements[0]->colbox->vtxcoords->x1 + 0.0465f;
    this->upscrbox->upvtxtoscr();
    this->downscrbox->upvtxtoscr();
    render_text("Loopstation", white, elements[0]->recbut->box->vtxcoords->x1 + 0.015f,
             elements[0]->recbut->box->vtxcoords->y1 + elements[0]->recbut->box->vtxcoords->h * 2.0f - 0.045f, 0.0005f, 0.0008f);
}

void LoopStationElement::handle() {
    if (this->pos >= this->lpst->scrpos && this->pos < this->lpst->scrpos + 8){
        this->mouse_handle();
        this->visualize();
        for (int i = 0; i < 2; i++) {
            std::vector<Layer *> &lvec = choose_layers(i);
            for (int j = 0; j < lvec.size(); j++) {
                std::vector<float> colvec;
                colvec.push_back(this->colbox->acolor[0]);
                colvec.push_back(this->colbox->acolor[1]);
                colvec.push_back(this->colbox->acolor[2]);
                colvec.push_back(this->colbox->acolor[3]);
                if (std::find(this->layers.begin(), this->layers.end(), lvec[j]) != this->layers.end()) {
                    lvec[j]->lpstcolors.emplace(colvec);
                }
                else {
                    lvec[j]->lpstcolors.erase(colvec);
                }
            }
        }
    }
	
	if ((this->loopbut->value || this->playbut->value) && !this->eventlist.empty()) this->set_values();
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
    float offdeck = 0.0f;
    if (mainmix->currlay[!mainprogram->prevmodus]) offdeck = !mainmix->currlay[!mainprogram->prevmodus]->deck;
	this->recbut->box->vtxcoords->x1 = -0.8f + 1.2f * offdeck;
	this->box->vtxcoords->x1 = this->recbut->box->vtxcoords->x1 - 0.02f;
	this->loopbut->box->vtxcoords->x1 = -0.8f + 1.2f * offdeck + this->loopbut->box->vtxcoords->w;
	this->playbut->box->vtxcoords->x1 = -0.8f + 1.2f * offdeck + this->playbut->box->vtxcoords->w * 2.0f;
	this->speed->box->vtxcoords->x1 = -0.8f + 1.2f * offdeck + this->recbut->box->vtxcoords->w * 3.0f;
    this->scritch->box->vtxcoords->x1 = -0.8f + 1.2f * offdeck + this->recbut->box->vtxcoords->w * 3.0f + this->speed->box->vtxcoords->w;
    this->colbox->vtxcoords->x1 = -0.8f + 1.2f * offdeck + this->recbut->box->vtxcoords->w * 3.0f + this->speed->box->vtxcoords->w * 2;
	this->recbut->box->vtxcoords->y1 = 0.4f - 0.075f * (float)(this->pos - this->lpst->scrpos);
	this->loopbut->box->vtxcoords->y1 = 0.4f - 0.075f * (float)(this->pos - this->lpst->scrpos);
	this->playbut->box->vtxcoords->y1 = 0.4f - 0.075f * (float)(this->pos - this->lpst->scrpos);
	this->speed->box->vtxcoords->y1 = 0.4f - 0.075f * (float)(this->pos - this->lpst->scrpos);
    this->scritch->box->vtxcoords->y1 = 0.4f - 0.075f * (float)(this->pos - this->lpst->scrpos);
    this->colbox->vtxcoords->y1 = 0.4f - 0.075f * (float)(this->pos - this->lpst->scrpos);
	this->box->vtxcoords->y1 = 0.4f - 0.075f * (float)(this->pos - this->lpst->scrpos);
	this->recbut->box->upvtxtoscr();
	this->loopbut->box->upvtxtoscr();
	this->playbut->box->upvtxtoscr();
	this->speed->box->upvtxtoscr();
	this->colbox->upvtxtoscr();
    this->box->upvtxtoscr();
    this->scritch->box->upvtxtoscr();
    this->speed->handle();
    draw_box(grey, this->colbox->acolor, this->colbox, -1);
	if (!this->eventlist.empty()) draw_box(black, black, this->colbox->vtxcoords->x1 + 0.02325f ,
                                      this->colbox->vtxcoords->y1 + 0.0375f, 0.0225f, 0.03f, -1);
	if (this == loopstation->currelem) draw_box(grey, white, this->box, -1);
	else draw_box(grey, nullptr, this->box, -1);
    draw_box(grey, green, this->scritch->box, -1);
    draw_box(white, white, this->scritch->box->vtxcoords->x1 + this->speedadaptedtime * (this->scritch->box->vtxcoords->w /
                                                                                    (this->totaltime)) - 0.002f,
             this->scritch->box->vtxcoords->y1, 0.004f, 0.075f, -1);
    render_text(std::to_string(this->pos + 1), white, this->recbut->box->vtxcoords->x1 - 0.05f, this->recbut->box->vtxcoords->y1 + 0.03f, 0.0006f, 0.001f);
	if (this->colbox->in()) {
        if (!mainprogram->menuondisplay || mainprogram->lpstmenuon) {
            if (mainprogram->menuactivation) {
                mainprogram->lpstmenu->state = 2;
                mainprogram->parammenu3->state = 0;
                mainprogram->parammenu4->state = 0;
                mainmix->mouselpstelem = this;
                mainprogram->menuactivation = false;
                mainprogram->lpstmenuon = true;            }
        }
	}
}
	
void LoopStationElement::erase_elem() {
	std::unordered_set<Param*>::iterator it1;
	for (it1 = this->params.begin(); it1 != this->params.end(); it1++) {
		Param* par = *it1;
		if (!par->name.empty()) {
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
    //current loopstation element selection
    if (this->box->in() && mainprogram->leftmouse) {
        loopstation->currelem = this;
    }

    if (this->eventlist.empty()) {
        this->playbut->value = false;
        this->loopbut->value = false;
    }

    if (this->recbut->value) {
        this->lpst->foundrec = true;
    }

    mainprogram->handle_button(this->recbut, true, false, true);
    if (this->recbut->toggled()) {
        if (mainprogram->adaptivelprow) {
            loopstation->currelem = this;
        }
        if (this->recbut->value) {
            this->loopbut->value = false;
            this->playbut->value = false;
            this->erase_elem();
            for (Param *par: this->lpst->allparams) {
                par->midistarttime = std::chrono::system_clock::from_time_t(0.0f);
            }
            for (Button *but: this->lpst->allbuttons) {
                but->midistarttime = std::chrono::system_clock::from_time_t(0.0f);
            }
            this->starttime = std::chrono::high_resolution_clock::now();
            mainprogram->recundo = false;
        } else {
            if (mainprogram->steplprow) {
                int rowpos = loopstation->currelem->pos;
                rowpos++;
                if (rowpos > 7) {
                    rowpos = 0;
                }
                loopstation->currelem = loopstation->elements[rowpos];
                mainprogram->waitonetime = true;
            }
            if (this->eventlist.size()) {
                std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> elapsed;
                elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - this->starttime);
                this->totaltime = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
                this->eventpos = 0;
                this->starttime = std::chrono::high_resolution_clock::now();
                this->interimtime = 0;
                this->speedadaptedtime = 0;
                for (Param *par: this->lpst->allparams) {
                    par->midistarttime = std::chrono::system_clock::from_time_t(0.0f);
                }
                for (Button *but: this->lpst->allbuttons) {
                    but->midistarttime = std::chrono::system_clock::from_time_t(0.0f);
                }
                // this->loopbut->value = true;
                //this->loopbut->oldvalue = false;
            }
        }
    }
    mainprogram->handle_button(this->loopbut, 1, 0);
    if (this->loopbut->toggled()) {
        // start/stop loop play of recording
        if (this->eventlist.size()) {
            if (mainprogram->adaptivelprow && !mainprogram->waitonetime) {
                loopstation->currelem = this;
            } else mainprogram->waitonetime = true;
            this->recbut->value = false;
            this->recbut->oldvalue = false;
            this->playbut->value = false;
            this->playbut->oldvalue = false;
            for (Param *par: this->lpst->allparams) {
                par->midistarttime = std::chrono::system_clock::from_time_t(0.0f);
            }
            for (Button *but: this->lpst->allbuttons) {
                but->midistarttime = std::chrono::system_clock::from_time_t(0.0f);
            }
            if (this->loopbut->value) {
                std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> elapsed;
                elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - this->starttime);
                this->totaltime = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
                this->eventpos = 0;
                this->starttime = std::chrono::high_resolution_clock::now();
                this->interimtime = 0;
                this->speedadaptedtime = 0;
            }
        } else {
            this->loopbut->value = false;
            this->loopbut->oldvalue = false;
        }
    }
    mainprogram->handle_button(this->playbut, 1, 0);
    if (this->playbut->toggled()) {
        // start/stop one-shot play of recording
        if (this->eventlist.size()) {
            if (mainprogram->adaptivelprow) loopstation->currelem = this;
            this->recbut->value = false;
            this->recbut->oldvalue = false;
            this->loopbut->value = false;
            this->loopbut->oldvalue = false;
            for (Param *par: this->lpst->allparams) {
                par->midistarttime = std::chrono::system_clock::from_time_t(0.0f);
            }
            for (Button *but: this->lpst->allbuttons) {
                but->midistarttime = std::chrono::system_clock::from_time_t(0.0f);
            }
            if (this->playbut->value) {
                this->eventpos = 0;
                this->starttime = std::chrono::high_resolution_clock::now();
                this->interimtime = 0;
                this->speedadaptedtime = 0;
            }
        } else {
            this->playbut->value = false;
            this->playbut->oldvalue = false;
        }
    }

    if (this->scritch->box->in()) {
        if (mainprogram->leftmousedown) {
            this->scritching = 1;
        }
        if (mainprogram->menuactivation) {
            mainprogram->parammenu3->state = 2;
            mainmix->learnparam = this->scritch;
            mainmix->learnbutton = nullptr;
            this->scritch->range[1] = this->totaltime;
            mainprogram->menuactivation = false;
        }
    }
    if (this->scritch->value != this->scritch->oldvalue) {
        this->speedadaptedtime = this->scritch->value;
        this->scritch->oldvalue = this->scritch->value;
        this->midiscritch = true;
    }
    if (this->scritching) mainprogram->leftmousedown = false;
    if (this->scritching == 1 || this->midiscritch) {
        if (!this->midiscritch) {
            this->speedadaptedtime = (this->totaltime) *
                                     ((mainprogram->mx - this->scritch->box->scrcoords->x1) /
                                      this->scritch->box->scrcoords->w);
        }
        if (this->speedadaptedtime > this->totaltime) {
            this->speedadaptedtime = this->totaltime;
        }
        if (this->speedadaptedtime < 0.0f) {
            this->speedadaptedtime = 0.0f;
        }
        std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
        this->starttime = now - std::chrono::milliseconds((long long) (this->speedadaptedtime));
        std::chrono::duration<double> elapsed;
        elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(mainprogram->now - this->starttime);
        long long millicount = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
        //int passed = millicount - this->interimtime;
        this->interimtime = millicount;
        this->eventpos = 0;
        auto event = this->eventlist[std::clamp(this->eventpos + 1, 0, (int)this->eventlist.size() - 1)];
        if (this->speedadaptedtime > std::get<0>(event)) {
            while (this->speedadaptedtime > std::get<0>(event)) {
                this->eventpos++;
                event = this->eventlist[std::clamp(this->eventpos + 1, 0, (int) this->eventlist.size() - 1)];
                if (std::get<1>(event)->name == "shiftx" || std::get<1>(event)->name == "shifty" || std::get<1>(event)->name == "wipex" || std::get<1>(event)->name == "wipey") {
                    event = this->eventlist[std::clamp(this->eventpos + 2, 0, (int) this->eventlist.size() - 1)];
                }
                if (this->eventpos >= this->eventlist.size()) {
                    this->eventpos = this->eventlist.size() - 1;
                    this->atend = true;
                    break;
                }
                else {
                    this->atend = false;
                }
            }
        }
        this->set_values();
        if (mainprogram->leftmouse && !mainprogram->menuondisplay) {
            this->scritching = 0;
            mainprogram->recundo = false;
            mainprogram->leftmouse = false;
        }
        this->midiscritch = false;
    }
}

void LoopStationElement::set_values() {
    if (this->eventlist.size() == 0) return;
	// if current elapsed time in loop > eventtime of events starting from eventpos then set their params to stored values
    std::chrono::system_clock::time_point now2 = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed;
	elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(mainprogram->now - this->starttime);
	long long millicount = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
	int passed = millicount - this->interimtime;
	this->interimtime = millicount;
	this->speedadaptedtime = this->speedadaptedtime + passed * this->speed->value;
	std::tuple<long long, Param*, Button*, float> event;
    /*if (this->eventpos > this->eventlist.size()) {
        this->eventpos = 1;
        return;
    }*/
	event = this->eventlist[std::clamp(this->eventpos, 0, (int)this->eventlist.size() - 1)];
	while (this->speedadaptedtime > std::get<0>(event) && !this->atend) {
	    // play all recorded events upto now
		Param *par = std::get<1>(event);
		Button *but = std::get<2>(event);
		if (par) {
            elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now2 - par->midistarttime);
            long long mc = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
            if (mc > 500 || mc < 0) {
                par->midistarted = false;
                if (par != mainmix->adaptparam) {
//                    if (std::find(this->lpst->allparams.begin(), this->lpst->allparams.end(), par) !=
//                        this->lpst->allparams.end()) {
                        par->value = std::get<3>(event);
//                   }
                }
            }
		}
		else if (but) {
            elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now2 - but->midistarttime);
            long long mc = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
            if (mc > 500 || mc < 0) {
                if (this->lpst->allbuttons.count(but)) {
                    but->value = (int) (std::get<3>(event) + 0.5f);
                }
            }
		}
		this->eventpos++;
		if (this->eventpos >= this->eventlist.size()) {
            this->atend = true;
		}
 		else {
            event = this->eventlist[this->eventpos];
        }
	}
    if (this->speedadaptedtime >= this->totaltime  && this->atend) {
        // reached end of loopstation element recording
        this->atend = false;
        if (this->eventlist.empty()) {
            // end loop when one-shot playing or no of the params exist anymore
            this->playbut->value = false;
            this->loopbut->value = false;
        }
        else {
            if (this->loopbut->value) {
                //start loop again
                this->eventpos = 0;
                this->speedadaptedtime = 0;
                this->interimtime = 0;
                this->starttime = mainprogram->now - std::chrono::milliseconds((long long)this->interimtime);
            }
            else if (this->playbut->value) {
                //end of single shot eventlist play
                this->playbut->value = false;
                this->playbut->oldvalue = true;
            }
        }
    }
    /*std::vector<Layer*> &lvec1 = choose_layers(0);
    for (Layer *lay : lvec1) {
        mainmix->copy_lpst(lay, lay, false, true);
    }
    std::vector<Layer*> &lvec2 = choose_layers(1);
    for (Layer *lay : lvec2) {
        mainmix->copy_lpst(lay, lay, false, true);
    }*/
}

void LoopStationElement::add_param_automationentry(Param* par) {
    this->add_param_automationentry(par, -1);
}

void LoopStationElement::add_param_automationentry(Param* par, long long mc) {
    //if (loopstation->parelemmap[par] != this && loopstation->parelemmap[par] != nullptr) return;  // each parameter can
    // be automated only once to avoid chaos
    if (par->name == "LPST speed") return;
	std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - this->starttime);
    long long millicount = mc;
    if (mc == -1) millicount = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    std::tuple<long long, Param*, Button*, float> event;
	event = std::make_tuple(millicount, par, nullptr, par->value);
	this->eventlist.push_back(event);
    this->params.emplace(par);
    this->lpst->allparams.emplace(par);
	loopstation->parelemmap[par] = this;
    if (par->name == "crossfade" || par->name == "crossfadecomp" || par->name == "wipex" || par->name == "wipey") {
    }
	else if (par->effect) {
		this->layers.emplace(par->effect->layer);
        par->layer = par->effect->layer;
	}
    else {
        this->layers.emplace(par->layer);
    }
	bool frame = false;
	for (int i = 0; i < 2; i++) {
		std::vector<Layer*>& lvec = choose_layers(i);
		for (auto & j : lvec) {
			if (par == j->speed) this->layers.emplace(j);
			if (par == j->opacity) this->layers.emplace(j);
            if (par == j->volume) this->layers.emplace(j);
            if (par == j->scritch) {
                this->layers.emplace(j);
                par->layer = j;
                frame = true;
            }
            if (par == j->startframe) {
                this->layers.emplace(j);
                par->layer = j;
                frame = true;
            }
            if (par == j->endframe) {
                this->layers.emplace(j);
                par->layer = j;
                frame = true;
            }
            if (par == j->blendnode->mixfac) this->layers.emplace(j);
			if (par == j->shiftx) {
				this->layers.emplace(j);
                par->layer = j;

				par->box->acolor[0] = this->colbox->acolor[0];
				par->box->acolor[1] = this->colbox->acolor[1];
				par->box->acolor[2] = this->colbox->acolor[2];

				par = j->shifty;
				this->add_param_automationentry(par);
				par = nullptr;
				return;
			}
			if (par == j->blendnode->wipex) {
				this->layers.emplace(j);
				par = j->blendnode->wipey;
                par->layer = j;
				this->add_param_automationentry(par);
				par = nullptr;
				return;
			}
			if (par == j->scale) {
				this->layers.emplace(j);
                par->layer = j;
				par = nullptr;
				return;
			}
		}
	}
 	if (par == mainmix->wipex[0]) {
		par = mainmix->wipey[0];
		this->add_param_automationentry(par);
		par = nullptr;
		return;
	}
	if (par == mainmix->wipex[1]) {
		par = mainmix->wipey[1];
		this->add_param_automationentry(par);
		par = nullptr;
		return;
	}

	if (!frame) {
        par->box->acolor[0] = this->colbox->acolor[0];
        par->box->acolor[1] = this->colbox->acolor[1];
        par->box->acolor[2] = this->colbox->acolor[2];
        par->box->acolor[3] = this->colbox->acolor[3];
    }
}

void LoopStationElement::add_button_automationentry(Button* but) {
    if (but->name[0] == "keepeffbut" || but->name[0] == "queuebut" || but->name[0] == "effcat") {
        return;
    }
    if (loopstation->butelemmap[but] != this && loopstation->butelemmap[but] != nullptr) return;  // each button can be
    // automated only once to avoid chaos
	std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - this->starttime);
	long long millicount = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
	std::tuple<long long, Param*, Button*, float> event;
	event = std::make_tuple(millicount, nullptr, but, (float)but->value);
	this->eventlist.push_back(event);
	this->buttons.emplace(but);
    this->lpst->allbuttons.emplace(but);
	loopstation->butelemmap[but] = this;
	but->box->acolor[0] = this->colbox->acolor[0];
	but->box->acolor[1] = this->colbox->acolor[1];
	but->box->acolor[2] = this->colbox->acolor[2];
	but->box->acolor[3] = this->colbox->acolor[3];
}


LoopStationElement* LoopStation::free_element() {
	LoopStationElement *loop = nullptr;
	for (auto elem : this->elements) {
        if (!elem->recbut->value && elem->eventlist.empty()) {
			loop = elem;
			break;
		}
	}
	return loop;
}


void LoopStation::remove_entries(int copycomp) {
    for (LoopStationElement *elem: this->elements) {
        std::vector<std::tuple<long long, Param *, Button *, float>> evlist = elem->eventlist;
        for (int i = evlist.size() - 1; i >= 0; i--) {
            std::tuple<long long, Param *, Button *, float> event = elem->eventlist[i];
            if (std::get<1>(event)) {
                if (std::get<1>(event)->name == "Crossfade" || std::get<1>(event)->name == "wipex" ||
                    std::get<1>(event)->name == "wipey") {
                    if (copycomp == 2) {
                        elem->eventlist.erase(elem->eventlist.begin() + i);
                        elem->params.erase(std::get<1>(event));
                    }
                } else if (std::get<1>(event)->effect) {
                    if (std::get<1>(event)->effect->layer->deck == mainmix->mousedeck) {
                        elem->eventlist.erase(elem->eventlist.begin() + i);
                        elem->params.erase(std::get<1>(event));
                    }
                } else {
                    if (std::get<1>(event)->layer->deck == mainmix->mousedeck) {
                        elem->eventlist.erase(elem->eventlist.begin() + i);
                        elem->params.erase(std::get<1>(event));
                    }
                }
            } else if (std::get<2>(event)) {
                if (std::get<2>(event)->layer->deck == mainmix->mousedeck) {
                    elem->eventlist.erase(elem->eventlist.begin() + i);
                    elem->buttons.erase(std::get<2>(event));
                }
            }
        }
        if (elem->eventlist.empty()) {
            elem->erase_elem();
        }
    }

}