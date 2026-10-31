// v83 skill formulas. AGPL-3.0-or-later.
#pragma once
#include <algorithm>
#include <utility>
namespace ms::combat_math {
// Recovered from local BeiDou.exe 0x791FBC..0x79215B. Constants are recorded in the audit evidence.
inline std::pair<double,double> meso_explosion_damage(int mesos,int skill_x) {
    if(mesos<=0 || skill_x<=0)return {0,0};
    const double ratio=mesos<=1000?(mesos*0.82+28.0)/5300.0:double(mesos)/(double(mesos)+5250.0);
    const double maximum=skill_x*50.0*ratio;return {maximum*0.5,maximum};
}
inline std::pair<double,double> throwing_damage(double luck,double weapon_attack) {
    const double unit=std::max(0.0,luck)*std::max(0.0,weapon_attack)/100.0;
    return {unit*2.5,unit*5.0};
}
inline std::pair<double,double> magic_damage(double magic, double intelligence, double skill_attack, int mastery) {
    magic=std::max(0.0,magic);skill_attack=std::max(0.0,skill_attack);
    const double trained=std::clamp(0.1+0.05*mastery,0.1,0.9);
    const double square=magic*magic/1000.0;
    return {((square+magic*trained*0.9)/30.0+intelligence/200.0)*skill_attack,
            ((square+magic)/30.0+intelligence/200.0)*skill_attack};
}
}
