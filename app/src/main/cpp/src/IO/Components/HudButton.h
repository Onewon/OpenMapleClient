#pragma once

#include "Button.h"
#include "Texture.h"
#include "Text.h"
#include <nlnx/nx.hpp>
#include <algorithm>

namespace ms {
// Reuse the original status-bar artwork, normalizing its embedded offsets.
class HudButton : public Button {
public:
    enum Icon { SHOP, BAG, MENU, PERSON, STATS, EQUIP, SKILL, PEOPLE, QUEST,
                WORLD, CHANNEL, DISPLAY, KEYBOARD, TOUCH, CHAT, EXIT };
    HudButton(Point<int16_t> pos, Point<int16_t> size, std::string label, Icon icon) :
        size_(size), label_(Text::A11B, Text::CENTER, Color::WHITE, label) {
        position_ = pos;
        active_ = true;
        pressed_ = false;
        state_ = NORMAL;
        const char *art = icon == SHOP ? "button:CashShop" :
            icon == MENU || icon == BAG || icon == EQUIP ? "button:Menu" :
            icon == PEOPLE || icon == CHAT ? "button:Community" :
            icon == QUEST ? "button:Event" :
            icon == PERSON || icon == STATS || icon == SKILL ? "button:Character" : "button:Setting";
        auto source = nl::nx::ui["StatusBar3.img"]["mainBar"]["menu"][art];
        const char *states[] = {"normal", "pressed", "mouseOver", "disabled"};
        textures_[NORMAL] = Texture(source[states[0]]["0"]);
        textures_[PRESSED] = Texture(source[states[1]]["0"]);
        textures_[MOUSEOVER] = Texture(source[states[2]]["0"]);
        textures_[DISABLED] = Texture(source[states[3]]["0"]);
    }
    void draw(Point<int16_t> parent) const override {
        if (!active_) return;
        const auto p = parent + position_;
        const auto &texture = textures_[state_].is_valid() ? textures_[state_] : textures_[NORMAL];
        const float scale = std::min(1.f, float(size_.y()-15)/std::max<int16_t>(1, texture.height()));
        const auto top = p + Point<int16_t>((size_.x()-int(texture.width()*scale))/2, 0);
        texture.draw(DrawArgument(top + Point<int16_t>(int16_t(texture.get_origin().x()*scale), int16_t(texture.get_origin().y()*scale)), scale, scale));
        label_.draw(p + Point<int16_t>(size_.x()/2, size_.y()-15));
    }
    void update() override {}
    Rectangle<int16_t> bounds(Point<int16_t> parent) const override {
        return {parent+position_, parent+position_+size_};
    }
    int16_t width() const override { return size_.x(); }
    Point<int16_t> origin() const override { return {}; }
    Cursor::State send_cursor(bool, Point<int16_t>) override { return Cursor::IDLE; }
private:
    Point<int16_t> size_;
    Texture textures_[4];
    Text label_;
};
}
