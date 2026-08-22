#ifdef __APPLE__

#include "MacFileDialogs.h"
#include "MacMenuActions.h"
#import <Cocoa/Cocoa.h>

namespace {

NSString* toNSString(const std::string& s) {
    return [NSString stringWithUTF8String:s.c_str()];
}

std::string fromNSString(NSString* s) {
    if (!s) return std::string();
    return std::string([s UTF8String]);
}

void applyDirectoryAndFilter(id panel, const std::string& defaultDir, const std::string& extensionNoDot) {
    if (!defaultDir.empty()) {
        NSString* dir = toNSString(defaultDir);
        BOOL isDir = NO;
        if ([[NSFileManager defaultManager] fileExistsAtPath:dir isDirectory:&isDir] && isDir) {
            [panel setDirectoryURL:[NSURL fileURLWithPath:dir isDirectory:YES]];
        }
    }
    if (!extensionNoDot.empty()) {
        // allowedFileTypes is deprecated in newer SDKs in favor of
        // UTType-based allowedContentTypes, but remains functional and
        // keeps this simple without an extra availability check / import.
        [panel setAllowedFileTypes:@[ toNSString(extensionNoDot) ]];
    }
}

// [panel runModal] runs its own nested run loop (NSModalPanelRunLoopMode)
// that blocks the calling thread until the panel is dismissed - same
// freeze-the-live-video-mix problem as native NSMenu tracking (see
// MacMenuBar.mm's startTrackingPump), just triggered by a file dialog
// instead of the menu bar. Same fix: a timer that only fires in that mode,
// driving the render loop for as long as the panel stays open.
struct ModalTrackingPump {
    NSTimer* timer = nil;
    ModalTrackingPump() {
        timer = [NSTimer timerWithTimeInterval:1.0 / 60.0
                                        repeats:YES
                                          block:^(NSTimer* _Nonnull t) {
            EWOCMenuActions::pumpFrameDuringMenuTracking();
        }];
        [[NSRunLoop currentRunLoop] addTimer:timer forMode:NSModalPanelRunLoopMode];
    }
    ~ModalTrackingPump() {
        [timer invalidate];
        timer = nil;
    }
};

} // namespace

namespace MacFileDialogs {

std::string saveFile(const std::string& title, const std::string& defaultDir,
                      const std::string& defaultName, const std::string& extensionNoDot) {
    __block std::string result;
    auto work = ^{
        NSSavePanel* panel = [NSSavePanel savePanel];
        if (!title.empty()) {
            [panel setTitle:toNSString(title)];
        }
        if (!defaultName.empty()) {
            [panel setNameFieldStringValue:toNSString(defaultName)];
        }
        applyDirectoryAndFilter(panel, defaultDir, extensionNoDot);

        [NSApp activateIgnoringOtherApps:YES];
        ModalTrackingPump pump;
        NSInteger response = [panel runModal];
        if (response == NSModalResponseOK && panel.URL) {
            result = fromNSString(panel.URL.path);
        }
    };
    if ([NSThread isMainThread]) {
        work();
    } else {
        dispatch_sync(dispatch_get_main_queue(), work);
    }
    return result;
}

std::string openFile(const std::string& title, const std::string& defaultDir,
                      const std::string& extensionNoDot) {
    __block std::string result;
    auto work = ^{
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        if (!title.empty()) {
            [panel setTitle:toNSString(title)];
        }
        [panel setCanChooseFiles:YES];
        [panel setCanChooseDirectories:NO];
        [panel setAllowsMultipleSelection:NO];
        applyDirectoryAndFilter(panel, defaultDir, extensionNoDot);

        [NSApp activateIgnoringOtherApps:YES];
        ModalTrackingPump pump;
        NSInteger response = [panel runModal];
        if (response == NSModalResponseOK && panel.URL) {
            result = fromNSString(panel.URL.path);
        }
    };
    if ([NSThread isMainThread]) {
        work();
    } else {
        dispatch_sync(dispatch_get_main_queue(), work);
    }
    return result;
}

std::vector<std::string> openFiles(const std::string& title, const std::string& defaultDir) {
    __block std::vector<std::string> result;
    auto work = ^{
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        if (!title.empty()) {
            [panel setTitle:toNSString(title)];
        }
        [panel setCanChooseFiles:YES];
        [panel setCanChooseDirectories:NO];
        [panel setAllowsMultipleSelection:YES];
        applyDirectoryAndFilter(panel, defaultDir, "");

        [NSApp activateIgnoringOtherApps:YES];
        ModalTrackingPump pump;
        NSInteger response = [panel runModal];
        if (response == NSModalResponseOK) {
            for (NSURL* url in panel.URLs) {
                result.push_back(fromNSString(url.path));
            }
        }
    };
    if ([NSThread isMainThread]) {
        work();
    } else {
        dispatch_sync(dispatch_get_main_queue(), work);
    }
    return result;
}

std::string chooseFolder(const std::string& title, const std::string& defaultDir) {
    __block std::string result;
    auto work = ^{
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        if (!title.empty()) {
            [panel setTitle:toNSString(title)];
        }
        [panel setCanChooseFiles:NO];
        [panel setCanChooseDirectories:YES];
        [panel setAllowsMultipleSelection:NO];
        applyDirectoryAndFilter(panel, defaultDir, "");

        [NSApp activateIgnoringOtherApps:YES];
        ModalTrackingPump pump;
        NSInteger response = [panel runModal];
        if (response == NSModalResponseOK && panel.URL) {
            result = fromNSString(panel.URL.path);
        }
    };
    if ([NSThread isMainThread]) {
        work();
    } else {
        dispatch_sync(dispatch_get_main_queue(), work);
    }
    return result;
}

} // namespace MacFileDialogs

#endif // __APPLE__
