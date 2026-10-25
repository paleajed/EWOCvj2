// Keep this file free of program.h: asio (via Link.hpp) has to be the first to include winsock2.h/windows.h.
#include <ableton/Link.hpp>

#include <atomic>
#include <chrono>

#include "LinkManager.h"

struct LinkManager::Impl {
    Impl() : link(120.0) {}

    ableton::Link link;
    std::atomic<double> quantum{4.0};
    std::atomic<double> offsetsecs{0.0};
};

LinkManager::LinkManager() : impl(new Impl) {
}

LinkManager::~LinkManager() {
    impl->link.enable(false);
}

void LinkManager::enable(bool on) {
    impl->link.enable(on);
}

bool LinkManager::enabled() const {
    return impl->link.isEnabled();
}

void LinkManager::set_quantum(double quantum) {
    if (quantum < 1.0) quantum = 1.0;
    impl->quantum = quantum;
}

void LinkManager::set_offset_ms(double ms) {
    impl->offsetsecs = ms / 1000.0;
}

bool LinkManager::get_state(LinkState &state) const {
    if (!impl->link.isEnabled()) {
        state.valid = false;
        return false;
    }
    // app thread session state: safe to call from any non-realtime thread
    const auto sessionstate = impl->link.captureAppSessionState();
    const auto time = impl->link.clock().micros() +
                      std::chrono::microseconds((long long) (impl->offsetsecs.load() * 1.0e6));
    const double quantum = impl->quantum;

    state.valid = true;
    state.quantum = quantum;
    state.bpm = sessionstate.tempo();
    state.beat = sessionstate.beatAtTime(time, quantum);
    state.phase = sessionstate.phaseAtTime(time, quantum);
    state.peers = impl->link.numPeers();
    return true;
}

void LinkManager::set_tempo(double bpm) {
    if (bpm < 20.0) bpm = 20.0;
    if (bpm > 999.0) bpm = 999.0;
    auto sessionstate = impl->link.captureAppSessionState();
    sessionstate.setTempo(bpm, impl->link.clock().micros());
    impl->link.commitAppSessionState(sessionstate);
}
