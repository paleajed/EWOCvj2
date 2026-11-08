#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#else
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <mmsystem.h>
#endif
#include <RtMidi.h>

#include "TimecodeManager.h"

// (see program.h)
#ifdef MACOS
#define EWOC_RTMIDI_API RtMidi::MACOSX_CORE
#else
#define EWOC_RTMIDI_API RtMidi::UNSPECIFIED
#endif

namespace {
    const double MTC_TIMEOUT = 0.25;   // seconds without a quarter frame (they come 4 per frame): the sender stopped

    double now_secs() {
        return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    double rate_fps(int code, bool &drop) {
        drop = false;
        switch (code & 3) {
            case 0: return 24.0;
            case 1: return 25.0;
            case 2: drop = true; return 30000.0 / 1001.0;
            default: return 30.0;
        }
    }
}

// ---------------------------------------------------------------- label <-> seconds

int timecode_nominal_fps(double fps) {
    return (int)std::lround(fps);
}

TimecodeLabel timecode_label(double secs, double fps, bool drop) {
    TimecodeLabel l;
    if (secs < 0.0) secs = 0.0;
    if (fps <= 0.0) fps = 25.0;
    const int nom = timecode_nominal_fps(fps);
    long long n = (long long)std::floor(secs * fps + 1e-6);
    if (drop && nom == 30) {
        // 29.97 drop frame: frame numbers 0 and 1 are skipped at the start of every minute except every tenth
        const long long per10 = 17982;   // frames in ten minutes
        const long long per1 = 1798;     // frames in a minute that drops
        const long long d = n / per10;
        const long long m = n % per10;
        n += 18 * d;
        if (m >= 2) n += 2 * ((m - 2) / per1);
    }
    l.f = (int)(n % nom);
    l.s = (int)((n / nom) % 60);
    l.m = (int)((n / ((long long)nom * 60)) % 60);
    l.h = (int)((n / ((long long)nom * 3600)) % 24);
    return l;
}

double timecode_secs(const TimecodeLabel &l, double fps, bool drop) {
    if (fps <= 0.0) fps = 25.0;
    const int nom = timecode_nominal_fps(fps);
    long long n = (((long long)l.h * 60 + l.m) * 60 + l.s) * nom + l.f;
    if (drop && nom == 30) {
        const long long totalmin = (long long)l.h * 60 + l.m;
        n -= 2 * (totalmin - totalmin / 10);
    }
    return (double)n / fps;
}

std::string timecode_string(const TimecodeLabel &l, bool drop) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d%c%02d", l.h, l.m, l.s, drop ? ';' : ':', l.f);
    return buf;
}

bool timecode_parse(const std::string &str, TimecodeLabel &label) {
    int v[4] = {0, 0, 0, 0};
    int count = 0;
    long cur = 0;
    bool incur = false;
    for (size_t i = 0; i <= str.size(); i++) {
        const char c = i < str.size() ? str[i] : ':';
        if (c >= '0' && c <= '9') {
            cur = cur * 10 + (c - '0');
            if (cur > 999) return false;
            incur = true;
        }
        else if (c == ':' || c == ';' || c == '.' || c == ' ') {
            if (incur) {
                if (count >= 4) return false;
                v[count++] = (int)cur;
                cur = 0;
                incur = false;
            }
        }
        else return false;
    }
    if (count == 0) return false;
    // fewer fields count from the right: "ss:ff", "ff"
    int out[4] = {0, 0, 0, 0};
    for (int k = 0; k < count; k++) out[4 - count + k] = v[k];
    label.h = out[0];
    label.m = out[1];
    label.s = out[2];
    label.f = out[3];
    return label.m < 60 && label.s < 60;
}

// ---------------------------------------------------------------- cues

const char *cue_action_name(int action) {
    static const char *names[CUE_ACTION_COUNT] = {"CLIP NEXT", "PLAY", "PAUSE", "STOP", "JUMP", "SHELF", "SCENE",
                                                  "LPST START", "LPST STOP", "LOAD BIN", "OSC"};
    return names[std::min(std::max(action, 0), (int)CUE_ACTION_COUNT - 1)];
}

const char *cue_color_name(int color) {
    static const char *names[CUE_COLOR_COUNT] = {"none", "red", "orange", "yellow", "green", "cyan", "blue", "purple", "pink"};
    return names[std::min(std::max(color, 0), CUE_COLOR_COUNT - 1)];
}

namespace {
    // "A1", "b12" -> deck letter and number; rest is what follows the number (trimmed)
    bool parse_deck_num(const std::string &s, char &deck, int &num, std::string &rest) {
        size_t i = 0;
        while (i < s.size() && s[i] == ' ') i++;
        if (i >= s.size()) return false;
        const char c = (char)toupper((unsigned char)s[i]);
        if (c != 'A' && c != 'B') return false;
        deck = c;
        i++;
        size_t st = i;
        long n = 0;
        while (i < s.size() && s[i] >= '0' && s[i] <= '9') {
            n = n * 10 + (s[i] - '0');
            if (n > 100000) return false;
            i++;
        }
        if (i == st) return false;
        num = (int)n;
        while (i < s.size() && s[i] == ' ') i++;
        rest = s.substr(i);
        return num >= 1;
    }

    bool parse_int_only(const std::string &s, int &num) {
        size_t i = 0;
        while (i < s.size() && s[i] == ' ') i++;
        size_t st = i;
        long long n = 0;
        while (i < s.size() && s[i] >= '0' && s[i] <= '9') {
            n = n * 10 + (s[i] - '0');
            if (n > 1000000000LL) return false;
            i++;
        }
        if (i == st) return false;
        while (i < s.size() && s[i] == ' ') i++;
        if (i != s.size()) return false;
        num = (int)n;
        return true;
    }
}

std::string cue_osc_line(int action, const std::string &target) {
    char deck = 'A';
    int n = 0;
    std::string rest;
    switch (action) {
        case CUE_CLIPNEXT:
        case CUE_PLAY:
        case CUE_PAUSE:
        case CUE_STOP:
        case CUE_JUMP: {
            if (!parse_deck_num(target, deck, n, rest)) return "";
            std::string base = std::string("/mix/output/deck") + deck + "/layer/" + std::to_string(n) + "/";
            if (action == CUE_CLIPNEXT) return base + "queue/next";
            if (action == CUE_PLAY) return base + "play 1";
            if (action == CUE_PAUSE) return base + "pause";
            if (action == CUE_STOP) return base + "stop";
            // jump: a frame number follows (a number past the end of the loop counts back from the end of the video)
            int frame = 0;
            if (!parse_int_only(rest, frame)) return "";
            return base + "gotoframe " + std::to_string(frame);
        }
        case CUE_SHELF: {
            // "A2 5 B3": element 5 of bank 2 of shelf A into layer 3 of deck B.  The bank and the layer are always given.
            std::vector<std::string> tok;
            {
                size_t i = 0;
                while (i < target.size()) {
                    while (i < target.size() && target[i] == ' ') i++;
                    size_t st = i;
                    while (i < target.size() && target[i] != ' ') i++;
                    if (i > st) tok.push_back(target.substr(st, i - st));
                }
            }
            if (tok.size() != 3) return "";
            int bank = 0, elem = 0;
            std::string r;
            if (!parse_deck_num(tok[0], deck, n, r) || !r.empty() || n > 4) return "";
            bank = n;
            if (!parse_int_only(tok[1], elem) || elem < 1 || elem > 16) return "";
            char dd = 'A';
            int dl = 0;
            if (!parse_deck_num(tok.back(), dd, dl, r) || !r.empty() || dl > 1000) return "";
            return std::string("/shelf/") + deck + "/trigger " + std::to_string(elem) + " " + std::to_string(bank) + " " +
                   dd + " " + std::to_string(dl);
        }
        case CUE_LOADBIN: {
            // "<bin name> A1 3": the bin name is everything before the last two words (it may hold spaces)
            std::string t = target;
            while (!t.empty() && t.back() == ' ') t.pop_back();
            size_t p2 = t.rfind(' ');
            if (p2 == std::string::npos) return "";
            int block = 0;
            if (!parse_int_only(t.substr(p2 + 1), block) || block < 1 || block > 9) return "";
            std::string head = t.substr(0, p2);
            while (!head.empty() && head.back() == ' ') head.pop_back();
            size_t p1 = head.rfind(' ');
            if (p1 == std::string::npos) return "";
            std::string bankstr = head.substr(p1 + 1), name = head.substr(0, p1);
            while (!name.empty() && name.back() == ' ') name.pop_back();
            size_t ns = name.find_first_not_of(' ');
            if (ns == std::string::npos || name.find('"') != std::string::npos) return "";
            name = name.substr(ns);
            std::string r;
            if (!parse_deck_num(bankstr, deck, n, r) || !r.empty() || n > 4) return "";
            return std::string("/bin/loadshelf ") + deck + " " + std::to_string(n) + " " + std::to_string(block) + " \"" + name + "\"";
        }
        case CUE_SCENE: {
            int sc = 0;
            if (parse_int_only(target, sc)) return (sc >= 1 && sc <= 4) ? "/mix/scene " + std::to_string(sc) : "";
            if (!parse_deck_num(target, deck, n, rest) || !rest.empty() || n > 4) return "";
            return std::string("/mix/scene/deck") + deck + " " + std::to_string(n);
        }
        case CUE_LPSTSTART:
        case CUE_LPSTSTOP: {
            int line = 0;
            if (!parse_int_only(target, line) || line < 1 || line > 256) return "";
            return "/loopstation/line/" + std::to_string(line) + "/loop " + (action == CUE_LPSTSTART ? "1" : "0");
        }
        case CUE_OSC: {
            size_t i = 0;
            while (i < target.size() && target[i] == ' ') i++;
            if (i >= target.size() || target[i] != '/') return "";
            return target.substr(i);
        }
        default:
            return "";
    }
}

void TimecodeManager::sort_cues() {
    std::stable_sort(cues.begin(), cues.end(), [](const TimecodeCue &a, const TimecodeCue &b) { return a.secs < b.secs; });
}

std::vector<int> TimecodeManager::due_cues(bool live, const std::function<double(const TimecodeCue &)> &lead) {
    std::vector<int> due;
    TimecodeState st;
    if (!live || !get_state(st)) {
        haslast = false;
        return due;
    }
    const double JUMP = 3.0;    // seconds: a bigger step forward is a locate, not playing
    if (haslast && st.secs > lastsecs && st.secs - lastsecs <= JUMP) {
        for (int i = 0; i < (int)cues.size(); i++) {
            TimecodeCue &c = cues[i];
            // (an armed cue exactly at the position the step started from, like one at 00:00:00:00, fires too.  A cue
            // fires its lead early: half a display frame, so the nearest frame gets it, or the pre-roll)
            const double ft = c.secs - (lead ? std::max(0.0, lead(c)) : 0.0);
            if ((ft > lastsecs || !c.fired) && ft <= st.secs) {
                c.fired = true;
                if (c.enabled) due.push_back(i);
            }
        }
    }
    else if (!haslast || st.secs != lastsecs) {
        // first position, or a jump: nothing fires; the cues behind the position count as passed
        // (a cue exactly at the position stays armed: it fires when the position moves on)
        for (TimecodeCue &c : cues) c.fired = c.secs < st.secs;
        jumpflag = true;
    }
    lastsecs = st.secs;
    haslast = true;
    return due;
}

int TimecodeManager::next_cue(double secs) const {
    for (int i = 0; i < (int)cues.size(); i++) {
        if (cues[i].enabled && cues[i].secs > secs) return i;
    }
    return -1;
}

// ---------------------------------------------------------------- frame rates

void timecode_fps_from_index(int idx, double &fps, bool &drop) {
    // (new rates are added at the end: the index is saved in the preferences)
    static const double rates[TIMECODE_FPS_COUNT] = {24000.0 / 1001.0, 24.0, 25.0, 30000.0 / 1001.0, 30000.0 / 1001.0, 30.0,
                                                     48.0, 50.0, 60000.0 / 1001.0, 60.0};
    idx = std::min(std::max(idx, 0), TIMECODE_FPS_COUNT - 1);
    fps = rates[idx];
    drop = (idx == 4);
}

int timecode_fps_index(double fps, bool drop) {
    for (int i = 0; i < TIMECODE_FPS_COUNT; i++) {
        double f;
        bool d;
        timecode_fps_from_index(i, f, d);
        if (std::fabs(f - fps) < 0.01 && d == drop) return i;
    }
    // (a source that counts drop frame at a rate that is not 29.97 DF: the plain rate)
    for (int i = 0; i < TIMECODE_FPS_COUNT; i++) {
        double f;
        bool d;
        timecode_fps_from_index(i, f, d);
        if (std::fabs(f - fps) < 0.01) return i;
    }
    return 2;
}

const char *timecode_fps_name(int idx) {
    static const char *names[TIMECODE_FPS_COUNT] = {"23.976", "24", "25", "29.97", "29.97 DF", "30",
                                                 "48", "50", "59.94", "60"};
    return names[std::min(std::max(idx, 0), TIMECODE_FPS_COUNT - 1)];
}

// ---------------------------------------------------------------- manager

struct TimecodeManager::Impl {
    mutable std::mutex mtx;
    // which input is listened to (own lock: the MIDI and network threads read it for every packet)
    mutable std::mutex filtmtx;
    std::string mtcport;                        // MIDI port the MTC comes from ("" any)
    std::string artsender;                      // IP address the Art-Net timecode comes from ("" any)
    std::map<std::string, double> artseen;      // senders of ArtTimeCode packets and when each was last heard
    int source = TC_OFF;
    double fps = 25.0;
    bool drop = false;
    std::atomic<double> offsetsecs{0.0};
    std::atomic<double> lookahead{0.0};
    std::atomic<double> jitter{0.0};    // average distance (s) of arriving positions from the smoothed clock
    std::atomic<double> freewheel{0.0}; // seconds that the clock runs on after the packets stop

    // lock: packets that follow the clock within a frame; and a far-off packet is only believed when the next one agrees
    int lockcount = 0;
    bool haspend = false;
    double pendpos = 0.0;
    double pendstamp = 0.0;

    // timecode out
    std::thread outthread;
    std::atomic<bool> outquit{false};
    std::atomic<int> outmode{0};
    std::atomic<bool> outok{true};
    std::string outport;
    RtMidiOut *midiout = nullptr;
    // LTC generator (audio thread): the frame being written, where in it (samples), the bits of that frame, the level
    std::atomic<bool> ltcreset{true};
    bool genvalid = false;
    long long genframe = 0;
    double genpos = 0.0;
    int gencell = -1;
    bool genmid = false;
    float genlevel = 0.5f;
    unsigned char genbits[80];

    // internal clock
    bool irunning = false;
    double ibase = 0.0;     // position when it was last started/stopped/set
    double istart = 0.0;    // when it was last started

    std::atomic<int> srcnow{TC_OFF};    // (source, readable without the lock from the audio thread)

    // the position that an external source (MIDI timecode, Art-Net, LTC) sent last
    bool extvalid = false;
    bool extheld = false;       // a locate (MTC full frame): the sender is stopped there
    double extpos = 0.0;
    double extstamp = 0.0;      // when extpos was true
    double extfps = 25.0;
    bool extdrop = false;
    double extrate = 1.0;       // clock seconds per timecode second (a sender that plays at another speed)

    // MIDI timecode
    unsigned char qf[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    unsigned qfmask = 0;
    int lastpiece = -1;

    // Art-Net listener
    std::thread artthread;
    std::atomic<bool> artquit{false};

    // LTC decoder (audio thread)
    // level: the signal's own centre and swing are followed (DC offset, any input level), a transition is the centre
    // crossing located between two samples and only believed once the signal has really passed to the other side
    double ltchi = 0.0, ltclo = 0.0;    // envelope of the signal
    float ltcprev = 0.0f;               // previous sample
    bool ltcside = false;               // the side of the centre that the signal is confirmed on
    bool ltchavesign = false;
    bool ltcpend = false;               // crossed the centre, not yet beyond the hysteresis
    double ltcpendt = 0.0;              // when it crossed (in samples, fractional)
    bool ltcnodt = true;                // the next transition has no predecessor to measure from
    long long ltcsample = 0;            // sample counter
    double ltclast = 0.0;               // time (samples) of the last transition
    double ltcbp = 0.0;                 // samples per bit cell, tracked
    bool ltchalf = false;               // a half cell was seen: its partner makes a 1
    unsigned char ltcbits[80];
    int ltccount = 0;                   // bits in ltcbits (up to 80)
    // sample clock -> system clock: time of sample i = ltcoff + i / samplerate.  The audio arrives in blocks at uneven
    // moments, but the sample count does not jitter, so the offset follows the earliest arrival (fastest delivery).
    double ltcoff = 0.0;
    bool ltcoffvalid = false;
    double ltclastarrival = 0.0;
    // validation
    bool ltcprevok = false;             // the previous frame's label is known (for the two-in-a-row check at the start)
    double ltcprevsecs = 0.0;
    int ltcprevf = -1;
    int ltcmaxf = 0;                    // highest frame number seen in a sequence: 29, 24 or 23 tells the counting
    // The correction bit (27, at 25 fps 59) makes the number of 0 bits of a frame even.  A generator that does not
    // follow this would never lock if the rule were enforced at once, so it is enforced only after this stream has
    // shown it follows it (a run of frames that comply).
    int ltcparstreak = 0;
    bool ltcstrict = false;
    // what the table shows of the LTC input, to see where a missing signal is lost: audio arriving at all, its level,
    // sync words found, frames thrown out
    std::atomic<unsigned long long> ltcdiagblocks{0};
    std::atomic<unsigned long long> ltcdiagsyncs{0};
    std::atomic<unsigned long long> ltcdiagbad{0};
    std::atomic<float> ltcdiagpeak{0.0f};
    std::atomic<int> ltcdiagrate{0};

    void ltc_lost() {
        ltcparstreak = 0;
        ltcstrict = false;
        ltchavesign = false;
        ltcpend = false;
        ltcnodt = true;
        ltchalf = false;
        ltccount = 0;
        ltcprevok = false;
        ltcprevf = -1;
        ltcmaxf = 0;
    }

    void reset_inputs() {
        qfmask = 0;
        lastpiece = -1;
        extvalid = false;
        extheld = false;
        lockcount = 0;
        haspend = false;
        ltc_lost();
        ltcbp = 0.0;
        ltchi = ltclo = 0.0;
        ltcoffvalid = false;
    }

    // How long without a packet before the source counts as stopped.  LTC delivers a position every frame, so four frame
    // periods tell (at least 0.1 s, for audio blocks that arrive late); MTC and Art-Net keep the longer time.
    double stop_timeout() const {
        if (source == TC_LTC) return std::min(MTC_TIMEOUT, std::max(0.1, 4.0 / (extfps > 0.0 ? extfps : 25.0)));
        return MTC_TIMEOUT;
    }

    // (caller holds the lock)
    void set_ext(double pos, double stamp, double fps, bool drop, bool held) {
        // A running source is followed by a small phase-locked clock instead of jumping to every packet: the packets
        // arrive with jitter (MIDI and network timing, audio block boundaries), the clock between them runs smoothly.
        // A packet that lies more than 100 ms off, a stop, a locate or a frame rate change takes over at once.
        if (extvalid && !extheld && !held && std::fabs(fps - extfps) < 1e-6 && drop == extdrop) {
            // (with a freewheel the clock ran on through the dropout: the packets that come back are compared with it)
            const double dt = stamp - extstamp;
            if (dt > 0.001 && dt < stop_timeout() + freewheel.load()) {
                const double err = pos - (extpos + extrate * dt);
                if (std::fabs(err) < 0.1) {
                    jitter.store(jitter.load() + 0.1 * (std::fabs(err) - jitter.load()));
                    extrate += 0.02 * err / dt;
                    extrate = 1.0 + (extrate - 1.0) * 0.995;
                    extrate = std::min(1.1, std::max(0.9, extrate));
                    extpos = extpos + extrate * dt + 0.25 * err;
                    extstamp = stamp;
                    haspend = false;
                    // locked: a few packets in a row within a frame of the clock
                    if (std::fabs(err) < 1.0 / fps) lockcount = std::min(lockcount + 1, 1000);
                    else lockcount = 0;
                    return;
                }
                // A packet that is far off a running clock is a glitch (a misread frame, a stray packet) or a locate.
                // It is only believed when the next packet agrees with it; one stray packet changes nothing.
                if (haspend && stamp - pendstamp < stop_timeout() &&
                    std::fabs(pos - (pendpos + (stamp - pendstamp))) < 0.1) {
                    haspend = false;     // confirmed: take over below
                }
                else {
                    haspend = true;
                    pendpos = pos;
                    pendstamp = stamp;
                    lockcount = 0;
                    return;
                }
            }
        }
        haspend = false;
        lockcount = 0;
        extrate = 1.0;
        extpos = pos;
        extstamp = stamp;
        extfps = fps;
        extdrop = drop;
        extvalid = true;
        extheld = held;
    }

    void artnet_loop();

    double internal_pos(double t) const {
        return ibase + (irunning ? t - istart : 0.0);
    }
};

TimecodeManager::TimecodeManager() : impl(new Impl) {
}

TimecodeManager::~TimecodeManager() {
    set_output(TCO_OFF, "");
    impl->artquit = true;
    if (impl->artthread.joinable()) impl->artthread.join();
}

void TimecodeManager::set_source(int source) {
    bool startart = false, stopart = false;
    {
        std::lock_guard<std::mutex> lock(impl->mtx);
        if (impl->source == source) return;
        stopart = impl->source == TC_ARTNET;
        startart = source == TC_ARTNET;
        impl->source = source;
        impl->srcnow = source;
        impl->reset_inputs();
    }
    // the Art-Net listener only runs while that is the source (the thread takes the lock itself: not held here)
    if (stopart) {
        impl->artquit = true;
        if (impl->artthread.joinable()) impl->artthread.join();
    }
    if (startart) {
        if (impl->artthread.joinable()) impl->artthread.join();
        impl->artquit = false;
        impl->artthread = std::thread([this]() { impl->artnet_loop(); });
    }
}

int TimecodeManager::source() const {
    std::lock_guard<std::mutex> lock(impl->mtx);
    return impl->source;
}

void TimecodeManager::set_fps(double fps, bool drop) {
    std::lock_guard<std::mutex> lock(impl->mtx);
    impl->fps = fps;
    impl->drop = drop && timecode_nominal_fps(fps) == 30;
}

void TimecodeManager::set_offset_ms(double ms) {
    impl->offsetsecs = ms / 1000.0;
}

bool TimecodeManager::get_state(TimecodeState &state, bool raw) const {
    std::lock_guard<std::mutex> lock(impl->mtx);
    state = TimecodeState();
    const double t = now_secs();
    switch (impl->source) {
        case TC_INTERNAL:
            state.valid = true;
            state.running = impl->irunning;
            state.secs = impl->internal_pos(t);
            state.locked = true;
            state.fps = impl->fps;
            state.drop = impl->drop;
            break;
        case TC_MTC:
        case TC_ARTNET:
        case TC_LTC: {
            if (!impl->extvalid) return false;
            state.valid = true;
            state.fps = impl->extfps;
            state.drop = impl->extdrop;
            if (impl->extheld) {
                state.running = false;
                state.secs = impl->extpos;
            }
            else {
                // positions only arrive every frame or two: in between it advances with the clock
                const double el = t - impl->extstamp;
                // no packets for longer than the timeout: a stop, or a dropout that the freewheel bridges (the clock
                // then runs on at the rate it had, and holds when the freewheel time is over)
                const double fw = impl->freewheel.load();
                const double tmo = impl->stop_timeout();
                state.running = el <= tmo + fw;
                state.freewheeling = el > tmo && el <= tmo + fw;
                state.locked = impl->lockcount >= 5 && el <= tmo;
                // A stop is only known once the timeout has passed, but the source stopped at its last packet: the
                // position holds there (plus the freewheel time that was asked for), not where the clock had run on to
                // while waiting for the next packet.
                const double run = el <= tmo + fw ? el : fw;
                state.secs = impl->extpos + impl->extrate * run;
            }
            break;
        }
        default:
            return false;
    }
    // (a stopped source stays where it is: only a running one is read ahead)
    if (raw) return true;
    state.secs = std::max(0.0, state.secs + impl->offsetsecs.load() + (state.running ? impl->lookahead.load() : 0.0));
    return true;
}

void TimecodeManager::set_freewheel_ms(double ms) {
    impl->freewheel = std::min(10.0, std::max(0.0, ms / 1000.0));
}

void TimecodeManager::set_lookahead(double secs) {
    impl->lookahead = std::min(0.2, std::max(0.0, secs));
}

double TimecodeManager::jitter_ms() const {
    return impl->jitter.load() * 1000.0;
}

void TimecodeManager::ltc_diag(float &peak, unsigned long long &blocks, unsigned long long &syncs, unsigned long long &bad, int &rate) const {
    peak = impl->ltcdiagpeak.load();
    blocks = impl->ltcdiagblocks.load();
    syncs = impl->ltcdiagsyncs.load();
    bad = impl->ltcdiagbad.load();
    rate = impl->ltcdiagrate.load();
}

void TimecodeManager::internal_play() {
    std::lock_guard<std::mutex> lock(impl->mtx);
    if (impl->irunning) return;
    impl->irunning = true;
    impl->istart = now_secs();
}

void TimecodeManager::internal_stop() {
    std::lock_guard<std::mutex> lock(impl->mtx);
    if (!impl->irunning) return;
    impl->ibase = impl->internal_pos(now_secs());
    impl->irunning = false;
}

void TimecodeManager::internal_reset() {
    std::lock_guard<std::mutex> lock(impl->mtx);
    impl->irunning = false;
    impl->ibase = 0.0;
}

void TimecodeManager::internal_set(double secs) {
    std::lock_guard<std::mutex> lock(impl->mtx);
    impl->ibase = std::max(0.0, secs);
    impl->istart = now_secs();
}

std::vector<std::string> timecode_midi_output_ports() {
    std::vector<std::string> out;
    try {
        RtMidiOut mo(EWOC_RTMIDI_API);
        const unsigned n = mo.getPortCount();
        for (unsigned i = 0; i < n; i++) {
            const std::string nm = mo.getPortName(i);
            if (!nm.empty() && std::find(out.begin(), out.end(), nm) == out.end()) out.push_back(nm);
        }
    }
    catch (RtMidiError &) {
    }
    return out;
}

void TimecodeManager::set_mtc_port(const std::string &port) {
    std::lock_guard<std::mutex> lock(impl->filtmtx);
    impl->mtcport = port;
}

void TimecodeManager::set_artnet_sender(const std::string &ip) {
    std::lock_guard<std::mutex> lock(impl->filtmtx);
    impl->artsender = ip;
}

std::vector<std::string> TimecodeManager::artnet_senders() const {
    std::vector<std::string> out;
    std::lock_guard<std::mutex> lock(impl->filtmtx);
    const double t = now_secs();
    for (const auto &kv : impl->artseen) {
        if (t - kv.second < 10.0) out.push_back(kv.first);
    }
    return out;
}

void TimecodeManager::on_midi(const std::string &port, const unsigned char *data, size_t size) {
    {
        std::lock_guard<std::mutex> lock(impl->filtmtx);
        if (!impl->mtcport.empty() && impl->mtcport != port) return;
    }
    on_midi(data, size);
}

void TimecodeManager::on_midi(const unsigned char *data, size_t size) {
    if (!data || size < 1) return;
    std::lock_guard<std::mutex> lock(impl->mtx);
    if (impl->source != TC_MTC) return;

    if (data[0] == 0xF1 && size >= 2) {
        // quarter frame: piece number in the high nibble, 4 bits of the time in the low one
        const int piece = (data[1] >> 4) & 7;
        const unsigned char val = data[1] & 0x0F;
        if (piece == 0 || piece == (impl->lastpiece + 1)) {
            if (piece == 0) impl->qfmask = 0;
            impl->qf[piece] = val;
            impl->qfmask |= 1u << piece;
            impl->lastpiece = piece;
        }
        else {
            // out of order (a lost message, or the sender runs backwards): wait for the next complete sequence
            impl->qfmask = 0;
            impl->lastpiece = -1;
            return;
        }
        if (piece == 7 && impl->qfmask == 0xFF) {
            TimecodeLabel l;
            l.f = impl->qf[0] | ((impl->qf[1] & 1) << 4);
            l.s = impl->qf[2] | ((impl->qf[3] & 3) << 4);
            l.m = impl->qf[4] | ((impl->qf[5] & 3) << 4);
            l.h = impl->qf[6] | ((impl->qf[7] & 1) << 4);
            bool drop = false;
            const double fps = rate_fps(impl->qf[7] >> 1, drop);
            if (l.m < 60 && l.s < 60) {
                // the time in the 8 pieces is the one of the first piece: two frames have passed since
                impl->set_ext(timecode_secs(l, fps, drop) + 2.0 / fps, now_secs(), fps, drop, false);
            }
            impl->qfmask = 0;
            impl->lastpiece = -1;
        }
    }
    else if (data[0] == 0xF0 && size >= 10 && data[1] == 0x7F && data[3] == 0x01 && data[4] == 0x01) {
        // full frame message (a locate): F0 7F <device> 01 01 hr mn sc fr F7, rate in bits 5-6 of hr
        TimecodeLabel l;
        l.h = data[5] & 0x1F;
        l.m = data[6] & 0x3F;
        l.s = data[7] & 0x3F;
        l.f = data[8] & 0x1F;
        bool drop = false;
        const double fps = rate_fps(data[5] >> 5, drop);
        if (l.m < 60 && l.s < 60) {
            impl->set_ext(timecode_secs(l, fps, drop), now_secs(), fps, drop, true);
            impl->qfmask = 0;
            impl->lastpiece = -1;
        }
    }
}

// ---------------------------------------------------------------- Art-Net timecode in

void TimecodeManager::Impl::artnet_loop() {
    // ArtTimeCode (opcode 0x9700) packets, broadcast on UDP port 6454:
    //   "Art-Net\0", OpCode lo/hi, ProtVer hi/lo, 2 filler bytes, Frames, Seconds, Minutes, Hours, Type (0 film 24,
    //   1 EBU 25, 2 drop frame 29.97, 3 SMPTE 30)
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    const bool bad = sock == INVALID_SOCKET;
#else
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    const bool bad = sock < 0;
#endif
    if (bad) return;

    int yes = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));
#ifdef _WIN32
    DWORD tmo = 100;    // ms: wakes up to see if it should quit
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tmo, sizeof(tmo));
#else
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
#endif
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(6454);
    if (bind(sock, (sockaddr*)&addr, sizeof(addr)) != 0) {
        printf("Timecode: could not listen for Art-Net on UDP port 6454\n");
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        return;
    }

    unsigned char buf[512];
    while (!artquit) {
        sockaddr_in from;
        memset(&from, 0, sizeof(from));
#ifdef _WIN32
        int fromlen = sizeof(from);
#else
        socklen_t fromlen = sizeof(from);
#endif
        int n = (int)recvfrom(sock, (char*)buf, sizeof(buf), 0, (sockaddr*)&from, &fromlen);
        if (n < 19) continue;
        if (memcmp(buf, "Art-Net", 8) != 0) continue;
        if (buf[8] != 0x00 || buf[9] != 0x97) continue;
        {
            // who sends it (listed in the table), and the choice of sender
            const std::string ip = inet_ntoa(from.sin_addr);
            std::lock_guard<std::mutex> flock(filtmtx);
            artseen[ip] = now_secs();
            if (!artsender.empty() && artsender != ip) continue;
        }
        TimecodeLabel l;
        l.f = buf[14];
        l.s = buf[15];
        l.m = buf[16];
        l.h = buf[17];
        if (l.m >= 60 || l.s >= 60 || l.f >= 30) continue;
        double fps = 25.0;
        bool drop = false;
        switch (buf[18] & 3) {
            case 0: fps = 24.0; break;
            case 1: fps = 25.0; break;
            case 2: fps = 30000.0 / 1001.0; drop = true; break;
            default: fps = 30.0; break;
        }
        // (the packet is the frame that starts now)
        std::lock_guard<std::mutex> lock(mtx);
        if (source == TC_ARTNET) set_ext(timecode_secs(l, fps, drop), now_secs(), fps, drop, false);
    }
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
}

// ---------------------------------------------------------------- timecode out

namespace {
    // MTC / Art-Net / LTC count 24, 25, 29.97 drop frame and 30 fps; 23.976 goes out as 24, 29.97 non-drop as 30.  The
    // code is the one of the rate bits (0 24, 1 25, 2 29.97 DF, 3 30); false for rates that these formats do not have.
    bool out_rate_code(double fps, bool drop, int &code) {
        const int nom = timecode_nominal_fps(fps);
        if (nom == 24) code = 0;
        else if (nom == 25) code = 1;
        else if (nom == 30) code = drop ? 2 : 3;
        else return false;
        return true;
    }

    void sleep_short() {
        std::this_thread::sleep_for(std::chrono::microseconds(500));
    }
}

bool TimecodeManager::set_output(int mode, const std::string &midiport) {
    Impl &I = *impl;
    mode = std::min(std::max(mode, 0), (int)TCO_COUNT - 1);
    if (mode != TCO_OFF && mode == I.outmode.load() && midiport == I.outport && I.outok.load()) return true;   // (running)

    I.outquit = true;
    if (I.outthread.joinable()) I.outthread.join();
    if (I.midiout) {
        try { delete I.midiout; } catch (RtMidiError &) {}
        I.midiout = nullptr;
    }
    I.outmode = TCO_OFF;
    I.outok = true;
    I.ltcreset = true;
    I.outport = midiport;
    if (mode == TCO_OFF) return true;

    I.outquit = false;
    if (mode == TCO_MTC) {
        RtMidiOut *out = nullptr;
        try {
            out = new RtMidiOut(EWOC_RTMIDI_API);
            const unsigned int n = out->getPortCount();
            int portnr = -1;
            if (midiport.empty()) {
                if (n > 0) portnr = 0;
            }
            else {
                for (unsigned int i = 0; i < n && portnr < 0; i++) {
                    if (out->getPortName(i) == midiport) portnr = (int)i;
                }
                for (unsigned int i = 0; i < n && portnr < 0; i++) {
                    if (out->getPortName(i).find(midiport) != std::string::npos) portnr = (int)i;
                }
            }
            if (portnr < 0) { delete out; I.outok = false; I.outmode = mode; return false; }
            out->openPort((unsigned int)portnr);
            if (!out->isPortOpen()) { delete out; I.outok = false; I.outmode = mode; return false; }
        }
        catch (RtMidiError &) {
            delete out;
            I.outok = false;
            I.outmode = mode;
            return false;
        }
        I.midiout = out;
        I.outmode = mode;
        I.outthread = std::thread([this]() { mtc_out_loop(); });
    }
    else if (mode == TCO_ARTNET) {
        I.outmode = mode;
        I.outthread = std::thread([this]() { artnet_out_loop(); });
    }
    else {
        I.outmode = mode;     // (LTC: the audio output pulls ltc_generate())
    }
    return true;
}

bool TimecodeManager::output_ok() const {
    return impl->outmode.load() == TCO_OFF || impl->outok.load();
}

void TimecodeManager::mtc_out_loop() {
    // Quarter frames (4 per frame, 8 pieces carry the label of the frame that the first piece began) while running; a
    // full frame message when stopped or after a locate.  The pieces follow the position: piece q % 8 goes out when
    // the position reaches quarter frame q, so the first piece of a group leaves exactly at the start of its frame.
    Impl &I = *impl;
#ifdef _WIN32
    timeBeginPeriod(1);
#endif
    long long lastq = -1;
    long long stopframe = -1;
    auto send = [&I](const std::vector<unsigned char> &msg) {
        if (!I.midiout) return;
        try {
            std::vector<unsigned char> m = msg;
            I.midiout->sendMessage(&m);
        }
        catch (RtMidiError &) {}
    };
    auto full = [&](long long frame, double fps, bool drop, int code) {
        const TimecodeLabel l = timecode_label(((double)frame + 0.5) / fps, fps, drop);
        send({0xF0, 0x7F, 0x7F, 0x01, 0x01, (unsigned char)((code << 5) | (l.h & 0x1F)), (unsigned char)l.m,
              (unsigned char)l.s, (unsigned char)l.f, 0xF7});
    };
    while (!I.outquit) {
        TimecodeState st;
        int code = 0;
        if (!get_state(st, true) || !st.valid || !out_rate_code(st.fps, st.drop, code)) {
            lastq = -1;
            stopframe = -1;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }
        const double fps = st.fps;
        if (!st.running) {
            // stopped: tell where it is, once per position
            const long long fr = (long long)std::floor(st.secs * fps + 1e-6);
            if (fr != stopframe) {
                full(fr, fps, st.drop, code);
                stopframe = fr;
            }
            lastq = -1;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }
        stopframe = -1;
        const long long q = (long long)std::floor(st.secs * fps * 4.0 + 1e-6);
        if (lastq < 0 || q < lastq || q - lastq > 8) {
            // start, or a locate: say where we are, the pieces follow from the next quarter frame
            full(q / 4, fps, st.drop, code);
            lastq = q;
        }
        else {
            while (lastq < q) {
                lastq++;
                const int piece = (int)(lastq % 8);
                const long long group = (lastq - piece) / 4;       // the frame the group's first piece began
                const TimecodeLabel l = timecode_label(((double)group + 0.5) / fps, fps, st.drop);
                int nib = 0;
                switch (piece) {
                    case 0: nib = l.f & 0xF; break;
                    case 1: nib = (l.f >> 4) & 0x1; break;
                    case 2: nib = l.s & 0xF; break;
                    case 3: nib = (l.s >> 4) & 0x3; break;
                    case 4: nib = l.m & 0xF; break;
                    case 5: nib = (l.m >> 4) & 0x3; break;
                    case 6: nib = l.h & 0xF; break;
                    default: nib = ((l.h >> 4) & 0x1) | (code << 1); break;
                }
                send({0xF1, (unsigned char)((piece << 4) | nib)});
            }
        }
        sleep_short();
    }
#ifdef _WIN32
    timeEndPeriod(1);
#endif
}

void TimecodeManager::artnet_out_loop() {
    // ArtTimeCode (see the input), broadcast on UDP 6454, one packet at the start of every frame while running
    Impl &I = *impl;
#ifdef _WIN32
    timeBeginPeriod(1);
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    const bool bad = sock == INVALID_SOCKET;
#else
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    const bool bad = sock < 0;
#endif
    if (bad) {
        I.outok = false;
#ifdef _WIN32
        timeEndPeriod(1);
#endif
        return;
    }
    int yes = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char*)&yes, sizeof(yes));
    sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    dest.sin_port = htons(6454);

    long long lastframe = -1;
    bool lastrunning = false;
    while (!I.outquit) {
        TimecodeState st;
        int code = 0;
        if (!get_state(st, true) || !st.valid || !out_rate_code(st.fps, st.drop, code)) {
            lastframe = -1;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }
        const long long fr = (long long)std::floor(st.secs * st.fps + 1e-6);
        // (while stopped only a new position is sent: a locate)
        if (fr != lastframe && (st.running || lastrunning || lastframe >= 0)) {
            const TimecodeLabel l = timecode_label(((double)fr + 0.5) / st.fps, st.fps, st.drop);
            unsigned char pkt[19];
            memcpy(pkt, "Art-Net", 8);
            pkt[8] = 0x00;
            pkt[9] = 0x97;
            pkt[10] = 0;
            pkt[11] = 14;
            pkt[12] = 0;
            pkt[13] = 0;
            pkt[14] = (unsigned char)l.f;
            pkt[15] = (unsigned char)l.s;
            pkt[16] = (unsigned char)l.m;
            pkt[17] = (unsigned char)l.h;
            pkt[18] = (unsigned char)code;
            sendto(sock, (const char*)pkt, sizeof(pkt), 0, (sockaddr*)&dest, sizeof(dest));
        }
        lastframe = fr;
        lastrunning = st.running;
        sleep_short();
    }
#ifdef _WIN32
    closesocket(sock);
    timeEndPeriod(1);
#else
    close(sock);
#endif
}

void TimecodeManager::ltc_generate(float *out, size_t n, int samplerate) {
    // Linear timecode: 80 bits per frame, biphase mark coded (a level change at the start of every bit cell, and one in
    // the middle of a cell for a 1).  The generator keeps its own sample clock; it follows the position and starts over
    // when it is more than a frame away from it.
    Impl &I = *impl;
    std::fill(out, out + n, 0.0f);
    if (samplerate <= 0 || I.outmode.load() != TCO_LTC) {
        I.genvalid = false;
        return;
    }
    TimecodeState st;
    int code = 0;
    if (!get_state(st, true) || !st.valid || !st.running || !out_rate_code(st.fps, st.drop, code)) {
        I.genvalid = false;
        return;
    }
    const double fps = st.fps;
    const double framelen = (double)samplerate / fps;      // samples
    const double cell = framelen / 80.0;
    const double exact = st.secs * fps;
    const long long tf = (long long)std::floor(exact + 1e-9);
    const double tpos = (exact - (double)tf) * framelen;

    auto encode = [&I, fps, &st](long long frame) {
        const TimecodeLabel l = timecode_label(((double)frame + 0.5) / fps, fps, st.drop);
        unsigned char* b = I.genbits;
        std::fill(b, b + 80, (unsigned char)0);
        auto put = [b](int first, int count, int value) {
            for (int k = 0; k < count; k++) b[first + k] = (unsigned char)((value >> k) & 1);
        };
        put(0, 4, l.f % 10);
        put(8, 2, l.f / 10);
        b[10] = st.drop ? 1 : 0;
        put(16, 4, l.s % 10);
        put(24, 3, l.s / 10);
        put(32, 4, l.m % 10);
        put(40, 3, l.m / 10);
        put(48, 4, l.h % 10);
        put(56, 2, l.h / 10);
        static const unsigned char SYNC[16] = {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1};
        for (int k = 0; k < 16; k++) b[64 + k] = SYNC[k];
    };

    if (I.ltcreset.exchange(false)) I.genvalid = false;
    if (!I.genvalid || std::fabs(((double)I.genframe * framelen + I.genpos) - ((double)tf * framelen + tpos)) > framelen) {
        I.genframe = tf;
        I.genpos = tpos;
        I.gencell = -1;
        I.genmid = false;
        encode(I.genframe);
        I.genvalid = true;
    }
    for (size_t k = 0; k < n; k++) {
        if (I.genpos >= framelen) {
            I.genpos -= framelen;
            I.genframe++;
            I.gencell = -1;
            encode(I.genframe);
        }
        int ci = (int)(I.genpos / cell);
        if (ci > 79) ci = 79;
        if (ci != I.gencell) {
            I.genlevel = -I.genlevel;
            I.gencell = ci;
            I.genmid = false;
        }
        if (I.genbits[ci] && !I.genmid && (I.genpos - (double)ci * cell) >= cell * 0.5) {
            I.genlevel = -I.genlevel;
            I.genmid = true;
        }
        out[k] = I.genlevel;
        I.genpos += 1.0;
    }
}

// ---------------------------------------------------------------- LTC (linear timecode) from audio

void TimecodeManager::on_audio(const float *samples, size_t n, int samplerate) {
    if (impl->srcnow.load() != TC_LTC || !samples || n == 0 || samplerate <= 0) return;
    std::lock_guard<std::mutex> lock(impl->mtx);
    if (impl->source != TC_LTC) return;
    Impl &I = *impl;
    {
        float bp = 0.0f;
        for (size_t k = 0; k < n; k++) bp = std::max(bp, std::fabs(samples[k]));
        I.ltcdiagpeak = std::max(bp, I.ltcdiagpeak.load() * 0.9f);
        I.ltcdiagblocks++;
        I.ltcdiagrate = samplerate;
    }
    const double sr = (double)samplerate;
    const double arrival = now_secs();   // (when this block was pulled from the audio device)
    const double MINSWING = 0.02;        // a peak-to-peak swing below this is no signal (noise floor)
    if (I.ltcbp <= 0.0) I.ltcbp = sr / (25.0 * 80.0);   // until measured: 25 fps

    // sample clock -> system clock.  cand is the offset that this block implies; it can only be too late (the block
    // waited to be pulled), never too early, so the offset follows the minimum and creeps up slowly to follow drift.
    {
        const double cand = arrival - (double)(I.ltcsample + (long long)n - 1) / sr;
        if (!I.ltcoffvalid || cand - I.ltcoff > 0.5) { I.ltcoff = cand; I.ltcoffvalid = true; }
        else {
            I.ltcoff += std::max(0.0, std::min(0.5, arrival - I.ltclastarrival)) * 5e-4;
            if (cand < I.ltcoff) I.ltcoff = cand;
        }
        I.ltclastarrival = arrival;
    }

    // sync word, in the order the bits arrive (bits 64 - 79 of the frame)
    static const unsigned char SYNC[16] = {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1};
    const double envdecay = 1.0 / (0.2 * sr);

    for (size_t k = 0; k < n; k++) {
        const float s = samples[k];
        const double idx = (double)I.ltcsample;     // index of this sample
        I.ltcsample++;
        const float pv = I.ltcprev;
        I.ltcprev = s;

        // envelope: both peaks drift towards the centre and are pushed out by the signal
        {
            const double m = (I.ltchi + I.ltclo) * 0.5;
            I.ltchi -= (I.ltchi - m) * envdecay;
            I.ltclo += (m - I.ltclo) * envdecay;
            if (s > I.ltchi) I.ltchi = s;
            if (s < I.ltclo) I.ltclo = s;
        }
        const double swing = I.ltchi - I.ltclo;
        const double mid = (I.ltchi + I.ltclo) * 0.5;
        const double hyst = 0.15 * swing;
        if (swing < MINSWING) {
            if (I.ltchavesign) I.ltc_lost();
            continue;
        }
        if (!I.ltchavesign) {
            if (std::fabs(s - mid) > hyst) {
                I.ltcside = s > mid;
                I.ltchavesign = true;
                I.ltcpend = false;
                I.ltcnodt = true;
                I.ltclast = idx;
            }
            continue;
        }

        // a transition: the signal crossed the centre and then went past the hysteresis on the other side
        const double v = (I.ltcside ? 1.0 : -1.0) * ((double)s - mid);     // > 0: still on the confirmed side
        bool transition = false;
        double tt = 0.0;
        if (v < 0.0 && !I.ltcpend) {
            I.ltcpend = true;
            const double den = (double)s - (double)pv;
            const double frac = std::fabs(den) > 1e-12 ? std::min(1.0, std::max(0.0, (mid - (double)pv) / den)) : 1.0;
            I.ltcpendt = idx - 1.0 + frac;      // between the previous sample and this one
        }
        else if (v >= 0.0 && I.ltcpend) {
            I.ltcpend = false;                  // it came back: a glitch, not a transition
        }
        if (I.ltcpend && v < -hyst) {
            transition = true;
            tt = I.ltcpendt;
            I.ltcpend = false;
            I.ltcside = !I.ltcside;
        }
        if (!transition) {
            // no transition for more than 3 cells: no signal
            if (idx - I.ltclast > I.ltcbp * 3.0) I.ltc_lost();
            continue;
        }
        if (I.ltcnodt) {
            I.ltcnodt = false;
            I.ltclast = tt;
            continue;
        }
        const double dt = tt - I.ltclast;
        I.ltclast = tt;
        int bit = -1;
        if (dt > I.ltcbp * 0.75 && dt < I.ltcbp * 1.5) {
            // a whole cell: a 0 (when a half cell was waiting for its partner, this is out of step)
            if (I.ltchalf) { I.ltchalf = false; I.ltccount = 0; continue; }
            bit = 0;
            I.ltcbp = I.ltcbp * 0.97 + dt * 0.03;
        }
        else if (dt > I.ltcbp * 0.25 && dt <= I.ltcbp * 0.75) {
            if (I.ltchalf) {
                I.ltchalf = false;
                bit = 1;
                I.ltcbp = I.ltcbp * 0.97 + (dt * 2.0) * 0.03;
            }
            else {
                I.ltchalf = true;
                continue;
            }
        }
        else {
            // not a cell length: lost
            I.ltchalf = false;
            I.ltccount = 0;
            if (dt >= I.ltcbp * 1.5 && dt < I.ltcbp * 6.0) {
                // (a different speed, 24 - 30 fps: start measuring again)
                I.ltcbp = std::min(std::max(dt, sr / (30.0 * 80.0 * 1.2)), sr / (24.0 * 80.0 * 0.8));
            }
            continue;
        }

        // keep the last 80 bits
        if (I.ltccount < 80) I.ltcbits[I.ltccount++] = (unsigned char)bit;
        else {
            memmove(I.ltcbits, I.ltcbits + 1, 79);
            I.ltcbits[79] = (unsigned char)bit;
        }
        if (I.ltccount < 80) continue;
        if (memcmp(I.ltcbits + 64, SYNC, 16) != 0) continue;
        I.ltcdiagsyncs++;

        // a frame: the fields are the bits, least significant first
        auto field = [&](int start, int len) {
            int v = 0;
            for (int b = 0; b < len; b++) v |= (I.ltcbits[start + b] & 1) << b;
            return v;
        };
        TimecodeLabel l;
        l.f = field(0, 4) + 10 * field(8, 2);
        l.s = field(16, 4) + 10 * field(24, 3);
        l.m = field(32, 4) + 10 * field(40, 3);
        l.h = field(48, 4) + 10 * field(56, 2);
        const bool dropflag = field(10, 1) != 0;
        // every digit has to be a decimal digit and the fields have to be in range (a bit error shows up here)
        if (field(0, 4) > 9 || field(16, 4) > 9 || field(32, 4) > 9 || field(48, 4) > 9 ||
            l.h >= 24 || l.m >= 60 || l.s >= 60 || l.f >= 30) {
            I.ltcdiagbad++;
            I.ltccount = 0;
            I.ltchalf = false;
            continue;
        }

        // even number of 0 bits in the 80 (the sync word included)
        {
            int zeros = 0;
            for (int b = 0; b < 80; b++) zeros += I.ltcbits[b] ? 0 : 1;
            if (zeros % 2 == 0) {
                if (++I.ltcparstreak >= 10) I.ltcstrict = true;
            }
            else {
                I.ltcparstreak = 0;
                if (I.ltcstrict) {      // a bit error in a stream that has been following the rule
                    I.ltcdiagbad++;
                    I.ltccount = 0;
                    I.ltchalf = false;
                    continue;
                }
            }
        }

        // the counting: the highest frame number of a sequence tells (29 / 24 / 23), else the speed of the bits (which
        // also holds at a varispeed of some percent); only consecutive frames count, so a bit error cannot set it
        if (I.ltcprevf >= 0 && l.f == I.ltcprevf + 1) I.ltcmaxf = std::max(I.ltcmaxf, l.f);
        int nominal;
        if (I.ltcmaxf >= 29) nominal = 30;
        else if (I.ltcmaxf >= 24) nominal = 25;
        else if (I.ltcmaxf >= 23) nominal = 24;
        else {
            const double est = sr / (I.ltcbp * 80.0);
            nominal = est < 24.5 ? 24 : (est < 27.5 ? 25 : 30);
        }
        double fps = 25.0;
        bool drop = false;
        if (nominal == 24) fps = (std::lround(I.fps) == 24) ? I.fps : 24.0;    // 23.976 or 24: the chosen rate says
        else if (nominal == 25) fps = 25.0;
        else {
            // 29.97 drop frame has its flag; a 30 without it may be 30 or 29.97 non-drop: the chosen frame rate says
            if (dropflag) { fps = 30000.0 / 1001.0; drop = true; }
            else fps = (std::lround(I.fps) == 30) ? I.fps : 30.0;
        }
        const int prevf = I.ltcprevf;
        I.ltcprevf = l.f;
        if (l.f >= timecode_nominal_fps(fps)) { I.ltccount = 0; I.ltchalf = false; continue; }

        const double secs = timecode_secs(l, fps, drop);
        // the first frame after a start or a loss is believed only when the next one follows it: noise can imitate a
        // sync word, two correct frames in a row it cannot
        const bool follows = I.ltcprevok && prevf >= 0 && std::fabs(secs - (I.ltcprevsecs + 1.0 / fps)) < 0.5 / fps;
        I.ltcprevsecs = secs;
        I.ltcprevok = true;
        I.ltccount = 0;
        I.ltchalf = false;
        if (!I.extvalid && !follows) continue;

        // The frame ended with the last transition (a cell ends where the next one begins), tt samples on the sample
        // clock; the label is the frame that began 80 cells before, so the position now is one frame on.
        const double stamp = I.ltcoff + tt / sr;
        I.set_ext(secs + 1.0 / fps, stamp, fps, drop, false);
    }
}
