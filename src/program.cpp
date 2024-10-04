#if defined(WIN32) && !defined(__linux__)
#define WINDOWS
#elif defined(__linux__) && !defined(WIN32)
#define POSIX
#endif

#include <boost/algorithm/string.hpp>

#include <ostream>
#include <istream>
#include <iostream>

#include <fstream>

#include <ios>
#include <string>
#include <codecvt>
#include <algorithm>
#include <filesystem>

#include "GL/glew.h"
#include "GL/gl.h"
#define FREEGLUT_STATIC
#define _LIB
#define FREEGLUT_LIB_PRAGMAS 0
#include "GL/freeglut.h"
#ifdef POSIX
#include <X11/Xlib.h>
#include <Xos_fixindexmacro.h>
#include <rtmidi/RtMidi.h>
#include <linux/videodev2.h>
#include <alsa/asoundlib.h>
#include <sys/ioctl.h>
#include "tinyfiledialogs.h"
#include <arpa/inet.h>
#endif

#ifdef WINDOWS
#include <tchar.h>
#include "direnthwin/dirent.h"
#include <intrin.h>
#include <shobjidl.h>
#include <winsock2.h>
#include <Vfw.h>
#include <winsock2.h>
#define STRSAFE_NO_DEPRECATE
#include <initguid.h>
#include <dshow.h>
#pragma comment (lib, "strmiids.lib")
#pragma comment (lib, "Quartz.lib")
#pragma comment (lib, "ole32.lib")
#include <windows.h>
#include <tlhelp32.h>
#include <shellscalingapi.h>
#include <comdef.h>
#endif

#ifdef WINDOWS
#include <Commdlg.h>
#include <initguid.h>
#include <KnownFolders.h>
#include <ShlObj.h>
#endif

#include "SDL2/SDL.h"
#include "SDL2/SDL_syswm.h"
#include <turbojpeg.h>

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

#include <tinyfiledialogs.h>

#define PROGRAM_NAME "EWOCvj"



typedef struct {
    // Red/Green/Blue color values struct
    double r;       // a fraction between 0 and 1
    double g;       // a fraction between 0 and 1
    double b;       // a fraction between 0 and 1
} rgb;

typedef struct {
    // Hue/Saturation/Value color values struct
    double h;       // angle in degrees
    double s;       // a fraction between 0 and 1
    double v;       // a fraction between 0 and 1
} hsv;

hsv rgb2hsv(rgb in)
{
    // convert Red/Green/Blue color values to Hue/Saturation/Value values
    hsv         out;
    double      min, max, delta;

    min = in.r < in.g ? in.r : in.g;
    min = min  < in.b ? min  : in.b;

    max = in.r > in.g ? in.r : in.g;
    max = max  > in.b ? max  : in.b;

    out.v = max;                                // v
    delta = max - min;
    if (delta < 0.00001)
    {
        out.s = 0;
        out.h = 0; // undefined, maybe nan?
        return out;
    }
    if( max > 0.0 ) { // NOTE: if Max is == 0, this divide would cause a crash
        out.s = (delta / max);                  // s
    } else {
        // if max is 0, then r = g = b = 0
        // s = 0, h is undefined
        out.s = 0.0;
        out.h = NAN;                            // its now undefined
        return out;
    }
    if( in.r >= max )                           // > is bogus, just keeps compilor happy
        out.h = ( in.g - in.b ) / delta;        // between yellow & magenta
    else
    if( in.g >= max )
        out.h = 2.0 + ( in.b - in.r ) / delta;  // between cyan & yellow
    else
        out.h = 4.0 + ( in.r - in.g ) / delta;  // between magenta & cyan

    out.h *= 60.0;                              // degrees

    if( out.h < 0.0 )
        out.h += 360.0;

    return out;
}


rgb hsv2rgb(hsv in)
{
    // convert Hue/Saturation/Value color values to Red/Green/Blue values
    double      hh, p, q, t, ff;
    long        i;
    rgb         out;

    if(in.s <= 0.0) {       // < is bogus, just shuts up warnings
        out.r = in.v;
        out.g = in.v;
        out.b = in.v;
        return out;
    }
    hh = in.h;
    if(hh >= 360.0) hh = 0.0;
    hh /= 60.0;
    i = (long)hh;
    ff = hh - i;
    p = in.v * (1.0 - in.s);
    q = in.v * (1.0 - (in.s * ff));
    t = in.v * (1.0 - (in.s * (1.0 - ff)));

    switch(i) {
        case 0:
            out.r = in.v;
            out.g = t;
            out.b = p;
            break;
        case 1:
            out.r = q;
            out.g = in.v;
            out.b = p;
            break;
        case 2:
            out.r = p;
            out.g = in.v;
            out.b = t;
            break;

        case 3:
            out.r = p;
            out.g = q;
            out.b = in.v;
            break;
        case 4:
            out.r = t;
            out.g = p;
            out.b = in.v;
            break;
        case 5:
        default:
            out.r = in.v;
            out.g = p;
            out.b = q;
            break;
    }
    return out;
}



LayMidi::LayMidi() {
    // set up a LayMidi structure: it holds settings for MIDI controlling the generic layer controls
    this->play = new MidiElement;
    this->backw = new MidiElement;
    this->pausestop = new MidiElement;
    this->bounce = new MidiElement;
    this->frforw = new MidiElement;
    this->frbackw = new MidiElement;
    this->stop = new MidiElement;
    this->loop = new MidiElement;
    this->scratch1 = new MidiElement;
    this->scratch2 = new MidiElement;
    this->scratchtouch = new MidiElement;
    this->speed = new MidiElement;
    this->speedzero = new MidiElement;
    this->opacity = new MidiElement;
    this->setcue = new MidiElement;
    this->tocue = new MidiElement;
    this->crossfade = new MidiElement;
    this->crossfadecomp = new MidiElement;
}

LayMidi::~LayMidi() {
    // kill a LayMidi structure: it held settings for MIDI controlling the generic layer controls
    delete(this->play);
    delete(this->backw);
    delete(this->pausestop);
    delete(this->bounce);
    delete(this->frforw);
    delete(this->frbackw);
    delete(this->stop);
    delete(this->loop);
    delete(this->scratch1);
    delete(this->scratch2);
    delete(this->scratchtouch);
    delete(this->speed);
    delete(this->speedzero);
    delete(this->opacity);
    delete(this->setcue);
    delete(this->tocue);
    delete(this->crossfade);
    delete(this->crossfadecomp);
}

void MidiElement::register_midi() {
    // register a MIDIlearn setup to belong to a button or a parameter
    registered_midi rm = mainmix->midi_registrations[!mainprogram->prevmodus][this->midi0][this->midi1][this->midiport];
    if (rm.but) {
        rm.but->midi[0] = -1;
        rm.but->midi[1] = -1;
        rm.but->midiport = "";
        rm.but = nullptr;
    }
    else if (rm.par) {
        rm.par->midi[0] = -1;
        rm.par->midi[1] = -1;
        rm.par->midiport = "";
        rm.par = nullptr;
    }
    else if (rm.midielem && rm.midielem != this) {
        rm.midielem->midi0 = -1;
        rm.midielem->midi1= -1;
        rm.midielem->midiport = "";
        rm.midielem = nullptr;
    }

    // put the registration structure in a list per comp mode
    // so we can get at the right button or parameter belonging to a certain MIDI control
    mainmix->midi_registrations[!mainprogram->prevmodus][this->midi0][this->midi1][this->midiport].midielem = this;
}

void MidiElement::unregister_midi() {
    // unregister a MIDI control
    mainmix->midi_registrations[!mainprogram->prevmodus][this->midi0][this->midi1][this->midiport].midielem = nullptr;
}


Program::Program() {
    // setup the main pervasive mainprogram structure
    // it holds all pervasive lements that don't directly belong to the mix (mainmix) or the bins screen (binsmain)
	this->project = new Project;

    // get the standard docpath (the EWOCvj2 directory in either Documents folder (Windows) or the home directory (Linux)
    // get the fontpath from the respective OS
    // set the contentpath to the Videos directory of the respective OS
    // get the OS temp path and create an EWOCvj2 folder in it
#ifdef WINDOWS
    wchar_t *wcharPath1 = 0;
	HRESULT hr = SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &wcharPath1);
    std::filesystem::path p1(wcharPath1);
    this->docpath = p1.generic_string() + "/EWOCvj2/";
    CoTaskMemFree(wcharPath1);
    if (!exists(this->docpath)) std::filesystem::create_directory(this->docpath);
    wchar_t *wcharPath2 = 0;
    HRESULT hr2 = SHGetKnownFolderPath(FOLDERID_Fonts, 0, nullptr, &wcharPath2);
    std::filesystem::path p2(wcharPath2);
    this->fontpath = p2.generic_string();
    CoTaskMemFree(wcharPath2);
    wchar_t *wcharPath3 = 0;
    HRESULT hr3 = SHGetKnownFolderPath(FOLDERID_Videos, 0, nullptr, &wcharPath3);
    std::filesystem::path p3(wcharPath3);
    this->contentpath = p3.generic_string() + "/";
    CoTaskMemFree(wcharPath3);
	std::wstring wstr4;
	wchar_t wcharPath[MAX_PATH];
	if (GetTempPathW(MAX_PATH, wcharPath)) wstr4 = wcharPath;
    std::filesystem::path p4(wstr4);
	this->temppath = p4.generic_string() + "EWOCvj2/";
    if (!exists(this->temppath)) std::filesystem::create_directory(std::filesystem::path(this->temppath));
#endif
#ifdef POSIX
	std::string homedir(getenv("HOME"));
	this->temppath = homedir + "/.ewocvj2/temp/";
    this->docpath = homedir + "/Documents/EWOCvj2/";
    this->contentpath = homedir + "/Videos/";
#endif

    // height of the small boxes top left and next to center holding, among other things, the scene letters
	this->numh = this->numh * glob->w / glob->h;
    // layer monitor height
	this->layh = this->layh * (glob->w / glob->h) / (1920.0f /  1080.0f);
	// height of the video output monitors
    this->monh = this->monh * (glob->w / glob->h) / (1920.0f /  1080.0f);

    // blue rectangle appearing to the right of a layer monitor when hovering its edge
    // allows inserting layers
	this->addbox = new Boxx;
	this->addbox->vtxcoords->y1 = 1.0f - this->layh;
	this->addbox->vtxcoords->w = 0.009f * 2.0f;
	this->addbox->vtxcoords->h = this->layh - 0.075f;
	this->addbox->upvtxtoscr();
	this->addbox->reserved = true;
	this->addbox->tooltiptitle = "Add layer ";
	this->addbox->tooltip = "Leftclick this blue box to add a layer at this point. ";

    // red rectangle appearing to the right of a layer monitor when hovering its edge
    // allows deleting layers
	this->delbox = new Boxx;
	this->delbox->vtxcoords->y1 = 0.925f;
	this->delbox->vtxcoords->w = 0.009f * 2.0f;
	this->delbox->vtxcoords->h = 0.075f;
	this->delbox->upvtxtoscr();
	this->delbox->reserved = true;
	this->delbox->tooltiptitle = "Delete layer to the left ";
	this->delbox->tooltip = "Leftclick this red box to delete the layer to the left of it. ";

	this->scrollboxes[0] = new Boxx;
	this->scrollboxes[0]->vtxcoords->x1 = -1.0f + this->numw;
	this->scrollboxes[0]->vtxcoords->y1 = 1.0f - this->layh - 0.05f;
	this->scrollboxes[0]->vtxcoords->w = this->layw * 3.0f;
	this->scrollboxes[0]->vtxcoords->h = 0.05f;
	this->scrollboxes[0]->upvtxtoscr();
	this->scrollboxes[0]->tooltiptitle = "Deck A Layer stack scroll bar ";
	this->scrollboxes[0]->tooltip = "Scroll bar for layer stack.  Divided in total number of layer parts for deck A. Lightgrey part is the visible layer monitors part.  Leftdragging the lightgrey part allows scrolling.  So does leftclicking the black parts. ";
	this->scrollboxes[1] = new Boxx;
	this->scrollboxes[1]->vtxcoords->x1 = this->numw;
	this->scrollboxes[1]->vtxcoords->y1 = 1.0f - this->layh - 0.05f;
	this->scrollboxes[1]->vtxcoords->w = this->layw * 3.0f;
	this->scrollboxes[1]->vtxcoords->h = 0.05f;
	this->scrollboxes[1]->upvtxtoscr();
	this->scrollboxes[1]->tooltiptitle = "Deck B Layer stack scroll bar ";
	this->scrollboxes[1]->tooltip = "Scroll bar for layer stack.  Divided in total number of layer parts for deck B. Lightgrey part is the visible layer monitors part.  Leftdragging the lightgrey part allows scrolling.  So does leftclicking the black parts. ";

    // color wheel box
	this->cwbox = new Boxx;

    // several copy_to_comp SEND boxes: they copy between preview and performance streams
    // first: copying to output streams
    // per deck
    this->toscreenA = new Button(false);
    this->toscreenA->toggle = 0;
    this->toscreenA->name[0] = "SEND";
    this->toscreenA->name[1] = "SEND";
    this->toscreenA->box->vtxcoords->x1 = -0.3;
    this->toscreenA->box->vtxcoords->y1 = -1.0f + this->monh / 2.0f;
    this->toscreenA->box->vtxcoords->w = 0.3 / 2.0;
    this->toscreenA->box->vtxcoords->h = 0.3 / 3.0;
    this->toscreenA->box->upvtxtoscr();
    this->toscreenA->box->tooltiptitle = "Send preview deck A stream to output ";
    this->toscreenA->box->tooltip = "Leftclick sends/copies the deck A stream being previewed to the output streams ";

    this->toscreenB = new Button(false);
    this->toscreenB->toggle = 0;
    this->toscreenB->name[0] = "SEND";
    this->toscreenB->name[1] = "SEND";
    this->toscreenB->box->vtxcoords->x1 = 0.15;
    this->toscreenB->box->vtxcoords->y1 = -1.0f + this->monh / 2.0f;
    this->toscreenB->box->vtxcoords->w = 0.3 / 2.0;
    this->toscreenB->box->vtxcoords->h = 0.3 / 3.0;
    this->toscreenB->box->upvtxtoscr();
    this->toscreenB->box->tooltiptitle = "Send preview deck B stream to output ";
    this->toscreenB->box->tooltip = "Leftclick sends/copies the deck B stream being previewed to the output streams ";

    // the mix
    this->toscreenM = new Button(false);
    this->toscreenM->toggle = 0;
    this->toscreenM->name[0] = "SEND";
    this->toscreenM->name[1] = "SEND";
    this->toscreenM->box->vtxcoords->x1 = 0.15;
    this->toscreenM->box->vtxcoords->y1 = -1.0f + this->monh * 2.0f - 0.1f;
    this->toscreenM->box->vtxcoords->w = 0.3 / 2.0;
    this->toscreenM->box->vtxcoords->h = 0.3 / 3.0;
    this->toscreenM->box->upvtxtoscr();
    this->toscreenM->box->tooltiptitle = "Send entire preview stream to output ";
    this->toscreenM->box->tooltip = "Leftclick sends/copies the entire stream being previewed to the output streams ";


    for (int m = 0; m < 2; m++) {
        std::string ab = "A";
        if (m == 1) ab = "B";
        for (int i = 0; i < 3; i++) {
            // for copying to/from scenes
            this->toscene[m][0][i] = new Button(false);
            this->toscene[m][0][i]->toggle = 0;
            this->toscene[m][0][i]->name[0] = "";
            this->toscene[m][0][i]->name[1] = "";
            this->toscene[m][0][i]->box->vtxcoords->x1 = -0.225f + m * (0.3f + 0.075f);
            this->toscene[m][0][i]->box->vtxcoords->h = (((-1.0f + this->monh * 2.0f - 0.1f) - (-1.0f + this->monh / 2.0f) - 0.1f) / 3.0f);
            this->toscene[m][0][i]->box->vtxcoords->y1 = (-1.0f + this->monh / 2.0f) + (2 - i) * this->toscene[m][0][i]->box->vtxcoords->h + 0.1f;
            this->toscene[m][0][i]->box->vtxcoords->w = 0.3f / 4.0f;
            this->toscene[m][0][i]->box->upvtxtoscr();
            this->toscene[m][0][i]->box->tooltiptitle = "Send deck" + ab + " preview stream to scene. ";
            this->toscene[m][0][i]->box->tooltip = "Leftclick sends/copies the deck" + ab + " stream being previewed to the scene number " + std::to_string(i) + ". ";
            this->toscene[m][1][i] = new Button(false);
            this->toscene[m][1][i]->toggle = 0;
            this->toscene[m][1][i]->name[0] = "";
            this->toscene[m][1][i]->name[1] = "";
            this->toscene[m][1][i]->box->vtxcoords->x1 = -0.3f + m * (0.3f + 0.225f);
            this->toscene[m][1][i]->box->vtxcoords->h = (((-1.0f + this->monh * 2.0f - 0.1f) - (-1.0f + this->monh / 2.0f) - 0.1f) / 3.0f);
            this->toscene[m][1][i]->box->vtxcoords->y1 = (-1.0f + this->monh / 2.0f) + (2 - i) * this->toscene[m][1][i]->box->vtxcoords->h + 0.1f;
            this->toscene[m][1][i]->box->vtxcoords->w = 0.3f / 4.0f;
            this->toscene[m][1][i]->box->upvtxtoscr();
            this->toscene[m][1][i]->box->tooltiptitle = "Send scene" + std::to_string(i) + " of output stream to deck " + ab + " of preview stream. ";
            this->toscene[m][1][i]->box->tooltip = "Leftclick sends/copies scene" + std::to_string(i) + " of ouput stream  to deck " + ab + " of the preview stream. ";
        }
    }

    // copying back to preview stream
    // per deck
    this->backtopreA = new Button(false);
    this->backtopreA->toggle = 0;
    this->backtopreA->name[0] = "SEND";
    this->backtopreA->name[1] = "SEND";
    this->backtopreA->box->vtxcoords->x1 = -0.3;
    this->backtopreA->box->vtxcoords->y1 = -1.0f + this->monh / 2.0f - 0.1f;
    this->backtopreA->box->vtxcoords->w = 0.15f;
    this->backtopreA->box->vtxcoords->h = 0.1f;
    this->backtopreA->box->upvtxtoscr();
    this->backtopreA->box->tooltiptitle = "Send output streams to preview ";
    this->backtopreA->box->tooltip = "Leftclick sends/copies the streams being output back into the preview streams ";

    this->backtopreB = new Button(false);
    this->backtopreB->toggle = 0;
    this->backtopreB->name[0] = "SEND";
    this->backtopreB->name[1] = "SEND";
    this->backtopreB->box->vtxcoords->x1 = 0.15;
    this->backtopreB->box->vtxcoords->y1 = -1.0f + this->monh / 2.0f - 0.1f;
    this->backtopreB->box->vtxcoords->w = 0.15f;
    this->backtopreB->box->vtxcoords->h = 0.1f;
    this->backtopreB->box->upvtxtoscr();
    this->backtopreB->box->tooltiptitle = "Send output streams to preview ";
    this->backtopreB->box->tooltip = "Leftclick sends/copies the streams being output back into the preview streams ";

    // the mix
    this->backtopreM = new Button(false);
    this->backtopreM->toggle = 0;
    this->backtopreM->name[0] = "SEND";
    this->backtopreM->name[1] = "SEND";
    this->backtopreM->box->vtxcoords->x1 = -0.3;
    this->backtopreM->box->vtxcoords->y1 = -1.0f + this->monh * 2.0f - 0.1f;
    this->backtopreM->box->vtxcoords->w = 0.15f;
    this->backtopreM->box->vtxcoords->h = 0.1f;
    this->backtopreM->box->upvtxtoscr();
    this->backtopreM->box->tooltiptitle = "Send entire output stream to preview ";
    this->backtopreM->box->tooltip = "Leftclick sends/copies the entire stream being output back into the preview streams ";

    // switching between preview and preformance streams
    this->modusbut = new Button(false);
	this->modusbut->toggle = 1;
	this->modusbut->name[0] = "LIVE MODUS";
	this->modusbut->name[1] = "PREVIEW MODUS";
    this->modusbut->box->lcolor[0] = 0.7f;
    this->modusbut->box->lcolor[1] = 0.7f;
    this->modusbut->box->lcolor[2] = 0.7f;
    this->modusbut->box->lcolor[3] = 1.0f;
	this->modusbut->box->vtxcoords->x1 = -0.3f - (0.3f / 2.0f);
	this->modusbut->box->vtxcoords->y1 = -1.0f + this->monh;
	this->modusbut->box->vtxcoords->w = 0.3f / 2.0f;
	this->modusbut->box->vtxcoords->h = 0.3f / 3.0f;
	this->modusbut->box->upvtxtoscr();
	this->modusbut->box->tooltiptitle = "LIVE/PREVIEW modus switch ";
	this->modusbut->box->tooltip = "Leftclicking toggles between preview modus (changes are previewed, then sent to/from output) and live modus (changes appear in output immediately) ";
	this->modusbut->tcol[0] = 0.0f;
	this->modusbut->tcol[1] = 0.0f;
	this->modusbut->tcol[2] = 0.0f;
	this->modusbut->tcol[3] = 1.0f;

    // output monitors: see tooltiptitle for explanation
	this->outputmonitor = new Boxx;
	this->outputmonitor->vtxcoords->x1 = -0.15f;
	this->outputmonitor->vtxcoords->y1 = -1.0f + this->monh;
	this->outputmonitor->vtxcoords->w = 0.3f;
	this->outputmonitor->vtxcoords->h = this->monh;
	this->outputmonitor->upvtxtoscr();
	this->outputmonitor->tooltiptitle = "Output monitor ";
	this->outputmonitor->tooltip = "Shows mix of output stream when in preview modus.  Rightclick menu allows sending monitor image fullscreen to another display device. ";

	this->mainmonitor = new Boxx;
	this->mainmonitor->vtxcoords->x1 = -0.3f;
	this->mainmonitor->vtxcoords->y1 = -1.0f;
	this->mainmonitor->vtxcoords->w = 0.6f;
	this->mainmonitor->vtxcoords->h = this->monh * 2.0f;
	this->mainmonitor->upvtxtoscr();
	this->mainmonitor->tooltiptitle = "A+B monitor ";
	this->mainmonitor->tooltip = "Shows crossfaded or wiped A+B mix of preview stream (preview modus) or output stream (live modus).  Rightclick menu allows setting wipes and sending monitor image fullscreen to another display device. Leftclick/drag on image affects some wipe modes. ";

	this->deckmonitor[0] = new Boxx;
	this->deckmonitor[0]->vtxcoords->x1 = -0.6f;
	this->deckmonitor[0]->vtxcoords->y1 = -1.0f;
	this->deckmonitor[0]->vtxcoords->w = 0.3f;
	this->deckmonitor[0]->vtxcoords->h = this->monh;
	this->deckmonitor[0]->upvtxtoscr();
	this->deckmonitor[0]->tooltiptitle = "Deck A monitor ";
	this->deckmonitor[0]->tooltip = "Shows deck A stream.  Rightclick menu allows sending monitor image fullscreen to another display device. Leftclick/drag on image affects some local layer wipe modes. ";

	this->deckmonitor[1] = new Boxx;
	this->deckmonitor[1]->vtxcoords->x1 = 0.3f;
	this->deckmonitor[1]->vtxcoords->y1 = -1.0f;
	this->deckmonitor[1]->vtxcoords->w = 0.3f;
	this->deckmonitor[1]->vtxcoords->h = this->monh;
	this->deckmonitor[1]->upvtxtoscr();
	this->deckmonitor[1]->tooltiptitle = "Deck B monitor ";
	this->deckmonitor[1]->tooltip = "Shows deck B stream.  Rightclick menu allows sending monitor image fullscreen to another display device. Leftclick/drag on image affects some local layer wipe modes. ";

    // buttons for switching between layer effects and stream effects
    // layer effects: affect one layer
    // stream effects: affect all layers up to this one

    // A deck
	this->effcat[0] = new Button(false);
    this->effcat[0]->name[0] = "effcat";
	this->effcat[0]->toggle = 1;
	this->effcat[0]->name[0] = "Layer effects";
	this->effcat[0]->name[1] = "Stream effects";
    this->effcat[0]->box->lcolor[0] = 0.7;
    this->effcat[0]->box->lcolor[1] = 0.7;
    this->effcat[0]->box->lcolor[2] = 0.7;
    this->effcat[0]->box->lcolor[3] = 1.0;
	this->effcat[0]->box->vtxcoords->x1 = -1.0f;
	this->effcat[0]->box->vtxcoords->y1 = 1.0f - this->layh * 1.5f - 0.285f;
	this->effcat[0]->box->vtxcoords->w = 0.0375f;
	this->effcat[0]->box->vtxcoords->h = 0.3f;
	this->effcat[0]->box->upvtxtoscr();
	this->effcat[0]->box->tooltiptitle = "Layer/stream effects toggle ";
	this->effcat[0]->box->tooltip = "Leftclick toggles between layer effects queue (these effects only affect the current layer) and stream effects queue (effects affect mix of all layers upto and including the current one). If set to Layer effects, a red box background notifies the user that there are stream effects enabled on this layer. ";

    //B deck
	this->effcat[1] = new Button(false);
    this->effcat[1]->name[0] = "effcat";
	this->effcat[1]->toggle = 1;
	this->effcat[1]->name[0] = "Layer effects";
	this->effcat[1]->name[1] = "Stream effects";
	float xoffset = 1.0f + this->layw - 0.019f;
    this->effcat[1]->box->lcolor[0] = 0.7;
    this->effcat[1]->box->lcolor[1] = 0.7;
    this->effcat[1]->box->lcolor[2] = 0.7;
    this->effcat[1]->box->lcolor[3] = 1.0;
	this->effcat[1]->box->vtxcoords->x1 = -1.0f + this->numw - 0.0375f + xoffset;
	this->effcat[1]->box->vtxcoords->y1 = 1.0f - this->layh * 1.5f - 0.285f;
	this->effcat[1]->box->vtxcoords->w = 0.0375f;
	this->effcat[1]->box->vtxcoords->h = 0.3f;
	this->effcat[1]->box->upvtxtoscr();
	this->effcat[1]->box->tooltiptitle = "Layer/stream effects toggle ";
	this->effcat[1]->box->tooltip = "Leftclick toggles between layer effects queue (these effects only affect the current layer) and stream effects queue (effects affect mix of all layers upto and including the current one). If set to Layer effects, a red box background notifies the user that there are stream effects enabled on this layer. ";

    // scroll boxes for effects list
    // deck A up
	this->effscrollupA = new Boxx;
	this->effscrollupA->vtxcoords->x1 = -1.0;
	this->effscrollupA->vtxcoords->y1 = 1.0 - this->layh * 1.5f;
	this->effscrollupA->vtxcoords->w = 0.0375f;
	this->effscrollupA->vtxcoords->h = 0.075f;
	this->effscrollupA->upvtxtoscr();
	this->effscrollupA->tooltiptitle = "Scroll effects queue up ";
	this->effscrollupA->tooltip = "Leftclicking scrolls the effect queue up ";

    // deck B up
	this->effscrollupB = new Boxx;
	this->effscrollupB->vtxcoords->x1 = 1.0 - 0.075f;
	this->effscrollupB->vtxcoords->y1 = 1.0 - this->layh * 1.5f;
	this->effscrollupB->vtxcoords->w = 0.0375f;
	this->effscrollupB->vtxcoords->h = 0.075f;
	this->effscrollupB->upvtxtoscr();
	this->effscrollupB->tooltiptitle = "Scroll effects queue up ";
	this->effscrollupB->tooltip = "Leftclicking scrolls the effect queue up ";

    // deck A down
	this->effscrolldownA = new Boxx;
	this->effscrolldownA->vtxcoords->x1 = -1.0;
	this->effscrolldownA->vtxcoords->y1 = 1.0 - this->layh * 1.5f - 0.375f - 0.075f;
	this->effscrolldownA->vtxcoords->w = 0.0375f;
	this->effscrolldownA->vtxcoords->h = 0.075f;
	this->effscrolldownA->upvtxtoscr();
	this->effscrolldownA->tooltiptitle = "Scroll effects queue down ";
	this->effscrolldownA->tooltip = "Leftclicking scrolls the effect queue down ";

    //deck B down
	this->effscrolldownB = new Boxx;
	this->effscrolldownB->vtxcoords->x1 = 1.0 - 0.075f;
	this->effscrolldownB->vtxcoords->y1 = 1.0 - this->layh * 1.5f - 0.375f - 0.075f;
	this->effscrolldownB->vtxcoords->w = 0.0375f;
	this->effscrolldownB->vtxcoords->h = 0.075f;
	this->effscrolldownB->upvtxtoscr();
	this->effscrolldownB->tooltiptitle = "Scroll effects queue down ";
	this->effscrolldownB->tooltip = "Leftclicking scrolls the effect queue down ";

    // scrolling entries in the list that allows reordering files to be loaded before loading
    this->orderscrollup = new Boxx;
    this->orderscrollup->vtxcoords->x1 = -0.45f;
    this->orderscrollup->vtxcoords->y1 = 0.8f;
    this->orderscrollup->vtxcoords->w = 0.05f;
    this->orderscrollup->vtxcoords->h = 0.1f;
    this->orderscrollup->upvtxtoscr();
    this->orderscrollup->tooltiptitle = "Scroll orderlist up ";
    this->orderscrollup->tooltip = "Leftclicking scrolls the orderlist up ";

    this->orderscrolldown = new Boxx;
    this->orderscrolldown->vtxcoords->x1 = -0.45f;
    this->orderscrolldown->vtxcoords->y1 = 0.7f;
    this->orderscrolldown->vtxcoords->w = 0.05f;
    this->orderscrolldown->vtxcoords->h = 0.1f;
    this->orderscrolldown->upvtxtoscr();
    this->orderscrolldown->tooltiptitle = "Scroll orderlist down ";
    this->orderscrolldown->tooltip = "Leftclicking scrolls the orderlist down ";

    // scroll the default searchlist in the preferences
    this->defaultsearchscrollup = new Boxx;
    this->defaultsearchscrollup->vtxcoords->x1 = -0.6f;
    this->defaultsearchscrollup->vtxcoords->y1 = -0.4f;
    this->defaultsearchscrollup->vtxcoords->w = 0.1f;
    this->defaultsearchscrollup->vtxcoords->h = 0.2f;
    this->defaultsearchscrollup->upvtxtoscr();
    this->defaultsearchscrollup->tooltiptitle = "Scroll default searchlist up ";
    this->defaultsearchscrollup->tooltip = "Leftclicking scrolls the default searchlist up ";

    this->defaultsearchscrolldown = new Boxx;
    this->defaultsearchscrolldown->vtxcoords->x1 = -0.6f;
    this->defaultsearchscrolldown->vtxcoords->y1 = -0.6f;
    this->defaultsearchscrolldown->vtxcoords->w = 0.1f;
    this->defaultsearchscrolldown->vtxcoords->h = 0.2f;
    this->defaultsearchscrolldown->upvtxtoscr();
    this->defaultsearchscrolldown->tooltiptitle = "Scroll default searchlist down ";
    this->defaultsearchscrolldown->tooltip = "Leftclicking scrolls the default searchlist down ";

    // scroll the search location list when retargeting files
    // retargeting is a system that allows retargeting files of a project that is being loaded that can't be found to another location
    this->searchscrollup = new Boxx;
    this->searchscrollup->vtxcoords->x1 = -0.45f;
    this->searchscrollup->vtxcoords->y1 = -0.2f;
    this->searchscrollup->vtxcoords->w = 0.05f;
    this->searchscrollup->vtxcoords->h = 0.1f;
    this->searchscrollup->upvtxtoscr();
    this->searchscrollup->tooltiptitle = "Scroll searchlist up ";
    this->searchscrollup->tooltip = "Leftclicking scrolls the searchlist up ";

    this->searchscrolldown = new Boxx;
    this->searchscrolldown->vtxcoords->x1 = -0.45f;
    this->searchscrolldown->vtxcoords->y1 = -0.3f;
    this->searchscrolldown->vtxcoords->w = 0.05f;
    this->searchscrolldown->vtxcoords->h = 0.1f;
    this->searchscrolldown->upvtxtoscr();
    this->searchscrolldown->tooltiptitle = "Scroll searchlist down ";
    this->searchscrolldown->tooltip = "Leftclicking scrolls the searchlist down ";

    // box at end of effects list: allows adding effects
    this->addeffectbox = new Boxx;
	this->addeffectbox->vtxcoords->w = this->layw * 1.5f;
	this->addeffectbox->vtxcoords->h = 0.057f;
	this->addeffectbox->tooltiptitle = "Add effect ";
	this->addeffectbox->tooltip = "Add effect to end of layer effect queue ";

    // box that pops up between effects entries of effects list: allows inserting effects
	this->inserteffectbox = new Boxx;
	this->inserteffectbox->vtxcoords->w = 0.24f;
	this->inserteffectbox->vtxcoords->h = 0.057f;
	this->inserteffectbox->tooltiptitle = "Add effect ";
	this->inserteffectbox->tooltip = "Add effect to end of layer effect queue ";

    // the category of MIDI config that is being set: see tooltiptitles
    this->tmcat[0] = new Boxx;
    this->tmcat[0]->smflag = 2;
    this->tmcat[0]->vtxcoords->x1 = -0.3f;
    this->tmcat[0]->vtxcoords->y1 = 0.92f;
    this->tmcat[0]->vtxcoords->w = 0.5f;
    this->tmcat[0]->vtxcoords->h = 0.08f;
    this->tmcat[0]->tooltiptitle = "Edit general MIDI layer controls ";
    this->tmcat[0]->tooltip = "Leftclick to toggle the settings window to the general MIDI layer controls settings. ";
    this->tmcat[1] = new Boxx;
    this->tmcat[1]->smflag = 2;
    this->tmcat[1]->vtxcoords->x1 = -0.3f;
    this->tmcat[1]->vtxcoords->y1 = 0.84f;
    this->tmcat[1]->vtxcoords->w = 0.5f;
    this->tmcat[1]->vtxcoords->h = 0.08f;
    this->tmcat[1]->tooltiptitle = "Edit shelf buttons MIDI ";
    this->tmcat[1]->tooltip = "Leftclick to toggle the settings window to the shelf buttons MIDI settings. ";
    this->tmcat[2] = new Boxx;
    this->tmcat[2]->smflag = 2;
    this->tmcat[2]->vtxcoords->x1 = -0.3f;
    this->tmcat[2]->vtxcoords->y1 = 0.76f;
    this->tmcat[2]->vtxcoords->w = 0.5f;
    this->tmcat[2]->vtxcoords->h = 0.08f;
    this->tmcat[2]->tooltiptitle = "Edit loopstation buttons MIDI";
    this->tmcat[2]->tooltip = "Leftclick to toggle the settings window to the loopstation buttons MIDI settings. ";
    // allows switching between the four possible sets of controls (A, B, C, D)
    // for general MIDI layer controls
    this->tmset[0] = new Boxx;
    this->tmset[0]->smflag = 2;
    this->tmset[0]->vtxcoords->x1 = 0.2f;
    this->tmset[0]->vtxcoords->y1 = 0.94f;
    this->tmset[0]->vtxcoords->w = 0.1f;
    this->tmset[0]->vtxcoords->h = 0.06f;
    this->tmset[0]->tooltiptitle = "Switch to set A ";
    this->tmset[0]->tooltip = "Leftclick to toggle the MIDI set that is being configured to set A. ";
    this->tmset[1] = new Boxx;
    this->tmset[1]->smflag = 2;
    this->tmset[1]->vtxcoords->x1 = 0.2f;
    this->tmset[1]->vtxcoords->y1 = 0.88f;
    this->tmset[1]->vtxcoords->w = 0.1f;
    this->tmset[1]->vtxcoords->h = 0.06f;
    this->tmset[1]->tooltiptitle = "Switch to set B ";
    this->tmset[1]->tooltip = "Leftclick to toggle the MIDI set that is being configured to set B. ";
    this->tmset[2] = new Boxx;
    this->tmset[2]->smflag = 2;
    this->tmset[2]->vtxcoords->x1 = 0.2f;
    this->tmset[2]->vtxcoords->y1 = 0.82f;
    this->tmset[2]->vtxcoords->w = 0.1f;
    this->tmset[2]->vtxcoords->h = 0.06f;
    this->tmset[2]->tooltiptitle = "Switch to set C ";
    this->tmset[2]->tooltip = "Leftclick to toggle the MIDI set that is being configured to set C. ";
    this->tmset[3] = new Boxx;
    this->tmset[3]->smflag = 2;
    this->tmset[3]->vtxcoords->x1 = 0.2f;
    this->tmset[3]->vtxcoords->y1 = 0.76f;
    this->tmset[3]->vtxcoords->w = 0.1f;
    this->tmset[3]->vtxcoords->h = 0.06f;
    this->tmset[3]->tooltiptitle = "Switch to set D ";
    this->tmset[3]->tooltip = "Leftclick to toggle the MIDI set that is being configured to set D. ";
	// boxes in general MIDI layer controls screen that allow setting specific controls
    this->tmplay = new Boxx;
    this->tmplay->smflag = 2;
	this->tmplay->vtxcoords->x1 = -0.075;
	this->tmplay->vtxcoords->y1 = -0.9f;
	this->tmplay->vtxcoords->w = 0.15f;
	this->tmplay->vtxcoords->h = 0.26f;
	this->tmplay->tooltiptitle = "Set MIDI for play button ";
	this->tmplay->tooltip = "Leftclick to start waiting for a MIDI command that will trigger normal video play for this preset. ";
	this->tmbackw = new Boxx;
    this->tmbackw->smflag = 2;
	this->tmbackw->vtxcoords->x1 = -0.375f;
	this->tmbackw->vtxcoords->y1 = -0.9f;
	this->tmbackw->vtxcoords->w = 0.15f;
	this->tmbackw->vtxcoords->h = 0.26f;
	this->tmbackw->tooltiptitle = "Set MIDI for reverse play button ";
	this->tmbackw->tooltip = "Leftclick to start waiting for a MIDI command that will trigger reverse video play for this preset. ";
	this->tmbounce = new Boxx;
    this->tmbounce->smflag = 2;
	this->tmbounce->vtxcoords->x1 = -0.225f;
	this->tmbounce->vtxcoords->y1 = -0.9f;
	this->tmbounce->vtxcoords->w = 0.15f;
	this->tmbounce->vtxcoords->h = 0.26f;
	this->tmbounce->tooltiptitle = "Set MIDI for bounce play button ";
	this->tmbounce->tooltip = "Leftclick to start waiting for a MIDI command that will trigger bounce video play for this preset. ";
	this->tmfrforw = new Boxx;
    this->tmfrforw->smflag = 2;
	this->tmfrforw->vtxcoords->x1 = 0.075f;
	this->tmfrforw->vtxcoords->y1 = -0.9f;
	this->tmfrforw->vtxcoords->w = 0.15;
	this->tmfrforw->vtxcoords->h = 0.26f;
	this->tmfrforw->tooltiptitle = "Set MIDI for frame forward button ";
	this->tmfrforw->tooltip = "Leftclick to start waiting for a MIDI command that will trigger frame forward for this preset. ";
    this->tmfrbackw = new Boxx;
    this->tmfrbackw->smflag = 2;
    this->tmfrbackw->vtxcoords->x1 = -0.525f;
    this->tmfrbackw->vtxcoords->y1 = -0.9f;
    this->tmfrbackw->vtxcoords->w = 0.15;
    this->tmfrbackw->vtxcoords->h = 0.26f;
    this->tmfrbackw->tooltiptitle = "Set MIDI for frame backward button ";
    this->tmfrbackw->tooltip = "Leftclick to start waiting for a MIDI command that will trigger frame backward for this preset. ";
    this->tmstop = new Boxx;
    this->tmstop->smflag = 2;
    this->tmstop->vtxcoords->x1 = 0.225f;
    this->tmstop->vtxcoords->y1 = -0.9f;
    this->tmstop->vtxcoords->w = 0.15;
    this->tmstop->vtxcoords->h = 0.26f;
    this->tmstop->tooltiptitle = "Set MIDI for stop play button ";
    this->tmstop->tooltip = "Leftclick to start waiting for a MIDI command that will trigger play stop for this preset. ";
    this->tmloop = new Boxx;
    this->tmloop->smflag = 2;
    this->tmloop->vtxcoords->x1 = 0.375f;
    this->tmloop->vtxcoords->y1 = -0.9f;
    this->tmloop->vtxcoords->w = 0.15;
    this->tmloop->vtxcoords->h = 0.26f;
    this->tmloop->tooltiptitle = "Set MIDI for loop play toggle button ";
    this->tmloop->tooltip = "Leftclick to start waiting for a MIDI command that will trigger loop play toggle for this preset. ";
	this->tmspeed = new Boxx;
    this->tmspeed->smflag = 2;
	this->tmspeed->vtxcoords->x1 = -0.8f;
	this->tmspeed->vtxcoords->y1 = -0.5f;
	this->tmspeed->vtxcoords->w = 0.2f;
	this->tmspeed->vtxcoords->h = 1.0f;
	this->tmspeed->tooltiptitle = "Set MIDI for setting video play speed ";
	this->tmspeed->tooltip = "Leftclick to start waiting for a MIDI command that will set the play speed factor for this preset. ";
	this->tmspeedzero = new Boxx;
    this->tmspeedzero->smflag = 2;
	this->tmspeedzero->vtxcoords->x1 = -0.775f;
	this->tmspeedzero->vtxcoords->y1 = -0.1f;
	this->tmspeedzero->vtxcoords->w = 0.15f;
	this->tmspeedzero->vtxcoords->h = 0.2f;
	this->tmspeedzero->tooltiptitle = "Set MIDI for setting video play speed to one";
	this->tmspeedzero->tooltip = "Leftclick to start waiting for a MIDI command that will set the play speed factor back to one for this preset. ";
    this->tmopacity = new Boxx;
    this->tmopacity->smflag = 2;
    this->tmopacity->vtxcoords->x1 = 0.6f;
    this->tmopacity->vtxcoords->y1 = -0.5f;
    this->tmopacity->vtxcoords->w = 0.2f;
    this->tmopacity->vtxcoords->h = 1.0f;
    this->tmopacity->tooltiptitle = "Set MIDI for setting video opacity ";
    this->tmopacity->tooltip = "Leftclick to start waiting for a MIDI command that will set the opacity for this preset. ";
    this->tmcross = new Boxx;
    this->tmcross->smflag = 2;
    this->tmcross->vtxcoords->x1 = -0.25f;
    this->tmcross->vtxcoords->y1 = -0.5f;
    this->tmcross->vtxcoords->w = 0.5f;
    this->tmcross->vtxcoords->h = 0.15f;
    this->tmcross->tooltiptitle = "Set MIDI for crossfade ";
    this->tmcross->tooltip = "Leftclick to start waiting for a MIDI command that will set the crossfader.  This setting is the same for each set. ";
    this->tmfreeze = new Boxx;
    this->tmfreeze->smflag = 2;
    this->tmfreeze->vtxcoords->x1 = -0.1f;
    this->tmfreeze->vtxcoords->y1 = 0.1f;
    this->tmfreeze->vtxcoords->w = 0.2f;
    this->tmfreeze->vtxcoords->h = 0.2f;
    this->tmfreeze->tooltiptitle = "Set MIDI for scratch wheel freeze ";
    this->tmfreeze->tooltip = "Leftclick to start waiting for a MIDI command that will trigger scratch wheel freeze for this preset. ";
    this->tmscrinvert = new Boxx;
    this->tmscrinvert->smflag = 2;
    this->tmscrinvert->vtxcoords->x1 = 0.3f;
    this->tmscrinvert->vtxcoords->y1 = -0.05f;
    this->tmscrinvert->vtxcoords->w = 0.05f;
    this->tmscrinvert->vtxcoords->h = 0.08f;
    this->tmscrinvert->tooltiptitle = "Toggle inversion of scratch wheel direction ";
    this->tmscrinvert->tooltip = "Leftclick to toggle inversion of scratch wheel direction for this preset. ";
    this->tmscratch1 = new Boxx;
    this->tmscratch1->smflag = 2;
    this->tmscratch1->vtxcoords->x1 = -1.0f;
    this->tmscratch1->vtxcoords->y1 = -2.0f;
    this->tmscratch1->vtxcoords->w = 2.0f;
    this->tmscratch1->vtxcoords->h = 1.0f;
    this->tmscratch1->tooltiptitle = "Set MIDI for scratch wheel when scratchwheel is not touched";
    this->tmscratch1->tooltip = "Leftclick to start waiting for a MIDI command that will trigger scratching for this preset. ";
    this->tmscratch2 = new Boxx;
    this->tmscratch2->smflag = 2;
    this->tmscratch2->vtxcoords->x1 = -1.0f;
    this->tmscratch2->vtxcoords->y1 = -1.0f;
    this->tmscratch2->vtxcoords->w = 2.0f;
    this->tmscratch2->vtxcoords->h = 1.0f;
    this->tmscratch2->tooltiptitle = "Set MIDI for scratch wheel when scratchwheel is touched";
    this->tmscratch2->tooltip = "Leftclick to start waiting for a MIDI command that will trigger scratching for this preset. ";

    // wormgates allow switching between mix and bins screen
    // you can also drag content through them by dragging up to the screen edge
    // wormgate rectangle to the left
	this->wormgate1 = new Button(false);
	this->wormgate1->toggle = 1;
	this->wormgate1->box->vtxcoords->x1 = -1.0f;
	this->wormgate1->box->vtxcoords->y1 = -0.58f;
	this->wormgate1->box->vtxcoords->w = 0.0375f;
	this->wormgate1->box->vtxcoords->h = 0.6f;
	this->wormgate1->box->upvtxtoscr();
	this->wormgate1->box->tooltiptitle = "Screen switching wormgate ";
	this->wormgate1->box->tooltip = "Connects mixing screen and media bins screen.  Leftclick to switch screen.  Drag content inside white rectangle up to the very edge of the screen to travel to the other screen. ";
    // wormgate rectangle to the right
	this->wormgate2 = new Button(false);
	this->wormgate2->toggle = 1;
	this->wormgate2->box->vtxcoords->x1 = 1.0f - 0.0375f;
	this->wormgate2->box->vtxcoords->y1 = -0.58f;
	this->wormgate2->box->vtxcoords->w = 0.0375f;
	this->wormgate2->box->vtxcoords->h = 0.6f;
	this->wormgate2->box->upvtxtoscr();
	this->wormgate2->box->tooltiptitle = "Screen switching wormgate ";
	this->wormgate2->box->tooltip = "Connects mixing screen and media bins screen."
                                    "  Leftclick to switch screen.  Leftclick to switch screen.  Drag content inside white rectangle up to the very edge of the screen to travel to the other screen. ";
    
    // layer stack scrollbar
    this->boxbig = new Boxx;
    this->boxbefore = new Boxx;
    this->boxafter = new Boxx;
    this->boxlayer = new Boxx;
}

void Program::make_menu(std::string name, Menu *&menu, std::vector<std::string> &entries) {
    // create a menu structure from a string vector of menu entries
    bool found = false;
	for (int i = 0; i < mainprogram->menulist.size(); i++) {
		if (mainprogram->menulist[i]->name == name) {
			found = true;
			break;
		}
	}
	if (!found) {
        // if first time to create the menu: reserve a new Menu structure and name the menu
		menu = new Menu;
		mainprogram->menulist.push_back(menu);
		menu->name = name;
	}
    else {
        delete menu->box;
    }
	menu->entries = entries;
	Boxx *box = new Boxx;
	menu->box = box;
	menu->box->scrcoords->x1 = 0;
	menu->box->scrcoords->y1 = mainprogram->yvtxtoscr(0.075f);
	menu->box->scrcoords->w = mainprogram->xvtxtoscr(0.234f);
	menu->box->scrcoords->h = mainprogram->yvtxtoscr(0.075f);
	menu->box->upscrtovtx();
}

#ifdef POSIX
const char* Program::mime_to_tinyfds(std::string filters) {
    // get file type extension for tinyfiledialogs from mime type: Linux
	if (filters == "") return "";
	if (filters == "application/ewocvj2-layer") return "*.layer";
	if (filters == "application/ewocvj2-deck") return "*.deck";
	if (filters == "application/ewocvj2-mix") return "*.mix";
	if (filters == "application/ewocvj2-state") return "*.state";
	if (filters == "application/ewocvj2-project") return "*.ewocvj";
	if (filters == "application/ewocvj2-shelf") return "*.shelf";
	if (filters == "application/ewocvj2-bin") return "*.bin";
	return "";
}
#endif

#ifdef WINDOWS
LPCSTR Program::mime_to_wildcard(std::string filters) {
    // get file type wildcard from mime type: Windows
	if (filters == "") return "";
	if (filters == "application/ewocvj2-layer") return "EWOCvj2 layer file (.layer)\0*.layer\0";
	if (filters == "application/ewocvj2-deck") return "EWOCvj2 deck file (.deck)\0*.deck\0";
	if (filters == "application/ewocvj2-mix") return "EWOCvj2 mix file (.mix)\0*.mix\0";
	if (filters == "application/ewocvj2-state") return "EWOCvj2 state file (.state)\0*.state\0";
	if (filters == "application/ewocvj2-project") return "EWOCvj2 project file (.ewocvj)\0*.ewocvj\0";
	if (filters == "application/ewocvj2-shelf") return "EWOCvj2 shelf file (.shelf)\0*.shelf\0";
	if (filters == "application/ewocvj2-bin") return "EWOCvj2 bin file (.bin)\0*.bin\0";
	return "";
}

void Program::win_dialog(const char* title, LPCSTR filters, std::string defaultdir, bool open, bool multi) {
	// Windows file dialog implementation
	boost::replace_all(defaultdir, "/", "\\");
	std::filesystem::path p(defaultdir);
	std::string name;
	if (!std::filesystem::is_directory(p)) {
		name = basename(defaultdir);
		defaultdir = defaultdir.substr(0, defaultdir.length() - name.length() - 1);
	}
#ifdef WINDOWS
	OPENFILENAME ofn;
	char szFile[4096];
	if (name != "") {
		int pos = 0;
		do {
			szFile[pos] = name[pos];
			pos++;
		} while (pos < name.length());
		szFile[pos] = '\0';
	}
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFile = szFile;
	if (name == "") ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = filters;
	ofn.nFilterIndex = 2;
	ofn.lpstrTitle = title;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = defaultdir.c_str();
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	if (multi) ofn.Flags = ofn.Flags | OFN_ALLOWMULTISELECT | OFN_EXPLORER;
	bool ret;
    mainprogram->blocking = true;
	if (open) ret = GetOpenFileName(&ofn);
	else ret = GetSaveFileName(&ofn);
    mainprogram->blocking = false;
	if (strlen(ofn.lpstrFile) == 0 || ret == 0) {
		binsmain->openfilesbin = false;
		return;
	}
	char* wstr = ofn.lpstrFile;
	std::string directory(wstr);
	if (!multi) this->path = directory;
	else {
		if (wstr[directory.length() + 1] == 0) {
			// only one file in selection
			this->paths.push_back(directory);
		}
		else {
			wstr += (directory.length() + 1);
			while (*wstr) {
				std::string filename = wstr;
				this->paths.push_back(directory + "/" + filename);
				wstr += (filename.length() + 1);
			}
		}
	}
#endif
}
#endif

void Program::postponed_to_front(std::string title) {
    this->postponed_to_front_win(title, nullptr);
}

void Program::postponed_to_front_win(std::string title, SDL_Window *win) {
    if (win) {
        // Windows normal operation
        SDL_ShowWindow(win);
    }

#ifdef POSIX
    // trick to get window to front in Linux with a slight delay
    std::string command = "sleep 0.5 && wmctrl -F -a \"" + title + "\" -b add,above &";
    system(command.c_str());
#endif

}

void Program::get_inname(const char *title, std::string filters, std::string defaultdir) {
    // pop up a requester for selecting an input file
	bool as = mainprogram->autosave;
    this->autosave = false;
	#ifdef WINDOWS
	LPCSTR lpcfilters = this->mime_to_wildcard(filters);
	this->win_dialog(title, lpcfilters, defaultdir, true, false);
    #endif
    #ifdef POSIX
	char const *p;
    const char* fi[1];
    if (kdialogPresent()) {
        fi[0] = filters.c_str();
    }
    else {
        fi[0] = this->mime_to_tinyfds(filters);
    }
    mainprogram->blocking = true;

    std::string tt(title);
    std::thread tofront = std::thread{&Program::postponed_to_front, this, tt};
    tofront.detach();

    if (fi[0] == "") {
        p = tinyfd_openFileDialog(title, defaultdir.c_str(), 0, nullptr, nullptr, 0);
    }
    else {
        p = tinyfd_openFileDialog(title, defaultdir.c_str(), 1, fi, "", 0);
    }
    if (p) this->path = p;
    #endif
    this->autosave = as;
    mainprogram->blocking = false;
}

void Program::get_outname(const char *title, std::string filters, std::string defaultdir) {
    // pop up a requester for selecting an output file
	bool as = mainprogram->autosave;
    this->autosave = false;

	#ifdef WINDOWS
	LPCSTR lpcfilters = this->mime_to_wildcard(filters);
	this->win_dialog(title, lpcfilters, defaultdir, false, false);
	#endif
    #ifdef POSIX
    char const *p;
    const char* fi[1];
    if (kdialogPresent()) {
        fi[0] = filters.c_str();
    }
    else {
        fi[0] = this->mime_to_tinyfds(filters);
    }
    mainprogram->blocking = true;

    std::string tt(title);
    std::thread tofront = std::thread{&Program::postponed_to_front, this, tt};
    tofront.detach();

    if (fi[0] == "") {
        p = tinyfd_saveFileDialog(title, defaultdir.c_str(), 0, nullptr, nullptr);
    }
    else {
        p = tinyfd_saveFileDialog(title, defaultdir.c_str(), 1, fi, nullptr);
    }
    if (p) this->path = p;
    #endif
    this->autosave = as;
    mainprogram->blocking = false;
}

void Program::get_multinname(const char* title, std::string filters, std::string defaultdir) {
    // pop up a tinyfiledialogs requester for selecting multiple files
	bool as = this->autosave;
	this->autosave = false;
    const char *outpaths;
    if (exists(defaultdir)) {
        if (std::filesystem::is_directory(defaultdir)) {
            defaultdir += "/*";
        }
    }
    char const* const dd = (defaultdir == "") ? "" : defaultdir.c_str();
    mainprogram->blocking = true;

	#ifdef POSIX
    std::string tt(title);
    std::thread tofront = std::thread{&Program::postponed_to_front, this, tt};
    tofront.detach();
    #endif

    outpaths = tinyfd_openFileDialog(title, dd, 0, nullptr, nullptr, 1);
    mainprogram->blocking = false;
    if (outpaths == nullptr) {
        binsmain->openfilesbin = false;
        return;
    }
    std::string opaths(outpaths);
    std::string currstr = "";
    std::string charstr;
    for (int i = 0; i < opaths.length(); i++) {
        charstr = opaths[i];
        if (charstr == "|") {
            this->paths.push_back(currstr);
            currstr = "";
            continue;
        }
        currstr += charstr;
        if (i == opaths.length() - 1) this->paths.push_back(currstr);
    }
	if (mainprogram->paths.size()) {
		this->path = (char*)"ENTER";
		this->counting = 0;
	}
	this->autosave = as;
}

void Program::get_dir(const char* title, std::string defaultdir) {
    // pop up a requester for selecting a directory using OS functionality
	bool as = mainprogram->autosave;
	mainprogram->autosave = false;

#ifdef WINDOWS
	OleInitialize(NULL); 
	TCHAR* szDir = new TCHAR[MAX_PATH];
	BROWSEINFO bInfo;
	ZeroMemory(&bInfo, sizeof(bInfo));
	bInfo.hwndOwner = nullptr;
	bInfo.pidlRoot = nullptr;
	bInfo.pszDisplayName = szDir;
	bInfo.lpszTitle = title;
	bInfo.ulFlags = 0;
	bInfo.lpfn = nullptr;
	bInfo.lParam = 0;
	bInfo.iImage = 0;
	bInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_BROWSEINCLUDEFILES;

    mainprogram->blocking = true;
	LPITEMIDLIST lpItem = SHBrowseForFolder(&bInfo);
    mainprogram->blocking = false;
	if (lpItem != NULL)
	{
		SHGetPathFromIDList(lpItem, szDir);
		CoTaskMemFree(lpItem);
	}
	this->path = (char*)szDir;
    if (lpItem == NULL)
    {
        this->path = "";
    }
	OleUninitialize();
#endif
#ifdef POSIX
    char const* const dd = (defaultdir == "") ? "" : defaultdir.c_str();
    const char *dir;
    mainprogram->blocking = true;

    std::string tt(title);
    std::thread tofront = std::thread{&Program::postponed_to_front, this, tt};
    tofront.detach();

    dir = tinyfd_selectFolderDialog(title, dd);
    mainprogram->blocking = false;
	if (dir) this->path = dir;
	#endif
	mainprogram->autosave = as;
}



GUIString::~GUIString() {
    // kill a GUIString with its precomputed texture
    for (GLuint tex : this->texturevec) {
        glDeleteTextures(1, &tex);
    }
}


GLuint Program::get_tex(Layer *lay) {
    // get a texture from decompressed data from an ELEM_FILE or an ELEM_IMAGE
    GLuint ctex;
    glGenTextures(1, &ctex);
    glBindTexture(GL_TEXTURE_2D, ctex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    //glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    if (lay->type == ELEM_IMAGE) {
        // image in layer
        ilBindImage(lay->boundimage);
        ilActiveImage(lay->numf / 2);
        int w = ilGetInteger(IL_IMAGE_WIDTH);
        int h = ilGetInteger(IL_IMAGE_HEIGHT);

        ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);

        glBindTexture(GL_TEXTURE_2D, ctex);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, w, h);

        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE,
                        ilGetData());
    }
    else {
        if (lay->vidformat == 188 || lay->vidformat == 187) {
            // HAP video in layer
            if (lay->decresult->compression == 187) {
                glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, lay->decresult->width,
                                       lay->decresult->height, 0, lay->decresult->size, lay->decresult->data);
            } else if (lay->decresult->compression == 190) {
                glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, lay->decresult->width,
                                       lay->decresult->height, 0, lay->decresult->size, lay->decresult->data);
            }
         } else {
            // CPU video in layer
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, lay->decresult->width, lay->decresult->height, 0, GL_BGRA,
                         GL_UNSIGNED_BYTE, lay->decresult->data);
        }
    }

    GLuint tex = copy_tex(ctex, 192, 108);
    mainprogram->texintfmap[ctex] = GL_RGBA8;
    mainprogram->add_to_texpool(ctex);

    return tex;
}

bool Program::order_paths(bool dodeckmix) {
    // get thumbnail textures for files to be loaded, then display filenames
    // and textures and allow reordering them before inserting the files
    // the process goes in multiple stages to allow spreading the workload
    if (this->multistage == 0) {
        // initial setup
        this->pathtexes.clear();
        mainprogram->orderondisplay = true;

        while (this->paths.size()) {
            std::string str = this->paths.front();

            // determine file type and set up data fields
            std::string istring = "";
            std::ifstream rfile;
            rfile.open(str);
            safegetline(rfile, istring);
            rfile.close();

            if (isimage(str)) {
                Layer *lay = new Layer(true);
                lay->type = ELEM_IMAGE;
                this->getvideotexlayers.push_back(lay);
                this->getvideotexpaths.push_back(str);
            } else if (istring.find("EWOCvj LAYERFILE") != std::string::npos) {
                Layer *lay = new Layer(true);
                lay->type = ELEM_LAYER;
                this->getvideotexlayers.push_back(lay);
                this->getvideotexpaths.push_back(str);
            } else if (istring.find("EWOCvj DECKFILE") != std::string::npos) {
                Layer *lay = new Layer(true);
                if (dodeckmix) {
                    lay->type = ELEM_DECK;
                    this->getvideotexlayers.push_back(lay);
                    this->getvideotexpaths.push_back(str);
                } else {
                }
            } else if (istring.find("EWOCvj MIXFILE") != std::string::npos) {
                Layer *lay = new Layer(true);
                lay->type = ELEM_MIX;
                if (dodeckmix) {
                    lay->type = ELEM_MIX;
                    this->getvideotexlayers.push_back(lay);
                    this->getvideotexpaths.push_back(str);
                } else {
                }
            }
            else {
                Layer *lay = new Layer(true);
                lay->type = ELEM_FILE;
                this->getvideotexlayers.push_back(lay);
                this->getvideotexpaths.push_back(str);
            }
            this->paths.erase(this->paths.begin());
            return false;
        }
        this->paths = this->getvideotexpaths;
        multistage = 1;
        return false;
    }

    if (this->multistage == 1) {
        for (int i = 0; i < this->getvideotexlayers.size(); i++) {
            // start texture fetch threads
            Layer * lay = this->getvideotexlayers[i];
            std::string str = this->getvideotexpaths[i];
            switch (lay->type) {
                case ELEM_FILE: {
                    std::thread tex0(&get_videotex, lay, str);
                    tex0.detach();
                    break;
                }
                case ELEM_IMAGE: {
                    std::thread tex1(&get_imagetex, lay, str);
                    tex1.detach();
                    break;
                }
                case ELEM_LAYER: {
                    lay->keepeffbut->value = 0;
                    std::vector<Layer*> layers;
                    this->gettinglayertexlay = mainmix->add_layer(layers, 0);
                    this->gettinglayertexlay->keepeffbut->value = 0;
                    this->gettinglayertexlay->type = ELEM_LAYER;
                    this->gettinglayertex = true;
                    std::thread tex2(&get_layertex, lay, str);
                    tex2.detach();
                    break;
                }
                case ELEM_DECK: {
                    std::thread tex3(&get_deckmixtex, lay, str);
                    tex3.detach();
                    break;
                }
                case ELEM_MIX: {
                    std::thread tex4(&get_deckmixtex, lay, str);
                    tex4.detach();
                    break;
                }
            }
        }
        multistage = 2;
        return false;
    }

    if (this->multistage == 2) {
        // wait for threads and get the textures
        while (this->getvideotexlayers.size()) {
            std::string str = this->getvideotexpaths[0];
            Layer *lay = this->getvideotexlayers[0];

            GLuint tex;

            // wait for all threads to finish, meanwhile return to continue the videostreams
            if (!lay->processed) return false;
            lay->processed = false;

            if (lay->type == ELEM_DECK || lay->type == ELEM_MIX) {
                glGenTextures(1, &tex);
                glBindTexture(GL_TEXTURE_2D, tex);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

                open_thumb(this->result + "_" + std::to_string(this->resnum - 2) + ".file", tex);
            } else if (lay->type == ELEM_LAYER) {
                glBindTexture(GL_TEXTURE_2D, lay->texture);
                if (lay->vidformat == 188 || lay->vidformat == 187) {
                    if (lay->decresult->compression == 187) {
                        glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
                                               lay->decresult->width,
                                               lay->decresult->height, 0, lay->decresult->size,
                                               lay->decresult->data);
                    } else if (lay->decresult->compression == 190) {
                        glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,
                                               lay->decresult->width,
                                               lay->decresult->height, 0, lay->decresult->size,
                                               lay->decresult->data);
                    }
                } else {
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, lay->decresult->width, lay->decresult->height, 0,
                                 GL_BGRA,
                                 GL_UNSIGNED_BYTE, lay->decresult->data);
                }
                GLuint butex = lay->fbotex;
                lay->fbotex = copy_tex(lay->texture);
                glDeleteTextures(1, &butex);
                for (int k = 0; k < lay->effects[0].size(); k++) {
                    lay->effects[0][k]->node->calc = true;
                    lay->effects[0][k]->node->walked = false;
                }
                lay->vidopen = false;
                mainprogram->directmode = true;
                onestepfrom(0, lay->node, nullptr, -1, -1);
                mainprogram->directmode = false;
                if (lay->effects[0].size()) {
                    tex = copy_tex(lay->effects[0][lay->effects[0].size() - 1]->fbotex,
                                   binsmain->elemboxes[0]->scrcoords->w, binsmain->elemboxes[0]->scrcoords->h);
                } else {
                    tex = copy_tex(lay->fbotex, binsmain->elemboxes[0]->scrcoords->w,
                                   binsmain->elemboxes[0]->scrcoords->h);
                }
                this->gettinglayertexlay->close();
                this->gettinglayertex = false;
            } else {
                tex = this->get_tex(lay);
            }

            glBindTexture(GL_TEXTURE_2D, tex);

            GLuint butex = tex;
            tex = copy_tex(tex, 192, 108);
            glDeleteTextures(1, &butex);
            this->pathtexes.push_back(tex);
            render_text(str, white, 2.0f, 2.0f, 0.00045f, 0.00075f); // init text string, to avoid slowdown later
            this->pathtstrs.push_back(str);

            lay->filename = "";   // to avoid adding the lay->texture to the texpool
            lay->close();

            this->getvideotexlayers.erase(this->getvideotexlayers.begin());
            return false;
        }

        mainprogram->getvideotexlayers.clear();
        mainprogram->getvideotexpaths.clear();
        mainprogram->errlays.clear();
        mainprogram->err = false;

        for (int j = 0; j < this->paths.size(); j++) {
            this->pathboxes.push_back(new Boxx);
            this->pathboxes[j]->vtxcoords->x1 = -0.4f;
            this->pathboxes[j]->vtxcoords->y1 = 0.8f - j * 0.1f;
            this->pathboxes[j]->vtxcoords->w = 0.8f;
            this->pathboxes[j]->vtxcoords->h = 0.1f;
            this->pathboxes[j]->upvtxtoscr();
            this->pathboxes[j]->tooltiptitle = "Order files to be opened ";
            this->pathboxes[j]->tooltip = "Leftmouse drag the files in the list to set the order in which they will be loaded.  Use arrows/mousewheel to scroll list when its bigger then the screen.  Click APPLY ORDER to continue. ";
        }
        this->ordertime = 0.0f;

        this->multistage = 3;
    }

    if (this->multistage == 3) {
        // then do interactive ordering
        if (this->paths.size() != 1) {
            bool cont = this->do_order_paths();
            if (!cont) return false;
        }
        this->multistage = 4;
    }
    if (this->multistage == 4) {
        // then cleanup
        for (int j = 0; j < this->pathboxes.size(); j++) {
            delete this->pathboxes[j];
        }
        this->pathboxes.clear();
        for (std::string tstr : this->pathtstrs) {
            this->delete_text(tstr);
        }
        this->pathtstrs.clear();

        this->pathscount = 0;
        this->orderondisplay = false;
        this->multistage = 5;
    }

    return true;
}
	
bool Program::do_order_paths() {
    mainprogram->directmode = false;
    mainprogram->frontbatch = false;
    SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
    glViewport(0, 0, glob->w, glob->h);
	if (this->paths.size() == 1) return true;
	// show interactive list with draggable elements to allow element ordering of mainprogram->paths, result of get_multinname
	int limit = this->paths.size();
	if (limit > 17) limit = 17;

	// mousewheel scroll
	this->pathscroll -= mainprogram->mousewheel;
	if (this->dragstr != "" && mainmix->time - this->ordertime > 0.1f) {
		// border scroll when dragging
		if (mainprogram->my < yvtxtoscr(0.1f)) {
		    this->ordertime = mainmix->time;
		    this->pathscroll--;
		}
		if (mainprogram->my > yvtxtoscr(1.9f)) {
            this->ordertime = mainmix->time;
		    this->pathscroll++;
		}
	}
	if (this->pathscroll < 0) this->pathscroll = 0;
	if (this->paths.size() > 18 && this->paths.size() - this->pathscroll < 18) this->pathscroll = this->paths.size() - 17;

	mainprogram->frontbatch = true;
	// draw and handle orderlist scrollboxes
	this->pathscroll = mainprogram->handle_scrollboxes(*this->orderscrollup, *this->orderscrolldown, this->paths.size(), this->pathscroll, 17);
	bool indragbox = false;
	for (int j = this->pathscroll; j < this->pathscroll + limit; j++) {
		Boxx* box = this->pathboxes[j];
		box->vtxcoords->y1 = 0.8f - (j - this->pathscroll) * 0.1f;
		box->upvtxtoscr();
		draw_box(white, black, box, -1);
		draw_box(white, black, 0.3f, box->vtxcoords->y1, 0.1f, 0.1f, this->pathtexes[j]);
        if (mainprogram->openfilesshelf && j > 15 - mainprogram->shelffileselem) {
            render_text(this->paths[j], grey, -0.4f + 0.015f, box->vtxcoords->y1 + 0.075f - 0.045f, 0.00045f,
                        0.00075f);
        }
        else {
            render_text(this->paths[j], white, -0.4f + 0.015f, box->vtxcoords->y1 + 0.075f - 0.045f, 0.00045f,
                        0.00075f);
        }
        this->pathtstrs.push_back(this->paths[j]);
        // prepare element dragging
		if (box->in()) {
			std::string path = this->paths[j];
			if (this->dragstr == "") {
				if (mainprogram->orderleftmousedown && !this->dragpathsense) {
					this->dragpathsense = true;
					dragbox = box;
					this->dragpathpos = j;
					mainprogram->orderleftmousedown = false;
				}
				if (j == this->dragpathpos) {
					indragbox = true;
				}
			}
		}
	}

	if (!indragbox && this->dragpathsense) {
		this->dragstr = this->paths[this->dragpathpos];
		this->dragpathsense = false;
	}

	// confirm box

    Boxx *applybox = new Boxx;
	applybox->vtxcoords->x1 = this->pathboxes.back()->vtxcoords->x1;
	applybox->vtxcoords->y1 = 0.8f - limit * 0.1f;
	applybox->vtxcoords->w = this->pathboxes.back()->vtxcoords->w;
	applybox->vtxcoords->h = this->pathboxes.back()->vtxcoords->h;
	applybox->upvtxtoscr();
	draw_box(white, black, applybox, -1);
	render_text("APPLY ORDER  (rightmouse cancels)", white, -0.4f + 0.015f, 0.8f - limit * 0.1f + 0.075f - 0.045f, 0.00045f, 0.00075f);
	if (applybox->in() && this->dragstr == "") {
        if (mainprogram->orderleftmouse) {
            this->pathscroll = 0;
            mainprogram->frontbatch = false;
            return true;
        }
	}
    if (mainprogram->orderrightmouse) {
        delete applybox;
        this->pathscroll = 0;
        this->paths.clear();
        mainprogram->frontbatch = false;
        return true;
    }
    delete applybox;

	// do drag
	if (this->dragstr != "") {
		int pos;
		for (int j = this->pathscroll; j < this->paths.size() + 1; j++) {
			// calculate dragged element temporary position pos when mouse between
			//limits under and upper
			int under1, under2, upper;
			if (j == this->pathscroll) {
				// mouse above first bin
				under2 = 0;
				under1 = this->pathboxes[this->pathscroll]->scrcoords->y1 - this->pathboxes[this->pathscroll]->scrcoords->h * 0.5f;
			}
			else {
				under1 = this->pathboxes[j - 1]->scrcoords->y1 + this->pathboxes[j - 1]->scrcoords->h * 0.5f;
				under2 = under1;
			}
			if (j == this->paths.size()) {
				// mouse under last bin
				upper = glob->h;
			}
			else upper = this->pathboxes[j]->scrcoords->y1 + this->pathboxes[j]->scrcoords->h * 0.5f;
			if (mainprogram->my > under2 && mainprogram->my < upper) {
				draw_box(white, darkgreygreen, this->pathboxes[this->dragpathpos]->vtxcoords->x1, 1.0f - mainprogram->yscrtovtx(under1), this->pathboxes[this->dragpathpos]->vtxcoords->w, this->pathboxes[this->dragpathpos]->vtxcoords->h, -1);
				draw_box(white, darkgreygreen, 0.3f, 1.0f - mainprogram->yscrtovtx(under1), 0.1f, 0.1f, this->pathtexes[this->dragpathpos]);
				render_text(this->dragstr, white, -0.4f + 0.015f, 1.0f - mainprogram->yscrtovtx(under1) + 0.075f - 0.045f, 0.00045f, 0.00075f);
				pos = j;
				break;
			}
		}
		if (mainprogram->orderleftmouse) {
			// do file path drag
			if (this->dragpathpos < pos) {
				std::rotate(this->paths.begin() + this->dragpathpos, this->paths.begin() + this->dragpathpos + 1, this->paths.begin() + pos);
				std::rotate(this->pathboxes.begin() + this->dragpathpos, this->pathboxes.begin() + this->dragpathpos + 1, this->pathboxes.begin() + pos);
				std::rotate(this->pathtexes.begin() + this->dragpathpos, this->pathtexes.begin() + this->dragpathpos + 1, this->pathtexes.begin() + pos);
			}
			else {
				std::rotate(this->paths.begin() + pos, this->paths.begin() + this->dragpathpos, this->paths.begin() + this->dragpathpos + 1);
				std::rotate(this->pathboxes.begin() + pos, this->pathboxes.begin() + this->dragpathpos, this->pathboxes.begin() + this->dragpathpos + 1);
				std::rotate(this->pathtexes.begin() + pos, this->pathtexes.begin() + this->dragpathpos, this->pathtexes.begin() + this->dragpathpos + 1);
			}
			this->dragstr = "";
			mainprogram->orderleftmouse = false;
		}
	}

	mainprogram->frontbatch = false;

	return false;

}


void Program::handle_wormgate(bool gate) {
    // draw and handle wormgates
    // wormgates allow switching between mix and bins screen
    // you can also drag content through them by dragging up to the screen edge
	Boxx* box;
	if (gate == 0) box = mainprogram->wormgate1->box;
	else box = mainprogram->wormgate2->box;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDrawBuffer(GL_BACK_LEFT);
	if (gate == 0) {
		register_triangle_draw(lightgrey, lightgrey, -1.0 + box->vtxcoords->w, box->vtxcoords->y1 + box->vtxcoords->h / 4.0f - 0.15f, box->vtxcoords->h / 4.0f, box->vtxcoords->h / 2.0f, LEFT, OPEN, true);
        mainprogram->directmode = true;
		if (mainprogram->binsscreen) render_text("MIX", lightgrey, -0.9f, -0.29f, 0.0006f, 0.001f);
		else render_text("BINS", lightgrey, -0.9f, -0.44f, 0.0006f, 0.001f);
        mainprogram->directmode = false;
	}
	else {
		if (mainprogram->binsscreen) {
			register_triangle_draw(lightgrey, lightgrey, 1.0f - box->vtxcoords->w - box->vtxcoords->h / 4.0f * 0.866f, box->vtxcoords->y1 + box->vtxcoords->h / 4.0f + 0.15f, box->vtxcoords->h / 4.0f, box->vtxcoords->h / 2.0f, RIGHT, OPEN, true);
            mainprogram->directmode = true;
			render_text("MIX", lightgrey, 0.86f, -0.14f, 0.0006f, 0.001f);
            mainprogram->directmode = false;
		}
		else {
			register_triangle_draw(lightgrey, lightgrey, 1.0f - box->vtxcoords->w - box->vtxcoords->h / 4.0f * 0.866f, box->vtxcoords->y1 + box->vtxcoords->h / 4.0f - 0.15f, box->vtxcoords->h / 4.0f, box->vtxcoords->h / 2.0f, RIGHT, OPEN, true);
            mainprogram->directmode = true;
			render_text("BINS", lightgrey, 0.86f, -0.44f, 0.0006f, 0.001f);
            mainprogram->directmode = false;
		}
	}

	if (mainprogram->binsscreen) {
		box->vtxcoords->y1 = -1.0f;
		box->vtxcoords->h = 2.0f;
		box->upvtxtoscr();
	}

	//draw and handle BINS wormgate
	if (mainprogram->fullscreen == -1) {
		if (box->in()) {
			draw_box(lightgrey, lightblue, box, -1);
			//mainprogram->tooltipbox = mainprogram->wormgate1->box;
			if (!mainprogram->menuondisplay) {
				if (mainprogram->leftmouse) {
					mainprogram->binsscreen = !mainprogram->binsscreen;
                    mainprogram->recundo = true;
                    if (mainprogram->binsscreen) {
                        for (int i = 0; i < 4; i++ ) {
                            for (Layer *lay : mainmix->layers[i]) {
                                if (lay->clips.size() > 1) {
                                    lay->compswitched = true;
                                    GLuint tex = copy_tex(lay->node->vidbox->tex, 192, 108);
                                    save_thumb(lay->currcliptexpath, tex);
                                }
                            }
                        }
                    }
				}
				if (mainprogram->menuactivation) {
					mainprogram->parammenu1->state = 2;
					mainmix->learnparam = nullptr;
					mainmix->learnbutton = mainprogram->wormgate1;
					mainprogram->menuactivation = false;
				}
			}
			if (mainprogram->dragbinel) {
				//dragging something inside wormgate
				if (!mainprogram->inwormgate && !mainprogram->menuondisplay) {
                    //if (mainprogram->mx == gate * (glob->w - 1)) {
                    if (!mainprogram->binsscreen) {
                        set_queueing(false);
                    }
                    mainprogram->binsscreen = !mainprogram->binsscreen;
                    mainprogram->inwormgate = true;
                    if (mainprogram->binsscreen) {
                        for (int i = 0; i < 4; i++ ) {
                            for (Layer *lay : mainmix->layers[i]) {
                                if (lay->clips.size() > 1) {
                                    lay->compswitched = true;
                                    GLuint tex = copy_tex(lay->node->vidbox->tex, 192, 108);
                                    save_thumb(lay->currcliptexpath, tex);
                                }
                            }
                        }
                    }
                }
			}
		}
		else {
			draw_box(lightgrey, lightgrey, box, -1);
		}
	}

	box->vtxcoords->y1 = -0.58f;
	box->vtxcoords->h = 0.6f;
	box->upvtxtoscr();
}


void Program::set_ow3oh3() {
    // set ow3 and oh3 mainprogram values: ow3 is always <= 640
    // these smaller width and height values are used for preview modus textures
    // because they are not displayed, preview modus textures can be less highly defined
    // performance and memory gain!
    mainprogram->ow = mainprogram->project->ow;
    mainprogram->oh = mainprogram->project->oh;

	float wmeas = 640.0f;
	if (mainprogram->ow < wmeas) wmeas = mainprogram->ow;
	float hmeas = 640.0f;
	if (mainprogram->oh < hmeas) hmeas = mainprogram->oh;
	if (mainprogram->ow > mainprogram->oh) {
		mainprogram->ow3 = wmeas;
		mainprogram->oh3 = wmeas * mainprogram->oh / mainprogram->ow;
	}
	else {
		mainprogram->oh3 = hmeas;
		mainprogram->ow3 = hmeas * mainprogram->ow / mainprogram->oh;
	}
}

void Program::handle_changed_owoh() {
    // when the project width or height is changed in the preferences
    // we need to renew some texture definitions
	if (this->ow != this->oldow || this->oh != this->oldoh) {
		for (int i = 0; i < this->nodesmain->pages.size(); i++) {
			for (int j = 0; j < this->nodesmain->pages[i]->nodes.size(); j++) {
				Node* node = this->nodesmain->pages[i]->nodes[j];
				node->renew_texes(this->ow3, this->oh3);
			}
			for (int j = 0; j < this->nodesmain->pages[i]->nodescomp.size(); j++) {
				Node* node = this->nodesmain->pages[i]->nodescomp[j];
				node->renew_texes(this->ow, this->oh);
			}
		}
		for (int j = 0; j < this->nodesmain->mixnodes[0].size(); j++) {
			Node* node = this->nodesmain->mixnodes[0][j];
			node->renew_texes(this->ow3, this->oh3);
		}
		for (int j = 0; j < this->nodesmain->mixnodes[1].size(); j++) {
			Node* node = this->nodesmain->mixnodes[1][j];
			node->renew_texes(this->ow, this->oh);
		}

		GLuint tex;
		tex = set_texes(this->fbotex[0], &this->frbuf[0], this->ow3, this->oh3);
		this->fbotex[0] = tex;
		tex = set_texes(this->fbotex[1], &this->frbuf[1], this->ow3, this->oh3);
		this->fbotex[1] = tex;
		tex = set_texes(this->fbotex[2], &this->frbuf[2], this->ow, this->oh);
		this->fbotex[2] = tex;
		tex = set_texes(this->fbotex[3], &this->frbuf[3], this->ow, this->oh);
		this->fbotex[3] = tex;

#ifdef POSIX
        for (auto entry : v4l2lbtexmap) {
		    // set v4l2 loopback devices parameters
            int output = open(entry.first.c_str(), O_RDWR);
            if (output < 0) {
                std::cerr << "ERROR: could not open output device!\n" <<
                          strerror(errno);
                break;
            }

            // acquire video format from device
            struct v4l2_format vid_format;
            memset(&vid_format, 0, sizeof(vid_format));
            vid_format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

            if (ioctl(output, VIDIOC_G_FMT, &vid_format) < 0) {
                std::cerr << "ERROR: unable to get video format!\n" <<
                          strerror(errno);
                break;
            }

            // configure desired video format on device
            size_t framesize;
            if (mainprogram->prevmodus) {
                framesize = mainprogram->ow3 * mainprogram->oh3 * 4;
                vid_format.fmt.pix.width = mainprogram->ow3;
                vid_format.fmt.pix.height = mainprogram->oh3;
            }
            else {
                framesize = mainprogram->ow * mainprogram->oh * 4;
                vid_format.fmt.pix.width = mainprogram->ow;
                vid_format.fmt.pix.height = mainprogram->oh;
            }
            vid_format.fmt.pix.pixelformat = V4L2_PIX_FMT_BGR32;
            vid_format.fmt.pix.sizeimage = framesize;
            vid_format.fmt.pix.field = V4L2_FIELD_NONE;

            if (ioctl(output, VIDIOC_S_FMT, &vid_format) < 0) {
                std::cerr << "ERROR: unable to set video format!\n" <<
                          strerror(errno);
                break;
            }
        }
#endif

		this->oldow = this->ow;
		this->oldoh = this->oh;
	}
}


void Program::handle_fullscreen() {
    // fullscreen mode allows inspecting output closeup when there is no video visualisation device
    mainprogram->frontbatch = true;

    loopstation->handle();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDrawBuffer(GL_BACK_LEFT);

    GLfloat vcoords1[8];
	GLfloat* p = vcoords1;
	*p++ = -1.0f; *p++ = 1.0f;
	*p++ = -1.0f; *p++ = -1.0f;
	*p++ = 1.0f; *p++ = 1.0f;
	*p++ = 1.0f; *p++ = -1.0f;
	GLfloat tcoords[] = { 0.0f, 0.0f,
						0.0f, 1.0f,
						1.0f, 0.0f,
						1.0f, 1.0f };
	GLuint vbuf, tbuf, vao;

	glGenBuffers(1, &vbuf);
	glBindBuffer(GL_ARRAY_BUFFER, vbuf);
	glBufferData(GL_ARRAY_BUFFER, 32, vcoords1, GL_DYNAMIC_DRAW);
	glGenBuffers(1, &tbuf);
	glBindBuffer(GL_ARRAY_BUFFER, tbuf);
	glBufferData(GL_ARRAY_BUFFER, 32, tcoords, GL_DYNAMIC_DRAW);
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbuf);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
	glBindBuffer(GL_ARRAY_BUFFER, tbuf);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8, nullptr);

	MixNode* node;
	if (this->fullscreen == this->nodesmain->mixnodes[0].size()) node = (MixNode*)this->nodesmain->mixnodes[1][this->fullscreen - 1];
	else node = (MixNode*)this->nodesmain->mixnodes[1][this->fullscreen];
	if (this->prevmodus) {
        if (this->fullscreen == 0) {
            node = (MixNode *) this->nodesmain->mixnodes[0][0];
        }
        else if (this->fullscreen == 1) {
            node = (MixNode *) this->nodesmain->mixnodes[0][1];
        }
        else if (this->fullscreen == 2) {
            node = (MixNode *) this->nodesmain->mixnodes[0][2];
        }
        else if (this->fullscreen == 3) {
            node = (MixNode *) this->nodesmain->mixnodes[1][2];
        }
    }
	GLfloat cf = glGetUniformLocation(this->ShaderProgram, "cf");
	GLint wipe = glGetUniformLocation(this->ShaderProgram, "wipe");
	GLint mixmode = glGetUniformLocation(this->ShaderProgram, "mixmode");
	if (mainmix->wipe[this->fullscreen == 2] > -1) {
		if (this->fullscreen == 2) glUniform1f(cf, mainmix->crossfadecomp->value);
		else glUniform1f(cf, mainmix->crossfade->value);
		glUniform1i(wipe, 1);
		glUniform1i(mixmode, 18);
	}
	draw_box(nullptr, nullptr, -1.0f, -1.0f, 2.0f, 2.0f, node->mixtex);
	glUniform1i(wipe, 0);
	glUniform1i(mixmode, 0);
	if (this->doubleleftmouse) {
        this->fullscreen = -1;
    }
	if (this->menuactivation) {
		this->fullscreenmenu->state = 2;
		this->menuondisplay = true;
		this->menuactivation = false;
	}

	// Draw and handle fullscreenmenu
	int k = mainprogram->handle_menu(this->fullscreenmenu);
	if (k == 0) {
		this->fullscreen = -1;
	}
	if (this->menuchosen) {
		this->menuchosen = false;
		this->menuactivation = 0;
		this->menuresults.clear();
        mainprogram->recundo = true;
	}

    glDeleteBuffers(1, &vbuf);
    glDeleteBuffers(1, &tbuf);
    glDeleteVertexArrays(1, &vao);

    mainprogram->frontbatch = false;
}


float Program::xscrtovtx(float scrcoord) {
    // convert X screen coordinate to X vertex coordinate
	return (scrcoord * 2.0 / (float)glob->w);
}

float Program::yscrtovtx(float scrcoord) {
    // convert Y screen coordinate to Y vertex coordinate
	return (scrcoord * 2.0 / (float)glob->h);
}

float Program::xvtxtoscr(float vtxcoord) {
    // convert X vertex coordinate to X screen coordinate
	return (vtxcoord * (float)glob->w / 2.0);
}

float Program::yvtxtoscr(float vtxcoord) {
    // convert Y vertex coordinate to Y screen coordinate
	return (vtxcoord * (float)glob->h / 2.0);
}



int Program::handle_scrollboxes(Boxx &upperbox, Boxx &lowerbox, int numlines, int scrollpos, int scrlines) {
    int ret = this->handle_scrollboxes(upperbox, lowerbox, numlines, scrollpos, scrlines, mainprogram->mx,
                                    mainprogram->my);
    return ret;
}

int Program::handle_scrollboxes(Boxx &upperbox, Boxx &lowerbox, int numlines, int scrollpos, int scrlines, int mx, int
my) {
	// general code for scrollbuttons of scrollable lists
	if (scrollpos > 0) {
		if (upperbox.in(mx, my)) {
			// scroll up
			upperbox.acolor[0] = 0.5f;
			upperbox.acolor[1] = 0.5f;
			upperbox.acolor[2] = 1.0f;
			upperbox.acolor[3] = 1.0f;
			if (mainprogram->leftmouse || mainprogram->orderleftmouse) {
				scrollpos--;
			}
		}
		else {
			upperbox.acolor[0] = 0.0f;
			upperbox.acolor[1] = 0.0f;
			upperbox.acolor[2] = 0.0f;
			upperbox.acolor[3] = 1.0f;
		}
		draw_box(&upperbox, -1);
        if (!mainprogram->insmall) {
            register_triangle_draw(white, white, upperbox.vtxcoords->x1 + (lowerbox.vtxcoords->w / 0.075f) * 0.0111f, upperbox.vtxcoords->y1 + (lowerbox.vtxcoords->w / 0.075f) * (0.0624f - 0.045f), 0.0165f, 0.0312f, DOWN, CLOSED);
        }
        else {
            register_triangle_draw(white, white, upperbox.vtxcoords->x1 + (lowerbox.vtxcoords->w / 0.075f) * 0.0111f, upperbox.vtxcoords->y1 + (lowerbox.vtxcoords->w / 0.075f) * (0.0624f - 0.045f),
            0.033f, 0.0624f, DOWN, CLOSED, true);
         }
	}
	if (numlines - scrollpos > scrlines) {
        if (lowerbox.in(mx, my)) {
			// scroll down
			lowerbox.acolor[0] = 0.5f;
			lowerbox.acolor[1] = 0.5f;
			lowerbox.acolor[2] = 1.0f;
			lowerbox.acolor[3] = 1.0f;
			if (mainprogram->leftmouse || mainprogram->orderleftmouse) {
				scrollpos++;
			}
		}
		else {
			lowerbox.acolor[0] = 0.0f;
			lowerbox.acolor[1] = 0.0f;
			lowerbox.acolor[2] = 0.0f;
			lowerbox.acolor[3] = 1.0f;
		}
		draw_box(&lowerbox, -1);
        if (!mainprogram->insmall) {
		register_triangle_draw(white, white, lowerbox.vtxcoords->x1 + (lowerbox.vtxcoords->w / 0.075f) * 0.0111f, lowerbox.vtxcoords->y1 + (lowerbox.vtxcoords->w / 0.075f) * (0.0624f - 0.045f), 0.0165f, 0.0312f, UP, CLOSED);
        }
        else {
            register_triangle_draw(white, white, lowerbox.vtxcoords->x1 + (lowerbox.vtxcoords->w / 0.075f) * 0.0111f, lowerbox.vtxcoords->y1 + (lowerbox.vtxcoords->w / 0.075f) * (0.0624f - 0.045f),
         0.033f, 0.0624f, UP, CLOSED, true);
        }
	}
	if (numlines > scrlines && scrollpos + scrlines > numlines) scrollpos = numlines - scrlines;
	return scrollpos;
}

void Program::layerstack_scrollbar_handle() {
	//Handle layer scrollbar
	bool inbox = false;
	bool comp = !mainprogram->prevmodus;
	for (int i = 0; i < 2; i++) {
        std::vector<Layer *> &lvec = choose_layers(i);
        this->boxlayer->vtxcoords->y1 = 1.0f - mainprogram->layh - 0.045f;
        this->boxlayer->vtxcoords->w = 0.031f;
        this->boxlayer->vtxcoords->h = 0.04f;
        int size = lvec.size() + 1;
        if (size < 3) size = 3;
        float slidex = 0.0f;
        if (mainmix->scrollon == i + 1) {
            slidex = (mainmix->scrollon != 0) * mainprogram->xscrtovtx(mainprogram->mx - mainmix->scrollmx);
        }
        this->boxbig->vtxcoords->x1 = -1.0f + mainprogram->numw + i +
                             mainmix->scenes[i][mainmix->currscene[i]]->scrollpos * (mainprogram->layw * 3.0f / size) +
                             slidex;
        if (this->boxbig->vtxcoords->x1 < -1.0f + mainprogram->numw + i) {
            this->boxbig->vtxcoords->x1 = -1.0f + mainprogram->numw + i;
            slidex = 0.0f;
        }
        if (this->boxbig->vtxcoords->x1 >
            -1.0f + mainprogram->numw + i + (lvec.size() - 2) * (mainprogram->layw * 3.0f / size)) {
            this->boxbig->vtxcoords->x1 = -1.0f + mainprogram->numw + i + (lvec.size() - 2) * (mainprogram->layw * 3.0f / size);
            slidex = 0.0f;
        }
        this->boxbig->vtxcoords->y1 = 1.0f - mainprogram->layh - 0.05f;
        this->boxbig->vtxcoords->w = (3.0f / size) * mainprogram->layw * 3.0f;
        this->boxbig->vtxcoords->h = 0.05f;
        this->boxbig->upvtxtoscr();
        this->boxbefore->vtxcoords->x1 = -1.0f + mainprogram->numw + i;
        this->boxbefore->vtxcoords->y1 = 1.0f - mainprogram->layh - 0.05f;
        this->boxbefore->vtxcoords->w = (3.0f / size) * mainprogram->layw * mainmix->scenes[i][mainmix->currscene[i]]->scrollpos;
        this->boxbefore->vtxcoords->h = 0.05f;
        this->boxbefore->upvtxtoscr();
        this->boxafter->vtxcoords->x1 = -1.0f + mainprogram->numw + i + (mainmix->scenes[i][mainmix->currscene[i]]->scrollpos + 3) *
                                                                  (mainprogram->layw * 3.0f / size);
        this->boxafter->vtxcoords->y1 = 1.0f - mainprogram->layh - 0.05f;
        this->boxafter->vtxcoords->w = (3.0f / size) * mainprogram->layw * (size - 3);
        this->boxafter->vtxcoords->h = 0.05f;
        this->boxafter->upvtxtoscr();
        bool inbox = false;
        if (this->boxbig->in()) {
            inbox = true;
            if (mainprogram->leftmousedown && mainmix->scrollon == 0 && !mainprogram->menuondisplay) {
                mainmix->scrollon = i + 1;
                mainmix->scrollmx = mainprogram->mx;
                mainprogram->leftmousedown = false;
            }
        }
        this->boxbig->vtxcoords->w = 3.0f * mainprogram->layw / size;
        if (mainmix->scrollon == i + 1) {
            if (mainprogram->lmover) {
                mainmix->scrollon = 0;
            } else {
                if ((mainprogram->mx - mainmix->scrollmx) > mainprogram->xvtxtoscr(this->boxbig->vtxcoords->w)) {
                    if (mainmix->scenes[i][mainmix->currscene[i]]->scrollpos < size - 3) {
                        mainmix->scenes[i][mainmix->currscene[i]]->scrollpos++;
                        mainmix->scrollmx += mainprogram->xvtxtoscr(this->boxbig->vtxcoords->w);
                    }
                } else if ((mainprogram->mx - mainmix->scrollmx) < -mainprogram->xvtxtoscr(this->boxbig->vtxcoords->w)) {
                    if (mainmix->scenes[i][mainmix->currscene[i]]->scrollpos > 0) {
                        mainmix->scenes[i][mainmix->currscene[i]]->scrollpos--;
                        mainmix->scrollmx -= mainprogram->xvtxtoscr(this->boxbig->vtxcoords->w);
                    }
                }
            }
        }
        this->boxbig->vtxcoords->x1 = -1.0f + mainprogram->numw + i + slidex;
        float remw = this->boxbig->vtxcoords->w;
        for (int j = 0; j < size; j++) {
            //draw scrollbar numbers+divisions+blocks
            if (j == 0) {
                if (slidex < 0.0f) this->boxbig->vtxcoords->x1 -= slidex;
            }
            if (j == size - 1) {
                if (slidex > 0.0f) this->boxbig->vtxcoords->w -= slidex;
            }
            if (j >= mainmix->scenes[i][mainmix->currscene[i]]->scrollpos &&
                j < mainmix->scenes[i][mainmix->currscene[i]]->scrollpos + 3) {
                if (inbox) draw_box(lightgrey, lightblue, this->boxbig, -1);
                else draw_box(lightgrey, grey, this->boxbig, -1);
            } else draw_box(lightgrey, black, this->boxbig, -1);
            // this->boxlayer: small coloured boxes(default grey) signalling there's a loopstation parameter in this layer
            this->boxlayer->vtxcoords->x1 = this->boxbig->vtxcoords->x1 + 0.031f;
            if (j < lvec.size()) {
                if (j >= mainmix->scenes[i][mainmix->currscene[i]]->scrollpos &&
                    j < mainmix->scenes[i][mainmix->currscene[i]]->scrollpos + 3) {
                    const int lpstcsz = lvec[j]->lpstcolors.size();
                    int sz = std::min(lpstcsz, 6);
                    for(auto col : lvec[j]->lpstcolors) {
                        float ac[4] = {col[0], col[1], col[2], col[3]};
                        draw_box(nullptr, ac, this->boxlayer, -1);
                        this->boxlayer->vtxcoords->x1 += this->boxlayer->vtxcoords->w;
                    }
                }
            }

            if (j == 0) {
                if (slidex < 0.0f) this->boxbig->vtxcoords->x1 += slidex;
            }
            render_text(std::to_string(j + 1), white, this->boxbig->vtxcoords->x1 + 0.0078f - slidex,
                        this->boxbig->vtxcoords->y1 + 0.0078f, 0.0006, 0.001);
            const int s = lvec.size() - mainmix->scenes[i][mainmix->currscene[i]]->scrollpos;
            if (mainprogram->dragbinel) {
                if (this->boxbig->in()) {
                    if (mainprogram->lmover) {
                        if (j < lvec.size()) {
                            mainprogram->draginscrollbarlay = lvec[j];
                        } else {
                            mainprogram->draginscrollbarlay = mainmix->add_layer(lvec, j);
                        }
                    }
                }
            }
            this->boxbig->vtxcoords->x1 += this->boxbig->vtxcoords->w;
            this->boxbig->upvtxtoscr();
        }
        if (this->boxbefore->in()) {
            //mouse in empty scrollbar part before the lightgrey visible layers part
            if (mainprogram->dragbinel) {
                if (mainmix->scrolltime == 0.0f) {
                    mainmix->scrolltime = mainmix->time;
                } else {
                    if (mainmix->time - mainmix->scrolltime > 1.0f) {
                        mainmix->scenes[i][mainmix->currscene[i]]->scrollpos--;
                        mainmix->scrolltime = mainmix->time;
                    }
                }
            }
            if (mainprogram->leftmouse) {
                mainmix->scenes[i][mainmix->currscene[i]]->scrollpos--;
                mainprogram->leftmouse = false;
            }
        } else if (this->boxafter->in()) {
            //mouse in empty scrollbar part after the lightgrey visible layers part
            if (mainprogram->dragbinel) {
                if (mainmix->scrolltime == 0.0f) {
                    mainmix->scrolltime = mainmix->time;
                } else {
                    if (mainmix->time - mainmix->scrolltime > 1.0f) {
                        mainmix->scenes[i][mainmix->currscene[i]]->scrollpos++;
                        mainmix->scrolltime = mainmix->time;
                    }
                }
            }
            if (mainprogram->leftmouse) {
                mainmix->scenes[i][mainmix->currscene[i]]->scrollpos++;
                mainprogram->leftmouse = false;
            }
        }
        //trigger scrollbar tooltips
        mainprogram->scrollboxes[i]->in();

        if (!inbox) mainmix->scrolltime = 0.0f;
    }
}


int Program::quit_requester() {
    // requester: choose between exiting / saving and exiting/ cancelling exit
	int mx = -1;
	int my = -1;
	if (SDL_GetMouseFocus() == mainprogram->requesterwindow) {
		//SDL_PumpEvents();
		SDL_GetMouseState(&mx, &my);
		mx *= 2.0f;
		my *= 2.0f;
	}

	mainprogram->insmall = true;
	mainprogram->bvao = mainprogram->quboxvao;
	mainprogram->bvbuf = mainprogram->quboxvbuf;
	mainprogram->btbuf = mainprogram->quboxtbuf;
	glClear(GL_COLOR_BUFFER_BIT);

	int ret = 0;

	render_text("Are you sure about quitting the program?", white, -0.5f, 0.2f, 0.0024f, 0.004f, 1, 0);
	for (int j = 0; j < 12; j++) {
		for (int i = 0; i < 12; i++) {
			Boxx* box = binsmain->elemboxes[i * 12 + j];
			box->upvtxtoscr();
			BinElement* binel = binsmain->currbin->elements[i * 12 + j];
			if (binel->encoding) {
				render_text("!!! There are still videos being encoded to HAP !!!", red, -0.5f, 0.0f, 0.0024f, 0.004f, 1, 0);
				break;
			}
		}
	}

    std::unique_ptr <Boxx> box = std::make_unique <Boxx> ();
	box->vtxcoords->x1 = 0.15f;
	box->vtxcoords->y1 = -1.0f;
	box->vtxcoords->w = 0.3f;
	box->vtxcoords->h = 0.2f;
	box->upvtxtoscr();
	draw_box(white, black, box, -1);
	if (box->in(mx, my)) {
		draw_box(white, lightblue, box, -1);
		if (mainprogram->leftmouse || mainprogram->orderleftmouse) {
            if (this->project->path.find("autosave") != std::string::npos) {
                this->path = this->project->bupp;
                this->project->save_as();
            }
            else {
                this->project->save(this->project->path);
            }
            while (mainprogram->concatting) {
                Sleep (5);
            }
			ret = 1;
		}
	}
	render_text("SAVE PROJECT", white, box->vtxcoords->x1 + 0.02f, box->vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, 1, 0);

	box->vtxcoords->x1 = 0.45f;
	box->upvtxtoscr();
	draw_box(white, black, box, -1);
	if (box->in(mx, my)) {
		draw_box(white, lightblue, box, -1);
		if (mainprogram->leftmouse || mainprogram->orderleftmouse) {
			ret = 2;
		}
	}
	render_text("QUIT", white, box->vtxcoords->x1 + 0.02f, box->vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, 1, 0);

	box->vtxcoords->x1 = 0.75f;
	box->upvtxtoscr();
	draw_box(white, black, box, -1);
	if (box->in(mx, my)) {
		draw_box(white, lightblue, box, -1);
		if (mainprogram->leftmouse || mainprogram->orderleftmouse) {
			ret = 3;
		}
	}
	render_text("CANCEL", white, box->vtxcoords->x1 + 0.02f, box->vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, 1, 0);

	this->bvao = this->boxvao;
	this->bvbuf = this->boxvbuf;
	this->btbuf = this->boxtbuf;
	this->middlemouse = 0;
	this->rightmouse = 0;
	this->menuactivation = false;
	this->mx = -1;
	this->my = -1;

	this->insmall = false;

	return ret;
}


void Program::shelf_triggering(ShelfElement* elem) {
	// do trigger from shelf, set up in callback, executed here at a fixed the_loop position
	if (elem) {
        std::vector<Layer *> clays = mainmix->currlays[!mainprogram->prevmodus];
        int checkeddeck = 2;
        Layer *laydeck0 = nullptr;
        Layer *laydeck1 = nullptr;
        for (int k = 0; k < clays.size(); k++) {
            Layer *lay = clays[k];
            clays[k]->deautomate();
            if (elem->type == ELEM_FILE) {
                clays[k] = clays[k]->open_video(0, elem->path, true);
                // currlay is handled in lay->transfer()
                std::thread setfr(&Mixer::set_frame, mainmix, elem, clays[k]);
                setfr.detach();
                if (elem->launchtype > 0) clays[k]->prevshelfdragelems.push_back(elem);
            } else if (elem->type == ELEM_IMAGE) {
                clays[k]->open_image(elem->path);
                // currlay is handled in lay->transfer()
                std::thread setfr(&Mixer::set_frame, mainmix, elem, clays[k]);
                setfr.detach();
                if (elem->launchtype > 0) clays[k]->prevshelfdragelems.push_back(elem);
            } else if (elem->type == ELEM_LAYER) {
                clays[k] = mainmix->open_layerfile(elem->path, clays[k], true, false);  // reminder : doclips?
                std::thread setfr(&Mixer::set_frame, mainmix, elem, clays[k]);
                setfr.detach();
                if (elem->launchtype == 1) {
                    clays[k]->prevshelfdragelems = lay->prevshelfdragelems;
                    clays[k]->prevshelfdragelems.push_back(elem);
                } else if (elem->launchtype == 2) {
                    clays[k]->prevshelfdragelems = lay->prevshelfdragelems;
                    clays[k]->prevshelfdragelems.push_back(elem);
                }
                lay->set_inlayer(clays[k], true);
                mainmix->currlay[!mainprogram->prevmodus] = clays[k];
                mainmix->currlays[!mainprogram->prevmodus][k] = clays[k];
            }
            if (clays[k]->deck == 0) laydeck0 = choose_layers(0)[0];
            if (clays[k]->deck == 1) laydeck1 = choose_layers(1)[0];
        }

        std::vector<Layer *> decklays;
        decklays.push_back(laydeck0);
        decklays.push_back(laydeck1);
        for (int k = 0; k < decklays.size(); k++) {
            if (decklays[k] == nullptr) continue;
            if (elem->type == ELEM_DECK) {
                mainmix->mousedeck = decklays[k]->deck;
                if (checkeddeck == decklays[k]->deck)
                    continue;
                checkeddeck = decklays[k]->deck;
                 // copy_lpst moved to swapmap

                mainmix->set_elems(elem, mainmix->mousedeck);
                mainmix->open_deck(elem->path, true, true);  // dont load loopstation events from shelf ever?
                std::thread setfr(&Mixer::set_frames, mainmix, elem, (bool)mainmix->mousedeck);
                setfr.detach();

            }
        }

        if (elem->type == ELEM_MIX) {

            mainmix->set_elems(elem, 0);
            mainmix->set_elems(elem, 1);
            mainmix->open_mix(elem->path, true, true);  // dont load loopstation events from shelf ever
            std::thread setfr1(&Mixer::set_frames, mainmix, elem, false);
            setfr1.detach();
            std::thread setfr2(&Mixer::set_frames, mainmix, elem, true);
            setfr2.detach();
            mainmix->set_frames(elem, 0);
            mainmix->set_frames(elem, 1);
        }
    }
    mainprogram->midishelfelem = nullptr;
}




Boxx::Boxx() {
    this->vtxcoords = new BOX_COORDS;
    this->scrcoords = new BOX_COORDS;
}

Boxx::~Boxx() {
    delete this->vtxcoords;
    delete this->scrcoords;
}

Button::Button(bool state) {
    this->box = new Boxx;
    this->value = state;
    this->ccol[3] = 1.0f;
}

Button::~Button() {
    delete this->box;
    this->deautomate();
    if (mainprogram) {
        if (mainprogram->prevmodus) {
            if (lp->allbuttons.count(this)) {
                lp->allbuttons.erase(this);
            }
        }
        else {
            if (lpc->allbuttons.count(this)) {
                lpc->allbuttons.erase(this);
            }
        }
    }
}

bool Program::handle_button(Button *but, bool circlein) {
    bool ret = this->handle_button(but, circlein, true);
    return ret;
}

bool Program::handle_button(Button *but, bool circlein, bool automation, bool copycomp) {
    bool changed = false;
    if (but->box->in()) {
        if (mainprogram->leftmouse || mainprogram->orderleftmouse) {
            but->oldvalue = but->value;
            but->value++;
            if (but->value > but->toggle) but->value = 0;
            if (but->toggle == 0) but->value = 1;
            if (automation) {
                for (int i = 0; i < loopstation->elements.size(); i++) {
                    if (loopstation->elements[i]->recbut->value) {
                        loopstation->elements[i]->add_button_automationentry(but);
                    }
                }
            }
            if (!copycomp) mainprogram->register_undo(nullptr, but);
            mainprogram->leftmouse = false;
            mainprogram->orderleftmouse = false;
            changed = true;
        }
        if (mainprogram->menuactivation && !mainprogram->menuondisplay) {
            if (loopstation->butelemmap.find(but) != loopstation->butelemmap.end()) mainprogram->parammenu4->state = 2;
            else mainprogram->parammenu3->state = 2;
            mainmix->learnparam = nullptr;
            mainmix->learnbutton = but;
            mainprogram->lpstmenuon = true;
            mainprogram->menuactivation = false;
        }
        but->box->acolor[0] = 0.5f;
        but->box->acolor[1] = 0.5f;
        but->box->acolor[2] = 1.0f;
        but->box->acolor[3] = 1.0f;
    }
    else if (but->value && !circlein) {
        but->box->acolor[0] = but->tcol[0];
        but->box->acolor[1] = but->tcol[1];
        but->box->acolor[2] = but->tcol[2];
        but->box->acolor[3] = but->tcol[3];
    }
    else {
        but->box->acolor[0] = 0.0f;
        but->box->acolor[1] = 0.0f;
        but->box->acolor[2] = 0.0f;
        but->box->acolor[3] = 1.0f;
    }
    draw_box(but->box, -1);

    if (circlein) {
        float radx = but->box->vtxcoords->w / 2.0f;
        float rady = but->box->vtxcoords->h / 2.0f;
        if (but->value) {
            int cs = 1;
            if (but == mainmix->recbutS || but == mainmix->recbutQ) {
                if ((int)(mainmix->time * 10) % 10 > 5) cs = 2;
            }
            draw_box(but->ccol, but->box->vtxcoords->x1 + radx, but->box->vtxcoords->y1 + rady, 0.0225f, cs);
        }
        else draw_box(but->ccol, but->box->vtxcoords->x1 + radx, but->box->vtxcoords->y1 + rady, 0.0225f, 2);
        std::vector<float> tws = render_text(but->name[0], nullptr, white, 0.0f, 0.0f, radx / 50.0f, rady / 50.0f, 0, 0, 0);
        float x = 0.0f;
        if (tws.size()) {
            float x = tws[0] / 2.0f;
        }
        render_text(but->name[0], white, but->box->vtxcoords->x1 - x + radx / 4.0f, but->box->vtxcoords->y1 - x * rady / radx + rady / 4.0f, radx / 50.0f, rady / 50.0f);
    }

    return changed;
}

bool Button::toggled() {
    if (this->value != this->oldvalue) {
        this->oldvalue = this->value;
        if (this->skiponetoggled) {
            this->skiponetoggled = false;
        }
        else {
            // UNDO registration
            //mainprogram->register_undo(nullptr, this);
        }
        return true;
    }
    else return false;
}

void Button::deautomate() {
    if (this) {
        LoopStationElement *elem = nullptr;
        if (loopstation->butelemmap.count(this)) {
            elem = loopstation->butelemmap[this];
        }
        if (!elem) return;

        elem->buttons.erase(this);
        loopstation->allbuttons.erase(this);
        loopstation->butelemmap.erase(this);
        bool found = false;
        for (Button *but : loopstation->allbuttons) {
            if (but->layer == this->layer) {
                found = true;
            }
        }
        if (!found) elem->layers.erase(this->layer);

        for (int i = elem->eventlist.size() - 1; i >= 0; i--) {
            if (std::get<2>(elem->eventlist[i]) == this)
                elem->eventlist.erase(elem->eventlist.begin() + i);
        }
        if (elem->eventlist.size() == 0) {
            elem->loopbut->value = 0;
            elem->playbut->value = 0;
            elem->loopbut->oldvalue = 0;
            elem->playbut->oldvalue = 0;
        }
        loopstation->allbuttons.erase(this);
        loopstation->butelemmap.erase(this);
        this->box->acolor[0] = 0.2f;
        this->box->acolor[1] = 0.2f;
        this->box->acolor[2] = 0.2f;
        this->box->acolor[3] = 1.0f;
    }
}

void Button::register_midi() {
    if (this->midiport == "") return;
    registered_midi rm = mainmix->midi_registrations[!mainprogram->prevmodus][this->midi[0]][this->midi[1]][this->midiport];
    if (rm.but && rm.but != this) {
        rm.but->midi[0] = -1;
        rm.but->midi[1] = -1;
        rm.but->midiport = "";
        rm.but = nullptr;
    }
    else if (rm.par) {
        rm.par->midi[0] = -1;
        rm.par->midi[1] = -1;
        rm.par->midiport = "";
        rm.par = nullptr;
    }
    else if (rm.midielem) {
        rm.midielem->midi0 = -1;
        rm.midielem->midi1= -1;
        rm.midielem->midiport = "";
        rm.midielem = nullptr;
    }

    mainmix->midi_registrations[!mainprogram->prevmodus][this->midi[0]][this->midi[1]][this->midiport].but = this;
}

void Button::unregister_midi() {
    mainmix->midi_registrations[!mainprogram->prevmodus][this->midi[0]][this->midi[1]][this->midiport].but = nullptr;
}


void Boxx::upscrtovtx() {
    int hw = glob->w / 2;
    int hh = glob->h / 2;
    this->vtxcoords->x1 = ((this->scrcoords->x1 - hw) / hw);
    this->vtxcoords->y1 = ((this->scrcoords->y1 - hh) / -hh);
    this->vtxcoords->w = this->scrcoords->w / hw;
    this->vtxcoords->h = this->scrcoords->h / hh;
}

void Boxx::upvtxtoscr() {
    int hw = glob->w / 2;
    int hh = glob->h / 2;
    this->scrcoords->x1 = ((this->vtxcoords->x1 * hw) + hw);
    this->scrcoords->h = this->vtxcoords->h * hh;
    this->scrcoords->y1 = ((this->vtxcoords->y1 * -hh) + hh);
    this->scrcoords->w = this->vtxcoords->w * hw;
}

bool Boxx::in(bool menu) {
    if (menu) return this->in();
    return false;
}

bool Boxx::in() {
    bool ret = this->in(mainprogram->mx, mainprogram->my);
    return ret;
}

bool Boxx::in(int mx, int my) {
    if (mainprogram->menuondisplay) return false;
    this->upvtxtoscr();
    if (this->scrcoords->x1 <= mx && mx <= this->scrcoords->x1 + this->scrcoords->w) {
        if (this->scrcoords->y1 - this->scrcoords->h <= my && my <= this->scrcoords->y1) {
            mainprogram->boxhit = true;
            if (mainprogram->showtooltips && !mainprogram->ttreserved) {
                if (mainprogram->tooltipbox) {
                    delete mainprogram->tooltipbox;
                }
                mainprogram->tooltipbox = this->copy();
                mainprogram->ttreserved = this->reserved;
            }
            else {
                delete mainprogram->tooltipbox;
                mainprogram->tooltipbox = nullptr;
            }
            mainprogram->inbox = true;
            return true;
        }
    }
    return false;
}

Boxx* Boxx::copy() {
    Boxx *box = new Boxx;
    box->lcolor[0] = this->lcolor[0];
    box->lcolor[1] = this->lcolor[1];
    box->lcolor[2] = this->lcolor[2];
    box->lcolor[3] = this->lcolor[3];
    box->acolor[0] = this->acolor[0];
    box->acolor[1] = this->acolor[1];
    box->acolor[2] = this->acolor[2];
    box->acolor[3] = this->acolor[3];
    box->vtxcoords->x1 = this->vtxcoords->x1;
    box->vtxcoords->y1 = this->vtxcoords->y1;
    box->vtxcoords->w = this->vtxcoords->w;
    box->vtxcoords->h = this->vtxcoords->h;
    box->scrcoords->x1 = this->scrcoords->x1;
    box->scrcoords->y1 = this->scrcoords->y1;
    box->scrcoords->w = this->scrcoords->w;
    box->scrcoords->h = this->scrcoords->h;
    box->tex = this->tex;
    box->tooltiptitle = this->tooltiptitle;
    box->tooltip = this->tooltip;
    return box;
}



EWindow::~EWindow() {
    this->closethread = 1;
    while (this->closethread) {
        this->syncnow = true;
        this->sync.notify_all();
    }
}

void output_video(EWindow* mwin) {

	GLuint tex;
	while (1) {
        SDL_GL_MakeCurrent(mwin->win, mwin->glc);

		std::unique_lock<std::mutex> lock(mwin->syncmutex);
		mwin->sync.wait(lock, [&] {return mwin->syncnow; });
		mwin->syncnow = false;
		lock.unlock();

		// receive end of thread signal
		if (mwin->closethread) {
			mwin->closethread = false;
			return;
		}

        if (mwin->mixid == 4) tex = mwin->lay->fbotex;
        else if (mwin->mixid == 3) tex = ((MixNode*)(mainprogram->nodesmain->mixnodes[1][2]))->mixtex;
        else if (mwin->mixid == 2) tex = ((MixNode*)(mainprogram->nodesmain->mixnodes[0][2]))->mixtex;
		else {
            if (mainprogram->prevmodus) {
                tex = ((MixNode*)(mainprogram->nodesmain->mixnodes[0][mwin->mixid]))->mixtex;
            }
            else {
                tex = ((MixNode*)(mainprogram->nodesmain->mixnodes[1][mwin->mixid]))->mixtex;
            }
		}

		GLfloat cf = glGetUniformLocation(mainprogram->ShaderProgram, "cf");
		GLint wipe = glGetUniformLocation(mainprogram->ShaderProgram, "wipe");
		GLint mixmode = glGetUniformLocation(mainprogram->ShaderProgram, "mixmode");
		if (mwin->mixid == 2 || mwin->mixid == 3) {
            if (mainmix->wipe[mwin->mixid == 3] > -1) {
                if (mwin->mixid == 3) {
                    glUniform1f(cf, mainmix->crossfadecomp->value);
                    MixNode *node = (MixNode*)mainprogram->nodesmain->mixnodes[1][0];
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, node->mixtex);
                    node = (MixNode*)mainprogram->nodesmain->mixnodes[1][1];
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, node->mixtex);
                    glActiveTexture(GL_TEXTURE0);
                }
                else {
                    glUniform1f(cf, mainmix->crossfade->value);
                    MixNode *node = (MixNode *) mainprogram->nodesmain->mixnodes[0][0];
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, node->mixtex);
                    node = (MixNode *) mainprogram->nodesmain->mixnodes[0][1];
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, node->mixtex);
                    glActiveTexture(GL_TEXTURE0);
                }
                glUniform1i(wipe, 1);
                glUniform1i(mixmode, 18);
                GLint wkind = glGetUniformLocation(mainprogram->ShaderProgram, "wkind");
                glUniform1i(wkind, mainmix->wipe[1]);
                GLint dir = glGetUniformLocation(mainprogram->ShaderProgram, "dir");
                glUniform1i(dir, mainmix->wipedir[1]);
                GLfloat xpos = glGetUniformLocation(mainprogram->ShaderProgram, "xpos");
                glUniform1f(xpos, mainmix->wipex[1]->value);
                GLfloat ypos = glGetUniformLocation(mainprogram->ShaderProgram, "ypos");
                glUniform1f(ypos, mainmix->wipey[1]->value);
            }
        }
		GLint down = glGetUniformLocation(mainprogram->ShaderProgram, "down");
		glUniform1i(down, 1);

		glViewport(0.0f, 0.0f, mwin->w, mwin->h);
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		glActiveTexture(GL_TEXTURE0);
		int sw, sh;
		glBindTexture(GL_TEXTURE_2D, tex);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &sw);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &sh);
		if (sw !=0 && sh != 0) {
		    if ((float)mwin->w / (float)mwin->h < (float)sw / (float)sh) {
		        float hoff = (float)mwin->h - (float)mwin->h / (((float)sw * ((float)mwin->h / (float)sh)) / (float)mwin->w);
		        glViewport(0.0f, hoff / 2.0f, mwin->w, mwin->h - hoff);
		    }
		    else {
		        float woff = (float)mwin->w - (float)mwin->w / (((float)sh * ((float)mwin->w / (float)sw)) / (float)mwin->h);
		        glViewport(woff / 2.0f, 0.0f, mwin->w - woff, mwin->h);
		    }
		}
		glBindVertexArray(mwin->vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glUniform1i(down, 0);
		glUniform1i(wipe, 0);
		glUniform1i(mixmode, 0);
		mwin->syncendnow = true;
		while (mwin->syncendnow) {
			mwin->syncend.notify_all();
		}
		SDL_GL_SwapWindow(mwin->win);
	}
    glDeleteBuffers(1, &mwin->vbuf);
    glDeleteBuffers(1, &mwin->tbuf);
    glDeleteVertexArrays(1, &mwin->vao);
}


void handle_binwin() {
    while (binsmain->floating) {
        std::unique_lock<std::mutex> lock(binsmain->syncmutex);
        binsmain->sync.wait(lock, [&] { return binsmain->syncnow; });
        binsmain->syncnow = false;
        lock.unlock();

        for (OutputEntry *entry : mainprogram->outputentries) {
            if (binsmain->screen == entry->screen) {
                entry->win->lay = nullptr;
                SDL_DestroyWindow(entry->win->win);
                mainprogram->outputentries.erase(
                        std::find(mainprogram->outputentries.begin(), mainprogram->outputentries.end(),
                                  entry));
                delete entry->win;
            }
        }

        SDL_GL_MakeCurrent(binsmain->win, binsmain->glc);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDrawBuffer(GL_BACK_LEFT);

        int bumx = mainprogram->mx;
        int bumy = mainprogram->my;
        bool buma = mainprogram->menuactivation;
        bool bulmo = mainprogram->lmover;
        if (mainprogram->binlmover) mainprogram->lmover = true;
        if (SDL_GetMouseFocus() == binsmain->win) {
            mainprogram->mx = mainprogram->binmx;
            mainprogram->my = mainprogram->binmy;
            mainprogram->menuactivation = mainprogram->binmenuactivation;
        }
        else if (SDL_GetMouseFocus() == mainprogram->mainwindow) {
            mainprogram->mx = 99999;
            mainprogram->my = 99999;
            mainprogram->menuactivation = false;
            mainprogram->rightmouse = false;
        }
        mainprogram->globw = glob->w;
        mainprogram->globh = glob->h;
        glob->w = binsmain->globw;
        glob->h = binsmain->globh;
        mainprogram->bvao = mainprogram->binvao;
        mainprogram->bvbuf = mainprogram->binvbuf;
        mainprogram->btbuf = mainprogram->bintbuf;

        glViewport(0, 0, glob->w, glob->h);
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        binsmain->inbin = true;
        binsmain->handle(true);

        if (mainprogram->dragbinel) {
            // draw texture of element being dragged
            float boxwidth = 0.3f;
            float nmx = mainprogram->xscrtovtx(mainprogram->mx) + boxwidth / 2.0f;
            float nmy = 2.0 - mainprogram->yscrtovtx(mainprogram->my) - boxwidth / 2.0f;
            mainprogram->frontbatch = true;
            draw_box(nullptr, black, -1.0f - 0.5 * boxwidth + nmx, -1.0f - 0.5 * boxwidth + nmy, boxwidth, boxwidth, mainprogram->dragbinel->tex);
            mainprogram->frontbatch = false;
        }
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        // draw frontbatch one by one: lines, triangles, menus, drag tex
        GLuint textmode = glGetUniformLocation(mainprogram->ShaderProgram, "textmode");
        for (int i = 0; i < mainprogram->binguielems.size(); i++) {
            GUI_Element *elem = mainprogram->binguielems[i];
            if (elem->type == GUI_LINE) {
                //draw_line(elem->line);
                delete elem->line;
            }
            else if (elem->type == GUI_TRIANGLE) {
                //draw_triangle(elem->triangle);
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
        mainprogram->binguielems.clear();

        mainprogram->bvao = mainprogram->boxvao;
        mainprogram->bvbuf = mainprogram->boxvbuf;
        mainprogram->btbuf = mainprogram->boxtbuf;
        if (SDL_GetMouseFocus() != mainprogram->mainwindow) {
            mainprogram->mx = -1;
            mainprogram->my = 100;
            mainprogram->menuactivation = false;
        }
        else {
            mainprogram->mx = bumx;
            mainprogram->my = bumy;
            mainprogram->menuactivation = buma;
        }
        mainprogram->lmover = bulmo;
        glob->w = mainprogram->globw;
        glob->h = mainprogram->globh;

        binsmain->inbin = false;

        binsmain->syncendnow = true;
        while (binsmain->syncendnow) {
            binsmain->syncend.notify_all();
        }

        SDL_GL_SwapWindow(binsmain->win);
    }
}


#ifdef POSIX
void Program::stream_to_v4l2loopbacks() {

    for (auto entry : v4l2lbtexmap) {

        int output = this->v4l2lboutputmap[entry.first];

        size_t framesize;
        if (this->v4l2lbnewmap[entry.first] == 0) {
            framesize = this->set_v4l2format(output, entry.second);
            if (framesize == 0) return;
            this->v4l2lbnewmap[entry.first] = framesize;
        }
        framesize = this->v4l2lbnewmap[entry.first];

        char *data = (char *) calloc(framesize, 1);
        glBindTexture(GL_TEXTURE_2D, entry.second);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, data);
        size_t written = write(output, data, framesize);
        free(data);
        if (written < 0) {
            std::cerr << "ERROR: could not write to output device!\n";
            close(output);
            break;
        }
    }
}
#endif


#ifdef WINDOWS
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

// camera utility functions

HRESULT EnumerateDevices(REFGUID category, IEnumMoniker** ppEnum)
{
	// Create the System Device Enumerator.
	ICreateDevEnum* pDevEnum;
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

void DisplayDeviceInformation(IEnumMoniker* pEnum)
{
	IMoniker* pMoniker = nullptr;
	mainprogram->livedevices.clear();

	while (pEnum->Next(1, &pMoniker, nullptr) == S_OK)
	{
		IPropertyBag* pPropBag;
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

// functions that handle menus

void get_cameras()
{
#ifdef WINDOWS
	HRESULT hr;
	if (1)
	{
		IEnumMoniker* pEnum;

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
#ifdef POSIX
    mainprogram->devvideomap.clear();
    mainprogram->livedevices.clear();
	std::unordered_map<std::string, std::wstring> map;
	std::filesystem::path dir("/sys/class/video4linux");
	for (std::filesystem::directory_iterator iter(dir), end; iter != end; ++iter) {
		std::ifstream name;
		name.open(iter->path().generic_string() + "/name");
		std::string istring;
		safegetline(name, istring);
		istring = istring.substr(0, istring.find(":"));
		std::wstring wstr(istring.begin(), istring.end());
		map["/dev/" + basename(iter->path().generic_string())] = wstr;
	}
    using convert_type = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_type, wchar_t> converter;
	std::unordered_map<std::string, std::wstring>::iterator it;
	for (it = map.begin(); it != map.end(); it++) {
		struct v4l2_capability cap;
		int fd = open(it->first.c_str(), O_RDONLY);
		ioctl(fd, VIDIOC_QUERYCAP, &cap);
		if (cap.device_caps & V4L2_CAP_VIDEO_CAPTURE) {
		    mainprogram->livedevices.push_back(it->second);
            mainprogram->devvideomap[converter.to_bytes(it->second)] = it->first;
		}
		close(fd);
	}
#endif
}    //utility function


int Program::handle_menu(Menu* menu) {
	int ret = mainprogram->handle_menu(menu, 0.0f, 0.0f);
	return ret;
}
int Program::handle_menu(Menu* menu, float xshift, float yshift) {
	if (menu->state > 1) {
        mainprogram->frontbatch = true;
        menu->box->upvtxtoscr();
        if (std::find(mainprogram->actmenulist.begin(), mainprogram->actmenulist.end(), menu) ==
            mainprogram->actmenulist.end()) {
            mainprogram->actmenulist.clear();
        }
        int size = 0;
        for (int k = 0; k < menu->entries.size(); k++) {
            std::size_t sub = menu->entries[k].find("submenu");
            if (sub != 0) size++;
        }
        if (menu->menuy + mainprogram->yvtxtoscr(yshift) > glob->h - size * mainprogram->yvtxtoscr(0.075f))
            menu->menuy = glob->h - size * mainprogram->yvtxtoscr(0.075f) + mainprogram->yvtxtoscr(yshift);
        if (size > 24) menu->menuy = mainprogram->yvtxtoscr(mainprogram->layh / 2.0f) - mainprogram->yvtxtoscr(yshift);
        float vmx = (float) menu->menux * 2.0 / glob->w;
        float vmy = (float) menu->menuy * 2.0 / glob->h;
        float lc[] = {0.0, 0.0, 0.0, 1.0};
        float ac1[] = {0.3, 0.3, 0.3, 1.0};
        float ac2[] = {0.5, 0.5, 1.0, 1.0};
        int numsubs = 0;
        float limit = 1.0f;
        int notsubk = 0;
        std::vector<std::string> entries = menu->entries;

        for (int k = 0; k < entries.size(); k++) {
			float xoff = 0.0f;
			int koff;
			if (notsubk > 23) {
				if (mainprogram->xscrtovtx(menu->menux) > limit) xoff = xshift;
				else xoff = menu->width * 1.5f + xshift;
				koff = menu->entries.size() - 24;
			}
			else {
                if (mainprogram->xscrtovtx(menu->menux) > limit) {
                    xoff = -menu->width * 1.5f + xshift;
                }
                else xoff = 0.0f + xshift;
                //if (menu->entries.size() < 25) xoff = 0.0f + xshift;
				koff = 0;
			}
			std::size_t sub = menu->entries[k].find("submenu");
			if (sub != 0) {
				notsubk++;
				if (menu->box->scrcoords->x1 + menu->menux + mainprogram->xvtxtoscr(xoff) < mainprogram->mx && mainprogram->mx < menu->box->scrcoords->x1 + menu->box->scrcoords->w + menu->menux + mainprogram->xvtxtoscr(xoff) && menu->box->scrcoords->y1 - menu->box->scrcoords->h + menu->menuy + (k - koff - numsubs) * mainprogram->yvtxtoscr(0.075f) - mainprogram->yvtxtoscr(yshift) < mainprogram->my && mainprogram->my < menu->box->scrcoords->y1 + menu->menuy + (k - koff - numsubs) * mainprogram->yvtxtoscr(0.075f) - mainprogram->yvtxtoscr(yshift)) {
					draw_box(lc, ac2, menu->box->vtxcoords->x1 + vmx + xoff, menu->box->vtxcoords->y1 - (k - koff -
                                                                                                         numsubs) * 0.075f - vmy + yshift, menu->width * 1.5f, 0.075f, -1);
					if (mainprogram->leftmousedown) mainprogram->leftmousedown = false;
					if (mainprogram->lmover) {
						for (int i = 0; i < mainprogram->menulist.size(); i++) {
							mainprogram->menulist[i]->state = 0;
						}
						mainprogram->menuchosen = true;
						menu->currsub = -1;
                        mainprogram->frontbatch = false;
                        mainprogram->lmover = false;
                        mainprogram->recundo = false;
                        mainprogram->inbox = true;
						return k - numsubs;
					}
				}
				else {
					draw_box(lc, ac1, menu->box->vtxcoords->x1 + vmx + xoff, menu->box->vtxcoords->y1 - (k - koff -
                                                                                                         numsubs) * 0.075f - vmy + yshift, menu->width * 1.5f, 0.075f, -1);
				}
				render_text(menu->entries[k], white, menu->box->vtxcoords->x1 + vmx + 0.0117f + xoff, menu->box->vtxcoords->y1 - (k - koff - numsubs) * 0.075f - vmy + yshift + 0.0225f, 0.00045f, 0.00075f);
			}
			else {
				numsubs++;
				if (menu->currsub == k || (menu->box->scrcoords->x1 + menu->menux + mainprogram->xvtxtoscr(xoff) < mainprogram->mx && mainprogram->mx < menu->box->scrcoords->x1 + menu->box->scrcoords->w + menu->menux + mainprogram->xvtxtoscr(xoff) && menu->box->scrcoords->y1 - menu->box->scrcoords->h + menu->menuy + (k - koff - numsubs + 1) * mainprogram->yvtxtoscr(0.075f) - mainprogram->yvtxtoscr(yshift) < mainprogram->my && mainprogram->my < menu->box->scrcoords->y1 + menu->menuy + (k - koff - numsubs + 1) * mainprogram->yvtxtoscr(0.075f) - mainprogram->yvtxtoscr(yshift))) {
					if (menu->currsub == k || mainprogram->lmover) {
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
								    xs = xshift - menu->width * 1.5f;
								}
								else {
								    xs = xshift + menu->width * 1.5f;
								}

								// start submenu
                                mainprogram->menulist[i]->menux = menu->menux;
                                mainprogram->menulist[i]->menuy = menu->box->scrcoords->y1 - menu->box->scrcoords->h + menu->menuy + (k - koff - numsubs + 1) * mainprogram->yvtxtoscr(0.075f) - mainprogram->yvtxtoscr(yshift);
								int ret = mainprogram->handle_menu(mainprogram->menulist[i], xs, yshift);
                                this->prevmenuchoices.push_back(k - numsubs + 1);
                                if (mainprogram->menuchosen) {
									menu->state = 0;
									mainprogram->menuresults.insert(mainprogram->menuresults.begin(), ret);
									menu->currsub = -1;
                                    mainprogram->frontbatch = false;
                                    mainprogram->recundo = false;
                                    mainprogram->inbox = true;
									return k - numsubs + 1;
								}
								else mainprogram->frontbatch = true;
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
        mainprogram->frontbatch = false;
	}
	return -1;
}

void Program::handle_mixenginemenu() {
	int k = -1;
	// Do mainprogram->mixenginemenu
	k = mainprogram->handle_menu(mainprogram->mixenginemenu);
	if (k == 0) {
		if (mainmix->mousenode && mainprogram->menuresults.size()) {
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
        mainprogram->recundo = true;
	}
}

void Program::handle_effectmenu() {
    if (!mainmix->insert) {
        mainprogram->effectmenu->entries.insert(mainprogram->effectmenu->entries.begin(), "Delete effect");
    }
	int k = -1;
	// Draw and handle mainprogram->effectmenu
	k = mainprogram->handle_menu(mainprogram->effectmenu);
	if (k > -1) {
        std::vector<Effect*>& evec = mainmix->mouselayer->choose_effects();
		if (k == 0 && mainmix->mouseeffect != evec.size()) {
			mainmix->mouselayer->delete_effect(mainmix->mouseeffect);
		}
		else if (mainmix->insert) {
		    mainmix->mouselayer->add_effect((EFFECT_TYPE)mainprogram->abeffects[k], mainmix->mouseeffect);
		}
		else {
			int mon = evec[mainmix->mouseeffect]->node->monitor;
			mainmix->mouselayer->replace_effect((EFFECT_TYPE)mainprogram->abeffects[k - 1], mainmix->mouseeffect);
			evec[mainmix->mouseeffect]->node->monitor = mon;
		}
		mainmix->mouselayer = nullptr;
		mainmix->mouseeffect = -1;
	}
    if (!mainmix->insert) {
        mainprogram->effectmenu->entries.erase(mainprogram->effectmenu->entries.begin());
    }
	if (mainprogram->menuchosen) {
		mainprogram->menuchosen = false;
		mainprogram->menuactivation = 0;
		mainprogram->menuresults.clear();
        mainprogram->recundo = true;
	}
 }

void Program::handle_parammenu1() {
    // Draw and Program::handle mainprogram->parammenu1
    int k = -1;
    k = mainprogram->handle_menu(mainprogram->parammenu1);
    if (k > -1) {
        if (k == 0) {
            mainmix->learn = true;
        }
        else if (k == 1) {
            mainmix->learnparam->value = mainmix->learnparam->deflt;
        }
    }
    if (mainprogram->menuchosen) {
        mainprogram->menuchosen = false;
        mainprogram->menuactivation = 0;
        mainprogram->menuresults.clear();
        mainprogram->recundo = true;
    }
}

void Program::handle_parammenu2() {
    int k = -1;
    // Draw and Program::handle mainprogram->parammenu2 (with automation removal)
    k = mainprogram->handle_menu(mainprogram->parammenu2);
    if (k > -1) {
        if (k == 0) {
            mainmix->learn = true;
        }
        else if (k == 1) {
            if (mainmix->learnparam) {
                LoopStationElement* elem = loopstation->parelemmap[mainmix->learnparam];
                elem->params.erase(mainmix->learnparam);
                if (mainmix->learnparam->effect) elem->layers.erase(mainmix->learnparam->effect->layer);
                for (int i = elem->eventlist.size() - 1; i >= 0; i--) {
                    if (std::get<1>(elem->eventlist[i]) == mainmix->learnparam) elem->eventlist.erase(elem->eventlist.begin() + i);
                }
                loopstation->parelemmap.erase(mainmix->learnparam);
                mainmix->learnparam->box->acolor[0] = 0.2f;
                mainmix->learnparam->box->acolor[1] = 0.2f;
                mainmix->learnparam->box->acolor[2] = 0.2f;
                mainmix->learnparam->box->acolor[3] = 1.0f;
            }
            else if (mainmix->learnbutton) {
                LoopStationElement* elem = loopstation->butelemmap[mainmix->learnbutton];
                elem->buttons.erase(mainmix->learnbutton);
                if (mainmix->learnbutton->layer) elem->layers.erase(mainmix->learnbutton->layer);
                for (int i = elem->eventlist.size() - 1; i >= 0; i--) {
                    if (std::get<2>(elem->eventlist[i]) == mainmix->learnbutton) elem->eventlist.erase(elem->eventlist.begin() + i);
                }
                loopstation->butelemmap.erase(mainmix->learnbutton);
                mainmix->learnbutton->box->acolor[0] = 0.2f;
                mainmix->learnbutton->box->acolor[1] = 0.2f;
                mainmix->learnbutton->box->acolor[2] = 0.2f;
                mainmix->learnbutton->box->acolor[3] = 1.0f;
            }
        }
        else if (k == 2) {
            mainmix->learnparam->value = mainmix->learnparam->deflt;
        }
    }
    if (mainprogram->menuchosen) {
        mainprogram->menuchosen = false;
        mainprogram->menuactivation = 0;
        mainprogram->menuresults.clear();
        mainprogram->recundo = true;
    }
}

void Program::handle_parammenu1b() {
    // Draw and Program::handle mainprogram->parammenu1b
    int k = -1;
    k = mainprogram->handle_menu(mainprogram->parammenu1b);
    if (k > -1) {
        if (k == 0) {
            mainmix->learn = true;
        }
        else if (k == 1) {
            mainmix->learnparam->value = mainmix->learnparam->deflt;
        }
        else if (k == 2) {
            mainmix->menulayer->lockspeed = !mainmix->menulayer->lockspeed;
        }
    }
    if (mainprogram->menuchosen) {
        mainprogram->menuchosen = false;
        mainprogram->menuactivation = 0;
        mainprogram->menuresults.clear();
        mainprogram->recundo = true;
    }
}

void Program::handle_parammenu2b() {
    int k = -1;
    // Draw and Program::handle mainprogram->parammenu2b (with automation removal)
    k = mainprogram->handle_menu(mainprogram->parammenu2b);
    if (k > -1) {
        if (k == 0) {
            mainmix->learn = true;
        }
        else if (k == 1) {
            if (mainmix->learnparam) {
                LoopStationElement* elem = loopstation->parelemmap[mainmix->learnparam];
                elem->params.erase(mainmix->learnparam);
                if (mainmix->learnparam->effect) elem->layers.erase(mainmix->learnparam->effect->layer);
                for (int i = elem->eventlist.size() - 1; i >= 0; i--) {
                    if (std::get<1>(elem->eventlist[i]) == mainmix->learnparam) elem->eventlist.erase(elem->eventlist.begin() + i);
                }
                loopstation->parelemmap.erase(mainmix->learnparam);
                mainmix->learnparam->box->acolor[0] = 0.2f;
                mainmix->learnparam->box->acolor[1] = 0.2f;
                mainmix->learnparam->box->acolor[2] = 0.2f;
                mainmix->learnparam->box->acolor[3] = 1.0f;
            }
            else if (mainmix->learnbutton) {
                LoopStationElement* elem = loopstation->butelemmap[mainmix->learnbutton];
                elem->buttons.erase(mainmix->learnbutton);
                if (mainmix->learnbutton->layer) elem->layers.erase(mainmix->learnbutton->layer);
                for (int i = elem->eventlist.size() - 1; i >= 0; i--) {
                    if (std::get<2>(elem->eventlist[i]) == mainmix->learnbutton) elem->eventlist.erase(elem->eventlist.begin() + i);
                }
                loopstation->butelemmap.erase(mainmix->learnbutton);
                mainmix->learnbutton->box->acolor[0] = 0.2f;
                mainmix->learnbutton->box->acolor[1] = 0.2f;
                mainmix->learnbutton->box->acolor[2] = 0.2f;
                mainmix->learnbutton->box->acolor[3] = 1.0f;
            }
        }
        else if (k == 2) {
            mainmix->learnparam->value = mainmix->learnparam->deflt;
        }
        else if (k == 3) {
            mainmix->menulayer->lockspeed = !mainmix->menulayer->lockspeed;
        }
    }
    if (mainprogram->menuchosen) {
        mainprogram->menuchosen = false;
        mainprogram->menuactivation = 0;
        mainprogram->menuresults.clear();
        mainprogram->recundo = true;
    }
}

void Program::handle_parammenu3() {
    // Draw and Program::handle mainprogram->parammenu3
    int k = -1;
    k = mainprogram->handle_menu(mainprogram->parammenu3);
    if (k > -1) {
        if (k == 0) {
            mainmix->learn = true;
        }
    }
    if (mainprogram->menuchosen) {
        mainprogram->menuchosen = false;
        mainprogram->menuactivation = 0;
        mainprogram->menuresults.clear();
        mainprogram->recundo = true;
    }
}

void Program::handle_parammenu4() {
    int k = -1;
    // Draw and Program::handle mainprogram->parammenu4 (with automation removal)
    k = mainprogram->handle_menu(mainprogram->parammenu4);
    if (k > -1) {
        if (k == 0) {
            mainmix->learn = true;
        }
        else if (k == 1) {
            mainmix->learnparam->deautomate();
            mainmix->learnbutton->deautomate();
        }
    }
    if (mainprogram->menuchosen) {
        mainprogram->menuchosen = false;
        mainprogram->menuactivation = 0;
        mainprogram->menuresults.clear();
        mainprogram->recundo = true;
    }
}

void Program::handle_parammenu5() {
    // Draw and Program::handle mainprogram->parammenu5
    int k = -1;
    k = mainprogram->handle_menu(mainprogram->parammenu5);
    if (k > -1) {
        if (k == 0) {
            mainmix->learn = true;
        }
        else if (k == 1) {
            // lock zoom and pan
            mainmix->menulayer->lockzoompan = !mainmix->menulayer->lockzoompan;
        }
    }
    if (mainprogram->menuchosen) {
        mainprogram->menuchosen = false;
        mainprogram->menuactivation = 0;
        mainprogram->menuresults.clear();
        mainprogram->recundo = true;
    }
}

void Program::handle_parammenu6() {
    // Draw and Program::handle mainprogram->parammenu6
    int k = -1;
    k = mainprogram->handle_menu(mainprogram->parammenu6);
    if (k > -1) {
        if (k == 0) {
            mainmix->learn = true;
        }
        else if (k == 1) {
            mainmix->learnparam->deautomate();
            mainmix->learnbutton->deautomate();
        }
        else if (k == 2) {
            // lock zoom and pan
            mainmix->menulayer->lockzoompan = !mainmix->menulayer->lockzoompan;
        }
    }
    if (mainprogram->menuchosen) {
        mainprogram->menuchosen = false;
        mainprogram->menuactivation = 0;
        mainprogram->menuresults.clear();
        mainprogram->recundo = true;
    }
}

void Program::handle_loopmenu() {
	int k = -1;
	// Draw and Program::handle mainprogram->loopmenu
	k = mainprogram->handle_menu(mainprogram->loopmenu);
	if (k > -1) {
		if (k == 0) {
		    // set start of playloop to frame position
			mainmix->mouselayer->startframe->value = mainmix->mouselayer->frame;
			if (mainmix->mouselayer->startframe->value > mainmix->mouselayer->endframe->value) {
				mainmix->mouselayer->endframe->value = mainmix->mouselayer->startframe->value;
			}
			mainmix->mouselayer->set_clones();
		}
		else if (k == 1) {
            // set end of playloop to frame position
			mainmix->mouselayer->endframe->value = mainmix->mouselayer->frame;
			if (mainmix->mouselayer->startframe->value > mainmix->mouselayer->endframe->value) {
				mainmix->mouselayer->startframe->value = mainmix->mouselayer->endframe->value;
			}
			mainmix->mouselayer->set_clones();
		}
		else if (k == 2) {
		    // copy playloop duration
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
			mainmix->cbduration = ((mainmix->mouselayer->endframe->value - mainmix->mouselayer->startframe->value) * mainmix->mouselayer->millif) / (mainmix->mouselayer->speed->value * mainmix->mouselayer->speed->value * fac * fac);
			int dummy = 0;
		}
		else if (k == 3) {
		    // paste playloop duration by changing the speed
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
			mainmix->mouselayer->speed->value = sqrt(((mainmix->mouselayer->endframe->value - mainmix->mouselayer->startframe->value) * mainmix->mouselayer->millif) / mainmix->cbduration / (fac * fac));
			mainmix->mouselayer->set_clones();
		}
		else if (k == 4) {
		    // paste playloop duration by changing the duration itself
			float sf, ef;
			float loop = mainmix->mouselayer->endframe->value - mainmix->mouselayer->startframe->value;
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
			float end = mainmix->mouselayer->numf - (mainmix->mouselayer->startframe->value + loop / fac);
			if (end > 0) {
				mainmix->mouselayer->endframe->value = mainmix->mouselayer->startframe->value + loop / fac;
			}
			else {
				mainmix->mouselayer->endframe->value = mainmix->mouselayer->numf;
				float start = mainmix->mouselayer->startframe->value + end;
				if (start > 0) {
					mainmix->mouselayer->startframe->value += end;
				}
				else {
					mainmix->mouselayer->startframe->value = 0.0f;
					mainmix->mouselayer->endframe->value = mainmix->mouselayer->numf;
					mainmix->mouselayer->speed->value *= sqrt((float)mainmix->mouselayer->numf / ((float)mainmix->mouselayer->numf - start));
				}
			}
			mainmix->mouselayer->set_clones();
		}
        else if (k == 5) {
            mainmix->learn = true;
        }
	}
	if (mainprogram->menuchosen) {
		mainprogram->menuchosen = false;
		mainprogram->menuactivation = 0;
		mainprogram->menuresults.clear();
        mainprogram->recundo = true;
	}
}

void Program::make_mixtargetmenu() {
    // the output display menu (SDL_GetNumVideoDisplays() doesn't allow hotplugging screens...
    int numd = SDL_GetNumVideoDisplays();
    std::vector<std::string> mixtargets;
    if (numd == 1) mixtargets.push_back("No external displays");
    for (int i = 1; i < numd; i++) {
        std::string dname = SDL_GetDisplayName(i);
        bool active = false;
        for (int j = 0; j < mainprogram->outputentries.size(); j++) {
            if (dname == SDL_GetDisplayName(mainprogram->outputentries[j]->screen)) {
                bool cond;
                if (mainprogram->monitormenu->value == 4) {
                    cond = (mainprogram->outputentries[j]->win->lay == mainmix->mouselayer);
                }
                else{
                    cond = (mainprogram->outputentries[j]->win->mixid == mainprogram->monitormenu->value);
                }
                if  (cond) {
                    active = true;
                    break;
                }
            }
        }
        if (active) {
            mixtargets.push_back("v " + dname);
        } else {
            mixtargets.push_back("  " + dname);
        }
    }
    mainprogram->make_menu("mixtargetmenu", mainprogram->mixtargetmenu, mixtargets);
}

void Program::handle_monitormenu() {
    int k = -1;
    // draw and handle monitormenu
    std::vector<OutputEntry*> currentries;
    std::vector<OutputEntry*> takenentries;
    GLuint tex;
    int numd = SDL_GetNumVideoDisplays();
    if (mainprogram->monitormenu->state > 1) {
        // the output display menu (SDL_GetNumVideoDisplays() doesn't allow hotplugging screens...
        std::vector<std::string> monitors;
        monitors.push_back("View full screen");
        monitors.push_back("submenu wipemenu");
        monitors.push_back("Choose wipe");
        monitors.push_back("MIDI learn wipe position");
        monitors.push_back("submenu mixtargetmenu");
        monitors.push_back("Show on display");
        mainprogram->make_menu("monitormenu", mainprogram->monitormenu, monitors);

        mainprogram->make_mixtargetmenu();

        for (int i = 0; i < mainprogram->outputentries.size(); i++) {
            bool cond;
            if (mainprogram->outputentries[i]->win->mixid == 4) {
                cond = false;
            }
            else {
                cond = (mainprogram->outputentries[i]->win->mixid == mainprogram->monitormenu->value);
            }
            if (cond) {
                currentries.push_back(mainprogram->outputentries[i]);
            } else {
                takenentries.push_back(mainprogram->outputentries[i]);
            }
        }
#ifdef POSIX
        // handle selection of active v4l2loopback devices used to stream video at
        if (mainprogram->monitormenu->value == 3) {
            tex = mainprogram->nodesmain->mixnodes[0][0]->mixtex;
        }
        else {
            tex = mainprogram->nodesmain->mixnodes[!mainprogram->prevmodus][mainprogram->monitormenu->value]->mixtex;
        }
        this->register_v4l2lbdevices(monitors, tex);

        mainprogram->make_menu("monitormenu", mainprogram->monitormenu, monitors);
#endif
    }
    k = mainprogram->handle_menu(mainprogram->monitormenu);
    if (k > -1) {
        if (k == 0) {
            mainprogram->fullscreen = mainprogram->monitormenu->value;
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
        else if (k == 2) {
            mainmix->learn = true;
            mainmix->learnbutton = nullptr;
            if (mainprogram->monitormenu->value > 1) {
                mainmix->learnparam = mainmix->wipex[mainprogram->monitormenu->value - 2];
            }
            else {
                mainmix->learnparam = mainmix->currlay[!mainprogram->prevmodus]->blendnode->wipex;
            }
        }
        else if (k == 3) {
            if (numd > 1) {
                // chosen output screen already used? re-use window
                int currdisp = SDL_GetWindowDisplayIndex(mainprogram->mainwindow);
                int disp = mainprogram->menuresults[0] + (mainprogram->menuresults[0] >= currdisp);
                bool switched = false;
                for (int i = 0; i < takenentries.size(); i++) {
                    if (takenentries[i]->screen == disp) {
                        takenentries[i]->win->mixid = mainprogram->monitormenu->value;
                        takenentries[i]->win->lay = nullptr;
                        switched = true;
                    }
                }
                if (switched) {
                } else {
                    int swoff = -1;
                    for (int i = 0; i < currentries.size(); i++) {
                        if (currentries[i]->screen == disp) {
                            swoff = i;
                            break;
                        }
                    }

                    // stop output display of chosen mixwindow on chosen screen
                    if (swoff != -1) {
                        currentries[swoff]->win->lay = nullptr;
                        SDL_DestroyWindow(currentries[swoff]->win->win);
                        mainprogram->outputentries.erase(
                                std::find(mainprogram->outputentries.begin(), mainprogram->outputentries.end(),
                                          currentries[swoff]));
                        delete currentries[swoff]->win;
                    } else {
                        // open new window on chosen output screen and start display thread (synced at end of the_loop)
                        if (binsmain->floating && binsmain->screen == disp) {
                            SDL_DestroyWindow(binsmain->win);
                            binsmain->floating = false;
                        }
                        EWindow *mwin = new EWindow;
                        mwin->mixid = mainprogram->monitormenu->value;
                        OutputEntry *entry = new OutputEntry;
                        entry->screen = disp;
                        entry->win = mwin;
                        mainprogram->outputentries.push_back(entry);
                        SDL_Rect rc1;
                        SDL_GetDisplayBounds(disp, &rc1);
                        SDL_Rect rc2;
                        SDL_GetDisplayUsableBounds(disp, &rc2);
                        mwin->w = rc1.w;
                        mwin->h = rc1.h;
                        SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
                        SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
                        mwin->win = SDL_CreateWindow(PROGRAM_NAME, rc1.x, rc1.y, rc2.w, rc2.h,
                                                     SDL_WINDOW_OPENGL | SDL_WINDOW_MAXIMIZED | SDL_WINDOW_RESIZABLE |
                                                     SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_ALLOW_HIGHDPI);
                        //SDL_RaiseWindow(mainprogram->mainwindow);
                        mainprogram->mixwindows.push_back(mwin);
                        mwin->glc = SDL_GL_CreateContext(mwin->win);
                        SDL_GL_MakeCurrent(mwin->win, mwin->glc);

                        GLfloat vcoords1[8];
                        GLfloat *p = vcoords1;
                        *p++ = -1.0f;
                        *p++ = 1.0f;
                        *p++ = -1.0f;
                        *p++ = -1.0f;
                        *p++ = 1.0f;
                        *p++ = 1.0f;
                        *p++ = 1.0f;
                        *p++ = -1.0f;
                        GLfloat tcoords[] = {0.0f, 0.0f,
                                             0.0f, 1.0f,
                                             1.0f, 0.0f,
                                             1.0f, 1.0f};
                        glGenBuffers(1, &mwin->vbuf);
                        glBindBuffer(GL_ARRAY_BUFFER, mwin->vbuf);
                        glBufferData(GL_ARRAY_BUFFER, 32, vcoords1, GL_DYNAMIC_DRAW);
                        glGenBuffers(1, &mwin->tbuf);
                        glBindBuffer(GL_ARRAY_BUFFER, mwin->tbuf);
                        glBufferData(GL_ARRAY_BUFFER, 32, tcoords, GL_DYNAMIC_DRAW);
                        glGenVertexArrays(1, &mwin->vao);
                        glBindVertexArray(mwin->vao);
                        glBindBuffer(GL_ARRAY_BUFFER, mwin->vbuf);
                        glEnableVertexAttribArray(0);
                        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
                        glBindBuffer(GL_ARRAY_BUFFER, mwin->tbuf);
                        glEnableVertexAttribArray(1);
                        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
                        //SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 0);

                        glUseProgram(mainprogram->ShaderProgram);

                        std::thread vidoutput(output_video, mwin);
                        vidoutput.detach();

                        SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
                    }
                }
            }
        }
#ifdef POSIX
        else if (k == 4) {
            // start up v4l2 loopback device
            std::string device = mainprogram->loopbackmenu->entries[mainprogram->menuresults[0]];
            device = device.substr(2, device.size() - 2);
            this->v4l2_start_device(device, tex);
        }
#endif
    }
    if (mainprogram->menuchosen) {
        mainprogram->menuchosen = false;
        mainprogram->menuactivation = 0;
        mainprogram->menuresults.clear();
        mainprogram->recundo = true;
    }
}

void Program::handle_bintargetmenu() {
    int k = -1;
    // handle bintargetmenu
    k = mainprogram->handle_menu(mainprogram->bintargetmenu);
    if (k > -1) {
        binsmain->floating = true;
        mainprogram->binsscreen = false;
        k++;
        binsmain->screen = k;
        SDL_Rect rc1;
        SDL_GetDisplayBounds(k, &rc1);
        SDL_Rect rc2;
        SDL_GetDisplayUsableBounds(k, &rc2);
        int w = rc2.w;
        int h = rc2.h;
        SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
        SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
        binsmain->win = SDL_CreateWindow(PROGRAM_NAME, rc1.x, rc1.y, w, h, SDL_WINDOW_OPENGL | SDL_WINDOW_MAXIMIZED | SDL_WINDOW_RESIZABLE |
                                                                                   SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_ALLOW_HIGHDPI);
        binsmain->glc = SDL_GL_CreateContext(binsmain->win);
        SDL_GL_MakeCurrent(binsmain->win, binsmain->glc);
        int wi, he;
        SDL_GL_GetDrawableSize(binsmain->win, &wi, &he);
        binsmain->globw = (float) wi;
        binsmain->globh = (float) he;

        glUseProgram(mainprogram->ShaderProgram);

        glGenBuffers(1, &mainprogram->binvbuf);
        glGenBuffers(1, &mainprogram->bintbuf);
        glGenVertexArrays(1, &mainprogram->binvao);
        glBindVertexArray(mainprogram->binvao);
        glBindBuffer(GL_ARRAY_BUFFER, mainprogram->binvbuf);
        glBufferData(GL_ARRAY_BUFFER, 48, nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12, nullptr);
        glBindBuffer(GL_ARRAY_BUFFER, mainprogram->bintbuf);
        glBufferData(GL_ARRAY_BUFFER, 32, nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8, nullptr);

        std::thread binwin(handle_binwin);
        binwin.detach();

        SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
    }

    if (mainprogram->menuchosen) {
        mainprogram->menuchosen = false;
        mainprogram->menuactivation = 0;
        mainprogram->menuresults.clear();
        mainprogram->recundo = true;
    }
}

void Program::handle_wipemenu() {
	int k = -1;
	// Draw and Program::handle wipemenu
	k = mainprogram->handle_menu(mainprogram->wipemenu);
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
        mainprogram->recundo = true;
	}
}

void Program::handle_laymenu1() {
    GLuint tex;
	int k = -1;
	// Draw and Program::handle mainprogram->laymenu1 (with clone layer) and laymenu2 (without)
    bool cond = (mainprogram->laymenu2->state == 2);
	if (mainprogram->laymenu1->state > 1 || mainprogram->laymenu2->state > 1 || mainprogram->newlaymenu->state > 1 || mainprogram->clipmenu->state > 1) {
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

    if (mainprogram->laymenu1->entries.back() == "Send to v4l2 loopback device") {
        mainprogram->laymenu1->entries.pop_back();
        mainprogram->laymenu1->entries.pop_back();
    }

    bool encode = false;
	if (mainprogram->laymenu1->state > 1) {
        if ((mainmix->mouselayer->vidformat == 188 || mainmix->mouselayer->vidformat == 187) ||
        mainmix->mouselayer->filename == "" || mainmix->mouselayer->type == ELEM_IMAGE || mainmix->mouselayer->type
        == ELEM_LIVE) {
            if (mainprogram->laymenu1->entries.back() == "HAP encode on-the-fly") {
                mainprogram->laymenu1->entries.pop_back();
            }
            encode = false;
        }
        else {
            if (mainprogram->laymenu1->entries.back() != "HAP encode on-the-fly") {
                mainprogram->laymenu1->entries.push_back("HAP encode on-the-fly");
            }
            encode = true;
        }

        mainprogram->monitormenu->value = 4;
        mainprogram->make_mixtargetmenu();

        tex = mainmix->mouselayer->fbotex;
#ifdef POSIX
        this->register_v4l2lbdevices(mainprogram->laymenu1->entries, tex);
#endif
        k = mainprogram->handle_menu(mainprogram->laymenu1);
	}
	else if (mainprogram->laymenu2->state > 1) {
        mainprogram->monitormenu->value = 4;
        mainprogram->make_mixtargetmenu();

        k = mainprogram->handle_menu(mainprogram->laymenu2);
		if (k > 10) k++;
	}


    std::vector<OutputEntry*> currentries;
    std::vector<OutputEntry*> takenentries;
    for (int i = 0; i < mainprogram->outputentries.size(); i++) {
        bool cond;
        if (mainprogram->outputentries[i]->win->mixid == 4) {
            cond = (mainprogram->outputentries[i]->win->lay == mainmix->mouselayer);
        }
        else {
            cond = (mainprogram->outputentries[i]->win->mixid == mainprogram->monitormenu->value);
        }
        if (cond) {
            currentries.push_back(mainprogram->outputentries[i]);
        } else {
            takenentries.push_back(mainprogram->outputentries[i]);
        }
    }


	if (k > -1) {
		if (k == 0) {
			if (mainprogram->menuresults.size()) {
				if (mainprogram->menuresults[0] > 0) {
#ifdef WINDOWS
					std::string livename = "video=" + mainprogram->devices[mainprogram->menuresults[0]];
#else
#ifdef POSIX
                    std::string livename;
                    livename = mainprogram->devvideomap[mainprogram->livemenu->entries[mainprogram->menuresults[0]]];
#endif
#endif
					mainmix->mouselayer->set_live_base(livename);
				}
			}
		}
        if (k == 1) {
            mainprogram->pathto = "OPENFILESLAYER";
            mainprogram->loadlay = mainmix->mouselayer;
            if (cond) {
                //mainprogram->clickednextto = mainmix->mouselayer->deck;
            }
            mainmix->addlay = false;
            std::thread filereq(&Program::get_multinname, mainprogram, "Open video/image/layer file", "", std::filesystem::canonical(mainprogram->currfilesdir).generic_string());
            filereq.detach();
        }
        if (k == 2) {
            mainprogram->pathto = "OPENFILESQUEUE";
            mainprogram->loadlay = mainmix->mouselayer;
            std::thread filereq(&Program::get_multinname, mainprogram, "Open video/image/layer file", "", std::filesystem::canonical(mainprogram->currfilesdir).generic_string());
            filereq.detach();
        }
		if (k == 3 && !cond) {
			mainprogram->pathto = "OPENFILESSTACK";
			mainprogram->loadlay = mainmix->mouselayer;
            mainmix->addlay = false;
			std::thread filereq(&Program::get_multinname, mainprogram, "Open video/image/layer file", "", std::filesystem::canonical(mainprogram->currelemsdir).generic_string());
			filereq.detach();
		}
		else if (k == 4 - cond) {
			mainprogram->pathto = "SAVELAYFILE";
			std::thread filereq(&Program::get_outname, mainprogram, "Save layer file", "application/ewocvj2-layer", std::filesystem::canonical(mainprogram->currelemsdir).generic_string());
			filereq.detach();
		}
		else if (k == 5 - cond) {
			mainmix->new_file(mainmix->mousedeck, 1, true);
		}
		else if (k == 6 - cond) {
			mainprogram->pathto = "OPENDECK";
			std::thread filereq(&Program::get_inname, mainprogram, "Open deck file", "application/ewocvj2-deck", std::filesystem::canonical(mainprogram->currelemsdir).generic_string());
			filereq.detach();
		}
		else if (k == 7 - cond) {
			mainprogram->pathto = "SAVEDECK";
			std::thread filereq(&Program::get_outname, mainprogram, "Save deck file", "application/ewocvj2-deck", std::filesystem::canonical(mainprogram->currelemsdir).generic_string());
			filereq.detach();
		}
		else if (k == 8 - cond) {
			mainmix->new_file(2, 1, true);
		}
		else if (k == 9 - cond) {
			mainprogram->pathto = "OPENMIX";
			std::thread filereq(&Program::get_inname, mainprogram, "Open mix file", "application/ewocvj2-mix", std::filesystem::canonical(mainprogram->currelemsdir).generic_string());
			filereq.detach();
		}
		else if (k == 10 - cond) {
			mainprogram->pathto = "SAVEMIX";
			std::thread filereq(&Program::get_outname, mainprogram, "Save mix file", "application/ewocvj2-mix",
                       std::filesystem::canonical(mainprogram->currelemsdir).generic_string());
			filereq.detach();
		}
		else if (k == 11 - cond) {
			if (std::find(mainmix->layers[0].begin(), mainmix->layers[0].end(), mainmix->mouselayer) != mainmix->layers[0].end()) {
				mainmix->delete_layer(mainmix->layers[0], mainmix->mouselayer, true);
			}
			else if (std::find(mainmix->layers[1].begin(), mainmix->layers[1].end(), mainmix->mouselayer) != mainmix->layers[1].end()) {
				mainmix->delete_layer(mainmix->layers[1], mainmix->mouselayer, true);
			}
			else if (std::find(mainmix->layers[2].begin(), mainmix->layers[2].end(), mainmix->mouselayer) != mainmix->layers[2].end()) {
				mainmix->delete_layer(mainmix->layers[2], mainmix->mouselayer, true);
			}
			else if (std::find(mainmix->layers[3].begin(), mainmix->layers[3].end(), mainmix->mouselayer) != mainmix->layers[3].end()) {
				mainmix->delete_layer(mainmix->layers[3], mainmix->mouselayer, true);
			}
		}
        else if (!cond && k == 12) {
            Layer* duplay = mainmix->mouselayer->clone();
            duplay->isclone = false;
            duplay->isduplay = mainmix->mouselayer;
        }
        else if (!cond && k == 13) {
            Layer* clonelay = mainmix->mouselayer->clone();
            if (mainmix->mouselayer->clonesetnr == -1) {
                mainmix->mouselayer->clonesetnr = mainmix->clonesets.size();
                //mainmix->firstlayers[mainmix->mouselayer->clonesetnr] = mainmix->mouselayer;      set in Layer::load_frame()
                std::unordered_set<Layer*> *uset = new std::unordered_set<Layer*>;
                mainmix->clonesets[mainmix->mouselayer->clonesetnr] = uset;
                uset->emplace(mainmix->mouselayer);
            }
            clonelay->clonesetnr = mainmix->mouselayer->clonesetnr;
            mainmix->clonesets[mainmix->mouselayer->clonesetnr]->emplace(clonelay);
        }
		else if (k == (14 - cond * 2)) {
			mainmix->mouselayer->shiftx->value = 0.0f;
			mainmix->mouselayer->shifty->value = 0.0f;
		}
		else if (k == 15 - cond * 2) {
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
        else if (k == 16 - cond * 2) {
            // chosen output screen already used? re-use window
            int currdisp = SDL_GetWindowDisplayIndex(mainprogram->mainwindow);
            int disp = mainprogram->menuresults[0] + (mainprogram->menuresults[0] >= currdisp);
            bool switched = false;
            for (int i = 0; i < takenentries.size(); i++) {
                if (takenentries[i]->screen == disp) {
                    takenentries[i]->win->mixid = 4;
                    takenentries[i]->win->lay = mainmix->mouselayer;
                    switched = true;
                }
            }
            if (switched) {}
            else {
                int swoff = -1;
                for (int i = 0; i < currentries.size(); i++) {
                    if (currentries[i]->screen == disp) {
                        swoff = i;
                        break;
                    }
                }

                // stop output display of chosen mixwindow on chosen screen
                if (swoff != -1) {
                    SDL_DestroyWindow(currentries[swoff]->win->win);
                    mainprogram->outputentries.erase(
                            std::find(mainprogram->outputentries.begin(), mainprogram->outputentries.end(),
                                      currentries[swoff]));
                    delete currentries[swoff]->win;
                } else {
                    // open new window on chosen output screen and start display thread (synced at end of the_loop)
                    EWindow *mwin = new EWindow;
                    mwin->mixid = 4;
                    OutputEntry *entry = new OutputEntry;
                    entry->screen = disp;
                    entry->win = mwin;
                    mwin->lay = mainmix->mouselayer;
                    mainprogram->outputentries.push_back(entry);
                    SDL_Rect rc1;
                    SDL_GetDisplayBounds(disp, &rc1);
                    SDL_Rect rc2;
                    SDL_GetDisplayUsableBounds(disp, &rc2);
                    mwin->w = rc2.w;
                    mwin->h = rc2.h;
                    SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
                    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
                    mwin->win = SDL_CreateWindow(PROGRAM_NAME, rc1.x, rc1.y, mwin->w, mwin->h,
                                                 SDL_WINDOW_OPENGL | SDL_WINDOW_MAXIMIZED | SDL_WINDOW_RESIZABLE |
                                                 SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_ALLOW_HIGHDPI);
                    //SDL_RaiseWindow(mainprogram->mainwindow);
                    mainprogram->mixwindows.push_back(mwin);
                    mwin->glc = SDL_GL_CreateContext(mwin->win);
                    SDL_GL_MakeCurrent(mwin->win, mwin->glc);

                    GLfloat vcoords1[8];
                    GLfloat *p = vcoords1;
                    *p++ = -1.0f;
                    *p++ = 1.0f;
                    *p++ = -1.0f;
                    *p++ = -1.0f;
                    *p++ = 1.0f;
                    *p++ = 1.0f;
                    *p++ = 1.0f;
                    *p++ = -1.0f;
                    GLfloat tcoords[] = {0.0f, 0.0f,
                                         0.0f, 1.0f,
                                         1.0f, 0.0f,
                                         1.0f, 1.0f};
                    glGenBuffers(1, &mwin->vbuf);
                    glBindBuffer(GL_ARRAY_BUFFER, mwin->vbuf);
                    glBufferData(GL_ARRAY_BUFFER, 32, vcoords1, GL_DYNAMIC_DRAW);
                    glGenBuffers(1, &mwin->tbuf);
                    glBindBuffer(GL_ARRAY_BUFFER, mwin->tbuf);
                    glBufferData(GL_ARRAY_BUFFER, 32, tcoords, GL_DYNAMIC_DRAW);
                    glGenVertexArrays(1, &mwin->vao);
                    glBindVertexArray(mwin->vao);
                    glBindBuffer(GL_ARRAY_BUFFER, mwin->vbuf);
                    glEnableVertexAttribArray(0);
                    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
                    glBindBuffer(GL_ARRAY_BUFFER, mwin->tbuf);
                    glEnableVertexAttribArray(1);
                    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
                    //SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 0);

                    glUseProgram(mainprogram->ShaderProgram);

                    std::thread vidoutput(output_video, mwin);
                    vidoutput.detach();

                    SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
                }
            }
        }
        else if ((!cond && k == 17) || k == 17 - cond * 2) {
            if (mainmix->mouselayer->clips.size() == 1) {
                if (!mainmix->recording[0]) {
                    // start recording layer with all effects, settings,... and replace with recorded video
                    mainmix->reclay = mainmix->mouselayer;
                    mainmix->reccodec = "hap";
                    mainmix->reckind = 0;
                    mainmix->recrep = true;
                    mainmix->start_recording();
                } else {
                    mainmix->recording[0] = false;
                }
            }
        }
        else if (!cond && k == 18 && encode) {
            BinElement *binel = new BinElement;
            binel->bin = nullptr;
            binel->type = ELEM_FILE;
            binel->path = mainmix->mouselayer->filename;
            binel->relpath = std::filesystem::relative(mainmix->mouselayer->filename, mainprogram->project->binsdir).generic_string();
            if (mainmix->mouselayer->isclone) {
                mainmix->mouselayer = mainmix->firstlayers[mainmix->mouselayer->clonesetnr];
            }
            binel->otflay = mainmix->mouselayer;
            mainmix->mouselayer->hapbinel = binel;
            binsmain->hap_binel(binel, nullptr);
        }
#ifdef POSIX
        else if (!cond && k == (18 + encode)) {
            // start up v4l2 loopback device
            std::string device = mainprogram->loopbackmenu->entries[mainprogram->menuresults[0]];
            device = device.substr(2, device.size() - 2);
            this->v4l2_start_device(device, tex);
        }
#endif
	}

	if (mainprogram->menuchosen) {
		mainprogram->menuchosen = false;
		mainprogram->menuactivation = 0;
		mainprogram->menuresults.clear();
        mainprogram->recundo = true;
	}
}

void Program::handle_newlaymenu() {
	int k = -1;
	k = mainprogram->handle_menu(mainprogram->newlaymenu);
	if (k > -1) {
		std::vector<Layer*>& lvec = choose_layers(mainmix->mousedeck);
		if (k == 0) {
			if (mainprogram->menuresults.size()) {
				if (mainprogram->menuresults[0] > 0) {
					mainmix->mouselayer = mainmix->add_layer(lvec, lvec.size());
#ifdef WINDOWS
					std::string livename = "video=" + mainprogram->devices[mainprogram->menuresults[0]];
#else
#ifdef POSIX
                    std::string livename;
                    livename = mainprogram->devvideomap[mainprogram->livemenu->entries[mainprogram->menuresults[0]]];
#endif
#endif
					mainmix->mouselayer->set_live_base(livename);
				}
			}
		}
		 if (k == 1) {
			mainprogram->pathto = "OPENFILESSTACK";
			mainmix->addlay = true;
			mainmix->mouselayer = nullptr;
			std::thread filereq(&Program::get_multinname, mainprogram, "Open video/image/layer file", "", std::filesystem::canonical(mainprogram->currfilesdir).generic_string());
			filereq.detach();
		}
		else if (k == 2) {
			mainmix->new_file(mainmix->mousedeck, 1, true);
		}
		else if (k == 3) {
			mainprogram->pathto = "OPENDECK";
			std::thread filereq(&Program::get_inname, mainprogram, "Open deck file", "application/ewocvj2-deck", std::filesystem::canonical(mainprogram->currelemsdir).generic_string());
			filereq.detach();
		}
		else if (k == 4) {
			mainprogram->pathto = "SAVEDECK";
			std::thread filereq(&Program::get_outname, mainprogram, "Save deck file", "application/ewocvj2-deck", std::filesystem::canonical(mainprogram->currelemsdir).generic_string());
			filereq.detach();
		}
		else if (k == 5) {
			mainmix->new_file(2, 1, true);
		}
		else if (k == 6) {
			mainprogram->pathto = "OPENMIX";
			std::thread filereq(&Program::get_inname, mainprogram, "Open mix file", "application/ewocvj2-mix", std::filesystem::canonical(mainprogram->currelemsdir).generic_string());
			filereq.detach();
		}
		else if (k == 7) {
			mainprogram->pathto = "SAVEMIX";
			std::thread filereq(&Program::get_outname, mainprogram, "Save mix file", "application/ewocvj2-mix", std::filesystem::canonical(mainprogram->currelemsdir).generic_string());
			filereq.detach();
		}
	}


	if (mainprogram->menuchosen) {
		mainprogram->menuchosen = false;
		mainprogram->menuactivation = 0;
		mainprogram->menuresults.clear();
        mainprogram->recundo = true;
	}
}

void Program::handle_clipmenu() {
	int k = -1;
	// Draw and Program::handle clipmenu
	k = mainprogram->handle_menu(mainprogram->clipmenu);
	if (k > -1) {
		std::vector<Layer*>& lvec = choose_layers(mainmix->mousedeck);
		if (k == 0) {
			// get_cameras() is done in handle_laymenu1()
			if (mainprogram->menuresults.size()) {
				if (mainprogram->menuresults[0] > 0) {
#ifdef WINDOWS
					std::string livename = "video=" + mainprogram->devices[mainprogram->menuresults[0]];
#else
#ifdef POSIX
					std::string livename = "/dev/video" + std::to_string(mainprogram->menuresults[0] - 1);
#endif
#endif
					int pos = std::find(mainmix->mouselayer->clips.begin(), mainmix->mouselayer->clips.end(), mainmix->mouseclip) - mainmix->mouselayer->clips.begin();
					if (pos == mainmix->mouselayer->clips.size() - 1) {
						Clip* clip = new Clip;
						if (mainmix->mouselayer->clips.size() > 4) mainmix->mouselayer->queuescroll++;
						mainmix->mouseclip = clip;
						clip->insert(mainmix->mouselayer, mainmix->mouselayer->clips.end() - 1);
					}
					mainmix->mouseclip->path = livename;
					mainmix->mouseclip->type = ELEM_LIVE;
				}
			}
		}
		if (k == 1) {
			mainprogram->pathto = "OPENFILESCLIP";
			std::thread filereq(&Program::get_multinname, mainprogram, "Open clip video file", "", std::filesystem::canonical(mainprogram->currfilesdir).generic_string());
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
            mainprogram->recundo = true;
		}
	}
}

void Program::handle_mainmenu() {
	int k = -1;
	// Draw and Program::handle mainmenu
	k = this->handle_menu(this->mainmenu);
	if (k == 0) {
		this->pathto = "NEWPROJECT";
		std::string reqdir = this->projdir;
		if (reqdir.substr(0, 1) == ".") reqdir.erase(0, 2);
		std::string path;
        std::string name = "Untitled_0";
        int count = 0;
        while (1) {
            path = this->projdir + name;
            if (!exists(path)) {
                break;
            }
            count++;
            name = remove_version(name) + "_" + std::to_string(count);
        }
		std::thread filereq(&Program::get_outname, this, "New project", "application/ewocvj2-project", std::filesystem::absolute(reqdir + name).generic_string());
		filereq.detach();
	}
	else if (k == 1) {
		this->pathto = "OPENPROJECT";
        std::thread filereq(&Program::get_inname, this, "Open project", "application/ewocvj2-project", std::filesystem::canonical(this->currprojdir).generic_string());
        filereq.detach();
	}
	else if (k == 2) {
        if (this->project->path.find("autosave") != std::string::npos) {
            this->path = this->project->bupp;
            this->pathto = "SAVEPROJECT";
        }
        else {
            this->project->save(this->project->path);
        }
	}
	else if (k == 3) {
		this->pathto = "SAVEPROJECT";
        std::string path = find_unused_filename(basename(this->project->path), this->currprojdir, "");
		std::thread filereq(&Program::get_outname, this, "Save project file", "", path);
		filereq.detach();
	}
    else if (k == 4) {
        // load autosave
        this->pathto = "OPENAUTOSAVE";
        std::thread filereq(&Program::get_inname, this, "Open autosaved project", "application/ewocvj2-project", std::filesystem::canonical(this->project->autosavedir).generic_string());
        filereq.detach();
    }
	else if (k == 5) {
		mainmix->new_state();
	}
     else if (k == 6) {
		if (!this->prefon) {
			this->prefon = true;

            std::string tt = "EWOCvj2 Preferences";
            std::thread tofront = std::thread{&Program::postponed_to_front_win, this, tt, this->prefwindow};
            tofront.detach();

            for (int i = 0; i < this->prefs->items.size(); i++) {
				PrefCat* item = this->prefs->items[i];
				if (item->name != "Invisible") item->box->upvtxtoscr();
			}
		}
		else {
			SDL_RaiseWindow(this->prefwindow);
		}
	}
	else if (k == 7) {
		if (!this->midipresets) {
			this->midipresets = true;

            std::string tt = "Tune MIDI";
            std::thread tofront = std::thread{&Program::postponed_to_front_win, this, tt, this->config_midipresetswindow};
            tofront.detach();

            this->tmcat[0]->upvtxtoscr();
            this->tmcat[1]->upvtxtoscr();
            this->tmcat[2]->upvtxtoscr();
            this->tmset[0]->upvtxtoscr();
            this->tmset[1]->upvtxtoscr();
            this->tmset[2]->upvtxtoscr();
            this->tmset[3]->upvtxtoscr();
            this->tmscratch1->upvtxtoscr();
            this->tmscratch2->upvtxtoscr();
			this->tmfreeze->upvtxtoscr();
			this->tmplay->upvtxtoscr();
			this->tmbackw->upvtxtoscr();
			this->tmbounce->upvtxtoscr();
			this->tmfrforw->upvtxtoscr();
			this->tmfrbackw->upvtxtoscr();
			this->tmspeed->upvtxtoscr();
			this->tmspeedzero->upvtxtoscr();
            this->tmopacity->upvtxtoscr();
            this->tmcross->upvtxtoscr();
		}
		else {
			SDL_RaiseWindow(this->config_midipresetswindow);
		}
	}
	else if (k == 8) {
		this->quitting = "quitted";
	}

	if (this->menuchosen) {
		this->menuchosen = false;
		this->menuactivation = 0;
		this->menuresults.clear();
        mainprogram->recundo = true;
	}
}

void Program::handle_shelfmenu() {
	int k = -1;
	// Draw and Program::handle shelfmenu
	k = mainprogram->handle_menu(mainprogram->shelfmenu);
	if (k == 0) {
	    // open file(s) into shelf
        mainprogram->pathto = "OPENFILESSHELF";
        std::thread filereq(&Program::get_multinname, mainprogram, "Load file(s) in shelf", "", std::filesystem::canonical(mainprogram->currfilesdir).generic_string());
        filereq.detach();
    }
    else if (k == 1) {
        // new shelf
        mainmix->mouseshelf->erase();
        }
	else if (k == 2) {
		mainprogram->pathto = "OPENSHELF";
		std::thread filereq(&Program::get_inname, mainprogram, "Open shelf file", "application/ewocvj2-shelf", std::filesystem::canonical(mainprogram->currshelfdir).generic_string());
		filereq.detach();
	}
	else if (k == 3) {
		mainprogram->pathto = "SAVESHELF";
		std::thread filereq(&Program::get_outname, mainprogram, "Save shelf file", "application/ewocvj2-shelf", std::filesystem::canonical(mainprogram->currshelfdir).generic_string());
		filereq.detach();
	}
	else if (k == 4 || k == 5) {
		// insert one of current decks into shelf element
		std::string path = find_unused_filename(basename(mainprogram->project->path), mainprogram->temppath, ".deck");
		mainmix->mousedeck = k - 4;
		mainmix->save_deck(path, false, true);
		mainmix->mouseshelf->insert_deck(path, k - 4, mainmix->mouseshelfelem);
	}
	else if (k == 6) {
		// insert current mix into shelf element
        std::string path = find_unused_filename(basename(mainprogram->project->path), mainprogram->temppath, ".mix");
		mainmix->save_mix(path, mainprogram->prevmodus, false);
		mainmix->mouseshelf->insert_mix(path, mainmix->mouseshelfelem);
	}
	else if (k == 7) {
		// insert entire shelf in a bin block
		binsmain->insertshelf = mainmix->mouseshelf;
		mainprogram->binsscreen = true;
	}
    else if (k == 8) {
        ShelfElement *elem = mainmix->mouseshelf->elements[mainmix->mouseshelfelem];
        elem->path = "";
        elem->type = ELEM_FILE;
        blacken(elem->tex);
        blacken(elem->oldtex);
        elem->button->midi[0] = -1;
        elem->button->midi[1] = -1;
        elem->button->midiport = "";
        elem->button->unregister_midi();
    }
    /*else if (k == 9) {
		// learn MIDI for element layer load
		mainmix->learn = true;
		mainmix->learnparam = nullptr;
		mainmix->learnbutton = mainmix->mouseshelf->buttons[mainmix->mouseshelfelem];
	}*/

	if (mainprogram->menuchosen) {
		mainprogram->menuchosen = false;
		mainprogram->menuactivation = 0;
		mainprogram->menuresults.clear();
        mainprogram->recundo = true;
	}
}

void Program::handle_filemenu() {
	int k = -1;
	// Draw and Program::handle filemenu
	if (mainprogram->filemenu->state > 1) {
		// make menus with layer numbers, one for each deck
		std::vector<Layer*>& lvec = choose_layers(0);
		std::vector<std::string> laylist1;
		for (int i = 0; i < lvec.size() + 1; i++) {
			laylist1.push_back("Layer slot " + std::to_string(i + 1));
		}
		mainprogram->make_menu("laylistmenu1", mainprogram->laylistmenu1, laylist1);
		std::vector<Layer*>& lvec2 = choose_layers(1);
		std::vector<std::string> laylist2;
		for (int i = 0; i < lvec2.size() + 1; i++) {
			laylist2.push_back("Layer slot " + std::to_string(i + 1));
		}
        mainprogram->make_menu("laylistmenu2", mainprogram->laylistmenu2, laylist2);
        laylist1.pop_back();
        mainprogram->make_menu("laylistmenu3", mainprogram->laylistmenu3, laylist1);
        laylist2.pop_back();
        mainprogram->make_menu("laylistmenu4", mainprogram->laylistmenu4, laylist2);
	}

	k = mainprogram->handle_menu(mainprogram->filemenu);
	if (k == 0) {
        if (mainprogram->menuresults.size()) {
            if (mainprogram->menuresults[0] == 0) {
                // new project
                mainprogram->pathto = "NEWPROJECT";
                std::string reqdir = mainprogram->projdir;
                if (reqdir.substr(0, 1) == ".") reqdir.erase(0, 2);
                std::string path;
                std::string name = "Untitled";
                int count = 0;
                while (1) {
                    path = mainprogram->projdir + name + ".ewocvj";
                    if (!exists(path)) {
                        break;
                    }
                    count++;
                    name = remove_version(name) + "_" + std::to_string(count);
                }
                std::thread filereq(&Program::get_outname, mainprogram, "New project", "application/ewocvj2-project",
                                    std::filesystem::absolute(reqdir + name).generic_string());
                filereq.detach();
            } else if (mainprogram->menuresults[0] == 1) {
                // new mix
                mainmix->new_file(2, 1, true);
            } else if (mainprogram->menuresults[0] == 2) {
                // new deck in deck A
                mainmix->new_file(0, 1, true);
            } else if (mainprogram->menuresults[0] == 3) {
                // open deck in deck B
                mainmix->new_file(1, 1, true);
            } else if (mainprogram->menuresults[0] == 4) {
                // open new layer in deck A
                std::vector<Layer *> &lvec = choose_layers(0);
                mainmix->add_layer(lvec, mainprogram->menuresults[1]);
            } else if (mainprogram->menuresults[0] == 5) {
                // open new layer in deck B
                std::vector<Layer *> &lvec = choose_layers(1);
                mainmix->add_layer(lvec, mainprogram->menuresults[1]);
            }
        }
	}
	else if (k == 1) {
        if (mainprogram->menuresults.size()) {
            if (mainprogram->menuresults[0] == 0) {
                mainprogram->pathto = "OPENPROJECT";
                std::thread filereq(&Program::get_inname, mainprogram, "Open project", "application/ewocvj2-project",
                                    std::filesystem::canonical(mainprogram->currprojdir).generic_string());
                filereq.detach();
            } else if (mainprogram->menuresults[0] == 1) {
                // open mix file
                mainprogram->pathto = "OPENMIX";
                std::thread filereq(&Program::get_inname, mainprogram, "Open mix file", "application/ewocvj2-mix",
                                    std::filesystem::canonical(mainprogram->currelemsdir).generic_string());
                filereq.detach();
            } else if (mainprogram->menuresults[0] == 2) {
                // open deck file in deck A
                mainmix->mousedeck = 0;
                mainprogram->pathto = "OPENDECK";
                std::thread filereq(&Program::get_inname, mainprogram, "Open deck file", "application/ewocvj2-deck",
                                    std::filesystem::canonical(mainprogram->currelemsdir).generic_string());
                filereq.detach();
            } else if (mainprogram->menuresults[0] == 3) {
                // open deck file in deck B
                mainmix->mousedeck = 1;
                mainprogram->pathto = "OPENDECK";
                std::thread filereq(&Program::get_inname, mainprogram, "Open deck file", "application/ewocvj2-deck",
                                    std::filesystem::canonical(mainprogram->currelemsdir).generic_string());
                filereq.detach();
            } else if (mainprogram->menuresults[0] == 4) {
                // open files in deck A
                std::vector<Layer *> &lvec = choose_layers(0);
                mainmix->mousedeck = 0;
                mainprogram->pathto = "OPENFILESLAYER";
                if (mainprogram->menuresults[1] == lvec.size()) {
                    mainmix->addlay = true;
                } else {
                    mainprogram->loadlay = lvec[mainprogram->menuresults[1]];
                }
                std::thread filereq(&Program::get_multinname, mainprogram, "Open video/image/layer file", "",
                                    std::filesystem::canonical(mainprogram->currfilesdir).generic_string());
                filereq.detach();
            } else if (mainprogram->menuresults[0] == 5) {
                // open files in layer in deck B
                std::vector<Layer *> &lvec = choose_layers(1);
                mainmix->mousedeck = 1;
                mainprogram->pathto = "OPENFILESLAYER";
                if (mainprogram->menuresults[1] == lvec.size()) {
                    mainmix->addlay = true;
                } else {
                    mainprogram->loadlay = lvec[mainprogram->menuresults[1]];
                }
                std::thread filereq(&Program::get_multinname, mainprogram, "Open video/image/layer file", "",
                                    std::filesystem::canonical(mainprogram->currfilesdir).generic_string());
                filereq.detach();
            } else if (mainprogram->menuresults[0] == 6) {
                // open files in in deck A
                std::vector<Layer *> &lvec = choose_layers(0);
                mainmix->mousedeck = 0;
                mainprogram->pathto = "OPENFILESQUEUE";
                std::thread filereq(&Program::get_multinname, mainprogram, "Open video/image/layer file", "",
                                    std::filesystem::canonical(mainprogram->currfilesdir).generic_string());
                filereq.detach();
                if (mainprogram->menuresults[1] == lvec.size()) {
                    mainmix->addlay = true;
                } else {
                    mainprogram->loadlay = lvec[mainprogram->menuresults[1]];
                }
            } else if (mainprogram->menuresults[0] == 7) {
                // open files in layer in deck B
                std::vector<Layer *> &lvec = choose_layers(1);
                mainmix->mousedeck = 1;
                mainprogram->pathto = "OPENFILESQUEUE";
                std::thread filereq(&Program::get_multinname, mainprogram, "Open video/image/layer file", "",
                                    std::filesystem::canonical(mainprogram->currfilesdir).generic_string());
                filereq.detach();
                if (mainprogram->menuresults[1] == lvec.size()) {
                    mainmix->addlay = true;
                } else {
                    mainprogram->loadlay = lvec[mainprogram->menuresults[1]];
                }
            }
        }
	}
	else if (k == 2) {
        if (mainprogram->menuresults.size()) {
            if (mainprogram->menuresults[0] == 0) {
                mainprogram->pathto = "SAVEPROJECT";
                std::string path = find_unused_filename(basename(mainprogram->project->path), mainprogram->currprojdir,
                                                        "");
                std::thread filereq(&Program::get_outname, mainprogram, "Save project file",
                                    "application/ewocvj2-project", path);
                filereq.detach();
            } else if (mainprogram->menuresults[0] == 1) {
                mainprogram->pathto = "SAVEMIX";
                std::thread filereq(&Program::get_outname, mainprogram, "Open mix file", "application/ewocvj2-mix",
                                    std::filesystem::canonical(mainprogram->currelemsdir).generic_string());
                filereq.detach();
            } else if (mainprogram->menuresults[0] == 2) {
                mainmix->mousedeck = 0;
                mainprogram->pathto = "SAVEDECK";
                std::thread filereq(&Program::get_outname, mainprogram, "Save deck file", "application/ewocvj2-deck",
                                    std::filesystem::canonical(mainprogram->currelemsdir).generic_string());
                filereq.detach();
            } else if (mainprogram->menuresults[0] == 3) {
                mainmix->mousedeck = 1;
                mainprogram->pathto = "SAVEDECK";
                std::thread filereq(&Program::get_outname, mainprogram, "Save deck file", "application/ewocvj2-deck",
                                    std::filesystem::canonical(mainprogram->currelemsdir).generic_string());
                filereq.detach();
            } else if (mainprogram->menuresults[0] == 4) {
                // save layer from deck A
                std::vector<Layer *> &lvec = choose_layers(0);
                mainmix->mouselayer = lvec[mainprogram->menuresults[1]];
                mainprogram->pathto = "SAVELAYFILE";
                std::thread filereq(&Program::get_outname, mainprogram, "Save layer file", "application/ewocvj2-layer",
                                    std::filesystem::canonical(mainprogram->currelemsdir).generic_string());
                filereq.detach();
            } else if (mainprogram->menuresults[0] == 5) {
                // save layer from deck B
                std::vector<Layer *> &lvec = choose_layers(1);
                mainmix->mouselayer = lvec[mainprogram->menuresults[1]];
                mainprogram->pathto = "SAVELAYFILE";
                std::thread filereq(&Program::get_outname, mainprogram, "Save layer file", "application/ewocvj2-layer",
                                    std::filesystem::canonical(mainprogram->currelemsdir).generic_string());
                filereq.detach();
            }
        }
	}
    else if (k == 3) {
        if (this->project->path.find("autosave") != std::string::npos) {
            this->path = this->project->bupp;
            this->pathto = "SAVEPROJECT";
        }
        else {
            this->project->save(this->project->path);
        }
    }
    else if (k == 4) {
        mainprogram->quitting = "quitted";
    }

	if (mainprogram->menuchosen) {
		mainprogram->menuchosen = false;
		mainprogram->menuactivation = 0;
		mainprogram->menuresults.clear();
        mainprogram->recundo = true;
	}
}

void Program::handle_editmenu() {
    int k = -1;
    // Draw and Program::handle editmenu
    k = mainprogram->handle_menu(mainprogram->editmenu);
    if (k == 0) {
        if (!this->prefon) {
            this->prefon = true;

            std::string tt = "EWOCvj2 Preferences";
            std::thread tofront = std::thread{&Program::postponed_to_front_win, this, tt, this->prefwindow};
            tofront.detach();

            for (int i = 0; i < this->prefs->items.size(); i++) {
                PrefCat* item = this->prefs->items[i];
                if (item->name != "Invisible") item->box->upvtxtoscr();
            }
        }
        else {
            SDL_RaiseWindow(this->prefwindow);
        }
    }
    else if (k == 1) {
        if (!mainprogram->midipresets) {

            std::string tt = "Tune MIDI";
            std::thread tofront = std::thread{&Program::postponed_to_front_win, this, tt, mainprogram->config_midipresetswindow};
            tofront.detach();

            mainprogram->tmcat[0]->upvtxtoscr();
            mainprogram->tmcat[1]->upvtxtoscr();
            mainprogram->tmcat[2]->upvtxtoscr();
            mainprogram->tmset[0]->upvtxtoscr();
            mainprogram->tmset[1]->upvtxtoscr();
            mainprogram->tmset[2]->upvtxtoscr();
            mainprogram->tmset[3]->upvtxtoscr();
            mainprogram->tmscratch1->upvtxtoscr();
            mainprogram->tmscratch2->upvtxtoscr();
            mainprogram->tmfreeze->upvtxtoscr();
            mainprogram->tmscrinvert->upvtxtoscr();
            mainprogram->tmplay->upvtxtoscr();
            mainprogram->tmbackw->upvtxtoscr();
            mainprogram->tmbounce->upvtxtoscr();
            mainprogram->tmfrforw->upvtxtoscr();
            mainprogram->tmfrbackw->upvtxtoscr();
            mainprogram->tmspeed->upvtxtoscr();
            mainprogram->tmspeedzero->upvtxtoscr();
            mainprogram->tmopacity->upvtxtoscr();
            mainprogram->tmcross->upvtxtoscr();

            mainprogram->midipresets = true;
        }
        else {
            SDL_RaiseWindow(mainprogram->config_midipresetswindow);
        }
    }

    if (mainprogram->menuchosen) {
        mainprogram->menuchosen = false;
        mainprogram->menuactivation = 0;
        mainprogram->menuresults.clear();
        mainprogram->recundo = true;
    }
}

void Program::handle_lpstmenu() {
    if (!mainmix->mouselpstelem) return;
    if (mainmix->mouselpstelem->eventlist.size() == 0) return;
    int k = -1;
    // Draw and handle lpstmenu
    k = mainprogram->handle_menu(mainprogram->lpstmenu);
    if (k == 0) {
        mainmix->mouselpstelem->erase_elem();
    }
    if (k == 1) {
        mainmix->cbduration = mainmix->mouselpstelem->totaltime;
    }
    else if (k == 2) {
        float buspeed = mainmix->mouselpstelem->speed->value;
        mainmix->mouselpstelem->speed->value = mainmix->mouselpstelem->totaltime / mainmix->cbduration;
        mainmix->mouselpstelem->speedadaptedtime *= buspeed / mainmix->mouselpstelem->speed->value;
        mainmix->mouselpstelem->interimtime *= buspeed / mainmix->mouselpstelem->speed->value;
    }
    else if (k == 3) {
        mainmix->mouselpstelem->totaltime = mainmix->cbduration;
    }

    if (mainprogram->menuchosen) {
        mainprogram->menuchosen = false;
        mainprogram->menuactivation = 0;
        mainprogram->menuresults.clear();
        mainprogram->recundo = true;
    }
}

// end of menu code



void Program::preview_modus_buttons() {
	// Draw and handle buttons
    for (Layer *lay : mainmix->layers[0]) {
        if (lay->changeinit < 1 && lay->filename != "" && !lay->isclone) return;
    }
    for (Layer *lay : mainmix->layers[1]) {
        if (lay->changeinit < 1 && lay->filename != "" && !lay->isclone) return;
    }
    for (Layer *lay : mainmix->layers[2]) {
        if (lay->changeinit < 1 && lay->filename != "" && !lay->isclone) {
            return;
        }
    }
    for (Layer *lay : mainmix->layers[3]) {
        if (lay->changeinit < 1 && lay->filename != "" && !lay->isclone) return;
    }
	if (mainprogram->prevmodus) {
        mainprogram->handle_button(mainprogram->toscreenA, false, false, true);
        if (mainprogram->toscreenA->toggled()) {
            mainprogram->toscreenA->value = 0;
            mainprogram->toscreenA->oldvalue = 0;
            // SEND UP button copies preview set entirely to comp set
            mainmix->copy_to_comp(true, false, true);
        }
        Boxx *box = mainprogram->toscreenA->box;
        register_triangle_draw(white, white, box->vtxcoords->x1 + box->vtxcoords->w / 2.0f + 0.0117f,
                               box->vtxcoords->y1 + 0.0225f, 0.0165f, 0.0312f, DOWN, CLOSED);
        render_text(mainprogram->toscreenA->name[0], white, box->vtxcoords->x1 + 0.0117f, box->vtxcoords->y1 + 0.0225f,
                    0.0006f, 0.001f);

        mainprogram->handle_button(mainprogram->toscreenB, false, false, true);
        if (mainprogram->toscreenB->toggled()) {
            mainprogram->toscreenB->value = 0;
            mainprogram->toscreenB->oldvalue = 0;
            // SEND UP button copies preview set entirely to comp set
            mainmix->copy_to_comp(false, true, true);
        }
        box = mainprogram->toscreenB->box;
        register_triangle_draw(white, white, box->vtxcoords->x1 + box->vtxcoords->w / 2.0f + 0.0117f,
                               box->vtxcoords->y1 + 0.0225f, 0.0165f, 0.0312f, DOWN, CLOSED);
        render_text(mainprogram->toscreenB->name[0], white, box->vtxcoords->x1 + 0.0117f, box->vtxcoords->y1 + 0.0225f,
                    0.0006f, 0.001f);

        mainprogram->handle_button(mainprogram->toscreenM, false, false, true);
        if (mainprogram->toscreenM->toggled()) {
            mainprogram->toscreenM->value = 0;
            mainprogram->toscreenM->oldvalue = 0;
            // SEND UP button copies preview set entirely to comp set
            mainmix->copy_to_comp(true, true, true);
        }
        box = mainprogram->toscreenM->box;
        register_triangle_draw(white, white, box->vtxcoords->x1 + box->vtxcoords->w / 2.0f + 0.0117f,
                               box->vtxcoords->y1 + 0.0225f, 0.0165f, 0.0312f, DOWN, CLOSED);
        render_text(mainprogram->toscreenM->name[0], white, box->vtxcoords->x1 + 0.0117f, box->vtxcoords->y1 + 0.0225f,
                    0.0006f, 0.001f);

        for (int m = 0; m < 2; m++) {
            std::vector<int> scns;
            int bucurr = mainmix->currscene[m];
            for (int j = 0; j < 4; j++) {
                mainmix->scenes[m][j]->pos = j;
                if (j != mainmix->currscene[m]) {
                    scns.push_back(j + 1);
                }
            }
            for (int i = 0; i < 3; i++) {
                mainprogram->handle_button(mainprogram->toscene[m][0][i], false, false, true);
                if (mainprogram->toscene[m][0][i]->toggled()) {
                    mainprogram->toscene[m][0][i]->value = 0;
                    mainprogram->toscene[m][0][i]->oldvalue = 0;
                    // SEND UP button copies deck preview set to scene
                    Scene *scene = mainmix->scenes[m][scns[i] - 1];
                    mainprogram->prevmodus = true;
                    mainmix->mousedeck = m;
                    mainmix->scenenum = -1;
                    mainmix->save_deck(
                            mainprogram->temppath + "tempdecksc_" + std::to_string(m) + std::to_string(scns[i] - 1) +
                            ".deck", false, true);

                    mainprogram->prevmodus = false;
                    loopstation = scene->lpst;
                    std::vector<Layer*> bul[2];
                    bul[0] = mainmix->layers[2];
                    bul[1] = mainmix->layers[3];
                    mainmix->scenenum = scene->pos;
                    mainmix->open_deck(mainprogram->temppath + "tempdecksc_" + std::to_string(m) + std::to_string(scns[i] - 1) +".deck", true);
                    scene->scnblayers = mainmix->newlrs[m + 2];
                    mainmix->layers[2] = bul[0];
                    mainmix->layers[3] = bul[1];
                    for (Layer *lv : mainmix->newlrs[m + 2]) {
                        if (lv) {
                            lv->initdeck = false;
                        }
                    }
                    mainmix->swapmap[m + 2].clear();
                    mainmix->scenenum = -1;

                    // correct loopstation current times for deck saving/opening lag
                    LoopStation *bunowlpst = lp;
                    std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
                    std::chrono::duration<double> elapsed;
                    elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - bunowlpst->bunow);
                    long long millicount = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
                    for (LoopStationElement *elem: loopstation->odelems) {
                          elem->starttime = now - std::chrono::milliseconds((long long) (elem->interimtime));
                    }

                    mainprogram->recundo = true;
                    mainprogram->prevmodus = true;
                }
                box = mainprogram->toscene[m][0][i]->box;
                register_triangle_draw(white, white, box->vtxcoords->x1 + box->vtxcoords->w / 2.0f + 0.0117f,
                                       box->vtxcoords->y1 + 0.0225f, 0.0165f, 0.0312f, DOWN, CLOSED);
                const char *charstr = std::to_string(scns[i]).c_str();
                render_text(charstr, white, box->vtxcoords->x1 + 0.0117f,
                            box->vtxcoords->y1 + 0.0225f, 0.0006f, 0.001f);

                mainprogram->handle_button(mainprogram->toscene[m][1][i], false, false, true);
                if (mainprogram->toscene[m][1][i]->toggled()) {
                    mainprogram->swappingscene = true;
                    mainprogram->toscene[m][1][i]->value = 0;
                    mainprogram->toscene[m][1][i]->oldvalue = 0;

                    // SEND DOWN button copies back scene to deck preview set
                    Scene *scene = mainmix->scenes[m][scns[i] - 1];
                    mainprogram->prevmodus = false;
                    scene->switch_to(false);
                    mainmix->currscene[m] = scns[i] - 1;
                    mainmix->mousedeck = m;
                    loopstation = scene->lpst;
                    mainmix->scenenum = scene->pos;
                    mainmix->save_deck(
                            mainprogram->temppath + "tempdecksc_" + std::to_string(m) +
                            std::to_string(scns[i] - 1) +
                            ".deck", false, true);
                    mainmix->scenes[m][bucurr]->switch_to(false);
                    mainmix->currscene[m] = bucurr;

                    mainprogram->prevmodus = true;
                    loopstation = lp;
                    mainmix->scenenum = -1;
                    mainmix->open_deck(mainprogram->temppath + "tempdecksc_" + std::to_string(m) + std::to_string(scns[i] - 1) +".deck", true);
                    LoopStation *bulp = lp;

                    // correct loopstation current times for deck saving/opening lag
                    loopstation = lp;
                    LoopStation *bunowlpst = scene->lpst;
                    std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
                    std::chrono::duration<double> elapsed;
                    elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - bunowlpst->bunow);
                    long long millicount = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
                    for (LoopStationElement *elem: loopstation->odelems) {
                        //elem->interimtime += millicount;
                        //elem->speedadaptedtime += millicount * elem->speed->value;
                        elem->starttime = now - std::chrono::milliseconds((long long) (elem->interimtime));
                    }
                    mainprogram->recundo = true;
                }
                
                box = mainprogram->toscene[m][1][i]->box;
                register_triangle_draw(white, white, box->vtxcoords->x1 + box->vtxcoords->w / 2.0f + 0.0117f,
                                       box->vtxcoords->y1 + 0.0225f, 0.0165f, 0.0312f, UP, CLOSED);
                render_text(std::to_string(scns[i]), white, box->vtxcoords->x1 + 0.0117f,
                            box->vtxcoords->y1 + 0.0225f, 0.0006f, 0.001f);
            }
        }
    }
	if (mainprogram->prevmodus) {
        mainprogram->handle_button(mainprogram->backtopreA, false, false, true);
        if (mainprogram->backtopreA->toggled()) {
            mainprogram->backtopreA->value = 0;
            mainprogram->backtopreA->oldvalue = 0;
            // SEND DOWN button copies comp set entirely back to preview set
            mainmix->copy_to_comp(true, false, false);
        }
        Boxx* box = mainprogram->backtopreA->box;
        register_triangle_draw(white, white, box->vtxcoords->x1 + box->vtxcoords->w / 2.0f + 0.0117f, box->vtxcoords->y1 + 0.0225f, 0.0165f, 0.0312f, UP, CLOSED);
        render_text(mainprogram->backtopreA->name[0], white, mainprogram->backtopreA->box->vtxcoords->x1 + 0.0117f, mainprogram->backtopreA->box->vtxcoords->y1 + 0.0225f, 0.0006, 0.001);

        mainprogram->handle_button(mainprogram->backtopreB, false, false, true);
        if (mainprogram->backtopreB->toggled()) {
            mainprogram->backtopreB->value = 0;
            mainprogram->backtopreB->oldvalue = 0;
            // SEND DOWN button copies comp set entirely back to preview set
            mainmix->copy_to_comp(false, true, false);
        }
        box = mainprogram->backtopreB->box;
        register_triangle_draw(white, white, box->vtxcoords->x1 + box->vtxcoords->w / 2.0f + 0.0117f, box->vtxcoords->y1 + 0.0225f, 0.0165f, 0.0312f, UP, CLOSED);
        render_text(mainprogram->backtopreB->name[0], white, mainprogram->backtopreB->box->vtxcoords->x1 + 0.0117f, mainprogram->backtopreB->box->vtxcoords->y1 + 0.0225f, 0.0006, 0.001);

        mainprogram->handle_button(mainprogram->backtopreM, false, false, true);
        if (mainprogram->backtopreM->toggled()) {
            mainprogram->backtopreM->value = 0;
            mainprogram->backtopreM->oldvalue = 0;
            // SEND DOWN button copies comp set entirely back to preview set
            mainmix->copy_to_comp(true, true, false);
        }
        box = mainprogram->backtopreM->box;
        register_triangle_draw(white, white, box->vtxcoords->x1 + box->vtxcoords->w / 2.0f + 0.0117f, box->vtxcoords->y1 + 0.0225f, 0.0165f, 0.0312f, UP, CLOSED);
        render_text(mainprogram->backtopreM->name[0], white, mainprogram->backtopreM->box->vtxcoords->x1 + 0.0117f, mainprogram->backtopreM->box->vtxcoords->y1 + 0.0225f, 0.0006, 0.001);

    }

    mainprogram->handle_button(mainprogram->modusbut, false, false, true);
	if (mainprogram->modusbut->toggled()) {
	    //mainmix->currlays[!mainprogram->prevmodus].clear();
		mainprogram->prevmodus = !mainprogram->prevmodus;
        std::swap(mainmix->swapscrollpos[0], mainmix->scenes[0][mainmix->currscene[0]]->scrollpos);
        std::swap(mainmix->swapscrollpos[1], mainmix->scenes[1][mainmix->currscene[1]]->scrollpos);
		//modusbut is button that toggles effect preview mode to performance mode and back
        for (int i = 0; i < 4; i++ ) {
            for (Layer *lay : mainmix->layers[i]) {
                if (lay->clips.size() > 1) {
                    lay->compswitched = true;
                    GLuint tex = copy_tex(lay->node->vidbox->tex, 192, 108);
                    save_thumb(lay->currcliptexpath, tex);
                }
            }
        }
	}
	render_text(mainprogram->modusbut->name[mainprogram->prevmodus], white, mainprogram->modusbut->box->vtxcoords->x1 + 0.0117f, mainprogram->modusbut->box->vtxcoords->y1 + 0.0225f, 0.00042, 0.00070);
}

void Program::preferences() {
	if (this->prefon) {
        this->directmode = true;
        SDL_GL_MakeCurrent(this->prefwindow, glc);
        SDL_RaiseWindow(this->prefwindow);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDrawBuffer(GL_BACK_LEFT);
		glViewport(0, 0, glob->w / 2.0f, glob->h / 2.0f);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
		bool prret = this->preferences_handle();
		if (prret) {
			SDL_GL_SwapWindow(this->prefwindow);
		}
		SDL_GL_MakeCurrent(this->mainwindow, glc);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDrawBuffer(GL_BACK_LEFT);
        glViewport(0, 0, glob->w, glob->h);
        if (this->prefoff) {
            this->prefoff = false;
        }
        this->directmode = false;
	}
	else {
	    this->prefoff = true;
	    this->prefsearchdirs = &retarget->globalsearchdirs;
	    this->oldseatname = this->seatname;
	}
}

bool Program::preferences_handle() {
	int mx = -1;
	int my = -1;
	if (SDL_GetMouseFocus() == this->prefwindow) {
		//SDL_PumpEvents();
		SDL_GetMouseState(&mx, &my);
		mx *= 2.0f;
		my *= 2.0f;
	}
	if (this->rightmouse) this->renaming = EDIT_CANCEL;

	this->insmall = true;
	float lightblue[] = { 0.5f, 0.5f, 1.0f, 1.0f };
	float green[] = { 0.0f, 0.7f, 0.0f, 1.0f };

	this->bvao = this->prboxvao;
	this->bvbuf = this->prboxvbuf;
	this->btbuf = this->prboxtbuf;
	glClear(GL_COLOR_BUFFER_BIT);

	for (int i = 1; i < this->prefs->items.size(); i++) {
		PrefCat* item = this->prefs->items[i];
		if (item->box->in(mx, my)) {
			draw_box(white, lightblue, item->box, -1);
			if (this->leftmouse && this->prefs->curritem != i) {
				this->renaming = EDIT_NONE;
				this->prefs->curritem = i;
			}
		}
		else if (this->prefs->curritem == i) {
			draw_box(white, green, item->box, -1);
		}
		else {
			draw_box(white, black, item->box, -1);
		}
		render_text(item->name, white, item->box->vtxcoords->x1 + 0.03f, item->box->vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, 1, 0);
	}
	draw_box(white, nullptr, -0.5f, -1.0f, 1.5f, 2.0f, -1);

    PrefCat* mci = this->prefs->items[1];  // Project settings
    for (int i = 0; i < mci->items.size(); i++) {
        if (this->prefoff) {
            if (mci->items[i]->dest == &this->project->name) {
                mci->items[i]->str = this->project->name;
            } else if (mci->items[i]->dest == &this->project->ow) {
                mci->items[i]->value = this->project->ow;
            } else if (mci->items[i]->dest == &this->project->oh) {
                mci->items[i]->value = this->project->oh;
            }
        }
    }

	mci = this->prefs->items[this->prefs->curritem];
	if (mci->name == "MIDI Devices") ((PIMidi*)mci)->populate();
    bool brk = false;
	for (int i = 0; i < mci->items.size(); i++) {
	    if (mci->items[i]->name == "Project name") {
	        mci->items[i]->dest = &mainprogram->project->name;
	    }
	    if (mci->items[i]->name == "Project output video width") {
	        mci->items[i]->dest = &mainprogram->project->ow;
	    }
	    if (mci->items[i]->name == "Project output video height") {
	        mci->items[i]->dest = &mainprogram->project->oh;
	    }
 		if (mci->items[i]->type == PREF_ONOFF) {
			if (!mci->items[i]->connected) continue;
			draw_box(white, black, mci->items[i]->namebox, -1);
			render_text(mci->items[i]->name, white, mci->items[i]->namebox->vtxcoords->x1 + 0.23f, mci->items[i]->namebox->vtxcoords->y1 + 0.06f, 0.0024f, 0.004f, 1, 0);
			if (mci->items[i]->valuebox->in(mx, my)) {
				draw_box(white, lightblue, mci->items[i]->valuebox, -1);
				if (this->leftmouse) {
					mci->items[i]->onoff = !mci->items[i]->onoff;
                    if (mci->name == "MIDI Devices") {
						PIMidi* midici = (PIMidi*)mci;
						if (!midici->items[i]->onoff) {
							if (std::find(midici->onnames.begin(), midici->onnames.end(), midici->items[i]->name) != midici->onnames.end()) {
								midici->onnames.erase(std::find(midici->onnames.begin(), midici->onnames.end(), midici->items[i]->name));
								if (std::find(this->openports.begin(), this->openports.end(), i) != this->openports.end()) {
                                    this->openports.erase(std::find(this->openports.begin(), this->openports.end(), i));
                                }
								mci->items[i]->midiin->cancelCallback();
								delete mci->items[i]->midiin;
							}
						}
						else {
                            if (std::find(midici->onnames.begin(), midici->onnames.end(), midici->items[i]->name) == midici->onnames.end()) {
                                midici->onnames.push_back(midici->items[i]->name);
                                RtMidiIn *midiin = new RtMidiIn();
                                if (std::find(this->openports.begin(), this->openports.end(), i) ==
                                    this->openports.end()) {
                                    midiin->openPort(i);
                                    midiin->setCallback(&mycallback, (void *) midici->items[i]);
                                    this->openports.push_back(i);
                                }
                                mci->items[i]->midiin = midiin;
                            }
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
            if (mci->items[i]->onoff && mci->items[i]->dest == &this->server) {
                // set server ip pref to localip
                for (int j = 0; j < mci->items.size(); j++) {
                    if (mci->items[j]->dest == &this->serverip) {
                        mci->items[j]->path = this->localip;
                    }
                }
            }
		}
		else if (mci->items[i]->type == PREF_NUMBER) {
			draw_box(white, black, mci->items[i]->namebox, -1);
			render_text(mci->items[i]->name, white, mci->items[i]->namebox->vtxcoords->x1 + 0.23f, mci->items[i]->namebox->vtxcoords->y1 + 0.06f, 0.0024f, 0.004f, 1, 0);
			draw_box(white, black, mci->items[i]->valuebox, -1);
			if (mci->items[i]->renaming) {
				if (this->renaming == EDIT_NONE) {
					mci->items[i]->renaming = false;
					try {
						mci->items[i]->value = std::stoi(this->inputtext);
					}
					catch (...) {
						mci->items[i]->value = ((PIVid*)(mci->items[i]))->oldvalue;
					}
                    if (mci->items[i]->dest == &this->project->ow || mci->items[i]->dest == &this->project->oh) {
                        this->saveproject = true;
                    }
				}
				else if (this->renaming == EDIT_CANCEL) {
					mci->items[i]->renaming = false;
				}
				else {
					do_text_input(mci->items[i]->valuebox->vtxcoords->x1 + 0.1f, mci->items[i]->valuebox->vtxcoords->y1 + 0.06f, 0.0024f, 0.004f, mx, my, this->xvtxtoscr(0.15f), 1, mci->items[i], true);
				}
			}
			else {
                char* charstr = (char*)std::to_string(mci->items[i]->value).c_str();
				render_text(charstr, white, mci->items[i]->valuebox->vtxcoords->x1 + 0.1f, mci->items[i]->valuebox->vtxcoords->y1 + 0.06f, 0.0024f, 0.004f, 1, 0);
			}
			if (mci->items[i]->valuebox->in(mx, my)) {
				if (this->leftmouse && this->renaming == EDIT_NONE) {
					mci->items[i]->renaming = true;
					((PIVid*)(mci->items[i]))->oldvalue = mci->items[i]->value;
					this->renaming = EDIT_NUMBER;
					this->inputtext = std::to_string(mci->items[i]->value);
					this->cursorpos0 = this->inputtext.length();
					SDL_StartTextInput();
				}
			}
		}
        else if (mci->items[i]->type == PREF_STRING) {
            draw_box(white, black, mci->items[i]->namebox->vtxcoords->x1,
                     mci->items[i]->namebox->vtxcoords->y1, mci->items[i]->namebox->vtxcoords->w,
                     mci->items[i]->namebox->vtxcoords->h, -1);
            render_text(mci->items[i]->name, white, -0.5f + 0.1f,
                        mci->items[i]->namebox->vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, 1, 0);
            draw_box(white, black, mci->items[i]->valuebox->vtxcoords->x1,
                     mci->items[i]->valuebox->vtxcoords->y1, mci->items[i]->valuebox->vtxcoords->w,
                     mci->items[i]->valuebox->vtxcoords->h, -1);
            if (mci->items[i]->renaming == false) {
                render_text(mci->items[i]->str, white,
                            mci->items[i]->valuebox->vtxcoords->x1 + 0.1f,
                            mci->items[i]->valuebox->vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, 1, 0);
            } else {
                if (this->renaming == EDIT_NONE) {
                    mci->items[i]->renaming = false;
                    mci->items[i]->str = this->inputtext;
                    if (mci->items[i]->dest == &this->project->name) {
                        this->projnamechanged = true;
                        this->saveproject = true;
                    }
                } else if (this->renaming == EDIT_CANCEL) {
                    mci->items[i]->renaming = false;
                } else {
                    do_text_input(mci->items[i]->valuebox->vtxcoords->x1 + 0.1f,
                                  mci->items[i]->valuebox->vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, mx, my,
                                  this->xvtxtoscr(0.7f), 1, mci->items[i], true);
                }
            }
            if (mci->items[i]->valuebox->in(mx, my)) {
                if (this->leftmouse) {
                    for (int i = 0; i < mci->items.size(); i++) {
                        if (mci->items[i]->renaming) {
                            mci->items[i]->renaming = false;
                            end_input();
                            break;
                        }
                    }
                    mci->items[i]->renaming = true;
                    this->renaming = EDIT_STRING;
                    this->inputtext = mci->items[i]->str;
                    this->cursorpos0 = this->inputtext.length();
                    SDL_StartTextInput();
                }
            }
        }

        bool cond1 = (mci->items[i]->type == PREF_PATH);
        bool cond2 = (mci->items[i]->type == PREF_PATHS);
        if (cond1 || cond2) {
            std::vector<std::string> paths;
            if (cond1) paths.push_back(mci->items[i]->path);
            else paths = *this->prefsearchdirs;

            if (cond2) {
                // mousewheel scroll
                this->pathscroll -= this->mousewheel;
                if (this->pathscroll < 0) this->pathscroll = 0;
                if (paths.size() > 7 && paths.size() - this->pathscroll < 7)
                    this->pathscroll = paths.size() - 6;

                // GUI arrow scroll
                this->pathscroll = this->handle_scrollboxes(*this->defaultsearchscrollup,
                                                                          *this->defaultsearchscrolldown,
                                                                          paths.size(), this->pathscroll, 6,
                                                                          mx, my);
            }

            int size = std::min(6, (int) paths.size());

            for (int j = 0; j < size; j++) {
                // display and handle directory item(s) (list)
                std::string path = paths[j];
                draw_box(white, black, mci->items[i]->namebox->vtxcoords->x1, mci->items[i]->namebox->vtxcoords->y1 - j * 0.2f, mci->items[i]->namebox->vtxcoords->w, mci->items[i]->namebox->vtxcoords->h, -1);
                render_text(mci->items[i]->name, white, -0.5f + 0.1f, mci->items[i]->namebox->vtxcoords->y1 - j * 0.2f + 0.03f, 0.0024f, 0.004f, 1, 0);
                draw_box(white, black, mci->items[i]->valuebox->vtxcoords->x1, mci->items[i]->valuebox->vtxcoords->y1 - j * 0.2f, mci->items[i]->valuebox->vtxcoords->w, mci->items[i]->valuebox->vtxcoords->h, -1);
                if (mci->items[i]->renaming == false) {
                    render_text(paths[j + (this->pathscroll * cond2)], white,
                                mci->items[i]->valuebox->vtxcoords->x1 + 0.1f, mci->items[i]->valuebox->vtxcoords->y1 - j * 0.2f + 0.03f, 0.0024f, 0.004f, 1, 0);
                }
                else {
                    if (this->renaming == EDIT_NONE) {
                        mci->items[i]->renaming = false;
                        paths[j + (this->pathscroll * cond2)] = this->inputtext;
                        if (size == 1) mci->items[i]->path = this->inputtext;
                        if (mci->items[i]->dest == &this->project->name) {
                            this->projnamechanged = true;
                            this->saveproject = true;
                        }
                    }
                    else if (this->renaming == EDIT_CANCEL) {
                        mci->items[i]->renaming = false;
                    }
                    else {
                        do_text_input(mci->items[i]->valuebox->vtxcoords->x1 + 0.1f, mci->items[i]->valuebox->vtxcoords->y1 - j * 0.2f + 0.03f, 0.0024f, 0.004f, mx, my, this->xvtxtoscr(0.7f), 1, mci->items[i], true);
                    }
                }
                if (mci->items[i]->valuebox->in(mx, my)) {
                    if (this->leftmouse) {
                        for (int i = 0; i < mci->items.size(); i++) {
                            if (mci->items[i]->renaming) {
                                mci->items[i]->renaming = false;
                                end_input();
                                break;
                            }
                        }
                        mci->items[i]->renaming = true;
                        this->renaming = EDIT_STRING;
                        this->inputtext = paths[j];
                        this->cursorpos0 = this->inputtext.length();
                        SDL_StartTextInput();
                    }
                }
                if (mci->items[i]->dest != &this->project->name) {
                    if (cond2) {
                        draw_box(white, black, mci->items[i]->rembox->vtxcoords->x1,
                                 mci->items[i]->rembox->vtxcoords->y1 - j * 0.2f, mci->items[i]->rembox->vtxcoords->w,
                                 mci->items[i]->rembox->vtxcoords->h, -1);
                        render_text("X", white,
                                    mci->items[i]->rembox->vtxcoords->x1 + 0.04f, mci->items[i]->rembox->vtxcoords->y1 - j * 0.2f + 0.07f, 0.0024f, 0.004f, 1, 0);
                        if (mci->items[i]->rembox->in(mx, my)) {
                            if (this->leftmouse) {
                                mci->items.erase(mci->items.begin() + i);
                                brk = true;
                                break;
                            }
                        }
                    }
                    draw_box(white, black, mci->items[i]->iconbox->vtxcoords->x1 + 0.02f,
                             mci->items[i]->iconbox->vtxcoords->y1 - j * 0.2f + 0.05f, 0.06f, 0.07f, -1);
                    draw_box(white, black, mci->items[i]->iconbox->vtxcoords->x1 + 0.05f,
                             mci->items[i]->iconbox->vtxcoords->y1 - j * 0.2f + 0.11f, 0.025f, 0.03f, -1);
                    draw_box(white, black, mci->items[i]->iconbox->vtxcoords->x1,
                             mci->items[i]->iconbox->vtxcoords->y1 - j * 0.2f, mci->items[i]->iconbox->vtxcoords->w,
                             mci->items[i]->iconbox->vtxcoords->h, -1);
                    draw_box(white, black, mci->items[i]->iconbox->vtxcoords->x1 + 0.02f,
                             mci->items[i]->iconbox->vtxcoords->y1 - j * 0.2f + 0.05f, 0.06f, 0.07f, -1);
                    draw_box(white, black, mci->items[i]->iconbox->vtxcoords->x1 + 0.05f,
                             mci->items[i]->iconbox->vtxcoords->y1 - j * 0.2f + 0.11f, 0.025f, 0.03f, -1);
                    if (mci->items[i]->iconbox->in(mx, my)) {
                        if (this->leftmouse) {
                            mci->items[i]->choosing = true;
                            this->pathto = "CHOOSEDIR";
                            std::string title = "Open " + mci->items[i]->name + " directory";
                            std::thread filereq(&Program::get_dir, this, title.c_str(),
                                                std::filesystem::canonical(mci->items[i]->path).generic_string());
                            filereq.detach();
                        }
                    }
                    if (mci->items[i]->choosing && this->choosedir != "") {
#ifdef WINDOWS
                        boost::replace_all(this->choosedir, "/", "\\");
#endif
                        paths[j] = this->choosedir;
                        this->choosedir = "";
                        mci->items[i]->choosing = false;
                    }
                }
            }
            if (brk) break;

            //if (cond1) *(std::string*)mci->items[i]->dest = paths[0];
            //else *(std::vector<std::string>*)mci->items[i]->dest = paths;
        }

        if (cond2) {
            std::unique_ptr <Boxx> box = std::make_unique <Boxx> ();
            box->vtxcoords->x1 = -0.3f;
            box->vtxcoords->y1 = -0.8f;
            box->vtxcoords->w = 0.4f;
            box->vtxcoords->h = 0.2f;
            box->upvtxtoscr();
            draw_box(white, black, box, -1);
            render_text("+ DEFAULT SEARCH DIR", white, -0.25f, -0.8f + 0.03f, 0.0024f, 0.004f, 1, 0);
            if (box->in(mx, my) && this->leftmouse) {
                this->pathto = "ADDSEARCHDIR";
                std::thread filereq(&Program::get_dir, this, "Add a search location",
                                    std::filesystem::canonical(this->currfilesdir).generic_string());
                filereq.detach();
            }
        }

        if (mci->items[i]->connected) mci->items[i]->namebox->in(mx, my); //trigger tooltip

	}

	this->qualfr = std::clamp((int)this->qualfr, 1, 10);

    std::unique_ptr <Boxx> box = std::make_unique <Boxx> ();
	box->vtxcoords->x1 = 0.75f;
	box->vtxcoords->y1 = -1.0f;
	box->vtxcoords->w = 0.3f;
	box->vtxcoords->h = 0.2f;
	box->upvtxtoscr();
	draw_box(white, black, box, -1);
	if (box->in(mx, my)) {
		draw_box(white, lightblue, box, -1);
		if (this->leftmouse) {
			for (int i = 0; i < mci->items.size(); i++) {
				if (mci->items[i]->renaming) {
					mci->items[i]->renaming = false;
					end_input();
					break;
				}
			}
			this->renaming = EDIT_NONE;
			this->prefs->load();
			this->prefon = false;
			this->drawnonce = false;
            this->pathscroll = 0;  // needed after default search listing
			SDL_HideWindow(this->prefwindow);
			SDL_RaiseWindow(this->mainwindow);
		}
	}
	render_text("CANCEL", white, box->vtxcoords->x1 + 0.02f, box->vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, 1, 0);


	// SAVE
	box->vtxcoords->x1 = 0.45f;
	box->upvtxtoscr();
	draw_box(white, black, box, -1);
	if (box->in(mx, my)) {
		draw_box(white, lightblue, box, -1);
		if (this->leftmouse) {
            for (int j = 1; j < this->prefs->items.size(); j++) {
                PrefCat *item = this->prefs->items[j];
                for (int i = 0; i < item->items.size(); i++) {
                    if (item->items[i]->name == "Project output video width") {
                        mainprogram->project->ow = item->items[i]->value;
                    }
                    if (item->items[i]->name == "Project output video height") {
                        mainprogram->project->oh = item->items[i]->value;
                    }
                    if (item->items[i]->renaming) {
                        if (item->items[i]->type == PREF_ONOFF) {
                            *(bool *) item->items[i]->dest = item->items[i]->value;
                            break;
                        }
                        if (item->items[i]->type == PREF_STRING) {
                            item->items[i]->str = this->inputtext;
                            end_input();
                            *(std::string *) item->items[i]->dest = item->items[i]->str;
                            break;
                        }
                        if (item->items[i]->type == PREF_PATH) {
                            item->items[i]->path = this->inputtext;
                            end_input();
                            *(std::string *) item->items[i]->dest = item->items[i]->path;
                            break;
                        }
                        if (item->items[i]->type == PREF_NUMBER) {
                            try {
                                item->items[i]->value = std::stoi(this->inputtext);
                                *(int *) item->items[i]->dest = item->items[i]->value;
                            }
                            catch (...) {}
                            end_input();
                            break;
                        }
                    }

                    /*if (item->items[i]->type == PREF_STRING) {
                        *(std::string *) item->items[i]->dest = item->items[i]->str;
                        break;
                    }
                    if (item->items[i]->type == PREF_PATH) {
                        *(std::string *) item->items[i]->dest = item->items[i]->path;
                        break;
                    }
                    if (item->items[i]->type == PREF_NUMBER) {
                        *(float*) item->items[i]->dest = (float)item->items[i]->value;
                    }*/

                    if (item->items[i]->dest == &this->seatname) {
                        if (item->items[i]->str != this->oldseatname) {
                            if (mainprogram->connected > 0) {
                                send(this->sock, "CHANGE_NAME", 12, 0);
                                send(this->sock, this->seatname.c_str(), this->seatname.size(), 0);
                            }
                        }
                    }
                }
            }
			if (this->projnamechanged) {  // project name not included in preferences file, only change if
			    // user clicks SAVE
                if (this->project->name != remove_extension(basename(this->project->path))) {
                    // project name changed
                    // rename project file
                    std::string pathdir = dirname(this->project->path);
                    std::string newdir = dirname(pathdir.substr(0, pathdir.size() - 2)) + this->project->name + "/";
                    std::filesystem::rename(pathdir, newdir);
                    // rename project directory
                    int pos = std::find(this->recentprojectpaths.begin(), this->recentprojectpaths.end(), this->project->path) - this->recentprojectpaths.begin();
                    this->project->path = newdir +
                                                 this->project->name + ".ewocvj";
                    if (pos < this->recentprojectpaths.size()) {
                        this->recentprojectpaths[pos] = this->project->path;
                        this->write_recentprojectlist();
                    }
                    std::string bubd = this->project->binsdir;
                    std::string busd = this->project->shelfdir;
                    std::string buad = this->project->autosavedir;
                    std::string bued = this->project->elementsdir;
                    this->project->binsdir = newdir + "bins/";
                    this->project->recdir = newdir + "recordings/";
                    this->project->shelfdir = newdir + "shelves/";
                    this->project->autosavedir = newdir + "autosaves/";
                    this->project->elementsdir = newdir + "elements/";
                    for (int i = 0; i < binsmain->bins.size(); i++) {
                        binsmain->bins[i]->path = this->project->binsdir + basename(binsmain->bins[i]->path);
                        for (int j = 0; j < binsmain->bins[i]->elements.size(); j++) {
                            std::string str = binsmain->bins[i]->elements[j]->path;
                            if (str.find(bubd) != std::string::npos) {
                                str = str.replace(str.find(bubd), bubd.size(), this->project->binsdir);
                                binsmain->bins[i]->elements[j]->path = str;
                                binsmain->bins[i]->elements[j]->relpath = std::filesystem::relative(str, mainprogram->project->binsdir).generic_string();
                            }
                        }
                    }
                    for (int i = 0; i < 2; i++) {
                        for (int j = 0; j < this->shelves[i]->elements.size(); j++) {
                            std::string str = this->shelves[i]->elements[j]->path;
                            std::string jstr = this->shelves[i]->elements[j]->jpegpath;
                            if (str.find(busd) != std::string::npos) {
                                str = str.replace(str.find(busd), busd.size(), this->project->shelfdir);
                                this->shelves[i]->elements[j]->path = str;
                            }
                            if (jstr.find(busd) != std::string::npos) {
                                jstr = jstr.replace(jstr.find(busd), busd.size(), this->project->shelfdir);
                                this->shelves[i]->elements[j]->jpegpath = jstr;
                            }
                        }
                    }
                }
            }

			retarget->globalsearchdirs = *this->prefsearchdirs;

			// set output resolution from project settings
            this->projnamechanged = false;
			this->renaming = EDIT_NONE;
			this->prefs->save();
			this->prefs->load();
			this->prefon = false;
			this->drawnonce = false;

			if (this->saveproject) {
                if (this->project->path.find("autosave") != std::string::npos) {
                    this->path = this->project->bupp;
                    this->pathto = "SAVEPROJECT";
                } else {
                    this->project->save(this->project->path);
                }
            }
            this->ow = this->project->ow;
            this->oh = this->project->oh;
            this->set_ow3oh3();
            this->handle_changed_owoh();

            SDL_HideWindow(this->prefwindow);
			SDL_RaiseWindow(this->mainwindow);
		}
	}
	render_text("SAVE", white, box->vtxcoords->x1 + 0.02f, box->vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, 1, 0);
    if (saveproject) render_text("+PROJECT", white, box->vtxcoords->x1 + 0.06f, box->vtxcoords->y1 + 0.1f, 0.0024f, 0.004f, 1, 0);

    this->tooltips_handle(1);

	this->bvao = this->boxvao;
	this->bvbuf = this->boxvbuf;
	this->btbuf = this->boxtbuf;
	this->middlemouse = false;
	this->rightmouse = false;
	this->menuactivation = false;
	this->mx = -1;
	this->my = -1;

	this->insmall = false;

	return true;
}

/*void Program::longtooltip_prepare(Boxx *box) {
    float fac = 1.0f;
    if (box->smflag) fac = 4.0f;

    std::vector<std::string> texts;
    int pos = 0;
    while (pos < box->tooltip.length() - 1) {
        int oldpos = pos;
        pos = box->tooltip.rfind(" ", pos + 58);
        if (pos == -1) break;
        texts.push_back(box->tooltip.substr(oldpos, pos - oldpos));
    }
    float x = 99.0f;  //first print offscreen
    float y = 99.0f;
    float textw = 0.5f * sqrt(1.0f);
    float texth = 0.092754f * sqrt(1.0f);
    render_text(box->tooltiptitle, orange, x, y, 0.00045f * fac, 0.00075f * fac, box->smflag, 0);
    for (int i = 0; i < texts.size(); i++) {
        render_text(texts[i], white, x, y, 0.00045f * fac, 0.00075f * fac, box->smflag, 0);
    }
    return;
}*/

void Program::tooltips_handle(int win) {
	// draw tooltip
	float orange[] = { 1.0f, 0.5f, 0.0f, 1.0f };
	float fac = 1.0f;

	mainprogram->frontbatch = true;

	if (mainprogram->prefon || mainprogram->midipresets) fac = 4.0f;

	if (mainprogram->tooltipmilli > 4000) {
		if (mainprogram->longtooltips) {
			std::vector<std::string> texts;
			int pos = 0;
			while (pos < mainprogram->tooltipbox->tooltip.length() - 1) {
				int oldpos = pos;
				pos = mainprogram->tooltipbox->tooltip.rfind(" ", pos + 58);
				if (pos == -1) break;
				texts.push_back(mainprogram->tooltipbox->tooltip.substr(oldpos, pos - oldpos));
			}
			float x = mainprogram->tooltipbox->vtxcoords->x1 + mainprogram->tooltipbox->vtxcoords->w + 0.015f;
			float y = mainprogram->tooltipbox->vtxcoords->y1 - 0.015f * glob->w / glob->h - 0.015f;
			float textw = 0.375f * sqrt(fac);
			float texth = 0.092754f * sqrt(fac);
			if ((x + textw) > 1.0f) x = x - textw - 0.03f - mainprogram->tooltipbox->vtxcoords->w;
			if ((y - texth * (texts.size() + 1) - 0.015f) < -1.0f) y = -1.0f + texth * (texts.size() + 1) - 0.015f;
			if (x < -1.0f) x = -1.0f;
			draw_box(black, black, x, y - texth, textw, texth + 0.015f, -1);
			render_text(mainprogram->tooltipbox->tooltiptitle, orange, x + 0.0225f * sqrt(fac), y - texth + 0.045f * sqrt(fac), 0.00045f * fac, 0.00075f * fac, win, 0);
			for (int i = 0; i < texts.size(); i++) {
				y -= texth;
				draw_box(black, black, x, y - texth, textw, texth + 0.015f, -1);
				render_text(texts[i], white, x + 0.0225f * sqrt(fac), y - texth + 0.045f * sqrt(fac), 0.00045f * fac, 0.00075f * fac, win, 0);
			}
		}
		else {
			float x = mainprogram->tooltipbox->vtxcoords->x1 + mainprogram->tooltipbox->vtxcoords->w + 0.015f;  //first print offscreen
			float y = mainprogram->tooltipbox->vtxcoords->y1 - 0.015f * glob->w / glob->h - 0.015f;
			float textw = 0.15f * sqrt(fac);
			draw_box(black, black, x, y - 0.092754f, textw, 0.092754f + 0.015f, -1);
			render_text(mainprogram->tooltipbox->tooltiptitle, orange, x + 0.0225f * sqrt(fac), y - 0.092754f + 0.045f * sqrt(fac), 0.00045f * fac, 0.00075f * fac, win, 0);
		}
	}

	mainprogram->frontbatch = false;
}

int Program::config_midipresets_handle() {
	int mx = -1;
	int my = -1;
	if (SDL_GetMouseFocus() == mainprogram->config_midipresetswindow) {
		SDL_PumpEvents();
		SDL_GetMouseState(&mx, &my);
		mx *= 2.0f;
		my *= 2.0f;
	}

	mainprogram->insmall = true;

	mainprogram->directmode = true;

	SDL_GL_MakeCurrent(mainprogram->config_midipresetswindow, glc);
	mainprogram->bvao = mainprogram->tmboxvao;
	mainprogram->bvbuf = mainprogram->tmboxvbuf;
	mainprogram->btbuf = mainprogram->tmboxtbuf;
	glClear(GL_COLOR_BUFFER_BIT);

	if (mainprogram->rightmouse) {
		mainprogram->tmlearn = TM_NONE;
		mainprogram->menuactivation = false;
	}

	std::string lmstr;
	if (mainprogram->midipresetsset == 0) lmstr = "A";
	else if (mainprogram->midipresetsset == 1) lmstr = "B";
	else if (mainprogram->midipresetsset == 2) lmstr = "C";
	else if (mainprogram->midipresetsset == 3) lmstr = "D";
    if (mainprogram->tmlearn != TM_NONE) {
        std::string tempstr = "Creating settings for midideck " + lmstr;
        render_text(tempstr, white, -0.3f, 0.2f, 0.0024f, 0.004f, 2);
    }
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
    case TM_STOP:
        render_text("Learn MIDI Stop play", white, -0.3f, 0.0f, 0.0024f, 0.004f, 2);
        break;
    case TM_LOOP:
        render_text("Learn MIDI Loop Play", white, -0.3f, 0.0f, 0.0024f, 0.004f, 2);
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
    case TM_SCRATCH1:
        render_text("Learn MIDI Scratchwheel when wheel not touched", white, -0.3f, 0.0f, 0.0024f, 0.004f, 2);
        break;
    case TM_SCRATCH2:
        render_text("Learn MIDI Scratchwheel when wheel touched", white, -0.3f, 0.0f, 0.0024f, 0.004f, 2);
        break;
    case TM_CROSS:
        render_text("Learn MIDI Crossfade", white, -0.3f, 0.0f, 0.0024f, 0.004f, 2);
        break;
	}
    if (mainprogram->tmlearn != TM_NONE) render_text("Rightmouse button cancels ", white, -0.3f, -0.2f, 0.0024f, 0.004f, 2);

    if (mainprogram->tmlearn == TM_NONE) {
        //draw config_midipresets_handle screen
        for (int i = 0; i < 3; i++) {
            if (mainprogram->configcatmidi == i) draw_box(white, darkgreen1, mainprogram->tmcat[i], -1);
            else draw_box(white, black, mainprogram->tmcat[i], -1);
            if (mainprogram->tmcat[i]->in(mx, my)) {
                draw_box(red, lightblue, mainprogram->tmcat[i], -1);
                if (mainprogram->leftmouse) {
                    mainprogram->configcatmidi = i;
                }
            }
        }
        render_text("Layer controls", white, -0.25f, 0.94f, 0.0024f, 0.004f, 2);
        render_text("Shelf buttons", white, -0.25f, 0.86f, 0.0024f, 0.004f, 2);
        render_text("Loopstation buttons", white, -0.25f, 0.78f, 0.0024f, 0.004f, 2);

        if (mainprogram->configcatmidi == 0) {
            for (int i = 0; i < 4; i++) {
                if (mainprogram->midipresetsset == i) draw_box(white, darkgreen1, mainprogram->tmset[i], -1);
                else draw_box(white, black, mainprogram->tmset[i], -1);
                if (mainprogram->tmset[i]->in(mx, my)) {
                    draw_box(red, lightblue, mainprogram->tmset[i], -1);
                    if (mainprogram->leftmouse) {
                        mainprogram->midipresetsset = i;
                    }
                }
            }
            render_text("Set A", white, 0.21f, 0.96f, 0.0018f, 0.003f, 2);
            render_text("Set B", white, 0.21f, 0.90f, 0.0018f, 0.003f, 2);
            render_text("Set C", white, 0.21f, 0.84f, 0.0018f, 0.003f, 2);
            render_text("Set D", white, 0.21f, 0.78f, 0.0018f, 0.003f, 2);
            LayMidi *lm = nullptr;
            if (mainprogram->midipresetsset == 0) lm = laymidiA;
            else if (mainprogram->midipresetsset == 1) lm = laymidiB;
            else if (mainprogram->midipresetsset == 2) lm = laymidiC;
            else if (mainprogram->midipresetsset == 3) lm = laymidiD;

            draw_box(white, black, mainprogram->tmplay, -1);
            if (lm->play->midi0 != -1) draw_box(white, darkgreen2, mainprogram->tmplay, -1);
            if (mainprogram->tmplay->in(mx, my)) {
                draw_box(white, lightblue, mainprogram->tmplay, -1);
                if (mainprogram->leftmouse) {
                    mainprogram->tmlearn = TM_PLAY;
                }
            }
            register_triangle_draw(white, white, -0.025f, -0.83f, 0.06f, 0.12f, RIGHT, CLOSED, true);

            draw_box(white, black, mainprogram->tmbackw, -1);
            if (lm->backw->midi0 != -1) draw_box(white, darkgreen2, mainprogram->tmbackw, -1);
            if (mainprogram->tmbackw->in(mx, my)) {
                draw_box(white, lightblue, mainprogram->tmbackw, -1);
                if (mainprogram->leftmouse) {
                    mainprogram->tmlearn = TM_BACKW;
                }
            }
            register_triangle_draw(white, white, -0.335f, -0.83f, 0.06f, 0.12f, LEFT, CLOSED, true);

            draw_box(white, black, mainprogram->tmbounce, -1);
            if (lm->bounce->midi0 != -1) draw_box(white, darkgreen2, mainprogram->tmbounce, -1);
            if (mainprogram->tmbounce->in(mx, my)) {
                draw_box(white, lightblue, mainprogram->tmbounce, -1);
                if (mainprogram->leftmouse) {
                    mainprogram->tmlearn = TM_BOUNCE;
                }
            }
            register_triangle_draw(white, white, -0.195f, -0.83f, 0.04f, 0.12f, LEFT, CLOSED, true);
            register_triangle_draw(white, white, -0.14f, -0.83f, 0.04f, 0.12f, RIGHT, CLOSED, true);

            draw_box(white, black, mainprogram->tmfrforw, -1);
            if (lm->frforw->midi0 != -1) draw_box(white, darkgreen2, mainprogram->tmfrforw, -1);
            if (mainprogram->tmfrforw->in(mx, my)) {
                draw_box(white, lightblue, mainprogram->tmfrforw, -1);
                if (mainprogram->leftmouse) {
                    mainprogram->tmlearn = TM_FRFORW;
                }
            }
            register_triangle_draw(white, white, 0.125f, -0.83f, 0.06f, 0.12f, RIGHT, OPEN, true);

            draw_box(white, black, mainprogram->tmstop, -1);
            if (lm->stop->midi0 != -1) draw_box(white, darkgreen2, mainprogram->tmstop, -1);
            if (mainprogram->tmstop->in(mx, my)) {
                draw_box(white, lightblue, mainprogram->tmstop, -1);
                if (mainprogram->leftmouse) {
                    mainprogram->tmlearn = TM_STOP;
                }
            }
            draw_box(white, white, mainprogram->tmstop->vtxcoords->x1 + 0.04f,
                     mainprogram->tmstop->vtxcoords->y1 + 0.065f, 0.07f, 0.13f, -1);

            draw_box(white, black, mainprogram->tmloop, -1);
            if (lm->loop->midi0 != -1) draw_box(white, darkgreen2, mainprogram->tmloop, -1);
            if (mainprogram->tmloop->in(mx, my)) {
                draw_box(white, lightblue, mainprogram->tmloop, -1);
                if (mainprogram->leftmouse) {
                    mainprogram->tmlearn = TM_LOOP;
                }
            }
            render_text("LP", white, mainprogram->tmloop->vtxcoords->x1 + 0.06f,
                        mainprogram->tmloop->vtxcoords->y1 + 0.1f, 0.0024f, 0.004f, 2);

            draw_box(white, black, mainprogram->tmfrbackw, -1);
            if (lm->frbackw->midi0 != -1) draw_box(white, darkgreen2, mainprogram->tmfrbackw, -1);
            if (mainprogram->tmfrbackw->in(mx, my)) {
                draw_box(white, lightblue, mainprogram->tmfrbackw, -1);
                if (mainprogram->leftmouse) {
                    mainprogram->tmlearn = TM_FRBACKW;
                }
            }
            register_triangle_draw(white, white, -0.485f, -0.83f, 0.06f, 0.12f, LEFT, OPEN, true);

            draw_box(white, black, mainprogram->tmspeed, -1);
            if (lm->speed->midi0 != -1) draw_box(white, darkgreen2, mainprogram->tmspeed, -1);
            if (mainprogram->tmspeedzero->in(mx, my)) {
                draw_box(white, lightblue, mainprogram->tmspeedzero, -1);
                if (mainprogram->leftmouse) {
                    mainprogram->tmlearn = TM_SPEEDZERO;
                }
            } else if (mainprogram->tmspeed->in(mx, my)) {
                draw_box(white, lightblue, mainprogram->tmspeed, -1);
                if (mainprogram->leftmouse) {
                    mainprogram->tmlearn = TM_SPEED;
                }
                draw_box(white, black, mainprogram->tmspeedzero, -1);
                if (lm->speedzero->midi0 != -1) draw_box(white, darkgreen2, mainprogram->tmspeedzero, -1);
            } else {
                draw_box(white, black, mainprogram->tmspeedzero, -1);
                if (lm->speedzero->midi0 != -1) draw_box(white, darkgreen2, mainprogram->tmspeedzero, -1);
            }
            render_text("ONE", white, -0.755f, -0.08f, 0.0024f, 0.004f, 2);
            render_text("SPEED", white, -0.765f, -0.48f, 0.0024f, 0.004f, 2);

            draw_box(white, black, mainprogram->tmcross, -1);
            if (lm->crossfade->midi0 != -1) draw_box(white, darkgreen2, mainprogram->tmcross, -1);
            if (mainprogram->tmcross->in(mx, my)) {
                draw_box(white, lightblue, mainprogram->tmcross, -1);
                if (mainprogram->leftmouse) {
                    mainprogram->tmlearn = TM_CROSS;
                }
            }
            render_text("CROSSFADE", white, -0.195f, -0.48f, 0.0024f, 0.004f, 2);

            draw_box(white, black, mainprogram->tmopacity, -1);
            if (lm->opacity->midi0 != -1) draw_box(white, darkgreen2, mainprogram->tmopacity, -1);
            if (mainprogram->tmopacity->in(mx, my)) {
                draw_box(white, lightblue, mainprogram->tmopacity, -1);
                if (mainprogram->leftmouse) {
                    mainprogram->tmlearn = TM_OPACITY;
                }
            }
            render_text("OPACITY", white, 0.605f, -0.48f, 0.0024f, 0.004f, 2);

            if (lm->scrinvert) {
                draw_box(white, green, mainprogram->tmscrinvert, -1);
            }
            else {
                draw_box(white, black, mainprogram->tmscrinvert, -1);
            }
            if (mainprogram->tmscrinvert->in(mx, my)) {
                draw_box(white, lightblue, mainprogram->tmscrinvert, -1);
                if (mainprogram->leftmouse) {
                    lm->scrinvert = !lm->scrinvert;
                }
            }
            render_text("INVERT", white, 0.26f, 0.12f, 0.0024f, 0.004f, 2);

            if (mainprogram->tmfreeze->in(mx, my)) {
                draw_box(white, lightblue, mainprogram->tmfreeze, -1);
                if (mainprogram->leftmouse) {
                    mainprogram->tmlearn = TM_FREEZE;
                }
            } else {
                if (lm->scratch1->midi0 != -1) draw_box(darkgreen2, 0.0f, 0.1f, 0.4f, 3, smw, smh);
                if (lm->scratch2->midi0 != -1) draw_box(darkgreen2, 0.0f, 0.1f, 0.4f, 4, smw, smh);
                if (sqrt(pow((mx / (glob->w / 2.0f) - 1.0f) * glob->w / glob->h, 2) +
                         pow((glob->h - my) / (glob->h / 2.0f) - 1.1f, 2)) < 0.4f) {
                    if (my < glob->h - mainprogram->yvtxtoscr(1.1f)) {
                        draw_box(lightblue, 0.0f, 0.1f, 0.4f, 3, smw, smh);
                        mainprogram->tmscratch1->in(mx, my);  //tooltip
                        if (mainprogram->leftmouse) {
                            mainprogram->tmlearn = TM_SCRATCH1;
                        }
                    }
                    else {
                        draw_box(lightblue, 0.0f, 0.1f, 0.4f, 4, smw, smh);
                        mainprogram->tmscratch2->in(mx, my);  //tooltip
                        if (mainprogram->leftmouse) {
                            mainprogram->tmlearn = TM_SCRATCH2;
                        }
                    }
                }
                register_line_draw(white, -0.2f, 0.1f, 0.2f, 0.1f, true);
                draw_box(white, black, mainprogram->tmfreeze, -1);
                if (lm->scratchtouch->midi0 != -1) draw_box(white, darkgreen2, mainprogram->tmfreeze, -1);
            }
            draw_box(white, 0.0f, 0.1f, 0.4f, 2, smw, smh);
            render_text("SCRATCH1", white, -0.1f, 0.35f, 0.0024f, 0.004f, 2);
            render_text("SCRATCH2", white, -0.1f, -0.2f, 0.0024f, 0.004f, 2);
            render_text("FREEZE", white, -0.08f, 0.12f, 0.0024f, 0.004f, 2);

            std::unique_ptr<Boxx> box = std::make_unique<Boxx>();
            box->vtxcoords->x1 = -1.0f;
            box->vtxcoords->y1 = -1.0f;
            box->vtxcoords->w = 0.3f;
            box->vtxcoords->h = 0.2f;
            box->upvtxtoscr();
            draw_box(white, black, box, -1);
            if (box->in(mx, my)) {
                draw_box(white, lightblue, box, -1);
                if (mainprogram->leftmouse) {
                    lm->play->midi0 = -1;
                    lm->backw->midi0 = -1;
                    lm->bounce->midi0 = -1;
                    lm->frforw->midi0 = -1;
                    lm->frbackw->midi0 = -1;
                    lm->stop->midi0 = -1;
                    lm->loop->midi0 = -1;
                    lm->scratch1->midi0 = -1;
                    lm->scratch2->midi0 = -1;
                    lm->scratchtouch->midi0 = -1;
                    lm->speed->midi0 = -1;
                    lm->speedzero->midi0 = -1;
                    lm->opacity->midi0 = -1;
                    lm->play->midi1 = -1;
                    lm->backw->midi1 = -1;
                    lm->bounce->midi1 = -1;
                    lm->frforw->midi1 = -1;
                    lm->frbackw->midi1 = -1;
                    lm->stop->midi1 = -1;
                    lm->loop->midi1 = -1;
                    lm->scratch1->midi1 = -1;
                    lm->scratch2->midi1 = -1;
                    lm->scratchtouch->midi1 = -1;
                    lm->speed->midi1 = -1;
                    lm->speedzero->midi1 = -1;
                    lm->opacity->midi1 = -1;
                    lm->play->unregister_midi();
                    lm->backw->unregister_midi();
                    lm->bounce->unregister_midi();
                    lm->frforw->unregister_midi();
                    lm->frbackw->unregister_midi();
                    lm->stop->unregister_midi();
                    lm->loop->unregister_midi();
                    lm->scratch1->unregister_midi();
                    lm->scratch2->unregister_midi();
                    lm->scratchtouch->unregister_midi();
                    lm->speed->unregister_midi();
                    lm->speedzero->unregister_midi();
                    lm->opacity->unregister_midi();
                    return 0;
                }
            }
            render_text("NEW", white, box->vtxcoords->x1 + 0.02f, box->vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, 2);
        }
    }


	if (mainprogram->configcatmidi == 1) {
	    for (int m = 0; m < 2; m++) {
            for (int j = 0; j < 4; j++) {
                for (int i = 0; i < 4; i++) {
                    ShelfElement *elem = mainprogram->shelves[m]->elements[j * 4 + i];
                    Boxx box;
                    box.vtxcoords->x1 = m + i * 0.16f - 0.8f;
                    box.vtxcoords->y1 = (3 - j) * 0.28f - 0.56f;
                    box.vtxcoords->w = 0.16f;
                    box.vtxcoords->h = 0.28f;
                    box.upvtxtoscr();
                    box.tooltiptitle = "Shelf button MIDI config ";
                    box.tooltip = "Leftclicking this box allows setting a MIDI control for this shelf element. ";
                    if (elem->button->midi[0] != -1) draw_box(white, darkgreen1, &box, -1);
                    else draw_box(white, black, &box, -1);
                    if (box.in(mx, my)) {
                        draw_box(white, lightblue, &box, -1);
                        if (mainprogram->leftmouse) {
                            mainmix->learn = true;
                            mainmix->learnparam = nullptr;
                            mainmix->learnbutton = mainprogram->shelves[m]->buttons[j * 4 + i];
                            mainmix->learndouble = true;
                        }
                    }
                }
            }
	    }
        if (mainmix->learn) {
            draw_box(red, blue, -0.3f, -0.0f, 0.6f, 0.3f, -1);
            render_text("Awaiting MIDI input.", white, -0.1f, 0.2f, 0.001f, 0.0016f);
            render_text("Rightclick cancels.", white, -0.1f, 0.06f, 0.001f, 0.0016f);
        }
	}

    if (mainprogram->configcatmidi == 2) {
        for (int i = 0; i < 8; i++) {
            LoopStationElement *elem = loopstation->elements[i + loopstation->confscrpos];
            loopstation->confscrpos = mainprogram->handle_scrollboxes(*loopstation->confupscrbox, *loopstation->confdownscrbox, loopstation->elements.size(), loopstation->confscrpos, 8, mx, my);

            Boxx box;
            box.vtxcoords->x1 = -0.33f;
            box.vtxcoords->y1 = 0.45f - 0.15f * i;
            box.vtxcoords->w = 0.08f;
            box.vtxcoords->h = 0.15f;
            box.upvtxtoscr();
            box.tooltiptitle = "Loopstation record button MIDI config ";
            box.tooltip = "Leftclicking this box allows setting a MIDI control for the record button of this loopstation element. ";
            float radx = box.vtxcoords->w / 2.0f;
            float rady = box.vtxcoords->h / 2.0f;
            if (elem->recbut->midi[0] != -1) draw_box(white, darkgreen1, &box, -1);
            else draw_box(white, black, &box, -1);
            if (box.in(mx, my)) {
                draw_box(white, lightblue, &box, -1);
                if (mainprogram->leftmouse) {
                    mainmix->learn = true;
                    mainmix->learnparam = nullptr;
                    mainmix->learnbutton = elem->recbut;
                    mainmix->learndouble = true;
                }
            }
            draw_box(red, box.vtxcoords->x1 + radx, box.vtxcoords->y1 + rady, 0.045f, 2, smw, smh);

            box.vtxcoords->x1 = -0.25f;
            box.upvtxtoscr();
            box.tooltiptitle = "Loopstation loop play button MIDI config ";
            box.tooltip = "Leftclicking this box allows setting a MIDI control for the loop play button of this loopstation element. ";
            if (elem->loopbut->midi[0] != -1) draw_box(white, darkgreen1, &box, -1);
            else draw_box(white, black, &box, -1);
            if (box.in(mx, my)) {
                draw_box(white, lightblue, &box, -1);
                if (mainprogram->leftmouse) {
                    mainmix->learn = true;
                    mainmix->learnparam = nullptr;
                    mainmix->learnbutton = elem->loopbut;
                    mainmix->learndouble = true;
                }
            }
            draw_box(green, box.vtxcoords->x1 + radx, box.vtxcoords->y1 + rady, 0.045f, 2, smw, smh);

            box.vtxcoords->x1 = -0.17f;
            box.upvtxtoscr();
            box.tooltiptitle = "Loopstation single shot play button MIDI config ";
            box.tooltip = "Leftclicking this box allows setting a MIDI control for the single shot play button of this loopstation element. ";
            if (elem->playbut->midi[0] != -1) draw_box(white, darkgreen1, &box, -1);
            else draw_box(white, black, &box, -1);
            if (box.in(mx, my)) {
                draw_box(white, lightblue, &box, -1);
                if (mainprogram->leftmouse) {
                    mainmix->learn = true;
                    mainmix->learnparam = nullptr;
                    mainmix->learnbutton = elem->playbut;
                    mainmix->learndouble = true;
                }
            }
            draw_box(blue, box.vtxcoords->x1 + radx, box.vtxcoords->y1 + rady, 0.045f, 2, smw, smh);

            render_text(std::to_string(i + loopstation->confscrpos + 1), white, box.vtxcoords->x1 - 0.3f, box.vtxcoords->y1 + 0.06f, 0.0024f, 0.004f, 2);
        }

        if (mainmix->learn) {
            draw_box(red, blue, -0.3f, -0.0f, 0.6f, 0.3f, -1);
            render_text("Awaiting MIDI input.", white, -0.1f, 0.2f, 0.001f, 0.0016f);
            render_text("Rightclick cancels.", white, -0.1f, 0.06f, 0.001f, 0.0016f);
        }
    }


    std::unique_ptr <Boxx> box = std::make_unique <Boxx> ();
	box->vtxcoords->x1 = 0.80f;
	box->vtxcoords->y1 = -1.0f;
	box->vtxcoords->w = 0.2f;
	box->vtxcoords->h = 0.2f;
	box->upvtxtoscr();
	draw_box(white, black, box, -1);
	if (box->in(mx, my)) {
		draw_box(white, lightblue, box, -1);
		if (mainprogram->leftmouse) {
            if (exists(mainprogram->docpath + "midiset.gm")) {
                open_genmidis(mainprogram->docpath + "midiset.gm");
            }
			mainprogram->tmlearn = TM_NONE;
			mainprogram->midipresets = false;
			SDL_HideWindow(mainprogram->config_midipresetswindow);
		}
	}
	render_text("CANCEL", white, box->vtxcoords->x1 + 0.02f, box->vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, 2);
	box->vtxcoords->x1 = 0.60f;
	box->upvtxtoscr();
	draw_box(white, black, box, -1);
	if (box->in(mx, my)) {
		draw_box(white, lightblue, box, -1);
		if (mainprogram->leftmouse) {
			save_genmidis(mainprogram->docpath + "midiset.gm");
			mainprogram->tmlearn = TM_NONE;
			mainprogram->midipresets = false;
			SDL_HideWindow(mainprogram->config_midipresetswindow);
		}
	}
	render_text("SAVE", white, box->vtxcoords->x1 + 0.02f, box->vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, 2);

	mainprogram->tooltips_handle(2);

	mainprogram->bvao = mainprogram->boxvao;
	mainprogram->bvbuf = mainprogram->boxvbuf;
	mainprogram->btbuf = mainprogram->boxtbuf;
	mainprogram->middlemouse = false;
	mainprogram->rightmouse = false;
	mainprogram->menuactivation = false;
	mainprogram->mx = -1;
	mainprogram->my = -1;

	mainprogram->insmall = false;

    mainprogram->directmode = false;

	return 1;
}


bool Program::config_midipresets_init() {
	if (mainprogram->midipresets) {
		mainprogram->directmode = true;
		if (mainprogram->waitmidi) {
			clock_t t = clock() - mainprogram->stt;
			double time_taken = ((double)t) / CLOCKS_PER_SEC; // in seconds
			if (time_taken > 0.1f) {
				mainprogram->waitmidi = 2;
				mycallback(0.0f, &mainprogram->savedmessage, mainprogram->savedmidiitem);
				mainprogram->waitmidi = 0;
			}
		}
		SDL_GL_MakeCurrent(mainprogram->config_midipresetswindow, glc);
        SDL_RaiseWindow(mainprogram->config_midipresetswindow);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDrawBuffer(GL_BACK_LEFT);
        glViewport(0, 0, glob->w / 2.0f, glob->h / 2.0f);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
		bool ret = mainprogram->config_midipresets_handle();
		if (ret) {
			SDL_GL_SwapWindow(mainprogram->config_midipresetswindow);
		}
		SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDrawBuffer(GL_BACK_LEFT);
        glViewport(0, 0, glob->w, glob->h);
        mainprogram->directmode = false;
	}
	return true;
}

void Program::pick_color(Layer* lay, Boxx* cbox) {
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
				Boxx* box = mainprogram->cwbox;
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
					if (length > 0.75f && length < 1.0f) {
						mainprogram->cwmouse = 0;
					}
					else mainprogram->cwmouse = 3;
				}
				else {
					if (length > 0.75f && length < 1.0f) {
						GLint cwmouse = glGetUniformLocation(mainprogram->ShaderProgram, "cwmouse");
						glUniform1i(cwmouse, 1);
						mainprogram->cwmouse = 2;
						GLfloat mx = glGetUniformLocation(mainprogram->ShaderProgram, "mx");
						glUniform1f(mx, mainprogram->mx);
						GLfloat my = glGetUniformLocation(mainprogram->ShaderProgram, "my");
						glUniform1f(my, mainprogram->my);
					}
					else if (mainprogram->cwmouse == 3) {
						mainprogram->cwmouse = 0;
						GLint cwmouse = glGetUniformLocation(mainprogram->ShaderProgram, "cwmouse");
						glUniform1i(cwmouse, 0);
						lay->cwon = false;
						mainprogram->cwon = false;
					}
				}
			}
			if (lay->cwon) {
				GLfloat globw = glGetUniformLocation(mainprogram->ShaderProgram, "globw");
				glUniform1f(globw, glob->w);
				GLfloat globh = glGetUniformLocation(mainprogram->ShaderProgram, "globh");
				glUniform1f(globh, glob->h);
				GLfloat cwx = glGetUniformLocation(mainprogram->ShaderProgram, "cwx");
				glUniform1f(cwx, mainprogram->cwx);
				GLfloat cwy = glGetUniformLocation(mainprogram->ShaderProgram, "cwy");
				glUniform1f(cwy, mainprogram->cwy);
				GLint cwon = glGetUniformLocation(mainprogram->ShaderProgram, "cwon");
				glUniform1i(cwon, 1);
				printf("x %f\n", 8.0f * (3500 - (mainprogram->cwx * mainprogram->ow)) / mainprogram->oh);
				printf("y %f\n", -8.0f * (400 - (mainprogram->cwy * mainprogram->oh)) / mainprogram->oh);
				Boxx* box = mainprogram->cwbox;
				mainprogram->directmode = true;
				draw_box(nullptr, box->acolor, box->vtxcoords->x1, box->vtxcoords->y1, box->vtxcoords->w, box->vtxcoords->h, -1);
				mainprogram->directmode = false;
				glUniform1i(cwon, 0);
				if (length <= 0.75f || length >= 1.0f) {
				    glReadBuffer(GL_COLOR_ATTACHMENT0);
					glReadPixels(mainprogram->mx, glob->h - mainprogram->my, 1, 1, GL_RGBA, GL_FLOAT, &lay->rgb);
					box = cbox;
                    if (lay->blendnode->blendtype == CHROMAKEY) {
                        rgb c1;
                        c1.r = lay->rgb[0];
                        c1.g = lay->rgb[1];
                        c1.b = lay->rgb[2];
                        hsv c2 = rgb2hsv(c1);
                        c2.s = 1.0f;
                        c2.v = 1.0f;
                        c1 = hsv2rgb(c2);
                        lay->rgb[0] = c1.r;
                        lay->rgb[1] = c1.g;
                        lay->rgb[2] = c1.b;
                    }
                    else if (lay->blendnode->blendtype == LUMAKEY) {
                        rgb c1;
                        c1.r = lay->rgb[0];
                        c1.g = lay->rgb[1];
                        c1.b = lay->rgb[2];
                        hsv c2 = rgb2hsv(c1);
                        c2.s = 0.0f;
                        c2.h = 0.0f;
                        c1 = hsv2rgb(c2);
                        lay->rgb[0] = c1.r;
                        lay->rgb[1] = c1.g;
                        lay->rgb[2] = c1.b;
                    }
					box->acolor[0] = lay->rgb[0];
					box->acolor[1] = lay->rgb[1];
					box->acolor[2] = lay->rgb[2];
					box->acolor[3] = 1.0f;
				}
			}
		}
	}
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
	unsigned long vlen = 0;
	unsigned long flen = 0;
	char *VShaderSource;
 	char *vshader = (char*)malloc(100);
 	#ifdef WINDOWS
 	if (exists("./shader.vs")) strcpy (vshader, "./shader.vs");
 	else mainprogram->quitting = "Unable to find vertex shader \"shader.vs\" in current directory";
 	#else
    #ifdef POSIX
    std::string ddir (this->docpath);
    if (exists("./shader.vs")) {
	    strcpy (vshader, "./shader.vs");
    }
    else if (exists("/usr/share/ewocvj2/shader.vs")) strcpy (vshader, "/usr/share/ewocvj2/shader.vs");
 	else mainprogram->quitting = "Unable to find vertex shader \"shader.vs\" in " + ddir;
 	#endif
 	#endif
 	load_shader(vshader, &VShaderSource, vlen);
	char *FShaderSource;
 	char *fshader = (char*)malloc(100);
 	#ifdef WINDOWS
 	if (exists("./shader.fs")) strcpy (fshader, "./shader.fs");
 	else mainprogram->quitting = "Unable to find fragment shader \"shader.fs\" in current directory";
 	#else
 	#ifdef POSIX
    if (exists("./shader.fs")) strcpy (fshader, "./shader.fs");
 	else if (exists("/usr/share/ewocvj2/shader.fs")) strcpy (fshader, "/usr/share/ewocvj2/shader.fs");
 	else mainprogram->quitting = "Unable to find fragment shader \"shader.fs\" in " + ddir;
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

    free(VShaderSource);
    free(FShaderSource);
    free(vshader);
    free(fshader);

	return program;
}




//  THINGS PROJECT RELATED



void Project::delete_dirs(std::string path) {
	//std::string dir = remove_extension(path);
	std::filesystem::directory_entry d{ path };
    for (auto &it : std::filesystem::directory_iterator(path)) {
        //std::string pp = it.path().stem().string();
        if (dirname(mainprogram->project->bupp) == dirname(mainprogram->project->bupp)) {
            std::string clearpath = it.path().string();
            if (clearpath.find("autosave") != std::string::npos) {
                continue;
            }
        }
        std::filesystem::remove_all(it.path());
    }
	for (int i = 0; i < binsmain->bins.size(); i++) {
		binsmain->bins[i]->path = path + "/bins/" + binsmain->bins[i]->name + ".bin";
	}
	binsmain->save_binslist();
}

void Project::copy_dirs(std::string path) {
    std::string src = pathtoplatform(dirname(this->path));
    std::string dest = pathtoplatform(path);
    copy_dir(src, dest);
    std::filesystem::remove(path + "/" + basename(this->path));
    this->binsdir = path + "/bins/";
    this->recdir = path + "/recordings/";
    this->shelfdir = path + "/shelves/";
    this->autosavedir = path + "/autosaves/";
    this->elementsdir = path + "/elements/";
}

void Project::create_dirs(const std::string path) {
    std::string dir = path;
    this->binsdir = dir + "bins/";
    this->recdir = dir + "recordings/";
    this->shelfdir = dir + "shelves/";
    this->autosavedir = dir + "autosaves/";
    this->elementsdir = dir + "elements/";
    std::filesystem::path d{ dir };
    std::filesystem::create_directory(d);
    std::filesystem::path p1{ this->binsdir };
    std::filesystem::create_directory(p1);
    std::filesystem::path p2{ this->recdir };
    std::filesystem::create_directory(p2);
    std::filesystem::path p3{ this->shelfdir };
    std::filesystem::create_directory(p3);
    std::filesystem::path p4{ this->autosavedir };
    std::filesystem::create_directory(p4);
    std::filesystem::path p5{ this->elementsdir };
    std::filesystem::create_directory(p5);

    std::filesystem::create_directory(this->autosavedir + "temp/");
    std::filesystem::create_directory(this->autosavedir + "temp/bins");
}

void Project::create_dirs_autosave(const std::string path) {
    std::string dir = path;
    std::filesystem::rename(mainprogram->project->autosavedir + "temp", path);
    std::string buad = mainprogram->project->autosavedir;
    this->binsdir = dir + "/bins/";
    this->shelfdir = dir + "/shelves/";
    this->autosavedir = dir + "/autosaves/";
    this->recdir = dir + "recordings/";
    this->elementsdir = dir + "elements/";
    std::filesystem::path d{ dir };
    std::filesystem::create_directory(d);
    std::filesystem::path p1{ dir + "/bins/" };
    std::filesystem::create_directory(p1);
    std::filesystem::path p2{ dir + "/autosaves/" };
    std::filesystem::create_directory(p2);
    std::filesystem::path p3{ dir + "/shelves/" };
    std::filesystem::create_directory(p3);
    std::filesystem::path p4{ dir + "/recordings/" };
    std::filesystem::create_directory(p4);
    std::filesystem::path p5{ dir + "/elements/" };
    std::filesystem::create_directory(p5);

    std::filesystem::create_directory(buad + "temp/");
    std::filesystem::create_directory(buad + "temp/bins");
}

void Project::newp(const std::string path) {
	std::string ext = path.substr(path.length() - 7, std::string::npos);
	std::string str;
	std::string path2;
	if (ext != ".ewocvj") {
	    str = path + ".ewocvj";
	    path2 = path;
	}
	else {
	    path2 = path.substr(0, path.size() - 7);
        str = path2 + "/" + basename(path) + ".ewocvj";
	}
	this->path = str;
	this->name = remove_extension(basename(this->path));

    mainprogram->newproject2 = true;

    // set project output width and height
    for (PrefCat *item : mainprogram->prefs->items) {
        // get the default output width and height
        for (PrefItem *pri : item->items) {
            if (pri->dest == &mainprogram->sow) {
                this->ow = pri->value;
            } else if (pri->dest == &mainprogram->soh) {
                this->oh = pri->value;
            }
        }
    }
    for (PrefCat *item : mainprogram->prefs->items) {
        // set the preferences destinations for project output width and height
        for (PrefItem *pri : item->items) {
            if (pri->dest == &mainprogram->project->ow) {
                pri->dest = &this->ow;
                pri->value = this->ow;
            } else if (pri->dest == &mainprogram->project->oh) {
                pri->dest = &this->oh;
                pri->value = this->oh;
            }
        }
    }

	this->create_dirs(dirname(path2));
	for (int i = 0; i < binsmain->bins.size(); i++) {
		delete binsmain->bins[i];
	}
	binsmain->bins.clear();
	binsmain->new_bin("this is a bin");
    binsmain->save_bin(binsmain->bins[0]->path);
	binsmain->save_binslist();
	mainprogram->shelves[0]->erase();
	mainprogram->shelves[1]->erase();
	mainprogram->shelves[0]->basepath = "shelfsA";
	mainprogram->shelves[1]->basepath = "shelfsB";
	mainprogram->shelves[0]->save(mainprogram->project->shelfdir + "shelfsA");
	mainprogram->shelves[1]->save(mainprogram->project->shelfdir + "shelfsB");
    mainmix->currlays[0].clear();
    mainmix->currlays[1].clear();
    std::vector<Layer*> &lvec = choose_layers(0);
    mainmix->currlays[!mainprogram->prevmodus].push_back(lvec[0]);
    mainmix->currlay[!mainprogram->prevmodus] = lvec[0];
    mainprogram->prevmodus = !mainprogram->prevmodus;
    lvec = choose_layers(0);
    mainmix->currlays[!mainprogram->prevmodus].push_back(lvec[0]);
    mainmix->currlay[!mainprogram->prevmodus] = lvec[0];
    mainprogram->prevmodus = !mainprogram->prevmodus;
    mainprogram->project->save(this->path);
}
	
bool Project::open(std::string path, bool autosave, bool newp, bool undo) {
	std::string result = mainprogram->deconcat_files(path);
	bool concat = (result != "");
	std::ifstream rfile;
	if (concat) rfile.open(result);
	else rfile.open(path);

    if (autosave) {
        for (Bin *bin : binsmain->bins) {
            mainprogram->oldbins.push_back(bin->path);
        }
        mainprogram->inautosave = true;
    }

	void **namedest;
	void **owdest;
	void **ohdest;
    for (PrefCat *item : mainprogram->prefs->items) {
        for (PrefItem *mci : item->items) {
            if (mci->dest == &mainprogram->project->name) {
                namedest = &mci->dest;
            } else if (mci->dest == &mainprogram->project->ow) {
                owdest = &mci->dest;
            } else if (mci->dest == &mainprogram->project->oh) {
                ohdest = &mci->dest;
            }
        }
    }

    mainprogram->project->path = path;
    if (!exists(path)) {  // reminder requester
        std::string err = "Project at " + path + " doesn't exist" + "\n";
        printf(err.c_str());
        return false;
    }

    if (!newp) {
        // remove unsaved bins
        std::vector<Bin *> bins = binsmain->bins;
        int correct = 0;
        for (int i = 0; i < bins.size(); i++) {
            std::filesystem::remove(bins[i]->path);
            std::filesystem::remove_all(mainprogram->project->binsdir + bins[i]->name);
            correct++;
        }
        binsmain->save_binslist();
        if (binsmain->bins.size() == 0) {
            std::filesystem::remove(mainprogram->project->binsdir + "bins.list");
        }
    }

    mainprogram->project->name = remove_extension(basename(path));
        *namedest = &mainprogram->project->name;
        std::string dir = dirname(path);
        this->binsdir = dir + "bins/";
        this->recdir = dir + "recordings/";
        this->shelfdir = dir + "shelves/";
        this->autosavedir = dir + "autosaves/";
        this->elementsdir = dir + "elements/";
        if (!exists(mainprogram->currfilesdir)) mainprogram->currfilesdir = this->elementsdir;
        mainprogram->currelemsdir = this->elementsdir;
    //}
    std::string bubinsdir = mainprogram->project->binsdir;
    if (autosave) {
        mainprogram->project->binsdir = mainprogram->autosavebinsdir;
    }

    if (!undo) {
        int cb = binsmain->read_binslist();
        for (int i = 0; i < binsmain->bins.size(); i++) {
            std::string binname = this->binsdir + binsmain->bins[i]->name + ".bin";
            if (exists(binname)) {
                binsmain->open_bin(binname, binsmain->bins[i]);
            }
        }
        mainprogram->project->binsdir = bubinsdir;
        if (binsmain->bins.size()) {
            binsmain->make_currbin(cb);
        }
        else {
            binsmain->new_bin("this is a bin");
            binsmain->make_currbin(0);
        }
    }

    mainprogram->set_ow3oh3();
    //mainmix->new_state();
    mainmix->open_state(result + "_0.file", undo);

    std::string istring;
	safegetline(rfile, istring);
	
	while (safegetline(rfile, istring)) {
		if (istring == "ENDOFFILE") break;
        if (istring == "PREVMODUS") {
            safegetline(rfile, istring);
            mainprogram->prevmodus = std::stoi(istring);
        }
        if (istring == "BINSSCREEN") {
            safegetline(rfile, istring);
            mainprogram->binsscreen = std::stoi(istring);
        }
        if (istring == "SWAPSCROLLPOS") {
            safegetline(rfile, istring);
            mainmix->swapscrollpos[0] = std::stoi(istring);
            safegetline(rfile, istring);
            mainmix->swapscrollpos[1] = std::stoi(istring);
        }
        if (istring == "OUTPUTWIDTH") {
            safegetline(rfile, istring);
            this->ow = std::stoi(istring);
            *owdest = &this->ow;
            ((PrefItem*)(*owdest))->value = this->ow;
            mainprogram->ow = this->ow;
        }
		else if (istring == "OUTPUTHEIGHT") {
			safegetline(rfile, istring);
			this->oh = std::stoi(istring);
            *ohdest = &this->oh;
            ((PrefItem*)(*ohdest))->value = this->oh;
            mainprogram->oh = this->oh;
		}
		/*if (istring == "CURRSHELFA") {
			safegetline(rfile, istring);
			if (istring != "") {
				mainprogram->shelves[0]->open(this->shelfdir + istring + "/" + istring);
				mainprogram->shelves[0]->basepath = istring;
			}
			else {
				mainprogram->shelves[0]->erase();
			}
		}
		if (istring == "CURRSHELFB") {
			safegetline(rfile, istring);
			if (istring != "") {
				mainprogram->shelves[1]->open(this->shelfdir + istring + "/" + istring);
				mainprogram->shelves[1]->basepath = istring;
			}
			else {
				mainprogram->shelves[1]->erase();
			}
		}*/

		if (istring == "CURRBINSDIR") {
			safegetline(rfile, istring);
			mainprogram->currbinsdir = istring;
		}
		if (istring == "CURRSHELFDIR") {
			safegetline(rfile, istring);
			mainprogram->currshelfdir = istring;
		}
		if (istring == "CURRRECDIR") {
			safegetline(rfile, istring);
			mainprogram->currrecdir = istring;
		}
	}
    mainprogram->set_ow3oh3();
    mainprogram->handle_changed_owoh();

	rfile.close();

	if (!autosave && !undo) {
        int pos = std::find(mainprogram->recentprojectpaths.begin(), mainprogram->recentprojectpaths.end(), path) -
                  mainprogram->recentprojectpaths.begin();
        mainprogram->recentprojectpaths.insert(mainprogram->recentprojectpaths.begin(), path);
        if (pos < mainprogram->recentprojectpaths.size() - 1) {
            mainprogram->recentprojectpaths.erase(mainprogram->recentprojectpaths.begin() + pos + 1);
        }
        mainprogram->write_recentprojectlist();
    }

    if (!undo) {
        // for initial bin screen entry speedup
        //mainprogram->rightmouse = true;
        binsmain->handle(1);
        //mainprogram->rightmouse = false;
    }

    std::vector<LoopStation*> lpsts;
    lpsts.push_back(lp);
    lpsts.push_back(lpc);
    for (int m = 0; m < 2; m++) {
        for (int k = 0; k < 4; k++) {
            lpsts.push_back(mainmix->scenes[m][k]->lpst);
        }
    }
    for (LoopStation *lpst : lpsts) {
        // correct loopstation current times for deck saving/opening lag
        LoopStation *bunowlpst = lpst;
        std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
        for (LoopStationElement *elem: lpst->odelems) {
        	std::chrono::system_clock::time_point::duration back = std::chrono::milliseconds((long long) (elem->interimtime));
            elem->starttime = now - back;
        }
    }
	return true;
}

void Project::save(std::string path, bool autosave, bool undo) {
    if (undo) {
        mainprogram->undoing = true;
    }

    if (undo) {
        mainprogram->concatlimit = 10;
    }
    else {
        mainprogram->concatlimit = 12;
    }
    mainprogram->goconcat = false;

    // save project file: if autosave is true
	std::string ext = path.substr(path.length() - 7, std::string::npos);
	std::string str;
	if (ext != ".ewocvj") str = path + ".ewocvj";
	else str = path;
	std::ofstream wfile;
	wfile.open(str);
	std::vector<std::string> filestoadd;

    if (mainprogram->inautosave) {
        for (std::string binpath : mainprogram->oldbins) {
            std::filesystem::remove(binpath);
            std::filesystem::remove_all(binpath.substr(0, binpath.length() - 4));
        }
        for (Bin *bin : binsmain->bins) {
            std::filesystem::copy(mainprogram->autosavebinsdir, mainprogram->project->binsdir, std::filesystem::copy_options::skip_existing | std::filesystem::copy_options::recursive);

            for (int k = 0; k < binsmain->bins.size(); k++) {
                std::vector<std::string> bup;
                for (int j = 0; j < 12; j++) {
                    for (int i = 0; i < 12; i++) {
                        BinElement *binel = binsmain->bins[k]->elements[i * 12 + j];
                        std::string s =
                                mainprogram->project->binsdir + "/bins/" + binsmain->bins[k]->name + "/";
                        if (binel->absjpath != "") {
                            binel->absjpath = s + basename(binel->absjpath);
                            binel->jpegpath = binel->absjpath;
                            binel->reljpath = std::filesystem::relative(binel->absjpath, s).generic_string();
                        }
                    }
                }
            }
        }
        mainprogram->inautosave = false;
    }

	wfile << "EWOCvj PROJECT V0.1\n";
    wfile << "PREVMODUS\n";
    wfile << std::to_string(mainprogram->prevmodus);
    wfile << "\n";
    wfile << "BINSSCREEN\n";
    wfile << std::to_string(mainprogram->binsscreen);
    wfile << "\n";
    wfile << "SWAPSCROLLPOS\n";
    wfile << std::to_string(mainmix->swapscrollpos[0]);
    wfile << "\n";
    wfile << std::to_string(mainmix->swapscrollpos[1]);
    wfile << "\n";
    wfile << "OUTPUTWIDTH\n";
    wfile << std::to_string((int)this->ow);
    wfile << "\n";
	wfile << "OUTPUTHEIGHT\n";
	wfile << std::to_string((int)this->oh);
	wfile << "\n";
	wfile << "CURRBINSDIR\n";
	wfile << mainprogram->currbinsdir;
	wfile << "\n";
	wfile << "CURRSHELFDIR\n";
	wfile << mainprogram->currshelfdir;
	wfile << "\n";
	wfile << "CURRRECDIR\n";
	wfile << mainprogram->currrecdir;
	wfile << "\n";

	wfile.close();

    while(!autosave && mainprogram->autosaving) {
        // wait for possible current autosaving to end
        Sleep(10);
    }

    // save main state file: is concatted in project file
	mainmix->save_state(mainprogram->temppath + "current.state", false, undo);
	filestoadd.push_back(mainprogram->temppath + "current.state");

    if (!undo) {
        //save bins
        int cbin = binsmain->currbin->pos;
        for (int i = 0; i < binsmain->bins.size(); i++) {
            binsmain->make_currbin(i);
            binsmain->save_bin(dirname(path) + "bins/" + basename(binsmain->bins[i]->path));
        }
        binsmain->make_currbin(cbin);
        binsmain->save_binslist();
    }

    // concat everyting in project file except for bins, they are saved separately
    std::vector<std::vector<std::string>> filestoadd2;
	filestoadd2.push_back(filestoadd);
    std::thread concat = std::thread(&Program::concat_files, mainprogram, mainprogram->temppath + "tempconcatproj",
                                     str, filestoadd2);
    concat.detach();

	if (!autosave && !undo) {
        // remember recent project files
        if (std::find(mainprogram->recentprojectpaths.begin(), mainprogram->recentprojectpaths.end(), str) ==
            mainprogram->recentprojectpaths.end()) {
            mainprogram->recentprojectpaths.insert(mainprogram->recentprojectpaths.begin(), str);
            while (mainprogram->recentprojectpaths.size() > 10) {
                mainprogram->recentprojectpaths.pop_back();
            }
#ifdef WINDOWS
            std::string dir = mainprogram->docpath;
#else
#ifdef POSIX
            std::string homedir(getenv("HOME"));
            std::string dir = homedir + "/.ewocvj2/";
#endif
#endif
            do {
                wfile.open(dir + "recentprojectslist");
            } while (!wfile.is_open());
            if (!wfile.fail()) {
                wfile << "EWOCvj RECENTPROJECTS V0.1\n";
                for (int i = 0; i < mainprogram->recentprojectpaths.size(); i++) {
                    wfile << mainprogram->recentprojectpaths[i];
                    wfile << "\n";
                }
                wfile << "ENDOFFILE\n";
                wfile.close();
            }
            else {
                printf("Failed opening recentprojectlist\n");
;            }
        }
    }

    if (!undo) {
        //remove redundant bin files
        for (Bin *bin: binsmain->bins) {
            for (BinElement *elem: bin->elements) {
                if (elem->jpegpath != "") {
                    binsmain->removeset[0].erase(elem->path);
                    binsmain->removeset[0].erase(elem->jpegpath);
                }
            }
        }
        for (auto &it: binsmain->removeset[0]) {
            std::filesystem::remove(it);
        }
    }
}

void Project::save_as() {
    std::string path2;
    std::string str;
    std::vector<std::vector<std::string>> bupaths;
    if (dirname(mainprogram->path) != "") {
        std::string oldprdir = mainprogram->project->name;
        mainprogram->currprojdir = dirname(mainprogram->path);
        std::string ext = mainprogram->path.substr(mainprogram->path.length() - 7, std::string::npos);
        if (ext != ".ewocvj") {
            str = mainprogram->path + "/" + basename(mainprogram->path) + ".ewocvj";
            path2 = mainprogram->path;
        } else {
            path2 = dirname(mainprogram->path.substr(0, mainprogram->path.size() - 7));
            path2 = path2.substr(0, path2.size() - 1);
            str = mainprogram->path;
            mainprogram->path = path2;
        }

        if (!exists(path2)) {
            mainprogram->project->copy_dirs(path2);
        } else {
            mainprogram->project->delete_dirs(path2);
            mainprogram->project->copy_dirs(path2);
        }

        if (mainprogram->project->bupp != "") {
            mainprogram->project->binsdir = mainprogram->project->bubd;
            mainprogram->project->shelfdir = mainprogram->project->busd;
            mainprogram->project->recdir = mainprogram->project->burd;
            mainprogram->project->autosavedir = mainprogram->project->buad;
            mainprogram->project->elementsdir = mainprogram->project->bued;
            mainprogram->project->path = mainprogram->project->bupp;
            mainprogram->project->name = mainprogram->project->bupn;
        }

        if (!std::filesystem::is_empty(mainprogram->path + "/autosaves/")) {
            // adapt autosave entries
            std::unordered_map<std::string, std::string> smap;
            // change autosave directory names
            for (const auto &dirEnt: std::filesystem::directory_iterator{
                    mainprogram->path + "/autosaves/"}) {
                const auto &path = dirEnt.path();
                auto pathstr = path.generic_string();
                if (basename(pathstr) == "autosavelist") continue;
                size_t start_pos = pathstr.rfind(oldprdir);
                if (start_pos == std::string::npos) continue;
                std::string newstr = pathstr;
                newstr.replace(start_pos, oldprdir.length(), basename(path2));
                smap[pathstr] = newstr;
            }
            std::unordered_map<std::string, std::string>::iterator it;
            for (it = smap.begin(); it != smap.end(); it++) {
                std::filesystem::rename(it->first, it->second);
            }
            // change autosave project file names
            smap.clear();
            for (const auto &dirEnt: std::filesystem::recursive_directory_iterator{
                    mainprogram->project->autosavedir}) {
                const auto &path = dirEnt.path();
                //if (std::filesystem::is_directory(path)) continue;
                auto pathstr = path.generic_string();
                std::string ext2 = pathstr.substr(pathstr.length() - 7, std::string::npos);
                if (ext2 != ".ewocvj") continue;
                size_t start_pos = pathstr.rfind(oldprdir);
                if (start_pos == std::string::npos) continue;
                std::string newstr = pathstr;
                newstr.replace(start_pos, oldprdir.length(), basename(path2));
                smap[pathstr] = newstr;
            }
            for (it = smap.begin(); it != smap.end(); it++) {
                std::filesystem::rename(it->first, it->second);
            }
        }
        /*std::string src = pathtoplatform(mainprogram->project->binsdir + binsmain->currbin->name + "/");
        std::string dest = pathtoplatform(dirname(mainprogram->path) + remove_extension(basename(mainprogram->path)) + "/");
        copy_dir(src, dest);*/
        for (int k = 0; k < binsmain->bins.size(); k++) {
            std::vector<std::string> bup;
            for (int j = 0; j < 12; j++) {
                for (int i = 0; i < 12; i++) {
                    BinElement *binel = binsmain->bins[k]->elements[i * 12 + j];
                    std::string s =
                            mainprogram->path + "/bins/" + binsmain->bins[k]->name + "/";
                    bup.push_back(binel->absjpath);
                    if (binel->absjpath != "") {
                        binel->absjpath = s + basename(binel->absjpath);
                        binel->jpegpath = binel->absjpath;
                        binel->reljpath = std::filesystem::relative(binel->absjpath, s).generic_string();
                    }
                }
            }
            bupaths.push_back(bup);
        }
        /*for (int k = 0; k < 2; k++) {
            Shelf *shelf = mainprogram->shelves[k];
            std::vector<std::string> bus;
            for (int j = 0; j < 16; j++) {
                ShelfElement *elem = shelf->elements[j];
                std::string s =
                        mainprogram->path + remove_extension(basename(mainprogram->path)) +
                        "/";
                bus.push_back(elem->jpegpath);
                elem->jpegpath = s + basename(elem->jpegpath);
            }
            bupathsshelf.push_back(bus);
        }*/
    }
    if (exists(str)) {
        std::filesystem::remove(str);
    }
    // save project
    mainprogram->saveas = true;
    mainprogram->project->save(str, false);
    mainprogram->saveas = false;

    for (int k = 0; k < binsmain->bins.size(); k++) {
        for (int j = 0; j < 12; j++) {
            for (int i = 0; i < 12; i++) {
                BinElement *binel = binsmain->bins[k]->elements[i * 12 + j];
                //bupaths[k][i * 12 + j] = binel->absjpath;
                binel->absjpath = bupaths[k][i * 12 + j];
                binel->jpegpath = binel->absjpath;
                binel->reljpath = std::filesystem::relative(binel->absjpath,
                                                            mainprogram->project->binsdir).generic_string();
            }
        }
    }
    /*for (int k = 0; k < 2; k++) {
        Shelf *shelf = mainprogram->shelves[k];
        for (int j = 0; j < 16; j++) {
            ShelfElement *elem = shelf->elements[j];
            //bupaths[k][i * 12 + j] = binel->absjpath;
            elem->jpegpath = bupathsshelf[k][j];
        }
    }*/

    mainprogram->project->bupp = "";
    mainprogram->project->bupn = "";
    mainprogram->project->bubd = "";
    mainprogram->project->busd = "";
    mainprogram->project->burd = "";
    mainprogram->project->buad = "";
    mainprogram->project->bued = "";
}

void Project::autosave() {
    if (binsmain->openfilesbin || binsmain->importbins || mainprogram->openfilesshelf || mainprogram->openfileslayers || mainprogram->openfilesqueue || mainprogram->concatting) {
        return;
    }
    if (mainprogram->project->path.find("autosave") != std::string::npos) {
        mainmix->time = 0.0f;
        return;
    }
    bool found = false;
    for (Bin *bin : binsmain->bins) {
        if (bin->open_positions.size()) {
            found = true;
            break;
        }
    }
    if (found) return;
    for (Bin *bin : binsmain->bins) {
        for (BinElement *binel: bin->elements) {
            if (!binel->autosavejpegsaved && binel->path != "") {
                return;
            }
        }
    }

    bool bupm = mainprogram->prevmodus;

    mainprogram->astimestamp = (int) mainmix->time;

    std::string name = remove_extension(basename(this->path)) + "_0";
    std::string p1 = find_unused_filename("autosave_" + name, this->autosavedir, "");

    mainprogram->project->bupp = this->path;
    mainprogram->project->bupn = this->name;
    mainprogram->project->bubd = this->binsdir;
    mainprogram->project->busd = this->shelfdir;
    mainprogram->project->burd = this->recdir;
    mainprogram->project->buad = this->autosavedir;
    mainprogram->project->bued = this->elementsdir;
    mainprogram->project->create_dirs_autosave(p1);
    mainprogram->autosaving = true;
    std::vector<std::vector<std::string>> bupaths;
    for (int k = 0; k < binsmain->bins.size(); k++) {
        std::vector<std::string> bup;
        for (int j = 0; j < 12; j++) {
            for (int i = 0; i < 12; i++) {
                BinElement *binel = binsmain->bins[k]->elements[i * 12 + j];
                std::string s =
                        p1 + "/bins/" + binsmain->bins[k]->name + "/";
                bup.push_back(binel->absjpath);
                if (binel->absjpath != "") {
                    binel->absjpath = s + basename(binel->absjpath);
                    binel->jpegpath = binel->absjpath;
                    binel->reljpath = std::filesystem::relative(binel->absjpath, s).generic_string();
                }
            }
        }
        bupaths.push_back(bup);
    }

    printf("autosaving\n");

    mainprogram->project->save(p1 + "/" + basename(p1) + ".ewocvj", true);

    for (int k = 0; k < binsmain->bins.size(); k++) {
        for (int j = 0; j < 12; j++) {
            for (int i = 0; i < 12; i++) {
                BinElement *binel = binsmain->bins[k]->elements[i * 12 + j];
                //bupaths[k][i * 12 + j] = binel->absjpath;
                binel->absjpath = bupaths[k][i * 12 + j];
                binel->jpegpath = binel->absjpath;
                binel->reljpath = std::filesystem::relative(binel->absjpath,
                                                            mainprogram->project->binsdir).generic_string();
            }
        }
    }
    mainprogram->autosaving = false;
    this->binsdir = mainprogram->project->bubd;
    this->shelfdir = mainprogram->project->busd;
    this->recdir = mainprogram->project->burd;
    this->autosavedir = mainprogram->project->buad;
    this->elementsdir = mainprogram->project->bued;
    this->path = mainprogram->project->bupp;
    this->name = mainprogram->project->bupn;

    mainprogram->prevmodus = bupm;
}



//  THINGS PREFERENCES RELATED


Preferences::Preferences() {
    PIInvisible *piin = new PIInvisible;
    this->items.push_back(piin);
    PIProj *pipr = new PIProj;
    pipr->box = new Boxx;
    pipr->box->smflag = 1;
    pipr->box->tooltiptitle = "Project settings ";
    pipr->box->tooltip = "Left click to set project related preferences ";
    this->items.push_back(pipr);
    PIVid *pvi = new PIVid;
    pvi->box = new Boxx;
    pvi->box->smflag = 1;
    pvi->box->tooltiptitle = "Video settings ";
    pvi->box->tooltip = "Left click to set video related preferences ";
    this->items.push_back(pvi);
    PIInt *pii = new PIInt;
    pii->box = new Boxx;
    pii->box->smflag = 1;
    pii->box->tooltiptitle = "Interface settings ";
    pii->box->tooltip = "Left click to set interface related preferences ";
    this->items.push_back(pii);
    PIProg *pip = new PIProg;
    pip->box = new Boxx;
    pip->box->smflag = 1;
    pip->box->tooltiptitle = "Program settings ";
    pip->box->tooltip = "Left click to set program related preferences ";
    this->items.push_back(pip);
    PIDirs *pidirs = new PIDirs;
    pidirs->box = new Boxx;
    pidirs->box->smflag = 1;
    pidirs->box->tooltiptitle = "Directory settings ";
    pidirs->box->tooltip = "Left click to set default directories ";
    this->items.push_back(pidirs);
    PIMidi *pimidi = new PIMidi;
    pimidi->populate();
    pimidi->box = new Boxx;
    pimidi->box->smflag = 1;
    pimidi->box->tooltiptitle = "MIDI device settings ";
    pimidi->box->tooltip = "Left click to set MIDI device related preferences ";
    this->items.push_back(pimidi);
    for (int i = 0; i < this->items.size(); i++) {
        PrefCat *item = this->items[i];
        if (item->name == "Invisible") continue;
        item->box->smflag = 1;
        item->box->vtxcoords->x1 = -1.0f;
        item->box->vtxcoords->y1 = 1.0f - (i + 1) * 0.2f;
        item->box->vtxcoords->w = 0.5f;
        item->box->vtxcoords->h = 0.2f;
    }
}

void Preferences::load() {
    std::ifstream rfile;
#ifdef WINDOWS
    std::string prstr = mainprogram->docpath + "preferences.prefs";
#else
#ifdef POSIX
    std::string homedir (getenv("HOME"));
    std::string prstr = homedir + "/.ewocvj2/preferences.prefs";
#endif
#endif
    if (!exists(prstr)) return;
    rfile.open(prstr);
    std::string istring;
    safegetline(rfile, istring);
    while (safegetline(rfile, istring)) {
        if (istring == "ENDOFFILE") break;

        else if (istring == "PREFCAT") {
            while (safegetline(rfile, istring)) {
                if (istring == "ENDOFPREFCAT") break;
                bool brk = false;
                for (int i = 0; i < mainprogram->prefs->items.size(); i++) {
                    if (mainprogram->prefs->items[i]->name == istring) {
                        std::string catname = istring;
                        if (istring == "MIDI Devices") ((PIMidi*)(mainprogram->prefs->items[i]))->populate();
                        while (safegetline(rfile, istring)) {
                            if (istring == "ENDOFPREFCAT") {
                                brk = true;
                                break;
                            }
                            int foundpos = -1;
                            PrefItem *pi = nullptr;
                            for (int j = 0; j < mainprogram->prefs->items[i]->items.size(); j++) {
                                pi = mainprogram->prefs->items[i]->items[j];
                                if (pi->name == istring && pi->connected) {
                                    foundpos = j;
                                    safegetline(rfile, istring);
                                    if (pi->type == PREF_ONOFF) {
                                        pi->onoff = std::stoi(istring);
                                        if (pi->dest) *(bool*)pi->dest = (bool)pi->onoff;
                                    }
                                    else if (pi->type == PREF_NUMBER) {
                                        pi->value = std::stof(istring);
                                        if (pi->dest) *(float*)pi->dest = (float)pi->value;
                                    }
                                    else if (pi->type == PREF_STRING) {
                                        pi->str = istring;
                                        if (pi->dest) *(std::string*)pi->dest = pi->str;
                                    }
                                    else if (pi->type == PREF_PATH) {
                                        std::filesystem::path p(istring);
                                        if (!std::filesystem::exists(p)) {
                                            pi->path = *(std::string*)pi->dest;
                                            continue;
                                        }
                                        pi->path = istring;
                                        std::string lastchar = pi->path.substr(pi->path.length() - 1, std::string::npos);
                                        if (lastchar != "/" && lastchar != "\\") pi->path += "/";
                                        if (pi->dest) *(std::string*)pi->dest = pi->path;
                                    }
                                    else if (pi->type == PREF_PATHS) {
                                                if (istring == "PATHS") {
                                                    retarget->globalsearchdirs.clear();
                                                    while (safegetline(rfile, istring)) {
                                                        if (istring == "ENDPATHS") break;
                                                if (exists(istring)) {
                                                    retarget->globalsearchdirs.push_back(istring);
                                                }
                                            }
                                            retarget->searchboxes.clear();
                                            retarget->searchclearboxes.clear();
                                            retarget->searchglobalbuttons.clear();
                                            for (std::string p : retarget->globalsearchdirs) {
                                                make_searchbox();
                                            }
                                        }
                                    }
                                    break;
                                }
                            }
                            if (catname == "MIDI Devices") {
                                if (foundpos == -1) {
                                    std::string name = istring;
                                    safegetline(rfile, istring);
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
                                            if (std::find(mainprogram->openports.begin(), mainprogram->openports.end(), i) == mainprogram->openports.end()) {
                                                mainprogram->openports.erase(std::find(mainprogram->openports.begin(),
                                                                                       mainprogram->openports.end(),
                                                                                       foundpos));
                                            }
                                            pi->midiin->cancelCallback();
                                            delete pi->midiin;
                                        }
                                    }
                                    else {
                                        if (std::find(pim->onnames.begin(), pim->onnames.end(), pi->name) == pim->onnames.end()) {
                                            pim->onnames.push_back(pi->name);
                                            RtMidiIn *midiin = new RtMidiIn();
                                            if (std::find(mainprogram->openports.begin(), mainprogram->openports.end(),
                                                          foundpos) == mainprogram->openports.end()) {
                                                midiin->openPort(foundpos);
                                                midiin->setCallback(&mycallback, (void *) pim->items[foundpos]);
                                                mainprogram->openports.push_back(foundpos);
                                            }
                                            pi->midiin = midiin;
                                        }
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
            safegetline(rfile, istring);
            std::filesystem::path p(istring);
            if (std::filesystem::exists(p)) mainprogram->currfilesdir = istring;
        }
        else if (istring == "CURRELEMSDIR") {
            safegetline(rfile, istring);
            std::filesystem::path p(istring);
            if (std::filesystem::exists(p)) mainprogram->currelemsdir = istring;
        }
    }

    mainprogram->set_ow3oh3();
    rfile.close();
}

void Preferences::save() {
    std::ofstream wfile;
#ifdef WINDOWS
    std::string prstr = mainprogram->docpath + "preferences.prefs";
#else
#ifdef POSIX
    std::string homedir (getenv("HOME"));
    std::string prstr = homedir + "/.ewocvj2/preferences.prefs";
#endif
#endif
    wfile.open(prstr);
    wfile << "EWOCvj Preferences V0.2\n";

    for (PrefCat *cat : mainprogram->prefs->items) {
        if (cat->name == "Invisible") {
            for (PrefItem *item : cat->items) {
                if (item->name == "Server IP") {
                    item->str = mainprogram->serverip;
                }
                else if (item->name == "Seat name") {
                    item->str = mainprogram->seatname;
                }
            }
        }
    }

    std::vector<std::string> mds;
    for (int i = 0; i < mainprogram->prefs->items.size(); i++) {
        PrefCat *pc = mainprogram->prefs->items[i];
        wfile << "PREFCAT\n";
        wfile << pc->name;
        wfile << "\n";
        for (int j = 0; j < pc->items.size(); j++) {
            if (!pc->items[j]->onfile) continue;
            if (pc->name == "MIDI Devices") {
                if (std::find(mds.begin(), mds.end(), pc->items[j]->name) != mds.end()) {
                    // reminder : hacky fix for repeated MIDI device entries
                    continue;
                }
                mds.push_back(pc->items[j]->str);
            }
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
            else if (pc->items[j]->type == PREF_STRING) {
                wfile << pc->items[j]->str;
                wfile << "\n";
            }
            else if (pc->items[j]->type == PREF_PATH) {
                wfile << pc->items[j]->path;
                wfile << "\n";
            }
            else if (pc->items[j]->type == PREF_PATHS) {
                wfile << "PATHS\n";
                for (std::string path : retarget->globalsearchdirs) {
                    wfile << path;
                    wfile << "\n";
                }
                wfile << "ENDPATHS\n";
            }
        }
        wfile << "ENDOFPREFCAT\n";
    }

    wfile << "CURRFILESDIR\n";
    wfile << mainprogram->currfilesdir;
    wfile << "\n";
    wfile << "CURRELEMSDIR\n";
    wfile << mainprogram->currelemsdir;
    wfile << "\n";

    wfile << "ENDOFFILE\n";
    wfile.close();
}


PrefItem::PrefItem(PrefCat *cat, int pos, std::string name, PREF_TYPE type, void *dest) {
    this->name = name;
    this->type = type;
    this->dest = dest;
    this->namebox = new Boxx;
    this->namebox->smflag = 1;
    this->namebox->vtxcoords->x1 = -0.5f;
    this->namebox->vtxcoords->y1 = 1.0f - (pos + 1) * 0.2f;
    this->namebox->vtxcoords->w = 1.5f;
    if (type == PREF_STRING || type == PREF_PATH || type == PREF_PATHS) this->namebox->vtxcoords->w = 0.6f;
    this->namebox->vtxcoords->h = 0.2f;
    this->namebox->upvtxtoscr();
    this->valuebox = new Boxx;
    this->valuebox->smflag = 1;
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
    else if (type == PREF_STRING) {
        this->valuebox->vtxcoords->x1 = 0.1f;
        this->valuebox->vtxcoords->y1 = 1.0f - (pos + 1) * 0.2f;
        this->valuebox->vtxcoords->w = 0.8f;
        this->valuebox->vtxcoords->h = 0.2f;
    }
    else if (type == PREF_PATH) {
        this->valuebox->vtxcoords->x1 = 0.1f;
        this->valuebox->vtxcoords->y1 = 1.0f - (pos + 1) * 0.2f;
        this->valuebox->vtxcoords->w = 0.8f;
        this->valuebox->vtxcoords->h = 0.2f;
        this->iconbox = new Boxx;
        this->iconbox->smflag = 1;
        this->iconbox->vtxcoords->x1 = 0.9f;
        this->iconbox->vtxcoords->y1 = 1.0f - (pos + 1) * 0.2f;
        this->iconbox->vtxcoords->w = 0.1f;
        this->iconbox->vtxcoords->h = 0.2f;
        this->iconbox->upvtxtoscr();
    }
    else if (type == PREF_PATHS) {
        this->valuebox->vtxcoords->x1 = 0.1f;
        this->valuebox->vtxcoords->y1 = 1.0f - (pos + 1) * 0.2f;
        this->valuebox->vtxcoords->w = 0.7f;
        this->valuebox->vtxcoords->h = 0.2f;
        this->rembox = new Boxx;
        this->rembox->smflag = 1;
        this->rembox->vtxcoords->x1 = 0.8f;
        this->rembox->vtxcoords->y1 = 1.0f - (pos + 1) * 0.2f;
        this->rembox->vtxcoords->w = 0.1f;
        this->rembox->vtxcoords->h = 0.2f;
        this->rembox->upvtxtoscr();
        this->iconbox = new Boxx;
        this->iconbox->smflag = 1;
        this->iconbox->vtxcoords->x1 = 0.9f;
        this->iconbox->vtxcoords->y1 = 1.0f - (pos + 1) * 0.2f;
        this->iconbox->vtxcoords->w = 0.1f;
        this->iconbox->vtxcoords->h = 0.2f;
        this->iconbox->upvtxtoscr();
    }
    this->valuebox->upvtxtoscr();
}


PIProj::PIProj() {
    // Set all preferences items that appear under the Directories tab
    PrefItem *pip;
    int pos = 0;
    this->name = "Project";

    pip = new PrefItem(this, pos, "Project name", PREF_STRING, (void *) &mainprogram->project->name);
    pip->namebox->tooltiptitle = "Project name ";
    pip->namebox->tooltip = "Name of current project. ";
    pip->valuebox->tooltiptitle = "Set name of current project ";
    pip->valuebox->tooltip = "Leftclick starts keyboard entry of current project name. It also changes the name of "
                             "the project on disk. ";

    pip->onfile = false;  // isn't saved on main prefs file but in the project
    this->items.push_back(pip);
    pos++;

    pip = new PrefItem(this, pos, "Project output video width", PREF_NUMBER, (void*)&mainprogram->project->ow);
    pip->namebox->tooltiptitle = "Project output video width ";
    pip->namebox->tooltip = "Sets the width in pixels of the video stream sent to the output for this project. ";
    pip->valuebox->tooltiptitle = "Project output video width ";
    pip->valuebox->tooltip = "Leftclicking the value allows setting the width in pixels of the video stream sent to the output for this project. ";
    pip->onfile = false;  // isn't saved on main prefs file but in the project
    this->items.push_back(pip);
    pos++;

    pip = new PrefItem(this, pos, "Project output video height", PREF_NUMBER, (void*)&mainprogram->project->oh);
    pip->namebox->tooltiptitle = "Project output video height ";
    pip->namebox->tooltip = "Sets the height in pixels of the video stream sent to the output for this project. ";
    pip->valuebox->tooltiptitle = "Project output video height ";
    pip->valuebox->tooltip = "Leftclicking the value allows setting the height in pixels of the video stream sent to the output for this project. ";
    pip->onfile = false;  // isn't saved on main prefs file but in the project
    this->items.push_back(pip);
    pos++;
    
}


PIDirs::PIDirs() {
    // Set all preferences items that appear under the Directories tab
    PrefItem *pdi;
    int pos = 0;
    this->name = "Directories";

    pdi = new PrefItem(this, pos, "Projects", PREF_PATH, (void*)&mainprogram->projdir);
    pdi->namebox->tooltiptitle = "Projects directory ";
    pdi->namebox->tooltip = "Directory where new projects will be created in. ";
    pdi->valuebox->tooltiptitle = "Set projects directory ";
    pdi->valuebox->tooltip = "Leftclick starts keyboard entry of location of projects directory. ";
    pdi->iconbox->tooltiptitle = "Browse to set projects directory ";
    pdi->iconbox->tooltip = "Leftclick allows browsing for location of projects directory. ";
#ifdef WINDOWS
    pdi->path = mainprogram->docpath + "projects/";
#else
#ifdef POSIX
    pdi->path = mainprogram->homedir + "/Documents/EWOCvj2/projects/";
#endif
#endif
    mainprogram->projdir = pdi->path;
    this->items.push_back(pdi);
    pos++;


    pdi = new PrefItem(this, pos, "Content root", PREF_PATH, (void*)&mainprogram->contentpath);
    pdi->namebox->tooltiptitle = "Content root directory ";
    pdi->namebox->tooltip = "Root directory relative to which referenced content will be searched and where CPU compressed videos will be placed after HAP conversion. ";
    pdi->valuebox->tooltiptitle = "Set content root directory ";
    pdi->valuebox->tooltip = "Leftclick starts keyboard entry of location of content root directory. ";
    pdi->iconbox->tooltiptitle = "Browse to set content root directory ";
    pdi->iconbox->tooltip = "Leftclick allows browsing for location of content root directory. ";
#ifdef WINDOWS
    pdi->path = mainprogram->contentpath;
#else
#ifdef POSIX
    pdi->path = mainprogram->homedir + "/Videos/";
#endif
#endif
    this->items.push_back(pdi);
    pos++;


    pdi = new PrefItem(this, pos, "Default search", PREF_PATHS, (void*)&retarget->globalsearchdirs);
    pdi->namebox->tooltiptitle = "Default search directories ";
    pdi->namebox->tooltip = "Default search directories in which lost content will be searched. ";
    pdi->valuebox->tooltiptitle = "Set default search directories ";
    pdi->valuebox->tooltip = "Leftclick starts keyboard entry of location of this default search directory. ";
    pdi->iconbox->tooltiptitle = "Browse to set default search directory ";
    pdi->iconbox->tooltip = "Leftclick allows browsing for location of this default search directory. ";
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
    std::vector<std::string> allports;
    std::unordered_map<std::string, int> allmap;
    for (int i = 0; i < nPorts; i++) {
        // Set all preferences items that appear under the Midi Devices tab
        std::string nm = midiin.getPortName(i);
        int pos = nm.find_last_of(" ");
        nm = nm.substr(0, pos - 1);
        if (std::find(allports.begin(), allports.end(), nm) != allports.end()) {
            allmap[nm]++;
        }
        else {
            allports.push_back(nm);
            allmap[nm] = 1;
        }
        nm += " " + std::to_string(allmap[nm]);
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
                // erase from itemsleft to allow for multiple same names
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
    // Set all preferences items that appear under the Interface tab

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

    pii = new PrefItem(this, pos, "Autostart playback default", PREF_ONOFF, (void*)&mainprogram->autoplay);
    pii->onoff = 1;
    pii->namebox->tooltiptitle = "Autostart playback default ";
    pii->namebox->tooltip = "Sets autostarting video playback at video load as default. ";
    pii->valuebox->tooltiptitle = "Autostart playback default toggle ";
    pii->valuebox->tooltip = "Leftclick to set autostart video playback default to on(green) or off(black). ";
    mainprogram->autoplay = pii->onoff;
    this->items.push_back(pii);
    pos++;

    pii = new PrefItem(this, pos, "Looped playback default", PREF_ONOFF, (void*)&mainprogram->repeatdefault);
    pii->onoff = 1;
    pii->namebox->tooltiptitle = "Looped playback default ";
    pii->namebox->tooltip = "Sets looped video playback default. ";
    pii->valuebox->tooltiptitle = "Looped playback default toggle ";
    pii->valuebox->tooltip = "Leftclick to set looped video playback default to on(green) or off(black). ";
    mainprogram->repeatdefault = pii->onoff;
    this->items.push_back(pii);
    pos++;

    pii = new PrefItem(this, pos, "Keep effects on video change", PREF_ONOFF, (void*)&mainprogram->keepeffpref);
    pii->onoff = 0;
    pii->namebox->tooltiptitle = "Keep effects ";
    pii->namebox->tooltip = "Keep effects on video change ";
    pii->valuebox->tooltiptitle = "Keep effects on video change. toggle ";
    pii->valuebox->tooltip = "Leftclick to change if effects of a layer are kept on video change. ";
    mainprogram->keepeffpref = pii->onoff;
    this->items.push_back(pii);
    pos++;

    pii = new PrefItem(this, pos, "Loopstation element follow", PREF_ONOFF, (void*)&mainprogram->adaptivelprow);
    pii->onoff = 0;
    pii->namebox->tooltiptitle = "Loopstation element follow ";
    pii->namebox->tooltip = "Sets Loopstation element follow mode. ";
    pii->valuebox->tooltiptitle = "Loopstation element follow toggle ";
    pii->valuebox->tooltip = "Leftclick makes the current loopstation element follow elment row button clicks. ";
    mainprogram->adaptivelprow = pii->onoff;
    this->items.push_back(pii);
    pos++;

    pii = new PrefItem(this, pos, "Loopstation element stepping", PREF_ONOFF, (void*)&mainprogram->steplprow);
    pii->onoff = 0;
    pii->namebox->tooltiptitle = "Loopstation element stepping ";
    pii->namebox->tooltip = "Sets Loopstation element stepping mode. ";
    pii->valuebox->tooltiptitle = "Loopstation element stepping toggle ";
    pii->valuebox->tooltip = "Leftclick makes the current loopstation element step one further on record end. ";
    mainprogram->steplprow = pii->onoff;
    this->items.push_back(pii);
    pos++;
}

PIVid::PIVid() {
    // Set all preferences items that appear under the Interface tab

    this->name = "Video";
    PrefItem *pvi;
    int pos = 0;

    pvi = new PrefItem(this, pos, "Default output video width", PREF_NUMBER, (void*)&mainprogram->sow);
    pvi->value = 1920;
    pvi->namebox->tooltiptitle = "Default output video width ";
    pvi->namebox->tooltip = "Sets the width in pixels of the video stream sent to the output for new projects. ";
    pvi->valuebox->tooltiptitle = "Default output video width ";
    pvi->valuebox->tooltip = "Leftclicking the value allows setting the width in pixels of the video stream sent to the output for new projects. ";
    mainprogram->ow = pvi->value;
    this->items.push_back(pvi);
    pos++;

    pvi = new PrefItem(this, pos, "Default output video height", PREF_NUMBER, (void*)&mainprogram->soh);
    pvi->value = 1080;
    pvi->namebox->tooltiptitle = "Default output video height ";
    pvi->namebox->tooltip = "Sets the height in pixels of the video stream sent to the output for new projects. ";
    pvi->valuebox->tooltiptitle = "Default output video height ";
    pvi->valuebox->tooltip = "Leftclicking the value allows setting the height in pixels of the video stream sent to the output for new projects. ";
    mainprogram->oh = pvi->value;
    this->items.push_back(pvi);
    pos++;

    pvi = new PrefItem(this, pos, "Highly compressed video quality", PREF_NUMBER, (void*)&mainprogram->qualfr);
    pvi->value = 3;
    pvi->namebox->tooltiptitle = "Video playback quality of highly compressed streams ";
    pvi->namebox->tooltip = "Sets the quality of the video stream playback for streams that don't have one keyframe per frame encoded. A tradeoff between quality and framerate. ";
    pvi->valuebox->tooltiptitle = "Video playback quality of highly compressed streams ";
    pvi->valuebox->tooltip = "Leftclicking the value allows setting the quality in the range of 1 to 10.  Lower is better quality and worse framerate/chopdier playback. ";
    mainprogram->qualfr = pvi->value;
    this->items.push_back(pvi);
    pos++;

    pvi = new PrefItem(this, pos, "Stash HAP encoded videos", PREF_ONOFF, (void*)&mainprogram->stashvideos);
    pvi->onoff = 0;
    pvi->namebox->tooltiptitle = "Stash HAP encoded videos ";
    pvi->namebox->tooltip = "The original videos are stashed in the EWOCvj2_CPU_vid_backups directory inside your Videos folder. ";
    pvi->valuebox->tooltiptitle = "Toggle encoded video stash ";
    pvi->valuebox->tooltip = "Leftclick toggles if, when HAP encoded in the program, the original videos are stashed in the EWOCvj2_CPU_vid_backups directory inside your Videos folder. ";
    mainprogram->stashvideos = pvi->onoff;
    this->items.push_back(pvi);
    pos++;
}

PIInvisible::PIInvisible() {
    // Set all preferences items that appear under the Program tab

    this->name = "Invisible";
    PrefItem *pii;
    int pos = 0;

    pii = new PrefItem(this, pos, "Seat name", PREF_STRING, (void *) &mainprogram->seatname);
    pii->namebox->tooltiptitle = "Name of this program seat ";
    pii->namebox->tooltip = "This is the name other seats can send bin information to. ";
    pii->valuebox->tooltiptitle = "Set program seat name ";
    pii->valuebox->tooltip = "Leftclick starts keyboard entry of program seat name. ";
    pii->str = "SEAT";
    mainprogram->seatname = pii->str;
    this->items.push_back(pii);
    pos++;

    pii = new PrefItem(this, pos, "Server IP", PREF_STRING, (void*)&mainprogram->serverip);
    pii->namebox->tooltiptitle = "IP address of the server ";
    pii->namebox->tooltip = "This is the address of the seat that has server status when transmitting bin information"
                            " to other seats. ";
    pii->valuebox->tooltiptitle = "Set IP address of the server ";
    pii->valuebox->tooltip = "Leftclick starts keyboard entry of the server IP address. ";
    pii->str = "0.0.0.0";
    mainprogram->serverip = pii->str;
    this->items.push_back(pii);
    pos++;
}

PIProg::PIProg() {
    // Set all preferences items that appear under the Program tab

    this->name = "Program";
    PrefItem *pdi;
    int pos = 0;

    pdi = new PrefItem(this, pos, "Undo", PREF_ONOFF, (void*)&mainprogram->undoon);
    pdi->onoff = 1;
    pdi->namebox->tooltiptitle = "Undo system ";
    pdi->namebox->tooltip = "Turns the undo system on or off. ";
    pdi->valuebox->tooltiptitle = "Undo system toggle ";
    pdi->valuebox->tooltip = "Leftclick to turn on/off the undo system.  The undo system can cause slight delays in the video output: good idea to turn it off when performing. ";
    mainprogram->undoon = pdi->onoff;
    this->items.push_back(pdi);
    pos++;

    pdi = new PrefItem(this, pos, "Autosave", PREF_ONOFF, (void*)&mainprogram->autosave);
    pdi->onoff = 1;
    pdi->namebox->tooltiptitle = "Autosave ";
    pdi->namebox->tooltip = "Saves project states at specified intervals in specified directory. ";
    pdi->valuebox->tooltiptitle = "Autosave toggle ";
    pdi->valuebox->tooltip = "Leftclick to turn on/off autosave functionality. ";
    mainprogram->autosave = pdi->onoff;
    this->items.push_back(pdi);
    pos++;

    pdi = new PrefItem(this, pos, "Autosave interval (minutes)", PREF_NUMBER, (void*)&mainprogram->asminutes);
    pdi->value = 3;
    pdi->namebox->tooltiptitle = "Autosave interval ";
    pdi->namebox->tooltip = "Sets the time interval between successive autosaves. ";
    pdi->valuebox->tooltiptitle = "Set autosave interval ";
    pdi->valuebox->tooltip = "Leftclicking the value allows typing the autosave interval as number of minutes. ";
    mainprogram->asminutes = pdi->value;
    this->items.push_back(pdi);
    pos++;

    pdi = new PrefItem(this, pos, "Loopstation: always same 8 MIDI controls", PREF_ONOFF, (void*)&mainprogram->sameeight);
    pdi->onoff = false;
    pdi->namebox->tooltiptitle = "Loopstation: always same 8 MIDI controls ";
    pdi->namebox->tooltip = "If the program always uses the same 8 MIDI controls even if the list has been scrolled. ";
    pdi->valuebox->tooltiptitle = "Loopstation: always same 8 MIDI controls: toggle ";
    pdi->valuebox->tooltip = "Leftclick to turn on/off if the program always uses the same 8 MIDI controls even if the list has been scrolled. ";
    mainprogram->sameeight = pdi->onoff;
    this->items.push_back(pdi);
    pos++;
}


void Program::define_menus() {
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
    effects.push_back("CHROMASTRETCH");
    std::vector<std::string> meffects = effects;
    std::sort(meffects.begin(), meffects.end());
    for (int i = 0; i < meffects.size(); i++) {
        for (int j = 0; j < effects.size(); j++) {
            if (meffects[i] == effects[j]) {
                mainprogram->abeffects.push_back((EFFECT_TYPE) j);
                break;
            }
        }
    }
    mainprogram->make_menu("effectmenu", mainprogram->effectmenu, meffects);
    for (int i = 0; i < effects.size(); i++) {
        mainprogram->effectsmap[(EFFECT_TYPE) i] = effects[i];
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
    mixmodes.push_back("CHROMAKEY");
    mixmodes.push_back("LUMAKEY");
    mixmodes.push_back("DISPLACEMENT");
    mainprogram->make_menu("mixmodemenu", mainprogram->mixmodemenu, mixmodes);

    std::vector<std::string> parammodes1;
    parammodes1.push_back("MIDI Learn");
    parammodes1.push_back("Reset to default");
    mainprogram->make_menu("parammenu1", mainprogram->parammenu1, parammodes1);

    std::vector<std::string> parammodes2;
    parammodes2.push_back("MIDI Learn");
    parammodes2.push_back("Remove automation");
    parammodes2.push_back("Reset to default");
    mainprogram->make_menu("parammenu2", mainprogram->parammenu2, parammodes2);

    std::vector<std::string> parammodes1b;
    parammodes1b.push_back("MIDI Learn");
    parammodes1b.push_back("Reset to default");
    parammodes1b.push_back("Toggle lock");
    mainprogram->make_menu("parammenu1b", mainprogram->parammenu1b, parammodes1b);

    std::vector<std::string> parammodes2b;
    parammodes2b.push_back("MIDI Learn");
    parammodes2b.push_back("Remove automation");
    parammodes2b.push_back("Reset to default");
    parammodes2b.push_back("Toggle lock");
    mainprogram->make_menu("parammenu2b", mainprogram->parammenu2b, parammodes2b);

    std::vector<std::string> parammodes3;
    parammodes3.push_back("MIDI Learn");
    mainprogram->make_menu("parammenu3", mainprogram->parammenu3, parammodes3);

    std::vector<std::string> parammodes4;
    parammodes4.push_back("MIDI Learn");
    parammodes4.push_back("Remove automation");
    mainprogram->make_menu("parammenu4", mainprogram->parammenu4, parammodes4);

    std::vector<std::string> parammodes5;
    parammodes5.push_back("MIDI Learn");
    parammodes5.push_back("Toggle lock");
    mainprogram->make_menu("parammenu5", mainprogram->parammenu5, parammodes5);

    std::vector<std::string> parammodes6;
    parammodes6.push_back("MIDI Learn");
    parammodes6.push_back("Remove automation");
    parammodes6.push_back("Toggle lock");
    mainprogram->make_menu("parammenu6", mainprogram->parammenu6, parammodes6);

    std::vector<std::string> mixtargets;
    mainprogram->make_menu("monitormenu", mainprogram->monitormenu, mixtargets);

    std::vector<std::string> bintargets;
    mainprogram->make_menu("bintargetmenu", mainprogram->bintargetmenu, bintargets);

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
    loopops.push_back("MIDI Learn move loop area");
    mainprogram->make_menu("loopmenu", mainprogram->loopmenu, loopops);
    mainprogram->loopmenu->width = 0.2f;

    std::vector<std::string> aspectops;
    aspectops.push_back("Same as output");
    aspectops.push_back("Original inside");
    aspectops.push_back("Original outside");
    mainprogram->make_menu("aspectmenu", mainprogram->aspectmenu, aspectops);

    std::vector<std::string> layops1;
    layops1.push_back("submenu livemenu");
    layops1.push_back("Connect live");
    layops1.push_back("Open file(s) into layer stack");
    layops1.push_back("Open file(s) into queue");
    layops1.push_back("Insert file(s) before");
    layops1.push_back("Save layer");
    layops1.push_back("New deck");
    layops1.push_back("Open deck");
    layops1.push_back("Save deck");
    layops1.push_back("New mix");
    layops1.push_back("Open mix");
    layops1.push_back("Save mix");
    layops1.push_back("Delete layer");
    layops1.push_back("Duplicate layer");
    layops1.push_back("Clone layer");
    layops1.push_back("Center image");
    layops1.push_back("submenu aspectmenu");
    layops1.push_back("Aspect ratio");
    layops1.push_back("submenu mixtargetmenu");
    layops1.push_back("Show on display");
    layops1.push_back("Record and replace");
    layops1.push_back("HAP encode on-the-fly");
    mainprogram->make_menu("laymenu1", mainprogram->laymenu1, layops1);

    std::vector<std::string> layops2;
    layops2.push_back("submenu livemenu");
    layops2.push_back("Connect live");
    layops2.push_back("Open file(s) into layer stack");
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
    layops2.push_back("submenu mixtargetmenu");
    layops2.push_back("Show on display");
    layops2.push_back("Record and replace");
    mainprogram->make_menu("laymenu2", mainprogram->laymenu2, layops2);

    std::vector<std::string> loadops;
    loadops.push_back("submenu livemenu");
    loadops.push_back("Connect live");
    loadops.push_back("Open file(s) in stack");
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
    wipes.push_back("DOT");
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
    generic.push_back("Recover autosave");
    generic.push_back("Clean slate");
    generic.push_back("Preferences");
    generic.push_back("Configure general MIDI");
    generic.push_back("Quit");
    mainprogram->make_menu("mainmenu", mainprogram->mainmenu, generic);

    std::vector<std::string> shelf1;
    shelf1.push_back("Open file(s)");
    shelf1.push_back("New shelf");
    shelf1.push_back("Open shelf");
    shelf1.push_back("Save shelf");
    shelf1.push_back("Insert deck A");
    shelf1.push_back("Insert deck B");
    shelf1.push_back("Insert full mix");
    shelf1.push_back("Insert in bin");
    shelf1.push_back("Erase element");
    mainprogram->make_menu("shelfmenu", mainprogram->shelfmenu, shelf1);

    std::vector<std::string> file;
    file.push_back("submenu filenewmenu");
    file.push_back("New");
    file.push_back("submenu fileopenmenu");
    file.push_back("Open");
    file.push_back("submenu filesavemenu");
    file.push_back("Save as");
    file.push_back("Save project");
    file.push_back("Quit");
    mainprogram->make_menu("filemenu", mainprogram->filemenu, file);

    std::vector<std::string> laylist1;
    mainprogram->make_menu("laylistmenu1", mainprogram->laylistmenu1, laylist1);

    std::vector<std::string> laylist2;
    mainprogram->make_menu("laylistmenu2", mainprogram->laylistmenu2, laylist2);

    std::vector<std::string> filenew;
    filenew.push_back("Project");
    filenew.push_back("Mix");
    filenew.push_back("Deck A");
    filenew.push_back("Deck B");
    filenew.push_back("submenu laylistmenu1");
    filenew.push_back("Layer in deck A before");
    filenew.push_back("submenu laylistmenu2");
    filenew.push_back("Layer in deck B before");
    mainprogram->make_menu("filenewmenu", mainprogram->filenewmenu, filenew);

    std::vector<std::string> fileopen;
    fileopen.push_back("Project");
    fileopen.push_back("Mix");
    fileopen.push_back("Deck A");
    fileopen.push_back("Deck B");
    fileopen.push_back("submenu laylistmenu1");
    fileopen.push_back("Files into layers in deck A");
    fileopen.push_back("submenu laylistmenu2");
    fileopen.push_back("Files into layers in deck B");
    fileopen.push_back("submenu laylistmenu1");
    fileopen.push_back("Files into queue in deck A");
    fileopen.push_back("submenu laylistmenu2");
    fileopen.push_back("Files into queue in deck B");
    mainprogram->make_menu("fileopenmenu", mainprogram->fileopenmenu, fileopen);

    std::vector<std::string> filesave;
    filesave.push_back("Project");
    filesave.push_back("Mix");
    filesave.push_back("Deck A");
    filesave.push_back("Deck B");
    filesave.push_back("submenu laylistmenu3");
    filesave.push_back("Layer in deck A");
    filesave.push_back("submenu laylistmenu4");
    filesave.push_back("Layer in deck B");
    mainprogram->make_menu("filesavemenu", mainprogram->filesavemenu, filesave);

    std::vector<std::string> edit;
    edit.push_back("Preferences");
    edit.push_back("Configure general MIDI");
    mainprogram->make_menu("editmenu", mainprogram->editmenu, edit);

    std::vector<std::string> lpst;
    lpst.push_back("Clear loopstation line");
    lpst.push_back("Copy loop duration");
    lpst.push_back("Paste loop duration by changing speed");
    lpst.push_back("Paste loop duration by changing loop length");
    mainprogram->make_menu("lpstmenu", mainprogram->lpstmenu, lpst);

    //make menu item names bitmaps
    for (int i = 0; i < mainprogram->menulist.size(); i++) {
        for (int j = 0; j < mainprogram->menulist[i]->entries.size(); j++) {
            render_text(mainprogram->menulist[i]->entries[j], nullptr, white, 2.0f, 2.0f, 0.00045f, 0.00075f, 0, 0, 0);
        }
    }
}


void Program::write_recentprojectlist() {
    std::ofstream wfile;
#ifdef WINDOWS
    std::string dir2 = mainprogram->docpath;
#else
#ifdef POSIX
    std::string homedir(getenv("HOME"));
    std::string dir2 = homedir + "/.ewocvj2/";
#endif
#endif
    wfile.open(dir2 + "recentprojectslist");
    if (!wfile.fail()) {
        wfile << "EWOCvj RECENTPROJECTS V0.1\n";
        while (mainprogram->recentprojectpaths.size() > 10) {
            mainprogram->recentprojectpaths.pop_back();
        }
        for (int i = 0; i < mainprogram->recentprojectpaths.size(); i++) {
            wfile << mainprogram->recentprojectpaths[i];
            wfile << "\n";
        }
        wfile << "ENDOFFILE\n";
        wfile.close();
    }
}


void Program::socket_server(struct sockaddr_in serv_addr, int opt) {
    this->serverip = this->localip;
    int new_socket;

    if (inet_pton(AF_INET, this->serverip.c_str(), &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid server address/ Address not supported \n");
    }

    int addrlen = sizeof(serv_addr);

    int ret = setsockopt(this->sock, SOL_SOCKET, SO_REUSEADDR,
                         (const char *) &opt, sizeof(opt));

    // Forcefully attaching socket to the port
    if (bind(this->sock, (struct sockaddr *) &serv_addr,
               addrlen) < 0)
    {
        //print the error message
        perror("bind failed. Error");
    }

    while (1) {
        ret = listen(this->sock, 3);
        if (!this->server) return;
        if (ret < 0) {
            printf("listen errno %d\n", errno);
            continue;
        }
        new_socket = accept(this->sock, (struct sockaddr *) &serv_addr,
                            (socklen_t *) &addrlen);
#ifdef POSIX
        int flags = fcntl(new_socket, F_GETFL);
        fcntl(new_socket, F_SETFL, flags | O_NONBLOCK);
#endif
#ifdef WINDOWS
        u_long flags = 1;
        ioctlsocket(this->sock, FIONBIO, &flags);
#endif

        // check self-connection
        sockaddr_in addr_client;
        socklen_t len;
        getsockname(new_socket, (struct sockaddr *)&addr_client, &len);
        if(addr_client.sin_port == serv_addr.sin_port &&
           addr_client.sin_addr.s_addr == serv_addr.sin_addr.s_addr) {
            //self connection
            continue;
        }

        mainprogram->connsockets.push_back(new_socket);

        // send number of clients for choosing unique osc address
        send(new_socket, std::to_string(mainprogram->connsockets.size()).c_str(),
             strlen(std::to_string(mainprogram->connsockets.size()).c_str()), 0);
        //get new socket name
        char *name = (char*)malloc(1024);
        char buf[1037];
        name = bl_recv(new_socket, name, 1024, 0);
        mainprogram->connsocknames.push_back(name);
        mainprogram->connmap[name] = new_socket;
        //send server name to client
        send(new_socket, mainprogram->seatname.c_str(), mainprogram->seatname.size(), 0);
        //send new socket name to all other client sockets
        char *walk2 = buf;
        auto put_in_buffer = [](const char* str, char* walk) {
            for (int i = 0; i < strlen(str); i++) {
                *walk++ = str[i];
            }
            char nll[2] = "\0";
            *walk++ = *nll;
            return walk;
        };
        walk2 = put_in_buffer("NEW_SIBLING", walk2);
        walk2 = put_in_buffer(name, walk2);
        for (auto sock : mainprogram->connsockets) {
            if (sock != new_socket) send(sock, buf, 1037, 0);
        }

        free(name);
        // start thread for recieving BIN_SENT messages from every client
        std::thread sockservrecv(&Program::socket_server_recieve, mainprogram, new_socket);
        sockservrecv.detach();
    }
}

void Program::socket_client(struct sockaddr_in serv_addr, int opt) {
    char *buf = (char*)malloc(1024);
    while (1) {
        std::unique_lock<std::mutex> lock(this->clientmutex);
        this->startclient.wait(lock, [&] {return !this->server; });
        lock.unlock();

        if (inet_pton(AF_INET, this->serverip.c_str(), &serv_addr.sin_addr) <= 0) {
            printf("\nInvalid server address/ Address not supported \n");
        }

#ifdef POSIX
        int flags = fcntl(this->sock, F_GETFL);
        fcntl(this->sock, F_SETFL, flags | O_NONBLOCK);
#endif
#ifdef WINDOWS
        u_long flags = 1;
        ioctlsocket(this->sock, FIONBIO, &flags);
#endif
        int ret = connect(this->sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        if (ret < 0) {
#ifdef POSIX
            sleep(1);
#endif
#ifdef WINDOWS
        Sleep(1000);
#endif
            ret = connect(this->sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        }

        if (ret >= 0) {
            // server found and connected succesfully!
            buf = bl_recv( this->sock , buf, 1024, 0);
            if (buf == "LOOP") return;
            send(this->sock, this->seatname.c_str(), this->seatname.size(), 0);
            // receive server name
            buf = bl_recv(this->sock , buf, 1024, 0);
            this->connsocknames.push_back(buf);
            this->connected = 1;
            this->prefs->save();
            break;
        }
        else {
            this->connfailed = true;
            this->connfailedmilli = 0;
            return;
        }
    }
    free(buf);

    char *buf2 = (char *) calloc(148489, 1);
    // wait for messages
    while (1) {
        buf2 = bl_recv(this->sock, buf2, 148489, 0);
        std::string str(buf2);
        if (buf2 != "") {
            if (str == "BIN_SENT") {
                buf2 += strlen(buf2) + 1;
                binsmain->messagelengths.push_back(std::stoi(buf2));
                buf2 += strlen(buf2) + 1;
                binsmain->messagesocknames.push_back(buf2);
                buf2 += strlen(buf2) + 1;
                binsmain->messages.push_back(buf2);
            } else if (str == "NEW_SIBLING") {
                buf2 += 12;
                mainprogram->connsocknames.push_back(buf2);
            } else if (str == "SERVER_QUITS") {
                // disconnect
                this->connected = 0;
            } else if (str == "CHANGE_NAME") {
                buf += strlen(buf) + 1;
                int pos =
                        std::find(this->connsockets.begin(), this->connsockets.end(), sock) - this->connsockets.begin();
                this->connsocknames[pos] = buf;
            }

            // Convert IPv4 and IPv6 addresses from text to binary form
            if (inet_pton(AF_INET, buf2, &serv_addr.sin_addr) <= 0) {
                printf("\nInvalid address/ Address not supported \n");
                return;
            }
            connect(mainprogram->sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
        }
    }
}

void Program::socket_server_recieve(SOCKET sock) {
    // wait for messages
    char *buf = (char *) malloc(148489);
    while (1) {
        buf = bl_recv(sock, buf, 148489, 0);
        if (!this->server) return;
        std::string str(buf);
        if (str == "BIN_SENT") {
            binsmain->rawmessages.push_back(buf);
            buf += strlen(buf) + 1;
            binsmain->messagelengths.push_back(std::stoi(buf));
            buf += strlen(buf) + 1;
            binsmain->messagesocknames.push_back(buf);
            buf += strlen(buf) + 1;
            binsmain->messages.push_back(buf);
        }
        else if (str == "CHANGE_NAME") {
            buf += strlen(buf) + 1;
            int pos = std::find(this->connsockets.begin(), this->connsockets.end(), sock) - this->connsockets.begin();
            this->connsocknames[pos] = buf;
        }
    }
    free(buf);
}

char* Program::bl_recv(int sock, char *buf, size_t sz, int flags) {
#ifdef POSIX
    int flags2 = fcntl(sock, F_GETFL);
    fcntl(sock, F_SETFL, flags2 & ~O_NONBLOCK);
    recv(sock, buf, sz, flags);
    fcntl(sock, F_SETFL, flags2);
#endif
#ifdef WINDOWS
    u_long flags2 = 0;
    ioctlsocket(sock, FIONBIO, &flags2);
    recv(sock, buf, sz, flags);
    flags2 = 1;
    ioctlsocket(sock, FIONBIO, &flags2);
#endif
    return buf;
}





//*************

// SHELVES

//*************


void Shelf::save(const std::string path, bool undo) {
    std::vector<std::string> filestoadd;
    std::ofstream wfile;
    wfile.open(path);
    wfile << "EWOCvj SHELFFILE V0.1\n";

    wfile << "ELEMS\n";
    for (int j = 0; j < 16; j++) {
        ShelfElement* elem = this->elements[j];
        wfile << "PATH\n";
        wfile << elem->path;
        wfile << "\n";
        wfile << "RELPATH\n";
        if (elem->path != "") {
            wfile << std::filesystem::relative(elem->path, mainprogram->contentpath).generic_string();
        }
        else {
            wfile << elem->path;
        }
        wfile << "\n";
        if (elem->path == "") continue;
        wfile << "TYPE\n";
        wfile << std::to_string(elem->type);
        wfile << "\n";
        wfile << "JPEGPATH\n";
        wfile << elem->jpegpath;
        wfile << "\n";
        if (!undo) {
            if (elem->type == ELEM_LAYER || elem->type == ELEM_DECK || elem->type == ELEM_MIX) {
                filestoadd.push_back(elem->path);
            }
            filestoadd.push_back(elem->jpegpath);
        }
        if (elem->path != "") {
            wfile << "FILESIZE\n";
            wfile << std::to_string(std::filesystem::file_size(elem->path));
            wfile << "\n";
        }
        wfile << "LAUNCHTYPE\n";
        wfile << std::to_string(elem->launchtype);
        wfile << "\n";
    }
    wfile << "ENDOFELEMS\n";
    wfile << "ENDOFFILE\n";
    wfile.close();

    if (!undo) {
        std::vector<std::vector<std::string>> filestoadd2;
        filestoadd2.push_back(filestoadd);
        std::string tpath = find_unused_filename("tempconcatshelf", mainprogram->temppath, "");
        std::thread concat = std::thread(&Program::concat_files, mainprogram, tpath,
                                         path, filestoadd2);
        concat.detach();
    }
}


void Shelf::open_files_shelf() {
    auto next_elem = []()
    {
        mainprogram->shelffileselem++;
        mainprogram->shelffilescount++;
        if (mainprogram->shelffilescount == mainprogram->paths.size() || mainprogram->shelffileselem == 16) {
            mainprogram->openfilesshelf = false;
            mainprogram->paths.clear();
            mainprogram->multistage = 0;
        }
    };

    // order elements
    bool cont = mainprogram->order_paths(true);
    if (!cont) return;

    std::string str;
    if (mainprogram->paths.size()) {
        // set element values
        ShelfElement *elem = this->elements[mainprogram->shelffileselem];
        str = mainprogram->paths[mainprogram->shelffilescount];
        // determine file type
        std::string istring = "";
        std::ifstream rfile;
        rfile.open(str);
        safegetline(rfile, istring);
        rfile.close();
        if (istring.find("EWOCvj LAYERFILE") != std::string::npos) {
            elem->type = ELEM_LAYER;
        } else if (istring.find("EWOCvj DECKFILE") != std::string::npos) {
            //elem->jpegpath = "";
            elem->type = ELEM_DECK;
        } else if (istring.find("EWOCvj MIXFILE") != std::string::npos) {
            //elem->jpegpath = "";
            elem->type = ELEM_MIX;
        } else if (isimage(str)) {
            elem->type = ELEM_IMAGE;
        } else if (isvideo(str)) {
            elem->type = ELEM_FILE;
        } else {
            next_elem();
            return;
        }

        elem->path = str;
        elem->jpegpath = find_unused_filename("shelftex", mainprogram->temppath, ".jpg");
        // texes are inserted after phase 2 of order_paths
        save_thumb(elem->jpegpath, mainprogram->pathtexes[mainprogram->shelffilescount]);
        glDeleteTextures(1, &elem->tex);
        elem->tex = mainprogram->pathtexes[mainprogram->shelffilescount];
    }

    // next element, clean up when at end
    next_elem();
    mainprogram->currfilesdir = dirname(str);
}

void Shelf::open_jpegpaths_shelf() {
    if (mainprogram->shelfjpegpaths[this]) {
        if (this->elements[this->elemcount]->path != "") {
            open_thumb(this->elements[this->elemcount]->jpegpath, this->elements[this->elemcount]->tex);
        }
        this->elemcount++;
        if (this->elemcount == 16) {
            mainprogram->openjpegpathsshelf = false;
        }
    }
}


bool Shelf::insert_deck(std::string path, bool deck, int pos) {
    ShelfElement* elem = this->elements[pos];
    elem->path = path;
    elem->type = ELEM_DECK;
    GLuint butex = -1;
    if (mainprogram->prevmodus) {
        butex = elem->tex;
        elem->tex = copy_tex(mainprogram->nodesmain->mixnodes[0][deck]->mixtex);
    }
    else {
        butex = elem->tex;
        elem->tex = copy_tex(mainprogram->nodesmain->mixnodes[1][deck]->mixtex);
    }
    if (butex != -1) glDeleteTextures(1, &butex);
    std::string jpegpath = path + ".jpeg";
    save_thumb(jpegpath, elem->tex);
    elem->jpegpath = jpegpath;
    return 1;
}

bool Shelf::insert_mix(const std::string path, int pos) {
    ShelfElement* elem = this->elements[pos];
    elem->path = path;
    elem->type = ELEM_MIX;
    GLuint butex = -1;
    if (mainprogram->prevmodus) {
        butex = elem->tex;
        elem->tex = copy_tex(mainprogram->nodesmain->mixnodes[0][2]->mixtex);
    }
    else {
        butex = elem->tex;
        elem->tex = copy_tex(mainprogram->nodesmain->mixnodes[1][2]->mixtex);
    }
    if (butex != -1) glDeleteTextures(1, &butex);
    std::string jpegpath = path + ".jpeg";
    save_thumb(jpegpath, elem->tex);
    elem->jpegpath = jpegpath;
    return 1;
}

bool Shelf::open(const std::string path, bool undo) {
    std::string result;
    std::ifstream rfile;
    bool concat = false;
    if (!undo) {
        result = mainprogram->deconcat_files(path);
        concat = (result != "");
        if (concat) rfile.open(result);
        else rfile.open(path);
    }
    else {
        rfile.open(path);
    }

    this->erase();
    int filecount = 0;
    std::string istring;
    safegetline(rfile, istring);
    while (safegetline(rfile, istring)) {
        if (istring == "ENDOFFILE") {
            break;
        }
        else if (istring == "ELEMS") {
            int count = 0;
            ShelfElement *elem = nullptr;
            while (safegetline(rfile, istring)) {
                if (istring == "ENDOFELEMS") break;
                if (istring == "PATH") {
                    safegetline(rfile, istring);
                    elem = this->elements[count];
                    elem->path = istring;
                    count++;
                    //if (!exists(elem->path)) {
                       // elem->path = "";
                    //}
                }
                if (istring == "RELPATH") {
                    safegetline(rfile, istring);
                    if (elem->path == "" && istring != "") {
                        std::filesystem::current_path(mainprogram->contentpath);
                        elem->path = pathtoplatform(std::filesystem::absolute(istring).generic_string());
                    }
                    if (elem->path == "") {
                        continue;
                    }
                }
                if (istring == "TYPE") {
                    safegetline(rfile, istring);
                    elem->type = (ELEM_TYPE) std::stoi(istring);
                    std::string suf = "";
                    if (elem->type == ELEM_LAYER) suf = ".layer";
                    if (elem->type == ELEM_DECK) suf = ".deck";
                    if (elem->type == ELEM_MIX) {
                        suf = ".mix";
                    }
                    if (elem->path != "" && !exists(elem->path) && (elem->type == ELEM_FILE || elem->type == ELEM_IMAGE)) {
                        mainmix->retargeting = true;
                        mainmix->newshelfpaths.push_back(elem->path);
                        mainmix->newpathshelfelems.push_back(elem);
                        elem->path = "";
                    }
                    else if (suf != "") {
                        if (concat) {
                            elem->path = find_unused_filename("shelffile", mainprogram->temppath, suf);
                            std::filesystem::rename(result + "_" + std::to_string(filecount) + ".file", elem->path);
                            filecount++;
                        }
                    }
                    safegetline(rfile, istring);
                    elem->jpegpath = result + "_" + std::to_string(filecount) + ".file";
                    if (!undo) {
                        glDeleteTextures(1, &elem->tex);
                        glGenTextures(1, &elem->tex);
                        glBindTexture(GL_TEXTURE_2D, elem->tex);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
                        open_thumb(result + "_" + std::to_string(filecount) + ".file", elem->tex);
                    }
                    filecount++;
                }
                if (undo) {
                    if (istring == "JPEGPATH") {
                        safegetline(rfile, istring);
                        elem->jpegpath = istring;
                        glDeleteTextures(1, &elem->tex);
                        glGenTextures(1, &elem->tex);
                        glBindTexture(GL_TEXTURE_2D, elem->tex);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
                        mainprogram->shelfjpegpaths[this] = true;
                        mainprogram->openjpegpathsshelf = true;
                        this->elemcount = 0;
                    }
                }
                if (istring == "FILESIZE") {
                    safegetline(rfile, istring);
                    elem->filesize = std::stoll(istring);
                }
                if (istring == "LAUNCHTYPE") {
                    safegetline(rfile, istring);
                    elem->launchtype = std::stoi(istring);
                }
            }
        }
    }

    rfile.close();

    return 1;
}

void Shelf::erase() {
    for (ShelfElement* elem : this->elements) {
        elem->path = "";
        elem->type = ELEM_FILE;
        blacken(elem->tex);
        blacken(elem->oldtex);
    }
}

void Shelf::handle() {
    // draw shelves and handle shelves
    int inelem = -1;
    for (int i = 0; i < 16; i++) {
        ShelfElement* elem = this->elements[i];
        // border coloring according to element type
        if (elem->type == ELEM_LAYER) {
            draw_box(orange, orange, this->buttons[i]->box, -1);
            draw_box(nullptr, orange, this->buttons[i]->box->vtxcoords->x1 + 0.0075f, this->buttons[i]->box->vtxcoords->y1, this->buttons[i]->box->vtxcoords->w - 0.0075f, this->buttons[i]->box->vtxcoords->h - 0.0075f, elem->tex);
        }
        else if (elem->type == ELEM_DECK) {
            draw_box(purple, purple, this->buttons[i]->box, -1);
            draw_box(nullptr, purple, this->buttons[i]->box->vtxcoords->x1 + 0.0075f, this->buttons[i]->box->vtxcoords->y1, this->buttons[i]->box->vtxcoords->w - 0.0075f, this->buttons[i]->box->vtxcoords->h - 0.0075f, elem->tex);
        }
        else if (elem->type == ELEM_MIX) {
            draw_box(green, green, this->buttons[i]->box, -1);
            draw_box(nullptr, green, this->buttons[i]->box->vtxcoords->x1 + 0.0075f, this->buttons[i]->box->vtxcoords->y1, this->buttons[i]->box->vtxcoords->w - 0.0075f, this->buttons[i]->box->vtxcoords->h - 0.0075f, elem->tex);
        }
        else if (elem->type == ELEM_IMAGE) {
            draw_box(white, white, this->buttons[i]->box, -1);
            draw_box(nullptr, white, this->buttons[i]->box->vtxcoords->x1 + 0.0075f, this->buttons[i]->box->vtxcoords->y1, this->buttons[i]->box->vtxcoords->w - 0.0075f, this->buttons[i]->box->vtxcoords->h - 0.0075f, elem->tex);
        }
        else {
            draw_box(grey, grey, this->buttons[i]->box, -1);
            draw_box(nullptr, grey, this->buttons[i]->box->vtxcoords->x1 + 0.0075f, this->buttons[i]->box->vtxcoords->y1, this->buttons[i]->box->vtxcoords->w - 0.0075f, this->buttons[i]->box->vtxcoords->h - 0.0075f, elem->tex);
        }

        // draw small icons for choice of launch play type of this video
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

        // calculate background frame numbers and loopstation posns for elements with launchtype 2
        for (int j = 0; j < elem->nblayers.size(); j++) {
            elem->nblayers[j]->vidopen = false;
            elem->nblayers[j]->progress(!mainprogram->prevmodus, 0);
            elem->nblayers[j]->cnt_lpst();
        }

        if (cond) {
            // mouse over this element
            if (mainprogram->dropfiles.size()) {
                // SDL drag'n'drop
                mainprogram->path = mainprogram->dropfiles[0];
                for (char *df : mainprogram->dropfiles) {
                    mainprogram->paths.push_back(df);
                }
                mainprogram->pathto = "OPENFILESSHELF";
                mainmix->mouseshelf = this;
                mainmix->mouseshelfelem = i;
            }
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
                mainprogram->dragbinel->relpath = std::filesystem::relative(elem->path, mainprogram->project->binsdir).generic_string();
                mainprogram->dragbinel->type = elem->type;
                mainprogram->dragbinel->tex = elem->tex;
                //if (elem->type == ELEM_DECK || elem->type == ELEM_MIX) {
                mainprogram->shelf_triggering(elem);
                mainprogram->lpstelem = elem;
                //}
                //else {
                //    for (int i = 0; i < mainmix->currlays[!mainprogram->prevmodus].size(); i++) {
                //        mainmix->open_dragbinel(mainmix->currlays[!mainprogram->prevmodus][i], i);
                //    }
                //}
                enddrag();
            }
            else if (mainprogram->leftmouse) {
                mainprogram->recundo = false;
            }
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
            if (mainprogram->dragbinel && i != this->prevnum) {
                std::swap(elem->tex, elem->oldtex);
                if (this->prevnum != -1) std::swap(this->elements[this->prevnum]->tex, this->elements[this->prevnum]->oldtex);
                if (mainprogram->shelfdragelem) {
                    std::swap(mainprogram->shelfdragelem->tex, mainprogram->shelfdragelem->oldtex);
                    mainprogram->shelfdragelem->tex = elem->oldtex;
                }
                elem->tex = mainprogram->dragbinel->tex;
                this->prevnum = i;
            }
            if (mainprogram->leftmousedown) {
                if (!elem->sbox->in() && !elem->pbox->in() && !elem->cbox->in()) {
                    if (!mainprogram->dragbinel) {
                        // user starts dragging this element
                        if (elem->path != "") {
                            mainprogram->shelfmx = mainprogram->mx;
                            mainprogram->shelfmy = mainprogram->my;
                            mainprogram->shelfdragelem = elem;
                            mainprogram->shelfdragnum = i;
                            mainprogram->leftmousedown = false;
                            mainprogram->dragbinel = new BinElement;
                            mainprogram->dragbinel->path = elem->path;
                            mainprogram->dragbinel->relpath = std::filesystem::relative(elem->path, mainprogram->project->binsdir).generic_string();
                            mainprogram->dragbinel->type = elem->type;
                            mainprogram->dragbinel->tex = elem->tex;
                        }
                    }
                }
            }
            else if (mainprogram->lmover) {
                // user drops file/layer/deck/mix in this element
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
                                                               mainprogram->project->shelfdir + this->basepath + "/",
                                                               extstr);
                                std::filesystem::copy_file(mainprogram->dragbinel->path, newpath);
                                mainprogram->dragbinel->path = newpath;
                                mainprogram->dragbinel->relpath = std::filesystem::relative(newpath, mainprogram->project->binsdir).generic_string();
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
                            this->prevnum = -1;
                            mainprogram->shelfdragelem = nullptr;
                            mainprogram->rightmouse = true;
                            binsmain->handle(0);
                            enddrag();
                            mainprogram->rightmouse = false;

                            return;
                        }
                        else enddrag();
                    }
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
    if (inelem > -1 && mainprogram->dragbinel) {
        mainprogram->dragout[this->side] = false;
        mainprogram->dragout[!this->side] = true;
    }
    if (inelem == -1 && !mainprogram->dragout[this->side] && mainprogram->dragbinel) {
        // mouse not over this element
        mainprogram->dragout[this->side] = true;
        if (this->prevnum != -1) {
            std::swap(this->elements[this->prevnum]->tex, this->elements[this->prevnum]->oldtex);
            if (mainprogram->shelfdragelem) {
                if (mainprogram->shelfdragnum == this->prevnum) {
                    std::swap(mainprogram->shelfdragelem->tex, mainprogram->shelfdragelem->oldtex);
                }
                else {
                    mainprogram->shelfdragelem->tex = mainprogram->dragbinel->tex;
                }
            }
        }
        this->prevnum = -1;
    }
    else if (!mainprogram->dragout[this->side]) this->prevnum = this->newnum;
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
    GLuint rettex = mainprogram->grab_from_texpool(tw, th, GL_RGBA8);
    if (rettex != -1) {
        smalltex = rettex;
    }
    else {
        glGenTextures(1, &smalltex);
        glBindTexture(GL_TEXTURE_2D, smalltex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, tw, th);
        mainprogram->texintfmap[smalltex] = GL_RGBA8;
    }
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
    int wi = 192;
    int he = 108;
    unsigned char *buf = new unsigned char[wi * he * 3];

    GLuint tex2 = copy_tex(tex, 192, 108);
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
    delete buf;
    outfile.close();
}

void open_thumb(std::string path, GLuint tex) {
    const int COLOR_COMPONENTS = 3;

    std::ifstream infile(path, std::ios::binary | std::ios::ate);
    std::streamsize sz = infile.tellg();
    unsigned char *buf = new unsigned char[sz];
    infile.seekg(0, std::ios::beg);
    infile.read((char*)buf, sz);

    long unsigned int _jpegSize = sz; //!< _jpegSize from above

    int jpegSubsamp, width, height, dummy;

    tjhandle _jpegDecompressor = tjInitDecompress();

    tjDecompressHeader3(_jpegDecompressor, buf, _jpegSize, &width, &height, &jpegSubsamp, &dummy);

    unsigned char *uncbuffer = new unsigned char[width*height*COLOR_COMPONENTS]; //!< will contain the decompressed image

    tjDecompress2(_jpegDecompressor, buf, _jpegSize, uncbuffer, width, 0/*pitch*/, height, TJPF_RGB, TJFLAG_FASTDCT);

    tjDestroy(_jpegDecompressor);

    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, uncbuffer);

    delete buf;
    delete uncbuffer;
    infile.close();

}


std::ifstream::pos_type getFileSize(std::string filename)
{
    std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);

    return ((int)in.tellg());
}

bool check_version(std::string path) {
    bool concat = true;
    std::fstream bfile;
    bfile.open(path, std::ios::in | std::ios::binary);
    char *buffer = new char[7];
    bfile.read(buffer, 7);
    if (buffer[0] == 0x45 && buffer[1] == 0x57 && buffer[2] == 0x4F && buffer[3] == 0x43 && buffer[4] == 0x76 && buffer[5] == 0x6A && buffer[6] == 0x20) {
        concat = false;
    }
    delete[] buffer;
    bfile.close();
    return concat;
}


std::string Program::deconcat_files(std::string path) {
    bool concat = check_version(path);
    std::string outpath;
    std::fstream bfile;
    bfile.open(path, std::ios::in | std::ios::binary);
    if (concat) {
        int32_t num;
        bfile.read((char *)&num, 4);
        if (num != 20011975) {
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
        std::ofstream ofile;
        outpath = find_unused_filename(basename(path), mainprogram->temppath, ".mainfile");
        ofile.open(outpath, std::ios::out | std::ios::binary);

        char *ibuffer = new char[sizes[0]];
        bfile.read(ibuffer, sizes[0]);
        ofile.write(ibuffer, sizes[0]);
        ofile.close();
        delete[] ibuffer;
        for (int i = 0; i < num - 1; i++) {
            std::ofstream ofile;
            ofile.open(outpath + "_" + std::to_string(i) + ".file", std::ios::out | std::ios::binary);
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

void Program::concat_files(std::string ofpath, std::string path, std::vector<std::vector<std::string>> filepaths) {

    int count = mainprogram->concatting;
    mainprogram->concatting++;

    if (mainprogram->concatting == mainprogram->concatlimit) {
        this->goconcat = true;
        this->concatvar.notify_all();
        mainprogram->concatlimit = 0;
    }
    if (mainprogram->concatlimit) {
        std::unique_lock<std::mutex> lock(this->concatlock);
        this->concatvar.wait(lock, [&] {return this->goconcat; });
        lock.unlock();
    }

    //if (mainprogram->concatting == mainprogram->concatlimit) {
        while (count > mainprogram->numconcatted) {
            Sleep(10);
        }
    //}

    //Sleep(100);

    std::ofstream ofile;
    ofile.open(ofpath, std::ios::out | std::ios::binary);

    std::vector<std::string> paths;
    paths.push_back(path);
    for (int i = 0; i < filepaths.size(); i++) {
        for (int j = 0; j < filepaths[i].size(); j++) {
            paths.push_back(filepaths[i][j]);
        }
    }

    /*if (paths.size() == 1) {
        mainprogram->concatting--;
        mainprogram->numconcatted++;
        if (mainprogram->concatting == 0) {
            mainprogram->numconcatted = 0;
        }
        return;
    }*/

    int recogcode = 20011975;  // for recognizing EWOCvj content files
    ofile.write((const char *)&recogcode, 4);
    int size = paths.size();
    ofile.write((const char *)&size, 4);
    for (int i = 0; i < paths.size(); i++) {
        // save header
        if (paths[i] == "") continue;
        std::fstream fileInput;
        fileInput.open(paths[i], std::ios::in | std::ios::binary);
        int fileSize = getFileSize(paths[i]);
        ofile.write((const char *)&fileSize, 4);
        fileInput.close();
    }
    for (int i = 0; i < paths.size(); i++) {
        // save body
        if (paths[i] == "") continue;
        int fileSize = getFileSize(paths[i]);
        if (fileSize == -1) continue;
        std::fstream fileInput;
        fileInput.open(paths[i], std::ios::in | std::ios::binary);
        char *inputBuffer = new char[fileSize];
        fileInput.read(inputBuffer, fileSize);
        ofile.write(inputBuffer, fileSize);
        delete[] inputBuffer;
        fileInput.close();
    }
    ofile.close();
    if (exists(ofpath)) {
        while (exists(path)) {
            std::error_code ec;
            std::filesystem::remove(path, ec);
        }
        std::filesystem::rename(ofpath, path);
    }

    mainprogram->concatting--;
    mainprogram->numconcatted++;
    if (mainprogram->concatting == 0) {
        mainprogram->numconcatted = 0;
    }
}


void Program::delete_text(std::string str) {
    // destroy a GUIString element from the map
    GUIString *gs = this->guitextmap[str];
    if (gs) {
        if (gs->texturevec.size() == 1) {
            mainprogram->guitextmap.erase(str);
            delete gs;  // texture is killed in the destructor
        }
    }
}


Menu::~Menu() {
    delete this->box;
}


void Program::register_undo(Param *par, Button *but) {
    if (!mainprogram->undoon || loopstation->foundrec) return;
    int sz = this->undomapvec[this->undopos - 1].size();
    for (int i = 0; i < sz - this->undopbpos; i++) {
        this->undomapvec[this->undopos - 1].pop_back();
    }
    int laydeck = -1;
    int laypos = -1;
    int effcat = -1;
    int effpos = -1;
    int parpos = -1;
    std::string name = "";
    if (par) {
        name = par->name;
        if (par->layer) {
            laydeck = par->layer->comp * 2 + par->layer->deck;
            laypos = par->layer->pos;
            if (par->effect){
                if (std::find(par->layer->effects[0].begin(), par->layer->effects[0].end(), par->effect) != par->layer->effects[0].end()) {
                    effcat = 0;
                }
                else if (std::find(par->layer->effects[1].begin(), par->layer->effects[1].end(), par->effect) != par->layer->effects[1].end()) {
                    effcat = 1;
                }
                effpos = par->effect->pos;
                parpos = std::find(par->effect->params.begin(), par->effect->params.end(), par) - par->effect->params.begin();
                if (parpos == par->effect->params.size()) {
                    parpos = -1;
                }
            }
        }

        if (par != (Param*)this->currundoelem) {
            std::tuple tup1 = std::make_tuple(par, nullptr, laydeck, laypos, effcat, effpos, parpos, name);
            std::tuple tup2 = std::make_tuple(tup1, par->oldvalue);
            this->undomapvec[this->undopos - 1].push_back(tup2);
            this->currundoelem = (void*)par;
            this->undopbpos++;
        }
        std::tuple tup1 = std::make_tuple(par, nullptr, laydeck, laypos, effcat, effpos, parpos, name);
        std::tuple tup2 = std::make_tuple(tup1, par->value);
        this->undomapvec[this->undopos - 1].push_back(tup2);
    }
    else if (but) {
        std::string name2 = but->name[0];
        if (but->layer) {
            laydeck = but->layer->comp * 2 + but->layer->deck;
            laypos = but->layer->pos;
            if (name2 == "onoffbutton") {
                for (Effect *eff : but->layer->effects[0]) {
                    if (but == eff->onoffbutton) {
                        effcat = 0;
                        effpos = eff->pos;
                        break;
                    }
                }
                if (effcat == -1) {
                    for (Effect *eff : but->layer->effects[1]) {
                        if (but == eff->onoffbutton) {
                            effcat = 1;
                            effpos = eff->pos;
                            break;
                        }
                    }
                }
            }
        }
        if (but != (Button*)this->currundoelem) {
            std::tuple tup1 = std::make_tuple(nullptr, but, laydeck, laypos, effcat, effpos, parpos, name2);
            std::tuple tup2 = std::make_tuple(tup1, !but->value);
            this->undomapvec[this->undopos - 1].push_back(tup2);;
            this->currundoelem = (void*)but;
            this->undopbpos++;
        }
        std::tuple tup1 = std::make_tuple(nullptr, but, laydeck, laypos, effcat, effpos, parpos, name2);
        std::tuple tup2 = std::make_tuple(tup1, but->value);
        this->undomapvec[this->undopos - 1].push_back(tup2);
    }
    this->undopbpos++;
    this->recundo = false;
}

void Program::undo_redo_parbut(char offset, bool again, bool swap) {
    if (!mainprogram->undoon || loopstation->foundrec) return;
    if (this->undomapvec.size()) {
        auto tup2 = this->undomapvec[this->undopos - 1][this->undopbpos + offset];
        int laydeck = -1;
        int laypos = -1;
        int effcat = -1;
        int effpos = -1;
        int parpos = -1;
        auto tup1 = std::get<0>(tup2);
        std::string name = std::get<7>(tup1);
        Param *newpar = std::get<0>(tup1);
        Param *par = newpar;
        std::vector<Layer *> layers;
        if (par) {
            // search corresponding parameter in possibly changed layers
            auto newpartuple = mainprogram->newparam(offset, swap);
            newpar = std::get<0>(newpartuple);
            laydeck = std::get<1>(newpartuple);
            if (laydeck != -1) {
                if (swap) {
                    layers = mainmix->newlrs[laydeck];
                } else {
                    layers = mainmix->layers[laydeck];
                }
            }
            laypos = std::get<2>(newpartuple);
            effcat = std::get<3>(newpartuple);
            effpos = std::get<4>(newpartuple);
            parpos = std::get<5>(newpartuple);
            std::tuple tup3 = std::make_tuple(newpar, nullptr, laydeck, laypos, effcat, effpos, parpos, name);
            std::tuple tup4 = std::make_tuple(tup3, std::get<1>(tup2));
            this->undomapvec[this->undopos - 1][this->undopbpos + offset] = tup4;
        }

        Button *newbut = std::get<1>(tup1);
        Button *but = newbut;
        if (but) {
            // search corresponding button in possibly changed layers
            auto newbuttuple = mainprogram->newbutton(offset, swap);
            newbut = std::get<0>(newbuttuple);
            laydeck = std::get<1>(newbuttuple);
            if (laydeck != -1) {
                if (swap) {
                    layers = mainmix->newlrs[laydeck];
                } else {
                    layers = mainmix->layers[laydeck];
                }
            }
            laypos = std::get<2>(newbuttuple);
            effcat = std::get<3>(newbuttuple);
            effpos = std::get<4>(newbuttuple);
            std::tuple tup3 = std::make_tuple(nullptr, newbut, laydeck, laypos, effcat, effpos, parpos, name);
            std::tuple tup4 = std::make_tuple(tup3, std::get<1>(tup2));
            this->undomapvec[this->undopos - 1][this->undopbpos + offset] = tup4;
        }
        if (laydeck != -1 && !again) {
            mainmix->change_currlay(mainmix->currlay[!mainprogram->prevmodus], layers[laypos]);
        }
    }

    auto tup5 = this->undomapvec[this->undopos - 1][this->undopbpos + offset];
    auto tup = std::get<0>(tup5);
    float val = std::get<1>(tup5);
    if (std::get<0>(tup)) {
        std::get<0>(tup)->value = val;
    }
    else {
        std::get<1>(tup)->value = val;
        std::get<1>(tup)->skiponetoggled = true;
    }
    if (!again) {
        bool cond;
        if (offset == -2) {
            cond = (this->undopbpos + offset > 0);
        } else if (offset == 0) {
            cond = (this->undomapvec[this->undopos - 1].size() > this->undopbpos + 1);
        }
        if (cond) {
            auto tup6 = this->undomapvec[this->undopos - 1][this->undopbpos + offset + offset + 1];
            auto tup7 = std::get<0>(tup6);
            if (std::get<0>(tup) != std::get<0>(tup7) || std::get<1>(tup) != std::get<1>(tup7)) {
                this->undo_redo_parbut(offset + offset + 1, true, swap);
                this->undopbpos += offset + 1;
            }
        }
    }
}

std::tuple<Param*, int, int, int, int, int> Program::newparam(int offset, bool swap) {
    auto tup2 = this->undomapvec[this->undopos - 1][this->undopbpos + offset];
    int laydeck = -1;
    int laypos = -1;
    int effcat = -1;
    int effpos = -1;
    int parpos = -1;
    auto tup1 = std::get<0>(tup2);
    Param *newpar = std::get<0>(tup1);
    std::string name = std::get<7>(tup1);
// search corresponding parameter in possibly changed layers
    if (std::get<2>(tup1) != -1) {
        laydeck = std::get<2>(tup1);
        std::vector<Layer *> layers;
        if (swap) {
            layers = mainmix->newlrs[laydeck];
        } else {
            layers = mainmix->layers[laydeck];
        }
        laypos = std::get<3>(tup1);
        Layer * lay = layers[laypos];
        if (swap) {
            if (lay == nullptr) {
                lay = mainmix->layers[laydeck][laypos];
            }
        }
        if (std::get<4>(tup1) != -1) {
            effcat = std::get<4>(tup1);
            effpos = std::get<5>(tup1);
            parpos = std::get<6>(tup1);
            if (parpos == -1) {
                if (name == "drywet") {
                    newpar = lay->effects[effcat][effpos]->drywet;
                }
            } else {
                newpar = lay->effects[effcat][effpos]->params[parpos];
            }
        } else {
            if (name == "shiftx") {
                newpar = lay->shiftx;
            } else if (name == "shifty") {
                newpar = lay->shifty;
            } else if (name == "scale") {
                newpar = lay->scale;
            } else if (name == "startframe") {
                newpar = lay->startframe;
            } else if (name == "endframe") {
                newpar = lay->endframe;
            } else if (name == "Tolerance") {
                newpar = lay->chtol;
            } else if (name == "Speed") {
                newpar = lay->speed;
            } else if (name == "Opacity") {
                newpar = lay->opacity;
            } else if (name == "Volume") {
                newpar = lay->volume;
            }
        }
    }
    auto rettuple = std::make_tuple(newpar, laydeck, laypos, effcat, effpos, parpos);
    return rettuple;
}

std::tuple<Button*, int, int, int, int> Program::newbutton(int offset, bool swap) {
    auto tup2 = this->undomapvec[this->undopos - 1][this->undopbpos + offset];
    auto tup1 = std::get<0>(tup2);
    Button *newbut = std::get<1>(tup1);
    int laydeck = -1;
    int laypos = -1;
    int effcat = -1;
    int effpos = -1;
    int parpos = -1;
    std::string name = std::get<7>(tup1);
    if (std::get<2>(tup1) != -1) {
        laydeck = std::get<2>(tup1);
        std::vector<Layer*> layers;
        if (swap) {
            layers = mainmix->newlrs[laydeck];
        }
        else {
            layers = mainmix->layers[laydeck];
        }
        laypos = std::get<3>(tup1);
        Layer * lay = layers[laypos];
        if (swap) {
            if (lay == nullptr) {
                lay = mainmix->layers[laydeck][laypos];
            }
        }
        effcat = std::get<4>(tup1);
        effpos = std::get<5>(tup1);
        if (effcat != -1) {
            if (name == "onoffbutton") {
                newbut = lay->effects[effcat][effpos]->onoffbutton;
            }
        }
        if (name == "mutebut") {
            newbut = lay->mutebut;
        } else if (name == "solobut") {
            newbut = lay->solobut;
        } else if (name == "keepeffbut") {
            newbut = lay->keepeffbut;
        } else if (name == "queuebut") {
            newbut = lay->queuebut;
        } else if (name == "playbut") {
            newbut = lay->playbut;
        } else if (name == "frameforward") {
            newbut = lay->frameforward;
        } else if (name == "framebackward") {
            newbut = lay->framebackward;
        } else if (name == "revbut") {
            newbut = lay->revbut;
        } else if (name == "bouncebut") {
            newbut = lay->bouncebut;
        } else if (name == "stopbut") {
            newbut = lay->stopbut;
        } else if (name == "lpbut") {
            newbut = lay->lpbut;
        } else if (name == "genmidibut") {
            newbut = lay->genmidibut;
        } else if (name == "chdir") {
            newbut = lay->chdir;
        } else if (name == "chinv") {
            newbut = lay->chinv;
        }
    }
    auto rettuple = std::make_tuple(newbut, laydeck, laypos, effcat, effpos);
    return rettuple;
}

void Program::undo_redo_save() {
    if (!mainprogram->undoon || loopstation->foundrec) return;
    bool found = false;
    for (int i = 0; i < 4; i++) {
        if (mainmix->swapmap[i].size()) {
            found = true;
            break;
        }
    }
    if (this->concatting) found = true;
    bool brk = false;
    for (int i = 0; i < 4; i++) {
        for (Layer *lay : mainmix->layers[i]) {
            if (lay->vidopen) {
                found = true;
                brk = true;
                break;
            }
        }
        if (brk) {
            break;
        }
    }
    if (this->pathto != "") {
        found = true;
    }
    if (this->openfileslayers || this->openfilesqueue || this->openfilesshelf || binsmain->openfilesbin) {
        found = true;
    }
    if (!found) {
        if (!this->binsscreen) {
            std::string undopath = find_unused_filename("UNDO_state", this->temppath, ".ewocvj");
            for (int i = 0; i < this->undopaths.size() - this->undopos; i++) {
                this->undopaths.pop_back();
            }
            this->undopaths.push_back(undopath);
            this->project->save(undopath, false, true);

            std::vector<std::tuple<std::tuple<Param*, Button*, int, int, int, int, int, std::string>, float>> uvec;
            this->undomapvec.push_back(uvec);
            this->undopbpos = 0;
            this->undopos++;
            this->recundo = false;
            this->undowaiting = false;
        }
    }
    if (found) {
        this->undowaiting = true;
    }

    found = false;
    if (binsmain->openfilesbin) found = true;
    if (!found) {
        if (this->binsscreen) {
            int bupos = binsmain->currbin->pos;
            Bin *bin = new Bin(-1);
            for (int i = 0; i < 12; i++) {
                for (int j = 0; j < 12; j++) {
                    BinElement *binel = new BinElement;
                    bin->elements.push_back(binel);
                }
            }
            bin->name = binsmain->currbin->name;
            bin->path = binsmain->currbin->path;
            bin->pos = binsmain->currbin->pos;
            bin->shared = binsmain->currbin->shared;
            bin->saved = binsmain->currbin->saved;
            for (int i = 0; i < 144; i++) {
                bin->elements[i]->path = binsmain->currbin->elements[i]->path;
                bin->elements[i]->type = binsmain->currbin->elements[i]->type;
                bin->elements[i]->name = binsmain->currbin->elements[i]->name;
                bin->elements[i]->oldname = binsmain->currbin->elements[i]->oldname;
                bin->elements[i]->oldpath = binsmain->currbin->elements[i]->oldpath;
                bin->elements[i]->relpath = binsmain->currbin->elements[i]->relpath;
                bin->elements[i]->absjpath = binsmain->currbin->elements[i]->absjpath;
                bin->elements[i]->reljpath = binsmain->currbin->elements[i]->reljpath;
                bin->elements[i]->jpegpath = binsmain->currbin->elements[i]->jpegpath;
                bin->elements[i]->jpegsaved = binsmain->currbin->elements[i]->jpegsaved;
                bin->elements[i]->autosavejpegsaved = binsmain->currbin->elements[i]->autosavejpegsaved;
                bin->elements[i]->filesize = binsmain->currbin->elements[i]->filesize;
                bin->elements[i]->tex = binsmain->currbin->elements[i]->tex;
                bin->elements[i]->oldtex = binsmain->currbin->elements[i]->oldtex;
                bin->elements[i]->full = binsmain->currbin->elements[i]->full;
                bin->elements[i]->oldfull = binsmain->currbin->elements[i]->oldfull;
                bin->elements[i]->select = binsmain->currbin->elements[i]->select;
                bin->elements[i]->oldselect = binsmain->currbin->elements[i]->oldselect;
                bin->elements[i]->boxselect = binsmain->currbin->elements[i]->boxselect;
                bin->elements[i]->temp = binsmain->currbin->elements[i]->temp;
                bin->elements[i]->full = binsmain->currbin->elements[i]->full;
            }
            std::tuple pair = std::make_tuple(bin, bin->name);
            for (int i = 0; i < binsmain->undobins.size() - binsmain->undopos; i++) {
                binsmain->undobins.pop_back();
            }
            binsmain->undobins.push_back(pair);
            binsmain->undopos++;

            this->recundo = false;
            this->undowaiting = false;
        }
    }
    if (found) {
        this->undowaiting = true;
    }
}


#ifdef POSIX
void Program::register_v4l2lbdevices(std::vector<std::string>& entries, GLuint tex) {
    // handle selection of active v4l2loopback devices used to stream video at
    std::string res = exec("v4l2-ctl --list-devices");
    int pos = res.find("platform:v4l2loopback",0);
    if (pos == std::string::npos) return;
    int v4lstart = entries.size();
    entries.push_back("submenu loopbackmenu");
    entries.push_back("Send to v4l2 loopback device");
    std::vector<std::string> lbentries;
    while (pos != std::string::npos) {
        int pos2 = res.find("/dev/video", pos);
        int n = std::stoi(res.substr(pos2 + 10, 1));
        int tot = n;
        pos2++;
        while (res.substr(pos2 + 11, 1) != "\n") {
            int n = std::stoi(res.substr(pos2, 1));
            tot *= 10;
            tot += n;
            pos2++;
        }
        std::string device = "/dev/video" + std::to_string(tot);

        bool set = false;
        if (this->v4l2lbtexmap.count(device)) {
            if (this->v4l2lbtexmap[device] == tex) {
                lbentries.push_back("V " + device);
                set = true;
            }
        }
        if (!set) {
            lbentries.push_back("  " + device);
        }
        pos = res.find("platform:v4l2loopback", pos + 1);
    }
    this->make_menu("loopbackmenu", this->loopbackmenu, lbentries);

    return;
}

void Program::v4l2_start_device(std::string device, GLuint tex) {
    // open output device
    if (v4l2lbtexmap[device] == tex) {
        v4l2lbtexmap.erase(device);
        close(this->v4l2lboutputmap[device]);
        return;
    } else {
        if (v4l2lbtexmap.count(device)) {
            close(this->v4l2lboutputmap[device]);
        }
        v4l2lbtexmap[device] = tex;
        v4l2lbnewmap[device] = 0;

        int output = open(device.c_str(), O_RDWR);
        if (output < 0) {
            std::cerr << "ERROR: could not open output device!\n" <<
                      strerror(errno);
            return;
        }
        this->v4l2lboutputmap[device] = output;
        this->set_v4l2format(output, tex);
    }
}

size_t Program::set_v4l2format(int output, GLuint tex) {
    // acquire video format struct from device
    struct v4l2_format vid_format;
    memset(&vid_format, 0, sizeof(vid_format));
    vid_format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

    if (ioctl(output, VIDIOC_G_FMT, &vid_format) < 0) {
        std::cerr << "ERROR: unable to get video format!\n" <<
                  strerror(errno);
        return 0;
    }

    int sw1, sh1;
    glBindTexture(GL_TEXTURE_2D, tex);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &sw1);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &sh1);
    size_t framesize;
    // configure desired video format on device
    framesize = sw1 * sh1 * 4;
    vid_format.fmt.pix.width = sw1;
    vid_format.fmt.pix.height = sh1;
    vid_format.fmt.pix.pixelformat = V4L2_PIX_FMT_BGR32;
    vid_format.fmt.pix.sizeimage = framesize;
    vid_format.fmt.pix.field = V4L2_FIELD_NONE;

    if (ioctl(output, VIDIOC_S_FMT, &vid_format) < 0) {
        std::cerr << "ERROR: unable to set video format!\n" <<
                  strerror(errno);
        return 0;
    }

    return framesize;
}
#endif



void Program::add_to_texpool(GLuint tex) {
    int sw, sh;
    glBindTexture(GL_TEXTURE_2D, tex);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &sw);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &sh);
    GLint compressedGL = this->texintfmap[tex];
    if (sw == 0) {
        glDeleteTextures(1, &tex);
        return;
    }
    glClearTexImage(tex, 0, GL_BGRA, GL_UNSIGNED_BYTE, black);
    this->texpool.insert(std::pair<std::tuple<int, int, GLint>, GLuint>(std::tuple<int, int, GLint>(sw, sh, compressedGL), tex));
}

GLuint Program::grab_from_texpool(int w, int h, GLint compressed) {
    auto elem = this->texpool.find(std::tuple<int, int, GLint>(w, h, compressed));
    if (elem == this->texpool.end()) {
        return -1;
    }
    GLuint tex = elem->second;
    this->texpool.erase(elem);
    return tex;
}

void Program::add_to_pbopool(GLuint pbo, GLubyte *mapptr) {
    GLint bufferSize = 0;
    glBindBuffer(GL_ARRAY_BUFFER, pbo);
    glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);
    mapptr = nullptr;
    this->pbopool.insert(std::pair<int, std::pair<GLuint, GLubyte*>>(bufferSize, std::pair<GLuint, GLubyte*>(pbo, mapptr)));
}

std::pair<GLuint, GLubyte*> Program::grab_from_pbopool(int bsize) {
    auto elem = this->pbopool.find(bsize);
    if (elem == this->pbopool.end()) {
        return std::pair<GLuint, GLubyte *>(-1, nullptr);
    }
    auto retelem = elem->second;
    this->pbopool.erase(elem);
    return retelem;
}

void Program::add_to_fbopool(GLuint fbo) {
    this->fbopool.insert(fbo);
}

GLuint Program::grab_from_fbopool() {
    if (this->fbopool.empty()) {
        return -1;
    }
    GLuint fbo = *this->fbopool.begin();
    this->fbopool.erase(fbo);
    return fbo;
}