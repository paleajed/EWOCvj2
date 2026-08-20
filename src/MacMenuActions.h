#pragma once
#include <string>
#include <vector>

// Thin, Cocoa-independent wrappers around Program::menu*() (see program.h),
// implemented in program.cpp. Kept separate so MacMenuBar.mm doesn't need
// to include program.h alongside Cocoa.h: program.h transitively pulls in
// styleroom.h's `class Style`, which collides with the macOS SDK's own
// legacy `typedef unsigned char Style` (MacTypes.h, reached via Cocoa.h).
namespace EWOCMenuActions {

void newProject();
void newMix();
void newDeckA();
void newDeckB();
void openProject();
void openMix();
void openDeckA();
void openDeckB();
void saveProjectAs();
void saveMix();
void saveDeckA();
void saveDeckB();
void saveProject();
void quit();
void preferences();
void configureMIDI();
std::vector<std::string> audioDeviceNames();
std::string currentAudioDevice();
void setBeatmatchDevice(int index);
// room: 0=Mix, 1=Bins, 2=Style, 3=Gen, 4=Segment (matches ROOMMENU_OPTION in program.h)
bool canSwitchToRoom(int room);
void switchRoom(int room);
void documentation();

// deck: 0=Deck A, 1=Deck B. layerCount() is the number of EXISTING layers
// (used for Save As); New/Open additionally offer one slot past the end
// (slot == layerCount(deck)) meaning "add a new layer".
int layerCount(int deck);
void newLayerInDeck(int deck, int slot);
void openFilesIntoLayer(int deck, int slot);
void openFilesIntoQueue(int deck, int slot);
void saveLayerAs(int deck, int slot);

// Cocoa's NSMenu tracking runs its own nested run loop that blocks the
// calling (main) thread until the menu is dismissed — called repeatedly
// from an NSTimer scheduled on NSEventTrackingRunLoopMode (see MacMenuBar.mm)
// so the live video mix keeps rendering while a native menu is open.
void pumpFrameDuringMenuTracking();

} // namespace EWOCMenuActions
