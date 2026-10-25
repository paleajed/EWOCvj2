#ifndef LAYERWARP_H
#define LAYERWARP_H

// Layer warp: a free-form network of knots (rows/columns of splines) that warps a layer's output.
// World space: the mix area is [-1,1] x [-1,1], x to the right, y UP on screen.
// Knots are indexed (i, j), i = 0..nx along x (left to right), j = 0..ny along y (bottom to top).

#include <vector>
#include <string>
#include <istream>
#include <ostream>

class Layer;

enum WarpKnotType {
    WK_SMOOTH = 0,
    WK_CONSTANT = 1,
    WK_FLUID = 2,
    WK_BROKEN = 3
};

struct WarpKnot {
    float x = 0.0f, y = 0.0f;
    int typeX = WK_SMOOTH;   // type of the knot in the horizontal (row) direction
    int typeY = WK_SMOOTH;   // type of the knot in the vertical (column) direction
    // handle offsets relative to the knot
    float hxl[2] = {0.0f, 0.0f};   // row direction, towards i-1
    float hxr[2] = {0.0f, 0.0f};   // row direction, towards i+1
    float hyd[2] = {0.0f, 0.0f};   // column direction, towards j-1
    float hyu[2] = {0.0f, 0.0f};   // column direction, towards j+1
};

struct LayerWarpData {
    static constexpr int MAXSTEPS = 32;

    int nx = 1;   // number of divisions (steps) in x -> nx + 1 knot columns
    int ny = 1;
    std::vector<WarpKnot> knots;   // (nx + 1) * (ny + 1), row-major (j * (nx + 1) + i)
    mutable std::vector<WarpKnot> res;     // resolved knots (effective handles), rebuilt by resolve()
    bool active = false;           // false = identity: the layer is not touched at all
    bool dirty = true;             // mesh needs a rebuild
    mutable bool resolved = false;

    // GPU side (not copied)
    unsigned int fbo = (unsigned int) -1;
    unsigned int tex = (unsigned int) -1;
    int texw = 0, texh = 0;
    unsigned int vao = 0, vbo = 0, ibo = 0;
    int nidx = 0;
    int vcap = 0;
    std::vector<float> verts;
    std::vector<unsigned int> idx;

    LayerWarpData();
    LayerWarpData(const LayerWarpData &o);
    LayerWarpData &operator=(const LayerWarpData &o);
    ~LayerWarpData();

    void copy_data_from(const LayerWarpData &o);   // only the knot data
    void init_identity(int nx, int ny);
    void reset();
    void touch();                 // mark changed: recompute active/dirty
    bool is_identity() const;
    void set_steps(int newnx, int newny);   // resample the surface onto a new network

    WarpKnot &K(int i, int j) { return knots[j * (nx + 1) + i]; }
    const WarpKnot &K(int i, int j) const { return knots[j * (nx + 1) + i]; }

    const WarpKnot &R(int i, int j) const { return res[j * (nx + 1) + i]; }

    void resolve() const;
    // set a knot's type in one direction (dirx = true: horizontal); the current shape is kept
    void set_knot_type(int i, int j, bool dirx, int type);

    // evaluation
    void row_seg(int i, int j, float u, float out[2]) const;   // row j, segment between knot i and i+1
    void col_seg(int i, int j, float v, float out[2]) const;   // column i, segment between knot j and j+1
    void eval_cell(int i, int j, float u, float v, float out[2]) const;   // Coons patch
    void eval(float s, float t, float out[2]) const;           // s,t in 0..1 over the whole layer
    // handle end positions of a knot (world space) as used for display; returns false when not available
    bool handle_pos(int i, int j, int which, float out[2]) const;   // which: 0 = xl, 1 = xr, 2 = yd, 3 = yu
    bool handle_available(int i, int j, int which) const;

    float tsc = 1.0f, tox = 0.0f, toy = 0.0f;   // the shift/scale transform the mesh was built with
    void build_mesh(float sc, float ox, float oy);
    void release_gl();

    void write(std::ostream &w) const;
    // reads the lines following a WARPSTEPS tag
    void read_steps(std::istream &r);
    void read_knots(std::istream &r);
};

// Mode handling (called from the UI)
extern void layerwarp_enter();
extern void layerwarp_leave();
extern void layerwarp_cancel();
extern void layerwarp_undo();   // ctrl-z (cmd-z) in warp mode: the warp has its own undo, separate from the global one
extern void layerwarp_redo();   // ctrl-y (cmd-y)
class Param;
extern bool layerwarp_isparam(Param *p);   // one of the warp controls (no global undo / automation for those)
extern void layerwarp_ui();   // draws + handles the canvas when layer warp mode is on

// The layer's final texture: the warped one when layer warp is active, else raw (the layer's fbotex)
extern unsigned int layerwarp_tex(Layer *lay, unsigned int raw);

// Render pass: warp the layer's finished texture; false when the layer isn't warped.
// Shift/scale of a layer as a transform of warp (layer) space to world space: world = sc * layer + (ox, oy)
struct WarpXf { float sc = 1.0f, ox = 0.0f, oy = 0.0f; };
extern WarpXf layerwarp_xf(Layer *lay);

extern bool layerwarp_pass(Layer *lay, unsigned int src, bool stage, unsigned int &outtex, unsigned int &outfbo);

#endif
