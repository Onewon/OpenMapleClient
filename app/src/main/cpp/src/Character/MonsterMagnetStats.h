// Paired server WZ overrides for the three Monster Magnet skills. AGPL-3.0-or-later.
#pragma once
namespace ms::adventurer {
struct MagnetStats {int range,count,chance;};
constexpr MagnetStats magnet_stats(int level) {switch(level) {
case 1: return {200,3,100};
case 2: return {210,3,100};
case 3: return {220,3,100};
case 4: return {230,3,100};
case 5: return {240,3,100};
case 6: return {250,3,100};
case 7: return {260,3,100};
case 8: return {270,4,100};
case 9: return {280,4,100};
case 10: return {290,4,100};
case 11: return {300,4,100};
case 12: return {310,4,100};
case 13: return {320,4,100};
case 14: return {330,4,100};
case 15: return {340,5,100};
case 16: return {350,5,100};
case 17: return {360,5,100};
case 18: return {370,5,100};
case 19: return {380,5,100};
case 20: return {390,5,100};
case 21: return {400,5,100};
case 22: return {410,6,100};
case 23: return {420,6,100};
case 24: return {430,6,100};
case 25: return {440,6,100};
case 26: return {450,6,100};
case 27: return {450,6,100};
case 28: return {450,6,100};
case 29: return {450,7,100};
case 30: return {450,7,100};
default:return {0,0,0};}}
}
