#pragma once
#include <atomic>

namespace ms::AndroidInput {
inline std::atomic<int> controls_opacity{80};
inline float opacity() { return controls_opacity.load() / 100.f; }
inline std::atomic<unsigned> state{2}; // keyboard present / controls visible
inline std::atomic<unsigned> release_generation{0};
inline std::atomic<bool> settings_requested{false};
inline std::atomic<bool> connection_lost{false};
inline bool has_keyboard() { return (state.load() & 1) != 0; }
inline void request_settings() { settings_requested.store(true); }
}
