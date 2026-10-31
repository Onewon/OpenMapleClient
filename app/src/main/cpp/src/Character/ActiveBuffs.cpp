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
#include "ActiveBuffs.h"

namespace ms {
template<EquipStat::Id STAT>
void SimpleStatBuff<STAT>::apply_to(CharStats &stats, int16_t value) const {
    stats.add_buff(STAT, value);
}

template<EquipStat::Id STAT>
void PercentageStatBuff<STAT>::apply_to(CharStats &stats, int16_t value) const {
    stats.add_percent(STAT, static_cast<float>(value) / 100);
}

void MapleWarriorBuff::apply_to(CharStats &stats, int16_t value) const {
    stats.add_value(EquipStat::STR,stats.get_stat(MapleStat::STR)*value/100);
    stats.add_value(EquipStat::DEX,stats.get_stat(MapleStat::DEX)*value/100);
    stats.add_value(EquipStat::INT,stats.get_stat(MapleStat::INT)*value/100);
    stats.add_value(EquipStat::LUK,stats.get_stat(MapleStat::LUK)*value/100);
}

void StanceBuff::apply_to(CharStats &stats, int16_t value) const {
    stats.set_stance(static_cast<float>(value) / 100);
}

void BoosterBuff::apply_to(CharStats &stats, int16_t value) const {
    stats.set_attackspeed(stats.get_attackspeed()+static_cast<int8_t>(value));
}

ActiveBuffs::ActiveBuffs() {
    buffs_[Buffstat::Id::MAPLE_WARRIOR] = std::make_unique<MapleWarriorBuff>();
    buffs_[Buffstat::Id::STANCE] = std::make_unique<StanceBuff>();
    buffs_[Buffstat::Id::ACC] = std::make_unique<SimpleStatBuff<EquipStat::ACC>>();
    buffs_[Buffstat::Id::AVOID] = std::make_unique<SimpleStatBuff<EquipStat::AVOID>>();
    buffs_[Buffstat::Id::BOOSTER] = std::make_unique<BoosterBuff>();
    buffs_[Buffstat::SPEED_INFUSION] = std::make_unique<BoosterBuff>();
    buffs_[Buffstat::DASH2] = std::make_unique<SimpleStatBuff<EquipStat::SPEED>>();
    buffs_[Buffstat::DASH] = std::make_unique<SimpleStatBuff<EquipStat::JUMP>>();
    buffs_[Buffstat::Id::WATK] =
        std::make_unique<SimpleStatBuff<EquipStat::Id::WATK>>();
    buffs_[Buffstat::Id::WDEF] =
        std::make_unique<SimpleStatBuff<EquipStat::Id::WDEF>>();
    buffs_[Buffstat::Id::MATK] =
        std::make_unique<SimpleStatBuff<EquipStat::Id::MAGIC>>();
    buffs_[Buffstat::Id::MDEF] =
        std::make_unique<SimpleStatBuff<EquipStat::Id::MDEF>>();
    buffs_[Buffstat::Id::SPEED] =
        std::make_unique<SimpleStatBuff<EquipStat::Id::SPEED>>();
    buffs_[Buffstat::Id::JUMP] =
        std::make_unique<SimpleStatBuff<EquipStat::Id::JUMP>>();
    buffs_[Buffstat::Id::HYPERBODYHP] =
        std::make_unique<PercentageStatBuff<EquipStat::Id::HP>>();
    buffs_[Buffstat::Id::HYPERBODYMP] =
        std::make_unique<PercentageStatBuff<EquipStat::Id::MP>>();
}

void ActiveBuffs::apply_buff(CharStats &stats,
                             Buffstat::Id stat,
                             int16_t value) const {
    if (const auto &buff = buffs_[stat]) {
        buff->apply_to(stats, value);
    }
}
}  // namespace ms