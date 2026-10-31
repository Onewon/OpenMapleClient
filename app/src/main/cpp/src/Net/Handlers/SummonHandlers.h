// v83 summon packets. AGPL-3.0-or-later.
#pragma once
#include "../PacketHandler.h"
#include "../../Gameplay/Stage.h"
namespace ms {
template<int Opcode> class SummonHandler : public PacketHandler {
    void handle(InPacket &recv) const override {Stage::get().get_combat().handle_summon(Opcode,recv);}
};
template<int Opcode> class AreaHandler : public PacketHandler {
    void handle(InPacket &recv) const override {Stage::get().get_combat().handle_area(Opcode,recv);}
};
}
