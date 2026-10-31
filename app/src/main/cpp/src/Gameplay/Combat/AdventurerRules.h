// v83 adventurer combat contracts. AGPL-3.0-or-later.
#pragma once
#include <algorithm>
#include <cctype>
#include <string_view>
namespace ms::adventurer {
// BeiDou.exe 0x500971 chooses 1000ms; 0x790494 and 0x791EBB scale physical/magic damage.
constexpr int charge_time(int milliseconds) {return milliseconds<1?1:milliseconds>1000?1000:milliseconds;}
constexpr double charge_multiplier(int milliseconds) {return (10+(charge_time(milliseconds)*90)/1000)/100.0;}
inline int mp_cost(int base,int amplification,bool infinity,int reduction) {
    if(infinity)return 0;
    const int amplified=static_cast<int>(std::max(0,base)*std::max(0,amplification)/100.0);
    return std::max(0,amplified-static_cast<int>(amplified*std::clamp(reduction,0,100)/100.0));
}
// Disorder/Threaten apply flat changes. Taunt's defense entry is a percentage.
constexpr int monster_stat(int base,int delta,bool percent=false) {
    return std::max(0,percent?static_cast<int>(base*(100.0+delta)/100.0):base+delta);
}
// PC 0x958768 uses random % 1000 for Guardian; Shadow Shifter uses percent.
constexpr double avoidance_probability(int skill,int prop) {return std::clamp(prop/((skill==1120005 || skill==1220006)?1000.0:100.0),0.0,1.0);}
// PC 0x790087..0x7900a2 adds Elemental Boost to the gun skill's damage percentage.
constexpr double elemental_gun_ratio(double damage,int bonus) {return damage>0?(damage+bonus/100.0)/damage:1.0;}
// Paired server AbstractDealDamageHandler scales Barrage's fifth and sixth hits.
constexpr double hit_multiplier(int skill,int line) {return skill==5121007 && line>=4?(line==4?2.0:4.0):1.0;}
constexpr double finisher_multiplier(int orbs) {return orbs>=5?2.5:orbs==4?2.0:orbs==3?1.54:orbs==2?1.2:1.0;}
constexpr bool upgrade(int id) {return id==5210000 || id==5220002 || id==5220011;}
constexpr bool mortal_blow(int id) {return id==3110001 || id==3210001;}
constexpr int upgraded(int id,bool burst,bool octopus,bool beacon) {
    return id==5001003 && burst?5210000:id==5211001 && octopus?5220002:id==5211006 && beacon?5220011:id;
}
constexpr char charge_element(int id) {
    return id==1211003 || id==1211004?'F':id==1211005 || id==1211006?'I':id==1211007 || id==1211008?'L':id==1221003 || id==1221004?'H':0;
}
inline double elemental_multiplier(std::string_view attributes,char element,bool neutralized=false,double weak=1.5) {
    if(!element || neutralized)return 1.0;
    element=static_cast<char>(std::toupper(static_cast<unsigned char>(element)));
    for(size_t i=0;i+1<attributes.size();i+=2) if(std::toupper(static_cast<unsigned char>(attributes[i]))==element) {
        switch(attributes[i+1]) {case '1':return 0;case '2':return 0.5;case '3':return weak;default:return 1;}
    }
    return 1.0;
}
}
