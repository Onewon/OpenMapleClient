// This file is part of the continued Journey MMORPG client
// Copyright (C) 2015-2024  Daniel Allendorf, Ryan Payton, Bizhou Xing
#include "VirtualJoyStick.h"

#include "../UI.h"
#include "../AndroidInput.h"
#include "../../Graphics/GraphicsGL.h"

namespace ms {
VirtualJoyStick::VirtualJoyStick(Point<int16_t> position, int16_t radius)
    : position_(position), thumb_(position), radius_(radius) {}

void VirtualJoyStick::draw() const {
    auto &graphics = GraphicsGL::get();
    const float opacity = AndroidInput::opacity();
    graphics.draw_circle(position_.x(), position_.y(), radius_ - 2,
                         0.10f, 0.13f, 0.18f, opacity * 0.45f);
    graphics.draw_ring(position_.x(), position_.y(), radius_, 2,
                       0.92f, 0.96f, 1.0f, opacity * 0.85f);
    graphics.draw_circle(thumb_.x(), thumb_.y(), std::max<int16_t>(12, radius_ / 3),
                         0.82f, 0.88f, 0.96f, opacity * (bind_touch_id_ >= 0 ? 0.7f : 0.425f));
    graphics.draw_ring(thumb_.x(), thumb_.y(), std::max<int16_t>(12, radius_ / 3), 1,
                       1.0f, 1.0f, 1.0f, opacity * 0.7f);
}

bool VirtualJoyStick::set_state(TouchInfo touch) const {
    const double dx = touch.relative_pos.x() - position_.x();
    const double dy = touch.relative_pos.y() - position_.y();
    return dx * dx + dy * dy <= static_cast<double>(radius_) * radius_;
}

void VirtualJoyStick::apply_direction(double dx, double dy) {
    const double dead_zone = radius_ * 0.18;
    const bool active = dx * dx + dy * dy >= dead_zone * dead_zone;
    const bool right = active && dx > std::abs(dy) * 0.45;
    const bool left = active && -dx > std::abs(dy) * 0.45;
    const bool down = active && dy > std::abs(dx) * 0.45;
    const bool up = active && -dy > std::abs(dx) * 0.45;
    auto &ui = UI::get();
    if (left_ != left) ui.send_touch_key(GLFMKeyCodeArrowLeft, left);
    if (right_ != right) ui.send_touch_key(GLFMKeyCodeArrowRight, right);
    if (up_ != up) ui.send_touch_key(GLFMKeyCodeArrowUp, up);
    if (down_ != down) ui.send_touch_key(GLFMKeyCodeArrowDown, down);
    left_ = left; right_ = right; up_ = up; down_ = down;
}

void VirtualJoyStick::update() {
    if (bind_touch_id_ < 0) return;
    const auto &touches = UI::get().get_touch_phase();
    const auto it = touches.find(bind_touch_id_);
    if (it == touches.end() || it->second.phase == GLFMTouchPhaseEnded ||
        it->second.phase == GLFMTouchPhaseCancelled) {
        const int16_t id = bind_touch_id_;
        bind_touch_id_ = -1;
        UI::get().remove_touch_phase(id);
        apply_direction(0, 0);
        thumb_ = position_;
        return;
    }
    const double dx = it->second.relative_pos.x() - position_.x();
    const double dy = it->second.relative_pos.y() - position_.y();
    const double length = std::hypot(dx, dy);
    const double max_thumb = radius_ * 0.58;
    const double scale = length > max_thumb ? max_thumb / length : 1.0;
    thumb_ = Point<int16_t>(static_cast<int16_t>(position_.x() + dx * scale),
                            static_cast<int16_t>(position_.y() + dy * scale));
    apply_direction(dx, dy);
}

void VirtualJoyStick::reset() {
    const int16_t id = bind_touch_id_;
    bind_touch_id_ = -1;
    if (id >= 0) UI::get().remove_touch_phase(id);
    apply_direction(0, 0);
    thumb_ = position_;
}
}  // namespace ms
