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

#include "../Gameplay/Combat/Attack.h"
#include "../OutPacket.h"
#include "../../Character/AdventurerSkills.h"

namespace ms {
// Notifies the server of an attack. The opcode is determined by the attack
// type. Attack::CLOSE = CLOSE_ATTACK(44) Attack::RANGED = RANGED_ATTACK(45)
// Attack::MAGIC = MAGIC_ATTACK(46)
class AttackPacket : public OutPacket {
public:
    AttackPacket(const AttackResult &attack) :
        OutPacket(opcodefor(attack.type)) {
        skip(1);

        write_byte((attack.mobcount << 4) | attack.hitcount);
        write_int(attack.skill);

        if (adventurer::charge(attack.skill) && attack.skill!=3221001) {
            write_int(std::max(1,attack.charge));
        }

        skip(8);

        write_byte(attack.display);
        write_byte(attack.toleft);
        write_byte(attack.stance);

        skip(1);

        write_byte(attack.speed);

        if (attack.type == Attack::Type::RANGED) {
            skip(1);
            write_byte(attack.toleft);
            skip(7);
            if(adventurer::hold(attack.skill) || attack.skill==3221001) write_int(attack.charge);
        } else {
            skip(4);
        }

        for (const auto &damage_to_mob : attack.damage_lines) {
            write_int(damage_to_mob.first);

            skip(14);

            for (const auto &single_damage : damage_to_mob.second) {
                write_int(single_damage.second ? (static_cast<uint32_t>(single_damage.first) | 0x80000000u) : single_damage.first);
            }

            // The paired v83 server consumes this trailer for every target, including Rapid Fire.
            skip(4);
        }
    }

private:
    static OutPacket::Opcode opcodefor(Attack::Type type) {
        switch (type) {
            case Attack::Type::CLOSE: return OutPacket::Opcode::CLOSE_ATTACK;
            case Attack::Type::RANGED: return OutPacket::Opcode::RANGED_ATTACK;
            default: return OutPacket::Opcode::MAGIC_ATTACK;
        }
    }
};

// Meso Explosion uses byte-sized per-target hit counts and a separate consumed-drop list.
class MesoExplosionPacket:public OutPacket {
public:MesoExplosionPacket(const AttackResult &attack,const std::vector<int32_t> &coins):OutPacket(Opcode::CLOSE_ATTACK) {
    skip(1);write_byte((attack.mobcount<<4)|(attack.mobcount?1:0));write_int(4211006);skip(8);
    write_byte(attack.display);write_byte(attack.toleft);write_byte(attack.stance);
    if(!attack.mobcount)skip(10);
    else {
        skip(6);
        for(const auto &[oid,lines]:attack.damage_lines) {
            write_int(oid);skip(12);write_byte(lines.size());for(const auto &line:lines)write_int(line.first);skip(4);
        }
        write_int(0);
    }
    write_byte(coins.size());for(int32_t oid:coins){write_int(oid);write_byte(0);}
}
};

// Tells the server that the player took damage.
// Opcode: TAKE_DAMAGE(48)
class TakeDamagePacket : public OutPacket {
public:
    enum From : int8_t { TOUCH = -1 };

    TakeDamagePacket(int8_t from,
                     uint8_t element,
                     int32_t damage,
                     int32_t mobid,
                     int32_t oid,
                     uint8_t direction) :
        OutPacket(OutPacket::Opcode::TAKE_DAMAGE) {
        write_time();
        write_byte(from);
        write_byte(element);
        write_int(damage);
        write_int(mobid);
        write_int(oid);
        write_byte(direction);
    }

    // From mob attack result.
    TakeDamagePacket(const MobAttackResult &result, From from) :
        TakeDamagePacket(from,
                         0,
                         result.reported_damage,
                         result.mobid,
                         result.oid,
                         result.direction) {
        if(result.guarded) {
            write_byte(0);write_byte(2);write_byte(0);write_int(result.oid);write_byte(0);
            write_point(result.mob_position);write_point(result.player_position);
        }
    }
};

class CancelSkillPacket : public OutPacket {
public:explicit CancelSkillPacket(int32_t skill):OutPacket(static_cast<Opcode>(92)){write_int(skill);}
};
class HealOverTimePacket : public OutPacket {
public: HealOverTimePacket(int16_t hp,int16_t mp):OutPacket(static_cast<Opcode>(89)) {skip(8);write_short(hp);write_short(mp);}
};
// Packet which notifies the server of a skill usage.
// Opcode: USE_SKILL(91)
class UseSkillPacket : public OutPacket {
public:
    UseSkillPacket(int32_t skillid, int32_t level, Point<int16_t> position={}, const std::vector<int32_t>& targets={}, bool left=false) :
        OutPacket(OutPacket::Opcode::USE_SKILL) {
        write_time();
        write_int(skillid);
        write_byte(static_cast<uint8_t>(level));

        if(adventurer::magnet(skillid)) {
            write_int(targets.size());
            for(int32_t oid:targets) {write_int(oid);write_byte(1);}
            write_byte(left);return;
        }

        if (skillid % 10000000 == 1004) {
            skip(2);  // TODO: No idea what this could be
        }

        write_point(position);write_byte(0);
    }
};
}  // namespace ms