// Legacy Meso Explosion: drop selection, per-pile fixed-position effects, per-mob damage and wire contract.
// AGPL-3.0-or-later. Formula recovered from the user's local PC executable; see audit evidence.
#include "Combat.h"
#include "CombatMath.h"
#include "../Stage.h"
#include "../../Data/SkillData.h"
#include "../../Net/Packets/AttackAndSkillPackets.h"
#include <nlnx/nx.hpp>
#include <android/log.h>
namespace ms {
void Combat::use_meso_explosion(const SpecialMove &move) {
    Attack a=player_.prepare_attack(true);a.type=Attack::CLOSE;a.skill=4211006;a.bullet=0;
    const int level=player_.get_skilllevel(a.skill);const auto stats=SkillData::get(a.skill).get_stats(level);
    const auto node=nl::nx::skill["421.img"]["skill"]["4211006"];
    const auto pos=player_.get_position();auto local=stats.range;
    Rectangle<int16_t> selection=a.toleft?Rectangle<int16_t>(pos.x()+local.left(),pos.x()+local.right(),pos.y()+local.top(),pos.y()+local.bottom()):Rectangle<int16_t>(pos.x()-local.right(),pos.x()-local.left(),pos.y()+local.top(),pos.y()+local.bottom());
    auto coins=Stage::get().get_drops().select_mesos(player_.get_oid(),selection,pos,std::min<int>(20,stats.attackcount));
    if(coins.empty()) {__android_log_print(ANDROID_LOG_INFO,"MesoExplosion","No eligible coins");return;}
    move.apply_actions(player_,Attack::CLOSE);move.apply_useeffects(player_);
    AttackResult result(a);result.attacker=player_.get_oid();result.hitcount=1;result.first_oid=result.last_oid=0;
    const int skill_x=node["level"][level]["x"].get_integer();
    for(const auto &[oid,obj]:*mobs_.get_mobs()) {
        auto &mob=static_cast<Mob&>(*obj);if(!mob.is_alive())continue;
        std::vector<std::pair<int32_t,bool>> lines;
        for(const auto &coin:coins) {
            const auto cp=coin.position;
            if(!mob.is_in_range({static_cast<int16_t>(cp.x()-50),static_cast<int16_t>(cp.x()+50),static_cast<int16_t>(cp.y()-50),static_cast<int16_t>(cp.y()+50)}))continue;
            const auto bounds=combat_math::meso_explosion_damage(coin.amount,skill_x);
            const int damage=mob.is_invincible()?0:mob.status(0x40000)?1:static_cast<int>(randomizer_.next_real(bounds.first,bounds.second));
            lines.emplace_back(damage,false);if(lines.size()==15)break;
        }
        if(lines.empty())continue;
        result.damage_lines.emplace(oid,std::move(lines));if(!result.mobcount)result.first_oid=oid;
        result.last_oid=oid;if(++result.mobcount>=stats.mobcount)break;
    }
    const auto now=std::chrono::steady_clock::now();std::vector<int32_t> ids;
    for(size_t i=0;i<coins.size();++i) {ids.push_back(coins[i].oid);meso_visuals_.push_back({coins[i].position,Animation(node["hit"]["0"]),now+std::chrono::milliseconds(100*i)});}
    MesoExplosionPacket(result,ids).dispatch();
    AttackUser user{level,player_.get_level(),player_.is_twohanded(),!a.toleft};user.origin=pos;
    for(const auto &[oid,lines]:result.damage_lines) {
        auto numbers=place_numbers(oid,lines);
        for(size_t i=0;i<lines.size();++i)damage_effects_.emplace(100*i,DamageEffect{user,numbers[i],lines[i].first,a.toleft,oid,a.skill});
    }
    __android_log_print(ANDROID_LOG_INFO,"MesoExplosion","coins=%zu mobs=%u",coins.size(),result.mobcount);
}
}
