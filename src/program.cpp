//#define _AFXDLL
//#include <afxwin.h>
#include <boost/algorithm/string.hpp>
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

#ifdef _WIN64
#include <atlstr.h>
#include <Commdlg.h>
#include <initguid.h>
#include <KnownFolders.h>
#include <ShlObj.h>
#endif
#include <wchar.h>
#include <string>

#include "SDL2/SDL.h"
#include "SDL2/SDL_syswm.h"

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
	this->project = new Project;

	#ifdef _WIN64
	PWSTR charbuf;
	HRESULT hr = SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &charbuf);
	std::wstring ws1(charbuf);
	std::string str1(ws1.begin(), ws1.end());
	this->docpath = boost::filesystem::canonical(str1).string() + "/EWOCvj2/";
	this->currfilesdir = this->docpath + "elems/";
	this->currclipfilesdir = this->docpath + "elems/";
	this->currshelffilesdir = this->docpath + "elems/";
	this->currbinfilesdir = this->docpath + "elems/";
	this->currstatedir = this->docpath + "elems/";
	hr = SHGetKnownFolderPath(FOLDERID_Fonts, 0, NULL, &charbuf);
	std::wstring ws2(charbuf);
	std::string str2(ws2.begin(), ws2.end());
	this->fontpath = str2;
	//hr = SHGetKnownFolderPath(FOLDERID_Videos, 0, NULL, &charbuf);
	//std::wstring ws4(charbuf);
	//std::string str4(ws4.begin(), ws4.end());
	//str4 += "/";   reminder: do something with Videos dir?
	std::wstring ws3;
	wchar_t wcharPath[MAX_PATH];
	if (GetTempPathW(MAX_PATH, wcharPath)) ws3 = wcharPath;
	std::string str3(ws3.begin(), ws3.end());
	this->temppath = str3 + "EWOCvj2/";
	#else
	#ifdef __linux__
	std::string homedir (getenv("HOME"));
	this->temppath = homedir + "/.ewocvj2/temp/";
	#endif
	#endif
	
	this->numh = this->numh * glob->w / glob->h;
	this->layh = this->layh * (glob->w / glob->h) / (1920.0f /  1080.0f);
	this->monh = this->monh * (glob->w / glob->h) / (1920.0f /  1080.0f);
	
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
	this->buttons.push_back(this->toscreen);
	this->toscreen->name[0] = "SEND";
	this->toscreen->name[1] = "SEND";
	this->toscreen->box->vtxcoords->x1 = -0.3;
	this->toscreen->box->vtxcoords->y1 = -1.0f + this->monh * 2.0f + 0.1f;
	this->toscreen->box->vtxcoords->w = 0.3 / 2.0;
	this->toscreen->box->vtxcoords->h = 0.3 / 3.0;
	this->toscreen->box->upvtxtoscr();
	this->toscreen->box->tooltiptitle = "Send preview streams to output ";
	this->toscreen->box->tooltip = "Leftclick sends/copies the streams being previewed to the output streams ";
	
	this->backtopre = new Button(false);
	this->buttons.push_back(this->backtopre);
	this->backtopre->name[0] = "SEND";
	this->backtopre->name[1] = "SEND";
	this->backtopre->box->vtxcoords->x1 = -0.3;
	this->backtopre->box->vtxcoords->y1 = -1.0f + this->monh * 2.0f;
	this->backtopre->box->vtxcoords->w = 0.15f;
	this->backtopre->box->vtxcoords->h = 0.1f;
	this->backtopre->box->upvtxtoscr();
	this->backtopre->box->tooltiptitle = "Send output streams to preview ";
	this->backtopre->box->tooltip = "Leftclick sends/copies the streams being output back into the preview streams ";
	
	this->modusbut = new Button(false);
	this->buttons.push_back(this->modusbut);
	this->modusbut->name[0] = "LIVE MODUS";
	this->modusbut->name[1] = "PREVIEW MODUS";
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
	this->outputmonitor->vtxcoords->y1 = -1.0f + this->monh * 2.0f + tf(0.05f);
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
	this->buttons.push_back(this->effcat[0]);
	this->effcat[0]->name[0] = "Layer effects";
	this->effcat[0]->name[1] = "Stream effects";
	this->effcat[0]->box->vtxcoords->x1 = -1.0f;
	this->effcat[0]->box->vtxcoords->y1 = 1.0f - tf(this->layh) - tf(0.19f);
	this->effcat[0]->box->vtxcoords->w = tf(0.025f);
	this->effcat[0]->box->vtxcoords->h = tf(0.2f);
	this->effcat[0]->box->upvtxtoscr();
	this->effcat[0]->box->tooltiptitle = "Layer/stream effects toggle ";
	this->effcat[0]->box->tooltip = "Leftclick toggles between layer effects queue (these effects only affect the current layer) and stream effects queue (effects affect mix of all layers upto and including the current one). ";
	
	this->effcat[1] = new Button(false);
	this->buttons.push_back(this->effcat[1]);
	this->effcat[1]->name[0] = "Layer effects";
	this->effcat[1]->name[1] = "Stream effects";
	float xoffset = 1.0f + this->layw - 0.019f;
	this->effcat[1]->box->vtxcoords->x1 = -1.0f + this->numw - tf(0.025f) + xoffset;
	this->effcat[1]->box->vtxcoords->y1 = 1.0f - tf(this->layh) - tf(0.19f);
	this->effcat[1]->box->vtxcoords->w = tf(0.025f);
	this->effcat[1]->box->vtxcoords->h = tf(0.2f);
	this->effcat[1]->box->upvtxtoscr();
	this->effcat[1]->box->tooltiptitle = "Layer/stream effects toggle ";
	this->effcat[1]->box->tooltip = "Leftclick toggles between layer effects queue (these effects only affect the current layer) and stream effects queue (effects affect mix of all layers upto and including the current one). ";
	
	this->effscrollupA = new Box;
	this->effscrollupA->vtxcoords->x1 = -1.0;
	this->effscrollupA->vtxcoords->y1 = 1.0 - tf(this->layh) - tf(0.25f);
	this->effscrollupA->vtxcoords->w = tf(0.025f);
	this->effscrollupA->vtxcoords->h = tf(0.05f);
	this->effscrollupA->upvtxtoscr();
	this->effscrollupA->tooltiptitle = "Scroll effects queue up ";
	this->effscrollupA->tooltip = "Leftclicking scrolls the effect queue up ";
	
	this->effscrollupB = new Box;
	this->effscrollupB->vtxcoords->x1 = 1.0 - tf(0.05f);
	this->effscrollupB->vtxcoords->y1 = 1.0 - tf(this->layh) - tf(0.25f);
	this->effscrollupB->vtxcoords->w = tf(0.025f);
	this->effscrollupB->vtxcoords->h = tf(0.05f);     
	this->effscrollupB->upvtxtoscr();
	this->effscrollupB->tooltiptitle = "Scroll effects queue up ";
	this->effscrollupB->tooltip = "Leftclicking scrolls the effect queue up ";
	
	this->effscrolldownA = new Box;
	this->effscrolldownA->vtxcoords->x1 = -1.0;
	this->effscrolldownA->vtxcoords->y1 = 1.0 - tf(this->layh) - tf(0.25f) - tf(0.05f);
	this->effscrolldownA->vtxcoords->w = tf(0.025f);
	this->effscrolldownA->vtxcoords->h = tf(0.05f);
	this->effscrolldownA->upvtxtoscr();
	this->effscrolldownA->tooltiptitle = "Scroll effects queue down ";
	this->effscrolldownA->tooltip = "Leftclicking scrolls the effect queue down ";
	
	this->effscrolldownB = new Box;
	this->effscrolldownB->vtxcoords->x1 = 1.0 - tf(0.05f);
	this->effscrolldownB->vtxcoords->y1 = 1.0 - tf(this->layh) - tf(0.25f) - tf(0.05f);
	this->effscrolldownB->vtxcoords->w = tf(0.025f);
	this->effscrolldownB->vtxcoords->h = tf(0.05f);
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

	this->addeffectbox = new Box;
	this->addeffectbox->vtxcoords->w = tf(this->layw);
	this->addeffectbox->vtxcoords->h = tf(0.038f);
	this->addeffectbox->tooltiptitle = "Add effect ";
	this->addeffectbox->tooltip = "Add effect to end of layer effect queue ";
	
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
	
	this->wormhole1 = new Button(false);
	this->wormhole1->box->vtxcoords->x1 = -1.0f;
	this->wormhole1->box->vtxcoords->y1 = -0.58f;
	this->wormhole1->box->vtxcoords->w = tf(0.025f);
	this->wormhole1->box->vtxcoords->h = 0.6f;
	this->wormhole1->box->upvtxtoscr();
	this->wormhole1->box->tooltiptitle = "Screen switching wormgate ";
	this->wormhole1->box->tooltip = "Connects mixing screen and media bins screen.  Leftclick to switch screen.  Drag content inside white rectangle up to the very edge of the screen to travel to the other screen. ";
	this->buttons.push_back(this->wormhole1);
	this->wormhole2 = new Button(false);
	this->wormhole2->box->vtxcoords->x1 = 1.0f - tf(0.025f);
	this->wormhole2->box->vtxcoords->y1 = -0.58f;
	this->wormhole2->box->vtxcoords->w = tf(0.025f);
	this->wormhole2->box->vtxcoords->h = 0.6f;
	this->wormhole2->box->upvtxtoscr();
	this->wormhole2->box->tooltiptitle = "Screen switching wormgate ";
	this->wormhole2->box->tooltip = "Connects mixing screen and media bins screen.  Leftclick to switch screen.  Leftclick to switch screen.  Drag content inside white rectangle up to the very edge of the screen to travel to the other screen. ";
	this->buttons.push_back(this->wormhole2);
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
	if (filters == "application/ewocvj2-layer") return ".layer (EWOCvj2 layer file)\0*.layer\0";
	if (filters == "application/ewocvj2-deck") return ".deck (EWOCvj2 deck file)\0*.deck\0";
	if (filters == "application/ewocvj2-mix") return ".mix (EWOCvj2 mix file)\0*.mix\0";
	if (filters == "application/ewocvj2-state") return ".state (EWOCvj2 state file)\0*.state\0";
	if (filters == "application/ewocvj2-project") return ".ewocvj (EWOCvj2 project file)\0*.deck\0";
	if (filters == "application/ewocvj2-shelf") return ".shelf (EWOCvj2 shelf file)\0*.shelf\0";
	if (filters == "application/ewocvj2-bin") return ".bin (EWOCvj2 bin file)\0*.bin\0";
}

void Program::win_dialog(const char* title, std::string filters, std::string defaultdir, bool open, bool multi) {
	SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN);
	boost::replace_all(defaultdir, "/", "\\");
	boost::filesystem::path p(defaultdir);
	std::string name;
	if (!boost::filesystem::is_directory(p)) {
		name = basename(defaultdir);
		defaultdir = defaultdir.substr(0, defaultdir.length() - name.length() - 1);
	}
#ifdef _WIN64
	OPENFILENAME ofn;
	char szFile[MAX_PATH];
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
	ofn.lpstrFilter = filters.c_str();
	ofn.nFilterIndex = 1;
	ofn.lpstrTitle = title;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = defaultdir.c_str();
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	if (multi) ofn.Flags = ofn.Flags | OFN_ALLOWMULTISELECT | OFN_EXPLORER;
	if (open) GetOpenFileName(&ofn);
	else GetSaveFileName(&ofn);
	if (strlen(ofn.lpstrFile) == 0) {
		binsmain->openbinfile = false;
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

void Program::get_inname(const char *title, std::string filters, std::string defaultdir) {
	bool as = mainprogram->autosave;
	mainprogram->autosave = false;
	#ifdef _WIN64
	filters = this->mime_to_wildcard(filters);
	this->win_dialog(title, filters.c_str(), defaultdir, true, false);
	#endif
	mainprogram->autosave = as;
}

void Program::get_outname(const char *title, std::string filters, std::string defaultdir) {
	bool as = mainprogram->autosave;
	mainprogram->autosave = false;

	#ifdef _WIN64
	filters = this->mime_to_wildcard(filters);
	this->win_dialog(title, filters.c_str(), defaultdir, false, false);
	#endif
	mainprogram->autosave = as;
}

void Program::get_multinname(const char* title, std::string defaultdir) {
	bool as = this->autosave;
	this->autosave = false;
	//outpaths = tinyfd_openFileDialog(title, dd, 0, nullptr, nullptr, 1);
	#ifdef _WIN64
	this->win_dialog(title, "", defaultdir, true, true);
	#endif
	#ifdef __linux__
	#endif
	if (mainprogram->paths.size()) {
		this->path = (char*)"ENTER";
		this->counting = 0;
	}
	this->autosave = as;
}

void Program::get_dir(std::string title, std::string defaultdir) {
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

#ifdef _WIN64
	OleInitialize(NULL); 
	TCHAR* szDir = new TCHAR[MAX_PATH];
	BROWSEINFO bInfo;
	ZeroMemory(&bInfo, sizeof(bInfo));
	bInfo.hwndOwner = nullptr;
	bInfo.pidlRoot = nullptr;
	bInfo.pszDisplayName = szDir;
	bInfo.lpszTitle = title.c_str();
	bInfo.ulFlags = 0;
	bInfo.lpfn = nullptr;
	bInfo.lParam = 0;
	bInfo.iImage = 0;
	bInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI | BIF_BROWSEINCLUDEFILES;

	LPITEMIDLIST lpItem = SHBrowseForFolder(&bInfo);
	if (lpItem != NULL)
	{
		SHGetPathFromIDList(lpItem, szDir);
		CoTaskMemFree(lpItem);
	}
	this->path = (char*)szDir;
	OleUninitialize();
#endif

	//this->path = tinyfd_selectFolderDialog(title, dd) ;
	mainprogram->autosave = as;
}

bool Program::order_paths(bool dodeckmix) {
	if (mainprogram->paths.size() == 1) return true;
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
		if (isimage(str)) {
			tex = get_imagetex(str);
		}
		else if (str.substr(str.length() - 6, std::string::npos) == ".layer") {
			tex = get_layertex(str);
		}
		else if (dodeckmix and str.substr(str.length() - 5, std::string::npos) == ".deck") {
			tex = get_deckmixtex(str);
		}
		else if (dodeckmix and str.substr(str.length() - 4, std::string::npos) == ".mix") {
			tex = get_deckmixtex(str);
		}
		else {
			tex = get_videotex(str);
		}
		mainprogram->pathtexes.push_back(tex);
		if (mainprogram->filescount < mainprogram->paths.size()) return false;
		for (int j = 0; j < mainprogram->paths.size() + 1; j++) {
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
		bool cont = mainprogram->do_order_paths();
		if (!cont) return false;
		mainprogram->multistage = 3;
	}
	if (mainprogram->multistage == 3) {
		// then cleanup
		for (int j = 0; j < mainprogram->paths.size(); j++) {
			delete mainprogram->pathboxes[j];
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
	float white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	int limit = this->paths.size();
	if (limit > 17) limit = 17;

	// mousewheel scroll
	this->pathscroll -= mainprogram->mousewheel;
	if (this->dragstr != "" and mainmix->time - mainmix->oldtime> 0.1f) {
		mainmix->oldtime = mainmix->time;
		// border scroll when dragging
		if (mainprogram->my < yvtxtoscr(0.1f)) this->pathscroll--;
		if (mainprogram->my > yvtxtoscr(1.9f)) this->pathscroll++;
	}
	if (this->pathscroll < 0) this->pathscroll = 0;
	if (this->paths.size() > 18 and this->paths.size() - this->pathscroll < 18) this->pathscroll = this->paths.size() - 17;

	// draw and handle orderlist scrollboxes
	this->pathscroll = mainprogram->handle_scrollboxes(this->orderscrollup, this->orderscrolldown, this->paths.size(), this->pathscroll, 17);
	bool indragbox = false;
	for (int j = this->pathscroll; j < this->pathscroll + limit; j++) {
		Box* box = this->pathboxes[j];
		box->vtxcoords->y1 = 0.8f - (j - this->pathscroll) * 0.1f;
		draw_box(white, black, box, -1);
		draw_box(white, black, 0.3f, box->vtxcoords->y1, 0.1f, 0.1f, this->pathtexes[j]);
		render_text(this->paths[j], white, -0.4f + tf(0.01f), box->vtxcoords->y1 + tf(0.05f) - tf(0.030f), tf(0.0003f), tf(0.0005f));
		// prepare effect dragging
		if (box->in()) {
			std::string path = this->paths[j];
			if (this->dragstr == "") {
				if (mainprogram->orderleftmousedown and !this->dragpathsense) {
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

	if (!indragbox and this->dragpathsense) {
		this->dragstr = this->paths[this->dragpathpos];
		this->dragpathsense = false;
	}

	// confirm box
	this->pathboxes.back()->vtxcoords->y1 = 0.8f - limit * 0.1f;
	this->pathboxes.back()->upvtxtoscr();
	draw_box(white, black, this->pathboxes.back(), -1);
	render_text("APPLY ORDER", white, -0.4f + tf(0.01f), this->pathboxes.back()->vtxcoords->y1 + tf(0.05f) - tf(0.030f), tf(0.0003f), tf(0.0005f));
	if (this->pathboxes.back()->in() and this->dragstr == "") {
		if (mainprogram->orderleftmouse) return true;
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
			if (mainprogram->my > under2 and mainprogram->my < upper) {
				draw_box(white, black, this->pathboxes[this->dragpathpos]->vtxcoords->x1, 1.0f - mainprogram->yscrtovtx(under1), this->pathboxes[this->dragpathpos]->vtxcoords->w, this->pathboxes[this->dragpathpos]->vtxcoords->h, -1);
				draw_box(white, black, 0.3f, 1.0f - mainprogram->yscrtovtx(under1), 0.1f, 0.1f, this->pathtexes[this->dragpathpos]);
				render_text(this->dragstr, white, -0.4f + tf(0.01f), 1.0f - mainprogram->yscrtovtx(under1) + tf(0.05f) - tf(0.030f), tf(0.0003f), tf(0.0005f));
				pos = j;
				break;
			}
		}
		if (mainprogram->orderleftmouse) {
			// do bin drag
			if (this->dragpathpos < pos) {
				std::rotate(this->paths.begin() + this->dragpathpos, this->paths.begin() + this->dragpathpos + 1, this->paths.begin() + pos);
				std::rotate(this->pathboxes.begin() + this->dragpathpos, this->pathboxes.begin() + this->dragpathpos + 1, this->pathboxes.begin() + pos - 1);
				std::rotate(this->pathtexes.begin() + this->dragpathpos, this->pathtexes.begin() + this->dragpathpos + 1, this->pathtexes.begin() + pos);
			}
			else {
				std::rotate(this->paths.begin() + pos, this->paths.begin() + this->dragpathpos, this->paths.begin() + this->dragpathpos + 1);
				std::rotate(this->pathboxes.begin() + pos, this->pathboxes.begin() + this->dragpathpos, this->pathboxes.begin() + this->dragpathpos);
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

	return false;

}


void Program::handle_wormhole(bool hole) {
	float white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	float lightblue[] = { 0.5f, 0.5f, 1.0f, 1.0f };

	Box* box;
	if (hole == 0) box = mainprogram->wormhole1->box;
	else box = mainprogram->wormhole2->box;

	if (hole == 0) {
		draw_triangle(white, white, -1.0 + box->vtxcoords->w, box->vtxcoords->y1 + box->vtxcoords->h / 4.0f, box->vtxcoords->h / 4.0f, box->vtxcoords->h / 2.0f, LEFT, OPEN);
		if (mainprogram->binsscreen) render_text("MIX", white, -0.9f, -0.29f, 0.0006f, 0.001f);
		else render_text("BINS", white, -0.9f, -0.29f, 0.0006f, 0.001f);
	}
	else {
		if (mainprogram->binsscreen) {
			draw_triangle(white, white, 1.0f - box->vtxcoords->w - box->vtxcoords->h / 4.0f * 0.866f, box->vtxcoords->y1 + box->vtxcoords->h / 4.0f + 0.3f, box->vtxcoords->h / 4.0f, box->vtxcoords->h / 2.0f, RIGHT, OPEN);
			render_text("MIX", white, 0.86f, 0.01f, 0.0006f, 0.001f);
		}
		else {
			draw_triangle(white, white, 1.0f - box->vtxcoords->w - box->vtxcoords->h / 4.0f * 0.866f, box->vtxcoords->y1 + box->vtxcoords->h / 4.0f, box->vtxcoords->h / 4.0f, box->vtxcoords->h / 2.0f, RIGHT, OPEN);
			render_text("BINS", white, 0.86f, -0.29f, 0.0006f, 0.001f);
		}
	}

	if (mainprogram->binsscreen) {
		box->vtxcoords->y1 = -1.0f;
		box->vtxcoords->h = 2.0f;
		box->upvtxtoscr();
	}

	//draw and handle BINS wormhole
	if (mainprogram->fullscreen == -1) {
		if (box->in()) {
			draw_box(white, lightblue, box, -1);
			mainprogram->tooltipbox = mainprogram->wormhole1->box;
			if (!mainprogram->menuondisplay) {
				if (mainprogram->leftmouse) {
					mainprogram->binsscreen = !mainprogram->binsscreen;
				}
				if (mainprogram->menuactivation) {
					mainprogram->parammenu1->state = 2;
					mainmix->learnparam = nullptr;
					mainmix->learnbutton = mainprogram->wormhole1;
					mainprogram->menuactivation = false;
				}
			}
			if (mainprogram->dragbinel) {
				//dragging something inside wormhole
				if (!mainprogram->inwormhole and !mainprogram->menuondisplay) {
					if (mainprogram->mx == hole * (glob->w - 1)) {
						if (!mainprogram->binsscreen) {
							set_queueing(false);
						}
						mainprogram->binsscreen = !mainprogram->binsscreen;
						mainprogram->inwormhole = true;
					}
				}
			}
		}
		else {
			draw_box(white, white, box, -1);
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
	if (this->ow != this->oldow or this->oh != this->oldoh) {
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
	glUniform1i(down, 0);
	glUniform1i(wipe, 0);
	glUniform1i(mixmode, 0);
	if (this->menuactivation) {
		this->fullscreenmenu->state = 2;
		this->menuondisplay = true;
		this->menuactivation = false;
	}

	// Draw and handle fullscreenmenu
	int k = handle_menu(this->fullscreenmenu);
	if (k == 0) this->fullscreen = -1;
	if (this->menuchosen) {
		this->menuchosen = false;
		this->menuactivation = 0;
		this->menuresults.clear();
	}
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
	// general code for scrollbuttons of scrollable lists
	float white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	if (scrollpos > 0) {
		if (upperbox->in()) {
			upperbox->acolor[0] = 0.5f;
			upperbox->acolor[1] = 0.5f;
			upperbox->acolor[2] = 1.0f;
			upperbox->acolor[3] = 1.0f;
			if (mainprogram->leftmouse or mainprogram->orderleftmouse) {
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
		draw_triangle(white, white, upperbox->vtxcoords->x1 + (lowerbox->vtxcoords->w / tf(0.05f)) * tf(0.0074f), upperbox->vtxcoords->y1 + (lowerbox->vtxcoords->w / tf(0.05f)) * (tf(0.0416f) - tf(0.030f)), tf(0.011f), tf(0.0208f), DOWN, CLOSED);
	}
	if (numlines - scrollpos > scrlines) {
		if (lowerbox->in()) {
			lowerbox->acolor[0] = 0.5f;
			lowerbox->acolor[1] = 0.5f;
			lowerbox->acolor[2] = 1.0f;
			lowerbox->acolor[3] = 1.0f;
			if (mainprogram->leftmouse or mainprogram->orderleftmouse) {
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
		draw_triangle(white, white, lowerbox->vtxcoords->x1 + (lowerbox->vtxcoords->w / tf(0.05f)) * tf(0.0074f), lowerbox->vtxcoords->y1 + (lowerbox->vtxcoords->w / tf(0.05f)) * (tf(0.0416f) - tf(0.030f)), tf(0.011f), tf(0.0208f), UP, CLOSED);
	}
	return scrollpos;
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

	float white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	float black[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float red[] = { 1.0f, 0.0f, 0.0f, 1.0f };
	float lightblue[] = { 0.5f, 0.5f, 1.0f, 1.0f };

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
	if (box.in(mx, my)) {
		draw_box(white, lightblue, &box, -1);
		if (mainprogram->leftmouse) {
			mainprogram->project->do_save(mainprogram->project->path);
			ret = 1;
		}
	}
	render_text("SAVE PROJECT", white, box.vtxcoords->x1 + 0.02f, box.vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, 1, 0);

	box.vtxcoords->x1 = 0.45f;
	box.upvtxtoscr();
	draw_box(white, black, &box, -1);
	if (box.in(mx, my)) {
		draw_box(white, lightblue, &box, -1);
		if (mainprogram->leftmouse) {
			ret = 2;
		}
	}
	render_text("QUIT", white, box.vtxcoords->x1 + 0.02f, box.vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, 1, 0);

	box.vtxcoords->x1 = 0.75f;
	box.upvtxtoscr();
	draw_box(white, black, &box, -1);
	if (box.in(mx, my)) {
		draw_box(white, lightblue, &box, -1);
		if (mainprogram->leftmouse) {
			ret = 3;
		}
	}
	render_text("CANCEL", white, box.vtxcoords->x1 + 0.02f, box.vtxcoords->y1 + 0.03f, 0.0024f, 0.004f, 1, 0);

	mainprogram->bvao = mainprogram->boxvao;
	mainprogram->bvbuf = mainprogram->boxvbuf;
	mainprogram->btbuf = mainprogram->boxtbuf;
	mainprogram->middlemouse = 0;
	mainprogram->rightmouse = 0;
	mainprogram->menuactivation = false;
	mainprogram->mx = -1;
	mainprogram->my = -1;

	mainprogram->insmall = false;

	return ret;
}
void Program::preview_init() {
	// extra initialization when prevmodus is changed (preview modus)
	std::vector<Layer*> &lvec = choose_layers(mainmix->currlay->deck);
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
	unsigned long vlen = 0;
	unsigned long flen = 0;
	char *VShaderSource;
 	char *vshader = (char*)malloc(100);
 	#ifdef _WIN64
 	if (exists("./shader.vs")) strcpy (vshader, "./shader.vs");
 	else mainprogram->quitting = "Unable to find vertex shader \"shader.vs\" in current directory";
 	#else
 	#ifdef __linuxd::string ddir (mainprogram(exists("./shader.vs")) strcpy (vshader, "./shader.vs");
 	else if (exists(ddir + "/shader.vs")) strcpy (vshader, (ddir + "/shader.vs").c_str());
 	else mainprogram->quitting = "Unable to find vertex shader \"shader.vs\" in " + ddir;
 	#endif
 	#endif
 	load_shader(vshader, &VShaderSource, vlen);
	char *FShaderSource;
 	char *fshader = (char*)malloc(100);
 	#ifdef _WIN64
 	if (exists("./shader.fs")) strcpy (fshader, "./shader.fs");
 	else mainprogram->quitting = "Unable to find fragment shader \"shader.fs\" in current directory";
 	#else
 	#ifdef __linux__
 	if (exists("./shader.fs")) strcpy (fshader, "./shader.fs");
 	else if (exists(ddir + "/shader.fs")) strcpy (fshader, (ddir + "/shader.fs").c_str());
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


void Project::newp(const std::string &path) {
	std::string ext = path.substr(path.length() - 7, std::string::npos);
	std::string str;
	if (ext != ".ewocvj") str = path + ".ewocvj";
	else str = path;
	mainprogram->project->path = str;
	
	std::string dir = remove_extension(path) + "/";
	this->binsdir = dir + "bins/";
	this->recdir = dir + "recordings/";
	this->shelfdir = dir + "shelves/";
	boost::filesystem::path d{dir};
	boost::filesystem::create_directory(d);
	boost::filesystem::path p1{this->binsdir};
	boost::filesystem::create_directory(p1);
	boost::filesystem::path p2{this->recdir};
	boost::filesystem::create_directory(p2);
	boost::filesystem::path p3{this->shelfdir};
	boost::filesystem::create_directory(p3);
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
	mainprogram->project->do_save(path);
}
	
void Project::open(const std::string &path) {
	std::string result = deconcat_files(path);
	bool concat = (result != "");
	std::ifstream rfile;
	if (concat) rfile.open(result);
	else rfile.open(path);
	
	mainprogram->project->path = path;
	std::string dir = remove_extension(path) + "/";
	this->binsdir = dir + "bins/";
	this->recdir = dir + "recordings/";
	this->shelfdir = dir + "shelves/";
	int cb = binsmain->read_binslist();
	for (int i = 0; i < binsmain->bins.size(); i++) {
		std::string binname = this->binsdir + binsmain->bins[i]->name + ".bin";
		if (exists(binname)) binsmain->open_bin(binname, binsmain->bins[i]);
	}
	binsmain->make_currbin(cb);
	
	std::string istring;
	getline(rfile, istring);
	
	while (getline(rfile, istring)) {
		if (istring == "ENDOFFILE") break;
		if (istring == "OUTPUTWIDTH") {  // reminder: what to do? project or program level?
			getline(rfile, istring);
			//mainprogram->ow = std::stoi(istring);
			//mainprogram->oldow = mainprogram->ow;
		}
		else if (istring == "OUTPUTHEIGHT") {
			getline(rfile, istring);
			//mainprogram->oh = std::stoi(istring);
			//mainprogram->oldoh = mainprogram->oh;
		}
		if (istring == "CURRSHELFA") {
			getline(rfile, istring);
			if (istring != "") {
				mainprogram->shelves[0]->open(this->shelfdir + istring);
				mainprogram->shelves[0]->basepath = istring;
			}
			else {
				mainprogram->shelves[0]->erase();
			}
		}
		if (istring == "CURRSHELFB") {
			getline(rfile, istring);
			if (istring != "") {
				mainprogram->shelves[1]->open(this->shelfdir + istring);
				mainprogram->shelves[1]->basepath = istring;
			}
			else {
				mainprogram->shelves[1]->erase();
			}
		}

		if (istring == "CURRBINSDIR") {
			getline(rfile, istring);
			mainprogram->currbinsdir = istring;
		}
		if (istring == "CURRSHELFDIR") {
			getline(rfile, istring);
			mainprogram->currshelfdir = istring;
		}
		if (istring == "CURRRECDIR") {
			getline(rfile, istring);
			mainprogram->currrecdir = istring;
		}
	}
	
	rfile.close();
	
	mainprogram->set_ow3oh3();
	//mainmix->new_state();
	mainmix->open_state(result + "_0.file");
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
	wfile << std::to_string((int)mainprogram->ow);
	wfile << "\n";
	wfile << "OUTPUTHEIGHT\n";
	wfile << std::to_string((int)mainprogram->oh);
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
		#ifdef _WIN64
		std::string dir = mainprogram->docpath;
		#else
		#ifdef __linux__
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
	

