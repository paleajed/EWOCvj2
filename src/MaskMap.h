#ifndef MASKMAP_H
#define MASKMAP_H

// Mapping view: every mask made in the layer mapping view (ProjMap) is remembered per beamer, as a closed curve of
// knots (smooth / constant / fluid / broken) that can be edited, and layers or mix monitors can be assigned to one or
// more masks: the element is then projected on those masks, also outside the view.
//
// Coordinates: normalized beamer image, x to the right, y DOWN, origin top left.

#include <cstdint>
#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

class Param;
class Layer;
class EWindow;

extern void maskmap_enter_layer(Layer *lay);        // "Mapping" in a layer menu
extern void maskmap_enter_monitor(int monitor);     // "Mapping" in a mix monitor menu (monitormenu->value)
extern void maskmap_leave();                        // DONE (or another mode took over): keeps everything
extern void maskmap_cancel();                       // CANCEL: reverts the changes made since the view was entered
extern void maskmap_ui();                           // draws + handles the view (only in mapping view mode)
extern void maskmap_tick();                         // every frame: the projection on the beamers
extern unsigned int maskmap_beam_tex(EWindow *win); // what a beamer window of this module shows (EWindow mixid 6)
extern bool maskmap_isparam(Param *p);              // one of the view controls (no global undo / automation)
struct SDL_Window;
// The mouse pointer is not shown at all in this (beamer) window, not even for a moment when it enters the window (Windows: the window
// answers WM_SETCURSOR with no cursor; elsewhere the pointer is hidden by maskmap_tick when it is over a beamer window)
extern void maskmap_hide_pointer(SDL_Window *win, bool hide);
extern void maskmap_layer_transfer(Layer *from, Layer *to);   // Layer::transfer(): 'to' replaces 'from' (a video / image was opened in it): its assignments go along
extern bool maskmap_undo();                        // ctrl-Z in the view
extern bool maskmap_redo();                         // ctrl-Y in the view

// The surface of a detected shape, as the webcam sees it (the "Warp" of a mask): for every cell of a grid over the beamer image
// (gw x gh cells, row 0 = top) the position in the shape's bounding box in the webcam image (0..1 across it, y down; values
// outside it where the shape is extended). aspect: the width / height of that bounding box in webcam pixels.
// A plane fitted to the shape from depth (MoGe, or two webcams): the lookup of the surface again, but with the positions in the
// plane's own box (0..1 across it, along the plane's directions "right" and "down" as the webcam sees them), whose width / height
// is 'aspect' in real proportions. Same grid as the lookup of the surface.
struct WarpPlane {
    bool valid = false;
    float aspect = 1.0f;
    float rms = 0.0f;           // how far the points are from the plane (as a share of the size of the shape / the pairs that did not fit)
    std::vector<float> uv;      // gw * gh * 2
};

struct WarpLut {
    int gw = 0, gh = 0;
    float aspect = 1.0f;
    std::vector<float> uv;      // gw * gh * 2
    std::vector<uint8_t> support;   // gw * gh: 1 where the lookup is known (or goes on from what is known), 0 where nothing is; empty = all
    WarpPlane plane[2];         // 0 from the AI depth, 1 from the two webcams
    // the surface in 3D (for the Viewpoint): for every cell the position of its point on the object, in the frame of webcam 1 (x right,
    // y down, z forward, metres as far as the AI depth goes); z <= 0 = not known. Empty: no depth was there.
    std::vector<float> pos3;    // gw * gh * 3
    float pivot[3] = {0.0f, 0.0f, 1.0f};   // the middle of the shape in that frame: where the viewer's orbit starts
    float hfov = 70.0f;                    // the horizontal field of view of webcam 1 (degrees)
};

// The point cloud of the AI depth of a snapshot of webcam 1 (what the Viewpoint orbits around). 16 bytes a point: x, y, z as floats
// (the frame of webcam 1: x right, y down, z forward) and r, g, b, a bytes.
struct ScenePoints {
    std::vector<uint8_t> data;
    int n = 0;
    float hfov = 70.0f;         // of webcam 1, degrees
    float aspect = 16.0f / 9.0f;
};

// ProjMap: a shape was resolved on the beamer of display index 'disp': its outline (closed polygon, normalized) is kept, with its
// surface when the stripe map gave one (may be null)
extern void maskmap_store_outline(int disp, int w, int h, const std::vector<float> &poly, std::shared_ptr<WarpLut> warp = nullptr);
// the point cloud of the snapshot the shapes were resolved from (for the beamer of display index 'disp'): what the Viewpoint shows
extern void maskmap_set_scene(int disp, std::shared_ptr<ScenePoints> scene);

extern void maskmap_open_file(const std::string &path);     // loads a .epm file as the current mapping set
extern std::string maskmap_current_path();

// the project file: lines to write after the other tags, and the reading of them
extern void maskmap_project_save(std::ostream &o);
extern bool maskmap_project_tag(const std::string &tag, std::istream &is);   // true if the tag was ours (value lines read)
extern void maskmap_project_reset();                                           // a new project: assignments go

#endif
