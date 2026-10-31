//	This file is part of the continued Journey MMORPG client
//	Copyright (C) 2015-2019  Daniel Allendorf, Ryan Payton
//
//	This program is free software: you can redistribute it and/or modify
//	it under the terms of the GNU Affero General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU Affero General Public License for more details.
//
//	You should have received a copy of the GNU Affero General Public License
//	along with this program.  If not, see <https://www.gnu.org/licenses/>.
#include "CashShopHandlers.h"
#include "../../Data/CashShopData.h"
#include "../../Gameplay/Stage.h"
#include "../../IO/UI.h"
#include "../../IO/Window.h"
#include "../../Timer.h"
#include "Helpers/CashShopParser.h"
#include "Helpers/ItemParser.h"
#include <algorithm>
#include <android/log.h>
namespace ms {
namespace {
CashShopData::LockerItem cash_item(InPacket& p) {
    CashShopData::LockerItem i;
    i.cashid=p.read_long(); p.skip(8); i.item=p.read_int();
    i.sn=p.read_int(); i.count=p.read_ushort(); p.skip(29);
    return i;
}
void add_locker(const CashShopData::LockerItem& i) {
    auto& items=CashShopData::locker;
    auto found=std::find_if(items.begin(),items.end(),[&](const auto& x){return x.cashid==i.cashid;});
    if(found==items.end()) items.push_back(i); else *found=i;
}
}
void SetCashShopHandler::handle(InPacket& p) const {
    CashShopParser::parseCharacterInfo(p);
    CashShopData::load();
    p.skip_byte(); p.skip_string(); p.skip_int();
    const auto count=p.read_ushort();
    for(unsigned i=0;i<count;++i) {
        const auto sn=p.read_int(); const auto flags=p.read_uint();
        auto& o=CashShopData::offers[sn]; o.sn=sn;
        if(flags&1) o.item=p.read_int();
        if(flags&2) o.count=p.read_ushort();
        if(flags&16) p.skip_byte(); // priority precedes price in v83
        if(flags&4) o.price=p.read_int();
        if(flags&8) p.skip_byte();
        if(flags&32) o.period=p.read_ushort();
        if(flags&64) p.skip_int();
        if(flags&128) p.skip_int();
        if(flags&256) p.skip_byte();
        if(flags&512) o.gender=p.read_ubyte();
        if(flags&1024) o.sale=p.read_ubyte();
        if(flags&2048) p.skip_byte();
        if(flags&4096) p.skip_byte();
        if(flags&8192) p.skip_short();
        if(flags&16384) p.skip_short();
        if(flags&32768) p.skip_short();
        if(flags&65536) { const auto n=p.read_ubyte(); p.skip(4*n); }
    }
    ++CashShopData::revision;
    transition();
    UI::get().change_state(UI::State::CASHSHOP);
    __android_log_print(ANDROID_LOG_INFO,"CashShop","Entered v83 shop; catalogue=%zu modifications=%u",CashShopData::offers.size(),count);
}
void SetCashShopHandler::transition() const {
    UI::get().release_keys();
    Constants::Constants::get().set_viewwidth(1024);
    Constants::Constants::get().set_viewheight(768);
    GraphicsGL::get().clear();
    Stage::get().clear();
    GraphicsGL::get().unlock();
    UI::get().enable();
    Timer::get().start();
}
void CashQueryHandler::handle(InPacket& p) const {
    using namespace CashShopData;
    credit=p.read_int(); point=p.read_int(); prepaid=p.read_int();
    if(pending_sn) {pending_sn=0;status="Server updated balance. Check locker for delivery or rejection.";}
    if(pending_take) {pending_take=0;status="Withdrawal rejected. Check inventory space and try again.";}
    exiting=false;
    ++revision;
    UI::get().enable();
}
void CashOperationHandler::handle(InPacket& p) const {
    using namespace CashShopData;
    const auto action=p.read_ubyte();
    __android_log_print(ANDROID_LOG_INFO,"CashShop","Operation reply=%02x remaining=%zu",action,p.length());
    switch(action) {
        case 0x4b: {
            locker.clear();const auto n=p.read_ushort();
            for(unsigned i=0;i<n;++i) add_locker(cash_item(p));
            break;
        }
        case 0x57:
            add_locker(cash_item(p)); pending_sn=0;status="Purchase completed. Open Locker to take the item.";break;
        case 0x89: {
            const auto n=p.read_ubyte();
            for(unsigned i=0;i<n;++i) add_locker(cash_item(p));
            pending_sn=0;status="Package delivered to Locker.";break;
        }
        case 0x5c:
            status="Purchase rejected by server (code "+std::to_string(p.read_ubyte())+").";
            pending_sn=0;pending_take=0;break;
        case 0x68: {
            const auto it=std::find_if(locker.begin(),locker.end(),[](const auto& x){return x.cashid==pending_take;});
            if(it!=locker.end()) {
                const int slot=p.read_short();
                ItemParser::parse_item(p,InventoryType::by_value(it->item/1000000),slot,Stage::get().get_player().get_inventory());
                locker.erase(it);
            }
            pending_take=0; status="Item moved to game inventory.";break;
        }
        default: break; // wishes and gifts are separate optional features
    }
    ++revision; UI::get().enable();
}
}