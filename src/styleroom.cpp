#if defined(WIN32) && !defined(__linux__)
#define WINDOWS
#elif defined(__linux__) && !defined(WIN32)
#define POSIX
#endif

#include "GL/glew.h"
#include "GL/gl.h"
#define FREEGLUT_STATIC
#define _LIB
#define FREEGLUT_LIB_PRAGMAS 0
#include "GL/freeglut.h"

// my own header
#include "program.h"
#include "ReCoNetTrainer.h"

StyleRoom::StyleRoom() {
    this->prepbin = new StylePreparationBin;

#ifdef RECONET_TRAINING_ENABLED
    // Initialize ReCoNet trainer
    this->reconetTrainer = new ReCoNetTrainer();

#else
    this->reconetTrainer = nullptr;
#endif

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            // initialize all 16 preparation file viz boxes
            Boxx *box = new Boxx;
            this->elemboxes.push_back(box);
            box->vtxcoords->x1 = -0.75f + j * 0.24f;
            box->vtxcoords->y1 = 0.60f - ((i % 12) + 1) * 0.30f;
            box->vtxcoords->w = 0.2f;
            box->vtxcoords->h = 0.2f;
            box->upvtxtoscr();
            box->lcolor[0] = 0.4f;
            box->lcolor[1] = 0.4f;
            box->lcolor[2] = 0.4f;
            box->lcolor[3] = 1.0f;
            box->acolor[3] = 1.0f;
            box->tooltiptitle = "Style room inspiration element ";
            box->tooltip = "Shows thumbnail of style room inspiration element, either being a video file (grey border) or an image (white border) or a layer file (orange border).  Hovering over this element shows its resolution.  Leftdrag allows dragging to bin screen via wormgate, going past the left screen border.  Rightclickmenu allows loading files in several ways. ";
        }
    }
    
    this->res = new Param;
    this->res->name = "Res width";
    this->res->value = 1270;
    this->res->deflt = 1270;
    this->res->range[0] = 0;
    this->res->range[1] = 1920;
    this->res->sliding = false;
    this->res->box->acolor[0] = 0.2;
    this->res->box->acolor[1] = 0.2;
    this->res->box->acolor[2] = 0.5;
    this->res->box->acolor[3] = 1.0;
    this->res->box->tooltiptitle = "Set inspiration images resolution ";
    this->res->box->tooltip = "Leftdrag to set the inspiration images resolution. Its value is the resolution width of the 16:9 box the images will be constrained to. Doubleclicking allows numeric entry. ";
    this->quality = new Param;
    this->quality->name = "Training quality";
    this->quality->options.push_back("FAST");
    this->quality->options.push_back("BALANCED");
    this->quality->options.push_back("HIGH");
    this->quality->value = 1;
    this->quality->deflt = 1;
    this->quality->range[0] = 0;
    this->quality->range[1] = 2;
    this->quality->sliding = false;
    this->quality->box->acolor[0] = 0.2;
    this->quality->box->acolor[1] = 0.2;
    this->quality->box->acolor[2] = 0.5;
    this->quality->box->acolor[3] = 1.0;
    this->quality->box->tooltiptitle = "Set training quality ";
    this->quality->box->tooltip = "Set quality of style training. Higher quality means slower training. ";
    this->usegpu = new Param;
    this->usegpu->name = "Use GPU";
    this->usegpu->value = 1;
    this->usegpu->deflt = 1;
    this->usegpu->range[0] = 0;
    this->usegpu->range[1] = 1;
    this->usegpu->sliding = false;
    this->usegpu->box->acolor[0] = 0.2;
    this->usegpu->box->acolor[1] = 0.2;
    this->usegpu->box->acolor[2] = 0.5;
    this->usegpu->box->acolor[3] = 1.0;
    this->usegpu->box->tooltiptitle = "Use GPU accelerated training? ";
    this->usegpu->box->tooltip = "Toggles use of the gpu for style training on or off. ";
    this->res->box->vtxcoords->x1 = -0.5f;
    this->res->box->vtxcoords->y1 = - 0.7f;
    this->res->box->vtxcoords->w = mainprogram->layw * 1.5f * 0.25f;
    this->res->box->vtxcoords->h = 0.075f;
    this->res->box->upvtxtoscr();
    this->quality->box->vtxcoords->x1 = -0.5f;
    this->quality->box->vtxcoords->y1 = - 0.8f;
    this->quality->box->vtxcoords->w = mainprogram->layw * 1.5f * 0.25f;
    this->quality->box->vtxcoords->h = 0.075f;
    this->quality->box->upvtxtoscr();
    this->usegpu->box->vtxcoords->x1 = -0.5f;
    this->usegpu->box->vtxcoords->y1 = - 0.9f;
    this->usegpu->box->vtxcoords->w = mainprogram->layw * 1.5f * 0.25f;
    this->usegpu->box->vtxcoords->h = 0.075f;
    this->usegpu->box->upvtxtoscr();
}

StylePreparationBin::StylePreparationBin() {
}

StylePreparationElement::StylePreparationElement() {
    glGenTextures(1, &this->tex);
    glBindTexture(GL_TEXTURE_2D, this->tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    blacken(this->tex);
}

void StylePreparationElement::erase() {
    this->name = "";
    this->abspath = "";
    this->relpath = "";
    this->absjpath = "";
    this->reljpath = "";
    this->type = ELEM_FILE;
    blacken(this->tex);
}

void StyleRoom::handle() {
    int numd = SDL_GetNumVideoDisplays();
    if (numd > 1) {
        draw_box(binsmain->floatbox, -1);
        if (binsmain->floating)
            render_text("DOCK", white, binsmain->floatbox->vtxcoords->x1 + 0.02f,
                        binsmain->floatbox->vtxcoords->y1 + 0.01f, 0.00045f, 0.00075f);
        else
            render_text("CHOOSE MONITOR", white, binsmain->floatbox->vtxcoords->x1 + 0.02f,
                        binsmain->floatbox->vtxcoords->y1 + 0.01f, 0.00045f, 0.00075f);
        if (binsmain->floatbox->in()) {
            if (mainprogram->leftmouse || mainprogram->rightmouse) {
                if (!binsmain->floating) {
                    std::vector<std::string> bintargets;
                    for (int i = 1; i < numd; i++) {
                        bintargets.push_back(SDL_GetDisplayName(i));
                    }
                    mainprogram->make_menu("bintargetmenu", mainprogram->bintargetmenu, bintargets);
                    mainprogram->bintargetmenu->state = 2;
                    mainprogram->bintargetmenu->menux = mainprogram->mx;
                    mainprogram->bintargetmenu->menuy = mainprogram->my;
                } else {
                    binsmain->floating = false;
                }
            }
        }
    }

    mainprogram->handle_bintargetmenu();

    if (!mainprogram->menuondisplay) this->menuelem = nullptr;
    if (mainprogram->menuactivation) this->menuelem = nullptr;
    for (int i = 0; i < 16; i++) {
        Boxx *box = this->elemboxes[i];
        StylePreparationElement *elem = this->prepbin->elements[i];

        float color[4];
        if (elem->type == ELEM_LAYER) {
            color[0] = 1.0f;
            color[1] = 0.5f;
            color[2] = 0.0f;
            color[3] = 1.0f;
        } else if (elem->type == ELEM_IMAGE) {
            color[0] = 0.9f;
            color[1] = 0.8f;
            color[2] = 0.0f;
            color[3] = 1.0f;
        } else {
            color[0] = 0.4f;
            color[1] = 0.4f;
            color[2] = 0.4f;
            color[3] = 1.0f;
        }

        // visualize elements
        draw_box(box, elem->tex);
        // grey areas next to each element column to cut off element titles
        draw_box(nullptr, color, box->vtxcoords->x1 - 0.01f, box->vtxcoords->y1 - 0.035f, box->vtxcoords->w + 0.02f,
                 0.028f, -1);
        if (elem->name != "") {
            if (elem->name != "")
                render_text(elem->name.substr(0, 20), white, box->vtxcoords->x1, box->vtxcoords->y1 - 0.03f, 0.00090f,
                            0.000150f);
        }
    }

    this->res->handle();
    this->quality->handle();
    this->usegpu->handle();

    Boxx box;
    box.vtxcoords->x1 = -0.3f;
    box.vtxcoords->y1 = -0.8f;
    box.vtxcoords->w = 0.3f;
    box.vtxcoords->h = 0.15f;
    box.upvtxtoscr();
    draw_box(&box, -1);
    render_text("TRAIN", white, -0.25f, -0.75f, 0.0009f, 0.000150f);
    if (box.in()) {
        if (mainprogram->leftmouse) {
            ReCoNetTrainer::Config config;
            switch ((int)this->quality->value) {
                case 0:
                    config.quality = ReCoNetTrainer::Quality::FAST;
                    break;
                case 1:
                    config.quality = ReCoNetTrainer::Quality::BALANCED;
                    break;
                case 2:
                    config.quality = ReCoNetTrainer::Quality::HIGH;
                    break;
            }
            config.useGPU = this->usegpu->value;  // User checkbox

            bool started = mainstyleroom->reconetTrainer->startTraining(
                    mainstyleroom->prepbin,
                    "Tierlantijn",  // Model name from text input
                    config);
            if (!started) {
                std::string error = mainstyleroom->reconetTrainer->getLastError();
                // Show error to user
                printf("%s\n", error.c_str());
            }
        }
    }

    if (mainstyleroom->reconetTrainer->isTraining()) {
        auto progress = mainstyleroom->reconetTrainer->getProgress();

        // Update UI:
        // - Progress bar: progress.currentIteration / progress.totalIterations
        // - Status text: progress.status
        // - Loss value: progress.totalLoss
        // - Percentage: (progress.currentIteration * 100) / progress.totalIterations

        // Example:
        int percent = (progress.currentIteration * 100) / progress.totalIterations;
        std::string statusText = progress.status + " " + std::to_string(percent) + "%";
        render_text(statusText, white, 0.5f, -0.75f, 0.00045f, 0.000075f);
    }
}