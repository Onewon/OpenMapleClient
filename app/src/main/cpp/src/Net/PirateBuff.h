// v83 givePirateBuff layout (PacketCreator). AGPL-3.0-or-later.
#pragma once
#include "InPacket.h"
#include "../Character/Buff.h"
#include <vector>
namespace ms {
inline bool decode_pirate_buff(InPacket &recv,std::vector<Buff> &result) {
    auto probe=recv;
    const uint64_t first=probe.read_long(),second=probe.read_long();
    constexpr uint64_t dash2=0x8000000000000ULL,dash=0x10000000000000ULL,infusion=0x40000000000000ULL;
    if(second || !first || (first&~(dash2|dash|infusion))) return false;
    probe.skip(2);
    for(auto entry:{std::pair<uint64_t,Buffstat::Id>{dash2,Buffstat::DASH2},{dash,Buffstat::DASH},{infusion,Buffstat::SPEED_INFUSION}}) {
        if(!(first&entry.first))continue;
        const auto value=probe.read_int();const auto skill=probe.read_int();
        probe.skip(entry.first==infusion ? 10:5);
        const int duration=probe.read_ushort()*1000;
        result.push_back({entry.second,static_cast<int16_t>(value),skill,duration});
    }
    probe.skip(3);recv=probe;return true;
}
}
