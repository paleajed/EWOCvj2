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

#include "IL/il.h"
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
static GLuint thmvbuf;
static GLuint thmvao;
static GLuint boxvao;
FT_Face face;
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

std::string dirname(std::string pathname)
{
	size_t last_slash_idx1 = pathname.find_last_of("//");
	size_t last_slash_idx2 = pathname.find_last_of("\\");
	if (last_slash_idx1 == std::string::npos) last_slash_idx1 = 0;
	if (last_slash_idx2 == std::string::npos) last_slash_idx2 = 0;
	size_t maxpos = last_slash_idx2;
	if (last_slash_idx1 > last_slash_idx2) maxpos = last_slash_idx1;
	if (std::string::npos != maxpos)
	{
		pathname.erase(maxpos + 1, std::string::npos);
	}
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


#ifdef _WIN64
HRESULT EnumerateDevices(REFGUID category, IEnumMoniker **ppEnum)
{
    // Create the System Device Enumerator.
    ICreateDevEnum *pDevEnum;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr,  
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
    IMoniker *pMoniker = nullptr;
	mainprogram->livedevices.clear();
    
    while (pEnum->Next(1, &pMoniker, nullptr) == S_OK)
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
	lay->decresult->width = 0;
	lay->initialized = false;
	lay->audioplaying = false;
	if (lay->clonesetnr != -1) {
		mainmix->clonesets[lay->clonesetnr]->erase(lay);
		if (mainmix->clonesets[lay->clonesetnr]->size() == 0) {
			mainmix->cloneset_destroy(mainmix->clonesets[lay->clonesetnr]);
		}
		lay->clonesetnr = -1;
	}
	lay->type = ELEM_LIVE;
	lay->filename = livename;
	if (lay->video) {
		std::unique_lock<std::mutex> lock(lay->enddecodelock);
		lay->enddecodevar.wait(lock, [&]{return lay->processed;});
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
		lay->reset = false;
		lay->frame = 0.0f;
		lay->prevframe = -1;
		lay->numf = 0;
		lay->startframe = 0;
		lay->endframe = 0;
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
			#define BUFFER_OFFSET(i) ((char *)nullptr + (i))
			glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, mainmix->ioBuf);
			glBufferData(GL_PIXEL_PACK_BUFFER_ARB, (int)(mainprogram->ow * mainprogram->oh) * 4, nullptr, GL_DYNAMIC_READ);
			glBindFramebuffer(GL_FRAMEBUFFER, ((MixNode*)mainprogram->nodesmain->mixnodescomp[2])->mixfbo);
			glReadBuffer(GL_COLOR_ATTACHMENT0);
			glReadPixels(0, 0, mainprogram->ow, (int)mainprogram->oh, GL_BGRA, GL_UNSIGNED_BYTE, BUFFER_OFFSET(0));
			mainmix->rgbdata = glMapBuffer(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY);
			assert(mainmix->rgbdata);
			mainmix->recordnow = true;
			while (mainmix->recordnow) {
				mainmix->startrecord.notify_one();
			}
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
  		if (midi0 == 176 and mainmix->learnparam and mainmix->learnparam->sliding) {
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
		else if (midi0 == 144 and mainmix->learnparam and !mainmix->learnparam->sliding) {
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
  		else if (midi0 == 176 and mainmix->learnbutton) {
			mainmix->learn = false;
			mainmix->learnbutton->midi[0] = midi0;
			mainmix->learnbutton->midi[1] = midi1;
			mainmix->learnbutton->midiport = midiport;
/* 			mainmix->learnbutton->node = new MidiNode;
			mainmix->learnbutton->node->button = mainmix->learnbutton;
			mainprogram->nodesmain->currpage->nodes.push_back(mainmix->learnbutton->node);
			if (mainmix->learnbutton->effect) {
				mainprogram->nodesmain->currpage->connect_in2(mainmix->learnbutton->node, mainmix->learnbutton->effect->node);
			}
IMPLEMENT */		}
		else if (midi0 == 144 and mainmix->learnbutton) {
			mainmix->learn = false;
			mainmix->learnbutton->midi[0] = midi0;
			mainmix->learnbutton->midi[1] = midi1;
			mainmix->learnbutton->midiport = midiport;
/* 			mainmix->learnparam->node = new MidiNode;
			mainmix->learnparam->node->param = mainmix->learnparam;
			mainprogram->nodesmain->currpage->nodes.push_back(mainmix->learnparam->node);
			if (mainmix->learnparam->effect) {
				mainprogram->nodesmain->currpage->connect_in2(mainmix->learnparam->node, mainmix->learnparam->effect->node);
			}
 */		}
 		mainmix->learn = false;
		return;
  	}
  	
  	
  	if (midi0 == 176) {
  		for (int i = 0; i < mainprogram->buttons.size(); i++) {
			Button *but = mainprogram->buttons[i];
			if (midi0 == but->midi[0] and midi1 == but->midi[1] and midiport == but->midiport) {
				mainmix->midi2 = midi2;
				mainmix->midibutton = but;
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
						fflush(stdout);
					}
				}
			}
		}
		mainmix->midiisspeed = false;
		for (int i = 0; i < ns.size(); i++) {
			if (mainprogram->nodesmain->currpage->nodes[i]->type == VIDEO) {
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
			if (mainprogram->nodesmain->currpage->nodes[i]->type == BLEND) {
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

	std::cout << "message = " << (int)message->at(0) << std::endl;
	std::cout << "? = " << (int)message->at(1) << std::endl;
	std::cout << "value = " << (int)message->at(2) << std::endl;
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
	int got_frame;
	if (outfile) {
		avcodec_send_frame(enc_ctx, frame);
 		int err = avcodec_receive_packet(enc_ctx, pkt);
 	}
	else {
		avcodec_send_frame(enc_ctx, frame);
 		int err = avcodec_receive_packet(enc_ctx, &enc_pkt);
		av_packet_rescale_ts(&enc_pkt, srcctx->streams[enc_pkt.stream_index]->time_base, fmtctx->streams[enc_pkt.stream_index]->time_base);
 	}
	if (outfile) fwrite(pkt->data, 1, pkt->size, outfile);
	else {
		/* prepare packet for muxing */
		enc_pkt.stream_index = pkt->stream_index;
		//enc_pkt.duration = pkt->duration;//(fmtctx->streams[enc_pkt.stream_index]->time_base.den * fmtctx->streams[enc_pkt.stream_index]->r_frame_rate.den) / (fmtctx->streams[enc_pkt.stream_index]->time_base.num * fmtctx->streams[enc_pkt.stream_index]->r_frame_rate.num);
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
    	while (1) {
			int err1 = avcodec_send_packet(lay->video_dec_ctx, &lay->decpkt);
			int err2 = avcodec_receive_frame(lay->video_dec_ctx, lay->decframe);
			if (err2 == AVERROR(EAGAIN)) {
				av_packet_unref(&lay->decpkt);
				av_read_frame(lay->video, &lay->decpkt);
			}
			else break;
		}
 		//char buffer[1024];
 		//av_strerror(err2, buffer, 1024);
 		//printf(buffer);
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
	int *snippet = nullptr;
	int *snip = nullptr;
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
			//lay->decpkt.data = nullptr;
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
		while (lay->chready) {
			lay->newchunk.notify_one();
		}
	}
}	
	
void get_frame_other(Layer *lay, int framenr, int prevframe, int errcount)
{
   	int ret = 0, got_frame;
	/* initialize packet, set data to nullptr, let the demuxer fill it */
	av_init_packet(&lay->decpkt);
	lay->decpkt.data = nullptr;
	lay->decpkt.size = 0;
	
	if (lay->type != ELEM_LIVE) {
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
//    lay->decpkt.data = nullptr;
//    lay->decpkt.size = 0;
//    do {
//        decode_packet(&got_frame, 1);
//    } while (got_frame);
    
		decode_audio(lay);
		if (!lay->dummy) {
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
			//avformat_seek_file(lay->video, lay->video_stream->index, 0, lay->video->streams[lay->video_stream->index]->start_time, lay->video->streams[lay->video_stream->index]->start_time, 0);
			av_seek_frame(lay->video, lay->video_stream->index, lay->video->streams[lay->video_stream->index]->start_time, 0);
			avcodec_flush_buffers(lay->video_dec_ctx);
				av_packet_unref(&lay->decpkt);
				if (av_read_frame(lay->video, &lay->decpkt) < 0) printf("READPROBLEM\n");;
			}
		}
		decode_packet(lay, &got_frame);
		lay->prevframe = framenr;
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
	if (st) this->decresult->data = nullptr;
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
			if (!this->dummy and this->video) {
				std::unique_lock<std::mutex> lock(this->enddecodelock);
				this->enddecodevar.wait(lock, [&]{return this->processed;});
				this->processed = false;
				lock.unlock();
				avformat_close_input(&this->video);
			}
			bool r = thread_vidopen(this, nullptr, false);
			this->vidopen = false;
			if (r == 0) {
				printf("load error\n");
				this->openerr = true;
			}
			this->opened = true;
			this->endopenvar.notify_one();
			this->initialized = false;
			continue;
		}
		if (this->liveinput == nullptr) {
			if (!this->video_stream) continue;
			if (this->vidformat != 188 and this->vidformat != 187) {
				if ((int)(this->frame) != this->prevframe) {
					get_frame_other(this, (int)this->frame, this->prevframe, 0);
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
				this->decode_frame();
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

void open_video(float frame, Layer *lay, const std::string &filename, int reset) {
	if (lay->boundimage != -1) {
		glDeleteTextures(1, &lay->boundimage);
		lay->boundimage = -1;
	}
	lay->video_dec_ctx = nullptr;
	lay->audioplaying = false;
	if (lay->effects[0].size() == 0) lay->type = ELEM_FILE;
	else lay->type = ELEM_LAYER;
	lay->filename = filename;
	if (frame >= 0.0f) lay->frame = frame;
	lay->prevframe = lay->frame - 1;
	lay->reset = reset;
	lay->vidopen = true;
	lay->ready = true;
	while (lay->ready) {
		lay->startdecode.notify_one();
	}
	if (lay->clonesetnr != -1) {
		mainmix->clonesets[lay->clonesetnr]->erase(lay);
		if (mainmix->clonesets[lay->clonesetnr]->size() == 0) {
			mainmix->cloneset_destroy(mainmix->clonesets[lay->clonesetnr]);
		}
		lay->clonesetnr = -1;
	}
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
	
	int r = avformat_open_input(&(lay->video), lay->filename.c_str(), ifmt, nullptr);
	printf("loading... %s\n", lay->filename.c_str());
	if (r < 0) {
		lay->filename = "";
		printf("%s\n", "Couldnt open video");
		return 0;
	}

    if (avformat_find_stream_info(lay->video, nullptr) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        return 0;
    }
	lay->video->max_picture_buffer = 20000000;

   if (open_codec_context(&(lay->video_stream_idx), lay->video, AVMEDIA_TYPE_VIDEO) >= 0) {
     	lay->video_stream = lay->video->streams[lay->video_stream_idx];
        lay->video_dec_ctx = lay->video_stream->codec;
        lay->vidformat = lay->video_dec_ctx->codec_id;
		if (lay->vidformat == 188 or lay->vidformat == 187) {
        	if (lay->databuf) free(lay->databuf);
			lay->databuf = (char*)malloc(lay->video_dec_ctx->width * lay->video_dec_ctx->height);
			lay->numf = lay->video_stream->nb_frames;
			float tbperframe = (float)lay->video_stream->duration / (float)lay->numf;
			lay->millif = tbperframe * (((float)lay->video_stream->time_base.num * 1000.0) / (float)lay->video_stream->time_base.den);
			if (lay->reset) {
				lay->startframe = 0;
				lay->endframe = lay->numf - 1;
				lay->frame = (lay->numf - 1) * (lay->reset - 1);
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
    lay->rgbframe->format = AV_PIX_FMT_BGRA;
    lay->rgbframe->width  = lay->video_dec_ctx->width;
    lay->rgbframe->height = lay->video_dec_ctx->height;
	int storage = av_image_alloc(lay->rgbframe->data, lay->rgbframe->linesize, lay->rgbframe->width, lay->rgbframe->height, AV_PIX_FMT_BGRA, 1);
  	lay->sws_ctx = sws_getContext
    (
        lay->video_dec_ctx->width,
       	lay->video_dec_ctx->height,
        lay->video_dec_ctx->pix_fmt,
        lay->video_dec_ctx->width,
        lay->video_dec_ctx->height,
        AV_PIX_FMT_BGRA,
        SWS_BILINEAR,
        nullptr,
        nullptr,
        nullptr);

    lay->decframe = av_frame_alloc();
	
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
			float fac = mainprogram->deckspeed[this->deck]->value;
			if (this->clonesetnr != -1) {
				std::unordered_set<Layer*>::iterator it;
				for (it = mainmix->clonesets[this->clonesetnr]->begin(); it != mainmix->clonesets[this->clonesetnr]->end(); it++) {
					Layer *lay = *it;
					if (lay->deck == !this->deck) {
						fac *= mainprogram->deckspeed[!this->deck]->value;
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
	float boxwidth = 0.1f;
	glGenTextures(16, this->texes);
	for (int i = 0; i < 16; i++) {
		this->paths[i] = "";
		this->types[i] = ELEM_FILE;
		glBindTexture(GL_TEXTURE_2D, this->texes[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		
		this->buttons[i] = new Button(false);
		Box *box = this->buttons[i]->box;
		box->vtxcoords->x1 = -1.0f + (i % 4) * boxwidth + (2.0f - boxwidth * 4) * side;
		box->vtxcoords->h = boxwidth * (glob->w / glob->h) / (1920.0f /  1080.0f);
		box->vtxcoords->y1 = -1.0f + (int)(i / 4) * box->vtxcoords->h;
		box->vtxcoords->w = boxwidth;
		box->upvtxtoscr();
		box->tooltiptitle = "Video launch shelf";
		box->tooltip = "Shelf containing up to 16 videos/layerfiles for quick and easy video launching.  Left drag'n'drop from other areas, both videos and layerfiles.  Rightdrag of videos to layers launches with empty effect stack.  Rightclick launches shelf menu. ";
	}
}
	

void set_fbo() {

    glGenTextures(1, &mainmix->mixbackuptex);
    glBindTexture(GL_TEXTURE_2D, mainmix->mixbackuptex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, glob->w, glob->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenTextures(1, &mainprogram->fbotex[0]); //comp = false
    glGenTextures(1, &mainprogram->fbotex[1]);
   	glGenTextures(1, &mainprogram->fbotex[2]); //comp = true
    glGenTextures(1, &mainprogram->fbotex[3]);
    glBindTexture(GL_TEXTURE_2D, mainprogram->fbotex[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, glob->w, glob->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, mainprogram->fbotex[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, glob->w, glob->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, mainprogram->fbotex[2]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, mainprogram->ow, mainprogram->oh, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, mainprogram->fbotex[3]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, mainprogram->ow, mainprogram->oh, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
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
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
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
	float pixelw = 2.0f / (float)glob->w;
	float pixelh = 2.0f / (float)glob->h;

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
	GLuint boxtbuf;
	glGenBuffers(1, &boxbuf);	
	glGenBuffers(1, &boxtbuf);
	glBindBuffer(GL_ARRAY_BUFFER, boxbuf);
   	glBufferData(GL_ARRAY_BUFFER, 32, fvcoords, GL_STATIC_DRAW);
	glGenVertexArrays(1, &boxvao);
	glBindVertexArray(boxvao);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
	if (linec) {
		glDrawArrays(GL_LINE_LOOP, 0, 4);
	} 
	if (areac) {
		float shx = -dx / ((float)glob->w * 0.1f);
		float shy = -dy / ((float)glob->h * 0.1f);
		GLfloat fvcoords2[8] = {
			x + pixelw,     y + he - pixelh,
			x + pixelw,     y + pixelh,
			x + wi - pixelw, y + he - pixelh,
			x + wi - pixelw, y + pixelh,
		};
		glBindBuffer(GL_ARRAY_BUFFER, boxbuf);
		glBufferData(GL_ARRAY_BUFFER, 32, fvcoords2, GL_STATIC_DRAW);
		//glBindVertexArray(boxvao);
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
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
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
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
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

float render_text(std::string text, float *textc, float x, float y, float sx, float sy, int smflag, bool vertical) {
 	y -= 0.03f;
	GLuint texture;
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
		glTexImage2D(        //reminder: texture size efficiency!
			GL_TEXTURE_2D,
			0,
			GL_RGBA8,
			2048,
			64,
			0,
			GL_BGRA,
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
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
		glBindBuffer(GL_ARRAY_BUFFER, textbuf);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
		GLuint ftex;
		glGenTextures(1, &ftex);
		glBindTexture(GL_TEXTURE_2D, ftex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		
		float pixelw, pixelh;
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
	
			pixelw = 2.0f / glob->w;
			pixelh = 2.0f / glob->h;
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
		
		//cropping texture
		int w = textw * 2.2f / pixelw;
		GLuint endtex;
		glGenTextures(1, &endtex);
		glBindTexture(GL_TEXTURE_2D, endtex);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RGBA8,
			w,
			64,
			0,
			GL_BGRA,
			GL_UNSIGNED_BYTE,
			0
			);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		GLuint endfrbuf;
		glGenFramebuffers(1, &endfrbuf);
		glBindFramebuffer(GL_FRAMEBUFFER, endfrbuf);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, endtex, 0);
		glBlitNamedFramebuffer(texfrbuf, endfrbuf, 0, 0, w, 64 , 0, 0, w, 64, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		
		GUIString *guistring = new GUIString;
		guistring->text = text;
		guistring->texture = endtex;
		guistring->textw = textw;
		guistring->texth = texth;
		guistring->sx = sx;
		if (smflag == 0) mainprogram->guistrings.push_back(guistring);
		else if (smflag == 1) mainprogram->prguistrings.push_back(guistring);
		else if (smflag == 2) mainprogram->tmguistrings.push_back(guistring);
		
		glDeleteFramebuffers(1, &texfrbuf);
		glDeleteFramebuffers(1, &endfrbuf);
		glDeleteTextures(1, &ftex);
		glDeleteTextures(1, &texture);
		
		return textw;
  	}
	else {
		GLfloat texvcoords[8];
		GLfloat *p = texvcoords;
		float th = 0.0012f;
		float pixelw = 2.0f / glob->w;
		float wfac = textw / pixelw / 512.0f;
		float thtrans[] = {th, -th};
		GLfloat trans = glGetUniformLocation(mainprogram->ShaderProgram, "translation");
		glUniform2fv(trans, 1, thtrans);
		if (vertical) {
			*p++ = x; *p++ = y;
			*p++ = x; *p++ = y - texth * 8 * wfac * glob->w / glob->h;
			*p++ = x + texth * glob->h / glob->w;     *p++ = y;
			*p++ = x + texth * glob->h / glob->w;     *p++ = y - texth * 8 * wfac * glob->w / glob->h;
		}
		else {
			*p++ = x;    *p++ = y;
			*p++ = x + texth * 8 * wfac; *p++ = y;
			*p++ = x;     *p++ = y + texth;
			*p++ = x + texth * 8 * wfac; *p++ = y + texth;
		}
		GLfloat textcoords[] = {0.0f, 0.0f,
							1.0f, 0.0f,
							0.0f, 1.0f,
							1.0f, 1.0f};
		GLint textmode = glGetUniformLocation(mainprogram->ShaderProgram, "textmode");
		glUniform1i(textmode, 1);
		float black[] = {0.0f, 0.0f, 0.0f, 1.0f};
		GLint color = glGetUniformLocation(mainprogram->ShaderProgram, "color");
		glUniform4fv(color, 1, black);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		GLuint texvao;
		glGenVertexArrays(1, &texvao);
		glBindVertexArray(texvao);
		GLuint vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, 32, texvcoords, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
		GLuint tbo;
		glGenBuffers(1, &tbo);
		glBindBuffer(GL_ARRAY_BUFFER, tbo);
		glBufferData(GL_ARRAY_BUFFER, 32, textcoords, GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
		if (mainprogram->startloop) {
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
		}
		if (textw != 0) glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);  //draw text shadow
		glUniform4fv(color, 1, textc);
		float zerotrans[] = {0.0f, 0.0f};
		glUniform2fv(trans, 1, zerotrans);
		if (textw != 0) glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);	//draw text
		
		glUniform1i(textmode, 0);
		glDeleteVertexArrays(1, &texvao);
		glDeleteBuffers(1, &vbo);
		glDeleteBuffers(1, &tbo);
	}
	
	return textw;
}


// calculates new frame numbers for a video layer
// also checks if new frame has been decompressed and loads it into layer
void calc_texture(Layer *lay, bool comp, bool alive) {
	// laynocomp is root layer for comp layer when video previewing is on
	Layer *laynocomp = lay;
	
	//measure time since last loop
	std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed;
	if (lay->timeinit) elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - lay->prevtime);
	else {
		lay->timeinit = true;
		lay->prevtime = now;
		return;
	}
	long long microcount = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
	lay->prevtime = now;
	float thismilli = (float)microcount / 1000.0f;
	
	if (mainprogram->tooltipbox and laynocomp->deck == 0 and laynocomp->pos == 0) {
		mainprogram->tooltipmilli += thismilli;
	} 
		
	if (lay->type != ELEM_LIVE) {
		if (!lay->vidopen) {
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
			float fac = mainprogram->deckspeed[laynocomp->deck]->value;
			if (laynocomp->clonesetnr != -1) {
				std::unordered_set<Layer*>::iterator it;
				for (it = mainmix->clonesets[laynocomp->clonesetnr]->begin(); it != mainmix->clonesets[laynocomp->clonesetnr]->end(); it++) {
					Layer *l = *it;
					if (l->deck == !laynocomp->deck) {
						fac *= mainprogram->deckspeed[!laynocomp->deck]->value;
						break;
					}
				}
			}
			if (lay->type == ELEM_IMAGE) {
				ilBindImage(lay->boundimage);
				lay->millif = ilGetInteger(IL_IMAGE_DURATION);
			}
			if ((laynocomp->speed->value > 0 and (laynocomp->playbut->value or laynocomp->bouncebut->value == 1)) or (laynocomp->speed->value < 0 and (laynocomp->revbut->value or laynocomp->bouncebut->value == 2))) {
				lay->frame += !laynocomp->scratchtouch * laynocomp->speed->value * fac * fac *  laynocomp->speed->value * thismilli / lay->millif;
			}
			else if ((laynocomp->speed->value > 0 and (laynocomp->revbut->value or laynocomp->bouncebut->value == 2)) or (laynocomp->speed->value < 0 and (laynocomp->playbut->value or laynocomp->bouncebut->value == 1))) {
				lay->frame -= !laynocomp->scratchtouch * laynocomp->speed->value * fac * fac *  laynocomp->speed->value * thismilli / lay->millif;
			}
			
			// on end of video (or beginning if reverse play) switch to next clip in queue
			if (lay->frame > (laynocomp->endframe)) {
				if (lay->scritching != 4) {
					if (laynocomp->bouncebut->value == 0) {
						lay->frame = laynocomp->startframe;
						if (lay->clips.size() > 1) {
							Clip *clip = lay->clips[0];
							lay->clips.erase(lay->clips.begin());
							VideoNode *node = lay->node;
							ELEM_TYPE temptype = lay->type;
							std::string tpath;
							GLuint temptex;
							if (temptype == ELEM_LAYER and lay->effects[0].size()) {
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
								lay->type = ELEM_FILE;
								open_video(0, lay, clip->path, 1);
							}
							else if (clip->type == ELEM_LAYER) {
								lay->type = ELEM_LAYER;
								mainmix->open_layerfile(clip->path, lay, 1, 0);
							}
							else if (clip->type == ELEM_LIVE) {
								set_live_base(lay, clip->path);
							}
							lay->type = clip->type;
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
							VideoNode *node = lay->node;
							GLuint temptex = copy_tex(node->vidbox->tex);
							ELEM_TYPE temptype;
							std::string tpath;
							if (lay->effects[0].size()) {
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
								lay->type = ELEM_FILE;
								open_video(0, lay, clip->path, 2);
							}
							else if (clip->type == ELEM_LAYER) {
								lay->type = ELEM_LAYER;
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
	
	// advance ripple effects
	for (int m = 0; m < 2; m++) {
		for (int i = 0; i < lay->effects[m].size(); i++) {
			if (lay->effects[m][i]->type == RIPPLE) {
				RippleEffect *effect = (RippleEffect*)(lay->effects[m][i]);
				effect->set_ripplecount(effect->get_ripplecount() + (effect->get_speed() * (thismilli / 50.0f)));
				if (effect->get_ripplecount() > 100) effect->set_ripplecount(0);
			}
		}
	}
	
	if (!alive) return;
	
	Layer *srclay = lay;
	if (lay->startframe != lay->endframe) {
		if (mainmix->firstlayers.count(lay->clonesetnr) == 0) {
			lay->ready = true;
			while (lay->ready) {
				lay->startdecode.notify_one();
			}
			if (lay->clonesetnr != -1) {
				mainmix->firstlayers[lay->clonesetnr] = lay;
			}
		}
		else {
			srclay = mainmix->firstlayers[lay->clonesetnr];
		}
	}
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, lay->texture);
	if (!lay->liveinput) {
		if ((!lay->initialized or srclay == lay)) {  // decresult contains new frame width, height, number of bitmaps and data
			if (srclay->decresult->width or srclay->type == ELEM_IMAGE) {
				if (srclay->vidformat == 188 or srclay->vidformat == 187 and lay->video_dec_ctx) {
					// HAP video layers
					if (!lay->initialized) {
						float w = lay->video_dec_ctx->width;
						float h = lay->video_dec_ctx->height;
						lay->initialize(w, h, srclay->decresult->compression);
					}
					if (srclay->decresult->compression == 187) {
						glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, srclay->decresult->width, srclay->decresult->height, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, srclay->decresult->size, srclay->decresult->data);
					}
					else if (srclay->decresult->compression == 190) {
						glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, srclay->decresult->width, srclay->decresult->height, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, srclay->decresult->size, srclay->decresult->data);
					}
				}
				else {
					if (srclay->type == ELEM_IMAGE) {
						ilBindImage(srclay->boundimage);
						ilActiveImage((int)srclay->frame);
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
					}
					else if (lay->video_dec_ctx) {
						if (!lay->initialized) {
							float w = lay->video_dec_ctx->width;
							float h = lay->video_dec_ctx->height;
							lay->initialize(w, h);
						}
						glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, srclay->decresult->width, srclay->decresult->height, GL_BGRA, GL_UNSIGNED_BYTE, srclay->decresult->data);
					}
				}
			}
			else return;
		}
	}
	else {
		if (!lay->initialized) {
			float w = lay->liveinput->video_dec_ctx->width;
			float h = lay->liveinput->video_dec_ctx->height;
			lay->initialize(w, h);
		}
	}
	glDisable(GL_BLEND);
	// put lay->texture into lay->fbo(tex)
	glBindFramebuffer(GL_FRAMEBUFFER, lay->fbo);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	GLuint tex;
	if (lay->liveinput) tex = lay->liveinput->texture;
	else if (srclay->decresult->width or srclay->type == ELEM_IMAGE) tex = srclay->texture;
	else {
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glEnable(GL_BLEND);
		return;
	}
	int sw, sh;
	glBindTexture(GL_TEXTURE_2D, lay->fbotex);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &sw);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &sh);
	glViewport(0, 0, sw, sh);
	float black[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	draw_box(nullptr, black, -1.0f, 1.0f, 2.0f, -2.0f, 0, 0, 1, 1, 0, tex, glob->w, glob->h);
	glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glEnable(GL_BLEND);
	glViewport(0, 0, glob->w, glob->h);
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
	float green[4] = {0.0f, 1.0f, 0.2f, 1.0f};
	float darkgreen1[4] = {0.0f, 0.75f, 0.0f, 1.0f};
	float darkgreen2[4] = {0.0f, 0.4f, 0.0f, 1.0f};
	float lightblue[4] = {0.0f, 0.2f, 1.0f, 1.0f};
	float white[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	float red[4] = {1.0f, 0.0f, 0.0f, 1.0f};
	float darkred1[4] = {0.75f, 0.0f, 0.0f, 1.0f};
	float darkred2[4] = {0.4f, 0.0f, 0.0f, 1.0f};
	float black[4] = {0.0f, 0.0f, 0.0f, 1.0f};

	glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	
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
					
	std::vector<Layer*> &lvec = choose_layers(deck);
	if (mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos > lvec.size() - 2) mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos = lvec.size() - 2;
	if (mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos < 0) mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos = 0;
	if (lay->pos >= mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos and lay->pos < mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos + 3) {
		Box *box = lay->node->vidbox;
		if (!lay->mutebut->value) {
			draw_box(box, -1);
		}
		else {
			draw_box(white, darkred2, box, -1);
		}
		int sxs = xs * box->scrcoords->w / 2.0f;
		int sys = ys * box->scrcoords->h / 2.0f;
		scix = scix * box->scrcoords->w / 2.0f;
		sciy = sciy * box->scrcoords->h / 2.0f;
		glViewport(box->scrcoords->x1 + sxs, glob->h - box->scrcoords->y1 + sys, box->scrcoords->w - sxs * 2.0f, box->scrcoords->h - sys  * 2.0f);
		glEnable(GL_SCISSOR_TEST);
		glScissor(box->scrcoords->x1 + scix, glob->h - box->scrcoords->y1 + sciy, box->scrcoords->w - scix * 2.0f, box->scrcoords->h - sciy  * 2.0f);
		if (!lay->mutebut->value) {
			draw_box(nullptr, box->acolor, -1.0f, -1.0f, 2.0f, 2.0f, box->tex);
		}
		else {
			draw_box(nullptr, box->acolor, -1.0f, -1.0f, 2.0f, 2.0f, 0, 0, 1, 0.5f, 0, box->tex, 0, 0);
		}
		glDisable(GL_SCISSOR_TEST);
		glViewport(0, 0, glob->w, glob->h);

		if (mainmix->mode == 0 and mainprogram->nodesmain->linked) {
			// Trigger mainprogram->laymenu1
			if (box->in()) {
				std::string deckstr;
				if (deck == 0) deckstr = "A";
				else if (deck == 1) deckstr = "B";
				render_text("Layer " + deckstr + std::to_string(lay->pos + 1), white, box->vtxcoords->x1 + tf(0.01f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.03f), 0.0005f, 0.0008f);
				render_text(remove_extension(basename(lay->filename)), white, box->vtxcoords->x1 + tf(0.01f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.06f), 0.0005f, 0.0008f);
				if (lay->vidformat == -1) {
					if (lay->type == ELEM_IMAGE) {
						render_text("IMAGE", white, box->vtxcoords->x1 + tf(0.01f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.09f), 0.0005f, 0.0008f);
					}
				}
				else if (lay->vidformat == 188 or lay->vidformat == 187) {
					render_text("HAP", white, box->vtxcoords->x1 + tf(0.01f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.09f), 0.0005f, 0.0008f);
				}
				else {
					render_text("CPU", white, box->vtxcoords->x1 + tf(0.01f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.09f), 0.0005f, 0.0008f);
				}
				if (!mainprogram->needsclick or mainprogram->leftmouse) {
					if (!mainmix->moving and !mainprogram->cwon) {
						mainmix->currlay = lay;
						mainprogram->effcat[lay->deck]->value = 0;
						if (mainprogram->menuactivation) {
							if (lay->type == ELEM_IMAGE or lay->type == ELEM_LIVE) mainprogram->laymenu2->state = 2;
							else mainprogram->laymenu1->state = 2;
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
				int numonscreen = size - mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos;
				if (0 <= numonscreen and numonscreen <= 2) {
					if (mainprogram->xvtxtoscr(mainprogram->numw + deck * 1.0f + numonscreen * mainprogram->layw) < mainprogram->mx and mainprogram->mx < deck * (glob->w / 2.0f) + glob->w / 2.0f) {
						if (0 < mainprogram->my and mainprogram->my < mainprogram->yvtxtoscr(mainprogram->layh)) {
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
					mixstr = "CKey";
					if (lay->pos > 0) {
						draw_box(lay->colorbox, -1);
						render_text("Color", white, lay->mixbox->vtxcoords->x1 + tf(0.08f), 1.0f - (tf(mainprogram->layh)) + tf(0.02f), tf(0.0003f), tf(0.0005f));
					}
					break;
				case 20:
					mixstr = "Disp";
					break;
			}
			if (lay->pos > 0) {
				render_text(mixstr, white, lay->mixbox->vtxcoords->x1 + tf(0.01f), 1.0f - (tf(mainprogram->layh)) + tf(0.02f), tf(0.0003f), tf(0.0005f));
			}
			else {
				render_text(mixstr, red, lay->mixbox->vtxcoords->x1 + tf(0.01f), 1.0f - (tf(mainprogram->layh)) + tf(0.02f), tf(0.0003f), tf(0.0005f));
			}
			
			// Draw and handle effect category buttons
			mainprogram->effcat[lay->deck]->handle();	
			Box *box = mainprogram->effcat[lay->deck]->box;
			render_text(mainprogram->effcat[lay->deck]->name[mainprogram->effcat[lay->deck]->value], white, box->vtxcoords->x1, box->vtxcoords->y1 + box->vtxcoords->h - tf(0.01f), tf(0.0003f), tf(0.0005f), 0, 1);
			std::vector<Effect*> &evec = lay->choose_effects();
			bool cat = mainprogram->effcat[lay->deck]->value;
				
			// Draw and handle effect stack scrollboxes
			if (lay->effscroll[cat] > 0 and mainmix->currlay->deck == 0) {
				if (mainprogram->effscrollupA->in()) {
					mainprogram->effscrollupA->acolor[0] = 0.5f;
					mainprogram->effscrollupA->acolor[1] = 0.5f;
					mainprogram->effscrollupA->acolor[2] = 1.0f;
					mainprogram->effscrollupA->acolor[3] = 1.0f;
					if (mainprogram->leftmouse) {
						lay->effscroll[cat]--;
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
			if (lay->effscroll[cat] > 0 and mainmix->currlay->deck == 1) {
				if (mainprogram->effscrollupB->in()) {
					mainprogram->effscrollupB->acolor[0] = 0.5f;
					mainprogram->effscrollupB->acolor[1] = 0.5f;
					mainprogram->effscrollupB->acolor[2] = 1.0f;
					mainprogram->effscrollupB->acolor[3] = 1.0f;
					if (mainprogram->leftmouse) {
						lay->effscroll[cat]--;
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
			if (lay->numefflines[cat] - lay->effscroll[cat] > 11 and mainmix->currlay->deck == 0) {
				if (mainprogram->effscrolldownA->in()) {
					mainprogram->effscrolldownA->acolor[0] = 0.5f;
					mainprogram->effscrolldownA->acolor[1] = 0.5f;
					mainprogram->effscrolldownA->acolor[2] = 1.0f;
					mainprogram->effscrolldownA->acolor[3] = 1.0f;
					if (mainprogram->leftmouse) {
						lay->effscroll[cat]++;
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
			if (lay->numefflines[cat] - lay->effscroll[cat] > 11 and mainmix->currlay->deck == 1) {
				if (mainprogram->effscrolldownB->in()) {
					mainprogram->effscrolldownB->acolor[0] = 0.5f;
					mainprogram->effscrolldownB->acolor[1] = 0.5f;
					mainprogram->effscrolldownB->acolor[2] = 1.0f;
					mainprogram->effscrolldownB->acolor[3] = 1.0f;
					if (mainprogram->leftmouse) {
						lay->effscroll[cat]++;
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
			if (lay->effects[cat].size()) {
				if ((glob->w / 2.0f > mainprogram->mx and mainmix->currlay->deck == 0) or (glob->w / 2.0f < mainprogram->mx and mainmix->currlay->deck == 1)) {
					if (mainprogram->my > mainprogram->yvtxtoscr(mainprogram->layh - tf(0.20f))) {
						if (mainprogram->mousewheel and lay->numefflines[cat] > 11) {
							lay->effscroll[cat] -= mainprogram->mousewheel;
							if (lay->effscroll[cat] < 0) lay->effscroll[cat] = 0;
							if (lay->numefflines[cat] - lay->effscroll[cat] < 11) lay->effscroll[cat] = lay->numefflines[cat] - 11;
						}
					}
				}
			}
			// Draw effectboxes and parameters
			std::string effstr;
			for(int i = 0; i < evec.size(); i++) {
				Effect *eff = evec[i];
				Box *box;
				float x1, y1, wi;
				
				if (eff->box->vtxcoords->y1 < 1.0 - tf(mainprogram->layh) - tf(0.22f) - tf(0.05f) * 10) break;
				if (eff->box->vtxcoords->y1 <= 1.0 - tf(mainprogram->layh) - tf(0.18f)) {
					eff->drywet->handle();		
					eff->onoffbutton->handle();
					
					box = eff->box;
					if (mainprogram->effcat[lay->deck]->value == 0) {
						if (eff->onoffbutton->value) draw_box(white, darkred1, box, -1);
						else draw_box(white, darkred2, box, -1);
					}
					else {
						if (eff->onoffbutton->value) draw_box(white, darkgreen1, box, -1);
						else draw_box(white, darkgreen2, box, -1);
					}
					
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
						case 38:
							effstr = "FLIP";
							break;
						case 39:
							effstr = "MIRROR";
							break;
					}
					float textw = tf(render_text(effstr, white, eff->box->vtxcoords->x1 + tf(0.01f), eff->box->vtxcoords->y1 + tf(0.05f) - tf(0.030f), tf(0.0003f), tf(0.0005f)));
					eff->box->vtxcoords->w = textw + tf(0.032f);
					x1 = eff->box->vtxcoords->x1 + tf(0.032f) + textw;
					wi = (0.7f - mainprogram->numw - tf(0.032f) - textw) / 4.0f;
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
						wi = (0.7f - mainprogram->numw - tf(0.02f)) / 4.0;
					}
					par->box->vtxcoords->x1 = x1;
					x1 += wi + tf(0.01f);
					par->box->vtxcoords->y1 = y1;
					par->box->vtxcoords->w = wi;
					par->box->vtxcoords->h = eff->box->vtxcoords->h;
					par->box->upvtxtoscr();
					
					if (par->box->vtxcoords->y1 < 1.0 - tf(mainprogram->layh) - tf(0.22f) - tf(0.05f) * 10) break;
					if (par->box->vtxcoords->y1 <= 1.0 - tf(mainprogram->layh) - tf(0.18f)) {
						par->handle();
					}
				}
			}
			// Handle color tolerance
			if (lay->pos > 0) {
				if (lay->blendnode->blendtype == COLOURKEY) {
					Param *par = lay->chtol;
					par->box->lcolor[0] = 1.0;
					par->box->lcolor[1] = 1.0;
					par->box->lcolor[2] = 1.0;
					par->box->lcolor[3] = 1.0;
					par->box->acolor[0] = 0.2;
					par->box->acolor[1] = 0.2;
					par->box->acolor[2] = 0.2;
					par->box->acolor[3] = 1.0;
					par->box->vtxcoords->x1 = lay->colorbox->vtxcoords->x1 + tf(0.07f);
					par->box->vtxcoords->y1 = lay->colorbox->vtxcoords->y1;
					par->box->vtxcoords->w = tf(0.08f);
					par->box->vtxcoords->h = lay->colorbox->vtxcoords->h;
					par->box->upvtxtoscr();
					
					par->handle();
				}
			}
			
					
			// Draw mixfac->box
			if (lay->pos > 0 and lay->blendnode->blendtype != COLOURKEY) {
				Param *par = lay->blendnode->mixfac;
				par->handle();
			}
				
			// Draw speed->box
			Param *par = lay->speed;
			par->handle();
			draw_box(white, nullptr, lay->speed->box->vtxcoords->x1, lay->speed->box->vtxcoords->y1, lay->speed->box->vtxcoords->w * 0.30f, tf(0.05f), -1);
			
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
					lay->set_clones();
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
					lay->set_clones();
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
					lay->set_clones();
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
					if (lay->frame > lay->numf - 1) lay->frame = 0.0f;
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
					if (lay->frame < 0) lay->frame = lay->numf - 1.0f;
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
					lay->set_clones();
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
			bool ends = false;
			if (lay->loopbox->in()) {
				if (mainprogram->menuactivation) {
					mainprogram->loopmenu->state = 2;
					mainmix->mouselayer = lay;
					mainprogram->menuactivation = false;
				}
				
				if (pdistance(mainprogram->mx, mainprogram->my, lay->loopbox->scrcoords->x1 + lay->startframe * (lay->loopbox->scrcoords->w / (lay->numf - 1)), lay->loopbox->scrcoords->y1, lay->loopbox->scrcoords->x1 + lay->startframe * (lay->loopbox->scrcoords->w / (lay->numf - 1)), lay->loopbox->scrcoords->y1 - mainprogram->yvtxtoscr(0.045f)) < 6) {
					ends = true;
					if (mainprogram->middlemousedown) {
						lay->scritching = 2;
						mainprogram->middlemousedown = false;
					}
				}
				else if (pdistance(mainprogram->mx, mainprogram->my, lay->loopbox->scrcoords->x1 + lay->startframe * (lay->loopbox->scrcoords->w / (lay->numf - 1)) +  (lay->endframe - lay->startframe) * (lay->loopbox->scrcoords->w / (lay->numf - 1)), lay->loopbox->scrcoords->y1, lay->loopbox->scrcoords->x1 + lay->startframe * (lay->loopbox->scrcoords->w / (lay->numf - 1)) +  (lay->endframe - lay->startframe) * (lay->loopbox->scrcoords->w / (lay->numf - 1)), lay->loopbox->scrcoords->y1 - mainprogram->yvtxtoscr(0.045f)) < 6) {
					ends = true;
					if (mainprogram->middlemousedown) {
						lay->scritching = 3;
						mainprogram->middlemousedown = false;
					}
				}
				else if (mainprogram->mx > lay->loopbox->scrcoords->x1 + lay->startframe * (lay->loopbox->scrcoords->w / (lay->numf - 1)) and mainprogram->mx  < lay->loopbox->scrcoords->x1 + lay->startframe * (lay->loopbox->scrcoords->w / (lay->numf - 1)) +  (lay->endframe - lay->startframe) * (lay->loopbox->scrcoords->w / (lay->numf - 1))) {
					if (mainprogram->middlemousedown) {
						mainmix->prevx = mainprogram->mx;
						lay->scritching = 5;
						mainprogram->middlemousedown = false;
					}
				}
			}
			if (lay->loopbox->in()) {
				if (mainprogram->leftmousedown) {
					lay->scritching = 1;
					lay->frame = (lay->numf - 1.0f) * ((mainprogram->mx - lay->loopbox->scrcoords->x1) / lay->loopbox->scrcoords->w);
					lay->set_clones();
				}
			}
			if (lay->scritching) mainprogram->leftmousedown = false;
			if (lay->scritching == 1) {
				lay->frame = (lay->numf - 1) * ((mainprogram->mx - lay->loopbox->scrcoords->x1) / lay->loopbox->scrcoords->w);
				if (lay->frame < 0) lay->frame = 0.0f;
				else if (lay->frame > lay->numf - 1) lay->frame = lay->numf - 1;
				lay->set_clones();
				if (mainprogram->leftmouse and !mainprogram->menuondisplay) lay->scritching = 4;
			}
			else if (lay->scritching == 2) {
				lay->startframe = (lay->numf - 1) * ((mainprogram->mx - lay->loopbox->scrcoords->x1) / lay->loopbox->scrcoords->w);
				if (lay->startframe < 0) lay->startframe = 0.0f;
				else if (lay->startframe > lay->numf - 1) lay->startframe = lay->numf - 1;
				if (lay->startframe > lay->frame) lay->frame = lay->startframe;
				if (lay->startframe > lay->endframe) lay->startframe = lay->endframe;
				lay->set_clones();
				if (mainprogram->middlemouse) lay->scritching = 0;
			}
			else if (lay->scritching == 3) {
				lay->endframe = (lay->numf - 1) * ((mainprogram->mx - lay->loopbox->scrcoords->x1) / lay->loopbox->scrcoords->w);
				if (lay->endframe < 0) lay->endframe = 0.0f;
				else if (lay->endframe > lay->numf - 1) lay->endframe = lay->numf - 1;
				//if (lay->endframe < lay->frame) lay->frame = lay->endframe;
				//if (lay->endframe < lay->startframe) lay->endframe = lay->startframe;
				lay->set_clones();
				if (mainprogram->middlemouse) lay->scritching = 0;
			}
			else if (lay->scritching == 5) {
				float start = 0.0f;
				float end = 0.0f;
				lay->startframe += (mainprogram->mx - mainmix->prevx) / (lay->loopbox->scrcoords->w / (lay->numf - 1));
				if (lay->startframe < 0) {
					start = lay->startframe - 1;
					lay->startframe = 0.0f;
				}
				else if (lay->startframe > lay->numf - 1) lay->startframe = lay->numf - 1;
				if (lay->startframe > lay->frame) lay->frame = lay->startframe;
				if (lay->startframe > lay->endframe) lay->startframe = lay->endframe;
				lay->endframe += (mainprogram->mx - mainmix->prevx) / (lay->loopbox->scrcoords->w / (lay->numf - 1)) - start;
				if (lay->endframe < 0) lay->endframe = 0.0f;
				else if (lay->endframe > lay->numf - 1) {
					end = lay->endframe - (lay->numf - 1);
					lay->startframe -= end;
					lay->endframe = lay->numf - 1;
				}
				mainmix->prevx = mainprogram->mx;
				//if (lay->endframe < lay->frame) lay->frame = lay->endframe;
				//if (lay->endframe < lay->startframe) lay->endframe = lay->startframe;
				lay->set_clones();
				if (mainprogram->middlemouse) lay->scritching = 0;
			}
			draw_box(lay->loopbox, -1);
			if (ends) {
				draw_box(white, lightblue, lay->loopbox->vtxcoords->x1 + lay->startframe * (lay->loopbox->vtxcoords->w / (lay->numf - 1)), lay->loopbox->vtxcoords->y1, (lay->endframe - lay->startframe) * (lay->loopbox->vtxcoords->w / (lay->numf - 1)), tf(0.05f), -1);
			}
			else {
				draw_box(white, green, lay->loopbox->vtxcoords->x1 + lay->startframe * (lay->loopbox->vtxcoords->w / (lay->numf - 1)), lay->loopbox->vtxcoords->y1, (lay->endframe - lay->startframe) * (lay->loopbox->vtxcoords->w / (lay->numf - 1)), tf(0.05f), -1);
			}
			draw_box(white, white, lay->loopbox->vtxcoords->x1 + lay->frame * (lay->loopbox->vtxcoords->w / (lay->numf - 1)), lay->loopbox->vtxcoords->y1, tf(0.00078f), tf(0.05f), -1);
			
			// Draw and handle chdir chinv
			if (lay->pos > 0) {
				if (lay->blendnode->blendtype == COLOURKEY) {
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
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glViewport(0, 0, glob->w, glob->h);
}

void midi_set() {
	if (mainmix->midibutton) {
		Button *but = mainmix->midibutton;
		but->value = (int)(mainmix->midi2 / 64);
		if (but == mainprogram->wormhole) mainprogram->binsscreen = but->value;
		mainmix->midibutton = nullptr;
	}
	
	if (mainmix->midiparam) {
		printf("MIDIPARAM\n");
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
	GLuint fcdiv = glGetUniformLocation(mainprogram->ShaderProgram, "fcdiv");
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
						doblur(stage, prevfbotex, ((BlurEffect*)effect)->times);
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
						doblur(stage, prevfbotex, 6);
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
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, mainprogram->ow3, mainprogram->oh3, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
				}
				else {
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, mainprogram->ow, mainprogram->oh, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
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
				glDrawBuffer(GL_COLOR_ATTACHMENT0);
				glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
				glClear(GL_COLOR_BUFFER_BIT);
				float black[4] = {0.0f, 0.0f, 0.0f, 0.0f};
				draw_box(nullptr, black, -1.0f, 1.0f, 2.0f, -2.0f, lay->shiftx->value, lay->shifty->value, lay->scale->value, lay->opacity->value, 0, fbocopy, 0, 0);
				glEnable(GL_BLEND);
				glDeleteTextures(1, &fbocopy);
			}
			
			glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			glViewport(0, 0, glob->w, glob->h);
		}
	}
	else if (node->type == VIDEO) {
		Layer *lay = ((VideoNode*)node)->layer;
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
		if (lay->node == lay->lasteffnode[0]) {
			GLuint fbocopy;
			glDisable(GL_BLEND);
			int sw, sh;
			glBindTexture(GL_TEXTURE_2D, lay->fbotex);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &sw);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &sh);
			fbocopy = copy_tex(lay->fbotex, sw, sh);
			glViewport(0, 0, sw, sh);
			glBindFramebuffer(GL_FRAMEBUFFER, lay->fbo);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			float black[4] = {0.0f, 0.0f, 0.0f, 0.0f};
			draw_box(nullptr, black, -1.0f, 1.0f, 2.0f, -2.0f, lay->shiftx->value, lay->shifty->value, lay->scale->value, lay->opacity->value, 0, fbocopy, 0, 0);
			glEnable(GL_BLEND);
			glDeleteTextures(1, &fbocopy);
			glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			glViewport(0, 0, glob->w, glob->h);
		}
		prevfbotex = lay->fbotex;
		prevfbo = lay->fbo;
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
						glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, mainprogram->ow3, mainprogram->oh3, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
					}
					else {	
						glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, mainprogram->ow, mainprogram->oh, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
					}
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		
					glGenFramebuffers(1, &(bnode->fbo));
					glBindFramebuffer(GL_FRAMEBUFFER, bnode->fbo);
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bnode->fbotex, 0);
				}
				
				glBindFramebuffer(GL_FRAMEBUFFER, bnode->fbo);
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
					glUniform1f(xpos, bnode->wipex);
					GLfloat ypos = glGetUniformLocation(mainprogram->ShaderProgram, "ypos");
					glUniform1f(ypos, bnode->wipey);
				}
				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D, bnode->in2tex);
				glBindVertexArray(mainprogram->fbovao);
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
				glDrawBuffer(GL_COLOR_ATTACHMENT0);
				glViewport(0, 0, glob->w, glob->h);
			}
			else {
				if (prevnode == bnode->in2) {
					node->walked = true;			//for when first layer is muted
					glBindFramebuffer(GL_FRAMEBUFFER, bnode->fbo);
					glDrawBuffer(GL_COLOR_ATTACHMENT0);
					if (stage) glViewport(0, 0, mainprogram->ow, mainprogram->oh);
					else glViewport(0, 0, mainprogram->ow3, mainprogram->oh3);
					glBindTexture(GL_TEXTURE_2D, prevfbotex);
					glBindVertexArray(mainprogram->fbovao);
					glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
					glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
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
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, mainprogram->ow3, mainprogram->oh3, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
			}
			else {
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, mainprogram->ow, mainprogram->oh, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
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
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glViewport(0, 0, glob->w, glob->h);
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
					if (node->monitor / 6 == mainmix->currscene[0]) {
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
			onestepfrom(0, lay->node, nullptr, -1, -1);
		}
		for (int i = 0; i < mainmix->layersB.size(); i++) {
			Layer *lay = mainmix->layersB[i];
			onestepfrom(0, lay->node, nullptr, -1, -1);
		}
	}
	else {
		for (int i = 0; i < mainmix->layersAcomp.size(); i++) {
			Layer *lay = mainmix->layersAcomp[i];
			onestepfrom(1, lay->node, nullptr, -1, -1);
		}
		for (int i = 0; i < mainmix->layersBcomp.size(); i++) {
			Layer *lay = mainmix->layersBcomp[i];
			onestepfrom(1, lay->node, nullptr, -1, -1);
		}
	}
}		
		

bool display_mix() {
	float xs = 0.0f;
	float ys = 0.0f;
	float fraco = mainprogram->ow / mainprogram->oh;
	float frachd = 1920.0f / 1080.0f;
	if (fraco > frachd) {
		ys = 0.15f * (1.0f - frachd / fraco);
	}
	else {
		xs = 0.15f * (1.0f - fraco / frachd);
	}
	MixNode *node;
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
		box = mainprogram->mainmonitor;
		draw_box(node->outputbox->lcolor, node->outputbox->acolor, box, -1);
		draw_box(nullptr, node->outputbox->acolor, box->vtxcoords->x1 + xs * 2.0f, box->vtxcoords->y1 + ys * 2.0f, box->vtxcoords->w - xs * 4.0f, box->vtxcoords->h - ys * 4.0f, node->mixtex);
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
		box = mainprogram->outputmonitor;
		draw_box(node->outputbox->lcolor, node->outputbox->acolor, box, -1);
		draw_box(nullptr, node->outputbox->acolor, box->vtxcoords->x1 + xs, box->vtxcoords->y1 + ys, box->vtxcoords->w - xs * 2.0f, box->vtxcoords->h - ys * 2.0f, node->mixtex);
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
		box = mainprogram->mainmonitor;
		draw_box(node->outputbox->lcolor, node->outputbox->acolor, box, -1);
		draw_box(nullptr, node->outputbox->acolor, box->vtxcoords->x1 + xs * 2.0f, box->vtxcoords->y1 + ys * 2.0f, box->vtxcoords->w - xs * 4.0f, box->vtxcoords->h - ys * 4.0f, node->mixtex);
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
		draw_box(nullptr, node->outputbox->acolor, box->vtxcoords->x1 + xs, box->vtxcoords->y1 + ys, box->vtxcoords->w - xs * 2.0f, box->vtxcoords->h - ys * 2.0f, node->mixtex);
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
		draw_box(nullptr, node->outputbox->acolor, box->vtxcoords->x1 + xs, box->vtxcoords->y1 + ys, box->vtxcoords->w - xs * 2.0f, box->vtxcoords->h - ys * 2.0f, node->mixtex);
		mainprogram->deckmonitor[1]->in();
	}
	
	return true;
}


bool handle_clips(std::vector<Layer*> &layers, bool deck) {
	float white[] = {1.0f, 1.0f, 1.0f, 1.0f};
	float transp[] = {0.0f, 0.0f, 0.0f, 0.0f};
	float black[] = {0.0f, 0.0f, 0.0f, 1.0f};
	float lightblue[] = {0.5f, 0.5f, 1.0f, 1.0f};

	Layer *lay;
	for (int i = 0; i < layers.size(); i++) {
		lay = layers[i];
		if (lay->pos < mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos or lay->pos > mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos + 2) continue;
		Box *box = lay->node->vidbox;
		int endx = false;
		if ((i == layers.size() - 1 or i == mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos + 2) and (box->scrcoords->x1 + box->scrcoords->w - mainprogram->xvtxtoscr(tf(0.075f)) < mainprogram->mx and mainprogram->mx < box->scrcoords->x1 + box->scrcoords->w + mainprogram->xvtxtoscr(0.075f))) {
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
					printf("IN2\n");
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
							else if (mainprogram->dragbinel->type == ELEM_FILE) {
								open_video(0, lay, mainprogram->dragbinel->path, true);
							}
							else if (mainprogram->dragbinel->type == ELEM_IMAGE) {
								lay->open_image(mainprogram->dragbinel->path);
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
						if (inlay->pos == mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos + 3) mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos++;
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

		
			int numonscreen = layers.size() - mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos;
			if (0 <= numonscreen and numonscreen <= 2) {
				if (mainprogram->xvtxtoscr(mainprogram->numw + deck * 1.0f + numonscreen * mainprogram->layw) < mainprogram->mx and mainprogram->mx < deck * (glob->w / 2.0f) + glob->w / 2.0f) {
					if (0 < mainprogram->my and mainprogram->my < mainprogram->yvtxtoscr(mainprogram->layh)) {
						float blue[] = {0.5, 0.5 , 1.0, 1.0};
						draw_box(nullptr, blue, -1.0f + mainprogram->numw + deck * 1.0f + numonscreen * mainprogram->layw, 1.0f - mainprogram->layh, mainprogram->layw, mainprogram->layh, -1);
						if (mainprogram->leftmouse) {
							mainprogram->leftmouse = false;
							lay = mainmix->add_layer(layers, layers.size());
							if (mainprogram->dragbinel) {
								if (mainprogram->dragbinel->type == ELEM_LAYER) {
									mainmix->open_layerfile(mainprogram->dragbinel->path, lay, 1, 1);
								}
								else if (mainprogram->dragbinel->type == ELEM_FILE) {
									open_video(0, lay, mainprogram->dragbinel->path, true);
								}
								else if (mainprogram->dragbinel->type == ELEM_IMAGE) {
									lay->open_image(mainprogram->dragbinel->path);
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
void Shelf::handle() {
	// draw shelves and handle shelves
	for (int i = 0; i < 16; i++) {
		draw_box(this->buttons[i]->box, this->texes[i]);
		if (this->buttons[i]->box->in()) {
			if (mainprogram->leftmousedown) {
				if (!mainprogram->dragbinel) {
					// user starts dragging shelf element
					mainprogram->shelfdrag = true;
					mainprogram->leftmousedown = false;
					mainprogram->dragbinel = new BinElement;
					mainprogram->dragbinel->path = this->paths[i];
					mainprogram->dragbinel->type = this->types[i];
					mainprogram->dragbinel->tex = this->texes[i];
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
							newpath = mainprogram->shelfdir + name + ".layer";
							if (!exists(newpath)) {
								boost::filesystem::copy_file(mainprogram->dragbinel->path, newpath);
								mainprogram->dragbinel->path = newpath;
								break;
							}
							count++;
							name = remove_version(name) + "_" + std::to_string(count);
						}
					}
					this->paths[i] = mainprogram->dragbinel->path;
					this->types[i] = mainprogram->dragbinel->type;
					this->texes[i] = copy_tex(mainprogram->dragbinel->tex);
					enddrag();
				}
			}
			else if (mainprogram->menuactivation) {
				mainprogram->shelfmenu->state = 2;
				mainmix->mouseshelf = this;
				mainmix->mouseshelfelem = i;
				mainprogram->menuactivation = false;
			}
		}
	}
}


void clip_dragging() {
	//handle clip dragging
	if (mainprogram->dragbinel) {
		if (mainprogram->prevmodus) {
			if (handle_clips(mainmix->layersA, 0)) return;
			if (handle_clips(mainmix->layersB, 1)) return;
		}
		else {
			if (handle_clips(mainmix->layersAcomp, 0)) return;
			if (handle_clips(mainmix->layersBcomp, 1)) return;
		}
		if (mainprogram->leftmouse) {
			mainprogram->leftmouse = false;
			enddrag();
		}
	}
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
					vnode->vidbox->vtxcoords->x1 = -1.0f + (float)((vnode->layer->pos - mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos + 9999) % 3) * mainprogram->layw + mainprogram->numw + xoffset;
					vnode->vidbox->vtxcoords->y1 = 1.0f - mainprogram->layh;
					vnode->vidbox->vtxcoords->w = mainprogram->layw;
					vnode->vidbox->vtxcoords->h = mainprogram->layh;
					vnode->vidbox->upvtxtoscr();
					
					vnode->vidbox->lcolor[0] = 1.0;
					vnode->vidbox->lcolor[1] = 1.0;
					vnode->vidbox->lcolor[2] = 1.0;
					vnode->vidbox->lcolor[3] = 1.0;
					if (vnode->layer->effects[0].size() == 0) {
						vnode->vidbox->tex = vnode->layer->fbotex;
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
					VideoNode *vnode = (VideoNode*)node;
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
					vnode->vidbox->vtxcoords->x1 = -1.0f + (float)((vnode->layer->pos - mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos + 9999) % 3) * mainprogram->layw + mainprogram->numw + xoffset;
					vnode->vidbox->vtxcoords->y1 = 1.0f - mainprogram->layh;
					vnode->vidbox->vtxcoords->w = mainprogram->layw;
					vnode->vidbox->vtxcoords->h = mainprogram->layh;
					vnode->vidbox->upvtxtoscr();
					
					vnode->vidbox->lcolor[0] = 1.0;
					vnode->vidbox->lcolor[1] = 1.0;
					vnode->vidbox->lcolor[2] = 1.0;
					vnode->vidbox->lcolor[3] = 1.0;
					if ((vnode)->layer->effects[0].size() == 0) {
						vnode->vidbox->tex = vnode->layer->fbotex;
					}
					else {
						vnode->vidbox->tex = vnode->layer->effects[0][vnode->layer->effects[0].size() - 1]->fbotex;
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
				xoffset = 1.0f + mainprogram->layw - 0.06f;
			}
			else if (j == 2) {
				lvec = mainmix->layersAcomp;
				xoffset = 0.0f;
			}
			else if (j == 3) {
				lvec = mainmix->layersBcomp;
				xoffset = 1.0f + mainprogram->layw - 0.019f;
			}
			for (int i = 0; i < lvec.size(); i++) {
				Layer *testlay = lvec[i];
				testlay->node->upeffboxes();
				// Make mixbox
				testlay->mixbox->vtxcoords->x1 = -1.0f + mainprogram->numw + xoffset + (testlay->pos % 3) * tf(0.04f);
				testlay->mixbox->vtxcoords->y1 = 1.0f - tf(mainprogram->layh);
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
					if (testlay->deck == 0) {
						eff->box->vtxcoords->x1 = -1.0f + mainprogram->numw + tf(0.05f);
						eff->onoffbutton->box->vtxcoords->x1 = -1.0f + mainprogram->numw + tf(0.025f);
						eff->drywet->box->vtxcoords->x1 =  -1.0f + mainprogram->numw;
					}
					else {
						xoffset = 1.0f + mainprogram->layw - 0.019f;
						eff->box->vtxcoords->x1 = -1.0f + mainprogram->numw + tf(0.05f) + xoffset;
						eff->onoffbutton->box->vtxcoords->x1 = -1.0f + mainprogram->numw + tf(0.025f) + xoffset;
						eff->drywet->box->vtxcoords->x1 =  -1.0f + mainprogram->numw + xoffset;
					}
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
						eff->box->vtxcoords->y1 = 1.0 - tf(mainprogram->layh) - tf(0.20f) + (tf(0.05f) * testlay->effscroll[mainprogram->effcat[testlay->deck]->value]);
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
	delete this->vtxcoords;
	delete this->scrcoords;
}

Button::Button(bool state) {
	this->box = new Box;
	this->value = state;
	this->ccol[3] = 1.0f;
	if (mainprogram) mainprogram->buttons.push_back(this);
}

Button::~Button() {
	delete this->box;
}

void Button::handle(bool circlein) {
	if (this->box->in()) {
		if (mainprogram->leftmouse) {
			this->oldvalue = this->value;
			this->value = !this->value;
		}
		if (mainprogram->menuactivation and !mainprogram->menuondisplay) {
			mainprogram->parammenu1->state = 2;
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
	if (this->scrcoords->x1 < mx and mx < this->scrcoords->x1 + this->scrcoords->w) {
		if (this->scrcoords->y1 - this->scrcoords->h < my and my < this->scrcoords->y1) {
			if (mainprogram->showtooltips and mainprogram->tooltipbox == nullptr and this->tooltiptitle != "") mainprogram->tooltipbox = this;
			return true;
		}
	}
	return false;
}


void tooltips_handle(float fac) {
	// draw tooltip
	float black[] = {0.0f, 0.0f, 0.0f, 1.0f};
	float orange[] = {1.0f, 0.5f, 0.0f, 1.0f};
	float white[] = {1.0f, 1.0f, 1.0f, 1.0f};
	if (mainprogram->tooltipmilli > 3000) {
		std::vector<std::string> texts;
		int pos = 0;
		while (pos < mainprogram->tooltipbox->tooltip.length() - 1) {
			int oldpos = pos;
			pos = mainprogram->tooltipbox->tooltip.rfind(" ", pos + 58);
			texts.push_back(mainprogram->tooltipbox->tooltip.substr(oldpos, pos - oldpos));
		}
		float x = mainprogram->tooltipbox->vtxcoords->x1 + mainprogram->tooltipbox->vtxcoords->w + tf(0.01f);  //first print offscreen
		float y = mainprogram->tooltipbox->vtxcoords->y1 - tf(0.01f) * glob->w / glob->h - tf(0.01f);
		float textw = 0.5f * sqrt(fac);
		float texth = 0.092754f * sqrt(fac);
		if ((x + textw) > 1.0f) x = x - textw - tf(0.02f) - mainprogram->tooltipbox->vtxcoords->w;
		if ((y - texth * (texts.size() + 1) - tf(0.01f)) < -1.0f) y = -1.0f + texth * (texts.size() + 1) - tf(0.01f);
		draw_box(nullptr, black, x, y - texth, textw, texth + tf(0.01f), -1);
		render_text(mainprogram->tooltipbox->tooltiptitle, orange, x + tf(0.015f) * sqrt(fac), y - texth + tf(0.03f) * sqrt(fac), tf(0.0003f) * fac, tf(0.0005f) * fac);
		for (int i = 0; i < texts.size(); i++) {
			y -= texth;
			draw_box(nullptr, black, x, y - texth, textw, texth + tf(0.01f), -1);
			render_text(texts[i], white, x + tf(0.015f) * sqrt(fac), y - texth + tf(0.03f) * sqrt(fac), tf(0.0003f) * fac, tf(0.0005f) * fac);
		}
	}
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
	if (istring == "EWOCvj PREFERENCES V0.1") {
		// reminder: ENTER OLD LOAD CODE HERE
	}
	else {
		while (getline(rfile, istring)) {
			if (istring == "ENDOFFILE") break;
		
			else if (istring == "PREFCAT") {
				while (getline(rfile, istring)) {
					if (istring == "ENDOFPREFCAT") break;
					bool brk = false;
					for (int i = 0; i < mainprogram->prefs->items.size(); i++) {
						if (mainprogram->prefs->items[i]->name == istring) {
							std::string catname = istring;
							if (istring == "MIDI Devices") ((PIMidi*)mainprogram->prefs->items[i])->populate();
							while (getline(rfile, istring)) {
								if (istring == "ENDOFPREFCAT") {
									brk = true;
									break;
								}
								int foundpos = -1;
								PrefItem *pi;
								for (int j = 0; j < mainprogram->prefs->items[i]->items.size(); j++) {
									pi = mainprogram->prefs->items[i]->items[j];
									if (pi->name == istring) {
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
										printf("istring \n", istring.c_str());
										fflush(stdout);
										bool onoff = std::stoi(istring);
										PMidiItem *pmi = new PMidiItem(mainprogram->prefs->items[i], mainprogram->prefs->items[i]->items.size(), name, PREF_ONOFF, nullptr);
										pmi->onoff = onoff;
										pmi->connected = false;
									}
									else {
										PIMidi *pim = (PIMidi*)mainprogram->prefs->items[i];
										if (!pi->onoff) {
											if (std::find(pim->onnames.begin(), pim->onnames.end(), pi->name) != pim->onnames.end()) {
												pim->onnames.erase(std::find(pim->onnames.begin(), pim->onnames.end(), pi->name));
												mainprogram->openports.erase(std::find(mainprogram->openports.begin(), mainprogram->openports.end(), foundpos));
												((PMidiItem*)pi)->midiin->cancelCallback();
												delete ((PMidiItem*)pi)->midiin;
											}
										}
										else {
											printf("AAN\n");
											pim->onnames.push_back(pi->name);
											RtMidiIn *midiin = new RtMidiIn();
											if (std::find(mainprogram->openports.begin(), mainprogram->openports.end(), foundpos) == mainprogram->openports.end()) {
												printf("OPEN\n");
												midiin->setCallback(&mycallback, (void*)pim->items[foundpos]);
												midiin->openPort(foundpos);
											}
											mainprogram->openports.push_back(foundpos);
											((PMidiItem*)pi)->midiin = midiin;
										}
										fflush(stdout);
									}
								}
							}
							if (brk) break;
						}
					}
					if (brk) break;
				}
			}
		}
	}
	
	mainprogram->set_ow3oh3();
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
	wfile << "EWOCvj PREFERENCES V0.2\n";
	
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
	pdi->path = "./projects/";
	#else
	#ifdef __linux__
	pdi->path = homedir + "/.ewocvj2/projects/";
	#endif
	#endif
	mainprogram->projdir = pdi->path;
	this->items.push_back(pdi);
	pos++;
	
	pdi = new PrefItem(this, pos, "General mediabins", PREF_PATH, (void*)&mainprogram->binsdir);
	pdi->namebox->tooltiptitle = "General mediabins directory ";
	pdi->namebox->tooltip = "This directory contains general mediabins, that can be accessed from every project. ";
	pdi->valuebox->tooltiptitle = "Set general mediabins directory ";
	pdi->valuebox->tooltip = "Leftclick starts keyboard entry of location of general mediabins directory. ";
	pdi->iconbox->tooltiptitle = "Browse to set general mediabins directory ";
	pdi->iconbox->tooltip = "Leftclick allows browsing for location of general mediabins directory. ";
	#ifdef _WIN64
	pdi->path = "./bins/";
	#else
	#ifdef __linux__
	std::string homedir (getenv("HOME"));
	pdi->path = homedir + "/.ewocvj2/bins/";
	#endif
	#endif
	mainprogram->binsdir = pdi->path;
	this->items.push_back(pdi);
	pos++;
	
	pdi = new PrefItem(this, pos, "General shelves", PREF_PATH, (void*)&mainprogram->shelfdir);
	pdi->namebox->tooltiptitle = "General shelves directory ";
	pdi->namebox->tooltip = "This directory contains general shelves, that can be accessed from every project. ";
	pdi->valuebox->tooltiptitle = "Set general shelves directory ";
	pdi->valuebox->tooltip = "Leftclick starts keyboard entry of location of general shelves directory. ";
	pdi->iconbox->tooltiptitle = "Browse to set general shelves directory ";
	pdi->iconbox->tooltip = "Leftclick allows browsing for location of general shelves directory. ";
	#ifdef _WIN64
	pdi->path = "./shelves/";
	# else
	#ifdef __linux__
	std::string homedir (getenv("HOME"));
	pdi->path = homedir + "/.ewocvj2/shelves/";
	#endif
	#endif
	mainprogram->shelfdir = pdi->path;
	this->items.push_back(pdi);
	pos++;

	pdi = new PrefItem(this, pos, "Recordings", PREF_PATH, (void*)&mainprogram->recdir);
	pdi->namebox->tooltiptitle = "Recordings directory ";
	pdi->namebox->tooltip = "Video file recordings of the program output stream are saved in this directory. ";
	pdi->valuebox->tooltiptitle = "Set recordings directory ";
	pdi->valuebox->tooltip = "Leftclick starts keyboard entry of location of recordings directory. ";
	pdi->iconbox->tooltiptitle = "Browse to set recordings directory ";
	pdi->iconbox->tooltip = "Leftclick allows browsing for location of recordings directory. ";
	#ifdef _WIN64
	pdi->path = "./recordings/";
	#else
	#ifdef __linux__
	pdi->path = homedir + "/.ewocvj2/recordings/";
	#endif
	#endif
	mainprogram->recdir = pdi->path;
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
	pdi->path = "./autosaves/";
	#else
	#ifdef __linux__
	pdi->path = homedir + "/.ewocvj2/autosaves/";
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
	RtMidiIn midiin;
	int nPorts = midiin.getPortCount();
	std::string portName;
	for (int i = 0; i < this->items.size(); i++) {
		delete this->items[i];
	}
	this->items.clear();
	for (int i = 0; i < nPorts; i++) {
		PMidiItem *pmi = new PMidiItem(this, i, midiin.getPortName(i), PREF_ONOFF, nullptr);
		pmi->onoff = (std::find(this->onnames.begin(), this->onnames.end(), pmi->name) != this->onnames.end());
		pmi->namebox->tooltiptitle = "MIDI device ";
		pmi->namebox->tooltip = "Name of a connected MIDI device. ";
		pmi->valuebox->tooltiptitle = "MIDI device on/off ";
		pmi->valuebox->tooltip = "Leftclicking toggles if this MIDI device is used by EWOCvj2. ";
		this->items.push_back(pmi);
	}
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

	pii = new PrefItem(this, pos, "Show tooltips", PREF_ONOFF, (void*)&mainprogram->showtooltips);
	pii->onoff = 1;
	pii->namebox->tooltiptitle = "Show tooltips toggle ";
	pii->namebox->tooltip = "Toggles if tooltips will be shown when hovering mouse over an interface element. ";
	pii->valuebox->tooltiptitle = "Show tooltips toggle ";
	pii->valuebox->tooltip = "Toggles if tooltips will be shown when hovering mouse over an interface element. ";
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


void handle_scenes(Scene *scene) {
	// Draw scene boxes
	float white[] = {1.0, 1.0, 1.0, 1.0};
	float red[] = {1.0, 0.5, 0.5, 1.0};
	for (int i = 4; i >= 0; i--) {
		Box *box = mainmix->scenes[scene->deck][i]->box;
		if (i == mainmix->currscene[scene->deck] + 1) {
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
	for (int i = 0; i < 5; i++) {
		Box *box = mainmix->scenes[scene->deck][i]->box;
		Button *but = mainmix->scenes[scene->deck][i]->button;
		box->acolor[0] = 0.0;
		box->acolor[1] = 0.0;
		box->acolor[2] = 0.0;
		box->acolor[3] = 1.0;
		if (box->in() or but->value) {
			if ((mainprogram->leftmouse and !mainmix->moving and !mainprogram->menuondisplay) or but->value) {
				but->value = false;
				if (i != 0) {
					std::string comp = "";
					if (!mainprogram->prevmodus) comp = "comp";
					// switch scenes
					Scene *si = mainmix->scenes[scene->deck][i - 1];
					if (i == mainmix->currscene[scene->deck] + 1) continue;
					scene->tempnbframes.clear();
					for (int j = 0; j < si->nbframes.size(); j++) {
						si->tempnbframes.push_back(si->nbframes[j]);
					}
					si->nbframes.clear();
					std::vector<Layer*> &lvec = choose_layers(0);
					scene->nbframes.clear();
					for (int j = 0; j < lvec.size(); j++) {
						// store layers of current scene into nbframes for running their framecounters in the background (to keep sync)
						scene->nbframes.push_back(lvec[j]);
					}
					mainmix->mousedeck = scene->deck;
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
					mainmix->open_deck(mainprogram->temppath + "tempdeck_" + std::to_string(scene->deck) + std::to_string(i) + comp + ".deck", 0);
					boost::filesystem::rename(mainprogram->temppath + "tempdeck_xch.deck", mainprogram->temppath + "tempdeck_" + std::to_string(scene->deck) + std::to_string(mainmix->currscene[scene->deck] + 1) + comp + ".deck");
					std::vector<Layer*> &lvec2 = choose_layers(scene->deck);
					for (int j = 0; j < lvec2.size(); j++) {
						// set layer frame to (running in background) nbframes->frame of loaded scene
						lvec2[j]->frame = si->tempnbframes[j]->frame;
					}
					mainmix->currscene[scene->deck] = i - 1;
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
		
		std::string s = std::to_string(i);
		std::string pchar;
		if (i == 0 and scene->deck == 0) pchar = "A";
		else if (i == 0 and scene->deck == 1) pchar = "B";
		else pchar = s;
		if (mainmix->learnbutton == but and mainmix->learn) pchar = "M";
		if (i == 0) {
			render_text(pchar, red, box->vtxcoords->x1 + 0.01f, box->vtxcoords->y1 + 0.025f, 0.0006f, 0.001f);
		}
		else {
			render_text(pchar, white, box->vtxcoords->x1 + 0.01f, box->vtxcoords->y1 + 0.025f, 0.0006f, 0.001f);
		}	
	}
}

// regulates the reordering of layers around the stacks by dragging
// both exchanging two layers and moving one layer
void exchange(Layer *lay, std::vector<Layer*> &slayers, std::vector<Layer*> &dlayers, bool deck) {
	int size = dlayers.size();
	for (int i = 0; i < size; i++) {
		Layer *inlay = dlayers[i];
		if (inlay->pos < mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos or inlay->pos > mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos + 2) continue;
		Box *box = inlay->node->vidbox;
		int endx = 0;
		if ((i == dlayers.size() - 1 - mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos) and (box->scrcoords->x1 + box->scrcoords->w - mainprogram->xvtxtoscr(tf(0.075f)) < mainprogram->mx and mainprogram->mx < box->scrcoords->x1 + box->scrcoords->w + mainprogram->xvtxtoscr(0.075f) * (i != dlayers.size() - 1 - mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos))) {
			endx = 1;
		}
		bool dropin = false;
		int numonscreen = size - mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos;
		if (0 <= numonscreen and numonscreen <= 2) {
			if (mainprogram->xvtxtoscr(mainprogram->numw + deck * 1.0f + numonscreen * mainprogram->layw) < mainprogram->mx and mainprogram->mx < deck * (glob->w / 2.0f) + glob->w / 2.0f) {
				if (0 < mainprogram->my and mainprogram->my < mainprogram->yvtxtoscr(mainprogram->layh)) {
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
				Node *len1 = inlay->lasteffnode[1];
				
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
				slayers[lay->pos]->lasteffnode[1] = lay->lasteffnode[1];
				
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
				dlayers[lay->pos]->lasteffnode[1] = len1;
				
				Node *inlayout = inlay->lasteffnode[0]->out[0];
				Node *layout = lay->lasteffnode[0]->out[0];
				if (inlay->effects[0].size()) {
					inlay->node->out.clear();
					mainprogram->nodesmain->currpage->connect_nodes(inlay->node, inlay->effects[0][0]->node);
					inlay->lasteffnode[0] = inlay->effects[0][inlay->effects[0].size() - 1]->node;
				}
				else inlay->lasteffnode[0] = inlay->node;
				inlay->lasteffnode[0]->out.clear();
				if (inlay->pos == 0) {
					layout->in = nullptr;
					mainprogram->nodesmain->currpage->connect_nodes(inlay->lasteffnode[0], layout);
				}
				else {
					((BlendNode*)(layout))->in2 = nullptr;
					mainprogram->nodesmain->currpage->connect_in2(inlay->lasteffnode[0], (BlendNode*)(layout));
				}	
				for (int j = 0; j < inlay->effects[0].size(); j++) {
					inlay->effects[0][j]->node->align = inlay->node;
				}
				if (lay->effects[0].size()) {
					lay->node->out.clear();
					mainprogram->nodesmain->currpage->connect_nodes(lay->node, lay->effects[0][0]->node);
					lay->lasteffnode[0] = lay->effects[0][lay->effects[0].size() - 1]->node;
				}
				else lay->lasteffnode[0] = lay->node;
				lay->lasteffnode[0]->out.clear();
				if (lay->pos == 0) {
					inlayout->in = nullptr;
					mainprogram->nodesmain->currpage->connect_nodes(lay->lasteffnode[0], inlayout);
				}
				else {
					((BlendNode*)(inlayout))->in2 = nullptr;
					mainprogram->nodesmain->currpage->connect_in2(lay->lasteffnode[0], (BlendNode*)inlayout);
				}			
				for (int j = 0; j < lay->effects[0].size(); j++) {
					lay->effects[0][j]->node->align = lay->node;
				}
				if (inlay->filename == "") {
					mainmix->delete_layer(slayers, inlay, false);
				}
				break;
			}
			else if (dropin or endx or (box->scrcoords->x1 - mainprogram->xvtxtoscr(0.075f) * (i - mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos != 0) < mainprogram->mx and mainprogram->mx < box->scrcoords->x1 + mainprogram->xvtxtoscr(0.075f))) {
				if (lay == dlayers[i]) return;
				//move one layer
				BLEND_TYPE nextbtype;
				float nextmfval;
				int nextwipetype, nextwipedir;
				float nextwipex, nextwipey;
				Layer *nextlay = nullptr;
				if (slayers.size() > lay->pos + 1) {
					nextlay = slayers[lay->pos + 1];
					nextbtype = nextlay->blendnode->blendtype;
				}
				lay->deck = deck;
				BLEND_TYPE btype = lay->blendnode->blendtype;
				BlendNode *firstbnode = (BlendNode*)dlayers[0]->lasteffnode[0]->out[0];
				Node *firstlasteffnode = dlayers[0]->lasteffnode[0];
				float mfval = lay->blendnode->mixfac->value;
				int wipetype = lay->blendnode->wipetype;
				int wipedir = lay->blendnode->wipedir;
				float wipex = lay->blendnode->wipex;
				float wipey = lay->blendnode->wipey;
				if (lay->pos > 0) {
					lay->blendnode->in->out.clear();
					mainprogram->nodesmain->currpage->connect_nodes(lay->blendnode->in, lay->lasteffnode[1]->out[0]);
					mainprogram->nodesmain->currpage->delete_node(lay->blendnode);
				}
				else if (i + endx != 0) {
					if (nextlay) {
						nextlay->lasteffnode[0]->out.clear();
						if (lay->effects[1].size()) {
							mainprogram->nodesmain->currpage->connect_nodes(nextlay->lasteffnode[0], nextlay->effects[1][0]->node);
						}
						else {
							nextlay->lasteffnode[1] = nextlay->lasteffnode[0];
						}
						nextlay->lasteffnode[1]->out.clear();
						mainprogram->nodesmain->currpage->connect_nodes(nextlay->lasteffnode[1], nextlay->blendnode->out[0]);
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
				if (lay->lasteffnode[0] == lay->node) lastisvid = true;
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
				if (dlayers[i + endx]->pos == mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos + 3) {
					mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos++;
				}
				dlayers[i + endx]->node = mainprogram->nodesmain->currpage->add_videonode(comp);
				dlayers[i + endx]->node->layer = dlayers[i + endx];
				for (int j = 0; j < dlayers[i + endx]->effects[0].size(); j++) {
					if (j == 0) mainprogram->nodesmain->currpage->connect_nodes(dlayers[i + endx]->node, dlayers[i + endx]->effects[0][j]->node);
					dlayers[i + endx]->effects[0][j]->node->align = dlayers[i + endx]->node;
				}
				if (lastisvid) dlayers[i + endx]->lasteffnode[0] = dlayers[i + endx]->node;
				if (dlayers[i + endx]->pos > 0) {
					Layer *prevlay = dlayers[dlayers[i + endx]->pos - 1];
					BlendNode *bnode = mainprogram->nodesmain->currpage->add_blendnode(MIXING, comp);
					bnode->blendtype = btype;
					dlayers[i + endx]->blendnode = bnode;
					dlayers[i + endx]->blendnode->mixfac->value = mfval;
					dlayers[i + endx]->blendnode->wipetype = wipetype;
					dlayers[i + endx]->blendnode->wipedir = wipedir;
					dlayers[i + endx]->blendnode->wipex = wipex;
					dlayers[i + endx]->blendnode->wipey = wipey;
					mainprogram->nodesmain->currpage->connect_nodes(prevlay->lasteffnode[1], dlayers[i + endx]->lasteffnode[0], dlayers[i + endx]->blendnode);
					if (dlayers[i + endx]->effects[1].size()) {
						dlayers[i + endx]->blendnode->out.clear();
						mainprogram->nodesmain->currpage->connect_nodes(dlayers[i + endx]->blendnode, dlayers[i + endx]->effects[1][0]->node);
					}
					else {
						dlayers[i + endx]->lasteffnode[1] = dlayers[i + endx]->blendnode;
					}
					if (dlayers[i + endx]->pos == dlayers.size() - 1 and mainprogram->nodesmain->mixnodes.size()) {
						if (&dlayers == &mainmix->layersA) {
							mainprogram->nodesmain->currpage->connect_nodes(dlayers[i + endx]->lasteffnode[1], mainprogram->nodesmain->mixnodes[0]);
						}
						else if (&dlayers == &mainmix->layersB) {
							mainprogram->nodesmain->currpage->connect_nodes(dlayers[i + endx]->lasteffnode[1], mainprogram->nodesmain->mixnodes[1]);
						}
						else if (&dlayers == &mainmix->layersAcomp) {
							mainprogram->nodesmain->currpage->connect_nodes(dlayers[i + endx]->lasteffnode[1], mainprogram->nodesmain->mixnodescomp[0]);
						}
						else if (&dlayers == &mainmix->layersBcomp) {
							mainprogram->nodesmain->currpage->connect_nodes(dlayers[i + endx]->lasteffnode[1], mainprogram->nodesmain->mixnodescomp[1]);
						}
					}
					else if (dlayers[i + endx]->pos < dlayers.size() - 1) {
						mainprogram->nodesmain->currpage->connect_nodes(dlayers[i + endx]->lasteffnode[1], dlayers[dlayers[i + endx]->pos + 1]->blendnode);
					}	
				}
				else {
					dlayers[i + endx]->blendnode = new BlendNode;
					//BlendNode *bnode = mainprogram->nodesmain->currpage->add_blendnode(MIXING, false);
					if (dlayers[i + endx]->effects[1].size()) {
						dlayers[i + endx]->lasteffnode[0]->out.clear();
						mainprogram->nodesmain->currpage->connect_nodes(dlayers[i + endx]->lasteffnode[0], dlayers[i + endx]->effects[1][0]->node);
					}
					else {
						dlayers[i + endx]->lasteffnode[1] = dlayers[i + endx]->lasteffnode[0];
					}
					Layer *nxlay = nullptr;
					if (dlayers.size() > 2) nxlay = dlayers[1];
					if (nxlay) {
						Node *cpn = nxlay->lasteffnode[1]->out[0];
						firstlasteffnode->out.clear();
						BlendNode *bnode = mainprogram->nodesmain->currpage->add_blendnode(MIXING, comp);
						nxlay->blendnode = bnode;
						dlayers[i + endx]->lasteffnode[1]->out.clear();
						mainprogram->nodesmain->currpage->connect_nodes(dlayers[i + endx]->lasteffnode[1], firstlasteffnode, bnode);
						if (nxlay->effects[1].size()) {
							mainprogram->nodesmain->currpage->connect_nodes(bnode, nxlay->effects[1][0]->node);
							nxlay->lasteffnode[1]->out.clear();
							mainprogram->nodesmain->currpage->connect_nodes(nxlay->lasteffnode[1], cpn);
						}
						else mainprogram->nodesmain->currpage->connect_nodes(bnode, cpn);
					}
					else {
						BlendNode *bnode = mainprogram->nodesmain->currpage->add_blendnode(MIXING, comp);
						dlayers[i + endx]->lasteffnode[1]->out.clear();
						dlayers[1]->blendnode = bnode;
						dlayers[1]->blendnode->blendtype = MIXING;
						mainprogram->nodesmain->currpage->connect_nodes(dlayers[i + endx]->lasteffnode[1], dlayers[1]->lasteffnode[0], bnode);
						dlayers[1]->lasteffnode[1] = bnode;
						if (&dlayers == &mainmix->layersA) {
							mainprogram->nodesmain->currpage->connect_nodes(bnode, mainprogram->nodesmain->mixnodes[0]);
						}
						else if (&dlayers == &mainmix->layersB) {
							mainprogram->nodesmain->currpage->connect_nodes(bnode, mainprogram->nodesmain->mixnodes[1]);
						}
						else if (&dlayers == &mainmix->layersAcomp) {
							mainprogram->nodesmain->currpage->connect_nodes(bnode, mainprogram->nodesmain->mixnodescomp[0]);
						}
						else if (&dlayers == &mainmix->layersBcomp) {
							mainprogram->nodesmain->currpage->connect_nodes(bnode, mainprogram->nodesmain->mixnodescomp[1]);
						}
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
						mainprogram->nodesmain->currpage->connect_nodes(newlay->lasteffnode[0], mainprogram->nodesmain->mixnodes[0]);
					}
					else if (&slayers == &mainmix->layersB) {
						mainprogram->nodesmain->currpage->connect_nodes(newlay->lasteffnode[0], mainprogram->nodesmain->mixnodes[1]);
					}
					else if (&slayers == &mainmix->layersAcomp) {
						mainprogram->nodesmain->currpage->connect_nodes(newlay->lasteffnode[0], mainprogram->nodesmain->mixnodescomp[0]);
					}
					else if (&slayers == &mainmix->layersBcomp) {
						mainprogram->nodesmain->currpage->connect_nodes(newlay->lasteffnode[0], mainprogram->nodesmain->mixnodescomp[1]);
					}
				}
				break;
			}
		}
		make_layboxes();
	}
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
					glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
					glReadBuffer(GL_COLOR_ATTACHMENT0);
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
	int ret = handle_menu(menu, 0.0f, 0.0f);
	return ret;
}
int handle_menu(Menu *menu, float xshift, float yshift) {
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
		if (mainprogram->menuy + mainprogram->yvtxtoscr(yshift) > glob->h - size * mainprogram->yvtxtoscr(tf(0.05f))) mainprogram->menuy = glob->h - size * mainprogram->yvtxtoscr(tf(0.05f)) + mainprogram->yvtxtoscr(yshift);
		if (size > 21) mainprogram->menuy = mainprogram->yvtxtoscr(mainprogram->layh) - mainprogram->yvtxtoscr(yshift);
		float vmx = (float)mainprogram->menux * 2.0 / glob->w;
		float vmy = (float)mainprogram->menuy * 2.0 / glob->h;
		float lc[] = {0.0, 0.0, 0.0, 1.0};
		float ac1[] = {0.3, 0.3, 0.3, 1.0};
		float ac2[] = {0.5, 0.5, 1.0, 1.0};
		int numsubs = 0;
		float limit = 1.0f;
		int notsubk = 0;
		for(int k = 0; k < menu->entries.size(); k++) {
			float xoff;
			int koff;
			if (notsubk > 20) {
				if (mainprogram->xscrtovtx(mainprogram->menux) > limit) xoff = -tf(0.195f) + xshift;
				else xoff = tf(0.195f) + xshift;
				koff = menu->entries.size() - 21;
			}
			else {
				xoff = 0.0f + xshift;
				koff = 0;
			}
			std::size_t sub = menu->entries[k].find("submenu");
			if (sub != 0) {
				notsubk++;
				if (menu->box->scrcoords->x1 + mainprogram->menux + mainprogram->xvtxtoscr(xoff) < mainprogram->mx and mainprogram->mx < menu->box->scrcoords->x1 + menu->box->scrcoords->w + mainprogram->menux + mainprogram->xvtxtoscr(xoff) and menu->box->scrcoords->y1 - menu->box->scrcoords->h + mainprogram->menuy + (k - koff - numsubs) * mainprogram->yvtxtoscr(tf(0.05f)) - mainprogram->yvtxtoscr(yshift) < mainprogram->my and mainprogram->my < menu->box->scrcoords->y1 + mainprogram->menuy + (k - koff - numsubs) * mainprogram->yvtxtoscr(tf(0.05f)) - mainprogram->yvtxtoscr(yshift)) {
					draw_box(lc, ac2, menu->box->vtxcoords->x1 + vmx + xoff, menu->box->vtxcoords->y1 - (k - koff - numsubs) * tf(0.05f) - vmy + yshift, tf(menu->width), tf(0.05f), -1);
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
					draw_box(lc, ac2, menu->box->vtxcoords->x1 + vmx + xoff, menu->box->vtxcoords->y1 - (k - koff - numsubs) * tf(0.05f) - vmy + yshift, tf(menu->width), tf(0.05f), -1);
				}
				else {
					draw_box(lc, ac1, menu->box->vtxcoords->x1 + vmx + xoff, menu->box->vtxcoords->y1 - (k - koff - numsubs) * tf(0.05f) - vmy + yshift, tf(menu->width), tf(0.05f), -1);
				}
				render_text(menu->entries[k], white, menu->box->vtxcoords->x1 + vmx + tf(0.0078f) + xoff, menu->box->vtxcoords->y1 - (k - koff - numsubs) * tf(0.05f) - vmy + yshift + tf(0.015f), tf(0.0003f), tf(0.0005f));
			}
			else {
				numsubs++;
				if (menu->currsub == k or (menu->box->scrcoords->x1 + mainprogram->menux + mainprogram->xvtxtoscr(xoff) < mainprogram->mx and mainprogram->mx < menu->box->scrcoords->x1 + menu->box->scrcoords->w + mainprogram->menux + mainprogram->xvtxtoscr(xoff) and menu->box->scrcoords->y1 - menu->box->scrcoords->h + mainprogram->menuy + (k - koff - numsubs + 1) * mainprogram->yvtxtoscr(tf(0.05f)) - mainprogram->yvtxtoscr(yshift) < mainprogram->my and mainprogram->my < menu->box->scrcoords->y1 + mainprogram->menuy + (k - koff - numsubs + 1) * mainprogram->yvtxtoscr(tf(0.05f)) - mainprogram->yvtxtoscr(yshift))) {
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
								float ys = (k - mainprogram->menulist[i]->entries.size() / 2.0f) * tf(-0.05f) + yshift;
								int ret = handle_menu(mainprogram->menulist[i], xs, ys);
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
	if (mainprogram->rightmouse) mainprogram->renaming = EDIT_CANCEL;
	
	mainprogram->insmall = true;
	float white[] = {1.0f, 1.0f, 1.0f, 1.0f};
	float black[] = {0.0f, 0.0f, 0.0f, 1.0f};
	float lightblue[] = {0.5f, 0.5f, 1.0f, 1.0f};
	float green[] = {0.0f, 0.7f, 0.0f, 1.0f};

	SDL_GL_MakeCurrent(mainprogram->prefwindow, glc_pr);
	glClear(GL_COLOR_BUFFER_BIT);
	
	for (int i = 0; i < mainprogram->prefs->items.size(); i++) {
		PrefCat *item = mainprogram->prefs->items[i];
		if (item->box->in(mx, my)) {	
			draw_box(white, lightblue, item->box, -1);
			if (mainprogram->lmsave and mainprogram->prefs->curritem != i) {
				mainprogram->renaming = EDIT_NONE;
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
	
	PrefCat *mci = mainprogram->prefs->items[mainprogram->prefs->curritem];
	if (mci->name == "MIDI Devices") ((PIMidi*)mci)->populate();
	for (int i = 0; i < mci->items.size(); i++) {
		if (mci->items[i]->type == PREF_ONOFF) {
			draw_box(white, black, mci->items[i]->namebox, -1);
			render_text(mci->items[i]->name, white, mci->items[i]->namebox->vtxcoords->x1 + 0.23f, mci->items[i]->namebox->vtxcoords->y1 + 0.06f, 0.0024f, 0.004f, 1);		
			if (mci->items[i]->valuebox->in(mx, my)) {
				draw_box(white, lightblue, mci->items[i]->valuebox, -1);
				if (mainprogram->lmsave) {
					mci->items[i]->onoff = !mci->items[i]->onoff;
					if (mci->name == "MIDI Devices") {
						PIMidi *midici = (PIMidi*)mci;
						if (!midici->items[i]->onoff) {
							if (std::find(midici->onnames.begin(), midici->onnames.end(), midici->items[i]->name) != midici->onnames.end()) {
								midici->onnames.erase(std::find(midici->onnames.begin(), midici->onnames.end(), midici->items[i]->name));
								((PMidiItem*)midici->items[i])->midiin->cancelCallback();
								delete ((PMidiItem*)midici->items[i])->midiin;
							}
						}
						else {
							midici->onnames.push_back(midici->items[i]->name);
							RtMidiIn *midiin = new RtMidiIn();
							midiin->setCallback(&mycallback, (void*)midici->items[i]);
							midiin->openPort(i);
							midiin->ignoreTypes( true, true, true );
							((PMidiItem*)midici->items[i])->midiin = midiin;
						}
					}
				}
			}
			else if (mci->items[i]->onoff) {
				draw_box(white, green, mci->items[i]->valuebox, -1);
			}
			else {
				draw_box(white, black, mci->items[i]->valuebox, -1);
			}
		}
		else if (mci->items[i]->type == PREF_NUMBER) {
			draw_box(white, black, mci->items[i]->namebox, -1);
			render_text(mci->items[i]->name, white, mci->items[i]->namebox->vtxcoords->x1 + 0.23f, mci->items[i]->namebox->vtxcoords->y1 + 0.06f, 0.0024f, 0.004f, 1);		
			draw_box(white, black, mci->items[i]->valuebox, -1);
			if (mci->items[i]->renaming) {
				if (mainprogram->renaming == EDIT_NONE) {
					 mci->items[i]->renaming = false;
					 try {
					 	mci->items[i]->value = std::stoi(mainprogram->inputtext);
					 }
					 catch (...) {}
				}
				else if (mainprogram->renaming == EDIT_CANCEL) {
					 mci->items[i]->renaming = false;
				}
				else {
					std::string part = mainprogram->inputtext.substr(0, mainprogram->cursorpos);
					float textw = render_text(part, white, mci->items[i]->valuebox->vtxcoords->x1 + 0.1f, mci->items[i]->valuebox->vtxcoords->y1 + 0.06f, 0.0024f, 0.004f, 1) * 0.5f;
					part = mainprogram->inputtext.substr(mainprogram->cursorpos, mainprogram->inputtext.length() - mainprogram->cursorpos);
					render_text(part, white, mci->items[i]->valuebox->vtxcoords->x1 + 0.102f + textw * 2, mci->items[i]->valuebox->vtxcoords->y1 + 0.06f, 0.0024f, 0.004f, 1);
					draw_line(white, mci->items[i]->valuebox->vtxcoords->x1 + 0.102f + textw * 2, mci->items[i]->valuebox->vtxcoords->y1 + 0.06f, mci->items[i]->valuebox->vtxcoords->x1 + 0.102f + textw * 2, mci->items[i]->valuebox->vtxcoords->y1 + tf(0.09f));
				}
			}
			else {
				render_text(std::to_string(mci->items[i]->value), white, mci->items[i]->valuebox->vtxcoords->x1 + 0.1f, mci->items[i]->valuebox->vtxcoords->y1 + 0.06f, 0.0024f, 0.004f, 1);
			}
			if (mci->items[i]->valuebox->in(mx, my)) {
				if (mainprogram->lmsave and mainprogram->renaming == EDIT_NONE) {
					mci->items[i]->renaming = true;
					mainprogram->renaming = EDIT_NUMBER;
					mainprogram->inputtext = std::to_string(mci->items[i]->value);
					mainprogram->cursorpos = mainprogram->inputtext.length();
					SDL_StartTextInput();
				}
			}
		}
		else if (mci->items[i]->type == PREF_PATH) {
			draw_box(white, black, mci->items[i]->namebox, -1);
			render_text(mci->items[i]->name, white, -0.5f + 0.1f, mci->items[i]->namebox->vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, 1);
			draw_box(white, black, mci->items[i]->valuebox, -1);
			if (mci->items[i]->renaming == false) {
				render_text(mci->items[i]->path, white, mci->items[i]->valuebox->vtxcoords->x1 + 0.1f, mci->items[i]->valuebox->vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, 1);
			}
			else {
				if (mainprogram->renaming == EDIT_NONE) {
					mci->items[i]->renaming = false;
				 	mci->items[i]->path = mainprogram->inputtext;
				}
				else if (mainprogram->renaming == EDIT_CANCEL) {
					 mci->items[i]->renaming = false;
				}
				else {
					std::string part = mainprogram->inputtext.substr(0, mainprogram->cursorpos);
					float textw = render_text(part, white, mci->items[i]->valuebox->vtxcoords->x1 + 0.1f, mci->items[i]->valuebox->vtxcoords->y1 + tf(0.03f), 0.0024f, 0.004f, 1) * 0.5f;
					part = mainprogram->inputtext.substr(mainprogram->cursorpos, mainprogram->inputtext.length() - mainprogram->cursorpos);
					render_text(part, white, mci->items[i]->valuebox->vtxcoords->x1 + 0.102f + textw * 2, mci->items[i]->valuebox->vtxcoords->y1 + tf(0.03f), 0.0024f, 0.004f, 1);
					draw_line(white, mci->items[i]->valuebox->vtxcoords->x1 + 0.102f + textw * 2, mci->items[i]->valuebox->vtxcoords->y1 + tf(0.028f), mci->items[i]->valuebox->vtxcoords->x1 + 0.102f + textw * 2, mci->items[i]->valuebox->vtxcoords->y1 + tf(0.09f));
				}
			}
			if (mci->items[i]->valuebox->in(mx, my)) {
				if (mainprogram->lmsave and mainprogram->renaming == EDIT_NONE) {
					mci->items[i]->renaming = true;
					mainprogram->renaming = EDIT_STRING;
					mainprogram->inputtext = mci->items[i]->path;
					mainprogram->cursorpos = mainprogram->inputtext.length();
					SDL_StartTextInput();
				}
			}
			draw_box(white, black, mci->items[i]->iconbox, -1);
			draw_box(white, black, mci->items[i]->iconbox->vtxcoords->x1 + 0.02f, mci->items[i]->iconbox->vtxcoords->y1 + 0.05f, 0.06f, 0.07f, -1);
			draw_box(white, black, mci->items[i]->iconbox->vtxcoords->x1 + 0.05f, mci->items[i]->iconbox->vtxcoords->y1 + 0.11f, 0.025f, 0.03f, -1);
			if (mci->items[i]->iconbox->in(mx, my)) {
				if (mainprogram->lmsave) {
					mci->items[i]->choosing = true;
					mainprogram->pathto = "CHOOSEDIR";
					std::string title = "Open " + mci->items[i]->name + " directory";
					std::thread filereq (&Program::get_dir, mainprogram, title.c_str());
					filereq.detach();
				}
			}
			if (mci->items[i]->choosing and mainprogram->choosedir != "") {
				mci->items[i]->path = mainprogram->choosedir;
				mainprogram->choosedir = "";
				mci->items[i]->choosing = false;
			}
		}
		
		mci->items[i]->namebox->in(mx, my); //trigger tooltip
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
			for (int i = 0; i < mci->items.size(); i++) {
				if (mci->items[i]->renaming) {
					mci->items[i]->renaming = false;
					break;
				}
			}
			mainprogram->renaming = EDIT_NONE;
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
			for (int i = 0; i < mci->items.size(); i++) {
				if (mci->items[i]->renaming) {
					if (mci->items[i]->type == PREF_PATH) {
						mci->items[i]->path = mainprogram->inputtext;
						SDL_StopTextInput();
						break;
					}
					if (mci->items[i]->type == PREF_NUMBER) {
						 try {
							mci->items[i]->value = std::stoi(mainprogram->inputtext);
						 }
						 catch (...) {}
						SDL_StopTextInput();
						break;
					}
				}
			}
			mainprogram->renaming = EDIT_NONE;
			mainprogram->prefs->save();
			mainprogram->prefs->load();
			mainprogram->prefon = false;
			mainprogram->drawnonce = false;
			SDL_HideWindow(mainprogram->prefwindow);
			SDL_RaiseWindow(mainprogram->mainwindow);
			return 0;
		}
	}
	render_text("SAVE", white, box.vtxcoords->x1 + 0.02f, box.vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, 1);
			
	tooltips_handle(4.0f);
		
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
	
	if (mainprogram->rightmouse) {
		mainprogram->tmlearn = TM_NONE;
		mainprogram->menuactivation = false;
	}
	
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
		//draw tune_midi screen
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
				mainprogram->tmscratch->in(mx, my);  //tooltip
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
	
	tooltips_handle(4.0f);
		
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
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
	glBindBuffer(GL_ARRAY_BUFFER, mwin->tbuf);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8, nullptr);

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
		
		if (1) node = (MixNode*)mainprogram->nodesmain->mixnodescomp[2];
		else node = (MixNode*)mainprogram->nodesmain->mixnodes[mwin->mixid];
		printf("%d %d\n", mwin->mixid, mainprogram->nodesmain->mixnodes.size());
		
		GLfloat cf = glGetUniformLocation(mainprogram->ShaderProgram, "cf");
		GLint wipe = glGetUniformLocation(mainprogram->ShaderProgram, "wipe");
		GLint mixmode = glGetUniformLocation(mainprogram->ShaderProgram, "mixmode");
		if (mainmix->wipe[mwin->mixid == 2] > -1) {
			if (mwin->mixid == 2) glUniform1f(cf, mainmix->crossfadecomp->value);
			else glUniform1f(cf, mainmix->crossfade->value);
			glUniform1i(wipe, 1);
			glUniform1i(mixmode, 18);
		}
		GLint down = glGetUniformLocation(mainprogram->ShaderProgram, "down");
		glUniform1i(down, 1);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, node->mixtex);
		glBindVertexArray(mwin->vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glUniform1i(down, 0);
		glUniform1i(wipe, 0);
		glUniform1i(mixmode, 0);
		mwin->syncendnow = true;
		while (mwin->syncendnow) {
			mwin->syncend.notify_one();
		}
		SDL_GL_SwapWindow(mwin->win);
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
		mainprogram->shelfdrag = false;
		//glDeleteTextures(1, &binsmain->dragtex);  maybe needs implementing in one case, check history
	}
}

void Layer::mute_handle() {
	// change node connections to accommodate for a mute layer
	if (this->mutebut->value) {
		if (this->pos > 0) {
			if (this->blendnode->in->out.size()) {
				this->blendnode->in->out.clear();
				mainprogram->nodesmain->currpage->connect_nodes(this->blendnode->in, this->lasteffnode[1]->out[0]);
			}
			else this->blendnode->out.clear();
		}
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
	float white[] = {1.0f, 1.0f, 1.0f, 1.0f};
	float halfwhite[] = {1.0f, 1.0f, 1.0f, 0.5f};
	float black[] = {0.0f, 0.0f, 0.0f, 1.0f};
	float red[] = {1.0f, 0.5f, 0.5f, 1.0f};
	float green[] = {0.0f, 0.75f, 0.0f, 1.0f};
	float orange[] = {1.0f, 0.5f, 0.0f, 1.0f};
	float blue[] = {0.5f, 0.5f, 1.0f, 1.0f};
	float grey[] = {0.5f, 0.5f, 0.5f, 1.0f};
	float darkgrey[] = {0.2f, 0.2f, 0.2f, 1.0f};
	float lightblue[] = {0.5f, 0.5f, 1.0f, 1.0f};
	
	if (mainprogram->blocking) {
		mainprogram->mx = -1;
		mainprogram->my = -1;
	}
	if (SDL_GetMouseFocus() != mainprogram->mainwindow) {
		mainprogram->mx = -1;
		mainprogram->my = -1;
		mainprogram->lmsave = mainprogram->leftmouse;
		mainprogram->leftmouse = false;
		mainprogram->menuactivation = false;
	}
	if (mainmix->adaptparam) {
		// no hovering while adapting param
		mainprogram->my = -1;
	}
	
	if (mainprogram->prevmodus) {
		loopstation = lp;
	}
	else {
		loopstation = lpc;
	}

	if (mainprogram->ow != mainprogram->oldow or mainprogram->oh != mainprogram->oldoh) {
		mainmix->save_state(mainprogram->temppath + "tempstate.state");
		mainmix->open_state(mainprogram->temppath + "tempstate.state");
		mainprogram->oldow = mainprogram->ow;
		mainprogram->oldoh = mainprogram->oh;
	}
				
	if (mainprogram->openshelfdir) {
		// load one item from mainprogram->opendir into shelf, one each loop not to slowdown output stream
		mainprogram->shelves[mainmix->mousedeck]->open_dir();
	}
	
	for (int m = 0; m < 2; m++) {
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < mainmix->scenes[m][i]->nbframes.size(); j++) {
				if (i == mainmix->currscene[m]) break;
				calc_texture(mainmix->scenes[m][i]->nbframes[j], 0, 0);
			}
		}
	}
	
	mainmix->firstlayers.clear();
	if (!mainprogram->prevmodus) {
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
	if (mainprogram->prevmodus) {
		for(int i = 0; i < mainmix->layersA.size(); i++) {
			Layer *testlay = mainmix->layersA[i];
			calc_texture(testlay, 0, 1);
		}
		for(int i = 0; i < mainmix->layersB.size(); i++) {
			Layer *testlay = mainmix->layersB[i];
			calc_texture(testlay, 0, 1);
		}
	}
	if (mainprogram->prevmodus) {
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
	if (mainprogram->prevmodus) walk_nodes(0);
	walk_nodes(1);
	
	//draw and handle BINS wormhole
	float darkgreen[4] = {0.0f, 0.3f, 0.0f, 1.0f};
	if (!mainprogram->menuondisplay) {
		if (sqrt(pow((mainprogram->mx / (glob->w / 2.0f) - 1.0f) * glob->w / glob->h, 2) + pow((glob->h - mainprogram->my) / (glob->h / 2.0f) - 1.25f, 2)) < 0.2f) {
			darkgreen[0] = 0.5f;
			darkgreen[1] = 0.5f;
			darkgreen[2] = 1.0f;
			darkgreen[3] = 1.0f;
			mainprogram->tooltipbox = mainprogram->wormhole->box;
			if (mainprogram->leftmouse and !binsmain->dragmix) {
				mainprogram->binsscreen = !mainprogram->binsscreen;
				mainprogram->leftmouse = false;
			}
			if (mainprogram->menuactivation) {
				mainprogram->parammenu1->state = 2;
				mainmix->learnparam = nullptr;
				mainmix->learnbutton = mainprogram->wormhole;
				mainprogram->menuactivation = false;
			}
		}
	}
		
	
	//if (mainprogram->renaming != EDIT_NONE and mainprogram->leftmouse) mainprogram->renaming = EDIT_NONE;
	
	// wormhole
	if (mainprogram->fullscreen == -1) {
		draw_box(darkgreen, 0.0f, 0.25f, 0.2f, 1);
		draw_box(white, 0.0f, 0.25f, 0.2f, 2);
		draw_box(white, 0.0f, 0.25f, 0.15f, 2);
		draw_box(white, 0.0f, 0.25f, 0.1f, 2);
		if (mainprogram->binsscreen) render_text("MIX", white, -0.024f, 0.24f, 0.0006f, 0.001f);
		else render_text("BINS", white, -0.025f, 0.24f, 0.0006f, 0.001f);
	}
	
	if (mainprogram->binsscreen) {
		binsmain->handle();
	}
	else if (mainprogram->fullscreen > -1) {
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
		GLuint vbuf, tbuf, vao;
		glGenBuffers(1, &vbuf);
		glBindBuffer(GL_ARRAY_BUFFER, vbuf);
		glBufferData(GL_ARRAY_BUFFER, 32, vcoords1, GL_STATIC_DRAW);
		glGenBuffers(1, &tbuf);
		glBindBuffer(GL_ARRAY_BUFFER, tbuf);
		glBufferData(GL_ARRAY_BUFFER, 32, tcoords, GL_STATIC_DRAW);
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbuf);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
		glBindBuffer(GL_ARRAY_BUFFER, tbuf);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
	
		MixNode *node;
		if (mainprogram->fullscreen == mainprogram->nodesmain->mixnodes.size()) node = (MixNode*)mainprogram->nodesmain->mixnodescomp[mainprogram->fullscreen - 1];
		else node = (MixNode*)mainprogram->nodesmain->mixnodes[mainprogram->fullscreen];
		GLfloat cf = glGetUniformLocation(mainprogram->ShaderProgram, "cf");
		GLint wipe = glGetUniformLocation(mainprogram->ShaderProgram, "wipe");
		GLint mixmode = glGetUniformLocation(mainprogram->ShaderProgram, "mixmode");
		if (mainmix->wipe[mainprogram->fullscreen == 2] > -1) {
			if (mainprogram->fullscreen == 2) glUniform1f(cf, mainmix->crossfadecomp->value);
			else glUniform1f(cf, mainmix->crossfade->value);
			glUniform1i(wipe, 1);
			glUniform1i(mixmode, 18);
		}
		GLint down = glGetUniformLocation(mainprogram->ShaderProgram, "down");
		glUniform1i(down, 1);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, node->mixtex);
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glUniform1i(down, 0);
		glUniform1i(wipe, 0);
		glUniform1i(mixmode, 0);
		if (mainprogram->menuactivation) {
			mainprogram->fullscreenmenu->state = 2;
			mainprogram->menuondisplay = true;
			mainprogram->menuactivation = false;
		}
		
		// Draw and handle fullscreenmenu
		int k = handle_menu(mainprogram->fullscreenmenu);
		if (k == 0) mainprogram->fullscreen = -1;
		if (mainprogram->menuchosen) {
			mainprogram->menuchosen = false;
			mainprogram->leftmouse = 0;
			mainprogram->menuactivation = 0;
			mainprogram->menuresults.clear();
		}
	}
	else {  //the_loop else
		for (int i = 0; i < mainprogram->menulist.size(); i++) {
			if (mainprogram->menulist[i]->state > 1) {
				mainprogram->leftmousedown = false;
				mainprogram->menuondisplay = true;
				break;
			}
			else {
				mainprogram->menuondisplay = false;
			}
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
			if (mainmix->adaptparam->value < mainmix->adaptparam->range[0]) {
				mainmix->adaptparam->value = mainmix->adaptparam->range[0];
			}
			if (mainmix->adaptparam->value > mainmix->adaptparam->range[1]) {
				mainmix->adaptparam->value = mainmix->adaptparam->range[1];
			}
			mainmix->prevx = mainprogram->mx;
			
			if (mainmix->adaptparam == mainmix->currlay->speed) mainmix->currlay->set_clones();
			if (mainmix->adaptparam == mainmix->currlay->opacity) mainmix->currlay->set_clones();
			
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
				if (!mainmix->adaptparam->sliding) {
					mainmix->adaptparam->value = (int)(mainmix->adaptparam->value + 0.5f);
				}
				mainprogram->leftmouse = false;
				mainmix->adaptparam = nullptr;
			}
		}
		
		
		//blue bars designating layer selection
		float blue[4] = {0.1, 0.1, 0.6, 1.0};
		if (mainmix->currlay) {
			draw_box(nullptr, blue, -1.0f + mainprogram->numw + mainmix->currlay->deck * 1.0f + (mainmix->currlay->pos - mainmix->scenes[mainmix->currlay->deck][mainmix->currscene[mainmix->currlay->deck]]->scrollpos) * mainprogram->layw, -0.1f + tf(0.05f), mainprogram->layw, 1.1f, -1);
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
		if (mainprogram->prevmodus) par = mainmix->crossfade;
		else par = mainmix->crossfadecomp;
		par->handle();
			
		// Draw effectboxes
		// Bottom bar
		float sx1, sy1, vx1, vy1, vx2, vy2, sw;
		bool bottom = false;
		bool inbetween = false;
		Layer *lay = mainmix->currlay;
		Effect *eff = nullptr;
		std::vector<Effect*> &evec = lay->choose_effects();
		if (!mainprogram->queueing) {
			if (evec.size()) {
				eff = evec[evec.size() - 1];
				box = eff->onoffbutton->box;
				sx1 = box->scrcoords->x1;
				sy1 = box->scrcoords->y1 + (eff->numrows - 1) * mainprogram->yvtxtoscr(tf(0.05f));
				vx1 = box->vtxcoords->x1;
				vy1 = box->vtxcoords->y1 - (eff->numrows - 1) * tf(0.05f);
				sw = tf(mainprogram->layw) * glob->w / 2.0;
			}
			else {
				box = lay->mixbox;
				sw = tf(mainprogram->layw) * glob->w / 2.0;
				sx1 = box->scrcoords->x1 + mainprogram->xvtxtoscr(tf(0.025f));
				sy1 = mainprogram->layh * glob->h / 2.0 + mainprogram->yvtxtoscr(tf(0.25f));
				vx1 = box->vtxcoords->x1 + tf(0.025f);
				vy1 = 1 - mainprogram->layh - tf(0.25f);
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
							mainmix->mouseeffect = evec.size();
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
			for(int j = 0; j < evec.size(); j++) {
				eff = evec[j];
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
					if (box->scrcoords->x1 < mainprogram->mx and mainprogram->mx < box->scrcoords->x1 + tf(mainprogram->layw) * glob->w / 2.0) {
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
			}
		}


		if (mainprogram->prevmodus) {
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
			display_mix();
		}
		
	
		// Handle scenes
		handle_scenes(mainmix->scenes[0][mainmix->currscene[0]]);
		handle_scenes(mainmix->scenes[1][mainmix->currscene[1]]);
		
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
		if (mainprogram->prevmodus) {
			mainprogram->toscreen->handle();
			if (mainprogram->toscreen->toggled()) {
				mainprogram->toscreen->value = 0;
				mainprogram->toscreen->oldvalue = 0;
				// MAKE LIVE button copies preview set entirely to comp set
				mainmix->copy_to_comp(mainmix->layersA, mainmix->layersAcomp, mainmix->layersB, mainmix->layersBcomp, mainprogram->nodesmain->currpage->nodes, mainprogram->nodesmain->currpage->nodescomp, mainprogram->nodesmain->mixnodescomp, true);
				mainprogram->leftmouse = false;
			}
			Box *box = mainprogram->toscreen->box;
			draw_triangle(white, white, box->vtxcoords->x1 + box->vtxcoords->w / 2.0f + tf(0.0078f), box->vtxcoords->y1 + tf(0.015f), tf(0.011f), tf(0.0208f), DOWN, CLOSED);
			render_text(mainprogram->toscreen->name[0], white, box->vtxcoords->x1 + tf(0.0078f), box->vtxcoords->y1 + tf(0.015f), 0.0006f, 0.001f);
		}
		if (mainprogram->prevmodus) {
			mainprogram->backtopre->handle();
			if (mainprogram->backtopre->toggled()) {
				mainprogram->backtopre->value = 0;
				mainprogram->backtopre->oldvalue = 0;
				// BACK button copies comp set entirely back to preview set
				mainmix->copy_to_comp(mainmix->layersAcomp, mainmix->layersA, mainmix->layersBcomp, mainmix->layersB, mainprogram->nodesmain->currpage->nodescomp, mainprogram->nodesmain->currpage->nodes, mainprogram->nodesmain->mixnodes, false);
				mainprogram->leftmouse = false;
			}
			Box *box = mainprogram->backtopre->box;
			draw_triangle(white, white, box->vtxcoords->x1 + box->vtxcoords->w / 2.0f + tf(0.0078f), box->vtxcoords->y1 + tf(0.015f), tf(0.011f), tf(0.0208f), UP, CLOSED);
			render_text(mainprogram->backtopre->name[0], white, mainprogram->backtopre->box->vtxcoords->x1 + tf(0.0078f), mainprogram->backtopre->box->vtxcoords->y1 + tf(0.015f), 0.0006, 0.001);
		}
		
		mainprogram->modusbut->handle();
		if (mainprogram->modusbut->toggled())  {
			mainprogram->prevmodus = !mainprogram->prevmodus;
			//modusbut is button that toggles effect preview mode to performance mode and back
			mainprogram->preveff_init();
		}
		render_text(mainprogram->modusbut->name[mainprogram->prevmodus], white, mainprogram->modusbut->box->vtxcoords->x1 + tf(0.0078f), mainprogram->modusbut->box->vtxcoords->y1 + tf(0.015f), 0.00042, 0.00070);
		
		
		//draw and handle global deck speed sliders
		par = mainprogram->deckspeed[0];
		par->handle();
		draw_box(white, nullptr, mainprogram->deckspeed[0]->box->vtxcoords->x1, mainprogram->deckspeed[0]->box->vtxcoords->y1, mainprogram->deckspeed[0]->box->vtxcoords->w * 0.30f, 0.1f, -1);
		par = mainprogram->deckspeed[1];
		par->handle();
		draw_box(white, nullptr, mainprogram->deckspeed[1]->box->vtxcoords->x1, mainprogram->deckspeed[1]->box->vtxcoords->y1, mainprogram->deckspeed[1]->box->vtxcoords->w * 0.30f, 0.1f, -1);
			
		//draw and handle recbut
		mainmix->recbut->handle(1);
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
	
		
		// Draw effectmenuhints
		if(!mainprogram->queueing) {
			if (vy1 < 1.0 - tf(mainprogram->layh) - tf(0.22f) - tf(0.05f) * 10) {
				vy1 = 1.0 - tf(mainprogram->layh) - tf(0.20f) - tf(0.05f) * 10;
			}
			
			mainprogram->addeffectbox->vtxcoords->x1 = vx1;
			mainprogram->addeffectbox->vtxcoords->y1 = vy1 - tf(0.05f);
			mainprogram->addeffectbox->upvtxtoscr();
			if (bottom) { //true when mouse over Add Effect box
				bottom = false;
				draw_box(white, lightblue, mainprogram->addeffectbox, -1);
			}
			else {
				draw_box(white, black, mainprogram->addeffectbox, -1);
			}
			render_text("+ Add effect", white, vx1 + tf(0.01f), vy1 - tf(0.035f), tf(0.0003f), tf(0.0005f));
			mainprogram->addeffectbox->in();
	
			if (inbetween) { //true when mouse inbetween effect stack entries
				inbetween = false;
				draw_box(lightblue, lightblue, vx2, vy2, tf(mainprogram->layw), tf(0.022f), -1);
				draw_box(white, lightblue, mainprogram->mx * 2.0f / glob->w - 1.0f - tf(0.08f), 1.0f - (mainprogram->my * 2.0f / glob->h) - tf(0.019f), tf(0.16f), tf(0.038f), -1);
				render_text("+ Insert effect", white, mainprogram->mx * 2.0f / glob->w - 1.0f - tf(0.07f), 1.0f - (mainprogram->my * 2.0f / glob->h) - tf(0.004f), tf(0.0003f), tf(0.0005f));
			}
		}
		
		
		//Handle scrollbar
		bool inbox = false;
		for (int i = 0; i < 2; i++) {
			std::vector<Layer*> &lvec = choose_layers(i);
			draw_box(white, nullptr, -1.0f + mainprogram->numw + i, 1.0f - mainprogram->layh - 0.05f, mainprogram->layh * 3.0f, 0.05f, -1);
			Box box;
			Box boxbefore;
			Box boxafter;
			Box boxlayer;
			int size = lvec.size() + 1;
			if (size < 3) size = 3;
			box.vtxcoords->x1 = -1.0f + mainprogram->numw + i + mainmix->scenes[i][mainmix->currscene[i]]->scrollpos * (mainprogram->layw * 3.0f / size); 
			box.vtxcoords->y1 = 1.0f - mainprogram->layh - 0.05f;
			box.vtxcoords->w = (3.0f / size) * mainprogram->layw * 3.0f;
			box.vtxcoords->h = 0.05f;
			box.upvtxtoscr();
			boxbefore.vtxcoords->x1 = -1.0f + mainprogram->numw + i; 
			boxbefore.vtxcoords->y1 = 1.0f - mainprogram->layh - 0.05f;
			boxbefore.vtxcoords->w = (3.0f / size) * mainprogram->layw * mainmix->scenes[i][mainmix->currscene[i]]->scrollpos;
			boxbefore.vtxcoords->h = 0.05f;
			boxbefore.upvtxtoscr();
			boxafter.vtxcoords->x1 = -1.0f + mainprogram->numw + i + (mainmix->scenes[i][mainmix->currscene[i]]->scrollpos + 3) * (mainprogram->layw * 3.0f / size); 
			boxafter.vtxcoords->y1 = 1.0f - mainprogram->layh - 0.05f;
			boxafter.vtxcoords->w = (3.0f / size) * mainprogram->layw * (size - 3);
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
				boxlayer.vtxcoords->y1 = 1.0f - mainprogram->layh - 0.05f;
				boxlayer.vtxcoords->w = 0.031f;
				boxlayer.vtxcoords->h = 0.05f;
				for (int j = 0; j < lvec.size(); j++) {
					boxlayer.vtxcoords->x1 = -1.0f + mainprogram->numw + i + (j + mainmix->scenes[i][mainmix->currscene[i]]->scrollpos) * (mainprogram->layw * 3.0f / size); 
					boxlayer.upvtxtoscr();
					draw_box(nullptr, lvec[j]->scrollcol, &boxlayer, -1);
				}
			}
			box.vtxcoords->w /= 3.0f;
			if (mainmix->scrollon == i + 1) {
				if (mainprogram->leftmouse) {
					mainmix->scrollon = 0;
					//mainprogram->leftmouse = false;
				}
				else {
					if ((mainprogram->mx - mainmix->scrollmx) > mainprogram->xvtxtoscr(box.vtxcoords->w)) {
						if (mainmix->scenes[i][mainmix->currscene[i]]->scrollpos < size - 3) {
							mainmix->scenes[i][mainmix->currscene[i]]->scrollpos++;
							mainmix->scrollmx += mainprogram->xvtxtoscr(box.vtxcoords->w);
						}	
					}
					else if ((mainprogram->mx - mainmix->scrollmx) < -mainprogram->xvtxtoscr(box.vtxcoords->w)) {
						if (mainmix->scenes[i][mainmix->currscene[i]]->scrollpos > 0) {
							mainmix->scenes[i][mainmix->currscene[i]]->scrollpos--;
							mainmix->scrollmx -= mainprogram->xvtxtoscr(box.vtxcoords->w);
						}
					}
				}
			}
			box.vtxcoords->x1 = -1.0f + mainprogram->numw + i; 
			float remw = box.vtxcoords->w;
			for (int j = 0; j < size; j++) {
				//draw scrollbar numbers+divisions
				draw_box(white, nullptr, &box, -1);
				render_text(std::to_string(j + 1), white, box.vtxcoords->x1 + 0.0078f, box.vtxcoords->y1 + 0.0078f, 0.0006, 0.001);
				const int s = lvec.size() - mainmix->scenes[i][mainmix->currscene[i]]->scrollpos;
				bool greycond = (j >= mainmix->scenes[i][mainmix->currscene[i]]->scrollpos and j < mainmix->scenes[i][mainmix->currscene[i]]->scrollpos + std::min(s, 3));
				if (!greycond) {
					box.vtxcoords->x1 += box.vtxcoords->w;
					box.upvtxtoscr();
					continue;
				}
				
				//draw and handle layer mute and solo buttons
				lvec[j]->mutebut->box->vtxcoords->x1 = box.vtxcoords->x1 + 0.03f;
				lvec[j]->mutebut->box->upvtxtoscr();
				if (lvec[j]->mutebut->box->in()) {
					if (mainprogram->leftmouse) {
						lvec[j]->mutebut->value = !lvec[j]->mutebut->value;
						lvec[j]->mute_handle();
						mainprogram->leftmouse = false;
					}
					if (mainprogram->menuactivation and !mainprogram->menuondisplay) {
						mainprogram->parammenu1->state = 2;
						mainmix->learnparam = nullptr;
						mainmix->learnbutton = lvec[j]->mutebut;
						mainprogram->menuactivation = false;
					}
				}
				lvec[j]->solobut->box->vtxcoords->x1 = lvec[j]->mutebut->box->vtxcoords->x1 + 0.03f;
				lvec[j]->solobut->box->upvtxtoscr();
				if (lvec[j]->solobut->box->in()) {
					if (mainprogram->leftmouse) {
						lvec[j]->solobut->value = !lvec[j]->solobut->value;
						if (lvec[j]->solobut->value) {
							if (lvec[j]->mutebut->value) {
								lvec[j]->mutebut->value = false;
								lvec[j]->mute_handle();
							}
							for (int k = 0; k < lvec.size(); k++) {
								if (k != j) {
									if (lvec[k]->mutebut->value == false) {
										lvec[k]->mutebut->value = true;
										lvec[k]->mute_handle();
									}
									lvec[k]->solobut->value = false;
								}
							}
						}
						else {
							for (int k = 0; k < lvec.size(); k++) {
								if (k != j) {
									lvec[k]->mutebut->value = false;
									lvec[k]->mute_handle();
								}
							}
						}
						mainprogram->leftmouse = false;
					}
					if (mainprogram->menuactivation and !mainprogram->menuondisplay) {
						mainprogram->parammenu1->state = 2;
						mainmix->learnparam = nullptr;
						mainmix->learnbutton = lvec[j]->solobut;
						mainprogram->menuactivation = false;
					}
				}
				if (lvec[j]->mutebut->value) {
					draw_box(white, green, lvec[j]->mutebut->box, -1);
				}
				else {
					draw_box(white, nullptr, lvec[j]->mutebut->box, -1);
				}
				render_text("M", white, lvec[j]->mutebut->box->vtxcoords->x1 + 0.0078f, lvec[j]->mutebut->box->vtxcoords->y1 + 0.0078f, 0.0006, 0.001);
				lvec[j]->mutebut->box->in();
				if (lvec[j]->solobut->value) {
					draw_box(white, green, lvec[j]->solobut->box, -1);
				}
				else {
					draw_box(white, nullptr, lvec[j]->solobut->box, -1);
				}
				render_text("S", white, lvec[j]->solobut->box->vtxcoords->x1 + 0.0078f, lvec[j]->solobut->box->vtxcoords->y1 + 0.0078f, 0.0006, 0.001);
				lvec[j]->solobut->box->in();
				box.vtxcoords->x1 += box.vtxcoords->w;
				box.upvtxtoscr();
			}
			if (boxbefore.in()) {
				//mouse in empty scrollbar part before the lightgrey visible layers part
				inbox = true;
				if (mainprogram->dragbinel) {
					if (mainmix->scrolltime == 0.0f) {
						mainmix->scrolltime = mainmix->time;
					}
					else {
						if (mainmix->time - mainmix->scrolltime > 1.0f) {
							mainmix->scenes[i][mainmix->currscene[i]]->scrollpos--;
							mainmix->scrolltime = mainmix->time;
						}
					}
				}
				if (mainprogram->leftmouse) {
					mainmix->scenes[i][mainmix->currscene[i]]->scrollpos--;
				}
			}
			else if (boxafter.in()) {
				//mouse in empty scrollbar part after the lightgrey visible layers part
				inbox = true;
				if (mainprogram->dragbinel) {
					if (mainmix->scrolltime == 0.0f) {
						mainmix->scrolltime = mainmix->time;
					}
					else {
						if (mainmix->time - mainmix->scrolltime > 1.0f) {
							mainmix->scenes[i][mainmix->currscene[i]]->scrollpos++;
							mainmix->scrolltime = mainmix->time;
						}
					}
				}
				if (mainprogram->leftmouse) {
					mainmix->scenes[i][mainmix->currscene[i]]->scrollpos++;
				}
			}
			//trigger scrollbar tooltips
			mainprogram->scrollboxes[i]->in();
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
						draw_box(white, black, box->vtxcoords->x1, box->vtxcoords->y1 - (k + 1) * mainprogram->layh, box->vtxcoords->w, box->vtxcoords->h, lay2->clips[k + lay2->queuescroll]->tex);
						render_text("Queued clip #" + std::to_string(k + lay2->queuescroll + 1), white, box->vtxcoords->x1 + tf(0.01f), box->vtxcoords->y1 - k * box->vtxcoords->h - tf(0.03f), 0.0005f, 0.0008f);
					}
					for (int k = 0; k < lay2->clips.size() - lay2->queuescroll; k++) {
						if (box->scrcoords->x1 + (k == 0) * mainprogram->xvtxtoscr(0.075f) < mainprogram->mx and mainprogram->mx < box->scrcoords->x1 + box->scrcoords->w - (k == 0) * mainprogram->xvtxtoscr(0.075f)) {
							if (box->scrcoords->y1 + (k - 0.25f) * box->scrcoords->h < mainprogram->my and mainprogram->my < box->scrcoords->y1 + (k + 0.25) * box->scrcoords->h) {
								if (mainprogram->dragbinel) draw_box(lightblue, lightblue, box->vtxcoords->x1 + (k == 0) * 0.075f, box->vtxcoords->y1 - (k + 0.25f) * mainprogram->layh, box->vtxcoords->w - ((k == 0) * 0.075) * 2.0f, box->vtxcoords->h / 2.0f, -1);
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

		//handle shelves
		mainprogram->shelves[0]->handle();
		mainprogram->shelves[1]->handle();
		
		//handle clip dragging
		clip_dragging();		
		
		// draw and handle loopstation
		if (mainprogram->prevmodus) {
			lp->handle();
			for (int i = 0; i < lpc->elems.size(); i++) {
				if (lpc->elems[i]->loopbut->value or lpc->elems[i]->playbut->value) lpc->elems[i]->set_params();
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
			if (k == 0) {
				if (mainmix->mouseeffect > -1) mainmix->mouselayer->delete_effect(mainmix->mouseeffect);
			}
			else if (mainmix->insert) mainmix->mouselayer->add_effect((EFFECT_TYPE)mainprogram->abeffects[k - 1], mainmix->mouseeffect);
			else {
				std::vector<Effect*> &evec = mainmix->mouselayer->choose_effects();
				int mon = evec[mainmix->mouseeffect]->node->monitor;
				mainmix->mouselayer->replace_effect((EFFECT_TYPE)mainprogram->abeffects[k - 1], mainmix->mouseeffect);
				evec[mainmix->mouseeffect]->node->monitor = mon;
			}
			mainmix->mouselayer = nullptr;
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
				mainmix->mouselayer->set_clones();
			}
			else if (k == 1) {
				mainmix->mouselayer->endframe = mainmix->mouselayer->frame;
				if (mainmix->mouselayer->startframe > mainmix->mouselayer->endframe) {
					mainmix->mouselayer->startframe = mainmix->mouselayer->endframe;
				}
				mainmix->mouselayer->set_clones();
			}
			else if (k == 2) {
				float fac = mainprogram->deckspeed[mainmix->mouselayer->deck]->value;
				if (mainmix->mouselayer->clonesetnr != -1) {
					std::unordered_set<Layer*>::iterator it;
					for (it = mainmix->clonesets[mainmix->mouselayer->clonesetnr]->begin(); it != mainmix->clonesets[mainmix->mouselayer->clonesetnr]->end(); it++) {
						Layer *lay = *it;
						if (lay->deck == !mainmix->mouselayer->deck) {
							fac *= mainprogram->deckspeed[!mainmix->mouselayer->deck]->value;
							break;
						}
					}
				}
				mainmix->cbduration = ((float)(mainmix->mouselayer->endframe - mainmix->mouselayer->startframe) * mainmix->mouselayer->millif) / (mainmix->mouselayer->speed->value * mainmix->mouselayer->speed->value * fac * fac);
				printf("fac %f\n", fac);
			}
			else if (k == 3) {
				float fac = mainprogram->deckspeed[mainmix->mouselayer->deck]->value;
				if (mainmix->mouselayer->clonesetnr != -1) {
					std::unordered_set<Layer*>::iterator it;
					for (it = mainmix->clonesets[mainmix->mouselayer->clonesetnr]->begin(); it != mainmix->clonesets[mainmix->mouselayer->clonesetnr]->end(); it++) {
						Layer *lay = *it;
						if (lay->deck == !mainmix->mouselayer->deck) {
							fac *= mainprogram->deckspeed[!mainmix->mouselayer->deck]->value;
							break;
						}
					}
				}
				mainmix->mouselayer->speed->value = sqrt(((float)(mainmix->mouselayer->endframe - mainmix->mouselayer->startframe) * mainmix->mouselayer->millif) / mainmix->cbduration / (fac * fac));
				mainmix->mouselayer->set_clones();
			}
			else if (k == 4) {
				float sf, ef;
				float loop = mainmix->mouselayer->endframe - mainmix->mouselayer->startframe;
				float dsp = mainprogram->deckspeed[mainmix->mouselayer->deck]->value;
				if (mainmix->mouselayer->clonesetnr != -1) {
					std::unordered_set<Layer*>::iterator it;
					for (it = mainmix->clonesets[mainmix->mouselayer->clonesetnr]->begin(); it != mainmix->clonesets[mainmix->mouselayer->clonesetnr]->end(); it++) {
						Layer *lay = *it;
						if (lay->deck == !mainmix->mouselayer->deck) {
							dsp *= mainprogram->deckspeed[!mainmix->mouselayer->deck]->value;
							break;
						}
					}
				}
				float fac = ((loop * mainmix->mouselayer->millif) / mainmix->cbduration) / (mainmix->mouselayer->speed->value * mainmix->mouselayer->speed->value * dsp * dsp);
				float end = mainmix->mouselayer->numf - (mainmix->mouselayer->startframe + loop / fac);
				if (end > 0) {
					mainmix->mouselayer->endframe = mainmix->mouselayer->startframe + loop / fac;
				}
				else {
					mainmix->mouselayer->endframe = mainmix->mouselayer->numf;
					float start = mainmix->mouselayer->startframe + end;
					if (start > 0) { 
						mainmix->mouselayer->startframe += end;
					}
					else {
						mainmix->mouselayer->startframe = 0.0f;
						mainmix->mouselayer->endframe = mainmix->mouselayer->numf;
						mainmix->mouselayer->speed->value *= sqrt((float)mainmix->mouselayer->numf / ((float)mainmix->mouselayer->numf - start));
					}
				}
				mainmix->mouselayer->set_clones();
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
			mixtargets.push_back("View full screen");
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
				mainprogram->fullscreen = mainprogram->mixtargetmenu->value;
			}
			else if (k == 1) {
				if (mainprogram->menuresults[!mainprogram->prevmodus] == 0) {
					mainmix->wipe[!mainprogram->prevmodus] = -1;
				}
				else {
					mainmix->wipe[!mainprogram->prevmodus] = mainprogram->menuresults[0] - 1;
					mainmix->wipedir[!mainprogram->prevmodus] = mainprogram->menuresults[1];
				}
			}
			else if (k > 2) {
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
							while (currentries[k - 2]->win->syncnow) {
								currentries[k - 2]->win->sync.notify_one();
							}
						}
					}
				}
				else {
					// open new window on chosen ouput screen and start display thread (synced at end of the_loop)
					int screen;
					if (currentries.size()) screen = possscreens[k - currentries.size() - 3];
					else screen = possscreens[k - currentries.size() - 3];
					EWindow *mwin = new EWindow;
					mwin->mixid = mainprogram->mixtargetmenu->value;
					OutputEntry *entry = new OutputEntry;
					entry->screen = screen;
					entry->win = mwin;
					mainprogram->outputentries.push_back(entry);
					SDL_Rect rc1;
					SDL_GetDisplayBounds(screen, &rc1);										SDL_Rect rc2;
					SDL_GetDisplayUsableBounds(screen, &rc2);
					mwin->w = rc2.w;
					mwin->h = rc2.h;
					mwin->win = SDL_CreateWindow(PROGRAM_NAME, rc1.x, rc1.y, mwin->w, mwin->h, SDL_WINDOW_OPENGL | SDL_WINDOW_MAXIMIZED | SDL_WINDOW_RESIZABLE | 
					SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_ALLOW_HIGHDPI);
					SDL_RaiseWindow(mainprogram->mainwindow);
					printf("screen %d\n", screen);
					mainprogram->mixwindows.push_back(mwin);
					SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
					HGLRC c1 = wglGetCurrentContext();
					mwin->glc = SDL_GL_CreateContext(mwin->win);
					SDL_GL_MakeCurrent(mwin->win, mwin->glc);
					HGLRC c2 = wglGetCurrentContext();
					wglShareLists(c1, c2);
					glUseProgram(mainprogram->ShaderProgram);
					
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
			mainmix->wipe[!mainprogram->prevmodus] = k - 1;
			mainmix->wipedir[!mainprogram->prevmodus] = mainprogram->menuresults[0];
		}
		else if (k == 0) {
			mainmix->wipe[!mainprogram->prevmodus] = -1;
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
		else if (k == 2) {
			mainmix->learn = true;
		}
		if (mainprogram->menuchosen) {
			mainprogram->menuchosen = false;
			mainprogram->leftmouse = 0;
			mainprogram->menuactivation = 0;
			mainprogram->menuresults.clear();
		}
		
		
		// Draw and handle mainprogram->laymenu1
		if (mainprogram->laymenu1->state > 1 or mainprogram->laymenu2->state > 1) {
			if (!mainprogram->gotcameras) {
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
				mainprogram->gotcameras = true;
			}
		}
		else mainprogram->gotcameras = false;
		if (mainprogram->laymenu1->state > 1) k = handle_menu(mainprogram->laymenu1);
		else if (mainprogram->laymenu2->state > 1) {
			k = handle_menu(mainprogram->laymenu2);
			if (k > 11) k++;
		}
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
				std::thread filereq (&Program::get_inname, mainprogram, "Open video file", "", "");
				filereq.detach();
				mainprogram->loadlay = mainmix->mouselayer;
			}
			if (k == 2) {
				mainprogram->pathto = "OPENIMAGE";
				std::thread filereq (&Program::get_inname, mainprogram, "Open image file", "", "");
				filereq.detach();
				mainprogram->loadlay = mainmix->mouselayer;
			}
			else if (k == 3) {
				mainprogram->pathto = "OPENLAYFILE";
				std::thread filereq (&Program::get_inname, mainprogram, "Open layer file", "application/ewocvj2-layer", "");
				filereq.detach();
			}
			else if (k == 4) {
				mainprogram->pathto = "SAVELAYFILE";
				std::thread filereq (&Program::get_outname, mainprogram, "Save layer file", "application/ewocvj2-layer", "");
				filereq.detach();
			}
			else if (k == 5) {
				mainmix->new_file(mainmix->mousedeck, 1);
			}
			else if (k == 6) {
				mainprogram->pathto = "OPENDECK";
				std::thread filereq (&Program::get_inname, mainprogram, "Open deck file", "application/ewocvj2-deck", "");
				filereq.detach();
			}
			else if (k == 7) {
				mainprogram->pathto = "SAVEDECK";
				std::thread filereq (&Program::get_outname, mainprogram, "Save deck file", "application/ewocvj2-deck", "");
				filereq.detach();
			}
			else if (k == 8) {
				mainmix->new_file(2, 1);
			}
			else if (k == 9) {
				mainprogram->pathto = "OPENMIX";
				std::thread filereq (&Program::get_inname, mainprogram, "Open mix file", "application/ewocvj2-mix", "");
				filereq.detach();
			}
			else if (k == 10) {
				mainprogram->pathto = "SAVEMIX";
				std::thread filereq (&Program::get_outname, mainprogram, "Open mix file", "application/ewocvj2-mix", "");
				filereq.detach();
			}
			else if (k == 11) {
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
			else if (k == 12) {
				std::vector<Layer*> &lvec = choose_layers(mainmix->mouselayer->deck);
				Layer *clonelay = mainmix->clone_layer(lvec, mainmix->mouselayer);
				if (mainmix->mouselayer->clonesetnr == -1) {
					std::unordered_set<Layer*> *uset = new std::unordered_set<Layer*>;
					mainmix->clonesets.push_back(uset);
					uset->emplace(mainmix->mouselayer);
					mainmix->mouselayer->clonesetnr = mainmix->clonesets.size() - 1;
				}
				clonelay->clonesetnr = mainmix->mouselayer->clonesetnr;
				mainmix->clonesets[mainmix->mouselayer->clonesetnr]->emplace(clonelay);
			}
			else if (k == 13) {
				mainmix->mouselayer->shiftx->value = 0.0f;
				mainmix->mouselayer->shifty->value = 0.0f;
			}
			else if (k == 14) {
				mainmix->mouselayer->aspectratio = (RATIO_TYPE)mainprogram->menuresults[0];
				if (mainmix->mouselayer->type == ELEM_IMAGE) {
					ilBindImage(mainmix->mouselayer->boundimage);
					ilActiveImage((int)mainmix->mouselayer->frame);
					mainmix->mouselayer->set_aspectratio(ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT));
				}
				else {
					mainmix->mouselayer->set_aspectratio(mainmix->mouselayer->video_dec_ctx->width, mainmix->mouselayer->video_dec_ctx->height);
				}
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
				std::thread filereq (&Program::get_inname, mainprogram, "Open video file", "", "");
				filereq.detach();
				mainprogram->loadlay = mainmix->add_layer(lvec, lvec.size());
				mainmix->currlay = mainprogram->loadlay;
			}
			if (k == 2) {
				mainprogram->pathto = "OPENIMAGE";
				std::thread filereq (&Program::get_inname, mainprogram, "Open image file", "", "");
				filereq.detach();
				mainprogram->loadlay = mainmix->add_layer(lvec, lvec.size());
				mainmix->currlay = mainprogram->loadlay;
			}
			else if (k == 3) {
				mainprogram->loadlay = mainmix->add_layer(lvec, lvec.size());
				mainmix->mouselayer = mainprogram->loadlay;
				mainmix->currlay = mainprogram->loadlay;
				mainprogram->pathto = "OPENLAYFILE";
				std::thread filereq (&Program::get_inname, mainprogram, "Open layer file", "application/ewocvj2-layer", "");
				filereq.detach();
			}
			else if (k == 4) {
				mainmix->new_file(mainmix->mousedeck, 1);
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
				mainmix->new_file(2, 1);
			}
			else if (k == 8) {
				mainprogram->pathto = "OPENMIX";
				std::thread filereq (&Program::get_inname, mainprogram, "Open mix file", "application/ewocvj2-mix", "");
				filereq.detach();
			}
			else if (k == 9) {
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
				mainprogram->tmscratch->upvtxtoscr();
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
			mainprogram->pathto = "NEWPROJECT";
			std::string reqdir = mainprogram->projdir;
			if (reqdir.substr(0, 1) == ".") reqdir.erase(0, 2);
			std::string name = "Untitled";
			std::string path;
			int count = 0;
			while (1) {
				path = mainprogram->projdir + name;
				if (!exists(path + ".ewocvj")) {
					break;
				}
				count++;
				name = remove_version(name) + "_" + std::to_string(count);
			}
			std::thread filereq (&Program::get_outname, mainprogram, "New project", "application/ewocvj2-project", boost::filesystem::absolute(reqdir + name).string());
			filereq.detach();
		}
		else if (k == 1) {
			mainprogram->pathto = "OPENPROJECT";
			std::thread filereq (&Program::get_inname, mainprogram, "Open project file", "application/ewocvj2-project", "");
			filereq.detach();
		}
		else if (k == 2) {
			mainprogram->project->save(mainprogram->project->path);
		}
		else if (k == 3) {
			new_state();
		}
		else if (k == 4) {
			mainprogram->pathto = "OPENSTATE";
			std::thread filereq (&Program::get_inname, mainprogram, "Open state file", "application/ewocvj2-state", "");
			filereq.detach();
		}
		else if (k == 5) {
			mainprogram->pathto = "SAVESTATE";
			std::thread filereq (&Program::get_outname, mainprogram, "Save state file", "application/ewocvj2-state", "");
			filereq.detach();
		}
		else if (k == 6) {
			if (!mainprogram->prefon) {
				mainprogram->prefon = true;
				SDL_ShowWindow(mainprogram->prefwindow);
				SDL_RaiseWindow(mainprogram->prefwindow);
				SDL_GL_MakeCurrent(mainprogram->prefwindow, glc_pr);
				glUseProgram(mainprogram->ShaderProgram_pr);
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				for (int i = 0; i < mainprogram->prefs->items.size(); i++) {
					PrefCat *item = mainprogram->prefs->items[i];
					item->box->upvtxtoscr();
				}
			}
			else {
				SDL_RaiseWindow(mainprogram->prefwindow);
			}
		}
		else if (k == 7) {
			mainprogram->quit("quitted");
		}
	
		if (mainprogram->menuchosen) {
			mainprogram->menuchosen = false;
			mainprogram->leftmouse = 0;
			mainprogram->menuactivation = 0;
			mainprogram->menuresults.clear();
		}
				
		// Draw and handle shelfmenu
		k = handle_menu(mainprogram->shelfmenu);
		if (k == 0) {
			mainprogram->shelves[mainmix->mousedeck]->erase();
		}
		else if (k == 1) {
			mainprogram->pathto = "OPENSHELF";
			std::thread filereq (&Program::get_inname, mainprogram, "Open shelf file", "application/ewocvj2-shelf", mainprogram->shelfdir);
			filereq.detach();
		}
		else if (k == 2) {
			mainprogram->pathto = "SAVESHELF";
			std::thread filereq (&Program::get_outname, mainprogram, "Save shelf file", "application/ewocvj2-shelf", mainprogram->shelfdir);
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
			mainprogram->pathto = "OPENSHELFIMAGE";
			std::thread filereq (&Program::get_inname, mainprogram, "Load image into shelf", "", "");
			filereq.detach();
		}
		else if (k == 7) {
			//reminder: MIDI LEARN
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
					if (lay->pos < mainmix->scenes[j][mainmix->currscene[j]]->scrollpos or lay->pos > mainmix->scenes[j][mainmix->currscene[j]]->scrollpos + 2) continue;
					Box *box = lay->node->vidbox;
					float thick;
					if (mainprogram->dragbinel) thick = mainprogram->xvtxtoscr(0.075f);
					else thick = mainprogram->xvtxtoscr(tf(0.006f));
					if (box->scrcoords->y1 - box->scrcoords->h < mainprogram->my and mainprogram->my < box->scrcoords->y1) {
						if (box->scrcoords->x1 - thick + (i - mainmix->scenes[j][mainmix->currscene[j]]->scrollpos == 0) * thick < mainprogram->mx and mainprogram->mx < box->scrcoords->x1 + thick) {
							mainprogram->leftmousedown = false;
							float blue[] = {0.5, 0.5, 1.0, 1.0};
							draw_box(blue, blue, box->vtxcoords->x1 - mainprogram->xscrtovtx(thick) + (i - mainmix->scenes[j][mainmix->currscene[j]]->scrollpos == 0) * mainprogram->xscrtovtx(thick), box->vtxcoords->y1, mainprogram->xscrtovtx(thick * (2.0f - (i - mainmix->scenes[j][mainmix->currscene[j]]->scrollpos == 0))), mainprogram->layh, -1);
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
							if (lay->pos == lvec.size() - 1 or lay->pos == mainmix->scenes[j][mainmix->currscene[j]]->scrollpos + 2) {
								float blue[] = {0.5, 0.5 , 1.0, 1.0};
								draw_box(blue, blue, box->vtxcoords->x1 + box->vtxcoords->w - mainprogram->xscrtovtx(thick), box->vtxcoords->y1, mainprogram->xscrtovtx(thick * (1.0f + (i - mainmix->scenes[j][mainmix->currscene[j]]->scrollpos != 2))), mainprogram->layh, -1);
								float red[] = {1.0, 0.0 , 0.0, 1.0};
								if (!mainprogram->dragbinel) draw_box(red, red, box->vtxcoords->x1 + box->vtxcoords->w - mainprogram->xscrtovtx(thick), box->vtxcoords->y1 + box->vtxcoords->h, mainprogram->xscrtovtx(thick * (1.0f + (i - mainmix->scenes[j][mainmix->currscene[j]]->scrollpos != 2))), -tf(0.05f), -1);
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
						lay->transmx = mainprogram->mx - lay->shiftx->value;
						lay->transmy = mainprogram->my - lay->shifty->value;
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
							if (lay->type == ELEM_LIVE) {
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
					lay->scale->value -= mainprogram->mousewheel * lay->scale->value / 10.0f;
					for (int i = 0; i < loopstation->elems.size(); i++) {
						if (loopstation->elems[i]->recbut->value) {
							mainmix->adaptparam = lay->scale;
							loopstation->elems[i]->add_param();
						}
					}
				}
			}
			
			if (lay->transforming == 1) {
				lay->shiftx->value = mainprogram->mx - lay->transmx;
				lay->shifty->value = mainprogram->my - lay->transmy;
				for (int i = 0; i < loopstation->elems.size(); i++) {
					if (loopstation->elems[i]->recbut->value) {
						mainmix->adaptparam = lay->shiftx;
						loopstation->elems[i]->add_param();
					}
				}
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
						if (mainprogram->prevmodus) {
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
					if (mainprogram->prevmodus) {
						mainprogram->mixtargetmenu->value = mainprogram->nodesmain->mixnodes.size();
					}
					else {
						mainprogram->mixtargetmenu->value = mainprogram->nodesmain->mixnodescomp.size();
					}
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
					if (box->in()) {
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
				
				// Handle colorbox
				pick_color(lay, lay->colorbox);
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
			boxA.vtxcoords->x1 = -1.0f + mainprogram->numw;
			boxA.vtxcoords->y1 = 1.0f - mainprogram->layh;
			boxA.vtxcoords->w = mainprogram->layw * 3;
			boxA.vtxcoords->h = mainprogram->layh;
			boxA.upvtxtoscr();
			Box boxB;
			boxB.vtxcoords->x1 = mainprogram->numw;
			boxB.vtxcoords->y1 = 1.0f - mainprogram->layh;
			boxB.vtxcoords->w = mainprogram->layw * 3;
			boxB.vtxcoords->h = mainprogram->layh;
			boxB.upvtxtoscr();
			if (boxA.in()) {
				draw_box(nullptr, lightblue, -1.0f + mainprogram->numw, 1.0f - mainprogram->layh, mainprogram->layw * 3, mainprogram->layh, -1);
				if (mainprogram->leftmouse) {
					mainmix->mousedeck = 0;
					mainmix->open_deck(binsmain->dragdeck->path, 1);
				}
			}
			else if (boxB.in()) {
				draw_box(nullptr, lightblue, mainprogram->numw, 1.0f - mainprogram->layh, mainprogram->layw * 3, mainprogram->layh, -1);
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
			if (!mainprogram->inwormhole and !mainprogram->menuondisplay and !mainprogram->shelfdrag) {
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
					if (mainprogram->prevmodus) {
						if (mainprogram->mx < glob->w / 2.0f) exchange(lay, lvec, mainmix->layersA, 0);
						else exchange(lay, lvec, mainmix->layersB, 1);
					}
					else {
						if (mainprogram->mx < glob->w / 2.0f) exchange(lay, lvec, mainmix->layersAcomp, 0);
						else exchange(lay, lvec, mainmix->layersBcomp, 1);
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
	
	
	if (!mainprogram->prefon and !mainprogram->tunemidi) tooltips_handle(1.0f);
	
	
	//autosave
	if (mainprogram->autosave and mainmix->time > mainprogram->astimestamp + mainprogram->asminutes * 60) {
		mainprogram->astimestamp = (int)mainmix->time;
		
		std::string name = "autosave_" + remove_extension(basename(mainprogram->project->path)) + "_0";
		std::string path;
		int count = 0;
		while (1) {
			path = mainprogram->autosavedir + name;
			if (!exists(path + ".state")) {
				break;
			}
			count++;
			name = remove_version(name) + "_" + std::to_string(count);
		}
		mainmix->save_state(path);
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
	
	glFlush();
	bool prret = false;
	GLuint tex, fbo;
	if (mainprogram->prefon) {
		SDL_GL_MakeCurrent(mainprogram->prefwindow, glc_pr);
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, smw, smh, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
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
	
	mainprogram->leftmouse = 0;
	mainprogram->doubleleftmouse = 0;
	mainprogram->middlemouse = 0;
	mainprogram->rightmouse = 0;
}


void new_state() {
	bool save = mainprogram->prevmodus;
	mainprogram->prevmodus = true;
	mainmix->new_file(2, 1);
	mainprogram->prevmodus = false;
	mainmix->new_file(2, 1);
	//clear genmidis?
	mainprogram->shelves[0]->erase();
	mainprogram->shelves[1]->erase();
	lp->init();
	lpc->init();
	mainprogram->prevmodus = save;
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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tw, th, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
	GLuint dfbo;
	glGenFramebuffers(1, &dfbo);
	glBindFramebuffer(GL_FRAMEBUFFER, dfbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, smalltex, 0);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glViewport(0, 0, tw, th);
	int sw, sh;
	glBindTexture(GL_TEXTURE_2D, tex);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &sw);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &sh);
	float black[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	if (yflip) {
		draw_box(nullptr, black, -1.0f, -1.0f, 2.0f, 2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, tex, glob->w, glob->h);
	}
	else {
		draw_box(nullptr, black, -1.0f, -1.0f + 2.0f, 2.0f, -2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, tex, glob->w, glob->h);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glViewport(0, 0, glob->w, glob->h);
	glDeleteFramebuffers(1, &dfbo);
	return smalltex;
}

void save_thumb(std::string path, GLuint tex) {
	int wi = mainprogram->ow3;
	int he = mainprogram->oh3;
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
		ifstream fileInput;
		fileInput.open(paths[i], ios::in | ios::binary);
		int fileSize = getFileSize(paths[i]);
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
		wfile << this->paths[j];
		wfile << "\n";
		wfile << std::to_string(this->types[j]);
		wfile << "\n";
		if (this->types[j] == ELEM_LAYER) {
			filestoadd.push_back(this->paths[j]);
		}
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

bool Shelf::open_videofile(const std::string &path, int pos) {
	this->paths[pos] = path;
	this->types[pos] = ELEM_FILE;
	Layer *lay = new Layer(true);
	lay->dummy = true;
	open_video(1, lay, path, true);
	lay->frame = lay->numf / 2.0f;
	lay->prevframe = -1;
	lay->ready = true;
	while (lay->ready) {
		lay->startdecode.notify_one();
	}
	std::unique_lock<std::mutex> lock(lay->enddecodelock);
	lay->enddecodevar.wait(lock, [&]{return lay->processed;});
	lay->processed = false;
	lock.unlock();
	if (lay->openerr) {
		this->paths[pos] = "";
		lay->closethread = true;
		while (lay->closethread) {
			lay->ready = true;
			lay->startdecode.notify_one();
		}
		delete lay;
		return 0;
	}
	
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
	this->texes[pos] = copy_tex(lay->texture);
	
	lay->closethread = true;
	while (lay->closethread) {
		lay->ready = true;
		lay->startdecode.notify_one();
	}
	delete lay;
	
	mainprogram->shelves[0]->save(mainprogram->shelfdir + "shelfsA.shelf");
	mainprogram->shelves[1]->save(mainprogram->shelfdir + "shelfsB.shelf");
	return 1;
}
	
bool Shelf::open_layer(const std::string &path, int pos) {
	float black[] = {0.0f, 0.0f, 0.0f, 1.0f};
	this->paths[pos] = path;
	this->types[pos] = ELEM_LAYER;
	Layer *lay = new Layer(false);
	lay->dummy = true;
	lay->pos = 0;
	lay->node = mainprogram->nodesmain->currpage->add_videonode(2);
	lay->node->layer = lay;
	lay->lasteffnode[0] = lay->node;
	mainmix->open_layerfile(path, lay, false, 0);
	lay->node->calc = true;
	lay->node->walked = false;
	if (lay->openerr) {
		this->paths[pos] = "";
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
		lay->enddecodevar.wait(lock, [&]{return lay->processed;});
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
		this->texes[pos] = copy_tex(lay->effects[0][lay->effects[0].size() - 1]->fbotex);
	}
	else {
		this->texes[pos] = copy_tex(lay->fbotex);
	}
	lay->closethread = true;
	while (lay->closethread) {
		lay->ready = true;
		lay->startdecode.notify_one();
	}
	delete lay;
	
	mainprogram->shelves[0]->save(mainprogram->shelfdir + "shelfsA.shelf");
	mainprogram->shelves[1]->save(mainprogram->shelfdir + "shelfsB.shelf");
	return 1;
}

bool Shelf::open_image(const std::string &path, int pos) {
	ILboolean ret = ilLoadImage(path.c_str());
	if (ret == IL_FALSE) return 0;
	this->paths[pos] = path;
	this->types[pos] = ELEM_IMAGE;
	float x = ilGetInteger(IL_IMAGE_WIDTH);
	float y = ilGetInteger(IL_IMAGE_HEIGHT);
	int iw, ih, xs, ys;
	if (x / y > mainprogram->ow / mainprogram->oh) {
		iw = x;
		ih = y * x * glob->h / y / glob->w;
		xs = 0;
		ys = (ih - y) / 2.0f;
	}
	else {
		iw = x * mainprogram->ow * y / mainprogram->oh / x;
		ih = y;
		xs = (iw - x) / 2.0f;
		ys = 0;
	}
	int bpp = ilGetInteger(IL_IMAGE_BPP);
	std::vector<int> emptydata(iw * ih);
	std::fill(emptydata.begin(), emptydata.end(), 0x000000ff);
	glBindTexture(GL_TEXTURE_2D, this->texes[pos]);
	if (bpp == 3) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, iw, ih, 0, GL_BGRA, GL_UNSIGNED_BYTE, &emptydata[0]);
		glTexSubImage2D(GL_TEXTURE_2D, 0, xs, ys, x, y, GL_RGB, GL_UNSIGNED_BYTE, ilGetData());
	}
	else if (bpp == 4) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, iw, ih, 0, GL_BGRA, GL_UNSIGNED_BYTE, &emptydata[0]);
		glTexSubImage2D(GL_TEXTURE_2D, 0, xs, ys, x, y, GL_BGRA, GL_UNSIGNED_BYTE, ilGetData());
	}
	
	mainprogram->shelves[0]->save(mainprogram->shelfdir + "shelfsA.shelf");
	mainprogram->shelves[1]->save(mainprogram->shelfdir + "shelfsB.shelf");
	return 1;
}
	
void Shelf::open(const std::string &path) {
	
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
			while (getline(rfile, istring)) {
				if (istring == "ENDOFELEMS") break;
				this->paths[count] = istring;
				getline(rfile, istring);
				this->types[count] = (ELEM_TYPE)std::stoi(istring);
				if (this->types[count] == ELEM_LAYER) {
					if (concat) {
						boost::filesystem::rename(result + "_" + std::to_string(filecount) + ".file", this->paths[count]);
						filecount++;
					}
				}
				if (this->paths[count] == "") {
					count++;
					continue;
				}
				if (this->types[count] == ELEM_FILE) {
					this->open_videofile(this->paths[count], count);
				}
				else if (this->types[count] == ELEM_LAYER) {
					this->open_layer(this->paths[count], count);
				}
				else if (this->types[count] == ELEM_IMAGE) {
					this->open_image(this->paths[count], count);
				}
				count++;
			}
		}
	}

	rfile.close();
}

void Shelf::erase() {
	std::vector<int> emptydata(4096);
	std::fill(emptydata.begin(), emptydata.end(), 0xff000000);
	for (int i = 0; i < 16; i++) {
		this->paths[i] = "";
		this->types[i] = ELEM_FILE;
		glBindTexture(GL_TEXTURE_2D, this->texes[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 64, 64, 0, GL_BGRA, GL_UNSIGNED_BYTE, &emptydata[0]);
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
	wfile << std::to_string(mainprogram->wormhole->midi[0]);
	wfile << "\n";
	wfile << "WORMHOLEMIDI1\n";
	wfile << std::to_string(mainprogram->wormhole->midi[1]);
	wfile << "\n";
	wfile << "WORMHOLEMIDIPORT\n";
	wfile << mainprogram->wormhole->midiport;
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
		
		if (istring == "WORMHOLE0MIDI0") {
			getline(rfile, istring);
			mainprogram->wormhole->midi[0] = std::stoi(istring);
		}
		if (istring == "WORMHOLEMIDI1") {
			getline(rfile, istring);
			mainprogram->wormhole->midi[1] = std::stoi(istring);
		}
		if (istring == "WORMHOLEMIDIPORT") {
			getline(rfile, istring);
			mainprogram->wormhole->midiport = istring;
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
	const char *defaultDeviceName = alcGetString(nullptr, ALC_DEFAULT_DEVICE_SPECIFIER);
	ALCdevice *device = alcOpenDevice(defaultDeviceName);
	ALCcontext *context = alcCreateContext(device, nullptr);
	bool succes = alcMakeContextCurrent(context);
	
	//initializing devIL
	ilInit();
	
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

	mainprogram = new Program;	
 	mainprogram->mainwindow = win;
 	lp = new LoopStation;
 	lpc = new LoopStation;
 	loopstation = lp;
 	mainmix = new Mixer;
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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, smw, smh, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, smw, smh, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, glob->w, glob->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
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
 	mainprogram->make_menu("parammenu1", mainprogram->parammenu1, parammodes1);
 	
	std::vector<std::string> parammodes2;
	parammodes2.push_back("MIDI Learn");
	parammodes2.push_back("Remove automation");
 	mainprogram->make_menu("parammenu2", mainprogram->parammenu2, parammodes2);
 	
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
 	layops1.push_back("Open video");
	layops1.push_back("Open image");
	layops1.push_back("Open layer");
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
 	layops2.push_back("Open video");
	layops2.push_back("Open image");
	layops2.push_back("Open layer");
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
 	loadops.push_back("Open video");
	loadops.push_back("Open image");
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
  	generic.push_back("New project");
  	generic.push_back("Open project");
  	generic.push_back("Save project");
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
  	shelf1.push_back("Open image");
  	shelf1.push_back("MIDI Learn");
  	mainprogram->make_menu("shelfmenu", mainprogram->shelfmenu, shelf1);

 	
	mainprogram->nodesmain = new NodesMain;
	mainprogram->nodesmain->add_nodepages(8);
	mainprogram->nodesmain->currpage = mainprogram->nodesmain->pages[0];
	mainprogram->toscreen->box->upvtxtoscr();
	mainprogram->backtopre->box->upvtxtoscr();
	mainprogram->modusbut->box->upvtxtoscr();
	
	
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
	boost::filesystem::path p5{"./projects"};
	if (!exists("./projects")) boost::filesystem::create_directory(p5);
	boost::filesystem::path p6{"./autosaves"};
	if (!exists("./autosaves")) boost::filesystem::create_directory(p6);
	#else
	#ifdef __linux__
	std::string homedir (getenv("HOME"));
	boost::filesystem::path e{homedir + "/.ewocvj2"};
	if (!exists(homedir + "/.ewocvj2")) boost::filesystem::create_directory(e);
	boost::filesystem::path p2{homedir + "/.ewocvj2/recordings"};
	if (!exists(homedir + "/.ewocvj2/bins")) boost::filesystem::create_directory(p1);
	boost::filesystem::path p2{homedir + "/.ewocvj2/recordings"};
	if (!exists(homedir + "/recordings")) boost::filesystem::create_directory(p2);
	boost::filesystem::path p3{homedir + "/.ewocvj2/shelves"};
	if (!exists(homedir + "/.ewocvj2/shelves")) boost::filesystem::create_directory(p3);
	boost::filesystem::path p4{homedir + "/.ewocvj2/temp"};
	if (!exists(homedir + "/.ewocvj2/temp")) boost::filesystem::create_directory(p4);
	boost::filesystem::path p5{homedir + "/.ewocvj2/projects"};
	if (!exists(homedir + "/.ewocvj2/projects")) boost::filesystem::create_directory(p5);
	boost::filesystem::path p6{homedir + "/.ewocvj2/autosaves"};
	if (!exists(homedir + "/.ewocvj2/autosaves")) boost::filesystem::create_directory(p6);
	#endif
	#endif

	GLint Sampler0 = glGetUniformLocation(mainprogram->ShaderProgram, "Sampler0");
	glUniform1i(Sampler0, 0);
	
	mainprogram->loadlay = new Layer(true);
	for (int m = 0; m < 2; m++) {
		std::vector<Layer*> &lvec = choose_layers(m);
		for (int i = 0; i < 4; i++) {
			mainmix->currscene[m] = i;
			Layer *lay = mainmix->add_layer(lvec, 0);
			lay->closethread = true;
			mainmix->scenes[m][mainmix->currscene[m]]->nbframes.insert(mainmix->scenes[m][mainmix->currscene[m]]->nbframes.begin(), lay);
			lay->clips.clear();
			mainmix->mousedeck = m;
			mainmix->save_deck(mainprogram->temppath + "tempdeck_" + std::to_string(m) + std::to_string(i + 1) + ".deck");
			mainmix->save_deck(mainprogram->temppath + "tempdeck_" + std::to_string(m) + std::to_string(i + 1) + "comp.deck");
			lvec.clear();
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
	
	mainmix->currscene[0] = 0;
	mainmix->mousedeck = 0;
	mainmix->open_deck(mainprogram->temppath + "tempdeck_01.deck", 1);
	mainmix->currscene[1] = 0;
	mainmix->mousedeck = 1;
	mainmix->open_deck(mainprogram->temppath + "tempdeck_11.deck", 1);
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
	std::string dir = "./";
	#else
	#ifdef __linux__
	std::string homedir (getenv("HOME"));
	std::string dir = homedir + "/.ewocvj2/"
	#endif
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

	
	while (!quit){

		io.poll();
		
		if (mainprogram->path != nullptr) {
			if (mainprogram->pathto == "OPENVIDEO") {
				std::string str(mainprogram->path);
				open_video(0, mainprogram->loadlay, str, true);
			}
			if (mainprogram->pathto == "OPENIMAGE") {
				std::string str(mainprogram->path);
				mainprogram->loadlay->open_image(str);
			}
			else if (mainprogram->pathto == "OPENSHELF") {
				std::string str(mainprogram->path);
				mainprogram->shelves[mainmix->mousedeck]->open(str);
			}
			else if (mainprogram->pathto == "SAVESHELF") {
				std::string str(mainprogram->path);
				mainprogram->shelves[mainmix->mousedeck]->save(str);
			}
			else if (mainprogram->pathto == "OPENSHELFVIDEO") {
				std::string str(mainprogram->path);
				mainmix->mouseshelf->open_videofile(str, mainmix->mouseshelfelem);
			}
			else if (mainprogram->pathto == "OPENSHELFLAYER") {
				std::string str(mainprogram->path);
				mainmix->mouseshelf->open_layer(str, mainmix->mouseshelfelem);
			}
			else if (mainprogram->pathto == "OPENSHELFDIR") {
				mainprogram->shelfpath = mainprogram->path;
				mainprogram->opendir = opendir(mainprogram->path);
				if (mainprogram->opendir) {
					mainprogram->openshelfdir = true;
					mainprogram->shelfdircount = 0;
				}
			}
			else if (mainprogram->pathto == "OPENSHELFIMAGE") {
				std::string str(mainprogram->path);
				mainmix->mouseshelf->open_image(str, mainmix->mouseshelfelem);
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
				if (driveletter1 == driveletter2) {
					mainprogram->choosedir = "./" + boost::filesystem::relative(str, "./").string() + "/";
				}
				else {
					mainprogram->choosedir = str + "/";
				}
			}
			else if (mainprogram->pathto == "NEWPROJECT") {
				std::string str(mainprogram->path);
				new_state();
				mainprogram->project->newp(str);
			}
			else if (mainprogram->pathto == "OPENPROJECT") {
				std::string str(mainprogram->path);
				mainprogram->project->open(str);
			}
			mainprogram->path = nullptr;
			mainprogram->pathto = "";
		}
		else mainprogram->blocking = false;
		
		bool focus = true;
		mainprogram->mousewheel = 0;
		SDL_Event e;
		while (SDL_PollEvent(&e)){
			SDL_PumpEvents();
			if (mainprogram->renaming != EDIT_NONE) {
                if (e.type == SDL_TEXTINPUT) {
                    /* Add new text onto the end of our text */
                    if( !( ( e.text.text[ 0 ] == 'c' || e.text.text[ 0 ] == 'C' ) && ( e.text.text[ 0 ] == 'v' || e.text.text[ 0 ] == 'V' ) && SDL_GetModState() & KMOD_CTRL ) ) {
						if (mainprogram->prefon) {
							for (int i = 0; i < mainprogram->prguistrings.size(); i++) {
								if (mainprogram->inputtext.compare(mainprogram->prguistrings[i]->text) == 0) {
									glDeleteTextures(1, &mainprogram->prguistrings[i]->texture);
									delete mainprogram->prguistrings[i];
									mainprogram->prguistrings.erase(mainprogram->prguistrings.begin() + i);
									break;
								}
							}
						}
						else if (!mainprogram->tunemidi) {
							for (int i = 0; i < mainprogram->guistrings.size(); i++) {
								if (mainprogram->inputtext.compare(mainprogram->guistrings[i]->text) == 0) {
									glDeleteTextures(1, &mainprogram->guistrings[i]->texture);
									delete mainprogram->guistrings[i];
									mainprogram->prguistrings.erase(mainprogram->guistrings.begin() + i);
									break;
								}
							}
						}
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
							if (mainprogram->prefon) {
								for (int i = 0; i < mainprogram->prguistrings.size(); i++) {
									if (mainprogram->inputtext.compare(mainprogram->prguistrings[i]->text) == 0) {
										glDeleteTextures(1, &mainprogram->prguistrings[i]->texture);
										delete mainprogram->prguistrings[i];
										mainprogram->prguistrings.erase(mainprogram->prguistrings.begin() + i);
										break;
									}
								}
							}
							else if (!mainprogram->tunemidi) {
								for (int i = 0; i < mainprogram->guistrings.size(); i++) {
									if (mainprogram->inputtext.compare(mainprogram->guistrings[i]->text) == 0) {
										glDeleteTextures(1, &mainprogram->guistrings[i]->texture);
										delete mainprogram->guistrings[i];
										mainprogram->prguistrings.erase(mainprogram->guistrings.begin() + i);
										break;
									}
								}
							}
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
						SDL_StopTextInput();
						continue;
					}
				}
				if (mainprogram->renaming == EDIT_BINNAME) {
					binsmain->menubin->name = mainprogram->inputtext;
					std::string oldpath = mainprogram->binsdir + mainprogram->backupname;
					std::string newpath = mainprogram->binsdir + mainprogram->inputtext;
					if (exists(oldpath)) boost::filesystem::rename(oldpath, newpath);
					oldpath += ".bin";
					newpath += ".bin";
					if (exists(oldpath)) boost::filesystem::rename(oldpath, newpath);
					binsmain->save_binslist();
				}
				else if (mainprogram->renaming == EDIT_PARAM) {
					int diff = mainprogram->inputtext.find(".") + 4 - mainprogram->inputtext.length();
					if (diff < 0) {
						mainprogram->inputtext = mainprogram->inputtext.substr(0, mainprogram->inputtext.find(".") + 4);
						if (mainprogram->cursorpos > mainprogram->inputtext.length()) {
							mainprogram->cursorpos = mainprogram->inputtext.length();
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
						mainmix->new_file(2, 1);
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
				if (e.key.keysym.sym == SDLK_ESCAPE) {
					mainprogram->fullscreen = -1;
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
				if (mainprogram->tooltipbox) {
					mainprogram->tooltipmilli = 0.0f;
					mainprogram->tooltipbox = nullptr;
				}
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
					if (e.button.clicks == 2) mainprogram->doubleleftmouse = true;
					mainprogram->leftmousedown = true;
				}
				if (e.button.button == SDL_BUTTON_MIDDLE) {
				
				
					mainprogram->middlemousedown = true;
				}
			}
			else if (e.type == SDL_MOUSEBUTTONUP){
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
		glClear(GL_COLOR_BUFFER_BIT);	
	
		// draw close window icon?
		if (mainprogram->mx == glob->w - 1 and mainprogram->my == 0) {
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDrawBuffer(GL_BACK_LEFT);
			draw_box(nullptr, deepred, 0.95f, 1.0f - 0.05f * glob->w / glob->h, 0.05f, 0.05f * glob->w / glob->h, -1);
			render_text("x", white, 0.962f, 1.015f - 0.05f * glob->w / glob->h, 0.0012f, 0.002f);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDrawBuffer(GL_BACK_LEFT);
			if (mainprogram->leftmouse)	mainprogram->quit("closed window");
		}
	
		if (!mainprogram->startloop) {
			//initial switch to live mode
			mainprogram->prevmodus = false;
			mainprogram->preveff_init();
			
			std::string reqdir = mainprogram->projdir;
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
					mainprogram->get_outname("New project", "application/ewocvj2-project", boost::filesystem::absolute(reqdir + name).string());
					if (mainprogram->path != nullptr) {
						mainprogram->project->newp(mainprogram->path);
						mainprogram->path = nullptr;
						mainprogram->startloop = true;
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
					mainprogram->get_inname("Open project", "application/ewocvj2-project", boost::filesystem::absolute(dir).string());
					if (mainprogram->path != nullptr) {
						mainprogram->project->open(mainprogram->path);
						mainprogram->path = nullptr;
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
						mainprogram->leftmouse = false;
						mainprogram->project->open(mainprogram->recentprojectpaths[i]);
						mainprogram->startloop = true;
						break;
					}
				}
				render_text(mainprogram->recentprojectpaths[i], white, box.vtxcoords->x1 + 0.015f, box.vtxcoords->y1 + 0.03f, 0.001f, 0.0016f);
				box.vtxcoords->y1 -= 0.125f;
				box.upvtxtoscr();
			}
				
			SDL_GL_SwapWindow(mainprogram->mainwindow);
			mainprogram->leftmouse = false;
		}
		else {
			std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
			elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - begintime);
			long long microcount = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
			mainmix->time = microcount / 1000000.0f;
			GLint iGlobalTime = glGetUniformLocation(mainprogram->ShaderProgram, "iGlobalTime");
			glUniform1f(iGlobalTime, mainmix->time);
			
			the_loop();
		}
	}

	mainprogram->quit("stop");

}


