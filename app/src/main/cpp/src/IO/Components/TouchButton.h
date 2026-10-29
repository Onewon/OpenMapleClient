// This file is part of the continued Journey MMORPG client
// Copyright (C) 2015-2024  Daniel Allendorf, Ryan Payton, Bizhou Xing
// Distributed under the GNU Affero General Public License, version 3 or later.
#pragma once

#include "TouchInfo.h"
#include "../../Graphics/Text.h"
#include "glfm.h"
#include "Texture.h"

namespace ms {
class TouchButton {
public:
    enum ActionType : uint16_t { Jump, Attack, Pickup, Skill, Potion, Default };

    TouchButton(Point<int16_t> center, int16_t radius, ActionType action_type,
                const std::string &text);
    TouchButton(Point<int16_t> center, int16_t radius, ActionType action_type,
                GLFMKeyCode bind_key, const std::string &text);

    void draw() const;
    void set_position(Point<int16_t> center) { position_ = center; }
    void set_geometry(Point<int16_t> center, int16_t radius) { position_ = center; radius_ = radius; }
    void update();
    void reset();
    void bind_touch_id(int16_t touch_id) { bind_touch_id_ = touch_id; }
    bool set_state(TouchInfo touch_info) const;
    int16_t get_bind_touch_id() const { return bind_touch_id_; }

private:
    void set_pressed(bool pressed);
    Point<int16_t> position_;
    int16_t radius_;
    GLFMKeyCode bind_key_;
    ActionType action_type_;
    Text text_;
    int16_t bind_touch_id_ = -1;
    bool pressed_ = false;
    Texture bound_icon_;
    int32_t icon_action_ = -1;
    int icon_type_ = -1;
};
}  // namespace ms
