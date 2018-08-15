#include <boost/filesystem.hpp>

#include "GL\glew.h"
#include "GL\gl.h"
#include "GL\glut.h"

#include "nfd.h"

// my own headers
#include "box.h"
#include "effect.h"
#include "node.h"
#include "layer.h"
#include "window.h"
#include "program.h"
#include "loopstation.h"
#include "bins.h"

Program::make_menu(const std::string &name, Menu *&menu, std::vector<std::string> &entries) {
	bool found = false;
	for (int i = 0; i < mainprogram->menulist.size(); i++) {
		if (mainprogram->menulist[i]->name == name) {
			found = true;
			break;
		}
	}
	if (!found) {
		menu = new Menu;
		mainprogram->menulist.push_back(menu);
		menu->name = name;
	}
	menu->entries = entries;
	Box *box = new Box;
	menu->box = box;
	menu->box->scrcoords->x1 = 0;
	menu->box->scrcoords->y1 = mainprogram->yvtxtoscr(tf(0.05f));
	menu->box->scrcoords->w = mainprogram->xvtxtoscr(tf(0.156f));
	menu->box->scrcoords->h = mainprogram->yvtxtoscr(tf(0.05f));
	menu->box->upscrtovtx();
}

Program::get_inname(const nfdchar_t *filters, const nfdchar_t *defaultdir) {
	nfdchar_t *outPath = NULL;
	nfdresult_t result = NFD_OpenDialog(filters, defaultdir, &outPath);
	if (!(result == NFD_OKAY)) {
		return 0;
	}
	mainprogram->path = (char *)outPath;
}

Program::get_multinname() {
	nfdpathset_t outPaths;
	nfdresult_t result = NFD_OpenDialogMultiple(NULL, NULL, &outPaths);
	if (!(result == NFD_OKAY)) {
		return 0;
	}
	mainprogram->paths = outPaths;
	mainprogram->path = (char*)"ENTER";
	mainprogram->counting = 0;
}

Program::get_dir() {
	nfdchar_t *outPath = NULL;
	nfdresult_t result = NFD_PickFolder(NULL, &outPath);
	if (!(result == NFD_OKAY)) {
		return 0;
	}
	mainprogram->path = (char *)outPath;
}

Program::get_outname(const nfdchar_t *filters, const nfdchar_t *defaultdir) {
	nfdchar_t *outPath = NULL;
	nfdresult_t result = NFD_SaveDialog(filters, defaultdir, &outPath);
	if (!(result == NFD_OKAY)) {
		return 0;
	}
	mainprogram->path = (char *)outPath;
}

float Program::xscrtovtx(float scrcoord) {
	return (scrcoord * 2.0 / (float)glob->w);
}

float Program::yscrtovtx(float scrcoord) {
	return (scrcoord * 2.0 / (float)glob->h);
}

float Program::xvtxtoscr(float vtxcoord) {
	return (vtxcoord * (float)glob->w / 2.0);
}

float Program::yvtxtoscr(float vtxcoord) {
	return (vtxcoord * (float)glob->h / 2.0);
}

/* A simple function that prints a message, the error code returned by SDL,
 * and quits the application */
Program::quit(const char *msg)
{
	//empty temp dir
	boost::filesystem::path path_to_remove(mainprogram->temppath);
	for (boost::filesystem::directory_iterator end_dir_it, it(path_to_remove); it!=end_dir_it; ++it) {
		boost::filesystem::remove_all(it->path());
	}
	printf("%s: %s\n", msg, SDL_GetError());
    printf("stopped\n");

    SDL_Quit();
    exit(1);
}

Program::prevvid_off() {
	for (int i = 0; i < mainmix->layersA.size(); i++) {
		Layer *lay = mainmix->layersA[i];
		Layer *laycomp = mainmix->layersAcomp[i];
		if (lay->filename != "" and !prevvid) {
			// initialize throughput when off
			laycomp->speed->value = lay->speed->value;
			laycomp->playbut->value = lay->playbut->value;
			laycomp->revbut->value = lay->revbut->value;
			laycomp->bouncebut->value = lay->bouncebut->value;
			laycomp->playkind = lay->playkind;
			laycomp->genmidibut->value = lay->genmidibut->value;
			laycomp->startframe = lay->startframe;
			laycomp->endframe = lay->endframe;
			laycomp->audioplaying = false;
			open_video(lay->frame, laycomp, lay->filename, false);
		}
	}
	for (int i = 0; i < mainmix->layersB.size(); i++) {
		Layer *lay = mainmix->layersB[i];
		Layer *laycomp = mainmix->layersBcomp[i];
		if (lay->filename != "" and !prevvid) {
			// initialize throughput when off
			laycomp->speed->value = lay->speed->value;
			laycomp->playbut->value = lay->playbut->value;
			laycomp->revbut->value = lay->revbut->value;
			laycomp->bouncebut->value = lay->bouncebut->value;
			laycomp->playkind = lay->playkind;
			laycomp->genmidibut->value = lay->genmidibut->value;
			laycomp->startframe = lay->startframe;
			laycomp->endframe = lay->endframe;
			laycomp->audioplaying = false;
			open_video(lay->frame, laycomp, lay->filename, false);
		}
	}
}

Program::preveff_init() {
	std::vector<Layer*> &lvec = choose_layers(mainmix->currlay->deck);
	if (!this->preveff and !this->prevvid) {
		// when prevvid, normally all comp layers are copied from normal layers
		// but when going into performance mode, its more coherent to open them in comp layers
		for (int i = 0; i < mainmix->layersA.size(); i++) {
			Layer *lay = mainmix->layersA[i];
			if (lay->filename != "") {
				Layer *laycomp = mainmix->layersAcomp[i];
				laycomp->audioplaying = false;
				open_video(lay->frame, laycomp, lay->filename, true);
			}
		}
		for (int i = 0; i < mainmix->layersB.size(); i++) {
			Layer *lay = mainmix->layersB[i];
			if (lay->filename != "") {
				Layer *laycomp = mainmix->layersBcomp[i];
				laycomp->audioplaying = false;
				open_video(lay->frame, laycomp, lay->filename, true);
			}
		}
	}
	int p = mainmix->currlay->pos;
	if (p > lvec.size() - 1) p = lvec.size() - 1;
	mainmix->currlay = lvec[p];
	GLint preff = glGetUniformLocation(this->ShaderProgram, "preff");
	glUniform1i(preff, this->preveff);
}
