#ifndef PROJMAP_H
#define PROJMAP_H

// Automated projection mapping.
//
// A webcam looks at the scene that a beamer (an external display) projects on. SAM3 (ComfyUI) segments a webcam
// snapshot; the segments that the user resolves become masks of that beamer (kept by the Mapping view, MaskMap.cpp, where
// layers and mix monitors are assigned to them). The outline of a segment is read off a stripe scan: the beamer shows sine
// stripes, the webcam measures which beamer position lights each of its pixels, and every point of the SAM3 outline gets its
// position in the coordinates of the BEAMER image (no intricate camera/beamer calibration is needed). MoGe depth of the same
// snapshot is estimated as well (to straighten the projection later).
//
// Coordinates: webcam and beamer images are both normalized, x to the right, y DOWN, origin top left.

class Param;

extern void projmap_enter();      // MAPPING button on
extern void projmap_leave();      // MAPPING button off: the UI goes away, results and the projection stay
extern void projmap_cancel();     // CANCEL: stops everything, throws the results away, closes camera + beamer, leaves mode
extern void projmap_ui();         // draws + handles the big window and the controls (only in mapping mode)
extern void projmap_tick();       // every frame, also outside mapping mode: results, beamer steering, beamer image
extern unsigned int projmap_beam_tex();   // what the beamer window shows (EWindow mixid 5)
extern bool projmap_isparam(Param *p);    // one of the mapping controls (no global undo / automation for those)
extern float projmap_exposure();          // the Exposure slider (kept in the preferences)
extern void projmap_set_exposure(float v);

#endif
