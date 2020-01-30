#include "boost/bind.hpp"
#include "boost/asio.hpp"
#include "boost/chrono.hpp"
#include "boost/thread/thread.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/filesystem.hpp"
#include "boost/foreach.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <cstdlib>

typedef struct float4 {
	float x;
	float y;
	float z;
	float w;
} float4;
typedef struct char4 {
	char x;
	char y;
	char z;
	char w;
} char4;

#include <cstring>
#include <string>
#include <assert.h>
#include <ostream>
#include <istream>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ios>
#include <vector>
#include <algorithm>
#include <numeric>
#include <unordered_set>
#include <unordered_map>
#include <list>
#include <map>
#include <time.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <sys/stat.h>
#ifndef UINT64_C
#define UINT64_C(c) (c ## ULL)
#endif

#ifdef _WIN64
#include "dirent.h"
#include <intrin.h>
#include <shobjidl.h>
#include <Vfw.h>
#define STRSAFE_NO_DEPRECATE
#include <initguid.h>
#include <dshow.h>
#pragma comment (lib, "strmiids.lib")
#pragma comment (lib, "Quartz.lib")
#pragma comment (lib, "ole32.lib")
#include <windows.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <shellscalingapi.h>
#include <comdef.h>
#endif

#include <RtMidi.h>
#include <jpeglib.h>
#define SDL_MAIN_HANDLED
#include "GL/glew.h"
#include "GL/gl.h"
#ifdef __linux__
#include </usr/include/dirent.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
//#include <linux/v4l2-common.h>
#include "GL/glx.h"
#endif
#include "GL/glut.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_syswm.h"
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include "snappy.h"
#include "snappy-c.h"

#include "IL/il.h"
extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
#include "libswscale/swscale.h"
#include "libavutil/avutil.h"
#include "libavutil/pixfmt.h"
#include "libavutil/dict.h"
#include "libavutil/opt.h"
#include "libavutil/imgutils.h"
}

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_BITMAP_H
#include FT_MODULE_H
#define  FT_HINTING_ADOBE     0

// my own headers
#include "box.h"
#include "effect.h"
#include "node.h"
#include "layer.h"
#include "window.h"
#include "program.h"
#include "loopstation.h"
#include "bins.h"

//#include "debug_new.h"

#define PROGRAM_NAME "EWOCvj"
#define _M_AMD64 100
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"


//#include "debug_new.h"
//#define _DEBUG_NEW_EMULATE_MALLOC 1

FT_Library ft;
Globals *glob = nullptr;
Program *mainprogram = nullptr;
Mixer *mainmix = nullptr;
BinsMain *binsmain = nullptr;
LoopStation *loopstation = nullptr;
LoopStation *lp = nullptr;
LoopStation *lpc = nullptr;
float smw, smh;
SDL_GLContext glc;
SDL_GLContext glc_tm;
SDL_GLContext glc_pr;
SDL_GLContext glc_th;
float white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
float halfwhite[] = { 1.0f, 1.0f, 1.0f, 0.5f };
float black[] = { 0.0f, 0.0f, 0.0f, 1.0f };
float alpha[] = { 0.0f, 0.0f, 0.0f, 0.0f };
float orange[] = { 1.0f, 0.5f, 0.0f, 1.0f };
float purple[] = { 0.5f, 0.5f, 1.0f, 1.0f };
float yellow[] = { 0.9f, 0.8f, 0.0f, 1.0f };
float lightblue[4] = { 0.5f, 0.5f, 1.0f, 1.0f };
float darkblue[] = { 0.1f, 0.5f, 1.0f, 1.0f };
float red[] = { 1.0f, 0.0f, 0.0f, 1.0f };
float grey[] = { 0.4f, 0.4f, 0.4f, 1.0f };
float pink[] = { 1.0f, 0.5f, 0.5f, 1.0f };
float green[] = { 0.0f, 1.0f, 0.2f, 1.0f };
float lightgreen[] = { 0.5f, 1.0f, 0.5f, 1.0f };
float darkgreen1[] = { 0.0f, 0.75f, 0.0f, 1.0f };
float darkgreen2[] = { 0.0f, 0.4f, 0.0f, 1.0f };
float blue[] = { 0.0f, 0.2f, 1.0f, 1.0f };
float alphawhite[] = { 1.0f, 1.0f, 1.0f, 0.5f };
float alphablue[] = { 0.7f, 0.7f, 1.0f, 0.5f };
float alphagreen[] = { 0.0f, 1.0f, 0.2f, 0.5f };
float darkred1[] = { 0.75f, 0.0f, 0.0f, 1.0f };
float darkred2[] = { 0.4f, 0.0f, 0.0f, 0.5f };
float darkgrey[] = { 0.2f, 0.2f, 0.2f, 1.0f };

//TCHAR buf [MAX_PATH];
//int retgtp = Getmainprogram->temppath(MAX_PATH, buf);
//std::string mainprogram->temppath (buf);
static GLuint mixvao;
static GLuint thmvbuf;
static GLuint thmvao;
static GLuint boxvao;
FT_Face face;
LayMidi *laymidiA;
LayMidi *laymidiB;
LayMidi *laymidiC;
LayMidi *laymidiD;
int sscount = 0;

using namespace boost::asio;
using namespace std;


bool exists(const std::string &name) {
    if (FILE *file = fopen(name.c_str(), "r")) {
        fclose(file);
        return true;
    } else {
        return false;
    }   
}

std::string replace_string(std::string subject, const std::string& search, const std::string& replace) {
	size_t pos = 0;
	while ((pos = subject.find(search, pos)) != std::string::npos) {
		subject.replace(pos, search.length(), replace);
		pos += replace.length();
	}
	return subject;
}

std::string dirname(std::string pathname)
{
	size_t last_slash_idx1 = pathname.find_last_of("//");
	size_t last_slash_idx2 = pathname.find_last_of("\\");
	if (last_slash_idx1 == std::string::npos) last_slash_idx1 = 0;
	if (last_slash_idx2 == std::string::npos) last_slash_idx2 = 0;
	size_t maxpos = last_slash_idx2;
	if (last_slash_idx1 > last_slash_idx2) maxpos = last_slash_idx1;
	if (pathname.size() != maxpos)
	{
		pathname.erase(maxpos + 1, std::string::npos);
	}
	if (maxpos == 0) pathname = "";
    return pathname;
}

std::string basename(std::string pathname)
{
	size_t last_slash_idx1 = pathname.find_last_of("//");
	size_t last_slash_idx2 = pathname.find_last_of("\\");
	if (last_slash_idx1 == std::string::npos) last_slash_idx1 = 0;
	if (last_slash_idx2 == std::string::npos) last_slash_idx2 = 0;
	size_t maxpos = last_slash_idx2;
	if (last_slash_idx1 > last_slash_idx2) maxpos = last_slash_idx1;
	if (std::string::npos != maxpos)
	{
		pathname.erase(0, maxpos + 1);
	}
    return pathname;
}

std::string remove_extension(std::string filename) {
	const size_t period_idx = filename.rfind('.');
	if (std::string::npos != period_idx)
	{
		filename.erase(period_idx);
	}
	return filename;
}

std::string chop_off(std::string filename) {
	const size_t period_idx = filename.rfind(' ');
	if (std::string::npos != period_idx)
	{
		filename.erase(period_idx);
	}
	return filename;
}

std::string remove_version(std::string filename) {
	const size_t underscore_idx = filename.rfind('_');
	try {
		int dummy = std::stoi(filename.substr(filename.rfind('_') + 1));
	}
	catch (...) {
		return filename;
	}
	if (std::string::npos != underscore_idx)
	{
		filename.erase(underscore_idx);
	}
	return filename;
}

bool isimage(const std::string &path) {
	ILboolean ret = ilLoadImage(path.c_str());
	return (bool)ret;
}


					

class Deadline 
{
public:
    Deadline(deadline_timer &timer) : t(timer) {
        wait();
    }

    void timeout() {
        cout << "tick" << endl;
        screenshot();
        wait();
    }

    void cancel() {
        t.cancel();
    }


private:
    void wait() {
        t.expires_from_now(boost::posix_time::seconds(20)); //repeat rate here
        t.async_wait(boost::bind(&Deadline::timeout, this));
    }

    deadline_timer &t;
};


class CancelDeadline {
public:
    CancelDeadline(Deadline &d) :dl(d) { }
    void operator()() {
        string cancel;
        cin >> cancel;
        dl.cancel();
        return;
    }
private:
    Deadline &dl;
};

class Deadline2
{
public:
    Deadline2(deadline_timer &timer) : t(timer) {
        wait();
    }

    void timeout() {
        if (!mainmix->donerec) {
			#define BUFFER_OFFSET(i) ((char *)nullptr + (i))
			glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, mainmix->ioBuf);
			glBufferData(GL_PIXEL_PACK_BUFFER_ARB, (int)(mainprogram->ow * mainprogram->oh) * 4, nullptr, GL_DYNAMIC_READ);
			glBindFramebuffer(GL_FRAMEBUFFER, ((MixNode*)mainprogram->nodesmain->mixnodescomp[2])->mixfbo);
			glReadBuffer(GL_COLOR_ATTACHMENT0);
			glReadPixels(0, 0, mainprogram->ow, (int)mainprogram->oh, GL_BGRA, GL_UNSIGNED_BYTE, BUFFER_OFFSET(0));
			mainmix->rgbdata = glMapBuffer(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY);
			assert(mainmix->rgbdata);
			mainmix->recordnow = true;
			while (mainmix->recordnow and !mainmix->donerec) {
				mainmix->startrecord.notify_one();
			}
			glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
			mainprogram->drawbuffer = mainprogram->globfbo;
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
		}
        wait();
    }

    void cancel() {
        t.cancel();
    }


private:
    void wait() {
        t.expires_from_now(boost::posix_time::milliseconds(40)); //repeat rate here
        t.async_wait(boost::bind(&Deadline2::timeout, this));
    }

    deadline_timer &t;
};


class CancelDeadline2 {
public:
    CancelDeadline2(Deadline2 &d) :dl(d) { }
    void operator()() {
        string cancel;
        cin >> cancel;
        dl.cancel();
        return;
    }
private:
    Deadline2 &dl;
};



void screenshot() {
	return;
	int wi = mainprogram->ow;
	int he = mainprogram->oh;
	char *buf = (char*)malloc(wi * he * 3);
	MixNode *node = (MixNode*)mainprogram->nodesmain->mixnodescomp[2];

	glBindFramebuffer(GL_FRAMEBUFFER, node->mixfbo);
	//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, node->mixtex, 0);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glReadPixels(0, 0, wi, he, GL_RGB, GL_UNSIGNED_BYTE, buf);
	glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
	mainprogram->drawbuffer = mainprogram->globfbo;
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	
	std::string name = mainprogram->docpath + "screenshots/screenshot" + std::to_string(sscount) + ".jpg";
	sscount++;
	FILE* outfile = fopen(name.c_str(), "wb");
	
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr       jerr;
	 
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, outfile);
	 
	cinfo.image_width      = wi;
	cinfo.image_height     = he;
	cinfo.input_components = 3;
	cinfo.in_color_space   = JCS_RGB;
	
	jpeg_set_defaults(&cinfo);
	jpeg_set_quality (&cinfo, 80, true);
	jpeg_start_compress(&cinfo, true);
	JSAMPROW row_pointer;          /* pointer to a single row */
	while (cinfo.next_scanline < cinfo.image_height) {
		row_pointer = (JSAMPROW) &buf[cinfo.next_scanline * 3 * wi];
		jpeg_write_scanlines(&cinfo, &row_pointer, 1);
	}
	jpeg_finish_compress(&cinfo);
	free(buf);
	fclose(outfile);
}
	
	
void handle_midi(LayMidi *laymidi, int deck, int midi0, int midi1, int midi2, std::string &midiport) {
	std::vector<Layer*> &lvec = choose_layers(deck);
	int genmidideck = mainmix->genmidi[deck]->value;
	for (int j = 0; j < lvec.size(); j++) {
		if (lvec[j]->genmidibut->value == genmidideck) {
			if (midi0 == laymidi->play[0] and midi1 == laymidi->play[1] and midi2 == 127 and midiport == laymidi->playstr) {
				lvec[j]->playbut->value = !lvec[j]->playbut->value;
			}
			if (midi0 == laymidi->pausestop[0] and midi1 == laymidi->pausestop[1] and midi2 == 127 and midiport == laymidi->pausestopstr) {
				lvec[j]->playbut->value = false;
			}
			if (midi0 == laymidi->bounce[0] and midi1 == laymidi->bounce[1] and midi2 == 127 and midiport == laymidi->bouncestr) {
				lvec[j]->bouncebut->value = !lvec[j]->bouncebut->value;
			}
			if (midi0 == laymidi->scratch[0] and midi1 == laymidi->scratch[1] and midiport == laymidi->scratchstr) {
				lvec[j]->scratch = ((float)midi2 - 64.0f) / 4.0f;
			}
			if (midi0 == laymidi->frforw[0] and midi1 == laymidi->frforw[1] and midi2 == 127 and midiport == laymidi->frforwstr) {
				lvec[j]->frame += 1;
				if (lvec[j]->frame >= lvec[j]->numf) lvec[j]->frame = 0;
			}
			if (midi0 == laymidi->frbackw[0] and midi1 == laymidi->frbackw[1] and midi2 == 127 and midiport == laymidi->frbackwstr) {
				lvec[j]->frame -= 1;
				if (lvec[j]->frame < 0) lvec[j]->frame = lvec[j]->numf - 1;
			}
			if (midi0 == laymidi->scratchtouch[0] and midi1 == laymidi->scratchtouch[1] and midi2 == 127 and midiport == laymidi->scratchtouchstr) {
				lvec[j]->scratchtouch = 1;
			}
			if (midi0 == laymidi->scratchtouch[0] and midi1 == laymidi->scratchtouch[1] and midi2 == 0 and midiport == laymidi->scratchtouchstr) {
				lvec[j]->scratchtouch = 0;
			}
			if (midi0 == laymidi->speed[0] and midi1 == laymidi->speed[1] and midiport == laymidi->speedstr) {
				int m2 = -(midi2 - 127);
				if (m2 >= 64.0f) {
					lvec[j]->speed->value = 1.0f + (2.33f / 64.0f) * (m2 - 64.0f);
				}
				else {
					lvec[j]->speed->value = 0.0f + (1.0f / 64.0f) * m2;
				}
			}
			if (midi0 == laymidi->speedzero[0] and midi1 == laymidi->speedzero[1] and midiport == laymidi->speedzerostr) {
				lvec[j]->speed->value = 1.0f;
			}
			if (midi0 == laymidi->opacity[0] and midi1 == laymidi->opacity[1] and midiport == laymidi->opacitystr) {
				lvec[j]->opacity->value = (float)midi2 / 127.0f;
			}
		}
	}
}

void mycallback( double deltatime, std::vector< unsigned char > *message, void *userData )
{
  	unsigned int nBytes = message->size();
  	int midi0 = (int)message->at(0);
  	int midi1 = (int)message->at(1);
  	float midi2 = (int)message->at(2);
  	std::string midiport = ((PrefItem*)userData)->name;
  	
 	if (mainprogram->waitmidi == 0 and mainprogram->tmlearn) {
    	mainprogram->stt = clock();
    	mainprogram->savedmessage = *message;
    	mainprogram->savedmidiitem = (PrefItem*)userData;
    	mainprogram->waitmidi = 1;
    	return;
    }
    if (mainprogram->waitmidi == 1) {
     	mainprogram->savedmessage = *message;
    	mainprogram->savedmidiitem = (PrefItem*)userData;
   		return;
   	}
  	
  	if (mainprogram->midipresets) {
  		LayMidi *lm = nullptr;
  		if (mainprogram->midipresetsset == 1) lm = laymidiA;
  		else if (mainprogram->midipresetsset == 2) lm = laymidiB;
  		else if (mainprogram->midipresetsset == 3) lm = laymidiC;
  		else if (mainprogram->midipresetsset == 4) lm = laymidiD;
		switch (mainprogram->tmlearn) {
			case TM_NONE:
				if (lm->play[0] == midi0 and lm->play[1] == midi1 and lm->playstr == midiport) mainprogram->tmchoice = TM_PLAY;
				else if (lm->backw[0] == midi0 and lm->backw[1] == midi1 and lm->backwstr == midiport) mainprogram->tmchoice = TM_BACKW;
				else if (lm->bounce[0] == midi0 and lm->bounce[1] == midi1 and lm->bouncestr == midiport) mainprogram->tmchoice = TM_BOUNCE;
				else if (lm->frforw[0] == midi0 and lm->frforw[1] == midi1 and lm->frforwstr == midiport) mainprogram->tmchoice = TM_FRFORW;
				else if (lm->frbackw[0] == midi0 and lm->frbackw[1] == midi1 and lm->frbackwstr == midiport) mainprogram->tmchoice = TM_FRBACKW;
				else if (lm->speed[0] == midi0 and lm->speed[1] == midi1 and lm->speedstr == midiport) mainprogram->tmchoice = TM_SPEED;
				else if (lm->speedzero[0] == midi0 and lm->speedzero[1] == midi1 and lm->speedzerostr == midiport) mainprogram->tmchoice = TM_SPEEDZERO;
				else if (lm->opacity[0] == midi0 and lm->opacity[1] == midi1 and lm->opacitystr == midiport) mainprogram->tmchoice = TM_OPACITY;
				else if (lm->scratchtouch[0] == midi0 and lm->scratchtouch[1] == midi1 and lm->scratchtouchstr == midiport) mainprogram->tmchoice = TM_FREEZE;
				else if (lm->scratch[0] == midi0 and lm->scratch[1] == midi1 and lm->scratchstr == midiport) mainprogram->tmchoice = TM_SCRATCH;
				else mainprogram->tmchoice = TM_NONE;
				return;
				break;
			case TM_PLAY:
				lm->play[0] = midi0;
				lm->play[1] = midi1;
				lm->playstr = midiport;
				mainprogram->tmlearn = TM_NONE;
				break;
			case TM_BACKW:
				lm->backw[0] = midi0;
				lm->backw[1] = midi1;
				lm->backwstr = midiport;
				mainprogram->tmlearn = TM_NONE;
				break;
			case TM_BOUNCE:
				lm->bounce[0] = midi0;
				lm->bounce[1] = midi1;
				lm->bouncestr = midiport;
				mainprogram->tmlearn = TM_NONE;
				break;
			case TM_FRFORW:
				lm->frforw[0] = midi0;
				lm->frforw[1] = midi1;
				lm->frforwstr = midiport;
				mainprogram->tmlearn = TM_NONE;
				break;
			case TM_FRBACKW:
				lm->frbackw[0] = midi0;
				lm->frbackw[1] = midi1;
				lm->frbackwstr = midiport;
				mainprogram->tmlearn = TM_NONE;
				break;
			case TM_SPEED:
				if (midi0 == 144) return;
				lm->speed[0] = midi0;
				lm->speed[1] = midi1;
				lm->speedstr = midiport;
				mainprogram->tmlearn = TM_NONE;
				break;
			case TM_SPEEDZERO:
				if (midi0 == 176) return;
				lm->speedzero[0] = midi0;
				lm->speedzero[1] = midi1;
				lm->speedzerostr = midiport;
				mainprogram->tmlearn = TM_NONE;
				break;
			case TM_OPACITY:
				lm->opacity[0] = midi0;
				lm->opacity[1] = midi1;
				lm->opacitystr = midiport;
				mainprogram->tmlearn = TM_NONE;
				break;
			case TM_FREEZE:
				if (midi0 == 176) return;
				lm->scratchtouch[0] = midi0;
				lm->scratchtouch[1] = midi1;
				lm->scratchtouchstr = midiport;
				mainprogram->tmlearn = TM_NONE;
				break;
			case TM_SCRATCH:
				if (midi0 == 144) return;
				lm->scratch[0] = midi0;
				lm->scratch[1] = midi1;
				lm->scratchstr = midiport;
				mainprogram->tmlearn = TM_NONE;
				break;
		}
		return;
	}
	
	
  	if (mainmix->learn) {
  		if (midi0 == 176 and midi2 == 127 and mainmix->learnparam and mainmix->learnparam->sliding) {
			mainmix->learn = false;
			mainmix->learnparam->midi[0] = midi0;
			mainmix->learnparam->midi[1] = midi1;
			mainmix->learnparam->midiport = midiport;
			mainmix->learnparam->node = new MidiNode;
			mainmix->learnparam->node->param = mainmix->learnparam;
			mainprogram->nodesmain->currpage->nodes.push_back(mainmix->learnparam->node);
			if (mainmix->learnparam->effect) {
				mainprogram->nodesmain->currpage->connect_in2(mainmix->learnparam->node, mainmix->learnparam->effect->node);
			}
		}
		//else if (midi0 == 144 and mainmix->learnparam and !mainmix->learnparam->sliding) {
		//	mainmix->learn = false;
		//	mainmix->learnparam->midi[0] = midi0;
		//	mainmix->learnparam->midi[1] = midi1;
		//	mainmix->learnparam->midiport = midiport;
		//	mainmix->learnparam->node = new MidiNode;
		//	mainmix->learnparam->node->param = mainmix->learnparam;
		//	mainprogram->nodesmain->currpage->nodes.push_back(mainmix->learnparam->node);
		//	if (mainmix->learnparam->effect) {
		//		mainprogram->nodesmain->currpage->connect_in2(mainmix->learnparam->node, mainmix->learnparam->effect->node);
		//	}
		//}
  		else if (midi0 == 176 and mainmix->learnbutton) {
			if (mainmix->midishelfstage == 1) {
				// start midi button
				mainmix->midishelfstage = 2;
				mainmix->midishelfstart = midi1;
				mainmix->learnbutton->midi[0] = midi0;
				mainmix->learnbutton->midi[1] = midi1;
				mainmix->learnbutton->midiport = midiport;
			}
			else if (mainmix->midishelfstage == 2) {
				// end midi button
				mainmix->midishelfstage = 0;
				mainmix->learnbutton->midi[0] = midi0;
				mainmix->learnbutton->midi[1] = midi1;
				mainmix->learnbutton->midiport = midiport;
				if (midi1 > mainmix->midishelfstart and midi1 < mainmix->midishelfstart + (16 - mainmix->midishelfpos)) {
					for (int i = mainmix->midishelfstart; i < midi1 + 1; i++) {
						mainmix->midishelf->elements[mainmix->midishelfpos + i - mainmix->midishelfstart]->button->midi[0] = midi0;
						mainmix->midishelf->elements[mainmix->midishelfpos + i - mainmix->midishelfstart]->button->midi[1] = i;
						mainmix->midishelf->elements[mainmix->midishelfpos + i - mainmix->midishelfstart]->button->midiport = midiport;
					}
				}
				mainmix->learn = false;
			}
			else {
				mainmix->learnbutton->midi[0] = midi0;
				mainmix->learnbutton->midi[1] = midi1;
				mainmix->learnbutton->midiport = midiport;
				mainmix->learn = false;
			}

/* 			mainmix->learnbutton->node = new MidiNode;
			mainmix->learnbutton->node->button = mainmix->learnbutton;
			mainprogram->nodesmain->currpage->nodes.push_back(mainmix->learnbutton->node);
			if (mainmix->learnbutton->effect) {
				mainprogram->nodesmain->currpage->connect_in2(mainmix->learnbutton->node, mainmix->learnbutton->effect->node);
			}
IMPLEMENT */		}
		else if (midi0 == 144 and mainmix->learnbutton) {
			if (mainmix->midishelfstage == 1) {
				// start midi button
				mainmix->midishelfstage = 2;
				mainmix->midishelfstart = midi1;
				mainmix->learnbutton->midi[0] = midi0;
				mainmix->learnbutton->midi[1] = midi1;
				mainmix->learnbutton->midiport = midiport;
			}
			else if (mainmix->midishelfstage == 2) {
				// end midi button
				mainmix->midishelfstage = 0;
				mainmix->learnbutton->midi[0] = midi0;
				mainmix->learnbutton->midi[1] = midi1;
				mainmix->learnbutton->midiport = midiport;
				if (midi1 > mainmix->midishelfstart and midi1 < mainmix->midishelfstart + (16 - mainmix->midishelfpos)) {
					for (int i = mainmix->midishelfstart; i < midi1 + 1; i++) {
						mainmix->midishelf->elements[mainmix->midishelfpos + i - mainmix->midishelfstart]->button->midi[0] = midi0;
						mainmix->midishelf->elements[mainmix->midishelfpos + i - mainmix->midishelfstart]->button->midi[1] = i;
						mainmix->midishelf->elements[mainmix->midishelfpos + i - mainmix->midishelfstart]->button->midiport = midiport;
					}
				}
				mainmix->learn = false;
			}
			else {
				mainmix->learnbutton->midi[0] = midi0;
				mainmix->learnbutton->midi[1] = midi1;
				mainmix->learnbutton->midiport = midiport;
				mainmix->learn = false;
			}
			/* 			mainmix->learnparam->node = new MidiNode;
			mainmix->learnparam->node->param = mainmix->learnparam;
			mainprogram->nodesmain->currpage->nodes.push_back(mainmix->learnparam->node);
			if (mainmix->learnparam->effect) {
				mainprogram->nodesmain->currpage->connect_in2(mainmix->learnparam->node, mainmix->learnparam->effect->node);
			}
 */		}
		return;
  	}
  	
  	
  	if (midi0 == 176 or midi0 == 144) {
		for (int i = 0; i < mainprogram->buttons.size(); i++) {
			Button *but = mainprogram->buttons[i];
			if (midi0 == but->midi[0] and midi1 == but->midi[1] and midiport == but->midiport) {
				mainmix->midi2 = midi2;
				mainmix->midibutton = but;
			}
		}
		for (int m = 0; m < 2; m++) {
			for (int i = 0; i < 16; i++) {
				Button *but = mainprogram->shelves[m]->buttons[i];
				if (midi0 == but->midi[0] and midi1 == but->midi[1] and midiport == but->midiport) {
					mainmix->midi2 = midi2;
					mainmix->midibutton = nullptr;
					mainmix->midishelfbutton = but;
				}
			}
		}

		Param *par;
		if (mainprogram->prevmodus) par = mainmix->crossfade;
		else par = mainmix->crossfadecomp;
		if (midi0 == par->midi[0] and midi1 == par->midi[1] and midiport == par->midiport) {
			mainmix->midi2 = midi2;
			mainmix->midiparam = par;
		}

		std::vector<Node*> ns;
		if (mainprogram->prevmodus) ns = mainprogram->nodesmain->currpage->nodes;
		else ns = mainprogram->nodesmain->currpage->nodescomp;
		for (int i = 0; i < ns.size(); i++) {
			if (ns[i]->type == EFFECT) {
				EffectNode *effnode = (EffectNode*)ns[i];
				for (int j = 0; j < effnode->effect->params.size(); j++) {
					Param *par = effnode->effect->params[j];
					if (midi0 == par->midi[0] and midi1 == par->midi[1] and midiport == par->midiport) {
						mainmix->midi2 = midi2;
						mainmix->midiparam = par;
					}
				}
			}
		}
		mainmix->midiisspeed = false;
		for (int i = 0; i < ns.size(); i++) {
			if (ns[i]->type == VIDEO) {
				VideoNode *vnode = (VideoNode*)ns[i]; 			
				if (midi0 == vnode->layer->speed->midi[0] and midi1 == vnode->layer->speed->midi[1] and midiport == vnode->layer->speed->midiport) {
					mainmix->midiisspeed = true;
					mainmix->midi2 = midi2;
					mainmix->midiparam = vnode->layer->speed;
				}
				if (midi0 == vnode->layer->opacity->midi[0] and midi1 == vnode->layer->opacity->midi[1] and midiport == vnode->layer->opacity->midiport) {
					mainmix->midi2 = midi2;
					mainmix->midiparam = vnode->layer->opacity;
				}
				if (midi0 == vnode->layer->volume->midi[0] and midi1 == vnode->layer->volume->midi[1] and midiport == vnode->layer->volume->midiport) {
					mainmix->midi2 = midi2;
					mainmix->midiparam = vnode->layer->volume;
				}
				if (midi0 == vnode->layer->chtol->midi[0] and midi1 == vnode->layer->chtol->midi[1] and midiport == vnode->layer->chtol->midiport) {
					mainmix->midi2 = midi2;
					mainmix->midiparam = vnode->layer->chtol;
				}
			}
			if (ns[i]->type == BLEND) {
				BlendNode *bnode = (BlendNode*)ns[i]; 			
				if (midi0 == bnode->mixfac->midi[0] and midi1 == bnode->mixfac->midi[1] and midiport == bnode->mixfac->midiport) {
					mainmix->midi2 = midi2;
					mainmix->midiparam = bnode->mixfac;
				}
			}
		}
	}
	LayMidi *lm = nullptr;
	if (mainmix->genmidi[0]->value == 1) lm = laymidiA;
	else if (mainmix->genmidi[0]->value == 2) lm = laymidiB;
	else if (mainmix->genmidi[0]->value == 3) lm = laymidiC;
	else if (mainmix->genmidi[0]->value == 4) lm = laymidiD;
	if (lm) handle_midi(lm, 0, midi0, midi1, midi2, midiport);
	lm = nullptr;
	if (mainmix->genmidi[1]->value == 1) lm = laymidiA;
	else if (mainmix->genmidi[1]->value == 2) lm = laymidiB;
	else if (mainmix->genmidi[1]->value == 3) lm = laymidiC;
	else if (mainmix->genmidi[1]->value == 4) lm = laymidiD;
	if (lm) handle_midi(lm, 1, midi0, midi1, midi2, midiport);

//	std::cout << "message = " << (int)message->at(0) << std::endl;
//	std::cout << "? = " << (int)message->at(1) << std::endl;
//	std::cout << "value = " << (int)message->at(2) << std::endl;
	if ( nBytes > 0 )
    	int stamp = deltatime;
}



int encode_frame(AVFormatContext *fmtctx, AVFormatContext *srcctx, AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt, FILE *outfile, int framenr) {
    int ret;
    /* send the frame to the encoder */
	AVPacket enc_pkt;
	enc_pkt.data = nullptr;
	enc_pkt.size = 0;
	av_init_packet(&enc_pkt);
	int got_frame = 0;
	if (outfile) {
		avcodec_send_frame(enc_ctx, frame);
 		int err = avcodec_receive_packet(enc_ctx, pkt);
 	}
	else {
		avcodec_send_frame(enc_ctx, frame);
		int err = avcodec_receive_packet(enc_ctx, &enc_pkt);
		if (srcctx) av_packet_rescale_ts(&enc_pkt, srcctx->streams[enc_pkt.stream_index]->time_base, fmtctx->streams[enc_pkt.stream_index]->time_base);
		else {
			// *2 still don't know why
			AVRational sttb = { fmtctx->streams[enc_pkt.stream_index]->time_base.num, fmtctx->streams[enc_pkt.stream_index]->time_base.den * 2};
			enc_pkt.pts = framenr;
			av_packet_rescale_ts(&enc_pkt, enc_ctx->time_base, sttb);
		}
	}
	if (outfile) fwrite(pkt->data, 1, pkt->size, outfile);
	else {
		/* prepare packet for muxing */
		enc_pkt.stream_index = pkt->stream_index;
		//enc_pkt.duration = pkt->duration;//(fmtctx->streams[enc_pkt.stream_index]->time_base.den * fmtctx->streams[enc_pkt.stream_index]->r_frame_rate.den) / (fmtctx->streams[enc_pkt.stream_index]->time_base.num * fmtctx->streams[enc_pkt.stream_index]->r_frame_rate.num);
		int err = av_interleaved_write_frame(fmtctx, &enc_pkt);
	}
   	av_packet_unref(pkt);
   	av_packet_unref(&enc_pkt);
   	return got_frame;
}

static int decode_packet(Layer *lay, bool show)
{
    int ret = 0;
    int decoded = lay->decpkt.size;
 	//av_frame_unref(lay->decframe);
	// lay->decpkt.dts = av_rescale_q_rnd(lay->decpkt.dts,
			// lay->video->streams[lay->decpkt.stream_index]->time_base,
			// lay->video->streams[lay->decpkt.stream_index]->codec->time_base,
			// AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
	// lay->decpkt.pts = av_rescale_q_rnd(lay->decpkt.pts,
			// lay->video->streams[lay->decpkt.stream_index]->time_base,
			// lay->video->streams[lay->decpkt.stream_index]->codec->time_base,
			// AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
	if (lay->decpkt.stream_index == lay->video_stream_idx) {
		/* decode video frame */
		int err2 = 0;
		if (!lay->vidopen) {
			while (1) {
				int err1 = avcodec_send_packet(lay->video_dec_ctx, &lay->decpkt);
				err2 = avcodec_receive_frame(lay->video_dec_ctx, lay->decframe);
				if (err2 == AVERROR(EAGAIN)) {
					av_packet_unref(&lay->decpkt);
					av_read_frame(lay->video, &lay->decpkt);
				}
				else break;
			}
			if (lay->vidformat == 187) {
				printf("");
			}
			//char buffer[1024];
			//av_strerror(err2, buffer, 1024);
			//printf(buffer);
			//if (err == AVERROR_EOF) lay->numf--;
			if (err2 == AVERROR(EINVAL)) {
				fprintf(stderr, "Error decoding video frame (%s)\n", 0);
				printf("codec %d", lay->decpkt);
				return ret;
			}
			if (err2 == AVERROR_EOF) {
				avcodec_flush_buffers(lay->video_dec_ctx);
			}
			if (show) {
				/* copy decoded frame to destination buffer:
				 * this is required since rawvideo expects non aligned data */
				int h = sws_scale
				(
					lay->sws_ctx,
					(uint8_t const* const*)lay->decframe->data,
					lay->decframe->linesize,
					0,
					lay->video_dec_ctx->height,
					lay->rgbframe->data,
					lay->rgbframe->linesize
				);
				if (h < 1) return 0;
				std::mutex datalock;
				datalock.lock();
				lay->decresult->hap = false;
				lay->decresult->data = (char*)*(lay->rgbframe->extended_data);
				lay->decresult->height = lay->video_dec_ctx->height;
				lay->decresult->width = lay->video_dec_ctx->width;
				lay->decresult->size = lay->decresult->width * lay->decresult->height * 4;
				lay->decresult->newdata = true;
				if (lay->clonesetnr != -1) {
					std::unordered_set<Layer*>::iterator it;
					for (it = mainmix->clonesets[lay->clonesetnr]->begin(); it != mainmix->clonesets[lay->clonesetnr]->end(); it++) {
						Layer* clay = *it;
						if (clay == lay) continue;
						clay->decresult->newdata = true;
					}
				}
				datalock.unlock();
			}
		}
	}
    else if (lay->decpkt.stream_index == lay->audio_stream_idx) {
        /* decode audio frame */
		int err2 = 0;
		int nsam = 0;
		while (1) {
			int err1 = avcodec_send_packet(lay->audio_dec_ctx, &lay->decpkt);
			err2 = avcodec_receive_frame(lay->audio_dec_ctx, lay->decframe);
			nsam += lay->decframe->nb_samples;
			if (err2 == AVERROR(EAGAIN)) {
				av_packet_unref(&lay->decpkt);
				av_read_frame(lay->video, &lay->decpkt);
			}
			else break;
		}
		if (err2 == AVERROR(EINVAL)) {
			fprintf(stderr, "Error decoding audio frame (%s)\n", 0);
			printf("codec %d", lay->decpkt);
			return ret;
		}
		/* Some audio decoders decode only part of the packet, and have to be
         * called again with the remainder of the packet data.
         * Sample: fate-suite/lossless-audio/luckynight-partial.shn
         * Also, some decoders might over-read the packet. */
        //decoded = FFMIN(ret, lay->decpkt.size);
		return nsam;
    }
    
 	//av_frame_unref(lay->decframe);
 	
	return decoded;
}


int open_codec_context(int *stream_idx,
                              AVFormatContext *video, enum AVMediaType type)
{
    int ret;
    AVStream *st;
    AVCodecContext *dec_ctx = nullptr;
    AVCodec *dec = nullptr;
    AVDictionary *opts = nullptr;
    ret = av_find_best_stream(video, type, -1, -1, nullptr, 0);
    if (ret <0) {
    	printf("%s\n", " cant find stream");
    	return -1;
    }
	*stream_idx = ret;
	st = video->streams[*stream_idx];
	/* find decoder for the stream */
	dec = avcodec_find_decoder(st->codecpar->codec_id);
	dec_ctx = avcodec_alloc_context3(dec);
	avcodec_parameters_to_context(dec_ctx, st->codecpar);
    //dec_ctx->refcounted_frames = false;
	dec = avcodec_find_decoder(dec_ctx->codec_id);
	if (!dec) {
		fprintf(stderr, "Failed to find %s codec\n",
				av_get_media_type_string(type));
		return AVERROR(EINVAL);
	}
	/* Init the decoders */
	if ((ret = avcodec_open2(dec_ctx, dec, &opts)) < 0) {
		fprintf(stderr, "Failed to open %s codec\n",
				av_get_media_type_string(type));
		return ret;
	}
    return 0;
}
static int get_format_from_sample_fmt(const char **fmt,
                                      enum AVSampleFormat sample_fmt)
{
    int i;
    struct sample_fmt_entry {
        enum AVSampleFormat sample_fmt; const char *fmt_be, *fmt_le;
    } sample_fmt_entries[] = {
        { AV_SAMPLE_FMT_U8,  "u8",    "u8"    },
        { AV_SAMPLE_FMT_S16, "s16be", "s16le" },
        { AV_SAMPLE_FMT_S32, "s32be", "s32le" },
        { AV_SAMPLE_FMT_FLT, "f32be", "f32le" },
        { AV_SAMPLE_FMT_DBL, "f64be", "f64le" },
    };
    *fmt = nullptr;
    for (i = 0; i < FF_ARRAY_ELEMS(sample_fmt_entries); i++) {
        struct sample_fmt_entry *entry = &sample_fmt_entries[i];
        if (sample_fmt == entry->sample_fmt) {
            *fmt = AV_NE(entry->fmt_be, entry->fmt_le);
            return 0;
        }
    }
    fprintf(stderr,
            "sample format %s is not supported as output format\n",
            av_get_sample_fmt_name(sample_fmt));
    return -1;
}

void decode_audio(Layer *lay) {
    int ret = 0, got_frame;
	char *snippet = nullptr;
	size_t unpadded_linesize;
    av_packet_unref(&lay->decpkt);
	av_frame_unref(lay->decframe);
	av_read_frame(lay->video, &lay->decpkt);
	while (lay->decpkt.stream_index == lay->audio_stream_idx) {
		if (!lay->dummy and lay->volume->value > 0.0f) {
			ret = decode_packet(lay, &got_frame);
			// flush
			//lay->decpkt.data = nullptr;
			//lay->decpkt.size = 0;
			//decode_packet(lay, &got_frame);
			if (ret >= 0) {
				int ps;
				unpadded_linesize = lay->decframe->linesize[0];
				if (av_sample_fmt_is_planar(lay->audio_dec_ctx->sample_fmt)) {
					snippet = new char[unpadded_linesize / 2];
					ps = unpadded_linesize / 2;
				}
				else {
					snippet = new char[unpadded_linesize / 2];
					ps = unpadded_linesize / 2;
				}
				for (int pos = 0; pos < unpadded_linesize / 2; pos++) {
					if (av_sample_fmt_is_planar(lay->audio_dec_ctx->sample_fmt)) {
						snippet[pos] = lay->decframe->extended_data[0][pos];
					}
					else {
						snippet[pos] = lay->decframe->extended_data[0][pos * 2 + 1];
					}
				}
				//if ((lay->playbut->value and lay->speed->value < 0) or (lay->revbut->value and lay->speed->value > 0)) {
				//	snippet = (int*)malloc(unpadded_linesize);
				//	for (int add = 0; add < unpadded_linesize / av_get_bytes_per_sample((AVSampleFormat)lay->decframe->format); add++) {
				//		snippet[add] = snip[add];
				//	}
				//	free(snip);
				//}
				lay->pslens.push_back(ps);
				lay->snippets.push_back(snippet);
				//	if (ret < 0) break;
				//	lay->decpkt.data += ret;
				//	lay->decpkt.size -= ret;
				//} while (lay->decpkt.size > 0);
				//av_free_packet(&orig_pkt);
			}
		}
    		av_packet_unref(&lay->decpkt);
		av_read_frame(lay->video, &lay->decpkt);
	}
	if (snippet) {
		lay->chready = true;
		while (lay->chready) {
			lay->newchunk.notify_one();
		}
	}
}	
	
void Layer::get_cpu_frame(int framenr, int prevframe, int errcount)
{
  	int ret = 0, got_frame;
	/* initialize packet, set data to nullptr, let the demuxer fill it */
	av_init_packet(&this->decpkt);
	av_init_packet(&this->decpktseek);
	/* flush cached frames */
	this->decpkt.data = nullptr;
	this->decpkt.size = 0;
	//do {
	//	decode_packet(&got_frame, 1);
	//} while (got_frame);


	if (this->type != ELEM_LIVE) {
		if (this->numf == 0) return;

		long long seekTarget = av_rescale(this->video_duration, framenr, this->numf) + this->video_stream->first_dts;
		if (framenr != 0) {
			if (framenr != prevframe + 1) {
				avformat_seek_file(this->videoseek, this->video_stream->index, this->video_stream->first_dts, seekTarget, seekTarget, 0);
				//avcodec_flush_buffers(this->video_dec_ctx);
				//int r = av_read_frame(this->videoseek, &this->decpktseek);
			}
			else {
				//avformat_seek_file(this->video, this->video_stream->index, 0, seekTarget, seekTarget, AVSEEK_FLAG_ANY);
			}
		}
		else {
			ret = avformat_seek_file(this->video, this->video_stream->index, seekTarget, seekTarget, this->video_duration + this->video_stream->first_dts, 0);
		}

		do {
			decode_audio(this);
		} while (this->decpkt.stream_index != this->video_stream_idx);
		if (!this->dummy) {
			if (framenr != prevframe + 1) {
				int r = av_read_frame(this->videoseek, &this->decpktseek);
				int readpos = ((this->decpktseek.dts - this->video_stream->first_dts) * this->numf) / this->video_duration;
				if (readpos <= framenr) {
					// readpos at keyframe after framenr
					if (framenr > prevframe and prevframe > readpos) {
						// starting from just past prevframe here is more efficient than decoding from readpos keyframe
						readpos = prevframe + 1;
					}
					else {
						avformat_seek_file(this->video, this->video_stream->index, this->video_stream->first_dts, seekTarget, seekTarget, 0);
					}
					for (int f = readpos; f < framenr; f = f + mainprogram->qualfr) {
						// decode sequentially frames starting from keyframe readpos to current framenr
						ret = decode_packet(this, false);
						do  {
							decode_audio(this);
						} while (this->decpkt.stream_index != this->video_stream_idx);
					}
				}
			}
		}
		ret = decode_packet(this, true);
		if (ret == 0) {
			return;
		}
		this->prevframe = framenr;
		if (this->decframe->width == 0) {
			this->prevframe = framenr;
			if ((this->speed->value > 0 and (this->playbut->value or this->bouncebut->value == 1)) or (this->speed->value < 0 and (this->revbut->value or this->bouncebut->value == 2))) {
				framenr++;
			}
			else if ((this->speed->value > 0 and (this->revbut->value or this->bouncebut->value == 2)) or (this->speed->value < 0 and (this->playbut->value or this->bouncebut->value == 1))) {
				framenr--;
			}
			else if (this->prevfbw) {
				//this->prevfbw = false;
				framenr--;
			}
			else framenr++;	
			if (framenr > this->endframe) framenr = this->startframe;
			else if (framenr < this->startframe) framenr = this->endframe;
			//avcodec_flush_buffers(this->video_dec_ctx);
			errcount++;
			if (errcount == 1000) {
				this->openerr = true;
				return;
			}
			get_cpu_frame(framenr, this->prevframe, errcount);
			this->frame == framenr;
		}
		//decode_audio(this);
		av_packet_unref(&this->decpkt);
	}
	else {
		//av_frame_unref(this->decframe);  reminder: leak?
		int r = av_read_frame(this->video, &this->decpkt);
		if (r < 0) printf("problem reading frame\n");
		else decode_packet(this, &got_frame);
		av_packet_unref(&this->decpkt);
	}
		
    
    if (this->audio_stream and 0) {
        enum AVSampleFormat sfmt = this->audio_dec_ctx->sample_fmt;
        int n_channels = this->audio_dec_ctx->channels;
        const char *fmt;
        if (av_sample_fmt_is_planar(this->audio_dec_ctx->sample_fmt)) {
            const char *packed = av_get_sample_fmt_name(sfmt);
            printf("Warning: the sample format the decoder produced is planar "
                   "(%s). This example will output the first channel only.\n",
                   packed ? packed : "?");
            sfmt = av_get_packed_sample_fmt(sfmt);
            n_channels = 1;
        }
        ret = get_format_from_sample_fmt(&fmt, sfmt);
    }
}


bool Layer::get_hap_frame() {

	if (!this->databuf) {
		//	this->decresult->width = 0;
		this->databuf = (char*)malloc(this->video_dec_ctx->width * this->video_dec_ctx->height * 4);
		if (this->databuf == nullptr) {
			return 0;
		}
		this->databufready = true;
	}

	std::mutex malock;
	malock.lock();
	long long seekTarget = av_rescale(this->video_stream->duration, this->frame, this->numf) + this->video_stream->first_dts;
	int r = av_seek_frame(this->video, this->video_stream->index, seekTarget, 0);
	//av_new_packet(&this->decpkt, this->video_dec_ctx->width * this->video_dec_ctx->height * 4);
	av_init_packet(&this->decpkt);
	this->decpkt.data = NULL;
	this->decpkt.size = 0;
	AVPacket* pktpnt = &this->decpkt;
	av_frame_unref(this->decframe);
    r = av_read_frame(this->video, &this->decpkt);
 	if (!this->dummy) this->prevframe = (int)this->frame;

	char *bptrData = (char*)(&this->decpkt)->data;
	size_t size = (&this->decpkt)->size;
	if (size == 0) { 
		malock.unlock();
		return false;
	}
	size_t uncomp;
	int headerl = 4;
	if (*bptrData == 0 and *(bptrData + 1) == 0 and *(bptrData + 2) == 0) {
		headerl = 8;
	}
	this->decresult->compression = *(bptrData + 3);

	int st = -1;
	if (this->databufready) {
		st = snappy_uncompress(bptrData + headerl, size - headerl, this->databuf, &uncomp);
	}
	av_packet_unref(&this->decpkt);

	if (!this->vidopen) {
		if (st != SNAPPY_OK) {
			this->decresult->data = nullptr;
			this->decresult->width = 0;
			malock.unlock();
			return false;
		}
		else this->decresult->data = this->databuf;
		this->decresult->height = this->video_dec_ctx->height;
		this->decresult->width = this->video_dec_ctx->width;
		this->decresult->size = uncomp;
		this->decresult->hap = true;
		this->decresult->newdata = true;
		if (this->clonesetnr != -1) {
			std::unordered_set<Layer*>::iterator it;
			for (it = mainmix->clonesets[this->clonesetnr]->begin(); it != mainmix->clonesets[this->clonesetnr]->end(); it++) {
				Layer* clay = *it;
				if (clay == this) continue;
				clay->decresult->newdata = true;
			}
		}
	}
	malock.unlock();
	return true;
}

void Layer::trigger() {
	//reminder: comment
	if (this->dummy) return;
	while (!this->closethread) {
		this->ready = true;
		while (this->ready) {
			this->startdecode.notify_one();
		}
		std::unique_lock<std::mutex> lock2(this->enddecodelock);
		this->enddecodevar.wait(lock2, [&] {return this->processed; });
		this->processed = false;
		lock2.unlock();
		//boost::this_thread::sleep(boost::posix_time::milliseconds(10));
	}
}

void Layer::get_frame(){

	float speed = 1.0;
	int frnums[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

	while (1) {
		std::unique_lock<std::mutex> lock(this->startlock);
		this->startdecode.wait(lock, [&] {return this->ready; });
		lock.unlock();
		this->ready = false;
		if (this->closethread) {
			this->closethread = false;
			//delete this;  implement with layer delete vector?
			return;
		}

		// calculate frame numbers, trigger clips in queue
		//bool ret = this->calc_texture(this->comp, true);
		//if (!this->comp and !mainprogram->prevmodus) continue;
		// reminder :: experiment failed -> no it works (priority non-realtime) - test both systems on slower computer!

		if (this->vidopen) {
			if (!this->dummy and this->video) {
				std::unique_lock<std::mutex> lock(this->enddecodelock);
				this->enddecodevar.wait(lock, [&] {return this->processed; });
				this->processed = false;
				lock.unlock();
				avformat_close_input(&this->video);
			}
			bool r = this->thread_vidopen();
			this->vidopen = false;
			if (r == 0) {
				printf("load error\n");
				this->openerr = true;
				this->opened = true;
				if (!this->isclone) this->initialized = false;
				this->startframe = 0;
				this->endframe = 0;
				if (this->dummy) {
					this->opened = true;
					while (this->opened) {
						this->endopenvar.notify_one();
					}
				}
				else {
					this->opened = true;
					this->endopenvar.notify_one();
				}
			}
			else {
				this->opened = true;
				this->initialized = false;
				if (this->dummy) {
					this->opened = true;
					while (this->opened) {
						this->endopenvar.notify_one();
					}
				}
				else {
					this->opened = true;
					this->endopenvar.notify_one();
				}
			}
			continue;
		}
		if (!this->liveinput) {
			if ((!this->initialized and this->decresult->width) or this->filename == "") {
				continue;
			}
			if (this->vidformat != 188 and this->vidformat != 187) {
				if (this->video) {
					if ((int)(this->frame) != this->prevframe or this->type == ELEM_LIVE) {
						get_cpu_frame((int)this->frame, this->prevframe, 0);
					}
				}
				if (this->dummy) {
					this->processed = true;
					while (this->processed) {
						this->enddecodevar.notify_one();
					}
				}
				else {
					this->processed = true;
					this->enddecodevar.notify_one();
				}
				continue;
			}
			if ((int)(this->frame) != this->prevframe) {
				bool ret = this->get_hap_frame();
				if (!ret) {	
					this->get_hap_frame();
				}
			}
		}
		if (this->dummy) {
			this->processed = true;
			while (this->processed) {
				this->enddecodevar.notify_one();
			}
		}
		else {
			this->processed = true;
			this->enddecodevar.notify_one();
		}
		continue;
	}
}

void Layer::open_video(float frame, const std::string &filename, int reset) {
	this->databufready = false;
	//delete this->decresult;
	//this->decresult = new frame_result;
	if (this->boundimage != -1) {
		glDeleteTextures(1, &this->boundimage);
		this->boundimage = -1;
	}

	if (!this->dummy) {
		ShelfElement* elem = this->prevshelfdragelem;
		bool ret = mainmix->set_prevshelfdragelem(this);
		if (!ret and elem->type == ELEM_DECK) {
			if (elem->launchtype == 2) {
				mainmix->mousedeck = elem->nblayers[0]->deck;
				mainmix->do_save_deck(mainprogram->temppath + "tempdeck_lnch.deck", false, false);
				mainmix->open_deck(mainprogram->temppath + "tempdeck_lnch.deck", false);
			}
		}
		else if (!ret and elem->type == ELEM_MIX) {
			if (elem->launchtype == 2) {
				mainmix->do_save_mix(mainprogram->temppath + "tempdeck_lnch.deck", mainprogram->prevmodus, false);
				mainmix->open_mix(mainprogram->temppath + "tempdeck_lnch.deck", false);
			}
		}
		else if (!ret) {
			LoopStation* l;
			if (!mainprogram->prevmodus) {
				l = lpc;
			}
			else {
				l = lp;
			}
			this->layers->erase(std::find(this->layers->begin(), this->layers->end(), this));
			Layer *lay = mainmix->add_layer(*this->layers, this->pos);
			for (int m = 0; m < 2; m++) {
				for (int j = 0; j < this->effects[m].size(); j++) {
					Effect *eff = lay->add_effect(this->effects[m][j]->type, j);
					for (int k = 0; k < this->effects[m][j]->params.size(); k++) {
						Param* par = this->effects[m][j]->params[k];
						Param* cpar = lay->effects[m][j]->params[k];
						LoopStationElement* lpe = l->parelemmap[par];
						if (lpe) {
							for (int i = 0; i < lpe->eventlist.size(); i++) {
								if (std::get<1>(lpe->eventlist[i]) == par) {
									std::get<1>(lpe->eventlist[i]) = cpar;
								}
							}
							cpar->box->acolor[0] = lpe->colbox->acolor[0];
							cpar->box->acolor[1] = lpe->colbox->acolor[1];
							cpar->box->acolor[2] = lpe->colbox->acolor[2];
							cpar->box->acolor[3] = lpe->colbox->acolor[3];
							l->parelemmap[cpar] = lpe;
							l->parelemmap.erase(par);
							lpe->params.erase(par);
							lpe->params.emplace(cpar);
							lpe->params.erase(par);
							lpe->layers.emplace(lay);
							lpe->layers.erase(this);
							l->parmap[par] = l->parmap[cpar];
							l->allparams.erase(std::find(l->allparams.begin(), l->allparams.end(), par));
							l->allparams.push_back(cpar);
						}
						cpar->value = par->value;
						cpar->midi[0] = par->midi[0];
						cpar->midi[1] = par->midi[1];
						cpar->effect = eff;
					}
					Button* but = this->effects[m][j]->onoffbutton;
					Button* cbut = eff->onoffbutton;
					LoopStationElement* lpe = l->butelemmap[but];
					if (lpe) {
						for (int i = 0; i < lpe->eventlist.size(); i++) {
							if (std::get<2>(lpe->eventlist[i]) == but) {
								std::get<2>(lpe->eventlist[i]) = cbut;
							}
						}
						//cbut->box->acolor[0] = lpe->colbox->acolor[0];
						//cbut->box->acolor[1] = lpe->colbox->acolor[1];
						//cbut->box->acolor[2] = lpe->colbox->acolor[2];
						//cbut->box->acolor[3] = lpe->colbox->acolor[3];
						l->butelemmap[cbut] = lpe;
						l->butelemmap.erase(but);
						lpe->buttons.erase(but);
						lpe->buttons.emplace(cbut);
						lpe->buttons.erase(but);
						lpe->layers.emplace(lay);
						lpe->layers.erase(this);
						l->butmap[but] = l->butmap[cbut];
						l->allbuttons.erase(std::find(l->allbuttons.begin(), l->allbuttons.end(), but));
						l->allbuttons.push_back(cbut);
					}
					cbut->value = but->value;
					cbut->midi[0] = but->midi[0];
					cbut->midi[1] = but->midi[1];
				}
			}
			this->inhibit(); // lay is passed over into only framecounting
			mainmix->set_values(lay, this, false);
			if (this == mainmix->currlay) mainmix->currlay = lay;
			lay->open_video(0, filename, true);
			return;
		}
	}

	//this->video_dec_ctx = nullptr;
	this->audioplaying = false;
	if (this->effects[0].size() == 0) this->type = ELEM_FILE;
	else this->type = ELEM_LAYER;
	this->filename = filename;
	if (frame >= 0.0f) this->frame = frame;
	this->prevframe = this->frame - 1;
	this->reset = reset;
	this->skip = false;
	this->ifmt = nullptr;
	this->vidopen = true;
	this->decresult->width = 0;
	this->decresult->compression = 0;
	this->ready = true;
	while (this->ready) {
		this->startdecode.notify_one();
	}
	if (this->clonesetnr != -1) {
		mainmix->clonesets[this->clonesetnr]->erase(this);
		if (mainmix->clonesets[this->clonesetnr]->size() == 0) {
			mainmix->cloneset_destroy(mainmix->clonesets[this->clonesetnr]);
		}
		this->clonesetnr = -1;
	}
}

bool Layer::thread_vidopen() {
	if (this->video) avformat_close_input(&this->video);
	this->video = nullptr;
	this->video_dec_ctx = nullptr;
	this->liveinput = nullptr;
		
	if (!this->skip) {
		bool foundnew = false;
		ptrdiff_t pos = std::find(mainprogram->busylayers.begin(), mainprogram->busylayers.end(), this) - mainprogram->busylayers.begin();
		if (pos < mainprogram->busylayers.size()) {
			foundnew = this->find_new_live_base(pos);
			if (!foundnew and this->filename != mainprogram->busylist[pos]) {
				avformat_close_input(&mainprogram->busylayers[pos]->video);
				mainprogram->busylayers.erase(mainprogram->busylayers.begin() + pos);
				mainprogram->busylist.erase(mainprogram->busylist.begin() + pos);
			}
		}
		pos = std::find(mainprogram->mimiclayers.begin(), mainprogram->mimiclayers.end(), this) - mainprogram->mimiclayers.begin();
		if (pos < mainprogram->mimiclayers.size()) {
			mainprogram->mimiclayers.erase(mainprogram->mimiclayers.begin() + pos);
		}
	}
	
	//av_opt_set_int(this->video, "probesize2", INT_MAX, 0);
	this->video = avformat_alloc_context();
	if (this->type != ELEM_LIVE) this->video->flags |= AVFMT_FLAG_NONBLOCK;
	int r = avformat_open_input(&(this->video), this->filename.c_str(), this->ifmt, nullptr);
	printf("loading... %s\n", this->filename.c_str());
	if (r < 0) {
		this->filename = "";
		this->openerr = true;
		printf("%s\n", "Couldnt open video");
		return 0;
	}

	//av_opt_set_int(this->video, "max_analyze_duration2", 100000000, 0);
	if (avformat_find_stream_info(this->video, nullptr) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        return 0;
    }
	//this->video->max_picture_buffer = 20000000;

	if (open_codec_context(&(this->video_stream_idx), this->video, AVMEDIA_TYPE_VIDEO) >= 0) {
		this->video_stream = this->video->streams[this->video_stream_idx];
		AVCodec* dec = avcodec_find_decoder(this->video_stream->codecpar->codec_id);
		this->vidformat = this->video_stream->codecpar->codec_id;
		this->video_dec_ctx = avcodec_alloc_context3(dec);
		avcodec_parameters_to_context(this->video_dec_ctx, this->video_stream->codecpar);
		avcodec_open2(this->video_dec_ctx, dec, nullptr);
		this->bpp = 4;
		if (this->vidformat == 188 or this->vidformat == 187) {
			this->numf = this->video_stream->nb_frames;
			if (this->numf == 0) {
				this->numf = (double)this->video->duration * (double)this->video_stream->avg_frame_rate.num / (double)this->video_stream->avg_frame_rate.den / (double)1000000.0f;
				this->video_duration = this->video->duration / (1000000.0f * this->video_stream->time_base.num / this->video_stream->time_base.den);
			}
			else this->video_duration = this->video_stream->duration;
			float tbperframe = (float)this->video_stream->duration / (float)this->numf;
			this->millif = tbperframe * (((float)this->video_stream->time_base.num * 1000.0) / (float)this->video_stream->time_base.den);

			this->startframe = 0;
			this->endframe = this->numf;
			if (0) { // this->reset?
				this->frame = 0.0f;
			}
			this->decframe = av_frame_alloc();

			std::mutex flock;
			flock.lock();
			if (this->databuf) free(this->databuf);
			this->databuf = nullptr;
			flock.unlock();

			return 1;
		}
		else if (this->type != ELEM_LIVE) {
			avformat_open_input(&(this->videoseek), this->filename.c_str(), this->ifmt, nullptr);
		}
	}
    else {
    	printf("Error2\n");
    	return 0;
    }

    if (this->rgbframe) {
    	//avcodec_close(this->video_dec_ctx);
    	//avcodec_close(this->audio_dec_ctx);
	    av_frame_free(&this->decframe);
	    av_frame_free(&this->rgbframe);
    }

	if (this->type != ELEM_LIVE) {
		this->numf = this->video_stream->nb_frames;
		if (this->numf == 0) {
			this->numf = (double)this->video->duration * (double)this->video_stream->avg_frame_rate.num / (double)this->video_stream->avg_frame_rate.den / (double)1000000.0f;
			this->video_duration = this->video->duration / (1000000.0f * this->video_stream->time_base.num / this->video_stream->time_base.den);
		}
		else this->video_duration = this->video_stream->duration;
		if (this->reset) {
			this->startframe = 0;
			this->endframe = this->numf - 1;
		}
		//if (this->numf == 0) return 0;	//reminder: implement!
		float tbperframe = (float)this->video_duration / (float)this->numf;
		this->millif = tbperframe * (((float)this->video_stream->time_base.num * 1000.0) / (float)this->video_stream->time_base.den);
	}

	if (open_codec_context(&(this->audio_stream_idx), this->video, AVMEDIA_TYPE_AUDIO) >= 0 and !this->dummy) {
		this->audio_stream = this->video->streams[this->audio_stream_idx];
		AVCodec *dec = avcodec_find_decoder(this->audio_stream->codecpar->codec_id);
        this->audio_dec_ctx = avcodec_alloc_context3(dec);
		avcodec_parameters_to_context(this->audio_dec_ctx, this->audio_stream->codecpar);
		avcodec_open2(this->audio_dec_ctx, dec, nullptr);
		this->sample_rate = this->audio_dec_ctx->sample_rate;
		this->channels = this->audio_dec_ctx->channels;
		this->channels = 1;
		switch (this->audio_dec_ctx->sample_fmt) {
			case AV_SAMPLE_FMT_U8:
				if (this->channels == 1) this->sampleformat = AL_FORMAT_MONO8;
				else this->sampleformat = AL_FORMAT_STEREO8;
				break;
			case AV_SAMPLE_FMT_S16:
				if (this->channels == 1) this->sampleformat = AL_FORMAT_MONO16;
				else this->sampleformat = AL_FORMAT_STEREO16;
				break;
			case AV_SAMPLE_FMT_S32:
				if (this->channels == 1) this->sampleformat = AL_FORMAT_MONO_FLOAT32;
				else this->sampleformat = AL_FORMAT_STEREO_FLOAT32;
				break;
			case AV_SAMPLE_FMT_FLT:
				if (this->channels == 1) this->sampleformat = AL_FORMAT_MONO_FLOAT32;
				else this->sampleformat = AL_FORMAT_STEREO_FLOAT32;
				break;
			case AV_SAMPLE_FMT_U8P:
				if (this->channels == 1) this->sampleformat = AL_FORMAT_MONO8;
				else this->sampleformat = AL_FORMAT_STEREO8;
				break;
			case AV_SAMPLE_FMT_S16P:
				if (this->channels == 1) this->sampleformat = AL_FORMAT_MONO16;
				else this->sampleformat = AL_FORMAT_STEREO16;
				break;
			case AV_SAMPLE_FMT_S32P:
				if (this->channels == 1) this->sampleformat = AL_FORMAT_MONO_FLOAT32;
				else this->sampleformat = AL_FORMAT_STEREO_FLOAT32;
				break;
			case AV_SAMPLE_FMT_FLTP:
				if (this->channels == 1) this->sampleformat = AL_FORMAT_MONO_FLOAT32;
				else this->sampleformat = AL_FORMAT_STEREO_FLOAT32;
				break;
		}
		
		this->audioplaying = true;
    	this->audiot = std::thread{&Layer::playaudio, this};
    	this->audiot.detach();
    }

    this->rgbframe = av_frame_alloc();
    this->rgbframe->format = AV_PIX_FMT_BGRA;
    this->rgbframe->width  = this->video_dec_ctx->width;
    this->rgbframe->height = this->video_dec_ctx->height;
	int storage = av_image_alloc(this->rgbframe->data, this->rgbframe->linesize, this->rgbframe->width, this->rgbframe->height, AV_PIX_FMT_BGRA, 1);
  	this->sws_ctx = sws_getContext
    (
        this->video_dec_ctx->width,
       	this->video_dec_ctx->height,
        this->video_dec_ctx->pix_fmt,
        this->video_dec_ctx->width,
        this->video_dec_ctx->height,
        AV_PIX_FMT_BGRA,
        SWS_BILINEAR,
        nullptr,
        nullptr,
        nullptr);

    this->decframe = av_frame_alloc();
	
    return 1;
}


void Layer::playaudio() {
	
	ALint availBuffers = 0; // Buffers to be recovered
	ALint buffqueued = 0;
	ALuint myBuff; // The buffer we're using
	ALuint buffHolder[128]; // An array to hold catch the unqueued buffers
	bool done = false;
	std::list<ALuint> bufferQueue; // A quick and dirty queue of buffer objects
	ALuint temp[128];
	alGenBuffers(128, &temp[0]);
    // Queue our buffers onto an STL list
	for (int ii = 0; ii < 128; ++ii) {
		bufferQueue.push_back(temp[ii]);
	}
	// Request a source name
	ALuint source[1];
 	alGenSources((ALuint)1, &source[0]);
	// Set the default volume
	alSourcef(source[0], AL_GAIN, this->volume->value);
	// Set the default position of the sound
	alSource3f(source[0], AL_POSITION, 0, 0, 0);
	while (this->audioplaying) {
		std::unique_lock<std::mutex> lock(this->chunklock);
		this->newchunk.wait(lock, [&]{return this->chready;});
		lock.unlock();
		this->chready = false;
		
		while ((this->playbut->value or this->revbut->value) and this->snippets.size()) {
			// Poll for recoverable buffers
			alSourcef(source[0], AL_GAIN, this->volume->value);
			alGetSourcei(source[0], AL_BUFFERS_PROCESSED, &availBuffers);
			if (availBuffers > 0) {
				alSourceUnqueueBuffers(source[0], availBuffers, buffHolder);
				for (int ii=0; ii<availBuffers; ++ii) {
					// Push the recovered buffers back on the queue
					bufferQueue.push_back(buffHolder[ii]);
				}
			}
			alGetSourcei(source[0], AL_BUFFERS_QUEUED, &buffqueued);
			// Stuff the data in a buffer-object
			int count = 0;
			if (this->snippets.size()) {
				char *input = this->snippets.front(); this->snippets.pop_front();
				int pslen = this->pslens.front(); this->pslens.pop_front();
				if (!bufferQueue.empty()) { // We just drop the data if no buffers are available
					myBuff = bufferQueue.front(); bufferQueue.pop_front();
					alBufferData(myBuff, this->sampleformat, input, pslen, this->sample_rate);
					// Queue the buffer
					alSourceQueueBuffers(source[0], 1, &myBuff);
				}
				if (pslen) delete[] input;
			}
				
			// Restart the source if needed
			// (if we take too long and the queue dries up,
			//  the source stops playing).
			ALint sState = 0;
			alGetSourcei(source[0], AL_SOURCE_STATE, &sState);
			if (sState != AL_PLAYING) {
				alSourcePlay(source[0]);
				alSourcei(source[0], AL_LOOPING, AL_FALSE);	
			}
			float fac = mainmix->deckspeed[!mainprogram->prevmodus][this->deck]->value;
			if (this->clonesetnr != -1) {
				std::unordered_set<Layer*>::iterator it;
				for (it = mainmix->clonesets[this->clonesetnr]->begin(); it != mainmix->clonesets[this->clonesetnr]->end(); it++) {
					Layer *lay = *it;
					if (lay->deck == !this->deck) {
						fac *= mainmix->deckspeed[!mainprogram->prevmodus][!this->deck]->value;
						break;
					}
				}
			}
			alSourcef(source[0], AL_PITCH, this->speed->value * fac * fac);	
		}
	}
}


float tf(float vtxcoord) {
	return 1.5f * vtxcoord;
}


Shelf::Shelf(bool side) {
	this->side = side;
	for (int i = 0; i < 16; i++) {
		this->buttons.push_back(new Button(false));
		this->elements.push_back(new ShelfElement(side, i, this->buttons.back()));
	}
}

ShelfElement::ShelfElement(bool side, int pos, Button *but) {
	float boxwidth = 0.1f;
	this->path = "";
	this->type = ELEM_FILE;
	glGenTextures(1, &this->tex);
	glBindTexture(GL_TEXTURE_2D, this->tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glGenTextures(1, &this->oldtex);
	glBindTexture(GL_TEXTURE_2D, this->oldtex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	this->button = but;
	this->button->toggle = false;
	Box* box = this->button->box;
	box->vtxcoords->x1 = -1.0f + (pos % 4) * boxwidth + (2.0f - boxwidth * 4) * side;
	box->vtxcoords->h = boxwidth * (glob->w / glob->h) / (1920.0f / 1080.0f);
	box->vtxcoords->y1 = -1.0f + (int)(3 - (pos / 4)) * box->vtxcoords->h;
	box->vtxcoords->w = boxwidth;
	box->upvtxtoscr();
	box->tooltiptitle = "Video launch shelf";
	box->tooltip = "Shelf containing up to 16 videos/layerfiles for quick and easy video launching.  Left drag'n'drop from other areas, both videos and layerfiles.  Rightdrag of videos to layers launches with empty effect stack.  Rightclick launches shelf menu. ";
	this->sbox = new Box;
	this->sbox->vtxcoords->x1 = box->vtxcoords->x1;
	this->sbox->vtxcoords->y1 = box->vtxcoords->y1 + 0.05f + tf(0.006f);
	this->sbox->vtxcoords->w = tf(0.005f);
	this->sbox->vtxcoords->h = tf(0.012f);
	this->sbox->upvtxtoscr();
	this->sbox->tooltiptitle = "Restart when triggered";
	this->sbox->tooltip = "When this video is put in the mix, either through MIDI or dragging, the video will restart from the beginning. ";
	this->pbox = new Box;
	this->pbox->vtxcoords->x1 = box->vtxcoords->x1;
	this->pbox->vtxcoords->y1 = box->vtxcoords->y1 + 0.05f - tf(0.006f);
	this->pbox->vtxcoords->w = tf(0.005f);
	this->pbox->vtxcoords->h = tf(0.012f);
	this->pbox->upvtxtoscr();
	this->pbox->tooltiptitle = "Continue when triggered";
	this->pbox->tooltip = "When this video is put in the mix, either through MIDI or dragging, the video will continue from where it was last stopped . ";
	this->cbox = new Box;
	this->cbox->vtxcoords->x1 = box->vtxcoords->x1;
	this->cbox->vtxcoords->y1 = box->vtxcoords->y1 + 0.05f - tf(0.018f);
	this->cbox->vtxcoords->w = tf(0.005f);
	this->cbox->vtxcoords->h = tf(0.012f);
	this->cbox->upvtxtoscr();
	this->cbox->tooltiptitle = "Catch up when triggered";
	this->cbox->tooltip = "When this video is put in the mix, either through MIDI or dragging, the video will continue from where it would have been if it had been virtually continuously playing . ";
}


void set_fbo() {

	glGenTextures(1, &mainmix->mixbackuptex);
	glBindTexture(GL_TEXTURE_2D, mainmix->mixbackuptex);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, glob->w, glob->h);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// tbo for quad color transfer to shader
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &mainprogram->maxtexes);
	mainprogram->maxtexes = 32;   // reminder card#
	glGenBuffers(1, &mainprogram->boxcoltbo);
	glBindBuffer(GL_TEXTURE_BUFFER, mainprogram->boxcoltbo);
	glBufferData(GL_TEXTURE_BUFFER, 1024 * 4, nullptr, GL_DYNAMIC_DRAW);
	glGenBuffers(1, &mainprogram->boxtextbo);
	glBindBuffer(GL_TEXTURE_BUFFER, mainprogram->boxtextbo);
	glBufferData(GL_TEXTURE_BUFFER, 1024, nullptr, GL_DYNAMIC_DRAW);
	GLint boxcolSampler = glGetUniformLocation(mainprogram->ShaderProgram, "boxcolSampler");
	glUniform1i(boxcolSampler, mainprogram->maxtexes - 2);
	glActiveTexture(GL_TEXTURE0 + mainprogram->maxtexes - 2);
	glGenTextures(1, &mainprogram->bdcoltex);
	glBindTexture(GL_TEXTURE_BUFFER, mainprogram->bdcoltex);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA8, mainprogram->boxcoltbo);
	GLint boxtexSampler = glGetUniformLocation(mainprogram->ShaderProgram, "boxtexSampler");
	glUniform1i(boxtexSampler, mainprogram->maxtexes - 1);
	glActiveTexture(GL_TEXTURE0 + mainprogram->maxtexes - 1);
	glGenTextures(1, &mainprogram->bdtextex);
	glBindTexture(GL_TEXTURE_BUFFER, mainprogram->bdtextex);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_R8UI, mainprogram->boxtextbo);


	glGenTextures(1, &mainprogram->fbotex[0]); //comp = false
    glGenTextures(1, &mainprogram->fbotex[1]);
   	glGenTextures(1, &mainprogram->fbotex[2]); //comp = true
    glGenTextures(1, &mainprogram->fbotex[3]);
    glBindTexture(GL_TEXTURE_2D, mainprogram->fbotex[0]);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, mainprogram->ow3, mainprogram->oh3);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, mainprogram->fbotex[1]);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, mainprogram->ow3, mainprogram->oh3);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, mainprogram->fbotex[2]);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, mainprogram->ow, mainprogram->oh);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, mainprogram->fbotex[3]);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, mainprogram->ow, mainprogram->oh);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glGenFramebuffers(1, &mainprogram->frbuf[0]);
	glGenFramebuffers(1, &mainprogram->frbuf[1]);
	glGenFramebuffers(1, &mainprogram->frbuf[2]);
	glGenFramebuffers(1, &mainprogram->frbuf[3]);
	glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->frbuf[0]);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mainprogram->fbotex[0], 0);
	glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->frbuf[1]);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mainprogram->fbotex[1], 0);
	glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->frbuf[2]);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mainprogram->fbotex[2], 0);
	glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->frbuf[3]);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mainprogram->fbotex[3], 0);


	GLfloat vcoords[8];
	GLfloat *p = vcoords;
	*p++ = -1.0f; *p++ = -1.0f;
	*p++ = -1.0f; *p++ = 1.0f;
	*p++ = 1.0f; *p++ = -1.0f;
	*p++ = 1.0f; *p++ = 1.0f;

	GLfloat tcoords[] = {0.0f, 0.0f,
						0.0f, 1.0f,
						1.0f, 0.0f,
						1.0f, 1.0f};
					
	GLuint vbuf;	
    glGenBuffers(1, &vbuf);
	glBindBuffer(GL_ARRAY_BUFFER, vbuf);
    glBufferData(GL_ARRAY_BUFFER, 32, vcoords, GL_STATIC_DRAW);
	GLuint tbuf;
    glGenBuffers(1, &tbuf);
	glBindBuffer(GL_ARRAY_BUFFER, tbuf);
    glBufferData(GL_ARRAY_BUFFER, 32, tcoords, GL_STATIC_DRAW);
	glGenVertexArrays(1, &mainprogram->fbovao);
	glBindVertexArray(mainprogram->fbovao);
	glBindBuffer(GL_ARRAY_BUFFER, vbuf);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
	glBindBuffer(GL_ARRAY_BUFFER, tbuf);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
	
	
	glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
	mainprogram->drawbuffer = mainprogram->globfbo;
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	
	// record buffer
	glGenBuffers(1, &mainmix->ioBuf);
	
	glGenTextures(1, &binsmain->binelpreviewtex);
	glBindTexture(GL_TEXTURE_2D, binsmain->binelpreviewtex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	//dragvao
	GLfloat tcoords2[8];
	p = tcoords2;
	*p++ = 0.0f; *p++ = 1.0f;
	*p++ = 1.0f; *p++ = 1.0f;
	*p++ = 0.0f; *p++ = 0.0f;
	*p++ = 1.0f; *p++ = 0.0f;
	GLuint tmbuf;
	glGenBuffers(1, &thmvbuf);
	glBindBuffer(GL_ARRAY_BUFFER, thmvbuf);
	glBufferData(GL_ARRAY_BUFFER, 32, vcoords, GL_STREAM_DRAW);
	glGenBuffers(1, &tmbuf);
	glBindBuffer(GL_ARRAY_BUFFER, tmbuf);
	glBufferData(GL_ARRAY_BUFFER, 32, tcoords2, GL_STATIC_DRAW);
	glGenVertexArrays(1, &thmvao);
	glBindVertexArray(thmvao);
	glBindBuffer(GL_ARRAY_BUFFER, thmvbuf);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
	glBindBuffer(GL_ARRAY_BUFFER, tmbuf);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8, nullptr);

	glGenBuffers(1, &mainprogram->boxvbuf);
	glGenBuffers(1, &mainprogram->boxtbuf);
	glGenVertexArrays(1, &mainprogram->boxvao);
	glBindVertexArray(mainprogram->boxvao);
	glBindBuffer(GL_ARRAY_BUFFER, mainprogram->boxvbuf);
	glBufferData(GL_ARRAY_BUFFER, 48, nullptr, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12, nullptr);
	glBindBuffer(GL_ARRAY_BUFFER, mainprogram->boxtbuf);
	glBufferData(GL_ARRAY_BUFFER, 32, nullptr, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
	SDL_GL_MakeCurrent(mainprogram->prefwindow, glc_pr);
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, smw, smh);
	glGenFramebuffers(1, &mainprogram->prfbo);
	glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->prfbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
	glGenBuffers(1, &mainprogram->prboxvbuf);
	glGenBuffers(1, &mainprogram->prboxtbuf);
	glGenVertexArrays(1, &mainprogram->prboxvao);
	glBindVertexArray(mainprogram->prboxvao);
	glBindBuffer(GL_ARRAY_BUFFER, mainprogram->prboxvbuf);
	glBufferData(GL_ARRAY_BUFFER, 48, vcoords, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12, nullptr);
	glBindBuffer(GL_ARRAY_BUFFER, mainprogram->prboxtbuf);
	glBufferData(GL_ARRAY_BUFFER, 32, tcoords2, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
	glGenVertexArrays(1, &mainprogram->pr_texvao);
	glBindVertexArray(mainprogram->pr_texvao);
	glGenBuffers(1, &mainprogram->pr_rtvbo);
	glBindBuffer(GL_ARRAY_BUFFER, mainprogram->pr_rtvbo);
	glBufferStorage(GL_ARRAY_BUFFER, 48, nullptr, GL_DYNAMIC_STORAGE_BIT);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12, nullptr);
	glGenBuffers(1, &mainprogram->pr_rttbo);
	glBindBuffer(GL_ARRAY_BUFFER, mainprogram->pr_rttbo);
	glBufferStorage(GL_ARRAY_BUFFER, 32, nullptr, GL_DYNAMIC_STORAGE_BIT);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
	SDL_GL_MakeCurrent(mainprogram->config_midipresetswindow, glc_tm);
	glGenBuffers(1, &mainprogram->tmboxvbuf);
	glGenBuffers(1, &mainprogram->tmboxtbuf);
	glGenVertexArrays(1, &mainprogram->tmboxvao);
	glBindVertexArray(mainprogram->tmboxvao);
	glBindBuffer(GL_ARRAY_BUFFER, mainprogram->tmboxvbuf);
	glBufferData(GL_ARRAY_BUFFER, 48, vcoords, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12, nullptr);
	glBindBuffer(GL_ARRAY_BUFFER, mainprogram->tmboxtbuf);
	glBufferData(GL_ARRAY_BUFFER, 32, tcoords2, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
	glGenVertexArrays(1, &mainprogram->tm_texvao);
	glBindVertexArray(mainprogram->tm_texvao);
	glGenBuffers(1, &mainprogram->tm_rtvbo);
	glBindBuffer(GL_ARRAY_BUFFER, mainprogram->tm_rtvbo);
	glBufferStorage(GL_ARRAY_BUFFER, 48, nullptr, GL_DYNAMIC_STORAGE_BIT);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12, nullptr);
	glGenBuffers(1, &mainprogram->tm_rttbo);
	glBindBuffer(GL_ARRAY_BUFFER, mainprogram->tm_rttbo);
	glBufferStorage(GL_ARRAY_BUFFER, 32, nullptr, GL_DYNAMIC_STORAGE_BIT);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
	SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
	mainprogram->bvao = mainprogram->boxvao;
	mainprogram->bvbuf = mainprogram->boxvbuf;
	mainprogram->btbuf = mainprogram->boxtbuf;

	glGenVertexArrays(1, &mainprogram->texvao);
	glBindVertexArray(mainprogram->texvao);
	glGenBuffers(1, &mainprogram->rtvbo);
	glBindBuffer(GL_ARRAY_BUFFER, mainprogram->rtvbo);
	glBufferStorage(GL_ARRAY_BUFFER, 48, nullptr, GL_DYNAMIC_STORAGE_BIT);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12, nullptr);
	glGenBuffers(1, &mainprogram->rttbo);
	glBindBuffer(GL_ARRAY_BUFFER, mainprogram->rttbo);
	glBufferStorage(GL_ARRAY_BUFFER, 32, nullptr, GL_DYNAMIC_STORAGE_BIT);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8, nullptr);

	//set boxdata buffers
	glGenVertexArrays(1, &mainprogram->bdvao);
	glBindVertexArray(mainprogram->bdvao);
	glGenBuffers(1, &mainprogram->bdvbo);
	glBindBuffer(GL_ARRAY_BUFFER, mainprogram->bdvbo);
	glBufferData(GL_ARRAY_BUFFER, 65536, nullptr, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12, nullptr);
	glGenBuffers(1, &mainprogram->bdtcbo);
	glBindBuffer(GL_ARRAY_BUFFER, mainprogram->bdtcbo);
	glBufferData(GL_ARRAY_BUFFER, 65536, nullptr, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8, nullptr);

	glGenBuffers(1, &mainprogram->bdibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mainprogram->bdibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 12288, nullptr, GL_STATIC_DRAW);
}


void register_line_draw(float* linec, float x1, float y1, float x2, float y2) {
	register_line_draw(linec, x1, y1, x2, y2, false);
}

void register_line_draw(float* linec, float x1, float y1, float x2, float y2, bool directdraw) {
	gui_line* line = new gui_line;
	line->linec[0] = linec[0];
	line->linec[1] = linec[1];
	line->linec[2] = linec[2];
	line->linec[3] = linec[3];
	line->x1 = x1;
	line->y1 = y1;
	line->x2 = x2;
	line->y2 = y2;
	GUI_Element* gelem = new GUI_Element;
	gelem->type = GUI_LINE;
	gelem->line = line;
	if (directdraw) draw_line(gelem->line);
	else mainprogram->guielems.push_back(gelem);
}

void draw_line(gui_line *line) {
	GLint linetriangle = glGetUniformLocation(mainprogram->ShaderProgram, "linetriangle");
	glUniform1i(linetriangle, 1);
	GLint color = glGetUniformLocation(mainprogram->ShaderProgram, "color");
	glUniform4fv(color, 1, line->linec);
	GLfloat fvcoords[6] = { line->x1, line->y1, 1.0f, line->x2, line->y2, 1.0f};
 	GLuint fvbuf, fvao;
    glGenBuffers(1, &fvbuf);
	glBindBuffer(GL_ARRAY_BUFFER, fvbuf);
    glBufferData(GL_ARRAY_BUFFER, 24, fvcoords, GL_STATIC_DRAW);
	glGenVertexArrays(1, &fvao);
	glBindVertexArray(fvao);
	glBindBuffer(GL_ARRAY_BUFFER, fvbuf);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12, nullptr);
	glDrawArrays(GL_LINES, 0, 2);
	glUniform1i(linetriangle, 0);
	
    glDeleteBuffers(1, &fvbuf);
	glDeleteVertexArrays(1, &fvao);
}

void draw_direct(float* linec, float* areac, float x, float y, float wi, float he, float dx, float dy, float scale, float opacity, int circle, GLuint tex, float smw, float smh, bool vertical) {
	GLint drawcircle = 0;
	GLfloat circleradius = 0;
	GLfloat cirx = 0;
	GLfloat ciry = 0;
	GLint down = 0;
	if (circle) {
		drawcircle = glGetUniformLocation(mainprogram->ShaderProgram, "circle");
		circleradius = glGetUniformLocation(mainprogram->ShaderProgram, "circleradius");
		cirx = glGetUniformLocation(mainprogram->ShaderProgram, "cirx");
		ciry = glGetUniformLocation(mainprogram->ShaderProgram, "ciry");
	}
	else if (tex != -1) {
		down = glGetUniformLocation(mainprogram->ShaderProgram, "down");
		glUniform1i(down, 1);
	}
	GLint box;
	if (areac) {
		box = glGetUniformLocation(mainprogram->ShaderProgram, "box");
		glUniform1i(box, 1);
	}

	GLint pixelw = glGetUniformLocation(mainprogram->ShaderProgram, "pixelw");
	GLint pixelh = 0;
	if (linec) {
		GLfloat lcolor = glGetUniformLocation(mainprogram->ShaderProgram, "lcolor");
		glUniform4fv(lcolor, 1, linec);
		if (tex == -1) {
			GLint m_viewport[4];
			glGetIntegerv(GL_VIEWPORT, m_viewport);
			glUniform1f(pixelw, 2.0f / (wi * ((float)(m_viewport[2] - m_viewport[0]) / 2.0f)));
			pixelh = glGetUniformLocation(mainprogram->ShaderProgram, "pixelh");
			glUniform1f(pixelh, 2.0f / (he * ((float)(m_viewport[3] - m_viewport[1]) / 2.0f)));
		}
	}
	else {
		glUniform1f(pixelw, 0.0f);
	}

	GLint opa = glGetUniformLocation(mainprogram->ShaderProgram, "opacity");
	glUniform1f(opa, opacity);

	if (areac) {
		float shx = -dx * 6.0f;
		float shy = -dy * 6.0f;
		if (!vertical) {
			GLfloat fvcoords2[12] = {
				x,     y + he, 1.0,
				x,     y, 1.0,
				x + wi, y + he, 1.0,
				x + wi, y, 1.0,
			};
			glBindBuffer(GL_ARRAY_BUFFER, mainprogram->bvbuf);
			glBufferSubData(GL_ARRAY_BUFFER, 0, 48, fvcoords2);
		}
		else {
			he /= 2.0f;
			wi *= 2.0f;
			GLfloat fvcoords2[12] = {
				x + he,     y - wi, 1.0,
				x,     y - wi, 1.0,
				x + he, y, 1.0,
				x , y, 1.0,
			};
			glBindBuffer(GL_ARRAY_BUFFER, mainprogram->bvbuf);
			glBufferSubData(GL_ARRAY_BUFFER, 0, 48, fvcoords2);
		}
		GLfloat color = glGetUniformLocation(mainprogram->ShaderProgram, "color");
		glUniform4fv(color, 1, areac);
		if (tex != -1) {
			GLfloat tcoords[8];
			GLfloat* p = tcoords;
			if (scale != 1.0f or dx != 0.0f or dy != 0.0f) {
				*p++ = ((0.0f) - 0.5f) * scale + 0.5f + shx; *p++ = ((0.0f) - 0.5f) * scale + 0.5f + shy;
				*p++ = ((0.0f) - 0.5f) * scale + 0.5f + shx; *p++ = ((1.0f) - 0.5f) * scale + 0.5f + shy;
				*p++ = ((1.0f) - 0.5f) * scale + 0.5f + shx; *p++ = ((0.0f) - 0.5f) * scale + 0.5f + shy;
				*p++ = ((1.0f) - 0.5f) * scale + 0.5f + shx; *p++ = ((1.0f) - 0.5f) * scale + 0.5f + shy;
			}
			else {
				*p++ = 0.0f; 
				*p++ = 0.0f;
				*p++ = 0.0f;
				*p++ = 1.0f;
				*p++ = 1.0f;
				*p++ = 0.0f;
				*p++ = 1.0f;
				*p++ = 1.0f;
			}
			glBindBuffer(GL_ARRAY_BUFFER, mainprogram->btbuf);
			glBufferSubData(GL_ARRAY_BUFFER, 0, 32, tcoords);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, tex);
			glUniform1i(box, 0);
			glUniform1i(down, 1);
		}
		if (circle) {
			glUniform1i(box, 0);
			glUniform1i(drawcircle, circle);
			if (mainprogram->insmall) {
				glUniform1f(circleradius, (wi / 4.0f) * smh);
				glUniform1f(cirx, ((x + (wi / 2.0f)) / 2.0f + 0.5f) * smw);
				glUniform1f(ciry, ((y + (wi / 2.0f)) / 2.0f + 0.5f) * smh);
			}
			else {
				glUniform1f(circleradius, (wi / 4.0f) * glob->h);
				glUniform1f(cirx, ((x + (wi / 2.0f)) / 2.0f + 0.5f) * glob->w);
				glUniform1f(ciry, ((y + (wi / 2.0f)) / 2.0f + 0.5f) * glob->h);
			}
		}
		glBindVertexArray(mainprogram->bvao);
		glEnable(GL_BLEND);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		// border is drawn in shader
		if (tex != -1) glUniform1i(down, 0);
		if (circle) glUniform1i(drawcircle, 0);
	}

	if (areac) glUniform1i(box, 0);
	glUniform1f(opa, 1.0f);
}

void draw_box(Box *box, GLuint tex) {
	draw_box(box->lcolor, box->acolor, box->vtxcoords->x1, box->vtxcoords->y1, box->vtxcoords->w, box->vtxcoords->h, 0.0f, 0.0f, 1.0f, 1.0f, 0, tex, glob->w, glob->h, 0);
}

void draw_box(float *linec, float *areac, Box *box, GLuint tex) {
	draw_box(linec, areac, box->vtxcoords->x1, box->vtxcoords->y1, box->vtxcoords->w, box->vtxcoords->h, 0.0f, 0.0f, 1.0f, 1.0f, 0, tex, glob->w, glob->h, 0);
}

void draw_box(Box *box, float opacity, GLuint tex) {
	draw_box(box->lcolor, box->acolor, box->vtxcoords->x1, box->vtxcoords->y1, box->vtxcoords->w, box->vtxcoords->h, 0.0f, 0.0f, 1.0f, opacity, 0, tex, glob->w, glob->h, 0);
}

void draw_box(Box *box, float dx, float dy, float scale, GLuint tex) {
	draw_box(box->lcolor, box->acolor, box->vtxcoords->x1, box->vtxcoords->y1, box->vtxcoords->w, box->vtxcoords->h, dx, dy, scale, 1.0f, 0, tex, glob->w, glob->h, 0);
}

void draw_box(float* linec, float* areac, float x, float y, float wi, float he, GLuint tex) {
	draw_box(linec, areac, x, y, wi, he, 0.0f, 0.0f, 1.0f, 1.0f, 0, tex, glob->w, glob->h, 0);
}

void draw_box(float* linec, float* areac, float x, float y, float wi, float he, GLuint tex, bool text, bool vertical) {
	draw_box(linec, areac, x, y, wi, he, 0.0f, 0.0f, 1.0f, 1.0f, 0, tex, glob->w, glob->h, text, vertical);
}

void draw_box(float *color, float x, float y, float radius, int circle) {
	draw_box(nullptr, color, x - radius, y - radius, radius * 2.0f, (radius * 2.0f), 0.0f, 0.0f, 1.0f, 1.0f, circle, -1, glob->w, glob->h, 0);
}

void draw_box(float *color, float x, float y, float radius, int circle, float smw, float smh) {
	draw_box(nullptr, color, x - radius, y - radius, radius * 2.0f, (radius * 2.0f), 0.0f, 0.0f, 1.0f, 1.0f, circle, -1, smw, smh, 0);
}

void draw_box(float* linec, float* areac, float x, float y, float wi, float he, float dx, float dy, float scale, float opacity, int circle, GLuint tex, float smw, float smh, bool text) {
	draw_box(linec, areac, x, y, wi, he, dx, dy, scale, opacity, circle, tex, smw, smh, text, false);
}

void draw_box(float* linec, float* areac, float x, float y, float wi, float he, float dx, float dy, float scale, float opacity, int circle, GLuint tex, float smw, float smh, bool text, bool vertical) {
	if (mainprogram->drawbuffer != mainprogram->globfbo or !mainprogram->startloop or mainprogram->directmode) {
		GLuint textmode = glGetUniformLocation(mainprogram->ShaderProgram, "textmode");
		if (text and !circle) {
			glUniform1i(textmode, 1);
		}
		draw_direct(linec, areac, x, y, wi, he, dx, dy, scale, opacity, circle, tex, smw, smh, vertical);
		if (text and !circle) {
			glUniform1i(textmode, 0);
		}
		return;
	}

	if (circle or mainprogram->frontbatch or text) {
		gui_box *box = new gui_box;
		if (linec) {
			box->linec[0] = linec[0];
			box->linec[1] = linec[1];
			box->linec[2] = linec[2];
			box->linec[3] = linec[3];
		}
		if (areac) {
			box->areac[0] = areac[0];
			box->areac[1] = areac[1];
			box->areac[2] = areac[2];
			box->areac[3] = areac[3];
		}
		box->x = x;
		box->y = y;
		box->wi = wi;
		box->he = he;
		box->circle = circle;
		box->tex = tex;
		box->text = text;
		box->vertical = vertical;
		GUI_Element *gelem = new GUI_Element;
		gelem->type = GUI_BOX;
		gelem->box = box;
		mainprogram->guielems.push_back(gelem);
		return;
	}

	if (linec) {
		// draw border
		draw_box(nullptr, linec, x, y, wi, he, dx, dy, scale, opacity, false, -1, smw, smh, false);
		x += 0.001f;
		y += 0.00185f;
		wi -= 0.002f;
		he -= 0.0037f;
	}

	// gather data for boxes drawn on globfbo -> draw in batches
	if (tex != -1) mainprogram->countingtexes[mainprogram->currbatch]++;
	*mainprogram->bdtnptr[mainprogram->currbatch]++ = tex;
	
	if (text) *mainprogram->bdtptr[mainprogram->currbatch]++ = 128 + mainprogram->countingtexes[mainprogram->currbatch] - 1;
	else if (tex != -1) *mainprogram->bdtptr[mainprogram->currbatch]++ = mainprogram->countingtexes[mainprogram->currbatch] - 1;
	else *mainprogram->bdtptr[mainprogram->currbatch]++ = 127;

	mainprogram->boxz += 0.001f;
	if (!vertical) {
		*mainprogram->bdvptr[mainprogram->currbatch]++ = x;
		*mainprogram->bdvptr[mainprogram->currbatch]++ = y + he;
		*mainprogram->bdvptr[mainprogram->currbatch]++ = 1.0f - mainprogram->boxz;
		*mainprogram->bdvptr[mainprogram->currbatch]++ = x;
		*mainprogram->bdvptr[mainprogram->currbatch]++ = y;
		*mainprogram->bdvptr[mainprogram->currbatch]++ = 1.0f - mainprogram->boxz;
		*mainprogram->bdvptr[mainprogram->currbatch]++ = x + wi;
		*mainprogram->bdvptr[mainprogram->currbatch]++ = y + he;
		*mainprogram->bdvptr[mainprogram->currbatch]++ = 1.0f - mainprogram->boxz;
		*mainprogram->bdvptr[mainprogram->currbatch]++ = x + wi;
		*mainprogram->bdvptr[mainprogram->currbatch]++ = y;
		*mainprogram->bdvptr[mainprogram->currbatch]++ = 1.0f - mainprogram->boxz;
	}
	else {
		*mainprogram->bdvptr[mainprogram->currbatch]++ = x + wi;
		*mainprogram->bdvptr[mainprogram->currbatch]++ = y;
		*mainprogram->bdvptr[mainprogram->currbatch]++ = 1.0f - mainprogram->boxz;
		*mainprogram->bdvptr[mainprogram->currbatch]++ = x;
		*mainprogram->bdvptr[mainprogram->currbatch]++ = y;
		*mainprogram->bdvptr[mainprogram->currbatch]++ = 1.0f - mainprogram->boxz;
		*mainprogram->bdvptr[mainprogram->currbatch]++ = x + wi;
		*mainprogram->bdvptr[mainprogram->currbatch]++ = y + he;
		*mainprogram->bdvptr[mainprogram->currbatch]++ = 1.0f - mainprogram->boxz;
		*mainprogram->bdvptr[mainprogram->currbatch]++ = x;
		*mainprogram->bdvptr[mainprogram->currbatch]++ = y + he;
		*mainprogram->bdvptr[mainprogram->currbatch]++ = 1.0f - mainprogram->boxz;
	}

	if (tex != -1 and (scale != 1.0f or dx != 0.0f or dy != 0.0f)) {
		float shx = -dx * 6.0f;
		float shy = -dy * 6.0f;
		*mainprogram->bdtcptr[mainprogram->currbatch]++ = ((0.0f) - 0.5f) * scale + 0.5f + shx;
		*mainprogram->bdtcptr[mainprogram->currbatch]++ = ((0.0f) - 0.5f) * scale + 0.5f + shy;
		*mainprogram->bdtcptr[mainprogram->currbatch]++ = ((0.0f) - 0.5f) * scale + 0.5f + shx;
		*mainprogram->bdtcptr[mainprogram->currbatch]++ = ((1.0f) - 0.5f) * scale + 0.5f + shy;
		*mainprogram->bdtcptr[mainprogram->currbatch]++ = ((1.0f) - 0.5f) * scale + 0.5f + shx;
		*mainprogram->bdtcptr[mainprogram->currbatch]++ = ((0.0f) - 0.5f) * scale + 0.5f + shy;
		*mainprogram->bdtcptr[mainprogram->currbatch]++ = ((1.0f) - 0.5f) * scale + 0.5f + shx;
		*mainprogram->bdtcptr[mainprogram->currbatch]++ = ((1.0f) - 0.5f) * scale + 0.5f + shy;
	}
	else {
		*mainprogram->bdtcptr[mainprogram->currbatch]++ = 0.0f;
		*mainprogram->bdtcptr[mainprogram->currbatch]++ = 0.0f;
		*mainprogram->bdtcptr[mainprogram->currbatch]++ = 0.0f;
		*mainprogram->bdtcptr[mainprogram->currbatch]++ = 1.0f;
		*mainprogram->bdtcptr[mainprogram->currbatch]++ = 1.0f;
		*mainprogram->bdtcptr[mainprogram->currbatch]++ = 0.0f;
		*mainprogram->bdtcptr[mainprogram->currbatch]++ = 1.0f;
		*mainprogram->bdtcptr[mainprogram->currbatch]++ = 1.0f;
	}

	if (areac) {
		*mainprogram->bdcptr[mainprogram->currbatch]++ = (char)(areac[0] * 255.0f);
		*mainprogram->bdcptr[mainprogram->currbatch]++ = (char)(areac[1] * 255.0f);
		*mainprogram->bdcptr[mainprogram->currbatch]++ = (char)(areac[2] * 255.0f);
		*mainprogram->bdcptr[mainprogram->currbatch]++ = (char)(areac[3] * 255.0f);
	}
	else {
		*mainprogram->bdcptr[mainprogram->currbatch]++ = 0;
		*mainprogram->bdcptr[mainprogram->currbatch]++ = 0;
		*mainprogram->bdcptr[mainprogram->currbatch]++ = 0;
		*mainprogram->bdcptr[mainprogram->currbatch]++ = 0;
	}

	if (mainprogram->countingtexes[mainprogram->currbatch] == mainprogram->maxtexes - 8) {
		mainprogram->currbatch++;
		mainprogram->bdvptr[mainprogram->currbatch] = mainprogram->bdcoords[mainprogram->currbatch];
		mainprogram->bdtcptr[mainprogram->currbatch] = mainprogram->bdtexcoords[mainprogram->currbatch];
		mainprogram->bdcptr[mainprogram->currbatch] = mainprogram->bdcolors[mainprogram->currbatch];
		mainprogram->bdtptr[mainprogram->currbatch] = mainprogram->bdtexes[mainprogram->currbatch];
		mainprogram->bdtnptr[mainprogram->currbatch] = mainprogram->boxtexes[mainprogram->currbatch];
		mainprogram->countingtexes[mainprogram->currbatch] = 0;
	}
}

void register_triangle_draw(float* linec, float* areac, float x1, float y1, float xsize, float ysize, ORIENTATION orient, TRIANGLE_TYPE type) {
	register_triangle_draw(linec, areac, x1, y1, xsize, ysize, orient, type, false);
}

void register_triangle_draw(float* linec, float* areac, float x1, float y1, float xsize, float ysize, ORIENTATION orient, TRIANGLE_TYPE type, bool directdraw) {
	gui_triangle* triangle = new gui_triangle;
	triangle->linec[0] = linec[0];
	triangle->linec[1] = linec[1];
	triangle->linec[2] = linec[2];
	triangle->linec[3] = linec[3];
	triangle->areac[0] = areac[0];
	triangle->areac[1] = areac[1];
	triangle->areac[2] = areac[2];
	triangle->areac[3] = areac[3];
	triangle->x1 = x1;
	triangle->y1 = y1;
	triangle->xsize = xsize;
	triangle->ysize = ysize;
	triangle->orient = orient;
	triangle->type = type;
	GUI_Element* gelem = new GUI_Element;
	gelem->type = GUI_TRIANGLE;
	gelem->triangle = triangle;
	if (directdraw) draw_triangle(gelem->triangle);
	else mainprogram->guielems.push_back(gelem);
}

void draw_triangle(gui_triangle *triangle) {

	GLint linetriangle = glGetUniformLocation(mainprogram->ShaderProgram, "linetriangle");
	glUniform1i(linetriangle, 1);

	GLint color = glGetUniformLocation(mainprogram->ShaderProgram, "color");
	if (triangle->linec) glUniform4fv(color, 1, triangle->linec);

	GLfloat fvcoords[9];
	GLfloat *p;
	switch (triangle->orient) {
		case RIGHT:
			p = fvcoords;
			*p++ = triangle->x1; *p++ = triangle->y1; *p++ = 1.0f;
			*p++ = triangle->x1; *p++ = triangle->y1 + triangle->ysize; *p++ = 1.0f;
			*p++ = triangle->x1 + 0.866f * triangle->xsize; *p++ = triangle->y1 + triangle->ysize / 2.0f; *p++ = 1.0f;
			break;
		case LEFT:
			p = fvcoords;
			*p++ = triangle->x1 + 0.866f * triangle->xsize; *p++ = triangle->y1; *p++ = 1.0f;
			*p++ = triangle->x1 + 0.866f * triangle->xsize; *p++ = triangle->y1 + triangle->ysize; *p++ = 1.0f;
			*p++ = triangle->x1; *p++ = triangle->y1 + triangle->ysize / 2.0f; *p++ = 1.0f;
			break;
		case UP:
			p = fvcoords;
			*p++ = triangle->x1 + triangle->xsize / 2.0f; *p++ = triangle->y1; *p++ = 1.0f;
			*p++ = triangle->x1; *p++ = triangle->y1 + 0.866f * triangle->ysize; *p++ = 1.0f;
			*p++ = triangle->x1 + triangle->xsize; *p++ = triangle->y1 + 0.866f * triangle->ysize; *p++ = 1.0f;
			break;
		case DOWN:
			p = fvcoords;
			*p++ = triangle->x1; *p++ = triangle->y1; *p++ = 1.0f;
			*p++ = triangle->x1 + triangle->xsize; *p++ = triangle->y1; *p++ = 1.0f;
			*p++ = triangle->x1 + triangle->xsize / 2.0f; *p++ = triangle->y1 + 0.866f * triangle->ysize; *p++ = 1.0f;
			break;
	}
 	GLuint fvbuf, fvao;
    glGenBuffers(1, &fvbuf);
	glBindBuffer(GL_ARRAY_BUFFER, fvbuf);
    glBufferData(GL_ARRAY_BUFFER, 36, fvcoords, GL_STATIC_DRAW);
	glGenVertexArrays(1, &fvao);
	glBindVertexArray(fvao);
	glBindBuffer(GL_ARRAY_BUFFER, fvbuf);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12, nullptr);
	if (triangle->type == CLOSED) {
		glUniform4fv(color, 1, triangle->areac);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);
	}
	if (triangle->type == OPEN) glDrawArrays(GL_LINE_LOOP, 0, 3);

	glUniform1i(linetriangle, 0);
    glDeleteBuffers(1, &fvbuf);
	glDeleteVertexArrays(1, &fvao);
}


std::vector<float> render_text(std::string text, float* textc, float x, float y, float sx, float sy) {
	std::vector<float> ret = render_text(text, textc, x, y, sx, sy, 0, 1, 0);
	return ret;
}

std::vector<float> render_text(std::string text, float* textc, float x, float y, float sx, float sy, int smflag) {
	std::vector<float> ret = render_text(text, textc, x, y, sx, sy, smflag, 1, 0);
	return ret;
}

std::vector<float> render_text(std::string text, float* textc, float x, float y, float sx, float sy, int smflag, bool vertical) {
	std::vector<float> ret = render_text(text, textc, x, y, sx, sy, smflag, 1, vertical);
	return ret;
}

std::vector<float> render_text(std::string text, float *textc, float x, float y, float sx, float sy, int smflag, bool display, bool vertical) {
	float bux = x;
	float buy = y;
	std::vector<float> textwsplay;
	if (text == "") return textwsplay;
	float white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	if (smflag == 0) y -= 0.012f;
	if (smflag > 0) y -= 0.02f;
	GLuint texture;
	float wfac;
	float textw = 0.0f;
	float texth = 0.0f;
	bool prepare = true;
	GUIString* gs = nullptr;
	int pos = 0;
	if (smflag == 0) {
		gs = mainprogram->guitextmap[text];
		if (gs) {
			pos = std::find(gs->sxvec.begin(), gs->sxvec.end(), sx) - gs->sxvec.begin();
			if (pos != gs->sxvec.size()) {
				prepare = false;
				texture = gs->texturevec[pos];
				textw = gs->textwvec[pos];
				texth = gs->texthvec[pos];
				textwsplay = gs->textwvecvec[pos];
			}
		}
	}
	else if (smflag == 1) {
		gs = mainprogram->prguitextmap[text];
		if (gs) {
			pos = std::find(gs->sxvec.begin(), gs->sxvec.end(), sx) - gs->sxvec.begin();
			if (pos != gs->sxvec.size()) {
				prepare = false;
				texture = gs->texturevec[pos];
				textw = gs->textwvec[pos];
				texth = gs->texthvec[pos];
				textwsplay = gs->textwvecvec[pos];
			}
		}
	}
	else if (smflag == 2) {
		gs = mainprogram->tmguitextmap[text];
		if (gs) {
			pos = std::find(gs->sxvec.begin(), gs->sxvec.end(), sx) - gs->sxvec.begin();
			if (pos != gs->sxvec.size()) {
				prepare = false;
				texture = gs->texturevec[pos];
				textw = gs->textwvec[pos];
				texth = gs->texthvec[pos];
				textwsplay = gs->textwvecvec[pos];
			}
		}
	}
	
	if (prepare) {
		// compose string texture for display all next times
		const char *t = text.c_str();
		const char *p;

		int w2 = 0;
		int h2 = 0;
		if (smflag == 0) {
			w2 = glob->w;
			h2 = glob->h;
		}
		else {
			w2 = smw;
			h2 = smh;
		}
		int psize = (int)(sy * 24000.0f * h2 / 1346.0f);
		glGenTextures(1, &texture);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexStorage2D(
			GL_TEXTURE_2D,
			1,
			GL_R8,
			2048,
			psize * 3
			);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		
		GLfloat texvcoords1[8] = {
			-1.0f,     -1.0f,
			1.0f , -1.0f,
			-1.0f,     1.0f,
			1.0f, 1.0f};

		GLfloat textcoords[] = {0.0f, 0.0f,
							1.0f, 0.0f,
							0.0f, 1.0f,
							1.0f, 1.0f};
		GLuint texfrbuf, texvbuf;
		glGenFramebuffers(1, &texfrbuf);
		glBindFramebuffer(GL_FRAMEBUFFER, texfrbuf);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);
		glGenBuffers(1, &texvbuf);
		glBindBuffer(GL_ARRAY_BUFFER, texvbuf);
		GLuint texvao, textbuf;
		glGenVertexArrays(1, &texvao);
		glGenBuffers(1, &textbuf);
		glBindBuffer(GL_ARRAY_BUFFER, texvbuf);
		glBufferData(GL_ARRAY_BUFFER, 32, texvcoords1, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, textbuf);
		glBufferData(GL_ARRAY_BUFFER, 32, textcoords, GL_STATIC_DRAW);
		glBindVertexArray(texvao);
		glBindBuffer(GL_ARRAY_BUFFER, texvbuf);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
		glBindBuffer(GL_ARRAY_BUFFER, textbuf);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
		GLuint ftex;
		glGenTextures(1, &ftex);
		glBindTexture(GL_TEXTURE_2D, ftex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		std::vector<float> textws;
		float pixelw = 2.0f / w2;
		float pixelh = 2.0f / h2;
		float th;
		int pxprogress = 0;
		FT_Set_Pixel_Sizes(face, (int)(psize * 0.75f), psize); //0
		x = -1.0f;
		y = 1.0f;
		FT_GlyphSlot g = face->glyph;
		for(p = t; *p; p++) {
			int glyph_index = FT_Get_Char_Index(face, *p);
			if(FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT))
				 continue;
			FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, ftex);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glTexImage2D(
				GL_TEXTURE_2D,
				0,
				GL_RED,
				g->bitmap.width,
				g->bitmap.rows,
				0,
				GL_RED,
				GL_UNSIGNED_BYTE,
				g->bitmap.buffer
				);

			GLuint glyphfrbuf;
			glGenFramebuffers(1, &glyphfrbuf);
			glBindFramebuffer(GL_FRAMEBUFFER, glyphfrbuf);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ftex, 0);
			glBlitNamedFramebuffer(glyphfrbuf, texfrbuf, 0, 0, g->bitmap.width, g->bitmap.rows, pxprogress, g->bitmap_top + 12, pxprogress + g->bitmap.width, g->bitmap_top - g->bitmap.rows + 12, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	
			pxprogress += g->advance.x / 64.0f;
	
			if (smflag == 1) {
				glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->smglobfbo_pr);
				mainprogram->drawbuffer = mainprogram->smglobfbo_pr;
			}
			else if (smflag == 2) {
				glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->smglobfbo_tm);
				mainprogram->drawbuffer = mainprogram->smglobfbo_tm;
			}
			else {
				glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
				mainprogram->drawbuffer = mainprogram->globfbo;
			}
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
	
			x += (g->advance.x/64.0f) * pixelw;
			textw += (g->advance.x/64.0f) * pixelw;
			textws.push_back((g->advance.x / 64.0f) * pixelw * ((smflag == 0) + 1) * 0.5f / 1.1f); //1.1 *
			texth = 64.0f * pixelh;
			th = std::max(th, (g->metrics.height / 64.0f) * pixelh);
		}

		//cropping texture
		int w = textw / pixelw; //2.2 *
		GLuint endtex;
		glGenTextures(1, &endtex);
		glBindTexture(GL_TEXTURE_2D, endtex);
		glTexStorage2D(
			GL_TEXTURE_2D,
			1,
			GL_R8,
			w,
			psize * 3
			);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		GLuint endfrbuf;
		glGenFramebuffers(1, &endfrbuf);
		glBindFramebuffer(GL_FRAMEBUFFER, endfrbuf);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, endtex, 0);
		glBlitNamedFramebuffer(texfrbuf, endfrbuf, 0, 0, w, 64 , 0, 0, w, 64, GL_COLOR_BUFFER_BIT, GL_NEAREST);

		if (pos == 0) {
			gs = new GUIString;
			gs->text = text;
			if (smflag == 0) mainprogram->guitextmap[text] = gs;
			else if (smflag == 1) mainprogram->prguitextmap[text] = gs;
			else if (smflag == 2) mainprogram->tmguitextmap[text] = gs;
		}
		gs->texturevec.push_back(endtex);
		gs->textwvec.push_back(textw);
		gs->texthvec.push_back(psize * 3);
		gs->textwvecvec.push_back(textws);
		gs->sxvec.push_back(sx);
		
		glDeleteFramebuffers(1, &texfrbuf);
		glDeleteFramebuffers(1, &endfrbuf);
		glDeleteTextures(1, &ftex);
		glDeleteTextures(1, &texture);
		
		
		//display
		textws = render_text(text, textc, bux, buy, sx, sy, smflag, display, vertical);
		return textws;
  	}

	else if (display) {
		// display string texture after first time preparation - fast!
		int w2 = 0;
		int h2 = 0;
		if (smflag == 0) {
			w2 = glob->w;
			h2 = glob->h;
		}
		else {
			w2 = smw;
			h2 = smh;
		}
		float pixelw = 2.0f / w2;
		float pixelh = 2.0f / h2;
		float textw2 = textw;
		float texth2 = texth * pixelh;

		float wi = 0.0f;
		float he = 0.0f;
		if (vertical) {
			he = texth2;
			wi = textw2;	
		}
		else {
			wi = textw2;
			he = -texth2;
		}
		wi /= (smflag > 0) + 1;
		he /= (smflag > 0) + 1;
		y -= he;

		float black[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		if (textw != 0) draw_box(nullptr, black, x + 0.001f, y - 0.00185f, wi, he, texture, true, vertical);  //draw text shadow
		if (textw != 0) draw_box(nullptr, textc, x, y, wi, he, texture, true, vertical);	//draw text

		mainprogram->texth = texth2;
	}

	
	return textwsplay;
}

float textwvec_total(std::vector<float> textwvec) {
	return std::accumulate(textwvec.begin(), textwvec.end(), 0.0f);
}




void LockBuffer(GLsync& syncObj)
{
	if (syncObj)
		glDeleteSync(syncObj);

	syncObj = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

void WaitBuffer(GLsync& syncObj)
{
	if (syncObj)
	{
		while (1)
		{
			GLenum waitReturn = glClientWaitSync(syncObj, GL_SYNC_FLUSH_COMMANDS_BIT, 1);
			if (waitReturn == GL_ALREADY_SIGNALED || waitReturn == GL_CONDITION_SATISFIED)
				return;
		}
	}
}



bool Layer::calc_texture(bool comp, bool alive) {
	// calculates new frame numbers for a video layer
		//measure time since last loop
	std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed;
	if (this->timeinit) elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - this->prevtime);
	else {
		this->timeinit = true;
		this->prevtime = now;
		return true;
	}
	long long microcount = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
	this->prevtime = now;
	float thismilli = (float)microcount / 1000.0f;

	if (mainprogram->tooltipbox and this->deck == 0 and this->pos == 0) {
		mainprogram->tooltipmilli += thismilli;
	}
	if (mainprogram->dragbinel and !mainmix->moving and mainprogram->onscenebutton and this->deck == 0 and this->pos == 0) {
		mainprogram->onscenemilli += thismilli;
	}

	if (this->type != ELEM_LIVE) {
		if (!this->vidopen) {
			this->frame = this->frame + this->scratch;
			this->scratch = 0;
			// calculate new frame numbers
			float fac;
			if (alive) fac = mainmix->deckspeed[comp][this->deck]->value;
			else fac = this->olddeckspeed;
			if (this->clonesetnr != -1) {
				std::unordered_set<Layer*>::iterator it;
				for (it = mainmix->clonesets[this->clonesetnr]->begin(); it != mainmix->clonesets[this->clonesetnr]->end(); it++) {
					Layer* l = *it;
					if (l->deck == !this->deck) {
						if (alive) fac *= mainmix->deckspeed[comp][!this->deck]->value;
						else fac *= this->olddeckspeed;
						break;
					}
				}
			}
			if (this->type == ELEM_IMAGE) {
				ilBindImage(this->boundimage);
				this->millif = ilGetInteger(IL_IMAGE_DURATION);
			}
			if ((this->speed->value > 0 and (this->playbut->value or this->bouncebut->value == 1)) or (this->speed->value < 0 and (this->revbut->value or this->bouncebut->value == 2))) {
				this->frame += !this->scratchtouch * this->speed->value * fac * fac * this->speed->value * thismilli / this->millif;
			}
			else if ((this->speed->value > 0 and (this->revbut->value or this->bouncebut->value == 2)) or (this->speed->value < 0 and (this->playbut->value or this->bouncebut->value == 1))) {
				this->frame -= !this->scratchtouch * this->speed->value * fac * fac * this->speed->value * thismilli / this->millif;
			}
			if ((int)(this->frame) != this->prevframe and this->type == ELEM_IMAGE and this->numf > 0) {
				// set animated gif to update now
				this->decresult->newdata = true;
			}

			// on end of video (or beginning if reverse play) switch to next clip in queue
			if (this->oldalive or !alive) {
				if (this->frame >= (this->endframe) and this->startframe != this->endframe) {
					if (this->scritching != 4) {
						if (this->bouncebut->value == 0) {
							this->frame = this->startframe;
							this->clip_display_next(0, alive);
						}
						else {
							this->frame = this->endframe - (this->frame - this->endframe);
							this->bouncebut->value = 2;
						}
					}
				}
				else if (this->frame < this->startframe and this->startframe != this->endframe) {
					if (this->scritching != 4) {
						if (this->bouncebut->value == 0) {
							this->frame = this->endframe;
							this->clip_display_next(1, alive);
						}
						else {
							this->frame = this->startframe + (this->startframe - this->frame);
							this->bouncebut->value = 1;
						}
					}
				}
				else if (this->scritching == 4) this->scritching = 0;
			}
			else {
				if (this->type == ELEM_FILE) {
					this->open_video(this->frame, this->filename, 0);
				}
				else if (this->type == ELEM_LAYER) {
					mainmix->open_layerfile(this->layerfilepath, this, 1, 0);
				}
				else if (this->type == ELEM_IMAGE) {
					this->open_image(this->filename);
				}
				else if (this->type == ELEM_LIVE) {
					this->set_live_base(this->filename);
				}
				this->oldalive = true;
			}
		}
	}

	// advance ripple effects
	for (int m = 0; m < 2; m++) {
		for (int i = 0; i < this->effects[m].size(); i++) {
			if (this->effects[m][i]->type == RIPPLE) {
				RippleEffect* effect = (RippleEffect*)(this->effects[m][i]);
				effect->set_ripplecount(effect->get_ripplecount() + (effect->get_speed() * (thismilli / 50.0f)));
				if (effect->get_ripplecount() > 100) effect->set_ripplecount(0);
			}
		}
	}

	return true;
}

void Layer::load_frame() {
	// checks if new frame has been decompressed and loads it into layer
	Layer* srclay = this;
	if (this->liveinput or this->type == ELEM_IMAGE);
	else if (this->startframe != this->endframe or this->type == ELEM_LIVE) {
		if (mainmix->firstlayers.count(this->clonesetnr) == 0) {
			this->ready = true;
			this->startdecode.notify_one();
			if (this->clonesetnr != -1) {
				mainmix->firstlayers[this->clonesetnr] = this;
			}
		}
		else {
			srclay = mainmix->firstlayers[this->clonesetnr];
			this->texture = srclay->texture;
			return;
		}
	}
	else return;

	// initialize layer?
	if (!this->initialized) {
		if (!this->liveinput) {
			if ((!this->isclone)) {
				if (this->video_dec_ctx) {
					if (this->vidformat == 188 or this->vidformat == 187) {
						if (srclay->decresult->compression) {
							float w = srclay->video_dec_ctx->width;
							float h = srclay->video_dec_ctx->height;
							this->initialize(w, h, srclay->decresult->compression);
						}
					}
					else {
						float w = this->video_dec_ctx->width;
						float h = this->video_dec_ctx->height;
						this->initialize(w, h);
					}
				}
			}
		}
		else {
			if (this->liveinput->video_dec_ctx) {
				float w = this->liveinput->video_dec_ctx->width;
				float h = this->liveinput->video_dec_ctx->height;
				this->initialize(w, h);
			}
		}
	}

	// set frame data to texture
	if (this->liveinput) {  // mimiclayers trick/// :(
		this->texture = this->liveinput->texture;
	}

	if (!srclay->decresult->newdata) {
		return;
	}	
	if (srclay->liveinput) srclay->decresult->newdata = false;


	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, this->texture);

	if (srclay->imageloaded >= 4 and srclay->type == ELEM_IMAGE and srclay->numf == 0) return;
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, srclay->pbo[srclay->pbodi]);
	if (!this->liveinput) {
		if ((!this->isclone)) {  // decresult contains new frame width, height, number of bitmaps and data
			if (!srclay->decresult->width) {
				return;
			}
			if ((this->vidformat == 188 or this->vidformat == 187) and srclay->video_dec_ctx) {
				if (!srclay->databufready) {
					return;
				}
				// HAP video layers
				if (srclay->decresult->compression == 187 or srclay->decresult->compression == 171) {
					if (srclay->decresult->data) glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, srclay->decresult->width, srclay->decresult->height, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, srclay->decresult->size, 0);
				}
				else if (srclay->decresult->compression == 190) {
					if (srclay->decresult->data) glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, srclay->decresult->width, srclay->decresult->height, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, srclay->decresult->size, 0);
				}
			}
			else {
				if (srclay->type == ELEM_IMAGE) {
					ilBindImage(srclay->boundimage);
					ilActiveImage((int)srclay->frame);
					srclay->decresult->data = (char*)ilGetData();
					int imageformat = ilGetInteger(IL_IMAGE_FORMAT);
					int w = ilGetInteger(IL_IMAGE_WIDTH);
					int h = ilGetInteger(IL_IMAGE_HEIGHT);
					GLuint mode = GL_BGR;
					if (imageformat == IL_RGBA) mode = GL_RGBA;
					if (imageformat == IL_BGRA) mode = GL_BGRA;
					if (imageformat == IL_RGB) mode = GL_RGB;
					if (imageformat == IL_BGR) mode = GL_BGR;
					glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, mode, GL_UNSIGNED_BYTE, 0);
				}
				else if (this->video_dec_ctx) {
					glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, srclay->decresult->width, srclay->decresult->height, GL_BGRA, GL_UNSIGNED_BYTE, 0);
				}
			}
		}
	}

	srclay->decresult->newdata = false;

	if (!this->isclone and !this->liveinput) {
		// start transferring to pbou
		// bind PBO to update pixel values
		//glBindBuffer(GL_PIXEL_UNPACK_BUFFER, srclay->pbo[srclay->pboui]);
		if (srclay->mapptr[srclay->pbodi]) {
			if (mainprogram->encthreads == 0) WaitBuffer(srclay->syncobj[srclay->pbodi]);
			// update data directly on the mapped buffer
			memcpy(srclay->mapptr[srclay->pbodi], srclay->decresult->data, srclay->decresult->size);
			if (mainprogram->encthreads == 0) LockBuffer(srclay->syncobj[srclay->pbodi]);
		}
	}
	else this->initialized = true;  //reminder: trick, is set false somewhere

	// round robin triple pbos: currently deactivated
	srclay->pbodi++;
	srclay->pboui++;
	if (srclay->pbodi == 3) srclay->pbodi = 0;
	if (srclay->pboui == 3) srclay->pboui = 0;

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);


	this->newtexdata = true;
}

void set_queueing(bool onoff) {
	if (onoff) {
		mainprogram->queueing = true;
	}
	else {
		for (int i = 0; i < 2; i++) {
			std::vector<Layer*> &lvec = choose_layers(i);
			for (int j = 0; j < lvec.size(); j++) {
				lvec[j]->queueing = false;
				mainprogram->queueing = false;
			}
		}
	}
}
	
	
void do_blur(bool stage, GLuint prevfbotex, int iter) {
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, mainprogram->fbotex[0 + stage * 2]);
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, mainprogram->fbotex[1 + stage * 2]);
	if (iter == 0) return;
	GLboolean horizontal = true, first_iteration = true;
	GLuint *tex;
	GLint fboSampler = glGetUniformLocation(mainprogram->ShaderProgram, "fboSampler");
	glUniform1i(fboSampler, 0);
	glActiveTexture(GL_TEXTURE0);
	if (stage) glViewport(0, 0, mainprogram->ow, mainprogram->oh);
	else glViewport(0, 0, mainprogram->ow3, mainprogram->oh3);
	for (GLuint i = 0; i < iter; i++) {
		glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->frbuf[horizontal + stage * 2]);
		mainprogram->drawbuffer = mainprogram->frbuf[horizontal + stage * 2];
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		if (first_iteration) tex = &prevfbotex;
		else tex = &mainprogram->fbotex[!horizontal + stage * 2];
		glBindTexture(GL_TEXTURE_2D, *tex);
		glUniform1i(glGetUniformLocation(mainprogram->ShaderProgram, "horizontal"), horizontal);
		glBindVertexArray(mainprogram->fbovao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		if (i % 2) glActiveTexture(GL_TEXTURE5);
		else glActiveTexture(GL_TEXTURE6);
		glUniform1i(fboSampler, horizontal + 5);
		horizontal = !horizontal;
		if (first_iteration)
			first_iteration = false;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
	mainprogram->drawbuffer = mainprogram->globfbo;
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glViewport(0, 0, glob->w, glob->h);
}

void midi_set() {
	if (mainprogram->midishelfelem) return;

	if (mainmix->midibutton) {
		Button *but = mainmix->midibutton;
		but->value++;
		if (but->value > but->toggle) but->value = 0;
		if (but->toggle == 0) but->value = 1;
		if (but == mainprogram->wormhole1 or but == mainprogram->wormhole2) mainprogram->binsscreen = but->value;
		
		for (int i = 0; i < loopstation->elems.size(); i++) {
			if (loopstation->elems[i]->recbut->value) {
				loopstation->elems[i]->add_button(mainmix->midibutton);
			}
		}

		mainmix->midibutton = nullptr;
	}

	if (mainmix->midishelfbutton) {
		bool shelf = 0;
		int pos = std::find(mainprogram->shelves[0]->buttons.begin(), mainprogram->shelves[0]->buttons.end(), mainmix->midishelfbutton) - mainprogram->shelves[0]->buttons.begin();
		if (pos == 16) {
			pos = std::find(mainprogram->shelves[1]->buttons.begin(), mainprogram->shelves[1]->buttons.end(), mainmix->midishelfbutton) - mainprogram->shelves[1]->buttons.begin();
			shelf = 1;
		}
		// the next elem is used in code somewhere at end of the_loop
		mainprogram->midishelfelem = mainprogram->shelves[shelf]->elements[pos];

		for (int i = 0; i < loopstation->elems.size(); i++) {
			if (loopstation->elems[i]->recbut->value) {
				loopstation->elems[i]->add_button(mainmix->midishelfbutton);
			}
		}

		mainmix->midishelfbutton = nullptr;
	}

	if (mainmix->midiparam) {
		Param *par = mainmix->midiparam;
		par->value = par->range[0] + mainmix->midi2 * ((par->range[1] - par->range[0]) / 127.0);
		if (par->shadervar == "ripple") {
			((RippleEffect*)par->effect)->speed = par->value;
		}
		else if (par->name == "Opacity") {
			par->value = (float)mainmix->midi2 / 127.0f;
		}
		else if (par->name == "Volume") {
			par->value = (float)mainmix->midi2 / 127.0f;
		}
		else if (mainmix->midiisspeed) {
			if (mainmix->midi2 >= 64.0f) {
				par->value = 1.0f + (2.33f / 64.0f) * (mainmix->midi2 - 64.0f);
			}
			else {
				par->value = 0.0f + (1.0f / 64.0f) * mainmix->midi2;
			}
		}
		else {
			GLint var = glGetUniformLocation(mainprogram->ShaderProgram, par->shadervar.c_str());
			glUniform1f(var, par->value);
		}
			
		for (int i = 0; i < loopstation->elems.size(); i++) {
			if (loopstation->elems[i]->recbut->value) {
				loopstation->elems[i]->add_param(mainmix->midiparam);
			}
		}
		
		mainmix->midiparam = nullptr;
	}
}

void onestepfrom(bool stage, Node *node, Node *prevnode, GLuint prevfbotex, GLuint prevfbo) {
	GLint Sampler0 = glGetUniformLocation(mainprogram->ShaderProgram, "Sampler0");
	glUniform1i(Sampler0, 0);
	GLint interm = glGetUniformLocation(mainprogram->ShaderProgram, "interm");
	GLint blurswitch = glGetUniformLocation(mainprogram->ShaderProgram, "blurswitch");
	GLint edgethickmode = glGetUniformLocation(mainprogram->ShaderProgram, "edgethickmode");
	GLint fxid;
		
	float div = 1.0f;
	if (mainprogram->ow > mainprogram->oh) {
		if (stage == 0) div = mainprogram->ow3 / mainprogram->ow;
	}
	else {
		if (stage == 0) div = mainprogram->oh3 / mainprogram->oh;
	}
	
	GLuint fbowidth = glGetUniformLocation(mainprogram->ShaderProgram, "fbowidth");
	glUniform1i(fbowidth, mainprogram->ow);
	GLuint fboheight = glGetUniformLocation(mainprogram->ShaderProgram, "fboheight");
	glUniform1i(fboheight, mainprogram->oh);
	GLfloat fcdiv = glGetUniformLocation(mainprogram->ShaderProgram, "fcdiv");
	glUniform1f(fcdiv, div);

	node->walked = true;
	
	midi_set();
	
	if (node->type == EFFECT) {
		Effect *effect = ((EffectNode*)node)->effect;

		if (effect->layer->initialized) {
			GLint drywet = glGetUniformLocation(mainprogram->ShaderProgram, "drywet");
			glUniform1f(drywet, effect->drywet->value);
			if (effect->onoffbutton->value) {
				for (int i = 0; i < effect->params.size(); i++) {
					Param *par = effect->params[i];
					float val;
					val = par->value;
					if (effect->type == RIPPLE) {
						((RippleEffect*)par->effect)->speed = val;
					}
					else if (par->shadervar == "edthickness") {
						((EdgeDetectEffect*)par->effect)->thickness = val;
					}
					else if (effect->type == BLUR) {
						((BlurEffect*)par->effect)->times = val;
					}
					else {
						GLint var = glGetUniformLocation(mainprogram->ShaderProgram, par->shadervar.c_str());
						glUniform1f(var, val);
					}
				}
				switch (effect->type) {
					case BLUR: {
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, BLUR);
						GLint interm = glGetUniformLocation(mainprogram->ShaderProgram, "interm");
						glUniform1i(interm, 1);
						do_blur(stage, prevfbotex, ((BlurEffect*)effect)->times);
						glActiveTexture(GL_TEXTURE0);
						prevfbotex = mainprogram->fbotex[1 + stage * 2];
						break;
					 }
		
					case BOXBLUR: {
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, BOXBLUR);
						GLint interm = glGetUniformLocation(mainprogram->ShaderProgram, "interm");
						glUniform1i(interm, 1);
						do_blur(stage, prevfbotex, 2);
						glActiveTexture(GL_TEXTURE0);
						prevfbotex = mainprogram->fbotex[1 + stage * 2];
						break;
					}

					case RADIALBLUR: {
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, RADIALBLUR);
						break;
					 }
		
					case GLOW: {
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, GLOW);
						GLint interm = glGetUniformLocation(mainprogram->ShaderProgram, "interm");
						glUniform1i(interm, 1);
						do_blur(stage, prevfbotex, 2);
						glActiveTexture(GL_TEXTURE0);
						prevfbotex = mainprogram->fbotex[1 + stage * 2];
						break;
					 }
		
					case CONTRAST: {
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, CONTRAST);
						break;
					 }
		
					case BRIGHTNESS: {
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, BRIGHTNESS);
						break;
					 }
		
					case SCALE: {
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, SCALE);
						break;
					 }
		
					case SWIRL: {
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, SWIRL);
						break;
					 }
		
					case CHROMAROTATE: {
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, CHROMAROTATE);
						break;
					 }
		
					case DOT: {
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, DOT);
						break;
					 }
		
					case SATURATION: {
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, SATURATION);
						break;
					 }
		
					case OLDFILM: {
						GLfloat RandomValue = glGetUniformLocation(mainprogram->ShaderProgram, "RandomValue");
						glUniform1f(RandomValue, (float)(rand() % 100) / 100.0);
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, OLDFILM);
						break;
					 }
		
					case RIPPLE: {
						float riptime = glGetUniformLocation(mainprogram->ShaderProgram, "riptime");
						glUniform1f(riptime, effect->get_ripplecount());
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, RIPPLE);
						break;
					 }
		
					case FISHEYE: {
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, FISHEYE);
						break;
					 }
		
					case TRESHOLD: {
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, TRESHOLD);
						break;
					 }
		
					case STROBE: {
						if (((StrobeEffect*)effect)->get_phase() == 0) ((StrobeEffect*)effect)->set_phase(1);
						else ((StrobeEffect*)effect)->set_phase(0);
						GLint strobephase = glGetUniformLocation(mainprogram->ShaderProgram, "strobephase");
						glUniform1f(strobephase, ((StrobeEffect*)effect)->get_phase());
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, STROBE);
						break;
					}
		
					case POSTERIZE: {
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, POSTERIZE);
						break;
					 }
		
					case PIXELATE: {
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, PIXELATE);
						break;
					 }
		
					case CROSSHATCH: {
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, CROSSHATCH);
						break;
					 }
		
					case INVERT: {
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, INVERT);
						break;
					 }
		
					case ROTATE: {
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, ROTATE);
						break;
					}
		
					case EMBOSS: {
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, EMBOSS);
						break;
					 }
		
					case ASCII: {
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, ASCII);
						break;
					 }
		
					case SOLARIZE: {
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, SOLARIZE);
						break;
					 }
		
					case VARDOT: {
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, VARDOT);
						break;
					 }
		
					case CRT: {
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, CRT);
						break;
					 }
					 
					case EDGEDETECT: {
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, EDGEDETECT);
						GLint fboSampler = glGetUniformLocation(mainprogram->ShaderProgram, "fboSampler");
						GLint interm = glGetUniformLocation(mainprogram->ShaderProgram, "interm");
						glUniform1i(interm, 1);
						glActiveTexture(GL_TEXTURE0);
						glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->frbuf[0 + stage * 2]);
						mainprogram->drawbuffer = mainprogram->frbuf[0 + stage * 2];
						glDrawBuffer(GL_COLOR_ATTACHMENT0);
						if (stage) glViewport(0, 0, mainprogram->ow, mainprogram->oh);
						else glViewport(0, 0, mainprogram->ow3, mainprogram->oh3);
						glBindTexture(GL_TEXTURE_2D, prevfbotex);
						glBindVertexArray(mainprogram->fbovao);
						glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
						glUniform1i(interm, 0);
						
						glUniform1i(edgethickmode, 1);
						glActiveTexture(GL_TEXTURE6);
						glBindTexture(GL_TEXTURE_2D, mainprogram->fbotex[1 + stage * 2]);
						glActiveTexture(GL_TEXTURE5);
						glBindTexture(GL_TEXTURE_2D, mainprogram->fbotex[0 + stage * 2]);
						glUniform1i(fboSampler, 5);
						GLboolean swits = true;
						GLuint *tex;
						for(GLuint i = 0; i < ((EdgeDetectEffect*)effect)->thickness; i++) {
							glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->frbuf[swits + stage * 2]);
							glDrawBuffer(GL_COLOR_ATTACHMENT0);
							mainprogram->drawbuffer = mainprogram->frbuf[swits + stage * 2];
							glBindTexture(GL_TEXTURE_2D, mainprogram->fbotex[!swits + stage * 2]);
							glBindVertexArray(mainprogram->fbovao);
							glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
							if (i % 2) glActiveTexture(GL_TEXTURE5);
							else glActiveTexture(GL_TEXTURE6);
							glUniform1i(fboSampler, swits + 5);
							swits = !swits;
						}
						glUniform1i(fboSampler, 0);
						glActiveTexture(GL_TEXTURE0);
						glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
						mainprogram->drawbuffer = mainprogram->globfbo;
						glDrawBuffer(GL_COLOR_ATTACHMENT0);
						glViewport(0, 0, glob->w, glob->h);
						if (((EdgeDetectEffect*)effect)->thickness % 2) prevfbotex = mainprogram->fbotex[1 + stage * 2];
						else prevfbotex = mainprogram->fbotex[0 + stage * 2];
						break;
					}
					 
					case KALEIDOSCOPE: {
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, KALEIDOSCOPE);
						break;
					 }
					 
					case HTONE: {
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, HTONE);
						break;
					 }
					 
					case CARTOON: {
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, CARTOON);
						break;
					 }
					 
					case CUTOFF: {
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, CUTOFF);
						break;
					 }
					 
					case GLITCH: {
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, GLITCH);
						break;
					 }
					 
					case COLORIZE: {
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, COLORIZE);
						break;
					 }
					 
					case NOISE: {
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, NOISE);
						break;
					 }
					 
					case GAMMA: {
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, GAMMA);
						break;
					 }
					 
					case THERMAL: {
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, THERMAL);
						break;
					 }
					 
					case BOKEH: {
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, BOKEH);
						break;
					 }
					 
					case SHARPEN: {
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, SHARPEN);
						break;
					 }
					 
					case DITHER: {
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, DITHER);
						break;
					 }
					 
					case FLIP: {
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, FLIP);
						break;
					 }

					case MIRROR: {
						fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
						glUniform1i(fxid, MIRROR);
						break;
					}
				}
			}
			
			glActiveTexture(GL_TEXTURE0);
			if (effect->fbo == -1) {
				glGenTextures(1, &(effect->fbotex));
				glBindTexture(GL_TEXTURE_2D, effect->fbotex);
				if ((stage == 0)) {

					glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, mainprogram->ow3, mainprogram->oh3);
				}
				else {
					glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, mainprogram->ow, mainprogram->oh);
				}
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
				glGenFramebuffers(1, &(effect->fbo));
				glBindFramebuffer(GL_FRAMEBUFFER, effect->fbo);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, effect->fbotex, 0);
			}
			GLuint down = glGetUniformLocation(mainprogram->ShaderProgram, "down");
			if (effect->onoffbutton->value) glUniform1i(interm, 1);
			else glUniform1i(down, 1);
			GLfloat opacity = glGetUniformLocation(mainprogram->ShaderProgram, "opacity");
			if (effect->node == (EffectNode*)(effect->layer->lasteffnode[0])) {
				glUniform1f(opacity, effect->layer->opacity->value);
			}
			else glUniform1f(opacity, 1.0f);
			
			
	// #define BUFFER_OFFSET(i) ((char *)nullptr + (i))
			// GLuint ioBuf;
			// glGenBuffers(1, &ioBuf); 
			// glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, ioBuf);
			// glBufferData(GL_PIXEL_PACK_BUFFER_ARB, mainprogram->ow * mainprogram->oh, nullptr, GL_STREAM_READ);
			// glBindFramebuffer(GL_FRAMEBUFFER, ((VideoNode*)(node->in))->layer->fbo);
			// glReadBuffer(GL_COLOR_ATTACHMENT0);
			// glReadPixels(0, 0, mainprogram->ow, mainprogram->oh, GL_BGRA, GL_UNSIGNED_BYTE, BUFFER_OFFSET(0));
			// void* mem = glMapBuffer(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY);   
			// assert(mem);
			// CopyMemory((void*)effect->pbuf, mem, 2073600);
			// //memcpy((void*)effect->pbuf, "SECRET", 7);
			// glUnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB);
			// glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0);		
			// //while((const char*)effect->pbuf != "TERCES") {}
			// //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, mainprogram->ow, mainprogram->oh, 0, GL_BGRA, GL_UNSIGNED_BYTE, effect->pbuf);
			
	 
			glBindFramebuffer(GL_FRAMEBUFFER, effect->fbo);
			mainprogram->drawbuffer = effect->fbo;
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			if (stage) glViewport(0, 0, mainprogram->ow, mainprogram->oh);
			else glViewport(0, 0, mainprogram->ow3, mainprogram->oh3);
			glClearColor( 0.f, 0.f, 0.f, 0.f );
			glClear(GL_COLOR_BUFFER_BIT);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, prevfbotex);
			glBindVertexArray(mainprogram->fbovao);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			glUniform1i(interm, 0);
			glUniform1i(down, 0);
			glUniform1i(edgethickmode, 0);
			prevfbotex = effect->fbotex;
			prevfbo = effect->fbo;
	
			Layer *lay = effect->layer;
			if (effect->node == lay->lasteffnode[0]) {
				GLuint fbocopy;
				glDisable(GL_BLEND);
				int sw, sh;
				glBindTexture(GL_TEXTURE_2D, effect->fbotex);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &sw);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &sh);
				fbocopy = copy_tex(effect->fbotex, sw, sh);
				glViewport(0, 0, sw, sh);
				glBindFramebuffer(GL_FRAMEBUFFER, effect->fbo);
				mainprogram->drawbuffer = effect->fbo;
				glDrawBuffer(GL_COLOR_ATTACHMENT0);
				glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
				glClear(GL_COLOR_BUFFER_BIT);
				float black[4] = {0.0f, 0.0f, 0.0f, 0.0f};
				draw_box(nullptr, black, -1.0f, 1.0f, 2.0f, -2.0f, lay->shiftx->value, lay->shifty->value, lay->scale->value, lay->opacity->value, 0, fbocopy, 0, 0, false);
				glEnable(GL_BLEND);
				glDeleteTextures(1, &fbocopy);
			}
			
			glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
			mainprogram->drawbuffer = mainprogram->globfbo;
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			glViewport(0, 0, glob->w, glob->h);
		}
	}
	else if (node->type == VIDEO) {
		Layer *lay = ((VideoNode*)node)->layer;
		if (!lay->newtexdata and mainmix->bulrs[lay->deck].size()) {
			if (lay->node == lay->lasteffnode[0]) {
				lay->drawfbo2 = true;
			}
			else {
				lay->drawfbo2 = false;
			}
			return;
		}
		else {
			for (int m = 0; m < 2; m++) {
				if (mainmix->bulrs[m].size()) {
					bool notyet = false;
					std::vector<Layer*>& lvec = choose_layers(m);
					for (int i = 0; i < lvec.size(); i++) {
						if (lvec[i]->filename == "" or lvec[i]->type == ELEM_LIVE) continue;
						if (lvec[i]->newtexdata) {
							lvec[i]->startup = true;
						}
						if (!lvec[i]->startup) notyet = true;
					}
					if (!notyet) {
						mainmix->bulrscopy[m] = mainmix->bulrs[m];
						mainmix->bulrscopy[m].erase(mainmix->bulrscopy[m].begin());
						mainmix->delete_layers(mainmix->bulrscopy[m], mainmix->bualive);  //reminder: change to threaded variant?
						mainmix->bulrs[m].clear();
						// delete the butexes when idle, reminder: <-
						for (int i = 0; i < mainmix->butexes[m].size(); i++) {
							glDeleteTextures(1, &mainmix->butexes[m][i]);
						}
					}
				}
			}
			if (lay->blendnode) {
				if (lay->blendnode->blendtype == 19) {
					GLfloat colortol = glGetUniformLocation(mainprogram->ShaderProgram, "colortol");
					glUniform1f(colortol, lay->chtol->value);
					GLfloat chdir = glGetUniformLocation(mainprogram->ShaderProgram, "chdir");
					glUniform1f(chdir, lay->chdir->value);
					GLfloat chinv = glGetUniformLocation(mainprogram->ShaderProgram, "chinv");
					glUniform1f(chinv, lay->chinv->value);
				}
			}
			float xs = 0.0f;
			float ys = 0.0f;
			float scix = 0.0f;
			float sciy = 0.0f;
			float frachd = 1920.0f / 1080.0f;
			float fraco = mainprogram->ow / mainprogram->oh;
			float frac;
			if (lay->type == ELEM_IMAGE) {
				ilBindImage(lay->boundimage);
				ilActiveImage((int)lay->frame);
				frac = (float)ilGetInteger(IL_IMAGE_WIDTH) / (float)ilGetInteger(IL_IMAGE_HEIGHT);
			}
			else frac = (float)lay->decresult->width / (float)lay->decresult->height;
			if (fraco > frachd) {
				ys = 1.0f - frachd / fraco;
				sciy = ys;
			}
			else {
				xs = 1.0f - fraco / frachd;
				scix = xs;
			}
			if (lay->aspectratio == RATIO_ORIGINAL_INSIDE) {
				if (frac > fraco) {
					ys = 1.0f - (((1.0f - xs) * frachd) / frac);
				}
				else {
					xs = 1.0f - (((1.0f - ys) / frachd) * frac);
				}
			}
			else if (lay->aspectratio == RATIO_ORIGINAL_OUTSIDE) {
				if (frac < fraco) {
					ys = 1.0f - (((1.0f - xs) * frachd) / frac);
				}
				else {
					xs = 1.0f - (((1.0f - ys) / frachd) * frac);
				}
			}

			glActiveTexture(GL_TEXTURE0);
			int sw, sh;
			glBindTexture(GL_TEXTURE_2D, lay->fbotex);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &sw);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &sh);
			float scw = sw;
			float sch = sh;
			int sxs = xs * scw / 2.0f;
			int sys = ys * sch / 2.0f;
			scix = scix * scw / 2.0f;
			sciy = sciy * sch / 2.0f;
			glBindFramebuffer(GL_FRAMEBUFFER, lay->fbointm);
			mainprogram->drawbuffer = lay->fbointm;
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			glViewport(sxs, sys, scw - sxs * 2.0f, sch - sys * 2.0f);
			float black[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			draw_box(nullptr, black, -1.0f, 1.0f, 2.0f, -2.0f, lay->texture);

			if (lay->node == lay->lasteffnode[0]) {
				lay->drawfbo2 = true;
				glDisable(GL_BLEND);
				glBindTexture(GL_TEXTURE_2D, lay->fbotexintm);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &sw);
				glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &sh);
				glViewport(0, 0, sw, sh);
				glBindFramebuffer(GL_FRAMEBUFFER, lay->fbo);
				mainprogram->drawbuffer = lay->fbo;
				glDrawBuffer(GL_COLOR_ATTACHMENT0);
				glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
				glClear(GL_COLOR_BUFFER_BIT);
				float black[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

				
				draw_box(nullptr, black, -1.0f, 1.0f, 2.0f, -2.0f, lay->shiftx->value, lay->shifty->value, lay->scale->value, lay->opacity->value, 0, lay->fbotexintm, 0, 0, false);
				glEnable(GL_BLEND);
				prevfbotex = lay->fbotex;
				prevfbo = lay->fbo;
			}
			else {
				lay->drawfbo2 = false;
				prevfbotex = lay->fbotexintm;
				prevfbo = lay->fbointm;
			}
			glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
			mainprogram->drawbuffer = mainprogram->globfbo;
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			glViewport(0, 0, glob->w, glob->h);
			lay->newtexdata = false;
		}
	}
	else if (node->type == BLEND) {
		BlendNode *bnode = (BlendNode*)node;
		if (bnode->in == prevnode) {
			bnode->intex = prevfbotex;
		}
		if (bnode->in2 == prevnode) {
			bnode->in2tex = prevfbotex;
		}
		if (bnode->in and bnode->in2) {
			if (bnode->intex != -1 and bnode->in2tex != -1) {
				if (bnode->fbo == -1) {
					glGenTextures(1, &(bnode->fbotex));
					glBindTexture(GL_TEXTURE_2D, bnode->fbotex);
					if (stage == 0) {
						glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, mainprogram->ow3, mainprogram->oh3);
					}
					else {	
						glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, mainprogram->ow, mainprogram->oh);
					}
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		
					glGenFramebuffers(1, &(bnode->fbo));
					glBindFramebuffer(GL_FRAMEBUFFER, bnode->fbo);
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bnode->fbotex, 0);
				}
				
				glBindFramebuffer(GL_FRAMEBUFFER, bnode->fbo);
				mainprogram->drawbuffer = bnode->fbo;
				glDrawBuffer(GL_COLOR_ATTACHMENT0);
				if (stage) {
					glViewport(0, 0, mainprogram->ow, mainprogram->oh);
				}
				else {
					glViewport(0, 0, mainprogram->ow3, mainprogram->oh3);
				}
				
				GLfloat mixfac = glGetUniformLocation(mainprogram->ShaderProgram, "mixfac");
				glUniform1f(mixfac, bnode->mixfac->value);
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, bnode->intex);

				GLfloat cf = glGetUniformLocation(mainprogram->ShaderProgram, "cf");
				if (stage) glUniform1f(cf, mainmix->crossfadecomp->value);
				
				GLint mixmode = glGetUniformLocation(mainprogram->ShaderProgram, "mixmode");
				glUniform1i(mixmode, bnode->blendtype);
				if (bnode->blendtype == 19) {
					GLfloat chred = glGetUniformLocation(mainprogram->ShaderProgram, "chred");
					glUniform1f(chred, bnode->chred);
					GLfloat chgreen = glGetUniformLocation(mainprogram->ShaderProgram, "chgreen");
					glUniform1f(chgreen, bnode->chgreen);
					GLfloat chblue = glGetUniformLocation(mainprogram->ShaderProgram, "chblue");
					glUniform1f(chblue, bnode->chblue);
				}
				GLint wipe = glGetUniformLocation(mainprogram->ShaderProgram, "wipe");
				if (bnode->blendtype == 18) {
					GLint inlayer = glGetUniformLocation(mainprogram->ShaderProgram, "inlayer");
					glUniform1f(inlayer, 1);
					if (bnode->wipetype > -1) glUniform1i(wipe, true);
					GLint wkind = glGetUniformLocation(mainprogram->ShaderProgram, "wkind");
					glUniform1i(wkind, bnode->wipetype);
					GLint dir = glGetUniformLocation(mainprogram->ShaderProgram, "dir");
					glUniform1i(dir, bnode->wipedir);
					GLfloat xpos = glGetUniformLocation(mainprogram->ShaderProgram, "xpos");
					glUniform1f(xpos, bnode->wipex->value);
					GLfloat ypos = glGetUniformLocation(mainprogram->ShaderProgram, "ypos");
					glUniform1f(ypos, bnode->wipey->value);
				}
				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D, bnode->in2tex);
				glActiveTexture(GL_TEXTURE0);
				glBindVertexArray(mainprogram->fbovao);
				GLfloat opacity = glGetUniformLocation(mainprogram->ShaderProgram, "opacity");
				glUniform1f(opacity, 1.0f);
				glDisable(GL_BLEND);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
				glEnable(GL_BLEND);
				prevfbotex = bnode->fbotex;
				prevfbo = bnode->fbo;
				
				GLint inlayer = glGetUniformLocation(mainprogram->ShaderProgram, "inlayer");
				glUniform1f(inlayer, 0);
				glUniform1i(wipe, 0);
				glUniform1i(mixmode, 0);
				if (mainprogram->prevmodus) {
					glUniform1f(cf, mainmix->crossfade->value);
				}
				else {
					glUniform1f(cf, mainmix->crossfadecomp->value);
				}
				glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
				mainprogram->drawbuffer = mainprogram->globfbo;
				glDrawBuffer(GL_COLOR_ATTACHMENT0);
				glViewport(0, 0, glob->w, glob->h);
			}
			else {
				if (prevnode == bnode->in2) {
					node->walked = true;			//for when first layer is muted
					glBindFramebuffer(GL_FRAMEBUFFER, bnode->fbo);
					mainprogram->drawbuffer = bnode->fbo;
					glDrawBuffer(GL_COLOR_ATTACHMENT0);
					if (stage) glViewport(0, 0, mainprogram->ow, mainprogram->oh);
					else glViewport(0, 0, mainprogram->ow3, mainprogram->oh3);
					glBindTexture(GL_TEXTURE_2D, prevfbotex);
					glBindVertexArray(mainprogram->fbovao);
					glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
					glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
					mainprogram->drawbuffer = mainprogram->globfbo;
					glDrawBuffer(GL_COLOR_ATTACHMENT0);
					glViewport(0, 0, glob->w, glob->h);
				}
				else {
					node->walked = false;
					return;
				}
			}
		}
	}
	else if (node->type == MIX) {
		MixNode *mnode = (MixNode*)node;
		if (mnode->mixfbo == -1) {
			glGenTextures(1, &(mnode->mixtex));
			glBindTexture(GL_TEXTURE_2D, mnode->mixtex);
			if (stage == 0) {
				glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, mainprogram->ow3, mainprogram->oh3);
			}
			else {
				glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, mainprogram->ow, mainprogram->oh);
			}
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glGenFramebuffers(1, &(mnode->mixfbo));
			glBindFramebuffer(GL_FRAMEBUFFER, mnode->mixfbo);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mnode->mixtex, 0);
		}
		glDisable(GL_BLEND);
		GLuint singlelayer = glGetUniformLocation(mainprogram->ShaderProgram, "singlelayer");
		GLuint down = glGetUniformLocation(mainprogram->ShaderProgram, "down");
		if (prevnode->type == VIDEO) glUniform1i(singlelayer, 1);
		else glUniform1i(down, 1);
		glBindFramebuffer(GL_FRAMEBUFFER, mnode->mixfbo);
		mainprogram->drawbuffer = mnode->mixfbo;
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		if (stage) glViewport(0, 0, mainprogram->ow, mainprogram->oh);
		else glViewport(0, 0, mainprogram->ow3, mainprogram->oh3);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, prevfbotex);
		glBindVertexArray(mainprogram->fbovao);
		glDisable(GL_BLEND);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glEnable(GL_BLEND);
		glUniform1i(singlelayer, 0);
		glUniform1i(down, 0);
		prevfbotex = mnode->mixtex;
		prevfbo = mnode->mixfbo;
		glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
		mainprogram->drawbuffer = mainprogram->globfbo;
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glViewport(0, 0, glob->w, glob->h);
	}
	for (int i = 0; i < node->out.size(); i++) {
		if (node->out[i]->calc and !node->out[i]->walked) onestepfrom(stage, node->out[i], node, prevfbotex, prevfbo);
	}
}

void walk_back(Node* node) {
	while (node->in and !node->calc) {
		if (node->type == BLEND) {
			if (((BlendNode*)node)->in2) walk_back(((BlendNode*)node)->in2);
		}
		node->calc = true;
		node = node->in;
		//try { Node* n = node->in; }
		//catch (...) { return; }
	}
}

void walk_forward(Node* node) {
	do {
		node->calc = true;
		node->walked = false;
		if (node->out.size() == 0) break;
		node = node->out[0];
	} while (1);
}

void walk_nodes(bool stage) {
	std::vector<Node*> fromnodes;
	if (stage == 0) {
		for (int i = 0; i < mainprogram->nodesmain->mixnodes.size(); i++) {
			fromnodes.push_back(mainprogram->nodesmain->mixnodes[i]);
		}
	}
	else {
		for (int i = 0; i < mainprogram->nodesmain->mixnodescomp.size(); i++) {
			fromnodes.push_back(mainprogram->nodesmain->mixnodescomp[i]);
		}
	}
	for (int i = 0; i < mainprogram->nodesmain->pages.size(); i++) {
		std::vector<Node*> nvec;
		if (stage == 0) nvec = mainprogram->nodesmain->pages[i]->nodes;
		else nvec = mainprogram->nodesmain->pages[i]->nodescomp;
		for (int j = 0; j < nvec.size(); j++) {
			Node *node = nvec[j];
			node->calc = false;
			node->walked = false;
			if (node->type == BLEND) {
				((BlendNode*)node)->intex = -1;
				((BlendNode*)node)->in2tex = -1;
			}
			if (mainprogram->nodesmain->linked) {
				if (node->monitor != -1) { //reminder: nodes structure
					if (node->monitor / 6 == mainmix->currscene[0][0]) {
						fromnodes.push_back(node);
					}
				}
			}
		}
	}
	for (int i = 0; i < fromnodes.size(); i++) {
		Node *node = fromnodes[i];		
		if (node) walk_back(node);
	}
	
	if (stage == 0) {
		int mutes = 0;
		bool muted = false;
		for (int i = 0; i < mainmix->layersA.size(); i++) {
			Layer* lay = mainmix->layersA[i];
			if (!lay->liveinput and !lay->decresult->width and lay->filename != "") return;
			if (lay->mutebut->value) mutes++;
			walk_forward(lay->node);
			onestepfrom(0, lay->node, nullptr, -1, -1);
		}
		if (mutes == mainmix->layersA.size()) {
			muted = true;
			glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->nodesmain->mixnodes[0]->mixfbo);
			mainprogram->drawbuffer = mainprogram->nodesmain->mixnodes[0]->mixfbo;
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			glClearColor(0, 0, 0, 0);
			glClear(GL_COLOR_BUFFER_BIT);
		}
		mutes = 0;
		for (int i = 0; i < mainmix->layersB.size(); i++) {
			Layer* lay = mainmix->layersB[i];
			if (!lay->liveinput and !lay->decresult->width and lay->filename != "") return;
			if (lay->mutebut->value) mutes++;
			walk_forward(lay->node);
			onestepfrom(0, lay->node, nullptr, -1, -1);
		}
		if (mutes == mainmix->layersB.size()) {
			muted = true;
			glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->nodesmain->mixnodes[1]->mixfbo);
			mainprogram->drawbuffer = mainprogram->nodesmain->mixnodes[1]->mixfbo;
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			glClearColor(0, 0, 0, 0);
			glClear(GL_COLOR_BUFFER_BIT);
		}
		if (muted) {
			mainprogram->bnodeend->intex = -1;
			mainprogram->bnodeend->in2tex = -1;
			onestepfrom(0, mainprogram->bnodeend, mainprogram->nodesmain->mixnodes[0], mainprogram->nodesmain->mixnodes[0]->mixtex, mainprogram->nodesmain->mixnodes[0]->mixfbo);
			onestepfrom(0, mainprogram->bnodeend, mainprogram->nodesmain->mixnodes[1], mainprogram->nodesmain->mixnodes[1]->mixtex, mainprogram->nodesmain->mixnodes[1]->mixfbo);
		}
	}
	else {
		int mutes = 0;
		bool muted = false;
		for (int i = 0; i < mainmix->layersAcomp.size(); i++) {
			Layer* lay = mainmix->layersAcomp[i];
			if (!lay->liveinput and !lay->decresult->width and lay->filename != "") return;
			if (lay->mutebut->value) mutes++;
			walk_forward(lay->node);
			onestepfrom(1, lay->node, nullptr, -1, -1);
		}
		if (mutes == mainmix->layersAcomp.size()) {
			muted = true;
			glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->nodesmain->mixnodescomp[0]->mixfbo);
			mainprogram->drawbuffer = mainprogram->nodesmain->mixnodescomp[0]->mixfbo;
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			glClearColor(0, 0, 0, 0);
			glClear(GL_COLOR_BUFFER_BIT);
		}
		mutes = 0;
		for (int i = 0; i < mainmix->layersBcomp.size(); i++) {
			Layer* lay = mainmix->layersBcomp[i];
			if (!lay->liveinput and !lay->decresult->width and lay->filename != "") return;
			if (lay->mutebut->value) mutes++;
			walk_forward(lay->node);
			onestepfrom(1, lay->node, nullptr, -1, -1);
		}
		if (mutes == mainmix->layersBcomp.size()) {
			muted = true;
			glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->nodesmain->mixnodescomp[1]->mixfbo);
			mainprogram->drawbuffer = mainprogram->nodesmain->mixnodescomp[1]->mixfbo;
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			glClearColor(0, 0, 0, 0);
			glClear(GL_COLOR_BUFFER_BIT);
		}
		if (muted) {
			mainprogram->bnodeendcomp->intex = -1;
			mainprogram->bnodeendcomp->in2tex = -1;
			onestepfrom(1, mainprogram->bnodeendcomp, mainprogram->nodesmain->mixnodescomp[0], mainprogram->nodesmain->mixnodescomp[0]->mixtex, mainprogram->nodesmain->mixnodescomp[0]->mixfbo);
			onestepfrom(1, mainprogram->bnodeendcomp, mainprogram->nodesmain->mixnodescomp[1], mainprogram->nodesmain->mixnodescomp[1]->mixtex, mainprogram->nodesmain->mixnodescomp[1]->mixfbo);
		}
	}
}		
		

bool display_mix() {
	float xs = 0.0f;
	float ys = 0.0f;
	float fraco = mainprogram->ow / mainprogram->oh;
	float frachd = 1920.0f / 1080.0f;
	fraco = frachd;
	if (fraco > frachd) {
		ys = 0.15f * (1.0f - frachd / fraco);
	}
	else {
		xs = 0.15f * (1.0f - fraco / frachd);
	}
	MixNode* node;
	Box *box;
	GLfloat cf = glGetUniformLocation(mainprogram->ShaderProgram, "cf");
	if (mainprogram->prevmodus) {
		glUniform1f(cf, mainmix->crossfade->value);
	}
	else {
		glUniform1f(cf, mainmix->crossfadecomp->value);
	}
	GLint wipe = glGetUniformLocation(mainprogram->ShaderProgram, "wipe");
	GLint mixmode = glGetUniformLocation(mainprogram->ShaderProgram, "mixmode");
	if (mainprogram->prevmodus) {
		if (mainmix->wipe[0] > -1) {
			glUniform1i(mixmode, 18);
			glUniform1i(wipe, 1);
			GLint wkind = glGetUniformLocation(mainprogram->ShaderProgram, "wkind");
			glUniform1i(wkind, mainmix->wipe[0]);
			GLint dir = glGetUniformLocation(mainprogram->ShaderProgram, "dir");
			glUniform1i(dir, mainmix->wipedir[0]);
			GLfloat xpos = glGetUniformLocation(mainprogram->ShaderProgram, "xpos");
			glUniform1f(xpos, mainmix->wipex[0]->value);
			GLfloat ypos = glGetUniformLocation(mainprogram->ShaderProgram, "ypos");
			glUniform1f(ypos, mainmix->wipey[0]->value);
		}
		node = (MixNode*)mainprogram->nodesmain->mixnodes[0];
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, node->mixtex);
		node = (MixNode*)mainprogram->nodesmain->mixnodes[1];
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, node->mixtex);
		glActiveTexture(GL_TEXTURE0);
		node = (MixNode*)mainprogram->nodesmain->mixnodes[2];
		box = mainprogram->mainmonitor;
		draw_box(node->outputbox->lcolor, node->outputbox->acolor, box, -1);
		draw_box(node->outputbox->lcolor, node->outputbox->acolor, box->vtxcoords->x1 + xs * 2.0f, box->vtxcoords->y1 + ys * 2.0f, box->vtxcoords->w - xs * 4.0f, box->vtxcoords->h - ys * 4.0f, node->mixtex);
		mainprogram->mainmonitor->in();
		
		glUniform1i(wipe, 0);
		glUniform1i(mixmode, 0);
		if (mainmix->wipe[1] > -1) {
			GLfloat cf = glGetUniformLocation(mainprogram->ShaderProgram, "cf");
			glUniform1f(cf, mainmix->crossfadecomp->value);
			glUniform1i(mixmode, 18);
			glUniform1i(wipe, 1);
			GLint wkind = glGetUniformLocation(mainprogram->ShaderProgram, "wkind");
			glUniform1i(wkind, mainmix->wipe[1]);
			GLint dir = glGetUniformLocation(mainprogram->ShaderProgram, "dir");
			glUniform1i(dir, mainmix->wipedir[1]);
			GLfloat xpos = glGetUniformLocation(mainprogram->ShaderProgram, "xpos");
			glUniform1f(xpos, mainmix->wipex[1]->value);
			GLfloat ypos = glGetUniformLocation(mainprogram->ShaderProgram, "ypos");
			glUniform1f(ypos, mainmix->wipey[1]->value);
			node = (MixNode*)mainprogram->nodesmain->mixnodescomp[0];
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, node->mixtex);
			node = (MixNode*)mainprogram->nodesmain->mixnodescomp[1];
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, node->mixtex);
			glActiveTexture(GL_TEXTURE0);
		}
		node = (MixNode*)mainprogram->nodesmain->mixnodescomp[2];
		box = mainprogram->outputmonitor;
		draw_box(node->outputbox->lcolor, node->outputbox->acolor, box, -1);
		draw_box(node->outputbox->lcolor, node->outputbox->acolor, box->vtxcoords->x1 + xs, box->vtxcoords->y1 + ys, box->vtxcoords->w - xs * 2.0f, box->vtxcoords->h - ys * 2.0f, node->mixtex);
		mainprogram->outputmonitor->in();
	}
	else {
		glUniform1i(wipe, 0);
		glUniform1i(mixmode, 0);
		if (mainmix->wipe[1] > -1) {
			GLfloat cf = glGetUniformLocation(mainprogram->ShaderProgram, "cf");
			glUniform1f(cf, mainmix->crossfadecomp->value);
			glUniform1i(mixmode, 18);
			glUniform1i(wipe, 1);
			GLint wkind = glGetUniformLocation(mainprogram->ShaderProgram, "wkind");
			glUniform1i(wkind, mainmix->wipe[1]);
			GLint dir = glGetUniformLocation(mainprogram->ShaderProgram, "dir");
			glUniform1i(dir, mainmix->wipedir[1]);
			GLfloat xpos = glGetUniformLocation(mainprogram->ShaderProgram, "xpos");
			glUniform1f(xpos, mainmix->wipex[1]->value);
			GLfloat ypos = glGetUniformLocation(mainprogram->ShaderProgram, "ypos");
			glUniform1f(ypos, mainmix->wipey[1]->value);
			node = (MixNode*)mainprogram->nodesmain->mixnodescomp[0];
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, node->mixtex);
			node = (MixNode*)mainprogram->nodesmain->mixnodescomp[1];
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, node->mixtex);
			glActiveTexture(GL_TEXTURE0);
		}
		node = (MixNode*)mainprogram->nodesmain->mixnodescomp[2];
		box = mainprogram->mainmonitor;
		draw_box(node->outputbox->lcolor, node->outputbox->acolor, box, -1);
		draw_box(node->outputbox->lcolor, node->outputbox->acolor, box->vtxcoords->x1 + xs * 2.0f, box->vtxcoords->y1 + ys * 2.0f, box->vtxcoords->w - xs * 4.0f, box->vtxcoords->h - ys * 4.0f, node->mixtex);
		mainprogram->mainmonitor->in();
	}
	glUniform1i(wipe, 0);
	glUniform1i(mixmode, 0);
	
	if (mainprogram->prevmodus) {
		node = (MixNode*)mainprogram->nodesmain->mixnodes[0];
		box = mainprogram->deckmonitor[0];
		draw_box(node->outputbox->lcolor, node->outputbox->acolor, box, -1);
		draw_box(nullptr, node->outputbox->acolor, box->vtxcoords->x1 + xs, box->vtxcoords->y1 + ys, box->vtxcoords->w - xs * 2.0f, box->vtxcoords->h - ys * 2.0f, node->mixtex);
		mainprogram->deckmonitor[0]->in();
		node = (MixNode*)mainprogram->nodesmain->mixnodes[1];
		box = mainprogram->deckmonitor[1];
		draw_box(node->outputbox->lcolor, node->outputbox->acolor, box, -1);
		draw_box(node->outputbox->lcolor, node->outputbox->acolor, box->vtxcoords->x1 + xs, box->vtxcoords->y1 + ys, box->vtxcoords->w - xs * 2.0f, box->vtxcoords->h - ys * 2.0f, node->mixtex);
		mainprogram->deckmonitor[1]->in();
	}
	else {
		node = (MixNode*)mainprogram->nodesmain->mixnodescomp[0];
		box = mainprogram->deckmonitor[0];
		draw_box(node->outputbox->lcolor, node->outputbox->acolor, box, -1);
		draw_box(nullptr, node->outputbox->acolor, box->vtxcoords->x1 + xs, box->vtxcoords->y1 + ys, box->vtxcoords->w - xs * 2.0f, box->vtxcoords->h - ys * 2.0f, node->mixtex);
		mainprogram->deckmonitor[0]->in();
		node = (MixNode*)mainprogram->nodesmain->mixnodescomp[1];
		box = mainprogram->deckmonitor[1];
		draw_box(node->outputbox->lcolor, node->outputbox->acolor, box, -1);
		draw_box(node->outputbox->lcolor, node->outputbox->acolor, box->vtxcoords->x1 + xs, box->vtxcoords->y1 + ys, box->vtxcoords->w - xs * 2.0f, box->vtxcoords->h - ys * 2.0f, node->mixtex);
		mainprogram->deckmonitor[1]->in();
	}
	
	return true;
}


void drag_into_layerstack(std::vector<Layer*>& layers, bool deck) {
	Layer* lay;
	for (int i = 0; i < layers.size(); i++) {
		lay = layers[i];
		if (mainprogram->draginscrollbarlay) {
			lay = mainprogram->draginscrollbarlay;
		}
		else if (lay->pos < mainmix->scenes[lay->comp][deck][mainmix->currscene[lay->comp][deck]]->scrollpos or lay->pos > mainmix->scenes[lay->comp][deck][mainmix->currscene[lay->comp][deck]]->scrollpos + 2) continue;
		Box* box = lay->node->vidbox;
		int endx = false;
		if ((i == layers.size() - 1 or i == mainmix->scenes[lay->comp][deck][mainmix->currscene[lay->comp][deck]]->scrollpos + 2) and (box->scrcoords->x1 + box->scrcoords->w - mainprogram->xvtxtoscr(tf(0.075f)) < mainprogram->mx and mainprogram->mx < box->scrcoords->x1 + box->scrcoords->w + mainprogram->xvtxtoscr(0.075f))) {
			endx = true;
		}

		if (!mainmix->moving) {
			bool no = false;
			if (mainprogram->dragbinel) {
				if (mainprogram->dragbinel->type == ELEM_DECK or mainprogram->dragbinel->type == ELEM_MIX) {
					no = true;
				}
			}
			if (!no) {
				bool cond1 = (box->scrcoords->y1 < mainprogram->my + box->scrcoords->h and mainprogram->my < box->scrcoords->y1);
				bool cond2 = (box->scrcoords->x1 + box->scrcoords->w * 0.25f < mainprogram->mx and mainprogram->mx < box->scrcoords->x1 + box->scrcoords->w * 0.75f);
				if (mainprogram->draginscrollbarlay or (cond1 and cond2)) {
					mainprogram->draginscrollbarlay = nullptr;
					// handle dragging things into layer monitors of deck
					lay->queueing = true;
					mainprogram->queueing = true;
					mainmix->currlay = lay;
					if (mainprogram->lmover or mainprogram->laymenu1->state > 1 or mainprogram->laymenu2->state > 1 or mainprogram->newlaymenu->state > 1) {
						set_queueing(false);
						if (mainprogram->dragbinel) {
							mainprogram->laymenu1->state = 0;
							mainprogram->laymenu2->state = 0;
							mainprogram->newlaymenu->state = 0;
							if (mainprogram->dragright) {
								mainprogram->dragright = false;
								while (!lay->effects[0].empty()) {
									lay->delete_effect(lay->effects[0].size() - 1);
								}
								lay->scale->value = 1.0f;
								lay->shiftx->value = 0.0f;
								lay->shifty->value = 0.0f;
							}
							if (mainprogram->dragbinel->type == ELEM_LAYER) {
								mainmix->open_layerfile(mainprogram->dragbinel->path, lay, 1, 1);
								if (mainprogram->shelfdragelem) {
									if (mainprogram->shelfdragelem->launchtype == 2) {
										mainprogram->shelfdragelem->nblayers.clear();
										mainprogram->shelfdragelem->nblayers.push_back(lay);
									}
								}
							}
							else if (mainprogram->dragbinel->type == ELEM_FILE) {
								lay->open_video(0, mainprogram->dragbinel->path, true);
							}
							else if (mainprogram->dragbinel->type == ELEM_IMAGE) {
								lay->open_image(mainprogram->dragbinel->path);
							}

							// when something new is dragged into layer, set frames from framecounters set in Mixer::set_prevshelfdragelem()
							if (mainprogram->shelfdragelem) {
								if (mainprogram->shelfdragelem->launchtype == 1) {
									if (mainprogram->shelfdragelem->cframes.size()) {
										lay->frame = mainprogram->shelfdragelem->cframes[0];
									}
								}
								else if (mainprogram->shelfdragelem->launchtype == 2) {
									if (mainprogram->shelfdragelem->nblayers.size()) {
										lay->frame = mainprogram->shelfdragelem->nblayers[0]->frame;
									}
								}
								lay->prevshelfdragelem = mainprogram->shelfdragelem;
							}
						}
						else {
							lay->open_video(0, mainprogram->dragbinel->path, true);
						}
						mainprogram->rightmouse = true;
						binsmain->handle(0);
						enddrag();
						return;
					}
				}
				else if (endx or (box->scrcoords->x1 - mainprogram->xvtxtoscr(0.075f) < mainprogram->mx and mainprogram->mx < box->scrcoords->x1 + mainprogram->xvtxtoscr(0.075f))) {
					// handle dragging things to the end of the last visible layer monitor of deck
					if (mainprogram->lmover) {
						Layer* inlay = mainmix->add_layer(layers, lay->pos + endx);
						if (inlay->pos == mainmix->scenes[inlay->comp][deck][mainmix->currscene[inlay->comp][deck]]->scrollpos + 3) mainmix->scenes[inlay->comp][deck][mainmix->currscene[inlay->comp][deck]]->scrollpos++;
						if (mainprogram->dragbinel) {
							if (mainprogram->dragbinel->type == ELEM_LAYER) {
								mainmix->open_layerfile(mainprogram->dragbinel->path, inlay, 1, 1);
							}
							else {
								inlay->open_video(0, mainprogram->dragbinel->path, true);
							}
						}
						else {
							inlay->open_video(0, mainprogram->dragbinel->path, true);
						}
						mainprogram->rightmouse = true;
						binsmain->handle(0);
						enddrag();
						return;
					}
				}

				int numonscreen = layers.size() - mainmix->scenes[lay->comp][deck][mainmix->currscene[lay->comp][deck]]->scrollpos;
				if (0 <= numonscreen and numonscreen <= 2) {
					if (mainprogram->xvtxtoscr(mainprogram->numw + deck * 1.0f + numonscreen * mainprogram->layw) < mainprogram->mx and mainprogram->mx < deck * (glob->w / 2.0f) + glob->w / 2.0f) {
						if (0 < mainprogram->my and mainprogram->my < mainprogram->yvtxtoscr(mainprogram->layh)) {
							float blue[] = { 0.5, 0.5 , 1.0, 1.0 };
							draw_box(nullptr, blue, -1.0f + mainprogram->numw + deck * 1.0f + numonscreen * mainprogram->layw, 1.0f - mainprogram->layh, mainprogram->layw, mainprogram->layh, -1);
							if (mainprogram->lmover) {
								// handle dragging into black space to the right of deck layer monitors
								lay = mainmix->add_layer(layers, layers.size());
								if (mainprogram->dragbinel) {
									if (mainprogram->dragbinel->type == ELEM_LAYER) {
										mainmix->open_layerfile(mainprogram->dragbinel->path, lay, 1, 1);
									}
									else if (mainprogram->dragbinel->type == ELEM_FILE) {
										lay->open_video(0, mainprogram->dragbinel->path, true);
									}
									else if (mainprogram->dragbinel->type == ELEM_IMAGE) {
										lay->open_image(mainprogram->dragbinel->path);
									}
								}
								else {
									lay->open_video(0, mainprogram->dragbinel->path, true);
								}
								mainprogram->rightmouse = true;
								binsmain->handle(0);
								enddrag();
								return;
							}
						}
					}
				}
			}
		}
	}
}


// check all STATIC/DYNAMIC draws
void Shelf::handle() {
	// draw shelves and handle shelves
	int inelem = -1;
	for (int i = 0; i < 16; i++) {
		ShelfElement* elem = this->elements[i];
		// border coloring according to element type
		if (elem->type == ELEM_LAYER) {
			draw_box(orange, orange, this->buttons[i]->box, -1);
			draw_box(nullptr, orange, this->buttons[i]->box->vtxcoords->x1 + tf(0.005f), this->buttons[i]->box->vtxcoords->y1, this->buttons[i]->box->vtxcoords->w - tf(0.005f), this->buttons[i]->box->vtxcoords->h - tf(0.005f), elem->tex);
		}
		else if (elem->type == ELEM_DECK) {
			draw_box(purple, purple, this->buttons[i]->box, -1);
			draw_box(nullptr, purple, this->buttons[i]->box->vtxcoords->x1 + tf(0.005f), this->buttons[i]->box->vtxcoords->y1, this->buttons[i]->box->vtxcoords->w - tf(0.005f), this->buttons[i]->box->vtxcoords->h - tf(0.005f), elem->tex);
		}
		else if (elem->type == ELEM_MIX) {
			draw_box(green, green, this->buttons[i]->box, -1);
			draw_box(nullptr, green, this->buttons[i]->box->vtxcoords->x1 + tf(0.005f), this->buttons[i]->box->vtxcoords->y1, this->buttons[i]->box->vtxcoords->w - tf(0.005f), this->buttons[i]->box->vtxcoords->h - tf(0.005f), elem->tex);
		}
		else if (elem->type == ELEM_IMAGE) {
			draw_box(pink, pink, this->buttons[i]->box, -1);
			draw_box(nullptr, pink, this->buttons[i]->box->vtxcoords->x1 + tf(0.005f), this->buttons[i]->box->vtxcoords->y1, this->buttons[i]->box->vtxcoords->w - tf(0.005f), this->buttons[i]->box->vtxcoords->h - tf(0.005f), elem->tex);
		}
		else {
			draw_box(grey, grey, this->buttons[i]->box, -1);
			draw_box(nullptr, grey, this->buttons[i]->box->vtxcoords->x1 + tf(0.005f), this->buttons[i]->box->vtxcoords->y1, this->buttons[i]->box->vtxcoords->w - tf(0.005f), this->buttons[i]->box->vtxcoords->h - tf(0.005f), elem->tex);
		}

		// draw small icons for choice of launch play type of shelf video
		if (elem->launchtype == 0) {
			draw_box(nullptr, yellow, elem->sbox, -1);
			//render_text("S", white, elem->sbox->vtxcoords->x1, elem->sbox->vtxcoords->y1 + 0.005f, tf(0.00024f), tf(0.0004f));
		}
		else {
			draw_box(white, black, elem->sbox, -1);
			//render_text("S", white, elem->sbox->vtxcoords->x1, elem->sbox->vtxcoords->y1 + 0.005f, tf(0.00024f), tf(0.0004f));
		}
		if (elem->launchtype == 1) {
			draw_box(nullptr, red, elem->pbox, -1);
			//render_text("P", white, elem->pbox->vtxcoords->x1, elem->pbox->vtxcoords->y1 + 0.005f, tf(0.00024f), tf(0.0004f));
		}
		else {
			draw_box(white, black, elem->pbox, -1);
			//render_text("P", white, elem->pbox->vtxcoords->x1, elem->pbox->vtxcoords->y1 + 0.005f, tf(0.00024f), tf(0.0004f));
		}
		if (elem->launchtype == 2) {
			draw_box(nullptr, darkblue, elem->cbox, -1);
			//render_text("C", white, elem->cbox->vtxcoords->x1, elem->cbox->vtxcoords->y1 + 0.005f, tf(0.00024f), tf(0.0004f));
		}
		else {
			draw_box(white, black, elem->cbox, -1);
			//render_text("C", white, elem->cbox->vtxcoords->x1, elem->cbox->vtxcoords->y1 + 0.005f, tf(0.00024f), tf(0.0004f));
		}
		bool cond = elem->button->box->in(); // trigger before launchtype boxes, to get the right tooltips
		if (elem->sbox->in()) {
			if (mainprogram->leftmouse) {
				// video restarts at each trigger
				elem->launchtype = 0;
			}
		}
		if (elem->pbox->in()) {
			if (mainprogram->leftmouse) {
				// video pauses when gone, continues when triggered again
				elem->launchtype = 1;
			}
		}
		if (elem->cbox->in()) {
			if (mainprogram->leftmouse) {
				// video keeps on running in background (at least, its frame numbers are counted!)
				elem->launchtype = 2;
			}
		}
		// calculate background frame numbers for elems with launchtype 2
		for (int j = 0; j < elem->nblayers.size(); j++) {
			elem->nblayers[j]->calc_texture(!mainprogram->prevmodus, 0);
		}

		if (cond) {
			inelem = i;
			if (mainprogram->rightmouse) {
				if (mainprogram->dragbinel) {
					if (mainprogram->shelfdragelem) {
						if (this->prevnum != -1) std::swap(this->elements[this->prevnum]->tex, this->elements[this->prevnum]->oldtex);
						std::swap(elem->tex, mainprogram->shelfdragelem->tex);
						mainprogram->shelfdragelem->tex = mainprogram->dragbinel->tex;
						mainprogram->shelfdragelem = nullptr;
						enddrag();
						//mainprogram->menuactivation = false;
					}
				}
			}
			this->newnum = i;
			mainprogram->inshelf = this->side;
			if (mainprogram->dragbinel and i != this->prevnum) {
				std::swap(elem->tex, elem->oldtex);
				if (this->prevnum != -1) std::swap(this->elements[this->prevnum]->tex, this->elements[this->prevnum]->oldtex);
				if (mainprogram->shelfdragelem) {
					std::swap(mainprogram->shelfdragelem->tex, mainprogram->shelfdragelem->oldtex);
					mainprogram->shelfdragelem->tex = elem->oldtex;
				}
				elem->tex = mainprogram->dragbinel->tex;
				this->prevnum = i;
			}
			if (mainprogram->leftmousedown or mainprogram->rightmousedown) {
				if (!elem->sbox->in()and !elem->pbox->in()and !elem->cbox->in()) {
					if (!mainprogram->dragbinel) {
						// user starts dragging shelf element
						if (elem->path != "") {
							mainprogram->dragright = mainprogram->rightmousedown;
							mainprogram->shelfdragelem = elem;
							mainprogram->shelfdragnum = i;
							mainprogram->leftmousedown = false;
							mainprogram->rightmousedown = false;
							mainprogram->dragbinel = new BinElement;
							mainprogram->dragbinel->path = elem->path;
							mainprogram->dragbinel->type = elem->type;
							mainprogram->dragbinel->tex = elem->tex;
						}
					}
				}
			}
			else if (mainprogram->lmover) {
				// user drops file/layer/deck/mix in shelf element
				if (mainprogram->dragbinel) {
					std::string newpath;
					std::string extstr = "";
					if (mainprogram->dragbinel->type == ELEM_LAYER) {
						extstr = ".layer";
					}
					else if (mainprogram->dragbinel->type == ELEM_DECK) {
						extstr = ".deck";
					}
					else if (mainprogram->dragbinel->type == ELEM_MIX) {
						extstr = ".mix";
					}
					if (extstr != "") {
						std::string base = basename(mainprogram->dragbinel->path);
						std::string name = remove_extension("shelf_" + base);
						int count = 0;
						while (1) {
							newpath = mainprogram->project->shelfdir + name + extstr;
							if (!exists(newpath)) {
								boost::filesystem::copy_file(mainprogram->dragbinel->path, newpath);
								mainprogram->dragbinel->path = newpath;
								break;
							}
							count++;
							name = remove_version(name) + "_" + std::to_string(count);
						}
					}
					if (mainprogram->shelfdragelem) {
						std::swap(elem->path, mainprogram->shelfdragelem->path);
						std::swap(elem->jpegpath, mainprogram->shelfdragelem->jpegpath);
						std::swap(elem->type, mainprogram->shelfdragelem->type);
					}
					else {
						elem->path = mainprogram->dragbinel->path;
						elem->type = mainprogram->dragbinel->type;
					}
					elem->tex = copy_tex(mainprogram->dragbinel->tex);
					blacken(elem->oldtex);
					this->prevnum = -1;
					mainprogram->shelfdragelem = nullptr;
					mainprogram->rightmouse = true;
					binsmain->handle(0);
					enddrag();
					break;
				}
			}
			if (mainprogram->menuactivation) {
				mainprogram->shelfmenu->state = 2;
				mainmix->mouseshelf = this;
				mainmix->mouseshelfelem = i;
				mainprogram->menuactivation = false;
			}
		}
	}
	if (inelem > -1 and mainprogram->dragbinel) {
		mainprogram->dragout[this->side] = false;
		mainprogram->dragout[!this->side] = true;
	}
	if (inelem == -1 and !mainprogram->dragout[this->side] and mainprogram->dragbinel) {
		// mouse not over shelf element
		mainprogram->dragout[this->side] == true;
		if (this->prevnum != -1) {
			std::swap(this->elements[this->prevnum]->tex, this->elements[this->prevnum]->oldtex);
			if (mainprogram->shelfdragelem) std::swap(mainprogram->shelfdragelem->tex, mainprogram->shelfdragelem->oldtex);
			if (mainprogram->shelfdragnum = prevnum) {
				std::swap(mainprogram->shelfdragelem->tex, mainprogram->shelfdragelem->oldtex);
			}
		}
		this->prevnum = -1;
	}
	else if (!mainprogram->dragout[this->side]) this->prevnum = this->newnum;
}


void make_layboxes() {

	for (int i = 0; i < mainprogram->nodesmain->pages.size(); i++) {
		for (int j = 0; j < mainprogram->nodesmain->pages[i]->nodescomp.size(); j++) {
			Node *node = mainprogram->nodesmain->pages[i]->nodescomp[j];
			
			if (mainprogram->nodesmain->linked) {
				if (node->type == VIDEO) {
					VideoNode *vnode = (VideoNode*)node;
					float xoffset;
					int deck;
					if (std::find(mainmix->layersAcomp.begin(), mainmix->layersAcomp.end(), vnode->layer) != mainmix->layersAcomp.end()) {
						xoffset = 0.0f;
						deck = 0;
					}
					else {
						xoffset = 1.0f;
						deck = 1;
					}
					int lpos = vnode->layer->pos - mainmix->scenes[vnode->layer->comp][deck][mainmix->currscene[vnode->layer->comp][deck]]->scrollpos;
					if (lpos < 0 or lpos > 2) vnode->vidbox->vtxcoords->x1 = 2.0f;
					else vnode->vidbox->vtxcoords->x1 = -1.0f + (float)(lpos % 3) * mainprogram->layw + mainprogram->numw + xoffset;
					vnode->vidbox->vtxcoords->y1 = 1.0f - mainprogram->layh;
					vnode->vidbox->vtxcoords->w = mainprogram->layw;
					vnode->vidbox->vtxcoords->h = mainprogram->layh;
					vnode->vidbox->upvtxtoscr();
					
					vnode->vidbox->lcolor[0] = 1.0;
					vnode->vidbox->lcolor[1] = 1.0;
					vnode->vidbox->lcolor[2] = 1.0;
					vnode->vidbox->lcolor[3] = 1.0;
					if (vnode->layer->effects[0].size() == 0) {
						if (vnode->layer->drawfbo2) vnode->vidbox->tex = vnode->layer->fbotex;
						else vnode->vidbox->tex = vnode->layer->fbotexintm;
					}
					else {
						vnode->vidbox->tex = vnode->layer->effects[0][vnode->layer->effects[0].size() - 1]->fbotex;
					}
				}
				else continue;
			}
		}
	}
	for (int i = 0; i < mainprogram->nodesmain->pages.size(); i++) {
		for (int j = 0; j < mainprogram->nodesmain->pages[i]->nodes.size(); j++) {
			Node *node = mainprogram->nodesmain->pages[i]->nodes[j];
			if (mainprogram->nodesmain->linked) {
				if (node->type == VIDEO) {
					if (((VideoNode*)node)->layer) {  // reminder: trick, not solution...
						VideoNode* vnode = (VideoNode*)node;
						float xoffset;
						int deck;
						if (std::find(mainmix->layersA.begin(), mainmix->layersA.end(), vnode->layer) != mainmix->layersA.end()) {
							xoffset = 0.0f;
							deck = 0;
						}
						else {
							xoffset = 1.0f;
							deck = 1;
						}
						int lpos = vnode->layer->pos - mainmix->scenes[vnode->layer->comp][deck][mainmix->currscene[vnode->layer->comp][deck]]->scrollpos;
						if (lpos < 0 or lpos > 2) vnode->vidbox->vtxcoords->x1 = 2.0f;
						else vnode->vidbox->vtxcoords->x1 = -1.0f + (float)(lpos % 3) * mainprogram->layw + mainprogram->numw + xoffset;
						vnode->vidbox->vtxcoords->y1 = 1.0f - mainprogram->layh;
						vnode->vidbox->vtxcoords->w = mainprogram->layw;
						vnode->vidbox->vtxcoords->h = mainprogram->layh;
						vnode->vidbox->upvtxtoscr();

						vnode->vidbox->lcolor[0] = 1.0;
						vnode->vidbox->lcolor[1] = 1.0;
						vnode->vidbox->lcolor[2] = 1.0;
						vnode->vidbox->lcolor[3] = 1.0;
						if ((vnode)->layer->effects[0].size() == 0) {
							if (vnode->layer->drawfbo2) vnode->vidbox->tex = vnode->layer->fbotex;
							else vnode->vidbox->tex = vnode->layer->fbotexintm;
						}
						else {
							vnode->vidbox->tex = vnode->layer->effects[0][vnode->layer->effects[0].size() - 1]->fbotex;
						}
					}
				}
				else continue;
			}
			else {
				if (node->monitor != -1) {
					node->monbox->vtxcoords->x1 = -1.0f + (float)(node->monitor % 6) * mainprogram->layw + mainprogram->numw;
					if (node->type == VIDEO) {
						node->monbox->tex = ((VideoNode*)node)->layer->fbotex;
					}
					else if (node->type == EFFECT) {
						node->monbox->tex = ((EffectNode*)node)->effect->fbotex;
					}
					else if (node->type == MIX) {
						node->monbox->tex = ((MixNode*)node)->mixtex;
					}
					else if (node->type == BLEND) {
						node->monbox->tex = ((BlendNode*)node)->fbotex;
					}
				}
				else continue;
			}
			node->monbox->vtxcoords->y1 = 1.0f - mainprogram->layh;
			node->monbox->vtxcoords->w = mainprogram->layw;
			node->monbox->vtxcoords->h = mainprogram->layh;
			node->monbox->upvtxtoscr();
			
			node->monbox->lcolor[0] = 1.0;
			node->monbox->lcolor[1] = 1.0;
			node->monbox->lcolor[2] = 1.0;
			node->monbox->lcolor[3] = 1.0;
		}
	}

	float xoffset;
	if (mainprogram->nodesmain->linked) {
		for (int j = 0; j < 4; j++) {
			std::vector<Layer*> lvec;
			if (j == 0) {
				lvec = mainmix->layersA;
				xoffset = 0.0f;
			}
			else if (j == 1) {
				lvec = mainmix->layersB;
				xoffset = 1.0f - 0.2f;
			}
			else if (j == 2) {
				lvec = mainmix->layersAcomp;
				xoffset = 0.0f;
			}
			else if (j == 3) {
				lvec = mainmix->layersBcomp;
				xoffset = 1.0f - 0.2f;
			}
			for (int i = 0; i < lvec.size(); i++) {
				Layer *testlay = lvec[i];
				//testlay->node->upeffboxes();
				// set mixbox position and dimensions
				testlay->mixbox->vtxcoords->x1 = -1.0f + mainprogram->numw + xoffset + ((mainmix->scenes[testlay->comp][testlay->deck][mainmix->currscene[testlay->comp][testlay->deck]]->scrollpos + testlay->pos) % 3) * mainprogram->layw;
				testlay->mixbox->vtxcoords->y1 = 1.0f - mainprogram->layh - tf(0.09f);
				testlay->mixbox->vtxcoords->w = tf(0.05f);
				testlay->mixbox->vtxcoords->h = tf(0.05f);
				testlay->mixbox->upvtxtoscr();
				testlay->colorbox->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + tf(0.07f);
				testlay->colorbox->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1;
				testlay->colorbox->vtxcoords->w = tf(0.05f);
				testlay->colorbox->vtxcoords->h = tf(0.05f);
				testlay->colorbox->upvtxtoscr();
		
				// shift effectboxes and parameterboxes according to position in stack and scrollposition of stack
				std::vector<Effect*> &evec = testlay->choose_effects();
				Effect *prevmodus = nullptr;
				for (int j = 0; j < evec.size(); j++) {
					Effect *eff = evec[j];
					eff->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + tf(0.05f);
					eff->onoffbutton->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + tf(0.025f);
					eff->drywet->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1;
					float dy;
					if (prevmodus) {
						if (prevmodus->params.size() < 4) {
							eff->box->vtxcoords->y1 = prevmodus->box->vtxcoords->y1 - tf(0.05f);
						}
						else if (prevmodus->params.size() > 5) {
							eff->box->vtxcoords->y1 = prevmodus->box->vtxcoords->y1 - tf(0.15f);
						}
						else {
							eff->box->vtxcoords->y1 = prevmodus->box->vtxcoords->y1 - tf(0.1f);
						}
					}
					else {
						eff->box->vtxcoords->y1 = 1.0 - mainprogram->layh - tf(0.29f) + (tf(0.05f) * testlay->effscroll[mainprogram->effcat[testlay->deck]->value]);
					}
					eff->box->upvtxtoscr();
					eff->onoffbutton->box->vtxcoords->y1 = eff->box->vtxcoords->y1;
					eff->onoffbutton->box->upvtxtoscr();
					eff->drywet->box->vtxcoords->y1 = eff->box->vtxcoords->y1;
					eff->drywet->box->upvtxtoscr();
					prevmodus = eff;
					
					//for (int k = 0; k < eff->params.size(); k++) {
					//	if (eff->params[k]->midi[0] != -1) {
					//		eff->params[k]->node->box->vtxcoords->x1 = eff->node->box->vtxcoords->x1 + tf(0.0625f) * (k - eff->params.size() / 2.0f) - tf(0.025);
					//		eff->params[k]->node->box->upvtxtoscr();
					//	}
					//}
				}
				
				// Make GUI box of mixing factor slider
				testlay->blendnode->mixfac->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + tf(0.08f);
				testlay->blendnode->mixfac->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1;
				testlay->blendnode->mixfac->box->vtxcoords->w = tf(mainprogram->layw) * 0.25f;
				testlay->blendnode->mixfac->box->vtxcoords->h = tf(0.05f);
				testlay->blendnode->mixfac->box->upvtxtoscr();
				
				// Make GUI box of video speed slider
				testlay->speed->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1;
				testlay->speed->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - tf(0.05f);
				testlay->speed->box->vtxcoords->w = tf(mainprogram->layw) * 0.5f;
				testlay->speed->box->vtxcoords->h = tf(0.05f);
				testlay->speed->box->upvtxtoscr();
				
				// GUI box of layer opacity slider
				testlay->opacity->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1;
				testlay->opacity->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - tf(0.15f);
				testlay->opacity->box->vtxcoords->w = tf(mainprogram->layw) * 0.25f;
				testlay->opacity->box->vtxcoords->h = tf(0.05f);
				testlay->opacity->box->upvtxtoscr();
				
				// GUI box of layer audio volume slider
				testlay->volume->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + tf(mainprogram->layw) * 0.30f;
				testlay->volume->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - tf(0.15f);
				testlay->volume->box->vtxcoords->w = tf(mainprogram->layw) * 0.25f;
				testlay->volume->box->vtxcoords->h = tf(0.05f);
				testlay->volume->box->upvtxtoscr();
				
				// GUI box of play video button
				testlay->playbut->box->acolor[0] = 0.5;
				testlay->playbut->box->acolor[1] = 0.2;
				testlay->playbut->box->acolor[2] = 0.5;
				testlay->playbut->box->acolor[3] = 1.0;
				testlay->playbut->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + tf(mainprogram->layw) * 0.5f + tf(0.088f);
				testlay->playbut->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - tf(0.05f);
				testlay->playbut->box->vtxcoords->w = tf(0.025f);
				testlay->playbut->box->vtxcoords->h = tf(0.05f);
				testlay->playbut->box->upvtxtoscr();
				
				// GUI box of bounce play video button
				testlay->bouncebut->box->acolor[0] = 0.5;
				testlay->bouncebut->box->acolor[1] = 0.2;
				testlay->bouncebut->box->acolor[2] = 0.5;
				testlay->bouncebut->box->acolor[3] = 1.0;
				testlay->bouncebut->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + tf(mainprogram->layw) * 0.5f + tf(0.058f);
				testlay->bouncebut->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - tf(0.05f);
				testlay->bouncebut->box->vtxcoords->w = tf(0.03f);
				testlay->bouncebut->box->vtxcoords->h = tf(0.05f);
				testlay->bouncebut->box->upvtxtoscr();
				
				// GUI box of reverse play video button
				testlay->revbut->box->acolor[0] = 0.5;
				testlay->revbut->box->acolor[1] = 0.2;
				testlay->revbut->box->acolor[2] = 0.5;
				testlay->revbut->box->acolor[3] = 1.0;
				testlay->revbut->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + tf(mainprogram->layw) * 0.5f + tf(0.033f);
				testlay->revbut->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - tf(0.05f);
				testlay->revbut->box->vtxcoords->w = tf(0.025f);
				testlay->revbut->box->vtxcoords->h = tf(0.05f);
				testlay->revbut->box->upvtxtoscr();
				
				// GUI box of video frame forward button
				testlay->frameforward->box->acolor[0] = 0.5;
				testlay->frameforward->box->acolor[1] = 0.2;
				testlay->frameforward->box->acolor[2] = 0.5;
				testlay->frameforward->box->acolor[3] = 1.0;
				testlay->frameforward->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + tf(mainprogram->layw) * 0.5f + tf(0.113f);
				testlay->frameforward->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - tf(0.05f);
				testlay->frameforward->box->vtxcoords->w = tf(0.025f);
				testlay->frameforward->box->vtxcoords->h = tf(0.05f);
				testlay->frameforward->box->upvtxtoscr();
				
				// GUI box of video frame backward button
				testlay->framebackward->box->acolor[0] = 0.5;
				testlay->framebackward->box->acolor[1] = 0.2;
				testlay->framebackward->box->acolor[2] = 0.5;
				testlay->framebackward->box->acolor[3] = 1.0;
				testlay->framebackward->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + tf(mainprogram->layw) * 0.5f + tf(0.008f);
				testlay->framebackward->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - tf(0.05f);
				testlay->framebackward->box->vtxcoords->w = tf(0.025f);
				testlay->framebackward->box->vtxcoords->h = tf(0.05f);
				testlay->framebackward->box->upvtxtoscr();
				
				// GUI box of specific general midi set for layer switch
				testlay->genmidibut->box->acolor[0] = 0.5;
				testlay->genmidibut->box->acolor[1] = 0.2;
				testlay->genmidibut->box->acolor[2] = 0.5;
				testlay->genmidibut->box->acolor[3] = 1.0;
				testlay->genmidibut->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + tf(mainprogram->layw) * 0.5f + tf(0.138f);
				testlay->genmidibut->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - tf(0.05f);
				testlay->genmidibut->box->vtxcoords->w = tf(0.025f);
				testlay->genmidibut->box->vtxcoords->h = tf(0.05f);
				testlay->genmidibut->box->upvtxtoscr();
				
				// GUI box of scratch video box
				testlay->loopbox->acolor[0] = 0.5;
				testlay->loopbox->acolor[1] = 0.2;
				testlay->loopbox->acolor[2] = 0.5;
				testlay->loopbox->acolor[3] = 1.0;
				testlay->loopbox->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1;
				testlay->loopbox->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - tf(0.10f);
				testlay->loopbox->vtxcoords->w = tf(mainprogram->layw);
				testlay->loopbox->vtxcoords->h = tf(0.05f);
				testlay->loopbox->upvtxtoscr();
				
				if (mainprogram->nodesmain->linked) {
					if (testlay->pos > 0) {
						testlay->node->box->scrcoords->x1 = testlay->blendnode->box->scrcoords->x1 - mainprogram->xvtxtoscr(tf(0.15));
					}
					else {
						testlay->node->box->scrcoords->x1 = mainprogram->xvtxtoscr(tf(0.078));
					}
					testlay->node->box->upscrtovtx();
				}
				
				// GUI box of colourkey direction switch
				testlay->chdir->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + tf(0.24f);
				testlay->chdir->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1;
				testlay->chdir->box->vtxcoords->w = tf(0.025f);
				testlay->chdir->box->vtxcoords->h = tf(0.05f);
				testlay->chdir->box->upvtxtoscr();
				
				// GUI box of colourkey inversion switch
				testlay->chinv->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + tf(0.285f);
				testlay->chinv->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1;
				testlay->chinv->box->vtxcoords->w = tf(0.025f);
				testlay->chinv->box->vtxcoords->h = tf(0.05f);
				testlay->chinv->box->upvtxtoscr();
			}
		}
	}
}


int osc_param(const char *path, const char *types, lo_arg **argv, int argc, lo_message m, void *data) {
	Param *par = (Param*)data;
	par->value = par->range[0] + argv[0]->f * (par->range[1] - par->range[0]);
	return 0;
}


Box::Box() {
	this->vtxcoords = new BOX_COORDS;
	this->scrcoords = new BOX_COORDS;
}

Box::~Box() {
	//delete this->vtxcoords;  reminder: throws breakpoint
	//delete this->scrcoords;
}

Button::Button(bool state) {
	this->box = new Box;
	this->value = state;
	this->ccol[3] = 1.0f;
	if (mainprogram) {
		mainprogram->buttons.push_back(this);
		if (mainprogram->prevmodus) {
			if (lp) lp->allbuttons.push_back(this);
		}
		else {
			if (lpc) lpc->allbuttons.push_back(this);
		}
	}
}

Button::~Button() {
	delete this->box;
}

bool Button::handle(bool circlein) {
	bool ret = this->handle(circlein, true);
	return ret;
}

bool Button::handle(bool circlein, bool automation) {
	bool changed = false;
	if (this->box->in()) {
		if (mainprogram->leftmouse) {
			this->oldvalue = this->value;
			this->value++;
			if (this->value > this->toggle) this->value = 0;
			if (this->toggle == 0) this->value = 1;
			if (automation) {
				for (int i = 0; i < loopstation->elems.size(); i++) {
					if (loopstation->elems[i]->recbut->value) {
						loopstation->elems[i]->add_button(this);
					}
				}
			}
			changed = true;
		}
		if (mainprogram->menuactivation and !mainprogram->menuondisplay) {
			if (loopstation->butelemmap.find(this) != loopstation->butelemmap.end()) mainprogram->parammenu4->state = 2;
			else mainprogram->parammenu3->state = 2;
			mainmix->learnparam = nullptr;
			mainmix->learnbutton = this;
			mainprogram->menuactivation = false;
		}
		this->box->acolor[0] = 0.5f;
		this->box->acolor[1] = 0.5f;
		this->box->acolor[2] = 1.0f;
		this->box->acolor[3] = 1.0f;
	}
	else if (this->value and !circlein) {
		this->box->acolor[0] = this->tcol[0];
		this->box->acolor[1] = this->tcol[1];
		this->box->acolor[2] = this->tcol[2];
		this->box->acolor[3] = this->tcol[3];
	}
	else {
		this->box->acolor[0] = 0.0f;
		this->box->acolor[1] = 0.0f;
		this->box->acolor[2] = 0.0f;
		this->box->acolor[3] = 1.0f;
	}
	draw_box(this->box, -1);
	
	if (circlein) {
		float radx = this->box->vtxcoords->w / 2.0f;
		float rady = this->box->vtxcoords->h / 2.0f;
		if (this->value) {
			draw_box(this->ccol, this->box->vtxcoords->x1 + radx, this->box->vtxcoords->y1 + rady, tf(0.015f), 1);
		}	
		else draw_box(this->ccol, this->box->vtxcoords->x1 + radx, this->box->vtxcoords->y1 + rady, tf(0.015f), 2);
	}

	return changed;
}

bool Button::toggled() {
	if (this->value != this->oldvalue) {
		this->oldvalue = this->value;
		return true;
	}
	else return false;
}


void Box::upscrtovtx() {
	int hw = glob->w / 2;
	int hh = glob->h / 2;
	this->vtxcoords->x1 = ((this->scrcoords->x1 - hw) / hw);
	this->vtxcoords->y1 = ((this->scrcoords->y1 - hh) / -hh);
	this->vtxcoords->w = this->scrcoords->w / hw;
	this->vtxcoords->h = this->scrcoords->h / hh;
}

void Box::upvtxtoscr() {
	int hw = glob->w / 2;
	int hh = glob->h / 2;
	this->scrcoords->x1 = ((this->vtxcoords->x1 * hw) + hw);
	this->scrcoords->h = this->vtxcoords->h * hh;
	this->scrcoords->y1 = ((this->vtxcoords->y1 * -hh) + hh);
	this->scrcoords->w = this->vtxcoords->w * hw;
}

bool Box::in(bool menu) {
	if (menu) return this->in();
	return false;
}

bool Box::in() {
	bool ret = this->in(mainprogram->mx, mainprogram->my);
	return ret;
}

bool Box::in(int mx, int my) {
	if (mainprogram->menuondisplay) return false;
	if (this->scrcoords->x1 <= mx and mx <= this->scrcoords->x1 + this->scrcoords->w) {
		if (this->scrcoords->y1 - this->scrcoords->h <= my and my <= this->scrcoords->y1) {
			if (mainprogram->showtooltips and !mainprogram->ttreserved) {
				mainprogram->tooltipbox = this;
				mainprogram->ttreserved = this->reserved;
			}
			return true;
		}
	}
	return false;
}


	
Clip::Clip() {
	this->box = new Box;
	this->box->tooltiptitle = "Clip queue ";
	this->box->tooltip = "Clip queue: clips (videos, images, layer files, live feeds) loaded here are played in order after the current clip.  Rightclick menu allows loading live feed / opening content into clip / deleting clip.  Clips can be dragged anywhere and anything can be dragged into or inserted between them. ";
	this->tex = -1;
}

Clip::~Clip() {
	glDeleteTextures(1, &this->tex);
	if (this->path.rfind(".layer") != std::string::npos) {
		if (this->path.find("cliptemp_") != std::string::npos) {
			boost::filesystem::remove(this->path);			
		}
	}
}

GLuint get_imagetex(const std::string& path) {
	Layer* lay = new Layer(true);
	lay->dummy = 1;
	lay->open_image(path);
	if (lay->filename == "") return -1;
	glBindTexture(GL_TEXTURE_2D, lay->texture);
	ilBindImage(lay->boundimage);
	ilActiveImage((int)lay->frame);
	int imageformat = ilGetInteger(IL_IMAGE_FORMAT);
	int w = ilGetInteger(IL_IMAGE_WIDTH);
	int h = ilGetInteger(IL_IMAGE_HEIGHT);
	int bpp = ilGetInteger(IL_IMAGE_BPP);
	GLuint mode = GL_BGR;
	if (imageformat == IL_RGBA) mode = GL_RGBA;
	if (imageformat == IL_BGRA) mode = GL_BGRA;
	if (imageformat == IL_RGB) mode = GL_RGB;
	if (imageformat == IL_BGR) mode = GL_BGR;
	if (bpp == 3) {
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, mode, GL_UNSIGNED_BYTE, ilGetData());
	}
	else if (bpp == 4) {
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, mode, GL_UNSIGNED_BYTE, ilGetData());
	}
	GLuint ctex = copy_tex(lay->texture);
	lay->closethread = true;
	while (lay->closethread) {
		lay->ready = true;
		lay->startdecode.notify_one();
	}
	return ctex;
}

bool Clip::get_imageframes() {
	ILuint boundimage;
	ilGenImages(1, &boundimage);
	ilBindImage(boundimage);
	ILboolean ret = ilLoadImage(this->path.c_str());
	if (ret == IL_FALSE) {
		printf("can't load image %s\n", this->path.c_str());
		return false;
	}
	int numf = ilGetInteger(IL_NUM_IMAGES);
	this->startframe = 0;
	this->frame = 0.0f;
	this->endframe = numf;

	return true;
}

GLuint get_videotex(const std::string& path) {
	Layer* lay = new Layer(true);
	lay->dummy = 1;
	GLuint ctex;
	glGenTextures(1, &ctex);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, ctex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	lay->open_video(0, path, true);
	std::unique_lock<std::mutex> olock(lay->endopenlock);
	lay->endopenvar.wait(olock, [&] {return lay->opened; });
	lay->opened = false;
	olock.unlock();
	if (lay->openerr) {
		printf("error!\n");
		lay->openerr = false;
		//delete lay;
		return -1;
	}
	lay->frame = lay->numf / 2.0f;
	lay->prevframe = lay->frame - 1;
	lay->initialized = true;
	lay->ready = true;
	while (lay->ready) {
		lay->startdecode.notify_one();
	}
	std::unique_lock<std::mutex> lock2(lay->enddecodelock);
	lay->enddecodevar.wait(lock2, [&] {return lay->processed; });
	lay->processed = false;
	lock2.unlock();
	if (lay->vidformat == 188 or lay->vidformat == 187) {
		if (lay->decresult->compression == 187) {
			glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, lay->decresult->width, lay->decresult->height, 0, lay->decresult->size, lay->decresult->data);
		}
		else if (lay->decresult->compression == 190) {
			glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, lay->decresult->width, lay->decresult->height, 0, lay->decresult->size, lay->decresult->data);
		}
	}
	else {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, lay->decresult->width, lay->decresult->height, 0, GL_BGRA, GL_UNSIGNED_BYTE, lay->decresult->data);
	}
	return ctex;
}

bool Clip::get_videoframes() {
	AVFormatContext* video = nullptr;
	AVStream* video_stream = nullptr;
	int video_stream_idx = -1;
	int r = avformat_open_input(&video, this->path.c_str(), nullptr, nullptr);
	printf("loading... %s\n", this->path.c_str());
	if (r < 0) {
		printf("%s\n", "Couldnt open video");
		return false;
	}
	avformat_find_stream_info(video, nullptr);
	open_codec_context(&(video_stream_idx), video, AVMEDIA_TYPE_VIDEO);
	video_stream = video->streams[video_stream_idx];
	this->frame = 0.0f;
	this->startframe = 0;
	this->endframe = video_stream->nb_frames;

	return true;
}

GLuint get_layertex(const std::string& path) {
	Layer* lay = new Layer(false);
	lay->dummy = true;
	lay->pos = 0;
	lay->node = mainprogram->nodesmain->currpage->add_videonode(2);
	lay->node->layer = lay;
	lay->lasteffnode[0] = lay->node;
	lay->lasteffnode[1] = lay->node;
	mainmix->open_layerfile(path, lay, true, 0);
	std::unique_lock<std::mutex> olock(lay->endopenlock);
	lay->endopenvar.wait(olock, [&] {return lay->opened; });
	lay->opened = false;
	olock.unlock();
	if (lay->filename == "") return -1;
	lay->node->calc = true;
	lay->node->walked = false;
	lay->playbut->value = false;
	lay->revbut->value = false;
	lay->bouncebut->value = false;
	for (int k = 0; k < lay->effects[0].size(); k++) {
		lay->effects[0][k]->node->calc = true;
		lay->effects[0][k]->node->walked = false;
	}
	lay->frame = 0.0f;
	lay->prevframe = -1;
	lay->ready = true;
	while (lay->ready) {
		lay->startdecode.notify_one();
	}
	std::unique_lock<std::mutex> lock(lay->enddecodelock);
	lay->enddecodevar.wait(lock, [&] {return lay->processed; });
	lay->processed = false;
	lock.unlock();

	lay->initialized = true;
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, lay->texture);
	if (lay->vidformat == 188 or lay->vidformat == 187) {
		if (lay->decresult->compression == 187) {
			glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, lay->decresult->width, lay->decresult->height, 0, lay->decresult->size, lay->decresult->data);
		}
		else if (lay->decresult->compression == 190) {
			glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, lay->decresult->width, lay->decresult->height, 0, lay->decresult->size, lay->decresult->data);
		}
	}
	else {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, lay->decresult->width, lay->decresult->height, 0, GL_BGRA, GL_UNSIGNED_BYTE, lay->decresult->data);
	}
	lay->initialized = true;
	lay->fbotex = copy_tex(lay->texture);
	onestepfrom(0, lay->node, nullptr, -1, -1);
	GLuint ctex;
	if (lay->effects[0].size()) {
		ctex = copy_tex(lay->effects[0][lay->effects[0].size() - 1]->fbotex);
	}
	else {
		if (lay->drawfbo2) ctex = copy_tex(lay->fbotex);
		else ctex = copy_tex(lay->fbotexintm);
	}
	return ctex;
}

bool Clip::get_layerframes() {
	std::string result = deconcat_files(this->path);
	bool concat = (result != "");
	std::ifstream rfile;
	if (concat) rfile.open(result);
	else rfile.open(this->path);

	std::string istring;

	while (getline(rfile, istring)) {
		if (istring == "ENDOFFILE") break;
		if (istring == "STARTFRAME") {
			getline(rfile, istring);
			this->startframe = std::stoi(istring);
		}
		if (istring == "ENDFRAME") {
			getline(rfile, istring);
			this->endframe = std::stoi(istring);
		}
		if (istring == "FRAME") {
			getline(rfile, istring);
			this->frame = std::stof(istring);
		}
	}

	rfile.close();

	return 1;
}

GLuint get_deckmixtex(const std::string& path) {

	std::string result = deconcat_files(path);
	bool concat = (result != "");
	std::ifstream rfile;
	if (concat) rfile.open(result);
	else rfile.open(path);
	std::string istring;

	int32_t num;
	if (concat) {
		ifstream bfile;
		bfile.open(path, ios::in | ios::binary);
		bfile.read((char*)& num, 4);
	}
	else return -1;

	GLuint ctex;
	glGenTextures(1, &ctex);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, ctex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	open_thumb(result + "_" + std::to_string(num - 2) + ".file", ctex);

	return ctex;
}

void Clip::open_clipfiles() {
	// order elements
	bool cont = mainprogram->order_paths(false);
	if (!cont) return;

	std::string str = mainprogram->paths[mainprogram->clipfilescount];
	int pos = std::find(mainprogram->clipfileslay->clips.begin(), mainprogram->clipfileslay->clips.end(), mainprogram->clipfilesclip) - mainprogram->clipfileslay->clips.begin();
	if (pos == mainprogram->clipfileslay->clips.size() - 1) {
		Clip* clip = new Clip;
		if (mainprogram->clipfileslay->clips.size() > 4) mainprogram->clipfileslay->queuescroll++;
		mainprogram->clipfilesclip = clip;
		mainprogram->clipfileslay->clips.insert(mainprogram->clipfileslay->clips.end() - 1, clip);
	}
	mainprogram->clipfilesclip->path = str;

	if (isimage(str)) {
		mainprogram->clipfilesclip->tex = get_imagetex(str);
		mainprogram->clipfilesclip->type = ELEM_IMAGE;
		mainprogram->clipfilesclip->get_imageframes();
	}
	else if (str.substr(str.length() - 6, std::string::npos) == ".layer") {
		mainprogram->clipfilesclip->tex = get_layertex(str);
		mainprogram->clipfilesclip->type = ELEM_LAYER;
		mainprogram->clipfilesclip->get_layerframes();
	}
	else {
		mainprogram->clipfilesclip->tex = get_videotex(str);
		mainprogram->clipfilesclip->type = ELEM_FILE;
		mainprogram->clipfilesclip->get_videoframes();
	}
	mainprogram->clipfilesclip->tex = copy_tex(mainprogram->clipfilesclip->tex);
	mainprogram->clipfilescount++;
	mainprogram->clipfilesclip = mainprogram->clipfileslay->clips[pos + 1];
	if (mainprogram->clipfilescount == mainprogram->paths.size()) {
		mainprogram->clipfileslay->cliploading = false;
		mainprogram->openclipfiles = false;
		mainprogram->paths.clear();
		mainprogram->multistage = 0;
	}
}


Preferences::Preferences() {
	PIVid *pvi = new PIVid;
	pvi->box = new Box;
	pvi->box->tooltiptitle = "Video settings ";
	pvi->box->tooltip = "Left click to set video related preferences ";
	this->items.push_back(pvi);
	PIInt *pii = new PIInt;
	pii->box = new Box;
	pii->box->tooltiptitle = "Interface settings ";
	pii->box->tooltip = "Left click to set interface related preferences ";
	this->items.push_back(pii);
	PIProg *pip = new PIProg;
	pip->box = new Box;
	pip->box->tooltiptitle = "Program settings ";
	pip->box->tooltip = "Left click to set program related preferences ";
	this->items.push_back(pip);
	PIDirs *pidirs = new PIDirs;
	pidirs->box = new Box;
	pidirs->box->tooltiptitle = "Directory settings ";
	pidirs->box->tooltip = "Left click to set default directories ";
	this->items.push_back(pidirs);
	PIMidi *pimidi = new PIMidi;
	pimidi->populate();
	pimidi->box = new Box;
	pimidi->box->tooltiptitle = "MIDI device settings ";
	pimidi->box->tooltip = "Left click to set MIDI device related preferences ";
	this->items.push_back(pimidi);
	for (int i = 0; i < this->items.size(); i++) {
		PrefCat *item = this->items[i];
		item->box->vtxcoords->x1 = -1.0f;
		item->box->vtxcoords->y1 = 1.0f - (i + 1) * 0.2f;
		item->box->vtxcoords->w = 0.5f;
		item->box->vtxcoords->h = 0.2f;
	}
}

void Preferences::load() {
	ifstream rfile;
	#ifdef _WIN64
	std::string prstr = mainprogram->docpath + "preferences.prefs";
	#else
	#ifdef __linux__
	std::string homedir (getenv("HOME"));
	std::string prstr = homedir + "/.ewocvj2/preferences.prefs";
	#endif
	#endif
	if (!exists(prstr)) return;
	rfile.open(prstr);
	std::string istring;
	getline(rfile, istring);
	while (getline(rfile, istring)) {
		if (istring == "ENDOFFILE") break;
		
		else if (istring == "PREFCAT") {
			while (getline(rfile, istring)) {
				if (istring == "ENDOFPREFCAT") break;
				bool brk = false;
				for (int i = 0; i < mainprogram->prefs->items.size(); i++) {
					if (mainprogram->prefs->items[i]->name == istring) {
						std::string catname = istring;
						if (istring == "MIDI Devices") ((PIMidi*)(mainprogram->prefs->items[i]))->populate();
						while (getline(rfile, istring)) {
							if (istring == "ENDOFPREFCAT") {
								brk = true;
								break;
							}
							int foundpos = -1;
							PrefItem *pi = nullptr;
							for (int j = 0; j < mainprogram->prefs->items[i]->items.size(); j++) {
								pi = mainprogram->prefs->items[i]->items[j];
								if (pi->name == istring and pi->connected) {
									foundpos = j;
									getline(rfile, istring);
									if (pi->type == PREF_ONOFF) {
										pi->onoff = std::stoi(istring);
										if (pi->dest) *(bool*)pi->dest = pi->onoff;
									}
									else if (pi->type == PREF_NUMBER) {
										pi->value = std::stoi(istring);
										if (pi->dest) *(float*)pi->dest = pi->value;
									}
									else if (pi->type == PREF_PATH) {
										boost::filesystem::path p(istring);
										if (!boost::filesystem::exists(p)) {
											pi->path = *(std::string*)pi->dest;
											continue;
										}
										pi->path = istring;
										std::string lastchar = pi->path.substr(pi->path.length() - 1, std::string::npos);
										if (lastchar != "/" and lastchar != "\\") pi->path += "/";
										if (pi->dest) *(std::string*)pi->dest = pi->path;
									}
									break;
								}
							}
							if (catname == "MIDI Devices") {
								if (foundpos == -1) {
									std::string name = istring;
									getline(rfile, istring);
									bool onoff = std::stoi(istring);
									PrefItem *pmi = new PrefItem(mainprogram->prefs->items[i], mainprogram->prefs->items[i]->items.size(), name, PREF_ONOFF, nullptr);
									mainprogram->prefs->items[i]->items.push_back(pmi);
									pmi->onoff = onoff;
									pmi->connected = false;
								}
								else {
									PIMidi *pim = (PIMidi*)mainprogram->prefs->items[i];
									if (!pi->onoff) {
										if (std::find(pim->onnames.begin(), pim->onnames.end(), pi->name) != pim->onnames.end()) {
											pim->onnames.erase(std::find(pim->onnames.begin(), pim->onnames.end(), pi->name));
											mainprogram->openports.erase(std::find(mainprogram->openports.begin(), mainprogram->openports.end(), foundpos));
											pi->midiin->cancelCallback();
											delete pi->midiin;
										}
									}
									else {
										pim->onnames.push_back(pi->name);
										RtMidiIn *midiin = new RtMidiIn();
										if (std::find(mainprogram->openports.begin(), mainprogram->openports.end(), foundpos) == mainprogram->openports.end()) {
											midiin->openPort(foundpos);
											midiin->setCallback(&mycallback, (void*)pim->items[foundpos]);
											mainprogram->openports.push_back(foundpos);
										}
										pi->midiin = midiin;
									}
								}
							}
						}
						if (brk) break;
					}
				}
				if (brk) break;
			}
		}
		if (istring == "CURRFILESDIR") {
			getline(rfile, istring);
			boost::filesystem::path p(istring);
			if (boost::filesystem::exists(p)) mainprogram->currfilesdir = istring;
		}
		else if (istring == "CURRCLIPFILESDIR") {
			getline(rfile, istring);
			boost::filesystem::path p(istring);
			if (boost::filesystem::exists(p)) mainprogram->currclipfilesdir = istring;
		}
		else if (istring == "CURRSHELFFILESDIR") {
			getline(rfile, istring);
			boost::filesystem::path p(istring);
			if (boost::filesystem::exists(p)) mainprogram->currshelffilesdir = istring;
		}
		else if (istring == "CURRBINFILESDIR") {
			getline(rfile, istring);
			boost::filesystem::path p(istring);
			if (boost::filesystem::exists(p)) mainprogram->currbinfilesdir = istring;
		}
		else if (istring == "CURRSTATEDIR") {
			getline(rfile, istring);
			boost::filesystem::path p(istring);
			if (boost::filesystem::exists(p)) mainprogram->currstatedir = istring;
		}
		else if (istring == "CURRSHELFFILESDIR") {
			getline(rfile, istring);
			boost::filesystem::path p(istring);
			if (boost::filesystem::exists(p)) mainprogram->currshelffilesdir = istring;
		}
	}
	
	mainprogram->set_ow3oh3();
	rfile.close();
}

void Preferences::save() {
	ofstream wfile;
	#ifdef _WIN64
	std::string prstr = mainprogram->docpath + "preferences.prefs";
	#else
	#ifdef __linux__
	std::string homedir (getenv("HOME"));
	std::string prstr = homedir + "/.ewocvj2/preferences.prefs";
	#endif
	#endif
	wfile.open(prstr);
	wfile << "EWOCvj Preferences V0.2\n";
	
	for (int i = 0; i < mainprogram->prefs->items.size(); i++) {
		PrefCat *pc = mainprogram->prefs->items[i];
		wfile << "PREFCAT\n";
		wfile << pc->name;
		wfile << "\n";
		for (int j = 0; j < pc->items.size(); j++) {
			wfile << pc->items[j]->name;
			wfile << "\n";
			if (pc->items[j]->type == PREF_ONOFF) {
				wfile << std::to_string(pc->items[j]->onoff);
				wfile << "\n";
			}
			else if (pc->items[j]->type == PREF_NUMBER) {
				wfile << std::to_string(pc->items[j]->value);
				wfile << "\n";
			}
			else if (pc->items[j]->type == PREF_PATH) {
				wfile << pc->items[j]->path;
				wfile << "\n";
			}
		}
		wfile << "ENDOFPREFCAT\n";
	}

	wfile << "CURRFILESDIR\n";
	wfile << mainprogram->currfilesdir;
	wfile << "\n";
	wfile << "CURRCLIPFILESDIR\n";
	wfile << mainprogram->currclipfilesdir;
	wfile << "\n";
	wfile << "CURRSHELFFILESDIR\n";
	wfile << mainprogram->currshelffilesdir;
	wfile << "\n";
	wfile << "CURRBINFILESDIR\n";
	wfile << mainprogram->currshelffilesdir;
	wfile << "\n";
	wfile << "CURRSTATEDIR\n";
	wfile << mainprogram->currstatedir;
	wfile << "\n";

	wfile << "ENDOFFILE\n";
	wfile.close();
}


PrefItem::PrefItem(PrefCat *cat, int pos, std::string name, PREF_TYPE type, void *dest) {
	this->name = name;
	this->type = type;
	this->dest = dest;
	this->namebox = new Box;
	this->namebox->vtxcoords->x1 = -0.5f;
	this->namebox->vtxcoords->y1 = 1.0f - (pos + 1) * 0.2f;
	this->namebox->vtxcoords->w = 1.5f;
	if (type == PREF_PATH) this->namebox->vtxcoords->w = 0.6f;
	this->namebox->vtxcoords->h = 0.2f;
	this->namebox->upvtxtoscr();
	this->valuebox = new Box;
	if (type == PREF_ONOFF) {
		this->valuebox->vtxcoords->x1 = -0.425f;
		this->valuebox->vtxcoords->y1 = 1.05f - (pos + 1) * 0.2f;
		this->valuebox->vtxcoords->w = 0.1f;
		this->valuebox->vtxcoords->h = 0.1f;
	}
	else if (type == PREF_NUMBER) {	
		this->valuebox->vtxcoords->x1 = 0.25f;
		this->valuebox->vtxcoords->y1 = 1.0f - (pos + 1) * 0.2f;
		this->valuebox->vtxcoords->w = 0.3f;
		this->valuebox->vtxcoords->h = 0.2f;
	}
	else if (type == PREF_PATH) {	
		this->valuebox->vtxcoords->x1 = 0.1f;
		this->valuebox->vtxcoords->y1 = 1.0f - (pos + 1) * 0.2f;
		this->valuebox->vtxcoords->w = 0.8f;
		this->valuebox->vtxcoords->h = 0.2f;
		this->iconbox = new Box;
		this->iconbox->vtxcoords->x1 = 0.9f;
		this->iconbox->vtxcoords->y1 = 1.0f - (pos + 1) * 0.2f;
		this->iconbox->vtxcoords->w = 0.1f;
		this->iconbox->vtxcoords->h = 0.2f;
		this->iconbox->upvtxtoscr();
	}
	this->valuebox->upvtxtoscr();
}

PIDirs::PIDirs() {
	PrefItem *pdi;
	int pos = 0;
	this->name = "Directories";
	
	pdi = new PrefItem(this, pos, "Projects", PREF_PATH, (void*)&mainprogram->projdir);
	pdi->namebox->tooltiptitle = "Projects directory ";
	pdi->namebox->tooltip = "Default directory where new projects will be created in. ";
	pdi->valuebox->tooltiptitle = "Set projects directory ";
	pdi->valuebox->tooltip = "Leftclick starts keyboard entry of location of projects directory. ";
	pdi->iconbox->tooltiptitle = "Browse to set projects directory ";
	pdi->iconbox->tooltip = "Leftclick allows browsing for location of projects directory. ";
	#ifdef _WIN64
	pdi->path = mainprogram->docpath + "projects/";
	#else
	#ifdef __linux__
	pdi->path = mainprogram->homedir + "/.ewocvj2/projects/";
	#endif
	#endif
	mainprogram->projdir = pdi->path;
	this->items.push_back(pdi);
	pos++;
	
	pdi = new PrefItem(this, pos, "Autosaves", PREF_PATH, (void*)&mainprogram->autosavedir);
	pdi->namebox->tooltiptitle = "Autosaves directory ";
	pdi->namebox->tooltip = "Project state autosaves are saved in this directory. ";
	pdi->valuebox->tooltiptitle = "Set autosaves directory ";
	pdi->valuebox->tooltip = "Leftclick starts keyboard entry of location of autosaves directory. ";
	pdi->iconbox->tooltiptitle = "Browse to set autosaves directory ";
	pdi->iconbox->tooltip = "Leftclick allows browsing for location of autosaves directory. ";
	#ifdef _WIN64
	pdi->path = mainprogram->docpath + "autosaves/";
	#else
	#ifdef __linux__
	pdi->path = mainprogram->homedir + "/.ewocvj2/autosaves/";
	#endif
	#endif
	mainprogram->autosavedir = pdi->path;
	this->items.push_back(pdi);
	pos++;
}

PIMidi::PIMidi() {
	this->name = "MIDI Devices";
}

void PIMidi::populate() {
	std::vector<PrefItem*> ncitems;
	for (int i = 0; i < this->items.size(); i++) {
		if (!this->items[i]->connected) ncitems.push_back(this->items[i]);
	}
	RtMidiIn midiin;
	int nPorts = midiin.getPortCount();
	std::string portName;
	std::vector<PrefItem*> intrmitems;
	std::vector<PrefItem*> itemsleft = this->items;
	for (int i = 0; i < nPorts; i++) {
		std::string nm = midiin.getPortName(i);
		PrefItem *pmi = new PrefItem(this, i, nm, PREF_ONOFF, nullptr);
		pmi->onoff = (std::find(this->onnames.begin(), this->onnames.end(), pmi->name) != this->onnames.end());
		pmi->connected = true;
		pmi->namebox->tooltiptitle = "MIDI device ";
		pmi->namebox->tooltip = "Name of a connected MIDI device. ";
		pmi->valuebox->tooltiptitle = "MIDI device on/off ";
		pmi->valuebox->tooltip = "Leftclicking toggles if this MIDI device is used by EWOCvj2. ";
		for (int j = 0; j < itemsleft.size(); j++) {
			if (itemsleft[j]->name == pmi->name) {
				// already opened ports, just assign
				pmi->midiin = itemsleft[j]->midiin;
				break;
			}
		}
		std::vector<PrefItem*> itemslefttemp = itemsleft;
		while (itemslefttemp.size()) {
			if (itemslefttemp.back()->name == pmi->name) {
				// items already in list
				// erase from itemsleft to allow for multiple same names  reminder: test!
				itemsleft.erase(itemsleft.begin() + itemslefttemp.size() - 1);
			}
			itemslefttemp.pop_back();
		}
		intrmitems.push_back(pmi);
	}
	for (int i = 0; i < this->items.size(); i++) {
		//if (this->items[i]->connected) delete this->items[i];
	}
	this->items = intrmitems;
	this->items.insert(this->items.end(), ncitems.begin(), ncitems.end());
}

PIInt::PIInt() {
	this->name = "Interface";
	PrefItem *pii;
	int pos = 0;
	
	pii = new PrefItem(this, pos, "Select needs click", PREF_ONOFF, (void*)&mainprogram->needsclick);
	pii->onoff = 0;
	pii->namebox->tooltiptitle = "Select needs click ";
	pii->namebox->tooltip = "Toggles if layer selection needs a click or not. ";
	pii->valuebox->tooltiptitle = "Select needs click ";
	pii->valuebox->tooltip = "Toggles if layer selection needs a click or not. ";
	mainprogram->needsclick = pii->onoff;
	this->items.push_back(pii);
	pos++;

	pii = new PrefItem(this, pos, "Show tooltips", PREF_ONOFF, (void*)& mainprogram->showtooltips);
	pii->onoff = 1;
	pii->namebox->tooltiptitle = "Show tooltips toggle ";
	pii->namebox->tooltip = "Toggles if tooltips will be shown when hovering mouse over an interface element. ";
	pii->valuebox->tooltiptitle = "Show tooltips toggle ";
	pii->valuebox->tooltip = "Toggles if tooltips will be shown when hovering mouse over an interface element. ";
	mainprogram->showtooltips = pii->onoff;
	this->items.push_back(pii);
	pos++;

	pii = new PrefItem(this, pos, "Long tooltips", PREF_ONOFF, (void*)& mainprogram->longtooltips);
	pii->onoff = 1;
	pii->namebox->tooltiptitle = "Long tooltips toggle ";
	pii->namebox->tooltip = "Toggles if tooltips will be long, if off only tooltip titles will be shown. ";
	pii->valuebox->tooltiptitle = "Long tooltips toggle ";
	pii->valuebox->tooltip = "Toggles if tooltips will be long, if off only tooltip titles will be shown. ";
	mainprogram->showtooltips = pii->onoff;
	this->items.push_back(pii);
	pos++;
}

PIVid::PIVid() {
	this->name = "Video";
	PrefItem *pvi;
	int pos = 0;
	
	pvi = new PrefItem(this, pos, "Output video width", PREF_NUMBER, (void*)&mainprogram->ow);
	pvi->value = 1920;
	pvi->namebox->tooltiptitle = "Output video width ";
	pvi->namebox->tooltip = "Sets the width in pixels of the video stream sent to the output. ";
	pvi->valuebox->tooltiptitle = "Output video width ";
	pvi->valuebox->tooltip = "Leftclicking the value allows setting the width in pixels of the video stream sent to the output. ";
	mainprogram->ow = pvi->value;
	this->items.push_back(pvi);
	pos++;

	pvi = new PrefItem(this, pos, "Output video height", PREF_NUMBER, (void*)&mainprogram->oh);
	pvi->value = 1080;
	pvi->namebox->tooltiptitle = "Output video height ";
	pvi->namebox->tooltip = "Sets the height in pixels of the video stream sent to the output. ";
	pvi->valuebox->tooltiptitle = "Output video height ";
	pvi->valuebox->tooltip = "Leftclicking the value allows setting the height in pixels of the video stream sent to the output. ";
	mainprogram->oh = pvi->value;
	this->items.push_back(pvi);
	pos++;

	pvi = new PrefItem(this, pos, "Highly compressed video quality", PREF_NUMBER, (void*)&mainprogram->qualfr);
	pvi->value = 3;
	pvi->namebox->tooltiptitle = "Video playback quality of highly compressed streams ";
	pvi->namebox->tooltip = "Sets the quality of the video stream playback for streams that don't have one keyframe per frame encoded. A tradeoff between quality and framerate. ";
	pvi->valuebox->tooltiptitle = "Video playback quality of highly compressed streams ";
	pvi->valuebox->tooltip = "Leftclicking the value allows setting the quality in the range of 1 to 10.  Lower is better quality and worse framerate/choppier playback. ";
	mainprogram->qualfr = pvi->value;
	this->items.push_back(pvi);
	pos++;
}

PIProg::PIProg() {
	this->name = "Program";
	PrefItem *ppi;
	int pos = 0;
	
	ppi = new PrefItem(this, pos, "Autosave", PREF_ONOFF, (void*)&mainprogram->autosave);
	ppi->onoff = 1;
	ppi->namebox->tooltiptitle = "Autosave ";
	ppi->namebox->tooltip = "Saves project states at specified intervals in specified directory. ";
	ppi->valuebox->tooltiptitle = "Autosave toggle ";
	ppi->valuebox->tooltip = "Leftclick to turn on/off autosave functionality. ";
	mainprogram->autosave = ppi->onoff;
	this->items.push_back(ppi);
	pos++;
	
	ppi = new PrefItem(this, pos, "Autosave interval (minutes)", PREF_NUMBER, (void*)&mainprogram->asminutes);
	ppi->value = 1;
	ppi->namebox->tooltiptitle = "Autosave interval ";
	ppi->namebox->tooltip = "Sets the time interval between successive autosaves. ";
	ppi->valuebox->tooltiptitle = "Set autosave interval ";
	ppi->valuebox->tooltip = "Leftclicking the value allows typing the autosave interval as number of minutes. ";
	mainprogram->asminutes = ppi->value;
	this->items.push_back(ppi);
	pos++;
}


void handle_scenes(Scene* scene) {
	// Draw scene boxes
	float white[] = { 1.0, 1.0, 1.0, 1.0 };
	float red[] = { 1.0, 0.5, 0.5, 1.0 };
	bool comp = !mainprogram->prevmodus;
	for (int i = 3; i > -1; i--) {
		Box* box = mainmix->scenes[comp][scene->deck][i]->box;
		if (i == mainmix->currscene[comp][scene->deck]) {
			box->acolor[0] = 1.0f;
			box->acolor[1] = 0.5f;
			box->acolor[2] = 0.5f;
			box->acolor[3] = 1.0f;
		}
		else {
			box->acolor[0] = 0.0f;
			box->acolor[1] = 0.0f;
			box->acolor[2] = 0.0f;
			box->acolor[3] = 1.0f;
		}
		draw_box(box, -1);
	}
	// Handle sceneboxes
	for (int i = 0; i < 4; i++) {
		Box* box = mainmix->scenes[comp][scene->deck][i]->box;
		Button* but = mainmix->scenes[comp][scene->deck][i]->button;
		box->acolor[0] = 0.0;
		box->acolor[1] = 0.0;
		box->acolor[2] = 0.0;
		box->acolor[3] = 1.0;
		if (box->in() or but->value) {
			if (mainprogram->menuactivation and !mainprogram->menuondisplay) {
				mainprogram->parammenu3->state = 2;
				mainmix->learnparam = nullptr;
				mainmix->learnbutton = but;
				mainprogram->menuactivation = false;
			}
			else {
				if (but != mainprogram->onscenebutton) {
					mainprogram->onscenebutton = but;
					mainprogram->onscenemilli = 0;
				}
				if (((mainprogram->onscenemilli > 6000 or mainprogram->leftmouse) and !mainmix->moving and !mainprogram->menuondisplay) or but->value) {
					but->value = false;
					// switch scenes
					Scene* si = mainmix->scenes[comp][scene->deck][i];
					if (i == mainmix->currscene[comp][scene->deck]) continue;
					si->tempnblayers.clear();
					si->tempnbframes.clear();
					for (int j = 0; j < si->nblayers.size(); j++) {
						si->tempnblayers.push_back(si->nblayers[j]);
						if (!si->loaded) {
							si->tempnbframes.push_back(si->nbframes[j]);
						}
					}
					std::vector<Layer*>& lvec = choose_layers(scene->deck);
					scene->nblayers.clear();
					scene->nbframes.clear();
					for (int j = 0; j < lvec.size(); j++) {
						// store layers and frames of current scene into nblayers for running their framecounters in the background (to keep sync)
						if (lvec[j]->filename == "") continue;
						scene->nblayers.push_back(lvec[j]);
						scene->nbframes.push_back(lvec[j]->frame);
					}
					mainmix->mousedeck = scene->deck;
					// save current scene to temp dir, open new scene
					mainmix->do_save_deck(mainprogram->temppath + "tempdeck_xch.deck", true, true);
					SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
					// stop current scene loopstation line if they don't extend to the other deck
					for (int j = 0; j < loopstation->elems.size(); j++) {
						std::unordered_set<Param*>::iterator it;
						std::vector<Param*> tobeerased;
						for (it = loopstation->elems[j]->params.begin(); it != loopstation->elems[j]->params.end(); it++) {
							Param* par = *it;
							if (par->effect) {
								if (std::find(lvec.begin(), lvec.end(), par->effect->layer) != lvec.end()) {
									tobeerased.push_back(par);
								}
							}
						}
						for (int k = 0; k < tobeerased.size(); k++) {
							loopstation->elems[j]->params.erase(tobeerased[k]);
						}
						if (loopstation->elems[j]->params.size() == 0) {
							loopstation->elems[j]->erase_elem();
						}
					}
					mainmix->open_deck(mainprogram->temppath + "tempdeck_" + std::to_string(comp) + std::to_string(scene->deck) + std::to_string(i) + ".deck", false);
					boost::filesystem::rename(mainprogram->temppath + "tempdeck_xch.deck", mainprogram->temppath + "tempdeck_" + std::to_string(comp) + std::to_string(scene->deck) + std::to_string(mainmix->currscene[comp][scene->deck]) + ".deck");
					std::vector<Layer*>& lvec2 = choose_layers(scene->deck);
					for (int j = 0; j < lvec2.size(); j++) {
						if (lvec2[j]->filename == "") continue;
						// set layer values to (running in background) values of loaded scene
						if (si->loaded) lvec2[j]->frame = si->nbframes[j];
						else {
							lvec2[j]->frame = si->tempnbframes[j];
							lvec2[j]->startframe = si->tempnblayers[j]->startframe;
							lvec2[j]->endframe = si->tempnblayers[j]->endframe;
							lvec2[j]->layerfilepath = si->tempnblayers[j]->layerfilepath;
							lvec2[j]->filename = si->tempnblayers[j]->filename;
							lvec2[j]->type = si->tempnblayers[j]->type;
							lvec2[j]->oldalive = si->tempnblayers[j]->oldalive;
						}
					}
					mainmix->currscene[comp][scene->deck] = i;
					si->loaded = false;

					mainmix->bulrs[scene->deck].clear();
					for (int j = 0; j < mainmix->butexes[scene->deck].size(); j++) {
						glDeleteTextures(1, &mainmix->butexes[scene->deck][j]);
					}
				}
			}
			box->acolor[0] = 0.5;
			box->acolor[1] = 0.5;
			box->acolor[2] = 1.0;
			box->acolor[3] = 1.0;
			if (mainprogram->menuactivation) {
				mainprogram->deckmenu->state = 2;
				mainmix->learnbutton = but;
				mainmix->mousedeck = scene->deck;
				mainprogram->menuactivation = false;
			}
		}

		std::string s = std::to_string(i + 1);
		std::string pchar;
		pchar = s;
		if (mainmix->learnbutton == but and mainmix->learn) pchar = "M";
		render_text(pchar, white, box->vtxcoords->x1 + 0.01f, box->vtxcoords->y1 + 0.025f, 0.0006f, 0.001f);
	}
	std::string pchar;
	if (scene->deck == 0) pchar = "A";
	else if (scene->deck == 1) pchar = "B";
	render_text(pchar, red, mainmix->decknamebox[scene->deck]->vtxcoords->x1 + 0.01f, mainmix->decknamebox[scene->deck]->vtxcoords->y1 + 0.025f, 0.0006f, 0.001f);
}

std::vector<Effect*>& Layer::choose_effects() {
	if (mainprogram->effcat[this->deck]->value) {
		return this->effects[1];
	}
	else {
		return this->effects[0];
	}
}

std::vector<Layer*>& choose_layers(bool j) {
	if (mainprogram->prevmodus) {
		if (j == 0) return mainmix->layersA;
		else return mainmix->layersB;
	}
	else {
		if (j == 0) return mainmix->layersAcomp;
		else return mainmix->layersBcomp;
	}
}


	
void do_text_input(float x, float y, float sx, float sy, int mx, int my, float width, int smflag, PrefItem* item) {
	do_text_input(x, y, sx, sy, mx, my, width, smflag, item, false);
}

void do_text_input(float x, float y, float sx, float sy, int mx, int my, float width, int smflag, PrefItem * item, bool directdraw) {
		// handle display and mouse selection of keyboard input
	float white[] = { 1.0f, 1.0f, 1.0f, 1.0f };

	float textw;
	std::vector<float> textwvec;
	std::vector<float> totvec = render_text(mainprogram->inputtext, white, x, y, sx, sy, smflag, 0, 0);
	float cps = textwvec_total(totvec);
	if (mainprogram->cursorpixels == -1) {
		// initialize: cursor at end, shift string if bigger than space
		mainprogram->cursorpixels = mainprogram->xvtxtoscr(cps);
		mainprogram->startcursor = 0;
		mainprogram->endcursor = totvec.size();
		float total2 = 0.0f;
		for (int j = 0; j < totvec.size(); j++) {
			total2 += mainprogram->xvtxtoscr(totvec[j]);
			if (total2 + width > mainprogram->cursorpixels) {
				mainprogram->startcursor = j + 1;
				std::string part = mainprogram->inputtext.substr(mainprogram->startcursor, mainprogram->inputtext.length() - mainprogram->startcursor);
				std::vector<float> parttextvec = render_text(part, white, x, y, sx, sy, smflag, 0, 0);
				float parttextw = textwvec_total(parttextvec);
				mainprogram->startpos = mainprogram->xvtxtoscr(cps - parttextw);
				break;
			}
		}
	}
	float distin = 0.0f;
	if (mainprogram->leftmousedown or mainprogram->leftmouse) {
		bool found = false;
		for (int j = 0; j < totvec.size(); j++) {
			if (my < mainprogram->yvtxtoscr(1.0f - (y - 0.003f)) and my > mainprogram->yvtxtoscr(1.0f - (y - 0.003f + mainprogram->texth))) {
				if (mx > mainprogram->xvtxtoscr(x + 1.0f + distin) - mainprogram->startpos and mx < mainprogram->xvtxtoscr(x + 1.0f + distin + (j > 0) * totvec[j - 1 + (j == 0)] / 2.0f + totvec[j] / 2.0f) - mainprogram->startpos) {
					// click or drag inside path moves edit cursor
					found = true;
					if (mainprogram->leftmousedown) {
						if (!mainprogram->cursorreset) {
							if (mainprogram->cursorpos1 == -1) {
								mainprogram->cursorpos1 = j;
								mainprogram->cursorpos2 = j;
	
							}
							else {
								mainprogram->cursorpos2 = j;
							}
						}
						else {
							if (mainprogram->cursortemp1 == -1) {
								mainprogram->cursortemp1 = j;
								mainprogram->cursortemp2 = j;
							}
							else mainprogram->cursortemp2 = j;
							if (mainprogram->cursortemp1 != mainprogram->cursortemp2) {
								mainprogram->cursorpos1 = mainprogram->cursortemp1;
								mainprogram->cursorpos2 = mainprogram->cursortemp2;
								mainprogram->cursorreset = false;
							}
						}
					}
					if (mainprogram->leftmouse) {
						mainprogram->cursorpos0 = j;
						if (mainprogram->cursorreset) {
							mainprogram->cursorpos1 = -1;
							mainprogram->cursorpos2 = -1;
						}
						mainprogram->cursorreset = true;
						mainprogram->cursortemp1 = -1;
						mainprogram->cursortemp2 = -1;
					}
					mainprogram->leftmouse = false;
					break;
				}
			}
			if (j > 0) distin += totvec[j - 1] / 2.0f;
			distin += totvec[j] / 2.0f;
		}
		if (found == false) {
			//when clicked outside path, cancel edit
			mainmix->adaptnumparam = nullptr;
			if (item) item->renaming = false;
			mainprogram->renaming = EDIT_NONE;
			end_input();
		}
	}
	if (mainprogram->renaming != EDIT_NONE) {
		std::string part = mainprogram->inputtext.substr(0, mainprogram->cursorpos0);
		textwvec = render_text(part, white, x, y, sx, sy, smflag, 0, 0);
		textw = textwvec_total(textwvec);
		mainprogram->cursorpixels = mainprogram->xvtxtoscr(textw);
		if (mainprogram->cursorpixels < mainprogram->startpos) {
			// cursor goes beyond left space border
			float total1 = 0.0f;
			for (int i = 0; i < totvec.size(); i++) {
				if (total1 > mainprogram->cursorpixels) {
					mainprogram->startpos = mainprogram->cursorpixels;
					mainprogram->startcursor = i - 1;
					break;
				}
				total1 += mainprogram->xvtxtoscr(totvec[i]);
			}
			float total2 = 0.0f;
			for (int i = 0; i < totvec.size(); i++) {
				total2 += mainprogram->xvtxtoscr(totvec[i]);
				if (total2 > mainprogram->startpos + width) {
					mainprogram->endcursor = i + 1;
					break;
				}
			}
		}
		else if (mainprogram->cursorpixels - mainprogram->startpos > width) {
			// cursor goes beyond right space border
			float total1 = 0.0f;
			for (int i = 0; i < totvec.size(); i++) {
				total1 += mainprogram->xvtxtoscr(totvec[i]);
				if (total1 > mainprogram->startpos + width) {
					mainprogram->endcursor = i + 1;
					float total2 = 0.0f;
					for (int j = 0; j < totvec.size(); j++) {
						total2 += mainprogram->xvtxtoscr(totvec[j]);
						if (total2 + width > mainprogram->cursorpixels) {
							mainprogram->startcursor = j + 1;
							part = mainprogram->inputtext.substr(mainprogram->startcursor, mainprogram->inputtext.length() - mainprogram->startcursor);
							std::vector<float> parttextvec = render_text(part, white, x, y, sx, sy, smflag, 0, 0);
							float parttextw = textwvec_total(parttextvec);
							mainprogram->startpos = mainprogram->xvtxtoscr(cps - parttextw);
							break;
						}
					}
					break;
				}
			}
		}
		if (mainprogram->xvtxtoscr(cps) < width) {
			mainprogram->startcursor = 0;
			mainprogram->endcursor = mainprogram->inputtext.length();
			mainprogram->startpos = 0;
		}
		mainprogram->startcursor = std::clamp(mainprogram->startcursor, 0, (int)mainprogram->inputtext.length());
		mainprogram->endcursor = std::clamp(mainprogram->endcursor, 0, (int)mainprogram->inputtext.length());
		part = mainprogram->inputtext.substr(mainprogram->startcursor, mainprogram->endcursor - mainprogram->startcursor);
		render_text(part, white, x, y, sx, sy, smflag, 0);
		if (mainprogram->cursorpos1 == -1) {
			// draw cursor line
			register_line_draw(white, x + textw - mainprogram->xscrtovtx(mainprogram->startpos), y - 0.010f + (smflag > 0) * 0.03f, x + textw - mainprogram->xscrtovtx(mainprogram->startpos), y + (mainprogram->texth / 2.3f) / (2070.0f / glob->h) + (smflag > 0) * 0.03f, directdraw);
		}
		else {
			// draw cursor block
			int c1 = mainprogram->cursorpos1;
			int c2 = mainprogram->cursorpos2;
			if (c2 < c1) {
				// when selecting from right to left
				std::swap(c1, c2);
			}
			std::string part = mainprogram->inputtext.substr(mainprogram->startcursor, c1 - mainprogram->startcursor);
			textwvec = render_text(part, white, x, y, sx, sy, smflag, 0, 0);
			float textw1 = textwvec_total(textwvec);
			part = mainprogram->inputtext.substr(c1, c2 - c1);
			textwvec = render_text(part, white, x + textw1, y, sx, sy, smflag, 0, 0);
			float textw2 = textwvec_total(textwvec);
			glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);
			draw_box(white, white, x + textw1, y - 0.010f, textw2, 0.010f + mainprogram->texth / (2070.0f / glob->h), -1);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
	}
}
	



void enddrag() {
	if (mainprogram->dragbinel) {
		//if (mainprogram->dragbinel->path.rfind, ".layer", nullptr != std::string::npos) {
		//	if (mainprogram->dragbinel->path.find("cliptemp_") != std::string::npos) {
		//		boost::filesystem::remove(mainprogram->dragbinel->path);			
		//	}
		//}
		if (mainprogram->shelfdragelem) {
			// when dragged inside shelf, shelfdragelem is deleted before enddrag() is called
			if (mainprogram->shelves[0]->prevnum != -1) {
				std::swap(mainprogram->shelves[0]->elements[mainprogram->shelves[0]->prevnum]->tex, mainprogram->shelves[0]->elements[mainprogram->shelves[0]->prevnum]->oldtex);
			}
			else if (mainprogram->shelves[1]->prevnum != -1) {
				std::swap(mainprogram->shelves[1]->elements[mainprogram->shelves[1]->prevnum]->tex, mainprogram->shelves[1]->elements[mainprogram->shelves[1]->prevnum]->oldtex);
			}
			//std::swap(elem->tex, mainprogram->shelfdragelem->tex);
			mainprogram->shelfdragelem->tex = mainprogram->dragbinel->tex;
			mainprogram->shelfdragelem = nullptr;
		}
		mainprogram->shelfdragelem = nullptr;

		mainprogram->dragbinel = nullptr;
		if (mainprogram->draglay) mainprogram->draglay->vidmoving = false;
		mainprogram->draglay = nullptr;
		mainprogram->dragclip = nullptr;
		mainprogram->dragpath = "";
		mainmix->moving = false;
		mainprogram->dragout[0] = true;
		mainprogram->dragout[1] = true;
		//glDeleteTextures(1, &binsmain->dragtex);  maybe needs implementing in one case, check history
	}

	if (0) {
		bool temp = binsmain->currbinel->full;
		binsmain->currbinel->full = binsmain->movingbinel->full;
		binsmain->movingbinel->full = temp;
		binsmain->currbinel->tex = binsmain->movingbinel->oldtex;
		binsmain->movingbinel->tex = binsmain->movingtex;
		binsmain->currbinel = nullptr;
		binsmain->movingbinel = nullptr;
		binsmain->movingtex = -1;
	}

	mainprogram->dragmousedown = false;
}

void end_input() {
	SDL_StopTextInput();
	mainprogram->cursorpos0 = -1;
	mainprogram->cursorpos1 = -1;
	mainprogram->cursorpos2 = -1;
	mainprogram->cursorpixels = -1;
}

void Layer::mute_handle() {
	// change node connections to accommodate for a mute layer
	if (this->mutebut->value) {
		if (this->pos > 0) {
			if (this->blendnode->in) {
				this->blendnode->in->out.clear();
				mainprogram->nodesmain->currpage->connect_nodes(this->blendnode->in, this->lasteffnode[1]->out[0]);
			}
		}
		else if (this->next() != this) this->next()->blendnode->in = nullptr;
		this->lasteffnode[1]->out.clear();
	}
	else {
		Layer *templay = this;
		while (1) {
			if (templay == templay->prev()) break;
			if ((templay->prev()->pos == 0) and !templay->prev()->mutebut->value) {
				//this->prev()->lasteffnode[1] = this->prev()->lasteffnode[0];
				this->prev()->lasteffnode[1]->out.clear();
				mainprogram->nodesmain->currpage->connect_nodes(this->prev()->lasteffnode[1], this->blendnode);
				break;
			}
			if (!templay->prev()->mutebut->value) {
				this->prev()->lasteffnode[1]->out.clear();
				mainprogram->nodesmain->currpage->connect_nodes(this->prev()->lasteffnode[1], this->blendnode);
				break;
			}
			templay = templay->prev();
		}
		this->lasteffnode[1]->out.clear();
		templay = this;
		while (1) {
			if (templay == templay->next()) {
				std::vector<Layer*> &lvec = choose_layers(this->deck);
				if (&lvec == &mainmix->layersA) {
					mainprogram->nodesmain->currpage->connect_nodes(this->lasteffnode[1], mainprogram->nodesmain->mixnodes[0]);
				}
				else if (&lvec == &mainmix->layersB) {
					mainprogram->nodesmain->currpage->connect_nodes(this->lasteffnode[1], mainprogram->nodesmain->mixnodes[1]);
				}
				else if (&lvec == &mainmix->layersAcomp) {
					mainprogram->nodesmain->currpage->connect_nodes(this->lasteffnode[1], mainprogram->nodesmain->mixnodescomp[0]);
				}
				else if (&lvec == &mainmix->layersBcomp) {
					mainprogram->nodesmain->currpage->connect_nodes(this->lasteffnode[1], mainprogram->nodesmain->mixnodescomp[1]);
				}
				break;
			}
			else if (!templay->next()->mutebut->value) {
				mainprogram->nodesmain->currpage->connect_nodes(this->lasteffnode[1], templay->next()->blendnode);
				break;
			}
			templay = templay->next();
		}
	}
}
	

void the_loop() {
	float white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	float halfwhite[] = { 1.0f, 1.0f, 1.0f, 0.5f };
	float black[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float deepred[4] = { 1.0, 0.0, 0.0, 1.0 };
	float red[] = { 1.0f, 0.5f, 0.5f, 1.0f };
	float green[] = { 0.0f, 0.75f, 0.0f, 1.0f };
	float orange[] = { 1.0f, 0.5f, 0.0f, 1.0f };
	float blue[] = { 0.5f, 0.5f, 1.0f, 1.0f };
	float grey[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	float darkgrey[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	float lightblue[] = { 0.5f, 0.5f, 1.0f, 1.0f };

	glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
	mainprogram->drawbuffer = mainprogram->globfbo;
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR) {
		std::cerr << "OpenGL error3: " << err << std::endl;
	}

	// prepare gathering of box data
	mainprogram->bdvptr[0] = mainprogram->bdcoords[0];
	mainprogram->bdtcptr[0] = mainprogram->bdtexcoords[0];
	mainprogram->bdcptr[0] = mainprogram->bdcolors[0];
	mainprogram->bdtptr[0] = mainprogram->bdtexes[0];
	mainprogram->bdtnptr[0] = mainprogram->boxtexes[0];
	mainprogram->countingtexes[0] = 0;
	mainprogram->currbatch = 0;
	mainprogram->boxz = 0.0f;
	mainprogram->guielems.clear();

	if (!mainprogram->binsscreen) {
		// draw background graphic
		draw_direct(nullptr, black, -1.0f, -1.0f, 2.0f, 2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, mainprogram->bgtex, glob->w, glob->h, false);
	}

	if (mainprogram->blocking) {
		// when waiting for some activity spread out per loop
		mainprogram->mx = -1;
		mainprogram->my = -1;
	}
	if (mainprogram->oldmy <= mainprogram->yvtxtoscr(tf(0.05f)) and mainprogram->oldmx > glob->w - mainprogram->xvtxtoscr(0.05f)) {
		// for exit while midi learn
		mainprogram->eXit = true;
	}
	else mainprogram->eXit = false;
	if (mainmix->learn) {
		// when waiting MIDI learn, can't click anywhere else
		mainprogram->mx = -1;
		mainprogram->my = -1;
	}

	if (mainprogram->leftmouse and (mainprogram->cwon or mainprogram->menuondisplay or mainprogram->wiping or mainmix->adaptparam or mainmix->scrollon or binsmain->dragbin or mainmix->moving or mainprogram->dragbinel or mainprogram->drageff or mainprogram->dragclip or mainprogram->shelfdragelem)) {
		// special cases when mouse can be released over element that should not be triggered
		mainprogram->lmover = true;
		mainprogram->leftmouse = false;
	}
	else mainprogram->lmover = false;
	if (SDL_GetMouseFocus() != mainprogram->mainwindow) {
		// when in prefs or config_midipresets_init windows
		mainprogram->mx = -1;
		mainprogram->my = -1;
		//mainprogram->leftmouse = false;
		mainprogram->menuactivation = false;
	}

	if (mainmix->adaptparam) {
		// no hovering while adapting param
		mainprogram->my = -1;
	}
	mainprogram->iemy = mainprogram->my;  // allow Insert effect click on border of parameter box
	
	// set mouse button values to enable blocking when item ordering overlay is on (reminder|maybe not the best solution)
	if (mainprogram->orderondisplay) {
		mainprogram->orderleftmouse = mainprogram->leftmouse;
		mainprogram->orderleftmousedown = mainprogram->leftmousedown;
		mainprogram->leftmousedown = false;
		mainprogram->leftmouse = false;
		mainprogram->menuactivation = false;
	}

	// set active loopstation
	if (mainprogram->prevmodus) {
		loopstation = lp;
	}
	else {
		loopstation = lpc;
	}
	
	// check if general video resolution changed, and reinitialize all textures concerned
	mainprogram->handle_changed_owoh();
	
	// calculate and visualize fps
	mainmix->fps[mainmix->fpscount] = (int)(1.0f / (mainmix->time - mainmix->oldtime));
	int total = 0;
	for (int i = 0; i < 25; i++) total += mainmix->fps[i];
	mainmix->rate = total / 25;
	mainmix->fpscount++;
	if (mainmix->fpscount > 24) mainmix->fpscount = 0;
	std::string s = std::to_string(mainmix->rate);
	render_text(s, white, 0.01f, 0.47f, 0.0006f, 0.001f);
	//printf("frrate %s\n", s.c_str());


	// keep advancing frame counters for non-displayed scenes (alive = 0)
	for (int m = 0; m < 2; m++) {
		for (int j = 0; j < 2; j++) {
			for (int i = 0; i < 4; i++) {
				for (int k = 0; k < mainmix->scenes[m][j][i]->nblayers.size(); k++) {
					if (i == mainmix->currscene[m][j]) break;
					mainmix->scenes[m][j][i]->nblayers[k]->calc_texture(0, 0);
					mainmix->scenes[m][j][i]->nbframes[k] = mainmix->scenes[m][j][i]->nblayers[k]->frame;
				}
			}
		}
	}
	mainmix->firstlayers.clear();
	if (!mainprogram->prevmodus) {
		//when in performance mode: keep advancing frame counters for preview layer stacks (alive = 0)
		for(int i = 0; i < mainmix->layersA.size(); i++) {
			Layer *testlay = mainmix->layersA[i];
			testlay->calc_texture(0, 0);
			testlay->load_frame();
		}
		for(int i = 0; i < mainmix->layersB.size(); i++) {
			Layer *testlay = mainmix->layersB[i];
			testlay->calc_texture(0, 0);
			testlay->load_frame();
		}
		// performance mode frame calc and load
		for(int i = 0; i < mainmix->layersAcomp.size(); i++) {
			Layer *testlay = mainmix->layersAcomp[i];
			testlay->calc_texture(1, 1);
			testlay->load_frame();
		}
		for(int i = 0; i < mainmix->layersBcomp.size(); i++) {
			Layer *testlay = mainmix->layersBcomp[i];
			testlay->calc_texture(1, 1);
			testlay->load_frame();
		}
		for(int i = 0; i < mainprogram->busylayers.size(); i++) {
			Layer *testlay = mainprogram->busylayers[i];  // needs to be revised, reminder: dont remember why, something with doubling...?
			testlay->calc_texture(1, 1);
			testlay->load_frame();
		}
	}
	// when in preview modus, do frame texture load for both mixes (preview and output)
	if (mainprogram->prevmodus) {
		for(int i = 0; i < mainmix->layersA.size(); i++) {
			Layer *testlay = mainmix->layersA[i];
			testlay->calc_texture(0, 1);
			testlay->load_frame();
		}
		for(int i = 0; i < mainmix->layersB.size(); i++) {
			Layer *testlay = mainmix->layersB[i];
			testlay->calc_texture(0, 1);
			testlay->load_frame();
		}
	}
	if (mainprogram->prevmodus) {
		for(int i = 0; i < mainmix->layersAcomp.size(); i++) {
			Layer *testlay = mainmix->layersAcomp[i];
			testlay->calc_texture(1, 1);
			testlay->load_frame();
		}
		for(int i = 0; i < mainmix->layersBcomp.size(); i++) {
			Layer *testlay = mainmix->layersBcomp[i];
			testlay->calc_texture(1, 1);
			testlay->load_frame();
		}
	}
		

	// Crawl web
	make_layboxes();
	if (mainprogram->prevmodus) walk_nodes(0);
	walk_nodes(1);
	

	// draw and handle wormgates
	if (!mainprogram->binsscreen) mainprogram->handle_wormhole(0);
	mainprogram->handle_wormhole(1);
	if (mainprogram->dragbinel) {
		if (!mainprogram->wormhole1->box->in() and !mainprogram->wormhole2->box->in()) {
			mainprogram->inwormhole = false;
		}
	}
	
	for (int i = 0; i < mainprogram->menulist.size(); i++) {
		// set menuondisplay: is there a menu active (state > 1)?
		if (mainprogram->menulist[i]->state > 1) {
			mainprogram->leftmousedown = false;
			mainprogram->menuondisplay = true;
			break;
		}
		else {
			mainprogram->menuondisplay = false;
		}
	}


	/////////////// STUFF THAT BELONGS TO EITHER BINS OR MIX OR FULL SCREEN

	if (mainprogram->binsscreen) {
		// big one this: this if decides if code for bins or mix screen is executed
		// the following statement is defined in bins.cpp
		// the 'true' value triggers the full version of this function: it draws the screen also
		// 'false' does a dummy run, used to rightmouse cancel things initiated in code (not the mouse)
		binsmain->handle(true);
	}
	else if (mainprogram->fullscreen > -1) {
		// part of the mix is showed fullscreen, so no bins or mix specifics self-understandingly
		mainprogram->handle_fullscreen();
	}

	// Handle node view someda
	//else if (mainmix->mode == 1) {
	//	mainprogram->nodesmain->currpage->handle_nodes();
	//}

	else {  // this is MIX screen specific stuff
		if (mainmix->learn and mainprogram->rightmouse) {
			// MIDI learn cancel
			mainmix->learn = false;
			mainprogram->rightmouse = false;
		}

		// reminder: what to do with color wheel
		if (mainprogram->cwon) {
			if (mainprogram->leftmousedown) {
				mainprogram->cwmouse = 1;
				mainprogram->leftmousedown = false;
			}
		}

		// Handle parameter adaptation
		if (mainmix->adaptparam) {
			mainmix->handle_adaptparam();
		}

		//blue bars designating layer selection
		float blue[4] = { 0.1f, 0.1f, 0.6f, 0.5f };
		if (mainmix->currlay) {
			int pos = mainmix->currlay->pos - mainmix->scenes[mainmix->currlay->comp][mainmix->currlay->deck][mainmix->currscene[mainmix->currlay->comp][mainmix->currlay->deck]]->scrollpos;
			if (pos >= 0 and pos <= 2) {
				draw_box(nullptr, blue, -1.0f + mainprogram->numw + mainmix->currlay->deck * 1.0f + pos * mainprogram->layw, 0.1f + tf(0.05f), mainprogram->layw, 0.9f, -1);
			}
		}

		// Draw and handle crossfade->box
		Param* par;
		if (mainprogram->prevmodus) par = mainmix->crossfade;
		else par = mainmix->crossfadecomp;
		par->handle();


		// draw and handle layer stacks, effect stacks and params
		if (mainprogram->prevmodus) {
			// when previewing
			for (int i = 0; i < mainmix->layersA.size(); i++) {
				Layer* testlay = mainmix->layersA[i];
				testlay->display();
			}
			for (int i = 0; i < mainmix->layersB.size(); i++) {
				Layer* testlay = mainmix->layersB[i];
				testlay->display();
			}
		}
		else {
			// when performing
			for (int i = 0; i < mainmix->layersAcomp.size(); i++) {
				Layer* testlay = mainmix->layersAcomp[i];
				testlay->display();
			}
			for (int i = 0; i < mainmix->layersBcomp.size(); i++) {
				Layer* testlay = mainmix->layersBcomp[i];
				testlay->display();
			}
		}


		// display the deck monitors and output monitors on the bottom of the screen
		display_mix();


		// handle scenes
		bool comp = !mainprogram->prevmodus;
		handle_scenes(mainmix->scenes[comp][0][mainmix->currscene[comp][0]]);
		handle_scenes(mainmix->scenes[comp][1][mainmix->currscene[comp][1]]);


		// draw and handle overall genmidi button
		mainmix->handle_genmidibuttons();


		mainprogram->preview_modus_buttons();


		//draw and handle global deck speed sliders
		par = mainmix->deckspeed[!mainprogram->prevmodus][0];
		draw_box(white, darkgrey, mainmix->deckspeed[!mainprogram->prevmodus][0]->box->vtxcoords->x1, mainmix->deckspeed[!mainprogram->prevmodus][0]->box->vtxcoords->y1, mainmix->deckspeed[!mainprogram->prevmodus][0]->box->vtxcoords->w * 0.30f, 0.1f, -1);
		par->handle();
		par = mainmix->deckspeed[!mainprogram->prevmodus][1];
		draw_box(white, darkgrey, mainmix->deckspeed[!mainprogram->prevmodus][1]->box->vtxcoords->x1, mainmix->deckspeed[!mainprogram->prevmodus][1]->box->vtxcoords->y1, mainmix->deckspeed[!mainprogram->prevmodus][1]->box->vtxcoords->w * 0.30f, 0.1f, -1);
		par->handle();

		//draw and handle recbut
		mainmix->recbut->handle(1, 0);
		if (mainmix->recbut->toggled()) {
			if (!mainmix->recording) {
				// start recording
				mainmix->start_recording();
			}
			else {
				mainmix->recording = false;
			}
		}
		float radx = mainmix->recbut->box->vtxcoords->w / 2.0f;
		float rady = mainmix->recbut->box->vtxcoords->h / 2.0f;



		mainprogram->layerstack_scrollbar_handle();


		//handle dragging into layerstack
		if (mainprogram->dragbinel) {
			if (mainprogram->dragbinel->type != ELEM_DECK and mainprogram->dragbinel->type != ELEM_MIX) {
				if (mainprogram->prevmodus) {
					drag_into_layerstack(mainmix->layersA, 0);
					drag_into_layerstack(mainmix->layersB, 1);
				}
				else {
					drag_into_layerstack(mainmix->layersAcomp, 0);
					drag_into_layerstack(mainmix->layersBcomp, 1);
				}
			}
		}


		// draw and handle loopstation
		if (mainprogram->prevmodus) {
			lp->handle();
			for (int i = 0; i < lpc->elems.size(); i++) {
				if (lpc->elems[i]->loopbut->value or lpc->elems[i]->playbut->value) lpc->elems[i]->set_values();
			}
		}
		else {
			lpc->handle();
		}

		//handle shelves
		mainprogram->inshelf = -1;
		mainprogram->shelves[0]->handle();
		mainprogram->shelves[1]->handle();


		// handle the layer clips queue
		mainmix->clips_handle();










		// draw "layer insert into stack" blue boxes
		if (!mainprogram->menuondisplay and mainprogram->dragbinel) {
			for (int j = 0; j < 2; j++) {
				std::vector<Layer*>& lvec = choose_layers(j);
				for (int i = 0; i < lvec.size(); i++) {
					Layer* lay = lvec[i];
					bool comp = !mainprogram->prevmodus;
					if (lay->pos < mainmix->scenes[comp][j][mainmix->currscene[comp][j]]->scrollpos or lay->pos > mainmix->scenes[comp][j][mainmix->currscene[comp][j]]->scrollpos + 2) continue;
					Box* box = lay->node->vidbox;
					float thick = mainprogram->xvtxtoscr(0.075f);
					if (box->scrcoords->y1 - box->scrcoords->h < mainprogram->my and mainprogram->my < box->scrcoords->y1) {
						if (box->scrcoords->x1 - thick + (i - mainmix->scenes[comp][j][mainmix->currscene[comp][j]]->scrollpos == 0) * thick < mainprogram->mx and mainprogram->mx < box->scrcoords->x1 + thick) {
							// this block handles the first boxes, just not the last
							mainprogram->leftmousedown = false;
							float blue[] = { 0.5, 0.5, 1.0, 1.0 };
							// draw broad blue boxes when inserting layers
							draw_box(blue, blue, box->vtxcoords->x1 - mainprogram->xscrtovtx(thick) + (i - mainmix->scenes[comp][j][mainmix->currscene[comp][j]]->scrollpos == 0) * mainprogram->xscrtovtx(thick), box->vtxcoords->y1, mainprogram->xscrtovtx(thick * (2.0f - (i - mainmix->scenes[comp][j][mainmix->currscene[comp][j]]->scrollpos == 0))), mainprogram->layh, -1);
						}
						else if (box->scrcoords->x1 + box->scrcoords->w - thick < mainprogram->mx and mainprogram->mx < box->scrcoords->x1 + box->scrcoords->w) {
							mainprogram->leftmousedown = false;
							if (lay->pos == lvec.size() - 1 or lay->pos == mainmix->scenes[comp][j][mainmix->currscene[comp][j]]->scrollpos + 2) {
								// this block handles the last box
								float blue[] = { 0.5, 0.5 , 1.0, 1.0 };
								// draw broad blue boxes when inserting layers
								draw_box(blue, blue, box->vtxcoords->x1 + box->vtxcoords->w - mainprogram->xscrtovtx(thick), box->vtxcoords->y1, mainprogram->xscrtovtx(thick * (1.0f + (i - mainmix->scenes[comp][j][mainmix->currscene[comp][j]]->scrollpos != 2))), mainprogram->layh, -1);
							}
						}
					}
				}
			}
		}		
		
		
		mainmix->vidbox_handle();

		mainmix->outputmonitors_handle();

		mainmix->layerdrag_handle();

	}	

		
		
	if (mainprogram->rightmouse) {
		if (mainprogram->dragclip) {
			// cancel clipdragging
			mainprogram->draglay->clips.insert(mainprogram->draglay->clips.begin() + mainprogram->dragpos, mainprogram->dragclip);
			mainprogram->dragclip = nullptr;
			mainprogram->draglay = nullptr;
			mainprogram->dragpos = -1;
			enddrag();
			mainprogram->rightmouse = false;
		}
	}
				
	mainprogram->handle_mixenginemenu();

	mainprogram->handle_effectmenu();

	mainprogram->handle_parammenu1();

	mainprogram->handle_parammenu2();

	mainprogram->handle_parammenu3();

	mainprogram->handle_parammenu4();

	mainprogram->handle_loopmenu();

	mainprogram->handle_mixtargetmenu();

	mainprogram->handle_wipemenu();

	mainprogram->handle_deckmenu();

	mainprogram->handle_laymenu1();

	mainprogram->handle_newlaymenu();

	mainprogram->handle_clipmenu();

	mainprogram->handle_mainmenu();

	mainprogram->handle_shelfmenu();

	mainprogram->handle_filemenu();

	mainprogram->handle_editmenu();

	if (mainprogram->menuactivation == true) {
		// main menu triggered
		mainprogram->mainmenu->state = 2;
		mainprogram->menuactivation = false;
	}
	
	if (mainprogram->lmover) {
		// left mouse outside menu cancels all menus
		mainprogram->menuondisplay = false;
		if (mainprogram->binselmenu->state > 1) {
			// reset selection of bin elements to false when nothing chosen
			for (int i = 0; i < 12; i++) {
				for (int j = 0; j < 12; j++) {
					binsmain->currbin->elements[i * 12 + j]->select = false;
				}
			}
		}
		for (int i = 0; i < mainprogram->menulist.size(); i++) {
			mainprogram->menulist[i]->state = 0;
		}
		
	}
	
	if (mainprogram->dragbinel) {
		// draw texture of element being dragged
		float boxwidth = tf(0.2f);
		float nmx = mainprogram->xscrtovtx(mainprogram->mx) + boxwidth / 2.0f;
		float nmy = 2.0 - mainprogram->yscrtovtx(mainprogram->my) - boxwidth / 2.0f;
		draw_box(nullptr, black, -1.0f - 0.5 * boxwidth + nmx, -1.0f - 0.5 * boxwidth + nmy, boxwidth, boxwidth, mainprogram->dragbinel->tex);

		mainmix->deckmixdrag_handle();
	}

	if (mainprogram->shelfdragelem) {
		if (mainprogram->lmover) {
			// delete dragged shelf element when dropped outside shelf
			ShelfElement* elem = mainprogram->shelfdragelem;
			elem->path = "";
			elem->jpegpath = "";
			elem->type = ELEM_FILE;
			blacken(elem->tex);
			blacken(elem->oldtex);
		}
	}


	mainprogram->shelf_miditriggering();


	// multi opening of files, handle one per loop
	if (mainprogram->openfileslayers) {
		// load one item from mainprogram->paths into layer and clips, one each loop not to slowdown output stream
		mainprogram->fileslay->open_files_layers();
	}
	if (mainprogram->openfilesqueue) {
		// load one item from mainprogram->paths into layer and clips, one each loop not to slowdown output stream
		mainprogram->fileslay->open_files_queue();
	}

	if (mainprogram->openclipfiles) {
		// load one item from mainprogram->paths into clips, one each loop not to slowdown output stream
		mainmix->mouseclip->open_clipfiles();
	}

	if (mainprogram->openshelffiles) {
		// load one item from mainprogram->paths into shelf, one each loop not to slowdown output stream
		mainmix->mouseshelf->open_shelffiles();
	}

	if (binsmain->importbins) {
		// load one item from mainprogram->paths into binslist, one each loop not to slowdown output stream
		binsmain->import_bins();
	}

	// implementation of a basic menu when mouse at top of screen
	if (mainprogram->my == 0 and !mainprogram->transforming) {
		mainprogram->intopmenu = true;
	}
	if (mainprogram->intopmenu) {
		float lc[] = { 0.0, 0.0, 0.0, 1.0 };
		float ac1[] = { 0.3, 0.3, 0.3, 1.0 };
		float ac2[] = { 0.6, 0.6, 0.6, 1.0 };
		float deepred[] = { 1.0, 0.0, 0.0, 1.0 };
		draw_box(lc, ac1, -1.0f, 1.0f - tf(0.05f), 0.156f, tf(0.05f), -1);
		draw_box(lc, ac1, -1.0f + 0.156f, 1.0f - tf(0.05f), 0.156f, tf(0.05f), -1);
		draw_box(lc, ac2, -1.0f + 0.312f, 1.0f - tf(0.05f), 2.0f - 0.312f, tf(0.05f), -1);
		draw_box(nullptr, deepred, 1.0f - 0.05f, 1.0f - tf(0.05f), 0.05f, tf(0.05f), -1);
		render_text("x", white, 0.966f, 1.019f - tf(0.05f), 0.0012f, 0.002f);
		render_text("FILE", white, -1.0f + tf(0.0078f), 1.0f - tf(0.05f) + tf(0.015f), tf(0.0003f), tf(0.0005f));
		render_text("CONFIGURE", white, -1.0f + 0.156f + tf(0.0078f), 1.0f - tf(0.05f) + tf(0.015f), tf(0.0003f), tf(0.0005f));
		if (mainprogram->my > mainprogram->yvtxtoscr(tf(0.05f))) {
			mainprogram->intopmenu = false;
		}
		else if (mainprogram->mx < mainprogram->xvtxtoscr(0.156f)) {
			if (mainprogram->leftmouse) {
				mainprogram->filemenu->menux = 0;
				mainprogram->filemenu->menuy = mainprogram->yvtxtoscr(tf(0.05f));
				mainprogram->filedomenu->menux = 0;
				mainprogram->filedomenu->menuy = mainprogram->yvtxtoscr(tf(0.05f));
				mainprogram->laylistmenu1->menux = 0;
				mainprogram->laylistmenu1->menuy = mainprogram->yvtxtoscr(tf(0.05f));
				mainprogram->laylistmenu2->menux = 0;
				mainprogram->laylistmenu2->menuy = mainprogram->yvtxtoscr(tf(0.05f));
				mainprogram->filemenu->state = 2;
			}
		}
		else if (mainprogram->mx < mainprogram->xvtxtoscr(0.312f)) {
			if (mainprogram->leftmouse) {
				mainprogram->editmenu->menux = mainprogram->xvtxtoscr(0.156f);
				mainprogram->editmenu->menuy = mainprogram->yvtxtoscr(tf(0.05f));
				mainprogram->editmenu->state = 2;
			}
		}
		else if (mainprogram->mx > glob->w - mainprogram->xvtxtoscr(0.05f)) {
			if (mainprogram->leftmouse or mainprogram->orderleftmouse) {
				mainprogram->quitting = "closed window";
			}
		}
	}

	if (mainmix->learn) {
		draw_box(red, blue, -0.3f, -0.0f, 0.6f, 0.3f, -1);
		if (mainmix->midishelfstage == 1) {
			render_text("Awaiting start point MIDI input.", white, -0.15f, 0.2f, 0.001f, 0.0016f);
			render_text("Rightclick cancels.", white, -0.1f, 0.06f, 0.001f, 0.0016f);
		
			// reminder: delete midi info for multiple buttons (shelf)
		}
		else if (mainmix->midishelfstage == 2) {
			render_text("Awaiting end point MIDI input.", white, -0.16f, 0.2f, 0.001f, 0.0016f);
			render_text("Rightclick cancels.", white, -0.1f, 0.06f, 0.001f, 0.0016f);
		}
		else {
			render_text("Awaiting MIDI input.", white, -0.1f, 0.2f, 0.001f, 0.0016f);
			render_text("Rightclick cancels.", white, -0.1f, 0.06f, 0.001f, 0.0016f);
		}
		// allow exiting with x icon during MIDI learn
		draw_box(nullptr, deepred, 1.0f - 0.05f, 1.0f - tf(0.05f), 0.05f, tf(0.05f), -1);
		render_text("x", white, 0.966f, 1.019f - tf(0.05f), 0.0012f, 0.002f);
		if (mainprogram->eXit) {
			if (mainprogram->leftmouse) {
				mainprogram->quitting = "closed window";
			}
		}
	}
	
	if ((mainprogram->lmover and mainprogram->dragbinel)) {
		enddrag();
	}

	if (!mainprogram->prefon and !mainprogram->midipresets and mainprogram->tooltipbox and mainprogram->quitting == "") {
		if (mainprogram->tooltipbox->tooltip != "") mainprogram->tooltips_handle(0);
	}
	
	if (!mainprogram->binsscreen) {
		// leftmouse outside clip queue cancels clip queue visualisation
		std::vector<Layer*>& lvec1 = choose_layers(0);
		if (mainprogram->leftmouse and !mainprogram->menuondisplay) {
			set_queueing(false);
		}
	}

	//autosave
	if (mainprogram->autosave and mainmix->time > mainprogram->astimestamp + mainprogram->asminutes * 60) {
		mainprogram->astimestamp = (int)mainmix->time;
		
		std::string name;
		if (mainprogram->autosavelist.size()) {
			name = remove_extension(mainprogram->autosavelist.back());
		}
		else {
			name = mainprogram->autosavedir + "autosave_" + remove_extension(basename(mainprogram->project->path)) + "_0";
		}
		int num = std::stoi(name.substr(name.rfind('_') + 1, name.length() - name.rfind('_') - 1));
		name = remove_version(name) + "_" + std::to_string(num + 1);
		std::string path = name + ".state";

		mainmix->do_save_state(path, true);

		mainprogram->autosavelist.push_back(path);
		if (mainprogram->autosavelist.size() > 20) mainprogram->autosavelist.erase(mainprogram->autosavelist.begin());
		std::ofstream wfile;
		wfile.open(mainprogram->autosavedir + "autosavelist");
		wfile << "EWOCvj AUTOSAVELIST V0.1\n";
		for (int i = 0; i < mainprogram->autosavelist.size(); i++) {
			wfile << mainprogram->autosavelist[i];
			wfile << "\n";
		}
		wfile << "ENDOFFILE\n";
		wfile.close();
	}
	
	// sync with output views
	for (int i = 0; i < mainprogram->outputentries.size(); i++) {
		EWindow *win = mainprogram->outputentries[i]->win;
		win->syncnow = true;
		while (win->syncnow) {
			win->sync.notify_one();
		}
		std::unique_lock<std::mutex> lock(win->syncendmutex);
		win->syncend.wait(lock, [&]{return win->syncendnow;});
		win->syncendnow = false;
		lock.unlock();
	}
	
	bool dmbu = mainprogram->directmode;
	bool prret = false;
	GLuint tex, fbo;
	if (mainprogram->quitting != "") {
		mainprogram->directmode = true;
		SDL_ShowWindow(mainprogram->prefwindow);
		SDL_RaiseWindow(mainprogram->prefwindow);
		SDL_GL_MakeCurrent(mainprogram->prefwindow, glc_pr);
		glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->smglobfbo_pr);
		mainprogram->drawbuffer = mainprogram->smglobfbo_pr;
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glViewport(0, 0, glob->w / 2.0f, glob->h / 2.0f);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
		int ret = mainprogram->quit_requester();
		if (ret == 1 or ret == 2) {
			//save midi map
			//save_genmidis(mainprogram->docpath + "midiset.gm");
			//empty temp dir
			for (int j = 0; j < 12; j++) {
				for (int i = 0; i < 12; i++) {
					// cancel hap encoding for all elements
					Box* box = binsmain->elemboxes[i * 12 + j];
					box->upvtxtoscr();
					BinElement* binel = binsmain->currbin->elements[i * 12 + j];
					binel->encoding = false; // delete the hap file under construction
				}
			}
			while (mainprogram->encthreads) {}

			mainprogram->prefs->save();

			boost::filesystem::path path_to_remove(mainprogram->temppath);
			for (boost::filesystem::directory_iterator end_dir_it, it(path_to_remove); it != end_dir_it; ++it) {
				boost::filesystem::remove(it->path());
			}

			printf("%s: %s\n", mainprogram->quitting.c_str(), SDL_GetError());
			printf("stopped\n");

			SDL_Quit();
			exit(1);
		}
		if (ret == 3) {
			mainprogram->quitting = "";
			SDL_HideWindow(mainprogram->prefwindow);
			SDL_RaiseWindow(mainprogram->mainwindow);
		}
		if (ret == 0) {
			glBlitNamedFramebuffer(mainprogram->smglobfbo_pr, 0, 0, 0, smw, smh, 0, 0, smw, smh, GL_COLOR_BUFFER_BIT, GL_NEAREST);
			SDL_GL_SwapWindow(mainprogram->prefwindow);
		}
		SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
	}



	mainprogram->preferences();

	mainprogram->config_midipresets_init();




	mainprogram->directmode = dmbu;


	glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
	mainprogram->drawbuffer = mainprogram->globfbo;
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	if (!mainprogram->directmode) {

		glEnable(GL_DEPTH_TEST);
		glClearDepth(1.0f);
		glClear(GL_DEPTH_BUFFER_BIT);
		glDepthFunc(GL_LESS);

		glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
		mainprogram->drawbuffer = mainprogram->globfbo;
		glDrawBuffer(GL_COLOR_ATTACHMENT0);

		GLuint textmode = glGetUniformLocation(mainprogram->ShaderProgram, "textmode");
		int bs[2048];
		std::iota(bs, bs + mainprogram->maxtexes - 2, 0);
		GLint boxSampler = glGetUniformLocation(mainprogram->ShaderProgram, "boxSampler");
		glUniform1iv(boxSampler, mainprogram->maxtexes - 2, bs);
		GLint glbox = glGetUniformLocation(mainprogram->ShaderProgram, "glbox");
		glActiveTexture(GL_TEXTURE0 + mainprogram->maxtexes - 2);
		glBindTexture(GL_TEXTURE_BUFFER, mainprogram->bdcoltex);
		glActiveTexture(GL_TEXTURE0 + mainprogram->maxtexes - 1);
		glBindTexture(GL_TEXTURE_BUFFER, mainprogram->bdtextex);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mainprogram->bdibo);
		glBindVertexArray(mainprogram->bdvao);
		glDisable(GL_BLEND);
		for (int i = mainprogram->currbatch + ((int)mainprogram->bdtptr[mainprogram->currbatch] - (int)mainprogram->bdtexes[mainprogram->currbatch] > 0) - 1; i >= 0; i--) {
			int numquads = (int)mainprogram->bdtptr[i] - (int)mainprogram->bdtexes[i];
			int pos = 0;
			for (int j = 0; j < numquads; j++) {
				if (mainprogram->boxtexes[i][j] != -1) {
					glActiveTexture(GL_TEXTURE0 + pos++);
					glBindTexture(GL_TEXTURE_2D, mainprogram->boxtexes[i][j]);
				}
			}
			glBindBuffer(GL_TEXTURE_BUFFER, mainprogram->boxcoltbo);
			glBufferSubData(GL_TEXTURE_BUFFER, 0, numquads * 4, mainprogram->bdcolors[i]);
			glBindBuffer(GL_TEXTURE_BUFFER, mainprogram->boxtextbo);
			glBufferSubData(GL_TEXTURE_BUFFER, 0, numquads, mainprogram->bdtexes[i]);

			glBindBuffer(GL_ARRAY_BUFFER, mainprogram->bdvbo);
			glBufferSubData(GL_ARRAY_BUFFER, 0, numquads * 4 * 3 * sizeof(float), mainprogram->bdcoords[i]);
			glBindBuffer(GL_ARRAY_BUFFER, mainprogram->bdtcbo);
			glBufferSubData(GL_ARRAY_BUFFER, 0, numquads * 4 * 2 * sizeof(float), mainprogram->bdtexcoords[i]);

			pos = numquads * 6 - 1;
			for (int j = 0; j < numquads * 4; j += 4) {
				mainprogram->indices[pos--] = j;
				mainprogram->indices[pos--] = j + 1;
				mainprogram->indices[pos--] = j + 2;
				mainprogram->indices[pos--] = j + 2;
				mainprogram->indices[pos--] = j + 1;
				mainprogram->indices[pos--] = j + 3;
			}
			glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, numquads * 6 * sizeof(unsigned short), mainprogram->indices);

			glUniform1i(glbox, 1);
			glDrawElements(GL_TRIANGLES, numquads * 6, GL_UNSIGNED_SHORT, (GLvoid*)0);
			glUniform1i(glbox, 0);
		}
		glEnable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);

		// draw frontbatch one by one: lines, triangles, menus, dragtex
		mainprogram->directmode = true;
		for (int i = 0; i < mainprogram->guielems.size(); i++) {
			GUI_Element* elem = mainprogram->guielems[i];
			if (elem->type == GUI_LINE) draw_line(elem->line);
			else if (elem->type == GUI_TRIANGLE) {
				draw_triangle(elem->triangle);
			}
			else {
				if (!elem->box->circle and elem->box->text) glUniform1i(textmode, 1);
				draw_box(elem->box->linec, elem->box->areac, elem->box->x, elem->box->y, elem->box->wi, elem->box->he, 0.0f, 0.0f, 1.0f, 1.0f, elem->box->circle, elem->box->tex, glob->w, glob->h, elem->box->text, elem->box->vertical);
				if (!elem->box->circle and elem->box->text) glUniform1i(textmode, 0);
			}
		}
		mainprogram->directmode = false;
	}

	Layer* lay = mainmix->currlay;

	// Handle colorbox
	mainprogram->pick_color(lay, lay->colorbox);
	if (lay->cwon) {
		lay->blendnode->chred = lay->rgb[0];
		lay->blendnode->chgreen = lay->rgb[1];
		lay->blendnode->chblue = lay->rgb[2];
	}

	glBlitNamedFramebuffer(mainprogram->globfbo, 0, 0, 0, glob->w, glob->h , 0, 0, glob->w, glob->h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	
	SDL_GL_SwapWindow(mainprogram->mainwindow);
	glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
	mainprogram->drawbuffer = mainprogram->globfbo;
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	
	mainprogram->ttreserved = false;
	mainprogram->leftmouse = 0;
	mainprogram->orderleftmouse = 0;
	mainprogram->doubleleftmouse = 0;
	mainprogram->middlemouse = 0;
	mainprogram->rightmouse = 0;
}


GLuint copy_tex(GLuint tex) {
	return copy_tex(tex, mainprogram->ow3, mainprogram->oh3, 0);
}

GLuint copy_tex(GLuint tex, bool yflip) {
	return copy_tex(tex, mainprogram->ow3, mainprogram->oh3, yflip);
}

GLuint copy_tex(GLuint tex, int tw, int th) {
	return copy_tex(tex, tw, th, 0);
}

GLuint copy_tex(GLuint tex, int tw, int th, bool yflip) {
	GLuint smalltex;
	glGenTextures(1, &smalltex);
	glBindTexture(GL_TEXTURE_2D, smalltex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, tw, th);
	GLuint dfbo;
	glGenFramebuffers(1, &dfbo);
	glBindFramebuffer(GL_FRAMEBUFFER, dfbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, smalltex, 0);
	mainprogram->drawbuffer = dfbo;
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glViewport(0, 0, tw, th);
	int sw, sh;
	glBindTexture(GL_TEXTURE_2D, tex);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &sw);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &sh);
	float black[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	if (yflip) {
		draw_box(nullptr, black, -1.0f, -1.0f, 2.0f, 2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, tex, glob->w, glob->h, false);
	}
	else {
		draw_box(nullptr, black, -1.0f, -1.0f + 2.0f, 2.0f, -2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, tex, glob->w, glob->h, false);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
	mainprogram->drawbuffer = mainprogram->globfbo;
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glViewport(0, 0, glob->w, glob->h);
	glDeleteFramebuffers(1, &dfbo);
	return smalltex;
}

void save_thumb(std::string path, GLuint tex) {
	int wi = mainprogram->ow3;
	int he = mainprogram->oh3;
	char *buf = (char*)malloc(wi * he * 3);

	GLuint texfrbuf, endfrbuf;
	glGenFramebuffers(1, &texfrbuf);
	glBindFramebuffer(GL_FRAMEBUFFER, texfrbuf);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
	GLuint smalltex;
	glGenTextures(1, &smalltex);
	glBindTexture(GL_TEXTURE_2D, smalltex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, wi, he);
	glGenFramebuffers(1, &endfrbuf);
	glBindFramebuffer(GL_FRAMEBUFFER, endfrbuf);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, smalltex, 0);
	int sw, sh;
	glBindTexture(GL_TEXTURE_2D, tex);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &sw);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &sh);
	glBlitNamedFramebuffer(texfrbuf, endfrbuf, 0, 0, sw, sh, 0, 0, wi, he, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glReadPixels(0, 0, wi, he, GL_RGB, GL_UNSIGNED_BYTE, buf);
	glDeleteTextures(1, &smalltex);
	
	FILE* outfile = fopen(path.c_str(), "wb");
	
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr       jerr;
	 
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, outfile);
	 
	cinfo.image_width      = wi;
	cinfo.image_height     = he;
	cinfo.input_components = 3;
	cinfo.in_color_space   = JCS_RGB;
	
	jpeg_set_defaults(&cinfo);
	jpeg_set_quality (&cinfo, 80, true);
	jpeg_start_compress(&cinfo, true);
	JSAMPROW row_pointer;          /* pointer to a single row */
	while (cinfo.next_scanline < cinfo.image_height) {
		row_pointer = (JSAMPROW) &buf[cinfo.next_scanline * 3 * wi];
		jpeg_write_scanlines(&cinfo, &row_pointer, 1);
	}
	jpeg_finish_compress(&cinfo);
	free(buf);
	fclose(outfile);
}
	
void open_thumb(std::string path, GLuint tex) {
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

	JSAMPROW row_pointer[1];
	
	FILE *infile = fopen(path.c_str(), "rb");
	unsigned long location = 0;
	int i = 0;
	
	if (!infile)
	{
		printf("Error opening jpeg file %s\n!", path.c_str() );
		return;
	}

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, infile);
	jpeg_read_header(&cinfo, TRUE);
	jpeg_start_decompress(&cinfo);

	unsigned char* raw_image = (unsigned char*)malloc(cinfo.output_width*cinfo.output_height*cinfo.num_components);
	row_pointer[0] = (unsigned char *)malloc(cinfo.output_width*cinfo.num_components);

	while(cinfo.output_scanline < cinfo.image_height)
	{
		jpeg_read_scanlines(&cinfo, row_pointer, 1);
		for (i = 0; i < cinfo.image_width*cinfo.num_components; i++) raw_image[location++] = row_pointer[0][i];
	}

	jpeg_finish_decompress(&cinfo);
	
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cinfo.image_width, cinfo.image_height, 0, GL_RGB, GL_UNSIGNED_BYTE, raw_image);

	jpeg_destroy_decompress(&cinfo);
	
	free(row_pointer[0]);
	fclose(infile);
	
}

std::ifstream::pos_type getFileSize(std::string filename)
{
	std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);

	return ((int)in.tellg());
}

void concat_files(std::ostream &ofile, const std::string &path, std::vector<std::vector<std::string>> &filepaths) {
	std::vector<std::string> paths;
	paths.push_back(path);
	for (int i = 0; i < filepaths.size(); i++) {
		for (int j = 0; j < filepaths[i].size(); j++) {
			paths.push_back(filepaths[i][j]);
		}
	}
	
	int size = paths.size();
	ofile.write((const char *)&size, 4);
	for (int i = 0; i < paths.size(); i++) {
		// save header
		if (paths[i] == "") continue;
		ifstream fileInput;
		fileInput.open(paths[i], ios::in | ios::binary);
		int fileSize = getFileSize(paths[i]);
		ofile.write((const char *)&fileSize, 4);
		fileInput.close();
	}
	for (int i = 0; i < paths.size(); i++) {
		// save body
		if (paths[i] == "") continue;
		int fileSize = getFileSize(paths[i]);
		ifstream fileInput;
		fileInput.open(paths[i], ios::in | ios::binary);
		printf("path %s\n", paths[i].c_str());
		fflush(stdout);
		char *inputBuffer = new char[fileSize];
		fileInput.read(inputBuffer, fileSize);
		ofile.write(inputBuffer, fileSize);
		delete(inputBuffer);
		fileInput.close();
	}
}

bool check_version(const std::string &path) {
	bool concat = true;
	ifstream bfile;
	bfile.open(path, ios::in | ios::binary);
	char *buffer = new char[7];
	bfile.read(buffer, 7);
	if (buffer[0] == 0x45 and buffer[1] == 0x57 and buffer[2] == 0x4F and buffer[3] == 0x43 and buffer[4] == 0x76 and buffer[5] == 0x6A and buffer[6] == 0x20) {
		concat = false;
	} 
	delete(buffer);
	bfile.close();
	return concat;
}

std::string deconcat_files(const std::string &path) {
	bool concat = check_version(path);
	std::string outpath;
	ifstream bfile;
	bfile.open(path, ios::in | ios::binary);
	if (concat) {
		int32_t num;
		bfile.read((char *)&num, 4);
		std::vector<int> sizes;
		//num = _byteswap_ulong(num - 1) + 1;
		for (int i = 0; i < num; i++) {
			int size;
			bfile.read((char *)&size, 4);
			sizes.push_back(size);
		}
		ofstream ofile;
		std::string name = remove_extension(basename(path));
		int count = 0;
		while (1) {
			outpath = mainprogram->temppath + name + ".mainfile";
			if (!exists(outpath)) {
				ofile.open(outpath, ios::out | ios::binary);
				break;
			}
			count++;
			name = remove_version(name) + "_" + std::to_string(count);
		}
		char *ibuffer = new char[sizes[0]];
		bfile.read(ibuffer, sizes[0]);
		ofile.write(ibuffer, sizes[0]);
		ofile.close();
		for (int i = 0; i < num - 1; i++) {
			ofstream ofile;
			ofile.open(outpath + "_" + std::to_string(i) + ".file", ios::out | ios::binary);
			char *ibuffer = new char[sizes[i + 1]];
			bfile.read(ibuffer, sizes[i + 1]);
			ofile.write(ibuffer, sizes[i + 1]);
			ofile.close();
		}
	}
	bfile.close();
	if (concat) return(outpath);
	else return "";
}

	
void Shelf::save(const std::string &path) {
	std::string ext = path.substr(path.length() - 6, std::string::npos);
	std::string str;
	if (ext != ".shelf") str = path + ".shelf";
	else str = path;
	
	std::vector<std::string> filestoadd;
	ofstream wfile;
	wfile.open(path);
	wfile << "EWOCvj SHELFFILE V0.1\n";
	
	wfile << "ELEMS\n";
	for (int j = 0; j < 16; j++) {
		ShelfElement* elem = this->elements[j];
		wfile << "PATH\n";
		wfile << elem->path;
		wfile << "\n";
		if (elem->path == "") continue;
		wfile << "TYPE\n";
		wfile << std::to_string(elem->type);
		wfile << "\n";
		if (elem->type == ELEM_LAYER or elem->type == ELEM_DECK or elem->type == ELEM_MIX) {
			filestoadd.push_back(elem->path);
		}
		filestoadd.push_back(elem->jpegpath);
		wfile << "JPEGPATH\n";
		wfile << elem->jpegpath;
		wfile << "\n";
		wfile << "LAUNCHTYPE\n";
		wfile << std::to_string(elem->launchtype);
		wfile << "\n";
		wfile << "MIDI0\n";
		wfile << std::to_string(elem->button->midi[0]);
		wfile << "\n";
		wfile << "MIDI1\n";
		wfile << std::to_string(elem->button->midi[1]);
		wfile << "\n";
		wfile << "MIDIPORT\n";
		wfile << elem->button->midiport;
		wfile << "\n";
	}
	wfile << "ENDOFELEMS\n";

	wfile << "ELEMMIDI\n";
	for (int j = 0; j < 16; j++) {
		ShelfElement* elem = this->elements[j];
		wfile << "MIDI0\n";
		wfile << std::to_string(this->buttons[j]->midi[0]);
		wfile << "\n";
		wfile << "MIDI1\n";
		wfile << std::to_string(this->buttons[j]->midi[1]);
		wfile << "\n";
		wfile << "MIDIPORT\n";
		wfile << elem->button->midiport;
		wfile << "\n";
	}
	wfile << "ENDOFELEMMIDI\n";

	wfile << "ENDOFFILE\n";
	wfile.close();
	
    ofstream outputfile;
	outputfile.open(mainprogram->temppath + "tempconcatshelf", ios::out | ios::binary);
	std::vector<std::vector<std::string>> filestoadd2;
	filestoadd2.push_back(filestoadd);
	concat_files(outputfile, path, filestoadd2);
	outputfile.close();
	if (exists(str)) boost::filesystem::remove(str);
	boost::filesystem::remove(path);
	boost::filesystem::rename(mainprogram->temppath + "tempconcatshelf", str);
}

void Shelf::open_dir() {
	struct dirent *ent;
	if ((ent = readdir(mainprogram->opendir)) != nullptr and mainprogram->shelfdircount < 16) {
		char *filepath = (char*)malloc(1024);
		strcpy(filepath, mainprogram->shelfpath.c_str());
		strcat(filepath, "/");
		strcat(filepath, ent->d_name);
		std::string str(filepath);
		
		bool ret;
		if (str.substr(str.length() - 6, std::string::npos) == ".layer") {
			ret = this->open_layer(str, mainprogram->shelfdircount);
		}
		else {
			ILboolean r = ilLoadImage(str.c_str());
			if (r == IL_TRUE) ret = this->open_image(str, mainprogram->shelfdircount);
			else ret = this->open_videofile(str, mainprogram->shelfdircount);
		}
		
		if (ret) mainprogram->shelfdircount++;
	}
	else mainprogram->openshelfdir = false;
}



void Shelf::open_shelffiles() {
	// order elements
	bool cont = mainprogram->order_paths(true);
	if (!cont) return;

	ShelfElement* elem = this->elements[mainprogram->shelffileselem];
	std::string str = mainprogram->paths[mainprogram->shelffilescount];
	if (isimage(str)) {
		mainmix->mouseshelf->open_image(str, mainprogram->shelffileselem);
	}
	else if (str.substr(str.length() - 6, std::string::npos) == ".layer") {
		mainmix->mouseshelf->open_layer(str, mainprogram->shelffileselem);
	}
	else if (str.substr(str.length() - 5, std::string::npos) == ".deck") {
		elem->tex = get_deckmixtex(str);
		elem->path = str;
		elem->jpegpath = "";
		elem->type = ELEM_DECK;
	}
	else if (str.substr(str.length() - 4, std::string::npos) == ".mix") {
		elem->tex = get_deckmixtex(str);
		elem->path = str;
		elem->jpegpath = "";
		elem->type = ELEM_MIX;
	}
	else {
		mainmix->mouseshelf->open_videofile(str, mainprogram->shelffileselem);
	}
	std::string name = remove_extension(this->basepath);
	int count = 0;
	while (1) {
		elem->jpegpath = mainprogram->temppath + name + ".jpg";
		if (!exists(elem->jpegpath)) {
			break;
		}
		count++;
		name = remove_version(name) + "_" + std::to_string(count);
	}
	save_thumb(elem->jpegpath, elem->tex);

	mainprogram->shelffileselem++;
	mainprogram->shelffilescount++;
	if (mainprogram->shelffilescount == mainprogram->paths.size() or mainprogram->shelffileselem == 16) {
		mainprogram->openshelffiles = false;
		mainprogram->paths.clear();
		mainprogram->multistage = 0;
	}
	mainprogram->currshelffilesdir = dirname(str);
}

bool Shelf::open_videofile(const std::string &path, int pos) {
	ShelfElement* elem = this->elements[pos];
	elem->path = path;
	elem->type = ELEM_FILE;
	Layer *lay = new Layer(true);
	lay->dummy = true;
	lay->open_video(0, path, true);
	lay->frame = lay->numf / 2.0f;
	lay->prevframe = -1;
	std::unique_lock<std::mutex> olock(lay->endopenlock);
	lay->endopenvar.wait(olock, [&] {return lay->opened; });
	lay->opened = false;
	olock.unlock();
	if (lay->openerr) {
		elem->path = "";
		lay->closethread = true;
		while (lay->closethread) {
			lay->ready = true;
			lay->startdecode.notify_one();
		}
		delete lay;
		return 0;
	}
	lay->ready = true;
	while (lay->ready) {
		lay->startdecode.notify_one();
	}
	std::unique_lock<std::mutex> lock(lay->enddecodelock);
	lay->enddecodevar.wait(lock, [&] {return lay->processed; });
	lay->processed = false;
	lock.unlock();

	lay->initialize(lay->decresult->width, lay->decresult->height);
	if (lay->vidformat == 188 or lay->vidformat == 187) {
		if (lay->decresult->compression == 187) {
			glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, lay->decresult->width, lay->decresult->height, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, lay->decresult->size, lay->decresult->data);
		}
		else if (lay->decresult->compression == 190) {
			glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, lay->decresult->width, lay->decresult->height, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, lay->decresult->size, lay->decresult->data);
		}
	}
	else {
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, lay->decresult->width, lay->decresult->height, GL_BGRA, GL_UNSIGNED_BYTE, lay->decresult->data);
	}
	elem->tex = copy_tex(lay->texture);
	
	lay->closethread = true;
	while (lay->closethread) {
		lay->ready = true;
		lay->startdecode.notify_one();
	}
	delete lay;
	
	return 1;
}
	
bool Shelf::open_layer(const std::string& path, int pos) {
	float black[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	ShelfElement* elem = this->elements[pos];
	elem->path = path;
	elem->type = ELEM_LAYER;
	Layer* lay = new Layer(false);
	lay->dummy = true;
	lay->pos = 0;
	lay->node = mainprogram->nodesmain->currpage->add_videonode(2);
	lay->node->layer = lay;
	lay->lasteffnode[0] = lay->node;
	mainmix->open_layerfile(path, lay, false, 0);
	std::unique_lock<std::mutex> olock(lay->endopenlock);
	lay->endopenvar.wait(olock, [&] {return lay->opened; });
	lay->opened = false;
	olock.unlock();
	lay->node->calc = true;
	lay->node->walked = false;
	if (lay->openerr) {
		elem->path = "";
		lay->closethread = true;
		while (lay->closethread) {
			lay->ready = true;
			lay->startdecode.notify_one();
		}
		delete lay;
		return 0;
	}
	lay->playbut->value = false;
	lay->revbut->value = false;
	lay->bouncebut->value = false;
	for (int k = 0; k < lay->effects[0].size(); k++) {
		lay->effects[0][k]->node->calc = true;
		lay->effects[0][k]->node->walked = false;
		lay->lasteffnode[0] = lay->effects[0][k]->node;
	}
	lay->lasteffnode[1] = lay->lasteffnode[0];
	if (lay->type != ELEM_IMAGE) {
		lay->frame = 0.0f;
		lay->prevframe = -1;
		lay->ready = true;
		while (lay->ready) {
			lay->startdecode.notify_one();
		}
		std::unique_lock<std::mutex> lock(lay->enddecodelock);
		lay->enddecodevar.wait(lock, [&] {return lay->processed; });
		lay->processed = false;
		lock.unlock();

		lay->initialize(lay->decresult->width, lay->decresult->height);
		glBindTexture(GL_TEXTURE_2D, lay->texture);
		if (lay->vidformat == 188 or lay->vidformat == 187) {
			if (lay->decresult->compression == 187) {
				glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, lay->decresult->width, lay->decresult->height, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, lay->decresult->size, lay->decresult->data);
			}
			else if (lay->decresult->compression == 190) {
				glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, lay->decresult->width, lay->decresult->height, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, lay->decresult->size, lay->decresult->data);
			}
		}
		else {
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, lay->decresult->width, lay->decresult->height, GL_BGRA, GL_UNSIGNED_BYTE, lay->decresult->data);
		}
	}
	lay->fbotex = copy_tex(lay->texture);
	onestepfrom(0, lay->node, nullptr, -1, -1);
	if (lay->effects[0].size()) {
		elem->tex = copy_tex(lay->effects[0][lay->effects[0].size() - 1]->fbotex);
	}
	else {
		elem->tex = copy_tex(lay->fbotex);
	}
	lay->closethread = true;
	while (lay->closethread) {
		lay->ready = true;
		lay->startdecode.notify_one();
	}
	delete lay;

	return 1;
}

bool Shelf::insert_deck(const std::string& path, bool deck, int pos) {
	ShelfElement* elem = this->elements[pos];
	elem->path = path;
	elem->type = ELEM_DECK;
	if (mainprogram->prevmodus) elem->tex = copy_tex(mainprogram->nodesmain->mixnodes[deck]->mixtex);
	else elem->tex = copy_tex(mainprogram->nodesmain->mixnodescomp[deck]->mixtex);
	std::string jpegpath = path + ".jpeg";
	save_thumb(jpegpath, elem->tex);
	elem->jpegpath = jpegpath;
	return 1;
}

bool Shelf::insert_mix(const std::string& path, int pos) {
	ShelfElement* elem = this->elements[pos];
	elem->path = path;
	elem->type = ELEM_MIX;
	if (mainprogram->prevmodus) elem->tex = copy_tex(mainprogram->nodesmain->mixnodes[2]->mixtex);
	else elem->tex = copy_tex(mainprogram->nodesmain->mixnodescomp[2]->mixtex);
	std::string jpegpath = path + ".jpeg";
	save_thumb(jpegpath, elem->tex);
	elem->jpegpath = jpegpath;
	return 1;
}

bool Shelf::open_image(const std::string &path, int pos) {
	ShelfElement* elem = this->elements[pos];
	ILuint tempim;
	ilGenImages(1, &tempim);
	ilBindImage(tempim);
	ILboolean ret = ilLoadImage(path.c_str());
	if (ret == IL_FALSE) return 0;
	elem->path = path;
	elem->type = ELEM_IMAGE;
	float x = ilGetInteger(IL_IMAGE_WIDTH);
	float y = ilGetInteger(IL_IMAGE_HEIGHT);
	int iw, ih, xs, ys;
	if (x / y > 1920.0f / 1080.0f) {
		iw = x;
		ih = y * x * glob->h / y / glob->w;
		xs = 0;
		ys = (ih - y) / 2.0f;
	}
	else {
		iw = x * 1920.0f * y / 1080.0f / x;
		ih = y;
		xs = (iw - x) / 2.0f;
		ys = 0;
	}
	int bpp = ilGetInteger(IL_IMAGE_BPP);
	glActiveTexture(GL_TEXTURE0);
	glDeleteTextures(1, &elem->tex);
	glGenTextures(1, &elem->tex);
	glBindTexture(GL_TEXTURE_2D, elem->tex);
	if (bpp == 3) {
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, iw, ih);
		glTexSubImage2D(GL_TEXTURE_2D, 0, xs, ys, x, y, GL_RGB, GL_UNSIGNED_BYTE, ilGetData());
	}
	else if (bpp == 4) {
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, iw, ih);
		glTexSubImage2D(GL_TEXTURE_2D, 0, xs, ys, x, y, GL_BGRA, GL_UNSIGNED_BYTE, ilGetData());
	}
	
	return 1;
}
	
bool Shelf::open(const std::string &path) {
	
	if (!exists(path)) return 0;
	std::string result = deconcat_files(path);
	bool concat = (result != "");
	ifstream rfile;
	if (concat) rfile.open(result);
	else rfile.open(path);
	
	this->erase();
	int filecount = 0;
	std::string istring;
	getline(rfile, istring);
	while (getline(rfile, istring)) {
		if (istring == "ENDOFFILE") {
			break;
		}
		else if (istring == "ELEMS") {
			int count = 0;
			ShelfElement* elem = nullptr;
			while (getline(rfile, istring)) {
				if (istring == "ENDOFELEMS") break;
				if (istring == "PATH") {
					getline(rfile, istring);
					elem = this->elements[count];
					elem->path = istring;
					count++;
					if (elem->path == "") {
						continue;
					}
				}
				if (istring == "TYPE") {
					getline(rfile, istring);
					elem->type = (ELEM_TYPE)std::stoi(istring);
					std::string suf = "";
					if (elem->type == ELEM_LAYER) suf = ".layer";
					if (elem->type == ELEM_DECK) suf = ".deck";
					if (elem->type == ELEM_MIX) suf = ".mix";
					if (suf != "") {
						if (concat) {
							std::string name = remove_extension(basename(elem->path));
							int pcount = 0;
							while (1) {
								elem->path = mainprogram->temppath + name + suf;
								if (!exists(elem->path)) {
									break;
								}
								pcount++;
								name = remove_version(name) + "_" + std::to_string(pcount);
							}
							boost::filesystem::rename(result + "_" + std::to_string(filecount) + ".file", elem->path);
							filecount++;
						}
					}
				}
				if (istring == "JPEGPATH") {
					getline(rfile, istring);
					elem->jpegpath = result + "_" + std::to_string(filecount) + ".file";
					open_thumb(result + "_" + std::to_string(filecount) + ".file", elem->tex);
					filecount++;
				}
				if (istring == "LAUNCHTYPE") {
					getline(rfile, istring);
					elem->launchtype = std::stoi(istring);
				}
				if (istring == "MIDI0") {
					getline(rfile, istring);
					elem->button->midi[0] = std::stoi(istring);
				}
				if (istring == "MIDI1") {
					getline(rfile, istring);
					elem->button->midi[1] = std::stoi(istring);
				}
				if (istring == "MIDIPORT") {
					getline(rfile, istring);
					elem->button->midiport = istring;
				}
			}
		}
		else if (istring == "ELEMMIDI") {
			int count = 0;
			while (getline(rfile, istring)) {
				ShelfElement* elem = this->elements[count];
				if (istring == "ENDOFELEMMIDI") break;
				if (istring == "MIDI0") {
					getline(rfile, istring);
					elem->button->midi[0] = std::stoi(istring);
				}
				if (istring == "MIDI1") {
					getline(rfile, istring);
					elem->button->midi[1] = std::stoi(istring);
				}
				if (istring == "MIDIPORT") {
					getline(rfile, istring);
					elem->button->midiport = istring;
				}
			}
		}
	}

	rfile.close();

	return 1;
}

void blacken(GLuint tex) {
	std::vector<int> emptydata(4096);
	std::fill(emptydata.begin(), emptydata.end(), 0xff000000);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 64, 64, 0, GL_BGRA, GL_UNSIGNED_BYTE, &emptydata[0]);
}

void Shelf::erase() {
	for (int i = 0; i < 16; i++) {
		ShelfElement* elem = this->elements[i];
		elem->path = "";
		elem->type = ELEM_FILE;
		blacken(elem->tex);
		blacken(elem->oldtex);
	}
}
	

void write_genmidi(ostream& wfile, LayMidi *lm) {
	wfile << "PLAY\n";
	wfile << std::to_string(lm->play[0]);
	wfile << "\n";
	wfile << std::to_string(lm->play[1]);
	wfile << "\n";
	wfile << lm->playstr;
	wfile << "\n";

	wfile << "BACKW\n";
	wfile << std::to_string(lm->backw[0]);
	wfile << "\n";
	wfile << std::to_string(lm->backw[1]);
	wfile << "\n";
	wfile << lm->backwstr;
	wfile << "\n";
	
	wfile << "BOUNCE\n";
	wfile << std::to_string(lm->bounce[0]);
	wfile << "\n";
	wfile << std::to_string(lm->bounce[1]);
	wfile << "\n";
	wfile << lm->bouncestr;
	wfile << "\n";

	wfile << "FRFORW\n";
	wfile << std::to_string(lm->frforw[0]);
	wfile << "\n";
	wfile << std::to_string(lm->frforw[1]);
	wfile << "\n";
	wfile << lm->frforwstr;
	wfile << "\n";

	wfile << "FRBACKW\n";
	wfile << std::to_string(lm->frbackw[0]);
	wfile << "\n";
	wfile << std::to_string(lm->frbackw[1]);
	wfile << "\n";
	wfile << lm->frbackwstr;
	wfile << "\n";

	wfile << "SPEED\n";
	wfile << std::to_string(lm->speed[0]);
	wfile << "\n";
	wfile << std::to_string(lm->speed[1]);
	wfile << "\n";
	wfile << lm->speedstr;
	wfile << "\n";

	wfile << "SPEEDZERO\n";
	wfile << std::to_string(lm->speedzero[0]);
	wfile << "\n";
	wfile << std::to_string(lm->speedzero[1]);
	wfile << "\n";
	wfile << lm->speedzerostr;
	wfile << "\n";

	wfile << "OPACITY\n";
	wfile << std::to_string(lm->opacity[0]);
	wfile << "\n";
	wfile << std::to_string(lm->opacity[1]);
	wfile << "\n";
	wfile << lm->opacitystr;
	wfile << "\n";

	wfile << "FREEZE\n";
	wfile << std::to_string(lm->scratchtouch[0]);
	wfile << "\n";
	wfile << std::to_string(lm->scratchtouch[1]);
	wfile << "\n";
	wfile << lm->scratchtouchstr;
	wfile << "\n";

	wfile << "SCRATCH\n";
	wfile << std::to_string(lm->scratch[0]);
	wfile << "\n";
	wfile << std::to_string(lm->scratch[1]);
	wfile << "\n";
	wfile << lm->scratchstr;
	wfile << "\n";
}



void save_genmidis(std::string path) {
	ofstream wfile;
	wfile.open(path);
	wfile << "EWOCvj GENMIDIS V0.1\n";
	
	wfile << "LAYMIDIA\n";
	write_genmidi(wfile, laymidiA);	

	wfile << "LAYMIDIB\n";
	write_genmidi(wfile, laymidiB);	

	wfile << "LAYMIDIC\n";
	write_genmidi(wfile, laymidiC);	

	wfile << "LAYMIDID\n";
	write_genmidi(wfile, laymidiD);
	
	
	wfile << "WORMHOLEMIDI0\n";
	wfile << std::to_string(mainprogram->wormhole1->midi[0]);
	wfile << "\n";
	wfile << "WORMHOLEMIDI1\n";
	wfile << std::to_string(mainprogram->wormhole1->midi[1]);
	wfile << "\n";
	wfile << "WORMHOLEMIDIPORT\n";
	wfile << mainprogram->wormhole1->midiport;
	wfile << "\n";
	
	wfile << "EFFCAT0MIDI0\n";
	wfile << std::to_string(mainprogram->effcat[0]->midi[0]);
	wfile << "\n";
	wfile << "EFFCAT0MIDI1\n";
	wfile << std::to_string(mainprogram->effcat[0]->midi[1]);
	wfile << "\n";
	wfile << "EFFCAT0MIDIPORT\n";
	wfile << mainprogram->effcat[0]->midiport;
	wfile << "\n";
	wfile << "EFFCAT1MIDI0\n";
	wfile << std::to_string(mainprogram->effcat[1]->midi[0]);
	wfile << "\n";
	wfile << "EFFCAT1MIDI1\n";
	wfile << std::to_string(mainprogram->effcat[1]->midi[1]);
	wfile << "\n";
	wfile << "EFFCAT1MIDIPORT\n";
	wfile << mainprogram->effcat[1]->midiport;
	wfile << "\n";
	
	wfile << "RECORDMIDI0\n";
	wfile << std::to_string(mainmix->recbut->midi[0]);
	wfile << "\n";
	wfile << "RECORDMIDI1\n";
	wfile << std::to_string(mainmix->recbut->midi[1]);
	wfile << "\n";
	wfile << "RECORDMIDIPORT\n";
	wfile << mainmix->recbut->midiport;
	wfile << "\n";
	
	wfile << "LOOPSTATIONMIDI\n";
	LoopStation *ls;
	for (int i = 0; i < 2; i++) {
		if (i == 0) ls = lp;
		else ls = lpc;
		for (int j = 0; j < ls->numelems; j++) {
			wfile << "LOOPSTATIONMIDI0:" + std::to_string(i) + ":" + std::to_string(j) + "\n";
			wfile << std::to_string(ls->elems[j]->recbut->midi[0]) + "\n";
			wfile << std::to_string(ls->elems[j]->loopbut->midi[0]) + "\n";
			wfile << std::to_string(ls->elems[j]->playbut->midi[0]) + "\n";
			wfile << std::to_string(ls->elems[j]->speed->midi[0]) + "\n";
			wfile << "LOOPSTATIONMIDI1:" + std::to_string(i) + ":" + std::to_string(j) + "\n";
			wfile << std::to_string(ls->elems[j]->recbut->midi[1]) + "\n";
			wfile << std::to_string(ls->elems[j]->loopbut->midi[1]) + "\n";
			wfile << std::to_string(ls->elems[j]->playbut->midi[1]) + "\n";
			wfile << std::to_string(ls->elems[j]->speed->midi[1]) + "\n";
			wfile << "LOOPSTATIONMIDIPORT:" + std::to_string(i) + ":" + std::to_string(j) + "\n";
			wfile << ls->elems[j]->recbut->midiport + "\n";
			wfile << ls->elems[j]->loopbut->midiport + "\n";
			wfile << ls->elems[j]->playbut->midiport + "\n";
			wfile << ls->elems[j]->speed->midiport + "\n";
		}
	}
	wfile << "LOOPSTATIONMIDIEND\n";
	

	wfile << "ENDOFFILE\n";
	wfile.close();
}

void open_genmidis(std::string path) {
	ifstream rfile;
	rfile.open(path);
	std::string istring;

	LayMidi *lm = nullptr;
	while (getline(rfile, istring)) {
		if (istring == "ENDOFFILE") break;
	
		if (istring == "LAYMIDIA") lm = laymidiA;
		if (istring == "LAYMIDIB") lm = laymidiB;
		if (istring == "LAYMIDIC") lm = laymidiC;
		if (istring == "LAYMIDID") lm = laymidiD;
		
		if (istring == "PLAY") {
			getline(rfile, istring);
			lm->play[0] = std::stoi(istring);
			getline(rfile, istring);
			lm->play[1] = std::stoi(istring);
			getline(rfile, istring);
			lm->playstr = istring;
		}
		if (istring == "BACKW") {
			getline(rfile, istring);
			lm->backw[0] = std::stoi(istring);
			getline(rfile, istring);
			lm->backw[1] = std::stoi(istring);
			getline(rfile, istring);
			lm->backwstr = istring;
		}
		if (istring == "BOUNCE") {
			getline(rfile, istring);
			lm->bounce[0] = std::stoi(istring);
			getline(rfile, istring);
			lm->bounce[1] = std::stoi(istring);
			getline(rfile, istring);
			lm->bouncestr = istring;
		}
		if (istring == "FRFORW") {
			getline(rfile, istring);
			lm->frforw[0] = std::stoi(istring);
			getline(rfile, istring);
			lm->frforw[1] = std::stoi(istring);
			getline(rfile, istring);
			lm->frforwstr = istring;
		}
		if (istring == "FRBACKW") {
			getline(rfile, istring);
			lm->frbackw[0] = std::stoi(istring);
			getline(rfile, istring);
			lm->frbackw[1] = std::stoi(istring);
			getline(rfile, istring);
			lm->frbackwstr = istring;
		}
		if (istring == "SPEED") {
			getline(rfile, istring);
			lm->speed[0] = std::stoi(istring);
			getline(rfile, istring);
			lm->speed[1] = std::stoi(istring);
			getline(rfile, istring);
			lm->speedstr = istring;
		}
		if (istring == "SPEEDZERO") {
			getline(rfile, istring);
			lm->speedzero[0] = std::stoi(istring);
			getline(rfile, istring);
			lm->speedzero[1] = std::stoi(istring);
			getline(rfile, istring);
			lm->speedzerostr = istring;
		}
		if (istring == "OPACITY") {
			getline(rfile, istring);
			lm->opacity[0] = std::stoi(istring);
			getline(rfile, istring);
			lm->opacity[1] = std::stoi(istring);
			getline(rfile, istring);
			lm->opacitystr = istring;
		}
		if (istring == "FREEZE") {
			getline(rfile, istring);
			lm->scratchtouch[0] = std::stoi(istring);
			getline(rfile, istring);
			lm->scratchtouch[1] = std::stoi(istring);
			getline(rfile, istring);
			lm->scratchtouchstr = istring;
		}
		if (istring == "SCRATCH") {
			getline(rfile, istring);
			lm->scratch[0] = std::stoi(istring);
			getline(rfile, istring);
			lm->scratch[1] = std::stoi(istring);
			getline(rfile, istring);
			lm->scratchstr = istring;
		}
		
		if (istring == "WORMHOLE0MIDI0") {
			getline(rfile, istring);
			mainprogram->wormhole1->midi[0] = std::stoi(istring);
		}
		if (istring == "WORMHOLEMIDI1") {
			getline(rfile, istring);
			mainprogram->wormhole1->midi[1] = std::stoi(istring);
		}
		if (istring == "WORMHOLEMIDIPORT") {
			getline(rfile, istring);
			mainprogram->wormhole1->midiport = istring;
		}
		
		if (istring == "EFFCAT0MIDI0") {
			getline(rfile, istring);
			mainprogram->effcat[0]->midi[0] = std::stoi(istring);
		}
		if (istring == "EFFCAT0MIDI1") {
			getline(rfile, istring);
			mainprogram->effcat[0]->midi[1] = std::stoi(istring);
		}
		if (istring == "EFFCAT0MIDIPORT") {
			getline(rfile, istring);
			mainprogram->effcat[0]->midiport = istring;
		}
		if (istring == "EFFCAT1MIDI0") {
			getline(rfile, istring);
			mainprogram->effcat[1]->midi[0] = std::stoi(istring);
		}
		if (istring == "EFFCAT1MIDI1") {
			getline(rfile, istring);
			mainprogram->effcat[1]->midi[1] = std::stoi(istring);
		}
		if (istring == "EFFCAT1MIDIPORT") {
			getline(rfile, istring);
			mainprogram->effcat[1]->midiport = istring;
		}
			
		if (istring == "RECORDMIDI0") {
			getline(rfile, istring);
			mainmix->recbut->midi[0] = std::stoi(istring);
		}
		if (istring == "RECORDMIDI1") {
			getline(rfile, istring);
			mainmix->recbut->midi[1] = std::stoi(istring);
		}
		if (istring == "RECORDMIDIPORT") {
			getline(rfile, istring);
			mainmix->recbut->midiport = istring;
		}
			
		if (istring == "LOOPSTATIONMIDI") {
			LoopStation *ls;
			for (int i = 0; i < 2; i++) {
				if (i == 0) ls = lp;
				else ls = lpc;
				for (int j = 0; j < ls->numelems; j++) {
					getline(rfile, istring);
					if (istring == "LOOPSTATIONMIDI0:" + std::to_string(i) + ":" + std::to_string(j)) {
						getline(rfile, istring);
						ls->elems[j]->recbut->midi[0] = std::stoi(istring);
						getline(rfile, istring);
						ls->elems[j]->loopbut->midi[0] = std::stoi(istring);
						getline(rfile, istring);
						ls->elems[j]->playbut->midi[0] = std::stoi(istring);
						getline(rfile, istring);
						ls->elems[j]->speed->midi[0] = std::stoi(istring);
					}
					if (istring == "LOOPSTATIONMIDI1:" + std::to_string(i) + ":" + std::to_string(j)) {
						getline(rfile, istring);
						ls->elems[j]->recbut->midi[1] = std::stoi(istring);
						getline(rfile, istring);
						ls->elems[j]->loopbut->midi[1] = std::stoi(istring);
						getline(rfile, istring);
						ls->elems[j]->playbut->midi[1] = std::stoi(istring);
						getline(rfile, istring);
						ls->elems[j]->speed->midi[1] = std::stoi(istring);
					}
					if (istring == "LOOPSTATIONMIDIPORT:" + std::to_string(i) + ":" + std::to_string(j)) {
						getline(rfile, istring);
						ls->elems[j]->recbut->midiport = istring;
						getline(rfile, istring);
						ls->elems[j]->loopbut->midiport = istring;
						getline(rfile, istring);
						ls->elems[j]->playbut->midiport = istring;
						getline(rfile, istring);
						ls->elems[j]->speed->midiport = istring;
					}
				}
			}
		}
	}
	rfile.close();
}





int main(int argc, char* argv[]){
	bool quit = false;

	//#ifdef _WIN64
 //  	HRESULT hr = SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
 //   if (FAILED(hr))
 //   {
 //       _com_error err(hr);
 //       fwprintf(stderr, L"SetProcessDpiAwareness: %s\n", err.ErrorMessage());
 //   }
 //   #endif
	
    
    // OPENAL
	const char *defaultDeviceName = alcGetString(nullptr, ALC_DEFAULT_DEVICE_SPECIFIER);
	ALCdevice *device = alcOpenDevice(defaultDeviceName);
	ALCcontext *context = alcCreateContext(device, nullptr);
	bool succes = alcMakeContextCurrent(context);
	
	//initializing devIL
	ilInit();
	
	if (SDL_Init(SDL_INIT_VIDEO) < 0) /* Initialize SDL's Video subsystem */
		mainprogram->quitting = "Unable to initialize SDL"; /* Or die on error */

	/* Request opengl 4.6 context.
	 * SDL doesn't have the ability to choose which profile at this time of writing,
	 * but it should default to the core profile */
 	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	/* Turn on double buffering */
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetSwapInterval(0); //vsync off

	SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");
	
	SDL_Rect rc;
 	SDL_GetDisplayUsableBounds(0, &rc);
	auto sw = rc.w;
	auto sh = rc.h;
 	SDL_Window *win = SDL_CreateWindow(PROGRAM_NAME, 0, 0, sw, sh, SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);

 	glob = new Globals;
 	int wi, he;	
	SDL_GL_GetDrawableSize(win, &wi, &he);
	glob->w = (float)wi;
	glob->h = (float)he;
	SDL_DisplayMode DM;
	SDL_GetCurrentDisplayMode(0, &DM);
	glob->resfac = (float)DM.w / (float)DM.h;

	mainprogram = new Program;	
 	mainprogram->mainwindow = win;
 	lp = new LoopStation;
 	lpc = new LoopStation;
 	loopstation = lp;
 	mainmix = new Mixer;
 	binsmain = new BinsMain;

#ifdef _WIN64
	boost::filesystem::path p0{ mainprogram->docpath };
	if (!exists(mainprogram->docpath)) boost::filesystem::create_directory(p0);
	boost::filesystem::path p2{ mainprogram->docpath + "recordings" };
	mainprogram->currrecdir = p2.string();
	if (!exists("mainprogram->docpath + /recordings")) boost::filesystem::create_directory(p2);
	boost::filesystem::path p4{ mainprogram->temppath };
	if (!exists(mainprogram->temppath)) boost::filesystem::create_directory(p4);
	boost::filesystem::path p5{ mainprogram->docpath + "projects" };
	mainprogram->currprojdir = p5.string();
	if (!exists("mainprogram->docpath + /projects")) boost::filesystem::create_directory(p5);
	boost::filesystem::path p6{ mainprogram->docpath + "autosaves" };
	if (!exists("mainprogram->docpath + /autosaves")) boost::filesystem::create_directory(p6);
	boost::filesystem::path p7{ mainprogram->docpath + "elems" };
	if (!exists("mainprogram->docpath + /elems")) boost::filesystem::create_directory(p7);
#else
#ifdef __linux__
	std::string homedir(getenv("HOME"));
	mainprogram->homedir = homedir;
	boost::filesystem::path e{ homedir + "/.ewocvj2" };
	mainprogram->datadir = e.string();
	if (!exists(homedir + "/.ewocvj2")) boost::filesystem::create_directory(e);
	boost::filesystem::path p2{ homedir + "/.ewocvj2/recordings" };
	mainprogram->currrecdir = p2.string();
	if (!exists(homedir + "/recordings")) boost::filesystem::create_directory(p2);
	boost::filesystem::path p4{ homedir + "/.ewocvj2/temp" };
	if (!exists(homedir + "/.ewocvj2/temp")) boost::filesystem::create_directory(p4);
	boost::filesystem::path p5{ homedir + "/.ewocvj2/projects" };
	mainprogram->currprojdir = p5.string();
	if (!exists(homedir + "/.ewocvj2/projects")) boost::filesystem::create_directory(p5);
	boost::filesystem::path p6{ homedir + "/.ewocvj2/autosaves" };
	if (!exists(homedir + "/.ewocvj2/autosaves")) boost::filesystem::create_directory(p6);
	boost::filesystem::path p7{ homedir + "/.ewocvj2/elems" };
	if (!exists(homedir + "/.ewocvj2/elems")) boost::filesystem::create_directory(p7);
#endif
#endif
	//empty temp dir if program crashed last time
	boost::filesystem::path path_to_remove(mainprogram->temppath);
	for (boost::filesystem::directory_iterator end_dir_it, it(path_to_remove); it != end_dir_it; ++it) {
		boost::filesystem::remove(it->path());
	}


	glc = SDL_GL_CreateContext(mainprogram->mainwindow);

	//glewExperimental = GL_TRUE;
	glewInit();

	mainprogram->dummywindow = SDL_CreateWindow("", 0, 0, 32, 32, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN | SDL_WINDOW_ALLOW_HIGHDPI);
	mainprogram->config_midipresetswindow = SDL_CreateWindow("Tune MIDI", glob->w / 4, glob->h / 4, glob->w / 2, glob->h / 2, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN | SDL_WINDOW_ALLOW_HIGHDPI);
	mainprogram->prefwindow = SDL_CreateWindow("Preferences", glob->w / 4, glob->h / 4, glob->w / 2, glob->h / 2, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN | SDL_WINDOW_ALLOW_HIGHDPI);
	SDL_GL_GetDrawableSize(mainprogram->prefwindow, &wi, &he);
	smw = (float)wi;
	smh = (float)he;
	
	glc_tm = SDL_GL_CreateContext(mainprogram->config_midipresetswindow);
	SDL_GL_MakeCurrent(mainprogram->config_midipresetswindow, glc_tm);
	mainprogram->ShaderProgram_tm = mainprogram->set_shader();
	glUseProgram(mainprogram->ShaderProgram_tm);
	glGenTextures(1, &mainprogram->smglobfbotex_tm);
	glBindTexture(GL_TEXTURE_2D, mainprogram->smglobfbotex_tm);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, smw, smh);
	glGenFramebuffers(1, &mainprogram->smglobfbo_tm);
	glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->smglobfbo_tm);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mainprogram->smglobfbotex_tm, 0);
	glc_pr = SDL_GL_CreateContext(mainprogram->prefwindow);
	SDL_GL_MakeCurrent(mainprogram->prefwindow, glc_pr);
	mainprogram->ShaderProgram_pr = mainprogram->set_shader();
	glUseProgram(mainprogram->ShaderProgram_pr);
	glGenTextures(1, &mainprogram->smglobfbotex_pr);
	glBindTexture(GL_TEXTURE_2D, mainprogram->smglobfbotex_pr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, smw, smh);
	glGenFramebuffers(1, &mainprogram->smglobfbo_pr);
	glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->smglobfbo_pr);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mainprogram->smglobfbotex_pr, 0);

	SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
	#ifdef _WIN64
	HGLRC c1 = wglGetCurrentContext();
	glc_th = SDL_GL_CreateContext(mainprogram->dummywindow);
	SDL_GL_MakeCurrent(mainprogram->dummywindow, glc_th);
	HGLRC c2 = wglGetCurrentContext();
	wglShareLists(c1, c2);
	#endif
	#ifdef __linux__
	GLXContext c1 = glXGetCurrentContext();
	static int	dblBuf[] = { GLX_RGBA,
					GLX_RED_SIZE, 1,
					GLX_GREEN_SIZE, 1,
					GLX_BLUE_SIZE, 1,
					GLX_DEPTH_SIZE, 12,
					GLX_DOUBLEBUFFER,
					None };
	Display* dpy = XOpenDisplay(NULL);
	XVisualInfo* vi = glXChooseVisual(dpy, DefaultScreen(dpy), dblBuf);
	GLXContext c2 = glXCreateContext(dpy, vi,
		c1,	/* sharing of display lists */
		True	/* direct rendering if possible */
	);
	#endif

	SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
	mainprogram->ShaderProgram = mainprogram->set_shader();
	glUseProgram(mainprogram->ShaderProgram);
	
	// mainscreen framebuffer target
	glGenTextures(1, &mainprogram->globfbotex);
	glBindTexture(GL_TEXTURE_2D, mainprogram->globfbotex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, glob->w, glob->h);
	glGenFramebuffers(1, &mainprogram->globfbo);
	glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mainprogram->globfbotex, 0);
	// depth buffer
	glGenTextures(1, &mainprogram->globdepthtex);
	glBindTexture(GL_TEXTURE_2D, mainprogram->globdepthtex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(
		GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, glob->w, glob->h, 0,
		GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL
	);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, mainprogram->globdepthtex, 0);

	mainprogram->shelves[0] = new Shelf(0);
	mainprogram->shelves[1] = new Shelf(1);
	mainprogram->shelves[0]->erase();
	mainprogram->shelves[1]->erase();
	mainprogram->cwbox->vtxcoords->w = glob->w / 5.0f;
	mainprogram->cwbox->vtxcoords->h = glob->h / 5.0f;
	mainprogram->cwbox->upvtxtoscr();


	if(FT_Init_FreeType(&ft)) {
	  fprintf(stderr, "Could not init freetype library\n");
	  return 1;
	}
  	FT_UInt interpreter_version = 40;
	FT_Property_Set(ft, "truetype", "interpreter-version", &interpreter_version);
	#ifdef _WIN64
	std::string fstr = mainprogram->fontpath + "/expressway.ttf";
	if (!exists(fstr)) mainprogram->quitting = "Can't find \"expressway.ttf\" TrueType font in current directory";
	#else
	#ifdef __linux__
	std::string fdir (mainprogram->fontdir);
	std::string fstr = fdir + "/expressway.ttf";
	if (!exists(fdir + "/expressway.ttf"))  mainprogram->quitting = "Can't find \"expressway.ttf\" TrueType font in " + fdir;
	#endif
	#endif
	if(FT_New_Face(ft, fstr.c_str(), 0, &face)) {
	  fprintf(stderr, "Could not open font\n");
	  return 1;
	}
	FT_Set_Pixel_Sizes(face, 0, 48);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

 	std::vector<std::string> effects;
 	effects.push_back("BLUR");
 	effects.push_back("BRIGHTNESS");
 	effects.push_back("CHROMAROTATE");
 	effects.push_back("CONTRAST");
 	effects.push_back("DOT");
 	effects.push_back("GLOW");
 	effects.push_back("RADIALBLUR");
 	effects.push_back("SATURATION");
 	effects.push_back("SCALE");
 	effects.push_back("SWIRL");
 	effects.push_back("OLDFILM");
 	effects.push_back("RIPPLE");
 	effects.push_back("FISHEYE");
 	effects.push_back("TRESHOLD");
 	effects.push_back("STROBE");
 	effects.push_back("POSTERIZE");
 	effects.push_back("PIXELATE");
 	effects.push_back("CROSSHATCH");
 	effects.push_back("INVERT");
 	effects.push_back("ROTATE");
 	effects.push_back("EMBOSS");
 	effects.push_back("ASCII");
 	effects.push_back("SOLARIZE");
 	effects.push_back("VARDOT");
 	effects.push_back("CRT");
 	effects.push_back("EDGEDETECT");
 	effects.push_back("KALEIDOSCOPE");
 	effects.push_back("HALFTONE");
 	effects.push_back("CARTOON");
 	effects.push_back("CUTOFF");
 	effects.push_back("GLITCH");
 	effects.push_back("COLORIZE");
  	effects.push_back("NOISE");
  	effects.push_back("GAMMA");
  	effects.push_back("THERMAL");
  	effects.push_back("BOKEH");
  	effects.push_back("SHARPEN");
  	effects.push_back("DITHER");
 	effects.push_back("FLIP");
	effects.push_back("MIRROR");
	effects.push_back("BOXBLUR");
	std::vector<std::string> meffects = effects;
 	std::sort(meffects.begin(), meffects.end());
 	for (int i = 0; i < meffects.size(); i++) {
 		for (int j = 0; j < effects.size(); j++) {
 			if (meffects[i] == effects[j]) {
 				mainprogram->abeffects.push_back((EFFECT_TYPE)j);
 				break;
 			}
 		}
 	}
 	meffects.insert(meffects.begin(), "Delete effect");
  	mainprogram->make_menu("effectmenu", mainprogram->effectmenu, meffects);
 	for (int i = 0; i < effects.size(); i++) {
 		mainprogram->effectsmap[(EFFECT_TYPE)(i - 1)] = effects[i];
  	}
 	
  	std::vector<std::string> mixengines;
 	mixengines.push_back("submenu mixmodemenu");
  	mixengines.push_back("Choose mixmode...");
  	mixengines.push_back("submenu wipemenu");
  	mixengines.push_back("Choose wipe...");
  	mainprogram->make_menu("mixenginemenu", mainprogram->mixenginemenu, mixengines);
 	
   	std::vector<std::string> mixmodes;
 	mixmodes.push_back("MIX");
 	mixmodes.push_back("MULTIPLY");
 	mixmodes.push_back("SCREEN");
 	mixmodes.push_back("OVERLAY");
 	mixmodes.push_back("HARD LIGHT");
 	mixmodes.push_back("SOFT LIGHT");
 	mixmodes.push_back("DIVIDE");
 	mixmodes.push_back("ADD");
 	mixmodes.push_back("SUBTRACT");
 	mixmodes.push_back("DIFFERENCE");
 	mixmodes.push_back("DODGE");
 	mixmodes.push_back("COLOR BURN");
 	mixmodes.push_back("LINEAR BURN");
 	mixmodes.push_back("VIVID LIGHT");
 	mixmodes.push_back("LINEAR LIGHT");
 	mixmodes.push_back("DARKEN ONLY");
 	mixmodes.push_back("LIGHTEN ONLY");
 	mixmodes.push_back("WIPE");
 	mixmodes.push_back("COLORKEY");
 	mixmodes.push_back("DISPLACEMENT");
  	mainprogram->make_menu("mixmodemenu", mainprogram->mixmodemenu, mixmodes);
 	
	std::vector<std::string> parammodes1;
	parammodes1.push_back("MIDI Learn");
	parammodes1.push_back("Reset to default");
 	mainprogram->make_menu("parammenu1", mainprogram->parammenu1, parammodes1);
 	
	std::vector<std::string> parammodes2;
	parammodes2.push_back("MIDI Learn");
	parammodes2.push_back("Remove automation");
	parammodes1.push_back("Reset to default");
	mainprogram->make_menu("parammenu2", mainprogram->parammenu2, parammodes2);

	std::vector<std::string> parammodes3;
	parammodes3.push_back("MIDI Learn");
	mainprogram->make_menu("parammenu3", mainprogram->parammenu3, parammodes3);

	std::vector<std::string> parammodes4;
	parammodes4.push_back("MIDI Learn");
	parammodes4.push_back("Remove automation");
	mainprogram->make_menu("parammenu4", mainprogram->parammenu4, parammodes4);

	std::vector<std::string> mixtargets;
 	mainprogram->make_menu("mixtargetmenu", mainprogram->mixtargetmenu, mixtargets);
 	
	std::vector<std::string> fullscreen;
	fullscreen.push_back("Exit fullscreen");
 	mainprogram->make_menu("fullscreenmenu", mainprogram->fullscreenmenu, fullscreen);
 	
	std::vector<std::string> livesources;
 	mainprogram->make_menu("livemenu", mainprogram->livemenu, livesources);
 	
 	std::vector<std::string> loopops;
 	loopops.push_back("Set loop start to current frame");
 	loopops.push_back("Set loop end to current frame");
 	loopops.push_back("Copy duration");
 	loopops.push_back("Paste duration (speed)");
 	loopops.push_back("Paste duration (loop length)");
 	mainprogram->make_menu("loopmenu", mainprogram->loopmenu, loopops);
 	mainprogram->loopmenu->width = 0.2f;
 	
 	std::vector<std::string> deckops;
 	deckops.push_back("Open deck");
	deckops.push_back("Save deck");
	deckops.push_back("MIDI Learn");
 	mainprogram->make_menu("deckmenu", mainprogram->deckmenu, deckops);

 	std::vector<std::string> aspectops;
  	aspectops.push_back("Same as output");
 	aspectops.push_back("Original inside");
 	aspectops.push_back("Original outside");
 	mainprogram->make_menu("aspectmenu", mainprogram->aspectmenu, aspectops);

 	std::vector<std::string> layops1;
 	layops1.push_back("submenu livemenu");
 	layops1.push_back("Connect live");
	layops1.push_back("Open file(s) before layer");
	layops1.push_back("Open file(s) into queue");
	layops1.push_back("Save layer");
 	layops1.push_back("New deck");
	layops1.push_back("Open deck");
	layops1.push_back("Save deck");
 	layops1.push_back("New mix");
 	layops1.push_back("Open mix");
	layops1.push_back("Save mix");
  	layops1.push_back("Delete layer");
  	layops1.push_back("Clone layer");
  	layops1.push_back("Center image");
 	layops1.push_back("submenu aspectmenu");
	layops1.push_back("Aspect ratio");
	mainprogram->make_menu("laymenu1", mainprogram->laymenu1, layops1);

 	std::vector<std::string> layops2;
 	layops2.push_back("submenu livemenu");
 	layops2.push_back("Connect live");
	layops2.push_back("Open file(s) before layer");
	layops2.push_back("Open file(s) into queue");
	layops2.push_back("Save layer");
 	layops2.push_back("New deck");
	layops2.push_back("Open deck");
	layops2.push_back("Save deck");
 	layops2.push_back("New mix");
 	layops2.push_back("Open mix");
	layops2.push_back("Save mix");
  	layops2.push_back("Delete layer");
  	layops2.push_back("Center image");
 	layops2.push_back("submenu aspectmenu");
 	layops2.push_back("Aspect ratio");
 	mainprogram->make_menu("laymenu2", mainprogram->laymenu2, layops2);

	std::vector<std::string> loadops;
	loadops.push_back("submenu livemenu");
	loadops.push_back("Connect live");
	loadops.push_back("Open file(s) into layers");
	loadops.push_back("Open file(s) into queue");
	loadops.push_back("New deck");
	loadops.push_back("Open deck");
	loadops.push_back("Save deck");
	loadops.push_back("New mix");
	loadops.push_back("Open mix");
	loadops.push_back("Save mix");
	mainprogram->make_menu("newlaymenu", mainprogram->newlaymenu, loadops);

	std::vector<std::string> clipops;
	clipops.push_back("submenu livemenu");
	clipops.push_back("Connect live");
	clipops.push_back("Open file(s)");
	clipops.push_back("Delete clip");
	mainprogram->make_menu("clipmenu", mainprogram->clipmenu, clipops);

 	std::vector<std::string> wipes;
 	wipes.push_back("CROSSFADE");
 	wipes.push_back("submenu dir1menu");
 	wipes.push_back("CLASSIC");
 	wipes.push_back("submenu dir1menu");
  	wipes.push_back("PUSH/PULL");
 	wipes.push_back("submenu dir1menu");
 	wipes.push_back("SQUASHED");
 	wipes.push_back("submenu dir2menu");
  	wipes.push_back("ELLIPSE");
 	wipes.push_back("submenu dir2menu");
 	wipes.push_back("RECTANGLE");
 	wipes.push_back("submenu dir2menu");
  	wipes.push_back("ZOOMED RECTANGLE");
 	wipes.push_back("submenu dir4menu");
 	wipes.push_back("CLOCK");
 	wipes.push_back("submenu dir2menu");
  	wipes.push_back("DOUBLE CLOCK");
 	wipes.push_back("submenu dir3menu");
 	wipes.push_back("BARS");
 	wipes.push_back("submenu dir3menu");
  	wipes.push_back("PATTERN");
 	wipes.push_back("submenu dir2menu");
  	wipes.push_back("REPEL");
 	mainprogram->make_menu("wipemenu", mainprogram->wipemenu, wipes);
 	int count = 0;
 	for (int i = 0; i < wipes.size(); i++) {
 		if (wipes[i].find("submenu") != std::string::npos) {
 			mainprogram->wipesmap[wipes[i]] = count;
 			count++;
 		}
 	}

 	std::vector<std::string> direction1;
 	direction1.push_back("Left->Right");
  	direction1.push_back("Right->Left");
  	direction1.push_back("Up->Down");
  	direction1.push_back("Down->Up");
 	mainprogram->make_menu("dir1menu", mainprogram->dir1menu, direction1);

 	std::vector<std::string> direction2;
  	direction2.push_back("In->Out");
  	direction2.push_back("Out->In");
  	mainprogram->make_menu("dir2menu", mainprogram->dir2menu, direction2);

 	std::vector<std::string> direction3;
 	direction3.push_back("Left->Right");
  	direction3.push_back("Right->Left");
  	direction3.push_back("LeftRight/InOut");
  	direction3.push_back("Up->Down");
  	direction3.push_back("Down->Up");
  	direction3.push_back("UpDown/InOut");
 	mainprogram->make_menu("dir3menu", mainprogram->dir3menu, direction3);

 	std::vector<std::string> direction4;
 	direction4.push_back("Up/Right->LeftUp");
  	direction4.push_back("Up/Left->Right");
  	direction4.push_back("Right/Down->Up");
  	direction4.push_back("Right/Up->Down");
 	direction4.push_back("Down/Left->Right");
  	direction4.push_back("Down/Right->Left");
  	direction4.push_back("Left/Up->Down");
  	direction4.push_back("Left/Down->Up");
 	mainprogram->make_menu("dir4menu", mainprogram->dir4menu, direction4);

 	std::vector<std::string> binel;
  	mainprogram->make_menu("binelmenu", mainprogram->binelmenu, binel);
  	
 	std::vector<std::string> bin;
  	bin.push_back("Delete bin");
  	bin.push_back("Rename bin");
  	mainprogram->make_menu("binmenu", mainprogram->binmenu, bin);

	std::vector<std::string> bin2;
	bin2.push_back("Rename bin");
	mainprogram->make_menu("bin2menu", mainprogram->bin2menu, bin2);

	std::vector<std::string> binsel;
	binsel.push_back("Delete elements");
	binsel.push_back("Move elements");
	mainprogram->make_menu("binselmenu", mainprogram->binselmenu, binsel);

	std::vector<std::string> generic;
  	generic.push_back("New project");
  	generic.push_back("Open project");
	generic.push_back("Save project");
	generic.push_back("Save project as");
	generic.push_back("New state");
  	generic.push_back("Open state");
  	generic.push_back("Save state");
  	generic.push_back("Preferences");
	generic.push_back("Configure general MIDI");
	generic.push_back("Quit");
  	mainprogram->make_menu("mainmenu", mainprogram->mainmenu, generic);

 	std::vector<std::string> shelf1;
  	shelf1.push_back("New shelf");
  	shelf1.push_back("Open shelf");
  	shelf1.push_back("Save shelf");
  	shelf1.push_back("Open file(s)");
	shelf1.push_back("Insert deck A");
	shelf1.push_back("Insert deck B");
	shelf1.push_back("Insert full mix");
	shelf1.push_back("Insert in bin");
	shelf1.push_back("MIDI Learn");
  	mainprogram->make_menu("shelfmenu", mainprogram->shelfmenu, shelf1);

	std::vector<std::string> file;
	file.push_back("submenu filedomenu");
	file.push_back("New");
	file.push_back("submenu filedomenu");
	file.push_back("Open");
	file.push_back("submenu filedomenu");
	file.push_back("Save");
	file.push_back("Quit");
	mainprogram->make_menu("filemenu", mainprogram->filemenu, file);

	std::vector<std::string> laylist1;
	mainprogram->make_menu("laylistmenu1", mainprogram->laylistmenu1, laylist1);

	std::vector<std::string> laylist2;
	mainprogram->make_menu("laylistmenu2", mainprogram->laylistmenu2, laylist2);

	std::vector<std::string> filedo;
	filedo.push_back("Project");
	filedo.push_back("State");
	filedo.push_back("Mix");
	filedo.push_back("Deck A");
	filedo.push_back("Deck B");
	filedo.push_back("submenu laylistmenu1");
	filedo.push_back("Files into layers in deck A before");
	filedo.push_back("submenu laylistmenu2");
	filedo.push_back("Files into layers in deck B before");
	filedo.push_back("submenu laylistmenu1");
	filedo.push_back("Files into queue in deck A on");
	filedo.push_back("submenu laylistmenu2");
	filedo.push_back("Files into queue in deck B on");
	mainprogram->make_menu("filedomenu", mainprogram->filedomenu, filedo);

	std::vector<std::string> edit;
	edit.push_back("Preferences");
	edit.push_back("Configure general MIDI");
	mainprogram->make_menu("editmenu", mainprogram->editmenu, edit);

	//make menu item names bitmaps
	for (int i = 0; i < mainprogram->menulist.size(); i++) {
		for (int j = 0; j < mainprogram->menulist[i]->entries.size(); j++) {
			render_text(mainprogram->menulist[i]->entries[j], white, 2.0f, 2.0f, tf(0.0003f), tf(0.0005f));
		}
	}



	mainprogram->nodesmain = new NodesMain;
	mainprogram->nodesmain->add_nodepages(8);
	mainprogram->nodesmain->currpage = mainprogram->nodesmain->pages[0];
	mainprogram->toscreen->box->upvtxtoscr();
	mainprogram->backtopre->box->upvtxtoscr();
	mainprogram->modusbut->box->upvtxtoscr();
	
	
	mainprogram->prefs = new Preferences;
	mainprogram->prefs->load();
	mainprogram->oldow = mainprogram->ow;
	mainprogram->oldoh = mainprogram->oh;

	GLint Sampler0 = glGetUniformLocation(mainprogram->ShaderProgram, "Sampler0");
	glUniform1i(Sampler0, 0);
	
	mainprogram->loadlay = new Layer(true);
	bool comp = !mainprogram->prevmodus;
	for (int m = 0; m < 2; m++) {
		std::vector<Layer*> &lvec = choose_layers(m);
		for (int i = 0; i < 4; i++) {
			mainmix->currscene[comp][m] = i;
			Layer *lay = mainmix->add_layer(lvec, 0);
			lay->closethread = true;
			mainmix->scenes[comp][m][mainmix->currscene[comp][m]]->nblayers.insert(mainmix->scenes[comp][m][mainmix->currscene[comp][m]]->nblayers.begin(), lay);
			mainmix->scenes[comp][m][mainmix->currscene[comp][m]]->nbframes.insert(mainmix->scenes[comp][m][mainmix->currscene[comp][m]]->nbframes.begin(), lay->frame);
			lay->clips.clear();
			mainmix->mousedeck = m;
			mainmix->do_save_deck(mainprogram->temppath + "tempdeck_0" + std::to_string(m) + std::to_string(i) + ".deck", false, false);
			SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
			mainmix->do_save_deck(mainprogram->temppath + "tempdeck_1" + std::to_string(m) + std::to_string(i) + ".deck", false, false);
			SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
			lvec.clear();
			lay->filename = "";
		}
	}
	mainprogram->nodesmain->currpage->nodes.clear();
	mainprogram->nodesmain->currpage->nodescomp.clear();

	GLint endSampler0 = glGetUniformLocation(mainprogram->ShaderProgram, "endSampler0");
	glUniform1i(endSampler0, 1);
	GLint endSampler1 = glGetUniformLocation(mainprogram->ShaderProgram, "endSampler1");
	glUniform1i(endSampler1, 2);

	MixNode *mixnodeA = mainprogram->nodesmain->currpage->add_mixnode(0, false);
	mixnodeA->outputbox->vtxcoords->x1 = -0.6f;
	mixnodeA->outputbox->vtxcoords->y1 = -1.0f;
	mixnodeA->outputbox->vtxcoords->w = 0.3f;
	mixnodeA->outputbox->vtxcoords->h = 0.3f;
	mixnodeA->outputbox->upvtxtoscr();
	MixNode *mixnodeB = mainprogram->nodesmain->currpage->add_mixnode(0, false);
	mixnodeB->outputbox->vtxcoords->x1 = 0.3f;
	mixnodeB->outputbox->vtxcoords->y1 = -1.0f;
	mixnodeB->outputbox->vtxcoords->w = 0.3f;
	mixnodeB->outputbox->vtxcoords->h = 0.3f;
	mixnodeB->outputbox->upvtxtoscr();
	MixNode *mixnodeAB = mainprogram->nodesmain->currpage->add_mixnode(0, false);
	mixnodeAB->outputbox->vtxcoords->x1 = -0.3f;
	mixnodeAB->outputbox->vtxcoords->y1 = -1.0f;
	mixnodeAB->outputbox->vtxcoords->w = 0.6f;
	mixnodeAB->outputbox->vtxcoords->h = 0.6f;
	mixnodeAB->outputbox->upvtxtoscr();
	mainprogram->bnodeend = mainprogram->nodesmain->currpage->add_blendnode(CROSSFADING, false);
	mainprogram->nodesmain->currpage->connect_nodes(mixnodeA, mixnodeB, mainprogram->bnodeend);
	mainprogram->nodesmain->currpage->connect_nodes(mainprogram->bnodeend, mixnodeAB);
	
	mainmix->currscene[!mainprogram->prevmodus][0] = 0;
	mainmix->mousedeck = 0;
	mainmix->open_deck(mainprogram->temppath + "tempdeck_000.deck", 1);
	mainmix->currscene[!mainprogram->prevmodus][1] = 0;
	mainmix->mousedeck = 1;
	mainmix->open_deck(mainprogram->temppath + "tempdeck_010.deck", 1);
	Layer *layA1 = mainmix->layersA[0];
	Layer *layB1 = mainmix->layersB[0];
	mainprogram->nodesmain->currpage->connect_nodes(layA1->node, mixnodeA);
	mainprogram->nodesmain->currpage->connect_nodes(layB1->node, mixnodeB);
	mainmix->currlay = mainmix->layersA[0];
	make_layboxes();

	//temporary
	layA1->lasteffnode[0]->monitor = 0;
	mainprogram->nodesmain->linked = true;
	
	laymidiA = new LayMidi;
	laymidiB = new LayMidi;
	laymidiC = new LayMidi;
	laymidiD = new LayMidi;
	if (exists(mainprogram->docpath + "/midiset.gm")) open_genmidis(mainprogram->docpath + "midiset.gm");
	
	mainmix->copy_to_comp(mainmix->layersA, mainmix->layersAcomp, mainmix->layersB, mainmix->layersBcomp, mainprogram->nodesmain->currpage->nodes, mainprogram->nodesmain->currpage->nodescomp, mainprogram->nodesmain->mixnodescomp, true);
	GLint preff = glGetUniformLocation(mainprogram->ShaderProgram, "preff");
	glUniform1i(preff, 1);
	
	// load background graphic
	glGenTextures(1, &mainprogram->bgtex);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mainprogram->bgtex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	#ifdef _WIN64
	open_thumb("./background.jpg", mainprogram->bgtex);
	#endif
	#ifdef __linux__
	open_thumb(mainprogram->homedir + "background.jpg", mainprogram->bgtex);
	#endif

	// get number of cores
	#ifdef _WIN64
	typedef BOOL (WINAPI *LPFN_GLPI)(
		PSYSTEM_LOGICAL_PROCESSOR_INFORMATION, 
		PDWORD);
	
	LPFN_GLPI glpi;
	BOOL done = FALSE;
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = nullptr;
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = nullptr;
	DWORD returnLength = 0;
	DWORD byteOffset = 0;

	glpi = (LPFN_GLPI) GetProcAddress(
							GetModuleHandle(TEXT("kernel32")),
							"GetLogicalProcessorInformation");

	
	while (!done)
    {
        DWORD rc2 = glpi(buffer, &returnLength);

        if (FALSE == rc2) 
        {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) 
            {
                if (buffer) 
                    free(buffer);

                buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(
                        returnLength);

                if (nullptr == buffer) 
                {
                    printf(TEXT("\nError: Allocation failure\n"));
                    return (2);
                }
            } 
            else 
            {
                printf(TEXT("\nError %d\n"), GetLastError());
                return (3);
            }
        } 
        else
        {
            done = TRUE;
        }
    }
    ptr = buffer;

    while (byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnLength) 
    {
        if (ptr->Relationship == RelationProcessorCore) mainprogram->numcores++;
        byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
        ptr++;
    }
    free(buffer);
    #endif
    
    #ifdef __linux__
    mainprogram->numcores = sysconf(_SC_NPROCESSORS_ONLN);
    #endif


	// OSC
	mainprogram->st = new lo::ServerThread (9000);
	mainprogram->add_main_oscmethods();
	mainprogram->st->start();
	
	
   	io_service io;
    deadline_timer t1(io);
    Deadline d(t1);
    CancelDeadline cd(d);
    std::thread thr1(cd);
    thr1.detach();
   	deadline_timer t2(io);
    Deadline2 d2(t2);
    CancelDeadline2 cd2(d2);
    std::thread thr2(cd2);
    thr2.detach();

	set_fbo();
	
	std::chrono::high_resolution_clock::time_point begintime = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed;
	
	#ifdef _WIN64
	std::string dir = mainprogram->docpath;
	#endif
	#ifdef __linux__
	std::string dir = homedir + "/.ewocvj2/";
	#endif
	if (exists(dir + "recentprojectslist")) {
		std::ifstream rfile;
		rfile.open(dir + "recentprojectslist");
		std::string istring;
		getline(rfile, istring);
		while (getline(rfile, istring)) {
			if (istring == "ENDOFFILE") break;
			mainprogram->recentprojectpaths.push_back(istring);
		}
	}
	if (exists(mainprogram->autosavedir + "autosavelist")) {
		std::ifstream rfile;
		rfile.open(mainprogram->autosavedir + "autosavelist");
		std::string istring;
		getline(rfile, istring);
		while (getline(rfile, istring)) {
			if (istring == "ENDOFFILE") break;
			mainprogram->autosavelist.push_back(istring);
		}
	}

	#ifdef _WIN64
	//SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);  reminder: what with priorities?
	#endif



	while (!quit){

		io.poll();
		
		if (mainprogram->path != "") {
			if (mainprogram->pathto == "OPENFILESLAYERS") {
				std::vector<Layer*>& lvec = choose_layers(mainmix->mousedeck);
				std::string str(mainprogram->paths[0]);
				if (mainmix->addlay) {
					mainmix->addlay = false;
					if (lvec.size() == 1 and mainprogram->loadlay->filename == "") {
						mainprogram->loadlay = lvec[0];
					}
					else mainprogram->loadlay = mainmix->add_layer(lvec, lvec.size());
					mainmix->currlay = mainprogram->loadlay;
				}
				else mainprogram->loadlay = mainmix->add_layer(lvec, mainprogram->loadlay->pos);
				mainprogram->currfilesdir = dirname(str);
				mainprogram->filescount = 0;
				mainprogram->fileslay = mainprogram->loadlay;
				mainprogram->openfileslayers = true;
			}
			else if (mainprogram->pathto == "OPENFILESQUEUE") {
				std::string str(mainprogram->paths[0]);
				if (mainmix->addlay) {
					mainmix->addlay = false;
					std::vector<Layer*>& lvec = choose_layers(mainmix->mousedeck);
					mainprogram->loadlay = mainmix->add_layer(lvec, lvec.size());
					mainmix->currlay = mainprogram->loadlay;
				}
				mainprogram->currfilesdir = dirname(str);
				mainprogram->filescount = 0;
				mainprogram->fileslay = mainprogram->loadlay;
				mainprogram->fileslay->cliploading = true;
				mainprogram->openfilesqueue = true;
			}
			else if (mainprogram->pathto == "CLIPOPENFILES") {
				std::string str(mainprogram->paths[0]);
				mainprogram->currclipfilesdir = dirname(str);
				mainprogram->clipfilescount = 0;
				mainprogram->clipfilesclip = mainmix->mouseclip;
				mainprogram->clipfileslay = mainmix->mouselayer;
				mainprogram->clipfileslay->cliploading = true;
				mainprogram->openclipfiles = true;
			}
			else if (mainprogram->pathto == "OPENSHELF") {
				mainprogram->currshelfdir = dirname(mainprogram->path);
				mainprogram->project->save(mainprogram->project->path);
				mainmix->mouseshelf->open(mainprogram->path);
			}
			else if (mainprogram->pathto == "SAVESHELF") {
				mainprogram->currshelfdir = dirname(mainprogram->path);
				mainprogram->project->save(mainprogram->project->path);
				mainmix->mouseshelf->save(mainprogram->path);
			}
			else if (mainprogram->pathto == "OPENSHELFFILES") {
				if (mainprogram->paths.size() > 0) {
					mainprogram->currshelffilesdir = dirname(mainprogram->paths[0]);
					mainprogram->openshelffiles = true;
					mainprogram->shelffilescount = 0;
					mainprogram->shelffileselem = mainmix->mouseshelfelem;
				}
			}
			else if (mainprogram->pathto == "SAVESTATE") {
				mainprogram->currstatedir = dirname(mainprogram->path);
				mainmix->save_state(mainprogram->path, false);
			}
			else if (mainprogram->pathto == "SAVEMIX") {
				mainprogram->currfilesdir = dirname(mainprogram->path);
				mainmix->save_mix(mainprogram->path);
			}
			else if (mainprogram->pathto == "SAVEDECK") {
				mainprogram->currfilesdir = dirname(mainprogram->path);
				mainmix->save_deck(mainprogram->path);
			}
			else if (mainprogram->pathto == "OPENDECK") {
				mainprogram->currfilesdir = dirname(mainprogram->path);
				mainmix->open_deck(mainprogram->path, 1);
			}
			else if (mainprogram->pathto == "SAVELAYFILE") {
				mainprogram->currfilesdir = dirname(mainprogram->path);
				mainmix->save_layerfile(mainprogram->path, mainmix->mouselayer, 1, 0);
			}
			else if (mainprogram->pathto == "OPENLAYFILE") {
				mainprogram->currfilesdir = dirname(mainprogram->path);
				mainmix->open_layerfile(mainprogram->path, mainmix->mouselayer, 1, 1);
			}
			else if (mainprogram->pathto == "OPENSTATE") {
				mainprogram->currstatedir = dirname(mainprogram->path);
				mainmix->open_state(mainprogram->path);
			}
			else if (mainprogram->pathto == "OPENMIX") {
				mainprogram->currfilesdir = dirname(mainprogram->path);
				mainmix->open_mix(mainprogram->path, true);
			}
			else if (mainprogram->pathto == "OPENBINFILES") {
				binsmain->openbinfile = true;
			}
			else if (mainprogram->pathto == "OPENBIN") {
				if (mainprogram->paths.size() > 0) {
					mainprogram->currbinsdir = dirname(mainprogram->paths[0]);
					binsmain->importbins = true;
					binsmain->binscount = 0;
				}
			}
			else if (mainprogram->pathto == "CHOOSEDIR") {
				mainprogram->choosedir = mainprogram->path + "/";
				//std::string driveletter1 = str.substr(0, 1);
				//std::string abspath = boost::filesystem::canonical(mainprogram->docpath).string();
				//std::string driveletter2 = abspath.substr(0, 1);
				//if (driveletter1 == driveletter2) {
				//	mainprogram->choosedir = boost::filesystem::relative(str, mainprogram->docpath).string() + "/";
				//}
				//else {
				//	mainprogram->choosedir = str + "/";
				//}
			}
			else if (mainprogram->pathto == "NEWPROJECT") {
				mainprogram->currprojdir = dirname(mainprogram->path);
				mainmix->new_state();
				mainprogram->project->newp(mainprogram->path);
			}
			else if (mainprogram->pathto == "OPENPROJECT") {
				mainprogram->currprojdir = dirname(mainprogram->path);
				mainprogram->project->open(mainprogram->path);
			}
			else if (mainprogram->pathto == "SAVEPROJECT") {
				if (dirname(mainprogram->path) != "") {
					mainprogram->currprojdir = dirname(mainprogram->path);
					if (!exists(mainprogram->path)) {
						mainprogram->project->create_dirs(mainprogram->path);
					}
					else {
						mainprogram->project->delete_dirs();
						mainprogram->project->create_dirs(mainprogram->path);
					}
					for (int i = 0; i < binsmain->bins.size(); i++) {
						binsmain->save_bin(binsmain->bins[i]->path);
					}
					binsmain->save_binslist();
					mainprogram->project->save(mainprogram->path);
				}
			}
			mainprogram->path = "";
			mainprogram->pathto = "";
		}
		else mainprogram->blocking = false;
		
		bool focus = true;
		mainprogram->mousewheel = 0;
		SDL_Event e;
		while (SDL_PollEvent(&e)){
			//SDL_PumpEvents();
			if (mainprogram->renaming != EDIT_NONE) {
				std::string old = mainprogram->inputtext;
				int c1 = mainprogram->cursorpos1;
				int c2 = mainprogram->cursorpos2;
				if (c2 < c1) {
					std::swap(c1, c2);
				}
				if (e.type == SDL_TEXTINPUT) {
                    /* Add new text onto the end of our text */
					if (!((e.text.text[0] == 'c' || e.text.text[0] == 'C') && (e.text.text[0] == 'v' || e.text.text[0] == 'V') && SDL_GetModState() & KMOD_CTRL)) {
						if (c1 != -1) {
							mainprogram->cursorpos0 = c1;
						}
						else {
							c1 = mainprogram->cursorpos0;
							c2 = mainprogram->cursorpos0;
						}
						std::string part = "";
						if (c1 == c2) part = mainprogram->inputtext.substr(c2, mainprogram->inputtext.length() - c2);
						mainprogram->inputtext = mainprogram->inputtext.substr(0, c1) + e.text.text + part;
						mainprogram->cursorpos0++;
						mainprogram->cursorpos1 = -1;
						mainprogram->cursorpos2 = -1;
					}
                }
				if (e.type == SDL_KEYDOWN) {
					//Handle cursor left/right 
					if (e.key.keysym.sym == SDLK_LEFT and mainprogram->cursorpos0 > 0) {
						mainprogram->cursorpos0--;
					} 
					if (e.key.keysym.sym == SDLK_RIGHT and mainprogram->cursorpos0 < mainprogram->inputtext.length()) {
						mainprogram->cursorpos0++;
					} 
					//Handle backspace 
					if (e.key.keysym.sym == SDLK_BACKSPACE and mainprogram->inputtext.length() > 0) {
						if (c1 != -1) {
							mainprogram->cursorpos0 = c1;
						}
						else if (mainprogram->cursorpos0 != 0) {
							mainprogram->cursorpos0--;
							c1 = mainprogram->cursorpos0;
							c2 = mainprogram->cursorpos0 + 1;
						}
						if (!((c1 == -1) and mainprogram->cursorpos0 == 0)) {
							mainprogram->inputtext = mainprogram->inputtext.substr(0, c1) + mainprogram->inputtext.substr(c2, mainprogram->inputtext.length() - c2);
						}
						mainprogram->cursorpos1 = -1;
						mainprogram->cursorpos2 = -1;
					}
					//Handle delete
					if (e.key.keysym.sym == SDLK_DELETE and mainprogram->inputtext.length() > 0) {
						if (c1 != -1) {
							mainprogram->cursorpos0 = c1;
						}
						else if (mainprogram->cursorpos0 != mainprogram->inputtext.length()) {
							c1 = mainprogram->cursorpos0;
							c2 = mainprogram->cursorpos0 + 1;
						}
						if (!((c1 == -1) and mainprogram->cursorpos0 == mainprogram->inputtext.length())) {
							mainprogram->inputtext = mainprogram->inputtext.substr(0, c1) + mainprogram->inputtext.substr(c2, mainprogram->inputtext.length() - c2);
						}
						mainprogram->cursorpos1 = -1;
						mainprogram->cursorpos2 = -1;
					}
					//Handle cut
					else if (e.key.keysym.sym == SDLK_x and SDL_GetModState() & KMOD_CTRL) {
						if (mainprogram->cursorpos1 != -1) {
							SDL_SetClipboardText(mainprogram->inputtext.substr(c1, c2 - c1).c_str());
							if (c1 != -1) {
								mainprogram->cursorpos0 = c1;
							}
							else if (mainprogram->cursorpos0 != 0) {
								mainprogram->cursorpos0--;
								c1 = mainprogram->cursorpos0;
								c2 = mainprogram->cursorpos0 + 1;
							}
							mainprogram->inputtext = mainprogram->inputtext.substr(0, c1) + mainprogram->inputtext.substr(c2, mainprogram->inputtext.length() - c2);
							mainprogram->cursorpos1 = -1;
							mainprogram->cursorpos2 = -1;
						}
					}
					//Handle copy 
					else if (e.key.keysym.sym == SDLK_c and SDL_GetModState() & KMOD_CTRL) {
						if (c1 != -1) {
							SDL_SetClipboardText(mainprogram->inputtext.substr(c1, c2 - c1).c_str());
						}
					}
					//Handle paste 
					else if (e.key.keysym.sym == SDLK_v and SDL_GetModState() & KMOD_CTRL) { 
						if (c1 == -1) {
							c1 = mainprogram->cursorpos0;
							c2 = mainprogram->cursorpos0;
						}
						mainprogram->inputtext = mainprogram->inputtext.substr(0, c1) + SDL_GetClipboardText() + mainprogram->inputtext.substr(c2, mainprogram->inputtext.length() - c2);
						
						if (mainprogram->cursorpos1 != -1) {
							mainprogram->cursorpos0 = mainprogram->cursorpos1;
						}
						mainprogram->cursorpos1 = -1;
						mainprogram->cursorpos2 = -1;
					}
					//Handle return
					else if (e.key.keysym.sym == SDLK_RETURN or e.key.keysym.sym == SDLK_KP_ENTER) {
						if (mainprogram->renaming == EDIT_PARAM) {
							try {
								mainmix->adaptnumparam->value = std::stof(mainprogram->inputtext);
								if (mainmix->adaptnumparam->powertwo) mainmix->adaptnumparam->value = sqrt(mainmix->adaptnumparam->value);
								if (mainmix->adaptnumparam->value < mainmix->adaptnumparam->range[0]) {
									mainmix->adaptnumparam->value = mainmix->adaptnumparam->range[0];
								}
								if (mainmix->adaptnumparam->value > mainmix->adaptnumparam->range[1]) {
									mainmix->adaptnumparam->value = mainmix->adaptnumparam->range[1];
								}
							}
							catch (...) {
							}
							mainmix->adaptnumparam = nullptr;
						}
						mainprogram->renaming = EDIT_NONE;
						end_input();
						continue;
					}
					if (mainprogram->inputtext != old) {
						if (mainprogram->prefon) {
							GUIString* gs = mainprogram->prguitextmap[mainprogram->inputtext];
							if (gs) {
								for (int i = 0; i < gs->texturevec.size(); i++) {
									glDeleteTextures(1, &gs->texturevec[i]);
								}
								delete gs;
								mainprogram->prguitextmap.erase(mainprogram->inputtext);
							}
						}
						else if (!mainprogram->midipresets) {
							GUIString* gs = mainprogram->tmguitextmap[mainprogram->inputtext];
							if (gs) {
								for (int i = 0; i < gs->texturevec.size(); i++) {
									glDeleteTextures(1, &gs->texturevec[i]);
								}
							}
							delete gs;
							mainprogram->tmguitextmap.erase(mainprogram->inputtext);
						}
					}
				}
				if (mainprogram->renaming == EDIT_BINNAME) {
					binsmain->menubin->name = mainprogram->inputtext;
					std::string oldpath = mainprogram->project->binsdir + mainprogram->backupname;
					std::string newpath = mainprogram->project->binsdir + mainprogram->inputtext;
					if (exists(oldpath)) boost::filesystem::rename(oldpath, newpath);
					oldpath += ".bin";
					newpath += ".bin";
					if (exists(oldpath)) boost::filesystem::rename(oldpath, newpath);
					binsmain->save_binslist();
				}
				else if (mainprogram->renaming == EDIT_BINELEMNAME) {
					binsmain->renamingelem->name = mainprogram->inputtext;
				}
				else if (mainprogram->renaming == EDIT_PARAM) {
					int diff = mainprogram->inputtext.find(".") + 4 - mainprogram->inputtext.length();
					if (diff < 0) {
						mainprogram->inputtext = mainprogram->inputtext.substr(0, mainprogram->inputtext.find(".") + 4);
						if (mainprogram->cursorpos0 > mainprogram->inputtext.length()) {
							mainprogram->cursorpos0 = mainprogram->inputtext.length();
						}
					}
				}
			}
		
			//If user closes the window
			if (e.type == SDL_WINDOWEVENT) {
				switch (e.window.event) {
					case SDL_WINDOWEVENT_CLOSE:
						if (e.window.windowID == SDL_GetWindowID(mainprogram->mainwindow)) {
							quit = true;
						}
						if (mainprogram->config_midipresetswindow) {
							if (e.window.windowID == SDL_GetWindowID(mainprogram->config_midipresetswindow)) {
								mainprogram->midipresets = false;
								mainprogram->tmlearn = TM_NONE;
								mainprogram->drawnonce = false;
								SDL_HideWindow(mainprogram->config_midipresetswindow);
							}
						}
						if (mainprogram->prefwindow) {
							if (e.window.windowID == SDL_GetWindowID(mainprogram->prefwindow)) {
								mainprogram->prefon = false;
								mainprogram->drawnonce = false;
								SDL_HideWindow(mainprogram->prefwindow);
							}
						}
						for (int i = 0; i < mainprogram->mixwindows.size(); i++) {
							if (e.window.windowID == SDL_GetWindowID(mainprogram->mixwindows[i]->win)) {
								SDL_DestroyWindow(mainprogram->mixwindows[i]->win);
								mainprogram->mixwindows[i]->closethread = true;
								while (mainprogram->mixwindows[i]->closethread) {
									mainprogram->mixwindows[i]->syncnow = true;
									mainprogram->mixwindows[i]->sync.notify_one();
								}
								mainprogram->mixwindows.erase(mainprogram->mixwindows.begin() + i);
							}
						}
						break;
				}
			}
			
			//If user presses any key
			mainprogram->del = 0;
			if (mainprogram->renaming == EDIT_NONE) {
				if (e.type == SDL_KEYDOWN) {
					if (e.key.keysym.sym == SDLK_LCTRL or e.key.keysym.sym == SDLK_RCTRL) {
						mainprogram->ctrl = true;
					}
					if (mainprogram->ctrl) {
						if (e.key.keysym.sym == SDLK_s) {
							mainprogram->pathto = "SAVESTATE";
							std::thread filereq(&Program::get_outname, mainprogram, "Save state file", "application/ewocvj2-state", boost::filesystem::canonical(mainprogram->currstatedir).generic_string());
							filereq.detach();
						}
						if (e.key.keysym.sym == SDLK_o) {
							mainprogram->pathto = "OPENSTATE";
							std::thread filereq(&Program::get_inname, mainprogram, "Open state file", "application/ewocvj2-state", boost::filesystem::canonical(mainprogram->currstatedir).generic_string());
							filereq.detach();
						}
						if (e.key.keysym.sym == SDLK_n) {
							mainmix->new_file(2, 1);
						}
					}
					else {
						if (e.key.keysym.sym == SDLK_r) {
							// toggle record button for current loopstation element
							loopstation->currelem->recbut->value = !loopstation->currelem->recbut->value;
							loopstation->currelem->recbut->oldvalue = !loopstation->currelem->recbut->value;
						}
						else if (e.key.keysym.sym == SDLK_t) {
							// toggle loop button for current loopstation element
							loopstation->currelem->loopbut->value = !loopstation->currelem->loopbut->value;
							loopstation->currelem->loopbut->oldvalue = !loopstation->currelem->loopbut->value;
						}
						if (e.key.keysym.sym == SDLK_y) {
							// toggle "one shot play" button for current loopstation element
							loopstation->currelem->playbut->value = !loopstation->currelem->playbut->value;
							loopstation->currelem->playbut->oldvalue = !loopstation->currelem->playbut->value;
						}
					}
				}
				if (e.type == SDL_KEYUP) {
					if (e.key.keysym.sym == SDLK_LCTRL or e.key.keysym.sym == SDLK_RCTRL) {
						mainprogram->ctrl = false;
					}
					if (e.key.keysym.sym == SDLK_DELETE or e.key.keysym.sym == SDLK_BACKSPACE) {
						mainprogram->del = 1;
						if (mainmix->learn) {
							if (mainmix->learnparam) {
								mainmix->learnparam->midi[0] = -1;
								mainmix->learnparam->midi[1] = -1;
								mainmix->learnparam->midiport = "";
							}
							else if (mainmix->learnbutton) {
								mainmix->learnbutton->midi[0] = -1;
								mainmix->learnbutton->midi[1] = -1;
								mainmix->learnbutton->midiport = "";
							}
							mainmix->learn = false;
						}
						else mainprogram->del = 1;
					}
					if (e.key.keysym.sym == SDLK_ESCAPE) {
						mainprogram->fullscreen = -1;
						mainprogram->directmode = false;
					}
					else if (e.key.keysym.sym == SDLK_SPACE) {
						if (mainmix->currlay) {
							if (mainmix->currlay->playbut->value) {
								mainmix->currlay->playkind = 0;
								mainmix->currlay->playbut->value = false;
							}
							else if (mainmix->currlay->revbut->value) {
								mainmix->currlay->playkind = 1;
								mainmix->currlay->revbut->value = false;
							}
							else if (mainmix->currlay->bouncebut->value == 1) {
								mainmix->currlay->playkind = 2;
								mainmix->currlay->bouncebut->value = false;
							}
							else if (mainmix->currlay->bouncebut->value == 2) {
								mainmix->currlay->playkind = 3;
								mainmix->currlay->bouncebut->value = false;
							}
							else {
								if (mainmix->currlay->playkind == 0) {
									mainmix->currlay->playbut->value = true;
								}
								else if (mainmix->currlay->playkind == 1) {
									mainmix->currlay->revbut->value = true;
								}
								else if (mainmix->currlay->playkind == 2) {
									mainmix->currlay->bouncebut->value = 1;
								}
								else if (mainmix->currlay->playkind == 3) {
									mainmix->currlay->bouncebut->value = 2;
								}
							}
						}
					}
					else if (e.key.keysym.sym == SDLK_RIGHT) {
						if (mainmix->currlay) {
							mainmix->currlay->frame += 1;
							if (mainmix->currlay->frame >= mainmix->currlay->numf) mainmix->currlay->frame = 0;
						}
					}
					else if (e.key.keysym.sym == SDLK_LEFT) {
						if (mainmix->currlay) {
							mainmix->currlay->frame -= 1;
							if (mainmix->currlay->frame < 0) mainmix->currlay->frame = mainmix->currlay->numf - 1;
						}
					}
				}
			}

			if (focus and SDL_GetMouseFocus() != mainprogram->prefwindow and SDL_GetMouseFocus() != mainprogram->config_midipresetswindow) {
				SDL_GetMouseState(&mainprogram->mx, &mainprogram->my);
			}
			else {
				mainprogram->mx = -1;
				mainprogram->my = -1;
			}
			
			if (e.type == SDL_MULTIGESTURE)	{
 				if (fabs(e.mgesture.dDist) > 0.002) {
					mainprogram->mx = e.mgesture.x * glob->w; 
					mainprogram->my = e.mgesture.y * glob->h;
					for (int i = 0; i < 2; i++) {
						std::vector<Layer*> &lvec = choose_layers(i);
						for (int j = 0; j < lvec.size(); j++) {
							if (lvec[j]->node->vidbox->in()) {
								lvec[j]->scale->value *= 1 - e.mgesture.dDist * glob->w / 100; 
							}
						}
					}
				}
				SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
			}
			//If user moves the mouse
			else if (e.type == SDL_MOUSEMOTION){
				mainprogram->mx = e.motion.x;
				mainprogram->my = e.motion.y;
				mainprogram->oldmx = mainprogram->mx;
				mainprogram->oldmy = mainprogram->my;
				if (mainprogram->tooltipbox) {
					mainprogram->tooltipmilli = 0.0f;
					mainprogram->tooltipbox = nullptr;
				}
			}
			//If user clicks the mouse
			else if (e.type == SDL_MOUSEBUTTONDOWN){
				if (e.button.button == SDL_BUTTON_LEFT) {
					if (e.button.clicks == 2) mainprogram->doubleleftmouse = true;
					mainprogram->leftmousedown = true;
				}
				if (e.button.button == SDL_BUTTON_MIDDLE) {
					mainprogram->middlemousedown = true;
				}
				if (e.button.button == SDL_BUTTON_RIGHT) {
					mainprogram->rightmousedown = true;
				}
			}
			else if (e.type == SDL_MOUSEBUTTONUP){
				if (e.button.button == SDL_BUTTON_RIGHT and !mainmix->learn) {
					for (int i = 0; i < mainprogram->menulist.size(); i++) {
						mainprogram->menulist[i]->state = 0;
						mainprogram->menulist[i]->menux = mainprogram->mx;
						mainprogram->menulist[i]->menuy = mainprogram->my;
					}
					mainprogram->menuactivation = true;
				}
				if (e.button.button == SDL_BUTTON_LEFT) {
					//if (e.button.clicks == 2) mainprogram->doubleleftmouse = true;
					mainprogram->leftmouse = true;
					mainprogram->leftmousedown = false;
				}
				if (e.button.button == SDL_BUTTON_MIDDLE) {
					mainprogram->middlemouse = true;
				}
				if (e.button.button == SDL_BUTTON_RIGHT) {
					mainprogram->rightmouse = true;
					mainprogram->rightmousedown = false;
				}
			}
			else if (e.type == SDL_MOUSEWHEEL) {
				mainprogram->mousewheel = e.wheel.y;
			}
		}

		
		float white[] = {1.0f, 1.0f, 1.0f, 1.0f};
		float deepred[4] = {1.0, 0.0, 0.0, 1.0};
		float lightblue[] = {0.5f, 0.5f, 1.0f, 1.0f};
		glClearColor(0.2f, 0.2f, 0.2f, 0.2f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
		if (!mainprogram->startloop) {
			//initial switch to live mode
			mainprogram->prevmodus = false;
			mainprogram->preview_init();
			
			// prepare gathering of box data
			mainprogram->bdvptr[0] = mainprogram->bdcoords[0];
			mainprogram->bdtcptr[0] = mainprogram->bdtexcoords[0];
			mainprogram->bdcptr[0] = mainprogram->bdcolors[0];
			mainprogram->bdtptr[0] = mainprogram->bdtexes[0];
			mainprogram->bdtnptr[0] = mainprogram->boxtexes[0];
			mainprogram->countingtexes[0] = 0;
			mainprogram->boxoffset[0] = 0;
			mainprogram->currbatch = 0;

			std::string reqdir = mainprogram->currprojdir;
			if (reqdir.substr(0, 1) == ".") reqdir.erase(0, 2);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDrawBuffer(GL_BACK_LEFT);
			Box box;
			box.acolor[3] = 1.0f;
			box.vtxcoords->x1 = -0.75;
			box.vtxcoords->y1 = 0.0f;
			box.vtxcoords->w = 0.5f;
			box.vtxcoords->h = 0.25f;
			box.upvtxtoscr();
			draw_box(&box, -1);
			if (box.in()) {
				draw_box(white, lightblue, &box, -1);
				if (mainprogram->leftmouse) {
					//start new project
					std::string name = "Untitled";
					std::string path;
					count = 0;
					while (1) {
						path = mainprogram->projdir + name;
						if (!exists(path + ".ewocvj")) {
							break;
						}
						count++;
						name = remove_version(name) + "_" + std::to_string(count);
					}
					mainprogram->get_outname("New project", "application/ewocvj2-project", boost::filesystem::absolute(reqdir + "/" + name).generic_string());
					if (mainprogram->path != "") {
						mainprogram->project->newp(mainprogram->path);
						mainprogram->currprojdir = dirname(mainprogram->path);
						mainprogram->path = "";
						mainprogram->startloop = true;
						mainprogram->newproject = true;
					}
				}
			}
			render_text("New project", white, box.vtxcoords->x1 + 0.015f, box.vtxcoords->y1 + 0.15f, 0.001f, 0.0016f);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDrawBuffer(GL_BACK_LEFT);
			
			box.vtxcoords->y1 = -0.25f;
			box.upvtxtoscr();
			draw_box(&box, -1);
			if (box.in()) {
				draw_box(white, lightblue, &box, -1);
				if (mainprogram->leftmouse) {
					mainprogram->get_inname("Open project", "application/ewocvj2-project", boost::filesystem::canonical(mainprogram->currprojdir).generic_string());
					if (mainprogram->path != "") {
						mainprogram->project->open(mainprogram->path);
						mainprogram->currprojdir = dirname(mainprogram->path);
						mainprogram->path = "";
						mainprogram->startloop = true;
					}
				}
			}
			render_text("Open project", white, box.vtxcoords->x1 + 0.015f, box.vtxcoords->y1 + 0.15f, 0.001f, 0.0016f);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDrawBuffer(GL_BACK_LEFT);
			
			box.vtxcoords->x1 = 0.0f;
			box.vtxcoords->y1 = 0.5f;
			box.vtxcoords->w = 0.95f;
			box.vtxcoords->h = 0.125f;
			box.upvtxtoscr();
			
			render_text("Recent project files:", white, box.vtxcoords->x1 + 0.015f, box.vtxcoords->y1 + box.vtxcoords->h * 2.0f + 0.03f, 0.001f, 0.0016f);
			for (int i = 0; i < mainprogram->recentprojectpaths.size(); i++) {
				draw_box(&box, -1);
				if (box.in()) {
					draw_box(white, lightblue, &box, -1);
					if (mainprogram->leftmouse) {
						mainprogram->project->open(mainprogram->recentprojectpaths[i]);
						mainprogram->currprojdir = dirname(mainprogram->recentprojectpaths[i]);
						mainprogram->startloop = true;
						break;
					}
				}
				render_text(remove_extension(basename(mainprogram->recentprojectpaths[i])), white, box.vtxcoords->x1 + 0.015f, box.vtxcoords->y1 + 0.03f, 0.001f, 0.0016f);
				box.vtxcoords->y1 -= 0.125f;
				box.upvtxtoscr();
			}

			if (mainprogram->startloop) {
				SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
				glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
				mainprogram->drawbuffer = mainprogram->globfbo;
				glDrawBuffer(GL_COLOR_ATTACHMENT0);
			}
						
			// allow exiting with x icon during project setup
			draw_box(nullptr, deepred, 1.0f - 0.05f, 1.0f - tf(0.05f), 0.05f, tf(0.05f), -1);
			render_text("x", white, 0.966f, 1.019f - tf(0.05f), 0.0012f, 0.002f);
			if (mainprogram->my <= mainprogram->yvtxtoscr(tf(0.05f)) and mainprogram->mx > glob->w - mainprogram->xvtxtoscr(0.05f)) {
				if (mainprogram->leftmouse) {
					printf("stopped\n");
					SDL_Quit();
					exit(1);
				}
			}

			mainprogram->leftmouse = false;
			
			SDL_GL_SwapWindow(mainprogram->mainwindow);
		}
		else {
			// update global timer iGlobalTime, used by some effects shaders
			std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
			elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - begintime);
			long long microcount = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
			mainmix->oldtime = mainmix->time;
			mainmix->time = microcount / 1000000.0f;
			GLint iGlobalTime = glGetUniformLocation(mainprogram->ShaderProgram, "iGlobalTime");
			glUniform1f(iGlobalTime, mainmix->time);
			


			the_loop();  // main loop



			if (mainprogram->newproject) {
				mainprogram->newproject = false;
				mainprogram->project->open(mainprogram->project->path);
			}
		}
	}

	mainprogram->quitting = "stop";

}


