#ifdef __APPLE__

#include "MacFileDialogs.h"
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
