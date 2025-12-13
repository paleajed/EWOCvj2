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

StyleRoom::StyleRoom() {
    this->prepbin = new StylePreparationBin;

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
}