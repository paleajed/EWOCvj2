#include <boost/filesystem.hpp>

#include "GL/glew.h"
#include "GL/gl.h"
#include "GL/glut.h"
#ifdef __linux__
#include "GL/glx.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include "paths.h"
#endif

#include "SDL2/SDL.h"
#include "SDL2/SDL_syswm.h"

#include "tinyfiledialogs.h"

#include <istream>
#include <ostream>
#include <iostream>
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

void Program::make_menu(const std::string &name, Menu *&menu, std::vector<std::string> &entries) {
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

std::string Program::mime_to_wildcard(std::string filters) {
	if (filters == "") return "";
	if (filters == "application/ewocvj2-layer") return "*.layer";
	if (filters == "application/ewocvj2-deck") return "*.deck";
	if (filters == "application/ewocvj2-mix") return "*.mix";
	if (filters == "application/ewocvj2-state") return "*.state";
	if (filters == "application/ewocvj2-shelf") return "*.shelf";
}

void Program::get_inname(const char *title, std::string filters, std::string defaultdir) {
	char const* const dd = (defaultdir == "") ? "" : defaultdir.c_str();
	#ifdef _WIN64
	filters = this->mime_to_wildcard(filters);
	#endif
	const char* fi[1];
	fi[0] = filters.c_str();
	if (fi[0] == "") {
		this->path = tinyfd_openFileDialog(title, dd, 0, nullptr, NULL, 0);
	}
	else {
		this->path = tinyfd_openFileDialog(title, dd, 1, fi, NULL, 0);
	}
}

void Program::get_outname(const char *title, std::string filters, std::string defaultdir) {
	char const* const dd = (defaultdir == "") ? "" : defaultdir.c_str();
	#ifdef _WIN64
	filters = this->mime_to_wildcard(filters);
	#endif
	const char* fi[1];
	fi[0] = filters.c_str();
	if (fi[0] == "") {
		this->path = tinyfd_saveFileDialog(title, dd, 0, nullptr, NULL);
	}
	else {
		this->path = tinyfd_saveFileDialog(title, dd, 1, fi, NULL);
	}
}

void Program::get_multinname(const char* title) {
	const char *outpaths;
	outpaths = tinyfd_openFileDialog(title, "", 0, NULL, NULL, 1);
	std::string opaths(outpaths);
	this->paths.push_back("");
	std::string currstr = this->paths.back();
	for (int i = 0; i < opaths.length(); i++) {
		std::string charstr;
		charstr = opaths[i];
		if (charstr == "|") {
			this->paths.push_back("");
			std::string currstr = this->paths.back();
			continue;
		}
		currstr += charstr;
	}
	this->path = (char*)"ENTER";
	this->counting = 0;
}

void Program::get_dir(const char *title) {
	this->path = tinyfd_selectFolderDialog(title, "") ;
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
void Program::quit(std::string msg)
{
	//empty temp dir
	boost::filesystem::path path_to_remove(mainprogram->temppath);
	for (boost::filesystem::directory_iterator end_dir_it, it(path_to_remove); it!=end_dir_it; ++it) {
		boost::filesystem::remove_all(it->path());
	}
	printf("%s: %s\n", msg.c_str(), SDL_GetError());
    printf("stopped\n");

    SDL_Quit();
    exit(1);
}

void Program::prevvid_off() {
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

void Program::preveff_init() {
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

unsigned long getFileLength(std::ifstream& file)
{
    if(!file.good()) return 0;

    unsigned long pos=file.tellg();
    file.seekg(0,std::ios::end);
    unsigned long len = file.tellg();
    file.seekg(std::ios::beg);

    return len;
}

int Program::load_shader(char* filename, char** ShaderSource, unsigned long len)
{
   std::ifstream file;
   file.open(filename, std::ios::in); // opens as ASCII!
   if(!file) return -1;

   len = getFileLength(file);

   if (len==0) return -2;   // Error: Empty File

   *ShaderSource = (char*)malloc(len+1);
   if (*ShaderSource == 0) return -3;   // can't reserve memory

    // len isn't always strlen cause some characters are stripped in ascii read...
    // it is important to 0-terminate the real length later, len is just max possible value...
   (*ShaderSource)[len] = 0;

   unsigned int i=0;
   while (file.good())
   {
       (*ShaderSource)[i] = file.get();       // get character from file.
       if (!file.eof())
        i++;
   }

   (*ShaderSource)[i] = 0;  // 0-terminate it at the correct position

   file.close();

   return 0; // No Error
}

GLuint Program::set_shader() {
	GLuint program;
	GLuint vertexShaderObject = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderObject = glCreateShader(GL_FRAGMENT_SHADER);
	unsigned long vlen, flen;
	char *VShaderSource;
 	char *vshader = (char*)malloc(100);
 	#ifdef _WIN64
 	if (exists("./shader.vs")) strcpy (vshader, "./shader.vs");
 	else mainprogram->quit("Unable to find vertex shader \"shader.vs\" in current directory");
 	#else
 	#ifdef __linux__
 	std::string ddir (DATADIR);
 	if (exists("./shader.vs")) strcpy (vshader, "./shader.vs");
 	else if (exists(ddir + "/shader.vs")) strcpy (vshader, (ddir + "/shader.vs").c_str());
 	else mainprogram->quit("Unable to find vertex shader \"shader.vs\" in " + ddir);
 	#endif
 	#endif
 	load_shader(vshader, &VShaderSource, vlen);
	char *FShaderSource;
 	char *fshader = (char*)malloc(100);
 	#ifdef _WIN64
 	if (exists("./shader.fs")) strcpy (fshader, "./shader.fs");
 	else mainprogram->quit("Unable to find fragment shader \"shader.fs\" in current directory");
 	#else
 	#ifdef __linux__
 	if (exists("./shader.fs")) strcpy (fshader, "./shader.fs");
 	else if (exists(ddir + "/shader.fs")) strcpy (fshader, (ddir + "/shader.fs").c_str());
 	else mainprogram->quit("Unable to find fragment shader \"shader.fs\" in " + ddir);
 	#endif
 	#endif
	load_shader(fshader, &FShaderSource, flen);
	glShaderSource(vertexShaderObject, 1, &VShaderSource, NULL);
	glShaderSource(fragmentShaderObject, 1, &FShaderSource, NULL);
	glCompileShader(vertexShaderObject);
	glCompileShader(fragmentShaderObject);

	GLint maxLength = 0;
	glGetShaderiv(fragmentShaderObject, GL_INFO_LOG_LENGTH, &maxLength);
 	GLchar *infolog = (GLchar*)malloc(maxLength);
	glGetShaderInfoLog(fragmentShaderObject, maxLength, &maxLength, &(infolog[0]));
	printf("compile log %s\n", infolog);

	program = glCreateProgram();
	glBindAttribLocation(program, 0, "Position");
	glBindAttribLocation(program, 1, "TexCoord");
	glAttachShader(program, vertexShaderObject);
	glAttachShader(program, fragmentShaderObject);
	glLinkProgram(program);

	maxLength = 1024;
 	infolog = (GLchar*)malloc(maxLength);
	glGetProgramInfoLog(program, maxLength, &maxLength, &(infolog[0]));
	printf("linker log %s\n", infolog);

	GLint isLinked = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
	printf("log %d\n", isLinked);
	fflush(stdout);
	
	return program;
}



