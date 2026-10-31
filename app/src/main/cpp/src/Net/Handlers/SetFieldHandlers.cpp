#include <android/log.h>
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
#include "SetFieldHandlers.h"

#include "../../Configuration.h"
#include "../../Gameplay/Stage.h"
#include "../../IO/UI.h"
#include "../../IO/UITypes/Login/UICharSelect.h"
#include "../../IO/Window.h"
#include "../Packets/GameplayPackets.h"
#include "Helpers/CharacterParser.h"
#include "Helpers/LoginParser.h"
#include "../../Timer.h"

namespace ms {
namespace {
auto fn_player_update = []<typename... T>(T && ...args) {
    PlayerUpdatePacket(std::forward<T>(args)...).dispatch();
};
}  // namespace

void SetFieldHandler::transition(int32_t mapid, uint8_t portalid, std::optional<Point<int16_t>> position) const {
    __android_log_print(ANDROID_LOG_INFO,"MapTransition","map=%d portal=%d",mapid,portalid);
    float fadestep = 0.025f;

    Window::get().fadeout(fadestep, [mapid, portalid, position]() {
        GraphicsGL::get().clear();

        Stage::get().load(mapid, portalid);
        if(position) Stage::get().place_player(*position);

        UI::get().enable();
        Timer::get().start();
        GraphicsGL::get().unlock();

        Stage::get().transfer_player();
    });

    GraphicsGL::get().lock();
    Stage::get().clear();
    Timer::get().start();
}

void SetFieldHandler::handle(InPacket &recv) const {
    Constants::Constants::get().set_viewwidth(Setting<Width>::get().load());
    Constants::Constants::get().set_viewheight(Setting<Height>::get().load());

    int32_t channel = recv.read_int();
    int8_t mode1 = recv.read_byte();
    int8_t mode2 = recv.read_byte();

    if (mode1 == 0 && mode2 == 0) {
        change_map(recv, channel);
    } else {
        set_field(recv);
    }
}

void SetFieldHandler::change_map(InPacket &recv, int32_t) const {
    recv.skip(3);

    int32_t mapid = recv.read_int();
    int8_t portalid = recv.read_byte();

    const auto spawn=decode_field_spawn(recv);
    __android_log_print(ANDROID_LOG_INFO,"MapTransition","spawn hp=%u positioned=%d x=%d y=%d",spawn.hp,spawn.position.has_value(),spawn.position?spawn.position->x():0,spawn.position?spawn.position->y():0);
    auto &stats=Stage::get().get_player().get_stats();stats.set_stat(MapleStat::HP,spawn.hp);stats.set_mapid(mapid);stats.set_portal(portalid);
    transition(mapid, portalid, spawn.position);
}

void SetFieldHandler::set_field(InPacket &recv) const {
    recv.skip(23);

    int32_t cid = recv.read_int();
    auto charselect = UI::get().get_element<UICharSelect>();

    if (charselect) {
    const CharEntry &playerentry = charselect->get().get_character(cid);

    if (playerentry.id != cid) {
        return;
    }

    // update with stats that was loaded on server side after character
    // selection. hp, mp, maxhp, maxmp, mapid...
    charselect->get().update_character(cid, LoginParser::parse_stats(recv));

    Stage::get().load_player(playerentry,
                             Configuration::get().get_worldid(),
                             Configuration::get().get_channelid());
    } else {
        // Returning from cash shop: no character-selection UI exists anymore.
        if (Stage::get().get_player().get_cid()!=cid) return;
        const auto entry=LoginParser::parse_stats(recv);
        auto& stats=Stage::get().get_player().get_stats();
        for(const auto& pair:entry.stats) stats.set_stat(pair.first,pair.second);
        stats.set_mapid(entry.mapid);
        stats.set_portal(entry.portal);
    }

    Player &player = Stage::get().get_player();

    recv.read_byte();  // 'buddycap'

    if (recv.read_bool()) {
        recv.read_string();  // 'linkedname'
    }

    CharacterParser::parse_inventory(recv, player.get_inventory());
    CharacterParser::parse_skillbook(recv, player.get_skills());
    CharacterParser::parse_cooldowns(recv, player);
    CharacterParser::parse_questlog(recv, player.get_quests());
    CharacterParser::parse_minigame(recv);
    CharacterParser::parse_ring1(recv);
    CharacterParser::parse_ring2(recv);
    CharacterParser::parse_ring3(recv);
    CharacterParser::parse_teleportrock(recv, player.get_teleport_rock());
    CharacterParser::parse_monsterbook(recv, player.get_monsterbook());
    CharacterParser::parse_nyinfo(recv);
    CharacterParser::parse_areainfo(recv);

    player.recalc_stats(true);

    fn_player_update();

    uint8_t portalid = player.get_stats().get_portal();
    int32_t mapid = player.get_stats().get_mapid();

    transition(mapid, portalid);

    Sound(Sound::Name::GAME_START).play();

    UI::get().change_state(UI::State::GAME);
}
}  // namespace ms
