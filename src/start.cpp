#if defined(WIN32) && !defined(__linux__)
#define WINDOWS
#elif defined(__linux__) && !defined(WIN32)
#define POSIX
#endif


#include "boost/bind.hpp"
#include "boost/asio.hpp"
#include "boost/thread/thread.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
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
#include <filesystem>
#include <ios>
#include <vector>
#include <numeric>
#include <unordered_set>
#include <list>
#include <map>
#include <time.h>
#include <thread>
#include <mutex>
#include <stdexcept>
#include <iphlpapi.h>

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

#include "GL/glew.h"
#include "GL/gl.h"
#define FREEGLUT_STATIC
#define _LIB
#define FREEGLUT_LIB_PRAGMAS 0
#include "GL/freeglut.h"
#ifdef POSIX
#include <pthread.h>
#include <alsa/asoundlib.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif
#include "SDL2/SDL.h"
#include "SDL2/SDL_syswm.h"
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
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

// my own header
#include "program.h"

#define PROGRAM_NAME "EWOCvj"

#ifdef WINDOWS
#include <shellapi.h>
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
SDL_GLContext splashglc;
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

std::mutex dellayslock;

static GLuint mixvao;
static GLuint thmvbuf;
static GLuint thmvao;
static GLuint boxvao;
FT_Face face;
LayMidi *laymidiA;
LayMidi *laymidiB;
LayMidi *laymidiC;
LayMidi *laymidiD;
std::vector<Boxx*> allboxes;
int sscount = 0;
bool collectingboxes = true;  // during startup

using namespace boost::asio;
using namespace std;


#ifdef POSIX
void Sleep(int milliseconds) {
	sleep((float)milliseconds / 1000.0f);
}

void strcat_s(char* dest, const char* input) {
    strcat(dest, input);
}
#endif

bool exists(std::string name) {
    if (std::filesystem::exists(name)) {
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

bool rename(std::string source, std::string destination) {
    namespace fs = std::filesystem;
    try {
        // Check if source exists
        if (!fs::exists(source)) {
            printf("ERROR: Source does not exist: %s\n", source.c_str());
            return false;
        }

        // Perform rename
        fs::rename(source, destination);
        return true;

    } catch (const fs::filesystem_error& e) {
        printf("ERROR: Rename failed: %s (code: %s)\n", e.what(), e.code().message().c_str());
        return false;
    } catch (const std::exception& e) {
        printf("ERROR: Exception: %s\n", e.what());
        return false;
    } catch (...) {
        printf("ERROR: Unknown error during rename\n");
        return false;
    }
}

bool copy_file(const std::string& source_path, const std::string& destination_path) {
    // Validate input parameters
    if (source_path.empty()) {
        printf("Error: Source path cannot be empty\n");
        return false;
    }
    if (destination_path.empty()) {
        printf("Error: Destination path cannot be empty\n");
        return false;
    }

    try {
        // Check if source file exists
        if (!std::filesystem::exists(source_path)) {
            printf("Error: Source file does not exist: %s\n", source_path.c_str());
            return false;
        }

        // Create destination directory if it doesn't exist
        std::filesystem::path dest_dir = std::filesystem::path(destination_path).parent_path();
        if (!dest_dir.empty()) {
            std::filesystem::create_directories(dest_dir);
        }

        // Copy the file
        copy_file(
                source_path,
                destination_path,
                std::filesystem::copy_options::overwrite_existing
        );

        return true;
    }
    catch (const std::filesystem::filesystem_error& e) {
        printf("Error: File system error: %s\n", e.what());
        return false;
    }
    catch (const std::exception& e) {
        printf("Error: %s\n", e.what());
        return false;
    }
}

bool copy_dir(const std::string& source_path, const std::string& destination_path, bool recursive, bool overwrite) {
    // Validate input parameters
    if (source_path.empty()) {
        printf("Error: Source path cannot be empty\n");
        return false;
    }
    if (destination_path.empty()) {
        printf("Error: Destination path cannot be empty\n");
        return false;
    }

    try {
        // Check if source directory exists
        if (!std::filesystem::exists(source_path)) {
            printf("Error: Source directory does not exist: %s\n", source_path.c_str());
            return false;
        }

        // Check if source is a directory
        if (!std::filesystem::is_directory(source_path)) {
            printf("Error: Source is not a directory: %s\n", source_path.c_str());
            return false;
        }

        // Choose copy options based on recursion parameter
        std::filesystem::copy_options options = std::filesystem::copy_options::overwrite_existing;
        if (recursive) {
            options |= std::filesystem::copy_options::recursive;
        }

         if (overwrite) {
            options |= std::filesystem::copy_options::overwrite_existing;
         }
         else {
             options |= std::filesystem::copy_options::skip_existing;
         }

        // Copy the directory
        std::filesystem::copy(source_path, destination_path, options);

        return true;
    }
    catch (const std::filesystem::filesystem_error& e) {
        printf("Error: File system error: %s\n", e.what());
        return false;
    }
    catch (const std::exception& e) {
        printf("Error: %s\n", e.what());
        return false;
    }
}

bool check_permission(std::string directory) {
    namespace fs = std::filesystem;
    try {
        // Check if directory exists
        if (!fs::exists(directory)) {
            return false;
        }

        // Create a temporary file to test write permission
        fs::path temp_file = directory + "write_test_temp_file";
        std::ofstream test_file(temp_file);

        if (!test_file.is_open()) {
            return false;
        }

        test_file.close();
        fs::remove(temp_file);
        return true;

    } catch (...) {
        return false;
    }
}

std::string remove_version(std::string filename) {
	const size_t underscore_idx = filename.rfind('_');
	std::string str = filename.substr(filename.rfind('_') + 1);
    for (int i = 0; i < str.length(); i++) {
        std::string s = str.substr(i, 1);
        if (s != "0" && s != "1" && s != "2" && s != "3" && s != "4" && s != "5" && s != "6" && s != "7" && s != "8" && s != "9") {
            return filename;
        }
    }
	if (std::string::npos != underscore_idx)
	{
        for (int i = underscore_idx; i < filename.length(); i++) {
            filename.erase(i);
        }
	}
	return filename;
}

std::string pathtoplatform(std::string path) {
#ifdef WINDOWS
    std::replace(path.begin(), path.end(), '/', '\\');
#endif
#ifdef POSIX
    std::replace(path.begin(), path.end(), '\\', '/');
#endif
    return path;
}

std::string pathtoposix(std::string path) {
    std::replace(path.begin(), path.end(), '\\', '/');
    return path;
}

std::string getdocumentspath() {
    const char* home = std::getenv("HOME");
    if (!home) return "";

    std::string configPath = std::string(home) + "/.config/user-dirs.dirs";
    std::ifstream file(configPath);

    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            // Look for XDG_DOCUMENTS_DIR="$HOME/Documents"
            if (line.find("XDG_DOCUMENTS_DIR=") == 0) {
                size_t start = line.find('"');
                size_t end = line.rfind('"');

                if (start != std::string::npos && end != std::string::npos && start < end) {
                    start++; // Skip opening quote
                    std::string path = line.substr(start, end - start);

                    // Replace $HOME with actual home directory
                    if (path.find("$HOME/") == 0) {
                        path = std::string(home) + path.substr(5);
                    } else if (path.find("$HOME") == 0) {
                        path = std::string(home) + path.substr(5);
                    }

                    return path;
                }
            }
        }
    }

    // Fallback to default
    return std::string(home) + "/Documents";
}

bool isimage(std::string path) {
    // is this file an image?
    ILuint boundimage;
    ilGenImages(1, &boundimage);
    ilBindImage(boundimage);
    ILboolean ret = ilLoadImage((const ILstring)path.c_str());
    ilDeleteImages(1, &boundimage);
	return (bool)ret;
}


bool isvideo(std::string path) {
    // is this file a video?
    // watch out: deck and mix files are wrongly checked as MJPEG videos, so always check isvideo last, after mix and deck checks
    AVFormatContext *test = avformat_alloc_context();
    int r = avformat_open_input(&test, path.c_str(), nullptr, nullptr);
    if (r < 0) return false;
    if (avformat_find_stream_info(test, nullptr) < 0) {
        avformat_close_input(&test);
        return false;
    }
    int w = test->streams[0]->codecpar->width;
    int h = test->streams[0]->codecpar->height;
    avformat_close_input(&test);
    return true;
}

bool islayerfile(std::string path) {
    // is this file a layer file?
    std::string istring = mainprogram->get_typestring(path);

    if (istring.find("EWOCvj LAYERFILE") != std::string::npos) {
        return true;
    }
    return false;
}

bool isdeckfile(std::string path) {
    // is this file a deck file?
    // always check before checking isvideo
    std::string istring = mainprogram->get_typestring(path);

    if (istring.find("EWOCvj DECKFILE") != std::string::npos) {
        return true;
    }
    return false;
}

bool ismixfile(std::string path) {
    // is this file a mix file?
    // always check before checking isvideo
    std::string istring = mainprogram->get_typestring(path);

    if (istring.find("EWOCvj MIXFILE") != std::string::npos) {
        return true;
    }
    return false;
}


bool safegetline(std::istream& is, std::string &t)
{
    t.clear();

    if (!std::getline(is, t, '\n')) {
        return false;  // Failed to read
    }

    // Remove carriage returns
    t.erase(std::remove(t.begin(), t.end(), '\r'), t.end());

    // Add bounds checking
    if (t.length() > 10000) {  // Reasonable config line limit
        printf("ERROR: Config line too long (%zu chars): '%.50s...'\n",
               t.length(), t.c_str());
        t.clear();
        return false;
    }

    return true;
}


std::string find_unused_filename(std::string basename, std::string path, std::string extension) {
    // find an unused filename with a certain basename and extension
    // with _0, _1, _2, and so on added to the basename
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


void check_stage(int step) {
    // hop through retargeting stages when every newpath is processed
    mainmix->newpathpos += step;
    if (mainmix->newpathpos >= (*(mainmix->newpaths)).size()) {
        mainmix->retargetstage++;
        if (mainmix->retargetstage == 6) {
            mainmix->retargetstage = 0;
            mainmix->retargetingdone = true;
        }
        mainmix->newpathpos = 0;
    }
}

bool retarget_search() {
    // search for a file in the local and glob/
    // earch directories
    // a global search directory is kept in th/
    // e preferences and used at every retarget
    // a local search directory is only used once
    // if the file isn't found, the directories are checked for files with the same filesize, which most reasonably represents one certain file, that was probably renamed
    std::function<std::string(std::string)> search_directory = [&search_directory](std::string path) {
        std::string sn = (*(mainmix->newpaths))[mainmix->newpathpos];
        std::filesystem::directory_entry p1(path);
        for (std::filesystem::recursive_directory_iterator itr(p1, std::filesystem::directory_options::skip_permission_denied); itr != std::filesystem::recursive_directory_iterator(); ++itr)
        {
            if (std::filesystem::is_regular_file(itr->path())) {
                if (basename(itr->path().generic_string()) == basename(sn)) {
                    return itr->path().generic_string();
                }
            }
        };
        std::string s("");
        return s;
    };
    std::function<std::string(std::string)> search_directory_for_same_size = [&search_directory_for_same_size](std::string path) {
        std::filesystem::directory_entry p1(path);
        for (std::filesystem::recursive_directory_iterator itr(p1, std::filesystem::directory_options::skip_permission_denied); itr!=std::filesystem::recursive_directory_iterator(); ++itr)
        {
            if (std::filesystem::is_regular_file(itr->path())) {
                if (std::filesystem::file_size(itr->path()) == retarget->filesize) {
                    return itr->path().generic_string();
                }
            }
        };
        std::string s("");
        return s;
    };

    bool ret = false;
    for (std::string dirpath : retarget->searchdirs) {
        std::string retstr = search_directory(dirpath);
        if (retstr != "") {
            (*(mainmix->newpaths))[mainmix->newpathpos] = retstr;
            ret = true;
            break;
        }
    }
    if (!ret) {
        for (std::string dirpath: retarget->searchdirs) {
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
    // in this retargeting stage, newfound files are assigned to the correct structures
    // being a layer, a clip, a shelf element or a bin element
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
        // load the retrageted videos/images
        if (lay->type == ELEM_FILE || lay->type == ELEM_LAYER) {
            lay->transfered = true;
            lay->open_video(lay->frame, lay->filename, false, true);
        } else if (lay->type == ELEM_IMAGE) {
            float bufr = lay->frame;
            lay->transfered = true;
            lay->open_image(lay->filename, true, true);
            lay->frame = bufr;
        }
    }

    // set up retraget clips
    for (int i = 0; i < mainmix->newpathclips.size(); i++) {
        Clip *clip = mainmix->newpathclips[i];
        clip->path = mainmix->newclippaths[i];
        if (clip->path == "") continue;
        clip->insert(clip->layer, clip->layer->clips->end() - 1);
    }
    for (int i = 0; i < mainmix->newpathlayclips.size(); i++) {
        Clip *clip = mainmix->newpathlayclips[i];
        clip->path = mainmix->newcliplaypaths[i];
        mainmix->newpathcliplays[i]->filename = clip->path;
        clip->path = find_unused_filename("cliptemp_" + remove_extension(basename(clip->path)), mainprogram->temppath, ".layer");
        mainmix->save_layerfile(clip->path, mainmix->newpathcliplays[i], 0, 0);
        mainmix->newpathcliplays[i]->close();
        if (clip->path == "") continue;
        clip->insert(clip->layer, clip->layer->clips->end() - 1);
    }

    // set up retraget shelf elements
    for (int i = 0; i < mainmix->newpathshelfelems.size(); i++) {
        mainmix->newpathshelfelems[i]->path = mainmix->newshelfpaths[i];
        if (mainmix->newpathshelfelems[i]->path == "") {
            blacken(mainmix->newpathshelfelems[i]->tex);
        }
    }

    // set up retraget bin elements
    for (int i = 0; i < mainmix->newpathbinels.size(); i++) {
        mainmix->newpathbinels[i]->path = mainmix->newbinelpaths[i];
        mainmix->newpathbinels[i]->absjpath = mainmix->newbineljpegpaths[i];
        mainmix->newpathbinels[i]->reljpath = std::filesystem::relative(mainmix->newpathbinels[i]->absjpath,
                                                                 mainprogram->project->binsdir).generic_string();
        mainmix->newpathbinels[i]->jpegpath = mainmix->newpathbinels[i]->absjpath;
        if (mainmix->newpathbinels[i]->name != "") {
            if (mainmix->newpathbinels[i]->absjpath != "") {
                mainmix->newpathbinels[i]->bin->open_positions.emplace(mainmix->newpathbinels[i]->pos);
            }
            else {
                bool dummy = false;
            }
        }
        mainmix->newpathbinels[i]->relpath = std::filesystem::relative(mainmix->newbinelpaths[i], mainprogram->project->binsdir).generic_string();
        if (mainmix->newpathbinels[i]->path == "") {
            mainmix->newpathbinels[i]->name = "";
            blacken(mainmix->newpathbinels[i]->tex);
        }
    }

    mainmix->newlaypaths.clear();
    mainmix->newclippaths.clear();
    mainmix->newshelfpaths.clear();
    mainmix->newbinelpaths.clear();
    mainmix->newpathlayers.clear();
    mainmix->newpathclips.clear();
    mainmix->newpathshelfelems.clear();
    mainmix->newpathbinels.clear();

    check_stage(1);
}

void make_searchbox(bool val) {
    // initialize boxes used for searching during retargeting of files
    short j = retarget->searchboxes.size();
    Boxx *box = new Boxx;
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
    retarget->searchglobalbuttons[j]->value = val;
    Boxx *clbox = new Boxx;
    clbox->vtxcoords->x1 = 0.35f;
    clbox->vtxcoords->y1 = -0.175f - j * 0.1f;
    clbox->vtxcoords->w = 0.025f;
    clbox->vtxcoords->h = 0.05f;
    clbox->upvtxtoscr();
    clbox->tooltiptitle = "Delete path ";
    clbox->tooltip = "Leftclick to delete this path from the list. ";
    retarget->searchclearboxes.push_back(clbox);
}


void rec_frames() {

    if (!mainmix->donerec[0]) {
        // layer recording:
        // grab frames for in-place recording
#define BUFFER_OFFSET(i) ((char *)nullptr + (i))
        glBindBuffer(GL_PIXEL_PACK_BUFFER, mainmix->ioBuf);
        bool cbool;
        if (!mainprogram->prevmodus) {
            glBufferData(GL_PIXEL_PACK_BUFFER, (int)(mainprogram->ow[1] * mainprogram->oh[1]) * 4, nullptr, GL_DYNAMIC_READ);
            cbool = 1;
        }
        else {
            glBufferData(GL_PIXEL_PACK_BUFFER, (int)(mainprogram->ow[0] * mainprogram->oh[0]) * 4, nullptr, GL_DYNAMIC_READ);
            cbool = 0;
        }
        if (mainmix->reclay->effects[0].size() == 0) {
            glBindFramebuffer(GL_FRAMEBUFFER, mainmix->reclay->fbo);
        }
        else {
            glBindFramebuffer(GL_FRAMEBUFFER, mainmix->reclay->effects[0][mainmix->reclay->effects[0].size() - 1]->fbo);
        }
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        if (cbool) glReadPixels(0, 0, mainprogram->ow[1], (int)mainprogram->oh[1], GL_BGRA, GL_UNSIGNED_BYTE, BUFFER_OFFSET(0));
        else glReadPixels(0, 0, mainprogram->ow[0], (int)mainprogram->oh[0], GL_BGRA, GL_UNSIGNED_BYTE, BUFFER_OFFSET(0));
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
        // output recording:
        // grab frames for output recording
#define BUFFER_OFFSET(i) ((char *)nullptr + (i))
        glBindBuffer(GL_PIXEL_PACK_BUFFER, mainmix->ioBuf);
        glBufferData(GL_PIXEL_PACK_BUFFER, (int) (mainprogram->ow[1] * mainprogram->oh[1]) * 4, nullptr,
                     GL_DYNAMIC_READ);
        glBindFramebuffer(GL_FRAMEBUFFER, ((MixNode *) mainprogram->nodesmain->mixnodes[1][2])->mixfbo);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glReadPixels(0, 0, mainprogram->ow[1], (int)mainprogram->oh[1], GL_BGRA, GL_UNSIGNED_BYTE, BUFFER_OFFSET(0));
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
 }


void handle_midi(int deck, int midi0, int midi1, int midi2, std::string midiport) {
	// handle general MIDI layer controls: set values
    LayMidi *laymidi;
    std::vector<Layer*> &lvec = choose_layers(deck);
	for (int j = 0; j < lvec.size(); j++) {
        Param *par = nullptr;
        Button *but = nullptr;
        if (lvec[j]->genmidibut->value == 1) laymidi = laymidiA;
        else if (lvec[j]->genmidibut->value == 2) laymidi = laymidiB;
        else if (lvec[j]->genmidibut->value == 3) laymidi = laymidiC;
        else if (lvec[j]->genmidibut->value == 4) laymidi = laymidiD;
		if (1) {
            if (midi0 == laymidi->play->midi0 && midi1 == laymidi->play->midi1 && midi2 != 0 && midiport == laymidi->play->midiport) {
                lvec[j]->playbut->value = !lvec[j]->playbut->value;
                lvec[j]->revbut->value = false;
                lvec[j]->bouncebut->value = false;
                mainmix->midi2 = midi2;
                but = lvec[j]->playbut;
                lvec[j]->playbut->midistarttime = std::chrono::system_clock::now();
            }
            if (midi0 == laymidi->backw->midi0 && midi1 == laymidi->backw->midi1 && midi2 != 0 && midiport == laymidi->backw->midiport) {
                lvec[j]->revbut->value = !lvec[j]->revbut->value;
                lvec[j]->playbut->value = false;
                lvec[j]->bouncebut->value = false;
                mainmix->midi2 = midi2;
                but = lvec[j]->revbut;
                lvec[j]->revbut->midistarttime = std::chrono::system_clock::now();
            }
			if (midi0 == laymidi->pausestop->midi0 && midi1 == laymidi->pausestop->midi1 && midi2 != 0 && midiport == laymidi->pausestop->midiport) {
                lvec[j]->playbut->value = false;
                lvec[j]->revbut->value = false;
                lvec[j]->bouncebut->value = false;
                mainmix->midi2 = midi2;
                but = lvec[j]->stopbut;
                lvec[j]->stopbut->midistarttime = std::chrono::system_clock::now();
			}
            if (midi0 == laymidi->bounce->midi0 && midi1 == laymidi->bounce->midi1 && midi2 != 0 && midiport == laymidi->bounce->midiport) {
                lvec[j]->bouncebut->value = !lvec[j]->bouncebut->value;
                lvec[j]->playbut->value = false;
                lvec[j]->revbut->value = false;
                mainmix->midi2 = midi2;
                but = lvec[j]->bouncebut;
                lvec[j]->bouncebut->midistarttime = std::chrono::system_clock::now();
            }
            if (midi0 == laymidi->loop->midi0 && midi1 == laymidi->loop->midi1 && midi2 != 0 && midiport == laymidi->loop->midiport) {
                lvec[j]->lpbut->value = !lvec[j]->lpbut->value;
                mainmix->midi2 = midi2;
                but = lvec[j]->lpbut;
                lvec[j]->lpbut->midistarttime = std::chrono::system_clock::now();
            }
            if (midi0 == laymidi->scratch1->midi0 && midi1 == laymidi->scratch1->midi1 && midiport == laymidi->scratch1->midiport) {
                lvec[j]->scratch->value = ((float)midi2 - 64.0f) * (laymidi->scrinvert * 2 - 1) / 4.0f;
                mainmix->midi2 = midi2;
                par = lvec[j]->scratch;
                lvec[j]->scratch->midistarttime = std::chrono::system_clock::now();
                lvec[j]->scratch->midistarted = true;
            }
            if (midi0 == laymidi->scratch2->midi0 && midi1 == laymidi->scratch2->midi1 && midiport == laymidi->scratch2->midiport) {
                lvec[j]->scratch->value = ((float)midi2 - 64.0f) * (laymidi->scrinvert * 2 - 1) / 4.0f;
                mainmix->midi2 = midi2;
                par = lvec[j]->scratch;
                lvec[j]->scratch->midistarttime = std::chrono::system_clock::now();
                lvec[j]->scratch->midistarted = true;
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
				lvec[j]->scratchtouch->value = 1;
                mainmix->midi2 = midi2;
                par = lvec[j]->scratchtouch;
                lvec[j]->scratchtouch->midistarttime = std::chrono::system_clock::now();
                lvec[j]->scratchtouch->midistarted = true;
			}
			if (midi0 == laymidi->scratchtouch->midi0 && midi1 == laymidi->scratchtouch->midi1 && midi2 == 0 && midiport == laymidi->scratchtouch->midiport) {
				lvec[j]->scratchtouch->value = 0;
                mainmix->midi2 = midi2;
                par = lvec[j]->scratchtouch;
                lvec[j]->scratchtouch->midistarttime = std::chrono::system_clock::now();
                lvec[j]->scratchtouch->midistarted = true;
			}
			if (midi0 == laymidi->speed->midi0 && midi1 == laymidi->speed->midi1 && midiport == laymidi->speed->midiport) {
				int m2 = -(midi2 - 127);
				if (m2 < 64.0f) {
					lvec[j]->speed->value = 1.0f + (5.0f / 64.0f) * (m2 - 64.0f);
				}
				else {
					lvec[j]->speed->value = 0.0f + (1.0f / 64.0f) * m2;
				}
                mainmix->midi2 = midi2;
                par = lvec[j]->speed;
                lvec[j]->speed->midistarttime = std::chrono::system_clock::now();
                lvec[j]->speed->midistarted = true;
			}
			if (midi0 == laymidi->speedzero->midi0 && midi1 == laymidi->speedzero->midi1 && midiport == laymidi->speedzero->midiport) {
				lvec[j]->speed->value = 1.0f;
                mainmix->midi2 = midi2;
                par = lvec[j]->speed;
                lvec[j]->speed->midistarttime = std::chrono::system_clock::now();
                lvec[j]->speed->midistarted = true;
			}
            if (midi0 == laymidi->opacity->midi0 && midi1 == laymidi->opacity->midi1 && midiport == laymidi->opacity->midiport) {
                lvec[j]->opacity->value = (float)midi2 / 127.0f;
                mainmix->midi2 = midi2;
                par = lvec[j]->opacity;
                lvec[j]->opacity->midistarttime = std::chrono::system_clock::now();
                lvec[j]->opacity->midistarted = true;
            }
            if (mainprogram->prevmodus && midi0 == laymidi->crossfade->midi0 && midi1 == laymidi->crossfade->midi1 && midiport == laymidi->crossfade->midiport) {
                mainmix->crossfade->value = (float)midi2 / 127.0f;
                mainmix->midi2 = midi2;
                par = mainmix->crossfade;
                mainmix->crossfade->midistarttime = std::chrono::system_clock::now();
                mainmix->crossfade->midistarted = true;
            }
            if (!mainprogram->prevmodus && midi0 == laymidi->crossfade->midi0 && midi1 == laymidi->crossfade->midi1 && midiport == laymidi->crossfade->midiport) {
                mainmix->crossfadecomp->value = (float) midi2 / 127.0f;
                mainmix->midi2 = midi2;
                par = mainmix->crossfadecomp;
                mainmix->crossfadecomp->midistarttime = std::chrono::system_clock::now();
                mainmix->crossfadecomp->midistarted = true;
            }
            if (but) {
                for (int i = 0; i < loopstation->elements.size(); i++) {
                    if (loopstation->elements[i]->recbut->value) {
                        loopstation->elements[i]->add_button_automationentry(mainmix->midibutton);
                    }
                }
            }
            if (par) {
                for (int i = 0; i < loopstation->elements.size(); i++) {
                    if (loopstation->elements[i]->recbut->value) {
                        loopstation->elements[i]->add_param_automationentry(mainmix->midiparam);
                    }
                }
                mainprogram->uniformCache->setFloat(par->shadervar.c_str(), par->value);
            }
		}
	}
}

void midi_callback( double deltatime, std::vector< unsigned char > *message, void *userData )
{
    // MIDI sensing callback
  	unsigned int nBytes = message->size();
  	int midi0 = (int)message->at(0);
  	int midi1 = (int)message->at(1);
  	float midi2 = (int)message->at(2);
  	//printf("MIDI %d %d %f \n", midi0, midi1, midi2);
  	std::string midiport = ((PrefItem*)userData)->name;

      if (midi0 == mainmix->prevmidi0 && midi1 == mainmix->prevmidi1 && midi2 == 0) {
          // same control triggered: reset prevmidis
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
            // when learning MIDI setup for assignment to general MIDI layer controls:
            LayMidi *lm = nullptr;
            if (mainprogram->midipresetsset == 0) lm = laymidiA;
            else if (mainprogram->midipresetsset == 1) lm = laymidiB;
            else if (mainprogram->midipresetsset == 2) lm = laymidiC;
            else if (mainprogram->midipresetsset == 3) lm = laymidiD;
            bool bupm = mainprogram->prevmodus;
            switch (mainprogram->tmlearn) {
                case TM_NONE:
                    // nothing being learned
                    // set tmchoice to the particular control, when its MIDI assignment is triggered
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
                    else if (lm->scratch1->midi0 == midi0 && lm->scratch1->midi1 == midi1 &&
                             lm->scratch1->midiport == midiport)
                        mainprogram->tmchoice = TM_SCRATCH1;
                    else if (lm->scratch2->midi0 == midi0 && lm->scratch2->midi1 == midi1 &&
                             lm->scratch2->midiport == midiport)
                        mainprogram->tmchoice = TM_SCRATCH2;
                    else if (lm->crossfade->midi0 == midi0 && lm->crossfade->midi1 == midi1 &&
                             lm->crossfade->midiport == midiport)
                        mainprogram->tmchoice = TM_CROSS;
                    else mainprogram->tmchoice = TM_NONE;
                    return;
                    break;
                case TM_PLAY:
                    // all the following learn MIDI parameters for a certain control
                    // they are registered
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
                    //if (midi0 == 144) return;
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
                    //if (midi0 == 144) return;
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
                    if ((midi0 >= 144 && midi0 < 160)) return;
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
                    if (midi0 >= 176 && midi0 < 192) return;
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
                    lm->crossfade->midi0 = midi0;
                    lm->crossfade->midi1 = midi1;
                    lm->crossfade->midiport = midiport;
                    mainprogram->prevmodus = true;
                    lm->crossfade->register_midi();
                    lm->crossfade->midi0 = midi0;
                    lm->crossfade->midi1 = midi1;
                    lm->crossfade->midiport = midiport;
                    mainprogram->prevmodus = false;
                    lm->crossfade->register_midi();
                    mainprogram->tmlearn = TM_NONE;
                    break;
                case TM_FREEZE:
                    //if (midi0 >= 176 && midi0 < 192) return;
                    lm->scratchtouch->midi0 = midi0;
                    lm->scratchtouch->midi1 = midi1;
                    lm->scratchtouch->midiport = midiport;
                    mainprogram->prevmodus = true;
                    lm->scratchtouch->register_midi();
                    mainprogram->prevmodus = false;
                    lm->scratchtouch->register_midi();
                    mainprogram->tmlearn = TM_NONE;
                    break;
                case TM_SCRATCH1:
                    if ((midi0 >= 144 && midi0 < 160)) return;
                    lm->scratch1->midi0 = midi0;
                    lm->scratch1->midi1 = midi1;
                    lm->scratch1->midiport = midiport;
                    mainprogram->prevmodus = true;
                    lm->scratch1->register_midi();
                    mainprogram->prevmodus = false;
                    lm->scratch1->register_midi();
                    mainprogram->tmlearn = TM_NONE;
                    break;
                case TM_SCRATCH2:
                    if ((midi0 >= 144 && midi0 < 160)) {
                        mainprogram->scratch2phase = 1;
                        return;
                    };
                    if (mainprogram->scratch2phase == 1) {
                        lm->scratch2->midi0 = midi0;
                        lm->scratch2->midi1 = midi1;
                        lm->scratch2->midiport = midiport;
                        mainprogram->prevmodus = true;
                        lm->scratch2->register_midi();
                        mainprogram->prevmodus = false;
                        lm->scratch2->register_midi();
                        mainprogram->tmlearn = TM_NONE;
                        mainprogram->scratch2phase = 0;
                        break;
                    }
            }
            mainprogram->prevmodus = bupm;
            return;
        }
	}
	
	
  	if (mainmix->learn) {
        // learn MIDI controls for certain parameters
  		if ((midi0 >= 176 && midi0 < 192) && mainmix->learnparam && mainmix->learnparam->sliding) {
            if (mainmix->learnparam->name == "shiftx" && (midi2 == 0.0f || midi2 == 127.0f)) return;
            if (mainmix->learnparam->name == "wipex" && (midi2 == 0.0f || midi2 == 127.0f)) return;
            if (mainmix->learnparam->name == "shifty" || mainmix->learnparam->name == "wipey") {
                for (int m = 0; m < 2; m++) {
                    // m is the performance/preview mode
                    // learn both x and y for wipe position parameters
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
                    // learn both x and y for shift position parameters of layers
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
                // learn for both comp modes: preview and performance
			    bool bupm = mainprogram->prevmodus;
                mainprogram->prevmodus = true;
                mainmix->learnparam->register_midi();
                mainprogram->prevmodus = false;
                mainmix->learnparam->register_midi();
                mainmix->learndouble = false;
			}
            else mainmix->learnparam->register_midi();

            // make a MidiNode for the parameter: preparing for later
			mainmix->learnparam->node = new MidiNode;
			mainmix->learnparam->node->param = mainmix->learnparam;
			mainprogram->nodesmain->currpage->nodes.push_back(mainmix->learnparam->node);
			if (mainmix->learnparam->effect) {
                // and connect it to its effect when there is one
				mainprogram->nodesmain->currpage->connect_in2(mainmix->learnparam->node, mainmix->learnparam->effect->node);
			}

            if (mainmix->learnparam->name == "shiftx" || mainmix->learnparam->name == "wipex") {
                // advance to learning the y parameter of layer/output wipe and shift positions
                for (int m = 0; m < 2; m++) {
                    // output wipe
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
                        // layer wipe
                        if (mainmix->learnparam == lvec[i]->blendnode->wipex) {
                            mainmix->learnparam = lvec[i]->blendnode->wipey;
                            mainmix->learn = true;
                        }
                    }
                }
            }
		}
  		else if ((midi0 >= 176 && midi0 < 192) && mainmix->learnbutton) {
            // learn MIDI controls for buttons
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
reminder: IMPLEMENT someday in node view */
		else if ((midi0 >= 144 && midi0 < 160) && midi2 != 0 && mainmix->learnbutton) {
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
  	
  	
  	if ((midi0 >= 176 && midi0 < 192) || (midi0 >= 144 && midi0 < 160)) {
        // learnMIDI controls for loopstation buttons
        Button *but = mainmix->midi_registrations[!mainprogram->prevmodus][midi0][midi1][midiport].but;
        if (but) {
            if (midi2 != 0) {
                if (mainprogram->sameeight) {
                    for (int i = 0; i < 8; i++) {
                        if (but == loopstation->elements[i]->recbut) {
                            but = loopstation->elements[i + loopstation->scrpos]->recbut;
                            break;
                        }
                        if (but == loopstation->elements[i]->loopbut) {
                            but = loopstation->elements[i + loopstation->scrpos]->loopbut;
                            break;
                        }
                        if (but == loopstation->elements[i]->playbut) {
                            but = loopstation->elements[i + loopstation->scrpos]->playbut;
                            break;
                        }
                    }
                }
                mainmix->midi2 = midi2;
                mainmix->midibutton = but;
                but->midistarttime = std::chrono::system_clock::now();
            }
        }
        // learn MIDI controls for sending shelf elements to the mix
		for (int m = 0; m < 2; m++) {
			for (int i = 0; i < 16; i++) {
				Button *but = mainprogram->shelves[m]->buttons[i];
				if (midi0 == but->midi[0] && midi1 == but->midi[1] && midi2 != 0 && midiport == but->midiport) {
                    if (mainprogram->shelves[m]->elements[i]->path != "") {
                        mainmix->midi2 = midi2;
                        mainmix->midibutton = nullptr;
                        mainmix->midishelfbutton = but;
                        but->midistarttime = std::chrono::system_clock::now();
                        mainmix->midibutton = nullptr;
                    }
				}
			}
		}

        Param *par = mainmix->midi_registrations[!mainprogram->prevmodus][midi0][midi1][midiport].par;
        if (par) {
            // set loopstation speed parameter
            if (mainprogram->sameeight) {
                for (int i = 0; i < 8; i++) {
                    if (par == loopstation->elements[i]->speed) {
                        par = loopstation->elements[i + loopstation->scrpos]->speed;
                        break;
                    }
                }
            }
            // set midiparam, handled in midi_set()
            mainmix->midi2 = midi2;
            mainmix->midiparam = par;
            par->midistarttime = std::chrono::system_clock::now();
            par->midistarted = true;
        }
	}




	LayMidi *lm = nullptr;
    // handling general MIDI
	handle_midi(0, midi0, midi1, midi2, midiport);
    handle_midi(1, midi0, midi1, midi2, midiport);
}






Shelf::Shelf(bool side) {
    // make a new shelf with 16 elements and respective overlayed button control elements
	this->side = side;
	for (int i = 0; i < 16; i++) {
 		this->buttons.push_back(new Button(false));
		this->elements.push_back(new ShelfElement(side, i, this->buttons.back()));
	}
    std::vector<std::string> vec;
    mainprogram->shelfjpegpaths[this] = false;
}

ShelfElement::ShelfElement(bool side, int pos, Button *but) {
    // make a new shelf element with its textures, buttons and boxes
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
	Boxx* box = this->button->box;
	box->vtxcoords->x1 = -1.0f + (pos % 4) * boxwidth + (2.0f - boxwidth * 4) * side;
	box->vtxcoords->h = boxwidth * (glob->w / glob->h) / (1920.0f / 1080.0f);
	box->vtxcoords->y1 = -1.0f + (int)(3 - (pos / 4)) * box->vtxcoords->h;
	box->vtxcoords->w = boxwidth;
	box->upvtxtoscr();
	box->tooltiptitle = "Video launch shelf";
	box->tooltip = "Shelf containing up to 16 videos/layerfiles/decks/mixes for quick and easy video launching.  Left drag'n'drop to/from other areas.  Doubleclick left loads the shelf element contents into all selected layers/decks. Rightclick launches shelf menu. ";
	// boxes that set the behaviour of multiple sends to the same mix element, interleaved with sending other elements to it
    this->sbox = new Boxx;
	this->sbox->vtxcoords->x1 = box->vtxcoords->x1;
	this->sbox->vtxcoords->y1 = box->vtxcoords->y1 + 0.05f + 0.009f;
	this->sbox->vtxcoords->w = 0.0075f;
	this->sbox->vtxcoords->h = 0.018f;
	this->sbox->upvtxtoscr();
	this->sbox->tooltiptitle = "Restart when triggered";
	this->sbox->tooltip = "When this video is put in the mix, either through MIDI or dragging, the video will restart from the beginning. ";
	this->pbox = new Boxx;
	this->pbox->vtxcoords->x1 = box->vtxcoords->x1;
	this->pbox->vtxcoords->y1 = box->vtxcoords->y1 + 0.05f - 0.009f;
	this->pbox->vtxcoords->w = 0.0075f;
	this->pbox->vtxcoords->h = 0.018f;
	this->pbox->upvtxtoscr();
	this->pbox->tooltiptitle = "Continue when triggered";
	this->pbox->tooltip = "When this video is put in the mix, either through MIDI or dragging, the video will continue from where it was last stopped . ";
	this->cbox = new Boxx;
	this->cbox->vtxcoords->x1 = box->vtxcoords->x1;
	this->cbox->vtxcoords->y1 = box->vtxcoords->y1 + 0.05f - 0.027f;
	this->cbox->vtxcoords->w = 0.0075f;
	this->cbox->vtxcoords->h = 0.018f;
	this->cbox->upvtxtoscr();
	this->cbox->tooltiptitle = "Catch up when triggered";
	this->cbox->tooltip = "When this video is put in the mix, either through MIDI or dragging, the video will continue from where it would have been if it had been virtually continuously playing . ";
}


void set_glstructures() {

    // set up the main program GL structures, used everywhere

	// tbo for GL quad color transfer to shader
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &mainprogram->maxtexes);
	glGenBuffers(1, &mainprogram->boxcoltbo);
	glBindBuffer(GL_TEXTURE_BUFFER, mainprogram->boxcoltbo);
	glBufferData(GL_TEXTURE_BUFFER, 2048 * 4, nullptr, GL_DYNAMIC_DRAW);
	glGenBuffers(1, &mainprogram->boxtextbo);
	glBindBuffer(GL_TEXTURE_BUFFER, mainprogram->boxtextbo);
	glBufferData(GL_TEXTURE_BUFFER, 2048, nullptr, GL_DYNAMIC_DRAW);
	mainprogram->uniformCache->setSampler("boxcolSampler", mainprogram->maxtexes - 2);
	glActiveTexture(GL_TEXTURE0 + mainprogram->maxtexes - 2);
	glGenTextures(1, &mainprogram->bdcoltex);
	glBindTexture(GL_TEXTURE_BUFFER, mainprogram->bdcoltex);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA8, mainprogram->boxcoltbo);
    glBindTexture(GL_TEXTURE_BUFFER, 0);
	mainprogram->uniformCache->setSampler("boxtexSampler", mainprogram->maxtexes - 1);
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
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, mainprogram->ow[0], mainprogram->oh[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glBindTexture(GL_TEXTURE_2D, mainprogram->fbotex[1]);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, mainprogram->ow[0], mainprogram->oh[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glBindTexture(GL_TEXTURE_2D, mainprogram->fbotex[2]);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, mainprogram->ow[1], mainprogram->oh[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glBindTexture(GL_TEXTURE_2D, mainprogram->fbotex[3]);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, mainprogram->ow[1], mainprogram->oh[1]);
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
    glBufferData(GL_ARRAY_BUFFER, 32, vcoords, GL_DYNAMIC_DRAW);
	GLuint tbuf;
    glGenBuffers(1, &tbuf);
	glBindBuffer(GL_ARRAY_BUFFER, tbuf);
    glBufferData(GL_ARRAY_BUFFER, 32, tcoords, GL_DYNAMIC_DRAW);
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

	GLfloat tcoords2[8];
	p = tcoords2;
	*p++ = 0.0f; *p++ = 1.0f;
	*p++ = 1.0f; *p++ = 1.0f;
	*p++ = 0.0f; *p++ = 0.0f;
	*p++ = 1.0f; *p++ = 0.0f;

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

    SDL_GL_MakeCurrent(mainprogram->requesterwindow, glc);
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

    SDL_GL_MakeCurrent(mainprogram->splashwindow, glc);
    glGenBuffers(1, &mainprogram->splboxvbuf);
    glGenBuffers(1, &mainprogram->splboxtbuf);
    glGenVertexArrays(1, &mainprogram->splboxvao);
    glBindVertexArray(mainprogram->splboxvao);
    glBindBuffer(GL_ARRAY_BUFFER, mainprogram->splboxvbuf);
    glBufferData(GL_ARRAY_BUFFER, 48, vcoords, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12, nullptr);
    glBindBuffer(GL_ARRAY_BUFFER, mainprogram->splboxtbuf);
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
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 12288, nullptr, GL_DYNAMIC_DRAW);

    glGenTextures(1, &mainmix->minitex);
    glBindTexture(GL_TEXTURE_2D, mainmix->minitex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 192, 108, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
    //glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, mainprogram->ow[0], mainprogram->oh[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    
    
    // gradient textures for ffgl parameter types red green blue visualisation
    glViewport(0, 0, 256, 32);
    mainprogram->uniformCache->setBool("redoption", true);
    glGenTextures(1, &mainprogram->redgradienttex);
    glBindTexture(GL_TEXTURE_2D, mainprogram->redgradienttex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 256, 32);
    glGenFramebuffers(1, &mainprogram->redgradientfbo);
    glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->redgradientfbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mainprogram->redgradienttex, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    draw_direct(nullptr, red, -1.0f, -1.0f, 2.0f, 2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, mainprogram->fbotex[0], glob->w, glob->h, false, false);  // initializes mainpogram->btbuf
    draw_direct(nullptr, red, -1.0f, -1.0f, 2.0f, 2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, -1, glob->w, glob->h, false, false);
    mainprogram->uniformCache->setBool("redoption", false);

    mainprogram->uniformCache->setBool("greenoption", true);
    glGenTextures(1, &mainprogram->greengradienttex);
    glBindTexture(GL_TEXTURE_2D, mainprogram->greengradienttex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 256, 32);
    glGenFramebuffers(1, &mainprogram->greengradientfbo);
    glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->greengradientfbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mainprogram->greengradienttex, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    draw_direct(nullptr, black, -1.0f, -1.0f, 2.0f, 2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, -1, glob->w, glob->h, false, false);
    mainprogram->uniformCache->setBool("greenoption", false);

    mainprogram->uniformCache->setBool("blueoption", true);
    glGenTextures(1, &mainprogram->bluegradienttex);
    glBindTexture(GL_TEXTURE_2D, mainprogram->bluegradienttex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 256, 32);
    glGenFramebuffers(1, &mainprogram->bluegradientfbo);
    glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->bluegradientfbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mainprogram->bluegradienttex, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    draw_direct(nullptr, black, -1.0f, -1.0f, 2.0f, 2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, -1, glob->w, glob->h, false, false);
    mainprogram->uniformCache->setBool("blueoption", false);

    mainprogram->uniformCache->setBool("hueoption", true);
    glGenTextures(1, &mainprogram->huegradienttex);
    glBindTexture(GL_TEXTURE_2D, mainprogram->huegradienttex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 256, 32);
    glGenFramebuffers(1, &mainprogram->huegradientfbo);
    glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->huegradientfbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mainprogram->huegradienttex, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    draw_direct(nullptr, black, -1.0f, -1.0f, 2.0f, 2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, -1, glob->w, glob->h, false, false);
    mainprogram->uniformCache->setBool("hueoption", false);

    mainprogram->uniformCache->setBool("satoption", true);
    glGenTextures(1, &mainprogram->satgradienttex);
    glBindTexture(GL_TEXTURE_2D, mainprogram->satgradienttex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 256, 32);
    glGenFramebuffers(1, &mainprogram->satgradientfbo);
    glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->satgradientfbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mainprogram->satgradienttex, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    draw_direct(nullptr, black, -1.0f, -1.0f, 2.0f, 2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, -1, glob->w, glob->h, false, false);
    mainprogram->uniformCache->setBool("satoption", false);

    mainprogram->uniformCache->setBool("brightoption", true);
    glGenTextures(1, &mainprogram->brightgradienttex);
    glBindTexture(GL_TEXTURE_2D, mainprogram->brightgradienttex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 256, 32);
    glGenFramebuffers(1, &mainprogram->brightgradientfbo);
    glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->brightgradientfbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mainprogram->brightgradienttex, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    draw_direct(nullptr, black, -1.0f, -1.0f, 2.0f, 2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, -1, glob->w, glob->h, false, false);
    mainprogram->uniformCache->setBool("brightoption", false);

    mainprogram->uniformCache->setBool("alphaoption", true);
    glGenTextures(1, &mainprogram->alphagradienttex);
    glBindTexture(GL_TEXTURE_2D, mainprogram->alphagradienttex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 256, 32);
    glGenFramebuffers(1, &mainprogram->alphagradientfbo);
    glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->alphagradientfbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mainprogram->alphagradienttex, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    draw_direct(nullptr, black, -1.0f, -1.0f, 2.0f, 2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, -1, glob->w, glob->h, false, false);
    mainprogram->uniformCache->setBool("alphaoption", false);
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
	mainprogram->uniformCache->setBool("linetriangle", true);
	mainprogram->uniformCache->setFloat4v("color", line->linec);
	GLfloat fvcoords[6] = { line->x1, line->y1, 1.0f, line->x2, line->y2, 1.0f};
 	GLuint fvbuf, fvao;
    glGenBuffers(1, &fvbuf);
	glBindBuffer(GL_ARRAY_BUFFER, fvbuf);
    glBufferData(GL_ARRAY_BUFFER, 24, fvcoords, GL_DYNAMIC_DRAW);
	glGenVertexArrays(1, &fvao);
	glBindVertexArray(fvao);
	glBindBuffer(GL_ARRAY_BUFFER, fvbuf);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12, nullptr);
	glDrawArrays(GL_LINES, 0, 2);
	mainprogram->uniformCache->setBool("linetriangle", false);
	
    glDeleteBuffers(1, &fvbuf);
	glDeleteVertexArrays(1, &fvao);
}

void draw_direct(float* linec, float* areac, float x, float y, float wi, float he, float dx, float dy, float scale,
                 float opacity, int circle, GLuint tex, float smw, float smh, bool vertical, bool inverted) {
    // Uniform variables no longer needed with UniformCache

	if (circle) {
		// Circle uniforms are handled inline
	}
	else if (tex != -1) {
		mainprogram->uniformCache->setBool("down", true);
	}
	if (areac) {
		mainprogram->uniformCache->setBool("box", true);
	}

	if (linec) {
		mainprogram->uniformCache->setFloat4v("lcolor", linec);
        GLint m_viewport[4];
        glGetIntegerv(GL_VIEWPORT, m_viewport);
        // Cache viewport calculations
        float viewport_w = (float)(m_viewport[2] - m_viewport[0]);
        float viewport_h = (float)(m_viewport[3] - m_viewport[1]);
        mainprogram->uniformCache->setFloat("pixelw", 4.0f / (wi * viewport_w));
        mainprogram->uniformCache->setFloat("pixelh", 4.0f / (he * viewport_h));
	}
	else {
		mainprogram->uniformCache->setFloat("pixelw", 0.0f);
	}

	mainprogram->uniformCache->setFloat("opacity", opacity);

	if (areac) {
		float shx = -dx * 6.0f;
		float shy = -dy * 6.0f;
		GLfloat fvcoords2[12];
		if (!vertical) {
			fvcoords2[0] = x; fvcoords2[1] = y + he; fvcoords2[2] = 1.0;
			fvcoords2[3] = x; fvcoords2[4] = y; fvcoords2[5] = 1.0;
			fvcoords2[6] = x + wi; fvcoords2[7] = y + he; fvcoords2[8] = 1.0;
			fvcoords2[9] = x + wi; fvcoords2[10] = y; fvcoords2[11] = 1.0;
		}
		else {
			he /= 2.0f;
			wi *= 2.0f;
			fvcoords2[0] = x + he; fvcoords2[1] = y - wi; fvcoords2[2] = 1.0;
			fvcoords2[3] = x; fvcoords2[4] = y - wi; fvcoords2[5] = 1.0;
			fvcoords2[6] = x + he; fvcoords2[7] = y; fvcoords2[8] = 1.0;
			fvcoords2[9] = x; fvcoords2[10] = y; fvcoords2[11] = 1.0;
		}
		
		GLfloat tcoords[8];
        if (scale != 1.0f || dx != 0.0f || dy != 0.0f) {
            // Pre-calculate common values to avoid redundant math
            float half_scale = scale * 0.5f;
            float base_offset = 0.5f - half_scale;
            float left = base_offset + shx;
            float right = base_offset + scale + shx;
            float top = base_offset + shy;
            float bottom = base_offset + scale + shy;
            
            tcoords[0] = left;  tcoords[1] = top;     // bottom-left
            tcoords[2] = left;  tcoords[3] = bottom;  // top-left
            tcoords[4] = right; tcoords[5] = top;     // bottom-right
            tcoords[6] = right; tcoords[7] = bottom;  // top-right
        }
        else {
            tcoords[0] = 0.0f; tcoords[1] = 0.0f;
            tcoords[2] = 0.0f; tcoords[3] = 1.0f;
            tcoords[4] = 1.0f; tcoords[5] = 0.0f;
            tcoords[6] = 1.0f; tcoords[7] = 1.0f;
        }
        
        // Upload both buffers with reduced bind calls
        glBindBuffer(GL_ARRAY_BUFFER, mainprogram->bvbuf);
        glBufferSubData(GL_ARRAY_BUFFER, 0, 48, fvcoords2);
        glBindBuffer(GL_ARRAY_BUFFER, mainprogram->btbuf);
        glBufferSubData(GL_ARRAY_BUFFER, 0, 32, tcoords);
        
        mainprogram->uniformCache->setFloat4v("color", areac);
		if (tex != -1) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, tex);
			mainprogram->uniformCache->setBool("box", false);
			mainprogram->uniformCache->setBool("down", true);
		}
		if (circle) {
			mainprogram->uniformCache->setBool("box", false);
			mainprogram->uniformCache->setInt("circle", circle);
			
			// Pre-calculate common circle values
			float half_wi = wi * 0.5f;
			float circle_center_x = x + half_wi;
			float circle_center_y = y + half_wi;
			float circle_radius = wi * 0.25f;
			
			if (mainprogram->insmall) {
				mainprogram->uniformCache->setFloat("circleradius", circle_radius * smh);
				mainprogram->uniformCache->setFloat("cirx", (circle_center_x * 0.5f + 0.5f) * smw);
				mainprogram->uniformCache->setFloat("ciry", (circle_center_y * 0.5f + 0.5f) * smh);
			}
			else {
				mainprogram->uniformCache->setFloat("circleradius", circle_radius * glob->h);
				mainprogram->uniformCache->setFloat("cirx", (circle_center_x * 0.5f + 0.5f) * glob->w);
				mainprogram->uniformCache->setFloat("ciry", (circle_center_y * 0.5f + 0.5f) * glob->h);
			}
		}
		glBindVertexArray(mainprogram->bvao);
		glEnable(GL_BLEND);
        if (inverted) {
            glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        } else {
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
		// border is drawn in shader
		if (tex != -1) mainprogram->uniformCache->setBool("down", false);
		if (circle) mainprogram->uniformCache->setInt("circle", 0);
	}

	if (areac) mainprogram->uniformCache->setBool("box", false);
	mainprogram->uniformCache->setFloat("opacity", 1.0f);
}

void draw_box(Boxx *box, GLuint tex) {
	draw_box(box->lcolor, box->acolor, box->vtxcoords->x1, box->vtxcoords->y1, box->vtxcoords->w, box->vtxcoords->h, 0.0f, 0.0f, 1.0f, 1.0f, 0, tex, glob->w, glob->h, 0);
}

void draw_box(float *linec, float *areac, Boxx *box, GLuint tex) {
    draw_box(linec, areac, box->vtxcoords->x1, box->vtxcoords->y1, box->vtxcoords->w, box->vtxcoords->h, 0.0f, 0.0f, 1.0f, 1.0f, 0, tex, glob->w, glob->h, 0);
}

void draw_box(float *linec, float *areac, std::unique_ptr <Boxx> const &box, GLuint tex) {
    draw_box(linec, areac, box->vtxcoords->x1, box->vtxcoords->y1, box->vtxcoords->w, box->vtxcoords->h, 0.0f, 0.0f, 1.0f, 1.0f, 0, tex, glob->w, glob->h, 0);
}

void draw_box(Boxx *box, float opacity, GLuint tex) {
	draw_box(box->lcolor, box->acolor, box->vtxcoords->x1, box->vtxcoords->y1, box->vtxcoords->w, box->vtxcoords->h, 0.0f, 0.0f, 1.0f, opacity, 0, tex, glob->w, glob->h, 0);
}

void draw_box(Boxx *box, float dx, float dy, float scale, GLuint tex) {
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
		if (text && !circle) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			mainprogram->uniformCache->setBool("textmode", true);
		}
		draw_direct(linec, areac, x, y, wi, he, dx, dy, scale, opacity, circle, tex, smw, smh, vertical, inverted);
		if (text && !circle) {
			mainprogram->uniformCache->setBool("textmode", false);
		}
		return;
	}

    if (circle || mainprogram->frontbatch) {
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
    if (text) {
        if (tex != -1) mainprogram->textcountingtexes[mainprogram->textcurrbatch]++;
        *mainprogram->textbdtnptr[mainprogram->textcurrbatch]++ = tex;
        
        *mainprogram->textbdtptr[mainprogram->textcurrbatch]++ =
                    mainprogram->textcountingtexes[mainprogram->textcurrbatch] - 1;

        mainprogram->boxz += 0.001f;
        if (!vertical) {
            *mainprogram->textbdvptr[mainprogram->textcurrbatch]++ = x;
            *mainprogram->textbdvptr[mainprogram->textcurrbatch]++ = y + he;
            *mainprogram->textbdvptr[mainprogram->textcurrbatch]++ = 0.5f - mainprogram->boxz;
            *mainprogram->textbdvptr[mainprogram->textcurrbatch]++ = x;
            *mainprogram->textbdvptr[mainprogram->textcurrbatch]++ = y;
            *mainprogram->textbdvptr[mainprogram->textcurrbatch]++ = 0.5f - mainprogram->boxz;
            *mainprogram->textbdvptr[mainprogram->textcurrbatch]++ = x + wi;
            *mainprogram->textbdvptr[mainprogram->textcurrbatch]++ = y + he;
            *mainprogram->textbdvptr[mainprogram->textcurrbatch]++ = 0.5f - mainprogram->boxz;
            *mainprogram->textbdvptr[mainprogram->textcurrbatch]++ = x + wi;
            *mainprogram->textbdvptr[mainprogram->textcurrbatch]++ = y;
            *mainprogram->textbdvptr[mainprogram->textcurrbatch]++ = 0.5f - mainprogram->boxz;
        } else {
            *mainprogram->textbdvptr[mainprogram->textcurrbatch]++ = x;
            *mainprogram->textbdvptr[mainprogram->textcurrbatch]++ = y;
            *mainprogram->textbdvptr[mainprogram->textcurrbatch]++ = 0.5f - mainprogram->boxz;
            *mainprogram->textbdvptr[mainprogram->textcurrbatch]++ = x - wi / 2.0f;
            *mainprogram->textbdvptr[mainprogram->textcurrbatch]++ = y;
            *mainprogram->textbdvptr[mainprogram->textcurrbatch]++ = 0.5f - mainprogram->boxz;
            *mainprogram->textbdvptr[mainprogram->textcurrbatch]++ = x;
            *mainprogram->textbdvptr[mainprogram->textcurrbatch]++ = y + he * 2.0f;
            *mainprogram->textbdvptr[mainprogram->textcurrbatch]++ = 0.5f - mainprogram->boxz;
            *mainprogram->textbdvptr[mainprogram->textcurrbatch]++ = x - wi / 2.0f;
            *mainprogram->textbdvptr[mainprogram->textcurrbatch]++ = y + he * 2.0f;
            *mainprogram->textbdvptr[mainprogram->textcurrbatch]++ = 0.5f - mainprogram->boxz;
        }

        *mainprogram->textbdtcptr[mainprogram->textcurrbatch]++ = 0.0f;
        *mainprogram->textbdtcptr[mainprogram->textcurrbatch]++ = 0.0f;
        *mainprogram->textbdtcptr[mainprogram->textcurrbatch]++ = 0.0f;
        *mainprogram->textbdtcptr[mainprogram->textcurrbatch]++ = 1.0f;
        *mainprogram->textbdtcptr[mainprogram->textcurrbatch]++ = 1.0f;
        *mainprogram->textbdtcptr[mainprogram->textcurrbatch]++ = 0.0f;
        *mainprogram->textbdtcptr[mainprogram->textcurrbatch]++ = 1.0f;
        *mainprogram->textbdtcptr[mainprogram->textcurrbatch]++ = 1.0f;
        
        *mainprogram->textbdcptr[mainprogram->textcurrbatch]++ = (char) (areac[0] * 255.0f);
        *mainprogram->textbdcptr[mainprogram->textcurrbatch]++ = (char) (areac[1] * 255.0f);
        *mainprogram->textbdcptr[mainprogram->textcurrbatch]++ = (char) (areac[2] * 255.0f);
        *mainprogram->textbdcptr[mainprogram->textcurrbatch]++ = (char) (areac[3] * 255.0f);

        if (mainprogram->textcountingtexes[mainprogram->textcurrbatch] == mainprogram->maxtexes - 3) {
            mainprogram->textcurrbatch++;
            mainprogram->textbdvptr[mainprogram->textcurrbatch] = mainprogram->textbdcoords[mainprogram->textcurrbatch];
            mainprogram->textbdtcptr[mainprogram->textcurrbatch] = mainprogram->textbdtexcoords[mainprogram->textcurrbatch];
            mainprogram->textbdcptr[mainprogram->textcurrbatch] = mainprogram->textbdcolors[mainprogram->textcurrbatch];
            mainprogram->textbdtptr[mainprogram->textcurrbatch] = mainprogram->textbdtexes[mainprogram->textcurrbatch];
            mainprogram->textbdtnptr[mainprogram->textcurrbatch] = mainprogram->textboxtexes[mainprogram->textcurrbatch];
            mainprogram->textcountingtexes[mainprogram->textcurrbatch] = 0;
        }        
    }
    else {
        if (tex != -1) mainprogram->countingtexes[mainprogram->currbatch]++;
        *mainprogram->bdtnptr[mainprogram->currbatch]++ = tex;

        if (tex != -1) {
            *mainprogram->bdtptr[mainprogram->currbatch]++ = mainprogram->countingtexes[mainprogram->currbatch] - 1;
        } else {
            *mainprogram->bdtptr[mainprogram->currbatch]++ = 255;
        }

        mainprogram->boxz += 0.001f;
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
        } else {
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
            *mainprogram->bdcptr[mainprogram->currbatch]++ = (char) (areac[0] * 255.0f);
            *mainprogram->bdcptr[mainprogram->currbatch]++ = (char) (areac[1] * 255.0f);
            *mainprogram->bdcptr[mainprogram->currbatch]++ = (char) (areac[2] * 255.0f);
            *mainprogram->bdcptr[mainprogram->currbatch]++ = (char) (areac[3] * 255.0f);
        } else {
            *mainprogram->bdcptr[mainprogram->currbatch]++ = 0;
            *mainprogram->bdcptr[mainprogram->currbatch]++ = 0;
            *mainprogram->bdcptr[mainprogram->currbatch]++ = 0;
            *mainprogram->bdcptr[mainprogram->currbatch]++ = 0;
        }

        if (mainprogram->countingtexes[mainprogram->currbatch] == mainprogram->maxtexes - 3) {
            mainprogram->currbatch++;
            mainprogram->bdvptr[mainprogram->currbatch] = mainprogram->bdcoords[mainprogram->currbatch];
            mainprogram->bdtcptr[mainprogram->currbatch] = mainprogram->bdtexcoords[mainprogram->currbatch];
            mainprogram->bdcptr[mainprogram->currbatch] = mainprogram->bdcolors[mainprogram->currbatch];
            mainprogram->bdtptr[mainprogram->currbatch] = mainprogram->bdtexes[mainprogram->currbatch];
            mainprogram->bdtnptr[mainprogram->currbatch] = mainprogram->boxtexes[mainprogram->currbatch];
            mainprogram->countingtexes[mainprogram->currbatch] = 0;
        }
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

	mainprogram->uniformCache->setBool("linetriangle", true);

	if (triangle->linec) mainprogram->uniformCache->setFloat4v("color", triangle->linec);

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
    glBufferData(GL_ARRAY_BUFFER, 36, fvcoords, GL_DYNAMIC_DRAW);
	glGenVertexArrays(1, &fvao);
	glBindVertexArray(fvao);
	//glBindBuffer(GL_ARRAY_BUFFER, fvbuf);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12, nullptr);
	if (triangle->type == CLOSED) {
		mainprogram->uniformCache->setFloat4v("color", triangle->areac);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);
	}
	if (triangle->type == OPEN) glDrawArrays(GL_LINE_LOOP, 0, 3);

	mainprogram->uniformCache->setBool("linetriangle", false);
    glDeleteBuffers(1, &fvbuf);
	glDeleteVertexArrays(1, &fvao);
}


std::vector<float> render_text(const char* text, float* textc, float x, float y, float sx, float sy, const char* save) {
    const std::string dummy;
    std::vector<float> ret = render_text(dummy, text, textc, x, y, sx, sy, 0, 1, 0, save);
    return ret;
}

std::vector<float> render_text(const char* text, float* textc, float x, float y, float sx, float sy, int smflag) {
    const std::string dummy;
    std::vector<float> ret = render_text(dummy, text, textc, x, y, sx, sy, smflag, 1, 0);
    return ret;
}

std::vector<float> render_text(const char* text, float* textc, float x, float y, float sx, float sy, int smflag, bool vertical) {
    const std::string dummy;
    std::vector<float> ret = render_text(dummy, text, textc, x, y, sx, sy, smflag, 1, vertical);
    return ret;
}

std::vector<float> render_text(const char* text, float *textc, float x, float y, float sx, float sy, int smflag, bool display, bool vertical) {
    const std::string dummy;
    std::vector<float> ret = render_text(dummy, text, textc, x, y, sx, sy, smflag, display, vertical);
    return ret;
}

std::vector<float> render_text(const std::string& text, float* textc, float x, float y, float sx, float sy) {
    std::vector<float> ret = render_text(text, nullptr, textc, x, y, sx, sy, 0, 1, 0);
    return ret;
}

std::vector<float> render_text(const std::string& text, float* textc, float x, float y, float sx, float sy, int smflag) {
    std::vector<float> ret = render_text(text, nullptr, textc, x, y, sx, sy, smflag, 1, 0);
    return ret;
}

std::vector<float> render_text(const std::string& text, float* textc, float x, float y, float sx, float sy, int smflag, bool vertical) {
    std::vector<float> ret = render_text(text, nullptr, textc, x, y, sx, sy, smflag, 1, vertical);
    return ret;
}

std::vector<float> render_text(const std::string& stext, const char* ctext, float *textc, float x, float y, float sx, float sy, int smflag, bool display, bool vertical, const char* save) {
    //text.reserve(60);
    char* text;
    if (stext == "") {
        text = (char*)ctext;
    }
    else if (ctext == nullptr) {
        text = (char*)stext.c_str();
    }
    std::vector<float> textwsplay;
    if (text == nullptr) {
        //textwsplay.push_back(0.0f);
        return textwsplay;
    }
    if (smflag > 0) y -= 0.055f;
    float bux = x;
	float buy = y;
	if (text == "") return textwsplay;
    if (binsmain->inbin) y -= 0.012f * (2070.0f * glob->h / 2400.0f / mainprogram->globh);
	else if (smflag == 0) y -= 0.012f * (2070.0f * glob->h / 2400.0f / glob->h);
	else if (smflag > 0) y -= 0.02f * (2070.0f * glob->h / 2400.0f / glob->h);
	GLuint texture;
	float wfac;
	float textw = 0.0f;
	float texth = 0.0f;
	bool prepare = true;
    GUIString *gs = nullptr;
    int pos = 0;
    if (strcmp(save, "true") == 0) {
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
        } else if (smflag == 1) {
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
        } else if (smflag == 2 || smflag == 3) {
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
    }

 	if (prepare) {
	    bool bufb = mainprogram->frontbatch;
        mainprogram->frontbatch = false;
		// compose string texture for display all next times
		const char *t = text;
		const char *p;

		int w2 = 0;
		int h2 = 0;
        if (smflag == 0) {
			w2 = glob->w;
			h2 = glob->h;
		}
        else if (smflag != 3) {
            w2 = smw;
            h2 = smh;
        }
        else if (smflag == 3) {
            w2 = glob->h / 2.0f;
            h2 = glob->h / 2.0f;
        }
		int psize = h2 * 13.5f * sy;

        if (smflag == 1) SDL_GL_MakeCurrent(mainprogram->prefwindow, glc);
        else if (smflag == 2) SDL_GL_MakeCurrent(mainprogram->config_midipresetswindow, glc);
        else if (smflag == 3) SDL_GL_MakeCurrent(mainprogram->splashwindow, glc);

        std::vector<float> textws;
		float pixelw = 2.0f / w2;
		float pixelh = 2.0f / h2;
		FT_Set_Pixel_Sizes(face, (int)(psize), psize * 1.33f);
		x = 0.0f;
		y = 0.0f;
        FT_GlyphSlot g = face->glyph;
        for (int i = 0; i < strlen(text); i++) {
            int glyph_index = FT_Get_Char_Index(face, t[i]);
            if (FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT))
                continue;
            //FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
            textw += (g->advance.x/64.0f) * pixelw;
            textws.push_back((g->advance.x / 64.0f) * pixelw * ((smflag == 0) + 1) * 0.5f);
            texth = 64.0f * pixelh;
        }
        glGenTextures(1, &texture);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexStorage2D(
                GL_TEXTURE_2D,
                1,
                GL_R8,
                textw / pixelw + 1,
                psize * 3
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glClearTexImage(texture, 0, GL_RED, GL_UNSIGNED_BYTE, black);

        for (int i = 0; i < strlen(text); i++) {
            int glyph_index = FT_Get_Char_Index(face, t[i]);
            if (FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT))
                continue;
            FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
            glTexSubImage2D(GL_TEXTURE_2D, 0, x, -g->bitmap_top + 60 * glob->h / 2400.0f + (6 * (smflag > 0)), g->bitmap.width, g->bitmap.rows, GL_RED, GL_UNSIGNED_BYTE, g->bitmap.buffer);
            x += (g->advance.x/64.0f);
        }

        if (strcmp(save, "true") == 0) {
            if (pos == 0) {
                gs = new GUIString;
                gs->text = text;
                if (smflag == 0) mainprogram->guitextmap[text] = gs;
                else if (smflag == 1) mainprogram->prguitextmap[text] = gs;
                else if (smflag == 2 || smflag == 3) mainprogram->tmguitextmap[text] = gs;
            }
            gs->texturevec.push_back(texture);
            gs->textwvec.push_back(textw);
            gs->texthvec.push_back(psize * 3);
            gs->textwvecvec.push_back(textws);
            gs->sxvec.push_back(sx);
        }

        if (smflag > 0) SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);

        mainprogram->frontbatch = bufb;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDrawBuffer(GL_BACK_LEFT);
        //display
        if (strcmp(save, "true") == 0) {
            if (!mainprogram->stringcomputing)
                textws = render_text(text, textc, bux, buy, sx, sy, smflag, display, vertical);
        }
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
        else if (smflag != 3) {
            w2 = smw;
            h2 = smh;
        }
        else if (smflag == 3) {
            w2 = glob->h / 2.0f;
            h2 = glob->h / 2.0f;
        }
		float pixelw = 2.0f / w2;
		float pixelh = 2.0f / h2;
		float textw2 = textw;
		float texth2 = texth * pixelh;

		float wi = 0.0f;
		float he = 0.0f;
		if (vertical) {
			he = texth2;
			wi = -textw2;
		}
		else {
			wi = textw2;
			he = texth2;
		}
		wi /= (smflag > 0) + 1;
        he /= (smflag > 0) + 1;

		y -= he;
        //draw text shadow
		if (textw != 0) draw_box(nullptr, black, x + 0.001f, y - 0.00185f + 72 * pixelh * glob->h / 2400.0f - he * 3 * vertical, wi + pixelw, he, texture, true, vertical, false);
		// draw text
        if (textw != 0) draw_box(nullptr, textc, x, y + 72 * pixelh * glob->h / 2400.0f - he * 3 * vertical, wi, he, texture, true, vertical, false);	//draw text

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
        GLbitfield waitFlags = GL_SYNC_FLUSH_COMMANDS_BIT;
        GLuint64 waitDuration = GL_TIMEOUT_IGNORED;
        GLenum waitReturn = GL_UNSIGNALED;
        while (waitReturn != GL_ALREADY_SIGNALED && waitReturn != GL_CONDITION_SATISFIED)
        {
            waitReturn = glClientWaitSync(syncObj, waitFlags, waitDuration);
            waitFlags = GL_SYNC_FLUSH_COMMANDS_BIT;
            waitDuration = UINT64_MAX;
        }
	}
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
	
	
void do_blur(bool stage, GLuint prevfbotex, int iter, bool notedgedetect) {
    // laststep handled inline with uniformCache
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, mainprogram->fbotex[stage * 2]);
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, mainprogram->fbotex[stage * 2 + 1]);
	if (iter == 0) return;
	GLboolean horizontal = false, first_iteration = true;
	GLuint *tex;
	mainprogram->uniformCache->setSampler("fboSampler", 0);
	glActiveTexture(GL_TEXTURE0);
	if (stage) glViewport(0, 0, mainprogram->ow[1], mainprogram->oh[1]);
	else glViewport(0, 0, mainprogram->ow[0], mainprogram->oh[0]);
	for (GLuint i = 0; i < iter; i++) {
		glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->frbuf[stage * 2 + horizontal]);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		if (first_iteration) tex = &prevfbotex;
		else tex = &mainprogram->fbotex[stage * 2 + !horizontal];
		glBindTexture(GL_TEXTURE_2D, *tex);
        if (notedgedetect && i == iter - 1) {
            mainprogram->uniformCache->setBool("laststep", true);
        }
		mainprogram->uniformCache->setBool("horizontal", horizontal);
		glBindVertexArray(mainprogram->fbovao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		if (i % 2) glActiveTexture(GL_TEXTURE5);
		else glActiveTexture(GL_TEXTURE6);
		mainprogram->uniformCache->setSampler("fboSampler", !horizontal + 5);
		horizontal = !horizontal;
		if (first_iteration)
			first_iteration = false;
	}
    mainprogram->uniformCache->setSampler("fboSampler", 0);
    mainprogram->uniformCache->setBool("laststep", false);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

        for (int i = 0; i < loopstation->elements.size(); i++) {
            if (loopstation->elements[i]->recbut->value) {
                loopstation->elements[i]->add_button_automationentry(mainmix->midishelfbutton);
            }
        }

        mainmix->midishelfbutton = nullptr;
        mainmix->midibutton = nullptr;
    }

	else if (mainmix->midibutton) {
		Button *but = mainmix->midibutton;
        if (but->toggle > 1) {
            but->value++;
            if (but->value > but->toggle) but->value = 0;
        }
		if (but->toggle == 0) but->value = !but->value;
		if (but == mainprogram->wormgate1 || but == mainprogram->wormgate2) mainprogram->binsscreen = but->value;
		
		for (int i = 0; i < loopstation->elements.size(); i++) {
			if (loopstation->elements[i]->recbut->value) {
				loopstation->elements[i]->add_button_automationentry(mainmix->midibutton);
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
				par->value = 1.0f + (5.0f / 64.0f) * (mainmix->midi2 - 64.0f);
			}
			else {
				par->value = 0.0f + (1.0f / 64.0f) * mainmix->midi2;
			}
		}
		else {
			mainprogram->uniformCache->setFloat(par->shadervar.c_str(), par->value);
		}
			
		for (int i = 0; i < loopstation->elements.size(); i++) {
			if (loopstation->elements[i]->recbut->value) {
				loopstation->elements[i]->add_param_automationentry(mainmix->midiparam);
			}
		}
		
		mainmix->midiparam = nullptr;
	}
}

void onestepfrom(bool stage, Node *node, Node *prevnode, GLuint prevfbotex, GLuint prevfbo) {
	mainprogram->uniformCache->setSampler("Sampler0", 0);
	//GLint blurswitch = glGetUniformLocation(mainprogram->ShaderProgram, "blurswitch");
	// fxid handled inline with uniformCache
		
	float div = 1.0f;
	if (mainprogram->ow[1] > mainprogram->oh[1]) {
		if (stage == 0) div = mainprogram->ow[0] / mainprogram->ow[1];
	}
	else {
		if (stage == 0) div = mainprogram->oh[0] / mainprogram->oh[1];
	}

    GLuint fbowidth;
    GLuint fboheight;
    GLfloat fcdiv;
    if (stage == 0) {
        mainprogram->uniformCache->setInt("fbowidth", mainprogram->ow[0]);
        mainprogram->uniformCache->setInt("fboheight", mainprogram->oh[0]);
        mainprogram->uniformCache->setFloat("fcdiv", div);
    }
    else {
        mainprogram->uniformCache->setInt("fbowidth", mainprogram->ow[1]);
        mainprogram->uniformCache->setInt("fboheight", mainprogram->oh[1]);
        mainprogram->uniformCache->setFloat("fcdiv", div);
    }

    node->walked = true;
	
	if (node->type == EFFECT) {
		Effect *effect = ((EffectNode*)node)->effect;

		if (1) {
		//if (effect->layer->initialized) {
			mainprogram->uniformCache->setFloat("drywet", effect->drywet->value);
			if (effect->onoffbutton->value) {
                for (int i = 0; i < effect->params.size(); i++) {
                    Param *par = effect->params[i];
                    float val;
                    val = par->value;
                    mainprogram->uniformCache->setFloat(par->shadervar.c_str(), val);
                }
				switch (effect->type) {
					case BLUR: {
                        mainprogram->uniformCache->setBool("ineffect", true);
                        mainprogram->uniformCache->setSampler("Sampler1", 1);
                        glActiveTexture(GL_TEXTURE1);
                        glBindTexture(GL_TEXTURE_2D, prevfbotex);
                        glActiveTexture(GL_TEXTURE0);
						mainprogram->uniformCache->setInt("fxid", BLUR);
						mainprogram->uniformCache->setBool("interm", true);
						do_blur(stage, prevfbotex, ((BlurEffect*)effect)->times, true);
						glActiveTexture(GL_TEXTURE0);
                        if (((BlurEffect*)effect)->times > 1) {
                            prevfbotex = mainprogram->fbotex[stage * 2 + 1];
                        }
                        mainprogram->uniformCache->setBool("ineffect", false);
						break;
					 }

					case BOXBLUR: {
                        mainprogram->uniformCache->setBool("ineffect", true);
                        mainprogram->uniformCache->setSampler("Sampler1", 1);
                        glActiveTexture(GL_TEXTURE1);
                        glBindTexture(GL_TEXTURE_2D, prevfbotex);
                        glActiveTexture(GL_TEXTURE0);
						mainprogram->uniformCache->setInt("fxid", BOXBLUR);
						mainprogram->uniformCache->setBool("interm", true);
						do_blur(stage, prevfbotex, 2, true);
						glActiveTexture(GL_TEXTURE0);
                        prevfbotex = mainprogram->fbotex[stage * 2 + 1];
                        mainprogram->uniformCache->setBool("ineffect", false);
						break;
					}

                    case CHROMASTRETCH: {
                        mainprogram->uniformCache->setInt("fxid", CHROMASTRETCH);
                        break;
                    }

                    case RADIALBLUR: {
                        mainprogram->uniformCache->setInt("fxid", RADIALBLUR);
                        break;
                    }

                    case GLOW: {
                        mainprogram->uniformCache->setBool("ineffect", true);
                        mainprogram->uniformCache->setSampler("Sampler1", 1);
                        glActiveTexture(GL_TEXTURE1);
                        glBindTexture(GL_TEXTURE_2D, prevfbotex);
                        glActiveTexture(GL_TEXTURE0);
						mainprogram->uniformCache->setInt("fxid", GLOW);
						mainprogram->uniformCache->setBool("interm", true);
						do_blur(stage, prevfbotex, 2, true);
						glActiveTexture(GL_TEXTURE0);
						prevfbotex = mainprogram->fbotex[stage * 2 + 1];
                        mainprogram->uniformCache->setBool("ineffect", false);
						break;
					 }

					case CONTRAST: {
						mainprogram->uniformCache->setInt("fxid", CONTRAST);
						break;
					 }

					case BRIGHTNESS: {
						mainprogram->uniformCache->setInt("fxid", BRIGHTNESS);
						break;
					 }

					case SCALE: {
						mainprogram->uniformCache->setInt("fxid", SCALE);
						break;
					 }

					case SWIRL: {
						mainprogram->uniformCache->setInt("fxid", SWIRL);
						break;
					 }

					case CHROMAROTATE: {
						mainprogram->uniformCache->setInt("fxid", CHROMAROTATE);
						break;
					 }

					case DOT: {
						mainprogram->uniformCache->setInt("fxid", DOT);
						break;
					 }

					case SATURATION: {
						mainprogram->uniformCache->setInt("fxid", SATURATION);
						break;
					 }

					case OLDFILM: {
						mainprogram->uniformCache->setFloat("RandomValue", (float)(rand() % 100) / 100.0);
						mainprogram->uniformCache->setInt("fxid", OLDFILM);
						break;
					 }

					case RIPPLE: {
						mainprogram->uniformCache->setFloat("riptime", effect->get_ripplecount());
						mainprogram->uniformCache->setInt("fxid", RIPPLE);
						break;
					 }

					case FISHEYE: {
						mainprogram->uniformCache->setInt("fxid", FISHEYE);
						break;
					 }

					case TRESHOLD: {
						mainprogram->uniformCache->setInt("fxid", TRESHOLD);
						break;
					 }

					case STROBE: {
                        if (mainmix->oldstrobetime == 0.0f || mainmix->time - mainmix->oldstrobetime > 0.05f) {
                            mainmix->oldstrobetime = mainmix->time;
                            if (((StrobeEffect *) effect)->get_phase() == 0) ((StrobeEffect *) effect)->set_phase(1);
                            else ((StrobeEffect *) effect)->set_phase(0);
                        }
						mainprogram->uniformCache->setFloat("strobephase", ((StrobeEffect*)effect)->get_phase());
						mainprogram->uniformCache->setInt("fxid", STROBE);
						break;
					}

					case POSTERIZE: {
						mainprogram->uniformCache->setInt("fxid", POSTERIZE);
						break;
					 }

					case PIXELATE: {
						mainprogram->uniformCache->setInt("fxid", PIXELATE);
						break;
					 }

					case CROSSHATCH: {
						mainprogram->uniformCache->setInt("fxid", CROSSHATCH);
						break;
					 }

					case INVERT: {
						mainprogram->uniformCache->setInt("fxid", INVERT);
						break;
					 }

					case ROTATE: {
						mainprogram->uniformCache->setInt("fxid", ROTATE);
						break;
					}

					case EMBOSS: {
						mainprogram->uniformCache->setInt("fxid", EMBOSS);
						break;
					 }

					case ASCII: {
						mainprogram->uniformCache->setInt("fxid", ASCII);
						break;
					 }

					case SOLARIZE: {
						mainprogram->uniformCache->setInt("fxid", SOLARIZE);
						break;
					 }

					case VARDOT: {
						mainprogram->uniformCache->setInt("fxid", VARDOT);
						break;
					 }

					case CRT: {
						mainprogram->uniformCache->setInt("fxid", CRT);
						break;
					 }

					case EDGEDETECT: {
                        bool swits = 0;

                        mainprogram->uniformCache->setSampler("Sampler1", 1);
                        glActiveTexture(GL_TEXTURE1);
                        glBindTexture(GL_TEXTURE_2D, prevfbotex);
                        mainprogram->uniformCache->setBool("ineffect", true);
                        mainprogram->uniformCache->setInt("fxid", EDGEDETECT);
                        mainprogram->uniformCache->setBool("interm", true);
                        glActiveTexture(GL_TEXTURE0);
                        glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->frbuf[swits + stage * 2]);
                        glDrawBuffer(GL_COLOR_ATTACHMENT0);
                        glClearColor( 0.f, 0.f, 0.f, 0.f );
                        glClear(GL_COLOR_BUFFER_BIT);
                        if (stage) glViewport(0, 0, mainprogram->ow[1], mainprogram->oh[1]);
                        else glViewport(0, 0, mainprogram->ow[0], mainprogram->oh[0]);
                        glBindTexture(GL_TEXTURE_2D, prevfbotex);
                        glBindVertexArray(mainprogram->fbovao);
                        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                        prevfbotex = mainprogram->fbotex[swits + stage * 2];
                        swits = !swits;

                        mainprogram->uniformCache->setInt("fxid", CUTOFF);
                        mainprogram->uniformCache->setFloat("cut", 0.9f);
                        glActiveTexture(GL_TEXTURE0);
                        glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->frbuf[swits + stage * 2]);
                        glDrawBuffer(GL_COLOR_ATTACHMENT0);
                        glClearColor( 0.f, 0.f, 0.f, 0.f );
                        glClear(GL_COLOR_BUFFER_BIT);
                        if (stage) glViewport(0, 0, mainprogram->ow[1], mainprogram->oh[1]);
                        else glViewport(0, 0, mainprogram->ow[0], mainprogram->oh[0]);
                        glBindTexture(GL_TEXTURE_2D, prevfbotex);
                        glBindVertexArray(mainprogram->fbovao);
                        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                        prevfbotex = mainprogram->fbotex[swits + stage * 2];
                        swits = !swits;

                        if (((EdgeDetectEffect*)effect)->thickness > 1) {
                            mainprogram->uniformCache->setInt("fxid", BLUR);
                            do_blur(stage, prevfbotex, ((EdgeDetectEffect *) effect)->thickness, false);
                            prevfbotex = mainprogram->fbotex[stage * 2];
                            swits = true;
                        }
                        glActiveTexture(GL_TEXTURE0);

                        mainprogram->uniformCache->setInt("fxid", GAMMA);
                        mainprogram->uniformCache->setFloat("gammaval", 2.5f);
                        glActiveTexture(GL_TEXTURE0);
                        glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->frbuf[swits + stage * 2]);
                        glDrawBuffer(GL_COLOR_ATTACHMENT0);
                        if (stage) glViewport(0, 0, mainprogram->ow[1], mainprogram->oh[1]);
                        else glViewport(0, 0, mainprogram->ow[0], mainprogram->oh[0]);
                        glBindTexture(GL_TEXTURE_2D, prevfbotex);
                        glBindVertexArray(mainprogram->fbovao);
                        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                        prevfbotex = mainprogram->fbotex[swits + stage * 2];
                        swits = !swits;

                        mainprogram->uniformCache->setInt("fxid", CONTRAST);
                        mainprogram->uniformCache->setFloat("contrastamount", 8.0f);
                        glActiveTexture(GL_TEXTURE0);
                        glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->frbuf[swits + stage * 2]);
                        glDrawBuffer(GL_COLOR_ATTACHMENT0);
                        if (stage) glViewport(0, 0, mainprogram->ow[1], mainprogram->oh[1]);
                        else glViewport(0, 0, mainprogram->ow[0], mainprogram->oh[0]);
                        glBindTexture(GL_TEXTURE_2D, prevfbotex);
                        glBindVertexArray(mainprogram->fbovao);
                        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                        prevfbotex = mainprogram->fbotex[swits + stage * 2];
                        swits = !swits;

                        mainprogram->uniformCache->setInt("fxid", INVERT);
                        glActiveTexture(GL_TEXTURE0);
                        glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->frbuf[swits + stage * 2]);
                        glDrawBuffer(GL_COLOR_ATTACHMENT0);
                        if (stage) glViewport(0, 0, mainprogram->ow[1], mainprogram->oh[1]);
                        else glViewport(0, 0, mainprogram->ow[0], mainprogram->oh[0]);
                        glBindTexture(GL_TEXTURE_2D, prevfbotex);
                        glBindVertexArray(mainprogram->fbovao);
                        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                        prevfbotex = mainprogram->fbotex[swits + stage * 2];
                        swits = !swits;

                        mainprogram->uniformCache->setBool("interm", false);

                        mainprogram->uniformCache->setInt("mixmode", 0);
                        mainprogram->uniformCache->setBool("edgethickmode", true);
                        glActiveTexture(GL_TEXTURE0);
                        glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->frbuf[swits + stage * 2]);
                        glDrawBuffer(GL_COLOR_ATTACHMENT0);
                        if (stage) glViewport(0, 0, mainprogram->ow[1], mainprogram->oh[1]);
                        else glViewport(0, 0, mainprogram->ow[0], mainprogram->oh[0]);
                        glBindTexture(GL_TEXTURE_2D, prevfbotex);
                        glBindVertexArray(mainprogram->fbovao);
                        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                        prevfbotex = mainprogram->fbotex[swits + stage * 2];
                        swits = !swits;
                        mainprogram->uniformCache->setBool("interm", true);
                        mainprogram->uniformCache->setBool("edgethickmode", false);

                        mainprogram->uniformCache->setInt("fxid", INVERT);
                        mainprogram->uniformCache->setBool("laststep", true);
                        glActiveTexture(GL_TEXTURE0);
                        glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->frbuf[swits + stage * 2]);
                        glDrawBuffer(GL_COLOR_ATTACHMENT0);
                        if (stage) glViewport(0, 0, mainprogram->ow[1], mainprogram->oh[1]);
                        else glViewport(0, 0, mainprogram->ow[0], mainprogram->oh[0]);
                        glBindTexture(GL_TEXTURE_2D, prevfbotex);
                        glBindVertexArray(mainprogram->fbovao);
                        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                        prevfbotex = mainprogram->fbotex[swits + stage * 2];
                        mainprogram->uniformCache->setBool("laststep", false);

                        mainprogram->uniformCache->setBool("interm", false);
                        mainprogram->uniformCache->setBool("ineffect", false);
                        glViewport(0, 0, glob->w, glob->h);
						break;
					}

					case KALEIDOSCOPE: {
						mainprogram->uniformCache->setInt("fxid", KALEIDOSCOPE);
						break;
					 }

					case HTONE: {
						mainprogram->uniformCache->setInt("fxid", HTONE);
						break;
					 }

					case CARTOON: {
						mainprogram->uniformCache->setInt("fxid", CARTOON);
						break;
					 }

					case CUTOFF: {
						mainprogram->uniformCache->setInt("fxid", CUTOFF);
						break;
					 }

					case GLITCH: {
						mainprogram->uniformCache->setInt("fxid", GLITCH);
						break;
					 }

					case COLORIZE: {
						mainprogram->uniformCache->setInt("fxid", COLORIZE);
						break;
					 }

					case NOISE: {
                        mainprogram->uniformCache->setFloat("RandomValue", (float)(rand() % 100) / 100.0);
						mainprogram->uniformCache->setInt("fxid", NOISE);
						break;
					 }

					case GAMMA: {
						mainprogram->uniformCache->setInt("fxid", GAMMA);
						break;
					 }

					case THERMAL: {
						mainprogram->uniformCache->setInt("fxid", THERMAL);
						break;
					 }

					case BOKEH: {
						mainprogram->uniformCache->setInt("fxid", BOKEH);
						break;
					 }

					case SHARPEN: {
						mainprogram->uniformCache->setInt("fxid", SHARPEN);
						break;
					 }

					case DITHER: {
						mainprogram->uniformCache->setInt("fxid", DITHER);
						break;
					 }

					case FLIP: {
						mainprogram->uniformCache->setInt("fxid", FLIP);
						break;
					 }

					case MIRROR: {
						mainprogram->uniformCache->setInt("fxid", MIRROR);
						break;
					}
				}
			}

           glActiveTexture(GL_TEXTURE0);
            if (effect->fbo == -1) {
                do {
                    if (effect->fbo != -1) {
                        glDeleteFramebuffers(1, &effect->fbo);
                    }
                    if (effect->fbotex != -1) {
                        glDeleteTextures(1, &effect->fbotex);
                    }
                    GLuint rettex;
                    if (stage == 0) {
                        rettex = mainprogram->grab_from_texpool(mainprogram->ow[0], mainprogram->oh[0], GL_RGBA8);
                    } else {
                        rettex = mainprogram->grab_from_texpool(mainprogram->ow[1], mainprogram->oh[1], GL_RGBA8);
                    }
                    if (rettex != -1) {
                        effect->fbotex = rettex;
                    } else {
                        glGenTextures(1, &(effect->fbotex));
                        glBindTexture(GL_TEXTURE_2D, effect->fbotex);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
                        if (stage == 0) {

                            glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, mainprogram->ow[0], mainprogram->oh[0]);
                        } else {
                            glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, mainprogram->ow[1], mainprogram->oh[1]);
                        }
                        mainprogram->texintfmap[effect->fbotex] = GL_RGBA8;
                    }
                    GLuint retfbo;
                    retfbo = mainprogram->grab_from_fbopool();
                    if (retfbo != -1) {
                        effect->fbo = retfbo;
                    } else {
                        glGenFramebuffers(1, &(effect->fbo));
                    }
                    glBindFramebuffer(GL_FRAMEBUFFER, effect->fbo);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, effect->fbotex, 0);
                } while (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE);
            }

            if (effect->tempfbo == -1) {
                do {
                    if (effect->tempfbo != -1) {
                        glDeleteFramebuffers(1, &effect->tempfbo);
                    }
                    if (effect->tempfbotex != -1) {
                        glDeleteTextures(1, &effect->tempfbotex);
                    }
                    GLuint rettex;
                    if (stage == 0) {
                        rettex = mainprogram->grab_from_texpool(mainprogram->ow[0], mainprogram->oh[0], GL_RGBA8);
                    } else {
                        rettex = mainprogram->grab_from_texpool(mainprogram->ow[1], mainprogram->oh[1], GL_RGBA8);
                    }
                    if (rettex != -1) {
                        effect->tempfbotex = rettex;
                    } else {
                        glGenTextures(1, &(effect->tempfbotex));
                        glBindTexture(GL_TEXTURE_2D, effect->tempfbotex);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
                        if (stage == 0) {

                            glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, mainprogram->ow[0], mainprogram->oh[0]);
                        } else {
                            glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, mainprogram->ow[1], mainprogram->oh[1]);
                        }
                        mainprogram->texintfmap[effect->tempfbotex] = GL_RGBA8;
                    }
                    GLuint retfbo;
                    retfbo = mainprogram->grab_from_fbopool();
                    if (retfbo != -1) {
                        effect->tempfbo = retfbo;
                    } else {
                        glGenFramebuffers(1, &(effect->tempfbo));
                    }
                    glBindFramebuffer(GL_FRAMEBUFFER, effect->tempfbo);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, effect->tempfbotex, 0);
                } while (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE);
            }

            if (effect->type == EDGEDETECT) {
                mainprogram->uniformCache->setBool("down", true);
            } else if (effect->onoffbutton->value) {
                mainprogram->uniformCache->setBool("interm", true);
            } else {
                mainprogram->uniformCache->setBool("down", true);
            }
            if (effect->node == (EffectNode *) (effect->layer->lasteffnode[0])) {
                mainprogram->uniformCache->setFloat("opacity", effect->layer->opacity->value);
            } else
                mainprogram->uniformCache->setFloat("opacity", 1.0f);

            glBindFramebuffer(GL_FRAMEBUFFER, effect->fbo);
            glDrawBuffer(GL_COLOR_ATTACHMENT0);
            if (stage) glViewport(0, 0, mainprogram->ow[1], mainprogram->oh[1]);
            else glViewport(0, 0, mainprogram->ow[0], mainprogram->oh[0]);
            glClearColor(0.f, 0.f, 0.f, 0.f);
            glClear(GL_COLOR_BUFFER_BIT);

            Layer *lay = effect->layer;

            float sx = 0.0f;
            float sy = 0.0f;
            float sc = 1.0f;
            float op = 1.0f;
            if (effect->node == lay->lasteffnode[0]) {
                sx = lay->shiftx->value;
                sy = lay->shifty->value;
                sc = lay->scale->value;
                op = lay->opacity->value;
            }
            if (effect->ffglnr != -1 && effect->onoffbutton->value) {
                FFGLEffect *eff = (FFGLEffect*)effect;
                FFGLFramebuffer infbo;
                infbo.fbo = prevfbo;
                infbo.colorTexture = prevfbotex;
                infbo.width = mainprogram->ow[stage];
                infbo.height = mainprogram->oh[stage];
                FFGLFramebuffer outfbo;
                outfbo.fbo = effect->tempfbo;
                outfbo.colorTexture = effect->tempfbotex;
                outfbo.width = mainprogram->ow[stage];
                outfbo.height = mainprogram->oh[stage];

                for (int i = 0; i < eff->instance->parameters.size(); i++) {
                    if (effect->params[i]->type == FF_TYPE_OPTION) {
                        eff->instance->setParameter(i, eff->instance->parameters[i].elements[effect->params[i]->value].value);
                    } else if (effect->params[i]->type == FF_TYPE_TEXT || effect->params[i]->type == FF_TYPE_FILE) {
                        eff->instance->setParameter(i, FFGLUtils::PointerToFFMixed((char*)(effect->params[i]->valuestr.c_str())));
                    } else {
                        eff->instance->setParameter(i, effect->params[i]->value);
                    }
                    if (effect->params[i]->type == FF_TYPE_EVENT) {
                        effect->params[i]->value = 0.0f;
                    }
                }

                static float effectTime = 0.0f;
                static auto lastFrame = std::chrono::high_resolution_clock::now();

                float currentTime = EffectTimer::getTime(); // Starts from 0.0
                //instance->setTime(effectTime);

                bool ret = eff->instance->processFrame({infbo}, outfbo);

                if (!ret) {
                    glBindFramebuffer(GL_FRAMEBUFFER, effect->fbo);
                    glDrawBuffer(GL_COLOR_ATTACHMENT0);
                    if (stage) glViewport(0, 0, mainprogram->ow[1], mainprogram->oh[1]);
                    else glViewport(0, 0, mainprogram->ow[0], mainprogram->oh[0]);
                    glClearColor(0.f, 0.f, 0.f, 0.f);
                    glClear(GL_COLOR_BUFFER_BIT);

                    float sx = 0.0f;
                    float sy = 0.0f;
                    float sc = 1.0f;
                    float op = 1.0f;
                    if (effect->node == lay->lasteffnode[0]) {
                        sx = lay->shiftx->value;
                        sy = lay->shifty->value;
                        sc = lay->scale->value;
                        op = lay->opacity->value;
                    }

                    mainprogram->uniformCache->setInt("interm", 0);
                    mainprogram->uniformCache->setBool("down", false);
                    draw_box(nullptr, black, -1.0f, 1.0f, 2.0f, -2.0f, sx, sy, sc, op, 0, prevfbotex, 0, 0, false);
                } else {
                    glBindFramebuffer(GL_FRAMEBUFFER, effect->fbo);
                    glDrawBuffer(GL_COLOR_ATTACHMENT0);
                    if (stage) glViewport(0, 0, mainprogram->ow[1], mainprogram->oh[1]);
                    else glViewport(0, 0, mainprogram->ow[0], mainprogram->oh[0]);
                    glClearColor(0.f, 0.f, 0.f, 0.f);
                    glClear(GL_COLOR_BUFFER_BIT);

                    float sx = 0.0f;
                    float sy = 0.0f;
                    float sc = 1.0f;
                    float op = 1.0f;
                    if (effect->node == lay->lasteffnode[0]) {
                        sx = lay->shiftx->value;
                        sy = lay->shifty->value;
                        sc = lay->scale->value;
                        op = lay->opacity->value;
                    }

                    mainprogram->uniformCache->setInt("interm", 2);
                    mainprogram->uniformCache->setSampler("Sampler1", 1);
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, prevfbotex);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, effect->tempfbotex);
                    draw_box(nullptr, black, -1.0f, 1.0f, 2.0f, -2.0f, sx, sy, sc, op, 0, effect->tempfbotex, 0, 0,
                             false);
                }
            } else if (effect->isfnr != -1 && effect->onoffbutton->value) {
                auto instance = mainprogram->isfinstances[effect->isfpluginnr][effect->isfinstancenr];

                glBindFramebuffer(GL_FRAMEBUFFER, effect->tempfbo);
                glDrawBuffer(GL_COLOR_ATTACHMENT0);
                if (stage) glViewport(0, 0, mainprogram->ow[1], mainprogram->oh[1]);
                else glViewport(0, 0, mainprogram->ow[0], mainprogram->oh[0]);
                glClearColor(0.f, 0.f, 0.f, 0.f);
                glClear(GL_COLOR_BUFFER_BIT);

                int pos = 0;
                for (int i = 0; i < instance->getParameterInfo().size(); i++) {
                    auto par = instance->getParameterInfo()[i];
                    int oldpos = pos;
                    if (par.type == ISFLoader::PARAM_COLOR) {
                        instance->setParameter(par.name, effect->params[pos]->value, effect->params[pos + 1]->value, effect->params[pos + 2]->value, effect->params[pos + 3]->value);
                        pos += 4;
                    }
                    else if (effect->params[pos]->type == ISFLoader::PARAM_POINT2D) {
                        instance->setParameter(par.name, effect->params[pos]->value, effect->params[pos + 1]->value);
                        pos += 2;
                    }
                    else if (effect->params[pos]->type == ISFLoader::PARAM_BOOL || effect->params[i]->type == ISFLoader::PARAM_EVENT) {
                        instance->setParameter(par.name, (int)effect->params[pos++]->value);
                    }
                    else if (effect->params[pos]->type == ISFLoader::PARAM_LONG) {
                        instance->setParameter(par.name, (int) par.values[effect->params[pos++]->value]);
                    }
                    else {
                        instance->setParameter(par.name, effect->params[pos++]->value);
                    }
                    if (effect->params[oldpos]->type == ISFLoader::PARAM_EVENT) {
                        effect->params[oldpos]->value = 0.0f;
                    }
                }

                instance->bindInputTexture(prevfbotex, 0);

                instance->render(mainmix->time, mainprogram->ow[stage], mainprogram->oh[stage]);

                glUseProgram(mainprogram->ShaderProgram);

                glBindFramebuffer(GL_FRAMEBUFFER, effect->fbo);
                glDrawBuffer(GL_COLOR_ATTACHMENT0);
                if (stage) glViewport(0, 0, mainprogram->ow[1], mainprogram->oh[1]);
                else glViewport(0, 0, mainprogram->ow[0], mainprogram->oh[0]);
                glClearColor(0.f, 0.f, 0.f, 0.f);
                glClear(GL_COLOR_BUFFER_BIT);

                float sx = 0.0f;
                float sy = 0.0f;
                float sc = 1.0f;
                float op = 1.0f;
                if (effect->node == lay->lasteffnode[0]) {
                    sx = lay->shiftx->value;
                    sy = lay->shifty->value;
                    sc = lay->scale->value;
                    op = lay->opacity->value;
                }

                mainprogram->uniformCache->setInt("interm", 2);
                mainprogram->uniformCache->setSampler("Sampler1", 1);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, prevfbotex);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, effect->tempfbotex);
                draw_box(nullptr, black, -1.0f, 1.0f, 2.0f, -2.0f, sx, sy, sc, op, 0, effect->tempfbotex, 0, 0,
                         false);

            } else {
                if (!lay->onhold)
                    draw_box(nullptr, black, -1.0f, 1.0f, 2.0f, -2.0f, sx, sy, sc, op, 0, prevfbotex, 0, 0, false);
            }

            if (effect->node == lay->lasteffnode[0]) {
                if (lay->ndioutput != nullptr) {
                    lay->ndiouttex.setFromExistingTexture(effect->fbotex, mainprogram->ow[stage], mainprogram->oh[stage]);
                    lay->ndioutput->sendFrame(lay->ndiouttex);
                }
            }

            prevfbotex = effect->fbotex;
            prevfbo = effect->fbo;

            mainprogram->uniformCache->setBool("down", false);
            mainprogram->uniformCache->setInt("interm", 0);
            glViewport(0, 0, glob->w, glob->h);
		}
	}


	else if (node->type == VIDEO) {
        Layer *lay = ((VideoNode*)node)->layer;

        if (lay->blendnode) {
            if (lay->blendnode->blendtype == 19 || lay->blendnode->blendtype == 20 || lay->blendnode->blendtype
            == 21) {
                mainprogram->uniformCache->setFloat("colortol", lay->chtol->value);
                mainprogram->uniformCache->setBool("chdir", lay->chdir->value);
                mainprogram->uniformCache->setBool("chinv", lay->chinv->value);
            }
        }
        float xs = 0.0f;
        float ys = 0.0f;
        float scix = 0.0f;
        float sciy = 0.0f;
        float frachd = 1920.0f / 1080.0f;
        float fraco = mainprogram->ow[1] / mainprogram->oh[1];
        float frac;
        if (lay->pos == 1 && lay->comp == true) {
            bool dummy = false;
        }
        if (lay->type == ELEM_IMAGE) {
            ilBindImage(lay->boundimage);
            ilActiveImage((int)lay->frame);
            frac = (float)ilGetInteger(IL_IMAGE_WIDTH) / (float)ilGetInteger(IL_IMAGE_HEIGHT);
        }
        else if (lay->type != ELEM_LIVE){
            if (lay->decresult->height == 0) return;
            frac = (float)(lay->decresult->width) / (float)(lay->decresult->height);
        }
        if (lay->dummy) {
            frac = (float)(lay->video_dec_ctx->width) / (float)(lay->video_dec_ctx->height);
        }
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
        float sx = 0.0f;
        float sy = 0.0f;
        float sc = 1.0f;
        float op = 1.0f;
        if (lay->effects[0].empty()) {
            sx = lay->shiftx->value;
            sy = lay->shifty->value;
            sc = lay->scale->value;
            if (lay->node == lay->lasteffnode[0]) {
                op = lay->opacity->value;
            } else {
                op = 1.0f;
            }
        }
        if (lay->ndisource != nullptr) {
            if (lay->ndisource->hasNewFrame()) {
                if (!lay->ndisource->getLatestFrame(lay->ndiintex)) {
                    std::cout << "Failed to get frame!" << std::endl;
                }
            }
            lay->filename = "";

            glBindFramebuffer(GL_FRAMEBUFFER, lay->fbo);
            glDrawBuffer(GL_COLOR_ATTACHMENT0);
            //glViewport(0, 0, lay->ndiintex.getWidth(), lay->ndiintex.getHeight());
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            draw_box(nullptr, black, -1.0f, 1.0f, 2.0f, -2.0f, sx, sy, sc, op, 0, lay->ndiintex.getTextureID(), 0, 0, false);
        }
        else if (lay->ffglsourcenr != -1) {
            FFGLFramebuffer infbo;
            infbo.fbo = prevfbo;
            infbo.colorTexture = prevfbotex;
            infbo.width = mainprogram->ow[stage];
            infbo.height = mainprogram->oh[stage];
            FFGLFramebuffer outfbo;
            outfbo.fbo = lay->tempfbo;
            outfbo.colorTexture = lay->tempfbotex;
            outfbo.width = mainprogram->ow[stage];
            outfbo.height = mainprogram->oh[stage];

            for (int i = 0; i < lay->instance->parameters.size(); i++) {
                if (lay->ffglparams[i]->type == FF_TYPE_BUFFER) {
                }
                else if (lay->ffglparams[i]->type == FF_TYPE_OPTION) {
                    float optionValue = lay->ffglparams[i]->value;
                    lay->instance->setParameter(i, optionValue);
                }
                else if (lay->ffglparams[i]->type == FF_TYPE_TEXT || lay->ffglparams[i]->type == FF_TYPE_FILE) {
                    lay->instance->setParameter(i, FFGLUtils::PointerToFFMixed((char*)(lay->ffglparams[i]->valuestr.c_str())));
                }
                else if (lay->ffglparams[i]->type == FF_TYPE_EVENT) {
                    if (lay->ffglparams[i]->value == 1.0f) {
                        lay->instance->setParameter(i, lay->ffglparams[i]->value);
                    }
                    lay->ffglparams[i]->value = 0.0f;
                }
                else {
                    lay->instance->setParameter(i, lay->ffglparams[i]->value);
                }
            }

            bool hasBufferParams = false;
            for (const auto &param: lay->instance->getParameters()) {
                if (param.isBufferParameter()) {
                    hasBufferParams = true;
                    break;
                }
            }

            if (!hasBufferParams) {
                static float effectTime = 0.0f;
                static auto lastFrame = std::chrono::high_resolution_clock::now();

                float currentTime = EffectTimer::getTime(); // Starts from 0.0
                lay->instance->setTime(effectTime);
            }

            lay->instance->applyStoredAudioData();

            lay->instance->processFrame({infbo}, outfbo);

            // opacity shift scale
            float sx = 0.0f;
            float sy = 0.0f;
            float sc = 1.0f;
            float op = 1.0f;
            if (lay->effects[0].empty()) {
                sx = lay->shiftx->value;
                sy = lay->shifty->value;
                sc = lay->scale->value;
                if (lay->node == lay->lasteffnode[0]) {
                    op = lay->opacity->value;
                } else {
                    op = 1.0f;
                }
            }

            glBindFramebuffer(GL_FRAMEBUFFER, lay->fbo);
            glDrawBuffer(GL_COLOR_ATTACHMENT0);
            if (stage) glViewport(0, 0, mainprogram->ow[1], mainprogram->oh[1]);
            else glViewport(0, 0, mainprogram->ow[0], mainprogram->oh[0]);
            glClearColor( 0.f, 0.f, 0.f, 0.f );
            glClear(GL_COLOR_BUFFER_BIT);
            draw_box(nullptr, black, -1.0f, 1.0f, 2.0f, -2.0f, sx, sy, sc, op, 0, lay->tempfbotex, 0, 0, false);
        }
        else if (lay->isfsourcenr != -1) {
            auto instance = mainprogram->isfinstances[lay->isfpluginnr][lay->isfinstancenr];

            glBindFramebuffer(GL_FRAMEBUFFER, lay->tempfbo);
            glDrawBuffer(GL_COLOR_ATTACHMENT0);
            if (stage) glViewport(0, 0, mainprogram->ow[1], mainprogram->oh[1]);
            else glViewport(0, 0, mainprogram->ow[0], mainprogram->oh[0]);
            glClearColor( 0.f, 0.f, 0.f, 0.f );
            glClear(GL_COLOR_BUFFER_BIT);

            int pos = 0;
            for (int i = 0; i < instance->getParameterInfo().size(); i++) {
                auto par = instance->getParameterInfo()[i];
                int oldpos = pos;
                if (par.type == ISFLoader::PARAM_COLOR) {
                    instance->setParameter(par.name, lay->isfparams[pos]->value, lay->isfparams[pos + 1]->value, lay->isfparams[pos + 2]->value, lay->isfparams[pos + 3]->value);
                    pos += 4;
                }
                else if (lay->isfparams[pos]->type == ISFLoader::PARAM_POINT2D) {
                    instance->setParameter(par.name, lay->isfparams[pos]->value, lay->isfparams[pos + 1]->value);
                    pos += 2;
                }
                else if (lay->isfparams[pos]->type == ISFLoader::PARAM_BOOL || lay->isfparams[i]->type == ISFLoader::PARAM_EVENT) {
                    instance->setParameter(par.name, (int)lay->isfparams[pos++]->value);
                }
                else if (lay->isfparams[pos]->type == ISFLoader::PARAM_LONG) {
                    instance->setParameter(par.name, (int) par.values[lay->isfparams[pos++]->value]);
                }
                else {
                    instance->setParameter(par.name, lay->isfparams[pos++]->value);
                }
                if (lay->isfparams[oldpos]->type == ISFLoader::PARAM_EVENT) {
                    lay->isfparams[oldpos]->value = 0.0f;
                }
            }

            instance->render(mainmix->time, mainprogram->ow[stage], mainprogram->oh[stage]);

            glUseProgram(mainprogram->ShaderProgram);

            glBindFramebuffer(GL_FRAMEBUFFER, lay->fbo);
            glDrawBuffer(GL_COLOR_ATTACHMENT0);
            if (stage) glViewport(0, 0, mainprogram->ow[1], mainprogram->oh[1]);
            else glViewport(0, 0, mainprogram->ow[0], mainprogram->oh[0]);
            glClearColor( 0.f, 0.f, 0.f, 0.f );
            glClear(GL_COLOR_BUFFER_BIT);

            // opacity shift scale
            float sx = 0.0f;
            float sy = 0.0f;
            float sc = 1.0f;
            float op = 1.0f;
            if (lay->effects[0].empty()) {
                sx = lay->shiftx->value;
                sy = lay->shifty->value;
                sc = lay->scale->value;
                if (lay->node == lay->lasteffnode[0]) {
                    op = lay->opacity->value;
                } else {
                    op = 1.0f;
                }
            }

            draw_box(nullptr, black, -1.0f, 1.0f, 2.0f, -2.0f, sx, sy, sc, op, 0, lay->tempfbotex, 0, 0, false);
        }
        else {
            glBindFramebuffer(GL_FRAMEBUFFER, lay->fbo);
            glDrawBuffer(GL_COLOR_ATTACHMENT0);
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            if (!lay->onhold && lay->filename != "") {
                if (lay->changeinit == 2) {
                    draw_box(nullptr, black, -1.0f, 1.0f, 2.0f, -2.0f, sx, sy, sc, op, 0, lay->texture, 0, 0, false);
                } else {
                    draw_box(nullptr, black, -1.0f, 1.0f, 2.0f, -2.0f, sx, sy, sc, op, 0, lay->oldtexture, 0, 0, false);
                }
            }
        }

        if (lay->effects[0].empty()) {
            if (lay->ndioutput != nullptr) {
                lay->ndiouttex.setFromExistingTexture(lay->fbotex, mainprogram->ow[stage], mainprogram->oh[stage]);
                lay->ndioutput->sendFrame(lay->ndiouttex);
            }
        }

        prevfbotex = lay->fbotex;
        prevfbo = lay->fbo;

        glViewport(0, 0, glob->w, glob->h);

        lay->newtexdata = false;
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
                do {
                    if (bnode->fbo != -1) {
                        glDeleteFramebuffers(1, &bnode->fbo);
                    }
                    if (bnode->fbotex != -1) {
                        glDeleteTextures(1, &bnode->fbotex);
                    }
                    GLuint rettex;
                    if (stage == 0) {
                        rettex = mainprogram->grab_from_texpool(mainprogram->ow[0], mainprogram->oh[0], GL_RGBA8);
                    } else {
                        rettex = mainprogram->grab_from_texpool(mainprogram->ow[1], mainprogram->oh[1], GL_RGBA8);
                    }
                    if (rettex != -1) {
                        bnode->fbotex = rettex;
                    } else {
                        glGenTextures(1, &(bnode->fbotex));
                        glBindTexture(GL_TEXTURE_2D, bnode->fbotex);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
                        if (stage == 0) {
                            glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, mainprogram->ow[0], mainprogram->oh[0]);
                        } else {
                            glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, mainprogram->ow[1], mainprogram->oh[1]);
                        }
                        mainprogram->texintfmap[bnode->fbotex] = GL_RGBA8;
                    }
                    GLuint retfbo;
                    retfbo = mainprogram->grab_from_fbopool();
                    if (retfbo != -1) {
                        bnode->fbo = retfbo;
                    } else {
                        glGenFramebuffers(1, &(bnode->fbo));
                    }
                    glBindFramebuffer(GL_FRAMEBUFFER, bnode->fbo);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bnode->fbotex, 0);
                } while (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE);
            }

            if (bnode->intex != -1 && bnode->in2tex != -1) {
                if (bnode->ffglmixernr != -1) {
                    FFGLFramebuffer infbo1;
                    infbo1.fbo = 0;
                    infbo1.colorTexture = bnode->intex;
                    infbo1.width = mainprogram->ow[stage];
                    infbo1.height = mainprogram->oh[stage];
                    FFGLFramebuffer infbo2;
                    infbo2.fbo = 0;
                    infbo2.colorTexture = bnode->in2tex;
                    infbo2.width = mainprogram->ow[stage];
                    infbo2.height = mainprogram->oh[stage];
                    FFGLFramebuffer outfbo;
                    outfbo.fbo = bnode->fbo;
                    outfbo.colorTexture = bnode->fbotex;
                    outfbo.width = mainprogram->ow[stage];
                    outfbo.height = mainprogram->oh[stage];

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, infbo1.colorTexture);
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, infbo2.colorTexture);

                    for (int i = 0; i < bnode->instance->parameters.size(); i++) {
                        bnode->instance->setParameter(i, bnode->ffglparams[i]->value);
                    }

                    static float effectTime = 0.0f;
                    static auto lastFrame = std::chrono::high_resolution_clock::now();

                    float currentTime = EffectTimer::getTime(); // Starts from 0.0
                    bnode->instance->setTime(effectTime);

                    bnode->instance->processFrame({infbo1, infbo2}, outfbo);
                }
                else if (bnode->isfmixernr != -1) {
                    auto instance = mainprogram->isfinstances[bnode->isfpluginnr][bnode->isfinstancenr];

                    glBindFramebuffer(GL_FRAMEBUFFER, bnode->fbo);
                    glDrawBuffer(GL_COLOR_ATTACHMENT0);
                    if (stage) glViewport(0, 0, mainprogram->ow[1], mainprogram->oh[1]);
                    else glViewport(0, 0, mainprogram->ow[0], mainprogram->oh[0]);
                    glClearColor( 0.f, 0.f, 0.f, 0.f );
                    glClear(GL_COLOR_BUFFER_BIT);

                    int pos = 0;
                    for (int i = 0; i < instance->getParameterInfo().size(); i++) {
                        auto par = instance->getParameterInfo()[i];
                        int oldpos = pos;
                        if (par.type == ISFLoader::PARAM_COLOR) {
                            instance->setParameter(par.name, bnode->isfparams[pos]->value, bnode->isfparams[pos + 1]->value, bnode->isfparams[pos + 2]->value, bnode->isfparams[pos + 3]->value);
                            pos += 4;
                        }
                        else if (bnode->isfparams[pos]->type == ISFLoader::PARAM_POINT2D) {
                            instance->setParameter(par.name, bnode->isfparams[pos]->value, bnode->isfparams[pos + 1]->value);
                            pos += 2;
                        }
                        else if (bnode->isfparams[pos]->type == ISFLoader::PARAM_BOOL || bnode->isfparams[i]->type == ISFLoader::PARAM_EVENT) {
                            instance->setParameter(par.name, (int)bnode->isfparams[pos++]->value);
                        }
                        else if (bnode->isfparams[pos]->type == ISFLoader::PARAM_LONG) {
                            instance->setParameter(par.name, (int) par.values[bnode->isfparams[pos++]->value]);
                        }
                        else {
                            instance->setParameter(par.name, bnode->isfparams[pos++]->value);
                        }
                        if (bnode->isfparams[oldpos]->type == ISFLoader::PARAM_EVENT) {
                            bnode->isfparams[oldpos]->value = 0.0f;
                        }
                    }

                    auto inputs = instance->getInputInfo();
                    instance->bindInputTexture(bnode->intex, 0);
                    instance->bindInputTexture(bnode->in2tex, 1);
                    instance->render(mainmix->time, mainprogram->ow[stage], mainprogram->oh[stage]);

                    //draw_box(nullptr, black, -1.0f, 1.0f, 2.0f, -2.0f, sx, sy, sc, op, 0, -1, 0, 0, false);

                    glUseProgram(mainprogram->ShaderProgram);
                }
                else {
                    glBindFramebuffer(GL_FRAMEBUFFER, bnode->fbo);
                    glDrawBuffer(GL_COLOR_ATTACHMENT0);
                    glClearColor( 0.f, 0.f, 0.f, 0.f );
                    glClear(GL_COLOR_BUFFER_BIT);
                    if (stage) {
                        glViewport(0, 0, mainprogram->ow[1], mainprogram->oh[1]);
                    } else {
                        glViewport(0, 0, mainprogram->ow[0], mainprogram->oh[0]);
                    }

                    mainprogram->uniformCache->setFloat("mixfac", bnode->mixfac->value);
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, bnode->intex);

                    if (stage) mainprogram->uniformCache->setFloat("cf", mainmix->crossfadecomp->value);

                    mainprogram->uniformCache->setInt("mixmode", bnode->blendtype);
                    if (bnode->blendtype == 19 || bnode->blendtype == 20 || bnode->blendtype == 21) {
                        mainprogram->uniformCache->setFloat("chred", bnode->chred);
                        mainprogram->uniformCache->setFloat("chgreen", bnode->chgreen);
                        mainprogram->uniformCache->setFloat("chblue", bnode->chblue);
                    }
                    if (bnode->blendtype == 18) {
                        mainprogram->uniformCache->setBool("inlayer", true);
                        if (bnode->wipetype > -1) mainprogram->uniformCache->setBool("wipe", true);
                        mainprogram->uniformCache->setInt("wkind", bnode->wipetype);
                        mainprogram->uniformCache->setInt("dir", bnode->wipedir);
                        mainprogram->uniformCache->setFloat("xpos", bnode->wipex->value);
                        mainprogram->uniformCache->setFloat("ypos", bnode->wipey->value);
                    }
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, bnode->in2tex);
                    glActiveTexture(GL_TEXTURE0);
                    glBindVertexArray(mainprogram->fbovao);
                    mainprogram->uniformCache->setFloat("opacity", 1.0f);
                    glDisable(GL_BLEND);
                    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                    glEnable(GL_BLEND);

                    mainprogram->uniformCache->setBool("inlayer", false);
                    mainprogram->uniformCache->setBool("wipe", false);
                    mainprogram->uniformCache->setInt("mixmode", 0);
                    if (mainprogram->prevmodus) {
                        mainprogram->uniformCache->setFloat("cf", mainmix->crossfade->value);
                    }
                    else {
                        mainprogram->uniformCache->setFloat("cf", mainmix->crossfadecomp->value);
                    }
                }

				prevfbotex = bnode->fbotex;
				prevfbo = bnode->fbo;
				
				glViewport(0, 0, glob->w, glob->h);
			}
			else {
				if (prevnode == bnode->in2) {
					node->walked = true;			//for when first layer is muted
					glBindFramebuffer(GL_FRAMEBUFFER, bnode->fbo);
					glDrawBuffer(GL_COLOR_ATTACHMENT0);
					if (stage) glViewport(0, 0, mainprogram->ow[1], mainprogram->oh[1]);
					else glViewport(0, 0, mainprogram->ow[0], mainprogram->oh[0]);
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
        MixNode *mnode = (MixNode*)node;
		if (mnode->mixfbo == -1) {
		    mnode->newmixfbo = false;
			glGenTextures(1, &(mnode->mixtex));
			glBindTexture(GL_TEXTURE_2D, mnode->mixtex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			if (stage == 0) {
				glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, mainprogram->ow[0], mainprogram->oh[0]);
			}
			else {
				glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, mainprogram->ow[1], mainprogram->oh[1]);
			}

			glGenFramebuffers(1, &(mnode->mixfbo));
			glBindFramebuffer(GL_FRAMEBUFFER, mnode->mixfbo);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mnode->mixtex, 0);
		}
		glBindFramebuffer(GL_FRAMEBUFFER, mnode->mixfbo);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		if (stage) glViewport(0, 0, mainprogram->ow[1], mainprogram->oh[1]);
		else glViewport(0, 0, mainprogram->ow[0], mainprogram->oh[0]);
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        draw_box(nullptr, black, -1.0f, 1.0f, 2.0f, -2.0f, prevfbotex);

        if (mnode->ndioutput != nullptr) {
            mnode->ndiouttex.setFromExistingTexture(mnode->mixtex, mainprogram->ow[stage], mainprogram->oh[stage]);
            mnode->ndioutput->sendFrame(mnode->ndiouttex);
        }

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
		if (node->out.size() == 0) {
            break;
        }
		node = node->out[0];
	} while (1);
}

void walk_nodes(bool stage) {
	std::vector<Node*> fromnodes;
	if (stage == 0) {
		for (int i = 0; i < mainprogram->nodesmain->mixnodes[0].size(); i++) {
			fromnodes.push_back(mainprogram->nodesmain->mixnodes[0][i]);
		}
	}
	else {
		for (int i = 0; i < mainprogram->nodesmain->mixnodes[1].size(); i++) {
			fromnodes.push_back(mainprogram->nodesmain->mixnodes[1][i]);
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
    for (int i = 0; i < 4; i = i + 2) {
        if (stage == i / 2) {
            int mutes = 0;
            bool muted = false;
            for (int j = 0; j < mainmix->layers[i].size(); j++) {
                Layer *lay = mainmix->layers[i][j];
                if (lay->mutebut->value) mutes++;
                walk_forward(lay->node);
                onestepfrom(i / 2, lay->node, nullptr, -1, -1);
            }
            if (mutes == mainmix->layers[i].size()) {
                muted = true;
                glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->nodesmain->mixnodes[i / 2][0]->mixfbo);
                glDrawBuffer(GL_COLOR_ATTACHMENT0);
                glClearColor(0, 0, 0, 0);
                glClear(GL_COLOR_BUFFER_BIT);
            }
            mutes = 0;
            for (int j = 0; j < mainmix->layers[i + 1].size(); j++) {
                Layer *lay = mainmix->layers[i + 1][j];
                if (lay->mutebut->value) mutes++;
                walk_forward(lay->node);
                onestepfrom(i / 2, lay->node, nullptr, -1, -1);
            }
            if (mutes == mainmix->layers[i + 1].size()) {
                muted = true;
                glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->nodesmain->mixnodes[i / 2][1]->mixfbo);
                glDrawBuffer(GL_COLOR_ATTACHMENT0);
                glClearColor(0, 0, 0, 0);
                glClear(GL_COLOR_BUFFER_BIT);
            }
            if (muted) {
                mainprogram->bnodeend[i / 2]->intex = -1;
                mainprogram->bnodeend[i / 2]->in2tex = -1;
                onestepfrom(i / 2, mainprogram->bnodeend[i / 2], mainprogram->nodesmain->mixnodes[i / 2][0],
                            mainprogram->nodesmain->mixnodes[i / 2][0]->mixtex, mainprogram->nodesmain->mixnodes[i / 2][0]->mixfbo);
                onestepfrom(i / 2, mainprogram->bnodeend[i / 2], mainprogram->nodesmain->mixnodes[i / 2][1],
                            mainprogram->nodesmain->mixnodes[i / 2][1]->mixtex, mainprogram->nodesmain->mixnodes[i / 2][1]->mixfbo);
            }
        }
    }

	mainprogram->directmode = false;
}
		

bool display_mix() {
    mainprogram->directmode = true;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDrawBuffer(GL_BACK_LEFT);

	float xs = 0.0f;
	float ys = 0.0f;
	float fraco = mainprogram->ow[1] / mainprogram->oh[1];
	float frachd = 1920.0f / 1080.0f;
	fraco = frachd;
	if (fraco > frachd) {
		ys = 0.15f * (1.0f - frachd / fraco);
	}
	else {
		xs = 0.15f * (1.0f - fraco / frachd);
	}
	MixNode* node;
	Boxx *box;
	if (mainprogram->prevmodus) {
		mainprogram->uniformCache->setFloat("cf", mainmix->crossfade->value);
	}
	else {
		mainprogram->uniformCache->setFloat("cf", mainmix->crossfadecomp->value);
	}
    mainprogram->uniformCache->setBool("wipe", false);
    mainprogram->uniformCache->setInt("mixmode", 0);


    if (mainprogram->prevmodus) {
		if (mainmix->wipe[0] > -1) {
			mainprogram->uniformCache->setInt("mixmode", 18);
			mainprogram->uniformCache->setBool("wipe", true);
			mainprogram->uniformCache->setInt("wkind", mainmix->wipe[0]);
			mainprogram->uniformCache->setInt("dir", mainmix->wipedir[0]);
			mainprogram->uniformCache->setFloat("xpos", mainmix->wipex[0]->value);
			mainprogram->uniformCache->setFloat("ypos", mainmix->wipey[0]->value);
		}
        // bottom monitor in preview modus
		node = (MixNode*)mainprogram->nodesmain->mixnodes[0][0];
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, node->mixtex);
		node = (MixNode*)mainprogram->nodesmain->mixnodes[0][1];
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, node->mixtex);
		glActiveTexture(GL_TEXTURE0);
		node = (MixNode*)mainprogram->nodesmain->mixnodes[0][2];
		box = mainprogram->mainmonitor;
        box->vtxcoords->x1 = -0.15f;
        box->vtxcoords->y1 = -1.0f;
        box->vtxcoords->w = 0.3f;
        box->vtxcoords->h = mainprogram->monh;
        box->upvtxtoscr();
        if (mainmix->wipe[0] > -1) {
            glBindFramebuffer(GL_FRAMEBUFFER, node->mixfbo);
            glDrawBuffer(GL_COLOR_ATTACHMENT0);
            glViewport(0, 0, mainprogram->ow[0], mainprogram->oh[0]);
            glClearColor(0, 0, 0, 0);
            glClear(GL_COLOR_BUFFER_BIT);
            draw_box(nullptr, black, -1.0f, 1.0f, 2.0f, -2.0f, -1);
        }
        mainprogram->uniformCache->setBool("wipe", false);
        mainprogram->uniformCache->setInt("mixmode", 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDrawBuffer(GL_BACK_LEFT);
        glViewport(0, 0, glob->w, glob->h);
		draw_box(red, black, box->vtxcoords->x1 + xs * 2.0f, box->vtxcoords->y1 + ys * 2.0f, box->vtxcoords->w - xs * 4.0f, box->vtxcoords->h - ys * 4.0f, node->mixtex);
		mainprogram->mainmonitor->in();

		if (mainmix->wipe[1] > -1) {
			mainprogram->uniformCache->setFloat("cf", mainmix->crossfadecomp->value);
			mainprogram->uniformCache->setInt("mixmode", 18);
			mainprogram->uniformCache->setBool("wipe", true);
			mainprogram->uniformCache->setInt("wkind", mainmix->wipe[1]);
			mainprogram->uniformCache->setInt("dir", mainmix->wipedir[1]);
			mainprogram->uniformCache->setFloat("xpos", mainmix->wipex[1]->value);
			mainprogram->uniformCache->setFloat("ypos", mainmix->wipey[1]->value);
			node = (MixNode*)mainprogram->nodesmain->mixnodes[1][0];
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, node->mixtex);
			node = (MixNode*)mainprogram->nodesmain->mixnodes[1][1];
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, node->mixtex);
			glActiveTexture(GL_TEXTURE0);
		}
        // top monitor in preview modus
		node = (MixNode*)mainprogram->nodesmain->mixnodes[1][2];
		box = mainprogram->outputmonitor;
        if (mainmix->wipe[1] > -1) {
            glBindFramebuffer(GL_FRAMEBUFFER, node->mixfbo);
            glDrawBuffer(GL_COLOR_ATTACHMENT0);
            glViewport(0, 0, mainprogram->ow[1], mainprogram->oh[1]);
            glClearColor(0, 0, 0, 0);
            glClear(GL_COLOR_BUFFER_BIT);
            draw_box(nullptr, black, -1.0f, 1.0f, 2.0f, -2.0f, -1);
        }
        mainprogram->uniformCache->setBool("wipe", false);
        mainprogram->uniformCache->setInt("mixmode", 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDrawBuffer(GL_BACK_LEFT);
        glViewport(0, 0, glob->w, glob->h);
		draw_box(red, black, box->vtxcoords->x1 + xs, box->vtxcoords->y1 + ys, box->vtxcoords->w - xs * 2.0f, box->vtxcoords->h - ys * 2.0f, node->mixtex);
		mainprogram->outputmonitor->in();
	}
	else {
		mainprogram->uniformCache->setBool("wipe", false);
		mainprogram->uniformCache->setInt("mixmode", 0);
		if (mainmix->wipe[1] > -1) {
			mainprogram->uniformCache->setFloat("cf", mainmix->crossfadecomp->value);
			mainprogram->uniformCache->setInt("mixmode", 18);
			mainprogram->uniformCache->setBool("wipe", true);
			mainprogram->uniformCache->setInt("wkind", mainmix->wipe[1]);
			mainprogram->uniformCache->setInt("dir", mainmix->wipedir[1]);
			mainprogram->uniformCache->setFloat("xpos", mainmix->wipex[1]->value);
			mainprogram->uniformCache->setFloat("ypos", mainmix->wipey[1]->value);
			node = (MixNode*)mainprogram->nodesmain->mixnodes[1][0];
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, node->mixtex);
			node = (MixNode*)mainprogram->nodesmain->mixnodes[1][1];
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, node->mixtex);
			glActiveTexture(GL_TEXTURE0);
		}
        // main monitor in performance modus
		node = (MixNode*)mainprogram->nodesmain->mixnodes[1][2];
		box = mainprogram->mainmonitor;
        box->vtxcoords->x1 = -0.3f;
        box->vtxcoords->y1 = -1.0f;
        box->vtxcoords->w = 0.6f;
        box->vtxcoords->h = mainprogram->monh * 2.0f;
        box->upvtxtoscr();
        if (mainmix->wipe[1] > -1) {
            glBindFramebuffer(GL_FRAMEBUFFER, node->mixfbo);
            glDrawBuffer(GL_COLOR_ATTACHMENT0);
            glViewport(0, 0, mainprogram->ow[1], mainprogram->oh[1]);
            glClearColor(0, 0, 0, 0);
            glClear(GL_COLOR_BUFFER_BIT);
            draw_box(nullptr, black, -1.0f, 1.0f, 2.0f, -2.0f, -1);
        }
        mainprogram->uniformCache->setBool("wipe", false);
        mainprogram->uniformCache->setInt("mixmode", 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDrawBuffer(GL_BACK_LEFT);
        glViewport(0, 0, glob->w, glob->h);
		draw_box(red, black, box->vtxcoords->x1 + xs * 2.0f, box->vtxcoords->y1 + ys * 2.0f, box->vtxcoords->w - xs * 4.0f, box->vtxcoords->h - ys * 4.0f, node->mixtex);
		mainprogram->mainmonitor->in();
	}

    // draw deck monitors
	if (mainprogram->prevmodus) {
        node = (MixNode*)mainprogram->nodesmain->mixnodes[0][0];
		box = mainprogram->deckmonitor[0];
		draw_box(red, box->acolor, box->vtxcoords->x1 + xs, box->vtxcoords->y1 + ys, box->vtxcoords->w - xs * 2.0f, box->vtxcoords->h - ys * 2.0f, node->mixtex);
		box->in();
		node = (MixNode*)mainprogram->nodesmain->mixnodes[0][1];
		box = mainprogram->deckmonitor[1];
		draw_box(red, black, box->vtxcoords->x1 + xs, box->vtxcoords->y1 + ys, box->vtxcoords->w - xs * 2.0f, box->vtxcoords->h - ys * 2.0f, node->mixtex);
        box->in();
	}
	else {
		node = (MixNode*)mainprogram->nodesmain->mixnodes[1][0];
		box = mainprogram->deckmonitor[0];
		draw_box(red, black, box->vtxcoords->x1 + xs, box->vtxcoords->y1 + ys, box->vtxcoords->w - xs * 2.0f, box->vtxcoords->h - ys * 2.0f, node->mixtex);
        box->in();
		node = (MixNode*)mainprogram->nodesmain->mixnodes[1][1];
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
		else if (lay->pos < mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos || lay->pos > mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos + 2) {
            continue;
        }
		Boxx* box = lay->node->vidbox;
        box->upvtxtoscr();
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
                        bool clipup = false;
						if (mainprogram->dragbinel) {
							mainprogram->laymenu1->state = 0;
							mainprogram->laymenu2->state = 0;
							mainprogram->newlaymenu->state = 0;
                            int pos = std::find(mainmix->currlays[!mainprogram->prevmodus].begin(), mainmix->currlays[!mainprogram->prevmodus].end(), lay) - mainmix->currlays[!mainprogram->prevmodus].begin();
                            if (pos == mainmix->currlays[!mainprogram->prevmodus].size()) {
                                pos = -1;
                            }
                            mainmix->open_dragbinel(lay, pos);

                            if (mainprogram->dragclip) {
                                if (lay == mainprogram->draglay) {
                                    mainprogram->draglay->clips->erase(std::find(mainprogram->draglay->clips->begin(),
                                                                                mainprogram->draglay->clips->end(),
                                                                                mainprogram->dragclip));
                                    delete mainprogram->dragclip;
                                    mainprogram->dragclip = nullptr;
                                    clipup = true;
                                }
                            }
                            mainprogram->leftmouse = false;
						}
						mainprogram->rightmouse = true;
						binsmain->handle(0);
						enddrag();
                        mainprogram->rightmouse = false;
                        if (clipup) {
                            lay->queueing = true;
                            mainprogram->queueing = true;
                        }
                        mainprogram->recundo = true;
						return;
					}
				}
				else if (cond1 && ((endx && mainprogram->mx > deck * (glob->w / 2.0f) && mainprogram->mx < (deck + 1) * (glob->w / 2.0f)) || (box->scrcoords->x1 - mainprogram->xvtxtoscr(0.075f) < mainprogram->mx && mainprogram->mx < box->scrcoords->x1 + mainprogram->xvtxtoscr(0.075f)))) {
					// handle dragging things to the end of the last visible layer monitor of deck
					if (mainprogram->lmover) {
						Layer* inlay = mainmix->add_layer(layers, lay->pos + endx);
						if (inlay->pos == mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos + 3) mainmix->scenes[deck][mainmix->currscene[deck]]->scrollpos++;
						if (mainprogram->dragbinel) {
                            mainmix->open_dragbinel(inlay, -1);
                        }
                        mainprogram->lmover = false;
                        mainprogram->leftmouse = false;
						mainprogram->rightmouse = true;
						binsmain->handle(0);
						enddrag();
                        mainprogram->rightmouse = false;
                        mainprogram->recundo = true;
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
                                    mainmix->open_dragbinel(lay, -1);
								}
								mainprogram->rightmouse = true;
								binsmain->handle(0);
								enddrag();
                                mainprogram->rightmouse = false;
                                mainprogram->recundo = true;
								return;
							}
						}
					}
				}
			}
		}
	}
}


int osc_param(const char *path, const char *types, lo_arg **argv, int argc, lo_message m, void *data) {
	Param *par = (Param*)data;
	par->value = par->range[0] + argv[0]->f * (par->range[1] - par->range[0]);
	return 0;
}


bool get_imagetex(Layer *lay, std::string path) {
	lay->dummy = 1;
    lay->transfered = true;
	lay->open_image(path);
	if (lay->filename == "") {
        return false;
    }
    lay->processed = true;

    return true;
}


bool get_videotex(Layer *lay, std::string path) {

    // get the middle frame of this video and put it in a GL texture, as representation for the video

    GLenum err;
    lay->dummy = 1;

    lay->transfered = true;
    lay->open_video(0, path, true, true);
    if (mainprogram->openerr) {
        mainprogram->openerr = false;
        mainprogram->errlays.push_back(lay);
        lay->close();
        return false;
    }
    std::unique_lock<std::mutex> olock(lay->endopenlock);
    lay->endopenvar.wait(olock, [&] { return lay->opened; });
    lay->opened = false;
    olock.unlock();
    if (mainprogram->openerr) {
        printf("error loading video texture!\n");
        mainprogram->errlays.push_back(lay);
        mainprogram->openerr = false;
        lay->close();
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

    return true;
}

bool get_layertex(Layer *lay, std::string path) {
    // get the middle frame of this layer files' video and put it in a GL texture, as representation for the video
    lay->dummy = true;
    lay->pos = 0;
    lay->node = mainprogram->nodesmain->currpage->add_videonode(2);
    lay->node->layer = lay;
    lay->transfered = true;
    Layer *l = mainmix->open_layerfile(path, lay, true, true);
    std::unique_lock<std::mutex> olock(l->endopenlock);
    l->endopenvar.wait(olock, [&] { return l->opened; });
    l->opened = false;
    olock.unlock();
    mainprogram->getvideotexlayers[std::find(mainprogram->getvideotexlayers.begin(), mainprogram->getvideotexlayers.end(), lay) - mainprogram->getvideotexlayers.begin()] = l;
    lay->close();
    lay = l;
    if (lay->filename == "") return -1;
    lay->node->calc = true;
    lay->node->walked = false;
    lay->playbut->value = true;
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
    lay->initialize(lay->decresult->width, lay->decresult->height);
    std::vector<Layer*> layers;
    layers.push_back(lay);
    lay->layers = &layers;
    mainmix->reconnect_all(layers);

    lay->keyframe = false;
    lay->processed = true;

    return true;
}

bool get_deckmixtex(Layer *lay, std::string path) {
    // get the representational jpeg of this deck/mix that was saved with/in the deck/mix file and put it in a GL
    // texture

	std::string result = mainprogram->deconcat_files(path);
	bool concat = (result != "");
	std::ifstream rfile;
	if (concat) rfile.open(result);
	else rfile.open(path);
	std::string istring;

	int32_t num;
	if (concat) {
		std::fstream bfile;
		bfile.open(path, ios::in | ios::binary);
        bfile.read((char*)& num, 4); // 20011975
		bfile.read((char*)& num, 4);
        mainprogram->result = result;
        mainprogram->resnum = num;
        if (lay) lay->processed = true;
	}
	else {
        mainprogram->resnum = -1;
        return false;
    }

    return true;
}


void handle_scenes(Scene* scene) {
	// Draw scene boxes
	float red[] = { 1.0, 0.5, 0.5, 1.0 };
	for (int i = 3; i > -1; i--) {
		Boxx* box = mainmix->scenes[scene->deck][i]->box;
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
		Boxx* box = mainmix->scenes[scene->deck][i]->box;
		Button* but = mainmix->scenes[scene->deck][i]->button;
		box->acolor[0] = 0.0;
		box->acolor[1] = 0.0;
		box->acolor[2] = 0.0;
		box->acolor[3] = 1.0;
        if (box->in()) {
            found = true;
            if (mainprogram->menuactivation && !mainprogram->menuondisplay) {
                mainprogram->parammenu3->state = 2;
                mainmix->learnparam = nullptr;
                mainmix->learnbutton = but;
                mainprogram->menuactivation = false;
            } else {
                /*if (but != mainprogram->onscenebutton) {
                    mainprogram->onscenedeck = scene->deck;
                    mainprogram->onscenebutton = but;
                    mainprogram->onscenemilli = 0;
                }*/
                if (((mainprogram->leftmouse) && !mainprogram->menuondisplay && !mainprogram->swappingscene)) {
                    mainprogram->recundo = false;
                    // switch scenes
                    Scene *si = mainmix->scenes[scene->deck][i];
                    //if (i == mainmix->currscene[scene->deck]) continue;

                    mainprogram->swappingscene = true;

                    if (mainprogram->shift) {
                        bool dck = 0;
                        if (si == mainmix->scenes[0][i]) {
                            dck = 1;
                        }
                        mainmix->scenes[dck][i]->switch_to(true);
                        mainmix->currscene[dck] = i;
                    }
                    si->switch_to(true);

                    mainmix->currscene[scene->deck] = i;
                    mainmix->setscene = -1;
                    si->loaded = false;
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
    /*if (!found && mainprogram->onscenedeck == scene->deck) {
        mainprogram->onscenebutton = nullptr;
        mainprogram->onscenemilli = 0.0f;
    }*/
}



void do_text_input(float x, float y, float sx, float sy, int mx, int my, float width, int smflag, PrefItem* item) {
    do_text_input(x, y, sx, sy, mx, my, width, smflag, item, false);
}

void do_text_input(float x, float y, float sx, float sy, int mx, int my, float width, int smflag, PrefItem *item, bool directdraw) {
		// handle display and mouse selection of keyboard input
	mainprogram->tooltipmilli = 0.0f;
	float textw;
	std::vector<float> textwvec;
	std::vector<float> totvec = render_text(mainprogram->inputtext, nullptr, white, x, y, sx, sy, smflag, 0, 0);
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
				std::vector<float> parttextvec = render_text(part, nullptr, white, x, y, sx, sy, smflag, 0, 0);
				float parttextw = textwvec_total(parttextvec);
				mainprogram->startpos = mainprogram->xvtxtoscr(cps - parttextw);
				break;
			}
		}
	}
	float distin = 0.0f;
	if (mainprogram->leftmousedown || mainprogram->leftmouse || mainprogram->orderleftmousedown || mainprogram->orderleftmouse) {
		// clicking inside the edited string
        bool found = false;
		if (totvec.size()) {
		    for (int j = 0; j < totvec.size() + 1; j++) {
		        if (my < mainprogram->yvtxtoscr(1.0f - (y - 0.005f)) && my > mainprogram->yvtxtoscr(1.0f - (y + (mainprogram->texth / 2.6f)
		        / (2070.0f / glob->h)))) {
                    float maxx;
                    if (j == totvec.size()) {
                        maxx = glob->w;
                    }
                    else {
                        maxx = mainprogram->xvtxtoscr(x + 1.0f + distin
                                                      + (j > 0) * totvec[j - 1 + (j == 0)] / 2.0f + totvec[j] / 2.0f);
                    }
		            if (mx >= mainprogram->xvtxtoscr(x + 1.0f + distin) * (j != 0) - mainprogram->startpos && mx <= maxx - mainprogram->startpos) {
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
		    	if (j != totvec.size()) {
		    		distin += totvec[j] / 2.0f;
		    	}
		    }
		}
		if (found == false) {
			//when clicked outside path, cancel edit
            mainmix->adaptnumparam = nullptr;
            mainmix->adapttextparam = nullptr;
			if (item) item->renaming = false;
			if (mainmix->retargeting) {
                mainmix->renaming = false;
			}
			mainprogram->renaming = EDIT_NONE;
			end_input();
		}
        mainprogram->recundo = false;
	}
	if (mainprogram->renaming != EDIT_NONE) {
		std::string part = mainprogram->inputtext.substr(0, mainprogram->cursorpos0);
		textwvec = render_text(part, nullptr, white, x, y, sx, sy, smflag, 0, 0);
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
							std::vector<float> parttextvec = render_text(part, nullptr, white, x, y, sx, sy, smflag, 0, 0);
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
                int psize = (int)((sy * 24000.0f * h2 / 1346.0f) * (0.5625f / ((float)h2 / (float)w2)));
                mainprogram->texth = mainprogram->yscrtovtx(psize * 3);
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
			textwvec = render_text(part, nullptr, white, x, y, sx, sy, smflag, 0, 0);
			float textw1 = textwvec_total(textwvec);
			part = mainprogram->inputtext.substr(c1, c2 - c1);
			textwvec = render_text(part, nullptr, white, x + textw1, y, sx, sy, smflag, 0, 0);
			float textw2 = textwvec_total(textwvec);

            float box_y = y - 0.005f - (smflag > 0) * 0.005f;
            float box_height = 0.005f + (mainprogram->texth / 2.6f) / (2070.0f / glob->h) - (smflag > 0) * (mainprogram->texth / 4.2f) + 0.005f + (smflag > 0) * 0.005f;
            draw_box(white, white, x + textw1, box_y, textw2, box_height, -1, false, false, true); // last parameter: graphic inversion
		}
	}
}
	



void enddrag() {
    bool found = false;
    std::vector<Layer*> &lvec1 = choose_layers(0);
    for (Layer *lay : lvec1) {
        if (lay->queueing) {
            found = true;
            break;
        }
    }
    std::vector<Layer*> &lvec2 = choose_layers(1);
    for (Layer *lay : lvec2) {
        if (lay->queueing) {
            found = true;
            break;
        }
    }
    if (!found) mainprogram->queueing = false;

	if (mainprogram->dragbinel) {
		//if (mainprogram->dragbinel->path.rfind, ".layer", nullptr != std::string::npos) {
		//	if (mainprogram->dragbinel->path.find("cliptemp_") != std::string::npos) {
		//		mainprogram->remove(mainprogram->dragbinel->path);
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
        mainprogram->draglaypos = -1;
        mainprogram->draglaydeck = -1;
		mainprogram->dragclip = nullptr;
		mainprogram->dragpath = "";
		mainmix->moving = false;
		mainprogram->dragout[0] = true;
		mainprogram->dragout[1] = true;
        mainprogram->draggingrec = false;
		//glDeleteTextures(1, mainprogram->dragbinel->tex);  maybe needs implementing in one case, check history
	}
    delete mainprogram->dragbinel;

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
}

void end_input() {
	SDL_StopTextInput();
	mainprogram->cursorpos0 = -1;
	mainprogram->cursorpos1 = -1;
	mainprogram->cursorpos2 = -1;
	mainprogram->cursorpixels = -1;
}


void the_loop() {
    //printf("concatting %d\n", mainprogram->concatting);
    float halfwhite[] = {1.0f, 1.0f, 1.0f, 0.5f};
    float deepred[4] = {1.0, 0.0, 0.0, 1.0};
    float red[] = {1.0f, 0.5f, 0.5f, 1.0f};
    float green[] = {0.0f, 0.75f, 0.0f, 1.0f};
    float orange[] = {1.0f, 0.5f, 0.0f, 1.0f};
    float blue[] = {0.5f, 0.5f, 1.0f, 1.0f};
    float grey[] = {0.5f, 0.5f, 0.5f, 1.0f};
    float darkgrey[] = {0.2f, 0.2f, 0.2f, 1.0f};
    float lightblue[] = {0.5f, 0.5f, 1.0f, 1.0f};

    // prepare gathering of box data
    mainprogram->bdvptr[0] = mainprogram->bdcoords[0];
    mainprogram->bdtcptr[0] = mainprogram->bdtexcoords[0];
    mainprogram->bdcptr[0] = mainprogram->bdcolors[0];
    mainprogram->bdtptr[0] = mainprogram->bdtexes[0];
    mainprogram->bdtnptr[0] = mainprogram->boxtexes[0];
    mainprogram->countingtexes[0] = 0;
    mainprogram->currbatch = 0;
    mainprogram->textbdvptr[0] = mainprogram->textbdcoords[0];
    mainprogram->textbdtcptr[0] = mainprogram->textbdtexcoords[0];
    mainprogram->textbdcptr[0] = mainprogram->textbdcolors[0];
    mainprogram->textbdtptr[0] = mainprogram->textbdtexes[0];
    mainprogram->textbdtnptr[0] = mainprogram->textboxtexes[0];
    mainprogram->textcountingtexes[0] = 0;
    mainprogram->textcurrbatch = 0;
    mainprogram->boxz = 0.0f;
    mainprogram->guielems.clear();

    //SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDrawBuffer(GL_BACK_LEFT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // check if user changed the screen resolution
    SDL_Rect rc;
    SDL_GetDisplayUsableBounds(0, &rc);
    float oldw = glob->w;
    float oldh = glob->h;
    glob->w = (float)rc.w;
    glob->h = (float)rc.h - 1.0f;
    mainprogram->set_ow3oh3();
    if (glob->w != oldw || glob->h != oldh) {
        SDL_SetWindowSize(mainprogram->mainwindow, glob->w, glob->h);
        SDL_DestroyWindow(mainprogram->prefwindow);
        SDL_DestroyWindow(mainprogram->config_midipresetswindow);
        SDL_DestroyWindow(mainprogram->requesterwindow);
        mainprogram->requesterwindow = SDL_CreateWindow("Quit EWOCvj2", glob->w / 4, glob->h / 4, glob->w / 2,
                                                        glob->h / 2, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN |
                                                                     SDL_WINDOW_ALLOW_HIGHDPI);
        mainprogram->config_midipresetswindow = SDL_CreateWindow("Tune MIDI", glob->w / 4, glob->h / 4, glob->w / 2,
                                                                 glob->h / 2, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN |
                                                                              SDL_WINDOW_ALLOW_HIGHDPI);
        mainprogram->prefwindow = SDL_CreateWindow("Preferences", glob->w / 4, glob->h / 4, glob->w / 2, glob->h / 2,
                                                   SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN | SDL_WINDOW_ALLOW_HIGHDPI);

        int wi, he;
        SDL_GL_GetDrawableSize(mainprogram->prefwindow, &wi, &he);
        smw = (float) wi;
        smh = (float) he;

        for (auto gs : mainprogram->guitextmap) {
            delete gs.second;
        }
        mainprogram->guitextmap.clear();
    }

    for (int i = 0; i < mainprogram->menulist.size(); i++) {
        // set menuondisplay: is there a menu active (state > 1)?
        if (mainprogram->menulist[i]->state > 1) {
            mainprogram->leftmousedown = false;
            mainprogram->menuondisplay = true;
            break;
        } else {
            mainprogram->menuondisplay = false;
        }
    }

    mainprogram->prefs->init_midi_devices();

    for (int m = 0; m < 2; m++) {
        for (auto scn : mainmix->scenes[m]) {
            if (scn->pos == mainmix->currscene[m]) continue;
            if (scn->button->toggled()) {
                mainprogram->swappingscene = true;
                scn->switch_to(true);
                mainmix->currscene[m] = scn->pos;
                mainmix->setscene = -1;
                scn->loaded = false;
            }
        }
    }

    if (!mainprogram->binsscreen) {
        // draw background graphic
        draw_direct(nullptr, black, -1.0f, -1.0f, 2.0f, 2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, mainprogram->bgtex, glob->w,
                    glob->h, false, false);
    }

    if (mainprogram->blocking) {
        // when waiting for some activity spread out per loop
        mainprogram->mx = -1;
        mainprogram->my = 100;
        mainprogram->menuactivation = false;
    }
    if (mainprogram->oldmy <= mainprogram->yvtxtoscr(0.075f) &&
        mainprogram->oldmx > glob->w - mainprogram->xvtxtoscr(0.05f)) {
        // for exit while midi learn
        mainprogram->eXit = true;
    } else mainprogram->eXit = false;
    if (mainmix->learn) {
        // when waiting MIDI learn, can't click anywhere else
        mainprogram->mx = -1;
        mainprogram->my = 100;
    }

    if (mainprogram->leftmouse && mainprogram->renaming == EDIT_NONE) {
        mainprogram->recundo = true;
    }

    bool brk = false;
    bool laywiping = false;
    for (auto sm : mainmix->layers) {
        for (auto lay : sm) {
            if (lay->wiping) {
                laywiping = true;
                brk = true;
                break;
            }
        }
        if (brk) break;
    }
    if (mainprogram->leftmouse &&
        (laywiping || mainprogram->cwon || mainprogram->menuondisplay || mainprogram->wiping || mainmix->adaptparam ||
         mainmix->scrollon || binsmain->dragbin || mainmix->moving || mainprogram->dragbinel || mainprogram->drageff ||
         mainprogram->shelfdragelem || mainprogram->wiping || mainprogram->inbetween)) {
        // special cases when mouse can be released over element that should not be triggered
        mainprogram->lmover = true;
        mainprogram->fsmouse = true;
        mainprogram->leftmouse = false;
    } else {
        mainprogram->lmover = false;
    }

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

    // set mouse button values to enable blocking when item ordering overlay is on
    if (mainprogram->orderondisplay || mainmix->retargeting) {
        mainprogram->orderleftmouse = mainprogram->leftmouse;
        mainprogram->orderleftmousedown = mainprogram->leftmousedown;
        mainprogram->orderrightmouse = mainprogram->rightmouse;
        mainprogram->leftmousedown = false;
        mainprogram->leftmouse = false;
        mainprogram->rightmouse = false;
        mainprogram->menuactivation = false;
    }

    // set active loopstation
    if (mainprogram->prevmodus) {
        loopstation = lp;
    } else {
        loopstation = lpc;
    }

    if (!mainprogram->server) {
        if (mainprogram->connected == 0) {
            // Auto-connect to first seat or become server
            std::vector<Program::DiscoveredSeat> seatsCopy;
            while (1) {
                std::unique_lock<std::mutex> lock(mainprogram->discoveryMutex, std::try_to_lock);
                if (lock.owns_lock()) {
                    seatsCopy = mainprogram->discoveredSeats;
                    break;
                }
            }

            // Check if we should auto-connect to the first discovered seat
            if (!seatsCopy.empty() && !mainprogram->autoConnectAttempted) {
                mainprogram->autoConnectAttempted = true;
                mainprogram->serverip = seatsCopy[0].ip;
                if (inet_pton(AF_INET, mainprogram->serverip.c_str(), &mainprogram->serv_addr_client.sin_addr) > 0) {
                    std::cout << "Auto-connecting to first seat: " << seatsCopy[0].name << " (" << seatsCopy[0].ip
                              << ")" << std::endl;
                    int opt = 1;
                    std::thread sockclient(&Program::socket_client, mainprogram, mainprogram->serv_addr_client, opt);
                    sockclient.detach();
                }
            }
                // Check if we should become server (no seats found after timeout)
            else if (seatsCopy.empty() && !mainprogram->autoServerAttempted) {
                auto now = std::chrono::steady_clock::now();
                if (std::chrono::duration_cast<std::chrono::seconds>(now - mainprogram->discoveryStartTime).count() >=
                    5) {
                    mainprogram->autoServerAttempted = true;
                    std::cout << "No seats found after 5 seconds, becoming server" << std::endl;
                    mainprogram->serverip = mainprogram->localip;
                    if (inet_pton(AF_INET, mainprogram->localip.c_str(), &mainprogram->serv_addr_server.sin_addr) > 0) {
                        int opt = 1;
                        mainprogram->server = true;
                        std::thread sockserver(&Program::socket_server, mainprogram, mainprogram->serv_addr_server,
                                               opt);
                        sockserver.detach();
                        std::thread broadcastThread(&Program::discovery_broadcast, mainprogram);
                        broadcastThread.detach();
                    }
                }
            }
        }
    }

    // check if general video resolution changed, and reinitialize all textures concerned
    mainprogram->handle_changed_owoh();

    // if not server then try to connect the client
    if (!mainprogram->server && !mainprogram->connfailed) mainprogram->startclient.notify_all();

    // calculate and visualize fps
    if (!mainmix->retargeting) {
        mainmix->fps[mainmix->fpscount] = (int) (1.0f / (mainmix->time - mainmix->oldtime));
        if (mainmix->fps[24] != 0) {
            int total = 0;
            for (int i = 0; i < 25; i++) total += mainmix->fps[i];
            mainmix->rate = total / 25;
            std::string s = std::to_string(mainmix->rate);
            if (!mainprogram->binsscreen) {
                render_text(s, white, 0.01f, 0.47f, 0.0006f, 0.001f);
            } else {
                render_text(s, white, 0.7f, 0.47f, 0.0006f, 0.001f);
            }
        }
        mainmix->fpscount++;
        if (mainmix->fpscount > 24) mainmix->fpscount = 0;
    }

    // keep advancing non-displayed scenes but dont load frames
    if (1) {
        for (int j = 0; j < 2; j++) {
            for (int i = 0; i < 4; i++) {
                for (int k = 0; k < mainmix->scenes[j][i]->scnblayers.size(); k++) {
                    if (i == mainmix->currscene[j]) break;
                    if (mainmix->scenes[j][i]->scnblayers[k]->filename == "") continue;

                    mainmix->scenes[j][i]->scnblayers[k]->progress(0, 0, true);

                    mainprogram->now = std::chrono::high_resolution_clock::now();
                    LoopStation *l = mainmix->scenes[j][i]->lpst;
                    for (int k = 0; k < l->elements.size(); k++) {
                        if (l->elements[k]->loopbut->value || l->elements[k]->playbut->value) l->elements[k]->set_values();
                    }
                }
            }
        }
    }



    // MIDI stuff
    midi_set();
    mainprogram->shelf_triggering(mainprogram->midishelfelem);
    mainprogram->lpstelem = mainprogram->midishelfelem;


    if (!mainprogram->binsscreen && !mainmix->retargeting) {
        //handle shelves
        mainprogram->inshelf = -1;
        mainprogram->shelves[0]->handle();
        mainprogram->shelves[1]->handle();
    }


    if (!mainprogram->binsscreen && !mainmix->retargeting) {
        mainprogram->preview_modus_buttons();
        make_layboxes();
    }


    // swap layers with newlrs when all newlrs have a decoded frame
    bool done = true;
    std::vector<std::vector<Layer*>> *tempmap;
    for (int i = 0; i < 4; i++) {
        if (i == 0) {
            tempmap = &mainmix->swapmap[0];
        } else if (i == 1) {
            tempmap = &mainmix->swapmap[1];
        } else if (i == 2) {
            tempmap = &mainmix->swapmap[2];
        } else if (i == 3) {
            tempmap = &mainmix->swapmap[3];
        }
        if (!mainmix->retargeting) {
            if (tempmap->size()) {
                bool brk = false;
                for (std::vector<Layer *> lv: *tempmap) {
                    if (lv[1]) {
                        Layer *testlay = lv[1];
                        if (!testlay->liveinput && !testlay->isclone &&
                            (testlay->changeinit < 1 && testlay->filename != "" &&
                             !(testlay->type == ELEM_IMAGE && testlay->numf == 0))) {
                            if (testlay->type != ELEM_IMAGE && testlay->vidformat != 188 && testlay->vidformat != 187) {
                                break;
                            }
                            testlay->load_frame();
                            done = false;
                            brk = true;
                            break;
                        }
                    }
                }
                if (brk) break;
            }
        }
        else {
            done = false;
        }
    }
    if (done || mainprogram->swappingscene) {
        for (int i = 0; i < 4; i++) {
            if (i == 0) {
                tempmap = &mainmix->swapmap[0];
            } else if (i == 1) {
                tempmap = &mainmix->swapmap[1];
            } else if (i == 2) {
                tempmap = &mainmix->swapmap[2];
            } else if (i == 3) {
                tempmap = &mainmix->swapmap[3];
            }
            for (std::vector<Layer *> lv: *tempmap) {
                if (lv[1]) {
                    Layer *testlay = lv[1];
                    testlay->nonewpbos = false;  // get new pbos
                    if (testlay->filename != "") testlay->progress(1, 1);
                    testlay->initdeck = false;
                    if (lv[1]->singleswap) {
                        mainmix->layers[i][lv[1]->pos] = lv[1];
                        tempmap->erase(std::find(tempmap->begin(), tempmap->end(), lv));

                        // transfer current layer settings to new layer
                        mainmix->change_currlay(lv[0], lv[1]);

                        lv[1]->singleswap = false;
                        mainmix->bulayers.push_back(lv[0]);
                        break;
                    }
                }
            }
            std::vector<Layer *> oldlayers;
            if (tempmap->size()) {
                int maxpos = -1;
                for (int j = 0; j < tempmap->size(); j++) {
                    std::vector<Layer *> lv = (*tempmap)[j];
                    if (lv[0] && !lv[1] && maxpos == -1) {
                        maxpos = lv[0]->pos;
                        break;
                    }
                    else {
                        maxpos = -1;
                    }
                }
                for (int j = 0; j < tempmap->size(); j++) {
                    std::vector<Layer *> lv = (*tempmap)[j];
                    bool nothing = false;

                    if (!mainmix->tempmapislayer) {
                        if (!lv[1]) {
                            if (lv[0]->pos >= maxpos) {
                                // don't add anything
                                nothing = true;
                            }
                        }
                    }
                    if (lv[1]) {
                        oldlayers.push_back(lv[1]);
                        lv[1]->pos = j;
                    } else if (lv[0] && (!nothing || lv[0]->keeplay)) {
                        oldlayers.push_back(lv[0]);
                        lv[0]->pos = j;
                        lv[0]->keeplay = false;
                    }
                    if (lv[0] && lv[1]) {
                        if (!mainprogram->swappingscene) {
                            if (!lv[0]->tagged) {
                                mainmix->bulayers.push_back(lv[0]);
                            }
                            if (lv[0]->clonesetnr != -1 && !lv[0]->tagged) {
                                int clnr = lv[0]->clonesetnr;
                                lv[0]->clonesetnr = -1;
                                lv[0]->texture = -1;
                                if (mainmix->clonesets[clnr]->count(lv[0])) {
                                    mainmix->clonesets[clnr]->erase(lv[0]);
                                }
                                if (mainmix->clonesets[clnr]->size() == 1) {
                                    mainmix->cloneset_destroy(clnr);
                                }
                            }
                        }
                        lv[1]->currclipjpegpath = lv[0]->currclipjpegpath;
                        lv[1]->currcliptexpath = lv[0]->currcliptexpath;
                        lv[1]->compswitched = lv[0]->compswitched;
                        // if layer is active webcam connection: look to activate a mimiclayer
                        int pos = std::find(mainprogram->busylayers.begin(), mainprogram->busylayers.end(), lv[1]) -
                                  mainprogram->busylayers.begin();
                        if (pos != mainprogram->busylayers.size()) {
                            bool found = lv[1]->find_new_live_base(pos);
                            if (!found) {
                                mainprogram->busylayers.erase(mainprogram->busylayers.begin() + pos);
                                mainprogram->busylist.erase(
                                        std::find(mainprogram->busylist.begin(), mainprogram->busylist.end(),
                                                  lv[1]->filename));
                            }
                        }
                    }
                }

                mainmix->layers[i] = oldlayers;
                mainmix->reconnect_all(mainmix->layers[i]);
                // transfer current layer settings to new layer
                for (int p = 0; p < mainmix->currlays[!mainprogram->prevmodus].size(); p++) {
                    auto cl = mainmix->currlays[!mainprogram->prevmodus][p];
                    if (!mainprogram->prevmodus == (i / 2) && cl->deck == i % 2) {
                        Layer *newcl = mainmix->layers[i][cl->pos];
                        mainmix->currlays[!mainprogram->prevmodus][p] = newcl;
                        if (newcl == cl) {
                            mainprogram->effcat[newcl->deck]->value = newcl->effcat;
                        }
                    }
                }
                mainmix->currlay[!mainprogram->prevmodus] = mainmix->currlays[!mainprogram->prevmodus][0];
            }
            tempmap->clear();
        }
    }

    if (mainprogram->swappingscene) {
        mainmix->bulayers.clear();
    }
    if (done) {
        mainprogram->swappingscene = false;
    }

    if (mainmix->bulayers.size() && !mainprogram->newproject2 && !mainprogram->swappingscene) { //
        // done switching to new layers -> delete old ones
        std::vector<Layer *> &lvec = mainmix->bulayers;
        for (int i = 0; i < lvec.size(); i++) {
            if (lvec[i]->clonesetnr != -1) {
                int clnr = lvec[i]->clonesetnr;
                lvec[i]->clonesetnr = -1;
                lvec[i]->texture = -1;
                mainmix->clonesets[clnr]->erase(lvec[i]);

                if (mainmix->clonesets[clnr]->size() == 1) {
                    mainmix->cloneset_destroy(clnr);
                }
            }
            lvec[i]->close();
        }
        mainmix->bulayers.clear();
    }

    if (!mainprogram->prevmodus) {
		//when in performance mode: keep advancing frame counters for preview layer stacks (alive = 0)
        mainprogram->bupm = mainprogram->prevmodus;
        mainprogram->prevmodus = true;
        for (int i = 0; i < mainmix->layers[0].size(); i++) {
            Layer *testlay = mainmix->layers[0][i];
            testlay->layers = &mainmix->layers[0];
            testlay->progress(0, 0);
            if (!testlay->initialized) testlay->load_frame();
            //testlay->load_frame();
        }
        for (int i = 0; i < mainmix->layers[1].size(); i++) {
            Layer *testlay = mainmix->layers[1][i];
            testlay->layers = &mainmix->layers[1];
            testlay->progress(0, 0);
            if (!testlay->initialized) testlay->load_frame();
            //testlay->load_frame();
        }
        mainprogram->prevmodus = false;
        // performance mode frame calc and load
        for (int i = 0; i < mainmix->layers[2].size(); i++) {
            Layer *testlay = mainmix->layers[2][i];
            testlay->layers = &mainmix->layers[2];
            testlay->progress(1, 1);
            testlay->load_frame();
        }
        for (int i = 0; i < mainmix->layers[3].size(); i++) {
            Layer *testlay = mainmix->layers[3][i];
            testlay->layers = &mainmix->layers[3];
            testlay->progress(1, 1);
            testlay->load_frame();
        }
	}

    // when in preview modus, do frame texture load for both mixes (preview and output)
	if (mainprogram->prevmodus) {
        for (int i = 0; i < mainmix->layers[0].size(); i++) {
            Layer *testlay = mainmix->layers[0][i];
            testlay->layers = &mainmix->layers[0];
            testlay->progress(0, 1);
            testlay->load_frame();
        }
        for (int i = 0; i < mainmix->layers[1].size(); i++) {
            Layer *testlay = mainmix->layers[1][i];
            testlay->layers = &mainmix->layers[1];
            testlay->progress(0, 1);
            testlay->load_frame();
        }
	}
	if (mainprogram->prevmodus) {
        mainprogram->prevmodus = false;
        for (int i = 0; i < mainmix->layers[2].size(); i++) {
            Layer *testlay = mainmix->layers[2][i];
            testlay->layers = &mainmix->layers[2];
            testlay->progress(1, 1);
            testlay->load_frame();
        }
        for (int i = 0; i < mainmix->layers[3].size(); i++) {
            Layer *testlay = mainmix->layers[3][i];
            testlay->layers = &mainmix->layers[3];
            testlay->progress(1, 1);
            testlay->load_frame();
        }
        mainprogram->prevmodus = true;
	}


    // Crawl web
	if (mainprogram->prevmodus) walk_nodes(0);
	walk_nodes(1);
    make_layboxes();



#ifdef POSIX
    mainprogram->stream_to_v4l2loopbacks();
#endif



    // when encoded, exchange original for hap version everywhere (layerstacks, shelves, bins)
    for (int j = 0; j < 12; j++) {
        for (int i = 0; i < 12; i++) {
            // handle elements, row per row
            Boxx* box = binsmain->elemboxes[i * 12 + j];
            box->upvtxtoscr();
            BinElement* binel = binsmain->currbin->elements[i * 12 + j];
            // show if element encoding/awaiting encoding
            if (!binel->encoding && binel->encodingend != "") {    // exchange original for hap version everywhere (layerstacks, shelves, bins)
                for (int k = 0; k < 4; k++) {
                    for (int m = 0; m < mainmix->layers[k].size(); m++) {
                        Layer *lay = mainmix->layers[k][m];
                        if (lay->isclone) continue;
                        if (lay == binel->otflay) continue;
                        if (lay->filename != binel->encodingend) continue;
                        bool bukeb = lay->keepeffbut->value;
                        lay->keepeffbut->value = 1;
                        lay->dontcloseeffs = 2;
                        lay->tagged = true;
                        std::vector<Layer*> *bulrs = lay->layers;
                        std::vector<Layer*> templrs = {lay};
                        lay->layers = &templrs;
                        lay->transfered = true;
                        Layer *lay2 = lay->open_video(lay->frame, binel->path, false);
                        lay->layers = bulrs;
                        lay2->layers = bulrs;
                        lay2->oldtexture = lay->texture;
                        mainmix->swapmap[k].push_back({lay, lay2});
                        lay2->singleswap = true;
                        lay2->startframe->value = lay->startframe->value;
                        lay2->endframe->value = lay->endframe->value;
                        lay2->numf = lay->numf;
                        lay2->pos = lay->pos;
                        // transfer current layer settings back to old layer
                        if (lay2 == mainmix->currlay[!mainprogram->prevmodus]) mainmix->currlay[!mainprogram->prevmodus] = lay;
                        if (std::find(mainmix->currlays[!mainprogram->prevmodus].begin(), mainmix->currlays[!mainprogram->prevmodus].end(),
                                      lay2) != mainmix->currlays[!mainprogram->prevmodus].end()) {
                            mainmix->currlays[!mainprogram->prevmodus].erase(std::find(mainmix->currlays[!mainprogram->prevmodus].begin(),
                                                                                       mainmix->currlays[!mainprogram->prevmodus].end(),
                                                                                       lay2));
                            mainmix->currlays[!mainprogram->prevmodus].push_back(lay);
                        }
                        lay2->initdeck = true;
                        lay2->keepeffbut->value = bukeb;
                        lay2->set_clones();
                    }
                }

                for (int m = 0; m < 2; m++) {
                    for (auto elem : mainprogram->shelves[m]->elements) {
                        if (elem->path != binel->encodingend) continue;
                        elem->path = binel->path;
                    }
                }

                for (int i = 0; i < binsmain->bins.size(); i++) {
                    for (int j = 0; j < binsmain->bins[i]->elements.size(); j++) {
                        BinElement *elem = binsmain->bins[i]->elements[j];
                        if (elem->path != binel->encodingend) continue;
                        elem->path = binel->path;
                    }
                }

                binel->encodingend = "";
            }
        }
    }



    /////////////// STUFF THAT BELONGS TO EITHER BINS OR MIX OR FULL SCREEN OR RETARGETING

    if (mainmix->retargeting) {
        if (mainmix->retargetenter) {
            retarget->searchdirs = retarget->globalsearchdirs;
        }
        mainmix->retargetenter = false;
        // handle searching for lost media files when loading a file
        if (mainmix->retargetingdone) {
            do_retarget();

            for (int i = 0; i < retarget->searchdirs.size(); i++) {
                if (retarget->searchglobalbuttons[i]->value == 1) {
                    if (std::find(retarget->globalsearchdirs.begin(), retarget->globalsearchdirs.end(), retarget->searchdirs[i]) == retarget->globalsearchdirs.end()) {
                        retarget->globalsearchdirs.push_back(retarget->searchdirs[i]);
                    }
                }
                else {
                    if (std::find(retarget->localsearchdirs.begin(), retarget->localsearchdirs.end(),
                                  retarget->searchdirs[i]) == retarget->localsearchdirs.end()) {
                        retarget->localsearchdirs.push_back(retarget->searchdirs[i]);
                    }
                }
            }

            mainprogram->pathscroll = 0;
            mainprogram->prefs->save();  // save default search dirs

            mainmix->retargetingdone = false;
            mainmix->retargetstage = 0;
            retarget->searchall = false;
            mainmix->newpathlayers.clear();
            mainmix->newpathclips.clear();
            mainmix->newpathcliplays.clear();
            mainmix->newpathshelfelems.clear();
            mainmix->newpathbinels.clear();
            mainmix->newpathpos = 0;
            mainmix->skipall = false;
            mainmix->retargetenter = true;
            mainmix->retargeting = false;
        }
        else {
            // retargeting lost videos/images
            mainprogram->frontbatch = true;

            draw_box(nullptr, deepred, 1.0f - 0.05f, 1.0f - 0.075f, 0.05f, 0.075f, -1);
            render_text("x", white, 0.966f, 1.019f - 0.075f, 0.0012f, 0.002f);
            if (mainprogram->my <= mainprogram->yvtxtoscr(0.075f) &&
                mainprogram->mx > glob->w - mainprogram->xvtxtoscr(0.05f)) {
                if (mainprogram->orderleftmouse) {
                    printf("stopped\n");
                    ;
                    SDL_Quit();
                    exit(0);
                }
            }

            std::function<void()> load_data = [&load_data]() {
                if (mainmix->retargetstage == 0) {
                    mainmix->newpaths = &mainmix->newlaypaths;
                    if (mainmix->newpathpos >= (*(mainmix->newpaths)).size()) {
                        check_stage(1);
                    } else {
                        mainmix->newpaths = &mainmix->newlaypaths;
                        retarget->lay = mainmix->newpathlayers[mainmix->newpathpos];
                        retarget->tex = retarget->lay->jpegtex;
                        retarget->filesize = retarget->lay->filesize;
                    }
                }
                if (mainmix->retargetstage == 1) {
                    mainmix->newpaths = &mainmix->newclippaths;
                    if (mainmix->newpathpos >= (*(mainmix->newpaths)).size()) {
                        check_stage(1);
                    } else {
                        mainmix->newpaths = &mainmix->newclippaths;
                        retarget->clip = mainmix->newpathclips[mainmix->newpathpos];
                        retarget->tex = retarget->clip->tex;
                        retarget->filesize = retarget->clip->filesize;
                    }
                }
                if (mainmix->retargetstage == 2) {
                    mainmix->newpaths = &mainmix->newcliplaypaths;
                    if (mainmix->newpathpos >= (*(mainmix->newpaths)).size()) {
                        check_stage(1);
                    } else {
                        mainmix->newpaths = &mainmix->newcliplaypaths;
                        retarget->clip = mainmix->newpathlayclips[mainmix->newpathpos];
                        retarget->tex = retarget->clip->tex;
                        retarget->filesize = retarget->clip->filesize;
                    }
                }
                if (mainmix->retargetstage == 3) {
                    mainmix->newpaths = &mainmix->newshelfpaths;
                    if (mainmix->newpathpos >= (*(mainmix->newpaths)).size()) {
                        check_stage(1);
                    } else {
                        mainmix->newpaths = &mainmix->newshelfpaths;
                        retarget->shelem = mainmix->newpathshelfelems[mainmix->newpathpos];
                        retarget->tex = retarget->shelem->tex;
                        retarget->filesize = retarget->shelem->filesize;
                    }
                }
                if (mainmix->retargetstage == 4) {
                    mainmix->newpaths = &mainmix->newbinelpaths;
                    if (mainmix->newpathpos >= (*(mainmix->newpaths)).size()) {
                        check_stage(1);
                        if (!mainmix->retargeting) {
                            mainprogram->frontbatch = false;
                            return;
                        }
                    } else {
                        retarget->binel = mainmix->newpathbinels[mainmix->newpathpos];
                        retarget->tex = retarget->binel->tex;
                        retarget->filesize = retarget->binel->filesize;
                    }
                }
                if (mainmix->retargetstage == 5) {
                    mainmix->newpaths = &mainmix->newbineljpegpaths;
                    if (mainmix->newpathpos >= (*(mainmix->newpaths)).size()) {
                        check_stage(1);
                        if (!mainmix->retargeting) {
                            mainprogram->frontbatch = false;
                            return;
                        }
                    } else {
                        retarget->binel = mainmix->newpathbinels[mainmix->newpathpos];
                        retarget->tex = retarget->binel->tex;
                        retarget->filesize = retarget->binel->filesize;
                    }
                }
            };
            load_data();

            if (retarget->searchall && !(*(mainmix->newpaths)).empty()) {
                bool ret = retarget_search();
                if (!ret) {
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
                check_stage(1);
            }
            if ((retarget->skipallbox->in() && mainprogram->orderleftmouse) || mainmix->skipall) {
                mainmix->skipall = true;
                (*(mainmix->newpaths))[mainmix->newpathpos] = "";
                check_stage(1);
            }
            if (retarget->iconbox->in() && mainprogram->orderleftmouse) {
                mainprogram->pathto = "RETARGETFILE";
                mainprogram->get_inname("Find file", "",
                                    std::filesystem::canonical(mainprogram->currfilesdir).generic_string());
                if (mainprogram->path != "") {
                    (*(mainmix->newpaths))[mainmix->newpathpos] = mainprogram->path;
                    check_stage(1);
                    mainprogram->currfilesdir = dirname(mainprogram->path);
                }
            }
            if (!(*(mainmix->newpaths)).empty()) {
                if (mainmix->renaming == false) {
                    render_text((*(mainmix->newpaths))[mainmix->newpathpos], white, -0.4f + 0.015f,
                                retarget->valuebox->vtxcoords->y1 + 0.075f - 0.045f,
                                0.00045f,
                                0.00075f);
                    if (exists((*(mainmix->newpaths))[mainmix->newpathpos])) {
                        mainprogram->currfilesdir = dirname((*(mainmix->newpaths))[mainmix->newpathpos]);
                        check_stage(1);
                    }
                } else {
                    if (mainprogram->renaming == EDIT_NONE) {
                        mainmix->renaming = false;
                        (*(mainmix->newpaths))[mainmix->newpathpos] = mainprogram->inputtext;
                        if (exists((*(mainmix->newpaths))[mainmix->newpathpos])) {
                            mainprogram->currfilesdir = dirname((*(mainmix->newpaths))[mainmix->newpathpos]);
                            check_stage(1);
                        }
                    } else if (mainprogram->renaming == EDIT_CANCEL) {
                        mainmix->renaming = false;
                    } else {
                        do_text_input(-0.4f + 0.015f, retarget->valuebox->vtxcoords->y1 + 0.075f - 0.045f, 0.00045f,
                                      0.00075f, mainprogram->mx, mainprogram->my, mainprogram->xvtxtoscr(0.55f), 0,
                                      nullptr, false);
                    }
                }
                if (retarget->valuebox->in() && mainprogram->orderleftmouse) {
                    mainmix->renaming = true;
                    mainprogram->renaming = EDIT_STRING;
                    mainprogram->inputtext = (*(mainmix->newpaths))[mainmix->newpathpos];
                    mainprogram->cursorpos0 = mainprogram->inputtext.length();
                    SDL_StartTextInput();
                }
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
                mainprogram->get_dir("Add a search location",
                                    std::filesystem::canonical(mainprogram->currfilesdir).generic_string());
                retarget->notfound = false;
                if (mainprogram->path != "") {
                    retarget->searchdirs.push_back(mainprogram->path);
                    make_searchbox(0);
                }
            }
            if (retarget->searchdirs.size()) {
                // mousewheel scroll
                mainprogram->pathscroll -= mainprogram->mousewheel;
                if (mainprogram->pathscroll < 0) mainprogram->pathscroll = 0;
                int totalsize = std::min(mainprogram->pathscroll + 7, (int)retarget->searchdirs.size());
                if (totalsize > 6 && totalsize - mainprogram->pathscroll < 7) mainprogram->pathscroll = totalsize - 6;

                // GUI arrow scroll
                mainprogram->pathscroll = mainprogram->handle_scrollboxes(*mainprogram->searchscrollup, *mainprogram->searchscrolldown, totalsize, mainprogram->pathscroll, 6);

                std::vector<Boxx> boxes;
                short count = 0;
                int size = std::min(6, int(retarget->searchdirs.size()));
                for (int j = 0; j < size - (mainprogram->pathscroll); j++) {
                    draw_box(white, black, retarget->searchboxes[count], -1);
                    render_text(retarget->searchdirs[count + mainprogram->pathscroll], white, -0.4f + 0.015f,
                                retarget->searchboxes[count]->vtxcoords->y1 + 0.075f - 0.045f,
                                0.00045f,
                                0.00075f);
                    mainprogram->handle_button(retarget->searchglobalbuttons[count], false, false, true);
                    render_text("X", white, retarget->searchclearboxes[count]->vtxcoords->x1 + 0.003f,
                                retarget->searchclearboxes[count]->vtxcoords->y1, 0.001125f,
                                0.001875f);
                    if (!retarget->searchglobalbuttons[count]->box->in()) {
                        if (retarget->searchclearboxes[count]->in() && mainprogram->orderleftmouse) {

                            (retarget->searchdirs).erase((retarget->searchdirs).begin() + j - (mainprogram->pathscroll));
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
                            mainprogram->orderleftmouse = false;
                            count--;
                        }
                    }
                    count++;
                    if (count == 6) {
                        break;
                    }
                }

                float y2 = -0.2f - std::min(6, totalsize) * 0.1f;
                std::unique_ptr <Boxx> box = std::make_unique <Boxx> ();;
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
                            check_stage(1);
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

    else if (mainprogram->binsscreen && !binsmain->floating) {
		// big one this: this if decides if code for bins or mix screen is executed
		// the following statement is defined in bins.cpp
		// the 'true' value triggers the full version of this function: it draws the screen also
		// 'false' does a dummy run, used to rightmouse cancel things initiated in code (not the mouse)
		binsmain->handle(true);
	}

    else if (mainprogram->fullscreen > -1) {
        // part of the mix is showed fullscreen, so no bins or mix specifics self-understandingly
       // mainprogram->handle_fullscreen();
        if (mainprogram->menuactivation) {
            mainprogram->fullscreenmenu->state = 2;
            mainprogram->menuondisplay = true;
            mainprogram->menuactivation = false;
        }
    }



        // Handle node view someday     reminder
	//else if (mainmix->mode == 1) {
	//	mainprogram->nodesmain->currpage->handle_nodes();
	//}



	else {  // this is MIX screen specific stuff


		if (mainmix->learn && mainprogram->rightmouse) {
			// MIDI learn cancel
			mainmix->learn = false;
			mainprogram->rightmouse = false;
		}


        // shelf element renaming
        if (mainprogram->renamingshelfelem) {
            if (mainprogram->renaming == EDIT_NONE) {
                mainprogram->renamingshelfelem = nullptr;
            }
            if (mainprogram->leftmousedown && !mainprogram->renamingbox->in()) {
                mainprogram->renaming = EDIT_NONE;
                mainprogram->renamingshelfelem = nullptr;
                SDL_StopTextInput();
                mainprogram->leftmousedown = false;
            }
            if (mainprogram->rightmouse) {
                mainprogram->renaming = EDIT_NONE;
                SDL_StopTextInput();
                mainprogram->renamingshelfelem->name = mainprogram->renamingshelfelem->oldname;
                mainprogram->renamingshelfelem = nullptr;
                mainprogram->rightmouse = false;
                mainprogram->menuactivation = false;
            }
        }
        if (mainprogram->renaming != EDIT_NONE && mainprogram->renamingshelfelem) {
            // bin element renaming with keyboard
            mainprogram->frontbatch = true;
            draw_box(white, black, mainprogram->renamingbox, -1);
            do_text_input(-0.5f + 0.1f, -0.2f + 0.05f, 0.0009f, 0.0015f, mainprogram->mx, mainprogram->my,
                          mainprogram->xvtxtoscr(0.8f), 0, nullptr);
            mainprogram->frontbatch = false;
        }
        
        
        // color wheel stuff
		if (mainprogram->cwon) {
			if (mainprogram->leftmousedown) {
				mainprogram->cwmouse = 1;
				mainprogram->leftmousedown = false;
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
			for (int i = 0; i < mainmix->layers[0].size(); i++) {
				Layer* testlay = mainmix->layers[0][i];
				testlay->display();
			}
			for (int i = 0; i < mainmix->layers[1].size(); i++) {
				Layer* testlay = mainmix->layers[1][i];
				testlay->display();
			}
		}
		else {
			// when performing
			for (int i = 0; i < mainmix->layers[2].size(); i++) {
				Layer* testlay = mainmix->layers[2][i];
				testlay->display();
			}
			for (int i = 0; i < mainmix->layers[3].size(); i++) {
				Layer* testlay = mainmix->layers[3][i];
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
        par->handle();
		par = mainmix->deckspeed[!mainprogram->prevmodus][1];
		draw_box(white, darkgrey, mainmix->deckspeed[!mainprogram->prevmodus][1]->box->vtxcoords->x1, mainmix->deckspeed[!mainprogram->prevmodus][1]->box->vtxcoords->y1, mainmix->deckspeed[!mainprogram->prevmodus][1]->box->vtxcoords->w * 0.30f, 0.1f, -1);
        par->handle();

        // do replace of layer after record
        if (mainmix->recswitch[0]) {
            mainmix->recswitch[0] = false;
            if (mainmix->recrep) {
                // replace recorded layer by recorded video
                mainmix->reclay->speed->value = 1.0f;
                int bukebv = mainmix->reclay->keepeffbut->value;
                mainmix->reclay->keepeffbut->value = 0;
                mainmix->reclay = mainmix->reclay->open_video(0.0f, mainmix->reclay->filename, true, false);
                mainmix->reclay->keepeffbut->value = bukebv;
                mainmix->reclay->type = ELEM_FILE;
                mainmix->reclay->shiftx->value = 0.0f;
                mainmix->reclay->shifty->value = 0.0f;
                mainmix->reclay->scale->value = 1.0f;
                mainmix->reclay->opacity->value = 1.0f;
                if (mainmix->reclay->revbut->value) {
                    mainmix->reclay->playbut->value = true;
                    mainmix->reclay->revbut->value = false;
                }
                mainmix->reclay->speed->value /= mainmix->deckspeed[!mainprogram->prevmodus][mainmix->reclay->deck]->value;
                mainmix->recrep = false;
            }
        }
        
		//draw and handle recbuts
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
            mainprogram->recundo = false;
        }
        // draw and handle lastly recorded video snippets
        if (mainmix->recswitch[1]) {
            mainmix->recswitch[1] = false;
            mainmix->recQthumbshow = copy_tex(mainmix->recQthumb);
        }

        if (mainmix->recQthumbshow != -1) {
            int sw, sh;
            glBindTexture(GL_TEXTURE_2D, mainmix->recQthumbshow);
            glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &sw);
            glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &sh);
            Boxx box;
            box.vtxcoords->x1 = 0.15f + 0.0465f;
            box.vtxcoords->y1 = -1.0f + mainprogram->monh * 2.0f;
            box.vtxcoords->w = 0.040f * ((float)sw / (float)sh);
            box.vtxcoords->h = 0.075f;
            box.upvtxtoscr();
            draw_box(&box, mainmix->recQthumbshow);
            if (box.in() && mainprogram->leftmousedown) {
                mainprogram->leftmousedown = false;
                mainprogram->draggingrec = true;
                mainprogram->dragbinel = new BinElement;
                mainprogram->dragbinel->tex = mainmix->recQthumbshow;
                mainprogram->dragbinel->path = mainmix->recpath[1];
                mainprogram->dragbinel->name = remove_extension(basename(mainmix->recpath[1]));
                mainprogram->dragbinel->relpath = std::filesystem::relative(mainmix->recpath[1], mainprogram->project->binsdir).generic_string();
                mainprogram->dragbinel->type = ELEM_FILE;
                mainprogram->shelves[0]->prevnum = -1;
                mainprogram->shelves[1]->prevnum = -1;
            }
        }



		mainprogram->layerstack_scrollbar_handle();



        //handle dragging into layerstack
		if (mainprogram->dragbinel && !mainmix->moving) {
			if (mainprogram->dragbinel->type != ELEM_DECK && mainprogram->dragbinel->type != ELEM_MIX) {
				if (mainprogram->prevmodus) {
					drag_into_layerstack(mainmix->layers[0], 0);
					drag_into_layerstack(mainmix->layers[1], 1);
				}
				else {
					drag_into_layerstack(mainmix->layers[2], 0);
					drag_into_layerstack(mainmix->layers[3], 1);
				}
			}
		}


        // handle the layer clips queue
		mainmix->handle_clips();


		// draw "layer insert into stack" blue boxes
		if (!mainprogram->menuondisplay && mainprogram->dragbinel) {
		    mainprogram->frontbatch = true;
            mainprogram->dragpos = -1;
			for (int j = 0; j < 2; j++) {
				std::vector<Layer*> lvec = choose_layers(j);
				for (int i = 0; i < lvec.size(); i++) {
					Layer* lay = lvec[i];
					bool comp = !mainprogram->prevmodus;
					if (lay->pos < mainmix->scenes[j][mainmix->currscene[j]]->scrollpos || lay->pos > mainmix->scenes[j][mainmix->currscene[j]]->scrollpos + 2) continue;
					Boxx* box = lay->node->vidbox;
                    box->upvtxtoscr();
					float thick = mainprogram->xvtxtoscr(0.075f);
					if (box->scrcoords->y1 - box->scrcoords->h < mainprogram->my && mainprogram->my < box->scrcoords->y1) {
						if (box->scrcoords->x1 - thick + (i - mainmix->scenes[j][mainmix->currscene[j]]->scrollpos == 0) * thick < mainprogram->mx && mainprogram->mx < box->scrcoords->x1 + thick) {
							// this block handles the first boxes, just not the last
							mainprogram->leftmousedown = false;
							float blue[] = { 0.5, 0.5, 1.0, 1.0 };
                            mainprogram->dragpos = lay->pos;
							// draw broad blue boxes when inserting layers
							draw_box(blue, blue, box->vtxcoords->x1 - mainprogram->xscrtovtx(thick) + (i - mainmix->scenes[j][mainmix->currscene[j]]->scrollpos == 0) * mainprogram->xscrtovtx(thick), box->vtxcoords->y1, mainprogram->xscrtovtx(thick * (2.0f - (i - mainmix->scenes[j][mainmix->currscene[j]]->scrollpos == 0))), mainprogram->layh, -1);
						}
						else if (box->scrcoords->x1 + box->scrcoords->w - thick < mainprogram->mx && mainprogram->mx < box->scrcoords->x1 + box->scrcoords->w) {
							mainprogram->leftmousedown = false;
							if (lay->pos == lvec.size() - 1 || lay->pos == mainmix->scenes[j][mainmix->currscene[j]]->scrollpos + 2) {
								// this block handles the last box
								float blue[] = { 0.5, 0.5 , 1.0, 1.0 };
								// draw broad blue boxes when inserting layers
                                mainprogram->dragpos = lvec.size();
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

        // Handle parameter adaptation
        if (mainmix->adaptparam) {
            mainmix->handle_adaptparam();
        }

    }



    // draw and handle loopstation
    if (!mainmix->retargeting) {
        mainprogram->now = std::chrono::high_resolution_clock::now();
        if (mainprogram->prevmodus) {
            loopstation = lp;
            lp->handle();
            for (int i = 0; i < lpc->elements.size(); i++) {
                if (lpc->elements[i]->loopbut->value || lpc->elements[i]->playbut->value)
                    lpc->elements[i]->set_values();
            }
        } else {
            loopstation = lpc;
            lpc->handle();
            for (int i = 0; i < lp->elements.size(); i++) {
                if (lp->elements[i]->loopbut->value || lp->elements[i]->playbut->value) lp->elements[i]->set_values();
            }
            for (int i = 0; i < 2; i++) {
                for (int j = 0; j < 4; j++) {
                    if (j == mainmix->currscene[i]) continue;
                    for (auto *elem: mainmix->scenes[i][j]->lpst->elements) {
                        if ((elem->loopbut->value || elem->playbut->value) && !elem->eventlist.empty())
                            elem->set_values();
                        std::vector<Layer *> &lvec = mainmix->scenes[i][j]->scnblayers;
                        for (int m = 0; m < lvec.size(); m++) {
                            std::vector<float> colvec;
                            colvec.push_back(elem->colbox->acolor[0]);
                            colvec.push_back(elem->colbox->acolor[1]);
                            colvec.push_back(elem->colbox->acolor[2]);
                            colvec.push_back(elem->colbox->acolor[3]);
                            if (std::find(elem->layers.begin(), elem->layers.end(), lvec[m]) != elem->layers.end()) {
                                lvec[m]->lpstcolors.emplace(colvec);
                            } else {
                                lvec[m]->lpstcolors.erase(colvec);
                            }
                        }
                    }
                }
            }
        }
        if (!mainprogram->binsscreen) {
            mainprogram->beatthres->handle();
        }
    }

    if (mainprogram->rightmouse) {
		if (mainprogram->dragclip) {
			// cancel clipdragging
            mainprogram->dragclip->insert(mainprogram->draglay, mainprogram->draglay->clips->begin() + mainprogram->dragpos);
			mainprogram->dragclip = nullptr;
			mainprogram->draglay = nullptr;
			mainprogram->dragpos = -1;
			enddrag();
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

    if (mainprogram->openjpegpathsshelf) {
        // UNDO: load one texture into shelf, one each loop not to slowdown output stream
        mainprogram->shelves[0]->open_jpegpaths_shelf();
        mainprogram->shelves[1]->open_jpegpaths_shelf();
    }

    if (binsmain->importbins) {
		// load one item from mainprogram->paths into binslist, one each loop not to slowdown output stream
		binsmain->import_bins();
	}

	// implementation of a basic top menu when the mouse is at the top of the screen
    // exitedtop and intoparea cater for linux graphical environments with a top bar
    if (mainprogram->my <= glob->h / 2.0f)  {
        mainprogram->intoparea = true;
    }
    else {
        mainprogram->intoparea = false;
    }
    if ((mainprogram->my <= 0 || mainprogram->exitedtop) && !mainprogram->transforming) {
	    if (!mainprogram->prefon && !mainprogram->midipresets && mainprogram->quitting == "") {
            mainprogram->intopmenu = true;
        }
	}
	if (mainprogram->intopmenu) {
        // display and set drop down visualisation values of main topmenu bar
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
            if (!mainprogram->exitedtop) {
                mainprogram->intopmenu = false;
            }
        }
		else if (mainprogram->mx < mainprogram->xvtxtoscr(0.156f)) {
			if (mainprogram->leftmouse) {
				mainprogram->filemenu->menux = 0;
				mainprogram->filemenu->menuy = mainprogram->yvtxtoscr(0.075f);
                mainprogram->filenewmenu->menux = 0;
                mainprogram->filenewmenu->menuy = mainprogram->yvtxtoscr(0.075f);
                mainprogram->fileopenmenu->menux = 0;
                mainprogram->fileopenmenu->menuy = mainprogram->yvtxtoscr(0.075f);
                mainprogram->filesavemenu->menux = 0;
                mainprogram->filesavemenu->menuy = mainprogram->yvtxtoscr(0.075f);
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
        //displaying "MIDI learn active" message box
        mainprogram->frontbatch = true;
		draw_box(red, blue, -0.3f, -0.0f, 0.6f, 0.3f, -1);
        render_text("Awaiting MIDI input.", white, -0.1f, 0.2f, 0.001f, 0.0016f);
        render_text("Rightclick cancels.", white, -0.1f, 0.06f, 0.001f, 0.0016f);
        mainprogram->frontbatch = false;
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
		enddrag();
	}

	if (!mainprogram->prefon && !mainprogram->midipresets && mainprogram->tooltipbox && mainprogram->quitting == "") {
		if (mainprogram->tooltipbox->tooltip != "") mainprogram->tooltips_handle(0);
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

    mainprogram->handle_beatmenu();

    if (mainprogram->optionmenu) {
        mainprogram->handle_optionmenu();
    }

    mainprogram->frontbatch = false;

    if (mainprogram->menuactivation == true) {
        // main menu triggered
        mainprogram->mainmenu->state = 2;
        mainprogram->menuactivation = false;
    }


    /*for (Bin *bin : binsmain->bins) {
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
    }*/


    // saving bin jpegs in background
    binsmain->save_binjpegs();


    //autosave
    if (mainmix->retargeting) mainprogram->astimestamp = (int) mainmix->time;  // postpone autosave
    if (mainprogram->autosave && (mainmix->time > mainprogram->astimestamp + mainprogram->asminutes * 60)) {
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
    if (binsmain->floatingsync) {
        {
            std::unique_lock<std::mutex> lock2(binsmain->syncmutex);
            binsmain->syncnow = true;
        }
        binsmain->sync.notify_all();
        std::unique_lock<std::mutex> lock(binsmain->syncendmutex);
        binsmain->syncend.wait(lock, [&]{return binsmain->syncendnow;});
        binsmain->syncendnow = false;
        lock.unlock();

        SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
    }


    if ((mainprogram->lmover || mainprogram->binlmover) && mainprogram->fullscreenmenu->state < 2) {
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
        binsmain->menuactbinel = nullptr;
        //mainprogram->recundo = false;
    }

    if (mainprogram->infostr != "") {
        // showing an info dialog
        mainprogram->directmode = true;
        SDL_ShowWindow(mainprogram->requesterwindow);
        SDL_RaiseWindow(mainprogram->requesterwindow);
        SDL_GL_MakeCurrent(mainprogram->requesterwindow, glc);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDrawBuffer(GL_BACK_LEFT);
        glViewport(0, 0, glob->w / 2.0f, glob->h / 2.0f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);

        mainprogram->show_info();

        SDL_GL_SwapWindow(mainprogram->requesterwindow);
        SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDrawBuffer(GL_BACK_LEFT);
        glViewport(0, 0, glob->w, glob->h);
        mainprogram->directmode = false;
    }

    bool prret = false;
	GLuint tex, fbo;
	if (mainprogram->quitting != "") {
        // exiting the program
		mainprogram->directmode = true;
        SDL_ShowWindow(mainprogram->requesterwindow);
        SDL_RaiseWindow(mainprogram->requesterwindow);
		SDL_GL_MakeCurrent(mainprogram->requesterwindow, glc);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDrawBuffer(GL_BACK_LEFT);
		glViewport(0, 0, glob->w / 2.0f, glob->h / 2.0f);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
		int ret = mainprogram->quit_requester();
		if (ret == 1 || ret == 2) {
            mainprogram->shutdown = true;

            mainprogram->project->wait_for_copyover(false);
            while (mainprogram->concatting) {
                Sleep (5);
            }

            // remove unsaved bins
            std::vector<Bin *> bins = binsmain->bins;
            int correct = 0;
            for (int i = 0; i < bins.size(); i++) {
                if (!bins[i]->saved) {
                    binsmain->bins.erase(binsmain->bins.begin() + i - correct);
                    mainprogram->remove(bins[i]->path);
                    std::filesystem::remove_all(mainprogram->project->bubd + bins[i]->name);
                    correct++;
                }
            }
            binsmain->save_binslist();
            if (binsmain->bins.size() == 0) {
                mainprogram->remove(mainprogram->project->bubd + "bins.list");
            }

            //remove redundant bin files
            for (std::filesystem::recursive_directory_iterator end_dir_it, it(mainprogram->project->binsdir); it != end_dir_it; ++it) {
                std::string p = it->path().string();
                if (p.rfind(".") != std::string::npos) {
                    if (p.substr(p.rfind(".")) == ".list") continue;
                    if (p.substr(p.rfind(".")) == ".bin") continue;
                }
                if (std::filesystem::is_directory(it->path())) continue;
                if (!mainprogram->project->pathsinbins.contains(pathtoplatform(p))) {
                    mainprogram->remove(p);
                }
            }

            // empty autosave temp dir
            std::filesystem::path path_to_rem(mainprogram->project->autosavedir + "temp");
            for (std::filesystem::directory_iterator end_dir_it, it(path_to_rem); it != end_dir_it; ++it) {
                std::filesystem::remove_all(it->path());
            }

            //save midi map
			//save_genmidis(mainprogram->docpath + "midiset.gm");
			//empty temp dir
			for (int j = 0; j < 12; j++) {
				for (int i = 0; i < 12; i++) {
					// cancel hap encoding for all elements
					Boxx* box = binsmain->elemboxes[i * 12 + j];
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

            // clean up alla layers
            // pause first to allow all layers to be added to dellays
#ifdef POSIX
            sleep(0.1f);
#endif
#ifdef WINDOWS
            Sleep(100);
#endif
            dellayslock.lock();
            for (Layer* lay : mainprogram->dellays) {
                delete lay;
            }
            mainprogram->dellays.clear();
            dellayslock.unlock();

			// close socket communication
			// if server ask other socket to become server else signal all other sockets that we're quitting
            if (mainprogram->connsockets.size()) {
                if (mainprogram->server) {
                    for (auto sock : mainprogram->connsockets) {
                        send(sock, "SERVER_QUITS", 12, 0);
                    }
                }
            }
#ifdef WINDOWS
            closesocket(mainprogram->sock);
#endif
#ifdef POSIX
            close(mainprogram->sock);
#endif

			mainprogram->prefs->save();

			std::filesystem::path path_to_remove(mainprogram->temppath);
			for (std::filesystem::directory_iterator end_dir_it, it(path_to_remove); it != end_dir_it; ++it) {
				mainprogram->remove(it->path().string());
			}

			printf("stopped\n");
            fflush(stdout);

            SDL_Quit();
			exit(0);
		}
		if (ret == 3) {
			mainprogram->quitting = "";
			SDL_HideWindow(mainprogram->requesterwindow);
			SDL_RaiseWindow(mainprogram->mainwindow);
		}
		if (ret == 0) {
			SDL_GL_SwapWindow(mainprogram->requesterwindow);
		}
		SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDrawBuffer(GL_BACK_LEFT);
        glViewport(0, 0, glob->w, glob->h);
        mainprogram->directmode = false;
	}



    mainprogram->preferences();

	mainprogram->config_midipresets_init();


    if (!mainprogram->binsscreen) {
        // leftmouse click outside clip queue cancels clip queue visualisation and deselects multiple selected layers
        bool found = false;
        bool found2 = false;
        for (int i = 0; i < 2; i++) {
            std::vector<Layer*> &lvec = choose_layers(i);
            for (int j = 0; j < lvec.size(); j++) {
                if (lvec[j]->clipscritching == 4) {
                    lvec[j]->clipscritching = 0;
                    found2 = true;
                    break;
                }
                if (lvec[j]->queueing) {
                    found = true;
                    break;
                }
            }
        }
        std::vector<Layer*>& lvec1 = choose_layers(0);
        if (!found2 && (!found || (mainprogram->leftmouse && !mainprogram->menuondisplay && !mainprogram->modusbut->box->in()))) {
            set_queueing(false);
        }
    }



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

    glEnable(GL_DEPTH_TEST);
    glClearDepth(1.0f);
    glClear(GL_DEPTH_BUFFER_BIT);
    glDepthFunc(GL_LESS);

    static int bs[2048];
    static bool bs_initialized = false;
    if (!bs_initialized) {
        std::iota(bs, bs + mainprogram->maxtexes - 2, 0);
        bs_initialized = true;
    }
    mainprogram->uniformCache->setSamplerArray("boxSampler", bs, mainprogram->maxtexes - 2);
    glActiveTexture(GL_TEXTURE0 + mainprogram->maxtexes - 2);
    glBindTexture(GL_TEXTURE_BUFFER, mainprogram->bdcoltex);
    glActiveTexture(GL_TEXTURE0 + mainprogram->maxtexes - 1);
    glBindTexture(GL_TEXTURE_BUFFER, mainprogram->bdtextex);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mainprogram->bdibo);
    glBindVertexArray(mainprogram->bdvao);
    glDisable(GL_BLEND);

    mainprogram->uniformCache->setBool("glbox", true);
    mainprogram->renderer->render();
    glClear(GL_DEPTH_BUFFER_BIT);
    mainprogram->renderer->text_render();
    mainprogram->uniformCache->setBool("glbox", false);
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
        if (elem == nullptr) continue;
        if (elem->type == GUI_LINE) {
            draw_line(elem->line);
            delete elem->line;
        }
        else if (elem->type == GUI_TRIANGLE) {
            draw_triangle(elem->triangle);
            delete elem->triangle;
        } else {
            if (!elem->box->circle && elem->box->text) {
                mainprogram->uniformCache->setBool("textmode", true);
            }
            draw_box(elem->box->linec, elem->box->areac, elem->box->x, elem->box->y, elem->box->wi,
                     elem->box->he, 0.0f, 0.0f, 1.0f, 1.0f, elem->box->circle, elem->box->tex, glob->w, glob->h,
                     elem->box->text, elem->box->vertical, elem->box->inverted);
            if (!elem->box->circle && elem->box->text) mainprogram->uniformCache->setBool("textmode", false);
            delete elem->box;
        }
        delete elem;
    }
    mainprogram->directmode = false;

    Layer *lay = mainmix->currlay[!mainprogram->prevmodus];

    if (lay) {
        // Handle colorbox
        mainprogram->pick_color(lay, lay->colorbox);
        if (lay->cwon) {
            lay->blendnode->chred = lay->rgb[0];
            lay->blendnode->chgreen = lay->rgb[1];
            lay->blendnode->chblue = lay->rgb[2];
        }
    }

    if (!mainprogram->binsscreen && mainprogram->fullscreen == -1) {
        // background texture overlay fakes transparency
        draw_direct(nullptr, black, -1.0f, -1.0f, 2.0f, 2.0f, 0.0f, 0.0f, 1.0f, 0.2f, 0, mainprogram->bgtex,
                    glob->w, glob->h, false, false);
    	// draw blue drop rectangles over monitors
    	if (mainmix->dropdeckblue == 1) {
    		mainmix->dropdeckblue = 0;
    		draw_direct(nullptr, lightblue, -1.0f + mainprogram->numw, 1.0f - mainprogram->layh,
					 mainprogram->layw * 3, mainprogram->layh, 0.0f, 0.0f, 1.0f, 0.2f, 0, -1, glob->w, glob->h, false, false);
    	}
    	if (mainmix->dropdeckblue == 2) {
    		mainmix->dropdeckblue = 0;
    		draw_direct(nullptr, lightblue, mainprogram->numw, 1.0f - mainprogram->layh, mainprogram->layw * 3,
					 mainprogram->layh, 0.0f, 0.0f, 1.0f, 0.2f, 0, -1, glob->w, glob->h, false, false);
    	}
    	if (mainmix->dropmixblue == 1) {
    		mainmix->dropmixblue = 0;
    		draw_direct(nullptr, lightblue, -1.0f + mainprogram->numw, 1.0f - mainprogram->layh,
					 mainprogram->layw * 6 + mainprogram->numw, mainprogram->layh, 0.0f, 0.0f, 1.0f, 0.2f, 0, -1, glob->w, glob->h, false, false);
    	}
    }

    if (mainprogram->fullscreen > -1) {
        // part of the mix is showed fullscreen, so no bins or mix specifics self-understandingly
        mainprogram->handle_fullscreen();
    }


    //glFinish();
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
    mainprogram->menuactivation = false;

    for (int i = 0; i < 4; i++) {
        for (Layer *lay : mainmix->layers[i]) {
            if (lay->blendnode) {
                lay->blendnode->wipex->layer = lay;
                lay->blendnode->wipey->layer = lay;
            }
            if (lay->opened) {
                lay->endopenvar.notify_all();
            }
        }
    }

    std::unordered_map<int, std::unordered_set<Layer*>*> css = mainmix->clonesets;
    for (auto &it : css) {
        if (it.second->size() == 1) {
            mainmix->cloneset_destroy(it.first);
        }
    }
    mainmix->csnrmap.clear();

    bool found = false;
    for (Layer* lay : mainmix->loadinglays) {
        if (lay->vidopen) {
            found = true;
            break;
        }
    }
    if (!found) {
        mainmix->loadinglays.clear();
    }

    mainmix->reconnect_all(mainmix->layers[0]);
    mainmix->reconnect_all(mainmix->layers[1]);
    mainmix->reconnect_all(mainmix->layers[2]);
    mainmix->reconnect_all(mainmix->layers[3]);
    make_layboxes();

    dellayslock.lock();
    for (Layer* lay : mainprogram->dellays) {
        delete lay;
    }
    mainprogram->dellays.clear();
    dellayslock.unlock();

    for (Effect* eff : mainprogram->deleffects) {
        mainprogram->nodesmain->currpage->delete_node(eff->node);
        delete eff;
    }
    mainprogram->deleffects.clear();

    if (mainprogram->dropfiles.size()) {
        for (char *df : mainprogram->dropfiles) {
            SDL_free(df);
        }
        mainprogram->dropfiles.clear();
    }

    if (!mainprogram->inbox && !mainprogram->undowaiting && mainprogram->recundo) {
        mainprogram->recundo = false;
    }
    mainprogram->inbox = false;

    //UNDO
    if ((mainprogram->recundo || mainprogram->undowaiting) && !mainmix->retargeting) {
        mainprogram->undo_redo_save();
    }
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
    glDeleteFramebuffers(1, &fbo);
}


void write_genmidi(ostream& wfile, LayMidi *lm) {
    wfile << "CROSSFADE\n";
    wfile << std::to_string(lm->crossfade->midi0);
    wfile << "\n";
    wfile << std::to_string(lm->crossfade->midi1);
    wfile << "\n";
    wfile << lm->crossfade->midiport;
    wfile << "\n";

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

    wfile << "SCRINVERT\n";
    wfile << std::to_string(lm->scrinvert);
    wfile << "\n";

    wfile << "SCRATCH1\n";
    wfile << std::to_string(lm->scratch1->midi0);
    wfile << "\n";
    wfile << std::to_string(lm->scratch1->midi1);
    wfile << "\n";
    wfile << lm->scratch1->midiport;
    wfile << "\n";

    wfile << "SCRATCH2\n";
    wfile << std::to_string(lm->scratch2->midi0);
    wfile << "\n";
    wfile << std::to_string(lm->scratch2->midi1);
    wfile << "\n";
    wfile << lm->scratch2->midiport;
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
            wfile << std::to_string(ls->elements[j]->recbut->midi[0]) + "\n";
            wfile << std::to_string(ls->elements[j]->loopbut->midi[0]) + "\n";
            wfile << std::to_string(ls->elements[j]->playbut->midi[0]) + "\n";
            wfile << std::to_string(ls->elements[j]->speed->midi[0]) + "\n";
            wfile << "LOOPSTATIONMIDI1:" + std::to_string(i) + ":" + std::to_string(j) + "\n";
            wfile << std::to_string(ls->elements[j]->recbut->midi[1]) + "\n";
            wfile << std::to_string(ls->elements[j]->loopbut->midi[1]) + "\n";
            wfile << std::to_string(ls->elements[j]->playbut->midi[1]) + "\n";
            wfile << std::to_string(ls->elements[j]->speed->midi[1]) + "\n";
            wfile << "LOOPSTATIONMIDIPORT:" + std::to_string(i) + ":" + std::to_string(j) + "\n";
            wfile << ls->elements[j]->recbut->midiport + "\n";
            wfile << ls->elements[j]->loopbut->midiport + "\n";
            wfile << ls->elements[j]->playbut->midiport + "\n";
            wfile << ls->elements[j]->speed->midiport + "\n";
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
	std::fstream rfile;
	rfile.open(path);
	std::string istring;

	LayMidi *lm = nullptr;
	while (safegetline(rfile, istring)) {
		if (istring == "ENDOFFILE") break;
	
		if (istring == "LAYMIDIA") lm = laymidiA;
		if (istring == "LAYMIDIB") lm = laymidiB;
		if (istring == "LAYMIDIC") lm = laymidiC;
		if (istring == "LAYMIDID") lm = laymidiD;

        if (istring == "CROSSFADE") {
            safegetline(rfile, istring);
            lm->crossfade->midi0 = std::stoi(istring);
            safegetline(rfile, istring);
            lm->crossfade->midi1 = std::stoi(istring);
            safegetline(rfile, istring);
            lm->crossfade->midiport= istring;
            lm->crossfade->register_midi();
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
        if (istring == "SCRINVERT") {
            safegetline(rfile, istring);
            lm->scrinvert = std::stoi(istring);
        }
        if (istring == "SCRATCH1") {
            safegetline(rfile, istring);
            lm->scratch1->midi0 = std::stoi(istring);
            safegetline(rfile, istring);
            lm->scratch1->midi1 = std::stoi(istring);
            safegetline(rfile, istring);
            lm->scratch1->midiport = istring;
            lm->scratch1->register_midi();
        }
        if (istring == "SCRATCH2") {
            safegetline(rfile, istring);
            lm->scratch2->midi0 = std::stoi(istring);
            safegetline(rfile, istring);
            lm->scratch2->midi1 = std::stoi(istring);
            safegetline(rfile, istring);
            lm->scratch2->midiport = istring;
            lm->scratch2->register_midi();
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
						ls->elements[j]->recbut->midi[0] = std::stoi(istring);
						safegetline(rfile, istring);
						ls->elements[j]->loopbut->midi[0] = std::stoi(istring);
						safegetline(rfile, istring);
						ls->elements[j]->playbut->midi[0] = std::stoi(istring);
						safegetline(rfile, istring);
						ls->elements[j]->speed->midi[0] = std::stoi(istring);
					}
					if (istring == "LOOPSTATIONMIDI1:" + std::to_string(i) + ":" + std::to_string(j)) {
						safegetline(rfile, istring);
						ls->elements[j]->recbut->midi[1] = std::stoi(istring);
						safegetline(rfile, istring);
						ls->elements[j]->loopbut->midi[1] = std::stoi(istring);
						safegetline(rfile, istring);
						ls->elements[j]->playbut->midi[1] = std::stoi(istring);
						safegetline(rfile, istring);
						ls->elements[j]->speed->midi[1] = std::stoi(istring);
					}
					if (istring == "LOOPSTATIONMIDIPORT:" + std::to_string(i) + ":" + std::to_string(j)) {
						safegetline(rfile, istring);
						ls->elements[j]->recbut->midiport = istring;
                        ls->elements[j]->recbut->register_midi();
						safegetline(rfile, istring);
						ls->elements[j]->loopbut->midiport = istring;
                        ls->elements[j]->loopbut->register_midi();
						safegetline(rfile, istring);
						ls->elements[j]->playbut->midiport = istring;
                        ls->elements[j]->playbut->register_midi();
						safegetline(rfile, istring);
						ls->elements[j]->speed->midiport = istring;
                        ls->elements[j]->speed->register_midi();
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
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow) {
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string direc = dirname(exePath);  // Removes the filename from the path
    std::filesystem::current_path(direc);
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
    const char *defaultDeviceName = alcGetString(nullptr, ALC_DEFAULT_DEVICE_SPECIFIER);
    ALCdevice *device = alcOpenDevice(defaultDeviceName);
    ALCcontext *context = alcCreateContext(device, nullptr);
    bool succes = alcMakeContextCurrent(context);

    // ALSA
    //snd_seq_t *seq_handle;
    //if (snd_seq_open(&seq_handle, "hw", SND_SEQ_OPEN_INPUT, 0) < 0) {
    //    fprintf(stderr, "Error opening ALSA sequencer.\n");
    //   exit(0);
    //}


    // initializing devIL
    ilInit();

    if (SDL_Init(SDL_INIT_VIDEO) < 0) /* Initialize SDL's Video subsystem */
        mainprogram->quitting = "Unable to initialize SDL"; /* Or die on error */
    //atexit(SDL_Quit);
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
    }
    /* Request opengl 4.6 context.
     * SDL doesn't have the ability to choose which profile at this time of writing,
     * but it should default to the core profile */
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    //SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);  // KEY CHANGE!

    /* Turn on double buffering */
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetSwapInterval(0); //vsync off

    SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");

    SDL_Rect rc;
    SDL_GetDisplayUsableBounds(0, &rc);
    auto sw = rc.w;
    auto sh = rc.h - 1;

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
    mainprogram->mainwindow = win;
    lp = new LoopStation;
    lpc = new LoopStation;
    loopstation = lp;
    mainmix = new Mixer;
    binsmain = new BinsMain;
    retarget = new Retarget;


#ifdef WINDOWS
    std::filesystem::path p5{mainprogram->docpath + "projects"};
    mainprogram->currprojdir = p5.generic_string();
    if (!exists(mainprogram->docpath + "projects")) std::filesystem::create_directory(p5);
#else
    #ifdef POSIX
    std::string homedir(getenv("HOME"));
    mainprogram->homedir = homedir;
    std::string xdg_docs = getdocumentspath();
    std::filesystem::path p1{xdg_docs + "/EWOCvj2"};
    std::string docdir = p1.generic_string();
    if (!exists(xdg_docs + "/EWOCvj2")) std::filesystem::create_directory(p1);
    std::filesystem::path e{homedir + "/.ewocvj2"};
    if (!exists(homedir + "/.ewocvj2")) std::filesystem::create_directory(e);
    std::filesystem::path p4{homedir + "/.ewocvj2/temp"};
    if (!exists(homedir + "/.ewocvj2/temp")) std::filesystem::create_directory(p4);
    std::filesystem::path p6{docdir + "/projects"};
    mainprogram->currprojdir = p6.generic_string();
    if (!exists(docdir + "/projects")) std::filesystem::create_directory(p6);
#endif
#endif

    std::filesystem::path p7{mainprogram->docpath + "bins"};
    mainprogram->currbinsdir = p7.generic_string();
    if (!exists(mainprogram->docpath + "bins")) std::filesystem::create_directory(p7);

    //empty temp dir if program crashed last time
    std::filesystem::path path_to_remove(mainprogram->temppath);
    for (std::filesystem::directory_iterator end_dir_it, it(path_to_remove); it != end_dir_it; ++it) {
        std::filesystem::remove_all(it->path());
    }

    glc = SDL_GL_CreateContext(mainprogram->mainwindow);
    SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
    mainprogram->dummywindow = SDL_CreateWindow("Dummy", glob->w / 4, glob->h / 4, glob->w / 2,
                                               glob->h / 2, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN |
                                                       SDL_WINDOW_ALLOW_HIGHDPI);
    orderglc = SDL_GL_CreateContext(mainprogram->dummywindow);
    SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 0);


    //glewExperimental = GL_TRUE;
    glewInit();

    mainprogram->splashwindow = SDL_CreateWindow("", (glob->w - (glob->h / 2)) / 2, glob->h / 4, glob->h / 2, glob->h / 2,
                                                 SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS | SDL_WINDOW_SHOWN |
                                                 SDL_WINDOW_ALLOW_HIGHDPI);
    mainprogram->requesterwindow = SDL_CreateWindow("EWOCvj2", glob->w / 4, glob->h / 4, glob->w / 2,
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

    if (FT_Init_FreeType(&ft)) {
        fprintf(stderr, "Could not init freetype library\n");
        return 1;
    }
    FT_UInt interpreter_version = 40;
    FT_Property_Set(ft, "truetype", "interpreter-version", &interpreter_version);

#ifdef WINDOWS
    std::string fstr = mainprogram->fontpath + "/expressway.ttf";
    if (!exists(fstr)) {
        fstr = "./expressway.ttf";
        if (!exists(fstr)) {
            mainprogram->quitting = "Can't find \"expressway.ttf\" TrueType font in current directory";
        }
    }
#else
    #ifdef POSIX
    mainprogram->appimagedir = "";
    if (std::getenv("APPDIR")) {
        mainprogram->appimagedir = std::getenv("APPDIR");
    }
    mainprogram->ffgldir = mainprogram->appimagedir + "/usr/share/FFGL";
    mainprogram->isfdir = mainprogram->appimagedir + "/usr/share/ISF";
    std::string fdir(mainprogram->appimagedir + mainprogram->fontdir);
    std::string fstr;
    if (mainprogram->appimagedir == "") {
        fstr = fdir + "/expressway.ttf";
    }
    else {
        fstr = fdir + "/truetype/expressway.ttf";
    }
    printf("%s /n", fstr.c_str());
    fflush(stdout);
    if (!exists(fstr))
        mainprogram->quitting = "Can't find \"expressway.ttf\" TrueType font";
#endif
#endif
    if (FT_New_Face(ft, fstr.c_str(), 0, &face)) {
        fprintf(stderr, "Could not open font %s\n", fstr.c_str());
        return 1;
    }
    FT_Set_Pixel_Sizes(face, 0, 48);

    SDL_GL_MakeCurrent(mainprogram->splashwindow, glc);
    SDL_RaiseWindow(mainprogram->splashwindow);
    ILuint splash;
    ilGenImages(1, &splash);
    ilBindImage(splash);
    ilActiveImage(0);
#ifdef WINDOWS
    bool ret = ilLoadImage((const ILstring)"./splash.jpeg");
#endif
#ifdef POSIX
    std::string spljpeg = mainprogram->appimagedir + "/usr/share/ewocvj2/splash.jpeg";
    bool ret = ilLoadImage(spljpeg.c_str());
#endif
    if (ret == IL_FALSE) {
        printf("can't load splash image\n");
        fflush(stdout);
    }
    int w = ilGetInteger(IL_IMAGE_WIDTH);
    int h = ilGetInteger(IL_IMAGE_HEIGHT);
    glGenTextures(1, &mainprogram->splashtex);
    glBindTexture(GL_TEXTURE_2D, mainprogram->splashtex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, (char *) ilGetData());
    glGenFramebuffers(1, &mainprogram->splashfbo);
    glBindFramebuffer(GL_FRAMEBUFFER, mainprogram->splashfbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mainprogram->splashtex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDrawBuffer(GL_FRONT);
    glViewport(0, 0, glob->h / 2.0f, glob->h / 2.0f);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, mainprogram->splashfbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // Default framebuffer
    int size = glob->h / 2.0f;
    glBlitFramebuffer(
            0, 0, 1024, 1024,
            0, 0, size, size,
            GL_COLOR_BUFFER_BIT,
            GL_LINEAR
    );
    glFlush();
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDrawBuffer(GL_FRONT);

    mainprogram->ShaderProgram = mainprogram->set_shader();
    mainprogram->uniformCache = new UniformCache(mainprogram->ShaderProgram);
    glUseProgram(mainprogram->ShaderProgram);

    mainprogram->shelves[0] = new Shelf(0);
    mainprogram->shelves[1] = new Shelf(1);
    mainprogram->shelves[0]->erase();
    mainprogram->shelves[1]->erase();
    mainprogram->cwbox->vtxcoords->w = glob->w / 5.0f;
    mainprogram->cwbox->vtxcoords->h = glob->h / 5.0f;
    mainprogram->cwbox->upvtxtoscr();


    mainprogram->nodesmain = new NodesMain;
    mainprogram->nodesmain->add_nodepages(1);
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
    mainprogram->oldow = mainprogram->ow[1];
    mainprogram->oldoh = mainprogram->oh[1];
    mainprogram->set_ow3oh3();

    mainprogram->uniformCache->setSampler("Sampler0", 0);

    //temporary
    mainprogram->nodesmain->linked = true;

    MixNode *mixnodeA = mainprogram->nodesmain->currpage->add_mixnode(0, false);
    MixNode *mixnodeB = mainprogram->nodesmain->currpage->add_mixnode(0, false);
    MixNode *mixnodeAB = mainprogram->nodesmain->currpage->add_mixnode(0, false);
    mainprogram->bnodeend[0] = mainprogram->nodesmain->currpage->add_blendnode(CROSSFADING, false);
    mainprogram->nodesmain->currpage->connect_nodes(mixnodeA, mixnodeB, mainprogram->bnodeend[0]);
    mainprogram->nodesmain->currpage->connect_nodes(mainprogram->bnodeend[0], mixnodeAB);

    MixNode *mixnodeAcomp = mainprogram->nodesmain->currpage->add_mixnode(0, true);
    MixNode *mixnodeBcomp = mainprogram->nodesmain->currpage->add_mixnode(0, true);
    MixNode *mixnodeABcomp = mainprogram->nodesmain->currpage->add_mixnode(0, true);
    mainprogram->bnodeend[1] = mainprogram->nodesmain->currpage->add_blendnode(CROSSFADING, true);
    mainprogram->nodesmain->currpage->connect_nodes(mixnodeAcomp, mixnodeBcomp, mainprogram->bnodeend[1]);
    mainprogram->nodesmain->currpage->connect_nodes(mainprogram->bnodeend[1], mixnodeABcomp);

    mainprogram->uniformCache->setSampler("endSampler0", 1);
    mainprogram->uniformCache->setSampler("endSampler1", 2);

    laymidiA = new LayMidi;
    laymidiB = new LayMidi;
    laymidiC = new LayMidi;
    laymidiD = new LayMidi;
    if (exists(mainprogram->docpath + "/midiset.gm")) open_genmidis(mainprogram->docpath + "midiset.gm");

    mainprogram->uniformCache->setBool("preff", true);

    // make init layer stacks
    for (int n = 0; n < 2; n++) {
        for (int m = 0; m < 2; m++) {
            std::vector<Layer *> &lvec = mainmix->layers[n * 2 + m];
            Layer *lay = mainmix->add_layer(lvec, 0);
            lay->filename = "";
        }
    }
    mainmix->currlay[0] = mainmix->layers[0][0];
    mainmix->currlay[1] = mainmix->layers[2][0];
    mainmix->currlays[0] = {mainmix->currlay[0]};
    mainmix->currlays[1] = {mainmix->currlay[1]};
    // make init scene layers stacks
    for (int n = 1; n < 4; n++) {
        for (int m = 0; m < 2; m++) {
            std::vector<Layer *> &lvec = mainmix->scenes[m][n]->scnblayers;
            Layer *lay = mainmix->add_layer(lvec, 0);
            lay->filename = "";
        }
    }

    SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);


    // load background graphic
    //ilEnable(IL_CONV_PAL);
    ILuint bg_ol;
    ilGenImages(1, &bg_ol);
    ilBindImage(bg_ol);
    ilActiveImage(0);
#ifdef WINDOWS
    ret = ilLoadImage((const ILstring)"./background.png");
#endif
#ifdef POSIX
    std::string bstr = mainprogram->appimagedir + "/usr/share/ewocvj2/background.png";
    ret = ilLoadImage(bstr.c_str());
#endif
    if (ret == IL_FALSE) {
        printf("can't load background image\n");
    }
    w = ilGetInteger(IL_IMAGE_WIDTH);
    h = ilGetInteger(IL_IMAGE_HEIGHT);
    glGenTextures(1, &mainprogram->bgtex);
    glBindTexture(GL_TEXTURE_2D, mainprogram->bgtex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, (char *) ilGetData());

    // load background graphic
    //ilEnable(IL_CONV_PAL);

    SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);

    set_glstructures();

    /*glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    draw_direct(nullptr, black, -2.0f, -1.0f, 4.0f, 2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, mainprogram->bgtex, glob->w, glob->h, false, false);
*/

#ifdef WINDOWS
    std::filesystem::path full_path(std::filesystem::current_path());
    printf("PATH %s", full_path.string().c_str());
    printf("\n");
    std::string pp(full_path.string() + "/lock.png");
    ILboolean ret2 = ilLoadImage((const ILstring)pp.c_str());
    mainprogram->ffgldir = mainprogram->docpath + "/ffglplugins";
    if (!exists(mainprogram->ffgldir)) {
        std::filesystem::create_directories(mainprogram->ffgldir);
    }
    mainprogram->isfdir = "C:/ProgramData/ISF";
    if (!exists(mainprogram->isfdir)) {
        std::filesystem::create_directories(mainprogram->isfdir);
    }
#endif
#ifdef POSIX
    std::string lstr = mainprogram->appimagedir + "/usr/share/ewocvj2/lock.png";
    ILboolean ret2 = ilLoadImage(lstr.c_str());
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
        // Get local IP from network interfaces
#ifdef POSIX
        struct ifaddrs *ifaddrs_ptr;
        if (getifaddrs(&ifaddrs_ptr) == 0) {
            for (struct ifaddrs *ifa = ifaddrs_ptr; ifa != NULL; ifa = ifa->ifa_next) {
                if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET && ifa->ifa_netmask && 
                    (ifa->ifa_flags & IFF_UP) && (ifa->ifa_flags & IFF_RUNNING) && !(ifa->ifa_flags & IFF_LOOPBACK)) {
                    struct sockaddr_in* addr_in = (struct sockaddr_in*)ifa->ifa_addr;
                    char* ip_str = inet_ntoa(addr_in->sin_addr);
                    // Skip loopback and look for local network addresses
                    if (strncmp(ip_str, "127.", 4) != 0 && strncmp(ip_str, "169.254.", 8) != 0) {
                        mainprogram->localip = ip_str;
                        // Use global broadcast instead of subnet detection
                        mainprogram->broadcastip = "255.255.255.255";
                        std::cout << "DEBUG: Interface " << ifa->ifa_name << " - IP: " << ip_str << ", Using global broadcast: " << mainprogram->broadcastip << std::endl;
                        break;
                    }
                }
            }
            freeifaddrs(ifaddrs_ptr);
        }
#endif
#ifdef WINDOWS
        // Get local IP info on Windows
        ULONG bufferSize = sizeof(IP_ADAPTER_INFO);
        PIP_ADAPTER_INFO adapterInfo = (IP_ADAPTER_INFO*)malloc(bufferSize);
        if (GetAdaptersInfo(adapterInfo, &bufferSize) == ERROR_BUFFER_OVERFLOW) {
            free(adapterInfo);
            adapterInfo = (IP_ADAPTER_INFO*)malloc(bufferSize);
        }
        
        if (GetAdaptersInfo(adapterInfo, &bufferSize) == NO_ERROR) {
            PIP_ADAPTER_INFO adapter = adapterInfo;
            while (adapter) {
                if (adapter->Type == MIB_IF_TYPE_ETHERNET || adapter->Type == IF_TYPE_IEEE80211) {
                    std::string ip_str = adapter->IpAddressList.IpAddress.String;
                    
                    if (ip_str != "0.0.0.0" && ip_str.substr(0, 4) != "127." && ip_str.substr(0, 8) != "169.254.") {
                        mainprogram->localip = ip_str;
                        // Use global broadcast instead of subnet detection
                        mainprogram->broadcastip = "255.255.255.255";
                        std::cout << "DEBUG: Windows adapter - IP: " << ip_str << ", Using global broadcast: " << mainprogram->broadcastip << std::endl;
                        break;
                    }
                }
                adapter = adapter->Next;
            }
        }
        free(adapterInfo);
#endif
        if (mainprogram->localip.empty()) {
            mainprogram->localip = "127.0.0.1"; // fallback
        }
        // Always use global broadcast
        mainprogram->broadcastip = "255.255.255.255";
        std::cout << "Local IP address is: " << mainprogram->localip << std::endl;

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


    mainprogram->ffglhost = new FFGLHost("EWOCvj2", "0.9");;

    // load installed ffgl plugins
    for (std::filesystem::directory_iterator iter(mainprogram->ffgldir), end; iter != end; ++iter) {
        if (!mainprogram->ffglhost->loadPlugin(iter->path().string())) {
            std::cerr << "Failed to load plugin: " << iter->path().string() << std::endl;
            std::cout << "Make sure you have a valid FFGL plugin in the plugins directory." << std::endl;
            continue;
        }

        auto plugin = mainprogram->ffglhost->getPlugin(iter->path().string());

        std::string name = plugin->getDisplayName();
        name.erase(std::find(name.begin(), name.end(), '\0'), name.end());
        std::transform(name.begin(), name.end(), name.begin(), ::toupper);
        mainprogram->ffglplugins.push_back(plugin);
        if (plugin->pluginInfo.PluginType == FF_EFFECT) {
            mainprogram->ffgleffectplugins.push_back(plugin);
            mainprogram->ffgleffectnames.push_back(name);
        }
        else if (plugin->pluginInfo.PluginType == FF_SOURCE) {
            mainprogram->ffglsourceplugins.push_back(plugin);
            mainprogram->ffglsourcenames.push_back(name);
        }
        else if (plugin->pluginInfo.PluginType == FF_MIXER) {
            mainprogram->ffglmixerplugins.push_back(plugin);
            mainprogram->ffglmixernames.push_back(name);
        }
    }

    // load installed isf plugins
    SDL_GL_MakeCurrent(mainprogram->splashwindow, glc);
    mainprogram->isfloader.loadISFDirectory(mainprogram->isfdir);
    SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
    std::vector<std::string> myShaderNames;
    mainprogram->isfloader.getShaderNames(myShaderNames);
    for (auto &name : myShaderNames) {
        std::transform(name.begin(), name.end(), name.begin(), ::toupper);
    }
    std::sort(myShaderNames.begin(), myShaderNames.end());
    for (auto name : myShaderNames) {
        auto shader = mainprogram->isfloader.findShader(name);
        if (shader->getType() == ISFLoader::EFFECT) {
            auto inputs = shader->getInputInfo();
            if (shader->getInputCount() == 1) {
                mainprogram->isfeffectnames.push_back(name);
            }
            else if (shader->getInputCount() == 2) {
                if (inputs[1].type != ISFLoader::INPUT_EXTERNAL_IMAGE) {
                    mainprogram->isfmixernames.push_back(name);
                }
                else {
                    mainprogram->isfeffectnames.push_back(name);
                }
            }
        }
        else if (shader->getType() == ISFLoader::GENERATOR) {
            mainprogram->isfsourcenames.push_back(name);
        }
        mainprogram->isfinstances.push_back({});
    }

    // define all menus
    mainprogram->define_menus();

    // start audio input processing in separate thread
    if (mainprogram->audevice != "") {
        mainprogram->init_audio(mainprogram->audevice.c_str());
    }
    std::thread auin(&Program::process_audio, mainprogram);
    auin.detach();


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

    glViewport(0, 0, glob->w, glob->h);
    SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDrawBuffer(GL_BACK_LEFT);


    std::filesystem::current_path(pathtoplatform(mainprogram->docpath));

    SDL_EventState(SDL_DROPFILE, SDL_ENABLE);
    SDL_EventState(SDL_DROPBEGIN, SDL_ENABLE);





    mainprogram->io = new boost::asio::io_context();

    SDL_DestroyWindow(mainprogram->splashwindow);


    while (!quit) {

        mainprogram->io->poll();

        if (mainprogram->path != "" || mainprogram->paths.size()) {
            mainprogram->path = pathtoplatform(mainprogram->path);
            for (int i = 0; i < mainprogram->paths.size(); i++) {
                mainprogram->paths[i] = pathtoplatform(mainprogram->paths[i]);
            }
            if (mainprogram->pathto == "ADDSEARCHDIR") {
                if (mainprogram->path != "") {
                    mainprogram->prefsearchdirs->push_back(mainprogram->path);
                }
                mainprogram->filereqon = false;
            }
            else if (mainprogram->pathto == "OPENFILESLAYER") {
                if (mainprogram->paths.size() > 0) {
                    std::vector<Layer *> &lvec = choose_layers(mainmix->mousedeck);
                    std::string str(mainprogram->paths[0]);
                    mainprogram->currfilesdir = dirname(str);
                    mainprogram->pathscount = 0;
                    mainprogram->fileslay = mainprogram->loadlay;
                    mainprogram->openfileslayers = true;
                }
            } else if (mainprogram->pathto == "OPENFILESSTACK") {
                if (mainprogram->paths.size()) {
                    std::vector<Layer *> &lvec = choose_layers(mainmix->mousedeck);
                    std::string str(mainprogram->paths[0]);
                    mainprogram->currfilesdir = dirname(str);
                    mainprogram->pathscount = 0;
                    mainprogram->openfileslayers = true;
                }
            } else if (mainprogram->pathto == "OPENFILESQUEUE") {
                if (mainprogram->paths.size()) {
                    std::string str(mainprogram->paths[0]);
                    mainprogram->currfilesdir = dirname(str);
                    mainprogram->pathscount = 0;
                    mainprogram->fileslay = mainprogram->loadlay;
                    for (Clip *clip: *mainprogram->fileslay->clips) {
                        delete clip;
                    }
                    mainprogram->fileslay->clips->clear();
                    Clip *clip = new Clip;  // empty never-active clip at queue end for adding to queue from the GUI
                    clip->insert(mainprogram->fileslay, mainprogram->fileslay->clips->end());
                    if (mainmix->addlay) {
                        std::vector<Layer *> &lvec = choose_layers(mainmix->mousedeck);
                        mainprogram->fileslay = mainmix->add_layer(lvec, lvec.size());
                        mainmix->addlay = false;
                    }
                    mainprogram->fileslay->cliploading = true;
                    mainprogram->openfilesqueue = true;
                }
            } else if (mainprogram->pathto == "OPENFILESCLIP") {
                if (mainprogram->paths.size()) {
                    std::string str(mainprogram->paths[0]);
                    mainprogram->currfilesdir = dirname(str);
                    mainprogram->clipfilescount = 0;
                    mainprogram->clipfilesclip = mainmix->mouseclip;
                    mainprogram->clipfileslay = mainmix->mouselayer;
                    mainprogram->clipfileslay->cliploading = true;
                    mainprogram->openclipfiles = true;
                }
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
                    std::filesystem::path p1{dest};
                    if (!std::filesystem::exists(p1)) {
                        std::filesystem::create_directory(p1);
                    }
                    copy_dir(src, dest);
                } else {
                    dest = dirname(mainprogram->path);
                    copy_dir(src, dest);
                }
                std::filesystem::path p3{dest + "/" + mainmix->mouseshelf->basepath + ".shelf"};
                if (std::filesystem::exists(p3)) {
                    mainprogram->remove(mainprogram->path);
                }
                mainmix->mouseshelf->save(dest + "/" + remove_extension(basename(mainprogram->path)) + ".shelf");
            } else if (mainprogram->pathto == "OPENFILESSHELF") {
                if (mainprogram->paths.size()) {
                    mainprogram->currfilesdir = dirname(mainprogram->paths[0]);
                    mainprogram->openfilesshelf = true;
                    mainprogram->shelffilescount = 0;
                    mainprogram->shelffileselem = mainmix->mouseshelfelem;
                }
            } else if (mainprogram->pathto == "SAVESTATE") {
                mainprogram->currelemsdir = dirname(mainprogram->path);
                mainmix->save_state(mainprogram->path, false);
            } else if (mainprogram->pathto == "SAVEMIX") {
                mainprogram->currelemsdir = dirname(mainprogram->path);
                mainmix->save_mix(mainprogram->path, mainprogram->prevmodus, true);
            } else if (mainprogram->pathto == "SAVEDECK") {
                mainprogram->currelemsdir = dirname(mainprogram->path);
                mainmix->save_deck(mainprogram->path, true, true);
            } else if (mainprogram->pathto == "OPENDECK") {
                mainprogram->currelemsdir = dirname(mainprogram->path);
                mainmix->open_deck(mainprogram->path, 1);
            } else if (mainprogram->pathto == "SAVELAYFILE") {
                mainprogram->currelemsdir = dirname(mainprogram->path);
                mainmix->save_layerfile(mainprogram->path, mainmix->mouselayer, 1, 0);
            }
            else if (mainprogram->pathto == "OPENSTATE") {
                mainprogram->currelemsdir = dirname(mainprogram->path);
                mainmix->open_state(mainprogram->path);
            } else if (mainprogram->pathto == "OPENMIX") {
                mainprogram->currelemsdir = dirname(mainprogram->path);
                mainmix->open_mix(mainprogram->path, true);
            } else if (mainprogram->pathto == "OPENFILESBIN") {
                if (mainprogram->paths.size()) {
                    mainprogram->currfilesdir = dirname(mainprogram->paths[0]);
                    binsmain->openfilesbin = true;
                }
            } else if (mainprogram->pathto == "OPENBIN") {
                if (mainprogram->paths.size() > 0) {
                    mainprogram->currbinsdir = dirname(mainprogram->paths[0]);
                    binsmain->importbins = true;
                    binsmain->binscount = 0;
                }
            } else if (mainprogram->pathto == "SAVEBIN") {
                mainprogram->currbinsdir = dirname(mainprogram->path);
                std::string src = pathtoplatform(mainprogram->project->binsdir + binsmain->currbin->name + "/");
                std::string dest = pathtoplatform(
                        dirname(mainprogram->path) + remove_extension(basename(mainprogram->path)) + "/");
                if (exists(dest)) {
                    std::filesystem::remove(dest + remove_extension(basename(mainprogram->path)) + ".bin");
                    std::filesystem::remove_all(dest);
                }
                copy_dir(src, dest);
                std::string bupaths[144];
                for (int j = 0; j < 12; j++) {
                    for (int i = 0; i < 12; i++) {
                        BinElement *binel = binsmain->currbin->elements[i * 12 + j];
                        std::string s =
                                dirname(mainprogram->path) + remove_extension(basename(mainprogram->path)) + "/";
                        bupaths[i * 12 + j] = binel->absjpath;\
                        if (binel->absjpath != "") {
                            binel->absjpath = s + basename(binel->absjpath);
                            binel->jpegpath = binel->absjpath;
                            binel->reljpath = std::filesystem::relative(binel->absjpath, s).generic_string();
                        }
                    }
                }
                binsmain->save_bin(
                        find_unused_filename(remove_extension(basename(mainprogram->path)), dirname(mainprogram->path),
                                             ".bin"));
                for (int j = 0; j < 12; j++) {
                    for (int i = 0; i < 12; i++) {
                        BinElement *binel = binsmain->currbin->elements[i * 12 + j];
                        bupaths[i * 12 + j] = binel->absjpath;
                        binel->absjpath = bupaths[i * 12 + j];
                        binel->jpegpath = binel->absjpath;
                        binel->reljpath = std::filesystem::relative(binel->absjpath,
                                                                    mainprogram->project->binsdir).generic_string();
                    }
                }
            } else if (mainprogram->pathto == "CHOOSEDIR") {
                mainprogram->choosedir = mainprogram->path + "/";
                //std::string driveletter1 = str.substr(0, 1);
                //std::string abspath = std::filesystem::canonical(mainprogram->docpath).generic_string();
                //std::string driveletter2 = abspath.substr(0, 1);
                //if (driveletter1 == driveletter2) {
                //	mainprogram->choosedir = std::filesystem::relative(str, mainprogram->docpath).generic_string() + "/";
                //}
                //else {
                //	mainprogram->choosedir = str + "/";
                //}
            } else if (mainprogram->pathto == "OPENAUTOSAVE") {
                //std::string p = dirname(mainprogram->path);
                if (exists(mainprogram->path)) {
                    mainprogram->openautosave = true;
                    if (!mainprogram->inautosave) {
                        mainprogram->project->bupp = mainprogram->project->path;
                        mainprogram->project->bupn = mainprogram->project->name;
                        mainprogram->project->bubd = mainprogram->project->binsdir;
                        mainprogram->project->busd = mainprogram->project->shelfdir;
                        mainprogram->project->burd = mainprogram->project->recdir;
                        //mainprogram->project->buad = mainprogram->project->autosavedir;
                        mainprogram->project->bued = mainprogram->project->elementsdir;
                    }
                    std::string ext = mainprogram->path.substr(mainprogram->path.length() - 7, std::string::npos);

                    mainprogram->project->open(mainprogram->path, true);

                    binsmain->clear_undo();

                    mainprogram->openautosave = false;
                }
            } else if (mainprogram->pathto == "NEWPROJECT") {
                mainprogram->currprojdir = dirname(mainprogram->path);
                mainmix->new_state();
#ifdef POSIX
                mainprogram->project->newp(mainprogram->path + "/" + basename(mainprogram->path));
#endif
#ifdef WINDOWS
                mainprogram->project->newp(mainprogram->path + "\\" + basename(mainprogram->path));
#endif
                binsmain->clear_undo();
                mainprogram->undo_redo_save();
            } else if (mainprogram->pathto == "OPENPROJECT") {
                std::string p = dirname(mainprogram->path);
                if (exists(mainprogram->path)) {
                    mainprogram->project->open(mainprogram->path, false);
                    binsmain->clear_undo();
                    mainprogram->currprojdir = dirname(p.substr(0, p.length() - 1));
                }
            } else if (mainprogram->pathto == "SAVEPROJECT") {
                mainprogram->project->save_as();
            } else if (mainprogram->pathto == "FFGLFILE") {
                mainprogram->ffglfiledir = dirname(mainprogram->path);
                mainmix->mouseparam->oldvaluestr = mainmix->mouseparam->valuestr;
                mainmix->mouseparam->valuestr = mainprogram->path;
                // UNDO registration
                mainprogram->register_undo(mainmix->mouseparam, nullptr);
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
                            if (mainprogram->intoparea) {
                                // for when a linux grahical environment has a top bar
                                mainprogram->intopmenu = true;
                                mainprogram->exitedtop = true;
                            }
                            if (binsmain->floating) SDL_SetWindowInputFocus(binsmain->win);
                        }
                        if (binsmain->floating) {
                            if (e.window.windowID == SDL_GetWindowID(binsmain->win)) {
                                SDL_SetWindowInputFocus(mainprogram->mainwindow);
                                binsmain->inbinwin = false;
                            }
                        }
                    }
                } else if (we.event == SDL_WINDOWEVENT_ENTER) {
                    if (e.window.windowID == SDL_GetWindowID(mainprogram->mainwindow)) {
                        mainprogram->exitedtop = false;
                    }
                    if (binsmain->floating) {
                        if (e.window.windowID == SDL_GetWindowID(binsmain->win)) {
                            binsmain->inbinwin = true;
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
                        if (mainprogram->renaming == EDIT_FLOATPARAM) {
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
                                bool dummy = false;
                            }
                            mainmix->adaptnumparam = nullptr;
                        }
                        mainmix->adapttextparam = nullptr;
                        mainprogram->renaming = EDIT_NONE;
                        // UNDO registration
                        mainprogram->register_undo(mainmix->adapttextparam, nullptr);
                        mainprogram->recundo = true;
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
                } else if (mainprogram->renaming == EDIT_BINELEMNAME) {
                    binsmain->renamingelem->name = mainprogram->inputtext;
                } else if (mainprogram->renaming == EDIT_SHELFELEMNAME) {
                    mainprogram->renamingshelfelem->name = mainprogram->inputtext;
                } else if (mainprogram->renaming == EDIT_TEXTPARAM) {
                    mainmix->adapttextparam->valuestr = mainprogram->inputtext;
                } else if (mainprogram->renaming == EDIT_FLOATPARAM) {
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
                                                "application/ewocvj2-state", std::filesystem::canonical(
                                            mainprogram->currelemsdir).generic_string());
                            filereq.detach();
                        }
                        if (e.key.keysym.sym == SDLK_o) {
                            mainprogram->pathto = "OPENSTATE";
                            std::thread filereq(&Program::get_inname, mainprogram, "Open state file",
                                                "application/ewocvj2-state", std::filesystem::canonical(
                                            mainprogram->currelemsdir).generic_string());
                            filereq.detach();
                        }
                        if (e.key.keysym.sym == SDLK_n) {
                            mainmix->new_file(2, 1, true);
                        }
                        if (e.key.keysym.sym == SDLK_z) {  // UNDO
                            if (mainprogram->undoon && !loopstation->foundrec) {
                                if (!mainprogram->binsscreen) {
                                    if (mainprogram->undomapvec[mainprogram->undopos - 1].size() &&
                                        mainprogram->undopbpos > 1) {
                                        mainprogram->undo_redo_parbut(-2);
                                        mainprogram->undopbpos--;
                                    } else if (mainprogram->undopaths.size() && mainprogram->undopos > 1) {
                                        mainprogram->project->bupp = mainprogram->project->path;
                                        mainprogram->project->bupn = mainprogram->project->name;
                                        mainprogram->project->bubd = mainprogram->project->binsdir;
                                        mainprogram->project->busd = mainprogram->project->shelfdir;
                                        mainprogram->project->burd = mainprogram->project->recdir;
                                        mainprogram->project->buad = mainprogram->project->autosavedir;
                                        mainprogram->project->bued = mainprogram->project->elementsdir;

                                        mainprogram->project->open(mainprogram->undopaths[mainprogram->undopos - 2],
                                                                   false,
                                                                   false, true);
                                        mainprogram->project->binsdir = mainprogram->project->bubd;
                                        mainprogram->project->shelfdir = mainprogram->project->busd;
                                        mainprogram->project->recdir = mainprogram->project->burd;
                                        mainprogram->project->autosavedir = mainprogram->project->buad;
                                        mainprogram->project->elementsdir = mainprogram->project->bued;
                                        mainprogram->project->path = mainprogram->project->bupp;
                                        mainprogram->project->name = mainprogram->project->bupn;

                                        mainprogram->undopos--;
                                        if (mainprogram->undopos > 0) {
                                            mainprogram->undopbpos =
                                                    mainprogram->undomapvec[mainprogram->undopos - 1].size();
                                        }
                                        if (mainprogram->undopbpos != 0) {
                                            std::unordered_set<Param *> params;
                                            std::unordered_set<Button *> buttons;
                                            for (int i = mainprogram->undomapvec[mainprogram->undopos - 1].size() - 1;
                                                 i >= 0; i--) {
                                                auto tup1 = mainprogram->undomapvec[mainprogram->undopos - 1][i];
                                                auto tup2 = std::get<0>(tup1);
                                                Param *par = std::get<0>(tup2);
                                                Button *but = std::get<1>(tup2);
                                                if (par && !params.contains(par)) {
                                                    auto tup = mainprogram->newparam(i - mainprogram->undomapvec[
                                                            mainprogram->undopos - 1].size(), true);
                                                    Param *newpar = std::get<0>(tup);
                                                    if (par->type == FF_TYPE_TEXT || par->type == FF_TYPE_FILE) {
                                                        newpar->valuestr = std::get<std::string>(std::get<1>(tup1));
                                                    }
                                                    else {
                                                        newpar->value = std::get<float>(std::get<1>(tup1));
                                                    }
                                                    params.emplace(par);
                                                }
                                                if (but && !buttons.contains(but)) {
                                                    auto tup = mainprogram->newbutton(i - mainprogram->undomapvec[
                                                            mainprogram->undopos - 1].size(), true);
                                                    Button *newbut = std::get<0>(tup);
                                                    newbut->value = std::get<float>(std::get<1>(tup1));
                                                    buttons.emplace(but);
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    if (binsmain->undobins.size() && binsmain->undopos > 0) {
                                        binsmain->undo_redo(-2);
                                        binsmain->undopos--;
                                    }
                                }
                            }
                        }
                        if (e.key.keysym.sym == SDLK_y) {  // UNDO
                            if (mainprogram->undoon && !loopstation->foundrec) {
                                if (!mainprogram->binsscreen) {
                                    if (mainprogram->undomapvec[mainprogram->undopos - 1].size() >
                                        mainprogram->undopbpos) {
                                        mainprogram->undo_redo_parbut(0);
                                        mainprogram->undopbpos++;
                                    } else if (mainprogram->undopaths.size() > mainprogram->undopos) {
                                        mainprogram->project->bupp = mainprogram->project->path;
                                        mainprogram->project->bupn = mainprogram->project->name;
                                        mainprogram->project->bubd = mainprogram->project->binsdir;
                                        mainprogram->project->busd = mainprogram->project->shelfdir;
                                        mainprogram->project->burd = mainprogram->project->recdir;
                                        mainprogram->project->buad = mainprogram->project->autosavedir;
                                        mainprogram->project->bued = mainprogram->project->elementsdir;

                                        mainprogram->project->open(mainprogram->undopaths[mainprogram->undopos], false,
                                                                   false, true);
                                        mainprogram->project->binsdir = mainprogram->project->bubd;
                                        mainprogram->project->shelfdir = mainprogram->project->busd;
                                        mainprogram->project->recdir = mainprogram->project->burd;
                                        mainprogram->project->autosavedir = mainprogram->project->buad;
                                        mainprogram->project->elementsdir = mainprogram->project->bued;
                                        mainprogram->project->path = mainprogram->project->bupp;
                                        mainprogram->project->name = mainprogram->project->bupn;

                                        mainprogram->undopos++;
                                        mainprogram->undopbpos = 0;
                                    }
                                } else {
                                    if (binsmain->undobins.size() > binsmain->undopos) {
                                        binsmain->undo_redo(0);
                                        binsmain->undopos++;
                                    }
                                }
                            }
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
                            loopstation->currelem = loopstation->elements[rowpos];
                        }
                        if (e.key.keysym.sym == SDLK_UP) {
                            int rowpos = loopstation->currelem->pos;
                            rowpos--;
                            if (rowpos < 0) {
                                rowpos = 7;
                            }
                            loopstation->currelem = loopstation->elements[rowpos];
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
                    delete mainprogram->tooltipbox;
                    mainprogram->tooltipbox = nullptr;
                }
                if (mainmix->prepadaptparam && !mainprogram->wiping) {
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
                    if (!mainprogram->cwon) {
                        mainprogram->menuactivation = true;
                    }
                    mainprogram->menuondisplay = false;
                }
                if (e.button.button == SDL_BUTTON_LEFT) {
                    if (e.button.clicks == 2) {
                        mainprogram->doubleleftmouse = true;
                        mainprogram->recundo = false;
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

            if (e.type == SDL_DROPFILE) {
                mainprogram->dropfiles.push_back(e.drop.file);
            }
        }


        float deepred[4] = {1.0, 0.0, 0.0, 1.0};
        float lightblue[] = {0.5f, 0.5f, 1.0f, 1.0f};
        glClearColor(0.2f, 0.2f, 0.2f, 0.2f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        mainprogram->currprojdir = mainprogram->projdir;

        if (!mainprogram->startloop) {
            if (mainprogram->firsttime) {
                mainprogram->firsttime = false;
                mainprogram->leftmouse = false;
                mainprogram->rightmouse = false;
            }
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
            mainprogram->textbdvptr[0] = mainprogram->textbdcoords[0];
            mainprogram->textbdtcptr[0] = mainprogram->textbdtexcoords[0];
            mainprogram->textbdcptr[0] = mainprogram->textbdcolors[0];
            mainprogram->textbdtptr[0] = mainprogram->textbdtexes[0];
            mainprogram->textbdtnptr[0] = mainprogram->textboxtexes[0];
            mainprogram->textcountingtexes[0] = 0;
            mainprogram->textcurrbatch = 0;

            Boxx box;

#ifdef WINDOWS
                // user opened a .ewocvj file in Explorer
            int w_argc = 0;
            LPWSTR w_argv = CommandLineToArgvW(GetCommandLineW(), &w_argc)[1];
            if (w_argv) {
                std::wstring ws = std::wstring(w_argv);
                std::string path = std::string(ws.begin(), ws.end());
                if (path != "") {
                    mainprogram->project->open(path, false, true);
                    mainprogram->undowaiting = 2;
                    mainprogram->binsscreen = true;
                    mainprogram->undo_redo_save();
                    mainprogram->binsscreen = false;
                    mainprogram->undo_redo_save();
                    std::string p = dirname(mainprogram->path);
                    mainprogram->currprojdir = dirname(p.substr(0, p.length() - 1));
                    mainprogram->path = "";
                    mainprogram->startloop = true;
                }
            }
#endif

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
                    std::string filepath = std::filesystem::canonical(mainprogram->currprojdir).generic_string() + "/" + name;
                    mainprogram->get_outname("Type name of new project (directory)", "", filepath);
                    if (mainprogram->path != "") {
                        SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
#ifdef WINDOWS
                        mainprogram->project->newp(mainprogram->path + "\\" + basename(mainprogram->path));
#endif
#ifdef POSIX
                        mainprogram->project->newp(mainprogram->path + "/" + basename(mainprogram->path));
#endif
                        mainprogram->undowaiting = 2;
                        binsmain->clear_undo();
                        mainprogram->undo_redo_save();
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
                    mainprogram->get_inname("Open project", "application/ewocvj2-project", std::filesystem::canonical(mainprogram->currprojdir).generic_string());
                    if (mainprogram->path != "") {
                        SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
                        mainprogram->project->open(mainprogram->path, false, true);
                        mainprogram->undowaiting = 2;
                        mainprogram->binsscreen = true;
                        mainprogram->undo_redo_save();
                        mainprogram->binsscreen = false;
                        mainprogram->undo_redo_save();
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
                        bool ret = mainprogram->project->open(mainprogram->recentprojectpaths[i], false, true);
                        mainprogram->undowaiting = 2;
                        mainprogram->binsscreen = true;
                        mainprogram->undo_redo_save();
                        mainprogram->binsscreen = false;
                        mainprogram->undo_redo_save();
                        if (ret) {
                            std::string p = dirname(mainprogram->recentprojectpaths[i]);
                            mainprogram->currprojdir = dirname(p.substr(0, p.length() - 1));
                            mainprogram->startloop = true;
                            brk = true;
                            mainprogram->leftmouse = false;
                            break;
                        }
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
                        exit(0);
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
            mainprogram->uniformCache->setFloat("iGlobalTime", mainmix->time);



            // Start network discovery for local seats (only once)
            if (!mainprogram->discoveryInitialized) {
                mainprogram->start_discovery();
                mainprogram->discoveryInitialized = true;
            }


            the_loop();  // main loop




        }
    }

    mainprogram->quitting = "stop";

}