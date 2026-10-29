// This file is part of the continued Journey MMORPG client
// Copyright (C) 2015-2024  Daniel Allendorf, Ryan Payton, Bizhou Xing
#include "TouchButton.h"
#include <algorithm>
#include "../AndroidInput.h"
#include "../../Data/SkillData.h"
#include "../../Data/ItemData.h"
#include "../KeyAction.h"

#include "../UI.h"
#include "../../Graphics/GraphicsGL.h"
#include "../../Gameplay/Stage.h"

namespace ms {
TouchButton::TouchButton(Point<int16_t> center, int16_t radius, ActionType action,
                         const std::string &label)
    : TouchButton(center, radius, action, GLFMKeyCodeUnknown, label) {}

TouchButton::TouchButton(Point<int16_t> center, int16_t radius, ActionType action,
                         GLFMKeyCode key, const std::string &label)
    : position_(center), radius_(radius), bind_key_(key), action_type_(action),
      text_(Text::Font::A11B, Text::Alignment::CENTER, Color::Name::WHITE) {
    text_.change_text(label);
}

void TouchButton::draw() const {
    auto &graphics = GraphicsGL::get();
    const float opacity = AndroidInput::opacity();
    const int16_t fill_radius = std::max<int16_t>(1, radius_ - 2);
    graphics.draw_circle(position_.x(), position_.y(), fill_radius,
                         0.10f, 0.13f, 0.18f, opacity * (pressed_ ? 0.75f : 0.5f));
    graphics.draw_ring(position_.x(), position_.y(), radius_, 2,
                       0.92f, 0.96f, 1.0f, opacity * (pressed_ ? 1.f : 0.85f));
    if (bound_icon_.is_valid()) {
        const float scale = std::min(float(radius_*2-12)/std::max<int16_t>(1,bound_icon_.width()),
                                     float(radius_*2-12)/std::max<int16_t>(1,bound_icon_.height()));
        const auto origin = bound_icon_.get_origin();
        const auto pos = position_ + Point<int16_t>(int16_t((origin.x()-bound_icon_.width()/2.f)*scale),
                                                   int16_t((origin.y()-bound_icon_.height()/2.f)*scale));
        bound_icon_.draw(DrawArgument(pos, scale, scale, opacity));
    } else {
        text_.draw(DrawArgument(position_, opacity));
    }
}

bool TouchButton::set_state(TouchInfo touch) const {
    const double dx = touch.relative_pos.x() - position_.x();
    const double dy = touch.relative_pos.y() - position_.y();
    return dx * dx + dy * dy <= static_cast<double>(radius_) * radius_;
}

void TouchButton::set_pressed(bool pressed) {
    if (pressed_ == pressed) return;
    pressed_ = pressed;
    switch (action_type_) {
        case ActionType::Jump:
            Stage::get().get_player().send_action(KeyAction::Id::JUMP, pressed);
            break;
        case ActionType::Attack:
            Stage::get().send_key(KeyType::Id::ACTION, KeyAction::Id::ATTACK, pressed);
            break;
        case ActionType::Pickup:
            Stage::get().send_key(KeyType::Id::ACTION, KeyAction::Id::PICKUP, pressed);
            break;
        case ActionType::Default:
            UI::get().send_touch_key(bind_key_, pressed);
            break;
        case ActionType::Skill:
        case ActionType::Potion:
            break;
    }
}

void TouchButton::update() {
    if (action_type_ == ActionType::Default) {
        const auto mapping = UI::get().get_keyboard().get_mapping(bind_key_);
        if (mapping.action != icon_action_ || int(mapping.type) != icon_type_) {
            icon_action_ = mapping.action;
            icon_type_ = int(mapping.type);
            bound_icon_ = Texture();
            if (mapping.action > 0 && mapping.type == KeyType::SKILL)
                bound_icon_ = SkillData::get(mapping.action).get_icon(SkillData::NORMAL);
            else if (mapping.action > 0 && mapping.type == KeyType::ITEM)
                bound_icon_ = ItemData::get(mapping.action).get_icon(false);
        }
    }
    if (bind_touch_id_ < 0) return;
    const auto &touches = UI::get().get_touch_phase();
    const auto it = touches.find(bind_touch_id_);
    if (it == touches.end()) {
        reset();
        return;
    }
    const auto phase = it->second.phase;
    if (phase == GLFMTouchPhaseEnded || phase == GLFMTouchPhaseCancelled) {
        UI::get().remove_touch_phase(bind_touch_id_);
        bind_touch_id_ = -1;
        set_pressed(false);
        return;
    }
    set_pressed(set_state(it->second));
}

void TouchButton::reset() {
    const int16_t touch_id = bind_touch_id_;
    bind_touch_id_ = -1;
    if (touch_id >= 0) UI::get().remove_touch_phase(touch_id);
    set_pressed(false);
}
}  // namespace ms
