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
#include "UIOptionMenu.h"

#include <nlnx/nx.hpp>

#include "../Components/MapleButton.h"
#include "../Components/TwoSpriteButton.h"
#include "../KeyAction.h"
#include "DisplayModes.h"
#include "Window.h"

namespace ms {
UIOptionMenu::UIOptionMenu() :
    UIDragElement<PosOPTIONMENU>(),
    selected_tab_(0) {
    nl::node OptionMenu = nl::nx::ui["StatusBar3.img"]["OptionMenu"];
    nl::node backgrnd = OptionMenu["backgrnd"];

    sprites_.emplace_back(backgrnd);
    sprites_.emplace_back(OptionMenu["backgrnd2"]);

    nl::node graphic = OptionMenu["graphic"];

    tab_background_[Buttons::TAB0] = graphic["layer:backgrnd"];
    tab_background_[Buttons::TAB1] = OptionMenu["sound"]["layer:backgrnd"];
    tab_background_[Buttons::TAB2] = OptionMenu["game"]["layer:backgrnd"];
    tab_background_[Buttons::TAB3] = OptionMenu["invite"]["layer:backgrnd"];
    tab_background_[Buttons::TAB4] = OptionMenu["screenshot"]["layer:backgrnd"];

    buttons_[Buttons::CANCEL] =
        std::make_unique<MapleButton>(OptionMenu["button:Cancel"]);
    buttons_[Buttons::OK] =
        std::make_unique<MapleButton>(OptionMenu["button:OK"]);
    buttons_[Buttons::UIRESET] =
        std::make_unique<MapleButton>(OptionMenu["button:UIReset"]);

    nl::node tab = OptionMenu["tab"];
    nl::node tab_disabled = tab["disabled"];
    nl::node tab_enabled = tab["enabled"];

    for (size_t i = Buttons::TAB0; i < Buttons::CANCEL; i++) {
        buttons_[i] =
            std::make_unique<TwoSpriteButton>(tab_disabled[i], tab_enabled[i]);
    }

    std::string sButtonUOL =
        graphic["combo:resolution"]["sButtonUOL"].get_string();
    std::string ctype = std::string(1, sButtonUOL.back());
    MapleComboBox::Type type =
        static_cast<MapleComboBox::Type>(std::stoi(ctype));

    std::vector<std::string> resolutions;
    uint16_t default_option = 3;
    const auto width = Setting<Width>::get().load();
    const auto height = Setting<Height>::get().load();
    for (size_t i = 0; i < DISPLAY_MODES.size(); ++i) {
        resolutions.push_back(DISPLAY_MODES[i].label());
        if (DISPLAY_MODES[i].width == width && DISPLAY_MODES[i].height == height) default_option = i;
    }

    int64_t combobox_width =
        graphic["combo:resolution"]["boxWidth"].get_integer();
    Point<int16_t> lt = Point<int16_t>(graphic["combo:resolution"]["lt"]);

    buttons_[Buttons::SELECT_RES] =
        std::make_unique<MapleComboBox>(type,
                                        resolutions,
                                        default_option,
                                        position_,
                                        lt,
                                        combobox_width);

    Point<int16_t> bg_dimensions = Texture(backgrnd).get_dimensions();

    dimension_ = bg_dimensions;
    drag_area_ = Point<int16_t>(bg_dimensions.x(), 20);

    change_tab(Buttons::TAB0);
}

void UIOptionMenu::draw(float inter) const {
    UIElement::draw_sprites(inter);

    tab_background_[selected_tab_].draw(position_);

    UIElement::draw_buttons(inter);
}

Button::State UIOptionMenu::button_pressed(uint16_t buttonid) {
    switch (buttonid) {
        case Buttons::TAB0:
        case Buttons::TAB1:
        case Buttons::TAB2:
        case Buttons::TAB3:
        case Buttons::TAB4:
            change_tab(buttonid);
            return Button::State::IDENTITY;
        case Buttons::CANCEL: deactivate(); return Button::State::NORMAL;
        case Buttons::OK:
            switch (selected_tab_) {
                case Buttons::TAB0: {
                    uint16_t selected_value =
                        buttons_[Buttons::SELECT_RES]->get_selected();

                    if (selected_value < DISPLAY_MODES.size()) {
                        const auto &mode = DISPLAY_MODES[selected_value];
                        Setting<Width>::get().save(mode.width);
                        Setting<Height>::get().save(mode.height);
                        Configuration::get().save();
                        Constants::Constants::get().set_viewwidth(mode.width);
                        Constants::Constants::get().set_viewheight(mode.height);
                    }
                } break;
                case Buttons::TAB1:
                case Buttons::TAB2:
                case Buttons::TAB3:
                case Buttons::TAB4:
                default: break;
            }

            deactivate();
            return Button::State::NORMAL;
        case Buttons::UIRESET: return Button::State::DISABLED;
        case Buttons::SELECT_RES:
            buttons_[Buttons::SELECT_RES]->toggle_pressed();
            return Button::State::NORMAL;
        default: return Button::State::DISABLED;
    }
}

Cursor::State UIOptionMenu::send_cursor(bool clicked,
                                        Point<int16_t> cursorpos) {
    Cursor::State dstate = UIDragElement::send_cursor(clicked, cursorpos);

    if (dragged_) {
        return dstate;
    }

    auto &button = buttons_[Buttons::SELECT_RES];

    if (button->is_pressed()) {
        if (button->in_combobox(cursorpos)) {
            if (Cursor::State new_state =
                    button->send_cursor(clicked, cursorpos)) {
                return new_state;
            }
        } else {
            remove_cursor();
        }
    }

    return UIElement::send_cursor(clicked, cursorpos);
}

void UIOptionMenu::send_key(int32_t keycode, bool pressed, bool escape) {
    if (pressed) {
        if (escape) {
            deactivate();
        } else if (keycode == KeyAction::Id::RETURN) {
            button_pressed(Buttons::OK);
        }
    }
}

UIElement::Type UIOptionMenu::get_type() const {
    return TYPE;
}

void UIOptionMenu::change_tab(uint16_t tabid) {
    buttons_[selected_tab_]->set_state(Button::State::NORMAL);
    buttons_[tabid]->set_state(Button::State::PRESSED);

    selected_tab_ = tabid;

    switch (tabid) {
        case Buttons::TAB0:
            buttons_[Buttons::SELECT_RES]->set_active(true);
            break;
        case Buttons::TAB1:
        case Buttons::TAB2:
        case Buttons::TAB3:
        case Buttons::TAB4:
            buttons_[Buttons::SELECT_RES]->set_active(false);
            break;
        default: break;
    }
}
}  // namespace ms