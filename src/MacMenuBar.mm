#ifdef __APPLE__

#include "MacMenuBar.h"
#import <Cocoa/Cocoa.h>
#include "MacMenuActions.h"

// room tags used on Rooms menu items — matches ROOMMENU_OPTION in program.h
// (0=Mix, 1=Bins, 2=Style, 3=Gen, 4=Segment). Kept as plain ints here rather
// than including program.h's enum directly, to stay decoupled from it (see
// MacMenuActions.h for why).
enum { EWOC_ROOM_MIX = 0, EWOC_ROOM_BINS = 1, EWOC_ROOM_STYLE = 2, EWOC_ROOM_GEN = 3, EWOC_ROOM_SEGMENT = 4 };

// Layer-slot submenus (New/Open/Save As > Layer in Deck A/B) are dynamic —
// rebuilt from the current per-deck layer count right before display, same
// as the Beatmatch Device submenu below. NSMenu carries no tag of its own,
// so -menuNeedsUpdate: identifies which of these submenus is asking by
// pointer identity against these globals; deck+slot are packed into each
// generated item's own tag (deck*10000 + slot) instead, since the menu's
// action selector already tells us the "kind" (new/open-layer/open-queue/save).
static NSMenu* g_newLayerMenu[2] = {nil, nil};
static NSMenu* g_openLayerMenu[2] = {nil, nil};
static NSMenu* g_openQueueMenu[2] = {nil, nil};
static NSMenu* g_saveLayerMenu[2] = {nil, nil};

// Target/action handler for every native menu item. A single Objective-C
// object rather than one per item — items are disambiguated either by
// separate action selectors (New/Open/Save's Project/Mix/Deck A/Deck B,
// small fixed sets) or by tag (Rooms, Beatmatch device, layer slots —
// data-driven lists).
@interface EWOCMenuTarget : NSObject <NSMenuDelegate>
- (void)newProject:(id)sender;
- (void)newMix:(id)sender;
- (void)newDeckA:(id)sender;
- (void)newDeckB:(id)sender;
- (void)openProject:(id)sender;
- (void)openMix:(id)sender;
- (void)openDeckA:(id)sender;
- (void)openDeckB:(id)sender;
- (void)saveProjectAs:(id)sender;
- (void)saveMixAs:(id)sender;
- (void)saveDeckAAs:(id)sender;
- (void)saveDeckBAs:(id)sender;
- (void)saveProject:(id)sender;
- (void)quit:(id)sender;
- (void)preferences:(id)sender;
- (void)configureMIDI:(id)sender;
- (void)selectRoom:(id)sender;
- (void)selectBeatmatchDevice:(id)sender;
- (void)documentation:(id)sender;
- (void)newLayerSlot:(id)sender;
- (void)openLayerSlot:(id)sender;
- (void)openQueueSlot:(id)sender;
- (void)saveLayerSlot:(id)sender;
- (void)rebuildLayerSlotMenu:(NSMenu *)menu deck:(int)deck action:(SEL)action includeNewSlot:(BOOL)includeNewSlot;
@end

@implementation EWOCMenuTarget

- (void)newProject:(id)sender { EWOCMenuActions::newProject(); }
- (void)newMix:(id)sender { EWOCMenuActions::newMix(); }
- (void)newDeckA:(id)sender { EWOCMenuActions::newDeckA(); }
- (void)newDeckB:(id)sender { EWOCMenuActions::newDeckB(); }
- (void)openProject:(id)sender { EWOCMenuActions::openProject(); }
- (void)openMix:(id)sender { EWOCMenuActions::openMix(); }
- (void)openDeckA:(id)sender { EWOCMenuActions::openDeckA(); }
- (void)openDeckB:(id)sender { EWOCMenuActions::openDeckB(); }
- (void)saveProjectAs:(id)sender { EWOCMenuActions::saveProjectAs(); }
- (void)saveMixAs:(id)sender { EWOCMenuActions::saveMix(); }
- (void)saveDeckAAs:(id)sender { EWOCMenuActions::saveDeckA(); }
- (void)saveDeckBAs:(id)sender { EWOCMenuActions::saveDeckB(); }
- (void)saveProject:(id)sender { EWOCMenuActions::saveProject(); }
- (void)quit:(id)sender { EWOCMenuActions::quit(); }
- (void)preferences:(id)sender { EWOCMenuActions::preferences(); }
- (void)configureMIDI:(id)sender { EWOCMenuActions::configureMIDI(); }
- (void)documentation:(id)sender { EWOCMenuActions::documentation(); }

- (void)selectRoom:(id)sender {
    NSMenuItem* item = (NSMenuItem*)sender;
    EWOCMenuActions::switchRoom((int)item.tag);
}

- (void)selectBeatmatchDevice:(id)sender {
    NSMenuItem* item = (NSMenuItem*)sender;
    EWOCMenuActions::setBeatmatchDevice((int)item.tag);
}

- (void)newLayerSlot:(id)sender {
    NSMenuItem* item = (NSMenuItem*)sender;
    EWOCMenuActions::newLayerInDeck((int)(item.tag / 10000), (int)(item.tag % 10000));
}

- (void)openLayerSlot:(id)sender {
    NSMenuItem* item = (NSMenuItem*)sender;
    EWOCMenuActions::openFilesIntoLayer((int)(item.tag / 10000), (int)(item.tag % 10000));
}

- (void)openQueueSlot:(id)sender {
    NSMenuItem* item = (NSMenuItem*)sender;
    EWOCMenuActions::openFilesIntoQueue((int)(item.tag / 10000), (int)(item.tag % 10000));
}

- (void)saveLayerSlot:(id)sender {
    NSMenuItem* item = (NSMenuItem*)sender;
    EWOCMenuActions::saveLayerAs((int)(item.tag / 10000), (int)(item.tag % 10000));
}

// NSMenuValidation — Cocoa calls this for every item right before a menu is
// shown, letting Rooms items grey out for the currently-active room or an
// AI feature (Style/Gen/Segment) that isn't installed, without needing to
// rebuild the menu each time.
- (BOOL)validateMenuItem:(NSMenuItem *)menuItem {
    if (menuItem.action == @selector(selectRoom:)) {
        return EWOCMenuActions::canSwitchToRoom((int)menuItem.tag) ? YES : NO;
    }
    return YES;
}

// Shared rebuild for the 8 layer-slot submenus (New/Open/Save As x Deck A/B):
// "Layer Slot 1".."Layer Slot N" for existing layers, plus a trailing "New
// Layer Slot" entry when includeNewSlot is set (New and Open can target a
// not-yet-created layer; Save As can only target an existing one).
- (void)rebuildLayerSlotMenu:(NSMenu *)menu deck:(int)deck action:(SEL)action includeNewSlot:(BOOL)includeNewSlot {
    [menu removeAllItems];
    int count = EWOCMenuActions::layerCount(deck);
    int upper = includeNewSlot ? count + 1 : count;
    for (int slot = 0; slot < upper; slot++) {
        NSString* title = (includeNewSlot && slot == count)
            ? @"New Layer Slot"
            : [NSString stringWithFormat:@"Layer Slot %d", slot + 1];
        NSMenuItem* item = [menu addItemWithTitle:title action:action keyEquivalent:@""];
        item.target = self;
        item.tag = deck * 10000 + slot;
    }
    if (upper == 0) {
        NSMenuItem* item = [menu addItemWithTitle:@"No layers" action:nil keyEquivalent:@""];
        item.enabled = NO;
    }
}

// NSMenuDelegate — the Beatmatch Device submenu and the 8 layer-slot
// submenus are all rebuilt right before display, since the data behind them
// (audio devices, per-deck layer counts) can change between menu opens.
- (void)menuNeedsUpdate:(NSMenu *)menu {
    for (int deck = 0; deck < 2; deck++) {
        if (menu == g_newLayerMenu[deck]) {
            [self rebuildLayerSlotMenu:menu deck:deck action:@selector(newLayerSlot:) includeNewSlot:YES];
            return;
        }
        if (menu == g_openLayerMenu[deck]) {
            [self rebuildLayerSlotMenu:menu deck:deck action:@selector(openLayerSlot:) includeNewSlot:YES];
            return;
        }
        if (menu == g_openQueueMenu[deck]) {
            [self rebuildLayerSlotMenu:menu deck:deck action:@selector(openQueueSlot:) includeNewSlot:YES];
            return;
        }
        if (menu == g_saveLayerMenu[deck]) {
            [self rebuildLayerSlotMenu:menu deck:deck action:@selector(saveLayerSlot:) includeNewSlot:NO];
            return;
        }
    }

    [menu removeAllItems];
    std::vector<std::string> devices = EWOCMenuActions::audioDeviceNames();
    std::string current = EWOCMenuActions::currentAudioDevice();
    for (int i = 0; i < (int)devices.size(); i++) {
        NSString* title = [NSString stringWithUTF8String:devices[i].c_str()];
        NSMenuItem* item = [menu addItemWithTitle:title action:@selector(selectBeatmatchDevice:) keyEquivalent:@""];
        item.target = self;
        item.tag = i;
        if (devices[i] == current) {
            item.state = NSControlStateValueOn;
        }
    }
    if (devices.empty()) {
        NSMenuItem* item = [menu addItemWithTitle:@"No input devices found" action:nil keyEquivalent:@""];
        item.enabled = NO;
    }
}

@end

namespace MacMenuBar {

static EWOCMenuTarget* g_target = nil;
static NSTimer* g_trackingPumpTimer = nil;

static NSMenuItem* addItem(NSMenu* menu, NSString* title, SEL action, NSString* key = @"") {
    NSMenuItem* item = [menu addItemWithTitle:title action:action keyEquivalent:key];
    item.target = g_target;
    return item;
}

// NSMenu tracking (any menu open, including submenus) runs its own nested
// run loop in NSEventTrackingRunLoopMode that blocks the main thread until
// the menu is dismissed — with a single-threaded SDL render loop, that would
// otherwise freeze the live video mix entirely for as long as a menu stays
// open. Standard Cocoa fix: schedule a timer that only runs in that mode, so
// it keeps firing while (and only while) a menu is being tracked.
static void startTrackingPump() {
    if (g_trackingPumpTimer) return;
    g_trackingPumpTimer = [NSTimer timerWithTimeInterval:1.0 / 60.0
                                                   repeats:YES
                                                     block:^(NSTimer * _Nonnull timer) {
        EWOCMenuActions::pumpFrameDuringMenuTracking();
    }];
    [[NSRunLoop currentRunLoop] addTimer:g_trackingPumpTimer forMode:NSEventTrackingRunLoopMode];
}

static void stopTrackingPump() {
    [g_trackingPumpTimer invalidate];
    g_trackingPumpTimer = nil;
}

void install() {
    g_target = [[EWOCMenuTarget alloc] init];

    [[NSNotificationCenter defaultCenter] addObserverForName:NSMenuDidBeginTrackingNotification
                                                        object:nil
                                                         queue:[NSOperationQueue mainQueue]
                                                    usingBlock:^(NSNotification * _Nonnull note) {
        startTrackingPump();
    }];
    [[NSNotificationCenter defaultCenter] addObserverForName:NSMenuDidEndTrackingNotification
                                                        object:nil
                                                         queue:[NSOperationQueue mainQueue]
                                                    usingBlock:^(NSNotification * _Nonnull note) {
        stopTrackingPump();
    }];

    NSMenu* mainMenu = [[NSMenu alloc] initWithTitle:@""];

    // --- Application menu (bold app name) ---
    NSMenuItem* appMenuItem = [[NSMenuItem alloc] init];
    [mainMenu addItem:appMenuItem];
    NSMenu* appMenu = [[NSMenu alloc] initWithTitle:@"EWOCvj2"];
    appMenuItem.submenu = appMenu;
    addItem(appMenu, @"Preferences…", @selector(preferences:), @",");
    [appMenu addItem:[NSMenuItem separatorItem]];
    // The standard "Quit EWOCvj2" item and its Cmd+Q key equivalent both
    // route through this one action — routed to the app's own quit flow
    // (mainprogram->quitting = "quitted", the same flag the existing
    // window-close-button and custom File>Quit menu already use to trigger
    // the save-confirmation dialog and orderly shutdown) instead of
    // NSApp terminate:, which would skip straight past both.
    addItem(appMenu, @"Quit EWOCvj2", @selector(quit:), @"q");

    // --- File menu ---
    NSMenuItem* fileMenuItem = [[NSMenuItem alloc] init];
    [mainMenu addItem:fileMenuItem];
    NSMenu* fileMenu = [[NSMenu alloc] initWithTitle:@"File"];
    fileMenuItem.submenu = fileMenu;

    NSMenuItem* newItem = [fileMenu addItemWithTitle:@"New" action:nil keyEquivalent:@""];
    NSMenu* newMenu = [[NSMenu alloc] initWithTitle:@"New"];
    newItem.submenu = newMenu;
    addItem(newMenu, @"Project", @selector(newProject:));
    addItem(newMenu, @"Mix", @selector(newMix:));
    addItem(newMenu, @"Deck A", @selector(newDeckA:));
    addItem(newMenu, @"Deck B", @selector(newDeckB:));
    [newMenu addItem:[NSMenuItem separatorItem]];
    NSMenuItem* newLayerAItem = [newMenu addItemWithTitle:@"Layer in Deck A" action:nil keyEquivalent:@""];
    g_newLayerMenu[0] = [[NSMenu alloc] initWithTitle:@"Layer in Deck A"];
    g_newLayerMenu[0].delegate = g_target;
    newLayerAItem.submenu = g_newLayerMenu[0];
    NSMenuItem* newLayerBItem = [newMenu addItemWithTitle:@"Layer in Deck B" action:nil keyEquivalent:@""];
    g_newLayerMenu[1] = [[NSMenu alloc] initWithTitle:@"Layer in Deck B"];
    g_newLayerMenu[1].delegate = g_target;
    newLayerBItem.submenu = g_newLayerMenu[1];

    NSMenuItem* openItem = [fileMenu addItemWithTitle:@"Open" action:nil keyEquivalent:@""];
    NSMenu* openMenu = [[NSMenu alloc] initWithTitle:@"Open"];
    openItem.submenu = openMenu;
    addItem(openMenu, @"Project…", @selector(openProject:), @"o");
    addItem(openMenu, @"Mix…", @selector(openMix:));
    addItem(openMenu, @"Deck A…", @selector(openDeckA:));
    addItem(openMenu, @"Deck B…", @selector(openDeckB:));
    [openMenu addItem:[NSMenuItem separatorItem]];
    NSMenuItem* openLayerAItem = [openMenu addItemWithTitle:@"Files into Layer, Deck A" action:nil keyEquivalent:@""];
    g_openLayerMenu[0] = [[NSMenu alloc] initWithTitle:@"Files into Layer, Deck A"];
    g_openLayerMenu[0].delegate = g_target;
    openLayerAItem.submenu = g_openLayerMenu[0];
    NSMenuItem* openLayerBItem = [openMenu addItemWithTitle:@"Files into Layer, Deck B" action:nil keyEquivalent:@""];
    g_openLayerMenu[1] = [[NSMenu alloc] initWithTitle:@"Files into Layer, Deck B"];
    g_openLayerMenu[1].delegate = g_target;
    openLayerBItem.submenu = g_openLayerMenu[1];
    NSMenuItem* openQueueAItem = [openMenu addItemWithTitle:@"Files into Queue, Deck A" action:nil keyEquivalent:@""];
    g_openQueueMenu[0] = [[NSMenu alloc] initWithTitle:@"Files into Queue, Deck A"];
    g_openQueueMenu[0].delegate = g_target;
    openQueueAItem.submenu = g_openQueueMenu[0];
    NSMenuItem* openQueueBItem = [openMenu addItemWithTitle:@"Files into Queue, Deck B" action:nil keyEquivalent:@""];
    g_openQueueMenu[1] = [[NSMenu alloc] initWithTitle:@"Files into Queue, Deck B"];
    g_openQueueMenu[1].delegate = g_target;
    openQueueBItem.submenu = g_openQueueMenu[1];

    NSMenuItem* saveAsItem = [fileMenu addItemWithTitle:@"Save As" action:nil keyEquivalent:@""];
    NSMenu* saveAsMenu = [[NSMenu alloc] initWithTitle:@"Save As"];
    saveAsItem.submenu = saveAsMenu;
    addItem(saveAsMenu, @"Project…", @selector(saveProjectAs:), @"S");
    addItem(saveAsMenu, @"Mix…", @selector(saveMixAs:));
    addItem(saveAsMenu, @"Deck A…", @selector(saveDeckAAs:));
    addItem(saveAsMenu, @"Deck B…", @selector(saveDeckBAs:));
    [saveAsMenu addItem:[NSMenuItem separatorItem]];
    NSMenuItem* saveLayerAItem = [saveAsMenu addItemWithTitle:@"Layer in Deck A" action:nil keyEquivalent:@""];
    g_saveLayerMenu[0] = [[NSMenu alloc] initWithTitle:@"Layer in Deck A"];
    g_saveLayerMenu[0].delegate = g_target;
    saveLayerAItem.submenu = g_saveLayerMenu[0];
    NSMenuItem* saveLayerBItem = [saveAsMenu addItemWithTitle:@"Layer in Deck B" action:nil keyEquivalent:@""];
    g_saveLayerMenu[1] = [[NSMenu alloc] initWithTitle:@"Layer in Deck B"];
    g_saveLayerMenu[1].delegate = g_target;
    saveLayerBItem.submenu = g_saveLayerMenu[1];

    [fileMenu addItem:[NSMenuItem separatorItem]];
    addItem(fileMenu, @"Save Project", @selector(saveProject:), @"s");

    // --- Configure menu (matches the app's own in-app label for this menu) ---
    NSMenuItem* configureMenuItem = [[NSMenuItem alloc] init];
    [mainMenu addItem:configureMenuItem];
    NSMenu* configureMenu = [[NSMenu alloc] initWithTitle:@"Configure"];
    configureMenuItem.submenu = configureMenu;
    addItem(configureMenu, @"Preferences…", @selector(preferences:));
    addItem(configureMenu, @"Configure General MIDI", @selector(configureMIDI:));

    NSMenuItem* beatmatchItem = [configureMenu addItemWithTitle:@"Beatmatch Device" action:nil keyEquivalent:@""];
    NSMenu* beatmatchMenu = [[NSMenu alloc] initWithTitle:@"Beatmatch Device"];
    beatmatchItem.submenu = beatmatchMenu;
    beatmatchMenu.delegate = g_target;

    // --- Rooms menu ---
    NSMenuItem* roomsMenuItem = [[NSMenuItem alloc] init];
    [mainMenu addItem:roomsMenuItem];
    NSMenu* roomsMenu = [[NSMenu alloc] initWithTitle:@"Rooms"];
    roomsMenuItem.submenu = roomsMenu;
    struct { NSString* title; NSInteger room; } rooms[] = {
        {@"Mix Room", EWOC_ROOM_MIX},
        {@"Bins Room", EWOC_ROOM_BINS},
        {@"Style Room", EWOC_ROOM_STYLE},
        {@"Gen Room", EWOC_ROOM_GEN},
        {@"Segment Room", EWOC_ROOM_SEGMENT},
    };
    for (auto& r : rooms) {
        NSMenuItem* item = addItem(roomsMenu, r.title, @selector(selectRoom:));
        item.tag = r.room;
    }

    // --- Help menu ---
    NSMenuItem* helpMenuItem = [[NSMenuItem alloc] init];
    [mainMenu addItem:helpMenuItem];
    NSMenu* helpMenu = [[NSMenu alloc] initWithTitle:@"Help"];
    helpMenuItem.submenu = helpMenu;
    addItem(helpMenu, @"EWOCvj2 Documentation", @selector(documentation:));

    [NSApp setMainMenu:mainMenu];
    [NSApp setHelpMenu:helpMenu];
}

} // namespace MacMenuBar

#endif // __APPLE__
