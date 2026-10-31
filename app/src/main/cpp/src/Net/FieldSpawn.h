// SET_FIELD post-portal spawn contract for the paired v83 server. AGPL-3.0-or-later.
#pragma once
#include "InPacket.h"
#include <optional>
namespace ms {
struct FieldSpawn {uint16_t hp;std::optional<Point<int16_t>> position;};
inline FieldSpawn decode_field_spawn(InPacket &recv) {
    FieldSpawn spawn{recv.read_ushort(),{}};
    if(recv.read_bool()) {const auto x=recv.read_int(),y=recv.read_int();spawn.position=Point<int16_t>(x,y);}
    return spawn;
}
}
