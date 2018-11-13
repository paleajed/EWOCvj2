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

Program::Program() {
	#ifdef _WIN64
	this->temppath = "./temp/";
	#else
	#ifdef __linux__
	std::string homedir (getenv("HOME"));
	this->temppath = homedir + "/.ewocvj2/temp/";
	#endif
	#endif
	
	this->numh = this->numh * glob->w / glob->h;
	
	for (int i = 0; i < 6; i++) {
		for (int j = 0; j < 24; j++) {
			Box *box = new Box;
			this->elemboxes.push_back(box);
			box->vtxcoords->x1 = -0.95f + i * 0.12f + (1.2f * (j > 11));
			box->vtxcoords->y1 = 0.95f - ((j % 12) + 1) * 0.15f;
			box->vtxcoords->w = 0.1f;
			box->vtxcoords->h = 0.1f;
			box->upvtxtoscr();
			box->lcolor[0] = 0.4f;
			box->lcolor[1] = 0.4f;
			box->lcolor[2] = 0.4f;
			box->lcolor[3] = 1.0f;
		}
	}
	
	this->cwbox = new Box;

	this->toscreen = new Button(false);
	this->buttons.push_back(this->toscreen);
	this->toscreen->name[0] = "SEND";
	this->toscreen->name[1] = "SEND";
	this->toscreen->box->vtxcoords->x1 = -0.3;
	this->toscreen->box->vtxcoords->y1 = -0.3;
	this->toscreen->box->vtxcoords->w = 0.3 / 2.0;
	this->toscreen->box->vtxcoords->h = 0.3 / 3.0;
	this->toscreen->box->upvtxtoscr();
	
	this->backtopre = new Button(false);
	this->buttons.push_back(this->backtopre);
	this->backtopre->name[0] = "SEND";
	this->backtopre->name[1] = "SEND";
	this->backtopre->box->vtxcoords->x1 = -0.3;
	this->backtopre->box->vtxcoords->y1 = -0.4;
	this->backtopre->box->vtxcoords->w = 0.15f;
	this->backtopre->box->vtxcoords->h = 0.1f;
	this->backtopre->box->upvtxtoscr();
	
	this->modusbut = new Button(false);
	this->buttons.push_back(this->modusbut);
	this->modusbut->name[0] = "LIVE MODUS";
	this->modusbut->name[1] = "PREVIEW MODUS";
	this->modusbut->box->vtxcoords->x1 = -0.3 - (0.3 / 2.0);
	this->modusbut->box->vtxcoords->y1 = -0.7;
	this->modusbut->box->vtxcoords->w = 0.3 / 2.0;
	this->modusbut->box->vtxcoords->h = 0.3 / 3.0;
	this->modusbut->box->upvtxtoscr();
	this->modusbut->tcol[0] = 0.0f;
	this->modusbut->tcol[1] = 0.0f;
	this->modusbut->tcol[2] = 0.0f;
	this->modusbut->tcol[3] = 1.0f;
	
	this->deckspeed[0] = new Param;
	this->deckspeed[0]->name = "Speed A"; 
	this->deckspeed[0]->value = 1.0f;
	this->deckspeed[0]->range[0] = 0.0f;
	this->deckspeed[0]->range[1] = 3.33f;
	this->deckspeed[0]->sliding = true;
	this->deckspeed[0]->powertwo = true;
	this->deckspeed[0]->box->vtxcoords->x1 = -0.3 - (0.3 / 2.0) - 0.15f;
	this->deckspeed[0]->box->vtxcoords->y1 = -0.7;
	this->deckspeed[0]->box->vtxcoords->w = 0.3 / 2.0;
	this->deckspeed[0]->box->vtxcoords->h = 0.3 / 3.0;
	this->deckspeed[0]->box->upvtxtoscr();
	
	this->deckspeed[1] = new Param;
	this->deckspeed[1]->name = "Speed B"; 
	this->deckspeed[1]->value = 1.0f;
	this->deckspeed[1]->range[0] = 0.0f;
	this->deckspeed[1]->range[1] = 3.33f;
	this->deckspeed[1]->sliding = true;
	this->deckspeed[1]->powertwo = true;
	this->deckspeed[1]->box->vtxcoords->x1 = -0.3 - (0.3 / 2.0) + 0.9f;
	this->deckspeed[1]->box->vtxcoords->y1 = -0.7;
	this->deckspeed[1]->box->vtxcoords->w = 0.3 / 2.0;
	this->deckspeed[1]->box->vtxcoords->h = 0.3 / 3.0;
	this->deckspeed[1]->box->upvtxtoscr();
	
	this->effscrollupA = new Box;
	this->effscrollupA->vtxcoords->x1 = -1.0;
	this->effscrollupA->vtxcoords->y1 = 1.0 - tf(this->layw) - tf(0.20f);
	this->effscrollupA->vtxcoords->w = tf(0.025f);
	this->effscrollupA->vtxcoords->h = tf(0.05f);
	this->effscrollupA->upvtxtoscr();
	
	this->effcat[0] = new Button(false);
	this->buttons.push_back(this->effcat[0]);
	this->effcat[0]->name[0] = "Layer effects";
	this->effcat[0]->name[1] = "Stream effects";
	this->effcat[0]->box->vtxcoords->x1 = -1.0f + this->numw - tf(0.025f);
	this->effcat[0]->box->vtxcoords->y1 = 1.0f - tf(this->layw) - tf(0.50f);
	this->effcat[0]->box->vtxcoords->w = tf(0.025f);
	this->effcat[0]->box->vtxcoords->h = tf(0.2f);
	this->effcat[0]->box->upvtxtoscr();
	
	this->effcat[1] = new Button(false);
	this->buttons.push_back(this->effcat[1]);
	this->effcat[1]->name[0] = "Layer effects";
	this->effcat[1]->name[1] = "Stream effects";
	float xoffset = 1.0f + this->layw - 0.019f;
	this->effcat[1]->box->vtxcoords->x1 = -1.0f + this->numw - tf(0.025f) + xoffset;
	this->effcat[1]->box->vtxcoords->y1 = 1.0f - tf(this->layw) - tf(0.50f);
	this->effcat[1]->box->vtxcoords->w = tf(0.025f);
	this->effcat[1]->box->vtxcoords->h = tf(0.2f);
	this->effcat[1]->box->upvtxtoscr();
	
	this->effscrollupB = new Box;
	this->effscrollupB->vtxcoords->x1 = 1.0 - tf(0.05f);
	this->effscrollupB->vtxcoords->y1 = 1.0 - tf(this->layw) - tf(0.20f);
	this->effscrollupB->vtxcoords->w = tf(0.025f);
	this->effscrollupB->vtxcoords->h = tf(0.05f);     
	this->effscrollupB->upvtxtoscr();
	
	this->effscrolldownA = new Box;
	this->effscrolldownA->vtxcoords->x1 = -1.0;
	this->effscrolldownA->vtxcoords->y1 = 1.0 - tf(this->layw) - tf(0.20f) - tf(0.05f) * 10;
	this->effscrolldownA->vtxcoords->w = tf(0.025f);
	this->effscrolldownA->vtxcoords->h = tf(0.05f);
	this->effscrolldownA->upvtxtoscr();
	
	this->effscrolldownB = new Box;
	this->effscrolldownB->vtxcoords->x1 = 1.0 - tf(0.05f);
	this->effscrolldownB->vtxcoords->y1 = 1.0 - tf(this->layw) - tf(0.20f) - tf(0.05f) * 10;
	this->effscrolldownB->vtxcoords->w = tf(0.025f);
	this->effscrolldownB->vtxcoords->h = tf(0.05f);
	this->effscrolldownB->upvtxtoscr();
	
	this->tmplay = new Box;
	this->tmplay->vtxcoords->x1 = 0.075;
	this->tmplay->vtxcoords->y1 = -0.9f;
	this->tmplay->vtxcoords->w = 0.15f;
	this->tmplay->vtxcoords->h = 0.26f;
	this->tmbackw = new Box;
	this->tmbackw->vtxcoords->x1 = -0.225f;
	this->tmbackw->vtxcoords->y1 = -0.9f;
	this->tmbackw->vtxcoords->w = 0.15f;
	this->tmbackw->vtxcoords->h = 0.26f;
	this->tmbounce = new Box;
	this->tmbounce->vtxcoords->x1 = -0.075f;
	this->tmbounce->vtxcoords->y1 = -0.9f;
	this->tmbounce->vtxcoords->w = 0.15f;
	this->tmbounce->vtxcoords->h = 0.26f;
	this->tmfrforw = new Box;
	this->tmfrforw->vtxcoords->x1 = 0.225f;
	this->tmfrforw->vtxcoords->y1 = -0.9f;
	this->tmfrforw->vtxcoords->w = 0.15;
	this->tmfrforw->vtxcoords->h = 0.26f;
	this->tmfrbackw = new Box;
	this->tmfrbackw->vtxcoords->x1 = -0.375f;
	this->tmfrbackw->vtxcoords->y1 = -0.9f;
	this->tmfrbackw->vtxcoords->w = 0.15;
	this->tmfrbackw->vtxcoords->h = 0.26f;
	this->tmspeed = new Box;
	this->tmspeed->vtxcoords->x1 = -0.8f;
	this->tmspeed->vtxcoords->y1 = -0.5f;
	this->tmspeed->vtxcoords->w = 0.2f;
	this->tmspeed->vtxcoords->h = 1.0f;
	this->tmspeedzero = new Box;
	this->tmspeedzero->vtxcoords->x1 = -0.775f;
	this->tmspeedzero->vtxcoords->y1 = -0.1f;
	this->tmspeedzero->vtxcoords->w = 0.15f;
	this->tmspeedzero->vtxcoords->h = 0.2f;
	this->tmopacity = new Box;
	this->tmopacity->vtxcoords->x1 = 0.6f;
	this->tmopacity->vtxcoords->y1 = -0.5f;
	this->tmopacity->vtxcoords->w = 0.2f;
	this->tmopacity->vtxcoords->h = 1.0f;
	this->tmfreeze = new Box;
	this->tmfreeze->vtxcoords->x1 = -0.1f;
	this->tmfreeze->vtxcoords->y1 = 0.1f;
	this->tmfreeze->vtxcoords->w = 0.2f;
	this->tmfreeze->vtxcoords->h = 0.2f;
	
	this->wormhole = new Button(false);
	this->buttons.push_back(this->wormhole);
}

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
		this->path = tinyfd_openFileDialog(title, dd, 0, nullptr, nullptr, 0);
	}
	else {
		this->path = tinyfd_openFileDialog(title, dd, 1, fi, nullptr, 0);
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
		this->path = tinyfd_saveFileDialog(title, dd, 0, nullptr, nullptr);
	}
	else {
		this->path = tinyfd_saveFileDialog(title, dd, 1, fi, nullptr);
	}
}

void Program::get_multinname(const char* title) {
	const char *outpaths;
	outpaths = tinyfd_openFileDialog(title, "", 0, nullptr, nullptr, 1);
	if (outpaths == nullptr) {
		binsmain->openbinfile = false;
		return;
	}
	std::string opaths(outpaths);
	std::string currstr = "";
	std::string charstr;
	for (int i = 0; i < opaths.length(); i++) {
		charstr = opaths[i];
		if (charstr == "|") {
			this->paths.push_back(currstr);
			std::string currstr = "";
			continue;
		}
		currstr += charstr;
		if (i == opaths.length() - 1) this->paths.push_back(currstr);
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
	//save midi map
	save_genmidis("./midiset.gm");
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

void Program::preveff_init() {
	std::vector<Layer*> &lvec = choose_layers(mainmix->currlay->deck);
	if (!this->prevmodus) {
		// normally all comp layers are copied from normal layers
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
	glUniform1i(preff, this->prevmodus);
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
	glShaderSource(vertexShaderObject, 1, &VShaderSource, nullptr);
	glShaderSource(fragmentShaderObject, 1, &FShaderSource, nullptr);
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



