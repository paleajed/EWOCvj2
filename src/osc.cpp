#include "GL/glew.h"
#include "GL/gl.h"
#define FREEGLUT_STATIC
#define _LIB
#define FREEGLUT_LIB_PRAGMAS 0
#include "GL/freeglut.h"

#include <string>

// my own header
#include "program.h"


void preview_wipetype(lo_arg **argv, int argc) {
	
}

void Program::add_main_oscmethods() {
	// crossfading
	mainprogram->st->add_method("/mix/crossfade_all", "f",[](lo_arg **argv, float){mainmix->crossfade->value = argv[0]->f; mainmix->crossfadecomp->value = argv[0]->f;});
	mainprogram->st->add_method("/mix/preview/crossfade", "f",[](lo_arg **argv, float){mainmix->crossfade->value = argv[0]->f;});
	mainprogram->st->add_method("/mix/output/crossfade", "f",[](lo_arg **argv, float){mainmix->crossfadecomp->value = argv[0]->f;});
	// output recording
    mainprogram->st->add_method("/mix/output/recordS", "s",[](const char* path, const lo::Message &msg){if (&msg.argv()[0]->s == "start") mainmix->start_recording(); else if (&msg.argv()[0]->s == "stop") mainmix->recording[0] = false;});
    mainprogram->st->add_method("/mix/output/recordS", "i",[](lo_arg **argv, int){if (argv[0]->i) mainmix->start_recording(); else mainmix->recording[0] = false;});
    mainprogram->st->add_method("/mix/output/recordQ", "s",[](const char* path, const lo::Message &msg){if (&msg.argv()[0]->s == "start") mainmix->start_recording(); else if (&msg.argv()[0]->s == "stop") mainmix->recording[1] = false;});
    mainprogram->st->add_method("/mix/output/recordQ", "i",[](lo_arg **argv, int){if (argv[0]->i) mainmix->start_recording(); else mainmix->recording[1] = false;});
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
	mainprogram->st->add_method("/mix/makelive", "", [](){mainmix->copy_to_comp(true, true, true);});
	mainprogram->st->add_method("/mix/back", "", [](){mainmix->copy_to_comp(true, true, false);});
	mainprogram->st->add_method("/mix/effectpreview", "i",[](lo_arg **argv, int){mainprogram->prevmodus = argv[0]->i;});
}
