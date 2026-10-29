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
#include "UIStatusBar.h"
#include <algorithm>
#include <nlnx/nx.hpp>
#include <iomanip>
#include <sstream>
#include "ExpTable.h"
#include "Stage.h"
#include "GameplayPackets.h"
#include "HudButton.h"
#include "UI.h"
#include "AndroidInput.h"
#include "UIChannel.h"
#include "UICharInfo.h"
#include "UIChatBar.h"
#include "UIEquipInventory.h"
#include "UIItemInventory.h"
#include "UIKeyConfig.h"
#include "UIOptionMenu.h"
#include "UIQuestLog.h"
#include "UIQuit.h"
#include "UISkillBook.h"
#include "UIStatsInfo.h"
#include "UIUserList.h"
#include "UIWorldMap.h"

namespace ms {
UIStatusBar::UIStatusBar(const CharStats &stats, uint8_t channels) :
    stats_(stats), channel_count_(channels),
    width_(Constants::Constants::get().get_viewwidth()),
    height_(Constants::Constants::get().get_viewheight()),
    name_text_(Text::A13B, Text::LEFT, Color::WHITE),
    hp_text_(Text::A11M, Text::LEFT, Color::WHITE),
    mp_text_(Text::A11M, Text::LEFT, Color::WHITE),
    exp_text_(Text::A11M, Text::RIGHT, Color::WHITE),
    menu_title_(Text::A15B, Text::LEFT, Color::WHITE, "MENU") {
    auto main = nl::nx::ui["StatusBar3.img"]["mainBar"];
    auto status = main["status"];
    for (int i=0;i<3;++i) menu_frame_[i] = Texture(main["submenu"]["backgrnd"][std::to_string(i)]);
    status_back_ = Texture(status["backgrnd"]);
    status_cover_ = Texture(status["layer:cover"]);
    status_lv_ = Texture(status["layer:Lv"]);
    hp_bar_ = Gauge(Gauge::GAME, status.resolve("gauge/hp/layer:0"), 169, 0.f);
    mp_bar_ = Gauge(Gauge::GAME, status.resolve("gauge/mp/layer:0"), 169, 0.f);
    level_digits_ = Charset(status["lvNumber"], Charset::LEFT);
    hpmp_digits_ = Charset(status["gauge"]["number"], Charset::RIGHT);
    auto exp = main["EXPBar"];
    // The 1024 artwork works at every viewport width when stretched explicitly.
    exp_back_ = Texture(exp["backgrnd"]);
    exp_track_ = Texture(exp["1024"]["layer:back"]);
    exp_bar_ = Gauge(Gauge::GAME, exp["1024"].resolve("layer:gauge"), width_-16, 0.f);
    buttons_[SHOP] = std::make_unique<HudButton>(Point<int16_t>(width_-224,12), Point<int16_t>(64,58), "Shop", HudButton::SHOP);
    buttons_[BAG_QUICK] = std::make_unique<HudButton>(Point<int16_t>(width_-152,12), Point<int16_t>(64,58), "Bag", HudButton::BAG);
    buttons_[MENU_TOGGLE] = std::make_unique<HudButton>(Point<int16_t>(width_-80,12), Point<int16_t>(64,58), "Menu", HudButton::MENU);
    const char *labels[] = {"Character", "Stats", "Equip", "Skills", "Bag", "Friends", "Party", "Quests", "World", "Channel", "Display", "Keybinds", "Touch", "Chat", "Logout"};
    const HudButton::Icon icons[] = {HudButton::PERSON,HudButton::STATS,HudButton::EQUIP,HudButton::SKILL,HudButton::BAG,HudButton::PEOPLE,HudButton::PEOPLE,HudButton::QUEST,HudButton::WORLD,HudButton::CHANNEL,HudButton::DISPLAY,HudButton::KEYBOARD,HudButton::TOUCH,HudButton::CHAT,HudButton::EXIT};
    for (uint16_t i=0;i<15;++i) {
        buttons_[INFO+i] = std::make_unique<HudButton>(Point<int16_t>(width_-318+(i%3)*100,117+(i/3)*62),Point<int16_t>(90,52),labels[i],icons[i]);
        buttons_[INFO+i]->set_active(false);
    }
    update();
}

Rectangle<int16_t> UIStatusBar::panel_bounds() const {
    return {Point<int16_t>(width_-328,82), Point<int16_t>(width_-18,436)};
}

void UIStatusBar::draw(float) const {
    auto &g = GraphicsGL::get();
    if (menu_active_) g.draw_screen_fill(0,0,0,.26f);
    const Point<int16_t> status_pos(17,16);
    status_back_.draw(status_pos);
    hp_bar_.draw(status_pos);
    mp_bar_.draw(status_pos);
    status_cover_.draw(status_pos);
    status_lv_.draw(status_pos);
    level_digits_.draw(std::to_string(stats_.get_stat(MapleStat::LEVEL)), status_pos + Point<int16_t>(49,8));
    name_text_.draw(status_pos + Point<int16_t>(77,0));
    hpmp_digits_.draw(hp_text_.get_text(), status_pos + Point<int16_t>(134,30));
    hpmp_digits_.draw(mp_text_.get_text(), status_pos + Point<int16_t>(130,46));
    const Point<int16_t> exp_pos(0,height_-33);
    exp_back_.draw(DrawArgument(Point<int16_t>(0,height_-24), Point<int16_t>(width_,24)));
    exp_track_.draw(DrawArgument(exp_pos, Point<int16_t>(width_,0)));
    exp_bar_.draw(exp_pos);
    exp_text_.draw(Point<int16_t>(width_-18,height_-26));
    if (menu_active_) {
        // Stretch the original three-piece menu frame, retaining its top/bottom trim.
        const int16_t heights[] = {29,281,44};
        int16_t y = 82;
        for (int i=0;i<3;++i) {
            menu_frame_[i].draw(DrawArgument(Point<int16_t>(width_-328,y) + menu_frame_[i].get_origin(),
                                              Point<int16_t>(310,heights[i])));
            y += heights[i];
        }
        menu_title_.draw(Point<int16_t>(width_-316,91));
    }
    UIElement::draw_buttons(1);
}

void UIStatusBar::update() {
    const int level = stats_.get_stat(MapleStat::LEVEL);
    const int hp = stats_.get_stat(MapleStat::HP), mp = stats_.get_stat(MapleStat::MP);
    const int maxhp = std::max(1, stats_.get_total(EquipStat::HP));
    const int maxmp = std::max(1, stats_.get_total(EquipStat::MP));
    hp_ = std::clamp(float(hp)/maxhp,0.f,1.f);
    mp_ = std::clamp(float(mp)/maxmp,0.f,1.f);
    exp_ = level>0 && level<int(ExpTable::LEVELCAP) && ExpTable::values[level]>0 ?
        std::clamp(float(double(stats_.get_exp())/ExpTable::values[level]),0.f,1.f) : 0;
    name_text_.change_text(stats_.get_name());
    hp_text_.change_text("[" + std::to_string(hp) + "/" + std::to_string(maxhp) + "]");
    mp_text_.change_text("[" + std::to_string(mp) + "/" + std::to_string(maxmp) + "]");
    std::ostringstream exp; exp << "EXP " << std::fixed << std::setprecision(2) << exp_*100 << "%";
    exp_text_.change_text(exp.str());
    hp_bar_.update(hp_);
    mp_bar_.update(mp_);
    exp_bar_.update(exp_);
    UIElement::update();
}

bool UIStatusBar::is_in_range(Point<int16_t> p) const {
    if (menu_active_) return true;
    if (Rectangle<int16_t>(Point<int16_t>(16,16),Point<int16_t>(220,86)).contains(p)) return true;
    for (const auto &entry:buttons_) if (entry.second->is_active() && entry.second->bounds(position_).contains(p)) return true;
    return false;
}

Cursor::State UIStatusBar::send_cursor(bool clicked, Point<int16_t> p) {
    if (menu_active_ && clicked && !panel_bounds().contains(p) &&
        !buttons_[SHOP]->bounds(position_).contains(p) &&
        !buttons_[BAG_QUICK]->bounds(position_).contains(p) &&
        !buttons_[MENU_TOGGLE]->bounds(position_).contains(p)) {
        remove_menus(); return Cursor::CLICKING;
    }
    if (!menu_active_ && clicked && Rectangle<int16_t>(Point<int16_t>(16,16),Point<int16_t>(220,86)).contains(p)) {
        button_pressed(INFO); return Cursor::CLICKING;
    }
    return UIElement::send_cursor(clicked,p);
}

void UIStatusBar::select(uint16_t id) {
    selected_=id;
    for (uint16_t i=INFO;i<=LOGOUT;++i) buttons_[i]->set_state(i==selected_ ? Button::MOUSEOVER : Button::NORMAL);
}
void UIStatusBar::toggle_menu() {
    menu_active_=!menu_active_;
    for(uint16_t i=INFO;i<=LOGOUT;++i) buttons_[i]->set_active(menu_active_);
    if(menu_active_) select(INFO);
}
void UIStatusBar::remove_menus() {
    menu_active_=false;
    for(uint16_t i=INFO;i<=LOGOUT;++i) buttons_[i]->set_active(false);
}
void UIStatusBar::send_key(int32_t action, bool pressed, bool escape) {
    if(!pressed) return;
    if(escape) { toggle_menu(); return; }
    if(!menu_active_) return;
    if(action==KeyAction::RETURN) { button_pressed(selected_); return; }
    int delta=0;
    if(action==KeyAction::LEFT) delta=-1;
    if(action==KeyAction::RIGHT) delta=1;
    if(action==KeyAction::UP) delta=-3;
    if(action==KeyAction::DOWN) delta=3;
    if(delta) select(INFO+(int(selected_)-INFO+delta+15)%15);
}
Button::State UIStatusBar::button_pressed(uint16_t id) {
    if(id==MENU_TOGGLE) { toggle_menu(); return Button::NORMAL; }
    remove_menus();
    auto &stage=Stage::get();
    switch(id) {
        case SHOP: EnterCashShopPacket().dispatch(); break;
        case BAG_QUICK: case BAG: UI::get().emplace<UIItemInventory>(stage.get_player().get_inventory()); break;
        case INFO: UI::get().emplace<UICharInfo>(stage.get_player().get_oid()); break;
        case STATS: UI::get().emplace<UIStatsInfo>(stats_); break;
        case EQUIP: UI::get().emplace<UIEquipInventory>(stage.get_player().get_inventory()); break;
        case SKILLS: UI::get().emplace<UISkillBook>(stats_,stage.get_player().get_skills()); break;
        case FRIENDS: case PARTY: {
            const auto tab=id==FRIENDS ? UIUserList::FRIEND : UIUserList::PARTY;
            auto list=UI::get().get_element<UIUserList>();
            if(list) { list->get().change_tab(tab); list->get().makeactive(); }
            else UI::get().emplace<UIUserList>(tab);
            break;
        }
        case QUESTS: UI::get().emplace<UIQuestLog>(stage.get_player().get_quests()); break;
        case WORLD: UI::get().emplace<UIWorldMap>(); break;
        case CHANNEL: UI::get().emplace<UIChannel>(stage.get_player().get_world_id(),stage.get_player().get_channel_id(),channel_count_); break;
        case DISPLAY: UI::get().emplace<UIOptionMenu>(); break;
        case KEYBINDS: UI::get().emplace<UIKeyConfig>(stage.get_player().get_inventory(),stage.get_player().get_skills()); break;
        case TOUCH: AndroidInput::request_settings(); break;
        case CHAT: if(auto chat=UI::get().get_element<UIChatBar>()) chat->get().toggle_chatfield(); break;
        case LOGOUT: UI::get().emplace<UIQuit>(stats_); break;
    }
    return Button::NORMAL;
}
}
