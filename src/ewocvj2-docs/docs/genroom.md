---
sidebar_position: 5
title: Gen room
---





![](https://www.ewocprojects.com/build/img/genroom.png)

# Gen room

### Intro

The gen room, or AI generation room allows you to locally (on your machine; no external services) generate either videos or images, depending on the model backends you have installed (choose "Optional AI installs" in the main startup project choice screen and download Flux and/or Hunyuan).  Do note you need a considerable amount of GPU VRAM (Flux: 12Gb+, Hunyuan: 16Gb+) to run either of these generation models.



### Backend

Depending on what you installed, you can choose here between the Flux backend (AI image generation) and the Hunyuan backend (AI video generation).



### Presets list

Depending on the chosen backend, here you can choose between a number of preset operations:

Flux:

- "Text-to-Image": translates a natural language text prompt into an image.  You can set:
  
  - "Seed": set this to "0" to have a randomized seed.  Other values trigger generations that are consistent even when run several times.
  
  - "Steps": the number of generation steps: the higher, the better the generation quality and the longer generation takes.  Generally kept at 4 for Flux.
  
  - "Width": width in pixels of the generated image.
  
  - "Height": height in pixels of the generated image.
  
  - "Reference images": see "Input box" below.

- "Image-to-Image": translates an input image (loaded into the Input box) into a newly generated image, taking into account the text prompt.  You can set:
  
  - "Seed", "Steps", "Width", "Height": see above.
  
  - "Keep original": sets how strong the influence of the input image is.

Hunyuan:

- "Text-to-Video": translates a natural language text prompt into a video. You can set:
  
  - "Seed": set this to "0" to have a randomized seed. Other values trigger generations that are consistent even when run several times.
  
  - "Steps": the number of generation steps: the higher, the better the generation quality and the longer generation takes.
  
  - "Prompt adherence": how strictly the text prompt influences the resulting video.
  
  - "Frames": the number of frames of the resulting video.
  
  - "Width": width in pixels of the generated video.
  
  - "Height": height in pixels of the generated video.

- "Image-to-Motion": translates an input image (loaded into the Input box) into a newly generated image, taking into account the text prompt. You can set:
  
  - See above.
  
  - "Keep original": sets how strong the influence of the input image is.

-  "Video Continuation": takes the last frame of an input video (loaded into the Input box) and does an Image-to-motion pass on it.  To continue generation from a previously generated video.  You can set:
  
  - See above.
  
  - Append: On or OFF.  When OFF a new video snippet is generated, when ON, the new snippet is added to the input snippet into a new, longer, video (which can then be put into the Input box once again, to create an even longer video).

- "Batch Variation Generator T2V": like "Text-to-Video", except that multiple generations (of the same prompt) are done sequentially.  You can set:
  
  - See above.
  
  - "Batch": batch size (number of videos to be generated).

- "Batch Variation Generator I2V": like "Image-to-Motion", except that multiple generations (of the same prompt and input image) are done sequentially. You can set:
  
  - See above.
  
  - "Batch": batch size (number of videos to be generated).

- "Frame Interpolation": takes an input video (loaded into the Input box) and adds AI interpolated frames between each frame pair.  You can set:
  
  - "Multiplier": how many times the number of frames will be multiplied.

- "Remix Existing Clip": takes an existing input video (loaded into the Input box) and "reinterprets" it into a new video, taking into account the text prompt.  You can set:
  
  - "Remix": remix strength, how "creative" the AI gets in reinterpreting your original video.



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
