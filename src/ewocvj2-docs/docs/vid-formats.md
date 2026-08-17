---
sidebar_position: 6
id: vjformats


---

# A note on video formats and VJ’ing

### “Standard” video formats versus VJ suited formats.

EWOCvj2 loads almost every video format through use of the ffmpeg framework, which supports a myriad of possible file formats. Still, not all formats are suited for VJ use. Especially a lot of commonly used, heavily compressed formats are wrongly composed for VJ’Ing. The most common videos (eg. mp4) use a system using only a few keyframes (full frames) and a lot of intermediate frames (containing the information that changed since the previous frame) to achieve high compression (so smaller video file sizes). This is fine when playing videos forward frame by frame, but VJ use also plays videos backwards, allows you to scrub video back and fro like DJs do with music and often play videos very fast, regularly skipping frames. For all these uses, these high compression systems are not the way to go, because to jump to any random frame means going to the keyframe before it, and decompressing all intermediate frames upto the needed frame. This is slow and places a heavy load on your CPU.  Still, EWOCvj 2 supports these formats too, since when playing the videos forward at normal to slow speeds works well (as long as the CPU doesn't get overburdned, we'll talk about CPU-dependency in a minute).
VJ suited formats are those that have a compressed full keyframe for each and every frame, which allows jumping around at will without slowdowns. Examples of these are MJPEG/PhotoJPEG on CPU and HAP for GPU. Do mind these formats yield very big files. When playing back many layers, investing in an SSD migth be smart…

### On the CPU

First you can use eg. MJPEG/PhotoJPEG to decompress videos on the CPU, your computers main processor. This is limited in that decompressing many layers at the same time will soon tax the CPU and make the overall framerate drop, most certainly when using the preview system. Decompression is multithreaded, so having many processor cores will help greatly.

### The modern VJ approach - on the GPU

A company named VIDVOX has developed a video codec named HAP, which allows the video to be mainly decompressed on the GPU (graphics card) instead of the CPU, offloading decompression stress. This works very well enabling a LOT of video layers to be decompressed concurrently, especially when you have a fast modern graphics card. EWOCvj2 supports transcoding to HAP from within the program. Read more in II.1. EWOCvj does not at
the moment support HAP files with audio

### HAP video codec

HAP is the recommended video codec for EWOCvj2 videos as it compresses on the GPU (which is *fast*) and has a keyframe for every frame (allowing smooth non-linear video access like scrubbing and reverse play), although file sizes are rather substantial. EWOCvj 2 allows transcoding your private video collection to HAP format in-program. This is done in the binsroom (or on-the-fly during playback in the mixroom).

In the BINS room, you can rightclick a video file in the bin and select “HAP encode element” in the menu. The text “Encoding…” will appear in the thumbnail together with a progress bar. When finished a new video file ending in “_hap.mov” will have been created in the same directory as the source video (the original being moved to the stash directory when stashing is set in Preferences). Also the thumbnail will now refer to the HAP version instead of the original.

Then you can right mouseclick a layer file also. This triggers the same process as with normal video files, except that the layer file will be replaced by a new one with the same name, but referring to the newly created HAP file as video source (all other settings/effects/masks being retained). Same goes for rightclicking deck or
mix files. Every single video in those files will be converted and the deck/mix file replaced by a new one referring to the new HAP videos. Of course videos that are already in HAP format will not be converted. Also files that are
queued for transcoding will display “Waiting…” on the thumbnail.

There are two HAP encoding modes for transcoding: Live and Max (in the top rightof the screen, when no file preview is active). Leftclicking the toggle will select one of the two. Live modus transcodes files one thread a time, which leaves at least some CPU power for the mainprogram, allowing for transcoding while performing with the main program. Max modus allows one to use a number of threads equal to the number of CPU cores you have plus 1. This is the ideal amount for mass-transcoding your files but consumes a lot of CPU power… When hovering over a thumbnail, HAP movies will display “HAP” on the thumbnail instead of “CPU”. Do note HAP files created with EWOCvj2 are without audio, but a .wav file containing the audio will be created alongside the video with the same name. This is a design choice with certain advantages. 

In the mix room, you can encode to HAP on-the-fly (layer right mouse menu), meaning it'll process while playing the video in your set on CPU concurrently (replacing the CPU version in the set with the HAP version when it is ready.)

##### 
