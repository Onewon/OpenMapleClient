#include "../app/src/main/cpp/src/Net/TextEncoding.h"
#include "../app/src/main/cpp/src/Net/HomingBuff.h"
// v83 packet contracts and skill state regressions. AGPL-3.0-or-later.
#include "../app/src/main/cpp/src/Character/AdventurerSkills.h"
#include "../app/src/main/cpp/src/Character/SkillCooldowns.h"
#include "../app/src/main/cpp/src/Gameplay/Combat/CombatMath.h"
#include "../app/src/main/cpp/src/Net/Packets/AttackAndSkillPackets.h"
#include "../app/src/main/cpp/src/Net/PirateBuff.h"
#include "../app/src/main/cpp/src/Net/MonsterStatus.h"
#include "../app/src/main/cpp/src/Net/FieldSpawn.h"
#include <cassert>
#include <map>
#include <cmath>
#include <iostream>
#include "../app/src/main/cpp/src/Template/TimedQueue.h"
using namespace ms;
struct Fixture:OutPacket {
 Fixture():OutPacket(32){}
 using OutPacket::write_long;using OutPacket::write_int;using OutPacket::write_short;using OutPacket::skip;
};
static void pirate(bool infusion) {
 Fixture f;f.write_long(infusion?0x40000000000000ULL:0x18000000000000ULL);f.write_long(0);f.write_short(0);
 for(int i=0;i<(infusion?1:2);++i) {f.write_int(infusion?-2:10+i);f.write_int(infusion?5121009:5001005);f.skip(infusion?10:5);f.write_short(20);}
 f.skip(3);auto bytes=f.build();InPacket p(bytes.data(),bytes.size());p.read_short();std::vector<Buff> b;
 assert(decode_pirate_buff(p,b));assert(!p.available());assert(b.size()==(infusion?1:2));
 assert(b[0].skillid==(infusion?5121009:5001005));assert(b[0].value==(infusion?-2:10));assert(b[0].duration==20000);
 bytes.pop_back();InPacket truncated(bytes.data(),bytes.size());truncated.read_short();b.clear();bool rejected=false;
 try {decode_pirate_buff(truncated,b);}catch(const PacketError&){rejected=true;}assert(rejected);
}
int main() {
 std::vector<std::vector<int>> delivered;
 TimedQueue<std::vector<int>> queue([&](const auto &v){delivered.push_back(v);});
 std::vector<int> projectile{1,2,3};queue.emplace(0,projectile);queue.emplace(0,projectile);
 assert(projectile.size()==3);queue.update();assert(delivered.size()==2 && delivered[0].size()==3 && delivered[1].size()==3);

 assert(!Buffstat::matches(0x200000000ULL,0xA00000040ULL));
 assert(Buffstat::matches(0x200000000ULL,0x200000000ULL));
 {
  Fixture f;f.write_short(-20);f.write_int(4001002);f.write_short(-1);f.write_short(1);f.write_int(2121003);f.write_short(-1);f.write_short(1);f.write_int(2211002);f.write_short(-1);
  auto raw=f.build();InPacket p(raw.data(),raw.size());p.skip(2);auto entries=decode_monster_status(p,2,2|256);
  assert(entries.size()==3 && entries[0].mask==2 && entries[0].value==-20 && entries[1].mask==0x200000000ULL && entries[2].source==2211002 && !p.available());
 }
 {
  Fixture f;f.write_short(401);f.skip(1);auto raw=f.build();InPacket p(raw.data(),raw.size());p.skip(2);auto spawn=decode_field_spawn(p);assert(spawn.hp==401 && !spawn.position);
  Fixture g;g.write_short(399);g.write_short(1); // true followed by first position byte (zero)
  g.skip(3);g.write_int(-500);raw=g.build();InPacket q(raw.data(),raw.size());q.skip(2);auto positioned=decode_field_spawn(q);assert(positioned.hp==399 && positioned.position && positioned.position->x()==0 && positioned.position->y()==-500 && !q.available());
 }
 {
  assert(adventurer::mp_cost(50,200,false,0)==100);
  assert(adventurer::mp_cost(50,200,true,0)==0);
  assert(adventurer::mp_cost(9,100,false,25)==7);
  assert(adventurer::mp_cost(50,200,false,100)==0);
  {
   MobAttack attack(500,{20,30},100100,345);MobAttackResult guard(attack,0,1);guard.guarded=true;guard.mob_position={20,30};guard.player_position={60,30};
   auto bytes=TakeDamagePacket(guard,TakeDamagePacket::TOUCH).build();InPacket p(bytes.data(),bytes.size());p.skip(21);
   assert(p.read_ubyte()==0 && p.read_ubyte()==2 && p.read_ubyte()==0 && p.read_int()==345 && p.read_ubyte()==0);
   assert(p.read_point()==Point<int16_t>(20,30) && p.read_point()==Point<int16_t>(60,30) && !p.available());
  }
  assert(adventurer::upgraded(5001003,true,false,false)==5210000);
  assert(adventurer::upgraded(5211001,false,true,false)==5220002);
  assert(adventurer::upgraded(5211006,false,false,true)==5220011);
  assert(adventurer::upgraded(5211006,true,true,false)==5211006);
  assert(adventurer::charge_element(1211004)=='F' && adventurer::charge_element(1221004)=='H');
  assert(adventurer::elemental_multiplier("F3I2H1D4",'f')==1.5);
  assert(adventurer::elemental_multiplier("F3I2H1D4",'I')==0.5);
  assert(adventurer::elemental_multiplier("F3I2H1D4",'H')==0);
  assert(adventurer::elemental_multiplier("F3I2H1D4",'D')==1);
  assert(adventurer::elemental_multiplier("F3I2H1D4",'H',true)==1);
  assert(adventurer::elemental_multiplier("F3I2H1D4",'F',false,1.25)==1.25);
  Fixture f;f.write_long(0x80000000000000ULL);f.write_long(0);f.write_short(1000000200&65535);f.write_int(1);f.write_int(5220011);f.skip(5);f.write_int(1000000200);f.skip(3);
  auto bytes=f.build();InPacket packet(bytes.data(),bytes.size());packet.skip(2);Buff beacon;
  assert(decode_homing_buff(packet,beacon));assert(beacon.value==1000000200 && beacon.skillid==5220011 && !packet.available());
  bytes.pop_back();InPacket truncated(bytes.data(),bytes.size());truncated.skip(2);bool rejected=false;try{decode_homing_buff(truncated,beacon);}catch(const PacketError&){rejected=true;}assert(rejected);
 }
 assert(!adventurer::uses_ammo(3101002) && !adventurer::uses_ammo(3101004) && !adventurer::uses_ammo(4111004));
 assert(adventurer::uses_ammo(3121004) && adventurer::uses_ammo(4121006) && !adventurer::uses_ammo(3101003));
 {
  MobAttack a(100,{0,0},100100,123);MobAttackResult r(a,85,0);r.reported_damage=100;
  auto raw=TakeDamagePacket(r,TakeDamagePacket::TOUCH).build();InPacket p(raw.data(),raw.size());p.skip(8);assert(p.read_int()==100 && r.damage==85);
 }
 {
  auto one=combat_math::meso_explosion_damage(1,1000);assert(std::abs(one.second-271.88679245283)<0.00001 && one.first==one.second*0.5);
  auto thousand=combat_math::meso_explosion_damage(1000,1000);assert(std::abs(thousand.second-8000)<0.00001);
  auto large=combat_math::meso_explosion_damage(50000,1000);assert(std::abs(large.second-45248.86877828054)<0.00001);
  auto zero=combat_math::meso_explosion_damage(0,1000);assert(zero.first==0 && zero.second==0);
  for(bool hit:{false,true}) {
   Attack a;a.type=Attack::CLOSE;a.skill=4211006;a.hitcount=1;AttackResult r(a);
   if(hit){r.mobcount=1;for(int i=0;i<15;++i)r.damage_lines[123].push_back({8000+i,false});}
   std::vector<int32_t> coins;for(int i=0;i<20;++i)coins.push_back(1000000000+i);
   auto raw=MesoExplosionPacket(r,coins).build();InPacket packet(raw.data(),raw.size());assert(packet.read_short()==44);packet.skip(1);assert(packet.read_ubyte()==(hit?17:0));assert(packet.read_int()==4211006);packet.skip(11);
   if(hit) {packet.skip(6);assert(packet.read_int()==123);packet.skip(12);assert(packet.read_ubyte()==15);for(int i=0;i<15;++i)assert(packet.read_int()==8000+i);packet.skip(4);assert(packet.read_int()==0);}
   else packet.skip(10);
   assert(packet.read_ubyte()==20);for(int i=0;i<20;++i){assert(packet.read_int()==coins[i]);assert(packet.read_ubyte()==0);}assert(!packet.available());
  }
 }
 assert(adventurer::avoidance_probability(1120005,150)==0.15 && adventurer::avoidance_probability(4120002,30)==0.30);
 assert(adventurer::avoidance_probability(1220006,150)==0.15 && adventurer::avoidance_probability(4220002,40)==0.40);
 assert(std::abs(adventurer::elemental_gun_ratio(1.6,200)-2.25)<1e-9);
 assert(adventurer::hit_multiplier(5121007,3)==1 && adventurer::hit_multiplier(5121007,4)==2 && adventurer::hit_multiplier(5121007,5)==4);
 assert(adventurer::hit_multiplier(1121008,5)==1);
 assert(adventurer::monster_stat(100,-20)==80 && adventurer::monster_stat(10,-20)==0);
 assert(adventurer::monster_stat(800,40,true)==1120);
 assert(adventurer::finisher_multiplier(0)==1 && adventurer::finisher_multiplier(3)==1.54 && adventurer::finisher_multiplier(10)==2.5);
 assert(adventurer::charge_time(4000)==1000 && adventurer::charge_time(0)==1);
 assert(adventurer::charge_multiplier(1)==0.1 && adventurer::charge_multiplier(500)==0.55 && adventurer::charge_multiplier(1000)==1.0);
 {
  const std::string utf8="\xe4\xb8\xad\xe6\x96\x87",gbk="\xd6\xd0\xce\xc4";
  assert(wire_text::decode(gbk)==utf8 && wire_text::encode(utf8)==gbk);
  assert(wire_text::decode("ASCII")=="ASCII");
 }
 using K=adventurer::Kind;
 for(int id:{2111002,2211002,3101005,3221007}) assert(adventurer::attack(id));
 assert(adventurer::kind(4001002)==K::Attack && adventurer::kind(5221009)==K::Attack);
 assert(adventurer::kind(2221006)==K::Attack);assert(adventurer::kind(4121007)==K::Attack);
 assert(adventurer::kind(2121005)==K::Summon);assert(!adventurer::attack(2121005));
 assert(!adventurer::attack(2101003));assert(!adventurer::attack(2111004));
 assert(adventurer::attack_mode(2221006)==Attack::MAGIC);assert(adventurer::attack_mode(4121007)==Attack::RANGED);
 assert(adventurer::attack_mode(5121001)==Attack::CLOSE);assert(adventurer::attack_mode(3101003)==Attack::CLOSE);
 assert(adventurer::hold(3121004) && adventurer::hold(5221004));
 auto now=SkillCooldowns::Clock::time_point{};SkillCooldowns cd;
 cd.set(5221006,90,now);assert(cd.active(5221006,now+std::chrono::seconds(89)));assert(!cd.active(5221006,now+std::chrono::seconds(90)));
 cd.set(5221006,0,now);assert(!cd.active(5221006,now));cd.set(5221006,5,now);cd.set(5221006,10,now);assert(cd.active(5221006,now+std::chrono::seconds(6)));
 auto throwing=combat_math::throwing_damage(200,50);assert(throwing.first==250 && throwing.second==500);
 auto range=combat_math::magic_damage(100,100,50,10);assert(std::abs(range.first-131.6666667)<0.001 && std::abs(range.second-208.3333333)<0.001);
 for(int magic=0;magic<=2000;magic+=10) {auto r=combat_math::magic_damage(magic,magic,50,10);assert(r.first>=0 && r.first<=r.second);}
 for(int id:{2001004,2121001,2221001,2321001,5101004,5201002,3121004,3221001,5221004}) {
  Attack a;a.skill=id;a.type=static_cast<Attack::Type>(adventurer::attack_mode(id));a.hitcount=1;AttackResult result(a);result.charge=0;
  auto raw=AttackPacket(result).build();const bool headerCharge=adventurer::charge(id)&&id!=3221001;
  const bool extraRanged=a.type==Attack::RANGED && (adventurer::hold(id)||id==3221001);
  assert(raw.size()==size_t((a.type==Attack::RANGED?30:25)+(headerCharge?4:0)+(extraRanged?4:0)));
 }
 {
  Attack a;a.type=Attack::RANGED;a.skill=5221004;a.hitcount=1;AttackResult r(a);r.mobcount=2;r.damage_lines[111]={{123,false}};r.damage_lines[222]={{456,false}};
  auto bytes=AttackPacket(r).build();InPacket p(bytes.data(),bytes.size());p.skip(34);
  std::map<int32_t,int32_t> targets;for(int i=0;i<2;++i){const auto oid=p.read_int();p.skip(14);targets[oid]=p.read_int();p.skip(4);}assert(targets[111]==123 && targets[222]==456 && !p.available());
 }
 auto cast=UseSkillPacket(2111003,30,{123,-45}).build();InPacket p(cast.data(),cast.size());assert(p.read_short()==91);p.skip(4);assert(p.read_int()==2111003);assert(p.read_ubyte()==30);assert(p.read_point()==Point<int16_t>(123,-45));assert(p.length()==1);
 auto magnet=UseSkillPacket(1121001,30,{0,0},{111,222},true).build();InPacket m(magnet.data(),magnet.size());m.skip(11);assert(m.read_int()==2);assert(m.read_int()==111 && m.read_ubyte()==1);assert(m.read_int()==222 && m.read_ubyte()==1);assert(m.read_ubyte()==1 && !m.available());
 pirate(false);pirate(true);
 std::cout<<"PASS adventurer dispatch, cooldowns, magic bounds, cast and attack layouts, pirate buffs and truncation\n";
}
