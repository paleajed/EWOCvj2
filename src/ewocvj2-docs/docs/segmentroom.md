---
sidebar_position: 4
title: Segmentation room

---



![](https://www.ewocprojects.com/build/img/segmentroom.png)

# 

# Segmentation room

### 

### Intro

This room uses Meta's SAM3 AI segmentation system to implement making masked versions of videos, steered by a prompt.  It selects and follows object(s) described by a prompt throughout the video and "erases" (makes transparent) all other content surrounding them.  The resulting videos can be exported to disk and used in your mixes like any other video.

For the segmentation room to be available you need to install the SAM3 optional AI module; optional AI install can  be accessed through the main project selection screen at startup.  Since the model runs locallyon your machine, a powerful GPU with enough VRAM is needed to do a segmentation run.

### Input video box

Drag content here through the program's wormgates or rightclick and choose "Browse" to open a filebrowser to open a video from disk.

### Output video box

After segmenting and exporting the output video, that video will become available in this box allowing you to drag it through the wormgate to any receiving location.

### Loop selection box

This is the video position/loop bar that allows selecting only a part of your video to be segmented. Especially handy for those videos that have different camera views; SAM3 only handles fluid motion from start to end, no camera cuts!  Its length is the current video length, the position of the (moving) white line the video position. Leftclicking on the bar will set current video position there. Leftclickdragging allows video/audio scrubbing. (Is your scrubbing not fluid? Read "Video formats and VJ’Ing"); You can select a part of the video duration as your "loop" to only segment a part of a video. Rightclicking the bar allows to set the current position as loop start (shortcut "L" key) or end (shortcut "P" key); the selected loop is shown as the green part of the bar. Also, when holding CTRL, you can drag the loop ends or the loop body with leftmouse drag.

### Prompt box

Leftclicking it allows text entry of a prompt that describes what needs to be masked.  Don't use plural.  Try to keep it simple: just "cow" or "windmill" is often enough.

### Outline preview

Before segmentation, it shows a preview of your input video.  After segmentation it will show (multiple) colored outlines denoting the different segmented objects.  Then you can click inside any outline to deselect/reselect that particular object to be included in the output video.

### Masked preview

After segmentation, it shows a preview of the first frame of your segmented video loop, masked like it would show in the resulting output video.

### Buttons

There are three buttons left of the progress message line:

- "SEGMENT/CANCEL":  starts/cancels the segmentation run.  When cancelling be aware the system, in  certain stages, can't be halted before it finishes doing a certain operation.  Try to cancel as little as possible, cause it can work confusing.

- "INVERT": after the segmentation run you can invert/de-invert the masks by leftclicking this button.

- "EXPORT": after a segmentation run, this button will become available: it will present you with a filebrowser to choose output video name/location and will then encode the output video (output videos are compressed with the HAP codec) and save it.  Once exported, the output video will become available in the output video box, allowing you to drag it through the wormgate to any receiving location.

### Recognition threshold

Leftclick drag this parameter to set how sure the model must be for it to indentify an object by the prompt.  The higher this value, the more certain the model must be.

### Gate to media bins

On the right side of the screen the "BINS" wormgate is shown that can transport you to the media bins screen. Functionality explained in "Wormgates" on the "Bins room" page.

### 
