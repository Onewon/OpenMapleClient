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
#include "../UIElement.h"
#include <vector>
namespace ms {
class UICashShop : public UIElement {
public:
    static constexpr Type TYPE=UIElement::Type::CASHSHOP;
    static constexpr bool FOCUSED=true, TOGGLED=false;
    UICashShop();
    void draw(float inter) const override;
    void update() override;
    Button::State button_pressed(uint16_t id) override;
    UIElement::Type get_type() const override { return TYPE; }
    void request_exit();
    Cursor::State send_cursor(bool down,Point<int16_t> pos) override;
private:
    void refresh();
    Sprite preview_,item_base_,item_line_;
    std::array<Sprite,10> sidebar_;
    bool pointer_down_=false;
    std::vector<int> visible_;
    int category_=0, page_=0, selected_=0, currency_=1;
    unsigned revision_=0;
};
}