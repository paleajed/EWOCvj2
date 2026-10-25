---
sidebar_position: 9
title: OSC control
---

# OSC control

### 

### Intro

EWOCvj2 can be remote controlled over OSC (Open Sound Control), by controller apps such as TouchOSC or Lemur, by software like Max, Pure Data or TouchDesigner, or by anything else that can send OSC messages over UDP.  By default the program listens for OSC messages on UDP port 9000 from the moment it starts; see "Preferences" below to change that.

The program also sends its state back to your controller, so faders, buttons and displays on a controller stay in sync with what happens in EWOCvj2, also when you change things with the mouse or MIDI.  See "Feedback" below.

This page only describes the OSC addresses.  What the functions themselves do is described in the other pages of this documentation.

### 

### Preferences

Everything about OSC itself is set in the "OSC" tab of the Preferences window, see [Preferences](mixroom.md#preferences), and takes effect when the preferences are saved:

- "OSC Control": turns OSC on or off.
- "OSC port" and "OSC Feedback port": the ports the program listens on and sends feedback to (9000 and 9001 by default).  The feedback port is used for the feedback that goes to the computer that sent the first message; a `/osc/target` message with its own port (see "Feedback") keeps that port.
- "Localhost only": only accept messages from this computer.
- "OSC Password" and "Safe mode": see "Security" below.
- "Old hardware": slower, less detailed feedback for slow computers or controllers, see "Feedback".

### 

### Addresses and values

All addresses are case sensitive.  In the lists below:

- `<modus>` is either `preview` (the preview modus, with its layers and mix) or `output` (the output modus).

- `<deck>` is either `deckA` or `deckB`.

- `N` and `K` are numbers, counting from 1.  `N` is the position of the layer in its deck, `K` the position of the effect in the effect chain of the layer.  Layers and effects are looked up at the moment a message arrives, so when you add, remove or reorder layers, the numbers shift along with them.

- `<layer prefix>` is `/mix/<modus>/<deck>/layer/N`, for example `/mix/output/deckA/layer/2`.

Value types:

- **0-1**: a float between 0 and 1.  For parameters that have another range inside the program (as most effect parameters do), this is stretched over their range automatically.  Values outside 0-1 are clamped.

- **raw**: a float that is used as is, within the range of the parameter (speed 0-6 for instance).

- **0/1**: an integer (or boolean) switch.  Sending 1 turns the thing on, 0 turns it off.  When these are sent without any argument at all, they toggle instead.

- **trigger**: fires when sent without argument, or with a non-zero value.  A value of 0 is ignored, so push buttons that send 1 on press and 0 on release work as expected.

Many functions only work on the modus that is on screen (preview in preview modus, otherwise output), like the menus they stand for; for the other modus the message is ignored.  Messages that can't be carried out (a file that isn't found, something that is busy) are ignored and a note is printed to the log.

Names in addresses and arguments are written in lower case, with every character that is not a letter or digit replaced by an underscore (`Push/Pull` becomes `push_pull`).  Arguments that are names are matched in the same way, so case and punctuation don't matter there.  Plugins (effects, mixers, wipes, sources) that have equal names are numbered in the menus (`EMBOSS`, `EMBOSS 2`), and that number is part of the name (`emboss_2`).  The names to use are always the names in the menus; `/list/...` requests (see "Names, counts and lists") give them.

### 

### Mix

| Address                                                                               | Value                                  | What it does                                                                                                                          |
| ------------------------------------------------------------------------------------- | -------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------- |
| `/mix/crossfade_all`                                                                  | 0-1                                    | sets the crossfader of both preview and output                                                                                        |
| `/mix/<modus>/crossfade`                                                              | 0-1                                    | sets the crossfader of that set                                                                                                       |
| `/mix/<modus>/wipe/type`                                                              | string or integer                      | selects the mix wipe, by name or number                                                                                               |
| `/mix/<modus>/wipe/dir`                                                               | integer                                | sets the wipe direction (counting from 0 in the order of the direction menu of that wipe)                                             |
| `/mix/<modus>/wipe/xpos`, `/mix/<modus>/wipe/ypos`                                    | 0-1                                    | sets the wipe position                                                                                                                |
| `/mix/<modus>/wipe/<parameter>`, `/mix/<modus>/wipe/param/J`                          | 0-1                                    | parameter of the ISF wipe, when one is selected (shared by both modi)                                                                 |
| `/mix/<modus>/<deck>/scroll`                                                          | integer                                | scrolls the layer stack of the deck to start at this layer (from 1)                                                                   |
| `/mix/<modus>/<deck>/genmidi`                                                         | integer 0-4 or string                  | sets the general MIDI preset of the whole deck: 0 or `off`, 1 or `A`, 2 or `B`, 3 or `C`, 4 or `D`                                    |
| `/mix/<modus>/<deck>/speed`                                                           | raw                                    | sets the speed factor of the whole deck, as the number shown on the deck speed slider (0 to about 11.1)                               |
| `/mix/<modus>/<deck>/maskup`                                                          | trigger                                | goes up one level in the mask hierarchy of the deck                                                                                   |
| `/mix/output/record`                                                                  | 0/1, "start" or "stop"                 | starts/stops recording the output to a HAP video file                                                                                 |
| `/mix/<modus>/current`                                                                | one or more strings, like "A1" "B2"    | sets the current layers of that modus: each string is a deck letter and a layer position; the last one becomes the current layer      |
| `/mix/<modus>/exchange`                                                               | three strings: operation, layer, layer | reorders layers: `"swap" "A2" "B1"` swaps those two layers, `"insert" "A2" "B1"` takes A2 out of its stack and puts it in front of B1 |
| `/mix/display/deckA`, `/mix/display/deckB`, `/mix/display/mix`, `/mix/display/output` | integer                                | shows that monitor on another display; the same message again closes it                                                               |
| `/mix/aistyle/deckA`, `/mix/aistyle/deckB`, `/mix/aistyle/mix`, `/mix/aistyle/output` | string                                 | runs the named AI style over that monitor, `none` (or `off`) switches it off                                                          |
| `/mix/fullscreen/deckA`, `/mix/fullscreen/deckB`                                      | 0/1                                    | shows the deck A or deck B monitor full screen (1) or stops it (0)                                                                    |
| `/mix/fullscreen/mix`                                                                 | 0/1                                    | the same for the mix monitor of the modus that is on screen                                                                           |
| `/mix/fullscreen/output`                                                              | 0/1                                    | the same for the output mix monitor (preview modus only)                                                                              |
| `/mix/modus`                                                                          | 1/0, "preview" or "live"               | switches between preview modus (1) and live modus (0); without argument it toggles                                                    |
| `/mix/scene/deckA`, `/mix/scene/deckB`                                                | integer 1-4                            | switches that deck to the given scene                                                                                                 |
| `/mix/scene`                                                                          | integer 1-4                            | switches both decks to the given scene                                                                                                |
| `/mix/scene/deckA/send/up`, `/mix/scene/deckB/send/up`                                | integer 1-4                            | copies the preview modus of that deck to the given scene                                                                              |
| `/mix/scene/deckA/send/down`, `/mix/scene/deckB/send/down`                            | integer 1-4                            | copies the given scene back to the preview modus of that deck                                                                         |
| `/mix/send/up`                                                                        | trigger                                | copies the preview modus to the output modus (both decks)                                                                             |
| `/mix/send/up/a`, `/b`, `/mix`                                                        | trigger                                | same, for only deck A, only deck B, or the mix (both decks)                                                                           |
| `/mix/send/down`                                                                      | trigger                                | copies the output modus back to the preview modus (both decks)                                                                        |
| `/mix/send/down/a`, `/b`, `/mix`                                                      | trigger                                | same, for only deck A, only deck B, or the mix (both decks)                                                                           |

Notes:

- `scroll` is limited to the layers that exist, and is kept separately for the preview and the output modus.

- `current`: layers that don't exist are skipped, and nothing changes when none of the given layers exist.  `select` on a layer (see "Layers") is the short form for a single layer.  There is no feedback for the current layers.

- `exchange`: the layers are given as a deck letter and a position, like `"A2"`; they can be in the same deck or in different decks.  To insert a layer at the end of a stack, give the position behind the last layer: `"insert" "A1" "B4"` puts layer A1 at the end of deck B when deck B has three layers.  Inserting or swapping a layer with itself, or with a position that doesn't exist, does nothing.  Positions are counted at the moment the message arrives, so after an `insert` the positions of the layers behind it have changed.  It isn't done while a scene change or file loading is busy.  For a deck that is in mask edit mode, the positions are those of the masks shown in its stack.

- `display`: the numbers count the displays in the order of the "Show on display" submenu, which lists all displays except the one the program is on, so `1` is the first one in that list (`/list/displays`).  Sending a different monitor to a display that already shows one replaces it.

- `aistyle`: the names are those of the installed styles.  Choosing the style that is already on does nothing.  Feedback reports the number of the style from 1, and 0 for none.

- `fullscreen`: there is one full screen display at a time, so starting one replaces the one that was on, and sending `0` only stops it when it is that one that is shown.  Without argument the address toggles.  In live modus there is only the mix monitor, and `/mix/fullscreen/output` is ignored.

- `modus` only works while the mix room is on screen; a request made in another room is carried out when you return to the mix room.

- Scenes: a scene switch that arrives while another one is still being swapped in is held and applied right after it, and when several arrive for the same deck in the meantime, the last one wins.  The scene send addresses only work for a scene the deck is not currently on, and only while the mix room is on screen; a send that arrives while a scene swap is busy, or while you are in another room, is held until the mix room can handle it.

- Wipe names are those of the wipe menu: CROSSFADE, CLASSIC, PUSH/PULL, SQUASHED, ELLIPSE, RECTANGLE, ZOOMED RECTANGLE, CLOCK, DOUBLE CLOCK, BARS, PATTERN, DOT and REPEL, followed by the ISF transitions that are available as wipes.  An unknown name is ignored.  By number, CROSSFADE is -1, CLASSIC is 0, PUSH/PULL is 1 and so on in the order above (ISF wipes can only be selected by name).

- `record` ends up in the recordings directory of the project.

#### 

#### Masks

| Address                              | Value   | What it does                                                        |
| ------------------------------------ | ------- | ------------------------------------------------------------------- |
| `<layer prefix>/maskedit`            | trigger | enters mask edit mode for the masks of the layer                    |
| `<layer prefix>/masked`              | 0/1     | switches on/off whether the mask influences the layer               |
| `<layer prefix>/maskscroll`          | integer | scrolls the mask stack of the layer to start at this mask (from 1)  |
| `<layer prefix>/effect/K/maskedit`   | trigger | enters mask edit mode for the masks of effect K                     |
| `<layer prefix>/effect/K/masked`     | 0/1     | switches on/off whether the masks influence the effect              |
| `<layer prefix>/effect/K/maskscroll` | integer | scrolls the mask stack of the effect to start at this mask (from 1) |
| `<layer prefix>/mask/M/...`          |         | any of the layer controls, for mask M of the layer                  |
| `<layer prefix>/effect/K/mask/M/...` |         | the same for mask M of effect K                                     |
| `/mix/<modus>/<deck>/maskup`         | trigger | goes up one level in the mask hierarchy of the deck                 |

Masks are layers too, with all the controls of a layer, reached through the layer or effect that owns them: `/mix/preview/deckA/layer/2/mask/1/opacity` is the opacity of the first mask of layer 2 and `.../layer/2/effect/1/mask/2/play` starts the second mask of the first effect.  Masks can have masks of their own: `.../layer/2/mask/1/maskedit` goes into the masks of the first mask, and `.../layer/2/mask/1/mask/1/...` addresses those.  `maskup` is the way back out.  The feedback `/mix/<modus>/<deck>/maskdepth` tells how deep a deck is: 0 for the normal layer stack, 1 inside the masks of a layer or effect, 2 inside the masks of a mask, and so on.

`maskedit` makes a first mask when there is none (this needs the SOLID COLOR source plugin).  The effect version doesn't work for a layer with a clip queue that doesn't keep its effects, as in the program.  `maskscroll` is limited to the masks that exist and does nothing without masks.

### 

### Layers

All layer addresses start with the layer prefix `/mix/<modus>/<deck>/layer/N/`, for example `/mix/output/deckA/layer/2/opacity`.

| Address (after the layer prefix)       | Value                          | What it does                                                                                                                                        |
| -------------------------------------- | ------------------------------ | --------------------------------------------------------------------------------------------------------------------------------------------------- |
| `opacity`                              | 0-1                            | layer opacity                                                                                                                                       |
| `volume`                               | 0-1                            | layer audio volume                                                                                                                                  |
| `speed`                                | raw                            | playing speed                                                                                                                                       |
| `scale`                                | raw                            | layer scale                                                                                                                                         |
| `shiftx`, `shifty`                     | raw                            | layer position shift                                                                                                                                |
| `loopbox/start`                        | 0-1                            | moves the loop start marker (L) to this position in the video                                                                                       |
| `loopbox/end`                          | 0-1                            | moves the loop end marker (P) to this position in the video                                                                                         |
| `loopbox/setstart`                     | trigger                        | sets the loop start to the current frame                                                                                                            |
| `loopbox/setend`                       | trigger                        | sets the loop end to the current frame                                                                                                              |
| `loopbox/copyduration`                 | trigger                        | copies the duration of the loop                                                                                                                     |
| `loopbox/pasteduration`                | trigger                        | gives the loop the copied duration by changing the speed                                                                                            |
| `loopbox/pastelength`                  | trigger                        | gives the loop the copied duration by changing the length of the loop                                                                               |
| `loopbeats`                            | float                          | beatmatches the loop of the layer to this many beats (0.5, 1, 2, 4, ...), 0 turns it off                                                            |
| `position`                             | 0-1                            | scrubs to this position inside the video (0 = first frame, 1 = last frame), as dragging in the loopbox does                                         |
| `play`                                 | 0/1                            | play forward                                                                                                                                        |
| `reverse`                              | 0/1                            | play backward                                                                                                                                       |
| `bounce`                               | 0/1                            | bounce playing                                                                                                                                      |
| `loop`                                 | 0/1                            | loop on/off                                                                                                                                         |
| `stop`                                 | trigger                        | stops playing                                                                                                                                       |
| `pause`                                | trigger                        | pauses playing; triggering again resumes in the same way and direction                                                                              |
| `frame/forward`                        | trigger                        | steps one frame forward                                                                                                                             |
| `frame/backward`                       | trigger                        | steps one frame backward                                                                                                                            |
| `load`                                 | string                         | opens the file at the given path into the layer (a single path)                                                                                     |
| `webcam`                               | string or number               | connects the layer to a webcam, by name or by its 1-based number in `/webcam/sources`                                                               |
| `save`                                 | string                         | saves the layer to a layer file at the path                                                                                                         |
| `select`                               | trigger                        | makes this layer the only current layer                                                                                                             |
| `source`                               | string                         | turns the layer into a source plugin layer, by plugin name                                                                                          |
| `source/<parameter>`, `source/param/J` | 0-1                            | sets a parameter of the source plugin of the layer                                                                                                  |
| `mixmode`                              | string or integer              | how the layer is mixed with the layers below it, by name or number                                                                                  |
| `mixfactor`                            | 0-1                            | the mix factor slider of the layer                                                                                                                  |
| `mixer/<parameter>`, `mixer/param/J`   | 0-1                            | sets a parameter of the mixer plugin that mixes the layer                                                                                           |
| `wipe/type`                            | string or integer              | uses a wipe to mix the layer with the layers below it, by name or number                                                                            |
| `wipe/dir`                             | integer                        | the wipe direction                                                                                                                                  |
| `wipe/xpos`, `wipe/ypos`               | 0-1                            | the wipe position                                                                                                                                   |
| `key/tolerance`                        | 0-1                            | key tolerance                                                                                                                                       |
| `key/feather`                          | 0-1                            | key feather                                                                                                                                         |
| `key/direction`                        | 0/1                            | the `D` button of the key settings                                                                                                                  |
| `key/invert`                           | 0/1                            | the `I` button of the key settings                                                                                                                  |
| `key/color`                            | three numbers 0-1, or a string | the key colour: red, green and blue, or a hex colour like `"#00ff00"`                                                                               |
| `add`                                  | trigger                        | the `+` button of the layer: adds a new layer behind this one                                                                                       |
| `addbefore`                            | trigger                        | the extra `+` button left of the first layer: adds a new layer in front of it (first layer of a stack only)                                         |
| `duplicate`                            | trigger                        | duplicates the layer                                                                                                                                |
| `clone`                                | trigger                        | makes a clone of the layer                                                                                                                          |
| `recordreplace`                        | 0/1                            | records the layer to a HAP video file and replaces the layer by the recording when done; 0 stops the recording; without argument it starts or stops |
| `hapencode`                            | trigger                        | encodes the video of the layer to HAP in the background                                                                                             |
| `fullscreen`                           | 0/1                            | shows the layer full screen (1) or stops it (0); without argument it toggles                                                                        |
| `display`                              | integer                        | shows the layer on another display; the same message again closes it                                                                                |
| `aspect`                               | string or integer              | the aspect ratio setting: `output` (0), `original_inside` (1) or `original_outside` (2)                                                             |
| `upscale`                              | 0/1                            | upscaling of the layer on/off                                                                                                                       |
| `sharpness`                            | 0-1                            | the sharpness amount of the upscaling                                                                                                               |
| `mute`                                 | 0/1                            | the `M` button; without argument it switches                                                                                                        |
| `solo`                                 | 0/1                            | the `S` button; without argument it switches                                                                                                        |
| `keepeffects`                          | 0/1                            | the `E` button (keep effects when another clip is loaded)                                                                                           |
| `keepmask`                             | 0/1                            | the `K` button (keep masks when another clip is loaded)                                                                                             |
| `lockspeed`                            | 0/1                            | locks the speed of the layer                                                                                                                        |
| `lockzoompan`                          | 0/1                            | locks the scale and the shift of the layer                                                                                                          |
| `genmidi`                              | integer 0-4 or string          | the general MIDI preset of this layer (0 or `off`, 1 or `A`, 2 or `B`, 3 or `C`, 4 or `D`)                                                          |
| `fxchain`                              | 0/1 or string                  | the effect stack category of the layer: layer effects (0, `layer`) or stream effects (1, `stream`)                                                  |

Notes:

- Play, reverse and bounce exclude each other, as in the program: turning one on turns the others off.

- `load` takes a single path on the machine EWOCvj2 runs on, and opens it with the same code as the file requester, so videos, images, layer files, deck files and mix files all work.  Loading takes a few frames.  The message is ignored when the file isn't found, when another file opening is still busy, or when the layer is in the modus that isn't on screen.  `save` and the other file addresses are described in "Files: layers, decks and mixes".

- `webcam` takes the name of the webcam as listed in `/webcam/sources`, or its number in that list (from 1).  `/webcam/refresh` (trigger, no layer prefix) looks for webcams and sends their names back as `/webcam/sources` to the feedback address, with every name as a string argument.  NDI sources are connected with `ndi` (see "NDI").

- `source` and `mixer`: the source plugin name is the name from the source plugin menu (`noise`, `noise_2`).  Parameters are addressed like effect parameters (see "Effects"), by name or by number from 1.  They only exist while the layer has a source plugin, or a mixer plugin as mix mode (or wipe).  The first parameter of an ISF wipe, its progress, isn't listed: use `mixfactor`.

- `mixmode` names are those of the mix mode menu: MIX, ALPHA OVER, MULTIPLY, SCREEN, OVERLAY, HARD LIGHT, SOFT LIGHT, DIVIDE, ADD, SUBTRACT, DIFFERENCE, DODGE, COLOR BURN, LINEAR BURN, VIVID LIGHT, LINEAR LIGHT, DARKEN ONLY, LIGHTEN ONLY, COLORKEY, CHROMAKEY, LUMAKEY, DISPLACEMENT and MASK, followed by the installed FFGL and ISF mixer plugins.  By number, MIX is 1, ALPHA OVER is 2 and so on in the order above (plugin mixers can only be selected by name).  Layer `wipe/type` takes the same names as the mix wipe; selecting a wipe replaces the mix mode and the other way round, and CROSSFADE means the plain MIX mode.

- `key/...` can be set at any time, but only matters while the mix mode is COLORKEY, CHROMAKEY or LUMAKEY.  For CHROMAKEY only the hue of `key/color` is used, for LUMAKEY only its brightness.  The colour is not part of the feedback.

- `loopbox/start` and `loopbox/end` can't cross each other.  The other `loopbox` addresses work like the menu entries and only work on layers of the modus that is on screen.

- `position` is a scrub: its clones follow, audio and video seek to it, and the scrub is recorded into loopstation lines that are armed for recording.  A controller fader can send it continuously.

- `loopbeats` needs the beat detection to be running (see "Beat detection").  While it is on, don't set `speed`.

- `add` and `addbefore` work on the modus that is on screen.  `add` scrolls the layer stack to the new layer when it ends up past the visible layers, as the button does.  On a mask (`mask/M/add`, in mask edit mode) they add a new mask, a layer with the SOLID COLOR source plugin.  The positions of the layers behind the new one change.

- `duplicate` and `clone` put the new layer behind the layer in the same stack, so the positions of the layers behind it change.  They work on the modus that is on screen, also in mask edit mode, where they act on the masks shown in its stack.  Masks can also be duplicated or cloned through their owner (`mask/M/duplicate`).

- `recordreplace 1` waits for the end of the loop of the layer, records one full pass and stops by itself; `recordreplace 0` stops it earlier.  It isn't possible for layers with a source plugin or a clip queue, or while another recording is busy.  The feedback is `1` while the layer is recording.

- `hapencode` is ignored for layers that can't be encoded (images, live and NDI input, source plugins, empty layers, HAP videos) and for a layer that is already encoding.  The feedback is the progress from 0 to 1, and 0 when not encoding.

- `aspect` also accepts the menu names (`same_as_output`, `inside`, `outside`).

- `lockspeed`, `lockzoompan`, `keepeffects`, `keepmask`, `mute` and `solo` are all part of the layer feedback.

- `fxchain` stores the category with the layer, and moves the switch under the layer stack when the layer is the current layer.  For the first layer of a deck it stays "Layer effects".

### 

### NDI

| Address                                        | Value   | What it does                                                              |
| ---------------------------------------------- | ------- | ------------------------------------------------------------------------- |
| `<layer prefix>/ndi`                           | string  | connects the layer to the NDI source with this name                       |
| `<layer prefix>/ndioutput`                     | 0/1     | sends the layer out as an NDI stream; without argument it toggles         |
| `/mix/ndioutput/deckA`, `/mix/ndioutput/deckB` | 0/1     | NDI output of the deck A or deck B monitor of the modus that is on screen |
| `/mix/ndioutput/mix`                           | 0/1     | NDI output of the mix monitor of the modus that is on screen              |
| `/mix/ndioutput/output`                        | 0/1     | NDI output of the output mix monitor (preview modus only)                 |
| `/ndi/refresh`                                 | trigger | looks for NDI sources and sends their names back as `/ndi/sources`        |

For `ndi`, use the name of the source as it is shown in the NDI source menu of the layer, for instance `MYPC (Camera 1)`.  If the name isn't known yet, the program looks for sources again before it gives up.  The reply to `/ndi/refresh` is the message `/ndi/sources` sent to the feedback address, with the name of every source found as a string argument.  The streams of this program itself are not offered as sources.

### 

### Files: layers, decks and mixes

| Address                    | Value   | What it does                                  |
| -------------------------- | ------- | --------------------------------------------- |
| `<layer prefix>/save`      | string  | saves the layer to a layer file at the path   |
| `/mix/<modus>/<deck>/save` | string  | saves the deck to a deck file at the path     |
| `/mix/<modus>/<deck>/open` | string  | opens the deck file at the path into the deck |
| `/mix/<modus>/<deck>/new`  | trigger | empties the deck                              |
| `/mix/<modus>/save`        | string  | saves the whole mix to a mix file at the path |
| `/mix/<modus>/open`        | string  | opens the mix file at the path                |
| `/mix/<modus>/new`         | trigger | empties the whole mix                         |

Opening a layer file into a layer is done with `load` (see "Layers"), which also opens videos, images, deck files and mix files.

The path is an absolute path on the machine EWOCvj2 runs on.  The program doesn't add a file extension, so include it.  A save is ignored when the directory of the path doesn't exist, an open when the file doesn't exist or isn't a deck or mix file.  File operations go through the same mechanism as the result of a file requester, so an operation waits when another file requester result or file opening is still being handled, and runs right after; up to 16 operations can be waiting.  Saving a layer only works for layers in the layer stacks, not for masks.

These work on the modus that is on screen; an operation for the other modus is ignored.  Opening and emptying replace what is in the deck or mix, and can't be undone from OSC.

### 

### Bins

The `/bin` addresses work on the bins of the project, with or without the bins room on screen.  Most of them work on the current bin; `/bin/select` changes which one that is.

| Address                       | Value                                       | What it does                                                                     |
| ----------------------------- | ------------------------------------------- | -------------------------------------------------------------------------------- |
| `/bin/select`                 | integer                                     | makes bin number N (from 1, in the order of the bins list) the current bin       |
| `/bin/new`                    | trigger                                     | adds a new bin and makes it the current bin                                      |
| `/bin/delete`                 | trigger                                     | deletes the current bin (the last bin can't be deleted)                          |
| `/bin/rename`                 | string                                      | gives the current bin a new name                                                 |
| `/bin/open`                   | string                                      | opens the bin file at the path and adds its bins                                 |
| `/bin/save`                   | string                                      | saves the current bin to a bin file at the path                                  |
| `/bin/loadshelf`              | string "A" or "B", integer 1-4, integer 1-9 | puts a block of the current bin in a bank of a shelf                             |
| `/bin/hapmode`                | 0/1 or string                               | the HAP encoding mode: `live` (0) or `max` (1); without argument it switches     |
| `/bin/hapencode`              | trigger                                     | HAP encodes all the elements of the current bin that can be encoded              |
| `/bin/selection/delete`       | trigger                                     | empties the selected elements of the current bin                                 |
| `/bin/selection/hapencode`    | trigger                                     | HAP encodes the selected elements                                                |
| `/bin/selection/clear`        | trigger                                     | deselects all elements                                                           |
| `/bin/element/R/C/select`     | 0/1                                         | selects or deselects the element at row R, column C; without argument it toggles |
| `/bin/element/R/C/delete`     | trigger                                     | empties the element                                                              |
| `/bin/element/R/C/hapencode`  | trigger                                     | HAP encodes the element; while it is encoding, the same message stops it         |
| `/bin/element/R/C/rename`     | string                                      | renames the element                                                              |
| `/bin/element/R/C/launchtype` | integer 0-2 or string                       | sets the launch type of that element (`restart`, `continue`, `catchup`)          |
| `/bin/element/R/C/loaddeck`   | string "A" or "B"                           | opens that element, a deck file, into deck A or B                                |
| `/bin/element/R/C/loadmix`    | trigger                                     | opens that element, a mix file, as the mix                                       |

- `new` gives the bin a name that isn't in use yet ("new bin", "new bin_1", ...).  `rename` is ignored when another bin already has that name.
- `open` and `save` use the same mechanism as the other file operations (see "Files: layers, decks and mixes"), so they wait for each other and for a bin import that is busy.  `save` makes a folder and a `.bin` file with the name of the path, in the directory of the path, which has to exist; `open` needs an existing bin file.
- Elements are in a grid of 12 rows by 12 columns, numbered from 1, from the top left.  `loaddeck` and `loadmix` only work for elements of the right kind, and on the modus that is on screen.
- The elements are grouped in 3 by 3 shelf blocks of 4 by 4 elements, numbered 1 to 9 from left to right and top to bottom.  `loadshelf` copies a block into a bank of shelf A or B, replacing what is in the bank.
- `hapmode` applies to encodes that start after it is set.
- Not available over OSC: moving the selected elements, AI upscaling of images and videos, exporting an element, and sharing bins.

### 

### Beat detection

| Address           | Value | What it does                      |
| ----------------- | ----- | --------------------------------- |
| `/beat/threshold` | 0-1   | sets the beat detection threshold |

The threshold is shared by the preview and output modus.  Changes are recorded into loopstation lines that are armed for recording.

The feedback also reports `/beat/bpm`: the tempo that the beat detection has found in the music, in beats per minute with one decimal, or 0 when it hasn't found one.  This is read only.

### 

### Layer clip queue

The queue addresses start with the layer prefix, `/mix/<modus>/<deck>/layer/N/queue/`.  The last place of a queue is always an empty one, which is where a new clip is added.

| Address (after the layer prefix) | Value   | What it does                                                                                                       |
| -------------------------------- | ------- | ------------------------------------------------------------------------------------------------------------------ |
| `queue/show`                     | 0/1     | unfolds (1) or folds (0) the queue under the layer; without argument it toggles                                    |
| `queue/next`                     | trigger | plays the next clip of the queue now                                                                               |
| `queue/scroll`                   | integer | scrolls the displayed queue to start at this clip (from 1)                                                         |
| `queue/position`                 | 0-1     | scrubs in the loop bar of the queue: 0 is the loop start of the layer, 1 the loop end, as dragging in the bar does |
| `queue/beats`                    | float   | beat switching: moves on to the next clip every this many beats (0.5, 1, 2, 4, ...), 0 turns it off                |
| `queue/P/load`                   | string  | loads the file at the path into place P of the queue (from 1)                                                      |
| `queue/P/delete`                 | trigger | deletes the clip at place P of the queue                                                                           |

`queue/position` is relative to the loop (between the loop markers), unlike the layer `position`, which covers the whole video, and it works like the bar does: the playhead moves and the clones follow, but nothing is recorded into loopstation lines.  The feedback is the playhead in the same range.  `queue/next` does nothing when the queue holds no clips.  `queue/P/load` takes a single path; use the number of the last, empty place to add the file to the end of the queue.  Videos, images and layer files can be loaded.  As with `load` for layers, the message is ignored when the file isn't found or another file opening is still busy.  The last, empty place can't be deleted.  `queue/beats` needs the beat detection, as in the program.

### 

### Effects

Effects are addressed through the layer they sit on, with the prefix `<layer prefix>/effect/K/` for the controls of effect number K.  Every layer has a second effect chain; this is reached through `effect2/K/` instead of `effect/K/`.

An effect that is already in a chain is always given by its place `K` in that chain, counting from 1, because the same effect can be in a chain several times.  Names are used to choose an effect: when adding or replacing one.  The name is the name in the effect menu (`blur`, `chromarotate`, `boxblur`, `emboss_2`).  AI styles are effects too, with the names of the styles in the "AI STYLE" entry of the effect menu: `add` and `replace` find them when no other effect has that name, and `addstyle` and `replacestyle` look only at the AI styles.  An AI style effect only has the on/off switch and the dry/wet slider.

| Address (after the layer prefix) | Value                      | What it does                                                                                |
| -------------------------------- | -------------------------- | ------------------------------------------------------------------------------------------- |
| `effect/add`                     | string, integer (optional) | adds the named effect to the effect chain, at the given place (from 1; default: at the end) |
| `effect2/add`                    | string, integer (optional) | the same for the second effect chain                                                        |
| `effect/addstyle`                | string, integer (optional) | adds the named AI style as an effect (place as for `add`)                                   |
| `effect/K/replace`               | string                     | replaces the effect in place K by the named effect                                          |
| `effect/K/replacestyle`          | string                     | replaces the effect in place K by the named AI style                                        |
| `effect/K/delete`                | trigger                    | deletes the effect in place K                                                               |

For instance, `/mix/preview/deckA/layer/1/effect/add "blur"` puts a BLUR at the end of the effect chain of that layer, `.../effect/add "blur" 1` puts it first, and `.../effect/2/replace "emboss_2"` replaces the second effect.  `effect2/...` works for all of these on the second chain, except that the first layer of a deck has no second chain.  Adding, replacing and deleting work on the modus that is on screen.  The numbers of the effects behind an added or deleted effect shift.

| Address (after the effect prefix)                | Value | What it does                                           |
| ------------------------------------------------ | ----- | ------------------------------------------------------ |
| `onoff`                                          | 0/1   | switches the effect on/off                             |
| `drywet`                                         | 0-1   | the dry/wet slider of the effect                       |
| `<parameter>`                                    | 0-1   | sets an effect parameter                               |
| `param/J`                                        | 0-1   | sets the Jth parameter of the effect (counting from 1) |
| `maskedit`, `masked`, `maskscroll`, `mask/M/...` |       | see "Masks"                                            |

For `<parameter>`, take the parameter name as it is shown on the effect, written as described under "Addresses and values": "Edge Thickness" is `edge_thickness`.  When a name is awkward, or when two parameters would end up with the same name, use `param/J` instead.  Source plugin parameters, mixer plugin parameters and ISF wipe parameters work in the same way.

**Colour parameters.**  A colour parameter (an ISF colour input, or a red/green/blue group of an FFGL plugin) is set with a hex string instead of a number: `#RRGGBB`, for instance `#FF8800` (the `#` is optional).  This works for effect parameters and for the parameters of source plugins, mixer plugins and ISF wipes, under the name of the colour parameter.  An FFGL colour is one parameter under the name of its red part; its green and blue parts have no address of their own.  The alpha that goes with an ISF colour is a normal 0-1 parameter, with the name of the colour followed by `_alpha` (`color_alpha`).  Feedback for a colour parameter is the hex string.  Colours aren't recorded into loopstation lines.

Values sent to effect parameters are recorded into loopstation lines that are armed for recording, like moving the parameter with the mouse or a MIDI controller.

### 

### Shelves

There are two shelves, A (left) and B (right), each with four banks of sixteen elements.  Elements are numbered 1 to 16 in the order of the shelf grid: left to right, row by row, starting at the top left.

| Address                                                          | Value                                                                          | What it does                                                                    |
| ---------------------------------------------------------------- | ------------------------------------------------------------------------------ | ------------------------------------------------------------------------------- |
| `/shelf/A/bank`, `/shelf/B/bank`                                 | integer 1-4                                                                    | selects the bank of that shelf                                                  |
| `/shelf/load`                                                    | string "A" or "B", integer 1-4, integer 1-16, string                           | loads the file at the path into the given shelf, bank and element               |
| `/shelf/A/load`, `/shelf/B/load`                                 | integer 1-4, integer 1-16, string                                              | the same, with the shelf in the address                                         |
| `/shelf/delete`                                                  | string "A" or "B", integer 1-4, integer 1-16                                   | erases the given element                                                        |
| `/shelf/A/delete`, `/shelf/B/delete`                             | integer 1-4, integer 1-16                                                      | the same, with the shelf in the address                                         |
| `/shelf/rename`                                                  | string "A" or "B", integer 1-4, integer 1-16, string                           | gives the element a new name                                                    |
| `/shelf/A/rename`, `/shelf/B/rename`                             | integer 1-4, integer 1-16, string                                              | the same, with the shelf in the address                                         |
| `/shelf/new`                                                     | string "A" or "B", integer 1-4                                                 | empties all 16 elements of that bank                                            |
| `/shelf/A/new`, `/shelf/B/new`                                   | integer 1-4                                                                    | the same, with the shelf in the address                                         |
| `/shelf/open`                                                    | string "A" or "B", integer 1-4, string                                         | opens the shelf file at the path into that bank                                 |
| `/shelf/A/open`, `/shelf/B/open`                                 | integer 1-4, string                                                            | the same, with the shelf in the address                                         |
| `/shelf/save`                                                    | string "A" or "B", integer 1-4, string                                         | saves that bank to a shelf file at the path                                     |
| `/shelf/A/save`, `/shelf/B/save`                                 | integer 1-4, string                                                            | the same, with the shelf in the address                                         |
| `/shelf/insertinbin`                                             | string "A" or "B", integer 1-4 (bank), integer 1-9 (block)                     | puts the whole bank in a block of the current bin                               |
| `/shelf/A/insertinbin`, `/shelf/B/insertinbin`                   | integer 1-4 (bank), integer 1-9 (block)                                        | the same, with the shelf in the address                                         |
| `/shelf/insertdeck`                                              | string "A" or "B" (shelf), integer 1-4, integer 1-16, string "A" or "B" (deck) | puts the current deck A or B in the given shelf element                         |
| `/shelf/A/insertdeck`, `/shelf/B/insertdeck`                     | integer 1-4, integer 1-16, string "A" or "B" (deck)                            | the same, with the shelf in the address                                         |
| `/shelf/insertmix`                                               | string "A" or "B", integer 1-4, integer 1-16                                   | puts the current mix in the given shelf element                                 |
| `/shelf/A/insertmix`, `/shelf/B/insertmix`                       | integer 1-4, integer 1-16                                                      | the same, with the shelf in the address                                         |
| `/shelf/A/trigger`, `/shelf/B/trigger`                           | integer 1-16                                                                   | triggers that element of the current bank into the mix                          |
| `/shelf/A/launchtype`, `/shelf/B/launchtype`                     | integer 0-2 or string, integer 1-4 (optional bank)                             | sets the launch type of all 16 elements of the bank (default: the current bank) |
| `/shelf/A/element/N/launchtype`, `/shelf/B/element/N/launchtype` | integer 0-2 or string, integer 1-4 (optional bank)                             | sets the launch type of element N of the bank (default: the current bank)       |

- The arguments of the address forms without the shelf in the address are, in order: the shelf, the bank, the element, then the extra argument (path, name or deck).  The bank doesn't have to be the current one.  `trigger` always works on the current bank; everything else, `launchtype` included (with its optional bank argument), takes effect on any bank.  The feedback for launch types is for the current bank only.
- `load` takes a single path on the machine EWOCvj2 runs on, and handles it like a file dropped on a shelf element, so videos, images, layer files, deck files and mix files are supported.  Loading takes a few frames.  It is ignored when the file isn't found or another file opening is still busy; files of an unsupported type are skipped.  The launch type of the element isn't changed by loading.
- `rename` ignores empty elements.
- `open` and `save` go through the same mechanism as the other file operations (see "Files: layers, decks and mixes") and wait for one another.  A `save` is ignored when the directory doesn't exist, an `open` when the file doesn't exist.  A saved shelf always gets the extension `.shelf`, whatever extension the path has.  `new`, `open` and `save` are not tied to the modus that is on screen.
- `insertinbin` copies the bank into one of the 9 blocks of the current bin of the bins room (block 1 is the top left one, block 9 the bottom right one, in a 3 by 3 grid), replacing what is there.  It only changes the bin in memory.  It is ignored when there is no current bin, or while "Insert in bin" is active in the bins room.
- `insertdeck` and `insertmix` use the deck or mix of the modus that is on screen.
- A trigger works like a MIDI-mapped shelf button: the element goes to the currently selected layer(s), and the trigger is recorded into armed loopstation lines.  Empty elements are ignored.  Triggers that arrive while a previous one is still being handled are queued and executed in order.
- Launch types: 0 or `restart`, 1 or `continue`, 2 or `catchup`.  They are set on the elements that are currently in the shelf banks, and are not stored in the shelf itself.

### 

### Loopstation

The loopstation addresses control the loopstation that is on screen: the preview loopstation in preview modus, otherwise the output one.  Lines are numbered 1 to 256, top to bottom.

| Address                            | Value         | What it does                                                                               |
| ---------------------------------- | ------------- | ------------------------------------------------------------------------------------------ |
| `/loopstation/current`             | integer 1-256 | selects the current line                                                                   |
| `/loopstation/scroll`              | integer       | scrolls the loopstation list to start at this line (from 1)                                |
| `/loopstation/line/N/rec`          | 0/1           | the record button of the line                                                              |
| `/loopstation/line/N/loop`         | 0/1           | the loop play button of the line                                                           |
| `/loopstation/line/N/play`         | 0/1           | the one-off play button of the line                                                        |
| `/loopstation/line/N/speed`        | raw           | the speed of the line (0 to 4)                                                             |
| `/loopstation/line/N/position`     | 0-1           | scrubs inside the recording of the line                                                    |
| `/loopstation/line/N/clear`        | trigger       | clears the line                                                                            |
| `/loopstation/line/N/copyduration` | trigger       | copies the loop duration of the line                                                       |
| `/loopstation/line/N/pastespeed`   | trigger       | gives the line the copied duration by changing its speed                                   |
| `/loopstation/line/N/pastelength`  | trigger       | gives the line the copied duration by changing the length of its loop                      |
| `/loopstation/line/N/beats`        | float         | beatmatches the line so its loop lasts this many beats (0.5, 1, 2, 4, ...), 0 turns it off |
| `/loopstation/line/N/copycurve`    | trigger       | copies the curve of the line (a line that holds a curve)                                   |
| `/loopstation/line/N/pastecurve`   | trigger       | pastes the copied curve on the line                                                        |

The buttons behave as when you click them; without argument they toggle.  `loop` and `play` do nothing on a line that holds no recording, and `position` is ignored for one.  The address forms work on any line, whether it is in view in the loopstation list or not; the list never scrolls because of them (use `scroll` for that).  `copycurve` and `pastecurve` only do something for lines that were made with the curve editor.  MIDI learn and the curve editor aren't available over OSC.

### 

### Feedback

As soon as the first message arrives from a controller, EWOCvj2 starts sending its state back to the computer that message came from, on the feedback port from the preferences (UDP port 9001 by default).  When an OSC password is set, this is the first computer that logs in.  That computer keeps getting the feedback, also when other computers send messages too, until you point the feedback somewhere else with `/osc/target`, or until the program restarts, OSC is restarted or the password is changed; then the next computer to send a message (or log in) gets it.  Only changes are sent, and at most about 30 times per second, so a playing video doesn't flood your network.  A controller that has just set a value itself doesn't get it echoed back.

With "Old hardware" turned on in the preferences, feedback is sent at most 4 times per second, and the more detailed parts are left out: effect parameters, mixer and source plugin parameters, playback positions of layers and loopstation lines, loopbox start and end, queue clip names and shelf element names and launch types.  Everything else is still reported, and turning the setting off sends the complete state again.

The addresses and value ranges of the feedback messages are exactly the same as the ones above, and the values are sent as floats (switches as 0.0 or 1.0), except for the names (and colours), which are sent as strings (see "Names, counts and lists" below).  The following is reported:

- Which NDI outputs are running: for layers (`ndioutput`) and for the monitors of the modus on screen (`/mix/ndioutput/...`).

- Crossfaders, the modus (`/mix/modus`: 1 for preview, 0 for live), wipe type/direction/position, the output recording state and the current scene of each deck (`/mix/scene/deckA` and `/mix/scene/deckB`).

- Deck speeds, deck general MIDI presets, the scroll position of the layer stacks and the mask depth of a deck (`maskdepth`).

- For the loopstation on screen: the current line, the scroll position of the list and, for every line, rec/loop/play, speed, beats and position.

- For each shelf: the current bank and the launch type of all elements in it.

- For the bins: the current bin, the number of bins and the HAP encoding mode.

- For every layer: opacity, volume, speed, play, reverse, bounce, loop, mute, solo, position, loop start and end, the displayed effect chain, general MIDI preset, upscale and sharpness, mix factor, wipe settings, the key settings, the locks and keep switches, the aspect setting, the state of the clip queue (`queue/show`, `queue/size`, `queue/scroll`, `queue/beats`) and the parameters of source and mixer plugins.  `mixmode` is reported as a number: the built-in modes 1 to 23 in the order of the list above, 25 for a wipe, 1000 for an FFGL mixer and 2000 for an ISF mixer.  `wipe/type` is reported as the wipe number, or -1 when the layer doesn't use a built-in wipe.

- For every effect: on/off, dry/wet and all parameters.

The `/osc` addresses control this feedback:

| Address         | Value                     | What it does                                                                                                                                          |
| --------------- | ------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------- |
| `/osc/target`   | string host, integer port | sends feedback to this host and port, instead of to the computer that was found automatically; the port is optional and defaults to the feedback port |
| `/osc/feedback` | 0/1                       | turns feedback off or on                                                                                                                              |
| `/osc/sync`     | (none)                    | resends the complete state, handy after your controller (re)connected                                                                                 |
| `/osc/auth`     | string password           | logs in when an "OSC Password" is set in the preferences (see below)                                                                                  |

### 

### Security

OSC is not encrypted, so these settings only keep out casual access; on untrusted networks use a firewall as well.  "Localhost only" (see "Preferences") is the strongest option when your controller runs on the same computer.

**Password.**  When the "OSC Password" in the Preferences is not empty, messages from a computer are ignored until it has sent `/osc/auth` with the password.  The answer, `/osc/auth ok` or `/osc/auth denied`, is sent to the sender on the feedback port.  After five wrong passwords that computer is ignored for a minute.  Feedback only goes to computers that logged in.  A controller has to send `/osc/auth` again after the program restarts or the password is changed.  The password is stored as plain text in the preferences file.

**Safe mode.**  When "Safe mode" is on, messages that write files, erase things or start long jobs are ignored, so a controller can play but not damage anything.  These are the addresses ending in `save`, `open`, `new`, `delete`, `rename`, `insertdeck`, `insertmix`, `insertinbin` or `hapencode`, and the `clear` of a loopstation line.  Loading files (`load`, `loaddeck`, `loadmix`, `loadshelf`) still works, as a show can't do without it.  Keep in mind that `load` takes any path on the computer.

### 

### Names, counts and lists

A controller needs to know how much there is and what it is called.  The feedback therefore also contains counts, and names as strings:

| Feedback address                                     | Value          | What it tells                                                                       |
| ---------------------------------------------------- | -------------- | ----------------------------------------------------------------------------------- |
| `/mix/<modus>/<deck>/layers`                         | number         | the number of layers in the layer stack of the deck                                 |
| `<layer prefix>/name`                                | string         | the name of what the layer shows: the file name, or the name of the source plugin   |
| `<layer prefix>/effect/count`, `.../effect2/count`   | number         | the number of effects in the two effect chains of the layer                         |
| `<layer prefix>/mask/count`                          | number         | the number of masks of the layer                                                    |
| `<layer prefix>/effect/K/name` (and `effect2`)       | string         | the name of effect K as in the effect menu                                          |
| `<layer prefix>/queue/size`, `.../queue/P/name`      | number, string | the number of clips in the queue, and the file name of clip P (up to 32)            |
| `/shelf/A/element/N/name`, `/shelf/B/element/N/name` | string         | the name of element N in the current bank of the shelf (empty for an empty element) |
| `/bin/count`, `/bin/name`                            | number, string | the number of bins and the name of the current bin                                  |

Names are sent when they change, and all of them again after `/osc/sync`.  Counts tell how far the numbers in the addresses go: layers, effects, masks and queue places are all numbered from 1 up to their count.

The names that you can use in addresses and the displays and bins can be asked for with a list request.  Send `/list/<what>` without arguments and the program answers with the message `/list/<what>` to the feedback address, with every name as a string argument:

| Request          | The answer lists                                                                                                   |
| ---------------- | ------------------------------------------------------------------------------------------------------------------ |
| `/list/effects`  | the effects of the effect menu, in the order of the menu                                                           |
| `/list/aistyles` | the AI styles                                                                                                      |
| `/list/mixmodes` | the mix modes: the first 23 are the built-in modes in the order of their numbers (1 to 23), then the plugin mixers |
| `/list/wipes`    | the wipes, in the order of their numbers (CROSSFADE is the first, number -1), then the ISF wipes                   |
| `/list/sources`  | the source plugins, in the order of the source plugin menu                                                         |
| `/list/displays` | the other displays (the numbers for `display`: the first one is 1), by their system name                           |
| `/list/bins`     | the names of the bins, in the order of the bins list (the numbers for `/bin/select`)                               |
| `/list/ndi`      | the NDI sources that are found (the same as `/ndi/refresh`, which answers with `/ndi/sources`)                     |

Except for the displays, the bins and the NDI sources, the names in the lists are written the way they are used in the addresses.  The list requests need a feedback address, so a controller has to have sent at least one message (or logged in, when there is a password) or `/osc/target` has to be set.

### 

### Not (yet) available

- Going back to the previous clip of a layer queue and reordering the clips of a queue.

- Choosing which layer a shelf element lands in: triggers go to the currently selected layer(s).

- Moving selected bin elements, AI upscaling, element export and sharing bins.

- The rooms of the program (switching rooms, the style, generation and segmentation rooms), the preferences, the MIDI configuration and quitting.
