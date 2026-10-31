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
#include "Player.h"
#include "AdventurerSkills.h"
#include <nlnx/nx.hpp>
#include "../Gameplay/Stage.h"

#include "../Data/WeaponData.h"
#include "../IO/UI.h"
#include "../IO/UITypes/UIStatsInfo.h"
#include "../Net/Packets/GameplayPackets.h"
#include "../Net/Packets/AttackAndSkillPackets.h"
#include "../Net/Packets/InventoryPackets.h"
#include "PlayerStates.h"

namespace ms {
namespace {
auto fn_move_player = [](auto&&... args) {
    MovePlayerPacket(std::forward<decltype(args)>(args)...).dispatch();
};

auto fn_use_item = [](auto&&... args) {
    UseItemPacket(std::forward<decltype(args)>(args)...).dispatch();
};
}  // namespace

const PlayerNullState nullstate;

const PlayerState *get_state(Char::State state) {
    static PlayerStandState standing;
    static PlayerWalkState walking;
    static PlayerFallState falling;
    static PlayerProneState lying;
    static PlayerClimbState climbing;
    static PlayerSitState sitting;
    static PlayerFlyState flying;

    switch (state) {
        case Char::State::STAND: return &standing;
        case Char::State::WALK: return &walking;
        case Char::State::FALL: return &falling;
        case Char::State::PRONE: return &lying;
        case Char::State::LADDER:
        case Char::State::ROPE: return &climbing;
        case Char::State::SIT: return &sitting;
        case Char::State::SWIM: return &flying;
        default: return nullptr;
    }
}

Player::Player(const CharEntry &entry, uint8_t wid, uint8_t channel_id) :
    Char(entry.id, entry.look, entry.stats.name),
    stats_(entry.stats),
    cid_(entry.id),
    wid_(wid),
    channel_id_(channel_id) {
    attacking_ = false;
    underwater_ = false;

    set_state(Char::State::STAND);
    set_direction(true);
}

Player::Player() : Char(0, {}, "") {}

void Player::respawn(Point<int16_t> pos, bool uw) {
    set_position(pos.x(), pos.y());
    underwater_ = uw;
    keys_down_.clear();
    attacking_ = false;
    ladder_ = {};
    nullstate.update_state(*this);
}

void Player::send_action(KeyAction::Id action, bool down) {
    const PlayerState *pst = get_state(state_);

    if (pst) {
        pst->send_action(*this, action, down);
    }

    keys_down_[action] = down;
}

void Player::recalc_stats(bool equipchanged) {
    Weapon::Type weapontype = get_weapontype();

    stats_.set_weapontype(weapontype);
    stats_.init_totalstats();

    if (equipchanged) {
        inventory_.recalc_stats(weapontype);
    }

    for (auto stat : EquipStat::values) {
        int32_t inventory_total = inventory_.get_stat(stat);
        stats_.add_value(stat, inventory_total);
    }

    auto passive_skills = skill_book_.collect_passives();

    for (auto &[skill_id, skill_level] : passive_skills) {
        passive_buffs_.apply_buff(stats_, skill_id, skill_level);
    }

    for (const Buff &buff : buffs_.values()) {
        active_buffs_.apply_buff(stats_, buff.stat, buff.value);
    }

    if (stats_.get_job().get_id()/100==21 && weapontype==Weapon::POLEARM) {
        auto levelnode=[&](int32_t id) {
            return nl::nx::skill[std::to_string(id/10000)+".img"]["skill"][std::to_string(id)]["level"][get_skilllevel(id)];
        };
        if(get_skilllevel(21100000)>0) {
            auto level=levelnode(21100000);
            stats_.set_mastery_total(0.1f+0.05f*level["mastery"].get_integer());
            stats_.add_value(EquipStat::ACC,level["x"]);
        }
        if(get_skilllevel(21120001)>0) {
            auto level=levelnode(21120001);
            stats_.set_mastery_total(0.1f+0.05f*level["mastery"].get_integer());
            stats_.add_value(EquipStat::WATK,level["x"]);
        }
        const auto groups=std::min(Stage::get().get_combat().get_combo()/10,get_skilllevel(21000000));
        stats_.add_value(EquipStat::WATK,groups);
        stats_.add_value(EquipStat::WDEF,groups*5);
        stats_.add_value(EquipStat::MDEF,groups*5);
        float retained=1.0f;
        if(get_skilllevel(21120004)>0) retained*=levelnode(21120004)["x"].get_integer(1000)/1000.0f;
        if(has_buff(Buffstat::COMBO_BARRIER)) retained*=buffs_[Buffstat::COMBO_BARRIER].value/1000.0f;
        stats_.set_reducedamage(1.0f-retained);
    }
    auto node=[&](int32_t id) {return nl::nx::skill[std::to_string(id/10000)+".img"]["skill"][std::to_string(id)]["level"][get_skilllevel(id)];};
    const auto job=stats_.get_job().get_id();
    if(job>=100 && job<=522) {
        if(get_skilllevel(3000000)>0) stats_.add_value(EquipStat::ACC,node(3000000)["x"]);
        for(int id:{4000000,5000000}) if(get_skilllevel(id)>0) {
            stats_.add_value(EquipStat::ACC,node(id)["x"]);stats_.add_value(EquipStat::AVOID,node(id)["y"]);
        }
        for(int id:{3110000,3210000}) if(get_skilllevel(id)>0) stats_.add_value(EquipStat::SPEED,node(id)["speed"]);
        if((weapontype==Weapon::BOW || weapontype==Weapon::CROSSBOW) && get_skilllevel(3000002)>0)
            stats_.add_projectile_range(node(3000002)["range"]);
        if(weapontype==Weapon::CLAW && get_skilllevel(4000001)>0) stats_.add_projectile_range(node(4000001)["range"]);
        for(int id:{3120005,3220004}) if(get_skilllevel(id)>0 && ((id==3120005 && weapontype==Weapon::BOW)||(id==3220004 && weapontype==Weapon::CROSSBOW))) {
            stats_.set_mastery_total(0.1f+0.05f*node(id)["mastery"].get_integer());stats_.add_value(EquipStat::WATK,node(id)["x"]);
        }
        if(auto shield=inventory_.get_equip(InventoryType::EQUIPPED,-10)) {
            for(int id:{1110001,1210001,4210000}) if(get_skilllevel(id)>0)
                stats_.add_value(EquipStat::WDEF,shield->get().get_stat(EquipStat::WDEF)*node(id)["x"].get_integer()/100);
        }
    }
    stats_.close_totalstats();

    if (auto statsinfo = UI::get().get_element<UIStatsInfo>()) {
        statsinfo->get().update_all_stats();
    }
}

void Player::change_equip(int16_t slot) {
    if (int32_t itemid =
            inventory_.get_item_id(InventoryType::Id::EQUIPPED, slot)) {
        look_.add_equip(itemid);
    } else {
        look_.remove_equip(EquipSlot::by_id(slot));
    }
}

void Player::use_item(int32_t itemid) {
    InventoryType::Id type = InventoryType::by_item_id(itemid);

    if (int16_t slot = inventory_.find_item(type, itemid)) {
        if (type == InventoryType::Id::USE) {
            fn_use_item(slot, itemid);
        }
    }
}

void Player::draw(Layer::Id layer,
                  double viewx,
                  double viewy,
                  float alpha) const {
    if (layer == get_layer()) {
        Char::draw(viewx, viewy, alpha);
    }
}

int8_t Player::update(const Physics &physics) {
    if(phobj_.onground) flash_jumped_=false;
    const auto now=std::chrono::steady_clock::now();
    if(recovery_at_==std::chrono::steady_clock::time_point{} || phobj_.mobile() || attacking_) recovery_at_=now+std::chrono::seconds(10);
    if(now>=recovery_at_) {
        recovery_at_=now+std::chrono::seconds(10);
        const bool endure=get_skilllevel(1000002)>0 || get_skilllevel(4100002)>0 || get_skilllevel(4200001)>0;
        if(stats_.get_stat(MapleStat::HP)>0 && (!is_climbing() || endure)) {
            int hp=10,mp=3;
            for(int id:{1000000,4100002,4200001,1110000,1210000}) if(get_skilllevel(id)>0) {
                const auto n=nl::nx::skill[std::to_string(id/10000)+".img"]["skill"][std::to_string(id)]["level"][get_skilllevel(id)];
                hp=std::max(hp,int(n["hp"].get_integer()));mp=std::max(mp,int(n["mp"].get_integer()));
            }
            hp=std::min(hp,stats_.get_total(EquipStat::HP)-int(stats_.get_stat(MapleStat::HP)));
            mp=std::min(mp,stats_.get_total(EquipStat::MP)-int(stats_.get_stat(MapleStat::MP)));
            if(hp>0 || mp>0) HealOverTimePacket(std::max(0,hp),std::max(0,mp)).dispatch();
        }
    }
    const PlayerState *pst = get_state(state_);

    if (pst) {
        pst->update(*this);
        physics.move_object(phobj_);

        bool aniend = Char::update(physics, get_stancespeed());

        if (aniend && attacking_) {
            attacking_ = false;
            nullstate.update_state(*this);
        } else {
            pst->update_state(*this);
        }
    }

    uint8_t stancebyte = facing_right_ ? state_ : state_ + 1;
    Movement newmove(phobj_, stancebyte);
    bool needupdate = last_move_.hasmoved(newmove);

    if (needupdate) {
        fn_move_player(newmove);
        last_move_ = newmove;
    }

    return get_layer();
}

int8_t Player::get_integer_attackspeed() const {
    int32_t weapon_id = look_.get_equips().get_weapon();

    if (weapon_id <= 0) {
        return 0;
    }

    const WeaponData &weapon = WeaponData::get(weapon_id);

    int8_t base_speed = stats_.get_attackspeed();
    int8_t weapon_speed = weapon.get_speed();

    return base_speed + weapon_speed;
}

void Player::set_direction(bool flipped) {
    if (!attacking_) {
        Char::set_direction(flipped);
    }
}

void Player::set_channel_id(uint8_t ch) {
    channel_id_ = ch;
}

void Player::set_state(State st) {
    if (!attacking_) {
        Char::set_state(st);

        const PlayerState *pst = get_state(st);

        if (pst) {
            pst->initialize(*this);
        }
    }
}

bool Player::is_attacking() const {
    return attacking_;
}

bool Player::can_attack() const {
    return state_!=Char::State::DIED && !attacking_ && !is_climbing() && !is_sitting()
           && look_.get_equips().has_weapon();
}

SpecialMove::ForbidReason Player::can_use(const SpecialMove &move) const {
    if (move.is_skill() && state_ == Char::State::PRONE && move.get_id()!=21120005) {
        return SpecialMove::ForbidReason::FBR_OTHER;
    }

    if (move.is_attack()
        && (state_ == Char::State::LADDER || state_ == Char::State::ROPE)) {
        return SpecialMove::ForbidReason::FBR_OTHER;
    }

    if (has_cooldown(move.get_id())) {
        return SpecialMove::ForbidReason::FBR_COOLDOWN;
    }

    const int id=move.get_id();
    if((id==5111002 || id==5111004 || id==5121002) && buffs_[Buffstat::ENERGY_CHARGE].value<10000) return SpecialMove::FBR_OTHER;
    if((id==1111003 || id==1111004 || id==1111005 || id==1111006) && buffs_[Buffstat::COMBO].value<=1) return SpecialMove::FBR_OTHER;
    if(id==1211002 && !has_buff(Buffstat::WK_CHARGE)) return SpecialMove::FBR_OTHER;
    if(id==4221001 && !has_buff(Buffstat::DARKSIGHT)) return SpecialMove::FBR_OTHER;
    if(id==4111006 && (phobj_.onground || flash_jumped_)) return SpecialMove::FBR_OTHER;
    const auto node=nl::nx::skill[std::to_string(id/10000)+".img"]["skill"][std::to_string(id)]["level"][get_skilllevel(id)];
    if(node["itemCon"].get_integer()>0 && inventory_.get_total_item_count(node["itemCon"])<node["itemConNo"].get_integer(1)) return SpecialMove::FBR_OTHER;
    if(node["moneyCon"].get_integer()>inventory_.get_meso()) return SpecialMove::FBR_OTHER;
    if(move.is_skill()) {
        int amplification=100;
        for(int passive:{2110001,2210001}) if(get_skilllevel(passive)>0) amplification=nl::nx::skill[std::to_string(passive/10000)+".img"]["skill"][std::to_string(passive)]["level"][get_skilllevel(passive)]["x"].get_integer(100);
        const auto cost=adventurer::mp_cost(node["mpCon"].get_integer(),amplification,has_buff(Buffstat::INFINITY_),has_buff(Buffstat::CONCENTRATE)?buffs_[Buffstat::CONCENTRATE].value:0);
        if(stats_.get_stat(MapleStat::MP)<cost)return SpecialMove::FBR_MPCOST;
        if(id==1311006 && stats_.get_stat(MapleStat::HP)<=stats_.get_total(EquipStat::HP)*node["x"].get_integer()/100)return SpecialMove::FBR_HPCOST;
    }
    int32_t level = skill_book_.get_level(move.get_id());
    Weapon::Type weapon = get_weapontype();
    const Job &job = stats_.get_job();
    uint16_t hp = stats_.get_stat(MapleStat::Id::HP);
    uint16_t mp = stats_.get_stat(MapleStat::Id::MP);
    uint16_t bullets = (has_buff(Buffstat::SOULARROW)||has_buff(Buffstat::SHADOW_CLAW)) ? 65535 : inventory_.get_bulletcount();

    if(move.is_attack() && has_buff(Buffstat::SHADOWPARTNER) && adventurer::uses_ammo(id) && !has_buff(Buffstat::SHADOW_CLAW) && !has_buff(Buffstat::SOULARROW))bullets/=2;
    return move.can_use(level, weapon, job, hp, mp, bullets);
}

Attack Player::prepare_attack(bool skill) const {
    Attack::Type attacktype;
    bool degenerate;

    if (state_ == Char::State::PRONE && !skill) {
        degenerate = true;
        attacktype = Attack::Type::CLOSE;
    } else {
        Weapon::Type weapontype;
        weapontype = get_weapontype();

        switch (weapontype) {
            case Weapon::Type::BOW:
            case Weapon::Type::CROSSBOW:
            case Weapon::Type::CLAW:
            case Weapon::Type::GUN:
                degenerate = !inventory_.has_projectile() && !has_buff(Buffstat::SOULARROW) && !has_buff(Buffstat::SHADOW_CLAW);
                attacktype =
                    degenerate ? Attack::Type::CLOSE : Attack::Type::RANGED;
                break;
            case Weapon::Type::WAND:
            case Weapon::Type::STAFF:
                degenerate = !skill;
                attacktype =
                    degenerate ? Attack::Type::CLOSE : Attack::Type::MAGIC;
                break;
            default:
                attacktype = Attack::Type::CLOSE;
                degenerate = false;
                break;
        }
    }

    Attack attack;
    attack.type = attacktype;
    attack.mindamage = stats_.get_mindamage();
    attack.maxdamage = stats_.get_maxdamage();

    if (degenerate) {
        attack.mindamage /= 10;
        attack.maxdamage /= 10;
    }

    attack.weapon_attack=stats_.get_total(EquipStat::WATK);
    attack.magic=stats_.get_total(EquipStat::MAGIC)+stats_.get_total(EquipStat::INT);
    attack.intelligence=stats_.get_total(EquipStat::INT);
    attack.luck=stats_.get_total(EquipStat::LUK);
    attack.mastery=stats_.get_mastery();
    attack.critical = stats_.get_job().get_id()/100==21 ? 0.0f : stats_.get_critical();
    if (stats_.get_job().get_id()/100==21 && get_skilllevel(21110003)>0)
        attack.air_damage=1.0f+nl::nx::skill["2111.img"]["skill"]["21110003"]["level"][get_skilllevel(21110003)]["x"].get_integer()/100.0f;
    if (stats_.get_job().get_id()/100==21 && get_skilllevel(21110000)>0 && get_weapontype()==Weapon::POLEARM) {
        const auto level=nl::nx::skill["2111.img"]["skill"]["21110000"]["level"][get_skilllevel(21110000)];
        const auto groups=std::min<int32_t>(Stage::get().get_combat().get_combo()/10,level["x"].get_integer());
        attack.critical=std::min(1.0f,groups*level["y"].get_integer()/100.0f);
        attack.critical_damage=1.0f+groups*level["damage"].get_integer()/100.0f;
    }
    if(stats_.get_job().get_id()/100==21 && has_buff(Buffstat::WK_CHARGE) && get_skilllevel(21111005)>0) {
        const float multiplier=nl::nx::skill["2111.img"]["skill"]["21111005"]["level"][get_skilllevel(21111005)]["damage"].get_integer(100)/100.0f;
        attack.mindamage*=multiplier; attack.maxdamage*=multiplier;
    }
    for(int id:{3000001,4100001}) if(get_skilllevel(id)>0 && ((id==3000001 && (get_weapontype()==Weapon::BOW || get_weapontype()==Weapon::CROSSBOW)) || (id==4100001 && get_weapontype()==Weapon::CLAW))) {
        const auto n=nl::nx::skill[std::to_string(id/10000)+".img"]["skill"][std::to_string(id)]["level"][get_skilllevel(id)];
        attack.critical=n["prop"].get_integer()/100.0f;attack.critical_damage=n["damage"].get_integer(150)/100.0f;
    }
    if(has_buff(Buffstat::SHARP_EYES)) {
        const uint16_t value=static_cast<uint16_t>(buffs_[Buffstat::SHARP_EYES].value);
        attack.critical=std::min(1.0f,attack.critical+(value>>8)/100.0f);attack.critical_damage+=(value&255)/100.0f;
    }
    if(has_buff(Buffstat::SHADOWPARTNER)) {
        const int id=buffs_[Buffstat::SHADOWPARTNER].skillid;
        attack.shadow_ratio=nl::nx::skill[std::to_string(id/10000)+".img"]["skill"][std::to_string(id)]["level"][get_skilllevel(id)][skill?"y":"x"].get_integer()/100.0f;
    }
    if(has_buff(Buffstat::COMBO)) {
        const int orbs=buffs_[Buffstat::COMBO].value;
        const int id=get_skilllevel(1120003)>0?1120003:1111002;
        const auto n=nl::nx::skill[std::to_string(id/10000)+".img"]["skill"][std::to_string(id)]["level"][get_skilllevel(id)];
        const double multiplier=orbs>1?(n["damage"].get_integer(100)+50)/100.0:1.0;
        attack.mindamage*=multiplier;attack.maxdamage*=multiplier;
    }
    if(buffs_[Buffstat::ENERGY_CHARGE].value>=10000 && get_skilllevel(5110001)>0) {
        const auto n=nl::nx::skill["511.img"]["skill"]["5110001"]["level"][get_skilllevel(5110001)];
        attack.mindamage*=1+n["damage"].get_integer()/100.0;attack.maxdamage*=1+n["damage"].get_integer()/100.0;
    }
    attack.neutral_elements=has_buff(Buffstat::ELEMENTAL_RESET);
    if(has_buff(Buffstat::WK_CHARGE)) {
        const int id=buffs_[Buffstat::WK_CHARGE].skillid;const int level=get_skilllevel(id);
        if(const char element=adventurer::charge_element(id);element && level>0) {
            const auto n=nl::nx::skill[std::to_string(id/10000)+".img"]["skill"][std::to_string(id)]["level"][level];
            const double multiplier=n["damage"].get_integer(100)/100.0;
            attack.mindamage*=multiplier;attack.maxdamage*=multiplier;attack.element=element;
            attack.weak_multiplier=(element=='H'?1.2:1.05)+level*0.015;
        }
    }
    attack.beacon_target=homing_target();
    if(attack.beacon_target && get_skilllevel(5220011)>0) attack.beacon_bonus=nl::nx::skill["522.img"]["skill"]["5220011"]["level"][get_skilllevel(5220011)]["x"].get_integer()/100.0f;
    attack.ignoredef = stats_.get_ignoredef();
    attack.accuracy = stats_.get_total(EquipStat::Id::ACC);
    attack.playerlevel = stats_.get_stat(MapleStat::Id::LEVEL);
    attack.range = stats_.get_range();
    attack.bullet = inventory_.get_bulletid();
    attack.origin = get_position();
    attack.toleft = !facing_right_;
    attack.speed = get_integer_attackspeed();

    return attack;
}

void Player::rush(double targetx) {
    if (phobj_.onground) {
        uint16_t delay = get_attackdelay(1);
        phobj_.movexuntil(targetx, delay);
        phobj_.set_flag(PhysicsObject::Flag::TURN_AT_EDGES);
    }
}

void Player::combat_step() {
    if (!phobj_.onground) return;
    const auto delay=std::max<uint16_t>(get_attackdelay(1),100);
    const auto level=get_skilllevel(21001001);
    const double speed=nl::nx::skill["2100.img"]["skill"]["21001001"]["level"][level]["x"].get_integer(650);
    const double distance=speed*delay/1000.0;
    phobj_.movexuntil(phobj_.get_x()+(facing_right_?distance:-distance),delay);
    phobj_.set_flag(PhysicsObject::Flag::TURN_AT_EDGES);
}

void Player::skill_movement(int32_t id,const Physics &physics) {
    const auto src=nl::nx::skill[std::to_string(id/10000)+".img"]["skill"][std::to_string(id)]["level"][get_skilllevel(id)];
    if(adventurer::teleport(id)) {
        const int range=src["range"].get_integer(130);
        const auto before=get_position();auto target=before;
        if(is_key_down(KeyAction::UP)) target.set_y(before.y()-range);
        else if(is_key_down(KeyAction::DOWN)) target.set_y(before.y()+range);
        else target.set_x(before.x()+(facing_right_?range:-range));
        const auto walls=physics.get_fht().get_walls();target.set_x(std::clamp<int16_t>(target.x(),walls.first(),walls.second()));
        const auto ground=physics.get_y_below(target-Point<int16_t>(0,10));
        if(std::abs(ground.y()-target.y())<=50 && before.distance(ground)<=range+50) {
            set_position(ground);phobj_.hspeed=phobj_.vspeed=0;physics.get_fht().update_fh(phobj_);
        }
    } else if(id==4111006 && !phobj_.onground && !flash_jumped_) {
        flash_jumped_=true;
        phobj_.hspeed=facing_right_?7.0:-7.0;phobj_.vspeed=-4.0;
    } else if(id==5201005) {
        phobj_.vspeed=-src["x"].get_integer(280)/40.0;phobj_.onground=false;
    } else if(id==5201006) {
        phobj_.hspeed=facing_right_?-5.0:5.0;phobj_.vspeed=-2.5;phobj_.onground=false;
    }
}
bool Player::is_invincible() const {
    if (state_ == Char::State::DIED) {
        return true;
    }

    if (has_buff(Buffstat::Id::DARKSIGHT)) {
        return true;
    }

    return Char::is_invincible();
}

MobAttackResult Player::damage(const MobAttack &attack) {
    const int32_t raw=stats_.calculate_damage(attack.watk,false);
    int32_t damage = stats_.calculate_damage(attack.watk);
    recovery_at_=std::chrono::steady_clock::now()+std::chrono::seconds(10);
    bool guarded=false;
    for(int id:{4120002,4220002,1120005,1220006}) if(get_skilllevel(id)>0) {
        if((id==1120005 || id==1220006) && !inventory_.has_equipped(EquipSlot::SHIELD))continue;
        const auto n=nl::nx::skill[std::to_string(id/10000)+".img"]["skill"][std::to_string(id)]["level"][get_skilllevel(id)];
        if(randomizer_.below(adventurer::avoidance_probability(id,n["prop"].get_integer()))) {damage=0;guarded=id==1120005 || id==1220006;break;}
    }
    show_damage(damage);

    bool fromleft = attack.origin.x() > phobj_.get_x();

    bool missed = damage <= 0;
    bool immovable = ladder_ || state_ == Char::State::DIED;
    bool knockback = !missed && !immovable;

    if (knockback && randomizer_.above(stats_.get_stance())) {
        phobj_.hspeed = fromleft ? -1.5 : 1.5;
        phobj_.vforce -= 3.5;
    }

    uint8_t direction = fromleft ? 0 : 1;

    MobAttackResult result(attack,damage,direction);result.reported_damage=damage?raw:0;
    result.guarded=guarded;result.mob_position=attack.origin;result.player_position=get_position();return result;
}

void Player::give_buff(Buff buff) {
    buffs_[buff.stat] = buff;
}

void Player::cancel_buff(Buffstat::Id stat) {
    buffs_[stat] = {};
    remove_recurring_effect();
}

bool Player::has_buff(Buffstat::Id stat) const {
    return buffs_[stat].skillid != 0;
}

void Player::change_skill(int32_t skill_id,
                          int32_t skill_level,
                          int32_t masterlevel,
                          int64_t expiration) {
    int32_t old_level = skill_book_.get_level(skill_id);
    skill_book_.set_skill(skill_id, skill_level, masterlevel, expiration);

    if (old_level != skill_level) {
        recalc_stats(false);
    }
}

void Player::add_cooldown(int32_t skill_id, int32_t cooltime) {
    cooldowns_.set(skill_id,cooltime);
}

bool Player::has_cooldown(int32_t skill_id) const {
    return cooldowns_.active(skill_id);
}

void Player::change_level(uint16_t level) {
    uint16_t oldlevel = get_level();

    if (level > oldlevel) {
        show_effect_id(CharEffect::Id::LEVEL_UP);
    }

    stats_.set_stat(MapleStat::Id::LEVEL, level);
}

uint16_t Player::get_level() const {
    return stats_.get_stat(MapleStat::Id::LEVEL);
}

int32_t Player::get_skilllevel(int32_t skillid) const {
    return skill_book_.get_level(skillid);
}

uint8_t Player::get_world_id() const {
    return wid_;
}

uint8_t Player::get_channel_id() const {
    return channel_id_;
}

void Player::change_job(uint16_t jobid) {
    show_effect_id(CharEffect::Id::JOB_CHANGE);
    stats_.change_job(jobid);
}

void Player::set_seat(std::optional<std::reference_wrapper<const Seat>> seat) {
    if (seat) {
        set_position(seat->get().getpos());
        set_state(Char::State::SIT);
    }
}

void Player::set_ladder(
    std::optional<std::reference_wrapper<const Ladder>> ldr) {
    ladder_ = ldr;

    if (ladder_) {
        phobj_.set_x(ldr->get().get_x());
        phobj_.hspeed = 0.0;
        phobj_.vspeed = 0.0;
        phobj_.fhlayer = 7;
        set_state(ldr->get().is_ladder() ? Char::State::LADDER
                                         : Char::State::ROPE);
    }
}

float Player::get_walkforce() const {
    return 0.05f
           + 0.11f * static_cast<float>(stats_.get_total(EquipStat::Id::SPEED))
                 / 100;
}

float Player::get_jumpforce() const {
    return 1.0f
           + 3.5f * static_cast<float>(stats_.get_total(EquipStat::Id::JUMP))
                 / 100;
}

float Player::get_climbforce() const {
    return static_cast<float>(stats_.get_total(EquipStat::Id::SPEED)) / 100;
}

float Player::get_flyforce() const {
    return 0.25f;
}

bool Player::is_underwater() const {
    return underwater_;
}

bool Player::is_key_down(KeyAction::Id action) const {
    return keys_down_.count(action) ? keys_down_.at(action) : false;
}

CharStats &Player::get_stats() {
    return stats_;
}

const CharStats &Player::get_stats() const {
    return stats_;
}

Inventory &Player::get_inventory() {
    return inventory_;
}

const Inventory &Player::get_inventory() const {
    return inventory_;
}

SkillBook &Player::get_skills() {
    return skill_book_;
}

QuestLog &Player::get_quests() {
    return quest_log_;
}

TeleportRock &Player::get_teleport_rock() {
    return teleport_rock_;
}

MonsterBook &Player::get_monsterbook() {
    return monster_book_;
}

std::optional<std::reference_wrapper<const Ladder>> Player::get_ladder() const {
    return ladder_;
}

int32_t Player::get_cid() const {
    return cid_;
}
}  // namespace ms
