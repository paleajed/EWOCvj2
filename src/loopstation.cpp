#ifndef USE_GLES
#include "GL/glew.h"
#include "GL/gl.h"
#define FREEGLUT_STATIC
#define FREEGLUT_LIB_PRAGMAS 0
#endif

#include <algorithm>
#include <set>

// my own header
#include "program.h"

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
    {
        std::lock_guard<std::mutex> lock(this->elements_mutex);
        for (LoopStationElement *elem : this->elements) {
            delete elem;
        }
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
	this->recbut->box->tooltip = "Start recording non-automated parameter movements on this loopstation row.  Rightclick for menu that allows clearing the line, copying and pasting loop length, beatmatch this line or do a MIDI learn. ";
	this->loopbut = new Button(0);
    this->loopbut->name[0] = "T";
	this->loopbut->toggle = 1;
    this->loopbut->box->lcolor[0] = 0.4f;
    this->loopbut->box->lcolor[1] = 0.4f;
    this->loopbut->box->lcolor[2] = 0.4f;
    this->loopbut->box->lcolor[3] = 1.0f;
	this->loopbut->box->tooltiptitle = "Loop play loopstation row ";
	this->loopbut->box->tooltip = "Start loop-playing of recorded parameter movements for this loopstation row. Stops recording if it was still running.  Rightclick for menu that allows clearing the line, copying and pasting loop length, beatmatch this line or do a MIDI learn. ";
	this->playbut = new Button(0);
    this->playbut->name[0] = "Y";
	this->playbut->toggle = 1;
    this->playbut->box->lcolor[0] = 0.4f;
    this->playbut->box->lcolor[1] = 0.4f;
    this->playbut->box->lcolor[2] = 0.4f;
    this->playbut->box->lcolor[3] = 1.0f;
	this->playbut->box->tooltiptitle = "One-off play loopstation row ";
	this->playbut->box->tooltip = "Start one-off-play of recorded parameter movements for this loopstation row. Stops at end of recording.  Rightclick for menu that allows clearing the line, copying and pasting loop length, beatmatch this line or do a MIDI learn. ";
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
	this->speed->box->tooltip = "Allows multiplying the recorded event's speed of this loopstation row.  Leftdrag sets value. Doubleclick allows numeric entry.  Rightclick for menu that allows clearing the line, copying and pasting loop length, beatmatch this line or do a MIDI learn. ";
	this->colbox = new Boxx;
    this->colbox->lcolor[0] = 0.4f;
    this->colbox->lcolor[1] = 0.4f;
    this->colbox->lcolor[2] = 0.4f;
    this->colbox->lcolor[3] = 1.0f;
	this->colbox->vtxcoords->w = 0.0465f;
	this->colbox->vtxcoords->h = 0.075f;
	this->colbox->upvtxtoscr();
	this->colbox->tooltiptitle = "Loopstation row color code ";
	this->colbox->tooltip = "Leftclicking this box shows colored boxes on the layer stack scroll strips for layers that contain parameters automated by this loopstation row. A small black box is drawn here when this loopstation row contains data.  Rightclick for menu that allows clearing the line, copying and pasting loop length, beatmatch this line or do a MIDI learn. ";
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
    this->scritch->box->tooltip = "Leftclicking/dragging inside this box sets the position inside the loopstation recording.  Rightclick for menu that allows clearing the line, copying and pasting loop length, beatmatch this line or do a MIDI learn. ";
 }

LoopStationElement::~LoopStationElement() {
	this->clear_curve();
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
	{
		std::lock_guard<std::mutex> lock(this->elements_mutex);
		this->elements.push_back(elem);
		elem->pos = this->elements.size() - 1;
	}
	elem->lpst = this;
	// float values are colors for circles in boxes
	LoopStation::setbut(elem->recbut, 0.8f, 0.0f, 0.0f);
	LoopStation::setbut(elem->loopbut, 0.0f, 0.8f, 0.0f);
	LoopStation::setbut(elem->playbut, 0.2f, 0.2f, 1.0f);
    elem->colbox->acolor[0] = this->colvals[(elem->pos % 8) * 3];
    elem->colbox->acolor[1] = this->colvals[(elem->pos % 8) * 3 + 1];
    elem->colbox->acolor[2] = this->colvals[(elem->pos % 8) * 3 + 2];
    elem->colbox->acolor[3] = 1.0f;
	return elem;
}

void LoopStation::handle() {
    if (!mainprogram->binsroom && !mainprogram->styleroom && !mainprogram->genroom && !mainprogram->segmentationroom) {
        this->scrpos = mainprogram->handle_scrollboxes(*this->upscrbox, *this->downscrbox, this->elements.size(), this->scrpos, 8);
    }
    int ce = this->currelem->pos;
    this->currelem = this->elements[ce];
    this->foundrec = false;
    LoopCurveEditor* curveeditor = lpcurveeditor();
    bool blockmouse = curveeditor->active && curveeditor->elem && curveeditor->elem->lpst == this;
    int bumx = mainprogram->mx, bumy = mainprogram->my;
    if (blockmouse) {
        // the curve editor covers the loopstation display: hide the mouse from the rows below it
        mainprogram->mx = -1000;
        mainprogram->my = -1000;
    }
    for (int i = 0; i < this->elements.size(); i++) {
		this->elements[i]->handle();
	}
    if (blockmouse) {
        mainprogram->mx = bumx;
        mainprogram->my = bumy;
    }
    this->upscrbox->vtxcoords->x1 = this->elements[0]->colbox->vtxcoords->x1 + 0.0465f;
    this->downscrbox->vtxcoords->x1 = this->elements[0]->colbox->vtxcoords->x1 + 0.0465f;
    this->upscrbox->upvtxtoscr();
    this->downscrbox->upvtxtoscr();
    if (!mainprogram->binsroom && !mainprogram->styleroom && !mainprogram->genroom && !mainprogram->segmentationroom) render_text("Loopstation", white, elements[0]->recbut->box->vtxcoords->x1 + 0.015f,
             elements[0]->recbut->box->vtxcoords->y1 + elements[0]->recbut->box->vtxcoords->h * 2.0f - 0.045f, 0.0005f, 0.0008f);
    if (curveeditor->active) {
        // the editor itself does get the real mouse while dragging
        int hmx = mainprogram->mx, hmy = mainprogram->my;
        bool hdown = mainprogram->leftmousedown, hleft = mainprogram->leftmouse;
        if (curveeditor->hid) {
            mainprogram->mx = curveeditor->rmx;
            mainprogram->my = curveeditor->rmy;
            mainprogram->leftmousedown = curveeditor->rdown;
            mainprogram->leftmouse = curveeditor->rleft;
        }
        curveeditor->handle();
        if (curveeditor->hid) {
            mainprogram->mx = hmx;
            mainprogram->my = hmy;
            mainprogram->leftmousedown = hdown;
            mainprogram->leftmouse = hleft;
        }
    }
}

void LoopStationElement::handle() {
    if (!mainprogram->binsroom && !mainprogram->styleroom && !mainprogram->genroom && !mainprogram->segmentationroom && this->pos >= this->lpst->scrpos && this->pos < this->lpst->scrpos + 8){
        this->visualize();
        this->mouse_handle();

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
    this->clear_curve();
	this->eventpos = 0;
    this->totaltime = 0.0f;
    this->interimtime = 0.0f;
    this->speedadaptedtime = 0.0f;
//    this->interimtime = 0.0f;
//    this->speedadaptedtime = 0.0f;
//    this->totaltime = 0.0f;
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
    mainprogram->beatthres->box->vtxcoords->x1 = -0.8f + 1.2f * offdeck + this->recbut->box->vtxcoords->w * 3.0f;
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
    if (this->beats == 0) {
        this->speed->handle();
    }
    else {
        draw_box(this->speed->box, -1);
        render_text(mainprogram->beatmenu->entries[log2(this->beats * 2.0f) + 1], white, this->speed->box->vtxcoords->x1 + 0.03f, this->speed->box->vtxcoords->y1 + 0.075f - 0.045f,
                    0.00045f, 0.00075f);
    }
    draw_box(grey, this->colbox->acolor, this->colbox, -1);
	if (!this->eventlist.empty()) draw_box(black, black, this->colbox->vtxcoords->x1 + 0.02325f ,
                                      this->colbox->vtxcoords->y1 + 0.0375f, 0.0225f, 0.03f, -1);
	if (this == loopstation->currelem) draw_box(grey, white, this->box, -1);
	else draw_box(grey, nullptr, this->box, -1);
    draw_box(grey, green, this->scritch->box, -1);
    if (this->curve) {
        if (this->eventlist.empty() || (this->params.empty() && this->buttons.empty())) {
            this->clear_curve();
        }
        else if (this->totaltime > 0.0f) {
            // lightblue rendering of the curve over the scrub box
            const Boxx* sb = this->scritch->box;
            float span = std::min(this->curve->totalsize * 1000.0f, this->totaltime);
            float rr = this->curve->rmax - this->curve->rmin;
            const int steps = 48;
            float lx = 0.0f, ly = 0.0f;
            for (int s = 0; s <= steps; s++) {
                float ms = span * (float) s / (float) steps;
                float v = this->curve->eval(ms / 1000.0f);
                float x = sb->vtxcoords->x1 + ms / this->totaltime * sb->vtxcoords->w;
                float y = sb->vtxcoords->y1 + 0.005f + (rr > 0.0f ? (v - this->curve->rmin) / rr : 0.0f) * (sb->vtxcoords->h - 0.01f);
                if (s > 0) register_line_draw(white, lx, ly, x, y);
                lx = x;
                ly = y;
            }
        }
    }
    draw_box(white, white, this->scritch->box->vtxcoords->x1 + this->speedadaptedtime * (this->scritch->box->vtxcoords->w /
                                                                                    (this->totaltime)) - 0.002f,
             this->scritch->box->vtxcoords->y1, 0.004f, 0.075f, -1);
    render_text(std::to_string(this->pos + 1), white, this->recbut->box->vtxcoords->x1 - 0.05f, this->recbut->box->vtxcoords->y1 + 0.03f, 0.0006f, 0.001f);
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
	    this->lpst->parelemmap.erase(par);
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

    auto step = []() {
        if (mainprogram->steplprow) {
            int rowpos = loopstation->currelem->pos;
            do
            {
                rowpos++;
            }
            while (loopstation->elements[rowpos]->eventlist.size());
            if (rowpos < 256) {
                loopstation->currelem = loopstation->elements[rowpos];
            }
        }
    };

    mainprogram->handle_button(this->recbut, true, false, true);
    if (this->recbut->toggled()) {
        loopstation->currelem = this;
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
            step();
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
            if (this->recbut->value) {
                std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> elapsed;
                elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - this->starttime);
                this->totaltime = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
                this->eventpos = 0;
                step();
            }
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
                //std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
                //std::chrono::duration<double> elapsed;
                //elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - this->starttime);
                //this->totaltime = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
                //this->eventpos = 0;
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
            loopstation->currelem = this;
            if (this->recbut->value) {
                std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> elapsed;
                elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - this->starttime);
                this->totaltime = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
                this->eventpos = 0;
                step();
            }
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
            if (this->scritching != 1) {
                // fresh scrub drag: jump to the clicked position once, then switch to SDL
                // relative mode for the rest of the drag - see Layer::handle_loopbox()'s scrub
                // gesture (mixer.cpp) for the full rationale
                SDL_SetWindowRelativeMouseMode(mainprogram->mainwindow, true);
                float discardX, discardY;
                SDL_GetRelativeMouseState(&discardX, &discardY);  // clear pre-drag accumulation
                this->speedadaptedtime = (this->totaltime) *
                                         ((mainprogram->mx - this->scritch->box->scrcoords->x1) /
                                          this->scritch->box->scrcoords->w);
                if (this->speedadaptedtime > this->totaltime) this->speedadaptedtime = this->totaltime;
                if (this->speedadaptedtime < 0.0f) this->speedadaptedtime = 0.0f;
            }
            this->scritching = 1;
        }
        if (mainprogram->menuactivation) {
            mainprogram->lpstmenu->state = 2;
            mainmix->mouselpstelem = this;
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
            float relX, relY;
            SDL_GetRelativeMouseState(&relX, &relY);
            this->speedadaptedtime += relX * this->totaltime / this->scritch->box->scrcoords->w;
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
                if (std::get<1>(event)) {
                    if (std::get<1>(event)->name == "shiftx" || std::get<1>(event)->name == "shifty" ||
                        std::get<1>(event)->name == "wipex" || std::get<1>(event)->name == "wipey") {
                        event = this->eventlist[std::clamp(this->eventpos + 2, 0, (int) this->eventlist.size() - 1)];
                    }
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
            SDL_SetWindowRelativeMouseMode(mainprogram->mainwindow, false);
        }
        this->midiscritch = false;
    }
    if (this->colbox->in() || this->recbut->box->in() || this->loopbut->box->in() || this->playbut->box->in() || this->speed->box->in() || this->scritch->box->in()) {
        if (!mainprogram->menuondisplay || mainprogram->lpstmenuon) {
            if (mainprogram->menuactivation) {
                mainprogram->lpstmenu->state = 2;
                mainprogram->parammenu3->state = 0;
                mainprogram->parammenu4->state = 0;
                if (this->colbox->in()) {
                    mainmix->learnbutton = nullptr;
                    mainmix->learnparam = nullptr;
                }
                else if (this->recbut->box->in()) {
                    mainmix->learnbutton = this->recbut;
                    mainmix->learnparam = nullptr;
                }
                else if (this->loopbut->box->in()) {
                    mainmix->learnbutton = this->loopbut;
                    mainmix->learnparam = nullptr;
                }
                else if (this->playbut->box->in()) {
                    mainmix->learnbutton = this->playbut;
                    mainmix->learnparam = nullptr;
                }
                else if (this->speed->box->in()) {
                    mainmix->learnbutton = nullptr;
                    mainmix->learnparam = this->speed;
                }
                else if (this->scritch->box->in()) {
                    mainmix->learnbutton = nullptr;
                    mainmix->learnparam = this->scritch;
                }
                mainmix->mouselpstelem = this;
                mainprogram->menuactivation = false;
                mainprogram->lpstmenuon = true;
            }
        }
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

    event = this->eventlist[std::clamp(this->eventpos, 0, (int)this->eventlist.size() - 1)];
    if (std::get<1>(event)) {
        std::get<1>(event)->box->acolor[0] = this->colbox->acolor[0];
        std::get<1>(event)->box->acolor[1] = this->colbox->acolor[1];
        std::get<1>(event)->box->acolor[2] = this->colbox->acolor[2];
        std::get<1>(event)->box->acolor[3] = this->colbox->acolor[3];
    }
	while (this->speedadaptedtime > std::get<0>(event) && !this->atend) {
	    // play all recorded events upto now
		Param *par = std::get<1>(event);
		Button *but = std::get<2>(event);
		if (par) {
            elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now2 - par->midistarttime);
            long long mc = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
            if (mc > 500 || mc < 0) {
                par->midistarted = false;
                if (par != mainmix->prepadaptparam && par != mainmix->adaptparam) {
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
                if (this->lpst->allbuttons.contains(but)) {
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
    if (this->speedadaptedtime >= this->totaltime) {
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
                this->speedadaptedtime -= this->totaltime;
                this->interimtime = (long long)this->speedadaptedtime;
                this->starttime = mainprogram->now - std::chrono::milliseconds((long long)this->speedadaptedtime);
            }
            else if (this->playbut->value) {
                //end of single shot eventlist play
                this->playbut->value = false;
                this->playbut->oldvalue = true;
            }
        }
    }
}

void LoopStationElement::add_param_automationentry(Param* par) {
    this->add_param_automationentry(par, -1);
}

void LoopStationElement::add_param_automationentry(Param* par, long long mc) {
    if (loopstation->parelemmap[par] != this && loopstation->parelemmap[par] != nullptr) {
        this->recbut->value = false;
        mainprogram->infostr = "The activated parameter has already been automated in the loopstation.";
        return;  // each parameter can be automated only once to avoid chaos
    }

    if (par->name == "LPST speed") return;
    if (par->type == ISFLoader::PARAM_COLOR || par->colslave) return;
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
    Layer *lay = nullptr;
    if (par->name == "Crossfade" || par->shadervar == "mixfac" || par->name == "wipex" || par->name == "wipey" || par->name == "wipexlay" || par->name == "wipeylay") {
    }
	else if (par->effect) {
        lay = par->effect->layer;
		this->layers.emplace(lay);
        par->layer = par->effect->layer;
	}
    else if (par->layer){
        lay = par->layer;
        this->layers.emplace(lay);
    }

    for (int i = 0; i < 2; i++) {
        std::vector<Layer*>& lvec = choose_layers(i);
        for (auto & j : lvec) {
            if (par == j->speed) this->layers.emplace(j);
            if (par == j->opacity) this->layers.emplace(j);
            if (par == j->volume) this->layers.emplace(j);
            if (par == j->scritch) {
                this->layers.emplace(j);
                par->layer = j;
            }
            if (par == j->startframe) {
                this->layers.emplace(j);
                par->layer = j;
            }
            if (par == j->endframe) {
                this->layers.emplace(j);
                par->layer = j;
            }
            if (par == j->blendnode->mixfac || par == j->blendnode->wipex || par == j->blendnode->wipey) {
                this->layers.emplace(j);
                par->layer = j;
            }
            if (par == j->shiftx) {
                this->layers.emplace(j);
                par->layer = j;

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
}

void LoopStationElement::add_button_automationentry(Button* but) {
    if (but->name[0] == "keepeffbut" || but->name[0] == "keepmaskbut" || but->name[0] == "queuebut" || but->name[0] == "effcat") {
        return;
    }
    if (loopstation->butelemmap[but] != this && loopstation->butelemmap[but] != nullptr) {
        this->recbut->value = false;
        mainprogram->infostr = "The activated parameter has already been automated in the loopstation.";
        return;  // each button can be automated only once to avoid chaos
    }
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


void LoopStation::remove_entries(int copycomp, bool deck) {
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
                } else if (std::get<1>(event)->name == "Speed A") {
                    if (deck == 0) {
                        elem->eventlist.erase(elem->eventlist.begin() + i);
                        elem->params.erase(std::get<1>(event));
                    }
                } else if (std::get<1>(event)->name == "Speed B") {
                    if (deck == 1) {
                        elem->eventlist.erase(elem->eventlist.begin() + i);
                        elem->params.erase(std::get<1>(event));
                    }
                } else if (std::get<1>(event)->effect) {
                    if (std::get<1>(event)->effect->layer->deck == mainmix->mousedeck) {
                        elem->eventlist.erase(elem->eventlist.begin() + i);
                        elem->params.erase(std::get<1>(event));
                        elem->layers.erase(std::get<1>(event)->effect->layer);
                    }
                } else {
                    if (std::get<1>(event)->layer->deck == mainmix->mousedeck) {
                        elem->eventlist.erase(elem->eventlist.begin() + i);
                        elem->params.erase(std::get<1>(event));
                        elem->layers.erase(std::get<1>(event)->layer);
                    }
                }
            } else if (std::get<2>(event)) {
                if (std::get<2>(event)->layer->deck == mainmix->mousedeck) {
                    elem->eventlist.erase(elem->eventlist.begin() + i);
                    elem->buttons.erase(std::get<2>(event));
                    elem->layers.erase(std::get<2>(event)->layer);
                }
            }
        }
        if (elem->eventlist.empty()) {
            elem->erase_elem();
        }
    }
}


void LoopStationElement::get_state_from(LoopStationElement* loop) {
    // copy loopstation element state
    if (this->eventlist.size()) {
        this->interimtime = loop->interimtime;
        this->speedadaptedtime = loop->speedadaptedtime;
        this->totaltime = loop->totaltime;
        this->starttime = loop->starttime;
        this->eventpos = loop->eventpos;
        this->atend = loop->atend;
        this->speed->value = loop->speed->value;
        this->beats = loop->beats;
        if (loop->curve && loop != this) {
            // the curve travels with the row; its Param is re-adopted by curve_param()
            delete this->curve;
            this->curve = new LoopCurve(*loop->curve);
            this->curvepar = nullptr;
            this->curvebut = nullptr;
        }
        this->loopbut->value = loop->loopbut->value;
        this->playbut->value = loop->playbut->value;
    }
}


void Param::lpst_replace_with(Param *cpar) {
    LoopStationElement *lpe = loopstation->parelemmap.contains(this) ?
                              loopstation->parelemmap[this] : nullptr;
    if (lpe) {
        for (int i = 0; i < lpe->eventlist.size(); i++) {
            if (std::get<1>(lpe->eventlist[i]) == this) {
                std::get<1>(lpe->eventlist[i]) = cpar;
            }
        }
        cpar->box->acolor[0] = lpe->colbox->acolor[0];
        cpar->box->acolor[1] = lpe->colbox->acolor[1];
        cpar->box->acolor[2] = lpe->colbox->acolor[2];
        cpar->box->acolor[3] = lpe->colbox->acolor[3];
        loopstation->parelemmap[cpar] = lpe;
        loopstation->parelemmap.erase(this);
        lpe->params.erase(this);
        lpe->params.emplace(cpar);
        lpe->params.erase(this);
        if (lpe->curvepar == this) lpe->curvepar = cpar;
        if (this->effect) {
            lpe->layers.emplace(cpar->effect->layer);
            lpe->layers.erase(this->effect->layer);
        }
        else {
            lpe->layers.emplace(cpar->layer);
            lpe->layers.erase(this->layer);
        }
        loopstation->parmap[this] = loopstation->parmap[cpar];
        loopstation->allparams.erase(this);
        loopstation->allparams.emplace(cpar);
    }
    cpar->value = this->value;
    if (this->type == ISFLoader::PARAM_COLOR) {
        memcpy(cpar->colvalue, this->colvalue, sizeof(this->colvalue));
    }
    cpar->midi[0] = this->midi[0];
    cpar->midi[1] = this->midi[1];
    cpar->register_midi();
}

void Button::lpst_replace_with(Button *cbut) {
    LoopStationElement *lpe = loopstation->butelemmap[this];
    if (lpe) {
        for (int i = 0; i < lpe->eventlist.size(); i++) {
            if (std::get<2>(lpe->eventlist[i]) == this) {
                std::get<2>(lpe->eventlist[i]) = cbut;
            }
        }
        loopstation->butelemmap[cbut] = lpe;
        loopstation->butelemmap.erase(this);
        lpe->buttons.erase(this);
        lpe->buttons.emplace(cbut);
        lpe->buttons.erase(this);
        if (lpe->curvebut == this) lpe->curvebut = cbut;
        lpe->layers.emplace(cbut->layer);
        lpe->layers.erase(this->layer);
        loopstation->butmap[this] = loopstation->butmap[cbut];
        loopstation->allbuttons.erase(this);
        loopstation->allbuttons.emplace(cbut);
    }
    cbut->value = this->value;
    cbut->midi[0] = this->midi[0];
    cbut->midi[1] = this->midi[1];
    cbut->register_midi();
}


										// LOOPSTATION CURVES

void LoopCurve::init_default(float rangemin, float rangemax) {
    this->rmin = rangemin;
    this->rmax = rangemax;
    this->knots.clear();
    CurveKnot a;
    a.x = 0.0f;
    a.y = rangemin;
    CurveKnot b;
    b.x = this->totalsize;
    b.y = rangemax;
    this->knots.push_back(a);
    this->knots.push_back(b);
}

void LoopCurve::normalize() {
    if (this->knots.size() < 2) {
        this->init_default(this->rmin, this->rmax);
    }
    std::stable_sort(this->knots.begin(), this->knots.end(), [](const CurveKnot& a, const CurveKnot& b) { return a.x < b.x; });
    int n = this->knots.size();
    this->knots.front().x = 0.0f;
    this->knots.back().x = this->totalsize;
    for (int i = 0; i < n; i++) {
        CurveKnot& k = this->knots[i];
        k.x = std::clamp(k.x, 0.0f, this->totalsize);
        k.y = std::clamp(k.y, this->rmin, this->rmax);
    }
    for (int i = 0; i < n; i++) {
        CurveKnot& k = this->knots[i];
        if (i == 0) k.hinx = k.hiny = 0.0f;
        if (i == n - 1) k.houtx = k.houty = 0.0f;
        if (k.type == 0) {
            k.hinx = k.hiny = k.houtx = k.houty = 0.0f;
            continue;
        }
        if (i + 1 < n) {
            // only a sanity cap: a handle may reach past the neighbour or the plot edge (then it is shown off-editor),
            // eval() keeps the segment monotonic in time
            float nx = 4.0f * this->totalsize;
            if (k.houtx < 0.0f) k.houtx = 0.0f;
            if (k.houtx > nx) {
                float s = nx / k.houtx;
                k.houtx *= s;
                k.houty *= s;
            }
        }
        if (i > 0) {
            float px = 4.0f * this->totalsize;
            if (k.hinx > 0.0f) k.hinx = 0.0f;
            if (-k.hinx > px) {
                float s = px / -k.hinx;
                k.hinx *= s;
                k.hiny *= s;
            }
        }
    }
}

void LoopCurve::set_type(int knotnr, int type) {
    if (knotnr < 0 || knotnr >= (int)this->knots.size()) return;
    int n = this->knots.size();
    CurveKnot& k = this->knots[knotnr];
    k.type = type;
    if (type != 0) {
        float nextspan = knotnr + 1 < n ? this->knots[knotnr + 1].x - k.x : 0.0f;
        float prevspan = knotnr > 0 ? k.x - this->knots[knotnr - 1].x : 0.0f;
        if (k.houtx == 0.0f && k.houty == 0.0f) k.houtx = nextspan / 3.0f;
        if (k.hinx == 0.0f && k.hiny == 0.0f) k.hinx = -prevspan / 3.0f;
        if (type == 1) {
            // fluid: both ends on one line
            if (knotnr > 0 && knotnr + 1 < n) {
                k.hinx = -k.houtx;
                k.hiny = -k.houty;
            }
        }
    }
    this->normalize();
}

void LoopCurve::set_totalsize(float ts) {
    if (ts <= 0.0f || this->totalsize <= 0.0f) return;
    float s = ts / this->totalsize;
    for (auto& k : this->knots) {
        k.x *= s;
        k.hinx *= s;
        k.houtx *= s;
    }
    this->totalsize = ts;
    this->normalize();
}

float LoopCurve::eval(float t) const {
    int n = this->knots.size();
    if (n == 0) return this->rmin;
    if (n == 1) return std::clamp(this->knots[0].y, this->rmin, this->rmax);
    t = std::clamp(t, 0.0f, this->totalsize);
    int i = 0;
    while (i < n - 2 && t > this->knots[i + 1].x) i++;
    const CurveKnot& a = this->knots[i];
    const CurveKnot& b = this->knots[i + 1];
    float span = b.x - a.x;
    if (span <= 1e-6f) return std::clamp(b.y, this->rmin, this->rmax);
    float ox = a.houtx, oy = a.houty, ix = b.hinx, iy = b.hiny;
    float sum = ox - ix;
    if (sum > span) {
        // keep x monotonic over the segment so the curve stays a function of time
        float s = span / sum;
        ox *= s; oy *= s; ix *= s; iy *= s;
    }
    float x0 = a.x, x1 = a.x + ox, x2 = b.x + ix, x3 = b.x;
    float y0 = a.y, y1 = a.y + oy, y2 = b.y + iy, y3 = b.y;
    float lo = 0.0f, hi = 1.0f;
    for (int it = 0; it < 30; it++) {
        float u = (lo + hi) * 0.5f;
        float m = 1.0f - u;
        float bx = m * m * m * x0 + 3.0f * m * m * u * x1 + 3.0f * m * u * u * x2 + u * u * u * x3;
        if (bx < t) lo = u; else hi = u;
    }
    float u = (lo + hi) * 0.5f;
    float m = 1.0f - u;
    float by = m * m * m * y0 + 3.0f * m * m * u * y1 + 3.0f * m * u * u * y2 + u * u * u * y3;
    return std::clamp(by, this->rmin, this->rmax);
}

void LoopCurve::sample(std::vector<std::pair<long long, float>>& out) const {
    out.clear();
    long long total = (long long)(this->totalsize * 1000.0f + 0.5f);
    long long step = std::max(10LL, total / 6000LL);
    for (long long ms = 0; ms < total; ms += step) {
        out.push_back(std::make_pair(ms, this->eval((float)ms / 1000.0f)));
    }
    out.push_back(std::make_pair(total, this->eval(this->totalsize)));
}


void LoopStationElement::clear_curve() {
    delete this->curve;
    this->curve = nullptr;
    this->curvepar = nullptr;
    this->curvebut = nullptr;
}

Button* LoopStationElement::curve_button() {
    if (!this->curve) return nullptr;
    if (this->curvebut && this->buttons.contains(this->curvebut)) return this->curvebut;
    // the Button object was replaced: a button curve row automates just that one Button
    if (!this->params.empty() || this->buttons.size() != 1) return nullptr;
    this->curvebut = *this->buttons.begin();
    return this->curvebut;
}

bool LoopStationElement::curve_is_for(Param* par, Button* but) {
    // a curve line steers all its elements with the same curve
    if (!this->curve) return false;
    return this->automates(par, but);
}

bool LoopStationElement::automates(Param* par, Button* but) {
    if (par) return this->params.contains(par);
    return but && this->buttons.contains(but);
}

Param* LoopStationElement::curve_param() {
    if (!this->curve) return nullptr;
    if (this->curvepar && this->params.contains(this->curvepar)) return this->curvepar;
    // the Param object was replaced (effect / source reloaded): a curve row automates just that one Param
    Param* found = nullptr;
    for (Param* p : this->params) {
        if (found) return nullptr;
        found = p;
    }
    this->curvepar = found;
    return found;
}

void LoopStationElement::apply_curve(Param* par, Button* but, const LoopCurve& lc) {
    std::vector<Param*> pars;
    std::vector<Button*> buts;
    if (par) pars.push_back(par);
    if (but) buts.push_back(but);
    this->apply_curve(pars, buts, lc);
}

void LoopStationElement::apply_curve(const std::vector<Param*>& allpars, const std::vector<Button*>& allbuts, const LoopCurve& lc) {
    // targets the loopstation can not automate are left out; when nothing is left the line is not touched
    std::vector<Param*> pars;
    std::vector<Button*> buts;
    for (Param* p : allpars) {
        if (p && p->name != "LPST speed" && p->type != ISFLoader::PARAM_COLOR && !p->colslave) pars.push_back(p);
    }
    for (Button* b : allbuts) {
        if (button_curvable(b)) buts.push_back(b);
    }
    if (pars.empty() && buts.empty()) {
        mainprogram->infostr = "This parameter can not be automated by the loopstation.";
        return;
    }
    // when the row is already running, keep it running through the re-apply
    bool wasrunning = (this->loopbut->value || this->playbut->value);
    int bulb = this->loopbut->value, bupb = this->playbut->value;
    float buspd = this->speed->value;
    float buspadt = this->speedadaptedtime;
    std::chrono::high_resolution_clock::time_point bustart = this->starttime;
    float buinter = this->interimtime;
    // overwriting a recorded line: the Params / Buttons it automated that are not part of the curve are released
    for (Param* p : this->params) {
        if (std::find(pars.begin(), pars.end(), p) == pars.end()) loopstation->parelemmap.erase(p);
    }
    for (Button* b : this->buttons) {
        if (std::find(buts.begin(), buts.end(), b) == buts.end()) loopstation->butelemmap.erase(b);
    }
    this->erase_elem();
    // stale map entries (a line that no longer automates the Param / Button) must not block it
    for (Param* p : pars) {
        auto it = loopstation->parelemmap.find(p);
        if (it != loopstation->parelemmap.end() && it->second != this && (!it->second || !it->second->params.contains(p))) {
            loopstation->parelemmap.erase(it);
        }
    }
    for (Button* b : buts) {
        auto it = loopstation->butelemmap.find(b);
        if (it != loopstation->butelemmap.end() && it->second != this && (!it->second || !it->second->buttons.contains(b))) {
            loopstation->butelemmap.erase(it);
        }
    }
    for (Param* p : pars) this->add_param_automationentry(p, 0);
    for (Button* b : buts) this->add_button_automationentry(b);
    // targets that were refused (already automated elsewhere...) drop out
    pars.erase(std::remove_if(pars.begin(), pars.end(), [this](Param* p) { return !this->params.contains(p); }), pars.end());
    buts.erase(std::remove_if(buts.begin(), buts.end(), [this](Button* b) { return !this->buttons.contains(b); }), buts.end());
    if (pars.empty() && buts.empty()) {
        return;
    }
    // replace the placeholder events of these Params / Buttons with the sampled curve
    for (int i = this->eventlist.size() - 1; i >= 0; i--) {
        Param* ep = std::get<1>(this->eventlist[i]);
        Button* eb = std::get<2>(this->eventlist[i]);
        if ((ep && std::find(pars.begin(), pars.end(), ep) != pars.end()) || (eb && std::find(buts.begin(), buts.end(), eb) != buts.end())) {
            this->eventlist.erase(this->eventlist.begin() + i);
        }
    }
    std::vector<std::pair<long long, float>> samples;
    lc.sample(samples);
    float lcr = lc.rmax - lc.rmin;
    // curve position 0 to 1 (the curve is in the units of its own range)
    auto norm = [&](float v) { return lcr > 0.0f ? std::clamp((v - lc.rmin) / lcr, 0.0f, 1.0f) : 0.0f; };
    for (Param* p : pars) {
        for (auto& s : samples) {
            // a Param follows the curve scaled to its own range
            float v = p->range[0] + norm(s.second) * (p->range[1] - p->range[0]);
            this->eventlist.push_back(std::make_tuple(s.first, p, (Button*)nullptr, v));
        }
    }
    for (Button* b : buts) {
        // a Button is on for the top half of the curve, off for the bottom half; events only where the state changes
        int laststate = -1;
        for (auto& s : samples) {
            int state = norm(s.second) >= 0.5f ? 1 : 0;
            if (state != laststate || &s == &samples.back()) {
                this->eventlist.push_back(std::make_tuple(s.first, (Param*)nullptr, b, (float)state));
                laststate = state;
            }
        }
    }
    std::stable_sort(this->eventlist.begin(), this->eventlist.end(), [](const auto& a, const auto& b) {
        return std::get<0>(a) < std::get<0>(b);
    });
    this->totaltime = (float)samples.back().first;
    this->interimtime = 0.0f;
    this->speedadaptedtime = 0.0f;
    this->eventpos = 0;
    this->atend = false;
    if (wasrunning) {
        this->loopbut->value = bulb;
        this->loopbut->oldvalue = bulb;
        this->playbut->value = bupb;
        this->playbut->oldvalue = bupb;
        this->speed->value = buspd;
        this->starttime = bustart;
        this->interimtime = buinter;
        this->speedadaptedtime = std::min(buspadt, this->totaltime);
        // continue from the current playhead position
        int pos = 0;
        while (pos < (int)this->eventlist.size() && std::get<0>(this->eventlist[pos]) < this->speedadaptedtime) pos++;
        this->eventpos = pos;
        // drive the Params / Buttons right away from the new curve at the current playhead
        float nv = norm(lc.eval(this->speedadaptedtime / 1000.0f));
        for (Param* p : pars) p->value = p->range[0] + nv * (p->range[1] - p->range[0]);
        for (Button* b : buts) b->value = nv >= 0.5f ? 1 : 0;
    }
    this->curve = new LoopCurve(lc);
    this->curvepar = (pars.size() == 1 && buts.empty()) ? pars[0] : nullptr;
    this->curvebut = (buts.size() == 1 && pars.empty()) ? buts[0] : nullptr;
    for (Param* p : pars) {
        p->box->acolor[0] = this->colbox->acolor[0];
        p->box->acolor[1] = this->colbox->acolor[1];
        p->box->acolor[2] = this->colbox->acolor[2];
        p->box->acolor[3] = this->colbox->acolor[3];
    }
    for (Button* b : buts) {
        b->box->acolor[0] = this->colbox->acolor[0];
        b->box->acolor[1] = this->colbox->acolor[1];
        b->box->acolor[2] = this->colbox->acolor[2];
        b->box->acolor[3] = this->colbox->acolor[3];
    }
    loopstation->currelem = this;
}


LoopCurve lpcurveclip;
bool lpcurveclipvalid = false;

bool lpst_curve_target(LoopStationElement* e, Param*& par, Button*& but) {
    par = nullptr;
    but = nullptr;
    if (!e || e->eventlist.empty()) return false;
    if (e->curve) {
        par = e->curve_param();
        if (!par) but = e->curve_button();
        return par || but;
    }
    if (e->params.size() == 1 && e->buttons.empty()) {
        par = *e->params.begin();
        return true;
    }
    if (e->buttons.size() == 1 && e->params.empty()) {
        but = *e->buttons.begin();
        return button_curvable(but);
    }
    return false;
}

bool button_curvable(Button* but) {
    if (!but) return false;
    if (but->name[0] == "keepeffbut" || but->name[0] == "keepmaskbut" || but->name[0] == "queuebut" || but->name[0] == "effcat") {
        return false;
    }
    for (LoopStationElement* el : loopstation->elements) {
        if (but == el->recbut || but == el->loopbut || but == el->playbut) return false;
    }
    return true;
}

static LoopStationElement* curve_owner(Param* par, Button* but) {
    // the loopstation line automating this Param / Button, if any
    if (par) {
        auto it = loopstation->parelemmap.find(par);
        return it != loopstation->parelemmap.end() ? it->second : nullptr;
    }
    if (but) {
        auto it = loopstation->butelemmap.find(but);
        return it != loopstation->butelemmap.end() ? it->second : nullptr;
    }
    return nullptr;
}

bool target_has_curve(Param* par, Button* but) {
    LoopStationElement* owner = curve_owner(par, but);
    return owner && owner->curve_is_for(par, but);
}

void target_copy_curve(Param* par, Button* but) {
    if (!target_has_curve(par, but)) return;
    lpcurveclip = *curve_owner(par, but)->curve;
    lpcurveclipvalid = true;
}

void target_paste_curve(Param* par, Button* but) {
    if ((!par && !but) || !lpcurveclipvalid) return;
    // scale the copied curve to the range of this Param (a Button curve has the range 0 to 1)
    float newmin = par ? par->range[0] : 0.0f;
    float newmax = par ? par->range[1] : 1.0f;
    LoopCurve nc = lpcurveclip;
    float oldr = lpcurveclip.rmax - lpcurveclip.rmin;
    float newr = newmax - newmin;
    float s = oldr > 0.0f ? newr / oldr : 1.0f;
    for (auto &kn : nc.knots) {
        kn.y = newmin + (kn.y - lpcurveclip.rmin) * s;
        kn.hiny *= s;
        kn.houty *= s;
    }
    nc.rmin = newmin;
    nc.rmax = newmax;
    nc.normalize();

    LoopStation* ls = loopstation;
    LoopStationElement* e = nullptr;
    LoopStationElement* owner = curve_owner(par, but);
    if (owner) {
        if (owner->curve && owner->params.size() + owner->buttons.size() > 1) {
            // the line steers several elements with one curve: they all get the pasted curve
            lpst_paste_curve(owner);
            return;
        }
        // a curve line is updated, a recorded line is overwritten
        e = owner;
    }
    else {
        e = ls->currelem;
        if (!e || e->recbut->value || !e->eventlist.empty()) {
            e = ls->free_element();
        }
        if (!e) {
            mainprogram->infostr = "No free loopstation line available.";
            return;
        }
    }
    e->apply_curve(par, but, nc);
}

bool lpst_has_targets(LoopStationElement* e) {
    return e && !e->eventlist.empty() && (!e->params.empty() || !e->buttons.empty());
}

void lpst_paste_curve(LoopStationElement* e) {
    if (!lpst_has_targets(e) || !lpcurveclipvalid) return;
    std::vector<Param*> pars(e->params.begin(), e->params.end());
    std::vector<Button*> buts(e->buttons.begin(), e->buttons.end());
    // one target: the curve is scaled to its range, several targets: the curve is normalized to 0 - 1
    // and every Param follows it scaled to its own range, every Button is on in the top half
    float newmin = 0.0f, newmax = 1.0f;
    if (pars.size() == 1 && buts.empty()) {
        newmin = pars[0]->range[0];
        newmax = pars[0]->range[1];
    }
    LoopCurve nc = lpcurveclip;
    float oldr = lpcurveclip.rmax - lpcurveclip.rmin;
    float s = oldr > 0.0f ? (newmax - newmin) / oldr : 1.0f;
    for (auto &kn : nc.knots) {
        kn.y = newmin + (kn.y - lpcurveclip.rmin) * s;
        kn.hiny *= s;
        kn.houty *= s;
    }
    nc.rmin = newmin;
    nc.rmax = newmax;
    nc.normalize();
    e->apply_curve(pars, buts, nc);
}

LoopCurveEditor* lpcurveeditor() {
    static LoopCurveEditor* editor = new LoopCurveEditor;
    return editor;
}

LoopCurveEditor::LoopCurveEditor() {
}

void LoopCurveEditor::init_widgets() {
    if (this->plot) return;
    this->plot = new Boxx;
    this->area = new Boxx;
    auto mkparam = [](const std::string& name, float val, float lo, float hi, float w) {
        Param* p = new Param;
        p->name = name;
        p->value = val;
        p->deflt = val;
        p->range[0] = lo;
        p->range[1] = hi;
        p->sliding = true;
        p->box->vtxcoords->w = w;
        p->box->vtxcoords->h = 0.075f;
        p->box->lcolor[0] = 0.4f; p->box->lcolor[1] = 0.4f; p->box->lcolor[2] = 0.4f; p->box->lcolor[3] = 1.0f;
        return p;
    };
    this->totalsize = mkparam("Total (s)", 1.0f, 0.1f, 60.0f, 0.15f);
    this->knotx = mkparam("Knot X", 0.0f, 0.0f, 1.0f, 0.15f);
    this->knoty = mkparam("Knot Y", 0.0f, 0.0f, 1.0f, 0.15f);
    this->totalsize->box->tooltiptitle = "Curve total length ";
    this->totalsize->box->tooltip = "Sets the length of the curve in seconds. Leftdrag sets value. Doubleclick allows numeric entry. ";
    this->knotx->box->tooltiptitle = "Selected knot X ";
    this->knotx->box->tooltip = "Sets the time position (seconds) of the selected knot. ";
    this->knoty->box->tooltiptitle = "Selected knot Y ";
    this->knoty->box->tooltip = "Sets the value of the selected knot. ";
    const char* names[3] = {"CONSTANT", "FLUID", "BROKEN"};
    const char* tips[3] = {"Knots of this type have straight lines protruding from them. ",
                           "Knots of this type have a single bezier handle line through the knot, giving a fluid transition. ",
                           "Knots of this type have two separately editable bezier handles, giving a sudden transition. "};
    for (int i = 0; i < 3; i++) {
        this->typebut[i] = new Button(i == 0);
        this->typebut[i]->name[0] = names[i];
        this->typebut[i]->box->vtxcoords->w = 0.112f;
        this->typebut[i]->box->vtxcoords->h = 0.075f;
        this->typebut[i]->box->tooltiptitle = std::string(names[i]) + " knot type ";
        this->typebut[i]->box->tooltip = std::string(tips[i]) + "Clicking sets the type for new knots and for the selected knot. ";
    }
    this->applybut = new Button(0);
    this->applybut->name[0] = "APPLY";
    this->loopsw = new Boxx;
    this->loopsw->vtxcoords->w = 0.0465f;
    this->loopsw->vtxcoords->h = 0.075f;
    this->loopsw->lcolor[0] = 0.4f; this->loopsw->lcolor[1] = 0.4f; this->loopsw->lcolor[2] = 0.4f; this->loopsw->lcolor[3] = 1.0f;
    this->loopsw->tooltiptitle = "Loop play this curve ";
    this->loopsw->tooltip = "Switches loop play of this loopstation row, so the curve can be tested. The setting stays after APPLY, and is restored when the editor is cancelled. ";
    this->applybut->box->vtxcoords->w = 0.1395f;
    this->applybut->box->vtxcoords->h = 0.075f;
    this->applybut->box->tooltiptitle = "Apply curve ";
    this->applybut->box->tooltip = "Fills the selected loopstation line with this curve. Play or loop the line to drive the parameter. ";
}

void LoopCurveEditor::open(Param* par, Button* but) {
    if (!par && !but) return;
    LoopStation* ls = loopstation;
    LoopStationElement* e = nullptr;
    LoopStationElement* owner = curve_owner(par, but);
    if (owner && !owner->automates(par, but)) owner = nullptr;  // stale map entry
    if (owner && !owner->eventlist.empty()) {
        // the Param / Button already has a line (curve or recorded): the editor takes that whole line,
        // a recorded line becomes a curve line steering all its elements
        this->open_row(owner);
        return;
    }
    // a Button curve has the range 0 to 1: on for the top half, off for the bottom half
    const float rmin = par ? par->range[0] : 0.0f;
    const float rmax = par ? par->range[1] : 1.0f;
    if (owner) {
        // the line of a curve is edited, a recorded line is overwritten when the curve is applied
        e = owner;
    }
    else {
        e = ls->currelem;
        if (!e || e->recbut->value || !e->eventlist.empty()) {
            e = ls->free_element();
        }
        if (!e) {
            mainprogram->infostr = "No free loopstation line available.";
            return;
        }
    }
    this->tpars.clear();
    this->tbuts.clear();
    if (par) this->tpars.push_back(par);
    if (but) this->tbuts.push_back(but);
    // a recorded line is backed up, so cancelling after testing can bring it back
    this->begin(e, owner != nullptr, rmin, rmax);
}

void LoopCurveEditor::open_row(LoopStationElement* e) {
    // edit the curve of a whole line; for a recorded line this turns it into a curve line steering all its elements
    if (!e || e->eventlist.empty()) return;
    this->tpars.clear();
    this->tbuts.clear();
    for (Param* p : e->params) {
        if (p->name != "LPST speed" && p->type != ISFLoader::PARAM_COLOR && !p->colslave) this->tpars.push_back(p);
    }
    for (Button* b : e->buttons) {
        if (button_curvable(b)) this->tbuts.push_back(b);
    }
    if (this->tpars.empty() && this->tbuts.empty()) {
        mainprogram->infostr = "The elements of this line can not be steered by a curve.";
        return;
    }
    float rmin = 0.0f, rmax = 1.0f;
    if (this->tpars.size() == 1 && this->tbuts.empty()) {
        rmin = this->tpars[0]->range[0];
        rmax = this->tpars[0]->range[1];
    }
    else if (e->curve) {
        rmin = e->curve->rmin;
        rmax = e->curve->rmax;
    }
    this->begin(e, !e->curve, rmin, rmax);
}

void LoopCurveEditor::begin(LoopStationElement* e, bool hadrec, float rmin, float rmax) {
    this->init_widgets();
    this->hadrecording = hadrec;
    if (this->hadrecording) {
        this->bu_events = e->eventlist;
        this->bu_params = e->params;
        this->bu_buttons = e->buttons;
        this->bu_layers = e->layers;
        this->bu_totaltime = e->totaltime;
        this->bu_speed = e->speed->value;
        this->bu_beats = e->beats;
        this->bu_loop = e->loopbut->value;
        this->bu_play = e->playbut->value;
        this->bu_start = e->starttime;
        this->bu_interim = e->interimtime;
        this->bu_spadt = e->speedadaptedtime;
    }
    this->testing = false;
    this->elem = e;
    this->target = (this->tpars.size() == 1 && this->tbuts.empty()) ? this->tpars[0] : nullptr;
    this->targetbut = (this->tbuts.size() == 1 && this->tpars.empty()) ? this->tbuts[0] : nullptr;
    this->hadcurve = (e->curve != nullptr);
    if (this->hadcurve) {
        this->curve = *e->curve;
        this->backup = *e->curve;
    }
    else {
        this->curve = LoopCurve();
        this->curve.init_default(rmin, rmax);
    }
    this->curve.rmin = rmin;
    this->curve.rmax = rmax;
    this->curve.normalize();
    this->initialloop = e->loopbut->value;
    this->touched = false;
    this->selknot = -1;
    this->selhandle = 0;
    this->dragging = 0;
    this->curtype = 0;
    this->totalsize->value = this->curve.totalsize;
    this->oldtotalsize = this->curve.totalsize;
    this->knotx->range[1] = this->curve.totalsize;
    this->knoty->range[0] = rmin;
    this->knoty->range[1] = rmax;
    this->oldsel = -2;
    this->prevdown = false;
    this->pressedonitem = false;
    this->dragended = false;
    this->skipframes = 3;
    for (int i = 0; i < 3; i++) this->typebut[i]->value = (i == 0);
    this->lastsig = this->signature();
    this->active = true;
}

size_t LoopCurveEditor::signature() const {
    size_t h = std::hash<float>()(this->curve.totalsize);
    auto mix = [&h](float v) { h ^= std::hash<float>()(v) + 0x9e3779b9 + (h << 6) + (h >> 2); };
    mix((float)this->curve.knots.size());
    for (auto& k : this->curve.knots) {
        mix(k.x); mix(k.y); mix((float)k.type);
        mix(k.hinx); mix(k.hiny); mix(k.houtx); mix(k.houty);
    }
    return h;
}

void LoopCurveEditor::restore_recording() {
    LoopStationElement* e = this->elem;
    if (!e) return;
    e->erase_elem();
    e->eventlist = this->bu_events;
    e->params = this->bu_params;
    e->buttons = this->bu_buttons;
    e->layers = this->bu_layers;
    e->totaltime = this->bu_totaltime;
    e->speed->value = this->bu_speed;
    e->beats = this->bu_beats;
    // running state as it was: the line continues as if it had never been interrupted
    e->loopbut->value = this->bu_loop;
    e->loopbut->oldvalue = this->bu_loop;
    e->playbut->value = this->bu_play;
    e->playbut->oldvalue = this->bu_play;
    e->starttime = this->bu_start;
    e->interimtime = this->bu_interim;
    e->speedadaptedtime = this->bu_spadt;
    e->eventpos = 0;
    e->atend = false;
    for (Param* p : e->params) {
        loopstation->parelemmap[p] = e;
        loopstation->allparams.emplace(p);
        p->box->acolor[0] = e->colbox->acolor[0];
        p->box->acolor[1] = e->colbox->acolor[1];
        p->box->acolor[2] = e->colbox->acolor[2];
        p->box->acolor[3] = e->colbox->acolor[3];
    }
    for (Button* b : e->buttons) {
        loopstation->butelemmap[b] = e;
        loopstation->allbuttons.emplace(b);
        b->box->acolor[0] = e->colbox->acolor[0];
        b->box->acolor[1] = e->colbox->acolor[1];
        b->box->acolor[2] = e->colbox->acolor[2];
        b->box->acolor[3] = e->colbox->acolor[3];
    }
}

void LoopCurveEditor::cancel() {
    this->active = false;
    if (!this->elem || (this->tpars.empty() && this->tbuts.empty())) return;
    if (this->touched) {
        // undo what testing / realtime editing did to the row
        if (this->hadrecording) {
            // bring the recorded line back
            this->restore_recording();
        }
        else if (this->hadcurve) {
            this->elem->apply_curve(this->tpars, this->tbuts, this->backup);
        }
        else {
            this->elem->erase_elem();
            for (Param* p : this->tpars) loopstation->parelemmap.erase(p);
            for (Button* b : this->tbuts) loopstation->butelemmap.erase(b);
        }
    }
    this->elem->loopbut->value = this->initialloop;
    this->elem->loopbut->oldvalue = this->initialloop;
}

void LoopCurveEditor::apply() {
    this->curve.normalize();
    this->elem->apply_curve(this->tpars, this->tbuts, this->curve);
    this->active = false;
}

float LoopCurveEditor::tox(float t) const {
    return this->px0 + (t / this->curve.totalsize) * this->pw;
}

float LoopCurveEditor::toy(float v) const {
    float r = this->curve.rmax - this->curve.rmin;
    if (r <= 0.0f) return this->py0;
    return this->py0 + ((v - this->curve.rmin) / r) * this->ph;
}

float LoopCurveEditor::fromx(float vx) const {
    return std::clamp((vx - this->px0) / this->pw * this->curve.totalsize, 0.0f, this->curve.totalsize);
}

float LoopCurveEditor::fromy(float vy) const {
    return std::clamp(this->curve.rmin + (vy - this->py0) / this->ph * (this->curve.rmax - this->curve.rmin),
                      this->curve.rmin, this->curve.rmax);
}

void LoopCurveEditor::set_current_type(int type) {
    this->curtype = type;
    for (int i = 0; i < 3; i++) this->typebut[i]->value = (i == type);
    if (this->selknot >= 0 && this->selknot < (int)this->curve.knots.size()) {
        this->curve.set_type(this->selknot, type);
    }
}

void LoopCurveEditor::draw_curve() {
    const int steps = 160;
    float px = this->tox(0.0f), py = this->toy(this->curve.eval(0.0f));
    for (int s = 1; s <= steps; s++) {
        float t = this->curve.totalsize * (float)s / (float)steps;
        float x = this->tox(t), y = this->toy(this->curve.eval(t));
        register_line_draw(lightblue, px, py, x, y);
        px = x;
        py = y;
    }
}

void LoopCurveEditor::handle() {
    if (!this->active) return;
    if (mainprogram->binsroom || mainprogram->styleroom || mainprogram->genroom || mainprogram->segmentationroom) {
        this->cancel();
        return;
    }
    if (!this->elem || this->elem->lpst != loopstation) return;
    int n = this->curve.knots.size();

    // layout, over the loopstation display
    float offdeck = 0.0f;
    if (mainmix->currlay[!mainprogram->prevmodus]) offdeck = !mainmix->currlay[!mainprogram->prevmodus]->deck;
    const float ax = -0.8f + 1.2f * offdeck;
    const float ay = -0.125f;
    const float W = 0.486f;
    const float H = 0.6f;
    this->area->vtxcoords->x1 = ax;
    this->area->vtxcoords->y1 = ay;
    this->area->vtxcoords->w = W;
    this->area->vtxcoords->h = H;
    this->plot->vtxcoords->x1 = ax;
    this->plot->vtxcoords->y1 = ay + 0.15f;
    this->plot->vtxcoords->w = W;
    this->plot->vtxcoords->h = H - 0.15f;
    this->px0 = ax + 0.02f;
    this->pw = W - 0.04f;
    this->py0 = ay + 0.15f + 0.03f;
    this->ph = H - 0.15f - 0.06f;
    this->totalsize->box->vtxcoords->x1 = ax;
    this->totalsize->box->vtxcoords->y1 = ay;
    for (int i = 0; i < 3; i++) {
        this->typebut[i]->box->vtxcoords->x1 = ax + 0.15f + 0.112f * i;
        this->typebut[i]->box->vtxcoords->y1 = ay;
    }
    this->knotx->box->vtxcoords->x1 = ax;
    this->knotx->box->vtxcoords->y1 = ay + 0.075f;
    this->knoty->box->vtxcoords->x1 = ax + 0.15f;
    this->knoty->box->vtxcoords->y1 = ay + 0.075f;
    this->loopsw->vtxcoords->x1 = ax + 0.3f;
    this->loopsw->vtxcoords->y1 = ay + 0.075f;
    this->loopsw->upvtxtoscr();
    this->applybut->box->vtxcoords->x1 = ax + 0.3f + 0.0465f;
    this->applybut->box->vtxcoords->y1 = ay + 0.075f;
    this->area->upvtxtoscr();
    this->plot->upvtxtoscr();
    this->totalsize->box->upvtxtoscr();
    this->knotx->box->upvtxtoscr();
    this->knoty->box->upvtxtoscr();
    this->applybut->box->upvtxtoscr();
    for (int i = 0; i < 3; i++) this->typebut[i]->box->upvtxtoscr();

    // cancel on rightmouse or on a click outside the editor
    if (this->skipframes > 0) {
        this->skipframes--;
        mainprogram->leftmouse = false;
    }
    else {
        if (mainprogram->leftmouse && !this->area->in() && !this->dragended && this->dragging == 0 &&
            mainmix->adaptparam == nullptr && mainprogram->renaming == EDIT_NONE) {
            this->cancel();
            mainprogram->leftmouse = false;
            mainprogram->recundo = false;
            return;
        }
    }
    this->dragended = false;

    // mouse in vertex coordinates
    const float mvx = this->plot->vtxcoords->x1 + ((float)mainprogram->mx - this->plot->scrcoords->x1) / this->plot->scrcoords->w * this->plot->vtxcoords->w;
    const float mvy = this->plot->vtxcoords->y1 + (this->plot->scrcoords->y1 - (float)mainprogram->my) / this->plot->scrcoords->h * this->plot->vtxcoords->h;
    const float hsx = 0.007f;
    const float hsy = hsx * (float)glob->w / (float)glob->h;

    bool wasfront = mainprogram->frontbatch;
    mainprogram->frontbatch = true;

    float bgcol[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float lightgrey[4] = {0.8f, 0.8f, 0.8f, 1.0f};
    float offcol[4] = {0.1f, 0.1f, 0.12f, 1.0f};
    float hovcol[4] = {0.3f, 0.3f, 0.5f, 1.0f};
    float handlecol[4] = {0.7f, 0.7f, 1.0f, 1.0f};
    draw_box(white, bgcol, ax, ay, W, H, -1);
    draw_box(grey, bgcol, this->plot->vtxcoords->x1, this->plot->vtxcoords->y1, this->plot->vtxcoords->w, this->plot->vtxcoords->h, -1);
    // a button curve shows ON / OFF at the borders instead of the range values
    // (several elements on one line: the curve is 0 - 1, every Param scales it to its own range)
    bool onlybuts = this->tpars.empty() && !this->tbuts.empty();
    std::string topstr = onlybuts ? "ON" : (this->multi() ? "MAX" : std::to_string(this->curve.rmax).substr(0, 6));
    std::string botstr = onlybuts ? "OFF" : (this->multi() ? "MIN" : std::to_string(this->curve.rmin).substr(0, 6));
    std::string namestr = this->target ? this->target->name : (this->targetbut ? this->targetbut->name[0] :
                          std::to_string(this->tpars.size() + this->tbuts.size()) + " elements");
    render_text(topstr, lightgrey, ax + 0.005f, this->py0 + this->ph - 0.03f, 0.0004f, 0.00065f);
    render_text(botstr, lightgrey, ax + 0.005f, this->py0 + 0.005f, 0.0004f, 0.00065f);
    render_text(namestr, lightgrey, ax + W * 0.5f - 0.05f, this->py0 + this->ph - 0.03f, 0.0004f, 0.00065f);

    // --- widgets
    // while a knot or handle is being dragged, the widgets must not see the mouse
    const int bumx = mainprogram->mx, bumy = mainprogram->my;
    if (this->dragging) {
        mainprogram->mx = -1000;
        mainprogram->my = -1000;
    }
    this->totalsize->handle();
    this->knotx->handle();
    this->knoty->handle();
    for (int i = 0; i < 3; i++) {
        Button* b = this->typebut[i];
        bool in = b->box->in();
        float* bc = b->value ? (float*)lightblue : (in ? (float*)hovcol : (float*)offcol);
        draw_box(white, bc, b->box, -1);
        render_text(b->name[0], white, b->box->vtxcoords->x1 + 0.01f, b->box->vtxcoords->y1 + 0.03f, 0.0004f, 0.00068f);
        if (in && mainprogram->leftmouse) {
            this->set_current_type(i);
            mainprogram->leftmouse = false;
            mainprogram->recundo = false;
        }
    }
    {
        // loopbut switch of the row (same look as on the loopstation line)
        LoopStationElement* le = this->elem;
        bool in = this->loopsw->in();
        draw_box(white, in ? (float*)hovcol : (float*)offcol, this->loopsw, -1);
        float rx = this->loopsw->vtxcoords->w / 2.0f;
        float ry = this->loopsw->vtxcoords->h / 2.0f;
        // for a recorded line the switch shows only whether the curve is being tested: the recorded line keeps running
        // as it was until the switch is turned on, and runs again when it is turned off
        bool swon = this->hadrecording ? this->testing : (le->loopbut->value != 0);
        draw_box(le->loopbut->ccol, this->loopsw->vtxcoords->x1 + rx, this->loopsw->vtxcoords->y1 + ry, 0.0225f, swon ? 1 : 2);
        //render_text("T", lightgrey, this->loopsw->vtxcoords->x1 + 0.0185f, this->loopsw->vtxcoords->y1 + 0.03f, 0.0004f, 0.00068f);
        if (in && mainprogram->leftmouse) {
            mainprogram->leftmouse = false;
            mainprogram->recundo = false;
            if (!swon) {
                if (!le->curve || le->eventlist.empty()) {
                    // nothing to play yet: put the curve on the row for testing
                    this->curve.normalize();
                    le->apply_curve(this->tpars, this->tbuts, this->curve);
                    this->lastsig = this->signature();
                    this->touched = true;
                }
                this->testing = true;
                if (!le->eventlist.empty()) {
                    // start looping the way the row's own loop button does
                    le->playbut->value = 0;
                    le->playbut->oldvalue = 0;
                    le->recbut->value = 0;
                    le->recbut->oldvalue = 0;
                    le->loopbut->value = 1;
                    le->loopbut->oldvalue = 1;
                    le->starttime = std::chrono::high_resolution_clock::now();
                    le->interimtime = 0;
                    le->speedadaptedtime = 0;
                    le->eventpos = 0;
                    le->atend = false;
                    for (Param* p : this->tpars) p->midistarttime = std::chrono::system_clock::from_time_t(0);
                    for (Button* b : this->tbuts) b->midistarttime = std::chrono::system_clock::from_time_t(0);
                }
            }
            else if (this->hadrecording) {
                // stop testing: the recorded line runs again, as it did before
                this->restore_recording();
                this->testing = false;
                this->touched = false;
            }
            else {
                le->loopbut->value = 0;
                le->loopbut->oldvalue = 0;
            }
        }
    }
    {
        bool in = this->applybut->box->in();
        draw_box(white, in ? (float*)hovcol : (float*)offcol, this->applybut->box, -1);
        render_text(this->applybut->name[0], white, this->applybut->box->vtxcoords->x1 + 0.03f, this->applybut->box->vtxcoords->y1 + 0.03f, 0.0004f, 0.00068f);
        if (in && mainprogram->leftmouse) {
            mainprogram->leftmouse = false;
            mainprogram->recundo = false;
            this->apply();
            mainprogram->mx = bumx;
            mainprogram->my = bumy;
            mainprogram->frontbatch = wasfront;
            return;
        }
    }
    mainprogram->mx = bumx;
    mainprogram->my = bumy;

    // total length edits
    if (this->totalsize->value != this->oldtotalsize) {
        this->curve.set_totalsize(this->totalsize->value);
        this->oldtotalsize = this->totalsize->value;
        this->knotx->range[1] = this->curve.totalsize;
    }

    // --- plot interaction
    auto valid = [&]() { return this->selknot >= 0 && this->selknot < (int)this->curve.knots.size(); };
    bool down = mainprogram->leftmousedown;
    if (down && !this->prevdown && this->plot->in()) {
        // press: pick a handle or knot
        this->pressedonitem = false;
        this->dragging = 0;
        int bestk = -1, besth = 0;
        for (int i = 0; i < n && bestk < 0; i++) {
            CurveKnot& k = this->curve.knots[i];
            if (k.type == 0) continue;
            for (int h = 1; h <= 2; h++) {
                if ((h == 1 && i == 0) || (h == 2 && i == n - 1)) continue;
                // handles are shown (and picked) at half their real length
                float hx = this->tox(k.x + 0.5f * (h == 1 ? k.hinx : k.houtx));
                float hy = this->toy(k.y + 0.5f * (h == 1 ? k.hiny : k.houty));
                // off-editor handles cannot be picked
                bool inside = hx >= this->px0 && hx <= this->px0 + this->pw && hy >= this->py0 && hy <= this->py0 + this->ph;
                if (inside && fabs(mvx - hx) <= hsx * 1.6f && fabs(mvy - hy) <= hsy * 1.6f) {
                    bestk = i;
                    besth = h;
                    break;
                }
            }
        }
        if (bestk >= 0) {
            this->selknot = bestk;
            this->selhandle = besth;
            this->dragging = 2;
            this->pressedonitem = true;
        }
        else {
            for (int i = 0; i < n; i++) {
                CurveKnot& k = this->curve.knots[i];
                if (fabs(mvx - this->tox(k.x)) <= hsx * 1.8f && fabs(mvy - this->toy(k.y)) <= hsy * 1.8f) {
                    this->selknot = i;
                    this->selhandle = 0;
                    this->dragging = 1;
                    this->pressedonitem = true;
                    this->curtype = k.type;
                    for (int b = 0; b < 3; b++) this->typebut[b]->value = (b == k.type);
                    break;
                }
            }
        }
    }
    else if (this->dragging && down) {
        if (valid()) {
            CurveKnot& k = this->curve.knots[this->selknot];
            int last = this->curve.knots.size() - 1;
            if (this->dragging == 1) {
                float eps = this->curve.totalsize * 0.002f;
                if (this->selknot != 0 && this->selknot != last) {
                    k.x = std::clamp(this->fromx(mvx), this->curve.knots[this->selknot - 1].x + eps, this->curve.knots[this->selknot + 1].x - eps);
                }
                k.y = this->fromy(mvy);
            }
            else {
                // the dragged (half length) end stays inside the plot, the real handle is twice as long
                float hx = 2.0f * (this->fromx(mvx) - k.x);
                float hy = 2.0f * (this->fromy(mvy) - k.y);
                if (this->selhandle == 2) {
                    k.houtx = std::max(0.0f, hx);
                    k.houty = hy;
                    if (k.type == 1 && this->selknot > 0) {
                        k.hinx = -k.houtx;
                        k.hiny = -k.houty;
                    }
                }
                else {
                    k.hinx = std::min(0.0f, hx);
                    k.hiny = hy;
                    if (k.type == 1 && this->selknot < last) {
                        k.houtx = -k.hinx;
                        k.houty = -k.hiny;
                    }
                }
            }
            this->curve.normalize();
        }
    }
    else if (this->dragging && !down) {
        this->dragging = 0;
        this->dragended = true;
    }

    if (mainprogram->leftmouse && this->plot->in() && !this->pressedonitem && !this->dragended) {
        // click on the curve in an empty area inserts a knot
        float t = this->fromx(mvx);
        float cy = this->toy(this->curve.eval(t));
        float eps = this->curve.totalsize * 0.01f;
        bool nearknot = false;
        for (auto& k : this->curve.knots) {
            if (fabs(k.x - t) < eps) nearknot = true;
        }
        if (!nearknot && fabs(mvy - cy) <= 0.03f) {
            CurveKnot nk;
            nk.x = t;
            nk.y = this->curve.eval(t);
            int pos = 0;
            while (pos < (int)this->curve.knots.size() && this->curve.knots[pos].x < t) pos++;
            this->curve.knots.insert(this->curve.knots.begin() + pos, nk);
            this->curve.set_type(pos, this->curtype);
            this->selknot = pos;
            this->selhandle = 0;
        }
        mainprogram->leftmouse = false;
        mainprogram->recundo = false;
    }
    if (!down) this->pressedonitem = false;
    this->prevdown = down;

    // DELETE removes the selected knot (never the end knots)
    if (mainprogram->del && mainprogram->renaming == EDIT_NONE) {
        if (valid() && this->selknot > 0 && this->selknot < (int)this->curve.knots.size() - 1) {
            this->curve.knots.erase(this->curve.knots.begin() + this->selknot);
            this->selknot = -1;
            this->selhandle = 0;
            this->curve.normalize();
        }
        mainprogram->del = false;
    }

    // --- selected knot value params
    n = this->curve.knots.size();
    if (valid()) {
        CurveKnot& k = this->curve.knots[this->selknot];
        if (this->selknot == this->oldsel) {
            if (this->knotx->value != this->oldknotx) {
                if (this->selknot > 0 && this->selknot < n - 1) {
                    float eps = this->curve.totalsize * 0.002f;
                    k.x = std::clamp(this->knotx->value, this->curve.knots[this->selknot - 1].x + eps, this->curve.knots[this->selknot + 1].x - eps);
                    this->curve.normalize();
                }
            }
            else if (this->knoty->value != this->oldknoty) {
                k.y = std::clamp(this->knoty->value, this->curve.rmin, this->curve.rmax);
                this->curve.normalize();
            }
        }
        this->oldsel = this->selknot;
        this->knotx->value = k.x;
        this->knoty->value = k.y;
        this->oldknotx = this->knotx->value;
        this->oldknoty = this->knoty->value;
    }
    else {
        this->oldsel = -2;
    }

    // realtime: while the curve is running on its row, edits immediately drive the Param
    {
        size_t sig = this->signature();
        if (sig != this->lastsig) {
            this->lastsig = sig;
            // (a recorded line keeps running untouched until the curve is switched on for testing)
            bool live = this->hadrecording ? this->testing : (this->elem->loopbut->value || this->elem->playbut->value);
            if ((!this->elem->params.empty() || !this->elem->buttons.empty()) && live) {
                this->curve.normalize();
                this->elem->apply_curve(this->tpars, this->tbuts, this->curve);
                this->lastsig = this->signature();
                this->touched = true;
            }
        }
    }

    // --- draw curve, handles and knots
    if (!this->tbuts.empty()) {
        // button curve: a darkgrey line at half Y shows the split between on (above) and off (below)
        float darkgrey[4] = {0.35f, 0.35f, 0.35f, 1.0f};
        register_line_draw(darkgrey, this->px0, this->toy((this->curve.rmin + this->curve.rmax) * 0.5f),
                           this->px0 + this->pw, this->toy((this->curve.rmin + this->curve.rmax) * 0.5f));
    }
    this->draw_curve();
    for (int i = 0; i < n; i++) {
        CurveKnot& k = this->curve.knots[i];
        bool sel = (i == this->selknot);
        float* kc = sel && this->selhandle == 0 ? (float*)orange : (float*)lightblue;
        if (k.type != 0) {
            for (int h = 1; h <= 2; h++) {
                if ((h == 1 && i == 0) || (h == 2 && i == n - 1)) continue;
                float hx = this->tox(k.x + 0.5f * (h == 1 ? k.hinx : k.houtx));
                float hy = this->toy(k.y + 0.5f * (h == 1 ? k.hiny : k.houty));
                float* hc = sel && this->selhandle == h ? (float*)orange : (float*)handlecol;
                float kx = this->tox(k.x), ky = this->toy(k.y);
                bool inside = hx >= this->px0 && hx <= this->px0 + this->pw && hy >= this->py0 && hy <= this->py0 + this->ph;
                float lx = hx, ly = hy;
                if (!inside) {
                    // off-editor: clip the line at the plot edge, hide the handle box (it still shapes the curve)
                    float t = 1.0f;
                    if (hx > this->px0 + this->pw) t = std::min(t, (this->px0 + this->pw - kx) / (hx - kx));
                    if (hx < this->px0) t = std::min(t, (this->px0 - kx) / (hx - kx));
                    if (hy > this->py0 + this->ph) t = std::min(t, (this->py0 + this->ph - ky) / (hy - ky));
                    if (hy < this->py0) t = std::min(t, (this->py0 - ky) / (hy - ky));
                    t = std::clamp(t, 0.0f, 1.0f);
                    lx = kx + (hx - kx) * t;
                    ly = ky + (hy - ky) * t;
                }
                register_line_draw(sel ? (float*)orange : (float*)handlecol, kx, ky, lx, ly);
                if (inside) draw_box(hc, hc, hx - hsx * 0.7f, hy - hsy * 0.7f, hsx * 1.4f, hsy * 1.4f, -1);
            }
        }
        draw_box(kc, kc, this->tox(k.x) - hsx, this->toy(k.y) - hsy, hsx * 2.0f, hsy * 2.0f, -1);
    }

    mainprogram->frontbatch = wasfront;
}
