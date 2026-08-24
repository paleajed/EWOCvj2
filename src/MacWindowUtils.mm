#ifdef __APPLE__

#include "MacWindowUtils.h"
#import <Cocoa/Cocoa.h>
#include <SDL3/SDL.h>

namespace MacWindowUtils {

// SDL3 removed SDL_SysWMinfo/SDL_GetWindowWMInfo entirely in favor of the
// generic window-properties API — the native NSWindow* is retrieved via
// SDL_PROP_WINDOW_COCOA_WINDOW_POINTER instead.
static NSWindow* cocoaWindow(SDL_Window* window) {
    if (!window) return nil;
    SDL_PropertiesID props = SDL_GetWindowProperties(window);
    if (!props) return nil;
    return (__bridge NSWindow*)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL);
}

float getBackingScaleFactor(SDL_Window* window) {
    NSWindow* nsWindow = cocoaWindow(window);
    if (!nsWindow) return 1.0f;

    return (float)nsWindow.backingScaleFactor;
}

void raiseAboveWindow(SDL_Window* windowToRaise, SDL_Window* referenceWindow) {
    NSWindow* nsWindowToRaise = cocoaWindow(windowToRaise);
    NSWindow* nsReferenceWindow = cocoaWindow(referenceWindow);
    if (!nsWindowToRaise) return;

    NSInteger level = nsReferenceWindow ? (nsReferenceWindow.level + 1) : (NSFloatingWindowLevel);
    [nsWindowToRaise setLevel:level];
    [nsWindowToRaise makeKeyAndOrderFront:nil];
}

bool setWindowLevelNormal(SDL_Window* window) {
    NSWindow* nsWindow = cocoaWindow(window);
    if (!nsWindow) return false;

    bool wasNotNormal = (nsWindow.level != NSNormalWindowLevel);
    [nsWindow setLevel:NSNormalWindowLevel];
    return wasNotNormal;
}

void setPermissiveFullscreenPresentation() {
    [[NSApplication sharedApplication] setPresentationOptions:
        (NSApplicationPresentationAutoHideDock | NSApplicationPresentationAutoHideMenuBar)];
}

void clearFullScreenPrimaryBehavior(SDL_Window* window) {
    NSWindow* nsWindow = cocoaWindow(window);
    if (!nsWindow) return;

    nsWindow.collectionBehavior = nsWindow.collectionBehavior & ~NSWindowCollectionBehaviorFullScreenPrimary;
}

void installActivationSelfHeal(SDL_Window* window) {
    // Confirmed: the Dock only starts auto-hiding correctly when a newly
    // launched app is actually closed/terminated - not on activation/focus
    // changes alone (switching to an already-running app doesn't do it).
    // That's NSWorkspaceDidTerminateApplicationNotification, a system-wide
    // notification fired whenever any app quits - not an NSApplication
    // activation event. Re-assert the fullscreen presentation/level/
    // collection behavior whenever it fires, so this self-corrects on
    // whatever real termination event happens next, rather than depending
    // on identifying why termination specifically is what flips it.
    void (^reassert)(NSNotification*) = ^(NSNotification * _Nonnull note) {
        setPermissiveFullscreenPresentation();
        setWindowLevelNormal(window);
        clearFullScreenPrimaryBehavior(window);
    };
    [[[NSWorkspace sharedWorkspace] notificationCenter]
        addObserverForName:NSWorkspaceDidTerminateApplicationNotification
                    object:nil
                     queue:[NSOperationQueue mainQueue]
                usingBlock:reassert];
    [[NSNotificationCenter defaultCenter]
        addObserverForName:NSApplicationDidBecomeActiveNotification
                    object:nil
                     queue:[NSOperationQueue mainQueue]
                usingBlock:reassert];
}

void activateAndMakeKey(SDL_Window* window) {
    [NSApp activateIgnoringOtherApps:YES];
    NSWindow* nsWindow = cocoaWindow(window);
    if (nsWindow) {
        [nsWindow makeKeyAndOrderFront:nil];
    }
}

} // namespace MacWindowUtils

#endif // __APPLE__
