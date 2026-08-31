---
sidebar_position: 5
title: Gen room
---

![](https://www.ewocprojects.com/build/img/genroom.png)

# Gen room

### Intro

The gen room, or AI generation room allows you to locally (on your machine; no external services) generate either videos or images, depending on the model backends you have installed (choose "Optional AI installs" in the main startup project choice screen and download Flux and/or LTX-2.5).  Do note you need a considerable amount of GPU VRAM to run either of these generation models: Flux needs 12Gb+, while LTX-2.5 comes in three quality/VRAM tiers - see "Backend" below.

LTX-2.5's weights are gated on HuggingFace: before you can download them, you need to visit the model's page on huggingface.co while logged in and accept its license/consent form, then generate a HuggingFace access token (huggingface.co -> your profile -> Access Tokens) and paste it into the "HF Token" field on the "Optional AI installs" screen. All three LTX-2.5 tiers share the one token.

### Backend

Depending on what you installed, you can choose here between the Flux backend (AI image generation) and the three LTX-2.5 backend tiers (AI video generation):

- "LTX 2 High Quality" (BF16): the full-precision, non-distilled 22B model. Best quality, but needs ~44Gb VRAM.

- "LTX 2 Fast Blackwell" (NVFP4): the distilled 22B model quantized for NVIDIA's Blackwell architecture. Needs a Blackwell-generation card (RTX 50-series, B100, B200) - it won't run on older GPUs.

- "LTX 2 Consumer" (GGUF): the distilled 22B model quantized to GGUF Q4_K_M. Most VRAM-efficient tier (~14Gb), and runs on any CUDA GPU that fits it - the practical choice if you don't have a Blackwell card or 44Gb+ VRAM to spare.

All three tiers offer the same set of presets below - pick whichever tier fits your hardware.

### Presets list

Depending on the chosen backend, here you can choose between a number of preset operations:

Flux:

- "Text-to-Image": translates a natural language text prompt into an image.  You can set:
  
  - "Seed": set this to "0" to have a randomized seed.  Other values trigger generations that are consistent even when run several times.
  
  - "Steps": the number of generation steps: the higher, the better the generation quality and the longer generation takes.  Generally kept at 4 for Flux.
  
  - "Width": width in pixels of the generated image.
  
  - "Height": height in pixels of the generated image.
  
  - "Reference images": see "Input box" below.

- "Content+Scene": keeps the visual content from the Input box (a person, object, product, etc.) consistent while placing it into a scene driven by the text prompt and an optional Scene reference image.  You can set:
  
  - "Seed", "Steps", "Width", "Height": see above.
  
  - "Reference strength": how strongly the Input box's content is retained.
  
  - "Scene": an optional second reference image (drag it into the Scene box below the Input box) supplying the environment/scene the content gets composited into.  Comes with its own "Strength" slider.

LTX-2.5:

- "Text-to-Video": translates a natural language text prompt into a video. You can set:
  
  - "Seed": set this to "0" to have a randomized seed. Other values trigger generations that are consistent even when run several times.
  
  - "Steps": the number of generation steps: the higher, the better the generation quality and the longer generation takes.
  
  - "Frames": the number of frames of the resulting video.
  
  - "Width": width in pixels of the generated video.
  
  - "Height": height in pixels of the generated video.

- "Image-to-Video": translates a still image (loaded into the Input box) into a video, taking into account the text prompt. You can set:
  
  - See above.

- "First & Last Frame to Video": generates a video that transitions from a first frame image (Input box) to a last frame image (Last Frame box). You can set:
  
  - See above.
  
  - "First Frame Strength"/"Last Frame Strength": how strongly each end anchor is honoured.

- "First Frame All Frames": takes a reference video (Input box) and propagates an edit made to its first frame across the *whole* clip. Load the original video into the Input box, and the already-edited version of its first frame into the Content box below it - the result follows the video's own motion/camera/timing while consistently carrying the visual edit established in that first frame through every subsequent frame. You can set:
  
  - See above.
  
  - "Strength" (on the Content box): how strongly the first-frame edit is propagated.

- "Character Retention": keeps a character/face consistent using a reference face image loaded into the Input box. You can set:
  
  - See above.
  
  - "Strength": low values create heavy "identity input" results, high values shift more influence to the prompt.

- "Cutout Guides": takes a reference video (Input box) that has a normal, moving background with a fixed/static "cutout" subject composited flatly on top of it (e.g. a pasted-in cutout that doesn't animate, but moves over the background video) - and makes that cutout "come alive", rendering it as a realistically moving, fully-integrated entity within the moving background. You can set:
  
  - See above.

### Input box

In this box, you can put an image or video that serves as an input for one of the presets.  Depending on the preset this needs to be either video or image.  You can drag over content from other parts of the program (using the wormgates), or you can rightclick the box, which will allow you to either clear it ("Clear") or load content into it ("Browse") from a filebrowser.  

##### REF images:

![](https://www.ewocprojects.com/build/img/REFbox.png)

When in the Flux backend, four "REF" boxes will appear.  These allow the user to set up to four reference images, which will be incorporated into the image generation.  Give it a try to get a taste for how the algorithm works.  The strength with which each reference image influences the result can be set with the respective "Strength" sliders.

### Prompt box

This box allows typing a natural language text prompt that will guide your generation.

### Negative prompt box

This box allows typing a natural language text prompt that will guide your generation by typing what should be *avoided* in the generation; like "busy movement" or "camera movement".

### GENERATE/CANCEL button

This button will start the generation process.  WHen processing it changes into the self-explaining CANCEL button.

### Status messages

Here status messages, like which part of the process is running, or a progress bar that shows the generation progress.

### History list

When a generation is finished, its result will show as an (animated) thumbnail.  All generations of this session will be put in a thumbnail list, which, when longer than four elements, can be scrolled by the "scroll items" boxes.  Elements can be dragged anywhere, or deleted (rightclick -> "Delete").  The rightclick -> "Export" option allows the item, which is, by default, saved in the Generations folder (settable in Preferences, defaults to EWOCvj2/generations in your platform specific Documents folder), to be exported to any location/name from within a filebrowser.   Once exported, the history item will refer to the new exported filepath, so when dragging it somewhere, you will be dragging the new file, not the old one.  All generations will stay in the Generations folder also, even when exported, so get into the habit of cleaning out the Generations folder once in a while, or it might grow big.  

Leftclick a history item to preview it in the preview box.

### Preview box

Shows generated items when clicked in the history list.

### Gate to media bins

On the right side of the screen the "BINS" wormgate is shown that can transport you to the media bins screen. Functionality explained in "Wormgates" on the "Bins room" page.

### 
