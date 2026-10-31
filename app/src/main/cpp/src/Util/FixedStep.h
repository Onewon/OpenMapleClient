#pragma once

#include <algorithm>
#include <cstdint>

namespace ms {
// Keep simulation time independent of the number of render callbacks.
class FixedStep {
public:
    explicit FixedStep(int64_t step_us) : step_us_(step_us) {}

    template<class Update>
    float advance(int64_t elapsed_us, Update update) {
        // Limit catch-up after a stall; background time must not fast-forward play.
        remainder_us_ += std::clamp<int64_t>(elapsed_us, 0, 250000);
        while (remainder_us_ >= step_us_) {
            update();
            remainder_us_ -= step_us_;
        }
        return static_cast<float>(remainder_us_) / step_us_;
    }

    void reset() { remainder_us_ = 0; }

private:
    const int64_t step_us_;
    int64_t remainder_us_ = 0;
};
} // namespace ms
