#pragma once
#ifdef __APPLE__

#include <string>
#include <vector>

// Native Cocoa (NSSavePanel/NSOpenPanel) file dialogs for macOS.
//
// tinyfiledialogs' AppleScript-based dialogs (osascript -e 'choose file
// name ...') run as a separate, unrelated process — and since mainwindow
// is a borderless window covering the full screen, macOS's window server
// silently blocks that foreign process's panel from ever displaying
// (confirmed via the unified log: the panel's XPC connection gets
// activated and invalidated within ~65ms, no error, no user interaction).
// A native panel lives inside our own window/process, so it isn't subject
// to that. AppKit requires panels to be created/run on the main thread;
// these functions marshal onto it (via dispatch_sync) regardless of which
// thread calls them, skipping the hop if already on the main thread — so
// existing call sites that run get_outname()/get_inname()/etc. on a
// background thread keep working unchanged.
namespace MacFileDialogs {

// Returns the chosen path, or "" if cancelled.
// extensionNoDot: e.g. "layer", or "" for no filter.
std::string saveFile(const std::string& title, const std::string& defaultDir,
                      const std::string& defaultName, const std::string& extensionNoDot);

std::string openFile(const std::string& title, const std::string& defaultDir,
                      const std::string& extensionNoDot);

// Empty vector if cancelled/no selection.
std::vector<std::string> openFiles(const std::string& title, const std::string& defaultDir);

std::string chooseFolder(const std::string& title, const std::string& defaultDir);

} // namespace MacFileDialogs

#endif // __APPLE__
