#ifndef EWOCVJ2_MIDICLOCK_H
#define EWOCVJ2_MIDICLOCK_H

#include <functional>
#include <memory>
#include <string>

// MIDI clock (24 pulses per quarter note) in and out.  RtMidi is only included by MidiClock.cpp.

struct MidiClockState {
    bool valid = false;     // pulses are coming in and the transport is running
    double bpm = 0.0;       // tempo measured from the spacing of the pulses
    double beatpos = 0.0;   // beats since the last Start message (or since the first pulse), continuous
    bool running = false;   // between Start/Continue and Stop
    unsigned startcount = 0;    // number of Start messages received: the beat position restarts from 0 when it changes
};

class MidiClock {
public:
    MidiClock();
    ~MidiClock();

    // ---- clock in ----
    // Called from the MIDI callback thread with the raw message of the port that is the clock source.
    void on_message(const unsigned char *data, size_t size);
    // forget the measured tempo and position (clock in switched on/off, source port changed)
    void reset_input();
    // seconds that the beat position is looked ahead (+) or behind (-): compensates output/display latency
    void set_offset_ms(double ms);
    // thread-safe; returns false while no clock is coming in or the sender is stopped
    bool get_state(MidiClockState &state) const;

    // ---- clock out ----
    // Source of the tempo/position that is sent: returns false while there is no tempo.  bpm is for information,
    // beatpos is the continuous beat position; the pulses are phase locked to it, so they follow tempo changes
    // and jumps without drifting.  A bar starts where beatpos is a multiple of quantum (a Start message is sent there).
    typedef std::function<bool(double &bpm, double &beatpos)> BeatSource;
    // opens the output port with this name (the first one when portname is empty) and starts sending; false if that fails
    bool start_output(const std::string &portname, const BeatSource &source, double quantum = 4.0);
    void stop_output();
    bool output_active() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

#endif //EWOCVJ2_MIDICLOCK_H
