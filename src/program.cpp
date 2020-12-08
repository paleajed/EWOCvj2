#if defined(WIN32) && !defined(__linux__)
#define WINDOWS
#elif defined(__linux__) && !defined(WIN32)
#define POSIX
#endif

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include <ostream>
#include <string>

#include "GL/glew.h"
#include "GL/gl.h"
#include "GL/glut.h"
#ifdef POSIX
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <rtmidi/RtMidi.h>
#include <linux/videodev2.h>
#include <alsa/asoundlib.h>
#include <sys/ioctl.h>
#include "tinyfiledialogs.h"
#endif

#ifdef WINDOWS
#include <tchar.h>
#include "direnthwin/dirent.h"
#include <intrin.h>
#include <shobjidl.h>
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




Program::Program() {
	this->project = new Project;

#ifdef WINDOWS
	PWSTR charbuf;
	HRESULT hr = SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &charbuf);
	std::wstring ws1(charbuf);
	std::string str1(ws1.begin(), ws1.end());
	this->docpath = boost::filesystem::canonical(str1).string() + "/EWOCvj2/";
	hr = SHGetKnownFolderPath(FOLDERID_Fonts, 0, NULL, &charbuf);
	std::wstring ws2(charbuf);
	std::string str2(ws2.begin(), ws2.end());
	this->fontpath = str2;
	hr = SHGetKnownFolderPath(FOLDERID_Videos, 0, NULL, &charbuf);
	std::wstring ws4(charbuf);
	std::string str4(ws4.begin(), ws4.end());
	this->contentpath = str4;
	std::wstring ws3;
	wchar_t wcharPath[MAX_PATH];
	if (GetTempPathW(MAX_PATH, wcharPath)) ws3 = wcharPath;
	std::string str3(ws3.begin(), ws3.end());
	this->temppath = str3 + "EWOCvj2/";
#endif
#ifdef POSIX
	std::string homedir(getenv("HOME"));
	this->temppath = homedir + "/.ewocvj2/temp/";
    this->docpath = homedir + "/Documents/EWOCvj2/";
    this->contentpath = homedir + "/Videos/";
#endif
	this->currfilesdir = this->contentpath;
	this->currclipfilesdir = this->contentpath;
	this->currshelffilesdir = this->contentpath;
	this->currbinfilesdir = this->contentpath;

	this->numh = this->numh * glob->w / glob->h;
	this->layh = this->layh * (glob->w / glob->h) / (1920.0f /  1080.0f);
	this->monh = this->monh * (glob->w / glob->h) / (1920.0f /  1080.0f);
	
	this->addbox = new Box;
	this->addbox->vtxcoords->y1 = 1.0f - this->layh;
	this->addbox->vtxcoords->w = 0.009f * 2.0f;
	this->addbox->vtxcoords->h = this->layh - 0.075f;
	this->addbox->upvtxtoscr();
	this->addbox->reserved = true;
	this->addbox->tooltiptitle = "Add layer ";
	this->addbox->tooltip = "Leftclick this blue box to add a layer at this point. ";

	this->delbox = new Box;
	this->delbox->vtxcoords->y1 = 0.925f;
	this->delbox->vtxcoords->w = 0.009f * 2.0f;
	this->delbox->vtxcoords->h = 0.075f;
	this->delbox->upvtxtoscr();
	this->delbox->reserved = true;
	this->delbox->tooltiptitle = "Delete layer to the left ";
	this->delbox->tooltip = "Leftclick this red box to delete the layer to the left of it. ";

	this->scrollboxes[0] = new Box;
	this->scrollboxes[0]->vtxcoords->x1 = -1.0f + this->numw;
	this->scrollboxes[0]->vtxcoords->y1 = 1.0f - this->layh - 0.05f;
	this->scrollboxes[0]->vtxcoords->w = this->layw * 3.0f;
	this->scrollboxes[0]->vtxcoords->h = 0.05f;
	this->scrollboxes[0]->upvtxtoscr();
	this->scrollboxes[0]->tooltiptitle = "Deck A Layer stack scroll bar ";
	this->scrollboxes[0]->tooltip = "Scroll bar for layer stack.  Divided in total number of layers parts for deck A. Lightgrey part is the visible layer monitors part.  Leftdragging the lightgrey part allows scrolling.  So does leftclicking the darkgrey parts. ";
	this->scrollboxes[1] = new Box;
	this->scrollboxes[1]->vtxcoords->x1 = this->numw;
	this->scrollboxes[1]->vtxcoords->y1 = 1.0f - this->layh - 0.05f;
	this->scrollboxes[1]->vtxcoords->w = this->layw * 3.0f;
	this->scrollboxes[1]->vtxcoords->h = 0.05f;
	this->scrollboxes[1]->upvtxtoscr();
	this->scrollboxes[1]->tooltiptitle = "Deck b Layer stack scroll bar ";
	this->scrollboxes[1]->tooltip = "Scroll bar for layer stack.  Divided in total number of layers parts for deck B. Lightgrey part is the visible layer monitors part.  Leftdragging the lightgrey part allows scrolling.  So does leftclicking the darkgrey parts. ";
	
	this->cwbox = new Box;

	this->toscreen = new Button(false);
	this->toscreen->toggle = 0;
	this->buttons.push_back(this->toscreen);
	this->toscreen->name[0] = "SEND";
	this->toscreen->name[1] = "SEND";
	this->toscreen->box->vtxcoords->x1 = -0.3;
	this->toscreen->box->vtxcoords->y1 = -1.0f + this->monh;
	this->toscreen->box->vtxcoords->w = 0.3 / 2.0;
	this->toscreen->box->vtxcoords->h = 0.3 / 3.0;
	this->toscreen->box->upvtxtoscr();
	this->toscreen->box->tooltiptitle = "Send preview streams to output ";
	this->toscreen->box->tooltip = "Leftclick sends/copies the streams being previewed to the output streams ";
	
	this->backtopre = new Button(false);
	this->backtopre->toggle = 0;
	this->buttons.push_back(this->backtopre);
	this->backtopre->name[0] = "SEND";
	this->backtopre->name[1] = "SEND";
	this->backtopre->box->vtxcoords->x1 = -0.3;
	this->backtopre->box->vtxcoords->y1 = -1.0f + this->monh - 0.1f;
	this->backtopre->box->vtxcoords->w = 0.15f;
	this->backtopre->box->vtxcoords->h = 0.1f;
	this->backtopre->box->upvtxtoscr();
	this->backtopre->box->tooltiptitle = "Send output streams to preview ";
	this->backtopre->box->tooltip = "Leftclick sends/copies the streams being output back into the preview streams ";
	
	this->modusbut = new Button(false);
	this->modusbut->toggle = 1;
	this->buttons.push_back(this->modusbut);
	this->modusbut->name[0] = "LIVE MODUS";
	this->modusbut->name[1] = "PREVIEW MODUS";
    this->modusbut->box->lcolor[0] = 0.7f;
    this->modusbut->box->lcolor[1] = 0.7f;
    this->modusbut->box->lcolor[2] = 0.7f;
    this->modusbut->box->lcolor[3] = 1.0f;
	this->modusbut->box->vtxcoords->x1 = -0.3 - (0.3 / 2.0);
	this->modusbut->box->vtxcoords->y1 = -1.0f + this->monh;
	this->modusbut->box->vtxcoords->w = 0.3 / 2.0;
	this->modusbut->box->vtxcoords->h = 0.3 / 3.0;
	this->modusbut->box->upvtxtoscr();
	this->modusbut->box->tooltiptitle = "LIVE/PREVIEW modus switch ";
	this->modusbut->box->tooltip = "Leftclicking toggles between preview modus (changes are previewed, then sent to/from output) and live modus (changes appear in output immediately) ";
	this->modusbut->tcol[0] = 0.0f;
	this->modusbut->tcol[1] = 0.0f;
	this->modusbut->tcol[2] = 0.0f;
	this->modusbut->tcol[3] = 1.0f;
	
	this->outputmonitor = new Box;
	this->outputmonitor->vtxcoords->x1 = -0.15f;
	this->outputmonitor->vtxcoords->y1 = -1.0f + this->monh;
	this->outputmonitor->vtxcoords->w = 0.3f;
	this->outputmonitor->vtxcoords->h = this->monh;
	this->outputmonitor->upvtxtoscr();
	this->outputmonitor->tooltiptitle = "Output monitor ";
	this->outputmonitor->tooltip = "Shows mix of output stream when in preview modus.  Rightclick menu allows sending monitor image fullscreen to another display device. ";

	this->mainmonitor = new Box;
	this->mainmonitor->vtxcoords->x1 = -0.3f;
	this->mainmonitor->vtxcoords->y1 = -1.0f;
	this->mainmonitor->vtxcoords->w = 0.6f;
	this->mainmonitor->vtxcoords->h = this->monh * 2.0f;
	this->mainmonitor->upvtxtoscr();
	this->mainmonitor->tooltiptitle = "A+B monitor ";
	this->mainmonitor->tooltip = "Shows crossfaded or wiped A+B mix of preview stream (preview modus) or output stream (live modus).  Rightclick menu allows setting wipes and sending monitor image fullscreen to another display device. Leftclick/drag on image affects some wipe modes. ";

	this->deckmonitor[0] = new Box;
	this->deckmonitor[0]->vtxcoords->x1 = -0.6f;
	this->deckmonitor[0]->vtxcoords->y1 = -1.0f;
	this->deckmonitor[0]->vtxcoords->w = 0.3f;
	this->deckmonitor[0]->vtxcoords->h = this->monh;
	this->deckmonitor[0]->upvtxtoscr();
	this->deckmonitor[0]->tooltiptitle = "Deck A monitor ";
	this->deckmonitor[0]->tooltip = "Shows deck A stream.  Rightclick menu allows sending monitor image fullscreen to another display device. Leftclick/drag on image affects some local layer wipe modes. ";

	this->deckmonitor[1] = new Box;
	this->deckmonitor[1]->vtxcoords->x1 = 0.3f;
	this->deckmonitor[1]->vtxcoords->y1 = -1.0f;
	this->deckmonitor[1]->vtxcoords->w = 0.3f;
	this->deckmonitor[1]->vtxcoords->h = this->monh;
	this->deckmonitor[1]->upvtxtoscr();
	this->deckmonitor[1]->tooltiptitle = "Deck B monitor ";
	this->deckmonitor[1]->tooltip = "Shows deck B stream.  Rightclick menu allows sending monitor image fullscreen to another display device. Leftclick/drag on image affects some local layer wipe modes. ";

	this->effcat[0] = new Button(false);
	this->effcat[0]->toggle = 1;
	this->buttons.push_back(this->effcat[0]);
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
	
	this->effcat[1] = new Button(false);
	this->effcat[1]->toggle = 1;
	this->buttons.push_back(this->effcat[1]);
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
	
	this->effscrollupA = new Box;
	this->effscrollupA->vtxcoords->x1 = -1.0;
	this->effscrollupA->vtxcoords->y1 = 1.0 - this->layh * 1.5f;
	this->effscrollupA->vtxcoords->w = 0.0375f;
	this->effscrollupA->vtxcoords->h = 0.075f;
	this->effscrollupA->upvtxtoscr();
	this->effscrollupA->tooltiptitle = "Scroll effects queue up ";
	this->effscrollupA->tooltip = "Leftclicking scrolls the effect queue up ";
	
	this->effscrollupB = new Box;
	this->effscrollupB->vtxcoords->x1 = 1.0 - 0.075f;
	this->effscrollupB->vtxcoords->y1 = 1.0 - this->layh * 1.5f;
	this->effscrollupB->vtxcoords->w = 0.0375f;
	this->effscrollupB->vtxcoords->h = 0.075f;
	this->effscrollupB->upvtxtoscr();
	this->effscrollupB->tooltiptitle = "Scroll effects queue up ";
	this->effscrollupB->tooltip = "Leftclicking scrolls the effect queue up ";
	
	this->effscrolldownA = new Box;
	this->effscrolldownA->vtxcoords->x1 = -1.0;
	this->effscrolldownA->vtxcoords->y1 = 1.0 - this->layh * 1.5f - 0.375f - 0.075f;
	this->effscrolldownA->vtxcoords->w = 0.0375f;
	this->effscrolldownA->vtxcoords->h = 0.075f;
	this->effscrolldownA->upvtxtoscr();
	this->effscrolldownA->tooltiptitle = "Scroll effects queue down ";
	this->effscrolldownA->tooltip = "Leftclicking scrolls the effect queue down ";
	
	this->effscrolldownB = new Box;
	this->effscrolldownB->vtxcoords->x1 = 1.0 - 0.075f;
	this->effscrolldownB->vtxcoords->y1 = 1.0 - this->layh * 1.5f - 0.375f - 0.075f;
	this->effscrolldownB->vtxcoords->w = 0.0375f;
	this->effscrolldownB->vtxcoords->h = 0.075f;
	this->effscrolldownB->upvtxtoscr();
	this->effscrolldownB->tooltiptitle = "Scroll effects queue down ";
	this->effscrolldownB->tooltip = "Leftclicking scrolls the effect queue down ";

    this->orderscrollup = new Box;
    this->orderscrollup->vtxcoords->x1 = -0.45f;
    this->orderscrollup->vtxcoords->y1 = 0.8f;
    this->orderscrollup->vtxcoords->w = 0.05f;
    this->orderscrollup->vtxcoords->h = 0.1f;
    this->orderscrollup->upvtxtoscr();
    this->orderscrollup->tooltiptitle = "Scroll orderlist up ";
    this->orderscrollup->tooltip = "Leftclicking scrolls the orderlist up ";

    this->orderscrolldown = new Box;
    this->orderscrolldown->vtxcoords->x1 = -0.45f;
    this->orderscrolldown->vtxcoords->y1 = 0.7f;
    this->orderscrolldown->vtxcoords->w = 0.05f;
    this->orderscrolldown->vtxcoords->h = 0.1f;
    this->orderscrolldown->upvtxtoscr();
    this->orderscrolldown->tooltiptitle = "Scroll orderlist down ";
    this->orderscrolldown->tooltip = "Leftclicking scrolls the orderlist down ";

    this->defaultsearchscrollup = new Box;
    this->defaultsearchscrollup->vtxcoords->x1 = -0.6f;
    this->defaultsearchscrollup->vtxcoords->y1 = -0.4f;
    this->defaultsearchscrollup->vtxcoords->w = 0.1f;
    this->defaultsearchscrollup->vtxcoords->h = 0.2f;
    this->defaultsearchscrollup->upvtxtoscr();
    this->defaultsearchscrollup->tooltiptitle = "Scroll default searchlist up ";
    this->defaultsearchscrollup->tooltip = "Leftclicking scrolls the default searchlist up ";

    this->defaultsearchscrolldown = new Box;
    this->defaultsearchscrolldown->vtxcoords->x1 = -0.6f;
    this->defaultsearchscrolldown->vtxcoords->y1 = -0.6f;
    this->defaultsearchscrolldown->vtxcoords->w = 0.1f;
    this->defaultsearchscrolldown->vtxcoords->h = 0.2f;
    this->defaultsearchscrolldown->upvtxtoscr();
    this->defaultsearchscrolldown->tooltiptitle = "Scroll default searchlist down ";
    this->defaultsearchscrolldown->tooltip = "Leftclicking scrolls the default searchlist down ";

    this->searchscrollup = new Box;
    this->searchscrollup->vtxcoords->x1 = -0.45f;
    this->searchscrollup->vtxcoords->y1 = -0.2f;
    this->searchscrollup->vtxcoords->w = 0.05f;
    this->searchscrollup->vtxcoords->h = 0.1f;
    this->searchscrollup->upvtxtoscr();
    this->searchscrollup->tooltiptitle = "Scroll searchlist up ";
    this->searchscrollup->tooltip = "Leftclicking scrolls the searchlist up ";

    this->searchscrolldown = new Box;
    this->searchscrolldown->vtxcoords->x1 = -0.45f;
    this->searchscrolldown->vtxcoords->y1 = -0.3f;
    this->searchscrolldown->vtxcoords->w = 0.05f;
    this->searchscrolldown->vtxcoords->h = 0.1f;
    this->searchscrolldown->upvtxtoscr();
    this->searchscrolldown->tooltiptitle = "Scroll searchlist down ";
    this->searchscrolldown->tooltip = "Leftclicking scrolls the searchlist down ";

    this->addeffectbox = new Box;
	this->addeffectbox->vtxcoords->w = this->layw * 1.5f;
	this->addeffectbox->vtxcoords->h = 0.057f;
	this->addeffectbox->tooltiptitle = "Add effect ";
	this->addeffectbox->tooltip = "Add effect to end of layer effect queue ";

	this->inserteffectbox = new Box;
	this->inserteffectbox->vtxcoords->w = 0.24f;
	this->inserteffectbox->vtxcoords->h = 0.057f;
	this->inserteffectbox->tooltiptitle = "Add effect ";
	this->inserteffectbox->tooltip = "Add effect to end of layer effect queue ";

	this->tmset = new Box;
	this->tmset->vtxcoords->x1 = -0.2f;
	this->tmset->vtxcoords->y1 = 0.74f;
	this->tmset->vtxcoords->w = 0.4f;
	this->tmset->vtxcoords->h = 0.26f;
	this->tmset->tooltiptitle = "Toggle MIDI set to configure ";
	this->tmset->tooltip = "Leftclick to toggle the MIDI set that is being configured. ";
	this->tmplay = new Box;
	this->tmplay->vtxcoords->x1 = 0.075;
	this->tmplay->vtxcoords->y1 = -0.9f;
	this->tmplay->vtxcoords->w = 0.15f;
	this->tmplay->vtxcoords->h = 0.26f;
	this->tmplay->tooltiptitle = "Set MIDI for play button ";
	this->tmplay->tooltip = "Leftclick to start waiting for a MIDI command that will trigger normal video play for this preset. ";
	this->tmbackw = new Box;
	this->tmbackw->vtxcoords->x1 = -0.225f;
	this->tmbackw->vtxcoords->y1 = -0.9f;
	this->tmbackw->vtxcoords->w = 0.15f;
	this->tmbackw->vtxcoords->h = 0.26f;
	this->tmbackw->tooltiptitle = "Set MIDI for reverse play button ";
	this->tmbackw->tooltip = "Leftclick to start waiting for a MIDI command that will trigger reverse video play for this preset. ";
	this->tmbounce = new Box;
	this->tmbounce->vtxcoords->x1 = -0.075f;
	this->tmbounce->vtxcoords->y1 = -0.9f;
	this->tmbounce->vtxcoords->w = 0.15f;
	this->tmbounce->vtxcoords->h = 0.26f;
	this->tmbounce->tooltiptitle = "Set MIDI for bounce play button ";
	this->tmbounce->tooltip = "Leftclick to start waiting for a MIDI command that will trigger bounce video play for this preset. ";
	this->tmfrforw = new Box;
	this->tmfrforw->vtxcoords->x1 = 0.225f;
	this->tmfrforw->vtxcoords->y1 = -0.9f;
	this->tmfrforw->vtxcoords->w = 0.15;
	this->tmfrforw->vtxcoords->h = 0.26f;
	this->tmfrforw->tooltiptitle = "Set MIDI for frame forward button ";
	this->tmfrforw->tooltip = "Leftclick to start waiting for a MIDI command that will trigger frame forward for this preset. ";
	this->tmfrbackw = new Box;
	this->tmfrbackw->vtxcoords->x1 = -0.375f;
	this->tmfrbackw->vtxcoords->y1 = -0.9f;
	this->tmfrbackw->vtxcoords->w = 0.15;
	this->tmfrbackw->vtxcoords->h = 0.26f;
	this->tmfrbackw->tooltiptitle = "Set MIDI for frame backward button ";
	this->tmfrbackw->tooltip = "Leftclick to start waiting for a MIDI command that will trigger frame backward for this preset. ";
	this->tmspeed = new Box;
	this->tmspeed->vtxcoords->x1 = -0.8f;
	this->tmspeed->vtxcoords->y1 = -0.5f;
	this->tmspeed->vtxcoords->w = 0.2f;
	this->tmspeed->vtxcoords->h = 1.0f;
	this->tmspeed->tooltiptitle = "Set MIDI for setting video play speed ";
	this->tmspeed->tooltip = "Leftclick to start waiting for a MIDI command that will set the play speed factor for this preset. ";
	this->tmspeedzero = new Box;
	this->tmspeedzero->vtxcoords->x1 = -0.775f;
	this->tmspeedzero->vtxcoords->y1 = -0.1f;
	this->tmspeedzero->vtxcoords->w = 0.15f;
	this->tmspeedzero->vtxcoords->h = 0.2f;
	this->tmspeedzero->tooltiptitle = "Set MIDI for setting video play speed to one";
	this->tmspeedzero->tooltip = "Leftclick to start waiting for a MIDI command that will set the play speed factor back to one for this preset. ";
	this->tmopacity = new Box;
	this->tmopacity->vtxcoords->x1 = 0.6f;
	this->tmopacity->vtxcoords->y1 = -0.5f;
	this->tmopacity->vtxcoords->w = 0.2f;
	this->tmopacity->vtxcoords->h = 1.0f;
	this->tmopacity->tooltiptitle = "Set MIDI for setting video opacity ";
	this->tmopacity->tooltip = "Leftclick to start waiting for a MIDI command that will set the opacity for this preset. ";
	this->tmfreeze = new Box;
	this->tmfreeze->vtxcoords->x1 = -0.1f;
	this->tmfreeze->vtxcoords->y1 = 0.1f;
	this->tmfreeze->vtxcoords->w = 0.2f;
	this->tmfreeze->vtxcoords->h = 0.2f;
	this->tmfreeze->tooltiptitle = "Set MIDI for scratch wheel freeze ";
	this->tmfreeze->tooltip = "Leftclick to start waiting for a MIDI command that will trigger scratch wheel freeze for this preset. ";
	this->tmscratch = new Box;
	this->tmscratch->vtxcoords->x1 = -1.0f;
	this->tmscratch->vtxcoords->y1 = -1.0f;
	this->tmscratch->vtxcoords->w = 2.0f;
	this->tmscratch->vtxcoords->h = 2.0f;
	this->tmscratch->tooltiptitle = "Set MIDI for scratch wheel ";
	this->tmscratch->tooltip = "Leftclick to start waiting for a MIDI command that will trigger the scratch wheel  for this preset. ";
	
	this->wormgate1 = new Button(false);
	this->wormgate1->toggle = 1;
	this->wormgate1->box->vtxcoords->x1 = -1.0f;
	this->wormgate1->box->vtxcoords->y1 = -0.58f;
	this->wormgate1->box->vtxcoords->w = 0.0375f;
	this->wormgate1->box->vtxcoords->h = 0.6f;
	this->wormgate1->box->upvtxtoscr();
	this->wormgate1->box->tooltiptitle = "Screen switching wormgate ";
	this->wormgate1->box->tooltip = "Connects mixing screen and media bins screen.  Leftclick to switch screen.  Drag content inside white rectangle up to the very edge of the screen to travel to the other screen. ";
	this->buttons.push_back(this->wormgate1);
	this->wormgate2 = new Button(false);
	this->wormgate2->toggle = 1;
	this->wormgate2->box->vtxcoords->x1 = 1.0f - 0.0375f;
	this->wormgate2->box->vtxcoords->y1 = -0.58f;
	this->wormgate2->box->vtxcoords->w = 0.0375f;
	this->wormgate2->box->vtxcoords->h = 0.6f;
	this->wormgate2->box->upvtxtoscr();
	this->wormgate2->box->tooltiptitle = "Screen switching wormgate ";
	this->wormgate2->box->tooltip = "Connects mixing screen and media bins screen.  Leftclick to switch screen.  Leftclick to switch screen.  Drag content inside white rectangle up to the very edge of the screen to travel to the other screen. ";
	this->buttons.push_back(this->wormgate2);
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
	menu->box->scrcoords->y1 = mainprogram->yvtxtoscr(0.075f);
	menu->box->scrcoords->w = mainprogram->xvtxtoscr(0.234f);
	menu->box->scrcoords->h = mainprogram->yvtxtoscr(0.075f);
	menu->box->upscrtovtx();
}

#ifdef POSIX
const char* Program::mime_to_tinyfds(std::string filters) {
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
	//SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN);
	boost::replace_all(defaultdir, "/", "\\");
	boost::filesystem::path p(defaultdir);
	std::string name;
	if (!boost::filesystem::is_directory(p)) {
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

void Program::get_inname(const char *title, std::string filters, std::string defaultdir) {
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
	bool as = this->autosave;
	this->autosave = false;
	//outpaths = tinyfd_openFileDialog(title, dd, 0, nullptr, nullptr, 1);
	#ifdef WINDOWS
	LPCSTR lpcfilters = this->mime_to_wildcard(filters);
	this->win_dialog(title, lpcfilters, defaultdir, true, true);
	#endif
	#ifdef POSIX
    const char *outpaths;
    char const* const dd = (defaultdir == "") ? "" : defaultdir.c_str();
    mainprogram->blocking = true;
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
	#endif
	if (mainprogram->paths.size()) {
		this->path = (char*)"ENTER";
		this->counting = 0;
	}
	this->autosave = as;
}

void Program::get_dir(const char* title, std::string defaultdir) {
	bool as = mainprogram->autosave;
	mainprogram->autosave = false;

	//boost::replace_all(defaultdir, "/", "\\");
	//LPITEMIDLIST pidl;
	//LPSHELLFOLDER pDesktopFolder; 
	//char szPath[MAX_PATH];
	//OLECHAR olePath[MAX_PATH];
	//HRESULT hr;
	//strncpy(szPath, defaultdir.c_str(), MAX_PATH);
	//SHGetDesktopFolder(&pDesktopFolder);
	//MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szPath, -1, olePath, MAX_PATH);
	//hr = pDesktopFolder->ParseDisplayName(NULL, NULL, olePath, nullptr, &pidl, nullptr);

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
	bInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI | BIF_BROWSEINCLUDEFILES;

    mainprogram->blocking = true;
	LPITEMIDLIST lpItem = SHBrowseForFolder(&bInfo);
    mainprogram->blocking = false;
	if (lpItem != NULL)
	{
		SHGetPathFromIDList(lpItem, szDir);
		CoTaskMemFree(lpItem);
	}
	this->path = (char*)szDir;
	OleUninitialize();
#endif
#ifdef POSIX
    char const* const dd = (defaultdir == "") ? "" : defaultdir.c_str();
    const char *dir;
    mainprogram->blocking = true;
	dir = tinyfd_selectFolderDialog(title, dd);
    mainprogram->blocking = false;
	if (dir) this->path = dir;
	#endif
	mainprogram->autosave = as;
}

bool Program::order_paths(bool dodeckmix) {
	if (mainprogram->multistage == 0) {
		mainprogram->filescount = 0;
		mainprogram->multistage = 1;
	}
	if (mainprogram->multistage == 1) {
		mainprogram->orderondisplay = true;
		// first get one file texture per loop
		std::string str = mainprogram->paths[mainprogram->filescount];
		mainprogram->filescount++;

		GLuint tex;
        // determine file type
        std::string istring = "";
        std::string result = deconcat_files(str);
        if (!mainprogram->openerr) {
            bool concat = (result != "");
            std::ifstream rfile;
            if (concat) rfile.open(result);
            else rfile.open(str);
            safegetline(rfile, istring);
        }
        else mainprogram->openerr = false;
        if (istring == "EWOCvj LAYERFILE") {
            tex = get_layertex(str);
        } else if (dodeckmix && istring == "EWOCvj DECKFILE") {
            tex = get_deckmixtex(str);
        } else if (dodeckmix && istring == "EWOCvj MIXFILE") {
            tex = get_deckmixtex(str);
        } else if (isimage(str)) {
            tex = get_imagetex(str);
        } else if (isvideo(str)) {
            tex = get_videotex(str);
        } else {
            mainprogram->orderondisplay = false;
            mainprogram->paths.erase(mainprogram->paths.begin() + mainprogram->filescount);
            mainprogram->filescount -= 2;
            if (mainprogram->filescount < 0) mainprogram->filescount = 0;
            return false;
        }

		mainprogram->pathtexes.push_back(tex);
		render_text(str, white, 2.0f, 2.0f, 0.00045f, 0.00075f);
		if (mainprogram->filescount < mainprogram->paths.size()) return false;
		for (int j = 0; j < mainprogram->paths.size(); j++) {
			mainprogram->pathboxes.push_back(new Box);
			mainprogram->pathboxes[j]->vtxcoords->x1 = -0.4f;
			mainprogram->pathboxes[j]->vtxcoords->y1 = 0.8f - j * 0.1f;
			mainprogram->pathboxes[j]->vtxcoords->w = 0.8f;
			mainprogram->pathboxes[j]->vtxcoords->h = 0.1f;
			mainprogram->pathboxes[j]->upvtxtoscr();
			mainprogram->pathboxes[j]->tooltiptitle = "Order files to be opened ";
			mainprogram->pathboxes[j]->tooltip = "Leftmouse drag the files in the list to set the order in which they will be loaded.  Use arrows/mousewheel to scroll list when its bigger then the screen.  Click APPLY ORDER to continue. ";
		}
		mainprogram->multistage = 2;
	}
	if (mainprogram->multistage == 2) {
		// then do interactive ordering
		if (mainprogram->paths.size() != 1) {
			bool cont = mainprogram->do_order_paths();
			if (!cont) return false;
		}
		mainprogram->multistage = 3;
	}
	if (mainprogram->multistage == 3) {
		if (mainprogram->openfilesshelf) {
			// special case: reuse pathtexes as shelfelement texes
			for (int i = 0; i < mainprogram->paths.size(); i++) {
				ShelfElement* elem = mainmix->mouseshelf->elements[i + mainprogram->shelffileselem];
				elem->path = mainprogram->paths[i];
				elem->tex = mainprogram->pathtexes[i];
			}
			mainprogram->pathtexes.clear();
		}
		// then cleanup
		for (int j = 0; j < mainprogram->pathboxes.size(); j++) {
			delete mainprogram->pathboxes[j];
		}
		for (int j = 0; j < mainprogram->pathtexes.size(); j++) {
			glDeleteTextures(1, &mainprogram->pathtexes[j]);
		}
		mainprogram->pathboxes.clear();
		mainprogram->pathtexes.clear();
		mainprogram->filescount = 0;
		mainprogram->orderondisplay = false;
		mainprogram->multistage = 4;
	}

	return true;
}

	
bool Program::do_order_paths() {
	if (this->paths.size() == 1) return true;
	// show interactive list with draggable elements to allow element ordering of mainprogram->paths, result of get_multinname
	int limit = this->paths.size();
	if (limit > 17) limit = 17;

	// mousewheel scroll
	this->pathscroll -= mainprogram->mousewheel;
	if (this->dragstr != "" && mainmix->time - mainmix->oldtime> 0.1f) {
		// border scroll when dragging
		if (mainprogram->my < yvtxtoscr(0.1f)) this->pathscroll--;
		if (mainprogram->my > yvtxtoscr(1.9f)) this->pathscroll++;
	}
	if (this->pathscroll < 0) this->pathscroll = 0;
	if (this->paths.size() > 18 && this->paths.size() - this->pathscroll < 18) this->pathscroll = this->paths.size() - 17;

	mainprogram->frontbatch = true;
	// draw and handle orderlist scrollboxes
	this->pathscroll = mainprogram->handle_scrollboxes(this->orderscrollup, this->orderscrolldown, this->paths.size(), this->pathscroll, 17);
	bool indragbox = false;
	for (int j = this->pathscroll; j < this->pathscroll + limit; j++) {
		Box* box = this->pathboxes[j];
		box->vtxcoords->y1 = 0.8f - (j - this->pathscroll) * 0.1f;
		box->upvtxtoscr();
		draw_box(white, black, box, -1);
		draw_box(white, black, 0.3f, box->vtxcoords->y1, 0.1f, 0.1f, this->pathtexes[j]);
		render_text(this->paths[j], white, -0.4f + 0.015f, box->vtxcoords->y1 + 0.075f - 0.045f, 0.00045f, 0.00075f);
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
	Box applybox;
	applybox.vtxcoords->x1 = this->pathboxes.back()->vtxcoords->x1;
	applybox.vtxcoords->y1 = 0.8f - limit * 0.1f;
	applybox.vtxcoords->w = this->pathboxes.back()->vtxcoords->w;
	applybox.vtxcoords->h = this->pathboxes.back()->vtxcoords->h;
	applybox.upvtxtoscr();
	draw_box(white, black, &applybox, -1);
	render_text("APPLY ORDER", white, -0.4f + 0.015f, 0.8f - limit * 0.1f + 0.075f - 0.045f, 0.00045f, 0.00075f);
	if (applybox.in2() && this->dragstr == "") {
		if (mainprogram->orderleftmouse) {
		    this->pathscroll = 0;
		    return true;
		}
	}

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
				draw_box(white, black, this->pathboxes[this->dragpathpos]->vtxcoords->x1, 1.0f - mainprogram->yscrtovtx(under1), this->pathboxes[this->dragpathpos]->vtxcoords->w, this->pathboxes[this->dragpathpos]->vtxcoords->h, -1);
				draw_box(white, black, 0.3f, 1.0f - mainprogram->yscrtovtx(under1), 0.1f, 0.1f, this->pathtexes[this->dragpathpos]);
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
		if (mainprogram->rightmouse) {
			// cancel element drag
			this->dragstr = "";
			mainprogram->rightmouse = false;
			mainprogram->menuactivation = false;
			for (int i = 0; i < mainprogram->menulist.size(); i++) {
				mainprogram->menulist[i]->state = 0;
			}
		}
	}

	mainprogram->frontbatch = false;

	return false;

}


void Program::handle_wormgate(bool gate) {
	Box* box;
	if (gate == 0) box = mainprogram->wormgate1->box;
	else box = mainprogram->wormgate2->box;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDrawBuffer(GL_BACK_LEFT);
    mainprogram->directmode = true;
	if (gate == 0) {
		register_triangle_draw(lightgrey, lightgrey, -1.0 + box->vtxcoords->w, box->vtxcoords->y1 + box->vtxcoords->h / 4.0f - 0.15f, box->vtxcoords->h / 4.0f, box->vtxcoords->h / 2.0f, LEFT, OPEN);
		GUI_Element* elem = mainprogram->guielems.back();
		mainprogram->guielems.pop_back();
		draw_triangle(elem->triangle);
		delete elem->triangle;
		delete elem;
		if (mainprogram->binsscreen) render_text("MIX", lightgrey, -0.9f, -0.29f, 0.0006f, 0.001f);
		else render_text("BINS", lightgrey, -0.9f, -0.44f, 0.0006f, 0.001f);
	}
	else {
		if (mainprogram->binsscreen) {
			register_triangle_draw(lightgrey, lightgrey, 1.0f - box->vtxcoords->w - box->vtxcoords->h / 4.0f * 0.866f, box->vtxcoords->y1 + box->vtxcoords->h / 4.0f + 0.15f, box->vtxcoords->h / 4.0f, box->vtxcoords->h / 2.0f, RIGHT, OPEN);
			GUI_Element* elem = mainprogram->guielems.back();
			mainprogram->guielems.pop_back();
			draw_triangle(elem->triangle);
            delete elem->triangle;
            delete elem;
			render_text("MIX", lightgrey, 0.86f, -0.14f, 0.0006f, 0.001f);
		}
		else {
			register_triangle_draw(lightgrey, lightgrey, 1.0f - box->vtxcoords->w - box->vtxcoords->h / 4.0f * 0.866f, box->vtxcoords->y1 + box->vtxcoords->h / 4.0f - 0.15f, box->vtxcoords->h / 4.0f, box->vtxcoords->h / 2.0f, RIGHT, OPEN);
			GUI_Element* elem = mainprogram->guielems.back();
			mainprogram->guielems.pop_back();
			draw_triangle(elem->triangle);
            delete elem->triangle;
            delete elem;
			render_text("BINS", lightgrey, 0.86f, -0.44f, 0.0006f, 0.001f);
		}
	}
    mainprogram->directmode = false;

	if (mainprogram->binsscreen) {
		box->vtxcoords->y1 = -1.0f;
		box->vtxcoords->h = 2.0f;
		box->upvtxtoscr();
	}

	//draw and handle BINS wormgate
	if (mainprogram->fullscreen == -1) {
		if (box->in()) {
			draw_box(lightgrey, lightblue, box, -1);
			mainprogram->tooltipbox = mainprogram->wormgate1->box;
			if (!mainprogram->menuondisplay) {
				if (mainprogram->leftmouse) {
					mainprogram->binsscreen = !mainprogram->binsscreen;
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
					if (mainprogram->mx == gate * (glob->w - 1)) {
						if (!mainprogram->binsscreen) {
							set_queueing(false);
						}
						mainprogram->binsscreen = !mainprogram->binsscreen;
						mainprogram->inwormgate = true;
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
		for (int j = 0; j < this->nodesmain->mixnodes.size(); j++) {
			Node* node = this->nodesmain->mixnodes[j];
			node->renew_texes(this->ow3, this->oh3);
		}
		for (int j = 0; j < this->nodesmain->mixnodescomp.size(); j++) {
			Node* node = this->nodesmain->mixnodescomp[j];
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

		this->oldow = this->ow;
		this->oldoh = this->oh;
	}
}


void Program::handle_fullscreen() {
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

	MixNode* node;
	if (this->fullscreen == this->nodesmain->mixnodes.size()) node = (MixNode*)this->nodesmain->mixnodescomp[this->fullscreen - 1];
	else node = (MixNode*)this->nodesmain->mixnodes[this->fullscreen];
	if (this->prevmodus == false) node = (MixNode*)this->nodesmain->mixnodescomp[this->fullscreen];
	GLfloat cf = glGetUniformLocation(this->ShaderProgram, "cf");
	GLint wipe = glGetUniformLocation(this->ShaderProgram, "wipe");
	GLint mixmode = glGetUniformLocation(this->ShaderProgram, "mixmode");
	if (mainmix->wipe[this->fullscreen == 2] > -1) {
		if (this->fullscreen == 2) glUniform1f(cf, mainmix->crossfadecomp->value);
		else glUniform1f(cf, mainmix->crossfade->value);
		glUniform1i(wipe, 1);
		glUniform1i(mixmode, 18);
	}
	GLint down = glGetUniformLocation(this->ShaderProgram, "down");
	glUniform1i(down, 1);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, node->mixtex);
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	mainprogram->directmode = true;
	glUniform1i(down, 0);
	glUniform1i(wipe, 0);
	glUniform1i(mixmode, 0);
	if (this->doubleleftmouse) {
        this->fullscreen = -1;
        mainprogram->directmode = false;
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
		mainprogram->directmode = false;
	}
	if (this->menuchosen) {
		this->menuchosen = false;
		this->menuactivation = 0;
		this->menuresults.clear();
	}

    glDeleteBuffers(1, &vbuf);
    glDeleteBuffers(1, &tbuf);
    glDeleteVertexArrays(1, &vao);
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



int Program::handle_scrollboxes(Box* upperbox, Box* lowerbox, int numlines, int scrollpos, int scrlines) {
    int ret = this->handle_scrollboxes(upperbox, lowerbox, numlines, scrollpos, scrlines, mainprogram->mx,
                                    mainprogram->my);
    return ret;
}

int Program::handle_scrollboxes(Box* upperbox, Box* lowerbox, int numlines, int scrollpos, int scrlines, int mx, int
my) {
	// general code for scrollbuttons of scrollable lists
	if (scrollpos > 0) {
		if (upperbox->in(mx, my)) {
			// scroll up
			upperbox->acolor[0] = 0.5f;
			upperbox->acolor[1] = 0.5f;
			upperbox->acolor[2] = 1.0f;
			upperbox->acolor[3] = 1.0f;
			if (mainprogram->leftmouse || mainprogram->orderleftmouse) {
				scrollpos--;
			}
		}
		else {
			upperbox->acolor[0] = 0.0f;
			upperbox->acolor[1] = 0.0f;
			upperbox->acolor[2] = 0.0f;
			upperbox->acolor[3] = 1.0f;
		}
		draw_box(upperbox, -1);
        if (!mainprogram->insmall) {
            register_triangle_draw(white, white, upperbox->vtxcoords->x1 + (lowerbox->vtxcoords->w / 0.075f) * 0.0111f, upperbox->vtxcoords->y1 + (lowerbox->vtxcoords->w / 0.075f) * (0.0624f - 0.045f), 0.0165f, 0.0312f, DOWN, CLOSED);
        }
        else {
            register_triangle_draw(white, white, upperbox->vtxcoords->x1 + (lowerbox->vtxcoords->w / 0.075f) * 0.0111f, upperbox->vtxcoords->y1 + (lowerbox->vtxcoords->w / 0.075f) * (0.0624f - 0.045f),
            0.033f, 0.0624f, DOWN, CLOSED);
            GUI_Element *elem = mainprogram->guielems.back();
            mainprogram->guielems.pop_back();
            draw_triangle(elem->triangle);
            delete elem->triangle;
            delete elem;
        }
	}
	if (numlines - scrollpos > scrlines) {
        if (lowerbox->in(mx, my)) {
			// scroll down
			lowerbox->acolor[0] = 0.5f;
			lowerbox->acolor[1] = 0.5f;
			lowerbox->acolor[2] = 1.0f;
			lowerbox->acolor[3] = 1.0f;
			if (mainprogram->leftmouse || mainprogram->orderleftmouse) {
				scrollpos++;
			}
		}
		else {
			lowerbox->acolor[0] = 0.0f;
			lowerbox->acolor[1] = 0.0f;
			lowerbox->acolor[2] = 0.0f;
			lowerbox->acolor[3] = 1.0f;
		}
		draw_box(lowerbox, -1);
        if (!mainprogram->insmall) {
		register_triangle_draw(white, white, lowerbox->vtxcoords->x1 + (lowerbox->vtxcoords->w / 0.075f) * 0.0111f, lowerbox->vtxcoords->y1 + (lowerbox->vtxcoords->w / 0.075f) * (0.0624f - 0.045f), 0.0165f, 0.0312f, UP, CLOSED);
        }
        else {
            register_triangle_draw(white, white, lowerbox->vtxcoords->x1 + (lowerbox->vtxcoords->w / 0.075f) * 0.0111f, lowerbox->vtxcoords->y1 + (lowerbox->vtxcoords->w / 0.075f) * (0.0624f - 0.045f),
         0.033f, 0.0624f, UP, CLOSED);
            GUI_Element *elem = mainprogram->guielems.back();
            mainprogram->guielems.pop_back();
            draw_triangle(elem->triangle);
            delete elem->triangle;
            delete elem;
        }
	}
	return scrollpos;
}

void Program::layerstack_scrollbar_handle() {
	//Handle layer scrollbar
	bool inbox = false;
	bool comp = !mainprogram->prevmodus;
	for (int i = 0; i < 2; i++) {
		std::vector<Layer*>& lvec = choose_layers(i);
		Box box;
		Box boxbefore;
		Box boxafter;
		Box boxlayer;
		boxlayer.vtxcoords->y1 = 1.0f - mainprogram->layh - 0.045f;
		boxlayer.vtxcoords->w = 0.031f;
		boxlayer.vtxcoords->h = 0.04f;
		int size = lvec.size() + 1;
		if (size < 3) size = 3;
		float slidex = 0.0f;
		if (mainmix->scrollon == i + 1) {
			slidex = (mainmix->scrollon != 0) * mainprogram->xscrtovtx(mainprogram->mx - mainmix->scrollmx);
		}
		box.vtxcoords->x1 = -1.0f + mainprogram->numw + i + mainmix->scenes[comp][i][mainmix->currscene[comp][i]]->scrollpos * (mainprogram->layw * 3.0f / size) + slidex;
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
		boxbefore.vtxcoords->w = (3.0f / size) * mainprogram->layw * mainmix->scenes[comp][i][mainmix->currscene[comp][i]]->scrollpos;
		boxbefore.vtxcoords->h = 0.05f;
		boxbefore.upvtxtoscr();
		boxafter.vtxcoords->x1 = -1.0f + mainprogram->numw + i + (mainmix->scenes[comp][i][mainmix->currscene[comp][i]]->scrollpos + 3) * (mainprogram->layw * 3.0f / size);
		boxafter.vtxcoords->y1 = 1.0f - mainprogram->layh - 0.05f;
		boxafter.vtxcoords->w = (3.0f / size) * mainprogram->layw * (size - 3);
		boxafter.vtxcoords->h = 0.05f;
		boxafter.upvtxtoscr();
		bool inbox = false;
		if (box.in2()) {
			inbox = true;
			if (mainprogram->leftmousedown && mainmix->scrollon == 0 && !mainprogram->menuondisplay) {
				mainmix->scrollon = i + 1;
				mainmix->scrollmx = mainprogram->mx;
				mainprogram->leftmousedown = false;
			}
		}
		box.vtxcoords->w = 3.0f * mainprogram->layw / size;
		if (mainmix->scrollon == i + 1) {
			if (mainprogram->lmover) {
				mainmix->scrollon = 0;
			}
			else {
				if ((mainprogram->mx - mainmix->scrollmx) > mainprogram->xvtxtoscr(box.vtxcoords->w)) {
					if (mainmix->scenes[comp][i][mainmix->currscene[comp][i]]->scrollpos < size - 3) {
						mainmix->scenes[comp][i][mainmix->currscene[comp][i]]->scrollpos++;
						mainmix->scrollmx += mainprogram->xvtxtoscr(box.vtxcoords->w);
					}
				}
				else if ((mainprogram->mx - mainmix->scrollmx) < -mainprogram->xvtxtoscr(box.vtxcoords->w)) {
					if (mainmix->scenes[comp][i][mainmix->currscene[comp][i]]->scrollpos > 0) {
						mainmix->scenes[comp][i][mainmix->currscene[comp][i]]->scrollpos--;
						mainmix->scrollmx -= mainprogram->xvtxtoscr(box.vtxcoords->w);
					}
				}
			}
		}
		box.vtxcoords->x1 = -1.0f + mainprogram->numw + i + slidex;
		float remw = box.vtxcoords->w;
		for (int j = 0; j < size; j++) {
			//draw scrollbar numbers+divisions+blocks
			if (j == 0) {
				if (slidex < 0.0f) box.vtxcoords->x1 -= slidex;
			}
			if (j == size - 1) {
				if (slidex > 0.0f) box.vtxcoords->w -= slidex;
			}
			if (j >= mainmix->scenes[comp][i][mainmix->currscene[comp][i]]->scrollpos && j < mainmix->scenes[comp][i][mainmix->currscene[comp][i]]->scrollpos + 3) {
				if (inbox) draw_box(lightgrey, lightblue, &box, -1);
				else draw_box(lightgrey, grey, &box, -1);
			}
			else draw_box(lightgrey, black, &box, -1);
			// boxlayer: small coloured box (default grey) signalling there's a loopstation parameter in this layer
			boxlayer.vtxcoords->x1 = box.vtxcoords->x1 + 0.031f;
			if (j < lvec.size()) {
				if (j >= mainmix->scenes[comp][i][mainmix->currscene[comp][i]]->scrollpos && j < mainmix->scenes[comp][i][mainmix->currscene[comp][i]]->scrollpos + 3) {
					draw_box(nullptr, lvec[j]->scrollcol, &boxlayer, -1);
				}
			}
 
			if (j == 0) {
				if (slidex < 0.0f) box.vtxcoords->x1 += slidex;
			}
			render_text(std::to_string(j + 1), white, box.vtxcoords->x1 + 0.0078f - slidex, box.vtxcoords->y1 + 0.0078f, 0.0006, 0.001);
			const int s = lvec.size() - mainmix->scenes[comp][i][mainmix->currscene[comp][i]]->scrollpos;
			if (mainprogram->dragbinel) {
				if (box.in2()) {
					if (mainprogram->lmover) {
						if (j < lvec.size()) {
							mainprogram->draginscrollbarlay = lvec[j];
						}
						else {
							mainprogram->draginscrollbarlay = mainmix->add_layer(lvec, j);
						}
					}
				}
			}
			box.vtxcoords->x1 += box.vtxcoords->w;
			box.upvtxtoscr();
		}
		if (boxbefore.in2()) {
			//mouse in empty scrollbar part before the lightgrey visible layers part
			if (mainprogram->dragbinel) {
				if (mainmix->scrolltime == 0.0f) {
					mainmix->scrolltime = mainmix->time;
				}
				else {
					if (mainmix->time - mainmix->scrolltime > 1.0f) {
						mainmix->scenes[comp][i][mainmix->currscene[comp][i]]->scrollpos--;
						mainmix->scrolltime = mainmix->time;
					}
				}
			}
			if (mainprogram->leftmouse) {
				mainmix->scenes[comp][i][mainmix->currscene[comp][i]]->scrollpos--;
			}
		}
		else if (boxafter.in2()) {
			//mouse in empty scrollbar part after the lightgrey visible layers part
			if (mainprogram->dragbinel) {
				if (mainmix->scrolltime == 0.0f) {
					mainmix->scrolltime = mainmix->time;
				}
				else {
					if (mainmix->time - mainmix->scrolltime > 1.0f) {
						mainmix->scenes[comp][i][mainmix->currscene[comp][i]]->scrollpos++;
						mainmix->scrolltime = mainmix->time;
					}
				}
			}
			if (mainprogram->leftmouse) {
				mainmix->scenes[comp][i][mainmix->currscene[comp][i]]->scrollpos++;
			}
		}
		//trigger scrollbar tooltips
		mainprogram->scrollboxes[i]->in();

		if (!inbox) mainmix->scrolltime = 0.0f;
	}
}


int Program::quit_requester() {
	int mx = -1;
	int my = -1;
	if (SDL_GetMouseFocus() == mainprogram->prefwindow) {
		//SDL_PumpEvents();
		SDL_GetMouseState(&mx, &my);
		mx *= 2.0f;
		my *= 2.0f;
	}

	mainprogram->insmall = true;
	mainprogram->bvao = mainprogram->prboxvao;
	mainprogram->bvbuf = mainprogram->prboxvbuf;
	mainprogram->btbuf = mainprogram->prboxtbuf;
	glClear(GL_COLOR_BUFFER_BIT);

	int ret = 0;

	render_text("Are you sure about quitting the program?", white, -0.5f, 0.2f, 0.0024f, 0.004f, 1, 0);
	for (int j = 0; j < 12; j++) {
		for (int i = 0; i < 12; i++) {
			Box* box = binsmain->elemboxes[i * 12 + j];
			box->upvtxtoscr();
			BinElement* binel = binsmain->currbin->elements[i * 12 + j];
			if (binel->encoding) {
				render_text("!!! There are still videos being encoded to HAP !!!", red, -0.5f, 0.0f, 0.0024f, 0.004f, 1, 0);
				break;
			}
		}
	}

	Box box;
	box.vtxcoords->x1 = 0.15f;
	box.vtxcoords->y1 = -1.0f;
	box.vtxcoords->w = 0.3f;
	box.vtxcoords->h = 0.2f;
	box.upvtxtoscr();
	draw_box(white, black, &box, -1);
	if (box.in2(mx, my)) {
		draw_box(white, lightblue, &box, -1);
		if (mainprogram->leftmouse || mainprogram->orderleftmouse) {
			mainprogram->project->do_save(mainprogram->project->path);
			ret = 1;
		}
	}
	render_text("SAVE PROJECT", white, box.vtxcoords->x1 + 0.02f, box.vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, 1, 0);

	box.vtxcoords->x1 = 0.45f;
	box.upvtxtoscr();
	draw_box(white, black, &box, -1);
	if (box.in2(mx, my)) {
		draw_box(white, lightblue, &box, -1);
		if (mainprogram->leftmouse || mainprogram->orderleftmouse) {
			ret = 2;
		}
	}
	render_text("QUIT", white, box.vtxcoords->x1 + 0.02f, box.vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, 1, 0);

	box.vtxcoords->x1 = 0.75f;
	box.upvtxtoscr();
	draw_box(white, black, &box, -1);
	if (box.in2(mx, my)) {
		draw_box(white, lightblue, &box, -1);
		if (mainprogram->leftmouse || mainprogram->orderleftmouse) {
			ret = 3;
		}
	}
	render_text("CANCEL", white, box.vtxcoords->x1 + 0.02f, box.vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, 1, 0);

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


void Program::shelf_miditriggering() {
	// do midi trigger from shelf, set up in callback, executed here at a fixed the_loop position
	ShelfElement* elem = mainprogram->midishelfelem;
	if (elem) {
	    std::vector<Layer*> clays = mainmix->currlays[!mainprogram->prevmodus];
	    for (int k = 0; k < clays.size(); k++) {
	        Layer *lay = clays[k];
            clays[k]->deautomate();
            if (elem->type == ELEM_FILE) {
                clays[k] = clays[k]->open_video(0, elem->path, true);
            } else if (elem->type == ELEM_IMAGE) {
                clays[k]->open_image(elem->path);
                clays[k] = (*(clays[k]->layers))[clays[k]->pos];
                clays[k]->frame = 0.0f;
            } else if (elem->type == ELEM_LAYER) {
                mainmix->open_layerfile(elem->path, clays[k], true, false);
                clays[k] = (*(clays[k]->layers))[clays[k]->pos];
                clays[k]->frame = 0.0f;
            } else if (elem->type == ELEM_DECK) {
                mainmix->mousedeck = clays[k]->deck;
                mainmix->open_deck(elem->path, true);
            } else if (elem->type == ELEM_MIX) {
                mainmix->open_mix(elem->path, true);
            }
            // setup framecounters for layers something was previously dragged into
            // when something new is dragged into layer, set frames from
            if (elem->launchtype == 1) {
                if (elem->cframes.size()) {
                    if (elem->type == ELEM_DECK) {
                        std::vector<Layer *> &lvec = choose_layers(clays[k]->deck);
                        for (int i = 0; i < lvec.size(); i++) {
                            lvec[i]->frame = elem->cframes[i];
                            lvec[i]->prevshelfdragelem = mainprogram->midishelfelem;
                        }
                    } else if (elem->type == ELEM_MIX) {
                        for (int m = 0; m < 2; m++) {
                            std::vector<Layer *> &lvec = choose_layers(m);
                            for (int i = 0; i < lvec.size(); i++) {
                                lvec[i]->frame = elem->cframes[i];
                                lvec[i]->prevshelfdragelem = mainprogram->midishelfelem;
                            }
                        }
                    } else {
                        clays[k]->frame = elem->cframes[0];
                    }
                    elem->cframes.clear();
                }
            } else if (elem->launchtype == 2) {
                if (elem->nblayers.size()) {
                    if (elem->type == ELEM_DECK) {
                        std::vector<Layer *> &lvec = choose_layers(clays[k]->deck);
                        for (int i = 0; i < lvec.size(); i++) {
                            lvec[i]->frame = elem->nblayers[i]->frame;
                            lvec[i]->prevshelfdragelem = mainprogram->midishelfelem;
                        }
                    } else if (elem->type == ELEM_MIX) {
                        int count = 0;
                        for (int m = 0; m < 2; m++) {
                            std::vector<Layer *> &lvec = choose_layers(m);
                            for (int i = 0; i < lvec.size(); i++) {
                                lvec[i]->frame = elem->nblayers[count]->frame;
                                lvec[i]->prevshelfdragelem = mainprogram->midishelfelem;
                                count++;
                            }
                        }
                    } else {
                        clays[k]->frame = elem->nblayers[0]->frame;
                    }
                    elem->nblayers.clear();
                }
            }
            if (elem->type == ELEM_DECK) {
                std::vector<Layer *> &lvec = choose_layers(clays[k]->deck);
                for (int i = 0; i < lvec.size(); i++) {
                    lvec[i]->prevshelfdragelem = mainprogram->midishelfelem;
                }
            } else if (elem->type == ELEM_MIX) {
                for (int m = 0; m < 2; m++) {
                    std::vector<Layer *> &lvec = choose_layers(m);
                    for (int i = 0; i < lvec.size(); i++) {
                        lvec[i]->prevshelfdragelem = mainprogram->midishelfelem;
                    }
                }
            }

            clays[k]->prevshelfdragelem = mainprogram->midishelfelem;
            mainprogram->midishelfelem = nullptr;
        }
	}
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
    this->deautomate();
}

bool Button::handle(bool circlein) {
    bool ret = this->handle(circlein, true);
    return ret;
}

bool Button::handle(bool circlein, bool automation) {
    bool changed = false;
    if (this->box->in()) {
        if (mainprogram->leftmouse || mainprogram->orderleftmouse) {
            this->oldvalue = this->value;
            this->value++;
            if (this->value > this->toggle) this->value = 0;
            if (this->toggle == 0) this->value = 1;
            if (automation) {
                for (int i = 0; i < loopstation->elems.size(); i++) {
                    if (loopstation->elems[i]->recbut->value) {
                        loopstation->elems[i]->add_button_automationentry(this);
                    }
                }
            }
            changed = true;
        }
        if (mainprogram->menuactivation && !mainprogram->menuondisplay) {
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
    else if (this->value && !circlein) {
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
            draw_box(this->ccol, this->box->vtxcoords->x1 + radx, this->box->vtxcoords->y1 + rady, 0.0225f, 1);
        }
        else draw_box(this->ccol, this->box->vtxcoords->x1 + radx, this->box->vtxcoords->y1 + rady, 0.0225f, 2);
        float x = render_text(this->name[0], white, 0.0f, 0.0f, radx / 50.0f, rady / 50.0f, 0, 0, 0)[0] / 2.0f;
        render_text(this->name[0], white, this->box->vtxcoords->x1 - x + radx / 4.0f, this->box->vtxcoords->y1 - x * rady / radx + rady / 4.0f, radx / 50.0f, rady / 50.0f);
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

void Button::deautomate() {
    if (this) {
        LoopStationElement *elem = loopstation->butelemmap[this];
        if (!elem) return;
        elem->buttons.erase(this);
        if (this->layer) elem->layers.erase(this->layer);
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
        loopstation->butelemmap.erase(this);
        this->box->acolor[0] = 0.2f;
        this->box->acolor[1] = 0.2f;
        this->box->acolor[2] = 0.2f;
        this->box->acolor[3] = 1.0f;
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
    this->upvtxtoscr();
    if (this->scrcoords->x1 <= mx && mx <= this->scrcoords->x1 + this->scrcoords->w) {
        if (this->scrcoords->y1 - this->scrcoords->h <= my && my <= this->scrcoords->y1) {
            mainprogram->boxhit = true;
            if (mainprogram->showtooltips && !mainprogram->ttreserved) {
                if (mainprogram->tooltipbox) delete mainprogram->tooltipbox;
                mainprogram->tooltipbox = this->copy();
                mainprogram->ttreserved = this->reserved;
            }
            return true;
        }
    }
    return false;
}

bool Box::in2() {
    bool ret = this->in2(mainprogram->mx, mainprogram->my);
    return ret;
}

bool Box::in2(int mx, int my) {
    // for boxes in limited scope (non-dynamically allocated)  reminder: new tooltip mechanism makes this redundant
    if (mainprogram->menuondisplay) return false;
    if (this->scrcoords->x1 <= mx && mx <= this->scrcoords->x1 + this->scrcoords->w) {
        if (this->scrcoords->y1 - this->scrcoords->h <= my && my <= this->scrcoords->y1) {
            return true;
        }
    }
    return false;
}

Box* Box::copy() {
    Box *box = new Box;
    box->lcolor[0] = this->lcolor[0];
    box->lcolor[1] = this->lcolor[1];
    box->lcolor[2] = this->lcolor[2];
    box->lcolor[3] = this->lcolor[3];
    box->acolor[0] = this->acolor[0];
    box->acolor[1] = this->acolor[1];
    box->acolor[2] = this->acolor[2];
    box->acolor[3] = this->acolor[3];
    box->vtxcoords = this->vtxcoords;
    box->scrcoords = this->scrcoords;
    box->tex = this->tex;
    box->tooltiptitle = this->tooltiptitle;
    box->tooltip = this->tooltip;
    return box;
}



void output_video(EWindow* mwin) {

	SDL_GL_MakeCurrent(mwin->win, mwin->glc);

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

	MixNode* node;
	while (1) {
		std::unique_lock<std::mutex> lock(mwin->syncmutex);
		mwin->sync.wait(lock, [&] {return mwin->syncnow; });
		mwin->syncnow = false;
		lock.unlock();

		// receive end of thread signal
		if (mwin->closethread) {
			mwin->closethread = false;
			return;
		}

		if (1) node = (MixNode*)mainprogram->nodesmain->mixnodescomp[2];
		else node = (MixNode*)mainprogram->nodesmain->mixnodes[mwin->mixid];

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
    glDeleteBuffers(1, &mwin->vbuf);
    glDeleteBuffers(1, &mwin->tbuf);
    glDeleteVertexArrays(1, &mwin->vao);
}


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
	mainprogram->livedevices.clear();
	std::map<std::string, std::wstring> map;
	boost::filesystem::path dir("/sys/class/video4linux");
	for (boost::filesystem::directory_iterator iter(dir), end; iter != end; ++iter) {
		std::ifstream name;
		name.open(iter->path().string() + "/name");
		std::string istring;
		safegetline(name, istring);
		istring = istring.substr(0, istring.find(":"));
		std::wstring wstr(istring.begin(), istring.end());
		map["/dev/" + basename(iter->path().string())] = wstr;
	}
	std::map<std::string, std::wstring>::iterator it;
	for (it = map.begin(); it != map.end(); it++) {
		struct v4l2_capability cap;
		int fd = open(it->first.c_str(), O_RDONLY);
		ioctl(fd, VIDIOC_QUERYCAP, &cap);
		if (cap.device_caps & V4L2_CAP_VIDEO_CAPTURE) mainprogram->livedevices.push_back(it->second);
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
		if (std::find(mainprogram->actmenulist.begin(), mainprogram->actmenulist.end(), menu) == mainprogram->actmenulist.end()) {
			mainprogram->actmenulist.clear();
		}
		int size = 0;
		for (int k = 0; k < menu->entries.size(); k++) {
			std::size_t sub = menu->entries[k].find("submenu");
			if (sub != 0) size++;
		}
		if (menu->menuy + mainprogram->yvtxtoscr(yshift) > glob->h - size * mainprogram->yvtxtoscr(0.075f)) menu->menuy = glob->h - size * mainprogram->yvtxtoscr(0.075f) + mainprogram->yvtxtoscr(yshift);
		if (size > 21) menu->menuy = mainprogram->yvtxtoscr(mainprogram->layh) - mainprogram->yvtxtoscr(yshift);
		float vmx = (float)menu->menux * 2.0 / glob->w;
		float vmy = (float)menu->menuy * 2.0 / glob->h;
		float lc[] = { 0.0, 0.0, 0.0, 1.0 };
		float ac1[] = { 0.3, 0.3, 0.3, 1.0 };
		float ac2[] = { 0.5, 0.5, 1.0, 1.0 };
		int numsubs = 0;
		float limit = 1.0f;
		int notsubk = 0;
		for (int k = 0; k < menu->entries.size(); k++) {
			float xoff;
			int koff;
			if (notsubk > 20) {
				if (mainprogram->xscrtovtx(menu->menux) > limit) xoff = -0.2925f + xshift;
				else xoff = 0.2925f + xshift;
				koff = menu->entries.size() - 21;
			}
			else {
				xoff = 0.0f + xshift;
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
									xs = xshift - 0.2925f;
								}
								else {
									xs = xshift + 0.2925f;
								}
								if (menu == mainprogram->filemenu && k - numsubs == 2) {
									// special rule: one layer choice less when saving layer
									mainprogram->laylistmenu1->entries.pop_back();
									mainprogram->laylistmenu2->entries.pop_back();
								}
								// start submenu
								int ret = mainprogram->handle_menu(mainprogram->menulist[i], xs, yshift);
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
		if (k == 0) {
			if (mainmix->mouseeffect > -1) mainmix->mouselayer->delete_effect(mainmix->mouseeffect);
		}
		else if (mainmix->insert) {
		    mainmix->mouselayer->add_effect((EFFECT_TYPE)mainprogram->abeffects[k], mainmix->mouseeffect);
		}
		else {
			std::vector<Effect*>& evec = mainmix->mouselayer->choose_effects();
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
	}
}

void Program::handle_loopmenu() {
	int k = -1;
	// Draw and Program::handle mainprogram->loopmenu
	k = mainprogram->handle_menu(mainprogram->loopmenu);
	if (k > -1) {
		if (k == 0) {
		    // set start of playloop to frame position
			mainmix->mouselayer->startframe = mainmix->mouselayer->frame;
			if (mainmix->mouselayer->startframe > mainmix->mouselayer->endframe) {
				mainmix->mouselayer->endframe = mainmix->mouselayer->startframe;
			}
			mainmix->mouselayer->set_clones();
		}
		else if (k == 1) {
            // set end of playloop to frame position
			mainmix->mouselayer->endframe = mainmix->mouselayer->frame;
			if (mainmix->mouselayer->startframe > mainmix->mouselayer->endframe) {
				mainmix->mouselayer->startframe = mainmix->mouselayer->endframe;
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
			mainmix->cbduration = ((float)(mainmix->mouselayer->numf) * mainmix->mouselayer->millif) / (mainmix->mouselayer->speed->value * mainmix->mouselayer->speed->value * fac * fac);
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
			mainmix->mouselayer->speed->value = sqrt(((float)(mainmix->mouselayer->numf) * mainmix->mouselayer->millif) / mainmix->cbduration / (fac * fac));
			mainmix->mouselayer->set_clones();
		}
		else if (k == 4) {
		    // paste playloop duration by changing the duration itself
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
}

void Program::handle_mixtargetmenu() {
	int k = -1;
	// Draw and Program::handle mixtargetmenu
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
	k = mainprogram->handle_menu(mainprogram->mixtargetmenu);
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
				if (k > 1 && k < 2 + currentries.size()) {
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
				// open new window on chosen output screen and start display thread (synced at end of the_loop)
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
                SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
				mwin->win = SDL_CreateWindow(PROGRAM_NAME, rc1.x, rc1.y, mwin->w, mwin->h, SDL_WINDOW_OPENGL | SDL_WINDOW_MAXIMIZED | SDL_WINDOW_RESIZABLE |
					SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_ALLOW_HIGHDPI);
				SDL_RaiseWindow(mainprogram->mainwindow);
				mainprogram->mixwindows.push_back(mwin);
				mwin->glc = SDL_GL_CreateContext(mwin->win);

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
	}
}

void Program::handle_deckmenu() {
	int k = -1;
	// Draw and Program::handle deckmenu
	// reminder: not necessary?
	k = mainprogram->handle_menu(mainprogram->deckmenu);
	if (k == 0) {
		mainprogram->pathto = "OPENDECK";
		std::thread filereq(&Program::get_inname, mainprogram, "Open deck file", "application/ewocvj2-deck", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
		filereq.detach();
	}
	else if (k == 1) {
		mainprogram->pathto = "SAVEDECK";
		std::thread filereq(&Program::get_outname, mainprogram, "Save deck file", "application/ewocvj2-deck", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
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
}

void Program::handle_laymenu1() {
	int k = -1;
	// Draw and Program::handle mainprogram->laymenu1 (with clone layer) and laymenu2 (without)
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

	if (mainprogram->laymenu1->state > 1) {
        if ((mainmix->mouselayer->vidformat == 188 || mainmix->mouselayer->vidformat == 187) ||
        mainmix->mouselayer->filename == "" || mainmix->mouselayer->type == ELEM_IMAGE || mainmix->mouselayer->type
        == ELEM_LIVE) {
            if (mainprogram->laymenu1->entries.back() == "HAP encode on-the-fly") {
                mainprogram->laymenu1->entries.pop_back();
            }
        }
        else {
            if (mainprogram->laymenu1->entries.back() != "HAP encode on-the-fly") {
                mainprogram->laymenu1->entries.push_back("HAP encode on-the-fly");
            }
        }
        k = mainprogram->handle_menu(mainprogram->laymenu1);
	}
	else if (mainprogram->laymenu2->state > 1) {
		k = mainprogram->handle_menu(mainprogram->laymenu2);
		if (k > 10) k++;
	}


    bool cond = mainprogram->laymenu2->state;
	if (k > -1) {
		if (k == 0) {
			if (mainprogram->menuresults.size()) {
				if (mainprogram->menuresults[0] > 0) {
#ifdef WINDOWS
					std::string livename = "video=" + mainprogram->devices[mainprogram->menuresults[0]];
#else
#ifdef POSIX
                    std::string livename;
                    livename = "/dev/video" + std::to_string(mainprogram->menuresults[0] - 1);
#endif
#endif
					mainmix->mouselayer->set_live_base(livename);
				}
			}
		}
        if (k == 1) {
            mainprogram->pathto = "OPENFILESLAYER";
            mainprogram->loadlay = mainmix->mouselayer;
            std::thread filereq(&Program::get_multinname, mainprogram, "Open video/image/layer file", "", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
            filereq.detach();
        }
        if (k == 2) {
            mainprogram->pathto = "OPENFILESQUEUE";
            mainprogram->loadlay = mainmix->mouselayer;
            std::thread filereq(&Program::get_multinname, mainprogram, "Open video/image/layer file", "", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
            filereq.detach();
        }
		if (k == 3 && !cond) {
			mainprogram->pathto = "OPENFILESSTACK";
			mainprogram->loadlay = mainmix->mouselayer;
            mainmix->addlay = false;
			std::thread filereq(&Program::get_multinname, mainprogram, "Open video/image/layer file", "", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
			filereq.detach();
		}
		else if (k == 4 - cond) {
			mainprogram->pathto = "SAVELAYFILE";
			std::thread filereq(&Program::get_outname, mainprogram, "Save layer file", "application/ewocvj2-layer", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
			filereq.detach();
		}
		else if (k == 5 - cond) {
			mainmix->new_file(mainmix->mousedeck, 1);
		}
		else if (k == 6 - cond) {
			mainprogram->pathto = "OPENDECK";
			std::thread filereq(&Program::get_inname, mainprogram, "Open deck file", "application/ewocvj2-deck", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
			filereq.detach();
		}
		else if (k == 7 - cond) {
			mainprogram->pathto = "SAVEDECK";
			std::thread filereq(&Program::get_outname, mainprogram, "Save deck file", "application/ewocvj2-deck", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
			filereq.detach();
		}
		else if (k == 8 - cond) {
			mainmix->new_file(2, 1);
		}
		else if (k == 9 - cond) {
			mainprogram->pathto = "OPENMIX";
			std::thread filereq(&Program::get_inname, mainprogram, "Open mix file", "application/ewocvj2-mix", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
			filereq.detach();
		}
		else if (k == 10 - cond) {
			mainprogram->pathto = "SAVEMIX";
			std::thread filereq(&Program::get_outname, mainprogram, "Open mix file", "application/ewocvj2-mix", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
			filereq.detach();
		}
		else if (k == 11 - cond) {
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
        else if (k == 12 - cond) {
            Layer* duplay = mainmix->mouselayer->clone();
            duplay->isclone = false;
            duplay->isduplay = mainmix->mouselayer;
        }
        else if (k == 13 - cond) {
            Layer* clonelay = mainmix->mouselayer->clone();
            if (mainmix->mouselayer->clonesetnr == -1) {
                std::unordered_set<Layer*>* uset = new std::unordered_set<Layer*>;
                mainmix->clonesets.push_back(uset);
                uset->emplace(mainmix->mouselayer);
                mainmix->mouselayer->clonesetnr = mainmix->clonesets.size() - 1;
            }
            clonelay->clonesetnr = mainmix->mouselayer->clonesetnr;
            mainmix->clonesets[mainmix->mouselayer->clonesetnr]->emplace(clonelay);
        }
		else if (k == 14 - cond) {
			mainmix->mouselayer->shiftx->value = 0.0f;
			mainmix->mouselayer->shifty->value = 0.0f;
		}
		else if (k == 15 - cond) {
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
		else if (k == 16 - cond) {
		    BinElement *binel = new BinElement;
		    binel->bin = nullptr;
            binel->type = ELEM_FILE;
            binel->path = mainmix->mouselayer->filename;
            binel->otflay = mainmix->mouselayer;
            mainmix->mouselayer->hapbinel = binel;
            binsmain->hap_binel(binel, nullptr);
		}
	}

	if (mainprogram->menuchosen) {
		mainprogram->menuchosen = false;
		mainprogram->menuactivation = 0;
		mainprogram->menuresults.clear();
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
                    livename = "/dev/video" + std::to_string(mainprogram->menuresults[0] - 1);
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
			std::thread filereq(&Program::get_multinname, mainprogram, "Open video/image/layer file", "", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
			filereq.detach();
		}
		else if (k == 2) {
			mainmix->new_file(mainmix->mousedeck, 1);
		}
		else if (k == 3) {
			mainprogram->pathto = "OPENDECK";
			std::thread filereq(&Program::get_inname, mainprogram, "Open deck file", "application/ewocvj2-deck", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
			filereq.detach();
		}
		else if (k == 4) {
			mainprogram->pathto = "SAVEDECK";
			std::thread filereq(&Program::get_outname, mainprogram, "Save deck file", "application/ewocvj2-deck", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
			filereq.detach();
		}
		else if (k == 5) {
			mainmix->new_file(2, 1);
		}
		else if (k == 6) {
			mainprogram->pathto = "OPENMIX";
			std::thread filereq(&Program::get_inname, mainprogram, "Open mix file", "application/ewocvj2-mix", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
			filereq.detach();
		}
		else if (k == 7) {
			mainprogram->pathto = "SAVEMIX";
			std::thread filereq(&Program::get_outname, mainprogram, "Save mix file", "application/ewocvj2-mix", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
			filereq.detach();
		}
	}


	if (mainprogram->menuchosen) {
		mainprogram->menuchosen = false;
		mainprogram->menuactivation = 0;
		mainprogram->menuresults.clear();
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
			mainprogram->pathto = "CLIPOPENFILES";
			std::thread filereq(&Program::get_multinname, mainprogram, "Open clip video file", "", boost::filesystem::canonical(mainprogram->currclipfilesdir).generic_string());
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
}

void Program::handle_mainmenu() {
	int k = -1;
	// Draw and Program::handle mainmenu
	k = mainprogram->handle_menu(mainprogram->mainmenu);
	if (k == 0) {
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
		std::thread filereq(&Program::get_outname, mainprogram, "New project", "application/ewocvj2-project", boost::filesystem::absolute(reqdir + name).generic_string());
		filereq.detach();
	}
	else if (k == 1) {
		mainprogram->pathto = "OPENPROJECT";
		std::thread filereq(&Program::get_inname, mainprogram, "Open project file", "application/ewocvj2-project", boost::filesystem::canonical(mainprogram->currprojdir).generic_string());
		filereq.detach();
	}
	else if (k == 2) {
		mainprogram->project->do_save(mainprogram->project->path);
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
		std::thread filereq(&Program::get_inname, mainprogram, "Open state file", "application/ewocvj2-state", boost::filesystem::canonical(mainprogram->currstatedir).generic_string());
		filereq.detach();
	}
	else if (k == 6) {
		mainprogram->pathto = "SAVESTATE";
		std::thread filereq(&Program::get_outname, mainprogram, "Save state file", "application/ewocvj2-state", boost::filesystem::canonical(mainprogram->currstatedir).generic_string());
		filereq.detach();
	}
	else if (k == 7) {
		if (!mainprogram->prefon) {
			mainprogram->prefon = true;
			SDL_ShowWindow(mainprogram->prefwindow);
			for (int i = 0; i < mainprogram->prefs->items.size(); i++) {
				PrefCat* item = mainprogram->prefs->items[i];
				item->box->upvtxtoscr();
			}
		}
		else {
			SDL_RaiseWindow(mainprogram->prefwindow);
		}
	}
	else if (k == 8) {
		if (!mainprogram->midipresets) {
			mainprogram->midipresets = true;
			SDL_ShowWindow(mainprogram->config_midipresetswindow);
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
			SDL_RaiseWindow(mainprogram->config_midipresetswindow);
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
}

void Program::handle_shelfmenu() {
	int k = -1;
	// Draw and Program::handle shelfmenu
	k = mainprogram->handle_menu(mainprogram->shelfmenu);
	if (k == 0) {
		// new shelf
		mainmix->mouseshelf->erase();
	}
	else if (k == 1) {
		mainprogram->pathto = "OPENSHELF";
		std::thread filereq(&Program::get_inname, mainprogram, "Open shelf file", "application/ewocvj2-shelf", boost::filesystem::canonical(mainprogram->currshelfdir).generic_string());
		filereq.detach();
	}
	else if (k == 2) {
		mainprogram->pathto = "SAVESHELF";
		std::thread filereq(&Program::get_outname, mainprogram, "Save shelf file", "application/ewocvj2-shelf", boost::filesystem::canonical(mainprogram->currshelfdir).generic_string());
		filereq.detach();
	}
	else if (k == 3) {
		mainprogram->pathto = "OPENFILESSHELF";
		std::thread filereq(&Program::get_multinname, mainprogram, "Load file(s) in shelf", "", boost::filesystem::canonical(mainprogram->currshelffilesdir).generic_string());
		filereq.detach();
	}
	else if (k == 4 || k == 5) {
		// insert one of current decks into shelf element
		std::string path = find_unused_filename(basename(mainprogram->project->path), mainprogram->temppath, ".deck");
		mainmix->mousedeck = k - 4;
		mainmix->do_save_deck(path, false, true);
		mainmix->mouseshelf->insert_deck(path, k - 4, mainmix->mouseshelfelem);
	}
	else if (k == 6) {
		// insert current mix into shelf element
        std::string path = find_unused_filename(basename(mainprogram->project->path), mainprogram->temppath, ".mix");
		mainmix->do_save_mix(path, mainprogram->prevmodus, false);
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
    }
    else if (k == 9) {
		// learn MIDI for element layer load
		mainmix->learn = true;
		mainmix->learnparam = nullptr;
		mainmix->midishelfstage = 1;
		mainmix->midishelfpos = mainmix->mouseshelfelem;
		mainmix->midishelf = mainmix->mouseshelf;
		mainmix->learnbutton = mainmix->mouseshelf->buttons[mainmix->mouseshelfelem];
	}

	if (mainprogram->menuchosen) {
		mainprogram->menuchosen = false;
		mainprogram->menuactivation = 0;
		mainprogram->menuresults.clear();
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

	k = mainprogram->handle_menu(mainprogram->filemenu);
	if (k == 0) {
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
		else if (mainprogram->menuresults[0] == 6) {
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
            mainmix->mousedeck = 0;
			mainprogram->pathto = "OPENFILESSTACK";
			if (mainprogram->menuresults[1] == lvec.size()) {
				mainmix->addlay = true;
			}
			else {
				mainprogram->loadlay = lvec[mainprogram->menuresults[1]];
			}
			std::thread filereq(&Program::get_multinname, mainprogram, "Open video/image/layer file", "", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
			filereq.detach();
		}
		else if (mainprogram->menuresults[0] == 6) {
			// open files in layer in deck B
			std::vector<Layer*>& lvec = choose_layers(1);
            mainmix->mousedeck = 1;
			mainprogram->pathto = "OPENFILESSTACK";
			if (mainprogram->menuresults[1] == lvec.size()) {
				mainmix->addlay = true;
			}
			else {
				mainprogram->loadlay = lvec[mainprogram->menuresults[1]];
			}
			std::thread filereq(&Program::get_multinname, mainprogram, "Open video/image/layer file", "", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
			filereq.detach();
		}
		else if (mainprogram->menuresults[0] == 7) {
			// open files in in deck A
			std::vector<Layer*>& lvec = choose_layers(0);
            mainmix->mousedeck = 0;
			mainprogram->pathto = "OPENFILESQUEUE";
			std::thread filereq(&Program::get_multinname, mainprogram, "Open video/image/layer file", "", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
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
            mainmix->mousedeck = 1;
			mainprogram->pathto = "OPENFILESQUEUE";
			std::thread filereq(&Program::get_multinname, mainprogram, "Open video/image/layer file", "", boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
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
}

void Program::handle_editmenu() {
    int k = -1;
    // Draw and Program::handle editmenu
    k = mainprogram->handle_menu(mainprogram->editmenu);
    if (k == 0) {
        if (!mainprogram->prefon) {
            mainprogram->prefon = true;
            SDL_ShowWindow(mainprogram->prefwindow);
            SDL_RaiseWindow(mainprogram->prefwindow);
            SDL_GL_MakeCurrent(mainprogram->prefwindow, glc);
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
        if (!mainprogram->midipresets) {
            SDL_ShowWindow(mainprogram->config_midipresetswindow);
            SDL_RaiseWindow(mainprogram->config_midipresetswindow);
            SDL_GL_MakeCurrent(mainprogram->config_midipresetswindow, glc);
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
    }
}

void Program::handle_lpstmenu() {
    if (!mainmix->mouselpstelem) return;
    if (mainmix->mouselpstelem->eventlist.size() == 0) return;
    int k = -1;
    // Draw and handle lpstmenu
    k = mainprogram->handle_menu(mainprogram->lpstmenu);
    if (k == 0) {
        mainmix->cbduration = mainmix->mouselpstelem->totaltime;
    }
    else if (k == 1) {
        float buspeed = mainmix->mouselpstelem->speed->value;
        mainmix->mouselpstelem->speed->value = mainmix->mouselpstelem->totaltime / mainmix->cbduration;
        mainmix->mouselpstelem->speedadaptedtime *= buspeed / mainmix->mouselpstelem->speed->value;
        mainmix->mouselpstelem->interimtime *= buspeed / mainmix->mouselpstelem->speed->value;
    }
    else if (k == 2) {
        mainmix->mouselpstelem->totaltime = mainmix->cbduration;
    }

    if (mainprogram->menuchosen) {
        mainprogram->menuchosen = false;
        mainprogram->menuactivation = 0;
        mainprogram->menuresults.clear();
    }
}

// end of menu code



void Program::preview_modus_buttons() {
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
		register_triangle_draw(white, white, box->vtxcoords->x1 + box->vtxcoords->w / 2.0f + 0.0117f, box->vtxcoords->y1 + 0.0225f, 0.0165f, 0.0312f, DOWN, CLOSED);
		render_text(mainprogram->toscreen->name[0], white, box->vtxcoords->x1 + 0.0117f, box->vtxcoords->y1 + 0.0225f, 0.0006f, 0.001f);
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
		register_triangle_draw(white, white, box->vtxcoords->x1 + box->vtxcoords->w / 2.0f + 0.0117f, box->vtxcoords->y1 + 0.0225f, 0.0165f, 0.0312f, UP, CLOSED);
		render_text(mainprogram->backtopre->name[0], white, mainprogram->backtopre->box->vtxcoords->x1 + 0.0117f, mainprogram->backtopre->box->vtxcoords->y1 + 0.0225f, 0.0006, 0.001);
	}

	mainprogram->modusbut->handle();
	if (mainprogram->modusbut->toggled()) {
		mainprogram->prevmodus = !mainprogram->prevmodus;
		//modusbut is button that toggles effect preview mode to performance mode and back
		mainprogram->preview_init();
	}
	render_text(mainprogram->modusbut->name[mainprogram->prevmodus], white, mainprogram->modusbut->box->vtxcoords->x1 + 0.0117f, mainprogram->modusbut->box->vtxcoords->y1 + 0.0225f, 0.00042, 0.00070);
}

void Program::preview_init() {
	// extra initialization when prevmodus is changed (preview modus)
    //std::vector<Layer *> &lvec = choose_layers(mainmix->currlay[mainprogram->prevmodus]->deck);
    //int p = mainmix->currlay[mainprogram->prevmodus]->pos;
    //if (p > lvec.size() - 1) p = lvec.size() - 1;
    //mainmix->currlay[!mainprogram->prevmodus] = lvec[p];

	GLint preff = glGetUniformLocation(this->ShaderProgram, "preff");
	glUniform1i(preff, this->prevmodus);
}


void Program::preferences() {
	if (mainprogram->prefon) {
        mainprogram->directmode = true;
        SDL_GL_MakeCurrent(mainprogram->prefwindow, glc);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDrawBuffer(GL_BACK_LEFT);
		glViewport(0, 0, glob->w / 2.0f, glob->h / 2.0f);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
		bool prret = mainprogram->preferences_handle();
		if (prret) {
			SDL_GL_SwapWindow(mainprogram->prefwindow);
		}
		SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDrawBuffer(GL_BACK_LEFT);
        glViewport(0, 0, glob->w, glob->h);
	}
}

bool Program::preferences_handle() {
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
	float lightblue[] = { 0.5f, 0.5f, 1.0f, 1.0f };
	float green[] = { 0.0f, 0.7f, 0.0f, 1.0f };

	mainprogram->bvao = mainprogram->prboxvao;
	mainprogram->bvbuf = mainprogram->prboxvbuf;
	mainprogram->btbuf = mainprogram->prboxtbuf;
	glClear(GL_COLOR_BUFFER_BIT);

	for (int i = 0; i < mainprogram->prefs->items.size(); i++) {
		PrefCat* item = mainprogram->prefs->items[i];
		if (item->box->in(mx, my)) {
			draw_box(white, lightblue, item->box, -1);
			if (mainprogram->leftmouse && mainprogram->prefs->curritem != i) {
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

	PrefCat* mci = mainprogram->prefs->items[mainprogram->prefs->curritem];
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
						PIMidi* midici = (PIMidi*)mci;
						if (!midici->items[i]->onoff) {
							if (std::find(midici->onnames.begin(), midici->onnames.end(), midici->items[i]->name) != midici->onnames.end()) {
								midici->onnames.erase(std::find(midici->onnames.begin(), midici->onnames.end(), midici->items[i]->name));
								mainprogram->openports.erase(std::find(mainprogram->openports.begin(), mainprogram->openports.end(), i));
								mci->items[i]->midiin->cancelCallback();
								delete mci->items[i]->midiin;
							}
						}
						else {
							midici->onnames.push_back(midici->items[i]->name);
							RtMidiIn* midiin = new RtMidiIn(RtMidi::UNIX_JACK);
							if (std::find(mainprogram->openports.begin(), mainprogram->openports.end(), i) == mainprogram->openports.end()) {
								midiin->openPort(i);
								midiin->setCallback(&mycallback, (void*)midici->items[i]);
								mainprogram->openports.push_back(i);
							}
							mci->items[i]->midiin = midiin;
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
                    if (mci->items[i]->dest == &mainprogram->project->ow || mci->items[i]->dest == &mainprogram->project->oh) {
                        mainprogram->saveproject = true;
                    }
				}
				else if (mainprogram->renaming == EDIT_CANCEL) {
					mci->items[i]->renaming = false;
				}
				else {
					do_text_input(mci->items[i]->valuebox->vtxcoords->x1 + 0.1f, mci->items[i]->valuebox->vtxcoords->y1 + 0.06f, 0.0024f, 0.004f, mx, my, mainprogram->xvtxtoscr(0.15f), 1, mci->items[i], true);
				}
			}
			else {
				render_text(std::to_string(mci->items[i]->value), white, mci->items[i]->valuebox->vtxcoords->x1 + 0.1f, mci->items[i]->valuebox->vtxcoords->y1 + 0.06f, 0.0024f, 0.004f, 1, 0);
			}
			if (mci->items[i]->valuebox->in(mx, my)) {
				if (mainprogram->leftmouse && mainprogram->renaming == EDIT_NONE) {
					mci->items[i]->renaming = true;
					((PIVid*)(mci->items[i]))->oldvalue = mci->items[i]->value;
					mainprogram->renaming = EDIT_NUMBER;
					mainprogram->inputtext = std::to_string(mci->items[i]->value);
					mainprogram->cursorpos0 = mainprogram->inputtext.length();
					SDL_StartTextInput();
				}
			}
		}
        bool cond1 = (mci->items[i]->type == PREF_PATH);
        bool cond2 = (mci->items[i]->type == PREF_PATHS);
        if (cond1 || cond2) {
            std::vector<std::string> paths;
            if (cond1) paths.push_back(*(std::string *) (mci->items[i]->dest));
            else paths = *(std::vector<std::string> *) (mci->items[i]->dest);

            if (cond2) {
                // mousewheel scroll
                mainprogram->pathscroll -= mainprogram->mousewheel;
                if (mainprogram->pathscroll < 0) mainprogram->pathscroll = 0;
                if (paths.size() > 7 && paths.size() - mainprogram->pathscroll < 7)
                    mainprogram->pathscroll = paths.size() - 6;

                // GUI arrow scroll
                mainprogram->pathscroll = mainprogram->handle_scrollboxes(mainprogram->defaultsearchscrollup,
                                                                          mainprogram->defaultsearchscrolldown,
                                                                          paths.size(), mainprogram->pathscroll, 6,
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
                    render_text(paths[j + (mainprogram->pathscroll * cond2)], white,
                                mci->items[i]->valuebox->vtxcoords->x1 + 0.1f, mci->items[i]->valuebox->vtxcoords->y1 - j * 0.2f + 0.03f, 0.0024f, 0.004f, 1, 0);
                }
                else {
                    if (mainprogram->renaming == EDIT_NONE) {
                        mci->items[i]->renaming = false;
                        paths[j + (mainprogram->pathscroll * cond2)] = mainprogram->inputtext;
                        if (mci->items[i]->dest == &mainprogram->project->name) {
                            mainprogram->projnamechanged = true;
                            mainprogram->saveproject = true;
                        }
                    }
                    else if (mainprogram->renaming == EDIT_CANCEL) {
                        mci->items[i]->renaming = false;
                    }
                    else {
                        do_text_input(mci->items[i]->valuebox->vtxcoords->x1 + 0.1f, mci->items[i]->valuebox->vtxcoords->y1 - j * 0.2f + 0.03f, 0.0024f, 0.004f, mx, my, mainprogram->xvtxtoscr(0.7f), 1, mci->items[i], true);
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
                        mainprogram->inputtext = paths[j];
                        mainprogram->cursorpos0 = mainprogram->inputtext.length();
                        SDL_StartTextInput();
                    }
                }
                if (mci->items[i]->dest != &mainprogram->project->name) {
                    draw_box(white, black, mci->items[i]->iconbox->vtxcoords->x1,
                             mci->items[i]->iconbox->vtxcoords->y1 - j * 0.2f, mci->items[i]->iconbox->vtxcoords->w,
                             mci->items[i]->iconbox->vtxcoords->h, -1);
                    draw_box(white, black, mci->items[i]->iconbox->vtxcoords->x1 + 0.02f,
                             mci->items[i]->iconbox->vtxcoords->y1 - j * 0.2f + 0.05f, 0.06f, 0.07f, -1);
                    draw_box(white, black, mci->items[i]->iconbox->vtxcoords->x1 + 0.05f,
                             mci->items[i]->iconbox->vtxcoords->y1 - j * 0.2f + 0.11f, 0.025f, 0.03f, -1);
                    if (mci->items[i]->iconbox->in(mx, my)) {
                        if (mainprogram->leftmouse) {
                            mci->items[i]->choosing = true;
                            mainprogram->pathto = "CHOOSEDIR";
                            std::string title = "Open " + mci->items[i]->name + " directory";
                            std::thread filereq(&Program::get_dir, mainprogram, title.c_str(),
                                                boost::filesystem::canonical(mci->items[i]->path).generic_string());
                            filereq.detach();
                        }
                    }
                    if (mci->items[i]->choosing && mainprogram->choosedir != "") {
#ifdef WINDOWS
                        boost::replace_all(mainprogram->choosedir, "/", "\\");
#endif
                        paths[j] = mainprogram->choosedir;
                        mainprogram->choosedir = "";
                        mci->items[i]->choosing = false;
                    }
                }
            }

            if (cond1) *(std::string*)mci->items[i]->dest = paths[0];
            else *(std::vector<std::string>*)mci->items[i]->dest = paths;
        }

        if (cond2) {
            Box box;
            box.vtxcoords->x1 = -0.3f;
            box.vtxcoords->y1 = -0.8f;
            box.vtxcoords->w = 0.4f;
            box.vtxcoords->h = 0.2f;
            box.upvtxtoscr();
            draw_box(white, black, &box, -1);
            render_text("+ DEFAULT SEARCH DIR", white, -0.25f, -0.8f + 0.03f, 0.0024f, 0.004f, 1, 0);
            if (box.in(mx, my) && mainprogram->leftmouse) {
                mainprogram->pathto = "ADDSEARCHDIR";
                std::thread filereq(&Program::get_dir, mainprogram, "Add a search location",
                                    boost::filesystem::canonical(mainprogram->currfilesdir).generic_string());
                filereq.detach();
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
	if (box.in2(mx, my)) {
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
            mainprogram->pathscroll = 0;  // needed after default search listing
			SDL_HideWindow(mainprogram->prefwindow);
			SDL_RaiseWindow(mainprogram->mainwindow);
		}
	}
	render_text("CANCEL", white, box.vtxcoords->x1 + 0.02f, box.vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, 1, 0);
	box.vtxcoords->x1 = 0.45f;
	box.upvtxtoscr();
	draw_box(white, black, &box, -1);
	if (box.in2(mx, my)) {
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
			if (mainprogram->projnamechanged) {  // project name not included in preferences file, only change if
			    // user clicks SAVE
                if (mainprogram->project->name != remove_extension(basename(mainprogram->project->path))) {
                    // project name changed
                    // rename project file
                    boost::filesystem::rename(mainprogram->project->path, dirname(mainprogram->project->path) +
                                                                          mainprogram->project->name + ".ewocvj");
                    // rename project directory
                    boost::filesystem::rename(remove_extension(mainprogram->project->path), dirname
                                                                                                    (mainprogram->project->path) + mainprogram->project->name);
                    int pos = std::find(mainprogram->recentprojectpaths.begin(), mainprogram->recentprojectpaths.end(), mainprogram->project->path) - mainprogram->recentprojectpaths.begin();
                    mainprogram->project->path = dirname(mainprogram->project->path) +
                                                 mainprogram->project->name + ".ewocvj";
                    if (pos < mainprogram->recentprojectpaths.size()) {
                        mainprogram->recentprojectpaths[pos] = mainprogram->project->path;
                        mainprogram->write_recentprojectlist();
                    }
                    std::string dir = remove_extension(mainprogram->project->path) + "/";
                    mainprogram->project->binsdir = dir + "bins/";
                    mainprogram->project->recdir = dir + "recordings/";
                    mainprogram->project->shelfdir = dir + "shelves/";
                    mainprogram->project->autosavedir = dir + "autosaves/";
                }
            }
			// set output resolution from project settings
            mainprogram->projnamechanged = false;
			mainprogram->renaming = EDIT_NONE;
			mainprogram->prefs->save();
			mainprogram->prefs->load();
			mainprogram->prefon = false;
			mainprogram->drawnonce = false;

			if (mainprogram->saveproject) mainprogram->project->do_save(mainprogram->project->path);
            mainprogram->ow = mainprogram->project->ow;
            mainprogram->oh = mainprogram->project->oh;
            mainprogram->set_ow3oh3();
            mainprogram->handle_changed_owoh();

            SDL_HideWindow(mainprogram->prefwindow);
			SDL_RaiseWindow(mainprogram->mainwindow);
		}
	}
	render_text("SAVE", white, box.vtxcoords->x1 + 0.02f, box.vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, 1, 0);
    if (saveproject) render_text("+PROJECT", white, box.vtxcoords->x1 + 0.06f, box.vtxcoords->y1 + 0.1f, 0.0024f, 0.004f, 1, 0);

    mainprogram->tooltips_handle(1);

	mainprogram->bvao = mainprogram->boxvao;
	mainprogram->bvbuf = mainprogram->boxvbuf;
	mainprogram->btbuf = mainprogram->boxtbuf;
	mainprogram->middlemouse = false;
	mainprogram->rightmouse = false;
	mainprogram->menuactivation = false;
	mainprogram->mx = -1;
	mainprogram->my = -1;

	mainprogram->insmall = false;

	return true;
}


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
			float x = mainprogram->tooltipbox->vtxcoords->x1 + mainprogram->tooltipbox->vtxcoords->w + 0.015f;  //first print offscreen
			float y = mainprogram->tooltipbox->vtxcoords->y1 - 0.015f * glob->w / glob->h - 0.015f;
			float textw = 0.5f * sqrt(fac);
			float texth = 0.092754f * sqrt(fac);
			if ((x + textw) > 1.0f) x = x - textw - 0.03f - mainprogram->tooltipbox->vtxcoords->w;
			if ((y - texth * (texts.size() + 1) - 0.015f) < -1.0f) y = -1.0f + texth * (texts.size() + 1) - 0.015f;
			if (x < -1.0f) x = -1.0f;
			draw_box(nullptr, black, x, y - texth, textw, texth + 0.015f, -1);
			render_text(mainprogram->tooltipbox->tooltiptitle, orange, x + 0.0225f * sqrt(fac), y - texth + 0.045f * sqrt(fac), 0.00045f * fac, 0.00075f * fac, win, 0);
			for (int i = 0; i < texts.size(); i++) {
				y -= texth;
				draw_box(nullptr, black, x, y - texth, textw, texth + 0.015f, -1);
				render_text(texts[i], white, x + 0.0225f * sqrt(fac), y - texth + 0.045f * sqrt(fac), 0.00045f * fac, 0.00075f * fac, win, 0);
			}
		}
		else {
			float x = mainprogram->tooltipbox->vtxcoords->x1 + mainprogram->tooltipbox->vtxcoords->w + 0.015f;  //first print offscreen
			float y = mainprogram->tooltipbox->vtxcoords->y1 - 0.015f * glob->w / glob->h - 0.015f;
			float textw = 0.25f * sqrt(fac);
			draw_box(nullptr, black, x, y - 0.092754f, textw, 0.092754f + 0.015f, -1);
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
	if (mainprogram->midipresetsset == 1) lmstr = "A";
	else if (mainprogram->midipresetsset == 2) lmstr = "B";
	else if (mainprogram->midipresetsset == 3) lmstr = "C";
	else if (mainprogram->midipresetsset == 4) lmstr = "D";
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
		//draw config_midipresets_handle screen
		draw_box(yellow, grey, mainprogram->tmset, -1);
		if (mainprogram->tmset->in(mx, my)) {
			draw_box(red, lightblue, mainprogram->tmset, -1);
			if (mainprogram->leftmouse) {
				mainprogram->midipresetsset++;
				if (mainprogram->midipresetsset == 5) mainprogram->midipresetsset = 1;
			}
		}
		if (mainprogram->midipresetsset == 1) render_text("set A", yellow, 0.01f, 0.78f, 0.0024f, 0.004f, 2);
		else if (mainprogram->midipresetsset == 2) render_text("set B", yellow, 0.01f, 0.78f, 0.0024f, 0.004f, 2);
		else if (mainprogram->midipresetsset == 3) render_text("set C", yellow, 0.01f, 0.78f, 0.0024f, 0.004f, 2);
		else if (mainprogram->midipresetsset == 4) render_text("set D", yellow, 0.01f, 0.78f, 0.0024f, 0.004f, 2);
		LayMidi* lm = nullptr;
		if (mainprogram->midipresetsset == 1) lm = laymidiA;
		else if (mainprogram->midipresetsset == 2) lm = laymidiB;
		else if (mainprogram->midipresetsset == 3) lm = laymidiC;
		else if (mainprogram->midipresetsset == 4) lm = laymidiD;

		draw_box(white, black, mainprogram->tmplay, -1);
		if (lm->play[0] != -1) draw_box(white, darkgreen2, mainprogram->tmplay, -1);
		if (mainprogram->tmplay->in(mx, my)) {
			draw_box(white, lightblue, mainprogram->tmplay, -1);
			if (mainprogram->leftmouse) {
				mainprogram->tmlearn = TM_PLAY;
			}
		}
		register_triangle_draw(white, white, 0.125f, -0.83f, 0.06f, 0.12f, RIGHT, CLOSED, true);

		draw_box(white, black, mainprogram->tmbackw, -1);
		if (lm->backw[0] != -1) draw_box(white, darkgreen2, mainprogram->tmbackw, -1);
		if (mainprogram->tmbackw->in(mx, my)) {
			draw_box(white, lightblue, mainprogram->tmbackw, -1);
			if (mainprogram->leftmouse) {
				mainprogram->tmlearn = TM_BACKW;
			}
		}
		register_triangle_draw(white, white, -0.185f, -0.83f, 0.06f, 0.12f, LEFT, CLOSED, true);

		draw_box(white, black, mainprogram->tmbounce, -1);
		if (lm->bounce[0] != -1) draw_box(white, darkgreen2, mainprogram->tmbounce, -1);
		if (mainprogram->tmbounce->in(mx, my)) {
			draw_box(white, lightblue, mainprogram->tmbounce, -1);
			if (mainprogram->leftmouse) {
				mainprogram->tmlearn = TM_BOUNCE;
			}
		}
		register_triangle_draw(white, white, -0.045f, -0.83f, 0.04f, 0.12f, LEFT, CLOSED, true);
		register_triangle_draw(white, white, 0.01f, -0.83f, 0.04f, 0.12f, RIGHT, CLOSED, true);

		draw_box(white, black, mainprogram->tmfrforw, -1);
		if (lm->frforw[0] != -1) draw_box(white, darkgreen2, mainprogram->tmfrforw, -1);
		if (mainprogram->tmfrforw->in(mx, my)) {
			draw_box(white, lightblue, mainprogram->tmfrforw, -1);
			if (mainprogram->leftmouse) {
				mainprogram->tmlearn = TM_FRFORW;
			}
		}
		register_triangle_draw(white, white, 0.275f, -0.83f, 0.06f, 0.12f, RIGHT, OPEN, true);

		draw_box(white, black, mainprogram->tmfrbackw, -1);
		if (lm->frbackw[0] != -1) draw_box(white, darkgreen2, mainprogram->tmfrbackw, -1);
		if (mainprogram->tmfrbackw->in(mx, my)) {
			draw_box(white, lightblue, mainprogram->tmfrbackw, -1);
			if (mainprogram->leftmouse) {
				mainprogram->tmlearn = TM_FRBACKW;
			}
		}
		register_triangle_draw(white, white, -0.335f, -0.83f, 0.06f, 0.12f, LEFT, OPEN, true);

		draw_box(white, black, mainprogram->tmspeed, -1);
		if (lm->speed[0] != -1) draw_box(white, darkgreen2, mainprogram->tmspeed, -1);
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
			if (lm->speedzero[0] != -1) draw_box(white, darkgreen2, mainprogram->tmspeedzero, -1);
		}
		else {
			draw_box(white, black, mainprogram->tmspeedzero, -1);
			if (lm->speedzero[0] != -1) draw_box(white, darkgreen2, mainprogram->tmspeedzero, -1);
		}
		render_text("ONE", white, -0.755f, -0.08f, 0.0024f, 0.004f, 2);
		render_text("SPEED", white, -0.765f, -0.48f, 0.0024f, 0.004f, 2);

		draw_box(white, black, mainprogram->tmopacity, -1);
		if (lm->opacity[0] != -1) draw_box(white, darkgreen2, mainprogram->tmopacity, -1);
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
			if (lm->scratch[0] != -1) draw_box(darkgreen2, 0.0f, 0.1f, 0.6f, 1, smw, smh);
			if (sqrt(pow((mx / (glob->w / 2.0f) - 1.0f) * glob->w / glob->h, 2) + pow((glob->h - my) / (glob->h / 2.0f) - 1.1f, 2)) < 0.6f) {
				draw_box(lightblue, 0.0f, 0.1f, 0.6f, 1, smw, smh);
				mainprogram->tmscratch->in(mx, my);  //tooltip
				if (mainprogram->leftmouse) {
					mainprogram->tmlearn = TM_SCRATCH;
				}
			}
			draw_box(white, black, mainprogram->tmfreeze, -1);
			if (lm->scratchtouch[0] != -1) draw_box(white, darkgreen2, mainprogram->tmfreeze, -1);
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
		if (box.in2(mx, my)) {
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
	if (box.in2(mx, my)) {
		draw_box(white, lightblue, &box, -1);
		if (mainprogram->leftmouse) {
			open_genmidis(mainprogram->docpath + "midiset.gm");
			mainprogram->tmlearn = TM_NONE;
			mainprogram->midipresets = false;
			SDL_HideWindow(mainprogram->config_midipresetswindow);
		}
	}
	render_text("CANCEL", white, box.vtxcoords->x1 + 0.02f, box.vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, 2);
	box.vtxcoords->x1 = 0.45f;
	box.upvtxtoscr();
	draw_box(white, black, &box, -1);
	if (box.in2(mx, my)) {
		draw_box(white, lightblue, &box, -1);
		if (mainprogram->leftmouse) {
			save_genmidis(mainprogram->docpath + "midiset.gm");
			mainprogram->tmlearn = TM_NONE;
			mainprogram->midipresets = false;
			SDL_HideWindow(mainprogram->config_midipresetswindow);
		}
	}
	render_text("SAVE", white, box.vtxcoords->x1 + 0.02f, box.vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, 2);

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
	}
	return true;
}

void Program::pick_color(Layer* lay, Box* cbox) {
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
				Box* box = mainprogram->cwbox;
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
				Box* box = mainprogram->cwbox;
				mainprogram->directmode = true;
				draw_box(nullptr, box->acolor, box->vtxcoords->x1, box->vtxcoords->y1, box->vtxcoords->w, box->vtxcoords->h, -1);
				mainprogram->directmode = false;
				glUniform1i(cwon, 0);
				if (length <= 0.75f || length >= 1.0f) {
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
    #ifdef __linux__
    std::string ddir (this->docpath);
    if (exists("./shader.vs")) strcpy (vshader, "./shader.vs");
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
	
	return program;
}




//  THINGS PROJECT RELATED



void Project::delete_dirs() {
	this->path = mainprogram->path;
	std::string dir = remove_extension(path) + "/";
	boost::filesystem::path d{ dir };
	boost::filesystem::remove_all(d);
	for (int i = 0; i < binsmain->bins.size(); i++) {
		binsmain->bins[i]->path = dir + "bins/" + binsmain->bins[i]->name + ".bin";
	}
	binsmain->save_binslist();
}

void Project::create_dirs(const std::string &path) {
	std::string dir = path + "/";
	this->binsdir = dir + "bins/";
	this->recdir = dir + "recordings/";
    this->shelfdir = dir + "shelves/";
    this->autosavedir = dir + "autosaves/";
	boost::filesystem::path d{ dir };
	boost::filesystem::create_directory(d);
	boost::filesystem::path p1{ this->binsdir };
	boost::filesystem::create_directory(p1);
	boost::filesystem::path p2{ this->recdir };
	boost::filesystem::create_directory(p2);
    boost::filesystem::path p3{ this->shelfdir };
    boost::filesystem::create_directory(p3);
    boost::filesystem::path p4{ this->autosavedir };
    boost::filesystem::create_directory(p4);
}

void Project::newp(const std::string &path) {
	std::string ext = path.substr(path.length() - 7, std::string::npos);
	std::string str;
	std::string path2;
	if (ext != ".ewocvj") {
	    str = path + ".ewocvj";
	    path2 = path;
	}
	else {
	    str = path;
	    path2 = path.substr(0, path.size() - 7);
	}
	this->path = str;
	this->name = remove_extension(basename(this->path));

	// set project output width and height
    for (PrefCat *item : mainprogram->prefs->items) {
        // get the default output width and height
        for (PrefItem *pri : item->items) {
            if (pri->dest == &mainprogram->ow) {
                this->ow = pri->value;
            } else if (pri->dest == &mainprogram->oh) {
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

	this->create_dirs(path2);
	for (int i = 0; i < binsmain->bins.size(); i++) {
		delete binsmain->bins[i];
	}
	binsmain->bins.clear();
	binsmain->new_bin("this is a bin");
	binsmain->save_binslist();
	mainprogram->shelves[0]->erase();
	mainprogram->shelves[1]->erase();
	mainprogram->shelves[0]->basepath = "shelfsA.shelf";
	mainprogram->shelves[1]->basepath = "shelfsB.shelf";
	mainprogram->shelves[0]->save(mainprogram->project->shelfdir + "shelfsA.shelf");
	mainprogram->shelves[1]->save(mainprogram->project->shelfdir + "shelfsB.shelf");
	mainprogram->project->do_save(str);
}
	
void Project::open(const std::string& path) {
	std::string result = deconcat_files(path);
	bool concat = (result != "");
	std::ifstream rfile;
	if (concat) rfile.open(result);
	else rfile.open(path);

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
    mainprogram->project->name = remove_extension(basename(path));
    *namedest = &mainprogram->project->name;
	std::string dir = remove_extension(path) + "/";
	this->binsdir = dir + "bins/";
	this->recdir = dir + "recordings/";
    this->shelfdir = dir + "shelves/";
    this->autosavedir = dir + "autosaves/";
	int cb = binsmain->read_binslist();
	for (int i = 0; i < binsmain->bins.size(); i++) {
		std::string binname = this->binsdir + binsmain->bins[i]->name + ".bin";
		if (exists(binname)) {
			binsmain->open_bin(binname, binsmain->bins[i]);
		}
	}
	binsmain->make_currbin(cb);

    mainprogram->set_ow3oh3();
    //mainmix->new_state();
    mainmix->open_state(result + "_0.file");

    std::string istring;
	safegetline(rfile, istring);
	
	while (safegetline(rfile, istring)) {
		if (istring == "ENDOFFILE") break;
        if (istring == "OUTPUTWIDTH") {  // reminder: what to do? project or program level?
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
		if (istring == "CURRSHELFA") {
			safegetline(rfile, istring);
			if (istring != "") {
				mainprogram->shelves[0]->open(this->shelfdir + istring);
				mainprogram->shelves[0]->basepath = istring;
			}
			else {
				mainprogram->shelves[0]->erase();
			}
		}
		if (istring == "CURRSHELFB") {
			safegetline(rfile, istring);
			if (istring != "") {
				mainprogram->shelves[1]->open(this->shelfdir + istring);
				mainprogram->shelves[1]->basepath = istring;
			}
			else {
				mainprogram->shelves[1]->erase();
			}
		}

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
	
	int pos = std::find(mainprogram->recentprojectpaths.begin(), mainprogram->recentprojectpaths.end(), path) - mainprogram->recentprojectpaths.begin();
	if (pos < mainprogram->recentprojectpaths.size()) {
		std::swap(mainprogram->recentprojectpaths[pos], mainprogram->recentprojectpaths[0]);
	}
	else {
        mainprogram->recentprojectpaths.insert(mainprogram->recentprojectpaths.begin(), path);
	}
    mainprogram->write_recentprojectlist();

    if (exists(this->autosavedir + "autosavelist")) {
        std::ifstream rfile;
        rfile.open(this->autosavedir + "autosavelist");
        std::string istring;
        safegetline(rfile, istring);
        while (safegetline(rfile, istring)) {
            if (istring == "ENDOFFILE") break;
            this->autosavelist.push_back(istring);
        }
    }
}

void Project::save(const std::string& path) {
	std::thread projsav(&Project::do_save, this, path);
	projsav.detach();
}

void Project::do_save(const std::string& path) {
	std::string ext = path.substr(path.length() - 7, std::string::npos);
	std::string str;
	if (ext != ".ewocvj") str = path + ".ewocvj";
	else str = path;
	std::ofstream wfile;
	wfile.open(str);
	std::vector<std::string> filestoadd;
	
	wfile << "EWOCvj PROJECT V0.1\n";
    wfile << "OUTPUTWIDTH\n";
    wfile << std::to_string((int)this->ow);
    wfile << "\n";
	wfile << "OUTPUTHEIGHT\n";
	wfile << std::to_string((int)this->oh);
	wfile << "\n";
	mainprogram->shelves[0]->save(this->shelfdir + mainprogram->shelves[0]->basepath);
	wfile << "CURRSHELFA\n";
	wfile << mainprogram->shelves[0]->basepath;
	wfile << "\n";
	mainprogram->shelves[1]->save(this->shelfdir + mainprogram->shelves[1]->basepath);
	wfile << "CURRSHELFB\n";
	wfile << mainprogram->shelves[1]->basepath;
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
	
	mainmix->do_save_state(mainprogram->temppath + "current.state", false);
	filestoadd.push_back(mainprogram->temppath + "current.state");

    for (int i = 0; i < binsmain->bins.size(); i++) {
        binsmain->save_bin(binsmain->bins[i]->path);
    }
    binsmain->save_binslist();

    std::ofstream outputfile;
	outputfile.open(mainprogram->temppath + "tempconcatproj", std::ios::out | std::ios::binary);
	std::vector<std::vector<std::string>> filestoadd2;
	filestoadd2.push_back(filestoadd);
	concat_files(outputfile, str, filestoadd2);
	outputfile.close();
	boost::filesystem::rename(mainprogram->temppath + "tempconcatproj", str);
	
	if (std::find(mainprogram->recentprojectpaths.begin(), mainprogram->recentprojectpaths.end(), str) == mainprogram->recentprojectpaths.end()) {
		mainprogram->recentprojectpaths.insert(mainprogram->recentprojectpaths.begin(), str);
		if (mainprogram->recentprojectpaths.size() > 10) {
			mainprogram->recentprojectpaths.pop_back();
		}
		#ifdef WINDOWS
		std::string dir = mainprogram->docpath;
		#else
		#ifdef POSIX
		std::string homedir (getenv("HOME"));
		std::string dir = homedir + "/.ewocvj2/";
		#endif
		#endif
		wfile.open(dir + "recentprojectslist");
		wfile << "EWOCvj RECENTPROJECTS V0.1\n";
		for (int i = 0; i < mainprogram->recentprojectpaths.size(); i++) {
			wfile << mainprogram->recentprojectpaths[i];
			wfile << "\n";
		}
		wfile << "ENDOFFILE\n";
		wfile.close();
	}
}

void Project::autosave() {
    mainprogram->astimestamp = (int) mainmix->time;

    std::string name;
    if (this->autosavelist.size()) {
        name = remove_extension(this->autosavelist.back());
    } else {
        name = this->autosavedir + "autosave_" +
               remove_extension(basename(this->path)) + "_0";
    }
    int num = std::stoi(name.substr(name.rfind('_') + 1, name.length() - name.rfind('_') - 1));
    name = this->autosavedir + basename(remove_version(name)) + "_" + std::to_string(num + 1);
    std::string path = name + ".state";

    mainmix->do_save_state(path, true);

    this->autosavelist.push_back(path);
    if (this->autosavelist.size() > 20)
        this->autosavelist.erase(this->autosavelist.begin());
    std::ofstream wfile;
    wfile.open(this->autosavedir + "autosavelist");
    wfile << "EWOCvj AUTOSAVELIST V0.1\n";
    for (int i = 0; i < this->autosavelist.size(); i++) {
        wfile << this->autosavelist[i];
        wfile << "\n";
    }
    wfile << "ENDOFFILE\n";
    wfile.close();
}



//  THINGS PREFERENCES RELATED


Preferences::Preferences() {
    PIProj *pipr = new PIProj;
    pipr->box = new Box;
    pipr->box->tooltiptitle = "Project settings ";
    pipr->box->tooltip = "Left click to set project related preferences ";
    this->items.push_back(pipr);
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
                                        if (pi->dest) *(bool*)pi->dest = pi->onoff;
                                    }
                                    else if (pi->type == PREF_NUMBER) {
                                        pi->value = std::stoi(istring);
                                        if (pi->dest) *(float*)pi->dest = (float)pi->value;
                                    }
                                    else if (pi->type == PREF_PATH) {
                                        boost::filesystem::path p(istring);
                                        if (!boost::filesystem::exists(p)) {
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
                                            mainprogram->openports.erase(std::find(mainprogram->openports.begin(), mainprogram->openports.end(), foundpos));
                                            pi->midiin->cancelCallback();
                                            delete pi->midiin;
                                        }
                                    }
                                    else {
                                        pim->onnames.push_back(pi->name);
                                        RtMidiIn *midiin = new RtMidiIn(RtMidi::UNIX_JACK);
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
            safegetline(rfile, istring);
            boost::filesystem::path p(istring);
            if (boost::filesystem::exists(p)) mainprogram->currfilesdir = istring;
        }
        else if (istring == "CURRCLIPFILESDIR") {
            safegetline(rfile, istring);
            boost::filesystem::path p(istring);
            if (boost::filesystem::exists(p)) mainprogram->currclipfilesdir = istring;
        }
        else if (istring == "CURRSHELFFILESDIR") {
            safegetline(rfile, istring);
            boost::filesystem::path p(istring);
            if (boost::filesystem::exists(p)) mainprogram->currshelffilesdir = istring;
        }
        else if (istring == "CURRBINFILESDIR") {
            safegetline(rfile, istring);
            boost::filesystem::path p(istring);
            if (boost::filesystem::exists(p)) mainprogram->currbinfilesdir = istring;
        }
        else if (istring == "CURRSTATEDIR") {
            safegetline(rfile, istring);
            boost::filesystem::path p(istring);
            if (boost::filesystem::exists(p)) mainprogram->currstatedir = istring;
        }
        else if (istring == "CURRSHELFFILESDIR") {
            safegetline(rfile, istring);
            boost::filesystem::path p(istring);
            if (boost::filesystem::exists(p)) mainprogram->currshelffilesdir = istring;
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
    if (type == PREF_PATH || type == PREF_PATHS) this->namebox->vtxcoords->w = 0.6f;
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
    else if (type == PREF_PATH || type == PREF_PATHS) {
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


PIProj::PIProj() {
    // Set all preferences items that appear under the Directories tab
    PrefItem *pip;
    int pos = 0;
    this->name = "Project";

    pip = new PrefItem(this, pos, "Project name", PREF_PATH, (void *) &mainprogram->project->name);
    pip->namebox->tooltiptitle = "Project name ";
    pip->namebox->tooltip = "Name of current project. ";
    pip->valuebox->tooltiptitle = "Set name of current project ";
    pip->valuebox->tooltip = "Leftclick starts keyboard entry of current project name. It also changes the name of "
                             "the project on disk.";

    this->items.push_back(pip);
    pos++;

    pip = new PrefItem(this, pos, "Project output video width", PREF_NUMBER, (void*)&mainprogram->project->ow);
    pip->namebox->tooltiptitle = "Project output video width ";
    pip->namebox->tooltip = "Sets the width in pixels of the video stream sent to the output for this project. ";
    pip->valuebox->tooltiptitle = "Project output video width ";
    pip->valuebox->tooltip = "Leftclicking the value allows setting the width in pixels of the video stream sent to the output for this project. ";
    mainprogram->ow = pip->value;
    this->items.push_back(pip);
    pos++;

    pip = new PrefItem(this, pos, "Project output video height", PREF_NUMBER, (void*)&mainprogram->project->oh);
    pip->namebox->tooltiptitle = "Project output video height ";
    pip->namebox->tooltip = "Sets the height in pixels of the video stream sent to the output for this project. ";
    pip->valuebox->tooltiptitle = "Project output video height ";
    pip->valuebox->tooltip = "Leftclicking the value allows setting the height in pixels of the video stream sent to the output for this project. ";
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
    pdi->namebox->tooltip = "Root directory relative to which referenced content will be searched. ";
    pdi->valuebox->tooltiptitle = "Set content root directory ";
    pdi->valuebox->tooltip = "Leftclick starts keyboard entry of location of content root directory. ";
    pdi->iconbox->tooltiptitle = "Browse to set content root directory ";
    pdi->iconbox->tooltip = "Leftclick allows browsing for location of content root directory. ";
#ifdef WINDOWS
    pdi->path = mainprogram->docpath + "projects/";
#else
#ifdef POSIX
    pdi->path = mainprogram->homedir + "/Videos/";
#endif
#endif
    mainprogram->projdir = pdi->path;
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
    for (int i = 0; i < nPorts; i++) {
        // Set all preferences items that appear under the Midi Devices tab
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

    pii = new PrefItem(this, pos, "Looped playback default", PREF_ONOFF, (void*)&mainprogram->repeatdefault);
    pii->onoff = 1;
    pii->namebox->tooltiptitle = "Looped playback default ";
    pii->namebox->tooltip = "Sets looped video playback default. ";
    pii->valuebox->tooltiptitle = "Looped playback default toggle ";
    pii->valuebox->tooltip = "Leftclick to set looped video playback default to on(green) or off(black). ";
    mainprogram->autosave = pii->onoff;
    this->items.push_back(pii);
    pos++;
}

PIVid::PIVid() {
    // Set all preferences items that appear under the Interface tab

    this->name = "Video";
    PrefItem *pvi;
    int pos = 0;

    pvi = new PrefItem(this, pos, "Default output video width", PREF_NUMBER, (void*)&mainprogram->ow);
    pvi->value = 1920;
    pvi->namebox->tooltiptitle = "Default output video width ";
    pvi->namebox->tooltip = "Sets the width in pixels of the video stream sent to the output for new projects. ";
    pvi->valuebox->tooltiptitle = "Default output video width ";
    pvi->valuebox->tooltip = "Leftclicking the value allows setting the width in pixels of the video stream sent to the output for new projects. ";
    mainprogram->ow = pvi->value;
    this->items.push_back(pvi);
    pos++;

    pvi = new PrefItem(this, pos, "Default output video height", PREF_NUMBER, (void*)&mainprogram->oh);
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
    pvi->valuebox->tooltip = "Leftclicking the value allows setting the quality in the range of 1 to 10.  Lower is better quality and worse framerate/choppier playback. ";
    mainprogram->qualfr = pvi->value;
    this->items.push_back(pvi);
    pos++;
}

PIProg::PIProg() {
    // Set all preferences items that appear under the Program tab

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
        mainprogram->effectsmap[(EFFECT_TYPE) (i - 1)] = effects[i];
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
    layops1.push_back("Open file(s) into layer");
    layops1.push_back("Open file(s) into queue");
    layops1.push_back("Insert file(s) in stack before layer");
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
    layops1.push_back("HAP encode on-the-fly");
    mainprogram->make_menu("laymenu1", mainprogram->laymenu1, layops1);

    std::vector<std::string> layops2;
    layops2.push_back("submenu livemenu");
    layops2.push_back("Connect live");
    layops2.push_back("Open file(s) into layer");
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
    shelf1.push_back("Erase element");
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

    std::vector<std::string> lpst;
    lpst.push_back("Copy loop duration");
    lpst.push_back("Paste loop duration by changing speed");
    lpst.push_back("Paste loop duration by changing loop length");
    mainprogram->make_menu("lpstmenu", mainprogram->lpstmenu, lpst);

    //make menu item names bitmaps
    for (int i = 0; i < mainprogram->menulist.size(); i++) {
        for (int j = 0; j < mainprogram->menulist[i]->entries.size(); j++) {
            render_text(mainprogram->menulist[i]->entries[j], white, 2.0f, 2.0f, 0.00045f, 0.00075f, 0, 0, 0);
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
    wfile << "EWOCvj RECENTPROJECTS V0.1\n";
    for (int i = 0; i < mainprogram->recentprojectpaths.size(); i++) {
        wfile << mainprogram->recentprojectpaths[i];
        wfile << "\n";
    }
    wfile << "ENDOFFILE\n";
    wfile.close();
}


void Program::socket_server(struct sockaddr_in serv_addr, int opt) {
    this->server = true;
    int new_socket;
    int addrlen = sizeof(serv_addr);
    int ret = setsockopt(this->sock, SOL_SOCKET, SO_REUSEADDR,
                   (const char*)&opt, sizeof(opt));

    // Forcefully attaching socket to the port 8000
    ret = bind(this->sock, (struct sockaddr *)&serv_addr,
             sizeof(serv_addr));

    while (listen(this->sock, 3) >= 0)
    {
        new_socket = accept(this->sock, (struct sockaddr *)&serv_addr,
                                 (socklen_t*)&addrlen);
 #ifdef POSIX
       set_nonblock(new_socket);
 #endif
        mainprogram->connsockets.push_back(new_socket);
        send(new_socket , std::to_string(mainprogram->connsockets.size()).c_str() , strlen(std::to_string(mainprogram->connsockets.size()).c_str()) , 0 );
        printf("CONNECTED\n");
    }
    perror("listen");
    exit(EXIT_FAILURE);
}