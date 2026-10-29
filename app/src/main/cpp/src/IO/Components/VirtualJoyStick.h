// This file is part of the continued Journey MMORPG client
// Copyright (C) 2015-2024  Daniel Allendorf, Ryan Payton, Bizhou Xing
#pragma once
#include "TouchInfo.h"

namespace ms {
class VirtualJoyStick {
public:
    VirtualJoyStick(Point<int16_t> position, int16_t radius);
    void draw() const;
    void set_position(Point<int16_t> position) { position_ = position; }
    void set_geometry(Point<int16_t> position, int16_t radius) { position_ = position; radius_ = radius; }
    void update();
    void reset();
    bool set_state(TouchInfo touch_info) const;
    void bind_touch_id(int16_t touch_id) { bind_touch_id_ = touch_id; }
    int16_t get_bind_touch_id() const { return bind_touch_id_; }

private:
    void apply_direction(double dx, double dy);
    Point<int16_t> position_;
    Point<int16_t> thumb_;
    int16_t radius_;
    int16_t bind_touch_id_ = -1;
    bool left_ = false, right_ = false, up_ = false, down_ = false;
};
}  // namespace ms
