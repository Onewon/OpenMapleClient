// PacketCreator.giveBuff(1, skillId, HOMING_BEACON): OID is repeated at full width in the tail.
// AGPL-3.0-or-later.
#pragma once
#include "InPacket.h"
#include "../Character/Buff.h"
namespace ms {
inline bool decode_homing_buff(InPacket &recv,Buff &buff) {
    auto probe=recv;
    if(probe.read_long()!=0x80000000000000ULL || probe.read_long()!=0)return false;
    probe.skip(2);const auto source=probe.read_int();const auto skill=probe.read_int();probe.skip(5);const auto target=probe.read_int();probe.skip(3);
    if(source!=1 || target<=0 || (skill!=5211006 && skill!=5220011))throw PacketError("Invalid homing beacon source or target");
    buff={Buffstat::HOMING_BEACON,target,skill,0};recv=probe;return true;
}
}
