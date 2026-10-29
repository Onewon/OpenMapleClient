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
#include "UICashShop.h"
#include "../../Data/CashShopData.h"
#include "../../Data/ItemData.h"
#include "../../Graphics/Geometry.h"
#include "../../Graphics/Text.h"
#include "../../Gameplay/Stage.h"
#include "../../Net/Packets/CashShopPackets.h"
#include "../../Net/Packets/GameplayPackets.h"
#include "../Components/MapleButton.h"
#include "../Components/Charset.h"
#include <nlnx/nx.hpp>
#include "../UI.h"
#include <algorithm>
#include <android/log.h>
namespace ms {
namespace {
constexpr int EXIT=1, PREV=2, NEXT=3, CONFIRM=4, CANCEL=5, TAB=20, BUY=100;
constexpr int SHOP_PAGE_ITEMS=14;
constexpr int SHOP_TABS=10;
std::string itemname(int id) {
    auto name=ItemData::get(id).get_name();
    return name.empty() ? "Item #"+std::to_string(id) : name;
}
void label(std::string value,int x,int y,Color::Name color=Color::WHITE,int width=0) {
    Text(Text::A12B,Text::LEFT,color,value,width).draw(Point<int16_t>(x,y));
}
}
// Reuse the original cash-locker frame and native checkmarks for shop controls.
class ShopTextButton : public Button {
    Point<int16_t> size_;
    Texture skin_,unchecked_,checked_;
    std::string label_;
    const int* selection_;
    int value_;
public:
    ShopTextButton(int x,int y,int w,int h,const std::string& text,const int* selection=nullptr,int value=0):
        size_(w,h),skin_(nl::nx::ui["CashShop.img"]["CSLockerNew"]["Normal"]["backgrnd"]),
        unchecked_(nl::nx::ui["Basic.img"]["CheckBox0"][0]),checked_(nl::nx::ui["Basic.img"]["CheckBox0"][1]),
        label_(text),selection_(selection),value_(value) {
        position_={static_cast<int16_t>(x),static_cast<int16_t>(y)};active_=true;pressed_=false;state_=NORMAL;
    }
    void draw(Point<int16_t> parent) const override {
        if(!active_)return;
        const auto p=parent+position_;
        const float sx=float(size_.x())/std::max<int16_t>(1,skin_.width());
        // The 80x45 NX frame has 13 transparent rows below its 32px artwork.
        const float sy=float(size_.y())/32.f;
        const auto o=skin_.get_origin();
        skin_.draw(DrawArgument(p+Point<int16_t>(int16_t(o.x()*sx),int16_t(o.y()*sy)),sx,sy));
        const bool chosen=selection_ && *selection_==value_;
        if(selection_) {
            const auto& tick=chosen?checked_:unchecked_;
            tick.draw(p+Point<int16_t>(12,(size_.y()-11)/2)+tick.get_origin());
        }
        Text(Text::A11B,Text::CENTER,chosen || state_==MOUSEOVER?Color::SUPERNOVA:Color::WHITE,label_)
            .draw(p+Point<int16_t>(size_.x()/2+(selection_?7:0),(size_.y()-12)/2-4));
    }
    void update() override {} int16_t width() const override{return size_.x();}
    Point<int16_t> origin() const override{return {};}
    Rectangle<int16_t> bounds(Point<int16_t> p) const override{return {p+position_,p+position_+size_};}
    Cursor::State send_cursor(bool,Point<int16_t>) override{return Cursor::IDLE;}
};
UICashShop::UICashShop() {
    position_={0,0}; dimension_={1024,768};
    auto shop=nl::nx::ui["CashShop.img"];
    preview_=Sprite(shop["Base"]["Preview"][0]);
    sprites_.emplace_back(shop["Base"]["backgrnd"]);
    sprites_.emplace_back(shop["Base"]["BestNew"],Point<int16_t>(139,346));
    sprites_.emplace_back(shop["CSPromotionBanner"]["shadow"],Point<int16_t>(138,40));
    sprites_.emplace_back(shop["CSPromotionBanner"]["basic"],Point<int16_t>(138,40));
    item_base_=Sprite(shop["CSList"]["Base"]);
    item_line_=Sprite(shop["Base"]["line"]);
    auto button=[&](int id,int x,int y,int w,int h,std::string text) {
        buttons_[id]=std::make_unique<ShopTextButton>(x,y,w,h,text);
    };
    buttons_[EXIT]=std::make_unique<MapleButton>(shop["CSTab"]["BtExit"],Point<int16_t>(5,728));
    for(int i=0;i<10;++i) sidebar_[i]=Sprite(shop["CSTab"]["Tab"][i]);
    buttons_[TAB+9]=std::make_unique<ShopTextButton>(5,682,116,34,"Cash Locker",&category_,9);
    button(PREV,143,345,60,24,"<");button(NEXT,290,345,60,24,">");
    button(CONFIRM,688,302,155,30,"Confirm purchase");
    button(CANCEL,850,302,146,30,"Cancel");
    buttons_[10]=std::make_unique<ShopTextButton>(5,583,116,30,"NX Credit",&currency_,1);
    buttons_[11]=std::make_unique<ShopTextButton>(5,615,116,30,"Points",&currency_,2);
    buttons_[12]=std::make_unique<ShopTextButton>(5,647,116,30,"Prepaid",&currency_,4);
    for(int i=0;i<SHOP_PAGE_ITEMS;++i) buttons_[BUY+i]=std::make_unique<MapleButton>(shop["CSList"]["BtBuy"],Point<int16_t>(146+(i%7)*124,523+(i/7)*205));
    refresh();
}
// A tap must activate on its first down event, without requiring mouse hover.
Cursor::State UICashShop::send_cursor(bool down,Point<int16_t> pos) {
    int hit=-1;
    for(auto& [id,button]:buttons_) {
        const bool inside=button->is_active() && button->bounds(position_).contains(pos);
        button->set_state(inside?Button::MOUSEOVER:Button::NORMAL);
        if(inside) hit=id;
    }
    if(pos.x()>=0 && pos.x()<126 && pos.y()>=63 && pos.y()<396) hit=TAB+std::min(8,int((pos.y()-63)/37));
    if(down && !pointer_down_ && hit>=0) button_pressed(static_cast<uint16_t>(hit));
    pointer_down_=down;
    return hit>=0?Cursor::CAN_CLICK:Cursor::IDLE;
}
void UICashShop::refresh() {
    using namespace CashShopData;
    visible_.clear();
    if(category_==9) {
        for(size_t i=0;i<locker.size();++i) visible_.push_back(static_cast<int>(i));
    } else {
        const int gender=Stage::get().get_player().get_stats().get_female() ? 1 : 0;
        for(const auto& pair : offers) {
            const auto& o=pair.second;
            if(!o.sale || o.item<=0 || o.count<=0 || o.price<0 || (o.gender!=2 && o.gender!=gender)) continue;
            if(!ItemData::get(o.item).get_icon(false).is_valid()) continue;
            const int group=o.item/1000000;
            const int type=o.item/10000;
            bool match=category_==0
                || (category_==2 && (type==204 || type==506 || type==507))
                || (category_==3 && (group==2 || (group==5 && type!=500 && type!=515 && type!=516 && type!=518)))
                || (category_==4 && group==1)
                || (category_==5 && (type==515 || type==516))
                || (category_==6 && (type==500 || type==518))
                || (category_==7 && group==9);
            if(match) visible_.push_back(o.sn);
        }
    }
    page_=std::clamp(page_,0,std::max(0,(static_cast<int>(visible_.size())-1)/SHOP_PAGE_ITEMS));
    for(int i=0;i<SHOP_PAGE_ITEMS;++i) buttons_[BUY+i]->set_active(page_*SHOP_PAGE_ITEMS+i<static_cast<int>(visible_.size()));
    buttons_[CONFIRM]->set_active(selected_!=0 && category_!=9);
    buttons_[CANCEL]->set_active(selected_!=0);
    buttons_[PREV]->set_active(page_>0);
    buttons_[NEXT]->set_active((page_+1)*SHOP_PAGE_ITEMS<static_cast<int>(visible_.size()));
    revision_=revision;
}
void UICashShop::draw(float inter) const {
    using namespace CashShopData;
    preview_.draw({644,65},inter);
    UIElement::draw_sprites(inter);
    sidebar_[category_<9?category_+1:0].draw({0,63},inter);
    Stage::get().get_player().draw_preview({824,245},inter);
    label(Stage::get().get_player().get_name(),12,8,Color::WHITE);
    // Original background labels and inset balance fields stay intact.
    static const Charset digits(nl::nx::ui["CashShop.img"]["CSTab"]["CSGLChargeNX"]["Number"],Charset::RIGHT);
    auto number=[&](int64_t value,int y) {
        auto text=std::to_string(value);
        for(int i=int(text.size())-3;i>0;i-=3)text.insert(i,",");
        digits.draw(text,Point<int16_t>(114,y));
    };
    number(prepaid,418);number(credit,448);number(point,478);
    number(Stage::get().get_player().get_inventory().get_meso(),508);
    Text(Text::A11M,Text::RIGHT,Color::GRAY,"--").draw(Point<int16_t>(114,530));
    label("Page "+std::to_string(page_+1)+" / "+std::to_string(std::max(1,(static_cast<int>(visible_.size())+SHOP_PAGE_ITEMS-1)/SHOP_PAGE_ITEMS)),210,350,Color::WHITE);
    label("PAYMENT",12,564,Color::MINESHAFT);
    item_line_.draw({139,566},inter);
    for(int i=0;i<SHOP_PAGE_ITEMS && page_*SHOP_PAGE_ITEMS+i<static_cast<int>(visible_.size());++i) {
        const int key=visible_[page_*SHOP_PAGE_ITEMS+i];
        const auto o=category_==9 ? Offer{} : offers.at(key);
        const int item=category_==9 ? locker.at(key).item : o.item;
        const int count=category_==9 ? locker.at(key).count : o.count;
        const int x=137+(i%7)*124,y=372+(i/7)*205;
        item_base_.draw(Point<int16_t>(x,y),inter);
        if(selected_==key && category_!=9) ColorBox(118,190,Color::ORANGE,.15f).draw(Point<int16_t>(x,y));
        ItemData::get(item).get_icon(false).draw(DrawArgument(Point<int16_t>(x+27,y+101),2.f,2.f));
        auto name=itemname(item);
        Text title(Text::A11B,Text::CENTER,Color::MINESHAFT,name);
        while(title.width()>108 && name.size()>3) {name.pop_back();title.change_text(name+"...");}
        title.draw(Point<int16_t>(x+57,y+108));
        Text(Text::A11M,Text::CENTER,Color::GRAY,category_==9?"Take to inventory":std::to_string(o.price)+" NX ("+std::to_string(count)+")").draw(Point<int16_t>(x+57,y+135));

    }
    if(visible_.empty())label("No available items in this category.",160,420,Color::WHITE);
    ColorBox(374,35,Color::TUNA,1.f).draw({644,298});
    UIElement::draw_buttons(inter);
    // Keep purchase confirmation beside the preview, away from the product grid.
    ColorBox(374,46,Color::WHITE,.94f).draw({644,252});
    label(status,651,257,Color::MINESHAFT,358);
}
void UICashShop::update() {
    UIElement::update();
    preview_.update();
    if(revision_!=CashShopData::revision) refresh();
}
void UICashShop::request_exit() {
    if(CashShopData::exiting) return;
    CashShopData::exiting=true;
    CashShopData::status="Returning to channel...";
    ChangeMapPacket().dispatch();
    __android_log_print(ANDROID_LOG_INFO,"CashShop","Exit requested (empty CHANGE_MAP)");
}
Button::State UICashShop::button_pressed(uint16_t id) {
    using namespace CashShopData;
    if(id==EXIT) {request_exit();return Button::NORMAL;}
    if(exiting) return Button::NORMAL;
    if(id>=TAB && id<TAB+SHOP_TABS) {category_=id-TAB;page_=0;selected_=0;status=category_==1?"No event offers are published in the current catalogue.":category_==8?"No search results. Choose a category to browse.":category_==9?"Select a locker item to take it to inventory.":"Select an item to review before buying.";refresh();return Button::NORMAL;}
    if(id==PREV || id==NEXT) {page_+=id==NEXT ? 1 : -1;selected_=0;refresh();return Button::NORMAL;}
    if(id>=10 && id<=12) {currency_=id==10?1:id==11?2:4;status="Payment source selected.";return Button::NORMAL;}
    if(id==CANCEL) {selected_=0;refresh();return Button::NORMAL;}
    if(id>=BUY && id<BUY+SHOP_PAGE_ITEMS) {
        int index=page_*SHOP_PAGE_ITEMS+id-BUY;
        if(index>=static_cast<int>(visible_.size())) return Button::NORMAL;
        if(pending_sn || pending_take) {status="Waiting for the server response.";return Button::NORMAL;}
        if(category_==9) {
            auto& item=locker.at(visible_[index]);
            pending_take=item.cashid; CashTakePacket(item.cashid).dispatch();
            status="Taking item to game inventory...";
        } else {
            selected_=visible_[index];
            const auto& offer=offers.at(selected_);
            status="Confirm: "+itemname(offer.item)+" x"+std::to_string(offer.count)+" / "+std::to_string(offer.price)+" NX / "+(offer.period?std::to_string(offer.period)+" days":"Permanent");
        }
        refresh();return Button::NORMAL;
    }
    if(id==CONFIRM && selected_ && !pending_sn && !pending_take) {
        const auto& offer=offers.at(selected_);
        const int balance=currency_==1?credit:currency_==2?point:prepaid;
        if(balance<offer.price) status="Insufficient balance for the selected payment source.";
        else {
            pending_sn=offer.sn;
            CashPurchasePacket(offer.sn,currency_,offer.item/1000000==9).dispatch();
            __android_log_print(ANDROID_LOG_INFO,"CashShop","Purchase SN=%d item=%d price=%d currency=%d",offer.sn,offer.item,offer.price,currency_);
            status="Purchase request sent. Waiting for server confirmation.";
            selected_=0;refresh();
        }
    }
    return Button::NORMAL;
}
}
