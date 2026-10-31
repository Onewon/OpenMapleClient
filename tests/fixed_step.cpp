#include "../app/src/main/cpp/src/Util/FixedStep.h"
#include <cassert>
#include <cstdint>
#include <iostream>

int main() {
    constexpr int64_t second = 1000000;
    for (int fps : {30, 60, 90, 120, 144, 240}) {
        ms::FixedStep clock(8000);
        int ticks = 0;
        double position = 0;
        int64_t previous = 0;
        for (int frame = 1; frame <= fps * 10; ++frame) {
            const int64_t now = int64_t(frame) * second / fps;
            const float alpha = clock.advance(now - previous, [&] {
                ++ticks;
                position += 1.0; // same movement per fixed physics tick
            });
            assert(alpha >= 0 && alpha < 1);
            previous = now;
        }
        assert(ticks == 1250 && position == 1250);
        std::cout << fps << " FPS: " << ticks << " ticks in 10 seconds\n";
    }
    // Uneven render pacing must preserve the same total simulation time.
    {
        ms::FixedStep jittered(8000);
        int ticks = 0;
        for (int i = 0; i < 1000; ++i)
            for (int64_t elapsed : {3000, 21000, 7000, 9000})
                jittered.advance(elapsed, [&] { ++ticks; });
        assert(ticks == 5000);
    }
    ms::FixedStep clock(8000);
    int ticks = 0;
    auto update = [&] { ++ticks; };
    clock.advance(4000, update);
    clock.advance(4000, update);
    assert(ticks == 1); // retain the fraction between frames
    clock.advance(-1000, update);
    assert(ticks == 1);
    clock.advance(4000, update);
    clock.reset(); // focus transition discards the previous remainder
    clock.advance(4000, update);
    assert(ticks == 1);
    clock.reset();
    clock.advance(10000000, update);
    assert(ticks == 32); // at most 250ms of catch-up, not ten seconds
    std::cout << "Fractional frames, reset and stall checks passed\n";
}
