---
sidebar_position: 8
title: Projection mapping
---

# Projection mapping

## 

## What is projection mapping?

Projection mapping (also called video mapping or spatial augmented reality) is the art of projecting video onto things that are not a flat white screen: a building, a stack of boxes, a stage set, a sculpture, a canvas on an easel.  Instead of letting the beamer (video projector) light up a rectangle, you tell it exactly where the objects are, so that the picture lands only on them, follows their outlines and, ideally, looks right on them: a square stays square on a wall that is turned away from you, and a video of a sheep looks like a sheep, not like a sheep that has been squeezed around a corner.

Doing this by hand means fitting masks on the objects with a lot of patience, usually in a separate mapping program, and starting over each time the beamer or the objects move.  EWOCvj2 can do most of this measuring for you by using AI assistance.

## 

## How EWOCvj2 does it

EWOCvj2 splits projection mapping into two steps, each with its own view:

1. **Detection** (the *MAPPING* button): a webcam watches the scene that the beamer lights up.  The AI segmentation model SAM3 finds the objects in the webcam image.  Then the beamer shows a series of stripe patterns, the webcam sees them on the objects, and from that the program knows, for every pixel of the webcam, which pixel of the beamer lights it.  With that map the outline of every object you select is traced *in beamer coordinates*, and stored as a **mask** of that beamer.  If the depth of the scene could be estimated (the "AI Depth" estimate) the masks also remember the 3D shape of the surface they are on.
2. **Mapping** (the *Mapping* entry in the menu of a layer or of a mix monitor): here you assign layers and mix monitors to the stored masks, edit the masks as curves if they need it, and choose how the video is placed in each mask: clipped out of the full frame, fitted into the mask, and, where there is a surface, warped to follow it, as seen from the webcam, as a flat plane in real proportions, or from a viewpoint of your own.

The masks are stored per beamer in a mapping file, so the result survives a restart, and, as long as the beamer and the objects stay where they are, you do not have to detect again.  The projection itself runs whenever a mask has something assigned to it, also when the Mapping view is closed.  Assignments are save in the project.

---

## 

## Before you start: what you need

- **A beamer on a second display** (an external screen).  The projection is shown in a borderless fullscreen window on that display, without the taskbar and without the mouse pointer.  The beamer is chosen by name, size and order, not by display number, so the masks find their beamer again when you reconnect it.
- **A webcam** that sees the objects *and the light of the beamer on them*.  Put it close to the beamer, looking the same way; the more it looks along the beamer's beam, the less the objects hide parts of each other's surfaces.  A second webcam, on the other side of the beamer, is optional: surfaces that turn away from one webcam are seen at a better angle by the other.  For optimal results (at least for the first webcam), use a good 4K webcam with a not too wide field-of-view, because more pixels per object give better detected outlines.  I use an ELP 4K webcam with 5-50mm optical zoom with great results.
- A computer that can run the AI models used for segmentation and depth (ComfyUI is started by the program, the first time the models are downloaded).  At least 10Gb of VRAM (or free unified memory) is needed, 12Gb being more appropriate.
- A scene that holds still.  Objects, beamer and webcam must not move between the snapshot, the scan and the show.  If they do, detect again.

---

## Getting the exposure right

This is the most important part of the whole workflow, and the one most often skipped.  **Read it before your first detection.**

Everything the detection knows comes from what the webcam sees.  Two things are measured through it: the *snapshot* (a picture of the objects lit by the beamer, which SAM3 segments) and the *stripe scan* (the stripe patterns, whose phase is read for every webcam pixel).  Both depend on the brightness of the picture being right.

### What goes wrong with a bad exposure

- **Too dark: under-detection.**  The segmentation misses objects or cuts them off at the dark parts, and in the stripe scan the stripes are too weak to read: those pixels get no position at all (shown blue in the *Map* preview).  The result is holes in the map, outlines that cannot be completed ("the stripe map covers too little of the outline, the shape is not resolved"), and warped surfaces with black patches.
- **Too bright: glare and clipping.**  A webcam can only record values up to white.  A bright or glossy surface, a highlight or a light-coloured object at full beamer brightness clips: the stripes then become flat white bands at their peaks, and the phase of the stripes cannot be read there.  Those pixels are marked as saturated (yellow in the *Map* preview) and get no position.  Glare also eats the edges of objects, so the outlines found are wrong exactly where they matter.
- **Auto exposure making it worse.**  Many webcams adjust their exposure continuously.  When the beamer switches pattern, the webcam can start "breathing" and the brightness changes while the program is measuring it, which shows up as bands and wrong positions.  The stripe patterns are designed to keep the average brightness constant, so that auto exposure has little to react to, but a fixed exposure is always better.

The goal is a picture in which *every object you want to map is clearly visible, and nothing you want to map is over bright*.

### Set the exposure in the webcam

If your webcam has exposure controls (the control software that comes with the webcam, or the camera settings of your operating system), use them first:

1. Turn **auto exposure off** and set the exposure by hand, so that the webcam keeps the same brightness during the whole scan.  If you can, also turn off auto white balance and **auto focus**: a webcam that refocuses during the scan changes the magnification of the image, and the map no longer fits the snapshot.  (The program notices and compensates for small changes, but a fixed setting is much safer.)
2. Turn off any *backlight compensation*, *HDR* and *low light* modes.
3. With the room as it will be during the show (room lights as dark as the show needs them), set the exposure so that the beamer-lit objects look bright but not washed out.

EWOCvj2 does not change the webcam's own settings; it reads whatever the webcam delivers.

### 

### Set the exposure with the detection view's Exposure slider

The detection view has its own control, the **Exposure** slider in the control panel.  It sets *how bright the beamer lights the scene* (an even grey) when the snapshot is taken, and it is also the brightness of the stripes in the scan.  It is the quick way to bring the scene into the range of the webcam, whatever the webcam's own settings are:

- Lower it when the snapshot is overexposed.
- Raise it when it is too dark for the segmentation.

The **EXPOSE** button next to the slider makes the beamer show that grey continuously, whenever it is not busy with a scan.  So the procedure is:

1. Click **EXPOSE**.  The beamer lights the scene evenly.
2. Look at the webcam image in the detection view while you move the **Exposure** slider.  The default is 0.7.
3. Make the objects clearly visible.  Look for the brightest object (a white or shiny one): lower the slider until it is no longer pure white.  Look for the darkest object you want to map: raise the slider until its edge is clear against the background.  If the two cannot both be satisfied with one setting, the difference in brightness in the scene is too large for the webcam: add light, change the webcam's own exposure, or accept that you will detect these objects in separate passes (see *Segmenting in several passes*, below, and re-set the exposure between passes).
4. Draw a **selection box** (see below) around a dim part of the image: while you draw it, the rest of the image is dimmed, which makes it easier to judge the box's content.
5. Click **EXPOSE** again to turn it off when you are done.  (While the program is busy with the beamer, for the snapshot or a scan, the grey gives way to what the program needs; it comes back afterwards as long as EXPOSE is on.)

The Exposure slider is remembered in the preferences, so you only calibrate again when the room, the beamer or the webcam changes.

### 

### The automatic brightness reduction of the scan

Just before the stripe scan, the program takes a reference picture of the scene lit white at the Exposure level.  This is as bright as the peaks of the stripes will be.  If many of the pixels of the shapes you are about to scan would clip at that level (a glossy rim, a highlight), the brightness of the stripes is **lowered for that scan** until almost nothing clips, and the log says so.  This is a safety net, not a substitute for a decent exposure: lowering the stripes lowers their contrast against the webcam's noise, so the dark parts of the scene get weaker stripes.

### 

### Check the result with the Map preview

After a scan, set **Preview** to **Map**.  The beamer shows an even light and the webcam image is painted with what the scan made of every pixel:

| Colour      | Meaning                                                             | What to do                                                   |
| ----------- | ------------------------------------------------------------------- | ------------------------------------------------------------ |
| faint green | has a position                                                      | fine                                                         |
| yellow      | saturated in several stripe patterns, no position (glare)           | lower the exposure                                           |
| blue        | stripes too weak (shadow, dark surface, outside the beamer's light) | raise the exposure, add light, move the beamer               |
| red         | the stripe periods disagree with each other                         | usually motion or a flickering light; keep everything still  |
| magenta     | ambiguous                                                           | usually a very dark or very glossy area                      |
| orange      | dropped because it disagrees with its neighbours                    | normal at depth edges; only a problem if it covers an object |

You want the objects you map to be green.  Yellow in the middle of an object means too much exposure, blue means too little.  Change the exposure and detect again.

---

## 

## The detection view

Click **MAPPING** in the mixing screen to open the detection view.  It shows the webcam image in the big window, with a control panel on its left.  The status bar below the image lists the steps of the current job.

### Choose the devices

- **Webcam**: the webcam that watches the scene.  The list is refreshed when you open it.
- **Webcam 2**: optional second webcam (see above).  Next to it is the **V** button, which switches the big window to the image of webcam 2 alone, to aim it; click again to go back.
- **Beamer**: the external display used for the projection.  Choosing it opens the beamer window on that display.
- **FOV / FOV 2**: the *horizontal* field of view of the webcams, in degrees, from the specifications.  Leave at **0** for automatic (what the AI Depth estimate finds for each snapshot).
- **Method**: *Outline* reads every point of the segmentation outline off the stripe map, with both webcams if webcam 2 is on.  *Knots curve* reads only the knots of a smooth curve through the shape (the **Knots** and **Corner threshold** sliders, which are shown only for this method) and traces the curve through them.  Outline is more exact, Knots curve smoother.
- **Exposure** and **EXPOSE**: see above.
- **Preview**: *Outline* shows the outline of the mask as a thin white line on the beamer, to check how well it follows the object; *Map* shows the scan result as described above.

### 

### Segment

Click **SEGMENT**.  The program

1. takes a snapshot of the webcam image (the beamer lights the scene with the Exposure grey first),
2. starts the AI server if it is not running yet,
3. lets SAM3 find the objects in the picture,
4. builds the outlines,
5. estimates the depth of the scene ("AI Depth").

The segments appear on the image, each with its own colour.  **Click a segment to select it for resolving**: a selected segment gets a white outline and a tint.  Clicking again deselects it; where segments overlap, repeated clicks on the same place cycle through the segments under the pointer.

- **SEGMENT** becomes **STOP** while it runs.
- **PROMPT**: instead of segmenting everything, you can type a text (for example "white box", dont use plural) that SAM3 should look for.  Click PROMPT, type, press ENTER.  Clicking PROMPT again while the text box is open closes it.
- **Selection box**: drag a rectangle on the image to segment only that part (in more detail).  The part outside the box is dimmed.  Click on the image without dragging to remove the box.
- **Zoom**: the mouse wheel zooms towards the pointer; the middle mouse button pans a zoomed image.

### 

### Segmenting in several passes

Segmenting again does **not** throw the earlier segments away: the new segments are *added* to the existing ones, each with a colour of its own, also when they cover the same pixels as an old one.  This allows a workflow for difficult scenes:

1. Set the exposure for one part of the scene (EXPOSE, the Exposure slider, the webcam's own settings).
2. Draw a selection box around that part and click SEGMENT.
3. Select the segments you want.
4. Change the exposure for another part, draw a new selection box, SEGMENT again, select the segments.
5. Click **RESOLVE** once: everything selected, from all passes, is traced in one global scan. 

#### Make it a habit: box select and segment object by object

For the best outlines, get into the habit of **not** segmenting the whole image in one go, but of drawing a selection box around **one object (or a small group) at a time** and clicking SEGMENT.  This is the most effective thing you can do for the quality of your masks, for these reasons:

- **Resolution.**  SAM3 does not look at your webcam image at its full size: the picture it is given is processed at a fixed, limited working resolution, and the mask it returns has a limited number of pixels as well.  Given the whole webcam image, a box in the middle of the scene is only a small part of those pixels, so its outline is made of a few coarse steps that are scaled up afterwards.  Given only the box, the object fills the model's picture, and its outline is built from many more pixels.  The difference is easily a factor of several on small or distant objects, and it shows in the edge of the projection as wobble, rounded corners and a "stair step" feel.  The outline you get now is also the outline that the stripe scan follows point by point, so the precision you win here is precision in the final mask.
- **Fewer wrong merges and misses.**  In a busy scene SAM3 may join two touching objects into one segment, split an object in two, or skip a small one.  In a box with one object in it, there is little to confuse it.  A box around a stack of boxes and a prompt such as "white box" often does better than the whole image with no prompt.
- **A separate exposure per object.**  The bright object and the dark one can each get their own exposure (see above), instead of one setting that compromises on both.
- **Quick to redo.**  If one outline is not good, you segment just that object again, with a tighter box, and you keep all the others.  The new segment is added next to the old one, so you can compare them and select the better one.

How to work:

1. Click **EXPOSE** and set the exposure for the object you are going to do.
2. Draw the selection box around it.  Leave a small margin (a few percent of the object's size) so that the whole edge is inside the box, but do not include more neighbours than needed.  Draw it a bit larger than the object rather than tight: a box that cuts through the object will cut the outline as well.
3. Click **SEGMENT**.  Zoom in with the mouse wheel and look at the white outline after selecting the segment: is it following the edge?
4. If it is not, move or resize the box (or turn on PROMPT and describe the object) and segment again.  When you are happy, select the segment.
5. Go on to the next object.  When all objects are done, click **RESOLVE** once.

Segmenting the whole image is fine for a first look at the scene, or for large simple objects (a wall, a screen), where the resolution of the outline hardly matters.  For everything with detail, box select.  You can do multiple RESOLVE steps alos, if so you desire. 

#### Depth and CLEAN when you segment several times

The depth estimate is only made again when the scene has changed or the webcam has moved since the snapshot it was made from (a quick comparison of the two snapshots); otherwise it is kept, so the second and later passes are faster, and all segments are placed against the same depth.

The webcam and the objects must not move between the passes.  If they do, click **CLEAN**: it wipes all segmentations and the depth estimate (masks that you resolved already stay in the layer/mix mapping view), so that you can start over clean.

### Resolve

**RESOLVE** traces all selected segments on the beamer.  The beamer shows the stripe patterns one after another (64 of them: eight stripe widths, four phase shifts each, in x and in y) and the webcam reads them.  Keep the room still while this runs; do not touch the webcam or beamer.

The width of the stripes is the same on the wall whatever the resolution of the beamer (the periods scale with the beamer's width), so that a 4K beamer gets the same fine and coarse stripes a Full-HD one does.

When the scan is done the outlines of the selected shapes are known in beamer coordinates.  The shapes are then stored as masks of this beamer in the current mapping file (see below).  With *Webcam 2* on, the two webcams' views of the same shape are matched through the beamer and merged.

Shapes that could not be read well enough (the stripe map covers too little of the outline) are reported in the status bar and the log, and are not stored.  Look at the Map preview to see why, fix the exposure or the setup, and resolve again.

**CANCEL** stops everything that is running, throws the detection away, closes the webcams and the beamer window, and leaves the view.  Resolved masks that are already stored stay stored.

When you are finished, click **MAPPING** again to leave the view.  The masks, and whatever is assigned to them, stay in use.

---

## The Mapping view

Detection only finds and stores masks; it does not project anything yet.  To project, open the **Mapping view** from the menu of the thing you want to project: right-click a **layer** and choose **Mapping**, or right-click a **mix monitor** (Monitor A, Monitor B, the mix, or the preview mix) and choose **Mapping**.  The view opens for that layer or monitor, which is named at the top of the canvas.

The canvas shows the beamer image with the stored masks as outlines.  Masks drawn in light blue are not used by the layer or monitor, **orange** ones have it assigned, and the **white** one is selected.  The mouse wheel zooms, the middle mouse button pans.

### The panel

- **Beamer**: which beamer's masks are shown.  The first time a beamer is used it shows black until it has masks.
- **Name** and **OPEN**: the name of the mapping file (without extension) and a file requester for loading another one.  Click the name to edit it, ENTER saves under the new name.
- **EXPOSE** and **Exposure color**: EXPOSE fills the *selected outline* on the beamer with the exposure colour (instead of the projection), to see how well it fits the real object, or to fit it.  The colour is chosen with the colour control.
- **EDIT** and **ASSIGN**: described below.
- **DONE** keeps everything, saves and leaves the view; **CANCEL** reverts all changes made since the view was opened and leaves.
- **NEW MASK** and **DELETE MASK**: described below.

### Selecting and assigning

**Click an outline to select it.  Clicking again at the same place selects the next outline under the pointer; clicking on nothing deselects.

**ASSIGN** toggles whether the layer or monitor is projected on the selected outline.  Several layers or monitors can be assigned to the same mask, and one layer can be assigned to several masks.  Where masks of different sources overlap on the beamer, the sources are mixed in equal parts.

Assignments are saved in the **project** and move along when a video, layer file or image is opened in the layer.  When a project is opened the projection starts by itself, as soon as the beamer is connected and the layers are loaded.

### Placing the video in an outline

Once an outline is selected, the controls for how the video goes into that outline appear:

- **Content**
  - *Fit* (the default): the whole video frame is placed inside the outline.
  - *Clip*: the full beamer frame is simply cut out by the outline.  The video appears at the position it has in the frame, as if the mask were a window.
  - *Grid*: a test grid instead of the picture, placed like *Fit*.  An even grid that follows the surface means a good map; gaps and kinks mean holes or wrong readings in it.
- **Placement** (with Fit or a warp): *Fit* keeps all of the frame in proportion; *Fill* covers the outline in proportion; *Stretch* stretches the frame to the proportions of the outline.
- **Warp** (only for outlines that came from detection, which have a surface):
  - *Off*: flat.
  - *Surface*: the video follows the surface as the webcam saw it.  This is exact *from the viewpoint of the webcam*; from other places it looks stretched.
  - *Plane (AI Depth)*: a plane is fitted to the shape from the depth estimate and the video is laid on it in real proportions: a square stays a square on a wall that is turned away.  The plane is turned to the edges of the object.
  - *Plane (2 webcams)*: the same, with the plane found from the two webcams (needs the Outline method and webcam 2 on).
  - *Viewpoint (AI Depth)*: see below.
  - Any warp implies *Fit*.  A mask made by hand with NEW MASK has no surface, and shows neither Warp nor Grid.
- **Rotate**, **Scale**, **Shift X**, **Shift Y**: turn, size and move the video inside the outline.
- **Inset**: moves the outline inwards by that many beamer pixels when something is projected, so that the picture stays clear of the edge of the object.  Every outline has its own.
- **Feather**: an alpha feathered edge.  The projection fades out over that many beamer pixels at the edge of the outline (after the Inset), growing inwards, instead of stopping sharply.  0 (the default) is a sharp edge.  Every outline has its own.

### The Viewpoint

A warp can only be right for one place to stand.  *Surface* is right for the webcam; *Plane* is right for flat faces.  For an audience somewhere else, or an object with several faces in one outline, use **Warp = Viewpoint (AI Depth)**.

Each outline gets a *viewer*: where the audience stands.  The **VIEWPOINT** button shows the scene as a point cloud (from the AI Depth estimate of webcam 1) as seen from that viewer, with the outline in orange.  It starts exactly at webcam 1, looking along its axis with its field of view, so the first view is the webcam's own picture.  Move the viewer like in the Camera Warp (see Gen Room) editor:

- **Alt + left mouse drag**: orbit.
- **Alt + middle mouse drag**: pan.
- **Mouse wheel**: closer / further.
- **Ctrl + mouse wheel**: move the pivot in depth.

The projection on the outline follows live (a small copy of the beamer image is shown in the corner).  **RESET VIEW** puts the viewer back at the webcam.  The viewpoint is saved with the mask.  While the view is on, clicks on the canvas do not select outlines; click VIEWPOINT again to go back.

### Editing outlines (EDIT)

The stored outline is a closed curve through knots.  A new outline from detection is described by as few knots as possible: corners get *Broken* or *Constant* knots, curved parts *Smooth* or *Fluid* ones, up to the **Knots** slider.  Turn **EDIT** on to change them:

- Drag knots and handles; box select knots by dragging on empty space (SHIFT adds); move the selected knots by dragging or with the arrow keys (one beamer pixel, SHIFT: ten); remove them with DEL; add a knot by clicking on the curve.  The projection follows live.
- **SMOOTH / CONSTANT / FLUID / BROKEN** set the type of the selected knots: smooth knots get their handles automatically, constant knots have none (straight to the neighbours), fluid knots have two handles that always lie opposite each other, broken knots have two independent handles for corners.
- **Knots**: describes the selected outline again with at most that many knots.
- **Ctrl+Z / Ctrl+Y** undo and redo.

### NEW MASK and DELETE MASK

**NEW MASK** makes a square mask in the middle of the beamer image, described by four Constant knots: shape it with the knot tools.  It is not a pixel mask: the curve itself is the mask, rasterized at the beamer's full resolution.  Use it for areas that detection did not give you, or for parts of the picture you want to place by hand, or when you don't have the hardware to use AI detection.  It has no surface, so Warp and Grid are not available for it.

**DELETE MASK** removes the selected mask, with everything assigned to it.

---

## 

## Files and projects

Masks are stored in a **mapping file** (`.epm`) in the folder `mappings`, next to the generations folder.  Everything is in that one file: the curves of the masks, how the video is placed in them, and the data that came from detection (the surfaces of the masks, with their planes and 3D positions, and the point clouds used by the viewpoints).  It is saved automatically after every change.  Mapping files made by older versions, which kept the detection data in `.epw` and `.epc` files next to the `.epm`, are still read, and are saved as one file the next time they change.

The mapping file does not belong to a single project: it can be used in all of them, and in other projects with **OPEN**.  The *project* stores the path of the mapping file in use and the assignments of layers and monitors to its masks.

If the beamer is not connected when you open a project, nothing is projected for it; connect it and the projection starts.  Beamers are recognised by name, size and order among identical displays.

---

## Tips and troubleshooting

- **Detect with the room as it will be.**  The ambient light you detect in is the ambient light the webcam saw when it decided what is bright and what is dark.
- **Everything holds still.**  Anchor the webcam and the beamer firmly (a tripod is not enough if it is on a stage).  Only a small motion of the webcam image is compensated for.
- **Matte beats glossy.**  Glossy and mirror-like surfaces give glare; if you cannot avoid them, lower the exposure for the pass in which you detect them.
- **Not everything resolves.**  Check the Map preview first.  Large blue or yellow areas inside the object are an exposure problem, not a program problem.
- **Wrong rotation in a Plane warp.**  Planes line up with the edges of the object; for round or irregular objects there are no edges and they follow the webcam's axes.  Rotate the video with **Rotate**.
- **Warp looks stretched from the audience.**  Use *Viewpoint*, and put the viewer where the audience is.
- **The detection does not find a small or dim object.**  Draw a selection box around it and segment again, after raising the exposure for it.
- **The beamer shows a white flash or black.**  White is the snapshot light; black is the projection with nothing assigned yet (or a layer that is not loaded); if the webcam image is too dark to box select objects for detection, press the EXPOSE button to light the scene.
