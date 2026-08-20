#pragma once
#ifdef __APPLE__

namespace MacMenuBar {

// Builds and installs a native macOS menu bar (File/Configure/Rooms/Help)
// mirroring the app's own custom-drawn top-bar menus, wired to the same
// underlying Program:: actions (see program.h's menu* methods). The
// standard "Quit EWOCvj2" item (and its Cmd+Q key equivalent) is routed to
// the app's own quit flow instead of terminating the process directly.
// Call once at startup, after mainprogram exists.
void install();

} // namespace MacMenuBar

#endif // __APPLE__
