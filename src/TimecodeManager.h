#ifndef EWOCVJ2_TIMECODEMANAGER_H
#define EWOCVJ2_TIMECODEMANAGER_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

// Absolute timecode (hh:mm:ss:ff) from an external source (MIDI timecode, Art-Net, LTC audio) or from an internal
// clock.  The position is kept as seconds (secs) with the frame rate it was counted at, so the cue list and layer
// chase can compare positions without caring how the label is written.  RtMidi is not needed here: the MIDI
// callback hands the raw messages to on_midi().

enum TimecodeSource {
    TC_OFF = 0,
    TC_MTC = 1,
    TC_ARTNET = 2,
    TC_LTC = 3,
    TC_INTERNAL = 4
};

enum TimecodeOutput {
    TCO_OFF = 0,
    TCO_MTC = 1,
    TCO_ARTNET = 2,
    TCO_LTC = 3,
    TCO_COUNT = 4
};

struct TimecodeState {
    bool valid = false;     // a position is known (signal present, or the internal clock exists)
    bool running = false;   // the position is advancing
    double secs = 0.0;      // position in seconds (offset applied)
    double fps = 25.0;      // exact frame rate: 23.976, 24, 25, 29.97 or 30
    bool drop = false;      // drop frame counting (29.97 only)
    bool locked = false;    // an external source that has been steady for a few packets (the internal clock: always)
    bool freewheeling = false;  // the source stopped sending but the clock still runs on (freewheel time)
};

// label <-> seconds for a frame rate (nominal 24/25/30 counting; drop frame skips frame numbers 0 and 1 of every
// minute that is not a multiple of ten at 29.97)
struct TimecodeLabel {
    int h = 0, m = 0, s = 0, f = 0;
};
TimecodeLabel timecode_label(double secs, double fps, bool drop);
double timecode_secs(const TimecodeLabel &label, double fps, bool drop);
int timecode_nominal_fps(double fps);
std::string timecode_string(const TimecodeLabel &label, bool drop);   // "hh:mm:ss:ff" ("hh:mm:ss;ff" for drop frame)
// parses "hh:mm:ss:ff" (also with ';' or '.' as the last separator, or fewer fields: "ss:ff", "ff"); false if it is not a time
bool timecode_parse(const std::string &str, TimecodeLabel &label);

// the names of the MIDI output ports (for choosing where MTC goes)
std::vector<std::string> timecode_midi_output_ports();

// ---- cue list: "at this timecode, do this" ----
// A cue's target is typed in the table; cue_osc_line() turns action + target into the OSC message that is run.
enum CueAction {
    CUE_CLIPNEXT = 0,   // target "A1": the next clip of the queue of layer 1 of deck A
    CUE_PLAY,           // "A1"
    CUE_PAUSE,          // "A1"
    CUE_STOP,           // "A1"
    CUE_JUMP,           // "A1 150": scrub layer 1 of deck A to frame 150 (a frame past the end marker counts from the end: numf is deducted)
    CUE_SHELF,          // "A2 5 B3": trigger element 5 of bank 2 of shelf A into layer 3 of deck B (bank and layer are always given)
    CUE_SCENE,          // "A2": switch deck A to scene 2; "2": both decks
    CUE_LPSTSTART,      // "3": loop play button of loopstation line 3 on
    CUE_LPSTSTOP,       // "3": off
    CUE_LOADBIN,        // "my bin A1 3": block 3 (1-9) of the bin named "my bin" into bank 1 of shelf A (A1-A4, B1-B4)
    CUE_OSC,           // "/mix/output/deckA/layer/1/opacity 0.5": any OSC message
    CUE_ACTION_COUNT
};
const char *cue_action_name(int action);
// the OSC message text of a cue ("" when the target does not fit the action)
std::string cue_osc_line(int action, const std::string &target);

// cue colours of the table: 0 none, then red, orange, yellow, green, cyan, blue, purple, pink
const int CUE_COLOR_COUNT = 9;
const char *cue_color_name(int color);

struct TimecodeCue {
    int color = 0;          // CUE_COLOR_COUNT choices, only for looking at the list
    bool enabled = true;
    std::string name;
    double secs = 0.0;      // when
    int action = CUE_SHELF;
    std::string target;
    bool fired = false;     // passed (fired, or jumped over): not shown as upcoming
};

// the frame rates the table view offers: 23.976, 24, 25, 29.97, 29.97 drop frame, 30
const int TIMECODE_FPS_COUNT = 10;
void timecode_fps_from_index(int idx, double &fps, bool &drop);
int timecode_fps_index(double fps, bool drop);     // the index of a frame rate (a close one if there is no exact match)
const char *timecode_fps_name(int idx);

class TimecodeManager {
public:
    TimecodeManager();
    ~TimecodeManager();

    void set_source(int source);   // TimecodeSource; the signal of the previous source is forgotten
    int source() const;

    // the frame rate the internal clock counts at, and that a label is written in while no external source tells
    void set_fps(double fps, bool drop);
    // seconds that the position is moved (+) or delayed (-): compensates for latency
    void set_offset_ms(double ms);
    // seconds that the position is read ahead: the time between reading it and the frame being on the display (about a
    // frame period; the offset covers the rest, like a projector's delay).  Set every frame by the table view.
    void set_lookahead(double secs);
    // seconds (ms here) that the clock keeps running when an external source stops sending: it bridges a dropout. 0: the
    // position holds as soon as the packets stop (a sender that is stopped looks the same, so a long freewheel lets the
    // clock run on past a real stop).
    void set_freewheel_ms(double ms);
    // how far the arriving positions lay from the smoothed clock (running average, ms): the quality of the source
    double jitter_ms() const;

    // thread-safe; returns false while there is no position (no signal, or the source is off).  raw: the position of
    // the source itself, without the offset and the look-ahead (what a timecode output sends on).
    bool get_state(TimecodeState &state, bool raw = false) const;

    // ---- timecode out: sends the position on (the internal clock as master, or any source re-sent in another format) ----
    // mode: TCO_OFF / TCO_MTC (to the MIDI output port midiport, empty: the first one; 24, 25, 29.97 DF and 30 fps) /
    // TCO_ARTNET (ArtTimeCode broadcast on UDP 6454; the same rates) / TCO_LTC (audio: the owner of the audio output
    // pulls the samples with ltc_generate(); the same rates).  Returns false when it could not start (no such port).
    bool set_output(int mode, const std::string &midiport);
    bool output_ok() const;     // the chosen output runs (or none is chosen)
    // n mono samples of linear timecode for the position now (silence while stopped); called from the audio thread
    void ltc_generate(float *out, size_t n, int samplerate);

    // ---- internal clock ----
    void internal_play();
    void internal_stop();
    void internal_reset();             // stop and back to 00:00:00:00
    void internal_set(double secs);    // jump (keeps running if it ran)

    // ---- cue list (main thread only) ----
    std::vector<TimecodeCue> cues;
    void sort_cues();
    // Compares the position with the one of the previous call and returns the cues that were passed (which are
    // then marked fired).  A jump (backward, or forward by more than a few seconds) fires nothing: it only
    // re-marks which cues lie behind the new position.  Pass live=false to forget the position (preview modus).
    // lead(cue) is how many seconds before its time a cue fires (the nearest display frame gets it; a pre-rolled SHELF
    // cue fires early enough to be loaded when its time comes); none: 0.
    std::vector<int> due_cues(bool live, const std::function<double(const TimecodeCue &)> &lead = nullptr);
    int next_cue(double secs) const;   // index of the first enabled cue after secs, or -1
    // true once after a due_cues() call that found a first position or a jump (the cues were only re-marked): layer
    // chase uses it to rebuild the layers that should be playing at the new position
    bool take_jump() { bool j = jumpflag; jumpflag = false; return j; }

    // ---- MIDI timecode in ----
    // Called from the MIDI callback thread with the raw message (0xF1 quarter frame, or a complete SysEx
    // full frame message F0 7F xx 01 01 hh mm ss ff F7).
    void on_midi(const unsigned char *data, size_t size);
    // the same, from the MIDI port of that name: ignored when another port is chosen with set_mtc_port()
    void on_midi(const std::string &port, const unsigned char *data, size_t size);
    // the MIDI port that the MTC is taken from (the name of the port as the preferences list it); "" any open port
    void set_mtc_port(const std::string &port);

    // ---- Art-Net in ----
    // the IP address that the Art-Net timecode is taken from ("" any sender)
    void set_artnet_sender(const std::string &ip);
    // the senders of Art-Net timecode heard in the last seconds (only while Art-Net is the source)
    std::vector<std::string> artnet_senders() const;

    // ---- LTC (linear timecode) in ----
    // Called from the audio thread with every block of mono samples that comes in (any block size); does nothing
    // unless the source is LTC.  Art-Net needs no call: its listener thread runs while Art-Net is the source.
    void on_audio(const float *samples, size_t n, int samplerate);
    // what the LTC input shows: peak level (0 - 1, held a little), blocks of audio received, sync words found, frames
    // thrown out (bad digits / parity), the sample rate of the stream
    void ltc_diag(float &peak, unsigned long long &blocks, unsigned long long &syncs, unsigned long long &bad, int &rate) const;

private:
    void mtc_out_loop();
    void artnet_out_loop();
    double lastsecs = 0.0;      // position at the previous due_cues() call
    bool haslast = false;
    bool jumpflag = false;
    struct Impl;
    std::unique_ptr<Impl> impl;
};

#endif //EWOCVJ2_TIMECODEMANAGER_H
