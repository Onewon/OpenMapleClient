#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace ms {
struct DisplayMode {
    int16_t width;
    int16_t height;
    const char *aspect;

    std::string label() const {
        return std::to_string(width) + " x " + std::to_string(height) + " (" + aspect + ")";
    }
};

// Logical game resolutions, independent of the panel's physical pixel count.
inline constexpr std::array<DisplayMode, 13> DISPLAY_MODES{{
    {800, 600, "4:3"}, {1024, 768, "4:3"},
    {1280, 720, "16:9"}, {1366, 768, "16:9"},
    {1920, 1080, "16:9"}, {1920, 1200, "16:10"},
    {1280, 800, "16:10"}, {1600, 900, "16:9"},
    {1600, 1000, "16:10"}, {1440, 1080, "4:3"},
    {1600, 1200, "4:3"}, {1560, 720, "19.5:9"},
    {1600, 720, "20:9"}
}};
}
