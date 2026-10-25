#ifndef USE_GLES
#include "GL/glew.h"
#include "GL/gl.h"
#define FREEGLUT_STATIC
#define FREEGLUT_LIB_PRAGMAS 0
#endif

#include <algorithm>
#include <cmath>
#include <set>
#include <map>

// my own header
#include "program.h"
#include "LayerWarp.h"

// ======================================================================================================
// DATA / MATH
// ======================================================================================================

// set whenever the warp (or a layer's shift/scale in warp mode) is changed: the warp undo stack is fed from it
static bool g_warpdirty = false;

// the warp clipboard (copy/paste network between layers): lives for the whole program run
static LayerWarpData g_wclip;
static bool g_wclipvalid = false;
static float g_wclipxf[3] = {0.0f, 0.0f, 1.0f};   // shiftx, shifty and scale go with the network

LayerWarpData::LayerWarpData() {
    init_identity(1, 1);
}

LayerWarpData::LayerWarpData(const LayerWarpData &o) {
    copy_data_from(o);
}

LayerWarpData &LayerWarpData::operator=(const LayerWarpData &o) {
    if (this != &o) copy_data_from(o);
    return *this;
}

LayerWarpData::~LayerWarpData() {
    // GL resources are released explicitly through release_gl() (the GL context may be gone by now)
}

void LayerWarpData::copy_data_from(const LayerWarpData &o) {
    nx = o.nx;
    ny = o.ny;
    knots = o.knots;
    active = o.active;
    dirty = true;
    resolved = false;
}

void LayerWarpData::init_identity(int newnx, int newny) {
    nx = std::clamp(newnx, 1, MAXSTEPS);
    ny = std::clamp(newny, 1, MAXSTEPS);
    knots.assign((nx + 1) * (ny + 1), WarpKnot());
    for (int j = 0; j <= ny; j++) {
        for (int i = 0; i <= nx; i++) {
            WarpKnot &k = K(i, j);
            k.x = -1.0f + 2.0f * (float) i / (float) nx;
            k.y = -1.0f + 2.0f * (float) j / (float) ny;
        }
    }
    active = false;
    dirty = true;
    resolved = false;
}

void LayerWarpData::reset() {
    init_identity(1, 1);
}

bool LayerWarpData::is_identity() const {
    for (int j = 0; j <= ny; j++) {
        for (int i = 0; i <= nx; i++) {
            const WarpKnot &k = K(i, j);
            if (k.typeX != WK_SMOOTH || k.typeY != WK_SMOOTH) return false;
            if (std::fabs(k.x - (-1.0f + 2.0f * (float) i / (float) nx)) > 1e-4f) return false;
            if (std::fabs(k.y - (-1.0f + 2.0f * (float) j / (float) ny)) > 1e-4f) return false;
        }
    }
    return true;
}

void LayerWarpData::touch() {
    g_warpdirty = true;
    resolved = false;
    dirty = true;
    active = !is_identity();
}

// compute the effective handles of all knots from their types
void LayerWarpData::resolve() const {
    if (resolved) return;
    res = knots;
    for (int j = 0; j <= ny; j++) {
        for (int i = 0; i <= nx; i++) {
            const WarpKnot &k = knots[j * (nx + 1) + i];
            WarpKnot &r = res[j * (nx + 1) + i];
            // horizontal direction
            if (k.typeX == WK_SMOOTH) {
                float tl[2] = {0.0f, 0.0f}, tr[2] = {0.0f, 0.0f};
                bool hp = i > 0, hn = i < nx;
                const WarpKnot *kp = hp ? &knots[j * (nx + 1) + i - 1] : nullptr;
                const WarpKnot *kn = hn ? &knots[j * (nx + 1) + i + 1] : nullptr;
                if (hp && hn) {
                    float tx = (kn->x - kp->x) / 6.0f, ty = (kn->y - kp->y) / 6.0f;
                    tr[0] = tx; tr[1] = ty; tl[0] = -tx; tl[1] = -ty;
                } else if (hn) {
                    tr[0] = (kn->x - k.x) / 3.0f; tr[1] = (kn->y - k.y) / 3.0f;
                } else if (hp) {
                    tl[0] = (kp->x - k.x) / 3.0f; tl[1] = (kp->y - k.y) / 3.0f;
                }
                r.hxl[0] = tl[0]; r.hxl[1] = tl[1]; r.hxr[0] = tr[0]; r.hxr[1] = tr[1];
            } else if (k.typeX == WK_CONSTANT) {
                r.hxl[0] = r.hxl[1] = r.hxr[0] = r.hxr[1] = 0.0f;
            } else if (k.typeX == WK_FLUID) {
                r.hxl[0] = -k.hxr[0]; r.hxl[1] = -k.hxr[1];
            }
            // vertical direction
            if (k.typeY == WK_SMOOTH) {
                float td[2] = {0.0f, 0.0f}, tu[2] = {0.0f, 0.0f};
                bool hp = j > 0, hn = j < ny;
                const WarpKnot *kp = hp ? &knots[(j - 1) * (nx + 1) + i] : nullptr;
                const WarpKnot *kn = hn ? &knots[(j + 1) * (nx + 1) + i] : nullptr;
                if (hp && hn) {
                    float tx = (kn->x - kp->x) / 6.0f, ty = (kn->y - kp->y) / 6.0f;
                    tu[0] = tx; tu[1] = ty; td[0] = -tx; td[1] = -ty;
                } else if (hn) {
                    tu[0] = (kn->x - k.x) / 3.0f; tu[1] = (kn->y - k.y) / 3.0f;
                } else if (hp) {
                    td[0] = (kp->x - k.x) / 3.0f; td[1] = (kp->y - k.y) / 3.0f;
                }
                r.hyd[0] = td[0]; r.hyd[1] = td[1]; r.hyu[0] = tu[0]; r.hyu[1] = tu[1];
            } else if (k.typeY == WK_CONSTANT) {
                r.hyd[0] = r.hyd[1] = r.hyu[0] = r.hyu[1] = 0.0f;
            } else if (k.typeY == WK_FLUID) {
                r.hyd[0] = -k.hyu[0]; r.hyd[1] = -k.hyu[1];
            }
        }
    }
    resolved = true;
}

void LayerWarpData::set_knot_type(int i, int j, bool dirx, int type) {
    resolve();
    WarpKnot &k = K(i, j);
    const WarpKnot &r = R(i, j);
    if (dirx) {
        if (k.typeX == type) return;
        // keep the current shape: take over the effective handles
        k.hxl[0] = r.hxl[0]; k.hxl[1] = r.hxl[1];
        k.hxr[0] = r.hxr[0]; k.hxr[1] = r.hxr[1];
        if (type == WK_FLUID) {
            if (i == nx) { k.hxr[0] = -k.hxl[0]; k.hxr[1] = -k.hxl[1]; }
        } else if (type == WK_CONSTANT) {
            k.hxl[0] = k.hxl[1] = k.hxr[0] = k.hxr[1] = 0.0f;
        }
        k.typeX = type;
    } else {
        if (k.typeY == type) return;
        k.hyd[0] = r.hyd[0]; k.hyd[1] = r.hyd[1];
        k.hyu[0] = r.hyu[0]; k.hyu[1] = r.hyu[1];
        if (type == WK_FLUID) {
            if (j == ny) { k.hyu[0] = -k.hyd[0]; k.hyu[1] = -k.hyd[1]; }
        } else if (type == WK_CONSTANT) {
            k.hyd[0] = k.hyd[1] = k.hyu[0] = k.hyu[1] = 0.0f;
        }
        k.typeY = type;
    }
    touch();
}

static void bezier(const float p0[2], const float p1[2], const float p2[2], const float p3[2], float u, float out[2]) {
    float a = 1.0f - u;
    float b0 = a * a * a, b1 = 3.0f * a * a * u, b2 = 3.0f * a * u * u, b3 = u * u * u;
    out[0] = b0 * p0[0] + b1 * p1[0] + b2 * p2[0] + b3 * p3[0];
    out[1] = b0 * p0[1] + b1 * p1[1] + b2 * p2[1] + b3 * p3[1];
}

void LayerWarpData::row_seg(int i, int j, float u, float out[2]) const {
    resolve();
    const WarpKnot &a = R(i, j), &b = R(i + 1, j);
    float p0[2] = {a.x, a.y};
    float p1[2] = {a.x + a.hxr[0], a.y + a.hxr[1]};
    float p2[2] = {b.x + b.hxl[0], b.y + b.hxl[1]};
    float p3[2] = {b.x, b.y};
    bezier(p0, p1, p2, p3, u, out);
}

void LayerWarpData::col_seg(int i, int j, float v, float out[2]) const {
    resolve();
    const WarpKnot &a = R(i, j), &b = R(i, j + 1);
    float p0[2] = {a.x, a.y};
    float p1[2] = {a.x + a.hyu[0], a.y + a.hyu[1]};
    float p2[2] = {b.x + b.hyd[0], b.y + b.hyd[1]};
    float p3[2] = {b.x, b.y};
    bezier(p0, p1, p2, p3, v, out);
}

void LayerWarpData::eval_cell(int i, int j, float u, float v, float out[2]) const {
    float b0[2], b1[2], c0[2], c1[2];
    row_seg(i, j, u, b0);
    row_seg(i, j + 1, u, b1);
    col_seg(i, j, v, c0);
    col_seg(i + 1, j, v, c1);
    const WarpKnot &k00 = R(i, j), &k10 = R(i + 1, j), &k01 = R(i, j + 1), &k11 = R(i + 1, j + 1);
    for (int c = 0; c < 2; c++) {
        float k00c = c == 0 ? k00.x : k00.y;
        float k10c = c == 0 ? k10.x : k10.y;
        float k01c = c == 0 ? k01.x : k01.y;
        float k11c = c == 0 ? k11.x : k11.y;
        out[c] = (1.0f - v) * b0[c] + v * b1[c] + (1.0f - u) * c0[c] + u * c1[c]
                 - ((1.0f - u) * (1.0f - v) * k00c + u * (1.0f - v) * k10c + (1.0f - u) * v * k01c + u * v * k11c);
    }
}

void LayerWarpData::eval(float s, float t, float out[2]) const {
    s = std::clamp(s, 0.0f, 1.0f);
    t = std::clamp(t, 0.0f, 1.0f);
    int i = std::min((int) (s * nx), nx - 1);
    int j = std::min((int) (t * ny), ny - 1);
    float u = s * nx - i;
    float v = t * ny - j;
    eval_cell(i, j, u, v, out);
}

bool LayerWarpData::handle_available(int i, int j, int which) const {
    switch (which) {
        case 0: return i > 0;
        case 1: return i < nx;
        case 2: return j > 0;
        case 3: return j < ny;
    }
    return false;
}

bool LayerWarpData::handle_pos(int i, int j, int which, float out[2]) const {
    if (!handle_available(i, j, which)) return false;
    resolve();
    const WarpKnot &r = R(i, j);
    const float *h = which == 0 ? r.hxl : which == 1 ? r.hxr : which == 2 ? r.hyd : r.hyu;
    out[0] = r.x + h[0];
    out[1] = r.y + h[1];
    return true;
}

void LayerWarpData::set_steps(int newnx, int newny) {
    newnx = std::clamp(newnx, 1, MAXSTEPS);
    newny = std::clamp(newny, 1, MAXSTEPS);
    if (newnx == nx && newny == ny) return;
    std::vector<WarpKnot> nk((newnx + 1) * (newny + 1));
    for (int j = 0; j <= newny; j++) {
        for (int i = 0; i <= newnx; i++) {
            float p[2];
            eval((float) i / (float) newnx, (float) j / (float) newny, p);
            WarpKnot &k = nk[j * (newnx + 1) + i];
            k.x = p[0];
            k.y = p[1];
        }
    }
    nx = newnx;
    ny = newny;
    knots = nk;
    touch();
}

void LayerWarpData::build_mesh(float sc, float ox, float oy) {
    resolve();
    tsc = sc;
    tox = ox;
    toy = oy;
    int T = std::clamp(128 / std::max(nx, ny), 3, 12);
    int gw = nx * T + 1;
    int gh = ny * T + 1;
    verts.resize((size_t) gw * gh * 4);
    float *vp = verts.data();
    for (int b = 0; b < gh; b++) {
        int j = std::min(b / T, ny - 1);
        float v = (float) (b - j * T) / (float) T;
        float t = (float) b / (float) (ny * T);
        for (int a = 0; a < gw; a++) {
            int i = std::min(a / T, nx - 1);
            float u = (float) (a - i * T) / (float) T;
            float s = (float) a / (float) (nx * T);
            float p[2];
            eval_cell(i, j, u, v, p);
            // world (y up on screen) -> layer fbo space (y flipped), source texture is stored flipped as well
            *vp++ = sc * p[0] + ox;
            *vp++ = -(sc * p[1] + oy);
            *vp++ = s;
            *vp++ = 1.0f - t;
        }
    }
    idx.clear();
    idx.reserve((size_t) (gw - 1) * (gh - 1) * 6);
    for (int b = 0; b < gh - 1; b++) {
        for (int a = 0; a < gw - 1; a++) {
            unsigned int i0 = b * gw + a, i1 = i0 + 1, i2 = i0 + gw, i3 = i2 + 1;
            idx.push_back(i0); idx.push_back(i1); idx.push_back(i2);
            idx.push_back(i1); idx.push_back(i3); idx.push_back(i2);
        }
    }
    nidx = (int) idx.size();

    if (vao == 0) {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ibo);
    }
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned int), idx.data(), GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 16, (void *) 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 16, (void *) 8);
    glBindVertexArray(0);
    dirty = false;
}

void LayerWarpData::release_gl() {
    if (vao) { glDeleteVertexArrays(1, &vao); vao = 0; }
    if (vbo) { glDeleteBuffers(1, &vbo); vbo = 0; }
    if (ibo) { glDeleteBuffers(1, &ibo); ibo = 0; }
    if (fbo != (unsigned int) -1) { glDeleteFramebuffers(1, &fbo); fbo = (unsigned int) -1; }
    if (tex != (unsigned int) -1) {
        if (mainprogram) {
            mainprogram->texsizemap.erase(tex);
            mainprogram->texintfmap.erase(tex);
        }
        glDeleteTextures(1, &tex);
        tex = (unsigned int) -1;
    }
    texw = texh = 0;
    dirty = true;
}

void LayerWarpData::write(std::ostream &w) const {
    if (!active) return;
    w << "WARPSTEPS\n";
    w << std::to_string(nx) << "\n";
    w << std::to_string(ny) << "\n";
    w << "WARPKNOTS\n";
    for (const WarpKnot &k : knots) {
        w << std::to_string(k.x) << "\n";
        w << std::to_string(k.y) << "\n";
        w << std::to_string(k.typeX) << "\n";
        w << std::to_string(k.typeY) << "\n";
        w << std::to_string(k.hxl[0]) << "\n";
        w << std::to_string(k.hxl[1]) << "\n";
        w << std::to_string(k.hxr[0]) << "\n";
        w << std::to_string(k.hxr[1]) << "\n";
        w << std::to_string(k.hyd[0]) << "\n";
        w << std::to_string(k.hyd[1]) << "\n";
        w << std::to_string(k.hyu[0]) << "\n";
        w << std::to_string(k.hyu[1]) << "\n";
    }
}

void LayerWarpData::read_steps(std::istream &r) {
    std::string istring;
    safegetline(r, istring);
    int newnx = std::stoi(istring);
    safegetline(r, istring);
    int newny = std::stoi(istring);
    init_identity(newnx, newny);
}

void LayerWarpData::read_knots(std::istream &r) {
    std::string istring;
    for (WarpKnot &k : knots) {
        float *dst[12] = {&k.x, &k.y, nullptr, nullptr, &k.hxl[0], &k.hxl[1], &k.hxr[0], &k.hxr[1],
                          &k.hyd[0], &k.hyd[1], &k.hyu[0], &k.hyu[1]};
        for (int n = 0; n < 12; n++) {
            safegetline(r, istring);
            if (n == 2) k.typeX = std::stoi(istring);
            else if (n == 3) k.typeY = std::stoi(istring);
            else *dst[n] = std::stof(istring);
        }
    }
    touch();
}

// ======================================================================================================
// RENDER PASS
// ======================================================================================================

// The shift/scale of a layer as a transform of warp space (the layer, aspect corrected, filling the frame at
// [-1,1]) to world space.  Derived from the texture coordinate mapping of the layer's last pass (see onestepfrom()):
// shift 'sx' moves the content by 24 * sx * xs, scale scales it about the frame centre.
WarpXf layerwarp_xf(Layer *lay) {
    WarpXf xf;
    bool effectspresent = false;
    for (auto eff : lay->effects[0]) {
        if (eff->onoffbutton->value) {
            effectspresent = true;
            break;
        }
    }
    bool isf = lay->isfsourcenr != -1;
    float xs = (isf || lay->xss <= 0.001f) ? 1.0f : lay->xss;
    float ys = (isf || lay->yss <= 0.001f) ? 1.0f : lay->yss;
    float sy = lay->shifty->value;
    if (isf && !effectspresent) sy = -sy;
    xf.sc = lay->scale->value;
    xf.ox = 24.0f * lay->shiftx->value * xs;
    xf.oy = -24.0f * sy * ys;   // (world y is up on screen, the shift is in texture space)
    return xf;
}

bool layerwarp_pass(Layer *lay, unsigned int src, bool stage, unsigned int &outtex, unsigned int &outfbo) {
    LayerWarpData &w = lay->warp;
    if (!w.active) return false;
    if (src == (unsigned int) -1) return false;

    int sw, sh;
    gl_get_tex_size(src, &sw, &sh);
    if (sw <= 0 || sh <= 0) {
        sw = mainprogram->ow[stage];
        sh = mainprogram->oh[stage];
    }
    if (w.tex == (unsigned int) -1 || w.fbo == (unsigned int) -1 || w.texw != sw || w.texh != sh) {
        if (w.fbo != (unsigned int) -1) { glDeleteFramebuffers(1, &w.fbo); w.fbo = (unsigned int) -1; }
        if (w.tex != (unsigned int) -1) {
            mainprogram->texsizemap.erase(w.tex);
            mainprogram->texintfmap.erase(w.tex);
            glDeleteTextures(1, &w.tex);
            w.tex = (unsigned int) -1;
        }
        glGenTextures(1, &w.tex);
        glBindTexture(GL_TEXTURE_2D, w.tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER_COMPAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER_COMPAT);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, sw, sh);
        mainprogram->texintfmap[w.tex] = GL_RGBA8;
        mainprogram->texsizemap[w.tex] = {sw, sh};
        glGenFramebuffers(1, &w.fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, w.fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, w.tex, 0);
        w.texw = sw;
        w.texh = sh;
    }
    WarpXf xf = layerwarp_xf(lay);
    if (w.dirty || w.vao == 0 || xf.sc != w.tsc || xf.ox != w.tox || xf.oy != w.toy) {
        w.build_mesh(xf.sc, xf.ox, xf.oy);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, w.fbo);
    glDrawBuffer_FBO();
    glViewport(0, 0, sw, sh);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    mainprogram->uniformCache->setInt("interm", 4);
    mainprogram->uniformCache->setBool("box", false);
    mainprogram->uniformCache->setBool("down", true);
    mainprogram->uniformCache->setBool("usemask", false);
    mainprogram->uniformCache->setBool("laymasked", false);
    mainprogram->uniformCache->setInt("ismask", 0);
    mainprogram->uniformCache->setFloat("opacity", 1.0f);
    mainprogram->uniformCache->setFloat("pixelw", 0.0f);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, src);
    GLboolean wasblend = glIsEnabled(GL_BLEND);
    glDisable(GL_BLEND);
    glBindVertexArray(w.vao);
    glDrawElements(GL_TRIANGLES, w.nidx, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    if (wasblend) glEnable(GL_BLEND);

    mainprogram->uniformCache->setBool("down", false);
    mainprogram->uniformCache->setInt("interm", 0);
    glViewport(0, 0, glob->w, glob->h);

    outtex = w.tex;
    outfbo = w.fbo;
    return true;
}

// ======================================================================================================
// UI
// ======================================================================================================

namespace {

struct WarpSnap {
    Layer *lay;
    LayerWarpData data;
    float shx, shy, sc;
};

enum WarpMode {
    WM_NONE = 0,
    WM_KNOT,
    WM_TOGGLE,   // shift/ctrl click on a knot: nothing to drag
    WM_HANDLE,
    WM_LINE,
    WM_EMPTY,
    WM_LAYER,
    WM_BOX
};

struct WarpUI {
    bool inited = false;
    Param *gridx = nullptr, *gridy = nullptr, *xsteps = nullptr, *ysteps = nullptr, *ktx = nullptr, *kty = nullptr;
    Boxx *cancelbox = nullptr;
    Boxx *copybox = nullptr;
    Boxx *pastebox = nullptr;
    Param *scale = nullptr;                // stand-in for Layer::scale (its own range is far too wide for a slider)
    float lastscale = 1.0f;
    float zoom = 1.0f;
    std::vector<Layer *> order;            // back ... front (front = current layer)
    std::vector<WarpSnap> snaps;
    std::set<int> sel;
    Layer *sellayer = nullptr;
    int lastktx = -1, lastkty = -1;
    int lastxs = -1, lastys = -1;          // steps as last synced to the params (-1 = not synced yet)

    // geometry of the canvas (NDC)
    float bx = 0, by = 0, bw = 1, bh = 1;
    float cx = 0, cy = 0, hx = 1, hy = 1;  // centre and half extents of the mix area

    // interaction
    WarpMode mode = WM_NONE;
    bool prevdown = false;
    float startpx = 0, startpy = 0;        // pixels at press
    float grabx = 0, graby = 0;            // grab offset (world): anchor - mouse
    float anchorx = 0, anchory = 0;        // current anchor position (world)
    float lastwx = 0, lastwy = 0;          // mouse world at last frame
    int hknot = -1, hwhich = 0;
    int linedir = 0, lineidx = 0;
    std::vector<int> dragknots;
    bool boxadd = false;
    bool knotwassel = false;               // the knot pressed was selected already
    bool knotmoved = false;                // the press turned into a drag
    float pressw[2] = {0, 0};              // mouse (world) at press
    float boxx0 = 0, boxy0 = 0;            // world
    bool snapxon = false, snapyon = false;
    float snapx = 0, snapy = 0;
};

WarpUI wui;

float g_lightblue[4] = {0.45f, 0.75f, 1.0f, 1.0f};
float g_orange[4] = {1.0f, 0.55f, 0.0f, 1.0f};
float g_netcol[4] = {0.75f, 0.75f, 0.75f, 0.9f};
float g_bgcol[4] = {0.8f, 0.8f, 0.8f, 1.0f};
float g_gridcol[4] = {0.22f, 0.22f, 0.22f, 1.0f};
float g_transp[4] = {0.0f, 0.0f, 0.0f, 0.0f};
float g_whitebox[4] = {1.0f, 1.0f, 1.0f, 1.0f};
float g_snapcol[4] = {1.0f, 1.0f, 0.2f, 1.0f};

void w2n(float wx, float wy, float &nx, float &ny) {
    nx = wui.cx + wx * wui.hx;
    ny = wui.cy + wy * wui.hy;
}

void n2w(float nx, float ny, float &wx, float &wy) {
    wx = (nx - wui.cx) / wui.hx;
    wy = (ny - wui.cy) / wui.hy;
}

void w2s(float wx, float wy, float &px, float &py) {
    float nx, ny;
    w2n(wx, wy, nx, ny);
    px = (nx + 1.0f) * 0.5f * glob->w;
    py = (1.0f - ny) * 0.5f * glob->h;
}

void s2w(float px, float py, float &wx, float &wy) {
    float nx = px / (glob->w * 0.5f) - 1.0f;
    float ny = 1.0f - py / (glob->h * 0.5f);
    n2w(nx, ny, wx, wy);
}

// Liang-Barsky clip of an NDC line against the canvas
bool clip_line(float &x1, float &y1, float &x2, float &y2) {
    float xmin = wui.bx, xmax = wui.bx + wui.bw, ymin = wui.by, ymax = wui.by + wui.bh;
    float dx = x2 - x1, dy = y2 - y1;
    float p[4] = {-dx, dx, -dy, dy};
    float q[4] = {x1 - xmin, xmax - x1, y1 - ymin, ymax - y1};
    float t0 = 0.0f, t1 = 1.0f;
    for (int i = 0; i < 4; i++) {
        if (p[i] == 0.0f) {
            if (q[i] < 0.0f) return false;
        } else {
            float t = q[i] / p[i];
            if (p[i] < 0.0f) {
                if (t > t1) return false;
                if (t > t0) t0 = t;
            } else {
                if (t < t0) return false;
                if (t < t1) t1 = t;
            }
        }
    }
    float nx1 = x1 + t0 * dx, ny1 = y1 + t0 * dy, nx2 = x1 + t1 * dx, ny2 = y1 + t1 * dy;
    x1 = nx1; y1 = ny1; x2 = nx2; y2 = ny2;
    return true;
}

void wline(float *col, float wx1, float wy1, float wx2, float wy2, bool direct = false) {
    float x1, y1, x2, y2;
    w2n(wx1, wy1, x1, y1);
    w2n(wx2, wy2, x2, y2);
    if (!clip_line(x1, y1, x2, y2)) return;
    register_line_draw(col, x1, y1, x2, y2, direct);
}

bool in_canvas(float nx, float ny) {
    return nx >= wui.bx && nx <= wui.bx + wui.bw && ny >= wui.by && ny <= wui.by + wui.bh;
}

Layer *cur_layer() {
    return mainmix->currlay[!mainprogram->prevmodus];
}

std::vector<Layer *> &sel_layers() {
    return mainmix->currlays[!mainprogram->prevmodus];
}

void mark_changed(Layer *lay) {
    g_warpdirty = true;
    if (lay->type == ELEM_IMAGE || lay->type == ELEM_LIVE) {
        std::lock_guard<std::mutex> lock(lay->decresult_mutex);
        if (lay->decresult) lay->decresult->newdata = true;
    }
    lay->set_clones();
}

void init_ui() {
    if (wui.inited) return;
    wui.inited = true;

    auto mk = [](const char *name, float deflt, float lo, float hi) {
        Param *p = new Param;
        p->name = name;
        p->value = deflt;
        p->deflt = deflt;
        p->range[0] = lo;
        p->range[1] = hi;
        p->sliding = false;
        p->box->vtxcoords->w = 0.156f;
        p->box->vtxcoords->h = 0.07f;
        p->box->upvtxtoscr();
        return p;
    };
    wui.gridx = mk("Grid X", 8.0f, 1.0f, 64.0f);
    wui.gridy = mk("Grid Y", 8.0f, 1.0f, 64.0f);
    wui.xsteps = mk("X Steps", 1.0f, 1.0f, (float) LayerWarpData::MAXSTEPS);
    wui.ysteps = mk("Y Steps", 1.0f, 1.0f, (float) LayerWarpData::MAXSTEPS);
    wui.gridx->box->tooltiptitle = "Grid X ";
    wui.gridx->box->tooltip = "Number of divisions of the main grid in X direction. ";
    wui.gridy->box->tooltiptitle = "Grid Y ";
    wui.gridy->box->tooltip = "Number of divisions of the main grid in Y direction. ";
    wui.xsteps->box->tooltiptitle = "X Steps ";
    wui.xsteps->box->tooltip = "Number of divisions of the current layer's warp network in X direction. ";
    wui.ysteps->box->tooltiptitle = "Y Steps ";
    wui.ysteps->box->tooltip = "Number of divisions of the current layer's warp network in Y direction. ";

    auto mkopt = [&](const char *name) {
        Param *p = mk(name, 0.0f, 0.0f, 3.0f);
        p->type = FF_TYPE_OPTION;
        p->options = {"Smooth ", "Constant", "Fluid", "Broken"};
        return p;
    };
    wui.ktx = mkopt("Knot type X");
    wui.kty = mkopt("Knot type Y");
    wui.ktx->box->tooltiptitle = "Knot type X ";
    wui.ktx->box->tooltip = "Type of the selected knot(s) in the horizontal direction: Smooth (no handles), Constant, Fluid (mirrored handles) or Broken (independent handles). ";
    wui.kty->box->tooltiptitle = "Knot type Y ";
    wui.kty->box->tooltip = "Type of the selected knot(s) in the vertical direction: Smooth (no handles), Constant, Fluid (mirrored handles) or Broken (independent handles). ";

    wui.scale = mk("Scale", 1.0f, 0.05f, 4.0f);
    wui.scale->sliding = true;
    wui.scale->box->tooltiptitle = "Scale ";
    wui.scale->box->tooltip = "Scale of the current layer. ";

    wui.cancelbox = new Boxx;
    wui.cancelbox->lcolor[0] = 0.7f; wui.cancelbox->lcolor[1] = 0.7f; wui.cancelbox->lcolor[2] = 0.7f;
    wui.cancelbox->acolor[0] = 0.5f; wui.cancelbox->acolor[1] = 0.1f; wui.cancelbox->acolor[2] = 0.1f;
    wui.cancelbox->acolor[3] = 1.0f;
    wui.cancelbox->vtxcoords->w = 0.156f;
    wui.cancelbox->vtxcoords->h = 0.07f;
    wui.cancelbox->tooltiptitle = "Cancel layer warp ";
    wui.cancelbox->tooltip = "Reverts all layer warp changes (warp, shift and scale) made since layer warp mode was entered, and leaves layer warp mode. ";
    wui.cancelbox->upvtxtoscr();

    auto mkbox = [](const char *title, const char *tip) {
        Boxx *b = new Boxx;
        b->lcolor[0] = 0.7f; b->lcolor[1] = 0.7f; b->lcolor[2] = 0.7f;
        b->acolor[0] = 0.15f; b->acolor[1] = 0.3f; b->acolor[2] = 0.15f; b->acolor[3] = 1.0f;
        b->vtxcoords->w = 0.156f;
        b->vtxcoords->h = 0.07f;
        b->tooltiptitle = title;
        b->tooltip = tip;
        b->upvtxtoscr();
        return b;
    };
    wui.copybox = mkbox("Copy network ", "Copies the current layer's entire warp network, with its shift and scale, to the warp clipboard (kept until the program closes, also outside warp mode). ");
    wui.pastebox = mkbox("Paste network ", "Replaces the current layer's warp network, shift and scale by those in the warp clipboard. Works between layers. ");
}

void snapshot_layer(Layer *lay) {
    for (auto &s : wui.snaps) if (s.lay == lay) return;
    WarpSnap s;
    s.lay = lay;
    s.data.copy_data_from(lay->warp);
    s.shx = lay->shiftx->value;
    s.shy = lay->shifty->value;
    s.sc = lay->scale->value;
    wui.snaps.push_back(s);
}

// keep order/snapshots in sync with the selection
void sync_layers() {
    std::vector<Layer *> &cl = sel_layers();
    Layer *cur = cur_layer();
    std::vector<Layer *> neworder;
    for (Layer *l : wui.order) {
        if (l != cur && std::find(cl.begin(), cl.end(), l) != cl.end()) neworder.push_back(l);
    }
    for (Layer *l : cl) {
        if (l != cur && std::find(neworder.begin(), neworder.end(), l) == neworder.end()) neworder.push_back(l);
    }
    if (cur) neworder.push_back(cur);
    wui.order = neworder;
    for (Layer *l : wui.order) snapshot_layer(l);
    if (wui.sellayer != cur) {
        wui.sellayer = cur;
        wui.sel.clear();
        wui.mode = WM_NONE;
        wui.lastxs = wui.lastys = -1;
        wui.lastktx = wui.lastkty = -1;
    }
}

// outline info of a (non-current) layer used for accentuation and snapping
struct Outline {
    bool warped = false;
    float c[4][2];     // corners: bl, br, tr, tl (world)
    float xmin, xmax, ymin, ymax;   // unwarped rect
};

Outline layer_outline(Layer *l) {
    Outline o;
    if (l->warp.active) {
        o.warped = true;
        l->warp.resolve();
        WarpXf xf = layerwarp_xf(l);
        const WarpKnot *ks[4] = {&l->warp.K(0, 0), &l->warp.K(l->warp.nx, 0),
                                 &l->warp.K(l->warp.nx, l->warp.ny), &l->warp.K(0, l->warp.ny)};
        for (int c = 0; c < 4; c++) {
            o.c[c][0] = xf.sc * ks[c]->x + xf.ox;
            o.c[c][1] = xf.sc * ks[c]->y + xf.oy;
        }
        o.xmin = std::min(o.c[0][0], o.c[3][0]); o.xmax = std::max(o.c[1][0], o.c[2][0]);
        o.ymin = std::min(o.c[0][1], o.c[1][1]); o.ymax = std::max(o.c[2][1], o.c[3][1]);
    } else {
        float xs = l->xss > 0.001f ? l->xss : 1.0f;
        float ys = l->yss > 0.001f ? l->yss : 1.0f;
        float sc = l->scale->value;
        float ccx = 24.0f * l->shiftx->value * xs;
        float ccy = -24.0f * l->shifty->value * ys;
        o.xmin = ccx - xs * sc; o.xmax = ccx + xs * sc;
        o.ymin = ccy - ys * sc; o.ymax = ccy + ys * sc;
        o.c[0][0] = o.xmin; o.c[0][1] = o.ymin;
        o.c[1][0] = o.xmax; o.c[1][1] = o.ymin;
        o.c[2][0] = o.xmax; o.c[2][1] = o.ymax;
        o.c[3][0] = o.xmin; o.c[3][1] = o.ymax;
    }
    return o;
}

// snap a world position (alt held). Candidates: main grid, mix area edges, other layers' corners and outer edges
void snap_pos(float &wx, float &wy) {
    wui.snapxon = wui.snapyon = false;
    float pxperw_x = wui.hx * glob->w * 0.5f;
    float pxperw_y = wui.hy * glob->h * 0.5f;
    float thx = 9.0f / pxperw_x, thy = 9.0f / pxperw_y;

    float bestdx = thx, bestdy = thy;
    float sx = wx, sy = wy;
    bool hx_ = false, hy_ = false;
    auto trysx = [&](float cx_) { float d = std::fabs(cx_ - wx); if (d < bestdx) { bestdx = d; sx = cx_; hx_ = true; } };
    auto trysy = [&](float cy_) { float d = std::fabs(cy_ - wy); if (d < bestdy) { bestdy = d; sy = cy_; hy_ = true; } };

    // main grid
    int gxn = std::max(1, (int) (wui.gridx->value + 0.5f));
    int gyn = std::max(1, (int) (wui.gridy->value + 0.5f));
    float stepx = 2.0f / gxn, stepy = 2.0f / gyn;
    trysx(std::round((wx + 1.0f) / stepx) * stepx - 1.0f);
    trysy(std::round((wy + 1.0f) / stepy) * stepy - 1.0f);
    // mix area edges
    trysx(-1.0f); trysx(1.0f); trysy(-1.0f); trysy(1.0f);

    // other layers
    for (Layer *l : wui.order) {
        if (l == wui.sellayer) continue;
        Outline o = layer_outline(l);
        if (!o.warped) {
            trysx(o.xmin); trysx(o.xmax); trysy(o.ymin); trysy(o.ymax);
        }
        for (int c = 0; c < 4; c++) {
            // corner points snap both axes together
            float dx = std::fabs(o.c[c][0] - wx), dy = std::fabs(o.c[c][1] - wy);
            if (dx < thx && dy < thy) {
                trysx(o.c[c][0]);
                trysy(o.c[c][1]);
            }
        }
    }
    if (hx_) { wx = sx; wui.snapxon = true; wui.snapx = sx; }
    if (hy_) { wy = sy; wui.snapyon = true; wui.snapy = sy; }
}

// distance (pixels) from a point to a world-space segment
float seg_dist_px(float px, float py, float wx1, float wy1, float wx2, float wy2) {
    float x1, y1, x2, y2;
    w2s(wx1, wy1, x1, y1);
    w2s(wx2, wy2, x2, y2);
    float dx = x2 - x1, dy = y2 - y1;
    float l2 = dx * dx + dy * dy;
    float t = l2 > 0.0f ? ((px - x1) * dx + (py - y1) * dy) / l2 : 0.0f;
    t = std::clamp(t, 0.0f, 1.0f);
    float qx = x1 + t * dx, qy = y1 + t * dy;
    return std::sqrt((px - qx) * (px - qx) + (py - qy) * (py - qy));
}

int samples_per_seg(const LayerWarpData &w) {
    return std::clamp(64 / std::max(w.nx, w.ny), 3, 16);
}

void compute_geometry() {
    // (below the layer stack scroll strip)
    float top = mainprogram->scrollboxes[0]->vtxcoords->y1 - 0.012f;
    float bottom = mainmix->crossfade->box->vtxcoords->y1 + mainmix->crossfade->box->vtxcoords->h + 0.012f;
    float h = top - bottom;
    float aspect = 16.0f / 9.0f * (float) glob->h / (float) glob->w;
    float w = h * aspect;
    if (w > 1.9f) {
        w = 1.9f;
        h = w / aspect;
    }
    wui.bw = w;
    wui.bh = h;
    wui.bx = -w * 0.5f;
    wui.by = bottom + (top - bottom - h) * 0.5f;
    wui.cx = wui.bx + w * 0.5f;
    wui.cy = wui.by + h * 0.5f;
    wui.hx = w * 0.5f * wui.zoom;
    wui.hy = h * 0.5f * wui.zoom;
}

void apply_snap_if(float &wx, float &wy) {
    if (mainprogram->alt) snap_pos(wx, wy);
    else wui.snapxon = wui.snapyon = false;
}

// ---- warp undo: a separate stack for warp edits (warp data, shift and scale of the layers in the warp session) ----
struct WarpUndoEntry {
    Layer *lay;
    LayerWarpData data;
    float shx, shy, sc;
};
typedef std::vector<WarpUndoEntry> WarpUndoState;
std::vector<WarpUndoState> g_wundo;
int g_wundopos = 0;   // g_wundo[g_wundopos - 1] is the current state

bool layer_alive(Layer *l) {
    for (auto &sm : mainmix->layers) {
        if (std::find(sm.begin(), sm.end(), l) != sm.end()) return true;
    }
    return false;
}

WarpUndoState undo_capture() {
    std::vector<Layer *> lays = wui.order;
    for (auto &s : wui.snaps) {
        if (std::find(lays.begin(), lays.end(), s.lay) == lays.end()) lays.push_back(s.lay);
    }
    WarpUndoState st;
    for (Layer *l : lays) {
        WarpUndoEntry e;
        e.lay = l;
        e.data.copy_data_from(l->warp);
        e.shx = l->shiftx->value;
        e.shy = l->shifty->value;
        e.sc = l->scale->value;
        st.push_back(e);
    }
    return st;
}

void undo_reset() {
    g_wundo.clear();
    g_wundo.push_back(undo_capture());
    g_wundopos = 1;
    g_warpdirty = false;
}

void undo_push() {
    g_wundo.resize(g_wundopos);
    g_wundo.push_back(undo_capture());
    if (g_wundo.size() > 200) g_wundo.erase(g_wundo.begin());
    g_wundopos = (int) g_wundo.size();
    g_warpdirty = false;
}

void undo_restore(const WarpUndoState &st) {
    for (auto &e : st) {
        if (!layer_alive(e.lay)) continue;
        e.lay->warp.copy_data_from(e.data);
        e.lay->warp.touch();
        e.lay->shiftx->value = e.shx;
        e.lay->shifty->value = e.shy;
        e.lay->scale->value = e.sc;
        mark_changed(e.lay);
    }
    wui.sel.clear();
    wui.mode = WM_NONE;
    g_warpdirty = false;
}

}  // namespace

void layerwarp_undo() {
    if (!mainprogram->layerwarp || wui.mode != WM_NONE) return;
    if (g_wundopos > 1) {
        g_wundopos--;
        undo_restore(g_wundo[g_wundopos - 1]);
    }
}

void layerwarp_redo() {
    if (!mainprogram->layerwarp || wui.mode != WM_NONE) return;
    if (g_wundopos < (int) g_wundo.size()) {
        g_wundopos++;
        undo_restore(g_wundo[g_wundopos - 1]);
    }
}

bool layerwarp_isparam(Param *p) {
    if (!wui.inited || !p) return false;
    return p == wui.gridx || p == wui.gridy || p == wui.xsteps || p == wui.ysteps || p == wui.ktx || p == wui.kty ||
           p == wui.scale;
}

void layerwarp_enter() {
    init_ui();
    wui.zoom = 1.0f;
    wui.snaps.clear();
    wui.order.clear();
    wui.sel.clear();
    wui.sellayer = nullptr;
    wui.mode = WM_NONE;
    wui.prevdown = false;
    std::vector<Layer *> &cl = sel_layers();
    if (cl.empty() && cur_layer()) cl.push_back(cur_layer());
    wui.order = cl;
    sync_layers();
    undo_reset();
}

void layerwarp_leave() {
    mainprogram->layerwarp = false;
    mainprogram->layerwarpbut->value = 0;
    mainprogram->layerwarpbut->oldvalue = 0;
    wui.mode = WM_NONE;
    wui.sel.clear();
    wui.snaps.clear();
    wui.order.clear();
    wui.sellayer = nullptr;
    g_wundo.clear();
    g_wundopos = 0;
}

void layerwarp_cancel() {
    for (auto &s : wui.snaps) {
        s.lay->warp.copy_data_from(s.data);
        s.lay->warp.touch();
        s.lay->shiftx->value = s.shx;
        s.lay->shifty->value = s.shy;
        s.lay->scale->value = s.sc;
        mark_changed(s.lay);
    }
    layerwarp_leave();
}

void layerwarp_ui() {
    if (!mainprogram->layerwarp) return;
    init_ui();
    Layer *cur = cur_layer();
    if (!cur) return;
    sync_layers();
    compute_geometry();
    LayerWarpData &W = cur->warp;

    // the network lives in layer (warp) space; the layer's shift and scale place it in the world
    WarpXf X = layerwarp_xf(cur);
    auto L2W = [&](float lx, float ly, float &wx, float &wy) { wx = X.sc * lx + X.ox; wy = X.sc * ly + X.oy; };
    auto W2L = [&](float wx, float wy, float &lx, float &ly) { lx = (wx - X.ox) / X.sc; ly = (wy - X.oy) / X.sc; };

    // mouse
    float mnx = mainprogram->xscrtovtx((float) mainprogram->mx) - 1.0f;
    float mny = 1.0f - mainprogram->yscrtovtx((float) mainprogram->my);
    float mpx = (float) mainprogram->mx, mpy = (float) mainprogram->my;
    float mwx, mwy;
    n2w(mnx, mny, mwx, mwy);

    // control panel geometry
    float panelw = 0.156f, rowh = 0.078f;
    float px0 = wui.bx - panelw - 0.01f;   // (outside the left edge of the canvas)
    float ptop = wui.cy + rowh * 5.0f;   // (the 10 rows centred on the left edge)
    Param *plist[7] = {wui.gridx, wui.gridy, wui.xsteps, wui.ysteps, wui.ktx, wui.kty, wui.scale};
    for (int n = 0; n < 7; n++) {
        Boxx *b = plist[n]->box;
        b->vtxcoords->x1 = px0;
        b->vtxcoords->y1 = ptop - rowh * (n + 1) + 0.004f;
        b->vtxcoords->w = panelw;
        b->vtxcoords->h = 0.07f;
        b->upvtxtoscr();
    }
    wui.cancelbox->vtxcoords->x1 = px0;
    wui.cancelbox->vtxcoords->y1 = ptop - rowh * 10 + 0.004f;
    wui.cancelbox->vtxcoords->w = panelw;
    wui.cancelbox->vtxcoords->h = 0.07f;
    wui.cancelbox->upvtxtoscr();
    Boxx *cpboxes[2] = {wui.copybox, wui.pastebox};
    for (int n = 0; n < 2; n++) {
        cpboxes[n]->vtxcoords->x1 = px0;
        cpboxes[n]->vtxcoords->y1 = ptop - rowh * (8 + n) + 0.004f;
        cpboxes[n]->vtxcoords->w = panelw;
        cpboxes[n]->vtxcoords->h = 0.07f;
        cpboxes[n]->upvtxtoscr();
    }
    float panel_x0 = px0 - 0.005f, panel_x1 = px0 + panelw + 0.005f, panel_y0 = ptop - rowh * 10 - 0.005f;
    bool inpanel = mnx >= panel_x0 && mnx <= panel_x1 && mny >= panel_y0 && mny <= wui.by + wui.bh;
    bool incanvas = in_canvas(mnx, mny);
    // knots on the canvas edge stick out of it: they stay clickable a little outside
    float mgx = 12.0f * 2.0f / glob->w, mgy = 12.0f * 2.0f / glob->h;
    bool inext = mnx >= wui.bx - mgx && mnx <= wui.bx + wui.bw + mgx && mny >= wui.by - mgy && mny <= wui.by + wui.bh + mgy;

    // sync param values with the data (unless being edited)
    auto editing = [](Param *p) {
        return mainmix->adaptparam == p || mainmix->prepadaptparam == p || mainmix->adaptnumparam == p;
    };
    // (values changed by the user since the last sync are applied first, then the params follow the data)
    if (!editing(wui.xsteps) && !editing(wui.ysteps) && wui.lastxs != -1) {
        int nsx = (int) (wui.xsteps->value + 0.5f), nsy = (int) (wui.ysteps->value + 0.5f);
        if (nsx != wui.lastxs || nsy != wui.lastys) {
            W.set_steps(nsx, nsy);
            wui.sel.clear();
        }
    }
    if (wui.lastktx != -1 && (int) wui.ktx->value != wui.lastktx) {
        for (int k : wui.sel) W.set_knot_type(k % (W.nx + 1), k / (W.nx + 1), true, (int) wui.ktx->value);
    }
    if (wui.lastkty != -1 && (int) wui.kty->value != wui.lastkty) {
        for (int k : wui.sel) W.set_knot_type(k % (W.nx + 1), k / (W.nx + 1), false, (int) wui.kty->value);
    }
    if (!editing(wui.xsteps)) wui.xsteps->value = (float) W.nx;
    if (!editing(wui.ysteps)) wui.ysteps->value = (float) W.ny;
    if (!editing(wui.xsteps) && !editing(wui.ysteps)) {
        wui.lastxs = W.nx;
        wui.lastys = W.ny;
    }
    if (!editing(wui.scale)) {
        wui.scale->value = std::clamp(cur->scale->value, wui.scale->range[0], wui.scale->range[1]);
        wui.lastscale = wui.scale->value;
    }
    if (wui.sel.empty()) {
        wui.ktx->value = 0.0f;
        wui.kty->value = 0.0f;
    } else {
        const WarpKnot &fk = W.knots[*wui.sel.begin()];
        wui.ktx->value = (float) fk.typeX;
        wui.kty->value = (float) fk.typeY;
    }
    wui.lastktx = (int) wui.ktx->value;
    wui.lastkty = (int) wui.kty->value;

    // ---- input ----
    bool down = mainprogram->leftmousedown;
    bool pressed = down && !wui.prevdown;
    bool nomenu = !mainprogram->menuondisplay;
    float pxw = wui.hx * glob->w * 0.5f;   // pixels per world unit (x)

    if (incanvas && !inpanel && mainprogram->mousewheel != 0.0f && nomenu) {
        wui.zoom = std::clamp(wui.zoom * (1.0f + 0.1f * mainprogram->mousewheel), 0.2f, 1.0f);
        mainprogram->mousewheel = 0.0f;
        compute_geometry();
        n2w(mnx, mny, mwx, mwy);
    }

    if (wui.mode == WM_NONE && pressed && inext && !inpanel && nomenu) {
        wui.startpx = mpx;
        wui.startpy = mpy;
        wui.lastwx = mwx;
        wui.lastwy = mwy;
        // (select modifier: shift or ctrl; ctrl is cmd on macOS)
        bool modkey = mainprogram->shift || mainprogram->ctrl;
        W.resolve();
        bool hit = false;

        // 1. handles of selected knots
        for (int k : wui.sel) {
            int ki = k % (W.nx + 1), kj = k / (W.nx + 1);
            for (int hw = 0; hw < 4 && !hit; hw++) {
                bool dirx = hw < 2;
                int type = dirx ? W.K(ki, kj).typeX : W.K(ki, kj).typeY;
                if (type != WK_FLUID && type != WK_BROKEN) continue;
                float hp[2];
                if (!W.handle_pos(ki, kj, hw, hp)) continue;
                float sx_, sy_, hwx, hwy;
                L2W(hp[0], hp[1], hwx, hwy);
                w2s(hwx, hwy, sx_, sy_);
                if (std::hypot(sx_ - mpx, sy_ - mpy) < 9.0f) {
                    wui.mode = WM_HANDLE;
                    wui.hknot = k;
                    wui.hwhich = hw;
                    hit = true;
                }
            }
            if (hit) break;
        }
        // 2. knots
        if (!hit) {
            int best = -1;
            float bestd = 11.0f;
            for (int j = 0; j <= W.ny; j++) {
                for (int i = 0; i <= W.nx; i++) {
                    const WarpKnot &kn = W.K(i, j);
                    float sx_, sy_, kwx, kwy;
                    L2W(kn.x, kn.y, kwx, kwy);
                    w2s(kwx, kwy, sx_, sy_);
                    float d = std::hypot(sx_ - mpx, sy_ - mpy);
                    if (d < bestd) { bestd = d; best = j * (W.nx + 1) + i; }
                }
            }
            if (best != -1) {
                hit = true;
                wui.knotwassel = wui.sel.count(best) > 0;
                wui.knotmoved = false;
                wui.hknot = best;
                wui.pressw[0] = mwx;
                wui.pressw[1] = mwy;
                if (modkey) {
                    if (wui.sel.count(best)) wui.sel.erase(best);
                    else wui.sel.insert(best);
                    wui.mode = WM_TOGGLE;
                } else {
                    if (!wui.sel.count(best)) {
                        wui.sel.clear();
                        wui.sel.insert(best);
                    }
                    wui.mode = WM_KNOT;
                    wui.hknot = best;
                    const WarpKnot &kn = W.knots[best];
                    float kwx, kwy;
                    L2W(kn.x, kn.y, kwx, kwy);
                    wui.grabx = kwx - mwx;
                    wui.graby = kwy - mwy;
                    wui.anchorx = kwx;
                    wui.anchory = kwy;
                    wui.dragknots.assign(wui.sel.begin(), wui.sel.end());
                }
            }
        }
        // 3. network lines (also the outer ones, which lie on the canvas edge; not with a modifier: that box selects)
        if (!hit && inext && !modkey) {
            int N = samples_per_seg(W);
            float bestd = 7.0f;
            int bdir = -1, bidx = -1;
            for (int j = 0; j <= W.ny; j++) {
                for (int i = 0; i < W.nx; i++) {
                    float prev[2];
                    W.row_seg(i, j, 0.0f, prev);
                    for (int n = 1; n <= N; n++) {
                        float p[2];
                        W.row_seg(i, j, (float) n / N, p);
                        float d = seg_dist_px(mpx, mpy, X.sc * prev[0] + X.ox, X.sc * prev[1] + X.oy,
                                              X.sc * p[0] + X.ox, X.sc * p[1] + X.oy);
                        if (d < bestd) { bestd = d; bdir = 0; bidx = j; }
                        prev[0] = p[0]; prev[1] = p[1];
                    }
                }
            }
            for (int i = 0; i <= W.nx; i++) {
                for (int j = 0; j < W.ny; j++) {
                    float prev[2];
                    W.col_seg(i, j, 0.0f, prev);
                    for (int n = 1; n <= N; n++) {
                        float p[2];
                        W.col_seg(i, j, (float) n / N, p);
                        float d = seg_dist_px(mpx, mpy, X.sc * prev[0] + X.ox, X.sc * prev[1] + X.oy,
                                              X.sc * p[0] + X.ox, X.sc * p[1] + X.oy);
                        if (d < bestd) { bestd = d; bdir = 1; bidx = i; }
                        prev[0] = p[0]; prev[1] = p[1];
                    }
                }
            }
            if (bdir != -1) {
                hit = true;
                wui.mode = WM_LINE;
                wui.linedir = bdir;
                wui.lineidx = bidx;
                wui.dragknots.clear();
                if (bdir == 0) for (int i = 0; i <= W.nx; i++) wui.dragknots.push_back(bidx * (W.nx + 1) + i);
                else for (int j = 0; j <= W.ny; j++) wui.dragknots.push_back(j * (W.nx + 1) + bidx);
                wui.grabx = 0.0f;
                wui.graby = 0.0f;
                wui.anchorx = mwx;
                wui.anchory = mwy;
            }
        }
        // 4. empty space
        if (!hit && incanvas) {
            if (modkey) {
                wui.mode = WM_BOX;
                wui.boxx0 = mwx;
                wui.boxy0 = mwy;
                wui.boxadd = true;
            } else {
                wui.mode = WM_EMPTY;
            }
        }
    }
    else if (wui.mode != WM_NONE) {
        if (!down) {
            // released
            if (wui.mode == WM_EMPTY) {
                float d = std::hypot(mpx - wui.startpx, mpy - wui.startpy);
                if (d <= 3.0f && wui.order.size() > 1) {
                    // bring the next background layer to the front, current one to the back
                    Layer *next = wui.order[wui.order.size() - 2];
                    std::vector<Layer *> &cl = sel_layers();
                    auto it = std::find(cl.begin(), cl.end(), next);
                    if (it != cl.end()) cl.erase(it);
                    cl.push_back(next);
                    mainmix->currlay[!mainprogram->prevmodus] = next;
                    mainprogram->effcat[next->deck]->value = next->effcat;
                    // old current goes to the back of the draw order, 'next' becomes the front
                    wui.order.erase(std::remove(wui.order.begin(), wui.order.end(), next), wui.order.end());
                    wui.order.erase(std::remove(wui.order.begin(), wui.order.end(), cur), wui.order.end());
                    wui.order.insert(wui.order.begin(), cur);
                    wui.order.push_back(next);
                }
            } else if (wui.mode == WM_BOX) {
                float x0 = std::min(wui.boxx0, mwx), x1 = std::max(wui.boxx0, mwx);
                float y0 = std::min(wui.boxy0, mwy), y1 = std::max(wui.boxy0, mwy);
                for (int j = 0; j <= W.ny; j++) {
                    for (int i = 0; i <= W.nx; i++) {
                        const WarpKnot &kn = W.K(i, j);
                        float kwx, kwy;
                        L2W(kn.x, kn.y, kwx, kwy);
                        if (kwx >= x0 && kwx <= x1 && kwy >= y0 && kwy <= y1) {
                            wui.sel.insert(j * (W.nx + 1) + i);
                        }
                    }
                }
            }
            if (wui.mode == WM_KNOT && wui.knotwassel && !wui.knotmoved) {
                // reclicking a selected knot (without dragging) deselects it
                wui.sel.erase(wui.hknot);
            }
            wui.mode = WM_NONE;
            wui.snapxon = wui.snapyon = false;
            mainprogram->leftmouse = false;
        }
        else {
            float pdist = std::hypot(mpx - wui.startpx, mpy - wui.startpy);
            if (wui.mode == WM_TOGGLE && pdist > 4.0f) {
                // shift/ctrl + drag starting on a knot is a box select: undo the toggle
                if (wui.knotwassel) wui.sel.insert(wui.hknot);
                else wui.sel.erase(wui.hknot);
                wui.mode = WM_BOX;
                wui.boxx0 = wui.pressw[0];
                wui.boxy0 = wui.pressw[1];
                wui.boxadd = true;
            }
            if (wui.mode == WM_KNOT && pdist > 3.0f) wui.knotmoved = true;
            if ((wui.mode == WM_KNOT && wui.knotmoved) || wui.mode == WM_LINE) {
                float tx = mwx + wui.grabx, ty = mwy + wui.graby;
                apply_snap_if(tx, ty);
                float ddx = tx - wui.anchorx, ddy = ty - wui.anchory;
                if (ddx != 0.0f || ddy != 0.0f) {
                    for (int k : wui.dragknots) {
                        // (world delta -> layer space)
                        W.knots[k].x += ddx / X.sc;
                        W.knots[k].y += ddy / X.sc;
                    }
                    wui.anchorx = tx;
                    wui.anchory = ty;
                    W.touch();
                }
            }
            else if (wui.mode == WM_HANDLE) {
                int ki = wui.hknot % (W.nx + 1), kj = wui.hknot / (W.nx + 1);
                WarpKnot &kn = W.K(ki, kj);
                float tx = mwx, ty = mwy;
                apply_snap_if(tx, ty);
                float tlx, tly;
                W2L(tx, ty, tlx, tly);
                float ox = tlx - kn.x, oy = tly - kn.y;
                bool dirx = wui.hwhich < 2;
                int type = dirx ? kn.typeX : kn.typeY;
                float *self = wui.hwhich == 0 ? kn.hxl : wui.hwhich == 1 ? kn.hxr : wui.hwhich == 2 ? kn.hyd : kn.hyu;
                float *other = wui.hwhich == 0 ? kn.hxr : wui.hwhich == 1 ? kn.hxl : wui.hwhich == 2 ? kn.hyu : kn.hyd;
                if (type == WK_FLUID) {
                    // only the stored out-handle (right/up) is kept; resolve() mirrors it into the in-handle
                    bool isout = (wui.hwhich == 1 || wui.hwhich == 3);
                    float *outh = dirx ? kn.hxr : kn.hyu;
                    outh[0] = isout ? ox : -ox;
                    outh[1] = isout ? oy : -oy;
                } else {
                    self[0] = ox; self[1] = oy;
                }
                (void) other;
                W.touch();
            }
            else if (wui.mode == WM_EMPTY || wui.mode == WM_LAYER) {
                float d = std::hypot(mpx - wui.startpx, mpy - wui.startpy);
                if (wui.mode == WM_EMPTY && d > 3.0f) wui.mode = WM_LAYER;
                if (wui.mode == WM_LAYER) {
                    float dwx = mwx - wui.lastwx, dwy = mwy - wui.lastwy;
                    // shift units per world unit, taken from the transform itself
                    const float eps = 1e-3f;
                    cur->shiftx->value += eps;
                    cur->shifty->value += eps;
                    WarpXf X1 = layerwarp_xf(cur);
                    cur->shiftx->value -= eps;
                    cur->shifty->value -= eps;
                    float dox = (X1.ox - X.ox) / eps, doy = (X1.oy - X.oy) / eps;
                    if (std::fabs(dox) > 1e-6f) cur->shiftx->value += dwx / dox;
                    if (std::fabs(doy) > 1e-6f) cur->shifty->value += dwy / doy;
                    mark_changed(cur);
                }
            }
            wui.lastwx = mwx;
            wui.lastwy = mwy;
        }
    }
    wui.prevdown = down;
    (void) pxw;

    // ---- drawing (direct) ----
    bool budm = mainprogram->directmode;
    mainprogram->directmode = true;
    {
        // canvas background
        draw_box(grey, black, wui.bx, wui.by, wui.bw, wui.bh, -1);

        // main grid (world space, fixed cell size)
        int gxn = std::max(1, (int) (wui.gridx->value + 0.5f));
        int gyn = std::max(1, (int) (wui.gridy->value + 0.5f));
        float stepx = 2.0f / gxn, stepy = 2.0f / gyn;
        float wxmin, wymin, wxmax, wymax;
        n2w(wui.bx, wui.by, wxmin, wymin);
        n2w(wui.bx + wui.bw, wui.by + wui.bh, wxmax, wymax);
        for (int k = (int) std::ceil((wxmin + 1.0f) / stepx); k * stepx - 1.0f <= wxmax; k++) {
            float x = k * stepx - 1.0f;
            wline(grey, x, wymin, x, wymax, true);
        }
        for (int k = (int) std::ceil((wymin + 1.0f) / stepy); k * stepy - 1.0f <= wymax; k++) {
            float y = k * stepy - 1.0f;
            wline(grey, wxmin, y, wxmax, y, true);
        }

        // layers: background ones at 30%, current at 70%
        float rx = wui.cx - wui.hx, ry = wui.cy - wui.hy, rw = wui.hx * 2.0f, rh = wui.hy * 2.0f;
        for (Layer *l : wui.order) {
            if (!l->node || !l->node->vidbox) continue;
            GLuint t = l->node->vidbox->tex;
            if (t == (GLuint) -1) continue;
            float op = (l == cur) ? 0.5f : 0.35f;
            draw_direct(nullptr, g_transp, rx, ry, rw, rh, 0.0f, 0.0f, 1.0f, op, 0, t, glob->w, glob->h, false, false);
        }
        // mix area border when zoomed out
        if (wui.zoom < 0.999f) {
            wline(g_whitebox, -1.0f, -1.0f, 1.0f, -1.0f, true);
            wline(g_whitebox, 1.0f, -1.0f, 1.0f, 1.0f, true);
            wline(g_whitebox, 1.0f, 1.0f, -1.0f, 1.0f, true);
            wline(g_whitebox, -1.0f, 1.0f, -1.0f, -1.0f, true);
        }
    }
    mainprogram->directmode = false;

    // ---- overlays (queued: drawn on top) ----
    // background layers: accentuated corners and outer edges
    for (Layer *l : wui.order) {
        if (l == cur) continue;
        Outline o = layer_outline(l);
        for (int c = 0; c < 4; c++) {
            int c2 = (c + 1) % 4;
            if (o.warped) {
                // outer edges of the warped network
                LayerWarpData &lw = l->warp;
                lw.resolve();
                WarpXf lxf = layerwarp_xf(l);
                int N = samples_per_seg(lw);
                auto edge = [&](int dir, int idx) {
                    int segs = dir == 0 ? lw.nx : lw.ny;
                    float prev[2];
                    bool havep = false;
                    for (int sg = 0; sg < segs; sg++) {
                        for (int n = 0; n <= N; n++) {
                            float p[2];
                            if (dir == 0) lw.row_seg(sg, idx, (float) n / N, p);
                            else lw.col_seg(idx, sg, (float) n / N, p);
                            p[0] = lxf.sc * p[0] + lxf.ox;
                            p[1] = lxf.sc * p[1] + lxf.oy;
                            if (havep) {
                                wline(g_bgcol, prev[0], prev[1], p[0], p[1]);
                                wline(g_bgcol, prev[0] + 0.002f, prev[1], p[0] + 0.002f, p[1]);
                            }
                            prev[0] = p[0]; prev[1] = p[1];
                            havep = true;
                        }
                    }
                };
                if (c == 0) edge(0, 0);
                if (c == 1) edge(1, lw.nx);
                if (c == 2) edge(0, lw.ny);
                if (c == 3) edge(1, 0);
            } else {
                wline(g_bgcol, o.c[c][0], o.c[c][1], o.c[c2][0], o.c[c2][1]);
                wline(g_bgcol, o.c[c][0] + 0.002f, o.c[c][1] + 0.002f, o.c[c2][0] + 0.002f, o.c[c2][1] + 0.002f);
            }
            float nx_, ny_;
            w2n(o.c[c][0], o.c[c][1], nx_, ny_);
            if (in_canvas(nx_, ny_)) {
                draw_box(g_bgcol, nx_, ny_, 0.01f, 1);
            }
        }
    }

    // current layer network
    {
        W.resolve();
        int N = samples_per_seg(W);

        // the network line (a whole row/column) under the mouse is shown lightblue; knots and handles have priority
        int hdir = -1, hidx = -1;
        if (wui.mode == WM_LINE) {
            hdir = wui.linedir;
            hidx = wui.lineidx;
        }
        else if (wui.mode == WM_NONE && inext && !inpanel && nomenu) {
            bool nearknot = false;
            for (int j = 0; j <= W.ny && !nearknot; j++) {
                for (int i = 0; i <= W.nx && !nearknot; i++) {
                    float sx_, sy_, kwx, kwy;
                    L2W(W.K(i, j).x, W.K(i, j).y, kwx, kwy);
                    w2s(kwx, kwy, sx_, sy_);
                    if (std::hypot(sx_ - mpx, sy_ - mpy) < 11.0f) nearknot = true;
                }
            }
            if (!nearknot) {
                float bestd = 7.0f;
                for (int j = 0; j <= W.ny; j++) {
                    for (int i = 0; i < W.nx; i++) {
                        float prev[2];
                        W.row_seg(i, j, 0.0f, prev);
                        for (int n = 1; n <= N; n++) {
                            float p[2];
                            W.row_seg(i, j, (float) n / N, p);
                            float d = seg_dist_px(mpx, mpy, X.sc * prev[0] + X.ox, X.sc * prev[1] + X.oy,
                                                  X.sc * p[0] + X.ox, X.sc * p[1] + X.oy);
                            if (d < bestd) { bestd = d; hdir = 0; hidx = j; }
                            prev[0] = p[0]; prev[1] = p[1];
                        }
                    }
                }
                for (int i = 0; i <= W.nx; i++) {
                    for (int j = 0; j < W.ny; j++) {
                        float prev[2];
                        W.col_seg(i, j, 0.0f, prev);
                        for (int n = 1; n <= N; n++) {
                            float p[2];
                            W.col_seg(i, j, (float) n / N, p);
                            float d = seg_dist_px(mpx, mpy, X.sc * prev[0] + X.ox, X.sc * prev[1] + X.oy,
                                                  X.sc * p[0] + X.ox, X.sc * p[1] + X.oy);
                            if (d < bestd) { bestd = d; hdir = 1; hidx = i; }
                            prev[0] = p[0]; prev[1] = p[1];
                        }
                    }
                }
            }
        }

        for (int j = 0; j <= W.ny; j++) {
            for (int i = 0; i < W.nx; i++) {
                float prev[2];
                W.row_seg(i, j, 0.0f, prev);
                for (int n = 1; n <= N; n++) {
                    float p[2];
                    W.row_seg(i, j, (float) n / N, p);
                    wline((hdir == 0 && hidx == j) ? g_lightblue : g_netcol, X.sc * prev[0] + X.ox, X.sc * prev[1] + X.oy, X.sc * p[0] + X.ox, X.sc * p[1] + X.oy);
                    prev[0] = p[0]; prev[1] = p[1];
                }
            }
        }
        for (int i = 0; i <= W.nx; i++) {
            for (int j = 0; j < W.ny; j++) {
                float prev[2];
                W.col_seg(i, j, 0.0f, prev);
                for (int n = 1; n <= N; n++) {
                    float p[2];
                    W.col_seg(i, j, (float) n / N, p);
                    wline((hdir == 1 && hidx == i) ? g_lightblue : g_netcol, X.sc * prev[0] + X.ox, X.sc * prev[1] + X.oy, X.sc * p[0] + X.ox, X.sc * p[1] + X.oy);
                    prev[0] = p[0]; prev[1] = p[1];
                }
            }
        }
        // handles of the selected knots
        for (int k : wui.sel) {
            if (k >= (int) W.knots.size()) continue;
            int ki = k % (W.nx + 1), kj = k / (W.nx + 1);
            const WarpKnot &kn = W.K(ki, kj);
            for (int hw = 0; hw < 4; hw++) {
                bool dirx = hw < 2;
                int type = dirx ? kn.typeX : kn.typeY;
                if (type != WK_FLUID && type != WK_BROKEN) continue;
                float hp[2];
                if (!W.handle_pos(ki, kj, hw, hp)) continue;
                float kwx, kwy, hwx, hwy;
                L2W(kn.x, kn.y, kwx, kwy);
                L2W(hp[0], hp[1], hwx, hwy);
                wline(white, kwx, kwy, hwx, hwy);
                float nx_, ny_;
                w2n(hwx, hwy, nx_, ny_);
                if (in_canvas(nx_, ny_)) draw_box(white, nx_, ny_, 0.008f, 1);
            }
        }
        // the knot under the mouse (shown filled)
        int hoverknot = -1;
        if (inext && !inpanel && nomenu && (wui.mode == WM_NONE || wui.mode == WM_KNOT || wui.mode == WM_TOGGLE)) {
            if (wui.mode == WM_KNOT) {
                hoverknot = wui.hknot;
            } else {
                float bestd = 11.0f;
                for (int j = 0; j <= W.ny; j++) {
                    for (int i = 0; i <= W.nx; i++) {
                        const WarpKnot &kn = W.K(i, j);
                        float sx_, sy_, kwx, kwy;
                        L2W(kn.x, kn.y, kwx, kwy);
                        w2s(kwx, kwy, sx_, sy_);
                        float d = std::hypot(sx_ - mpx, sy_ - mpy);
                        if (d < bestd) { bestd = d; hoverknot = j * (W.nx + 1) + i; }
                    }
                }
            }
        }
        // knots
        for (int j = 0; j <= W.ny; j++) {
            for (int i = 0; i <= W.nx; i++) {
                const WarpKnot &kn = W.K(i, j);
                float nx_, ny_, kwx, kwy;
                L2W(kn.x, kn.y, kwx, kwy);
                w2n(kwx, kwy, nx_, ny_);
                if (!in_canvas(nx_, ny_)) continue;
                int ki = j * (W.nx + 1) + i;
                bool s = wui.sel.count(ki) > 0;
                if (s) draw_box(g_orange, nx_, ny_, 0.013f, 1);
                else if (ki == hoverknot) draw_box(g_lightblue, nx_, ny_, 0.013f, 1);
                else draw_box(g_lightblue, nx_, ny_, 0.013f, 2);
            }
        }
    }

    // snap guides
    if (wui.snapxon) wline(g_snapcol, wui.snapx, -1000.0f, wui.snapx, 1000.0f);
    if (wui.snapyon) wline(g_snapcol, -1000.0f, wui.snapy, 1000.0f, wui.snapy);

    // box select rectangle
    if (wui.mode == WM_BOX) {
        wline(white, wui.boxx0, wui.boxy0, mwx, wui.boxy0);
        wline(white, mwx, wui.boxy0, mwx, mwy);
        wline(white, mwx, mwy, wui.boxx0, mwy);
        wline(white, wui.boxx0, mwy, wui.boxx0, wui.boxy0);
    }

    // ---- control panel ----
    for (int n = 0; n < 7; n++) plist[n]->handle();
    {
        // copy / paste network (the clipboard lives on, also after leaving warp mode)
        draw_box(wui.copybox->lcolor, wui.copybox->acolor, wui.copybox, -1);
        render_text("COPY NETWORK", white, wui.copybox->vtxcoords->x1 + 0.012f, wui.copybox->vtxcoords->y1 + 0.022f, 0.0005f, 0.0008f);
        float pacol[4] = {0.15f, 0.3f, 0.15f, 1.0f};
        if (!g_wclipvalid) { pacol[0] = pacol[1] = pacol[2] = 0.2f; }
        draw_box(wui.pastebox->lcolor, pacol, wui.pastebox, -1);
        render_text("PASTE NETWORK", white, wui.pastebox->vtxcoords->x1 + 0.012f, wui.pastebox->vtxcoords->y1 + 0.022f, 0.0005f, 0.0008f);
        if (wui.mode == WM_NONE && mainprogram->leftmouse) {
            if (wui.copybox->in()) {
                g_wclip.copy_data_from(W);
                g_wclipxf[0] = cur->shiftx->value;
                g_wclipxf[1] = cur->shifty->value;
                g_wclipxf[2] = cur->scale->value;
                g_wclipvalid = true;
                mainprogram->leftmouse = false;
            }
            else if (wui.pastebox->in() && g_wclipvalid) {
                W.copy_data_from(g_wclip);
                W.touch();
                cur->shiftx->value = g_wclipxf[0];
                cur->shifty->value = g_wclipxf[1];
                cur->scale->value = g_wclipxf[2];
                mark_changed(cur);
                wui.sel.clear();
                mainprogram->leftmouse = false;
            }
        }
    }
    draw_box(wui.cancelbox->lcolor, wui.cancelbox->acolor, wui.cancelbox, -1);
    render_text("CANCEL", white, wui.cancelbox->vtxcoords->x1 + 0.04f, wui.cancelbox->vtxcoords->y1 + 0.022f, 0.00055f, 0.0009f);
    if (wui.cancelbox->in() && mainprogram->leftmouse && wui.mode == WM_NONE) {
        mainprogram->leftmouse = false;
        layerwarp_cancel();
        mainprogram->directmode = budm;
        return;
    }

    // apply changes from the params
    if (std::fabs(wui.scale->value - wui.lastscale) > 1e-6f) {
        float ratio = wui.scale->value / cur->scale->value;
        cur->scale->value = wui.scale->value;
        cur->shiftx->value *= ratio;
        cur->shifty->value *= ratio;
        wui.lastscale = wui.scale->value;
        mark_changed(cur);
    }
    if ((int) wui.ktx->value != wui.lastktx) {
        for (int k : wui.sel) W.set_knot_type(k % (W.nx + 1), k / (W.nx + 1), true, (int) wui.ktx->value);
    }
    if ((int) wui.kty->value != wui.lastkty) {
        for (int k : wui.sel) W.set_knot_type(k % (W.nx + 1), k / (W.nx + 1), false, (int) wui.kty->value);
    }

    // warp undo: one step per finished operation (not while dragging or while a param is being edited)
    bool paramediting = editing(wui.scale) || editing(wui.xsteps) || editing(wui.ysteps) || editing(wui.ktx) ||
                        editing(wui.kty) || editing(wui.gridx) || editing(wui.gridy);
    if (g_warpdirty && wui.mode == WM_NONE && !paramediting && !mainprogram->menuondisplay) {
        undo_push();
    }
    // clicks in the warp canvas / controls are not for the global undo
    if (inext || inpanel) {
        mainprogram->recundo = false;
    }
    mainprogram->directmode = budm;
}

unsigned int layerwarp_tex(Layer *lay, unsigned int raw) {
    if (lay && lay->warp.active && lay->warp.tex != (unsigned int) -1) return lay->warp.tex;
    return raw;
}
