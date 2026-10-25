#ifndef USE_GLES
#include "GL/glew.h"
#include "GL/gl.h"
#endif
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <thread>
#include <utility>
#include <vector>
#include "program.h"
#include "MaskMap.h"
#include "Camera3D.h"
#include "ProjMap.h"
#include "LayerWarp.h"

namespace fs = std::filesystem;

extern void output_video(EWindow *);  // program.cpp (the display thread of an output window)

namespace {

// ======================================================================================================
// Data model
// ======================================================================================================

enum KnotType { KT_SMOOTH = 0, KT_CONST = 1, KT_FLUID = 2, KT_BROKEN = 3 };

// A knot of the closed mask curve. Handles are offsets from the knot. SMOOTH: tangent from the neighbours (the stored handles
// are ignored), CONSTANT: no handles, FLUID: hin = -hout (only hout is used), BROKEN: both handles are free.
struct MapKnot {
    float x = 0.0f, y = 0.0f;
    int type = KT_SMOOTH;
    float hin[2] = {0.0f, 0.0f}, hout[2] = {0.0f, 0.0f};
};

// The surface lookup for the viewer of a mask (Warp "Viewpoint"): the 3D positions of the surface (WarpLut::pos3) seen from the
// viewpoint, as positions in the box of the shape as the viewer sees it. Made again whenever the viewpoint changes.
struct ViewCache {
    unsigned ver = 0;               // g_viewver it was made for
    std::vector<float> uv;          // gw * gh * 2
    std::vector<uint8_t> support;   // gw * gh
    float aspect = 1.0f;
    std::vector<int> core;          // the cells of the shape itself (the box is of these)
};

struct MapMask {
    int id = 0;
    std::vector<MapKnot> k;
    // cache of the flattened curve (normalized), with for each sample the segment and the t on it
    std::vector<float> poly;
    std::vector<int> pseg;
    std::vector<float> pt;
    float area = 0.0f;
    bool pdirty = true;
    bool ctrl = false;          // a control curve (NEW MASK): the curve is the mask; Edit outlines cannot be left while it is selected
    float inset = 0.0f;         // beamer pixels this mask is moved inwards (after its curve edits) when something is projected on it
    float feather = 0.0f;       // beamer pixels over which the projection fades out at the edge of the mask (inside it, after the inset)
    // What is projected on it: Content 0 = Clip (the full beamer frame cut out by the mask), 1 = Fit (the whole frame placed in the
    // mask); Warp = the Fit follows the surface the webcam saw (needs 'warp'); Placement 0 = Fit, 1 = Fill, 2 = Stretch
    int content = 1;            // (the menu lists Fit first, Clip second: see content_ui)
    int warpon = 0;
    int place = 0;
    float rot = 0.0f, scale = 1.0f, shx = 0.0f, shy = 0.0f;   // rotation (degrees), size and shift of the frame in the mask
    std::shared_ptr<WarpLut> warp;                            // its surface (null: none; shared by the undo copies)
    OrbitCamera cam;                                          // the viewpoint (Warp "Viewpoint"): where the viewer is, as in the Camera Warp editor
    bool camset = false;                                      // 'cam' has been set (else it starts at webcam 1)
    std::shared_ptr<ViewCache> vc;                            // (not saved)
};

// The Content menu lists Fit first and Clip second, the mask keeps its values (0 Clip, 1 Fit, 2 Grid, so that saved files stay
// valid): this turns a menu entry into the value and back (Clip and Fit swap)
inline int content_ui(int v) { return v == 0 ? 1 : (v == 1 ? 0 : v); }

struct BeamerMaps {
    std::string key;
    int w = 1920, h = 1080;
    std::vector<MapMask> masks;
    int nextid = 1;
    std::shared_ptr<ScenePoints> scene;   // the point cloud of the snapshot the shapes were resolved from (the Viewpoint orbits it)
};

struct Assign {
    std::string key;
    int maskid = 0;
    int kind = 0;               // 0 layer, 1 monitor
    Layer *lay = nullptr;       // (nullptr: pending, found again from plist / pidx)
    int mon = 0;
    int plist = -1, pidx = -1;  // place of the layer in mainmix->layers (project file)
};

struct Elem {
    int kind = -1;              // 0 layer, 1 monitor
    Layer *lay = nullptr;
    int mon = 0;
};

std::vector<BeamerMaps> g_set;          // the current mapping set
std::string g_name = "mapping";         // its file name (without .epm)
std::string g_path;                     // the file it was last saved to / loaded from
bool g_dirty = false;                   // changed since it was saved
unsigned g_ver = 1;                     // counts every change of masks / assignments (the projection is rebuilt from it)
struct Move { Layer *first, *second; int stale = 0; };
std::vector<Move> g_moves;              // layers replaced by new ones (Layer::transfer): old -> new
int g_loadgrace = 0;                   // frames left after a project was opened in which assignments wait for their layers
unsigned g_viewver = 1;                // counts every change of a viewpoint (only the surface lookups of the viewpoint masks are made again)
std::vector<Assign> g_assign;
int g_knotbudget = 16;
bool g_loadedonce = false;

float g_lb[4] = {0.45f, 0.75f, 1.0f, 1.0f};
float g_or[4] = {1.0f, 0.55f, 0.0f, 1.0f};
float g_wh[4] = {1.0f, 1.0f, 1.0f, 1.0f};
float g_tr[4] = {0.0f, 0.0f, 0.0f, 0.0f};
float g_ye[4] = {1.0f, 0.9f, 0.2f, 1.0f};
float g_gr[4] = {0.3f, 0.9f, 0.4f, 1.0f};
float g_pk[4] = {1.0f, 0.4f, 0.8f, 1.0f};
float g_ex[4] = {1.0f, 1.0f, 1.0f, 1.0f};
float g_mm_clip[4] = {-2.0f, -2.0f, 2.0f, 2.0f};   // the rectangle (NDC) the lines of the view are clipped to

BeamerMaps *find_maps(const std::string &key, bool create) {
    for (BeamerMaps &b : g_set) if (b.key == key) return &b;
    if (!create) return nullptr;
    g_set.push_back(BeamerMaps());
    g_set.back().key = key;
    return &g_set.back();
}

MapMask *find_mask(BeamerMaps *bm, int id) {
    if (!bm) return nullptr;
    for (MapMask &m : bm->masks) if (m.id == id) return &m;
    return nullptr;
}

bool layer_alive(Layer *l) {
    if (!l) return false;
    for (auto &sm : mainmix->layers) {
        if (std::find(sm.begin(), sm.end(), l) != sm.end()) return true;
    }
    return false;
}

// ======================================================================================================
// Curve
// ======================================================================================================

void resolve(const std::vector<MapKnot> &k, std::vector<float> &ein, std::vector<float> &eout) {
    int n = (int) k.size();
    ein.assign(n * 2, 0.0f);
    eout.assign(n * 2, 0.0f);
    for (int i = 0; i < n; i++) {
        const MapKnot &c = k[i];
        if (c.type == KT_SMOOTH) {
            const MapKnot &pv = k[(i + n - 1) % n], &nx = k[(i + 1) % n];
            float tx = (nx.x - pv.x) / 6.0f, ty = (nx.y - pv.y) / 6.0f;
            eout[i * 2] = tx; eout[i * 2 + 1] = ty;
            ein[i * 2] = -tx; ein[i * 2 + 1] = -ty;
        }
        else if (c.type == KT_FLUID) {
            eout[i * 2] = c.hout[0]; eout[i * 2 + 1] = c.hout[1];
            ein[i * 2] = -c.hout[0]; ein[i * 2 + 1] = -c.hout[1];
        }
        else if (c.type == KT_BROKEN) {
            eout[i * 2] = c.hout[0]; eout[i * 2 + 1] = c.hout[1];
            ein[i * 2] = c.hin[0]; ein[i * 2 + 1] = c.hin[1];
        }
    }
}

// the stored handles become what is in effect (before a knot changes its type: the shape stays)
void bake_knot(std::vector<MapKnot> &k, int i) {
    std::vector<float> ein, eout;
    resolve(k, ein, eout);
    k[i].hin[0] = ein[i * 2]; k[i].hin[1] = ein[i * 2 + 1];
    k[i].hout[0] = eout[i * 2]; k[i].hout[1] = eout[i * 2 + 1];
}

void flatten(MapMask &m, int w, int h) {
    m.poly.clear(); m.pseg.clear(); m.pt.clear();
    m.area = 0.0f;
    m.pdirty = false;
    int n = (int) m.k.size();
    if (n < 2) return;
    std::vector<float> ein, eout;
    resolve(m.k, ein, eout);
    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;
        float p0x = m.k[i].x, p0y = m.k[i].y;
        float p1x = p0x + eout[i * 2], p1y = p0y + eout[i * 2 + 1];
        float p3x = m.k[j].x, p3y = m.k[j].y;
        float p2x = p3x + ein[j * 2], p2y = p3y + ein[j * 2 + 1];
        auto pl = [&](float ax, float ay, float bx, float by) { return std::hypot((ax - bx) * (float) w, (ay - by) * (float) h); };
        float L = pl(p0x, p0y, p1x, p1y) + pl(p1x, p1y, p2x, p2y) + pl(p2x, p2y, p3x, p3y);
        int s = std::clamp((int) (L / 6.0f) + 4, 8, 64);
        for (int q = 0; q < s; q++) {
            float t = (float) q / (float) s, u = 1.0f - t;
            float b0 = u * u * u, b1 = 3.0f * u * u * t, b2 = 3.0f * u * t * t, b3 = t * t * t;
            m.poly.push_back(b0 * p0x + b1 * p1x + b2 * p2x + b3 * p3x);
            m.poly.push_back(b0 * p0y + b1 * p1y + b2 * p2y + b3 * p3y);
            m.pseg.push_back(i);
            m.pt.push_back(t);
        }
    }
    int np = (int) m.poly.size() / 2;
    double a = 0.0;
    for (int i = 0; i < np; i++) {
        int j = (i + 1) % np;
        a += (double) m.poly[i * 2] * m.poly[j * 2 + 1] - (double) m.poly[j * 2] * m.poly[i * 2 + 1];
    }
    m.area = (float) std::fabs(a * 0.5);
}

void ensure_flat(MapMask &m, const BeamerMaps &bm) {
    if (m.pdirty) flatten(m, bm.w, bm.h);
}

bool point_in_poly(const std::vector<float> &p, float x, float y) {
    int n = (int) p.size() / 2;
    bool in = false;
    for (int i = 0, j = n - 1; i < n; j = i++) {
        float xi = p[i * 2], yi = p[i * 2 + 1], xj = p[j * 2], yj = p[j * 2 + 1];
        if (((yi > y) != (yj > y)) && (x < (xj - xi) * (y - yi) / (yj - yi) + xi)) in = !in;
    }
    return in;
}

// splits segment 'seg' at t (de Casteljau): the shape does not change; returns the index of the new knot
int insert_knot(std::vector<MapKnot> &k, int seg, float t) {
    int n = (int) k.size();
    int j = (seg + 1) % n;
    std::vector<float> ein, eout;
    resolve(k, ein, eout);
    float P0[2] = {k[seg].x, k[seg].y};
    float P1[2] = {P0[0] + eout[seg * 2], P0[1] + eout[seg * 2 + 1]};
    float P3[2] = {k[j].x, k[j].y};
    float P2[2] = {P3[0] + ein[j * 2], P3[1] + ein[j * 2 + 1]};
    auto lerp = [](const float *a, const float *b, float tt, float *o) { o[0] = a[0] + (b[0] - a[0]) * tt; o[1] = a[1] + (b[1] - a[1]) * tt; };
    float Q0[2], Q1[2], Q2[2], R0[2], R1[2], S[2];
    lerp(P0, P1, t, Q0); lerp(P1, P2, t, Q1); lerp(P2, P3, t, Q2);
    lerp(Q0, Q1, t, R0); lerp(Q1, Q2, t, R1);
    lerp(R0, R1, t, S);
    MapKnot nk;
    nk.x = S[0]; nk.y = S[1];
    nk.hin[0] = R0[0] - S[0]; nk.hin[1] = R0[1] - S[1];
    nk.hout[0] = R1[0] - S[0]; nk.hout[1] = R1[1] - S[1];
    float li = std::hypot(nk.hin[0], nk.hin[1]), lo = std::hypot(nk.hout[0], nk.hout[1]);
    float sm = std::hypot(nk.hin[0] + nk.hout[0], nk.hin[1] + nk.hout[1]);
    nk.type = (sm <= 0.05f * std::max(li, lo) + 1e-6f) ? KT_FLUID : KT_BROKEN;
    // the neighbours keep the shape of the two halves
    auto fix = [&](int idx, bool outer, const float *Hq) {
        MapKnot &c = k[idx];
        if (c.type == KT_CONST) return;
        if (c.type != KT_BROKEN) {
            c.hin[0] = ein[idx * 2]; c.hin[1] = ein[idx * 2 + 1];
            c.hout[0] = eout[idx * 2]; c.hout[1] = eout[idx * 2 + 1];
            c.type = KT_BROKEN;
        }
        if (outer) { c.hout[0] = Hq[0]; c.hout[1] = Hq[1]; }
        else { c.hin[0] = Hq[0]; c.hin[1] = Hq[1]; }
    };
    float h1[2] = {Q0[0] - P0[0], Q0[1] - P0[1]};
    float h2[2] = {Q2[0] - P3[0], Q2[1] - P3[1]};
    fix(seg, true, h1);
    fix(j, false, h2);
    k.insert(k.begin() + seg + 1, nk);
    return seg + 1;
}

// ======================================================================================================
// Outline fit: a closed polygon -> as few knots as possible (corners are Broken / Constant, curved parts Smooth / Fluid)
// ======================================================================================================

struct D2 { double x = 0.0, y = 0.0; };
inline D2 operator+(D2 a, D2 b) { return {a.x + b.x, a.y + b.y}; }
inline D2 operator-(D2 a, D2 b) { return {a.x - b.x, a.y - b.y}; }
inline D2 operator*(D2 a, double s) { return {a.x * s, a.y * s}; }
inline double dot(D2 a, D2 b) { return a.x * b.x + a.y * b.y; }
inline double len(D2 a) { return std::sqrt(dot(a, a)); }
inline D2 unit(D2 a) { double l = len(a); return l > 1e-12 ? D2{a.x / l, a.y / l} : D2{0.0, 0.0}; }

struct Bz { D2 c[4]; };

D2 bz_at(const Bz &b, double t) {
    double s = 1.0 - t;
    return b.c[0] * (s * s * s) + b.c[1] * (3.0 * s * s * t) + b.c[2] * (3.0 * s * t * t) + b.c[3] * (t * t * t);
}
D2 bz_d1(const Bz &b, double t) {
    D2 a = (b.c[1] - b.c[0]) * 3.0, bb = (b.c[2] - b.c[1]) * 3.0, c = (b.c[3] - b.c[2]) * 3.0;
    double s = 1.0 - t;
    return a * (s * s) + bb * (2.0 * s * t) + c * (t * t);
}
D2 bz_d2(const Bz &b, double t) {
    D2 a = (b.c[2] - b.c[1] * 2.0 + b.c[0]) * 6.0, c = (b.c[3] - b.c[2] * 2.0 + b.c[1]) * 6.0;
    return a * (1.0 - t) + c * t;
}

std::vector<double> chord_param(const std::vector<D2> &d, int first, int last) {
    std::vector<double> u(last - first + 1, 0.0);
    for (int i = first + 1; i <= last; i++) u[i - first] = u[i - first - 1] + len(d[i] - d[i - 1]);
    double tot = u.back();
    if (tot > 1e-12) for (double &v : u) v /= tot;
    return u;
}

Bz generate_bezier(const std::vector<D2> &d, int first, int last, const std::vector<double> &u, D2 t1, D2 t2) {
    int n = last - first + 1;
    double C00 = 0, C01 = 0, C11 = 0, X0 = 0, X1 = 0;
    Bz b;
    b.c[0] = d[first];
    b.c[3] = d[last];
    for (int i = 0; i < n; i++) {
        double ui = u[i], s = 1.0 - ui;
        double b0 = s * s * s, b1 = 3.0 * ui * s * s, b2 = 3.0 * ui * ui * s, b3 = ui * ui * ui;
        D2 A1 = t1 * b1, A2 = t2 * b2;
        C00 += dot(A1, A1); C01 += dot(A1, A2); C11 += dot(A2, A2);
        D2 tmp = d[first + i] - (d[first] * (b0 + b1) + d[last] * (b2 + b3));
        X0 += dot(A1, tmp); X1 += dot(A2, tmp);
    }
    double det = C00 * C11 - C01 * C01;
    double al = 0.0, ar = 0.0;
    if (std::fabs(det) > 1e-12) {
        al = (X0 * C11 - X1 * C01) / det;
        ar = (C00 * X1 - C01 * X0) / det;
    }
    double seg = len(d[last] - d[first]), eps = 1e-6 * seg;
    if (al < eps || ar < eps) al = ar = seg / 3.0;
    b.c[1] = b.c[0] + t1 * al;
    b.c[2] = b.c[3] + t2 * ar;
    return b;
}

double max_error(const std::vector<D2> &d, int first, int last, const Bz &b, const std::vector<double> &u, int &split) {
    double maxd = 0.0;
    split = (first + last) / 2;
    for (int i = first + 1; i < last; i++) {
        D2 df = bz_at(b, u[i - first]) - d[i];
        double dist = dot(df, df);
        if (dist >= maxd) { maxd = dist; split = i; }
    }
    return maxd;
}

void reparameterize(const std::vector<D2> &d, int first, int last, std::vector<double> &u, const Bz &b) {
    for (int i = first; i <= last; i++) {
        double t = u[i - first];
        D2 q = bz_at(b, t) - d[i];
        D2 q1 = bz_d1(b, t), q2 = bz_d2(b, t);
        double num = dot(q, q1), den = dot(q1, q1) + dot(q, q2);
        if (std::fabs(den) > 1e-12) t -= num / den;
        u[i - first] = std::clamp(t, 0.0, 1.0);
    }
}

void fit_cubic(const std::vector<D2> &d, int first, int last, D2 t1, D2 t2, double tol, std::vector<Bz> &out) {
    int n = last - first + 1;
    if (n == 2) {
        double dist = len(d[last] - d[first]) / 3.0;
        Bz b;
        b.c[0] = d[first]; b.c[3] = d[last];
        b.c[1] = b.c[0] + t1 * dist;
        b.c[2] = b.c[3] + t2 * dist;
        out.push_back(b);
        return;
    }
    std::vector<double> u = chord_param(d, first, last);
    Bz b = generate_bezier(d, first, last, u, t1, t2);
    int split;
    double maxe = max_error(d, first, last, b, u, split);
    double tol2 = tol * tol;
    if (maxe < tol2) { out.push_back(b); return; }
    if (maxe < tol2 * 16.0) {
        for (int it = 0; it < 4; it++) {
            reparameterize(d, first, last, u, b);
            b = generate_bezier(d, first, last, u, t1, t2);
            maxe = max_error(d, first, last, b, u, split);
            if (maxe < tol2) { out.push_back(b); return; }
        }
    }
    split = std::clamp(split, first + 1, last - 1);
    D2 tc = unit(d[split - 1] - d[split + 1]);
    fit_cubic(d, first, split, t1, tc, tol, out);
    fit_cubic(d, split, last, tc * -1.0, t2, tol, out);
}

struct FitIn {
    std::vector<D2> p;          // uniformly resampled closed polygon (pixels)
    std::vector<char> corner;
    int k = 4;                  // window of the tangent estimates
    int w = 1920, h = 1080;
};

std::vector<MapKnot> knots_from_fit(const FitIn &f, double tol) {
    const int N = (int) f.p.size();
    std::vector<int> cs;
    for (int i = 0; i < N; i++) if (f.corner[i]) cs.push_back(i);
    std::vector<Bz> bz;
    std::vector<char> startsrun;
    bool hascorner = !cs.empty();
    auto wrap = [&](int i) { return ((i % N) + N) % N; };
    if (!hascorner) {
        // one closed run: starts where the bend is sharpest
        int s = 0;
        double best = -1.0;
        for (int i = 0; i < N; i++) {
            D2 a = unit(f.p[i] - f.p[wrap(i - f.k)]), b = unit(f.p[wrap(i + f.k)] - f.p[i]);
            double ang = std::acos(std::clamp(dot(a, b), -1.0, 1.0));
            if (ang > best) { best = ang; s = i; }
        }
        std::vector<D2> r;
        for (int i = 0; i <= N; i++) r.push_back(f.p[wrap(s + i)]);
        D2 t1 = unit(f.p[wrap(s + f.k)] - f.p[wrap(s - f.k)]);
        size_t before = bz.size();
        fit_cubic(r, 0, (int) r.size() - 1, t1, t1 * -1.0, tol, bz);
        for (size_t i = before; i < bz.size(); i++) startsrun.push_back(0);
    }
    else {
        for (size_t ci = 0; ci < cs.size(); ci++) {
            int a = cs[ci], b = cs[(ci + 1) % cs.size()];
            int cnt = wrap(b - a);
            if (cnt == 0) cnt = N;
            std::vector<D2> r;
            for (int i = 0; i <= cnt; i++) r.push_back(f.p[wrap(a + i)]);
            int m = std::max(1, std::min(f.k, (int) r.size() / 3));
            D2 t1 = unit(r[m] - r[0]);
            D2 t2 = unit(r[r.size() - 1 - m] - r.back());
            size_t before = bz.size();
            fit_cubic(r, 0, (int) r.size() - 1, t1, t2, tol, bz);
            for (size_t i = before; i < bz.size(); i++) startsrun.push_back(i == before ? 1 : 0);
        }
    }
    const int M = (int) bz.size();
    std::vector<MapKnot> k(M);
    std::vector<D2> hin(M), hout(M), pos(M);
    for (int i = 0; i < M; i++) {
        pos[i] = bz[i].c[0];
        hout[i] = bz[i].c[1] - bz[i].c[0];
        const Bz &pv = bz[(i + M - 1) % M];
        hin[i] = pv.c[2] - pv.c[3];
    }
    for (int i = 0; i < M; i++) {
        MapKnot &c = k[i];
        c.x = (float) (pos[i].x / f.w);
        c.y = (float) (pos[i].y / f.h);
        double la = len(hin[i]), lb = len(hout[i]);
        int type;
        if (la < 0.5 && lb < 0.5) type = KT_CONST;
        else if (hascorner && startsrun[i]) type = KT_BROKEN;
        else {
            D2 t = (pos[(i + 1) % M] - pos[(i + M - 1) % M]) * (1.0 / 6.0);
            double lt = len(t);
            if (len(hout[i] - t) <= 0.12 * lt + 0.3 && len(hin[i] + t) <= 0.12 * lt + 0.3) type = KT_SMOOTH;
            else if (len(hin[i] + hout[i]) <= 0.08 * std::max(la, lb) + 0.3) type = KT_FLUID;
            else type = KT_BROKEN;
        }
        c.type = type;
        c.hout[0] = (float) (hout[i].x / f.w); c.hout[1] = (float) (hout[i].y / f.h);
        c.hin[0] = (float) (hin[i].x / f.w); c.hin[1] = (float) (hin[i].y / f.h);
    }
    return k;
}

// 'poly': closed polygon, normalized; w, h: the beamer size (tolerances are in beamer pixels)
std::vector<MapKnot> fit_outline(const std::vector<float> &poly, int w, int h, int maxknots) {
    maxknots = std::max(3, maxknots);
    FitIn f;
    f.w = w; f.h = h;
    std::vector<D2> raw;
    for (size_t i = 0; i + 1 < poly.size(); i += 2) {
        D2 p = {poly[i] * (double) w, poly[i + 1] * (double) h};
        if (raw.empty() || len(p - raw.back()) > 0.05) raw.push_back(p);
    }
    if (raw.size() > 1 && len(raw.front() - raw.back()) < 0.05) raw.pop_back();
    std::vector<MapKnot> res;
    if (raw.size() < 3) return res;
    double perim = 0.0;
    for (size_t i = 0; i < raw.size(); i++) perim += len(raw[(i + 1) % raw.size()] - raw[i]);
    double ds = std::max(1.5, perim / 1200.0);
    int N = std::max(12, (int) std::round(perim / ds));
    ds = perim / N;
    // arc length resampling
    {
        size_t seg = 0;
        double segstart = 0.0;
        double segl = len(raw[1 % raw.size()] - raw[0]);
        for (int i = 0; i < N; i++) {
            double target = i * ds;
            while (seg + 1 < raw.size() && target > segstart + segl) {
                segstart += segl;
                seg++;
                segl = len(raw[(seg + 1) % raw.size()] - raw[seg]);
            }
            double tt = segl > 1e-12 ? std::clamp((target - segstart) / segl, 0.0, 1.0) : 0.0;
            f.p.push_back(raw[seg] + (raw[(seg + 1) % raw.size()] - raw[seg]) * tt);
        }
    }
    f.k = std::clamp((int) std::round(8.0 / ds), 2, std::max(2, N / 8));
    // corners: sharp bends that are the strongest in their neighbourhood
    {
        std::vector<double> ang(N, 0.0);
        auto wrap = [&](int i) { return ((i % N) + N) % N; };
        for (int i = 0; i < N; i++) {
            D2 a = unit(f.p[i] - f.p[wrap(i - f.k)]), b = unit(f.p[wrap(i + f.k)] - f.p[i]);
            ang[i] = std::acos(std::clamp(dot(a, b), -1.0, 1.0));
        }
        f.corner.assign(N, 0);
        const double thr = 40.0 * 3.14159265358979 / 180.0;
        for (int i = 0; i < N; i++) {
            if (ang[i] < thr) continue;
            bool top = true;
            for (int d = -f.k; d <= f.k && top; d++) {
                if (d == 0) continue;
                double o = ang[wrap(i + d)];
                if (o > ang[i] || (o == ang[i] && d < 0)) top = false;
            }
            if (top) f.corner[i] = 1;
        }
    }
    // the smallest tolerance that keeps within the knot budget
    double lo = 1.0, hi = 40.0;
    std::vector<MapKnot> best = knots_from_fit(f, lo);
    if ((int) best.size() > maxknots) {
        std::vector<MapKnot> hik = knots_from_fit(f, hi);
        if ((int) hik.size() > maxknots) {
            // (does not fit even with the loosest tolerance: evenly spread smooth knots)
            best.clear();
            for (int i = 0; i < maxknots; i++) {
                MapKnot c;
                int idx = (int) ((long long) i * N / maxknots);
                c.x = (float) (f.p[idx].x / w);
                c.y = (float) (f.p[idx].y / h);
                best.push_back(c);
            }
        }
        else {
            best = hik;
            for (int it = 0; it < 12; it++) {
                double mid = std::sqrt(lo * hi);
                std::vector<MapKnot> mk = knots_from_fit(f, mid);
                if ((int) mk.size() <= maxknots) { hi = mid; best = mk; }
                else lo = mid;
            }
        }
    }
    while (best.size() < 3) {
        // (fewer than three: split the longest segment)
        int bi = 0;
        float bl = -1.0f;
        for (int i = 0; i < (int) best.size(); i++) {
            const MapKnot &a = best[i], &b = best[(i + 1) % best.size()];
            float l = std::hypot((a.x - b.x) * w, (a.y - b.y) * h);
            if (l > bl) { bl = l; bi = i; }
        }
        insert_knot(best, bi, 0.5f);
    }
    return best;
}

// ======================================================================================================
// Files (.epm)
// ======================================================================================================

std::string maps_dir() {
    std::string s = mainprogram->gendir;
    while (!s.empty() && (s.back() == '/' || s.back() == '\\')) s.pop_back();
    fs::path base;
    if (s.empty()) base = fs::current_path();
    else base = fs::path(s).parent_path();
    return (base / "mappings").generic_string();
}

std::string path_for_name(const std::string &name) {
    return (fs::path(maps_dir()) / (name + ".epm")).generic_string();
}

std::string fnum(float v) {
    char b[48];
    snprintf(b, sizeof(b), "%.7g", v);
    return b;
}

// ---- the surfaces (Warp) of the masks: a binary file next to the .epm file, same name, extension .epw ----
std::string warp_path(const std::string &epm) {
    return (fs::path(epm).parent_path() / (fs::path(epm).stem().string() + ".epw")).generic_string();
}

const char kWarpMagic[] = "EWOCWARP5\n";      // (4 had no 3D positions, 3 no 'support' bytes, 1 no planes, 2 had them as matrices: all still read)
const char kWarpMagic4[] = "EWOCWARP4\n";
const char kWarpMagic1[] = "EWOCWARP1\n";
const char kWarpMagic2[] = "EWOCWARP2\n";
const char kWarpMagic3[] = "EWOCWARP3\n";

// (the surfaces, as a block of the .epm file: see save_set_to; the older .epw file next to the .epm is still read)
bool write_warps(std::ostream &o) {
    uint32_t count = 0;
    for (const BeamerMaps &b : g_set) for (const MapMask &m : b.masks) if (m.warp && m.warp->gw > 0) count++;
    if (count == 0) return true;   // (nothing to write)
    {
        o.write(kWarpMagic, (std::streamsize) strlen(kWarpMagic));
        o.write((const char *) &count, 4);
        for (const BeamerMaps &b : g_set) {
            for (const MapMask &m : b.masks) {
                if (!m.warp || m.warp->gw <= 0) continue;
                const uint32_t kl = (uint32_t) b.key.size();
                const int32_t id = m.id, gw = m.warp->gw, gh = m.warp->gh;
                const float asp = m.warp->aspect;
                o.write((const char *) &kl, 4);
                o.write(b.key.data(), (std::streamsize) kl);
                o.write((const char *) &id, 4);
                o.write((const char *) &gw, 4);
                o.write((const char *) &gh, 4);
                o.write((const char *) &asp, 4);
                std::vector<uint16_t> q(m.warp->uv.size());
                for (size_t i = 0; i < q.size(); i++) q[i] = (uint16_t) std::clamp((long) std::lround((m.warp->uv[i] + 1.0f) * 16000.0f), 0L, 65535L);
                o.write((const char *) q.data(), (std::streamsize) (q.size() * 2));
                {
                    std::vector<uint8_t> sup((size_t) gw * gh, 1);
                    if (m.warp->support.size() == sup.size()) sup = m.warp->support;
                    o.write((const char *) sup.data(), (std::streamsize) sup.size());
                }
                for (int pi = 0; pi < 2; pi++) {
                    const WarpPlane &pl = m.warp->plane[pi];
                    const uint8_t valid = (pl.valid && pl.uv.size() == m.warp->uv.size()) ? 1 : 0;
                    o.write((const char *) &valid, 1);
                    o.write((const char *) &pl.aspect, 4);
                    o.write((const char *) &pl.rms, 4);
                    if (valid) {
                        for (size_t i = 0; i < q.size(); i++) q[i] = (uint16_t) std::clamp((long) std::lround((pl.uv[i] + 1.0f) * 16000.0f), 0L, 65535L);
                        o.write((const char *) q.data(), (std::streamsize) (q.size() * 2));
                    }
                }
                const uint8_t has3 = m.warp->pos3.size() == (size_t) gw * gh * 3 ? 1 : 0;
                o.write((const char *) &has3, 1);
                if (has3) {
                    o.write((const char *) m.warp->pivot, 12);
                    o.write((const char *) &m.warp->hfov, 4);
                    o.write((const char *) m.warp->pos3.data(), (std::streamsize) (m.warp->pos3.size() * 4));
                }
            }
        }
        if (!o.good()) return false;
    }
    return true;
}

void load_warps(std::istream &r, std::vector<BeamerMaps> &set) {
    char magic[16] = {0};
    const size_t ml = strlen(kWarpMagic);
    r.read(magic, (std::streamsize) ml);
    if (!r) return;
    const bool v5 = memcmp(magic, kWarpMagic, ml) == 0;           // (the 3D positions, the support bytes and the planes are in the file)
    const bool v4 = v5 || memcmp(magic, kWarpMagic4, ml) == 0;    // (the support bytes and the planes are in the file)
    const bool v3 = memcmp(magic, kWarpMagic3, ml) == 0;          // (the planes are in the file)
    const bool v2 = v4 || v3;
    const bool old2 = memcmp(magic, kWarpMagic2, ml) == 0;        // (planes as matrices: skipped)
    if (!v2 && !old2 && memcmp(magic, kWarpMagic1, ml) != 0) return;
    uint32_t count = 0;
    r.read((char *) &count, 4);
    for (uint32_t n = 0; n < count && r; n++) {
        uint32_t kl = 0;
        r.read((char *) &kl, 4);
        if (!r || kl > 4096) return;
        std::string key(kl, '\0');
        r.read(&key[0], (std::streamsize) kl);
        int32_t id = 0, gw = 0, gh = 0;
        float asp = 1.0f;
        r.read((char *) &id, 4);
        r.read((char *) &gw, 4);
        r.read((char *) &gh, 4);
        r.read((char *) &asp, 4);
        if (!r || gw <= 0 || gh <= 0 || gw > 4096 || gh > 4096) return;
        std::vector<uint16_t> q((size_t) gw * gh * 2);
        r.read((char *) q.data(), (std::streamsize) (q.size() * 2));
        if (!r) return;
        std::vector<uint8_t> support;
        if (v4) {
            support.resize((size_t) gw * gh);
            r.read((char *) support.data(), (std::streamsize) support.size());
            if (!r) return;
        }
        WarpPlane planes[2];
        if (old2) {
            r.seekg(90, std::ios::cur);
            if (!r) return;
        }
        if (v2) {
            for (int pi = 0; pi < 2; pi++) {
                uint8_t valid = 0;
                r.read((char *) &valid, 1);
                r.read((char *) &planes[pi].aspect, 4);
                r.read((char *) &planes[pi].rms, 4);
                if (valid) {
                    std::vector<uint16_t> pq((size_t) gw * gh * 2);
                    r.read((char *) pq.data(), (std::streamsize) (pq.size() * 2));
                    planes[pi].uv.resize(pq.size());
                    for (size_t i = 0; i < pq.size(); i++) planes[pi].uv[i] = (float) pq[i] / 16000.0f - 1.0f;
                }
                planes[pi].valid = valid != 0;
            }
            if (!r) return;
        }
        float piv[3] = {0.0f, 0.0f, 1.0f}, hf = 70.0f;
        std::vector<float> p3;
        if (v5) {
            uint8_t has3 = 0;
            r.read((char *) &has3, 1);
            if (has3) {
                r.read((char *) piv, 12);
                r.read((char *) &hf, 4);
                p3.resize((size_t) gw * gh * 3);
                r.read((char *) p3.data(), (std::streamsize) (p3.size() * 4));
            }
            if (!r) return;
        }
        for (BeamerMaps &b : set) {
            if (b.key != key) continue;
            for (MapMask &m : b.masks) {
                if (m.id != id) continue;
                auto w = std::make_shared<WarpLut>();
                w->gw = gw; w->gh = gh; w->aspect = asp;
                w->pos3 = p3; memcpy(w->pivot, piv, 12); w->hfov = hf;
                w->plane[0] = planes[0]; w->plane[1] = planes[1];
                w->support = support;
                w->uv.resize(q.size());
                for (size_t i = 0; i < q.size(); i++) w->uv[i] = (float) q[i] / 16000.0f - 1.0f;
                m.warp = w;
            }
        }
    }
}

// ---- the point clouds (Viewpoint) of the beamers: a block of the .epm file (the older .epc file next to it is still read) ----
std::string scene_path(const std::string &epm) {
    return (fs::path(epm).parent_path() / (fs::path(epm).stem().string() + ".epc")).generic_string();
}

const char kSceneMagic[] = "EWOCSCENE1\n";

bool write_scenes(std::ostream &o) {
    uint32_t count = 0;
    for (const BeamerMaps &b : g_set) if (b.scene && b.scene->n > 0) count++;
    if (count == 0) return true;   // (nothing to write)
    o.write(kSceneMagic, (std::streamsize) strlen(kSceneMagic));
    o.write((const char *) &count, 4);
    for (const BeamerMaps &b : g_set) {
        if (!b.scene || b.scene->n <= 0) continue;
        const uint32_t kl = (uint32_t) b.key.size();
        const int32_t n = b.scene->n;
        o.write((const char *) &kl, 4);
        o.write(b.key.data(), (std::streamsize) kl);
        o.write((const char *) &n, 4);
        o.write((const char *) &b.scene->hfov, 4);
        o.write((const char *) &b.scene->aspect, 4);
        o.write((const char *) b.scene->data.data(), (std::streamsize) ((size_t) n * 16));
    }
    return o.good();
}

void load_scenes(std::istream &r, std::vector<BeamerMaps> &set) {
    char magic[16] = {0};
    const size_t ml = strlen(kSceneMagic);
    r.read(magic, (std::streamsize) ml);
    if (!r || memcmp(magic, kSceneMagic, ml) != 0) return;
    uint32_t count = 0;
    r.read((char *) &count, 4);
    for (uint32_t i = 0; i < count && r; i++) {
        uint32_t kl = 0;
        r.read((char *) &kl, 4);
        if (!r || kl > 4096) return;
        std::string key(kl, '\0');
        r.read(&key[0], (std::streamsize) kl);
        int32_t n = 0;
        auto sc = std::make_shared<ScenePoints>();
        r.read((char *) &n, 4);
        r.read((char *) &sc->hfov, 4);
        r.read((char *) &sc->aspect, 4);
        if (!r || n <= 0 || n > 20000000) return;
        sc->n = n;
        sc->data.resize((size_t) n * 16);
        r.read((char *) sc->data.data(), (std::streamsize) sc->data.size());
        if (!r) return;
        for (BeamerMaps &b : set) if (b.key == key) b.scene = sc;
    }
}

bool save_set_to(const std::string &path) {
    try {
        fs::create_directories(fs::path(path).parent_path());
    }
    catch (...) {}
    std::string tmp = path + ".tmp";
    {
        std::ofstream o(tmp, std::ios::binary);
        if (!o.is_open()) return false;
        o << "EWOCvj MAPPINGSET V0.1\n";
        for (const BeamerMaps &b : g_set) {
            o << "BEAMER\n" << b.key << "\n";
            o << "SIZE\n" << b.w << " " << b.h << "\n";
            o << "NEXTID\n" << b.nextid << "\n";
            for (const MapMask &m : b.masks) {
                o << "MASK\n" << m.id << "\n";
                if (m.ctrl) o << "CURVE\n1\n";
                o << "INSET\n" << fnum(m.inset) << "\n";
                if (m.feather > 0.0f) o << "FEATHER\n" << fnum(m.feather) << "\n";
                o << "CONTENT\n" << m.content << " " << m.warpon << " " << m.place << "\n";
                o << "XFORM\n" << fnum(m.rot) << " " << fnum(m.scale) << " " << fnum(m.shx) << " " << fnum(m.shy) << "\n";
                if (m.camset) {
                    o << "VIEW\n" << fnum(m.cam.azimuthDeg) << " " << fnum(m.cam.elevationDeg) << " " << fnum(m.cam.distance) << " "
                      << fnum(m.cam.hfovDeg) << " " << fnum(m.cam.pivot[0]) << " " << fnum(m.cam.pivot[1]) << " " << fnum(m.cam.pivot[2]) << "\n";
                }
                o << "KNOTS\n" << m.k.size() << "\n";
                for (const MapKnot &c : m.k) {
                    o << fnum(c.x) << " " << fnum(c.y) << " " << c.type << " " << fnum(c.hin[0]) << " " << fnum(c.hin[1]) << " "
                      << fnum(c.hout[0]) << " " << fnum(c.hout[1]) << "\n";
                }
                o << "ENDMASK\n";
            }
            o << "ENDBEAMER\n";
        }
        o << "ENDOFFILE\n";
        // After the text, the binary data of the file, each block with its size: the surfaces of the masks (with their planes and 3D
        // positions) and the point clouds of the Viewpoint. A reader of the text part stops at ENDOFFILE and never sees them.
        {
            std::ostringstream wb(std::ios::binary), sb(std::ios::binary);
            write_warps(wb);
            write_scenes(sb);
            const std::string ws = wb.str(), ss = sb.str();
            if (!ws.empty()) {
                o << "WARPS " << ws.size() << "\n";
                o.write(ws.data(), (std::streamsize) ws.size());
                o << "\n";
            }
            if (!ss.empty()) {
                o << "SCENES " << ss.size() << "\n";
                o.write(ss.data(), (std::streamsize) ss.size());
                o << "\n";
            }
            o << "ENDBLOBS\n";
        }
        if (!o.good()) return false;
    }
    try {
        fs::rename(tmp, path);
    }
    catch (...) {
        try {
            fs::copy_file(tmp, path, fs::copy_options::overwrite_existing);
            fs::remove(tmp);
        }
        catch (...) { return false; }
    }
    // (the .epw and .epc files of older versions are in the .epm now)
    try {
        fs::remove(warp_path(path));
        fs::remove(scene_path(path));
    }
    catch (...) {}
    return true;
}

bool load_set_from(const std::string &path, std::vector<BeamerMaps> &out) {
    std::ifstream r(path, std::ios::binary);
    if (!r.is_open()) return false;
    std::string line;
    if (!safegetline(r, line) || line.rfind("EWOCvj MAPPINGSET", 0) != 0) return false;
    BeamerMaps *cur = nullptr;
    MapMask *cm = nullptr;
    float legacyinset = 0.0f;
    out.clear();
    while (safegetline(r, line)) {
        if (line == "ENDOFFILE") break;
        if (line == "BEAMER") {
            legacyinset = 0.0f;
            safegetline(r, line);
            out.push_back(BeamerMaps());
            cur = &out.back();
            cur->key = line;
        }
        else if (line == "SIZE" && cur) {
            safegetline(r, line);
            std::istringstream ss(line);
            ss >> cur->w >> cur->h;
        }
        else if (line == "NEXTID" && cur) {
            safegetline(r, line);
            cur->nextid = std::max(1, atoi(line.c_str()));
        }
        else if (line == "INSET" && cur) {
            // (per mask; an older file has one for the beamer, before its masks: that goes to all of them)
            safegetline(r, line);
            const float v = std::clamp((float) atof(line.c_str()), 0.0f, 50.0f);
            if (cm) cm->inset = v;
            else legacyinset = v;
        }
        else if (line == "FEATHER" && cm) {
            safegetline(r, line);
            cm->feather = std::clamp((float) atof(line.c_str()), 0.0f, 100.0f);
        }
        else if (line == "MASK" && cur) {
            safegetline(r, line);
            cur->masks.push_back(MapMask());
            cm = &cur->masks.back();
            cm->inset = legacyinset;
            cm->id = atoi(line.c_str());
            cur->nextid = std::max(cur->nextid, cm->id + 1);
        }
        else if (line == "CURVE" && cm) {
            safegetline(r, line);
            cm->ctrl = atoi(line.c_str()) != 0;
        }
        else if (line == "CONTENT" && cm) {
            safegetline(r, line);
            std::istringstream ss(line);
            ss >> cm->content >> cm->warpon >> cm->place;
            cm->content = std::clamp(cm->content, 0, 2);   // 0 clip, 1 fit, 2 test grid
            cm->warpon = std::clamp(cm->warpon, 0, 4);   // 0 off, 1 surface, 2 plane (AI depth), 3 plane (two webcams), 4 viewpoint
            cm->place = std::clamp(cm->place, 0, 2);
        }
        else if (line == "VIEW" && cm) {
            safegetline(r, line);
            std::istringstream ss(line);
            ss >> cm->cam.azimuthDeg >> cm->cam.elevationDeg >> cm->cam.distance >> cm->cam.hfovDeg
               >> cm->cam.pivot[0] >> cm->cam.pivot[1] >> cm->cam.pivot[2];
            cm->cam.distance = std::clamp(cm->cam.distance, 0.1f, 3.0f);
            cm->cam.hfovDeg = std::clamp(cm->cam.hfovDeg, 10.0f, 150.0f);
            cm->camset = true;
        }
        else if (line == "XFORM" && cm) {
            safegetline(r, line);
            std::istringstream ss(line);
            ss >> cm->rot >> cm->scale >> cm->shx >> cm->shy;
            cm->scale = std::clamp(cm->scale, 0.05f, 20.0f);
        }
        else if (line == "KNOTS" && cm) {
            safegetline(r, line);
            int n = atoi(line.c_str());
            for (int i = 0; i < n; i++) {
                safegetline(r, line);
                std::istringstream ss(line);
                MapKnot c;
                ss >> c.x >> c.y >> c.type >> c.hin[0] >> c.hin[1] >> c.hout[0] >> c.hout[1];
                c.type = std::clamp(c.type, 0, 3);
                cm->k.push_back(c);
            }
        }
        else if (line == "ENDMASK") {
            if (cm && cm->k.size() < 3 && cur) cur->masks.pop_back();
            cm = nullptr;
        }
        else if (line == "ENDBEAMER") {
            cur = nullptr;
            cm = nullptr;
        }
    }
    // the binary blocks after the text (see save_set_to)
    bool gotwarps = false, gotscenes = false;
    if (line == "ENDOFFILE") {
        while (safegetline(r, line)) {
            const bool isw = line.rfind("WARPS ", 0) == 0, iss = line.rfind("SCENES ", 0) == 0;
            if (!isw && !iss) break;   // (ENDBLOBS, or the end)
            const size_t sz = (size_t) std::strtoull(line.c_str() + (isw ? 6 : 7), nullptr, 10);
            if (sz == 0 || sz > ((size_t) 1 << 31)) break;
            std::string blob(sz, '\0');
            r.read(&blob[0], (std::streamsize) sz);
            if (!r) break;
            r.get();   // (the newline after the block)
            std::istringstream bs(blob, std::ios::binary);
            if (isw) { load_warps(bs, out); gotwarps = true; }
            else { load_scenes(bs, out); gotscenes = true; }
        }
    }
    r.close();
    // (a file of an older version has its surfaces and point clouds in files of their own next to it)
    if (!gotwarps) {
        std::ifstream wf(warp_path(path), std::ios::binary);
        if (wf.is_open()) load_warps(wf, out);
    }
    if (!gotscenes) {
        std::ifstream sf(scene_path(path), std::ios::binary);
        if (sf.is_open()) load_scenes(sf, out);
    }
    return true;
}

void save_if_dirty() {
    if (!g_dirty) return;
    if (g_path.empty()) g_path = path_for_name(g_name);
    if (save_set_to(g_path)) g_dirty = false;
}

void mark_changed(bool geometry) {
    g_ver++;
    g_viewver++;
    if (geometry) g_dirty = true;
}

// ======================================================================================================
// Displays (beamers)
// ======================================================================================================

struct Disp {
    int idx = 0;
    std::string key;
    std::string name;
    int w = 1920, h = 1080;     // desktop size of the display
};

std::vector<Disp> g_disps;   // all displays but the one of the main window

void refresh_disps() {
    g_disps.clear();
    int numd = 0;
    SDL_DisplayID *ids = SDL_GetDisplays(&numd);
    if (!ids) return;
    struct Info { int idx; std::string name; int w, h; int x, y; };
    std::vector<Info> all;
    for (int i = 0; i < numd; i++) {
        SDL_DisplayID id = EWOC_DisplayIndexToID(i);
        Info in;
        in.idx = i;
        const char *nm = SDL_GetDisplayName(id);
        in.name = nm ? nm : "Display";
        const SDL_DisplayMode *dm = SDL_GetDesktopDisplayMode(id);
        in.w = dm ? dm->w : 0;
        in.h = dm ? dm->h : 0;
        SDL_Rect rc = {0, 0, 0, 0};
        SDL_GetDisplayBounds(id, &rc);
        in.x = rc.x; in.y = rc.y;
        all.push_back(in);
    }
    SDL_free(ids);
    int curr = EWOC_DisplayIDToIndex(SDL_GetDisplayForWindow(mainprogram->mainwindow));
    for (const Info &in : all) {
        if (in.idx == curr) continue;
        // (the place among displays with the same name and size, left to right, top to bottom: not the display index, which
        // changes with the order the displays were plugged in)
        std::vector<const Info *> same;
        for (const Info &o : all) if (o.name == in.name && o.w == in.w && o.h == in.h) same.push_back(&o);
        std::sort(same.begin(), same.end(), [](const Info *a, const Info *b) { return a->x != b->x ? a->x < b->x : a->y < b->y; });
        int ord = 0;
        for (size_t i = 0; i < same.size(); i++) if (same[i]->idx == in.idx) ord = (int) i;
        Disp d;
        d.idx = in.idx;
        d.name = in.name;
        d.w = in.w > 0 ? in.w : 1920;
        d.h = in.h > 0 ? in.h : 1080;
        d.key = in.name + " " + std::to_string(in.w) + "x" + std::to_string(in.h) + " #" + std::to_string(ord);
        g_disps.push_back(d);
    }
}

const Disp *disp_for_key(const std::string &key) {
    for (const Disp &d : g_disps) if (d.key == key) return &d;
    return nullptr;
}

// ======================================================================================================
// The projection on the beamers
// ======================================================================================================

// one mask of a source whose frame is placed in its masks (Content = Fit, or Warp)
struct MaskDraw {
    int content = 0, warp = 0, place = 0;
    int grid = 0;                               // a test grid instead of the picture
    float rot = 0.0f, scale = 1.0f, shx = 0.0f, shy = 0.0f;
    float bb[4] = {0.0f, 0.0f, 1.0f, 1.0f};     // the bounding box of the mask on the beamer image (normalized, y down)
    float aspect = 1.0f;                        // width / height of the box the frame goes in (Fit / Fill / Stretch are relative to it)
    GLuint covtex = (GLuint) -1;                // black, alpha = 255 - coverage of this mask alone (inset applied)
    GLuint luttex = (GLuint) -1;                // the surface (Warp)
    bool view = false;                          // the surface is the viewpoint of the mask 'maskid': made again when the viewpoint changes
    int maskid = 0;
    unsigned viewver = 0;
};

struct SrcW {
    Elem el;
    GLuint masktex = (GLuint) -1;               // black, alpha = 255 - this source's share of the coverage of all its masks
    bool indiv = false;                         // the masks are drawn one by one (Content = Fit / Warp somewhere)
    std::vector<MaskDraw> mdraws;
};

struct Out {
    std::string key;
    int disp = -1;
    EWindow *win = nullptr;
    bool borrowed = false;
    int prevmixid = 0;
    Layer *prevlay = nullptr;
    GLuint fbo[2] = {(GLuint) -1, (GLuint) -1}, tex[2] = {(GLuint) -1, (GLuint) -1};
    GLuint tmpfbo = (GLuint) -1, tmptex = (GLuint) -1;
    GLuint srcfbo = (GLuint) -1, srctex = (GLuint) -1;   // all the masks of one source together (maximum), before its share is added
    int w = 0, h = 0, idx = 0;
    GLuint show = (GLuint) -1;
    bool ok = false;
    std::vector<SrcW> srcs;
    GLuint expmask = (GLuint) -1;
    unsigned built = 0;
    bool builtexpose = false;
    int mw = 0, mh = 0;
};

std::vector<Out *> g_outs;

void free_tex(GLuint &t) {
    if (t != (GLuint) -1) glDeleteTextures(1, &t);
    t = (GLuint) -1;
}

// the textures of the sources (their share masks and the masks drawn one by one)
void free_src_textures(Out *o) {
    for (SrcW &s : o->srcs) {
        free_tex(s.masktex);
        for (MaskDraw &d : s.mdraws) { free_tex(d.covtex); free_tex(d.luttex); }
        s.mdraws.clear();
    }
}

void make_target(Out *o, int w, int h) {
    if (o->fbo[0] != (GLuint) -1 && o->w == w && o->h == h) return;
    o->ok = true;
    for (int i = 0; i < 2; i++) {
        if (o->fbo[i] != (GLuint) -1) glDeleteFramebuffers(1, &o->fbo[i]);
        if (o->tex[i] != (GLuint) -1) {
            mainprogram->texsizemap.erase(o->tex[i]);
            mainprogram->texintfmap.erase(o->tex[i]);
            glDeleteTextures(1, &o->tex[i]);
        }
        glGenTextures(1, &o->tex[i]);
        glBindTexture(GL_TEXTURE_2D, o->tex[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER_COMPAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER_COMPAT);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, w, h);
        mainprogram->texintfmap[o->tex[i]] = GL_RGBA8;
        mainprogram->texsizemap[o->tex[i]] = {w, h};
        glGenFramebuffers(1, &o->fbo[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, o->fbo[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, o->tex[i], 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) o->ok = false;
    }
    if (o->tmpfbo != (GLuint) -1) glDeleteFramebuffers(1, &o->tmpfbo);
    if (o->tmptex != (GLuint) -1) {
        mainprogram->texsizemap.erase(o->tmptex);
        mainprogram->texintfmap.erase(o->tmptex);
        glDeleteTextures(1, &o->tmptex);
    }
    glGenTextures(1, &o->tmptex);
    glBindTexture(GL_TEXTURE_2D, o->tmptex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER_COMPAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER_COMPAT);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, w, h);
    mainprogram->texintfmap[o->tmptex] = GL_RGBA8;
    mainprogram->texsizemap[o->tmptex] = {w, h};
    glGenFramebuffers(1, &o->tmpfbo);
    glBindFramebuffer(GL_FRAMEBUFFER, o->tmpfbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, o->tmptex, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        glDeleteFramebuffers(1, &o->tmpfbo);
        o->tmpfbo = (GLuint) -1;
    }
    // the buffer one source's masks are collected in
    if (o->srcfbo != (GLuint) -1) glDeleteFramebuffers(1, &o->srcfbo);
    if (o->srctex != (GLuint) -1) {
        mainprogram->texsizemap.erase(o->srctex);
        mainprogram->texintfmap.erase(o->srctex);
        glDeleteTextures(1, &o->srctex);
    }
    glGenTextures(1, &o->srctex);
    glBindTexture(GL_TEXTURE_2D, o->srctex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER_COMPAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER_COMPAT);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, w, h);
    mainprogram->texintfmap[o->srctex] = GL_RGBA8;
    mainprogram->texsizemap[o->srctex] = {w, h};
    glGenFramebuffers(1, &o->srcfbo);
    glBindFramebuffer(GL_FRAMEBUFFER, o->srcfbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, o->srctex, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        glDeleteFramebuffers(1, &o->srcfbo);
        o->srcfbo = (GLuint) -1;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    o->show = o->tex[0];
    o->idx = 0;
    o->w = w;
    o->h = h;
    o->built = 0;
}

void close_out_window(Out *o) {
    if (!o->win) return;
    EWindow *win = o->win;
    OutputEntry *entry = nullptr;
    for (OutputEntry *e : mainprogram->outputentries) if (e->win == win) entry = e;
    if (o->borrowed) {
        win->mixid = o->prevmixid;
        win->lay = o->prevlay;
    }
    else if (entry) {
        win->lay = nullptr;
        SDL_GL_MakeCurrent(win->win, NULL);
        glFinish();
        SDL_GL_DestroyContext(win->glc);
        SDL_HideWindow(win->win);
        SDL_Delay(10);
        mainprogram->outputentries.erase(std::find(mainprogram->outputentries.begin(), mainprogram->outputentries.end(), entry));
        mainprogram->mixwindows.erase(std::remove(mainprogram->mixwindows.begin(), mainprogram->mixwindows.end(), win),
                                      mainprogram->mixwindows.end());
        maskmap_hide_pointer(win->win, false);
        SDL_SetWindowFullscreen(win->win, false);   // (the pool gives the window to other outputs too)
        mainprogram->add_to_winpool(win->win, win->w, win->h);
        delete win;
        delete entry;
        // (SDL_GL_MakeCurrent(win, NULL) above left the main thread without a context: everything drawn after this needs it back)
        SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
    }
    o->win = nullptr;
    o->borrowed = false;
}

void free_out_gl(Out *o) {
    for (int i = 0; i < 2; i++) {
        if (o->fbo[i] != (GLuint) -1) glDeleteFramebuffers(1, &o->fbo[i]);
        if (o->tex[i] != (GLuint) -1) {
            mainprogram->texsizemap.erase(o->tex[i]);
            mainprogram->texintfmap.erase(o->tex[i]);
            glDeleteTextures(1, &o->tex[i]);
        }
        o->fbo[i] = o->tex[i] = (GLuint) -1;
    }
    if (o->tmpfbo != (GLuint) -1) glDeleteFramebuffers(1, &o->tmpfbo);
    if (o->tmptex != (GLuint) -1) {
        mainprogram->texsizemap.erase(o->tmptex);
        mainprogram->texintfmap.erase(o->tmptex);
        glDeleteTextures(1, &o->tmptex);
    }
    o->tmpfbo = o->tmptex = (GLuint) -1;
    if (o->srcfbo != (GLuint) -1) glDeleteFramebuffers(1, &o->srcfbo);
    if (o->srctex != (GLuint) -1) {
        mainprogram->texsizemap.erase(o->srctex);
        mainprogram->texintfmap.erase(o->srctex);
        glDeleteTextures(1, &o->srctex);
    }
    o->srcfbo = o->srctex = (GLuint) -1;
    free_src_textures(o);
    o->srcs.clear();
    free_tex(o->expmask);
    o->show = (GLuint) -1;
    o->w = o->h = 0;
}

// (same window set-up as the monitor menu's "Show on display" and ProjMap's beamer, with mixid 6). A window ProjMap shows its
// beamer image in (mixid 5) is left alone.
bool open_out_window(Out *o, int disp) {
    for (OutputEntry *e : mainprogram->outputentries) {
        if (e->screen == disp) {
            // (the layer MAPPING view keeps its beamer window open after it is left: that one is used from then on, and
            // given back when MAPPING mode is entered again)
            if (e->win->mixid == 5 && mainprogram->projmap) return false;
            o->win = e->win;
            o->borrowed = true;
            o->prevmixid = e->win->mixid;
            o->prevlay = e->win->lay;
            e->win->mixid = 6;
            e->win->lay = nullptr;
            make_target(o, e->win->w, e->win->h);
            return true;
        }
    }
    EWindow *mwin = new EWindow;
    mwin->mixid = 6;
    OutputEntry *entry = new OutputEntry;
    entry->screen = disp;
    entry->win = mwin;
    mainprogram->outputentries.push_back(entry);
    SDL_Rect rc1, rc2;
    SDL_GetDisplayBounds(EWOC_DisplayIndexToID(disp), &rc1);
    SDL_GetDisplayUsableBounds(EWOC_DisplayIndexToID(disp), &rc2);
    // (the whole display, fullscreen: the usable area leaves out the Windows taskbar, which must not be on the beamer)
    mwin->w = rc1.w;
    mwin->h = rc1.h;
    SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
    mwin->win = mainprogram->grab_from_winpool(mwin->w, mwin->h);
    if (mwin->win == nullptr) {
        SDL_PropertiesID pr = SDL_CreateProperties();
        SDL_SetStringProperty(pr, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "EWOCvj");
        SDL_SetNumberProperty(pr, SDL_PROP_WINDOW_CREATE_X_NUMBER, rc1.x);
        SDL_SetNumberProperty(pr, SDL_PROP_WINDOW_CREATE_Y_NUMBER, rc1.y);
        SDL_SetNumberProperty(pr, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, mwin->w);
        SDL_SetNumberProperty(pr, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, mwin->h);
        SDL_SetBooleanProperty(pr, SDL_PROP_WINDOW_CREATE_OPENGL_BOOLEAN, true);
        SDL_SetBooleanProperty(pr, SDL_PROP_WINDOW_CREATE_BORDERLESS_BOOLEAN, true);
        SDL_SetBooleanProperty(pr, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
        SDL_SetBooleanProperty(pr, SDL_PROP_WINDOW_CREATE_MAXIMIZED_BOOLEAN, true);
        SDL_SetBooleanProperty(pr, SDL_PROP_WINDOW_CREATE_HIGH_PIXEL_DENSITY_BOOLEAN, true);
        mwin->win = SDL_CreateWindowWithProperties(pr);
        SDL_DestroyProperties(pr);
    }
    SDL_SetWindowPosition(mwin->win, rc1.x, rc1.y);
    SDL_ShowWindow(mwin->win);   // (a window taken from the pool was hidden when it was put there)
    SDL_SetWindowSize(mwin->win, rc1.w, rc1.h);
    SDL_SetWindowFullscreen(mwin->win, true);   // (borderless fullscreen on this display: above the taskbar)
    maskmap_hide_pointer(mwin->win, true);
    mainprogram->mixwindows.push_back(mwin);
    mwin->glc = SDL_GL_CreateContext(mwin->win);
    SDL_GL_MakeCurrent(mwin->win, mwin->glc);

    GLfloat vcoords1[8] = {-1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f};
    GLfloat tcoords[8] = {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f};
    glGenBuffers(1, &mwin->vbuf);
    glBindBuffer(GL_ARRAY_BUFFER, mwin->vbuf);
    glBufferData(GL_ARRAY_BUFFER, 32, vcoords1, GL_DYNAMIC_DRAW);
    glGenBuffers(1, &mwin->tbuf);
    glBindBuffer(GL_ARRAY_BUFFER, mwin->tbuf);
    glBufferData(GL_ARRAY_BUFFER, 32, tcoords, GL_DYNAMIC_DRAW);
    glGenVertexArrays(1, &mwin->vao);
    glBindVertexArray(mwin->vao);
    glBindBuffer(GL_ARRAY_BUFFER, mwin->vbuf);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
    glBindBuffer(GL_ARRAY_BUFFER, mwin->tbuf);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
    glUseProgram(mainprogram->ShaderProgram);

    std::thread vidoutput(output_video, mwin);
    vidoutput.detach();

    SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
    o->win = mwin;
    o->borrowed = false;
    make_target(o, mwin->w, mwin->h);
    return true;
}

void close_out(Out *o) {
    free_out_gl(o);        // (while the main context is current)
    close_out_window(o);
}

Out *find_out(const std::string &key) {
    for (Out *o : g_outs) if (o->key == key) return o;
    return nullptr;
}

// the texture an element shows: a layer as the mix shows it, a mix monitor
GLuint elem_tex(const Elem &e) {
    if (e.kind == 0) {
        if (!e.lay || !layer_alive(e.lay)) return (GLuint) -1;
        GLuint tex = (e.lay->node && e.lay->node->vidbox) ? e.lay->node->vidbox->tex : (GLuint) -1;
        if (tex == (GLuint) -1 || tex == 0) return (GLuint) -1;
        return tex;
    }
    if (e.kind == 1) {
        MixNode *mn = (e.mon == 3) ? mainprogram->nodesmain->mixnodes[1][2] : mainprogram->nodesmain->mixnodes[!mainprogram->prevmodus][e.mon];
        if (!mn) return (GLuint) -1;
        GLuint tex = mn->mixtex;
        if (tex == (GLuint) -1 || tex == 0) return (GLuint) -1;
        return tex;
    }
    return (GLuint) -1;
}

bool same_elem(const Elem &a, const Elem &b) {
    if (a.kind != b.kind) return false;
    return a.kind == 0 ? a.lay == b.lay : a.mon == b.mon;
}

bool assign_matches(const Assign &a, const Elem &e) {
    if (a.kind != e.kind) return false;
    return a.kind == 0 ? (a.lay == e.lay && e.lay != nullptr) : a.mon == e.mon;
}

// fills the polygon (closed, normalized) into the coverage image (even-odd scanline fill)
void fill_polygon(std::vector<uint8_t> &cov, int mw, int mh, const std::vector<float> &pts) {
    int n = (int) pts.size() / 2;
    if (n < 3) return;
    std::vector<float> xs;
    for (int y = 0; y < mh; y++) {
        float yc = (float) y + 0.5f;
        xs.clear();
        for (int i = 0; i < n; i++) {
            int j = (i + 1) % n;
            float ay = pts[i * 2 + 1] * mh, by = pts[j * 2 + 1] * mh;
            if ((ay <= yc && by > yc) || (by <= yc && ay > yc)) {
                float ax = pts[i * 2] * mw, bx = pts[j * 2] * mw;
                xs.push_back(ax + (yc - ay) / (by - ay) * (bx - ax));
            }
        }
        std::sort(xs.begin(), xs.end());
        for (size_t k = 0; k + 1 < xs.size(); k += 2) {
            int xa = std::max(0, (int) std::ceil(xs[k] - 0.5f)), xb = std::min(mw - 1, (int) std::floor(xs[k + 1] - 0.5f));
            for (int x = xa; x <= xb; x++) cov[(size_t) y * mw + x] = 255;
        }
    }
}

// Moves the edge of a coverage image inwards by d pixels, with a soft (one pixel wide) edge: a chamfer distance of every inside
// pixel to the nearest outside one. The border of the image is not an edge (a mask that reaches over the border of the beamer
// image keeps reaching over it).
void erode_coverage(std::vector<uint8_t> &cov, int mw, int mh, float d, float feather = 0.0f) {
    if (d <= 0.0f && feather <= 0.0f) return;
    const float fw = std::max(1.0f, feather);   // (the width of the soft edge: one pixel, or the Feather)
    const int INF = 1 << 28;
    std::vector<int> dist((size_t) mw * mh);
    for (size_t i = 0; i < dist.size(); i++) dist[i] = cov[i] ? INF : 0;
    auto at = [&](int x, int y) -> int & { return dist[(size_t) y * mw + x]; };
    for (int y = 0; y < mh; y++) {
        for (int x = 0; x < mw; x++) {
            int &v = at(x, y);
            if (v == 0) continue;
            if (x > 0) v = std::min(v, at(x - 1, y) + 3);
            if (y > 0) {
                v = std::min(v, at(x, y - 1) + 3);
                if (x > 0) v = std::min(v, at(x - 1, y - 1) + 4);
                if (x < mw - 1) v = std::min(v, at(x + 1, y - 1) + 4);
            }
        }
    }
    for (int y = mh - 1; y >= 0; y--) {
        for (int x = mw - 1; x >= 0; x--) {
            int &v = at(x, y);
            if (v == 0) continue;
            if (x < mw - 1) v = std::min(v, at(x + 1, y) + 3);
            if (y < mh - 1) {
                v = std::min(v, at(x, y + 1) + 3);
                if (x < mw - 1) v = std::min(v, at(x + 1, y + 1) + 4);
                if (x > 0) v = std::min(v, at(x - 1, y + 1) + 4);
            }
        }
    }
    for (size_t i = 0; i < cov.size(); i++) {
        if (!cov[i]) continue;
        const float dpx = dist[i] >= INF / 2 ? 1e9f : (float) dist[i] / 3.0f;   // (pixels to the nearest outside pixel)
        float t = std::clamp((dpx - d) / fw, 0.0f, 1.0f);
        // A feathered edge is a blend with black on the encoded pixel values, which the eye reads much darker than the alpha says
        // (50% looks like a quarter): the ramp is bent so that the fade looks even
        if (feather > 0.0f) t = std::pow(t, 1.0f / 2.2f);
        cov[i] = (uint8_t) std::lround(255.0f * t);
    }
}

// the coverage of a mask moved inwards by 'inset' beamer pixels, with an edge that fades in over 'feather' pixels inside it (0: a one
// pixel soft edge), merged (maximum) into cov
void add_mask_coverage(std::vector<uint8_t> &cov, std::vector<uint8_t> &tmp, int mw, int mh, const std::vector<float> &poly, float inset_mpx,
                       float feather_mpx = 0.0f) {
    tmp.assign((size_t) mw * mh, 0);
    fill_polygon(tmp, mw, mh, poly);
    erode_coverage(tmp, mw, mh, inset_mpx, feather_mpx);
    for (size_t i = 0; i < cov.size(); i++) cov[i] = std::max(cov[i], tmp[i]);
}

void upload_mask(GLuint &tex, int mw, int mh, const std::vector<uint8_t> &rgba) {
    if (tex == (GLuint) -1) {
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA_INTERNAL, mw, mh, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

// the surface of a mask (Warp) as a texture: for each cell of the beamer image the position in the box the frame is placed in
void upload_lut(GLuint &tex, int gw, int gh, const std::vector<float> &uv, const std::vector<uint8_t> &support) {
    // (red, green: the position in the box; blue: 1 where it is known, 0 where it is not: the shader projects nothing there)
    std::vector<float> rgba((size_t) gw * gh * 4);
    for (size_t i = 0; i < (size_t) gw * gh; i++) {
        rgba[i * 4] = uv[i * 2];
        rgba[i * 4 + 1] = uv[i * 2 + 1];
        rgba[i * 4 + 2] = (support.size() == (size_t) gw * gh && !support[i]) ? 0.0f : 1.0f;
        rgba[i * 4 + 3] = 1.0f;
    }
    if (tex == (GLuint) -1) {
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, gw, gh, 0, GL_RGBA, GL_FLOAT, rgba.data());   // (16 bit floats filter everywhere)
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// ---- placing the frame of a source in a mask: Fit / Fill / Stretch with a shift, size and rotation, flat or following the
// surface the webcam saw. One small shader: for every pixel of the beamer image the position in the mask's box (from the surface
// lookup, or from the box on the beamer image) says where in the frame it is. The images here have row 0 = top; the beamer
// image is drawn into with that same orientation, so the coordinates of the pixel are the ones of the picture, y down. ----
GLuint g_fitprog = 0, g_fitvao = 0, g_fitvbo = 0;
bool g_fitfailed = false;
GLint g_fuContent = -1, g_fuLut = -1, g_fuFlat = -1, g_fuBB = -1, g_fuAb = -1, g_fuSize = -1, g_fuRot = -1, g_fuShift = -1, g_fuGrid = -1;

bool ensure_fit_prog() {
    if (g_fitprog) return true;
    if (g_fitfailed) return false;
#ifdef USE_GLES
    const char *vh = "#version 300 es\nprecision highp float;\n";
    const char *fh = "#version 300 es\nprecision highp float;\nprecision highp sampler2D;\n";
#else
    const char *vh = "#version 430 core\n";
    const char *fh = "#version 430 core\n";
#endif
    const char *vs =
        "layout(location = 0) in vec2 pos;\n"
        "out vec2 vuv;\n"
        "void main() { gl_Position = vec4(pos, 0.0, 1.0); vuv = pos * 0.5 + 0.5; }\n";
    const char *fs =
        "uniform sampler2D content;\n"
        "uniform sampler2D lut;\n"
        "uniform int flatmode;\n"
        "uniform vec4 bb;\n"
        "uniform float ab;\n"
        "uniform vec2 csize;\n"
        "uniform float rot;\n"
        "uniform vec2 shift;\n"
        "uniform int gridmode;\n"
        "in vec2 vuv;\n"
        "out vec4 outc;\n"
        "void main() {\n"
        "    vec2 p;\n"
        "    if (flatmode != 0) p = (vuv - bb.xy) / bb.zw;\n"
        "    else {\n"
        "        vec3 t = texture(lut, vuv).rgb;\n"
        "        if (t.b < 0.5) { outc = vec4(0.0, 0.0, 0.0, 1.0); return; }   // (nothing is known of the surface here)\n"
        "        p = t.rg;\n"
        "    }\n"
        "    p = (p - 0.5) * vec2(ab, 1.0);\n"
        "    float c = cos(rot), s = sin(rot);\n"
        "    p = vec2(c * p.x + s * p.y, -s * p.x + c * p.y);\n"
        "    vec2 cuv = p / csize + 0.5 - shift;\n"
        "    if (cuv.x < 0.0 || cuv.x > 1.0 || cuv.y < 0.0 || cuv.y > 1.0) { outc = vec4(0.0, 0.0, 0.0, 1.0); return; }\n"
        "    if (gridmode != 0) {\n"
        "        // a test grid instead of the picture (10 x 10 over the frame, the colour is the position in it): shows how the surface goes\n"
        "        vec2 g = cuv * 10.0;\n"
        "        vec2 f = abs(fract(g - 0.5) - 0.5) / max(fwidth(g), vec2(1e-4));\n"
        "        float ln = 1.0 - min(min(f.x, f.y), 1.0);\n"
        "        outc = vec4(mix(vec3(cuv.x, cuv.y, 0.35) * 0.55, vec3(1.0), ln), 1.0);\n"
        "        return;\n"
        "    }\n"
        "    outc = vec4(texture(content, cuv).rgb, 1.0);\n"
        "}\n";
    auto compile = [&](GLenum type, const char *hdr, const char *src) -> GLuint {
        GLuint sh = glCreateShader(type);
        const char *srcs[2] = {hdr, src};
        glShaderSource(sh, 2, srcs, nullptr);
        glCompileShader(sh);
        GLint ok = 0;
        glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[1024] = {0};
            glGetShaderInfoLog(sh, 1023, nullptr, log);
            fprintf(stderr, "[MaskMap] fit shader: %s\n", log);
            glDeleteShader(sh);
            return 0;
        }
        return sh;
    };
    GLuint v = compile(GL_VERTEX_SHADER, vh, vs), f = compile(GL_FRAGMENT_SHADER, fh, fs);
    if (!v || !f) {
        if (v) glDeleteShader(v);
        if (f) glDeleteShader(f);
        g_fitfailed = true;
        return false;
    }
    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    glDeleteShader(v);
    glDeleteShader(f);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024] = {0};
        glGetProgramInfoLog(p, 1023, nullptr, log);
        fprintf(stderr, "[MaskMap] fit shader link: %s\n", log);
        glDeleteProgram(p);
        g_fitfailed = true;
        return false;
    }
    g_fuContent = glGetUniformLocation(p, "content");
    g_fuLut = glGetUniformLocation(p, "lut");
    g_fuFlat = glGetUniformLocation(p, "flatmode");
    g_fuBB = glGetUniformLocation(p, "bb");
    g_fuAb = glGetUniformLocation(p, "ab");
    g_fuSize = glGetUniformLocation(p, "csize");
    g_fuRot = glGetUniformLocation(p, "rot");
    g_fuShift = glGetUniformLocation(p, "shift");
    g_fuGrid = glGetUniformLocation(p, "gridmode");
    GLint prevvao = 0, prevabuf = 0;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevvao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prevabuf);
    const GLfloat quad[8] = {-1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f};
    glGenVertexArrays(1, &g_fitvao);
    glBindVertexArray(g_fitvao);
    glGenBuffers(1, &g_fitvbo);
    glBindBuffer(GL_ARRAY_BUFFER, g_fitvbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
    glBindVertexArray((GLuint) prevvao);
    glBindBuffer(GL_ARRAY_BUFFER, (GLuint) prevabuf);
    g_fitprog = p;
    return true;
}

// draws the frame 'contenttex' placed as the mask draw says into the framebuffer that is bound (the whole beamer image)
void draw_fit(Out *o, GLuint contenttex, const MaskDraw &md) {
    if (!ensure_fit_prog()) {
        // (no shader: the full frame, as Clip)
        draw_direct(nullptr, g_tr, -1.0f, 1.0f, 2.0f, -2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, contenttex, o->w, o->h, false, false);
        return;
    }
    float Ac = 16.0f / 9.0f;
    auto it = mainprogram->texsizemap.find(contenttex);
    if (it != mainprogram->texsizemap.end() && it->second.first > 0 && it->second.second > 0)
        Ac = (float) it->second.first / (float) it->second.second;
    const float Ab = std::max(0.01f, md.aspect);
    float cw, ch;
    if (md.place == 2) { cw = Ab; ch = 1.0f; }                                    // Stretch
    else {
        const float s = md.place == 0 ? std::min(Ab / Ac, 1.0f) : std::max(Ab / Ac, 1.0f);   // Fit / Fill
        cw = s * Ac; ch = s;
    }
    cw *= md.scale; ch *= md.scale;

    GLint prevprog = 0, prevvao = 0, prevabuf = 0, prevactive = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevprog);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevvao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prevabuf);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &prevactive);
    glDisable(GL_BLEND);
    glUseProgram(g_fitprog);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, contenttex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, md.warp ? md.luttex : 0);
    glUniform1i(g_fuContent, 0);
    glUniform1i(g_fuLut, 1);
    glUniform1i(g_fuFlat, md.warp ? 0 : 1);
    glUniform4f(g_fuBB, md.bb[0], md.bb[1], std::max(md.bb[2], 1e-4f), std::max(md.bb[3], 1e-4f));
    glUniform1f(g_fuAb, Ab);
    glUniform2f(g_fuSize, std::max(cw, 1e-4f), std::max(ch, 1e-4f));
    glUniform1f(g_fuRot, md.rot * 3.14159265f / 180.0f);
    glUniform2f(g_fuShift, md.shx, md.shy);
    glUniform1i(g_fuGrid, md.grid);
    glBindVertexArray(g_fitvao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture((GLenum) prevactive);
    glBindVertexArray((GLuint) prevvao);
    glBindBuffer(GL_ARRAY_BUFFER, (GLuint) prevabuf);
    glUseProgram((GLuint) prevprog);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}

// ---- the Viewpoint: the surface of a mask as seen from where the viewer is (the camera of the mask) ----
// A mask can have a viewpoint when its surface came with 3D positions (the AI depth of webcam 1)
bool view_possible(const MapMask &m) {
    return m.warp && m.warp->gw > 0 && m.warp->pos3.size() == (size_t) m.warp->gw * m.warp->gh * 3;
}

// the viewer starts where webcam 1 is, looking at the middle of the shape
void init_cam(MapMask &m) {
    if (m.camset || !m.warp) return;
    m.cam = OrbitCamera();
    m.cam.azimuthDeg = 0.0f; m.cam.elevationDeg = 0.0f; m.cam.distance = 1.0f;
    m.cam.hfovDeg = m.warp->hfov;
    // (the viewer is webcam 1: at its place, looking along its axis with its field of view, so that the first view is the webcam's own
    // picture; the orbit turns around the point on the axis at the depth of the shape)
    m.cam.pivot[0] = 0.0f; m.cam.pivot[1] = 0.0f; m.cam.pivot[2] = m.warp->pivot[2];
    m.camset = true;
}

// the lookup of the viewpoint: every cell's point projected into the view of the camera; the box is of the cells of the shape itself
bool compute_view_lut(MapMask &m) {
    if (!view_possible(m)) return false;
    init_cam(m);
    if (m.vc && m.vc->ver == g_viewver) return !m.vc->core.empty();
    auto vc = std::make_shared<ViewCache>();
    vc->ver = g_viewver;
    m.vc = vc;
    const WarpLut &W = *m.warp;
    const size_t gn = (size_t) W.gw * W.gh;
    const Mat4 V = m.cam.getViewMatrix();
    std::vector<float> x(gn, 0.0f), y(gn, 0.0f);
    std::vector<uint8_t> ok(gn, 0);
    for (size_t i = 0; i < gn; i++) {
        const float *P = &W.pos3[i * 3];
        if (P[2] <= 0.0f) continue;
        float v[4];
        mat4TransformPoint(V, P, v);
        const float d = -v[2];
        if (d < 0.05f) continue;    // (behind the viewer or on top of it)
        x[i] = v[0] / d;
        y[i] = -v[1] / d;
        ok[i] = 1;
        const bool known = W.support.size() != gn || W.support[i];
        const float u = W.uv[i * 2], w = W.uv[i * 2 + 1];
        if (known && u >= 0.0f && u <= 1.0f && w >= 0.0f && w <= 1.0f) vc->core.push_back((int) i);
    }
    if (vc->core.size() < 4) { vc->core.clear(); return false; }
    std::vector<float> cx, cy;
    for (int i : vc->core) { cx.push_back(x[i]); cy.push_back(y[i]); }
    std::sort(cx.begin(), cx.end());
    std::sort(cy.begin(), cy.end());
    const size_t lo = cx.size() / 100, hi = std::min(cx.size() - 1, cx.size() - 1 - cx.size() / 100);
    const float x0 = cx[lo], x1 = cx[hi], y0 = cy[lo], y1 = cy[hi];
    const float sx = std::max(x1 - x0, 1e-4f), sy = std::max(y1 - y0, 1e-4f);
    vc->aspect = std::clamp(sx / sy, 0.05f, 20.0f);
    vc->uv.assign(gn * 2, 0.0f);
    vc->support.assign(gn, 0);
    for (size_t i = 0; i < gn; i++) {
        if (!ok[i]) continue;
        vc->uv[i * 2] = (x[i] - x0) / sx;
        vc->uv[i * 2 + 1] = (y[i] - y0) / sy;
        vc->support[i] = (W.support.size() != gn || W.support[i]) ? 1 : 0;
    }
    return true;
}

bool g_exposeOn = false;       // (the view sets these)
std::string g_exposeKey;
int g_exposeSel = -1;          // the outline EXPOSE shows (the one selected in the view); -1 with EXPOSE off
float g_exposeCol[3] = {1.0f, 1.0f, 1.0f};

// the sources assigned to masks of this beamer, and for each the weight masks: where several sources cover a pixel each gets
// an equal share
void rebuild_out_masks(Out *o) {
    BeamerMaps *bm = find_maps(o->key, false);
    free_src_textures(o);
    o->srcs.clear();
    free_tex(o->expmask);
    o->mw = std::max(64, o->w);   // (the full beamer resolution: the curves are rasterized at the finest the beamer can show)
    o->mh = std::max(64, o->h);
    o->built = g_ver;
    if (!bm) return;
    const int mw = o->mw, mh = o->mh;
    for (MapMask &m : bm->masks) ensure_flat(m, *bm);
    // sources
    std::vector<Elem> els;
    for (const Assign &a : g_assign) {
        if (a.key != o->key) continue;
        Elem e;
        e.kind = a.kind; e.lay = a.lay; e.mon = a.mon;
        if (a.kind == 0 && !a.lay) continue;
        bool have = false;
        for (const Elem &x : els) if (same_elem(x, e)) have = true;
        if (!have) els.push_back(e);
    }
    // (the Inset of each mask: it is moved inwards by that many beamer pixels, after its curve edits)
    const float insetscale = (float) mw / (float) std::max(1, o->w);
    std::vector<uint8_t> tmp;
    std::vector<std::vector<uint8_t>> covs;
    for (const Elem &e : els) {
        std::vector<uint8_t> cov((size_t) mw * mh, 0);
        for (const Assign &a : g_assign) {
            if (a.key != o->key || !assign_matches(a, e)) continue;
            MapMask *m = find_mask(bm, a.maskid);
            if (m) add_mask_coverage(cov, tmp, mw, mh, m->poly, m->inset * insetscale, m->feather * insetscale);
        }
        covs.push_back(std::move(cov));
    }
    std::vector<uint8_t> cnt((size_t) mw * mh, 0);
    for (auto &c : covs) for (size_t i = 0; i < c.size(); i++) if (c[i]) cnt[i]++;
    for (size_t s = 0; s < els.size(); s++) {
        std::vector<uint8_t> rgba((size_t) mw * mh * 4, 0);
        for (size_t i = 0; i < covs[s].size(); i++) {
            int wgt = covs[s][i] / std::max<int>(1, cnt[i]);   // (a share of the coverage: equal parts where several sources meet)
            rgba[i * 4 + 3] = (uint8_t) (255 - wgt);
        }
        SrcW sw;
        sw.el = els[s];
        upload_mask(sw.masktex, mw, mh, rgba);
        // The masks are drawn one by one when one of them places the frame in itself (Fit, or Warp): each gets its own coverage
        // image, and the surface for the Warp
        bool indiv = false;
        for (const Assign &a : g_assign) {
            if (a.key != o->key || !assign_matches(a, els[s])) continue;
            MapMask *m = find_mask(bm, a.maskid);
            if (m && (m->content >= 1 || (m->warpon && m->warp))) indiv = true;
        }
        if (indiv) {
            sw.indiv = true;
            for (const Assign &a : g_assign) {
                if (a.key != o->key || !assign_matches(a, els[s])) continue;
                MapMask *m = find_mask(bm, a.maskid);
                if (!m || m->poly.size() < 6) continue;
                const bool effwarp = m->warpon && m->warp && m->warp->gw > 0;
                MaskDraw d;
                d.content = (m->content >= 1 || effwarp) ? 1 : 0;
                d.grid = m->content == 2 ? 1 : 0;
                d.warp = effwarp ? 1 : 0;
                // the viewpoint: the surface as seen from the camera of the mask (else the surface as the webcam sees it)
                const bool viewmode = effwarp && m->warpon == 4 && compute_view_lut(*m);
                // a plane (real proportions) when asked for and there; else the surface as the webcam sees it
                const WarpPlane *pl = (effwarp && (m->warpon == 2 || m->warpon == 3)) ? &m->warp->plane[m->warpon - 2] : nullptr;
                const bool useplane = pl && pl->valid && pl->uv.size() == m->warp->uv.size();
                d.place = m->place;
                d.rot = m->rot; d.scale = m->scale; d.shx = m->shx; d.shy = m->shy;
                float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f;
                for (size_t i = 0; i + 1 < m->poly.size(); i += 2) {
                    x0 = std::min(x0, m->poly[i]); x1 = std::max(x1, m->poly[i]);
                    y0 = std::min(y0, m->poly[i + 1]); y1 = std::max(y1, m->poly[i + 1]);
                }
                d.bb[0] = x0; d.bb[1] = y0; d.bb[2] = std::max(x1 - x0, 1e-4f); d.bb[3] = std::max(y1 - y0, 1e-4f);
                d.aspect = effwarp ? (viewmode ? m->vc->aspect : (useplane ? pl->aspect : m->warp->aspect)) : (d.bb[2] * (float) o->w) / (d.bb[3] * (float) o->h);
                if (viewmode) { d.view = true; d.maskid = m->id; d.viewver = g_viewver; }
                std::vector<uint8_t> cov1((size_t) mw * mh, 0);
                add_mask_coverage(cov1, tmp, mw, mh, m->poly, m->inset * insetscale, m->feather * insetscale);
                std::vector<uint8_t> rgba1((size_t) mw * mh * 4, 0);
                for (size_t i = 0; i < cov1.size(); i++) rgba1[i * 4 + 3] = (uint8_t) (255 - cov1[i]);
                upload_mask(d.covtex, mw, mh, rgba1);
                if (viewmode) upload_lut(d.luttex, m->warp->gw, m->warp->gh, m->vc->uv, m->vc->support);
                else if (effwarp) upload_lut(d.luttex, m->warp->gw, m->warp->gh, useplane ? pl->uv : m->warp->uv, m->warp->support);
                sw.mdraws.push_back(d);
            }
        }
        o->srcs.push_back(sw);
    }
    // all masks of the beamer, for the exposure colour
    {
        std::vector<uint8_t> cov((size_t) mw * mh, 0);
        // (the curves as they are, for fitting them to the objects; only the selected outline when the view is open: no selection, none)
        for (MapMask &m : bm->masks) if (g_exposeSel < 0 ? !g_exposeOn : m.id == g_exposeSel) add_mask_coverage(cov, tmp, mw, mh, m.poly, 0.0f);
        std::vector<uint8_t> rgba((size_t) mw * mh * 4, 0);
        for (size_t i = 0; i < cov.size(); i++) rgba[i * 4 + 3] = (uint8_t) (255 - cov[i]);
        upload_mask(o->expmask, mw, mh, rgba);
    }
}

void render_out(Out *o) {
    if (o->fbo[o->idx] == (GLuint) -1 || !o->win || !o->ok) return;
    const bool expose = g_exposeOn && g_exposeKey == o->key;
    if (o->built != g_ver) rebuild_out_masks(o);

    bool budm = mainprogram->directmode;
    mainprogram->directmode = true;
    glBindFramebuffer(GL_FRAMEBUFFER, o->fbo[o->idx]);
    glDrawBuffer_FBO();
    glViewport(0, 0, o->w, o->h);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    mainprogram->uniformCache->setInt("interm", 0);
    mainprogram->uniformCache->setInt("mixmode", 0);
    mainprogram->uniformCache->setBool("usemask", false);

    if (expose) {
        g_ex[0] = g_exposeCol[0]; g_ex[1] = g_exposeCol[1]; g_ex[2] = g_exposeCol[2];
        draw_direct(nullptr, g_ex, -1.0f, -1.0f, 2.0f, 2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, -1, o->w, o->h, false, false);
        if (o->expmask != (GLuint) -1)
            draw_direct(nullptr, g_tr, -1.0f, 1.0f, 2.0f, -2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, o->expmask, o->w, o->h, false, false);
    }
    else if (o->tmpfbo != (GLuint) -1) {
        for (SrcW &s : o->srcs) {
            if (s.masktex == (GLuint) -1) continue;
            GLuint tex = elem_tex(s.el);
            if (tex == (GLuint) -1) continue;
            if (s.indiv && o->srcfbo != (GLuint) -1) {
                // the masks of this source one by one (each with its own placement of the frame), collected as a maximum
                glBindFramebuffer(GL_FRAMEBUFFER, o->srcfbo);
                glDrawBuffer_FBO();
                glViewport(0, 0, o->w, o->h);
                glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);
                for (MaskDraw &d : s.mdraws) {
                    if (d.covtex == (GLuint) -1) continue;
                    if (d.view && d.viewver != g_viewver) {
                        // (the viewpoint changed: only the lookup is made again)
                        BeamerMaps *vb = find_maps(o->key, false);
                        MapMask *vm = vb ? find_mask(vb, d.maskid) : nullptr;
                        d.viewver = g_viewver;
                        if (vm && vm->warpon == 4 && compute_view_lut(*vm)) {
                            upload_lut(d.luttex, vm->warp->gw, vm->warp->gh, vm->vc->uv, vm->vc->support);
                            d.aspect = vm->vc->aspect;
                        }
                    }
                    glBindFramebuffer(GL_FRAMEBUFFER, o->tmpfbo);
                    glDrawBuffer_FBO();
                    glViewport(0, 0, o->w, o->h);
                    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                    glClear(GL_COLOR_BUFFER_BIT);
                    if (d.content == 0 && d.warp == 0)
                        draw_direct(nullptr, g_tr, -1.0f, 1.0f, 2.0f, -2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, tex, o->w, o->h, false, false);
                    else draw_fit(o, tex, d);
                    draw_direct(nullptr, g_tr, -1.0f, 1.0f, 2.0f, -2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, d.covtex, o->w, o->h, false, false);
                    glBindFramebuffer(GL_FRAMEBUFFER, o->srcfbo);
                    glDrawBuffer_FBO();
                    glViewport(0, 0, o->w, o->h);
                    glBlendEquation(GL_MAX);
                    glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE);
                    draw_direct(nullptr, g_tr, -1.0f, 1.0f, 2.0f, -2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, o->tmptex, o->w, o->h, false, false);
                    glBlendEquation(GL_FUNC_ADD);
                    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                }
                // (then its share, like a source with one mask)
                glBindFramebuffer(GL_FRAMEBUFFER, o->tmpfbo);
                glDrawBuffer_FBO();
                glViewport(0, 0, o->w, o->h);
                glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);
                draw_direct(nullptr, g_tr, -1.0f, 1.0f, 2.0f, -2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, o->srctex, o->w, o->h, false, false);
                draw_direct(nullptr, g_tr, -1.0f, 1.0f, 2.0f, -2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, s.masktex, o->w, o->h, false, false);
            }
            else {
            glBindFramebuffer(GL_FRAMEBUFFER, o->tmpfbo);
            glDrawBuffer_FBO();
            glViewport(0, 0, o->w, o->h);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            draw_direct(nullptr, g_tr, -1.0f, 1.0f, 2.0f, -2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, tex, o->w, o->h, false, false);
            draw_direct(nullptr, g_tr, -1.0f, 1.0f, 2.0f, -2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, s.masktex, o->w, o->h, false, false);
            }
            glBindFramebuffer(GL_FRAMEBUFFER, o->fbo[o->idx]);
            glDrawBuffer_FBO();
            glViewport(0, 0, o->w, o->h);
            glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE);   // (added: black outside the mask adds nothing)
            draw_direct(nullptr, g_tr, -1.0f, 1.0f, 2.0f, -2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, o->tmptex, o->w, o->h, false, false);
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        }
    }

    mainprogram->directmode = budm;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDrawBuffer_Back();
    glViewport(0, 0, glob->w, glob->h);
    glFinish();
    o->show = o->tex[o->idx];
    o->idx ^= 1;
}

// ======================================================================================================
// The view
// ======================================================================================================

struct Snap {
    std::string key;
    std::vector<MapMask> masks;
};

struct View {
    bool on = false;
    bool inited = false;
    Elem el;
    Param *beam = nullptr, *name = nullptr, *color = nullptr, *knots = nullptr, *inset = nullptr, *feather = nullptr;
    // how the frame is placed in the selected outline
    Param *content = nullptr, *warp = nullptr, *placement = nullptr, *rot = nullptr, *scale = nullptr, *shx = nullptr, *shy = nullptr;
    std::vector<int> warpmodes;                             // the Warp option of the selected outline -> its mode (0 off, 1 surface, 2 / 3 planes)
    std::string placekey = "\x01";                          // the outline the placement controls were last set from
    float lastcontent = 0, lastwarp = 0, lastplace = 0, lastrot = 0, lastscale = 1, lastshx = 0, lastshy = 0;
    float lastinset = 0.0f, lastfeather = 0.0f;
    int pressknot = -1;                                      // the knot a press started on that was selected already (-1: none)
    bool pressshift = false, pressmoved = false, knotpush = false;   // that press had SHIFT / has moved the mouse / pushed an undo step
    bool nudgeheld = false;                                // an arrow key is held (knot nudging)
    std::chrono::steady_clock::time_point nudget;            // when a held arrow key repeats next
    std::string insetkey = "\x01";   // the beamer the Inset slider was last set from (reset: set from the beamer again)
    Boxx *editbox = nullptr, *exposebox = nullptr, *openbox = nullptr, *donebox = nullptr, *cancelbox = nullptr, *newbox = nullptr, *delbox = nullptr;
    Boxx *tbox[4] = {nullptr, nullptr, nullptr, nullptr};
    // the Viewpoint: the point cloud of the scene (AI depth of webcam 1) seen from the camera of the selected outline
    Boxx *viewbox = nullptr, *resetbox = nullptr, *assignbox = nullptr;
    bool viewing = false;                                   // the canvas shows the point cloud instead of the beamer image
    bool vdrag = false, vdragorbit = false;                 // an orbit / pan drag is on (the pointer is captured)
    GLuint pcprog = 0, pcvao = 0, pcvbo = 0, hivao = 0, hivbo = 0;
    bool pcfailed = false;
    std::shared_ptr<ScenePoints> pcscene;                   // what pcvbo holds
    const WarpLut *hiwarp = nullptr;                        // what hivbo holds the points of (the selected outline)
    int hin = 0;
    std::vector<std::string> beamkeys;      // key of each option of the beamer Param (index 0: none)
    bool edit = false;
    bool expose = false;
    int curtype = KT_SMOOTH;
    std::string curkey;
    int sel = -1;
    std::set<int> ksel;
    float zoom = 1.0f, zu = 0.5f, zv = 0.5f;
    bool panning = false, mprev = false;
    float panmx = 0.0f, panmy = 0.0f;
    bool prevdown = false;
    enum Mode { M_NONE, M_KNOT, M_HANDLE, M_BOX, M_PENDING, M_CLICK } mode = M_NONE;
    float pressnx = 0.0f, pressny = 0.0f, lastnx = 0.0f, lastny = 0.0f;
    int pressmx = 0, pressmy = 0;
    int hk = -1, hwhich = 0;
    bool pendingmod = false;
    std::vector<float> fitsrc;
    int lastknots = 16;
    bool wasediting = false;
    std::string lastname;
    std::vector<Snap> undo, redo;
    // entry state (CANCEL)
    std::vector<BeamerMaps> entryset;
    std::vector<Assign> entryassign;
    std::string entryname, entrypath;
    bool entrydirty = false;
    // click cycling
    int cycId = -1, cycX = -100, cycY = -100;
};

View V;

void sync_beam_options();

Boxx *mkbox(const char *title, const char *tip, const float *ac) {
    Boxx *b = new Boxx;
    b->lcolor[0] = 0.7f; b->lcolor[1] = 0.7f; b->lcolor[2] = 0.7f;
    b->acolor[0] = ac[0]; b->acolor[1] = ac[1]; b->acolor[2] = ac[2]; b->acolor[3] = 1.0f;
    b->vtxcoords->w = 0.25f;
    b->vtxcoords->h = 0.07f;
    b->tooltiptitle = title;
    b->tooltip = tip;
    b->upvtxtoscr();
    return b;
}

Param *mkparam(const char *name, float deflt, float lo, float hi) {
    Param *p = new Param;
    p->name = name;
    p->value = deflt;
    p->deflt = deflt;
    p->range[0] = lo;
    p->range[1] = hi;
    p->sliding = false;
    p->box->vtxcoords->w = 0.25f;
    p->box->vtxcoords->h = 0.07f;
    p->box->upvtxtoscr();
    return p;
}

void init_ui() {
    if (V.inited) return;
    V.inited = true;
    V.beam = mkparam("Beamer", 0.0f, 0.0f, 1.0f);
    V.beam->type = FF_TYPE_OPTION;
    V.beam->options = {"No beamer"};
    V.beam->box->tooltiptitle = "Beamer ";
    V.beam->box->tooltip = "The beamer (external display) whose masks are shown. The masks are the ones made in the layer mapping view. ";
    V.name = mkparam("Name", 0.0f, 0.0f, 1.0f);
    V.name->type = FF_TYPE_TEXT;
    V.name->valuestr = g_name;
    V.name->box->tooltiptitle = "Name ";
    V.name->box->tooltip = "The name of the mapping file (without .epm) the masks and their edit curves are saved in, in the mappings folder next to the generations folder. Mappings are available to all projects. Click to edit, ENTER saves under the new name. ";
    V.color = mkparam("Exposure color", 0.0f, 0.0f, 1.0f);
    V.color->type = ISFLoader::PARAM_COLOR;
    V.color->colvalue[0] = V.color->colvalue[1] = V.color->colvalue[2] = 1.0f;
    V.color->defltcol[0] = V.color->defltcol[1] = V.color->defltcol[2] = 1.0f;
    V.color->box->tooltiptitle = "Exposure color ";
    V.color->box->tooltip = "The color EXPOSE shows on all masks of the beamer. ";
    V.knots = mkparam("Knots", 16.0f, 3.0f, 64.0f);
    V.knots->box->tooltiptitle = "Knots ";
    V.knots->box->tooltip = "The most knots the curve of an outline may have. A new outline (made in the layer mapping view) is described by as few knots as possible - corners get Broken or Constant knots, curved parts Smooth or Fluid ones - up to this number. With Edit outlines on, changing it describes the selected outline again with the number of knots as upper bound. ";
    V.lastknots = 16;
    V.inset = mkparam("Inset", 0.0f, 0.0f, 20.0f);
    V.inset->sliding = true;
    V.inset->box->tooltiptitle = "Inset ";
    V.inset->box->tooltip = "Beamer pixels the selected outline is moved inwards when something is projected on it. It works on the mask as it is after the curve edits, so the projection stays clear of the edge of the object whatever the curve does. 0 = exactly on the curve. Every outline has its own Inset, saved with it. ";
    V.feather = mkparam("Feather", 0.0f, 0.0f, 100.0f);
    V.feather->sliding = true;
    V.feather->box->tooltiptitle = "Feather ";
    V.feather->box->tooltip = "Beamer pixels over which the projection on the selected outline fades out at its edge: an alpha feathered edge, starting at the edge of the outline (after the Inset) and growing inwards. 0 = a sharp edge. Every outline has its own Feather, saved with it. ";
    V.content = mkparam("Content", 0.0f, 0.0f, 2.0f);
    V.content->type = FF_TYPE_OPTION;
    V.content->options = {"Fit", "Clip", "Grid"};
    V.content->box->tooltiptitle = "Content ";
    V.content->box->tooltip = "What the selected outline shows of the layer or mix. Fit: the whole frame, placed inside the outline (see Placement). Clip: the full beamer frame, cut out by the outline. Grid: a test grid instead of the picture, placed like Fit, to see how Warp follows the object: an even grid that follows the surface means a good map; gaps and kinks mean holes or wrong readings in it. ";
    V.warp = mkparam("Warp", 0.0f, 0.0f, 1.0f);
    V.warp->type = FF_TYPE_OPTION;
    V.warp->options = {"Off", "Surface"};
    V.warp->box->tooltiptitle = "Warp ";
    V.warp->box->tooltip = "Viewpoint (AI Depth): the frame follows the surface as seen from a viewpoint you set yourself (VIEWPOINT button): put the viewer where the audience is. Surface: the frame follows the surface the webcam saw (a tilted or curved object), as the webcam saw it. Plane (AI Depth): a plane is fitted to the shape from the AI depth estimate, and the frame is laid on it in real proportions (a square stays square on a tilted wall). Plane (2 webcams): the same, with the plane found from the two webcams (needs the Outline method with webcam 2 on). Only outlines made in the layer mapping view have a surface and the planes they could get; any of these implies Fit. ";
    V.placement = mkparam("Placement", 0.0f, 0.0f, 2.0f);
    V.placement->type = FF_TYPE_OPTION;
    V.placement->options = {"Fit", "Fill", "Stretch"};
    V.placement->box->tooltiptitle = "Placement ";
    V.placement->box->tooltip = "How the frame goes in the outline (with Content Fit or Warp). Fit: all of the frame, in proportion. Fill: the outline is covered, in proportion. Stretch: the frame takes the proportions of the outline. ";
    V.rot = mkparam("Rotate", 0.0f, -180.0f, 180.0f);
    V.rot->sliding = true;
    V.rot->box->tooltiptitle = "Rotate ";
    V.rot->box->tooltip = "Turns the frame in the outline, in degrees (Content Fit or Warp). ";
    V.scale = mkparam("Scale", 1.0f, 0.1f, 4.0f);
    V.scale->sliding = true;
    V.scale->box->tooltiptitle = "Scale ";
    V.scale->box->tooltip = "The size of the frame in the outline (Content Fit or Warp). ";
    V.shx = mkparam("Shift X", 0.0f, -1.0f, 1.0f);
    V.shx->sliding = true;
    V.shx->box->tooltiptitle = "Shift X ";
    V.shx->box->tooltip = "Moves the frame sideways in the outline, in frame widths (Content Fit or Warp). ";
    V.shy = mkparam("Shift Y", 0.0f, -1.0f, 1.0f);
    V.shy->sliding = true;
    V.shy->box->tooltiptitle = "Shift Y ";
    V.shy->box->tooltip = "Moves the frame up or down in the outline, in frame heights (Content Fit or Warp). ";
    const float gc[3] = {0.0f, 0.4f, 0.0f}, rc[3] = {0.5f, 0.1f, 0.1f};
    V.editbox = mkbox("Edit outlines ", "Toggles editing of the outlines: click an outline to select it, then drag its knots and handles, box select knots (SHIFT adds), move the selected knots with the arrow keys (one beamer pixel, SHIFT: ten), remove them with DEL, add a knot by clicking on the curve. The mask follows the changes live. Ctrl+Z / Ctrl+Y undo. Clicking inside an outline selects it (also with Edit outlines off; click again for the next outline under the pointer); use ASSIGN to project the layer or mix on the selected outline. Orange outlines are projected on, the selected one is white. ", gc);
    V.exposebox = mkbox("Expose ", "Toggles showing the exposure color on all masks of the beamer instead of projecting the layers / mixes: the outlines can be fitted to the real objects more clearly. ", gc);
    V.openbox = mkbox("Open ", "Opens a saved mapping file (.epm). It replaces the masks that are loaded now; the assignments of layers and mixes to masks are cleared. ", gc);
    V.newbox = mkbox("New mask ", "Makes a new square mask in the middle of the beamer image, described by four Constant knots on its corners. It is a control curve: the mask is the curve, rasterized at the full beamer resolution whenever the curve changes (not a pixel mask that a curve bends). Edit outlines stays on while it is selected; shape it with the knot tools as any other mask. ", gc);
    V.delbox = mkbox("Delete mask ", "Deletes the selected mask (any kind) with everything assigned to it. Select a mask with Edit outlines on first. ", rc);
    V.viewbox = mkbox("Viewpoint ", "Shows the scene as a point cloud (the AI depth of webcam 1) seen from the viewpoint of the selected outline: the viewer's position. Orbit with ALT + left mouse drag, pan with ALT + middle mouse drag, move closer or further with the wheel, move the pivot depth with CTRL + wheel (as in the Camera Warp editor). The projection on the outline follows live. Needs Warp = Viewpoint (AI Depth). The orange points are the outline itself. ", gc);
    V.assignbox = mkbox("Assign ", "Toggles whether the layer or mix is projected on the selected outline (orange outlines are projected on). Click an outline to select it first (click again for the next outline under the pointer). ", gc);
    V.resetbox = mkbox("Reset viewpoint ", "Puts the viewer back where webcam 1 is. ", gc);
    V.donebox = mkbox("Done ", "Leaves the Mapping view and keeps the changes (saved in the mapping file). ", gc);
    V.cancelbox = mkbox("Cancel ", "Reverts all changes made since the view was entered and leaves it. ", rc);
    const char *tn[4] = {"Smooth ", "Constant ", "Fluid ", "Broken "};
    const char *tt[4] = {
        "Smooth knots: the curve through them is smooth, their handles are set automatically. Applied to the selected knots. ",
        "Constant knots: no handles, the curve leaves them straight towards the neighbours. Applied to the selected knots. ",
        "Fluid knots: two handles that always lie opposite each other (a smooth curve you can steer). Applied to the selected knots. ",
        "Broken knots: two independent handles, for corners. Applied to the selected knots. "};
    for (int i = 0; i < 4; i++) V.tbox[i] = mkbox(tn[i], tt[i], gc);
}

Layer *any_layer() {
    Layer *l = mainmix->currlay[!mainprogram->prevmodus];
    if (l) return l;
    for (auto &sm : mainmix->layers) if (!sm.empty()) return sm[0];
    return nullptr;
}

std::string elem_label(const Elem &e) {
    if (e.kind == 1) {
        const char *nm[4] = {"Monitor A", "Monitor B", "Mix monitor", "Preview mix monitor"};
        return nm[std::clamp(e.mon, 0, 3)];
    }
    for (int i = 0; i < (int) mainmix->layers.size(); i++) {
        for (int j = 0; j < (int) mainmix->layers[i].size(); j++) {
            if (mainmix->layers[i][j] == e.lay) {
                const char *ln[4] = {"A", "B", "A", "B"};
                (void) ln;
                return "Layer " + std::to_string(j + 1) + " of deck " + (e.lay->deck ? "B" : "A") + (e.lay->comp ? " (mix)" : "");
            }
        }
    }
    return "Layer";
}

bool is_assigned(const std::string &key, int maskid, const Elem &e) {
    for (const Assign &a : g_assign) if (a.key == key && a.maskid == maskid && assign_matches(a, e)) return true;
    return false;
}

void set_assigned(const std::string &key, int maskid, const Elem &e, bool on) {
    for (size_t i = 0; i < g_assign.size(); i++) {
        const Assign &a = g_assign[i];
        if (a.key == key && a.maskid == maskid && assign_matches(a, e)) {
            if (!on) {
                g_assign.erase(g_assign.begin() + i);
                mark_changed(false);
            }
            return;
        }
    }
    if (on) {
        Assign a;
        a.key = key; a.maskid = maskid; a.kind = e.kind; a.lay = e.lay; a.mon = e.mon;
        g_assign.push_back(a);
        mark_changed(false);
    }
}

void push_undo() {
    BeamerMaps *bm = find_maps(V.curkey, false);
    if (!bm) return;
    Snap s;
    s.key = V.curkey;
    s.masks = bm->masks;
    V.undo.push_back(s);
    if (V.undo.size() > 100) V.undo.erase(V.undo.begin());
    V.redo.clear();
    V.fitsrc.clear();
}

void touch_mask(MapMask &m) {
    m.pdirty = true;
    mark_changed(true);
}

void sync_beam_options() {
    refresh_disps();
    std::string curkey = V.curkey;
    V.beam->options.clear();
    V.beamkeys.clear();
    V.beam->options.push_back("No beamer");
    V.beamkeys.push_back("");
    for (const Disp &d : g_disps) {
        V.beam->options.push_back(d.name);
        V.beamkeys.push_back(d.key);
    }
    int pos = 0;
    for (size_t i = 0; i < V.beamkeys.size(); i++) if (!curkey.empty() && V.beamkeys[i] == curkey) pos = (int) i;
    V.beam->value = (float) pos;
}

void do_open_file(const std::string &path, bool clearassign) {
    std::vector<BeamerMaps> loaded;
    if (!load_set_from(path, loaded)) {
        mainprogram->infostr = "Could not open mapping file " + path;
        return;
    }
    g_set = loaded;
    g_path = path;
    g_name = fs::path(path).stem().string();
    g_dirty = false;
    if (clearassign) g_assign.clear();
    for (BeamerMaps &b : g_set) for (MapMask &m : b.masks) m.pdirty = true;
    mark_changed(false);
    if (V.inited) { V.name->valuestr = g_name; V.insetkey = "\x01"; V.placekey = "\x01"; }
    g_loadedonce = true;
}

void refit_selected() {
    BeamerMaps *bm = find_maps(V.curkey, false);
    MapMask *m = find_mask(bm, V.sel);
    if (!m || m->ctrl) return;   // (a control curve is its own description: it is not fitted again)
    ensure_flat(*m, *bm);
    if (V.fitsrc.empty()) V.fitsrc = m->poly;
    std::vector<MapKnot> nk = fit_outline(V.fitsrc, bm->w, bm->h, (int) (V.knots->value + 0.5f));
    if (nk.size() < 3) return;
    std::vector<float> keep = V.fitsrc;
    m->k = nk;
    touch_mask(*m);
    V.fitsrc = keep;
    V.ksel.clear();
}

std::vector<int> masks_under(BeamerMaps *bm, float u, float v) {
    std::vector<std::pair<float, int>> hit;
    if (!bm) return {};
    for (size_t i = 0; i < bm->masks.size(); i++) {
        MapMask &m = bm->masks[i];
        ensure_flat(m, *bm);
        if (point_in_poly(m.poly, u, v)) hit.push_back({m.area, (int) i});
    }
    std::sort(hit.begin(), hit.end());
    std::vector<int> r;
    for (auto &h : hit) r.push_back(h.second);
    return r;
}

// the next mask of the stack under the pointer (click after click at the same spot goes round)
int cycle_pick(BeamerMaps *bm, const std::vector<int> &under, int sx, int sy) {
    if (under.empty()) { V.cycId = -1; return -1; }
    int pick = under[0];
    bool same = std::abs(sx - V.cycX) <= 4 && std::abs(sy - V.cycY) <= 4;
    if (same) {
        for (size_t i = 0; i < under.size(); i++) {
            if (bm->masks[under[i]].id == V.cycId) { pick = under[(i + 1) % under.size()]; break; }
        }
    }
    V.cycId = bm->masks[pick].id;
    V.cycX = sx; V.cycY = sy;
    return pick;
}

void set_knot_type(MapMask &m, int idx, int type) {
    bake_knot(m.k, idx);
    MapKnot &c = m.k[idx];
    c.type = type;
    if (type == KT_CONST) { c.hin[0] = c.hin[1] = c.hout[0] = c.hout[1] = 0.0f; }
    else if (type == KT_FLUID) { c.hin[0] = -c.hout[0]; c.hin[1] = -c.hout[1]; }
}

}  // namespace

// ======================================================================================================
// Public
// ======================================================================================================

void maskmap_store_outline(int disp, int w, int h, const std::vector<float> &poly, std::shared_ptr<WarpLut> warp) {
    refresh_disps();
    const Disp *d = nullptr;
    for (const Disp &x : g_disps) if (x.idx == disp) d = &x;
    if (!d) return;
    std::vector<MapKnot> k = fit_outline(poly, w, h, g_knotbudget);
    if (k.size() < 3) return;
    BeamerMaps *bm = find_maps(d->key, true);
    bm->w = w; bm->h = h;
    MapMask m;
    m.id = bm->nextid++;
    m.k = k;
    m.warp = warp;
    bm->masks.push_back(m);
    mark_changed(true);
    save_if_dirty();
}

void maskmap_set_scene(int disp, std::shared_ptr<ScenePoints> scene) {
    refresh_disps();
    const Disp *d = nullptr;
    for (const Disp &x : g_disps) if (x.idx == disp) d = &x;
    if (!d || !scene || scene->n <= 0) return;
    BeamerMaps *bm = find_maps(d->key, true);
    bm->scene = scene;
    mark_changed(true);
    save_if_dirty();
}

void maskmap_open_file(const std::string &path) {
    do_open_file(path, true);
}

std::string maskmap_current_path() {
    return g_path;
}

bool maskmap_isparam(Param *p) {
    if (!V.inited || !p) return false;
    return p == V.beam || p == V.name || p == V.color || p == V.knots || p == V.inset || p == V.feather || p == V.content || p == V.warp || p == V.placement ||
           p == V.rot || p == V.scale || p == V.shx || p == V.shy;
}

unsigned int maskmap_beam_tex(EWindow *win) {
    for (Out *o : g_outs) if (o->win == win) return o->show;
    return (unsigned int) -1;
}

static void enter_common() {
    init_ui();
    if (mainprogram->layerwarp) {
        mainprogram->layerwarpbut->value = 0;
        mainprogram->layerwarpbut->oldvalue = 0;
        layerwarp_leave();
    }
    if (mainprogram->projmap) projmap_leave();
    refresh_disps();
    V.on = true;
    mainprogram->mapview = true;
    V.edit = false;
    V.expose = false;
    V.sel = -1;
    V.ksel.clear();
    V.undo.clear();
    V.redo.clear();
    V.fitsrc.clear();
    V.zoom = 1.0f; V.zu = V.zv = 0.5f;
    V.prevdown = false;
    V.mode = View::M_NONE;
    V.name->valuestr = g_name;
    V.lastname = g_name;
    V.lastknots = (int) (V.knots->value + 0.5f);
    V.insetkey = "\x01";
    V.placekey = "\x01";
    V.entryset = g_set;
    V.entryassign = g_assign;
    V.entryname = g_name;
    V.entrypath = g_path;
    V.entrydirty = g_dirty;
    // the beamer: the one that was used last, else the first that has masks, else the first one
    std::string key = V.curkey;
    if (!disp_for_key(key)) {
        key.clear();
        for (const Disp &d : g_disps) {
            BeamerMaps *bm = find_maps(d.key, false);
            if (bm && !bm->masks.empty()) { key = d.key; break; }
        }
        if (key.empty() && !g_disps.empty()) key = g_disps[0].key;
    }
    V.curkey = key;
    sync_beam_options();
}

void maskmap_enter_layer(Layer *lay) {
    if (!lay) return;
    enter_common();
    V.el = Elem();
    V.el.kind = 0;
    V.el.lay = lay;
}

void maskmap_enter_monitor(int monitor) {
    enter_common();
    V.el = Elem();
    V.el.kind = 1;
    V.el.mon = std::clamp(monitor, 0, 3);
}

static void viewport_release();

void maskmap_leave() {
    if (V.inited) {
        V.viewing = false;
        viewport_release();
    }
    if (V.inited && V.on) {
        if (mainmix->adapttextparam == V.name) {
            mainmix->adapttextparam = nullptr;
            mainprogram->renaming = EDIT_NONE;
            SDL_StopTextInput(mainprogram->mainwindow);
        }
        save_if_dirty();
    }
    V.on = false;
    V.expose = false;
    g_exposeOn = false;
    mainprogram->mapview = false;
}

void maskmap_cancel() {
    if (V.inited && V.on) {
        g_set = V.entryset;
        g_assign = V.entryassign;
        g_name = V.entryname;
        g_path = V.entrypath;
        g_dirty = V.entrydirty;
        for (BeamerMaps &b : g_set) for (MapMask &m : b.masks) m.pdirty = true;
        mark_changed(false);
    }
    maskmap_leave();
}

bool maskmap_undo() {
    if (!V.on || !V.edit || V.undo.empty()) return V.on;
    Snap s = V.undo.back();
    V.undo.pop_back();
    BeamerMaps *bm = find_maps(s.key, false);
    if (bm) {
        Snap r;
        r.key = s.key;
        r.masks = bm->masks;
        V.redo.push_back(r);
        bm->masks = s.masks;
        for (MapMask &m : bm->masks) m.pdirty = true;
        mark_changed(true);
        if (!find_mask(bm, V.sel)) { V.sel = -1; }
        V.ksel.clear();
        V.fitsrc.clear();
    }
    return true;
}

bool maskmap_redo() {
    if (!V.on || !V.edit || V.redo.empty()) return V.on;
    Snap s = V.redo.back();
    V.redo.pop_back();
    BeamerMaps *bm = find_maps(s.key, false);
    if (bm) {
        Snap r;
        r.key = s.key;
        r.masks = bm->masks;
        V.undo.push_back(r);
        bm->masks = s.masks;
        for (MapMask &m : bm->masks) m.pdirty = true;
        mark_changed(true);
        if (!find_mask(bm, V.sel)) V.sel = -1;
        V.ksel.clear();
        V.fitsrc.clear();
    }
    return true;
}

// No mouse pointer on output windows, on every platform: SDL has no cursor setting per window, but its event watch is called
// at the moment the pointer enters a window, before the cursor is drawn for it: the cursor is hidden (or shown again, for the
// main window) there, instead of a frame later (maskmap_tick keeps the same state when no event was seen).
static bool g_ptrhidden = false;

static bool is_output_window(SDL_Window *w) {
    if (!w) return false;
    for (OutputEntry *e : mainprogram->outputentries) if (e->win && e->win->win == w) return true;
    return false;
}

static bool SDLCALL pointer_watch(void *, SDL_Event *ev) {
    if (ev->type == SDL_EVENT_WINDOW_MOUSE_ENTER) {
        const bool out = is_output_window(SDL_GetWindowFromID(ev->window.windowID));
        if (out != g_ptrhidden) {
            if (out) SDL_HideCursor();
            else SDL_ShowCursor();
            g_ptrhidden = out;
        }
    }
    return true;
}

#ifdef WINDOWS
static std::map<HWND, WNDPROC> g_oldprocs;   // the window procedures of the beamer windows before ours

static LRESULT CALLBACK nocursor_proc(HWND h, UINT m, WPARAM w, LPARAM l) {
    auto it = g_oldprocs.find(h);
    if (m == WM_SETCURSOR && LOWORD(l) == HTCLIENT) {
        SetCursor(NULL);   // (asked for the cursor over the window: none)
        return TRUE;
    }
    if (it == g_oldprocs.end()) return DefWindowProc(h, m, w, l);
    return CallWindowProc(it->second, h, m, w, l);
}
#endif

void maskmap_hide_pointer(SDL_Window *win, bool hide) {
#ifdef WINDOWS
    if (!win) return;
    HWND h = (HWND) SDL_GetPointerProperty(SDL_GetWindowProperties(win), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
    if (!h) return;
    auto it = g_oldprocs.find(h);
    if (hide && it == g_oldprocs.end()) {
        WNDPROC old = (WNDPROC) SetWindowLongPtr(h, GWLP_WNDPROC, (LONG_PTR) nocursor_proc);
        if (old) g_oldprocs[h] = old;
    }
    else if (!hide && it != g_oldprocs.end()) {
        SetWindowLongPtr(h, GWLP_WNDPROC, (LONG_PTR) it->second);
        g_oldprocs.erase(it);
    }
#else
    (void) win; (void) hide;
#endif
}

void maskmap_layer_transfer(Layer *from, Layer *to) {
    if (!from || !to || from == to) return;
    // (the new layer is not in the mix yet: the assignments move when the old one is gone and the new one is there)
    for (auto &m : g_moves) if (m.second == from) m.second = to;   // (a layer replaced twice before the swap)
    g_moves.push_back(Move{from, to, 0});
}

void maskmap_tick() {
    // layers that were replaced by a new one (a video or image opened in them): their assignments go along
    for (size_t i = 0; i < g_moves.size();) {
        const bool fromalive = layer_alive(g_moves[i].first), toalive = layer_alive(g_moves[i].second);
        if (!fromalive && toalive) {
            for (Assign &a : g_assign) if (a.kind == 0 && a.lay == g_moves[i].first) { a.lay = g_moves[i].second; g_ver++; }
        }
        if ((toalive && !fromalive) || (!toalive && ++g_moves[i].stale > 600)) g_moves.erase(g_moves.begin() + i);
        else i++;
    }
    static int cnt = 0;
    if ((cnt++ % 30) == 0) refresh_disps();
    if (g_loadgrace > 0) g_loadgrace--;
    // assignments: pending layers are found, closed layers / removed masks go
    for (size_t i = 0; i < g_assign.size();) {
        Assign &a = g_assign[i];
        bool drop = false;
        if (a.kind == 0) {
            if (!a.lay && a.plist >= 0 && a.plist < (int) mainmix->layers.size() && a.pidx >= 0 && a.pidx < (int) mainmix->layers[a.plist].size()) {
                a.lay = mainmix->layers[a.plist][a.pidx];
                if (a.lay) g_ver++;   // (the projection is built from the assignments that have their layer: build it again)
            }
            if (a.lay && !layer_alive(a.lay)) {
                // (just after a project was opened its layers may still be replaced while it loads: the assignment goes back to
                // the place in the project and finds its layer again; later a layer that is gone takes its assignments with it)
                bool moving = false;
                for (const Move &mv : g_moves) if (mv.first == a.lay) moving = true;   // (replaced by a new layer that is not in the mix yet)
                if (moving) {}
                else if (g_loadgrace > 0 && a.plist >= 0) a.lay = nullptr;
                else drop = true;
            }
            if (!a.lay && g_loadgrace <= 0 && a.plist >= 0 && !drop) {
                // (the place did not exist for the whole wait: nothing to wait for)
                if (a.plist >= (int) mainmix->layers.size() || a.pidx >= (int) mainmix->layers[a.plist].size()) drop = true;
            }
        }
        if (!drop && g_loadedonce) {
            BeamerMaps *bm = find_maps(a.key, false);
            if (bm && !find_mask(bm, a.maskid)) drop = true;
        }
        if (drop) { g_assign.erase(g_assign.begin() + i); g_ver++; }
        else i++;
    }
    // the view's exposure
    g_exposeOn = V.inited && V.on && V.expose;
    g_exposeKey = V.curkey;
    {
        // (another outline selected, or EXPOSE just switched: the exposure mask is made again)
        const int sel = g_exposeOn ? V.sel : -1;
        if (sel != g_exposeSel) { g_exposeSel = sel; g_ver++; }
    }
    if (V.inited) {
        g_exposeCol[0] = V.color->colvalue[0];
        g_exposeCol[1] = V.color->colvalue[1];
        g_exposeCol[2] = V.color->colvalue[2];
    }
    // which beamers need an output
    std::set<std::string> need;
    for (const Assign &a : g_assign) need.insert(a.key);   // (also layers still to be found: no flicker while a project loads)
    if (g_exposeOn && !g_exposeKey.empty()) need.insert(g_exposeKey);
    // While the view is open the beamer window of its beamer stays (black when nothing is assigned): clicking a mask off and on
    // again does not close and reopen the window
    if (V.inited && V.on) {
        for (Out *o : g_outs) if (o->key == V.curkey) need.insert(o->key);
    }
    for (size_t i = 0; i < g_outs.size();) {
        Out *o = g_outs[i];
        const Disp *d = disp_for_key(o->key);
        if (o->win) {
            // (the window was closed by someone else, e.g. Show on display was switched off: it is gone, not to be touched)
            bool alive = false;
            for (OutputEntry *e : mainprogram->outputentries) if (e->win == o->win) alive = true;
            if (!alive) { o->win = nullptr; o->borrowed = false; free_out_gl(o); }
        }
        bool keep = need.count(o->key) && d;
        if (keep && o->win && d->idx != o->disp) { close_out(o); }   // (the display changed)
        if (!keep) {
            close_out(o);
            delete o;
            g_outs.erase(g_outs.begin() + i);
        }
        else i++;
    }
    for (const std::string &key : need) {
        const Disp *d = disp_for_key(key);
        if (!d) continue;
        Out *o = find_out(key);
        if (!o) {
            o = new Out;
            o->key = key;
            g_outs.push_back(o);
        }
        if (!o->win) {
            o->disp = d->idx;
            if (!open_out_window(o, d->idx)) continue;   // (ProjMap has the display: later)
        }
    }
    for (Out *o : g_outs) {
        if (!o->win) continue;
        // (MAPPING mode was left: its window is ours again)
        if (o->win->mixid == 5 && !mainprogram->projmap) o->win->mixid = 6;
        render_out(o);
    }
    // The mouse pointer is hidden while it is over an output window (a beamer of the mapping, or "Show on display"): it would be
    // projected on the objects / shown on the screen of the audience
    {
        static bool registered = false;
        if (!registered) {
            registered = true;
            SDL_AddEventWatch(pointer_watch, nullptr);
        }
        SDL_Window *under = SDL_GetMouseFocus();
        const bool over = is_output_window(under);
        if (over != g_ptrhidden) {
            if (over) SDL_HideCursor();
            else SDL_ShowCursor();
            g_ptrhidden = over;
        }
    }
}

void maskmap_project_save(std::ostream &o) {
    save_if_dirty();
    o << "MAPPINGFILE\n" << g_path << "\n";
    std::vector<std::string> lines;
    for (const Assign &a : g_assign) {
        int pl = a.plist, pi = a.pidx;
        if (a.kind == 0) {
            if (a.lay) {
                pl = -1; pi = -1;
                for (int i = 0; i < (int) mainmix->layers.size() && pl < 0; i++) {
                    for (int j = 0; j < (int) mainmix->layers[i].size(); j++) {
                        if (mainmix->layers[i][j] == a.lay) { pl = i; pi = j; break; }
                    }
                }
            }
            if (pl < 0) continue;
        }
        lines.push_back(a.key + "|" + std::to_string(a.kind) + "|" + std::to_string(pl) + "|" + std::to_string(pi) + "|" +
                        std::to_string(a.mon) + "|" + std::to_string(a.maskid));
    }
    o << "MAPASSIGNS\n" << lines.size() << "\n";
    for (const std::string &l : lines) o << l << "\n";
}

bool maskmap_project_tag(const std::string &tag, std::istream &is) {
    if (tag == "MAPPINGFILE") {
        std::string path;
        safegetline(is, path);
        if (!path.empty() && !(path == g_path && g_loadedonce)) {
            std::vector<BeamerMaps> loaded;
            if (load_set_from(path, loaded)) {
                g_set = loaded;
                g_path = path;
                g_name = fs::path(path).stem().string();
                g_dirty = false;
                for (BeamerMaps &b : g_set) for (MapMask &m : b.masks) m.pdirty = true;
                if (V.inited) V.name->valuestr = g_name;
                g_loadedonce = true;
                g_ver++;
            }
        }
        return true;
    }
    if (tag == "MAPASSIGNS") {
        std::string line;
        safegetline(is, line);
        int n = atoi(line.c_str());
        g_assign.clear();
        for (int i = 0; i < n; i++) {
            if (!safegetline(is, line)) break;
            std::vector<std::string> parts;
            size_t st = 0;
            while (true) {
                size_t p = line.find('|', st);
                if (p == std::string::npos) { parts.push_back(line.substr(st)); break; }
                parts.push_back(line.substr(st, p - st));
                st = p + 1;
            }
            if (parts.size() < 6) continue;
            Assign a;
            a.key = parts[0];
            a.kind = atoi(parts[1].c_str());
            a.plist = atoi(parts[2].c_str());
            a.pidx = atoi(parts[3].c_str());
            a.mon = atoi(parts[4].c_str());
            a.maskid = atoi(parts[5].c_str());
            g_assign.push_back(a);
        }
        g_ver++;
        g_loadgrace = 600;
        return true;
    }
    return false;
}

void maskmap_project_reset() {
    g_assign.clear();
    g_ver++;
}

// ---- the Viewpoint view: the point cloud seen from the camera of an outline (the code of the Camera Warp editor, on the canvas) ----

static void viewport_release() {
    if (V.vdrag) {
        V.vdrag = false;
        SDL_SetWindowRelativeMouseMode(mainprogram->mainwindow, false);
    }
}

// fills a vertex buffer of points (16 bytes each: x, y, z floats, r, g, b, a bytes)
static void upload_points(GLuint &vao, GLuint &vbo, const void *data, size_t bytes) {
    GLint prevvao = 0, prevabuf = 0;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevvao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prevabuf);
    if (!vao) {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 16, (void *) 0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, 16, (void *) 12);
    }
    else {
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
    }
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr) bytes, data, GL_STATIC_DRAW);
    glBindVertexArray((GLuint) prevvao);
    glBindBuffer(GL_ARRAY_BUFFER, (GLuint) prevabuf);
}

// Alt + left drag orbits, Alt + middle drag pans, the wheel moves closer / further, Ctrl + wheel moves the pivot in depth. Returns true
// when the camera changed.
static bool viewport_input(MapMask &m, bool inside) {
    const bool altHeld = (SDL_GetModState() & SDL_KMOD_ALT) != 0;
#ifdef MACOS
    const bool ctrlHeld = (SDL_GetModState() & SDL_KMOD_GUI) != 0;
#else
    const bool ctrlHeld = (SDL_GetModState() & SDL_KMOD_CTRL) != 0;
#endif
    const SDL_MouseButtonFlags mb = SDL_GetMouseState(nullptr, nullptr);
    const bool wantOrbit = altHeld && (mb & SDL_BUTTON_LMASK);
    const bool wantPan = altHeld && (mb & SDL_BUTTON_MMASK);
    const bool wantDrag = wantOrbit || wantPan;
    bool changed = false;
    OrbitCamera &camera = m.cam;
    if (!V.vdrag && wantDrag && inside) {
        V.vdrag = true;
        V.vdragorbit = wantOrbit;
        push_undo();
        SDL_SetWindowRelativeMouseMode(mainprogram->mainwindow, true);
        float dx, dy;
        SDL_GetRelativeMouseState(&dx, &dy);
    }
    if (V.vdrag) {
        float relX = 0.0f, relY = 0.0f;
        SDL_GetRelativeMouseState(&relX, &relY);
        if (V.vdragorbit) {
            camera.azimuthDeg -= relX * 0.4f;
            camera.wrapAzimuth();
            camera.elevationDeg -= relY * 0.4f;
            camera.clampElevation();
        }
        else {
            // (moving the pivot re-aims the camera: azimuth / elevation are solved again so that it keeps looking the same way)
            float oldEye[3], oldTarget[3];
            camera.getEyeAndTargetServerFrame(oldEye, oldTarget);
            float oldForward[3] = {oldTarget[0] - oldEye[0], oldTarget[1] - oldEye[1], oldTarget[2] - oldEye[2]};
            const float scale = std::max(camera.pivot[2], 0.1f) * 0.0025f;
            camera.pivot[0] += relX * scale;
            camera.pivot[1] += relY * scale;
            float newAz, newEl;
            if (camera.solveAzElForForward(camera.pivot, oldForward, camera.azimuthDeg, camera.elevationDeg, newAz, newEl)) {
                camera.azimuthDeg = newAz;
                camera.wrapAzimuth();
                camera.elevationDeg = newEl;
                camera.clampElevation();
            }
        }
        changed = true;
        if (!wantDrag) viewport_release();
    }
    if (mainprogram->mousewheel != 0.0f && inside) {
        if (ctrlHeld) {
            camera.pivot[2] *= (1.0f - mainprogram->mousewheel * 0.05f);
            camera.pivot[2] = std::clamp(camera.pivot[2], 0.01f, 1000.0f);
        }
        else {
            camera.distance *= (1.0f - mainprogram->mousewheel * 0.03f);
            camera.distance = std::clamp(camera.distance, 0.1f, 3.0f);
        }
        mainprogram->mousewheel = 0.0f;
        changed = true;
    }
    return changed;
}

// draws the point cloud of the scene (and the points of the outline in orange) in the rectangle (vertex coordinates) as the camera sees it
static void draw_cloud(MapMask &m, const std::shared_ptr<ScenePoints> &sc, float rx, float ry, float rw, float rh) {
    if (V.pcfailed || !sc || sc->n <= 0 || !m.warp) return;
    if (!V.pcprog) {
        V.pcprog = mainprogram->set_shader_from_files("pointcloud.vs", "pointcloud.fs");
        if (!V.pcprog) { V.pcfailed = true; return; }
    }
    if (V.pcscene != sc) {
        upload_points(V.pcvao, V.pcvbo, sc->data.data(), sc->data.size());
        V.pcscene = sc;
    }
    {
        // The curve of the outline itself, in 3D: its points on the beamer image looked up in the grid of 3D positions (between the
        // cells that are known). Made again every frame: it is small, and the curve may be edited.
        const WarpLut &W = *m.warp;
        std::vector<uint8_t> hp;
        V.hin = 0;
        const int np = (int) m.poly.size() / 2;
        const float step = 0.5f / (float) std::max(W.gw, W.gh);
        auto at3 = [&](float x, float y, float *out) {
            const float fx = x * W.gw - 0.5f, fy = y * W.gh - 0.5f;
            const int x0 = (int) std::floor(fx), y0 = (int) std::floor(fy);
            float acc[3] = {0.0f, 0.0f, 0.0f}, wsum = 0.0f;
            for (int dy = 0; dy <= 1; dy++) {
                for (int dx = 0; dx <= 1; dx++) {
                    const int cx = std::clamp(x0 + dx, 0, W.gw - 1), cy = std::clamp(y0 + dy, 0, W.gh - 1);
                    const size_t i = (size_t) cy * W.gw + cx;
                    if (W.pos3[i * 3 + 2] <= 0.0f) continue;
                    const float wx = dx ? fx - (float) x0 : 1.0f - (fx - (float) x0), wy = dy ? fy - (float) y0 : 1.0f - (fy - (float) y0);
                    const float wt = wx * wy + 1e-4f;
                    for (int k = 0; k < 3; k++) acc[k] += W.pos3[i * 3 + k] * wt;
                    wsum += wt;
                }
            }
            if (wsum <= 0.0f) return false;
            for (int k = 0; k < 3; k++) out[k] = acc[k] / wsum;
            return true;
        };
        for (int i = 0; i < np; i++) {
            const int j = (i + 1) % np;
            const float ax = m.poly[i * 2], ay = m.poly[i * 2 + 1], bx = m.poly[j * 2], by = m.poly[j * 2 + 1];
            const int ns = std::max(1, (int) (std::hypot(bx - ax, by - ay) / step));
            for (int s = 0; s < ns; s++) {
                const float t = (float) s / (float) ns;
                float P[3];
                if (!at3(ax + (bx - ax) * t, ay + (by - ay) * t, P)) continue;
                const uint8_t col[4] = {255, 140, 0, 255};
                const size_t at = hp.size();
                hp.resize(at + 16);
                memcpy(&hp[at], P, 12);
                memcpy(&hp[at + 12], col, 4);
                V.hin++;
            }
        }
        if (V.hin > 0) upload_points(V.hivao, V.hivbo, hp.data(), hp.size());
    }
    int px = (int) ((rx + 1.0f) * 0.5f * glob->w), py = (int) ((ry + 1.0f) * 0.5f * glob->h);
    int pw = (int) (rw * 0.5f * glob->w), ph = (int) (rh * 0.5f * glob->h);
    if (pw <= 0 || ph <= 0) return;
    {
        // (the picture of webcam 1 has its own proportions: the view is letterboxed to them, so that the default viewpoint is that picture)
        const float wa = std::max(0.2f, sc->aspect);
        if ((float) pw / (float) ph > wa) { const int nw = (int) ((float) ph * wa); px += (pw - nw) / 2; pw = nw; }
        else { const int nh = (int) ((float) pw / wa); py += (ph - nh) / 2; ph = nh; }
        if (pw <= 0 || ph <= 0) return;
    }
    GLint prevViewport[4], prevProgram = 0, prevVAO = 0, prevFB = 0;
    glGetIntegerv(GL_VIEWPORT, prevViewport);
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVAO);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFB);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDrawBuffer_Back();
    glEnable(GL_SCISSOR_TEST);
    glScissor(px, py, pw, ph);
    glViewport(px, py, pw, ph);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
#ifndef USE_GLES
    glEnable(GL_PROGRAM_POINT_SIZE);
#endif
    const Mat4 mvp = mat4Multiply(m.cam.getProjectionMatrix((float) pw / (float) ph, 0.01f, 100.0f), m.cam.getViewMatrix());
    glUseProgram(V.pcprog);
    glUniformMatrix4fv(glGetUniformLocation(V.pcprog, "MVP"), 1, GL_FALSE, mvp.m);
    glUniform1f(glGetUniformLocation(V.pcprog, "PointSize"), 3.0f);
    glBindVertexArray(V.pcvao);
    glDrawArrays(GL_POINTS, 0, (GLsizei) sc->n);
    if (V.hin > 0) {
        glDisable(GL_DEPTH_TEST);   // (the outline is always visible, also where the surface hides it)
        glUniform1f(glGetUniformLocation(V.pcprog, "PointSize"), 6.0f);
        glBindVertexArray(V.hivao);
        glDrawArrays(GL_POINTS, 0, (GLsizei) V.hin);
    }
    glBindVertexArray((GLuint) prevVAO);
    glUseProgram((GLuint) prevProgram);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint) prevFB);
}

void maskmap_ui() {
    if (!V.on || !V.inited) return;
    BeamerMaps *bm = find_maps(V.curkey, false);

    // ---- geometry (the same canvas as the layer warp / mapping view) ----
    float top = mainprogram->scrollboxes[0]->vtxcoords->y1 - 0.012f;
    float bottom = mainmix->crossfade->box->vtxcoords->y1 + mainmix->crossfade->box->vtxcoords->h + 0.012f;
    float chh = top - bottom;
    float aspect = 16.0f / 9.0f * (float) glob->h / (float) glob->w;
    float cww = chh * aspect;
    if (cww > 1.9f) { cww = 1.9f; chh = cww / aspect; }
    const float cvx = -cww * 0.5f, cvy = bottom + (top - bottom - chh) * 0.5f, cvw = cww, cvh = chh;

    // The panel: with Edit outlines on there are more rows (the knot controls and how the frame is placed in the outline);
    // when they do not fit in the canvas height the rows get smaller
    const float panelw = 0.25f;
    // (the controls that belong to an outline are there once one is selected)
    const bool haveSel = bm && find_mask(bm, V.sel);
    const int nrows = !haveSel ? 8 : (V.edit ? 16 : 13);
    const float rowh = std::min(0.071f, (cvh - 0.01f) / (float) nrows), boxh = rowh - 0.007f;
    float cyy = cvy + cvh * 0.5f;
    float px0 = std::max(-0.99f, cvx - panelw - 0.01f);
    float ptop = std::min(cvy + cvh - 0.003f, cyy + rowh * (float) nrows * 0.5f);
    auto rowy = [&](int r) { return ptop - rowh * r + 0.004f; };
    auto place = [&](Boxx *b, float x, float y, float w) {
        b->vtxcoords->x1 = x; b->vtxcoords->y1 = y; b->vtxcoords->w = w; b->vtxcoords->h = boxh;
        b->upvtxtoscr();
    };
    const float half = panelw * 0.5f - 0.003f, hx = px0 + panelw * 0.5f + 0.003f;
    place(V.beam->box, px0, rowy(1), panelw);
    place(V.name->box, px0, rowy(2), panelw);
    place(V.openbox, px0, rowy(3), half);
    place(V.exposebox, hx, rowy(3), half);
    place(V.color->box, px0, rowy(4), panelw);
    place(V.editbox, px0, rowy(5), half);
    place(V.assignbox, hx, rowy(5), half);
    int rbase = 5;
    if (haveSel) {
        // (an outline without a surface, from NEW MASK, has no Warp: Content takes the row)
        const MapMask *hm = find_mask(bm, V.sel);
        const bool hasw = hm && hm->warp;
        (void) hasw;
        place(V.content->box, px0, rowy(6), half);
        place(V.placement->box, hx, rowy(6), half);
        place(V.warp->box, px0, rowy(7), panelw);
        place(V.rot->box, px0, rowy(8), half);
        place(V.scale->box, hx, rowy(8), half);
        place(V.shx->box, px0, rowy(9), half);
        place(V.shy->box, hx, rowy(9), half);
        place(V.inset->box, px0, rowy(10), half);
        place(V.feather->box, hx, rowy(10), half);
        rbase = 10;
    }
    if (haveSel && V.edit) {
        place(V.knots->box, px0, rowy(11), panelw);
        place(V.tbox[0], px0, rowy(12), half);
        place(V.tbox[1], hx, rowy(12), half);
        place(V.tbox[2], px0, rowy(13), half);
        place(V.tbox[3], hx, rowy(13), half);
        place(V.viewbox, px0, rowy(14), half);
        place(V.resetbox, hx, rowy(14), half);
        rbase = 14;
    }
    else if (haveSel) {
        place(V.viewbox, px0, rowy(11), half);
        place(V.resetbox, hx, rowy(11), half);
        rbase = 11;
    }
    place(V.newbox, px0, rowy(rbase + 1), half);
    place(V.delbox, hx, rowy(rbase + 1), half);
    place(V.donebox, px0, rowy(rbase + 2), half);
    place(V.cancelbox, hx, rowy(rbase + 2), half);
    const float panel_x0 = px0 - 0.005f, panel_x1 = px0 + panelw + 0.005f, panel_y0 = ptop - rowh * (float) (rbase + 2) - 0.005f;

    float mnx = mainprogram->xscrtovtx((float) mainprogram->mx) - 1.0f;
    float mny = 1.0f - mainprogram->yscrtovtx((float) mainprogram->my);
    bool inpanel = mnx >= panel_x0 && mnx <= panel_x1 && mny >= panel_y0 && mny <= cvy + cvh;
    bool incanvas = mnx >= cvx && mnx <= cvx + cvw && mny >= cvy && mny <= cvy + cvh;
    bool nomenu = !mainprogram->menuondisplay;
    bool typing = mainprogram->renaming != EDIT_NONE;

    // the beamer image, letterboxed in the canvas (above the hint line)
    float barh = 0.0f;
    float areay = cvy + barh, areah = cvh - barh;
    float bar = bm ? (float) bm->w / (float) std::max(1, bm->h) : 16.0f / 9.0f;
    float iw = cvw, ih = areah, irx = cvx, iry = areay;
    {
        float boxaspect = (cvw * (float) glob->w) / (areah * (float) glob->h);
        if (bar > boxaspect) {
            ih = cvw * (float) glob->w / bar / (float) glob->h;
            iry = areay + (areah - ih) * 0.5f;
        }
        else {
            iw = areah * (float) glob->h * bar / (float) glob->w;
            irx = cvx + (cvw - iw) * 0.5f;
        }
    }

    // The Viewpoint view is on while an outline with Warp = Viewpoint (AI Depth) is selected and the view button is on
    {
        MapMask *vm = bm ? find_mask(bm, V.sel) : nullptr;
        if (!(vm && vm->warpon == 4 && view_possible(*vm) && bm && bm->scene)) V.viewing = false;
        if (!V.viewing) viewport_release();
    }
    const bool vwing = V.viewing;

    // zoom towards the pointer
    if (mainprogram->mousewheel != 0.0f && nomenu && !inpanel && !typing && !vwing) {
        float relx = (mnx - irx) / iw, rely = (iry + ih - mny) / ih;
        if (relx >= 0.0f && relx <= 1.0f && rely >= 0.0f && rely <= 1.0f) {
            float u = V.zu - 0.5f / V.zoom + relx / V.zoom, v = V.zv - 0.5f / V.zoom + rely / V.zoom;
            float nz = std::clamp(V.zoom * (1.0f + 0.15f * mainprogram->mousewheel), 1.0f, 12.0f);
            V.zu = std::clamp(u - relx / nz + 0.5f / nz, 0.5f / nz, 1.0f - 0.5f / nz);
            V.zv = std::clamp(v - rely / nz + 0.5f / nz, 0.5f / nz, 1.0f - 0.5f / nz);
            V.zoom = nz;
            mainprogram->mousewheel = 0.0f;
        }
    }
    // pan with the middle mouse button
    {
        const bool mdown = (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_MMASK) != 0;
        if (mdown && !V.mprev && V.zoom > 1.0f && !vwing && mnx >= irx && mnx <= irx + iw && mny >= iry && mny <= iry + ih && nomenu && !inpanel && !typing)
            V.panning = true;
        if (!mdown) V.panning = false;
        if (V.panning) {
            float dnx = mnx - V.panmx, dny = mny - V.panmy;
            V.zu = std::clamp(V.zu - dnx / (iw * V.zoom), 0.5f / V.zoom, 1.0f - 0.5f / V.zoom);
            V.zv = std::clamp(V.zv + dny / (ih * V.zoom), 0.5f / V.zoom, 1.0f - 0.5f / V.zoom);
        }
        V.panmx = mnx; V.panmy = mny;
        V.mprev = mdown;
    }
    const float vz = V.zoom, vu0 = V.zu - 0.5f / vz, vv0 = V.zv - 0.5f / vz;
    auto uv2n = [&](float u, float v, float &nx, float &ny) { nx = irx + (u - vu0) * vz * iw; ny = iry + ih - (v - vv0) * vz * ih; };
    float mu = vu0 + ((mnx - irx) / iw) / vz, mv = vv0 + ((iry + ih - mny) / ih) / vz;
    auto sdist = [&](float ax, float ay, float bx, float by) {
        float dx = (ax - bx) * (float) glob->w * 0.5f, dy = (ay - by) * (float) glob->h * 0.5f;
        return std::sqrt(dx * dx + dy * dy);
    };
    bool inimg = mnx >= irx && mnx <= irx + iw && mny >= iry && mny <= iry + ih;

    // ---- the controls ----
    bool budm = mainprogram->directmode;
    mainprogram->directmode = true;

    // the Name was edited (ENTER): save under that name
    {
        bool editing = mainmix->adapttextparam == V.name;
        if (V.wasediting && !editing) {
            std::string nm = V.name->valuestr;
            std::string clean;
            for (char c : nm) if (std::string("\\/:*?\"<>|").find(c) == std::string::npos) clean += c;
            while (!clean.empty() && clean.back() == ' ') clean.pop_back();
            while (!clean.empty() && clean.front() == ' ') clean.erase(clean.begin());
            if (clean.empty()) clean = g_name;
            V.name->valuestr = clean;
            if (clean != g_name || g_path.empty()) {
                g_name = clean;
                // (in the folder the file is in: a file opened from elsewhere stays there; a new one goes in the mappings folder)
                g_path = g_path.empty() ? path_for_name(g_name)
                                        : (fs::path(g_path).parent_path() / (g_name + ".epm")).generic_string();
                g_dirty = true;
                save_if_dirty();
            }
        }
        V.wasediting = editing;
    }
    // the beamer option
    {
        int bi = (int) (V.beam->value + 0.5f);
        std::string nk = (bi >= 0 && bi < (int) V.beamkeys.size()) ? V.beamkeys[bi] : "";
        if (nk != V.curkey) {
            V.curkey = nk;
            V.sel = -1;
            V.ksel.clear();
            V.undo.clear();
            V.redo.clear();
            V.fitsrc.clear();
            bm = find_maps(V.curkey, false);
        }
    }
    // The outline the placement controls (Inset, Content, Warp, Placement, Rotate, Scale, Shift) work on: the selected one
    std::vector<MapMask *> tgts;
    if (bm) {
        MapMask *sm0 = find_mask(bm, V.sel);
        if (sm0) tgts.push_back(sm0);
    }
    {
        // The Inset slider shows the Inset of the first of them, and moving it changes all of them (the mask is projected moved
        // inwards by it)
        std::string ikey;
        for (MapMask *t : tgts) ikey += std::to_string(t->id) + ",";
        if (!ikey.empty()) ikey = V.curkey + "|" + ikey;
        if (ikey != V.insetkey) {
            V.insetkey = ikey;
            if (!tgts.empty()) { V.inset->value = tgts[0]->inset; V.feather->value = tgts[0]->feather; }
            V.lastinset = V.inset->value;
            V.lastfeather = V.feather->value;
        }
        const float fv = std::round(V.feather->value * 10.0f) / 10.0f;
        if (fv != V.lastfeather) {
            V.lastfeather = fv;
            if (!tgts.empty()) {
                for (MapMask *t : tgts) t->feather = fv;
                mark_changed(true);
            }
        }
        const float iv = std::round(V.inset->value * 10.0f) / 10.0f;
        if (iv != V.lastinset) {
            V.lastinset = iv;
            if (!tgts.empty()) {
                for (MapMask *t : tgts) t->inset = iv;
                mark_changed(true);
            }
        }
    }
    {
        // How the frame is placed in the outlines (Content, Warp, Placement, Rotate, Scale, Shift): the controls show the
        // values of the first, and moving them changes those of all
        MapMask *pm = tgts.empty() ? nullptr : tgts[0];
        std::string pkey;
        for (MapMask *t : tgts) pkey += std::to_string(t->id) + ",";
        if (!pkey.empty()) pkey = V.curkey + "|" + pkey;
        // the Warp options: Off, the surface as the webcam sees it, and the planes the first outline with a surface has
        MapMask *wm = pm;
        for (MapMask *t : tgts) if (t->warp) { wm = t; break; }
        if (pm) {
            // (Grid is a test of the Warp: an outline without a surface has Clip and Fit only)
            if (pm->warp) V.content->options = {"Fit", "Clip", "Grid"};
            else {
                V.content->options = {"Fit", "Clip"};
                V.content->range[1] = 1.0f;
                if (pm->content > 1) { pm->content = 1; mark_changed(true); }
                if (V.content->value > 1.0f) { V.content->value = 0.0f; V.lastcontent = 0.0f; }   // (Grid is gone: Fit)
            }
            if (pm->warp) V.content->range[1] = 2.0f;
            V.warpmodes.assign(1, 0);
            std::vector<std::string> names = {"Off"};
            if (wm->warp) {
                V.warpmodes.push_back(1); names.push_back("Surface");
                if (wm->warp->plane[0].valid) { V.warpmodes.push_back(2); names.push_back("Plane (AI Depth)"); }
                if (wm->warp->plane[1].valid) { V.warpmodes.push_back(3); names.push_back("Plane (2 webcams)"); }
                if (view_possible(*wm)) { V.warpmodes.push_back(4); names.push_back("Viewpoint (AI Depth)"); }
            }
            V.warp->options = names;
            V.warp->range[1] = (float) std::max<size_t>(1, names.size() - 1);
            if (V.warp->value > (float) (names.size() - 1)) V.warp->value = (float) (names.size() - 1);
        }
        auto warp_index = [&](int mode) {
            for (size_t i = 0; i < V.warpmodes.size(); i++) if (V.warpmodes[i] == mode) return (int) i;
            return 0;
        };
        if (pkey != V.placekey) {
            V.placekey = pkey;
            if (pm) {
                V.content->value = (float) content_ui(pm->content);
                V.warp->value = (float) warp_index(pm->warpon);
                V.placement->value = (float) pm->place;
                V.rot->value = pm->rot;
                V.scale->value = pm->scale;
                V.shx->value = pm->shx;
                V.shy->value = pm->shy;
            }
            V.lastcontent = V.content->value; V.lastwarp = V.warp->value; V.lastplace = V.placement->value;
            V.lastrot = V.rot->value; V.lastscale = V.scale->value; V.lastshx = V.shx->value; V.lastshy = V.shy->value;
        }
        if (pm) {
            const float rr = std::round(V.rot->value), ss = std::round(V.scale->value * 100.0f) / 100.0f;
            const float xx = std::round(V.shx->value * 100.0f) / 100.0f, yy = std::round(V.shy->value * 100.0f) / 100.0f;
            bool changed = false;
            if (V.content->value != V.lastcontent) { for (MapMask *t : tgts) t->content = content_ui((int) (V.content->value + 0.5f)); V.lastcontent = V.content->value; changed = true; }
            if (V.warp->value != V.lastwarp) {
                const int wi = std::clamp((int) (V.warp->value + 0.5f), 0, (int) V.warpmodes.size() - 1);
                for (MapMask *t : tgts) {
                    t->warpon = V.warpmodes[wi];
                    if (t->warpon == 4) init_cam(*t);
                }
                if (pm->warpon == 4) V.viewing = true;
                V.warp->value = (float) wi;
                V.lastwarp = V.warp->value;
                changed = true;
            }
            if (V.placement->value != V.lastplace) { for (MapMask *t : tgts) t->place = (int) (V.placement->value + 0.5f); V.lastplace = V.placement->value; changed = true; }
            if (rr != V.lastrot) { for (MapMask *t : tgts) t->rot = rr; V.lastrot = rr; changed = true; }
            if (ss != V.lastscale) { for (MapMask *t : tgts) t->scale = ss; V.lastscale = ss; changed = true; }
            if (xx != V.lastshx) { for (MapMask *t : tgts) t->shx = xx; V.lastshx = xx; changed = true; }
            if (yy != V.lastshy) { for (MapMask *t : tgts) t->shy = yy; V.lastshy = yy; changed = true; }
            if (changed) mark_changed(true);
        }
    }
    g_knotbudget = (int) (V.knots->value + 0.5f);
    {
        int kv = g_knotbudget;
        if (kv != V.lastknots) {
            V.lastknots = kv;
            MapMask *fm = bm ? find_mask(bm, V.sel) : nullptr;
            if (V.edit && fm && !fm->ctrl) {
                if (V.fitsrc.empty()) push_undo();
                refit_selected();
            }
        }
    }
    V.color->layer = any_layer();

    // ---- input on the canvas ----
    bool down = mainprogram->leftmousedown;
    bool pressed = down && !V.prevdown;
    bool released = !down && V.prevdown;
    bool canvasact = nomenu && !typing && !mainprogram->selectingparcol;
    MapMask *selm = bm ? find_mask(bm, V.sel) : nullptr;
    if (!selm) { V.sel = -1; V.ksel.clear(); }
    if (bm) for (MapMask &m : bm->masks) ensure_flat(m, *bm);

    std::vector<float> ein, eout;
    if (selm) resolve(selm->k, ein, eout);

    if (vwing && selm) {
        // (the camera of the outline: the projection on it follows live)
        if (viewport_input(*selm, inimg && !inpanel && nomenu && !typing)) {
            g_viewver++;
            g_dirty = true;
        }
    }
    if (bm && pressed && inimg && !inpanel && canvasact && !vwing) {
        V.pressnx = V.lastnx = mnx; V.pressny = V.lastny = mny;
        V.pressknot = -1;
        V.pressshift = false;
        V.pressmoved = false;
        V.knotpush = false;
        V.pressmx = mainprogram->mx; V.pressmy = mainprogram->my;
        bool modkey = mainprogram->shift || mainprogram->ctrl;
        V.mode = View::M_NONE;
        if (!V.edit) {
            V.mode = View::M_CLICK;
        }
        else {
            bool done = false;
            if (selm) {
                int n = (int) selm->k.size();
                // handles of the selected knots
                for (int i : V.ksel) {
                    if (i >= n || done) continue;
                    int t = selm->k[i].type;
                    if (t != KT_FLUID && t != KT_BROKEN) continue;
                    float kx, ky, hx2, hy2;
                    uv2n(selm->k[i].x, selm->k[i].y, kx, ky);
                    uv2n(selm->k[i].x + eout[i * 2], selm->k[i].y + eout[i * 2 + 1], hx2, hy2);
                    if (sdist(mnx, mny, hx2, hy2) < 9.0f) { V.mode = View::M_HANDLE; V.hk = i; V.hwhich = 1; done = true; break; }
                    if (t == KT_BROKEN) {
                        uv2n(selm->k[i].x + ein[i * 2], selm->k[i].y + ein[i * 2 + 1], hx2, hy2);
                        if (sdist(mnx, mny, hx2, hy2) < 9.0f) { V.mode = View::M_HANDLE; V.hk = i; V.hwhich = 0; done = true; break; }
                    }
                }
                if (done) push_undo();
                // knots
                if (!done) {
                    int bk = -1;
                    float bd = 11.0f;
                    for (int i = 0; i < n; i++) {
                        float kx, ky;
                        uv2n(selm->k[i].x, selm->k[i].y, kx, ky);
                        float dd = sdist(mnx, mny, kx, ky);
                        if (dd < bd) { bd = dd; bk = i; }
                    }
                    if (bk >= 0) {
                        done = true;
                        if (modkey && !V.ksel.count(bk)) {
                            V.ksel.insert(bk);   // (SHIFT on a knot that is not selected: it joins the selection)
                            V.mode = View::M_NONE;
                        }
                        else {
                            // A knot that is selected already: dragging it moves the whole selection, but a click without a drag
                            // (see the release below) makes it the only selected knot, or, with SHIFT, takes it out of the selection.
                            V.pressknot = V.ksel.count(bk) ? bk : -1;
                            V.pressshift = modkey;
                            if (!V.ksel.count(bk)) { V.ksel.clear(); V.ksel.insert(bk); }
                            push_undo();
                            V.knotpush = true;
                            V.mode = View::M_KNOT;
                        }
                    }
                }
                // a click on the curve adds a knot
                if (!done) {
                    int np = (int) selm->poly.size() / 2;
                    float bd = 7.0f, bt = 0.0f;
                    int bs = -1;
                    for (int i = 0; i < np; i++) {
                        int j = (i + 1) % np;
                        float ax, ay, bx2, by2;
                        uv2n(selm->poly[i * 2], selm->poly[i * 2 + 1], ax, ay);
                        uv2n(selm->poly[j * 2], selm->poly[j * 2 + 1], bx2, by2);
                        float dx = (bx2 - ax) * (float) glob->w * 0.5f, dy = (by2 - ay) * (float) glob->h * 0.5f;
                        float l2 = dx * dx + dy * dy;
                        float px = (mnx - ax) * (float) glob->w * 0.5f, py = (mny - ay) * (float) glob->h * 0.5f;
                        float f = l2 > 1e-6f ? std::clamp((px * dx + py * dy) / l2, 0.0f, 1.0f) : 0.0f;
                        float ex = px - f * dx, ey = py - f * dy;
                        float dd = std::sqrt(ex * ex + ey * ey);
                        if (dd < bd) {
                            bd = dd;
                            bs = selm->pseg[i];
                            float t0 = selm->pt[i];
                            float t1 = (j != 0 && selm->pseg[j] == selm->pseg[i]) ? selm->pt[j] : 1.0f;
                            bt = t0 + f * (t1 - t0);
                        }
                    }
                    if (bs >= 0) {
                        push_undo();
                        int ni = insert_knot(selm->k, bs, std::clamp(bt, 0.02f, 0.98f));
                        touch_mask(*selm);
                        V.ksel.clear();
                        V.ksel.insert(ni);
                        V.mode = View::M_KNOT;
                        V.pressknot = -1;
                        V.knotpush = false;   // (the knot was added: that undo step stays)
                        done = true;
                    }
                }
            }
            if (!done) {
                V.mode = View::M_PENDING;
                V.pendingmod = modkey;
            }
        }
    }
    // dragging
    if (bm && down && V.edit && selm) {
        float dnu = (mnx - V.lastnx) / (iw * vz), dnv = -(mny - V.lastny) / (ih * vz);
        if (V.mode == View::M_KNOT) {
            // (nothing moves until the mouse has travelled a few pixels: a click is not a drag)
            if (!V.pressmoved) {
                if (std::hypot((float) mainprogram->mx - (float) V.pressmx, (float) mainprogram->my - (float) V.pressmy) > 4.0f) {
                    V.pressmoved = true;
                    dnu = (mnx - V.pressnx) / (iw * vz);
                    dnv = -(mny - V.pressny) / (ih * vz);
                }
                else dnu = dnv = 0.0f;
            }
            if (V.pressmoved) {
                for (int i : V.ksel) {
                    if (i >= (int) selm->k.size()) continue;
                    selm->k[i].x += dnu;
                    selm->k[i].y += dnv;
                }
                touch_mask(*selm);
            }
        }
        else if (V.mode == View::M_HANDLE && V.hk >= 0 && V.hk < (int) selm->k.size()) {
            MapKnot &c = selm->k[V.hk];
            float hxn = mu - c.x, hyn = mv - c.y;
            if (c.type == KT_FLUID) {
                if (V.hwhich == 1) { c.hout[0] = hxn; c.hout[1] = hyn; }
                else { c.hout[0] = -hxn; c.hout[1] = -hyn; }
            }
            else {
                if (V.hwhich == 1) { c.hout[0] = hxn; c.hout[1] = hyn; }
                else { c.hin[0] = hxn; c.hin[1] = hyn; }
            }
            touch_mask(*selm);
        }
        else if (V.mode == View::M_PENDING) {
            if (std::hypot((float) mainprogram->mx - (float) V.pressmx, (float) mainprogram->my - (float) V.pressmy) > 4.0f) V.mode = View::M_BOX;
        }
    }
    else if (bm && down && V.edit && !selm && V.mode == View::M_PENDING) {
        if (std::hypot((float) mainprogram->mx - (float) V.pressmx, (float) mainprogram->my - (float) V.pressmy) > 4.0f) V.mode = View::M_NONE;
    }
    V.lastnx = mnx; V.lastny = mny;

    if (bm && released) {
        if (V.mode == View::M_KNOT && !V.pressmoved) {
            // a click on a knot, not a drag
            if (V.pressknot >= 0) {
                if (V.pressshift) V.ksel.erase(V.pressknot);
                else { V.ksel.clear(); V.ksel.insert(V.pressknot); }
            }
            if (V.knotpush && !V.undo.empty()) V.undo.pop_back();   // (nothing changed: no undo step)
        }
        bool moved = std::hypot((float) mainprogram->mx - (float) V.pressmx, (float) mainprogram->my - (float) V.pressmy) > 4.0f;
        if (V.mode == View::M_CLICK && !V.edit && !moved && inimg && !inpanel) {
            // (a click selects the outline under the pointer; again for the next one under it; on nothing: no selection)
            std::vector<int> under = masks_under(bm, mu, mv);
            int pick = cycle_pick(bm, under, mainprogram->mx, mainprogram->my);
            V.sel = pick >= 0 ? bm->masks[pick].id : -1;
            V.ksel.clear();
            V.fitsrc.clear();
        }
        else if (V.mode == View::M_PENDING && V.edit && !moved) {
            std::vector<int> under = masks_under(bm, mu, mv);
            int pick = cycle_pick(bm, under, mainprogram->mx, mainprogram->my);
            if (pick >= 0) {
                V.sel = bm->masks[pick].id;
                V.ksel.clear();
                V.fitsrc.clear();
            }
            else if (!V.pendingmod) {
                V.sel = -1;
                V.ksel.clear();
            }
        }
        else if (V.mode == View::M_BOX && selm) {
            float x0 = std::min(V.pressnx, mnx), x1 = std::max(V.pressnx, mnx), y0 = std::min(V.pressny, mny), y1 = std::max(V.pressny, mny);
            if (!V.pendingmod) V.ksel.clear();
            for (int i = 0; i < (int) selm->k.size(); i++) {
                float kx, ky;
                uv2n(selm->k[i].x, selm->k[i].y, kx, ky);
                if (kx >= x0 && kx <= x1 && ky >= y0 && ky <= y1) V.ksel.insert(i);
            }
        }
        V.mode = View::M_NONE;
    }
    V.prevdown = down;

    // DEL removes the selected knots
    if (V.edit && selm && mainprogram->del && !typing && !V.ksel.empty() && nomenu && !vwing) {
        mainprogram->del = false;
        int remaining = (int) selm->k.size() - (int) V.ksel.size();
        if (remaining >= 3) {
            push_undo();
            std::vector<MapKnot> nk;
            for (int i = 0; i < (int) selm->k.size(); i++) {
                if (V.ksel.count(i)) {
                    continue;
                }
                nk.push_back(selm->k[i]);
            }
            // (neighbours of removed knots with automatic handles follow along; fixed handles keep what they have)
            selm->k = nk;
            V.ksel.clear();
            touch_mask(*selm);
        }
    }

    // The arrow keys nudge the selected knots by one beamer pixel (SHIFT: ten); held down they repeat after a short wait
    if (V.edit && selm && !V.ksel.empty() && !typing && nomenu && bm && !vwing) {
        const bool *ks = SDL_GetKeyboardState(nullptr);
        const int ndx = (ks[SDL_SCANCODE_RIGHT] ? 1 : 0) - (ks[SDL_SCANCODE_LEFT] ? 1 : 0);
        const int ndy = (ks[SDL_SCANCODE_DOWN] ? 1 : 0) - (ks[SDL_SCANCODE_UP] ? 1 : 0);   // (y is down)
        const auto tnow = std::chrono::steady_clock::now();
        if (ndx != 0 || ndy != 0) {
            bool fire = false;
            if (!V.nudgeheld) {
                V.nudgeheld = true;
                V.nudget = tnow + std::chrono::milliseconds(400);
                push_undo();
                fire = true;
            }
            else if (tnow >= V.nudget) {
                V.nudget = tnow + std::chrono::milliseconds(40);
                fire = true;
            }
            if (fire) {
                const float step = mainprogram->shift ? 10.0f : 1.0f;
                for (int i : V.ksel) {
                    if (i >= (int) selm->k.size()) continue;
                    selm->k[i].x += (float) ndx * step / (float) bm->w;
                    selm->k[i].y += (float) ndy * step / (float) bm->h;
                }
                touch_mask(*selm);
            }
        }
        else V.nudgeheld = false;
    }
    else V.nudgeheld = false;

    // ---- drawing ----
    draw_box(grey, black, cvx, cvy, cvw, cvh, -1);
    {
        Out *o = find_out(V.curkey);
        if (vwing) {
            // the point cloud as seen from the viewpoint, and the beamer image small in the corner (the projection, live)
            draw_box(grey, black, irx, iry, iw, ih, -1);
            if (selm) draw_cloud(*selm, bm->scene, irx, iry, iw, ih);
            if (o && o->show != (GLuint) -1) {
                const float tw = iw * 0.28f, th = ih * 0.28f, tx = irx + iw - tw - 0.01f, ty = iry + 0.01f;
                draw_box(white, black, tx - 0.003f, ty - 0.004f, tw + 0.006f, th + 0.008f, -1);
                draw_direct(nullptr, g_tr, tx, ty, tw, th, 0.0f, 0.0f, 1.0f, 1.0f, 0, o->show, glob->w, glob->h, false, false);
            }
        }
        else if (o && o->show != (GLuint) -1) {
            // (the beamer image as it is now: the projection / the exposure color)
            draw_direct(nullptr, g_tr, irx, iry, iw, ih, -(V.zu - 0.5f) / 6.0f, -(V.zv - 0.5f) / 6.0f, 1.0f / vz, 1.0f, 0, o->show, glob->w, glob->h, false, false);
        }
        else {
            draw_box(grey, darkgrey, irx, iry, iw, ih, -1);
        }
    }
    g_mm_clip[0] = irx; g_mm_clip[1] = iry; g_mm_clip[2] = irx + iw; g_mm_clip[3] = iry + ih;
    auto nline = [&](float *col, float x1, float y1, float x2, float y2) {
        float dx = x2 - x1, dy = y2 - y1;
        float p[4] = {-dx, dx, -dy, dy};
        float q[4] = {x1 - g_mm_clip[0], g_mm_clip[2] - x1, y1 - g_mm_clip[1], g_mm_clip[3] - y1};
        float t0 = 0.0f, t1 = 1.0f;
        for (int i = 0; i < 4; i++) {
            if (p[i] == 0.0f) { if (q[i] < 0.0f) return; }
            else {
                float t = q[i] / p[i];
                if (p[i] < 0.0f) { if (t > t1) return; if (t > t0) t0 = t; }
                else { if (t < t0) return; if (t < t1) t1 = t; }
            }
        }
        register_line_draw(col, x1 + t0 * dx, y1 + t0 * dy, x1 + t1 * dx, y1 + t1 * dy, true);
    };
    if (bm && vwing && selm) {
        char hb[160];
        snprintf(hb, sizeof(hb), "Viewpoint: az=%.1f  el=%.1f  dist=%.2f   pivot (%.2f, %.2f, %.2f)", selm->cam.azimuthDeg, selm->cam.elevationDeg,
                 selm->cam.distance, selm->cam.pivot[0], selm->cam.pivot[1], selm->cam.pivot[2]);
        render_text(hb, white, irx + 0.01f, iry + ih - 0.09f, 0.0005f, 0.0009f);
        render_text("ALT+left drag: orbit    ALT+middle drag: pan    wheel: closer / further    CTRL+wheel: pivot depth", white, irx + 0.01f,
                    iry + ih - 0.13f, 0.0004f, 0.0008f);
    }
    if (bm && !vwing) {
        for (MapMask &m : bm->masks) {
            bool assigned = is_assigned(V.curkey, m.id, V.el);
            float *col = (m.id == V.sel) ? g_wh : (assigned ? g_or : g_lb);
            int np = (int) m.poly.size() / 2;
            for (int i = 0; i < np; i++) {
                int j = (i + 1) % np;
                float ax, ay, bx2, by2;
                uv2n(m.poly[i * 2], m.poly[i * 2 + 1], ax, ay);
                uv2n(m.poly[j * 2], m.poly[j * 2 + 1], bx2, by2);
                nline(col, ax, ay, bx2, by2);
                if (m.id == V.sel && assigned) nline(g_or, ax + 0.002f, ay, bx2 + 0.002f, by2);
            }
        }
        if (V.edit && selm) {
            std::vector<float> ein2, eout2;
            resolve(selm->k, ein2, eout2);
            for (int i = 0; i < (int) selm->k.size(); i++) {
                const MapKnot &c = selm->k[i];
                float kx, ky;
                uv2n(c.x, c.y, kx, ky);
                bool inside = kx >= irx && kx <= irx + iw && ky >= iry && ky <= iry + ih;
                bool s = V.ksel.count(i) > 0;
                if (s && (c.type == KT_FLUID || c.type == KT_BROKEN)) {
                    float hx2, hy2;
                    // (the tip of a handle is only drawn where it is in the view: zoomed in it can be outside it)
                    auto tipin = [&](float x, float y) { return x >= irx && x <= irx + iw && y >= iry && y <= iry + ih; };
                    uv2n(c.x + eout2[i * 2], c.y + eout2[i * 2 + 1], hx2, hy2);
                    nline(g_ye, kx, ky, hx2, hy2);
                    if (tipin(hx2, hy2)) draw_box(g_ye, hx2, hy2, 0.005f, 1);
                    if (c.type == KT_BROKEN) {
                        uv2n(c.x + ein2[i * 2], c.y + ein2[i * 2 + 1], hx2, hy2);
                        nline(g_ye, kx, ky, hx2, hy2);
                        if (tipin(hx2, hy2)) draw_box(g_ye, hx2, hy2, 0.005f, 1);
                    }
                }
                if (!inside) continue;
                float *kc = s ? g_or : (c.type == KT_SMOOTH ? g_lb : c.type == KT_CONST ? g_ye : c.type == KT_FLUID ? g_gr : g_pk);
                draw_box(kc, kx, ky, s ? 0.009f : 0.007f, 1);
            }
            // the box select
            if (V.mode == View::M_BOX) {
                nline(g_wh, V.pressnx, V.pressny, mnx, V.pressny);
                nline(g_wh, mnx, V.pressny, mnx, mny);
                nline(g_wh, mnx, mny, V.pressnx, mny);
                nline(g_wh, V.pressnx, mny, V.pressnx, V.pressny);
            }
        }
    }
    // text on the canvas
    render_text("Mapping: " + elem_label(V.el), white, cvx + 0.01f, cvy + cvh - 0.045f, 0.0006f, 0.001f);

    // ---- panel ----
    // opening the Beamer dropdown scans the displays again (a beamer may have been plugged in since)
    if (mainprogram->leftmouse && nomenu && V.beam->box->in()) sync_beam_options();
    V.beam->handle();
    V.name->handle();
    V.color->handle();
    if (V.edit && haveSel) V.knots->handle();   // (the Knots slider and the knot type buttons are for editing outlines)
    if (!tgts.empty()) {
        V.feather->handle();
        V.inset->handle();   // (the Inset and the placement of the frame belong to the selected outline, or the outlines assigned to the element)
        V.content->handle();
        bool anywarp = false;
        for (MapMask *t : tgts) if (t->warp) anywarp = true;
        if (anywarp) V.warp->handle();   // (only outlines made in the layer mapping view have a surface)
        V.placement->handle();
        V.rot->handle();
        V.scale->handle();
        V.shx->handle();
        V.shy->handle();
    }
    {
        // buttons
        auto btn = [&](Boxx *b, const char *txt, bool on, float tx) {
            draw_box(b->lcolor, on ? green : darkgrey, b, -1);
            render_text(txt, white, b->vtxcoords->x1 + tx, b->vtxcoords->y1 + 0.022f, 0.00042f, 0.0009f);
        };
        btn(V.editbox, "EDIT", V.edit, 0.04f);
        {
            const bool asg = selm && is_assigned(V.curkey, selm->id, V.el);
            draw_box(V.assignbox->lcolor, asg ? green : (selm ? darkgreen1 : darkgrey), V.assignbox, -1);
            render_text("ASSIGN", white, V.assignbox->vtxcoords->x1 + 0.03f, V.assignbox->vtxcoords->y1 + 0.022f, 0.00042f, 0.0009f);
        }
        btn(V.exposebox, "EXPOSE", V.expose, 0.02f);
        btn(V.openbox, "OPEN", false, 0.03f);
        const bool canmake = !V.curkey.empty();
        draw_box(V.newbox->lcolor, canmake ? darkgreen1 : darkgrey, V.newbox, -1);
        render_text("NEW MASK", white, V.newbox->vtxcoords->x1 + 0.012f, V.newbox->vtxcoords->y1 + 0.022f, 0.00042f, 0.0009f);
        draw_box(V.delbox->lcolor, selm ? darkred1 : darkgrey, V.delbox, -1);
        render_text("DELETE MASK", white, V.delbox->vtxcoords->x1 + 0.006f, V.delbox->vtxcoords->y1 + 0.022f, 0.00042f, 0.0009f);
        int showtype = V.curtype;
        if (V.edit && selm && !V.ksel.empty()) {
            int i = *V.ksel.begin();
            if (i < (int) selm->k.size()) showtype = selm->k[i].type;
        }
        const char *tn[4] = {"SMOOTH", "CONSTANT", "FLUID", "BROKEN"};
        if (V.edit && selm) for (int i = 0; i < 4; i++) btn(V.tbox[i], tn[i], showtype == i, 0.012f);
        const bool canview = selm && selm->warpon == 4 && view_possible(*selm) && bm && bm->scene;
        if (selm) {
            draw_box(V.viewbox->lcolor, canview ? (V.viewing ? green : darkgreen1) : darkgrey, V.viewbox, -1);
            render_text("VIEWPOINT", white, V.viewbox->vtxcoords->x1 + 0.012f, V.viewbox->vtxcoords->y1 + 0.022f, 0.00042f, 0.0009f);
            draw_box(V.resetbox->lcolor, canview ? darkgreen1 : darkgrey, V.resetbox, -1);
            render_text("RESET VIEW", white, V.resetbox->vtxcoords->x1 + 0.006f, V.resetbox->vtxcoords->y1 + 0.022f, 0.00042f, 0.0009f);
        }
        draw_box(V.donebox->lcolor, darkgreen1, V.donebox, -1);
        render_text("DONE", white, V.donebox->vtxcoords->x1 + 0.03f, V.donebox->vtxcoords->y1 + 0.022f, 0.00042f, 0.0009f);
        draw_box(V.cancelbox->lcolor, V.cancelbox->acolor, V.cancelbox, -1);
        render_text("CANCEL", white, V.cancelbox->vtxcoords->x1 + 0.02f, V.cancelbox->vtxcoords->y1 + 0.022f, 0.00042f, 0.0009f);

        if (mainprogram->leftmouse && nomenu && !typing) {
            if (V.editbox->in()) {
                // (a control curve is edited by its knots: Edit outlines cannot be switched off while one is selected)
                if (!(V.edit && selm && selm->ctrl)) {
                    V.edit = !V.edit;
                    V.ksel.clear();
                    V.fitsrc.clear();
                    V.mode = View::M_NONE;
                }
                mainprogram->leftmouse = false;
            }
            else if (V.assignbox->in()) {
                mainprogram->leftmouse = false;
                if (selm) set_assigned(V.curkey, selm->id, V.el, !is_assigned(V.curkey, selm->id, V.el));
            }
            else if (selm && V.viewbox->in()) {
                mainprogram->leftmouse = false;
                if (canview) V.viewing = !V.viewing;
            }
            else if (selm && V.resetbox->in()) {
                mainprogram->leftmouse = false;
                if (canview) {
                    push_undo();
                    selm->camset = false;
                    init_cam(*selm);
                    g_viewver++;
                    g_dirty = true;
                }
            }
            else if (V.newbox->in()) {
                mainprogram->leftmouse = false;
                if (canmake) {
                    BeamerMaps *nb = find_maps(V.curkey, true);   // (may move the others: nothing of this frame's pointers is used after it)
                    const Disp *dsp = disp_for_key(V.curkey);
                    if (nb->masks.empty() && dsp) { nb->w = dsp->w; nb->h = dsp->h; }
                    push_undo();
                    // a square (in pixels) in the middle of the beamer image: four Constant knots on its corners
                    const float side = 0.5f * (float) std::min(nb->w, nb->h);
                    const float hxn = 0.5f * side / (float) nb->w, hyn = 0.5f * side / (float) nb->h;
                    MapMask m;
                    m.id = nb->nextid++;
                    m.ctrl = true;
                    const float cxs[4] = {0.5f - hxn, 0.5f + hxn, 0.5f + hxn, 0.5f - hxn}, cys[4] = {0.5f - hyn, 0.5f - hyn, 0.5f + hyn, 0.5f + hyn};
                    for (int i = 0; i < 4; i++) {
                        MapKnot c;
                        c.x = cxs[i]; c.y = cys[i];
                        c.type = KT_CONST;
                        m.k.push_back(c);
                    }
                    nb->masks.push_back(m);
                    V.edit = true;                  // (it is a control curve: its knots are what is edited)
                    V.sel = m.id;
                    V.ksel.clear();
                    V.fitsrc.clear();
                    V.mode = View::M_NONE;
                    mark_changed(true);
                }
            }
            else if (V.delbox->in()) {
                mainprogram->leftmouse = false;
                if (selm) {
                    const int delid = selm->id;
                    push_undo();
                    for (size_t i = 0; i < g_assign.size();) {
                        if (g_assign[i].key == V.curkey && g_assign[i].maskid == delid) g_assign.erase(g_assign.begin() + i);
                        else i++;
                    }
                    for (size_t i = 0; i < bm->masks.size(); i++) {
                        if (bm->masks[i].id == delid) { bm->masks.erase(bm->masks.begin() + i); break; }
                    }
                    V.sel = -1;
                    V.ksel.clear();
                    V.fitsrc.clear();
                    V.mode = View::M_NONE;
                    mark_changed(true);
                }
            }
            else if (V.exposebox->in()) {
                V.expose = !V.expose;
                mainprogram->leftmouse = false;
            }
            else if (V.openbox->in()) {
                mainprogram->pathto = "OPENMAPPING";
                std::string dir = maps_dir();
                try { fs::create_directories(dir); } catch (...) {}
                std::thread filereq(&Program::get_inname, mainprogram, "Open mapping", "", dir);
                filereq.detach();
                mainprogram->leftmouse = false;
            }
            else if (V.donebox->in()) {
                mainprogram->leftmouse = false;
                mainprogram->directmode = budm;
                maskmap_leave();
                return;
            }
            else if (V.cancelbox->in()) {
                mainprogram->leftmouse = false;
                mainprogram->directmode = budm;
                maskmap_cancel();
                return;
            }
            else {
                for (int i = 0; i < 4 && V.edit && selm; i++) {
                    if (!V.tbox[i]->in()) continue;
                    mainprogram->leftmouse = false;
                    V.curtype = i;
                    if (V.edit && selm && !V.ksel.empty()) {
                        push_undo();
                        for (int idx : V.ksel) if (idx < (int) selm->k.size()) set_knot_type(*selm, idx, i);
                        touch_mask(*selm);
                    }
                }
            }
        }
    }

    if (incanvas || inpanel) mainprogram->recundo = false;
    mainprogram->directmode = budm;
}
