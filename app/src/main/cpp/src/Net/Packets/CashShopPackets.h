#pragma once
#include "../OutPacket.h"
namespace ms {
class CashPurchasePacket : public OutPacket {
public:
    CashPurchasePacket(int sn, int currency, bool package) : OutPacket(static_cast<Opcode>(229)) {
        write_byte(package ? 0x1e : 0x03);
        write_byte(0);
        write_int(currency);
        write_int(sn);
    }
};
class CashTakePacket : public OutPacket {
public:
    explicit CashTakePacket(int64_t id) : OutPacket(static_cast<Opcode>(229)) {
        write_byte(0x0d); write_int(static_cast<int32_t>(id));
    }
};
}
