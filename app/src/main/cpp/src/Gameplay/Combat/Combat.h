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

#include "../../Character/Player.h"
#include "../../Template/TimedQueue.h"
#include "../MapleMap/MapChars.h"
#include "../MapleMap/MapMobs.h"
#include "../MapleMap/MapReactors.h"
#include "RegularAttack.h"
#include "../../Util/Randomizer.h"
#include <chrono>

namespace ms {
class InPacket;
class Combat {
public:
    Combat(Player &player,
           MapChars &chars,
           MapMobs &mobs,
           MapReactors &reactors);

    // Draw bullets, damage numbers etc.
    void draw(double viewx, double viewy, float alpha) const;

    // Poll attacks, damage effects, etc.
    void update();

    // Make the player use a special move
    void use_move(int32_t move_id);
    void skill_input(int32_t id, bool down);
    void clear_inputs();
    void use_meso_explosion(const SpecialMove &move);
    void handle_summon(int opcode, InPacket &recv);
    void clear_summons();
    void handle_area(int opcode, InPacket &recv);
    bool use_door(Point<int16_t> position);
    void direction(int32_t action, bool down);
    void body_pressure(int32_t oid);
    void set_combo(int32_t count);
    int32_t get_combo() const;

    // Add an attack to the attack queue.
    void push_attack(const AttackResult &attack);

    // Show a buff effect.
    void show_buff(int32_t cid, int32_t skillid, int8_t level);

    // Show a buff effect.
    void show_player_buff(int32_t skillid);

    // Show affected by buff effect
    void show_affected_by_buff(int32_t cid, int32_t skillid, int8_t level);

    // Show affected by buff effect
    void show_player_affected_by_buff(int32_t skillid);

    void give_foreign_buff(int32_t cid, int32_t skillid, int8_t level);

    void show_player_disease(int32_t skillid, int8_t level);

private:
    struct AreaState {
        int32_t skill=0,owner=0;
        Rectangle<int16_t> bounds;
        Point<int16_t> position;
        Animation animation;
        bool door=false,town=false;
    };
    std::unordered_map<int32_t,AreaState> areas_;
    std::chrono::steady_clock::time_point door_next_{};
    struct MesoVisual {Point<int16_t> position;Animation animation;std::chrono::steady_clock::time_point ready;};
    std::list<MesoVisual> meso_visuals_;
    struct SummonState {
        int32_t owner=0,oid=0,skill=0,level=0;
        uint8_t movement=0;bool can_attack=false,left=false,attacking=false;
        Point<int16_t> position;
        Animation idle,attack;
        std::chrono::steady_clock::time_point next_attack{},next_move{},next_hurt{};
    };
    void update_summons();
    void draw_summons(double x,double y,float alpha) const;
    std::unordered_map<int32_t,SummonState> summons_;
    struct DamageEffect {
        AttackUser user;
        DamageNumber number;
        int32_t damage;
        bool toleft;
        int32_t target_oid;
        int32_t move_id;
    };

    struct BulletEffect {
        DamageEffect damageeffect;
        Bullet bullet;
        Point<int16_t> target;
    };

    void apply_attack(const AttackResult &attack);

    void apply_move(const SpecialMove &move);

    std::vector<int32_t> find_closest(MapObjects *objs,
                                      Rectangle<int16_t> range,
                                      Point<int16_t> origin,
                                      uint8_t objcount,
                                      bool use_mobs) const;

    void apply_use_movement(const SpecialMove &move);

    void apply_result_movement(const SpecialMove &move,
                               const AttackResult &result);

    void apply_rush(const AttackResult &result);

    void apply_bullet_effect(const BulletEffect &effect);

    void apply_damage_effect(const DamageEffect &effect);

    void extract_effects(const Char &user,
                         const SpecialMove &move,
                         const AttackResult &result);

    std::vector<DamageNumber> place_numbers(
        int32_t oid,
        const std::vector<std::pair<int32_t, bool>> &damage_lines);

    const SpecialMove &get_move(int32_t move_id);

    Player &player_;
    MapChars &chars_;
    MapMobs &mobs_;
    MapReactors &reactors_;

    std::unordered_map<int32_t, Skill> skills_;
    RegularAttack regular_attack_;
    Randomizer randomizer_;
    bool aran_chain_ready_=false, aran_attack_buffered_=false;
    int aran_chain_stage_=0;
    Animation charge_effect_;
    int32_t held_skill_=0, charged_skill_=0, charge_ms_=0, final_attack_=0;
    std::chrono::steady_clock::time_point held_next_{},charge_start_{};
    int32_t aran_combo_=0, aran_buffered_move_=0;
    int32_t aran_last_direction_=0, aran_previous_direction_=0;
    std::chrono::steady_clock::time_point aran_direction_time_{}, aran_combo_time_{};
    int32_t aran_chain_map_=-1;
    std::chrono::steady_clock::time_point aran_chain_deadline_{};

    TimedQueue<AttackResult> attack_results_;
    TimedQueue<BulletEffect> bullet_effects_;
    TimedQueue<DamageEffect> damage_effects_;

    std::list<BulletEffect> bullets_;
    std::list<DamageNumber> damage_numbers_;
};
}  // namespace ms
