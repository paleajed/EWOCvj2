#pragma once
#ifdef __APPLE__

struct SDL_Window;

// SDL_GL_GetDrawableSize() is written for SDL's native Cocoa GL path
// (NSOpenGLContext) — with ANGLE (Metal-layer-backed EGL surface, used for
// our GLES rendering on macOS) it doesn't reliably report the true 2x
// Retina backing size, silently falling back to the logical window size
// instead. Query NSWindow's backingScaleFactor directly instead.
namespace MacWindowUtils {

// Returns the window's real backing scale factor (e.g. 2.0 on Retina,
// 1.0 otherwise), or 1.0 if it can't be determined.
float getBackingScaleFactor(SDL_Window* window);

// SDL_RaiseWindow only reorders within the same NSWindow level, so simply
// raising an auxiliary window isn't enough to guarantee it stays above
// mainwindow — explicitly put it one level above mainwindow's current
// level. Call this after showing/raising the aux window.
void raiseAboveWindow(SDL_Window* windowToRaise, SDL_Window* referenceWindow);

// SDL puts SDL_WINDOW_FULLSCREEN_DESKTOP windows at CGShieldingWindowLevel()
// on macOS — the lock-screen shield level, deliberately above absolutely
// everything, including the Dock's hover-reveal and the menu bar's own
// dropdown windows. Call this right after creating the window to drop it to
// NSNormalWindowLevel, below both, so they can render above it normally.
// Returns true if the level actually needed changing (was not already
// NSNormalWindowLevel) — a one-time call finding this false every frame
// would mean something is continuously resetting it back.
bool setWindowLevelNormal(SDL_Window* window);

// mainwindow occupies its own dedicated macOS Space even under
// SDL_WINDOW_FULLSCREEN_DESKTOP (confirmed via Mission Control), and the
// Dock/menu-bar popups still don't render above it even at
// NSNormalWindowLevel — suggesting their visibility inside a dedicated
// Space is gated by NSApplicationPresentationOptions rather than window
// z-order. Call this once the window exists to explicitly request the
// permissive auto-hide behavior for both.
void setPermissiveFullscreenPresentation();

// mainwindow is confirmed (via Mission Control) to occupy its own dedicated
// Space even under SDL_WINDOW_FULLSCREEN_DESKTOP, at NSNormalWindowLevel,
// with permissive presentation options — neither of which stopped it.
// NSWindowCollectionBehaviorFullScreenPrimary is what tells Mission Control
// a window deserves its own Space; SDL's Cocoa backend may be setting it
// even for the desktop-fullscreen path. Call this to explicitly clear it.
void clearFullScreenPrimaryBehavior(SDL_Window* window);

// Registers a persistent NSApplicationDidBecomeActiveNotification observer
// that re-applies setPermissiveFullscreenPresentation() (plus window level
// and collection behavior) every time this app regains activation. The Dock
// was observed to only start auto-hiding correctly after specific external
// events (e.g. launching a brand-new app from the Dock) rather than
// anything this app does at startup — this makes the fix self-healing on
// whatever the next real activation event turns out to be, instead of
// depending on identifying the exact trigger. Call once at startup.
void installActivationSelfHeal(SDL_Window* window);

// A Finder-launched .app gets promoted to macOS's key window automatically
// via LaunchServices; a debugger-launched raw binary (e.g. CLion/lldb execing
// the binary inside the bundle directly, bypassing that activation path)
// often doesn't, even though it's visually frontmost and receiving mouse
// events. SDL3's text input (SDL_StartTextInput) is gated on real key-window
// status, not just mouse focus, so under a debugger-launched process typed
// keys and even backspace silently went nowhere in every text field, while
// clicks/hover still worked fine. Call this once right after creating
// mainwindow to explicitly activate the app and make it key regardless of
// how the process was launched.
void activateAndMakeKey(SDL_Window* window);

} // namespace MacWindowUtils

#endif // __APPLE__
