#if defined(WIN32) && !defined(__linux__)
#define WINDOWS
#elif defined(__linux__) && !defined(WIN32)
#define POSIX
#endif


#include "boost/bind.hpp"
#include "boost/asio.hpp"
#include "boost/thread/thread.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/filesystem.hpp"
#include "boost/foreach.hpp"

#include <stdio.h>
#include <stdint.h>
#include <cstdlib>

#include <cstring>
#include <string>
#include <assert.h>
#include <ostream>
#include <istream>
#include <iostream>
#include <fstream>
#include <ios>
#include <vector>
#include <numeric>
#include <unordered_set>
#include <list>
#include <map>
#include <time.h>
#include <thread>
#include <mutex>
#ifndef UINT64_C
#define UINT64_C(c) (c ## ULL)
#endif

#define NTDDI_VERSION NTDDI_WIN10
#ifdef WINDOWS
#include "direnthwin/dirent.h"
#include <intrin.h>
#include <shobjidl.h>
#include <Vfw.h>
#include <winsock2.h>
#include <winbase.h>
#define STRSAFE_NO_DEPRECATE
#include <tchar.h>
#include <initguid.h>
#include <dshow.h>
#pragma comment (lib, "strmiids.lib")
#pragma comment (lib, "Quartz.lib")
#pragma comment (lib, "ole32.lib")
#include <windows.h>
#include <tlhelp32.h>
#include <ShellScalingApi.h>
#include <comdef.h>
#endif

#include <turbojpeg.h>
#include "GL/glew.h"
#include "GL/gl.h"
#include "GL/glut.h"
#ifdef POSIX
#include <alsa/asoundlib.h>
#endif
#include "SDL2/SDL.h"
#include "SDL2/SDL_syswm.h"
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include "snappy-c.h"
#include <lo/lo.h>
#include <lo/lo_cpp.h>

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

// my own headers
#include "box.h"
#include "effect.h"
#include "node.h"
#include "layer.h"
#include "window.h"
#include "program.h"
#include "loopstation.h"
#include "bins.h"
#include "retarget.h"

#define PROGRAM_NAME "EWOCvj"

#ifdef WINDOWS

#endif


FT_Library ft;
Globals *glob = nullptr;
Program *mainprogram = nullptr;
Mixer *mainmix = nullptr;
BinsMain *binsmain = nullptr;
LoopStation *loopstation = nullptr;
LoopStation *lp = nullptr;
LoopStation *lpc = nullptr;
Retarget *retarget = nullptr;
float smw, smh;
SDL_GLContext glc;
SDL_GLContext orderglc;
float white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
float halfwhite[] = { 1.0f, 1.0f, 1.0f, 0.5f };
float black[] = { 0.0f, 0.0f, 0.0f, 1.0f };
float orange[] = { 1.0f, 0.5f, 0.0f, 1.0f };
float purple[] = { 0.5f, 0.5f, 1.0f, 1.0f };
float yellow[] = { 0.9f, 0.8f, 0.0f, 1.0f };
float lightblue[4] = { 0.5f, 0.5f, 1.0f, 1.0f };
float darkblue[] = { 0.1f, 0.5f, 1.0f, 1.0f };
float red[] = { 1.0f, 0.0f, 0.0f, 1.0f };
float lightgrey[] = { 0.7f, 0.7f, 0.7f, 1.0f };
float grey[] = { 0.4f, 0.4f, 0.4f, 1.0f };
float pink[] = { 1.0f, 0.5f, 0.5f, 1.0f };
float green[] = { 0.3f, 0.8f, 0.4f, 1.0f };
float darkgreygreen[] = { 0.0f, 0.15f, 0.0f, 1.0f };
float darkgreen1[] = { 0.0f, 0.4f, 0.0f, 1.0f };
float darkgreen2[] = { 0.0f, 0.2f, 0.0f, 1.0f };
float blue[] = { 0.0f, 0.2f, 1.0f, 1.0f };
float alphawhite[] = { 1.0f, 1.0f, 1.0f, 0.5f };
float alphablue[] = { 0.7f, 0.7f, 1.0f, 0.5f };
float alphagreen[] = { 0.0f, 1.0f, 0.2f, 0.5f };
float darkred1[] = { 0.3f, 0.0f, 0.0f, 1.0f };
float darkred2[] = { 0.2f, 0.0f, 0.0f, 1.0f };
float darkred3[] = { 0.2f, 0.0f, 0.0f, 0.5f };
float darkgrey[] = { 0.2f, 0.2f, 0.2f, 1.0f };

static GLuint mixvao;
static GLuint thmvbuf;
static GLuint thmvao;
static GLuint boxvao;
FT_Face face;
LayMidi *laymidiA;
LayMidi *laymidiB;
LayMidi *laymidiC;
LayMidi *laymidiD;
std::vector<Box*> allboxes;
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
	if (pathname.size() != maxpos)
	{
		pathname.erase(maxpos + 1, std::string::npos);
	}
	if (maxpos == 0) pathname = "";
    return pathname;
}

std::string basename(std::string pathname)
{
    if (pathname != "") {
        while (pathname.substr(pathname.size() - 1, 1) == "/") {
            pathname = pathname.substr(0, pathname.size() - 1);
        }
        while (pathname.substr(pathname.size() - 1, 1) == "\\") {
            pathname = pathname.substr(0, pathname.size() - 1);
        }

        size_t last_slash_idx1 = pathname.find_last_of("/");
        size_t last_slash_idx2 = pathname.find_last_of("\\");
        if (last_slash_idx1 == std::string::npos) last_slash_idx1 = 0;
        if (last_slash_idx2 == std::string::npos) last_slash_idx2 = 0;
        size_t maxpos = last_slash_idx2;
        if (last_slash_idx1 > last_slash_idx2) maxpos = last_slash_idx1;
        if (std::string::npos != maxpos) {
            pathname.erase(0, maxpos + 1);
        }
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

std::string pathtoplatform(std::string path) {
#ifdef WINDOWS
    std::replace(path.begin(), path.end(), '/', '\\');
#endif
#ifdef POSIX
    std::replace(path.begin(), path.end(), '\\', '\');
    std::replace(path.begin(), path.end(), '\', '\\');
    std::replace(path.begin(), path.end(), '\\', '/');
#endif
    return path;
}

void copy_dir(std::string &src, std::string &dest) {
    namespace fs = boost::filesystem;

    if (!fs::exists(src) || !fs::is_directory(src)) {
        throw std::runtime_error("Source directory " + src + " does not exist or is not a directory");
    }
    if (!fs::exists(dest)) {
        if (!fs::create_directory(fs::path(dest))) {
            throw std::runtime_error("Cannot create destination directory " + dest);
        }
    }

    for (const auto& dirEnt : fs::recursive_directory_iterator{src})
    {
        const auto& path = dirEnt.path();
        auto relativePathStr = path.generic_string();
        boost::replace_first(relativePathStr, src, "");
        if (exists(dest + "/" + relativePathStr)) {
            boost::filesystem::remove_all(dest + "/" + relativePathStr);
        }
        fs::copy(path, dest + "/" + relativePathStr);
    }
}


bool isimage(const std::string &path) {
    ILuint boundimage;
    ilGenImages(1, &boundimage);
    ilBindImage(boundimage);
    ILboolean ret = ilLoadImage(path.c_str());
    ilDeleteImages(1, &boundimage);
	return (bool)ret;
}


bool isvideo(const std::string &path) {
    AVFormatContext *test = avformat_alloc_context();
    int r = avformat_open_input(&test, path.c_str(), nullptr, nullptr);
    if (r < 0) return false;
    if (avformat_find_stream_info(test, nullptr) < 0) {
        avformat_close_input(&test);
        return false;
    }
    avformat_close_input(&test);
    return true;
}

std::istream& safegetline(std::istream& is, std::string& t)
{
    t.clear();

    // The characters in the stream are read one-by-one using a std::streambuf.
    // That is faster than reading them one-by-one using the std::istream.
    // Code that uses streambuf this way must be guarded by a sentry object.
    // The sentry object performs various tasks,
    // such as thread synchronization and updating the stream state.

    std::istream::sentry se(is, true);
    std::streambuf* sb = is.rdbuf();

    for(;;) {
        int c = sb->sbumpc();
        switch (c) {
            case '\n':
                return is;
            case '\r':
                if(sb->sgetc() == '\n')
                    sb->sbumpc();
                return is;
            case std::streambuf::traits_type::eof():
                // Also handle the case when the last line has no line ending
                if(t.empty())
                    is.setstate(std::ios::eofbit);
                return is;
            default:
                t += (char)c;
        }
    }
}


std::string find_unused_filename(std::string basename, std::string path, std::string extension) {
    std::string outpath;
    std::string name = remove_extension(basename);
    int count = 0;
    while (1) {
        outpath = path + name + extension;
        if (!exists(outpath)) {
            return outpath;
        }
        count++;
        name = remove_version(name) + "_" + std::to_string(count);
    }
}


#ifdef POSIX
std::string exec(const char* cmd) {
    // executes Linux command cmd and returns output in result
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}
#endif


void check_stage() {
    mainmix->newpathpos++;
    if (mainmix->newpathpos >= (*(mainmix->newpaths)).size()) {
        mainmix->retargetstage++;
        if (mainmix->retargetstage == 4) {
            mainmix->retargetstage = 0;
            mainmix->retargetingdone = true;
        }
        mainmix->newpathpos = 0;
    }
}

bool retarget_search() {
    std::function<bool(std::string)> search_directory = [&search_directory](std::string path) {
        boost::filesystem::recursive_directory_iterator end;
        std::string bn = basename((*(mainmix->newpaths))[mainmix->newpathpos]);
        auto it = find_if(boost::filesystem::recursive_directory_iterator(path), end, [&bn](boost::filesystem::directory_entry& e) {
            return (e.path().filename() == bn);
        });
        if (it == end) return false;
        else return true;
    };
    std::function<std::string(std::string)> search_directory_for_same_size = [&search_directory_for_same_size](std::string path) {
        for (boost::filesystem::recursive_directory_iterator itr(path); itr!=boost::filesystem::recursive_directory_iterator(); ++itr)
        {
            if (boost::filesystem::file_size(itr->path()) == retarget->filesize) {
                return itr->path().generic_string();
            }
        };
        std::string s("");
        return s;
    };
    bool ret;
    for (std::string dirpath : retarget->globalsearchdirs) {
        ret = search_directory(dirpath);
        if (ret) {
            (*(mainmix->newpaths))[mainmix->newpathpos] = dirpath = "/" + basename((*(mainmix->newpaths))[mainmix->newpathpos]);
            break;
        }
    }
    if (!ret) {
        for (std::string dirpath : retarget->localsearchdirs) {
            ret = search_directory(dirpath);
            if (ret) {
                (*(mainmix->newpaths))[mainmix->newpathpos] = dirpath + "/" + basename((*(mainmix->newpaths))[mainmix->newpathpos]);
                break;
            }
        }
    }
    if (!ret) {
        for (std::string dirpath: retarget->globalsearchdirs) {
            std::string retstr = search_directory_for_same_size(dirpath);
            if (retstr != "") {
                (*(mainmix->newpaths))[mainmix->newpathpos] = retstr;
                ret = true;
                break;
            }
            else ret = false;
        }
    }
    if (!ret) {
        for (std::string dirpath: retarget->localsearchdirs) {
            std::string retstr = search_directory_for_same_size(dirpath);
            if (retstr != "") {
                (*(mainmix->newpaths))[mainmix->newpathpos] = retstr;
                ret = true;
                break;
            }
            else ret = false;
        }
    }
    return ret;
}

void do_retarget() {
    for (int i = 0; i < mainmix->newpathlayers.size(); i++) {
        Layer *lay = mainmix->newpathlayers[i];
        lay->filename = mainmix->newlaypaths[i];
        if (lay->filename == "") {
            lay->frame = 0.0f;
            lay->prevframe = -1;
            lay->numf = 0.0f;
            lay->startframe->value = 0.0f;
            lay->endframe->value = 0.0f;
            continue;
        }
        lay->timeinit = false;
        if (lay->type == ELEM_FILE || lay->type == ELEM_LAYER) {
            lay = lay->open_video(lay->frame, lay->filename, false);
        } else if (lay->type == ELEM_IMAGE) {
            float bufr = lay->frame;
            lay->open_image(lay->filename);
            lay->frame = bufr;
        }
    }
    for (int i = 0; i < mainmix->newpathclips.size(); i++) {
        Clip *clip = mainmix->newpathclips[i];
        clip->path = mainmix->newclippaths[i];
        if (clip->path == "") continue;
        clip->insert(clip->layer, clip->layer->clips.end() - 1);
    }
    for (int i = 0; i < mainmix->newpathshelfelems.size(); i++) {
        mainmix->newpathshelfelems[i]->path = mainmix->newshelfpaths[i];
        if (mainmix->newpathshelfelems[i]->path == "") {
            blacken(mainmix->newpathshelfelems[i]->tex);
        }
    }
    for (int i = 0; i < mainmix->newpathbinels.size(); i++) {
        mainmix->newpathbinels[i]->path = mainmix->newbinelpaths[i];
        if (mainmix->newpathbinels[i]->path == "") {
            mainmix->newpathbinels[i]->name = "";
            blacken(mainmix->newpathbinels[i]->tex);
        }
    }
    check_stage();
}

void make_searchbox() {
    short j = retarget->searchboxes.size();
    Box *box = new Box;
    box->vtxcoords->x1 = -0.4f;
    box->vtxcoords->y1 = -0.2f - j * 0.1f;
    box->vtxcoords->w = 0.8f;
    box->vtxcoords->h = 0.1f;
    box->upvtxtoscr();
    box->tooltiptitle = "Search location list ";
    box->tooltip = "Leftclick path to edit. ";
    retarget->searchboxes.push_back(box);
    Button *globbut = new Button(true);
    globbut->toggle = 1;
    globbut->box->vtxcoords->x1 = 0.3f;
    globbut->box->vtxcoords->y1 = -0.175f - j * 0.1f;
    globbut->box->vtxcoords->w = 0.025f;
    globbut->box->vtxcoords->h = 0.05f;
    globbut->box->upvtxtoscr();
    globbut->box->tooltiptitle = "Save to default search list? ";
    globbut->box->tooltip = "Leftclick to toggle if this path will be saved to the permanent search directory list. ";
    retarget->searchglobalbuttons.push_back(globbut);
    Box *clbox = new Box;
    clbox->vtxcoords->x1 = 0.35f;
    clbox->vtxcoords->y1 = -0.175f - j * 0.1f;
    clbox->vtxcoords->w = 0.025f;
    clbox->vtxcoords->h = 0.05f;
    clbox->upvtxtoscr();
    clbox->tooltiptitle = "Delete path ";
    clbox->tooltip = "Leftclick to delete this path from the list. ";
    retarget->searchclearboxes.push_back(clbox);
}


class Deadline2
{
public:
    Deadline2(deadline_timer &timer) : t(timer) {
        wait();
    }

    void timeout() {
        if (!mainmix->donerec[0]) {
            // layer rec
            // grab frames for output recording
#define BUFFER_OFFSET(i) ((char *)nullptr + (i))
            glBindBuffer(GL_PIXEL_PACK_BUFFER, mainmix->ioBuf);
            bool cbool;
            if (!mainprogram->prevmodus) {
                glBufferData(GL_PIXEL_PACK_BUFFER, (int)(mainprogram->ow * mainprogram->oh) * 4, nullptr, GL_DYNAMIC_READ);
                cbool = 1;
            }
            else {
                glBufferData(GL_PIXEL_PACK_BUFFER, (int)(mainprogram->ow3 * mainprogram->oh3) * 4, nullptr, GL_DYNAMIC_READ);
                cbool = 0;
            }
            if (mainmix->reclay->effects[0].size() == 0) {
                glBindFramebuffer(GL_FRAMEBUFFER, mainmix->reclay->fbo);
            }
            else {
                glBindFramebuffer(GL_FRAMEBUFFER, mainmix->reclay->effects[0][mainmix->reclay->effects[0].size() - 1]->fbo);
            }
            glReadBuffer(GL_COLOR_ATTACHMENT0);
            if (cbool) glReadPixels(0, 0, mainprogram->ow, (int)mainprogram->oh, GL_BGRA, GL_UNSIGNED_BYTE, BUFFER_OFFSET(0));
            else glReadPixels(0, 0, mainprogram->ow3, (int)mainprogram->oh3, GL_BGRA, GL_UNSIGNED_BYTE, BUFFER_OFFSET(0));
            mainmix->rgbdata = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
            assert(mainmix->rgbdata);

            if (mainmix->recording[0]) {
                mainmix->recordnow[0] = true;
                while (mainmix->recordnow[0] && !mainmix->donerec[0]) {
                    mainmix->startrecord[0].notify_all();
                }
            }

            glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
            glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        }
        if (!mainmix->donerec[1]) {
            // Q rec
            // grab frames for output recording
#define BUFFER_OFFSET(i) ((char *)nullptr + (i))
            glBindBuffer(GL_PIXEL_PACK_BUFFER, mainmix->ioBuf);
            glBufferData(GL_PIXEL_PACK_BUFFER, (int) (mainprogram->ow * mainprogram->oh) * 4, nullptr,
                         GL_DYNAMIC_READ);
            glBindFramebuffer(GL_FRAMEBUFFER, ((MixNode *) mainprogram->nodesmain->mixnodescomp[2])->mixfbo);
            glReadBuffer(GL_COLOR_ATTACHMENT0);
            glReadPixels(0, 0, mainprogram->ow, (int)mainprogram->oh, GL_BGRA, GL_UNSIGNED_BYTE, BUFFER_OFFSET(0));
            mainmix->rgbdata = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
            assert(mainmix->rgbdata);
            if (mainmix->recording[1]) {
                mainmix->recordnow[1] = true;
                while (mainmix->recordnow[1] && !mainmix->donerec[1]) {
                    mainmix->startrecord[1].notify_all();
                }
            }
            glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
            glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
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
        dl.cancel();
        return;
    }
private:
    Deadline2 &dl;
};



void handle_midi(LayMidi *laymidi, int deck, int midi0, int midi1, int midi2, std::string &midiport) {
	std::vector<Layer*> &lvec = choose_layers(deck);
	int genmidideck = mainmix->genmidi[deck]->value;
	for (int j = 0; j < lvec.size(); j++) {
		if (lvec[j]->genmidibut->value == genmidideck) {
            if (midi0 == laymidi->play->midi0 && midi1 == laymidi->play->midi1 && midi2 != 0 && midiport == laymidi->play->midiport) {
                lvec[j]->playbut->value = !lvec[j]->playbut->value;
                lvec[j]->revbut->value = false;
                lvec[j]->bouncebut->value = false;
            }
            if (midi0 == laymidi->backw->midi0 && midi1 == laymidi->backw->midi1 && midi2 != 0 && midiport == laymidi->backw->midiport) {
                lvec[j]->revbut->value = !lvec[j]->revbut->value;
                lvec[j]->playbut->value = false;
                lvec[j]->bouncebut->value = false;
            }
			if (midi0 == laymidi->pausestop->midi0 && midi1 == laymidi->pausestop->midi1 && midi2 != 0 && midiport == laymidi->pausestop->midiport) {
                lvec[j]->playbut->value = false;
                lvec[j]->revbut->value = false;
                lvec[j]->bouncebut->value = false;
			}
			if (midi0 == laymidi->bounce->midi0 && midi1 == laymidi->bounce->midi1 && midi2 != 0 && midiport == laymidi->bounce->midiport) {
				lvec[j]->bouncebut->value = !lvec[j]->bouncebut->value;
                lvec[j]->playbut->value = false;
                lvec[j]->revbut->value = false;
            }
			if (midi0 == laymidi->scratch->midi0 && midi1 == laymidi->scratch->midi1 && midiport == laymidi->scratch->midiport) {
				lvec[j]->scratch = ((float)midi2 - 64.0f) / 4.0f;
			}
			if (midi0 == laymidi->frforw->midi0 && midi1 == laymidi->frforw->midi1 && midi2 != 0 && midiport == laymidi->frforw->midiport) {
				lvec[j]->frame += 1;
				if (lvec[j]->frame >= lvec[j]->numf) lvec[j]->frame = 0;
			}
			if (midi0 == laymidi->frbackw->midi0 && midi1 == laymidi->frbackw->midi1 && midi2 != 0 && midiport == laymidi->frbackw->midiport) {
				lvec[j]->frame -= 1;
				if (lvec[j]->frame < 0) lvec[j]->frame = lvec[j]->numf - 1;
			}
			if (midi0 == laymidi->scratchtouch->midi0 && midi1 == laymidi->scratchtouch->midi1 && midi2 != 0 && midiport == laymidi->scratchtouch->midiport) {
				lvec[j]->scratchtouch = 1;
			}
			if (midi0 == laymidi->scratchtouch->midi0 && midi1 == laymidi->scratchtouch->midi1 && midi2 == 0 && midiport == laymidi->scratchtouch->midiport) {
				lvec[j]->scratchtouch = 0;
			}
			if (midi0 == laymidi->speed->midi0 && midi1 == laymidi->speed->midi1 && midiport == laymidi->speed->midiport) {
				int m2 = -(midi2 - 127);
				if (m2 >= 64.0f) {
					lvec[j]->speed->value = 1.0f + (2.33f / 64.0f) * (m2 - 64.0f);
				}
				else {
					lvec[j]->speed->value = 0.0f + (1.0f / 64.0f) * m2;
				}
			}
			if (midi0 == laymidi->speedzero->midi0 && midi1 == laymidi->speedzero->midi1 && midiport == laymidi->speedzero->midiport) {
				lvec[j]->speed->value = 1.0f;
			}
			if (midi0 == laymidi->opacity->midi0 && midi1 == laymidi->opacity->midi1 && midiport == laymidi->opacity->midiport) {
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
  	printf("MIDI %d %d %f \n", midi0, midi1, midi2);
  	std::string midiport = ((PrefItem*)userData)->name;

      if (midi0 == mainmix->prevmidi0 && midi1 == mainmix->prevmidi1 && midi2 == 0) {
          mainmix->prevmidi0 = -1;
          mainmix->prevmidi1 = -1;
          return;
      }
  	
 	if (mainprogram->waitmidi == 0 && mainprogram->tmlearn) {
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
  	    if (mainprogram->configcatmidi == 0) {
            LayMidi *lm = nullptr;
            if (mainprogram->midipresetsset == 0) lm = laymidiA;
            else if (mainprogram->midipresetsset == 1) lm = laymidiB;
            else if (mainprogram->midipresetsset == 2) lm = laymidiC;
            else if (mainprogram->midipresetsset == 3) lm = laymidiD;
            bool bupm = mainprogram->prevmodus;
            switch (mainprogram->tmlearn) {
                case TM_NONE:
                    if (lm->play->midi0 == midi0 && lm->play->midi1 == midi1 &&
                        lm->play->midiport == midiport)
                        mainprogram->tmchoice = TM_PLAY;
                    else if (lm->backw->midi0 == midi0 && lm->backw->midi1 == midi1 &&
                             lm->backw->midiport == midiport)
                        mainprogram->tmchoice = TM_BACKW;
                    else if (lm->bounce->midi0 == midi0 && lm->bounce->midi1 == midi1 &&
                             lm->bounce->midiport == midiport)
                        mainprogram->tmchoice = TM_BOUNCE;
                    else if (lm->frforw->midi0 == midi0 && lm->frforw->midi1 == midi1 &&
                             lm->frforw->midiport == midiport)
                        mainprogram->tmchoice = TM_FRFORW;
                    else if (lm->frbackw->midi0 == midi0 && lm->frbackw->midi1 == midi1 &&
                             lm->frbackw->midiport == midiport)
                        mainprogram->tmchoice = TM_FRBACKW;
                    else if (lm->stop->midi0 == midi0 && lm->stop->midi1 == midi1 &&
                             lm->stop->midiport == midiport)
                        mainprogram->tmchoice = TM_STOP;
                    else if (lm->loop->midi0 == midi0 && lm->loop->midi1 == midi1 &&
                             lm->loop->midiport == midiport)
                        mainprogram->tmchoice = TM_LOOP;
                    else if (lm->speed->midi0 == midi0 && lm->speed->midi1 == midi1 &&
                             lm->speed->midiport == midiport)
                        mainprogram->tmchoice = TM_SPEED;
                    else if (lm->speedzero->midi0 == midi0 && lm->speedzero->midi1 == midi1 &&
                             lm->speedzero->midiport == midiport)
                        mainprogram->tmchoice = TM_SPEEDZERO;
                    else if (lm->opacity->midi0 == midi0 && lm->opacity->midi1 == midi1 &&
                             lm->opacity->midiport == midiport)
                        mainprogram->tmchoice = TM_OPACITY;
                    else if (lm->scratchtouch->midi0 == midi0 && lm->scratchtouch->midi0 == midi1 &&
                             lm->scratchtouch->midiport == midiport)
                        mainprogram->tmchoice = TM_FREEZE;
                    else if (lm->scratch->midi0 == midi0 && lm->scratch->midi1 == midi1 &&
                             lm->scratch->midiport == midiport)
                        mainprogram->tmchoice = TM_SCRATCH;
                    else if (mainmix->crossfade->midi[0] == midi0 && mainmix->crossfade->midi[1] == midi1 &&
                            mainmix->crossfade->midiport == midiport)
                        mainprogram->tmchoice = TM_CROSS;
                    else mainprogram->tmchoice = TM_NONE;
                    return;
                    break;
                case TM_PLAY:
                    lm->play->midi0 = midi0;
                    lm->play->midi1 = midi1;
                    lm->play->midiport = midiport;
                    mainprogram->prevmodus = true;
                    lm->play->register_midi();
                    mainprogram->prevmodus = false;
                    lm->play->register_midi();
                    mainprogram->tmlearn = TM_NONE;
                    break;
                case TM_BACKW:
                    lm->backw->midi0 = midi0;
                    lm->backw->midi1 = midi1;
                    lm->backw->midiport = midiport;
                    mainprogram->prevmodus = true;
                    lm->backw->register_midi();
                    mainprogram->prevmodus = false;
                    lm->backw->register_midi();
                    mainprogram->tmlearn = TM_NONE;
                    break;
                case TM_BOUNCE:
                    lm->bounce->midi0 = midi0;
                    lm->bounce->midi1 = midi1;
                    lm->bounce->midiport = midiport;
                    mainprogram->prevmodus = true;
                    lm->bounce->register_midi();
                    mainprogram->prevmodus = false;
                    lm->bounce->register_midi();
                    mainprogram->tmlearn = TM_NONE;
                    break;
                case TM_FRFORW:
                    lm->frforw->midi0 = midi0;
                    lm->frforw->midi1 = midi1;
                    lm->frforw->midiport = midiport;
                    mainprogram->prevmodus = true;
                    lm->frforw->register_midi();
                    mainprogram->prevmodus = false;
                    lm->frforw->register_midi();
                    mainprogram->tmlearn = TM_NONE;
                    break;
                case TM_FRBACKW:
                    lm->frbackw->midi0 = midi0;
                    lm->frbackw->midi1 = midi1;
                    lm->frbackw->midiport = midiport;
                    mainprogram->prevmodus = true;
                    lm->frbackw->register_midi();
                    mainprogram->prevmodus = false;
                    lm->frbackw->register_midi();
                    mainprogram->tmlearn = TM_NONE;
                    break;
                case TM_STOP:
                    if (midi0 == 144) return;
                    lm->stop->midi0 = midi0;
                    lm->stop->midi1 = midi1;
                    lm->stop->midiport = midiport;
                    mainprogram->prevmodus = true;
                    lm->stop->register_midi();
                    mainprogram->prevmodus = false;
                    lm->stop->register_midi();
                    mainprogram->tmlearn = TM_NONE;
                    break;
                case TM_LOOP:
                    if (midi0 == 144) return;
                    lm->loop->midi0 = midi0;
                    lm->loop->midi1 = midi1;
                    lm->loop->midiport = midiport;
                    mainprogram->prevmodus = true;
                    lm->loop->register_midi();
                    mainprogram->prevmodus = false;
                    lm->loop->register_midi();
                    mainprogram->tmlearn = TM_NONE;
                    break;
                case TM_SPEED:
                    if (midi0 == 144) return;
                    lm->speed->midi0 = midi0;
                    lm->speed->midi1 = midi1;
                    lm->speed->midiport = midiport;
                    mainprogram->prevmodus = true;
                    lm->speed->register_midi();
                    mainprogram->prevmodus = false;
                    lm->speed->register_midi();
                    mainprogram->tmlearn = TM_NONE;
                    break;
                case TM_SPEEDZERO:
                    if (midi0 == 176) return;
                    lm->speedzero->midi0 = midi0;
                    lm->speedzero->midi1 = midi1;
                    lm->speedzero->midiport = midiport;
                    mainprogram->prevmodus = true;
                    lm->speedzero->register_midi();
                    mainprogram->prevmodus = false;
                    lm->speedzero->register_midi();
                    mainprogram->tmlearn = TM_NONE;
                    break;
                case TM_OPACITY:
                    lm->opacity->midi0 = midi0;
                    lm->opacity->midi1 = midi1;
                    lm->opacity->midiport = midiport;
                    mainprogram->prevmodus = true;
                    lm->opacity->register_midi();
                    mainprogram->prevmodus = false;
                    lm->opacity->register_midi();
                    mainprogram->tmlearn = TM_NONE;
                    break;
                case TM_CROSS:
                    mainprogram->prevmodus = true;
                    mainmix->crossfade->midi[0] = midi0;
                    mainmix->crossfade->midi[1] = midi1;
                    mainmix->crossfade->midiport = midiport;
                    mainmix->crossfade->register_midi();
                    mainprogram->prevmodus = false;
                    mainmix->crossfadecomp->midi[0] = midi0;
                    mainmix->crossfadecomp->midi[1] = midi1;
                    mainmix->crossfadecomp->midiport = midiport;
                    mainmix->crossfadecomp->register_midi();
                    mainprogram->tmlearn = TM_NONE;
                    break;
                case TM_FREEZE:
                    if (midi0 == 176) return;
                    lm->scratchtouch->midi0 = midi0;
                    lm->scratchtouch->midi1 = midi1;
                    lm->scratchtouch->midiport = midiport;
                    mainprogram->prevmodus = true;
                    lm->scratchtouch->register_midi();
                    mainprogram->prevmodus = false;
                    lm->scratchtouch->register_midi();
                    mainprogram->tmlearn = TM_NONE;
                    break;
                case TM_SCRATCH:
                    if (midi0 == 144) return;
                    lm->scratch->midi0 = midi0;
                    lm->scratch->midi1 = midi1;
                    lm->scratch->midiport = midiport;
                    mainprogram->prevmodus = true;
                    lm->scratch->register_midi();
                    mainprogram->prevmodus = false;
                    lm->scratch->register_midi();
                    mainprogram->tmlearn = TM_NONE;
                    break;
            }
            mainprogram->prevmodus = bupm;
            return;
        }
	}
	
	
  	if (mainmix->learn) {
  		if (midi0 == 176 && mainmix->learnparam && mainmix->learnparam->sliding) {
            if (mainmix->learnparam->name == "shiftx" && (midi2 == 0.0f || midi2 == 127.0f)) return;
            if (mainmix->learnparam->name == "wipex" && (midi2 == 0.0f || midi2 == 127.0f)) return;
            if (mainmix->learnparam->name == "shifty" || mainmix->learnparam->name == "wipey") {
                for (int m = 0; m < 2; m++) {
                    if (mainmix->learnparam == mainmix->wipey[0]) {
                        if ((mainmix->wipex[0]->midi[1] == midi1 && mainmix->wipex[0]->midiport == midiport) ||
                            midi2 == 0.0f || midi2 == 127.0f) {
                            return;
                        }
                        mainmix->learnparam = mainmix->wipey[0];
                        mainmix->learn = true;
                        break;
                    }
                    if (mainmix->learnparam == mainmix->wipey[1]) {
                        if ((mainmix->wipex[1]->midi[1] == midi1 && mainmix->wipex[1]->midiport == midiport) ||
                            midi2 == 0.0f || midi2 == 127.0f) {
                            return;
                        }
                        mainmix->learnparam = mainmix->wipey[1];
                        mainmix->learn = true;
                        break;
                    }
                    std::vector<Layer *> &lvec = choose_layers(m);
                    for (int i = 0; i < lvec.size(); i++) {
                        if (mainmix->learnparam == lvec[i]->shifty) {
                            if ((lvec[i]->shiftx->midi[1] == midi1 && lvec[i]->shiftx->midiport == midiport) ||
                                midi2 == 0.0f || midi2 == 127.0f) {
                                return;
                            }
                            mainmix->learnparam = lvec[i]->shifty;
                            mainmix->learn = true;
                        }
                        if (mainmix->learnparam == lvec[i]->blendnode->wipey) {
                            if ((lvec[i]->blendnode->wipex->midi[1] == midi1 && lvec[i]->blendnode->wipex->midiport == midiport) ||
                                midi2 == 0.0f || midi2 == 127.0f) {
                                return;
                            }
                            mainmix->learnparam = lvec[i]->blendnode->wipey;
                            mainmix->learn = true;
                        }
                    }
                }
            }
  		    mainmix->learn = false;
			mainmix->learnparam->midi[0] = midi0;
			mainmix->learnparam->midi[1] = midi1;
			mainmix->learnparam->midiport = midiport;
			if (mainmix->learndouble) {
			    bool bupm = mainprogram->prevmodus;
                mainprogram->prevmodus = true;
                mainmix->learnparam->register_midi();
                mainprogram->prevmodus = false;
                mainmix->learnparam->register_midi();
                mainmix->learndouble = false;
			}
            else mainmix->learnparam->register_midi();
			mainmix->learnparam->node = new MidiNode;
			mainmix->learnparam->node->param = mainmix->learnparam;
			mainprogram->nodesmain->currpage->nodes.push_back(mainmix->learnparam->node);
			if (mainmix->learnparam->effect) {
				mainprogram->nodesmain->currpage->connect_in2(mainmix->learnparam->node, mainmix->learnparam->effect->node);
			}

            if (mainmix->learnparam->name == "shiftx" || mainmix->learnparam->name == "wipex") {
                for (int m = 0; m < 2; m++) {
                    if (mainmix->learnparam == mainmix->wipex[0]) {
                        mainmix->learnparam = mainmix->wipey[0];
                        mainmix->learn = true;
                        break;
                    }
                    if (mainmix->learnparam == mainmix->wipex[1]) {
                        mainmix->learnparam = mainmix->wipey[1];
                        mainmix->learn = true;
                        break;
                    }
                    std::vector<Layer *> &lvec = choose_layers(m);
                    for (int i = 0; i < lvec.size(); i++) {
                        if (mainmix->learnparam == lvec[i]->shiftx) {
                            mainmix->learnparam = lvec[i]->shifty;
                            mainmix->learn = true;
                        }
                        if (mainmix->learnparam == lvec[i]->blendnode->wipex) {
                            mainmix->learnparam = lvec[i]->blendnode->wipey;
                            mainmix->learn = true;
                        }
                    }
                }
            }
		}
		//else if (midi0 == 144 && mainmix->learnparam && !mainmix->learnparam->sliding) {
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
  		else if (midi0 == 176 && mainmix->learnbutton) {
            mainmix->learnbutton->midi[0] = midi0;
            mainmix->learnbutton->midi[1] = midi1;
            mainmix->learnbutton->midiport = midiport;
            if (mainmix->learndouble) {
                bool bupm = mainprogram->prevmodus;
                mainprogram->prevmodus = true;
                mainmix->learnbutton->register_midi();
                mainprogram->prevmodus = false;
                mainmix->learnbutton->register_midi();
                mainmix->learndouble = false;
            }
            else mainmix->learnbutton->register_midi();
            mainmix->learn = false;
        }

/* 			mainmix->learnbutton->node = new MidiNode;
			mainmix->learnbutton->node->button = mainmix->learnbutton;
			mainprogram->nodesmain->currpage->nodes.push_back(mainmix->learnbutton->node);
			if (mainmix->learnbutton->effect) {
				mainprogram->nodesmain->currpage->connect_in2(mainmix->learnbutton->node, mainmix->learnbutton->effect->node);
			}
IMPLEMENT */
		else if (midi0 == 144 && midi2 != 0 && mainmix->learnbutton) {
            mainmix->learnbutton->midi[0] = midi0;
            mainmix->learnbutton->midi[1] = midi1;
            mainmix->learnbutton->midiport = midiport;
            if (mainmix->learndouble) {
                bool bupm = mainprogram->prevmodus;
                mainprogram->prevmodus = true;
                mainmix->learnbutton->register_midi();
                mainprogram->prevmodus = false;
                mainmix->learnbutton->register_midi();
                mainmix->learndouble = false;
            }
            else mainmix->learnbutton->register_midi();
            mainmix->learn = false;
        }
        /* 			mainmix->learnparam->node = new MidiNode;
        mainmix->learnparam->node->param = mainmix->learnparam;
        mainprogram->nodesmain->currpage->nodes.push_back(mainmix->learnparam->node);
        if (mainmix->learnparam->effect) {
            mainprogram->nodesmain->currpage->connect_in2(mainmix->learnparam->node, mainmix->learnparam->effect->node);
        }\
        */

        mainmix->prevmidi0 = midi0;
        mainmix->prevmidi1 = midi1;

		return;
  	}
  	
  	
  	if (midi0 == 176 || midi0 == 144) {
		for (int i = 0; i < mainprogram->buttons.size(); i++) {
			Button *but = mainprogram->buttons[i];
			if (midi0 == but->midi[0] && midi1 == but->midi[1] && midi2 != 0 && midiport == but->midiport) {
                if (mainprogram->sameeight) {
                    for (int i = 0; i < 8; i++) {
                        if (but == loopstation->elems[i]->recbut) {
                            but = loopstation->elems[i + loopstation->scrpos]->recbut;
                            break;
                        }
                        if (but == loopstation->elems[i]->loopbut) {
                            but = loopstation->elems[i + loopstation->scrpos]->loopbut;
                            break;
                        }
                        if (but == loopstation->elems[i]->playbut) {
                            but = loopstation->elems[i + loopstation->scrpos]->playbut;
                            break;
                        }
                    }
                }
				mainmix->midi2 = midi2;
				mainmix->midibutton = but;
                but->midistarttime = std::chrono::high_resolution_clock::now();
			}
		}
		for (int m = 0; m < 2; m++) {
			for (int i = 0; i < 16; i++) {
				Button *but = mainprogram->shelves[m]->buttons[i];
				if (midi0 == but->midi[0] && midi1 == but->midi[1] && midi2 != 0 && midiport == but->midiport) {
                    if (mainprogram->shelves[m]->elements[i]->path != "") {
                        mainmix->midi2 = midi2;
                        mainmix->midibutton = nullptr;
                        mainmix->midishelfbutton = but;
                        but->midistarttime = std::chrono::high_resolution_clock::now();
                        mainmix->midibutton = nullptr;
                    }
				}
			}
		}

		for (Param *par : loopstation->allparams) {
            if (midi0 == par->midi[0] && midi1 == par->midi[1] && midiport == par->midiport) {
                if (mainprogram->sameeight) {
                    for (int i = 0; i < 8; i++) {
                        if (par == loopstation->elems[i]->speed) {
                            par = loopstation->elems[i + loopstation->scrpos]->speed;
                            break;
                        }
                    }
                }
                mainmix->midi2 = midi2;
                mainmix->midiparam = par;
                par->midistarttime = std::chrono::high_resolution_clock::now();
                par->midistarted = true;
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
					lay->rgbframe[lay->pbofri]->data,
					lay->rgbframe[lay->pbofri]->linesize
				);
				if (h < 1) {
				    return 0;
				}
				std::mutex datalock;
				datalock.lock();
				lay->decresult->hap = false;
                lay->remfr[lay->pbofri]->data = (char*)*(lay->rgbframe[lay->pbofri]->extended_data);
 				lay->decresult->height = lay->video_dec_ctx->height;
				lay->decresult->width = lay->video_dec_ctx->width;
				lay->decresult->size = lay->decresult->width * lay->decresult->height * 4;
				lay->remfr[lay->pbofri]->newdata = true;
				if (lay->clonesetnr != -1) {
					std::unordered_set<Layer*>::iterator it;
					for (it = mainmix->clonesets[lay->clonesetnr]->begin(); it != mainmix->clonesets[lay->clonesetnr]->end(); it++) {
						Layer* clay = *it;
						if (clay == lay) continue;
						clay->remfr[lay->pbofri]->newdata = true;
					}
				}
				datalock.unlock();
			}
		}
	}
    else if (lay->decpkt.stream_index == lay->audio_stream_idx) {
        return 0;
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


int find_stream_index(int *stream_idx,
                              AVFormatContext *video, enum AVMediaType type)
{
    int ret;
    AVDictionary *opts = nullptr;
    ret = av_find_best_stream(video, type, -1, -1, nullptr, 0);
    if (ret <0) {
    	printf("%s\n", " cant find stream");
    	return -1;
    }
	*stream_idx = ret;
    return ret;
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
		/*if (!lay->dummy && lay->volume->value > 0.0f) {
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
				//if ((lay->playbut->value && lay->speed->value < 0) || (lay->revbut->value && lay->speed->value > 0)) {
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
		}*/

		av_packet_unref(&lay->decpkt);
		av_read_frame(lay->video, &lay->decpkt);
	}
	return;
	if (snippet) {
		lay->chready = true;
		while (lay->chready) {
			lay->newchunk.notify_all();
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
		if (framenr != (int)this->startframe->value) {
			if (framenr != prevframe + 1) {
			    // hop to not-next-frame
				avformat_seek_file(this->videoseek, this->video_stream->index, this->video_stream->first_dts, seekTarget, seekTarget, 0);
				//avcodec_flush_buffers(this->video_dec_ctx);
				//int r = av_read_frame(this->videoseek, &this->decpktseek);
			}
			else {
				//avformat_seek_file(this->video, this->video_stream->index, 0, seekTarget, seekTarget, AVSEEK_FLAG_ANY);
                //avformat_seek_file(this->video, this->video_stream->index, this->video_stream->first_dts,
                 //                  seekTarget, seekTarget, 0);
			}
		}
		else {
			ret = avformat_seek_file(this->video, this->video_stream->index, seekTarget, seekTarget, this->video_duration + this->video_stream->first_dts, 0);
		}

		do {
			decode_audio(this);
		} while (this->decpkt.stream_index != this->video_stream_idx);
        if (framenr != prevframe + 1) {
            int r = av_read_frame(this->videoseek, &this->decpktseek);
            int readpos = ((this->decpktseek.dts - this->video_stream->first_dts) * this->numf) / this->video_duration;
            if (!this->keyframe) {
                if (readpos <= framenr) {
                    // readpos at keyframe after framenr
                    if (framenr > prevframe && prevframe > readpos) {
                        // starting from just past prevframe here is more efficient than decoding from readpos keyframe
                        readpos = prevframe + 1;
                    } else {
                        avformat_seek_file(this->video, this->video_stream->index, this->video_stream->first_dts,
                                           seekTarget, seekTarget, 0);
                    }
                    for (int f = readpos; f < framenr; f = f + mainprogram->qualfr) {
                        // decode sequentially frames starting from keyframe readpos to current framenr
                        ret = decode_packet(this, false);
                        do {
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
			if ((this->speed->value > 0 && (this->playbut->value || this->bouncebut->value == 1)) || (this->speed->value < 0 && (this->revbut->value || this->bouncebut->value == 2))) {
				framenr++;
			}
			else if ((this->speed->value > 0 && (this->revbut->value || this->bouncebut->value == 2)) || (this->speed->value < 0 && (this->playbut->value || this->bouncebut->value == 1))) {
				framenr--;
			}
			else if (this->prevfbw) {
				//this->prevfbw = false;
				framenr--;
			}
			else framenr++;	
			if (framenr > this->endframe->value) framenr = this->startframe->value;
			else if (framenr < this->startframe->value) framenr = this->endframe->value;
			//avcodec_flush_buffers(this->video_dec_ctx);
			errcount++;
			if (errcount == 1000) {
				mainprogram->openerr = true;
				return;
			}
            av_packet_unref(&this->decpkt);
			get_cpu_frame(framenr, this->prevframe, errcount);
			this->frame = framenr;
		}
		//decode_audio(this);
		av_packet_unref(&this->decpkt);
	}
	else {
		int r = av_read_frame(this->video, &this->decpkt);
		if (r < 0) printf("problem reading frame\n");
		else decode_packet(this, &got_frame);
		av_packet_unref(&this->decpkt);
	}
		
    
    if (this->audio_stream && 0) {
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

    av_free_packet(&this->decpkt);
    av_free_packet(&this->decpktseek);
}


bool Layer::get_hap_frame() {
    if (!this->video) return false;

	if (!this->databuf[0]) {
		//	this->decresult->width = 0;
        for (int k = 0; k < 3; k++) {
            this->databuf[k] = (char *) malloc(this->video_dec_ctx->width * this->video_dec_ctx->height * 4);
            if (this->databuf[k] == nullptr) {
                return 0;
            }
        }
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
	size_t uncomp = this->video_dec_ctx->width * this->video_dec_ctx->height * 4;
	int headerl = 4;
	if (*bptrData == 0 && *(bptrData + 1) == 0 && *(bptrData + 2) == 0) {
		headerl = 8;
	}
	this->decresult->compression = *(bptrData + 3);

	int st = -1;
	if (this->databufready) {
		st = snappy_uncompress(bptrData + headerl, size - headerl, this->databuf[this->pbofri], &uncomp);
	}
	av_packet_unref(&this->decpkt);

	if (!this->vidopen) {
		if (st != SNAPPY_OK) {
			this->decresult->width = 0;
			return false;
		}
		this->decresult->height = this->video_dec_ctx->height;
		this->decresult->width = this->video_dec_ctx->width;
		this->decresult->size = uncomp;
		this->decresult->hap = true;
		this->remfr[this->pbofri]->newdata = true;
		if (this->clonesetnr != -1) {
			std::unordered_set<Layer*>::iterator it;
			for (it = mainmix->clonesets[this->clonesetnr]->begin(); it != mainmix->clonesets[this->clonesetnr]->end(); it++) {
				Layer* clay = *it;
				if (clay == this) continue;
				clay->remfr[clay->pbofri]->newdata = true;
			}
		}
        this->remfr[this->pbofri]->data = this->databuf[this->pbofri];
	}
	return true;
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
            mainprogram->dellays.push_back(this);
            //delete this;  implement with layer delete vector?
            return;
        }

        if (this->vidopen) {
			if (!this->dummy && this->video) {
				std::unique_lock<std::mutex> lock(this->enddecodelock);
				this->enddecodevar.wait(lock, [&] {return this->processed; });
				this->processed = false;
				lock.unlock();
				avformat_close_input(&this->video);
				if (this->videoseek) avformat_close_input(&this->videoseek);
			}
			bool r = this->thread_vidopen();
			this->vidopen = false;
			if (this->isduplay) {
                this->decresult->width = 0;
                this->frame = this->isduplay->frame;
                this->isduplay = nullptr;
            }
			if (r == 0) {
				printf("load error\n");
				mainprogram->openerr = true;
				this->opened = true;
				if (!this->isclone) this->initialized = false;
				this->startframe->value = 0;
				this->endframe->value = 0;
                this->opened = true;
 			}
			else {
				this->opened = true;
			}
			if (mainprogram->autoplay && this->revbut->value == 0 && this->bouncebut->value == 0) {
			    this->playbut->value = 1;
			}
			continue;
		}
		if (!this->liveinput) {
			if (this->filename == "" || this->isclone) {
				continue;
			}
			if (this->vidformat != 188 && this->vidformat != 187) {
				if (this->video) {
					if ((int)(this->frame) != this->prevframe || this->type == ELEM_LIVE) {
						get_cpu_frame((int)this->frame, this->prevframe, 0);
					}
				}
				this->processed = true;
				continue;
			}
			if ((int)(this->frame) != this->prevframe) {
				bool ret = this->get_hap_frame();
				if (!ret) {	
					this->get_hap_frame();
				}
			}
		}
		this->processed = true;
		continue;
	}
}

void Layer::trigger() {
    while(!this->closethread) {
#ifdef POSIX
        sleep(0.1f);
#endif
#ifdef WINDOWS
        Sleep(100);
#endif
        this->endopenvar.notify_all();  // use return variable as trigger
        this->enddecodevar.notify_all();  // use return variable as trigger
    }
    while(this->closethread) {
        this->ready = true;
        this->startdecode.notify_all();
    }
}

void reset_par(Param *par, float val) {
// reset all speed settings
    LoopStationElement *elem = loopstation->parelemmap[par];
    if (elem) {
        if (!elem->eventlist.empty()) {
            for (int i = elem->eventlist.size() - 1; i >= 0; i--) {
                std::tuple<long long, Param *, Button *, float> event = elem->eventlist[i];
                if (std::get<1>(event) == par) {
                    elem->eventlist.erase(elem->eventlist.begin() + i);
                }
            }
        }
        loopstation->parelemmap[par]->params.erase(par);
        loopstation->parelemmap.erase(par);
    }
    par->value = val;
}

Layer* Layer::open_video(float frame, const std::string &filename, int reset, bool dontdeleffs) {
    // do configuration and thread launch for opening a video into a layer
    if (!isvideo(filename)) {
        this->filename = "";
        mainmix->addlay = false;
        return this;
    }

    this->databufready = false;

	if (!this->dummy) {
		ShelfElement* elem = this->prevshelfdragelem;
		bool ret = mainmix->set_prevshelfdragelem(this);
        if (!ret && elem->type == ELEM_DECK) {
            if (elem->launchtype == 2) {
                mainmix->mousedeck = elem->nblayers[0]->deck;
                mainmix->do_save_deck(mainprogram->temppath + "tempdeck_lnch.deck", false, false);
                mainmix->open_deck(mainprogram->temppath + "tempdeck_lnch.deck", false);
            }
        } else if (!ret && elem->type == ELEM_MIX) {
            if (elem->launchtype == 2) {
                mainmix->do_save_mix(mainprogram->temppath + "tempdeck_lnch.deck", mainprogram->prevmodus, false);
                mainmix->open_mix(mainprogram->temppath + "tempdeck_lnch.deck", false);
            }
        }
		else if (!ret && !this->encodeload) {
            Layer *lay = this->transfer();
            lay->open_video(0, filename, true);
			return lay;
		}
	}

	//this->video_dec_ctx = nullptr;
    if (this->boundimage != -1) {
        glDeleteTextures(1, &this->boundimage);
        this->boundimage = -1;
    }
	this->audioplaying = false;
    if (!this->keepeffbut->value && !dontdeleffs && !this->dummy) {
        Node *out = this->lasteffnode[0]->out[0];
        // remove effects if keep effects isnt on
        while (!this->effects[0].empty()) {
            // reminder : code duplication
            mainprogram->deleffects.push_back(this->effects[0].back());
            this->effects[0].pop_back();
        }
        this->lasteffnode[0] = this->node;
        if (this->pos == 0) {
            this->lasteffnode[1] = this->lasteffnode[0];
        }
        mainprogram->nodesmain->currpage->connect_nodes(this->lasteffnode[0], out);
    }
    if (!this->lockspeed) {
        reset_par(this->speed, 1.0f);
    }
    if (!this->lockzoompan) {
        reset_par(this->shiftx, 0.0f);
        reset_par(this->shifty, 0.0f);
        reset_par(this->scale, 1.0f);
    }
	if (this->effects[0].size() == 0) this->type = ELEM_FILE;
	else this->type = ELEM_LAYER;
	this->filename = filename;
	if (frame >= 0.0f) this->frame = frame;
	this->prevframe = this->frame - 1;
	this->reset = reset;
	this->skip = false;
	this->ifmt = nullptr;
    this->vidopen = true;
    this->remfr[0]->newdata = false;
    this->remfr[1]->newdata = false;
    this->remfr[2]->newdata = false;
    this->decresult->width = 0;
	this->decresult->compression = 0;
	this->ready = true;
	this->startdecode.notify_all();
	/*if (this->clonesetnr != -1) {
		mainmix->clonesets[this->clonesetnr]->erase(this);
		if (mainmix->clonesets[this->clonesetnr]->size() == 0) {
			mainmix->cloneset_destroy(mainmix->clonesets[this->clonesetnr]);
		}
		this->clonesetnr = -1;
	}*/
	return this;
}

bool Layer::thread_vidopen() {
    if (this->video_dec_ctx) avcodec_free_context(&this->video_dec_ctx);
    if (this->video) avformat_close_input(&this->video);
    if (this->videoseek) avformat_close_input(&this->videoseek);
	this->video = nullptr;
	this->video_dec_ctx = nullptr;
	this->liveinput = nullptr;
		
	if (!this->skip) {
		bool foundnew = false;
		ptrdiff_t pos = std::find(mainprogram->busylayers.begin(), mainprogram->busylayers.end(), this) - mainprogram->busylayers.begin();
		if (pos < mainprogram->busylayers.size()) {
			foundnew = this->find_new_live_base(pos);
			if (!foundnew && this->filename != mainprogram->busylist[pos]) {
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
	if (!this->ifmt) this->video->flags |= AVFMT_FLAG_NONBLOCK;
    if (this->ifmt) {
        this->type = ELEM_LIVE;
    }
    int r = avformat_open_input(&(this->video), this->filename.c_str(), this->ifmt, nullptr);
	printf("loading... %s\n", this->filename.c_str());
	if (r < 0) {
		this->filename = "";
		mainprogram->openerr = true;
		printf("%s\n", "Couldnt open video");
		return 0;
	}

	//av_opt_set_int(this->video, "max_analyze_duration2", 100000000, 0);
	if (avformat_find_stream_info(this->video, nullptr) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        mainprogram->openerr = true;
        return 0;
    }
	//this->video->max_picture_buffer = 20000000;

	if (find_stream_index(&(this->video_stream_idx), this->video, AVMEDIA_TYPE_VIDEO) >= 0) {
		this->video_stream = this->video->streams[this->video_stream_idx];
		const AVCodec* dec = avcodec_find_decoder(this->video_stream->codecpar->codec_id);
		this->vidformat = this->video_stream->codecpar->codec_id;
		this->video_dec_ctx = avcodec_alloc_context3(dec);
        if (this->video_stream->codecpar->codec_id == AV_CODEC_ID_MPEG2VIDEO || this->video_stream->codecpar->codec_id == AV_CODEC_ID_H264 || this->video_stream->codecpar->codec_id == AV_CODEC_ID_H264) {
            //this->video_dec_ctx->ticks_per_frame = 2;
        }
		avcodec_parameters_to_context(this->video_dec_ctx, this->video_stream->codecpar);
		avcodec_open2(this->video_dec_ctx, dec, nullptr);
		this->bpp = 4;
		if (this->vidformat == 188 || this->vidformat == 187) {
			if (oldvidformat != -1) {
                if (this->oldvidformat != 188 && this->oldvidformat != 187) {
                    // hap cpu change needs new texstorage
                    this->initialized = false;
                }
            }
			this->numf = this->video_stream->nb_frames;
			if (this->numf == 0) {
				this->numf = (double)this->video->duration * (double)this->video_stream->avg_frame_rate.num / (double)this->video_stream->avg_frame_rate.den / (double)1000000.0f;
				this->video_duration = this->video->duration / (1000000.0f * this->video_stream->time_base.num / this->video_stream->time_base.den);
			}
			else this->video_duration = this->video_stream->duration;
			float tbperframe = (float)this->video_stream->duration / (float)this->numf;
			this->millif = tbperframe * (((float)this->video_stream->time_base.num * 1000.0) / (float)this->video_stream->time_base.den);

			this->startframe->value = 0;
			this->endframe->value = this->numf;
			if (0) { // this->reset?
				this->frame = 0.0f;
			}
			this->decframe = av_frame_alloc();

			std::mutex flock;
			flock.lock();
			if (this->databuf[0]) {
                free(this->databuf[0]);
                free(this->databuf[1]);
                free(this->databuf[2]);
            }
            this->databuf[0] = nullptr;
            this->databuf[1] = nullptr;
            this->databuf[2] = nullptr;
			flock.unlock();

			return 1;
		}
		else if (this->type != ELEM_LIVE) {
            this->videoseek = avformat_alloc_context();
            this->videoseek->flags |= AVFMT_FLAG_NONBLOCK;
			avformat_open_input(&(this->videoseek), this->filename.c_str(), this->ifmt, nullptr);
		}
        if (oldvidformat != -1) {
            if (this->oldvidformat == 188 || this->oldvidformat == 187) {
                // hap cpu change needs new texstorage
                this->initialized = false;
            }
        }
	}
    else {
    	printf("Error2\n");
        mainprogram->openerr = true;
    	return 0;
    }

    for (int k = 0; k < 3; k++) {
        if (this->rgbframe[k]) {
            //avcodec_close(this->video_dec_ctx);
            //avcodec_close(this->audio_dec_ctx);
            av_freep(&this->rgbframe[k]->data[0]);
            av_frame_free(&this->rgbframe[k]);
        }
    }
    if (this->decframe) {
        //avcodec_close(this->video_dec_ctx);
        //avcodec_close(this->audio_dec_ctx);
        av_frame_free(&this->decframe);
    }

    if (this->type != ELEM_LIVE) {
		this->numf = this->video_stream->nb_frames;
		if (this->numf == 0) {
			this->numf = (double)this->video->duration * (double)this->video_stream->avg_frame_rate.num / (double)this->video_stream->avg_frame_rate.den / (double)1000000.0f;
			this->video_duration = this->video->duration / (1000000.0f * this->video_stream->time_base.num / this->video_stream->time_base.den);
		}
		else this->video_duration = this->video_stream->duration;
		if (this->reset) {
			this->startframe->value = 0;
			this->endframe->value = this->numf - 1;
		}
		float tbperframe = (float)this->video_duration / (float)this->numf;
		this->millif = tbperframe * (((float)this->video_stream->time_base.num * 1000.0) / (float)this->video_stream->time_base.den);
	}

	/*if (find_stream_index(&(this->audio_stream_idx), this->video, AVMEDIA_TYPE_AUDIO) >= 0 && !this->dummy) {
		this->audio_stream = this->video->streams[this->audio_stream_idx];
		const AVCodec *dec = avcodec_find_decoder(this->audio_stream->codecpar->codec_id);
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
    }*/

    for (int k = 0; k < 3; k++) {
        this->rgbframe[k] = av_frame_alloc();
        this->rgbframe[k]->format = AV_PIX_FMT_BGRA;
        this->rgbframe[k]->width = this->video_dec_ctx->width;
        this->rgbframe[k]->height = this->video_dec_ctx->height;
        int storage = av_image_alloc(this->rgbframe[k]->data, this->rgbframe[k]->linesize, this->rgbframe[k]->width,
                                     this->rgbframe[k]->height, AV_PIX_FMT_BGRA, 32);
        if (storage < 0) {
            mainprogram->openerr = true;
            return 0;
        }
    }

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
		
		while ((this->playbut->value || this->revbut->value) && this->snippets.size()) {
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
    alDeleteBuffers(128, &temp[0]);
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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glGenTextures(1, &this->oldtex);
	glBindTexture(GL_TEXTURE_2D, this->oldtex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	this->button = but;
	this->button->toggle = false;
	Box* box = this->button->box;
	box->vtxcoords->x1 = -1.0f + (pos % 4) * boxwidth + (2.0f - boxwidth * 4) * side;
	box->vtxcoords->h = boxwidth * (glob->w / glob->h) / (1920.0f / 1080.0f);
	box->vtxcoords->y1 = -1.0f + (int)(3 - (pos / 4)) * box->vtxcoords->h;
	box->vtxcoords->w = boxwidth;
	box->upvtxtoscr();
	box->tooltiptitle = "Video launch shelf";
	box->tooltip = "Shelf containing up to 16 videos/layerfiles for quick and easy video launching.  Left drag'n'drop from other areas, both videos and layerfiles.  Doubleclick left loads the shelf element contents into all selected layers. Rightclick launches shelf menu. ";
	this->sbox = new Box;
	this->sbox->vtxcoords->x1 = box->vtxcoords->x1;
	this->sbox->vtxcoords->y1 = box->vtxcoords->y1 + 0.05f + 0.009f;
	this->sbox->vtxcoords->w = 0.0075f;
	this->sbox->vtxcoords->h = 0.018f;
	this->sbox->upvtxtoscr();
	this->sbox->tooltiptitle = "Restart when triggered";
	this->sbox->tooltip = "When this video is put in the mix, either through MIDI or dragging, the video will restart from the beginning. ";
	this->pbox = new Box;
	this->pbox->vtxcoords->x1 = box->vtxcoords->x1;
	this->pbox->vtxcoords->y1 = box->vtxcoords->y1 + 0.05f - 0.009f;
	this->pbox->vtxcoords->w = 0.0075f;
	this->pbox->vtxcoords->h = 0.018f;
	this->pbox->upvtxtoscr();
	this->pbox->tooltiptitle = "Continue when triggered";
	this->pbox->tooltip = "When this video is put in the mix, either through MIDI or dragging, the video will continue from where it was last stopped . ";
	this->cbox = new Box;
	this->cbox->vtxcoords->x1 = box->vtxcoords->x1;
	this->cbox->vtxcoords->y1 = box->vtxcoords->y1 + 0.05f - 0.027f;
	this->cbox->vtxcoords->w = 0.0075f;
	this->cbox->vtxcoords->h = 0.018f;
	this->cbox->upvtxtoscr();
	this->cbox->tooltiptitle = "Catch up when triggered";
	this->cbox->tooltip = "When this video is put in the mix, either through MIDI or dragging, the video will continue from where it would have been if it had been virtually continuously playing . ";
}


void set_glstructures() {

	glGenTextures(1, &mainmix->mixbackuptex);
	glBindTexture(GL_TEXTURE_2D, mainmix->mixbackuptex);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, glob->w, glob->h);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	// tbo for quad color transfer to shader
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &mainprogram->maxtexes);
	glGenBuffers(1, &mainprogram->boxcoltbo);
	glBindBuffer(GL_TEXTURE_BUFFER, mainprogram->boxcoltbo);
	glBufferData(GL_TEXTURE_BUFFER, 2048 * 4, nullptr, GL_DYNAMIC_DRAW);
	glGenBuffers(1, &mainprogram->boxtextbo);
	glBindBuffer(GL_TEXTURE_BUFFER, mainprogram->boxtextbo);
	glBufferData(GL_TEXTURE_BUFFER, 2048, nullptr, GL_DYNAMIC_DRAW);
	GLint boxcolSampler = glGetUniformLocation(mainprogram->ShaderProgram, "boxcolSampler");
	glUniform1i(boxcolSampler, mainprogram->maxtexes - 2);
	glActiveTexture(GL_TEXTURE0 + mainprogram->maxtexes - 2);
	glGenTextures(1, &mainprogram->bdcoltex);
	glBindTexture(GL_TEXTURE_BUFFER, mainprogram->bdcoltex);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA8, mainprogram->boxcoltbo);
    glBindTexture(GL_TEXTURE_BUFFER, 0);
	GLint boxtexSampler = glGetUniformLocation(mainprogram->ShaderProgram, "boxtexSampler");
	glUniform1i(boxtexSampler, mainprogram->maxtexes - 1);
	glActiveTexture(GL_TEXTURE0 + mainprogram->maxtexes - 1);
	glGenTextures(1, &mainprogram->bdtextex);
	glBindTexture(GL_TEXTURE_BUFFER, mainprogram->bdtextex);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_R8UI, mainprogram->boxtextbo);
    glBindTexture(GL_TEXTURE_BUFFER, 0);


	glGenTextures(1, &mainprogram->fbotex[0]); //comp = false
    glGenTextures(1, &mainprogram->fbotex[1]);
   	glGenTextures(1, &mainprogram->fbotex[2]); //comp = true
    glGenTextures(1, &mainprogram->fbotex[3]);
    glBindTexture(GL_TEXTURE_2D, mainprogram->fbotex[0]);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, mainprogram->ow3, mainprogram->oh3);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glBindTexture(GL_TEXTURE_2D, mainprogram->fbotex[1]);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, mainprogram->ow3, mainprogram->oh3);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glBindTexture(GL_TEXTURE_2D, mainprogram->fbotex[2]);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, mainprogram->ow, mainprogram->oh);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glBindTexture(GL_TEXTURE_2D, mainprogram->fbotex[3]);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, mainprogram->ow, mainprogram->oh);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

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
	
	
	// record buffer
    glGenBuffers(1, &mainmix->ioBuf);

	glGenTextures(1, &binsmain->binelpreviewtex);
	glBindTexture(GL_TEXTURE_2D, binsmain->binelpreviewtex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

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

    SDL_GL_MakeCurrent(mainprogram->quitwindow, glc);
    glGenBuffers(1, &mainprogram->quboxvbuf);
    glGenBuffers(1, &mainprogram->quboxtbuf);
    glGenVertexArrays(1, &mainprogram->quboxvao);
    glBindVertexArray(mainprogram->quboxvao);
    glBindBuffer(GL_ARRAY_BUFFER, mainprogram->quboxvbuf);
    glBufferData(GL_ARRAY_BUFFER, 48, vcoords, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12, nullptr);
    glBindBuffer(GL_ARRAY_BUFFER, mainprogram->quboxtbuf);
    glBufferData(GL_ARRAY_BUFFER, 32, tcoords2, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
    SDL_GL_MakeCurrent(mainprogram->prefwindow, glc);
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
 	SDL_GL_MakeCurrent(mainprogram->config_midipresetswindow, glc);
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
	if (directdraw) {
        draw_line(gelem->line);
        delete gelem->line;
        delete gelem;
    }
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

void draw_direct(float* linec, float* areac, float x, float y, float wi, float he, float dx, float dy, float scale,
                 float opacity, int circle, GLuint tex, float smw, float smh, bool vertical, bool inverted) {
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
		if (1) {
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
			if (scale != 1.0f || dx != 0.0f || dy != 0.0f) {
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
        if (inverted) glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        if (inverted) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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

void draw_box(float *linec, float *areac, std::unique_ptr <Box> const &box, GLuint tex) {
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

void draw_box(float* linec, float* areac, float x, float y, float wi, float he, GLuint tex, bool text, bool vertical,
 bool inverted) {
	draw_box(linec, areac, x, y, wi, he, 0.0f, 0.0f, 1.0f, 1.0f, 0, tex, glob->w, glob->h, text, vertical, inverted);
}

void draw_box(float *color, float x, float y, float radius, int circle) {
	draw_box(nullptr, color, x - radius, y - radius, radius * 2.0f, (radius * 2.0f), 0.0f, 0.0f, 1.0f, 1.0f, circle, -1, glob->w, glob->h, 0);
}

void draw_box(float *color, float x, float y, float radius, int circle, float smw, float smh) {
	draw_box(nullptr, color, x - radius, y - radius, radius * 2.0f, (radius * 2.0f), 0.0f, 0.0f, 1.0f, 1.0f, circle, -1, smw, smh, 0);
}

void draw_box(float* linec, float* areac, float x, float y, float wi, float he, float dx, float dy, float scale, float opacity, int circle, GLuint tex, float smw, float smh, bool text) {
	draw_box(linec, areac, x, y, wi, he, dx, dy, scale, opacity, circle, tex, smw, smh, text, false, false);
}

void draw_box(float* linec, float* areac, float x, float y, float wi, float he, float dx, float dy, float scale,
              float opacity, int circle, GLuint tex, float smw, float smh, bool text, bool vertical, bool inverted) {
    if ((!mainprogram->startloop || mainprogram->directmode) || (!mainprogram->frontbatch && binsmain->inbin)) {
		GLuint textmode = glGetUniformLocation(mainprogram->ShaderProgram, "textmode");
		if (text && !circle) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glUniform1i(textmode, 1);
		}
		draw_direct(linec, areac, x, y, wi, he, dx, dy, scale, opacity, circle, tex, smw, smh, vertical, inverted);
		if (text && !circle) {
			glUniform1i(textmode, 0);
		}
		return;
	}

    if (circle || mainprogram->frontbatch || text) {
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
		box->inverted = inverted;
		GUI_Element *gelem = new GUI_Element;
		gelem->type = GUI_BOX;
		gelem->box = box;
		if (binsmain->inbin) mainprogram->binguielems.push_back(gelem);
		else mainprogram->guielems.push_back(gelem);
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

	// gather data for boxes drawn -> draw in batches
	if (tex != -1) mainprogram->countingtexes[mainprogram->currbatch]++;
	*mainprogram->bdtnptr[mainprogram->currbatch]++ = tex;
	
	if (text) *mainprogram->bdtptr[mainprogram->currbatch]++ = 24 + mainprogram->countingtexes[mainprogram->currbatch] - 1;
	else if (tex != -1) *mainprogram->bdtptr[mainprogram->currbatch]++ = mainprogram->countingtexes[mainprogram->currbatch] - 1;
	else *mainprogram->bdtptr[mainprogram->currbatch]++ = 23;

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

	if (tex != -1 && (scale != 1.0f || dx != 0.0f || dy != 0.0f)) {
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
		if (mainprogram->bdcolors[mainprogram->currbatch][0] < 0) {
		    printf("");
		}
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

	if (mainprogram->countingtexes[mainprogram->currbatch] == 23) {
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
	if (directdraw) {
        draw_triangle(gelem->triangle);
        delete gelem->triangle;
        delete gelem;
    }
    else if (binsmain->inbin) mainprogram->binguielems.push_back(gelem);
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
	//glBindBuffer(GL_ARRAY_BUFFER, fvbuf);
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
    if (binsmain->inbin) y -= 0.012f * (2070.0f / mainprogram->globh);
	else if (smflag == 0) y -= 0.012f * (2070.0f / glob->h);
	else if (smflag > 0) y -= 0.02f * (2070.0f / glob->h);
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
	    bool bufb = mainprogram->frontbatch;
        mainprogram->frontbatch = false;
		// compose string texture for display all next times
		const char *t = text.c_str();
		const char *p;

		int w2 = 0;
		int h2 = 0;
		if (binsmain->inbin) {
		    w2 = mainprogram->globw;
		    h2 = mainprogram->globh;
		}
		else if (smflag == 0) {
			w2 = glob->w;
			h2 = glob->h;
		}
		else {
			w2 = smw;
			h2 = smh;
		}
		int psize = (int)(sy * 24000.0f * h2 / 1346.0f);

        if (smflag == 1) SDL_GL_MakeCurrent(mainprogram->prefwindow, glc);
        else if (smflag == 2) SDL_GL_MakeCurrent(mainprogram->config_midipresetswindow, glc);

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

            GLenum err;
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
 			if (g->bitmap.width) glBlitNamedFramebuffer(glyphfrbuf, texfrbuf, 0, 0, g->bitmap.width, g->bitmap.rows, pxprogress, g->bitmap_top + 12, pxprogress + g->bitmap.width, g->bitmap_top - g->bitmap.rows + 12, GL_COLOR_BUFFER_BIT, GL_NEAREST);
            glDrawBuffer(GL_COLOR_ATTACHMENT0);
            glClearColor(0, 0, 0, 0);
            glClear(GL_COLOR_BUFFER_BIT);

            pxprogress += g->advance.x / 64.0f;

            x += (g->advance.x/64.0f) * pixelw;
			textw += (g->advance.x/64.0f) * pixelw;
			textws.push_back((g->advance.x / 64.0f) * pixelw * (((smflag == 0) && (binsmain->inbin == 0))+ 1) * 0.5f); //1.1 *
			texth = 64.0f * pixelh;
			th = (std::max)(th, (g->metrics.height / 64.0f) * pixelh);

            glDeleteFramebuffers(1, &glyphfrbuf);
		}

		//cropping texture
		int w = textw / pixelw; //2.2 *
		GLuint endtex;
		glGenTextures(1, &endtex);
		glBindTexture(GL_TEXTURE_2D, endtex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexStorage2D(
			GL_TEXTURE_2D,
			1,
			GL_R8,
			w,
			psize * 3
			);
		GLuint endfrbuf;
		glGenFramebuffers(1, &endfrbuf);
		glBindFramebuffer(GL_FRAMEBUFFER, endfrbuf);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, endtex, 0);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);
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

        if (smflag > 0) SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);

        mainprogram->frontbatch = bufb;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDrawBuffer(GL_BACK_LEFT);
        //display
		textws = render_text(text, textc, bux, buy, sx, sy, smflag, display, vertical);
		return textws;
  	}

	else if (display) {
		// display string texture after first time preparation - fast!
		int w2 = 0;
		int h2 = 0;
        if (binsmain->inbin) {
            w2 = mainprogram->globw;
            h2 = mainprogram->globh;
        }
        else if (smflag == 0) {
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
        //he /= (binsmain->inbin) + 1;
		y -= he;
		if (textw != 0) draw_box(nullptr, black, x + 0.001f, y - 0.00185f, wi, he, texture, true, vertical, false);
		//draw text shadow
		if (textw != 0) draw_box(nullptr, textc, x, y, wi, he, texture, true, vertical, false);	//draw text

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
	if (syncObj) {
        GLbitfield waitFlags = 0;
        GLuint64 waitDuration = 0;
        GLenum waitReturn = GL_UNSIGNALED;
        while (waitReturn != GL_ALREADY_SIGNALED && waitReturn != GL_CONDITION_SATISFIED)
        {
            waitReturn = glClientWaitSync(syncObj, waitFlags, waitDuration);
            waitFlags = GL_SYNC_FLUSH_COMMANDS_BIT;
            waitDuration = UINT64_MAX;
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

	if (mainprogram->tooltipbox && this->deck == 0 && this->pos == 0) {
		mainprogram->tooltipmilli += thismilli;
	}
	if (mainprogram->dragbinel && mainprogram->onscenebutton && this->deck == 0 && this->pos == 0) {
		mainprogram->onscenemilli += thismilli;
	}
	if (mainprogram->connfailed) mainprogram->connfailedmilli += thismilli;

	if (this->type != ELEM_LIVE) {
		if (!this->vidopen) {
			this->frame = this->frame + this->scratch;
			this->scratch = 0;
			// calculate new frame numbers
			float fac = 0.0f;
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
			if ((this->speed->value > 0 && (this->playbut->value || this->bouncebut->value == 1)) || (this->speed->value < 0 && (this->revbut->value || this->bouncebut->value == 2))) {
				this->frame += !this->scratchtouch * this->speed->value * fac * fac * this->speed->value * thismilli / this->millif;
			}
			else if ((this->speed->value > 0 && (this->revbut->value || this->bouncebut->value == 2)) || (this->speed->value < 0 && (this->playbut->value || this->bouncebut->value == 1))) {
				this->frame -= !this->scratchtouch * this->speed->value * fac * fac * this->speed->value * thismilli / this->millif;
			}
			if ((int)(this->frame) != this->prevframe && this->type == ELEM_IMAGE && this->numf > 0) {
				// set animated gif to update now
				this->remfr[this->pbofri]->newdata = true;
			}
            bool found = false;
            for (int i = 0; i < loopstation->elems.size(); i++) {
                if (std::find(loopstation->elems[i]->params.begin(), loopstation->elems[i]->params.end(), this->scritch) != loopstation->elems[i]->params.end()) {
                    found = true;
                    break;
                }
            }
            if (found) {
 			    this->frame = this->scritch->value;
			}

			// on video end (or video beginning if reverse playing) switch to the next clip in the queue
			if (this->oldalive || !alive) {
			    if (this->scritching == 0) {
                    if (this->bouncebut->value || this->playbut->value || this->revbut->value) {
                        if (this->frame > (this->endframe->value) && this->startframe->value != this->endframe->value) {
                            if (this->scritching != 4) {
                                if (this->bouncebut->value == 0) {
                                    if (this->lpbut->value == 0) {
                                        this->playbut->value = 0;
                                        this->onhold = true;
                                    }
                                    this->frame = this->startframe->value;
                                    if (mainmix->checkre) mainmix->rerun = true;
                                    this->clip_display_next(0, alive);
                                } else {
                                    this->frame = this->endframe->value - (this->frame - this->endframe->value);
                                    this->bouncebut->value = 2;
                                }
                            }
                        } else if (this->frame < this->startframe->value && this->startframe->value != this->endframe->value) {
                            if (this->scritching != 4) {
                                if (this->bouncebut->value == 0) {
                                    if (this->lpbut->value == 0) {
                                        this->revbut->value = 0;
                                        this->onhold = true;
                                    }
                                    if (mainmix->checkre) mainmix->rerun = true;
                                    this->frame = this->endframe->value;
                                    this->clip_display_next(1, alive);
                                } else {
                                    if (mainmix->checkre) mainmix->rerun = true;
                                    this->frame = this->startframe->value + (this->startframe->value - this->frame);
                                    this->bouncebut->value = 1;
                                }
                            }
                        }
                    }
                }
                if (this->scritching == 4) this->scritching = 0;
            }
			else {
				if (this->type == ELEM_FILE) {
					this->open_video(this->frame, this->filename, 0);
				}
				else if (this->type == ELEM_LAYER) {
					Layer *l = mainmix->open_layerfile(this->layerfilepath, this, 1, 0);
                    this->set_inlayer(l, true);
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
    if (this->filename == "") return;
    Layer *srclay = this;
    bool ret = false;
    //if (this->vidopen) return;
    if (this->liveinput || this->type == ELEM_IMAGE);
    else if (this->startframe->value != this->endframe->value || this->type == ELEM_LIVE) {
        if (mainmix->firstlayers.count(this->clonesetnr) == 0) {
            if (!this->remfr[this->pbofri]->newdata) {
                // promote first layer found in layer stack with this clonesetnr to element of firstlayers
                this->ready = true;
                if ((int) (this->frame) != this->prevframe) {
                    this->startdecode.notify_all();
                }
                if (this->clonesetnr != -1) {
                    mainmix->firstlayers[this->clonesetnr] = this;
                    this->isclone = false;
                }
            }
        } else {
            srclay = mainmix->firstlayers[this->clonesetnr];
            int sw, sh;
            glBindTexture(GL_TEXTURE_2D, srclay->texture);
            glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &sw);
            glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &sh);
            this->texture = srclay->texture;
            this->newtexdata = true;
            ret = true;
        }
    } else return;

    /*if (this->vidopen) {
        return;
    }*/

    // initialize layer?
    if (this->isduplay) return;
    if (!srclay->video_dec_ctx && srclay->type != ELEM_IMAGE) return;
    if (!srclay->mapptr[srclay->pbodi] || (!srclay->initialized && srclay->changeinit < 0) || srclay->changeinit  == 4) {
        // reminder : test with different bpp
        if (!this->liveinput) {
            if (srclay->video_dec_ctx) {
                if (srclay->vidformat == 188 || srclay->vidformat == 187) {
                    if (srclay->decresult->compression) {
                        float w = srclay->video_dec_ctx->width;
                        float h = srclay->video_dec_ctx->height;
                        this->initialize(w, h, srclay->decresult->compression);
                    }
                } else {
                    float w = srclay->video_dec_ctx->width;
                    float h = srclay->video_dec_ctx->height;
                    this->initialize(w, h);
                }
            } else return;
        } else {
            if (this->liveinput->video_dec_ctx) {
                float w = this->liveinput->video_dec_ctx->width;
                float h = this->liveinput->video_dec_ctx->height;
                this->initialize(w, h);
            }
        }
    }
    if (ret) return;

    // set frame data to texture
    if (this->liveinput) {  // mimiclayers trick/// :(
        this->texture = this->liveinput->texture;
    }

    if (this->isclone) return;


    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "OpenGL error3: " << err << std::endl;
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, srclay->texture);

    if (srclay->started2) {

       glBindBuffer(GL_PIXEL_UNPACK_BUFFER, srclay->pbo[srclay->pboui]);


        if ((!srclay->remfr[srclay->pboui]->liveinput && srclay->remfr[srclay->pboui]->initialized) || srclay->numf == 0) {
            if ((!srclay->remfr[srclay->pboui]->isclone)) {  // decresult contains new frame width, height, number of bitmaps && data
                //if (!srclay->decresult->width) {
                //    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
                //	return;
                //}
                if ((srclay->remfr[srclay->pboui]->vidformat == 188 || srclay->remfr[srclay->pboui]->vidformat == 187)) {
                    //if (!srclay->databufready) {
                    //    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
                    //	return;
                    //}
                    // HAP video layers

                    if (srclay->remfr[srclay->pboui]->compression == 187 ||
                        srclay->remfr[srclay->pboui]->compression == 171) {

                        glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, srclay->remfr[srclay->pboui]->width,
                                                  srclay->remfr[srclay->pboui]->height,
                                                  GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, srclay->remfr[srclay->pboui]->size, 0);
                    } else if (srclay->remfr[srclay->pboui]->compression == 190) {
                        glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, srclay->remfr[srclay->pboui]->width,
                                                  srclay->remfr[srclay->pboui]->height, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,
                                                  srclay->remfr[srclay->pboui]->size, 0);
                    }
                } else {
                    if (srclay->type == ELEM_IMAGE) {
                        int imageformat = ilGetInteger(IL_IMAGE_FORMAT);
                        int w = ilGetInteger(IL_IMAGE_WIDTH);
                        int h = ilGetInteger(IL_IMAGE_HEIGHT);
                        GLuint mode = GL_BGR;
                        if (imageformat == IL_RGBA) mode = GL_RGBA;
                        if (imageformat == IL_BGRA) mode = GL_BGRA;
                        if (imageformat == IL_RGB) mode = GL_RGB;
                        if (imageformat == IL_BGR) mode = GL_BGR;
                        if (srclay->numf == 0) {
                            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
                            glBindTexture(GL_TEXTURE_2D, srclay->texture);
                            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, mode, GL_UNSIGNED_BYTE,
                                            srclay->remfr[srclay->pbodi]->data);
                        } else {
                            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, mode, GL_UNSIGNED_BYTE, 0);
                        }
                    } else if (1) {
                        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, srclay->remfr[srclay->pboui]->width,
                                        srclay->remfr[srclay->pboui]->height, GL_BGRA, GL_UNSIGNED_BYTE, 0);
                    }
                }
            }
        }
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }

    if (srclay->type == ELEM_IMAGE) {
        ilBindImage(srclay->boundimage);
        ilActiveImage((int) srclay->frame);
        srclay->remfr[srclay->pbofri]->data = (char *) ilGetData();
        srclay->remfr[srclay->pbofri]->newdata = true;
        if (srclay->numf == 0) return;
    }
    if (!srclay->remfr[srclay->pbofri]->newdata) {
        return;
    }

    int w, h;
    if (srclay->type == ELEM_IMAGE) {
        w = ilGetInteger(IL_IMAGE_WIDTH);
        h = ilGetInteger(IL_IMAGE_HEIGHT);
    } else {
        w = srclay->video_dec_ctx->width;
        h = srclay->video_dec_ctx->height;
    };
    if (srclay->started2 && (srclay->changeinit == 3 || srclay->remfr[srclay->pbofri]->width != w ||
        srclay->remfr[srclay->pbofri]->height != h || srclay->remfr[srclay->pbofri]->bpp != srclay->bpp)) {
        // video (size) changed
        srclay->changeinit = srclay->pbodi;
    }
    if (!srclay->isclone && !srclay->liveinput) {
        // start transferring to pbou
        // bind PBO to update pixel values
        //glBindBuffer(GL_PIXEL_UNPACK_BUFFER, srclay->pbo[srclay->pboui]);
        if (srclay->mapptr[srclay->pbodi]) {
            // first check of new size content is loaded, if so, make new pbo for current write operation
            if (srclay->changeinit > -1) {
                printf("changepbo %d\n", srclay->pboui);
                glDeleteBuffers(1, &srclay->pbo[srclay->pboui]);
                glGenBuffers(1, &srclay->pbo[srclay->pboui]);
                int bsize = w * h * srclay->bpp;
                int flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
                glBindBuffer(GL_ARRAY_BUFFER, srclay->pbo[srclay->pboui]);
                glBufferStorage(GL_ARRAY_BUFFER, bsize, 0, flags);
                srclay->mapptr[srclay->pboui] = (GLubyte *) glMapBufferRange(GL_ARRAY_BUFFER, 0, bsize, flags);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
            }

            if (srclay->started) {
                if (mainprogram->encthreads == 0) WaitBuffer(srclay->syncobj[srclay->pbodi]);
                // update data directly on the mapped buffer
                memcpy(srclay->mapptr[srclay->pbodi], srclay->remfr[srclay->pbodi]->data, srclay->decresult->size);
                if (mainprogram->encthreads == 0) LockBuffer(srclay->syncobj[srclay->pbodi]);
            }
            if (srclay->started) {
                srclay->started2 = true;
            }
            if (srclay->started == false) {
                srclay->started = true;
            }
        }
    } else srclay->initialized = true;  //reminder: trick, is set false somewhere

	// round robin triple pbos: currently deactivated
    srclay->remfr[srclay->pbodi]->compression = srclay->decresult->compression;
    srclay->remfr[srclay->pbodi]->vidformat = srclay->vidformat;
    srclay->remfr[srclay->pbodi]->initialized = srclay->initialized;
    srclay->remfr[srclay->pbodi]->liveinput = srclay->liveinput;
    srclay->remfr[srclay->pbodi]->isclone = srclay->isclone;
    srclay->remfr[srclay->pbodi]->width = srclay->decresult->width;
    srclay->remfr[srclay->pbodi]->height = srclay->decresult->height;
    srclay->remfr[srclay->pbodi]->bpp = srclay->bpp;
    srclay->remfr[srclay->pbodi]->size = srclay->decresult->size;
    srclay->remfr[srclay->pbodi]->newdata = false;
    srclay->pbodi++;
    srclay->pboui++;
    srclay->pbofri++;
	if (srclay->pbodi == 3) srclay->pbodi = 0;
    if (srclay->pboui == 3) srclay->pboui = 0;
    if (srclay->pbofri == 3) srclay->pbofri = 0;
    if (srclay->changeinit == srclay->pbofri) srclay->changeinit = 4;


    srclay->newtexdata = true;
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
	glViewport(0, 0, glob->w, glob->h);
}

void midi_set() {
	if (mainprogram->midishelfelem) return;

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
                loopstation->elems[i]->add_button_automationentry(mainmix->midishelfbutton);
            }
        }

        mainmix->midishelfbutton = nullptr;
        mainmix->midibutton = nullptr;
    }

	else if (mainmix->midibutton) {
		Button *but = mainmix->midibutton;
		but->value++;
		if (but->value > but->toggle) but->value = 0;
		if (but->toggle == 0) but->value = 1;
		if (but == mainprogram->wormgate1 || but == mainprogram->wormgate2) mainprogram->binsscreen = but->value;
		
		for (int i = 0; i < loopstation->elems.size(); i++) {
			if (loopstation->elems[i]->recbut->value) {
				loopstation->elems[i]->add_button_automationentry(mainmix->midibutton);
			}
		}

		mainmix->midibutton = nullptr;
	}

	else if (mainmix->midiparam) {
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
				loopstation->elems[i]->add_param_automationentry(mainmix->midiparam);
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
	
	if (node->type == EFFECT) {
		Effect *effect = ((EffectNode*)node)->effect;

		if (1) {
		//if (effect->layer->initialized) {
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

                    case CHROMASTRETCH: {
                        fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
                        glUniform1i(fxid, CHROMASTRETCH);
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
                        bool swits = true;

                        fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
                        glUniform1i(fxid, EDGEDETECT);
                        GLint fboSampler = glGetUniformLocation(mainprogram->ShaderProgram, "fboSampler");
                        GLint interm = glGetUniformLocation(mainprogram->ShaderProgram, "interm");
                        glUniform1i(interm, 1);
                        glActiveTexture(GL_TEXTURE0);
                        glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->frbuf[swits + stage * 2]);
                        glDrawBuffer(GL_COLOR_ATTACHMENT0);
                        if (stage) glViewport(0, 0, mainprogram->ow, mainprogram->oh);
                        else glViewport(0, 0, mainprogram->ow3, mainprogram->oh3);
                        glBindTexture(GL_TEXTURE_2D, prevfbotex);
                        glBindVertexArray(mainprogram->fbovao);
                        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                        prevfbotex = mainprogram->fbotex[swits + stage * 2];
                        swits = !swits;

                        fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
                        glUniform1i(fxid, CUTOFF);
                        GLuint cut = glGetUniformLocation(mainprogram->ShaderProgram, "cut");
                        glUniform1f(cut, 0.9f);
                        glActiveTexture(GL_TEXTURE0);
                        glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->frbuf[swits + stage * 2]);
                        glDrawBuffer(GL_COLOR_ATTACHMENT0);
                        if (stage) glViewport(0, 0, mainprogram->ow, mainprogram->oh);
                        else glViewport(0, 0, mainprogram->ow3, mainprogram->oh3);
                        glBindTexture(GL_TEXTURE_2D, prevfbotex);
                        glBindVertexArray(mainprogram->fbovao);
                        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                        prevfbotex = mainprogram->fbotex[swits + stage * 2];
                        swits = !swits;

                        fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
                        glUniform1i(fxid, BLUR);
                        if (((EdgeDetectEffect*)effect)->thickness > 1) {
                            do_blur(stage, prevfbotex, (((EdgeDetectEffect *) effect)->thickness - 1));
                            prevfbotex = mainprogram->fbotex[swits + stage * 2];
                            swits = !swits;
                        }
                        glActiveTexture(GL_TEXTURE0);

                        if (((EdgeDetectEffect*)effect)->thickness > 1) {
                            fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
                            glUniform1i(fxid, GAMMA);
                            GLuint level = glGetUniformLocation(mainprogram->ShaderProgram, "gammaval");
                            glUniform1f(level, 3.0f);
                            glActiveTexture(GL_TEXTURE0);
                            glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->frbuf[swits + stage * 2]);
                            glDrawBuffer(GL_COLOR_ATTACHMENT0);
                            if (stage) glViewport(0, 0, mainprogram->ow, mainprogram->oh);
                            else glViewport(0, 0, mainprogram->ow3, mainprogram->oh3);
                            glBindTexture(GL_TEXTURE_2D, prevfbotex);
                            glBindVertexArray(mainprogram->fbovao);
                            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                            prevfbotex = mainprogram->fbotex[swits + stage * 2];
                            swits = !swits;

                            fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
                            glUniform1i(fxid, INVERT);
                            glActiveTexture(GL_TEXTURE0);
                            glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->frbuf[swits + stage * 2]);
                            glDrawBuffer(GL_COLOR_ATTACHMENT0);
                            if (stage) glViewport(0, 0, mainprogram->ow, mainprogram->oh);
                            else glViewport(0, 0, mainprogram->ow3, mainprogram->oh3);
                            glBindTexture(GL_TEXTURE_2D, prevfbotex);
                            glBindVertexArray(mainprogram->fbovao);
                            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                            prevfbotex = mainprogram->fbotex[swits + stage * 2];
                            swits = !swits;

                            glUniform1i(interm, 0);

                            GLuint mm = glGetUniformLocation(mainprogram->ShaderProgram, "mixmode");
                            glUniform1i(mm, 0);
                            GLuint edgethickmode = glGetUniformLocation(mainprogram->ShaderProgram, "edgethickmode");
                            glUniform1i(edgethickmode, 1);
                            glUniform1i(fboSampler, 0);
                            glActiveTexture(GL_TEXTURE0);
                            glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->frbuf[swits + stage * 2]);
                            glDrawBuffer(GL_COLOR_ATTACHMENT0);
                            if (stage) glViewport(0, 0, mainprogram->ow, mainprogram->oh);
                            else glViewport(0, 0, mainprogram->ow3, mainprogram->oh3);
                            glBindTexture(GL_TEXTURE_2D, prevfbotex);
                            glBindVertexArray(mainprogram->fbovao);
                            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                            prevfbotex = mainprogram->fbotex[swits + stage * 2];
                            swits = !swits;

                            glUniform1i(interm, 1);
                            glUniform1i(fboSampler, 0);
                            glUniform1i(edgethickmode, 0);
                            fxid = glGetUniformLocation(mainprogram->ShaderProgram, "fxid");
                            glUniform1i(fxid, INVERT);
                            glActiveTexture(GL_TEXTURE0);
                            glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->frbuf[swits + stage * 2]);
                            glDrawBuffer(GL_COLOR_ATTACHMENT0);
                            if (stage) glViewport(0, 0, mainprogram->ow, mainprogram->oh);
                            else glViewport(0, 0, mainprogram->ow3, mainprogram->oh3);
                            glBindTexture(GL_TEXTURE_2D, prevfbotex);
                            glBindVertexArray(mainprogram->fbovao);
                            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                            prevfbotex = mainprogram->fbotex[swits + stage * 2];
                            glUniform1i(interm, 0);
                        }

                        glUniform1i(interm, 0);
                        glViewport(0, 0, glob->w, glob->h);
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
                if (stage == 0) {

                    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, mainprogram->ow3, mainprogram->oh3);
                }
                else {
                    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, mainprogram->ow, mainprogram->oh);
                }
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

                glGenFramebuffers(1, &(effect->fbo));
                glBindFramebuffer(GL_FRAMEBUFFER, effect->fbo);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, effect->fbotex, 0);
			}
			GLuint down = glGetUniformLocation(mainprogram->ShaderProgram, "down");
            if (effect->type == EDGEDETECT) glUniform1i(down, 1);
            else if (effect->onoffbutton->value) glUniform1i(interm, 1);
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

			Layer *lay = effect->layer;
			float sx, sy, sc, op;
			if (effect->node == lay->lasteffnode[0]) {
                sx = lay->shiftx->value;
                sy = lay->shifty->value;
                sc = lay->scale->value;
                op = lay->opacity->value;
           }
			else {
			    sx = 0.0f;
                sy = 0.0f;
                sc = 1.0f;
                op = 1.0f;
			}

			if (mainmix->waitmixtex == 0 && !lay->onhold) draw_box(nullptr, black, -1.0f, 1.0f, 2.0f, -2.0f, sx, sy, sc, op, 0, prevfbotex, 0, 0, false);
            prevfbotex = effect->fbotex;
            prevfbo = effect->fbo;

            glUniform1i(interm, 0);
            glUniform1i(down, 0);
			glViewport(0, 0, glob->w, glob->h);
		}
	}
	else if (node->type == VIDEO) {
	    Layer *lay = ((VideoNode*)node)->layer;
	    if (lay->vidopen) {
	        mainmix->domix = false;  // freezes output frames during vidopen
            return;
        }
 		/*if (!lay->newtexdata && mainmix->bulrs[lay->deck].size()) {
            //mainmix->domix = false;  // freezes output frames during vidopen
			return;
		}*/
		else {
			/*for (int m = 0; m < 2; m++) {
				if (mainmix->bulrs[m].size()) {
					bool notyet = false;
					std::vector<Layer*>& lvec = choose_layers(m);
					for (int i = 0; i < lvec.size(); i++) {
						if (lvec[i]->filename == "" || lvec[i]->type == ELEM_LIVE) continue;
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
						for (int i = 0; i < mainmix->butexes[m].size(); i++) {
							glDeleteTextures(1, &mainmix->butexes[m][i]);
						}
					}
				}
			}*/
			if (lay->blendnode) {
				if (lay->blendnode->blendtype == 19 || lay->blendnode->blendtype == 20 || lay->blendnode->blendtype
				== 21) {
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
			else {
			    if (lay->remfr[lay->pboui]->height == 0) return;
			    frac = lay->remfr[lay->pboui]->width / lay->remfr[lay->pboui]->height;
			}
			if (lay->dummy) frac = lay->video_dec_ctx->width / lay->video_dec_ctx->height;
			if (fraco > frachd) {
				ys = 0.0f;
				sciy = ys;
			}
			else {
				xs = 0.0f;
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
			glViewport(sxs, sys, scw - sxs * 2.0f, sch - sys * 2.0f);
			float sx, sy, sc, op;
            if (lay->node == lay->lasteffnode[0]) {
                sx = lay->shiftx->value;
                sy = lay->shifty->value;
                sc = lay->scale->value;
                op = lay->opacity->value;
            }
            else {
                sx = 0.0f;
                sy = 0.0f;
                sc = 1.0f;
                op = 1.0f;
            }
            glBindFramebuffer(GL_FRAMEBUFFER, lay->fbo);
            glDrawBuffer(GL_COLOR_ATTACHMENT0);
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            if (mainmix->waitmixtex == 0 && !lay->onhold) draw_box(nullptr, black, -1.0f, 1.0f, 2.0f, -2.0f, sx, sy, sc, op, 0, lay->texture, 0, 0, false);
            prevfbotex = lay->fbotex;
            prevfbo = lay->fbo;

			glViewport(0, 0, glob->w, glob->h);
			lay->newtexdata = false;
		}
		if (!lay->dummy && lay->pos == 0) lay->blendnode->fbotex = lay->fbotex;
	}
	else if (node->type == BLEND) {
		BlendNode *bnode = (BlendNode*)node;
		if (bnode->in == prevnode) {
			bnode->intex = prevfbotex;
		}
		if (bnode->in2 == prevnode) {
			bnode->in2tex = prevfbotex;
		}
		if (bnode->in && bnode->in2) {
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
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

                glGenFramebuffers(1, &(bnode->fbo));
                glBindFramebuffer(GL_FRAMEBUFFER, bnode->fbo);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bnode->fbotex, 0);
            }

            if (bnode->intex != -1 && bnode->in2tex != -1) {
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
				if (bnode->blendtype == 19 || bnode->blendtype == 20 || bnode->blendtype == 21) {
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
	    if (!mainmix->domix) {
	        return;
	    }
		MixNode *mnode = (MixNode*)node;
		if (mnode->mixfbo == -1) {
		    mnode->newmixfbo = false;
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
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

			glGenFramebuffers(1, &(mnode->mixfbo));
			glBindFramebuffer(GL_FRAMEBUFFER, mnode->mixfbo);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mnode->mixtex, 0);
		}
		glBindFramebuffer(GL_FRAMEBUFFER, mnode->mixfbo);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		if (stage) glViewport(0, 0, mainprogram->ow, mainprogram->oh);
		else glViewport(0, 0, mainprogram->ow3, mainprogram->oh3);
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        if (mainmix->waitmixtex == 0) draw_box(nullptr, black, -1.0f, 1.0f, 2.0f, -2.0f, prevfbotex);
		prevfbotex = mnode->mixtex;
		prevfbo = mnode->mixfbo;
		glViewport(0, 0, glob->w, glob->h);
	}
	for (int i = 0; i < node->out.size(); i++) {
		if (node->out[i]->calc && !node->out[i]->walked) onestepfrom(stage, node->out[i], node, prevfbotex, prevfbo);
	}
}

void walk_back(Node* node) {
	while (node->in && !node->calc) {
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
		    // reset all nodes
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

	mainprogram->directmode = true;
	if (stage == 0) {
		int mutes = 0;
		bool muted = false;
        mainmix->domix = true;
		for (int i = 0; i < mainmix->layersA.size(); i++) {
            Layer *lay = mainmix->layersA[i];
            //if (!lay->liveinput && !lay->decresult->width && lay->filename != "") {
            //    mainprogram->directmode = false;
            //    return;
            //}
			if (lay->mutebut->value) mutes++;
			walk_forward(lay->node);
			onestepfrom(0, lay->node, nullptr, -1, -1);
		}
		if (mutes == mainmix->layersA.size()) {
			muted = true;
			glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->nodesmain->mixnodes[0]->mixfbo);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			glClearColor(0, 0, 0, 0);
			glClear(GL_COLOR_BUFFER_BIT);
		}
		mutes = 0;
        mainmix->domix = true;
		for (int i = 0; i < mainmix->layersB.size(); i++) {
			Layer* lay = mainmix->layersB[i];
            //if (!lay->liveinput && !lay->decresult->width && lay->filename != "") {
            //    mainprogram->directmode = false;
            //    return;
            //}
			if (lay->mutebut->value) mutes++;
			walk_forward(lay->node);
			onestepfrom(0, lay->node, nullptr, -1, -1);
		}
		if (mutes == mainmix->layersB.size()) {
			muted = true;
			glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->nodesmain->mixnodes[1]->mixfbo);
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
        mainmix->domix = true;
		for (int i = 0; i < mainmix->layersAcomp.size(); i++) {
			Layer* lay = mainmix->layersAcomp[i];
            //if (!lay->liveinput && !lay->decresult->width && lay->filename != "") {
            //    mainprogram->directmode = false;
            //    return;
            //}
			if (lay->mutebut->value) mutes++;
			walk_forward(lay->node);
			onestepfrom(1, lay->node, nullptr, -1, -1);
		}
		if (mutes == mainmix->layersAcomp.size()) {
			muted = true;
			glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->nodesmain->mixnodescomp[0]->mixfbo);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			glClearColor(0, 0, 0, 0);
			glClear(GL_COLOR_BUFFER_BIT);
		}
		mutes = 0;
        mainmix->domix = true;
		for (int i = 0; i < mainmix->layersBcomp.size(); i++) {
			Layer* lay = mainmix->layersBcomp[i];
            //if (!lay->liveinput && !lay->decresult->width && lay->filename != "") {
            //    mainprogram->directmode = false;
            //    return;
            //}
			if (lay->mutebut->value) mutes++;
			walk_forward(lay->node);
			onestepfrom(1, lay->node, nullptr, -1, -1);
		}
		if (mutes == mainmix->layersBcomp.size()) {
			muted = true;
			glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->nodesmain->mixnodescomp[1]->mixfbo);
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

    if (mainmix->waitmixtex) mainmix->waitmixtex++;
    if (mainmix->waitmixtex > 1) mainmix->waitmixtex = 0;
    mainmix->waitmixtex = 0;
	mainprogram->directmode = false;
}
		

bool display_mix() {
    mainprogram->directmode = true;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDrawBuffer(GL_BACK_LEFT);

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
        box->vtxcoords->x1 = -0.15f;
        box->vtxcoords->y1 = -1.0f;
        box->vtxcoords->w = 0.3f;
        box->vtxcoords->h = mainprogram->monh;
        box->upvtxtoscr();
		draw_box(red, black, box->vtxcoords->x1 + xs * 2.0f, box->vtxcoords->y1 + ys * 2.0f, box->vtxcoords->w - xs * 4.0f, box->vtxcoords->h - ys * 4.0f, node->mixtex);
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
		draw_box(red, black, box->vtxcoords->x1 + xs, box->vtxcoords->y1 + ys, box->vtxcoords->w - xs * 2.0f, box->vtxcoords->h - ys * 2.0f, node->mixtex);
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
        box->vtxcoords->x1 = -0.3f;
        box->vtxcoords->y1 = -1.0f;
        box->vtxcoords->w = 0.6f;
        box->vtxcoords->h = mainprogram->monh * 2.0f;
        box->upvtxtoscr();
		draw_box(red, black, box->vtxcoords->x1 + xs * 2.0f, box->vtxcoords->y1 + ys * 2.0f, box->vtxcoords->w - xs * 4.0f, box->vtxcoords->h - ys * 4.0f, node->mixtex);
		mainprogram->mainmonitor->in();
	}
	glUniform1i(wipe, 0);
	glUniform1i(mixmode, 0);

	if (mainprogram->prevmodus) {
		node = (MixNode*)mainprogram->nodesmain->mixnodes[0];
		box = mainprogram->deckmonitor[0];
		draw_box(red, box->acolor, box->vtxcoords->x1 + xs, box->vtxcoords->y1 + ys, box->vtxcoords->w - xs * 2.0f, box->vtxcoords->h - ys * 2.0f, node->mixtex);
		box->in();
		node = (MixNode*)mainprogram->nodesmain->mixnodes[1];
		box = mainprogram->deckmonitor[1];
		draw_box(red, black, box->vtxcoords->x1 + xs, box->vtxcoords->y1 + ys, box->vtxcoords->w - xs * 2.0f, box->vtxcoords->h - ys * 2.0f, node->mixtex);
        box->in();
	}
	else {
		node = (MixNode*)mainprogram->nodesmain->mixnodescomp[0];
		box = mainprogram->deckmonitor[0];
		draw_box(red, black, box->vtxcoords->x1 + xs, box->vtxcoords->y1 + ys, box->vtxcoords->w - xs * 2.0f, box->vtxcoords->h - ys * 2.0f, node->mixtex);
        box->in();
		node = (MixNode*)mainprogram->nodesmain->mixnodescomp[1];
		box = mainprogram->deckmonitor[1];
		draw_box(red, black, box->vtxcoords->x1 + xs, box->vtxcoords->y1 + ys, box->vtxcoords->w - xs * 2.0f, box->vtxcoords->h - ys * 2.0f, node->mixtex);
        box->in();
	}

    mainprogram->directmode = false;

	return true;
}


void drag_into_layerstack(std::vector<Layer*>& layers, bool deck) {
	Layer* lay;
	for (int i = 0; i < layers.size(); i++) {
		lay = layers[i];
		if (mainprogram->draginscrollbarlay) {
			lay = mainprogram->draginscrollbarlay;
		}
		else if (lay->pos < mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos || lay->pos > mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos + 2) continue;
		Box* box = lay->node->vidbox;
		int endx = false;
		if ((i == layers.size() - 1 || i == mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos + 2)) {
			endx = true;
		}

		if (!mainmix->moving) {
			bool no = false;
			if (mainprogram->dragbinel) {
				if (mainprogram->dragbinel->type == ELEM_DECK || mainprogram->dragbinel->type == ELEM_MIX) {
					no = true;
				}
			}
			if (!no) {
				bool cond1 = (box->scrcoords->y1 < mainprogram->my + box->scrcoords->h && mainprogram->my < box->scrcoords->y1);
				bool cond2 = (box->scrcoords->x1 + box->scrcoords->w * 0.25f < mainprogram->mx && mainprogram->mx < box->scrcoords->x1 + box->scrcoords->w * 0.75f);
				if (mainprogram->draginscrollbarlay || (cond1 && cond2)) {
					mainprogram->draginscrollbarlay = nullptr;
					// handle dragging things into layer monitors of deck
					lay->queueing = true;
					mainprogram->queueing = true;
					mainmix->currlay[!mainprogram->prevmodus] = lay;
					if (mainprogram->lmover || mainprogram->middlemouse || mainprogram->laymenu1->state > 1 || mainprogram->laymenu2->state > 1 || mainprogram->newlaymenu->state > 1) {
						set_queueing(false);
						if (mainprogram->dragbinel) {
							mainprogram->laymenu1->state = 0;
							mainprogram->laymenu2->state = 0;
							mainprogram->newlaymenu->state = 0;
                            int pos = std::find(mainmix->currlays[!mainprogram->prevmodus].begin(), mainmix->currlays[!mainprogram->prevmodus].end(), lay) - mainmix->currlays[!mainprogram->prevmodus].begin();
                            if (pos == mainmix->currlays[!mainprogram->prevmodus].size()) {
                                pos = -1;
                            }
                            mainmix->open_dragbinel(lay, pos);
						}
						mainprogram->rightmouse = true;
						binsmain->handle(0);
						enddrag(false);
                        mainprogram->rightmouse = false;
						return;
					}
				}
				else if (cond1 && ((endx && mainprogram->mx > deck * (glob->w / 2.0f) && mainprogram->mx < (deck + 1) * (glob->w / 2.0f)) || (box->scrcoords->x1 - mainprogram->xvtxtoscr(0.075f) < mainprogram->mx && mainprogram->mx < box->scrcoords->x1 + mainprogram->xvtxtoscr(0.075f)))) {
					// handle dragging things to the end of the last visible layer monitor of deck
					if (mainprogram->lmover) {
						Layer* inlay = mainmix->add_layer(layers, lay->pos + endx);
						if (inlay->pos == mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos + 3) mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos++;
						if (mainprogram->dragbinel) {
							if (mainprogram->dragbinel->type == ELEM_LAYER) {
								Layer *l = mainmix->open_layerfile(mainprogram->dragbinel->path, inlay, 1, 1);
                                inlay->set_inlayer(l, false);
							}
							else {
								inlay->open_video(0, mainprogram->dragbinel->path, true);
							}
						}
						mainprogram->lmover = false;
						mainprogram->rightmouse = true;
						binsmain->handle(0);
						enddrag(false);
                        mainprogram->rightmouse = false;
						return;
					}
				}

				int numonscreen = layers.size() - mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos;
				if (0 <= numonscreen && numonscreen <= 2) {
				    if (mainprogram->xvtxtoscr(mainprogram->numw + deck * 1.0f + numonscreen * mainprogram->layw) < mainprogram->mx && mainprogram->mx > deck * (glob->w / 2.0f) && mainprogram->mx < (deck + 1) * (glob->w / 2.0f)) {
						if (0 < mainprogram->my && mainprogram->my < mainprogram->yvtxtoscr(mainprogram->layh)) {
							float blue[] = { 0.5, 0.5 , 1.0, 1.0 };
							draw_box(nullptr, blue, -1.0f + mainprogram->numw + deck * 1.0f + numonscreen * mainprogram->layw, 1.0f - mainprogram->layh, mainprogram->layw, mainprogram->layh, -1);
							if (mainprogram->lmover) {
								// handle draggingndropping into black space to the right of deck layer monitors
								lay = mainmix->add_layer(layers, layers.size());
                                lay->keepeffbut->value = 0;
                                if (mainprogram->dragbinel) {
									if (mainprogram->dragbinel->type == ELEM_LAYER) {
										Layer *l = mainmix->open_layerfile(mainprogram->dragbinel->path, lay, 1, 1);
                                        lay->set_inlayer(l, false);
                                        //lay->initialize(lay->video_dec_ctx->width, lay->video_dec_ctx->height);
									}
									else if (mainprogram->dragbinel->type == ELEM_FILE) {
										lay->open_video(0, mainprogram->dragbinel->path, true);
									}
									else if (mainprogram->dragbinel->type == ELEM_IMAGE) {
										lay->open_image(mainprogram->dragbinel->path);
									}
								}
								mainprogram->rightmouse = true;
								binsmain->handle(0);
								enddrag(false);
                                mainprogram->rightmouse = false;
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
void Program::handle_shelf(Shelf *shelf) {
	// draw shelves and handle shelves
	int inelem = -1;
	for (int i = 0; i < 16; i++) {
		ShelfElement* elem = shelf->elements[i];
		// border coloring according to element type
		if (elem->type == ELEM_LAYER) {
			draw_box(orange, orange, shelf->buttons[i]->box, -1);
			draw_box(nullptr, orange, shelf->buttons[i]->box->vtxcoords->x1 + 0.0075f, shelf->buttons[i]->box->vtxcoords->y1, shelf->buttons[i]->box->vtxcoords->w - 0.0075f, shelf->buttons[i]->box->vtxcoords->h - 0.0075f, elem->tex);
		}
		else if (elem->type == ELEM_DECK) {
			draw_box(purple, purple, shelf->buttons[i]->box, -1);
			draw_box(nullptr, purple, shelf->buttons[i]->box->vtxcoords->x1 + 0.0075f, shelf->buttons[i]->box->vtxcoords->y1, shelf->buttons[i]->box->vtxcoords->w - 0.0075f, shelf->buttons[i]->box->vtxcoords->h - 0.0075f, elem->tex);
		}
		else if (elem->type == ELEM_MIX) {
			draw_box(green, green, shelf->buttons[i]->box, -1);
			draw_box(nullptr, green, shelf->buttons[i]->box->vtxcoords->x1 + 0.0075f, shelf->buttons[i]->box->vtxcoords->y1, shelf->buttons[i]->box->vtxcoords->w - 0.0075f, shelf->buttons[i]->box->vtxcoords->h - 0.0075f, elem->tex);
		}
		else if (elem->type == ELEM_IMAGE) {
			draw_box(pink, pink, shelf->buttons[i]->box, -1);
			draw_box(nullptr, pink, shelf->buttons[i]->box->vtxcoords->x1 + 0.0075f, shelf->buttons[i]->box->vtxcoords->y1, shelf->buttons[i]->box->vtxcoords->w - 0.0075f, shelf->buttons[i]->box->vtxcoords->h - 0.0075f, elem->tex);
		}
		else {
			draw_box(grey, grey, shelf->buttons[i]->box, -1);
			draw_box(nullptr, grey, shelf->buttons[i]->box->vtxcoords->x1 + 0.0075f, shelf->buttons[i]->box->vtxcoords->y1, shelf->buttons[i]->box->vtxcoords->w - 0.0075f, shelf->buttons[i]->box->vtxcoords->h - 0.0075f, elem->tex);
		}

		// draw small icons for choice of launch play type of shelf video
		if (elem->launchtype == 0) {
			draw_box(nullptr, yellow, elem->sbox, -1);
		}
		else {
			draw_box(white, black, elem->sbox, -1);
		}
		if (elem->launchtype == 1) {
			draw_box(nullptr, red, elem->pbox, -1);
		}
		else {
			draw_box(white, black, elem->pbox, -1);
		}
		if (elem->launchtype == 2) {
			draw_box(nullptr, darkblue, elem->cbox, -1);
		}
		else {
			draw_box(white, black, elem->cbox, -1);
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
		    // mouse over shelf element
			inelem = i;
            if (mainprogram->doubleleftmouse) {
                // doubleclick loads elem in currlay layer slots
                mainprogram->shelfdragelem = elem;
                mainprogram->shelfdragnum = i;
                mainprogram->doubleleftmouse = false;
                mainprogram->doublemiddlemouse = false;
                //mainprogram->lmover = false;
                mainprogram->leftmouse = false;
                mainprogram->leftmousedown = false;
                mainprogram->middlemouse = false;
                mainprogram->middlemousedown = false;
                mainprogram->dragbinel = new BinElement;
                mainprogram->dragbinel->path = elem->path;
                mainprogram->dragbinel->type = elem->type;
                mainprogram->dragbinel->tex = elem->tex;
                for (int i = 0; i < mainmix->currlays[!mainprogram->prevmodus].size(); i++) {
                    mainmix->open_dragbinel(mainmix->currlays[!mainprogram->prevmodus][i], i);
                }
                enddrag(false);
            }
			if (mainprogram->rightmouse) {
				if (mainprogram->dragbinel) {
					if (mainprogram->shelfdragelem) {
						if (shelf->prevnum != -1) std::swap(shelf->elements[shelf->prevnum]->tex, shelf->elements[shelf->prevnum]->oldtex);
						std::swap(elem->tex, mainprogram->shelfdragelem->tex);
						mainprogram->shelfdragelem->tex = mainprogram->dragbinel->tex;
						mainprogram->shelfdragelem = nullptr;
						enddrag(false);
						//mainprogram->menuactivation = false;
					}
				}
			}
			shelf->newnum = i;
			mainprogram->inshelf = shelf->side;
			if (mainprogram->dragbinel && i != shelf->prevnum) {
				std::swap(elem->tex, elem->oldtex);
				if (shelf->prevnum != -1) std::swap(shelf->elements[shelf->prevnum]->tex, shelf->elements[shelf->prevnum]->oldtex);
				if (mainprogram->shelfdragelem) {
					std::swap(mainprogram->shelfdragelem->tex, mainprogram->shelfdragelem->oldtex);
					mainprogram->shelfdragelem->tex = elem->oldtex;
				}
				elem->tex = mainprogram->dragbinel->tex;
				shelf->prevnum = i;
			}
			if (mainprogram->leftmousedown) {
				if (!elem->sbox->in() && !elem->pbox->in() && !elem->cbox->in()) {
					if (!mainprogram->dragbinel) {
						// user starts dragging shelf element
						if (elem->path != "") {
                            mainprogram->shelfmx = mainprogram->mx;
                            mainprogram->shelfmy = mainprogram->my;
							mainprogram->shelfdragelem = elem;
							mainprogram->shelfdragnum = i;
							mainprogram->leftmousedown = false;
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
				if (mainprogram->mx != mainprogram->shelfmx || mainprogram->my != mainprogram->shelfmy) {
                    if (mainprogram->dragbinel) {
//                  if (mainprogram->dragbinel && mainprogram->shelfdragelem) {
                        if (elem != mainprogram->shelfdragelem) {
                            std::string newpath;
                            std::string extstr = "";
                            if (mainprogram->dragbinel->type == ELEM_LAYER) {
                                extstr = ".layer";
                            } else if (mainprogram->dragbinel->type == ELEM_DECK) {
                                extstr = ".deck";
                            } else if (mainprogram->dragbinel->type == ELEM_MIX) {
                                extstr = ".mix";
                            }
                            if (extstr != "") {
                                std::string base = basename(mainprogram->dragbinel->path);
                                newpath = find_unused_filename("shelf_" + base,
                                                               mainprogram->project->shelfdir + shelf->basepath + "/",
                                                               extstr);
                                boost::filesystem::copy_file(mainprogram->dragbinel->path, newpath);
                                mainprogram->dragbinel->path = newpath;
                            }
                            if (mainprogram->shelfdragelem) {
                                std::swap(elem->path, mainprogram->shelfdragelem->path);
                                std::swap(elem->jpegpath, mainprogram->shelfdragelem->jpegpath);
                                std::swap(elem->type, mainprogram->shelfdragelem->type);
                            } else {
                                elem->path = mainprogram->dragbinel->path;
                                elem->type = mainprogram->dragbinel->type;
                            }
                            elem->tex = copy_tex(mainprogram->dragbinel->tex);
                            elem->jpegpath = find_unused_filename(basename(elem->path), mainprogram->temppath, ".jpg");
                            save_thumb(elem->jpegpath, elem->tex);
                            if (mainprogram->shelfdragelem) mainprogram->shelfdragelem->tex = copy_tex(mainprogram->shelfdragelem->tex);
                            blacken(elem->oldtex);
                            shelf->prevnum = -1;
                            mainprogram->shelfdragelem = nullptr;
                            mainprogram->rightmouse = true;
                            binsmain->handle(0);
                            enddrag(false);
                            mainprogram->rightmouse = false;

                            return;
                        }
                    }
                }
			}
			if (mainprogram->menuactivation) {
				mainprogram->shelfmenu->state = 2;
				mainmix->mouseshelf = shelf;
				mainmix->mouseshelfelem = i;
				mainprogram->menuactivation = false;
			}
		}
	}
	if (inelem > -1 && mainprogram->dragbinel) {
		mainprogram->dragout[shelf->side] = false;
		mainprogram->dragout[!shelf->side] = true;
	}
	if (inelem == -1 && !mainprogram->dragout[shelf->side] && mainprogram->dragbinel) {
		// mouse not over shelf element
		mainprogram->dragout[shelf->side] = true;
		if (shelf->prevnum != -1) {
			std::swap(shelf->elements[shelf->prevnum]->tex, shelf->elements[shelf->prevnum]->oldtex);
			if (mainprogram->shelfdragelem) {
                if (mainprogram->shelfdragnum == shelf->prevnum) {
                    std::swap(mainprogram->shelfdragelem->tex, mainprogram->shelfdragelem->oldtex);
                }
                else {
                    mainprogram->shelfdragelem->tex = mainprogram->dragbinel->tex;
                }
            }
		}
		shelf->prevnum = -1;
	}
	else if (!mainprogram->dragout[shelf->side]) shelf->prevnum = shelf->newnum;
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
                    float pixelw = 2.0f / glob->w;
                    float pixelh = 2.0f / glob->h;
					int lpos = vnode->layer->pos - mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos;
					if (lpos < 0 || lpos > 2) vnode->vidbox->vtxcoords->x1 = 2.0f;
					else vnode->vidbox->vtxcoords->x1 = -1.0f + (float)(lpos % 3) * mainprogram->layw + mainprogram->numw + xoffset + pixelw;
					vnode->vidbox->vtxcoords->y1 = 1.0f - mainprogram->layh + pixelh;
					vnode->vidbox->vtxcoords->w = mainprogram->layw - 2.0f * pixelw;
					vnode->vidbox->vtxcoords->h = mainprogram->layh - 2.0f * pixelh;
					vnode->vidbox->upvtxtoscr();
					
					vnode->vidbox->lcolor[0] = 0.0;
					vnode->vidbox->lcolor[1] = 0.0;
					vnode->vidbox->lcolor[2] = 0.0;
					vnode->vidbox->lcolor[3] = 1.0;
					if (vnode->layer->effects[0].size() == 0) {
						vnode->vidbox->tex = vnode->layer->fbotex;
					}
					else {
						vnode->vidbox->tex = vnode->layer->effects[0][vnode->layer->effects[0].size() - 1]->fbotex;
					}
                    if (mainprogram->effcat[vnode->layer->deck]->value == 1) {
                        if ((vnode)->layer == mainmix->currlay[!mainprogram->prevmodus]) {
                            if ((vnode)->layer->effects[1].size() == 0) {
                                vnode->vidbox->tex = vnode->layer->blendnode->fbotex;
                            } else {
                                vnode->vidbox->tex = vnode->layer->effects[1][vnode->layer->effects[1].size() -
                                                                              1]->fbotex;
                            }
                        }
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
                        float pixelw = 2.0f / glob->w;
                        float pixelh = 2.0f / glob->h;
						int lpos = vnode->layer->pos - mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos;
						if (lpos < 0 || lpos > 2) vnode->vidbox->vtxcoords->x1 = 2.0f;
						else vnode->vidbox->vtxcoords->x1 = -1.0f + (float)(lpos % 3) * mainprogram->layw + mainprogram->numw + xoffset + pixelw;
						vnode->vidbox->vtxcoords->y1 = 1.0f - mainprogram->layh + pixelh;
						vnode->vidbox->vtxcoords->w = mainprogram->layw - 2.0f * pixelw;
						vnode->vidbox->vtxcoords->h = mainprogram->layh - 2.0f * pixelh;
						vnode->vidbox->upvtxtoscr();

						vnode->vidbox->lcolor[0] = 0.0;
						vnode->vidbox->lcolor[1] = 0.0;
						vnode->vidbox->lcolor[2] = 0.0;
						vnode->vidbox->lcolor[3] = 1.0;
						if ((vnode)->layer->effects[0].size() == 0) {
							vnode->vidbox->tex = vnode->layer->fbotex;
						}
						else {
							vnode->vidbox->tex = vnode->layer->effects[0][vnode->layer->effects[0].size() - 1]->fbotex;
						}
                        if (vnode->layer == mainmix->currlay[!mainprogram->prevmodus] && mainprogram->effcat[vnode->layer->deck]->value == 1) {
                            if ((vnode)->layer->effects[1].size() == 0) {
                                vnode->vidbox->tex = vnode->layer->blendnode->fbotex;
                            }
                            else {
                                vnode->vidbox->tex = vnode->layer->effects[1][vnode->layer->effects[1].size() - 1]->fbotex;
                            }
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
                testlay->mixbox->lcolor[0] = 0.7;
                testlay->mixbox->lcolor[1] = 0.7;
                testlay->mixbox->lcolor[2] = 0.7;
                testlay->mixbox->lcolor[3] = 1.0;
                int sp = mainmix->scenes[testlay->deck][mainmix->currscene[testlay->deck]]->scrollpos;
                if (testlay->pos - sp == 2 && testlay->deck == 1) xoffset -= 0.1f;
				testlay->mixbox->vtxcoords->x1 = -1.0f + mainprogram->numw + xoffset + ((testlay->pos - sp) % 3) * mainprogram->layw;
				testlay->mixbox->vtxcoords->y1 = 1.0f - mainprogram->layh - 0.135f;
				testlay->mixbox->vtxcoords->w = 0.075f;
				testlay->mixbox->vtxcoords->h = 0.075f;
				testlay->mixbox->upvtxtoscr();
                testlay->colorbox->lcolor[0] = 0.7;
                testlay->colorbox->lcolor[1] = 0.7;
                testlay->colorbox->lcolor[2] = 0.7;
                testlay->colorbox->lcolor[3] = 1.0;
				testlay->colorbox->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + 0.105f;
				testlay->colorbox->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1;
				testlay->colorbox->vtxcoords->w = 0.075f;
				testlay->colorbox->vtxcoords->h = 0.075f;
				testlay->colorbox->upvtxtoscr();
		
				// shift effectboxes and parameterboxes according to position in stack and scrollposition of stack
				std::vector<Effect*> &evec = testlay->choose_effects();
				Effect *prevmodus = nullptr;
				for (int j = 0; j < evec.size(); j++) {
					Effect *eff = evec[j];
					eff->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + 0.075f;
					eff->onoffbutton->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + 0.0375f;
					eff->drywet->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1;
					float dy;
					if (prevmodus) {
						if (prevmodus->params.size() < 4) {
							eff->box->vtxcoords->y1 = prevmodus->box->vtxcoords->y1 - 0.075f;
						}
						else if (prevmodus->params.size() > 5) {
							eff->box->vtxcoords->y1 = prevmodus->box->vtxcoords->y1 - 0.225f;
						}
						else {
							eff->box->vtxcoords->y1 = prevmodus->box->vtxcoords->y1 - 0.15f;
						}
					}
					else {
						eff->box->vtxcoords->y1 = 1.0 - mainprogram->layh - 0.435f + (0.075f *
						        testlay->effscroll[mainprogram->effcat[testlay->deck]->value]);
					}
					eff->box->upvtxtoscr();
					eff->onoffbutton->box->vtxcoords->y1 = eff->box->vtxcoords->y1;
					eff->onoffbutton->box->upvtxtoscr();
					eff->drywet->box->vtxcoords->y1 = eff->box->vtxcoords->y1;
					eff->drywet->box->upvtxtoscr();
					prevmodus = eff;
				}
				
				// Make GUI box of mixing factor slider
                testlay->blendnode->box->lcolor[0] = 0.7;
                testlay->blendnode->box->lcolor[1] = 0.7;
                testlay->blendnode->box->lcolor[2] = 0.7;
                testlay->blendnode->box->lcolor[3] = 1.0;
				testlay->blendnode->mixfac->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + 0.12f;
				testlay->blendnode->mixfac->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1;
				testlay->blendnode->mixfac->box->vtxcoords->w = mainprogram->layw * 1.5f * 0.25f;
				testlay->blendnode->mixfac->box->vtxcoords->h = 0.075f;
				testlay->blendnode->mixfac->box->upvtxtoscr();
				
				// Make GUI box of video speed slider
                testlay->speed->box->lcolor[0] = 0.7;
                testlay->speed->box->lcolor[1] = 0.7;
                testlay->speed->box->lcolor[2] = 0.7;
                testlay->speed->box->lcolor[3] = 1.0;
				testlay->speed->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1;
				testlay->speed->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - 0.075f;
				testlay->speed->box->vtxcoords->w = mainprogram->layw * 1.5f * 0.5f;
				testlay->speed->box->vtxcoords->h = 0.075f;
				testlay->speed->box->upvtxtoscr();
				
				// GUI box of layer opacity slider
                testlay->opacity->box->lcolor[0] = 0.7;
                testlay->opacity->box->lcolor[1] = 0.7;
                testlay->opacity->box->lcolor[2] = 0.7;
                testlay->opacity->box->lcolor[3] = 1.0;
				testlay->opacity->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1;
				testlay->opacity->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - 0.225f;
				testlay->opacity->box->vtxcoords->w = mainprogram->layw * 1.5f * 0.25f;
				testlay->opacity->box->vtxcoords->h = 0.075f;
				testlay->opacity->box->upvtxtoscr();
				
				// GUI box of layer audio volume slider
				testlay->volume->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + mainprogram->layw * 1.5f * 0.30f;
				testlay->volume->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - 0.225f;
				testlay->volume->box->vtxcoords->w = mainprogram->layw * 1.5f * 0.25f;
				testlay->volume->box->vtxcoords->h = 0.075f;
				testlay->volume->box->upvtxtoscr();
				
				// GUI box of play video button
                testlay->playbut->box->lcolor[0] = 0.7;
                testlay->playbut->box->lcolor[1] = 0.7;
                testlay->playbut->box->lcolor[2] = 0.7;
                testlay->playbut->box->lcolor[3] = 1.0;
				testlay->playbut->box->acolor[0] = 0.5;
				testlay->playbut->box->acolor[1] = 0.2;
				testlay->playbut->box->acolor[2] = 0.5;
				testlay->playbut->box->acolor[3] = 1.0;
				testlay->playbut->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + mainprogram->layw * 1.5f * 0.5f
				        + 0.132f;
				testlay->playbut->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - 0.075f;
				testlay->playbut->box->vtxcoords->w = 0.0375f;
				testlay->playbut->box->vtxcoords->h = 0.075f;
				testlay->playbut->box->upvtxtoscr();
				
				// GUI box of bounce play video button
                testlay->bouncebut->box->lcolor[0] = 0.7;
                testlay->bouncebut->box->lcolor[1] = 0.7;
                testlay->bouncebut->box->lcolor[2] = 0.7;
                testlay->bouncebut->box->lcolor[3] = 1.0;
				testlay->bouncebut->box->acolor[0] = 0.5;
				testlay->bouncebut->box->acolor[1] = 0.2;
				testlay->bouncebut->box->acolor[2] = 0.5;
				testlay->bouncebut->box->acolor[3] = 1.0;
				testlay->bouncebut->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + mainprogram->layw * 1.5f * 0.5f + 0.087f;
				testlay->bouncebut->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - 0.075f;
				testlay->bouncebut->box->vtxcoords->w = 0.045f;
				testlay->bouncebut->box->vtxcoords->h = 0.075f;
				testlay->bouncebut->box->upvtxtoscr();
				
				// GUI box of reverse play video button
                testlay->revbut->box->lcolor[0] = 0.7;
                testlay->revbut->box->lcolor[1] = 0.7;
                testlay->revbut->box->lcolor[2] = 0.7;
                testlay->revbut->box->lcolor[3] = 1.0;
				testlay->revbut->box->acolor[0] = 0.5;
				testlay->revbut->box->acolor[1] = 0.2;
				testlay->revbut->box->acolor[2] = 0.5;
				testlay->revbut->box->acolor[3] = 1.0;
				testlay->revbut->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + mainprogram->layw * 1.5f * 0.5f + 0.0495f;
				testlay->revbut->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - 0.075f;
				testlay->revbut->box->vtxcoords->w = 0.0375f;
				testlay->revbut->box->vtxcoords->h = 0.075f;
				testlay->revbut->box->upvtxtoscr();
				
				// GUI box of video frame forward button
                testlay->frameforward->box->lcolor[0] = 0.7;
                testlay->frameforward->box->lcolor[1] = 0.7;
                testlay->frameforward->box->lcolor[2] = 0.7;
                testlay->frameforward->box->lcolor[3] = 1.0;
				testlay->frameforward->box->acolor[0] = 0.3;
				testlay->frameforward->box->acolor[1] = 0.3;
				testlay->frameforward->box->acolor[2] = 0.6;
				testlay->frameforward->box->acolor[3] = 1.0;
				testlay->frameforward->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + mainprogram->layw * 1.5f *
				        0.5f + 0.1695f;
				testlay->frameforward->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - 0.075f;
				testlay->frameforward->box->vtxcoords->w = 0.0375f;
				testlay->frameforward->box->vtxcoords->h = 0.075f;
				testlay->frameforward->box->upvtxtoscr();
				
				// GUI box of video frame backward button
                testlay->framebackward->box->lcolor[0] = 0.7;
                testlay->framebackward->box->lcolor[1] = 0.7;
                testlay->framebackward->box->lcolor[2] = 0.7;
                testlay->framebackward->box->lcolor[3] = 1.0;
				testlay->framebackward->box->acolor[0] = 0.3;
				testlay->framebackward->box->acolor[1] = 0.3;
				testlay->framebackward->box->acolor[2] = 0.3;
				testlay->framebackward->box->acolor[3] = 1.0;
				testlay->framebackward->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + mainprogram->layw * 1.5f *
				        0.5f + 0.012f;
				testlay->framebackward->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - 0.075f;
				testlay->framebackward->box->vtxcoords->w = 0.0375f;
				testlay->framebackward->box->vtxcoords->h = 0.075f;
				testlay->framebackward->box->upvtxtoscr();

                // GUI box of reverse play video button
                testlay->lpbut->box->lcolor[0] = 0.7;
                testlay->lpbut->box->lcolor[1] = 0.7;
                testlay->lpbut->box->lcolor[2] = 0.7;
                testlay->lpbut->box->lcolor[3] = 1.0;
                testlay->lpbut->box->acolor[0] = 0.5;
                testlay->lpbut->box->acolor[1] = 0.2;
                testlay->lpbut->box->acolor[2] = 0.5;
                testlay->lpbut->box->acolor[3] = 1.0;
                testlay->lpbut->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + mainprogram->layw * 1.5f * 0.5f +
                                                     0.2445f;
                testlay->lpbut->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - 0.075f;
                testlay->lpbut->box->vtxcoords->w = 0.0375f;
                testlay->lpbut->box->vtxcoords->h = 0.075f;
                testlay->lpbut->box->upvtxtoscr();

                // GUI box of reverse play video button
                testlay->stopbut->box->lcolor[0] = 0.7;
                testlay->stopbut->box->lcolor[1] = 0.7;
                testlay->stopbut->box->lcolor[2] = 0.7;
                testlay->stopbut->box->lcolor[3] = 1.0;
                testlay->stopbut->box->acolor[0] = 0.3;
                testlay->stopbut->box->acolor[1] = 0.3;
                testlay->stopbut->box->acolor[2] = 0.3;
                testlay->stopbut->box->acolor[3] = 1.0;
                testlay->stopbut->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + mainprogram->layw * 1.5f * 0.5f +
                                                     0.207f;
                testlay->stopbut->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - 0.075f;
                testlay->stopbut->box->vtxcoords->w = 0.0375f;
                testlay->stopbut->box->vtxcoords->h = 0.075f;
                testlay->stopbut->box->upvtxtoscr();

                // GUI box of specific general midi set for layer switch
                testlay->genmidibut->box->lcolor[0] = 0.7;
                testlay->genmidibut->box->lcolor[1] = 0.7;
                testlay->genmidibut->box->lcolor[2] = 0.7;
                testlay->genmidibut->box->lcolor[3] = 1.0;
                testlay->genmidibut->box->acolor[0] = 0.5;
                testlay->genmidibut->box->acolor[1] = 0.2;
                testlay->genmidibut->box->acolor[2] = 0.5;
                testlay->genmidibut->box->acolor[3] = 1.0;
				testlay->genmidibut->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + mainprogram->layw * 1.5f *
				        0.5f + 0.282f;
				testlay->genmidibut->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - 0.075f;
				testlay->genmidibut->box->vtxcoords->w = 0.0375f;
				testlay->genmidibut->box->vtxcoords->h = 0.075f;
				testlay->genmidibut->box->upvtxtoscr();
				
				// GUI box of scratch video box
                testlay->loopbox->lcolor[0] = 0.7;
                testlay->loopbox->lcolor[1] = 0.7;
                testlay->loopbox->lcolor[2] = 0.7;
                testlay->loopbox->lcolor[3] = 1.0;
                bool found = false;
                for (int i = 0; i < loopstation->elems.size(); i++) {
                    if (std::find(loopstation->elems[i]->params.begin(), loopstation->elems[i]->params.end(), testlay->scritch) != loopstation->elems[i]->params.end()) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    testlay->loopbox->acolor[0] = 0.3;
                    testlay->loopbox->acolor[1] = 0.3;
                    testlay->loopbox->acolor[2] = 0.3;
                    testlay->loopbox->acolor[3] = 1.0;
                }
				testlay->loopbox->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1;
				testlay->loopbox->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1 - 0.15f;
				testlay->loopbox->vtxcoords->w = mainprogram->layw * 1.5f;
				testlay->loopbox->vtxcoords->h = 0.075f;
				testlay->loopbox->upvtxtoscr();
				
				if (mainprogram->nodesmain->linked) {
					if (testlay->pos > 0) {
						testlay->node->box->scrcoords->x1 = testlay->blendnode->box->scrcoords->x1 -
						        mainprogram->xvtxtoscr(0.225f);
					}
					else {
						testlay->node->box->scrcoords->x1 = mainprogram->xvtxtoscr(0.132f);
					}
					testlay->node->box->upscrtovtx();
				}
				
				// GUI box of colourkey direction switch
                testlay->chdir->box->lcolor[0] = 0.7;
                testlay->chdir->box->lcolor[1] = 0.7;
                testlay->chdir->box->lcolor[2] = 0.7;
                testlay->chdir->box->lcolor[3] = 1.0;
				testlay->chdir->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + 0.36f;
				testlay->chdir->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1;
				testlay->chdir->box->vtxcoords->w = 0.0375f;
				testlay->chdir->box->vtxcoords->h = 0.075f;
				testlay->chdir->box->upvtxtoscr();
				
				// GUI box of colourkey inversion switch
                testlay->chinv->box->lcolor[0] = 0.7;
                testlay->chinv->box->lcolor[1] = 0.7;
                testlay->chinv->box->lcolor[2] = 0.7;
                testlay->chinv->box->lcolor[3] = 1.0;
				testlay->chinv->box->vtxcoords->x1 = testlay->mixbox->vtxcoords->x1 + 0.4275f;
				testlay->chinv->box->vtxcoords->y1 = testlay->mixbox->vtxcoords->y1;
				testlay->chinv->box->vtxcoords->w = 0.0375f;
				testlay->chinv->box->vtxcoords->h = 0.075f;
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


Clip::Clip() {
	this->box = new Box;
	this->box->tooltiptitle = "Clip queue ";
	this->box->tooltip = "Clip queue: clips (videos, images, layer files, live feeds) loaded here are played in order after the current clip.  Rightclick menu allows loading live feed / opening content into clip / deleting clip.  Clips can be dragged anywhere and anything can be dragged into or inserted between them. ";
	this->tex = -1;
    this->startframe = new Param;
    this->startframe->value = -1;
    this->endframe = new Param;
    this->endframe->value = -1;
}

Clip::~Clip() {
	glDeleteTextures(1, &this->tex);
	if (this->path.rfind(".layer") != std::string::npos) {
		if (this->path.find("cliptemp_") != std::string::npos) {
			boost::filesystem::remove(this->path);			
		}
	}
}

Clip* Clip::copy() {
    Clip *clip = new Clip;
    clip->path = this->path;
    clip->type = this->type;
    clip->tex = copy_tex(this->tex);
    clip->frame = this->frame;
    clip->startframe->value = this->startframe->value;
    clip->endframe->value = this->endframe->value;
    clip->box = this->box->copy();
    return clip;
}

void Clip::insert(Layer* lay, std::vector<Clip*>::iterator it) {
    lay->clips.insert(it, this);
    this->layer = lay;
}

bool get_imagetex(Layer *lay, const std::string& path) {
	lay->dummy = 1;
	lay->open_image(path, false);
	if (lay->filename == "") {
        return false;
    }
    lay->processed = true;
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
	this->startframe->value = 0;
	this->frame = 0.0f;
	this->endframe->value = numf;

	return true;
}


bool get_videotex(Layer *lay, const std::string& path) {

    // get the middle frame of this video and put it in a GL texture, as representation for the video

    GLenum err;
    lay->dummy = 1;

    lay = lay->open_video(0, path, true, true);
    if (mainprogram->openerr) {
        lay->closethread = true;
        return false;
    }
    std::unique_lock<std::mutex> olock(lay->endopenlock);
    lay->endopenvar.wait(olock, [&] { return lay->opened; });
    lay->opened = false;
    olock.unlock();
    if (mainprogram->openerr) {
        printf("error loading video texture!\n");
        mainprogram->openerr = false;
        lay->closethread = true;
        return false;
    }
    lay->frame = lay->numf / 2.0f;
    lay->prevframe = lay->frame - 1;
    lay->initialized = true;
    lay->keyframe = true;
    lay->ready = true;
    lay->keyframe = false;
    while (lay->ready) {
        lay->startdecode.notify_all();
    }
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
    find_stream_index(&(video_stream_idx), video, AVMEDIA_TYPE_VIDEO);
	video_stream = video->streams[video_stream_idx];
	this->frame = 0.0f;
	this->startframe->value = 0;
	this->endframe->value = video_stream->nb_frames;
    avformat_close_input(&video);

	return true;
}

bool get_layertex(Layer *lay, const std::string& path) {
    // get the middle frame of this layer files' video and put it in a GL texture, as representation for the video
    lay->dummy = true;
    lay->pos = 0;
    lay->node = mainprogram->nodesmain->currpage->add_videonode(2);
    lay->node->layer = lay;
    lay->lasteffnode[0] = lay->node;
    lay->lasteffnode[1] = lay->node;
    Layer *l = mainmix->open_layerfile(path, lay, true, 0);
    //lay->set_inlayer(l, true);
    mainprogram->getvideotexlayers[std::find(mainprogram->getvideotexlayers.begin(), mainprogram->getvideotexlayers.end(), lay) - mainprogram->getvideotexlayers.begin()] = l;
    lay->closethread = true;
    lay = l;
    std::unique_lock<std::mutex> olock(lay->endopenlock);
    lay->endopenvar.wait(olock, [&] { return lay->opened; });
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
    lay->frame = lay->numf / 2.0f;
    lay->prevframe = lay->frame - 1;
    lay->keyframe = true;
	lay->ready = true;
	while (lay->ready) {
		lay->startdecode.notify_all();
	}
	std::unique_lock<std::mutex> lock(lay->enddecodelock);
	lay->enddecodevar.wait(lock, [&] {return lay->processed; });
	lay->processed = false;
	lock.unlock();
	lay->keyframe = false;

    lay->initialized = true;

    lay->processed = true;
}

bool Clip::get_layerframes() {
	std::string result = deconcat_files(this->path);
	bool concat = (result != "");
	std::ifstream rfile;
	if (concat) rfile.open(result);
	else rfile.open(this->path);

	std::string istring;

	while (safegetline(rfile, istring)) {
		if (istring == "ENDOFFILE") break;
		if (istring == "startframe->value") {
			safegetline(rfile, istring);
			this->startframe->value = std::stoi(istring);
		}
		if (istring == "endframe->value") {
			safegetline(rfile, istring);
			this->endframe->value = std::stoi(istring);
		}
		if (istring == "FRAME") {
			safegetline(rfile, istring);
			this->frame = std::stof(istring);
		}
	}

	rfile.close();

	return 1;
}

bool get_deckmixtex(Layer *lay, const std::string& path) {
    // get the representational jpeg of this deck/mix that was saved with/in the deck/mix file and put it in a GL
    // texture

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
        bfile.read((char*)& num, 4); // 20011975
		bfile.read((char*)& num, 4);
        mainprogram->result = result;
        mainprogram->resnum = num;
	}
	else {
        mainprogram->resnum = -1;
        return false;
    }
}

void Clip::open_clipfiles() {
	// order elements
    if (mainprogram->paths.size() == 0) {
        mainprogram->openclipfiles = false;
        mainprogram->multistage = 0;
        return;
    }
	bool cont = mainprogram->order_paths(false);
	if (!cont) return;

	const std::string str = mainprogram->paths[mainprogram->clipfilescount];
	int pos = std::find(mainprogram->clipfileslay->clips.begin(), mainprogram->clipfileslay->clips.end(), mainprogram->clipfilesclip) - mainprogram->clipfileslay->clips.begin();
	if (pos == mainprogram->clipfileslay->clips.size() - 1) {
		Clip* clip = new Clip;
		if (mainprogram->clipfileslay->clips.size() > 4) mainprogram->clipfileslay->queuescroll++;
		mainprogram->clipfilesclip = clip;
		clip->insert(mainprogram->clipfileslay, mainprogram->clipfileslay->clips.end() - 1);
	}
	mainprogram->clipfilesclip->path = str;

	if (isimage(str)) {
        Layer *lay = new Layer(true);
        get_imagetex(lay, str);
        std::unique_lock<std::mutex> lock2(lay->enddecodelock);
        lay->enddecodevar.wait(lock2, [&] {return lay->processed; });
        lay->processed = false;
        lock2.unlock();
        mainprogram->clipfilesclip->tex = mainprogram->get_tex(lay);
        lay->closethread = true;

		mainprogram->clipfilesclip->type = ELEM_LAYER;
		mainprogram->clipfilesclip->get_imageframes();
	}
	else if (str.substr(str.length() - 6, std::string::npos) == ".layer") {
        Layer *lay = new Layer(true);
        get_layertex(lay, str);
        std::unique_lock<std::mutex> lock2(lay->enddecodelock);
        lay->enddecodevar.wait(lock2, [&] {return lay->processed; });
        lay->processed = false;
        lock2.unlock();
        GLuint butex = lay->fbotex;
        lay->fbotex = copy_tex(lay->texture);
        glDeleteTextures(1, &butex);
        onestepfrom(0, lay->node, nullptr, -1, -1);
        if (lay->effects[0].size()) {
            mainprogram->clipfilesclip->tex = copy_tex(lay->effects[0][lay->effects[0].size() - 1]->fbotex, binsmain->elemboxes[0]->scrcoords->w, binsmain->elemboxes[0]->scrcoords->h);
        } else {
            mainprogram->clipfilesclip->tex = copy_tex(lay->fbotex, binsmain->elemboxes[0]->scrcoords->w, binsmain->elemboxes[0]->scrcoords->h);
        }
        lay->closethread = true;

		mainprogram->clipfilesclip->type = ELEM_LAYER;
		mainprogram->clipfilesclip->get_layerframes();
	}
	else {
        Layer *lay = new Layer(true);
        get_videotex(lay, str);
        std::unique_lock<std::mutex> lock2(lay->enddecodelock);
        lay->enddecodevar.wait(lock2, [&] {return lay->processed; });
        lay->processed = false;
        lock2.unlock();

        mainprogram->clipfilesclip->tex = mainprogram->get_tex(lay);
        lay->closethread = true;
		mainprogram->clipfilesclip->type = ELEM_LAYER;
		mainprogram->clipfilesclip->get_videoframes();
	}
	GLuint butex = mainprogram->clipfilesclip->tex;
	mainprogram->clipfilesclip->tex = copy_tex(mainprogram->clipfilesclip->tex);
    mainprogram->clipfilesclip->jpegpath = find_unused_filename(basename(mainprogram->clipfilesclip->path), mainprogram->temppath, ".jpg");
    save_thumb(mainprogram->clipfilesclip->jpegpath, mainprogram->clipfilesclip->tex);
	if (butex != -1) glDeleteTextures(1, &butex);
	mainprogram->clipfilescount++;
	mainprogram->clipfilesclip = mainprogram->clipfileslay->clips[pos + 1];
	if (mainprogram->clipfilescount == mainprogram->paths.size()) {
		mainprogram->clipfileslay->cliploading = false;
		mainprogram->openclipfiles = false;
		mainprogram->paths.clear();
		mainprogram->multistage = 0;
	}
}


void handle_scenes(Scene* scene) {
	// Draw scene boxes
	float red[] = { 1.0, 0.5, 0.5, 1.0 };
	for (int i = 3; i > -1; i--) {
		Box* box = mainmix->scenes[scene->deck][i]->box;
		if (i == mainmix->currscene[scene->deck]) {
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
    bool found = false;
	for (int i = 0; i < 4; i++) {
		Box* box = mainmix->scenes[scene->deck][i]->box;
		Button* but = mainmix->scenes[scene->deck][i]->button;
		box->acolor[0] = 0.0;
		box->acolor[1] = 0.0;
		box->acolor[2] = 0.0;
		box->acolor[3] = 1.0;
        if (box->in() || but->value) {
            found = true;
            if (mainprogram->menuactivation && !mainprogram->menuondisplay) {
                mainprogram->parammenu3->state = 2;
                mainmix->learnparam = nullptr;
                mainmix->learnbutton = but;
                mainprogram->menuactivation = false;
            } else {
                if (but != mainprogram->onscenebutton) {
                    mainprogram->onscenedeck = scene->deck;
                    mainprogram->onscenebutton = but;
                    mainprogram->onscenemilli = 0;
                }
                if (((mainprogram->onscenemilli > 3000 || mainprogram->leftmouse) && !mainprogram->menuondisplay) ||
                    but->value) {
                    but->value = false;
                    // switch scenes
                    Scene *si = mainmix->scenes[scene->deck][i];
                    if (i == mainmix->currscene[scene->deck]) continue;
                    si->tempnblayers.clear();
                    si->tempnbframes.clear();
                    for (int j = 0; j < si->nblayers.size(); j++) {
                        si->tempnblayers.push_back(si->nblayers[j]);
                        if (!si->loaded) {
                            si->tempnbframes.push_back(si->nbframes[j]);
                        }
                    }
                    std::vector<Layer *> &lvec = choose_layers(scene->deck);
                    for (Layer *nbl: scene->nblayers) {
                        nbl->tobedeleted = true;
                    }
                    scene->nblayers.clear();
                    scene->nbframes.clear();
                    for (int j = 0; j < lvec.size(); j++) {
                        // store layers and frames of current scene into nblayers for running their framecounters in the background (to keep sync)
                        if (lvec[j]->filename == "") continue;
                        lvec[j]->tobedeleted = false;
                        scene->nblayers.push_back(lvec[j]);
                        scene->nbframes.push_back(lvec[j]->frame);
                    }
                    mainmix->mousedeck = scene->deck;
                    // save current scene to temp dir, open new scene
                    mainmix->do_save_deck(mainprogram->temppath + "tempdeck_xch.deck", true, true);
                    //SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
                    // stop current scene loopstation line if they don't extend to the other deck
                    for (int j = 0; j < loopstation->elems.size(); j++) {
                        std::unordered_set<Param *>::iterator it;
                        std::vector<Param *> tobeerased;
                        for (it = loopstation->elems[j]->params.begin();
                             it != loopstation->elems[j]->params.end(); it++) {
                            Param *par = *it;
                            if (par->effect) {
                                if (std::find(lvec.begin(), lvec.end(), par->effect->layer) != lvec.end()) {
                                    tobeerased.push_back(par);
                                }
                            }
                        }
                        std::unordered_set<Button *>::iterator it2;
                        std::vector<Button *> tobeerased2;
                        for (it2 = loopstation->elems[j]->buttons.begin();
                             it2 != loopstation->elems[j]->buttons.end(); it2++) {
                            Button *but = *it2;
                            if (std::find(lvec.begin(), lvec.end(), but->layer) != lvec.end()) {
                                tobeerased2.push_back(but);
                            }
                        }
                        for (int k = 0; k < tobeerased.size(); k++) {
                            if (tobeerased[k] == mainmix->crossfadecomp) {
#ifdef POSIX
                                sleep(1);
#endif
#ifdef WINDOWS
                                Sleep(1000);
#endif
                            }
                            loopstation->elems[j]->params.erase(tobeerased[k]);
                        }
                        for (int k = 0; k < tobeerased2.size(); k++) {
                            loopstation->elems[j]->buttons.erase(tobeerased2[k]);
                        }
                        if (loopstation->elems[j]->params.size() == 0 && loopstation->elems[j]->buttons.size()) {
                            loopstation->elems[j]->erase_elem();
                        }
                    }
                    mainmix->mousedeck = scene->deck;
                    Layer *bulay = mainmix->currlay[!mainprogram->prevmodus];
                    mainmix->open_deck(mainprogram->temppath + "tempdecksc_" + std::to_string(scene->deck) +
                                       std::to_string(i) + ".deck", true);
                    boost::filesystem::rename(mainprogram->temppath + "tempdeck_xch.deck",
                                              mainprogram->temppath + "tempdecksc_" + std::to_string(scene->deck) +
                                              std::to_string(mainmix->currscene[scene->deck]) + ".deck");
                    std::vector<Layer *> &lvec2 = choose_layers(scene->deck);
                    for (int j = 0; j < lvec2.size(); j++) {
                        if (lvec2[j]->filename == "") continue;
                        // set layer values to (running in background) values of loaded scene
                        if (si->loaded) lvec2[j]->frame = si->nbframes[j];
                        else {
                            lvec2[j]->frame = si->tempnbframes[j];
                            lvec2[j]->startframe->value = si->tempnblayers[j]->startframe->value;
                            lvec2[j]->endframe->value = si->tempnblayers[j]->endframe->value;
                            lvec2[j]->layerfilepath = si->tempnblayers[j]->layerfilepath;
                            lvec2[j]->filename = si->tempnblayers[j]->filename;
                            lvec2[j]->type = si->tempnblayers[j]->type;
                            lvec2[j]->oldalive = si->tempnblayers[j]->oldalive;
                        }
                    }
                    mainmix->reconnect_all(lvec2);
                    const int max = lvec.size() - 1;
                    if (scene->deck == bulay->deck)
                        mainmix->currlay[!mainprogram->prevmodus] = lvec2[std::clamp(bulay->pos, 0, max)];
                    mainmix->currscene[scene->deck] = i;
                    si->loaded = false;

                    /*mainmix->bulrs[scene->deck].clear();
                    for (int j = 0; j < mainmix->butexes[scene->deck].size(); j++) {
                        glDeleteTextures(1, &mainmix->butexes[scene->deck][j]);
                    }*/
                }
                box->acolor[0] = 0.5;
                box->acolor[1] = 0.5;
                box->acolor[2] = 1.0;
                box->acolor[3] = 1.0;
            }
        }

		std::string s = std::to_string(i + 1);
		std::string pchar;
		pchar = s;
		if (mainmix->learnbutton == but && mainmix->learn) pchar = "M";
		render_text(pchar, white, box->vtxcoords->x1 + 0.01f, box->vtxcoords->y1 + 0.025f, 0.0006f, 0.001f);
	}
    if (!found && mainprogram->onscenedeck == scene->deck) {
        mainprogram->onscenebutton = nullptr;
        mainprogram->onscenemilli = 0.0f;
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



void do_text_input(float x, float y, float sx, float sy, int mx, int my, float width, int smflag, PrefItem* item) {
    do_text_input(x, y, sx, sy, mx, my, width, smflag, item, false);
}

void do_text_input(float x, float y, float sx, float sy, int mx, int my, float width, int smflag, PrefItem * item, bool directdraw) {
		// handle display and mouse selection of keyboard input

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
	if (mainprogram->leftmousedown || mainprogram->leftmouse || mainprogram->orderleftmousedown || mainprogram->orderleftmouse) {
		bool found = false;
		if (totvec.size()) {
		    for (int j = 0; j < totvec.size() + 1; j++) {
		        if (my < mainprogram->yvtxtoscr(1.0f - (y - 0.005f)) && my > mainprogram->yvtxtoscr(1.0f - (y + (mainprogram->texth / 2.6f)
		        / (2070.0f / glob->h)))) {
		            if (mx >= mainprogram->xvtxtoscr(x + 1.0f + distin) * (j != 0) && mx <= mainprogram->xvtxtoscr(x + 1.0f + distin
		            + (j > 0) * totvec[j - 1 + (j == 0)] / 2.0f + totvec[j] / 2.0f) + (j == totvec.size()) * 99999) {
		                // click or drag inside path moves edit cursor
		                found = true;
		                if (mainprogram->leftmousedown || mainprogram->orderleftmousedown) {
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
		                if (mainprogram->leftmouse || mainprogram->orderleftmouse) {
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
		}
		if (found == false) {
			//when clicked outside path, cancel edit
			mainmix->adaptnumparam = nullptr;
			if (item) item->renaming = false;
			if (mainmix->retargeting) {
                mainmix->renaming = false;
			}
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
			if (mainprogram->inputtext == "") {
			    mainprogram->texth = mainprogram->buth;
			}
			else mainprogram->buth = mainprogram->texth;
			register_line_draw(white, x + textw - mainprogram->xscrtovtx(mainprogram->startpos), y - 0.005f - (smflag
			> 0) * 0.005f, x + textw - mainprogram->xscrtovtx(mainprogram->startpos), y + (mainprogram->texth / 2.6f)
			/ (2070.0f / glob->h) - (smflag > 0) * (mainprogram->texth / 4.2f), directdraw);
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
			draw_box(white, white, x + textw1, y - 0.010f, textw2, 0.010f + mainprogram->texth / (2070.0f / glob->h),
            -1, false, false, true); // last parameter: graphic inversion
		}
	}
}
	



void enddrag(bool clips) {
    if (!clips) {
        set_queueing(false);
    }
	if (mainprogram->dragbinel) {
		//if (mainprogram->dragbinel->path.rfind, ".layer", nullptr != std::string::npos) {
		//	if (mainprogram->dragbinel->path.find("cliptemp_") != std::string::npos) {
		//		boost::filesystem::remove(mainprogram->dragbinel->path);			
		//	}
		//}
		if (mainprogram->shelfdragelem) {
			// when dragged inside shelf, shelfdragelem is deleted before enddrag(false) is called
			if (mainprogram->shelves[0]->prevnum != -1) {
				//std::swap(mainprogram->shelves[0]->elements[mainprogram->shelves[0]->prevnum]->tex, mainprogram->shelves[0]->elements[mainprogram->shelves[0]->prevnum]->oldtex);
			}
			else if (mainprogram->shelves[1]->prevnum != -1) {
				//std::swap(mainprogram->shelves[1]->elements[mainprogram->shelves[1]->prevnum]->tex, mainprogram->shelves[1]->elements[mainprogram->shelves[1]->prevnum]->oldtex);
			}
			//std::swap(elem->tex, mainprogram->shelfdragelem->tex);
			//mainprogram->shelfdragelem->tex = mainprogram->dragbinel->tex;
			mainprogram->shelfdragelem = nullptr;
		}
		mainprogram->shelfdragelem = nullptr;

		mainprogram->dragbinel = nullptr;
		if (mainprogram->draglay) {
            mainprogram->draglay->vidmoving = false;
        }
		mainprogram->draglay = nullptr;
		mainprogram->dragclip = nullptr;
		mainprogram->dragpath = "";
		mainmix->moving = false;
		mainprogram->dragout[0] = true;
		mainprogram->dragout[1] = true;
		//glDeleteTextures(1, mainprogram->dragbinel->tex);  maybe needs implementing in one case, check history
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
			if ((templay->prev()->pos == 0) && !templay->prev()->mutebut->value) {
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
	float halfwhite[] = { 1.0f, 1.0f, 1.0f, 0.5f };
	float deepred[4] = { 1.0, 0.0, 0.0, 1.0 };
	float red[] = { 1.0f, 0.5f, 0.5f, 1.0f };
	float green[] = { 0.0f, 0.75f, 0.0f, 1.0f };
	float orange[] = { 1.0f, 0.5f, 0.0f, 1.0f };
	float blue[] = { 0.5f, 0.5f, 1.0f, 1.0f };
	float grey[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	float darkgrey[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	float lightblue[] = { 0.5f, 0.5f, 1.0f, 1.0f };

    SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDrawBuffer(GL_BACK_LEFT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

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
		draw_direct(nullptr, black, -1.0f, -1.0f, 2.0f, 2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, mainprogram->bgtex, glob->w,
              glob->h, false, false);
	}

    if (mainprogram->blocking ) {
		// when waiting for some activity spread out per loop
		mainprogram->mx = -1;
		mainprogram->my = 100;
		mainprogram->menuactivation = false;
	}
	if (mainprogram->oldmy <= mainprogram->yvtxtoscr(0.075f) && mainprogram->oldmx > glob->w - mainprogram->xvtxtoscr(0.05f)) {
		// for exit while midi learn
		mainprogram->eXit = true;
	}
	else mainprogram->eXit = false;
	if (mainmix->learn) {
		// when waiting MIDI learn, can't click anywhere else
		mainprogram->mx = -1;
		mainprogram->my = 100;
	}

	if (mainprogram->leftmouse && (mainprogram->cwon || mainprogram->menuondisplay || mainprogram->wiping || mainmix->adaptparam || mainmix->scrollon || binsmain->dragbin || mainmix->moving || mainprogram->dragbinel || mainprogram->drageff || mainprogram->dragclip || mainprogram->shelfdragelem)) {
		// special cases when mouse can be released over element that should not be triggered
		mainprogram->lmover = true;
		mainprogram->leftmouse = false;
	}
	else mainprogram->lmover = false;

	if (binsmain->floating) {
	    if (SDL_GetMouseFocus() == binsmain->win) {
            mainprogram->binmenuactivation = mainprogram->menuactivation;
            if (mainprogram->lmover) {
                mainprogram->lmover = false;
                mainprogram->binlmover = true;
            }
        }
	}
    if (SDL_GetMouseFocus() != mainprogram->mainwindow) {
        // when in prefs or config_midipresets_init windows
        mainprogram->mx = -100;
        mainprogram->my = 100;
        mainprogram->menuactivation = false;
    }

	if (mainmix->adaptparam) {
		// no hovering while adapting param
		mainprogram->my = 999999;
	}
	mainprogram->iemy = mainprogram->my;  // allow Insert effect click on border of parameter box

	// set mouse button values to enable blocking when item ordering overlay is on (reminder|maybe not the best solution)
	if (mainprogram->orderondisplay || mainmix->retargeting) {
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

	// if not server then try to connect the client
	if (!mainprogram->server && !mainprogram->connfailed) mainprogram->startclient.notify_all();

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
    for (int j = 0; j < 2; j++) {
        for (int i = 0; i < 4; i++) {
            for (int k = 0; k < mainmix->scenes[j][i]->nblayers.size(); k++) {
                if (i == mainmix->currscene[j]) break;
                mainmix->scenes[j][i]->nblayers[k]->calc_texture(0, 0);
                mainmix->scenes[j][i]->nbframes[k] = mainmix->scenes[j][i]->nblayers[k]->frame;
            }
        }
    }




    // MIDI stuff
    midi_set();
    mainprogram->shelf_miditriggering();


    if (!mainprogram->binsscreen) {
        //handle shelves
        mainprogram->inshelf = -1;
        mainprogram->handle_shelf(mainprogram->shelves[0]);
        mainprogram->handle_shelf(mainprogram->shelves[1]);
    }


    if (!mainprogram->binsscreen) {
        mainprogram->preview_modus_buttons();
        make_layboxes();
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
            if (testlay->filename != "") testlay->calc_texture(1, 1);
            testlay->load_frame();
        }
        for(int i = 0; i < mainmix->layersBcomp.size(); i++) {
			Layer *testlay = mainmix->layersBcomp[i];
            if (testlay->filename != "") testlay->calc_texture(1, 1);
			testlay->load_frame();
		}
		//for(int i = 0; i < mainprogram->busylayers.size(); i++) {
		//	Layer *testlay = mainprogram->busylayers[i];  // needs to be revised, reminder: dont remember why, something with doubling...?
        //    if (testlay->filename != "") testlay->calc_texture(1, 1);
		//	testlay->load_frame();
		//}
	}
    // when in preview modus, do frame texture load for both mixes (preview and output)
	if (mainprogram->prevmodus) {
		for(int i = 0; i < mainmix->layersA.size(); i++) {
			Layer *testlay = mainmix->layersA[i];
            if (testlay->filename != "") testlay->calc_texture(0, 1);
			testlay->load_frame();
		}
		for(int i = 0; i < mainmix->layersB.size(); i++) {
			Layer *testlay = mainmix->layersB[i];
            if (testlay->filename != "") testlay->calc_texture(0, 1);
			testlay->load_frame();
		}
	}
	if (mainprogram->prevmodus) {
		for(int i = 0; i < mainmix->layersAcomp.size(); i++) {
			Layer *testlay = mainmix->layersAcomp[i];
            if (testlay->filename != "") testlay->calc_texture(1, 1);
			testlay->load_frame();
		}
		for(int i = 0; i < mainmix->layersBcomp.size(); i++) {
			Layer *testlay = mainmix->layersBcomp[i];
            if (testlay->filename != "") testlay->calc_texture(1, 1);
			testlay->load_frame();
		}
	}

    //for (GUI_Element *elem : mainprogram->guielems) {
    //    if (elem->line) delete elem->line;
    //    if (elem->triangle) delete elem->triangle;
    //    if (elem->box) delete elem->box;
    //    delete elem;
    //}
    //sleep(0.01);
    //return;


    // Crawl web
	if (mainprogram->prevmodus) walk_nodes(0);
	walk_nodes(1);
    make_layboxes();



#ifdef POSIX
    mainprogram->stream_to_v4l2loopbacks();
#endif


    /////////////// STUFF THAT BELONGS TO EITHER BINS OR MIX OR FULL SCREEN OR RETARGETING

    if (mainprogram->binsscreen && !binsmain->floating) {
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

    else if (mainmix->retargeting) {
        // handle searching for lost media files when loading a file
        if (mainmix->retargetingdone) {
            do_retarget();

            for (int i = 0; i < retarget->localsearchdirs.size(); i++) {
                if (retarget->searchglobalbuttons[i]->value) {
                    if (std::find(retarget->globalsearchdirs.begin(), retarget->globalsearchdirs.end(), retarget->localsearchdirs[i]) == retarget->globalsearchdirs.end()) {
                        retarget->globalsearchdirs.push_back(retarget->localsearchdirs[i]);
                    }
                }
            }

            mainprogram->pathscroll = 0;
            mainprogram->prefs->save();  // save default search dirs

            mainmix->newpathlayers.clear();
            mainmix->newpathclips.clear();
            mainmix->newpathshelfelems.clear();
            mainmix->newpathbinels.clear();
            mainmix->newpathpos = 0;
            mainmix->skipall = false;
            mainmix->retargeting = false;
        }
        else {
            // retargeting lost videos/images
            mainprogram->frontbatch = true;

            std::function<void()> load_data = [&load_data]() {
                if (mainmix->retargetstage == 0) {
                    mainmix->newpaths = &mainmix->newlaypaths;
                    if ((*(mainmix->newpaths)).size() == 0) {
                        check_stage();
                        load_data();
                    } else {
                        mainmix->newpaths = &mainmix->newlaypaths;
                        retarget->lay = mainmix->newpathlayers[mainmix->newpathpos];
                        retarget->tex = retarget->lay->jpegtex;
                        retarget->filesize = retarget->lay->filesize;
                    }
                }
                if (mainmix->retargetstage == 1) {
                    mainmix->newpaths = &mainmix->newclippaths;
                    if ((*(mainmix->newpaths)).size() == 0) {
                        check_stage();
                        load_data();
                    } else {
                        mainmix->newpaths = &mainmix->newclippaths;
                        retarget->clip = mainmix->newpathclips[mainmix->newpathpos];
                        retarget->tex = retarget->clip->tex;
                        retarget->filesize = retarget->clip->filesize;
                    }
                }
                if (mainmix->retargetstage == 2) {
                    mainmix->newpaths = &mainmix->newshelfpaths;
                    if ((*(mainmix->newpaths)).size() == 0) {
                        check_stage();
                        load_data();
                    } else {
                        mainmix->newpaths = &mainmix->newshelfpaths;
                        retarget->shelem = mainmix->newpathshelfelems[mainmix->newpathpos];
                        retarget->tex = retarget->shelem->tex;
                        retarget->filesize = retarget->shelem->filesize;
                    }
                }
                if (mainmix->retargetstage == 3) {
                    mainmix->newpaths = &mainmix->newbinelpaths;
                    if ((*(mainmix->newpaths)).size() == 0) {
                        check_stage();
                        if (!mainmix->retargeting) {
                            mainprogram->frontbatch = false;
                            return;
                        }
                        load_data();
                    } else {
                        retarget->binel = mainmix->newpathbinels[mainmix->newpathpos];
                        retarget->tex = retarget->binel->tex;
                        retarget->filesize = retarget->binel->filesize;
                    }
                }
            };
            load_data();

            if (retarget->searchall) {
                bool ret = retarget_search();
                if (ret) check_stage();
                else {
                    retarget->notfound = true;
                    retarget->searchall = false;
                }
            }

            draw_box(white, black, -0.4, 0.1f, 0.8f, 0.1f, -1);
            render_text("THIS FILE WASN'T FOUND, THIS IS YOUR CHANCE TO RETARGET IT", white, -0.4f + 0.015f,
                        0.1f + 0.075f - 0.045f, 0.00045f,
                        0.00075f);
            draw_box(white, black, retarget->valuebox, -1);
            draw_box(white, black, retarget->skipbox, -1);
            render_text("SKIP", white, 0.13f, 0.1f + 0.075f - 0.045f, 0.0009f,
                        0.0015f);
            draw_box(white, black, retarget->skipallbox, -1);
            render_text("SKIP", white, 0.23f, 0.125f + 0.075f - 0.045f, 0.00045f,
                        0.00075f);
            render_text("ALL", white, 0.23f, 0.1f + 0.075f - 0.045f, 0.00045f,
                        0.00075f);
            draw_box(white, black, 0.3f, retarget->valuebox->vtxcoords->y1, 0.2f, 0.2f, retarget->tex);
            draw_box(white, nullptr, 0.3f, retarget->valuebox->vtxcoords->y1, 0.2f, 0.2f, -1);
            if (retarget->skipbox->in() && mainprogram->orderleftmouse) {
                (*(mainmix->newpaths))[mainmix->newpathpos] = "";
                check_stage();
            }
            if ((retarget->skipallbox->in() && mainprogram->orderleftmouse) || mainmix->skipall) {
                mainmix->skipall = true;
                (*(mainmix->newpaths))[mainmix->newpathpos] = "";
                check_stage();
            }
            if (retarget->iconbox->in() && mainprogram->orderleftmouse) {
                mainprogram->pathto = "RETARGETFILE";
                std::thread filereq(&Program::get_inname, mainprogram, "Find file", "",
                                    boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
                filereq.detach();
            }
            if (mainmix->renaming == false) {
                render_text((*(mainmix->newpaths))[mainmix->newpathpos], white, -0.4f + 0.015f, retarget->valuebox->vtxcoords->y1 + 0.075f - 0.045f,
                            0.00045f,
                            0.00075f);
                if (exists((*(mainmix->newpaths))[mainmix->newpathpos])) {
                    mainprogram->currfilesdir = dirname((*(mainmix->newpaths))[mainmix->newpathpos]);
                    check_stage();
                }
            }
            else {
                if (mainprogram->renaming == EDIT_NONE) {
                    mainmix->renaming = false;
                    (*(mainmix->newpaths))[mainmix->newpathpos] = mainprogram->inputtext;
                    if (exists((*(mainmix->newpaths))[mainmix->newpathpos])) {
                        mainprogram->currfilesdir = dirname((*(mainmix->newpaths))[mainmix->newpathpos]);
                        check_stage();
                    }
                }
                else if (mainprogram->renaming == EDIT_CANCEL) {
                    mainmix->renaming = false;
                }
                else {
                    do_text_input(-0.4f + 0.015f, retarget->valuebox->vtxcoords->y1 + 0.075f - 0.045f, 0.00045f, 0.00075f, mainprogram->mx, mainprogram->my, mainprogram->xvtxtoscr(0.55f), 0, nullptr, false);
                }
            }
            if (retarget->valuebox->in() && mainprogram->orderleftmouse) {
                mainmix->renaming = true;
                mainprogram->renaming = EDIT_STRING;
                mainprogram->inputtext = (*(mainmix->newpaths))[mainmix->newpathpos];
                mainprogram->cursorpos0 = mainprogram->inputtext.length();
                SDL_StartTextInput();
            }
            draw_box(white, black, retarget->iconbox, -1);
            draw_box(white, black, retarget->iconbox->vtxcoords->x1 + 0.035f, retarget->iconbox->vtxcoords->y1 + 0.035f, 0.03f, 0.035f, -1);
            draw_box(white, black, retarget->iconbox->vtxcoords->x1 + 0.05f, retarget->iconbox->vtxcoords->y1 + 0.060f, 0.0125f, 0.015f, -1);

            draw_box(white, black, retarget->searchbox, -1);
            render_text("OR CLICK HERE TO SET A SEARCH LOCATION", white, -0.4f + 0.015f,
                        -0.1f + 0.075f - 0.045f, 0.00045f,
                        0.00075f);
            draw_box(white, black, retarget->searchbox->vtxcoords->x1 + 0.635f, retarget->searchbox->vtxcoords->y1 + 0.035f, 0.03f, 0.035f, -1);
            draw_box(white, black, retarget->searchbox->vtxcoords->x1 + 0.65f, retarget->searchbox->vtxcoords->y1 + 0.060f, 0.0125f, 0.015f, -1);
            if (retarget->searchbox->in() && mainprogram->orderleftmouse) {
                mainprogram->pathto = "SEARCHDIR";
                std::thread filereq(&Program::get_dir, mainprogram, "Add a search location",
                                    boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
                filereq.detach();
                retarget->notfound = false;
            }
            if (retarget->globalsearchdirs.size() || retarget->localsearchdirs.size()) {
                // mousewheel scroll
                mainprogram->pathscroll -= mainprogram->mousewheel;
                if (mainprogram->pathscroll < 0) mainprogram->pathscroll = 0;
                int totalsize = std::min(mainprogram->pathscroll + 7, (int)retarget->globalsearchdirs.size() + (int)retarget->localsearchdirs.size());
                if (totalsize > 6 && totalsize - mainprogram->pathscroll < 7) mainprogram->pathscroll = totalsize - 6;

                // GUI arrow scroll
                mainprogram->pathscroll = mainprogram->handle_scrollboxes(mainprogram->searchscrollup, mainprogram->searchscrolldown, totalsize, mainprogram->pathscroll, 6);

                std::vector<Box> boxes;
                short count = 0;
                bool brk = false;
                for (int i = 0; i < 2; i++) {
                    std::vector<std::string> *dirs;
                    if (i == 0) dirs = &retarget->globalsearchdirs;
                    else dirs = &retarget->localsearchdirs;
                    int size = std::min(6, (int)(*(dirs)).size());
                    for (int j = 0; j < size - (mainprogram->pathscroll * (i == 0)); j++) {
                        draw_box(white, black, retarget->searchboxes[count], -1);
                        render_text((*(dirs))[j + (mainprogram->pathscroll * (i == 0))], white, -0.4f + 0.015f,
                                    retarget->searchboxes[count]->vtxcoords->y1 + 0.075f - 0.045f,
                                    0.00045f,
                                    0.00075f);
                        mainprogram->handle_button(retarget->searchglobalbuttons[count]);
                        render_text("X", white, retarget->searchclearboxes[count]->vtxcoords->x1 + 0.003f,
                                    retarget->searchclearboxes[count]->vtxcoords->y1, 0.001125f,
                                    0.001875f);
                        if (!retarget->searchglobalbuttons[count]->box->in()) {
                            if (retarget->searchclearboxes[count]->in() && mainprogram->orderleftmouse) {

                                (*(dirs)).erase((*(dirs)).begin() + j - (mainprogram->pathscroll * (i == 0)));
                                retarget->searchboxes.erase(retarget->searchboxes.begin() + count);
                                retarget->searchglobalbuttons.erase(retarget->searchglobalbuttons.begin() + count);
                                retarget->searchclearboxes.erase(retarget->searchclearboxes.begin() + count);
                                for (int k = count; k < retarget->searchboxes.size(); k++) {
                                    retarget->searchboxes[k]->vtxcoords->y1 += 0.1f;
                                    retarget->searchglobalbuttons[k]->box->vtxcoords->y1 += 0.1f;
                                    retarget->searchclearboxes[k]->vtxcoords->y1 += 0.1f;
                                    retarget->searchboxes[k]->upvtxtoscr();
                                    retarget->searchglobalbuttons[k]->box->upvtxtoscr();
                                    retarget->searchclearboxes[k]->upvtxtoscr();
                                }
                            } else if (retarget->searchboxes[count]->in() && mainprogram->orderleftmouse) {
                                //reminder: mainmix->renaming = true;
                                //mainprogram->renaming = EDIT_STRING;
                                //mainprogram->inputtext = retarget->localsearchdirs[j];
                                //mainprogram->cursorpos0 = mainprogram->inputtext.length();
                                //SDL_StartTextInput();
                            }
                        }
                        count++;
                        if (count == 6) {
                            brk = true;
                            break;
                        }
                    }
                    if (brk) break;
                }
                float y2 = -0.2f - std::min(6, totalsize) * 0.1f;
                std::unique_ptr <Box> box = std::make_unique <Box> ();;
                box->vtxcoords->x1 = -0.15f;
                box->vtxcoords->y1 = y2;
                box->vtxcoords->w = 0.15f;
                box->vtxcoords->h = 0.1f;
                box->upvtxtoscr();
                draw_box(white, black, box, -1);
                render_text("SEARCH ONE", white, -0.15f + 0.015f, y2 + 0.075f - 0.045f,
                            0.00045f,
                            0.00075f);
                if (box->in()) {
                    if (mainprogram->orderleftmouse) {
                        bool ret = retarget_search();
                        if (ret) {
                            check_stage();
                            retarget->notfound = false;
                        }
                        else retarget->notfound = true;
                    }
                }
                box->vtxcoords->x1 = 0.0f;
                box->vtxcoords->y1 = y2;
                box->vtxcoords->w = 0.15f;
                box->vtxcoords->h = 0.1f;
                box->upvtxtoscr();
                draw_box(white, black, box, -1);
                render_text("SEARCH ALL", white, 0.0f + 0.015f, y2 + 0.075f - 0.045f,
                            0.00045f,
                            0.00075f);
                if (box->in()) {
                    if (mainprogram->orderleftmouse) {
                        retarget->searchall = true;
                        retarget->notfound = false;
                    }
                }

                if (retarget->notfound) {
                    box->vtxcoords->x1 = -0.075f;
                    box->vtxcoords->y1 = y2 - 0.1f;
                    box->vtxcoords->w = 0.15f;
                    box->vtxcoords->h = 0.1f;
                    box->upvtxtoscr();
                    draw_box(white, black, box, -1);
                    render_text("NOT FOUND!", white, -0.075f + 0.015f, y2 -0.1f + 0.075f - 0.045f,
                                0.00045f,
                                0.00075f);
                }
            }

            mainprogram->frontbatch = false;
        }
    }


        // Handle node view someda
	//else if (mainmix->mode == 1) {
	//	mainprogram->nodesmain->currpage->handle_nodes();
	//}

	else {  // this is MIX screen specific stuff
		if (mainmix->learn && mainprogram->rightmouse) {
			// MIDI learn cancel
			mainmix->learn = false;
			mainprogram->rightmouse = false;
		}


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


		// Draw and handle crossfade->box
		Param* par;
		if (mainprogram->prevmodus) par = mainmix->crossfade;
		else par = mainmix->crossfadecomp;
        mainmix->handle_param(par);


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


		// handle scenes
        render_text("A", red, mainmix->decknamebox[0]->vtxcoords->x1 + 0.01f, mainmix->decknamebox[0]->vtxcoords->y1 + 0.025f, 0.0006f, 0.001f);
        render_text("B", red, mainmix->decknamebox[1]->vtxcoords->x1 + 0.01f, mainmix->decknamebox[1]->vtxcoords->y1 + 0.025f, 0.0006f, 0.001f);
		if (!mainprogram->prevmodus) {
            handle_scenes(mainmix->scenes[0][mainmix->currscene[0]]);
            handle_scenes(mainmix->scenes[1][mainmix->currscene[1]]);
        }

        if (!mainprogram->binsscreen) {
            // draw and handle overall genmidi button
            mainmix->handle_genmidibuttons();
        }


		//draw and handle global deck speed sliders
		par = mainmix->deckspeed[!mainprogram->prevmodus][0];
		draw_box(white, darkgrey, mainmix->deckspeed[!mainprogram->prevmodus][0]->box->vtxcoords->x1, mainmix->deckspeed[!mainprogram->prevmodus][0]->box->vtxcoords->y1, mainmix->deckspeed[!mainprogram->prevmodus][0]->box->vtxcoords->w * 0.30f, 0.1f, -1);
        mainmix->handle_param(par);
		par = mainmix->deckspeed[!mainprogram->prevmodus][1];
		draw_box(white, darkgrey, mainmix->deckspeed[!mainprogram->prevmodus][1]->box->vtxcoords->x1, mainmix->deckspeed[!mainprogram->prevmodus][1]->box->vtxcoords->y1, mainmix->deckspeed[!mainprogram->prevmodus][1]->box->vtxcoords->w * 0.30f, 0.1f, -1);
        mainmix->handle_param(par);

		//draw and handle recbuts
        /*mainprogram->handle_button(mainmix->recbutS, 1, 0);
        if (mainmix->recbutS->toggled()) {
            if (!mainmix->recording[0]) {
                // start recording
                mainmix->reccodec = "libxvid";
                mainmix->start_recording();
            }
            else {
                mainmix->recording[0] = false;
            }
        }*/
        mainprogram->handle_button(mainmix->recbutQ, 1, 0);
        if (mainmix->recbutQ->toggled()) {
            if (!mainmix->recording[1]) {
                // start recording
                mainmix->reccodec = "hap";
                mainmix->reckind = 1;
                mainmix->start_recording();
             }
            else {
                mainmix->recording[1] = false;
            }
        }
        // draw and handle lastly recorded video snippets
        /*if (mainmix->recswitch[0]) {
            mainmix->recswitch[0] = false;
            mainmix->recSthumbshow = copy_tex(mainmix->recSthumb);
        }*/
        if (mainmix->recswitch[1]) {
            mainmix->recswitch[1] = false;
            mainmix->recQthumbshow = copy_tex(mainmix->recQthumb);
        }
        /*if (mainmix->recSthumbshow != -1) {
            int sw, sh;
            glBindTexture(GL_TEXTURE_2D, mainmix->recSthumbshow);
            glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &sw);
            glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &sh);
            Box box;
            box.vtxcoords->x1 = 0.15f + 0.0465f;
            box.vtxcoords->y1 = -1.0f + mainprogram->monh * 2.0f;
            box.vtxcoords->w = 0.040f * ((float)sw / (float)sh);
            box.vtxcoords->h = 0.075f;
            box.upvtxtoscr();
            draw_box(&box, mainmix->recSthumbshow);
            if (box.in() && mainprogram->leftmousedown) {
                mainprogram->leftmousedown = false;
                mainprogram->dragbinel = new BinElement;
                mainprogram->dragbinel->tex = mainmix->recSthumbshow;
                mainprogram->dragbinel->path = mainmix->recpath[0];
                mainprogram->dragbinel->type = ELEM_FILE;
            }
        }*/
        if (mainmix->recQthumbshow != -1) {
            int sw, sh;
            glBindTexture(GL_TEXTURE_2D, mainmix->recQthumbshow);
            glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &sw);
            glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &sh);
            Box box;
            box.vtxcoords->x1 = 0.15f + 0.0465f;
            box.vtxcoords->y1 = -1.0f + mainprogram->monh * 2.0f;
            box.vtxcoords->w = 0.040f * ((float)sw / (float)sh);
            box.vtxcoords->h = 0.075f;
            box.upvtxtoscr();
            draw_box(&box, mainmix->recQthumbshow);
            if (box.in() && mainprogram->leftmousedown) {
                mainprogram->leftmousedown = false;
                mainprogram->dragbinel = new BinElement;
                mainprogram->dragbinel->tex = mainmix->recQthumbshow;
                mainprogram->dragbinel->path = mainmix->recpath[1];
                mainprogram->dragbinel->type = ELEM_FILE;
            }
        }



		mainprogram->layerstack_scrollbar_handle();



        //handle dragging into layerstack
		if (mainprogram->dragbinel && !mainmix->moving) {
			if (mainprogram->dragbinel->type != ELEM_DECK && mainprogram->dragbinel->type != ELEM_MIX) {
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
                if (lpc->elems[i]->loopbut->value || lpc->elems[i]->playbut->value) lpc->elems[i]->set_values();
            }
        }
        else {
            lpc->handle();
        }


        // handle the layer clips queue
		mainmix->handle_clips();


		// draw "layer insert into stack" blue boxes
		if (!mainprogram->menuondisplay && mainprogram->dragbinel) {
		    mainprogram->frontbatch = true;
			for (int j = 0; j < 2; j++) {
				std::vector<Layer*>& lvec = choose_layers(j);
				for (int i = 0; i < lvec.size(); i++) {
					Layer* lay = lvec[i];
					bool comp = !mainprogram->prevmodus;
					if (lay->pos < mainmix->scenes[j][mainmix->currscene[j]]->scrollpos || lay->pos > mainmix->scenes[j][mainmix->currscene[j]]->scrollpos + 2) continue;
					Box* box = lay->node->vidbox;
					float thick = mainprogram->xvtxtoscr(0.075f);
					if (box->scrcoords->y1 - box->scrcoords->h < mainprogram->my && mainprogram->my < box->scrcoords->y1) {
						if (box->scrcoords->x1 - thick + (i - mainmix->scenes[j][mainmix->currscene[j]]->scrollpos == 0) * thick < mainprogram->mx && mainprogram->mx < box->scrcoords->x1 + thick) {
							// this block handles the first boxes, just not the last
							mainprogram->leftmousedown = false;
							float blue[] = { 0.5, 0.5, 1.0, 1.0 };
							// draw broad blue boxes when inserting layers
							draw_box(blue, blue, box->vtxcoords->x1 - mainprogram->xscrtovtx(thick) + (i - mainmix->scenes[j][mainmix->currscene[j]]->scrollpos == 0) * mainprogram->xscrtovtx(thick), box->vtxcoords->y1, mainprogram->xscrtovtx(thick * (2.0f - (i - mainmix->scenes[j][mainmix->currscene[j]]->scrollpos == 0))), mainprogram->layh, -1);
						}
						else if (box->scrcoords->x1 + box->scrcoords->w - thick < mainprogram->mx && mainprogram->mx < box->scrcoords->x1 + box->scrcoords->w) {
							mainprogram->leftmousedown = false;
							if (lay->pos == lvec.size() - 1 || lay->pos == mainmix->scenes[j][mainmix->currscene[j]]->scrollpos + 2) {
								// this block handles the last box
								float blue[] = { 0.5, 0.5 , 1.0, 1.0 };
								// draw broad blue boxes when inserting layers
								draw_box(blue, blue, box->vtxcoords->x1 + box->vtxcoords->w - mainprogram->xscrtovtx(thick), box->vtxcoords->y1, mainprogram->xscrtovtx(thick * (1.0f + (i - mainmix->scenes[j][mainmix->currscene[j]]->scrollpos != 2))), mainprogram->layh, -1);
							}
						}
					}
				}
			}
            mainprogram->frontbatch = false;
		}


        mainmix->vidbox_handle();

		mainmix->outputmonitors_handle();


		mainmix->layerdrag_handle();

        mainmix->deckmixdrag_handle();
	}



    if (mainprogram->rightmouse) {
		if (mainprogram->dragclip) {
			// cancel clipdragging
            mainprogram->dragclip->insert(mainprogram->draglay, mainprogram->draglay->clips.begin() + mainprogram->dragpos);
			mainprogram->dragclip = nullptr;
			mainprogram->draglay = nullptr;
			mainprogram->dragpos = -1;
			enddrag(false);
			mainprogram->rightmouse = false;
		}
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

	if (mainprogram->openfilesshelf) {
		// load one item from mainprogram->paths into shelf, one each loop not to slowdown output stream
		mainmix->mouseshelf->open_files_shelf();
	}

	if (binsmain->importbins) {
		// load one item from mainprogram->paths into binslist, one each loop not to slowdown output stream
		binsmain->import_bins();
	}

	// implementation of a basic top menu when the mouse is at the top of the screen
	if (mainprogram->my <= 0 && !mainprogram->transforming) {
	    if (!mainprogram->prefon && !mainprogram->midipresets && mainprogram->quitting == "") {
            mainprogram->intopmenu = true;
        }
	}
	if (mainprogram->intopmenu) {
		float lc[] = { 0.0, 0.0, 0.0, 1.0 };
		float ac1[] = { 0.3, 0.3, 0.3, 1.0 };
		float ac2[] = { 0.6, 0.6, 0.6, 1.0 };
		float deepred[] = { 1.0, 0.0, 0.0, 1.0 };
		mainprogram->frontbatch = true;
		draw_box(lc, ac1, -1.0f, 1.0f - 0.075f, 0.156f, 0.075f, -1);
		draw_box(lc, ac1, -1.0f + 0.156f, 1.0f - 0.075f, 0.156f, 0.075f, -1);
		draw_box(lc, ac2, -1.0f + 0.312f, 1.0f - 0.075f, 2.0f - 0.312f, 0.075f, -1);
		draw_box(nullptr, deepred, 1.0f - 0.05f, 1.0f - 0.075f, 0.05f, 0.075f, -1);
		render_text("x", white, 0.966f, 1.019f - 0.075f, 0.0012f, 0.002f);
		render_text("FILE", white, -1.0f + 0.0117f, 1.0f - 0.075f + 0.0225f, 0.00045f, 0.00075f);
		render_text("CONFIGURE", white, -1.0f + 0.156f + 0.0117f, 1.0f - 0.075f + 0.0225f, 0.00045f, 0.00075f);
        mainprogram->frontbatch = false;
		if (mainprogram->my > mainprogram->yvtxtoscr(0.075f)) {
			mainprogram->intopmenu = false;
		}
		else if (mainprogram->mx < mainprogram->xvtxtoscr(0.156f)) {
			if (mainprogram->leftmouse) {
				mainprogram->filemenu->menux = 0;
				mainprogram->filemenu->menuy = mainprogram->yvtxtoscr(0.075f);
				mainprogram->filedomenu->menux = 0;
				mainprogram->filedomenu->menuy = mainprogram->yvtxtoscr(0.075f);
				mainprogram->laylistmenu1->menux = 0;
				mainprogram->laylistmenu1->menuy = mainprogram->yvtxtoscr(0.075f);
				mainprogram->laylistmenu2->menux = 0;
				mainprogram->laylistmenu2->menuy = mainprogram->yvtxtoscr(0.075f);
				mainprogram->filemenu->state = 2;
			}
		}
		else if (mainprogram->mx < mainprogram->xvtxtoscr(0.312f)) {
			if (mainprogram->leftmouse) {
				mainprogram->editmenu->menux = mainprogram->xvtxtoscr(0.156f);
				mainprogram->editmenu->menuy = mainprogram->yvtxtoscr(0.075f);
				mainprogram->editmenu->state = 2;
			}
		}
		else if (mainprogram->mx > glob->w - mainprogram->xvtxtoscr(0.05f)) {
			if (mainprogram->leftmouse || mainprogram->orderleftmouse) {
				mainprogram->quitting = "closed window";
			}
		}
	}

	if (mainmix->learn && !mainprogram->prefon && !mainprogram->midipresets) {
        mainprogram->frontbatch = true;
		draw_box(red, blue, -0.3f, -0.0f, 0.6f, 0.3f, -1);
        mainprogram->frontbatch = false;
        render_text("Awaiting MIDI input.", white, -0.1f, 0.2f, 0.001f, 0.0016f);
        render_text("Rightclick cancels.", white, -0.1f, 0.06f, 0.001f, 0.0016f);
		// allow exiting with x icon during MIDI learn.
		draw_box(nullptr, deepred, 1.0f - 0.05f, 1.0f - 0.075f, 0.05f, 0.075f, -1);
		render_text("x", white, 0.966f, 1.019f - 0.075f, 0.0012f, 0.002f);
		if (mainprogram->eXit) {
			if (mainprogram->leftmouse) {
				mainprogram->quitting = "closed window";
			}
		}
	}

	if ((mainprogram->lmover && mainprogram->dragbinel)) {
		enddrag(false);
	}

	if (!mainprogram->prefon && !mainprogram->midipresets && mainprogram->tooltipbox && mainprogram->quitting == "") {
		if (mainprogram->tooltipbox->tooltip != "") mainprogram->tooltips_handle(0);
	}

	if (!mainprogram->binsscreen) {
		// leftmouse click outside clip queue cancels clip queue visualisation and deselects multiple selected layers
        bool found = false;
		for (int i = 0; i < 2; i++) {
            std::vector<Layer*> &lvec = choose_layers(i);
            for (int j = 0; j < lvec.size(); j++) {
                if (lvec[j]->queueing) {
                    found = true;
                    break;
                }
            }
        }
		std::vector<Layer*>& lvec1 = choose_layers(0);
		if (!found || (mainprogram->leftmouse && !mainprogram->menuondisplay)) {
			set_queueing(false);
			/*if (!mainprogram->boxhit && mainmix->currlay[!mainprogram->prevmodus]) {
                mainmix->currlays[!mainprogram->prevmodus].clear();
                mainmix->currlays[!mainprogram->prevmodus].push_back(mainmix->currlay[!mainprogram->prevmodus]);
            }  reminder : what was this?*/
		}
	}



    // Menu block

    mainprogram->frontbatch = true;

    mainprogram->handle_mixenginemenu();

    mainprogram->handle_effectmenu();

    mainprogram->handle_parammenu1();

    mainprogram->handle_parammenu2();

    mainprogram->handle_parammenu1b();

    mainprogram->handle_parammenu2b();

    mainprogram->handle_parammenu3();

    mainprogram->handle_parammenu4();

    mainprogram->handle_parammenu5();

    mainprogram->handle_parammenu6();

    mainprogram->handle_loopmenu();

    mainprogram->handle_monitormenu();

    mainprogram->handle_wipemenu();

    mainprogram->handle_laymenu1();

    mainprogram->handle_newlaymenu();

    mainprogram->handle_clipmenu();

    mainprogram->handle_mainmenu();

    mainprogram->handle_shelfmenu();

    mainprogram->handle_filemenu();

    mainprogram->handle_editmenu();

    mainprogram->handle_lpstmenu();

    mainprogram->frontbatch = false;

    if (mainprogram->menuactivation == true) {
        // main menu triggered
        mainprogram->mainmenu->state = 2;
        mainprogram->menuactivation = false;
    }


    for (Bin *bin : binsmain->bins) {
        // every loop iteration, save one bin element jpeg if there are any unsaved ones
        bool brk = false;
        for (int i = 0; i < 12; i++) {
            for (int j = 0; j < 12; j++) {
                if (bin->elements[i * 12 + j]->name != "") {
                    if (!exists(bin->elements[i * 12 + j]->jpegpath)) {
                        bin->elements[i * 12 + j]->jpegpath = find_unused_filename(bin->name,
                                                                                             mainprogram->temppath,
                                                                                             ".jpg");
                        save_thumb(bin->elements[i * 12 + j]->jpegpath,
                                   bin->elements[i * 12 + j]->tex);
                    }
                    brk = true;
                    break;
                }
            }
            if (brk) break;
        }
        if (brk) break;
    }



    //autosave
    if (mainmix->retargeting) mainprogram->astimestamp = (int) mainmix->time;  // postpone autosave
    if (mainprogram->autosave && mainmix->time > mainprogram->astimestamp + mainprogram->asminutes * 60) {
        mainprogram->project->autosave();
	}


    // sync with output views
    for (int i = 0; i < mainprogram->outputentries.size(); i++) {
        EWindow *win = mainprogram->outputentries[i]->win;
        win->syncnow = true;
        while (win->syncnow) {
            win->sync.notify_all();
        }
        std::unique_lock<std::mutex> lock(win->syncendmutex);
        win->syncend.wait(lock, [&]{return win->syncendnow;});
        win->syncendnow = false;
        lock.unlock();
    }

    // sync with bins window
    if (binsmain->floating) {
        binsmain->syncnow = true;
        while (binsmain->syncnow) {
            binsmain->sync.notify_all();
        }
        std::unique_lock<std::mutex> lock(binsmain->syncendmutex);
        binsmain->syncend.wait(lock, [&]{return binsmain->syncendnow;});
        binsmain->syncendnow = false;
        lock.unlock();

        SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
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


    bool prret = false;
	GLuint tex, fbo;
	if (mainprogram->quitting != "") {
		mainprogram->directmode = true;
        SDL_ShowWindow(mainprogram->quitwindow);
        SDL_RaiseWindow(mainprogram->quitwindow);
		SDL_GL_MakeCurrent(mainprogram->quitwindow, glc);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDrawBuffer(GL_BACK_LEFT);
		glViewport(0, 0, glob->w / 2.0f, glob->h / 2.0f);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
		int ret = mainprogram->quit_requester();
		if (ret == 1 || ret == 2) {
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

			// close all webcams
			for (Layer *lay : mainprogram->busylayers) {
			    avformat_close_input(&lay->video);
			}

			// close socket communication
			// if server ask other socket to become server else signal all other sockets that we're quitting
            if (mainprogram->connsockets.size()) {
                if (mainprogram->server) {
                    for (auto sock : mainprogram->connsockets) {
                        send(sock, "SERVER_QUITS", 12, 0);
                    }
                }
            }
            close(mainprogram->sock);

			mainprogram->prefs->save();

			boost::filesystem::path path_to_remove(mainprogram->temppath);
			for (boost::filesystem::directory_iterator end_dir_it, it(path_to_remove); it != end_dir_it; ++it) {
				boost::filesystem::remove(it->path());
			}

			printf("%s: %s\n", mainprogram->quitting.c_str(), SDL_GetError());
			printf("stopped\n");

			exit(1);
		}
		if (ret == 3) {
			mainprogram->quitting = "";
			SDL_HideWindow(mainprogram->quitwindow);
			SDL_RaiseWindow(mainprogram->mainwindow);
		}
		if (ret == 0) {
			SDL_GL_SwapWindow(mainprogram->quitwindow);
		}
		SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDrawBuffer(GL_BACK_LEFT);
        glViewport(0, 0, glob->w, glob->h);
        mainprogram->directmode = false;
	}



    mainprogram->preferences();

	mainprogram->config_midipresets_init();



    // render code

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDrawBuffer(GL_BACK_LEFT);


    if (mainprogram->fullscreen == -1 && !mainmix->retargeting) {
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        if (!binsmain->floating) {
            // draw and handle wormgates
            if (!mainprogram->binsscreen) mainprogram->handle_wormgate(0);
            mainprogram->handle_wormgate(1);
            if (mainprogram->dragbinel) {
                if (!mainprogram->wormgate1->box->in() && !mainprogram->wormgate2->box->in()) {
                    mainprogram->inwormgate = false;
                }
            }
        }

        // display the deck monitors and output monitors on the bottom of the screen
        if (!mainprogram->binsscreen) display_mix();
    }

    mainprogram->directmode = false;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDrawBuffer(GL_BACK_LEFT);
    glViewport(0, 0, glob->w, glob->h);
    if (1) {

        glEnable(GL_DEPTH_TEST);
        glClearDepth(1.0f);
        glClear(GL_DEPTH_BUFFER_BIT);
        glDepthFunc(GL_LESS);

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

        for (int i = mainprogram->currbatch + ((intptr_t) mainprogram->bdtptr[mainprogram->currbatch] -
                                               (intptr_t) mainprogram->bdtexes[mainprogram->currbatch] > 0) - 1;
             i >= 0; i--) {
            int numquads = (intptr_t) mainprogram->bdtptr[i] - (intptr_t) mainprogram->bdtexes[i];
            int pos = 0;
            for (int j = 0; j < numquads; j++) {
                if (mainprogram->boxtexes[i][j] != -1) {
                    glActiveTexture(GL_TEXTURE0 + pos++);
                    glBindTexture(GL_TEXTURE_2D, mainprogram->boxtexes[i][j]);
                }
            }

            glBindBuffer(GL_TEXTURE_BUFFER, mainprogram->boxcoltbo);
            GLint size = 0;
            glGetBufferParameteriv(GL_TEXTURE_BUFFER, GL_BUFFER_SIZE, &size);
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
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, numquads * 6 * sizeof(unsigned short),
                            mainprogram->indices);

            glUniform1i(glbox, 1);
            glDrawElements(GL_TRIANGLES, numquads * 6, GL_UNSIGNED_SHORT, (GLvoid *) 0);
            glUniform1i(glbox, 0);
        }
        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);

        if (mainprogram->dragbinel && SDL_GetMouseFocus() != binsmain->win) {
            // draw texture of element being dragged
            float boxwidth = 0.3f;
            float nmx = mainprogram->xscrtovtx(mainprogram->mx) + boxwidth / 2.0f;
            float nmy = 2.0 - mainprogram->yscrtovtx(mainprogram->my) - boxwidth / 2.0f;
            mainprogram->frontbatch = true;
            draw_box(nullptr, black, -1.0f - 0.5 * boxwidth + nmx, -1.0f - 0.5 * boxwidth + nmy, boxwidth, boxwidth, mainprogram->dragbinel->tex);
            mainprogram->frontbatch = false;
        }

        // draw frontbatch one by one: lines, triangles, menus, drag tex
        mainprogram->directmode = true;
        for (int i = 0; i < mainprogram->guielems.size(); i++) {
            GUI_Element *elem = mainprogram->guielems[i];
            if (elem->type == GUI_LINE) {
                draw_line(elem->line);
                delete elem->line;
            }
            else if (elem->type == GUI_TRIANGLE) {
                draw_triangle(elem->triangle);
                delete elem->triangle;
            } else {
                if (!elem->box->circle && elem->box->text) {
                    glUniform1i(textmode, 1);
                }
                draw_box(elem->box->linec, elem->box->areac, elem->box->x, elem->box->y, elem->box->wi,
                         elem->box->he, 0.0f, 0.0f, 1.0f, 1.0f, elem->box->circle, elem->box->tex, glob->w, glob->h,
                         elem->box->text, elem->box->vertical, elem->box->inverted);
                if (!elem->box->circle && elem->box->text) glUniform1i(textmode, 0);
                delete elem->box;
            }
            delete elem;
        }
        mainprogram->directmode = false;
    }

    Layer *lay = mainmix->currlay[!mainprogram->prevmodus];

    // Handle colorbox
    mainprogram->pick_color(lay, lay->colorbox);
    if (lay->cwon) {
        lay->blendnode->chred = lay->rgb[0];
        lay->blendnode->chgreen = lay->rgb[1];
        lay->blendnode->chblue = lay->rgb[2];
    }

    if (!mainprogram->binsscreen && mainprogram->fullscreen == -1) {
        // background texture overlay fakes transparency
        draw_direct(nullptr, black, -1.0f, -1.0f, 2.0f, 2.0f, 0.0f, 0.0f, 1.0f, 0.2f, 0, mainprogram->bgtex,
                    glob->w, glob->h, false, false);
    }

    mainmix->reconnect_all(mainmix->layersA);
    mainmix->reconnect_all(mainmix->layersB);
    mainmix->reconnect_all(mainmix->layersAcomp);
    mainmix->reconnect_all(mainmix->layersBcomp);

    glFlush();   //reminder : test under Windows
    glFinish();
    //sleep(0.01f);
    SDL_GL_SwapWindow(mainprogram->mainwindow);

    mainprogram->ttreserved = false;
    mainprogram->boxhit = false;
    mainprogram->leftmouse = 0;
    mainprogram->binlmover = 0;
	mainprogram->orderleftmouse = 0;
	mainprogram->doubleleftmouse = 0;
    mainprogram->middlemouse = 0;
    mainprogram->middlemousedown = false;
	mainprogram->rightmouse = 0;
	mainprogram->openerr = false;

    for (Layer* lay : mainprogram->dellays) {
        delete lay;
    }
    mainprogram->dellays.clear();
    for (Effect* eff : mainprogram->deleffects) {
        mainprogram->nodesmain->currpage->delete_node(eff->node);
        delete eff;
    }
    mainprogram->deleffects.clear();
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
	GLuint smalltex = 0;
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
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glViewport(0, 0, tw, th);
	int sw, sh;
	glBindTexture(GL_TEXTURE_2D, tex);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &sw);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &sh);
	mainprogram->directmode = true;
	if (yflip) {
		draw_box(nullptr, black, -1.0f, -1.0f, 2.0f, 2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, tex, glob->w, glob->h, false);
	}
	else {
		draw_box(nullptr, black, -1.0f, -1.0f + 2.0f, 2.0f, -2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, tex, glob->w, glob->h, false);
	}
    mainprogram->directmode = false;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDrawBuffer(GL_BACK_LEFT);
	glViewport(0, 0, glob->w, glob->h);
	glDeleteFramebuffers(1, &dfbo);
	return smalltex;
}

void save_thumb(std::string path, GLuint tex) {
    int wi = mainprogram->ow3;
    int he = mainprogram->oh3;
    unsigned char *buf = (unsigned char*)malloc(wi * he * 3);

    GLuint tex2 = copy_tex(tex);
    GLuint texfrbuf, endfrbuf;
    glGenFramebuffers(1, &texfrbuf);
    glBindFramebuffer(GL_FRAMEBUFFER, texfrbuf);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex2, 0);
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
    glBindTexture(GL_TEXTURE_2D, tex2);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &sw);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &sh);
    glBlitNamedFramebuffer(texfrbuf, endfrbuf, 0, 0, sw, sh, 0, 0, wi, he, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(0, 0, wi, he, GL_RGB, GL_UNSIGNED_BYTE, buf);
    glDeleteTextures(1, &smalltex);
    glDeleteTextures(1, &tex2);
    glDeleteFramebuffers(1, &texfrbuf);
    glDeleteFramebuffers(1, &endfrbuf);

    const int JPEG_QUALITY = 75;
    const int COLOR_COMPONENTS = 3;
    long unsigned int _jpegSize = 0;
    unsigned char* _compressedImage = NULL; //!< Memory is allocated by tjCompress2 if _jpegSize == 0

    tjhandle _jpegCompressor = tjInitCompress();

    tjCompress2(_jpegCompressor, buf, wi, 0, he, TJPF_RGB,
                &_compressedImage, &_jpegSize, TJSAMP_444, JPEG_QUALITY,
                TJFLAG_FASTDCT);

    tjDestroy(_jpegCompressor);

    std::ofstream outfile(path, std::ios::binary | std::ios::ate);
    outfile.write(reinterpret_cast<const char *>(_compressedImage), _jpegSize);

    //to free the memory allocated by TurboJPEG (either by tjAlloc(),
    //or by the Compress/Decompress) after you are done working on it:
    tjFree(_compressedImage);
	free(buf);
	outfile.close();
}

void open_thumb(std::string path, GLuint tex) {
    const int COLOR_COMPONENTS = 3;

    std::ifstream infile(path, std::ios::binary | std::ios::ate);
    std::streamsize sz = infile.tellg();
    unsigned char *buf = (unsigned char*)malloc(sz);
    infile.seekg(0, std::ios::beg);
    infile.read((char*)buf, sz);

    long unsigned int _jpegSize = sz; //!< _jpegSize from above

    int jpegSubsamp, width, height, dummy;

    tjhandle _jpegDecompressor = tjInitDecompress();

    tjDecompressHeader3(_jpegDecompressor, buf, _jpegSize, &width, &height, &jpegSubsamp, &dummy);

    unsigned char *uncbuffer = (unsigned char*)malloc(width*height*COLOR_COMPONENTS); //!< will contain the decompressed image

    tjDecompress2(_jpegDecompressor, buf, _jpegSize, uncbuffer, width, 0/*pitch*/, height, TJPF_RGB, TJFLAG_FASTDCT);

    tjDestroy(_jpegDecompressor);

    glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, uncbuffer);

	free(buf);
	free(uncbuffer);
	infile.close();

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

	int recogcode = 20011975;  // for recognizing EWOCvj content files
    ofile.write((const char *)&recogcode, 4);
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
		if (fileSize == -1) continue;
		ifstream fileInput;
		fileInput.open(paths[i], ios::in | ios::binary);
		printf("path %s\n", paths[i].c_str());
		fflush(stdout);
		char *inputBuffer = new char[fileSize];
		fileInput.read(inputBuffer, fileSize);
		ofile.write(inputBuffer, fileSize);
		delete[] inputBuffer;
		fileInput.close();
	}
}

bool check_version(const std::string &path) {
	bool concat = true;
	ifstream bfile;
	bfile.open(path, ios::in | ios::binary);
	char *buffer = new char[7];
	bfile.read(buffer, 7);
	if (buffer[0] == 0x45 && buffer[1] == 0x57 && buffer[2] == 0x4F && buffer[3] == 0x43 && buffer[4] == 0x76 && buffer[5] == 0x6A && buffer[6] == 0x20) {
		concat = false;
	} 
	delete[] buffer;
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
        if (num != 20011975) {
            mainprogram->openerr = true;
            return "";
        }
        bfile.read((char *)&num, 4);
        std::vector<int> sizes;
        //num = _byteswap_ulong(num - 1) + 1;
        for (int i = 0; i < num; i++) {
            int size;
            bfile.read((char *)&size, 4);
            sizes.push_back(size);
        }
        ofstream ofile;
        outpath = find_unused_filename(basename(path), mainprogram->temppath, ".mainfile");
        ofile.open(outpath, ios::out | ios::binary);

        char *ibuffer = new char[sizes[0]];
        bfile.read(ibuffer, sizes[0]);
        ofile.write(ibuffer, sizes[0]);
        ofile.close();
        delete[] ibuffer;
        for (int i = 0; i < num - 1; i++) {
            ofstream ofile;
            ofile.open(outpath + "_" + std::to_string(i) + ".file", ios::out | ios::binary);
            if (sizes[i + 1] == -1) break;
            char *ibuffer = new char[sizes[i + 1]];
            bfile.read(ibuffer, sizes[i + 1]);
            ofile.write(ibuffer, sizes[i + 1]);
            ofile.close();
            delete[] ibuffer;
        }
    }
    bfile.close();
    if (concat) return(outpath);
    else return "";
}


void blacken(GLuint tex) {
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDrawBuffer(GL_BACK_LEFT);
}


void write_genmidi(ostream& wfile, LayMidi *lm) {
	wfile << "PLAY\n";
	wfile << std::to_string(lm->play->midi0);
	wfile << "\n";
	wfile << std::to_string(lm->play->midi1);
	wfile << "\n";
	wfile << lm->play->midiport;
	wfile << "\n";

	wfile << "BACKW\n";
	wfile << std::to_string(lm->backw->midi0);
	wfile << "\n";
	wfile << std::to_string(lm->backw->midi1);
	wfile << "\n";
	wfile << lm->backw->midiport;
	wfile << "\n";
	
	wfile << "BOUNCE\n";
	wfile << std::to_string(lm->bounce->midi0);
	wfile << "\n";
	wfile << std::to_string(lm->bounce->midi1);
	wfile << "\n";
	wfile << lm->bounce->midiport;
	wfile << "\n";

	wfile << "FRFORW\n";
	wfile << std::to_string(lm->frforw->midi0);
	wfile << "\n";
	wfile << std::to_string(lm->frforw->midi1);
	wfile << "\n";
	wfile << lm->frforw->midiport;
	wfile << "\n";

    wfile << "FRBACKW\n";
    wfile << std::to_string(lm->frbackw->midi0);
    wfile << "\n";
    wfile << std::to_string(lm->frbackw->midi1);
    wfile << "\n";
    wfile << lm->frbackw->midiport;
    wfile << "\n";

    wfile << "STOP\n";
    wfile << std::to_string(lm->stop->midi0);
    wfile << "\n";
    wfile << std::to_string(lm->stop->midi1);
    wfile << "\n";
    wfile << lm->stop->midiport;
    wfile << "\n";

    wfile << "LOOP\n";
    wfile << std::to_string(lm->loop->midi0);
    wfile << "\n";
    wfile << std::to_string(lm->loop->midi1);
    wfile << "\n";
    wfile << lm->loop->midiport;
    wfile << "\n";

    wfile << "SPEED\n";
	wfile << std::to_string(lm->speed->midi0);
	wfile << "\n";
	wfile << std::to_string(lm->speed->midi1);
	wfile << "\n";
	wfile << lm->speed->midiport;
	wfile << "\n";

	wfile << "SPEEDZERO\n";
	wfile << std::to_string(lm->speedzero->midi0);
	wfile << "\n";
	wfile << std::to_string(lm->speedzero->midi1);
	wfile << "\n";
	wfile << lm->speedzero->midiport;
	wfile << "\n";

	wfile << "OPACITY\n";
	wfile << std::to_string(lm->opacity->midi0);
	wfile << "\n";
	wfile << std::to_string(lm->opacity->midi1);
	wfile << "\n";
	wfile << lm->opacity->midiport;
	wfile << "\n";

	wfile << "FREEZE\n";
	wfile << std::to_string(lm->scratchtouch->midi0);
	wfile << "\n";
	wfile << std::to_string(lm->scratchtouch->midi1);
	wfile << "\n";
	wfile << lm->scratchtouch->midiport;
	wfile << "\n";

	wfile << "SCRATCH\n";
	wfile << std::to_string(lm->scratch->midi0);
	wfile << "\n";
	wfile << std::to_string(lm->scratch->midi1);
	wfile << "\n";
	wfile << lm->scratch->midiport;
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


    wfile << "CROSSFADEMIDI0\n";
    wfile << std::to_string(mainmix->crossfade->midi[0]);
    wfile << "\n";
    wfile << "CROSSFADEMIDI1\n";
    wfile << std::to_string(mainmix->crossfade->midi[1]);
    wfile << "\n";
    wfile << "CROSSFADEMIDIPORT\n";
    wfile << mainmix->crossfade->midiport;
    wfile << "\n";
    wfile << "CROSSFADECOMPMIDI0\n";
    wfile << std::to_string(mainmix->crossfadecomp->midi[0]);
    wfile << "\n";
    wfile << "CROSSFADECOMPMIDI1\n";
    wfile << std::to_string(mainmix->crossfadecomp->midi[1]);
    wfile << "\n";
    wfile << "CROSSFADECOMPMIDIPORT\n";
    wfile << mainmix->crossfadecomp->midiport;
    wfile << "\n";

	wfile << "WORMGATEMIDI0\n";
	wfile << std::to_string(mainprogram->wormgate1->midi[0]);
	wfile << "\n";
	wfile << "WORMGATEMIDI1\n";
	wfile << std::to_string(mainprogram->wormgate1->midi[1]);
	wfile << "\n";
	wfile << "WORMGATEMIDIPORT\n";
	wfile << mainprogram->wormgate1->midiport;
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

    /*wfile << "RECORDSMIDI0\n";
    wfile << std::to_string(mainmix->recbutS->midi[0]);
    wfile << "\n";
    wfile << "RECORDSMIDI1\n";
    wfile << std::to_string(mainmix->recbutS->midi[1]);
    wfile << "\n";
    wfile << "RECORDSMIDIPORT\n";
    wfile << mainmix->recbutS->midiport;
    wfile << "\n";*/
    wfile << "RECORDQMIDI0\n";
    wfile << std::to_string(mainmix->recbutQ->midi[0]);
    wfile << "\n";
    wfile << "RECORDQMIDI1\n";
    wfile << std::to_string(mainmix->recbutQ->midi[1]);
    wfile << "\n";
    wfile << "RECORDQMIDIPORT\n";
    wfile << mainmix->recbutQ->midiport;
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

    wfile << "SHELFMIDI\n";
    for (int m = 0; m < 2; m++) {
        for (int j = 0; j < 16; j++) {
            ShelfElement *elem = mainprogram->shelves[m]->elements[j];
            wfile << std::to_string(elem->button->midi[0]) + "\n";
            wfile << std::to_string(elem->button->midi[1]) + "\n";
            wfile << elem->button->midiport + "\n";
        }
    }
    wfile << "SHELFMIDIEND\n";



    wfile << "ENDOFFILE\n";
	wfile.close();
}

void open_genmidis(std::string path) {
	ifstream rfile;
	rfile.open(path);
	std::string istring;

	LayMidi *lm = nullptr;
	while (safegetline(rfile, istring)) {
		if (istring == "ENDOFFILE") break;
	
		if (istring == "LAYMIDIA") lm = laymidiA;
		if (istring == "LAYMIDIB") lm = laymidiB;
		if (istring == "LAYMIDIC") lm = laymidiC;
		if (istring == "LAYMIDID") lm = laymidiD;

        if (istring == "CROSSFADEMIDI0") {
            safegetline(rfile, istring);
            mainmix->crossfade->midi[0] = std::stoi(istring);
        }
        if (istring == "CROSSFADEMIDI1") {
            safegetline(rfile, istring);
            mainmix->crossfade->midi[1] = std::stoi(istring);
        }
        if (istring == "CROSSFADEMIDIPORT") {
            safegetline(rfile, istring);
            mainmix->crossfade->midiport = istring;
            mainmix->crossfade->register_midi();
        }
        if (istring == "CROSSFADECOMPMIDI0") {
            safegetline(rfile, istring);
            mainmix->crossfadecomp->midi[0] = std::stoi(istring);
        }
        if (istring == "CROSSFADECOMPMIDI1") {
            safegetline(rfile, istring);
            mainmix->crossfadecomp->midi[1] = std::stoi(istring);
        }
        if (istring == "CROSSFADECOMPMIDIPORT") {
            safegetline(rfile, istring);
            mainmix->crossfadecomp->midiport = istring;
            mainmix->crossfadecomp->register_midi();
        }

		if (istring == "PLAY") {
			safegetline(rfile, istring);
			lm->play->midi0 = std::stoi(istring);
			safegetline(rfile, istring);
			lm->play->midi1 = std::stoi(istring);
			safegetline(rfile, istring);
			lm->play->midiport= istring;
			lm->play->register_midi();
		}
		if (istring == "BACKW") {
			safegetline(rfile, istring);
			lm->backw->midi0 = std::stoi(istring);
			safegetline(rfile, istring);
			lm->backw->midi1 = std::stoi(istring);
			safegetline(rfile, istring);
			lm->backw->midiport= istring;
            lm->backw->register_midi();
		}
		if (istring == "BOUNCE") {
			safegetline(rfile, istring);
			lm->bounce->midi0 = std::stoi(istring);
			safegetline(rfile, istring);
			lm->bounce->midi1 = std::stoi(istring);
			safegetline(rfile, istring);
			lm->bounce->midiport= istring;
            lm->bounce->register_midi();
		}
		if (istring == "FRFORW") {
			safegetline(rfile, istring);
			lm->frforw->midi0 = std::stoi(istring);
			safegetline(rfile, istring);
			lm->frforw->midi1 = std::stoi(istring);
			safegetline(rfile, istring);
			lm->frforw->midiport= istring;
            lm->frforw->register_midi();
		}
        if (istring == "FRBACKW") {
            safegetline(rfile, istring);
            lm->frbackw->midi0 = std::stoi(istring);
            safegetline(rfile, istring);
            lm->frbackw->midi1 = std::stoi(istring);
            safegetline(rfile, istring);
            lm->frbackw->midiport= istring;
            lm->frbackw->register_midi();
        }
        if (istring == "STOP") {
            safegetline(rfile, istring);
            lm->stop->midi0 = std::stoi(istring);
            safegetline(rfile, istring);
            lm->stop->midi1 = std::stoi(istring);
            safegetline(rfile, istring);
            lm->stop->midiport = istring;
            lm->stop->register_midi();
        }
        if (istring == "LOOP") {
            safegetline(rfile, istring);
            lm->loop->midi0 = std::stoi(istring);
            safegetline(rfile, istring);
            lm->loop->midi1 = std::stoi(istring);
            safegetline(rfile, istring);
            lm->loop->midiport = istring;
            lm->loop->register_midi();
        }
		if (istring == "SPEED") {
			safegetline(rfile, istring);
			lm->speed->midi0 = std::stoi(istring);
			safegetline(rfile, istring);
			lm->speed->midi1 = std::stoi(istring);
			safegetline(rfile, istring);
			lm->speed->midiport = istring;
            lm->speed->register_midi();
		}
		if (istring == "SPEEDZERO") {
			safegetline(rfile, istring);
			lm->speedzero->midi0 = std::stoi(istring);
			safegetline(rfile, istring);
			lm->speedzero->midi1 = std::stoi(istring);
			safegetline(rfile, istring);
			lm->speedzero->midiport = istring;
            lm->speedzero->register_midi();
		}
		if (istring == "OPACITY") {
			safegetline(rfile, istring);
			lm->opacity->midi0 = std::stoi(istring);
			safegetline(rfile, istring);
			lm->opacity->midi1 = std::stoi(istring);
			safegetline(rfile, istring);
			lm->opacity->midiport = istring;
            lm->opacity->register_midi();
		}
		if (istring == "FREEZE") {
			safegetline(rfile, istring);
			lm->scratchtouch->midi0 = std::stoi(istring);
			safegetline(rfile, istring);
			lm->scratchtouch->midi1 = std::stoi(istring);
			safegetline(rfile, istring);
			lm->scratchtouch->midiport = istring;
            lm->scratchtouch->register_midi();
		}
		if (istring == "SCRATCH") {
			safegetline(rfile, istring);
			lm->scratch->midi0 = std::stoi(istring);
			safegetline(rfile, istring);
			lm->scratch->midi1 = std::stoi(istring);
			safegetline(rfile, istring);
			lm->scratch->midiport = istring;
            lm->scratch->register_midi();
		}
		
		if (istring == "WORMGATE0MIDI0") {
			safegetline(rfile, istring);
			mainprogram->wormgate1->midi[0] = std::stoi(istring);
		}
		if (istring == "WORMGATEMIDI1") {
			safegetline(rfile, istring);
			mainprogram->wormgate1->midi[1] = std::stoi(istring);
		}
		if (istring == "WORMGATEMIDIPORT") {
			safegetline(rfile, istring);
			mainprogram->wormgate1->midiport = istring;
            mainprogram->wormgate1->register_midi();
		}
		
		if (istring == "EFFCAT0MIDI0") {
			safegetline(rfile, istring);
			mainprogram->effcat[0]->midi[0] = std::stoi(istring);
		}
		if (istring == "EFFCAT0MIDI1") {
			safegetline(rfile, istring);
			mainprogram->effcat[0]->midi[1] = std::stoi(istring);
		}
		if (istring == "EFFCAT0MIDIPORT") {
			safegetline(rfile, istring);
			mainprogram->effcat[0]->midiport = istring;
            mainprogram->effcat[0]->register_midi();
		}
		if (istring == "EFFCAT1MIDI0") {
			safegetline(rfile, istring);
			mainprogram->effcat[1]->midi[0] = std::stoi(istring);
		}
		if (istring == "EFFCAT1MIDI1") {
			safegetline(rfile, istring);
			mainprogram->effcat[1]->midi[1] = std::stoi(istring);
		}
		if (istring == "EFFCAT1MIDIPORT") {
			safegetline(rfile, istring);
			mainprogram->effcat[1]->midiport = istring;
            mainprogram->effcat[1]->register_midi();
		}

        /*if (istring == "RECORDSMIDI0") {
            safegetline(rfile, istring);
            mainmix->recbutS->midi[0] = std::stoi(istring);
        }
        if (istring == "RECORDSMIDI1") {
            safegetline(rfile, istring);
            mainmix->recbutS->midi[1] = std::stoi(istring);
        }
        if (istring == "RECORDSMIDIPORT") {
            safegetline(rfile, istring);
            mainmix->recbutS->midiport = istring;
            mainmix->recbutS->register_midi();
        }*/
        if (istring == "RECORDQMIDI0") {
            safegetline(rfile, istring);
            mainmix->recbutQ->midi[0] = std::stoi(istring);
        }
        if (istring == "RECORDQMIDI1") {
            safegetline(rfile, istring);
            mainmix->recbutQ->midi[1] = std::stoi(istring);
        }
        if (istring == "RECORDQMIDIPORT") {
            safegetline(rfile, istring);
            mainmix->recbutQ->midiport = istring;
            mainmix->recbutQ->register_midi();
        }

		if (istring == "LOOPSTATIONMIDI") {
			LoopStation *ls;
			for (int i = 0; i < 2; i++) {
				if (i == 0) ls = lp;
				else ls = lpc;
				for (int j = 0; j < ls->numelems; j++) {
					safegetline(rfile, istring);
					if (istring == "LOOPSTATIONMIDI0:" + std::to_string(i) + ":" + std::to_string(j)) {
						safegetline(rfile, istring);
						ls->elems[j]->recbut->midi[0] = std::stoi(istring);
						safegetline(rfile, istring);
						ls->elems[j]->loopbut->midi[0] = std::stoi(istring);
						safegetline(rfile, istring);
						ls->elems[j]->playbut->midi[0] = std::stoi(istring);
						safegetline(rfile, istring);
						ls->elems[j]->speed->midi[0] = std::stoi(istring);
					}
					if (istring == "LOOPSTATIONMIDI1:" + std::to_string(i) + ":" + std::to_string(j)) {
						safegetline(rfile, istring);
						ls->elems[j]->recbut->midi[1] = std::stoi(istring);
						safegetline(rfile, istring);
						ls->elems[j]->loopbut->midi[1] = std::stoi(istring);
						safegetline(rfile, istring);
						ls->elems[j]->playbut->midi[1] = std::stoi(istring);
						safegetline(rfile, istring);
						ls->elems[j]->speed->midi[1] = std::stoi(istring);
					}
					if (istring == "LOOPSTATIONMIDIPORT:" + std::to_string(i) + ":" + std::to_string(j)) {
						safegetline(rfile, istring);
						ls->elems[j]->recbut->midiport = istring;
                        ls->elems[j]->recbut->register_midi();
						safegetline(rfile, istring);
						ls->elems[j]->loopbut->midiport = istring;
                        ls->elems[j]->loopbut->register_midi();
						safegetline(rfile, istring);
						ls->elems[j]->playbut->midiport = istring;
                        ls->elems[j]->playbut->register_midi();
						safegetline(rfile, istring);
						ls->elems[j]->speed->midiport = istring;
                        ls->elems[j]->speed->register_midi();
					}
				}
			}
		}

        if (istring == "SHELFMIDI") {
            for (int m = 0; m < 2; m++) {
                for (int j = 0; j < 16; j++) {
                    ShelfElement *elem = mainprogram->shelves[m]->elements[j];
                    safegetline(rfile, istring);
                    elem->button->midi[0] = std::stoi(istring);
                    safegetline(rfile, istring);
                    elem->button->midi[1] = std::stoi(istring);
                    safegetline(rfile, istring);
                    elem->button->midiport = istring;
                    elem->button->register_midi();
                }
            }
            safegetline(rfile, istring);
        }
	}
	rfile.close();
}




#ifdef WINDOWS
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow) {
    printf("YES\n");
    fflush(stdout);
#endif
#ifdef POSIX
int main(int argc, char* argv[]) {
#endif

    bool quit = false;

#ifdef WINDOWS
    HRESULT hr = SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
    if (FAILED(hr)) {
        _com_error err(hr);
        fwprintf(stderr, L"SetProcessDpiAwareness: %s\n", err.ErrorMessage());
    }
#endif


    // OPENAL
    /*const char *defaultDeviceName = alcGetString(nullptr, ALC_DEFAULT_DEVICE_SPECIFIER);
    ALCdevice *device = alcOpenDevice(defaultDeviceName);
    ALCcontext *context = alcCreateContext(device, nullptr);
    bool succes = alcMakeContextCurrent(context);*/

    // ALSA
    //snd_seq_t *seq_handle;
    //if (snd_seq_open(&seq_handle, "hw", SND_SEQ_OPEN_INPUT, 0) < 0) {
    //    fprintf(stderr, "Error opening ALSA sequencer.\n");
    //   exit(1);
    //}

    // initializing devIL
    ilInit();

    if (SDL_Init(SDL_INIT_VIDEO) < 0) /* Initialize SDL's Video subsystem */
        mainprogram->quitting = "Unable to initialize SDL"; /* Or die on error */
    //atexit(SDL_Quit);

    /* Request opengl 4.6 context.
     * SDL doesn't have the ability to choose which profile at this time of writing,
     * but it should default to the core profile */
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    /* Turn on double buffering */
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetSwapInterval(0); //vsync off

    SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");

    SDL_Rect rc;
    SDL_GetDisplayUsableBounds(0, &rc);
    auto sw = rc.w;
    auto sh = rc.h;

    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
    SDL_Window *win = SDL_CreateWindow(PROGRAM_NAME, 0, 0, sw, sh,
                                       SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS | SDL_WINDOW_SHOWN |
                                       SDL_WINDOW_ALLOW_HIGHDPI);

    glob = new Globals;
    int wi, he;
    SDL_GL_GetDrawableSize(win, &wi, &he);
    glob->w = (float) wi;
    glob->h = (float) he;
    SDL_DisplayMode DM;
    SDL_GetCurrentDisplayMode(0, &DM);

    mainprogram = new Program;
    printf("12\n");
    mainprogram->mainwindow = win;
    lp = new LoopStation;
    lpc = new LoopStation;
    printf("13\n");
    loopstation = lp;
    mainmix = new Mixer;
    printf("14\n");
    binsmain = new BinsMain;
    printf("15\n");
    retarget = new Retarget;
    printf("16\n");

    printf("1\n");

#ifdef WINDOWS
    boost::filesystem::path p5{mainprogram->docpath + "projects"};
    mainprogram->currprojdir = p5.generic_string();
    if (!exists(mainprogram->docpath + "/projects")) boost::filesystem::create_directory(p5);
#else
#ifdef POSIX
    std::string homedir(getenv("HOME"));
    mainprogram->homedir = homedir;
    boost::filesystem::path p1{homedir + "/Documents/EWOCvj2"};
    std::string docdir = p1.generic_string();
    if (!exists(homedir + "/Documents/EWOCvj2")) boost::filesystem::create_directory(p1);
    boost::filesystem::path e{homedir + "/.ewocvj2"};
    if (!exists(homedir + "/.ewocvj2")) boost::filesystem::create_directory(e);
    boost::filesystem::path p4{homedir + "/.ewocvj2/temp"};
    if (!exists(homedir + "/.ewocvj2/temp")) boost::filesystem::create_directory(p4);
    boost::filesystem::path p5{docdir + "/projects"};
    mainprogram->currprojdir = p5.generic_string();
    if (!exists(docdir + "/projects")) boost::filesystem::create_directory(p5);
#endif
#endif
    //empty temp dir if program crashed last time
    printf("2\n");
    boost::filesystem::path path_to_remove(mainprogram->temppath);
    for (boost::filesystem::directory_iterator end_dir_it, it(path_to_remove); it != end_dir_it; ++it) {
        boost::filesystem::remove_all(it->path());
    }
    printf("3\n");

    glc = SDL_GL_CreateContext(mainprogram->mainwindow);
    SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
    mainprogram->dummywindow = SDL_CreateWindow("Dummy", glob->w / 4, glob->h / 4, glob->w / 2,
                                               glob->h / 2, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN |
                                                       SDL_WINDOW_ALLOW_HIGHDPI);
    orderglc = SDL_GL_CreateContext(mainprogram->dummywindow);
    SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 0);
    printf("4\n");


    //glewExperimental = GL_TRUE;
    glewInit();

    printf("5\n");
    mainprogram->quitwindow = SDL_CreateWindow("Quit EWOCvj2", glob->w / 4, glob->h / 4, glob->w / 2,
                                                             glob->h / 2, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN |
                                                                          SDL_WINDOW_ALLOW_HIGHDPI);
    mainprogram->config_midipresetswindow = SDL_CreateWindow("Tune MIDI", glob->w / 4, glob->h / 4, glob->w / 2,
                                                             glob->h / 2, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN |
                                                                          SDL_WINDOW_ALLOW_HIGHDPI);
    mainprogram->prefwindow = SDL_CreateWindow("Preferences", glob->w / 4, glob->h / 4, glob->w / 2, glob->h / 2,
                                               SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_GL_GetDrawableSize(mainprogram->prefwindow, &wi, &he);
    smw = (float) wi;
    smh = (float) he;

    SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
    printf("6\n");
    mainprogram->ShaderProgram = mainprogram->set_shader();
    fflush(stdout);
    glUseProgram(mainprogram->ShaderProgram);

    mainprogram->shelves[0] = new Shelf(0);
    mainprogram->shelves[1] = new Shelf(1);
    mainprogram->shelves[0]->erase();
    mainprogram->shelves[1]->erase();
    mainprogram->cwbox->vtxcoords->w = glob->w / 5.0f;
    mainprogram->cwbox->vtxcoords->h = glob->h / 5.0f;
    mainprogram->cwbox->upvtxtoscr();


    if (FT_Init_FreeType(&ft)) {
        fprintf(stderr, "Could not init freetype library\n");
        return 1;
    }
    FT_UInt interpreter_version = 40;
    FT_Property_Set(ft, "truetype", "interpreter-version", &interpreter_version);
#ifdef WINDOWS
    std::string fstr = mainprogram->fontpath + "/expressway.ttf";
    if (!exists(fstr)) mainprogram->quitting = "Can't find \"expressway.ttf\" TrueType font in current directory";
#else
#ifdef POSIX
    std::string fdir(mainprogram->fontdir);
    std::string fstr = fdir + "/expressway.ttf";
    printf("%s /n", fstr.c_str());
    fflush(stdout);
    if (!exists(fdir + "/expressway.ttf"))
        mainprogram->quitting = "Can't find \"expressway.ttf\" TrueType font in " + fdir;
#endif
#endif
    if (FT_New_Face(ft, fstr.c_str(), 0, &face)) {
        fprintf(stderr, "Could not open font %s\n", fstr.c_str());
        return 1;
    }
    FT_Set_Pixel_Sizes(face, 0, 48);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    mainprogram->define_menus();

    mainprogram->nodesmain = new NodesMain;
    mainprogram->nodesmain->add_nodepages(8);
    mainprogram->nodesmain->currpage = mainprogram->nodesmain->pages[0];
    mainprogram->toscreenA->box->upvtxtoscr();
    mainprogram->toscreenB->box->upvtxtoscr();
    mainprogram->toscreenM->box->upvtxtoscr();
    mainprogram->backtopreA->box->upvtxtoscr();
    mainprogram->backtopreB->box->upvtxtoscr();
    mainprogram->backtopreM->box->upvtxtoscr();
    mainprogram->modusbut->box->upvtxtoscr();


    mainprogram->prefs = new Preferences;
    mainprogram->prefs->load();
    mainprogram->oldow = mainprogram->ow;
    mainprogram->oldoh = mainprogram->oh;

    GLint Sampler0 = glGetUniformLocation(mainprogram->ShaderProgram, "Sampler0");
    glUniform1i(Sampler0, 0);

    //temporary
    mainprogram->nodesmain->linked = true;

    MixNode *mixnodeA = mainprogram->nodesmain->currpage->add_mixnode(0, false);
    MixNode *mixnodeB = mainprogram->nodesmain->currpage->add_mixnode(0, false);
    MixNode *mixnodeAB = mainprogram->nodesmain->currpage->add_mixnode(0, false);
    mainprogram->bnodeend = mainprogram->nodesmain->currpage->add_blendnode(CROSSFADING, false);
    mainprogram->nodesmain->currpage->connect_nodes(mixnodeA, mixnodeB, mainprogram->bnodeend);
    mainprogram->nodesmain->currpage->connect_nodes(mainprogram->bnodeend, mixnodeAB);

    MixNode *mixnodeAcomp = mainprogram->nodesmain->currpage->add_mixnode(0, true);
    MixNode *mixnodeBcomp = mainprogram->nodesmain->currpage->add_mixnode(0, true);
    MixNode *mixnodeABcomp = mainprogram->nodesmain->currpage->add_mixnode(0, true);
    mainprogram->bnodeendcomp = mainprogram->nodesmain->currpage->add_blendnode(CROSSFADING, true);
    mainprogram->nodesmain->currpage->connect_nodes(mixnodeAcomp, mixnodeBcomp, mainprogram->bnodeendcomp);
    mainprogram->nodesmain->currpage->connect_nodes(mainprogram->bnodeendcomp, mixnodeABcomp);

    mainprogram->loadlay = new Layer(true);
    bool comp = !mainprogram->prevmodus;
    for (int m = 0; m < 2; m++) {
        std::vector<Layer *> &lvec = choose_layers(m);
        Layer *lay = mainmix->add_layer(lvec, 0);
        lay->filename = "";
    }
    for (int m = 0; m < 2; m++) {
        for (int i = 0; i < 4; i++) {
            mainmix->currscene[m] = i;
            mainmix->mousedeck = m;
            mainmix->do_save_deck(
                    mainprogram->temppath + "tempdecksc_" + std::to_string(m) + std::to_string(i) + ".deck", false,
                    false);
            mainmix->do_save_deck(
                    mainprogram->temppath + "tempdecksc_" + std::to_string(m) + std::to_string(i) + ".deck", false,
                    false);
        }
    }

    GLint endSampler0 = glGetUniformLocation(mainprogram->ShaderProgram, "endSampler0");
    glUniform1i(endSampler0, 1);
    GLint endSampler1 = glGetUniformLocation(mainprogram->ShaderProgram, "endSampler1");
    glUniform1i(endSampler1, 2);

    mainmix->currscene[0] = 0;
    mainmix->mousedeck = 0;
    mainmix->open_deck(mainprogram->temppath + "tempdecksc_00.deck", 1);
    mainmix->currscene[1] = 0;
    mainmix->mousedeck = 1;
    mainmix->open_deck(mainprogram->temppath + "tempdecksc_10.deck", 1);

    mainprogram->prevmodus = false;
    mainmix->currscene[0] = 0;
    mainmix->mousedeck = 0;
    mainmix->open_deck(mainprogram->temppath + "tempdecksc_00.deck", 1);
    mainmix->currscene[1] = 0;
    mainmix->mousedeck = 1;
    mainmix->open_deck(mainprogram->temppath + "tempdecksc_10.deck", 1);
    mainprogram->prevmodus = true;

    Layer *layA1 = mainmix->layersA[0];
    Layer *layB1 = mainmix->layersB[0];
    mainprogram->nodesmain->currpage->connect_nodes(layA1->node, mixnodeA);
    mainprogram->nodesmain->currpage->connect_nodes(layB1->node, mixnodeB);
    mainmix->currlay[0] = mainmix->layersA[0];
    mainmix->currlays[0].push_back(mainmix->currlay[0]);
    Layer *layA1comp = mainmix->layersAcomp[0];
    Layer *layB1comp = mainmix->layersBcomp[0];
    mainprogram->nodesmain->currpage->connect_nodes(layA1comp->node, mixnodeAcomp);
    mainprogram->nodesmain->currpage->connect_nodes(layB1comp->node, mixnodeBcomp);
    mainmix->currlay[1] = mainmix->layersAcomp[0];
    mainmix->currlays[1].push_back(mainmix->currlay[1]);
    make_layboxes();

    laymidiA = new LayMidi;
    laymidiB = new LayMidi;
    laymidiC = new LayMidi;
    laymidiD = new LayMidi;
    if (exists(mainprogram->docpath + "/midiset.gm")) open_genmidis(mainprogram->docpath + "midiset.gm");

    //mainmix->copy_to_comp(true);
    GLint preff = glGetUniformLocation(mainprogram->ShaderProgram, "preff");
    glUniform1i(preff, 1);

    // load background graphic
    ilEnable(IL_CONV_PAL);
    ILuint bg_ol;
    ilGenImages(1, &bg_ol);
    ilBindImage(bg_ol);
    ilActiveImage(0);
#ifdef WINDOWS
    ILboolean ret = ilLoadImage("./background.png");
#endif
#ifdef POSIX
    ILboolean ret = ilLoadImage("/usr/share/ewocvj2/background.png");
#endif
    if (ret == IL_FALSE) {
        printf("can't load background image\n");
    }
    int w = ilGetInteger(IL_IMAGE_WIDTH);
    int h = ilGetInteger(IL_IMAGE_HEIGHT);
    glGenTextures(1, &mainprogram->bgtex);
    glBindTexture(GL_TEXTURE_2D, mainprogram->bgtex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, (char *) ilGetData());

    // load background graphic
    ilEnable(IL_CONV_PAL);
    ILuint lok_ol;
    ilGenImages(1, &lok_ol);
    ilBindImage(lok_ol);
    ilActiveImage(0);
#ifdef WINDOWS
    ILboolean ret2 = ilLoadImage("./lock.png");
#endif
#ifdef POSIX
    ILboolean ret2 = ilLoadImage("/usr/share/ewocvj2/lock.png");
#endif
    if (ret2 == IL_FALSE) {
        printf("can't load lock image\n");
    }
    int w2 = ilGetInteger(IL_IMAGE_WIDTH);
    int h2 = ilGetInteger(IL_IMAGE_HEIGHT);
    glGenTextures(1, &mainprogram->loktex);
    glBindTexture(GL_TEXTURE_2D, mainprogram->loktex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w2, h2, 0, GL_RGBA, GL_UNSIGNED_BYTE, (char *) ilGetData());

    // get number of cores
#ifdef WINDOWS
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


    while (!done) {
        DWORD rc2 = glpi(buffer, &returnLength);

        if (FALSE == rc2) {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                if (buffer)
                    free(buffer);

                buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION) malloc(
                        returnLength);

                if (nullptr == buffer) {
                    printf(TEXT("\nError: Allocation failure\n"));
                    return (2);
                }
            } else {
                printf(TEXT("\nError %d\n"), GetLastError());
                return (3);
            }
        } else {
            done = TRUE;
        }
    }
    ptr = buffer;

    while (byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnLength) {
        if (ptr->Relationship == RelationProcessorCore) mainprogram->numcores++;
        byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
        ptr++;
    }
    free(buffer);
#endif

#ifdef POSIX
    mainprogram->numcores = sysconf(_SC_NPROCESSORS_ONLN);
#endif


    int oscport = 9000;


    // Multi-user code using sockets
    if (1) {
        //get local ip by connecting to Google DNS
        const char *google_dns_server = "8.8.8.8";
        int dns_port = 53;

        struct sockaddr_in serv;
        mainprogram->sock = socket(AF_INET, SOCK_DGRAM, 0);
#ifdef POSIX
        int flags = fcntl(new_socket, F_GETFL);
        fcntl(new_socket, F_SETFL, flags | O_NONBLOCK);
#endif
#ifdef WINDOWS
        u_long flags = 1;
        ioctlsocket(mainprogram->sock, FIONBIO, &flags);
#endif
        //Socket could not be created
        if (mainprogram->sock < 0) {
            std::cout << "Socket error" << std::endl;
        }

        memset(&serv, 0, sizeof(serv));
        serv.sin_family = AF_INET;
        serv.sin_addr.s_addr = inet_addr(google_dns_server);
        serv.sin_port = htons(dns_port);

        int err = connect(mainprogram->sock, (const struct sockaddr *) &serv, sizeof(serv));
        if (err < 0) {
            std::cout << "Error number: " << errno
                      << ". Error message: " << strerror(errno) << std::endl;
        }

        struct sockaddr_in name;
        socklen_t namelen = sizeof(name);
        err = getsockname(mainprogram->sock, (struct sockaddr *) &name, &namelen);

        char li[80];
        const char *p = inet_ntop(AF_INET, &name.sin_addr, li, 80);
        mainprogram->localip = li;
        if (p != NULL) {
            std::cout << "Local IP address is: " << mainprogram->localip << std::endl;
        } else {
            std::cout << "Error number: " << errno
                      << ". Error message: " << strerror(errno) << std::endl;
        }

        // socket communication for sharing bins one-way
        int opt = 1;
        char buf[1024] = {0};
        if ((mainprogram->sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            printf("\n Socket creation error \n");
            return -1;
        }

        mainprogram->serv_addr_server.sin_family = AF_INET;
        mainprogram->serv_addr_server.sin_addr.s_addr = INADDR_ANY;
        mainprogram->serv_addr_server.sin_port = htons(8000);
        mainprogram->serv_addr_client.sin_family = AF_INET;
        mainprogram->serv_addr_client.sin_addr.s_addr = INADDR_ANY;
        mainprogram->serv_addr_client.sin_port = htons(8000);

    }



    // OSC
    //mainprogram->st = new lo::ServerThread(oscport);
    //mainprogram->st->start();
    //mainprogram->add_main_oscmethods();


    io_service io;
    deadline_timer t2(io);
    Deadline2 d2(t2);
    CancelDeadline2 cd2(d2);
    std::thread thr2(cd2);
    thr2.detach();


    set_glstructures();

    std::chrono::high_resolution_clock::time_point begintime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed;

#ifdef WINDOWS
    std::string dir = mainprogram->docpath;
#endif
#ifdef POSIX
    std::string dir = homedir + "/.ewocvj2/";
#endif
    if (exists(dir + "recentprojectslist")) {
        std::ifstream rfile;
        rfile.open(dir + "recentprojectslist");
        std::string istring;
        safegetline(rfile, istring);
        while (safegetline(rfile, istring)) {
            if (istring == "ENDOFFILE") break;
            mainprogram->recentprojectpaths.push_back(istring);
        }
        rfile.close();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDrawBuffer(GL_BACK_LEFT);

    for (Box *box : allboxes) {
        // predraw all tooltips so no slowdowns will happen when stringtextures are initialized
        mainprogram->longtooltip_prepare(box);
    }
    mainprogram->collectingboxes = false;
    // predraw all pref names so no slowdowns will happen when prefs window is first opened
    for (PrefCat *cat : mainprogram->prefs->items) {
        render_text(cat->name, white, 99.0f, 99.0f, 0.00045f * 4.0, 0.00075f * 4.0, 1, 0);
        for (PrefItem *item : cat->items) {
            render_text(item->name, white, 99.0f, 99.0f, 0.0024f, 0.004f, 1, 0);
        }
    }


#ifdef WINDOWS
    SetCurrentDirectory((LPCTSTR)((pathtoplatform(mainprogram->docpath).c_str())));
#endif
#ifdef POSIX
    boost::filesystem::current_path(pathtoplatform(mainprogram->docpath));
#endif




    while (!quit) {

        io.poll();

        if (mainprogram->path != "") {
            if (mainprogram->pathto == "RETARGETFILE") {
                // open retargeted file
                (*(mainmix->newpaths))[mainmix->newpathpos] = mainprogram->path;
                check_stage();
                mainprogram->currfilesdir = dirname(mainprogram->path);
            } else if (mainprogram->pathto == "SEARCHDIR") {
                bool cond1 = (std::find(retarget->localsearchdirs.begin(), retarget->localsearchdirs.end(), mainprogram->path) == retarget->localsearchdirs.end());
                bool cond2 = (std::find(retarget->globalsearchdirs.begin(), retarget->globalsearchdirs.end(), mainprogram->path) == retarget->globalsearchdirs.end());
                if (cond1 && cond2) retarget->localsearchdirs.push_back(mainprogram->path);
                make_searchbox();
            } else if (mainprogram->pathto == "ADDSEARCHDIR") {
                bool cond1 = (std::find(retarget->globalsearchdirs.begin(), retarget->globalsearchdirs.end(),
                                        mainprogram->path) == retarget->globalsearchdirs.end());
                bool cond2 = exists(mainprogram->path);
                if (cond1 && cond2) retarget->globalsearchdirs.push_back(mainprogram->path);
            } else if (mainprogram->pathto == "OPENFILESLAYER") {
                std::vector<Layer *> &lvec = choose_layers(mainmix->mousedeck);
                std::string str(mainprogram->paths[0]);
                mainprogram->currfilesdir = dirname(str);
                mainprogram->filescount = 0;
                mainprogram->fileslay = mainprogram->loadlay;
                mainprogram->openfileslayers = true;
            } else if (mainprogram->pathto == "OPENFILESSTACK") {
                std::vector<Layer *> &lvec = choose_layers(mainmix->mousedeck);
                std::string str(mainprogram->paths[0]);
                if (!mainmix->addlay) {
                    mainprogram->loadlay = mainmix->add_layer(lvec, mainprogram->loadlay->pos);
                    mainprogram->loadlay->keepeffbut->value = 0;
                }
                else if (mainmix->mouselayer == nullptr) {
                    mainprogram->loadlay = mainmix->add_layer(lvec, lvec.size());
                    mainprogram->loadlay->keepeffbut->value = 0;
                }
                if (mainprogram->loadlay->filename == "") {
                    mainprogram->loadlay->keepeffbut->value = 0;
                }
                mainprogram->currfilesdir = dirname(str);
                mainprogram->filescount = 0;
                mainprogram->fileslay = mainprogram->loadlay;
                mainprogram->openfileslayers = true;
            } else if (mainprogram->pathto == "OPENFILESQUEUE") {
                std::string str(mainprogram->paths[0]);
                mainprogram->currfilesdir = dirname(str);
                mainprogram->filescount = 0;
                mainprogram->fileslay = mainprogram->loadlay;
                if (mainmix->addlay) {
                    std::vector<Layer*>& lvec = choose_layers(mainmix->mousedeck);
                    mainprogram->fileslay = mainmix->add_layer(lvec, lvec.size());
                    mainmix->addlay = false;
                }
                mainprogram->fileslay->cliploading = true;
                mainprogram->openfilesqueue = true;
            } else if (mainprogram->pathto == "CLIPOPENFILES") {
                std::string str(mainprogram->paths[0]);
                mainprogram->currfilesdir = dirname(str);
                mainprogram->clipfilescount = 0;
                mainprogram->clipfilesclip = mainmix->mouseclip;
                mainprogram->clipfileslay = mainmix->mouselayer;
                mainprogram->clipfileslay->cliploading = true;
                mainprogram->openclipfiles = true;
            } else if (mainprogram->pathto == "OPENSHELF") {
                mainprogram->currshelfdir = dirname(mainprogram->path);
                //mainprogram->project->save(mainprogram->project->path);
                mainmix->mouseshelf->open(mainprogram->path);
            } else if (mainprogram->pathto == "SAVESHELF") {
                mainprogram->currshelfdir = dirname(mainprogram->path);
                std::string ext = mainprogram->path.substr(mainprogram->path.length() - 6, std::string::npos);
                std::string src = mainprogram->project->shelfdir + mainmix->mouseshelf->basepath;
                std::string dest;
                if (ext != ".shelf") {
                    dest = mainprogram->path;
                    boost::filesystem::path p1{dest};
                    if (!boost::filesystem::exists(p1)) {
                        boost::filesystem::create_directory(p1);
                    }
                    copy_dir(src, dest);
                }
                else {
                    dest = dirname(mainprogram->path);
                    //if (basename(dest) != remove_extension(basename(mainprogram->path))) {
                    //    std::string dest2 = dest.substr(0, dest.size() - basename(dest).length() - 1) + remove_extension(basename(mainprogram->path));
                    //    boost::filesystem::path p2{dest2};
                    //    if (!boost::filesystem::exists(p2)) {
                    //        boost::filesystem::create_directory(p2);
                    //    }
                    //    copy_dir(dest,dest2);
                    //}
                    copy_dir(src, dest);
                }
                boost::filesystem::path p3{dest + "/" + mainmix->mouseshelf->basepath + ".shelf"};
                if (boost::filesystem::exists(p3)) {
                    boost::filesystem::remove(mainprogram->path);
                }
                mainmix->mouseshelf->save(dest + "/" + remove_extension(basename(mainprogram->path)) + ".shelf");
            }
            else if (mainprogram->pathto == "OPENFILESSHELF") {
                if (mainprogram->paths.size() > 0) {
                    mainprogram->currfilesdir = dirname(mainprogram->paths[0]);
                    mainprogram->openfilesshelf = true;
                    mainprogram->shelffilescount = 0;
                    mainprogram->shelffileselem = mainmix->mouseshelfelem;
                }
            }
            else if (mainprogram->pathto == "SAVESTATE") {
                mainprogram->currelemsdir = dirname(mainprogram->path);
                mainmix->do_save_state(mainprogram->path, false);
            }
            else if (mainprogram->pathto == "SAVEMIX") {
                mainprogram->currelemsdir = dirname(mainprogram->path);
                mainmix->save_mix(mainprogram->path);
            }
            else if (mainprogram->pathto == "SAVEDECK") {
                mainprogram->currelemsdir = dirname(mainprogram->path);
                mainmix->save_deck(mainprogram->path);
            }
            else if (mainprogram->pathto == "OPENDECK") {
                mainprogram->currelemsdir = dirname(mainprogram->path);
                mainmix->open_deck(mainprogram->path, 1);
            }
            else if (mainprogram->pathto == "SAVELAYFILE") {
                mainprogram->currelemsdir = dirname(mainprogram->path);
                mainmix->save_layerfile(mainprogram->path, mainmix->mouselayer, 1, 0);
            }
            /*else if (mainprogram->pathto == "OPENLAYFILE") {
                mainprogram->currelemsdir = dirname(mainprogram->path);
                Layer *l = mainmix->open_layerfile(mainprogram->path, mainmix->mouselayer, 1, 1);
                mainmix->mouselayer->set_inlayer(l, true);
            }*/
            else if (mainprogram->pathto == "OPENSTATE") {
                mainprogram->currelemsdir = dirname(mainprogram->path);
                mainmix->open_state(mainprogram->path);
            } else if (mainprogram->pathto == "OPENMIX") {
                mainprogram->currelemsdir = dirname(mainprogram->path);
                mainmix->open_mix(mainprogram->path, true);
            }
            else if (mainprogram->pathto == "OPENFILESBIN") {
                mainprogram->currfilesdir = dirname(mainprogram->path);
                binsmain->openfilesbin = true;
                binsmain->thumbtime = mainmix->time;
            }
            else if (mainprogram->pathto == "OPENBIN") {
                if (mainprogram->paths.size() > 0) {
                    mainprogram->currbinsdir = dirname(mainprogram->paths[0]);
                    binsmain->importbins = true;
                    binsmain->binscount = 0;
                }
            }else if (mainprogram->pathto == "SAVEBIN") {
                mainprogram->currbinsdir = dirname(mainprogram->path);
                binsmain->save_bin(mainprogram->path);
            }else if (mainprogram->pathto == "CHOOSEDIR") {
                mainprogram->choosedir = mainprogram->path + "/";
                //std::string driveletter1 = str.substr(0, 1);
                //std::string abspath = boost::filesystem::canonical(mainprogram->docpath).generic_string();
                //std::string driveletter2 = abspath.substr(0, 1);
                //if (driveletter1 == driveletter2) {
                //	mainprogram->choosedir = boost::filesystem::relative(str, mainprogram->docpath).generic_string() + "/";
                //}
                //else {
                //	mainprogram->choosedir = str + "/";
                //}
            }
            else if (mainprogram->pathto == "OPENAUTOSAVE") {
                std::string p = dirname(mainprogram->path);
                if (exists(mainprogram->path)) {
                    std::string bupp = mainprogram->project->path;
                    std::string bupn = mainprogram->project->name;
                    std::string bubd = mainprogram->project->binsdir;
                    std::string busd = mainprogram->project->shelfdir;
                    std::string burd = mainprogram->project->recdir;
                    std::string buad = mainprogram->project->autosavedir;
                    std::string bued = mainprogram->project->elementsdir;
                    mainprogram->project->open(mainprogram->path, true);
                    mainprogram->project->binsdir = bubd;
                    mainprogram->project->shelfdir = busd;
                    mainprogram->project->recdir = burd;
                    mainprogram->project->autosavedir = buad;
                    mainprogram->project->elementsdir = bued;
                    mainprogram->project->path = bupp;
                    mainprogram->project->name = bupn;
                }
            }
            else if (mainprogram->pathto == "NEWPROJECT") {
                mainprogram->currprojdir = dirname(mainprogram->path);
                mainmix->new_state();
                mainprogram->project->newp(mainprogram->path + "/" + basename(mainprogram->path));
            }
            else if (mainprogram->pathto == "OPENPROJECT") {
                std::string p = dirname(mainprogram->path);
                mainprogram->currprojdir = dirname(p.substr(0, p.length() - 1));
                if (exists(mainprogram->path)) {
                    mainprogram->project->open(mainprogram->path, false);
                }
            }
            else if (mainprogram->pathto == "SAVEPROJECT") {
                if (dirname(mainprogram->path) != "") {
                    std::string oldprdir = mainprogram->project->name;
                    mainprogram->currprojdir = dirname(mainprogram->path);
                    std::string ext = mainprogram->path.substr(mainprogram->path.length() - 7, std::string::npos);
                    std::string path2;
                    std::string str;
                    if (ext != ".ewocvj") {
                        str = mainprogram->path + "/" + basename(mainprogram->path) + ".ewocvj";
                        path2 = mainprogram->path;
                    }
                    else {
                        path2 = dirname(mainprogram->path.substr(0, mainprogram->path.size() - 7));
                        path2 = path2.substr(0, path2.size() - 1);
                        str = mainprogram->path;
                    }
                    mainprogram->path = str;
                    if (!exists(path2)) {
                        mainprogram->project->copy_dirs(path2);
                    } else {
                        mainprogram->project->delete_dirs(path2);
                        mainprogram->project->copy_dirs(path2);
                    }

                    // adapt autosave entries
                    std::unordered_map<std::string, std::string> smap;
                    // change autosave directory names
                    for (const auto& dirEnt : boost::filesystem::directory_iterator{mainprogram->project->autosavedir})
                    {
                        const auto& path = dirEnt.path();
                        auto pathstr = path.generic_string();
                        size_t start_pos = pathstr.find(oldprdir);
                        if (start_pos == std::string::npos) continue;
                        std::string newstr = pathstr;
                        newstr.replace(start_pos, oldprdir.length(), basename(path2));
                        smap[pathstr] = newstr;
                    }
                    std::unordered_map<std::string, std::string>::iterator it;
                    for (it = smap.begin(); it != smap.end(); it++) {
                        boost::filesystem::rename(it->first, it->second);
                    }
                    // change autosave project file names
                    smap.clear();
                    for (const auto& dirEnt : boost::filesystem::recursive_directory_iterator{mainprogram->project->autosavedir})
                    {
                        const auto& path = dirEnt.path();
                        auto pathstr = path.generic_string();
                        size_t start_pos = pathstr.find(oldprdir);
                        if (start_pos == std::string::npos) continue;
                        std::string newstr = pathstr;
                        newstr.replace(start_pos, oldprdir.length(), basename(path2));
                        smap[pathstr] = newstr;
                    }
                    for (it = smap.begin(); it != smap.end(); it++) {
                        boost::filesystem::rename(it->first, it->second);
                    }

                    for (int i = 0; i < mainprogram->project->autosavelist.size(); i++)
                    {
                        while (1) {
                            size_t start_pos = mainprogram->project->autosavelist[i].find(oldprdir);
                            if (start_pos == std::string::npos) break;
                            std::string newstr = mainprogram->project->autosavelist[i];
                            newstr.replace(start_pos, oldprdir.length(), basename(path2));
                            mainprogram->project->autosavelist[i] = newstr;
                        }
                    }
                    std::ofstream wfile;
                    wfile.open(mainprogram->project->autosavedir + "autosavelist");
                    wfile << "EWOCvj AUTOSAVELIST V0.1\n";
                    for (int i = 0; i < mainprogram->project->autosavelist.size(); i++) {
                        wfile << mainprogram->project->autosavelist[i];
                        wfile << "\n";
                    }
                    wfile << "ENDOFFILE\n";
                    wfile.close();

                    // save project
                    mainprogram->project->do_save(mainprogram->path, false);
                }
            }
            mainprogram->path = "";
            mainprogram->pathto = "";
        }
        // else mainprogram->blocking = false;

        mainprogram->mousewheel = 0;
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            //SDL_PumpEvents();
            if (e.type == SDL_WINDOWEVENT) {
                SDL_WindowEvent we = e.window;
                if (we.event == SDL_WINDOWEVENT_LEAVE) {
                    if (!mainprogram->prefon && !mainprogram->midipresets) {
                        // activate focus on window when its entered (for dragndrop between windows)
                        if (e.window.windowID == SDL_GetWindowID(mainprogram->mainwindow)) {
                            mainprogram->intopmenu = true;
                            if (binsmain->floating) SDL_SetWindowInputFocus(binsmain->win);
                        }
                        if (binsmain->floating) {
                            if (e.window.windowID == SDL_GetWindowID(binsmain->win)) {
                                SDL_SetWindowInputFocus(mainprogram->mainwindow);
                            }
                        }
                    }
                }
            }
            if (mainprogram->renaming != EDIT_NONE) {
                std::string old = mainprogram->inputtext;
                int c1 = mainprogram->cursorpos1;
                int c2 = mainprogram->cursorpos2;
                if (c2 < c1) {
                    std::swap(c1, c2);
                }
                if (e.type == SDL_TEXTINPUT) {
                    /* Add new text onto the end of our text */
                    if (!((e.text.text[0] == 'c' || e.text.text[0] == 'C') &&
                          (e.text.text[0] == 'v' || e.text.text[0] == 'V') && SDL_GetModState() & KMOD_CTRL)) {
                        if (c1 != -1) {
                            mainprogram->cursorpos0 = c1;
                        } else {
                            c1 = mainprogram->cursorpos0;
                            c2 = mainprogram->cursorpos0;
                        }
                        std::string part = mainprogram->inputtext.substr(c2, mainprogram->inputtext.length() - c2);
                        mainprogram->inputtext = mainprogram->inputtext.substr(0, c1) + e.text.text + part;
                        mainprogram->cursorpos0++;
                        mainprogram->cursorpos1 = -1;
                        mainprogram->cursorpos2 = -1;
                    }
                }
                if (e.type == SDL_KEYDOWN) {
                    //Handle cursor left/right
                    if (e.key.keysym.sym == SDLK_LEFT && mainprogram->cursorpos0 > 0) {
                        mainprogram->cursorpos0--;
                    }
                    if (e.key.keysym.sym == SDLK_RIGHT && mainprogram->cursorpos0 < mainprogram->inputtext.length()) {
                        mainprogram->cursorpos0++;
                    }
                    //Handle backspace
                    if (e.key.keysym.sym == SDLK_BACKSPACE && mainprogram->inputtext.length() > 0) {
                        if (c1 != -1) {
                            mainprogram->cursorpos0 = c1;
                        } else if (mainprogram->cursorpos0 != 0) {
                            mainprogram->cursorpos0--;
                            c1 = mainprogram->cursorpos0;
                            c2 = mainprogram->cursorpos0 + 1;
                        }
                        if (!((c1 == -1) && mainprogram->cursorpos0 == 0)) {
                            mainprogram->inputtext = mainprogram->inputtext.substr(0, c1) +
                                                     mainprogram->inputtext.substr(c2, mainprogram->inputtext.length() -
                                                                                       c2);
                        }
                        mainprogram->cursorpos1 = -1;
                        mainprogram->cursorpos2 = -1;
                    }
                    //Handle delete
                    if (e.key.keysym.sym == SDLK_DELETE && mainprogram->inputtext.length() > 0) {
                        if (c1 != -1) {
                            mainprogram->cursorpos0 = c1;
                        } else if (mainprogram->cursorpos0 != mainprogram->inputtext.length()) {
                            c1 = mainprogram->cursorpos0;
                            c2 = mainprogram->cursorpos0 + 1;
                        }
                        if (!((c1 == -1) && mainprogram->cursorpos0 == mainprogram->inputtext.length())) {
                            mainprogram->inputtext = mainprogram->inputtext.substr(0, c1) +
                                                     mainprogram->inputtext.substr(c2, mainprogram->inputtext.length() -
                                                                                       c2);
                        }
                        mainprogram->cursorpos1 = -1;
                        mainprogram->cursorpos2 = -1;
                    }
                        //Handle cut
                    else if (e.key.keysym.sym == SDLK_x && SDL_GetModState() & KMOD_CTRL) {
                        if (mainprogram->cursorpos1 != -1) {
                            SDL_SetClipboardText(mainprogram->inputtext.substr(c1, c2 - c1).c_str());
                            if (c1 != -1) {
                                mainprogram->cursorpos0 = c1;
                            } else if (mainprogram->cursorpos0 != 0) {
                                mainprogram->cursorpos0--;
                                c1 = mainprogram->cursorpos0;
                                c2 = mainprogram->cursorpos0 + 1;
                            }
                            mainprogram->inputtext = mainprogram->inputtext.substr(0, c1) +
                                                     mainprogram->inputtext.substr(c2, mainprogram->inputtext.length() -
                                                                                       c2);
                            mainprogram->cursorpos1 = -1;
                            mainprogram->cursorpos2 = -1;
                        }
                    }
                        //Handle copy
                    else if (e.key.keysym.sym == SDLK_c && SDL_GetModState() & KMOD_CTRL) {
                        if (c1 != -1) {
                            SDL_SetClipboardText(mainprogram->inputtext.substr(c1, c2 - c1).c_str());
                        }
                    }
                        //Handle paste
                    else if (e.key.keysym.sym == SDLK_v && SDL_GetModState() & KMOD_CTRL) {
                        if (c1 == -1) {
                            c1 = mainprogram->cursorpos0;
                            c2 = mainprogram->cursorpos0;
                        }
                        mainprogram->inputtext = mainprogram->inputtext.substr(0, c1) + SDL_GetClipboardText() +
                                                 mainprogram->inputtext.substr(c2,
                                                                               mainprogram->inputtext.length() - c2);

                        if (mainprogram->cursorpos1 != -1) {
                            mainprogram->cursorpos0 = mainprogram->cursorpos1;
                        }
                        mainprogram->cursorpos1 = -1;
                        mainprogram->cursorpos2 = -1;
                    }
                        //Handle return
                    else if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER) {
                        if (mainprogram->renaming == EDIT_PARAM) {
                            try {
                                mainmix->adaptnumparam->value = std::stof(mainprogram->inputtext);
                                if (mainmix->adaptnumparam->powertwo)
                                    mainmix->adaptnumparam->value = sqrt(mainmix->adaptnumparam->value);
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
                            GUIString *gs = mainprogram->prguitextmap[mainprogram->inputtext];
                            if (gs) {
                                for (int i = 0; i < gs->texturevec.size(); i++) {
                                    glDeleteTextures(1, &gs->texturevec[i]);
                                }
                                delete gs;
                                mainprogram->prguitextmap.erase(mainprogram->inputtext);
                            }
                        } else if (!mainprogram->midipresets) {
                            GUIString *gs = mainprogram->tmguitextmap[mainprogram->inputtext];
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
                } else if (mainprogram->renaming == EDIT_BINELEMNAME) {
                    binsmain->renamingelem->name = mainprogram->inputtext;
                } else if (mainprogram->renaming == EDIT_PARAM) {
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
                                    mainprogram->mixwindows[i]->sync.notify_all();
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
                    if (e.key.keysym.sym == SDLK_LCTRL || e.key.keysym.sym == SDLK_RCTRL) {
                        mainprogram->ctrl = true;
                    }
                    if (e.key.keysym.sym == SDLK_LSHIFT || e.key.keysym.sym == SDLK_RSHIFT) {
                        mainprogram->shift = true;
                    }
                    if (mainprogram->ctrl) {
                        if (e.key.keysym.sym == SDLK_s) {
                            mainprogram->pathto = "SAVESTATE";
                            std::thread filereq(&Program::get_outname, mainprogram, "Save state file",
                                                "application/ewocvj2-state", boost::filesystem::canonical(
                                            mainprogram->currelemsdir).generic_string());
                            filereq.detach();
                        }
                        if (e.key.keysym.sym == SDLK_o) {
                            mainprogram->pathto = "OPENSTATE";
                            std::thread filereq(&Program::get_inname, mainprogram, "Open state file",
                                                "application/ewocvj2-state", boost::filesystem::canonical(
                                            mainprogram->currelemsdir).generic_string());
                            filereq.detach();
                        }
                        if (e.key.keysym.sym == SDLK_n) {
                            mainmix->new_file(2, 1);
                        }
                    } else {
                        // loopstation keyboard shortcuts
                        if (e.key.keysym.sym == SDLK_r) {
                            // toggle record button for current loopstation element
                            loopstation->currelem->recbut->value = !loopstation->currelem->recbut->value;
                            //loopstation->currelem->recbut->oldvalue = !loopstation->currelem->recbut->value;
                        } else if (e.key.keysym.sym == SDLK_t) {
                            // toggle loop button for current loopstation element
                            loopstation->currelem->loopbut->value = !loopstation->currelem->loopbut->value;
                            //loopstation->currelem->loopbut->oldvalue = !loopstation->currelem->loopbut->value;
                        }
                        if (e.key.keysym.sym == SDLK_y) {
                            // toggle "one shot play" button for current loopstation element
                            loopstation->currelem->playbut->value = !loopstation->currelem->playbut->value;
                            //loopstation->currelem->playbut->oldvalue = !loopstation->currelem->playbut->value;
                        }
                        if (e.key.keysym.sym == SDLK_DOWN) {
                            int rowpos = loopstation->currelem->pos;
                            rowpos++;
                            if (rowpos > 7) {
                                rowpos = 0;
                            }
                            loopstation->currelem = loopstation->elems[rowpos];
                        }
                        if (e.key.keysym.sym == SDLK_UP) {
                            int rowpos = loopstation->currelem->pos;
                            rowpos--;
                            if (rowpos < 0) {
                                rowpos = 7;
                            }
                            loopstation->currelem = loopstation->elems[rowpos];
                        }

                        // video loop keyboard shortcuts
                        if (e.key.keysym.sym == SDLK_l) {
                            // set video loop start frame
                            Layer *lay = mainmix->currlay[!mainprogram->prevmodus];
                            lay->startframe->value = lay->frame;
                            if (lay->startframe->value > lay->endframe->value) lay->startframe->value = lay->endframe->value;
                        }
                        if (e.key.keysym.sym == SDLK_p) {
                            // set video loop start frame
                            Layer *lay = mainmix->currlay[!mainprogram->prevmodus];
                            lay->endframe->value = lay->frame;
                            if (lay->startframe->value > lay->endframe->value) lay->endframe->value = lay->startframe->value;
                        }
                    }
                }
                if (e.type == SDL_KEYUP) {
                    if (e.key.keysym.sym == SDLK_LCTRL || e.key.keysym.sym == SDLK_RCTRL) {
                        mainprogram->ctrl = false;
                    }
                    if (e.key.keysym.sym == SDLK_LSHIFT || e.key.keysym.sym == SDLK_RSHIFT) {
                        mainprogram->shift = false;
                    }
                    if (e.key.keysym.sym == SDLK_DELETE || e.key.keysym.sym == SDLK_BACKSPACE) {
                        mainprogram->del = 1;
                        if (mainmix->learn) {
                            if (mainmix->learnparam) {
                                mainmix->learnparam->midi[0] = -1;
                                mainmix->learnparam->midi[1] = -1;
                                mainmix->learnparam->midiport = "";
                                mainmix->learnparam->unregister_midi();
                            } else if (mainmix->learnbutton) {
                                mainmix->learnbutton->midi[0] = -1;
                                mainmix->learnbutton->midi[1] = -1;
                                mainmix->learnbutton->unregister_midi();
                            }
                            mainmix->learn = false;
                        } else mainprogram->del = 1;
                    }
                    if (e.key.keysym.sym == SDLK_ESCAPE) {
                        mainprogram->fullscreen = -1;
                        mainprogram->directmode = false;
                    } else if (e.key.keysym.sym == SDLK_SPACE) {
                        if (mainmix->currlay[!mainprogram->prevmodus]) {
                            for (int i = 0; i < mainmix->currlays[!mainprogram->prevmodus].size(); i++) {
                                if (mainmix->currlays[!mainprogram->prevmodus][i]->playbut->value) {
                                    mainmix->currlays[!mainprogram->prevmodus][i]->playkind = 0;
                                    mainmix->currlays[!mainprogram->prevmodus][i]->playbut->value = false;
                                } else if (mainmix->currlays[!mainprogram->prevmodus][i]->revbut->value) {
                                    mainmix->currlays[!mainprogram->prevmodus][i]->playkind = 1;
                                    mainmix->currlays[!mainprogram->prevmodus][i]->revbut->value = false;
                                } else if (mainmix->currlays[!mainprogram->prevmodus][i]->bouncebut->value == 1) {
                                    mainmix->currlays[!mainprogram->prevmodus][i]->playkind = 2;
                                    mainmix->currlays[!mainprogram->prevmodus][i]->bouncebut->value = false;
                                } else if (mainmix->currlays[!mainprogram->prevmodus][i]->bouncebut->value == 2) {
                                    mainmix->currlays[!mainprogram->prevmodus][i]->playkind = 3;
                                    mainmix->currlays[!mainprogram->prevmodus][i]->bouncebut->value = false;
                                } else {
                                    if (mainmix->currlays[!mainprogram->prevmodus][i]->playkind == 0) {
                                        mainmix->currlays[!mainprogram->prevmodus][i]->playbut->value = true;
                                    } else if (mainmix->currlays[!mainprogram->prevmodus][i]->playkind == 1) {
                                        mainmix->currlays[!mainprogram->prevmodus][i]->revbut->value = true;
                                    } else if (mainmix->currlays[!mainprogram->prevmodus][i]->playkind == 2) {
                                        mainmix->currlays[!mainprogram->prevmodus][i]->bouncebut->value = 1;
                                    } else if (mainmix->currlays[!mainprogram->prevmodus][i]->playkind == 3) {
                                        mainmix->currlays[!mainprogram->prevmodus][i]->bouncebut->value = 2;
                                    }
                                }
                            }
                        }
                    } else if (e.key.keysym.sym == SDLK_RIGHT) {
                          if (mainmix->currlay[!mainprogram->prevmodus]) {
                            for (int i = 0; i < mainmix->currlays[!mainprogram->prevmodus].size(); i++) {
                                mainmix->currlays[!mainprogram->prevmodus][i]->frame += 1;
                                if (mainmix->currlays[!mainprogram->prevmodus][i]->frame >= mainmix->currlays[!mainprogram->prevmodus][i]->numf) mainmix->currlays[!mainprogram->prevmodus][i]->frame = 0;
                            }
                        }
                    } else if (e.key.keysym.sym == SDLK_LEFT) {
                        if (mainmix->currlay[!mainprogram->prevmodus]) {
                            for (int i = 0; i < mainmix->currlays[!mainprogram->prevmodus].size(); i++) {
                                mainmix->currlays[!mainprogram->prevmodus][i]->frame -= 1;
                                if (mainmix->currlays[!mainprogram->prevmodus][i]->frame < 0) mainmix->currlays[!mainprogram->prevmodus][i]->frame = mainmix->currlays[!mainprogram->prevmodus][i]->numf - 1;
                            }
                        }
                    }
                }
            }

            if (SDL_GetMouseFocus() != mainprogram->prefwindow &&
                SDL_GetMouseFocus() != mainprogram->config_midipresetswindow) {
                //SDL_GetMouseState(&mainprogram->mx, &mainprogram->my);
            } else {
                //mainprogram->mx = -1;
                //mainprogram->my = -1;
            }

            if (e.type == SDL_MULTIGESTURE) {
                if (fabs(e.mgesture.dDist) > 0.002) {
                    mainprogram->mx = e.mgesture.x * glob->w;
                    mainprogram->my = e.mgesture.y * glob->h;
                    for (int i = 0; i < 2; i++) {
                        std::vector<Layer *> &lvec = choose_layers(i);
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
            else if (e.type == SDL_MOUSEMOTION) {
                mainprogram->mx = e.motion.x;
                mainprogram->my = e.motion.y;
                mainprogram->oldmx = mainprogram->mx;
                mainprogram->oldmy = mainprogram->my;
                mainprogram->binmx = mainprogram->mx;
                mainprogram->binmy = mainprogram->my;
                if (mainprogram->tooltipbox) {
                    mainprogram->tooltipmilli = 0.0f;
                    mainprogram->tooltipbox = nullptr;
                }
                if (mainmix->prepadaptparam) {
                    mainmix->adaptparam = mainmix->prepadaptparam;
                    mainmix->prepadaptparam = nullptr;
                }
            }
                //If user clicks the mouse
            else if (e.type == SDL_MOUSEBUTTONDOWN) {
                if (e.button.button == SDL_BUTTON_LEFT) {
                    mainprogram->leftmousedown = true;
                }
                if (e.button.button == SDL_BUTTON_MIDDLE) {
                    if (e.button.clicks == 2) mainprogram->doublemiddlemouse = true;
                    mainprogram->middlemousedown = true;
                }
                if (e.button.button == SDL_BUTTON_RIGHT) {
                    mainprogram->rightmousedown = true;
                }
            } else if (e.type == SDL_MOUSEBUTTONUP) {
                if (e.button.button == SDL_BUTTON_RIGHT && !mainmix->learn) {
                    for (int i = 0; i < mainprogram->menulist.size(); i++) {
                        mainprogram->menulist[i]->state = 0;
                        mainprogram->menulist[i]->menux = mainprogram->mx;
                        mainprogram->menulist[i]->menuy = mainprogram->my;
                    }
                    mainprogram->menuactivation = true;
                    mainprogram->menuondisplay = false;
                }
                if (e.button.button == SDL_BUTTON_LEFT) {
                    if (e.button.clicks == 2) {
                        mainprogram->doubleleftmouse = true;
                    }
                    else mainprogram->leftmouse = true;
                    mainprogram->leftmousedown = false;
                    mainmix->prepadaptparam = nullptr;
                }
                if (e.button.button == SDL_BUTTON_MIDDLE) {
                    mainprogram->middlemouse = true;
                }
                if (e.button.button == SDL_BUTTON_RIGHT) {
                    mainprogram->rightmouse = true;
                    mainprogram->rightmousedown = false;
                }
            } else if (e.type == SDL_MOUSEWHEEL) {
                mainprogram->mousewheel = e.wheel.y;
            }
        }


        float deepred[4] = {1.0, 0.0, 0.0, 1.0};
        float lightblue[] = {0.5f, 0.5f, 1.0f, 1.0f};
        glClearColor(0.2f, 0.2f, 0.2f, 0.2f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (!mainprogram->startloop) {
            //initial switch to live mode
            mainprogram->prevmodus = false;

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
            Box box;

            // handle starting with a new project on the drive
            box.acolor[3] = 1.0f;
            box.vtxcoords->x1 = -0.75;
            box.vtxcoords->y1 = 0.0f;
            box.vtxcoords->w = 0.5f;
            box.vtxcoords->h = 0.25f;
            box.upvtxtoscr();
            draw_box(box.lcolor, box.acolor, &box, -1);
            if (box.in()) {
                draw_box(white, lightblue, &box, -1);
                if (mainprogram->leftmouse) {
                    //start new project
                    std::string name = "Untitled_0";
                    std::string path;
                    int count = 0;
                    while (1) {
                        path = mainprogram->currprojdir + "/" + name;
                        if (!exists(path)) {
                            break;
                        }
                        count++;
                        name = remove_version(name) + "_" + std::to_string(count);
                    }
                    mainprogram->get_outname("New project", "application/ewocvj2-project",
                                             boost::filesystem::absolute(reqdir + "/" + name).generic_string
                                             ());
                    if (mainprogram->path != "") {
                        SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
                        mainprogram->project->newp(mainprogram->path + "/" + basename(mainprogram->path));
                        mainprogram->currprojdir = dirname(mainprogram->path);
                        mainprogram->path = "";
                        mainprogram->startloop = true;
                        mainprogram->newproject = true;
                    }
                }
            }
            if (!mainprogram->startloop) {
                render_text("New project", white, box.vtxcoords->x1 + 0.015f, box.vtxcoords->y1 + 0.15f, 0.001f,
                            0.0016f);
                //glBindFramebuffer(GL_FRAMEBUFFER, 0);
                //glDrawBuffer(GL_BACK_LEFT);
            }

            // handle opening an existing project on the drive
            box.vtxcoords->y1 = -0.25f;
            box.upvtxtoscr();
            draw_box(box.lcolor, box.acolor, &box, -1);
            if (box.in()) {
                draw_box(white, lightblue, &box, -1);
                if (mainprogram->leftmouse) {
                    mainprogram->get_inname("Open project", "application/ewocvj2-project", boost::filesystem::canonical(mainprogram->currprojdir).generic_string());
                    if (mainprogram->path != "") {
                        SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
                        mainprogram->project->open(mainprogram->path, false);
                        std::string p = dirname(mainprogram->path);
                        mainprogram->currprojdir = dirname(p.substr(0, p.length() - 1));
                        mainprogram->path = "";
                        mainprogram->startloop = true;
                    }
                }
            }
            if (!mainprogram->startloop) {
                render_text("Open project", white, box.vtxcoords->x1 + 0.015f, box.vtxcoords->y1 + 0.15f, 0.001f,
                            0.0016f);
                //glBindFramebuffer(GL_FRAMEBUFFER, 0);
                //glDrawBuffer(GL_BACK_LEFT);
            }

            box.vtxcoords->x1 = 0.0f;
            box.vtxcoords->y1 = 0.5f;
            box.vtxcoords->w = 0.95f;
            box.vtxcoords->h = 0.125f;
            box.upvtxtoscr();

            bool brk = false;
            // handle choosing recently used projects
            render_text("Recent project files:", white, box.vtxcoords->x1 + 0.015f,
                        box.vtxcoords->y1 + box.vtxcoords->h * 2.0f + 0.03f, 0.001f, 0.0016f);
            for (int i = 0; i < mainprogram->recentprojectpaths.size(); i++) {
                draw_box(box.lcolor, box.acolor, &box, -1);
                if (box.in()) {
                    draw_box(white, lightblue, &box, -1);
                    if (mainprogram->leftmouse) {
                        //SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
                        mainprogram->project->open(mainprogram->recentprojectpaths[i], false);
                        std::string p = dirname(mainprogram->recentprojectpaths[i]);
                        mainprogram->currprojdir = dirname(p.substr(0, p.length() - 1));
                        mainprogram->startloop = true;
                        brk = true;
                        mainprogram->leftmouse = false;
                        break;
                    }
                }
                render_text(remove_extension(basename(mainprogram->recentprojectpaths[i])), white,
                            box.vtxcoords->x1 + 0.015f, box.vtxcoords->y1 + 0.03f, 0.001f, 0.0016f);
                box.vtxcoords->y1 -= 0.125f;
                box.upvtxtoscr();
            }

            if (!brk) {
                // allow exiting with x icon during project setup
                draw_box(nullptr, deepred, 1.0f - 0.05f, 1.0f - 0.075f, 0.05f, 0.075f, -1);
                render_text("x", white, 0.966f, 1.019f - 0.075f, 0.0012f, 0.002f);
                if (mainprogram->my <= mainprogram->yvtxtoscr(0.075f) &&
                    mainprogram->mx > glob->w - mainprogram->xvtxtoscr(0.05f)) {
                    if (mainprogram->leftmouse) {
                        printf("stopped\n");
                        SDL_Quit();
                        exit(1);
                    }
                }

                mainprogram->leftmouse = false;

                SDL_GL_SwapWindow(mainprogram->mainwindow);
            }

        }

        if (mainprogram->startloop) {
            // update global timer iGlobalTime, used by some effects shaders
            std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
            elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - begintime);
            long long microcount = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
            mainmix->oldtime = mainmix->time;
            mainmix->time = microcount / 1000000.0f;
            GLint iGlobalTime = glGetUniformLocation(mainprogram->ShaderProgram, "iGlobalTime");
            glUniform1f(iGlobalTime, mainmix->time);


            if (mainprogram->newproject) {
                mainprogram->newproject = false;
                mainprogram->project->open(mainprogram->project->path, false);
                if (mainprogram->prevmodus) {
                    mainmix->currlay[0] = mainmix->layersA[0];
                    mainmix->currlays[0].push_back(mainmix->currlay[0]);
                } else {
                    mainmix->currlay[1] = mainmix->layersAcomp[0];
                    mainmix->currlays[1].push_back(mainmix->currlay[1]);
                }
            }


            the_loop();  // main loop




        }
    }

    mainprogram->quitting = "stop";

}