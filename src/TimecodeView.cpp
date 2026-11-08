#ifndef USE_GLES
#include "GL/glew.h"
#include "GL/gl.h"
#define FREEGLUT_STATIC
#define FREEGLUT_LIB_PRAGMAS 0
#endif

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <thread>
#include <map>

// my own header
#include "program.h"
#include "LayerWarp.h"
#include "ProjMap.h"
#include "MaskMap.h"

// ======================================================================================================
// Timecode table (live modus only): the timecode source, and the cue list "at this timecode, do this".
// It covers the deck and mix monitors (and the buttons around them); small copies of the monitors are
// drawn under the loopstation meanwhile.  Cues run OSC messages, see cue_osc_line() and osc_inject().
// ======================================================================================================

namespace {
    // text size: that of the names of the Params
    const float TX = 0.00045f;
    const float TY = 0.00075f;
    const float CHARW = 0.0068f;     // about the width of a character at that size, to cut texts to their cell
    const float ROWH = 0.04f;
    const int ROWS = 9;              // cue rows that are visible (the tenth row of the table is the strip with IMPORT / EXPORT / SHIFT ALL)

    const char *SOURCENAMES[5] = {"OFF", "MIDI (MTC)", "ART-NET", "LTC", "INTERNAL"};

    // keyboard entry in a cell
    struct CellEdit {
        bool active = false;
        int row = -1;
        int col = -1;       // 0 time, 1 name, 2 target, 3 the internal clock's time
        std::string buf;
    } ed;

    // the dedicated editor of a time (hh:mm:ss:ff): the four numbers are clicked and typed one by one, the colons stay
    struct FieldEdit {
        bool on = false;
        int field = 0;          // 0 hours .. 3 frames
        int val[4] = {0, 0, 0, 0};
        std::string digits;     // typed into the focused field so far (0-2 digits)
        int nominal = 25;       // frames per second of the counting: the frames run to nominal-1
        bool drop = false;
    } fe;

    int field_max(int f) {
        return f == 0 ? 99 : (f == 3 ? std::max(0, fe.nominal - 1) : 59);
    }

    // the typed digits become the value of the focused field
    void field_settle() {
        if (!fe.digits.empty()) {
            fe.val[fe.field] = std::min(field_max(fe.field), std::atoi(fe.digits.c_str()));
            fe.digits.clear();
        }
    }

    void field_focus(int f) {
        field_settle();
        fe.field = ((f % 4) + 4) % 4;
    }

    void field_digit(int d) {
        fe.digits += (char)('0' + d);
        if (fe.digits.size() >= 2) {
            field_settle();
            fe.field = (fe.field + 1) % 4;      // full: on to the next, wrapping around
        }
    }

    // the value shown for a field: what is being typed, else what it was
    int field_shown(int f) {
        if (f == fe.field && !fe.digits.empty()) return std::min(field_max(f), std::atoi(fe.digits.c_str()));
        return fe.val[f];
    }

    std::string field_string() {
        char buf[32];
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d:%02d", fe.val[0], fe.val[1], fe.val[2], fe.val[3]);
        return buf;
    }

    bool shiftneg = false;      // SHIFT ALL moves the cues earlier

    int scrollrow = 0;
    int actionpick = -1;        // the cue whose action list is open
    bool prefsdirty = false;    // source / frame rate / offset changed: saved when the table is left
    Param *offpar = nullptr;
    Param *prerollpar = nullptr;
    Param *fpspar = nullptr;
    const char *DIAG_TIP = "LTC input: the level of the audio that arrives (dB, 0 = full scale; -60 or lower is silence), its sample rate, how many sync words were found and how many frames were rejected (bad digits or parity). 'no audio arrives' means the input device delivers nothing. jitter: how far the arriving positions lie from the smoothed clock. cue: how late the last cue fired against its time. lag: how many frames the decoders of chased layers are behind the frame that the timecode asks for. ";
    // the list that opens when MTC or LTC is chosen as the timecode output (and on a rightclick on OUT): the MIDI output
    // port / the audio output device to send it to.  Kept out of the header, which has enough in it.
    // It is the standard foldout menu of the program, opened at the mouse.
    int outpick = 0;                        // 0 closed, 1 audio output devices (LTC), 2 MIDI output ports (MTC)
    std::vector<std::string> outpicknames;  // the choices, the first one ("") is the default
    Menu *outmenu = nullptr;

    void open_outpick(int kind) {
        outpick = kind;
        outpicknames.clear();
        if (kind == 3) {
            // the timecode source (kind 3): the same foldout, the index is the source
            std::vector<std::string> entries;
            const int curs = (int)std::lround(mainprogram->tcsource);
            for (int s = 0; s < 5; s++) {
                outpicknames.push_back(SOURCENAMES[s]);
                entries.push_back((s == curs ? "v  " : "   ") + std::string(SOURCENAMES[s]));
            }
            mainprogram->make_menu("timecodeoutmenu", outmenu, entries);
            outmenu->state = 2;
            outmenu->menux = mainprogram->mx;
            outmenu->menuy = mainprogram->my;
            mainprogram->menuactivation = false;
            return;
        }
        outpicknames.push_back("");
        if (kind == 1) {
            int cnt = 0;
            SDL_AudioDeviceID *devs = SDL_GetAudioPlaybackDevices(&cnt);
            for (int k = 0; k < cnt; k++) {
                const char *nm = SDL_GetAudioDeviceName(devs[k]);
                if (nm) outpicknames.push_back(nm);
            }
            if (devs) SDL_free(devs);
        }
        else {
            for (const std::string &nm : timecode_midi_output_ports()) outpicknames.push_back(nm);
        }
        // a chosen one that is not there (unplugged) stays in the list
        const std::string &cur = kind == 1 ? mainprogram->ltcoutdevice : mainprogram->mtcoutport;
        if (!cur.empty() && std::find(outpicknames.begin(), outpicknames.end(), cur) == outpicknames.end()) outpicknames.push_back(cur);
        // the foldout: the current choice is marked with a "v"
        std::vector<std::string> entries;
        for (const std::string &nm : outpicknames) {
            std::string label = nm;
            if (label.empty()) label = kind == 1 ? "(default output device)" : "(the MIDI clock output port)";
            entries.push_back((nm == cur ? "v  " : "   ") + label);
        }
        mainprogram->make_menu("timecodeoutmenu", outmenu, entries);
        outmenu->state = 2;
        outmenu->menux = mainprogram->mx;
        outmenu->menuy = mainprogram->my;
        mainprogram->menuactivation = false;
    }

    Param *inpar = nullptr;                 // the MIDI port (MTC) / the sender (Art-Net) to listen to
    float inparsetval = -1.0f;
    int inparsrc = -1;                      // the source that the list of inpar belongs to
    Param *ltcchanpar = nullptr;            // the channel of that input
    float ltcchansetval = 0.0f;             // (the value that its param is made with: not a choice made in the menu)
    Param *ltcpar = nullptr;                // the audio input for LTC (an option param)
    float ltcsetval = 0.0f;                 // (the value that its param is made with: not a choice made in the menu)
    std::vector<std::string> ltcnames;      // the recording devices (refreshed now and then)
    int ltcnamesage = 1000;
    Param *fwpar = nullptr;
    Param *outpar = nullptr;
    float outsetval = 0.0f;                 // (the value that its param is made with: not a choice made in the menu)
    int colorpick = -1;         // the cue whose colour palette is open

    // the colours of the cue palette (index 1..), and a dark version of each for the row background
    float *cue_fill(int color, bool dim) {
        static const float pal[CUE_COLOR_COUNT][4] = {
            {0.3f, 0.3f, 0.3f, 1.0f}, {0.85f, 0.2f, 0.2f, 1.0f}, {0.92f, 0.5f, 0.12f, 1.0f}, {0.92f, 0.85f, 0.2f, 1.0f},
            {0.25f, 0.7f, 0.3f, 1.0f}, {0.2f, 0.75f, 0.8f, 1.0f}, {0.25f, 0.4f, 0.9f, 1.0f}, {0.6f, 0.3f, 0.85f, 1.0f},
            {0.9f, 0.4f, 0.7f, 1.0f}};
        static float full[CUE_COLOR_COUNT][4], darkv[CUE_COLOR_COUNT][4];
        static bool init = false;
        if (!init) {
            for (int k = 0; k < CUE_COLOR_COUNT; k++) {
                for (int j = 0; j < 4; j++) {
                    full[k][j] = pal[k][j];
                    darkv[k][j] = j < 3 ? pal[k][j] * 0.35f + 0.06f : 1.0f;
                }
            }
            init = true;
        }
        color = std::min(std::max(color, 0), CUE_COLOR_COUNT - 1);
        return dim ? darkv[color] : full[color];
    }
    float fpssetval = -1.0f;

    // the target of a cue that has just got this action: the first element of the type, in the format it needs
    std::string default_target(int action) {
        switch (action) {
            case CUE_JUMP: return "A1 0";
            case CUE_LPSTSTART:
            case CUE_LPSTSTOP: return "1";
            case CUE_LOADBIN: {
                const std::string name = (binsmain && !binsmain->bins.empty()) ? binsmain->bins[0]->name : "bin";
                return name + " A1 1";
            }
            case CUE_OSC: return "";
            case CUE_SHELF: return "A1 1 A1";   // bank 1 of shelf A, element 1, into layer 1 of deck A
            default: return "A1";   // CLIP NEXT, PLAY, PAUSE, STOP, SCENE
        }
    }

    // LOAD BIN target: "<bin name> A1 3".  The bin is chosen with an option Param named BIN (listing the bins), the bank
    // and block (the last two words) are typed.
    void loadbin_split(const std::string &t, std::string &name, std::string &rest) {
        std::string s = t;
        while (!s.empty() && s.back() == ' ') s.pop_back();
        name.clear();
        rest = s;
        const size_t p2 = s.rfind(' ');
        if (p2 == std::string::npos || p2 == 0) return;
        const size_t p1 = s.rfind(' ', p2 - 1);
        if (p1 == std::string::npos) return;
        name = s.substr(0, p1);
        while (!name.empty() && name.back() == ' ') name.pop_back();
        rest = s.substr(p1 + 1);
    }

    Param *binpars[ROWS] = {};      // the BIN param of each visible row
    float binsetval[ROWS] = {};     // the value that was set last frame: a different value is the user's choice
    int binrow = -1;                // the cue whose BIN was clicked

    Param *get_binpar(int r) {
        if (!binpars[r]) {
            Param *p = new Param;
            p->name = "BIN";
            p->type = FF_TYPE_OPTION;
            p->value = 0.0f;
            p->deflt = 0.0f;
            p->range[0] = 0.0f;
            p->range[1] = 1.0f;
            p->sliding = false;
            p->options = {"(no bins)"};
            p->box->tooltiptitle = "Bin ";
            p->box->tooltip = "The bin that the block is loaded from. Leftclick lists all bins. ";
            binpars[r] = p;
            binsetval[r] = 0.0f;
        }
        return binpars[r];
    }

    // hit boxes, reused from frame to frame
    // (the tooltip is set before the box is asked whether the mouse is in it: that is when the tooltip box is copied)
    Boxx *hitbox(int &idx, float x, float y, float w, float h, const char *ttitle = "", const char *tt = "") {
        static std::vector<Boxx*> pool;
        if (idx >= (int)pool.size()) pool.push_back(new Boxx);
        Boxx *b = pool[idx++];
        b->tooltiptitle = ttitle;
        b->tooltip = tt;
        b->vtxcoords->x1 = x;
        b->vtxcoords->y1 = y;
        b->vtxcoords->w = w;
        b->vtxcoords->h = h;
        b->upvtxtoscr();
        return b;
    }

    void text(const std::string &s, float *col, float x, float y, float w) {
        // cut to the width of the cell
        size_t maxc = (size_t)std::max(1.0f, w / CHARW);
        if (s.size() > maxc) render_text(s.substr(0, maxc), col, x, y, TX, TY);
        else render_text(s, col, x, y, TX, TY);
    }

    float texty(float y, float h) {
        return y + h * 0.5f - 0.0075f;
    }

    // a button with a label; true when clicked
    bool button(int &pi, const std::string &label, float x, float y, float w, float h, float *outline,
                const char *ttitle = "", const char *tt = "") {
        Boxx *b = hitbox(pi, x, y, w, h, ttitle, tt);
        bool in = b->in();
        draw_box(outline, in ? grey : black, x, y, w, h, -1);
        text(label, white, x + 0.008f, texty(y, h), w - 0.012f);
        if (in && mainprogram->leftmouse) {
            mainprogram->leftmouse = false;
            return true;
        }
        return false;
    }

    // the frame rate that labels are written in: the one the source sends, else the chosen one
    void label_fps(double &fps, bool &drop) {
        TimecodeState st;
        if (mainprogram->timecode && mainprogram->timecode->get_state(st)) {
            fps = st.fps;
            drop = st.drop;
        }
        else timecode_fps_from_index((int)std::lround(mainprogram->tcfpsidx), fps, drop);
    }

    std::string secs_string(double secs) {
        double fps;
        bool drop;
        label_fps(fps, drop);
        return timecode_string(timecode_label(secs, fps, drop), drop);
    }

    // a short message at the bottom of the table (imports, shifts)
    std::string notice;
    int noticeframes = 0;

    // A time as typed in the table (hh:mm:ss:ff, also ss:ff), or as a DAW writes it: plain seconds (12.5) or
    // h:mm:ss.mmm.  False if it is none of those.
    bool parse_time(std::string s, double fps, bool drop, double &out) {
        while (!s.empty() && (s.back() == ' ' || s.back() == '\t')) s.pop_back();
        size_t b = 0;
        while (b < s.size() && (s[b] == ' ' || s[b] == '\t')) b++;
        s = s.substr(b);
        if (s.empty()) return false;
        if (s.find(':') == std::string::npos && s.find(';') == std::string::npos) {
            char *e = nullptr;
            const double v = strtod(s.c_str(), &e);
            if (e == s.c_str() || *e) return false;
            out = v;
            return v >= 0.0;
        }
        const size_t dot = s.rfind('.');
        if (dot != std::string::npos && s.size() - dot - 1 >= 3 && s.find(':', dot) == std::string::npos) {
            double t = 0.0;
            long cur = 0;
            bool incur = false;
            for (size_t i = 0; i <= dot; i++) {
                const char c = i < dot ? s[i] : ':';
                if (c >= '0' && c <= '9') { cur = cur * 10 + (c - '0'); incur = true; }
                else if (c == ':' || c == ';') {
                    if (!incur) return false;
                    t = t * 60.0 + (double)cur;
                    cur = 0;
                    incur = false;
                }
                else return false;
            }
            out = t + strtod(("0" + s.substr(dot)).c_str(), nullptr);
            return true;
        }
        TimecodeLabel l;
        if (!timecode_parse(s, l)) return false;
        out = timecode_secs(l, fps, drop);
        return true;
    }

    // moves every cue by a typed amount ("+00:00:01:00", "-2:00", "-1.5")
    void shift_all(const std::string &typed, double fps, bool drop) {
        TimecodeManager *tm = mainprogram->timecode;
        if (!tm) return;
        std::string s = typed;
        while (!s.empty() && s[0] == ' ') s.erase(0, 1);
        double sign = 1.0;
        if (!s.empty() && (s[0] == '-' || s[0] == '+')) {
            if (s[0] == '-') sign = -1.0;
            s.erase(0, 1);
        }
        double d;
        if (!parse_time(s, fps, drop, d)) {
            notice = "Shift: not a time";
            noticeframes = 300;
            return;
        }
        d *= sign;
        int moved = 0;
        for (TimecodeCue &c : tm->cues) {
            c.secs = std::max(0.0, c.secs + d);
            moved++;
        }
        TimecodeState st;
        const bool v = tm->get_state(st);
        for (TimecodeCue &c : tm->cues) c.fired = v && c.secs <= st.secs;
        tm->sort_cues();
        notice = "Shifted " + std::to_string(moved) + " cues";
        noticeframes = 300;
    }

    void commit_edit() {
        if (!ed.active) return;
        if (fe.on) {
            field_settle();
            ed.buf = field_string();
            fe.on = false;
        }
        ed.active = false;
        TimecodeManager *tm = mainprogram->timecode;
        if (!tm) return;
        double fps;
        bool drop;
        label_fps(fps, drop);
        if (ed.col == 5) {
            shift_all((shiftneg ? "-" : "+") + ed.buf, fps, drop);
            return;
        }
        if (ed.col == 3) {
            TimecodeLabel l;
            if (timecode_parse(ed.buf, l)) tm->internal_set(timecode_secs(l, fps, drop));
            return;
        }
        if (ed.row < 0 || ed.row >= (int)tm->cues.size()) return;
        TimecodeCue &c = tm->cues[ed.row];
        if (ed.col == 0) {
            TimecodeLabel l;
            if (timecode_parse(ed.buf, l)) {
                c.secs = timecode_secs(l, fps, drop);
                TimecodeState st;
                c.fired = tm->get_state(st) && c.secs <= st.secs;
                tm->sort_cues();
            }
        }
        else if (ed.col == 1) c.name = ed.buf;
        else if (ed.col == 2) c.target = ed.buf;
        else if (ed.col == 4) {
            // LOAD BIN: only the bank and block were typed; the bin stays
            std::string name, rest;
            loadbin_split(c.target, name, rest);
            c.target = name.empty() ? ed.buf : name + " " + ed.buf;
        }
    }

    void start_edit(int row, int col, const std::string &current) {
        commit_edit();   // (an edit that ended this frame by a click outside it)
        ed.active = true;
        ed.row = row;
        ed.col = col;
        ed.buf = current;
        mainprogram->renaming = EDIT_TCCELL;
        mainprogram->inputtext = current;
        mainprogram->cursorpos0 = mainprogram->inputtext.length();
        SDL_StartTextInput(mainprogram->mainwindow);
        mainprogram->leftmouse = false;
        mainprogram->recundo = false;
    }

    // starts the four-part editor on a time cell, the clicked part focused
    void start_field_edit(int row, int col, const std::string &current, int field) {
        start_edit(row, col, current);
        TimecodeLabel l;
        if (!timecode_parse(current, l)) l = TimecodeLabel();
        double fps;
        label_fps(fps, fe.drop);
        fe.nominal = timecode_nominal_fps(fps);
        fe.val[0] = l.h; fe.val[1] = l.m; fe.val[2] = l.s; fe.val[3] = l.f;
        fe.field = std::max(0, std::min(3, field));
        fe.digits.clear();
        fe.on = true;
    }

    void end_field_edit(bool commit) {
        if (commit) commit_edit();
        else {
            ed.active = false;
            fe.on = false;
        }
        mainprogram->renaming = EDIT_NONE;
        end_input();
    }

    // the colon between two numbers as two small boxes (the font's dots come out faint and uneven at this size);
    // drop frame's semicolon gets a taller lower dot
    void colon_dots(float x, float y, float h, float *col, bool semi) {
        const float dw = 0.0016f, dh = 0.0030f;
        const float cx = x + (0.0085f - dw) * 0.5f;
        const float mid = y + h * 0.5f;
        draw_box(col, col, cx, mid + 0.0030f, dw, dh, -1);
        draw_box(col, col, cx, mid - 0.0055f, dw, semi ? dh + 0.0025f : dh, -1);
    }

    // a time cell: the four numbers (with the colons between) are separate click targets; the one being edited is inverted
    void time_cell(int &pi, float x, float y, float w, float h, int row, int col, const std::string &str, float *tcol,
                   const char *ttitle = "", const char *tt = "") {
        const float PW = 0.0185f;       // a number (generous: the digits of the font are wider than CHARW suggests)
        const float STEP = PW + 0.0085f; // a number and its colon
        const float x0 = x + 0.006f;
        const bool editing = ed.active && ed.row == row && ed.col == col && fe.on;
        Boxx *cell = hitbox(pi, x, y, w, h, ttitle, tt);
        Boxx *parts[4];
        for (int k = 0; k < 4; k++) parts[k] = hitbox(pi, x0 + k * STEP - 0.003f, y, PW + 0.006f, h, ttitle, tt);
        cell->in();     // (registers the tooltip)
        if (editing) {
            draw_box(white, black, x, y, w, h, -1);
            for (int k = 0; k < 4; k++) {
                char b[8];
                snprintf(b, sizeof(b), "%02d", field_shown(k));
                if (k == fe.field) {
                    draw_box(white, white, x0 + k * STEP - 0.002f, y + 0.004f, PW + 0.004f, h - 0.008f, -1);
                    render_text(b, black, x0 + k * STEP, texty(y, h), TX, TY);
                }
                else render_text(b, white, x0 + k * STEP, texty(y, h), TX, TY);
                if (k < 3) colon_dots(x0 + k * STEP + PW, y, h, white, k == 2 && fe.drop);
            }
            if (mainprogram->leftmouse) {
                for (int k = 0; k < 4; k++) {
                    if (parts[k]->in()) {
                        field_focus(k);
                        mainprogram->leftmouse = false;
                        return;
                    }
                }
                // anywhere else: the edit ends (a click in the cell between the numbers does not)
                if (cell->in()) mainprogram->leftmouse = false;
                else end_field_edit(true);
            }
            return;
        }
        // the same slots as while editing, so the numbers do not move when the edit starts
        for (int k = 0; k < 4; k++) {
            size_t p = (size_t)k * 3;
            if (str.size() >= p + 2) render_text(str.substr(p, 2), tcol, x0 + k * STEP, texty(y, h), TX, TY);
            if (k < 3 && str.size() > p + 2) colon_dots(x0 + k * STEP + PW, y, h, tcol, str[p + 2] == ';');
        }
        if (mainprogram->leftmouse && actionpick < 0) {
            for (int k = 0; k < 4; k++) {
                if (parts[k]->in()) {
                    start_field_edit(row, col, str, k);
                    return;
                }
            }
            if (cell->in()) start_field_edit(row, col, str, 0);
        }
    }

    // the area that the table takes: from the left edge of deck A's monitor to the right edge of deck B's,
    // from the bottom to the top of the (live modus) mix monitor
    void table_region(float &x, float &y, float &w, float &h) {
        Boxx *a = mainprogram->deckmonitor[0];
        Boxx *b = mainprogram->deckmonitor[1];
        x = a->vtxcoords->x1;
        w = b->vtxcoords->x1 + b->vtxcoords->w - x;
        y = mainprogram->mainmonitor->vtxcoords->y1;
        h = 2.0f * mainprogram->monh;
    }

    std::string sanitize_field(const std::string &s) {
        std::string r = s;
        for (char &c : r) if (c == '\t' || c == '\n' || c == '\r') c = ' ';
        return r;
    }
}

bool timecode_isparam(Param *par) {
    // the BIN params of the LOAD BIN cues and the FPS param (they have no undo)
    if (par && (fpspar == par || outpar == par)) return true;
    for (int r = 0; r < ROWS; r++) {
        if (par && binpars[r] == par) return true;
    }
    return false;
}

// ---------------------------------------------------------------------------------------------------------
// cue list as CSV: export, and import of our own files or the marker list of a DAW

namespace {
    std::string csv_quote(const std::string &s) {
        if (s.find_first_of(",;\"\t\r\n") == std::string::npos) return s;
        std::string r = "\"";
        for (char c : s) {
            if (c == '"') r += "\"\"";
            else if (c == '\r' || c == '\n') r += ' ';
            else r += c;
        }
        return r + "\"";
    }

    std::vector<std::string> csv_split(const std::string &line, char delim) {
        std::vector<std::string> out;
        std::string cur;
        bool q = false;
        for (size_t i = 0; i < line.size(); i++) {
            const char c = line[i];
            if (q) {
                if (c == '"') {
                    if (i + 1 < line.size() && line[i + 1] == '"') { cur += '"'; i++; }
                    else q = false;
                }
                else cur += c;
            }
            else if (c == '"') q = true;
            else if (c == delim) { out.push_back(cur); cur.clear(); }
            else cur += c;
        }
        out.push_back(cur);
        return out;
    }

    std::string lower_trim(const std::string &s) {
        size_t a = 0, b = s.size();
        while (a < b && (s[a] == ' ' || s[a] == '\t')) a++;
        while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t')) b--;
        std::string r = s.substr(a, b - a);
        for (char &c : r) c = (char)tolower((unsigned char)c);
        return r;
    }
}

void timecode_export_csv(const std::string &pathin) {
    TimecodeManager *tm = mainprogram->timecode;
    if (!tm) return;
    std::string path = pathin;
    const size_t slash = path.find_last_of("/\\");
    if (path.find('.', slash == std::string::npos ? 0 : slash) == std::string::npos) path += ".csv";
    double fps;
    bool drop;
    label_fps(fps, drop);
    std::ofstream o(path);
    if (!o) {
        notice = "Export failed";
        noticeframes = 300;
        return;
    }
    o << "enabled,time,action,name,target,color\n";
    for (const TimecodeCue &c : tm->cues) {
        o << (c.enabled ? 1 : 0) << "," << timecode_string(timecode_label(c.secs, fps, drop), drop) << ","
          << csv_quote(cue_action_name(c.action)) << "," << csv_quote(c.name) << "," << csv_quote(c.target) << ","
          << cue_color_name(c.color) << "\n";
    }
    notice = "Exported " + std::to_string(tm->cues.size()) + " cues";
    noticeframes = 300;
}

namespace {
    bool edl_is_tc(const std::string &t) {
        if (t.size() != 11) return false;
        for (int i = 0; i < 11; i++) {
            if (i == 2 || i == 5) { if (t[i] != ':') return false; }
            else if (i == 8) { if (t[i] != ':' && t[i] != ';') return false; }
            else if (t[i] < '0' || t[i] > '9') return false;
        }
        return true;
    }

    std::vector<std::string> edl_tokens(const std::string &line) {
        std::vector<std::string> out;
        std::string cur;
        for (char c : line) {
            if (c == ' ' || c == '\t') { if (!cur.empty()) { out.push_back(cur); cur.clear(); } }
            else cur += c;
        }
        if (!cur.empty()) out.push_back(cur);
        return out;
    }

    // CMX3600 EDL: an edit (event) line ends with source in/out and record in/out timecodes; the cue goes at the
    // record in.  A "* FROM CLIP NAME:" or "* COMMENT:" line after it names the cue.  "* LOC:" lines (the markers
    // that Resolve and Premiere write) become cues too, with their colour.  The file carries no frame rate: the one
    // of the table is used, with the FCM line deciding drop frame at 29.97.
    int import_edl(const std::vector<std::string> &lines, double fps, bool drop) {
        TimecodeManager *tm = mainprogram->timecode;
        TimecodeState st;
        const bool valid = tm->get_state(st);
        const int nom = timecode_nominal_fps(fps);
        for (const std::string &l : lines) {
            std::string u = l;
            for (char &c : u) c = (char)toupper((unsigned char)c);
            if (u.rfind("FCM:", 0) == 0 && nom == 30) drop = u.find("NON") == std::string::npos && u.find("DROP") != std::string::npos;
        }
        int added = 0;
        std::vector<TimecodeCue> made;     // (the last one gets its name from the comment lines that follow its event)
        std::vector<bool> named;
        for (const std::string &l : lines) {
            if (l.empty()) continue;
            if (l[0] == '*') {
                std::string u = l;
                for (char &c : u) c = (char)toupper((unsigned char)c);
                const size_t loc = u.find("LOC:");
                if (loc != std::string::npos) {
                    const std::vector<std::string> t = edl_tokens(l.substr(loc + 4));
                    double secs;
                    if (!t.empty() && edl_is_tc(t[0]) && parse_time(t[0], fps, drop, secs)) {
                        TimecodeCue c;
                        c.secs = secs;
                        c.enabled = false;
                        std::string name;
                        for (size_t k = 2; k < t.size(); k++) name += (k > 2 ? " " : "") + t[k];
                        c.name = name.empty() ? "Marker" : name;
                        if (t.size() > 1) {
                            std::string col = t[1];
                            for (char &ch : col) ch = (char)tolower((unsigned char)ch);
                            static const char *keys[8] = {"red", "orange", "yellow", "green", "cyan", "blue", "purple", "pink"};
                            for (int k = 0; k < 8; k++) if (col.find(keys[k]) != std::string::npos) c.color = k + 1;
                            if (col.find("magenta") != std::string::npos) c.color = 7;
                        }
                        c.target = default_target(c.action);
                        made.push_back(c);
                        named.push_back(true);
                    }
                    continue;
                }
                const size_t pos = u.find("FROM CLIP NAME:");
                if (pos != std::string::npos && !made.empty() && !named.back()) {
                    std::string n = l.substr(pos + 15);
                    while (!n.empty() && n[0] == ' ') n.erase(0, 1);
                    if (!n.empty()) { made.back().name = n; named.back() = true; }
                }
                continue;
            }
            const std::vector<std::string> t = edl_tokens(l);
            // event line: number, reel, track, transition, (duration), src in, src out, rec in, rec out
            if (t.size() < 8 || !edl_is_tc(t[t.size() - 1]) || !edl_is_tc(t[t.size() - 2]) || !edl_is_tc(t[t.size() - 3])
                || !edl_is_tc(t[t.size() - 4])) continue;
            if (t[0].find_first_not_of("0123456789") != std::string::npos) continue;
            const std::string &track = t[2];
            if (track.find('V') == std::string::npos && track.find('B') == std::string::npos) continue;   // (audio only)
            double secs;
            if (!parse_time(t[t.size() - 2], fps, drop, secs)) continue;
            TimecodeCue c;
            c.secs = secs;
            c.enabled = false;
            c.name = t[1] == "BL" ? "Black" : t[1];
            c.target = default_target(c.action);
            made.push_back(c);
            named.push_back(false);
        }
        for (TimecodeCue &c : made) {
            c.fired = valid && c.secs <= st.secs;
            tm->cues.push_back(c);
            added++;
        }
        tm->sort_cues();
        return added;
    }
}

void timecode_import_csv(const std::string &path) {
    TimecodeManager *tm = mainprogram->timecode;
    if (!tm) return;
    std::ifstream in(path);
    if (!in) {
        notice = "Import failed";
        noticeframes = 300;
        return;
    }
    double fps;
    bool drop;
    label_fps(fps, drop);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (lines.empty() && line.size() >= 3 && (unsigned char)line[0] == 0xEF) line.erase(0, 3);    // (BOM)
        if (!line.empty()) lines.push_back(line);
    }
    if (lines.empty()) return;
    {
        // an EDL (by its extension, or its TITLE: line)
        std::string ext = path.size() >= 4 ? path.substr(path.size() - 4) : "";
        for (char &c : ext) c = (char)tolower((unsigned char)c);
        std::string first = lines[0].substr(0, 6);
        for (char &c : first) c = (char)toupper((unsigned char)c);
        if (ext == ".edl" || first == "TITLE:") {
            const int added = import_edl(lines, fps, drop);
            notice = "Imported " + std::to_string(added) + " cues from the EDL (switched off: give them an action)";
            noticeframes = 400;
            return;
        }
    }
    // the separator: whichever of , ; tab the first line has most of
    char delim = ',';
    {
        size_t best = 0;
        for (char d : {',', ';', '\t'}) {
            const size_t n = (size_t)std::count(lines[0].begin(), lines[0].end(), d);
            if (n > best) { best = n; delim = d; }
        }
    }
    // columns: from the header names (ours, or a DAW's marker list: Name / Start / Position / Description ...); a file
    // without a header is time, name
    int ctime = 0, cname = 1, caction = -1, ctarget = -1, cenabled = -1, ccolor = -1;
    size_t first = 0;
    {
        const std::vector<std::string> h = csv_split(lines[0], delim);
        bool header = true;
        double tmp;
        for (const std::string &f : h) {
            if (f.find_first_of("0123456789") != std::string::npos && parse_time(f, fps, drop, tmp)) header = false;
        }
        if (header) {
            first = 1;
            ctime = cname = -1;
            for (int k = 0; k < (int)h.size(); k++) {
                const std::string n = lower_trim(h[k]);
                if (ctime < 0 && (n == "time" || n == "timecode" || n == "start" || n == "position" || n == "tc" || n == "in")) ctime = k;
                else if (cname < 0 && (n == "name" || n == "marker" || n == "description" || n == "title" || n == "label" || n == "comment")) cname = k;
                else if (n == "action") caction = k;
                else if (n == "target") ctarget = k;
                else if (n == "enabled" || n == "on") cenabled = k;
                else if (n == "color" || n == "colour") ccolor = k;
            }
            if (ctime < 0) {
                notice = "Import: no time column";
                noticeframes = 300;
                return;
            }
        }
    }
    TimecodeState st;
    const bool valid = tm->get_state(st);
    int added = 0;
    for (size_t li = first; li < lines.size(); li++) {
        const std::vector<std::string> f = csv_split(lines[li], delim);
        if (ctime >= (int)f.size()) continue;
        double secs;
        if (!parse_time(f[ctime], fps, drop, secs)) continue;
        TimecodeCue c;
        c.secs = secs;
        c.name = (cname >= 0 && cname < (int)f.size()) ? f[cname] : "";
        if (c.name.empty()) c.name = "Cue " + std::to_string(tm->cues.size() + 1);
        bool haveaction = false;
        if (caction >= 0 && caction < (int)f.size()) {
            const std::string a = lower_trim(f[caction]);
            for (int k = 0; k < CUE_ACTION_COUNT; k++) {
                if (lower_trim(cue_action_name(k)) == a) { c.action = k; haveaction = true; }
            }
        }
        c.target = (ctarget >= 0 && ctarget < (int)f.size() && haveaction) ? f[ctarget] : "";
        if (c.target.empty()) c.target = default_target(c.action);
        // (a marker with no action: the cue is there, switched off until it has been given one)
        c.enabled = haveaction;
        if (cenabled >= 0 && cenabled < (int)f.size()) {
            const std::string e = lower_trim(f[cenabled]);
            c.enabled = (e == "1" || e == "true" || e == "yes" || e == "on");
        }
        if (ccolor >= 0 && ccolor < (int)f.size()) {
            const std::string cn = lower_trim(f[ccolor]);
            for (int k = 0; k < CUE_COLOR_COUNT; k++) {
                if (cn == cue_color_name(k) || (cn.size() == 1 && cn[0] == (char)('0' + k))) c.color = k;
            }
        }
        c.fired = valid && c.secs <= st.secs;
        tm->cues.push_back(c);
        added++;
    }
    tm->sort_cues();
    notice = "Imported " + std::to_string(added) + " cues" + (added && caction < 0 ? " (switched off: give them an action)" : "");
    noticeframes = 400;
}

// ---------------------------------------------------------------------------------------------------------
// enter / leave

void timecode_view_enter() {
    if (mainprogram->prevmodus) {
        mainprogram->timecodebut->value = 0;
        mainprogram->timecodebut->oldvalue = 0;
        return;
    }
    if (mainprogram->layerwarp) layerwarp_leave();
    if (mainprogram->projmap) projmap_leave();
    if (mainprogram->mapview) maskmap_leave();
    mainprogram->timecodeview = true;
    if (!mainprogram->timecoderesume) scrollrow = 0;    // (coming back to the table keeps the scroll position)
    mainprogram->timecoderesume = false;
    mainprogram->timecodebut->value = 1;
    mainprogram->timecodebut->oldvalue = 1;
    actionpick = -1;
    ed = CellEdit();
}

// ---------------------------------------------------------------------------------------------------------
// latency calibration: the outputs show a test pattern that flashes white at the start of every second of the
// timecode as the cues see it (offset and look-ahead included).  Film the output next to the thing that the timecode is
// to be in step with (a click, a light, the sender's own display), or read both with a light sensor, and move Offset
// until the two flashes coincide.

namespace {
    bool calibactive = false;
    bool calibwhite = false;
    GLuint calibtex[2] = {0, 0};

    GLuint make_solid_tex(unsigned char v) {
        unsigned char px[16 * 16 * 4];
        for (int i = 0; i < 16 * 16; i++) {
            px[i * 4] = px[i * 4 + 1] = px[i * 4 + 2] = v;
            px[i * 4 + 3] = 255;
        }
        GLuint t = 0;
        glGenTextures(1, &t);
        glBindTexture(GL_TEXTURE_2D, t);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA_INTERNAL, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        mainprogram->texsizemap[t] = {16, 16};
        return t;
    }
}

// the texture that the output windows show instead of the mix while calibrating (0: not calibrating)
GLuint timecode_calibration_tex() {
    if (!calibactive) return 0;
    if (!calibtex[0]) {
        calibtex[0] = make_solid_tex(0);
        calibtex[1] = make_solid_tex(255);
    }
    return calibtex[calibwhite ? 1 : 0];
}

void timecode_view_leave() {
    if (ed.active && mainprogram->renaming == EDIT_TCCELL) {
        mainprogram->renaming = EDIT_NONE;
        end_input();
    }
    commit_edit();
    mainprogram->timecodeview = false;
    mainprogram->timecodebut->value = 0;
    mainprogram->timecodebut->oldvalue = 0;
    actionpick = -1;
    outpick = 0;
    if (outmenu) outmenu->state = 0;
    if (prefsdirty && mainprogram->prefs) {
        mainprogram->prefs->save();
        prefsdirty = false;
    }
}

// ---------------------------------------------------------------------------------------------------------
// cues

namespace {
    // the layers of the live set (what the cues work on)
    void live_layers(std::vector<Layer *> &out) {
        for (int d = 0; d < 2; d++) {
            for (Layer *l : mainmix->layers[2 + d]) if (l) out.push_back(l);
        }
    }

    // Layer chase after the timecode has jumped (a locate, or the first position): the layers that should be playing at
    // the new position are the ones that the last SHELF cue before it launched, per destination layer (the last word
    // of the target, like B3).  Layers that are chased from a cue at or after the new position are
    // stopped; a layer that already follows the right cue only finds its new frame (Layer::chase_step()); the others are
    // launched again from their cue.  At most 8 launches, they run one after the other.
    void chase_rebuild(TimecodeManager *tm, double tc) {
        std::vector<Layer *> lays;
        live_layers(lays);
        for (Layer *l : lays) {
            if (l->chased && l->chasestart >= tc) {
                l->playbut->value = false;
                l->revbut->value = false;
                l->bouncebut->value = false;
                l->onhold = true;
                l->frame = 0.0f;
                l->chased = false;
                l->set_clones();
            }
        }
        std::map<std::string, int> latest;
        for (int i = 0; i < (int)tm->cues.size(); i++) {
            const TimecodeCue &c = tm->cues[i];
            if (!c.enabled || c.action != CUE_SHELF || c.secs >= tc || cue_osc_line(c.action, c.target).empty()) continue;
            // (the destination layer is the last word of the target)
            std::string key;
            const size_t lastw = c.target.find_last_not_of(' ');
            if (lastw != std::string::npos) {
                const size_t sp = c.target.rfind(' ', lastw);
                for (size_t k = (sp == std::string::npos ? 0 : sp + 1); k <= lastw; k++) {
                    key += (char)toupper((unsigned char)c.target[k]);
                }
            }
            latest[key] = i;    // (the cues are sorted: a later one replaces an earlier one)
        }
        std::vector<int> order;
        for (auto &kv : latest) order.push_back(kv.second);
        std::sort(order.begin(), order.end(), [tm](int a, int b) { return tm->cues[a].secs < tm->cues[b].secs; });
        // the destination layer of a target or key like "B3": deck letter and layer number
        auto dest_of = [](const std::string &key, int &deck, int &pos) {
            deck = -1;
            pos = -1;
            if (key.size() >= 2 && (key[0] == 'A' || key[0] == 'B') && isdigit((unsigned char)key[1])) {
                deck = key[0] == 'B' ? 1 : 0;
                pos = std::atoi(key.c_str() + 1) - 1;
            }
        };
        int launched = 0;
        for (int i : order) {
            const TimecodeCue &c = tm->cues[i];
            // the key of this cue's destination (as above)
            std::string key;
            const size_t lastw = c.target.find_last_not_of(' ');
            if (lastw != std::string::npos) {
                const size_t sp = c.target.rfind(' ', lastw);
                for (size_t k = (sp == std::string::npos ? 0 : sp + 1); k <= lastw; k++) key += (char)toupper((unsigned char)c.target[k]);
            }
            int deck, pos;
            dest_of(key, deck, pos);
            // the CLIP NEXT cues of that layer between this cue and the new position: they advance its queue
            std::vector<double> nexts;
            for (const TimecodeCue &n : tm->cues) {
                if (!n.enabled || n.action != CUE_CLIPNEXT || n.secs <= c.secs || n.secs >= tc) continue;
                std::string nk;
                size_t b = n.target.find_first_not_of(' ');
                while (b != std::string::npos && b < n.target.size() && n.target[b] != ' ') nk += (char)toupper((unsigned char)n.target[b++]);
                if (nk == key) nexts.push_back(n.secs);
            }
            // a layer that this cue launched and that is on the clip of the new position already is left alone
            Layer *run = nullptr;
            for (Layer *l : lays) {
                if (l->chased && std::fabs(l->chasecue - c.secs) < 1e-6 && l->deck == deck && l->pos == pos) run = l;
            }
            bool running = false;
            if (run) {
                int j = 0, m = 0;
                double tn = 0.0;
                if (mainprogram->chase_queue_sim(run, c.secs, nexts, tc, j, tn, m)) {
                    running = m == run->chasem && std::fabs(tn - run->chasestart) < 1e-3;
                }
                else running = std::fabs(run->chasestart - c.secs) < 1e-6;
            }
            else {
                for (Layer *l : lays) {
                    if (l->chased && std::fabs(l->chasestart - c.secs) < 1e-6) running = true;
                }
            }
            if (running) continue;
            if (launched++ >= 8) break;
            // (the layer comes back with the first clip of its queue: the record brings the queue to the right clip)
            if (deck >= 0) mainprogram->chase_catchup_add(deck, pos, c.secs, tc, nexts);
            osc_inject(cue_osc_line(c.action, c.target), c.secs);
        }
    }
}

void timecode_update() {
    Program *mp = mainprogram;
    TimecodeManager *tm = mp->timecode;
    if (!tm) {
        mp->tcchasevalid = false;
        mp->process_deferred_state(true);
        return;
    }
    timecode_undo_poll();       // (an edit of the cue list becomes an undo step)
    const bool live = !mp->prevmodus && !mainmix->retargeting && tm->source() != TC_OFF;
    // layer chase: chased layers read this position in Layer::progress()
    TimecodeState st;
    const bool chase = live && mp->tcchase > 0.5f && tm->get_state(st);
    mp->tcchasevalid = chase;
    if (chase) mp->tcchasesecs = st.secs;

    // half the display frame period (smoothed): a cue fires when it is nearer than that, so the nearest display frame
    // gets it instead of always the one after
    {
        static std::chrono::steady_clock::time_point last;
        static double framedur = 1.0 / 60.0;
        const auto now = std::chrono::steady_clock::now();
        const double dt = std::chrono::duration<double>(now - last).count();
        last = now;
        if (dt > 0.002 && dt < 0.1) framedur = framedur * 0.9 + dt * 0.1;
        mp->tcframelead = 0.5 * std::min(0.05, std::max(0.004, framedur));
        // the position is read when the frame starts; what it shows is on the display about a frame period later
        tm->set_lookahead(std::min(0.05, std::max(0.004, framedur)));
    }
    // pre-roll: a SHELF cue for a video or layer file fires this long before its time, loads and decodes its first frame
    // meanwhile (the old layers stay on screen), and appears at its time (Layer::chasehold, the swap loop)
    const double preroll = chase ? std::max(0.0f, mp->tcpreroll) / 1000.0 : 0.0;
    auto lead = [mp, preroll](const TimecodeCue &c) -> double {
        // CLIP NEXT: the next clip of the queue loads meanwhile (the old clip keeps playing until the swap)
        if (preroll > 0.0 && c.action == CUE_CLIPNEXT) return preroll;
        if (preroll > 0.0 && c.action == CUE_SHELF) {
            char sh = 'A';
            int elem = 0, bank = 0;
            if (sscanf(cue_osc_line(c.action, c.target).c_str(), "/shelf/%c/trigger %d %d", &sh, &elem, &bank) == 3 &&
                (sh == 'A' || sh == 'B') && elem >= 1 && elem <= 16 && bank >= 1 && bank <= 4) {
                ShelfElement *se = mp->shelves[sh == 'B'][bank - 1]->elements[elem - 1];
                if (se && se->path != "" && (se->type == ELEM_FILE || se->type == ELEM_LAYER || se->type == ELEM_DECK || se->type == ELEM_MIX)) return preroll;
            }
        }
        return mp->tcframelead;
    };

    std::vector<int> due = tm->due_cues(live, lead);
    const bool jumped = tm->take_jump();
    mp->process_deferred_state(jumped || !chase);
    if (jumped && chase) {
        // a jump ends the wait of pre-rolled layers (their cue time is not where the timecode goes)
        for (int k = 0; k < 4; k++) {
            for (const std::vector<Layer *> &lv : mainmix->swapmap[k]) {
                if (lv.size() > 1 && lv[1]) lv[1]->chasehold = false;
            }
        }
        chase_rebuild(tm, st.secs);
    }
    for (int i : due) {
        if (i < 0 || i >= (int)tm->cues.size()) continue;
        const TimecodeCue &c = tm->cues[i];
        mp->tcdiagcue = (float)((st.secs - (c.secs - lead(c))) * 1000.0);
        std::string line = cue_osc_line(c.action, c.target);
        // (a SHELF cue makes the layers that it launches follow the timecode from the time of the cue)
        if (!line.empty()) osc_inject(line, chase && (c.action == CUE_SHELF || c.action == CUE_CLIPNEXT) ? c.secs : CHASE_NONE);
    }
    mp->process_chase_clipnext();
    mp->process_deferred_state(false);
    // diagnostics: how far the decoders of the chased layers are from the frame that the timecode asks for
    {
        float lag = 0.0f;
        if (chase) {
            std::vector<Layer *> lays;
            live_layers(lays);
            for (Layer *l : lays) {
                if (l->chased && !l->chasehold && l->prevframe.load() >= 0) {
                    lag = std::max(lag, std::fabs((float)l->prevframe.load() - l->frame.load()));
                }
            }
        }
        mp->tcdiaglag = mp->tcdiaglag * 0.9f + lag * 0.1f;
    }
    // latency calibration: white during the first quarter second of every timecode second (the position as the cues
    // read it, offset and look-ahead included)
    calibwhite = false;
    if (calibactive && !mp->prevmodus) {
        TimecodeState cs;
        if (tm->get_state(cs) && cs.valid) {
            const TimecodeLabel l = timecode_label(cs.secs, cs.fps, cs.drop);
            calibwhite = l.f < std::max(2, timecode_nominal_fps(cs.fps) / 4);
        }
    }
}

// ---------------------------------------------------------------------------------------------------------
// undo / redo of the cue list: snapshots of the whole list; every edit of the list (typing, colours, DEL, DUP, import,
// shift ...) is found by comparing it with the snapshot that is current, and becomes a step in the program's
// undo list (Program::register_cue_undo), between the steps of params, buttons and curve edits.  Project loads
// (also the undo of a project state) only move the baseline: they are not steps of their own.

namespace {
    std::vector<std::vector<TimecodeCue>> cuestates;
    int cuecur = -1;

    bool cues_equal(const std::vector<TimecodeCue> &a, const std::vector<TimecodeCue> &b) {
        if (a.size() != b.size()) return false;
        for (size_t i = 0; i < a.size(); i++) {
            if (a[i].enabled != b[i].enabled || a[i].secs != b[i].secs || a[i].action != b[i].action ||
                a[i].color != b[i].color || a[i].name != b[i].name || a[i].target != b[i].target) return false;
        }
        return true;
    }

    // the list as it is now is the baseline (no undo step)
    void cue_rebase() {
        TimecodeManager *tm = mainprogram->timecode;
        if (!tm) return;
        if (cuecur >= 0 && cuecur < (int)cuestates.size() && cues_equal(tm->cues, cuestates[cuecur])) return;
        cuestates.push_back(tm->cues);
        cuecur = (int)cuestates.size() - 1;
    }
}

void timecode_undo_poll() {
    TimecodeManager *tm = mainprogram->timecode;
    if (!tm) return;
    if (cuecur < 0 || cuecur >= (int)cuestates.size()) {
        cue_rebase();
        return;
    }
    if (cues_equal(tm->cues, cuestates[cuecur])) return;
    cuestates.push_back(tm->cues);
    const int ns = (int)cuestates.size() - 1;
    mainprogram->register_cue_undo(cuecur, ns);
    cuecur = ns;
}

void timecode_undo_restore(int state) {
    TimecodeManager *tm = mainprogram->timecode;
    if (!tm || state < 0 || state >= (int)cuestates.size()) return;
    // (a cell that is being typed in is dropped)
    if (ed.active) {
        if (mainprogram->renaming == EDIT_TCCELL) {
            mainprogram->renaming = EDIT_NONE;
            end_input();
        }
        ed.active = false;
    }
    actionpick = -1;
    colorpick = -1;
    binrow = -1;
    tm->cues = cuestates[state];
    cuecur = state;
    TimecodeState st;
    const bool v = tm->get_state(st);
    for (TimecodeCue &c : tm->cues) c.fired = v && c.secs <= st.secs;
}

void timecode_project_reset() {
    if (mainprogram->timecode) {
        mainprogram->timecode->cues.clear();
        mainprogram->timecode->due_cues(false);
        cue_rebase();
    }
    scrollrow = 0;
    actionpick = -1;
    ed = CellEdit();
}

void timecode_project_save(std::ostream &o) {
    TimecodeManager *tm = mainprogram->timecode;
    if (!tm) return;
    o << "TIMECODECUES\n" << tm->cues.size() << "\n";
    for (const TimecodeCue &c : tm->cues) {
        char buf[48];
        snprintf(buf, sizeof(buf), "%.6f", c.secs);
        o << (c.enabled ? 1 : 0) << "\t" << buf << "\t" << c.action << "\t" << sanitize_field(c.name) << "\t"
          << sanitize_field(c.target) << "\t" << c.color << "\n";
    }
}

bool timecode_project_tag(const std::string &tag, std::istream &is) {
    if (tag != "TIMECODECUES") return false;
    std::string line;
    safegetline(is, line);
    int n = atoi(line.c_str());
    TimecodeManager *tm = mainprogram->timecode;
    if (tm) tm->cues.clear();
    for (int i = 0; i < n; i++) {
        if (!safegetline(is, line)) break;
        if (!tm) continue;
        std::vector<std::string> parts;
        size_t st = 0;
        while (true) {
            size_t p = line.find('\t', st);
            if (p == std::string::npos) {
                parts.push_back(line.substr(st));
                break;
            }
            parts.push_back(line.substr(st, p - st));
            st = p + 1;
        }
        if (parts.size() < 5) continue;
        TimecodeCue c;
        c.enabled = atoi(parts[0].c_str()) != 0;
        c.secs = atof(parts[1].c_str());
        c.action = std::min(std::max(atoi(parts[2].c_str()), 0), (int)CUE_ACTION_COUNT - 1);
        c.name = parts[3];
        c.target = parts[4];
        if (parts.size() >= 6) c.color = std::min(std::max(atoi(parts[5].c_str()), 0), CUE_COLOR_COUNT - 1);
        tm->cues.push_back(c);
    }
    if (tm) {
        tm->sort_cues();
        tm->due_cues(false);    // (the position is looked at again: nothing fires on the first one)
        cue_rebase();           // (a project load is not an undo step of the cue list)
    }
    return true;
}

// ---------------------------------------------------------------------------------------------------------
// mini monitors under the loopstation

void timecode_view_minimonitors(float xs, float ys) {
    // The deck and mix monitors as they are (deck monitors half the width and height of the mix monitor, either
    // side of it), smaller: as wide as the loopstation block, right under its last line.
    if (!lpc || lpc->elements.empty()) return;
    LoopStationElement *e0 = lpc->elements[0];
    const float left = e0->recbut->box->vtxcoords->x1;
    const float right = e0->colbox->vtxcoords->x1 + e0->colbox->vtxcoords->w;

    Boxx *da = mainprogram->deckmonitor[0];
    Boxx *db = mainprogram->deckmonitor[1];
    const float ox = da->vtxcoords->x1;
    const float fullw = db->vtxcoords->x1 + db->vtxcoords->w - ox;
    const float oy = mainprogram->mainmonitor->vtxcoords->y1;
    const float fullh = 2.0f * mainprogram->monh;
    const float s = (right - left) / fullw;

    const float rowsbottom = 0.4f - 0.075f * 7.0f;    // y of the last of the 8 visible loopstation lines
    const float bottom = rowsbottom - 0.02f - fullh * s;

    struct Mini {
        float x1, y1, w, h;
        int node;
        float inset;
    };
    // (the mix monitor in live modus: x1 -0.3, width 0.6, as display_mix() sets it)
    const Mini minis[3] = {
        {-0.3f, oy, 0.6f, fullh, 2, 2.0f},
        {da->vtxcoords->x1, da->vtxcoords->y1, da->vtxcoords->w, da->vtxcoords->h, 0, 1.0f},
        {db->vtxcoords->x1, db->vtxcoords->y1, db->vtxcoords->w, db->vtxcoords->h, 1, 1.0f},
    };
    for (const Mini &m : minis) {
        MixNode *node = (MixNode*)mainprogram->nodesmain->mixnodes[1][m.node];
        const float nx = left + (m.x1 - ox) * s;
        const float ny = bottom + (m.y1 - oy) * s;
        draw_box(darkred1, black, nx + xs * m.inset * s, ny + ys * m.inset * s,
                 m.w * s - xs * 2.0f * m.inset * s, m.h * s - ys * 2.0f * m.inset * s, node->mixtex);
    }
}

// ---------------------------------------------------------------------------------------------------------
// the table

void timecode_view_ui() {
    TimecodeManager *tm = mainprogram->timecode;
    if (!tm) {
        timecode_view_leave();
        return;
    }
    int pi = 0;     // hit boxes used so far
    float rx, ry, rw, rh;
    table_region(rx, ry, rw, rh);
    const float top = ry + rh;
    const float lx = rx + 0.01f;

    // the state of the clock
    TimecodeState st;
    const bool valid = tm->get_state(st);
    const int source = tm->source();

    // finish an edit that ended (Enter, or a click outside the cell)
    if (ed.active && mainprogram->renaming != EDIT_TCCELL) commit_edit();
    if (ed.active) ed.buf = mainprogram->inputtext;

    draw_box(grey, black, rx, ry, rw, rh, -1);
    Boxx *regionbox = hitbox(pi, rx, ry, rw, rh);
    const bool inregion = regionbox->in();

    // the output foldout menu (standard menu): the choice sets the device / port
    if (outpick) {
        if (!outmenu || outmenu->state != 2) {
            outpick = 0;   // closed without a choice
        }
        else {
            const int chosen = mainprogram->handle_menu(outmenu);
            if (chosen >= 0 && chosen < (int)outpicknames.size()) {
                if (outpick == 3) {
                    if (chosen != (int)std::lround(mainprogram->tcsource)) {
                        mainprogram->tcsource = (float)chosen;
                        timecode_apply_prefs();
                        prefsdirty = true;
                        tm->due_cues(false);
                    }
                }
                else {
                    std::string &dest = outpick == 1 ? mainprogram->ltcoutdevice : mainprogram->mtcoutport;
                    if (dest != outpicknames[chosen]) {
                        dest = outpicknames[chosen];
                        timecode_apply_prefs();
                        prefsdirty = true;
                    }
                }
                outpick = 0;
                mainprogram->menuchosen = false;
                mainprogram->menuactivation = 0;
                mainprogram->menuresults.clear();
            }
        }
    }

    // ---- header, row 1: source, frame rate, offset, position, internal transport, X
    const float hy = top - 0.055f;
    const float hh = 0.045f;
    float x = lx;
    if (button(pi, std::string("SOURCE: ") + SOURCENAMES[std::min(std::max(source, 0), 4)], x, hy, 0.19f, hh, white,
               "Timecode source ",
               "Where the timecode comes from: OFF, MIDI (MTC), ART-NET, LTC (audio input) or the INTERNAL clock. Leftclick opens a menu with the sources. ")) {
        open_outpick(3);
    }
    x += 0.20f;
    double fps;
    bool drop;
    label_fps(fps, drop);
    {
        // frame rate: an option Param (a source that sends its own rate shows that rate; the chosen one is for the
        // internal clock and for when there is no signal)
        if (!fpspar) {
            fpspar = new Param;
            fpspar->name = "FPS";
            fpspar->type = FF_TYPE_OPTION;
            fpspar->value = 0.0f;
            fpspar->deflt = 2.0f;
            fpspar->range[0] = 0.0f;
            fpspar->range[1] = (float)(TIMECODE_FPS_COUNT - 1);
            fpspar->sliding = false;
            for (int k = 0; k < TIMECODE_FPS_COUNT; k++) fpspar->options.push_back(timecode_fps_name(k));
            fpspar->box->tooltiptitle = "Frame rate ";
            fpspar->box->tooltip = "The frame rate that the timecode counts at: for the internal clock, and for the labels while there is no signal. A source that sends its own rate (MTC, Art-Net, LTC) shows that rate. Leftclick lists the rates. ";
        }
        const int idx = (int)std::lround(mainprogram->tcfpsidx);
        // (a value that differs from the one set last frame was chosen in the menu)
        if (fpspar->value != fpssetval) {
            const int k = (int)(fpspar->value + 0.5f);
            if (k >= 0 && k < TIMECODE_FPS_COUNT && k != idx) {
                mainprogram->tcfpsidx = (float)k;
                timecode_apply_prefs();
                prefsdirty = true;
            }
        }
        const int shown = (valid && source != TC_INTERNAL) ? timecode_fps_index(fps, drop)
                                                            : (int)std::lround(mainprogram->tcfpsidx);
        fpspar->value = (float)shown;
        fpssetval = fpspar->value;
        fpspar->box->vtxcoords->x1 = x;
        fpspar->box->vtxcoords->y1 = hy;
        fpspar->box->vtxcoords->w = 0.12f;
        fpspar->box->vtxcoords->h = hh;
        fpspar->box->upvtxtoscr();
        fpspar->handle();
    }
    x += 0.13f;

    // offset slider
    if (!offpar) {
        offpar = new Param;
        offpar->name = "Offset (ms)";
        offpar->value = mainprogram->tcoffset;
        offpar->deflt = 0.0f;
        offpar->range[0] = -500.0f;
        offpar->range[1] = 500.0f;
        offpar->sliding = true;
        offpar->box->lcolor[0] = 0.4f;
        offpar->box->lcolor[1] = 0.4f;
        offpar->box->lcolor[2] = 0.4f;
        offpar->box->lcolor[3] = 1.0f;
        offpar->box->tooltiptitle = "Timecode offset (ms) ";
        offpar->box->tooltip = "Moves the timecode position (milliseconds) to compensate for latency. Positive values make cues come earlier. The frame period between reading the position and the frame being on screen is added automatically; use this for what comes after the screen output (projector, LED processor) and for the delay of the timecode source. Leftdrag sets value. Doubleclick allows numeric entry. ";
    }
    offpar->box->vtxcoords->x1 = x;
    offpar->box->vtxcoords->y1 = hy;
    offpar->box->vtxcoords->w = 0.22f;
    offpar->box->vtxcoords->h = hh;
    offpar->box->upvtxtoscr();
    offpar->handle();
    if (offpar->value != mainprogram->tcoffset) {
        mainprogram->tcoffset = offpar->value;
        tm->set_offset_ms(mainprogram->tcoffset);
        prefsdirty = true;
    }
    x += 0.23f;

    // the position (the internal clock's can be typed)
    {
        std::string tcstr = valid ? timecode_string(timecode_label(st.secs, st.fps, st.drop), st.drop) : "--:--:--:--";
        const float tw = 0.19f;
        if (source == TC_INTERNAL) {
            if (!(ed.active && ed.col == 3)) draw_box(valid && st.running ? green : white, black, x, hy, tw, hh, -1);
            time_cell(pi, x, hy, tw, hh, -1, 3, tcstr, white, "Internal clock position ",
                      "The position of the internal clock. Leftclick hours, minutes, seconds or frames and type digits (two digits move on to the next part, Tab also moves, Return sets the position, Esc cancels). ");
        }
        else button(pi, tcstr, x, hy, tw, hh, valid && st.running ? green : white, "Timecode position ",
                    "The position of the timecode source, as the cues read it (offset applied). Green: running. ");
    }
    x += 0.20f;

    if (source != TC_INTERNAL && source != TC_OFF) {
        // freewheel: how long the clock runs on when the packets stop (a dropout); 0 holds the position at once
        if (!fwpar) {
            fwpar = new Param;
            fwpar->name = "Freewheel (ms)";
            fwpar->value = mainprogram->tcfreewheel;
            fwpar->deflt = 0.0f;
            fwpar->range[0] = 0.0f;
            fwpar->range[1] = 5000.0f;
            fwpar->sliding = true;
            fwpar->box->lcolor[0] = 0.4f;
            fwpar->box->lcolor[1] = 0.4f;
            fwpar->box->lcolor[2] = 0.4f;
            fwpar->box->lcolor[3] = 1.0f;
            fwpar->box->tooltiptitle = "Freewheel (ms) ";
            fwpar->box->tooltip = "When an external source stops sending, the clock keeps running this long, so a short dropout (cable, audio glitch) does not stop the show. A sender that is really stopped looks the same, so the clock then runs on past the stop for this long. 0: the position holds as soon as the packets stop. Leftdrag sets value. Doubleclick allows numeric entry. ";
        }
        fwpar->box->vtxcoords->x1 = x;
        fwpar->box->vtxcoords->y1 = hy;
        fwpar->box->vtxcoords->w = 0.17f;
        fwpar->box->vtxcoords->h = hh;
        fwpar->box->upvtxtoscr();
        fwpar->handle();
        if (fwpar->value != mainprogram->tcfreewheel) {
            mainprogram->tcfreewheel = fwpar->value;
            tm->set_freewheel_ms(mainprogram->tcfreewheel);
            prefsdirty = true;
        }
    }
    if (source == TC_INTERNAL) {
        if (button(pi, valid && st.running ? "STOP" : "PLAY", x, hy, 0.075f, hh, white,
                   "Internal clock play/stop ", "Starts or stops the internal clock. ")) {
            if (valid && st.running) tm->internal_stop();
            else tm->internal_play();
        }
        x += 0.085f;
        if (button(pi, "RESET", x, hy, 0.075f, hh, white,
                   "Internal clock reset ", "Stops the internal clock and sets it back to 00:00:00:00. ")) {
            tm->internal_reset();
            tm->due_cues(false);
        }
    }

    // timecode out: sends the position on (the internal clock as master, or any source re-sent in another format)
    {
        if (!outpar) {
            outpar = new Param;
            outpar->name = "OUT";
            outpar->type = FF_TYPE_OPTION;
            outpar->value = 0.0f;
            outpar->deflt = 0.0f;
            outpar->range[0] = 0.0f;
            outpar->range[1] = (float)(TCO_COUNT - 1);
            outpar->sliding = false;
            outpar->box->tooltiptitle = "Timecode out ";
            outpar->box->tooltip = "Sends the timecode position on: MTC goes to a MIDI output port, Art-Net is broadcast on UDP 6454, LTC plays on an audio output device. Choosing MTC or LTC opens a menu to choose the port or device from; rightclick this box to open it again. 24, 25, 29.97 DF and 30 fps only (23.976 goes out as 24). With the internal clock as source this is the master; any other source is re-sent as it arrives, without the offset. Leftclick lists the outputs. ";
        }
        const int cur = (int)std::lround(mainprogram->tcoutmode);
        // (a value that differs from the one set last frame was chosen in the menu)
        if (outpar->value != outsetval) {
            const int k = (int)(outpar->value + 0.5f);
            if (k >= 0 && k < TCO_COUNT && k != cur) {
                mainprogram->tcoutmode = (float)k;
                timecode_apply_prefs();
                prefsdirty = true;
                // MTC / LTC: a list of the MIDI output ports / audio output devices opens to choose from
                if (k == TCO_LTC) open_outpick(1);
                else if (k == TCO_MTC) open_outpick(2);
                else outpick = 0;
            }
        }
        const bool ok = tm->output_ok();
        outpar->options = {"off", std::string("MTC") + (ok || cur != TCO_MTC ? "" : " (no port)"),
                           std::string("Art-Net") + (ok || cur != TCO_ARTNET ? "" : " (failed)"), "LTC"};
        outpar->value = (float)std::min(std::max((int)std::lround(mainprogram->tcoutmode), 0), TCO_COUNT - 1);
        outsetval = outpar->value;
        outpar->box->vtxcoords->x1 = rx + rw - 0.01f - 0.07f - 0.005f - 0.17f;
        outpar->box->vtxcoords->y1 = hy;
        outpar->box->vtxcoords->w = 0.165f;
        outpar->box->vtxcoords->h = hh;
        outpar->box->upvtxtoscr();
        // a rightclick on OUT opens the list again (to change the port / device)
        if ((cur == TCO_LTC || cur == TCO_MTC) && mainprogram->rightmouse && outpar->box->in()) {
            open_outpick(cur == TCO_LTC ? 1 : 2);
            mainprogram->rightmouse = false;
        }
        outpar->handle();
    }

    // close
    if (button(pi, "CLOSE", rx + rw - 0.01f - 0.07f, hy, 0.07f, hh, white,
               "Close the timecode table ", "Closes the table and brings back the monitors. The cues keep firing. ")) {
        calibactive = false;
        timecode_view_leave();
        return;
    }

    // ---- header, row 2: add a cue, status
    const float h2y = top - 0.105f;
    std::string diagstr;        // the timing diagnostics, when they are shown in the bottom strip (LTC)
    if (button(pi, "+ CUE", lx, h2y, 0.1f, hh, white,
               "Add cue ", "Adds a cue at the current timecode position (or behind the last cue when there is no position). ")) {
        TimecodeCue c;
        c.secs = valid ? st.secs : (tm->cues.empty() ? 0.0 : tm->cues.back().secs + 1.0);
        c.fired = valid;
        c.name = "Cue " + std::to_string(tm->cues.size() + 1);
        c.target = default_target(c.action);
        const std::string nm = c.name;
        const double cs = c.secs;
        tm->cues.push_back(c);
        tm->sort_cues();
        for (int i = 0; i < (int)tm->cues.size(); i++) {
            if (tm->cues[i].secs == cs && tm->cues[i].name == nm) {
                scrollrow = std::max(0, i - ROWS + 1);
                break;
            }
        }
    }
    // layer chase: layers that SHELF cues launch follow the timecode (also after a locate)
    if (button(pi, mainprogram->tcchase > 0.5f ? "CHASE: ON" : "CHASE: OFF", lx + 0.11f, h2y, 0.14f, hh, mainprogram->tcchase > 0.5f ? green : white,
               "Layer chase ", "When on, layers launched by SHELF cues (and CLIP NEXT cues) follow the timecode: they play the frame that belongs to the position, also after a locate. Leftclick toggles. ")) {
        mainprogram->tcchase = mainprogram->tcchase > 0.5f ? 0.0f : 1.0f;
        prefsdirty = true;
        tm->due_cues(false);    // (looks at the position again: a locate then rebuilds the layers)
    }
    // pre-roll: how long before their time SHELF cues for video and layer files start loading (0: off; needs CHASE)
    if (!prerollpar) {
        prerollpar = new Param;
        prerollpar->name = "Pre-roll (ms)";
        prerollpar->value = mainprogram->tcpreroll;
        prerollpar->deflt = 500.0f;
        prerollpar->range[0] = 0.0f;
        prerollpar->range[1] = 3000.0f;
        prerollpar->sliding = true;
        prerollpar->box->lcolor[0] = 0.4f;
        prerollpar->box->lcolor[1] = 0.4f;
        prerollpar->box->lcolor[2] = 0.4f;
        prerollpar->box->lcolor[3] = 1.0f;
        prerollpar->box->tooltiptitle = "Pre-roll (ms) ";
        prerollpar->box->tooltip = "With CHASE on, a SHELF cue for a video, layer, deck or mix (and a CLIP NEXT cue) starts loading this long before its time. The file is decoded and waits while the old layers stay on screen, and appears exactly at the cue time. Choose it longer than the slowest file takes to load; 0 is off (the file then appears a little after its time). While a file waits, other launches and layer swaps wait too. Leftdrag sets value. Doubleclick allows numeric entry. ";
    }
    prerollpar->box->vtxcoords->x1 = lx + 0.26f;
    prerollpar->box->vtxcoords->y1 = h2y;
    prerollpar->box->vtxcoords->w = 0.2f;
    prerollpar->box->vtxcoords->h = hh;
    prerollpar->box->upvtxtoscr();
    prerollpar->handle();
    if (prerollpar->value != mainprogram->tcpreroll) {
        mainprogram->tcpreroll = prerollpar->value;
        prefsdirty = true;
    }
    {
        std::string status;
        float *scol = lightgrey;
        if (source == TC_OFF) status = "No timecode source: choose one";
        else if (!valid) { status = "NO SIGNAL"; scol = red; }
        else if (st.freewheeling) { status = "FREEWHEEL"; scol = yellow; }
        else if (st.running && source != TC_INTERNAL) {
            // locked after a few steady packets (a source that is still settling, or jumping about, is not)
            status = st.locked ? "LOCKED" : "LOCKING";
            scol = st.locked ? green : yellow;
        }
        else if (st.running) { status = "RUNNING"; scol = green; }
        else status = "STOPPED";
        int nxt = valid ? tm->next_cue(st.secs) : -1;
        if (nxt >= 0) status += "    NEXT: " + tm->cues[nxt].name + " at " + secs_string(tm->cues[nxt].secs);
        // latency calibration: the outputs flash white at the start of every timecode second; move Offset until the
        // flash coincides with whatever the timecode has to be in step with
        if (button(pi, calibactive ? "CALIBRATING" : "CALIBRATE", lx + 0.475f, h2y, 0.115f, hh, calibactive ? green : white,
                   "Latency calibration ", "The output windows flash white at the start of every timecode second. Compare the flash with whatever the timecode has to be in step with (a click, a light, a camera film of both) and move the Offset until they coincide. Leftclick toggles; CLOSE also ends it. ")) {
            calibactive = !calibactive;
        }
        if (calibactive) {
            status = "Flash = TC second: set Offset";
            scol = yellow;
        }
        text(status, scol, lx + 0.595f, texty(h2y, hh), 0.235f);
        hitbox(pi, lx + 0.595f, h2y, 0.235f, hh, "Timecode status ",
               "NO SIGNAL: nothing arrives. LOCKING / LOCKED: an external source that is settling / steady. FREEWHEEL: the packets stopped but the clock runs on. RUNNING / STOPPED: the position advances or not. NEXT: the cue that fires next. ")->in();
        // timing diagnostics: source jitter, how late the last cue fired against its scheduled time, and how far the
        // chased layers' decoders are behind the frame that the timecode asks for
        // the audio input for LTC: the beat detection's device (shared) or any other recording device
        float diagx = lx + 0.83f;
        if (source == TC_LTC) {
            if (!ltcpar) {
                ltcpar = new Param;
                ltcpar->name = "LTC IN";
                ltcpar->type = FF_TYPE_OPTION;
                ltcpar->value = 0.0f;
                ltcpar->deflt = 0.0f;
                ltcpar->range[0] = 0.0f;
                ltcpar->sliding = false;
                ltcpar->box->tooltiptitle = "LTC input device ";
                ltcpar->box->tooltip = "The audio input that the LTC comes from. The first choice is the beat detection's device (the Audio input device of the preferences), which is then shared; any other recording device gets its own stream, so LTC and the beat detection can listen to different devices. The signal is taken as mono. Leftclick lists the devices. ";
            }
            if (++ltcnamesage > 120) {
                ltcnamesage = 0;
                ltcnames.clear();
                int cnt = 0;
                SDL_AudioDeviceID *devs = SDL_GetAudioRecordingDevices(&cnt);
                for (int k = 0; k < cnt; k++) {
                    const char *nm = SDL_GetAudioDeviceName(devs[k]);
                    if (nm) ltcnames.push_back(nm);
                }
                if (devs) SDL_free(devs);
            }
            // the entries: 0 = the beat detection's device, then the devices (a chosen one that is gone stays listed)
            std::vector<std::string> opts, names;
            opts.push_back("beat device");
            names.push_back("");
            int curidx = 0;
            for (const std::string &nm : ltcnames) {
                if (nm == mainprogram->ltcdevice) curidx = (int)opts.size();
                opts.push_back(nm);
                names.push_back(nm);
            }
            if (!mainprogram->ltcdevice.empty() && curidx == 0) {
                curidx = (int)opts.size();
                opts.push_back(mainprogram->ltcdevice + " (missing)");
                names.push_back(mainprogram->ltcdevice);
            }
            // (a value that differs from the one set last frame was chosen in the menu)
            if (ltcpar->value != ltcsetval) {
                const int k = (int)(ltcpar->value + 0.5f);
                if (k >= 0 && k < (int)names.size() && names[k] != mainprogram->ltcdevice) {
                    mainprogram->ltcdevice = names[k];
                    curidx = k;
                    timecode_apply_prefs();
                    prefsdirty = true;
                }
            }
            ltcpar->options = opts;
            ltcpar->range[1] = (float)std::max(1, (int)opts.size() - 1);
            ltcpar->value = (float)curidx;
            ltcsetval = ltcpar->value;
            ltcpar->box->vtxcoords->x1 = diagx;
            ltcpar->box->vtxcoords->y1 = h2y;
            ltcpar->box->vtxcoords->w = 0.17f;
            ltcpar->box->vtxcoords->h = hh;
            ltcpar->box->upvtxtoscr();
            ltcpar->handle();
            diagx += 0.18f;

            // the channel of that input
            if (!ltcchanpar) {
                ltcchanpar = new Param;
                ltcchanpar->name = "LTC CH";
                ltcchanpar->type = FF_TYPE_OPTION;
                ltcchanpar->value = 0.0f;
                ltcchanpar->deflt = 0.0f;
                ltcchanpar->range[0] = 0.0f;
                ltcchanpar->range[1] = 2.0f;
                ltcchanpar->sliding = false;
                ltcchanpar->options = {"mix", "left", "right"};
                ltcchanpar->box->tooltiptitle = "LTC channel ";
                ltcchanpar->box->tooltip = "Which channel of the input carries the LTC. Mix averages both (fine when the other channel is silent); left or right takes that channel alone, so music on the other one cannot disturb it. Left and right use an input stream of their own, also on the beat detection's device. Leftclick lists the choices. ";
            }
            if (ltcchanpar->value != ltcchansetval) {
                const int k = (int)(ltcchanpar->value + 0.5f);
                if (k >= 0 && k <= 2 && k != (int)std::lround(mainprogram->tcltcchan)) {
                    mainprogram->tcltcchan = (float)k;
                    timecode_apply_prefs();
                    prefsdirty = true;
                }
            }
            ltcchanpar->value = (float)std::min(std::max((int)std::lround(mainprogram->tcltcchan), 0), 2);
            ltcchansetval = ltcchanpar->value;
            ltcchanpar->box->vtxcoords->x1 = diagx;
            ltcchanpar->box->vtxcoords->y1 = h2y;
            ltcchanpar->box->vtxcoords->w = 0.1f;
            ltcchanpar->box->vtxcoords->h = hh;
            ltcchanpar->box->upvtxtoscr();
            ltcchanpar->handle();
            diagx += 0.11f;
        }
        else if (source == TC_MTC || source == TC_ARTNET) {
            // which MIDI port the MTC comes from, or which Art-Net sender: any, or one of the choices
            const bool mtc = source == TC_MTC;
            if (!inpar) {
                inpar = new Param;
                inpar->name = "IN";
                inpar->type = FF_TYPE_OPTION;
                inpar->value = 0.0f;
                inpar->deflt = 0.0f;
                inpar->range[0] = 0.0f;
                inpar->sliding = false;
            }
            std::string &cur = mtc ? mainprogram->mtcinport : mainprogram->artnetsender;
            inpar->box->tooltiptitle = mtc ? "MIDI timecode port " : "Art-Net sender ";
            inpar->box->tooltip = mtc
                ? "The MIDI port that the timecode is taken from. The first choice takes it from any opened port; the list holds the ports that are opened in the Input devices tab of the preferences. Leftclick lists the ports. "
                : "The sender (IP address) that the Art-Net timecode is taken from. The first choice takes it from any sender; the list holds the senders that were heard in the last seconds. Leftclick lists the senders. ";
            std::vector<std::string> opts, names;
            opts.push_back(mtc ? "any port" : "any sender");
            names.push_back("");
            std::vector<std::string> avail = mtc ? mainprogram->openports : tm->artnet_senders();
            int curidx = 0;
            for (const std::string &nm : avail) {
                if (std::find(names.begin(), names.end(), nm) != names.end()) continue;
                if (nm == cur) curidx = (int)opts.size();
                opts.push_back(nm);
                names.push_back(nm);
            }
            if (!cur.empty() && curidx == 0) {
                curidx = (int)opts.size();
                opts.push_back(cur + (mtc ? " (not open)" : " (silent)"));
                names.push_back(cur);
            }
            if (inparsrc != source) {
                inparsrc = source;      // (another list: no choice made yet)
                inpar->value = (float)curidx;
                inparsetval = inpar->value;
            }
            // (a value that differs from the one set last frame was chosen in the menu)
            if (inpar->value != inparsetval) {
                const int k = (int)(inpar->value + 0.5f);
                if (k >= 0 && k < (int)names.size() && names[k] != cur) {
                    cur = names[k];
                    curidx = k;
                    timecode_apply_prefs();
                    tm->due_cues(false);
                    prefsdirty = true;
                }
            }
            inpar->options = opts;
            inpar->range[1] = (float)std::max(1, (int)opts.size() - 1);
            inpar->value = (float)curidx;
            inparsetval = inpar->value;
            inpar->box->vtxcoords->x1 = diagx;
            inpar->box->vtxcoords->y1 = h2y;
            inpar->box->vtxcoords->w = 0.2f;
            inpar->box->vtxcoords->h = hh;
            inpar->box->upvtxtoscr();
            inpar->handle();
            diagx += 0.21f;
        }
        if (source == TC_LTC) {
            // the state of the input, also without a signal: is audio arriving, how loud, are sync words found
            float peak;
            unsigned long long blocks, syncs, bad;
            int rate;
            tm->ltc_diag(peak, blocks, syncs, bad, rate);
            char ibuf[128];
            if (blocks == 0) snprintf(ibuf, sizeof(ibuf), "input: no audio arrives");
            else snprintf(ibuf, sizeof(ibuf), "input: %.0f dB  %d Hz  sync %llu  rejected %llu", 20.0 * std::log10(std::max(peak, 1e-6f)), rate, syncs, bad);
            diagstr = ibuf;
        }
        if (valid) {
            char dbuf[96];
            if (source == TC_INTERNAL) snprintf(dbuf, sizeof(dbuf), "cue %+.1f ms  lag %.2f f", mainprogram->tcdiagcue, mainprogram->tcdiaglag);
            else snprintf(dbuf, sizeof(dbuf), "jitter %.1f ms  cue %+.1f ms  lag %.2f f", tm->jitter_ms(), mainprogram->tcdiagcue, mainprogram->tcdiaglag);
            if (source == TC_LTC || source == TC_MTC || source == TC_ARTNET) {
                diagstr = diagstr.empty() ? std::string(dbuf) : diagstr + "  |  " + dbuf;     // (no room here next to the input choices: it is shown in the bottom strip)
            }
            else {
                text(dbuf, lightgrey, diagx, texty(h2y, hh), lx + rw - 0.02f - diagx);
                hitbox(pi, diagx, h2y, lx + rw - 0.02f - diagx, hh, "Timing diagnostics ", DIAG_TIP)->in();
            }
        }
    }

    // ---- column headers
    const float chy = top - 0.150f;
    const float cx[6] = {0.0f, 0.04f, 0.22f, 0.39f, 0.76f, 1.07f};
    const float cw[6] = {0.03f, 0.17f, 0.16f, 0.36f, 0.25f, 0.05f};
    const float colx = 1.02f;       // the colour swatch (to the right of NAME)
    const float colw = 0.04f;
    const char *cn[6] = {"ON", "TIME", "ACTION", "TARGET", "NAME", ""};
    for (int c = 0; c < 6; c++) {
        if (cn[c][0]) text(cn[c], yellow, lx + cx[c] + 0.004f, texty(chy, 0.035f), cw[c]);
    }

    // ---- cue rows
    const int ncues = (int)tm->cues.size();
    if (inregion && mainprogram->mousewheel != 0.0f) {
        scrollrow -= (mainprogram->mousewheel > 0.0f) ? 1 : -1;
        mainprogram->mousewheel = 0.0f;
    }
    scrollrow = std::min(std::max(scrollrow, 0), std::max(0, ncues - ROWS));

    // the open action list takes the click first
    float pickx = 0.0f, picktop = 0.0f;
    if (actionpick >= 0 && actionpick < ncues) {
        int rowpos = actionpick - scrollrow;
        float rowtop = chy - ROWH * (float)std::max(rowpos, 0);
        picktop = std::max(rowtop, ry + ROWH * (float)CUE_ACTION_COUNT + 0.005f);
        pickx = lx + cx[2];
        if (mainprogram->leftmouse) {
            int chosen = -1;
            for (int a = 0; a < CUE_ACTION_COUNT; a++) {
                Boxx *b = hitbox(pi, pickx, picktop - ROWH * (float)(a + 1), cw[2] + 0.02f, ROWH);
                if (b->in()) chosen = a;
            }
            if (chosen >= 0) {
                TimecodeCue &pc = tm->cues[actionpick];
                if (pc.action != chosen) pc.target = default_target(chosen);
                pc.action = chosen;
            }
            actionpick = -1;
            mainprogram->leftmouse = false;
        }
    }
    else actionpick = -1;

    // the colour palette (a strip of swatches left of the row's swatch) takes the click first
    const float swstep = colw + 0.004f;
    const float swx0 = lx + colx + colw - swstep * (float)CUE_COLOR_COUNT;
    float swy = 0.0f;
    if (colorpick >= 0 && colorpick < ncues && colorpick >= scrollrow && colorpick < scrollrow + ROWS) {
        swy = chy - ROWH * (float)(colorpick - scrollrow + 1);
        if (mainprogram->leftmouse) {
            for (int k = 0; k < CUE_COLOR_COUNT; k++) {
                Boxx *b = hitbox(pi, swx0 + swstep * (float)k, swy, colw, ROWH - 0.003f);
                if (b->in()) tm->cues[colorpick].color = k;
            }
            colorpick = -1;
            mainprogram->leftmouse = false;
        }
    }
    else colorpick = -1;

    for (int r = 0; r < ROWS; r++) {
        int i = scrollrow + r;
        if (i >= ncues) break;
        TimecodeCue &c = tm->cues[i];
        const float y = chy - ROWH * (float)(r + 1);
        const bool isnext = valid && tm->next_cue(st.secs) == i;
        const bool okline = !cue_osc_line(c.action, c.target).empty();
        float *tcol = c.fired ? yellow : white;

        // (draw_box takes the border colour first, then the fill)
        draw_box(isnext ? lightblue : (c.color > 0 ? cue_fill(c.color, false) : darkgrey),
                 c.color > 0 ? cue_fill(c.color, true) : black, lx, y, rw - 0.02f, ROWH - 0.003f, -1);

        // on / off
        {
            Boxx *b = hitbox(pi, lx + cx[0], y + 0.006f, cw[0], ROWH - 0.015f, "Cue on/off ",
                             "Green: the cue fires when the timecode passes its time. Leftclick toggles. ");
            bool in = b->in();
            draw_box(white, c.enabled ? green : (in ? grey : black), lx + cx[0], y + 0.006f, cw[0], ROWH - 0.015f, -1);
            if (in && mainprogram->leftmouse && actionpick < 0) {
                c.enabled = !c.enabled;
                mainprogram->leftmouse = false;
            }
        }
        // time
        {
            time_cell(pi, lx + cx[1], y, cw[1], ROWH - 0.003f, i, 0, secs_string(c.secs), tcol, "Cue time ",
                      "When the cue fires (yellow: passed). Leftclick hours, minutes, seconds or frames and type digits (two digits move on to the next part, Tab also moves, Return sets the time, Esc cancels). ");
        }
        // action
        {
            Boxx *b = hitbox(pi, lx + cx[2], y, cw[2], ROWH - 0.003f, "Cue action ",
                             "What the cue does (it runs as an OSC message). Leftclick lists the actions. ");
            bool in = b->in();
            draw_box(in ? white : grey, black, lx + cx[2], y, cw[2], ROWH - 0.003f, -1);
            text(cue_action_name(c.action), tcol, lx + cx[2] + 0.006f, texty(y, ROWH), cw[2]);
            if (in && mainprogram->leftmouse && actionpick < 0) {
                actionpick = i;
                mainprogram->leftmouse = false;
            }
        }
        // target
        if (c.action == CUE_LOADBIN) {
            // LOAD BIN: the BIN option param lists the bins; the bank and block (like A1 3) are typed next to it
            std::vector<std::string> names;
            for (Bin *bin : binsmain->bins) names.push_back(bin->name);
            if (names.empty()) names.push_back("(no bins)");
            std::string bname, brest;
            loadbin_split(c.target, bname, brest);
            Param *bp = get_binpar(r);
            // (a value that differs from the one set last frame was chosen in the menu)
            if (bp->value != binsetval[r] && binrow == i) {
                const int k = (int)(bp->value + 0.5f);
                if (k >= 0 && k < (int)names.size() && names[k] != "(no bins)") {
                    bname = names[k];
                    c.target = bname + " " + brest;
                }
            }
            int idx = -1;
            for (int k = 0; k < (int)names.size(); k++) {
                if (names[k] == bname) idx = k;
            }
            if (idx < 0) {
                // the bin of the cue is gone (renamed, deleted): shown as it is, until another one is chosen
                names.push_back((bname.empty() ? std::string("?") : bname) + " (missing)");
                idx = (int)names.size() - 1;
            }
            bp->options = names;
            bp->range[1] = (float)std::max(1, (int)names.size() - 1);
            bp->value = (float)idx;
            binsetval[r] = bp->value;
            const float bw = 0.235f;
            bp->box->vtxcoords->x1 = lx + cx[3];
            bp->box->vtxcoords->y1 = y;
            bp->box->vtxcoords->w = bw;
            bp->box->vtxcoords->h = ROWH - 0.003f;
            bp->box->upvtxtoscr();
            if (bp->box->in() && mainprogram->leftmouse && actionpick < 0 && !ed.active) binrow = i;
            bp->handle();

            const float x2 = lx + cx[3] + bw + 0.008f;
            const float w2 = cw[3] - bw - 0.008f;
            Boxx *b = hitbox(pi, x2, y, w2, ROWH - 0.003f, "Bank and block ",
                             "The shelf bank to load into (A1-A4, B1-B4) and the block of the bin (1-9), like A1 3. ");
            if (ed.active && ed.row == i && ed.col == 4) {
                draw_box(white, black, x2, y, w2, ROWH - 0.003f, -1);
                do_text_input(x2 + 0.006f, texty(y, ROWH), TX, TY, mainprogram->mx, mainprogram->my, mainprogram->xvtxtoscr(w2 - 0.01f), 0, nullptr);
            }
            else {
                draw_box(grey, black, x2, y, w2, ROWH - 0.003f, -1);
                text(brest.empty() ? "(A1 3)" : brest, okline ? tcol : red, x2 + 0.006f, texty(y, ROWH), w2);
                if (b->in()) {
                    b->tooltiptitle = "Bank and block ";
                    b->tooltip = "The shelf bank to load into (A1-A4, B1-B4) and the block of the bin (1-9), like A1 3. ";
                    if (mainprogram->leftmouse && actionpick < 0) start_edit(i, 4, brest);
                }
            }
        }
        else {
            Boxx *b = hitbox(pi, lx + cx[3], y, cw[3], ROWH - 0.003f, "Cue target ",
                             "CLIP NEXT / PLAY / PAUSE / STOP: deck letter and layer, like A1. JUMP: A1 150 (frame number; one past the end marker has the video's frame count deducted). SHELF: bank, element and layer, like A2 5 B3 (element 5 of bank 2 of shelf A into layer 3 of deck B); with CHASE on, launched layers follow the timecode, and a locate relaunches the last cue per layer. SCENE: A2 (deck A, scene 2) or 2 (both decks). LPST START / STOP: the line number. LOAD BIN: choose the bin with BIN, then type bank and block, like A1 3 (block 3 of that bin into bank A1; banks A1-A4, B1-B4, blocks 1-9). OSC: an OSC message like /mix/output/deckA/layer/1/opacity 0.5. Red means the target does not fit the action. ");
            b->in();
            if (ed.active && ed.row == i && ed.col == 2) {
                draw_box(white, black, lx + cx[3], y, cw[3], ROWH - 0.003f, -1);
                do_text_input(lx + cx[3] + 0.006f, texty(y, ROWH), TX, TY, mainprogram->mx, mainprogram->my, mainprogram->xvtxtoscr(cw[3] - 0.01f), 0, nullptr);
            }
            else {
                text(c.target.empty() ? "(click to type)" : c.target, okline ? tcol : red, lx + cx[3] + 0.006f, texty(y, ROWH), cw[3]);
                if (b->in()) {
                    b->tooltiptitle = "Cue target ";
                    b->tooltip = "CLIP NEXT / PLAY / PAUSE / STOP: deck letter and layer, like A1. JUMP: A1 150 (frame number; one past the end marker has the video's frame count deducted). SHELF: bank, element and layer, like A2 5 B3 (element 5 of bank 2 of shelf A into layer 3 of deck B); with CHASE on, launched layers follow the timecode, and a locate relaunches the last cue per layer. SCENE: A2 (deck A, scene 2) or 2 (both decks). LPST START / STOP: the line number. LOAD BIN: choose the bin with BIN, then type bank and block, like A1 3 (block 3 of that bin into bank A1; banks A1-A4, B1-B4, blocks 1-9). OSC: an OSC message like /mix/output/deckA/layer/1/opacity 0.5. Red means the target does not fit the action. ";
                    if (mainprogram->leftmouse && actionpick < 0) start_edit(i, 2, c.target);
                }
            }
        }
        // name
        {
            Boxx *b = hitbox(pi, lx + cx[4], y, cw[4], ROWH - 0.003f, "Cue name ",
                             "A name for the cue, only for looking at the list. Leftclick to type. ");
            b->in();
            if (ed.active && ed.row == i && ed.col == 1) {
                draw_box(white, black, lx + cx[4], y, cw[4], ROWH - 0.003f, -1);
                do_text_input(lx + cx[4] + 0.006f, texty(y, ROWH), TX, TY, mainprogram->mx, mainprogram->my, mainprogram->xvtxtoscr(cw[4] - 0.01f), 0, nullptr);
            }
            else {
                text(c.name.empty() ? "(name)" : c.name, tcol, lx + cx[4] + 0.006f, texty(y, ROWH), cw[4]);
                if (b->in() && mainprogram->leftmouse && actionpick < 0) start_edit(i, 1, c.name);
            }
        }
        // colour: the swatch opens the palette
        {
            Boxx *b = hitbox(pi, lx + colx, y, colw, ROWH - 0.003f, "Cue colour ",
                             "Only for looking at the list. Leftclick chooses a colour. ");
            bool in = b->in();
            draw_box(in ? white : grey, c.color > 0 ? cue_fill(c.color, false) : darkgrey, lx + colx, y, colw, ROWH - 0.003f, -1);
            if (in) {
                b->tooltiptitle = "Cue colour ";
                b->tooltip = "Only for looking at the list. Leftclick chooses a colour. ";
            }
            if (in && mainprogram->leftmouse && actionpick < 0) {
                colorpick = (colorpick == i) ? -1 : i;
                mainprogram->leftmouse = false;
            }
        }
        // duplicate: a copy right behind it (same time; change the time to move it)
        {
            Boxx *b = hitbox(pi, lx + cx[5] + 0.055f, y, 0.05f, ROWH - 0.003f, "Duplicate cue ",
                             "Adds a copy right behind this cue, at the same time. Change the time to move it. ");
            bool in = b->in();
            draw_box(white, in ? grey : black, lx + cx[5] + 0.055f, y, 0.05f, ROWH - 0.003f, -1);
            text("DUP", white, lx + cx[5] + 0.063f, texty(y, ROWH), 0.05f);
            if (in && mainprogram->leftmouse && actionpick < 0) {
                mainprogram->leftmouse = false;
                if (ed.active) {
                    mainprogram->renaming = EDIT_NONE;
                    end_input();
                    ed.active = false;
                }
                TimecodeCue copy = tm->cues[i];
                tm->cues.insert(tm->cues.begin() + i + 1, copy);
                break;      // (the rows shifted)
            }
        }
        // delete
        {
            Boxx *b = hitbox(pi, lx + cx[5], y, cw[5], ROWH - 0.003f, "Delete cue ", "Removes this cue. ");
            bool in = b->in();
            draw_box(white, in ? grey : black, lx + cx[5], y, cw[5], ROWH - 0.003f, -1);
            text("DEL", white, lx + cx[5] + 0.008f, texty(y, ROWH), cw[5]);
            if (in && mainprogram->leftmouse && actionpick < 0) {
                mainprogram->leftmouse = false;
                if (ed.active) {
                    mainprogram->renaming = EDIT_NONE;
                    end_input();
                    ed.active = false;
                }
                tm->cues.erase(tm->cues.begin() + i);
                break;      // (the rows shifted)
            }
        }
    }
    if (ncues == 0) {
        text("No cues. + CUE adds one at the current timecode.", yellow, lx + 0.004f, texty(chy - ROWH, ROWH), rw - 0.04f);
    }
    else if (ncues > ROWS) {
        char buf[48];
        snprintf(buf, sizeof(buf), "%d-%d of %d (mousewheel scrolls)", scrollrow + 1, std::min(scrollrow + ROWS, ncues), ncues);
        text(buf, yellow, lx + 0.004f, texty(chy - ROWH * (float)(ROWS + 1), ROWH), 0.35f);
    }

    // ---- bottom strip: import / export / shift all
    {
        // (the tenth row of the table's height)
        const float sh = ROWH - 0.003f;
        {
            const float sy = chy - ROWH * (float)(ROWS + 1);
            float bx = lx + 0.37f;
            if (button(pi, "IMPORT", bx, sy, 0.1f, sh, white,
                       "Import cues ", "Adds cues from a CSV file (this table's own export, or a DAW marker list) or a CMX3600 EDL file. Imported marker lists come in switched off, so nothing fires before you have set the actions. ")) {
                mainprogram->pathto = "TCIMPORT";
                std::string dir;
                try { dir = std::filesystem::canonical(mainprogram->currelemsdir).generic_string(); } catch (...) {}
                std::thread filereq(&Program::get_inname, mainprogram, "Import cues (CSV or EDL)", "", dir);
                filereq.detach();
            }
            bx += 0.11f;
            if (button(pi, "EXPORT", bx, sy, 0.1f, sh, white,
                       "Export cues ", "Saves the cue list as a CSV file. ")) {
                mainprogram->pathto = "TCEXPORT";
                std::string dir;
                try { dir = std::filesystem::canonical(mainprogram->currelemsdir).generic_string(); } catch (...) {}
                std::thread filereq(&Program::get_outname, mainprogram, "Export cues (CSV)", "", dir);
                filereq.detach();
            }
            bx += 0.11f;
            if (ed.active && ed.col == 5) {
                // a sign (click or type + / -) and the time, edited like any other time
                Boxx *sb = hitbox(pi, bx, sy, 0.03f, sh, "Shift direction ",
                                  "+ moves all cues later, - earlier. Leftclick toggles, or type + or -. ");
                if (sb->in() && mainprogram->leftmouse) {
                    shiftneg = !shiftneg;
                    mainprogram->leftmouse = false;
                }
                draw_box(white, sb->in() ? grey : black, bx, sy, 0.03f, sh, -1);
                text(shiftneg ? "-" : "+", white, bx + 0.011f, texty(sy, sh), 0.03f);
                time_cell(pi, bx + 0.032f, sy, 0.17f, sh, -1, 5, "00:00:00:00", white, "Shift amount ",
                          "How far all cues move. Type digits per part; Return shifts all cues, Esc cancels. ");
            }
            else if (button(pi, "SHIFT ALL", bx, sy, 0.12f, sh, white,
                            "Shift all cues ", "Moves every cue later or earlier by an amount you set, e.g. to line the list up with a new start time. ")) {
                start_field_edit(-1, 5, "00:00:00:00", 0);
            }
            bx += 0.21f;
            if (noticeframes > 0) {
                noticeframes--;
                text(notice, yellow, bx, texty(sy, sh), rx + rw - bx - 0.01f);
            }
            else if (!diagstr.empty()) {
                text(diagstr, lightgrey, bx, texty(sy, sh), rx + rw - bx - 0.01f);
                hitbox(pi, bx, sy, rx + rw - bx - 0.01f, sh, "Timing diagnostics ", DIAG_TIP)->in();
            }
            else if (ncues > 0) {
                text("SHIFT ALL: sign + hh:mm:ss:ff", lightgrey, bx, texty(sy, sh), rx + rw - bx - 0.01f);
            }
        }
    }

    // the palette of the cue whose colour is being chosen, on top
    if (colorpick >= 0 && colorpick < ncues) {
        const bool bufb = mainprogram->frontbatch;
        mainprogram->frontbatch = true;
        for (int k = 0; k < CUE_COLOR_COUNT; k++) {
            const float x = swx0 + swstep * (float)k;
            Boxx *b = hitbox(pi, x, swy, colw, ROWH - 0.003f);
            const bool in = b->in();
            draw_box(k == tm->cues[colorpick].color ? green : (in ? white : black), k > 0 ? cue_fill(k, false) : darkgrey,
                     x, swy, colw, ROWH - 0.003f, -1);
            if (k == 0) text("-", white, x + 0.016f, texty(swy, ROWH), colw);
        }
        mainprogram->frontbatch = bufb;
    }

    // the action list of the cue that is being changed, on top
    if (actionpick >= 0 && actionpick < ncues) {
        // drawn in the front batch so the opaque dark grey boxes and their texts come after the cue rows underneath
        const bool bufb = mainprogram->frontbatch;
        mainprogram->frontbatch = true;
        for (int a = 0; a < CUE_ACTION_COUNT; a++) {
            const float y = picktop - ROWH * (float)(a + 1);
            Boxx *b = hitbox(pi, pickx, y, cw[2] + 0.02f, ROWH);
            const bool in = b->in();
            draw_box(white, in ? grey : darkgrey, pickx, y, cw[2] + 0.02f, ROWH, -1);
            text(cue_action_name(a), a == tm->cues[actionpick].action ? green : white, pickx + 0.006f, texty(y, ROWH), cw[2] + 0.01f);
        }
        mainprogram->frontbatch = bufb;
    }

}

// Keyboard of the four-part time editor (called by the event loop before the generic text input): only digits and the
// editing keys get through; true when the event was used.
bool timecode_field_edit_event(const SDL_Event &e) {
    if (!(ed.active && fe.on)) return false;
    if (e.type == SDL_EVENT_TEXT_INPUT) {
        for (const char *p = e.text.text; *p; p++) {
            if (*p >= '0' && *p <= '9') field_digit(*p - '0');
            else if (ed.col == 5 && *p == '-') shiftneg = true;
            else if (ed.col == 5 && *p == '+') shiftneg = false;
        }
        return true;
    }
    if (e.type == SDL_EVENT_KEY_DOWN) {
        const SDL_Keycode k = e.key.key;
        const bool shift = (SDL_GetModState() & SDL_KMOD_SHIFT) != 0;
        if (k == SDLK_TAB) field_focus(fe.field + (shift ? -1 : 1));
        else if (k == SDLK_RIGHT) field_focus(fe.field + 1);
        else if (k == SDLK_LEFT) field_focus(fe.field - 1);
        else if (k == SDLK_BACKSPACE || k == SDLK_DELETE) {
            if (!fe.digits.empty()) fe.digits.pop_back();
        }
        else if (k == SDLK_RETURN || k == SDLK_KP_ENTER) end_field_edit(true);
        else if (k == SDLK_ESCAPE) end_field_edit(false);
        return true;
    }
    return false;
}
