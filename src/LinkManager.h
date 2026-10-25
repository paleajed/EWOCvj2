#ifndef EWOCVJ2_LINKMANAGER_H
#define EWOCVJ2_LINKMANAGER_H

#include <cstddef>
#include <memory>

// Ableton Link (tempo/beat sync over the network, see external/link).  The Link headers pull in asio and with it
// winsock2/windows.h, so they are only included by LinkManager.cpp: nothing here exposes them.

struct LinkState {
    bool valid = false;     // Link is enabled and a state could be read
    double bpm = 120.0;     // session tempo
    double beat = 0.0;      // beat position on the shared Link timeline (fractional, continuous; can be negative)
    double phase = 0.0;     // beat position inside the quantum: 0 <= phase < quantum
    double quantum = 4.0;   // beats per bar/phrase that phase is measured against
    size_t peers = 0;       // number of other Link participants
};

// The tempo/beat the program is synced to: from audio beat detection, or from Ableton Link when that is enabled.
struct BeatInfo {
    bool valid = false;         // a tempo is known (audio: after detection has found one)
    bool fromlink = false;      // comes from Ableton Link or MIDI clock: exact, so the audio peak threshold doesn't apply
    bool frommidiclock = false; // (with fromlink) comes from MIDI clock in, not from Link
    int sourceid = 2;           // 0 Link, 1 MIDI clock, 2 audio: the beat sync restarts its counting when this changes
    double bpm = 0.0;
    double secsperbeat = 0.0;
    double beatpos = 0.0;       // fractional position in beats on a continuous timeline
    double barphase = 0.0;      // 0..1 over a bar (4 beats for audio, the Link quantum for Link)
    size_t peers = 0;           // Link only
};

class LinkManager {
public:
    LinkManager();
    ~LinkManager();

    void enable(bool on);
    bool enabled() const;

    // beats per bar: the phase of everybody in the session is aligned modulo this value
    void set_quantum(double quantum);
    // seconds that the beat position is looked ahead (+) or behind (-): compensates output/display latency
    void set_offset_ms(double ms);

    // thread-safe; returns false if Link is not enabled
    bool get_state(LinkState &state) const;

    // propose a new session tempo to the other participants
    void set_tempo(double bpm);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

#endif //EWOCVJ2_LINKMANAGER_H
