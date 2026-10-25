#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <mmsystem.h>
#endif

#include <RtMidi.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

#include "MidiClock.h"

// (see program.h)
#ifdef MACOS
#define EWOC_RTMIDI_API RtMidi::MACOSX_CORE
#else
#define EWOC_RTMIDI_API RtMidi::UNSPECIFIED
#endif

namespace {
    const unsigned char MIDI_CLOCK = 0xF8;
    const unsigned char MIDI_START = 0xFA;
    const unsigned char MIDI_CONTINUE = 0xFB;
    const unsigned char MIDI_STOP = 0xFC;
    const double PPQN = 24.0;
    const size_t MAXSTAMPS = 48;        // pulses the tempo is averaged over: 2 beats
    const double INPUT_TIMEOUT = 0.5;   // seconds without a pulse: the sender is gone (24 pulses/beat: even 30 bpm sends 12/s)

    double now_secs() {
        return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }
}

struct MidiClock::Impl {
    // clock in
    mutable std::mutex mtx;
    unsigned startcount = 0;
    bool running = true;            // no Start seen yet when connecting to a sender that already plays: assume it runs
    unsigned long long npulses = 0; // pulses since the start, the next pulse is number npulses
    double lastpulsepos = 0.0;      // beat position of the last pulse
    double lastpulse = 0.0;         // when it arrived
    double interval = 0.0;          // seconds per pulse
    std::deque<double> stamps;
    std::atomic<double> offsetsecs{0.0};

    // clock out
    std::thread thread;
    std::atomic<bool> quit{false};
    std::atomic<bool> active{false};
    RtMidiOut *out = nullptr;

    void send(unsigned char b) {
        if (!out) return;
        try {
            std::vector<unsigned char> msg(1, b);
            out->sendMessage(&msg);
        }
        catch (RtMidiError &) {}
    }

    void output_loop(BeatSource source, double quantum);
};

MidiClock::MidiClock() : impl(new Impl) {
}

MidiClock::~MidiClock() {
    stop_output();
}

void MidiClock::reset_input() {
    std::lock_guard<std::mutex> lock(impl->mtx);
    impl->running = true;
    impl->npulses = 0;
    impl->lastpulsepos = 0.0;
    impl->lastpulse = 0.0;
    impl->interval = 0.0;
    impl->stamps.clear();
}

void MidiClock::set_offset_ms(double ms) {
    impl->offsetsecs = ms / 1000.0;
}

void MidiClock::on_message(const unsigned char *data, size_t size) {
    if (!data || size < 1) return;
    const unsigned char status = data[0];
    if (status < MIDI_CLOCK) return;

    std::lock_guard<std::mutex> lock(impl->mtx);
    switch (status) {
        case MIDI_CLOCK: {
            const double t = now_secs();
            if (!impl->stamps.empty() && t - impl->stamps.back() > INPUT_TIMEOUT) {
                // the sender was gone for a while: its tempo may be a different one now
                impl->stamps.clear();
                impl->interval = 0.0;
            }
            impl->stamps.push_back(t);
            if (impl->stamps.size() > MAXSTAMPS) impl->stamps.pop_front();
            if (impl->stamps.size() >= 2) {
                impl->interval = (impl->stamps.back() - impl->stamps.front()) / (double)(impl->stamps.size() - 1);
            }
            if (impl->running) {
                impl->lastpulsepos = (double)impl->npulses / PPQN;
                impl->npulses++;
            }
            impl->lastpulse = t;
            break;
        }
        case MIDI_START:
            // the next pulse is the first of beat 0
            impl->npulses = 0;
            impl->lastpulsepos = 0.0;
            impl->running = true;
            impl->startcount++;
            break;
        case MIDI_CONTINUE:
            impl->running = true;
            break;
        case MIDI_STOP:
            impl->running = false;
            break;
        default:
            break;
    }
}

bool MidiClock::get_state(MidiClockState &state) const {
    std::lock_guard<std::mutex> lock(impl->mtx);
    state.running = impl->running;
    state.startcount = impl->startcount;
    state.valid = false;
    if (!impl->running || impl->interval <= 0.0 || impl->lastpulse <= 0.0) return false;
    const double t = now_secs();
    if (t - impl->lastpulse > INPUT_TIMEOUT) return false;

    state.bpm = 60.0 / (impl->interval * PPQN);
    // continuous: the time since the last pulse counts too (but no further than where the next pulse is due)
    const double frac = std::min(std::max((t - impl->lastpulse) / impl->interval, 0.0), 1.0);
    state.beatpos = impl->lastpulsepos + frac / PPQN + impl->offsetsecs.load() * state.bpm / 60.0;
    state.valid = true;
    return true;
}

// Pulses are derived from the beat position of the source instead of timed on their own: whatever the source does
// (tempo changes, a jump of the Link timeline) the output stays locked to it, with no drift.  The only error is
// the polling interval.
void MidiClock::Impl::output_loop(BeatSource source, double quantum) {
#ifdef _WIN32
    timeBeginPeriod(1);  // otherwise sleeping takes up to 15 ms, which would be a sixth of a pulse at 120 bpm
#endif
    const long long barpulses = std::max(1LL, (long long)std::llround(PPQN * quantum));
    long long prev = -1;
    bool started = false;

    while (!quit) {
        double bpm = 0.0, beatpos = 0.0;
        if (!source(bpm, beatpos) || beatpos < 0.0) {
            if (started) send(MIDI_STOP);
            started = false;
            prev = -1;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        const long long cur = (long long)std::floor(beatpos * PPQN);
        if (prev < 0) {
            prev = cur;
        }
        else if (cur != prev) {
            const long long d = cur - prev;
            if (d > 8 || d < -2) {
                // the position jumped (other Link session, source changed): stop, and start again on the next bar
                if (started) send(MIDI_STOP);
                started = false;
                prev = cur;
            }
            else if (d > 0) {
                long long first = prev + 1;  // first pulse to send
                if (!started) {
                    // wait for a bar: the Start message goes just before the pulse that begins it
                    const long long bar = cur - (cur % barpulses);
                    if (bar > prev) {
                        send(MIDI_START);
                        started = true;
                        first = bar;
                    }
                }
                if (started) {
                    for (long long p = first; p <= cur; p++) send(MIDI_CLOCK);
                }
                prev = cur;
            }
            // (a small step back is the source correcting itself: send nothing, wait to get past prev again)
        }
        std::this_thread::sleep_for(std::chrono::microseconds(500));
    }
    if (started) send(MIDI_STOP);
#ifdef _WIN32
    timeEndPeriod(1);
#endif
}

bool MidiClock::start_output(const std::string &portname, const BeatSource &source, double quantum) {
    stop_output();
    if (!source) return false;

    RtMidiOut *out = nullptr;
    try {
        out = new RtMidiOut(EWOC_RTMIDI_API);
        unsigned int n = out->getPortCount();
        int portnr = -1;
        if (portname.empty()) {
            if (n > 0) portnr = 0;
        }
        else {
            for (unsigned int i = 0; i < n && portnr < 0; i++) {
                if (out->getPortName(i) == portname) portnr = (int)i;
            }
            for (unsigned int i = 0; i < n && portnr < 0; i++) {
                if (out->getPortName(i).find(portname) != std::string::npos) portnr = (int)i;
            }
        }
        if (portnr < 0) {
            delete out;
            return false;
        }
        out->openPort((unsigned int)portnr);
        if (!out->isPortOpen()) {
            delete out;
            return false;
        }
    }
    catch (RtMidiError &) {
        delete out;
        return false;
    }

    impl->out = out;
    impl->quit = false;
    impl->active = true;
    impl->thread = std::thread(&MidiClock::Impl::output_loop, impl.get(), source, quantum);
    return true;
}

void MidiClock::stop_output() {
    impl->quit = true;
    if (impl->thread.joinable()) impl->thread.join();  // sends Stop on its way out
    impl->active = false;
    if (impl->out) {
        try {
            delete impl->out;
        }
        catch (RtMidiError &) {}
        impl->out = nullptr;
    }
}

bool MidiClock::output_active() const {
    return impl->active;
}
