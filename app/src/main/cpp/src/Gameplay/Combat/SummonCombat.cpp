// v83 summon lifecycle, movement and owner-authoritative attacks. AGPL-3.0-or-later.
#include "Combat.h"
#include "../Stage.h"
#include "../../Net/InPacket.h"
#include "../../Net/OutPacket.h"
#include "../../Net/Packets/MovementPacket.h"
#include "../../Net/Handlers/Helpers/MovementParser.h"
#include <nlnx/nx.hpp>
#include <cmath>
#include <android/log.h>
namespace ms {
namespace {
class SummonMovePacket : public MovementPacket {
public: SummonMovePacket(int32_t oid,Point<int16_t> before,Point<int16_t> pos,uint8_t stance):MovementPacket(static_cast<Opcode>(175)) {
    write_int(oid);write_point(before);write_byte(1);write_movement(Movement(pos.x(),pos.y(),0,0,stance,200));
}};
class SummonAttackPacket : public OutPacket {
public: SummonAttackPacket(int32_t oid,Point<int16_t> pos,const AttackResult &result):OutPacket(static_cast<Opcode>(176)) {
    write_int(oid);write_time();write_byte(result.toleft?4:0);write_byte(result.mobcount);write_point(pos);write_point(pos);
    for(const auto &[target,lines]:result.damage_lines) {write_int(target);skip(18);write_int(lines.empty()?0:lines.front().first);}
}};
class SummonHurtPacket : public OutPacket {
public: SummonHurtPacket(int32_t oid,int32_t damage,int32_t mob):OutPacket(static_cast<Opcode>(177)) {write_int(oid);write_byte(-1);write_int(damage);write_int(mob);}
};
}
void Combat::clear_summons() {summons_.clear();areas_.clear();meso_visuals_.clear();door_next_={};}
void Combat::handle_summon(int opcode,InPacket &recv) {
    const int owner=recv.read_int(),oid=recv.read_int();
    if(opcode==175) {
        SummonState s;s.owner=owner;s.oid=oid;s.skill=recv.read_int();recv.skip(1);s.level=recv.read_ubyte();s.position=recv.read_point();s.left=recv.read_ubyte()%2;recv.skip(2);s.movement=recv.read_ubyte();s.can_attack=recv.read_bool();recv.skip(1);
        auto src=nl::nx::skill[std::to_string(s.skill/10000)+".img"]["skill"][std::to_string(s.skill)]["summon"];
        if(src["stand"].size()) s.idle=Animation(src["stand"]);else if(src["fly"].size()) s.idle=Animation(src["fly"]);
        if(src["attack1"].size()) s.attack=Animation(src["attack1"]);else s.can_attack=false;
        s.next_attack=std::chrono::steady_clock::now()+std::chrono::milliseconds(1000);
        __android_log_print(ANDROID_LOG_INFO,"SummonCombat","Spawn owner=%d skill=%d oid=%d",owner,s.skill,oid);
        summons_.insert_or_assign(oid,std::move(s));return;
    }
    auto it=summons_.find(oid);if(it==summons_.end() || it->second.owner!=owner)return;
    auto &s=it->second;
    if(opcode==176) {summons_.erase(it);return;}
    if(opcode==177) {
        recv.read_point();auto moves=MovementParser::parse_movements(recv);
        for(const auto &m:moves) if(m.type==Movement::ABSOLUTE) {s.position={m.xpos,m.ypos};s.left=m.newstate%2;}return;
    }
    if(opcode==178) {
        recv.skip(1);s.left=recv.read_ubyte()&4;const int count=recv.read_ubyte();s.attacking=true;s.attack.reset();
        for(int i=0;i<count;++i) {const int target=recv.read_int();recv.skip(1);const int damage=recv.read_int();
            if(mobs_.contains(target)) {auto pos=mobs_.get_mob_head_position(target);damage_numbers_.emplace_back(DamageNumber::NORMAL,damage,pos.y(),pos.x());}
        }
    }
}
void Combat::draw_summons(double x,double y,float alpha) const {
    for(const auto &[id,a]:areas_) {
        if(a.door) a.animation.draw(DrawArgument(a.position+Point<int16_t>(x,y)),alpha);
        else for(int tx=a.bounds.left()+50;tx<a.bounds.right();tx+=100)
            for(int ty=a.bounds.top()+40;ty<a.bounds.bottom();ty+=70)
                a.animation.draw(DrawArgument(Point<int16_t>(tx+x,ty+y),0.65f),alpha);
    }
    for(const auto &[oid,s]:summons_) (s.attacking?s.attack:s.idle).draw(DrawArgument(s.position+Point<int16_t>(x,y),s.left),alpha);
}
void Combat::update_summons() {
    const auto now=std::chrono::steady_clock::now();
    for(auto &[id,a]:areas_) a.animation.update();
    for(auto &[oid,s]:summons_) {
        if(s.attacking) {if(s.attack.update()){s.attacking=false;s.idle.reset();}} else s.idle.update();
        const bool mine=s.owner==player_.get_oid();
        if(!mine || !player_.get_stats().get_stat(MapleStat::HP))continue;
        if(s.movement && now>=s.next_move) {
            s.next_move=now+std::chrono::milliseconds(200);
            const auto before=s.position;auto anchor=player_.get_position();
            if(s.movement==3) anchor+=Point<int16_t>(-35,-65);else anchor+=Point<int16_t>(-45,0);
            if(before.distance(anchor)>60) {s.left=before.x()>anchor.x();s.position=anchor;SummonMovePacket(oid,before,s.position,s.left?1:0).dispatch();}
        }
        const auto src=nl::nx::skill[std::to_string(s.skill/10000)+".img"]["skill"][std::to_string(s.skill)];
        const auto level=src["level"][s.level];
        if(!s.can_attack) {
            if((s.skill==3111002 || s.skill==3211002) && now>=s.next_hurt) {
                s.next_hurt=now+std::chrono::seconds(1);
                for(const auto &[mobid,obj]:*mobs_.get_mobs()) {const auto &mob=static_cast<const Mob&>(*obj);
                    if(mob.is_alive() && mob.is_in_range(Rectangle<int16_t>(s.position.x()-25,s.position.x()+25,s.position.y()-60,s.position.y()))) {auto touch=mob.create_touch_attack();if(touch.valid)SummonHurtPacket(oid,std::max(1,touch.watk),touch.mobid).dispatch();break;}
                }
            }
            continue;
        }
        if(now<s.next_attack)continue;
        auto info=src["summon"]["attack1"]["info"];
        Rectangle<int16_t> local(info["range"]);
        int reach=local.empty()?300:std::max(std::abs(local.left()),std::abs(local.right()));
        const auto targets=find_closest(mobs_.get_mobs(),Rectangle<int16_t>(s.position.x()-reach,s.position.x()+reach,s.position.y()-160,s.position.y()+100),s.position,info["mobCount"].get_integer(1),true);
        if(targets.empty()) {s.next_attack=now+std::chrono::milliseconds(200);continue;}
        const auto nearest=mobs_.get_mob_position(targets.front());s.left=nearest.x()<s.position.x();
        Attack a=player_.prepare_attack(true);a.critical=0;a.hitcount=1;a.mobcount=info["mobCount"].get_integer(1);a.skill=s.skill;
        const int pad=level["pad"].get_integer();
        double maxdamage;
        if(pad) maxdamage=a.maxdamage*(a.maxdamage>=438?0.054:0.077)*pad;
        else {
            int base=std::max(14,a.magic);const int intelligence=a.intelligence;
            if(intelligence>2000) base=base-2000+int(0.09033024267*intelligence+3823.8038);
            else base=base-intelligence+int(0.1996049769*std::pow(intelligence,intelligence>1700?1.300631341:1.290631341));
            maxdamage=(base*107/100)*0.05*level["mad"].get_integer();
        }
        a.maxdamage=std::max(1.0,maxdamage);a.mindamage=a.maxdamage*0.8;a.damagetype=pad?Attack::DMG_WEAPON:Attack::DMG_MAGIC;
        AttackResult result(a);result.toleft=s.left;mobs_.send_attack(result,a,targets,a.mobcount);SummonAttackPacket(oid,s.position,result).dispatch();
        for(const auto &[target,lines]:result.damage_lines) if(!lines.empty()) {
            const auto pos=mobs_.get_mob_head_position(target);damage_numbers_.emplace_back(DamageNumber::NORMAL,lines.front().first,pos.y(),pos.x());
            if(auto mob=mobs_.get_mobs()->get<Mob>(target)) mob->get().apply_damage(lines.front().first,s.left);
        }
        s.attacking=true;s.attack.reset();
        const int delay=std::max(600,int(s.attack.getdelayuntil(32767))+int(info["attackAfter"].get_integer(500)));
        s.next_attack=now+std::chrono::milliseconds(delay);
        __android_log_print(ANDROID_LOG_INFO,"SummonCombat","Attack skill=%d mobs=%d",s.skill,result.mobcount);
    }
}
void Combat::handle_area(int opcode,InPacket &recv) {
    if(opcode==274) {areas_.erase(recv.read_int());return;}
    if(opcode==276) {recv.skip(1);areas_.erase(recv.read_int());return;}
    AreaState a;int32_t id=0;
    if(opcode==273) {
        id=recv.read_int();const int type=recv.read_int();a.owner=recv.read_int();a.skill=recv.read_int();
        recv.skip(3);const int l=recv.read_int(),t=recv.read_int(),r=recv.read_int(),b=recv.read_int();recv.skip(4);
        if(type!=1 && type!=2)return;
        if(l>r || t>b || r-l>3000 || b-t>3000)return;
        a.bounds={static_cast<int16_t>(l),static_cast<int16_t>(r),static_cast<int16_t>(t),static_cast<int16_t>(b)};
        auto src=nl::nx::skill[std::to_string(a.skill/10000)+".img"]["skill"][std::to_string(a.skill)];
        a.animation=Animation(src["tile"]["0"]);
    } else {
        a.door=true;a.skill=2311002;
        if(opcode==67) {
            const int town=recv.read_int(),target=recv.read_int();
            if(town==999999999 || target==999999999) {areas_.erase(-1);return;}
            auto mapname=std::to_string(town);if(mapname.size()<9)mapname=std::string(9-mapname.size(),'0')+mapname;
            a.town=nl::nx::map["Map"]["Map"+std::to_string(town/100000000)][mapname+".img"]["info"]["town"].get_bool();
            // The field-side owner packet follows separately. Town portals omit the owner ID.
            if(!a.town)return;
            id=-1;a.owner=player_.get_oid();recv.read_point(); // opposite field position, not the town door position
            std::map<int,nl::node> doors;
            for(const auto &portal:nl::nx::map["Map"]["Map"+std::to_string(town/100000000)][mapname+".img"]["portal"])
                if(portal["pt"].get_integer()==6)doors.emplace(std::stoi(portal.name()),portal);
            if(doors.empty())return;
            int slot=std::max(0,int(player_.get_stats().get_portal())-128);if(slot>=doors.size())slot=0;
            auto portal=doors.begin();std::advance(portal,slot);a.position={portal->second["x"],portal->second["y"]};
        } else {recv.skip(1);id=a.owner=recv.read_int();a.position=recv.read_point();}
        a.animation=Animation(nl::nx::skill["231.img"]["skill"]["2311002"]["mDoor"]);
    }
    __android_log_print(ANDROID_LOG_INFO,"SkillArea","Spawn skill=%d id=%d door=%d x=%d y=%d",a.skill,id,a.door,a.position.x(),a.position.y());
    areas_.insert_or_assign(id,std::move(a));
}
bool Combat::use_door(Point<int16_t> position) {
    const auto now=std::chrono::steady_clock::now();
    for(const auto &[id,a]:areas_) {
        if(!a.door || std::abs(position.x()-a.position.x())>35 || position.y()<a.position.y()-100 || position.y()>a.position.y()+30)continue;
        if(now<door_next_)return true;
        class DoorPacket:public OutPacket {
        public:DoorPacket(int32_t owner,bool town):OutPacket(static_cast<Opcode>(133)){write_int(owner);write_byte(town);}
        };
        __android_log_print(ANDROID_LOG_INFO,"SkillArea","UseDoor owner=%d town=%d",a.owner,a.town);
        DoorPacket(a.owner,a.town).dispatch();door_next_=now+std::chrono::seconds(1);return true;
    }
    return false;
}

}
