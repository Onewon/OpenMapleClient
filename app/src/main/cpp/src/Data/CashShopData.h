#pragma once
#include <map>
#include <vector>
#include <string>
#include <nlnx/nx.hpp>

namespace ms::CashShopData {
struct Offer {
    int sn=0, item=0, count=1, price=0, period=0, gender=2, sale=0;
};
struct LockerItem { int64_t cashid=0; int item=0, sn=0, count=1; };
inline std::map<int,Offer> offers;
inline std::vector<LockerItem> locker;
inline int credit=0, point=0, prepaid=0;
inline int pending_sn=0;
inline int64_t pending_take=0;
inline unsigned revision=0;
inline std::string status="Select an item to review before buying.";
inline bool exiting=false;
inline void load() {
    offers.clear(); locker.clear(); credit=point=prepaid=0;
    pending_sn=0; pending_take=0; exiting=false;
    status="Select an item to review before buying.";
    static constexpr Offer catalogue[]={
#include "CashShopCatalog.inc"
    };
    for (const auto& o : catalogue) {
        if(o.sn>0 && o.item>0) offers[o.sn]=o;
    }
    ++revision;
}
}
