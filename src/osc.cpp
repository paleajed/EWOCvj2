#include "GL/glew.h"
#include "GL/gl.h"
#include "GL/glut.h"

#include <string>

// my own headers
#include "box.h"
#include "effect.h"
#include "node.h"
#include "layer.h"
#include "window.h"
#include "program.h"
#include "loopstation.h"
#include "bins.h"


void preview_wipetype(lo_arg **argv, int argc) {
	
}

void Program::add_main_oscmethods() {
	// crossfading
	mainprogram->st->add_method("/mix/crossfade_all", "f",[](lo_arg **argv, float){mainmix->crossfade->value = argv[0]->f; mainmix->crossfadecomp->value = argv[0]->f;});
	mainprogram->st->add_method("/mix/preview/crossfade", "f",[](lo_arg **argv, float){mainmix->crossfade->value = argv[0]->f;});
	mainprogram->st->add_method("/mix/output/crossfade", "f",[](lo_arg **argv, float){mainmix->crossfadecomp->value = argv[0]->f;});
	// output recording
	mainprogram->st->add_method("/mix/output/record", "s",[](const char* path, const lo::Message &msg){if (&msg.argv()[0]->s == "start") mainmix->start_recording(); else if (&msg.argv()[0]->s == "stop") mainmix->recording = false;});
	mainprogram->st->add_method("/mix/output/record", "i",[](lo_arg **argv, int){if (argv[0]->i) mainmix->start_recording(); else mainmix->recording = false;});
	// main wipes
	mainprogram->st->add_method("/mix/preview/wipe/type", "s", [](const char* path, const lo::Message &msg){mainmix->wipe[0] = mainprogram->wipesmap[&msg.argv()[0]->s];});
	mainprogram->st->add_method("/mix/preview/wipe/type", "i", [](lo_arg **argv, int){mainmix->wipe[0] = argv[0]->i;});
	mainprogram->st->add_method("/mix/preview/wipe/dir", "i", [](lo_arg **argv, int){mainmix->wipedir[0] = argv[0]->i;});
	mainprogram->st->add_method("/mix/preview/wipe/xpos", "i", [](lo_arg **argv, int){mainmix->wipex[0]->value = argv[0]->i;});
	mainprogram->st->add_method("/mix/preview/wipe/ypos", "i", [](lo_arg **argv, int){mainmix->wipey[0]->value = argv[0]->i;});
	mainprogram->st->add_method("/mix/output/wipe/type", "s", [](const char* path, const lo::Message &msg){mainmix->wipe[1] = mainprogram->wipesmap[&msg.argv()[0]->s];});
	mainprogram->st->add_method("/mix/output/wipe/type", "i", [](lo_arg **argv, int){mainmix->wipe[1] = argv[0]->i;});
	mainprogram->st->add_method("/mix/output/wipe/direction", "i", [](lo_arg **argv, int){mainmix->wipedir[1] = argv[0]->i;});
	mainprogram->st->add_method("/mix/output/wipe/xpos", "i", [](lo_arg **argv, int){mainmix->wipex[1]->value = argv[0]->i;});
	mainprogram->st->add_method("/mix/output/wipe/ypos", "i", [](lo_arg **argv, int){mainmix->wipey[1]->value = argv[0]->i;});
	// makelive, back, previewmodes
	mainprogram->st->add_method("/mix/makelive", "", [](){mainmix->copy_to_comp(mainmix->layersA, mainmix->layersAcomp, mainmix->layersB, mainmix->layersBcomp, mainprogram->nodesmain->currpage->nodes, mainprogram->nodesmain->currpage->nodescomp, mainprogram->nodesmain->mixnodescomp, true);});
	mainprogram->st->add_method("/mix/back", "", [](){mainmix->copy_to_comp(mainmix->layersAcomp, mainmix->layersA, mainmix->layersBcomp, mainmix->layersB, mainprogram->nodesmain->currpage->nodescomp, mainprogram->nodesmain->currpage->nodes, mainprogram->nodesmain->mixnodes, false);});
	mainprogram->st->add_method("/mix/effectpreview", "i",[](lo_arg **argv, int){mainprogram->prevmodus = argv[0]->i; mainprogram->preview_init();});
}
