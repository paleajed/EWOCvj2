#include "GL/glew.h"
#include "GL/gl.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <random>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>
#include "program.h"
#include "ProjMap.h"
#include "LayerWarp.h"
#include "SAMSegmentation.h"
#include "videogenroom.h"
#include "ImageLoader.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/avutil.h"
#include "libavutil/imgutils.h"
}

namespace fs = std::filesystem;

extern void get_cameras();            // program.cpp
extern void output_video(EWindow *);  // program.cpp (the display thread of an output window)

namespace {

// default concept for the promptless "segment everything" run: SAM3 needs a text concept or geometry to start from
const char *kDefaultPrompt = "object";
const long kCamMaxPixels = 7680L * 4320L;   // highest webcam mode asked for (effectively: the highest the device offers)
const bool kSharpenCorners = false;   // TEMPORARILY OFF: the corner points at the tips of the Outline method (see sharpen_corners)
float g_prefexposure = 0.7f;  // the Exposure slider before it exists / from the preferences
float g_cornerang = 0.61f;   // turning angle (radians) above which an outline bend is a corner (Corner threshold slider)
const int kMaxSegs = 32;
const int kMinKnots = 3, kMaxKnots = 64;

// ======================================================================================================
// Webcam capture (own FFmpeg thread; independent of the layers)
// ======================================================================================================

// ---- supported webcam modes: FFmpeg logs them when the device is opened with list_options (dshow) or
// list_formats (v4l2); that open fails on purpose, only the log is wanted ----
struct CamMode {
    int w = 0, h = 0;
    std::string vcodec, pixfmt;
};

std::mutex g_camlogmtx;
std::vector<std::string> g_camlog;

void camlog_cb(void *, int, const char *fmt, va_list vl) {
    char buf[1024];
    vsnprintf(buf, sizeof(buf), fmt, vl);
    std::lock_guard<std::mutex> lock(g_camlogmtx);
    g_camlog.push_back(buf);
}

// every "WxH" in the line (dshow: "min s=640x480 fps=5 max s=1920x1080 fps=30"; v4l2: "... : 1920x1080 1280x720")
void sizes_in_line(const std::string &line, std::vector<std::pair<int, int>> &out) {
    for (size_t i = 0; i < line.size(); i++) {
        if (!isdigit((unsigned char) line[i]) || (i > 0 && isdigit((unsigned char) line[i - 1]))) continue;
        size_t j = i;
        while (j < line.size() && isdigit((unsigned char) line[j])) j++;
        if (j >= line.size() || line[j] != 'x' || j + 1 >= line.size() || !isdigit((unsigned char) line[j + 1])) continue;
        size_t k = j + 1;
        while (k < line.size() && isdigit((unsigned char) line[k])) k++;
        int w = atoi(line.substr(i, j - i).c_str()), h = atoi(line.substr(j + 1, k - j - 1).c_str());
        if (w >= 160 && h >= 120 && w <= 16384 && h <= 16384) out.push_back({w, h});
        i = k;
    }
}

std::string token_after(const std::string &line, const std::string &key) {
    size_t p = line.find(key);
    if (p == std::string::npos) return "";
    p += key.size();
    size_t e = p;
    while (e < line.size() && !isspace((unsigned char) line[e])) e++;
    return line.substr(p, e - p);
}

std::atomic<long> g_cammax{0};   // the most pixels the webcam mode may have; 0 = the highest there is

CamMode query_best_mode(const AVInputFormat *ifmt, const std::string &name) {
    CamMode best;
    const long maxpix = g_cammax.load() > 0 ? g_cammax.load() : kCamMaxPixels;
#if defined(WINDOWS) || defined(LINUX)
    {
        std::lock_guard<std::mutex> lock(g_camlogmtx);
        g_camlog.clear();
    }
    av_log_set_callback(camlog_cb);
    AVDictionary *opts = nullptr;
#ifdef WINDOWS
    av_dict_set(&opts, "list_options", "true", 0);
#else
    av_dict_set(&opts, "list_formats", "all", 0);
#endif
    AVFormatContext *fc = avformat_alloc_context();
    avformat_open_input(&fc, name.c_str(), (AVInputFormat *) ifmt, &opts);   // (fails or lists, either way only the log matters)
    if (fc) avformat_close_input(&fc);
    av_dict_free(&opts);
    av_log_set_callback(ewoc_av_log_callback);

    std::vector<std::string> lines;
    {
        std::lock_guard<std::mutex> lock(g_camlogmtx);
        lines = g_camlog;
    }
    long bestarea = 0;
    for (const std::string &line : lines) {
        std::vector<std::pair<int, int>> sz;
        sizes_in_line(line, sz);
        if (sz.empty()) continue;
        std::string vc = token_after(line, "vcodec="), pf = token_after(line, "pixel_format=");
        for (auto &s : sz) {
            long area = (long) s.first * s.second;
            if (area > maxpix) continue;
            // (larger wins; at equal size a compressed mode, which is the faster one over USB)
            if (area > bestarea || (area == bestarea && !vc.empty() && best.vcodec.empty())) {
                bestarea = area;
                best.w = s.first;
                best.h = s.second;
                best.vcodec = vc;
                best.pixfmt = pf;
            }
        }
    }
#else
    (void) ifmt; (void) name;
#endif
    return best;
}

struct CamCapture {
    std::thread th;
    std::atomic<bool> run{false};
    std::atomic<uint64_t> counter{0};
    std::atomic<int> state{0};        // 0 idle, 1 opening, 2 streaming, 3 failed
    std::mutex mtx;
    std::vector<uint8_t> frame;       // RGBA, top-down
    int w = 0, h = 0;

    static int interrupt_cb(void *opaque) { return !((CamCapture *) opaque)->run.load(); }

    void start(const std::string &name) {
        stop();
        {
            std::lock_guard<std::mutex> lock(mtx);
            frame.clear();
            w = h = 0;
        }
        counter = 0;
        state = 1;
        run = true;
        th = std::thread(&CamCapture::loop, this, name);
    }

    void stop() {
        run = false;
        if (th.joinable()) th.join();
        state = 0;
    }

    // copies the latest frame; false when there is none yet
    bool grab(std::vector<uint8_t> &out, int &ow, int &oh) {
        std::lock_guard<std::mutex> lock(mtx);
        if (frame.empty() || w <= 0 || h <= 0) return false;
        out = frame;
        ow = w;
        oh = h;
        return true;
    }

    void loop(std::string name) {
        avdevice_register_all();
#ifdef WINDOWS
        const AVInputFormat *ifmt = av_find_input_format("dshow");
#elif defined(MACOS)
        const AVInputFormat *ifmt = av_find_input_format("avfoundation");
#else
        const AVInputFormat *ifmt = av_find_input_format("v4l2");
#endif
        // Asks the device for its supported modes and takes the highest (up to kCamMaxPixels); falls back to
        // the default mode when the device refuses or says nothing.
        CamMode mode = query_best_mode(ifmt, name);
        char sizestr[32];
        snprintf(sizestr, sizeof(sizestr), "%dx%d", mode.w > 0 ? mode.w : 1280, mode.h > 0 ? mode.h : 720);
        fprintf(stderr, "[ProjMap] webcam best mode: %s %s %s\n", mode.w > 0 ? sizestr : "(unknown, trying 1280x720)",
                mode.vcodec.c_str(), mode.pixfmt.c_str());
        AVFormatContext *fc = nullptr;
        for (int attempt = 0; attempt < 3 && run.load(); attempt++) {
            fc = avformat_alloc_context();
            fc->interrupt_callback.callback = &CamCapture::interrupt_cb;
            fc->interrupt_callback.opaque = this;
            AVDictionary *opts = nullptr;
#ifdef MACOS
            av_dict_set(&opts, "framerate", "30", 0);   // (avfoundation's default 29.97 is rejected by most cameras)
#endif
            if (attempt < 2) av_dict_set(&opts, "video_size", sizestr, 0);
#ifdef WINDOWS
            if (attempt == 0) {
                if (!mode.vcodec.empty()) av_dict_set(&opts, "vcodec", mode.vcodec.c_str(), 0);
                else if (!mode.pixfmt.empty()) av_dict_set(&opts, "pixel_format", mode.pixfmt.c_str(), 0);
            }
#endif
            int r = avformat_open_input(&fc, name.c_str(), (AVInputFormat *) ifmt, &opts);
            av_dict_free(&opts);
            if (r >= 0) break;   // (on failure fc has been freed and is nullptr)
            fc = nullptr;
        }
        if (!fc) {
            state = 3;
            return;
        }
        if (avformat_find_stream_info(fc, nullptr) < 0) {
            avformat_close_input(&fc);
            state = 3;
            return;
        }
        int si = av_find_best_stream(fc, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        if (si < 0) {
            avformat_close_input(&fc);
            state = 3;
            return;
        }
        AVCodecParameters *par = fc->streams[si]->codecpar;
        const AVCodec *dec = avcodec_find_decoder(par->codec_id);
        AVCodecContext *cc = dec ? avcodec_alloc_context3(dec) : nullptr;
        if (!cc || avcodec_parameters_to_context(cc, par) < 0 || avcodec_open2(cc, dec, nullptr) < 0) {
            if (cc) avcodec_free_context(&cc);
            avformat_close_input(&fc);
            state = 3;
            return;
        }
        AVPacket *pkt = av_packet_alloc();
        AVFrame *fr = av_frame_alloc();
        SwsContext *sws = nullptr;
        std::vector<uint8_t> buf;
        state = 2;
        while (run.load()) {
            if (av_read_frame(fc, pkt) < 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            }
            if (pkt->stream_index == si && avcodec_send_packet(cc, pkt) == 0) {
                while (avcodec_receive_frame(cc, fr) == 0) {
                    int W = fr->width, H = fr->height;
                    // (MJPEG decodes to the deprecated full range "YUVJ" formats: use the plain ones with the range set
                    // explicitly, which silences swscale's warning on every frame and keeps the levels right)
                    AVPixelFormat sf = (AVPixelFormat) fr->format;
                    bool fullrange = false;
                    switch (sf) {
                        case AV_PIX_FMT_YUVJ420P: sf = AV_PIX_FMT_YUV420P; fullrange = true; break;
                        case AV_PIX_FMT_YUVJ422P: sf = AV_PIX_FMT_YUV422P; fullrange = true; break;
                        case AV_PIX_FMT_YUVJ444P: sf = AV_PIX_FMT_YUV444P; fullrange = true; break;
                        case AV_PIX_FMT_YUVJ440P: sf = AV_PIX_FMT_YUV440P; fullrange = true; break;
                        default: break;
                    }
                    sws = sws_getCachedContext(sws, W, H, sf, W, H, AV_PIX_FMT_RGBA,
                                               SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
                    if (!sws) break;
                    if (fullrange) {
                        const int *co = sws_getCoefficients(SWS_CS_DEFAULT);
                        sws_setColorspaceDetails(sws, co, 1, co, 1, 0, 1 << 16, 1 << 16);
                    }
                    buf.resize((size_t) W * H * 4);
                    uint8_t *dst[4] = {buf.data(), nullptr, nullptr, nullptr};
                    int dstride[4] = {W * 4, 0, 0, 0};
                    sws_scale(sws, fr->data, fr->linesize, 0, H, dst, dstride);
                    for (size_t i = 3; i < buf.size(); i += 4) buf[i] = 255;
                    {
                        std::lock_guard<std::mutex> lock(mtx);
                        frame.swap(buf);
                        w = W;
                        h = H;
                    }
                    counter++;
                }
            }
            av_packet_unref(pkt);
        }
        if (sws) sws_freeContext(sws);
        av_frame_free(&fr);
        av_packet_free(&pkt);
        avcodec_free_context(&cc);
        avformat_close_input(&fc);
    }
};

// ======================================================================================================
// Segments, outlines
// ======================================================================================================

struct Seg {
    int id = 0;
    int camidx = 0;                    // 0 = built from webcam 1 (the one you select on), 1 = from webcam 2
    float col[4] = {1, 1, 1, 1};
    std::vector<uint8_t> mask;         // snapshot size, 255 inside
    int x0 = 0, y0 = 0, x1 = 0, y1 = 0;   // bounding box (inclusive)
    std::vector<uint32_t> edge;        // boundary pixels (for the outline in the overlay)
    std::vector<float> dense;          // closed outline, normalized webcam coords, evenly spaced along its length
    std::vector<float> kcam;           // 2 * N knots on the outline (webcam)
    std::vector<char> kcorner;         // knot is a corner (sharp)
    std::vector<char> kretried;        // knot was traced a second time (it looked like a wrong find)
    std::vector<float> kbeam;          // 2 * N knots (beamer), valid where kdone
    std::vector<char> kdone;
    std::vector<float> kscan;          // 2 * N knots (beamer) as the stripe scan gave them, valid where kscanok
    std::vector<char> kscanok;
    std::vector<float> kscale;         // per scanned knot: beamer pixels per webcam pixel there (how stretched the surface is)
    bool selected = false;
    bool traced = false;               // all knots found on the beamer
    bool resolving = false;            // RESOLVE was clicked: its knots are being found on the beamer
    bool resolved = false;             // its outline is on the beamer and a layer is assigned (see mapped)
    int mapidx = -1;                   // index in PM::mapped
    Layer *lay = nullptr;              // the layer assigned to it
    bool verified = false;             // the outline was checked on the beamer (see verify_local)
    // what the checks on the beamer found, added up: at every knot the lit outline was displaced by vu (2 numbers per knot,
    // webcam pixels), so the beamer knot is moved by minus vJ (beamer per webcam) times that
    bool vhas = false;
    std::vector<float> vu;
    float vJ[4] = {1, 0, 0, 1};
    int vtries = 0;
    std::vector<uint8_t> soft;         // after the refinement: per pixel the share of the SAM3 runs that had it in the mask (0..255), else empty
};

// Traces the largest boundary loop of a binary mask (pixel corners, y down).
bool trace_outer(const std::vector<uint8_t> &m, int W, int H, int bx0, int by0, int bx1, int by1,
                 std::vector<float> &out) {
    struct E { int x0, y0, x1, y1; };
    std::vector<E> es;
    auto in = [&](int x, int y) { return x >= 0 && y >= 0 && x < W && y < H && m[(size_t) y * W + x] > 128; };
    for (int y = by0; y <= by1; y++) {
        for (int x = bx0; x <= bx1; x++) {
            if (!in(x, y)) continue;
            // (interior on the right of the direction of travel)
            if (!in(x, y - 1)) es.push_back({x, y, x + 1, y});
            if (!in(x + 1, y)) es.push_back({x + 1, y, x + 1, y + 1});
            if (!in(x, y + 1)) es.push_back({x + 1, y + 1, x, y + 1});
            if (!in(x - 1, y)) es.push_back({x, y + 1, x, y});
        }
    }
    if (es.size() < 4) return false;
    std::unordered_multimap<uint64_t, int> start;
    auto key = [&](int x, int y) { return (uint64_t) y * (uint64_t) (W + 1) + (uint64_t) x; };
    for (size_t i = 0; i < es.size(); i++) start.emplace(key(es[i].x0, es[i].y0), (int) i);

    std::vector<char> used(es.size(), 0);
    double bestA = 0.0;
    for (size_t s = 0; s < es.size(); s++) {
        if (used[s]) continue;
        std::vector<float> loop;
        int cur = (int) s;
        while (true) {
            used[cur] = 1;
            loop.push_back((float) es[cur].x0);
            loop.push_back((float) es[cur].y0);
            int dx = es[cur].x1 - es[cur].x0, dy = es[cur].y1 - es[cur].y0;
            auto rg = start.equal_range(key(es[cur].x1, es[cur].y1));
            int nxt = -1, bestScore = -1;
            for (auto it = rg.first; it != rg.second; ++it) {
                int c = it->second;
                if (used[c] && c != (int) s) continue;
                int cdx = es[c].x1 - es[c].x0, cdy = es[c].y1 - es[c].y0;
                int score = (cdx == -dy && cdy == dx) ? 2 : (cdx == dx && cdy == dy ? 1 : 0);   // right, straight, left
                if (score > bestScore) { bestScore = score; nxt = c; }
            }
            if (nxt == -1 || nxt == (int) s) break;
            cur = nxt;
        }
        double a = 0.0;
        size_t n = loop.size() / 2;
        for (size_t i = 0; i < n; i++) {
            size_t j = (i + 1) % n;
            a += (double) loop[i * 2] * loop[j * 2 + 1] - (double) loop[j * 2] * loop[i * 2 + 1];
        }
        a = std::fabs(a) * 0.5;
        if (a > bestA) { bestA = a; out = loop; }
    }
    return out.size() >= 12;
}

// smooths the pixel staircase and resamples to D points evenly spaced along the loop (normalized by W,H);
// starts at the topmost-leftmost point and always runs the same way round
void smooth_resample(std::vector<float> p, int D, int W, int H, std::vector<float> &out) {
    int n = (int) p.size() / 2;
    for (int pass = 0; pass < 2; pass++) {
        std::vector<float> t(p.size());
        for (int i = 0; i < n; i++) {
            float sx = 0, sy = 0;
            for (int k = -2; k <= 2; k++) {
                int j = (i + k + n * 2) % n;
                sx += p[j * 2];
                sy += p[j * 2 + 1];
            }
            t[i * 2] = sx / 5.0f;
            t[i * 2 + 1] = sy / 5.0f;
        }
        p.swap(t);
    }
    // orientation
    double a = 0.0;
    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;
        a += (double) p[i * 2] * p[j * 2 + 1] - (double) p[j * 2] * p[i * 2 + 1];
    }
    if (a < 0.0) {
        for (int i = 0; i < n / 2; i++) {
            std::swap(p[i * 2], p[(n - 1 - i) * 2]);
            std::swap(p[i * 2 + 1], p[(n - 1 - i) * 2 + 1]);
        }
    }
    int s0 = 0;
    for (int i = 1; i < n; i++) {
        if (p[i * 2 + 1] < p[s0 * 2 + 1] || (p[i * 2 + 1] == p[s0 * 2 + 1] && p[i * 2] < p[s0 * 2])) s0 = i;
    }
    std::vector<float> cum(n + 1, 0.0f);
    for (int i = 0; i < n; i++) {
        int a_ = (s0 + i) % n, b_ = (s0 + i + 1) % n;
        cum[i + 1] = cum[i] + std::hypot(p[b_ * 2] - p[a_ * 2], p[b_ * 2 + 1] - p[a_ * 2 + 1]);
    }
    float L = cum[n];
    out.assign((size_t) D * 2, 0.0f);
    if (L <= 0.0f) return;
    int seg = 0;
    for (int k = 0; k < D; k++) {
        float s = L * (float) k / (float) D;
        while (seg < n - 1 && cum[seg + 1] < s) seg++;
        int a_ = (s0 + seg) % n, b_ = (s0 + seg + 1) % n;
        float len = cum[seg + 1] - cum[seg];
        float t = len > 0.0f ? (s - cum[seg]) / len : 0.0f;
        out[k * 2] = (p[a_ * 2] + t * (p[b_ * 2] - p[a_ * 2])) / (float) W;
        out[k * 2 + 1] = (p[a_ * 2 + 1] + t * (p[b_ * 2 + 1] - p[a_ * 2 + 1])) / (float) H;
    }
}

// corners of the outline: points where the direction turns sharply over a short stretch (local maxima of the
// turning angle above ~35 degrees)
void find_corners(const std::vector<float> &d, std::vector<int> &corners) {
    int D = (int) d.size() / 2;
    corners.clear();
    if (D < 24) return;
    int k = std::max(3, D / 48);
    std::vector<float> ang(D, 0.0f);
    for (int i = 0; i < D; i++) {
        int a = (i - k + D) % D, b = (i + k) % D;
        float v1x = d[i * 2] - d[a * 2], v1y = d[i * 2 + 1] - d[a * 2 + 1];
        float v2x = d[b * 2] - d[i * 2], v2y = d[b * 2 + 1] - d[i * 2 + 1];
        float l = std::hypot(v1x, v1y) * std::hypot(v2x, v2y);
        if (l <= 1e-12f) continue;
        ang[i] = std::acos(std::clamp((v1x * v2x + v1y * v2y) / l, -1.0f, 1.0f));
    }
    // the same turning angle over a 3x wider window: on a curve it grows with the window (about 3x), at a real corner
    // it stays the same, so a corner is a bend that is concentrated in one spot
    const int k3 = 3 * k;
    std::vector<float> ang3(D, 0.0f);
    for (int i = 0; i < D; i++) {
        int a = (i - k3 + D * 4) % D, b = (i + k3) % D;
        float v1x = d[i * 2] - d[a * 2], v1y = d[i * 2 + 1] - d[a * 2 + 1];
        float v2x = d[b * 2] - d[i * 2], v2y = d[b * 2 + 1] - d[i * 2 + 1];
        float l = std::hypot(v1x, v1y) * std::hypot(v2x, v2y);
        if (l <= 1e-12f) continue;
        ang3[i] = std::acos(std::clamp((v1x * v2x + v1y * v2y) / l, -1.0f, 1.0f));
    }
    const float kCornerAngle = g_cornerang;
    std::vector<int> cand;
    for (int i = 0; i < D; i++) {
        if (ang[i] < kCornerAngle) continue;
        if (ang[i] < 0.65f * ang3[i]) continue;   // (a curve, not a corner)
        bool maxi = true;
        for (int j = -k; j <= k && maxi; j++) {
            if (j == 0) continue;
            float o = ang[(i + j + D) % D];
            if (o > ang[i] || (o == ang[i] && j < 0)) maxi = false;
        }
        if (maxi) cand.push_back(i);
    }
    // corners at least 3k apart (the strongest wins), at most ~12
    std::sort(cand.begin(), cand.end(), [&](int a, int b) { return ang[a] > ang[b]; });
    std::vector<int> keep;
    for (int c : cand) {
        bool ok = (int) keep.size() < 12;
        for (int q : keep) {
            int dd = std::abs(c - q);
            if (std::min(dd, D - dd) < k3) ok = false;
        }
        if (ok) keep.push_back(c);
    }
    std::sort(keep.begin(), keep.end());
    corners = keep;
}

// The evenly resampled outline cuts off the tip of a corner (by up to half a sample spacing). The true corner is
// where the two straight stretches next to it meet: intersect the lines through the samples just before and just
// after the corner.
void refine_corner(const std::vector<float> &d, int c, float *out) {
    int D = (int) d.size() / 2;
    out[0] = d[c * 2];
    out[1] = d[c * 2 + 1];
    int k = std::max(3, D / 48);
    if (k < 5) return;
    int a0 = (c - k + D) % D, a1 = (c - 3 + D) % D, b0 = (c + 3) % D, b1 = (c + k) % D;
    float p1x = d[a0 * 2], p1y = d[a0 * 2 + 1], r1x = d[a1 * 2] - p1x, r1y = d[a1 * 2 + 1] - p1y;
    float p2x = d[b0 * 2], p2y = d[b0 * 2 + 1], r2x = d[b1 * 2] - p2x, r2y = d[b1 * 2 + 1] - p2y;
    float den = r1x * r2y - r1y * r2x;
    float l1 = std::hypot(r1x, r1y), l2 = std::hypot(r2x, r2y);
    if (l1 <= 1e-9f || l2 <= 1e-9f || std::fabs(den) < 0.2f * l1 * l2) return;   // (no clear corner: lines too parallel)
    float t = ((p2x - p1x) * r2y - (p2y - p1y) * r2x) / den;
    float ix = p1x + t * r1x, iy = p1y + t * r1y;
    float spacing = 0.0f;
    for (int i = 0; i < D; i++) {
        int j = (i + 1) % D;
        spacing += std::hypot(d[j * 2] - d[i * 2], d[j * 2 + 1] - d[i * 2 + 1]);
    }
    spacing /= (float) D;
    if (std::hypot(ix - out[0], iy - out[1]) > 4.0f * spacing) return;
    out[0] = ix;
    out[1] = iy;
}

// Better than the lines through the few samples next to a corner (a SAM3 mask rounds its corners off over a few pixels,
// which is exactly where those samples are): a line is fitted through the whole straight stretch on each side of the
// corner (without the 20% at both ends, the rounded parts), and the corner is where those two lines meet. Falls back
// to refine_corner() when a stretch is short, not straight, the lines nearly parallel, or the result far from the
// detected corner.
bool fit_stretch_line(const std::vector<float> &d, int D, int a, int cnt, double &px, double &py, double &dx, double &dy,
                      double &len, double &dev) {
    double mx = 0, my = 0;
    for (int k = 0; k < cnt; k++) { int i = (a + k) % D; mx += d[i * 2]; my += d[i * 2 + 1]; }
    mx /= cnt; my /= cnt;
    double sxx = 0, syy = 0, sxy = 0;
    for (int k = 0; k < cnt; k++) {
        int i = (a + k) % D;
        double x = d[i * 2] - mx, y = d[i * 2 + 1] - my;
        sxx += x * x; syy += y * y; sxy += x * y;
    }
    double th = 0.5 * std::atan2(2.0 * sxy, sxx - syy);
    px = mx; py = my; dx = std::cos(th); dy = std::sin(th);
    dev = 0.0;
    for (int k = 0; k < cnt; k++) {
        int i = (a + k) % D;
        dev = std::max(dev, std::fabs((d[i * 2] - mx) * dy - (d[i * 2 + 1] - my) * dx));
    }
    int e = (a + cnt - 1) % D;
    len = std::hypot(d[e * 2] - d[a * 2], d[e * 2 + 1] - d[a * 2 + 1]);
    return len > 1e-6;
}

void refine_corner_edges(const std::vector<float> &d, const std::vector<int> &corners, int j, float *out) {
    const int D = (int) d.size() / 2, nc = (int) corners.size();
    const int c = corners[j];
    if (nc < 2) { refine_corner(d, c, out); return; }
    const int cp = corners[(j - 1 + nc) % nc], cn = corners[(j + 1) % nc];
    int LA = (c - cp + D) % D, LB = (cn - c + D) % D;
    if (LA == 0) LA = D;
    if (LB == 0) LB = D;
    const int tA = std::max(3, LA * 20 / 100), tB = std::max(3, LB * 20 / 100);
    const int cntA = LA - 2 * tA + 1, cntB = LB - 2 * tB + 1;
    if (cntA < 6 || cntB < 6) { refine_corner(d, c, out); return; }
    double ax, ay, adx, ady, alen, adev, bx, by, bdx, bdy, blen, bdev;
    if (!fit_stretch_line(d, D, (cp + tA) % D, cntA, ax, ay, adx, ady, alen, adev) ||
        !fit_stretch_line(d, D, (c + tB) % D, cntB, bx, by, bdx, bdy, blen, bdev) ||
        adev > 0.04 * alen + 0.0005 || bdev > 0.04 * blen + 0.0005) {   // (not straight)
        refine_corner(d, c, out);
        return;
    }
    double den = adx * bdy - ady * bdx;
    if (std::fabs(den) < 0.42) { refine_corner(d, c, out); return; }   // (nearly parallel)
    double t = ((bx - ax) * bdy - (by - ay) * bdx) / den;
    double ix = ax + t * adx, iy = ay + t * ady;
    double perim = 0.0;
    for (int i = 0; i < D; i++) {
        int jn = (i + 1) % D;
        perim += std::hypot(d[jn * 2] - d[i * 2], d[jn * 2 + 1] - d[i * 2 + 1]);
    }
    const double maxmove = std::min(0.3 * std::min(alen, blen) + 0.002, 6.0 * perim / D);   // (never far from the outline)
    if (std::hypot(ix - d[c * 2], iy - d[c * 2 + 1]) > maxmove) { refine_corner(d, c, out); return; }
    out[0] = (float) ix;
    out[1] = (float) iy;
}

// unit direction pointing into the segment at a webcam pixel position (towards the centre of mass of the mask
// pixels around it; at a corner that is the diagonal); zero when there is no clear direction
// How many times larger than a 2560 px wide image a webcam image is (never less than 1). The sizes in webcam pixels in this file
// (windows, margins, insets, blurs) stand for a size on the object (SAM3's mask edge, for one, is only good to about half a cell
// of its 252 x 252 grid), so they scale with the webcam.
// The sizes (windows 12 to 30 px, margins 3 to 6 px, 5x5 blur, 1 px inset) were found on a webcam 2560 px wide, so that is 1: a
// 2560 px image is treated exactly as it was, a 3840 px image 1.5 times as large. (They were first taken to be for 1280 px, which
// scaled a 2560 px image by 2 and a 3840 px one by 3, and the outline got worse.)
float cam_scale(int W) {
    (void) W;
    return 1.0f;   // TEST: the sizes as they were originally, whatever the webcam resolution (the scaling is switched off)
}

void inward_dir(const Seg &s, float xpx, float ypx, int W, int H, float *out) {
    out[0] = out[1] = 0.0f;
    if (s.mask.size() < (size_t) W * H) return;
    const int r = std::max(6, (int) std::lround(6.0f * cam_scale(W)));
    double sx = 0.0, sy = 0.0;
    int n = 0;
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            int x = (int) xpx + dx, y = (int) ypx + dy;
            if (x < 0 || y < 0 || x >= W || y >= H) continue;
            if (s.mask[(size_t) y * W + x] > 128) { sx += dx; sy += dy; n++; }
        }
    }
    if (n == 0) return;
    sx /= n; sy /= n;
    double l = std::hypot(sx, sy);
    if (l < 0.3) return;
    out[0] = (float) (sx / l);
    out[1] = (float) (sy / l);
}

// Knots on the closed (evenly sampled) outline: one on every corner (flagged, the curve is sharp there), the
// rest of the N spread evenly over the stretches between the corners.
void make_knots(const std::vector<float> &dense, int N, std::vector<float> &out, std::vector<char> &flags) {
    int D = (int) dense.size() / 2;
    out.clear();
    flags.clear();
    if (D < 3) return;
    auto at = [&](float f) {
        f = std::fmod(f, (float) D);
        if (f < 0.0f) f += (float) D;
        int a = (int) f % D, b = (a + 1) % D;
        float t = f - std::floor(f);
        out.push_back(dense[a * 2] + t * (dense[b * 2] - dense[a * 2]));
        out.push_back(dense[a * 2 + 1] + t * (dense[b * 2 + 1] - dense[a * 2 + 1]));
    };
    std::vector<int> corners;
    find_corners(dense, corners);
    if ((int) corners.size() > N / 3) {   // (few knots: only the sharpest corners get their own knot)
        corners.resize(std::max(0, N / 3));
    }
    int nc = (int) corners.size();
    if (nc == 0) {
        for (int i = 0; i < N; i++) { at((float) i * (float) D / (float) N); flags.push_back(0); }
        return;
    }
    for (int j = 0; j < nc; j++) {
        int c0 = corners[j], c1 = corners[(j + 1) % nc];
        int len = (c1 - c0 + D) % D;
        if (len == 0) len = D;
        int extra = std::max(0, (int) std::lround((float) (N - nc) * (float) len / (float) D));
        float cp[2];
        refine_corner_edges(dense, corners, j, cp);
        out.push_back(cp[0]);
        out.push_back(cp[1]);
        flags.push_back(1);
        for (int m = 0; m < extra; m++) { at((float) c0 + (float) len * (float) (m + 1) / (float) (extra + 1)); flags.push_back(0); }
    }
}

// closed smooth (Catmull-Rom) curve through the knots, `per` samples per span; flagged knots are corners (the
// curve leaves them straight towards the next knot and arrives straight from the previous one)
void closed_curve(const std::vector<float> &k, const std::vector<char> &corner, int per, std::vector<float> &out) {
    int n = (int) k.size() / 2;
    out.clear();
    if (n < 3) { out = k; return; }
    auto isc = [&](int i) { return i < (int) corner.size() && corner[i]; };
    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;
        const float *p1 = &k[i * 2], *p2 = &k[j * 2];
        float p0[2], p3[2];
        const float *a = &k[((i - 1 + n) % n) * 2], *b = &k[((i + 2) % n) * 2];
        for (int c = 0; c < 2; c++) {
            p0[c] = isc(i) ? 2.0f * p1[c] - p2[c] : a[c];
            p3[c] = isc(j) ? 2.0f * p2[c] - p1[c] : b[c];
        }
        for (int s = 0; s < per; s++) {
            float t = (float) s / (float) per, t2 = t * t, t3 = t2 * t;
            for (int c = 0; c < 2; c++) {
                out.push_back(0.5f * ((2.0f * p1[c]) + (-p0[c] + p2[c]) * t + (2.0f * p0[c] - 5.0f * p1[c] + 4.0f * p2[c] - p3[c]) * t2 +
                                      (-p0[c] + 3.0f * p1[c] - 3.0f * p2[c] + p3[c]) * t3));
            }
        }
    }
}

void hsv2rgb(float h, float s, float v, float *rgb) {
    float c = v * s, x = c * (1.0f - std::fabs(std::fmod(h * 6.0f, 2.0f) - 1.0f)), m = v - c;
    float r = 0, g = 0, b = 0;
    int hi = (int) (h * 6.0f) % 6;
    if (hi == 0) { r = c; g = x; } else if (hi == 1) { r = x; g = c; } else if (hi == 2) { g = c; b = x; }
    else if (hi == 3) { g = x; b = c; } else if (hi == 4) { r = x; b = c; } else { r = c; b = x; }
    rgb[0] = r + m; rgb[1] = g + m; rgb[2] = b + m;
}

// ======================================================================================================
// State
// ======================================================================================================

enum BeamMode { BM_BLACK = 0, BM_PROJECT, BM_WHITE, BM_FILL, BM_VERIFY, BM_PATTERN };

// a resolved shape: its outline on the beamer and the layer that is projected on it
struct Mapped {
    std::vector<float> kb;       // beamer knots (as used for the mask)
    std::vector<char> kcorner;
    Layer *lay = nullptr;
};

struct GroupTex {
    Layer *lay = nullptr;
    GLuint masktex = (GLuint) -1;
};

struct Step {
    const char *name;
    int state = 0;   // 0 pending, 1 running, 2 done, 3 failed
};

// the stripe scan data of one webcam (see begin_scan): every webcam pixel gets the beamer position that lights it
struct ScanData {
    int scanW = 0, scanH = 0;
    std::vector<uint8_t> scanF[4];         // the four phase shifted frames of the pattern being shown (brightness)
    std::vector<uint16_t> scanPh[2][8];    // per axis and stripe period: the phase (0..65535 = one cycle)
    std::vector<uint8_t> scanMod[2][8];    // per axis and stripe period: the stripe contrast (2 B)
    std::vector<uint8_t> scanSig;          // per webcam pixel: how exact its beamer position is, in 0.05 beamer pixels (the larger of x and y)
    std::vector<uint8_t> scanSatG;         // saturated in one of the frames of the stripe period being taken (that period is no good at that pixel)
    std::vector<uint8_t> scanSatCnt;       // per pixel: in how many stripe periods it was saturated (glare, a highlight)
    std::vector<uint8_t> scanWhy;          // per pixel: 0 it has a position, 1 stripes too weak, 2 no candidate, 3 the periods disagree, 4 ambiguous, 5 dropped by the neighbour check
    std::vector<uint8_t> scanLv;           // per webcam pixel: the finest frequency level its position is known to (1..3)
    std::vector<uint8_t> scanSat;          // a frame was saturated here
    std::vector<float> scanBx, scanBy;     // result: beamer position (normalized) per webcam pixel, -1 = unknown
};

// what the stripe maps of the scan say about the webcams (see calibrate_cameras): the focal lengths (in pixels of the snapshots) of
// webcam 1 and 2 and the radial distortion of each (division model: undistorted = distorted / (1 + k r^2), r in focal lengths)
struct CamCal {
    bool ok = false;               // f1 and f2 are the self-calibrated ones
    double f1 = 0.0, f2 = 0.0;
    bool kok[2] = {false, false};  // k[i] was found
    double k[2] = {0.0, 0.0};
};

struct Tracer {
    CamCal cal;
    enum St { IDLE, BG_WAIT, REF_WAIT, VERIFY_WAIT, SCAN_WAIT } st = IDLE;
    bool scanned = false;                  // the scan is done for this run
    int scanIdx = 0;
    ScanData sd[2];                        // webcam 1 and webcam 2
    // how the webcam image moved between the snapshot that was segmented and the frames of the scan (autofocus changes the
    // magnification, and a webcam can shift): a webcam point p of the snapshot is at M p in the scan frames, M = (a b tx / c d ty)
    double camM[6] = {1, 0, 0, 0, 1, 0};   // before the scan
    double camM2[6] = {1, 0, 0, 0, 1, 0};  // after it
    double camMf[6] = {1, 0, 0, 0, 1, 0};  // used (the average of both, or the one there is)
    bool camMok = false, camM2ok = false;
    float scanG = 0.0f;                    // the brightness of the stripes of this scan (0 = the Exposure slider), see scan_brightness
    bool scan2 = false;                    // webcam 2 takes part in the scan
    int scanRetry = 0;                     // times the four frames of the current stripe period were taken again
    uint64_t cset2 = 0;                    // frame counter of webcam 2 when the pattern was changed
    std::vector<uint8_t> bg;
    bool haveBg = false;
    // the scene lit evenly white (at the brightness of the Exposure slider): the reference the stripe brightness is set from
    // (scan_brightness) and the webcam image motion since the snapshot is measured with (estimate_cam_motion)
    bool haveRef = false;
    std::vector<uint8_t> ref;
    int vseg = -1;   // the shape whose outline is being checked
    std::chrono::steady_clock::time_point tset;
    uint64_t cset = 0;
    bool blocked = false;
};

struct PM {
    bool inited = false;
    Param *cam = nullptr, *beam = nullptr, *knots = nullptr, *light = nullptr, *corner = nullptr, *preview = nullptr, *cam2 = nullptr, *method = nullptr, *fov = nullptr, *fov2 = nullptr;
    Boxx *segbox = nullptr, *promptbox = nullptr, *cancelbox = nullptr, *textbox = nullptr, *resolvebox = nullptr, *exposebox = nullptr, *cleanbox = nullptr;
    bool expose = false;   // the EXPOSE button is on: the beamer shows the exposure grey when it is not busy
    std::vector<int> dispidx;           // beamer option -> display index
    int lastcam2 = 0;
    int lastcam = 0, lastbeam = 0, lastknots = 0, lastcorner = 35, lastpreview = 0;

    // canvas
    float bx = 0, by = 0, bw = 1, bh = 1;

    // webcam
    CamCapture cap;
    CamCapture cap2;   // the optional second webcam (on the other side of the beamer): sees surfaces the first sees at a steep angle better
    std::vector<Seg> segs2, pendingSegs2;   // the segments of webcam 2 (matched to those of webcam 1 through the beamer)
    int sw2 = 0, sh2 = 0;
    GLuint camtex = (GLuint) -1;
    int camtexw = 0, camtexh = 0;
    uint64_t camtexcnt = 0;
    GLuint camtex2 = (GLuint) -1;       // the image of webcam 2 (the V button shows it, for pointing it)
    int camtexw2 = 0, camtexh2 = 0;
    uint64_t camtexcnt2 = 0;
    bool viewcam2 = false;              // the V button is on: the big window shows webcam 2, and nothing else
    Boxx *vbox = nullptr;

    // snapshot (taken when SEGMENT is clicked)
    std::vector<uint8_t> snap;
    int sw = 0, sh = 0;

    // selection box (normalized webcam coords)
    bool selvalid = false;
    float sel[4] = {0, 0, 1, 1};
    bool dragging = false;
    float dragu = 0, dragv = 0, dragpx = 0, dragpy = 0;
    bool prevdown = false;

    // prompt
    bool promptOn = false, promptEditing = false;
    std::string promptstr;
    std::vector<std::string> promptlines;

    // segmentation job
    SAMSegmentation *sam = nullptr;
    std::atomic<bool> running{false}, cancel{false};
    std::atomic<int> gen{0};
    std::mutex mtx;
    Step steps[6] = {{"SNAPSHOT"}, {"COMFYUI"}, {"SAM3"}, {"MASKS"}, {"AI DEPTH"}, {"TRACE"}};
    std::string status = "Select a webcam and a beamer, then click SEGMENT";
    float pct = 0.0f;
    std::vector<Seg> segs;
    std::vector<Seg> pendingSegs;
    std::vector<uint8_t> depthsnap;        // the snapshot the depth estimate was made from
    bool keepdepth = false;                // the depth estimate of the earlier snapshot is kept (the scene is as it was): not made again
    int nextsegid = 0;                   // the id the next segment gets: the segments of earlier segmentations stay, each keeps its own
    std::atomic<bool> segsReady{false}, segs2Ready{false};
    std::vector<float> vdiag;   // what the last outline check measured, 5 numbers per outline point: target x, y (webcam px), offset x, y (webcam px), offset along the outward normal
    std::shared_ptr<ScenePoints> scene;   // the point cloud of the AI depth of the snapshot (for the Viewpoint in the Mapping view)
    ScenePoints *sceneSent = nullptr;     // the one the Mapping view has been given
    std::atomic<float> mogefov2{0.0f};   // the horizontal field of view (radians) MoGe finds for the snapshot of webcam 2 (0: not known)
    std::vector<float> depth, pendingDepth;   // MoGe metric depth of the snapshot (metres, -1 invalid)
    int dw = 0, dh = 0, pdw = 0, pdh = 0;
    float dfx = 0.0f, pdfx = 0.0f;
    std::atomic<bool> depthReady{false};

    // overlay (dimming outside the box, selected segments, outlines)
    GLuint ovtex = (GLuint) -1;
    bool ovdirty = true;
    int ovw = 0, ovh = 0;

    // beamer
    EWindow *bwin = nullptr;
    bool borrowed = false;
    int prevmixid = 0;
    Layer *prevlay = nullptr;
    GLuint beamtex[2] = {(GLuint) -1, (GLuint) -1}, beamfbo[2] = {(GLuint) -1, (GLuint) -1};
    GLuint beamshow = (GLuint) -1;   // the texture the beamer window shows (the one finished last frame)
    int bidx = 0;
    int bdisp = -1;   // the display index of the beamer window (the Mapping view keeps the masks per beamer)
    bool beamok = false;
    int beamw = 0, beamh = 0;
    GLuint masktex = (GLuint) -1;   // (unused: every layer has its own, see groups)
    std::vector<Mapped> mapped;     // the resolved shapes: each projects its own layer, whatever the current layer is
    std::vector<GroupTex> groups;   // per layer in mapped: the mask of all its shapes
    GLuint tmptex = (GLuint) -1, tmpfbo = (GLuint) -1;   // a layer through its mask, before it is added to the beamer image
    GLuint scantex = (GLuint) -1;     // the stripe pattern of the scan
    GLuint whytex = (GLuint) -1;      // the map of the last scan painted over the webcam image (why a pixel has no position), Preview "Map"
    GLuint verifytex = (GLuint) -1;   // the outline of a shape, drawn for the check of where it really lands
    bool maskdirty = true;
    BeamMode bmode = BM_BLACK;
    float zoom = 1.0f, zu = 0.5f, zv = 0.5f;   // zoom of the webcam image in the big window (mouse wheel) and the centre it is zoomed on
    bool panning = false, mprev = false;   // panning the zoomed image with the middle mouse button
    float panmx = 0.0f, panmy = 0.0f;
    float fillcol[4] = {1, 1, 1, 1};   // BM_FILL
    Tracer tr;
    bool snapWait = false;   // waiting for the beamer to show white before the snapshot
    std::chrono::steady_clock::time_point snapT;
    uint64_t snapC = 0;
};

PM &P = *new PM;   // (never destroyed: the worker threads may outlive main)

float g_white[4] = {1, 1, 1, 1};
float g_transp[4] = {0, 0, 0, 0};
float g_lightblue[4] = {0.45f, 0.75f, 1.0f, 1.0f};
float g_orange[4] = {1.0f, 0.55f, 0.0f, 1.0f};
float g_lgreen[4] = {0.3f, 0.9f, 0.4f, 1.0f};
float g_bgdark[4] = {0.0f, 0.0f, 0.0f, 1.0f};
float g_greyb[4] = {0.25f, 0.25f, 0.25f, 1.0f};
float g_light[4] = {0.5f, 0.5f, 0.5f, 1.0f};   // the grey the beamer lights the scene with for the snapshot (the Exposure slider)

int knot_count() {
    return std::clamp((int) (P.knots->value + 0.5f), kMinKnots, kMaxKnots);
}

void set_status(const std::string &s) {
    std::lock_guard<std::mutex> lock(P.mtx);
    P.status = s;
}

void set_step(int i, int st, const std::string &txt = "", float pct = -1.0f) {
    std::lock_guard<std::mutex> lock(P.mtx);
    P.steps[i].state = st;
    if (!txt.empty()) P.status = txt;
    if (pct >= 0.0f) P.pct = pct;
}

Layer *cur_layer() {
    return mainmix->currlay[!mainprogram->prevmodus];
}

// ======================================================================================================
// Segmentation job (worker thread)
// ======================================================================================================

bool load_metric_depth(const std::string &path, std::vector<float> &depth, int &w, int &h, float &fx) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    char magic[4] = {0};
    int32_t B = 0, H = 0, W = 0;
    float fxx = 0.0f;
    f.read(magic, 4);
    f.read((char *) &B, 4);
    f.read((char *) &H, 4);
    f.read((char *) &W, 4);
    f.read((char *) &fxx, 4);
    if (!f || std::memcmp(magic, "MOGZ", 4) != 0 || B <= 0 || H <= 0 || W <= 0) return false;
    depth.resize((size_t) H * W);
    f.read((char *) depth.data(), (std::streamsize) (depth.size() * sizeof(float)));   // (first frame)
    if (!f && !f.eof()) return false;
    w = W;
    h = H;
    fx = fxx;
    return true;
}

void blur_box(std::vector<float> &a, int W, int H, int r);

// The outline traced along the pixel edges of a mask is a staircase: on an edge that is nearly vertical or horizontal but not
// quite, the steps are a pixel wide and many pixels apart, and no smoothing over a few points removes that (it comes back as an
// evenly spaced wobble in the projection). So every point of the outline is moved along its normal to where the slightly blurred
// mask crosses one half: the edge at sub-pixel precision, for any shape and any slope.
void refine_outline_subpixel(const Seg &s, int W, int H, std::vector<float> &dense) {
    const std::vector<uint8_t> &mask = s.mask;
    const int D = (int) dense.size() / 2;
    if (D < 8 || mask.size() < (size_t) W * H) return;
    const int pad = 8;
    const int rx0 = std::max(0, s.x0 - pad), ry0 = std::max(0, s.y0 - pad), rx1 = std::min(W - 1, s.x1 + pad), ry1 = std::min(H - 1, s.y1 + pad);
    const int w = rx1 - rx0 + 1, h = ry1 - ry0 + 1;
    if (w < 8 || h < 8) return;
    std::vector<float> f((size_t) w * h);
    // after the refinement there is a soft mask (the share of the SAM3 runs that had the pixel): that is already an edge at
    // sub-pixel precision
    const bool soft = s.soft.size() >= (size_t) W * H;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            const size_t i = (size_t) (y + ry0) * W + (x + rx0);
            float v;
            if (soft) v = (float) s.soft[i] / 255.0f;
            else v = mask[i] > 128 ? 1.0f : 0.0f;
            f[(size_t) y * w + x] = v;
        }
    }
    blur_box(f, w, h, 1);
    if (!soft) blur_box(f, w, h, 1);
    auto val = [&](double px, double py) -> double {   // (bilinear, pixel centres at +0.5, in snapshot pixels)
        double fx = px - 0.5 - rx0, fy = py - 0.5 - ry0;
        int ix = (int) std::floor(fx), iy = (int) std::floor(fy);
        double tx = fx - ix, ty = fy - iy;
        auto at = [&](int x, int y) { return f[(size_t) std::clamp(y, 0, h - 1) * w + std::clamp(x, 0, w - 1)]; };
        return (1 - ty) * ((1 - tx) * at(ix, iy) + tx * at(ix + 1, iy)) + ty * ((1 - tx) * at(ix, iy + 1) + tx * at(ix + 1, iy + 1));
    };
    std::vector<double> off(D, 0.0), nx(D, 0.0), ny(D, 0.0);
    std::vector<char> ok(D, 0);
    for (int i = 0; i < D; i++) {
        const int a = (i - 2 + D) % D, b = (i + 2) % D;
        double tx = (dense[b * 2] - dense[a * 2]) * W, ty = (dense[b * 2 + 1] - dense[a * 2 + 1]) * H;
        double l = std::hypot(tx, ty);
        if (l < 1e-6) continue;
        nx[i] = ty / l;
        ny[i] = -tx / l;
        const double px = dense[i * 2] * W, py = dense[i * 2 + 1] * H;
        // the crossing of one half nearest to the traced edge
        double best = 1e9;
        double prevt = -3.0, prevv = val(px + nx[i] * prevt, py + ny[i] * prevt) - 0.5;
        for (double t = -2.75; t <= 3.0001; t += 0.25) {
            double v = val(px + nx[i] * t, py + ny[i] * t) - 0.5;
            if ((prevv < 0.0) != (v < 0.0)) {
                double tc = prevt + (t - prevt) * (prevv / (prevv - v));
                if (std::fabs(tc) < std::fabs(best)) best = tc;
            }
            prevt = t;
            prevv = v;
        }
        if (std::fabs(best) <= 3.0) { off[i] = best; ok[i] = 1; }
    }
    // (the offsets a little smoothed along the outline, then applied)
    std::vector<double> so(off);
    for (int i = 0; i < D; i++) {
        if (!ok[i]) continue;
        double s = 2.0 * off[i], c = 2.0;
        int a = (i - 1 + D) % D, b = (i + 1) % D;
        if (ok[a]) { s += off[a]; c += 1.0; }
        if (ok[b]) { s += off[b]; c += 1.0; }
        so[i] = s / c;
    }
    for (int i = 0; i < D; i++) {
        if (!ok[i]) continue;
        dense[i * 2] += (float) (nx[i] * so[i] / W);
        dense[i * 2 + 1] += (float) (ny[i] * so[i] / H);
    }
}

// SAM3 masks (of a crop at (OX, OY), size CW x CH, of a snapshot W x H) -> segments: mask in snapshot size, bounding box, boundary
// pixels, the evenly spaced closed outline and (for N > 0) the knots on it. camidx: which webcam the snapshot is from.
std::vector<Seg> build_segs(const SegmentationResult &res, int W, int H, int CW, int CH, int OX, int OY, int camidx, int N) {
    std::vector<Seg> segs;
    fprintf(stderr, "[ProjMap] SAM3 returned %zu masks (%dx%d), crop %dx%d%s\n", res.masks.size(), res.width, res.height, CW, CH, camidx ? " (webcam 2)" : "");
    for (size_t m = 0; m < res.masks.size() && (int) segs.size() < kMaxSegs; m++) {
        const std::vector<uint8_t> &src = res.masks[m].mask;
        if (src.empty() || res.width <= 0 || res.height <= 0) continue;
        if (!camidx) set_step(3, 1, "Building outlines: mask " + std::to_string(m + 1) + "/" + std::to_string(res.masks.size()), 65.0f);
        Seg s;
        s.mask.assign((size_t) W * H, 0);
        s.x0 = W; s.y0 = H; s.x1 = -1; s.y1 = -1;
        int count = 0;
        for (int y = 0; y < CH; y++) {
            int sy = res.height == CH ? y : std::min(res.height - 1, y * res.height / CH);
            for (int x = 0; x < CW; x++) {
                int sx = res.width == CW ? x : std::min(res.width - 1, x * res.width / CW);
                size_t si = (size_t) sy * res.width + sx;
                if (si < src.size() && src[si] > 128) {
                    s.mask[(size_t) (y + OY) * W + (x + OX)] = 255;
                    s.x0 = std::min(s.x0, x + OX); s.x1 = std::max(s.x1, x + OX);
                    s.y0 = std::min(s.y0, y + OY); s.y1 = std::max(s.y1, y + OY);
                    count++;
                }
            }
        }
        fprintf(stderr, "[ProjMap] mask %zu: %d px\n", m, count);
        if (count < (int) (200.0f * cam_scale(W) * cam_scale(W))) continue;   // (too small to be a shape: 200 pixels on a 2560 px wide image)
        std::vector<float> raw;
        if (!trace_outer(s.mask, W, H, s.x0, s.y0, s.x1, s.y1, raw)) continue;
        smooth_resample(raw, 240, W, H, s.dense);
        refine_outline_subpixel(s, W, H, s.dense);
        for (int y = s.y0; y <= s.y1; y++) {
            for (int x = s.x0; x <= s.x1; x++) {
                size_t i = (size_t) y * W + x;
                if (s.mask[i] < 128) continue;
                bool b = x == 0 || y == 0 || x == W - 1 || y == H - 1 || s.mask[i - 1] < 128 || s.mask[i + 1] < 128 ||
                         s.mask[i - W] < 128 || s.mask[i + W] < 128;
                if (b) s.edge.push_back((uint32_t) i);
            }
        }
        s.id = (int) segs.size();
        hsv2rgb(std::fmod(0.137f + 0.381966f * (float) s.id, 1.0f), 0.85f, 1.0f, s.col);
        s.camidx = camidx;
        if (N > 0) {
            make_knots(s.dense, N, s.kcam, s.kcorner);
            s.kbeam.assign(s.kcam.size(), 0.0f);
            s.kdone.assign(s.kcam.size() / 2, 0);
        }
        segs.push_back(std::move(s));
    }
    return segs;
}

void worker_body(std::vector<uint8_t> snap, int W, int H, bool hasBox, int cx0, int cy0, int cx1, int cy1,
                 std::string prompt, int N, int gen, std::vector<uint8_t> snap2, int W2, int H2) {
    auto cancelled = [&]() { return P.cancel.load() || gen != P.gen.load(); };
    auto fail = [&](int step, const std::string &msg) {
        set_step(step, 3, msg);
        P.running = false;
    };

    // ---- 0. snapshot files ----
    std::string fullPath = mainprogram->temppath + "/projmap_snapshot.png";
    std::string cropPath = fullPath;
    set_step(0, 1, "Saving snapshot...", 2.0f);
    double bsum = 0.0;
    for (size_t i = 0; i + 3 < snap.size(); i += 4 * 16) bsum += 0.3 * snap[i] + 0.59 * snap[i + 1] + 0.11 * snap[i + 2];
    int bright = (int) (bsum / std::max<size_t>(1, snap.size() / 64));
    fprintf(stderr, "[ProjMap] snapshot %dx%d, mean brightness %d/255\n", W, H, bright);
    if (!ImageLoader::saveImagePNG(fullPath, snap.data(), W, H, 4)) return fail(0, "Could not save the snapshot");
    int CW = W, CH = H, OX = 0, OY = 0;
    if (hasBox) {
        OX = cx0; OY = cy0; CW = cx1 - cx0; CH = cy1 - cy0;
        std::vector<uint8_t> crop((size_t) CW * CH * 4);
        for (int y = 0; y < CH; y++) std::memcpy(&crop[(size_t) y * CW * 4], &snap[((size_t) (y + OY) * W + OX) * 4], (size_t) CW * 4);
        cropPath = mainprogram->temppath + "/projmap_selection.png";
        if (!ImageLoader::saveImagePNG(cropPath, crop.data(), CW, CH, 4)) return fail(0, "Could not save the selection");
    }
    set_step(0, 2);

    // ---- 1. ComfyUI ----
    set_step(1, 1, "Starting ComfyUI server...", 5.0f);
    if (!startComfyUIServer([&](const std::string &s) { set_status(s); })) return fail(1, "Failed to start ComfyUI server");
    P.sam->initialize("127.0.0.1", 8188);
    bool connected = false;
    for (int retry = 0; retry < 300 && !cancelled(); retry++) {
        if (!P.sam->testConnection().empty()) { connected = true; break; }
        set_status("Waiting for ComfyUI server... (" + std::to_string(retry + 1) + "s)");
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    if (cancelled()) { P.running = false; return; }
    if (!connected) return fail(1, "ComfyUI server not responding");
    set_step(1, 2);

    // ---- 2. SAM3 ----
    set_step(2, 1, "Segmenting...", 10.0f);
    P.sam->scoreThreshold = 0.2f;
    if (!P.sam->segmentFrame(cropPath, prompt)) return fail(2, "Could not start SAM3");
    bool sentcancel = false;
    while (P.sam->isProcessing()) {
        if (cancelled() && !sentcancel) { P.sam->cancelSegmentation(); sentcancel = true; }
        set_step(2, 1, "SAM3: " + P.sam->getStatus(), 10.0f + P.sam->getProgress() * 0.5f);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    if (cancelled()) { P.running = false; return; }
    SegmentationResult res = P.sam->getResult();
    if (res.masks.empty()) return fail(2, "SAM3 found no segments");
    set_step(2, 2);

    // ---- 3. masks -> segments, outlines ----
    set_step(3, 1, "Building outlines...", 65.0f);
    std::vector<Seg> segs = build_segs(res, W, H, CW, CH, OX, OY, 0, N);
    if (cancelled()) { P.running = false; return; }
    if (segs.empty()) return fail(3, "No usable segments (snapshot brightness " + std::to_string(bright) + "/255: is the scene lit?)");
    {
        std::lock_guard<std::mutex> lock(P.mtx);
        P.pendingSegs = std::move(segs);
    }
    P.segsReady = true;
    set_step(3, 2, "Segments ready: click them to select, the depth is being estimated...", 75.0f);

    // ---- 3b. webcam 2 (optional): its own segmentation, matched to the first one later through the beamer ----
    if (!snap2.empty() && W2 > 0 && !cancelled()) {
        set_status("Segmenting webcam 2...");
        std::string path2 = mainprogram->temppath + "/projmap_snapshot2.png";
        if (ImageLoader::saveImagePNG(path2, snap2.data(), W2, H2, 4) && P.sam->segmentFrame(path2, prompt)) {
            bool sentcancel2 = false;
            while (P.sam->isProcessing()) {
                if (cancelled() && !sentcancel2) { P.sam->cancelSegmentation(); sentcancel2 = true; }
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
            if (!cancelled()) {
                SegmentationResult res2 = P.sam->getResult();
                std::vector<Seg> segs2 = build_segs(res2, W2, H2, W2, H2, 0, 0, 1, 0);
                fprintf(stderr, "[ProjMap] webcam 2: %zu segments\n", segs2.size());
                {
                    std::lock_guard<std::mutex> lock(P.mtx);
                    P.pendingSegs2 = std::move(segs2);
                    P.sw2 = W2; P.sh2 = H2;
                }
                P.segs2Ready = true;
            }
        }
        else fprintf(stderr, "[ProjMap] webcam 2: could not be segmented\n");
    }

    // ---- 4. MoGe depth (non-fatal) ----
    if (P.keepdepth) {
        set_step(4, 2, "Depth kept: the scene is as it was", 100.0f);
        P.running = false;
        return;
    }
    set_step(4, 1, "Estimating depth (AI Depth)...", 80.0f);
    std::string metricPath = mainprogram->temppath + "/projmap_depth.bin";
    if (P.sam->estimateDepth(fullPath, metricPath) && !cancelled()) {
        std::vector<float> d;
        int dw = 0, dh = 0;
        float fx = 0.0f;
        if (load_metric_depth(metricPath, d, dw, dh, fx)) {
            {
                std::lock_guard<std::mutex> lock(P.mtx);
                P.pendingDepth = std::move(d);
                P.pdw = dw; P.pdh = dh; P.pdfx = fx;
            }
            P.depthReady = true;
            set_step(4, 2, "Ready", 100.0f);
        } else {
            set_step(4, 3, "Depth file unreadable (continuing without depth)");
        }
    } else if (!cancelled()) {
        set_step(4, 3, "Depth estimation failed (continuing without depth)");
    }
    // ---- 4b. webcam 2: MoGe finds its field of view too (the depth itself is not used: only for the planes from two webcams) ----
    if (!snap2.empty() && W2 > 0 && !cancelled()) {
        std::string path2 = mainprogram->temppath + "/projmap_snapshot2.png";
        std::string metricPath2 = mainprogram->temppath + "/projmap_depth2.bin";
        std::error_code ec;
        if (fs::exists(path2, ec) && P.sam->estimateDepth(path2, metricPath2) && !cancelled()) {
            std::vector<float> d2;
            int dw2 = 0, dh2 = 0;
            float fx2 = 0.0f;
            if (load_metric_depth(metricPath2, d2, dw2, dh2, fx2) && fx2 > 0.0f && dw2 > 0) {
                P.mogefov2 = (float) (2.0 * std::atan(0.5 * (double) dw2 / (double) fx2));
                fprintf(stderr, "[ProjMap] webcam 2: AI Depth finds a field of view of %.1f degrees\n", (double) P.mogefov2.load() * 180.0 / 3.14159265358979);
            }
        }
    }
    P.running = false;
}

void worker_main(std::vector<uint8_t> snap, int W, int H, bool hasBox, int cx0, int cy0, int cx1, int cy1,
                 std::string prompt, int N, int gen, std::vector<uint8_t> snap2, int W2, int H2) {
    try {
        worker_body(std::move(snap), W, H, hasBox, cx0, cy0, cx1, cy1, prompt, N, gen, std::move(snap2), W2, H2);
    } catch (const std::exception &e) {
        fprintf(stderr, "[ProjMap] worker exception: %s\n", e.what());
        set_step(3, 3, std::string("Error: ") + e.what());
        P.running = false;
    }
}

void start_segmentation(bool lit = false) {
    if (P.running.load() || P.cap.state != 2) return;
    if (P.bwin && !lit) {
        // the beamer is busy (scan / projection): light the scene first, the snapshot follows in projmap_ui()
        P.tr.st = Tracer::IDLE;
        P.bmode = BM_WHITE;
        P.snapWait = true;
        P.snapT = std::chrono::steady_clock::now();
        P.snapC = P.cap.counter.load();
        set_status("Lighting the scene...");
        return;
    }
    std::vector<uint8_t> frame;
    int W = 0, H = 0;
    if (!P.cap.grab(frame, W, H)) return;
    if (!P.sam) {
        P.sam = new SAMSegmentation();
        P.sam->initialize();
    }
    // The depth estimate is made from the whole snapshot, whatever is segmented, so it is still good when the scene and the
    // webcam are as they were for the snapshot it was made from: it is kept then (and not made again, which takes a while and
    // would give a slightly different depth for the outlines of this segmentation).
    bool keepdepth = false;
    if (!P.depth.empty() && !P.depthsnap.empty() && P.depthsnap.size() == frame.size()) {
        double diff = 0.0;
        size_t cnt = 0;
        for (size_t i = 0; i + 2 < frame.size(); i += 4 * 7) {
            diff += std::abs((int) frame[i] - (int) P.depthsnap[i]) + std::abs((int) frame[i + 1] - (int) P.depthsnap[i + 1]) +
                    std::abs((int) frame[i + 2] - (int) P.depthsnap[i + 2]);
            cnt += 3;
        }
        keepdepth = cnt > 0 && diff / (double) cnt < 4.0;   // (mean difference under 4 of 255: camera noise, not a moved webcam or object)
        fprintf(stderr, "[ProjMap] snapshot differs from the one the depth was made from by %.2f/255: depth %s\n", cnt ? diff / (double) cnt : 0.0, keepdepth ? "kept" : "made again");
    }
    P.keepdepth = keepdepth;
    if (!keepdepth) P.depthsnap = frame;   // (the snapshot the depth is made from)
    P.snap = frame;
    P.sw = W;
    P.sh = H;
    // (webcam 2, when it is on: its snapshot at the same moment, with the same light)
    std::vector<uint8_t> frame2;
    int W2 = 0, H2 = 0;
    if (P.cap2.state == 2 && !P.cap2.grab(frame2, W2, H2)) frame2.clear();
    {
        std::lock_guard<std::mutex> lock(P.mtx);
        for (Step &s : P.steps) s.state = 0;
        P.pct = 0.0f;
        P.pendingSegs2.clear();
    }
    // The segments of the earlier segmentations stay (the new ones are added to them, see projmap_tick): several cropped
    // segmentations can be followed by one RESOLVE. Only when the webcam image has another size they cannot be kept.
    {
        bool same = true;
        for (const Seg &s : P.segs) if (s.mask.size() != (size_t) W * H) same = false;
        if (!same) { P.segs.clear(); P.nextsegid = 0; }
        if (!frame2.empty()) {
            for (const Seg &s : P.segs2) if (s.mask.size() != (size_t) W2 * H2) { P.segs2.clear(); break; }
        }
    }
    P.segs2Ready = false;
    if (!keepdepth) {
        P.depth.clear();
        P.mogefov2 = 0.0f;
    }
    P.segsReady = false;
    P.depthReady = false;
    P.tr.st = Tracer::IDLE;
    P.tr.blocked = false;
    P.tr.haveRef = false;   // (the reference frames, and the motion of the webcam image since the snapshot, are taken again for this one)
    P.tr.camMok = false;
    P.bmode = BM_BLACK;
    P.ovdirty = P.maskdirty = true;

    bool hasBox = false;
    int cx0 = 0, cy0 = 0, cx1 = W, cy1 = H;
    if (P.selvalid) {
        cx0 = std::clamp((int) (P.sel[0] * W), 0, W - 1);
        cy0 = std::clamp((int) (P.sel[1] * H), 0, H - 1);
        cx1 = std::clamp((int) (P.sel[2] * W), 0, W);
        cy1 = std::clamp((int) (P.sel[3] * H), 0, H);
        hasBox = (cx1 - cx0) >= 32 && (cy1 - cy0) >= 32;
    }
    std::string prompt = (P.promptOn && !P.promptstr.empty()) ? P.promptstr : kDefaultPrompt;
    P.cancel = false;
    P.running = true;
    int gen = ++P.gen;
    std::thread(worker_main, std::move(frame), W, H, hasBox, cx0, cy0, cx1, cy1, prompt, knot_count(), gen, std::move(frame2), W2, H2).detach();
}

// ======================================================================================================
// Beamer window
// ======================================================================================================

void close_beamer() {
    if (!P.bwin) return;
    OutputEntry *entry = nullptr;
    for (OutputEntry *e : mainprogram->outputentries) if (e->win == P.bwin) entry = e;
    if (P.borrowed) {
        P.bwin->mixid = P.prevmixid;
        P.bwin->lay = P.prevlay;
    }
    else if (entry) {
        P.bwin->lay = nullptr;
        SDL_GL_MakeCurrent(P.bwin->win, NULL);
        glFinish();
        SDL_GL_DestroyContext(P.bwin->glc);
        SDL_HideWindow(P.bwin->win);
        SDL_Delay(10);
        mainprogram->outputentries.erase(std::find(mainprogram->outputentries.begin(), mainprogram->outputentries.end(), entry));
        mainprogram->mixwindows.erase(std::remove(mainprogram->mixwindows.begin(), mainprogram->mixwindows.end(), P.bwin),
                                      mainprogram->mixwindows.end());
        maskmap_hide_pointer(P.bwin->win, false);
        SDL_SetWindowFullscreen(P.bwin->win, false);   // (the pool gives the window to other outputs too)
        mainprogram->add_to_winpool(P.bwin->win, P.bwin->w, P.bwin->h);
        delete P.bwin;
        delete entry;
        SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);   // (the main thread had no context left after the one above)
    }
    P.bwin = nullptr;
    P.borrowed = false;
    P.bmode = BM_BLACK;
    P.tr.st = Tracer::IDLE;
}

// Two beamer textures, used alternately: the output window's thread may still be reading the one shown last frame
// while the next frame is drawn into the other one (a single texture gave random flicker, depending on timing).
void make_beam_target(int w, int h) {
    if (P.beamfbo[0] != (GLuint) -1 && P.beamw == w && P.beamh == h) return;
    P.beamok = true;
    for (int i = 0; i < 2; i++) {
        if (P.beamfbo[i] != (GLuint) -1) glDeleteFramebuffers(1, &P.beamfbo[i]);
        if (P.beamtex[i] != (GLuint) -1) {
            mainprogram->texsizemap.erase(P.beamtex[i]);
            mainprogram->texintfmap.erase(P.beamtex[i]);
            glDeleteTextures(1, &P.beamtex[i]);
        }
        glGenTextures(1, &P.beamtex[i]);
        glBindTexture(GL_TEXTURE_2D, P.beamtex[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER_COMPAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER_COMPAT);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, w, h);
        mainprogram->texintfmap[P.beamtex[i]] = GL_RGBA8;
        mainprogram->texsizemap[P.beamtex[i]] = {w, h};
        glGenFramebuffers(1, &P.beamfbo[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, P.beamfbo[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, P.beamtex[i], 0);
        GLenum fbst = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (fbst != GL_FRAMEBUFFER_COMPLETE) P.beamok = false;
        fprintf(stderr, "[ProjMap] beamer FBO %dx%d tex %u fbo %u status 0x%x\n", w, h, P.beamtex[i], P.beamfbo[i], (unsigned) fbst);
    }
    // the scratch image a layer is drawn through its mask in (only the main thread uses it: one is enough)
    if (P.tmpfbo != (GLuint) -1) glDeleteFramebuffers(1, &P.tmpfbo);
    if (P.tmptex != (GLuint) -1) {
        mainprogram->texsizemap.erase(P.tmptex);
        mainprogram->texintfmap.erase(P.tmptex);
        glDeleteTextures(1, &P.tmptex);
    }
    glGenTextures(1, &P.tmptex);
    glBindTexture(GL_TEXTURE_2D, P.tmptex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER_COMPAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER_COMPAT);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, w, h);
    mainprogram->texintfmap[P.tmptex] = GL_RGBA8;
    mainprogram->texsizemap[P.tmptex] = {w, h};
    glGenFramebuffers(1, &P.tmpfbo);
    glBindFramebuffer(GL_FRAMEBUFFER, P.tmpfbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, P.tmptex, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        glDeleteFramebuffers(1, &P.tmpfbo);
        P.tmpfbo = (GLuint) -1;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    P.beamshow = P.beamtex[0];
    P.bidx = 0;
    P.beamw = w;
    P.beamh = h;
    P.maskdirty = true;
}

// (same window set-up as the monitor menu's "Show on display")
void open_beamer(int disp) {
    close_beamer();
    P.bdisp = disp;
    for (OutputEntry *e : mainprogram->outputentries) {
        if (e->screen == disp) {
            P.bwin = e->win;
            P.borrowed = true;
            P.prevmixid = e->win->mixid;
            P.prevlay = e->win->lay;
            e->win->mixid = 5;
            e->win->lay = nullptr;
            make_beam_target(e->win->w, e->win->h);
            return;
        }
    }
    EWindow *mwin = new EWindow;
    mwin->mixid = 5;
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
    {
        const char *dn = SDL_GetDisplayName(EWOC_DisplayIndexToID(disp));
        fprintf(stderr, "[ProjMap] beamer on display %d (%s): %dx%d at (%d,%d); usable area %dx%d -> beamer image %dx%d\n", disp,
                dn ? dn : "?", rc1.w, rc1.h, rc1.x, rc1.y, rc2.w, rc2.h, mwin->w, mwin->h);
    }
    SDL_GL_MakeCurrent(mainprogram->mainwindow, glc);
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
    mwin->win = mainprogram->grab_from_winpool(mwin->w, mwin->h);
    if (mwin->win == nullptr) {
        // created AT the beamer's position: a maximized window created first and moved afterwards stays on the
        // display it was created on (the laptop)
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
    P.bwin = mwin;
    P.borrowed = false;
    make_beam_target(mwin->w, mwin->h);
}

// ======================================================================================================
// Beamer image
// ======================================================================================================

// The beamer knots of a segment as they are used for the mask. Every knot was found to about a pixel, which in the
// beamer is a couple of pixels of noise sideways: a Catmull-Rom curve through noisy knots on a straight edge waves.
// So where the outline between two corners is straight in the webcam image (the knots between them lie within a few
// pixels of the chord), the knots in between are moved onto the best fit line through them, and each corner is moved to
// where the fitted lines of its two edges meet (the corner knot itself was found just as noisily, and there are three
// knots on every edge to average that away). Curved stretches, and anything that does not look sound, stay as measured.
std::vector<float> straighten_knots(const Seg &s) {
    std::vector<float> kb = s.kbeam;
    std::vector<float> out = kb;
    if (true) return out;   // (switched off: it only worked on simple shapes; the knots stay as measured)
    const int n = (int) s.kdone.size();
    if (n < 4 || (int) kb.size() < n * 2 || (int) s.kcam.size() < n * 2 || (int) s.kcorner.size() < n) return out;
    std::vector<int> cs;
    for (int i = 0; i < n; i++) if (s.kcorner[i]) cs.push_back(i);
    const int nc = (int) cs.size();
    if (nc < 2) return out;
    const double bsx = P.beamw > 0 ? P.beamw : 1920.0, bsy = P.beamh > 0 ? P.beamh : 1080.0;   // (beamer pixels)
    const double csx = P.sw > 0 ? P.sw : 1280.0, csy = P.sh > 0 ? P.sh : 720.0;               // (webcam pixels)

    struct Line { bool ok = false; double px = 0, py = 0, dx = 1, dy = 0; };
    std::vector<Line> lines(nc);
    std::vector<std::vector<int>> inner(nc);
    for (int j = 0; j < nc; j++) {
        int a = cs[j], b = cs[(j + 1) % nc];
        for (int i = (a + 1) % n; i != b; i = (i + 1) % n) inner[j].push_back(i);
        // straight in the webcam image?
        double ax = s.kcam[a * 2] * csx, ay = s.kcam[a * 2 + 1] * csy, bx = s.kcam[b * 2] * csx, by = s.kcam[b * 2 + 1] * csy;
        double L = std::hypot(bx - ax, by - ay);
        if (L < 8.0) continue;
        double maxdev = 0.0;
        for (int i : inner[j]) {
            double x = s.kcam[i * 2] * csx, y = s.kcam[i * 2 + 1] * csy;
            maxdev = std::max(maxdev, std::fabs((bx - ax) * (ay - y) - (ax - x) * (by - ay)) / L);
        }
        if (maxdev > 3.0) continue;   // curved
        Line &ln = lines[j];
        if (inner[j].size() >= 2) {
            // total least squares line through the knots in between
            double mx = 0, my = 0;
            for (int i : inner[j]) { mx += kb[i * 2] * bsx; my += kb[i * 2 + 1] * bsy; }
            mx /= inner[j].size(); my /= inner[j].size();
            double sxx = 0, syy = 0, sxy = 0;
            for (int i : inner[j]) {
                double dx = kb[i * 2] * bsx - mx, dy = kb[i * 2 + 1] * bsy - my;
                sxx += dx * dx; syy += dy * dy; sxy += dx * dy;
            }
            double th = 0.5 * std::atan2(2.0 * sxy, sxx - syy);
            ln.px = mx; ln.py = my; ln.dx = std::cos(th); ln.dy = std::sin(th);
            // (the knots in between must really be spread along it, else the direction means nothing)
            if (std::sqrt(sxx + syy) < 5.0) continue;
        }
        else {
            ln.px = kb[a * 2] * bsx; ln.py = kb[a * 2 + 1] * bsy;
            double dx = kb[b * 2] * bsx - ln.px, dy = kb[b * 2 + 1] * bsy - ln.py;
            double l = std::hypot(dx, dy);
            if (l < 1e-6) continue;
            ln.dx = dx / l; ln.dy = dy / l;
        }
        ln.ok = true;
    }
    // the knots in between, onto their line (their place along it stays)
    for (int j = 0; j < nc; j++) {
        if (!lines[j].ok) continue;
        for (int i : inner[j]) {
            double x = kb[i * 2] * bsx, y = kb[i * 2 + 1] * bsy;
            double t = (x - lines[j].px) * lines[j].dx + (y - lines[j].py) * lines[j].dy;
            out[i * 2] = (float) ((lines[j].px + t * lines[j].dx) / bsx);
            out[i * 2 + 1] = (float) ((lines[j].py + t * lines[j].dy) / bsy);
        }
    }
    // the corners, where the lines of their two edges meet
    for (int j = 0; j < nc; j++) {
        const Line &l1 = lines[(j - 1 + nc) % nc], &l2 = lines[j];
        if (!l1.ok || !l2.ok) continue;
        double den = l1.dx * l2.dy - l1.dy * l2.dx;
        if (std::fabs(den) < 0.42) continue;   // (edges nearly parallel: no clear corner)
        double t = ((l2.px - l1.px) * l2.dy - (l2.py - l1.py) * l2.dx) / den;
        double ix = l1.px + t * l1.dx, iy = l1.py + t * l1.dy;
        int c = cs[j];
        double mx = kb[c * 2] * bsx, my = kb[c * 2 + 1] * bsy;
        if (std::hypot(ix - mx, iy - my) > 15.0) continue;   // (far from what was measured: do not trust it)
        out[c * 2] = (float) (ix / bsx);
        out[c * 2 + 1] = (float) (iy / bsy);
    }
    return out;
}

bool layer_alive(Layer *l) {
    if (!l) return false;
    for (auto &sm : mainmix->layers) {
        if (std::find(sm.begin(), sm.end(), l) != sm.end()) return true;
    }
    return false;
}

// a mapped shape whose layer has been closed goes away with it
void prune_mapped() {
    bool changed = false;
    for (size_t i = 0; i < P.mapped.size();) {
        if (P.mapped[i].lay && !layer_alive(P.mapped[i].lay)) {   // (resolving assigns no layer: that is done in the Mapping view)
            P.mapped.erase(P.mapped.begin() + i);
            changed = true;
            // (the shapes of the current run that pointed at it: they are not resolved any more)
            for (Seg &s : P.segs) {
                if (s.mapidx == (int) i) { s.resolved = false; s.mapidx = -1; s.lay = nullptr; }
                else if (s.mapidx > (int) i) s.mapidx--;
            }
        }
        else i++;
    }
    if (changed) { P.maskdirty = true; P.ovdirty = true; }
}

// fills the polygon (closed outline in normalized coordinates) into the coverage image (even-odd scanline fill)
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

// One mask texture per layer that has resolved shapes: black everywhere outside its shapes (alpha = 255 - coverage), or,
// in the outline preview, only the edge of its shapes as a white line.
void rebuild_masks() {
    prune_mapped();
    std::vector<Layer *> lays;
    for (const Mapped &m : P.mapped) {
        if (std::find(lays.begin(), lays.end(), m.lay) == lays.end()) lays.push_back(m.lay);
    }
    for (size_t i = lays.size(); i < P.groups.size(); i++) {
        if (P.groups[i].masktex != (GLuint) -1) glDeleteTextures(1, &P.groups[i].masktex);
    }
    P.groups.resize(lays.size());
    const int mw = std::max(64, P.beamw > 2560 ? P.beamw / 2 : P.beamw), mh = std::max(64, P.beamw > 2560 ? P.beamh / 2 : P.beamh);   // (full beamer resolution: the edge of the mask is as fine as the beamer can show it)
    const bool outline = (int) (P.preview->value + 0.5f) == 0;
    for (size_t g = 0; g < lays.size(); g++) {
        GroupTex &gt = P.groups[g];
        gt.lay = lays[g];
        std::vector<uint8_t> cov((size_t) mw * mh, 0);
        for (const Mapped &m : P.mapped) {
            if (m.lay != lays[g]) continue;
            std::vector<float> pts;
            closed_curve(m.kb, m.kcorner, 12, pts);
            fill_polygon(cov, mw, mh, pts);
        }
        std::vector<uint8_t> rgba((size_t) mw * mh * 4, 0);
        if (outline) {
            for (int y = 0; y < mh; y++) {
                for (int x = 0; x < mw; x++) {
                    // (one pixel thin, on the inside of the mask: a line on both sides of the edge would show the
                    // projection outside the shape)
                    bool edge = false;
                    if (cov[(size_t) y * mw + x]) {
                        const int nx[4] = {-1, 1, 0, 0}, ny[4] = {0, 0, -1, 1};
                        for (int d = 0; d < 4 && !edge; d++) {
                            int xx = x + nx[d], yy = y + ny[d];
                            if (xx < 0 || yy < 0 || xx >= mw || yy >= mh || !cov[(size_t) yy * mw + xx]) edge = true;
                        }
                    }
                    if (edge) {
                        size_t o = ((size_t) y * mw + x) * 4;
                        rgba[o] = rgba[o + 1] = rgba[o + 2] = rgba[o + 3] = 255;
                    }
                }
            }
        }
        else {
            for (size_t i = 0; i < cov.size(); i++) rgba[i * 4 + 3] = (uint8_t) (255 - cov[i]);
        }
        if (gt.masktex == (GLuint) -1) {
            glGenTextures(1, &gt.masktex);
            glBindTexture(GL_TEXTURE_2D, gt.masktex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
        glBindTexture(GL_TEXTURE_2D, gt.masktex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA_INTERNAL, mw, mh, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    P.maskdirty = false;
}

// In this app every texture has row 0 = top of the picture, and drawing into an FBO is mirrored: textures (layer or
// CPU) are drawn into the beamer FBO with a negative height, plain shapes at the mirrored y (2 * y - 1 for y down
// in 0..1).
void render_beam() {
    if (P.beamfbo[P.bidx] == (GLuint) -1 || !P.bwin || !P.beamok) return;   // (never draw when the FBO is not bound: that would paint the main window)
    if (P.bmode == BM_PROJECT && P.maskdirty) rebuild_masks();

    bool budm = mainprogram->directmode;
    mainprogram->directmode = true;
    glBindFramebuffer(GL_FRAMEBUFFER, P.beamfbo[P.bidx]);
    glDrawBuffer_FBO();
    glViewport(0, 0, P.beamw, P.beamh);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    mainprogram->uniformCache->setInt("interm", 0);
    mainprogram->uniformCache->setInt("mixmode", 0);
    mainprogram->uniformCache->setBool("usemask", false);

    if (P.bmode == BM_WHITE) {
        g_light[0] = g_light[1] = g_light[2] = P.light->value;
        draw_direct(nullptr, g_light, -1.0f, -1.0f, 2.0f, 2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, -1, P.beamw, P.beamh, false, false);
    }
    else if (P.bmode == BM_FILL) {
        draw_direct(nullptr, P.fillcol, -1.0f, -1.0f, 2.0f, 2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, -1, P.beamw, P.beamh, false, false);
    }
    else if (P.bmode == BM_PATTERN) {
        if (P.scantex != (GLuint) -1)
            draw_direct(nullptr, g_transp, -1.0f, 1.0f, 2.0f, -2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, P.scantex, P.beamw, P.beamh, false, false);
    }
    else if (P.bmode == BM_VERIFY) {
        if (P.verifytex != (GLuint) -1)
            draw_direct(nullptr, g_transp, -1.0f, 1.0f, 2.0f, -2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, P.verifytex, P.beamw, P.beamh, false, false);
    }
    else if (P.bmode == BM_PROJECT) {
        // every layer that has resolved shapes is projected on those, on its own and whatever the current layer is:
        // layer and mask in a scratch image (black outside the shapes), which is then added to the beamer image
        const bool outline = (int) (P.preview->value + 0.5f) == 0;
        for (GroupTex &g : P.groups) {
            if (g.masktex == (GLuint) -1) continue;
            if (outline) {
                draw_direct(nullptr, g_transp, -1.0f, 1.0f, 2.0f, -2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, g.masktex, P.beamw, P.beamh, false, false);
                continue;
            }
            Layer *lay = g.lay;
            if (!layer_alive(lay) || P.tmpfbo == (GLuint) -1) continue;
            // (the layer as the mix shows it, like the layer warp canvas does)
            GLuint tex = (lay->node && lay->node->vidbox) ? lay->node->vidbox->tex : (GLuint) -1;
            if (tex == (GLuint) -1 || tex == 0) continue;
            glBindFramebuffer(GL_FRAMEBUFFER, P.tmpfbo);
            glDrawBuffer_FBO();
            glViewport(0, 0, P.beamw, P.beamh);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            draw_direct(nullptr, g_transp, -1.0f, 1.0f, 2.0f, -2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, tex, P.beamw, P.beamh, false, false);
            draw_direct(nullptr, g_transp, -1.0f, 1.0f, 2.0f, -2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, g.masktex, P.beamw, P.beamh, false, false);
            glBindFramebuffer(GL_FRAMEBUFFER, P.beamfbo[P.bidx]);
            glDrawBuffer_FBO();
            glViewport(0, 0, P.beamw, P.beamh);
            glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE);   // (added: black outside the shapes adds nothing)
            draw_direct(nullptr, g_transp, -1.0f, 1.0f, 2.0f, -2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, P.tmptex, P.beamw, P.beamh, false, false);
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        }
    }

    mainprogram->directmode = budm;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDrawBuffer_Back();
    glViewport(0, 0, glob->w, glob->h);
    glFinish();   // (the finished texture goes to the output window; the next frame draws into the other one)
    P.beamshow = P.beamtex[P.bidx];
    P.bidx ^= 1;
}

// ======================================================================================================
// Waiting for the webcam
// ======================================================================================================

bool settled() {
    float el = std::chrono::duration<float>(std::chrono::steady_clock::now() - P.tr.tset).count();
    if (el > 3.0f) return true;
    // the stripe scan waits longer for every pattern: a webcam (an HDMI capture box especially) can be a few frames behind, and
    // a frame that still shows the pattern before spoils all pixels at once
    const bool scanning = P.tr.st == Tracer::SCAN_WAIT;
    if (scanning && P.tr.scan2 && P.cap2.counter.load() < P.tr.cset2 + 8) return false;   // (webcam 2 too)
    if (scanning) return el >= 0.7f && P.cap.counter.load() >= P.tr.cset + 8;
    // (after the scene changed: the reference frames, the outline check)
    return el >= 0.45f && P.cap.counter.load() >= P.tr.cset + 5;
}

// the beamer knots as corrected by the checks on the beamer (see verify_offset)
void apply_verify(const Seg &s, std::vector<float> &kb) {
    if (!s.vhas || P.sw <= 0 || P.sh <= 0) return;
    const double W = P.sw, H = P.sh;
    const int n = (int) std::min(std::min(kb.size(), s.kcam.size()), s.vu.size()) / 2;
    for (int i = 0; i < n; i++) {
        double unx = s.vu[i * 2] / W, uny = s.vu[i * 2 + 1] / H;
        kb[i * 2] -= (float) (s.vJ[0] * unx + s.vJ[1] * uny);
        kb[i * 2 + 1] -= (float) (s.vJ[2] * unx + s.vJ[3] * uny);
    }
}

void reset_verify(Seg &s) {
    s.verified = false;
    s.vtries = 0;
    s.vhas = false;
    s.vu.clear();
    s.vJ[0] = s.vJ[3] = 1.0f;
    s.vJ[1] = s.vJ[2] = 0.0f;
}


// ======================================================================================================
// Planes from depth (for the Warp "Plane" of a mask): the shape is taken as a plane, found from the depth of its pixels (MoGe), or
// from how the two webcams see it (the stripe maps say which pixel of the other webcam is the same point, through the beamer).
// A plane gives the frame real proportions on a tilted surface.
// ======================================================================================================

// eigenvalues / eigenvectors of a symmetric n x n matrix (row major, destroyed): w[k], the vector k is column k of V
void jacobi_eigen(std::vector<double> &a, int n, std::vector<double> &w, std::vector<double> &V) {
    V.assign((size_t) n * n, 0.0);
    for (int i = 0; i < n; i++) V[(size_t) i * n + i] = 1.0;
    for (int sweep = 0; sweep < 80; sweep++) {
        double off = 0.0;
        for (int p = 0; p < n; p++) for (int q = p + 1; q < n; q++) off += a[(size_t) p * n + q] * a[(size_t) p * n + q];
        if (off < 1e-24) break;
        for (int p = 0; p < n; p++) {
            for (int q = p + 1; q < n; q++) {
                const double apq = a[(size_t) p * n + q];
                if (std::fabs(apq) < 1e-300) continue;
                const double theta = (a[(size_t) q * n + q] - a[(size_t) p * n + p]) / (2.0 * apq);
                const double t = (theta >= 0.0 ? 1.0 : -1.0) / (std::fabs(theta) + std::sqrt(theta * theta + 1.0));
                const double c = 1.0 / std::sqrt(t * t + 1.0), s = t * c;
                for (int r = 0; r < n; r++) {
                    if (r == p || r == q) continue;
                    const double arp = a[(size_t) r * n + p], arq = a[(size_t) r * n + q];
                    a[(size_t) r * n + p] = a[(size_t) p * n + r] = c * arp - s * arq;
                    a[(size_t) r * n + q] = a[(size_t) q * n + r] = c * arq + s * arp;
                }
                const double app = a[(size_t) p * n + p], aqq = a[(size_t) q * n + q];
                a[(size_t) p * n + p] = app - t * apq;
                a[(size_t) q * n + q] = aqq + t * apq;
                a[(size_t) p * n + q] = a[(size_t) q * n + p] = 0.0;
                for (int r = 0; r < n; r++) {
                    const double vrp = V[(size_t) r * n + p], vrq = V[(size_t) r * n + q];
                    V[(size_t) r * n + p] = c * vrp - s * vrq;
                    V[(size_t) r * n + q] = c * vrq + s * vrp;
                }
            }
        }
    }
    w.resize(n);
    for (int i = 0; i < n; i++) w[i] = a[(size_t) i * n + i];
}

struct V3d {
    double x = 0.0, y = 0.0, z = 0.0;
};
inline V3d v3(double x, double y, double z) { V3d r; r.x = x; r.y = y; r.z = z; return r; }
inline V3d operator+(V3d a, V3d b) { return v3(a.x + b.x, a.y + b.y, a.z + b.z); }
inline V3d operator-(V3d a, V3d b) { return v3(a.x - b.x, a.y - b.y, a.z - b.z); }
inline V3d operator*(V3d a, double s) { return v3(a.x * s, a.y * s, a.z * s); }
inline double dot3(V3d a, V3d b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline V3d cross3(V3d a, V3d b) { return v3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }
inline double len3(V3d a) { return std::sqrt(dot3(a, a)); }
inline V3d unit3(V3d a) { const double l = len3(a); return l > 1e-15 ? a * (1.0 / l) : v3(0, 0, 0); }

// the pixels of a shape (the ones with 2 pixels of shape around them, in the edge's inside), at most about 'maxn' of them
void shape_pixels(const Seg &s, int W, int H, int maxn, std::vector<std::pair<int, int>> &px) {
    px.clear();
    auto inm = [&](int x, int y) { return x >= 0 && y >= 0 && x < W && y < H && s.mask[(size_t) y * W + x] >= 128; };
    long total = 0;
    for (int y = s.y0; y <= s.y1; y++) for (int x = s.x0; x <= s.x1; x++) if (inm(x, y)) total++;
    const int stride = std::max(1, (int) std::lround(std::sqrt((double) total / (double) std::max(1, maxn))));
    for (int y = s.y0; y <= s.y1; y += stride) {
        for (int x = s.x0; x <= s.x1; x += stride) {
            if (inm(x, y) && inm(x - 2, y) && inm(x + 2, y) && inm(x, y - 2) && inm(x, y + 2)) px.push_back({x, y});
        }
    }
}

// a plane n . X = d (d > 0) through the points: random samples for the plane most points are close to, then a least squares fit
// (the smallest eigenvector of the covariance) through those. c: the centre of the points on it; rms: their distance from it,
// as a share of the median depth.
bool fit_plane(const std::vector<V3d> &pts, V3d &n, double &d, V3d &c, double &rms) {
    const size_t N = pts.size();
    if (N < 50) return false;
    std::vector<double> zs(N);
    for (size_t i = 0; i < N; i++) zs[i] = std::fabs(pts[i].z);
    std::nth_element(zs.begin(), zs.begin() + N / 2, zs.end());
    const double med = std::max(zs[N / 2], 1e-9);
    const double thr = 0.015 * med;
    std::mt19937 rng(12345);
    std::uniform_int_distribution<size_t> pick(0, N - 1);
    size_t bestc = 0;
    V3d bn = v3(0, 0, 1);
    double bd = 0.0;
    for (int it = 0; it < 400; it++) {
        const size_t i = pick(rng), j = pick(rng), k = pick(rng);
        if (i == j || j == k || i == k) continue;
        V3d nn = cross3(pts[j] - pts[i], pts[k] - pts[i]);
        const double l = len3(nn);
        if (l < 1e-12 * med * med) continue;
        nn = nn * (1.0 / l);
        const double dd = dot3(nn, pts[i]);
        size_t cnt = 0;
        for (size_t q = 0; q < N; q++) if (std::fabs(dot3(nn, pts[q]) - dd) < thr) cnt++;
        if (cnt > bestc) { bestc = cnt; bn = nn; bd = dd; }
    }
    if (bestc < N / 5) return false;
    V3d cen = v3(0, 0, 0);
    std::vector<size_t> in;
    for (size_t q = 0; q < N; q++) if (std::fabs(dot3(bn, pts[q]) - bd) < thr) { in.push_back(q); cen = cen + pts[q]; }
    cen = cen * (1.0 / (double) in.size());
    std::vector<double> cov(9, 0.0);
    for (size_t q : in) {
        const V3d dv = pts[q] - cen;
        const double a[3] = {dv.x, dv.y, dv.z};
        for (int r = 0; r < 3; r++) for (int cc = 0; cc < 3; cc++) cov[r * 3 + cc] += a[r] * a[cc];
    }
    std::vector<double> w, V;
    jacobi_eigen(cov, 3, w, V);
    int mi = 0;
    for (int i = 1; i < 3; i++) if (w[i] < w[mi]) mi = i;
    n = unit3(v3(V[0 * 3 + mi], V[1 * 3 + mi], V[2 * 3 + mi]));
    d = dot3(n, cen);
    if (d < 0.0) { n = n * -1.0; d = -d; }
    c = cen;
    double ss = 0.0;
    for (size_t q : in) { const double e = dot3(n, pts[q]) - d; ss += e * e; }
    rms = std::sqrt(ss / (double) in.size()) / med;
    return d > 1e-12;
}

// ---- the webcams: lens distortion and field of view ----

// the position of a pixel as an ideal pinhole camera would see it: in focal lengths from the centre of the image (division model)
inline void undistort_px(double x, double y, double cx, double cy, double f, double k, double &xu, double &yu) {
    const double xn = (x - cx) / f, yn = (y - cy) / f;
    const double s = 1.0 / (1.0 + k * (xn * xn + yn * yn));
    xu = xn * s;
    yu = yn * s;
}

// The focal length (pixels) a webcam with an image of W x H is taken to have: from its FOV slider (the horizontal field of view),
// else from what MoGe finds (also a horizontal field of view), else for 70 degrees.
double focal_for_cam(int cam, int W, int H) {
    (void) H;
    Param *sl = cam == 0 ? P.fov : P.fov2;
    if (sl && sl->value > 0.5f) {
        const double hfov = (double) sl->value * 3.14159265358979 / 180.0;
        return 0.5 * W / std::tan(0.5 * hfov);
    }
    double hfov = 70.0 * 3.14159265358979 / 180.0;
    if (cam == 1 && P.mogefov2.load() > 0.01f) hfov = (double) P.mogefov2.load();
    else if (cam == 0 && P.dfx > 0.0f && P.dw > 0) hfov = 2.0 * std::atan(0.5 * (double) P.dw / (double) P.dfx);
    else if (cam == 1 && P.dfx > 0.0f && P.dw > 0) hfov = 2.0 * std::atan(0.5 * (double) P.dw / (double) P.dfx);   // (webcam 2: as webcam 1 when MoGe has not told)
    return 0.5 * W / std::tan(0.5 * hfov);
}

// the grid over the beamer image that says which pixel of a webcam sees each cell (a scan map turned around)
struct BeamerInverse {
    int gw = 0, gh = 0;
    std::vector<float> ax, ay, aw;
};

void build_inverse(const ScanData &S, int W, int H, BeamerInverse &inv) {
    inv.gw = 256;
    inv.gh = std::max(36, (int) std::lround(256.0 * (double) (P.beamh > 0 ? P.beamh : 1080) / (double) (P.beamw > 0 ? P.beamw : 1920)));
    const size_t n = (size_t) inv.gw * inv.gh;
    inv.ax.assign(n, 0.0f); inv.ay.assign(n, 0.0f); inv.aw.assign(n, 0.0f);
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            const size_t i = (size_t) y * W + x;
            const float bx = S.scanBx[i], by = S.scanBy[i];
            if (bx < 0.0f || by < 0.0f || bx > 1.0f || by > 1.0f) continue;
            const float gx = bx * inv.gw - 0.5f, gy = by * inv.gh - 0.5f;
            const int ix = (int) std::floor(gx), iy = (int) std::floor(gy);
            const float fx = gx - (float) ix, fy = gy - (float) iy;
            for (int dy = 0; dy < 2; dy++) {
                for (int dx = 0; dx < 2; dx++) {
                    const int cxx = ix + dx, cyy = iy + dy;
                    if (cxx < 0 || cyy < 0 || cxx >= inv.gw || cyy >= inv.gh) continue;
                    const float wgt = (dx ? fx : 1.0f - fx) * (dy ? fy : 1.0f - fy);
                    const size_t c = (size_t) cyy * inv.gw + cxx;
                    inv.ax[c] += wgt * ((float) x + 0.5f); inv.ay[c] += wgt * ((float) y + 0.5f); inv.aw[c] += wgt;
                }
            }
        }
    }
}

// the webcam pixel (continuous coordinates) that sees the beamer position (bx, by); false when there is none
bool sample_inverse(const BeamerInverse &inv, float bx, float by, double &x, double &y) {
    const float gx = bx * inv.gw - 0.5f, gy = by * inv.gh - 0.5f;
    const int ix = (int) std::floor(gx), iy = (int) std::floor(gy);
    const float fx = gx - (float) ix, fy = gy - (float) iy;
    double sx = 0.0, sy = 0.0, sw = 0.0;
    for (int dy = 0; dy < 2; dy++) {
        for (int dx = 0; dx < 2; dx++) {
            const int cxx = ix + dx, cyy = iy + dy;
            if (cxx < 0 || cyy < 0 || cxx >= inv.gw || cyy >= inv.gh) continue;
            const size_t c = (size_t) cyy * inv.gw + cxx;
            if (inv.aw[c] < 0.05f) continue;
            const double wgt = (dx ? fx : 1.0f - fx) * (dy ? fy : 1.0f - fy);
            sx += wgt * inv.ax[c] / inv.aw[c]; sy += wgt * inv.ay[c] / inv.aw[c]; sw += wgt;
        }
    }
    if (sw < 0.5) return false;
    x = sx / sw; y = sy / sw;
    return true;
}

bool homography_dlt(const std::vector<double> &p1, const std::vector<double> &p2, const std::vector<int> &idx, double Hout[9]);

// The radial distortion of a webcam, from its own stripe map and the beamer (an ideal pinhole image of the beamer's flat image):
// the dominant plane of the scene (a wall) is a homography from the undistorted webcam image to the beamer image, whatever the
// scene is made of otherwise. The k that makes the most points fit one homography is the distortion of the lens.
bool estimate_distortion(const ScanData &S, int W, int H, double f, double &kout) {
    if (W <= 0 || H <= 0 || f <= 0.0 || S.scanBx.size() < (size_t) W * H || S.scanBy.size() < (size_t) W * H) return false;
    const double cx = 0.5 * W, cy = 0.5 * H;
    const double bw = P.beamw > 0 ? P.beamw : 1920.0, bh = P.beamh > 0 ? P.beamh : 1080.0;
    std::vector<double> xs, ys, bxs, bys;
    const int stride = std::max(1, (int) std::sqrt((double) W * (double) H / 3000.0));
    for (int y = stride / 2; y < H; y += stride) {
        for (int x = stride / 2; x < W; x += stride) {
            const size_t i = (size_t) y * W + x;
            const float bx = S.scanBx[i], by = S.scanBy[i];
            if (bx < 0.0f || by < 0.0f || bx > 1.0f || by > 1.0f) continue;
            xs.push_back(x + 0.5); ys.push_back(y + 0.5);
            bxs.push_back((double) bx); bys.push_back((double) by * bh / bw);   // (the beamer image, isotropic)
        }
    }
    const int N = (int) xs.size();
    if (N < 400) return false;
    const double thr = 1.5 / bw;   // (a beamer pixel and a half)
    auto score = [&](double k, int &inliers) {
        std::vector<double> p1((size_t) N * 2), p2((size_t) N * 2);
        for (int i = 0; i < N; i++) {
            double xu, yu;
            undistort_px(xs[i], ys[i], cx, cy, f, k, xu, yu);
            p1[i * 2] = xu; p1[i * 2 + 1] = yu;
            p2[i * 2] = bxs[i]; p2[i * 2 + 1] = bys[i];
        }
        auto err = [&](const double Hm[9], int i) {
            const double x = p1[i * 2], y = p1[i * 2 + 1];
            const double hx = Hm[0] * x + Hm[1] * y + Hm[2], hy = Hm[3] * x + Hm[4] * y + Hm[5], hw = Hm[6] * x + Hm[7] * y + Hm[8];
            if (std::fabs(hw) < 1e-12) return 1e9;
            return std::hypot(hx / hw - p2[i * 2], hy / hw - p2[i * 2 + 1]);
        };
        std::mt19937 rng(4242);
        std::uniform_int_distribution<int> pick(0, N - 1);
        std::vector<int> best;
        for (int it = 0; it < 120; it++) {
            std::vector<int> smp;
            while ((int) smp.size() < 4) {
                const int c = pick(rng);
                if (std::find(smp.begin(), smp.end(), c) == smp.end()) smp.push_back(c);
            }
            double Hm[9];
            if (!homography_dlt(p1, p2, smp, Hm)) continue;
            std::vector<int> in;
            for (int i = 0; i < N; i++) if (err(Hm, i) < 3.0 * thr) in.push_back(i);
            if (in.size() > best.size()) best = in;
        }
        inliers = (int) best.size();
        if (best.size() < 12) return 1e9;
        double Hm[9];
        if (!homography_dlt(p1, p2, best, Hm)) return 1e9;
        int cnt = 0;
        double se = 0.0;
        for (int i = 0; i < N; i++) { const double e = err(Hm, i); if (e < thr) { cnt++; se += e; } }
        inliers = cnt;
        return -(double) cnt + (cnt ? se / cnt / thr : 1.0);   // (the most points on one plane, then the smallest error)
    };
    double bestk = 0.0, bestv = 1e18;
    int bestin = 0, in0 = 0;
    for (double k = -0.30; k <= 0.30001; k += 0.03) {
        int inl;
        const double v = score(k, inl);
        if (std::fabs(k) < 1e-9) in0 = inl;
        if (v < bestv) { bestv = v; bestk = k; bestin = inl; }
    }
    for (double k = bestk - 0.03; k <= bestk + 0.03001; k += 0.006) {
        int inl;
        const double v = score(k, inl);
        if (v < bestv) { bestv = v; bestk = k; bestin = inl; }
    }
    // (only when it clearly beats no distortion at all: the dominant plane has to be there and a good deal better)
    if (bestin < N / 5 || bestin < in0 * 1.05 + 5) { kout = 0.0; return false; }
    kout = bestk;
    return true;
}

// The focal lengths of the two webcams from what they both see, by themselves: through the beamer the stripe maps give thousands of
// pairs of pixels (one of each webcam) that show the same point of the scene. They give the fundamental matrix F (the epipolar
// geometry); the true focal lengths are those for which E = K2' F K1 has two equal singular values. F alone fixes only one
// relation between the two focal lengths, so the search is anchored at the starting values (the FOV slider / MoGe) and prefers
// the solution nearest to them. Fails (keeps the starting values) when the scene is a single plane, or the pairs fit badly.
bool estimate_focals(const ScanData &S1, int W1, int H1, const ScanData &S2, int W2, int H2, const double f0[2], const bool kok[2], const double k[2],
                     double &f1out, double &f2out) {
    BeamerInverse inv;
    build_inverse(S2, W2, H2, inv);
    const double cx1 = 0.5 * W1, cy1 = 0.5 * H1, cx2 = 0.5 * W2, cy2 = 0.5 * H2;
    std::vector<double> u1, u2;   // (pixels again, undistorted with the starting focal length)
    const int stride = std::max(1, (int) std::sqrt((double) W1 * (double) H1 / 5000.0));
    for (int y = stride / 2; y < H1; y += stride) {
        for (int x = stride / 2; x < W1; x += stride) {
            const size_t i = (size_t) y * W1 + x;
            const float bx = S1.scanBx[i], by = S1.scanBy[i];
            if (bx < 0.0f || by < 0.0f || bx > 1.0f || by > 1.0f) continue;
            double x2, y2;
            if (!sample_inverse(inv, bx, by, x2, y2)) continue;
            double a, b, c, d;
            undistort_px(x + 0.5, y + 0.5, cx1, cy1, f0[0], kok[0] ? k[0] : 0.0, a, b);
            undistort_px(x2, y2, cx2, cy2, f0[1], kok[1] ? k[1] : 0.0, c, d);
            u1.push_back(cx1 + a * f0[0]); u1.push_back(cy1 + b * f0[0]);
            u2.push_back(cx2 + c * f0[1]); u2.push_back(cy2 + d * f0[1]);
        }
    }
    const int N = (int) u1.size() / 2;
    if (N < 500) return false;
    // F by the normalized eight point algorithm, robustly
    auto normalize = [&](const std::vector<double> &p, double T[9]) {
        double mx = 0.0, my = 0.0;
        for (int i = 0; i < N; i++) { mx += p[i * 2]; my += p[i * 2 + 1]; }
        mx /= N; my /= N;
        double md = 0.0;
        for (int i = 0; i < N; i++) md += std::hypot(p[i * 2] - mx, p[i * 2 + 1] - my);
        md /= N;
        const double sc = md > 1e-9 ? std::sqrt(2.0) / md : 1.0;
        T[0] = sc; T[1] = 0; T[2] = -sc * mx; T[3] = 0; T[4] = sc; T[5] = -sc * my; T[6] = 0; T[7] = 0; T[8] = 1;
    };
    double T1[9], T2[9];
    normalize(u1, T1);
    normalize(u2, T2);
    std::vector<double> n1((size_t) N * 2), n2((size_t) N * 2);
    for (int i = 0; i < N; i++) {
        n1[i * 2] = T1[0] * u1[i * 2] + T1[2]; n1[i * 2 + 1] = T1[4] * u1[i * 2 + 1] + T1[5];
        n2[i * 2] = T2[0] * u2[i * 2] + T2[2]; n2[i * 2 + 1] = T2[4] * u2[i * 2 + 1] + T2[5];
    }
    auto eightpoint = [&](const std::vector<int> &idx, double Fo[9]) {
        std::vector<double> AtA(81, 0.0);
        for (int i : idx) {
            const double x1 = n1[i * 2], y1 = n1[i * 2 + 1], x2 = n2[i * 2], y2 = n2[i * 2 + 1];
            const double r[9] = {x2 * x1, x2 * y1, x2, y2 * x1, y2 * y1, y2, x1, y1, 1.0};
            for (int a = 0; a < 9; a++) for (int b = 0; b < 9; b++) AtA[a * 9 + b] += r[a] * r[b];
        }
        std::vector<double> w, V;
        jacobi_eigen(AtA, 9, w, V);
        int mi = 0;
        for (int i = 1; i < 9; i++) if (w[i] < w[mi]) mi = i;
        for (int q = 0; q < 9; q++) Fo[q] = V[q * 9 + mi];
        // rank 2: take away the smallest singular direction (F v3 v3^T)
        std::vector<double> FtF(9, 0.0);
        for (int a = 0; a < 3; a++) for (int b = 0; b < 3; b++) for (int k2 = 0; k2 < 3; k2++) FtF[a * 3 + b] += Fo[k2 * 3 + a] * Fo[k2 * 3 + b];
        std::vector<double> ew, EV;
        jacobi_eigen(FtF, 3, ew, EV);
        int m3 = 0;
        for (int i = 1; i < 3; i++) if (ew[i] < ew[m3]) m3 = i;
        const double v3x[3] = {EV[0 * 3 + m3], EV[1 * 3 + m3], EV[2 * 3 + m3]};
        double Fv[3];
        for (int r = 0; r < 3; r++) Fv[r] = Fo[r * 3] * v3x[0] + Fo[r * 3 + 1] * v3x[1] + Fo[r * 3 + 2] * v3x[2];
        for (int r = 0; r < 3; r++) for (int c = 0; c < 3; c++) Fo[r * 3 + c] -= Fv[r] * v3x[c];
    };
    auto sampson = [&](const double Fm[9], int i) {
        const double x1 = n1[i * 2], y1 = n1[i * 2 + 1], x2 = n2[i * 2], y2 = n2[i * 2 + 1];
        const double Fx1[3] = {Fm[0] * x1 + Fm[1] * y1 + Fm[2], Fm[3] * x1 + Fm[4] * y1 + Fm[5], Fm[6] * x1 + Fm[7] * y1 + Fm[8]};
        const double Ftx2[3] = {Fm[0] * x2 + Fm[3] * y2 + Fm[6], Fm[1] * x2 + Fm[4] * y2 + Fm[7], 0.0};
        const double num = x2 * Fx1[0] + y2 * Fx1[1] + Fx1[2];
        const double den = Fx1[0] * Fx1[0] + Fx1[1] * Fx1[1] + Ftx2[0] * Ftx2[0] + Ftx2[1] * Ftx2[1];
        return den > 1e-18 ? num * num / den : 1e18;
    };
    std::mt19937 rng(99);
    std::uniform_int_distribution<int> pick(0, N - 1);
    // (the threshold in normalized units: about a pixel and a half)
    double sc1 = T1[0];
    const double thr2 = (1.5 * sc1) * (1.5 * sc1);
    std::vector<int> best;
    for (int it = 0; it < 250; it++) {
        std::vector<int> smp;
        while ((int) smp.size() < 8) {
            const int c = pick(rng);
            if (std::find(smp.begin(), smp.end(), c) == smp.end()) smp.push_back(c);
        }
        double Fm[9];
        eightpoint(smp, Fm);
        std::vector<int> in;
        for (int i = 0; i < N; i++) if (sampson(Fm, i) < thr2) in.push_back(i);
        if (in.size() > best.size()) best = in;
    }
    if ((int) best.size() < N / 3) { fprintf(stderr, "[ProjMap] calibration: the pairs of points fit no epipolar geometry (%zu of %d)\n", best.size(), N); return false; }
    double Fn[9];
    eightpoint(best, Fn);
    // the scene must be more than one plane, or F is not determined: a homography through the inliers must not explain them all
    {
        std::vector<double> q1, q2;
        for (int i : best) { q1.push_back(n1[i * 2]); q1.push_back(n1[i * 2 + 1]); q2.push_back(n2[i * 2]); q2.push_back(n2[i * 2 + 1]); }
        std::vector<int> idx((size_t) best.size());
        for (size_t i = 0; i < idx.size(); i++) idx[i] = (int) i;
        double Hm[9];
        std::vector<int> planar;
        std::mt19937 rng2(5);
        std::uniform_int_distribution<int> pick2(0, (int) best.size() - 1);
        size_t bestp = 0;
        for (int it = 0; it < 150; it++) {
            std::vector<int> smp;
            while ((int) smp.size() < 4) {
                const int c = pick2(rng2);
                if (std::find(smp.begin(), smp.end(), c) == smp.end()) smp.push_back(c);
            }
            if (!homography_dlt(q1, q2, smp, Hm)) continue;
            size_t cnt = 0;
            for (size_t i = 0; i < best.size(); i++) {
                const double x = q1[i * 2], y = q1[i * 2 + 1];
                const double hx = Hm[0] * x + Hm[1] * y + Hm[2], hy = Hm[3] * x + Hm[4] * y + Hm[5], hw = Hm[6] * x + Hm[7] * y + Hm[8];
                if (std::fabs(hw) < 1e-12) continue;
                if (std::hypot(hx / hw - q2[i * 2], hy / hw - q2[i * 2 + 1]) < 1.5 * sc1) cnt++;
            }
            bestp = std::max(bestp, cnt);
        }
        if ((double) bestp > 0.92 * (double) best.size()) {
            fprintf(stderr, "[ProjMap] calibration: what both webcams see is nearly one plane (%.0f%% of the pairs): the field of view cannot be told, the starting values stay\n",
                    100.0 * (double) bestp / (double) best.size());
            return false;
        }
    }
    // F in pixels: F = T2' Fn T1
    double F[9];
    {
        double tmp[9];
        for (int r = 0; r < 3; r++) for (int c = 0; c < 3; c++) tmp[r * 3 + c] = Fn[r * 3 + 0] * T1[0 * 3 + c] + Fn[r * 3 + 1] * T1[1 * 3 + c] + Fn[r * 3 + 2] * T1[2 * 3 + c];
        for (int r = 0; r < 3; r++) for (int c = 0; c < 3; c++) F[r * 3 + c] = T2[0 * 3 + r] * tmp[0 * 3 + c] + T2[1 * 3 + r] * tmp[1 * 3 + c] + T2[2 * 3 + r] * tmp[2 * 3 + c];
    }
    // E = K2' F K1 for focal lengths f1, f2 (the centres of the images are the principal points); the cost: how different the two
    // largest singular values of E are
    auto cost = [&](double f1, double f2) {
        const double K1[9] = {f1, 0, cx1, 0, f1, cy1, 0, 0, 1}, K2[9] = {f2, 0, cx2, 0, f2, cy2, 0, 0, 1};
        double tmp[9], E[9];
        for (int r = 0; r < 3; r++) for (int c = 0; c < 3; c++) tmp[r * 3 + c] = F[r * 3 + 0] * K1[0 * 3 + c] + F[r * 3 + 1] * K1[1 * 3 + c] + F[r * 3 + 2] * K1[2 * 3 + c];
        for (int r = 0; r < 3; r++) for (int c = 0; c < 3; c++) E[r * 3 + c] = K2[0 * 3 + r] * tmp[0 * 3 + c] + K2[1 * 3 + r] * tmp[1 * 3 + c] + K2[2 * 3 + r] * tmp[2 * 3 + c];
        std::vector<double> EtE(9, 0.0);
        for (int a = 0; a < 3; a++) for (int b = 0; b < 3; b++) for (int k2 = 0; k2 < 3; k2++) EtE[a * 3 + b] += E[k2 * 3 + a] * E[k2 * 3 + b];
        std::vector<double> ew, EV;
        jacobi_eigen(EtE, 3, ew, EV);
        std::sort(ew.begin(), ew.end());
        const double s1 = std::sqrt(std::max(ew[2], 0.0)), s2 = std::sqrt(std::max(ew[1], 0.0));
        return (s1 + s2) > 1e-18 ? (s1 - s2) / (s1 + s2) : 1.0;
    };
    auto total = [&](double f1, double f2) {
        const double p1 = std::log(f1 / f0[0]), p2 = std::log(f2 / f0[1]);
        return cost(f1, f2) + 0.01 * (p1 * p1 + p2 * p2);   // (the pull to the starting values)
    };
    double bf1 = f0[0], bf2 = f0[1], bv = total(bf1, bf2);
    for (int pass = 0; pass < 2; pass++) {
        const double span = pass == 0 ? 0.5 : 0.08;   // (log range around the best so far)
        const double c1 = bf1, c2 = bf2;
        for (int a = -12; a <= 12; a++) {
            for (int b = -12; b <= 12; b++) {
                const double f1 = c1 * std::exp(span * a / 12.0), f2 = c2 * std::exp(span * b / 12.0);
                const double v = total(f1, f2);
                if (v < bv) { bv = v; bf1 = f1; bf2 = f2; }
            }
        }
    }
    const double c0 = cost(bf1, bf2), cstart = cost(f0[0], f0[1]);
    fprintf(stderr, "[ProjMap] calibration: %zu of %d pairs fit the epipolar geometry; focal lengths %.0f -> %.0f px (webcam 1), %.0f -> %.0f px (webcam 2), E mismatch %.3f (%.3f with the starting values)\n",
            best.size(), N, f0[0], bf1, f0[1], bf2, c0, cstart);
    if (c0 > 0.25) return false;   // (does not look like an essential matrix at all: not to be trusted)
    // F fixes only one relation between the two focal lengths: when the starting values are about as consistent as the
    // refined ones, the data does not tell them apart and the starting values stay
    if (cstart - c0 < 0.03) {
        fprintf(stderr, "[ProjMap] calibration: the starting focal lengths are consistent with the pairs as well (the data does not tell more): kept\n");
        return false;
    }
    f1out = bf1;
    f2out = bf2;
    return true;
}

// all of it for the maps of this scan: the distortion of each webcam, then the focal lengths of both
void calibrate_cameras() {
    Tracer &T = P.tr;
    T.cal = CamCal();
    const int W = P.sw, H = P.sh;
    const ScanData &S1 = T.sd[0], &S2 = T.sd[1];
    const bool two = T.scan2 && S2.scanW > 0 && S2.scanH > 0 && S2.scanBx.size() >= (size_t) S2.scanW * S2.scanH;
    if (W <= 0 || H <= 0 || S1.scanBx.size() < (size_t) W * H) return;
    double f0[2];
    f0[0] = focal_for_cam(0, W, H);
    f0[1] = two ? focal_for_cam(1, S2.scanW, S2.scanH) : f0[0];
    T.cal.f1 = f0[0];
    T.cal.f2 = f0[1];
    double k;
    if (estimate_distortion(S1, W, H, f0[0], k)) { T.cal.kok[0] = true; T.cal.k[0] = k; }
    if (two && estimate_distortion(S2, S2.scanW, S2.scanH, f0[1], k)) { T.cal.kok[1] = true; T.cal.k[1] = k; }
    fprintf(stderr, "[ProjMap] calibration: lens distortion %s (webcam 1)%s\n", T.cal.kok[0] ? std::to_string(T.cal.k[0]).c_str() : "not found",
            two ? (std::string(", ") + (T.cal.kok[1] ? std::to_string(T.cal.k[1]) : std::string("not found")) + " (webcam 2)").c_str() : "");
    if (two) {
        double a, b;
        if (estimate_focals(S1, W, H, S2, S2.scanW, S2.scanH, f0, T.cal.kok, T.cal.k, a, b)) {
            T.cal.ok = true;
            T.cal.f1 = a;
            T.cal.f2 = b;
        }
    }
}

// The plane n . X = d (d > 0, c a point of it) as a lookup like the one of the surface: for every cell of the grid the position in
// the plane's own box, 0..1 across it (the plane's "right" and "down" are those of the webcam image). The webcam is a pinhole of
// focal length f at the centre of the snapshot after undoing its lens distortion k. 'lut' gives for every cell the position in
// the shape's box in the webcam image; the plane lookup is what this position means on the plane.
bool bake_plane_lut(const Seg &s, const WarpLut &lut, int W, int H, double f, double k, V3d n, double d, V3d c, WarpPlane &out) {
    const double cx = 0.5 * W, cy = 0.5 * H;
    const V3d ex = v3(1, 0, 0), ey = v3(0, 1, 0);
    V3d ub = unit3(ex - n * dot3(ex, n));
    V3d vb = ey - n * dot3(ey, n);
    vb = unit3(vb - ub * dot3(vb, ub));
    if (len3(ub) < 0.5 || len3(vb) < 0.5) return false;
    // a pixel on the plane: false when the ray from the webcam does not meet it in front
    auto onplane = [&](double x, double y, double &u, double &v) {
        double xu, yu;
        undistort_px(x, y, cx, cy, f, k, xu, yu);
        const V3d r = v3(xu, yu, 1.0);
        const double nr = dot3(n, r);
        if (nr < 1e-9) return false;
        const V3d X = r * (d / nr) - c;
        u = dot3(X, ub);
        v = dot3(X, vb);
        return true;
    };
    std::vector<std::pair<int, int>> px;
    shape_pixels(s, W, H, 6000, px);
    std::vector<double> us, vs;
    for (auto &p : px) {
        double u, v;
        if (onplane(p.first + 0.5, p.second + 0.5, u, v)) { us.push_back(u); vs.push_back(v); }
    }
    if (us.size() < 50) return false;
    {
        // Turn the plane's axes to the shape: the frame should lie along the edges of the object (a face of a box), not along the
        // image axes of a tilted or oblique view. The angle (within +-45 degrees of the image axes) with the smallest box around the
        // shape is taken, when it is clearly smaller than the box along the image axes.
        auto boxarea = [&](double th) {
            const double c = std::cos(th), sn = std::sin(th);
            std::vector<double> a(us.size()), b(us.size());
            for (size_t i = 0; i < us.size(); i++) { a[i] = c * us[i] + sn * vs[i]; b[i] = -sn * us[i] + c * vs[i]; }
            std::sort(a.begin(), a.end());
            std::sort(b.begin(), b.end());
            const size_t l = a.size() / 100, h = a.size() - 1 - a.size() / 100;
            return (a[h] - a[l]) * (b[h] - b[l]);
        };
        const double a0 = boxarea(0.0);
        double best = a0, bth = 0.0;
        for (int deg = -45; deg <= 45; deg++) {
            if (deg == 0) continue;
            const double th = deg * 3.14159265358979 / 180.0, ar = boxarea(th);
            if (ar < best) { best = ar; bth = th; }
        }
        if (bth != 0.0 && best < 0.93 * a0) {
            const double c = std::cos(bth), sn = std::sin(bth);
            const V3d nu = ub * c + vb * sn, nv = vb * c - ub * sn;
            ub = nu; vb = nv;
            for (size_t i = 0; i < us.size(); i++) {
                const double u = c * us[i] + sn * vs[i], v = -sn * us[i] + c * vs[i];
                us[i] = u; vs[i] = v;
            }
        }
    }
    std::sort(us.begin(), us.end());
    std::sort(vs.begin(), vs.end());
    const size_t lo = us.size() / 200, hi = us.size() - 1 - us.size() / 200;   // (the middle 99%)
    const double umin = us[lo], umax = us[hi], vmin = vs[lo], vmax = vs[hi];
    const double Wm = umax - umin, Hm = vmax - vmin;
    if (Wm < 1e-9 || Hm < 1e-9) return false;
    const double bw = s.x1 - s.x0 + 1, bh = s.y1 - s.y0 + 1;
    out.uv.assign(lut.uv.size(), 0.0f);
    const size_t cells = (size_t) lut.gw * lut.gh;
    for (size_t i = 0; i < cells; i++) {
        const double x = (double) s.x0 + (double) lut.uv[i * 2] * bw, y = (double) s.y0 + (double) lut.uv[i * 2 + 1] * bh;
        double u, v;
        if (!onplane(x, y, u, v)) { u = umin; v = vmin; }   // (behind the webcam: far outside, harmless)
        out.uv[i * 2] = (float) std::clamp((u - umin) / Wm, -0.9, 2.9);
        out.uv[i * 2 + 1] = (float) std::clamp((v - vmin) / Hm, -0.9, 2.9);
    }
    out.aspect = (float) (Wm / Hm);
    out.valid = true;
    return true;
}

// the plane of a shape from the MoGe depth of the snapshot
bool moge_plane(const Seg &s, const WarpLut &lut, WarpPlane &out, V3d &nout) {
    const int W = P.sw, H = P.sh;
    if (P.depth.empty() || P.dw <= 0 || P.dh <= 0 || P.dfx <= 0.0f || W <= 0 || H <= 0 || (int) P.depth.size() < P.dw * P.dh) return false;
    if (s.mask.size() < (size_t) W * H || s.x1 < s.x0) return false;
    const double fx = (double) P.dfx * (double) W / (double) P.dw, cx = 0.5 * W, cy = 0.5 * H;
    const double k = P.tr.cal.kok[0] ? P.tr.cal.k[0] : 0.0;   // (the lens distortion found for webcam 1)
    std::vector<std::pair<int, int>> px;
    shape_pixels(s, W, H, 5000, px);
    std::vector<V3d> pts;
    for (auto &p : px) {
        const int dx = std::clamp((int) ((long) p.first * P.dw / W), 0, P.dw - 1), dy = std::clamp((int) ((long) p.second * P.dh / H), 0, P.dh - 1);
        const double z = (double) P.depth[(size_t) dy * P.dw + dx];
        if (!(z > 0.0) || !std::isfinite(z)) continue;
        double xu, yu;
        undistort_px(p.first + 0.5, p.second + 0.5, cx, cy, fx, k, xu, yu);
        pts.push_back(v3(xu * z, yu * z, z));
    }
    V3d n, c;
    double d, rms;
    if (!fit_plane(pts, n, d, c, rms)) return false;
    if (!bake_plane_lut(s, lut, W, H, fx, k, n, d, c, out)) return false;
    out.rms = (float) rms;
    nout = n;
    fprintf(stderr, "[ProjMap] plane (AI Depth): normal (%.2f, %.2f, %.2f), proportions %.3f : 1, off the plane by %.1f%% of the depth\n", n.x, n.y, n.z, (double) out.aspect, 100.0 * rms);
    return true;
}

// a homography x2 ~ H x1 (3 x 3, row major) through at least 4 point pairs (normalized coordinates), least squares
bool homography_dlt(const std::vector<double> &p1, const std::vector<double> &p2, const std::vector<int> &idx, double Hout[9]) {
    std::vector<double> AtA(81, 0.0);
    for (int i : idx) {
        const double x1 = p1[i * 2], y1 = p1[i * 2 + 1], x2 = p2[i * 2], y2 = p2[i * 2 + 1];
        const double r1[9] = {-x1, -y1, -1, 0, 0, 0, x2 * x1, x2 * y1, x2};
        const double r2[9] = {0, 0, 0, -x1, -y1, -1, y2 * x1, y2 * y1, y2};
        for (int a = 0; a < 9; a++) for (int b = 0; b < 9; b++) AtA[a * 9 + b] += r1[a] * r1[b] + r2[a] * r2[b];
    }
    std::vector<double> w, V;
    jacobi_eigen(AtA, 9, w, V);
    int mi = 0;
    for (int i = 1; i < 9; i++) if (w[i] < w[mi]) mi = i;
    for (int q = 0; q < 9; q++) Hout[q] = V[q * 9 + mi];
    const double nrm = std::sqrt(Hout[0] * Hout[0] + Hout[1] * Hout[1] + Hout[2] * Hout[2] + Hout[3] * Hout[3] + Hout[4] * Hout[4] +
                                 Hout[5] * Hout[5] + Hout[6] * Hout[6] + Hout[7] * Hout[7] + Hout[8] * Hout[8]);
    if (nrm < 1e-12) return false;
    for (int q = 0; q < 9; q++) Hout[q] /= nrm;
    return true;
}

// the plane of a shape from the two webcams. Through the beamer, the stripe maps say which pixel of webcam 2 is the same point as
// a pixel of the shape in webcam 1; for a plane these pairs are related by a homography, which (with the webcams' fields of view)
// has the plane's orientation in it (the decomposition of Faugeras & Lustman, as in Ma, Soatto, Kosecka, Sastry). Of its two
// solutions that can both be seen from both webcams the one nearest to npref (the MoGe normal, when there is one) is taken, else
// the one that faces both webcams best.
bool stereo_plane(const Seg &s, const WarpLut &lut, WarpPlane &out, const V3d *npref) {
    const int W = P.sw, H = P.sh;
    const ScanData &S1 = P.tr.sd[0], &S2 = P.tr.sd[1];
    const int W2 = S2.scanW, H2 = S2.scanH;
    if (!P.tr.scan2 || W2 <= 0 || H2 <= 0 || W <= 0 || H <= 0) return false;
    if (S2.scanBx.size() < (size_t) W2 * H2 || S1.scanBx.size() < (size_t) W * H || s.mask.size() < (size_t) W * H || s.x1 < s.x0) return false;
    // the webcams: the focal lengths the self-calibration found (else the FOV slider / MoGe / 70 degrees) and their lens distortion
    const CamCal &cal = P.tr.cal;
    const double f1 = cal.f1 > 0.0 ? cal.f1 : focal_for_cam(0, W, H);
    const double f2 = cal.f2 > 0.0 ? cal.f2 : focal_for_cam(1, W2, H2);
    const double k1 = cal.kok[0] ? cal.k[0] : 0.0, k2 = cal.kok[1] ? cal.k[1] : 0.0;
    const double cx1 = 0.5 * W, cy1 = 0.5 * H, cx2 = 0.5 * W2, cy2 = 0.5 * H2;
    BeamerInverse inv;
    build_inverse(S2, W2, H2, inv);   // (webcam 2 turned around: which of its pixels sees each cell of the beamer image)
    // the pairs: the shape's pixels in webcam 1 and where webcam 2 sees the same beamer position (undistorted, in focal lengths)
    std::vector<std::pair<int, int>> px;
    shape_pixels(s, W, H, 3000, px);
    std::vector<double> p1, p2;
    for (auto &p : px) {
        const size_t i = (size_t) p.second * W + p.first;
        const float bx = S1.scanBx[i], by = S1.scanBy[i];
        if (bx < 0.0f || by < 0.0f || bx > 1.0f || by > 1.0f) continue;
        double x2, y2;
        if (!sample_inverse(inv, bx, by, x2, y2)) continue;
        double a, b, c, d;
        undistort_px(p.first + 0.5, p.second + 0.5, cx1, cy1, f1, k1, a, b);
        undistort_px(x2, y2, cx2, cy2, f2, k2, c, d);
        p1.push_back(a); p1.push_back(b);
        p2.push_back(c); p2.push_back(d);   // (rays: K^-1 of each webcam)
    }
    const int N = (int) p1.size() / 2;
    if (N < 100) { fprintf(stderr, "[ProjMap] plane (2 webcams): only %d pairs of points between the webcams\n", N); return false; }
    // the homography, robustly
    std::mt19937 rng(777);
    std::uniform_int_distribution<int> pick(0, N - 1);
    const double thr = 4.0 / std::min(f1, f2);   // (about four pixels of the coarser webcam: the pairs come from a grid over the beamer image)
    auto transfer_err = [&](const double Hm[9], int i) {
        const double x = p1[i * 2], y = p1[i * 2 + 1];
        const double hx = Hm[0] * x + Hm[1] * y + Hm[2], hy = Hm[3] * x + Hm[4] * y + Hm[5], hw = Hm[6] * x + Hm[7] * y + Hm[8];
        if (std::fabs(hw) < 1e-12) return 1e9;
        return std::hypot(hx / hw - p2[i * 2], hy / hw - p2[i * 2 + 1]);
    };
    std::vector<int> best;
    for (int it = 0; it < 300; it++) {
        std::vector<int> smp;
        while ((int) smp.size() < 4) {
            const int c = pick(rng);
            if (std::find(smp.begin(), smp.end(), c) == smp.end()) smp.push_back(c);
        }
        double Hm[9];
        if (!homography_dlt(p1, p2, smp, Hm)) continue;
        std::vector<int> in;
        for (int i = 0; i < N; i++) if (transfer_err(Hm, i) < thr) in.push_back(i);
        if (in.size() > best.size()) best = in;
    }
    if ((int) best.size() < std::max(60, N / 4)) { fprintf(stderr, "[ProjMap] plane (2 webcams): the pairs do not fit one plane (%zu of %d)\n", best.size(), N); return false; }
    double Hn[9];
    if (!homography_dlt(p1, p2, best, Hn)) return false;
    // the sign: points in front of both webcams
    {
        double sgn = 0.0;
        for (int i : best) {
            const double x = p1[i * 2], y = p1[i * 2 + 1];
            const double hx = Hn[0] * x + Hn[1] * y + Hn[2], hy = Hn[3] * x + Hn[4] * y + Hn[5], hw = Hn[6] * x + Hn[7] * y + Hn[8];
            sgn += p2[i * 2] * hx + p2[i * 2 + 1] * hy + hw;
        }
        if (sgn < 0.0) for (int q = 0; q < 9; q++) Hn[q] = -Hn[q];
    }
    // the singular values: eigenvalues of H^T H
    std::vector<double> HtH(9, 0.0);
    for (int a = 0; a < 3; a++) for (int b = 0; b < 3; b++) for (int k = 0; k < 3; k++) HtH[a * 3 + b] += Hn[k * 3 + a] * Hn[k * 3 + b];
    std::vector<double> ew, EV;
    jacobi_eigen(HtH, 3, ew, EV);
    int order[3] = {0, 1, 2};
    std::sort(order, order + 3, [&](int a, int b) { return ew[a] > ew[b]; });
    double sig[3];
    V3d vv[3];
    for (int k = 0; k < 3; k++) {
        sig[k] = std::sqrt(std::max(ew[order[k]], 0.0));
        vv[k] = v3(EV[0 * 3 + order[k]], EV[1 * 3 + order[k]], EV[2 * 3 + order[k]]);
    }
    if (sig[1] < 1e-9) return false;
    if (dot3(cross3(vv[0], vv[1]), vv[2]) < 0.0) vv[2] = vv[2] * -1.0;
    for (int q = 0; q < 9; q++) Hn[q] /= sig[1];   // (the middle singular value is 1)
    const double s1 = sig[0] / sig[1], s3 = sig[2] / sig[1];
    const double c13 = std::sqrt(std::max(0.0, s1 * s1 - s3 * s3));
    if (c13 < 1e-3) { fprintf(stderr, "[ProjMap] plane (2 webcams): the webcams see the shape from nearly the same place, no plane can be told\n"); return false; }
    const double Aq = std::sqrt(std::max(0.0, 1.0 - s3 * s3)), Bq = std::sqrt(std::max(0.0, s1 * s1 - 1.0));
    const V3d u1 = (vv[0] * Aq + vv[2] * Bq) * (1.0 / c13), u2 = (vv[0] * Aq - vv[2] * Bq) * (1.0 / c13);
    auto apply = [&](V3d v) { return v3(Hn[0] * v.x + Hn[1] * v.y + Hn[2] * v.z, Hn[3] * v.x + Hn[4] * v.y + Hn[5] * v.z, Hn[6] * v.x + Hn[7] * v.y + Hn[8] * v.z); };
    V3d cand_n[2], cand_rn[2];   // (the normal in webcam 1, and in webcam 2)
    double candR[2][9];
    for (int k = 0; k < 2; k++) {
        const V3d u = k == 0 ? u1 : u2;
        const V3d U[3] = {vv[1], u, cross3(vv[1], u)};
        const V3d Hv2 = apply(vv[1]), Hu = apply(u);
        const V3d Wc[3] = {Hv2, Hu, cross3(Hv2, Hu)};
        // R = W U^T (the columns of W and U are given)
        double R[9];
        for (int r = 0; r < 3; r++) {
            const double wr[3] = {Wc[0].x * (r == 0) + Wc[0].y * (r == 1) + Wc[0].z * (r == 2), Wc[1].x * (r == 0) + Wc[1].y * (r == 1) + Wc[1].z * (r == 2),
                                  Wc[2].x * (r == 0) + Wc[2].y * (r == 1) + Wc[2].z * (r == 2)};
            for (int cc = 0; cc < 3; cc++) {
                const double ucc[3] = {U[0].x * (cc == 0) + U[0].y * (cc == 1) + U[0].z * (cc == 2), U[1].x * (cc == 0) + U[1].y * (cc == 1) + U[1].z * (cc == 2),
                                       U[2].x * (cc == 0) + U[2].y * (cc == 1) + U[2].z * (cc == 2)};
                R[r * 3 + cc] = wr[0] * ucc[0] + wr[1] * ucc[1] + wr[2] * ucc[2];
            }
        }
        V3d Nk = cross3(vv[1], u);
        for (int q = 0; q < 9; q++) candR[k][q] = R[q];
        cand_n[k] = Nk;
        cand_rn[k] = v3(R[0] * Nk.x + R[1] * Nk.y + R[2] * Nk.z, R[3] * Nk.x + R[4] * Nk.y + R[5] * Nk.z, R[6] * Nk.x + R[7] * Nk.y + R[8] * Nk.z);
    }
    // the mean viewing directions of the pairs, in both webcams
    V3d r1m = v3(0, 0, 0), r2m = v3(0, 0, 0);
    for (int i : best) {
        r1m = r1m + unit3(v3(p1[i * 2], p1[i * 2 + 1], 1.0));
        r2m = r2m + unit3(v3(p2[i * 2], p2[i * 2 + 1], 1.0));
    }
    r1m = unit3(r1m); r2m = unit3(r2m);
    int bestk = -1;
    double bestscore = -1e9;
    V3d chosen = v3(0, 0, 1);
    for (int k = 0; k < 2; k++) {
        V3d Nk = cand_n[k], Nk2 = cand_rn[k];
        if (dot3(Nk, r1m) < 0.0) { Nk = Nk * -1.0; Nk2 = Nk2 * -1.0; }   // (the plane faces webcam 1)
        const double f1c = dot3(Nk, r1m), f2c = dot3(Nk2, r2m);
        double score = f1c + f2c;
        if (f2c < 0.0) score -= 10.0;   // (not seen from webcam 2: not possible)
        if (npref) score = dot3(Nk, *npref) - (f2c < 0.0 ? 10.0 : 0.0);
        if (score > bestscore) { bestscore = score; bestk = k; chosen = Nk; }
    }
    if (bestk < 0) return false;
    // How well can the plane be told? The homography is H = R + (T / d) N^T: the plane's normal N is only in it as far as the
    // webcams are apart (T / d, the distance between the webcams as a share of their distance to the plane). When they are close
    // together compared with the distance, H is nearly a rotation and N is noise.
    {
        const V3d Nk = cand_n[bestk];
        const double *R = candR[bestk];
        const V3d HN = v3(Hn[0] * Nk.x + Hn[1] * Nk.y + Hn[2] * Nk.z, Hn[3] * Nk.x + Hn[4] * Nk.y + Hn[5] * Nk.z, Hn[6] * Nk.x + Hn[7] * Nk.y + Hn[8] * Nk.z);
        const V3d RN = v3(R[0] * Nk.x + R[1] * Nk.y + R[2] * Nk.z, R[3] * Nk.x + R[4] * Nk.y + R[5] * Nk.z, R[6] * Nk.x + R[7] * Nk.y + R[8] * Nk.z);
        const double tb = len3(HN - RN);
        const V3d c0 = unit3(cand_n[0] * (dot3(cand_n[0], r1m) < 0.0 ? -1.0 : 1.0)), c1 = unit3(cand_n[1] * (dot3(cand_n[1], r1m) < 0.0 ? -1.0 : 1.0));
        fprintf(stderr, "[ProjMap] plane (2 webcams): the two solutions have normals (%.2f, %.2f, %.2f) and (%.2f, %.2f, %.2f); the webcams are %.1f%% of the distance to the plane apart\n",
                c0.x, c0.y, c0.z, c1.x, c1.y, c1.z, 100.0 * tb);
        if (tb < 0.04) {
            fprintf(stderr, "[ProjMap] plane (2 webcams): the webcams are too close together compared with their distance to the shape: the plane cannot be told, not used\n");
            return false;
        }
    }
    if (npref) {
        // (not a reason to turn it down: a shape of several faces has one plane per face, and the two methods may well find
        // different ones; it is only logged)
        fprintf(stderr, "[ProjMap] plane (2 webcams): %.0f degrees from the plane AI Depth finds\n",
                std::acos(std::clamp(dot3(unit3(chosen), *npref), -1.0, 1.0)) * 180.0 / 3.14159265358979);
    }
    // the plane n . X = 1 (the size of the scene is not known from the homography: only proportions matter)
    const V3d n = unit3(chosen);
    const double xc = 0.5 * (s.x0 + s.x1 + 1), yc = 0.5 * (s.y0 + s.y1 + 1);
    double xcu, ycu;
    undistort_px(xc, yc, cx1, cy1, f1, k1, xcu, ycu);
    const V3d rc = v3(xcu, ycu, 1.0);
    const double nr = dot3(n, rc);
    if (nr < 1e-6) return false;
    const V3d c = rc * (1.0 / nr);
    if (!bake_plane_lut(s, lut, W, H, f1, k1, n, 1.0, c, out)) return false;
    out.rms = (float) (1.0 - (double) best.size() / (double) N);   // (the share of the pairs that did not fit)
    fprintf(stderr, "[ProjMap] plane (2 webcams): normal (%.2f, %.2f, %.2f), proportions %.3f : 1, %zu of %d pairs fit (focal lengths %.0f / %.0f px%s)\n",
            n.x, n.y, n.z, (double) out.aspect, best.size(), N, f1, f2, cal.ok ? ", self-calibrated" : "");
    return true;
}

// The point cloud of the AI depth of the snapshot, with the colours of the snapshot (for the Viewpoint in the Mapping view): about
// 150000 points, in the frame of webcam 1 (x right, y down, z forward).
std::shared_ptr<ScenePoints> build_scene_points() {
    const int W = P.sw, H = P.sh;
    if (P.depth.empty() || P.dw <= 0 || P.dh <= 0 || P.dfx <= 0.0f || W <= 0 || H <= 0 || (int) P.depth.size() < P.dw * P.dh) return nullptr;
    if (P.snap.size() < (size_t) W * H * 4) return nullptr;
    const double fx = (double) P.dfx * (double) W / (double) P.dw, cx = 0.5 * W, cy = 0.5 * H;
    int stride = 1;
    while (stride < 64 && ((long long) W * H / ((long long) stride * stride)) > 150000) stride++;
    auto sp = std::make_shared<ScenePoints>();
    sp->hfov = (float) (2.0 * std::atan(0.5 * (double) W / fx) * 180.0 / 3.14159265358979);
    sp->aspect = (float) W / (float) H;
    sp->data.reserve((size_t) (W / stride + 1) * (H / stride + 1) * 16);
    for (int y = stride / 2; y < H; y += stride) {
        for (int x = stride / 2; x < W; x += stride) {
            const int dx = std::clamp((int) ((long) x * P.dw / W), 0, P.dw - 1), dy = std::clamp((int) ((long) y * P.dh / H), 0, P.dh - 1);
            const float z = P.depth[(size_t) dy * P.dw + dx];
            if (!(z > 0.0f) || !std::isfinite(z)) continue;
            const float px = (float) ((x + 0.5 - cx) / fx * z), py = (float) ((y + 0.5 - cy) / fx * z);
            uint8_t rec[16];
            std::memcpy(rec, &px, 4); std::memcpy(rec + 4, &py, 4); std::memcpy(rec + 8, &z, 4);
            const size_t i = ((size_t) y * W + x) * 4;
            rec[12] = P.snap[i]; rec[13] = P.snap[i + 1]; rec[14] = P.snap[i + 2]; rec[15] = 255;
            sp->data.insert(sp->data.end(), rec, rec + 16);
            sp->n++;
        }
    }
    if (sp->n < 1000) return nullptr;
    return sp;
}

// the 3D position (frame of webcam 1) of the point of the object every cell of the surface lookup shows: the webcam pixel of the
// cell, its AI depth, unprojected; smoothed a little over the neighbours that are at about the same depth
void compute_pos3(const Seg &s, WarpLut &lut) {
    const int W = P.sw, H = P.sh;
    if (P.depth.empty() || P.dw <= 0 || P.dh <= 0 || P.dfx <= 0.0f || W <= 0 || H <= 0 || (int) P.depth.size() < P.dw * P.dh) return;
    const int gw = lut.gw, gh = lut.gh;
    const size_t gn = (size_t) gw * gh;
    const double fx = (double) P.dfx * (double) W / (double) P.dw, cx = 0.5 * W, cy = 0.5 * H;
    const double k = P.tr.cal.kok[0] ? P.tr.cal.k[0] : 0.0;
    const double bw = s.x1 - s.x0 + 1, bh = s.y1 - s.y0 + 1;
    auto depth_at = [&](double x, double y, double &z) {
        const double fxd = x * P.dw / W - 0.5, fyd = y * P.dh / H - 0.5;
        const int ix = (int) std::floor(fxd), iy = (int) std::floor(fyd);
        const double tx = fxd - ix, ty = fyd - iy;
        double sum = 0.0, sw = 0.0;
        for (int dy = 0; dy < 2; dy++) {
            for (int dx = 0; dx < 2; dx++) {
                const int xx = std::clamp(ix + dx, 0, P.dw - 1), yy = std::clamp(iy + dy, 0, P.dh - 1);
                const double zz = (double) P.depth[(size_t) yy * P.dw + xx];
                if (!(zz > 0.0) || !std::isfinite(zz)) continue;
                const double wgt = (dx ? tx : 1.0 - tx) * (dy ? ty : 1.0 - ty);
                sum += wgt * zz; sw += wgt;
            }
        }
        if (sw < 0.5) return false;
        z = sum / sw;
        return true;
    };
    std::vector<float> pos(gn * 3, 0.0f);
    for (size_t c = 0; c < gn; c++) {
        if (lut.support.size() == gn && !lut.support[c]) continue;
        const double x = (double) s.x0 + (double) lut.uv[c * 2] * bw, y = (double) s.y0 + (double) lut.uv[c * 2 + 1] * bh;
        if (x < 0.0 || y < 0.0 || x >= W || y >= H) continue;
        double z;
        if (!depth_at(x, y, z)) continue;
        double xu, yu;
        undistort_px(x, y, cx, cy, fx, k, xu, yu);
        pos[c * 3] = (float) (xu * z); pos[c * 3 + 1] = (float) (yu * z); pos[c * 3 + 2] = (float) z;
    }
    // smoothing: the average of the cells around that are at about the same depth (a depth edge stays an edge)
    std::vector<float> sm = pos;
    for (int y = 0; y < gh; y++) {
        for (int x = 0; x < gw; x++) {
            const size_t c = (size_t) y * gw + x;
            if (pos[c * 3 + 2] <= 0.0f) continue;
            double sx = 0.0, sy = 0.0, sz = 0.0;
            int n = 0;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    const int xx = x + dx, yy = y + dy;
                    if (xx < 0 || yy < 0 || xx >= gw || yy >= gh) continue;
                    const size_t cc = (size_t) yy * gw + xx;
                    if (pos[cc * 3 + 2] <= 0.0f || std::fabs(pos[cc * 3 + 2] - pos[c * 3 + 2]) > 0.1f * pos[c * 3 + 2]) continue;
                    sx += pos[cc * 3]; sy += pos[cc * 3 + 1]; sz += pos[cc * 3 + 2];
                    n++;
                }
            }
            if (n) { sm[c * 3] = (float) (sx / n); sm[c * 3 + 1] = (float) (sy / n); sm[c * 3 + 2] = (float) (sz / n); }
        }
    }
    long nv = 0;
    double mx = 0.0, my = 0.0, mz = 0.0;
    for (size_t c = 0; c < gn; c++) {
        if (sm[c * 3 + 2] <= 0.0f) continue;
        nv++;
        const float u = lut.uv[c * 2], v = lut.uv[c * 2 + 1];
        if (u >= 0.0f && u <= 1.0f && v >= 0.0f && v <= 1.0f) { mx += sm[c * 3]; my += sm[c * 3 + 1]; mz += sm[c * 3 + 2]; }
    }
    if (nv < 200 || mz <= 0.0) return;
    // the shape's own cells: the middle of them is where the viewer's orbit starts
    long nc = 0;
    for (size_t c = 0; c < gn; c++) {
        const float u = lut.uv[c * 2], v = lut.uv[c * 2 + 1];
        if (sm[c * 3 + 2] > 0.0f && u >= 0.0f && u <= 1.0f && v >= 0.0f && v <= 1.0f) nc++;
    }
    if (nc < 50) return;
    lut.pivot[0] = (float) (mx / nc); lut.pivot[1] = (float) (my / nc); lut.pivot[2] = (float) (mz / nc);
    lut.hfov = (float) (2.0 * std::atan(0.5 * (double) W / fx) * 180.0 / 3.14159265358979);
    lut.pos3.swap(sm);
}

// The surface of a shape as the webcam sees it (the Warp of a mask in the Mapping view): the stripe map says for every pixel of
// the shape which beamer position lights it; this turns it around, for a grid over the beamer image: where in the shape's box in
// the webcam image (0..1 across it) the beamer cell lies. The pixels are splatted onto the grid, the holes (cells the shape's
// pixels did not reach, and the surroundings) are filled from their neighbours, and the result is smoothed a little. Null when
// the map reads too little of the shape.
std::shared_ptr<WarpLut> build_warp_lut(const Seg &s) {
    const ScanData &S = P.tr.sd[0];
    const int W = P.sw, H = P.sh;
    if (W <= 0 || H <= 0 || S.scanBx.size() < (size_t) W * H || S.scanBy.size() < (size_t) W * H || s.mask.size() < (size_t) W * H) return nullptr;
    if (s.x1 < s.x0 || s.y1 < s.y0) return nullptr;
    const int bw = s.x1 - s.x0 + 1, bh = s.y1 - s.y0 + 1;
    if (bw < 8 || bh < 8) return nullptr;
    const int gw = std::clamp(P.beamw / 4, 64, 512);
    const int gh = std::max(36, (int) std::lround((double) gw * (double) (P.beamh > 0 ? P.beamh : 1080) / (double) (P.beamw > 0 ? P.beamw : 1920)));
    const size_t gn = (size_t) gw * gh;
    std::vector<float> au(gn, 0.0f), av(gn, 0.0f), aw(gn, 0.0f);
    auto inmask = [&](int x, int y) { return x >= 0 && y >= 0 && x < W && y < H && s.mask[(size_t) y * W + x] >= 128; };
    long used = 0;
    for (int y = s.y0; y <= s.y1; y++) {
        for (int x = s.x0; x <= s.x1; x++) {
            if (!inmask(x, y)) continue;
            if (!inmask(x - 2, y) || !inmask(x + 2, y) || !inmask(x, y - 2) || !inmask(x, y + 2)) continue;   // (the edge also sees what lies behind)
            const size_t i = (size_t) y * W + x;
            const float bxn = S.scanBx[i], byn = S.scanBy[i];
            if (bxn < 0.0f || byn < 0.0f || bxn > 1.0f || byn > 1.0f) continue;
            const float gx = bxn * (float) gw - 0.5f, gy = byn * (float) gh - 0.5f;
            const int ix = (int) std::floor(gx), iy = (int) std::floor(gy);
            const float fx = gx - (float) ix, fy = gy - (float) iy;
            const float u = ((float) x + 0.5f - (float) s.x0) / (float) bw, v = ((float) y + 0.5f - (float) s.y0) / (float) bh;
            for (int dy = 0; dy < 2; dy++) {
                for (int dx = 0; dx < 2; dx++) {
                    const int cx = ix + dx, cy = iy + dy;
                    if (cx < 0 || cy < 0 || cx >= gw || cy >= gh) continue;
                    const float wgt = (dx ? fx : 1.0f - fx) * (dy ? fy : 1.0f - fy);
                    const size_t c = (size_t) cy * gw + cx;
                    au[c] += wgt * u; av[c] += wgt * v; aw[c] += wgt;
                }
            }
            used++;
        }
    }
    if (used < 200) return nullptr;
    auto lut = std::make_shared<WarpLut>();
    lut->gw = gw; lut->gh = gh;
    lut->aspect = (float) bw / (float) bh;
    lut->uv.assign(gn * 2, 0.0f);
    std::vector<uint8_t> valid(gn, 0);
    for (size_t c = 0; c < gn; c++) {
        if (aw[c] > 0.05f) {
            lut->uv[c * 2] = au[c] / aw[c];
            lut->uv[c * 2 + 1] = av[c] / aw[c];
            valid[c] = 1;
        }
    }
    long nvalid = 0;
    for (size_t c = 0; c < gn; c++) nvalid += valid[c];
    long mcount = 0;
    for (int y = s.y0; y <= s.y1; y++) for (int x = s.x0; x <= s.x1; x++) if (inmask(x, y)) mcount++;
    // wrong decodes (a stripe period off) are isolated cells that differ from the median of their neighbours: replaced by it
    {
        std::vector<float> nu = lut->uv;
        long fixed = 0;
        for (int y = 0; y < gh; y++) {
            for (int x = 0; x < gw; x++) {
                const size_t c = (size_t) y * gw + x;
                if (!valid[c]) continue;
                float us[9], vs[9];
                int n = 0;
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        const int xx = x + dx, yy = y + dy;
                        if (xx < 0 || yy < 0 || xx >= gw || yy >= gh) continue;
                        const size_t cc = (size_t) yy * gw + xx;
                        if (!valid[cc]) continue;
                        us[n] = lut->uv[cc * 2]; vs[n] = lut->uv[cc * 2 + 1];
                        n++;
                    }
                }
                if (n < 5) continue;
                std::nth_element(us, us + n / 2, us + n);
                std::nth_element(vs, vs + n / 2, vs + n);
                if (std::fabs(lut->uv[c * 2] - us[n / 2]) > 0.03f || std::fabs(lut->uv[c * 2 + 1] - vs[n / 2]) > 0.03f) {
                    nu[c * 2] = us[n / 2]; nu[c * 2 + 1] = vs[n / 2];
                    fixed++;
                }
            }
        }
        lut->uv.swap(nu);
        if (fixed) fprintf(stderr, "[ProjMap] surface: %ld isolated cells that did not fit their neighbours were corrected\n", fixed);
    }
    // The cells without data: close to data (up to 10 cells, 40 beamer pixels) the surface goes on as a plane through the nearest
    // cells with data (a local affine fit: it carries on the slope and does not smear the edge value outwards); further away
    // there is nothing known, and the cells are not used (support 0: nothing is projected there)
    lut->support.assign(gn, 0);
    std::vector<int> dist(gn, -1);
    {
        std::vector<size_t> q;
        for (size_t c = 0; c < gn; c++) if (valid[c]) { dist[c] = 0; lut->support[c] = 1; q.push_back(c); }
        for (size_t h = 0; h < q.size(); h++) {
            const size_t c = q[h];
            const int x = (int) (c % gw), y = (int) (c / gw);
            const int nx[4] = {-1, 1, 0, 0}, ny[4] = {0, 0, -1, 1};
            for (int d = 0; d < 4; d++) {
                const int xx = x + nx[d], yy = y + ny[d];
                if (xx < 0 || yy < 0 || xx >= gw || yy >= gh) continue;
                const size_t cc = (size_t) yy * gw + xx;
                if (dist[cc] >= 0) continue;
                dist[cc] = dist[c] + 1;
                q.push_back(cc);
            }
        }
    }
    const int RE = 10, RW = 12;
    std::vector<uint8_t> orig = valid;   // (the cells with data: the fits use only those)
    std::vector<float> ext(lut->uv.size(), 0.0f);
    std::vector<size_t> extc;
    for (int y = 0; y < gh; y++) {
        for (int x = 0; x < gw; x++) {
            const size_t c = (size_t) y * gw + x;
            if (valid[c] || dist[c] < 0 || dist[c] > RE) continue;
            double M[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0}, ru[3] = {0, 0, 0}, rv[3] = {0, 0, 0};
            double sw = 0.0, su = 0.0, sv = 0.0;
            int n = 0;
            for (int dy = -RW; dy <= RW; dy++) {
                for (int dx = -RW; dx <= RW; dx++) {
                    const int xx = x + dx, yy = y + dy;
                    if (xx < 0 || yy < 0 || xx >= gw || yy >= gh) continue;
                    const size_t cc = (size_t) yy * gw + xx;
                    if (!orig[cc]) continue;
                    const double wgt = 1.0 / (1.0 + (double) (dx * dx + dy * dy) / 16.0);
                    const double p[3] = {(double) dx, (double) dy, 1.0};
                    for (int a = 0; a < 3; a++) {
                        for (int b = 0; b < 3; b++) M[a * 3 + b] += wgt * p[a] * p[b];
                        ru[a] += wgt * p[a] * lut->uv[cc * 2];
                        rv[a] += wgt * p[a] * lut->uv[cc * 2 + 1];
                    }
                    sw += wgt; su += wgt * lut->uv[cc * 2]; sv += wgt * lut->uv[cc * 2 + 1];
                    n++;
                }
            }
            if (n == 0) continue;
            float ou = (float) (su / sw), ov = (float) (sv / sw);   // (fallback: the weighted mean)
            const double det = M[0] * (M[4] * M[8] - M[5] * M[7]) - M[1] * (M[3] * M[8] - M[5] * M[6]) + M[2] * (M[3] * M[7] - M[4] * M[6]);
            if (n >= 8 && std::fabs(det) > 1e-6 * sw * sw * sw) {
                // (the value at the cell itself: the constant term, by Cramer's rule)
                auto solve_c = [&](const double r[3]) {
                    const double Mc[9] = {M[0], M[1], r[0], M[3], M[4], r[1], M[6], M[7], r[2]};
                    return (Mc[0] * (Mc[4] * Mc[8] - Mc[5] * Mc[7]) - Mc[1] * (Mc[3] * Mc[8] - Mc[5] * Mc[6]) + Mc[2] * (Mc[3] * Mc[7] - Mc[4] * Mc[6])) / det;
                };
                const double cu = solve_c(ru), cv = solve_c(rv);
                if (std::isfinite(cu) && std::isfinite(cv)) { ou = (float) cu; ov = (float) cv; }
            }
            ext[c * 2] = ou; ext[c * 2 + 1] = ov;
            extc.push_back(c);
        }
    }
    for (size_t c : extc) {
        lut->uv[c * 2] = ext[c * 2]; lut->uv[c * 2 + 1] = ext[c * 2 + 1];
        valid[c] = 1;
        lut->support[c] = 1;
    }
    // the rest: filled from the neighbours (only so that the lookup is smooth up to the edge of what is used; not used itself)
    for (int pass = 0; pass < 2000; pass++) {
        std::vector<size_t> fillc;
        std::vector<float> fu, fv;
        for (int y = 0; y < gh; y++) {
            for (int x = 0; x < gw; x++) {
                const size_t c = (size_t) y * gw + x;
                if (valid[c]) continue;
                float su = 0.0f, sv = 0.0f;
                int n = 0;
                const int nx[4] = {-1, 1, 0, 0}, ny[4] = {0, 0, -1, 1};
                for (int d = 0; d < 4; d++) {
                    const int xx = x + nx[d], yy = y + ny[d];
                    if (xx < 0 || yy < 0 || xx >= gw || yy >= gh) continue;
                    const size_t cc = (size_t) yy * gw + xx;
                    if (!valid[cc]) continue;
                    su += lut->uv[cc * 2]; sv += lut->uv[cc * 2 + 1];
                    n++;
                }
                if (n == 0) continue;
                fillc.push_back(c); fu.push_back(su / n); fv.push_back(sv / n);
            }
        }
        if (fillc.empty()) break;
        for (size_t k = 0; k < fillc.size(); k++) {
            lut->uv[fillc[k] * 2] = fu[k];
            lut->uv[fillc[k] * 2 + 1] = fv[k];
            valid[fillc[k]] = 1;
        }
    }
    {
        long ns = 0;
        for (size_t c = 0; c < gn; c++) ns += lut->support[c];
        fprintf(stderr, "[ProjMap] surface: %ld of the shape's %ld pixels had a position on the beamer; the lookup has data in %ld cells, goes on (as a plane) in %ld more, %ld cells of %zu are not used\n",
                used, mcount, nvalid, ns - nvalid, (long) gn - ns, gn);
    }
    // smoothed a little (3 x 3)
    {
        std::vector<float> sm(lut->uv.size());
        for (int y = 0; y < gh; y++) {
            for (int x = 0; x < gw; x++) {
                float su = 0.0f, sv = 0.0f;
                int n = 0;
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        const int xx = x + dx, yy = y + dy;
                        if (xx < 0 || yy < 0 || xx >= gw || yy >= gh) continue;
                        const size_t cc = (size_t) yy * gw + xx;
                        su += lut->uv[cc * 2]; sv += lut->uv[cc * 2 + 1];
                        n++;
                    }
                }
                const size_t c = (size_t) y * gw + x;
                sm[c * 2] = su / n; sm[c * 2 + 1] = sv / n;
            }
        }
        lut->uv.swap(sm);
    }
    // the surface in 3D (for the Viewpoint), from the AI depth
    compute_pos3(s, *lut);
    // the planes (real proportions), from the AI depth and from the two webcams, when there is what they need
    V3d nmoge;
    const bool hasmoge = moge_plane(s, *lut, lut->plane[0], nmoge);
    if (P.tr.scan2) stereo_plane(s, *lut, lut->plane[1], hasmoge ? &nmoge : nullptr);
    return lut;
}

// all knots of a shape are found: it is resolved. Its outline on the beamer and its layer go into the mapped list, from
// where the layer is projected on it, whatever happens to the current layer or to the segmentation.
void finalize_seg(Seg &s) {
    s.resolving = false;
    s.resolved = true;
    s.selected = false;
    Mapped m;
    m.kb = straighten_knots(s);
    apply_verify(s, m.kb);   // (what the check on the beamer found)
    m.kcorner = s.kcorner;
    m.lay = s.lay;
    if (s.mapidx >= 0 && s.mapidx < (int) P.mapped.size()) P.mapped[s.mapidx] = m;   // (resolved again: replaces the old one)
    else {
        s.mapidx = (int) P.mapped.size();
        P.mapped.push_back(m);
        // the Mapping view remembers the outline of every shape resolved on this beamer
        if (P.bdisp >= 0 && P.beamw > 0 && P.beamh > 0) {
            std::vector<float> poly;
            closed_curve(m.kb, m.kcorner, 12, poly);
            // (with its surface, as the webcam saw it, for the Warp of the mask)
            if (P.scene && P.sceneSent != P.scene.get()) {   // (the point cloud of this snapshot, for the Viewpoint)
                maskmap_set_scene(P.bdisp, P.scene);
                P.sceneSent = P.scene.get();
            }
            maskmap_store_outline(P.bdisp, P.beamw, P.beamh, poly, s.camidx == 0 ? build_warp_lut(s) : nullptr);
        }
    }
    P.maskdirty = true;
    P.ovdirty = true;
}

bool next_pending(int &si, int &ki) {
    for (size_t s = 0; s < P.segs.size(); s++) {
        Seg &sg = P.segs[s];
        if (!sg.resolving || sg.traced || sg.kcam.empty()) continue;   // (only shapes that RESOLVE was clicked for)
        for (size_t k = 0; k < sg.kdone.size(); k++) {
            if (!sg.kdone[k]) { si = (int) s; ki = (int) k; return true; }
        }
        sg.traced = true;   // (all outline points there)
        P.maskdirty = true;
    }
    return false;
}

// what the beamer shows when nothing is being steered (white is only for the moment of the snapshot, see snapWait):
// the projection, else black
BeamMode idle_mode() {
    if (P.expose) return BM_WHITE;   // (the EXPOSE button: the grey for setting the exposure)
    if (P.preview && (int) (P.preview->value + 0.5f) == 1) return BM_WHITE;   // (Preview "Map": the scene evenly lit, no layer on it, so that the webcam image under the painted map is clean)
    if (!P.mapped.empty()) return BM_PROJECT;   // (the resolved shapes keep their layers running)
    return BM_BLACK;
}

void finish_trace() {
    Tracer &T = P.tr;
    T.st = Tracer::IDLE;
    P.bmode = idle_mode();
    P.maskdirty = true;
    set_step(5, 2, "Projection ready");
}

// ======================================================================================================
// The check: where does the outline really land?
// The stripe map can carry a small systematic error (the whole outline then lands shifted). So when all outline points of a
// shape are read, the beamer shows the outline of the shape as a thin white line, the webcam looks at where that line really
// is, and the difference is measured and logged (nothing is corrected from it).
// ======================================================================================================

bool needs_verify() {
    for (const Seg &s : P.segs) if (s.resolving && s.traced && !s.verified) return true;
    return false;
}

// the outline of the shape as it will be projected, drawn as a white line (4 beamer pixels) into verifytex
void upload_verify_outline(const Seg &s) {
    std::vector<float> kb = straighten_knots(s);
    apply_verify(s, kb);
    std::vector<float> pts;
    closed_curve(kb, s.kcorner, 12, pts);
    const int mw = std::max(64, P.beamw > 2560 ? P.beamw / 2 : P.beamw), mh = std::max(64, P.beamw > 2560 ? P.beamh / 2 : P.beamh);   // (full beamer resolution: the edge of the mask is as fine as the beamer can show it)
    std::vector<uint8_t> cov((size_t) mw * mh, 0);
    fill_polygon(cov, mw, mh, pts);
    std::vector<uint8_t> rgba((size_t) mw * mh * 4, 0);
    for (int y = 0; y < mh; y++) {
        for (int x = 0; x < mw; x++) {
            bool edge = false;
            for (int dy = -1; dy <= 1 && !edge; dy++)
                for (int dx = -1; dx <= 1 && !edge; dx++) {
                    int xx = x + dx, yy = y + dy;
                    if (xx < 0 || yy < 0 || xx >= mw || yy >= mh) continue;
                    if (cov[(size_t) yy * mw + xx] != cov[(size_t) y * mw + x]) edge = true;
                }
            if (edge) {
                size_t o = ((size_t) y * mw + x) * 4;
                rgba[o] = rgba[o + 1] = rgba[o + 2] = rgba[o + 3] = 255;
            }
        }
    }
    if (P.verifytex == (GLuint) -1) {
        glGenTextures(1, &P.verifytex);
        glBindTexture(GL_TEXTURE_2D, P.verifytex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    glBindTexture(GL_TEXTURE_2D, P.verifytex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA_INTERNAL, mw, mh, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

// Where does the white outline seen by the webcam lie compared with where the outline of the shape (moved inwards by the
// inset, as the knots are) should be? At every point of the outline, along its normal (up to 20 px either way), the position
// of the lit line gives an offset d (positive: the lit line is outside the target). Every knot gets the median of the offsets
// around it, as a displacement of its own: nothing global is fitted, so one edge can be off while the others are right (an
// edge next to a side face, a different depth), and any shape can be corrected. Knots without enough measurements take the
// average of their neighbours'. u: 2 numbers per knot, webcam pixels; rms their root mean square; sectors: the mean offset
// of the points whose normal points left / right / up / down (for the log). False with too few usable points.
// RRpx: how far either way the lit line is looked for. dOff / nOut (when given): for every one of the D outline points the offset
// found (webcam px, + = outside the target; 1e9 where the line was not found) and the outward normal (2 numbers per point): what
// the closed loop (apply_closed_loop) corrects with.
bool verify_local(const Seg &s, const std::vector<uint8_t> &f, const std::vector<uint8_t> &bg, const std::vector<float> *lit,
                  int W, int H, std::vector<float> &u, float *rms, float *sectors, int RRpx = 20, std::vector<float> *dOff = nullptr,
                  std::vector<float> *nOut = nullptr) {
    const int D = (int) s.dense.size() / 2, nk = (int) s.kcam.size() / 2;
    if (D < 24 || nk < 3 || f.size() < (size_t) W * H * 4 || bg.size() != f.size()) return false;
    const bool flat = lit && lit->size() >= (size_t) W * H;
    auto ex = [&](int x, int y) -> float {   // (brightness excess over the dark frame, flat fielded)
        if (x < 0 || y < 0 || x >= W || y >= H) return 0.0f;
        size_t i = ((size_t) y * W + x) * 4;
        float d = 0.299f * ((float) f[i] - bg[i]) + 0.587f * ((float) f[i + 1] - bg[i + 1]) + 0.114f * ((float) f[i + 2] - bg[i + 2]);
        if (flat) {
            float l = (*lit)[(size_t) y * W + x];
            return 100.0f * d * l / (l * l + 400.0f);
        }
        return d;
    };
    auto sample = [&](double x, double y) {   // (bilinear, pixel centres at +0.5)
        double fx = x - 0.5, fy = y - 0.5;
        int xa = (int) std::floor(fx), ya = (int) std::floor(fy);
        double tx = fx - xa, ty = fy - ya;
        return (1 - ty) * ((1 - tx) * ex(xa, ya) + tx * ex(xa + 1, ya)) + ty * ((1 - tx) * ex(xa, ya + 1) + tx * ex(xa + 1, ya + 1));
    };
    struct Smp { double qx, qy, nx, ny, d; int i; };
    std::vector<Smp> sm;
    const float cs = cam_scale(W);                // (sizes in webcam pixels are for 2560 px wide images)
    const int RR = std::max(8, RRpx);               // (how far either way the lit line is looked for)
    if (dOff) dOff->assign((size_t) D, 1e9f);
    if (nOut) nOut->assign((size_t) D * 2, 0.0f);
    double perimeter = 0;
    for (int i = 0; i < D; i++) {
        int b = (i + 1) % D;
        perimeter += std::hypot((s.dense[b * 2] - s.dense[i * 2]) * W, (s.dense[b * 2 + 1] - s.dense[i * 2 + 1]) * H);
    }
    for (int i = 0; i < D; i++) {
        int a = (i - 3 + D) % D, b = (i + 3) % D;
        double tx = (s.dense[b * 2] - s.dense[a * 2]) * W, ty = (s.dense[b * 2 + 1] - s.dense[a * 2 + 1]) * H;
        double l = std::hypot(tx, ty);
        if (l < 1e-6) continue;
        tx /= l; ty /= l;
        double qx0 = s.dense[i * 2] * W, qy0 = s.dense[i * 2 + 1] * H;
        double nxo = ty, nyo = -tx;
        float in[2];
        inward_dir(s, (float) qx0, (float) qy0, W, H, in);
        if (in[0] == 0.0f && in[1] == 0.0f) continue;
        if (nxo * in[0] + nyo * in[1] > 0) { nxo = -nxo; nyo = -nyo; }   // (outward)
        // (where the projected line should be, in the frame being looked at: the snapshot point moved the way the webcam image moved)
        const double *Mf = P.tr.camMf;
        const double tqx = Mf[0] * qx0 + Mf[1] * qy0 + Mf[2], tqy = Mf[3] * qx0 + Mf[4] * qy0 + Mf[5];
        double qx = tqx, qy = tqy;
        std::vector<float> prof(2 * RR + 1);
        float bv = 0.0f;
        for (int o = -RR; o <= RR; o++) {
            prof[o + RR] = (float) sample(qx + nxo * o, qy + nyo * o);
            bv = std::max(bv, prof[o + RR]);
        }
        if (bv < 20.0f) continue;   // (nothing there)
        // the strong peak nearest to where the line should be, not simply the brightest: a bright reflection further off must not win
        int bi = -1;
        for (int o = -RR + 1; o <= RR - 1; o++) {
            const float v = prof[o + RR];
            if (v < 0.5f * bv || v < prof[o + RR - 1] || v < prof[o + RR + 1]) continue;
            if (bi < 0 || std::abs(o) < std::abs(bi - RR)) bi = o + RR;
        }
        if (bi <= 1 || bi >= 2 * RR - 1) continue;   // (no peak, or it is at the border)
        double sw = 0, so = 0;
        const int pk = std::max(3, (int) std::lround(3.0f * cs));   // (the line is wider on a larger webcam image)
        for (int o = std::max(0, bi - pk); o <= std::min(2 * RR, bi + pk); o++) { sw += prof[o]; so += prof[o] * (o - RR); }
        if (sw <= 0) continue;
        sm.push_back({qx, qy, nxo, nyo, so / sw, i});
        if (dOff) (*dOff)[i] = (float) (so / sw);
        if (nOut) { (*nOut)[i * 2] = (float) nxo; (*nOut)[i * 2 + 1] = (float) nyo; }
    }
    if (sm.size() < 40) return false;
    // what every outline point measured (webcam pixels, positive = the lit line is outside the target), for the log: the overall
    // picture, and where the worst points are (position in the webcam image 0..1, offset along the outward normal)
    {
        std::vector<std::pair<float, int>> by;   // |offset|, index into sm
        std::vector<float> ds;
        double sum = 0.0, suma = 0.0;
        for (size_t q = 0; q < sm.size(); q++) {
            by.push_back({(float) std::fabs(sm[q].d), (int) q});
            ds.push_back((float) sm[q].d);
            sum += sm[q].d;
            suma += std::fabs(sm[q].d);
        }
        std::sort(by.begin(), by.end(), [](const std::pair<float, int> &a, const std::pair<float, int> &b) { return a.first > b.first; });
        std::sort(ds.begin(), ds.end());
        P.vdiag.clear();   // (drawn over the webcam image, see projmap_ui)
        for (const Smp &p : sm) {
            P.vdiag.push_back((float) p.qx);
            P.vdiag.push_back((float) p.qy);
            P.vdiag.push_back((float) (p.nx * p.d));
            P.vdiag.push_back((float) (p.ny * p.d));
            P.vdiag.push_back((float) p.d);
        }
        std::string worst;
        for (size_t q = 0; q < by.size() && q < 8; q++) {
            const Smp &p = sm[by[q].second];
            char buf[80];
            snprintf(buf, sizeof(buf), " (%.2f,%.2f) %+.0f", p.qx / (double) W, p.qy / (double) H, p.d);
            worst += buf;
        }
        fprintf(stderr, "[ProjMap] outline check, %zu of %d outline points measured (webcam px of %dx%d, + = lit line outside the target): mean %+.1f, mean |.| %.1f, "
                        "median %+.1f, 10%% below %+.1f, 10%% above %+.1f; worst (x,y offset):%s\n",
                sm.size(), D, W, H, sum / (double) sm.size(), suma / (double) sm.size(), ds[ds.size() / 2], ds[ds.size() / 10],
                ds[ds.size() * 9 / 10], worst.c_str());
    }
    // the sectors, for the log
    {
        double sum[4] = {0, 0, 0, 0};
        int cnt[4] = {0, 0, 0, 0};
        for (const Smp &p : sm) {
            if (p.nx < -0.7) { sum[0] += p.d; cnt[0]++; }
            if (p.nx > 0.7) { sum[1] += p.d; cnt[1]++; }
            if (p.ny < -0.7) { sum[2] += p.d; cnt[2]++; }
            if (p.ny > 0.7) { sum[3] += p.d; cnt[3]++; }
        }
        for (int k = 0; k < 4; k++) sectors[k] = cnt[k] ? (float) (sum[k] / cnt[k]) : 0.0f;
    }
    // every knot: the displacement that explains the offsets of the points around it, n . u = d in the least squares sense
    // (outliers dropped). Along an edge all normals are alike, so only the displacement along them is determined and that is
    // what is taken (the median offset); at a corner the normals differ and the full 2D displacement is found.
    std::vector<double> kx(nk), ky(nk);
    for (int k = 0; k < nk; k++) { kx[k] = s.kcam[k * 2] * W; ky[k] = s.kcam[k * 2 + 1] * H; }
    const double reach = 1.5 * perimeter / (double) nk;
    std::vector<std::vector<Smp>> near_(nk);
    for (const Smp &p : sm) {
        int bk = -1;
        double bd = 1e18;
        for (int k = 0; k < nk; k++) {
            double d2 = (p.qx - kx[k]) * (p.qx - kx[k]) + (p.qy - ky[k]) * (p.qy - ky[k]);
            if (d2 < bd) { bd = d2; bk = k; }
        }
        if (bk < 0 || std::sqrt(bd) > reach) continue;
        near_[bk].push_back(p);
    }
    u.assign((size_t) nk * 2, 0.0f);
    std::vector<char> have(nk, 0);
    for (int k = 0; k < nk; k++) {
        std::vector<Smp> pts = near_[k];
        if (pts.size() < 3) continue;
        double ux = 0, uy = 0;
        bool solved = false;
        for (int pass = 0; pass < 3; pass++) {
            double a11 = 0, a12 = 0, a22 = 0, b1 = 0, b2 = 0;
            for (const Smp &p : pts) { a11 += p.nx * p.nx; a12 += p.nx * p.ny; a22 += p.ny * p.ny; b1 += p.nx * p.d; b2 += p.ny * p.d; }
            double det = a11 * a22 - a12 * a12;
            double n = (double) pts.size();
            if (det > 0.15 * n * n * 0.25) {
                // (the normals differ enough: a corner) full 2D
                ux = (b1 * a22 - b2 * a12) / det;
                uy = (a11 * b2 - a12 * b1) / det;
            }
            else {
                // (an edge) along the mean normal, by the median offset
                std::vector<double> dd;
                double mnx = 0, mny = 0;
                for (const Smp &p : pts) { dd.push_back(p.d); mnx += p.nx; mny += p.ny; }
                std::sort(dd.begin(), dd.end());
                double nl = std::hypot(mnx, mny);
                if (nl < 1e-6) break;
                ux = dd[dd.size() / 2] * mnx / nl;
                uy = dd[dd.size() / 2] * mny / nl;
            }
            solved = true;
            if (pass == 2) break;
            std::vector<double> r;
            for (const Smp &p : pts) r.push_back(std::fabs(p.nx * ux + p.ny * uy - p.d));
            std::vector<double> sr = r;
            std::sort(sr.begin(), sr.end());
            double lim = 2.5 * sr[sr.size() / 2] + 0.5;
            std::vector<Smp> keep;
            for (size_t i = 0; i < pts.size(); i++) if (r[i] <= lim) keep.push_back(pts[i]);
            if (keep.size() < 3) break;
            pts.swap(keep);
        }
        if (!solved) continue;
        u[k * 2] = (float) ux;
        u[k * 2 + 1] = (float) uy;
        have[k] = 1;
    }
    int nh = 0;
    for (int k = 0; k < nk; k++) nh += have[k];
    if (nh < (nk * 2) / 5) return false;
    // knots without measurements: the average of the nearest ones on both sides
    for (int k = 0; k < nk; k++) {
        if (have[k]) continue;
        int a = -1, b = -1;
        for (int j = 1; j < nk && a < 0; j++) if (have[(k - j + nk) % nk]) a = (k - j + nk) % nk;
        for (int j = 1; j < nk && b < 0; j++) if (have[(k + j) % nk]) b = (k + j) % nk;
        float ux = 0, uy = 0;
        int c = 0;
        if (a >= 0) { ux += u[a * 2]; uy += u[a * 2 + 1]; c++; }
        if (b >= 0) { ux += u[b * 2]; uy += u[b * 2 + 1]; c++; }
        if (c) { u[k * 2] = ux / c; u[k * 2 + 1] = uy / c; }
    }
    double ss = 0;
    for (int k = 0; k < nk; k++) ss += (double) u[k * 2] * u[k * 2] + (double) u[k * 2 + 1] * u[k * 2 + 1];
    *rms = (float) std::sqrt(ss / nk);
    return true;
}

void begin_verify(int si) {
    Tracer &T = P.tr;
    T.vseg = si;
    upload_verify_outline(P.segs[si]);
    P.bmode = BM_VERIFY;
    T.tset = std::chrono::steady_clock::now() + std::chrono::milliseconds(600);
    T.cset = P.cap.counter.load();
    T.st = Tracer::VERIFY_WAIT;
    set_step(5, 1, "Checking where the outline lands...");
}

// all outline points of the shapes that are being resolved are there: their outlines are checked one by one, then it is done
void after_all_knots() {
    for (size_t i = 0; i < P.segs.size(); i++) {
        Seg &s = P.segs[i];
        // (the outline check only measures, and logs, where the lit outline lands: nothing is corrected from it)
        if (s.resolving && s.traced && !s.verified) { begin_verify((int) i); return; }
    }
    finish_trace();
}

// box blur (radius r) of an image of floats, edges clamped
void blur_box(std::vector<float> &a, int W, int H, int r) {
    std::vector<float> tmp(a.size());
    std::vector<double> pre((size_t) std::max(W, H) + 1);
    for (int y = 0; y < H; y++) {
        pre[0] = 0.0;
        for (int x = 0; x < W; x++) pre[x + 1] = pre[x] + a[(size_t) y * W + x];
        for (int x = 0; x < W; x++) {
            int lo = std::max(0, x - r), hi = std::min(W - 1, x + r);
            tmp[(size_t) y * W + x] = (float) ((pre[hi + 1] - pre[lo]) / (double) (hi - lo + 1));
        }
    }
    for (int x = 0; x < W; x++) {
        pre[0] = 0.0;
        for (int y = 0; y < H; y++) pre[y + 1] = pre[y] + tmp[(size_t) y * W + x];
        for (int y = 0; y < H; y++) {
            int lo = std::max(0, y - r), hi = std::min(H - 1, y + r);
            a[(size_t) y * W + x] = (float) ((pre[hi + 1] - pre[lo]) / (double) (hi - lo + 1));
        }
    }
}

// ======================================================================================================
// Did the webcam image move?
// The shape is segmented on the snapshot, the stripe scan is taken later: if the webcam changed its magnification (autofocus
// breathing) or shifted in between, a point of the snapshot is somewhere else in the scan frames, and every outline lands off
// by an amount that grows with the distance from the centre of the image. The grey reference frame at the start of the run (the
// same light as the snapshot) is compared with the snapshot by matching blocks (zero-mean normalized correlation, coarse then
// finer, to a fraction of a pixel), and an affine motion is fitted through the shifts of the blocks.
// ======================================================================================================

void lum_down(const std::vector<uint8_t> &img, int W, int H, int f, std::vector<float> &out, int &w, int &h) {
    w = W / f;
    h = H / f;
    out.assign((size_t) w * h, 0.0f);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            double s = 0.0;
            for (int dy = 0; dy < f; dy++) {
                const uint8_t *row = &img[((size_t) (y * f + dy) * W + (size_t) x * f) * 4];
                for (int dx = 0; dx < f; dx++) s += 0.299 * row[dx * 4] + 0.587 * row[dx * 4 + 1] + 0.114 * row[dx * 4 + 2];
            }
            out[(size_t) y * w + x] = (float) (s / (double) (f * f));
        }
    }
}

// the block of A at (x0, y0) of size bs against B, shifted by (cx, cy) plus up to sr either way: the best shift (sub-pixel) and
// its correlation; false when the block has no structure or nothing correlates
bool match_block(const std::vector<float> &A, const std::vector<float> &B, int w, int h, int x0, int y0, int bs, int cx, int cy, int sr,
                 double &odx, double &ody, double &ocorr) {
    if (x0 < 0 || y0 < 0 || x0 + bs > w || y0 + bs > h) return false;
    double ma = 0.0;
    for (int y = 0; y < bs; y++) for (int x = 0; x < bs; x++) ma += A[(size_t) (y0 + y) * w + x0 + x];
    ma /= (double) (bs * bs);
    double na = 0.0;
    for (int y = 0; y < bs; y++) for (int x = 0; x < bs; x++) { double d = A[(size_t) (y0 + y) * w + x0 + x] - ma; na += d * d; }
    if (na < (double) (bs * bs) * 9.0) return false;   // (a block of nothing: less than 3 levels of spread)
    const int n = 2 * sr + 1;
    std::vector<double> cc((size_t) n * n, -2.0);
    double best = -2.0;
    int bsx = 0, bsy = 0;
    for (int sy = -sr; sy <= sr; sy++) {
        for (int sx = -sr; sx <= sr; sx++) {
            const int bx0 = x0 + cx + sx, by0 = y0 + cy + sy;
            if (bx0 < 0 || by0 < 0 || bx0 + bs > w || by0 + bs > h) continue;
            double mb = 0.0;
            for (int y = 0; y < bs; y++) for (int x = 0; x < bs; x++) mb += B[(size_t) (by0 + y) * w + bx0 + x];
            mb /= (double) (bs * bs);
            double nb = 0.0, cr = 0.0;
            for (int y = 0; y < bs; y++) {
                for (int x = 0; x < bs; x++) {
                    const double da = A[(size_t) (y0 + y) * w + x0 + x] - ma, db = B[(size_t) (by0 + y) * w + bx0 + x] - mb;
                    nb += db * db;
                    cr += da * db;
                }
            }
            if (nb < 1e-9) continue;
            const double c = cr / std::sqrt(na * nb);
            cc[(size_t) (sy + sr) * n + (sx + sr)] = c;
            if (c > best) { best = c; bsx = sx; bsy = sy; }
        }
    }
    if (best < 0.5) return false;
    double fx = 0.0, fy = 0.0;   // (a parabola through the best and its neighbours: the sub-pixel shift)
    if (bsx > -sr && bsx < sr) {
        const double l = cc[(size_t) (bsy + sr) * n + (bsx - 1 + sr)], r = cc[(size_t) (bsy + sr) * n + (bsx + 1 + sr)];
        const double den = l - 2.0 * best + r;
        if (l > -1.5 && r > -1.5 && den < -1e-9) fx = std::clamp(0.5 * (l - r) / den, -0.5, 0.5);
    }
    if (bsy > -sr && bsy < sr) {
        const double u = cc[(size_t) (bsy - 1 + sr) * n + (bsx + sr)], d = cc[(size_t) (bsy + 1 + sr) * n + (bsx + sr)];
        const double den = u - 2.0 * best + d;
        if (u > -1.5 && d > -1.5 && den < -1e-9) fy = std::clamp(0.5 * (u - d) / den, -0.5, 0.5);
    }
    odx = cx + bsx + fx;
    ody = cy + bsy + fy;
    ocorr = best;
    return true;
}

// The motion of the webcam image from the snapshot P.snap to a frame of the same webcam (same size, same light): M such that a
// point p of the snapshot is at M p in the frame. False when the frames cannot be compared (then the identity stays).
bool estimate_cam_motion(const std::vector<uint8_t> &frame, int W, int H, double *M, const char *what) {
    M[0] = 1; M[1] = 0; M[2] = 0; M[3] = 0; M[4] = 1; M[5] = 0;
    if (P.snap.size() != (size_t) W * H * 4 || frame.size() != (size_t) W * H * 4 || P.sw != W || P.sh != H) return false;
    const int fc = std::max(1, W / 480), ff = std::max(1, fc / 4);   // (coarse and fine image: every fc / ff pixels averaged)
    std::vector<float> Ac, Bc, Af, Bf;
    int wc, hc, wf, hf;
    lum_down(P.snap, W, H, fc, Ac, wc, hc);
    lum_down(frame, W, H, fc, Bc, wc, hc);
    lum_down(P.snap, W, H, ff, Af, wf, hf);
    lum_down(frame, W, H, ff, Bf, wf, hf);
    struct Pt { double x, y, dx, dy; };   // block centre in the snapshot (webcam pixels) and its shift
    std::vector<Pt> pts;
    const int nx = 6, ny = 4, bsc = 40, srC = 6;
    const int mc = 8;   // (coarse pixels at the border where there are no blocks)
    for (int j = 0; j < ny; j++) {
        for (int i = 0; i < nx; i++) {
            const int x0 = mc + i * (wc - 2 * mc - bsc) / (nx - 1), y0 = mc + j * (hc - 2 * mc - bsc) / (ny - 1);
            double dxc, dyc, cc;
            if (!match_block(Ac, Bc, wc, hc, x0, y0, bsc, 0, 0, srC, dxc, dyc, cc)) continue;
            // the fine image: around where the coarse one says, over the same piece of the scene
            const double k = (double) fc / (double) ff;
            const int bsf = (int) std::lround(bsc * k), xf = (int) std::lround(x0 * k), yf = (int) std::lround(y0 * k);
            double dxf, dyf, cf;
            if (!match_block(Af, Bf, wf, hf, xf, yf, bsf, (int) std::lround(dxc * k), (int) std::lround(dyc * k), 3, dxf, dyf, cf)) continue;
            if (cf < 0.6) continue;
            pts.push_back({(xf + 0.5 * bsf) * ff, (yf + 0.5 * bsf) * ff, dxf * ff, dyf * ff});
        }
    }
    if (pts.size() < 8) {
        fprintf(stderr, "[ProjMap] webcam motion (%s): only %zu blocks of the image could be compared, assumed not to have moved\n", what, pts.size());
        return false;
    }
    // x' = a x + b y + tx, y' = c x + d y + ty by least squares (the blocks whose shift disagrees are dropped, twice)
    std::vector<char> keep(pts.size(), 1);
    double co[6] = {1, 0, 0, 0, 1, 0};
    for (int pass = 0; pass < 3; pass++) {
        double A[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}}, r0[3] = {0, 0, 0}, r1[3] = {0, 0, 0};
        int cnt = 0;
        for (size_t q = 0; q < pts.size(); q++) {
            if (!keep[q]) continue;
            cnt++;
            const double v[3] = {pts[q].x, pts[q].y, 1.0};
            for (int a = 0; a < 3; a++) {
                for (int b = 0; b < 3; b++) A[a][b] += v[a] * v[b];
                r0[a] += v[a] * (pts[q].x + pts[q].dx);
                r1[a] += v[a] * (pts[q].y + pts[q].dy);
            }
        }
        if (cnt < 6) return false;
        const double det = A[0][0] * (A[1][1] * A[2][2] - A[1][2] * A[2][1]) - A[0][1] * (A[1][0] * A[2][2] - A[1][2] * A[2][0]) +
                           A[0][2] * (A[1][0] * A[2][1] - A[1][1] * A[2][0]);
        if (std::fabs(det) < 1e-9 * (A[0][0] * A[1][1] * A[2][2] + 1e-12)) return false;
        auto solve = [&](const double *r, double *out) {
            for (int c = 0; c < 3; c++) {
                double Mx[3][3];
                for (int a = 0; a < 3; a++) for (int b = 0; b < 3; b++) Mx[a][b] = (b == c) ? r[a] : A[a][b];
                out[c] = (Mx[0][0] * (Mx[1][1] * Mx[2][2] - Mx[1][2] * Mx[2][1]) - Mx[0][1] * (Mx[1][0] * Mx[2][2] - Mx[1][2] * Mx[2][0]) +
                          Mx[0][2] * (Mx[1][0] * Mx[2][1] - Mx[1][1] * Mx[2][0])) / det;
            }
        };
        solve(r0, co);
        solve(r1, co + 3);
        std::vector<double> res(pts.size());
        std::vector<double> sorted;
        for (size_t q = 0; q < pts.size(); q++) {
            const double ex = co[0] * pts[q].x + co[1] * pts[q].y + co[2] - (pts[q].x + pts[q].dx);
            const double ey = co[3] * pts[q].x + co[4] * pts[q].y + co[5] - (pts[q].y + pts[q].dy);
            res[q] = std::hypot(ex, ey);
            if (keep[q]) sorted.push_back(res[q]);
        }
        std::sort(sorted.begin(), sorted.end());
        const double lim = std::max(2.0, 2.5 * sorted[sorted.size() / 2]);
        for (size_t q = 0; q < pts.size(); q++) keep[q] = res[q] <= lim ? 1 : 0;
    }
    double ss = 0.0;
    int cnt = 0;
    for (size_t q = 0; q < pts.size(); q++) {
        if (!keep[q]) continue;
        const double ex = co[0] * pts[q].x + co[1] * pts[q].y + co[2] - (pts[q].x + pts[q].dx);
        const double ey = co[3] * pts[q].x + co[4] * pts[q].y + co[5] - (pts[q].y + pts[q].dy);
        ss += ex * ex + ey * ey;
        cnt++;
    }
    const double scale = std::sqrt(std::fabs(co[0] * co[4] - co[1] * co[3]));
    // how far the image moved at its centre and at its corners, for the log
    auto disp = [&](double x, double y) { return std::hypot(co[0] * x + co[1] * y + co[2] - x, co[3] * x + co[4] * y + co[5] - y); };
    fprintf(stderr, "[ProjMap] webcam motion (%s): scale %.5f, shift at the centre %.1f px, at the corners %.1f / %.1f / %.1f / %.1f px (webcam image %dx%d); "
                    "%d of %zu blocks, fit rms %.2f px\n", what, scale, disp(W * 0.5, H * 0.5), disp(0, 0), disp(W, 0), disp(0, H), disp(W, H), W, H, cnt, pts.size(),
            cnt > 0 ? std::sqrt(ss / cnt) : 0.0);
    if (std::fabs(scale - 1.0) > 0.1 || disp(W * 0.5, H * 0.5) > 0.2 * W) return false;   // (not a believable motion)
    for (int k = 0; k < 6; k++) M[k] = co[k];
    return true;
}

// the scan map (beamer position per webcam pixel of the scan frames) as seen from the snapshot: the pixel p of the snapshot
// takes the value of the scan frame at M p
void warp_scan_map(ScanData &S, int W, int H, const double *M) {
    const size_t n = (size_t) W * H;
    if (S.scanBx.size() < n || S.scanSig.size() < n) return;
    std::vector<float> nbx(n, -1.0f), nby(n, -1.0f);
    std::vector<uint8_t> nsig(n, 255);
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            const double qx = M[0] * x + M[1] * y + M[2], qy = M[3] * x + M[4] * y + M[5];
            const int x0 = (int) std::floor(qx), y0 = (int) std::floor(qy);
            if (x0 < 0 || y0 < 0 || x0 + 1 >= W || y0 + 1 >= H) continue;
            const double tx = qx - x0, ty = qy - y0;
            const size_t i00 = (size_t) y0 * W + x0, i10 = i00 + 1, i01 = i00 + W, i11 = i01 + 1;
            const size_t o = (size_t) y * W + x;
            if (S.scanBx[i00] >= 0.0f && S.scanBx[i10] >= 0.0f && S.scanBx[i01] >= 0.0f && S.scanBx[i11] >= 0.0f) {
                const double w00 = (1 - tx) * (1 - ty), w10 = tx * (1 - ty), w01 = (1 - tx) * ty, w11 = tx * ty;
                nbx[o] = (float) (w00 * S.scanBx[i00] + w10 * S.scanBx[i10] + w01 * S.scanBx[i01] + w11 * S.scanBx[i11]);
                nby[o] = (float) (w00 * S.scanBy[i00] + w10 * S.scanBy[i10] + w01 * S.scanBy[i01] + w11 * S.scanBy[i11]);
                nsig[o] = std::max(std::max(S.scanSig[i00], S.scanSig[i10]), std::max(S.scanSig[i01], S.scanSig[i11]));
            }
            else {   // (a hole next to it: the nearest pixel, when that has a value)
                const size_t ni = (size_t) std::clamp((int) std::lround(qy), 0, H - 1) * W + (size_t) std::clamp((int) std::lround(qx), 0, W - 1);
                if (S.scanBx[ni] >= 0.0f) { nbx[o] = S.scanBx[ni]; nby[o] = S.scanBy[ni]; nsig[o] = S.scanSig[ni]; }
            }
        }
    }
    S.scanBx.swap(nbx);
    S.scanBy.swap(nby);
    S.scanSig.swap(nsig);
}

// the scene lit evenly white, at the brightness of the Exposure slider (the same light as the snapshot)
void begin_ref() {
    Tracer &T = P.tr;
    const float v = std::clamp(P.light->value, 0.15f, 0.8f);
    P.fillcol[0] = P.fillcol[1] = P.fillcol[2] = v;
    P.fillcol[3] = 1.0f;
    P.bmode = BM_FILL;
    T.tset = std::chrono::steady_clock::now() + std::chrono::milliseconds(1000);   // (the webcam adapts to the brighter scene)
    T.cset = P.cap.counter.load();
    T.st = Tracer::REF_WAIT;
    set_step(5, 1, "Lighting the scene...");
}

void begin_scan();

// Method "Knots curve" (else "Outline")
bool knots_method() {
    return P.method && (int) (P.method->value + 0.5f) == 1;
}

// Webcam 2 is used when it is streaming (for the Outline method: both webcams read the outline)
bool cam2_active() {
    return P.cap2.state == 2 && !knots_method();
}

// ======================================================================================================
// The stripe scan
// The beamer shows sine stripes, the webcam sees them on whatever they fall on, and from the phase of the stripes every
// webcam pixel learns which beamer position lights it. Nothing is assumed about the shape or its depth: the map is
// measured pixel by pixel. Six stripe periods (19 to 113 beamer pixels, no two sharing a factor) and four phase shifts each,
// for x and for y: 48 patterns. Every pixel measures the phase of each period, and its beamer position is the one where
// all six agree (the phases of different periods only agree at one place, over a stretch much longer than the beamer).
// No pattern is coarser than 113 pixels, so a webcam that sees only a small part of the beamer image still sees whole
// periods: the average brightness of what it sees stays the same from pattern to pattern, and its auto exposure has nothing
// to react to. The knots are then read off the map next to the webcam position of the knot, with a small robust local fit
// through the pixels just inside the segment.
// ======================================================================================================

// the stripe periods, in beamer pixels (the same for x and y)
// The first kScanFine are the fine ones (7 and 11 px): they only refine, where they can be read (a webcam that is zoomed in
// shows them wide; one that is not shows them squeezed to nothing), and never take part in finding the position, so a pixel
// that cannot read them is no worse off than before.
const int kScanLevels = 8;
const int kScanFine = 2;
// (the periods above are for a beamer 1920 pixels wide; for other resolutions they are scaled with the width, so that the stripes
// are as wide on the wall, and the webcam sees them as wide, whatever the resolution of the beamer: a 4K beamer would otherwise
// lose the fine ones)
const double kScanPeriod0[kScanLevels] = {7.0, 11.0, 19.0, 26.0, 37.0, 53.0, 79.0, 113.0};
double scan_scale() { return std::clamp((P.beamw > 0 ? (double) P.beamw : 1920.0) / 1920.0, 0.5, 4.0); }
double scan_period(int lev) { return kScanPeriod0[lev] * scan_scale(); }
int scan_levels() { return kScanLevels; }
int scan_patterns() { return 2 * kScanLevels * 4; }

// periods across the whole beamer image along an axis, for a level
double scan_freq(int axis, int lev) {
    const double n = axis == 0 ? (double) (P.beamw > 0 ? P.beamw : 1920) : (double) (P.beamh > 0 ? P.beamh : 1080);
    return n / scan_period(lev);
}
bool g_scanquiet = false;   // no log line per point (the outline method reads hundreds)

void upload_scan_pattern(int idx) {
    const int L = scan_levels();
    const int axis = idx / (L * 4), lev = (idx % (L * 4)) / 4, step = idx % 4;
    const int mw = std::max(64, P.beamw > 2560 ? P.beamw / 2 : P.beamw), mh = std::max(64, P.beamw > 2560 ? P.beamh / 2 : P.beamh);
    const float g = P.tr.scanG > 0.0f ? P.tr.scanG : std::clamp(P.light->value, 0.15f, 0.8f);
    const int n = axis == 0 ? mw : mh;
    std::vector<uint8_t> line(n);
    for (int i = 0; i < n; i++) {
        double u = ((double) i + 0.5) / (double) n;
        double v = 0.5 + 0.5 * std::cos(2.0 * 3.14159265358979 * scan_freq(axis, lev) * u + step * 0.5 * 3.14159265358979);
        line[i] = (uint8_t) std::lround(255.0 * g * v);
    }
    std::vector<uint8_t> rgba((size_t) mw * mh * 4, 255);
    for (int y = 0; y < mh; y++) {
        for (int x = 0; x < mw; x++) {
            uint8_t v = line[axis == 0 ? x : y];
            size_t o = ((size_t) y * mw + x) * 4;
            rgba[o] = rgba[o + 1] = rgba[o + 2] = v;
        }
    }
    if (P.scantex == (GLuint) -1) {
        glGenTextures(1, &P.scantex);
        glBindTexture(GL_TEXTURE_2D, P.scantex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    glBindTexture(GL_TEXTURE_2D, P.scantex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA_INTERNAL, mw, mh, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

// One camera frame of the pattern idx into the scan data of that webcam; after the fourth shift of a frequency the phase is
// computed. False when the webcam changed its image size during the scan. *bad (when given) tells, at the fourth shift,
// that the four frames do not fit a stripe pattern shifted by quarter periods (see below): a frame was stale or the light
// changed, and the four frames should be taken again.
bool scan_frame(ScanData &S, const std::vector<uint8_t> &frame, int W, int H, int idx, bool *bad = nullptr) {
    const size_t n = (size_t) W * H;
    if (S.scanW == 0) {
        S.scanW = W; S.scanH = H;
        for (auto &a : S.scanPh) for (auto &b : a) b.assign(n, 0);
        for (auto &a : S.scanMod) for (auto &b : a) b.assign(n, 0);
        S.scanSat.assign(n, 0);
        S.scanSatG.assign(n, 0);
        S.scanSatCnt.assign(n, 0);
    }
    if (W != S.scanW || H != S.scanH || frame.size() < n * 4) return false;
    const int L = scan_levels();
    const int axis = idx / (L * 4), lev = (idx % (L * 4)) / 4, step = idx % 4;
    std::vector<uint8_t> &f = S.scanF[step];
    f.resize(n);
    if (step == 0) std::fill(S.scanSatG.begin(), S.scanSatG.end(), 0);
    for (size_t i = 0; i < n; i++) {
        const uint8_t *px = &frame[i * 4];
        f[i] = (uint8_t) ((299 * px[0] + 587 * px[1] + 114 * px[2]) / 1000);
        if (px[0] >= 250 || px[1] >= 250 || px[2] >= 250) S.scanSatG[i] = 1;   // (spoils this stripe period here, not the pixel)
    }
    if (step == 3) {
        // phase of the stripes from the four shifts: I_k = A + B cos(theta + k pi/2)  =>  theta = atan2(I3 - I1, I0 - I2)
        const float twopi = 6.28318530718f;
        // (each frame is blurred a little first, 5x5: the noise of a pixel is mostly independent of its neighbours', the
        // stripes are many pixels wide, and every frame gets the same blur, so the phase is not shifted by it)
        std::vector<float> bf[4];
        for (int q = 0; q < 4; q++) {
            bf[q].resize(n);
            for (size_t i = 0; i < n; i++) bf[q][i] = (float) S.scanF[q][i];
            blur_box(bf[q], W, H, std::max(1, (int) std::lround(((lev < 2) ? 1.0f : 2.0f) * cam_scale(W))));   // (the finest stripes are narrow: a smaller blur keeps their contrast; the size is on the object, so it scales with the webcam)
        }
        // For four frames of A + B cos(theta + k pi/2) the sums of opposite frames are equal: I0 + I2 = I1 + I3 = 2A. How far
        // they are from it, compared with the size of the stripes, tells whether the four frames belong together. A frame that
        // still showed the previous pattern, or a change of the light between them, breaks it at every pixel at once.
        double sumres = 0.0, summod = 0.0;
        for (size_t i = 0; i < n; i++) {
            float sx = bf[0][i] - bf[2][i];
            float sy = bf[3][i] - bf[1][i];
            float th = std::atan2(sy, sx) / twopi;
            if (th < 0.0f) th += 1.0f;
            S.scanPh[axis][lev][i] = (uint16_t) ((int) std::lround(th * 65536.0f) & 0xFFFF);
            const float mf = std::hypot(sx, sy);   // (= 2 B)
            int m = (int) std::lround(mf);
            if (S.scanSatG[i]) { m = 0; if (S.scanSatCnt[i] < 255) S.scanSatCnt[i]++; }
            S.scanMod[axis][lev][i] = (uint8_t) std::clamp(m, 0, 255);
            if (mf > 16.0f && !S.scanSatG[i]) {
                sumres += std::fabs(bf[0][i] + bf[2][i] - bf[1][i] - bf[3][i]);
                summod += mf;
            }
        }
        if (bad) *bad = summod > 0.0 && sumres / summod > 0.35;
    }
    return true;
}

void show_scan_pattern(int idx) {
    Tracer &T = P.tr;
    upload_scan_pattern(idx);
    P.bmode = BM_PATTERN;
    T.tset = std::chrono::steady_clock::now() + std::chrono::milliseconds(idx == 0 ? 600 : 0);
    T.cset = P.cap.counter.load();
    T.cset2 = P.cap2.counter.load();
    set_step(5, 1, "Scanning stripes " + std::to_string(idx + 1) + "/" + std::to_string(scan_patterns()), (float) idx / (float) scan_patterns());
}

// The brightness of the stripes: the Exposure slider, lowered when the shapes about to be scanned are bright enough to clip the
// camera at the peaks of the stripes (a glossy rim, a highlight): a pixel that clips in the stripe patterns gets no position.
// The reference frame lit white at the Exposure level is as bright as the peaks of the stripes will be. Pixels in the shapes,
// per colour channel; if more than 3% are above 225 of 255, the brightness is scaled so that that 97th percentile lands on 225
// (the camera answers about linearly to the beamer's drive value: the beamer's gamma and the camera's cancel).
float scan_brightness() {
    const float g0 = std::clamp(P.light->value, 0.15f, 0.8f);
    Tracer &T = P.tr;
    const std::vector<uint8_t> &ref = T.ref;
    if (!T.haveRef || P.sw <= 0 || ref.size() != (size_t) P.sw * P.sh * 4) return g0;
    std::vector<long> hist(256, 0);
    long cnt = 0;
    for (const Seg &s : P.segs) {
        if (!s.resolving || s.traced || s.mask.size() != (size_t) P.sw * P.sh) continue;
        for (int y = std::max(0, s.y0); y <= std::min(P.sh - 1, s.y1); y++) {
            for (int x = std::max(0, s.x0); x <= std::min(P.sw - 1, s.x1); x++) {
                const size_t i = (size_t) y * P.sw + x;
                if (s.mask[i] < 128) continue;
                hist[std::max(ref[i * 4], std::max(ref[i * 4 + 1], ref[i * 4 + 2]))]++;
                cnt++;
            }
        }
    }
    if (cnt < 1000) return g0;
    long acc = 0;
    int p = 255;
    for (int v = 0; v < 256; v++) {
        acc += hist[v];
        if ((double) acc >= 0.97 * (double) cnt) { p = v; break; }
    }
    if (p <= 225) return g0;
    const float g = std::clamp(g0 * 225.0f / (float) std::max(p, 1), 0.06f, g0);
    long clipped = 0;
    for (int v = 250; v < 256; v++) clipped += hist[v];
    fprintf(stderr, "[ProjMap] scan: %.1f%% of the shape's pixels would clip at the stripe brightness %.2f (97th percentile %d of 255): the stripes are shown at %.2f\n",
            100.0 * (double) clipped / (double) cnt, g0, p, g);
    return g;
}

void begin_scan() {
    Tracer &T = P.tr;
    T.scanG = scan_brightness();
    T.scanIdx = 0;
    T.scanRetry = 0;
    T.scan2 = cam2_active();   // (webcam 2 looks at the stripes too)
    for (ScanData &S : T.sd) S = ScanData();
    T.st = Tracer::SCAN_WAIT;
    show_scan_pattern(0);
}

// f(begin, end) over [0, n) split into pieces, one thread each (the decoding of millions of pixels is the long part of a scan)
template <class F> void parallel_range(size_t n, F f) {
    const unsigned nt = std::max(1u, std::min(8u, std::thread::hardware_concurrency()));
    const size_t chunk = (n + nt - 1) / nt;
    std::vector<std::thread> th;
    for (unsigned t = 0; t < nt; t++) {
        const size_t b = (size_t) t * chunk, e = std::min(n, b + chunk);
        if (b >= e) break;
        th.emplace_back([&f, b, e]() { f(b, e); });
    }
    for (std::thread &x : th) x.join();
}

// the map from the phases of both axes
void decode_scan(ScanData &S, int W, int H) {
    Tracer &T = P.tr;
    const size_t n = (size_t) W * H;
    S.scanBx.assign(n, -1.0f);
    S.scanBy.assign(n, -1.0f);
    const float inv = 1.0f / 65536.0f;
    const double sc = scan_scale();   // (the stripes are scaled with the resolution of the beamer: so are the distances and the exactness measured in beamer pixels)
    double kScanPeriod[kScanLevels];
    for (int k = 0; k < kScanLevels; k++) kScanPeriod[k] = scan_period(k);
    size_t nvalid = 0;
    S.scanLv.assign(n, 0);
    size_t rej[4] = {0, 0, 0, 0};     // (why pixels were turned down, for the log: weak stripes, no candidate, periods disagree, ambiguous)
    size_t nexact = 0;
    S.scanSig.assign(n, 255);
    S.scanWhy.assign(n, 0);
    uint8_t curwhy = 0;   // (why the pixel last looked at was turned down, see ScanData::scanWhy)
    {
    // ---- the candidate positions along each axis (every half beamer pixel) with the phase every stripe period has there,
    // listed by the phases of the two longest periods (64 x 64 buckets): a pixel only has to look at the candidates in the
    // buckets around its own two phases, a few dozen, instead of at all of them ----
    const int B = 64, pa = kScanLevels - 2, pb = kScanLevels - 1;
    struct AxisTab {
        int len = 0, nc = 0;                      // beamer pixels along the axis, candidates
        std::vector<float> fr[kScanLevels];       // per period: the phase (0..1) of every candidate
        std::vector<std::vector<int>> bucket;     // per bucket: the candidates
    } tab[2];
    for (int a = 0; a < 2; a++) {
        AxisTab &t = tab[a];
        t.len = a == 0 ? (P.beamw > 0 ? P.beamw : 1920) : (P.beamh > 0 ? P.beamh : 1080);
        t.nc = t.len * 2;
        for (int k = 0; k < kScanLevels; k++) t.fr[k].resize(t.nc);
        t.bucket.assign((size_t) B * B, std::vector<int>());
        for (int j = 0; j < t.nc; j++) {
            const double x = ((double) j + 0.5) * 0.5;
            for (int k = 0; k < kScanLevels; k++) {
                const double q = x / kScanPeriod[k];
                t.fr[k][j] = (float) (q - std::floor(q));
            }
            const int ba = std::min(B - 1, (int) (t.fr[pa][j] * B)), bb = std::min(B - 1, (int) (t.fr[pb][j] * B));
            t.bucket[(size_t) ba * B + bb].push_back(j);
        }
    }
    // The beamer position of a pixel along one axis: of the candidates the one where the phases of all the stripe periods that
    // could be read agree best; refined from the position every period implies for it (the finer the period, the more it
    // counts). It has to fit well, and clearly better than any other place at least 6 px away: a pixel that fits two places
    // equally is no information. The two finest periods (7 and 11 px) take no part in finding the place; once it is known
    // they refine it if their phase agrees with it (a webcam that does not resolve them gives a phase that does not). sig: how
    // exact the position is in beamer pixels, from the periods that counted (every period reads its phase to about 0.03 cycles).
    // prior: where the neighbour pixel was (beamer pixels, or -1): the position of two neighbouring webcam pixels differs by a few
    // beamer pixels at most, so most pixels do not need the search: every period is unwrapped around the neighbour's position,
    // and if they all agree that is the position (a couple of dozen operations instead of a few thousand). Only where there is no
    // neighbour yet, or the periods do not agree with it (an edge, a depth jump), the full search is made.
    auto decode_axis = [&](size_t i, int a, double prior, float &out, float &sig) -> bool {
        float ph[kScanLevels];
        bool use[kScanLevels];
        int nuse = 0;
        for (int k = 0; k < kScanLevels; k++) {
            use[k] = S.scanMod[a][k][i] >= 8;   // (too weak: shadow, dark surface)
            ph[k] = (float) S.scanPh[a][k][i] * inv;
            if (k >= kScanFine) nuse += use[k] ? 1 : 0;
        }
        if (!use[pa] || !use[pb] || nuse < 4) { rej[0]++; curwhy = 1; return false; }
        const AxisTab &t = tab[a];
        double x0 = 0.0;
        bool fast = false;
        if (prior >= 0.0) {
            double sx0 = 0.0, sw0 = 0.0, xk0[kScanLevels];
            for (int k = kScanFine; k < kScanLevels; k++) {
                if (!use[k]) continue;
                xk0[k] = kScanPeriod[k] * (std::round(prior / kScanPeriod[k] - (double) ph[k]) + (double) ph[k]);
                const double w = 1.0 / (kScanPeriod[k] * kScanPeriod[k]);
                sx0 += w * xk0[k];
                sw0 += w;
            }
            const double xm = sx0 / sw0;
            double cost0 = 0.0;
            for (int k = kScanFine; k < kScanLevels; k++) {
                if (!use[k]) continue;
                const double d = (xk0[k] - xm) / kScanPeriod[k];
                cost0 += d * d;
            }
            if (cost0 <= 0.0064 * (double) nuse) { x0 = xm; fast = true; }   // (rms under 0.08 cycles: as for the full search)
        }
        if (!fast) {
        const int ba = std::min(B - 1, (int) (ph[pa] * B)), bb = std::min(B - 1, (int) (ph[pb] * B));
        struct Cand { float cost; int j; };
        Cand cs[640];   // (the candidates are every half beamer pixel: a high resolution beamer has more in a bucket)
        int nc = 0;
        for (int da = -3; da <= 3; da++) {
            for (int db = -3; db <= 3; db++) {
                const std::vector<int> &lst = t.bucket[(size_t) ((ba + da + B) % B) * B + (bb + db + B) % B];
                for (int j : lst) {
                    float cost = 0.0f;
                    for (int k = kScanFine; k < kScanLevels; k++) {
                        if (!use[k]) continue;
                        float d = t.fr[k][j] - ph[k];
                        d -= std::round(d);
                        cost += d * d;
                    }
                    if (nc < 640) cs[nc++] = {cost, j};
                }
            }
        }
        if (nc == 0) { rej[1]++; curwhy = 2; return false; }
        int b0 = 0;
        for (int c = 1; c < nc; c++) if (cs[c].cost < cs[b0].cost) b0 = c;
        x0 = ((double) cs[b0].j + 0.5) * 0.5;
        float second = 1e9f;
        for (int c = 0; c < nc; c++) {
            if (std::fabs(((double) cs[c].j + 0.5) * 0.5 - x0) > 6.0 * sc && cs[c].cost < second) second = cs[c].cost;
        }
        const float best = cs[b0].cost;
        if (best > 0.0064f * (float) nuse) { rej[2]++; curwhy = 3; return false; }                    // (the periods do not agree: rms over 0.08 cycles)
        if (second < 1.6f * best + 0.0015f * (float) nuse) { rej[3]++; curwhy = 4; return false; }    // (another place fits about as well)
        }   // (end of the full search)
        double sx = 0.0, sw = 0.0;
        for (int k = kScanFine; k < kScanLevels; k++) {
            if (!use[k]) continue;
            const double xk = kScanPeriod[k] * (std::round(x0 / kScanPeriod[k] - (double) ph[k]) + (double) ph[k]);
            const double w = 1.0 / (kScanPeriod[k] * kScanPeriod[k]);
            sx += w * xk;
            sw += w;
        }
        double xr = sx / sw;
        // the fine periods, the longer one first: each is unwrapped around the position so far (which is good to well under half
        // a period) and counts if its phase agrees with it
        for (int k = kScanFine - 1; k >= 0; k--) {
            if (!use[k]) continue;
            const double q = xr / kScanPeriod[k] - (double) ph[k];
            const double d = q - std::round(q);   // (cycles)
            if (std::fabs(d) > 0.14) continue;
            const double xk = kScanPeriod[k] * (std::round(q) + (double) ph[k]);
            const double w = 1.0 / (kScanPeriod[k] * kScanPeriod[k]);
            sx += w * xk;
            sw += w;
            xr = sx / sw;
        }
        out = (float) std::clamp(xr / (double) t.len, 0.0, 1.0);
        sig = (float) (0.03 / std::sqrt(sw));
        return true;
    };
    for (size_t i = 0; i < n; i++) {
        if (S.scanSat[i]) continue;
        float res[2], sg[2];
        // (the neighbour to the left, else the one above, if it was decoded: where to start from)
        double pr0 = -1.0, pr1 = -1.0;
        if (i % (size_t) W > 0 && S.scanBx[i - 1] >= 0.0f) { pr0 = (double) S.scanBx[i - 1] * tab[0].len; pr1 = (double) S.scanBy[i - 1] * tab[1].len; }
        else if (i >= (size_t) W && S.scanBx[i - W] >= 0.0f) { pr0 = (double) S.scanBx[i - W] * tab[0].len; pr1 = (double) S.scanBy[i - W] * tab[1].len; }
        if (!decode_axis(i, 0, pr0, res[0], sg[0]) || !decode_axis(i, 1, pr1, res[1], sg[1])) { S.scanWhy[i] = curwhy; continue; }
        S.scanBx[i] = res[0];
        S.scanBy[i] = res[1];
        const float sgm = std::max(sg[0], sg[1]);
        S.scanSig[i] = (uint8_t) std::clamp((int) std::lround(sgm * 20.0f), 1, 255);
        S.scanLv[i] = sgm < 0.8f * sc ? 3 : 2;
        nvalid++;
        if (sgm < 0.5f * sc) nexact++;
    }
    }   // (end of the decode of the pixels)
    fprintf(stderr, "[ProjMap] scan: %.1f%% of the decoded pixels are exact to under half a beamer pixel (the fine stripes could be read there)\n",
            100.0 * (double) nexact / (double) std::max<size_t>(1, nvalid));
    fprintf(stderr, "[ProjMap] scan: %.1f%% of the webcam pixels decoded; axes turned down: stripes too weak %zu, no candidate %zu, periods disagree %zu, ambiguous %zu\n",
            100.0 * (double) nvalid / (double) std::max<size_t>(1, n), rej[0], rej[1], rej[2], rej[3]);
    // A pixel whose position does not agree with what its neighbours say is a wrong stripe (a jump of a whole period):
    // dropped. (Pixels at a depth edge go too; the knots do not need them.)
    {
        std::vector<float> nbx(S.scanBx), nby(S.scanBy);
        size_t dropped = 0;
        // (the 24 neighbours lie 2 steps either way, the step being larger on a larger webcam image: the same stretch of the object)
        const int st = std::max(1, (int) std::lround(cam_scale(W)));
        for (int y = 2 * st; y < H - 2 * st; y++) {
            for (int x = 2 * st; x < W - 2 * st; x++) {
                size_t i = (size_t) y * W + x;
                if (S.scanBx[i] < 0.0f) continue;
                double sx = 0, sy = 0;
                int c = 0;
                for (int dy = -2; dy <= 2; dy++) {
                    const size_t r = (size_t) (y + dy * st) * W + x;
                    for (int dx = -2; dx <= 2; dx++) {
                        if (!dx && !dy) continue;
                        const size_t q = r + (size_t) ((long) dx * st);
                        if (S.scanBx[q] < 0.0f) continue;
                        sx += S.scanBx[q]; sy += S.scanBy[q];
                        c++;
                    }
                }
                // (the more exact a pixel is, the closer it has to agree with its neighbours: 8 times its precision, but never
                // under 0.004 = about 8 beamer pixels, as the surface itself changes from pixel to pixel too)
                const double tol = std::max(0.004, 8.0 * (double) S.scanSig[i] * 0.05 / (P.beamw > 0 ? (double) P.beamw : 1920.0));
                if (c < 6 || std::fabs(sx / c - S.scanBx[i]) > tol || std::fabs(sy / c - S.scanBy[i]) > tol) {
                    nbx[i] = nby[i] = -1.0f;
                    S.scanWhy[i] = 5;
                    dropped++;
                }
            }
        }
        S.scanBx.swap(nbx);
        S.scanBy.swap(nby);
        nvalid = nvalid > dropped ? nvalid - dropped : 0;
    }
    fprintf(stderr, "[ProjMap] scan: %.1f%% of the webcam pixels know their beamer position\n", 100.0 * (double) nvalid / (double) std::max<size_t>(1, n));
    for (auto &f : S.scanF) { f.clear(); f.shrink_to_fit(); }
    for (auto &a : S.scanPh) for (auto &b : a) { b.clear(); b.shrink_to_fit(); }
    for (auto &a : S.scanMod) for (auto &b : a) { b.clear(); b.shrink_to_fit(); }
    S.scanSat.clear(); S.scanSat.shrink_to_fit();
}

// The beamer position of every knot of a segment from the map: a distance weighted affine fit (beamer position as a
// function of webcam position) through the map pixels around the knot that lie well inside the segment, trimmed of
// outliers (pixels of another depth), evaluated at the knot.
void scan_knots(Seg &s, int W, int H, const ScanData &S, bool relaxed = false) {
    Tracer &T = P.tr;
    const int nk = (int) s.kcam.size() / 2;
    if (!relaxed) { s.kscan.assign(nk * 2, 0.0f); s.kscale.assign(nk, 1.0f); }
    if (!relaxed) s.kscanok.assign(nk, 0);
    if ((int) s.kscanok.size() != nk) return;
    if (s.mask.size() != (size_t) W * H || S.scanBx.size() < (size_t) W * H) return;
    const double bsx = P.beamw > 0 ? P.beamw : 1920.0, bsy = P.beamh > 0 ? P.beamh : 1080.0;
    // Distance of every pixel of the segment to its outline (the sub-pixel outline, not the staircase of the pixel mask): how
    // far inside the segment a map pixel lies. The fit below weights a pixel by a smooth function of that distance instead of
    // keeping or dropping it by a hard test on the pixel mask: with a hard test, the set of pixels in the fit changes in steps
    // along a slanted edge, the fit's extrapolation to the edge steps with it, and the outline wobbles evenly spaced. With
    // smooth weights the estimate moves smoothly as the point moves along the edge.
    const int dx0 = std::max(0, s.x0 - 1), dy0 = std::max(0, s.y0 - 1), dx1 = std::min(W - 1, s.x1 + 1), dy1 = std::min(H - 1, s.y1 + 1);
    const int dw = dx1 - dx0 + 1, dh = dy1 - dy0 + 1;
    const int nd = (int) s.dense.size() / 2;
    if (dw <= 0 || dh <= 0 || nd < 8) return;
    std::vector<float> dist((size_t) dw * dh, 0.0f);
    const float cs = cam_scale(W);   // (sizes below are for 2560 px wide images; see cam_scale)
    {
        std::vector<float> px(nd * 2);
        for (int i = 0; i < nd; i++) { px[i * 2] = s.dense[i * 2] * W; px[i * 2 + 1] = s.dense[i * 2 + 1] * H; }
        const int pfar = (int) std::lround(7.0f * cs), pnear = (int) std::lround(5.0f * cs);   // (what counts as deep inside)
        for (int y = dy0; y <= dy1; y++) {
            for (int x = dx0; x <= dx1; x++) {
                if (s.mask[(size_t) y * W + x] < 128) continue;
                auto in = [&](int xx, int yy) { return xx >= 0 && yy >= 0 && xx < W && yy < H && s.mask[(size_t) yy * W + xx] >= 128; };
                if (in(x - pfar, y) && in(x + pfar, y) && in(x, y - pfar) && in(x, y + pfar) && in(x - pnear, y - pnear) && in(x + pnear, y - pnear) && in(x - pnear, y + pnear) && in(x + pnear, y + pnear)) {
                    dist[(size_t) (y - dy0) * dw + (x - dx0)] = 99.0f;   // (deep inside: the exact distance does not matter, it counts fully)
                    continue;
                }
                const float cxp = x + 0.5f, cyp = y + 0.5f;
                float best = 1e9f;
                for (int i = 0; i < nd; i++) {
                    const int j = (i + 1) % nd;
                    const float ax = px[i * 2], ay = px[i * 2 + 1], ex = px[j * 2] - ax, ey = px[j * 2 + 1] - ay;
                    const float l2 = ex * ex + ey * ey;
                    float t = l2 > 1e-9f ? ((cxp - ax) * ex + (cyp - ay) * ey) / l2 : 0.0f;
                    t = std::clamp(t, 0.0f, 1.0f);
                    const float qx = ax + t * ex - cxp, qy = ay + t * ey - cyp;
                    best = std::min(best, qx * qx + qy * qy);
                }
                dist[(size_t) (y - dy0) * dw + (x - dx0)] = std::sqrt(best);
            }
        }
    }
    struct Smp { double dx, dy, bx, by, w; };
    int nok = 0;
    for (int k = 0; k < nk; k++) {
        if (relaxed && s.kscanok[k]) continue;
        const double cx = s.kcam[k * 2] * W, cy = s.kcam[k * 2 + 1] * H;
        const int radii[3] = {(int) std::lround((relaxed ? 16 : 12) * cs), (int) std::lround((relaxed ? 28 : 20) * cs), (int) std::lround((relaxed ? 44 : 30) * cs)};   // (the same window for neighbouring points where possible: windows that jump in size make the outline jump)
        // a pixel counts from d0 px inside the outline, fully from d1 px: the mask edge of SAM3 is only good to about half a
        // cell of its grid, and the frames are blurred, so a mixed pixel at the edge of a shape is a wrong one
        const double d0 = (relaxed ? 1.0 : 3.0) * cs, d1 = (relaxed ? 3.0 : 6.0) * cs;
        const size_t minpix = relaxed ? 6 : 12;
        const double maxrms = relaxed ? 6.0 : 2.5;
        for (int ri = 0; ri < 3; ri++) {
            const int R = radii[ri];
            std::vector<Smp> sm;
            const double sig = R * 0.5;
            // How deep the shape is around here: on a thin shape (a branch a dozen pixels wide) there is nothing 3 to 6 px
            // inside the outline, so the margins shrink with the depth that exists (the pixels there are as clean as thin
            // shapes allow, which is better than none)
            double dmax = 0.0;
            for (int y = std::max(dy0, (int) cy - R); y <= std::min(dy1, (int) cy + R); y++)
                for (int x = std::max(dx0, (int) cx - R); x <= std::min(dx1, (int) cx + R); x++)
                    if (s.mask[(size_t) y * W + x] >= 128) dmax = std::max(dmax, (double) dist[(size_t) (y - dy0) * dw + (x - dx0)]);
            const double e0 = std::min(d0, 0.35 * dmax), e1 = std::max(e0 + 1.0, std::min(d1, 0.8 * dmax));
            for (int y = std::max(dy0, (int) cy - R); y <= std::min(dy1, (int) cy + R); y++) {
                for (int x = std::max(dx0, (int) cx - R); x <= std::min(dx1, (int) cx + R); x++) {
                    if (s.mask[(size_t) y * W + x] < 128) continue;
                    const double dd = dist[(size_t) (y - dy0) * dw + (x - dx0)];
                    if (dd <= e0) continue;
                    size_t i = (size_t) y * W + x;
                    if (S.scanBx[i] < 0.0f) continue;
                    double dx = x + 0.5 - cx, dy = y + 0.5 - cy, d2 = dx * dx + dy * dy;
                    if (d2 > (double) R * R) continue;
                    const double u = std::clamp((dd - e0) / (e1 - e0), 0.0, 1.0), wd = u * u * (3.0 - 2.0 * u);   // (smooth step)
                    // (a pixel counts the less the less exact its position is: about 1 for a tenth of a beamer pixel, a tenth for 1.2 px)
                    const double sgp = S.scanSig.size() > i ? (double) S.scanSig[i] * 0.05 : 1.0;
                    const double wp = 1.0 / (1.0 + (sgp / 0.4) * (sgp / 0.4));
                    sm.push_back({dx, dy, S.scanBx[i] * bsx, S.scanBy[i] * bsy, wd * wp * std::exp(-d2 / (2.0 * sig * sig))});
                }
            }
            if (sm.size() < minpix) continue;
            double co[2][3] = {{0, 0, 0}, {0, 0, 0}};
            bool fitok = false;
            // iteratively reweighted least squares: a pixel that disagrees with the fit (another depth, a wrong stripe) loses weight
            // smoothly (Tukey's biweight) instead of being kept or thrown out at a threshold, which would make the result jump
            std::vector<double> rw(sm.size(), 1.0);
            std::vector<char> keep(sm.size(), 1);
            for (int pass = 0; pass < 4; pass++) {
                double A[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}}, r0[3] = {0, 0, 0}, r1[3] = {0, 0, 0};
                int cnt = 0;
                for (size_t q = 0; q < sm.size(); q++) {
                    const double wq = sm[q].w * rw[q];
                    if (wq <= 1e-9) continue;
                    cnt++;
                    const double v[3] = {1.0, sm[q].dx, sm[q].dy};
                    for (int a = 0; a < 3; a++) {
                        for (int b = 0; b < 3; b++) A[a][b] += wq * v[a] * v[b];
                        r0[a] += wq * v[a] * sm[q].bx;
                        r1[a] += wq * v[a] * sm[q].by;
                    }
                }
                if (cnt < (int) minpix - 2) { fitok = false; break; }
                double det = A[0][0] * (A[1][1] * A[2][2] - A[1][2] * A[2][1]) - A[0][1] * (A[1][0] * A[2][2] - A[1][2] * A[2][0]) +
                             A[0][2] * (A[1][0] * A[2][1] - A[1][1] * A[2][0]);
                if (std::fabs(det) < 1e-9 * (A[0][0] * A[1][1] * A[2][2] + 1e-12)) { fitok = false; break; }
                auto solve = [&](const double *r, double *out) {
                    for (int c = 0; c < 3; c++) {
                        double M[3][3];
                        for (int a = 0; a < 3; a++) for (int b = 0; b < 3; b++) M[a][b] = (b == c) ? r[a] : A[a][b];
                        out[c] = (M[0][0] * (M[1][1] * M[2][2] - M[1][2] * M[2][1]) - M[0][1] * (M[1][0] * M[2][2] - M[1][2] * M[2][0]) +
                                  M[0][2] * (M[1][0] * M[2][1] - M[1][1] * M[2][0])) / det;
                    }
                };
                solve(r0, co[0]);
                solve(r1, co[1]);
                fitok = true;
                // residuals (beamer pixels) -> weights for the next pass
                std::vector<double> res(sm.size());
                std::vector<double> sorted;
                for (size_t q = 0; q < sm.size(); q++) {
                    double ex = co[0][0] + co[0][1] * sm[q].dx + co[0][2] * sm[q].dy - sm[q].bx;
                    double ey = co[1][0] + co[1][1] * sm[q].dx + co[1][2] * sm[q].dy - sm[q].by;
                    res[q] = std::hypot(ex, ey);
                    if (sm[q].w * rw[q] > 1e-9) sorted.push_back(res[q]);
                }
                std::sort(sorted.begin(), sorted.end());
                double med = sorted[sorted.size() / 2];
                const double sc = std::sqrt(std::fabs(co[0][1] * co[1][2] - co[0][2] * co[1][1]));   // beamer px per webcam px here
                // scale of what still counts as agreeing: robust spread of the residuals, and not below the size of a webcam pixel on
                // this surface (on a stretched surface that is several beamer pixels, which is not noise)
                const double c = std::max(std::max(1.5, 4.685 * 1.4826 * med), 1.0 * sc);
                for (size_t q = 0; q < sm.size(); q++) {
                    const double u = res[q] / c;
                    rw[q] = u < 1.0 ? (1.0 - u * u) * (1.0 - u * u) : 0.0;
                    keep[q] = rw[q] > 0.0 ? 1 : 0;
                }
            }
            if (!fitok) continue;
            double ss = 0;
            int cnt = 0;
            for (size_t q = 0; q < sm.size(); q++) {
                if (!keep[q]) continue;
                double ex = co[0][0] + co[0][1] * sm[q].dx + co[0][2] * sm[q].dy - sm[q].bx;
                double ey = co[1][0] + co[1][1] * sm[q].dx + co[1][2] * sm[q].dy - sm[q].by;
                ss += ex * ex + ey * ey;
                cnt++;
            }
            if (cnt < (int) minpix - 2) continue;
            double rms = std::sqrt(ss / cnt);
            double bxn = co[0][0] / bsx, byn = co[1][0] / bsy;
            const double fsc = std::sqrt(std::fabs(co[0][1] * co[1][2] - co[0][2] * co[1][1]));
            if (rms > std::max(maxrms, 0.8 * fsc) || bxn < -0.02 || bxn > 1.02 || byn < -0.02 || byn > 1.02) continue;
            s.kscan[k * 2] = (float) bxn;
            s.kscan[k * 2 + 1] = (float) byn;
            s.kscale[k] = (float) fsc;
            s.kscanok[k] = 1;
            nok++;
            if (!g_scanquiet) fprintf(stderr, "[ProjMap] scan: segment %d knot %d%s: beamer (%.4f,%.4f) from %d map pixels within %d px, fit rms %.2f beamer px\n",
                    (s.id + 100 * s.camidx), k, (k < (int) s.kcorner.size() && s.kcorner[k]) ? " (corner)" : "", bxn, byn, cnt, R, rms);
            break;
        }
        if (!s.kscanok[k])
            if (!g_scanquiet) fprintf(stderr, "[ProjMap] scan: segment %d knot %d: no usable map around it\n", (s.id + 100 * s.camidx), k);
    }
    fprintf(stderr, "[ProjMap] scan: segment %d: %d of %d knots read off the map\n", (s.id + 100 * s.camidx), nok, nk);
}

// A knot that the map placed far from where its neighbours along the outline are is a wrong read (map pixels of a wrong
// stripe, or of another surface): it is dropped. Knots next to a corner may lie off the line of their neighbours, so they only
// have to be within reach of them.
void reject_scan_outliers(Seg &s) {
    const int n = (int) s.kscanok.size();
    if (n < 5 || (int) s.kscan.size() < n * 2) return;
    auto isc = [&](int i) { return i < (int) s.kcorner.size() && s.kcorner[i]; };
    for (int pass = 0; pass < 3; pass++) {
        std::vector<float> sp;
        for (int i = 0; i < n; i++) {
            int j = (i + 1) % n;
            if (s.kscanok[i] && s.kscanok[j]) sp.push_back(std::hypot(s.kscan[j * 2] - s.kscan[i * 2], s.kscan[j * 2 + 1] - s.kscan[i * 2 + 1]));
        }
        if (sp.size() < 3) return;
        std::sort(sp.begin(), sp.end());
        const float med = sp[sp.size() / 2];
        int dropped = 0;
        std::vector<char> bad(n, 0);
        for (int i = 0; i < n; i++) {
            if (!s.kscanok[i]) continue;
            int a = (i - 1 + n) % n, b = (i + 1) % n;
            if (!s.kscanok[a] || !s.kscanok[b]) continue;
            float da = std::hypot(s.kscan[i * 2] - s.kscan[a * 2], s.kscan[i * 2 + 1] - s.kscan[a * 2 + 1]);
            float db = std::hypot(s.kscan[i * 2] - s.kscan[b * 2], s.kscan[i * 2 + 1] - s.kscan[b * 2 + 1]);
            bool off = std::min(da, db) > 2.5f * med + 0.015f;   // (far from both neighbours)
            // (where the outline turns sharply, a branch tip or a bay, the point lies far from the midpoint of its neighbours
            // for good reason: only the "far from both neighbours" test applies there)
            bool sharp = false;
            if ((int) s.kcam.size() >= n * 2) {
                const double ux = (s.kcam[i * 2] - s.kcam[a * 2]) * 16.0, uy = (s.kcam[i * 2 + 1] - s.kcam[a * 2 + 1]) * 9.0;
                const double vx = (s.kcam[b * 2] - s.kcam[i * 2]) * 16.0, vy = (s.kcam[b * 2 + 1] - s.kcam[i * 2 + 1]) * 9.0;
                const double l = std::hypot(ux, uy) * std::hypot(vx, vy);
                if (l > 1e-12) sharp = std::acos(std::clamp((ux * vx + uy * vy) / l, -1.0, 1.0)) > 0.5;   // (about 30 degrees)
            }
            if (!off && !sharp && !isc(i) && !isc(a) && !isc(b)) {
                float mx = 0.5f * (s.kscan[a * 2] + s.kscan[b * 2]), my = 0.5f * (s.kscan[a * 2 + 1] + s.kscan[b * 2 + 1]);
                float sc = 0.5f * std::hypot(s.kscan[b * 2] - s.kscan[a * 2], s.kscan[b * 2 + 1] - s.kscan[a * 2 + 1]);
                off = std::hypot(s.kscan[i * 2] - mx, s.kscan[i * 2 + 1] - my) > 0.6f * sc + 0.015f;
            }
            if (off) bad[i] = 1;
        }
        for (int i = 0; i < n; i++) if (bad[i]) {
            s.kscanok[i] = 0;
            dropped++;
            if (!g_scanquiet) fprintf(stderr, "[ProjMap] scan: segment %d knot %d: does not fit between its neighbours, dropped\n", (s.id + 100 * s.camidx), i);
        }
        if (!dropped) break;
    }
}

// The corners of the outline: a corner of a mask is rounded by the SAM3 mask (its grid is a few webcam pixels wide at best) and
// then by the evenly spaced sampling and the smooth curve through the samples. Each corner found (a sharp bend, see
// find_corners) gets a point of its own at its tip: where the straight stretches on both sides meet. The curve keeps it sharp
// (isc), and the inset there is larger so that the lit shape stays the same distance inside both edges (fac).
void sharpen_corners(Seg &s, int W, int H, std::vector<char> &isc, std::vector<float> &fac) {
    const int D = (int) s.dense.size() / 2;
    isc.assign(D, 0);
    fac.assign(D, 1.0f);
    std::vector<int> cs;
    find_corners(s.dense, cs);
    if (cs.empty()) return;
    std::vector<float> tips(cs.size() * 2);
    for (size_t j = 0; j < cs.size(); j++) refine_corner_edges(s.dense, cs, (int) j, &tips[j * 2]);   // (all from the unchanged outline)
    for (size_t j = 0; j < cs.size(); j++) {
        const int c = cs[j];
        // the angle between the two edges at the tip: the inset across both is the inset / sin(half the angle)
        const int a = (c - 6 + D) % D, b = (c + 6) % D;
        const double v1x = (s.dense[a * 2] - tips[j * 2]) * W, v1y = (s.dense[a * 2 + 1] - tips[j * 2 + 1]) * H;
        const double v2x = (s.dense[b * 2] - tips[j * 2]) * W, v2y = (s.dense[b * 2 + 1] - tips[j * 2 + 1]) * H;
        const double l = std::hypot(v1x, v1y) * std::hypot(v2x, v2y);
        double f = 1.0;
        if (l > 1e-9) {
            const double phi = std::acos(std::clamp((v1x * v2x + v1y * v2y) / l, -1.0, 1.0));
            f = 1.0 / std::max(0.35, std::sin(0.5 * phi));
        }
        s.dense[c * 2] = tips[j * 2];
        s.dense[c * 2 + 1] = tips[j * 2 + 1];
        isc[c] = 1;
        fac[c] = (float) std::clamp(f, 1.0, 2.5);
    }
}

// Method "Outline": no knots. Every point of the dense SAM3 outline is read off the map, and
// the segment's knots become those points: the beamer outline follows the SAM3 outline point by point. False (everything as
// it was) when the map covers too little of the outline.
bool outline_from_scan(Seg &s, int W, int H, const ScanData &S) {
    const int D = (int) s.dense.size() / 2;
    if (D < 24 || s.mask.size() != (size_t) W * H) return false;
    const int si = (s.id + 100 * s.camidx);
    fprintf(stderr, "[ProjMap] scan: segment %d outline: points of the SAM3 outline, webcam image %dx%d, sizes scaled %.1fx\n", si,
            W, H, cam_scale(W));
    std::vector<float> kcam0 = s.kcam, kscan0 = s.kscan, dense0 = s.dense;
    std::vector<char> corner0 = s.kcorner, ok0 = s.kscanok;
    std::vector<char> isc;
    std::vector<float> fac;
    if (kSharpenCorners) sharpen_corners(s, W, H, isc, fac);   // (the corners get a point at their tip, kept sharp)
    else { isc.assign(D, 0); fac.assign(D, 1.0f); }
    s.kcam = s.dense;   // (the outline itself: the Inset of a mask is set in the Mapping view, after the curve edits)
    s.kcorner = isc;
    g_scanquiet = true;
    // (the relaxed fit does all of it: margins of 1 to 3 px inside the outline, windows of 16 to 44 px. The stricter one with wider
    // margins did worse, and so did a strip along the edge, which hardly read anything)
    s.kscan.assign((size_t) D * 2, 0.0f);
    s.kscale.assign(D, 1.0f);
    s.kscanok.assign(D, 0);
    scan_knots(s, W, H, S, true);
    reject_scan_outliers(s);
    g_scanquiet = false;
    int have = 0;
    for (int i = 0; i < D; i++) have += s.kscanok[i] ? 1 : 0;
    if (have < D * 3 / 10) {   // (below that the outline would be mostly guessed between a few points: the knots do better)
        fprintf(stderr, "[ProjMap] scan: segment %d outline: only %d of %d outline points could be read off the map\n", si, have, D);
        s.kcam = kcam0; s.kcorner = corner0; s.kscan = kscan0; s.kscanok = ok0; s.dense = dense0;   // (the corner tips go back too)
        return false;
    }
    int filled = 0;
    for (int k = 0; k < D; k++) {   // (points without map data: between their neighbours along the outline)
        if (s.kscanok[k]) continue;
        int a = -1, b = -1, da = 0, db = 0;
        for (int j = 1; j < D && a < 0; j++) if (s.kscanok[(k - j + D) % D]) { a = (k - j + D) % D; da = j; }
        for (int j = 1; j < D && b < 0; j++) if (s.kscanok[(k + j) % D]) { b = (k + j) % D; db = j; }
        float t = (float) da / (float) (da + db);
        s.kscan[k * 2] = s.kscan[a * 2] + t * (s.kscan[b * 2] - s.kscan[a * 2]);
        s.kscan[k * 2 + 1] = s.kscan[a * 2 + 1] + t * (s.kscan[b * 2 + 1] - s.kscan[a * 2 + 1]);
        s.kscanok[k] = 1;
        s.kscale[k] = std::max(s.kscale[a], s.kscale[b]);
        filled++;
    }
    // (no smoothing of the outline afterwards: every point is what the map says at that point, see scan_knots)
    s.kbeam = s.kscan;
    s.kdone.assign(D, 1);
    s.kretried.assign(D, 1);
    s.kcorner = isc;   // (the corner points stay sharp in the curve through them)
    {
        int ncorners = 0;
        for (int i = 0; i < D; i++) ncorners += isc[i] ? 1 : 0;
        fprintf(stderr, "[ProjMap] scan: segment %d outline: %d corner point(s) kept sharp\n", si, ncorners);
    }
    fprintf(stderr, "[ProjMap] scan: segment %d outline: %d of %d outline points read off the map, %d placed between their neighbours\n", si, have, D, filled);
    return true;
}

// Which segment of webcam 2 is the same object as segment s1 of webcam 1? Both are put on the beamer: every webcam pixel of
// the segment that knows its beamer position marks that place on a coarse grid (a little widened: a stretched surface leaves
// gaps between the pixels), and the segment of webcam 2 whose grid overlaps most (intersection over union, at least 0.4)
// wins. Nothing but the beamer is needed to compare: it is the one coordinate system both webcams share.
int match_seg2(const Seg &s1, int W1, int H1, const ScanData &S1, int W2, int H2, const ScanData &S2) {
    const int GW = 240, GH = 135;
    auto mark = [&](const Seg &s, int W, int H, const ScanData &S, std::vector<uint8_t> &g) -> int {
        g.assign((size_t) GW * GH, 0);
        if (s.mask.size() != (size_t) W * H || S.scanBx.size() < (size_t) W * H) return 0;
        int count = 0;
        for (int y = std::max(0, s.y0); y <= std::min(H - 1, s.y1); y++) {
            for (int x = std::max(0, s.x0); x <= std::min(W - 1, s.x1); x++) {
                size_t i = (size_t) y * W + x;
                if (s.mask[i] < 128 || S.scanBx[i] < 0.0f) continue;
                int cx = std::clamp((int) (S.scanBx[i] * GW), 0, GW - 1), cy = std::clamp((int) (S.scanBy[i] * GH), 0, GH - 1);
                g[(size_t) cy * GW + cx] = 1;
                count++;
            }
        }
        for (int pass = 0; pass < 2; pass++) {   // (widened by two cells)
            std::vector<uint8_t> t(g);
            for (int y = 0; y < GH; y++) {
                for (int x = 0; x < GW; x++) {
                    if (g[(size_t) y * GW + x]) continue;
                    if ((x > 0 && g[(size_t) y * GW + x - 1]) || (x < GW - 1 && g[(size_t) y * GW + x + 1]) ||
                        (y > 0 && g[(size_t) (y - 1) * GW + x]) || (y < GH - 1 && g[(size_t) (y + 1) * GW + x])) t[(size_t) y * GW + x] = 1;
                }
            }
            g.swap(t);
        }
        return count;
    };
    std::vector<uint8_t> g1, g2;
    if (mark(s1, W1, H1, S1, g1) < 50) return -1;
    int best = -1;
    double bestiou = 0.4;
    for (size_t j = 0; j < P.segs2.size(); j++) {
        if (mark(P.segs2[j], W2, H2, S2, g2) < 50) continue;
        double inter = 0, uni = 0;
        for (size_t c = 0; c < g1.size(); c++) {
            inter += (g1[c] && g2[c]) ? 1 : 0;
            uni += (g1[c] || g2[c]) ? 1 : 0;
        }
        double iou = uni > 0 ? inter / uni : 0.0;
        fprintf(stderr, "[ProjMap] scan: segment %d and webcam 2 segment %zu overlap on the beamer by %.2f\n", s1.id, j, iou);
        if (iou > bestiou) { bestiou = iou; best = (int) j; }
    }
    return best;
}

// The union of two outlines on the beamer (each is what one webcam sees of the object, so the object is what either sees):
// both are filled into one image, and the outline of that is traced and spaced out evenly again into D points.
bool fuse_outlines(const std::vector<float> &a, const std::vector<float> &b, int D, std::vector<float> &out) {
    if (a.size() < 6 || b.size() < 6 || P.beamw <= 0) return false;
    const int mw = std::max(64, P.beamw > 2560 ? P.beamw / 2 : P.beamw), mh = std::max(64, P.beamw > 2560 ? P.beamh / 2 : P.beamh);
    std::vector<uint8_t> cov((size_t) mw * mh, 0);
    const std::vector<char> none;
    for (const std::vector<float> *pts : {&a, &b}) {
        std::vector<float> curve;
        closed_curve(*pts, none, 4, curve);
        fill_polygon(cov, mw, mh, curve);
    }
    int x0 = mw, y0 = mh, x1 = -1, y1 = -1;
    for (int y = 0; y < mh; y++) {
        for (int x = 0; x < mw; x++) {
            if (cov[(size_t) y * mw + x] < 128) continue;
            x0 = std::min(x0, x); x1 = std::max(x1, x); y0 = std::min(y0, y); y1 = std::max(y1, y);
        }
    }
    if (x1 < 0) return false;
    std::vector<float> raw;
    if (!trace_outer(cov, mw, mh, x0, y0, x1, y1, raw)) return false;
    smooth_resample(raw, D, mw, mh, out);
    return out.size() == (size_t) D * 2;
}

// The map of the scan as a picture over the webcam image (Preview "Map"): where a pixel has no position, and why. Yellow: it was
// saturated in several stripe periods and has no position (glare, a highlight); blue: stripes too weak (shadow, dark surface,
// outside the beamer's light); cyan: no candidate; red: the stripe periods do not agree with each other; magenta: ambiguous;
// orange: dropped because it disagrees with its neighbours. Pixels that do have a position are faint green, faint yellow where
// one stripe period was saturated at them.
void upload_why_tex(const ScanData &S, int W, int H) {
    const size_t n = (size_t) W * H;
    if (S.scanWhy.size() < n || S.scanSatCnt.size() < n) return;
    std::vector<uint8_t> rgba(n * 4, 0);
    for (size_t i = 0; i < n; i++) {
        uint8_t r = 0, g = 0, b = 0, a = 0;
        const uint8_t why = S.scanWhy[i], sat = S.scanSatCnt[i];
        if (sat >= 2 && why != 0) { r = 255; g = 230; b = 0; a = 210; }
        else if (why == 1) { r = 40; g = 90; b = 255; a = 80; }
        else if (why == 2) { r = 0; g = 230; b = 230; a = 170; }
        else if (why == 3) { r = 255; g = 0; b = 0; a = 170; }
        else if (why == 4) { r = 255; g = 0; b = 255; a = 170; }
        else if (why == 5) { r = 255; g = 140; b = 0; a = 170; }
        else if (why == 0 && sat >= 1) { r = 255; g = 230; b = 0; a = 90; }
        else if (why == 0) { r = 0; g = 255; b = 60; a = 35; }
        rgba[i * 4] = r; rgba[i * 4 + 1] = g; rgba[i * 4 + 2] = b; rgba[i * 4 + 3] = a;
    }
    if (P.whytex == (GLuint) -1) {
        glGenTextures(1, &P.whytex);
        glBindTexture(GL_TEXTURE_2D, P.whytex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    glBindTexture(GL_TEXTURE_2D, P.whytex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA_INTERNAL, W, H, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

void finish_scan(int W, int H, int W2, int H2) {
    Tracer &T = P.tr;
    const auto tstart = std::chrono::steady_clock::now();
    decode_scan(T.sd[0], W, H);
    const bool two = T.scan2 && T.sd[1].scanW > 0 && W2 > 0;
    if (two) decode_scan(T.sd[1], W2, H2);
    fprintf(stderr, "[ProjMap] scan: decoded in %.1f s\n", std::chrono::duration<float>(std::chrono::steady_clock::now() - tstart).count());
    upload_why_tex(T.sd[0], W, H);   // (for the Preview "Map": what the scan made of every webcam pixel)
    // If the webcam image moved between the snapshot and the scan (more than 1.5 px anywhere), the map is brought back to the
    // snapshot, where the masks and outlines are; otherwise nothing is touched.
    for (int k = 0; k < 6; k++) T.camMf[k] = (k == 0 || k == 4) ? 1.0 : 0.0;
    if (T.camMok) {
        auto disp = [&](double x, double y) { return std::hypot(T.camM[0] * x + T.camM[1] * y + T.camM[2] - x, T.camM[3] * x + T.camM[4] * y + T.camM[5] - y); };
        const double worst = std::max(std::max(disp(0, 0), disp(W, 0)), std::max(disp(0, H), disp(W, H)));
        if (worst > 1.5) {
            fprintf(stderr, "[ProjMap] scan: the webcam image moved by up to %.1f px since the snapshot: the map is brought back to the snapshot\n", worst);
            warp_scan_map(T.sd[0], W, H, T.camM);
            for (int k = 0; k < 6; k++) T.camMf[k] = T.camM[k];
        }
    }
    T.scanned = true;
    P.bmode = BM_BLACK;
    calibrate_cameras();   // (lens distortion of the webcams, and their focal lengths from what both see: for the planes of the shapes)
    int failed = 0;   // (shapes the Outline method could not read)
    for (Seg &s : P.segs) {
        if (!s.resolving || s.traced || s.kcam.empty()) continue;
        if (knots_method()) {
            // Method "Knots curve": the knots (the Knots slider, corners on the bends) are read off the stripe map, and the curve
            // through them is what is traced. The ones the map could not read well are tried with a wider, more tolerant fit,
            // and those still missing take their place between their neighbours.
            make_knots(s.dense, knot_count(), s.kcam, s.kcorner);   // (the knots: an earlier Outline run may have replaced them)
            const int n = (int) s.kcam.size() / 2;
            s.kbeam.assign((size_t) n * 2, 0.0f);
            s.kdone.assign(n, 0);
            s.kretried.assign(n, 1);
            g_scanquiet = true;
            scan_knots(s, W, H, T.sd[0]);
            reject_scan_outliers(s);
            scan_knots(s, W, H, T.sd[0], true);
            reject_scan_outliers(s);
            g_scanquiet = false;
            int have = 0;
            for (int k = 0; k < n && k < (int) s.kscanok.size(); k++) have += s.kscanok[k] ? 1 : 0;
            if (have < 3 || have < n / 3) {
                s.resolving = false;
                failed++;
                fprintf(stderr, "[ProjMap] scan: segment %d: only %d of %d knots could be read off the stripe map, the shape is not resolved\n", s.id, have, n);
                continue;
            }
            for (int k = 0; k < n; k++) {
                if (s.kscanok[k]) continue;
                int a = -1, b = -1, da = 0, db = 0;
                for (int j = 1; j < n && a < 0; j++) if (s.kscanok[(k - j + n) % n]) { a = (k - j + n) % n; da = j; }
                for (int j = 1; j < n && b < 0; j++) if (s.kscanok[(k + j) % n]) { b = (k + j) % n; db = j; }
                float t = (float) da / (float) (da + db);
                s.kscan[k * 2] = s.kscan[a * 2] + t * (s.kscan[b * 2] - s.kscan[a * 2]);
                s.kscan[k * 2 + 1] = s.kscan[a * 2 + 1] + t * (s.kscan[b * 2 + 1] - s.kscan[a * 2 + 1]);
                s.kscanok[k] = 1;
            }
            s.kbeam = s.kscan;
            s.kdone.assign(n, 1);
            continue;
        }
        {
            const int D = (int) s.dense.size() / 2;
            std::vector<float> beam1, beam2;
            bool ok1 = outline_from_scan(s, W, H, T.sd[0]);
            if (ok1) beam1 = s.kbeam;
            if (two) {
                int j = match_seg2(s, W, H, T.sd[0], W2, H2, T.sd[1]);
                if (j >= 0) {
                    Seg &s2 = P.segs2[j];
                    if (outline_from_scan(s2, W2, H2, T.sd[1])) beam2 = s2.kbeam;
                    fprintf(stderr, "[ProjMap] scan: segment %d is webcam 2 segment %d: its outline %s\n", s.id, j, beam2.empty() ? "could not be read" : "was read");
                }
                else fprintf(stderr, "[ProjMap] scan: segment %d: no segment of webcam 2 lies on the same place on the beamer\n", s.id);
            }
            std::vector<float> fused;
            if (!beam1.empty() && !beam2.empty() && fuse_outlines(beam1, beam2, D, fused)) {
                s.kbeam = fused;
                s.kdone.assign(D, 1);
                s.kretried.assign(D, 1);
                s.kcorner.assign(D, 0);
                fprintf(stderr, "[ProjMap] scan: segment %d: outlines of both webcams merged\n", s.id);
                continue;
            }
            if (ok1) continue;
            if (!beam2.empty() && (int) beam2.size() == D * 2) {   // (only webcam 2 could read it)
                s.kbeam = beam2;
                s.kdone.assign(D, 1);
                s.kretried.assign(D, 1);
                s.kcorner.assign(D, 0);
                s.kcam = s.dense;
                fprintf(stderr, "[ProjMap] scan: segment %d: the outline comes from webcam 2 alone\n", s.id);
                continue;
            }
            // (the shape is left as it was and the user is told)
            s.resolving = false;
            failed++;
            fprintf(stderr, "[ProjMap] scan: segment %d: the stripe map covers too little of the outline, the shape is not resolved\n", s.id);
            continue;
        }
    }
    int si, ki;
    next_pending(si, ki);   // (marks the shapes whose outline points are all there as traced)
    after_all_knots();      // (on to the outline check, then the end)
    if (failed > 0) set_step(5, 3, "Outline: the stripe map covers too little of " + std::to_string(failed) + " shape(s) (see the log): not resolved");
    fprintf(stderr, "[ProjMap] scan: everything after the last stripe took %.1f s\n", std::chrono::duration<float>(std::chrono::steady_clock::now() - tstart).count());
}

void trace_tick() {
    Tracer &T = P.tr;
    if (P.snapWait) {   // (hold the white until the snapshot is taken)
        T.st = Tracer::IDLE;
        P.bmode = BM_WHITE;
        return;
    }
    bool ready = P.bwin && P.cap.state == 2 && !P.segs.empty();
    if (!ready) {
        T.st = Tracer::IDLE;
        P.bmode = idle_mode();
        return;
    }
    if (T.st == Tracer::IDLE) {
        int si, ki;
        bool pending = next_pending(si, ki) || needs_verify();
        if (!pending || T.blocked) {
            BeamMode want = idle_mode();
            if (P.bmode != want) { P.bmode = want; P.maskdirty = true; }
            return;
        }
        T.scanned = false;    // (a scan for every run: the scene may have changed)
        P.bmode = BM_BLACK;   // (the webcam needs the unlit scene as the reference)
        T.tset = std::chrono::steady_clock::now() + std::chrono::milliseconds(1000);   // (the webcam needs time to adapt to the dark)
        T.cset = P.cap.counter.load();
        T.st = Tracer::BG_WAIT;
        set_step(5, 1, "Reference frame...");
        return;
    }
    if (!settled()) return;

    std::vector<uint8_t> frame;
    int W = 0, H = 0;
    if (!P.cap.grab(frame, W, H)) return;

    if (T.st == Tracer::BG_WAIT) {
        T.bg = frame;
        T.haveBg = true;
        begin_ref();
    }
    else if (T.st == Tracer::REF_WAIT) {
        T.ref = frame;
        T.haveRef = true;
        // (did the webcam image move since the snapshot that was segmented? the reference frame is lit like it)
        T.camMok = estimate_cam_motion(T.ref, W, H, T.camM, "snapshot to the start of the scan");
        begin_scan();
    }
    else if (T.st == Tracer::SCAN_WAIT) {
        std::vector<uint8_t> frame2;
        int W2 = 0, H2 = 0;
        if (T.scan2 && !P.cap2.grab(frame2, W2, H2)) return;   // (webcam 2 has no frame yet)
        bool bad0 = false, bad1 = false;
        bool okf = scan_frame(T.sd[0], frame, W, H, T.scanIdx, &bad0);
        if (okf && T.scan2) okf = scan_frame(T.sd[1], frame2, W2, H2, T.scanIdx, &bad1);
        if (!okf) {
            fprintf(stderr, "[ProjMap] scan: a webcam image changed size, scan abandoned\n");
            T.scanned = true;
            P.bmode = BM_BLACK;
            for (Seg &s : P.segs) if (s.resolving && !s.traced) s.resolving = false;   // (nothing was read: not resolved)
            finish_trace();
            set_step(5, 3, "The webcam image changed size during the scan: not resolved");
            return;
        }
        if (T.scanIdx % 4 == 3) {   // (the four frames of a stripe period are complete)
            if ((bad0 || bad1) && T.scanRetry < 2) {   // (they do not fit together: take them again)
                T.scanRetry++;
                fprintf(stderr, "[ProjMap] scan: the four frames of pattern %d-%d do not fit together (a frame was stale?), taking them again (%d)\n",
                        T.scanIdx - 2, T.scanIdx + 1, T.scanRetry);
                T.scanIdx -= 3;
                show_scan_pattern(T.scanIdx);
                return;
            }
            T.scanRetry = 0;
        }
        T.scanIdx++;
        if (T.scanIdx < scan_patterns()) show_scan_pattern(T.scanIdx);
        else {
            // the decoding takes a while on the main thread, which also draws the beamer: the last stripes would stay on it
            // all that time, so the beamer goes dark first
            P.bmode = BM_BLACK;
            render_beam();
            set_step(5, 1, "Decoding the stripes...");
            finish_scan(W, H, T.scan2 ? W2 : 0, T.scan2 ? H2 : 0);
        }
    }
    else if (T.st == Tracer::VERIFY_WAIT) {
        if (T.vseg < 0 || T.vseg >= (int) P.segs.size()) { T.st = Tracer::IDLE; return; }
        Seg &s = P.segs[T.vseg];
        std::vector<float> u;
        float rms = 0, sec[4] = {0, 0, 0, 0};
        bool ok = verify_local(s, frame, T.bg, nullptr, W, H, u, &rms, sec, 40);
        fprintf(stderr, "[ProjMap] segment %d outline check %d: %s lit outline off the target by rms %.2f px; outside the target (px, negative = inside): left %+.1f, right %+.1f, top %+.1f, bottom %+.1f\n",
                T.vseg, s.vtries + 1, ok ? "" : "(not measurable)", rms, sec[0], sec[1], sec[2], sec[3]);
        if (ok) {
            // Only a measurement, nothing is corrected: how far the lit outline lies from where it should (webcam pixels, the
            // size of the offset of every outline point), and where the worst points are (position in the webcam image 0..1)
            std::vector<std::pair<float, int>> mag;
            for (size_t k = 0; k + 1 < u.size(); k += 2) mag.push_back({std::hypot(u[k], u[k + 1]), (int) (k / 2)});
            std::sort(mag.begin(), mag.end(), [](const std::pair<float, int> &a, const std::pair<float, int> &b) { return a.first > b.first; });
            if (!mag.empty()) {
                float mean = 0.0f;
                for (auto &m : mag) mean += m.first;
                mean /= (float) mag.size();
                std::string worst;
                for (size_t q = 0; q < mag.size() && q < 8; q++) {
                    const int k = mag[q].second;
                    char buf[96];
                    snprintf(buf, sizeof(buf), " #%d at (%.2f,%.2f) off (%+.0f,%+.0f)", k, s.kcam[k * 2], s.kcam[k * 2 + 1], u[k * 2], u[k * 2 + 1]);
                    worst += buf;
                }
                fprintf(stderr, "[ProjMap] segment %d outline check (webcam px, webcam image %dx%d): mean offset %.1f, median %.1f, 90%% under %.1f, largest %.1f; worst points (position, offset x,y):%s\n",
                        T.vseg, W, H, mean, mag[mag.size() / 2].first, mag[mag.size() / 10].first, mag[0].first, worst.c_str());
            }
        }
        s.verified = true;   // (the check only measures and logs: nothing is corrected from it)
        finalize_seg(s);
        after_all_knots();
    }
}

// ======================================================================================================
// Controls
// ======================================================================================================

std::string cam_name(int idx) {
    if (idx <= 0 || idx > (int) mainprogram->livedevices.size()) return "";
    const std::wstring &w = mainprogram->livedevices[idx - 1];
    std::string s(w.begin(), w.end());
#ifdef WINDOWS
    return "video=" + s;
#elif defined(LINUX) || defined(MACOS)
    return mainprogram->devvideomap[s];
#else
    return "";
#endif
}

void refresh_device_lists() {
    get_cameras();
    P.cam->options.clear();
    P.cam->options.push_back("No webcam");
    for (auto &w : mainprogram->livedevices) P.cam->options.push_back(std::string(w.begin(), w.end()));
    P.cam2->options.clear();
    P.cam2->options.push_back("No webcam 2");
    for (auto &w : mainprogram->livedevices) P.cam2->options.push_back(std::string(w.begin(), w.end()));
    if (P.cam2->value >= (float) P.cam2->options.size()) P.cam2->value = 0.0f;
    P.beam->options.clear();
    P.beam->options.push_back("No beamer");
    P.dispidx.clear();
    int numd = 0;
    SDL_DisplayID *ids = SDL_GetDisplays(&numd);
    if (ids) SDL_free(ids);
    int currdisp = EWOC_DisplayIDToIndex(SDL_GetDisplayForWindow(mainprogram->mainwindow));
    for (int i = 0; i < numd; i++) {
        if (i == currdisp) continue;
        const char *nm = SDL_GetDisplayName(EWOC_DisplayIndexToID(i));
        P.beam->options.push_back(nm ? nm : ("Display " + std::to_string(i)));
        P.dispidx.push_back(i);
    }
    if (P.cam->value >= (float) P.cam->options.size()) P.cam->value = 0.0f;
    if (P.beam->value >= (float) P.beam->options.size()) P.beam->value = 0.0f;
}

void init_ui() {
    if (P.inited) return;
    P.inited = true;
    auto mk = [](const char *name, float deflt, float lo, float hi) {
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
    };
    P.cam = mk("Webcam", 0.0f, 0.0f, 1.0f);
    P.cam->type = FF_TYPE_OPTION;
    P.cam->options = {"No webcam"};
    P.cam->box->tooltiptitle = "Webcam ";
    P.cam->box->tooltip = "The webcam that watches the surfaces the beamer projects on. ";
    P.cam2 = mk("Webcam 2", 0.0f, 0.0f, 1.0f);
    P.cam2->type = FF_TYPE_OPTION;
    P.cam2->options = {"No webcam 2"};
    P.cam2->box->tooltiptitle = "Webcam 2 ";
    P.cam2->box->tooltip = "An optional second webcam, best placed on the other side of the beamer from the first. Surfaces that are tilted away from one webcam are seen at a better angle by the other, so the stripe scan reads them more exactly. Used by the Outline method: both webcams segment and scan, the shape on the beamer is what either of them sees of it. ";
    P.beam = mk("Beamer", 0.0f, 0.0f, 1.0f);
    P.beam->type = FF_TYPE_OPTION;
    P.beam->options = {"No beamer"};
    P.beam->box->tooltiptitle = "Beamer ";
    P.beam->box->tooltip = "The external display (beamer) used for the projection. ";
    P.knots = mk("Knots", 16.0f, (float) kMinKnots, (float) kMaxKnots);
    P.knots->box->tooltiptitle = "Knots ";
    P.knots->box->tooltip = "Number of evenly distributed knots of the smooth outline curve through each selected segment (shown on the webcam image). More knots follow the shape better. ";
    P.lastknots = 16;
    P.corner = mk("Corner threshold", 35.0f, 10.0f, 120.0f);
    P.corner->box->tooltiptitle = "Corner threshold ";
    P.corner->box->tooltip = "Turning angle in degrees above which a bend of a segment outline counts as a corner: it gets a knot of its own and stays sharp. Lower = more corners, higher = fewer (smoother) corners. ";
    P.lastcorner = 35;
    P.method = mk("Method", 0.0f, 0.0f, 1.0f);
    P.method->type = FF_TYPE_OPTION;
    P.method->options = {"Outline", "Knots curve"};
    P.method->box->tooltiptitle = "Method ";
    P.method->box->tooltip = "How a shape is traced on the beamer, and shown on the webcam image. Outline: every point of the SAM3 mask outline is read off the stripe map (both webcams, when webcam 2 is on), and the outline is shown in lightblue. Knots curve: only the knots on the smooth curve through the shape (the Knots and Corner threshold sliders) are read off the stripe map (webcam 1), and the curve through them is traced and shown, with its knots. ";
    P.fov = mk("FOV", 0.0f, 0.0f, 150.0f);
    P.fov->box->tooltiptitle = "Webcam FOV ";
    P.fov->box->tooltip = "The horizontal field of view of webcam 1 in degrees (it changes with an optical zoom). It is where the planes of the shapes start from. 0 = automatic: what the AI depth estimate finds for each snapshot, else 70 degrees. Either way the scan refines the focal lengths of both webcams from what they both see (when the scene is more than one plane) and measures the lens distortion of each; the log says what it found. ";
    P.fov2 = mk("FOV 2", 0.0f, 0.0f, 150.0f);
    P.fov2->box->tooltiptitle = "Webcam 2 FOV ";
    P.fov2->box->tooltip = "The horizontal field of view of webcam 2 in degrees, from its specifications (0 = automatic: what the AI depth estimate finds for its snapshot). ";
    P.preview = mk("Preview", 0.0f, 0.0f, 1.0f);
    P.preview->type = FF_TYPE_OPTION;
    P.preview->options = {"Outline", "Map"};
    P.preview->box->tooltiptitle = "Preview ";
    P.preview->box->tooltip = "Outline: only the outline of the mask, as a thin white line, to check how well it follows the object. Map: the beamer shows an even light, and over the webcam image the last stripe scan is painted: yellow = saturated in several stripe patterns and without a position (glare), blue = stripes too weak, red = the stripe periods disagree, magenta = ambiguous, orange = dropped as it disagrees with its neighbours, faint green = has a position. ";
    g_cornerang = 35.0f * 3.14159265f / 180.0f;
    P.light = mk("Exposure", g_prefexposure, 0.05f, 1.0f);
    P.light->deflt = 0.7f;   // (what resetting the slider gives, whatever was saved in the preferences)
    P.light->deflt = 0.25f;
    P.light->sliding = true;
    P.light->box->tooltiptitle = "Exposure ";
    P.light->box->tooltip = "How bright the beamer lights the scene (a grey) while the snapshot is taken. Lower it when the snapshot is overexposed, raise it when it is too dark for the segmentation. ";

    auto mkbox = [](const char *title, const char *tip, const float *ac) {
        Boxx *b = new Boxx;
        b->lcolor[0] = 0.7f; b->lcolor[1] = 0.7f; b->lcolor[2] = 0.7f;
        b->acolor[0] = ac[0]; b->acolor[1] = ac[1]; b->acolor[2] = ac[2]; b->acolor[3] = 1.0f;
        b->vtxcoords->w = 0.25f;
        b->vtxcoords->h = 0.07f;
        b->tooltiptitle = title;
        b->tooltip = tip;
        b->upvtxtoscr();
        return b;
    };
    const float gc[3] = {0.0f, 0.4f, 0.0f}, rc[3] = {0.5f, 0.1f, 0.1f};
    P.segbox = mkbox("Segment ", "Takes a snapshot of the webcam image and segments it with SAM3 (all of it, or only the part in the selection box you can draw on the image; with the prompt when PROMPT is on). Then click segments to select them for projection. The segments stay when you segment again (e.g. another part of the image): the new ones are added, each outline keeps its own colour, and one RESOLVE traces everything selected. CLEAN wipes them all. ", gc);
    P.promptbox = mkbox("Prompt ", "Toggles segmenting with a text prompt: type it and press ENTER. When off, everything in the image (or selection box) is segmented. ", gc);
    P.exposebox = mkbox("Expose ", "Toggles the grey light on the beamer (at the brightness of the Exposure slider) whenever the beamer is not busy resolving: set the exposure while you look at the webcam image, and see dim objects when you draw the selection box. ", gc);
    P.cleanbox = mkbox("Clean ", "Wipes all segmentations (and the depth estimate) and starts over clean. Segmenting again adds to the segments that are there, so that several cropped segmentations can be followed by one RESOLVE of everything selected; CLEAN removes them all. Masks that were resolved already stay in the Mapping view. ", rc);
    P.resolvebox = mkbox("Resolve ", "Traces the selected shapes (click them first) on the beamer and stores them as masks of that beamer. No layer is projected on them yet: choose \"Mapping\" in the menu of a layer or mix monitor to assign it to the masks. ", gc);
    P.cancelbox = mkbox("Cancel projection mapping ", "Stops everything that is running, throws the segmentation and the projection away, closes the webcam and the beamer window and leaves mapping mode. ", rc);
    P.vbox = mkbox("View webcam 2 ", "Switches the big window to the image of webcam 2, with nothing else on it (no segments, no selection box): to point webcam 2 at the scene. Click again to go back to webcam 1 and everything. Only there when webcam 2 is chosen. ", gc);
    P.vbox->vtxcoords->w = 0.045f;
    P.vbox->vtxcoords->h = 0.045f * (float) glob->w / (float) glob->h;   // (a square on the screen)
    P.vbox->upvtxtoscr();
    P.textbox = new Boxx;
    P.textbox->vtxcoords->w = 0.6f;
    P.textbox->vtxcoords->h = 0.3f;
    P.textbox->upvtxtoscr();
}

void compute_geometry() {
    // (the same box as the layer warp canvas)
    float top = mainprogram->scrollboxes[0]->vtxcoords->y1 - 0.012f;
    float bottom = mainmix->crossfade->box->vtxcoords->y1 + mainmix->crossfade->box->vtxcoords->h + 0.012f;
    float h = top - bottom;
    float aspect = 16.0f / 9.0f * (float) glob->h / (float) glob->w;
    float w = h * aspect;
    if (w > 1.9f) {
        w = 1.9f;
        h = w / aspect;
    }
    P.bw = w;
    P.bh = h;
    P.bx = -w * 0.5f;
    P.by = bottom + (top - bottom - h) * 0.5f;
}

void invalidate_trace() {
    P.tr.st = Tracer::IDLE;
    P.tr.blocked = false;
    P.tr.haveBg = false;
    P.maskdirty = true;
}

void reset_results() {
    P.snapWait = false;
    P.segs.clear();
    P.pendingSegs.clear();
    P.segs2.clear();
    P.pendingSegs2.clear();
    P.vdiag.clear();
    P.segsReady = false;
    P.segs2Ready = false;
    P.depth.clear();
    P.depthReady = false;
    P.snap.clear();
    P.tr = Tracer();
    P.bmode = BM_BLACK;
    P.ovdirty = P.maskdirty = true;
}

// no resolved shapes any more (a different beamer, or CANCEL): the projection of the layers stops
void clear_mapped() {
    P.mapped.clear();
    for (Seg &s : P.segs) { s.resolved = false; s.resolving = false; s.mapidx = -1; s.lay = nullptr; s.traced = false; }
    P.maskdirty = true;
    P.ovdirty = true;
}

void apply_param_changes() {
    int cam = (int) (P.cam->value + 0.5f), beam = (int) (P.beam->value + 0.5f), kn = knot_count();
    if (cam != P.lastcam) {
        P.lastcam = cam;
        P.cap.stop();
        reset_results();
        P.tr = Tracer();
        if (cam > 0) {
            std::string nm = cam_name(cam);
            if (nm.empty()) set_status("Webcam name unknown");
            else {
                P.cap.start(nm);
                set_status("Opening webcam...");
            }
        }
    }
    int cam2 = (int) (P.cam2->value + 0.5f);
    if (cam2 != P.lastcam2) {
        P.lastcam2 = cam2;
        P.cap2.stop();
        P.segs2.clear();
        P.tr.scanned = false;
        if (cam2 > 0 && cam2 == cam) {   // (the same webcam cannot be opened twice)
            P.cam2->value = 0.0f;
            P.lastcam2 = 0;
            set_status("Webcam 2 has to be a different webcam than webcam 1");
        }
        else if (cam2 > 0) {
            std::string nm = cam_name(cam2);
            if (nm.empty()) set_status("Webcam 2 name unknown");
            else {
                P.cap2.start(nm);
                set_status("Opening webcam 2...");
            }
        }
    }
    if (beam != P.lastbeam) {
        P.lastbeam = beam;
        close_beamer();
        clear_mapped();   // (shapes mapped on another beamer mean nothing on this one)
        P.tr = Tracer();
        if (beam > 0 && beam - 1 < (int) P.dispidx.size()) open_beamer(P.dispidx[beam - 1]);
    }
    int pv = (int) (P.preview->value + 0.5f);
    if (pv != P.lastpreview) {
        P.lastpreview = pv;
        P.maskdirty = true;
    }
    int cd = (int) (P.corner->value + 0.5f);
    if (kn != P.lastknots || cd != P.lastcorner) {
        P.lastcorner = cd;
        g_cornerang = (float) cd * 3.14159265f / 180.0f;
        P.lastknots = kn;
        for (Seg &s : P.segs) {
            make_knots(s.dense, kn, s.kcam, s.kcorner);
            s.kbeam.assign(s.kcam.size(), 0.0f);
            s.kdone.assign(s.kcam.size() / 2, 0);
            s.kretried.clear();
            s.kscan.clear();
            s.kscanok.clear();
            s.traced = false;
            reset_verify(s);
        }
        P.tr.st = Tracer::IDLE;
        P.tr.blocked = false;
        P.maskdirty = true;
    }
}

// ---- overlay of the webcam image ----
void bake_overlay(int W, int H) {
    std::vector<uint8_t> ov((size_t) W * H * 4, 0);
    if (P.selvalid) {
        int x0 = std::clamp((int) (P.sel[0] * W), 0, W), x1 = std::clamp((int) (P.sel[2] * W), 0, W);
        int y0 = std::clamp((int) (P.sel[1] * H), 0, H), y1 = std::clamp((int) (P.sel[3] * H), 0, H);
        for (int y = 0; y < H; y++) {
            for (int x = 0; x < W; x++) {
                if (x < x0 || x >= x1 || y < y0 || y >= y1) ov[((size_t) y * W + x) * 4 + 3] = 128;   // (black, 50%)
            }
        }
    }
    if (P.sw == W && P.sh == H) {
        for (const Seg &s : P.segs) {
            // selected (to be resolved): darkred2 at 50%; resolved (a layer is projected on it): a green tint; being resolved: orange
            if (!s.selected && !s.resolved && !s.resolving) continue;
            const float *tint = s.selected ? darkred2 : (s.resolving ? orange : darkgreen1);
            const uint8_t talpha = s.selected ? 128 : (s.resolving ? 90 : 100);
            for (int y = s.y0; y <= s.y1; y++) {
                for (int x = s.x0; x <= s.x1; x++) {
                    size_t i = (size_t) y * W + x;
                    if (s.mask[i] < 128) continue;
                    ov[i * 4] = (uint8_t) (tint[0] * 255.0f);
                    ov[i * 4 + 1] = (uint8_t) (tint[1] * 255.0f);
                    ov[i * 4 + 2] = (uint8_t) (tint[2] * 255.0f);
                    ov[i * 4 + 3] = talpha;
                }
            }
        }
        for (const Seg &s : P.segs) {
            // (white while selected for RESOLVE, else the colour of the segment)
            const float ecol[3] = {s.selected ? 1.0f : s.col[0], s.selected ? 1.0f : s.col[1], s.selected ? 1.0f : s.col[2]};
            for (uint32_t i : s.edge) {
                int x = (int) (i % W), y = (int) (i / W);
                for (int dy = 0; dy <= 1; dy++) {
                    for (int dx = 0; dx <= 1; dx++) {
                        if (x + dx >= W || y + dy >= H) continue;
                        size_t o = ((size_t) (y + dy) * W + x + dx) * 4;
                        ov[o] = (uint8_t) (ecol[0] * 255.0f);
                        ov[o + 1] = (uint8_t) (ecol[1] * 255.0f);
                        ov[o + 2] = (uint8_t) (ecol[2] * 255.0f);
                        ov[o + 3] = 255;
                    }
                }
            }
        }
    }
    if (P.ovtex == (GLuint) -1) {
        glGenTextures(1, &P.ovtex);
        glBindTexture(GL_TEXTURE_2D, P.ovtex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    glBindTexture(GL_TEXTURE_2D, P.ovtex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA_INTERNAL, W, H, 0, GL_RGBA, GL_UNSIGNED_BYTE, ov.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    mainprogram->texsizemap[P.ovtex] = {W, H};
    P.ovw = W;
    P.ovh = H;
    P.ovdirty = false;
}

bool upload_cam() {
    std::vector<uint8_t> f;
    int W, H;
    uint64_t cnt = P.cap.counter.load();
    if (P.camtex != (GLuint) -1 && cnt == P.camtexcnt) return true;
    if (!P.cap.grab(f, W, H)) return P.camtex != (GLuint) -1;
    if (P.camtex == (GLuint) -1 || W != P.camtexw || H != P.camtexh) {
        if (P.camtex == (GLuint) -1) glGenTextures(1, &P.camtex);
        glBindTexture(GL_TEXTURE_2D, P.camtex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA_INTERNAL, W, H, 0, GL_RGBA, GL_UNSIGNED_BYTE, f.data());
        mainprogram->texsizemap[P.camtex] = {W, H};
        P.camtexw = W;
        P.camtexh = H;
        P.ovdirty = true;
    }
    else {
        glBindTexture(GL_TEXTURE_2D, P.camtex);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, W, H, GL_RGBA, GL_UNSIGNED_BYTE, f.data());
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    P.camtexcnt = cnt;
    return true;
}

// the same for webcam 2
bool upload_cam2() {
    std::vector<uint8_t> f;
    int W, H;
    uint64_t cnt = P.cap2.counter.load();
    if (P.camtex2 != (GLuint) -1 && cnt == P.camtexcnt2) return true;
    if (!P.cap2.grab(f, W, H)) return P.camtex2 != (GLuint) -1;
    if (P.camtex2 == (GLuint) -1 || W != P.camtexw2 || H != P.camtexh2) {
        if (P.camtex2 == (GLuint) -1) glGenTextures(1, &P.camtex2);
        glBindTexture(GL_TEXTURE_2D, P.camtex2);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA_INTERNAL, W, H, 0, GL_RGBA, GL_UNSIGNED_BYTE, f.data());
        mainprogram->texsizemap[P.camtex2] = {W, H};
        P.camtexw2 = W;
        P.camtexh2 = H;
    }
    else {
        glBindTexture(GL_TEXTURE_2D, P.camtex2);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, W, H, GL_RGBA, GL_UNSIGNED_BYTE, f.data());
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    P.camtexcnt2 = cnt;
    return true;
}

float g_clip[4] = {-2.0f, -2.0f, 2.0f, 2.0f};   // the rectangle (NDC) lines of the webcam image are clipped to (it is zoomed)

void nline(float *col, float x1, float y1, float x2, float y2) {
    // Liang-Barsky clip against g_clip
    float dx = x2 - x1, dy = y2 - y1;
    float p[4] = {-dx, dx, -dy, dy};
    float q[4] = {x1 - g_clip[0], g_clip[2] - x1, y1 - g_clip[1], g_clip[3] - y1};
    float t0 = 0.0f, t1 = 1.0f;
    for (int i = 0; i < 4; i++) {
        if (p[i] == 0.0f) {
            if (q[i] < 0.0f) return;
        } else {
            float t = q[i] / p[i];
            if (p[i] < 0.0f) { if (t > t1) return; if (t > t0) t0 = t; }
            else { if (t < t0) return; if (t < t1) t1 = t; }
        }
    }
    register_line_draw(col, x1 + t0 * dx, y1 + t0 * dy, x1 + t1 * dx, y1 + t1 * dy, true);
}

void end_prompt_edit() {
    if (mainprogram->renaming == EDIT_SEGPROMPT) {
        mainprogram->renaming = EDIT_NONE;
        SDL_StopTextInput(mainprogram->mainwindow);
    }
    P.promptEditing = false;
    if (P.promptstr.empty()) P.promptOn = false;
}

}  // namespace

// ======================================================================================================
// Public
// ======================================================================================================

unsigned int projmap_beam_tex() {
    return P.beamshow;
}

float projmap_exposure() {
    return P.inited && P.light ? P.light->value : g_prefexposure;
}

void projmap_set_exposure(float v) {
    g_prefexposure = std::clamp(v, 0.05f, 1.0f);
    if (P.inited && P.light) P.light->value = g_prefexposure;
}

bool projmap_isparam(Param *p) {
    if (!P.inited || !p) return false;
    return p == P.cam || p == P.beam || p == P.knots || p == P.light || p == P.corner || p == P.preview || p == P.cam2 || p == P.method || p == P.fov || p == P.fov2;
}

// RESOLVE: the selected shapes get the current layer. Shapes that are resolved already only change their layer (they
// replace the old one); the others are traced on the beamer first (see trace_tick), then they are resolved.
bool resolve_selected(Layer *lay) {
    int needtrace = 0;
    for (const Seg &s : P.segs) {
        if (s.selected && !(s.resolved && s.traced && s.mapidx >= 0 && s.mapidx < (int) P.mapped.size())) needtrace++;
    }
    if (needtrace && (!P.bwin || P.cap.state != 2)) {
        set_status("Choose a webcam and a beamer first: the shapes are traced on the beamer");
        return false;
    }
    int traced = 0, reassigned = 0;
    for (Seg &s : P.segs) {
        if (!s.selected) continue;
        s.lay = lay;
        s.selected = false;
        if (s.resolved && s.traced && s.mapidx >= 0 && s.mapidx < (int) P.mapped.size()) {
            reassigned++;   // (traced already: nothing to do)
        }
        else {
            s.resolving = true;
            reset_verify(s);
            traced++;
        }
    }
    P.tr.blocked = false;
    P.maskdirty = true;
    P.ovdirty = true;
    set_status("Resolving: " + std::to_string(traced) + " shape(s) to trace, " + std::to_string(reassigned) + " traced already");
    return true;
}

void projmap_enter() {
    init_ui();
    if (P.cap.state == 0 && (int) (P.cam->value + 0.5f) == 0) refresh_device_lists();
    if (P.bwin) P.bwin->mixid = 5;   // (the Mapping view may have used the beamer window meanwhile)
    P.prevdown = false;
    P.dragging = false;
    P.zoom = 1.0f;
    P.zu = P.zv = 0.5f;
}

void projmap_leave() {
    mainprogram->projmap = false;
    mainprogram->projmapbut->value = 0;
    mainprogram->projmapbut->oldvalue = 0;
    if (P.inited && P.promptEditing) end_prompt_edit();
    if (P.inited) P.expose = false;   // (the grey is for setting up: the projection comes back)
    P.dragging = false;
}

void projmap_cancel() {
    if (P.inited) {
        if (P.promptEditing) end_prompt_edit();
        P.cancel = true;
        P.gen++;
        if (P.sam && P.sam->isProcessing()) P.sam->cancelSegmentation();
        P.cap.stop();
        P.cap2.stop();
        close_beamer();
        clear_mapped();
        reset_results();
        P.selvalid = false;
        P.promptOn = false;
        P.cam->value = 0.0f;
        P.cam2->value = 0.0f;
        P.beam->value = 0.0f;
        P.lastcam = P.lastbeam = P.lastcam2 = 0;
        {
            std::lock_guard<std::mutex> lock(P.mtx);
            for (Step &s : P.steps) s.state = 0;
            P.status = "Select a webcam and a beamer, then click SEGMENT";
        }
    }
    projmap_leave();
}

void projmap_tick() {
    if (!P.inited) return;
    // results of the worker
    if (P.segsReady.exchange(false)) {
        std::lock_guard<std::mutex> lock(P.mtx);
        if (!P.cancel.load()) {
            // The new segments are added to those of the earlier segmentations (each with an id of its own, and so its own colour);
            // none is replaced, also not one that covers the same pixels as an old one
            for (Seg &ns : P.pendingSegs) {
                ns.id = P.nextsegid++;
                hsv2rgb(std::fmod(0.137f + 0.381966f * (float) ns.id, 1.0f), 0.85f, 1.0f, ns.col);
                P.segs.push_back(std::move(ns));
            }
            P.pendingSegs.clear();
            P.tr.st = Tracer::IDLE;
            P.tr.blocked = false;
            P.ovdirty = P.maskdirty = true;
        }
    }
    if (P.segs2Ready.exchange(false)) {
        std::lock_guard<std::mutex> lock(P.mtx);
        if (!P.cancel.load()) {
            P.segs2 = std::move(P.pendingSegs2);
            P.pendingSegs2.clear();
        }
    }
    bool depthnow = false;
    if (P.depthReady.exchange(false)) {
        std::lock_guard<std::mutex> lock(P.mtx);
        if (!P.cancel.load()) {
            P.depth = std::move(P.pendingDepth);
            P.pendingDepth.clear();
            P.dw = P.pdw; P.dh = P.pdh; P.dfx = P.pdfx;
            depthnow = true;
        }
    }
    if (depthnow) {
        P.scene = build_scene_points();   // (for the Viewpoint of the outlines: handed to the Mapping view when a shape is resolved)
        P.sceneSent = nullptr;
    }
    prune_mapped();   // (a mapped layer that was closed is gone from the beamer)
    trace_tick();
    render_beam();
}

void projmap_ui() {
    if (!mainprogram->projmap) return;
    init_ui();
    compute_geometry();
    apply_param_changes();

    if (P.snapWait) {
        float el = std::chrono::duration<float>(std::chrono::steady_clock::now() - P.snapT).count();
        if (el > 10.0f || (el >= 3.0f && P.cap.counter.load() >= P.snapC + 20)) {
            P.snapWait = false;
            start_segmentation(true);
        }
    }

    // geometry of the control panel (like the layer warp one)
    float panelw = 0.25f, rowh = 0.071f, boxh = 0.064f;   // (nine rows; the boxes are 60% wider than the 0.156 of the layer warp ones)
    float cx = P.bx + P.bw * 0.5f, cy = P.by + P.bh * 0.5f;
    float px0 = std::max(-0.99f, P.bx - panelw - 0.01f);   // (never off the left of the screen)
    float ptop = cy + rowh * 5.5f;
    Param *plist[10] = {P.cam, P.cam2, P.beam, P.method, P.knots, P.light, P.corner, P.preview, P.fov, P.fov2};
    const int prow[10] = {1, 2, 3, 4, 5, 8, 5, 9, 10, 10};
    const bool cam2set = (int) (P.cam2->value + 0.5f) > 0;
    for (int n = 0; n < 10; n++) {
        Boxx *b = plist[n]->box;
        b->vtxcoords->x1 = px0;
        b->vtxcoords->y1 = ptop - rowh * prow[n] + 0.004f;
        b->vtxcoords->w = panelw;
        b->vtxcoords->h = boxh;
        if (plist[n] == P.corner || plist[n] == P.knots) {   // (these share a row)
            b->vtxcoords->w = panelw * 0.5f - 0.003f;
            if (plist[n] == P.corner) b->vtxcoords->x1 = px0 + panelw * 0.5f + 0.003f;
        }
        if (plist[n] == P.fov && cam2set) b->vtxcoords->w = panelw * 0.5f - 0.003f;   // (the FOV of each webcam share a row)
        if (plist[n] == P.fov2) {
            b->vtxcoords->x1 = px0 + panelw * 0.5f + 0.003f;
            b->vtxcoords->w = panelw * 0.5f - 0.003f;
        }
        if (plist[n] == P.cam2) {   // (the Webcam 2 option shares its row with the V button: a square on the screen)
            const float vw = boxh * (float) glob->h / (float) glob->w;
            b->vtxcoords->w = panelw - vw - 0.004f;
            P.vbox->vtxcoords->x1 = px0 + panelw - vw;
            P.vbox->vtxcoords->y1 = b->vtxcoords->y1;
            P.vbox->vtxcoords->w = vw;
            P.vbox->vtxcoords->h = boxh;
            P.vbox->upvtxtoscr();
        }
        if (plist[n] == P.light) {   // (Exposure shares its row with the EXPOSE button)
            b->vtxcoords->w = panelw * 0.62f;
            P.exposebox->vtxcoords->x1 = px0 + panelw * 0.62f + 0.004f;
            P.exposebox->vtxcoords->y1 = b->vtxcoords->y1;
            P.exposebox->vtxcoords->w = panelw * 0.38f - 0.004f;
            P.exposebox->vtxcoords->h = boxh;
            P.exposebox->upvtxtoscr();
        }
        b->upvtxtoscr();
    }
    // SEGMENT and PROMPT share a row (half the width each), RESOLVE has the row below, CANCEL the last one
    {
        const float half = panelw * 0.5f - 0.003f;
        P.segbox->vtxcoords->x1 = px0;
        P.segbox->vtxcoords->w = half;
        P.promptbox->vtxcoords->x1 = px0 + panelw * 0.5f + 0.003f;
        P.promptbox->vtxcoords->w = half;
        Boxx *row4[2] = {P.segbox, P.promptbox};
        for (Boxx *b : row4) {
            b->vtxcoords->y1 = ptop - rowh * 6 + 0.004f;
            b->vtxcoords->h = boxh;
            b->upvtxtoscr();
        }
        Boxx *full[3] = {P.resolvebox, P.cleanbox, P.cancelbox};
        int frows[3] = {7, 7, 11};
        for (int n = 0; n < 3; n++) {
            // (RESOLVE and CLEAN share a row)
            full[n]->vtxcoords->x1 = n == 1 ? px0 + panelw * 0.5f + 0.003f : px0;
            full[n]->vtxcoords->y1 = ptop - rowh * frows[n] + 0.004f;
            full[n]->vtxcoords->w = n < 2 ? half : panelw;
            full[n]->vtxcoords->h = boxh;
            full[n]->upvtxtoscr();
        }
    }
    float panel_x0 = px0 - 0.005f, panel_x1 = px0 + panelw + 0.005f, panel_y0 = ptop - rowh * 11 - 0.005f;

    // mouse
    float mnx = mainprogram->xscrtovtx((float) mainprogram->mx) - 1.0f;
    float mny = 1.0f - mainprogram->yscrtovtx((float) mainprogram->my);
    bool inpanel = mnx >= panel_x0 && mnx <= panel_x1 && mny >= panel_y0 && mny <= P.by + P.bh;
    bool incanvas = mnx >= P.bx && mnx <= P.bx + P.bw && mny >= P.by && mny <= P.by + P.bh;
    bool nomenu = !mainprogram->menuondisplay;

    // the webcam image, letterboxed in the canvas (above the status bar)
    float barh = 0.06f;
    float areay = P.by + barh + 0.02f, areah = P.bh - barh - 0.02f;
    // The V button (top right of the canvas) is there when webcam 2 is chosen; on, the window shows webcam 2 alone
    const bool hasv = (int) (P.cam2->value + 0.5f) > 0;
    if (!hasv) P.viewcam2 = false;
    const bool v2 = P.viewcam2;
    const bool overv = hasv && P.vbox->in();
    bool haveframe = v2 ? (P.cap2.state == 2 && upload_cam2() && P.camtexw2 > 0) : (P.cap.state == 2 && upload_cam() && P.camtexw > 0);
    const int imgw = v2 ? P.camtexw2 : P.camtexw, imgh = v2 ? P.camtexh2 : P.camtexh;
    float iw = P.bw, ih = areah, irx = P.bx, iry = areay;
    if (haveframe) {
        float camaspect = (float) imgw / (float) imgh;
        float boxaspect = (P.bw * (float) glob->w) / (areah * (float) glob->h);
        if (camaspect > boxaspect) {
            ih = P.bw * (float) glob->w / camaspect / (float) glob->h;
            iry = areay + (areah - ih) * 0.5f;
        } else {
            iw = areah * (float) glob->h * camaspect / (float) glob->w;
            irx = P.bx + (P.bw - iw) * 0.5f;
        }
    }
    // zoom: the mouse wheel zooms towards / away from the mouse position (the image point under the pointer stays put)
    if (haveframe && mainprogram->mousewheel != 0.0f && nomenu && !inpanel && !P.promptEditing) {
        float relx = (mnx - irx) / iw, rely = (iry + ih - mny) / ih;   // 0..1 across the image rectangle
        if (relx >= 0.0f && relx <= 1.0f && rely >= 0.0f && rely <= 1.0f) {
            float u = P.zu - 0.5f / P.zoom + relx / P.zoom, v = P.zv - 0.5f / P.zoom + rely / P.zoom;   // (under the pointer)
            float nz = std::clamp(P.zoom * (1.0f + 0.15f * mainprogram->mousewheel), 1.0f, 12.0f);
            P.zu = std::clamp(u - relx / nz + 0.5f / nz, 0.5f / nz, 1.0f - 0.5f / nz);
            P.zv = std::clamp(v - rely / nz + 0.5f / nz, 0.5f / nz, 1.0f - 0.5f / nz);
            P.zoom = nz;
            mainprogram->mousewheel = 0.0f;
        }
    }
    // pan: dragging with the middle mouse button moves the zoomed image along with the pointer (the button state is read
    // from SDL: mainprogram only has the click events)
    {
        const bool mdown = (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_MMASK) != 0;
        if (mdown && !P.mprev && haveframe && P.zoom > 1.0f && mnx >= irx && mnx <= irx + iw && mny >= iry && mny <= iry + ih && nomenu &&
            !inpanel && !P.promptEditing)
            P.panning = true;
        if (!mdown) P.panning = false;
        if (P.panning && haveframe) {
            float dnx = mnx - P.panmx, dny = mny - P.panmy;
            P.zu = std::clamp(P.zu - dnx / (iw * P.zoom), 0.5f / P.zoom, 1.0f - 0.5f / P.zoom);
            P.zv = std::clamp(P.zv + dny / (ih * P.zoom), 0.5f / P.zoom, 1.0f - 0.5f / P.zoom);
        }
        P.panmx = mnx;
        P.panmy = mny;
        P.mprev = mdown;
    }
    const float vz = P.zoom, vu0 = P.zu - 0.5f / vz, vv0 = P.zv - 0.5f / vz;   // (the part of the image that is shown)
    g_clip[0] = irx; g_clip[1] = iry; g_clip[2] = irx + iw; g_clip[3] = iry + ih;
    auto uv2n = [&](float u, float v, float &nx, float &ny) { nx = irx + (u - vu0) * vz * iw; ny = iry + ih - (v - vv0) * vz * ih; };
    float mu = vu0 + ((mnx - irx) / iw) / vz, mv = vv0 + ((iry + ih - mny) / ih) / vz;
    bool inimg = haveframe && mnx >= irx && mnx <= irx + iw && mny >= iry && mny <= iry + ih;

    // ---- input ----
    bool down = mainprogram->leftmousedown;
    bool pressed = down && !P.prevdown;
    bool released = !down && P.prevdown;
    bool busyprompt = P.promptEditing;
    if (v2) P.dragging = false;   // (webcam 2 is only looked at: no box select, no segments)
    if (pressed && inimg && !inpanel && nomenu && !busyprompt && !v2 && !overv) {
        P.dragging = true;
        P.dragu = mu; P.dragv = mv;
        P.dragpx = (float) mainprogram->mx; P.dragpy = (float) mainprogram->my;
    }
    if (P.dragging && released) {
        P.dragging = false;
        float ex = std::clamp(mu, 0.0f, 1.0f), ey = std::clamp(mv, 0.0f, 1.0f);
        bool moved = std::hypot((float) mainprogram->mx - P.dragpx, (float) mainprogram->my - P.dragpy) > 4.0f;
        if (moved) {
            float x0 = std::min(P.dragu, ex), x1 = std::max(P.dragu, ex), y0 = std::min(P.dragv, ey), y1 = std::max(P.dragv, ey);
            if (x1 - x0 > 0.03f && y1 - y0 > 0.03f) {
                P.selvalid = true;
                P.sel[0] = x0; P.sel[1] = y0; P.sel[2] = x1; P.sel[3] = y1;
                P.ovdirty = true;
            }
        }
        else {
            // a click: toggles the segment under it, elsewhere it clears the selection box
            // (masks can overlap: clicking again at the same spot goes on to the next mask under it, round and round)
            static int cycLast = -1, cycX = -100, cycY = -100;
            int hit = -1;
            if (P.sw > 0 && P.sh > 0) {
                int x = std::clamp((int) (P.dragu * P.sw), 0, P.sw - 1), y = std::clamp((int) (P.dragv * P.sh), 0, P.sh - 1);
                std::vector<int> under;
                for (int i = (int) P.segs.size() - 1; i >= 0; i--) {
                    if (P.segs[i].mask.size() >= (size_t) P.sw * P.sh && P.segs[i].mask[(size_t) y * P.sw + x] > 128) under.push_back(i);
                }
                if (!under.empty()) {
                    hit = under[0];
                    bool same = std::abs(x - cycX) <= 3 && std::abs(y - cycY) <= 3;
                    if (same && under.size() > 1) {
                        auto it = std::find(under.begin(), under.end(), cycLast);
                        if (it != under.end()) {
                            hit = under[(it - under.begin() + 1) % under.size()];
                            // (the one before it was selected by the previous click of this round: it gives way)
                            if (P.segs[cycLast].selected && !P.segs[cycLast].resolved && !P.segs[cycLast].resolving) P.segs[cycLast].selected = false;
                            if (P.segs[hit].selected) P.segs[hit].selected = false;   // (the toggle below selects it)
                        }
                    }
                    cycLast = hit;
                    cycX = x; cycY = y;
                }
                else cycLast = -1;
            }
            if (hit >= 0) {
                // (a segment traced before keeps its beamer knots: toggling it back on is instant)
                P.segs[hit].selected = !P.segs[hit].selected;
                P.ovdirty = P.maskdirty = true;
                P.tr.blocked = false;
            }
            else if (P.selvalid) {
                P.selvalid = false;
                P.ovdirty = true;
            }
        }
    }
    P.prevdown = down;

    // ---- drawing (direct) ----
    bool budm = mainprogram->directmode;
    mainprogram->directmode = true;
    draw_box(grey, black, P.bx, P.by, P.bw, P.bh, -1);

    if (v2) {
        // webcam 2 alone
        if (haveframe) {
            draw_direct(nullptr, g_transp, irx, iry, iw, ih, -(P.zu - 0.5f) / 6.0f, -(P.zv - 0.5f) / 6.0f, 1.0f / vz, 1.0f, 0, P.camtex2, glob->w, glob->h, false, false);
        }
        else {
            std::string msg = P.cap2.state == 3 ? "Webcam 2 could not be opened (in use?)" : "Opening webcam 2...";
            render_text(msg, white, P.bx + P.bw * 0.5f - 0.15f, P.by + P.bh * 0.5f, 0.0007f, 0.0012f);
        }
    }
    else if (haveframe) {
        // (draw_direct shows row 0 of a texture at the top: CPU top-down data is upright with a positive height)
        draw_direct(nullptr, g_transp, irx, iry, iw, ih, -(P.zu - 0.5f) / 6.0f, -(P.zv - 0.5f) / 6.0f, 1.0f / vz, 1.0f, 0, P.camtex, glob->w, glob->h, false, false);
        int ow = P.snap.empty() ? P.camtexw : P.sw, oh = P.snap.empty() ? P.camtexh : P.sh;
        if (P.ovdirty || P.ovw != ow || P.ovh != oh) bake_overlay(ow, oh);
        if (P.ovtex != (GLuint) -1) {
            draw_direct(nullptr, g_transp, irx, iry, iw, ih, -(P.zu - 0.5f) / 6.0f, -(P.zv - 0.5f) / 6.0f, 1.0f / vz, 1.0f, 0, P.ovtex, glob->w, glob->h, false, false);
        }
        // Preview "Map": what the last stripe scan made of every webcam pixel, over the image (same size as the snapshot)
        if ((int) (P.preview->value + 0.5f) == 1 && P.whytex != (GLuint) -1) {
            draw_direct(nullptr, g_transp, irx, iry, iw, ih, -(P.zu - 0.5f) / 6.0f, -(P.zv - 0.5f) / 6.0f, 1.0f / vz, 1.0f, 0, P.whytex, glob->w, glob->h, false, false);
        }
    }
    else {
        std::string msg = (int) (P.cam->value + 0.5f) == 0 ? "Choose a webcam" :
                          (P.cap.state == 3 ? "The webcam could not be opened (in use?)" : "Opening webcam...");
        render_text(msg, white, P.bx + P.bw * 0.5f - 0.15f, P.by + P.bh * 0.5f, 0.0007f, 0.0012f);
    }
    if (hasv && !v2) {   // (the state of webcam 2, small, in the corner of the canvas)
        std::string m2 = P.cap2.state == 2 ? "Webcam 2: streaming" : (P.cap2.state == 3 ? "Webcam 2 could not be opened (in use?)" : "Opening webcam 2...");
        render_text(m2, white, P.bx + 0.01f, P.by + P.bh - 0.04f, 0.0005f, 0.0009f);
    }

    // selection box while dragging / when set
    if (haveframe && !v2) {
        float bx0 = 0, by0 = 0, bx1 = 0, by1 = 0;
        bool show = false;
        if (P.dragging && std::hypot((float) mainprogram->mx - P.dragpx, (float) mainprogram->my - P.dragpy) > 4.0f) {
            float ex = std::clamp(mu, 0.0f, 1.0f), ey = std::clamp(mv, 0.0f, 1.0f);
            uv2n(std::min(P.dragu, ex), std::min(P.dragv, ey), bx0, by1);
            uv2n(std::max(P.dragu, ex), std::max(P.dragv, ey), bx1, by0);
            show = true;
        }
        else if (P.selvalid) {
            uv2n(P.sel[0], P.sel[1], bx0, by1);
            uv2n(P.sel[2], P.sel[3], bx1, by0);
            show = true;
        }
        if (show) {
            nline(white, bx0, by0, bx1, by0);
            nline(white, bx1, by0, bx1, by1);
            nline(white, bx1, by1, bx0, by1);
            nline(white, bx0, by1, bx0, by0);
        }
        // outline curves + knots of the selected segments (webcam side); green = found on the beamer
        for (const Seg &s : P.segs) {
            if (!s.selected && !s.resolving) continue;   // (the curve and knots of shapes to be resolved / being resolved)
            if ((int) (P.method->value + 0.5f) == 0) {
                // Method "Outline": the SAM3 mask outline itself, no curve approximation
                const int nd = (int) s.dense.size() / 2;
                for (int i = 0; i < nd; i++) {
                    int j = (i + 1) % nd;
                    float ax, ay, bx_, by_;
                    uv2n(s.dense[i * 2], s.dense[i * 2 + 1], ax, ay);
                    uv2n(s.dense[j * 2], s.dense[j * 2 + 1], bx_, by_);
                    nline(white, ax, ay, bx_, by_);   // (white: selected for RESOLVE)
                }
                continue;
            }
            std::vector<float> pts;
            closed_curve(s.kcam, s.kcorner, 6, pts);
            int n = (int) pts.size() / 2;
            for (int i = 0; i < n; i++) {
                int j = (i + 1) % n;
                float ax, ay, bx_, by_;
                uv2n(pts[i * 2], pts[i * 2 + 1], ax, ay);
                uv2n(pts[j * 2], pts[j * 2 + 1], bx_, by_);
                nline(white, ax, ay, bx_, by_);
            }
            for (size_t k = 0; k < s.kdone.size(); k++) {
                float kx, ky;
                uv2n(s.kcam[k * 2], s.kcam[k * 2 + 1], kx, ky);
                if (kx < irx || kx > irx + iw || ky < iry || ky > iry + ih) continue;   // (zoomed: not in view)
                draw_box(s.kdone[k] ? g_lgreen : g_lightblue, kx, ky, (k < s.kcorner.size() && s.kcorner[k]) ? 0.011f : 0.007f, 1);   // (corners bigger)
            }
        }
        // what the last outline check measured: from every outline point (where the lit line should be) a line along the outward
        // normal to where the lit line really is, four times as long as the real offset so that it can be seen. Orange: the lit
        // line is outside the SAM3 outline, green: inside it.
        if (P.sw > 0 && P.sh > 0) {
            for (size_t i = 0; i + 4 < P.vdiag.size(); i += 5) {
                float ax, ay, bx_, by_;
                uv2n(P.vdiag[i] / (float) P.sw, P.vdiag[i + 1] / (float) P.sh, ax, ay);
                uv2n((P.vdiag[i] + 4.0f * P.vdiag[i + 2]) / (float) P.sw, (P.vdiag[i + 1] + 4.0f * P.vdiag[i + 3]) / (float) P.sh, bx_, by_);
                nline(P.vdiag[i + 4] > 0.0f ? g_orange : g_lgreen, ax, ay, bx_, by_);
            }
        }
    }

    // the V button
    if (hasv) {
        draw_box(P.vbox->lcolor, v2 ? green : darkgrey, P.vbox, -1);
        render_text("V", white, P.vbox->vtxcoords->x1 + P.vbox->vtxcoords->w * 0.3f, P.vbox->vtxcoords->y1 + P.vbox->vtxcoords->h * 0.3f, 0.00055f, 0.0009f);
        if (overv && mainprogram->leftmouse && nomenu && !P.promptEditing) {
            P.viewcam2 = !P.viewcam2;
            P.dragging = false;
            mainprogram->leftmouse = false;
        }
    }

    // prompt box: replaces the big window while typing
    if (P.promptEditing) {
        draw_box(grey, black, P.bx, P.by, P.bw, P.bh, -1);
        float tw = P.bw * 0.7f, th = 0.3f;
        P.textbox->vtxcoords->x1 = P.bx + (P.bw - tw) * 0.5f;
        P.textbox->vtxcoords->y1 = cy - th * 0.5f;
        P.textbox->vtxcoords->w = tw;
        P.textbox->vtxcoords->h = th;
        P.textbox->upvtxtoscr();
        draw_box(white, darkgreen2, P.textbox, -1);
        render_text("PROMPT (ENTER to finish)", white, P.textbox->vtxcoords->x1, P.textbox->vtxcoords->y1 + th + 0.01f, 0.0006f, 0.001f);
        if (mainprogram->renaming == EDIT_SEGPROMPT) {
            P.promptlines = do_text_input_multiple_lines(
                P.textbox->vtxcoords->x1 + 0.025f, P.textbox->vtxcoords->y1 + th - 0.1f, 0.00072f, 0.00120f,
                mainprogram->mx, mainprogram->my, mainprogram->xvtxtoscr(tw - 0.05f), 0.05f, 4, 0, nullptr);
            P.promptstr.clear();
            for (auto &line : P.promptlines) {
                if (!P.promptstr.empty()) P.promptstr += " ";
                P.promptstr += line;
            }
            if (!P.textbox->in() && mainprogram->leftmouse) {
                // (a click on PROMPT itself only closes the box: it must not toggle PROMPT, or open the box again, in the same click)
                const bool onprompt = P.promptbox->in();
                end_prompt_edit();
                if (onprompt) mainprogram->leftmouse = false;
            }
        }
        else {
            // (ENTER ended the text input: the prompt box disappears, PROMPT stays on)
            end_prompt_edit();
        }
    }

    // ---- status bar: the steps ----
    {
        std::string status;
        Step steps[6];
        float pct;
        {
            std::lock_guard<std::mutex> lock(P.mtx);
            for (int i = 0; i < 6; i++) steps[i] = P.steps[i];
            status = P.status;
            pct = P.pct;
        }
        float sx = P.bx, sy = P.by, sw = P.bw / 6.0f;
        render_text(status, white, P.bx + 0.01f, sy + barh + 0.005f, 0.00048f, 0.0008f);
        for (int i = 0; i < 6; i++) {
            float *ac = steps[i].state == 1 ? g_orange : steps[i].state == 2 ? darkgreen1 : steps[i].state == 3 ? darkred1 : g_greyb;
            draw_box(white, ac, sx + i * sw, sy, sw, barh, -1);
            render_text(steps[i].name, white, sx + i * sw + 0.012f, sy + 0.022f, 0.0005f, 0.0008f);
        }
        (void) pct;
    }

    // ---- control panel ----
    // opening the Webcam / Webcam 2 / Beamer dropdown scans for devices and displays again (they may have been plugged in since)
    if (mainprogram->leftmouse && nomenu && !P.promptEditing && (P.cam->box->in() || P.cam2->box->in() || P.beam->box->in())) {
        auto curname = [](Param *p) {
            int i = (int) (p->value + 0.5f);
            return (i >= 1 && i < (int) p->options.size()) ? p->options[i] : std::string();
        };
        std::string camname = curname(P.cam), cam2name = curname(P.cam2), beamname = curname(P.beam);
        refresh_device_lists();
        // (the choice stays on the same device when the lists changed order; a device that is gone leaves the choice as it is)
        auto reselect = [](Param *p, const std::string &nm, int &last) {
            if (nm.empty()) return;
            for (int i = 1; i < (int) p->options.size(); i++) {
                if (p->options[i] == nm) { p->value = (float) i; last = i; return; }
            }
        };
        reselect(P.cam, camname, P.lastcam);
        reselect(P.cam2, cam2name, P.lastcam2);
        reselect(P.beam, beamname, P.lastbeam);
    }
    const bool showknots = (int) (P.method->value + 0.5f) == 1;   // (Knots and Corner threshold belong to the Knots curve method)
    for (int n = 0; n < 10; n++) {
        if (!showknots && (plist[n] == P.knots || plist[n] == P.corner)) continue;
        if (plist[n] == P.fov2 && (int) (P.cam2->value + 0.5f) == 0) continue;   // (webcam 2's own field of view: only with webcam 2)
        plist[n]->handle();
    }
    bool camok = (int) (P.cam->value + 0.5f) > 0 && P.cap.state == 2;
    bool running = P.running.load();
    {
        // SEGMENT (greyed out without a webcam)
        draw_box(P.segbox->lcolor, running ? darkred1 : (camok ? darkgreen1 : darkgrey), P.segbox, -1);
        render_text(running ? "STOP" : "SEGMENT", white, P.segbox->vtxcoords->x1 + 0.008f, P.segbox->vtxcoords->y1 + 0.022f, 0.00042f, 0.0009f);
        // PROMPT (green when on)
        draw_box(P.promptbox->lcolor, P.promptOn ? green : darkgrey, P.promptbox, -1);
        render_text("PROMPT", white, P.promptbox->vtxcoords->x1 + 0.012f, P.promptbox->vtxcoords->y1 + 0.022f, 0.00042f, 0.0009f);
        // EXPOSE: toggles the grey on the beamer (green when on)
        draw_box(P.exposebox->lcolor, P.expose ? green : darkgrey, P.exposebox, -1);
        render_text("EXPOSE", white, P.exposebox->vtxcoords->x1 + 0.008f, P.exposebox->vtxcoords->y1 + 0.022f, 0.00042f, 0.0009f);
        // RESOLVE: the selected shapes get the current layer (needs at least one selected shape and a current layer)
        int nsel = 0;
        for (const Seg &s : P.segs) if (s.selected) nsel++;
        Layer *curlay = nullptr;   // (no layer is given here: layers are assigned to the masks in the Mapping view)
        bool canresolve = nsel > 0 && !running && P.bwin != nullptr;   // (a beamer has to be chosen: the shapes are traced on it)
        draw_box(P.resolvebox->lcolor, canresolve ? darkgreen1 : darkgrey, P.resolvebox, -1);
        render_text("RESOLVE", white, P.resolvebox->vtxcoords->x1 + 0.02f, P.resolvebox->vtxcoords->y1 + 0.022f, 0.00042f, 0.0009f);
        // CLEAN: wipes all segmentations
        const bool canclean = !P.segs.empty() || !P.segs2.empty() || running || !P.snap.empty();
        draw_box(P.cleanbox->lcolor, canclean ? darkred1 : darkgrey, P.cleanbox, -1);
        render_text("CLEAN", white, P.cleanbox->vtxcoords->x1 + 0.03f, P.cleanbox->vtxcoords->y1 + 0.022f, 0.00042f, 0.0009f);
        if (mainprogram->leftmouse && nomenu && !P.promptEditing) {
            if (P.cleanbox->in()) {
                if (canclean) {
                    if (running) {
                        P.cancel = true;
                        P.gen++;
                        if (P.sam) P.sam->cancelSegmentation();
                    }
                    clear_mapped();   // (the projection of the resolved shapes in this view stops; their masks stay in the Mapping view)
                    reset_results();
                    {
                        std::lock_guard<std::mutex> lock(P.mtx);
                        for (Step &st : P.steps) st.state = 0;
                        P.pct = 0.0f;
                    }
                    P.nextsegid = 0;
                    P.selvalid = false;
                    P.scene = nullptr;
                    P.sceneSent = nullptr;
                    set_status("Cleaned: click SEGMENT to start over");
                }
                mainprogram->leftmouse = false;
            }
            else if (P.exposebox->in()) {
                P.expose = !P.expose;   // (the beamer shows the grey when idle: see idle_mode)
                mainprogram->leftmouse = false;
            }
            else if (P.resolvebox->in()) {
                if (canresolve) resolve_selected(curlay);
                mainprogram->leftmouse = false;
            }
            else if (P.segbox->in()) {
                if (running) {
                    P.cancel = true;
                    P.gen++;
                    if (P.sam) P.sam->cancelSegmentation();
                    set_status("Stopped");
                }
                else if (camok) start_segmentation();
                mainprogram->leftmouse = false;
            }
            else if (P.promptbox->in()) {
                P.promptOn = !P.promptOn;
                if (P.promptOn) {
                    P.promptEditing = true;
                    mainprogram->renaming = EDIT_SEGPROMPT;
                    mainprogram->inputtext = P.promptstr;
                    mainprogram->cursorpos0 = mainprogram->inputtext.length();
                    SDL_StartTextInput(mainprogram->mainwindow);
                }
                mainprogram->leftmouse = false;
            }
        }
    }
    draw_box(P.cancelbox->lcolor, P.cancelbox->acolor, P.cancelbox, -1);
    render_text("CANCEL", white, P.cancelbox->vtxcoords->x1 + 0.04f, P.cancelbox->vtxcoords->y1 + 0.022f, 0.00055f, 0.0009f);
    if (P.cancelbox->in() && mainprogram->leftmouse && nomenu) {
        mainprogram->leftmouse = false;
        projmap_cancel();
        mainprogram->directmode = budm;
        return;
    }

    // clicks in the canvas / controls are not for the global undo
    if (incanvas || inpanel) mainprogram->recundo = false;
    mainprogram->directmode = budm;
}
