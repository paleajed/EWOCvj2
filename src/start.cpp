#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <cstdlib>

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
#include <dirent.h>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <time.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <sys/stat.h>
#ifndef UINT64_C
#define UINT64_C(c) (c ## ULL)
#endif

#ifdef _WIN64
#include <shobjidl.h>
#include <Vfw.h>
#define STRSAFE_NO_DEPRECATE
#include <initguid.h>
#include <dshow.h>
#pragma comment (lib, "strmiids.lib")
#pragma comment (lib, "Quartz.lib")
#pragma comment (lib, "ole32.lib")
#include <windows.h>
#include <ShellScalingAPI.h>
#include <comdef.h>
#endif

#include "RtMidi.h"
#include <jpeglib.h>
#define SDL_MAIN_HANDLED
#include "GL/glew.h"
#include "GL/gl.h"
#ifdef __linux__
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

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
#include "libswscale/swscale.h"
#include "libavutil/avutil.h"
#include "libavutil/pixfmt.h"
#include "libavutil/opt.h"
#include "libavutil/imgutils.h"
}

#include <ft2build.h>
#include FT_FREETYPE_H
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

Globals *glob = nullptr;
Program *mainprogram = nullptr;
Mixer *mainmix = nullptr;
BinsMain *binsmain = nullptr;
LoopStation *loopstation = nullptr;
LoopStation *lp = nullptr;
LoopStation *lpc = nullptr;

//TCHAR buf [MAX_PATH];
//int retgtp = Getmainprogram->temppath(MAX_PATH, buf);
//std::string mainprogram->temppath (buf);
static GLuint mixvao;
static GLuint fbotex[2];
static GLuint frbuf[2];
static GLuint vbuf2, vbuf3;
std::vector<GLuint> thumbtex;
static GLuint thmvbuf;
static GLuint thmvao;
std::vector<GLuint> thvao;
std::vector<GLuint> thbvao;
static GLuint boxvao;
FT_Face face;
std::vector<std::string> thpath;
std::vector<ELEM_TYPE> thtype;
float smw, smh;
SDL_GLContext glc;
SDL_GLContext glc_tm;
SDL_GLContext glc_pr;
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

std::string basename(std::string pathname)
{
	const size_t last_slash_idx = pathname.find_last_of("//");
	if (std::string::npos != last_slash_idx)
	{
		pathname.erase(0, last_slash_idx + 1);
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


#ifdef _WIN64
HRESULT EnumerateDevices(REFGUID category, IEnumMoniker **ppEnum)
{
    // Create the System Device Enumerator.
    ICreateDevEnum *pDevEnum;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL,  
        CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDevEnum));

    if (SUCCEEDED(hr))
    {
        // Create an enumerator for the category.
        hr = pDevEnum->CreateClassEnumerator(category, ppEnum, 0);
        if (hr == S_FALSE)
        {
            hr = VFW_E_NOT_FOUND;  // The category is empty. Treat as an error.
        }
        pDevEnum->Release();
    }
    return hr;
}

void DisplayDeviceInformation(IEnumMoniker *pEnum)
{
    IMoniker *pMoniker = NULL;
	mainprogram->livedevices.clear();
    
    while (pEnum->Next(1, &pMoniker, NULL) == S_OK)
    {
        IPropertyBag *pPropBag;
        HRESULT hr = pMoniker->BindToStorage(0, 0, IID_PPV_ARGS(&pPropBag));
        if (FAILED(hr))
        {
            pMoniker->Release();
            continue;  
        } 

        VARIANT var;
        VariantInit(&var);

        // Get description or friendly name.
        hr = pPropBag->Read(L"Description", &var, 0);
        if (FAILED(hr))
        {
            hr = pPropBag->Read(L"FriendlyName", &var, 0);
        }
        if (SUCCEEDED(hr))
        {
            printf("%S\n", var.bstrVal);
			std::wstring string(var.bstrVal);
            mainprogram->livedevices.push_back(string);
            VariantClear(&var); 
        }

        hr = pPropBag->Write(L"FriendlyName", &var);

        // WaveInID applies only to audio capture devices.
        hr = pPropBag->Read(L"WaveInID", &var, 0);
        if (SUCCEEDED(hr))
        {
            printf("WaveIn ID: %d\n", var.lVal);
            VariantClear(&var); 
        }

        hr = pPropBag->Read(L"DevicePath", &var, 0);
        if (SUCCEEDED(hr))
        {
            // The device path is not intended for display.
            printf("Device path: %S\n", var.bstrVal);
            VariantClear(&var); 
        }

        pPropBag->Release();
        pMoniker->Release();
    }
}
#endif

void get_cameras()
{
	#ifdef _WIN64
    HRESULT hr;
    if (1)
     {
       	IEnumMoniker *pEnum;

        hr = EnumerateDevices(CLSID_VideoInputDeviceCategory, &pEnum);
        if (SUCCEEDED(hr))
        {
            DisplayDeviceInformation(pEnum);
            pEnum->Release();
        }
        //hr = EnumerateDevices(CLSID_AudioInputDeviceCategory, &pEnum);
        //if (SUCCEEDED(hr))
        //{
            //DisplayDeviceInformation(pEnum);
            //pEnum->Release();
        //}
        //CoUninitialize();
    }
    #endif
    #ifdef __linux__
    mainprogram->livedevices.clear();
    std::map<std::string, std::wstring> map;
	boost::filesystem::path dir("/sys/class/video4linux");
	for (boost::filesystem::directory_iterator iter(dir), end; iter != end; ++iter) {
		std::ifstream name;
		name.open(iter->path().string() + "/name");
		std::string istring;
		getline(name, istring);
		istring = istring.substr(0, istring.find(":"));
		std::wstring wstr (istring.begin(), istring.end());
		map["/dev/" + basename(iter->path().string())] = wstr;
	}
	std::map<std::string, std::wstring>::iterator it;
    for (it = map.begin(); it != map.end(); it++) {
		struct v4l2_capability cap;
		int fd = open(it->first.c_str(), O_RDONLY);
		ioctl(fd, VIDIOC_QUERYCAP, &cap);
		if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) mainprogram->livedevices.push_back(it->second);
		close(fd);
    }
    #endif
}

void set_live_base(Layer *lay, std::string livename) {
	lay->audioplaying = false;
	lay->live = true;
	lay->filename = livename;
	if (lay->video) {
		std::unique_lock<std::mutex> lock(lay->endlock);
		lay->enddecode.wait(lock, [&]{return lay->processed;});
		lock.unlock();
		lay->processed = false;
		avformat_close_input(&lay->video);
	}
	avdevice_register_all();
	ptrdiff_t pos = std::find(mainprogram->busylist.begin(), mainprogram->busylist.end(), livename) - mainprogram->busylist.begin();
	if (pos >= mainprogram->busylist.size()) {
		mainprogram->busylist.push_back(lay->filename);
		mainprogram->busylayers.push_back(lay);
		#ifdef _WIN64
		AVInputFormat *ifmt = av_find_input_format("dshow");
		#else
		#ifdef __linux__
		AVInputFormat *ifmt = av_find_input_format("v4l2");
		#endif
		#endif
		thread_vidopen(lay, ifmt, false);
	}
	else {
		lay->liveinput = mainprogram->busylayers[pos];
		mainprogram->mimiclayers.push_back(lay);
	}
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
			#define BUFFER_OFFSET(i) ((char *)NULL + (i))
			glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, mainmix->ioBuf);
			glBufferData(GL_PIXEL_PACK_BUFFER_ARB, (int)(mainprogram->ow * mainprogram->oh) * 4, NULL, GL_DYNAMIC_READ);
			glBindFramebuffer(GL_FRAMEBUFFER, ((MixNode*)mainprogram->nodesmain->mixnodescomp[2])->mixfbo);
			glReadBuffer(GL_COLOR_ATTACHMENT0);
			glReadPixels(0, 0, mainprogram->ow, (int)mainprogram->oh, GL_RGBA, GL_UNSIGNED_BYTE, BUFFER_OFFSET(0));
			mainmix->rgbdata = glMapBuffer(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY);
			assert(mainmix->rgbdata);
			mainmix->recordnow = true;
			mainmix->startrecord.notify_one();
			glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
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
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	
	std::string name = "./screenshots/screenshot" + std::to_string(sscount) + ".jpg";
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
				if (lvec[j]->frame > lvec[j]->numf - 1) lvec[j]->frame = 0;
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
  	std::string midiport = ((PMidiItem*)userData)->name;
  	
 	if (mainprogram->waitmidi == 0 and mainprogram->tmlearn) {
    	mainprogram->stt = clock();
    	mainprogram->savedmessage = *message;
    	mainprogram->savedmidiitem = (PMidiItem*)userData;
    	mainprogram->waitmidi = 1;
    	return;
    }
    if (mainprogram->waitmidi == 1) {
     	mainprogram->savedmessage = *message;
    	mainprogram->savedmidiitem = (PMidiItem*)userData;
   		return;
   	}
  	
  	if (mainprogram->tunemidi) {
  		LayMidi *lm;
  		if (mainprogram->tunemidideck == 1) lm = laymidiA;
  		else if (mainprogram->tunemidideck == 2) lm = laymidiB;
  		else if (mainprogram->tunemidideck == 3) lm = laymidiC;
  		else if (mainprogram->tunemidideck == 4) lm = laymidiD;
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
  		if (midi0 == 176 and mainmix->learnparam->sliding) {
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
		else if (midi0 == 144 and !mainmix->learnparam->sliding) {
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
		return;
  	}
  	if (midi0 == 176) {
		Param *par;
		if (mainprogram->preveff) par = mainmix->crossfade;
		else par = mainmix->crossfadecomp;
		if (midi0 == par->midi[0] and midi1 == par->midi[1] and midiport == par->midiport) {
			mainmix->midi2 = midi2;
			mainmix->midiparam = par;
		}
		
		for (int i = 0; i < mainprogram->nodesmain->currpage->nodes.size(); i++) {
			if (mainprogram->nodesmain->currpage->nodes[i]->type == EFFECT) {
				EffectNode *effnode = (EffectNode*)mainprogram->nodesmain->currpage->nodes[i];
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
		for (int i = 0; i < mainprogram->nodesmain->currpage->nodes.size(); i++) {
			if (mainprogram->nodesmain->currpage->nodes[i]->type == VIDEO) {
				VideoNode *vnode = (VideoNode*)mainprogram->nodesmain->currpage->nodes[i]; 			
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
			if (mainprogram->nodesmain->currpage->nodes[i]->type == BLEND) {
				BlendNode *bnode = (BlendNode*)mainprogram->nodesmain->currpage->nodes[i]; 			
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

	std::cout << "message = " << (int)message->at(0) << std::endl;
	std::cout << "? = " << (int)message->at(1) << std::endl;
	std::cout << "value = " << (int)message->at(2) << std::endl;
	if ( nBytes > 0 )
    	int stamp = deltatime;
}



int encode_frame(AVFormatContext *fmtctx, AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt, FILE *outfile, int framenr) {
    int ret;
    /* send the frame to the encoder */
	AVPacket enc_pkt;
	enc_pkt.data = NULL;
	enc_pkt.size = 0;
	av_init_packet(&enc_pkt);
	int got_frame;
	if (outfile) ret = avcodec_encode_video2(enc_ctx, pkt, frame, &got_frame);
	else ret = avcodec_encode_video2(enc_ctx, &enc_pkt, frame, &got_frame);
	if (outfile) fwrite(pkt->data, 1, pkt->size, outfile);
	else {
		/* prepare packet for muxing */
		enc_pkt.stream_index = pkt->stream_index;
		enc_pkt.duration = (fmtctx->streams[enc_pkt.stream_index]->time_base.den * fmtctx->streams[enc_pkt.stream_index]->r_frame_rate.den) / (fmtctx->streams[enc_pkt.stream_index]->time_base.num * fmtctx->streams[enc_pkt.stream_index]->r_frame_rate.num);
		enc_pkt.dts = framenr * enc_pkt.duration;
		enc_pkt.pts = enc_pkt.dts;
		av_write_frame(fmtctx, &enc_pkt);
	}
   	av_packet_unref(pkt);
   	av_packet_unref(&enc_pkt);
   	return got_frame;
}

static int decode_packet(Layer *lay, int *got_frame)
{
    int ret = 0;
    int decoded = lay->decpkt.size;
    *got_frame = 0;
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
	//clock_t stt;
    //stt = clock();
		avcodec_send_packet(lay->video_dec_ctx, &lay->decpkt);
 		int err = avcodec_receive_frame(lay->video_dec_ctx, lay->decframe);
 		//if (err == AVERROR_EOF) lay->numf--;
   //clock_t t = clock() - stt;
    //double time_taken = ((double)t)/CLOCKS_PER_SEC; // in seconds
	//printf("decodetime1 %f\n", time_taken);
        if (ret < 0) {
            fprintf(stderr, "Error decoding video frame (%s)\n", 0);
            return ret;
        }
        if (1) {
            /* copy decoded frame to destination buffer:
             * this is required since rawvideo expects non aligned data */
			sws_scale
			(
				lay->sws_ctx,
				(uint8_t const * const *)lay->decframe->data,
				lay->decframe->linesize,
				0,
				lay->video_dec_ctx->height,
				lay->rgbframe->data,
				lay->rgbframe->linesize
			);
			lay->decresult->data = (char*)*(lay->rgbframe->extended_data);
			lay->decresult->height = lay->video_dec_ctx->height;
			lay->decresult->width = lay->video_dec_ctx->width;
       }
    } 
    else if (lay->decpkt.stream_index == lay->audio_stream_idx) {
        /* decode audio frame */
        ret = avcodec_decode_audio4(lay->audio_dec_ctx, lay->decframe, got_frame, &lay->decpkt);
        if (ret < 0) {
            fprintf(stderr, "Error decoding audio frame (%s)\n", 0);
            return ret;
        }
        /* Some audio decoders decode only part of the packet, and have to be
         * called again with the remainder of the packet data.
         * Sample: fate-suite/lossless-audio/luckynight-partial.shn
         * Also, some decoders might over-read the packet. */
        decoded = FFMIN(ret, lay->decpkt.size);
    }
    
 	//av_frame_unref(lay->decframe);
 	
	return decoded;
}


int open_codec_context(int *stream_idx,
                              AVFormatContext *video, enum AVMediaType type)
{
    int ret;
    AVStream *st;
    AVCodecContext *dec_ctx = NULL;
    AVCodec *dec = NULL;
    AVDictionary *opts = NULL;
    ret = av_find_best_stream(video, type, -1, -1, NULL, 0);
    if (ret <0) {
    	printf("%s\n", " cant find stream");
    	return -1;
    }
	*stream_idx = ret;
	st = video->streams[*stream_idx];
	/* find decoder for the stream */
	dec_ctx = st->codec;
    dec_ctx->refcounted_frames = false;
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
    *fmt = NULL;
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
	int *snippet = NULL;
	int *snip = NULL;
	size_t unpadded_linesize;
    av_packet_unref(&lay->decpkt);
	av_frame_unref(lay->decframe);
	av_read_frame(lay->video, &lay->decpkt);
	while (lay->decpkt.stream_index != lay->video_stream_idx) {
		if (!lay->dummy and lay->volume->value > 0.0f) {
			do {
				ret = decode_packet(lay, &got_frame);
				lay->decpkt.data += ret;
				lay->decpkt.size -= ret;
			} while (lay->decpkt.size > 0);
			// flush
			//lay->decpkt.data = NULL;
			//lay->decpkt.size = 0;
			//decode_packet(lay, &got_frame);
			if (ret >= 0 ) {
				unpadded_linesize = lay->decframe->nb_samples * av_get_bytes_per_sample((AVSampleFormat)lay->decframe->format);
				snip = (int*)malloc(unpadded_linesize);
				snippet = snip;
				memcpy(snippet, lay->decframe->extended_data[0], unpadded_linesize);
				if ((lay->playbut->value and lay->speed->value < 0) or (lay->revbut->value and lay->speed->value > 0)) {
					snippet = (int*)malloc(unpadded_linesize);
					for (int add = 0; add < unpadded_linesize / av_get_bytes_per_sample((AVSampleFormat)lay->decframe->format); add++) {
						snippet[add] = snip[add];
					}
					free(snip);
				}
				lay->pslen = unpadded_linesize;
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
		lay->newchunk.notify_one();
	}
}	
	
void get_frame_other(Layer *lay, int framenr, int prevframe, int errcount)
{
   	int ret = 0, got_frame;
	/* initialize packet, set data to NULL, let the demuxer fill it */
	av_init_packet(&lay->decpkt);
	lay->decpkt.data = NULL;
	lay->decpkt.size = 0;
	
	if (!lay->live) {
		if (lay->numf == 0) return;
		int64_t seekTarget = ((lay->video_stream->duration * (framenr)) / lay->numf);
		if (framenr != 0) {
			if (framenr != prevframe + 1) {
				avformat_seek_file(lay->video, lay->video_stream->index, 0, seekTarget, seekTarget, 0);
			}
			else {
				avformat_seek_file(lay->video, lay->video_stream->index, 0, seekTarget, seekTarget, AVSEEK_FLAG_ANY);
			}
		}

    /* flush cached frames */
//    lay->decpkt.data = NULL;
//    lay->decpkt.size = 0;
//    do {
//        decode_packet(&got_frame, 1);
//    } while (got_frame);
    
		decode_audio(lay);
		int readpos = (lay->decpkt.dts * lay->numf) / lay->video_stream->duration;
		int count = readpos;
		if (framenr != 0) {
			if (framenr != prevframe + 1) {
				if (readpos < framenr) {
					if (framenr > prevframe and prevframe > readpos) {
						readpos = prevframe + 1;
						int64_t seekTarget2 = ((lay->video_stream->duration * (prevframe + 1)) / lay->numf);
						avformat_seek_file(lay->video, lay->video_stream->index, 0, seekTarget2, seekTarget2, AVSEEK_FLAG_ANY);
						do {
							av_packet_unref(&lay->decpkt);
							av_read_frame(lay->video, &lay->decpkt);
						} while (lay->decpkt.stream_index != lay->video_stream_idx);
						int pos = (lay->decpkt.dts * lay->numf) / lay->video_stream->duration;
					}
					for (int f = readpos; f < framenr; f++) {
						avcodec_send_packet(lay->video_dec_ctx, &lay->decpkt);
						avcodec_receive_frame(lay->video_dec_ctx, lay->decframe);
						do {
							av_packet_unref(&lay->decpkt);
							av_read_frame(lay->video, &lay->decpkt);
						} while (lay->decpkt.stream_index != lay->video_stream_idx);
						int pos = (lay->decpkt.dts * lay->numf) / lay->video_stream->duration;
					}
				}
			}
		}
		else {
			av_seek_frame(lay->video, lay->video_stream->index, 0, AVSEEK_FLAG_BACKWARD);
    		av_packet_unref(&lay->decpkt);
			av_read_frame(lay->video, &lay->decpkt);
		}
		decode_packet(lay, &got_frame);
		lay->prevframe = framenr;
		lay->dataformat = lay->vidformat;
		lay->pktloaded = true;
		if (lay->decframe->width == 0) {
			lay->prevframe = framenr;
			if ((lay->speed->value > 0 and (lay->playbut->value or lay->bouncebut->value == 1)) or (lay->speed->value < 0 and (lay->revbut->value or lay->bouncebut->value == 2))) {
				framenr++;
			}
			else if ((lay->speed->value > 0 and (lay->revbut->value or lay->bouncebut->value == 2)) or (lay->speed->value < 0 and (lay->playbut->value or lay->bouncebut->value == 1))) {
				framenr--;
			}
			else if (lay->prevfbw) {
				//lay->prevfbw = false;
				framenr--;
			}
			else framenr++;	
			if (framenr > lay->endframe) framenr = lay->startframe;
			else if (framenr < lay->startframe) framenr = lay->endframe;
			avcodec_flush_buffers(lay->video_dec_ctx);
			errcount++;
			if (errcount == 10) {
				lay->openerr = true;
				return;
			}
			get_frame_other(lay, framenr, lay->prevframe, errcount);
		}
		decode_audio(lay);
		av_packet_unref(&lay->decpkt);
	}
	else {
		av_frame_unref(lay->decframe);
		int r = av_read_frame(lay->video, &lay->decpkt);
	 	if (r >= 0) decode_packet(lay, &got_frame);
		av_packet_unref(&lay->decpkt);
	}
		
    
    if (lay->audio_stream and 0) {
        enum AVSampleFormat sfmt = lay->audio_dec_ctx->sample_fmt;
        int n_channels = lay->audio_dec_ctx->channels;
        const char *fmt;
        if (av_sample_fmt_is_planar(sfmt)) {
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


void Layer::decode_frame() {

	this->numf = this->video_stream->nb_frames;
	if (this->numf == 0) return;	//implement!
	float tbperframe = (float)this->video_stream->duration / (float)this->numf;
	this->millif = tbperframe * (((float)this->video_stream->time_base.num * 1000.0) / (float)this->video_stream->time_base.den);
	int64_t seekTarget = ((this->video_stream->duration * this->frame) / this->numf);
	int r = av_seek_frame(this->video, this->video_stream->index, seekTarget, AVSEEK_FLAG_ANY);
	av_init_packet(&this->decpkt);
	av_frame_unref(this->decframe);
    r = av_read_frame(this->video, &this->decpkt);
 	this->prevframe = (int)this->frame;

	char *bptrData = (char*)(&this->decpkt)->data;
	size_t size = (&this->decpkt)->size;
	if (size == 0) return;
	size_t uncomp = 40000000;
	int headerl = 4;
	if (*bptrData == 0 and *(bptrData + 1) == 0 and *(bptrData + 2) == 0) {
		headerl = 8;
	}
	this->decresult->compression = *(bptrData + 3);

	int st = snappy_uncompress(bptrData + headerl, size - headerl, this->databuf, &uncomp);
    av_packet_unref(&this->decpkt);
	if (st) this->decresult->data = NULL;
	else this->decresult->data = this->databuf;
	this->decresult->height = this->video_dec_ctx->height;
	this->decresult->width = this->video_dec_ctx->width;
	this->decresult->size = uncomp;

}

void Layer::get_frame(){

	float speed = 1.0;
	int frnums[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

	while (1) {
		this->building = false;
		std::unique_lock<std::mutex> lock(this->startlock);
		this->startdecode.wait(lock, [&]{return this->ready;});
		lock.unlock();
		this->ready = false;
		if (this->closethread) {
			this->closethread = false;
			//delete this;  implement with layer delete vector?
			return;
		}
		if (this->vidopen) {
			if (this->video) {
				std::unique_lock<std::mutex> lock(this->endlock);
				this->enddecode.wait(lock, [&]{return this->processed;});
				this->processed = false;
				lock.unlock();
				avformat_close_input(&this->video);
			}
			bool r = thread_vidopen(this, nullptr, false);
			this->vidopen = false;
			if (r == 0) {
				printf("load error\n");
				this->openerr = true;
				this->processed = true;
				this->enddecode.notify_one();
				continue;
			}
		}
		if (this->liveinput == nullptr) {
			if (!this->video_stream) continue;
			if (this->vidformat != 188 and this->vidformat != 187) {
				if ((int)(this->frame) != this->prevframe) {
					get_frame_other(this, (int)this->frame, this->prevframe, 0);
				}
				this->processed = true;
				this->enddecode.notify_one();
				if (mainmix->firststage) {
					if (std::find(mainmix->layersAcomp.begin(), mainmix->layersAcomp.end(), this) != mainmix->layersAcomp.end()) {
						mainmix->compon = true;
					}
					if (std::find(mainmix->layersBcomp.begin(), mainmix->layersBcomp.end(), this) != mainmix->layersBcomp.end()) {
						mainmix->compon = true;
					}
				}
				continue;
			}
			if ((int)(this->frame) != this->prevframe) {
				this->decode_frame();
			}
			if (mainmix->firststage) {
				if (std::find(mainmix->layersAcomp.begin(), mainmix->layersAcomp.end(), this) != mainmix->layersAcomp.end()) {
					mainmix->compon = true;
				}
				if (std::find(mainmix->layersBcomp.begin(), mainmix->layersBcomp.end(), this) != mainmix->layersBcomp.end()) {
					mainmix->compon = true;
				}
			}
		}
		this->processed = true;
		this->enddecode.notify_one();
		this->dataformat = this->vidformat;
		continue;
	}
}

void open_video(float frame, Layer *lay, const std::string &filename, int reset) {
	lay->audioplaying = false;
	lay->live = false;
	lay->filename = filename;
	lay->frame = frame;
	lay->prevframe = lay->frame - 1;
	lay->vidopen = true;
	lay->reset = reset;
}

bool thread_vidopen(Layer *lay, AVInputFormat *ifmt, bool skip) {
	lay->video = nullptr;
	lay->liveinput = nullptr;
		
	if (!skip) {
		bool foundnew = false;
		ptrdiff_t pos = std::find(mainprogram->busylayers.begin(), mainprogram->busylayers.end(), lay) - mainprogram->busylayers.begin();
		if (pos < mainprogram->busylayers.size()) {
			int size = mainprogram->mimiclayers.size();
			for (int i = 0; i < size; i++) {
				Layer *mlay = mainprogram->mimiclayers[i];
				if (mlay->liveinput == lay) {
					mlay->liveinput = nullptr;
					mainprogram->mimiclayers.erase(mainprogram->mimiclayers.begin() + i);
					mainprogram->busylayers[pos] = mlay;
					#ifdef _WIN64
					AVInputFormat *ifmt = av_find_input_format("dshow");
					#else
					#ifdef __linux__
					AVInputFormat *ifmt = av_find_input_format("v4l2");
					#endif
					#endif
					thread_vidopen(mlay, ifmt, true);
					for (int j = 0; j < mainprogram->mimiclayers.size(); j++) {
						if (mainprogram->mimiclayers[j]->liveinput == lay) {
							mainprogram->mimiclayers[j]->liveinput == mlay;
						}
					}
					foundnew = true;
					break;
				}
			}
			if (!foundnew and lay->filename != mainprogram->busylist[pos]) {
				mainprogram->busylayers.erase(mainprogram->busylayers.begin() + pos);
				mainprogram->busylist.erase(mainprogram->busylist.begin() + pos);
			}
		}
		pos = std::find(mainprogram->mimiclayers.begin(), mainprogram->mimiclayers.end(), lay) - mainprogram->mimiclayers.begin();
		if (pos < mainprogram->mimiclayers.size()) {
			mainprogram->mimiclayers.erase(mainprogram->mimiclayers.begin() + pos);
		}
	}
	
	int r = avformat_open_input(&(lay->video), lay->filename.c_str(), ifmt, NULL);
	printf("loading... %s\n", lay->filename.c_str());
	if (r < 0) {
		lay->filename = "";
		printf("%s\n", "Couldnt open video");
		return 0;
	}

    if (avformat_find_stream_info(lay->video, nullptr) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        exit(1);
    }

   if (open_codec_context(&(lay->video_stream_idx), lay->video, AVMEDIA_TYPE_VIDEO) >= 0) {
     	lay->video_stream = lay->video->streams[lay->video_stream_idx];
        lay->video_dec_ctx = lay->video_stream->codec;
        lay->vidformat = lay->video_dec_ctx->codec_id;
        if (lay->vidformat == 188 or lay->vidformat == 187) {
        	if (lay->databuf) free(lay->databuf);
			lay->databuf = (char*)malloc(lay->video_dec_ctx->width * lay->video_dec_ctx->height);
			lay->numf = lay->video_stream->nb_frames;
			if (lay->reset) {
				lay->startframe = 0;
				lay->endframe = lay->numf - 1;
				lay->frame = (lay->numf - 1) * (lay->reset - 1);
			}
			if (std::find(mainmix->layersAcomp.begin(), mainmix->layersAcomp.end(), lay) != mainmix->layersAcomp.end()) {
				mainmix->firststage = true;
			}
			if (std::find(mainmix->layersBcomp.begin(), mainmix->layersBcomp.end(), lay) != mainmix->layersBcomp.end()) {
				mainmix->firststage = true;
			}
			return 1;
        }
  	}
    else {
    	printf("Error2\n");
    	return 0;
    }

    if (lay->rgbframe) {
    	//avcodec_close(lay->video_dec_ctx);
    	//avcodec_close(lay->audio_dec_ctx);
	    av_frame_free(&lay->decframe);
	    av_frame_free(&lay->rgbframe);
    }

 	lay->numf = lay->video_stream->nb_frames;
	if (lay->reset) {
		lay->startframe = 0;
		lay->endframe = lay->numf - 1;
	}
	//if (lay->numf == 0) return 0;	//implement!
	float tbperframe = (float)lay->video_stream->duration / (float)lay->numf;
	lay->millif = tbperframe * (((float)lay->video_stream->time_base.num * 1000.0) / (float)lay->video_stream->time_base.den);
	float waitmilli = lay->millif;  // fix this
		

	if (open_codec_context(&(lay->audio_stream_idx), lay->video, AVMEDIA_TYPE_AUDIO) >= 0 and !lay->dummy) {
		lay->audio_stream = lay->video->streams[lay->audio_stream_idx];
		lay->audio_dec_ctx = lay->audio_stream->codec;
		lay->sample_rate = lay->audio_dec_ctx->sample_rate;
		lay->channels = lay->audio_dec_ctx->channels;
		lay->channels = 1;
		switch (lay->audio_dec_ctx->sample_fmt) {
			case AV_SAMPLE_FMT_U8:
				if (lay->channels == 1) lay->sampleformat = AL_FORMAT_MONO8;
				else lay->sampleformat = AL_FORMAT_STEREO8;
				break;
			case AV_SAMPLE_FMT_S16:
				if (lay->channels == 1) lay->sampleformat = AL_FORMAT_MONO16;
				else lay->sampleformat = AL_FORMAT_STEREO16;
				break;
			case AV_SAMPLE_FMT_S32:
				if (lay->channels == 1) lay->sampleformat = AL_FORMAT_MONO_FLOAT32;
				else lay->sampleformat = AL_FORMAT_STEREO_FLOAT32;
				break;
			case AV_SAMPLE_FMT_FLT:
				if (lay->channels == 1) lay->sampleformat = AL_FORMAT_MONO_FLOAT32;
				else lay->sampleformat = AL_FORMAT_STEREO_FLOAT32;
				break;
			case AV_SAMPLE_FMT_U8P:
				if (lay->channels == 1) lay->sampleformat = AL_FORMAT_MONO8;
				else lay->sampleformat = AL_FORMAT_STEREO8;
				break;
			case AV_SAMPLE_FMT_S16P:
				if (lay->channels == 1) lay->sampleformat = AL_FORMAT_MONO16;
				else lay->sampleformat = AL_FORMAT_STEREO16;
				break;
			case AV_SAMPLE_FMT_S32P:
				if (lay->channels == 1) lay->sampleformat = AL_FORMAT_MONO_FLOAT32;
				else lay->sampleformat = AL_FORMAT_STEREO_FLOAT32;
				break;
			case AV_SAMPLE_FMT_FLTP:
				if (lay->channels == 1) lay->sampleformat = AL_FORMAT_MONO_FLOAT32;
				else lay->sampleformat = AL_FORMAT_STEREO_FLOAT32;
				break;
		}
		
		lay->audioplaying = true;
    	lay->audiot = std::thread{&Layer::playaudio, lay};
    	lay->audiot.detach();
    }

    lay->rgbframe = av_frame_alloc();
    lay->rgbframe->format = AV_PIX_FMT_RGBA;
    lay->rgbframe->width  = lay->video_dec_ctx->width;
    lay->rgbframe->height = lay->video_dec_ctx->height;
	int storage = av_image_alloc(lay->rgbframe->data, lay->rgbframe->linesize, lay->rgbframe->width, lay->rgbframe->height, AV_PIX_FMT_RGBA, 1);
  	lay->sws_ctx = sws_getContext
    (
        lay->video_dec_ctx->width,
       	lay->video_dec_ctx->height,
        lay->video_dec_ctx->pix_fmt,
        lay->video_dec_ctx->width,
        lay->video_dec_ctx->height,
        AV_PIX_FMT_RGBA,
        SWS_BILINEAR,
        NULL,
        NULL,
        NULL);

    lay->decframe = av_frame_alloc();
	
	lay->pktloaded = false;

	if (std::find(mainmix->layersAcomp.begin(), mainmix->layersAcomp.end(), lay) != mainmix->layersAcomp.end()) {
		mainmix->firststage = true;
	}
	if (std::find(mainmix->layersBcomp.begin(), mainmix->layersBcomp.end(), lay) != mainmix->layersBcomp.end()) {
		mainmix->firststage = true;
	}
				
    return 1;
}


void Layer::playaudio() {
	
	ALint availBuffers = 0; // Buffers to be recovered
	ALint buffqueued = 0;
	ALuint myBuff; // The buffer we're using
	ALuint buffHolder[8]; // An array to hold catch the unqueued buffers
	bool done = false;
	std::list<ALuint> bufferQueue; // A quick and dirty queue of buffer objects
	ALuint temp[8];
	alGenBuffers(8, &temp[0]);
    // Queue our buffers onto an STL list
	for (int ii = 0; ii < 8; ++ii) {
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
		//std::unique_lock<std::mutex> lock(this->chunklock);
		//this->newchunk.wait(lock, [&]{return this->chready;});
		//lock.unlock();
		//this->chready = false;
		
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
				int *input = this->snippets.front(); this->snippets.pop_front();
				if (!bufferQueue.empty()) { // We just drop the data if no buffers are available
					myBuff = bufferQueue.front(); bufferQueue.pop_front();
					alBufferData(myBuff, this->sampleformat, input, this->pslen, this->sample_rate);
					// Queue the buffer
					alSourceQueueBuffers(source[0], 1, &myBuff);
				}
				free(input);
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
			alSourcef(source[0], AL_PITCH, this->speed->value);	
		}
	}
}


float tf(float vtxcoord) {
	return 1.5f * vtxcoord;
}


void set_thumbs() {
	float boxwidth = 0.1f;
	GLfloat vcoords[2][16][8];
	for(int y = 0; y < 4; y++) {
		for(int x = 0; x < 4; x++) {
			GLfloat *p = vcoords[0][(3 - y) * 4 + x];
			*p++ = -1.0f + (float)x * boxwidth;
			*p++ = -1.0f + (float)y * boxwidth;
			*p++ = -1.0f + (float)(x + 1) * boxwidth;
			*p++ = -1.0f + (float)y * boxwidth;
			*p++ = -1.0f + (float)x * boxwidth;
			*p++ = -1.0f + (float)(y + 1) * boxwidth;
			*p++ = -1.0f + (float)(x + 1)* boxwidth;
			*p++ = -1.0f + (float)(y + 1) * boxwidth;
		}
	}
	for(int y = 0; y < 4; y++) {
		for(int x = 0; x < 4; x++) {
			GLfloat *p = vcoords[1][(3 - y) * 4 + x];
			*p++ = 1.0f + (float)x * boxwidth - boxwidth * 4;
			*p++ = -1.0f + (float)y * boxwidth;
			*p++ = 1.0f + (float)(x + 1) * boxwidth - boxwidth * 4;
			*p++ = -1.0f + (float)y * boxwidth;
			*p++ = 1.0f + (float)x * boxwidth - boxwidth * 4;
			*p++ = -1.0f + (float)(y + 1) * boxwidth;
			*p++ = 1.0f + (float)(x + 1)* boxwidth - boxwidth * 4;
			*p++ = -1.0f + (float)(y + 1) * boxwidth;
		}
	}
	GLfloat vbcoords[2][16][8];
	for(int y = 0; y < 4; y++) {
		for(int x = 0; x < 4; x++) {
			GLfloat *p = vbcoords[0][(3 - y) * 4 + x];
			*p++ = -1.0f + (float)x * boxwidth;
			*p++ = -1.0f + (float)y * boxwidth;
			*p++ = -1.0f + (float)(x + 1) * boxwidth;
			*p++ = -1.0f + (float)y * boxwidth;
			*p++ = -1.0f + (float)(x + 1)* boxwidth;
			*p++ = -1.0f + (float)(y + 1) * boxwidth;
			*p++ = -1.0f + (float)x * boxwidth;
			*p++ = -1.0f + (float)(y + 1) * boxwidth;
		}
	}
	for(int y = 0; y < 4; y++) {
		for(int x = 0; x < 4; x++) {
			GLfloat *p = vbcoords[1][(3 - y) * 4 + x];
			*p++ = 1.0f + (float)x * boxwidth - boxwidth * 4;
			*p++ = -1.0f + (float)y * boxwidth;
			*p++ = 1.0f + (float)(x + 1) * boxwidth - boxwidth * 4;
			*p++ = -1.0f + (float)y * boxwidth;
			*p++ = 1.0f + (float)(x + 1)* boxwidth - boxwidth * 4;
			*p++ = -1.0f + (float)(y + 1) * boxwidth;
			*p++ = 1.0f + (float)x * boxwidth - boxwidth * 4;
			*p++ = -1.0f + (float)(y + 1) * boxwidth;
		}
	}
	for(int i = 0; i < 32; i++) {
		thpath.push_back("");
		thtype.push_back(ELEM_FILE);
		thumbtex.push_back(0);
		glGenTextures(1, &thumbtex.back());
		glBindTexture(GL_TEXTURE_2D, thumbtex.back());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		GLfloat tcoords[8];
		GLfloat *p = tcoords;
		*p++ = 0.0f; *p++ = 1.0f;
		*p++ = 1.0f; *p++ = 1.0f;
		*p++ = 0.0f; *p++ = 0.0f;
		*p++ = 1.0f; *p++ = 0.0f;

		GLuint vbuf;
		glGenBuffers(1, &vbuf);
		glBindBuffer(GL_ARRAY_BUFFER, vbuf);
		glBufferData(GL_ARRAY_BUFFER, 32, vcoords[i > 15][i - 16 * (i > 15)], GL_STATIC_DRAW);
		GLuint tbuf;
		glGenBuffers(1, &tbuf);
		glBindBuffer(GL_ARRAY_BUFFER, tbuf);
		glBufferData(GL_ARRAY_BUFFER, 32, tcoords, GL_STATIC_DRAW);
		thvao.push_back(0);
		glGenVertexArrays(1, &thvao.back());
		glBindVertexArray(thvao.back());
		glBindBuffer(GL_ARRAY_BUFFER, vbuf);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, NULL);
		glBindBuffer(GL_ARRAY_BUFFER, tbuf);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8, NULL);
		glBindVertexArray(thvao.back());

		GLuint vbbuf;
		glGenBuffers(1, &vbbuf);
		glBindBuffer(GL_ARRAY_BUFFER, vbbuf);
		glBufferData(GL_ARRAY_BUFFER, 32, vbcoords[i > 15][i - 16 * (i > 15)], GL_STATIC_DRAW);
		thbvao.push_back(0);
		glGenVertexArrays(1, &thbvao.back());
		glBindVertexArray(thbvao.back());
		glBindBuffer(GL_ARRAY_BUFFER, vbbuf);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, NULL);
		glBindVertexArray(thbvao.back());

		GLuint tmbuf;
		glGenBuffers(1, &thmvbuf);
		glBindBuffer(GL_ARRAY_BUFFER, thmvbuf);
		glBufferData(GL_ARRAY_BUFFER, 32, vcoords[i > 15][i - 16 * (i > 15)], GL_STREAM_DRAW);
		glGenBuffers(1, &tmbuf);
		glBindBuffer(GL_ARRAY_BUFFER, tmbuf);
		glBufferData(GL_ARRAY_BUFFER, 32, tcoords, GL_STATIC_DRAW);
		glGenVertexArrays(1, &thmvao);
		glBindVertexArray(thmvao);
		glBindBuffer(GL_ARRAY_BUFFER, thmvbuf);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, NULL);
		glBindBuffer(GL_ARRAY_BUFFER, tmbuf);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8, NULL);
		glBindVertexArray(thmvao);
	}
}


void set_fbovao2() {
	for (int i = 0; i < 1; i++) { //mainprogram->nodesmain->pages.size()
		for (int j = 0; j < mainprogram->nodesmain->currpage->nodescomp.size(); j++) {
			Node *node = mainprogram->nodesmain->currpage->nodescomp[j];
			if (node->type == EFFECT) {
				EffectNode *enode = (EffectNode*)node;
				glDeleteFramebuffers(1, &enode->effect->fbo);
				enode->effect->fbo = -1;
				glDeleteTextures(1, &enode->effect->fbotex);
			}
			else if (node->type == BLEND) {
				BlendNode *bnode = (BlendNode*)node;
				glDeleteFramebuffers(1, &bnode->fbo);
				bnode->fbo = -1;
				glDeleteTextures(1, &bnode->fbotex);
			}
			else if (node->type == MIX) {
				MixNode *mnode = (MixNode*)node;
				glDeleteFramebuffers(1, &mnode->mixfbo);
				mnode->mixfbo = -1;
				glDeleteTextures(1, &mnode->mixtex);
			}
		}
	}

	float vidwidth = mainprogram->ow;
	float vidheight = mainprogram->oh;
	
	GLfloat vcoords3[8];
	GLfloat *p = vcoords3;
	*p++ = -1.0f; *p++ = -1.0f;
	*p++ = -1.0f; *p++ = (1.0f - (2.0f - 2.0f * (vidheight / (float)glob->h)));
	*p++ = 1.0f - (2.0f - 2.0f * (vidwidth / (float)glob->w)); *p++ = -1.0f;
	*p++ = 1.0f - (2.0f - 2.0f * (vidwidth / (float)glob->w)); *p++ = (1.0f - (2.0f - 2.0f * (vidheight / (float)glob->h)));
	GLfloat tcoords[] = {0.0f, 0.0f,
						0.0f, 1.0f,
						1.0f, 0.0f,
						1.0f, 1.0f};

	glBindBuffer(GL_ARRAY_BUFFER, vbuf3);
    glBufferData(GL_ARRAY_BUFFER, 32, vcoords3, GL_STATIC_DRAW);
	GLuint tbuf;
    glGenBuffers(1, &tbuf);
	glBindBuffer(GL_ARRAY_BUFFER, tbuf);
    glBufferData(GL_ARRAY_BUFFER, 32, tcoords, GL_STATIC_DRAW);
    
	glBindVertexArray(mainprogram->fbovao[2]);
	glBindBuffer(GL_ARRAY_BUFFER, vbuf3);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, NULL);
	glBindBuffer(GL_ARRAY_BUFFER, tbuf);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8, NULL);
}	

void set_fbo() {

    glGenTextures(1, &mainmix->mixbackuptex);
    glBindTexture(GL_TEXTURE_2D, mainmix->mixbackuptex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, glob->w, glob->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenTextures(1, &fbotex[0]);
    glGenTextures(1, &fbotex[1]);
	glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, fbotex[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, glob->w, glob->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, fbotex[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, glob->w, glob->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glGenFramebuffers(1, &frbuf[0]);
	glGenFramebuffers(1, &frbuf[1]);
	glBindFramebuffer(GL_FRAMEBUFFER, frbuf[0]);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbotex[0], 0);
	glBindFramebuffer(GL_FRAMEBUFFER, frbuf[1]);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbotex[1], 0);

	
	float vidwidth = mainprogram->ow;
	float vidheight = mainprogram->oh;
	
	GLfloat vcoords1[8];
	GLfloat *p = vcoords1;
	*p++ = -1.0f; *p++ = -1.0f;
	*p++ = -1.0f; *p++ = 1.0f;
	*p++ = 1.0f; *p++ = -1.0f;
	*p++ = 1.0f; *p++ = 1.0f;

	GLfloat vcoords2[8];
	p = vcoords2;
	*p++ = -1.0f; *p++ = -1.0f;
	*p++ = -1.0f; *p++ = -0.4f;
	*p++ = -0.4f; *p++ = -1.0f;
	*p++ = -0.4f; *p++ = -0.4f;

	GLfloat vcoords3[8];
	p = vcoords3;
	*p++ = -1.0f; *p++ = -1.0f;
	*p++ = -1.0f; *p++ = 1.0f - (2.0f - 2.0f * vidheight / (float)glob->h);
	*p++ = 1.0f - (2.0f - 2.0f * vidwidth / (float)glob->w); *p++ = -1.0f;
	*p++ = 1.0f - (2.0f - 2.0f * vidwidth / (float)glob->w); *p++ = 1.0f - (2.0f - 2.0f * vidheight / (float)glob->h);

	GLfloat tcoords[] = {0.0f, 0.0f,
						0.0f, 1.0f,
						1.0f, 0.0f,
						1.0f, 1.0f};
    glGenBuffers(1, &vbuf2);
	glBindBuffer(GL_ARRAY_BUFFER, vbuf2);
    glBufferData(GL_ARRAY_BUFFER, 32, vcoords2, GL_STATIC_DRAW);
    glGenBuffers(1, &vbuf3);
	glBindBuffer(GL_ARRAY_BUFFER, vbuf3);
    glBufferData(GL_ARRAY_BUFFER, 32, vcoords3, GL_STATIC_DRAW);
	GLuint tbuf;
    glGenBuffers(1, &tbuf);
	glBindBuffer(GL_ARRAY_BUFFER, tbuf);
    glBufferData(GL_ARRAY_BUFFER, 32, tcoords, GL_STATIC_DRAW);
	glGenVertexArrays(1, &mainprogram->fbovao[1]);
	glBindVertexArray(mainprogram->fbovao[1]);
	glBindBuffer(GL_ARRAY_BUFFER, vbuf2);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, NULL);
	glBindBuffer(GL_ARRAY_BUFFER, tbuf);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8, NULL);
	
	glGenVertexArrays(1, &mainprogram->fbovao[2]);
	glBindVertexArray(mainprogram->fbovao[2]);
	glBindBuffer(GL_ARRAY_BUFFER, vbuf3);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, NULL);
	glBindBuffer(GL_ARRAY_BUFFER, tbuf);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8, NULL);
	
	
	glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	
	// record buffer
	glGenBuffers(1, &mainmix->ioBuf);
	
	glGenTextures(1, &binsmain->binelpreviewtex);
	glBindTexture(GL_TEXTURE_2D, binsmain->binelpreviewtex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}


Layer::Layer() {
	Layer(true);
}

Layer::Layer(bool comp) {
    glClearColor(0, 0, 0, 0);

    glGenTextures(1, &this->texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, this->texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glGenTextures(1, &this->fbotex);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, this->fbotex);
    if (comp) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mainprogram->ow, mainprogram->oh, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	}
	else {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, glob->w * 0.3f, glob->h * 0.3f, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glGenFramebuffers(1, &this->fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, this->fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->fbotex, 0);

    this->mixbox = new Box;
    this->chromabox = new Box;
    this->chromabox->acolor[3] = 1.0f;
    this->chtol = new Param;
    this->chtol->name = "Tolerance";
    this->chtol->value = 0.8f;
    this->chtol->range[0] = -0.1f;
    this->chtol->range[1] = 3.3f;
    this->chtol->shadervar = "chromatol";
	this->speed = new Param;
	this->speed->name = "Speed"; 
	this->speed->value = 1.0f;
	this->speed->range[0] = 0.0f;
	this->speed->range[1] = 3.33f;
	this->speed->sliding = true;
	this->speed->box->acolor[0] = 0.5;
	this->speed->box->acolor[1] = 0.2;
	this->speed->box->acolor[2] = 0.5;
	this->speed->box->acolor[3] = 1.0;
	this->opacity = new Param;
	this->opacity->name = "Opacity"; 
	this->opacity->value = 1.0f;
	this->opacity->range[0] = 0.0f;
	this->opacity->range[1] = 1.0f;
	this->opacity->sliding = true;
	this->opacity->box->acolor[0] = 0.5;
	this->opacity->box->acolor[1] = 0.2;
	this->opacity->box->acolor[2] = 0.5;
	this->opacity->box->acolor[3] = 1.0;
	this->volume = new Param;
	this->volume->name = "Volume"; 
	this->volume->value = 0.0f;
	this->volume->range[0] = 0.0f;
	this->volume->range[1] = 1.0f;
	this->volume->sliding = true;
	this->volume->box->acolor[0] = 0.5;
	this->volume->box->acolor[1] = 0.2;
	this->volume->box->acolor[2] = 0.5;
	this->volume->box->acolor[3] = 1.0;
	this->playbut = new Button(false);
	this->frameforward = new Button(false);
	this->framebackward = new Button(false);
	this->revbut = new Button(false);
	this->bouncebut = new Button(false);
	this->genmidibut = new Button(false);
	this->loopbox = new Box;
	this->chdir = new Button(false);
	this->chinv = new Button(false);
	this->chdir->box->acolor[3] = 1.0f;
	this->chinv->box->acolor[3] = 1.0f;

    GLfloat vcoords[8];
 	GLfloat *p = vcoords;
	GLfloat tcoords[] = {0.0f, 1.0f,
						1.0f, 1.0f,
						0.0f, 0.0f,
						1.0f, 0.0f};

  GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        printf("%X\n", err);
    }

    glGenBuffers(1, &this->vbuf);
	glBindBuffer(GL_ARRAY_BUFFER, this->vbuf);
    glBufferData(GL_ARRAY_BUFFER, 32, vcoords, GL_STATIC_DRAW);
	GLuint tbuf;
    glGenBuffers(1, &tbuf);
	glBindBuffer(GL_ARRAY_BUFFER, tbuf);
    glBufferData(GL_ARRAY_BUFFER, 32, tcoords, GL_STATIC_DRAW);
	glGenVertexArrays(1, &(this->vao));
	glBindVertexArray(this->vao);
	glBindBuffer(GL_ARRAY_BUFFER, this->vbuf);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, NULL);
	glBindBuffer(GL_ARRAY_BUFFER, tbuf);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8, NULL);
 	glBindVertexArray(this->vao);

 	//glDisableVertexAttribArray(0);
 	//glDisableVertexAttribArray(1);

	this->decresult = new frame_result;
	this->decresult->data = NULL;

    this->decoding = std::thread{&Layer::get_frame, this};
    this->decoding.detach();

	glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
}

Effect *new_effect(Effect *effect) {
	switch (effect->type) {
		case BLUR:
			return new BlurEffect(*(BlurEffect *)effect);
			break;
		case BRIGHTNESS:
			return new BrightnessEffect(*(BrightnessEffect *)effect);
			break;
		case CHROMAROTATE:
			return new ChromarotateEffect(*(ChromarotateEffect *)effect);
			break;
		case CONTRAST:
			return new ContrastEffect(*(ContrastEffect *)effect);
			break;
		case DOT:
			return new DotEffect(*(DotEffect *)effect);
			break;
		case GLOW:
			return new GlowEffect(*(GlowEffect *)effect);
			break;
		case RADIALBLUR:
			return new RadialblurEffect(*(RadialblurEffect *)effect);
			break;
		case SATURATION:
			return new SaturationEffect(*(SaturationEffect *)effect);
			break;
		case SCALE:
			return new ScaleEffect(*(ScaleEffect *)effect);
			break;
		case SWIRL:
			return new SwirlEffect(*(SwirlEffect *)effect);
			break;
		case OLDFILM:
			return new OldFilmEffect(*(OldFilmEffect *)effect);
			break;
		case RIPPLE:
			return new RippleEffect(*(RippleEffect *)effect);
			break;
		case FISHEYE:
			return new FishEyeEffect(*(FishEyeEffect *)effect);
			break;
		case TRESHOLD:
			return new TresholdEffect(*(TresholdEffect *)effect);
			break;
		case STROBE:
			return new StrobeEffect(*(StrobeEffect *)effect);
			break;
		case POSTERIZE:
			return new PosterizeEffect(*(PosterizeEffect *)effect);
			break;
		case PIXELATE:
			return new PixelateEffect(*(PixelateEffect *)effect);
			break;
		case CROSSHATCH:
			return new CrosshatchEffect(*(CrosshatchEffect *)effect);
			break;
		case INVERT:
			return new InvertEffect(*(InvertEffect *)effect);
			break;
		case ROTATE:
			return new RotateEffect(*(RotateEffect *)effect);
			break;
		case EMBOSS:
			return new EmbossEffect(*(EmbossEffect *)effect);
			break;
		case ASCII:
			return new AsciiEffect(*(AsciiEffect *)effect);
			break;
		case SOLARIZE:
			return new SolarizeEffect(*(SolarizeEffect *)effect);
			break;
		case VARDOT:
			return new VarDotEffect(*(VarDotEffect *)effect);
			break;
		case CRT:
			return new CRTEffect(*(CRTEffect *)effect);
			break;
		case EDGEDETECT:
			return new EdgeDetectEffect(*(EdgeDetectEffect *)effect);
			break;
		case KALEIDOSCOPE:
			return new KaleidoScopeEffect(*(KaleidoScopeEffect *)effect);
			break;
		case HTONE:
			return new HalfToneEffect(*(HalfToneEffect *)effect);
			break;
		case CARTOON:
			return new CartoonEffect(*(CartoonEffect *)effect);
			break;
		case CUTOFF:
			return new CutoffEffect(*(CutoffEffect *)effect);
			break;
		case GLITCH:
			return new GlitchEffect(*(GlitchEffect *)effect);
			break;
		case COLORIZE:
			return new ColorizeEffect(*(ColorizeEffect *)effect);
			break;
		case NOISE:
			return new NoiseEffect(*(NoiseEffect *)effect);
			break;
		case GAMMA:
			return new GammaEffect(*(GammaEffect *)effect);
			break;
		case THERMAL:
			return new ThermalEffect(*(ThermalEffect *)effect);
			break;
		case BOKEH:
			return new BokehEffect(*(BokehEffect *)effect);
			break;
		case SHARPEN:
			return new SharpenEffect(*(SharpenEffect *)effect);
			break;
		case DITHER:
			return new DitherEffect(*(DitherEffect *)effect);
			break;
	}
	return nullptr;
}

Layer::~Layer() {
   	glDeleteTextures(1, &this->fbotex);
    glDeleteBuffers(1, &(this->vbuf));
    glDeleteBuffers(1, &(this->tbuf));
    glDeleteFramebuffers(1, &(this->fbo));
	glDeleteVertexArrays(1, &(this->vao));
}


void draw_line(float *linec, float x1, float y1, float x2, float y2) {

	GLint box = glGetUniformLocation(mainprogram->ShaderProgram, "box");
	glUniform1i(box, 1);
	GLint color = glGetUniformLocation(mainprogram->ShaderProgram, "color");
	glUniform4fv(color, 1, linec);
	GLfloat fvcoords[4] = {x1, y1, x2, y2};
 	GLuint fvbuf, fvao;
    glGenBuffers(1, &fvbuf);
	glBindBuffer(GL_ARRAY_BUFFER, fvbuf);
    glBufferData(GL_ARRAY_BUFFER, 16, fvcoords, GL_STATIC_DRAW);
	glGenVertexArrays(1, &fvao);
	glBindVertexArray(fvao);
	glBindBuffer(GL_ARRAY_BUFFER, fvbuf);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, NULL);
	glDrawArrays(GL_LINES, 0, 2);
	glUniform1i(box, 0);
	
    glDeleteBuffers(1, &fvbuf);
	glDeleteVertexArrays(1, &fvao);
}

void draw_box(Box *box, GLuint tex) {
	draw_box(box->lcolor, box->acolor, box->vtxcoords->x1, box->vtxcoords->y1, box->vtxcoords->w, box->vtxcoords->h, 0.0f, 0.0f, 1.0f, 1.0f, 0, tex, glob->w, glob->h);
}

void draw_box(float *linec, float *areac, Box *box, GLuint tex) {
	draw_box(linec, areac, box->vtxcoords->x1, box->vtxcoords->y1, box->vtxcoords->w, box->vtxcoords->h, 0.0f, 0.0f, 1.0f, 1.0f, 0, tex, glob->w, glob->h);
}

void draw_box(Box *box, float opacity, GLuint tex) {
	draw_box(box->lcolor, box->acolor, box->vtxcoords->x1, box->vtxcoords->y1, box->vtxcoords->w, box->vtxcoords->h, 0.0f, 0.0f, 1.0f, opacity, 0, tex, glob->w, glob->h);
}

void draw_box(Box *box, float dx, float dy, float scale, GLuint tex) {
	draw_box(box->lcolor, box->acolor, box->vtxcoords->x1, box->vtxcoords->y1, box->vtxcoords->w, box->vtxcoords->h, dx, dy, scale, 1.0f, 0, tex, glob->w, glob->h);
}

void draw_box(float *linec, float *areac, float x, float y, float wi, float he, GLuint tex) {
	draw_box(linec, areac, x, y, wi, he, 0.0f, 0.0f, 1.0f, 1.0f, 0, tex, glob->w, glob->h);
}

void draw_box(float *color, float x, float y, float radius, int circle) {
	draw_box(nullptr, color, x - radius, y - radius, radius * 2.0f, (radius * 2.0f), 0.0f, 0.0f, 1.0f, 1.0f, circle, -1, glob->w, glob->h);
}

void draw_box(float *color, float x, float y, float radius, int circle, float smw, float smh) {
	draw_box(nullptr, color, x - radius, y - radius, radius * 2.0f, (radius * 2.0f), 0.0f, 0.0f, 1.0f, 1.0f, circle, -1, smw, smh);
}

void draw_box(float *linec, float *areac, float x, float y, float wi, float he, float dx, float dy, float scale, float opacity, int circle, GLuint tex, float smw, float smh) {
	GLint down = glGetUniformLocation(mainprogram->ShaderProgram, "down");
	GLint drawcircle = glGetUniformLocation(mainprogram->ShaderProgram, "circle");
	GLfloat circleradius = glGetUniformLocation(mainprogram->ShaderProgram, "circleradius");
	GLfloat cirx = glGetUniformLocation(mainprogram->ShaderProgram, "cirx");
	GLfloat ciry = glGetUniformLocation(mainprogram->ShaderProgram, "ciry");
	GLint box = glGetUniformLocation(mainprogram->ShaderProgram, "box");
	glUniform1i(box, 1);

	GLint opa = glGetUniformLocation(mainprogram->ShaderProgram, "opacity");
	glUniform1f(opa, opacity);
	GLint color = glGetUniformLocation(mainprogram->ShaderProgram, "color");
	if (linec) glUniform4fv(color, 1, linec);

	GLfloat fvcoords[8] = {
				x, y,
    			x + wi, y,
    			x + wi, y + he,
    			x, y + he,
    			};
  
	GLfloat tcoords[8];
	GLfloat *p = tcoords;
	*p++ = 0.0f; *p++ = 0.0f;
	*p++ = 0.0f; *p++ = 1.0f;
	*p++ = 1.0f; *p++ = 0.0f;
	*p++ = 1.0f; *p++ = 1.0f;
	
				
	GLuint boxvao, boxbuf;
	glGenBuffers(1, &boxbuf);	
	glBindBuffer(GL_ARRAY_BUFFER, boxbuf);
   	glBufferData(GL_ARRAY_BUFFER, 32, fvcoords, GL_STATIC_DRAW);
	glGenVertexArrays(1, &boxvao);
	glBindVertexArray(boxvao);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, NULL);
	GLuint boxtbuf;
	glGenBuffers(1, &boxtbuf);
	glBindBuffer(GL_ARRAY_BUFFER, boxtbuf);
	glBufferData(GL_ARRAY_BUFFER, 32, tcoords, GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8, NULL);
	if (linec) glDrawArrays(GL_LINE_LOOP, 0, 4);
	if (areac) {
		float pixelw = 2.0f / (float)glob->w;
		float pixelh = 2.0f / (float)glob->h;
		float shx = -dx / ((float)glob->w * 0.1f);
		float shy = -dy / ((float)glob->h * 0.1f);
		GLfloat fvcoords2[8] = {
			x + pixelw,     y + he - pixelh,
			x + pixelw,     y + pixelh   ,
			x + wi - pixelw, y + he - pixelh,
			x + wi - pixelw, y + pixelh   ,
		};
		glBindBuffer(GL_ARRAY_BUFFER, boxbuf);
		glBufferData(GL_ARRAY_BUFFER, 32, fvcoords2, GL_STATIC_DRAW);
		glBindVertexArray(boxvao);
		glUniform4fv(color, 1, areac);
		if (tex != -1) {
			GLfloat tcoords[8];
			GLfloat *p = tcoords;
			*p++ = ((0.0f) - 0.5f) * scale + 0.5f + shx; *p++ = ((0.0f) - 0.5f) * scale + 0.5f + shy;
			*p++ = ((0.0f) - 0.5f) * scale + 0.5f + shx; *p++ = ((1.0f) - 0.5f) * scale + 0.5f + shy;
			*p++ = ((1.0f) - 0.5f) * scale + 0.5f + shx; *p++ = ((0.0f) - 0.5f) * scale + 0.5f + shy;
			*p++ = ((1.0f) - 0.5f) * scale + 0.5f + shx; *p++ = ((1.0f) - 0.5f) * scale + 0.5f + shy;
			glBindBuffer(GL_ARRAY_BUFFER, boxtbuf);
			glBufferData(GL_ARRAY_BUFFER, 32, tcoords, GL_STATIC_DRAW);
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
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glUniform1i(down, 0);
		glUniform1i(drawcircle, 0);
		glDeleteBuffers(1, &boxbuf);
	}

	glUniform1i(box, 0);
	glUniform1f(opa, 1.0f);
	glDeleteVertexArrays(1, &boxvao);
	glDeleteBuffers(1, &boxbuf);	
	glDeleteBuffers(1, &boxtbuf);
 }

void draw_triangle(float *linec, float *areac, float x1, float y1, float xsize, float ysize, ORIENTATION orient, TRIANGLE_TYPE type) {

	GLint box = glGetUniformLocation(mainprogram->ShaderProgram, "box");
	glUniform1i(box, 1);

	GLint color = glGetUniformLocation(mainprogram->ShaderProgram, "color");
	if (linec) glUniform4fv(color, 1, linec);

	GLfloat fvcoords[8];
	GLfloat *p;
	switch (orient) {
		case RIGHT:
			p = fvcoords;
			*p++ = x1; *p++ = y1;
			*p++ = x1; *p++ = y1 + ysize;
			*p++ = x1 + 0.866f * xsize; *p++ = y1 + ysize / 2.0f;
			break;
		case LEFT:
			p = fvcoords;
			*p++ = x1 + 0.866f * xsize; *p++ = y1;
			*p++ = x1 + 0.866f * xsize; *p++ = y1 + ysize;
			*p++ = x1; *p++ = y1 + ysize / 2.0f;
			break;
		case UP:
			p = fvcoords;
			*p++ = x1 + xsize / 2.0f; *p++ = y1;
			*p++ = x1; *p++ = y1 + 0.866f * ysize;
			*p++ = x1 + xsize; *p++ = y1 + 0.866f * ysize;
			break;
		case DOWN:
			p = fvcoords;
			*p++ = x1; *p++ = y1;
			*p++ = x1 + xsize; *p++ = y1;
			*p++ = x1 + xsize / 2.0f; *p++ = y1 + 0.866f * ysize;
			break;
	}
 	GLuint fvbuf, fvao;
    glGenBuffers(1, &fvbuf);
	glBindBuffer(GL_ARRAY_BUFFER, fvbuf);
    glBufferData(GL_ARRAY_BUFFER, 24, fvcoords, GL_STATIC_DRAW);
	glGenVertexArrays(1, &fvao);
	glBindVertexArray(fvao);
	glBindBuffer(GL_ARRAY_BUFFER, fvbuf);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, NULL);
	if (type == CLOSED) {
		glUniform4fv(color, 1, areac);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);
	}
	if (type == OPEN) glDrawArrays(GL_LINE_LOOP, 0, 3);

	glUniform1i(box, 0);
    glDeleteBuffers(1, &fvbuf);
	glDeleteVertexArrays(1, &fvao);
}


float render_text(std::string text, float *textc, float x, float y, float sx, float sy) {
	float ret = render_text(text, textc, x, y, sx, sy, 0);
	return ret;
}

float render_text(std::string text, float *textc, float x, float y, float sx, float sy, int smflag) {
 	y -= 0.03f;
	GLuint texture;
	GLuint vbo, tbo, vao;
	float textw = 0.0f;
	float texth = 0.0f;
	bool prepare = true;
	if (smflag == 0) {
		for (int i = 0; i < mainprogram->guistrings.size(); i++) {
			if (text.compare(mainprogram->guistrings[i]->text) != 0 or sx != mainprogram->guistrings[i]->sx) {
				prepare = true;
			}
			else {
				prepare = false;
				texture = mainprogram->guistrings[i]->texture;
				vbo = mainprogram->guistrings[i]->vbo;
				tbo = mainprogram->guistrings[i]->tbo;
				vao = mainprogram->guistrings[i]->vao;
				textw = mainprogram->guistrings[i]->textw;
				texth = mainprogram->guistrings[i]->texth;
				break;
			}
		}
	}
	else if (smflag == 1) {
		for (int i = 0; i < mainprogram->prguistrings.size(); i++) {
			if (text.compare(mainprogram->prguistrings[i]->text) != 0 or sx != mainprogram->prguistrings[i]->sx) {
				prepare = true;
			}
			else {
				prepare = false;
				texture = mainprogram->prguistrings[i]->texture;
				vbo = mainprogram->prguistrings[i]->vbo;
				tbo = mainprogram->prguistrings[i]->tbo;
				vao = mainprogram->prguistrings[i]->vao;
				textw = mainprogram->prguistrings[i]->textw;
				texth = mainprogram->prguistrings[i]->texth;
				break;
			}
		}
	}
	else if (smflag == 2) {
		for (int i = 0; i < mainprogram->tmguistrings.size(); i++) {
			if (text.compare(mainprogram->tmguistrings[i]->text) != 0 or sx != mainprogram->tmguistrings[i]->sx) {
				prepare = true;
			}
			else {
				prepare = false;
				texture = mainprogram->tmguistrings[i]->texture;
				vbo = mainprogram->tmguistrings[i]->vbo;
				tbo = mainprogram->tmguistrings[i]->tbo;
				vao = mainprogram->tmguistrings[i]->vao;
				textw = mainprogram->tmguistrings[i]->textw;
				texth = mainprogram->tmguistrings[i]->texth;
				break;
			}
		}
	}
	
	if (prepare) {
		const char *t = text.c_str();
		const char *p;
		
		glGenTextures(1, &texture);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RGBA,
			2048,
			128,
			0,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			0
			);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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
		//glBindTexture(GL_TEXTURE_2D, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
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
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, NULL);
		glBindBuffer(GL_ARRAY_BUFFER, textbuf);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8, NULL);
		GLuint ftex;
		glGenTextures(1, &ftex);
		glBindTexture(GL_TEXTURE_2D, ftex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		
		FT_Set_Pixel_Sizes(face, 0, (int)(sy * 32000.0f * glob->h / 1346.0f)); //
		x = -1.0f;
		y = 1.0f;
		FT_GlyphSlot g = face->glyph;
		for(p = t; *p; p++) {
			int glyph_index = FT_Get_Char_Index(face, *p);
			if(FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT|FT_LOAD_MONOCHROME))
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
	
			float pixelw = 2.0f / glob->w;
			float pixelh = 2.0f / glob->h;
			float x2 = x + g->bitmap_left * pixelw;
			float y2 = y - (g->bitmap_top + 16 * glob->h / 1346.0f) * pixelh;
			float wi = g->bitmap.width * pixelw;
			float he = g->bitmap.rows * pixelh;
	
			GLint textmode = glGetUniformLocation(mainprogram->ShaderProgram, "textmode");
			glUniform1i(textmode, 1);
			GLint color = glGetUniformLocation(mainprogram->ShaderProgram, "color");
			glUniform4fv(color, 1, textc);
			GLfloat texvcoords2[8] = {
					x2,     -y2,
					x2 + wi, -y2,
					x2,     -y2 - he,
					x2 + wi, -y2 - he};

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, ftex);
			glBindBuffer(GL_ARRAY_BUFFER, texvbuf);
			glBufferData(GL_ARRAY_BUFFER, 32, texvcoords2, GL_STATIC_DRAW);
			glBindVertexArray(texvao);
			glBindFramebuffer(GL_FRAMEBUFFER, texfrbuf);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			if (smflag == 1) {
				glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->smglobfbo_pr);
			}
			else if (smflag == 2) {
				glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->smglobfbo_tm);
			}
			else {
				glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
			}
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			glUniform1i(textmode, 0);
	
			x += (g->advance.x/64) * pixelw;
			//y += (g->advance.y/64) * pixelh;
			textw += (g->advance.x/128) * pixelw;
			texth = 64.0f * pixelh;
		}
		GUIString *guistring = new GUIString;
		guistring->text = text;
		guistring->texture = texture;
		guistring->vbo = texvbuf;
		guistring->tbo = textbuf;
		guistring->vao = texvao;
		guistring->textw = textw;
		guistring->texth = texth;
		guistring->sx = sx;
		if (smflag == 0) mainprogram->guistrings.push_back(guistring);
		else if (smflag == 1) mainprogram->prguistrings.push_back(guistring);
		else if (smflag == 2) mainprogram->tmguistrings.push_back(guistring);
		
		glDeleteFramebuffers(1, &texfrbuf);
  	}
	else {
		GLfloat texvcoords[8] = {
			x,     y   ,
			x + texth * 16, y    ,
			x,     y + texth * 2,
			x + texth * 16, y + texth * 2};
		GLfloat textcoords[] = {0.0f, 0.0f,
							1.0f, 0.0f,
							0.0f, 1.0f,
							1.0f, 1.0f};
		GLint textmode = glGetUniformLocation(mainprogram->ShaderProgram, "textmode");
		glUniform1i(textmode, 2);
		GLint color = glGetUniformLocation(mainprogram->ShaderProgram, "color");
		glUniform4fv(color, 1, textc);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		GLuint texvao;
		glGenVertexArrays(1, &texvao);
		glBindVertexArray(texvao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, 32, texvcoords, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, NULL);
		glBindBuffer(GL_ARRAY_BUFFER, tbo);
		glBufferData(GL_ARRAY_BUFFER, 32, textcoords, GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8, NULL);
		if (smflag == 1) {
			glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->smglobfbo_pr);
		}
		else if (smflag == 2) {
			glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->smglobfbo_tm);
		}
		else {
			glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
		}
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		if (textw != 0) glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
		glUniform1i(textmode, 0);
		glDeleteVertexArrays(1, &texvao);
	}
	
	return textw;
}


// calculates new frame numbers for a video layer
// also checks if new frame has been decompressed and loads it into layer
void calc_texture(Layer *lay, bool comp, bool alive) {
	// laynocomp is root layer for comp layer when video previewing is on
	Layer *laynocomp;
	if (!comp) {
		laynocomp = lay;
	}
	else if (mainprogram->prevvid and mainprogram->preveff) {
		laynocomp = lay;
	}
	else if (!mainprogram->preveff) {
		laynocomp = lay;
	}
	else if (std::find(mainmix->layersAcomp.begin(), mainmix->layersAcomp.end(), lay) != mainmix->layersAcomp.end()) {
		if (lay->pos < mainmix->layersA.size()) laynocomp = mainmix->layersA[lay->pos];
		else laynocomp = lay;
	}
	else {
		if (lay->pos < mainmix->layersB.size()) laynocomp = mainmix->layersB[lay->pos];
		else laynocomp = lay;
	}
	
	//measure time since last loop
	std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed;
	if (lay->timeinit) elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - lay->prevtime);
	else {
		lay->timeinit = true;
		lay->prevtime = now;
	}
	long long microcount = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
	lay->prevtime = now;
	float thismilli = (float)microcount / 1000.0f;
	
	if (!lay->live) {
		if (comp and mainprogram->preveff and !mainprogram->prevvid) {
			lay->prevtime = laynocomp->prevtime;
			lay->prevframe = laynocomp->prevframe;
			lay->frame = laynocomp->frame;
		}
		else {
			if (lay->prevframe != -1) {
				// calculate and visualize fps
				lay->fps[lay->fpscount] = (int)(1000.0f / thismilli);
				int total = 0;
				for (int i = 0; i < 25; i++) total += lay->fps[i];
				int rate = total / 25;
				float white[] = {1.0f, 1.0f, 1.0f, 1.0f};
				std::string s = std::to_string(rate);
				render_text(s, white, 0.01f, 0.47f, 0.0006f, 0.001f);
				lay->fpscount++;
				if (lay->fpscount > 24) lay->fpscount = 0;
		
				lay->frame = lay->frame + laynocomp->scratch;
				laynocomp->scratch = 0;
				// calculate new frame numbers
				if ((laynocomp->speed->value > 0 and (laynocomp->playbut->value or laynocomp->bouncebut->value == 1)) or (laynocomp->speed->value < 0 and (laynocomp->revbut->value or laynocomp->bouncebut->value == 2))) {
					lay->frame += !laynocomp->scratchtouch * laynocomp->speed->value * laynocomp->speed->value * thismilli / lay->millif;
				}
				else if ((laynocomp->speed->value > 0 and (laynocomp->revbut->value or laynocomp->bouncebut->value == 2)) or (laynocomp->speed->value < 0 and (laynocomp->playbut->value or laynocomp->bouncebut->value == 1))) {
					lay->frame -= !laynocomp->scratchtouch * laynocomp->speed->value * laynocomp->speed->value * thismilli / lay->millif;
				}
				
				// on end of video (or beginning if reverse play) switch to next clip in queue
				if (lay->frame > (laynocomp->endframe)) {
					if (lay->scritching != 4) {
						if (laynocomp->bouncebut->value == 0) {
							lay->frame = laynocomp->startframe;
							if (lay->clips.size() > 1) {
								Clip *clip = lay->clips[0];
								lay->clips.erase(lay->clips.begin());
								Node *node = lay->node;
								ELEM_TYPE temptype = lay->currcliptype;
								std::string tpath;
								GLuint temptex;
								if (temptype == ELEM_LAYER and lay->effects.size()) {
									temptex = copy_tex(node->vidbox->tex);
									std::string name = remove_extension(basename(lay->filename));
									int count = 0;
									while (1) {
										tpath = mainprogram->temppath + "cliptemp_" + name + ".layer";
										if (!exists(tpath)) {
											mainmix->save_layerfile(tpath, lay, 0);
											break;
										}
										count++;
										name = remove_version(name) + "_" + std::to_string(count);
									}
								}
								else if (temptype == ELEM_LIVE) {
									temptex = copy_tex(node->vidbox->tex);
									temptype = ELEM_LIVE;
									tpath = lay->filename;
								}
								else {
									temptex = copy_tex(lay->fbotex);
									temptype = ELEM_FILE;
									tpath = lay->filename;
								}
								if (clip->type == ELEM_FILE) {
									lay->live = false;
									open_video(0, lay, clip->path, 1);
								}
								else if (clip->type == ELEM_LAYER) {
									lay->live = false;
									mainmix->open_layerfile(clip->path, lay, 1, 0);
								}
								else if (clip->type == ELEM_LIVE) {
									set_live_base(lay, clip->path);
								}
								lay->currcliptype = clip->type;
								delete clip;
								clip = new Clip;
								clip->tex = temptex;
								clip->type = temptype;
								clip->path = tpath;
								lay->clips.insert(lay->clips.end() - 1, clip);
							}
						}
						else {
							lay->frame = laynocomp->endframe - (lay->frame - laynocomp->endframe);
							laynocomp->bouncebut->value = 2;
						}
					}
				}
				else if (lay->frame < laynocomp->startframe) {
					if (lay->scritching != 4) {
						if (laynocomp->bouncebut->value == 0) {
							lay->frame = laynocomp->endframe;
							if (lay->clips.size() > 1) {
								Clip *clip = lay->clips[0];
								lay->clips.erase(lay->clips.begin());
								Node *node = lay->node;
								GLuint temptex = copy_tex(node->vidbox->tex);
								ELEM_TYPE temptype;
								std::string tpath;
								if (lay->effects.size()) {
									temptype = ELEM_LAYER;
									std::string name = remove_extension(basename(lay->filename));
									int count = 0;
									while (1) {
										tpath = mainprogram->binsdir + "temp_" + name + ".layer";
										if (!exists(tpath)) {
											mainmix->save_layerfile(tpath, lay, 0);
											break;
										}
										count++;
										name = remove_version(name) + "_" + std::to_string(count);
									}
								}
								else if (temptype == ELEM_LIVE) {
									temptex = copy_tex(node->vidbox->tex);
									temptype = ELEM_LIVE;
									tpath = lay->filename;
								}
								else {
									temptype = ELEM_FILE;
									tpath = lay->filename;
								}
								if (clip->type == ELEM_FILE) {
									lay->live = false;
									open_video(0, lay, clip->path, 2);
								}
								else if (clip->type == ELEM_LAYER) {
									lay->live = false;
									mainmix->open_layerfile(clip->path, lay, 1, 0);
								}
								else if (clip->type == ELEM_LIVE) {
									set_live_base(lay, clip->path);
								}
								delete clip;
								clip = new Clip;
								clip->tex = temptex;
								clip->type = temptype;
								clip->path = tpath;
								lay->clips.insert(lay->clips.end() - 1, clip);
							}
						}
						else {
							lay->frame = laynocomp->startframe + (laynocomp->startframe - lay->frame);
							laynocomp->bouncebut->value = 1;
						}
					}
				}
				else {
					if (lay->scritching == 4) lay->scritching = 0;
				}
			}
		}
	}
	
	// advance ripple effects
	for (int i = 0; i < lay->effects.size(); i++) {
		if (lay->effects[i]->type == RIPPLE) {
			RippleEffect *effect = (RippleEffect*)(lay->effects[i]);
			effect->set_ripplecount(effect->get_ripplecount() + (effect->get_speed() * (thismilli / 50.0f)));
			if (effect->get_ripplecount() > 100) effect->set_ripplecount(0);
		}
	}
	
	if (!alive) return;
		
	if (comp and !mainprogram->prevvid and mainprogram->preveff) {
	}
	else {
		lay->ready = true;
		lay->startdecode.notify_one();
	}
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, lay->texture);
	if (!lay->liveinput) {
		if (lay->decresult) {  // decresult contains new frame width, height and data
			if (lay->decresult->width) {
				if (lay->dataformat == 188 or lay->vidformat == 187) {
					// HAP video layers
					if (lay->decresult->compression == 187) {
						glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, lay->decresult->width, lay->decresult->height, 0, lay->decresult->size, lay->decresult->data);
					}
					else if (lay->decresult->compression == 190) {
						glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, lay->decresult->width, lay->decresult->height, 0, lay->decresult->size, lay->decresult->data);
					}
				}
				else { // implement Sub
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, lay->decresult->width, lay->decresult->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, lay->decresult->data);
				}
			}
		}
		lay->decresult->data = nullptr;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, lay->fbo);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	float black[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	float div;
	float fac = 1.0f;
	if (!comp) {
		// previewing frames are on 30%
		div = 0.3f;
	}
	else {
		div = mainprogram->ow / glob->w;
		fac = (glob->w / glob->h) / (mainprogram->ow / mainprogram->oh);  //aspect ratio correction
	}
	glDisable(GL_BLEND);
	// put lay->texture into lay->fbo(tex)
	if (lay->liveinput) {
		draw_box(NULL, black, -1.0f, -1.0f, 2.0f * div, 2.0f * div * fac, 0.0f, 0.0f, 1.0f, 1.0f, 0, lay->liveinput->texture, glob->w, glob->h);
	}
	else if (lay->filename != "") {
		draw_box(NULL, black, -1.0f, -1.0f, 2.0f * div, 2.0f * div * fac, 0.0f, 0.0f, 1.0f, 1.0f, 0, lay->texture, glob->w, glob->h);
	}
	else {
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
	}
	glEnable(GL_BLEND);
	glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
}

void set_queueing(Layer *lay, bool onoff) {
	if (onoff) {
		lay->queueing = true;
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
	
	
void display_texture(Layer *lay, bool deck) {
	float green[4] = {0.0, 1.0, 0.2, 1.0};
	float white[4] = {1.0, 1.0, 1.0, 1.0};
	float red[4] = {1.0, 0.0, 0.0, 1.0};
	float black[4] = {0.0, 0.0, 0.0, 1.0};

	std::vector<Layer*> &lvec = choose_layers(deck);
	if (mainmix->scrollpos[deck] > lvec.size() - 2) mainmix->scrollpos[deck] = lvec.size() - 2;
	if (lay->pos >= mainmix->scrollpos[deck] and lay->pos < mainmix->scrollpos[deck] + 3) {
		Box *box = lay->node->vidbox;
		
		draw_box(box, box->tex);

		if (mainmix->mode == 0 and mainprogram->nodesmain->linked) {
			// Trigger mainprogram->laymenu
			if (box->in()) {
				std::string deckstr;
				if (deck == 0) deckstr = "A";
				else if (deck == 1) deckstr = "B";
				render_text("Layer " + deckstr + std::to_string(lay->pos + 1), white, box->vtxcoords->x1 + tf(0.01f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.03f), 0.0005f, 0.0008f);
				render_text(remove_extension(basename(lay->filename)), white, box->vtxcoords->x1 + tf(0.01f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.06f), 0.0005f, 0.0008f);
				if (lay->dataformat == -1) {}
				else if (lay->dataformat == 188 or lay->vidformat == 187) {
					render_text("HAP", white, box->vtxcoords->x1 + tf(0.01f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.09f), 0.0005f, 0.0008f);
				}
				else {
					render_text("CPU", white, box->vtxcoords->x1 + tf(0.01f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.09f), 0.0005f, 0.0008f);
				}
				if (!mainprogram->needsclick or mainprogram->leftmouse) {
					if (!mainmix->moving and !mainprogram->cwon) {
						mainmix->currlay = lay;
						if (mainprogram->menuactivation) {
							mainprogram->laymenu->state = 2;
							mainmix->mouselayer = lay;
							mainmix->mousedeck = deck;
							mainprogram->menuactivation = false;
						}
						if (mainprogram->menuactivation) {
							mainprogram->livemenu->state = 2;
							mainmix->mouselayer = lay;
							mainprogram->menuactivation = false;
						}
					}
				}
				if (mainprogram->doubleleftmouse) {
					set_queueing(lay, true);
					mainprogram->doubleleftmouse = false;
					mainprogram->leftmouse = false;
					enddrag();
				}
			}
			else {
				int size = lvec.size();
				int numonscreen = size - mainmix->scrollpos[deck];
				if (0 <= numonscreen and numonscreen <= 2) {
					if (mainprogram->xvtxtoscr(mainmix->numw + deck * 1.0f + numonscreen * mainmix->layw) < mainprogram->mx and mainprogram->mx < deck * (glob->w / 2.0f) + glob->w / 2.0f) {
						if (0 < mainprogram->my and mainprogram->my < mainprogram->yvtxtoscr(mainmix->layw)) {
							if (mainprogram->menuactivation) {
								mainprogram->loadmenu->state = 2;
								mainmix->mousedeck = deck;
								mainprogram->menuactivation = false;
							}
						}
					}
				}
			}
		}
		

		// Show current layer controls
		if (lay != mainmix->currlay) return;
		
		if (!mainprogram->queueing) {
			// Draw mixbox
			std::string mixstr;
			box = lay->mixbox;
			draw_box(box, -1);
			switch (lay->blendnode->blendtype) {
				case 1:
					mixstr = "Mix";
					break;
				case 2:
					mixstr = "Mult";
					break;
				case 3:
					mixstr = "Scrn";
					break;
				case 4:
					mixstr = "Ovrl";
					break;
				case 5:
					mixstr = "HaLi";
					break;
				case 6:
					mixstr = "SoLi";
					break;
				case 7:
					mixstr = "Divi";
					break;
				case 8:
					mixstr = "Add";
					break;
				case 9:
					mixstr = "Sub";
					break;
				case 10:
					mixstr = "Diff";
					break;
				case 11:
					mixstr = "Dodg";
					break;
				case 12:
					mixstr = "CoBu";
					break;
				case 13:
					mixstr = "LiBu";
					break;
				case 14:
					mixstr = "ViLi";
					break;
				case 15:
					mixstr = "LiLi";
					break;
				case 16:
					mixstr = "DaOn";
					break;
				case 17:
					mixstr = "LiOn";
					break;
				case 18:
					mixstr = "Wipe";
					break;
				case 19:
					mixstr = "Chro";
					if (lay->pos > 0) {
						draw_box(lay->chromabox, -1);
						render_text("Color", white, lay->mixbox->vtxcoords->x1 + tf(0.08f), 1.0f - (tf(mainmix->layw)) + tf(0.02f), tf(0.0003f), tf(0.0005f));
					}
					break;
				case 20:
					mixstr = "Disp";
					break;
			}
			if (lay->pos > 0) {
				render_text(mixstr, white, lay->mixbox->vtxcoords->x1 + tf(0.01f), 1.0f - (tf(mainmix->layw)) + tf(0.02f), tf(0.0003f), tf(0.0005f));
			}
			else {
				render_text(mixstr, red, lay->mixbox->vtxcoords->x1 + tf(0.01f), 1.0f - (tf(mainmix->layw)) + tf(0.02f), tf(0.0003f), tf(0.0005f));
			}
			
			// Draw and handle effect stack scrollboxes
			if (lay->effscroll > 0 and mainmix->currlay->deck == 0) {
				if (mainprogram->effscrollupA->in()) {
					mainprogram->effscrollupA->acolor[0] = 0.5f;
					mainprogram->effscrollupA->acolor[1] = 0.5f;
					mainprogram->effscrollupA->acolor[2] = 1.0f;
					mainprogram->effscrollupA->acolor[3] = 1.0f;
					if (mainprogram->leftmouse) {
						lay->effscroll--;
					}
				}
				else {			
					mainprogram->effscrollupA->acolor[0] = 0.0f;
					mainprogram->effscrollupA->acolor[1] = 0.0f;
					mainprogram->effscrollupA->acolor[2] = 0.0f;
					mainprogram->effscrollupA->acolor[3] = 1.0f;
				}
				draw_box(mainprogram->effscrollupA, -1);
				draw_triangle(white, white, mainprogram->effscrollupA->vtxcoords->x1 + tf(0.0074f), mainprogram->effscrollupA->vtxcoords->y1 + tf(0.0416f) - tf(0.030f), tf(0.011f), tf(0.0208f), DOWN, CLOSED);
			}
			if (lay->effscroll > 0 and mainmix->currlay->deck == 1) {
				if (mainprogram->effscrollupB->in()) {
					mainprogram->effscrollupB->acolor[0] = 0.5f;
					mainprogram->effscrollupB->acolor[1] = 0.5f;
					mainprogram->effscrollupB->acolor[2] = 1.0f;
					mainprogram->effscrollupB->acolor[3] = 1.0f;
					if (mainprogram->leftmouse) {
						lay->effscroll--;
					}
				}
				else {			
					mainprogram->effscrollupB->acolor[0] = 0.0f;
					mainprogram->effscrollupB->acolor[1] = 0.0f;
					mainprogram->effscrollupB->acolor[2] = 0.0f;
					mainprogram->effscrollupB->acolor[3] = 1.0f;
				}
				draw_box(mainprogram->effscrollupB, -1);
				draw_triangle(white, white, mainprogram->effscrollupB->vtxcoords->x1 + tf(0.0074f), mainprogram->effscrollupB->vtxcoords->y1 + tf(0.0416f) - tf(0.030f), tf(0.011f), tf(0.0208f), DOWN, CLOSED);
			}
			if (lay->numefflines - lay->effscroll > 11 and mainmix->currlay->deck == 0) {
				if (mainprogram->effscrolldownA->in()) {
					mainprogram->effscrolldownA->acolor[0] = 0.5f;
					mainprogram->effscrolldownA->acolor[1] = 0.5f;
					mainprogram->effscrolldownA->acolor[2] = 1.0f;
					mainprogram->effscrolldownA->acolor[3] = 1.0f;
					if (mainprogram->leftmouse) {
						lay->effscroll++;
					}
				}
				else {			
					mainprogram->effscrolldownA->acolor[0] = 0.0f;
					mainprogram->effscrolldownA->acolor[1] = 0.0f;
					mainprogram->effscrolldownA->acolor[2] = 0.0f;
					mainprogram->effscrolldownA->acolor[3] = 1.0f;
				}
				draw_box(mainprogram->effscrolldownA, -1);
				draw_triangle(white, white, mainprogram->effscrolldownA->vtxcoords->x1 + tf(0.0074f), mainprogram->effscrolldownA->vtxcoords->y1 + tf(0.0416f) - tf(0.030f), tf(0.011f), tf(0.0208f), UP, CLOSED);
			}			
			if (lay->numefflines - lay->effscroll > 11 and mainmix->currlay->deck == 1) {
				if (mainprogram->effscrolldownB->in()) {
					mainprogram->effscrolldownB->acolor[0] = 0.5f;
					mainprogram->effscrolldownB->acolor[1] = 0.5f;
					mainprogram->effscrolldownB->acolor[2] = 1.0f;
					mainprogram->effscrolldownB->acolor[3] = 1.0f;
					if (mainprogram->leftmouse) {
						lay->effscroll++;
					}
				}
				else {			
					mainprogram->effscrolldownB->acolor[0] = 0.0f;
					mainprogram->effscrolldownB->acolor[1] = 0.0f;
					mainprogram->effscrolldownB->acolor[2] = 0.0f;
					mainprogram->effscrolldownB->acolor[3] = 1.0f;
				}
				draw_box(mainprogram->effscrolldownB, -1);
				draw_triangle(white, white, mainprogram->effscrolldownB->vtxcoords->x1 + tf(0.0074f), mainprogram->effscrolldownB->vtxcoords->y1 + tf(0.0416f) - tf(0.030f), tf(0.011f), tf(0.0208f), UP, CLOSED);
			}
			if (lay->effects.size()) {
				if ((glob->w / 2.0f > mainprogram->mx and mainmix->currlay->deck == 0) or (glob->w / 2.0f < mainprogram->mx and mainmix->currlay->deck == 1)) {
					if (mainprogram->my > mainprogram->yvtxtoscr(mainmix->layw - tf(0.20f))) {
						if (mainprogram->mousewheel and lay->numefflines > 11) {
							lay->effscroll -= mainprogram->mousewheel;
							if (lay->effscroll < 0) lay->effscroll = 0;
							if (lay->numefflines - lay->effscroll < 11) lay->effscroll = lay->numefflines - 11;
						}
					}
				}
			}
			// Draw effectboxes and parameters
			std::string effstr;
			for(int i = 0; i < lay->effects.size(); i++) {
				Effect *eff = lay->effects[i];
				Box *box;
				float x1, y1, wi;
				
				if (eff->box->vtxcoords->y1 < 1.0 - tf(mainmix->layw) - tf(0.22f) - tf(0.05f) * 10) break;
				if (eff->box->vtxcoords->y1 <= 1.0 - tf(mainmix->layw) - tf(0.18f)) {
					eff->drywet->handle();		
					
					box = eff->box;
					draw_box(box, -1);
					if (eff->onoffbutton->value) {
						eff->onoffbutton->box->acolor[1] = 0.7f;
					}
					else {
						eff->onoffbutton->box->acolor[1] = 0.0f;
					}
					draw_box(eff->onoffbutton->box, -1);
					switch (eff->type) {
						case 0:
							effstr = "BLUR";
							break;
						case 1:
							effstr = "BRIGHTNESS";
							break;
						case 2:
							effstr = "CHROMAROTATE";
							break;
						case 3:
							effstr = "CONTRAST";
							break;
						case 4:
							effstr = "DOT";
							break;
						case 5:
							effstr = "GLOW";
							break;
						case 6:
							effstr = "RADIALBLUR";
							break;
						case 7:
							effstr = "SATURATION";
							break;
						case 8:
							effstr = "SCALE";
							break;
						case 9:
							effstr = "SWIRL";
							break;
						case 10:
							effstr = "OLDFILM";
							break;
						case 11:
							effstr = "RIPPLE";
							break;
						case 12:
							effstr = "FISHEYE";
							break;
						case 13:
							effstr = "TRESHOLD";
							break;
						case 14:
							effstr = "STROBE";
							break;
						case 15:
							effstr = "POSTERIZE";
							break;
						case 16:
							effstr = "PIXELATE";
							break;
						case 17:
							effstr = "CROSSHATCH";
							break;
						case 18:
							effstr = "INVERT";
							break;
						case 19:
							effstr = "ROTATE";
							break;
						case 20:
							effstr = "EMBOSS";
							break;
						case 21:
							effstr = "ASCII";
							break;
						case 22:
							effstr = "SOLARIZE";
							break;
						case 23:
							effstr = "VARDOT";
							break;
						case 24:
							effstr = "CRT";
							break;
						case 25:
							effstr = "EDGEDETECT";
							break;
						case 26:
							effstr = "KALEIDOSCOPE";
							break;
						case 27:
							effstr = "HALFTONE";
							break;
						case 28:
							effstr = "CARTOON";
							break;
						case 29:
							effstr = "CUTOFF";
							break;
						case 30:
							effstr = "GLITCH";
							break;
						case 31:
							effstr = "COLORIZE";
							break;
						case 32:
							effstr = "NOISE";
							break;
						case 33:
							effstr = "GAMMA";
							break;
						case 34:
							effstr = "THERMAL";
							break;
						case 35:
							effstr = "BOKEH";
							break;
						case 36:
							effstr = "SHARPEN";
							break;
						case 37:
							effstr = "DITHER";
							break;
					}
					float textw = tf(render_text(effstr, white, eff->box->vtxcoords->x1 + tf(0.01f), eff->box->vtxcoords->y1 + tf(0.05f) - tf(0.030f), tf(0.0003f), tf(0.0005f)));
					eff->box->vtxcoords->w = textw + tf(0.032f);
					x1 = eff->box->vtxcoords->x1 + tf(0.032f) + textw;
					wi = (0.7f - mainmix->numw - tf(0.032f) - textw) / 4.0f;
				}
				y1 = eff->box->vtxcoords->y1;
				for (int j = 0; j < eff->params.size(); j++) {
					Param *par = eff->params[j];
					par->box->lcolor[0] = 1.0;
					par->box->lcolor[1] = 1.0;
					par->box->lcolor[2] = 1.0;
					par->box->lcolor[3] = 1.0;
					if (par->nextrow) { 
						x1 = eff->box->vtxcoords->x1 + tf(0.02f);
						y1 -= tf(0.05f);
						wi = (0.7f - mainmix->numw - tf(0.02f)) / 4.0;
					}
					par->box->vtxcoords->x1 = x1;
					x1 += wi + tf(0.01f);
					par->box->vtxcoords->y1 = y1;
					par->box->vtxcoords->w = wi;
					par->box->vtxcoords->h = eff->box->vtxcoords->h;
					par->box->upvtxtoscr();
					
					if (par->box->vtxcoords->y1 < 1.0 - tf(mainmix->layw) - tf(0.22f) - tf(0.05f) * 10) break;
					if (par->box->vtxcoords->y1 <= 1.0 - tf(mainmix->layw) - tf(0.18f)) {
						par->handle();
					}
				}
			}
			// Handle chroma tolerance
			if (lay->pos > 0) {
				if (lay->blendnode->blendtype == CHROMAKEY) {
					Param *par = lay->chtol;
					par->box->lcolor[0] = 1.0;
					par->box->lcolor[1] = 1.0;
					par->box->lcolor[2] = 1.0;
					par->box->lcolor[3] = 1.0;
					par->box->acolor[0] = 0.2;
					par->box->acolor[1] = 0.2;
					par->box->acolor[2] = 0.2;
					par->box->acolor[3] = 1.0;
					par->box->vtxcoords->x1 = lay->chromabox->vtxcoords->x1 + tf(0.07f);
					par->box->vtxcoords->y1 = lay->chromabox->vtxcoords->y1;
					par->box->vtxcoords->w = tf(0.08f);
					par->box->vtxcoords->h = lay->chromabox->vtxcoords->h;
					par->box->upvtxtoscr();
					
					par->handle();
				}
			}
			
					
			// Draw mixfac->box
			if (lay->pos > 0 and lay->blendnode->blendtype != CHROMAKEY) {
				Param *par = lay->blendnode->mixfac;
				par->handle();
			}
				
			// Draw speed->box
			Param *par = lay->speed;
			par->handle();
			draw_box(white, NULL, lay->speed->box->vtxcoords->x1, lay->speed->box->vtxcoords->y1, lay->speed->box->vtxcoords->w * 0.30f, tf(0.05f), -1);
			
			// Draw opacity->box
			par = lay->opacity;
			par->handle();
			
			// Draw volume->box
			if (lay->audioplaying) {
				par = lay->volume;
				par->handle();
			}
				
			// Draw and handle playbutton revbutton bouncebutton
			if (lay->playbut->box->in()) {
				lay->playbut->box->acolor[0] = 0.5;
				lay->playbut->box->acolor[1] = 0.5;
				lay->playbut->box->acolor[2] = 1.0;
				lay->playbut->box->acolor[3] = 1.0;
				if (mainprogram->leftmouse) {
					lay->playbut->value = !lay->playbut->value;
					if (lay->playbut->value) {
						lay->revbut->value = false;
						lay->bouncebut->value = false;
					}
				}
			}
			else {
				lay->playbut->box->acolor[0] = 0.5;
				lay->playbut->box->acolor[1] = 0.2;
				lay->playbut->box->acolor[2] = 0.5;
				lay->playbut->box->acolor[3] = 1.0;
				if (lay->playbut->value) {
					lay->playbut->box->acolor[0] = 1.0;
					lay->playbut->box->acolor[1] = 0.5;
					lay->playbut->box->acolor[2] = 0.0;
					lay->playbut->box->acolor[3] = 1.0;
				}
			}
			draw_box(lay->playbut->box, -1);
			draw_triangle(white, white, lay->playbut->box->vtxcoords->x1 + tf(0.0078f), lay->playbut->box->vtxcoords->y1 + tf(0.0416f) - tf(0.030f), tf(0.011f), tf(0.0208f), RIGHT, CLOSED);
			if (lay->revbut->box->in()) {
				lay->revbut->box->acolor[0] = 0.5;
				lay->revbut->box->acolor[1] = 0.5;
				lay->revbut->box->acolor[2] = 1.0;
				lay->revbut->box->acolor[3] = 1.0;
				if (mainprogram->leftmouse) {
					lay->revbut->value = !lay->revbut->value;
					if (lay->revbut->value) {
						lay->playbut->value = false;
						lay->bouncebut->value = false;
					}
				}
			}
			else {
				lay->revbut->box->acolor[0] = 0.5;
				lay->revbut->box->acolor[1] = 0.2;
				lay->revbut->box->acolor[2] = 0.5;
				lay->revbut->box->acolor[3] = 1.0;
				if (lay->revbut->value) {
					lay->revbut->box->acolor[0] = 1.0;
					lay->revbut->box->acolor[1] = 0.5;
					lay->revbut->box->acolor[2] = 0.0;
					lay->revbut->box->acolor[3] = 1.0;
				}
			}
			draw_box(lay->revbut->box, -1);
			draw_triangle(white, white, lay->revbut->box->vtxcoords->x1 + tf(0.00625f), lay->revbut->box->vtxcoords->y1 + tf(0.0416f) - tf(0.030f), tf(0.011f), tf(0.0208f), LEFT, CLOSED);
			if (lay->bouncebut->box->in()) {
				lay->bouncebut->box->acolor[0] = 0.5;
				lay->bouncebut->box->acolor[1] = 0.5;
				lay->bouncebut->box->acolor[2] = 1.0;
				lay->bouncebut->box->acolor[3] = 1.0;
				if (mainprogram->leftmouse) {
					lay->bouncebut->value = !lay->bouncebut->value;
					if (lay->bouncebut->value) {
						if (lay->revbut->value) {
							lay->bouncebut->value = 2;
							lay->revbut->value = 0;
						}
						else lay->playbut->value = 0;
					}
				}
			}
			else {
				lay->bouncebut->box->acolor[0] = 0.5;
				lay->bouncebut->box->acolor[1] = 0.2;
				lay->bouncebut->box->acolor[2] = 0.5;
				lay->bouncebut->box->acolor[3] = 1.0;
				if (lay->bouncebut->value) {
					lay->bouncebut->box->acolor[0] = 1.0;
					lay->bouncebut->box->acolor[1] = 0.5;
					lay->bouncebut->box->acolor[2] = 0.0;
					lay->bouncebut->box->acolor[3] = 1.0;
				}
			}
			draw_box(lay->bouncebut->box, -1);
			draw_triangle(white, white, lay->bouncebut->box->vtxcoords->x1 + tf(0.00625f), lay->bouncebut->box->vtxcoords->y1 + tf(0.0416f) - tf(0.030f), tf(0.0078f), tf(0.0208f), LEFT, CLOSED);
			draw_triangle(white, white, lay->bouncebut->box->vtxcoords->x1 + tf(0.017f), lay->bouncebut->box->vtxcoords->y1 + tf(0.0416f) - tf(0.030f), tf(0.0078f), tf(0.0208f), RIGHT, CLOSED);
			
			// Draw and handle frameforward framebackward
			if (lay->frameforward->box->in()) {
				lay->frameforward->box->acolor[0] = 0.5;
				lay->frameforward->box->acolor[1] = 0.5;
				lay->frameforward->box->acolor[2] = 1.0;
				lay->frameforward->box->acolor[3] = 1.0;
				if (mainprogram->leftmouse) {
					lay->frame += 1;
					if (lay->frame > lay->numf - 1) lay->frame = 0;
					lay->frameforward->box->acolor[0] = 1.0;
					lay->frameforward->box->acolor[1] = 0.5;
					lay->frameforward->box->acolor[2] = 0.0;
					lay->frameforward->box->acolor[3] = 1.0;
					lay->prevffw = true;
					lay->prevfbw = false;
				}
			}
			else {
				lay->frameforward->box->acolor[0] = 0.5;
				lay->frameforward->box->acolor[1] = 0.2;
				lay->frameforward->box->acolor[2] = 0.5;
				lay->frameforward->box->acolor[3] = 1.0;
			}
			draw_box(lay->frameforward->box, -1);
			draw_triangle(white, white, lay->frameforward->box->vtxcoords->x1 + tf(0.0075f), lay->frameforward->box->vtxcoords->y1 + tf(0.0416f) - tf(0.030f), tf(0.011f), tf(0.0208f), RIGHT, OPEN);
			if (lay->framebackward->box->in()) {
				lay->framebackward->box->acolor[0] = 0.5;
				lay->framebackward->box->acolor[1] = 0.5;
				lay->framebackward->box->acolor[2] = 1.0;
				lay->framebackward->box->acolor[3] = 1.0;
				if (mainprogram->leftmouse) {
					lay->frame -= 1;
					if (lay->frame < 0) lay->frame = lay->numf - 1;
					lay->framebackward->box->acolor[0] = 1.0;
					lay->framebackward->box->acolor[1] = 0.5;
					lay->framebackward->box->acolor[2] = 0.0;
					lay->framebackward->box->acolor[3] = 1.0;
					lay->prevfbw = true;
					lay->prevffw = false;
				}
			}
			else {
				lay->framebackward->box->acolor[0] = 0.5;
				lay->framebackward->box->acolor[1] = 0.2;
				lay->framebackward->box->acolor[2] = 0.5;
				lay->framebackward->box->acolor[3] = 1.0;
			}
			draw_box(lay->framebackward->box, -1);
			draw_triangle(white, white, lay->framebackward->box->vtxcoords->x1 + tf(0.00625f), lay->framebackward->box->vtxcoords->y1 + tf(0.0416f) - tf(0.030f), tf(0.011f), tf(0.0208f), LEFT, OPEN);
		
			// Draw and handle genmidibutton
			if (lay->genmidibut->box->in()) {
				lay->genmidibut->box->acolor[0] = 0.5;
				lay->genmidibut->box->acolor[1] = 0.5;
				lay->genmidibut->box->acolor[2] = 1.0;
				lay->genmidibut->box->acolor[3] = 1.0;
				if (mainprogram->leftmouse) {
					lay->genmidibut->value++;
					if (lay->genmidibut->value == 5) lay->genmidibut->value = 0;
				}
			}
			else {
				lay->genmidibut->box->acolor[0] = 0.5;
				lay->genmidibut->box->acolor[1] = 0.2;
				lay->genmidibut->box->acolor[2] = 0.5;
				lay->genmidibut->box->acolor[3] = 1.0;
				if (lay->genmidibut->value) {
					lay->genmidibut->box->acolor[0] = 1.0;
					lay->genmidibut->box->acolor[1] = 0.5;
					lay->genmidibut->box->acolor[2] = 0.0;
					lay->genmidibut->box->acolor[3] = 1.0;
				}
			}
			draw_box(lay->genmidibut->box, -1);
			std::string butstr;
			if (lay->genmidibut->value == 0) butstr = "off";
			else if (lay->genmidibut->value == 1) butstr = "A";
			else if (lay->genmidibut->value == 2) butstr = "B";
			else if (lay->genmidibut->value == 3) butstr = "C";
			else if (lay->genmidibut->value == 4) butstr = "D";
			render_text(butstr, white, lay->genmidibut->box->vtxcoords->x1 + tf(0.0078f), lay->genmidibut->box->vtxcoords->y1 + tf(0.0416f) - tf(0.030f), tf(0.0003f), tf(0.0005f));
			
			// Draw and handle loopbox
			draw_box(lay->loopbox, -1);
			draw_box(white, green, lay->loopbox->vtxcoords->x1 + lay->startframe * (lay->loopbox->vtxcoords->w / (lay->numf - 1)), lay->loopbox->vtxcoords->y1, (lay->endframe - lay->startframe) * (lay->loopbox->vtxcoords->w / (lay->numf - 1)), tf(0.05f), -1);
			draw_box(white, white, lay->loopbox->vtxcoords->x1 + lay->frame * (lay->loopbox->vtxcoords->w / (lay->numf - 1)), lay->loopbox->vtxcoords->y1, tf(0.00078f), tf(0.05f), -1);
			if (lay->loopbox->in()) {
				if (mainprogram->menuactivation) {
					mainprogram->loopmenu->state = 2;
					mainmix->mouselayer = lay;
					mainprogram->menuactivation = false;
				}	
				if (mainprogram->middlemousedown) {
					if (pdistance(mainprogram->mx, mainprogram->my, lay->loopbox->scrcoords->x1 + lay->startframe * (lay->loopbox->scrcoords->w / (lay->numf - 1)), lay->loopbox->scrcoords->y1, lay->loopbox->scrcoords->x1 + lay->startframe * (lay->loopbox->scrcoords->w / (lay->numf - 1)), lay->loopbox->scrcoords->y1 - mainprogram->yvtxtoscr(0.045f)) < 6) {
						lay->scritching = 2;
						mainprogram->middlemousedown = false;
					}
					if (pdistance(mainprogram->mx, mainprogram->my, lay->loopbox->scrcoords->x1 + lay->startframe * (lay->loopbox->scrcoords->w / (lay->numf - 1)) +  (lay->endframe - lay->startframe) * (lay->loopbox->scrcoords->w / (lay->numf - 1)), lay->loopbox->scrcoords->y1, lay->loopbox->scrcoords->x1 + lay->startframe * (lay->loopbox->scrcoords->w / (lay->numf - 1)) +  (lay->endframe - lay->startframe) * (lay->loopbox->scrcoords->w / (lay->numf - 1)), lay->loopbox->scrcoords->y1 - mainprogram->yvtxtoscr(0.045f)) < 6) {
						lay->scritching = 3;
						mainprogram->middlemousedown = false;
					}
				}
			}
			if (lay->loopbox->in()) {
				if (mainprogram->leftmousedown) {
					lay->scritching = 1;
					lay->frame = (lay->numf - 1) * ((mainprogram->mx - lay->loopbox->scrcoords->x1) / lay->loopbox->scrcoords->w);
				}
			}
			if (lay->scritching) mainprogram->leftmousedown = false;
			if (lay->scritching == 1) {
				lay->frame = (lay->numf - 1) * ((mainprogram->mx - lay->loopbox->scrcoords->x1) / lay->loopbox->scrcoords->w);
				if (lay->frame < 0) lay->frame = 0.0f;
				else if (lay->frame > lay->numf - 1) lay->frame = lay->numf - 1;
				if (mainprogram->leftmouse and !mainprogram->menuondisplay) lay->scritching = 4;
			}
			else if (lay->scritching == 2) {
				lay->startframe = (lay->numf - 1) * ((mainprogram->mx - lay->loopbox->scrcoords->x1) / lay->loopbox->scrcoords->w);
				if (lay->startframe < 0) lay->startframe = 0.0f;
				else if (lay->startframe > lay->numf - 1) lay->startframe = lay->numf - 1;
				if (lay->startframe > lay->frame) lay->frame = lay->startframe;
				if (lay->startframe > lay->endframe) lay->startframe = lay->endframe;
				if (mainprogram->middlemouse) lay->scritching = 0;
			}
			else if (lay->scritching == 3) {
				lay->endframe = (lay->numf - 1) * ((mainprogram->mx - lay->loopbox->scrcoords->x1) / lay->loopbox->scrcoords->w);
				if (lay->endframe < 0) lay->endframe = 0.0f;
				else if (lay->endframe > lay->numf - 1) lay->endframe = lay->numf - 1;
				//if (lay->endframe < lay->frame) lay->frame = lay->endframe;
				//if (lay->endframe < lay->startframe) lay->endframe = lay->startframe;
				if (mainprogram->middlemouse) lay->scritching = 0;
			}
			
			// Set all clone layers to possibly changed values
			lay->set_clones();

			
			// Draw and handle chdir chinv
			if (lay->pos > 0) {
				if (lay->blendnode->blendtype == CHROMAKEY) {
					if (lay->chdir->box->in()) {
						if (mainprogram->leftmouse) {
							mainprogram->leftmouse = false;
							lay->chdir->value = !lay->chdir->value;
							GLint chdir = glGetUniformLocation(mainprogram->ShaderProgram, "chdir");
							glUniform1i(chdir, lay->chdir->value);
						}
					}
					if (lay->chdir->value) {
						lay->chdir->box->acolor[1] = 0.7f;
					}
					else {
						lay->chdir->box->acolor[1] = 0.0f;
					}
					draw_box(lay->chdir->box, -1);
					render_text("D", white, lay->chdir->box->vtxcoords->x1 + tf(0.0078f), lay->chdir->box->vtxcoords->y1 + tf(0.0416f) - tf(0.030f), tf(0.0003f), tf(0.0005f));
					if (lay->chinv->box->in()) {
						if (mainprogram->leftmouse) {
							mainprogram->leftmouse = false;
							lay->chinv->value = !lay->chinv->value;
							GLint chinv = glGetUniformLocation(mainprogram->ShaderProgram, "chinv");
							glUniform1i(chinv, lay->chinv->value);
						}
					}
					if (lay->chinv->value) {
						lay->chinv->box->acolor[1] = 0.7f;
					}
					else {
						lay->chinv->box->acolor[1] = 0.0f;
					}
					draw_box(lay->chinv->box, -1);
					render_text("I", white, lay->chinv->box->vtxcoords->x1 + tf(0.0078f), lay->chinv->box->vtxcoords->y1 + tf(0.0416f) - tf(0.030f), tf(0.0003f), tf(0.0005f));
				}
			}
		}
	}
}

void doblur(bool stage, GLuint prevfbotex, int iter) {
	if (iter == 0) return;
	GLboolean horizontal = true, first_iteration = true;
	GLuint *tex;
	GLint blurring = glGetUniformLocation(mainprogram->ShaderProgram, "blurring");
	glUniform1i(blurring, 1);
	GLint fboSampler = glGetUniformLocation(mainprogram->ShaderProgram, "fboSampler");
	glUniform1i(fboSampler, 0);
	glActiveTexture(GL_TEXTURE0);
	for(GLuint i = 0; i < iter; i++) {
		glBindFramebuffer(GL_FRAMEBUFFER, frbuf[horizontal]);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		if (first_iteration) tex = &prevfbotex;
		else tex = &fbotex[!horizontal];
		glBindTexture(GL_TEXTURE_2D, *tex);
		glUniform1i(glGetUniformLocation(mainprogram->ShaderProgram, "horizontal"), horizontal);
		glBindVertexArray(mainprogram->fbovao[2 - (mainprogram->preveff and !stage)]);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		if (i % 2) glActiveTexture(GL_TEXTURE5);
		else glActiveTexture(GL_TEXTURE6);
		glUniform1i(fboSampler, horizontal + 5);
		horizontal = !horizontal;
		if (first_iteration)
			first_iteration = false;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glUniform1i(blurring, 0);
}

void midi_set() {
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
				loopstation->elems[i]->add_param();
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
	
	
	int wi, he;
	float div;
	if (!mainprogram->preveff) {
		div = mainprogram->ow / glob->w;
	}
	if (mainprogram->preveff and stage == 0) {
		div = 0.3f;
	}
	else {
		div = mainprogram->ow / glob->w;
	}
	
	GLuint fbowidth = glGetUniformLocation(mainprogram->ShaderProgram, "fbowidth");
	glUniform1i(fbowidth, (int)glob->w);
	GLuint fboheight = glGetUniformLocation(mainprogram->ShaderProgram, "fboheight");
	glUniform1i(fboheight, (int)glob->h);
	GLuint fcdiv = glGetUniformLocation(mainprogram->ShaderProgram, "fcdiv");
	glUniform1f(fcdiv, div);
	

	node->walked = true;
	
	midi_set();
	
	if (node->type == EFFECT) {
		Effect *effect = ((EffectNode*)node)->effect;
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
					glUniform1i(blurswitch, 1);
					glBindTexture(GL_TEXTURE_2D, fbotex[0]);
					if (mainprogram->preveff and stage == 0) {
						glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (int)(glob->w * div), (int)(glob->h * div), 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
					}
					else {
						glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mainprogram->ow, mainprogram->oh, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
					}
					glBindTexture(GL_TEXTURE_2D, fbotex[1]);
					if (mainprogram->preveff and stage == 0) {
						glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (int)(glob->w * div), (int)(glob->h * div), 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
					}
					else {
						glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mainprogram->ow, mainprogram->oh, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
					}
					doblur(stage, prevfbotex, ((BlurEffect*)effect)->times);
					glUniform1i(Sampler0, 0);
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, fbotex[1]);
					glUniform1i(blurswitch, 0);
					prevfbotex = fbotex[1];
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
					glUniform1i(blurswitch, 1);
					glBindTexture(GL_TEXTURE_2D, fbotex[0]);
					if (mainprogram->preveff and stage == 0) {
						glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (int)(glob->w * div), (int)(glob->h * div), 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
					}
					else {
						glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mainprogram->ow, mainprogram->oh, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
					}
					glBindTexture(GL_TEXTURE_2D, fbotex[1]);
					if (mainprogram->preveff and stage == 0) {
						glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (int)(glob->w * div), (int)(glob->h * div), 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
					}
					else {
						glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mainprogram->ow, mainprogram->oh, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
					}
					doblur(stage, prevfbotex, 6);
					glUniform1i(Sampler0, 0);
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, fbotex[1]);
					glUniform1i(blurswitch, 0);
					prevfbotex = fbotex[1];
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
					printf("riptime %d\n", effect->get_ripplecount());
					fflush(stdout);
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
					glBindTexture(GL_TEXTURE_2D, fbotex[0]);
					if (mainprogram->preveff and stage == 0) {
						glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (int)(glob->w * div), (int)(glob->h * div), 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
					}
					else {
						glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mainprogram->ow, mainprogram->oh, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
					}
					glBindTexture(GL_TEXTURE_2D, fbotex[1]);
					if (mainprogram->preveff and stage == 0) {
						glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (int)(glob->w * div), (int)(glob->h * div), 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
					}
					else {
						glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mainprogram->ow, mainprogram->oh, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
					}
					glBindFramebuffer(GL_FRAMEBUFFER, frbuf[0]);
					glDrawBuffer(GL_COLOR_ATTACHMENT0);
					glBindTexture(GL_TEXTURE_2D, prevfbotex);
					glBindVertexArray(mainprogram->fbovao[2 - (mainprogram->preveff and !stage)]);
					glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
					
					glUniform1i(interm, 0);
					glUniform1i(edgethickmode, 1);
					GLboolean swits = true;
					GLuint *tex;
					for(GLuint i = 0; i < ((EdgeDetectEffect*)effect)->thickness; i++) {
						glBindFramebuffer(GL_FRAMEBUFFER, frbuf[swits]);
						glDrawBuffer(GL_COLOR_ATTACHMENT0);
						glBindTexture(GL_TEXTURE_2D, fbotex[!swits]);
						glBindVertexArray(mainprogram->fbovao[2 - (mainprogram->preveff and !stage)]);
						glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
						if (i % 2) glActiveTexture(GL_TEXTURE5);
						else glActiveTexture(GL_TEXTURE6);
						glUniform1i(fboSampler, swits + 5);
						swits = !swits;
					}
					glUniform1i(fboSampler, 0);
					glActiveTexture(GL_TEXTURE0);
					glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
					glDrawBuffer(GL_COLOR_ATTACHMENT0);
					if (((EdgeDetectEffect*)effect)->thickness % 2) prevfbotex = fbotex[1];
					else prevfbotex = fbotex[0];
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
			}
		}
		
		glActiveTexture(GL_TEXTURE0);
		if (effect->fbo == -1) {
			glGenTextures(1, &(effect->fbotex));
			glBindTexture(GL_TEXTURE_2D, effect->fbotex);
			if ((mainprogram->preveff and stage == 0)) {
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (int)(glob->w * div), (int)(glob->h * div), 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			}
			else {
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mainprogram->ow, mainprogram->oh, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			}
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glGenFramebuffers(1, &(effect->fbo));
			glBindFramebuffer(GL_FRAMEBUFFER, effect->fbo);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, effect->fbotex, 0);
		}
		glUniform1i(blurswitch, 0);
		GLuint down = glGetUniformLocation(mainprogram->ShaderProgram, "down");
		if (effect->onoffbutton->value) glUniform1i(interm, 1);
		else glUniform1i(down, 1);
		GLfloat opacity = glGetUniformLocation(mainprogram->ShaderProgram, "opacity");
		if (effect->node == (EffectNode*)(effect->layer->lasteffnode)) {
			glUniform1f(opacity, effect->layer->opacity->value);
		}
		else glUniform1f(opacity, 1.0f);
		
		
// #define BUFFER_OFFSET(i) ((char *)NULL + (i))
		// GLuint ioBuf;
 		// glGenBuffers(1, &ioBuf); 
		// glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, ioBuf);
 		// glBufferData(GL_PIXEL_PACK_BUFFER_ARB, mainprogram->ow * mainprogram->oh, NULL, GL_STREAM_READ);
 		// glBindFramebuffer(GL_FRAMEBUFFER, ((VideoNode*)(node->in))->layer->fbo);
		// glReadBuffer(GL_COLOR_ATTACHMENT0);
 		// glReadPixels(0, 0, mainprogram->ow, mainprogram->oh, GL_RGBA, GL_UNSIGNED_BYTE, BUFFER_OFFSET(0));
 		// void* mem = glMapBuffer(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY);   
 		// assert(mem);
		// CopyMemory((void*)effect->pbuf, mem, 2073600);
		// //memcpy((void*)effect->pbuf, "SECRET", 7);
 		// glUnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB);
 		// glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0);		
		// //while((const char*)effect->pbuf != "TERCES") {}
		// //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mainprogram->ow, mainprogram->oh, 0, GL_RGBA, GL_UNSIGNED_BYTE, effect->pbuf);
		
 
 		glBindFramebuffer(GL_FRAMEBUFFER, effect->fbo);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glClearColor( 0.f, 0.f, 0.f, 0.f );
		glClear(GL_COLOR_BUFFER_BIT);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, prevfbotex);
		glBindVertexArray(mainprogram->fbovao[2 - (mainprogram->preveff and !stage)]);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glUniform1i(interm, 0);
		glUniform1i(down, 0);
		glUniform1i(edgethickmode, 0);
		prevfbotex = effect->fbotex;
		prevfbo = effect->fbo;

		Layer *lay = effect->layer;
		if (effect->node == lay->lasteffnode) {
			GLuint fbocopy;
			float fac = 1.0f;
			glDisable(GL_BLEND);
			if (mainprogram->preveff and stage == 0) fbocopy = copy_tex(effect->fbotex);
			else {
				fbocopy = copy_tex(effect->fbotex, mainprogram->ow, mainprogram->oh);
				fac = (glob->w / glob->h) / (mainprogram->ow / mainprogram->oh);
			}
			glBindFramebuffer(GL_FRAMEBUFFER, effect->fbo);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			float black[4] = {0.0f, 0.0f, 0.0f, 0.0f};
			draw_box(NULL, black, -1.0f, -1.0f, 2.0f * div, 2.0f * div * fac, lay->shiftx * div, lay->shifty * div, lay->scale, lay->opacity->value, 0, fbocopy, glob->w, glob->h);
			glEnable(GL_BLEND);
			glDeleteTextures(1, &fbocopy);
		}
		
		glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
	}
	else if (node->type == VIDEO) {
		Layer *lay = ((VideoNode*)node)->layer;
		if (lay->blendnode) {
			if (lay->blendnode->blendtype == 19) {
				GLfloat chromatol = glGetUniformLocation(mainprogram->ShaderProgram, "chromatol");
				glUniform1f(chromatol, lay->chtol->value);
				GLfloat chdir = glGetUniformLocation(mainprogram->ShaderProgram, "chdir");
				glUniform1f(chdir, lay->chdir->value);
				GLfloat chinv = glGetUniformLocation(mainprogram->ShaderProgram, "chinv");
				glUniform1f(chinv, lay->chinv->value);
			}
		}
		if (lay->node == lay->lasteffnode) {
			GLuint fbocopy;
			float fac = 1.0f;
			glDisable(GL_BLEND);
			if ((mainprogram->preveff and stage == 0)) fbocopy = copy_tex(lay->fbotex, glob->w * div, glob->h * div);
			else {
				fbocopy = copy_tex(lay->fbotex, mainprogram->ow, mainprogram->oh);
				fac = (glob->w / glob->h) / (mainprogram->ow / mainprogram->oh);
			}
			glBindFramebuffer(GL_FRAMEBUFFER, lay->fbo);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			float black[4] = {0.0f, 0.0f, 0.0f, 0.0f};
			float pixelw = 2.0f / (float)glob->w;
			float pixelh = 2.0f / (float)glob->h;
			draw_box(NULL, black, -1.0f - pixelw, -1.0f - pixelh, 2.0f * (div + pixelw), 2.0f * (div + pixelh) * fac, lay->shiftx * div, lay->shifty * div, lay->scale, lay->opacity->value, 0, fbocopy, glob->w, glob->h);
			glEnable(GL_BLEND);
			glDeleteTextures(1, &fbocopy);
		}
		prevfbotex = lay->fbotex;
		prevfbo = lay->fbo;
		glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
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
					if (mainprogram->preveff and stage == 0) {
						glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, glob->w * div, glob->h * div, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
					}
					else {	
						glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mainprogram->ow, mainprogram->oh, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
					}
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		
					glGenFramebuffers(1, &(bnode->fbo));
					glBindFramebuffer(GL_FRAMEBUFFER, bnode->fbo);
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bnode->fbotex, 0);
				}
				
				glBindFramebuffer(GL_FRAMEBUFFER, bnode->fbo);
				glDrawBuffer(GL_COLOR_ATTACHMENT0);
				
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
					glUniform1f(xpos, bnode->wipex);
					GLfloat ypos = glGetUniformLocation(mainprogram->ShaderProgram, "ypos");
					glUniform1f(ypos, bnode->wipey);
				}
				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D, bnode->in2tex);
				glBindVertexArray(mainprogram->fbovao[2 - (mainprogram->preveff and !stage)]);
				glDisable(GL_BLEND);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
				glEnable(GL_BLEND);
				prevfbotex = bnode->fbotex;
				prevfbo = bnode->fbo;
				
				GLint inlayer = glGetUniformLocation(mainprogram->ShaderProgram, "inlayer");
				glUniform1f(inlayer, 0);
				glUniform1i(wipe, 0);
				glUniform1i(mixmode, 0);
				if (mainprogram->preveff) {
					glUniform1f(cf, mainmix->crossfade->value);
				}
				else {
					glUniform1f(cf, mainmix->crossfadecomp->value);
				}
				glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
				glDrawBuffer(GL_COLOR_ATTACHMENT0);
			}
			else {
				node->walked = false;
				return;
			}
		}
	}
	else if (node->type == MIX) {
		MixNode *mnode = (MixNode*)node;
		if (mnode->mixfbo == -1) {
			glGenTextures(1, &(mnode->mixtex));
			glBindTexture(GL_TEXTURE_2D, mnode->mixtex);
			if (mainprogram->preveff and stage == 0) {
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (int)(glob->w * div), (int)(glob->h * div), 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			}
			else {
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mainprogram->ow, mainprogram->oh, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			}
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glGenFramebuffers(1, &(mnode->mixfbo));
			glBindFramebuffer(GL_FRAMEBUFFER, mnode->mixfbo);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mnode->mixtex, 0);
		}
		glDisable(GL_BLEND);
		if (mainprogram->preveff and stage == 0) {
			glBlitNamedFramebuffer(prevfbo, mnode->mixfbo, 0, 0, glob->w * div, glob->h * div , 0, 0, glob->w * div, glob->h * div, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		}
		else {
			glBlitNamedFramebuffer(prevfbo, mnode->mixfbo, 0, 0, mainprogram->ow, mainprogram->oh, 0, 0, mainprogram->ow, mainprogram->oh, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		}
		glEnable(GL_BLEND);
		prevfbotex = mnode->mixtex;
		prevfbo = mnode->mixfbo;
	}
	for (int i = 0; i < node->out.size(); i++) {
		if (node->out[i]->calc and !node->out[i]->walked) onestepfrom(stage, node->out[i], node, prevfbotex, prevfbo);
	}
}

void walkback(Node *node) {
	while (node->in and !node->calc) {
		if (node->type == BLEND) {
			if (((BlendNode*)node)->in2) walkback(((BlendNode*)node)->in2);
		}
		node->calc = true;
		node = node->in;
	}
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
				if (node->monitor != -1) {
					if (node->monitor / 6 == mainmix->page[0]) {
						fromnodes.push_back(node);
					}
				}
			}
		}
	}
	for (int i = 0; i < fromnodes.size(); i++) {
		Node *node = fromnodes[i];		
		if (node) walkback(node);
	}
	
	if (stage == 0) {
		for (int i = 0; i < mainmix->layersA.size(); i++) {
			Layer *lay = mainmix->layersA[i];
			onestepfrom(0, lay->node, NULL, -1, -1);
		}
		for (int i = 0; i < mainmix->layersB.size(); i++) {
			Layer *lay = mainmix->layersB[i];
			onestepfrom(0, lay->node, NULL, -1, -1);
		}
	}
	else {
		for (int i = 0; i < mainmix->layersAcomp.size(); i++) {
			Layer *lay = mainmix->layersAcomp[i];
			onestepfrom(1, lay->node, NULL, -1, -1);
		}
		for (int i = 0; i < mainmix->layersBcomp.size(); i++) {
			Layer *lay = mainmix->layersBcomp[i];
			onestepfrom(1, lay->node, NULL, -1, -1);
		}
	}
}		
		

bool displaymix() {
	MixNode *node;
	GLint wipe = glGetUniformLocation(mainprogram->ShaderProgram, "wipe");
	GLint mixmode = glGetUniformLocation(mainprogram->ShaderProgram, "mixmode");
	node = (MixNode*)mainprogram->nodesmain->mixnodes[0];
	node = (MixNode*)mainprogram->nodesmain->mixnodes[1];
	if (mainprogram->preveff) {
		if (mainmix->wipe[0] > -1) {
			glUniform1i(mixmode, 18);
			glUniform1i(wipe, 1);
			GLint wkind = glGetUniformLocation(mainprogram->ShaderProgram, "wkind");
			glUniform1i(wkind, mainmix->wipe[0]);
			GLint dir = glGetUniformLocation(mainprogram->ShaderProgram, "dir");
			glUniform1i(dir, mainmix->wipedir[0]);
			GLfloat xpos = glGetUniformLocation(mainprogram->ShaderProgram, "xpos");
			glUniform1f(xpos, mainmix->wipex[0]);
			GLfloat ypos = glGetUniformLocation(mainprogram->ShaderProgram, "ypos");
			glUniform1f(ypos, mainmix->wipey[0]);
		}
		node = (MixNode*)mainprogram->nodesmain->mixnodes[0];
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, node->mixtex);
		node = (MixNode*)mainprogram->nodesmain->mixnodes[1];
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, node->mixtex);
		glActiveTexture(GL_TEXTURE0);
		node = (MixNode*)mainprogram->nodesmain->mixnodes[2];
		draw_box(node->outputbox->lcolor, node->outputbox->acolor, -0.3f, -1.0f, 0.6f, 0.6f, node->mixtex);
		
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
			glUniform1f(xpos, mainmix->wipex[1]);
			GLfloat ypos = glGetUniformLocation(mainprogram->ShaderProgram, "ypos");
			glUniform1f(ypos, mainmix->wipey[1]);
			node = (MixNode*)mainprogram->nodesmain->mixnodescomp[0];
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, node->mixtex);
			node = (MixNode*)mainprogram->nodesmain->mixnodescomp[1];
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, node->mixtex);
			glActiveTexture(GL_TEXTURE0);
		}
		node = (MixNode*)mainprogram->nodesmain->mixnodescomp[2];
		draw_box(node->outputbox->lcolor, node->outputbox->acolor, -0.15f, -0.4f + tf(0.05f), 0.3f, 0.3f, node->mixtex);
		GLfloat cf = glGetUniformLocation(mainprogram->ShaderProgram, "cf");
		if (mainprogram->preveff) {
			glUniform1f(cf, mainmix->crossfade->value);
		}
		else {
			glUniform1f(cf, mainmix->crossfadecomp->value);
		}
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
			glUniform1f(xpos, mainmix->wipex[1]);
			GLfloat ypos = glGetUniformLocation(mainprogram->ShaderProgram, "ypos");
			glUniform1f(ypos, mainmix->wipey[1]);
			node = (MixNode*)mainprogram->nodesmain->mixnodescomp[0];
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, node->mixtex);
			node = (MixNode*)mainprogram->nodesmain->mixnodescomp[1];
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, node->mixtex);
			glActiveTexture(GL_TEXTURE0);
		}
		node = (MixNode*)mainprogram->nodesmain->mixnodescomp[2];
		draw_box(node->outputbox->lcolor, node->outputbox->acolor, -0.3f, -1.0f, 0.6f, 0.6f, node->mixtex);
		GLfloat cf = glGetUniformLocation(mainprogram->ShaderProgram, "cf");
		if (mainprogram->preveff) {
			glUniform1f(cf, mainmix->crossfade->value);
		}
		else {
			glUniform1f(cf, mainmix->crossfadecomp->value);
		}
	}
	glUniform1i(wipe, 0);
	glUniform1i(mixmode, 0);
	
	if (mainprogram->preveff) {
		node = (MixNode*)mainprogram->nodesmain->mixnodes[0];
		draw_box(node->outputbox->lcolor, node->outputbox->acolor, -0.6f, -1.0f, 0.3f, 0.3f, node->mixtex);
		node = (MixNode*)mainprogram->nodesmain->mixnodes[1];
		draw_box(node->outputbox->lcolor, node->outputbox->acolor, 0.3f, -1.0f, 0.3f, 0.3f, node->mixtex);
	}
	else {
		node = (MixNode*)mainprogram->nodesmain->mixnodescomp[0];
		draw_box(node->outputbox->lcolor, node->outputbox->acolor, -0.6f, -1.0f, 0.3f, 0.3f, node->mixtex);
		node = (MixNode*)mainprogram->nodesmain->mixnodescomp[1];
		draw_box(node->outputbox->lcolor, node->outputbox->acolor, 0.3f, -1.0f, 0.3f, 0.3f, node->mixtex);
	}
	
	return true;
}


bool check_thumbs(std::vector<Layer*> &layers, bool deck) {
	float white[] = {1.0f, 1.0f, 1.0f, 1.0f};
	float transp[] = {0.0f, 0.0f, 0.0f, 0.0f};
	float black[] = {0.0f, 0.0f, 0.0f, 1.0f};
	float lightblue[] = {0.5f, 0.5f, 1.0f, 1.0f};

	Layer *lay;
	for (int i = 0; i < layers.size(); i++) {
		lay = layers[i];
		if (lay->pos < mainmix->scrollpos[deck] or lay->pos > mainmix->scrollpos[deck] + 2) continue;
		Box *box = lay->node->vidbox;
		int endx = false;
		if ((i == layers.size() - 1 or i == mainmix->scrollpos[deck] + 2) and (box->scrcoords->x1 + box->scrcoords->w - mainprogram->xvtxtoscr(tf(0.075f)) < mainprogram->mx and mainprogram->mx < box->scrcoords->x1 + box->scrcoords->w + mainprogram->xvtxtoscr(0.075f))) {
			endx = true;
		}
		
		// handle clip queue?
		if (lay->queueing and lay->filename != "") {
			if (box->scrcoords->x1 + (i == 0) * mainprogram->xvtxtoscr(tf(0.075)) < mainprogram->mx and mainprogram->mx < box->scrcoords->x1 + box->scrcoords->w - (i == 0) * mainprogram->xvtxtoscr(tf(0.075))) {
				for (int j = 0; j < lay->clips.size() - lay->queuescroll; j++) {
					float last = (j == lay->clips.size() - lay->queuescroll - 1) * 0.25f;
					if (box->scrcoords->y1 + (j - 0.25f) * box->scrcoords->h < mainprogram->my and mainprogram->my < box->scrcoords->y1 + (j + 0.25) * box->scrcoords->h) {
						// inserting
						if (mainprogram->leftmouse) {
							if (mainprogram->dragbinel) {
								if (mainprogram->draglay == lay and j == 0) {
									mainprogram->leftmouse = false;
									enddrag();
									return true;
								}
							}
							if (mainprogram->dragbinel) {
								mainprogram->leftmouse = false;
								Clip *nc = new Clip;
								nc->tex = copy_tex(mainprogram->dragbinel->tex);
								nc->type = mainprogram->dragbinel->type;
								nc->path = mainprogram->dragbinel->path;
								lay->clips.insert(lay->clips.begin() + j + lay->queuescroll, nc);
								enddrag();
								if (j + lay->queuescroll == lay->clips.size() - 1) {
									Clip *clip = new Clip;
									lay->clips.push_back(clip);
									if (lay->clips.size() > 4) lay->queuescroll++;
								}
								return true;
							}
						}
					}
					else if (box->scrcoords->y1 + (j + 0.25f) * box->scrcoords->h < mainprogram->my and mainprogram->my < box->scrcoords->y1 + (j + 0.75 + last) * box->scrcoords->h) {
						// replacing
						Clip *jc = lay->clips[j + lay->queuescroll];
						if (mainprogram->leftmouse) {
							if (mainprogram->dragbinel) {
								mainprogram->leftmouse = false;
								if (mainprogram->dragclip) {
									mainprogram->dragclip->tex = jc->tex;
									mainprogram->dragclip->type = jc->type;
									mainprogram->dragclip->path = jc->path;
									mainprogram->draglay->clips.insert(mainprogram->draglay->clips.begin() + mainprogram->dragpos, mainprogram->dragclip);
									mainprogram->dragclip = nullptr;
								}
								jc->tex = copy_tex(mainprogram->dragbinel->tex);
								jc->type = mainprogram->dragbinel->type;
								jc->path = mainprogram->dragbinel->path;
								enddrag();
								if (j + lay->queuescroll == lay->clips.size() - 1) {
									Clip *clip = new Clip;
									lay->clips.push_back(clip);
									if (lay->clips.size() > 4) lay->queuescroll++;
								}
								return true;
							}
						}
					}
				}
			}
		}

		if (!mainmix->moving) {
			if (box->scrcoords->y1 < mainprogram->my + box->scrcoords->h and mainprogram->my < box->scrcoords->y1) {
				if (box->scrcoords->x1 + box->scrcoords->w * 0.25f < mainprogram->mx and mainprogram->mx < box->scrcoords->x1 + box->scrcoords->w * 0.75f) {
					//clips queue
					set_queueing(lay, true);
					mainmix->currlay = lay;
					if (mainprogram->leftmouse) {
						mainprogram->leftmouse = false;
						set_queueing(lay, false);
						if (mainprogram->dragbinel) {
							if (mainprogram->dragbinel->type == ELEM_LAYER) {
								mainmix->open_layerfile(mainprogram->dragbinel->path, lay, 1, 1);
							}
							else {
								open_video(0, lay, mainprogram->dragbinel->path, true);
							}
						}
						else {
							open_video(0, lay,  mainprogram->dragbinel->path, true);
						}
						enddrag();
						return 1;
					}
				}
				else if (endx or (box->scrcoords->x1 - mainprogram->xvtxtoscr(0.075f) < mainprogram->mx and mainprogram->mx < box->scrcoords->x1 + mainprogram->xvtxtoscr(0.075f))) {
					if (mainprogram->leftmouse) {
						mainprogram->leftmouse = false;
						Layer *inlay = mainmix->add_layer(layers, lay->pos + endx);
						if (inlay->pos == mainmix->scrollpos[deck] + 3) mainmix->scrollpos[deck]++;
						if (mainprogram->dragbinel) {
							if (mainprogram->dragbinel->type == ELEM_LAYER) {
								mainmix->open_layerfile(mainprogram->dragbinel->path, inlay, 1, 1);
							}
							else {
								open_video(0, inlay, mainprogram->dragbinel->path, true);
							}
							enddrag();
						}
						else {
							open_video(0, inlay,  mainprogram->dragbinel->path, true);
						}
						return 1;
					}
				}
			}

		
			int numonscreen = layers.size() - mainmix->scrollpos[deck];
			if (0 <= numonscreen and numonscreen <= 2) {
				if (mainprogram->xvtxtoscr(mainmix->numw + deck * 1.0f + numonscreen * mainmix->layw) < mainprogram->mx and mainprogram->mx < deck * (glob->w / 2.0f) + glob->w / 2.0f) {
					if (0 < mainprogram->my and mainprogram->my < mainprogram->yvtxtoscr(mainmix->layw)) {
						float blue[] = {0.5, 0.5 , 1.0, 1.0};
						draw_box(nullptr, blue, -1.0f + mainmix->numw + deck * 1.0f + numonscreen * mainmix->layw, 1.0f - mainmix->layw, mainmix->layw, mainmix->layw, -1);
						if (mainprogram->leftmouse) {
							mainprogram->leftmouse = false;
							lay = mainmix->add_layer(layers, layers.size());
							if (mainprogram->dragbinel) {
								if (mainprogram->dragbinel->type == ELEM_LAYER) {
									mainmix->open_layerfile(mainprogram->dragbinel->path, lay, 1, 1);
								}
								else {
									open_video(0, lay, mainprogram->dragbinel->path, true);
								}
								enddrag();
							}
							else {
								open_video(0, lay,  mainprogram->dragbinel->path, true);
							}
							return 1;
						}
					}
				}
			}
		}
		else return 1;
	}
	return 0;
}

// check all STATIC/DYNAMIC draws
void visu_thumbs() {
	// draw shelves and handle shelves
	GLint thumb = glGetUniformLocation(mainprogram->ShaderProgram, "thumb");
	GLint box = glGetUniformLocation(mainprogram->ShaderProgram, "box");
	glUniform1i(thumb, 1);
	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_BLEND);
	for (int m = 0; m < 2; m++) {
		for(int i = 0; i < 4; i++) {
			for(int j = 0; j < 4; j++) {
				int k = m * 16 + i * 4 + j;
				if (k < thpath.size()) {
					glBindTexture(GL_TEXTURE_2D, thumbtex[k]);
					glBindVertexArray(thvao[k]);
					glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
				}
				glUniform1i(box, 1);
				glBindVertexArray(thbvao[k]);
				glDrawArrays(GL_LINE_LOOP, 0, 4);
				glUniform1i(box, 0);
				if (mainprogram->xvtxtoscr(0.1f) * j + (m * glob->w) - mainprogram->xvtxtoscr(0.1f) * 4 * m < mainprogram->mx and mainprogram->mx < mainprogram->xvtxtoscr(0.1f) * (j + 1) + (m * glob->w) - mainprogram->xvtxtoscr(0.1f) * 4 * m) {
					if (glob->h - mainprogram->yvtxtoscr(0.1f) * (4 - i) < mainprogram->my and mainprogram->my < (glob->h - mainprogram->yvtxtoscr(0.1f) * (3 - i))) {
						if (mainprogram->leftmousedown) {
							if (!mainprogram->dragbinel) {
								// user starts dragging shelf element
								mainprogram->leftmousedown = false;
								mainprogram->dragbinel = new BinElement;
								mainprogram->dragbinel->path = thpath[k];
								mainprogram->dragbinel->type = thtype[k];
								mainprogram->dragbinel->tex = thumbtex[k];
							}
						}
						else if (mainprogram->leftmouse) {
							if (mainprogram->dragbinel) {
								mainprogram->leftmouse = false;
								// user drops something in shelf element
								if (mainprogram->dragbinel->path.rfind(".layer") != std::string::npos) {
									std::string base = basename(mainprogram->dragbinel->path);
									std::string newpath;
									std::string name = remove_extension("shelf_" + base.substr(9, std::string::npos));
									int count = 0;
									while (1) {
										newpath = mainprogram->temppath + name + ".layer";
										if (!exists(newpath)) {
											if (mainprogram->dragbinel->path.find("cliptemp_") != std::string::npos) {
												boost::filesystem::rename(mainprogram->dragbinel->path, newpath);			
												boost::filesystem::remove(mainprogram->dragbinel->path);			
												mainprogram->dragbinel->path = newpath;
											}
											break;
										}
										count++;
										name = remove_version(name) + "_" + std::to_string(count);
									}
								}
								thpath[k] = mainprogram->dragbinel->path;
								thtype[k] = mainprogram->dragbinel->type;
								thumbtex[k] = copy_tex(mainprogram->dragbinel->tex);
								enddrag();
								#ifdef _WIN64
								if (mainprogram->mainshelf) save_shelf("./shelves/shelfs.shelf", 2);
								# else
								#ifdef __linux__
								std::string homedir (getenv("HOME"));
								if (mainprogram->mainshelf) save_shelf(homedir + "/.ewocvj2/shelves/shelfs.shelf", 2);
								if (exists(homedir + "/.ewocvj2/shelves/shelfs.shelf")) open_shelf(homedir + "/.ewocvj2/shelves/shelfs.shelf", 2);
								#endif
								#endif
							}
						}
						else if (mainprogram->menuactivation) {
							if (mainprogram->mainshelf) mainprogram->shelfmenu1->state = 2;
							else mainprogram->shelfmenu2->state = 2;
							mainmix->mouseshelfelem = k;
							mainmix->mousedeck = (k > 15);
							mainprogram->menuactivation = false;
						}
					}	
				}
			}
		}
	}
	glEnable(GL_BLEND);
	if (mainprogram->dragbinel) {
		if (mainprogram->preveff) {
			if (check_thumbs(mainmix->layersA, 0)) return;
			if (check_thumbs(mainmix->layersB, 1)) return;
		}
		else {
			if (check_thumbs(mainmix->layersAcomp, 0)) return;
			if (check_thumbs(mainmix->layersBcomp, 1)) return;
		}
		if (mainprogram->leftmouse) {
			mainprogram->leftmouse = 0;
			enddrag();
		}
	}
	glUniform1i(thumb, 0);
	
	if (!mainprogram->binsscreen) {
		std::vector<Layer*> &lvec1 = choose_layers(0);
		for (int i = 0; i < lvec1.size(); i++) {
			if (mainprogram->leftmouse) set_queueing(lvec1[i], false);
		}
		std::vector<Layer*> &lvec2 = choose_layers(1);
		for (int i = 0; i < lvec2.size(); i++) {
			if (mainprogram->leftmouse) set_queueing(lvec2[i], false);
		}
		if (mainprogram->leftmouse and mainprogram->dragclip) {
			delete mainprogram->dragclip;
			mainprogram->dragclip = nullptr;
		}
	}
}


void make_layboxes() {

	for (int i = 0; i < mainprogram->nodesmain->pages.size(); i++) {
		for (int j = 0; j < mainprogram->nodesmain->pages[i]->nodescomp.size(); j++) {
			Node *node = mainprogram->nodesmain->pages[i]->nodescomp[j];
			
			if (mainprogram->nodesmain->linked) {
				if (node->type == VIDEO) {
					float xoffset;
					int deck;
					if (std::find(mainmix->layersAcomp.begin(), mainmix->layersAcomp.end(), ((VideoNode*)node)->layer) != mainmix->layersAcomp.end()) {
						xoffset = 0.0f;
						deck = 0;
					}
					else {
						xoffset = 1.0f;
						deck = 1;
					}
					node->vidbox->vtxcoords->x1 = -1.0f + (float)((((VideoNode*)node)->layer->pos - mainmix->scrollpos[deck] + 9999) % 3) * mainmix->layw + mainmix->numw + xoffset;
					node->vidbox->vtxcoords->y1 = 1.0f - mainmix->layw;
					node->vidbox->vtxcoords->w = mainmix->layw;
					node->vidbox->vtxcoords->h = mainmix->layw;
					node->vidbox->upvtxtoscr();
					
					node->vidbox->lcolor[0] = 1.0;
					node->vidbox->lcolor[1] = 1.0;
					node->vidbox->lcolor[2] = 1.0;
					node->vidbox->lcolor[3] = 1.0;
					if (((VideoNode*)node)->layer->effects.size() == 0) {
						node->vidbox->tex = ((VideoNode*)node)->layer->fbotex;
					}
					else {
						node->vidbox->tex = ((VideoNode*)node)->layer->effects[((VideoNode*)node)->layer->effects.size() - 1]->fbotex;
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
					float xoffset;
					int deck;
					if (std::find(mainmix->layersA.begin(), mainmix->layersA.end(), ((VideoNode*)node)->layer) != mainmix->layersA.end()) {
						xoffset = 0.0f;
						deck = 0;
					}
					else {
						xoffset = 1.0f;
						deck = 1;
					}
					node->vidbox->vtxcoords->x1 = -1.0f + (float)((((VideoNode*)node)->layer->pos - mainmix->scrollpos[deck] + 9999) % 3) * mainmix->layw + mainmix->numw + xoffset;
					if (((VideoNode*)node)->layer->effects.size() == 0) {
						node->vidbox->tex = ((VideoNode*)node)->layer->fbotex;
					}
					else {
						node->vidbox->tex = ((VideoNode*)node)->layer->effects[((VideoNode*)node)->layer->effects.size() - 1]->fbotex;
					}
				}
				else continue;
			}
			else {
				if (node->monitor != -1) {
					node->vidbox->vtxcoords->x1 = -1.0f + (float)(node->monitor % 6) * mainmix->layw + mainmix->numw;
					if (node->type == VIDEO) {
						node->vidbox->tex = ((VideoNode*)node)->layer->fbotex;
					}
					else if (node->type == EFFECT) {
						node->vidbox->tex = ((EffectNode*)node)->effect->fbotex;
					}
					else if (node->type == MIX) {
						node->vidbox->tex = ((MixNode*)node)->mixtex;
					}
					else if (node->type == BLEND) {
						node->vidbox->tex = ((BlendNode*)node)->fbotex;
					}
				}
				else continue;
			}
			node->vidbox->vtxcoords->y1 = 1.0f - mainmix->layw;
			node->vidbox->vtxcoords->w = mainmix->layw;
			node->vidbox->vtxcoords->h = mainmix->layw;
			node->vidbox->upvtxtoscr();
			
			node->vidbox->lcolor[0] = 1.0;
			node->vidbox->lcolor[1] = 1.0;
			node->vidbox->lcolor[2] = 1.0;
			node->vidbox->lcolor[3] = 1.0;
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
				xoffset = 1.0f + mainmix->layw - 0.06f;
			}
			else if (j == 2) {
				lvec = mainmix->layersAcomp;
				xoffset = 0.0f;
			}
			else if (j == 3) {
				lvec = mainmix->layersBcomp;
				xoffset = 1.0f + mainmix->layw - 0.019f;
			}
			for (int i = 0; i < lvec.size(); i++) {
				Layer *testlay = lvec[i];
				testlay->node->upeffboxes();
				// Make mixbox
				testlay->mixbox->vtxcoords->x1 = -1.0f + mainmix->numw + xoffset + (testlay->pos % 3) * tf(0.04f);
				testlay->mixbox->vtxcoords->y1 = 1.0f - tf(mainmix->layw);
				testlay->mixbox->vtxcoords->w = tf(0.05f);
				testlay->mixbox->vtxcoords->h = tf(0.05f);
				testlay->mixbox->upvtxtoscr();
				testlay->chromabox->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + tf(0.07f);
				testlay->chromabox->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1;
				testlay->chromabox->vtxcoords->w = tf(0.05f);
				testlay->chromabox->vtxcoords->h = tf(0.05f);
				testlay->chromabox->upvtxtoscr();
		
				// shift effectboxes and parameterboxes according to position in stack and scrollposition of stack
				Effect *preveff = NULL;
				for (int j = 0; j < testlay->effects.size(); j++) {
					Effect *eff = testlay->effects[j];
					eff->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + tf(0.05f) - (testlay->pos % 3) * tf(0.04f);
					eff->onoffbutton->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + tf(0.025f) - (testlay->pos % 3) * tf(0.04f);
					eff->drywet->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 - (testlay->pos % 3) * tf(0.04f);
					float dy;
					if (preveff) {
						if (preveff->params.size() < 4) {
							eff->box->vtxcoords->y1 = preveff->box->vtxcoords->y1 - tf(0.05f);
						}
						else if (preveff->params.size() > 5) {
							eff->box->vtxcoords->y1 = preveff->box->vtxcoords->y1 - tf(0.15f);
						}
						else {
							eff->box->vtxcoords->y1 = preveff->box->vtxcoords->y1 - tf(0.1f);
						}
					}
					else {
						eff->box->vtxcoords->y1 = 1.0 - tf(mainmix->layw) - tf(0.20f) + (tf(0.05f) * testlay->effscroll);
					}
					eff->box->upvtxtoscr();
					eff->onoffbutton->box->vtxcoords->y1 = eff->box->vtxcoords->y1;
					eff->onoffbutton->box->upvtxtoscr();
					eff->drywet->box->vtxcoords->y1 = eff->box->vtxcoords->y1;
					eff->drywet->box->upvtxtoscr();
					preveff = eff;
					
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
				testlay->blendnode->mixfac->box->vtxcoords->w = tf(mainmix->layw) * 0.25f;
				testlay->blendnode->mixfac->box->vtxcoords->h = tf(0.05f);
				testlay->blendnode->mixfac->box->upvtxtoscr();
				
				// Make GUI box of video speed slider
				testlay->speed->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1;
				testlay->speed->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - tf(0.05f);
				testlay->speed->box->vtxcoords->w = tf(mainmix->layw) * 0.5f;
				testlay->speed->box->vtxcoords->h = tf(0.05f);
				testlay->speed->box->upvtxtoscr();
				
				// GUI box of layer opacity slider
				testlay->opacity->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1;
				testlay->opacity->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - tf(0.15f);
				testlay->opacity->box->vtxcoords->w = tf(mainmix->layw) * 0.25f;
				testlay->opacity->box->vtxcoords->h = tf(0.05f);
				testlay->opacity->box->upvtxtoscr();
				
				// GUI box of layer audio volume slider
				testlay->volume->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + tf(mainmix->layw) * 0.30f;
				testlay->volume->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - tf(0.15f);
				testlay->volume->box->vtxcoords->w = tf(mainmix->layw) * 0.25f;
				testlay->volume->box->vtxcoords->h = tf(0.05f);
				testlay->volume->box->upvtxtoscr();
				
				// GUI box of play video button
				testlay->playbut->box->acolor[0] = 0.5;
				testlay->playbut->box->acolor[1] = 0.2;
				testlay->playbut->box->acolor[2] = 0.5;
				testlay->playbut->box->acolor[3] = 1.0;
				testlay->playbut->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + tf(mainmix->layw) * 0.5f + tf(0.088f);
				testlay->playbut->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - tf(0.05f);
				testlay->playbut->box->vtxcoords->w = tf(0.025f);
				testlay->playbut->box->vtxcoords->h = tf(0.05f);
				testlay->playbut->box->upvtxtoscr();
				
				// GUI box of bounce play video button
				testlay->bouncebut->box->acolor[0] = 0.5;
				testlay->bouncebut->box->acolor[1] = 0.2;
				testlay->bouncebut->box->acolor[2] = 0.5;
				testlay->bouncebut->box->acolor[3] = 1.0;
				testlay->bouncebut->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + tf(mainmix->layw) * 0.5f + tf(0.058f);
				testlay->bouncebut->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - tf(0.05f);
				testlay->bouncebut->box->vtxcoords->w = tf(0.03f);
				testlay->bouncebut->box->vtxcoords->h = tf(0.05f);
				testlay->bouncebut->box->upvtxtoscr();
				
				// GUI box of reverse play video button
				testlay->revbut->box->acolor[0] = 0.5;
				testlay->revbut->box->acolor[1] = 0.2;
				testlay->revbut->box->acolor[2] = 0.5;
				testlay->revbut->box->acolor[3] = 1.0;
				testlay->revbut->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + tf(mainmix->layw) * 0.5f + tf(0.033f);
				testlay->revbut->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - tf(0.05f);
				testlay->revbut->box->vtxcoords->w = tf(0.025f);
				testlay->revbut->box->vtxcoords->h = tf(0.05f);
				testlay->revbut->box->upvtxtoscr();
				
				// GUI box of video frame forward button
				testlay->frameforward->box->acolor[0] = 0.5;
				testlay->frameforward->box->acolor[1] = 0.2;
				testlay->frameforward->box->acolor[2] = 0.5;
				testlay->frameforward->box->acolor[3] = 1.0;
				testlay->frameforward->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + tf(mainmix->layw) * 0.5f + tf(0.113f);
				testlay->frameforward->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - tf(0.05f);
				testlay->frameforward->box->vtxcoords->w = tf(0.025f);
				testlay->frameforward->box->vtxcoords->h = tf(0.05f);
				testlay->frameforward->box->upvtxtoscr();
				
				// GUI box of video frame backward button
				testlay->framebackward->box->acolor[0] = 0.5;
				testlay->framebackward->box->acolor[1] = 0.2;
				testlay->framebackward->box->acolor[2] = 0.5;
				testlay->framebackward->box->acolor[3] = 1.0;
				testlay->framebackward->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + tf(mainmix->layw) * 0.5f + tf(0.008f);
				testlay->framebackward->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - tf(0.05f);
				testlay->framebackward->box->vtxcoords->w = tf(0.025f);
				testlay->framebackward->box->vtxcoords->h = tf(0.05f);
				testlay->framebackward->box->upvtxtoscr();
				
				// GUI box of specific general midi set for layer switch
				testlay->genmidibut->box->acolor[0] = 0.5;
				testlay->genmidibut->box->acolor[1] = 0.2;
				testlay->genmidibut->box->acolor[2] = 0.5;
				testlay->genmidibut->box->acolor[3] = 1.0;
				testlay->genmidibut->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + tf(mainmix->layw) * 0.5f + tf(0.138f);
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
				testlay->loopbox->vtxcoords->w = tf(mainmix->layw);
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
				
				// GUI box of chromakey direction switch
				testlay->chdir->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + tf(0.24f);
				testlay->chdir->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1;
				testlay->chdir->box->vtxcoords->w = tf(0.025f);
				testlay->chdir->box->vtxcoords->h = tf(0.05f);
				testlay->chdir->box->upvtxtoscr();
				
				// GUI box of chromakey inversion switch
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
	printf("value %f\n", argv[0]->f);
	fflush(stdout);
	return 0;
}

// adds an effect of a certain type to a layer at a certain position in its effect list
// comp is set when the layer belongs to the output layer stacks
Effect *do_add_effect(Layer *lay, EFFECT_TYPE type, int pos, bool comp) {
	Effect *effect;
	switch (type) {
		case BLUR:
			effect = new BlurEffect();
			break;
		case BRIGHTNESS:
			effect = new BrightnessEffect();
			break;
		case CHROMAROTATE:
			effect = new ChromarotateEffect();
			break;
		case CONTRAST:
			effect = new ContrastEffect();
			break;
		case DOT:
			effect = new DotEffect();
			break;
		case GLOW:
			effect = new GlowEffect();
			break;
		case RADIALBLUR:
			effect = new RadialblurEffect();
			break;
		case SATURATION:
			effect = new SaturationEffect();
			break;
		case SCALE:
			effect = new ScaleEffect();
			break;
		case SWIRL:
			effect = new SwirlEffect();
			break;
		case OLDFILM:
			effect = new OldFilmEffect();
			break;
		case RIPPLE:
			effect = new RippleEffect();
			((RippleEffect*)(effect))->speed = ((RippleEffect*)(effect))->params[0]->value;			
			break;
		case FISHEYE:
			effect = new FishEyeEffect();
			break;
		case TRESHOLD:
			effect = new TresholdEffect();
			break;
		case STROBE:
			effect = new StrobeEffect();
			break;
		case POSTERIZE:
			effect = new PosterizeEffect();
			break;
		case PIXELATE:
			effect = new PixelateEffect();
			break;
		case CROSSHATCH:
			effect = new CrosshatchEffect();
			break;
		case INVERT:
			effect = new InvertEffect();
			break;
		case ROTATE:
			effect = new RotateEffect();
			break;
		case EMBOSS:
			effect = new EmbossEffect();
			break;
		case ASCII:
			effect = new AsciiEffect();
			break;
		case SOLARIZE:
			effect = new SolarizeEffect();
			break;
		case VARDOT:
			effect = new VarDotEffect();
			break;
		case CRT:
			effect = new CRTEffect();
			break;
		case EDGEDETECT:
			effect = new EdgeDetectEffect();
			break;
		case KALEIDOSCOPE:
			effect = new KaleidoScopeEffect();
			break;
		case HTONE:
			effect = new HalfToneEffect();
			break;
		case CARTOON:
			effect = new CartoonEffect();
			break;
		case CUTOFF:
			effect = new CutoffEffect();
			break;
		case GLITCH:
			effect = new GlitchEffect();
			break;
		case COLORIZE:
			effect = new ColorizeEffect();
			break;
		case NOISE:
			effect = new NoiseEffect();
			break;
		case GAMMA:
			effect = new GammaEffect();
			break;
		case THERMAL:
			effect = new ThermalEffect();
			break;
		case BOKEH:
			effect = new BokehEffect();
			break;
		case SHARPEN:
			effect = new SharpenEffect();
			break;
		case DITHER:
			effect = new DitherEffect();
			break;
	}

	effect->type = type;
	effect->pos = pos;
	effect->layer = lay;
	

	// does scrolling when effect stack reaches bottom of GUI space
	lay->numefflines += effect->numrows;
	if (lay->numefflines > 11) {
		int further = (lay->numefflines - lay->effscroll) - 11;
		lay->effscroll = lay->effscroll + further * (further > 0);
		int linepos = 0;
		for (int i = 0; i < effect->pos; i++) {
			linepos += lay->effects[i]->numrows;
		}
		if (lay->effscroll > linepos) lay->effscroll = linepos;
	}
	
	lay->effects.insert(lay->effects.begin() + pos, effect);

	
	EffectNode *effnode1 = mainprogram->nodesmain->currpage->add_effectnode(effect, lay->node, pos, comp);
	effnode1->align = lay->node;
	effnode1->alignpos = pos;
	lay->node->aligned += 1;
	
	if (pos == lay->effects.size() - 1) {
		if (lay->pos > 0 and lay->blendnode) {
			lay->blendnode->in2 = nullptr;
			mainprogram->nodesmain->currpage->connect_in2(effnode1, lay->blendnode);
		}
		else if (lay->lasteffnode->out.size()) {
			lay->lasteffnode->out[0]->in = nullptr;
			mainprogram->nodesmain->currpage->connect_nodes(effnode1, lay->lasteffnode->out[0]);
		}
		lay->lasteffnode->out.clear();
		lay->lasteffnode = effnode1;
	}

	if (pos == 0) {
		if (lay->effects.size() > 1) {
			EffectNode *effnode2 = lay->effects[pos + 1]->node;
			mainprogram->nodesmain->currpage->connect_nodes(effnode1, effnode2);
		}
		lay->node->out.clear();
		mainprogram->nodesmain->currpage->connect_nodes(lay->node, effnode1);
	}
	else {
		EffectNode *effnode2 = lay->effects[pos - 1]->node;
		if (lay->effects.size() > pos + 1) {
			EffectNode *effnode3 = lay->effects[pos + 1]->node;
			mainprogram->nodesmain->currpage->connect_nodes(effnode1, effnode3);
		}
		effnode2->out.clear();
		mainprogram->nodesmain->currpage->connect_nodes(effnode2, effnode1);
	}
	
	// add parameters to OSC ssystem
	if (lay->numoftypemap.find(effect->type) != lay->numoftypemap.end()) lay->numoftypemap[effect->type]++;
	else lay->numoftypemap[effect->type] = 1;
	for (int i = 0; i < effect->params.size(); i++) {
		std::string deckstr;
		if (lay->deck == 0) deckstr = "/deckA";
		else deckstr = "/deckB";
		std::string compstr;
		if (mainprogram->preveff) compstr = "preview";
		else compstr = "output";
		std::string path = "/mix/" + compstr + deckstr + "/layer/" + std::to_string(lay->pos + 1) + "/effect/" + mainprogram->effectsmap[effect->type] + "/" + std::to_string(lay->numoftypemap[effect->type]) + "/" + effect->params[i]->name;
		effect->params[i]->oscpaths.push_back(path);
		printf("osc %s\n", path.c_str());
		fflush(stdout);
		mainprogram->st->add_method(path, "f", osc_param, effect->params[i]);
	}
	
	return effect;
}
Effect* Layer::add_effect(EFFECT_TYPE type, int pos) {
	Effect *eff;

	if (mainprogram->preveff) {
		eff = do_add_effect(this, type, pos, false);
	}
	else {
		eff = do_add_effect(this, type, pos, true);
	}
	this->currcliptype = ELEM_LAYER;
	return eff;
}		

Effect* Layer::replace_effect(EFFECT_TYPE type, int pos) {
	Effect *effect;

	this->delete_effect(pos);
	effect = this->add_effect(type, pos);
	
	return effect;
}

void do_delete_effect(Layer *lay, int pos) {
	Effect *effect = lay->effects[pos];
	
	for(int i = pos; i < lay->effects.size(); i++) {
		Effect *eff = lay->effects[i];
		eff->node->alignpos -= 1;
	}
	lay->node->aligned -= 1;
	
	for (int i = 0; i < lay->node->page->nodes.size(); i++) {
		if (lay->effects[pos]->node == lay->node->page->nodes[i]) {
			lay->node->page->nodes.erase(lay->node->page->nodes.begin() + i);
		}
	}
	if (lay->lasteffnode == lay->effects[pos]->node) {
		if (pos == 0) lay->lasteffnode = lay->node;
		else lay->lasteffnode = lay->effects[pos - 1]->node;
		lay->lasteffnode->out = lay->effects[pos]->node->out;
		if (lay->pos == 0) {
			lay->lasteffnode->out[0]->in = lay->lasteffnode;
		}
		else {
			((BlendNode*)lay->lasteffnode->out[0])->in2 = lay->lasteffnode;
		}
	}
	else {
		lay->effects[pos + 1]->node->in = lay->effects[pos]->node->in;
		if (pos != 0) lay->effects[pos - 1]->node->out.push_back(lay->effects[pos + 1]->node);
		else lay->node->out.push_back(lay->effects[pos + 1]->node);
	}	
	
	lay->node->page->delete_node(lay->effects[pos]->node);
	lay->node->upeffboxes();

	lay->effects.erase(lay->effects.begin() + pos);
	delete effect;
	
	make_layboxes();
}

void Layer::delete_effect(int pos) {
	do_delete_effect(this, pos);
	this->currcliptype = ELEM_LAYER;
}		

void Layer::set_clones() {
	std::unordered_set<Layer*>::iterator it;
	if (mainmix->clonemap.find(this) != mainmix->clonemap.end()) {
		for (it = mainmix->clonemap[this]->begin(); it != mainmix->clonemap[this]->end(); it++) {	
			Layer *lay = *it;
			if (lay != this) {
				lay->speed->value = this->speed->value;
				lay->opacity->value = this->opacity->value;
				lay->playbut->value = this->playbut->value;
				lay->revbut->value = this->revbut->value;
				lay->bouncebut->value = this->bouncebut->value;
				lay->genmidibut->value = this->genmidibut->value;
				lay->frame = this->frame;
				lay->startframe = this->startframe;
				lay->endframe = this->endframe;
			}
		}
	}
}


Effect::Effect() {
	Box *box = new Box;
	this->box = box;
	box->vtxcoords->w = tf(0.2f);
	box->vtxcoords->h = tf(0.05f);
	box->upvtxtoscr();
	this->onoffbutton = new Button(true);
	box = this->onoffbutton->box;
	box->vtxcoords->w = tf(0.025f);
	box->vtxcoords->h = tf(0.05f);
	box->upvtxtoscr();
	box->acolor[3] = 1.0f;
	
	// sets the dry/wet (mix of no-effect with effect) amount of the effect as a parameter
	// read comment at BlurEffect::BlurEffect()
	this->drywet = new Param;
	this->drywet->name = ""; 
	this->drywet->value = 1.0f;
	this->drywet->range[0] = 0.0f;
	this->drywet->range[1] = 1.0f;
	this->drywet->sliding = true;
	this->drywet->shadervar = "drywet";
	this->drywet->box->vtxcoords->w = tf(0.025f);
	this->drywet->box->vtxcoords->h = tf(0.05f);
	this->drywet->box->upvtxtoscr();
}

Effect::~Effect() {
	delete this->box;
	delete this->onoffbutton;
	glDeleteTextures(1, &this->fbotex);
	glDeleteFramebuffers(1, &this->fbo);
}

// The following section defines the different effects and their respective parameters
// this->numrows defines the number of lines the effect+params takes up in the GUI
// param->name is the parameter name as it appears in the GUI
// param->value is the default parameter value
// param->range defines the boundaries of possible parameter values
// param->sliding: true when value is continuous, false when value is integer (not used at the moment, all params are continuous)
// param->shadervar is the uniform variable name used in the shader to represent the parameter
// param->effect: the effect the parameter belongs to
BlurEffect::BlurEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Amount"; 
	param->value = 10.0;
	param->range[0] = 2;
	param->range[1] = 132;
	param->sliding = true;
	param->shadervar = "glowblur";
	param->effect = this;
	this->params.push_back(param);
}

BrightnessEffect::BrightnessEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Amount"; 
	param->value = 0.5f;
	param->range[0] = -1.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "brightamount";
	param->effect = this;
	this->params.push_back(param);
}

ChromarotateEffect::ChromarotateEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Amount"; 
	param->value = -0.5;
	param->range[0] = -0.5;
	param->range[1] = 0.5;
	param->sliding = true;
	param->shadervar = "chromarot";
	param->effect = this;
	this->params.push_back(param);
}

ContrastEffect::ContrastEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Amount"; 
	param->value = 2.0;
	param->range[0] = 0;
	param->range[1] = 4;
	param->sliding = true;
	param->shadervar = "contrastamount";
	param->effect = this;
	this->params.push_back(param);
}

DotEffect::DotEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Size"; 
	param->value = 300.0;
	param->range[0] = 7.5;
	param->range[1] = 2000;
	param->sliding = true;
	param->shadervar = "dotsize";
	param->effect = this;
	this->params.push_back(param);
}

GlowEffect::GlowEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Blur"; 
	param->value = 32.0;
	param->range[0] = 8;
	param->range[1] = 132;
	param->sliding = true;
	param->shadervar = "glowblur";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "Factor"; 
	param->value = 1.2f;
	param->range[0] = 1.0f;
	param->range[1] = 1.5f;
	param->sliding = true;
	param->shadervar = "glowfac";
	param->effect = this;
	this->params.push_back(param);
}

RadialblurEffect::RadialblurEffect() {
	this->numrows = 2;
	Param *param = new Param;
	param->name = "Strength"; 
	param->value = 0.1f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "radstrength";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "InnerRadius"; 
	param->value = 0.0f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "radinner";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->nextrow = true;
	param->name = "X"; 
	param->value = 0.5f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "radialX";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "Y"; 
	param->value = 0.5f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "radialY";
	param->effect = this;
	this->params.push_back(param);
}

SaturationEffect::SaturationEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Amount"; 
	param->value = 4;
	param->range[0] = 0;
	param->range[1] = 8;
	param->sliding = true;
	param->shadervar = "satamount";
	param->effect = this;
	this->params.push_back(param);
}

ScaleEffect::ScaleEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Factor"; 
	param->value = 1.1f;
	param->range[0] = 0.75f;
	param->range[1] = 1.33f;
	param->sliding = true;
	param->shadervar = "scalefactor";
	param->effect = this;
	this->params.push_back(param);
}

SwirlEffect::SwirlEffect() {
	this->numrows = 2;
	Param *param = new Param;
	param->name = "Angle"; 
	param->value = 0.8;
	param->range[0] = 0;
	param->range[1] = 2;
	param->sliding = true;
	param->shadervar = "swirlangle";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "Radius"; 
	param->value = 0.5;
	param->range[0] = 0;
	param->range[1] = 1;
	param->sliding = true;
	param->shadervar = "swradius";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->nextrow = true;
	param->name = "X"; 
	param->value = 0.5;
	param->range[0] = 0;
	param->range[1] = 1;
	param->sliding = true;
	param->shadervar = "swirlx";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "Y"; 
	param->value = 0.5;
	param->range[0] = 0;
	param->range[1] = 1;
	param->sliding = true;
	param->shadervar = "swirly";
	param->effect = this;
	this->params.push_back(param);
}

OldFilmEffect::OldFilmEffect() {
	this->numrows = 2;
	Param *param = new Param;
	param->name = "Noise"; 
	param->value = 2.0;
	param->range[0] = 0;
	param->range[1] = 4;
	param->sliding = true;
	param->shadervar = "filmnoise";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "Sepia"; 
	param->value = 2.0;
	param->range[0] = 0;
	param->range[1] = 4;
	param->sliding = true;
	param->shadervar = "SepiaValue";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "Scratch"; 
	param->value = 2.0;
	param->range[0] = 0;
	param->range[1] = 4;
	param->sliding = true;
	param->shadervar = "ScratchValue";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->nextrow = true;
	param->name = "InVign"; 
	param->range[0] = 0;
	param->range[1] = 1;
	param->sliding = true;
	param->shadervar = "InnerVignetting";
	param->value = 0.7;
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "OutVign"; 
	param->range[0] = 0;
	param->range[1] = 1;
	param->sliding = true;
	param->shadervar = "OuterVignetting";
	param->value = 1.0;
	param->effect = this;
	this->params.push_back(param);
}

RippleEffect::RippleEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Speed"; 
	param->value = 0.1;
	param->range[0] = 0;
	param->range[1] = 0.5;
	param->sliding = true;
	param->shadervar = "ripple";
	param->effect = this;
	this->params.push_back(param);
}

FishEyeEffect::FishEyeEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Amount"; 
	param->value = 0.7;
	param->range[0] = 0.1f;
	param->range[1] = 2;
	param->sliding = true;
	param->shadervar = "fishamount";
	param->effect = this;
	this->params.push_back(param);
}

TresholdEffect::TresholdEffect() {
	this->numrows = 3;
	Param *param = new Param;
	param->name = "Level"; 
	param->value = 0.5;
	param->range[0] = 0;
	param->range[1] = 2;
	param->sliding = true;
	param->shadervar = "treshheight";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->nextrow = true;
	param->name = "R1"; 
	param->value = 1.0;
	param->range[0] = 0;
	param->range[1] = 1;
	param->sliding = true;
	param->shadervar = "treshred1";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "G1"; 
	param->value = 1.0;
	param->range[0] = 0;
	param->range[1] = 1;
	param->sliding = true;
	param->shadervar = "treshgreen1";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "B1"; 
	param->value = 1.0;
	param->range[0] = 0;
	param->range[1] = 1;
	param->sliding = true;
	param->shadervar = "treshblue1";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->nextrow = true;
	param->name = "R2"; 
	param->value = 0.0;
	param->range[0] = 0;
	param->range[1] = 1;
	param->sliding = true;
	param->shadervar = "treshred2";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "G2"; 
	param->value = 0.0;
	param->range[0] = 0;
	param->range[1] = 1;
	param->sliding = true;
	param->shadervar = "treshgreen2";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "B2"; 
	param->value = 0.0;
	param->range[0] = 0;
	param->range[1] = 1;
	param->sliding = true;
	param->shadervar = "treshblue2";
	param->effect = this;
	this->params.push_back(param);
}

StrobeEffect::StrobeEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "R"; 
	param->value = 1.0;
	param->range[0] = 0;
	param->range[1] = 1;
	param->sliding = true;
	param->shadervar = "strobered";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "G"; 
	param->value = 1.0;
	param->range[0] = 0;
	param->range[1] = 1;
	param->sliding = true;
	param->shadervar = "strobegreen";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "B"; 
	param->value = 1.0;
	param->range[0] = 0;
	param->range[1] = 1;
	param->sliding = true;
	param->shadervar = "strobeblue";
	param->effect = this;
	this->params.push_back(param);
}

PosterizeEffect::PosterizeEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Number"; 
	param->value = 8.0;
	param->range[0] = 0;
	param->range[1] = 16;
	param->sliding = true;
	param->shadervar = "numColors";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "Gamma"; 
	param->value = 0.6;
	param->range[0] = 0;
	param->range[1] = 1;
	param->sliding = true;
	param->shadervar = "gamma";
	param->effect = this;
	this->params.push_back(param);
}

PixelateEffect::PixelateEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Width"; 
	param->value = 64.0;
	param->range[0] = 1;
	param->range[1] = 512;
	param->sliding = true;
	param->shadervar = "pixel_w";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "Height"; 
	param->value = 128.0;
	param->range[0] = 1;
	param->range[1] = 1024;
	param->sliding = true;
	param->shadervar = "pixel_h";
	param->effect = this;
	this->params.push_back(param);
}

CrosshatchEffect::CrosshatchEffect() {
	this->numrows = 3;
	Param *param = new Param;
	param->name = "Size"; 
	param->value = 20.0;
	param->range[0] = 0;
	param->range[1] = 100;
	param->sliding = true;
	param->shadervar = "hatchsize";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "Y Offset"; 
	param->value = 5.0;
	param->range[0] = 0;
	param->range[1] = 40;
	param->sliding = true;
	param->shadervar = "hatch_y_offset";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->nextrow = true;
	param->name = "Lum Tresh 1"; 
	param->value = 1.0;
	param->range[0] = 0;
	param->range[1] = 1.0;
	param->sliding = true;
	param->shadervar = "lum_threshold_1";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "Lum Tresh 2"; 
	param->value = 0.7;
	param->range[0] = 0;
	param->range[1] = 1.0;
	param->sliding = true;
	param->shadervar = "lum_threshold_2";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->nextrow = true;
	param->name = "Lum Tresh 3"; 
	param->value = 0.5;
	param->range[0] = 0;
	param->range[1] = 1.0;
	param->sliding = true;
	param->shadervar = "lum_threshold_3";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "Lum Tresh 4"; 
	param->value = 0.3;
	param->range[0] = 0;
	param->range[1] = 1.0;
	param->sliding = true;
	param->shadervar = "lum_threshold_4";
	param->effect = this;
	this->params.push_back(param);
}

InvertEffect::InvertEffect() {
	this->numrows = 1;
}

RotateEffect::RotateEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Angle"; 
	param->value = 45.0;
	param->range[0] = 0;
	param->range[1] = 360;
	param->sliding = true;
	param->shadervar = "rotamount";
	param->effect = this;
	this->params.push_back(param);
}

EmbossEffect::EmbossEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Strength"; 
	param->value = 5.0f;
	param->range[0] = 0;
	param->range[1] = 20.0f;
	param->sliding = true;
	param->shadervar = "embstrength";
	param->effect = this;
	this->params.push_back(param);
}

SolarizeEffect::SolarizeEffect() {
	this->numrows = 1;
}

AsciiEffect::AsciiEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Size"; 
	param->value = 50.0f;
	param->range[0] = 20;
	param->range[1] = 200;
	param->sliding = true;
	param->shadervar = "asciisize";
	param->effect = this;
	this->params.push_back(param);
}

VarDotEffect::VarDotEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Size"; 
	param->value = 0.1f;
	param->range[0] = 0.0f;
	param->range[1] = 0.2f;
	param->sliding = true;
	param->shadervar = "vardotsize";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "Angle"; 
	param->value = 0.0f;
	param->range[0] = 0.0f;
	param->range[1] = 4.0f;
	param->sliding = true;
	param->shadervar = "vardotangle";
	param->effect = this;
	this->params.push_back(param);
}

CRTEffect::CRTEffect() {
	this->numrows = 3;
	Param *param = new Param;
	param->name = "Curvature";
	param->value = 1.0f;
	param->range[0] = 0.0f;
	param->range[1] = 10.0f;
	param->sliding = true;
	param->shadervar = "crtcurvature";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "LineWidth";
	param->value = 1.0f;
	param->range[0] = 0.0f;
	param->range[1] = 50.0f;
	param->sliding = true;
	param->shadervar = "crtlineWidth";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "LineContrast";
	param->value = 0.25f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "crtlineContrast";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->nextrow = true;
	param->name = "Noise";
	param->value = 0.3f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "crtnoise";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "NoiseSize";
	param->value = 1.0f;
	param->range[0] = 0.0f;
	param->range[1] = 20.0f;
	param->sliding = true;
	param->shadervar = "crtnoiseSize";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->nextrow = true;
	param->name = "Vignet";
	param->value = 0.3f;
	param->range[0] = 0.0f;
	param->range[1] = 0.5f;
	param->sliding = true;
	param->shadervar = "crtvignetting";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "VignetAlpha";
	param->value = 1.0f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "crtvignettingAlpha";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "VignetBlur";
	param->value = 0.3f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "crtvignettingBlur";
	param->effect = this;
	this->params.push_back(param);
}

EdgeDetectEffect::EdgeDetectEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Threshold1";
	param->value = 1.1f;
	param->range[0] = 0.0f;
	param->range[1] = 1.1f;
	param->sliding = true;
	param->shadervar = "edge_thres";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "Threshold2";
	param->value = 5.0f;
	param->range[0] = 0.0f;
	param->range[1] = 20.0f;
	param->sliding = true;
	param->shadervar = "edge_thres2";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "Thickness";
	param->value = 0.0f;
	param->range[0] = 0.0f;
	param->range[1] = 40.0f;
	param->sliding = true;
	param->shadervar = "edthickness";  //dummy
	param->effect = this;
	this->params.push_back(param);
}

KaleidoScopeEffect::KaleidoScopeEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Slices"; 
	param->value = 8.0;
	param->range[0] = 0;
	param->range[1] = 32.0f;
	param->sliding = true;
	param->shadervar = "kalslices";
	param->effect = this;
	this->params.push_back(param);
}

HalfToneEffect::HalfToneEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Scale";
	param->value = 30.0f;
	param->range[0] = 1.0f;
	param->range[1] = 200.0f;
	param->sliding = true;
	param->shadervar = "hts";
	param->effect = this;
	this->params.push_back(param);
}

CartoonEffect::CartoonEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Threshold1";
	param->value = 0.2f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "edge_thres";
	param->effect = this;
	this->params.push_back(param);
	param = new Param;
	param->name = "Threshold2";
	param->value = 5.0f;
	param->range[0] = 0.0f;
	param->range[1] = 10.0f;
	param->sliding = true;
	param->shadervar = "edge_thres2";
	param->effect = this;
	this->params.push_back(param);
}

CutoffEffect::CutoffEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Treshold";
	param->value = 0.5f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "cut";
	param->effect = this;
	this->params.push_back(param);
}

GlitchEffect::GlitchEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Size";
	param->value = 1.0f;
	param->range[0] = 0.2f;
	param->range[1] = 4.0f;
	param->sliding = true;
	param->shadervar = "glitchstr";
	param->effect = this;
	this->params.push_back(param);
}

ColorizeEffect::ColorizeEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Hue";
	param->value = 0.0f;
	param->range[0] = 0.0f;
	param->range[1] = 1.0f;
	param->sliding = true;
	param->shadervar = "colhue";
	param->effect = this;
	this->params.push_back(param);
}

NoiseEffect::NoiseEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Level";
	param->value = 0.5f;
	param->range[0] = 0.0f;
	param->range[1] = 8.0f;
	param->sliding = true;
	param->shadervar = "noiselevel";
	param->effect = this;
	this->params.push_back(param);
}

GammaEffect::GammaEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Level";
	param->value = 2.2f;
	param->range[0] = 0.0f;
	param->range[1] = 4.0f;
	param->sliding = true;
	param->shadervar = "gammaval";
	param->effect = this;
	this->params.push_back(param);
}

ThermalEffect::ThermalEffect() {
	this->numrows = 1;
}

BokehEffect::BokehEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Radius";
	param->value = 1.0f;
	param->range[0] = 0.0f;
	param->range[1] = 4.0f;
	param->sliding = true;
	param->shadervar = "bokehrad";
	param->effect = this;
	this->params.push_back(param);
}

SharpenEffect::SharpenEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Strength";
	param->value = 10.0f;
	param->range[0] = 0.0f;
	param->range[1] = 40.0f;
	param->sliding = true;
	param->shadervar = "curve_height";
	param->effect = this;
	this->params.push_back(param);
}

DitherEffect::DitherEffect() {
	this->numrows = 1;
	Param *param = new Param;
	param->name = "Size";
	param->value = 0.5f;
	param->range[0] = 0.4f;
	param->range[1] = 0.55f;
	param->sliding = true;
	param->shadervar = "dither_size";
	param->effect = this;
	this->params.push_back(param);
}



float RippleEffect::get_speed() { return this->speed; }
float RippleEffect::get_ripplecount() { return this->ripplecount; }
void RippleEffect::set_ripplecount(float count) { this->ripplecount = count; }

int StrobeEffect::get_phase() { return this->phase; }
void StrobeEffect::set_phase(int phase) { this->phase = phase; }


Box::Box() {
	this->vtxcoords = new BOX_COORDS;
	this->scrcoords = new BOX_COORDS;
}

Box::~Box() {
	delete this->vtxcoords;
	delete this->scrcoords;
}

Button::Button(bool state) {
	this->box = new Box;
	this->value = state;
	this->ccol[3] = 1.0f;
}

Button::~Button() {
	delete this->box;
}

void Button::draw(bool circlein) {
	if (this->box->in()) {
		this->box->acolor[0] = 0.5;
		this->box->acolor[1] = 0.5;
		this->box->acolor[2] = 1.0;
		this->box->acolor[3] = 1.0;
	}
	else {
		this->box->acolor[0] = 0.0;
		this->box->acolor[1] = 0.0;
		this->box->acolor[2] = 0.0;
		this->box->acolor[3] = 1.0;
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
	if (this->scrcoords->x1 < mx and mx < this->scrcoords->x1 + this->scrcoords->w) {
		if (this->scrcoords->y1 - this->scrcoords->h < my and my < this->scrcoords->y1) {
			return true;
		}
	}
	return false;
}


Clip::Clip() {
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


Param::Param() {
	this->box = new Box;
	this->box->acolor[0] = 0.2;
	this->box->acolor[1] = 0.2;
	this->box->acolor[2] = 0.2;
	this->box->acolor[3] = 1.0;
	if (loopstation) loopstation->allparams.push_back(this);
}

Param::~Param() {
	delete this->box;
	loopstation->allparams.erase(std::find(loopstation->allparams.begin(), loopstation->allparams.end(), this));
}

void Param::handle() {
	float green[4] = {0.0, 1.0, 0.2, 1.0};
	float white[4] = {1.0, 1.0, 1.0, 1.0};
	std::string parstr;
	draw_box(this->box, -1);
	if (mainmix->learnparam == this and mainmix->learn) {
		parstr = "MIDI";
	}
	else parstr = this->name;
	render_text(parstr, white, this->box->vtxcoords->x1 + tf(0.01f), this->box->vtxcoords->y1 + tf(0.05f) - tf(0.030f), tf(0.0003f), tf(0.0005f));
	if (this->box->in()) {
		if (mainprogram->leftmousedown) {
			mainprogram->leftmousedown = false;
			mainmix->adaptparam = this;
			mainmix->prevx = mainprogram->mx;
		}
		if (mainprogram->menuactivation) {
			if (loopstation->elemmap.find(this) != loopstation->elemmap.end()) mainprogram->parammenu2->state = 2;
			else mainprogram->parammenu1->state = 2;
			mainmix->learnparam = this;
			mainprogram->menuactivation = false;
		}
	}
	draw_box(green, green, this->box->vtxcoords->x1 + this->box->vtxcoords->w * ((this->value - this->range[0]) / (this->range[1] - this->range[0])) - tf(0.00078f), this->box->vtxcoords->y1, 2.0f / glob->w, this->box->vtxcoords->h, -1);
}


Preferences::Preferences() {
	PIVid *pvi = new PIVid;
	this->items.push_back(pvi);
	PIInt *pii = new PIInt;
	this->items.push_back(pii);
	PIDirs *pidirs = new PIDirs;
	this->items.push_back(pidirs);
	PIMidi *pimidi = new PIMidi;
	this->items.push_back(pimidi);
	for (int i = 0; i < this->items.size(); i++) {
		PrefItem *item = this->items[i];
		item->pos = i;
		item->box = new Box;
		item->box->vtxcoords->x1 = -1.0f;
		item->box->vtxcoords->y1 = 1.0f - (i + 1) * 0.2f;
		item->box->vtxcoords->w = 0.5f;
		item->box->vtxcoords->h = 0.2f;
	}
}

void Preferences::load() {
	ifstream rfile;
	#ifdef _WIN64
	std::string prstr = "./preferences.prefs";
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
	
		else if (istring == "VIDEO") {
			while (getline(rfile, istring)) {
				if (istring == "ENDOFVIDEO") break;
				for (int i = 0; i < mainprogram->prefs->items.size(); i++) {
					if (mainprogram->prefs->items[i]->name == "Video") {
						PIVid *piv = (PIVid*)(mainprogram->prefs->items[i]);
						for (int j = 0; j < piv->items.size(); j++) {
							if (piv->items[j]->name == istring) {
								getline(rfile, istring);
								piv->items[j]->type = (PIVID_TYPE)std::stoi(istring);
								getline(rfile, istring);
								piv->items[j]->value = (PIVID_TYPE)std::stoi(istring);
								if (piv->items[j]->type == PIVID_W) {
									mainprogram->ow = piv->items[j]->value;
								}
								if (piv->items[j]->type == PIVID_H) {
									mainprogram->oh = piv->items[j]->value;
								}
							}
						}
					}
				}
			}
		}
		else if (istring == "INTERFACE") {
			while (getline(rfile, istring)) {
				if (istring == "ENDOFINTERFACE") break;
				for (int i = 0; i < mainprogram->prefs->items.size(); i++) {
					if (mainprogram->prefs->items[i]->name == "Interface") {
						PIInt *pii = (PIInt*)(mainprogram->prefs->items[i]);
						for (int j = 0; j < pii->items.size(); j++) {
							if (pii->items[j]->name == istring) {
								getline(rfile, istring);
								pii->items[j]->type = (PIINT_TYPE)std::stoi(istring);
								getline(rfile, istring);
								pii->items[j]->onoff = std::stoi(istring);
								if (pii->items[j]->type == PIINT_CLICK) {
									mainprogram->needsclick = pii->items[j]->onoff;
								}
							}
						}
					}
				}
			}
		}
		else if (istring == "MIDI") {
			mainprogram->pmon.clear();
			while (getline(rfile, istring)) {
				if (istring == "ENDOFMIDI") break;
				PMidiItem *newpm = new PMidiItem;
				newpm->name = istring;
				getline(rfile, istring);
				newpm->onoff = std::stoi(istring);
				if (newpm->onoff) mainprogram->pmon.push_back(newpm);
				for (int i = 0; i < mainprogram->prefs->items.size(); i++) {
					if (mainprogram->prefs->items[i]->name == "MIDI Devices") {
						PIMidi *pmi = (PIMidi*)(mainprogram->prefs->items[i]);
						pmi->populate();
						for (int j = 0; j < pmi->items.size(); j++) {
							if (pmi->items[j]->name == newpm->name) {
								if (!newpm->onoff) {
									if (std::find(pmi->onnames.begin(), pmi->onnames.end(), pmi->items[j]->name) != pmi->onnames.end()) {
										pmi->onnames.erase(std::find(pmi->onnames.begin(), pmi->onnames.end(), pmi->items[j]->name));
										mainprogram->openports.erase(std::find(mainprogram->openports.begin(), mainprogram->openports.end(), j));
										pmi->items[j]->midiin->cancelCallback();
										delete pmi->items[j]->midiin;
									}
								}
								else {
									pmi->onnames.push_back(pmi->items[j]->name);
									RtMidiIn *midiin = new RtMidiIn();
									if (std::find(mainprogram->openports.begin(), mainprogram->openports.end(), j) == mainprogram->openports.end()) {
										midiin->setCallback(&mycallback, (void*)pmi->items[j]);
										midiin->openPort(j);
									}
									mainprogram->openports.push_back(j);
									pmi->items[j]->midiin = midiin;
								}
							}
						}
					}
				}
			}
		}
	
		else if (istring == "BINSDIR") {
			getline(rfile, istring);
			mainprogram->binsdir = istring;
		}
		else if (istring == "RECDIR") {
			getline(rfile, istring);
			mainprogram->recdir = istring;
		}
	}
	rfile.close();
}

void Preferences::save() {
	ofstream wfile;
	#ifdef _WIN64
	std::string prstr = "./preferences.prefs";
	#else
	#ifdef __linux__
	std::string homedir (getenv("HOME"));
	std::string prstr = homedir + "/.ewocvj2/preferences.prefs";
	#endif
	#endif
	wfile.open(prstr);
	wfile << "EWOCvj PREFERENCES V0.1\n";
	
	wfile << "VIDEO\n";
	for (int i = 0; i < mainprogram->prefs->items.size(); i++) {
		if (mainprogram->prefs->items[i]->name == "Video") {
			PIVid *piv = (PIVid*)(mainprogram->prefs->items[i]);
			for (int j = 0; j < piv->items.size(); j++) {
				wfile << piv->items[j]->name;
				wfile << "\n";
				wfile << std::to_string(piv->items[j]->type);
				wfile << "\n";
				wfile << std::to_string(piv->items[j]->value);
				wfile << "\n";
			}
		}
	}
	wfile << "ENDOFVIDEO\n";
	
	wfile << "INTERFACE\n";
	for (int i = 0; i < mainprogram->prefs->items.size(); i++) {
		if (mainprogram->prefs->items[i]->name == "Interface") {
			PIInt *pii = (PIInt*)(mainprogram->prefs->items[i]);
			for (int j = 0; j < pii->items.size(); j++) {
				wfile << pii->items[j]->name;
				wfile << "\n";
				wfile << std::to_string(pii->items[j]->type);
				wfile << "\n";
				wfile << std::to_string(pii->items[j]->onoff);
				wfile << "\n";
			}
		}
	}
	wfile << "ENDOFINTERFACE\n";
	
	wfile << "MIDI\n";
	for (int i = 0; i < mainprogram->prefs->items.size(); i++) {
		if (mainprogram->prefs->items[i]->name == "MIDI Devices") {
			PIMidi *pmi = (PIMidi*)(mainprogram->prefs->items[i]);
			for (int j = 0; j < pmi->items.size(); j++) {
				wfile << pmi->items[j]->name;
				wfile << "\n";
				wfile << std::to_string(pmi->items[j]->onoff);
				wfile << "\n";
			}
			for (int j = 0; j < mainprogram->pmon.size(); j++) {
				bool found = false;
				for (int k = 0; k < pmi->items.size(); k++) {
					if (mainprogram->pmon[j]->name == pmi->items[k]->name) {
						found = true;
						break;
					}
				}
				if (!found) {
					wfile << mainprogram->pmon[j]->name;
					wfile << "\n";
					wfile << std::to_string(mainprogram->pmon[j]->onoff);
					wfile << "\n";
				}
			}
		}
	}
	wfile << "ENDOFMIDI\n";
	
	wfile << "BINSDIR\n";
	wfile << mainprogram->binsdir;
	wfile << "\n";
	
	wfile << "RECDIR\n";
	wfile << mainprogram->recdir;
	wfile << "\n";
	
	wfile << "ENDOFFILE\n";
	wfile.close();
}

PIDirs::PIDirs() {
	this->name = "Directories";
	PDirItem *pdi;
	#ifdef _WIN64
	pdi = this->additem("BINS", "./bins/");
	#else
	#ifdef __linux__
	std::string homedir (getenv("HOME"));
	pdi = this->additem("BINS", homedir + "/.ewocvj2/bins/");
	#endif
	#endif
	mainprogram->binsdir = pdi->path;
	#ifdef _WIN64
	pdi = this->additem("Recordings", "./recordings/");
	#else
	#ifdef __linux__
	pdi = this->additem("Recordings", homedir + "/.ewocvj2/recordings/");
	#endif
	#endif
	mainprogram->recdir = pdi->path;
}

PDirItem* PIDirs::additem(const std::string &name, const std::string &path) {
	PDirItem *pdi = new PDirItem;
	pdi->name = name;
	pdi->path = path;
	this->items.push_back(pdi);
	return pdi;
}

PIMidi::PIMidi() {
	this->name = "MIDI Devices";
}

void PIMidi::populate() {
	RtMidiIn midiin;
	int nPorts = midiin.getPortCount();
	std::string portName;
	this->items.clear();
	for (int i = 0; i < nPorts; i++) {
		PMidiItem *pmi = new PMidiItem;
		pmi->name = midiin.getPortName(i);
		pmi->onoff = (std::find(this->onnames.begin(), this->onnames.end(), pmi->name) != this->onnames.end());
		this->items.push_back(pmi);	
	}
}

PIInt::PIInt() {
	this->name = "Interface";
	PIntItem *pii = new PIntItem;
	pii->name = "Select needs click";
	pii->type = PIINT_CLICK;
	pii->onoff = 0;
	this->items.push_back(pii);
	//pii = new PIntItem;
	//pii->name = "Select needs click";
	//pii->onoff = 0;
	//this->items.push_back(pii);
}

PIVid::PIVid() {
	this->name = "Video";
	PVidItem *pvi = new PVidItem;
	pvi->name = "Output video width";
	pvi->type = PIVID_W;
	pvi->value = 1920;
	this->items.push_back(pvi);
	pvi = new PVidItem;
	pvi->name = "Output video height";
	pvi->type = PIVID_H;
	pvi->value = 1080;
	this->items.push_back(pvi);
}


Bin::Bin(int pos) {
	this->box = new Box;
	this->box->vtxcoords->x1 = -0.15f;
	this->box->vtxcoords->y1 = (pos + 1) * -0.05f;
	this->box->vtxcoords->w = 0.3f;
	this->box->vtxcoords->h = 0.05f;
	this->box->upvtxtoscr();
}

Bin::~Bin() {
	delete this->box;
}

BinElement::BinElement() {
	glGenTextures(1, &this->tex);
	glBindTexture(GL_TEXTURE_2D, this->tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

BinElement::~BinElement() {
	glDeleteTextures(1, &this->tex);
	glDeleteTextures(1, &this->oldtex);
}

BinDeck::BinDeck() {
	this->box = new Box;
	this->box->vtxcoords->w = 0.36f;
	this->box->upvtxtoscr();
	this->box->lcolor[0] = 0.5f;
	this->box->lcolor[1] = 0.5f;
	this->box->lcolor[2] = 1.0f;
	this->box->lcolor[3] = 1.0f;
}

BinDeck::~BinDeck() {
	delete this->box;
}

BinMix::BinMix() {
	this->box = new Box;
	this->box->vtxcoords->w = 0.72f;
	this->box->upvtxtoscr();
	this->box->lcolor[0] = 0.5f;
	this->box->lcolor[1] = 1.0f;
	this->box->lcolor[2] = 0.5f;
	this->box->lcolor[3] = 1.0f;
}

BinMix::~BinMix() {
	delete this->box;
}


Program::Program() {
	#ifdef _WIN64
	this->temppath = "./temp/";
	#else
	#ifdef __linux__
	std::string homedir (getenv("HOME"));
	this->temppath = homedir + "/.ewocvj2/temp/";
	#endif
	#endif
	
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
	this->toscreen->name = "MAKE LIVE";
	this->toscreen->box->vtxcoords->x1 = -0.3 - (0.3 / 2.0);
	this->toscreen->box->vtxcoords->y1 = -0.7;
	this->toscreen->box->vtxcoords->w = 0.3 / 2.0;
	this->toscreen->box->vtxcoords->h = 0.3 / 3.0;
	this->toscreen->box->upvtxtoscr();
	
	this->backtopre = new Button(false);
	this->backtopre->name = "BACK";
	this->backtopre->box->vtxcoords->x1 = -0.3;
	this->backtopre->box->vtxcoords->y1 = -0.4;
	this->backtopre->box->vtxcoords->w = 0.15f;
	this->backtopre->box->vtxcoords->h = 0.1f;
	this->backtopre->box->upvtxtoscr();
	
	this->vidprev = new Button(false);
	this->vidprev->name = "VIDEO PREVIEW";
	this->vidprev->box->vtxcoords->x1 = -0.45f;
	this->vidprev->box->vtxcoords->y1 = -0.6f;
	this->vidprev->box->vtxcoords->w = 0.15f;
	this->vidprev->box->vtxcoords->h = 0.1f;
	this->vidprev->box->upvtxtoscr();
	
	this->effprev = new Button(false);
	this->effprev->name = "EFFECT PREVIEW";
	this->effprev->box->vtxcoords->x1 = -0.45f;
	this->effprev->box->vtxcoords->y1 = -0.5f;
	this->effprev->box->vtxcoords->w = 0.15f;
	this->effprev->box->vtxcoords->h = 0.1f;
	this->effprev->box->upvtxtoscr();
	
	this->effscrollupA = new Box;
	this->effscrollupA->vtxcoords->x1 = -1.0;
	this->effscrollupA->vtxcoords->y1 = 1.0 - tf(mainmix->layw) - tf(0.20f);
	this->effscrollupA->vtxcoords->w = tf(0.025f);
	this->effscrollupA->vtxcoords->h = tf(0.05f);
	this->effscrollupA->upvtxtoscr();
	
	this->effscrollupB = new Box;
	this->effscrollupB->vtxcoords->x1 = 1.0 - tf(0.05f);
	this->effscrollupB->vtxcoords->y1 = 1.0 - tf(mainmix->layw) - tf(0.20f);
	this->effscrollupB->vtxcoords->w = tf(0.025f);
	this->effscrollupB->vtxcoords->h = tf(0.05f);     
	this->effscrollupB->upvtxtoscr();
	
	this->effscrolldownA = new Box;
	this->effscrolldownA->vtxcoords->x1 = -1.0;
	this->effscrolldownA->vtxcoords->y1 = 1.0 - tf(mainmix->layw) - tf(0.20f) - tf(0.05f) * 10;
	this->effscrolldownA->vtxcoords->w = tf(0.025f);
	this->effscrolldownA->vtxcoords->h = tf(0.05f);
	this->effscrolldownA->upvtxtoscr();
	
	this->effscrolldownB = new Box;
	this->effscrolldownB->vtxcoords->x1 = 1.0 - tf(0.05f);
	this->effscrolldownB->vtxcoords->y1 = 1.0 - tf(mainmix->layw) - tf(0.20f) - tf(0.05f) * 10;
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
}

void handle_numboxes(std::vector<Box*> &numboxes) {
	// Draw numboxes - put in mainmix function
	float white[] = {1.0, 1.0, 1.0, 1.0};
	float red[] = {1.0, 0.5, 0.5, 1.0};
	for (int i = numboxes.size() - 1; i >= 0; i--) {
		Box *box = numboxes[i];
		std::string s = std::to_string(i);
		std::string pchar;
		if ((i == mainmix->page[0] + 1 and numboxes == mainmix->numboxesA) or (i == mainmix->page[1] + 1 and numboxes == mainmix->numboxesB)) {
			box->acolor[0] = 1.0f;
			box->acolor[1] = 0.5f;
			box->acolor[2] = 0.5f;
		}
		else {
			box->acolor[0] = 0.0f;
			box->acolor[1] = 0.0f;
			box->acolor[2] = 0.0f;
		}
		draw_box(box, -1);
		if (i == 0 and numboxes == mainmix->numboxesA) pchar = "A";
		else if (i == 0 and numboxes == mainmix->numboxesB) pchar = "B";
		else pchar = s;
		if (i == 0) {
			render_text(pchar, red, box->vtxcoords->x1 + 0.01f, box->vtxcoords->y1 + 0.025f, 0.0006f, 0.001f);
		}
		else {
			render_text(pchar, white, box->vtxcoords->x1 + 0.01f, box->vtxcoords->y1 + 0.025f, 0.0006f, 0.001f);
		}	
	}
	// Handle numboxes
	Box *box1 = numboxes[0];
	Box *box2 = numboxes[numboxes.size() - 1];
	if (mainprogram->menuactivation) {
		if (box1->scrcoords->x1 < mainprogram->mx and mainprogram->mx < box2->scrcoords->x1 + box2->scrcoords->w) {
			if (box1->scrcoords->y1 - box1->scrcoords->h < mainprogram->my and mainprogram->my < box2->scrcoords->y1) {
				mainprogram->deckmenu->state = 2;
				mainmix->mousedeck = (numboxes == mainmix->numboxesB);
				mainprogram->menuactivation = false;
			}
		}
	}
	for (int i = 0; i < numboxes.size(); i++) {
		Box *box = numboxes[i];
		box->acolor[0] = 0.0;
		box->acolor[1] = 0.0;
		box->acolor[2] = 0.0;
		box->acolor[3] = 1.0;
		if (box->scrcoords->x1 < mainprogram->mx and mainprogram->mx < box->scrcoords->x1 + box->scrcoords->w) {
			if (box->scrcoords->y1 - box->scrcoords->h < mainprogram->my and mainprogram->my < box->scrcoords->y1) {
				if (mainprogram->leftmouse and !mainmix->moving and !mainprogram->menuondisplay) {
					if (i != 0) {
						std::string comp = "";
						if (!mainprogram->preveff) comp = "comp";
						if (numboxes == mainmix->numboxesA) {
							// switch scenes
							if (i == mainmix->page[0] + 1) continue;
							mainmix->tempnbframes[i - 1].clear();
							for (int j = 0; j < mainmix->nbframesA[i - 1].size(); j++) {
								mainmix->tempnbframes[i - 1].push_back(mainmix->nbframesA[i - 1][j]);
							}
							mainmix->nbframesA[i - 1].clear();
							std::vector<Layer*> &lvec = choose_layers(0);
							mainmix->nbframesA[mainmix->page[0]].clear();
							for (int j = 0; j < lvec.size(); j++) {
								// store layers of current scene into nbframesA for running their framecounters in the background (to keep sync)
								mainmix->nbframesA[mainmix->page[0]].push_back(lvec[j]);
							}
							mainmix->mousedeck = 0;
							// save current scene to temp dir, open new scene
							mainmix->save_deck(mainprogram->temppath + "tempdeck_xch.deck");
							// stop current scene loopstation line if they don't extend to the other deck
							for (int j = 0; j < loopstation->elems.size(); j++) {
								std::unordered_set<Param*>::iterator it;
								for (it = loopstation->elems[j]->params.begin(); it != loopstation->elems[j]->params.end(); it++) {
									Param *par = *it;
									if (par->effect) {
										if (std::find(lvec.begin(), lvec.end(), par->effect->layer) != lvec.end()) {
											loopstation->elems[j]->params.erase(par);
										}
									}
								}
								if (loopstation->elems[j]->params.size() == 0) {
									loopstation->elems[j]->erase_elem();
								}
							}
							mainmix->open_deck(mainprogram->temppath + "tempdeck_A" + std::to_string(i) + comp + ".deck", 0);
							boost::filesystem::rename(mainprogram->temppath + "tempdeck_xch.deck", mainprogram->temppath + "tempdeck_A" + std::to_string(mainmix->page[0] + 1) + comp + ".deck");
							std::vector<Layer*> &lvec2 = choose_layers(0);
							for (int j = 0; j < lvec2.size(); j++) {
								// set layer frame to (running in background) nbframesA->frame of loaded scene
								lvec2[j]->frame = mainmix->tempnbframes[i - 1][j]->frame;
							}
							mainmix->page[0] = i - 1;
							
						}
						else {
							if (i == mainmix->page[1] + 1) continue;
							mainmix->tempnbframes[i - 1].clear();
							for (int j = 0; j < mainmix->nbframesB[i - 1].size(); j++) {
								mainmix->tempnbframes[i - 1].push_back(mainmix->nbframesB[i - 1][j]);
							}
							mainmix->nbframesB[i - 1].clear();
							std::vector<Layer*> &lvec = choose_layers(1);
							mainmix->nbframesB[mainmix->page[1]].clear();
							for (int j = 0; j < lvec.size(); j++) {
								mainmix->nbframesB[mainmix->page[1]].push_back(lvec[j]);
							}
							mainmix->mousedeck = 1;
							mainmix->save_deck(mainprogram->temppath + "tempdeck_xch.deck");
							// stop current scene loopstation line if they don't extend to the other deck
							for (int j = 0; j < loopstation->elems.size(); j++) {
								std::unordered_set<Param*>::iterator it;
								for (it = loopstation->elems[j]->params.begin(); it != loopstation->elems[j]->params.end(); it++) {
									Param *par = *it;
									if (par->effect) {
										if (std::find(lvec.begin(), lvec.end(), par->effect->layer) != lvec.end()) {
											loopstation->elems[j]->params.erase(par);
										}
									}
								}
								if (loopstation->elems[j]->params.size() == 0) {
									loopstation->elems[j]->erase_elem();
								}
							}
							mainmix->open_deck(mainprogram->temppath + "tempdeck_B" + std::to_string(i) + comp + ".deck", 0);
							boost::filesystem::rename(mainprogram->temppath + "tempdeck_xch.deck", mainprogram->temppath + "tempdeck_B" + std::to_string(mainmix->page[1] + 1) + comp + ".deck");
							std::vector<Layer*> &lvec2 = choose_layers(0);
							for (int j = 0; j < lvec2.size(); j++) {
								lvec2[j]->frame = mainmix->tempnbframes[i - 1][j]->frame;
							}
							mainmix->page[1] = i - 1;
						}
					}
				}
				box->acolor[0] = 0.5;
				box->acolor[1] = 0.5;
				box->acolor[2] = 1.0;
				box->acolor[3] = 1.0;
			}
		}
	}
}

// regulates the reordering of layers around the stacks by dragging
// both exchanging two layers and moving one layer
void exchange(Layer *lay, std::vector<Layer*> &slayers, std::vector<Layer*> &dlayers, bool deck) {
	int size = dlayers.size();
	for (int i = 0; i < size; i++) {
		Layer *inlay = dlayers[i];
		if (inlay->pos < mainmix->scrollpos[deck] or inlay->pos > mainmix->scrollpos[deck] + 2) continue;
		Box *box = inlay->node->vidbox;
		int endx = 0;
		if ((i == dlayers.size() - 1 - mainmix->scrollpos[deck]) and (box->scrcoords->x1 + box->scrcoords->w - mainprogram->xvtxtoscr(tf(0.075f)) < mainprogram->mx and mainprogram->mx < box->scrcoords->x1 + box->scrcoords->w + mainprogram->xvtxtoscr(0.075f) * (i != dlayers.size() - 1 - mainmix->scrollpos[deck]))) {
			endx = 1;
		}
		bool dropin = false;
		int numonscreen = size - mainmix->scrollpos[deck];
		if (0 <= numonscreen and numonscreen <= 2) {
			if (mainprogram->xvtxtoscr(mainmix->numw + deck * 1.0f + numonscreen * mainmix->layw) < mainprogram->mx and mainprogram->mx < deck * (glob->w / 2.0f) + glob->w / 2.0f) {
				if (0 < mainprogram->my and mainprogram->my < mainprogram->yvtxtoscr(mainmix->layw)) {
					if (size == i + 1) {
						dropin = true;
						endx = 1;
					}
				}
			}
		}
		if (dropin or (box->scrcoords->y1 < mainprogram->my + box->scrcoords->h and mainprogram->my < box->scrcoords->y1)) {
			if ((box->scrcoords->x1 + mainprogram->xvtxtoscr(0.075f) < mainprogram->mx and mainprogram->mx < box->scrcoords->x1 + box->scrcoords->w - mainprogram->xvtxtoscr(0.075f))) {
				if (lay == inlay) return;
				//exchange two layers
				bool indeck = inlay->deck;
				BlendNode *bnode = inlay->blendnode;
				BLEND_TYPE btype = bnode->blendtype;
				VideoNode *node = inlay->node;
				
				slayers[lay->pos] = inlay;
				slayers[lay->pos]->pos = lay->pos;
				slayers[lay->pos]->deck = lay->deck;
				slayers[lay->pos]->node = lay->node;
				slayers[lay->pos]->node->layer = inlay;
				slayers[lay->pos]->blendnode = lay->blendnode;
				slayers[lay->pos]->blendnode->blendtype = lay->blendnode->blendtype;
				slayers[lay->pos]->blendnode->mixfac->value = lay->blendnode->mixfac->value;
				slayers[lay->pos]->blendnode->wipetype = lay->blendnode->wipetype;
				slayers[lay->pos]->blendnode->wipedir = lay->blendnode->wipedir;
				slayers[lay->pos]->blendnode->wipex = lay->blendnode->wipex;
				slayers[lay->pos]->blendnode->wipey = lay->blendnode->wipey;
				
				dlayers[i] = lay;
				dlayers[i]->pos = i;
				dlayers[i]->deck = indeck;
				dlayers[i]->node = node;
				dlayers[i]->node->layer = lay;
				dlayers[i]->blendnode = bnode;
				dlayers[i]->blendnode->blendtype = btype;
				dlayers[i]->blendnode->mixfac->value = bnode->mixfac->value;
				dlayers[i]->blendnode->wipetype = bnode->wipetype;
				dlayers[i]->blendnode->wipedir = bnode->wipedir;
				dlayers[i]->blendnode->wipex = bnode->wipex;
				dlayers[i]->blendnode->wipey = bnode->wipey;
				
				Node *inlayout = inlay->lasteffnode->out[0];
				Node *layout = lay->lasteffnode->out[0];
				if (inlay->effects.size()) {
					inlay->node->out.clear();
					mainprogram->nodesmain->currpage->connect_nodes(inlay->node, inlay->effects[0]->node);
					inlay->lasteffnode = inlay->effects[inlay->effects.size() - 1]->node;
				}
				else inlay->lasteffnode = inlay->node;
				inlay->lasteffnode->out.clear();
				if (inlay->pos == 0) {
					layout->in = nullptr;
					mainprogram->nodesmain->currpage->connect_nodes(inlay->lasteffnode, layout);
				}
				else {
					((BlendNode*)(layout))->in2 = nullptr;
					mainprogram->nodesmain->currpage->connect_in2(inlay->lasteffnode, (BlendNode*)(layout));
				}	
				for (int j = 0; j < inlay->effects.size(); j++) {
					inlay->effects[j]->node->align = inlay->node;
				}
				if (lay->effects.size()) {
					lay->node->out.clear();
					mainprogram->nodesmain->currpage->connect_nodes(lay->node, lay->effects[0]->node);
					lay->lasteffnode = lay->effects[lay->effects.size() - 1]->node;
				}
				else lay->lasteffnode = lay->node;
				lay->lasteffnode->out.clear();
				if (lay->pos == 0) {
					inlayout->in = nullptr;
					mainprogram->nodesmain->currpage->connect_nodes(lay->lasteffnode, inlayout);
				}
				else {
					((BlendNode*)(inlayout))->in2 = nullptr;
					mainprogram->nodesmain->currpage->connect_in2(lay->lasteffnode, (BlendNode*)inlayout);
				}			
				for (int j = 0; j < lay->effects.size(); j++) {
					lay->effects[j]->node->align = lay->node;
				}
				printf("%d %d\n", inlay->lasteffnode, layout->in);
				printf("%d %d\n", lay->lasteffnode, ((BlendNode*)(layout))->in2);
				printf("%d %d\n", layout->out[0], mainprogram->nodesmain->mixnodes[0]);
				break;
			}
			else if (dropin or endx or (box->scrcoords->x1 - mainprogram->xvtxtoscr(0.075f) * (i - mainmix->scrollpos[deck] != 0) < mainprogram->mx and mainprogram->mx < box->scrcoords->x1 + mainprogram->xvtxtoscr(0.075f))) {
				if (lay == dlayers[i]) return;
				//move one layer
				BLEND_TYPE nextbtype;
				float nextmfval;
				int nextwipetype, nextwipedir;
				float nextwipex, nextwipey;
				Layer *nextlay = NULL;
				if (slayers.size() > lay->pos + 1) {
					nextlay = slayers[lay->pos + 1];
					nextbtype = nextlay->blendnode->blendtype;
				}
				lay->deck = deck;
				BLEND_TYPE btype = lay->blendnode->blendtype;
				BlendNode *firstbnode = (BlendNode*)dlayers[0]->lasteffnode->out[0];
				Node *firstlasteffnode = dlayers[0]->lasteffnode;
				float mfval = lay->blendnode->mixfac->value;
				int wipetype = lay->blendnode->wipetype;
				int wipedir = lay->blendnode->wipedir;
				float wipex = lay->blendnode->wipex;
				float wipey = lay->blendnode->wipey;
				if (lay->pos > 0 and i + endx != 0) {
					mainprogram->nodesmain->currpage->connect_nodes(lay->blendnode->in, lay->blendnode->out[0]);
					mainprogram->nodesmain->currpage->delete_node(lay->blendnode);
				}
				else if (i + endx != 0) {
					if (nextlay) {
						nextlay->lasteffnode->out.clear();
						mainprogram->nodesmain->currpage->connect_nodes(nextlay->lasteffnode, nextlay->blendnode->out[0]);
						mainprogram->nodesmain->currpage->delete_node(nextlay->blendnode);
						nextlay->blendnode = new BlendNode;
						nextlay->blendnode->blendtype = nextbtype;
						nextlay->blendnode->mixfac->value = nextmfval;
						nextlay->blendnode->wipetype = nextwipetype;
						nextlay->blendnode->wipedir = nextwipedir;
						nextlay->blendnode->wipex = nextwipex;
						nextlay->blendnode->wipey = nextwipey;
					}
				}
				lay->blendnode = nullptr;
				bool lastisvid = false;
				if (lay->lasteffnode == lay->node) lastisvid = true;
				mainprogram->nodesmain->currpage->delete_node(lay->node);
				
				int slayerssize = slayers.size();
				for (int j = 0; j < slayerssize; j++) {
					if (slayers[j] == lay) {
						slayers.erase(slayers.begin() + j);
						if (slayers == dlayers and i > j) endx--;
						break;
					}
				}
				bool comp;
				if (dlayers == mainmix->layersA or dlayers == mainmix->layersB) comp = false;
				else comp = true;
				for (int j = 0; j < slayers.size(); j++) {
					slayers[j]->pos = j;
				}
				
				if (endx) {
					dlayers.insert(dlayers.end(), lay);
					if (slayers == dlayers) endx = 0;
				}
				else dlayers.insert(dlayers.begin() + i, lay);
				for (int j = 0; j < dlayers.size(); j++) {
					dlayers[j]->pos = j;
				}
				if (dlayers[i + endx]->pos == mainmix->scrollpos[deck] + 3) mainmix->scrollpos[deck]++;
				dlayers[i + endx]->node = mainprogram->nodesmain->currpage->add_videonode(comp);
				dlayers[i + endx]->node->layer = lay;
				for (int j = 0; j < dlayers[i + endx]->effects.size(); j++) {
					if (j == 0) mainprogram->nodesmain->currpage->connect_nodes(dlayers[i + endx]->node, dlayers[i + endx]->effects[j]->node);
					dlayers[i + endx]->effects[j]->node->align = dlayers[i + endx]->node;
				}
				if (lastisvid) lay->lasteffnode = lay->node;
				if (dlayers[i + endx]->pos > 0) {
					Layer *prevlay = dlayers[dlayers[i + endx]->pos - 1];
					BlendNode *bnode = mainprogram->nodesmain->currpage->add_blendnode(MIXING, comp);
					bnode->blendtype = btype;
					lay->blendnode = bnode;
					lay->blendnode->mixfac->value = mfval;
					lay->blendnode->wipetype = wipetype;
					lay->blendnode->wipedir = wipedir;
					lay->blendnode->wipex = wipex;
					lay->blendnode->wipey = wipey;
					Node *node;
					if (prevlay->pos > 0) {
						node = prevlay->blendnode;
					}
					else {
						node = prevlay->lasteffnode;
					}
					mainprogram->nodesmain->currpage->connect_nodes(node, lay->lasteffnode, lay->blendnode);
					if (dlayers[i + endx]->pos == dlayers.size() - 1 and mainprogram->nodesmain->mixnodes.size()) {
						if (&dlayers == &mainmix->layersA) {
							mainprogram->nodesmain->currpage->connect_nodes(lay->blendnode, mainprogram->nodesmain->mixnodes[0]);
						}
						else if (&dlayers == &mainmix->layersB) {
							mainprogram->nodesmain->currpage->connect_nodes(lay->blendnode, mainprogram->nodesmain->mixnodes[1]);
						}
						else if (&dlayers == &mainmix->layersAcomp) {
							mainprogram->nodesmain->currpage->connect_nodes(lay->blendnode, mainprogram->nodesmain->mixnodescomp[0]);
						}
						else if (&dlayers == &mainmix->layersBcomp) {
							mainprogram->nodesmain->currpage->connect_nodes(lay->blendnode, mainprogram->nodesmain->mixnodescomp[1]);
						}
					}
					else if (dlayers[i + endx]->pos < dlayers.size() - 1) {
						mainprogram->nodesmain->currpage->connect_nodes(lay->blendnode, dlayers[dlayers[i + endx]->pos + 1]->blendnode);
					}	
				}
				else {
					dlayers[i + endx]->blendnode = new BlendNode;
					//BlendNode *bnode = mainprogram->nodesmain->currpage->add_blendnode(MIXING, false);
					Layer *nxlay = NULL;
					if (dlayers.size() > 1) nxlay = dlayers[1];
					if (nxlay) {
						lay->node->out.clear();
						firstlasteffnode->out.clear();
						nxlay->blendnode = firstbnode;
						nxlay->blendnode->blendtype = MIXING;
						mainprogram->nodesmain->currpage->connect_nodes(lay->node, firstlasteffnode, firstbnode);
					}
				}
				dlayers[i + endx]->blendnode->blendtype = btype;
				dlayers[i + endx]->blendnode->mixfac->value = mfval;
				dlayers[i + endx]->blendnode->wipetype = wipetype;
				dlayers[i + endx]->blendnode->wipedir = wipedir;
				dlayers[i + endx]->blendnode->wipex = wipex;
				dlayers[i + endx]->blendnode->wipey = wipey;
				if (slayers.size() == 0) {
					Layer *newlay = mainmix->add_layer(slayers, 0);
					if (&slayers == &mainmix->layersA) {
						mainprogram->nodesmain->currpage->connect_nodes(newlay->lasteffnode, mainprogram->nodesmain->mixnodes[0]);
					}
					else if (&slayers == &mainmix->layersB) {
						mainprogram->nodesmain->currpage->connect_nodes(newlay->lasteffnode, mainprogram->nodesmain->mixnodes[1]);
					}
					else if (&slayers == &mainmix->layersAcomp) {
						mainprogram->nodesmain->currpage->connect_nodes(newlay->lasteffnode, mainprogram->nodesmain->mixnodescomp[0]);
					}
					else if (&slayers == &mainmix->layersBcomp) {
						mainprogram->nodesmain->currpage->connect_nodes(newlay->lasteffnode, mainprogram->nodesmain->mixnodescomp[1]);
					}
				}
				break;
			}
		}
		make_layboxes();
	}
}

std::vector<Layer*>& choose_layers(bool j) {
	if (mainprogram->preveff) {
		if (j == 0) return mainmix->layersA;
		else return mainmix->layersB;
	}
	else {
		if (j == 0) return mainmix->layersAcomp;
		else return mainmix->layersBcomp;
	}
}
void pick_color(Layer *lay, Box *cbox) {
	if (lay->pos > 0) {
		if (cbox->in()) {
			if (mainprogram->leftmouse) {
				lay->cwon = true;
				mainprogram->cwon = true;
				mainprogram->cwx = mainprogram->mx / glob->w;
				mainprogram->cwy = (glob->h - mainprogram->my) / glob->h - 0.15f;
				GLfloat cwx = glGetUniformLocation(mainprogram->ShaderProgram, "cwx");
				glUniform1f(cwx, mainprogram->cwx);
				GLfloat cwy = glGetUniformLocation(mainprogram->ShaderProgram, "cwy");
				glUniform1f(cwy, mainprogram->cwy);
				Box *box = mainprogram->cwbox;
				box->scrcoords->x1 = mainprogram->mx - (glob->w / 10.0f);
				box->scrcoords->y1 = mainprogram->my + (glob->w / 5.0f);
				box->upscrtovtx();
			}
		}
		if (lay->cwon) {
			float vx = mainprogram->mx - (glob->w / 2.0f);
			float vy = mainprogram->my - (glob->h / 2.0f);
			float length = sqrt(pow(abs(vx - (mainprogram->cwx - 0.5f) * glob->w), 2) + pow(abs(vy - (-mainprogram->cwy + 0.5f) * glob->h), 2));
			length /= glob->h / 8.0f;
			if (mainprogram->cwmouse) {
				if (mainprogram->leftmouse) {
					mainprogram->cwmouse = 0;
				}
				else {
					if (length > 0.75f and length < 1.0f) {
						GLint cwmouse = glGetUniformLocation(mainprogram->ShaderProgram, "cwmouse");
						glUniform1i(cwmouse, 1);
						mainprogram->cwmouse = 2;
						GLfloat mx = glGetUniformLocation(mainprogram->ShaderProgram, "mx");
						glUniform1f(mx, mainprogram->mx);
						GLfloat my = glGetUniformLocation(mainprogram->ShaderProgram, "my");
						glUniform1f(my, mainprogram->my);
					}
					else if (mainprogram->cwmouse == 1) {
						mainprogram->cwmouse = 0;
						GLint cwmouse = glGetUniformLocation(mainprogram->ShaderProgram, "cwmouse");
						glUniform1i(cwmouse, 0);
						lay->cwon = false;
						mainprogram->cwon = false;
					}
				}
			}
			if (lay->cwon) {
				GLint cwon = glGetUniformLocation(mainprogram->ShaderProgram, "cwon");
				glUniform1i(cwon, 1);
				Box *box = mainprogram->cwbox;
				draw_box(nullptr, box->acolor, box->vtxcoords->x1, box->vtxcoords->y1, box->vtxcoords->w, box->vtxcoords->h, -1); 
				glUniform1i(cwon, 0);
				if (length <= 0.75f or length >= 1.0f) {
					glReadPixels(mainprogram->mx, glob->h - mainprogram->my, 1, 1, GL_RGBA, GL_FLOAT, &lay->rgb);
					box = cbox;
					box->acolor[0] = lay->rgb[0];
					box->acolor[1] = lay->rgb[1];
					box->acolor[2] = lay->rgb[2];
					box->acolor[3] = 1.0f;
				}
			}
		}
	}
}

int handle_menu(Menu *menu) {
	int ret = handle_menu(menu, 0.0f);
	return ret;
}
int handle_menu(Menu *menu, float xshift) {
	float white[] = {1.0, 1.0, 1.0, 1.0};
	if (menu->state > 1) {
		if (std::find(mainprogram->actmenulist.begin(), mainprogram->actmenulist.end(), menu) == mainprogram->actmenulist.end()) {
			mainprogram->actmenulist.clear();
		}
		int size = 0;
		for(int k = 0; k < menu->entries.size(); k++) {
			std::size_t sub = menu->entries[k].find("submenu");
			if (sub != 0) size++;
		}
		if (mainprogram->menuy > glob->h - size * mainprogram->yvtxtoscr(tf(0.05f))) mainprogram->menuy = glob->h - size * mainprogram->yvtxtoscr(tf(0.05f));
		if (size > 26) mainprogram->menuy = 0.0f;
		float vmx = (float)mainprogram->menux * 2.0 / glob->w;
		float vmy = (float)mainprogram->menuy * 2.0 / glob->h;
		float lc[] = {0.0, 0.0, 0.0, 1.0};
		float ac1[] = {0.3, 0.3, 0.3, 1.0};
		float ac2[] = {0.5, 0.5, 1.0, 1.0};
		int numsubs = 0;
		float limit = 1.0f;
		for(int k = 0; k < menu->entries.size(); k++) {
			float xoff;
			int koff;
			if (k > 25) {
				if (mainprogram->xscrtovtx(mainprogram->menux) > limit) xoff = -tf(0.195f) + xshift;
				else xoff = tf(0.195f) + xshift;
				koff = menu->entries.size() - 26;
			}
			else {
				xoff = 0.0f + xshift;
				koff = 0;
			}
			std::size_t sub = menu->entries[k].find("submenu");
			if (sub != 0) {
				if (menu->box->scrcoords->x1 + mainprogram->menux + mainprogram->xvtxtoscr(xoff) < mainprogram->mx and mainprogram->mx < menu->box->scrcoords->x1 + menu->box->scrcoords->w + mainprogram->menux + mainprogram->xvtxtoscr(xoff) and menu->box->scrcoords->y1 - menu->box->scrcoords->h + mainprogram->menuy + (k - koff - numsubs) * mainprogram->yvtxtoscr(tf(0.05f)) < mainprogram->my and mainprogram->my < menu->box->scrcoords->y1 + mainprogram->menuy + (k - koff - numsubs) * mainprogram->yvtxtoscr(tf(0.05f))) {
					draw_box(lc, ac2, menu->box->vtxcoords->x1 + vmx + xoff, menu->box->vtxcoords->y1 - (k - koff - numsubs) * tf(0.05f) - vmy, tf(0.156f), tf(0.05f), -1);
					if (mainprogram->leftmousedown) mainprogram->leftmousedown = false;
					if (mainprogram->leftmouse) {
						for (int i = 0; i < mainprogram->menulist.size(); i++) {
							mainprogram->menulist[i]->state = 0;
						}
						mainprogram->menuchosen = true;
						return k - numsubs;
					}
				}
				else if (menu->currsub == k - 1) {
					draw_box(lc, ac2, menu->box->vtxcoords->x1 + vmx + xoff, menu->box->vtxcoords->y1 - (k - koff - numsubs) * tf(0.05f) - vmy, tf(0.156f), tf(0.05f), -1);
				}
				else {
					draw_box(lc, ac1, menu->box->vtxcoords->x1 + vmx + xoff, menu->box->vtxcoords->y1 - (k - koff - numsubs) * tf(0.05f) - vmy, tf(0.156f), tf(0.05f), -1);
				}
				render_text(menu->entries[k], white, menu->box->vtxcoords->x1 + vmx + tf(0.0078f) + xoff, menu->box->vtxcoords->y1 - (k - koff - numsubs) * tf(0.05f) - vmy + tf(0.015f), tf(0.0003f), tf(0.0005f));
			}
			else {
				numsubs++;
				if (menu->currsub == k or (menu->box->scrcoords->x1 + mainprogram->menux + mainprogram->xvtxtoscr(xoff) < mainprogram->mx and mainprogram->mx < menu->box->scrcoords->x1 + menu->box->scrcoords->w + mainprogram->menux + mainprogram->xvtxtoscr(xoff) and menu->box->scrcoords->y1 - menu->box->scrcoords->h + mainprogram->menuy + (k - koff - numsubs + 1) * mainprogram->yvtxtoscr(tf(0.05f)) < mainprogram->my and mainprogram->my < menu->box->scrcoords->y1 + mainprogram->menuy + (k - koff - numsubs + 1) * mainprogram->yvtxtoscr(tf(0.05f)))) {
					if (menu->currsub == k or mainprogram->leftmouse) {
						if (menu->currsub != k) mainprogram->leftmouse = false;
						std::string name = menu->entries[k].substr(8, std::string::npos);
						for (int i = 0; i < mainprogram->menulist.size(); i++) {
							if (mainprogram->menulist[i]->name == name) {
								menu->currsub = k;
								mainprogram->menulist[i]->state = 2;
								mainprogram->actmenulist.push_back(menu);
								mainprogram->actmenulist.push_back(mainprogram->menulist[i]);
								float xs;
								if (mainprogram->xscrtovtx(mainprogram->menux) > limit) {
									xs = xshift - tf(0.195f);
								}
								else {
									xs = xshift + tf(0.195f);
								}
								int ret = handle_menu(mainprogram->menulist[i], xs);
								if (mainprogram->menuchosen) {
									menu->state = 0;
									mainprogram->menuresults.insert(mainprogram->menuresults.begin(), ret);
									return k - numsubs + 1;
								}
								mainprogram->menulist[i]->state = 0;
								break;
							}
						}
					}
				}
			}
		}
		for (int i = 0; i < mainprogram->menulist.size(); i++) {
			if (std::find(mainprogram->actmenulist.begin(), mainprogram->actmenulist.end(), menu) == mainprogram->actmenulist.end()) {
				if (mainprogram->menulist[i] != menu) mainprogram->menulist[i]->state = 0;
			}
		}
	}
	return -1;
}


bool preferences() {
	int mx = -1;
	int my = -1;
	if (SDL_GetMouseFocus() == mainprogram->prefwindow) {
		//SDL_PumpEvents();
		SDL_GetMouseState(&mx, &my);
		mx *= 2.0f;
		my *= 2.0f;
	}
	
	mainprogram->insmall = true;
	float white[] = {1.0f, 1.0f, 1.0f, 1.0f};
	float black[] = {0.0f, 0.0f, 0.0f, 1.0f};
	float lightblue[] = {0.5f, 0.5f, 1.0f, 1.0f};
	float green[] = {0.0f, 0.7f, 0.0f, 1.0f};

	SDL_GL_MakeCurrent(mainprogram->prefwindow, glc_pr);
	glClear(GL_COLOR_BUFFER_BIT);
	
	for (int i = 0; i < mainprogram->prefs->items.size(); i++) {
		PrefItem *item = mainprogram->prefs->items[i];
		if (item->box->in(mx, my)) {	
			draw_box(white, lightblue, item->box, -1);
			if (mainprogram->lmsave) {
				mainprogram->prefs->curritem = i;
			}
		}
		else if (mainprogram->prefs->curritem == i) {
			draw_box(white, green, item->box, -1);
		}
		else {
			draw_box(white, black, item->box, -1);
		}
		render_text(item->name, white, item->box->vtxcoords->x1 + 0.03f, item->box->vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, 1);
	}
	draw_box(white, nullptr, -0.5f, -1.0f, 1.5f, 2.0f, -1);
	
	PrefItem *ci = mainprogram->prefs->items[mainprogram->prefs->curritem];
	if (ci->name == "Interface") {
		PIInt *mci = (PIInt*)ci;
		for (int i = 0; i < mci->items.size(); i++) {
			Box box;
			box.vtxcoords->x1 = -0.5f;
			box.vtxcoords->y1 = 1.0f - (i + 1) * 0.2f;
			box.vtxcoords->w = 1.5f;
			box.vtxcoords->h = 0.2f;
			box.upvtxtoscr();
			draw_box(white, black, &box, -1);
			render_text(mci->items[i]->name, white, box.vtxcoords->x1 + 0.23f, box.vtxcoords->y1 + 0.06f, 0.0024f, 0.004f, 1);
			
			box.vtxcoords->x1 = -0.425f;
			box.vtxcoords->y1 = 1.05f - (i + 1) * 0.2f;
			box.vtxcoords->w = 0.1f;
			box.vtxcoords->h = 0.1f;
			box.upvtxtoscr();
			if (box.in(mx, my)) {
				draw_box(white, lightblue, &box, -1);
				if (mainprogram->lmsave) {
					mci->items[i]->onoff = !mci->items[i]->onoff;
				}
			}
			else if (mci->items[i]->onoff) {
				draw_box(white, green, &box, -1);
			}
			else {
				draw_box(white, black, &box, -1);
			}
			mainprogram->needsclick = mci->items[i]->onoff;
		}
	}
	if (ci->name == "Video") {
		PIVid *mci = (PIVid*)ci;
		for (int i = 0; i < mci->items.size(); i++) {
			Box box;
			box.vtxcoords->x1 = -0.5f;
			box.vtxcoords->y1 = 1.0f - (i + 1) * 0.2f;
			box.vtxcoords->w = 1.5f;
			box.vtxcoords->h = 0.2f;
			box.upvtxtoscr();
			draw_box(white, black, &box, -1);
			render_text(mci->items[i]->name, white, box.vtxcoords->x1 + 0.23f, box.vtxcoords->y1 + 0.06f, 0.0024f, 0.004f, 1);
			
			box.vtxcoords->x1 = 0.25f;
			box.vtxcoords->y1 = 1.0f - (i + 1) * 0.2f;
			box.vtxcoords->w = 0.3f;
			box.vtxcoords->h = 0.2f;
			box.upvtxtoscr();
			draw_box(white, black, &box, -1);
			if ((mainprogram->renaming == EDIT_VIDW and mci->items[i]->type == PIVID_W) or (mainprogram->renaming == EDIT_VIDH and mci->items[i]->type == PIVID_H)) {
				std::string part = mainprogram->inputtext.substr(0, mainprogram->cursorpos);
				float textw = render_text(part, white, box.vtxcoords->x1 + 0.1f, box.vtxcoords->y1 + 0.06f, 0.0024f, 0.004f, 1) * 0.5f;
				part = mainprogram->inputtext.substr(mainprogram->cursorpos, mainprogram->inputtext.length() - mainprogram->cursorpos);
				render_text(part, white, box.vtxcoords->x1 + 0.102f + textw, box.vtxcoords->y1 + 0.06f, 0.0024f, 0.004f, 1);
				draw_line(white, box.vtxcoords->x1 + 0.102f + textw, box.vtxcoords->y1 + 0.06f, box.vtxcoords->x1 + 0.102f + textw, box.vtxcoords->y1 + tf(0.09f)); 
			}
			else {
				render_text(std::to_string(mci->items[i]->value), white, box.vtxcoords->x1 + 0.1f, box.vtxcoords->y1 + 0.06f, 0.0024f, 0.004f, 1);
			}
			if (box.in(mx, my)) {
				if (mainprogram->lmsave) {
					if (mci->items[i]->type == PIVID_W) {
						mainprogram->ow = mci->items[i]->value;
						mainprogram->renaming = EDIT_VIDW;
					}
					else if (mci->items[i]->type == PIVID_H) {
						mainprogram->oh = mci->items[i]->value;
						mainprogram->renaming = EDIT_VIDH;
					}
					mainprogram->inputtext = std::to_string(mci->items[i]->value);
					mainprogram->cursorpos = mainprogram->inputtext.length();
					SDL_StartTextInput();
				}
			}
			if (mci->items[i]->type == PIVID_W) {
				mci->items[i]->value = mainprogram->ow;
			}
			else if (mci->items[i]->type == PIVID_H) {
				mci->items[i]->value = mainprogram->oh;
			}
		}
	}
	if (ci->name == "MIDI Devices") {
		PIMidi *mci = (PIMidi*)ci;
		mci->populate();
		for (int i = 0; i < mci->items.size(); i++) {
			Box box;
			box.vtxcoords->x1 = -0.5f;
			box.vtxcoords->y1 = 1.0f - (i + 1) * 0.2f;
			box.vtxcoords->w = 1.5f;
			box.vtxcoords->h = 0.2f;
			box.upvtxtoscr();
			draw_box(white, black, &box, -1);
			render_text(mci->items[i]->name, white, box.vtxcoords->x1 + 0.23f, box.vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, 1);
			
			box.vtxcoords->x1 = -0.425f;
			box.vtxcoords->y1 = 1.05f - (i + 1) * 0.2f;
			box.vtxcoords->w = 0.1f;
			box.vtxcoords->h = 0.1f;
			box.upvtxtoscr();
			if (box.in(mx, my)) {
				draw_box(white, lightblue, &box, -1);
				if (mainprogram->lmsave) {
					mci->items[i]->onoff = !mci->items[i]->onoff;
					if (!mci->items[i]->onoff) {
						if (std::find(mci->onnames.begin(), mci->onnames.end(), mci->items[i]->name) != mci->onnames.end()) {
							mci->onnames.erase(std::find(mci->onnames.begin(), mci->onnames.end(), mci->items[i]->name));
							mci->items[i]->midiin->cancelCallback();
							delete mci->items[i]->midiin;
						}
					}
					else {
						mci->onnames.push_back(mci->items[i]->name);
						RtMidiIn *midiin = new RtMidiIn();
						midiin->setCallback(&mycallback, (void*)mci->items[i]);
						midiin->openPort(i);
						midiin->ignoreTypes( true, true, true );
						mci->items[i]->midiin = midiin;
					}
				}
			}
			else if (mci->items[i]->onoff) {
				draw_box(white, green, &box, -1);
			}
			else {
				draw_box(white, black, &box, -1);
			}
		}
	}
	if (ci->name == "Directories") {
		PIDirs *dci = (PIDirs*)ci;
		EDIT_TYPE type;
		for (int i = 0; i < dci->items.size(); i++) {
			Box box;
			box.vtxcoords->x1 = 0.25f;
			box.vtxcoords->y1 = 1.0f - (i + 1) * 0.2f;
			box.vtxcoords->w = 0.65f;
			box.vtxcoords->h = 0.2f;
			box.upvtxtoscr();
			draw_box(white, black, &box, -1);
			render_text(dci->items[i]->name, white, -0.5f + 0.1f, box.vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, 1);
			if (dci->items[i]->name == "BINS") {
				dci->items[i]->path = mainprogram->binsdir;
				type = EDIT_BINSDIR;
			}
			else if (dci->items[i]->name == "Recordings") {
				dci->items[i]->path = mainprogram->recdir;
				type = EDIT_RECDIR;
			}
			if (mainprogram->renaming != type) {
				render_text(dci->items[i]->path, white, box.vtxcoords->x1 + 0.1f, box.vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, 1);
			}
			else {
				std::string part = mainprogram->inputtext.substr(0, mainprogram->cursorpos);
				float textw = render_text(part, white, box.vtxcoords->x1 + 0.1f, box.vtxcoords->y1 + tf(0.03f), 0.0024f, 0.004f, 1) * 0.5f;
				part = mainprogram->inputtext.substr(mainprogram->cursorpos, mainprogram->inputtext.length() - mainprogram->cursorpos);
				render_text(part, white, box.vtxcoords->x1 + 0.102f + textw, box.vtxcoords->y1 + tf(0.03f), 0.0024f, 0.004f, 1);
				draw_line(white, box.vtxcoords->x1 + 0.102f + textw, box.vtxcoords->y1 + tf(0.028f), box.vtxcoords->x1 + 0.102f + textw, box.vtxcoords->y1 + tf(0.09f)); 
			}
			if (box.in(mx, my)) {
				if (mainprogram->lmsave) {
					if (type == EDIT_BINSDIR) {
						mainprogram->inputtext = mainprogram->binsdir;
					}
					else if (type == EDIT_RECDIR) {
						mainprogram->inputtext = mainprogram->recdir;
					}
					mainprogram->renaming = type;
					mainprogram->cursorpos = mainprogram->inputtext.length();
					SDL_StartTextInput();
				}
			}
			box.vtxcoords->x1 = 0.9f;
			box.vtxcoords->w = 0.1f;
			box.upvtxtoscr();
			draw_box(white, black, &box, -1);
			draw_box(white, black, box.vtxcoords->x1 + 0.02f, box.vtxcoords->y1 + 0.05f, 0.06f, 0.07f, -1);
			draw_box(white, black, box.vtxcoords->x1 + 0.05f, box.vtxcoords->y1 + 0.11f, 0.025f, 0.03f, -1);
			if (box.in(mx, my)) {
				if (mainprogram->lmsave) {
					mainprogram->choosing = type;
					mainprogram->pathto = "CHOOSEDIR";
					std::string title = "Open " + dci->items[i]->name + " directory";
					std::thread filereq (&Program::get_dir, mainprogram, title.c_str());
					filereq.detach();
				}
			}
		}
	}

	Box box;
	box.vtxcoords->x1 = 0.75f;
	box.vtxcoords->y1 = -1.0f;
	box.vtxcoords->w = 0.3f;
	box.vtxcoords->h = 0.2f;
	box.upvtxtoscr();
	draw_box(white, black, &box, -1);
	if (box.in(mx, my)) {
		draw_box(white, lightblue, &box, -1);
		if (mainprogram->lmsave) {
			mainprogram->prefs->load();
			mainprogram->prefon = false;
			mainprogram->drawnonce = false;
			SDL_HideWindow(mainprogram->prefwindow);
			SDL_RaiseWindow(mainprogram->mainwindow);
			return 0;
		}
	}
	render_text("CANCEL", white, box.vtxcoords->x1 + 0.02f, box.vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, 1);
	box.vtxcoords->x1 = 0.45f;
	box.upvtxtoscr();
	draw_box(white, black, &box, -1);
	if (box.in(mx, my)) {
		draw_box(white, lightblue, &box, -1);
		if (mainprogram->lmsave) {
			mainprogram->prefs->save();
			mainprogram->prefon = false;
			mainprogram->drawnonce = false;
			SDL_HideWindow(mainprogram->prefwindow);
			SDL_RaiseWindow(mainprogram->mainwindow);
			return 0;
		}
	}
	render_text("SAVE", white, box.vtxcoords->x1 + 0.02f, box.vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, 1);
			
	mainprogram->middlemouse = 0;
	mainprogram->rightmouse = 0;
	mainprogram->menuactivation = false;
	mainprogram->mx = -1;
	mainprogram->my = -1;
	
	mainprogram->insmall = false;
	
	return 1;
}
		
	
int tune_midi() {
	int mx = -1;
	int my = -1;
	if (SDL_GetMouseFocus() == mainprogram->tunemidiwindow) {
		SDL_PumpEvents();
		SDL_GetMouseState(&mx, &my);
		mx *= 2.0f;
		my *= 2.0f;
	}
	
	mainprogram->insmall = true;
	float white[] = {1.0f, 1.0f, 1.0f, 1.0f};
	float black[] = {0.0f, 0.0f, 0.0f, 1.0f};
	float lightblue[] = {0.5f, 0.5f, 1.0f, 1.0f};
	float green[] = {0.0f, 0.7f, 0.0f, 1.0f};
		
	SDL_GL_MakeCurrent(mainprogram->tunemidiwindow, glc_tm);
	glClear(GL_COLOR_BUFFER_BIT);
	
	if (mainprogram->rightmouse) mainprogram->tmlearn = TM_NONE; 
	
	std::string lmstr;
	if (mainprogram->tunemidideck == 1) lmstr = "A";
	else if (mainprogram->tunemidideck == 2) lmstr = "B";
	else if (mainprogram->tunemidideck == 3) lmstr = "C";
	else if (mainprogram->tunemidideck == 4) lmstr = "D";
	if (mainprogram->tmlearn != TM_NONE) render_text("Creating settings for midideck " + lmstr, white, -0.3f, 0.2f, 0.0024f, 0.004f, 2);
	switch (mainprogram->tmlearn) {
		case TM_NONE:
			break;
		case TM_PLAY:
			render_text("Learn MIDI Play Forward", white, -0.3f, 0.0f, 0.0024f, 0.004f, 2);
			break;
		case TM_BACKW:
			render_text("Learn MIDI Play Backward", white, -0.3f, 0.0f, 0.0024f, 0.004f, 2);
			break;
		case TM_BOUNCE:
			render_text("Learn MIDI Play Bounce", white, -0.3f, 0.0f, 0.0024f, 0.004f, 2);
			break;
		case TM_FRFORW:
			render_text("Learn MIDI Frame Forward", white, -0.3f, 0.0f, 0.0024f, 0.004f, 2);
			break;
		case TM_FRBACKW:
			render_text("Learn MIDI Frame Backward", white, -0.3f, 0.0f, 0.0024f, 0.004f, 2);
			break;
		case TM_SPEED:
			render_text("Learn MIDI Set Play Speed", white, -0.3f, 0.0f, 0.0024f, 0.004f, 2);
			break;
		case TM_SPEEDZERO:
			render_text("Learn MIDI Set Play Speed to Zero", white, -0.3f, 0.0f, 0.0024f, 0.004f, 2);
			break;
		case TM_OPACITY:
			render_text("Learn MIDI Set Opacity", white, -0.3f, 0.0f, 0.0024f, 0.004f, 2);
			break;
		case TM_FREEZE:
			render_text("Learn MIDI Scratchwheel Freeze", white, -0.3f, 0.0f, 0.0024f, 0.004f, 2);
			break;
		case TM_SCRATCH:
			render_text("Learn MIDI Scratchwheel", white, -0.3f, 0.0f, 0.0024f, 0.004f, 2);
			break;
	}
	
	if (mainprogram->tmlearn == TM_NONE) {
		//draw scratchwheel
		draw_box(white, black, mainprogram->tmplay, -1);
		if (mainprogram->tmchoice == TM_PLAY) draw_box(white, green, mainprogram->tmplay, -1);
		if (mainprogram->tmplay->in(mx, my)) {
			draw_box(white, lightblue, mainprogram->tmplay, -1);
			if (mainprogram->lmsave) {
				mainprogram->tmlearn = TM_PLAY;
			}
		}
		draw_triangle(white, white, 0.125f, -0.83f, 0.06f, 0.12f, RIGHT, CLOSED);
		draw_box(white, black, mainprogram->tmbackw, -1);
		if (mainprogram->tmchoice == TM_BACKW) draw_box(white, green, mainprogram->tmbackw, -1);
		if (mainprogram->tmbackw->in(mx, my)) {
			draw_box(white, lightblue, mainprogram->tmbackw, -1);
			if (mainprogram->lmsave) {
				mainprogram->tmlearn = TM_BACKW;
			}
		}
		draw_triangle(white, white, -0.185f, -0.83f, 0.06f, 0.12f, LEFT, CLOSED);
		draw_box(white, black, mainprogram->tmbounce, -1);
		if (mainprogram->tmchoice == TM_BOUNCE) draw_box(white, green, mainprogram->tmbounce, -1);
		if (mainprogram->tmbounce->in(mx, my)) {
			draw_box(white, lightblue, mainprogram->tmbounce, -1);
			if (mainprogram->lmsave) {
				mainprogram->tmlearn = TM_BOUNCE;
			}
		}
		draw_triangle(white, white, -0.045f, -0.83f, 0.04f, 0.12f, LEFT, CLOSED);
		draw_triangle(white, white, 0.01f, -0.83f, 0.04f, 0.12f, RIGHT, CLOSED);
		draw_box(white, black, mainprogram->tmfrforw, -1);
		if (mainprogram->tmchoice == TM_FRFORW) draw_box(white, green, mainprogram->tmfrforw, -1);
		if (mainprogram->tmfrforw->in(mx, my)) {
			draw_box(white, lightblue, mainprogram->tmfrforw, -1);
			if (mainprogram->lmsave) {
				mainprogram->tmlearn = TM_FRFORW;
			}
		}
		draw_triangle(white, white, 0.275f, -0.83f, 0.06f, 0.12f, RIGHT, OPEN);
		draw_box(white, black, mainprogram->tmfrbackw, -1);
		if (mainprogram->tmchoice == TM_FRBACKW) draw_box(white, green, mainprogram->tmfrbackw, -1);
		if (mainprogram->tmfrbackw->in(mx, my)) {
			draw_box(white, lightblue, mainprogram->tmfrbackw, -1);
			if (mainprogram->lmsave) {
				mainprogram->tmlearn = TM_FRBACKW;
			}
		}
		draw_triangle(white, white, -0.335f, -0.83f, 0.06f, 0.12f, LEFT, OPEN);
		draw_box(white, black, mainprogram->tmspeed, -1);
		if (mainprogram->tmchoice == TM_SPEED) draw_box(white, green, mainprogram->tmspeed, -1);
		if (mainprogram->tmspeedzero->in(mx, my)) {
			draw_box(white, lightblue, mainprogram->tmspeedzero, -1);
			if (mainprogram->lmsave) {
				mainprogram->tmlearn = TM_SPEEDZERO;
			}
		}
		else if (mainprogram->tmspeed->in(mx, my)) {
			draw_box(white, lightblue, mainprogram->tmspeed, -1);
			if (mainprogram->lmsave) {
				mainprogram->tmlearn = TM_SPEED;
			}
			draw_box(white, black, mainprogram->tmspeedzero, -1);
			if (mainprogram->tmchoice == TM_SPEEDZERO) draw_box(white, green, mainprogram->tmspeedzero, -1);
		}
		else {
			draw_box(white, black, mainprogram->tmspeedzero, -1);
			if (mainprogram->tmchoice == TM_SPEEDZERO) draw_box(white, green, mainprogram->tmspeedzero, -1);
		}
		render_text("ONE", white, -0.755f, -0.08f, 0.0024f, 0.004f, 2);
		render_text("SPEED", white, -0.765f, -0.48f, 0.0024f, 0.004f, 2);
		draw_box(white, black, mainprogram->tmopacity, -1);
		if (mainprogram->tmchoice == TM_OPACITY) draw_box(white, green, mainprogram->tmopacity, -1);
		if (mainprogram->tmopacity->in(mx, my)) {
			draw_box(white, lightblue, mainprogram->tmopacity, -1);
			if (mainprogram->lmsave) {
				mainprogram->tmlearn = TM_OPACITY;
			}
		}
		render_text("OPACITY", white, 0.605f, -0.48f, 0.0024f, 0.004f, 2);
		if (mainprogram->tmfreeze->in(mx, my)) {
			draw_box(white, lightblue, mainprogram->tmfreeze, -1);
			if (mainprogram->lmsave) {
				mainprogram->tmlearn = TM_FREEZE;
			}
		}
		else {
			if (mainprogram->tmchoice == TM_SCRATCH) draw_box(green, 0.0f, 0.1f, 0.6f, 1);
			if (sqrt(pow((mx / (glob->w / 2.0f) - 1.0f) * glob->w / glob->h, 2) + pow((glob->h - my) / (glob->h / 2.0f) - 1.1f, 2)) < 0.6f) {
				draw_box(lightblue, 0.0f, 0.1f, 0.6f, 1, smw, smh);
				if (mainprogram->lmsave) {
					mainprogram->tmlearn = TM_SCRATCH;
				}
			}
			draw_box(white, black, mainprogram->tmfreeze, -1);
			if (mainprogram->tmchoice == TM_FREEZE) draw_box(white, green, mainprogram->tmfreeze, -1);
		}
		draw_box(white, 0.0f, 0.1f, 0.6f, 2, smw, smh);
		render_text("SCRATCH", white, -0.1f, -0.3f, 0.0024f, 0.004f, 2);
		render_text("FREEZE", white, -0.08f, 0.12f, 0.0024f, 0.004f, 2);
	}
	
	Box box;
	box.vtxcoords->x1 = 0.75f;
	box.vtxcoords->y1 = -1.0f;
	box.vtxcoords->w = 0.3f;
	box.vtxcoords->h = 0.2f;
	box.upvtxtoscr();
	draw_box(white, black, &box, -1);
	if (box.in(mx, my)) {
		draw_box(white, lightblue, &box, -1);
		if (mainprogram->lmsave) {
			open_genmidis("./midiset.gm");
			mainprogram->tunemidi = false;
			SDL_HideWindow(mainprogram->tunemidiwindow);
			return 0;
		}
	}
	render_text("CANCEL", white, box.vtxcoords->x1 + 0.02f, box.vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, 2);
	box.vtxcoords->x1 = 0.45f;
	box.upvtxtoscr();
	draw_box(white, black, &box, -1);
	if (box.in(mx, my)) {
		draw_box(white, lightblue, &box, -1);
		if (mainprogram->lmsave) {
			save_genmidis("./midiset.gm");
			mainprogram->tunemidi = false;
			SDL_HideWindow(mainprogram->tunemidiwindow);
			return 0;
		}
	}
	render_text("SAVE", white, box.vtxcoords->x1 + 0.02f, box.vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, 2);
		
	mainprogram->middlemouse = 0;
	mainprogram->rightmouse = 0;
	mainprogram->menuactivation = false;
	mainprogram->mx = -1;
	mainprogram->my = -1;
	
	mainprogram->insmall = false;
	
	return 1;
}


void output_video(EWindow *mwin) {
	SDL_GL_MakeCurrent(mwin->win, mwin->glc);
	glUseProgram(mainprogram->ShaderProgram);
	
	GLfloat vcoords1[8];
	GLfloat *p = vcoords1;
	*p++ = -1.0f; *p++ = 1.0f;
	*p++ = -1.0f; *p++ = -1.0f;
	*p++ = 1.0f; *p++ = 1.0f;
	*p++ = 1.0f; *p++ = -1.0f;
	GLfloat tcoords[] = {0.0f, 0.0f,
						0.0f, 1.0f,
						1.0f, 0.0f,
						1.0f, 1.0f};
	glGenBuffers(1, &mwin->vbuf);
	glBindBuffer(GL_ARRAY_BUFFER, mwin->vbuf);
	glBufferData(GL_ARRAY_BUFFER, 32, vcoords1, GL_STATIC_DRAW);
	glGenBuffers(1, &mwin->tbuf);
	glBindBuffer(GL_ARRAY_BUFFER, mwin->tbuf);
	glBufferData(GL_ARRAY_BUFFER, 32, tcoords, GL_STATIC_DRAW);
	glGenVertexArrays(1, &mwin->vao);
	glBindVertexArray(mwin->vao);
	glBindBuffer(GL_ARRAY_BUFFER, mwin->vbuf);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, NULL);
	glBindBuffer(GL_ARRAY_BUFFER, mwin->tbuf);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8, NULL);

	MixNode *node;
	while(1) {
		std::unique_lock<std::mutex> lock(mwin->syncmutex);
		mwin->sync.wait(lock, [&]{return mwin->syncnow;});
		mwin->syncnow = false;
		lock.unlock();
		
		// receive end of thread signal
		if (mwin->closethread) {
			mwin->closethread = false;
			return;
		}
		
		if (mwin->mixid == mainprogram->nodesmain->mixnodes.size()) node = (MixNode*)mainprogram->nodesmain->mixnodescomp[mwin->mixid - 1];
		else node = (MixNode*)mainprogram->nodesmain->mixnodes[mwin->mixid];
		
		GLuint temptex;
		GLuint mixtex;
		temptex = copy_tex(node->mixtex, mainprogram->ow, (int)mainprogram->oh);
		mixtex = copy_tex(temptex, mainprogram->ow, (int)mainprogram->oh);

		GLfloat cf = glGetUniformLocation(mainprogram->ShaderProgram, "cf");
		GLint wipe = glGetUniformLocation(mainprogram->ShaderProgram, "wipe");
		GLint mixmode = glGetUniformLocation(mainprogram->ShaderProgram, "mixmode");
		if (mainmix->wipe[mwin->mixid == 2] > -1) {
			if (mwin->mixid == 2) glUniform1f(cf, mainmix->crossfadecomp->value);
			else glUniform1f(cf, mainmix->crossfade->value);
			glUniform1i(wipe, 1);
			glUniform1i(mixmode, 18);
		}
		if (mwin->mixid != 2 or mainmix->compon) {
			GLint down = glGetUniformLocation(mainprogram->ShaderProgram, "down");
			glUniform1i(down, 1);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, mixtex);
			glBindVertexArray(mwin->vao);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			glUniform1i(down, 0);
		}
		else {
			glClearColor( 0.f, 0.f, 0.f, 0.f );
			glClear(GL_COLOR_BUFFER_BIT);
		}
		glUniform1i(wipe, 0);
		glUniform1i(mixmode, 0);
		SDL_GL_SwapWindow(mwin->win);
		glDeleteTextures(1, &temptex);
		glDeleteTextures(1, &mixtex);
	}
}


void enddrag() {
	if (mainprogram->dragbinel) {
		//if (mainprogram->dragbinel->path.rfind, ".layer", nullptr != std::string::npos) {
		//	if (mainprogram->dragbinel->path.find("cliptemp_") != std::string::npos) {
		//		boost::filesystem::remove(mainprogram->dragbinel->path);			
		//	}
		//}
		mainprogram->dragbinel = nullptr;
		if (mainprogram->draglay) mainprogram->draglay->vidmoving = false;
		mainprogram->draglay = nullptr;
		mainprogram->dragpath = "";
		mainmix->moving = false;
		//glDeleteTextures(1, &binsmain->dragtex);  maybe needs implementing in one case, check history
	}
}
	

void the_loop() {
	glClearColor(0.2f, 0.2f, 0.2f, 0.2f);
	glClear(GL_COLOR_BUFFER_BIT);

	float white[] = {1.0f, 1.0f, 1.0f, 1.0f};
	float halfwhite[] = {1.0f, 1.0f, 1.0f, 0.5f};
	float black[] = {0.0f, 0.0f, 0.0f, 1.0f};
	float red[] = {1.0f, 0.5f, 0.5f, 1.0f};
	float blue[] = {0.5f, 0.5f, 1.0f, 1.0f};
	float grey[] = {0.5f, 0.5f, 0.5f, 1.0f};
	float darkgrey[] = {0.2f, 0.2f, 0.2f, 1.0f};
	float lightblue[] = {0.5f, 0.5f, 1.0f, 1.0f};
	
	if (mainprogram->blocking) {
		mainprogram->mx = -1;
		mainprogram->my = -1;
	}
	if (SDL_GetMouseFocus() == mainprogram->prefwindow or SDL_GetMouseFocus() == mainprogram->tunemidiwindow) {
		mainprogram->mx = -1;
		mainprogram->my = -1;
		mainprogram->lmsave = mainprogram->leftmouse;
		mainprogram->leftmouse = false;
		mainprogram->rightmouse = false;
	}
	if (mainmix->adaptparam) {
		// no hovering while adapting param
		mainprogram->my = -1;
	}
	
	if (mainprogram->preveff) {
		loopstation = lp;
	}
	else {
		loopstation = lpc;
	}

	if (mainprogram->openshelfdir) {
		// load one item from mainprogram->opendir into shelf, one each loop not to slowdown output stream
		open_shelfdir();
	}
		
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < mainmix->nbframesA[i].size(); j++) {
			if (i == mainmix->page[0]) break;
			calc_texture(mainmix->nbframesA[i][j], 0, 0);
		}
		for (int j = 0; j < mainmix->nbframesB[i].size(); j++) {
			if (i == mainmix->page[1]) break;
			calc_texture(mainmix->nbframesB[i][j], 0, 0);
		}
	}
	if (!mainprogram->preveff) {
		//when in performance mode: count normal layers frames (alive = 0)
		for(int i = 0; i < mainmix->layersA.size(); i++) {
			Layer *testlay = mainmix->layersA[i];
			calc_texture(testlay, 0, 0);
		}
		for(int i = 0; i < mainmix->layersB.size(); i++) {
			Layer *testlay = mainmix->layersB[i];
			calc_texture(testlay, 0, 0);
		}
		for(int i = 0; i < mainmix->layersAcomp.size(); i++) {
			Layer *testlay = mainmix->layersAcomp[i];
			calc_texture(testlay, 1, 1);
		}
		for(int i = 0; i < mainmix->layersBcomp.size(); i++) {
			Layer *testlay = mainmix->layersBcomp[i];
			calc_texture(testlay, 1, 1);
		}
		for(int i = 0; i < mainprogram->busylayers.size(); i++) {
			Layer *testlay = mainprogram->busylayers[i];  // needs to be revised
			calc_texture(testlay, 1, 1);
		}
	}
	if (mainprogram->preveff) {
		for(int i = 0; i < mainmix->layersA.size(); i++) {
			Layer *testlay = mainmix->layersA[i];
			calc_texture(testlay, 0, 1);
		}
		for(int i = 0; i < mainmix->layersB.size(); i++) {
			Layer *testlay = mainmix->layersB[i];
			calc_texture(testlay, 0, 1);
		}
	}
	if (mainprogram->preveff and mainmix->compon) {
		for(int i = 0; i < mainmix->layersAcomp.size(); i++) {
			Layer *testlay = mainmix->layersAcomp[i];
			calc_texture(testlay, 1, 1);
		}
		for(int i = 0; i < mainmix->layersBcomp.size(); i++) {
			Layer *testlay = mainmix->layersBcomp[i];
			calc_texture(testlay, 1, 1);
		}
	}
		
	// Crawl web
	make_layboxes();
	if (mainprogram->preveff) walk_nodes(0);
	walk_nodes(1);
	
	//draw and handle BINS wormhole
	float darkgreen[4] = {0.0f, 0.3f, 0.0f, 1.0f};
	if (!mainprogram->menuondisplay) {
		if (sqrt(pow((mainprogram->mx / (glob->w / 2.0f) - 1.0f) * glob->w / glob->h, 2) + pow((glob->h - mainprogram->my) / (glob->h / 2.0f) - 1.25f, 2)) < 0.2f) {
			darkgreen[0] = 0.5f;
			darkgreen[1] = 0.5f;
			darkgreen[2] = 1.0f;
			darkgreen[3] = 1.0f;
			if (mainprogram->leftmouse and !binsmain->dragmix) {
				mainprogram->binsscreen = !mainprogram->binsscreen;
				mainprogram->leftmouse = false;
			}
		}
	}
		
	
	if (mainprogram->renaming != EDIT_NONE and mainprogram->leftmouse) mainprogram->renaming = EDIT_NONE;
	
	// wormhole
	draw_box(darkgreen, 0.0f, 0.25f, 0.2f, 1);
	draw_box(white, 0.0f, 0.25f, 0.2f, 2);
	draw_box(white, 0.0f, 0.25f, 0.15f, 2);
	draw_box(white, 0.0f, 0.25f, 0.1f, 2);
	if (mainprogram->binsscreen) render_text("MIX", white, -0.024f, 0.24f, 0.0006f, 0.001f);
	else render_text("BINS", white, -0.025f, 0.24f, 0.0006f, 0.001f);
	
	
	if (mainprogram->binsscreen) {
		binsmain->handle();
	}
	else {  //the_loop else
		if ((mainprogram->effectmenu->state > 1) or (mainprogram->mixmodemenu->state > 1) or (mainprogram->mixenginemenu->state > 1) or (mainprogram->parammenu1->state > 1) or (mainprogram->parammenu2->state > 1) or (mainprogram->loopmenu->state > 1) or (mainprogram->deckmenu->state > 1) or (mainprogram->laymenu->state > 1) or (mainprogram->loadmenu->state > 1) or (mainprogram->mixtargetmenu->state > 1) or (mainprogram->wipemenu->state > 1) or (mainprogram->livemenu->state > 1) or (mainprogram->binmenu->state > 1) or (mainprogram->binelmenu->state > 1) or (mainprogram->genmidimenu->state > 1) or (mainprogram->genericmenu->state > 1) or (mainprogram->shelfmenu1->state > 1) or (mainprogram->shelfmenu2->state > 1)) {
			mainprogram->leftmousedown = false;
			mainprogram->menuondisplay = true;
		}
		else {
			mainprogram->menuondisplay = false;
		}
			
		if (mainmix->learn and mainprogram->rightmouse) {
			mainmix->learn = false;
			mainprogram->rightmouse = false;
		}
	
		if (mainprogram->cwon) {
			if (mainprogram->leftmousedown) {
				mainprogram->cwmouse = 1;
				mainprogram->leftmousedown = false;
			}
		}
					
		// Draw and handle modebox
		Box *box = mainmix->modebox;
		// draw_box(box, -1);
		// if (box->in()) {
			// draw_box(white, lightblue, box, -1);
			// if (mainprogram->leftmouse) {
				// //mainprogram->quit("quitted");
			// }
		// }
	// 
		// const char *modestr;
		// switch (mainmix->mode) {
			// case 0:
				// modestr = "QUIT";
				// break;
			// case 1:
				// modestr = "Node View";
				// break;
		// }
		// render_text(modestr, white, box->vtxcoords->x1 + 0.015f, box->vtxcoords->y1 + 0.04f, 0.0006f, 0.001f);
	
		
		// Handle params
		if (mainmix->adaptparam) {
			mainmix->adaptparam->value += (mainprogram->mx - mainmix->prevx) * (mainmix->adaptparam->range[1] - mainmix->adaptparam->range[0]) / mainprogram->xvtxtoscr(mainmix->adaptparam->box->vtxcoords->w);
			if (mainmix->adaptparam->value < mainmix->adaptparam->range[0]) mainmix->adaptparam->value = mainmix->adaptparam->range[0];
			if (mainmix->adaptparam->value > mainmix->adaptparam->range[1]) mainmix->adaptparam->value = mainmix->adaptparam->range[1];
			mainmix->prevx = mainprogram->mx;
			
			if (mainmix->adaptparam->shadervar == "ripple") {
				((RippleEffect*)mainmix->adaptparam->effect)->speed = mainmix->adaptparam->value;
			}
			else {
				GLint var = glGetUniformLocation(mainprogram->ShaderProgram, mainmix->adaptparam->shadervar.c_str());
				glUniform1f(var, mainmix->adaptparam->value);
				mainmix->midiparam = nullptr;
			}
			
			for (int i = 0; i < loopstation->elems.size(); i++) {
				if (loopstation->elems[i]->recbut->value) {
					loopstation->elems[i]->add_param();
				}
			}
			
			if (mainprogram->leftmouse) {
				mainprogram->leftmouse = false;
				mainmix->adaptparam = nullptr;
			}
		}
		
		
		//blue bars designating layer selection
		float blue[4] = {0.1, 0.1, 0.6, 1.0};
		if (mainmix->currlay) {
			draw_box(nullptr, blue, -1.0f + mainmix->numw + mainmix->currlay->deck * 1.0f + (mainmix->currlay->pos - mainmix->scrollpos[mainmix->currlay->deck]) * mainmix->layw, -0.1f + tf(0.05f), mainmix->layw, 1.1f, -1);
		}
			
		//draw wormhole
		draw_box(darkgreen, 0.0f, 0.25f, 0.2f, 1);
		draw_box(white, 0.0f, 0.25f, 0.2f, 2);
		draw_box(white, 0.0f, 0.25f, 0.15f, 2);
		draw_box(white, 0.0f, 0.25f, 0.1f, 2);
		if (mainprogram->binsscreen) render_text("MIX", white, -0.024f, 0.24f, 0.0006f, 0.001f);
		else render_text("BINS", white, -0.025f, 0.24f, 0.0006f, 0.001f);
		
		// Draw crossfade->box
		Param *par;
		if (mainprogram->preveff) par = mainmix->crossfade;
		else par = mainmix->crossfadecomp;
		float green[4] = {0.0f, 1.0f, 0.2f, 1.0f};
		draw_box(par->box, -1);
		const char *parstr2;
		if (mainmix->learnparam == par and mainmix->learn) {
			parstr2 = "MIDI";
		}
		else parstr2 = "Crossfade";
		render_text(parstr2, white, par->box->vtxcoords->x1 + tf(0.01f), par->box->vtxcoords->y1 + tf(0.05f) - tf(0.030f), tf(0.0003f), tf(0.0005f));
		if (par->box->in()) {	
			if (mainprogram->menuactivation) {
				if (loopstation->elemmap.find(par) != loopstation->elemmap.end()) mainprogram->parammenu2->state = 2;
				else mainprogram->parammenu1->state = 2;
				mainmix->learnparam = par;
				mainprogram->menuondisplay = true;
				mainprogram->menuactivation = false;
			}
			if (mainprogram->leftmousedown) {
				mainprogram->leftmousedown = false;
				mainmix->adaptparam = par;
				mainmix->prevx = mainprogram->mx;
			}
		}
		draw_box(green, green, par->box->vtxcoords->x1 + par->box->vtxcoords->w * ((par->value - par->range[0]) / (par->range[1] - par->range[0])) - tf(0.00078f), par->box->vtxcoords->y1, tf(0.00156f), par->box->vtxcoords->h, -1);
			
		// Draw effectboxes
		// Bottom bar
		float sx1, sy1, vx1, vy1, vx2, vy2, sw;
		bool bottom = false;
		bool inbetween = false;
		Layer *lay = mainmix->currlay;
		Effect *eff = nullptr;
		if (!mainprogram->queueing) {
			if (lay->effects.size()) {
				eff = lay->effects[lay->effects.size() - 1];
				box = eff->onoffbutton->box;
				sx1 = box->scrcoords->x1;
				sy1 = box->scrcoords->y1 + (eff->numrows - 1) * mainprogram->yvtxtoscr(tf(0.05f));
				vx1 = box->vtxcoords->x1;
				vy1 = box->vtxcoords->y1 - (eff->numrows - 1) * tf(0.05f);
				sw = tf(mainmix->layw) * glob->w / 2.0;
			}
			else {
				box = lay->mixbox;
				sw = tf(mainmix->layw) * glob->w / 2.0;
				sx1 = box->scrcoords->x1 + mainprogram->xvtxtoscr(tf(0.025f));
				sy1 = mainmix->layw * glob->h / 2.0 + mainprogram->yvtxtoscr(tf(0.25f));
				vx1 = box->vtxcoords->x1 + tf(0.025f);
				vy1 = 1 - mainmix->layw - tf(0.25f);
			}
			if (!mainprogram->menuondisplay) {
				if (sx1 < mainprogram->mx and mainprogram->mx < sx1 + sw) {
					bool cond1 = (sy1 + mainprogram->yvtxtoscr(tf(0.01f)) < mainprogram->my and mainprogram->my < sy1 + mainprogram->yvtxtoscr(tf(0.048f)));
					bool cond2 = (sy1 - 7.5 <= mainprogram->my and mainprogram->my <= sy1 + 7.5);
					if (cond2) {
						inbetween = true;
						vx2 = vx1;
						vy2 = vy1 - tf(0.011f);
					}
					if (cond1 or cond2) {
						if (mainprogram->menuactivation or mainprogram->leftmouse) {
							mainprogram->effectmenu->state = 2;
							mainmix->insert = 1;
							mainmix->mouseeffect = lay->effects.size();
							mainmix->mouselayer = lay;
							mainprogram->menux = mainprogram->mx;
							mainprogram->menuy = mainprogram->my;
							mainprogram->leftmouse = false;
							mainprogram->menuactivation = false;
							mainprogram->menuondisplay = true;
						}
						bottom = true;
					}
				}
			}
			for(int j = 0; j < lay->effects.size(); j++) {
				eff = lay->effects[j];
				box = eff->box;
				eff->box->acolor[0] = 0.75f;
				eff->box->acolor[1] = 0.0f;
				eff->box->acolor[2] = 0.0f;
				eff->box->acolor[3] = 1.0f;
				if (!mainprogram->menuondisplay) {
					if (box->scrcoords->x1 < mainprogram->mx and mainprogram->mx < box->scrcoords->x1 + box->scrcoords->w) {
						if (box->scrcoords->y1 - box->scrcoords->h + 7.5 <= mainprogram->my and mainprogram->my <= box->scrcoords->y1 - 7.5) {
							if (mainprogram->del) {
								lay->delete_effect(j);
								mainprogram->del = 0;
								break;
							}
							if (mainprogram->menuactivation or mainprogram->leftmouse) {
								mainprogram->effectmenu->state = 2;
								mainmix->mouselayer = lay;
								mainmix->mouseeffect = j;
								mainmix->insert = false;
								mainprogram->menux = mainprogram->mx;
								mainprogram->menuy = mainprogram->my;
								mainprogram->leftmouse = false;
								mainprogram->menuactivation = false;
								mainprogram->menuondisplay = true;
							}
							eff->box->acolor[0] = 0.5;
							eff->box->acolor[1] = 0.5;
							eff->box->acolor[2] = 1.0;
							eff->box->acolor[3] = 1.0;
						}
					}
					box = eff->onoffbutton->box;
					if (box->scrcoords->x1 < mainprogram->mx and mainprogram->mx < box->scrcoords->x1 + tf(mainmix->layw) * glob->w / 2.0) {
						if (box->scrcoords->y1 - box->scrcoords->h - 7.5 < mainprogram->my and mainprogram->my < box->scrcoords->y1 - box->scrcoords->h + 7.5) {
							// mouse over "Insert Effect" box, inbetween effects
							if (mainprogram->menuactivation or mainprogram->leftmouse) {
								mainprogram->effectmenu->state = 2;
								mainmix->insert = true;
								mainmix->mouseeffect = j;
								mainmix->mouselayer = lay;
								mainprogram->menux = mainprogram->mx;
								mainprogram->menuy = mainprogram->my;
								mainprogram->leftmouse = false;
								mainprogram->menuactivation = false;
							}
							mainprogram->menuondisplay = true;
							inbetween = true;
							vx2 = box->vtxcoords->x1;
							vy2 = box->vtxcoords->y1 + box->vtxcoords->h - tf(0.011f);
						}
					}
				}
				if (eff->onoffbutton->box->in()) {
					if (mainprogram->leftmouse) {
						eff->onoffbutton->value = !eff->onoffbutton->value;
					}
				}
				if (mainprogram->menuactivation and !mainprogram->menuondisplay) {
					for (int k = 0; k < eff->params.size(); k++) {
						if (eff->params[k]->box) {
							if (eff->params[k]->box->in()) {
								if (loopstation->elemmap.find(eff->params[k]) != loopstation->elemmap.end()) mainprogram->parammenu2->state = 2;
								else mainprogram->parammenu1->state = 2;
								mainmix->learnparam = eff->params[k];
								mainprogram->menuactivation = false;
								break;
							}
						}
					}
				}
			}
		}


		if (mainprogram->preveff) {
			for(int i = 0; i < mainmix->layersA.size(); i++) {
				Layer *testlay = mainmix->layersA[i];
				display_texture(testlay, 0);
			}
			for(int i = 0; i < mainmix->layersB.size(); i++) {
				Layer *testlay = mainmix->layersB[i];
				display_texture(testlay, 1);
			}
		}
		else {
			for(int i = 0; i < mainmix->layersAcomp.size(); i++) {
				Layer *testlay = mainmix->layersAcomp[i];
				display_texture(testlay, 0);
			}
			for(int i = 0; i < mainmix->layersBcomp.size(); i++) {
				Layer *testlay = mainmix->layersBcomp[i];
				display_texture(testlay, 1);
			}
		}
		
		if (mainmix->mode == 0) {
			displaymix();
		}
		
	
		// Handle numboxes
		handle_numboxes(mainmix->numboxesA);
		handle_numboxes(mainmix->numboxesB);
		
		// Draw and handle overall genmidi button
		Button *but;
		for (int i = 0; i < 2; i++) {
			std::vector<Layer*> &lvec = choose_layers(i);
			if (i == 0) but = mainmix->genmidi[0];
			else but = mainmix->genmidi[1];
			if (but->box->in()) {
				but->box->acolor[0] = 0.5f;
				but->box->acolor[1] = 0.5f;
				but->box->acolor[2] = 1.0f;
				but->box->acolor[3] = 1.0f;
				if (mainprogram->leftmouse) {
					bool found = false;
					for (int j = 0; j < lvec.size(); j++) {
						if (but->value != lvec[j]->genmidibut->value) {
							found = true;
							break;
						}
					}
					if (!found) but->value++;
					if (but->value == 5) but->value = 0;
					for (int j = 0; j < lvec.size(); j++) {
						lvec[j]->genmidibut->value = but->value;
					}
				}
				if (mainprogram->menuactivation and but->value != 0) {
					mainprogram->tunemidideck = but->value;
					mainprogram->genmidimenu->state = 2;
					mainprogram->rightmouse = false;
					mainprogram->menuactivation = false;
				}
			}
			else {
				but->box->acolor[0] = 0.5;
				but->box->acolor[1] = 0.2;
				but->box->acolor[2] = 0.5;
				but->box->acolor[3] = 1.0;
				if (but->value) {
					but->box->acolor[0] = 1.0;
					but->box->acolor[1] = 0.5;
					but->box->acolor[2] = 0.0;
					but->box->acolor[3] = 1.0;
				}
			}
			draw_box(but->box, -1);
			std::string butstr;
			if (but->value == 0) butstr = "off";
			else if (but->value == 1) butstr = "A";
			else if (but->value == 2) butstr = "B";
			else if (but->value == 3) butstr = "C";
			else if (but->value == 4) butstr = "D";
			render_text(butstr, white, but->box->vtxcoords->x1 + 0.0078f, but->box->vtxcoords->y1 + 0.0416f - 0.030, 0.0006, 0.001);
		}
		
		// Handle node view
		if (mainmix->mode == 1) {
			mainprogram->nodesmain->currpage->handle_nodes();
		}
		
		
		// Draw and handle buttons
		if (mainprogram->preveff) {
			if (mainprogram->toscreen->box->in()) {
				mainprogram->toscreen->box->acolor[0] = 0.5;
				mainprogram->toscreen->box->acolor[1] = 0.5;
				mainprogram->toscreen->box->acolor[2] = 1.0;
				mainprogram->toscreen->box->acolor[3] = 1.0;
				if (mainprogram->leftmouse) {
					// MAKE LIVE button copies preview set entirely to comp set
					mainmix->firststage = false;  //??????
					mainmix->compon = true;
					mainmix->copy_to_comp(mainmix->layersA, mainmix->layersAcomp, mainmix->layersB, mainmix->layersBcomp, mainprogram->nodesmain->currpage->nodes, mainprogram->nodesmain->currpage->nodescomp, mainprogram->nodesmain->mixnodescomp, true);
					mainprogram->leftmouse = false;
				}
			}
			else {
				mainprogram->toscreen->box->acolor[0] = 0.0;
				mainprogram->toscreen->box->acolor[1] = 0.0;
				mainprogram->toscreen->box->acolor[2] = 0.0;
				mainprogram->toscreen->box->acolor[3] = 1.0;
			}
			draw_box(mainprogram->toscreen->box, -1);
			render_text(mainprogram->toscreen->name, white, mainprogram->toscreen->box->vtxcoords->x1 + tf(0.0078f), mainprogram->toscreen->box->vtxcoords->y1 + tf(0.015f), 0.0006f, 0.001f);
		}
		if (mainprogram->preveff) {
			if (mainprogram->backtopre->box->in()) {
				mainprogram->backtopre->box->acolor[0] = 0.5;
				mainprogram->backtopre->box->acolor[1] = 0.5;
				mainprogram->backtopre->box->acolor[2] = 1.0;
				mainprogram->backtopre->box->acolor[3] = 1.0;
				if (mainprogram->leftmouse) {
					// BACK button copies comp set entirely back to preview set
					mainmix->copy_to_comp(mainmix->layersAcomp, mainmix->layersA, mainmix->layersBcomp, mainmix->layersB, mainprogram->nodesmain->currpage->nodescomp, mainprogram->nodesmain->currpage->nodes, mainprogram->nodesmain->mixnodes, false);
					mainprogram->leftmouse = false;
				}
			}
			else {
				mainprogram->backtopre->box->acolor[0] = 0.0;
				mainprogram->backtopre->box->acolor[1] = 0.0;
				mainprogram->backtopre->box->acolor[2] = 0.0;
				mainprogram->backtopre->box->acolor[3] = 1.0;
			}
			draw_box(mainprogram->backtopre->box, -1);
			render_text(mainprogram->backtopre->name, white, mainprogram->backtopre->box->vtxcoords->x1 + tf(0.0078f), mainprogram->backtopre->box->vtxcoords->y1 + tf(0.015f), 0.0006, 0.001);
		}
		if (mainprogram->preveff) {
			if (mainprogram->vidprev->box->in()) {
				mainprogram->vidprev->box->acolor[0] = 0.5;
				mainprogram->vidprev->box->acolor[1] = 0.5;
				mainprogram->vidprev->box->acolor[2] = 1.0;
				mainprogram->vidprev->box->acolor[3] = 1.0;
				if (mainprogram->leftmouse) {
					// toggles video preview mode (video frame throughput to comp from preview (off)
					// or separate videos in normal and comp (on)
					mainprogram->prevvid = !mainprogram->prevvid;
					if (!mainprogram->prevvid) mainprogram->prevvid_off();
				}
			}
			else {
				if (mainprogram->prevvid) {
					mainprogram->vidprev->box->acolor[0] = 1.0;
					mainprogram->vidprev->box->acolor[1] = 0.5;
					mainprogram->vidprev->box->acolor[2] = 0.0;
					mainprogram->vidprev->box->acolor[3] = 1.0;
				}
				else {
					mainprogram->vidprev->box->acolor[0] = 0.0;
					mainprogram->vidprev->box->acolor[1] = 0.0;
					mainprogram->vidprev->box->acolor[2] = 0.0;
					mainprogram->vidprev->box->acolor[3] = 1.0;
				}
			}
			//if (mainmix->compon) {
				draw_box(mainprogram->vidprev->box, -1);
				render_text(mainprogram->vidprev->name, white, mainprogram->vidprev->box->vtxcoords->x1 + tf(0.0078f), mainprogram->vidprev->box->vtxcoords->y1 + tf(0.015f), 0.00042, 0.00070);
			//}
		}
		if (mainprogram->effprev->box->in()) {
			mainprogram->effprev->box->acolor[0] = 0.5;
			mainprogram->effprev->box->acolor[1] = 0.5;
			mainprogram->effprev->box->acolor[2] = 1.0;
			mainprogram->effprev->box->acolor[3] = 1.0;
			if (mainprogram->leftmouse) {
				//effprev is button that toggles effect preview mode to performance mode and back
				mainprogram->preveff = !mainprogram->preveff;
				mainprogram->preveff_init();
			}
		}
		else {
			if (mainprogram->preveff) {
				mainprogram->effprev->box->acolor[0] = 1.0;
				mainprogram->effprev->box->acolor[1] = 0.5;
				mainprogram->effprev->box->acolor[2] = 0.0;
				mainprogram->effprev->box->acolor[3] = 1.0;
			}
			else {
				mainprogram->effprev->box->acolor[0] = 0.0;
				mainprogram->effprev->box->acolor[1] = 0.0;
				mainprogram->effprev->box->acolor[2] = 0.0;
				mainprogram->effprev->box->acolor[3] = 1.0;
			}
		}
		draw_box(mainprogram->effprev->box, -1);
		render_text(mainprogram->effprev->name, white, mainprogram->effprev->box->vtxcoords->x1 + tf(0.0078f), mainprogram->effprev->box->vtxcoords->y1 + tf(0.015f), 0.00042, 0.00070);
		
		//draw and handle recbut
		if (mainmix->compon) {
			if (mainmix->recbut->box->in()) {
				mainmix->recbut->box->acolor[0] = 0.5;
				mainmix->recbut->box->acolor[1] = 0.5;
				mainmix->recbut->box->acolor[2] = 1.0;
				mainmix->recbut->box->acolor[3] = 1.0;
				if (mainprogram->leftmouse) {
					if (!mainmix->recording) {
						// start recording
						mainmix->start_recording();
					}
					else {
						mainmix->recording = false;
					}
				}
			}
			else {
				mainmix->recbut->box->acolor[0] = 0.0;
				mainmix->recbut->box->acolor[1] = 0.0;
				mainmix->recbut->box->acolor[2] = 0.0;
				mainmix->recbut->box->acolor[3] = 1.0;
			}
			draw_box(mainmix->recbut->box, -1);
			float radx = mainmix->recbut->box->vtxcoords->w / 2.0f;
			float rady = mainmix->recbut->box->vtxcoords->h / 2.0f;
			float red2[4] = {1.0f, 0.0f, 0.0f, 1.0f};
			if (mainmix->recording) {
				draw_box(red2, mainmix->recbut->box->vtxcoords->x1 + radx, mainmix->recbut->box->vtxcoords->y1 + rady, tf(0.015f), 1);
			}	
			else draw_box(red2, mainmix->recbut->box->vtxcoords->x1 + radx, mainmix->recbut->box->vtxcoords->y1 + rady, tf(0.015f), 2);
		}
	
		
		// Draw effectmenuhints
		if(!mainprogram->queueing) {
			if (vy1 < 1.0 - tf(mainmix->layw) - tf(0.22f) - tf(0.05f) * 10) {
				vy1 = 1.0 - tf(mainmix->layw) - tf(0.20f) - tf(0.05f) * 10;
			}
			if (bottom) {
				bottom = false;
				draw_box(white, lightblue, vx1, vy1 - tf(0.05f), tf(mainmix->layw), tf(0.038f), -1);
			}
			else {
				draw_box(white, black, vx1, vy1 - tf(0.05f), tf(mainmix->layw), tf(0.038f), -1);
			}
			render_text("+ Add effect", white, vx1 + tf(0.01f), vy1 - tf(0.035f), tf(0.0003f), tf(0.0005f));
	
			if (inbetween) {
				inbetween = false;
				draw_box(lightblue, lightblue, vx2, vy2, tf(mainmix->layw), tf(0.022f), -1);
				draw_box(white, lightblue, mainprogram->mx * 2.0f / glob->w - 1.0f - tf(0.08f), 1.0f - (mainprogram->my * 2.0f / glob->h) - tf(0.019f), tf(0.16f), tf(0.038f), -1);
				render_text("+ Insert effect", white, mainprogram->mx * 2.0f / glob->w - 1.0f - tf(0.07f), 1.0f - (mainprogram->my * 2.0f / glob->h) - tf(0.004f), tf(0.0003f), tf(0.0005f));
			}
		}
		
		
		//Handle scrollbar
		bool inbox = false;
		for (int i = 0; i < 2; i++) {
			std::vector<Layer*> &lvec = choose_layers(i);
			draw_box(white, nullptr, -1.0f + mainmix->numw + i, 1.0f - mainmix->layw - 0.05f, mainmix->layw * 3.0f, 0.05f, -1);
			Box box;
			Box boxbefore;
			Box boxafter;
			Box boxlayer;
			int size = lvec.size() + 1;
			if (size < 3) size = 3;
			box.vtxcoords->x1 = -1.0f + mainmix->numw + i + mainmix->scrollpos[i] * (mainmix->layw * 3.0f / size); 
			box.vtxcoords->y1 = 1.0f - mainmix->layw - 0.05f;
			box.vtxcoords->w = (3.0f / size) * mainmix->layw * 3.0f;
			box.vtxcoords->h = 0.05f;
			box.upvtxtoscr();
			boxbefore.vtxcoords->x1 = -1.0f + mainmix->numw + i; 
			boxbefore.vtxcoords->y1 = 1.0f - mainmix->layw - 0.05f;
			boxbefore.vtxcoords->w = (3.0f / size) * mainmix->layw * mainmix->scrollpos[i];
			boxbefore.vtxcoords->h = 0.05f;
			boxbefore.upvtxtoscr();
			boxafter.vtxcoords->x1 = -1.0f + mainmix->numw + i + (mainmix->scrollpos[i] + 3) * (mainmix->layw * 3.0f / size); 
			boxafter.vtxcoords->y1 = 1.0f - mainmix->layw - 0.05f;
			boxafter.vtxcoords->w = (3.0f / size) * mainmix->layw * (size - 3);
			boxafter.vtxcoords->h = 0.05f;
			boxafter.upvtxtoscr();
			if (box.in()) {
				draw_box(white, lightblue, &box, -1);
				if (mainprogram->leftmousedown and mainmix->scrollon == 0 and !mainprogram->menuondisplay) {
					mainmix->scrollon = i + 1;
					mainmix->scrollmx = mainprogram->mx;
					mainprogram->leftmousedown = false;
				}
			}
			else {
				draw_box(white, grey, &box, -1);
				boxlayer.vtxcoords->y1 = 1.0f - mainmix->layw - 0.05f;
				boxlayer.vtxcoords->w = 0.062f;
				boxlayer.vtxcoords->h = 0.05f;
				for (int j = 0; j < lvec.size(); j++) {
					boxlayer.vtxcoords->x1 = -1.0f + mainmix->numw + i + (j + mainmix->scrollpos[i]) * (mainmix->layw * 3.0f / size); 
					boxlayer.upvtxtoscr();
					draw_box(nullptr, lvec[j]->scrollcol, &boxlayer, -1);
				}
			}
			box.vtxcoords->w /= 3.0f;
			if (mainmix->scrollon == i + 1) {
				if (mainprogram->leftmouse) {
					mainmix->scrollon = 0;
					mainprogram->leftmouse = false;
				}
				else {
					if ((mainprogram->mx - mainmix->scrollmx) > mainprogram->xvtxtoscr(box.vtxcoords->w)) {
						if (mainmix->scrollpos[i] < size - 3) {
							mainmix->scrollpos[i]++;
							mainmix->scrollmx += mainprogram->xvtxtoscr(box.vtxcoords->w);
						}	
					}
					else if ((mainprogram->mx - mainmix->scrollmx) < -mainprogram->xvtxtoscr(box.vtxcoords->w)) {
						if (mainmix->scrollpos[i] > 0) {
							mainmix->scrollpos[i]--;
							mainmix->scrollmx -= mainprogram->xvtxtoscr(box.vtxcoords->w);
						}
					}
				}
			}
			if (boxbefore.in()) {
				inbox = true;
				if (mainprogram->dragbinel) {
					if (mainmix->scrolltime == 0.0f) {
						mainmix->scrolltime = mainmix->time;
					}
					else {
						if (mainmix->time - mainmix->scrolltime > 1.0f) {
							mainmix->scrollpos[i]--;
							mainmix->scrolltime = mainmix->time;
						}
					}
				}
				if (mainprogram->leftmouse) {
					mainmix->scrollpos[i]--;
				}
			}
			else if (boxafter.in()) {
				inbox = true;
				if (mainprogram->dragbinel) {
					if (mainmix->scrolltime == 0.0f) {
						mainmix->scrolltime = mainmix->time;
					}
					else {
						if (mainmix->time - mainmix->scrolltime > 1.0f) {
							mainmix->scrollpos[i]++;
							mainmix->scrolltime = mainmix->time;
						}
					}
				}
				if (mainprogram->leftmouse) {
					mainmix->scrollpos[i]++;
				}
			}
			
			box.vtxcoords->x1 = -1.0f + mainmix->numw + i; 
			for (int j = 0; j < size; j++) {
				draw_box(white, nullptr, &box, -1);
				render_text(std::to_string(j + 1), white, box.vtxcoords->x1 + 0.0078f, box.vtxcoords->y1 + 0.0078f, 0.0006, 0.001);
				box.vtxcoords->x1 += box.vtxcoords->w;
			}
		} 
		if (!inbox) mainmix->scrolltime = 0.0f;

		
		for (int i = 0; i < 2; i++) {
			std::vector<Layer*> &lays = choose_layers(i);
			for (int j = 0; j < lays.size(); j++) {
				Layer *lay2 = lays[j];
				// draw clips queue?
				box = lay2->node->vidbox;
				if (lay2->queueing and lay2->filename != "") {
					int until = lay2->clips.size() - lay2->queuescroll;
					if (until > 4) until = 4;
					for (int k = 0; k < until; k++) {
						draw_box(white, black, box->vtxcoords->x1, box->vtxcoords->y1 - (k + 1) * mainmix->layw, box->vtxcoords->w, box->vtxcoords->h, lay2->clips[k + lay2->queuescroll]->tex);
						render_text("Queued clip #" + std::to_string(k + lay2->queuescroll + 1), white, box->vtxcoords->x1 + tf(0.01f), box->vtxcoords->y1 - k * box->vtxcoords->h - tf(0.03f), 0.0005f, 0.0008f);
					}
					for (int k = 0; k < lay2->clips.size() - lay2->queuescroll; k++) {
						if (box->scrcoords->x1 + (k == 0) * mainprogram->xvtxtoscr(0.075f) < mainprogram->mx and mainprogram->mx < box->scrcoords->x1 + box->scrcoords->w - (k == 0) * mainprogram->xvtxtoscr(0.075f)) {
							if (box->scrcoords->y1 + (k - 0.25f) * box->scrcoords->h < mainprogram->my and mainprogram->my < box->scrcoords->y1 + (k + 0.25) * box->scrcoords->h) {
								if (mainprogram->dragbinel) draw_box(lightblue, lightblue, box->vtxcoords->x1 + (k == 0) * 0.075f, box->vtxcoords->y1 - (k + 0.25f) * mainmix->layw, box->vtxcoords->w - ((k == 0) * 0.075) * 2.0f, box->vtxcoords->h / 2.0f, -1);
							}
							if (box->scrcoords->y1 + k * box->scrcoords->h < mainprogram->my and mainprogram->my < box->scrcoords->y1 + (k + 1) * box->scrcoords->h) {
								if (mainprogram->mousewheel) {
									lay2->queuescroll -= mainprogram->mousewheel;
									if (lay2->clips.size() >= 4) {
										lay2->queuescroll = std::clamp(lay2->queuescroll, 0, (int)(lay2->clips.size() - 4));
									}
									break;
								}
								if (mainprogram->leftmousedown) {
									if (k + lay2->queuescroll != lay2->clips.size() - 1) {
										Clip *clip = lay2->clips[k + lay2->queuescroll];
										mainprogram->dragbinel = new BinElement;
										mainprogram->dragbinel->type = clip->type;
										mainprogram->dragbinel->path = clip->path;
										mainprogram->dragbinel->tex = clip->tex;
										binsmain->dragtex = clip->tex;
										lay2->clips.erase(lay2->clips.begin() + k + lay2->queuescroll);
										mainprogram->dragclip = clip;
										mainprogram->draglay = lay2;
										mainprogram->dragpos = k + lay2->queuescroll;
									}
									mainprogram->leftmousedown = false;
								}
							}
						}
					}
				}
			}
		}

		visu_thumbs();
		
		
		// draw and handle loopstation
		if (mainprogram->preveff) {
			lp->handle();
			if (mainmix->compon) {
				for (int i = 0; i < lpc->elems.size(); i++) {
					if (lpc->elems[i]->loopbut->value or lpc->elems[i]->playbut->value) lpc->elems[i]->set_params();
				}			
			}
		}
		else {
			lpc->handle();
		}
		
		
		int k = -1;
		// Do mainprogram->mixenginemenu
		k = handle_menu(mainprogram->mixenginemenu);
		if (k == 0) {
			if (mainmix->mousenode) {
				if (mainmix->mousenode->type == BLEND) {
					((BlendNode*)mainmix->mousenode)->blendtype = (BLEND_TYPE)(mainprogram->menuresults[0] + 1);
				}
				mainmix->mousenode = nullptr;
			}
		}
		else if (k == 1) {
			if (mainmix->mousenode) {
				if (mainmix->mousenode->type == BLEND) {
					if (mainprogram->menuresults[0] == 0) {
						((BlendNode*)mainmix->mousenode)->blendtype = MIXING;
					}
					else {
						((BlendNode*)mainmix->mousenode)->blendtype = WIPE;
						((BlendNode*)mainmix->mousenode)->wipetype = mainprogram->menuresults[0] - 1;
						((BlendNode*)mainmix->mousenode)->wipedir = mainprogram->menuresults[1];
					}
				}
				mainmix->mousenode = nullptr;
			}
		}
		if (mainprogram->menuchosen) {
			mainprogram->menuchosen = false;
			mainprogram->leftmouse = 0;
			mainprogram->menuactivation = 0;
			mainprogram->menuresults.clear();
		}
		
		// Draw and handle mainprogram->effectmenu
		k = handle_menu(mainprogram->effectmenu);
		if (k > -1) {
			if (k == 0) mainmix->mouselayer->delete_effect(mainmix->mouseeffect);
			else if (mainmix->insert) mainmix->mouselayer->add_effect((EFFECT_TYPE)(k - 1), mainmix->mouseeffect);
			else {
				int mon = mainmix->mouselayer->effects[mainmix->mouseeffect]->node->monitor;
				mainmix->mouselayer->replace_effect((EFFECT_TYPE)(k - 1), mainmix->mouseeffect);
				mainmix->mouselayer->effects[mainmix->mouseeffect]->node->monitor = mon;
			}
			mainmix->mouselayer = NULL;
			mainmix->mouseeffect = -1;
		}
		if (mainprogram->menuchosen) {
			mainprogram->menuchosen = false;
			mainprogram->leftmouse = 0;
			mainprogram->menuactivation = 0;
			mainprogram->menuresults.clear();
		}
		
		// Draw and handle mainprogram->parammenu1
		k = handle_menu(mainprogram->parammenu1);
		if (k > -1) {
			if (k == 0) {
				mainmix->learn = true;
			}
		}
		if (mainprogram->menuchosen) {
			mainprogram->menuchosen = false;
			mainprogram->leftmouse = 0;
			mainprogram->menuactivation = 0;
			mainprogram->menuresults.clear();
		}
		
		// Draw and handle mainprogram->parammenu2
		k = handle_menu(mainprogram->parammenu2);
		if (k > -1) {
			if (k == 0) {
				mainmix->learn = true;
			}
			else if (k == 1) {
				LoopStationElement *elem = loopstation->elemmap[mainmix->learnparam];
				elem->params.erase(mainmix->learnparam);
				if (mainmix->learnparam->effect) elem->layers.erase(mainmix->learnparam->effect->layer);
				for (int i = elem->eventlist.size() - 1; i >= 0; i--) {
					if (std::get<1>(elem->eventlist[i]) == mainmix->learnparam) elem->eventlist.erase(elem->eventlist.begin() + i);
				}
				loopstation->elemmap.erase(mainmix->learnparam);
				mainmix->learnparam->box->acolor[0] = 0.2f;
				mainmix->learnparam->box->acolor[1] = 0.2f;
				mainmix->learnparam->box->acolor[2] = 0.2f;
				mainmix->learnparam->box->acolor[3] = 1.0f;
			}
		}
		if (mainprogram->menuchosen) {
			mainprogram->menuchosen = false;
			mainprogram->leftmouse = 0;
			mainprogram->menuactivation = 0;
			mainprogram->menuresults.clear();
		}
		
		// Draw and handle mainprogram->loopmenu
		k = handle_menu(mainprogram->loopmenu);
		if (k > -1) {
			if (k == 0) {
				mainmix->mouselayer->startframe = mainmix->mouselayer->frame;
				if (mainmix->mouselayer->startframe > mainmix->mouselayer->endframe) {
					mainmix->mouselayer->endframe = mainmix->mouselayer->startframe;
				}
			}
			else if (k == 1) {
				mainmix->mouselayer->endframe = mainmix->mouselayer->frame;
				if (mainmix->mouselayer->startframe > mainmix->mouselayer->endframe) {
					mainmix->mouselayer->startframe = mainmix->mouselayer->endframe;
				}
			}
		}
		if (mainprogram->menuchosen) {
			mainprogram->menuchosen = false;
			mainprogram->leftmouse = 0;
			mainprogram->menuactivation = 0;
			mainprogram->menuresults.clear();
		}
		
		// Draw and handle mixtargetmenu
		std::vector<OutputEntry*> currentries;
		std::vector<int> possscreens;
		std::vector<OutputEntry*> takenentries;
		if (mainprogram->mixtargetmenu->state > 1) {
			// make the output display menu (SDL_GetNumVideoDisplays() doesn't allow hotplugging screens... :( )
			int numd = SDL_GetNumVideoDisplays();
			std::vector<std::string> mixtargets;
			mixtargets.push_back("submenu wipemenu");
			mixtargets.push_back("Choose wipe");
			std::vector<int> currscreens;
			if (numd == 1) mixtargets.push_back("No external displays");
			else {
				for (int i = 0; i < mainprogram->outputentries.size(); i++) {
					if (mainprogram->outputentries[i]->win->mixid == mainprogram->mixtargetmenu->value) {
						// current display(s) of chosen mixwindow, allow shutting down
						currentries.push_back(mainprogram->outputentries[i]);
						if (currentries.size() == 1) mixtargets.push_back("Stop displaying at:");
						mixtargets.push_back(SDL_GetDisplayName(currentries.back()->screen));
						currscreens.push_back(currentries.back()->screen);
					}
					else {
						// other entries taking up chosen screen
						takenentries.push_back(mainprogram->outputentries[i]);
					}
				}
			}
			bool first = true;
			for (int i = 1; i < numd; i++) {
				if (std::find(currscreens.begin(), currscreens.end(), i) == currscreens.end()) {
					// possible new screens to start outputting to
					if (first) {
						first = false;
						mixtargets.push_back("Mix down to display:");
					}
					mixtargets.push_back(SDL_GetDisplayName(i));
					possscreens.push_back(i);
				}
			}
			mainprogram->make_menu("mixtargetmenu", mainprogram->mixtargetmenu, mixtargets);
		}
		k = handle_menu(mainprogram->mixtargetmenu);
		if (k > -1) {
			if (k == 0) {
				if (mainprogram->menuresults[!mainprogram->preveff] == 0) {
					mainmix->wipe[!mainprogram->preveff] = -1;
				}
				else {
					mainmix->wipe[!mainprogram->preveff] = mainprogram->menuresults[0] - 1;
					mainmix->wipedir[!mainprogram->preveff] = mainprogram->menuresults[1];
				}
			}
			if (k > 1) {
				// chosen output screen already used? re-use window
				bool switched = false;
				for (int i = 0; i < takenentries.size(); i++) {
					if (takenentries[i]->screen == k - currentries.size() - 1 - (currentries.size() != 0)) {
						takenentries[i]->win->mixid = mainprogram->mixtargetmenu->value;
						switched = true;
					}
				}
				if (switched) {}
				else if (currentries.size()) {
					// stop output display of chosen mixwindow on chosen screen
					if (k > 1 and k < 2 + currentries.size()) {
						SDL_DestroyWindow(currentries[k - 2]->win->win);
						mainprogram->outputentries.erase(std::find(mainprogram->outputentries.begin(), mainprogram->outputentries.end(), currentries[k - 2]));
						// deleting entry itself?  closethread...
						currentries[k - 2]->win->closethread = true;
						while (currentries[k - 2]->win->closethread) {
							currentries[k - 2]->win->syncnow = true;
							currentries[k - 2]->win->sync.notify_one();
						}
					}
				}
				else {
					// open new window on chosen ouput screen and start display thread (synced at end of the_loop)
					int screen;
					if (currentries.size()) screen = possscreens[k - currentries.size() - 3];
					else screen = possscreens[k - currentries.size() - 2];
					EWindow *mwin = new EWindow;
					mwin->mixid = mainprogram->mixtargetmenu->value;
					OutputEntry *entry = new OutputEntry;
					entry->screen = screen;
					entry->win = mwin;
					mainprogram->outputentries.push_back(entry);
					SDL_Rect rc;
					SDL_GetDisplayUsableBounds(screen, &rc);
					auto sw = rc.w;
					auto sh = rc.h;
					mwin->win = SDL_CreateWindow(PROGRAM_NAME, SDL_WINDOWPOS_CENTERED_DISPLAY(screen), SDL_WINDOWPOS_CENTERED_DISPLAY(screen), sw, sh, SDL_WINDOW_OPENGL | SDL_WINDOW_MAXIMIZED | SDL_WINDOW_RESIZABLE | SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_ALLOW_HIGHDPI);
					SDL_RaiseWindow(mainprogram->mainwindow);
					mainprogram->mixwindows.push_back(mwin);
					mwin->glc = SDL_GL_CreateContext(mwin->win);
					GLuint prog = mainprogram->set_shader();
					glUseProgram(prog);
					
					std::thread vidoutput (output_video, mwin);
					vidoutput.detach();
					
					SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
				}
			}
		}
		if (mainprogram->menuchosen) {
			mainprogram->menuchosen = false;
			mainprogram->leftmouse = 0;
			mainprogram->menuactivation = 0;
			mainprogram->menuresults.clear();
		}
		
	
		// Draw and handle wipemenu
		k = handle_menu(mainprogram->wipemenu);
		if (k > 0) {
			mainmix->wipe[!mainprogram->preveff] = k - 1;
			mainmix->wipedir[!mainprogram->preveff] = mainprogram->menuresults[0];
		}
		else if (k == 0) {
			mainmix->wipe[!mainprogram->preveff] = -1;
		}
		
		if (mainprogram->menuchosen) {
			mainprogram->menuchosen = false;
			mainprogram->leftmouse = 0;
			mainprogram->menuactivation = 0;
			mainprogram->menuresults.clear();
		}
				
		// Draw and handle deckmenu
		k = handle_menu(mainprogram->deckmenu);
		if (k == 0) {
			mainprogram->pathto = "OPENDECK";
			std::thread filereq (&Program::get_inname, mainprogram, "Open deck file", "application/ewocvj2-deck", "");
			filereq.detach();
		}
		else if (k == 1) {
			mainprogram->pathto = "SAVEDECK";
			std::thread filereq (&Program::get_outname, mainprogram, "Save deck file", "application/ewocvj2-deck", "");
			filereq.detach();
		}
		if (mainprogram->menuchosen) {
			mainprogram->menuchosen = false;
			mainprogram->leftmouse = 0;
			mainprogram->menuactivation = 0;
			mainprogram->menuresults.clear();
		}
		
		
		// Draw and handle mainprogram->laymenu
		if (mainprogram->laymenu->state > 1) {
			get_cameras();
			mainprogram->devices.clear();
			int numd = mainprogram->livedevices.size();
			if (numd == 0) mainprogram->devices.push_back("No live devices");
			else mainprogram->devices.push_back("Connect live to:");
			for (int i = 0; i < numd; i++) {
				std::string str(mainprogram->livedevices[i].begin(), mainprogram->livedevices[i].end());
				mainprogram->devices.push_back(str);
			}
			mainprogram->make_menu("livemenu", mainprogram->livemenu, mainprogram->devices);
			mainprogram->livemenu->box->upscrtovtx();
		}
		k = handle_menu(mainprogram->laymenu);
		if (k > -1) {
			if (k == 0) {
				if (mainprogram->menuresults[0] > 0) {
					#ifdef _WIN64
					std::string livename = "video=" + mainprogram->devices[mainprogram->menuresults[0]];
					#else
					#ifdef __linux__
					std::string livename = "/dev/video" + std::to_string(mainprogram->menuresults[0] - 1);
					#endif
					#endif
					set_live_base(mainmix->mouselayer, livename);
				}
			}
			if (k == 1) {
				mainprogram->pathto = "OPENVIDEO";
				std::thread filereq (&Program::get_inname, mainprogram, "Open video file", nullptr, "");
				filereq.detach();
				mainprogram->loadlay = mainmix->mouselayer;
			}
			else if (k == 2) {
				mainprogram->pathto = "OPENLAYFILE";
				std::thread filereq (&Program::get_inname, mainprogram, "Open layer file", "application/ewocvj2-layer", "");
				filereq.detach();
			}
			else if (k == 3) {
				mainprogram->pathto = "SAVELAYFILE";
				std::thread filereq (&Program::get_outname, mainprogram, "Save layer file", "application/ewocvj2-layer", "");
				filereq.detach();
			}
			else if (k == 4) {
				new_file(mainmix->mousedeck, 1);
			}
			else if (k == 5) {
				mainprogram->pathto = "OPENDECK";
				std::thread filereq (&Program::get_inname, mainprogram, "Open deck file", "application/ewocvj2-deck", "");
				filereq.detach();
			}
			else if (k == 6) {
				mainprogram->pathto = "SAVEDECK";
				std::thread filereq (&Program::get_outname, mainprogram, "Save deck file", "application/ewocvj2-deck", "");
				filereq.detach();
			}
			else if (k == 7) {
				new_file(2, 1);
			}
			else if (k == 8) {
				mainprogram->pathto = "OPENMIX";
				std::thread filereq (&Program::get_inname, mainprogram, "Open mix file", "application/ewocvj2-mix", "");
				filereq.detach();
			}
			else if (k == 9) {
				mainprogram->pathto = "SAVEMIX";
				std::thread filereq (&Program::get_outname, mainprogram, "Open mix file", "application/ewocvj2-mix", "");
				filereq.detach();
			}
			else if (k == 10) {
				if (std::find(mainmix->layersA.begin(), mainmix->layersA.end(), mainmix->mouselayer) != mainmix->layersA.end()) {
					mainmix->delete_layer(mainmix->layersA, mainmix->mouselayer, true);
				}
				else if (std::find(mainmix->layersB.begin(), mainmix->layersB.end(), mainmix->mouselayer) != mainmix->layersB.end()) {
					mainmix->delete_layer(mainmix->layersB, mainmix->mouselayer, true);
				}
				else if (std::find(mainmix->layersAcomp.begin(), mainmix->layersAcomp.end(), mainmix->mouselayer) != mainmix->layersAcomp.end()) {
					mainmix->delete_layer(mainmix->layersAcomp, mainmix->mouselayer, true);
				}
				else if (std::find(mainmix->layersBcomp.begin(), mainmix->layersBcomp.end(), mainmix->mouselayer) != mainmix->layersBcomp.end()) {
					mainmix->delete_layer(mainmix->layersBcomp, mainmix->mouselayer, true);
				}
			}
			else if (k == 11) {
				std::vector<Layer*> &lvec = choose_layers(mainmix->mouselayer->deck);
				Layer *clonelay = mainmix->clone_layer(lvec, mainmix->mouselayer);
				if (mainmix->clonemap.find(mainmix->mouselayer) != mainmix->clonemap.end()) {
					mainmix->clonemap.emplace(clonelay, mainmix->clonemap[mainmix->mouselayer]);
				}
				else {
					std::unordered_set<Layer*> *uset = new std::unordered_set<Layer*>;
					mainmix->clonemap.emplace(mainmix->mouselayer, uset);
					mainmix->clonemap.emplace(clonelay, uset);
					uset->emplace(mainmix->mouselayer);
				}
				mainmix->clonemap[mainmix->mouselayer]->emplace(clonelay);
			}
			else if (k == 12) {
				mainmix->mouselayer->shiftx = 0.0f;
				mainmix->mouselayer->shifty = 0.0f;
			}
		}
	
		if (mainprogram->menuchosen) {
			mainprogram->menuchosen = false;
			mainprogram->leftmouse = 0;
			mainprogram->menuactivation = 0;
			mainprogram->menuresults.clear();
		}
		
			
		k = handle_menu(mainprogram->loadmenu);
		if (k > -1) {
			std::vector<Layer*> &lvec = choose_layers(mainmix->mousedeck);
			if (k == 0) {
				if (mainprogram->menuresults[0] > 0) {
					mainmix->mouselayer = mainmix->add_layer(lvec, lvec.size());
					#ifdef _WIN64
					std::string livename = "video=" + mainprogram->devices[mainprogram->menuresults[0]];
					#else
					#ifdef __linux__
					std::string livename = "/dev/video" + std::to_string(mainprogram->menuresults[0] - 1);
					#endif
					#endif
					set_live_base(mainmix->mouselayer, livename);
				}
			}
			if (k == 1) {
				mainprogram->pathto = "OPENVIDEO";
				std::thread filereq (&Program::get_inname, mainprogram, "Open video file", nullptr, "");
				filereq.detach();
				mainprogram->loadlay = mainmix->add_layer(lvec, lvec.size());
			}
			else if (k == 2) {
				mainmix->mouselayer = mainmix->add_layer(lvec, lvec.size());
				mainprogram->pathto = "OPENLAYFILE";
				std::thread filereq (&Program::get_inname, mainprogram, "Open layer file", "application/ewocvj2-layer", "");
				filereq.detach();
			}
			else if (k == 3) {
				new_file(mainmix->mousedeck, 1);
			}
			else if (k == 4) {
				mainprogram->pathto = "OPENDECK";
				std::thread filereq (&Program::get_inname, mainprogram, "Open deck file", "application/ewocvj2-deck", "");
				filereq.detach();
			}
			else if (k == 5) {
				mainprogram->pathto = "SAVEDECK";
				std::thread filereq (&Program::get_outname, mainprogram, "Save deck file", "application/ewocvj2-deck", "");
				filereq.detach();
			}
			else if (k == 6) {
				new_file(2, 1);
			}
			else if (k == 7) {
				mainprogram->pathto = "OPENMIX";
				std::thread filereq (&Program::get_inname, mainprogram, "Open mix file", "application/ewocvj2-mix", "");
				filereq.detach();
			}
			else if (k == 8) {
				mainprogram->pathto = "SAVEMIX";
				std::thread filereq (&Program::get_outname, mainprogram, "Save mix file", "application/ewocvj2-mix", "");
				filereq.detach();
			}
		}
	
		if (mainprogram->menuchosen) {
			mainprogram->menuchosen = false;
			mainprogram->leftmouse = 0;
			mainprogram->menuactivation = 0;
			mainprogram->menuresults.clear();
		}
		
		
		// Draw and handle genmidimenu
		k = handle_menu(mainprogram->genmidimenu);
		if (k == 0) {
			if (!mainprogram->tunemidi) {
				SDL_ShowWindow(mainprogram->tunemidiwindow);
				SDL_RaiseWindow(mainprogram->tunemidiwindow);
				//mainprogram->share_lists(&glc, mainprogram->mainwindow, &glc_tm, mainprogram->tunemidiwindow);
				SDL_GL_MakeCurrent(mainprogram->tunemidiwindow, glc_tm);
				glUseProgram(mainprogram->ShaderProgram_tm);
				mainprogram->tmfreeze->upvtxtoscr();
				mainprogram->tmplay->upvtxtoscr();
				mainprogram->tmbackw->upvtxtoscr();
				mainprogram->tmbounce->upvtxtoscr();
				mainprogram->tmfrforw->upvtxtoscr();
				mainprogram->tmfrbackw->upvtxtoscr();
				mainprogram->tmspeed->upvtxtoscr();
				mainprogram->tmspeedzero->upvtxtoscr();
				mainprogram->tmopacity->upvtxtoscr();
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				mainprogram->tunemidi = true;
			}
			else {
				SDL_RaiseWindow(mainprogram->tunemidiwindow);
			}
		}
	
		if (mainprogram->menuchosen) {
			mainprogram->menuchosen = false;
			mainprogram->leftmouse = 0;
			mainprogram->menuactivation = 0;
			mainprogram->menuresults.clear();
		}
		
		// Draw and handle genericmenu
		k = handle_menu(mainprogram->genericmenu);
		if (k == 0) {
			new_state();
		}
		else if (k == 1) {
			mainprogram->pathto = "OPENSTATE";
			std::thread filereq (&Program::get_inname, mainprogram, "Open state file", "application/ewocvj2-state", "");
			filereq.detach();
		}
		else if (k == 2) {
			mainprogram->pathto = "SAVESTATE";
			std::thread filereq (&Program::get_outname, mainprogram, "Save state file", "application/ewocvj2-state", "");
			filereq.detach();
		}
		else if (k == 3) {
			if (!mainprogram->prefon) {
				mainprogram->prefs->load();
				mainprogram->prefon = true;
				SDL_ShowWindow(mainprogram->prefwindow);
				SDL_RaiseWindow(mainprogram->prefwindow);
				SDL_GL_MakeCurrent(mainprogram->prefwindow, glc_pr);
				glUseProgram(mainprogram->ShaderProgram_pr);
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				for (int i = 0; i < mainprogram->prefs->items.size(); i++) {
					PrefItem *item = mainprogram->prefs->items[i];
					item->box->upvtxtoscr();
				}
			}
			else {
				SDL_RaiseWindow(mainprogram->prefwindow);
			}
		}
		else if (k == 4) {
			mainprogram->quit("quitted");
		}
	
		if (mainprogram->menuchosen) {
			mainprogram->menuchosen = false;
			mainprogram->leftmouse = 0;
			mainprogram->menuactivation = 0;
			mainprogram->menuresults.clear();
		}
				
		// Draw and handle shelfmenu
		if (mainprogram->mainshelf) k = handle_menu(mainprogram->shelfmenu1);
		else k = handle_menu(mainprogram->shelfmenu2);
		if (k == 0) {
			new_shelf(mainmix->mousedeck);
		}
		else if (k == 1) {
			mainprogram->pathto = "OPENSHELF";
			#ifdef _WIN64
			std::string sdir = ".\\shelves\\";
			#else
			#ifdef __linux__
			std::string homedir (getenv("HOME"));
			std::string sdir = homedir + "/.ewocvj2/shelves";
			#endif
			#endif
			printf("shelfdir %s\n", sdir.c_str());
			std::thread filereq (&Program::get_inname, mainprogram, "Open shelf file", "application/ewocvj2-shelf", sdir);
			filereq.detach();
		}
		else if (k == 2) {
			mainprogram->pathto = "SAVESHELF";
			#ifdef _WIN64
			std::string sdir = ".\\shelves\\";
			#else
			#ifdef __linux__
			std::string homedir (getenv("HOME"));
			std::string sdir = homedir + "/.ewocvj2/shelves";
			#endif
			#endif
			std::thread filereq (&Program::get_outname, mainprogram, "Save shelf file", "application/ewocvj2-shelf", sdir);
			filereq.detach();
		}
		else if (k == 3) {
			mainprogram->pathto = "OPENSHELFVIDEO";
			std::thread filereq (&Program::get_inname, mainprogram,  "Load video file in shelf", "", "");
			filereq.detach();
		}
		else if (k == 4) {
			mainprogram->pathto = "OPENSHELFLAYER";
			std::thread filereq (&Program::get_inname, mainprogram, "Load layer file in shelf", "application/ewocvj2-layer", "");
			filereq.detach();
		}
		else if (k == 5) {
			mainprogram->pathto = "OPENSHELFDIR";
			std::thread filereq (&Program::get_dir, mainprogram, "Load directory into shelf");
			filereq.detach();
		}
		else if (k == 6) {
			#ifdef _WIN64
			if (exists("./shelves/shelfs.shelf")) open_shelf("./shelves/shelfs.shelf", 2);
			# else
			#ifdef __linux__
			std::string homedir (getenv("HOME"));
			if (exists(homedir + "/.ewocvj2/shelves/shelfs.shelf")) open_shelf(homedir + "/.ewocvj2/shelves/shelfs.shelf", 2);
			#endif
			#endif
			else {
				new_shelf(0);
				new_shelf(1);
			}
			mainprogram->mainshelf = true;
		}
	
		if (mainprogram->menuchosen) {
			mainprogram->menuchosen = false;
			mainprogram->leftmouse = 0;
			mainprogram->menuactivation = 0;
			mainprogram->menuresults.clear();
		}
		
		// end of menu code
		
		//add/delete layer
		if (!mainprogram->menuondisplay) {
			for (int j = 0; j < 2; j++) {
				std::vector<Layer*> &lvec = choose_layers(j);
				for (int i = 0; i < lvec.size(); i++) {
					Layer *lay = lvec[i];
					if (lay->pos < mainmix->scrollpos[j] or lay->pos > mainmix->scrollpos[j] + 2) continue;
					Box *box = lay->node->vidbox;
					float thick;
					if (mainprogram->dragbinel) thick = mainprogram->xvtxtoscr(0.075f);
					else thick = mainprogram->xvtxtoscr(tf(0.006f));
					if (box->scrcoords->y1 - box->scrcoords->h < mainprogram->my and mainprogram->my < box->scrcoords->y1) {
						if (box->scrcoords->x1 - thick + (i - mainmix->scrollpos[j] == 0) * thick < mainprogram->mx and mainprogram->mx < box->scrcoords->x1 + thick) {
							mainprogram->leftmousedown = false;
							float blue[] = {0.5, 0.5, 1.0, 1.0};
							draw_box(blue, blue, box->vtxcoords->x1 - mainprogram->xscrtovtx(thick) + (i - mainmix->scrollpos[j] == 0) * mainprogram->xscrtovtx(thick), box->vtxcoords->y1, mainprogram->xscrtovtx(thick * (2.0f - (i - mainmix->scrollpos[j] == 0))), mainmix->layw, -1);
							float red[] = {1.0, 0.0 , 0.0, 1.0};
							if (lay->pos > 0 and !mainprogram->dragbinel) draw_box(red, red, box->vtxcoords->x1 - mainprogram->xscrtovtx(thick), box->vtxcoords->y1 + box->vtxcoords->h, mainprogram->xscrtovtx(thick * 2.0f), -tf(0.05f), -1);
							if (mainprogram->leftmouse and !mainmix->moving) {
								mainprogram->leftmouse = 0;
								if (lay->pos > 0 and box->scrcoords->y1 - box->scrcoords->h < mainprogram->my and mainprogram->my < box->scrcoords->y1 - box->scrcoords->h + mainprogram->yvtxtoscr(tf(0.05f))) {
									mainmix->delete_layer(lvec, lvec[lay->pos - 1], true);
								}
								else {
									Layer *lay1;
									lay1 = mainmix->add_layer(lvec, lay->pos);
									make_layboxes();
								}
							}
						}
						else if (box->scrcoords->x1 + box->scrcoords->w - thick < mainprogram->mx and mainprogram->mx < box->scrcoords->x1 + box->scrcoords->w) {
							mainprogram->leftmousedown = false;
							if (lay->pos == lvec.size() - 1 or lay->pos == mainmix->scrollpos[j] + 2) {
								float blue[] = {0.5, 0.5 , 1.0, 1.0};
								draw_box(blue, blue, box->vtxcoords->x1 + box->vtxcoords->w - mainprogram->xscrtovtx(thick), box->vtxcoords->y1, mainprogram->xscrtovtx(thick * (1.0f + (i - mainmix->scrollpos[j] != 2))), mainmix->layw, -1);
								float red[] = {1.0, 0.0 , 0.0, 1.0};
								if (!mainprogram->dragbinel) draw_box(red, red, box->vtxcoords->x1 + box->vtxcoords->w - mainprogram->xscrtovtx(thick), box->vtxcoords->y1 + box->vtxcoords->h, mainprogram->xscrtovtx(thick * (1.0f + (i - mainmix->scrollpos[j] != 2))), -tf(0.05f), -1);
								if (mainprogram->leftmouse and !mainmix->moving) {
									mainprogram->leftmouse = 0;
									if ((box->scrcoords->y1 - box->scrcoords->h < mainprogram->my and mainprogram->my < box->scrcoords->y1 - box->scrcoords->h + mainprogram->yvtxtoscr(tf(0.05f)))) {
										mainmix->delete_layer(lvec, lay, true);
									}
									else {
										Layer *lay1 = mainmix->add_layer(lvec, lay->pos + 1);
										make_layboxes();
									}
								}
							}
						}
					}
				}
			}
		}
		
		if (mainprogram->nodesmain->linked and mainmix->currlay) {
			// Handle vidbox
			std::vector<Layer*> &lvec = choose_layers(mainmix->currlay->deck);
			Layer *lay = mainmix->currlay;
			Box *box = lay->node->vidbox;
			if (box->in() and !lay->transforming) {
				draw_box(black, halfwhite, box->vtxcoords->x1 + (box->vtxcoords->w / 2.0f) - 0.015f, box->vtxcoords->y1 + (box->vtxcoords->h / 2.0f) - 0.025f, 0.03f, 0.05f, -1);
				if (box->scrcoords->x1 + (box->scrcoords->w / 2.0f) - mainprogram->xvtxtoscr(0.015f) < mainprogram->mx and mainprogram->mx < box->scrcoords->x1 + (box->scrcoords->w / 2.0f) + mainprogram->xvtxtoscr(0.015f) and box->scrcoords->y1 - (box->scrcoords->h / 2.0f) - mainprogram->yvtxtoscr(0.025f) < mainprogram->my and mainprogram->my < box->scrcoords->y1 - (box->scrcoords->h / 2.0f) + mainprogram->yvtxtoscr(0.025f)) {
					draw_box(black, lightblue, box->vtxcoords->x1 + (box->vtxcoords->w / 2.0f) - 0.015f, box->vtxcoords->y1 + (box->vtxcoords->h / 2.0f) - 0.025f, 0.03f, 0.05f, -1);
					if (mainprogram->leftmousedown) {
						mainprogram->leftmousedown = false;
						lay->transforming = 1;
						lay->transmx = mainprogram->mx - lay->shiftx;
						lay->transmy = mainprogram->my - lay->shifty;
					}
				}
				//if (mainprogram->middlemousedown) {
				//	mainprogram->middlemousedown = false;
				//	lay->transforming = 2;
				//	lay->transmx = mainprogram->mx;
				//	lay->transmy = mainprogram->my;
				//}
				else if (box->in()) {
					// move layer
					if (mainprogram->leftmousedown) {
						if (!lay->vidmoving and !mainmix->moving) {
							binsmain->dragtex = copy_tex(lay->node->vidbox->tex);
							mainprogram->draglay = lay;
							
							mainprogram->dragbinel = new BinElement;
							mainprogram->dragbinel->tex = binsmain->dragtex;
							if (lay->live) {
								mainprogram->dragbinel->path = lay->filename;
								mainprogram->dragbinel->type = ELEM_LIVE;
							}
							else {
								std::string name = remove_extension(basename(mainprogram->draglay->filename));
								int count = 0;
								while (1) {
									mainprogram->dragpath = mainprogram->temppath + "cliptemp_" + name + ".layer";
									if (!exists(mainprogram->dragpath)) {
										mainmix->save_layerfile(mainprogram->dragpath, mainprogram->draglay, 1);
										break;
									}
									count++;
									name = remove_version(name) + "_" + std::to_string(count);
								}
								mainprogram->dragbinel->path = mainprogram->dragpath;
								mainprogram->dragbinel->type = ELEM_LAYER;
								mainprogram->draglay = lay;
							}
	
							mainprogram->leftmousedown = false;
							lay->vidmoving = true;
							mainmix->moving = true;
							binsmain->currbinel = nullptr;
						}
					}
				}
			}	
			if (box->in()) {
				if (mainprogram->mousewheel) {
					lay->scale -= mainprogram->mousewheel * lay->scale / 10.0f;
				}
			}
			
			if (lay->transforming == 1) {
				lay->shiftx = mainprogram->mx - lay->transmx;
				lay->shifty = mainprogram->my - lay->transmy;
				if (mainprogram->leftmouse) {
					lay->transforming = 0;
				}
			}
			//if (lay->transforming == 2) {
			//	lay->scale = 5.0f / (((float)(mainprogram->my - lay->transmy) / 20.0f) + 5.0f);
			//	if (lay->scale > 1.0f) lay->scale *= lay->scale;
			//	lay->scale *= lay->oldscale;
			//	if (mainprogram->middlemouse) {
			//		lay->oldscale = lay->scale;
			//		lay->transforming = 0;
			//	}
			//}
		}
			
			
		// Draw mix view
		if (mainmix->mode == 0) {
			// Handle mixtargets
			for (int i = 0; i < mainprogram->nodesmain->mixnodes.size(); i++) {
				Box *box = mainprogram->nodesmain->mixnodes[i]->outputbox;
				if (box->in()) {
					if (i == 0) {
						render_text("Deck A Monitor", white, box->vtxcoords->x1 + tf(0.01f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.03f), 0.0005f, 0.0008f);
					}
					else if (i == 1) {
						render_text("Deck B Monitor", white, box->vtxcoords->x1 + tf(0.01f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.03f), 0.0005f, 0.0008f);
					}
					if (mainprogram->menuactivation) {
						mainprogram->mixtargetmenu->state = 2;
						mainprogram->mixtargetmenu->value = i;
						mainprogram->menuactivation = false;
					}
				}
			}
			box = mainprogram->nodesmain->mixnodescomp[2]->outputbox;
			box->vtxcoords->x1 = -0.15f;
			box->vtxcoords->y1 = -0.4f + tf(0.05f);
			box->vtxcoords->w = 0.3f;
			box->vtxcoords->h = 0.3f;
			box->upvtxtoscr();
			if (!mainprogram->menuondisplay) {
				if (box->scrcoords->x1 < mainprogram->mx and mainprogram->mx < box->scrcoords->x1 + box->scrcoords->w) {
					if (box->scrcoords->y1 - box->scrcoords->h <= mainprogram->my and mainprogram->my <= box->scrcoords->y1) {
						if (mainprogram->preveff) {
							render_text("Output Mix Monitor", white, box->vtxcoords->x1 + tf(0.01f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.03f), 0.0005f, 0.0008f);
							if (mainprogram->menuactivation) {
								mainprogram->mixtargetmenu->state = 2;
								mainprogram->mixtargetmenu->value = mainprogram->nodesmain->mixnodes.size();
								mainprogram->menuactivation = false;
							}
						}
					}
				}
			}
			
			// Handle wipes
			if (mainprogram->nodesmain->mixnodes[mainmix->currlay->deck]->outputbox->in()) {
				if (mainprogram->leftmousedown) {
					mainmix->currlay->blendnode->wipex = -(((1.0f - ((mainprogram->xscrtovtx(mainprogram->mx) - 0.55f - mainmix->currlay->deck * 0.9f) / 0.3f)) - 0.5f) * 2.0f - 1.5f);
					mainmix->currlay->blendnode->wipey = -((((2.0f - mainprogram->yscrtovtx(mainprogram->my)) / 0.3f) - 0.5f) * 2.0f - 0.50f);
					if (mainmix->currlay->blendnode->wipetype == 8 or mainmix->currlay->blendnode->wipetype == 9) {
						mainmix->currlay->blendnode->wipex *= 16.0f;
						mainmix->currlay->blendnode->wipey *= 16.0f;
					}
				}
			}
			
			Box *box = mainprogram->nodesmain->mixnodes[2]->outputbox;
			if (box->in()) {
				render_text("Preview Mix Monitor", white, box->vtxcoords->x1 + tf(0.01f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.03f), 0.0005f, 0.0008f);
				if (mainprogram->menuactivation) {
					mainprogram->wipemenu->state = 2;
					mainprogram->menuactivation = false;
					mainmix->wipex[0] = -(((1.0f - ((mainprogram->xscrtovtx(mainprogram->mx) - 0.7f) / 0.6f)) - 0.5f) * 2.0f - 0.5f);
					mainmix->wipey[0] = -((((2.0f - mainprogram->yscrtovtx(mainprogram->my)) / 0.6f) - 0.5f) * 2.0f - 0.5f);
					if (mainmix->currlay->blendnode->wipetype == 8 or mainmix->currlay->blendnode->wipetype == 9) {
						mainmix->wipex[0] *= 16.0f;
						mainmix->wipey[0] *= 16.0f;
					}
				}
				if (mainprogram->leftmousedown) {
					mainmix->wipex[0] = -(((1.0f - ((mainprogram->xscrtovtx(mainprogram->mx) - 0.7f) / 0.6f)) - 0.5f) * 2.0f - 0.5f);
					mainmix->wipey[0] = -((((2.0f - mainprogram->yscrtovtx(mainprogram->my)) / 0.6f) - 0.5f) * 2.0f - 0.5f);
					printf("wipex, wipey %f %f\n", mainmix->wipex[0], mainmix->wipey[0]);
					if (mainmix->currlay->blendnode->wipetype == 8 or mainmix->currlay->blendnode->wipetype == 9) {
						mainmix->wipex[0] *= 16.0f;
						mainmix->wipey[0] *= 16.0f;
					}
				}
				if (mainprogram->middlemouse) {
					GLint wipe = glGetUniformLocation(mainprogram->ShaderProgram, "wipe");
					glUniform1i(wipe, 0);
				}
			}
			
			// Handle vidboxes
			Layer *lay = mainmix->currlay;
			Effect *eff;
			
			if (mainmix->currlay) {
				// Handle mixbox
				box = lay->mixbox;
				box->acolor[0] = 0.0;
				box->acolor[1] = 0.0;
				box->acolor[2] = 0.0;
				box->acolor[3] = 1.0;
				if (!mainprogram->menuondisplay) {
					if (box->scrcoords->x1 < mainprogram->mx and mainprogram->mx < box->scrcoords->x1 + box->scrcoords->w) {
						if (box->scrcoords->y1 - box->scrcoords->h + 7.5 <= mainprogram->my and mainprogram->my <= box->scrcoords->y1 - 7.5) {
							if (mainprogram->menuactivation or mainprogram->leftmouse) {
								mainprogram->mixenginemenu->state = 2;
								mainprogram->menux = mainprogram->mx;
								mainprogram->menuy = mainprogram->my;
								mainmix->mousenode = lay->blendnode;
								mainprogram->leftmouse = false;
								mainprogram->menuactivation = false;
							}
							box->acolor[0] = 0.5;
							box->acolor[1] = 0.5;
							box->acolor[2] = 1.0;
							box->acolor[3] = 1.0;
						}
					}
				}
				
				// Handle chromabox
				pick_color(lay, lay->chromabox);
				if (lay->cwon) {
					lay->blendnode->chred = lay->rgb[0];
					lay->blendnode->chgreen = lay->rgb[1];
					lay->blendnode->chblue = lay->rgb[2];
				}
			}
		}
	
	}	

	//deck and mix dragging
	if (binsmain->dragdeck) {
		if (sqrt(pow((mainprogram->mx / (glob->w / 2.0f) - 1.0f) * glob->w / glob->h, 2) + pow((glob->h - mainprogram->my) / (glob->h / 2.0f) - 1.25f, 2)) < 0.2f) {
			mainprogram->binsscreen = false;
		}
		if (!mainprogram->binsscreen) {
			float lightblue[] = {0.5f, 0.5f, 1.0f, 1.0f};
			Box boxA;
			boxA.vtxcoords->x1 = -1.0f + mainmix->numw;
			boxA.vtxcoords->y1 = 1.0f - mainmix->layw;
			boxA.vtxcoords->w = mainmix->layw * 3;
			boxA.vtxcoords->h = mainmix->layw;
			boxA.upvtxtoscr();
			Box boxB;
			boxB.vtxcoords->x1 = mainmix->numw;
			boxB.vtxcoords->y1 = 1.0f - mainmix->layw;
			boxB.vtxcoords->w = mainmix->layw * 3;
			boxB.vtxcoords->h = mainmix->layw;
			boxB.upvtxtoscr();
			if (boxA.in()) {
				draw_box(nullptr, lightblue, -1.0f + mainmix->numw, 1.0f - mainmix->layw, mainmix->layw * 3, mainmix->layw, -1);
				if (mainprogram->leftmouse) {
					mainmix->mousedeck = 0;
					mainmix->open_deck(binsmain->dragdeck->path, 1);
				}
			}
			else if (boxB.in()) {
				draw_box(nullptr, lightblue, mainmix->numw, 1.0f - mainmix->layw, mainmix->layw * 3, mainmix->layw, -1);
				if (mainprogram->leftmouse) {
					mainmix->mousedeck = 1;
					mainmix->open_deck(binsmain->dragdeck->path, 1);
				}
			}
		}
		if (mainprogram->leftmouse or binsmain->movingstruct) {
			binsmain->dragtexes[0].clear();
			binsmain->dragdeck = nullptr;
			mainprogram->leftmouse = false;
		}
		else {
			for (int k = 0; k < binsmain->dragtexes[0].size(); k++) {
				GLint thumb = glGetUniformLocation(mainprogram->ShaderProgram, "thumb");
				glUniform1i(thumb, 1);
				GLfloat vcoords[8];
				float boxwidth = tf(0.06f);
		
				float nmx = mainprogram->xscrtovtx(mainprogram->mx) + (k % 3) * boxwidth;
				float nmy = 2.0 - mainprogram->yscrtovtx(mainprogram->my) - (int)(k / 3) * boxwidth;
				GLfloat *p = vcoords;
				*p++ = -1.0f - 0.5 * boxwidth + nmx;
				*p++ = -1.0f - 0.5 * boxwidth + nmy;
				*p++ = -1.0f + 0.5 * boxwidth + nmx;
				*p++ = -1.0f - 0.5 * boxwidth + nmy;
				*p++ = -1.0f - 0.5 * boxwidth + nmx;
				*p++ = -1.0f + 0.5 * boxwidth + nmy;
				*p++ = -1.0f + 0.5* boxwidth + nmx;
				*p++ = -1.0f + 0.5 * boxwidth + nmy;
				glBindBuffer(GL_ARRAY_BUFFER, thmvbuf);
				glBufferData(GL_ARRAY_BUFFER, 32, vcoords, GL_STREAM_DRAW);
				glBindVertexArray(thmvao);
				glBindTexture(GL_TEXTURE_2D, binsmain->dragtexes[0][k]);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
				glUniform1i(thumb, 0);
			}
		}
	}
	if (binsmain->dragmix) {
		if (mainprogram->leftmouse or binsmain->movingstruct) {
			if (sqrt(pow((mainprogram->mx / (glob->w / 2.0f) - 1.0f) * glob->w / glob->h, 2) + pow((glob->h - mainprogram->my) / (glob->h / 2.0f) - 1.25f, 2)) < 0.2f) {
				mainprogram->binsscreen = false;
				mainmix->open_mix(binsmain->dragmix->path.c_str());
				binsmain->dragmix = nullptr;
			}
			for (int m = 0; m < 2; m++) {
				binsmain->dragtexes[m].clear();
			}
			binsmain->dragmix = nullptr;
			mainprogram->leftmouse = false;
		}
		else {
			for (int m = 0; m < 2; m++) {
				for (int k = 0; k < binsmain->dragtexes[m].size(); k++) {
					GLint thumb = glGetUniformLocation(mainprogram->ShaderProgram, "thumb");
					glUniform1i(thumb, 1);
					GLfloat vcoords[8];
					float boxwidth = tf(0.06f);
			
					float nmx = mainprogram->xscrtovtx(mainprogram->mx) + (k % 3) * boxwidth + m * boxwidth * 3;
					float nmy = 2.0 - mainprogram->yscrtovtx(mainprogram->my) - (int)(k / 3) * boxwidth;
					GLfloat *p = vcoords;
					*p++ = -1.0f - 0.5 * boxwidth + nmx;
					*p++ = -1.0f - 0.5 * boxwidth + nmy;
					*p++ = -1.0f + 0.5 * boxwidth + nmx;
					*p++ = -1.0f - 0.5 * boxwidth + nmy;
					*p++ = -1.0f - 0.5 * boxwidth + nmx;
					*p++ = -1.0f + 0.5 * boxwidth + nmy;
					*p++ = -1.0f + 0.5* boxwidth + nmx;
					*p++ = -1.0f + 0.5 * boxwidth + nmy;
					glBindBuffer(GL_ARRAY_BUFFER, thmvbuf);
					glBufferData(GL_ARRAY_BUFFER, 32, vcoords, GL_STREAM_DRAW);
					glBindVertexArray(thmvao);
					glBindTexture(GL_TEXTURE_2D, binsmain->dragtexes[m][k]);
					glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
					glUniform1i(thumb, 0);
				}
			}
		}
	}
		
	if (mainprogram->dragbinel) {
		//dragging something inside wormhole
		if (sqrt(pow((mainprogram->mx / (glob->w / 2.0f) - 1.0f) * glob->w / glob->h, 2) + pow((glob->h - mainprogram->my) / (glob->h / 2.0f) - 1.25f, 2)) < 0.2f) {
			if (!mainprogram->inwormhole and !mainprogram->menuondisplay) {
				if (!mainprogram->binsscreen) {
					set_queueing(mainmix->currlay, false);
				}
				mainprogram->binsscreen = !mainprogram->binsscreen;
				mainprogram->inwormhole = true;
			}
		}
		else {
			mainprogram->inwormhole = false;
		}
	}
	
	// layer dragging
	if (mainprogram->nodesmain->linked and mainmix->currlay) {
		std::vector<Layer*> &lvec = choose_layers(mainmix->currlay->deck);
		Layer *lay = mainmix->currlay;
		Box *box = lay->node->vidbox;
		if (lay->vidmoving) {
			if (!mainprogram->binsscreen) {
				for (int i = 0; i < 2; i++) {
					std::vector<Layer*> &lays = choose_layers(i);
					for (int j = 0; j < lays.size(); j++) {
						if (lays[j]->node->vidbox->in()) {
							if (mainprogram->draglay != lays[j]) {
								set_queueing(lays[j], true);
								break;
							}
						}
					}
				}
			}				
			
			if (mainprogram->binsscreen) {

				GLint thumb = glGetUniformLocation(mainprogram->ShaderProgram, "thumb");
				glUniform1i(thumb, 1);
				GLfloat vcoords[8];
				float boxwidth = tf(0.2f);
		
				float nmx = mainprogram->xscrtovtx(mainprogram->mx) + boxwidth / 2.0f;
				float nmy = 2.0 - mainprogram->yscrtovtx(mainprogram->my) - boxwidth / 2.0f;
				GLfloat *p = vcoords;
				*p++ = -1.0f - 0.5 * boxwidth + nmx;
				*p++ = -1.0f - 0.5 * boxwidth + nmy;
				*p++ = -1.0f + 0.5 * boxwidth + nmx;
				*p++ = -1.0f - 0.5 * boxwidth + nmy;
				*p++ = -1.0f - 0.5 * boxwidth + nmx;
				*p++ = -1.0f + 0.5 * boxwidth + nmy;
				*p++ = -1.0f + 0.5* boxwidth + nmx;
				*p++ = -1.0f + 0.5 * boxwidth + nmy;
				glBindBuffer(GL_ARRAY_BUFFER, thmvbuf);
				glBufferData(GL_ARRAY_BUFFER, 32, vcoords, GL_STREAM_DRAW);
				glBindVertexArray(thmvao);
				glBindTexture(GL_TEXTURE_2D, mainprogram->dragbinel->tex);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
				glUniform1i(thumb, 0);
			}
				
			if (mainprogram->leftmouse) {
				mainprogram->leftmouse = false;
				enddrag();
				if (!mainprogram->binsscreen) {
					if (mainprogram->preveff) {
						exchange(lay, lvec, mainmix->layersA, 0);
						exchange(lay, lvec, mainmix->layersB, 1);
					}
					else {
						exchange(lay, lvec, mainmix->layersAcomp, 0);
						exchange(lay, lvec, mainmix->layersBcomp, 1);
					}
				}
				set_queueing(lay, false);
			}				
		}
	}
				
				
	if (mainprogram->menuactivation == true) {
		mainprogram->genericmenu->state = 2;
		mainprogram->menuactivation = false;
	}
	
	if (mainprogram->leftmouse) {
		mainprogram->menuondisplay = false;
		for (int i = 0; i < mainprogram->menulist.size(); i++) {
			mainprogram->menulist[i]->state = 0;
		}
	}
	
	if (mainprogram->dragbinel) {
		GLint thumb = glGetUniformLocation(mainprogram->ShaderProgram, "thumb");
		glUniform1i(thumb, 1);
		GLfloat vcoords[8];
		float boxwidth = tf(0.2f);

		float nmx = mainprogram->xscrtovtx(mainprogram->mx) + boxwidth / 2.0f;
		float nmy = 2.0 - mainprogram->yscrtovtx(mainprogram->my) - boxwidth / 2.0f;
		GLfloat *p = vcoords;
		*p++ = -1.0f - 0.5 * boxwidth + nmx;
		*p++ = -1.0f - 0.5 * boxwidth + nmy;
		*p++ = -1.0f + 0.5 * boxwidth + nmx;
		*p++ = -1.0f - 0.5 * boxwidth + nmy;
		*p++ = -1.0f - 0.5 * boxwidth + nmx;
		*p++ = -1.0f + 0.5 * boxwidth + nmy;
		*p++ = -1.0f + 0.5* boxwidth + nmx;
		*p++ = -1.0f + 0.5 * boxwidth + nmy;
		glBindBuffer(GL_ARRAY_BUFFER, thmvbuf);
		glBufferData(GL_ARRAY_BUFFER, 32, vcoords, GL_STREAM_DRAW);
		glBindVertexArray(thmvao);
		glBindTexture(GL_TEXTURE_2D, mainprogram->dragbinel->tex);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glUniform1i(thumb, 0);
	}
	
			
	// draw close window icon?
	float deepred[4] = {1.0, 0.0, 0.0, 1.0};
	if (mainprogram->mx == glob->w - 1 and mainprogram->my == 0) {
		draw_box(NULL, deepred, 0.95f, 1.0f - 0.05f * glob->w / glob->h, 0.05f, 0.05f * glob->w / glob->h, -1);
		render_text("x", white, 0.962f, 1.015f - 0.05f * glob->w / glob->h, 0.0012f, 0.002f);
		if (mainprogram->leftmouse)	mainprogram->quit("closed window");
	}
	
		
	mainprogram->leftmouse = 0;
	mainprogram->middlemouse = 0;
	mainprogram->rightmouse = 0;
	
	// sync with output views
	for (int i = 0; i < mainprogram->outputentries.size(); i++) {
		EWindow *win = mainprogram->outputentries[i]->win;
		win->syncnow = true;
		win->sync.notify_one();
	}
	
	glFlush();
	bool prret = false;
	GLuint tex, fbo;
	if (mainprogram->prefon) {
		SDL_GL_MakeCurrent(mainprogram->prefwindow, glc_pr);
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, smw, smh, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->smglobfbo_pr);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		prret = preferences();
		glBlitNamedFramebuffer(mainprogram->smglobfbo_pr, fbo, 0, 0, smw, smh , 0, 0, smw, smh, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		glFlush();
	}
	if (mainprogram->tunemidi) {
		if (mainprogram->waitmidi){
			clock_t t = clock() - mainprogram->stt;
			double time_taken = ((double)t) / CLOCKS_PER_SEC; // in seconds
			if (time_taken > 0.1f) {
				mainprogram->waitmidi = 2;
				mycallback(0.0f, &mainprogram->savedmessage, mainprogram->savedmidiitem);
				mainprogram->waitmidi = 0;
			}
		}
		SDL_GL_MakeCurrent(mainprogram->tunemidiwindow, glc_tm);
		glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->smglobfbo_tm);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		bool ret = tune_midi();
		glFlush();
		if (ret) {
			glBlitNamedFramebuffer(mainprogram->smglobfbo_tm, 0, 0, 0, smw, smh , 0, 0, smw, smh, GL_COLOR_BUFFER_BIT, GL_NEAREST);
			SDL_GL_SwapWindow(mainprogram->tunemidiwindow);
		}
	}
	if (prret) {
		SDL_GL_MakeCurrent(mainprogram->prefwindow, glc_pr);
		glBlitNamedFramebuffer(fbo, 0, 0, 0, smw, smh , 0, 0, smw, smh, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		SDL_GL_SwapWindow(mainprogram->prefwindow);
	}
	SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
	glBlitNamedFramebuffer(mainprogram->globfbo, 0, 0, 0, glob->w, glob->h , 0, 0, glob->w, glob->h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	SDL_GL_SwapWindow(mainprogram->mainwindow);
	glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
}


void delete_layers(std::vector<Layer*> &layers, bool alive) {
	while (!layers.empty()) {
		//layers.back()->node = nullptr;
		//layers.back()->blendnode = nullptr;
		Layer *lay = layers.back();
		if (alive and std::find(mainprogram->busylayers.begin(), mainprogram->busylayers.end(), lay) == mainprogram->busylayers.end()) {
			mainmix->delete_layer(layers, lay, false);
		}
		else {
			int size = layers.size();
			for (int i = 0; i < size; i++) {
				if (layers[i] == lay) {
					layers.erase(layers.begin() + i);
				}
			}
			while (!lay->effects.empty()) {
				mainprogram->nodesmain->currpage->delete_node(lay->effects.back()->node);
				for (int j = 0; j < lay->effects.back()->params.size(); j++) {
					delete lay->effects.back()->params[j];
				}
				delete lay->effects.back();
				lay->effects.pop_back();
			}
			mainprogram->nodesmain->currpage->delete_node(lay->node);
		}
	}
}

void new_shelf() {
	for (int i = 0; i < 32; i++) {
		thpath[i] = "";
		thtype[i] = ELEM_FILE;
		glBindTexture(GL_TEXTURE_2D, thumbtex[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	}
}

void new_state() {
	bool save = mainprogram->preveff;
	mainprogram->preveff = true;
	new_file(2, 1);
	mainprogram->preveff = false;
	new_file(2, 1);
	//clear genmidis?
	new_shelf();
	mainprogram->preveff = save;
}

void new_file(int decks, bool alive) {
	// kill mixnodes
	//mainprogram->nodesmain->mixnodes.clear();
	//mainprogram->nodesmain->mixnodescomp.clear();
	
	// reset layers
	bool currdeck = 0;
	if (mainmix->currlay) currdeck = mainmix->currlay->deck;
	if (decks == 0 or decks == 2) {
		std::vector<Layer*> &lvec = choose_layers(0);
		delete_layers(lvec, alive);
		Layer *lay = mainmix->add_layer(lvec, 0);
		if (mainprogram->preveff) {
			mainprogram->nodesmain->currpage->connect_nodes(lvec[lvec.size() - 1]->lasteffnode, mainprogram->nodesmain->mixnodes[0]);
		}
		else {
			mainprogram->nodesmain->currpage->connect_nodes(lvec[lvec.size() - 1]->lasteffnode, mainprogram->nodesmain->mixnodescomp[0]);
		}
		if (currdeck == 0) mainmix->currlay = lay;
	}
	if (decks == 1 or decks == 2) {
		std::vector<Layer*> &lvec = choose_layers(1);
		delete_layers(lvec, alive);
		Layer *lay = mainmix->add_layer(lvec, 0);
		if (mainprogram->preveff) {
			mainprogram->nodesmain->currpage->connect_nodes(lvec[lvec.size() - 1]->lasteffnode, mainprogram->nodesmain->mixnodes[1]);
		}
		else {
			mainprogram->nodesmain->currpage->connect_nodes(lvec[lvec.size() - 1]->lasteffnode, mainprogram->nodesmain->mixnodescomp[1]);
		}
		if (currdeck == 1) mainmix->currlay = lay;
	}
		
	// set comp layers
	//mainmix->copy_to_comp(mainmix->layersA, mainmix->layersAcomp, mainmix->layersB, mainmix->layersBcomp, mainprogram->nodesmain->currpage->nodes, mainprogram->nodesmain->currpage->nodescomp, mainprogram->nodesmain->mixnodescomp, true);
}


GLuint copy_tex(GLuint tex) {
	return copy_tex(tex, (int)(glob->w * 0.3f), (int)(glob->h * 0.3f), 0);
}

GLuint copy_tex(GLuint tex, bool yflip) {
	return copy_tex(tex, (int)(glob->w * 0.3f), (int)(glob->h * 0.3f), yflip);
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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tw, th, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	GLuint dfbo;
	glGenFramebuffers(1, &dfbo);
	glBindFramebuffer(GL_FRAMEBUFFER, dfbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, smalltex, 0);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	int sw, sh;
	glBindTexture(GL_TEXTURE_2D, tex);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &sw);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &sh);
	float black[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	float facx = (float)tw / glob->w;
	float facy = (float)th / glob->h;
	if (yflip) {
		draw_box(NULL, black, -1.0f, -1.0f, 2.0f * facx, 2.0f * facy, 0.0f, 0.0f, 1.0f, 1.0f, 0, tex, glob->w, glob->h);
	}
	else {
		draw_box(NULL, black, -1.0f, -1.0f + 2.0f * facy, 2.0f * facx, -2.0f * facy, 0.0f, 0.0f, 1.0f, 1.0f, 0, tex, glob->w, glob->h);
	}
	glDeleteFramebuffers(1, &dfbo);
	return smalltex;
}

void save_thumb(std::string path, GLuint tex) {
	int wi = glob->w * 0.3f;
	int he = glob->h * 0.3f;
	char *buf = (char*)malloc(wi * he * 3);

	GLuint smalltex = copy_tex(tex);
	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, smalltex, 0);
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
		printf("Error opening jpeg file %s\n!", path );
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

long getFileSize(std::string filename)
{
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
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
		ifstream fileInput;
		fileInput.open(paths[i], ios::in | ios::binary);
		int fileSize = getFileSize(paths[i]);
		ofile.write((const char *)&fileSize, 4);
		fileInput.close();
	}
	for (int i = 0; i < paths.size(); i++) {
		ifstream fileInput;
		fileInput.open(paths[i], ios::in | ios::binary);
		int fileSize = getFileSize(paths[i]);
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
		int num;
		bfile.read((char *)&num, 4);
		std::vector<int> sizes;
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

	
void save_shelf(const std::string &path, int deck) {
	std::string ext = path.substr(path.length() - 6, std::string::npos);
	std::string str;
	if (ext != ".shelf") str = path + ".shelf";
	else str = path;
	
	std::vector<std::string> filestoadd;
	ofstream wfile;
	wfile.open(path);
	wfile << "EWOCvj SHELFFILE V0.1\n";
	
	wfile << "ELEMS\n";
	for (int i = 16 * (deck == 1); i < 16 + 16 * (deck == 1 or deck == 2); i++) {
		wfile << thpath[i];
		wfile << "\n";
		wfile << std::to_string(thtype[i]);
		wfile << "\n";
		if (thtype[i] == ELEM_LAYER) filestoadd.push_back(thpath[i]);
	}
	wfile << "ENDOFELEMS\n";
	
	wfile << "ENDOFFILE\n";
	wfile.close();
	
    ofstream outputfile;
	outputfile.open(mainprogram->temppath + "tempconcatshelf", ios::out | ios::binary);
	std::vector<std::vector<std::string>> filestoadd2;
	filestoadd2.push_back(filestoadd);
	concat_files(outputfile, path, filestoadd2);
	outputfile.close();
	boost::filesystem::rename(mainprogram->temppath + "tempconcatshelf", path);
}

void open_shelfdir() {
	struct dirent *ent;
	if ((ent = readdir(mainprogram->opendir)) != NULL and mainprogram->shelfdircount < 16) {
		char *filepath = (char*)malloc(1024);
		strcpy(filepath, mainprogram->shelfpath.c_str());
		strcat(filepath, "/");
		strcat(filepath, ent->d_name);
		std::string str(filepath);
		
		bool ret;
		if (str.substr(str.length() - 6, std::string::npos) == ".layer") {
			ret = open_shelflayer(str, mainprogram->shelfdircount + mainmix->mousedeck * 16);
		}
		else {
			ret = open_shelfvideo(str, mainprogram->shelfdircount + mainmix->mousedeck * 16);
		}
		
		if (ret) mainprogram->shelfdircount++;
	}
	else mainprogram->openshelfdir = false;
}

bool open_shelfvideo(const std::string &path, int pos) {
	thpath[pos] = path;
	thtype[pos] = ELEM_FILE;
	Layer *lay = new Layer(true);
	lay->dummy = true;
	open_video(1, lay, path, true);
	lay->frame = lay->numf / 2.0f;
	lay->prevframe = -1;
	lay->ready = true;
	lay->startdecode.notify_one();
	std::unique_lock<std::mutex> lock(lay->endlock);
	lay->enddecode.wait(lock, [&]{return lay->processed;});
	lay->processed = false;
	lock.unlock();
	if (lay->openerr) {
		thpath[pos] = "";
		lay->closethread = true;
		while (lay->closethread) {
			lay->ready = true;
			lay->startdecode.notify_one();
		}
		delete lay;
		return 0;
	}
	glBindTexture(GL_TEXTURE_2D, thumbtex[pos]);
	if (lay->dataformat == 188 or lay->vidformat == 187) {
		if (lay->decresult->compression == 187) {
			glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, lay->decresult->width, lay->decresult->height, 0, lay->decresult->size, lay->decresult->data);
		}
		else if (lay->decresult->compression == 190) {
			glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, lay->decresult->width, lay->decresult->height, 0, lay->decresult->size, lay->decresult->data);
		}
	}
	else { 
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, lay->decresult->width, lay->decresult->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, lay->decresult->data);
	}
	lay->closethread = true;
	while (lay->closethread) {
		lay->ready = true;
		lay->startdecode.notify_one();
	}
	delete lay;
	
	return 1;
}
	
bool open_shelflayer(const std::string &path, int pos) {
	thpath[pos] = path;
	thtype[pos] = ELEM_LAYER;
	Layer *lay = new Layer(true);
	lay->dummy = true;
	lay->pos = 0;
	lay->node = mainprogram->nodesmain->currpage->add_videonode(2);
	lay->node->layer = lay;
	lay->lasteffnode = lay->node;
	lay->node->calc = true;
	lay->node->walked = false;
	mainmix->open_layerfile(path, lay, false, 0);
	if (lay->openerr) {
		thpath[pos] = "";
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
	for (int k = 0; k < lay->effects.size(); k++) {
		lay->effects[k]->node->calc = true;
		lay->effects[k]->node->walked = false;
		lay->lasteffnode = lay->effects[k]->node;
	}
	lay->frame = 0.0f;
	lay->prevframe = -1;
	lay->ready = true;
	lay->startdecode.notify_one();
	std::unique_lock<std::mutex> lock(lay->endlock);
	lay->enddecode.wait(lock, [&]{return lay->processed;});
	lay->processed = false;
	lock.unlock();
	glBindTexture(GL_TEXTURE_2D, lay->fbotex);
	if (lay->dataformat == 188 or lay->vidformat == 187) {
		if (lay->decresult->compression == 187) {
			glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, lay->decresult->width, lay->decresult->height, 0, lay->decresult->size, lay->decresult->data);
		}
		else if (lay->decresult->compression == 190) {
			glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, lay->decresult->width, lay->decresult->height, 0, lay->decresult->size, lay->decresult->data);
		}
	}
	else {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, lay->decresult->width, lay->decresult->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, lay->decresult->data);
	}
	onestepfrom(0, lay->node, NULL, -1, -1);
	if (lay->effects.size()) {
		thumbtex[pos] = copy_tex(lay->effects[lay->effects.size() - 1]->fbotex, 1);
	}
	else {
		thumbtex[pos] = copy_tex(lay->fbotex, 1);
	}
	lay->closethread = true;
	while (lay->closethread) {
		lay->ready = true;
		lay->startdecode.notify_one();
	}
	delete lay;
	
	return 1;
}

void open_shelf(const std::string &path, int deck) {
	
	#ifdef _WIN64
	if (path != "./shelves/shelfs.shelf") mainprogram->mainshelf = false;
	else mainprogram->mainshelf = true;
	# else
	#ifdef __linux__
	std::string homedir (getenv("HOME"));
	if (path != homedir + "/.ewocvj2/shelves/shelfs.shelf") mainprogram->mainshelf = false;
	else mainprogram->mainshelf = true;
	#endif
	#endif
	
	std::string result = deconcat_files(path);
	bool concat = (result != "");
	ifstream rfile;
	if (concat) rfile.open(result);
	else rfile.open(path);
	
	int filecount = 0;
	std::string istring;
	getline(rfile, istring);
	while (getline(rfile, istring)) {
		if (istring == "ENDOFFILE") {
			break;
		}
		else if (istring == "ELEMS") {
			int count = (deck == 1) * 16;
			while (getline(rfile, istring)) {
				if (istring == "ENDOFELEMS") break;
				thpath[count] = istring;
				getline(rfile, istring);
				thtype[count] = (ELEM_TYPE)std::stoi(istring);
				if (thtype[count] == ELEM_LAYER) {
					if (concat) {
						boost::filesystem::rename(result + "_" + std::to_string(filecount) + ".file", thpath[count]);
						filecount++;
					}
				}
				if (thpath[count] == "") {
					glBindTexture(GL_TEXTURE_2D, thumbtex[count]);
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
					count++;
					continue;
				}
				if (thtype[count] == ELEM_FILE) {
					open_shelfvideo(thpath[count], count);
				}
				else if (thtype[count] == ELEM_LAYER) {
					open_shelflayer(thpath[count], count);
				}
				count++;
			}
		}
	}

	rfile.close();
}

void new_shelf(bool deck) {
	for (int i = 16 * deck; i < 16 + 16 * deck; i++) {
		thpath[i] = "";
		thtype[i] == ELEM_FILE;
		glBindTexture(GL_TEXTURE_2D, thumbtex[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
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

	wfile << "ENDOFFILE\n";
	wfile.close();
}

void open_genmidis(std::string path) {
	ifstream rfile;
	rfile.open(path);
	std::string istring;

	LayMidi *lm;
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
	}
	rfile.close();
}

BinElement *find_element(int size, int k, int i, int j, bool overlapchk) {
	if (overlapchk) {
		std::vector<int> rows;
		for (int m = 0; m < 24; m++) {
			rows.push_back(0);
		}
		for (int m = 0; m < binsmain->currbin->decks.size(); m++) {
			BinDeck *bd = binsmain->currbin->decks[m];
			if (bd == binsmain->movingdeck) continue;
			for (int n = 0; n < bd->height; n++) {
				if (bd->i == 0) rows[bd->j + n] += 1;
				if (bd->i == 3) rows[bd->j + n] += 2;
			}
		}
		for (int m = 0; m < binsmain->currbin->mixes.size(); m++) {
			BinMix *bm = binsmain->currbin->mixes[m];
			if (bm == binsmain->movingmix) continue;
			for (int n = 0; n < bm->height; n++) {
				rows[bm->j + n] = 3;
			}
		}
		int pos1 = j;
		int pos2 = j;
		int newi = i;
		j = -1;
		while (pos1 >= 0 or pos2 < 24) {
			bool found1 = true;
			for (int m = 0; m <= (int)((size - 1) / 3); m++) {
				if (rows[pos1 + m] == 1) {
					if (binsmain->inserting == 2) {
						found1 = false;
						break;
					}
					else newi = 3;
				}
				else if (rows[pos1 + m] == 2) {
					if (binsmain->inserting == 2) {
						found1 = false;
						break;
					}
					else newi = 0;
				}
				else if (rows[pos1 + m] == 3) {
					found1 = false;
					break;
				}
			}
			if (found1) {
				i = newi;
				j = pos1;
				break;
			}
			bool found2 = true;
			for (int m = 0; m <= (int)((size - 1) / 3); m++) {
				if (rows[pos2 + m] == 1) {
					if (binsmain->inserting == 2) {
						found2 = false;
						break;
					}
					else newi = 3;
				}
				else if (rows[pos2 + m] == 2) {
					if (binsmain->inserting == 2) {
						found2 = false;
						break;
					}
					else newi = 0;
				}
				else if (rows[pos2 + m] == 3) {
					found2 = false;
					break;
				}
			}
			if (found2) {
				i = newi;
				j = pos2;
				break;
			}
			pos1--;
			pos2++;
		}
		if (j == -1) return nullptr;
	}
	
	int kk = ((int)(k / 3) * 6) + (k % 3);
	int move = (j * 6 + i + 144) % 3;
	int fronti = (int)(i / 3) * 3 + ((144 + i - move) % 3);
	int frontj = (int)((j * 6 + i - move) / 6);
	int intm = (144 - (fronti + frontj * 6) - (((int)((size - 1) / 3) * 6) + 3));
	intm = (intm < 0) * intm;
	int jj = frontj + (int)((kk + fronti + intm) / 6) - ((kk + fronti + intm) < 0);
	int ii = ((kk + intm + 144) % 6 + fronti + 144) % 6;
	
	return binsmain->currbin->elements[ii * 24 + jj];
}



int main(int argc, char* argv[]){
	bool quit = false;

	#ifdef _WIN64
   	HRESULT hr = SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
    if (FAILED(hr))
    {
        _com_error err(hr);
        fwprintf(stderr, L"SetProcessDpiAwareness: %s\n", err.ErrorMessage());
    }
    #endif
	
    
    // OPENAL
	const char *defaultDeviceName = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
	ALCdevice *device = alcOpenDevice(defaultDeviceName);
	ALCcontext *context = alcCreateContext(device, NULL);
	bool succes = alcMakeContextCurrent(context);
	
	if (SDL_Init(SDL_INIT_VIDEO) < 0) /* Initialize SDL's Video subsystem */
		mainprogram->quit("Unable to initialize SDL"); /* Or die on error */

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

 	lp = new LoopStation;
 	lpc = new LoopStation;
 	loopstation = lp;
 	mainmix = new Mixer;
	mainprogram = new Program;	
 	mainprogram->mainwindow = win;
 	binsmain = new BinsMain;

	glc = SDL_GL_CreateContext(mainprogram->mainwindow);

	//glewExperimental = GL_TRUE;
	glewInit();

	mainprogram->tunemidiwindow = SDL_CreateWindow("Tune MIDI", glob->w / 4, glob->h / 4, glob->w / 2, glob->h / 2, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN | SDL_WINDOW_ALLOW_HIGHDPI);
	mainprogram->prefwindow = SDL_CreateWindow("Preferences", glob->w / 4, glob->h / 4, glob->w / 2, glob->h / 2, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN | SDL_WINDOW_ALLOW_HIGHDPI);
	SDL_GL_GetDrawableSize(mainprogram->prefwindow, &wi, &he);
	smw = (float)wi;
	smh = (float)he;
	
	glc_tm = SDL_GL_CreateContext(mainprogram->tunemidiwindow);
	SDL_GL_MakeCurrent(mainprogram->tunemidiwindow, glc_tm);
	mainprogram->ShaderProgram_tm = mainprogram->set_shader();
	glGenTextures(1, &mainprogram->smglobfbotex_tm);
	glBindTexture(GL_TEXTURE_2D, mainprogram->smglobfbotex_tm);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, smw, smh, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glGenFramebuffers(1, &mainprogram->smglobfbo_tm);
	glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->smglobfbo_tm);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mainprogram->smglobfbotex_tm, 0);
	glc_pr = SDL_GL_CreateContext(mainprogram->prefwindow);
	SDL_GL_MakeCurrent(mainprogram->prefwindow, glc_pr);
	mainprogram->ShaderProgram_pr = mainprogram->set_shader();
	glGenTextures(1, &mainprogram->smglobfbotex_pr);
	glBindTexture(GL_TEXTURE_2D, mainprogram->smglobfbotex_pr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, smw, smh, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glGenFramebuffers(1, &mainprogram->smglobfbo_pr);
	glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->smglobfbo_pr);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mainprogram->smglobfbotex_pr, 0);
	SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
	mainprogram->ShaderProgram = mainprogram->set_shader();
	glUseProgram(mainprogram->ShaderProgram);
	
	glGenTextures(1, &mainprogram->globfbotex);
	glBindTexture(GL_TEXTURE_2D, mainprogram->globfbotex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, glob->w, glob->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glGenFramebuffers(1, &mainprogram->globfbo);
	glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mainprogram->globfbotex, 0);

	mainprogram->cwbox->vtxcoords->w = glob->w / 5.0f;
	mainprogram->cwbox->vtxcoords->h = glob->h / 5.0f;
	mainprogram->cwbox->upvtxtoscr();


	FT_Library ft;
	if(FT_Init_FreeType(&ft)) {
	  fprintf(stderr, "Could not init freetype library\n");
	  return 1;
	}
  	FT_UInt interpreter_version = 40;
	FT_Property_Set(ft, "truetype", "interpreter-version", &interpreter_version);
	#ifdef _WIN64
	std::string fstr = "./expressway.ttf";
	if (!exists(fstr)) mainprogram->quit("Can't find \"expressway.ttf\" TrueType font in current directory");
	#else
	#ifdef __linux__
	std::string fdir (FONTDIR);
	std::string fstr = fdir + "/expressway.ttf";
	if (!exists(fdir + "/expressway.ttf"))  mainprogram->quit("Can't find \"expressway.ttf\" TrueType font in " + fdir);
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
 	effects.push_back("Delete effect");
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
  	mainprogram->make_menu("effectmenu", mainprogram->effectmenu, effects);
 	for (int i = 1; i < effects.size(); i++) {
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
 	mixmodes.push_back("CHROMAKEY");
 	mixmodes.push_back("DISPLACEMENT");
  	mainprogram->make_menu("mixmodemenu", mainprogram->mixmodemenu, mixmodes);
 	
	std::vector<std::string> parammodes1;
	parammodes1.push_back("MIDI Learn");
 	mainprogram->make_menu("parammenu1", mainprogram->parammenu1, parammodes1);
 	
	std::vector<std::string> parammodes2;
	parammodes2.push_back("MIDI Learn");
	parammodes2.push_back("Remove automation");
 	mainprogram->make_menu("parammenu2", mainprogram->parammenu2, parammodes2);
 	
	std::vector<std::string> mixtargets;
 	mainprogram->make_menu("mixtargetmenu", mainprogram->mixtargetmenu, mixtargets);
 	
	std::vector<std::string> livesources;
 	mainprogram->make_menu("livemenu", mainprogram->livemenu, livesources);
 	
 	std::vector<std::string> loopops;
 	loopops.push_back("Set loop start to current frame");
 	loopops.push_back("Set loop end to current frame");
 	mainprogram->make_menu("loopmenu", mainprogram->loopmenu, loopops);
 	
 	std::vector<std::string> deckops;
 	deckops.push_back("Open deck");
	deckops.push_back("Save deck");
 	mainprogram->make_menu("deckmenu", mainprogram->deckmenu, deckops);

 	std::vector<std::string> layops;
 	layops.push_back("submenu livemenu");
 	layops.push_back("Connect live");
 	layops.push_back("Open video");
	layops.push_back("Open layer");
	layops.push_back("Save layer");
 	layops.push_back("New deck");
	layops.push_back("Open deck");
	layops.push_back("Save deck");
 	layops.push_back("New mix");
 	layops.push_back("Open mix");
	layops.push_back("Save mix");
  	layops.push_back("Delete layer");
  	layops.push_back("Clone layer");
  	layops.push_back("Center video");
 	mainprogram->make_menu("laymenu", mainprogram->laymenu, layops);

 	std::vector<std::string> loadops;
 	loadops.push_back("submenu livemenu");
 	loadops.push_back("Connect live");
 	loadops.push_back("Open video");
	loadops.push_back("Open layer");
 	loadops.push_back("New deck");
	loadops.push_back("Open deck");
	loadops.push_back("Save deck");
 	loadops.push_back("New mix");
 	loadops.push_back("Open mix");
	loadops.push_back("Save mix");
 	mainprogram->make_menu("loadmenu", mainprogram->loadmenu, loadops);

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
  	binel.push_back("Insert file(s) from disk");
  	binel.push_back("Insert dir from disk");
  	binel.push_back("Insert deck A");
  	binel.push_back("Insert deck B");
  	binel.push_back("Insert full mix");
  	mainprogram->make_menu("binelmenu", mainprogram->binelmenu, binel);
  	
 	std::vector<std::string> bin;
  	bin.push_back("Delete bin");
  	bin.push_back("Rename bin");
  	mainprogram->make_menu("binmenu", mainprogram->binmenu, bin);

  	std::vector<std::string> bin2;
  	bin2.push_back("Rename bin");
  	mainprogram->make_menu("bin2menu", mainprogram->bin2menu, bin2);

 	std::vector<std::string> genmidi;
  	genmidi.push_back("Tune deck MIDI");
  	mainprogram->make_menu("genmidimenu", mainprogram->genmidimenu, genmidi);

 	std::vector<std::string> generic;
  	generic.push_back("New state");
  	generic.push_back("Open state");
  	generic.push_back("Save state");
  	generic.push_back("Preferences");
  	generic.push_back("Quit");
  	mainprogram->make_menu("genericmenu", mainprogram->genericmenu, generic);

 	std::vector<std::string> shelf1;
  	shelf1.push_back("New shelf");
  	shelf1.push_back("Open shelf");
  	shelf1.push_back("Save shelf");
  	shelf1.push_back("Open video");
  	shelf1.push_back("Open layerfile");
  	shelf1.push_back("Open dir");
  	mainprogram->make_menu("shelfmenu1", mainprogram->shelfmenu1, shelf1);

 	std::vector<std::string> shelf2;
  	shelf2.push_back("New shelf");
  	shelf2.push_back("Open shelf");
  	shelf2.push_back("Save shelf");
  	shelf2.push_back("Open video");
  	shelf2.push_back("Open layerfile");
  	shelf2.push_back("Open dir");
  	shelf2.push_back("Back to main shelf");
  	mainprogram->make_menu("shelfmenu2", mainprogram->shelfmenu2, shelf2);

 	
	mainprogram->nodesmain = new NodesMain;
	mainprogram->nodesmain->add_nodepages(8);
	mainprogram->nodesmain->currpage = mainprogram->nodesmain->pages[0];
	mainprogram->toscreen->box->upvtxtoscr();
	mainprogram->backtopre->box->upvtxtoscr();
	mainprogram->vidprev->box->upvtxtoscr();
	mainprogram->effprev->box->upvtxtoscr();
	
	
	mainprogram->prefs = new Preferences;
	mainprogram->prefs->load();
	#ifdef _WIN64
	boost::filesystem::path p1{"./bins"};
	if (!exists("./bins")) boost::filesystem::create_directory(p1);
	boost::filesystem::path p2{"./recordings"};
	if (!exists("./recordings")) boost::filesystem::create_directory(p2);
	boost::filesystem::path p3{"./shelves"};
	if (!exists("./shelves")) boost::filesystem::create_directory(p3);
	boost::filesystem::path p4{"./temp"};
	if (!exists("./temp")) boost::filesystem::create_directory(p4);
	#else
	#ifdef __linux__
	std::string homedir (getenv("HOME"));
	boost::filesystem::path e{homedir + "/.ewocvj2"};
	if (!exists(homedir + "/.ewocvj2")) boost::filesystem::create_directory(e);
	boost::filesystem::path p1{homedir + "/.ewocvj2/bins"};
	if (!exists(homedir + "/.ewocvj2/bins")) boost::filesystem::create_directory(p1);
	boost::filesystem::path p2{homedir + "/.ewocvj2/recordings"};
	if (!exists(homedir + "/recordings")) boost::filesystem::create_directory(p2);
	boost::filesystem::path p3{homedir + "/.ewocvj2/shelves"};
	if (!exists(homedir + "/.ewocvj2/shelves")) boost::filesystem::create_directory(p3);
	boost::filesystem::path p4{homedir + "/.ewocvj2/temp"};
	if (!exists(homedir + "/.ewocvj2/temp")) boost::filesystem::create_directory(p4);
	#endif
	#endif

	binsmain->newbinbox = new Box;
	binsmain->newbinbox->vtxcoords->x1 = -0.15f;
	binsmain->newbinbox->vtxcoords->w = 0.3f;
	binsmain->newbinbox->vtxcoords->h = 0.05f;
	binsmain->newbinbox->upvtxtoscr();
	
	binsmain->inputbinel = new BinElement;
	
	binsmain->currbin = new Bin(-1);
	int cb = 0; 
	if (exists(mainprogram->binsdir + "bins.list")) {
		cb = binsmain->read_binslist();
	}
	else {
		binsmain->new_bin("this is a bin");
	}
	
	for (int i = 0; i < binsmain->bins.size(); i++) {
		std::string binname = mainprogram->binsdir + binsmain->bins[i]->name + ".bin";
		if (exists(binname)) binsmain->open_bin(binname, binsmain->bins[i]);
	}
	binsmain->make_currbin(cb);
	binsmain->save_binslist(); // updates old bins to latest bin version
	
	set_thumbs();
		
	GLint Sampler0 = glGetUniformLocation(mainprogram->ShaderProgram, "Sampler0");
	glUniform1i(Sampler0, 0);
	
	for (int i = 0; i < 4; i++) {
		mainmix->nbframesA.push_back(std::vector<Layer*>());
		mainmix->nbframesB.push_back(std::vector<Layer*>());
		mainmix->tempnbframes.push_back(std::vector<Layer*>());
	}
	
	mainprogram->loadlay = new Layer(true);
	for (int i = 0; i < 4; i++) {
		mainmix->page[0] = i;
		Layer *layA = mainmix->add_layer(mainmix->layersA, 0);
		layA->closethread = true;
		mainmix->nbframesA[mainmix->page[0]].insert(mainmix->nbframesA[mainmix->page[0]].begin(), layA);
		layA->clips.clear();
		mainmix->mousedeck = 0;
		mainmix->save_deck(mainprogram->temppath + "tempdeck_A" + std::to_string(i + 1) + ".deck");
		mainmix->save_deck(mainprogram->temppath + "tempdeck_A" + std::to_string(i + 1) + "comp.deck");
		mainmix->page[1] = i;
		Layer *layB = mainmix->add_layer(mainmix->layersB, 0);
		layB->closethread = true;
		layB->deck = 1;
		mainmix->nbframesB[mainmix->page[1]].insert(mainmix->nbframesB[mainmix->page[1]].begin(), layB);
		layB->clips.clear();
		mainmix->mousedeck = 1;
		mainmix->save_deck(mainprogram->temppath + "tempdeck_B" + std::to_string(i + 1) + ".deck");
		mainmix->save_deck(mainprogram->temppath + "tempdeck_B" + std::to_string(i + 1) + "comp.deck");
		mainmix->layersA.clear();
		mainmix->layersB.clear();
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
	BlendNode* bnodeend = mainprogram->nodesmain->currpage->add_blendnode(CROSSFADING, false);
	mainprogram->nodesmain->currpage->connect_nodes(mixnodeA, mixnodeB, bnodeend);
	mainprogram->nodesmain->currpage->connect_nodes(bnodeend, mixnodeAB);
	
	mainmix->page[0] = 0;
	mainmix->mousedeck = 0;
	mainmix->open_deck(mainprogram->temppath + "tempdeck_A1.deck", 1);
	mainmix->page[1] = 0;
	mainmix->mousedeck = 1;
	mainmix->open_deck(mainprogram->temppath + "tempdeck_B1.deck", 1);
	Layer *layA1 = mainmix->layersA[0];
	Layer *layB1 = mainmix->layersB[0];
	mainprogram->nodesmain->currpage->connect_nodes(layA1->node, mixnodeA);
	mainprogram->nodesmain->currpage->connect_nodes(layB1->node, mixnodeB);
	mainmix->currlay = mainmix->layersA[0];
	make_layboxes();

	//temporary
	layA1->lasteffnode->monitor = 0;
	mainprogram->nodesmain->linked = true;
	
	laymidiA = new LayMidi;
	laymidiB = new LayMidi;
	laymidiC = new LayMidi;
	laymidiD = new LayMidi;
	if (exists("./midiset.gm")) open_genmidis("./midiset.gm");
	
	mainmix->copy_to_comp(mainmix->layersA, mainmix->layersAcomp, mainmix->layersB, mainmix->layersBcomp, mainprogram->nodesmain->currpage->nodes, mainprogram->nodesmain->currpage->nodescomp, mainprogram->nodesmain->mixnodescomp, true);
	GLint preff = glGetUniformLocation(mainprogram->ShaderProgram, "preff");
	glUniform1i(preff, 1);
	
	
	// get number of cores
	#ifdef _WIN64
	typedef BOOL (WINAPI *LPFN_GLPI)(
		PSYSTEM_LOGICAL_PROCESSOR_INFORMATION, 
		PDWORD);
	
	LPFN_GLPI glpi;
	BOOL done = FALSE;
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = NULL;
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = NULL;
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

                if (NULL == buffer) 
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

    // must put this here or problem with fbovao[2] ?????
	set_fbo();
	#ifdef _WIN64
	if (exists("./shelves/shelfs.shelf")) open_shelf("./shelves/shelfs.shelf", 2);
	# else
	#ifdef __linux__
	if (exists(homedir + "/.ewocvj2/shelves/shelfs.shelf")) open_shelf(homedir + "/.ewocvj2/shelves/shelfs.shelf", 2);
	#endif
	#endif
	
	std::chrono::high_resolution_clock::time_point begintime = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed;
		
	while (!quit){

		io.poll();
		
		if (!(mainprogram->path == NULL)) {
			if (mainprogram->pathto == "OPENVIDEO") {
				std::string str(mainprogram->path);
				open_video(0, mainprogram->loadlay, str, true);
			}
			else if (mainprogram->pathto == "OPENSHELF") {
				printf("shelfpath %s\n", mainprogram->path);
				std::string str(mainprogram->path);
				open_shelf(str, mainmix->mousedeck);
			}
			else if (mainprogram->pathto == "SAVESHELF") {
				std::string str(mainprogram->path);
				save_shelf(str, mainmix->mousedeck);
			}
			else if (mainprogram->pathto == "OPENSHELFVIDEO") {
				std::string str(mainprogram->path);
				open_shelfvideo(str, mainmix->mouseshelfelem);
			}
			else if (mainprogram->pathto == "OPENSHELFLAYER") {
				std::string str(mainprogram->path);
				open_shelflayer(str, mainmix->mouseshelfelem);
			}
			else if (mainprogram->pathto == "OPENSHELFDIR") {
				mainprogram->shelfpath = mainprogram->path;
				mainprogram->opendir = opendir(mainprogram->path);
				if (mainprogram->opendir) {
					mainprogram->openshelfdir = true;
					mainprogram->shelfdircount = 0;
				}
			}
			else if (mainprogram->pathto == "SAVESTATE") {
				std::string str(mainprogram->path);
				mainmix->save_state(str);
			}
			else if (mainprogram->pathto == "SAVEMIX") {
				std::string str(mainprogram->path);
				mainmix->save_mix(str);
			}
			else if (mainprogram->pathto == "SAVEDECK") {
				std::string str(mainprogram->path);
				mainmix->save_deck(str);
			}
			else if (mainprogram->pathto == "OPENDECK") {
				std::string str(mainprogram->path);
				mainmix->open_deck(str, 1);
			}
			else if (mainprogram->pathto == "SAVELAYFILE") {
				std::string str(mainprogram->path);
				mainmix->save_layerfile(str, mainmix->mouselayer, 1);
			}
			else if (mainprogram->pathto == "OPENLAYFILE") {
				mainmix->open_layerfile(mainprogram->path, mainmix->mouselayer, 1, 1);
			}
			else if (mainprogram->pathto == "OPENSTATE") {
				std::string str(mainprogram->path);
				mainmix->open_state(str);
			}
			else if (mainprogram->pathto == "OPENMIX") {
				std::string str(mainprogram->path);
				mainmix->open_mix(str);
			}
			else if (mainprogram->pathto == "OPENBINFILES") {
				binsmain->openbinfile = true;
			}
			else if (mainprogram->pathto == "OPENBINDIR") {
				binsmain->binpath = mainprogram->path;
				mainprogram->opendir = opendir(binsmain->binpath.c_str());
				if (mainprogram->opendir) {
					binsmain->openbindir = true;
				}
				else {
					mainprogram->blocking = false;
				}
			}
			else if (mainprogram->pathto == "CHOOSEDIR") {
				std::string str(mainprogram->path);
				std::string driveletter1 = str.substr(0, 1);
				std::string abspath = boost::filesystem::absolute("./").string();
				std::string driveletter2 = abspath.substr(0, 1);
				std::string path;
				if (driveletter1 == driveletter2) {
					path = "./" + boost::filesystem::relative(str, "./").string() + "/";
				}
				else {
					path = str + "/";
				}
				if (mainprogram->choosing == EDIT_BINSDIR) {
					mainprogram->binsdir = path;
				}
				if (mainprogram->choosing == EDIT_RECDIR) {
					mainprogram->recdir = path;
				}
				mainprogram->choosing = EDIT_NONE;
			}
			mainprogram->path = NULL;
		}

		bool focus = true;
		mainprogram->mousewheel = 0;
		SDL_Event e;
		while (SDL_PollEvent(&e)){
			SDL_PumpEvents();
			if (mainprogram->renaming != EDIT_NONE) {
                if (e.type == SDL_TEXTINPUT) {
                    /* Add new text onto the end of our text */
                    if( !( ( e.text.text[ 0 ] == 'c' || e.text.text[ 0 ] == 'C' ) && ( e.text.text[ 0 ] == 'v' || e.text.text[ 0 ] == 'V' ) && SDL_GetModState() & KMOD_CTRL ) ) {
						mainprogram->inputtext = mainprogram->inputtext.substr(0, mainprogram->cursorpos) + e.text.text + mainprogram->inputtext.substr(mainprogram->cursorpos, mainprogram->inputtext.length() - mainprogram->cursorpos);
						mainprogram->cursorpos++;
					}
                }
				if (e.type == SDL_KEYDOWN) {
					//Handle cursor left/right 
					if (e.key.keysym.sym == SDLK_LEFT and mainprogram->cursorpos > 0) {
						mainprogram->cursorpos--;
					} 
					if (e.key.keysym.sym == SDLK_RIGHT and mainprogram->cursorpos < mainprogram->inputtext.length()) {
						mainprogram->cursorpos++;
					} 
					//Handle backspace 
					if (e.key.keysym.sym == SDLK_BACKSPACE and mainprogram->inputtext.length() > 0) {
						if (mainprogram->cursorpos != 0) {
							mainprogram->cursorpos--;
							mainprogram->inputtext.erase(mainprogram->inputtext.begin() + mainprogram->cursorpos); 
						}
					} 
					//Handle copy 
					else if (e.key.keysym.sym == SDLK_c and SDL_GetModState() & KMOD_CTRL) { 
						SDL_SetClipboardText( mainprogram->inputtext.c_str() ); 
					} 
					//Handle paste 
					else if (e.key.keysym.sym == SDLK_v and SDL_GetModState() & KMOD_CTRL) { 
						mainprogram->inputtext = SDL_GetClipboardText();
						mainprogram->cursorpos = mainprogram->inputtext.length();
					}
					else if (e.key.keysym.sym == SDLK_RETURN) {
						mainprogram->renaming = EDIT_NONE;
						SDL_StopTextInput();
						continue;
					}
				}
				if (mainprogram->renaming == EDIT_BINNAME) {
					binsmain->menubin->name = mainprogram->inputtext;
					std::string oldpath = mainprogram->binsdir + mainprogram->backupname;
					std::string newpath = mainprogram->binsdir + mainprogram->inputtext;
					boost::filesystem::rename(oldpath, newpath);
					oldpath += ".bin";
					newpath += ".bin";
					boost::filesystem::rename(oldpath, newpath);
					binsmain->save_binslist();
				}
				else if (mainprogram->renaming == EDIT_BINSDIR) {
					mainprogram->binsdir = mainprogram->inputtext;
					std::string lastchar = mainprogram->binsdir.substr(mainprogram->binsdir.length() - 1, std::string::npos);
					if (lastchar != "/" and lastchar != "\\") mainprogram->binsdir += "/";
				}
				else if (mainprogram->renaming == EDIT_RECDIR) {
					mainprogram->recdir = mainprogram->inputtext;
				}
				else if (mainprogram->renaming == EDIT_VIDW) {
					try {
						mainprogram->ow = std::stoi(mainprogram->inputtext);
						set_fbovao2();
					}
					catch(const std::exception& e) {}
				}
				else if (mainprogram->renaming == EDIT_VIDH) {
					try {
						mainprogram->oh = std::stoi(mainprogram->inputtext);
						set_fbovao2();
					}
					catch(const std::exception& e) {}
				}
			}
		
			//If user closes the window
			if (e.type == SDL_WINDOWEVENT) {
				switch (e.window.event) {
					case SDL_WINDOWEVENT_CLOSE:
						if (e.window.windowID == SDL_GetWindowID(mainprogram->mainwindow)) {
							quit = true;
						}
						if (mainprogram->tunemidiwindow) {
							if (e.window.windowID == SDL_GetWindowID(mainprogram->tunemidiwindow)) {
								mainprogram->tunemidi = false;
								mainprogram->tmlearn = TM_NONE;
								mainprogram->drawnonce = false;
								SDL_HideWindow(mainprogram->tunemidiwindow);
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
			if (e.type == SDL_KEYDOWN){
				if (e.key.keysym.sym == SDLK_LCTRL or e.key.keysym.sym == SDLK_RCTRL) {
					mainprogram->ctrl = true;
				}
				if (mainprogram->ctrl) {
					if (e.key.keysym.sym == SDLK_s) {
						mainprogram->pathto = "SAVESTATE";
						std::thread filereq (&Program::get_outname, mainprogram, "Save state file", "application/ewocvj2-state", "");
						filereq.detach();
					}
					if (e.key.keysym.sym == SDLK_o) {
						mainprogram->pathto = "OPENSTATE";
						std::thread filereq (&Program::get_inname, mainprogram, "Open state file", "application/ewocvj2-state", "");
						filereq.detach();
					}
					if (e.key.keysym.sym == SDLK_n) {
						new_file(2, 1);
					}
				}
			}
			if (e.type == SDL_KEYUP){
				if (e.key.keysym.sym == SDLK_LCTRL or e.key.keysym.sym == SDLK_RCTRL) {
					mainprogram->ctrl = false;
				}
				if (e.key.keysym.sym == SDLK_DELETE) {
					mainprogram->del = 1;
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
						if (mainmix->currlay->frame > mainmix->currlay->numf - 1) mainmix->currlay->frame = 0;
					}
				}
				else if (e.key.keysym.sym == SDLK_LEFT) {
					if (mainmix->currlay) {
						mainmix->currlay->frame -= 1;
						if (mainmix->currlay->frame < 0) mainmix->currlay->frame = mainmix->currlay->numf - 1;
					}
				}
			}
			
			if (focus and SDL_GetMouseFocus() != mainprogram->prefwindow and SDL_GetMouseFocus() != mainprogram->tunemidiwindow) {
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
								lvec[j]->scale *= 1 - e.mgesture.dDist * glob->w / 100; 
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
			}
			//If user clicks the mouse
			else if (e.type == SDL_MOUSEBUTTONDOWN){
				if (e.button.button == SDL_BUTTON_RIGHT and !mainmix->learn) {
					for (int i = 0; i < mainprogram->menulist.size(); i++) {
						mainprogram->menulist[i]->state = 0;
					}
					mainprogram->menuactivation = true;
					mainprogram->menux = mainprogram->mx;
					mainprogram->menuy = mainprogram->my;
				}
				if (e.button.button == SDL_BUTTON_LEFT) {
					mainprogram->leftmousedown = true;
				}
				if (e.button.button == SDL_BUTTON_MIDDLE) {
				
				
					mainprogram->middlemousedown = true;
				}
			}
			else if (e.type == SDL_MOUSEBUTTONUP){
				if (e.button.button == SDL_BUTTON_LEFT) {
					if (e.button.clicks == 2) mainprogram->doubleleftmouse = true;
					mainprogram->leftmouse = true;
					mainprogram->leftmousedown = false;
				}
				if (e.button.button == SDL_BUTTON_MIDDLE) {
					mainprogram->middlemouse = true;
				}
				if (e.button.button == SDL_BUTTON_RIGHT) {
					mainprogram->rightmouse = true;
				}
			}
			else if (e.type == SDL_MOUSEWHEEL) {
				mainprogram->mousewheel = e.wheel.y;
			}
		}

		std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
		elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - begintime);
		long long microcount = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
		mainmix->time = microcount / 1000000.0f;
		GLint iGlobalTime = glGetUniformLocation(mainprogram->ShaderProgram, "iGlobalTime");
		glUniform1f(iGlobalTime, mainmix->time);
		
		the_loop();
		
	}

	mainprogram->quit("stop");

}

