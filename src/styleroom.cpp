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
#include "AIStyleTransfer.h"

StyleRoom::StyleRoom() {
    this->prepbin = new StylePreparationBin;

#ifdef RECONET_TRAINING_ENABLED
    // Initialize ReCoNet trainer
    this->reconetTrainer = new ReCoNetTrainer();

#else
    this->reconetTrainer = nullptr;
#endif

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 3; j++) {
            // initialize all 12 preparation file viz boxes
            Boxx *box = new Boxx;
            this->elemboxes.push_back(box);
            box->vtxcoords->x1 = -0.8f + j * 0.24f;
            box->vtxcoords->y1 = 0.95f - ((i % 12) + 1) * 0.30f;
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

    this->stylenamesbox = new Boxx;
    this->stylenamesbox->vtxcoords->x1 = -0.04f;
    this->stylenamesbox->vtxcoords->y1 = this->elemboxes[11]->vtxcoords->y1;
    this->stylenamesbox->vtxcoords->w = 0.248f;
    this->stylenamesbox->vtxcoords->h = this->elemboxes[0]->vtxcoords->y1 + this->elemboxes[0]->vtxcoords->h - this->stylenamesbox->vtxcoords->y1;
    this->stylenamesbox->vtxcoords->h = (float)((int)(this->stylenamesbox->vtxcoords->h / 0.05f)) * 0.05f;
    this->stylenamesbox->upvtxtoscr();

    // arrow box to scroll bins list up
    this->stylesscrollup = new Boxx;
    this->stylesscrollup->vtxcoords->x1 = -0.065f;
    this->stylesscrollup->vtxcoords->y1 = this->stylenamesbox->vtxcoords->y1 + this->stylenamesbox->vtxcoords->w - 0.05f;
    this->stylesscrollup->vtxcoords->w = 0.025f;
    this->stylesscrollup->vtxcoords->h = 0.05f;
    this->stylesscrollup->upvtxtoscr();
    this->stylesscrollup->tooltiptitle = "Scroll styles list up ";
    this->stylesscrollup->tooltip = "Leftclicking scrolls the styles list up ";

    // arrow box to scroll styles list down-0.029f
    this->stylesscrolldown = new Boxx;
    this->stylesscrolldown->vtxcoords->x1 = -0.065f;
    this->stylesscrolldown->vtxcoords->y1 = this->stylenamesbox->vtxcoords->y1;
    this->stylesscrolldown->vtxcoords->w = 0.025f;
    this->stylesscrolldown->vtxcoords->h = 0.05f;
    this->stylesscrolldown->upvtxtoscr();
    this->stylesscrolldown->tooltiptitle = "Scroll styles list down ";
    this->stylesscrolldown->tooltip = "Leftclicking scrolls the styles list down ";

    this->mode = new Param;
    this->mode->type = FF_TYPE_OPTION;
    this->mode->name = "Mode";
    this->mode->options.push_back("BEGINNER");
    this->mode->options.push_back("ADVANCED");
    this->mode->value = 0;
    this->mode->deflt = 0;
    this->mode->range[0] = 0.0f;
    this->mode->range[1] = 1.0f;
    this->mode->sliding = false;
    this->mode->box->acolor[0] = 0.2;
    this->mode->box->acolor[1] = 0.5;
    this->mode->box->acolor[2] = 0.2;
    this->mode->box->acolor[3] = 1.0;
    this->mode->box->tooltiptitle = "Set training mode ";
    this->mode->box->tooltip = "Switch between beginner and advanced mode. ";
    this->res = new Param;
    this->res->name = "Inputres";
    this->res->value = 1280;
    this->res->deflt = 1280;
    this->res->range[0] = 0;
    this->res->range[1] = 1920;
    this->res->sliding = false;
    this->res->box->acolor[0] = 0.2;
    this->res->box->acolor[1] = 0.2;
    this->res->box->acolor[2] = 0.5;
    this->res->box->acolor[3] = 1.0;
    this->res->box->tooltiptitle = "Set inspiration images resolution ";
    this->res->box->tooltip = "Leftdrag to set the inspiration images resolution. Its value is the resolution width of the 16:9 box the images will be constrained to.  Smaller sizes use less VRAM.  Doubleclicking allows numeric entry. ";
    this->quality = new Param;
    this->quality->type = FF_TYPE_OPTION;
    this->quality->name = "Quality";
    this->quality->options.push_back("FAST");
    this->quality->options.push_back("COARSE");
    this->quality->options.push_back("FINE");
    this->quality->options.push_back("HI-COARSE");
    this->quality->options.push_back("HI-FINE");
    this->quality->value = 2;
    this->quality->deflt = 2;
    this->quality->range[0] = 0;
    this->quality->range[1] = 4;
    this->quality->sliding = false;
    this->quality->box->acolor[0] = 0.2;
    this->quality->box->acolor[1] = 0.2;
    this->quality->box->acolor[2] = 0.5;
    this->quality->box->acolor[3] = 1.0;
    this->quality->box->tooltiptitle = "Set training quality ";
    this->quality->box->tooltip = "Set quality of style training. Higher quality means slower training. ";
    this->influence = new Param;
    this->influence->type = FF_TYPE_OPTION;
    this->influence->name = "Influence";
    this->influence->options.push_back("MINIMAL");
    this->influence->options.push_back("BALANCED");
    this->influence->options.push_back("STRONG");
    this->influence->value = 1;
    this->influence->deflt = 1;
    this->influence->range[0] = 0;
    this->influence->range[1] = 2;
    this->influence->sliding = false;
    this->influence->box->acolor[0] = 0.2;
    this->influence->box->acolor[1] = 0.2;
    this->influence->box->acolor[2] = 0.5;
    this->influence->box->acolor[3] = 1.0;
    this->influence->box->tooltiptitle = "Set training style influence ";
    this->influence->box->tooltip = "Set influence of style in training. ";
    this->abstraction = new Param;
    this->abstraction->type = FF_TYPE_OPTION;
    this->abstraction->name = "Abstraction";
    this->abstraction->options.push_back("LOW");
    this->abstraction->options.push_back("MEDIUM");
    this->abstraction->options.push_back("HIGH");
    this->abstraction->value = 1;
    this->abstraction->deflt = 1;
    this->abstraction->range[0] = 0;
    this->abstraction->range[1] = 2;
    this->abstraction->sliding = false;
    this->abstraction->box->acolor[0] = 0.2;
    this->abstraction->box->acolor[1] = 0.2;
    this->abstraction->box->acolor[2] = 0.5;
    this->abstraction->box->acolor[3] = 1.0;
    this->abstraction->box->tooltiptitle = "Set training style abstraction level ";
    this->abstraction->box->tooltip = "Set abstraction level of trained style. ";
    this->coherence = new Param;
    this->coherence->type = FF_TYPE_OPTION;
    this->coherence->options.push_back("OFF");
    this->coherence->options.push_back("ON");
    this->coherence->name = "Coherence";
    this->coherence->value = 1;
    this->coherence->deflt = 1;
    this->coherence->range[0] = 0;
    this->coherence->range[1] = 1;
    this->coherence->sliding = false;
    this->coherence->box->acolor[0] = 0.2;
    this->coherence->box->acolor[1] = 0.2;
    this->coherence->box->acolor[2] = 0.5;
    this->coherence->box->acolor[3] = 1.0;
    this->coherence->box->tooltiptitle = "Temporal coherence ";
    this->coherence->box->tooltip = "Toggles temporal coherence of the trained style on or off.  Coherence battles flashing styles but takes more training time. ";
    this->usegpu = new Param;
    this->usegpu->type = FF_TYPE_OPTION;
    this->usegpu->options.push_back("OFF");
    this->usegpu->options.push_back("ON");
    this->usegpu->name = "UseGPU";
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

    this->layrelu1 = new Param;
    this->layrelu1->name = "Layer 1";
    this->layrelu1->value = 1.0f;
    this->layrelu1->deflt = 1.0f;
    this->layrelu1->range[0] = 0.0f;
    this->layrelu1->range[1] = 5.0f;
    this->layrelu1->sliding = true;
    this->layrelu1->box->acolor[0] = 0.5;
    this->layrelu1->box->acolor[1] = 0.2;
    this->layrelu1->box->acolor[2] = 0.2;
    this->layrelu1->box->acolor[3] = 1.0;
    this->layrelu1->box->tooltiptitle = "Low-level abstraction layer ";
    this->layrelu1->box->tooltip = "Captures edges, colors, and basic textures. Leftdrag to set weight.  Doubleclicking allows numeric entry. ";
    this->layrelu2 = new Param;
    this->layrelu2->name = "Layer 2";
    this->layrelu2->value = 1.0f;
    this->layrelu2->deflt = 1.0f;
    this->layrelu2->range[0] = 0.0f;
    this->layrelu2->range[1] = 5.0f;
    this->layrelu2->sliding = true;
    this->layrelu2->box->acolor[0] = 0.5;
    this->layrelu2->box->acolor[1] = 0.2;
    this->layrelu2->box->acolor[2] = 0.2;
    this->layrelu2->box->acolor[3] = 1.0;
    this->layrelu2->box->tooltiptitle = "Early mid-level abstraction layer ";
    this->layrelu2->box->tooltip = "Detects simple patterns and local structures. Leftdrag to set weight.  Doubleclicking allows numeric entry. ";
    this->layrelu3 = new Param;
    this->layrelu3->name = "Layer 3";
    this->layrelu3->value = 1.0f;
    this->layrelu3->deflt = 1.0f;
    this->layrelu3->range[0] = 0.0f;
    this->layrelu3->range[1] = 5.0f;
    this->layrelu3->sliding = true;
    this->layrelu3->box->acolor[0] = 0.5;
    this->layrelu3->box->acolor[1] = 0.2;
    this->layrelu3->box->acolor[2] = 0.2;
    this->layrelu3->box->acolor[3] = 1.0;
    this->layrelu3->box->tooltiptitle = "Mid-level abstraction layer ";
    this->layrelu3->box->tooltip = "Captures texture patterns and repeated motifs. Leftdrag to set weight.  Doubleclicking allows numeric entry. ";
    this->layrelu4 = new Param;
    this->layrelu4->name = "Layer 4";
    this->layrelu4->value = 1.0f;
    this->layrelu4->deflt = 1.0f;
    this->layrelu4->range[0] = 0.0f;
    this->layrelu4->range[1] = 5.0f;
    this->layrelu4->sliding = true;
    this->layrelu4->box->acolor[0] = 0.5;
    this->layrelu4->box->acolor[1] = 0.2;
    this->layrelu4->box->acolor[2] = 0.2;
    this->layrelu4->box->acolor[3] = 1.0;
    this->layrelu4->box->tooltiptitle = "Late mid-level abstraction layer ";
    this->layrelu4->box->tooltip = "Abstracts object parts and complex texture arrangements. Leftdrag to set weight.  Doubleclicking allows numeric entry. ";
    this->layrelu5 = new Param;
    this->layrelu5->name = "Layer 5";
    this->layrelu5->value = 1.0f;
    this->layrelu5->deflt = 1.0f;
    this->layrelu5->range[0] = 0.0f;
    this->layrelu5->range[1] = 5.0f;
    this->layrelu5->sliding = true;
    this->layrelu5->box->acolor[0] = 0.5;
    this->layrelu5->box->acolor[1] = 0.2;
    this->layrelu5->box->acolor[2] = 0.2;
    this->layrelu5->box->acolor[3] = 1.0;
    this->layrelu5->box->tooltiptitle = "High-level abstraction layer ";
    this->layrelu5->box->tooltip = "Most abstract; represents semantic content and overall structure. Leftdrag to set weight.  Doubleclicking allows numeric entry. ";
    this->contentweight = new Param;
    this->contentweight->name = "Content";
    this->contentweight->value = 2.0f;
    this->contentweight->deflt = 2.0f;
    this->contentweight->range[0] = 0.0f;
    this->contentweight->range[1] = 10.0f;
    this->contentweight->sliding = true;
    this->contentweight->box->acolor[0] = 0.5;
    this->contentweight->box->acolor[1] = 0.2;
    this->contentweight->box->acolor[2] = 0.2;
    this->contentweight->box->acolor[3] = 1.0;
    this->contentweight->box->tooltiptitle = "Weight of input content ";
    this->contentweight->box->tooltip = "Sets how much of the original video input content to preserve when processing. Leftdrag to set weight.  Doubleclicking allows numeric entry. ";
    this->styleweight = new Param;
    this->styleweight->name = "Style";
    this->styleweight->value = 6.0f;
    this->styleweight->deflt = 6.0f;
    this->styleweight->range[0] = 0.0f;
    this->styleweight->range[1] = 10.0f;
    this->styleweight->sliding = true;
    this->styleweight->box->acolor[0] = 0.5;
    this->styleweight->box->acolor[1] = 0.2;
    this->styleweight->box->acolor[2] = 0.2;
    this->styleweight->box->acolor[3] = 1.0;
    this->styleweight->box->tooltiptitle = "Weight of style influence ";
    this->styleweight->box->tooltip = "Sets the style influence weight when processing. Leftdrag to set weight.  Doubleclicking allows numeric entry. ";
    this->temporalweight = new Param;
    this->temporalweight->name = "Temporal";
    this->temporalweight->value = 10.0f;
    this->temporalweight->deflt = 10.0f;
    this->temporalweight->range[0] = 0.0f;
    this->temporalweight->range[1] = 10.0f;
    this->temporalweight->sliding = false;
    this->temporalweight->box->acolor[0] = 0.5;
    this->temporalweight->box->acolor[1] = 0.2;
    this->temporalweight->box->acolor[2] = 0.2;
    this->temporalweight->box->acolor[3] = 1.0;
    this->temporalweight->box->tooltiptitle = "Weight of input content ";
    this->temporalweight->box->tooltip = "Sets how much of the original video input content to preserve when processing. Leftdrag to set weight.  Doubleclicking allows numeric entry. ";
    this->trainres = new Param;
    this->trainres->type = FF_TYPE_OPTION;
    this->trainres->options.push_back("256");
    this->trainres->options.push_back("512");
    this->trainres->name = "Trainres";
    this->trainres->value = 0;
    this->trainres->deflt = 0;
    this->trainres->range[0] = 0;
    this->trainres->range[1] = 1;
    this->trainres->sliding = false;
    this->trainres->box->acolor[0] = 0.5;
    this->trainres->box->acolor[1] = 0.2;
    this->trainres->box->acolor[2] = 0.2;
    this->trainres->box->acolor[3] = 1.0;
    this->trainres->box->tooltiptitle = "Set training resolution ";
    this->trainres->box->tooltip = "Leftdrag to set the training resolution.  Doubleclicking allows numeric entry. ";
    this->trainiter = new Param;
    this->trainiter->name = "Iterations";
    this->trainiter->value = 20;
    this->trainiter->deflt = 20;
    this->trainiter->range[0] = 5;
    this->trainiter->range[1] = 50;
    this->trainiter->sliding = false;
    this->trainiter->box->acolor[0] = 0.5;
    this->trainiter->box->acolor[1] = 0.2;
    this->trainiter->box->acolor[2] = 0.2;
    this->trainiter->box->acolor[3] = 1.0;
    this->trainiter->box->tooltiptitle = "Set number of training iterations ";
    this->trainiter->box->tooltip = "Leftdrag to set the number of training iterations.  Doubleclicking allows numeric entry. ";
    this->batchsize = new Param;
    this->batchsize->name = "Batchsize";
    this->batchsize->value = 4;
    this->batchsize->deflt = 4;
    this->batchsize->range[0] = 1;
    this->batchsize->range[1] = 8;
    this->batchsize->sliding = false;
    this->batchsize->box->acolor[0] = 0.5;
    this->batchsize->box->acolor[1] = 0.2;
    this->batchsize->box->acolor[2] = 0.2;
    this->batchsize->box->acolor[3] = 1.0;
    this->batchsize->box->tooltiptitle = "Set content batchsize ";
    this->batchsize->box->tooltip = "Leftdrag to set content batchsize.  Larger batch: More stable gradients, faster training per iteration, but uses more VRAM.  Smaller batch: Less VRAM, can be noisier gradients, but sometimes generalizes better.  Doubleclicking allows numeric entry. ";

    float paramx = -0.75f;
    float paramy = -0.6f;
    float paramw = mainprogram->layw * 1.5f * 0.25f;
    float paramh = 0.1f;
    this->quality->box->vtxcoords->x1 = paramx;
    this->quality->box->vtxcoords->y1 = paramy;
    this->quality->box->vtxcoords->w = paramw;
    this->quality->box->vtxcoords->h = 0.075f;
    this->quality->box->upvtxtoscr();
    this->influence->box->vtxcoords->x1 = paramx;
    this->influence->box->vtxcoords->y1 = paramy - paramh;
    this->influence->box->vtxcoords->w = paramw;
    this->influence->box->vtxcoords->h = 0.075f;
    this->influence->box->upvtxtoscr();
    this->abstraction->box->vtxcoords->x1 = paramx;
    this->abstraction->box->vtxcoords->y1 = paramy - paramh * 2.0f;
    this->abstraction->box->vtxcoords->w = paramw;
    this->abstraction->box->vtxcoords->h = 0.075f;
    this->abstraction->box->upvtxtoscr();
    this->coherence->box->vtxcoords->x1 = paramx;
    this->coherence->box->vtxcoords->y1 = paramy - paramh * 3.0f;
    this->coherence->box->vtxcoords->w = paramw;
    this->coherence->box->vtxcoords->h = 0.075f;
    this->coherence->box->upvtxtoscr();
    this->res->box->vtxcoords->x1 = paramx + paramw + 0.015f;
    this->res->box->vtxcoords->y1 = paramy;
    this->res->box->vtxcoords->w = paramw;
    this->res->box->vtxcoords->h = 0.075f;
    this->res->box->upvtxtoscr();
    this->usegpu->box->vtxcoords->x1 = paramx + paramw + 0.015f;
    this->usegpu->box->vtxcoords->y1 = paramy - paramh;
    this->usegpu->box->vtxcoords->w = paramw;
    this->usegpu->box->vtxcoords->h = 0.075f;
    this->usegpu->box->upvtxtoscr();

    float paramx2 = 0.34f;
    float paramy2 = 0.2f;
    this->mode->box->vtxcoords->x1 = paramx2;
    this->mode->box->vtxcoords->y1 = paramy2 + paramh * 5.0;
    this->mode->box->vtxcoords->w = paramw;
    this->mode->box->vtxcoords->h = 0.075f;
    this->mode->box->upvtxtoscr();
    this->layrelu1->box->vtxcoords->x1 = paramx2;
    this->layrelu1->box->vtxcoords->y1 = paramy2 + paramh * 4.0;
    this->layrelu1->box->vtxcoords->w = paramw;
    this->layrelu1->box->vtxcoords->h = 0.075f;
    this->layrelu1->box->upvtxtoscr();
    this->layrelu2->box->vtxcoords->x1 = paramx2;
    this->layrelu2->box->vtxcoords->y1 = paramy2 + paramh * 3.0;
    this->layrelu2->box->vtxcoords->w = paramw;
    this->layrelu2->box->vtxcoords->h = 0.075f;
    this->layrelu2->box->upvtxtoscr();
    this->layrelu3->box->vtxcoords->x1 = paramx2;
    this->layrelu3->box->vtxcoords->y1 = paramy2 + paramh * 2.0;
    this->layrelu3->box->vtxcoords->w = paramw;
    this->layrelu3->box->vtxcoords->h = 0.075f;
    this->layrelu3->box->upvtxtoscr();
    this->layrelu4->box->vtxcoords->x1 = paramx2;
    this->layrelu4->box->vtxcoords->y1 = paramy2 + paramh;
    this->layrelu4->box->vtxcoords->w = paramw;
    this->layrelu4->box->vtxcoords->h = 0.075f;
    this->layrelu4->box->upvtxtoscr();
    this->layrelu5->box->vtxcoords->x1 = paramx2;
    this->layrelu5->box->vtxcoords->y1 = paramy2;
    this->layrelu5->box->vtxcoords->w = paramw;
    this->layrelu5->box->vtxcoords->h = 0.075f;
    this->layrelu5->box->upvtxtoscr();
    this->contentweight->box->vtxcoords->x1 = paramx2 + paramw + 0.015f;;
    this->contentweight->box->vtxcoords->y1 = paramy2 + paramh * 4.0f;
    this->contentweight->box->vtxcoords->w = paramw;
    this->contentweight->box->vtxcoords->h = 0.075f;
    this->contentweight->box->upvtxtoscr();
    this->styleweight->box->vtxcoords->x1 = paramx2 + paramw + 0.015f;;
    this->styleweight->box->vtxcoords->y1 = paramy2 + paramh * 3.0f;
    this->styleweight->box->vtxcoords->w = paramw;
    this->styleweight->box->vtxcoords->h = 0.075f;
    this->styleweight->box->upvtxtoscr();
    this->temporalweight->box->vtxcoords->x1 = paramx2 + paramw + 0.015f;;
    this->temporalweight->box->vtxcoords->y1 = paramy2 + paramh * 2.0f;
    this->temporalweight->box->vtxcoords->w = paramw;
    this->temporalweight->box->vtxcoords->h = 0.075f;
    this->temporalweight->box->upvtxtoscr();
    this->trainres->box->vtxcoords->x1 = paramx2 + paramw * 2.0f + 0.03f;
    this->trainres->box->vtxcoords->y1 = paramy2 + paramh * 4.0;
    this->trainres->box->vtxcoords->w = paramw;
    this->trainres->box->vtxcoords->h = 0.075f;
    this->trainres->box->upvtxtoscr();
    this->trainiter->box->vtxcoords->x1 = paramx2 + paramw * 2.0f + 0.03f;
    this->trainiter->box->vtxcoords->y1 = paramy2 + paramh * 3.0f;
    this->trainiter->box->vtxcoords->w = paramw;
    this->trainiter->box->vtxcoords->h = 0.075f;
    this->trainiter->box->upvtxtoscr();
    this->batchsize->box->vtxcoords->x1 = paramx2 + paramw * 2.0f + 0.03f;
    this->batchsize->box->vtxcoords->y1 = paramy2 + paramh * 2.0f;
    this->batchsize->box->vtxcoords->w = paramw;
    this->batchsize->box->vtxcoords->h = 0.075f;
    this->batchsize->box->upvtxtoscr();

    std::string modelsdir;
#ifdef _WIN32
    modelsdir = "C:/ProgramData/EWOCvj2/models/styles/";
#else
    modelsdir = "/usr/share/EWOCvj2/models/styles/";
#endif
    std::string path;
    int count = 0;
    while (1) {
        path = modelsdir + this->currstylename + ".onnx";
        if (!exists(path)) {
            break;
        }
        count++;
        this->currstylename = remove_version(this->currstylename) + "_" + std::to_string(count);
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
    this->type = ELEM_FILE;
    blacken(this->tex);
}

StylePreparationElement::~StylePreparationElement() {
    // Join upscale thread if it's running
    if (this->upscaleBinel && this->upscaleBinel->upscaleThread && this->upscaleBinel->upscaleThread->joinable()) {
        this->upscaleBinel->upscaleThread->join();
    }
}

void StylePreparationElement::check_upscale_complete() {
    if (this->upscaleBinel && this->upscaleBinel->upscaleComplete.load()) {
        // Join the thread
        if (this->upscaleBinel->upscaleThread && this->upscaleBinel->upscaleThread->joinable()) {
            this->upscaleBinel->upscaleThread->join();
        }

        if (this->upscaleBinel->upscaleSuccess.load()) {
            // Update this element's path to the upscaled version
            this->abspath = this->upscaleBinel->upscaledPath;
            this->name = remove_extension(basename(this->abspath));
        }

        // Clean up the temporary BinElement
        this->upscaleBinel.reset();
    }
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

    float border = 0.07f;
    draw_box(white, darkgreen2, this->elemboxes[0]->vtxcoords->x1 - border, this->elemboxes[11]->vtxcoords->y1 - border - 0.07f, 0.24f * 4.0f + border * 2.0f + 0.05f, 0.3f * 4.0f + border * 2.0f, -1);
    bool inbox = false;
    for (int i = 0; i < 12; i++) {
        Boxx *box = this->elemboxes[i];
        StylePreparationElement *elem = this->prepbin->elements[i];

        // Check if async upscaling is complete
        elem->check_upscale_complete();

        float color[4];
        color[0] = 0.4f;
        color[1] = 0.4f;
        color[2] = 0.4f;
        color[3] = 1.0f;

        // visualize elements
        draw_box(box, elem->tex);
        // grey areas next to each element column to cut off element titles
        draw_box(nullptr, color, box->vtxcoords->x1 - 0.01f, box->vtxcoords->y1 - 0.035f, box->vtxcoords->w + 0.02f,
                 0.028f, -1);
        if (elem->name != "") {
            if (elem->name != "")
                render_text(elem->name.substr(0, 20), white, box->vtxcoords->x1, box->vtxcoords->y1 - 0.03f, 0.00045f,
                            0.00075f);
        }
        
        if (box->in()) {
            inbox = true;
            this->menuelem = elem;
            if (elem->name != "") {
                // Set menu when over non-empty element
                this->elemmenuoptions.clear();
                std::vector<std::string> snlm;
                snlm.push_back("Delete element");
                elemmenuoptions.push_back(SET_DELETE);
                snlm.push_back("Open file(s) from disk");
                elemmenuoptions.push_back(SET_OPENFILES);
                snlm.push_back("Insert deck A");
                elemmenuoptions.push_back(SET_INSDECKA);
                snlm.push_back("Insert deck B");
                elemmenuoptions.push_back(SET_INSDECKB);
                snlm.push_back("Insert full mix");
                elemmenuoptions.push_back(SET_INSMIX);
                snlm.push_back("Open style profile");
                elemmenuoptions.push_back(SET_OPENSTYLE);
                snlm.push_back("Save style profile");
                elemmenuoptions.push_back(SET_SAVESTYLE);
                snlm.push_back("submenu upscalemenu");
                snlm.push_back("Upscale image");
                elemmenuoptions.push_back(SET_UPSCALEIMAGE);
                snlm.push_back("Quit");
                elemmenuoptions.push_back(SET_QUIT);

                mainprogram->make_menu("styleroommenu", mainstyleroom->styleroommenu, snlm);
            } else {
                // Set menu when over empty element
                this->elemmenuoptions.clear();
                std::vector<std::string> snlm;
                snlm.push_back("Open file(s) from disk");
                elemmenuoptions.push_back(SET_OPENFILES);
                snlm.push_back("Insert deck A");
                elemmenuoptions.push_back(SET_INSDECKA);
                snlm.push_back("Insert deck B");
                elemmenuoptions.push_back(SET_INSDECKB);
                snlm.push_back("Insert full mix");
                elemmenuoptions.push_back(SET_INSMIX);
                snlm.push_back("Open style profile");
                elemmenuoptions.push_back(SET_OPENSTYLE);
                snlm.push_back("Save style profile");
                elemmenuoptions.push_back(SET_SAVESTYLE);
                snlm.push_back("Quit");
                elemmenuoptions.push_back(SET_QUIT);

                mainprogram->make_menu("styleroommenu", mainstyleroom->styleroommenu, snlm);
            }
            if (mainprogram->menuactivation) {
                this->styleroommenu->state = 2;
                this->styleroommenu->menux = mainprogram->mx;
                this->styleroommenu->menuy = mainprogram->my;
            }
            if (mainprogram->lmover && mainprogram->dragbinel) {
                elem->abspath = mainprogram->dragbinel->path;
                elem->name = mainprogram->dragbinel->name;
                elem->tex = mainprogram->dragbinel->tex;
                enddrag();
            }
        }
    }
    if (!inbox) {
        if (mainprogram->menuactivation) {
            // Set menu when over no element
            this->elemmenuoptions.clear();
            std::vector<std::string> snlm;
            snlm.push_back("Save project");
            elemmenuoptions.push_back(SET_SAVPROJ);
            snlm.push_back("Open style profile");
            elemmenuoptions.push_back(SET_OPENSTYLE);
            snlm.push_back("Save style profile");
            elemmenuoptions.push_back(SET_SAVESTYLE);
            snlm.push_back("Quit");
            elemmenuoptions.push_back(SET_QUIT);

            mainprogram->make_menu("styleroommenu", mainstyleroom->styleroommenu, snlm);

            this->styleroommenu->state = 2;
            this->styleroommenu->menux = mainprogram->mx;
            this->styleroommenu->menuy = mainprogram->my;
        }
    }

 	// handle binelmenu thats been populated above, menuset controls which options sets are used
	int k = -1;
	k = mainprogram->handle_menu(mainstyleroom->styleroommenu);
	if (elemmenuoptions.size() && k > -1) {
	    //if (elemmenuoptions[k] != SET_OPENFILES) this->menuactbinel = nullptr;
	    if (elemmenuoptions[k] == SET_DELETE) {
	        // delete hovered bin element
	    	this->menuelem->erase();
	    }
	    else if (elemmenuoptions[k] == SET_OPENFILES) {
	        // open videos/images/layer files into bin
	        mainprogram->pathto = "OPENFILESSTYLE";
	        std::thread filereq(&Program::get_multinname, mainprogram, "Open file(s)", "", std::filesystem::canonical(mainprogram->currfilesdir).generic_string());
	        filereq.detach();
	    }
	    else if (elemmenuoptions[k] == SET_INSDECKA) {
	        // insert deck A into bin
	        {
	            std::lock_guard<std::mutex> lock(mainprogram->pathmutex);
	            mainprogram->paths.clear();
	        }
	        mainmix->mousedeck = 0;
	    	std::string path = find_unused_filename("deckA", mainprogram->temppath, ".jpg");
			if (mainprogram->prevmodus) {
	            this->menuelem->tex = copy_tex(mainprogram->nodesmain->mixnodes[0][mainmix->mousedeck]->mixtex, 1920, 1080);
	        }
	        else {
	            this->menuelem->tex = copy_tex(mainprogram->nodesmain->mixnodes[1][mainmix->mousedeck]->mixtex, 1920, 1080);
	        }
	    	save_thumb(path, this->menuelem->tex);
	        this->menuelem->type = ELEM_IMAGE;
	        this->menuelem->abspath = path;
	        this->menuelem->name = remove_extension(basename(this->menuelem->abspath));
	    }
	    else if (elemmenuoptions[k] == SET_INSDECKB) {
	        // insert deck B into bin
		    {
		    	std::lock_guard<std::mutex> lock(mainprogram->pathmutex);
		    	mainprogram->paths.clear();
		    }
	    	mainmix->mousedeck = 1;
	    	std::string path = find_unused_filename("deckB", mainprogram->temppath, ".jpg");
	    	if (mainprogram->prevmodus) {
	    		this->menuelem->tex = copy_tex(mainprogram->nodesmain->mixnodes[0][mainmix->mousedeck]->mixtex, 1920, 1080);
	    	}
	    	else {
	    		this->menuelem->tex = copy_tex(mainprogram->nodesmain->mixnodes[1][mainmix->mousedeck]->mixtex, 1920, 1080);
	    	}
	    	save_thumb(path, this->menuelem->tex);
	    	this->menuelem->type = ELEM_IMAGE;
	    	this->menuelem->abspath = path;
	    	this->menuelem->name = remove_extension(basename(this->menuelem->abspath));
	    }
	    else if (elemmenuoptions[k] == SET_INSMIX) {
	        // insert current mix into bin
	        {
	            std::lock_guard<std::mutex> lock(mainprogram->pathmutex);
	            mainprogram->paths.clear();
	        }

	        std::string path = find_unused_filename("mix", mainprogram->temppath, ".jpg");
	        if (mainprogram->prevmodus) {
	            this->menuelem->tex = copy_tex(mainprogram->nodesmain->mixnodes[0][2]->mixtex, 1920, 1080);
	        }
	        else {
	            this->menuelem->tex = copy_tex(mainprogram->nodesmain->mixnodes[1][2]->mixtex, 1920, 1080);
	        }
	        save_thumb(path, this->menuelem->tex);
	        this->menuelem->type = ELEM_IMAGE;
	        this->menuelem->abspath = path;
	        this->menuelem->name = remove_extension(basename(this->menuelem->abspath));
	        //this->menuelem->oldjpegpath = this->menuelem->jpegpath;
	        std::string jpegpath = path + ".jpeg";
	        save_thumb(jpegpath, this->menuelem->tex);
	    }
	    else if (elemmenuoptions[k] == SET_UPSCALEIMAGE) {
	    	// ai realesrgan upscale image - async
            this->menuelem->upscaleBinel = std::make_unique<BinElement>();
            this->menuelem->upscaleBinel->path = this->menuelem->abspath;
	    	this->menuelem->upscaleBinel->upscale_image_async(mainprogram->menuresults[0]);
        }
        else if (elemmenuoptions[k] == SET_SAVPROJ) {
            // save project
            mainprogram->project->save(mainprogram->project->path);
        }
        else if (elemmenuoptions[k] == SET_QUIT) {
            // quit program
            mainprogram->quitting = "quitted";
        }
        else if (elemmenuoptions[k] == SET_OPENSTYLE) {
            mainprogram->pathto = "OPENSTYLE";
            std::thread filereq(&Program::get_inname, mainprogram, "Open style file", "application/ewocvj2-style", "C:/ProgramData/EWOCvj2/models/styles/setup");
            filereq.detach();
        }
        else if (elemmenuoptions[k] == SET_SAVESTYLE) {
            this->prepbin->save();
        }
        else if (elemmenuoptions[k] == SET_RENAMESTYLE) {
            // start renaming style
            this->backupname = this->currstyle->name;
            mainprogram->inputtext = this->currstyle->name;
            mainprogram->cursorpos0 = mainprogram->inputtext.length();
            SDL_StartTextInput();
            mainprogram->renaming = EDIT_BINNAME;
            this->currstyle->oldname = this->currstyle->name;
        }
        else if (elemmenuoptions[k] == SET_DELETESTYLE) {
            // delete style model or profile
            safe_remove(this->currstyle->abspath);
            this->updatelists();
        }
    }
	    
    draw_box(white, darkgreen2, this->elemboxes[0]->vtxcoords->x1 - border, -0.96f, 1.65f, 0.5f, -1);
    draw_box(white, black, this->stylenamesbox, -1);
    //draw and handle stylelist
    inbox = false;
    //handle styleslist scroll
    if (this->stylenamesbox->in()) {
        // mousewheel scroll
        this->stylesscroll -= mainprogram->mousewheel;
        if (this->stylesscroll < 0) this->stylesscroll = 0;
        if (this->styles.size() > 22 && this->styles.size() - this->stylesscroll < 23) {
            this->stylesscroll = this->styles.size() - 22;
        }
    }

    // draw and handle binslist scrollboxes
    this->stylesscroll = mainprogram->handle_scrollboxes(*this->stylesscrollup, *this->stylesscrolldown, this->styles.size(), this->stylesscroll, 22);

    for (int i = 0; i < 22; i++) {
        Style* style = this->styles[i + this->stylesscroll];
        style->box->vtxcoords->y1 = this->stylenamesbox->vtxcoords->y1 + this->stylenamesbox->vtxcoords->h + (i + 1) * -0.05f;
        style->box->upvtxtoscr();
        if (style->box->in()) {
            inbox = true;
            this->menustyle = style;
            if (mainprogram->renaming == EDIT_NONE) {
                if (mainprogram->leftmouse) {
                    // click to choose current bin
                    this->currstyle = style;
                }
                if (mainprogram->doubleleftmouse) {
                    // start renaming bin
                    this->backupname = this->currstyle->name;
                    mainprogram->inputtext = this->currstyle->name;
                    mainprogram->cursorpos0 = mainprogram->inputtext.length();
                    SDL_StartTextInput();
                    mainprogram->renaming = EDIT_BINNAME;
                    this->currstyle->oldname = this->currstyle->name;
                }
                style->box->acolor[0] = 0.5f;
                style->box->acolor[1] = 0.5f;
                style->box->acolor[2] = 1.0f;
                style->box->acolor[3] = 1.0f;
            }
            if (mainprogram->menuactivation) {
                // Set style list menu
                this->elemmenuoptions.clear();
                std::vector<std::string> snlm;
                snlm.push_back("Delete model");
                elemmenuoptions.push_back(SET_DELETESTYLE);
                snlm.push_back("Rename model");
                elemmenuoptions.push_back(SET_RENAMESTYLE);
                snlm.push_back("Quit");
                elemmenuoptions.push_back(SET_QUIT);

                mainprogram->make_menu("styleroommenu", mainstyleroom->styleroommenu, snlm);

                this->styleroommenu->state = 2;
                this->styleroommenu->menux = mainprogram->mx;
                this->styleroommenu->menuy = mainprogram->my;
            }
        }
        else if (style == this->currstyle) {
            // current style colored differently
            style->box->acolor[0] = 0.4f;
            style->box->acolor[1] = 0.2f;
            style->box->acolor[2] = 0.2f;
            style->box->acolor[3] = 1.0f;
        }
        else {
            style->box->acolor[0] = 0.0f;
            style->box->acolor[1] = 0.0f;
            style->box->acolor[2] = 0.0f;
            style->box->acolor[3] = 1.0f;
        }
        draw_box(style->box, -1);
        std::string namedisplay = style->name;
        if (mainprogram->renaming == EDIT_BINNAME && style == this->currstyle) {
            // bin renaming with keyboard
            do_text_input(style->box->vtxcoords->x1 + 0.025f, style->box->vtxcoords->y1 + 0.018f, 0.00045f, 0.00075f, mainprogram->mx, mainprogram->my, mainprogram->xvtxtoscr(0.3f - 0.03f), 0, nullptr);
        }
        else render_text(namedisplay, white, style->box->vtxcoords->x1 + 0.025f, style->box->vtxcoords->y1 + 0.018f, 0.00045f, 0.00075f);
    }

    if (!inbox && this->stylenamesbox->in()) {
        if (this->menustyle) {
            if (this->menustyle->name != "") {
                // Set menu when over non-empty element
                this->elemmenuoptions.clear();
                std::vector<std::string> snlm;
                snlm.push_back("Delete element");
                elemmenuoptions.push_back(SET_DELETE);
                snlm.push_back("Open file(s) from disk");
                elemmenuoptions.push_back(SET_OPENFILES);
                snlm.push_back("Insert deck A");
                elemmenuoptions.push_back(SET_INSDECKA);
                snlm.push_back("Insert deck B");
                elemmenuoptions.push_back(SET_INSDECKB);
                snlm.push_back("Insert full mix");
                elemmenuoptions.push_back(SET_INSMIX);
                snlm.push_back("Open style profile");
                elemmenuoptions.push_back(SET_OPENSTYLE);
                snlm.push_back("Save style profile");
                elemmenuoptions.push_back(SET_SAVESTYLE);
                snlm.push_back("submenu upscalemenu");
                snlm.push_back("Upscale image");
                elemmenuoptions.push_back(SET_UPSCALEIMAGE);
                snlm.push_back("Quit");
                elemmenuoptions.push_back(SET_QUIT);

                mainprogram->make_menu("styleroommenu", mainstyleroom->styleroommenu, snlm);
            } else {
                // Set menu when over empty element
                this->elemmenuoptions.clear();
                std::vector<std::string> snlm;
                snlm.push_back("Open file(s) from disk");
                elemmenuoptions.push_back(SET_OPENFILES);
                snlm.push_back("Insert deck A");
                elemmenuoptions.push_back(SET_INSDECKA);
                snlm.push_back("Insert deck B");
                elemmenuoptions.push_back(SET_INSDECKB);
                snlm.push_back("Insert full mix");
                elemmenuoptions.push_back(SET_INSMIX);
                snlm.push_back("Open style profile");
                elemmenuoptions.push_back(SET_OPENSTYLE);
                snlm.push_back("Save style profile");
                elemmenuoptions.push_back(SET_SAVESTYLE);
                snlm.push_back("Quit");
                elemmenuoptions.push_back(SET_QUIT);

                mainprogram->make_menu("styleroommenu", mainstyleroom->styleroommenu, snlm);
            }
        }
        if (mainprogram->menuactivation) {
            this->styleroommenu->state = 2;
            this->styleroommenu->menux = mainprogram->mx;
            this->styleroommenu->menuy = mainprogram->my;
        }
    }

    this->res->handle();
    if (this->mode->value == 0.0f) {
        this->quality->handle();
        this->influence->handle();
        this->abstraction->handle();
        this->coherence->handle();
    }
    this->usegpu->handle();
    int height = this->res->value * 9.0f / 16.0f;
    render_text(std::to_string((int)this->res->value) + " x " + std::to_string(height), white, this->res->box->vtxcoords->x1 + this->res->box->vtxcoords->w + 0.015f, this->res->box->vtxcoords->y1 + 0.02f, 0.0006f, 0.00100f);

    draw_box(white, darkgreen2, 0.29f, this->elemboxes[11]->vtxcoords->y1 - border - 0.07f, 0.52f, 0.3f * 4.0f + border * 2.0f, -1);
    this->mode->handle();
    if (this->mode->value == 1.0f) {
        this->layrelu1->handle();
        this->layrelu2->handle();
        this->layrelu3->handle();
        this->layrelu4->handle();
        this->layrelu5->handle();

        this->contentweight->handle();
        this->styleweight->handle();
        this->temporalweight->handle();

        this->trainres->handle();
        this->trainiter->handle();
        render_text(std::to_string(((int)this->trainiter->value) * 1000), white, this->trainiter->box->vtxcoords->x1 + this->trainiter->box->vtxcoords->w + 0.015f, this->trainiter->box->vtxcoords->y1 + 0.02f, 0.0006f, 0.00100f);
        this->batchsize->handle();
    }


    float locx = -0.2f;
    float locy = -0.75f;
    Boxx box;
    box.vtxcoords->x1 = locx;
    box.vtxcoords->y1 = locy;
    box.vtxcoords->w = 0.24f;
    box.vtxcoords->h = 0.10f;
    box.upvtxtoscr();
    draw_box(&box, -1);
    render_text("Style name:", white, box.vtxcoords->x1 - 0.15f, box.vtxcoords->y1 + 0.03f, 0.0009f, 0.00150f);
    if (mainprogram->renamingstyle == false) {
        render_text(mainstyleroom->currstylename, white, box.vtxcoords->x1 + 0.02f, box.vtxcoords->y1 + 0.03f, 0.0009f, 0.00150f);
    } else {
        if (mainprogram->renaming == EDIT_NONE) {
            mainprogram->renamingstyle = false;
            mainstyleroom->currstylename = mainprogram->inputtext;
            std::string modelsdir;
#ifdef _WIN32
            modelsdir = "C:/ProgramData/EWOCvj2/models/styles/";
#else
            modelsdir = "/usr/share/EWOCvj2/models/styles/";
#endif
            std::string path;
            int count = 0;
            while (1) {
                path = modelsdir + mainstyleroom->currstylename + ".onnx";
                if (!exists(path)) {
                    break;
                }
                count++;
                mainstyleroom->currstylename = remove_version(mainstyleroom->currstylename) + "_" + std::to_string(count);
            }
        } else if (mainprogram->renaming == EDIT_CANCEL) {
            mainprogram->renamingstyle = false;
        } else {
            do_text_input(box.vtxcoords->x1 + 0.02f,
                          box.vtxcoords->y1 + 0.03f, 0.00075f, 0.0012f, mainprogram->mx, mainprogram->my,
                          mainprogram->xvtxtoscr(0.3f), 0, nullptr, false);
        }
    }
    if (box.in()) {
        if (mainprogram->leftmouse) {
            mainprogram->leftmouse = false;
            mainprogram->renamingstyle = true;
            mainprogram->renaming = EDIT_STRING;
            mainprogram->inputtext = mainstyleroom->currstylename;
            mainprogram->cursorpos0 = mainprogram->inputtext.length();
            SDL_StartTextInput();
        }
    }

    bool found = false;
    for (auto elem : mainstyleroom->prepbin->elements) {
        if (elem->abspath != "") {
            found = true;
            break;
        }
    }
    if (found)
    {
        box.vtxcoords->x1 = box.vtxcoords->x1;
        box.vtxcoords->y1 = box.vtxcoords->y1 + 0.13f;
        box.vtxcoords->w = 0.12f;
        box.vtxcoords->h = 0.10f;
        box.upvtxtoscr();
        if (mainstyleroom->reconetTrainer->isTraining()) {
            draw_box(white, darkred1, &box, -1);
            render_text("STOP", white, box.vtxcoords->x1 + 0.03f, box.vtxcoords->y1 + 0.03f, 0.0009f, 0.00150f);
            if (box.in()) {
                if (mainprogram->leftmouse) {
                    mainstyleroom->reconetTrainer->stopTraining();
                }
            }
        }
        else {
            draw_box(white, darkgreen1, &box, -1);
            render_text("TRAIN", white, box.vtxcoords->x1 + 0.02f, box.vtxcoords->y1 + 0.03f, 0.0009f, 0.00150f);
            if (box.in()) {
                if (mainprogram->leftmouse) {
                    ReCoNetTrainer::Config config;
                    switch ((int)this->quality->value) {
                        case 0:
                            config.quality = ReCoNetTrainer::Quality::FAST;
                            break;
                        case 1:
                            config.quality = ReCoNetTrainer::Quality::BALANCED_256;
                            break;
                        case 2:
                            config.quality = ReCoNetTrainer::Quality::BALANCED_512;
                            break;
                        case 3:
                            config.quality = ReCoNetTrainer::Quality::HIGH_256;
                            break;
                        case 4:
                            config.quality = ReCoNetTrainer::Quality::HIGH_512;
                            break;
                    }
                    config.useGPU = this->usegpu->value;  // User checkbox

                    // Inspiration image preprocessing resolution (smallest side)
                    config.inspirationResolution = (int)this->res->value;

                    // Content dataset for style transfer training (photos to stylize)
                    // Use COCO, ImageNet, or any folder with diverse photos
#ifdef _WIN32
                    config.contentDataset = "C:/ProgramData/EWOCvj2/datasets/content";
#else
                    config.contentDataset = "/usr/share/EWOCvj2/datasets/content";
#endif

                    // Temporal coherence training (for flicker-free video style transfer)
                    // Set video dataset to a folder with video files or frame sequences
#ifdef _WIN32
                    config.videoDataset = "C:/ProgramData/EWOCvj2/datasets/video";
#else
                    config.videoDataset = "/usr/share/EWOCvj2/datasets/video";
#endif
                    // Enable temporal training (set > 0 before applyStyleInfluence)
                    config.temporalWeight = this->coherence->value;  // Will be adjusted by preset or advanced settings
                    config.sequenceLength = 2;

                    if (this->mode->value == 1.0f) {
                        // advanced options
                        config.styleWeightRelu1 = this->layrelu1->value;
                        config.styleWeightRelu2 = this->layrelu2->value;
                        config.styleWeightRelu3 = this->layrelu3->value;
                        config.styleWeightRelu4 = this->layrelu4->value;
                        config.styleWeightRelu5 = this->layrelu5->value;

                        config.styleWeight = 1e5 * 10.0f * this->styleweight->value;
                        config.contentWeight = this->contentweight->value;
                        config.temporalWeight = 1e3f * this->temporalweight->value;

                        config.resolution = ((int)this->trainres->value + 1) * 256;
                        config.iterations = (int)this->trainiter->value * 1000;
                        config.batchSize = (int)this->batchsize->value;
                    }
                    else {
                        // Style influence preset: MINIMAL, BALANCED, or STRONG
                        switch ((int)this->influence->value) {
                            case 0:
                                config.styleInfluence = ReCoNetTrainer::StyleInfluence::MINIMAL;
                                break;
                            case 1:
                                config.styleInfluence = ReCoNetTrainer::StyleInfluence::BALANCED;
                                break;
                            case 2:
                                config.styleInfluence = ReCoNetTrainer::StyleInfluence::STRONG;
                                break;
                        }
                        // Apply the style influence preset (sets content/style/temporal weights)
                        config.applyStyleInfluence();

                        switch ((int)this->abstraction->value) {
                            case 0:
                                config.abstractionLevel = ReCoNetTrainer::AbstractionLevel::LOW;
                                break;
                            case 1:
                                config.abstractionLevel = ReCoNetTrainer::AbstractionLevel::MEDIUM;
                                break;
                            case 2:
                                config.abstractionLevel = ReCoNetTrainer::AbstractionLevel::HIGH;
                                break;
                        }
                        config.applyAbstractionLevel();
                    }

                    bool started = mainstyleroom->reconetTrainer->startTraining(
                            mainstyleroom->prepbin,
                            mainstyleroom->currstylename,  // Model name from text input
                            config);
                    if (!started) {
                        std::string error = mainstyleroom->reconetTrainer->getLastError();
                        // Show error to user
                        printf("%s\n", error.c_str());
                    }
                }
            }
        }
    }

    if (mainstyleroom->reconetTrainer->isTraining())
    {
	    auto progress = mainstyleroom->reconetTrainer->getProgress();

    	// Update UI:
    	// - Progress bar: progress.currentIteration / progress.totalIterations
    	// - Status text: progress.status
    	// - Loss value: progress.totalLoss
    	// - Percentage: (progress.currentIteration * 100) / progress.totalIterations

    	// Example:
    	int percent = (progress.currentIteration * 100) / progress.totalIterations;
    	std::string statusText1 = progress.status + " " + std::to_string(percent) + "%";
    	std::string statusText2 = "Estimated time remaining: " + std::to_string((int)progress.estimatedTimeRemaining) + "s";
    	render_text(statusText1, white, 0.2f, -0.65f, 0.0009f, 0.00150f);
    	if (percent > 0) {
    		render_text(statusText2, white, 0.2f, -0.75f, 0.0009f, 0.00150f);
    	}
    }

	// load one file into prepbin each loop, at end to allow drawing ordering dialog on top
	if (this->openfilesstyle) {
		open_files_bin();
	}
}

void StyleRoom::open_files_bin() {
	// open videos/images/layer/deck/mix files into bin

	size_t pathsSize;
	std::string str;
	{
		std::lock_guard<std::mutex> lock(mainprogram->pathmutex);
		pathsSize = mainprogram->paths.size();
		if (mainprogram->counting < pathsSize) {
			str = mainprogram->paths[mainprogram->counting];
		}
	}

	if (mainprogram->counting == pathsSize) {
		this->openfilesstyle = false;
		{
			std::lock_guard<std::mutex> lock(mainprogram->pathmutex);
			mainprogram->paths.clear();
		}
		return;
	}

	StylePreparationElement* elem = mainstyleroom->prepbin->elements[this->menuelem->pos + mainprogram->counting];
	GLuint endtex = -1;;
	Layer *lay = new Layer(true);
	get_imagetex(lay, str);
	std::unique_lock<std::mutex> lock2(lay->enddecodelock);
	lay->enddecodevar.wait(lock2, [&] {return lay->processed; });
	lay->processed = false;
	lock2.unlock();
	endtex = mainprogram->get_tex(lay);
	lay->close();

	elem->abspath = str;
	elem->relpath = std::filesystem::relative(this->menuelem->abspath, mainprogram->contentpath).generic_string();
	elem->name = remove_extension(basename(str));
	elem->tex = endtex;
	mainprogram->counting++;
}

void StylePreparationBin::save() {
    // save bin file
    std::ofstream wfile;
    std::string path = "C:/ProgramData/EWOCvj2/models/styles/setup/" + mainstyleroom->currstylename + ".style";
    wfile.open(path.c_str());
    wfile << "EWOCvj STYLEFILE\n";

    wfile << "ELEMS\n";
    // save elements
    for (int i = 0; i < 12; i++) {
        StylePreparationElement *elem = this->elements[i];
        wfile << "ABSPATH\n";
        wfile << elem->abspath;
        wfile << "\n";
        wfile << "RELPATH\n";
        if (elem->relpath == "")  {
            elem->relpath = std::filesystem::relative(elem->abspath, mainprogram->contentpath).generic_string();
        }
        wfile << elem->relpath;
        wfile << "\n";
        wfile << "NAME\n";
        wfile << elem->name;
        wfile << "\n";
        if (elem->abspath != "") {
            wfile << "FILESIZE\n";
            wfile << std::to_string(std::filesystem::file_size(elem->abspath));
            wfile << "\n";
        }
    }
    wfile << "ENDOFELEMS\n";

    wfile << "MODE\n";
    wfile << std::to_string(mainstyleroom->mode->value);
    wfile << "\n";
    wfile << "RES\n";
    wfile << std::to_string(mainstyleroom->res->value);
    wfile << "\n";
    wfile << "USEGPU\n";
    wfile << std::to_string(mainstyleroom->usegpu->value);
    wfile << "\n";
    wfile << "QUALITY\n";
    wfile << std::to_string(mainstyleroom->quality->value);
    wfile << "\n";
    wfile << "INFLUENCE\n";
    wfile << std::to_string(mainstyleroom->influence->value);
    wfile << "\n";
    wfile << "ABSTRACTION\n";
    wfile << std::to_string(mainstyleroom->abstraction->value);
    wfile << "\n";
    wfile << "COHERENCE\n";
    wfile << std::to_string(mainstyleroom->coherence->value);
    wfile << "\n";
    wfile << "LAYRELU1\n";
    wfile << std::to_string(mainstyleroom->layrelu1->value);
    wfile << "\n";
    wfile << "LAYRELU2\n";
    wfile << std::to_string(mainstyleroom->layrelu2->value);
    wfile << "\n";
    wfile << "LAYRELU3\n";
    wfile << std::to_string(mainstyleroom->layrelu3->value);
    wfile << "\n";
    wfile << "LAYRELU4\n";
    wfile << std::to_string(mainstyleroom->layrelu4->value);
    wfile << "\n";
    wfile << "LAYRELU5\n";
    wfile << std::to_string(mainstyleroom->layrelu5->value);
    wfile << "\n";
    wfile << "STYLEWEIGHT\n";
    wfile << std::to_string(mainstyleroom->styleweight->value);
    wfile << "\n";
    wfile << "CONTENTWEIGHT\n";
    wfile << std::to_string(mainstyleroom->contentweight->value);
    wfile << "\n";
    wfile << "TEMPORALWEIGHT\n";
    wfile << std::to_string(mainstyleroom->temporalweight->value);
    wfile << "\n";
    wfile << "TRAINRES\n";
    wfile << std::to_string(mainstyleroom->trainres->value);
    wfile << "\n";
    wfile << "TRAINITER\n";
    wfile << std::to_string(mainstyleroom->trainiter->value);
    wfile << "\n";
    wfile << "BATCHSIZE\n";
    wfile << std::to_string(mainstyleroom->batchsize->value);
    wfile << "\n";

    wfile << "ENDOFFILE\n";
    wfile.close();
}

void StylePreparationBin::open(std::string path) {
    // open a style preparation project
    mainstyleroom->currstylename = remove_extension(basename(path));

    std::ifstream rfile;
    rfile.open(path);

    int count = 0;
    std::string abspath;
    std::string istring;
    safegetline(rfile, istring);
    //check if binfile
    while (safegetline(rfile, istring)) {
        if (istring == "ENDOFFILE") {
            break;
        }
        else if (istring == "ELEMS") {
            // open style preparation elements
            while (safegetline(rfile, istring)) {
                StylePreparationElement *elem = this->elements[count];
                bool notfound = false;
                if (istring == "ENDOFELEMS") break;
                if (istring == "ABSPATH") {
                    safegetline(rfile, istring);
                    elem->abspath = istring;
                    elem->relpath = std::filesystem::relative(istring, mainprogram->contentpath).generic_string();
                }
                if (istring == "RELPATH") {
                    safegetline(rfile, istring);
                    if (istring == "" && elem->abspath == "") {
                        continue;
                    }
                    if (istring != "" && elem->abspath == "" && exists(istring)) {
                        elem->abspath = pathtoplatform(std::filesystem::absolute(istring).generic_string());
                        elem->relpath = istring;
                    }
                    if (!exists(elem->abspath)) {
                        auto teststr = test_driveletters(elem->abspath);
                        if (teststr == "") {
                            notfound = true;
                            mainmix->retargeting = true;
                            mainmix->newstyleimagepaths.push_back(elem->abspath);
                            mainmix->newpathstylelems.push_back(elem);
                        }
                        else {
                            elem->abspath = teststr;
                        }
                    }
                }
                if (istring == "NAME") {
                    safegetline(rfile, istring);
                    elem->name = istring;
                }
                if (istring == "FILESIZE") {
                    safegetline(rfile, istring);
                    elem->filesize = std::stoll(istring);
                }
            }
        }
        else if (istring == "MODE") {
            mainstyleroom->mode->value = std::stof(istring);
        }
        else if (istring == "RES") {
            mainstyleroom->res->value = std::stof(istring);
        }
        else if (istring == "USEGPU") {
            mainstyleroom->usegpu->value = std::stof(istring);
        }
        else if (istring == "QUALITY") {
            mainstyleroom->quality->value = std::stof(istring);
        }
        else if (istring == "INFLUENCE") {
            mainstyleroom->influence->value = std::stof(istring);
        }
        else if (istring == "ABSTRACTION") {
            mainstyleroom->abstraction->value = std::stof(istring);
        }
        else if (istring == "COHERENCE") {
            mainstyleroom->coherence->value = std::stof(istring);
        }
        else if (istring == "LAYRELU1") {
            mainstyleroom->layrelu1->value = std::stof(istring);
        }
        else if (istring == "LAYRELU2") {
            mainstyleroom->layrelu2->value = std::stof(istring);
        }
        else if (istring == "LAYRELU3") {
            mainstyleroom->layrelu3->value = std::stof(istring);
        }
        else if (istring == "LAYRELU4") {
            mainstyleroom->layrelu4->value = std::stof(istring);
        }
        else if (istring == "LAYRELU5") {
            mainstyleroom->layrelu5->value = std::stof(istring);
        }
        else if (istring == "STYLEWEIGHT") {
            mainstyleroom->styleweight->value = std::stof(istring);
        }
        else if (istring == "CONTENTWEIGHT") {
            mainstyleroom->contentweight->value = std::stof(istring);
        }
        else if (istring == "TEMPORALWEIGHT") {
            mainstyleroom->temporalweight->value = std::stof(istring);
        }
        else if (istring == "TRAINRES") {
            mainstyleroom->trainres->value = std::stof(istring);
        }
        else if (istring == "TRAINITER") {
            mainstyleroom->trainiter->value = std::stof(istring);
        }
        else if (istring == "BATCHSIZE") {
            mainstyleroom->batchsize->value = std::stof(istring);
        }
    }

    rfile.close();
}

void StyleRoom::updatelists() {
    AIStyleTransfer tempStyleTransfer;
    std::string modelsPath;
#ifdef _WIN32
    modelsPath = "C:/ProgramData/EWOCvj2/models/styles/";
#else
    modelsPath = "/usr/share/EWOCvj2/models/styles/";
#endif
    mainprogram->aistylenames.clear();
    mainprogram->aistylepaths.clear();
    if (tempStyleTransfer.initialize()) {
        int numStyles = tempStyleTransfer.loadStyles(modelsPath);
        if (numStyles > 0) {
            auto styleNames = tempStyleTransfer.getAvailableStyles();
            for (auto &name : styleNames) {
                std::transform(name.begin(), name.end(), name.begin(), ::toupper);
                mainprogram->aistylenames.push_back(name);
            }
            std::cerr << "[ReCoNetTrainer] Loaded " << numStyles << " AI style models" << std::endl;
        }
    } else {
        std::cerr << "[ReCoNetTrainer] Failed to initialize temp AIStyleTransfer for style reload" << std::endl;
    }

    mainprogram->create_stylemenu();

    for (auto style : this->styles) {
        delete style->box;
        delete style;
    }
    this->styles.clear();
    for (int i = 0; i < mainprogram->aistylenames.size(); i++) {
        Style* style = new Style;
        style->name = mainprogram->aistylenames[i];
        style->abspath = mainprogram->aistylepaths[i];
        style->box = new Boxx;
        style->box->vtxcoords->x1 = -0.04f;
        style->box->vtxcoords->w = 0.248f;
        style->box->vtxcoords->h = this->stylenamesbox->vtxcoords->h / 20.0f;
        this->styles.push_back(style);
    }
}