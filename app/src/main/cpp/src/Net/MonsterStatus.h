// Paired v83 MonsterStatus enum order and entry layout. AGPL-3.0-or-later.
#pragma once
#include "InPacket.h"
#include <vector>
namespace ms {
struct MonsterStatusEntry {uint64_t mask;int16_t value;int32_t source;};
inline std::vector<MonsterStatusEntry> decode_monster_status(InPacket &recv,uint32_t first,uint32_t second) {
    // The server writes an EnumMap in declaration order, with first-mask aliases interleaved.
    constexpr uint64_t order[]={1,2,0x200000000ULL,0x400000000ULL,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768,65536,131072,262144,524288,2097152,4194304,8388608,16777216,33554432,67108864,268435456,0x2000000000000000ULL,0x4000000000000000ULL};
    const uint64_t mask=(static_cast<uint64_t>(first)<<32)|second;
    std::vector<MonsterStatusEntry> result;
    for(uint64_t flag:order) if(mask&flag) {const int16_t value=recv.read_short();const int32_t source=recv.read_int();recv.skip(2);result.push_back({flag,value,source});}
    return result;
}
}
