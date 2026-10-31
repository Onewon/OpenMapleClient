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
#include "Combat.h"
#include "../../Character/AranSkills.h"
#include "../../Character/AdventurerSkills.h"
#include "../../Character/MonsterMagnetStats.h"
#include "../Stage.h"
#include "../../Data/SkillData.h"

#include "../../Character/SkillId.h"
#include "../../IO/Messages.h"
#include "../../Net/Packets/AttackAndSkillPackets.h"
#include "../../Net/Packets/GameplayPackets.h"
#include <android/log.h>
#include <nlnx/nx.hpp>

namespace ms {
namespace {
auto fn_attack = []<typename... T>(T && ...args) {
    AttackPacket(std::forward<T>(args)...).dispatch();
};
auto fn_damage_reactor = []<typename... T>(T && ...args) {
    DamageReactorPacket(std::forward<T>(args)...).dispatch();
};
auto fn_use_skill = []<typename... T>(T && ...args) {
    UseSkillPacket(std::forward<T>(args)...).dispatch();
};
}  // namespace

Combat::Combat(Player &player,
               MapChars &chars,
               MapMobs &mobs,
               MapReactors &reactors) :
    player_(player),
    chars_(chars),
    mobs_(mobs),
    reactors_(reactors),
    attack_results_([&](const AttackResult &attack) { apply_attack(attack); }),
    bullet_effects_(
        [&](const BulletEffect &effect) { apply_bullet_effect(effect); }),
    damage_effects_(
        [&](const DamageEffect &effect) { apply_damage_effect(effect); }) {}

void Combat::draw(double viewx, double viewy, float alpha) const {
    draw_summons(viewx,viewy,alpha);
    if(charged_skill_)charge_effect_.draw(DrawArgument(player_.get_position()+Point<int16_t>(viewx,viewy)),alpha);
    const auto now=std::chrono::steady_clock::now();
    for(const auto &effect:meso_visuals_)if(now>=effect.ready)effect.animation.draw(DrawArgument(effect.position+Point<int16_t>(viewx,viewy)),alpha);
    for (const auto &bullet_eff : bullets_) {
        bullet_eff.bullet.draw(viewx, viewy, alpha);
    }

    for (const auto &dmg_eff : damage_numbers_) {
        dmg_eff.draw(viewx, viewy, alpha);
    }
}

void Combat::clear_inputs() {
    if(held_skill_) CancelSkillPacket(held_skill_).dispatch();
    held_skill_=charged_skill_=charge_ms_=final_attack_=0;
    aran_chain_ready_=aran_attack_buffered_=false;aran_chain_stage_=0;
}
void Combat::skill_input(int32_t id, bool down) {
    const auto now=std::chrono::steady_clock::now();
    if(adventurer::charge(id)) {
        if(down) {if(!charged_skill_ && player_.can_attack()){
            charged_skill_=id;charge_start_=now;
            const auto src=nl::nx::skill[std::to_string(id/10000)+".img"]["skill"][std::to_string(id)];charge_effect_=Animation(src["keydown"]);
            if(src["prepare"]["0"].data_type()==nl::node::type::bitmap)player_.show_attack_effect(Animation(src["prepare"]),1);
        }}
        else if(charged_skill_==id) {
            charged_skill_=0;charge_ms_=adventurer::charge_time(std::chrono::duration_cast<std::chrono::milliseconds>(now-charge_start_).count());
            use_move(id);charge_ms_=0;
        }
    } else if(adventurer::hold(id)) {
        if(down && held_skill_!=id){held_skill_=id;held_next_=now+std::chrono::milliseconds(120);use_move(id);}
        else if(!down && held_skill_==id) {held_skill_=0;CancelSkillPacket(id).dispatch();}
    } else if(down) use_move(id);
}
void Combat::update() {
    update_summons();
    if(charged_skill_)charge_effect_.update();
    const auto now=std::chrono::steady_clock::now();
    meso_visuals_.remove_if([&](auto &effect){return now>=effect.ready && effect.animation.update();});
    if(player_.get_stats().get_stat(MapleStat::HP)==0) clear_inputs();
    if(held_skill_ && std::chrono::steady_clock::now()>=held_next_) {
        held_next_=std::chrono::steady_clock::now()+std::chrono::milliseconds(120);
        use_move(held_skill_);
    }
    if (aran_combo_>0 && get_combo()==0) set_combo(0);
    if (aran_attack_buffered_) {
        if (std::chrono::steady_clock::now()>aran_chain_deadline_ || player_.get_stats().get_mapid()!=aran_chain_map_) {
            aran_attack_buffered_=false; aran_chain_ready_=false;
        } else if (player_.can_attack()) {
            aran_attack_buffered_=false;
            use_move(aran_buffered_move_);
        }
    }
    if(final_attack_ && player_.can_attack()) {const int id=final_attack_;final_attack_=0;use_move(id);}
    attack_results_.update();
    bullet_effects_.update();
    damage_effects_.update();

    bullets_.remove_if([&](BulletEffect &mb) {
        int32_t target_oid = mb.damageeffect.target_oid;

        if (mobs_.contains(target_oid)) {
            mb.target = mobs_.get_mob_head_position(target_oid);
            bool apply = mb.bullet.update(mb.target);

            if (apply) {
                apply_damage_effect(mb.damageeffect);
            }

            return apply;
        }
        return mb.bullet.update(mb.target);
    });

    damage_numbers_.remove_if([](DamageNumber &dn) { return dn.update(); });
}

void Combat::body_pressure(int32_t oid) {
    if(!player_.has_buff(Buffstat::BODY_PRESSURE) || player_.get_skilllevel(21101003)<=0) return;
    Attack attack=player_.prepare_attack(true);
    attack.skill=21101003;
    const auto stats=SkillData::get(21101003).get_stats(player_.get_skilllevel(21101003));
    attack.mindamage*=stats.damage; attack.maxdamage*=stats.damage;
    attack.hitcount=attack.mobcount=1;
    AttackResult result=attack;
    mobs_.send_attack(result,attack,{oid},1);
    result.attacker=player_.get_oid();
    fn_attack(result);
}
void Combat::set_combo(int32_t count) {
    aran_combo_=std::max(0,count);
    aran_combo_time_=std::chrono::steady_clock::now();
    player_.recalc_stats(false);
}
int32_t Combat::get_combo() const {
    return std::chrono::steady_clock::now()-aran_combo_time_>std::chrono::seconds(3) ? 0 : aran_combo_;
}
void Combat::direction(int32_t action, bool down) {
    if (!down || player_.is_key_down(static_cast<KeyAction::Id>(action))) return;
    if (action!=KeyAction::LEFT && action!=KeyAction::RIGHT && action!=KeyAction::UP && action!=KeyAction::DOWN) return;
    const auto now=std::chrono::steady_clock::now();
    if(now-aran_direction_time_>std::chrono::milliseconds(500)) aran_last_direction_=0;
    __android_log_print(ANDROID_LOG_DEBUG,"AranCombat","Direction=%d stage=%d finalBlowLevel=%d",action,aran_chain_stage_,player_.get_skilllevel(21120005));
    aran_previous_direction_=aran_last_direction_;
    aran_last_direction_=action;
    aran_direction_time_=now;
    if(action==aran_previous_direction_ && (action==KeyAction::LEFT || action==KeyAction::RIGHT)
        && player_.get_stats().get_job().get_id()/100==21 && player_.get_skilllevel(21001001)>0) {
        player_.set_direction(action==KeyAction::LEFT);
        use_move(21001001);
        aran_last_direction_=0;
    }
}
void Combat::use_move(int32_t move_id) {
    move_id=adventurer::upgraded(move_id,player_.get_skilllevel(5210000)>0,player_.get_skilllevel(5220002)>0,player_.get_skilllevel(5220011)>0);
    if(player_.get_stats().get_stat(MapleStat::HP)==0) return;
    const bool aran=player_.get_stats().get_job().get_id()/100==21
        && player_.get_weapontype()==Weapon::POLEARM;
    const auto now=std::chrono::steady_clock::now();
    if(player_.get_stats().get_mapid()!=aran_chain_map_ || now>aran_chain_deadline_) {
        aran_chain_ready_=false; aran_chain_stage_=0; aran_attack_buffered_=false;
    }
    if(aran && move_id==0 && now-aran_direction_time_<std::chrono::milliseconds(500)) {
        if(aran_previous_direction_==KeyAction::UP && aran_last_direction_==KeyAction::UP && player_.get_skilllevel(21110006)>0) {
            move_id=21110006;
        } else if(aran_previous_direction_==KeyAction::DOWN) {
            if(aran_last_direction_==KeyAction::DOWN) {
                if(player_.get_skilllevel(21120007)>0 && get_combo()>=200) move_id=21120007;
                else if(player_.get_skilllevel(21100005)>0 && get_combo()>=30) move_id=21100005;
            } else if(aran_last_direction_==KeyAction::LEFT || aran_last_direction_==KeyAction::RIGHT) {
                if(player_.get_skilllevel(21120006)>0 && get_combo()>=200) move_id=21120006;
                else if(player_.get_skilllevel(21110004)>0 && get_combo()>=100) move_id=21110004;
                else if(player_.get_skilllevel(21100004)>0 && get_combo()>=30) move_id=21100004;
            }
        }
        if(move_id!=0) aran_last_direction_=aran_previous_direction_=0;
    }
    if (!player_.can_attack() && !(held_skill_==move_id && adventurer::hold(move_id))) {
        if(aran && (move_id==0 || aran::command(move_id)) && aran_chain_ready_) {
            aran_attack_buffered_=true; aran_buffered_move_=move_id;
        }
        return;
    }
    if(move_id==0 && aran && player_.get_skilllevel(21000002)>0) {
        if(aran_chain_stage_==3 && player_.get_skilllevel(21120005)>0 && (player_.is_key_down(KeyAction::DOWN) || (now-aran_direction_time_<std::chrono::milliseconds(500) && aran_last_direction_==KeyAction::DOWN))) {
            move_id=21120005; aran_chain_stage_=0;
        } else if(aran_chain_stage_==3 && (player_.is_key_down(KeyAction::UP) || (now-aran_direction_time_<std::chrono::milliseconds(500) && aran_last_direction_==KeyAction::UP)) && player_.get_skilllevel(21110003)>0) {
            move_id=21110003; aran_chain_stage_=0;
        } else if(aran_chain_stage_==3 && (player_.is_key_down(KeyAction::LEFT)||player_.is_key_down(KeyAction::RIGHT)) && player_.get_skilllevel(21100002)>0) {
            move_id=21100002; aran_chain_stage_=0;
        } else if(aran_chain_stage_==1) {
            move_id=aran::swing(2,player_.get_skilllevel(21110002),player_.get_skilllevel(21120002));
            aran_chain_stage_=2;
        } else if(aran_chain_stage_==2 && player_.get_skilllevel(21100001)>0) {
            move_id=aran::swing(3,player_.get_skilllevel(21110002),player_.get_skilllevel(21120002));
            aran_chain_stage_=3;
        } else aran_chain_stage_=1;
        aran_chain_ready_=true;
        aran_chain_map_=player_.get_stats().get_mapid();
        aran_chain_deadline_=now+std::chrono::milliseconds(1500);
    } else {aran_chain_ready_=false; aran_chain_stage_=0; aran_attack_buffered_=false;}
    if(aran::combo_cost(move_id)>get_combo()) return;
    if(move_id==0 && (player_.get_weapontype()==Weapon::BOW || player_.get_weapontype()==Weapon::CROSSBOW)) {
        const auto origin=player_.get_position();const int id=player_.get_weapontype()==Weapon::BOW?3110001:3210001;
        const int level=player_.get_skilllevel(id);
        if(level>0 && !find_closest(mobs_.get_mobs(),Rectangle<int16_t>(origin.x()-100,origin.x()+100,origin.y()-70,origin.y()+30),origin,1,true).empty()) {
            if(randomizer_.below(nl::nx::skill[std::to_string(id/10000)+".img"]["skill"][std::to_string(id)]["level"][level]["prop"].get_integer()/100.0f)) move_id=id;
        }
    }
    const SpecialMove &move=get_move(move_id);
    if(move_id && SkillData::get(move_id).is_passive() && !aran::attack(move_id) && !adventurer::final_attack(move_id) && !adventurer::upgrade(move_id) && !adventurer::mortal_blow(move_id)) return;
    const auto reason=player_.can_use(move);
    if(reason==SpecialMove::ForbidReason::FBR_NONE) {
        apply_move(move);
        if(aran::combo_cost(move_id)) set_combo(0);
        if(aran) __android_log_print(ANDROID_LOG_INFO,"AranCombat","Move=%d combo=%d stage=%d",move_id,get_combo(),aran_chain_stage_);
    } else {
        __android_log_print(ANDROID_LOG_INFO,"SkillCombat","Rejected skill=%d reason=%d",move_id,int(reason));
        aran_chain_ready_=false; aran_chain_stage_=0;
        ForbidSkillMessage(reason,player_.get_weapontype()).drop();
    }
}

void Combat::apply_move(const SpecialMove &move) {
    if(move.is_attack() && player_.has_buff(Buffstat::DARKSIGHT) && move.get_id()!=4221001) {
        CancelSkillPacket(4001003).dispatch();player_.cancel_buff(Buffstat::DARKSIGHT);
    }
    if(move.get_id()==4211006) {use_meso_explosion(move);return;}
    if (move.is_attack()) {
        Attack attack = player_.prepare_attack(move.is_skill());
        if(adventurer::attack(move.get_id())) attack.type=static_cast<Attack::Type>(adventurer::attack_mode(move.get_id()));

        move.apply_useeffects(player_);
        if(!adventurer::hold(move.get_id()) || !player_.is_attacking()) move.apply_actions(player_, attack.type);

        player_.set_afterimage(move.get_id());

        move.apply_stats(player_, attack);
        if(move.get_id()==1111003 || move.get_id()==1111004 || move.get_id()==1111005 || move.get_id()==1111006) {
            const double scale=adventurer::finisher_multiplier(player_.combo_orbs());
            attack.mindamage*=scale;attack.maxdamage*=scale;
        }
        if(adventurer::charge(move.get_id())) {const double scale=adventurer::charge_multiplier(charge_ms_);attack.mindamage*=scale;attack.maxdamage*=scale;}

        Point<int16_t> origin = attack.origin;
        Rectangle<int16_t> range = attack.range;
        auto hrange = static_cast<int16_t>(range.left() * attack.hrange);

        if (attack.toleft) {
            range = { static_cast<int16_t>(origin.x() + hrange),
                      static_cast<int16_t>(origin.x() + range.right()),
                              static_cast<int16_t>(origin.y() + range.top()),
                                      static_cast<int16_t>(origin.y() + range.bottom()) };
        } else {
            range = { static_cast<int16_t>(origin.x() - range.right()),
                    static_cast<int16_t>(origin.x() - hrange),
                            static_cast<int16_t>(origin.y() + range.top()),
                                    static_cast<int16_t>(origin.y() + range.bottom()) };
        }

        // This approach should also make it easier to implement PvP
        uint8_t mobcount = attack.mobcount;
        AttackResult result = attack;

        MapObjects *mob_objs = mobs_.get_mobs();
        MapObjects *reactor_objs = reactors_.get_reactors();

        std::vector<int32_t> mob_targets =
            find_closest(mob_objs, range, origin, mobcount, true);
        std::vector<int32_t> reactor_targets =
            find_closest(reactor_objs, range, origin, mobcount, false);

        const int32_t beacon=player_.homing_target();
        if(beacon>0) {
            auto marked=mob_objs->get<Mob>(beacon);
            if(marked && marked->get().is_alive() && marked->get().is_in_range(range) && mobcount>0) {
                auto it=std::find(mob_targets.begin(),mob_targets.end(),beacon);
                if(it!=mob_targets.end())mob_targets.erase(it);
                mob_targets.insert(mob_targets.begin(),beacon);
                if(mob_targets.size()>mobcount)mob_targets.resize(mobcount);
            }
        }
        if(attack.undead_only) mob_targets.erase(std::remove_if(mob_targets.begin(),mob_targets.end(),[&](int32_t oid) {auto m=mob_objs->get<Mob>(oid);return !m || !m->get().is_undead();}),mob_targets.end());
        mobs_.send_attack(result, attack, mob_targets, mobcount);
        result.attacker = player_.get_oid();
        if(adventurer::charge(move.get_id())) result.charge=std::max(1,charge_ms_);
        extract_effects(player_, move, result);

        apply_use_movement(move);
        apply_result_movement(move, result);

        __android_log_print(ANDROID_LOG_INFO,"SkillCombat","Range skill=%d origin=%d,%d box=%d,%d,%d,%d loaded=%zu",move.get_id(),origin.x(),origin.y(),range.left(),range.right(),range.top(),range.bottom(),mob_objs->size());
        __android_log_print(ANDROID_LOG_INFO,"SkillCombat","Attack skill=%d type=%d mobs=%d hits=%d charge=%d",move.get_id(),int(result.type),result.mobcount,result.hitcount,result.charge);
        fn_attack(result);
        if(move.get_id()==4221001 && player_.has_buff(Buffstat::DARKSIGHT)) {
            CancelSkillPacket(4001003).dispatch();player_.cancel_buff(Buffstat::DARKSIGHT);
        }
        const auto skillnode=nl::nx::skill[std::to_string(move.get_id()/10000)+".img"]["skill"][std::to_string(move.get_id())];
        if(result.mobcount>0 && skillnode["finalAttack"].get_bool()) {
            int follow=0;
            switch(player_.get_weapontype()) {
                case Weapon::SWORD_1H:case Weapon::SWORD_2H:follow=player_.get_skilllevel(1100002)>0?1100002:1200002;break;
                case Weapon::AXE_1H:case Weapon::AXE_2H:follow=1100003;break;
                case Weapon::MACE_1H:case Weapon::MACE_2H:follow=1200003;break;
                case Weapon::SPEAR:follow=1300002;break;case Weapon::POLEARM:follow=1300003;break;
                case Weapon::BOW:follow=3100001;break;case Weapon::CROSSBOW:follow=3200001;break;default:break;
            }
            const int level=player_.get_skilllevel(follow);
            if(level>0 && randomizer_.below(nl::nx::skill[std::to_string(follow/10000)+".img"]["skill"][std::to_string(follow)]["level"][level]["prop"].get_integer()/100.0f)) final_attack_=follow;
        }
        if (result.mobcount>0 && !aran::combo_cost(move.get_id()) && player_.get_skilllevel(21000000)>0 && player_.get_stats().get_job().get_id()/100==21) {
            OutPacket combo(static_cast<OutPacket::Opcode>(163));
            combo.dispatch();
        }

        if (!reactor_targets.empty()) {
            if (auto reactor =
                    reactor_objs->get<Reactor>(reactor_targets.at(0))) {
                fn_damage_reactor(reactor->get().get_oid(),
                                  player_.get_position(),
                                  0,
                                  0);
            }
        }
    } else {
        move.apply_useeffects(player_);
        move.apply_actions(player_, Attack::Type::MAGIC);

        int32_t moveid = move.get_id();
        int32_t level = player_.get_skills().get_level(moveid);
        if(adventurer::magnet(moveid)) {
            const auto origin=player_.get_position();
            const auto magnet=adventurer::magnet_stats(level);
            const int16_t reach=magnet.range;
            auto targets=find_closest(mobs_.get_mobs(),Rectangle<int16_t>(origin.x()-reach,origin.x()+reach,origin.y()-150,origin.y()+150),origin,magnet.count,true);
            UseSkillPacket(moveid,level,origin,targets,player_.prepare_attack(true).toleft).dispatch();
        } else UseSkillPacket(moveid,level,player_.get_position()).dispatch();
        __android_log_print(ANDROID_LOG_INFO,"SkillCombat","Use skill=%d",moveid);
        apply_use_movement(move);
        if(moveid==21001001) player_.combat_step();
    }
}

std::vector<int32_t> Combat::find_closest(MapObjects *objs,
                                          Rectangle<int16_t> range,
                                          Point<int16_t> origin,
                                          uint8_t objcount,
                                          bool use_mobs) const {
    std::multimap<uint16_t, int32_t> distances;

    for (auto &mmo : *objs) {
        if (use_mobs) {
            const Mob *mob = static_cast<const Mob *>(mmo.second.get());

            if (mob && mob->is_alive() && mob->is_in_range(range)) {
                int32_t oid = mob->get_oid();
                uint16_t distance = mob->get_position().distance(origin);
                distances.emplace(distance, oid);
            }
        } else {
            // Assume Reactor
            const Reactor *reactor =
                static_cast<const Reactor *>(mmo.second.get());

            if (reactor && reactor->is_hittable()
                && reactor->is_in_range(range)) {
                int32_t oid = reactor->get_oid();
                uint16_t distance = reactor->get_position().distance(origin);
                distances.emplace(distance, oid);
            }
        }
    }

    std::vector<int32_t> targets;

    for (auto &iter : distances) {
        if (targets.size() >= objcount) {
            break;
        }

        targets.push_back(iter.second);
    }

    return targets;
}

void Combat::apply_use_movement(const SpecialMove &move) {
    const auto id=move.get_id();
    if(adventurer::teleport(id) || id==4111006 || id==5201005 || id==5201006) player_.skill_movement(id,Stage::get().get_physics());
    switch (move.get_id()) {
        case SkillId::Id::TELEPORT_FP:
        case SkillId::Id::IL_TELEPORT:
        case SkillId::Id::PRIEST_TELEPORT:
        case SkillId::Id::FLASH_JUMP:
        default: break;
    }
}

void Combat::apply_result_movement(const SpecialMove &move,
                                   const AttackResult &result) {
    switch (move.get_id()) {
        case SkillId::Id::RUSH_HERO:
        case SkillId::Id::RUSH_PALADIN:
        case SkillId::Id::RUSH_DK:
        case 21100002: apply_rush(result); break;
        default: break;
    }
}

void Combat::apply_rush(const AttackResult &result) {
    if (result.mobcount == 0) {
        return;
    }

    Point<int16_t> mob_position = mobs_.get_mob_position(result.last_oid);
    int16_t targetx = mob_position.x();
    player_.rush(targetx);
}

void Combat::apply_bullet_effect(const BulletEffect &effect) {
    bullets_.push_back(effect);

    if (bullets_.back().bullet.settarget(effect.target)) {
        apply_damage_effect(effect.damageeffect);
        bullets_.pop_back();
    }
}

void Combat::apply_damage_effect(const DamageEffect &effect) {
    Point<int16_t> head_position =
        mobs_.get_mob_head_position(effect.target_oid);
    damage_numbers_.push_back(effect.number);
    damage_numbers_.back().set_x(head_position.x());

    const SpecialMove &move = get_move(effect.move_id);
    mobs_.apply_damage(effect.target_oid,
                       effect.damage,
                       effect.toleft,
                       effect.user,
                       move);
}

void Combat::push_attack(const AttackResult &attack) {
    attack_results_.push(400, attack);
}

void Combat::apply_attack(const AttackResult &attack) {
    if (auto ouser = chars_.get_char(attack.attacker)) {
        OtherChar &user = *ouser;
        user.update_skill(attack.skill, attack.level);
        user.update_speed(attack.speed);

        const SpecialMove &move = get_move(attack.skill);
        move.apply_useeffects(user);

        if (Stance::Id stance = Stance::by_id(attack.stance)) {
            user.attack(stance);
        } else {
            move.apply_actions(user, attack.type);
        }

        user.set_afterimage(attack.skill);

        extract_effects(user, move, attack);
    }
}

void Combat::extract_effects(const Char &user,
                             const SpecialMove &move,
                             const AttackResult &result) {
    AttackUser attack_user = { user.get_skilllevel(move.get_id()),
                               user.get_level(),
                               user.is_twohanded(),
                               !result.toleft };
    if (&user==&player_ && player_.has_buff(Buffstat::SMART_KNOCKBACK)) {
        attack_user.knockback_ratio=nl::nx::skill["2111.img"]["skill"]["21111001"]["level"][player_.get_skilllevel(21111001)]["x"].get_integer(100)/100.0f;
    }

    attack_user.origin=user.get_position();
    if (result.bullet) {
        Bullet bullet { move.get_bullet(user, result.bullet),
                        user.get_position(),
                        result.toleft };

        for (const auto &[oid, vec] : result.damage_lines) {
            if (mobs_.contains(oid)) {
                std::vector<DamageNumber> numbers = place_numbers(oid, vec);
                Point<int16_t> head = mobs_.get_mob_head_position(oid);

                size_t i = 0;

                for (auto &number : numbers) {
                    DamageEffect effect {
                        attack_user,   number, vec[i].first,
                        result.toleft, oid,    move.get_id()
                    };

                    bullet_effects_.emplace(user.get_attackdelay(i),
                                            std::move(effect),
                                            bullet,
                                            head);
                    i++;
                }
            }
        }

        if (result.damage_lines.empty()) {
            int16_t xshift = result.toleft ? -400 : 400;
            Point<int16_t> target =
                user.get_position() + Point<int16_t>(xshift, -26);

            for (int i = 0; i < result.hitcount; i++) {
                DamageEffect effect { attack_user, {}, 0, false, 0, 0 };
                bullet_effects_.emplace(user.get_attackdelay(i),
                                        std::move(effect),
                                        bullet,
                                        target);
            }
        }
    } else {
        for (const auto &[oid, vec] : result.damage_lines) {
            if (mobs_.contains(oid)) {
                std::vector<DamageNumber> numbers = place_numbers(oid, vec);

                size_t i = 0;

                for (auto &number : numbers) {
                    damage_effects_.emplace(user.get_attackdelay(i),
                                            attack_user,
                                            number,
                                            vec[i].first,
                                            result.toleft,
                                            oid,
                                            move.get_id());

                    i++;
                }
            }
        }
    }
}

std::vector<DamageNumber> Combat::place_numbers(
    int32_t oid,
    const std::vector<std::pair<int32_t, bool>> &damage_lines) {
    std::vector<DamageNumber> numbers;
    int16_t head = mobs_.get_mob_head_position(oid).y();

    for (const auto &[amount, is_critical] : damage_lines) {
        DamageNumber::Type type = is_critical ? DamageNumber::Type::CRITICAL
                                              : DamageNumber::Type::NORMAL;
        numbers.emplace_back(type, amount, head);

        head -= DamageNumber::row_height(is_critical);
    }

    return numbers;
}

void Combat::show_buff(int32_t cid, int32_t skillid, int8_t level) {
    if (auto ouser = chars_.get_char(cid)) {
        OtherChar &user = *ouser;
        user.update_skill(skillid, level);

        const SpecialMove &move = get_move(skillid);
        move.apply_useeffects(user);
        move.apply_actions(user, Attack::Type::MAGIC);
    }
}

void Combat::show_player_buff(int32_t skillid) {
    get_move(skillid).apply_useeffects(player_);
}

void Combat::show_affected_by_buff(int32_t cid, int32_t skillid, int8_t level) {
    if (auto ouser = chars_.get_char(cid)) {
        OtherChar &user = *ouser;
        user.update_skill(skillid, level);

        const SpecialMove &move = get_move(skillid);
        move.apply_affected_effects(user);
    }
}

void Combat::show_player_affected_by_buff(int32_t skillid) {
    get_move(skillid).apply_affected_effects(player_);
}

void Combat::give_foreign_buff(int32_t cid, int32_t skillid, int8_t level) {
    if (auto ouser = chars_.get_char(cid)) {
        OtherChar &user = *ouser;
        user.update_skill(skillid, level);

        const SpecialMove &move = get_move(skillid);
        user.add_recurring_effect(
            1,
            nl::nx::skill["MobSkill.img"][skillid]["level"][level]["affected"],
            1);
    }
}

void Combat::show_player_disease(int32_t skillid, int8_t level) {
    player_.add_recurring_effect(
        1,
        nl::nx::skill["MobSkill.img"][skillid]["level"][level]["affected"],
        1);
}

const SpecialMove &Combat::get_move(int32_t move_id) {
    if (move_id == 0) {
        return regular_attack_;
    }

    auto iter = skills_.find(move_id);

    if (iter == skills_.end()) {
        iter = skills_.emplace(move_id, move_id).first;
    }

    return iter->second;
}
}  // namespace ms
