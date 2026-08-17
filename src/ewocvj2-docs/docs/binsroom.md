---
sidebar_position: 2
title: Bins room


---

![](https://www.ewocprojects.com/build/img/binsroom.png)



# Bins room

### Intro

The bins room, that can be reached by clicking the BINS wormgate or dragging content through the gate up to the very edge of the screen, is a room that visualizes the bin structures that house and arrange the different elements that can be used to make compositions/performances with, their thumbnails, their name, resolution and category. 

A bin is an on-screen visual structure that consists of a grid of 12x12 elements, divided into 9 (3x3 times a 4x4 block) bin blocks (yellow lines).
Each grid element can contain:

- a video file (grey mark)

- an image file (yellow mark)

- a layer file (orange mark): a file that contains one layer with all of its settings/effect/masks at a certain time point

- a deck file (green mark): a file that contains all layers of a deck, their settings/effect/masks and order at a certain time point

- a mix file (purple mark): a file that contains all layers of both decks, their settings/effect/masks and order at a certain time point

This way you can arrange different visually assembled file sets with very different purposes.   The bin(s) are saved with the project; an unlimited list of bins can be added to a project.  Bin elements can be hovered over, showing if it decompresses on CPU or GPU (" HAP"), the video resolution and when hovered over, the video of a bin element (videos/images or layer files with all their effects) will be previewed at top right of the screen, using the mousewheel to  fastforward through the file.  Content can be dragged into the bin from every draggeable item in the program.  When dragging a layer over from the layer stack, you get the option to insert it either as a layer file (with all settings) or as a plain video or image.  Rightclicking on a bin element opens the bin element menu.

### 

### Bin element menu

- "Delete element": deletes the bin element

- "Rename element": allows renaming the bin element, the "name" is separate from the filepath, which isnt changed when the name changes.  Name is a data field that belongs to the EWOCvj2 bin element data structure to allow giving names to elements that are specific to the workflow approach taken by the user.  It is also wise to rename files as a preparation for when bin elements are loaded into the shelf.

- "Open file(s) from disk": will open up a file browser that allows you to open any number of video/image files, layer files  deck files or mix files into that layer. When multiple files are selected, you are dropped into the "ordering" list display. It lists all file paths to be opened accompanied by a thumbnail preview image. You can drag entries in this list up and down to change the order in which files are added; each additional file will end up in a new layer to the right (remember: higher up the layer stack) of the previous one. Click "APPLY ORDER" to confirm the order and open the files. Clicking right mousebutton here cancels the open operation.

- "Export element": will open up a file browser that allows you to save (export) the element file to a certain file other than the original.  Mainly used to relocate files, notably those dragged over from the genroom.

- "Insert deck A": inserts the entire A deck into the bin element, a deck file is made and placed into the bins/#*binname*# folder in the current project folder, and a thumbnail of the deck is created.  The deck snapshot is taken at the current timepoint.  Renaming the element afterwards is always a smart move.

- "Insert deck B": inserts the entire B deck into the bin element, a deck file is made and placed into the bins/#*binname*# folder in the current project folder, and a thumbnail of the deck is created. The deck snapshot is taken at the current timepoint.  Renaming the element afterwards is always a smart move.

- "Insert mix": inserts the entire mix into the bin element, a mix file is made and placed into the bins/#*binname*# folder in the current project folder, and a thumbnail of the mix is created. The mix snapshot is taken at the current timepoint.  Renaming the element afterwards is always a smart move.

- "Load block in shelf A": pops up a submenu allowing a choice of bank in the current shelf A in which the bin elements in the 4x4 block (yellow lines) hovered by the mouse pointer will be loaded.

- "Load block in shelf B": pops up a submenu allowing a choice of bank in the current shelf B in which the bin elements in the 4x4 block (yellow lines) hovered by the mouse pointer will be loaded.

- "Upscale image": pops up a submenu that allows upscaling the image (if there is one in the bin element) using four RealESRGAN (an AI image upscaler) variants.  "realesr-animevideov2-x2",  "realesr-animevideov3-x3", "realesr-animevideov3-x4" and "realesrgan-x4plus-anime" are Anime models while "realesrgan-x4plus" is photorealistic.  The upscale factor is at the end of each variant name.

- "Upscale video": pops up a submenu that allows upscaling the video (if there is one in the bin element; only one element can be upscaled at any given time) using two different local AI engines (EDVR: non-hallucinating and FlashVSR: hallucinating detail).  Take care: the better the quality and upscalefactor of each option, the more VRAM and power it requires on your GPU:
  
  - "FAST": uses EDVR to do fast less quality cleanup, x2, x3 or x4 upscaling
  
  - "BALANCED": uses EDVR to do medium quality cleanup, x2, x3 or x4 upscaling
  
  - "ULTRA": uses FlashVSR to do slow high quality cleanup, x2, x3 or x4 upscaling

- "Cancel upscaling": appears when upscaling a video; allows cancelling the upscaling operation.

- "HAP encode element": starts HAP encode of the bin element video.   HAP versions get "_hap" appended to their path and are placed in the same folder as the original video. When video stashing is on (see Preferences), the original video will be moved to the stashing directory.

- "Quit": quits the program.  A requester will pop up that allows just quitting, quitting after saving the current project at the current timepoint, or cancelling the quit operation.

### Launchtypes

Every bin element has three tiny buttons on the left. One of these is always selected. They set the default launchtype of the element for when it is loaded into and launched from one of the shelves:

- "Restart": every time the element will be triggered from the shelf, its contents are played back from the start.

- "Continue": the triggered content (when put into the shelf), when overwritten by, for example, a trigger of another element on the same layer(s), will have its playback position remembered, and at subsequent launches, it will continue on from the position where it was overwritten.

- "Catch up": the triggered content (when put into the shelf), when overwritten by, for example, a trigger of another element on the same layer(s), will have its playback position remembered, and, although not displayed, it will keep running in the background. At subsequent launches, it will continue on from the position it is at if it had been running all along.

### 

### List of bins

The box just to the right of the grid contains a list of all bins inside the project. When first starting there will be one empty bin called “this is a bin”. Rightclicking the bins list box will pop up a menu:

- "New bin": adds a new empty bin to the bins list.

- "Open bin": opens up a filebrowser in which you can select a .bin file on disk (like those in other projects' bins folders) to open and add it to the bins list.

- "Save bin": opens up a filebrowser in which you can save the bin to disk as a .bin file with additional folder #*binname*# next to it that contains thumbnails and possible content files like layer, deck or mix files in the bin.

- "Rename bin": allows renaming the bin, rightmouse button cancels.

### 

### Moving elements

You can leftclickdrag an element to another position in the grid.  When that element already contains something, the two elements will be swapped.

Leftclickdrag outside any bin element thumbnail allows you to bo select a box-shaped set of elements (white marked when selected), which you can move around by leftclickdragging a second time (starting from whitin a thumbnail of one of the bo-selected elements).  This type of move overwrites other content when dropped; nothing is swapped.  

### 

### Wormgate to other rooms

On the extreme right of the bins screen, you find the wormgates to other rooms.  Clicking these gates will transport you to the respective room (style, gen and segment rooms only become available when optional AI modules are properly installed; access these optional downloads at first program run after installation or through the initial project choice screen).  Dragging content into the wormgate and then moving the mouse up to *the very edge* of the screen allows dragging the content over into the other rooms.

### 

### Layer elements

EWOCvj 2 uses the concept of “layer files” to assemble all parameters of a certain layer you set up in the mixing part into an open- and saveable file format. So its effects plus parameters plus all masks plus  its playing state plus its loop settings and all
other settings will be saved together, allowing reuse of entire layer setups. In the mixroom you can drag layers with leftmouse from the layer monitors on the mix screen into the BINS wormgate to transport yourself to the binsroom and then drop them in one of the bin grid element boxes. A layer file box in the bin (or in the mixroom shelf) always has an orange border.

### 

### Deck elements

EWOCvj 2 uses, alongside layer files also a deck file structure. Its an assembly of all the layers files (layers + settings) contained in either the A or B deck in the mix. All the information of the decks setup can be saved/opened together.  Deck files that are available in a bin can be launched by dragging them through the MIX room wormgate and dropping them anywhere on a layer stack deck; the entire deck (either A or B) will show a blue/purple box to signify the entire deck will be affected by the drop.

### 

### Mix elements

EWOCvj 2 uses, alongside layer and deck files also a mix file structure. Its an assembly of all the layers files (layers + settings) contained in both A and B decks together; so, the entire active mix (without inactive scenes). All the information of the mix setup can be saved/opened together.  Mix files that are available in a bin can be launched by dragging them through the MIX room wormgate and dropping them anywhere on a layer stack; the entire mix will show a blue/purple box to signify the entire mix (but only the current scene) will be affected by the drop.
