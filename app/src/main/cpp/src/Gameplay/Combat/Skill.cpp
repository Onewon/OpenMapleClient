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
#include "Skill.h"
#include "../../Character/AdventurerSkills.h"
#include "CombatMath.h"

#include <nlnx/nx.hpp>

#include "../../Character/SkillId.h"
#include "../../Data/SkillData.h"
#include "StringHandling.h"

namespace ms {
Skill::Skill(int32_t id) : skill_id_(id) {
    const SkillData &data = SkillData::get(skill_id_);

    std::string strid;

    if (skill_id_ < 10000000) {
        strid = string_format::extend_id(skill_id_, 7);
    } else {
        strid = std::to_string(skill_id_);
    }

    nl::node src = nl::nx::skill[strid.substr(0, strid.size() - 4) + ".img"]["skill"][strid];

    projectile_ = true;
    over_regular_ = false;

    sound_ = std::make_unique<SingleSkillSound>(strid);

    bool has_affected_effect = src["affected"].size() > 0;
    bool by_level_effect = src["CharLevel"]["10"]["effect"].size() > 0;
    bool multi_effect = src["effect0"].size() > 0;

    if (has_affected_effect) {
        affected_effects_ = std::make_unique<SingleAffectedEffect>(src);
    } else {
        affected_effects_ = std::make_unique<NoAffectedEffect>();
    }

    if (by_level_effect) {
        use_effect_ = std::make_unique<ByLevelUseEffect>(src);
    } else if (multi_effect) {
        use_effect_ = std::make_unique<MultiUseEffect>(src);
    } else {
        bool is_animation =
            src["effect"]["0"].data_type() == nl::node::type::bitmap;
        bool has_effect1 = src["effect"]["1"].size() > 0;

        if (is_animation) {
            use_effect_ = std::make_unique<SingleUseEffect>(src);
        } else if (has_effect1) {
            use_effect_ = std::make_unique<TwoHandedUseEffect>(src);
        } else {
            switch (skill_id_) {
                case SkillId::IRON_BODY:
                case SkillId::MAGIC_ARMOR:
                    use_effect_ = std::make_unique<IronBodyUseEffect>();
                    break;
                default: use_effect_ = std::make_unique<NoUseEffect>(); break;
            }
        }
    }

    bool by_level_hit = src["CharLevel"]["10"]["hit"].size() > 0;
    bool by_skill_level_hit = src["level"]["1"]["hit"].size() > 0;
    bool has_hit0 = src["hit"]["0"].size() > 0;
    bool has_hit1 = src["hit"]["1"].size() > 0;

    if (by_level_hit) {
        if (has_hit0 && has_hit1) {
            hit_effect_ = std::make_unique<ByLevelTwoHandedHitEffect>(src);
        } else {
            hit_effect_ = std::make_unique<ByLevelHitEffect>(src);
        }
    } else if (by_skill_level_hit) {
        hit_effect_ = std::make_unique<BySkillLevelHitEffect>(src);
    } else if (has_hit0 && has_hit1) {
        hit_effect_ = std::make_unique<TwoHandedHitEffect>(src);
    } else if (has_hit0) {
        hit_effect_ = std::make_unique<SingleHitEffect>(src);
    } else {
        hit_effect_ = std::make_unique<NoHitEffect>();
    }

    bool has_action0 = src["action"]["0"].data_type() == nl::node::type::string;
    bool has_action1 = src["action"]["1"].data_type() == nl::node::type::string;

    if (has_action0 && has_action1) {
        action_ = std::make_unique<TwoHandedAction>(src);
    } else if (has_action0) {
        action_ = std::make_unique<SingleAction>(src);
    } else if (data.is_attack()) {
        bool by_level =
            src["level"]["1"]["action"].data_type() == nl::node::type::string;

        if (by_level) {
            action_ = std::make_unique<ByLevelAction>(src, skill_id_);
        } else {
            action_ = std::make_unique<RegularAction>();
            over_regular_ = true;
        }
    } else {
        action_ = std::make_unique<NoAction>();
    }

    bool has_ball = src["ball"].size() > 0;
    bool by_level_ball = src["level"]["1"]["ball"].size() > 0;

    if (by_level_ball) {
        bullet_ = std::make_unique<BySkillLevelBullet>(src, skill_id_);
    } else if (has_ball) {
        bullet_ = std::make_unique<SingleBullet>(src);
    } else {
        bullet_ = std::make_unique<RegularBullet>();
        projectile_ = false;
    }
}

void Skill::apply_useeffects(Char &user) const {
    use_effect_->apply(user);

    sound_->play_use();
}

void Skill::apply_actions(Char &user, Attack::Type type) const {
    action_->apply(user, type);
}

void Skill::apply_stats(const Char &user, Attack &attack) const {
    attack.skill = skill_id_;
    const auto src=nl::nx::skill[std::to_string(skill_id_/10000)+".img"]["skill"][std::to_string(skill_id_)];
    const auto element=src["elemAttr"].get_string();if(!element.empty()){attack.element=element.front();attack.weak_multiplier=1.5;}
    if(adventurer::attack(skill_id_) || skill_id_==5220011 || adventurer::mortal_blow(skill_id_)) attack.type=static_cast<Attack::Type>(adventurer::attack_mode(skill_id_));

    int32_t level = user.get_skilllevel(skill_id_);
    const SkillData::Stats stats = SkillData::get(skill_id_).get_stats(level);

    if(skill_id_==4001344 || skill_id_==4121007) {
        const auto range=combat_math::throwing_damage(attack.luck,attack.weapon_attack);
        attack.mindamage=range.first;attack.maxdamage=range.second;
    }
    if (stats.fixdamage) {
        attack.fixdamage = stats.fixdamage;
        attack.damagetype = Attack::DMG_FIXED;
    } else if (stats.matk) {
        attack.matk = stats.matk;
        attack.damagetype = Attack::DMG_MAGIC;
        const auto range=combat_math::magic_damage(attack.magic,attack.intelligence,stats.matk,stats.mastery);
        attack.mindamage=range.first;attack.maxdamage=range.second;
        for(int id:{2110001,2210001}) if(user.get_skilllevel(id)>0) {
            const double multiplier=nl::nx::skill[std::to_string(id/10000)+".img"]["skill"][std::to_string(id)]["level"][user.get_skilllevel(id)]["y"].get_integer(100)/100.0;
            attack.mindamage*=multiplier;attack.maxdamage*=multiplier;
        }
    } else {
        attack.mindamage *= stats.damage;
        attack.maxdamage *= stats.damage;
        attack.damagetype = Attack::DMG_WEAPON;
    }

    if((skill_id_==5211004 || skill_id_==5211005) && user.get_skilllevel(5220001)>0) {
        const int bonus=nl::nx::skill["522.img"]["skill"]["5220001"]["level"][user.get_skilllevel(5220001)]["damage"].get_integer();
        const double ratio=adventurer::elemental_gun_ratio(stats.damage,bonus);
        attack.mindamage*=ratio;attack.maxdamage*=ratio;
    }
    if(skill_id_==1211002 && user.get_skilllevel(1220010)>0) {
        const auto advanced=nl::nx::skill["122.img"]["skill"]["1220010"]["level"][user.get_skilllevel(1220010)];
        const double ratio=advanced["damage"].get_integer(350)/(stats.damage*100.0);
        attack.mindamage*=ratio;attack.maxdamage*=ratio;
    }
    if(skill_id_==4111004) {
        const int cost=nl::nx::skill["411.img"]["skill"]["4111004"]["level"][level]["moneyCon"].get_integer();
        attack.mindamage=cost*10;attack.maxdamage=cost*15;attack.critical=0;attack.shadow_ratio=0;
    }
    if(skill_id_==3101005) {
        const double multiplier=nl::nx::skill["310.img"]["skill"]["3101005"]["level"][level]["x"].get_integer(100)/100.0;
        attack.mindamage*=multiplier;attack.maxdamage*=multiplier;
    }
    if(skill_id_==3221007) {
        // This server rewrites Snipe to 195000..199999. Keep the local preview in that range.
        attack.mindamage=195000;attack.maxdamage=199999;
    }
    if(skill_id_==2301002) {
        const double percent=nl::nx::skill["230.img"]["skill"]["2301002"]["level"][level]["hp"].get_integer()/100.0;
        attack.maxdamage=(attack.intelligence*4.8+attack.luck*4.0)*attack.magic/1000.0*percent;
        attack.mindamage=attack.maxdamage*0.8;attack.damagetype=Attack::DMG_MAGIC;attack.undead_only=true;
    }
    attack.critical += stats.critical;
    attack.ignoredef += stats.ignoredef;
    attack.mobcount = stats.mobcount;
    attack.hrange = skill_id_==21110006 ? 1.0f : stats.hrange;

    switch (attack.type) {
        case Attack::RANGED: attack.hitcount = stats.bulletcount; break;
        default: attack.hitcount = stats.attackcount; break;
    }

    if ((skill_id_==21100004 || skill_id_==21110004) && stats.range.empty()) {
        const int16_t reach=static_cast<int16_t>(stats.hrange*100);
        attack.range=Rectangle<int16_t>(-reach,0,-100,50);
        attack.hrange=1.0f;
    }
    if (!stats.range.empty()) {
        attack.range = stats.range;
    }
    if(adventurer::kind(skill_id_)!=adventurer::Kind::Unknown && stats.range.empty() && (attack.type==Attack::RANGED || projectile_)) {
        const int reach=src["level"][level]["range"].get_integer();
        if(reach>0) {attack.range={static_cast<int16_t>(-reach),-5,-50,50};attack.hrange=1.0f;}
    }


    if(attack.type==Attack::CLOSE && !projectile_ && stats.range.empty()) {
        const auto reach=user.get_afterimage().get_range();
        if(!reach.empty())attack.range=reach;
    }

    if(attack.shadow_ratio>0 && attack.hitcount<=7) attack.hitcount*=2;
    if (projectile_ && !attack.bullet) {
        switch (skill_id_) {
            case SkillId::THREE_SNAILS:
                switch (level) {
                    case 1: attack.bullet = 4000019; break;
                    case 2: attack.bullet = 4000000; break;
                    case 3: attack.bullet = 4000016; break;
                }
                break;
            default: attack.bullet = skill_id_; break;
        }
    }

    if (over_regular_) {
        attack.stance = user.get_look().get_stance();

        if (attack.type == Attack::CLOSE && !projectile_) {
            attack.range = user.get_afterimage().get_range();
        }
    }
}

void Skill::apply_hiteffects(const AttackUser &user, Mob &target) const {
    hit_effect_->apply(user, target);

    sound_->play_hit();
}

void Skill::apply_affected_effects(Char &user) const {
    affected_effects_->apply(user);
}

Animation Skill::get_bullet(const Char &user, int32_t bulletid) const {
    return bullet_->get(user, bulletid);
}

bool Skill::is_attack() const {
    return SkillData::get(skill_id_).is_attack();
}

bool Skill::is_skill() const {
    return true;
}

int32_t Skill::get_id() const {
    return skill_id_;
}

SpecialMove::ForbidReason Skill::can_use(int32_t level,
                                         Weapon::Type weapon,
                                         const Job &job,
                                         uint16_t hp,
                                         uint16_t mp,
                                         uint16_t bullets) const {
    if (level <= 0 || level > SkillData::get(skill_id_).get_masterlevel()) {
        return FBR_OTHER;
    }

    if (job.can_use(skill_id_) == false) {
        return FBR_OTHER;
    }

    const SkillData::Stats stats = SkillData::get(skill_id_).get_stats(level);

    if (hp <= stats.hpcost) {
        return FBR_HPCOST;
    }

    // Player::can_use checks the effective cost after amplification, Infinity and Concentrate.

    const bool sword=weapon==Weapon::SWORD_1H || weapon==Weapon::SWORD_2H;
    const bool axe=weapon==Weapon::AXE_1H || weapon==Weapon::AXE_2H;
    const bool mace=weapon==Weapon::MACE_1H || weapon==Weapon::MACE_2H;
    switch(skill_id_) {
        case 1101004:case 1201004:case 1111003:case 1111005:case 1211003:case 1211005:case 1211007:case 1221003:
            if(!sword)return FBR_WEAPONTYPE;break;
        case 1101005:case 1111004:case 1111006:if(!axe)return FBR_WEAPONTYPE;break;
        case 1201005:case 1211004:case 1211006:case 1211008:case 1221004:if(!mace)return FBR_WEAPONTYPE;break;
        case 1301004:if(weapon!=Weapon::SPEAR)return FBR_WEAPONTYPE;break;
        case 1301005:if(weapon!=Weapon::POLEARM)return FBR_WEAPONTYPE;break;
        default:break;
    }
    Weapon::Type reqweapon = SkillData::get(skill_id_).get_required_weapon();

    if (weapon != reqweapon && reqweapon != Weapon::NONE) {
        return FBR_WEAPONTYPE;
    }

    if(adventurer::kind(skill_id_)!=adventurer::Kind::Unknown && !adventurer::uses_ammo(skill_id_))return FBR_NONE;

    switch (weapon) {
        case Weapon::BOW:
        case Weapon::CROSSBOW:
        case Weapon::CLAW:
        case Weapon::GUN:
            return (bullets >= stats.bulletcost) ? FBR_NONE : FBR_BULLETCOST;
        default: return FBR_NONE;
    }
}
}  // namespace ms
