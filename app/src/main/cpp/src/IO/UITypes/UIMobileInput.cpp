// This file is part of the continued Journey MMORPG client
// Copyright (C) 2015-2024  Daniel Allendorf, Ryan Payton
#include "UIMobileInput.h"

#include "../UI.h"
#include "../UIStateGame.h"
#include "../../Constants.h"
#include "../../Gameplay/Stage.h"
#include "../../Graphics/Text.h"

namespace ms {
UIMobileInput::UIMobileInput() : joystick_(Point<int16_t>(90, 490), 58) {
    touch_buttons_[ButtonJump] = std::make_unique<TouchButton>(Point<int16_t>(757, 550), 25,
        TouchButton::ActionType::Jump, "JMP");
    touch_buttons_[ButtonAttack] = std::make_unique<TouchButton>(Point<int16_t>(727, 495), 40,
        TouchButton::ActionType::Attack, "ATK");
    touch_buttons_[ButtonPickup] = std::make_unique<TouchButton>(Point<int16_t>(757, 435), 23,
        TouchButton::ActionType::Pickup, "GET");
    const GLFMKeyCode keys[] = {GLFMKeyCode0, GLFMKeyCode1, GLFMKeyCode2, GLFMKeyCode3,
        GLFMKeyCode4, GLFMKeyCode5, GLFMKeyCode6};
    for (uint16_t i = 0; i < 7; ++i) {
        touch_buttons_[ButtonNormal0 + i] = std::make_unique<TouchButton>(
            Point<int16_t>(620, 480), 23, TouchButton::ActionType::Default, keys[i],
            std::to_string(i));
    }
    update_layout();
}

void UIMobileInput::draw() const {
    if (UI::get().get_state_type() != typeid(UIStateGame)) return;
    joystick_.draw();
    for (const auto &entry : touch_buttons_) entry.second->draw();
}

void UIMobileInput::reset() {
    joystick_.reset();
    for (auto &entry : touch_buttons_) entry.second->reset();
}

void UIMobileInput::update() {
    update_layout();
    if (UI::get().get_state_type() != typeid(UIStateGame)) return;
    const auto &touches = UI::get().get_touch_phase();
    for (const auto &touch : touches) {
        if (touch.second.phase != GLFMTouchPhaseBegan) continue;
        bool claimed = false;
        for (auto &entry : touch_buttons_) {
            auto &button = *entry.second;
            if (button.get_bind_touch_id() < 0 && button.set_state(touch.second)) {
                button.bind_touch_id(touch.first);
                claimed = true;
                break;
            }
        }
        if (!claimed && joystick_.get_bind_touch_id() < 0 && joystick_.set_state(touch.second))
            joystick_.bind_touch_id(touch.first);
    }
    for (auto &entry : touch_buttons_) entry.second->update();
    joystick_.update();
}

void UIMobileInput::update_layout() {
    const auto width = Constants::Constants::get().get_viewwidth();
    const auto height = Constants::Constants::get().get_viewheight();
    if (width == layout_width_ && height == layout_height_) return;
    layout_width_ = width;
    layout_height_ = height;
    const int16_t h = std::max<int16_t>(height, 480);
    const int16_t w = std::max<int16_t>(width, 760);
    joystick_.set_geometry(Point<int16_t>(92, h - 135), 56);
    touch_buttons_[ButtonAttack]->set_geometry(Point<int16_t>(w - 73, h - 150), 38);
    touch_buttons_[ButtonJump]->set_geometry(Point<int16_t>(w - 42, h - 98), 25);
    touch_buttons_[ButtonPickup]->set_geometry(Point<int16_t>(w - 42, h - 210), 23);
    const Point<int16_t> centers[] = {
        Point<int16_t>(w - 262, h - 110), Point<int16_t>(w - 215, h - 144),
        Point<int16_t>(w - 168, h - 178), Point<int16_t>(w - 121, h - 212),
        Point<int16_t>(w - 74, h - 256), Point<int16_t>(w - 272, h - 185),
        Point<int16_t>(w - 221, h - 234)
    };
    for (uint16_t i = 0; i < 7; ++i)
        touch_buttons_[ButtonNormal0 + i]->set_geometry(centers[i], 22);
}

std::map<uint16_t, std::unique_ptr<TouchButton>> &UIMobileInput::getTouchButtons() {
    return touch_buttons_;
}

VirtualJoyStick &UIMobileInput::getVirtualJoyStick() { return joystick_; }
}  // namespace ms
