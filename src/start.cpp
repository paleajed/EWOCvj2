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
#include	 FT_FREETYPE_H
#include	 FT_MODULE_H
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
SDL_GLContext glc_th;
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


#ifdef _WIN64
void SetProcessPriority(LPWSTR ProcessName, int Priority)
{
	PROCESSENTRY32 proc32;
	HANDLE hSnap;
	if (hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
	if (hSnap == INVALID_HANDLE_VALUE)
	{

	}
	else
	{
		proc32.dwSize = sizeof(PROCESSENTRY32);
		while ((Process32Next(hSnap, &proc32)) == TRUE)
		{
			if (_wcsicmp((wchar_t*)(proc32.szExeFile), ProcessName) == 0)
			{
				HANDLE h = OpenProcess(PROCESS_SET_INFORMATION, TRUE, proc32.th32ProcessID);
				SetPriorityClass(h, Priority);
				CloseHandle(h);
			}
		}
		CloseHandle(hSnap);
	}
}

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
  	
  	if (mainprogram->tunemidi) {
  		LayMidi *lm = nullptr;
  		if (mainprogram->tunemidiset == 1) lm = laymidiA;
  		else if (mainprogram->tunemidiset == 2) lm = laymidiB;
  		else if (mainprogram->tunemidiset == 3) lm = laymidiC;
  		else if (mainprogram->tunemidiset == 4) lm = laymidiD;
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
			lay->decresult->hap = false;
			lay->decresult->data = (char*) * (lay->rgbframe->extended_data);
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
		this->databuf = (char*)malloc(this->video_dec_ctx->width * this->video_dec_ctx->height * sizeof(int));
		this->databufready = true;
	}

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
		return false; 
	}
	size_t uncomp;
	int headerl = 4;
	if (*bptrData == 0 and *(bptrData + 1) == 0 and *(bptrData + 2) == 0) {
		headerl = 8;
	}
	this->decresult->compression = *(bptrData + 3);

	std::mutex lock;
	lock.lock();
	int st = -1;
	if (this->databufready) {
		st = snappy_uncompress(bptrData + headerl, size - headerl, this->databuf, &uncomp);
	}
	av_packet_unref(&this->decpkt);

	if (st != SNAPPY_OK) {
		this->decresult->data = nullptr;
		this->decresult->width = 0;
		lock.unlock();
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
	lock.unlock();
	return true;
}

void Layer::trigger() {
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
			bool r = thread_vidopen(this, nullptr, false);
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
		if (this->liveinput == nullptr) {
			if ((!this->initialized and this->decresult->width) or this->filename == "") continue;
			if (this->vidformat != 188 and this->vidformat != 187) {
				if ((int)(this->frame) != this->prevframe or this->type == ELEM_LIVE) {
					get_cpu_frame((int)this->frame, this->prevframe, 0);
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
				this->get_hap_frame();
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
	if (this->boundimage != -1) {
		glDeleteTextures(1, &this->boundimage);
		this->boundimage = -1;
	}

	if (!this->dummy) {
		ShelfElement* elem = this->prevshelfdragelem;
		bool ret = mainmix->set_prevshelfdragelem(this);
		if (!ret and elem->type == ELEM_DECK) {
			if (elem->launchtype == 2) {
				mainmix->mousedeck = elem->nbframes[0]->deck;
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
			this->inhibit(); // lay is passed over into only framecounting
			this->layers->erase(std::find(this->layers->begin(), this->layers->end(), this));
			Layer *lay = mainmix->add_layer(*this->layers, this->pos);
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
	this->vidopen = true;
	if (!this->isclone) this->decresult->width = 0;
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

bool thread_vidopen(Layer *lay, AVInputFormat *ifmt, bool skip) {
	if (lay->video) avformat_close_input(&lay->video);
	lay->video = nullptr;
	lay->video_dec_ctx = nullptr;
	lay->liveinput = nullptr;
		
	if (!skip) {
		bool foundnew = false;
		ptrdiff_t pos = std::find(mainprogram->busylayers.begin(), mainprogram->busylayers.end(), lay) - mainprogram->busylayers.begin();
		if (pos < mainprogram->busylayers.size()) {
			int size = mainprogram->mimiclayers.size();
			for (int i = 0; i < size; i++) {
				Layer *mlay = mainprogram->mimiclayers[i]; 
				if (mlay->liveinputpos != -1) continue;
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
	
	//av_opt_set_int(lay->video, "probesize2", INT_MAX, 0);
	lay->video = avformat_alloc_context();
	lay->video->flags |= AVFMT_FLAG_NONBLOCK;
	int r = avformat_open_input(&(lay->video), lay->filename.c_str(), ifmt, nullptr);
	printf("loading... %s\n", lay->filename.c_str());
	if (r < 0) {
		lay->filename = "";
		lay->openerr = true;
		printf("%s\n", "Couldnt open video");
		return 0;
	}

	//av_opt_set_int(lay->video, "max_analyze_duration2", 100000000, 0);
	if (avformat_find_stream_info(lay->video, nullptr) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        return 0;
    }
	lay->video->max_picture_buffer = 20000000;
	avformat_open_input(&(lay->videoseek), lay->filename.c_str(), ifmt, nullptr);

    if (open_codec_context(&(lay->video_stream_idx), lay->video, AVMEDIA_TYPE_VIDEO) >= 0) {
     	lay->video_stream = lay->video->streams[lay->video_stream_idx];
		AVCodec *dec = avcodec_find_decoder(lay->video_stream->codecpar->codec_id);
        lay->vidformat = lay->video_stream->codecpar->codec_id;
        lay->video_dec_ctx = avcodec_alloc_context3(dec);
		avcodec_parameters_to_context(lay->video_dec_ctx, lay->video_stream->codecpar);
		avcodec_open2(lay->video_dec_ctx, dec, nullptr);
		lay->bpp = 4;
		if (lay->vidformat == 188 or lay->vidformat == 187) {
			lay->numf = lay->video_stream->nb_frames;
			if (lay->numf == 0) {
				lay->numf = (double)lay->video->duration * (double)lay->video_stream->avg_frame_rate.num / (double)lay->video_stream->avg_frame_rate.den / (double)1000000.0f;
				lay->video_duration = lay->video->duration / (1000000.0f * lay->video_stream->time_base.num / lay->video_stream->time_base.den);
			}
			else lay->video_duration = lay->video_stream->duration;
			float tbperframe = (float)lay->video_stream->duration / (float)lay->numf;
			lay->millif = tbperframe * (((float)lay->video_stream->time_base.num * 1000.0) / (float)lay->video_stream->time_base.den);

			lay->startframe = 0;
			lay->endframe = lay->numf;
			if (0) { // lay->reset?
				lay->frame = 0.0f;
			}
			lay->decframe = av_frame_alloc();

			if (lay->databuf) free(lay->databuf);
			lay->databuf = nullptr;

			return 1;
        }
		else avformat_open_input(&(lay->videoseek), lay->filename.c_str(), ifmt, nullptr);
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

	if (lay->type != ELEM_LIVE) {
		lay->numf = lay->video_stream->nb_frames;
		if (lay->numf == 0) {
			lay->numf = (double)lay->video->duration * (double)lay->video_stream->avg_frame_rate.num / (double)lay->video_stream->avg_frame_rate.den / (double)1000000.0f;
			lay->video_duration = lay->video->duration / (1000000.0f * lay->video_stream->time_base.num / lay->video_stream->time_base.den);
		}
		else lay->video_duration = lay->video_stream->duration;
		if (lay->reset) {
			lay->startframe = 0;
			lay->endframe = lay->numf - 1;
		}
		//if (lay->numf == 0) return 0;	//reminder: implement!
		float tbperframe = (float)lay->video_duration / (float)lay->numf;
		lay->millif = tbperframe * (((float)lay->video_stream->time_base.num * 1000.0) / (float)lay->video_stream->time_base.den);
	}

	if (open_codec_context(&(lay->audio_stream_idx), lay->video, AVMEDIA_TYPE_AUDIO) >= 0 and !lay->dummy) {
		lay->audio_stream = lay->video->streams[lay->audio_stream_idx];
		AVCodec *dec = avcodec_find_decoder(lay->audio_stream->codecpar->codec_id);
        lay->audio_dec_ctx = avcodec_alloc_context3(dec);
		avcodec_parameters_to_context(lay->audio_dec_ctx, lay->audio_stream->codecpar);
		avcodec_open2(lay->audio_dec_ctx, dec, nullptr);
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
	glBufferData(GL_ARRAY_BUFFER, 32, vcoords, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
	glBindBuffer(GL_ARRAY_BUFFER, mainprogram->prboxtbuf);
	glBufferData(GL_ARRAY_BUFFER, 32, tcoords2, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
	glGenVertexArrays(1, &mainprogram->pr_texvao);
	glBindVertexArray(mainprogram->pr_texvao);
	glGenBuffers(1, &mainprogram->pr_rtvbo);
	glBindBuffer(GL_ARRAY_BUFFER, mainprogram->pr_rtvbo);
	glBufferStorage(GL_ARRAY_BUFFER, 32, nullptr, GL_DYNAMIC_STORAGE_BIT);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
	glGenBuffers(1, &mainprogram->pr_rttbo);
	glBindBuffer(GL_ARRAY_BUFFER, mainprogram->pr_rttbo);
	glBufferStorage(GL_ARRAY_BUFFER, 32, nullptr, GL_DYNAMIC_STORAGE_BIT);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
	SDL_GL_MakeCurrent(mainprogram->tunemidiwindow, glc_tm);
	glGenBuffers(1, &mainprogram->tmboxvbuf);
	glGenBuffers(1, &mainprogram->tmboxtbuf);
	glGenVertexArrays(1, &mainprogram->tmboxvao);
	glBindVertexArray(mainprogram->tmboxvao);
	glBindBuffer(GL_ARRAY_BUFFER, mainprogram->tmboxvbuf);
	glBufferData(GL_ARRAY_BUFFER, 32, vcoords, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
	glBindBuffer(GL_ARRAY_BUFFER, mainprogram->tmboxtbuf);
	glBufferData(GL_ARRAY_BUFFER, 32, tcoords2, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
	glGenVertexArrays(1, &mainprogram->tm_texvao);
	glBindVertexArray(mainprogram->tm_texvao);
	glGenBuffers(1, &mainprogram->tm_rtvbo);
	glBindBuffer(GL_ARRAY_BUFFER, mainprogram->tm_rtvbo);
	glBufferStorage(GL_ARRAY_BUFFER, 32, nullptr, GL_DYNAMIC_STORAGE_BIT);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
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
	glBufferStorage(GL_ARRAY_BUFFER, 32, nullptr, GL_DYNAMIC_STORAGE_BIT);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
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
	mainprogram->guielems.push_back(gelem);
}

void draw_line(gui_line *line) {
	GLint linetriangle = glGetUniformLocation(mainprogram->ShaderProgram, "linetriangle");
	glUniform1i(linetriangle, 1);
	GLint color = glGetUniformLocation(mainprogram->ShaderProgram, "color");
	glUniform4fv(color, 1, line->linec);
	GLfloat fvcoords[6] = { line->x1, line->y1, 0.0f, line->x2, line->y2, 0.0f};
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
	glUniform1i(linetriangle, 0);
	
    glDeleteBuffers(1, &fvbuf);
	glDeleteVertexArrays(1, &fvao);
}

void draw_direct(float* linec, float* areac, float x, float y, float wi, float he, float dx, float dy, float scale, float opacity, int circle, GLuint tex, float smw, float smh) {
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
			glUniform1f(pixelw, 2.0f / (wi * ((float)glob->w / 2.0f)));
			pixelh = glGetUniformLocation(mainprogram->ShaderProgram, "pixelh");
			glUniform1f(pixelh, 2.0f / (he * ((float)glob->h / 2.0f)));
		}
	}
	else {
		glUniform1f(pixelw, 0.0f);
	}

	if (opacity != 1.0f) {
		GLint opa = glGetUniformLocation(mainprogram->ShaderProgram, "opacity");
		glUniform1f(opa, opacity);
	}

	if (areac) {
		float shx = -dx * 6.0f;
		float shy = -dy * 6.0f;
		GLfloat fvcoords2[12] = {
			x,     y + he, 1.0,
			x,     y, 1.0,
			x + wi, y + he, 1.0,
			x + wi, y, 1.0,
		};
		glBindBuffer(GL_ARRAY_BUFFER, mainprogram->bvbuf);
		glBufferSubData(GL_ARRAY_BUFFER, 0, 48, fvcoords2);
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
	//glUniform1f(opa, 1.0f);
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
		if (text) glUniform1i(textmode, 1);
		draw_direct(linec, areac, x, y, wi, he, dx, dy, scale, opacity, circle, tex, smw, smh);
		if (text) glUniform1i(textmode, 0);
		return;
	}

	if (circle or mainprogram->frontbatch) {
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

	if (mainprogram->countingtexes[mainprogram->currbatch] == mainprogram->maxtexes - 2) {
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
	mainprogram->guielems.push_back(gelem);
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
	y -= 0.008f;
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

		glGenTextures(1, &texture);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexStorage2D(
			GL_TEXTURE_2D,
			1,
			GL_R8,
			2048,
			64
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
		float pixelw = 2.0f / glob->w;
		float pixelh = 2.0f / glob->h;
		float th;
		int pxprogress = 0;
		FT_Set_Pixel_Sizes(face, 0, (int)(sy * 24000.0f * glob->h / 1346.0f)); //
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
	
			GLuint glyphfrbuf;
			glGenFramebuffers(1, &glyphfrbuf);
			glBindFramebuffer(GL_FRAMEBUFFER, glyphfrbuf);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ftex, 0);
			glBlitNamedFramebuffer(glyphfrbuf, texfrbuf, 0, 0, g->bitmap.width, g->bitmap.rows, pxprogress, g->bitmap_top + 8, pxprogress + g->bitmap.width, g->bitmap_top - g->bitmap.rows + 8, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	
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
			textw += (g->advance.x/128.0f) * pixelh;
			textws.push_back((g->advance.x / 128.0f) * pixelh * ((smflag == 0) + 1) * 0.5f / 1.1f); //1.1 *
			texth = 64.0f * pixelh;
			th = std::max(th, (g->metrics.height / 64.0f) * pixelh);
		}

		//cropping texture
		int w = textw * 2.2f / pixelw; //2.2 *
		GLuint endtex;
		glGenTextures(1, &endtex);
		glBindTexture(GL_TEXTURE_2D, endtex);
		glTexStorage2D(
			GL_TEXTURE_2D,
			1,
			GL_R8,
			w,
			64
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
		gs->texthvec.push_back(th);
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
		float pixelw = 2.0f / glob->w;
		float pixelh = 2.0f / glob->h;
		float texth2 = 64.0f * pixelh;
		wfac = textw / pixelw / 512.0f;

		float wi = 0.0f;
		float he = 0.0f;
		if (vertical) {
			he = texth2 * 8.0f * wfac * glob->w / glob->h;
			wi = texth2 * glob->h / glob->w;	
		}
		else {
			wi = texth2 * 8.0f * wfac;
			he = -texth2;
		}
		y -= he;
			
		float black[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		if (textw != 0) draw_box(nullptr, black, x + 0.001f, y - 0.00185f, wi, he, texture, true, vertical);  //draw text shadow
		if (textw != 0) draw_box(nullptr, textc, x, y, wi, he, texture, true, vertical);	//draw text
	}

	mainprogram->texth = texth;
	
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
							this->clip_followup(0, alive);
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
							this->clip_followup(1, alive);
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
					set_live_base(this, this->filename);
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
		}
	}
	else return;

	if (!srclay->decresult->newdata) {
		return;
	}	
	srclay->decresult->newdata = false;


	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, this->texture);

	if (srclay->imageloaded >= 4 and srclay->type == ELEM_IMAGE and srclay->numf == 0) return;
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, srclay->pbo[srclay->pbodi]);
	if (!this->liveinput) {
		if ((!this->isclone)) {  // decresult contains new frame width, height, number of bitmaps and data
			if (!srclay->decresult->width) {
				return;
			}

			if (srclay->decresult->hap and srclay->video_dec_ctx) {
				if (!srclay->databufready) return;

				// HAP video layers
				if (!this->initialized) {
					float w = srclay->video_dec_ctx->width;
					float h = srclay->video_dec_ctx->height;
					this->initialize(w, h, srclay->decresult->compression);
				}
				if (srclay->decresult->compression == 187 or srclay->decresult->compression == 171) {
					if (srclay->decresult->data) glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, srclay->decresult->width, srclay->decresult->height, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, srclay->decresult->size, 0);
				}
				else if (srclay->decresult->compression == 190) {
					if (srclay->decresult->data) glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, srclay->decresult->width, srclay->decresult->height, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, srclay->decresult->size, 0);
				}
			}
			else {
				if (this->initialized and srclay->type == ELEM_IMAGE) {
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
					if (!this->initialized) {
						float w = this->video_dec_ctx->width;
						float h = this->video_dec_ctx->height;
						this->initialize(w, h);
					}
					glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, srclay->decresult->width, srclay->decresult->height, GL_BGRA, GL_UNSIGNED_BYTE, 0);
				}
			}
		}
	}
	else {
		if (!this->initialized) {
			float w = this->liveinput->video_dec_ctx->width;
			float h = this->liveinput->video_dec_ctx->height;
			this->initialize(w, h);
		}
	}

	if (!this->isclone) {
		// start transferring to pbou
		// bind PBO to update pixel values
		//glBindBuffer(GL_PIXEL_UNPACK_BUFFER, srclay->pbo[srclay->pboui]);
		if (srclay->mapptr[srclay->pbodi]) {
			WaitBuffer(srclay->syncobj[srclay->pbodi]);
			// update data directly on the mapped buffer
			memcpy(srclay->mapptr[srclay->pbodi], srclay->decresult->data, srclay->decresult->size);
			LockBuffer(srclay->syncobj[srclay->pbodi]);
		}
		// round robin triple pbos: currently deactivated
		srclay->pbodi++;
		srclay->pboui++;
		if (srclay->pbodi == 3) srclay->pbodi = 0;
		if (srclay->pboui == 3) srclay->pboui = 0;

		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	}
	else this->initialized = true;  //reminder: trick, is set false somewhere
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
	
	
void display_layers(Layer *lay, bool deck) {
	float green[4] = {0.0f, 1.0f, 0.2f, 1.0f};
	float darkgreen1[4] = {0.0f, 0.75f, 0.0f, 1.0f};
	float darkgreen2[4] = {0.0f, 0.4f, 0.0f, 1.0f};
	float lightblue[4] = { 0.5f, 0.5f, 1.0f, 1.0f };
	float blue[4] = { 0.0f, 0.2f, 1.0f, 1.0f };
	float white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	float alphawhite[4] = { 1.0f, 1.0f, 1.0f, 0.5f };
	float alphablue[4] = { 0.7f, 0.7f, 1.0f, 0.5f };
	float alphagreen[4] = { 0.0f, 1.0f, 0.2f, 0.5f };
	float red[4] = {1.0f, 0.0f, 0.0f, 1.0f};
	float darkred1[4] = {0.75f, 0.0f, 0.0f, 1.0f};
	float darkred2[4] = {0.4f, 0.0f, 0.0f, 0.5f};
	float black[4] = {0.0f, 0.0f, 0.0f, 1.0f};

	glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
	mainprogram->drawbuffer = mainprogram->globfbo;
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	
	std::vector<Layer*>& lvec = choose_layers(deck);
	bool notyet = false;
	for (int i = 0; i < lvec.size(); i++) {
		if (!lvec[i]->newtexdata) {
			notyet = true;
			break;
		}
	}
	if (notyet and lay->filename != "") {
		if (lay->node == lay->lasteffnode[0]) {
			lay->drawfbo2 = true;
		}
		else {
			lay->drawfbo2 = false;
		}
		if (mainmix->bulrs[lay->deck].size() > lay->pos) {
			lay->node->vidbox->tex = mainmix->butexes[lay->deck][lay->pos];
		}
	}
	if (mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos > lvec.size() - 2) mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos = lvec.size() - 2;
	if (mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos < 0) mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos = 0;
	if (lay->pos >= mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos and lay->pos < mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos + 3) {
		Box *box = lay->node->vidbox;
		if (!lay->mutebut->value) {
			draw_box(box, box->tex);
		}
		else {
			draw_box(box, box->tex);
			draw_box(white, darkred2, box, -1);
		}

		if (mainmix->mode == 0 and mainprogram->nodesmain->linked) {
			// Trigger mainprogram->laymenu1
			if (box->in()) {
				std::string deckstr;
				if (deck == 0) deckstr = "A";
				else if (deck == 1) deckstr = "B";
				render_text("Layer " + deckstr + std::to_string(lay->pos + 1), white, box->vtxcoords->x1 + tf(0.01f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.03f), 0.0005f, 0.0008f);
				GLenum err;
				while ((err = glGetError()) != GL_NO_ERROR) {
					cerr << "OpenGL error2: " << err << endl;
				}
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
					}
				}

				//draw and handle layer mute and solo buttons
				lay->mutebut->box->vtxcoords->x1 = box->vtxcoords->x1 + mainprogram->layw - 0.06f;
				lay->mutebut->box->upvtxtoscr();
				if (lay->mutebut->box->in()) {
					render_text("M", alphablue, lay->mutebut->box->vtxcoords->x1 + 0.0078f, lay->mutebut->box->vtxcoords->y1 + 0.0078f, 0.0006, 0.001);
					if (mainprogram->leftmousedown) {
						lay->muting = true;
						mainprogram->leftmousedown = false;
					}
					if (lay->muting and mainprogram->leftmouse) {
						lay->muting = false;
						lay->mutebut->value = !lay->mutebut->value;
						lay->mute_handle();
					}
					if (mainprogram->menuactivation and !mainprogram->menuondisplay) {
						mainprogram->parammenu1->state = 2;
						mainmix->learnparam = nullptr;
						mainmix->learnbutton = lay->mutebut;
						mainprogram->menuactivation = false;
					}
				}
				else if (!lay->mutebut->value) {
					render_text("M", alphawhite, lay->mutebut->box->vtxcoords->x1 + 0.0078f, lay->mutebut->box->vtxcoords->y1 + 0.0078f, 0.0006, 0.001);
				}
				lay->solobut->box->vtxcoords->x1 = lay->mutebut->box->vtxcoords->x1 + 0.03f;
				lay->solobut->box->upvtxtoscr();
				if (lay->solobut->box->in()) {
					render_text("S", alphablue, lay->solobut->box->vtxcoords->x1 + 0.0078f, lay->solobut->box->vtxcoords->y1 + 0.0078f, 0.0006, 0.001);
					if (mainprogram->leftmousedown) {
						lay->soloing = true;
						mainprogram->leftmousedown = false;
					}
					if (lay->soloing and mainprogram->leftmouse) {
						mainprogram->leftmouse = false;
						lay->soloing = false;
						lay->solobut->value = !lay->solobut->value;
						if (lay->solobut->value) {
							if (lay->mutebut->value) {
								lay->mutebut->value = false;
								lay->mute_handle();
							}
							for (int k = 0; k < lvec.size(); k++) {
								if (k != lay->pos) {
									if (lvec[k]->mutebut->value == false) {
										lvec[k]->mutebut->value = true;
										lvec[k]->mute_handle();
									}
									else {
										lvec[k]->mutebut->value = false;
										lvec[k]->mute_handle();
										lvec[k]->mutebut->value = true;
										lvec[k]->mute_handle();
									}
									lvec[k]->solobut->value = false;
								}
							}
						}
						else {
							for (int k = 0; k < lvec.size(); k++) {
								if (k != lay->pos) {
									lvec[k]->mutebut->value = false;
									lvec[k]->mute_handle();
								}
							}
						}
					}
					if (mainprogram->menuactivation and !mainprogram->menuondisplay) {
						mainprogram->parammenu1->state = 2;
						mainmix->learnparam = nullptr;
						mainmix->learnbutton = lay->solobut;
						mainprogram->menuactivation = false;
					}
				}
				else if (!lay->solobut->value) {
					render_text("S", alphawhite, lay->solobut->box->vtxcoords->x1 + 0.0078f, lay->solobut->box->vtxcoords->y1 + 0.0078f, 0.0006, 0.001);
				}

				// queue fold/unfold button
				lay->queuebut->box->vtxcoords->x1 = box->vtxcoords->x1 + 0.015f;
				lay->queuebut->box->upvtxtoscr();
				if (lay->queuebut->box->in()) {
					render_text("I", alphablue, lay->queuebut->box->vtxcoords->x1 + 0.0108f, lay->queuebut->box->vtxcoords->y1 + 0.018f, 0.0006, 0.001);
					render_text("V", alphablue, lay->queuebut->box->vtxcoords->x1 + 0.0078f, lay->queuebut->box->vtxcoords->y1 + 0.0078f, 0.0006, 0.001);
					if (mainprogram->leftmousedown) {
						lay->mousequeue = true;
						mainprogram->leftmousedown = false;
					}
					if (lay->mousequeue and mainprogram->leftmouse) {
						lay->mousequeue = false;
						mainprogram->leftmouse = false;
						lay->queueing = !lay->queueing;
						if (lay->queueing) mainprogram->queueing = true;
						else {
							bool found = false;
							for (int i = 0; i < 2; i++) {
								std::vector<Layer*>& lvec = choose_layers(i);
								for (int j = 0; j < lvec.size(); j++) {
									if (lvec[j]->queueing) {
										found = true;
										break;
									}
								}
							}
							if (!found)	mainprogram->queueing = false;
						}
					}
				}
				else {
					render_text("I", alphawhite, lay->queuebut->box->vtxcoords->x1 + 0.0108f, lay->queuebut->box->vtxcoords->y1 + 0.018f, 0.0006, 0.001);
					render_text("V", alphawhite, lay->queuebut->box->vtxcoords->x1 + 0.0078f, lay->queuebut->box->vtxcoords->y1 + 0.0078f, 0.0006, 0.001);
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
			if (lay->mutebut->value) {
				render_text("M", alphagreen, lay->mutebut->box->vtxcoords->x1 + 0.0078f, lay->mutebut->box->vtxcoords->y1 + 0.0078f, 0.0006, 0.001);
			}
			if (lay->solobut->value) {
				render_text("S", alphagreen, lay->solobut->box->vtxcoords->x1 + 0.0078f, lay->solobut->box->vtxcoords->y1 + 0.0078f, 0.0006, 0.001);
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
			if (mainmix->currlay->deck == 0) {
				lay->effscroll[cat] = mainprogram->handle_scrollboxes(mainprogram->effscrollupA, mainprogram->effscrolldownA, lay->numefflines[cat], lay->effscroll[cat], 10);
			}
			else {
				lay->effscroll[cat] = mainprogram->handle_scrollboxes(mainprogram->effscrollupB, mainprogram->effscrolldownB, lay->numefflines[cat], lay->effscroll[cat], 10);
			}
			if (lay->effects[cat].size()) {
				if ((glob->w / 2.0f > mainprogram->mx and mainmix->currlay->deck == 0) or (glob->w / 2.0f < mainprogram->mx and mainmix->currlay->deck == 1)) {
					if (mainprogram->my > mainprogram->yvtxtoscr(mainprogram->layh - tf(0.20f))) {
						if (mainprogram->mousewheel and lay->numefflines[cat] > 10) {
							lay->effscroll[cat] -= mainprogram->mousewheel;
							if (lay->effscroll[cat] < 0) lay->effscroll[cat] = 0;
							if (lay->numefflines[cat] - lay->effscroll[cat] < 10) lay->effscroll[cat] = lay->numefflines[cat] - 10;
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
				x1 = eff->box->vtxcoords->x1 + tf(0.032f);
				wi = (0.7f - mainprogram->numw - tf(0.032f)) / 4.0f;

				if (eff->box->vtxcoords->y1 < 1.0 - tf(mainprogram->layh) - tf(0.22f) - tf(0.05f) * 9) break;
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
					effstr = eff->get_namestring();
					float textw = tf(textwvec_total(render_text(effstr, white, eff->box->vtxcoords->x1 + tf(0.01f), eff->box->vtxcoords->y1 + tf(0.05f) - tf(0.030f), tf(0.0003f), tf(0.0005f))));
					eff->box->vtxcoords->w = textw + tf(0.032f);
					x1 = eff->box->vtxcoords->x1 + tf(0.032f) + textw;
					wi = (0.7f - mainprogram->numw - tf(0.032f) - textw) / 4.0f;
				}
				y1 = eff->box->vtxcoords->y1;
				for (int j = 0; j < eff->params.size(); j++) {
					Param* par = eff->params[j];
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

					if (par->box->vtxcoords->y1 < 1.0 - tf(mainprogram->layh) - tf(0.22f) - tf(0.05f) * 9) break;
					if (par->box->vtxcoords->y1 <= 1.0 - tf(mainprogram->layh) - tf(0.18f)) {
						par->handle();
					}
				}
			}

			// Draw effectboxes
			// Bottom bar
			float sx1, sy1, vx1, vy1, vx2, vy2, sw;
			bool bottom = false;
			bool inbetween = false;
			Effect* eff = nullptr;
			if (!mainprogram->queueing) {
				if (evec.size()) {
					eff = evec[evec.size() - 1];
					box = eff->onoffbutton->box;
					sx1 = box->scrcoords->x1;
					sy1 = box->scrcoords->y1 + (eff->numrows - 1) * mainprogram->yvtxtoscr(tf(0.05f));
					if (1.0f - mainprogram->yscrtovtx(sy1) < -0.49f) {
						sy1 = mainprogram->yvtxtoscr(1.49f);
					}
					vx1 = box->vtxcoords->x1;
					vy1 = box->vtxcoords->y1 - (eff->numrows - 1) * tf(0.05f);
					if (vy1 < -0.49f) vy1 = -0.49f;
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
							if ((mainprogram->menuactivation or mainprogram->leftmouse) and !mainprogram->drageff) {
								mainprogram->effectmenu->state = 2;
								mainmix->insert = 1;
								mainmix->mouseeffect = evec.size();
								mainmix->mouselayer = lay;
								mainprogram->effectmenu->menux = mainprogram->mx;
								mainprogram->effectmenu->menuy = mainprogram->my;
								mainprogram->menuactivation = false;
								mainprogram->menuondisplay = true;
							}
							bottom = true;
						}
					}
				}
				mainprogram->indragbox = false;
				for (int j = 0; j < evec.size(); j++) {
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
								if ((mainprogram->menuactivation or mainprogram->leftmouse) and !mainprogram->drageff) {
									mainprogram->effectmenu->state = 2;
									mainmix->mouselayer = lay;
									mainmix->mouseeffect = j;
									mainmix->insert = false;
									mainprogram->effectmenu->menux = mainprogram->mx;
									mainprogram->effectmenu->menuy = mainprogram->my;
									mainprogram->menuactivation = false;
									mainprogram->menuondisplay = true;
									mainprogram->dragbox = nullptr;
									mainprogram->drageffsense = false;
								}
								eff->box->acolor[0] = 0.5;
								eff->box->acolor[1] = 0.5;
								eff->box->acolor[2] = 1.0;
								eff->box->acolor[3] = 1.0;

								// prepare effect dragging
								eff->pos = j;
								if (!mainprogram->drageff) {
									if (mainprogram->leftmousedown and !mainprogram->drageffsense) {
										mainprogram->drageffsense = true;
										mainprogram->dragbox = eff->box;
										mainprogram->drageffpos = j;
										mainprogram->leftmousedown = false;
									}
									if (j == mainprogram->drageffpos) {
										mainprogram->indragbox = true;
									}
								}
							}
						}
						box = eff->onoffbutton->box;
						if (box->scrcoords->x1 < mainprogram->mx and mainprogram->mx < box->scrcoords->x1 + tf(mainprogram->layw) * glob->w / 2.0) {
							if (box->scrcoords->y1 - box->scrcoords->h - 7.5 < mainprogram->my and mainprogram->my < box->scrcoords->y1 - box->scrcoords->h + 7.5) {
								// mouse over "Insert Effect" box, inbetween effects
								if ((mainprogram->menuactivation or mainprogram->leftmouse) and !mainprogram->drageff) {
									mainprogram->effectmenu->state = 2;
									mainmix->insert = true;
									mainmix->mouseeffect = j;
									mainmix->mouselayer = lay;
									mainprogram->effectmenu->menux = mainprogram->mx;
									mainprogram->effectmenu->menuy = mainprogram->my;
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
				if (!mainprogram->indragbox and mainprogram->drageffsense) {
					mainprogram->drageff = evec[mainprogram->drageffpos];
					mainprogram->drageffsense = false;
				}
			}

			// Draw effectmenuhints
			if (!mainprogram->queueing) {
				if (vy1 < 1.0 - tf(mainprogram->layh) - tf(0.22f) - tf(0.05f) * 9) {
					vy1 = 1.0 - tf(mainprogram->layh) - tf(0.20f) - tf(0.05f) * 9;
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

			// handle effect drag
			if (mainprogram->drageff) {
				int pos;
				for (int j = lay->effscroll[cat]; j < evec.size() + 1; j++) {
					// calculate dragged effect temporary position pos when mouse between
					//limits under and upper
					int under1, under2, upper;
					if (j == lay->effscroll[cat]) {
						// mouse above first bin
						under2 = 0;
						under1 = evec[lay->effscroll[cat]]->box->scrcoords->y1 - evec[lay->effscroll[cat]]->box->scrcoords->h * 0.5f;
					}
					else {
						under1 = evec[j - 1]->box->scrcoords->y1 + evec[j - 1]->box->scrcoords->h * 0.5f;
						under2 = under1;
					}
					if (j == evec.size()) {
						// mouse under last bin
						upper = glob->h;
					}
					else upper = evec[j]->box->scrcoords->y1 + evec[j]->box->scrcoords->h * 0.5f;
					if (mainprogram->my > under2 and mainprogram->my < upper) {
						std::string name = mainprogram->drageff->get_namestring();
						draw_box(white, darkred1, mainprogram->drageff->box->vtxcoords->x1, 1.0f - mainprogram->yscrtovtx(under1), mainprogram->drageff->box->vtxcoords->w, mainprogram->drageff->box->vtxcoords->h, -1);
						render_text(name, white, mainprogram->drageff->box->vtxcoords->x1 + tf(0.01f), 1.0f - mainprogram->yscrtovtx(under1) + tf(0.05f) - tf(0.030f), tf(0.0003f), tf(0.0005f));
						pos = j;
						break;
					}
				}
				if (mainprogram->lmover) {
					// do effect drag
					Node* bulastoutnode = lay->lasteffnode[0]->out[0];
					if (mainprogram->drageffpos < pos) {
						std::rotate(evec.begin() + mainprogram->drageffpos, evec.begin() + mainprogram->drageffpos + 1, evec.begin() + pos);
					}
					else {
						std::rotate(evec.begin() + pos, evec.begin() + mainprogram->drageffpos, evec.begin() + mainprogram->drageffpos + 1);
					}
					for (int k = 0; k < evec.size(); k++) {
						// set pos and box y1 and connect nodes for all effects in new list
						if (k == 0) {
							mainprogram->nodesmain->currpage->connect_nodes(lay->node, evec[0]->node);
						}
						else if (k < evec.size()) {
							mainprogram->nodesmain->currpage->connect_nodes(evec[k-1]->node, evec[k]->node);
						}
						if (k == evec.size() - 1) {
							lay->lasteffnode[0] = evec[k]->node;
							mainprogram->nodesmain->currpage->connect_nodes(evec[k]->node, bulastoutnode);
						}
						if (lay->pos == 0 and lay->effects[1].size() == 0) {
							lay->lasteffnode[1] = lay->lasteffnode[0];
						}
						evec[k]->pos = k;
						//evec->box->vtxcoords->y1 = (i + 1) * -0.05f;
						//evec->box->upvtxtoscr();
					}
					mainprogram->drageff = nullptr;
				}
				if (mainprogram->rightmouse) {
					// cancel bin drag
					mainprogram->drageff = nullptr;
					mainprogram->rightmouse = false;
					mainprogram->menuactivation = false;
					for (int i = 0; i < mainprogram->menulist.size(); i++) {
						mainprogram->menulist[i]->state = 0;
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
			register_triangle_draw(white, white, lay->playbut->box->vtxcoords->x1 + tf(0.0078f), lay->playbut->box->vtxcoords->y1 + tf(0.0416f) - tf(0.030f), tf(0.011f), tf(0.0208f), RIGHT, CLOSED);
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
			register_triangle_draw(white, white, lay->revbut->box->vtxcoords->x1 + tf(0.00625f), lay->revbut->box->vtxcoords->y1 + tf(0.0416f) - tf(0.030f), tf(0.011f), tf(0.0208f), LEFT, CLOSED);
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
			register_triangle_draw(white, white, lay->bouncebut->box->vtxcoords->x1 + tf(0.00625f), lay->bouncebut->box->vtxcoords->y1 + tf(0.0416f) - tf(0.030f), tf(0.0078f), tf(0.0208f), LEFT, CLOSED);
			register_triangle_draw(white, white, lay->bouncebut->box->vtxcoords->x1 + tf(0.017f), lay->bouncebut->box->vtxcoords->y1 + tf(0.0416f) - tf(0.030f), tf(0.0078f), tf(0.0208f), RIGHT, CLOSED);
			
			// Draw and handle frameforward framebackward
			if (lay->frameforward->box->in()) {
				lay->frameforward->box->acolor[0] = 0.5;
				lay->frameforward->box->acolor[1] = 0.5;
				lay->frameforward->box->acolor[2] = 1.0;
				lay->frameforward->box->acolor[3] = 1.0;
				if (mainprogram->leftmouse) {
					lay->frame += 1;
					if (lay->frame >= lay->numf) lay->frame = 0.0f;
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
			register_triangle_draw(white, white, lay->frameforward->box->vtxcoords->x1 + tf(0.0075f), lay->frameforward->box->vtxcoords->y1 + tf(0.0416f) - tf(0.030f), tf(0.011f), tf(0.0208f), RIGHT, OPEN);
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
			register_triangle_draw(white, white, lay->framebackward->box->vtxcoords->x1 + tf(0.00625f), lay->framebackward->box->vtxcoords->y1 + tf(0.0416f) - tf(0.030f), tf(0.011f), tf(0.0208f), LEFT, OPEN);
		
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
				else if (lay->frame >= lay->numf) lay->frame = lay->numf - 1;
				lay->set_clones();
				if (mainprogram->leftmouse and !mainprogram->menuondisplay) lay->scritching = 4;
			}
			else if (lay->scritching == 2) {
				lay->startframe = (lay->numf - 1) * ((mainprogram->mx - lay->loopbox->scrcoords->x1) / lay->loopbox->scrcoords->w);
				if (lay->startframe < 0) lay->startframe = 0.0f;
				else if (lay->startframe >= lay->numf) lay->startframe = lay->numf - 1;
				if (lay->startframe > lay->frame) lay->frame = lay->startframe;
				if (lay->startframe > lay->endframe) lay->startframe = lay->endframe;
				lay->set_clones();
				if (mainprogram->middlemouse) lay->scritching = 0;
			}
			else if (lay->scritching == 3) {
				lay->endframe = (lay->numf - 1) * ((mainprogram->mx - lay->loopbox->scrcoords->x1) / lay->loopbox->scrcoords->w);
				if (lay->endframe < 0) lay->endframe = 0.0f;
				else if (lay->endframe >= lay->numf) lay->endframe = lay->numf - 1;
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
				else if (lay->startframe >= lay->numf) lay->startframe = lay->numf - 1;
				if (lay->startframe > lay->frame) lay->frame = lay->startframe;
				if (lay->startframe > lay->endframe) lay->startframe = lay->endframe;
				lay->endframe += (mainprogram->mx - mainmix->prevx) / (lay->loopbox->scrcoords->w / (lay->numf - 1)) - start;
				if (lay->endframe < 0) lay->endframe = 0.0f;
				else if (lay->endframe >= lay->numf) {
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
			glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
			mainprogram->drawbuffer = mainprogram->globfbo;
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			draw_box(lay->loopbox, -1);
			if (ends) {
				draw_box(white, blue, lay->loopbox->vtxcoords->x1 + lay->startframe * (lay->loopbox->vtxcoords->w / (lay->numf - 1)), lay->loopbox->vtxcoords->y1, (lay->endframe - lay->startframe) * (lay->loopbox->vtxcoords->w / (lay->numf - 1)), tf(0.05f), -1);
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
		but->value = (int)(mainmix->midi2 / 64);
		if (but == mainprogram->wormhole1 or but == mainprogram->wormhole2) mainprogram->binsscreen = but->value;
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
						if (!lvec[i]->newtexdata) {
							notyet = true;
							break;
						}
					}
					if (!notyet) {
						mainmix->bulrscopy[m] = mainmix->bulrs[m];
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
		
		// handle dragging things into clip queue
		if (lay->queueing and lay->filename != "") {
			if (box->scrcoords->x1 < mainprogram->mx and mainprogram->mx < box->scrcoords->x1 + box->scrcoords->w) {
				for (int j = 0; j < lay->clips.size() - lay->queuescroll; j++) {
					float last = (j == lay->clips.size() - lay->queuescroll - 1) * 0.25f;
					if (box->scrcoords->y1 + (j - 0.25f) * box->scrcoords->h < mainprogram->my and mainprogram->my < box->scrcoords->y1 + (j + 0.25) * box->scrcoords->h) {
						if (j == 0) {
							if (box->scrcoords->x1 + mainprogram->xvtxtoscr(0.075f) > mainprogram->mx or mainprogram->mx > box->scrcoords->x1 + box->scrcoords->w - mainprogram->xvtxtoscr(0.075f)) {
								continue;
							}
						}
						// inserting
						if (mainprogram->lmover) {
							if (mainprogram->dragbinel) {
								if (mainprogram->draglay == lay and j == 0) {
									enddrag();
									return true;
								}
							}
							if (mainprogram->dragbinel) {
								Clip *nc = new Clip;
								nc->tex = copy_tex(mainprogram->dragbinel->tex);
								nc->type = mainprogram->dragbinel->type;
								nc->path = mainprogram->dragbinel->path;
								if (nc->type == ELEM_IMAGE) {
									nc->get_imageframes();
								}
								else if (nc->type == ELEM_FILE) {
									nc->get_videoframes();
								}
								else if (nc->type == ELEM_LAYER) {
									nc->get_layerframes();
								}
								lay->clips.insert(lay->clips.begin() + j + lay->queuescroll, nc);
								enddrag();
								if (j + lay->queuescroll == lay->clips.size() - 1) {
									Clip *clip = new Clip;
									lay->clips.push_back(clip);
									if (lay->clips.size() > 4) lay->queuescroll++;
								}
								if (mainprogram->dragclip) mainprogram->dragclip = nullptr;								return true;
							}
						}
					}
					else if (box->scrcoords->y1 + (j + 0.25f) * box->scrcoords->h < mainprogram->my and mainprogram->my < box->scrcoords->y1 + (j + 0.75 + last) * box->scrcoords->h) {
						// replacing
						Clip* jc = lay->clips[j + lay->queuescroll];
						if (mainprogram->lmover) {
							if (mainprogram->dragbinel) {
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
								if (jc->type == ELEM_IMAGE) {
									jc->get_imageframes();
								}
								else if (jc->type == ELEM_FILE) {
									jc->get_videoframes();
								}
								else if (jc->type == ELEM_LAYER) {
									jc->get_layerframes();
								}
								enddrag();
								if (j + lay->queuescroll == lay->clips.size() - 1) {
									Clip* clip = new Clip;
									lay->clips.push_back(clip);
									if (lay->clips.size() > 4) lay->queuescroll++;
								}
								return true;
							}
						}
						if (mainprogram->menuactivation) {
							mainprogram->clipmenu->state = 2;
							mainmix->mouseclip = jc;
							mainprogram->menuactivation = false;
						}
					}
				}
			}
		}

		if (!mainmix->moving) {
			bool no = false;
			if (mainprogram->dragbinel) {
				if (mainprogram->dragbinel->type == ELEM_DECK or mainprogram->dragbinel->type == ELEM_MIX) {
					no = true;
				}
			}
			if (!no) {
				if (box->scrcoords->y1 < mainprogram->my + box->scrcoords->h and mainprogram->my < box->scrcoords->y1) {
					if (box->scrcoords->x1 + box->scrcoords->w * 0.25f < mainprogram->mx and mainprogram->mx < box->scrcoords->x1 + box->scrcoords->w * 0.75f) {
						// handle dragging things into layer monitors of deck
						lay->queueing = true;
						mainprogram->queueing = true;
						mainmix->currlay = lay;
						if (mainprogram->lmover or mainprogram->laymenu1->state > 1 or mainprogram->laymenu2->state > 1 or mainprogram->loadmenu->state > 1) {
							set_queueing(false);
							if (mainprogram->dragbinel) {
								mainprogram->laymenu1->state = 0;
								mainprogram->laymenu2->state = 0;
								mainprogram->loadmenu->state = 0;
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
											mainprogram->shelfdragelem->nbframes.clear();
											mainprogram->shelfdragelem->nbframes.push_back(lay);
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
										if (mainprogram->shelfdragelem->nbframes.size()) {
											lay->frame = mainprogram->shelfdragelem->nbframes[0]->frame;
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
							return 1;
						}
					}
					else if (endx or (box->scrcoords->x1 - mainprogram->xvtxtoscr(0.075f) < mainprogram->mx and mainprogram->mx < box->scrcoords->x1 + mainprogram->xvtxtoscr(0.075f))) {
						// handle dragging things to the end of the last visible layer monitor of deck
						if (mainprogram->lmover) {
							Layer* inlay = mainmix->add_layer(layers, lay->pos + endx);
							if (inlay->pos == mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos + 3) mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos++;
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
							return 1;
						}
					}
				}

				int numonscreen = layers.size() - mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos;
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
								return 1;
							}
						}
					}
				}
			}
		}
	}
	return 0;
}


void drag_into_layerstack(std::vector<Layer*>& layers, bool deck) {
	Layer* lay;
	for (int i = 0; i < layers.size(); i++) {
		lay = layers[i];
		if (lay->pos < mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos or lay->pos > mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos + 2) continue;
		Box* box = lay->node->vidbox;
		int endx = false;
		if ((i == layers.size() - 1 or i == mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos + 2) and (box->scrcoords->x1 + box->scrcoords->w - mainprogram->xvtxtoscr(tf(0.075f)) < mainprogram->mx and mainprogram->mx < box->scrcoords->x1 + box->scrcoords->w + mainprogram->xvtxtoscr(0.075f))) {
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
				if (box->scrcoords->y1 < mainprogram->my + box->scrcoords->h and mainprogram->my < box->scrcoords->y1) {
					if (box->scrcoords->x1 + box->scrcoords->w * 0.25f < mainprogram->mx and mainprogram->mx < box->scrcoords->x1 + box->scrcoords->w * 0.75f) {
						// handle dragging things into layer monitors of deck
						lay->queueing = true;
						mainprogram->queueing = true;
						mainmix->currlay = lay;
						if (mainprogram->lmover or mainprogram->laymenu1->state > 1 or mainprogram->laymenu2->state > 1 or mainprogram->loadmenu->state > 1) {
							set_queueing(false);
							if (mainprogram->dragbinel) {
								mainprogram->laymenu1->state = 0;
								mainprogram->laymenu2->state = 0;
								mainprogram->loadmenu->state = 0;
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
											mainprogram->shelfdragelem->nbframes.clear();
											mainprogram->shelfdragelem->nbframes.push_back(lay);
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
										if (mainprogram->shelfdragelem->nbframes.size()) {
											lay->frame = mainprogram->shelfdragelem->nbframes[0]->frame;
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
							if (inlay->pos == mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos + 3) mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos++;
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
				}

				int numonscreen = layers.size() - mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos;
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
	float white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float orange[] = { 1.0f, 0.5f, 0.0f, 1.0f };
	float purple[] = { 0.5f, 0.5f, 1.0f, 1.0f };
	float green[] = { 0.5f, 1.0f, 0.5f, 1.0f };
	float yellow[] = { 0.9f, 0.8f, 0.0f, 1.0f };
	float darkblue[] = { 0.1f, 0.5f, 1.0f, 1.0f };
	float red[] = { 1.0f, 0.0f, 0.0f, 1.0f };
	float grey[] = { 0.4f, 0.4f, 0.4f, 1.0f };
	float pink[] = { 1.0f, 0.5f, 0.5f, 1.0f };
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
		for (int j = 0; j < elem->nbframes.size(); j++) {
			elem->nbframes[j]->calc_texture(!mainprogram->prevmodus, 0);
		}

		if (elem->button->box->in()) {
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

void clip_intest(std::vector<Layer*>& layers, bool deck) {
	// test if inside a clip
	Layer* lay;
	mainprogram->inclips = -1;
	for (int i = 0; i < layers.size(); i++) {
		lay = layers[i];
		if (lay->pos < mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos or lay->pos > mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos + 2) continue;
		Box* box = lay->node->vidbox;
		if (lay->queueing and lay->filename != "") {
			if (box->scrcoords->x1 < mainprogram->mx and mainprogram->mx < box->scrcoords->x1 + box->scrcoords->w) {
				int limit = lay->clips.size() - lay->queuescroll;
				if (limit > 4) limit = 4;
				for (int j = 0; j < limit; j++) {
					Clip* jc = lay->clips[j + lay->queuescroll];
					jc->box->vtxcoords->x1 = box->vtxcoords->x1;
					jc->box->vtxcoords->y1 = box->vtxcoords->y1 - (j + 1) * box->vtxcoords->h;
					jc->box->vtxcoords->w = box->vtxcoords->w;
					jc->box->vtxcoords->h = box->vtxcoords->h;
					jc->box->upvtxtoscr();
					if (jc->box->in()) {
						if (mainprogram->menuactivation) {
							mainprogram->clipmenu->state = 2;
							mainmix->mouseclip = jc;
							mainmix->mouselayer = lay;
							mainprogram->menuactivation = false;
						}
						else mainprogram->inclips = i;
						break;
					}
				}
			}
		}
	}
}

void clip_dragging() {
	//handle dragging into clips
	if (mainprogram->dragbinel) {
		if (mainprogram->dragbinel->type != ELEM_DECK and mainprogram->dragbinel->type != ELEM_MIX) {
			if (mainprogram->prevmodus) {
				if (handle_clips(mainmix->layersA, 0)) return;
				if (handle_clips(mainmix->layersB, 1)) return;
			}
			else {
				if (handle_clips(mainmix->layersAcomp, 0)) return;
				if (handle_clips(mainmix->layersBcomp, 1)) return;
			}
			if (!mainprogram->binsscreen) {
				if (mainprogram->lmover) {
					// leftmouse dragging clip into nothing destroys the dragged clip
					bool inlayers = false;
					for (int deck = 0; deck < 2; deck++) {
						if (mainprogram->xvtxtoscr(mainprogram->numw + deck * 1.0f) < mainprogram->mx and mainprogram->mx < deck * (glob->w / 2.0f) + glob->w / 2.0f) {
							if (0 < mainprogram->my and mainprogram->my < mainprogram->yvtxtoscr(mainprogram->layh)) {
								// but not when dragged into layer stack field, even when there's no layer there
								inlayers = true;
								break;
							}
						}
					}
					if (!inlayers) {
						delete mainprogram->dragclip;
						mainprogram->dragclip = nullptr;
						enddrag();
					}
				}
			}
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
				//testlay->node->upeffboxes();
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
	//delete this->vtxcoords;  reminder: throws breakpoint
	//delete this->scrcoords;
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
	if (this->scrcoords->x1 <= mx and mx <= this->scrcoords->x1 + this->scrcoords->w) {
		if (this->scrcoords->y1 - this->scrcoords->h <= my and my <= this->scrcoords->y1) {
			if (mainprogram->showtooltips and mainprogram->tooltipbox == nullptr and this->tooltiptitle != "") mainprogram->tooltipbox = this;
			return true;
		}
	}
	return false;
}


void tooltips_handle(int win) {
	// draw tooltip
	float black[] = {0.0f, 0.0f, 0.0f, 1.0f};
	float orange[] = {1.0f, 0.5f, 0.0f, 1.0f};
	float white[] = {1.0f, 1.0f, 1.0f, 1.0f};
	float fac = 1.0f;
	if (mainprogram->prefon or mainprogram->tunemidi) fac = 4.0f;

	if (mainprogram->tooltipmilli > 3000) {
		if (mainprogram->longtooltips) {
			std::vector<std::string> texts;
			int pos = 0;
			while (pos < mainprogram->tooltipbox->tooltip.length() - 1) {
				int oldpos = pos;
				pos = mainprogram->tooltipbox->tooltip.rfind(" ", pos + 58);
				if (pos == -1) break;
				texts.push_back(mainprogram->tooltipbox->tooltip.substr(oldpos, pos - oldpos));
			}
			float x = mainprogram->tooltipbox->vtxcoords->x1 + mainprogram->tooltipbox->vtxcoords->w + tf(0.01f);  //first print offscreen
			float y = mainprogram->tooltipbox->vtxcoords->y1 - tf(0.01f) * glob->w / glob->h - tf(0.01f);
			float textw = 0.5f * sqrt(fac);
			float texth = 0.092754f * sqrt(fac);
			if ((x + textw) > 1.0f) x = x - textw - tf(0.02f) - mainprogram->tooltipbox->vtxcoords->w;
			if ((y - texth * (texts.size() + 1) - tf(0.01f)) < -1.0f) y = -1.0f + texth * (texts.size() + 1) - tf(0.01f);
			if (x < -1.0f) x = -1.0f;
			draw_box(nullptr, black, x, y - texth, textw, texth + tf(0.01f), -1);
			render_text(mainprogram->tooltipbox->tooltiptitle, orange, x + tf(0.015f) * sqrt(fac), y - texth + tf(0.03f) * sqrt(fac), tf(0.0003f) * fac, tf(0.0005f) * fac, win, 0);
			for (int i = 0; i < texts.size(); i++) {
				y -= texth;
				draw_box(nullptr, black, x, y - texth, textw, texth + tf(0.01f), -1);
				render_text(texts[i], white, x + tf(0.015f) * sqrt(fac), y - texth + tf(0.03f) * sqrt(fac), tf(0.0003f) * fac, tf(0.0005f) * fac, win, 0);
			}
		}
		else {
			float x = mainprogram->tooltipbox->vtxcoords->x1 + mainprogram->tooltipbox->vtxcoords->w + tf(0.01f);  //first print offscreen
			float y = mainprogram->tooltipbox->vtxcoords->y1 - tf(0.01f) * glob->w / glob->h - tf(0.01f);
			float textw = 0.25f * sqrt(fac);
			draw_box(nullptr, black, x, y - 0.092754f, textw, 0.092754f + tf(0.01f), -1);
			render_text(mainprogram->tooltipbox->tooltiptitle, orange, x + tf(0.015f) * sqrt(fac), y - 0.092754f + tf(0.03f) * sqrt(fac), tf(0.0003f) * fac, tf(0.0005f) * fac, win, 0);
		}
	}
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
	printf("num %d\n", num);

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
		if (this->items[i]->connected) delete this->items[i];
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

	pvi = new PrefItem(this, pos, "High-compressed video quality", PREF_NUMBER, (void*)&mainprogram->qualfr);
	pvi->value = 3;
	pvi->namebox->tooltiptitle = "Video decompression quality of high-compressed streams ";
	pvi->namebox->tooltip = "Sets the quality of the video stream decompression for streams that don't have one keyframe per frame. A tradeoff between quality and framerate. ";
	pvi->valuebox->tooltiptitle = "Video decompression quality of high-compressed streams ";
	pvi->valuebox->tooltip = "Leftclicking the value allows setting the quality in the range of 1 to 10.  Lower is better quality and worse framerate. ";
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
					si->tempnbframes.clear();
					for (int j = 0; j < si->nbframes.size(); j++) {
						si->tempnbframes.push_back(si->nbframes[j]);
					}
					std::vector<Layer*> &lvec = choose_layers(scene->deck);
					scene->nbframes.clear();
					for (int j = 0; j < lvec.size(); j++) {
						// store layers of current scene into nbframes for running their framecounters in the background (to keep sync)
						scene->nbframes.push_back(lvec[j]);
					}
					mainmix->mousedeck = scene->deck;
					// save current scene to temp dir, open new scene
					mainmix->do_save_deck(mainprogram->temppath + "tempdeck_xch.deck", false, false);
					SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
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
					mainmix->open_deck(mainprogram->temppath + "tempdeck_" + std::to_string(scene->deck) + std::to_string(i) + comp + ".deck", false);
					boost::filesystem::rename(mainprogram->temppath + "tempdeck_xch.deck", mainprogram->temppath + "tempdeck_" + std::to_string(scene->deck) + std::to_string(mainmix->currscene[scene->deck] + 1) + comp + ".deck");
					std::vector<Layer*> &lvec2 = choose_layers(scene->deck);
					for (int j = 0; j < lvec2.size(); j++) {
						// set layer frame to (running in background) nbframes->frame of loaded scene
						lvec2[j]->frame = si->tempnbframes[j]->frame;
						lvec2[j]->startframe = si->tempnbframes[j]->startframe;
						lvec2[j]->endframe = si->tempnbframes[j]->endframe;
						lvec2[j]->clips = si->tempnbframes[j]->clips;
						lvec2[j]->layerfilepath = si->tempnbframes[j]->layerfilepath;
						lvec2[j]->filename = si->tempnbframes[j]->filename;
						lvec2[j]->type = si->tempnbframes[j]->type;
						lvec2[j]->oldalive = si->tempnbframes[j]->oldalive;
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
bool exchange(Layer *lay, std::vector<Layer*> &slayers, std::vector<Layer*> &dlayers, bool deck) {
	bool ret = false;
	int size = dlayers.size();
	Layer* inlay = nullptr;
	for (int i = 0; i < size; i++) {
		inlay = dlayers[i];
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
				if (lay == inlay) return false;
				ret = true;
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
				slayers[lay->pos]->blendnode->wipex->value = lay->blendnode->wipex->value;
				slayers[lay->pos]->blendnode->wipey->value = lay->blendnode->wipey->value;
				if (lay->pos == 0) inlay->lasteffnode[1] = inlay->lasteffnode[0];
				else inlay->lasteffnode[1] = lay->lasteffnode[1];

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
				dlayers[i]->blendnode->wipex->value = bnode->wipex->value;
				dlayers[i]->blendnode->wipey->value = bnode->wipey->value;
				if (i == 0) lay->lasteffnode[1] = lay->lasteffnode[0];
				else lay->lasteffnode[1] = len1;
				
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
				//if (inlay->filename == "") {
				//	mainmix->delete_layer(slayers, inlay, false);
				//}
				if (slayers.size() == 0) {
					Layer* newlay = mainmix->add_layer(slayers, 0);
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
			else if (dropin or endx or (box->scrcoords->x1 - mainprogram->xvtxtoscr(0.075f) * (i - mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos != 0) < mainprogram->mx and mainprogram->mx < box->scrcoords->x1 + mainprogram->xvtxtoscr(0.075f))) {
				if (lay == dlayers[i]) return false;
				ret = true;
				//move one layer
				BLEND_TYPE nextbtype;
				float nextmfval = 0.0f;
				int nextwipetype = 0;
				int nextwipedir = 0;
				float nextwipex = 0.0f;
				float nextwipey = 0.0f;
				Layer* nextlay = nullptr;
				Layer* nextnextlay = nullptr;
				if (slayers.size() > lay->pos + 1) {
					nextlay = slayers[lay->pos + 1];
					nextbtype = nextlay->blendnode->blendtype;
					if (slayers.size() > nextlay->pos + 1) {
						nextnextlay = slayers[nextlay->pos + 1];
						//nextbtype = nextlay->blendnode->blendtype;
					}
					else nextnextlay = lay;
				}
				lay->deck = deck;
				BLEND_TYPE btype = lay->blendnode->blendtype;
				BlendNode *firstbnode = (BlendNode*)dlayers[0]->lasteffnode[0]->out[0];
				Node *firstlasteffnode = dlayers[0]->lasteffnode[0];
				float mfval = lay->blendnode->mixfac->value;
				int wipetype = lay->blendnode->wipetype;
				int wipedir = lay->blendnode->wipedir;
				float wipex = lay->blendnode->wipex->value;
				float wipey = lay->blendnode->wipey->value;
				if (lay->pos > 0) {
					lay->blendnode->in->out.clear();
					mainprogram->nodesmain->currpage->connect_nodes(lay->blendnode->in, lay->lasteffnode[1]->out[0]);
					mainprogram->nodesmain->currpage->delete_node(lay->blendnode);
				}
				else if (i + endx != 0) {
					if (nextlay) {
						if (nextlay->effects[1].size()) {
							nextlay->lasteffnode[0]->out.clear();
							mainprogram->nodesmain->currpage->connect_nodes(nextlay->lasteffnode[0], nextlay->effects[1][0]->node);
						}
						mainprogram->nodesmain->currpage->delete_node(nextlay->blendnode);
						nextlay->blendnode = new BlendNode;
						nextlay->blendnode->blendtype = nextbtype;
						nextlay->blendnode->mixfac->value = nextmfval;
						nextlay->blendnode->wipetype = nextwipetype;
						nextlay->blendnode->wipedir = nextwipedir;
						nextlay->blendnode->wipex->value = nextwipex;
						nextlay->blendnode->wipey->value = nextwipey;
						if (!nextlay->effects[1].size()) {
							nextlay->lasteffnode[1] = nextlay->lasteffnode[0];
						}
					}
				}
				lay->blendnode = nullptr;
				bool lastisvid = false;
				if (lay->lasteffnode[0] == lay->node) lastisvid = true;
				mainprogram->nodesmain->currpage->delete_node(lay->node);
				
				// do layer move in layer vectors
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
				//// end of layer move

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
					dlayers[i + endx]->blendnode->wipex->value = wipex;
					dlayers[i + endx]->blendnode->wipey->value = wipey;
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
				dlayers[i + endx]->blendnode->wipex->value = wipex;
				dlayers[i + endx]->blendnode->wipey->value = wipey;
				if (nextnextlay) {
					nextlay->lasteffnode[1]->out.clear();
					nextnextlay->blendnode->in = nullptr;
					mainprogram->nodesmain->currpage->connect_nodes(nextlay->lasteffnode[1], nextnextlay->blendnode);
				}
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
	}

	if (ret) {
		if (!lay->dummy) {
			ShelfElement* elem = lay->prevshelfdragelem;
			bool ret = mainmix->set_prevshelfdragelem(lay);
			if (!ret and elem->type == ELEM_DECK) {
				if (elem->launchtype == 2) {
					mainmix->mousedeck = elem->nbframes[0]->deck;
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
		}
		if (!inlay->dummy) {
			ShelfElement* elem = inlay->prevshelfdragelem;
			bool ret = mainmix->set_prevshelfdragelem(inlay);
			if (!ret and elem->type == ELEM_DECK) {
				if (elem->launchtype == 2) {
					mainmix->mousedeck = elem->nbframes[0]->deck;
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
		}
	}

	return ret;
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
				if (mainprogram->lmover) {
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
	mainprogram->frontbatch = true;
	if (menu->state > 1) {
		if (std::find(mainprogram->actmenulist.begin(), mainprogram->actmenulist.end(), menu) == mainprogram->actmenulist.end()) {
			mainprogram->actmenulist.clear();
		}
		int size = 0;
		for(int k = 0; k < menu->entries.size(); k++) {
			std::size_t sub = menu->entries[k].find("submenu");
			if (sub != 0) size++;
		}
		if (menu->menuy + mainprogram->yvtxtoscr(yshift) > glob->h - size * mainprogram->yvtxtoscr(tf(0.05f))) menu->menuy = glob->h - size * mainprogram->yvtxtoscr(tf(0.05f)) + mainprogram->yvtxtoscr(yshift);
		if (size > 21) menu->menuy = mainprogram->yvtxtoscr(mainprogram->layh) - mainprogram->yvtxtoscr(yshift);
		float vmx = (float)menu->menux * 2.0 / glob->w;
		float vmy = (float)menu->menuy * 2.0 / glob->h;
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
				if (mainprogram->xscrtovtx(menu->menux) > limit) xoff = -tf(0.195f) + xshift;
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
				if (menu->box->scrcoords->x1 + menu->menux + mainprogram->xvtxtoscr(xoff) < mainprogram->mx and mainprogram->mx < menu->box->scrcoords->x1 + menu->box->scrcoords->w + menu->menux + mainprogram->xvtxtoscr(xoff) and menu->box->scrcoords->y1 - menu->box->scrcoords->h + menu->menuy + (k - koff - numsubs) * mainprogram->yvtxtoscr(tf(0.05f)) - mainprogram->yvtxtoscr(yshift) < mainprogram->my and mainprogram->my < menu->box->scrcoords->y1 + menu->menuy + (k - koff - numsubs) * mainprogram->yvtxtoscr(tf(0.05f)) - mainprogram->yvtxtoscr(yshift)) {
					draw_box(lc, ac2, menu->box->vtxcoords->x1 + vmx + xoff, menu->box->vtxcoords->y1 - (k - koff - numsubs) * tf(0.05f) - vmy + yshift, tf(menu->width), tf(0.05f), -1);
					if (mainprogram->leftmousedown) mainprogram->leftmousedown = false;
					if (mainprogram->lmover) {
						for (int i = 0; i < mainprogram->menulist.size(); i++) {
							mainprogram->menulist[i]->state = 0;
						}
						mainprogram->menuchosen = true;
						menu->currsub = -1;
						mainprogram->frontbatch = false;
						return k - numsubs;
					}
				}
				else {
					draw_box(lc, ac1, menu->box->vtxcoords->x1 + vmx + xoff, menu->box->vtxcoords->y1 - (k - koff - numsubs) * tf(0.05f) - vmy + yshift, tf(menu->width), tf(0.05f), -1);
				}
				render_text(menu->entries[k], white, menu->box->vtxcoords->x1 + vmx + tf(0.0078f) + xoff, menu->box->vtxcoords->y1 - (k - koff - numsubs) * tf(0.05f) - vmy + yshift + tf(0.015f), tf(0.0003f), tf(0.0005f));
			}
			else {
				numsubs++;
				if (menu->currsub == k or (menu->box->scrcoords->x1 + menu->menux + mainprogram->xvtxtoscr(xoff) < mainprogram->mx and mainprogram->mx < menu->box->scrcoords->x1 + menu->box->scrcoords->w + menu->menux + mainprogram->xvtxtoscr(xoff) and menu->box->scrcoords->y1 - menu->box->scrcoords->h + menu->menuy + (k - koff - numsubs + 1) * mainprogram->yvtxtoscr(tf(0.05f)) - mainprogram->yvtxtoscr(yshift) < mainprogram->my and mainprogram->my < menu->box->scrcoords->y1 + menu->menuy + (k - koff - numsubs + 1) * mainprogram->yvtxtoscr(tf(0.05f)) - mainprogram->yvtxtoscr(yshift))) {
					if (menu->currsub == k or mainprogram->lmover) {
						if (menu->currsub != k) mainprogram->lmover = false;
						std::string name = menu->entries[k].substr(8, std::string::npos);
						for (int i = 0; i < mainprogram->menulist.size(); i++) {
							if (mainprogram->menulist[i]->name == name) {
								menu->currsub = k;
								mainprogram->menulist[i]->state = 2;
								mainprogram->actmenulist.push_back(menu);
								mainprogram->actmenulist.push_back(mainprogram->menulist[i]);
								float xs;
								if (mainprogram->xscrtovtx(menu->menux) > limit) {
									xs = xshift - tf(0.195f);
								}
								else {
									xs = xshift + tf(0.195f);
								}
								//float ys = (k - mainprogram->menulist[i]->entries.size() / 2.0f) * tf(-0.05f) + yshift;
								//float intm = std::clamp(ys + mainprogram->yscrtovtx(menu->menuy), 0.0f, 2.0f - mainprogram->menulist[i]->entries.size() * tf(0.05f));
								//ys = intm + mainprogram->menulist[i]->entries.size() * tf(0.05f);
								if (menu == mainprogram->filemenu and k - numsubs == 2) {
									// special rule: one layer choice less when saving layer
									mainprogram->laylistmenu1->entries.pop_back();
									mainprogram->laylistmenu2->entries.pop_back();
								}
								// start submenu
								int ret = handle_menu(mainprogram->menulist[i], xs, yshift);
								if (mainprogram->menuchosen) {
									menu->state = 0;
									mainprogram->menuresults.insert(mainprogram->menuresults.begin(), ret);
									menu->currsub = -1;
									mainprogram->frontbatch = false;
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
	mainprogram->frontbatch = false;
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

	mainprogram->bvao = mainprogram->prboxvao;
	mainprogram->bvbuf = mainprogram->prboxvbuf;
	mainprogram->btbuf = mainprogram->prboxtbuf;
	glClear(GL_COLOR_BUFFER_BIT);
	
	for (int i = 0; i < mainprogram->prefs->items.size(); i++) {
		PrefCat *item = mainprogram->prefs->items[i];
		if (item->box->in(mx, my)) {
			draw_box(white, lightblue, item->box, -1);
			if (mainprogram->leftmouse and mainprogram->prefs->curritem != i) {
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
		render_text(item->name, white, item->box->vtxcoords->x1 + 0.03f, item->box->vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, 1, 0);
	}
	draw_box(white, nullptr, -0.5f, -1.0f, 1.5f, 2.0f, -1);
	
	PrefCat *mci = mainprogram->prefs->items[mainprogram->prefs->curritem];
	if (mci->name == "MIDI Devices") ((PIMidi*)mci)->populate();
	for (int i = 0; i < mci->items.size(); i++) {
		if (mci->items[i]->type == PREF_ONOFF) {
			if (!mci->items[i]->connected) continue;
			draw_box(white, black, mci->items[i]->namebox, -1);
			render_text(mci->items[i]->name, white, mci->items[i]->namebox->vtxcoords->x1 + 0.23f, mci->items[i]->namebox->vtxcoords->y1 + 0.06f, 0.0024f, 0.004f, 1, 0);		
			if (mci->items[i]->valuebox->in(mx, my)) {
				draw_box(white, lightblue, mci->items[i]->valuebox, -1);
				if (mainprogram->leftmouse) {
					mci->items[i]->onoff = !mci->items[i]->onoff;
					if (mci->name == "MIDI Devices") {
						PIMidi *midici = (PIMidi*)mci;
						if (!midici->items[i]->onoff) {
							if (std::find(midici->onnames.begin(), midici->onnames.end(), midici->items[i]->name) != midici->onnames.end()) {
								midici->onnames.erase(std::find(midici->onnames.begin(), midici->onnames.end(), midici->items[i]->name));
								//midici->items[i]->midiin->cancelCallback();
								midici->items[i]->midiin->closePort();
								delete midici->items[i]->midiin;
							}
						}
						else {
							midici->onnames.push_back(midici->items[i]->name);
							RtMidiIn *midiin = new RtMidiIn();
							midiin->openPort(i);
							midiin->setCallback(&mycallback, (void*)mci->items[i]);
							mainprogram->openports.push_back(i);
							midiin->ignoreTypes( true, true, true );
							midici->items[i]->midiin = midiin;
							midici->items[i]->midiin = midiin;
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
			render_text(mci->items[i]->name, white, mci->items[i]->namebox->vtxcoords->x1 + 0.23f, mci->items[i]->namebox->vtxcoords->y1 + 0.06f, 0.0024f, 0.004f, 1, 0);		
			draw_box(white, black, mci->items[i]->valuebox, -1);
			if (mci->items[i]->renaming) {
				if (mainprogram->renaming == EDIT_NONE) {
					 mci->items[i]->renaming = false;
					 try {
					 	mci->items[i]->value = std::stoi(mainprogram->inputtext);
					 }
					 catch (...) {
						 mci->items[i]->value = ((PIVid*)(mci->items[i]))->oldvalue;
					 }
				}
				else if (mainprogram->renaming == EDIT_CANCEL) {
					 mci->items[i]->renaming = false;
				}
				else {
					do_text_input(mci->items[i]->valuebox->vtxcoords->x1 + 0.1f, mci->items[i]->valuebox->vtxcoords->y1 + 0.06f, 0.0024f, 0.004f, mx, my, mainprogram->xvtxtoscr(0.15f), 1, mci->items[i]);
				}
			}
			else {
				render_text(std::to_string(mci->items[i]->value), white, mci->items[i]->valuebox->vtxcoords->x1 + 0.1f, mci->items[i]->valuebox->vtxcoords->y1 + 0.06f, 0.0024f, 0.004f, 1, 0);
			}
			if (mci->items[i]->valuebox->in(mx, my)) {
				if (mainprogram->leftmouse and mainprogram->renaming == EDIT_NONE) {
					mci->items[i]->renaming = true;
					((PIVid*)(mci->items[i]))->oldvalue = mci->items[i]->value;
					mainprogram->renaming = EDIT_NUMBER;
					mainprogram->inputtext = std::to_string(mci->items[i]->value);
					mainprogram->cursorpos0 = mainprogram->inputtext.length();
					SDL_StartTextInput();
				}
			}
		}
		else if (mci->items[i]->type == PREF_PATH) {
			draw_box(white, black, mci->items[i]->namebox, -1);
			render_text(mci->items[i]->name, white, -0.5f + 0.1f, mci->items[i]->namebox->vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, 1, 0);
			draw_box(white, black, mci->items[i]->valuebox, -1);
			if (mci->items[i]->renaming == false) {
				render_text(mci->items[i]->path, white, mci->items[i]->valuebox->vtxcoords->x1 + 0.1f, mci->items[i]->valuebox->vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, 1, 0);
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
					do_text_input(mci->items[i]->valuebox->vtxcoords->x1 + 0.1f, mci->items[i]->valuebox->vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, mx, my, mainprogram->xvtxtoscr(0.7f), 1, mci->items[i]);
				}
			}
			if (mci->items[i]->valuebox->in(mx, my)) {
				if (mainprogram->leftmouse) {
					for (int i = 0; i < mci->items.size(); i++) {
						if (mci->items[i]->renaming) {
							mci->items[i]->renaming = false;
							end_input();
							break;
						}
					}
					mci->items[i]->renaming = true;
					mainprogram->renaming = EDIT_STRING;
					mainprogram->inputtext = mci->items[i]->path;
					mainprogram->cursorpos0 = mainprogram->inputtext.length();
					SDL_StartTextInput();
				}
			}
			draw_box(white, black, mci->items[i]->iconbox, -1);
			draw_box(white, black, mci->items[i]->iconbox->vtxcoords->x1 + 0.02f, mci->items[i]->iconbox->vtxcoords->y1 + 0.05f, 0.06f, 0.07f, -1);
			draw_box(white, black, mci->items[i]->iconbox->vtxcoords->x1 + 0.05f, mci->items[i]->iconbox->vtxcoords->y1 + 0.11f, 0.025f, 0.03f, -1);
			if (mci->items[i]->iconbox->in(mx, my)) {
				if (mainprogram->leftmouse) {
					mci->items[i]->choosing = true;
					mainprogram->pathto = "CHOOSEDIR";
					std::string title = "Open " + mci->items[i]->name + " directory";
					std::thread filereq (&Program::get_dir, mainprogram, title, boost::filesystem::canonical(mci->items[i]->path).generic_string());
					filereq.detach();
				}
			}
			if (mci->items[i]->choosing and mainprogram->choosedir != "") {
				#ifdef _WIN64
				boost::replace_all(mainprogram->choosedir, "/", "\\");
				#endif			
				mci->items[i]->path = mainprogram->choosedir;
				mainprogram->choosedir = "";
				mci->items[i]->choosing = false;
			}
		}
		
		if (mci->items[i]->connected) mci->items[i]->namebox->in(mx, my); //trigger tooltip

	}

	mainprogram->qualfr = std::clamp(mainprogram->qualfr, 1, 10);

	Box box;
	box.vtxcoords->x1 = 0.75f;
	box.vtxcoords->y1 = -1.0f;
	box.vtxcoords->w = 0.3f;
	box.vtxcoords->h = 0.2f;
	box.upvtxtoscr();
	draw_box(white, black, &box, -1);
	if (box.in(mx, my)) {
		draw_box(white, lightblue, &box, -1);
		if (mainprogram->leftmouse) {
			for (int i = 0; i < mci->items.size(); i++) {
				if (mci->items[i]->renaming) {
					mci->items[i]->renaming = false;
					end_input();
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
	render_text("CANCEL", white, box.vtxcoords->x1 + 0.02f, box.vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, 1, 0);
	box.vtxcoords->x1 = 0.45f;
	box.upvtxtoscr();
	draw_box(white, black, &box, -1);
	if (box.in(mx, my)) {
		draw_box(white, lightblue, &box, -1);
		if (mainprogram->leftmouse) {
			for (int i = 0; i < mci->items.size(); i++) {
				if (mci->items[i]->renaming) {
					if (mci->items[i]->type == PREF_PATH) {
						mci->items[i]->path = mainprogram->inputtext;
						end_input();
						break;
					}
					if (mci->items[i]->type == PREF_NUMBER) {
						 try {
							mci->items[i]->value = std::stoi(mainprogram->inputtext);
						 }
						 catch (...) {}
						 end_input();
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
	render_text("SAVE", white, box.vtxcoords->x1 + 0.02f, box.vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, 1, 0);
			
	tooltips_handle(1);
		
	mainprogram->bvao = mainprogram->boxvao;
	mainprogram->bvbuf = mainprogram->boxvbuf;
	mainprogram->btbuf = mainprogram->boxtbuf;
	mainprogram->middlemouse = 0;
	mainprogram->rightmouse = 0;
	mainprogram->menuactivation = false;
	mainprogram->mx = -1;
	mainprogram->my = -1;
	
	mainprogram->insmall = false;
	
	return 1;
}
	
void do_text_input(float x, float y, float sx, float sy, int mx, int my, float width, int smflag, PrefItem *item) {
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
			register_line_draw(white, x + textw - mainprogram->xscrtovtx(mainprogram->startpos), y - 0.010f, x + textw - mainprogram->xscrtovtx(mainprogram->startpos), y + (mainprogram->texth / ((smflag > 0) + 1)) / (2070.0f / glob->h));
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
	float black[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float grey[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	float lightblue[] = {0.5f, 0.5f, 1.0f, 1.0f};
	float green[] = { 0.0f, 0.7f, 0.0f, 1.0f };
	float red[] = { 0.9f, 0.0f, 0.0f, 1.0f };
	float yellow[] = { 1.0f, 1.0f, 0.0f, 1.0f };

	SDL_GL_MakeCurrent(mainprogram->tunemidiwindow, glc_tm);
	mainprogram->bvao = mainprogram->tmboxvao;
	mainprogram->bvbuf = mainprogram->tmboxvbuf;
	mainprogram->btbuf = mainprogram->tmboxtbuf;
	glClear(GL_COLOR_BUFFER_BIT);
	
	if (mainprogram->rightmouse) {
		mainprogram->tmlearn = TM_NONE;
		mainprogram->menuactivation = false;
	}
	
	std::string lmstr;
	if (mainprogram->tunemidiset == 1) lmstr = "A";
	else if (mainprogram->tunemidiset == 2) lmstr = "B";
	else if (mainprogram->tunemidiset == 3) lmstr = "C";
	else if (mainprogram->tunemidiset == 4) lmstr = "D";
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
		draw_box(yellow, grey, mainprogram->tmset, -1);
		if (mainprogram->tmset->in(mx, my)) {
			draw_box(red, lightblue, mainprogram->tmset, -1);
			if (mainprogram->leftmouse) {
				mainprogram->tunemidiset++;
				if (mainprogram->tunemidiset == 5) mainprogram->tunemidiset = 1;
			}
		}
		if (mainprogram->tunemidiset == 1) render_text("set A", yellow, 0.01f, 0.78f, 0.0024f, 0.004f, 2);
		else if (mainprogram->tunemidiset == 2) render_text("set B", yellow, 0.01f, 0.78f, 0.0024f, 0.004f, 2);
		else if (mainprogram->tunemidiset == 3) render_text("set C", yellow, 0.01f, 0.78f, 0.0024f, 0.004f, 2);
		else if (mainprogram->tunemidiset == 4) render_text("set D", yellow, 0.01f, 0.78f, 0.0024f, 0.004f, 2);
		LayMidi* lm = nullptr;
		if (mainprogram->tunemidiset == 1) lm = laymidiA;
		else if (mainprogram->tunemidiset == 2) lm = laymidiB;
		else if (mainprogram->tunemidiset == 3) lm = laymidiC;
		else if (mainprogram->tunemidiset == 4) lm = laymidiD;

		draw_box(white, black, mainprogram->tmplay, -1);
		if (lm->play[0] != -1) draw_box(white, green, mainprogram->tmplay, -1);
		if (mainprogram->tmplay->in(mx, my)) {
			draw_box(white, lightblue, mainprogram->tmplay, -1);
			if (mainprogram->leftmouse) {
				mainprogram->tmlearn = TM_PLAY;
			}
		}
		register_triangle_draw(white, white, 0.125f, -0.83f, 0.06f, 0.12f, RIGHT, CLOSED);
		
		draw_box(white, black, mainprogram->tmbackw, -1);
		if (lm->backw[0] != -1) draw_box(white, green, mainprogram->tmbackw, -1);
		if (mainprogram->tmbackw->in(mx, my)) {
			draw_box(white, lightblue, mainprogram->tmbackw, -1);
			if (mainprogram->leftmouse) {
				mainprogram->tmlearn = TM_BACKW;
			}
		}
		register_triangle_draw(white, white, -0.185f, -0.83f, 0.06f, 0.12f, LEFT, CLOSED);
		
		draw_box(white, black, mainprogram->tmbounce, -1);
		if (lm->bounce[0] != -1) draw_box(white, green, mainprogram->tmbounce, -1);
		if (mainprogram->tmbounce->in(mx, my)) {
			draw_box(white, lightblue, mainprogram->tmbounce, -1);
			if (mainprogram->leftmouse) {
				mainprogram->tmlearn = TM_BOUNCE;
			}
		}
		register_triangle_draw(white, white, -0.045f, -0.83f, 0.04f, 0.12f, LEFT, CLOSED);
		register_triangle_draw(white, white, 0.01f, -0.83f, 0.04f, 0.12f, RIGHT, CLOSED);
		
		draw_box(white, black, mainprogram->tmfrforw, -1);
		if (lm->frforw[0] != -1) draw_box(white, green, mainprogram->tmfrforw, -1);
		if (mainprogram->tmfrforw->in(mx, my)) {
			draw_box(white, lightblue, mainprogram->tmfrforw, -1);
			if (mainprogram->leftmouse) {
				mainprogram->tmlearn = TM_FRFORW;
			}
		}
		register_triangle_draw(white, white, 0.275f, -0.83f, 0.06f, 0.12f, RIGHT, OPEN);
		
		draw_box(white, black, mainprogram->tmfrbackw, -1);
		if (lm->frbackw[0] != -1) draw_box(white, green, mainprogram->tmfrbackw, -1);
		if (mainprogram->tmfrbackw->in(mx, my)) {
			draw_box(white, lightblue, mainprogram->tmfrbackw, -1);
			if (mainprogram->leftmouse) {
				mainprogram->tmlearn = TM_FRBACKW;
			}
		}
		register_triangle_draw(white, white, -0.335f, -0.83f, 0.06f, 0.12f, LEFT, OPEN);
		
		draw_box(white, black, mainprogram->tmspeed, -1);
		if (lm->speed[0] != -1) draw_box(white, green, mainprogram->tmspeed, -1);
		if (mainprogram->tmspeedzero->in(mx, my)) {
			draw_box(white, lightblue, mainprogram->tmspeedzero, -1);
			if (mainprogram->leftmouse) {
				mainprogram->tmlearn = TM_SPEEDZERO;
			}
		}
		else if (mainprogram->tmspeed->in(mx, my)) {
			draw_box(white, lightblue, mainprogram->tmspeed, -1);
			if (mainprogram->leftmouse) {
				mainprogram->tmlearn = TM_SPEED;
			}
			draw_box(white, black, mainprogram->tmspeedzero, -1);
			if (lm->speedzero[0] != -1) draw_box(white, green, mainprogram->tmspeedzero, -1);
		}
		else {
			draw_box(white, black, mainprogram->tmspeedzero, -1);
			if (lm->speedzero[0] != -1) draw_box(white, green, mainprogram->tmspeedzero, -1);
		}
		render_text("ONE", white, -0.755f, -0.08f, 0.0024f, 0.004f, 2);
		render_text("SPEED", white, -0.765f, -0.48f, 0.0024f, 0.004f, 2);
		
		draw_box(white, black, mainprogram->tmopacity, -1);
		if (lm->opacity[0] != -1) draw_box(white, green, mainprogram->tmopacity, -1);
		if (mainprogram->tmopacity->in(mx, my)) {
			draw_box(white, lightblue, mainprogram->tmopacity, -1);
			if (mainprogram->leftmouse) {
				mainprogram->tmlearn = TM_OPACITY;
			}
		}
		render_text("OPACITY", white, 0.605f, -0.48f, 0.0024f, 0.004f, 2);
		
		if (mainprogram->tmfreeze->in(mx, my)) {
			draw_box(white, lightblue, mainprogram->tmfreeze, -1);
			if (mainprogram->leftmouse) {
				mainprogram->tmlearn = TM_FREEZE;
			}
		}
		else {
			if (lm->scratch[0] != -1) draw_box(green, 0.0f, 0.1f, 0.6f, 1, smw, smh);
			if (sqrt(pow((mx / (glob->w / 2.0f) - 1.0f) * glob->w / glob->h, 2) + pow((glob->h - my) / (glob->h / 2.0f) - 1.1f, 2)) < 0.6f) {
				draw_box(lightblue, 0.0f, 0.1f, 0.6f, 1, smw, smh);
				mainprogram->tmscratch->in(mx, my);  //tooltip
				if (mainprogram->leftmouse) {
					mainprogram->tmlearn = TM_SCRATCH;
				}
			}
			draw_box(white, black, mainprogram->tmfreeze, -1);
			if (lm->scratchtouch[0] != -1) draw_box(white, green, mainprogram->tmfreeze, -1);
		}
		draw_box(white, 0.0f, 0.1f, 0.6f, 2, smw, smh);
		render_text("SCRATCH", white, -0.1f, -0.3f, 0.0024f, 0.004f, 2);
		render_text("FREEZE", white, -0.08f, 0.12f, 0.0024f, 0.004f, 2);

		Box box;
		box.vtxcoords->x1 = -1.0f;
		box.vtxcoords->y1 = -1.0f;
		box.vtxcoords->w = 0.3f;
		box.vtxcoords->h = 0.2f;
		box.upvtxtoscr();
		draw_box(white, black, &box, -1);
		if (box.in(mx, my)) {
			draw_box(white, lightblue, &box, -1);
			if (mainprogram->leftmouse) {
				lm->play[0] = -1;
				lm->backw[0] = -1;
				lm->bounce[0] = -1;
				lm->frforw[0] = -1;
				lm->frbackw[0] = -1;
				lm->scratch[0] = -1;
				lm->scratchtouch[0] = -1;
				lm->speed[0] = -1;
				lm->speedzero[0] = -1;
				lm->opacity[0] = -1;
				lm->play[1] = -1;
				lm->backw[1] = -1;
				lm->bounce[1] = -1;
				lm->frforw[1] = -1;
				lm->frbackw[1] = -1;
				lm->scratch[1] = -1;
				lm->scratchtouch[1] = -1;
				lm->speed[1] = -1;
				lm->speedzero[1] = -1;
				lm->opacity[1] = -1;
				return 0;
			}
		}
		render_text("NEW", white, box.vtxcoords->x1 + 0.02f, box.vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, 2);
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
		if (mainprogram->leftmouse) {
			open_genmidis(mainprogram->docpath + "midiset.gm");
			mainprogram->tmlearn = TM_NONE;
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
		if (mainprogram->leftmouse) {
			save_genmidis(mainprogram->docpath + "midiset.gm");
			mainprogram->tmlearn = TM_NONE;
			mainprogram->tunemidi = false;
			SDL_HideWindow(mainprogram->tunemidiwindow);
			return 0;
		}
	}
	render_text("SAVE", white, box.vtxcoords->x1 + 0.02f, box.vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, 2);
	
	tooltips_handle(2);
		
	mainprogram->bvao = mainprogram->boxvao;
	mainprogram->bvbuf = mainprogram->boxvbuf;
	mainprogram->btbuf = mainprogram->boxtbuf;
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
	//glXMakeCurrent(dpy, sdl_win, c2);
	
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
		draw_box(nullptr, black, -1.0f, -1.0f, 2.0f, 2.0f, mainprogram->bgtex);
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
		// when in prefs or tunemidi windows
		mainprogram->mx = -1;
		mainprogram->my = -1;
		//mainprogram->leftmouse = false;
		mainprogram->menuactivation = false;
	}
	if (mainmix->adaptparam) {
		// no hovering while adapting param
		mainprogram->my = -1;
	}
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
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < mainmix->scenes[m][i]->nbframes.size(); j++) {
				if (i == mainmix->currscene[m]) break;
				mainmix->scenes[m][i]->nbframes[j]->calc_texture(0, 0);
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
			draw_box(nullptr, blue, -1.0f + mainprogram->numw + mainmix->currlay->deck * 1.0f + (mainmix->currlay->pos - mainmix->scenes[mainmix->currlay->deck][mainmix->currscene[mainmix->currlay->deck]]->scrollpos) * mainprogram->layw, 0.1f + tf(0.05f), mainprogram->layw, 0.9f, -1);
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
				display_layers(testlay, 0);
			}
			for (int i = 0; i < mainmix->layersB.size(); i++) {
				Layer* testlay = mainmix->layersB[i];
				display_layers(testlay, 1);
			}
		}
		else {
			// when performing
			for (int i = 0; i < mainmix->layersAcomp.size(); i++) {
				Layer* testlay = mainmix->layersAcomp[i];
				display_layers(testlay, 0);
			}
			for (int i = 0; i < mainmix->layersBcomp.size(); i++) {
				Layer* testlay = mainmix->layersBcomp[i];
				display_layers(testlay, 1);
			}
		}


		// display the deck monitors and output monitors on the bottom of the screen
		display_mix();


		// handle scenes
		handle_scenes(mainmix->scenes[0][mainmix->currscene[0]]);
		handle_scenes(mainmix->scenes[1][mainmix->currscene[1]]);


		// draw and handle overall genmidi button
		mainmix->handle_genmidi();


		// Draw and handle buttons
		if (mainprogram->prevmodus) {
			mainprogram->toscreen->handle();
			if (mainprogram->toscreen->toggled()) {
				mainprogram->toscreen->value = 0;
				mainprogram->toscreen->oldvalue = 0;
				// SEND UP button copies preview set entirely to comp set
				mainmix->copy_to_comp(mainmix->layersA, mainmix->layersAcomp, mainmix->layersB, mainmix->layersBcomp, mainprogram->nodesmain->currpage->nodes, mainprogram->nodesmain->currpage->nodescomp, mainprogram->nodesmain->mixnodescomp, true);
			}
			Box* box = mainprogram->toscreen->box;
			register_triangle_draw(white, white, box->vtxcoords->x1 + box->vtxcoords->w / 2.0f + tf(0.0078f), box->vtxcoords->y1 + tf(0.015f), tf(0.011f), tf(0.0208f), DOWN, CLOSED);
			render_text(mainprogram->toscreen->name[0], white, box->vtxcoords->x1 + tf(0.0078f), box->vtxcoords->y1 + tf(0.015f), 0.0006f, 0.001f);
		}
		if (mainprogram->prevmodus) {
			mainprogram->backtopre->handle();
			if (mainprogram->backtopre->toggled()) {
				mainprogram->backtopre->value = 0;
				mainprogram->backtopre->oldvalue = 0;
				// SEND DOWN button copies comp set entirely back to preview set
				mainmix->copy_to_comp(mainmix->layersAcomp, mainmix->layersA, mainmix->layersBcomp, mainmix->layersB, mainprogram->nodesmain->currpage->nodescomp, mainprogram->nodesmain->currpage->nodes, mainprogram->nodesmain->mixnodes, false);
			}
			Box* box = mainprogram->backtopre->box;
			register_triangle_draw(white, white, box->vtxcoords->x1 + box->vtxcoords->w / 2.0f + tf(0.0078f), box->vtxcoords->y1 + tf(0.015f), tf(0.011f), tf(0.0208f), UP, CLOSED);
			render_text(mainprogram->backtopre->name[0], white, mainprogram->backtopre->box->vtxcoords->x1 + tf(0.0078f), mainprogram->backtopre->box->vtxcoords->y1 + tf(0.015f), 0.0006, 0.001);
		}

		mainprogram->modusbut->handle();
		if (mainprogram->modusbut->toggled()) {
			mainprogram->prevmodus = !mainprogram->prevmodus;
			//modusbut is button that toggles effect preview mode to performance mode and back
			mainprogram->preview_init();
		}
		render_text(mainprogram->modusbut->name[mainprogram->prevmodus], white, mainprogram->modusbut->box->vtxcoords->x1 + tf(0.0078f), mainprogram->modusbut->box->vtxcoords->y1 + tf(0.015f), 0.00042, 0.00070);


		//draw and handle global deck speed sliders
		par = mainmix->deckspeed[!mainprogram->prevmodus][0];
		draw_box(white, darkgrey, mainmix->deckspeed[!mainprogram->prevmodus][0]->box->vtxcoords->x1, mainmix->deckspeed[!mainprogram->prevmodus][0]->box->vtxcoords->y1, mainmix->deckspeed[!mainprogram->prevmodus][0]->box->vtxcoords->w * 0.30f, 0.1f, -1);
		par->handle();
		par = mainmix->deckspeed[!mainprogram->prevmodus][1];
		draw_box(white, darkgrey, mainmix->deckspeed[!mainprogram->prevmodus][1]->box->vtxcoords->x1, mainmix->deckspeed[!mainprogram->prevmodus][1]->box->vtxcoords->y1, mainmix->deckspeed[!mainprogram->prevmodus][1]->box->vtxcoords->w * 0.30f, 0.1f, -1);
		par->handle();

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


		//Handle layer scrollbar
		bool inbox = false;
		for (int i = 0; i < 2; i++) {
			std::vector<Layer*>& lvec = choose_layers(i);
			Box box;
			Box boxbefore;
			Box boxafter;
			Box boxlayer;
			int size = lvec.size() + 1;
			if (size < 3) size = 3;
			float slidex = 0.0f;
			if (mainmix->scrollon == i + 1) {
				slidex = (mainmix->scrollon != 0) * mainprogram->xscrtovtx(mainprogram->mx - mainmix->scrollmx);
			}
			box.vtxcoords->x1 = -1.0f + mainprogram->numw + i + mainmix->scenes[i][mainmix->currscene[i]]->scrollpos * (mainprogram->layw * 3.0f / size) + slidex;
			if (box.vtxcoords->x1 < -1.0f + mainprogram->numw + i) {
				box.vtxcoords->x1 = -1.0f + mainprogram->numw + i;
				slidex = 0.0f;
			}
			if (box.vtxcoords->x1 > -1.0f + mainprogram->numw + i + (lvec.size() - 2) * (mainprogram->layw * 3.0f / size)) {
				box.vtxcoords->x1 = -1.0f + mainprogram->numw + i + (lvec.size() - 2) * (mainprogram->layw * 3.0f / size);
				slidex = 0.0f;
			}
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
			bool inbox = false;
			if (box.in()) {
				inbox = true;
				if (mainprogram->leftmousedown and mainmix->scrollon == 0 and !mainprogram->menuondisplay) {
					mainmix->scrollon = i + 1;
					mainmix->scrollmx = mainprogram->mx;
					mainprogram->leftmousedown = false;
				}
			}
			else {
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
				if (mainprogram->lmover) {
					mainmix->scrollon = 0;
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
			box.vtxcoords->x1 = -1.0f + mainprogram->numw + i + slidex;
			float remw = box.vtxcoords->w;
			for (int j = 0; j < size; j++) {
				//draw scrollbar numbers+divisions
				if (j == 0) {
					if (slidex < 0.0f) box.vtxcoords->x1 -= slidex;
				}
				if (j == size - 1) {
					if (slidex > 0.0f) box.vtxcoords->w -= slidex;
				}
				if (j >= mainmix->scenes[i][mainmix->currscene[i]]->scrollpos and j < mainmix->scenes[i][mainmix->currscene[i]]->scrollpos + 3) {
					if (inbox) draw_box(white, lightblue, &box, -1);
					else draw_box(white, grey, &box, -1);
				}
				else draw_box(white, black, &box, -1);
				if (j == 0) {
					if (slidex < 0.0f) box.vtxcoords->x1 += slidex;
				}
				render_text(std::to_string(j + 1), white, box.vtxcoords->x1 + 0.0078f - slidex, box.vtxcoords->y1 + 0.0078f, 0.0006, 0.001);
				const int s = lvec.size() - mainmix->scenes[i][mainmix->currscene[i]]->scrollpos;
				bool greycond = (j >= mainmix->scenes[i][mainmix->currscene[i]]->scrollpos and j < mainmix->scenes[i][mainmix->currscene[i]]->scrollpos + std::min(s, 3));
				if (!greycond) {
					box.vtxcoords->x1 += box.vtxcoords->w;
					box.upvtxtoscr();
					continue;
				}

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

		//handle shelves
		mainprogram->inshelf = -1;
		mainprogram->shelves[0]->handle();
		mainprogram->shelves[1]->handle();

		Box *box;
		for (int i = 0; i < 2; i++) {
			std::vector<Layer*>& lays = choose_layers(i);
			for (int j = 0; j < lays.size(); j++) {
				Layer* lay2 = lays[j];
				box = lay2->node->vidbox;
				if (lay2->queueing and lay2->filename != "") {
					// drawing clip queue
					int sc = mainmix->scenes[lay2->deck][mainmix->currscene[lay2->deck]]->scrollpos;
					int max = 4;
					if (mainprogram->dragclip) {
						if (mainprogram->inshelf == 0 and (lay2->pos == sc or lay2->pos == sc + 1) and lay2->deck == 0) {
							max = 3;
						}
						if (mainprogram->inshelf == 1 and (lay2->pos == sc + 1 or lay2->pos == sc + 2) and lay2->deck == 1) {
							max = 3;
						}
					}
					int until = lay2->clips.size() - lay2->queuescroll;
					if (until > max) until = max;
					for (int k = 0; k < until; k++) {
						draw_box(white, black, box->vtxcoords->x1, box->vtxcoords->y1 - (k + 1) * mainprogram->layh, box->vtxcoords->w, box->vtxcoords->h, lay2->clips[k + lay2->queuescroll]->tex);
						render_text("Queued clip #" + std::to_string(k + lay2->queuescroll + 1), white, box->vtxcoords->x1 + tf(0.01f), box->vtxcoords->y1 - k * box->vtxcoords->h - tf(0.03f), 0.0005f, 0.0008f);
					}
					for (int k = 0; k < max; k++) {
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
									// starting dragging a clip
									if (k + lay2->queuescroll != lay2->clips.size() - 1) {
										Clip* clip = lay2->clips[k + lay2->queuescroll];
										mainprogram->dragbinel = new BinElement;
										mainprogram->dragbinel->type = clip->type;
										mainprogram->dragbinel->path = clip->path;
										mainprogram->dragbinel->tex = clip->tex;
										binsmain->dragtex = clip->tex;
										lay2->vidmoving = true;
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
		//clip menu?
		if (mainprogram->prevmodus) {
			clip_intest(mainmix->layersA, 0);
			clip_intest(mainmix->layersB, 1);
		}
		else {
			clip_intest(mainmix->layersAcomp, 0);
			clip_intest(mainmix->layersBcomp, 1);
		}
		//handle clip dragging
		clip_dragging();


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


		int k = -1;
		// Do mainprogram->mixenginemenu
		k = handle_menu(mainprogram->mixenginemenu);
		if (k == 0) {
			if (mainmix->mousenode and mainprogram->menuresults.size()) {
				if (mainmix->mousenode->type == BLEND) {
					((BlendNode*)mainmix->mousenode)->blendtype = (BLEND_TYPE)(mainprogram->menuresults[0] + 1);
				}
				mainmix->mousenode = nullptr;
			}
		}
		else if (k == 1) {
			if (mainmix->mousenode) {
				if (mainmix->mousenode->type == BLEND) {
					if (mainprogram->menuresults.size()) {
						if (mainprogram->menuresults[0] == 0) {
							((BlendNode*)mainmix->mousenode)->blendtype = MIXING;
						}
						else {
							if (mainprogram->menuresults.size() == 2) {
								((BlendNode*)mainmix->mousenode)->blendtype = WIPE;
								((BlendNode*)mainmix->mousenode)->wipetype = mainprogram->menuresults[0] - 1;
								((BlendNode*)mainmix->mousenode)->wipedir = mainprogram->menuresults[1];
							}
						}
					}
				}
				mainmix->mousenode = nullptr;
			}
		}
		if (mainprogram->menuchosen) {
			mainprogram->menuchosen = false;
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
				std::vector<Effect*>& evec = mainmix->mouselayer->choose_effects();
				int mon = evec[mainmix->mouseeffect]->node->monitor;
				mainmix->mouselayer->replace_effect((EFFECT_TYPE)mainprogram->abeffects[k - 1], mainmix->mouseeffect);
				evec[mainmix->mouseeffect]->node->monitor = mon;
			}
			mainmix->mouselayer = nullptr;
			mainmix->mouseeffect = -1;
		}
		if (mainprogram->menuchosen) {
			mainprogram->menuchosen = false;
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
				LoopStationElement* elem = loopstation->elemmap[mainmix->learnparam];
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
				float fac = mainmix->deckspeed[!mainprogram->prevmodus][mainmix->mouselayer->deck]->value;
				if (mainmix->mouselayer->clonesetnr != -1) {
					std::unordered_set<Layer*>::iterator it;
					for (it = mainmix->clonesets[mainmix->mouselayer->clonesetnr]->begin(); it != mainmix->clonesets[mainmix->mouselayer->clonesetnr]->end(); it++) {
						Layer* lay = *it;
						if (lay->deck == !mainmix->mouselayer->deck) {
							fac *= mainmix->deckspeed[!mainprogram->prevmodus][!mainmix->mouselayer->deck]->value;
							break;
						}
					}
				}
				mainmix->cbduration = ((float)(mainmix->mouselayer->endframe - mainmix->mouselayer->startframe) * mainmix->mouselayer->millif) / (mainmix->mouselayer->speed->value * mainmix->mouselayer->speed->value * fac * fac);
				printf("fac %f\n", fac);
			}
			else if (k == 3) {
				float fac = mainmix->deckspeed[!mainprogram->prevmodus][mainmix->mouselayer->deck]->value;
				if (mainmix->mouselayer->clonesetnr != -1) {
					std::unordered_set<Layer*>::iterator it;
					for (it = mainmix->clonesets[mainmix->mouselayer->clonesetnr]->begin(); it != mainmix->clonesets[mainmix->mouselayer->clonesetnr]->end(); it++) {
						Layer* lay = *it;
						if (lay->deck == !mainmix->mouselayer->deck) {
							fac *= mainmix->deckspeed[!mainprogram->prevmodus][!mainmix->mouselayer->deck]->value;
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
				float dsp = mainmix->deckspeed[!mainprogram->prevmodus][mainmix->mouselayer->deck]->value;
				if (mainmix->mouselayer->clonesetnr != -1) {
					std::unordered_set<Layer*>::iterator it;
					for (it = mainmix->clonesets[mainmix->mouselayer->clonesetnr]->begin(); it != mainmix->clonesets[mainmix->mouselayer->clonesetnr]->end(); it++) {
						Layer* lay = *it;
						if (lay->deck == !mainmix->mouselayer->deck) {
							dsp *= mainmix->deckspeed[!mainprogram->prevmodus][!mainmix->mouselayer->deck]->value;
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
				if (mainprogram->menuresults.size() == 2) {
					if (mainprogram->menuresults[0] != 0) {
						mainmix->wipe[!mainprogram->prevmodus] = mainprogram->menuresults[0] - 1;
						mainmix->wipedir[!mainprogram->prevmodus] = mainprogram->menuresults[1];
					}
				}
				if (mainprogram->menuresults.size() == 1) {
					if (mainprogram->menuresults[0] == 0) {
						mainmix->wipe[!mainprogram->prevmodus] = -1;
					}
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
					EWindow* mwin = new EWindow;
					mwin->mixid = mainprogram->mixtargetmenu->value;
					OutputEntry* entry = new OutputEntry;
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
					mainprogram->mixwindows.push_back(mwin);
					SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
					#ifdef _WIN64
					HGLRC c1 = wglGetCurrentContext();
					mwin->glc = SDL_GL_CreateContext(mwin->win);
					SDL_GL_MakeCurrent(mwin->win, mwin->glc);
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
					glUseProgram(mainprogram->ShaderProgram);

					std::thread vidoutput(output_video, mwin);
					vidoutput.detach();

					SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
				}
			}
		}
		if (mainprogram->menuchosen) {
			mainprogram->menuchosen = false;
			mainprogram->menuactivation = 0;
			mainprogram->menuresults.clear();
		}

		// Draw and handle wipemenu
		k = handle_menu(mainprogram->wipemenu);
		if (k > 0) {
			if (mainprogram->menuresults.size()) {
				mainmix->wipe[!mainprogram->prevmodus] = k - 1;
				mainmix->wipedir[!mainprogram->prevmodus] = mainprogram->menuresults[0];
			}
		}
		else if (k == 0) {
			mainmix->wipe[!mainprogram->prevmodus] = -1;
		}
		
		if (mainprogram->menuchosen) {
			mainprogram->menuchosen = false;
			mainprogram->menuactivation = 0;
			mainprogram->menuresults.clear();
		}
				
		// Draw and handle deckmenu
		k = handle_menu(mainprogram->deckmenu);
		if (k == 0) {
			mainprogram->pathto = "OPENDECK";
			std::thread filereq (&Program::get_inname, mainprogram, "Open deck file", "application/ewocvj2-deck", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
			filereq.detach();
		}
		else if (k == 1) {
			mainprogram->pathto = "SAVEDECK";
			std::thread filereq (&Program::get_outname, mainprogram, "Save deck file", "application/ewocvj2-deck", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
			filereq.detach();
		}
		else if (k == 2) {
			mainmix->learn = true;
		}
		if (mainprogram->menuchosen) {
			mainprogram->menuchosen = false;
			mainprogram->menuactivation = 0;
			mainprogram->menuresults.clear();
		}
		
		
		// Draw and handle mainprogram->laymenu1
		if (mainprogram->laymenu1->state > 1 or mainprogram->laymenu2->state > 1 or mainprogram->loadmenu->state > 1 or mainprogram->clipmenu->state > 1) {
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
			if (k > 10) k++;
		}
		if (k > -1) {
			if (k == 0) {
				if (mainprogram->menuresults.size()) {
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
			}
			if (k == 1) {
				mainprogram->pathto = "OPENFILESLAYERS";
				mainprogram->loadlay = mainmix->mouselayer;
				std::thread filereq(&Program::get_multinname, mainprogram, "Open video/image/layer file", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
				filereq.detach();
			}
			if (k == 2) {
				mainprogram->pathto = "OPENFILESQUEUE";
				mainprogram->loadlay = mainmix->mouselayer;
				std::thread filereq(&Program::get_multinname, mainprogram, "Open video/image/layer file", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
				filereq.detach();
			}
			else if (k == 3) {
				mainprogram->pathto = "SAVELAYFILE";
				std::thread filereq (&Program::get_outname, mainprogram, "Save layer file", "application/ewocvj2-layer", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
				filereq.detach();
			}
			else if (k == 4) {
				mainmix->new_file(mainmix->mousedeck, 1);
			}
			else if (k == 5) {
				mainprogram->pathto = "OPENDECK";
				std::thread filereq (&Program::get_inname, mainprogram, "Open deck file", "application/ewocvj2-deck", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
				filereq.detach();
			}
			else if (k == 6) {
				mainprogram->pathto = "SAVEDECK";
				std::thread filereq (&Program::get_outname, mainprogram, "Save deck file", "application/ewocvj2-deck", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
				filereq.detach();
			}
			else if (k == 7) {
				mainmix->new_file(2, 1);
			}
			else if (k == 8) {
				mainprogram->pathto = "OPENMIX";
				std::thread filereq (&Program::get_inname, mainprogram, "Open mix file", "application/ewocvj2-mix", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
				filereq.detach();
			}
			else if (k == 9) {
				mainprogram->pathto = "SAVEMIX";
				std::thread filereq (&Program::get_outname, mainprogram, "Open mix file", "application/ewocvj2-mix", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
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
				if (mainmix->mouselayer->clonesetnr == -1) {
					std::unordered_set<Layer*> *uset = new std::unordered_set<Layer*>;
					mainmix->clonesets.push_back(uset);
					uset->emplace(mainmix->mouselayer);
					mainmix->mouselayer->clonesetnr = mainmix->clonesets.size() - 1;
				}
				clonelay->clonesetnr = mainmix->mouselayer->clonesetnr;
				mainmix->clonesets[mainmix->mouselayer->clonesetnr]->emplace(clonelay);
			}
			else if (k == 12) {
				mainmix->mouselayer->shiftx->value = 0.0f;
				mainmix->mouselayer->shifty->value = 0.0f;
			}
			else if (k == 13) {
				mainmix->mouselayer->aspectratio = (RATIO_TYPE)mainprogram->menuresults[0];
				if (mainmix->mouselayer->type == ELEM_IMAGE) {
					ilBindImage(mainmix->mouselayer->boundimage);
					ilActiveImage((int)mainmix->mouselayer->frame);
					mainmix->mouselayer->set_aspectratio(ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT));
					mainmix->mouselayer->decresult->newdata = true;
				}
				else {
					mainmix->mouselayer->set_aspectratio(mainmix->mouselayer->video_dec_ctx->width, mainmix->mouselayer->video_dec_ctx->height);
				}
			}
		}
	
		if (mainprogram->menuchosen) {
			mainprogram->menuchosen = false;
			mainprogram->menuactivation = 0;
			mainprogram->menuresults.clear();
		}
		
			
		k = handle_menu(mainprogram->loadmenu);
		if (k > -1) {
			std::vector<Layer*> &lvec = choose_layers(mainmix->mousedeck);
			if (k == 0) {
				if (mainprogram->menuresults.size()) {
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
			}
			if (k == 1) {
				mainprogram->pathto = "OPENFILESLAYERS";
				std::thread filereq(&Program::get_multinname, mainprogram, "Open video/image/layer file", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
				filereq.detach();
				mainmix->addlay = true;
			}
			else if (k == 2) {
				mainprogram->pathto = "OPENFILESQUEUE";
				std::thread filereq(&Program::get_multinname, mainprogram, "Open video/image/layer file", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
				filereq.detach();
				mainmix->addlay = true;
			}
			else if (k == 3) {
				mainmix->new_file(mainmix->mousedeck, 1);
			}
			else if (k == 4) {
				mainprogram->pathto = "OPENDECK";
				std::thread filereq (&Program::get_inname, mainprogram, "Open deck file", "application/ewocvj2-deck", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
				filereq.detach();
			}
			else if (k == 5) {
				mainprogram->pathto = "SAVEDECK";
				std::thread filereq (&Program::get_outname, mainprogram, "Save deck file", "application/ewocvj2-deck", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
				filereq.detach();
			}
			else if (k == 6) {
				mainmix->new_file(2, 1);
			}
			else if (k == 7) {
				mainprogram->pathto = "OPENMIX";
				std::thread filereq (&Program::get_inname, mainprogram, "Open mix file", "application/ewocvj2-mix", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
				filereq.detach();
			}
			else if (k == 8) {
				mainprogram->pathto = "SAVEMIX";
				std::thread filereq (&Program::get_outname, mainprogram, "Save mix file", "application/ewocvj2-mix", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
				filereq.detach();
			}
		}
	
		if (mainprogram->menuchosen) {
			mainprogram->menuchosen = false;
			mainprogram->menuactivation = 0;
			mainprogram->menuresults.clear();
		}
		
		
		// Draw and handle clipmenu
		k = handle_menu(mainprogram->clipmenu);
		if (k > -1) {
			std::vector<Layer*>& lvec = choose_layers(mainmix->mousedeck);
			if (k == 0) {
				if (mainprogram->menuresults.size()) {
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
			}
			if (k == 1) {
				mainprogram->pathto = "CLIPOPENFILES";
				std::thread filereq(&Program::get_multinname, mainprogram, "Open clip video file", boost::filesystem::canonical(mainprogram->currclipfilesdir).generic_string());
				filereq.detach();
			}
			if (k == 2) {
				mainmix->mouselayer->clips.erase(std::find(mainmix->mouselayer->clips.begin(), mainmix->mouselayer->clips.end(), mainmix->mouseclip));
				delete mainmix->mouseclip;
			}
			if (mainprogram->menuchosen) {
				mainprogram->menuchosen = false;
				mainprogram->menuactivation = 0;
				mainprogram->menuresults.clear();
			}
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
			std::thread filereq (&Program::get_outname, mainprogram, "New project", "application/ewocvj2-project", boost::filesystem::absolute(reqdir + name).generic_string());
			filereq.detach();
		}
		else if (k == 1) {
			mainprogram->pathto = "OPENPROJECT";
			std::thread filereq (&Program::get_inname, mainprogram, "Open project file", "application/ewocvj2-project", boost::filesystem::canonical(mainprogram->currprojdir).generic_string());
			filereq.detach();
		}
		else if (k == 2) {
			mainprogram->project->save(mainprogram->project->path);
		}
		else if (k == 3) {
			mainprogram->pathto = "SAVEPROJECT";
			std::thread filereq(&Program::get_outname, mainprogram, "Save project file", "application/ewocvj2-project", boost::filesystem::canonical(mainprogram->project->path).generic_string());
			filereq.detach();
		}
		else if (k == 4) {
			mainmix->new_state();
		}
		else if (k == 5) {
			mainprogram->pathto = "OPENSTATE";
			std::thread filereq (&Program::get_inname, mainprogram, "Open state file", "application/ewocvj2-state", boost::filesystem::canonical(mainprogram->currstatedir).generic_string());
			filereq.detach();
		}
		else if (k == 6) {
			mainprogram->pathto = "SAVESTATE";
			std::thread filereq (&Program::get_outname, mainprogram, "Save state file", "application/ewocvj2-state", boost::filesystem::canonical(mainprogram->currstatedir).generic_string());
			filereq.detach();
		}
		else if (k == 7) {
			if (!mainprogram->prefon) {
				mainprogram->prefon = true;
				SDL_ShowWindow(mainprogram->prefwindow);
				//SDL_RaiseWindow(mainprogram->prefwindow);
				//SDL_GL_MakeCurrent(mainprogram->prefwindow, glc_pr);
				//glUseProgram(mainprogram->ShaderProgram_pr);
				for (int i = 0; i < mainprogram->prefs->items.size(); i++) {
					PrefCat *item = mainprogram->prefs->items[i];
					item->box->upvtxtoscr();
				}
			}
			else {
				SDL_RaiseWindow(mainprogram->prefwindow);
			}
		}
		else if (k == 8) {
			if (!mainprogram->tunemidi) {
				mainprogram->tunemidi = true;
				SDL_ShowWindow(mainprogram->tunemidiwindow);
				//SDL_RaiseWindow(mainprogram->tunemidiwindow);
				//SDL_GL_MakeCurrent(mainprogram->tunemidiwindow, glc_tm);
				//glUseProgram(mainprogram->ShaderProgram_tm);
				mainprogram->tmset->upvtxtoscr();
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
			}
			else {
				SDL_RaiseWindow(mainprogram->tunemidiwindow);
			}
		}
		else if (k == 9) {
			mainprogram->quitting = "quitted";
		}

		if (mainprogram->menuchosen) {
			mainprogram->menuchosen = false;
			mainprogram->menuactivation = 0;
			mainprogram->menuresults.clear();
		}
				
		// Draw and handle shelfmenu
		k = handle_menu(mainprogram->shelfmenu);
		if (k == 0) {
			// new shelf
			mainmix->mouseshelf->erase();
		}
		else if (k == 1) {
			mainprogram->pathto = "OPENSHELF";
			std::thread filereq (&Program::get_inname, mainprogram, "Open shelf file", "application/ewocvj2-shelf", boost::filesystem::canonical(mainprogram->currshelfdir).generic_string());
			filereq.detach();
		}
		else if (k == 2) {
			mainprogram->pathto = "SAVESHELF";
			std::thread filereq (&Program::get_outname, mainprogram, "Save shelf file", "application/ewocvj2-shelf", boost::filesystem::canonical(mainprogram->currshelfdir).generic_string());
			filereq.detach();
		}
		else if (k == 3) {
			mainprogram->pathto = "OPENSHELFFILES";
			std::thread filereq (&Program::get_multinname, mainprogram,  "Load file(s) in shelf", boost::filesystem::canonical(mainprogram->currshelffilesdir).generic_string());
			filereq.detach();
		}
		else if (k == 4 or k == 5) {
			// insert one of current decks into shelf element
			std::string name = remove_extension(basename(mainprogram->project->path));
			std::string path;
			int count = 0;
			while (1) {
				path = mainprogram->temppath + name + ".deck";
				if (!exists(path)) {
					break;
				}
				count++;
				name = remove_version(name) + "_" + std::to_string(count);
			}
			mainmix->mousedeck = k - 4;
			mainmix->save_deck(path);
			mainmix->mouseshelf->insert_deck(path, k - 4, mainmix->mouseshelfelem);
		}
		else if (k == 6) {
			// insert current mix into shelf element
			std::string name = remove_extension(basename(mainprogram->project->path));
			std::string path;
			int count = 0;
			while (1) {
				path = mainprogram->temppath + name + ".mix";
				if (!exists(path)) {
					break;
				}
				count++;
				name = remove_version(name) + "_" + std::to_string(count);
			}
			mainmix->save_mix(path);
			mainmix->mouseshelf->insert_mix(path, mainmix->mouseshelfelem);
		}
		else if (k == 7) {
			// insert entire shelf in a bin block
			binsmain->insertshelf = mainmix->mouseshelf;
			mainprogram->binsscreen = true;
		}
		else if (k == 8) {
			// learn MIDI for element layer load
			mainmix->learn = true;
			mainmix->learnparam = nullptr;
			mainmix->learnbutton = mainmix->mouseshelf->buttons[mainmix->mouseshelfelem];
		}

		if (mainprogram->menuchosen) {
			mainprogram->menuchosen = false;
			mainprogram->menuactivation = 0;
			mainprogram->menuresults.clear();
		}
		
		// Draw and handle filemenu
		if (mainprogram->filemenu->state > 1) {
			// make menus with layer numbers, one for each deck
			std::vector<Layer*>& lvec = choose_layers(0);
			std::vector<std::string> laylist1;
			for (int i = 0; i < lvec.size() + 1; i++) {
				laylist1.push_back("Layer " + std::to_string(i + 1));
			}
			mainprogram->make_menu("laylistmenu1", mainprogram->laylistmenu1, laylist1);
			std::vector<Layer*>& lvec2 = choose_layers(1);
			std::vector<std::string> laylist2;
			for (int i = 0; i < lvec2.size() + 1; i++) {
				laylist2.push_back("Layer " + std::to_string(i + 1));
			}
			mainprogram->make_menu("laylistmenu2", mainprogram->laylistmenu2, laylist2);
		}
		
		k = handle_menu(mainprogram->filemenu);
		if (k == 0) {
			if (mainprogram->menuresults[0] == 0) {
				// new project
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
				std::thread filereq(&Program::get_outname, mainprogram, "New project", "application/ewocvj2-project", boost::filesystem::absolute(reqdir + name).generic_string());
				filereq.detach();
			}
			else if (mainprogram->menuresults[0] == 1) {
				// new state
				mainmix->new_state();
			}
			else if (mainprogram->menuresults[0] == 2) {
				// new mix
				mainmix->new_file(2, 1);
			}
			else if (mainprogram->menuresults[0] == 3) {
				// new deck in deck A
				mainmix->new_file(0, 1);
			}
			else if (mainprogram->menuresults[0] == 4) {
				// open deck in deck B
				mainmix->new_file(1, 1);
			}
			else if (mainprogram->menuresults[0] == 5) {
				// open new layer in deck A
				std::vector<Layer*>& lvec = choose_layers(0);
				mainmix->add_layer(lvec, mainprogram->menuresults[1]);
			}
			else if (mainprogram->menuresults[0] == 5) {
				// open new layer in deck B
				std::vector<Layer*>& lvec = choose_layers(1);
				mainmix->add_layer(lvec, mainprogram->menuresults[1]);
			}
		}
		else if (k == 1) {
			if (mainprogram->menuresults[0] == 0) {
				mainprogram->pathto = "OPENPROJECT";
				std::thread filereq(&Program::get_inname, mainprogram, "Open project file", "application/ewocvj2-project", boost::filesystem::canonical(mainprogram->currprojdir).generic_string());
				filereq.detach();
			}
			else if (mainprogram->menuresults[0] == 1) {
				mainprogram->pathto = "OPENSTATE";
				std::thread filereq(&Program::get_inname, mainprogram, "Open state file", "application/ewocvj2-state", boost::filesystem::canonical(mainprogram->currstatedir).generic_string());
				filereq.detach();
			}
			else if (mainprogram->menuresults[0] == 2) {
				// open mix file
				mainprogram->pathto = "OPENMIX";
				std::thread filereq(&Program::get_inname, mainprogram, "Open mix file", "application/ewocvj2-mix", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
				filereq.detach();
			}
			else if (mainprogram->menuresults[0] == 3) {
				// open deck file in deck A
				mainmix->mousedeck = 0;
				mainprogram->pathto = "OPENDECK";
				std::thread filereq(&Program::get_inname, mainprogram, "Open deck file", "application/ewocvj2-deck", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
				filereq.detach();
			}
			else if (mainprogram->menuresults[0] == 4) {
				// open deck file in deck B
				mainmix->mousedeck = 1;
				mainprogram->pathto = "OPENDECK";
				std::thread filereq(&Program::get_inname, mainprogram, "Open deck file", "application/ewocvj2-deck", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
				filereq.detach();
			}
			else if (mainprogram->menuresults[0] == 5) {
				// open files in in deck A
				std::vector<Layer*>& lvec = choose_layers(0);
				mainprogram->pathto = "OPENFILESLAYERS";
				if (mainprogram->menuresults[1] == lvec.size()) {
					mainmix->addlay = true;
				}
				else {
					mainprogram->loadlay = lvec[mainprogram->menuresults[1]];
				}
				std::thread filereq(&Program::get_multinname, mainprogram, "Open video/image/layer file", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
				filereq.detach();
			}
			else if (mainprogram->menuresults[0] == 6) {
				// open files in layer in deck B
				std::vector<Layer*>& lvec = choose_layers(1);
				mainprogram->pathto = "OPENFILESLAYERS";
				if (mainprogram->menuresults[1] == lvec.size()) {
					mainmix->addlay = true;
				}
				else {
					mainprogram->loadlay = lvec[mainprogram->menuresults[1]];
				}
				std::thread filereq(&Program::get_multinname, mainprogram, "Open video/image/layer file", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
				filereq.detach();
			}
			else if (mainprogram->menuresults[0] == 7) {
				// open files in in deck A
				std::vector<Layer*>& lvec = choose_layers(0);
				mainprogram->pathto = "OPENFILESQUEUE";
				std::thread filereq(&Program::get_multinname, mainprogram, "Open video/image/layer file", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
				filereq.detach();
				if (mainprogram->menuresults[1] == lvec.size()) {
					mainmix->addlay = true;
				}
				else {
					mainprogram->loadlay = lvec[mainprogram->menuresults[1]];
				}
			}
			else if (mainprogram->menuresults[0] == 8) {
				// open files in layer in deck B
				std::vector<Layer*>& lvec = choose_layers(1);
				mainprogram->pathto = "OPENFILESQUEUE";
				std::thread filereq(&Program::get_multinname, mainprogram, "Open video/image/layer file", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
				filereq.detach();
				if (mainprogram->menuresults[1] == lvec.size()) {
					mainmix->addlay = true;
				}
				else {
					mainprogram->loadlay = lvec[mainprogram->menuresults[1]];
				}
			}
		}
		else if (k == 2) {
			if (mainprogram->menuresults[0] == 0) {
				mainprogram->pathto = "SAVEPROJECT";
				std::thread filereq(&Program::get_outname, mainprogram, "Save project file", "application/ewocvj2-project", boost::filesystem::canonical(mainprogram->project->path).generic_string());
				filereq.detach();
			}
			else if (mainprogram->menuresults[0] == 1) {
				mainprogram->pathto = "SAVESTATE";
				std::thread filereq(&Program::get_outname, mainprogram, "Save state file", "application/ewocvj2-state", boost::filesystem::canonical(mainprogram->currstatedir).generic_string());
				filereq.detach();
			}
			else if (mainprogram->menuresults[0] == 2) {
				mainprogram->pathto = "SAVEMIX";
				std::thread filereq(&Program::get_outname, mainprogram, "Open mix file", "application/ewocvj2-mix", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
				filereq.detach();
			}
			else if (mainprogram->menuresults[0] == 3) {
				mainmix->mousedeck = 0;
				mainprogram->pathto = "SAVEDECK";
				std::thread filereq(&Program::get_outname, mainprogram, "Save deck file", "application/ewocvj2-deck", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
				filereq.detach();
			}
			else if (mainprogram->menuresults[0] == 4) {
				mainmix->mousedeck = 1;
				mainprogram->pathto = "SAVEDECK";
				std::thread filereq(&Program::get_outname, mainprogram, "Save deck file", "application/ewocvj2-deck", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
				filereq.detach();
			}
			else if (mainprogram->menuresults[0] == 5) {
				// save layer from deck A
				std::vector<Layer*>& lvec = choose_layers(0);
				mainmix->mouselayer = lvec[mainprogram->menuresults[1]];
				mainprogram->pathto = "SAVELAYFILE";
				std::thread filereq(&Program::get_outname, mainprogram, "Save layer file", "application/ewocvj2-layer", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
				filereq.detach();
			}
			else if (mainprogram->menuresults[0] == 6) {
				// save layer from deck B
				std::vector<Layer*>& lvec = choose_layers(1);
				mainmix->mouselayer = lvec[mainprogram->menuresults[1]];
				mainprogram->pathto = "SAVELAYFILE";
				std::thread filereq(&Program::get_outname, mainprogram, "Save layer file", "application/ewocvj2-layer", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
				filereq.detach();
			}
		}
		else if (k == 3) {
			mainprogram->quitting = "quitted";
		}

		if (mainprogram->menuchosen) {
			mainprogram->menuchosen = false;
			mainprogram->menuactivation = 0;
			mainprogram->menuresults.clear();
		}

		// Draw and handle editmenu
		k = handle_menu(mainprogram->editmenu);
		if (k == 0) {
			if (!mainprogram->prefon) {
				mainprogram->prefon = true;
				SDL_ShowWindow(mainprogram->prefwindow);
				SDL_RaiseWindow(mainprogram->prefwindow);
				SDL_GL_MakeCurrent(mainprogram->prefwindow, glc_pr);
				glUseProgram(mainprogram->ShaderProgram_pr);
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				for (int i = 0; i < mainprogram->prefs->items.size(); i++) {
					PrefCat* item = mainprogram->prefs->items[i];
					item->box->upvtxtoscr();
				}
			}
			else {
				SDL_RaiseWindow(mainprogram->prefwindow);
			}
		}
		else if (k == 1) {
			if (!mainprogram->tunemidi) {
				SDL_ShowWindow(mainprogram->tunemidiwindow);
				SDL_RaiseWindow(mainprogram->tunemidiwindow);
				SDL_GL_MakeCurrent(mainprogram->tunemidiwindow, glc_tm);
				glUseProgram(mainprogram->ShaderProgram_tm);
				mainprogram->tmset->upvtxtoscr();
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
			mainprogram->menuactivation = 0;
			mainprogram->menuresults.clear();
		}

		// end of menu code
		

		// draw "layer insert into stack" blue boxes
		if (!mainprogram->menuondisplay and mainprogram->dragbinel) {
			for (int j = 0; j < 2; j++) {
				std::vector<Layer*>& lvec = choose_layers(j);
				for (int i = 0; i < lvec.size(); i++) {
					Layer* lay = lvec[i];
					if (lay->pos < mainmix->scenes[j][mainmix->currscene[j]]->scrollpos or lay->pos > mainmix->scenes[j][mainmix->currscene[j]]->scrollpos + 2) continue;
					Box* box = lay->node->vidbox;
					float thick = mainprogram->xvtxtoscr(0.075f);
					if (box->scrcoords->y1 - box->scrcoords->h < mainprogram->my and mainprogram->my < box->scrcoords->y1) {
						if (box->scrcoords->x1 - thick + (i - mainmix->scenes[j][mainmix->currscene[j]]->scrollpos == 0) * thick < mainprogram->mx and mainprogram->mx < box->scrcoords->x1 + thick) {
							// this block handles the first boxes, just not the last
							mainprogram->leftmousedown = false;
							float blue[] = { 0.5, 0.5, 1.0, 1.0 };
							// draw broad blue boxes when inserting layers
							draw_box(blue, blue, box->vtxcoords->x1 - mainprogram->xscrtovtx(thick) + (i - mainmix->scenes[j][mainmix->currscene[j]]->scrollpos == 0) * mainprogram->xscrtovtx(thick), box->vtxcoords->y1, mainprogram->xscrtovtx(thick * (2.0f - (i - mainmix->scenes[j][mainmix->currscene[j]]->scrollpos == 0))), mainprogram->layh, -1);
						}
						else if (box->scrcoords->x1 + box->scrcoords->w - thick < mainprogram->mx and mainprogram->mx < box->scrcoords->x1 + box->scrcoords->w) {
							mainprogram->leftmousedown = false;
							if (lay->pos == lvec.size() - 1 or lay->pos == mainmix->scenes[j][mainmix->currscene[j]]->scrollpos + 2) {
								// this block handles the last box
								float blue[] = { 0.5, 0.5 , 1.0, 1.0 };
								// draw broad blue boxes when inserting layers
								draw_box(blue, blue, box->vtxcoords->x1 + box->vtxcoords->w - mainprogram->xscrtovtx(thick), box->vtxcoords->y1, mainprogram->xscrtovtx(thick * (1.0f + (i - mainmix->scenes[j][mainmix->currscene[j]]->scrollpos != 2))), mainprogram->layh, -1);
							}
						}
					}
				}
			}
		}		
		
		
		//add/delete layer
		if (!mainprogram->menuondisplay and !mainprogram->dragbinel) {
			for (int j = 0; j < 2; j++) {
				std::vector<Layer*>& lvec = choose_layers(j);
				for (int i = 0; i < lvec.size(); i++) {
					Layer* lay = lvec[i];
					if (lay->pos < mainmix->scenes[j][mainmix->currscene[j]]->scrollpos or lay->pos > mainmix->scenes[j][mainmix->currscene[j]]->scrollpos + 2) continue;
					Box* box = lay->node->vidbox;
					float thick = mainprogram->xvtxtoscr(tf(0.006f));
					if (box->scrcoords->y1 - box->scrcoords->h < mainprogram->my and mainprogram->my < box->scrcoords->y1) {
						if (box->scrcoords->x1 - thick + (i - mainmix->scenes[j][mainmix->currscene[j]]->scrollpos == 0) * thick < mainprogram->mx and mainprogram->mx < box->scrcoords->x1 + thick + (i - mainmix->scenes[j][mainmix->currscene[j]]->scrollpos == 0) * thick) {
							// handle vertical boxes at layer side for adding and deleting layer
							// this block handles the first boxes, just not the last
							mainprogram->leftmousedown = false;
							float blue[] = { 0.5, 0.5, 1.0, 1.0 };
							float red[] = { 1.0, 0.0 , 0.0, 1.0 };
							draw_box(blue, blue, box->vtxcoords->x1 - mainprogram->xscrtovtx(thick) + (i - mainmix->scenes[j][mainmix->currscene[j]]->scrollpos == 0) * mainprogram->xscrtovtx(thick), box->vtxcoords->y1, mainprogram->xscrtovtx(thick) * 2.0f, mainprogram->layh, -1);
							render_text("ADD LAYER", white, box->vtxcoords->x1 - mainprogram->xscrtovtx(thick) + (i - mainmix->scenes[j][mainmix->currscene[j]]->scrollpos == 0) * mainprogram->xscrtovtx(thick), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.09f), tf(0.00024f), tf(0.0004f), 0, 1);
							if (lay->pos > 0) {
								draw_box(red, red, box->vtxcoords->x1 - mainprogram->xscrtovtx(thick) + (i - mainmix->scenes[j][mainmix->currscene[j]]->scrollpos == 0) * mainprogram->xscrtovtx(thick), box->vtxcoords->y1 + box->vtxcoords->h, mainprogram->xscrtovtx(thick * 2.0f), -tf(0.05f), -1);
								render_text("DEL", white, box->vtxcoords->x1 - mainprogram->xscrtovtx(thick) + (i - mainmix->scenes[j][mainmix->currscene[j]]->scrollpos == 0) * mainprogram->xscrtovtx(thick), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.01f), tf(0.00024f), tf(0.0004f), 0, 1);
							}
							if (mainprogram->leftmouse and !mainmix->moving and !mainprogram->intopmenu) {
								// do add/delete layer
								if (lay->pos > 0 and box->scrcoords->y1 - box->scrcoords->h < mainprogram->my and mainprogram->my < box->scrcoords->y1 - box->scrcoords->h + mainprogram->yvtxtoscr(tf(0.05f))) {
									mainmix->delete_layer(lvec, lvec[lay->pos - 1], true);
								}
								else {
									Layer* lay1;
									lay1 = mainmix->add_layer(lvec, lay->pos);
									make_layboxes();
								}
							}
						}
						else if (box->scrcoords->x1 + box->scrcoords->w - thick - (i - mainmix->scenes[j][mainmix->currscene[j]]->scrollpos == 2) * thick < mainprogram->mx and mainprogram->mx < box->scrcoords->x1 + box->scrcoords->w + (i - mainmix->scenes[j][mainmix->currscene[j]]->scrollpos != 2) * thick) {
							mainprogram->leftmousedown = false;
							if (lay->pos == lvec.size() - 1 or lay->pos == mainmix->scenes[j][mainmix->currscene[j]]->scrollpos + 2) {
								// handle vertical boxes at layer side for adding and deleting layer
								// this block handles the last box
								float blue[] = { 0.5, 0.5 , 1.0, 1.0 };
								float red[] = { 1.0, 0.0 , 0.0, 1.0 };
								draw_box(blue, blue, box->vtxcoords->x1 + box->vtxcoords->w - mainprogram->xscrtovtx(thick) * 2.0f + mainprogram->xscrtovtx(thick * ((i - mainmix->scenes[j][mainmix->currscene[j]]->scrollpos != 2))), box->vtxcoords->y1, mainprogram->xscrtovtx(thick) * 2.0f, mainprogram->layh, -1);
								render_text("ADD LAYER", white, box->vtxcoords->x1 + box->vtxcoords->w - mainprogram->xscrtovtx(thick) * 2.0f + mainprogram->xscrtovtx(thick * ((i - mainmix->scenes[j][mainmix->currscene[j]]->scrollpos != 2))), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.09f), tf(0.00024f), tf(0.0004f), 0, 1);
								draw_box(red, red, box->vtxcoords->x1 + box->vtxcoords->w - mainprogram->xscrtovtx(thick) * 2.0f + mainprogram->xscrtovtx(thick * ((i - mainmix->scenes[j][mainmix->currscene[j]]->scrollpos != 2))), box->vtxcoords->y1 + box->vtxcoords->h, mainprogram->xscrtovtx(thick) * 2.0f, -tf(0.05f), -1);
								render_text("DEL", white, box->vtxcoords->x1 + box->vtxcoords->w - mainprogram->xscrtovtx(thick) * 2.0f + mainprogram->xscrtovtx(thick * ((i - mainmix->scenes[j][mainmix->currscene[j]]->scrollpos != 2))), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.01f), tf(0.00024f), tf(0.0004f), 0, 1);
								if (mainprogram->leftmouse and !mainmix->moving) {
									// do add/delete layer
									if ((box->scrcoords->y1 - box->scrcoords->h < mainprogram->my and mainprogram->my < box->scrcoords->y1 - box->scrcoords->h + mainprogram->yvtxtoscr(tf(0.05f)))) {
										mainmix->delete_layer(lvec, lay, true);
									}
									else {
										Layer* lay1 = mainmix->add_layer(lvec, lay->pos + 1);
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
						// layer view pan
						mainprogram->leftmousedown = false;
						mainprogram->transforming = true;
						lay->transforming = 1;
						lay->transmx = mainprogram->mx - (lay->shiftx->value * (float)glob->w / 2.0f);
						printf("transmx %d\n", lay->transmx);
						lay->transmy = mainprogram->my - (lay->shifty->value * (float)glob->w / 2.0f);
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
					if (mainprogram->leftmousedown and !mainprogram->intopmenu) {
						if (!lay->vidmoving and !mainmix->moving and !lay->cliploading) {
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
										mainmix->save_layerfile(mainprogram->dragpath, mainprogram->draglay, 1, 0);
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
					// scaling layer view
					lay->scale->value -= mainprogram->mousewheel * lay->scale->value / 10.0f;
					for (int i = 0; i < loopstation->elems.size(); i++) {
						if (loopstation->elems[i]->recbut->value) {
							mainmix->adaptparam = lay->scale;
							loopstation->elems[i]->add_param();
						}
					}
					if (lay->type == ELEM_IMAGE) {
						lay->decresult->newdata = true;
					}
				}
			}
			
			if (lay->transforming == 1) {
				lay->shiftx->value = (float)(mainprogram->mx - lay->transmx) / ((float)glob->w / 2.0f);

				lay->shifty->value = (float)(mainprogram->my - lay->transmy) / ((float)glob->w / 2.0f);
				for (int i = 0; i < loopstation->elems.size(); i++) {
					if (loopstation->elems[i]->recbut->value) {
						mainmix->adaptparam = lay->shiftx;
						loopstation->elems[i]->add_param();
					}
				}
				if (mainprogram->leftmouse) {
					lay->transforming = 0;
					mainprogram->transforming = false;
				}
				if (lay->type == ELEM_IMAGE) {
					lay->decresult->newdata = true;
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
			if (mainprogram->prevmodus) {
				box->vtxcoords->y1 = -0.4f + tf(0.075f);
				box->vtxcoords->x1 = -0.15f;
				box->vtxcoords->w = 0.3f;
				box->vtxcoords->h = 0.3f;
			}
			else {
				box->vtxcoords->y1 = -1.0f;
				box->vtxcoords->x1 = -0.3f;
				box->vtxcoords->w = 0.6f;
				box->vtxcoords->h = 0.6f;
			}
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
			std::vector<MixNode*> m;
			bool pm = !mainprogram->prevmodus;
			if (mainprogram->prevmodus) {
				m = mainprogram->nodesmain->mixnodes;
			}
			else {
				m = mainprogram->nodesmain->mixnodescomp;
			}
			int deck = -1;
			if (mainprogram->deckmonitor[0]->in()) deck = 0;
			else if (mainprogram->deckmonitor[1]->in()) deck = 1;
			if (deck > -1) {
				if (mainprogram->leftmousedown) {
					mainprogram->wiping = true;
					mainmix->currlay->blendnode->wipex->value = -(((1.0f - ((mainprogram->xscrtovtx(mainprogram->mx) - 0.55f - deck * 0.9f) / 0.3f)) - 0.5f) * 2.0f - 1.5f);
					mainmix->currlay->blendnode->wipey->value = -((((2.0f - mainprogram->yscrtovtx(mainprogram->my)) / 0.3f) - 0.5f) * 2.0f - 0.50f);
					if (mainmix->currlay->blendnode->wipetype == 8 or mainmix->currlay->blendnode->wipetype == 9) {
						mainmix->currlay->blendnode->wipex->value *= 16.0f;
						mainmix->currlay->blendnode->wipey->value *= 16.0f;
					}
					for (int i = 0; i < loopstation->elems.size(); i++) {
						if (loopstation->elems[i]->recbut->value) {
							mainmix->adaptparam = mainmix->currlay->blendnode->wipex;
							loopstation->elems[i]->add_param();
						}
					}
				}
				else {
					mainprogram->wiping = false;
				}
			}
			
			Box *box = m[2]->outputbox;
			if (box->in()) {
				if (mainprogram->prevmodus) render_text("Preview Mix Monitor", white, box->vtxcoords->x1 + tf(0.01f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.03f), 0.0005f, 0.0008f);
				else render_text("Output Mix Monitor", white, box->vtxcoords->x1 + tf(0.01f), box->vtxcoords->y1 + box->vtxcoords->h - tf(0.03f), 0.0005f, 0.0008f);
				if (mainprogram->menuactivation) {
					mainprogram->wipemenu->state = 2;
					mainprogram->mixtargetmenu->value = m.size();
					mainprogram->menuactivation = false;
				}
				if (mainprogram->leftmousedown) {
					mainmix->wipex[pm]->value = -(((1.0f - ((mainprogram->xscrtovtx(mainprogram->mx) - 0.7f) / 0.6f)) - 0.5f) * 2.0f - 0.5f);
					mainmix->wipey[pm]->value = -((((2.0f - mainprogram->yscrtovtx(mainprogram->my)) / 0.6f) - 0.5f) * 2.0f - 0.5f);
					if (mainmix->currlay->blendnode->wipetype == 8 or mainmix->currlay->blendnode->wipetype == 9) {
						mainmix->wipex[pm]->value *= 16.0f;
						mainmix->wipey[pm]->value *= 16.0f;
					}
					for (int i = 0; i < loopstation->elems.size(); i++) {
						if (loopstation->elems[i]->recbut->value) {
							mainmix->adaptparam = mainmix->wipex[pm];
							loopstation->elems[i]->add_param();
						}
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
							for (int i = 0; i < mainprogram->menulist.size(); i++) {
								mainprogram->menulist[i]->menux = mainprogram->mx;
								mainprogram->menulist[i]->menuy = mainprogram->my;
							}
							mainmix->mousenode = lay->blendnode;
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
								lays[j]->queueing = true;
								mainprogram->queueing = true;
								break;
							}
						}
					}
				}
			}				
			
			if (mainprogram->binsscreen) {
				float boxwidth = tf(0.2f);
				float nmx = mainprogram->xscrtovtx(mainprogram->mx) + boxwidth / 2.0f;
				float nmy = 2.0 - mainprogram->yscrtovtx(mainprogram->my) - boxwidth / 2.0f;
				draw_box(nullptr, black, -1.0f - 0.5 * boxwidth + nmx, -1.0f - 0.5 * boxwidth + nmy, boxwidth, boxwidth, mainprogram->dragbinel->tex);
			}
				
			if (mainprogram->lmover) {
				enddrag();
				mainprogram->rightmouse = true;
				binsmain->handle(0);
				bool ret;
				if (!mainprogram->binsscreen) {
					if (mainprogram->prevmodus) {
						if (mainprogram->mx < glob->w / 2.0f) ret = exchange(lay, lvec, mainmix->layersA, 0);
						else ret = exchange(lay, lvec, mainmix->layersB, 1);
					}
					else {
						if (mainprogram->mx < glob->w / 2.0f) ret = exchange(lay, lvec, mainmix->layersAcomp, 0);
						else ret = exchange(lay, lvec, mainmix->layersBcomp, 1);
					}
				}
				if (ret) set_queueing(false);
			}				
		}
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
				
	if (mainprogram->menuactivation == true) {
		// main menu triggered
		mainprogram->genericmenu->state = 2;
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

		//deck and mix dragging
		if (mainprogram->dragbinel->type == ELEM_DECK) {
			if (!mainprogram->binsscreen) {
				// check drop of dragged deck into mix
				float lightblue[] = { 0.5f, 0.5f, 1.0f, 1.0f };
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
				if (boxA.in(mainprogram->mx, mainprogram->my)) {
					draw_box(nullptr, lightblue, -1.0f + mainprogram->numw, 1.0f - mainprogram->layh, mainprogram->layw * 3, mainprogram->layh, -1);
					if (mainprogram->lmover) {
						mainmix->mousedeck = 0;
						mainmix->open_deck(mainprogram->dragbinel->path, 1);
						mainprogram->rightmouse = true;
						binsmain->handle(0);
						enddrag();
					}
				}
				else if (boxB.in(mainprogram->mx, mainprogram->my)) {
					draw_box(nullptr, lightblue, mainprogram->numw, 1.0f - mainprogram->layh, mainprogram->layw * 3, mainprogram->layh, -1);
					if (mainprogram->lmover) {
						mainmix->mousedeck = 1;
						mainmix->open_deck(mainprogram->dragbinel->path, 1);
						enddrag();
					}
				}
			}
		}
		else if (mainprogram->dragbinel->type == ELEM_MIX) {
			if (!mainprogram->binsscreen) {
				// check drop of dragged mix into mix
				Box box;
				box.vtxcoords->x1 = -1.0f + mainprogram->numw;
				box.vtxcoords->y1 = 1.0f - mainprogram->layh;
				box.vtxcoords->w = mainprogram->layw * 6 + mainprogram->numw;
				box.vtxcoords->h = mainprogram->layh;
				box.upvtxtoscr();
				if (box.in(mainprogram->mx, mainprogram->my)) {
					draw_box(nullptr, lightblue, -1.0f + mainprogram->numw, 1.0f - mainprogram->layh, mainprogram->layw * 6 + mainprogram->numw, mainprogram->layh, -1);
					if (mainprogram->lmover) {
						mainmix->open_mix(mainprogram->dragbinel->path.c_str(), true);
						enddrag();
						mainprogram->rightmouse = true;
						binsmain->handle(0);
						enddrag();
					}
				}
			}
		}
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


	// do midi load from shelf, set up in callback, executed here at a fixed the_loop position
	ShelfElement* elem = mainprogram->midishelfelem;
	if (elem) {
		if (elem->type == ELEM_FILE) {
			mainmix->currlay->open_video(0, elem->path, true);
		}
		else if (elem->type == ELEM_IMAGE) {
			mainmix->currlay->open_image(elem->path);
		}
		else if (elem->type == ELEM_LAYER) {
			mainmix->open_layerfile(elem->path, mainmix->currlay, true, false);
		}
		else if (elem->type == ELEM_DECK) {
			mainmix->mousedeck = mainmix->currlay->deck;
			mainmix->open_deck(elem->path, true);
		}
		else if (elem->type == ELEM_MIX) {
			mainmix->open_mix(elem->path, true);
		}
		// setup framecounters for layers something was previously dragged into
		// when something new is dragged into layer, set frames from
		if (elem->launchtype == 1) {
			if (elem->cframes.size()) {
				if (elem->type == ELEM_DECK) {
					std::vector<Layer*>& lvec = choose_layers(mainmix->currlay->deck);
					for (int i = 0; i < lvec.size(); i++) {
						lvec[i]->frame = elem->cframes[i];
						lvec[i]->prevshelfdragelem = mainprogram->midishelfelem;
					}
				}
				else if (elem->type == ELEM_MIX) {
					for (int m = 0; m < 2; m++) {
						std::vector<Layer*>& lvec = choose_layers(m);
						for (int i = 0; i < lvec.size(); i++) {
							lvec[i]->frame = elem->cframes[i];
							lvec[i]->prevshelfdragelem = mainprogram->midishelfelem;
						}
					}
				}
				else {
					mainmix->currlay->frame = elem->cframes[0];
				}
				elem->cframes.clear();
			}
		}
		else if (elem->launchtype == 2) {
			if (elem->nbframes.size()) {
				if (elem->type == ELEM_DECK) {
					std::vector<Layer*>& lvec = choose_layers(mainmix->currlay->deck);
					for (int i = 0; i < lvec.size(); i++) {
						lvec[i]->frame = elem->nbframes[i]->frame;
						lvec[i]->prevshelfdragelem = mainprogram->midishelfelem;
					}
				}
				else if (elem->type == ELEM_MIX) {
					int count = 0;
					for (int m = 0; m < 2; m++) {
						std::vector<Layer*>& lvec = choose_layers(m);
						for (int i = 0; i < lvec.size(); i++) {
							lvec[i]->frame = elem->nbframes[count]->frame;
							lvec[i]->prevshelfdragelem = mainprogram->midishelfelem;
							count++;
						}
					}
				}
				else {
					mainmix->currlay->frame = elem->nbframes[0]->frame;
				}
				elem->nbframes.clear();
			}
		}
		if (elem->type == ELEM_DECK) {
			std::vector<Layer*>& lvec = choose_layers(mainmix->currlay->deck);
			for (int i = 0; i < lvec.size(); i++) {
				lvec[i]->prevshelfdragelem = mainprogram->midishelfelem;
			}
		}
		else if (elem->type == ELEM_MIX) {
			for (int m = 0; m < 2; m++) {
				std::vector<Layer*>& lvec = choose_layers(m);
				for (int i = 0; i < lvec.size(); i++) {
					lvec[i]->prevshelfdragelem = mainprogram->midishelfelem;
				}
			}
		}

		mainmix->currlay->prevshelfdragelem = mainprogram->midishelfelem;
		mainprogram->midishelfelem = nullptr;
	}


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
		render_text("Awaiting MIDI input.", white, -0.1f, 0.2f, 0.001f, 0.0016f);
		render_text("Rightclick cancels.", white, -0.1f, 0.06f, 0.001f, 0.0016f);
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

	if (!mainprogram->prefon and !mainprogram->tunemidi) tooltips_handle(0);
	
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
		mainmix->save_state(path, true);
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
	
	bool prret = false;
	GLuint tex, fbo;
	if (mainprogram->quitting != "") {
		SDL_ShowWindow(mainprogram->prefwindow);
		SDL_RaiseWindow(mainprogram->prefwindow);
		SDL_GL_MakeCurrent(mainprogram->prefwindow, glc_pr);
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
	}

	bool dmbu = mainprogram->directmode;
	if (mainprogram->prefon) {
		mainprogram->directmode = true;
		SDL_GL_MakeCurrent(mainprogram->prefwindow, glc_pr);
		glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->smglobfbo_pr);
		mainprogram->drawbuffer = mainprogram->smglobfbo_pr;
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
		prret = preferences();
		if (prret) {
			glBlitNamedFramebuffer(mainprogram->smglobfbo_pr, 0, 0, 0, smw, smh, 0, 0, smw, smh, GL_COLOR_BUFFER_BIT, GL_NEAREST);
			SDL_GL_SwapWindow(mainprogram->prefwindow);
		}
		SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
	}
	if (mainprogram->tunemidi) {
		mainprogram->directmode = true;
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
		mainprogram->drawbuffer = mainprogram->smglobfbo_tm;
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
		bool ret = tune_midi();
		if (ret) {
			mainprogram->drawbuffer = -1;
			glBlitNamedFramebuffer(mainprogram->smglobfbo_tm, 0, 0, 0, smw, smh , 0, 0, smw, smh, GL_COLOR_BUFFER_BIT, GL_NEAREST);
			SDL_GL_SwapWindow(mainprogram->tunemidiwindow);
		}
		SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
	}
	mainprogram->directmode = dmbu;

	glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
	mainprogram->drawbuffer = mainprogram->globfbo;
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	if (!mainprogram->directmode and mainprogram->quitting == "") {

		glEnable(GL_DEPTH_TEST);
		glClearDepth(1.0f);
		glClear(GL_DEPTH_BUFFER_BIT);
		glDepthFunc(GL_LESS);

		glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
		mainprogram->drawbuffer = mainprogram->globfbo;
		glDrawBuffer(GL_COLOR_ATTACHMENT0);

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
			else draw_box(elem->box->linec, elem->box->areac, elem->box->x, elem->box->y, elem->box->wi, elem->box->he, 0.0f, 0.0f, 1.0f, 1.0f, elem->box->circle, elem->box->tex, glob->w, glob->h, elem->box->text);
		}
		mainprogram->directmode = false;
	}
	mainprogram->directmode = dmbu;

	glBlitNamedFramebuffer(mainprogram->globfbo, 0, 0, 0, glob->w, glob->h , 0, 0, glob->w, glob->h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	
	SDL_GL_SwapWindow(mainprogram->mainwindow);
	glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->globfbo);
	mainprogram->drawbuffer = mainprogram->globfbo;
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	
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

	glc = SDL_GL_CreateContext(mainprogram->mainwindow);

	//glewExperimental = GL_TRUE;
	glewInit();

	mainprogram->dummywindow = SDL_CreateWindow("", 0, 0, 32, 32, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN | SDL_WINDOW_ALLOW_HIGHDPI);
	mainprogram->tunemidiwindow = SDL_CreateWindow("Tune MIDI", glob->w / 4, glob->h / 4, glob->w / 2, glob->h / 2, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN | SDL_WINDOW_ALLOW_HIGHDPI);
	mainprogram->prefwindow = SDL_CreateWindow("Preferences", glob->w / 4, glob->h / 4, glob->w / 2, glob->h / 2, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN | SDL_WINDOW_ALLOW_HIGHDPI);
	SDL_GL_GetDrawableSize(mainprogram->prefwindow, &wi, &he);
	smw = (float)wi;
	smh = (float)he;
	
	glc_tm = SDL_GL_CreateContext(mainprogram->tunemidiwindow);
	SDL_GL_MakeCurrent(mainprogram->tunemidiwindow, glc_tm);
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


	FT_Library ft;
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
	mainprogram->make_menu("loadmenu", mainprogram->loadmenu, loadops);

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
  	mainprogram->make_menu("genericmenu", mainprogram->genericmenu, generic);

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
	for (int m = 0; m < 2; m++) {
		std::vector<Layer*> &lvec = choose_layers(m);
		for (int i = 0; i < 4; i++) {
			mainmix->currscene[m] = i;
			Layer *lay = mainmix->add_layer(lvec, 0);
			lay->closethread = true;
			mainmix->scenes[m][mainmix->currscene[m]]->nbframes.insert(mainmix->scenes[m][mainmix->currscene[m]]->nbframes.begin(), lay);
			lay->clips.clear();
			mainmix->mousedeck = m;
			mainmix->do_save_deck(mainprogram->temppath + "tempdeck_" + std::to_string(m) + std::to_string(i + 1) + ".deck", false, false);
			SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
			mainmix->do_save_deck(mainprogram->temppath + "tempdeck_" + std::to_string(m) + std::to_string(i + 1) + "comp.deck", false, false);
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
				mainprogram->currbinsdir = dirname(mainprogram->path);
				Bin *bin = binsmain->new_bin(remove_extension(basename(mainprogram->path)));
				binsmain->open_bin(mainprogram->path, bin);
				if (binsmain->bins.size() >= 20) binsmain->binsscroll++;
				std::string path = mainprogram->project->binsdir + bin->name + ".bin";
				binsmain->save_bin(path);
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
				mainprogram->currprojdir = dirname(mainprogram->path);
				if (!exists(mainprogram->path)) mainprogram->project->newp(mainprogram->path);
				mainprogram->project->save(mainprogram->path);
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
						else if (!mainprogram->tunemidi) {
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


