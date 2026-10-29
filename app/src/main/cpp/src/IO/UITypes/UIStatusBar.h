//	This file is part of the continued Journey MMORPG client
//	Copyright (C) 2015-2019  Daniel Allendorf, Ryan Payton
//
//	This program is free software: you can redistribute it and/or modify
//	it under the terms of the GNU Affero General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU Affero General Public License for more details.
//
//	You should have received a copy of the GNU Affero General Public License
//	along with this program.  If not, see <https://www.gnu.org/licenses/>.
#pragma once
#include "../../Character/CharStats.h"
#include "../../Graphics/Text.h"
#include "../UIElement.h"
#include "../Components/Gauge.h"
#include "../Components/Charset.h"

namespace ms {
class UIStatusBar : public UIElement {
public:
    static constexpr Type TYPE = UIElement::Type::STATUS_BAR;
    static constexpr bool FOCUSED = false;
    static constexpr bool TOGGLED = true;
    enum MenuType { MENU, SETTING, COMMUNITY, CHARACTER, EVENT };
    UIStatusBar(const CharStats &stats, uint8_t channel_count);
    void draw(float alpha) const override;
    void update() override;
    bool is_in_range(Point<int16_t> pos) const override;
    Cursor::State send_cursor(bool clicked, Point<int16_t> pos) override;
    void send_key(int32_t action, bool pressed, bool escape) override;
    UIElement::Type get_type() const override { return TYPE; }
    void toggle_qs() { toggle_menu(); }
    void toggle_menu();
    void remove_menus();
    bool is_menu_active() { return menu_active_; }
protected:
    Button::State button_pressed(uint16_t id) override;
private:
    enum Action : uint16_t {
        SHOP, BAG_QUICK, MENU_TOGGLE, INFO, STATS, EQUIP, SKILLS, BAG,
        FRIENDS, PARTY, QUESTS, WORLD, CHANNEL, DISPLAY, KEYBINDS, TOUCH,
        CHAT, LOGOUT
    };
    const CharStats &stats_;
    uint8_t channel_count_;
    int16_t width_, height_;
    bool menu_active_ = false;
    uint16_t selected_ = INFO;
    float hp_ = 1, mp_ = 1, exp_ = 0;
    Text name_text_, hp_text_, mp_text_, exp_text_, menu_title_;
    Texture status_back_, status_cover_, status_lv_, exp_back_, exp_track_;
    Texture menu_frame_[3];
    Gauge hp_bar_, mp_bar_, exp_bar_;
    Charset level_digits_, hpmp_digits_;
    Rectangle<int16_t> panel_bounds() const;
    void select(uint16_t id);
};
}
