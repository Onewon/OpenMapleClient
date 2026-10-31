// Adventurer v83 dispatch catalog. AGPL-3.0-or-later.
#pragma once
#include "../Gameplay/Combat/AdventurerRules.h"
#include <cstdint>
namespace ms::adventurer {
enum class Kind { Unknown, Passive, Attack, Auxiliary, Status, Summon, Heal, Hold, Charge };
constexpr Kind kind(int32_t id) {
    switch(id) {
        case 1000000: return Kind::Passive; // Improved HP Recovery
        case 1000001: return Kind::Passive; // Improved MaxHP Increase
        case 1000002: return Kind::Passive; // Endure
        case 1001003: return Kind::Auxiliary; // Iron Body
        case 1001004: return Kind::Attack; // Power Strike
        case 1001005: return Kind::Attack; // Slash Blast
        case 1100000: return Kind::Passive; // Sword Mastery
        case 1100001: return Kind::Passive; // Axe Mastery
        case 1100002: return Kind::Passive; // Final Attack : Sword
        case 1100003: return Kind::Passive; // Final Attack : Axe
        case 1101004: return Kind::Auxiliary; // Sword Booster
        case 1101005: return Kind::Auxiliary; // Axe Booster
        case 1101006: return Kind::Auxiliary; // Rage
        case 1101007: return Kind::Auxiliary; // Power Guard
        case 1110000: return Kind::Passive; // Improving MP Recovery
        case 1110001: return Kind::Passive; // Shield Mastery
        case 1111002: return Kind::Auxiliary; // Combo Attack
        case 1111003: return Kind::Attack; // Panic : Sword
        case 1111004: return Kind::Attack; // Panic : Axe
        case 1111005: return Kind::Attack; // Coma: Sword
        case 1111006: return Kind::Attack; // Coma: Axe
        case 1111007: return Kind::Status; // Armor Crash
        case 1111008: return Kind::Attack; // Shout
        case 1120003: return Kind::Passive; // Advanced Combo Attack
        case 1120004: return Kind::Passive; // Achilles
        case 1120005: return Kind::Passive; // Guardian
        case 1121000: return Kind::Auxiliary; // Maple Warrior
        case 1121001: return Kind::Status; // Monster Magnet
        case 1121002: return Kind::Auxiliary; // Power Stance
        case 1121006: return Kind::Attack; // Rush
        case 1121008: return Kind::Attack; // Brandish
        case 1121010: return Kind::Auxiliary; // Enrage
        case 1121011: return Kind::Auxiliary; // Hero's Will
        case 1200000: return Kind::Passive; // Sword Mastery
        case 1200001: return Kind::Passive; // BW Mastery
        case 1200002: return Kind::Passive; // Final Attack : Sword
        case 1200003: return Kind::Passive; // Final Attack : BW
        case 1201004: return Kind::Auxiliary; // Sword Booster
        case 1201005: return Kind::Auxiliary; // BW Booster
        case 1201006: return Kind::Status; // Threaten
        case 1201007: return Kind::Auxiliary; // Power Guard
        case 1210000: return Kind::Passive; // Improving MP Recovery
        case 1210001: return Kind::Passive; // Shield Mastery
        case 1211002: return Kind::Attack; // Charged Blow
        case 1211003: return Kind::Auxiliary; // Fire Charge: Sword
        case 1211004: return Kind::Auxiliary; // Flame Charge: BW
        case 1211005: return Kind::Auxiliary; // Ice Charge: Sword
        case 1211006: return Kind::Auxiliary; // Blizzard Charge: BW
        case 1211007: return Kind::Auxiliary; // Thunder Charge: Sword
        case 1211008: return Kind::Auxiliary; // Lightning Charge: BW
        case 1211009: return Kind::Status; // Magic Crash
        case 1220005: return Kind::Passive; // Achilles
        case 1220006: return Kind::Passive; // Guardian
        case 1220010: return Kind::Passive; // Advanced Charge
        case 1221000: return Kind::Auxiliary; // Maple Warrior
        case 1221001: return Kind::Status; // Monster Magnet
        case 1221002: return Kind::Auxiliary; // Power Stance
        case 1221003: return Kind::Auxiliary; // Holy Charge : Sword
        case 1221004: return Kind::Auxiliary; // Divine Charge : BW
        case 1221007: return Kind::Attack; // Rush
        case 1221009: return Kind::Attack; // Blast
        case 1221011: return Kind::Attack; // Heaven's Hammer
        case 1221012: return Kind::Auxiliary; // Hero's Will
        case 1300000: return Kind::Passive; // Spear Mastery
        case 1300001: return Kind::Passive; // Pole Arm Mastery
        case 1300002: return Kind::Passive; // Final Attack : Spear
        case 1300003: return Kind::Passive; // Final Attack : Pole Arm
        case 1301004: return Kind::Auxiliary; // Spear Booster
        case 1301005: return Kind::Auxiliary; // Pole Arm Booster
        case 1301006: return Kind::Auxiliary; // Iron Will
        case 1301007: return Kind::Auxiliary; // Hyper Body
        case 1310000: return Kind::Passive; // Elemental Resistance
        case 1311001: return Kind::Attack; // Spear Crusher
        case 1311002: return Kind::Attack; // Pole Arm Crusher
        case 1311003: return Kind::Attack; // Dragon Fury: Spear
        case 1311004: return Kind::Attack; // Dragon Fury: Pole Arm
        case 1311005: return Kind::Attack; // Sacrifice
        case 1311006: return Kind::Attack; // Dragon Roar
        case 1311007: return Kind::Status; // Power Crash
        case 1311008: return Kind::Auxiliary; // Dragon Blood
        case 1320005: return Kind::Passive; // Achilles
        case 1320006: return Kind::Passive; // Berserk
        case 1320008: return Kind::Passive; // Aura of the Beholder
        case 1320009: return Kind::Passive; // Hex of the Beholder
        case 1321000: return Kind::Auxiliary; // Maple Warrior
        case 1321001: return Kind::Status; // Monster Magnet
        case 1321002: return Kind::Auxiliary; // Power Stance
        case 1321003: return Kind::Attack; // Rush
        case 1321007: return Kind::Summon; // Beholder
        case 1321010: return Kind::Auxiliary; // Hero's Will
        case 2000000: return Kind::Passive; // Improved MP Recovery
        case 2000001: return Kind::Passive; // Improved MaxMP Increase
        case 2001002: return Kind::Auxiliary; // Magic Guard
        case 2001003: return Kind::Auxiliary; // Magic Armor
        case 2001004: return Kind::Attack; // Energy Bolt
        case 2001005: return Kind::Attack; // Magic Claw
        case 2100000: return Kind::Passive; // MP Eater
        case 2101001: return Kind::Auxiliary; // Meditation
        case 2101002: return Kind::Auxiliary; // Teleport
        case 2101003: return Kind::Status; // Slow
        case 2101004: return Kind::Attack; // Fire Arrow
        case 2101005: return Kind::Attack; // Poison Breath
        case 2110000: return Kind::Passive; // Partial Resistance
        case 2110001: return Kind::Passive; // Element Amplification
        case 2111002: return Kind::Attack; // Explosion
        case 2111003: return Kind::Auxiliary; // Poison Mist
        case 2111004: return Kind::Status; // Seal
        case 2111005: return Kind::Auxiliary; // Spell Booster
        case 2111006: return Kind::Attack; // Element Composition
        case 2121000: return Kind::Auxiliary; // Maple Warrior
        case 2121001: return Kind::Charge; // Big Bang
        case 2121002: return Kind::Auxiliary; // Mana Reflection
        case 2121003: return Kind::Attack; // Fire Demon
        case 2121004: return Kind::Auxiliary; // Infinity
        case 2121005: return Kind::Summon; // Elquines
        case 2121006: return Kind::Attack; // Paralyze
        case 2121007: return Kind::Attack; // Meteor Shower
        case 2121008: return Kind::Auxiliary; // Hero's Will
        case 2200000: return Kind::Passive; // MP Eater
        case 2201001: return Kind::Auxiliary; // Meditation
        case 2201002: return Kind::Auxiliary; // Teleport
        case 2201003: return Kind::Status; // Slow
        case 2201004: return Kind::Attack; // Cold Beam
        case 2201005: return Kind::Attack; // Thunder Bolt
        case 2210000: return Kind::Passive; // Partial Resistance
        case 2210001: return Kind::Passive; // Element Amplification
        case 2211002: return Kind::Attack; // Ice Strike
        case 2211003: return Kind::Attack; // Thunder Spear
        case 2211004: return Kind::Status; // Seal
        case 2211005: return Kind::Auxiliary; // Spell Booster
        case 2211006: return Kind::Attack; // Element Composition
        case 2221000: return Kind::Auxiliary; // Maple Warrior
        case 2221001: return Kind::Charge; // Big Bang
        case 2221002: return Kind::Auxiliary; // Mana Reflection
        case 2221003: return Kind::Attack; // Ice Demon
        case 2221004: return Kind::Auxiliary; // Infinity
        case 2221005: return Kind::Summon; // Ifrit
        case 2221006: return Kind::Attack; // Chain Lightning
        case 2221007: return Kind::Attack; // Blizzard
        case 2221008: return Kind::Auxiliary; // Hero's Will
        case 2300000: return Kind::Passive; // MP Eater
        case 2301001: return Kind::Auxiliary; // Teleport
        case 2301002: return Kind::Heal; // Heal
        case 2301003: return Kind::Auxiliary; // Invincible
        case 2301004: return Kind::Auxiliary; // Bless
        case 2301005: return Kind::Attack; // Holy Arrow
        case 2310000: return Kind::Passive; // Elemental Resistance
        case 2311001: return Kind::Status; // Dispel
        case 2311002: return Kind::Auxiliary; // Mystic Door
        case 2311003: return Kind::Auxiliary; // Holy Symbol
        case 2311004: return Kind::Attack; // Shining Ray
        case 2311005: return Kind::Status; // Doom
        case 2311006: return Kind::Summon; // Summon Dragon
        case 2321000: return Kind::Auxiliary; // Maple Warrior
        case 2321001: return Kind::Charge; // Big Bang
        case 2321002: return Kind::Auxiliary; // Mana Reflection
        case 2321003: return Kind::Summon; // Bahamut
        case 2321004: return Kind::Auxiliary; // Infinity
        case 2321005: return Kind::Auxiliary; // Holy Shield
        case 2321006: return Kind::Auxiliary; // Resurrection
        case 2321007: return Kind::Attack; // Angel Ray
        case 2321008: return Kind::Attack; // Genesis
        case 2321009: return Kind::Auxiliary; // Hero's Will
        case 3000000: return Kind::Passive; // The Blessing of Amazon
        case 3000001: return Kind::Passive; // Critical Shot
        case 3000002: return Kind::Passive; // The Eye of Amazon
        case 3001003: return Kind::Auxiliary; // Focus
        case 3001004: return Kind::Attack; // Arrow Blow
        case 3001005: return Kind::Attack; // Double Shot
        case 3100000: return Kind::Passive; // Bow Mastery
        case 3100001: return Kind::Passive; // Final Attack : Bow
        case 3101002: return Kind::Auxiliary; // Bow Booster
        case 3101003: return Kind::Attack; // Power Knock-Back
        case 3101004: return Kind::Auxiliary; // Soul Arrow : Bow
        case 3101005: return Kind::Attack; // Arrow Bomb : Bow
        case 3110000: return Kind::Passive; // Thrust
        case 3110001: return Kind::Passive; // Mortal Blow
        case 3111002: return Kind::Summon; // Puppet
        case 3111003: return Kind::Attack; // Inferno
        case 3111004: return Kind::Attack; // Arrow Rain
        case 3111005: return Kind::Summon; // Silver Hawk
        case 3111006: return Kind::Attack; // Strafe
        case 3120005: return Kind::Passive; // Bow Expert
        case 3121000: return Kind::Auxiliary; // Maple Warrior
        case 3121002: return Kind::Auxiliary; // Sharp Eyes
        case 3121003: return Kind::Attack; // Dragon's Breath
        case 3121004: return Kind::Hold; // Hurricane
        case 3121006: return Kind::Summon; // Phoenix
        case 3121007: return Kind::Auxiliary; // Hamstring
        case 3121008: return Kind::Auxiliary; // Concentrate
        case 3121009: return Kind::Auxiliary; // Hero's Will
        case 3200000: return Kind::Passive; // Crossbow Mastery
        case 3200001: return Kind::Passive; // Final Attack : Crossbow
        case 3201002: return Kind::Auxiliary; // Crossbow Booster
        case 3201003: return Kind::Attack; // Power Knock-Back
        case 3201004: return Kind::Auxiliary; // Soul Arrow : Crossbow
        case 3201005: return Kind::Attack; // Iron Arrow : Crossbow
        case 3210000: return Kind::Passive; // Thrust
        case 3210001: return Kind::Passive; // Mortal Blow
        case 3211002: return Kind::Summon; // Puppet
        case 3211003: return Kind::Attack; // Blizzard
        case 3211004: return Kind::Attack; // Arrow Eruption
        case 3211005: return Kind::Summon; // Golden Eagle
        case 3211006: return Kind::Attack; // Strafe
        case 3220004: return Kind::Passive; // Marksman Boost
        case 3221000: return Kind::Auxiliary; // Maple Warrior
        case 3221001: return Kind::Charge; // Piercing Arrow
        case 3221002: return Kind::Auxiliary; // Sharp Eyes
        case 3221003: return Kind::Attack; // Dragon's Breath
        case 3221005: return Kind::Summon; // Frostprey
        case 3221006: return Kind::Auxiliary; // Blind
        case 3221007: return Kind::Attack; // Snipe
        case 3221008: return Kind::Auxiliary; // Hero's Will
        case 4000000: return Kind::Passive; // Nimble Body
        case 4000001: return Kind::Passive; // Keen Eyes
        case 4001002: return Kind::Attack; // Disorder
        case 4001003: return Kind::Auxiliary; // Dark Sight
        case 4001334: return Kind::Attack; // Double Stab
        case 4001344: return Kind::Attack; // Lucky Seven
        case 4100000: return Kind::Passive; // Claw Mastery
        case 4100001: return Kind::Passive; // Critical Throw
        case 4100002: return Kind::Passive; // Endure
        case 4101003: return Kind::Auxiliary; // Claw Booster
        case 4101004: return Kind::Auxiliary; // Haste
        case 4101005: return Kind::Attack; // Drain
        case 4110000: return Kind::Passive; // Alchemist
        case 4111001: return Kind::Auxiliary; // Meso Up
        case 4111002: return Kind::Auxiliary; // Shadow Partner
        case 4111003: return Kind::Status; // Shadow Web
        case 4111004: return Kind::Attack; // Shadow Meso
        case 4111005: return Kind::Attack; // Avenger
        case 4111006: return Kind::Auxiliary; // Flash Jump
        case 4120002: return Kind::Passive; // Shadow Shifter
        case 4120005: return Kind::Passive; // Venomous Star
        case 4121000: return Kind::Auxiliary; // Maple Warrior
        case 4121003: return Kind::Attack; // Taunt
        case 4121004: return Kind::Auxiliary; // Ninja Ambush
        case 4121006: return Kind::Auxiliary; // Shadow Stars
        case 4121007: return Kind::Attack; // Triple Throw
        case 4121008: return Kind::Attack; // Ninja Storm
        case 4121009: return Kind::Auxiliary; // Hero's Will
        case 4200000: return Kind::Passive; // Dagger Mastery
        case 4200001: return Kind::Passive; // Endure
        case 4201002: return Kind::Auxiliary; // Dagger Booster
        case 4201003: return Kind::Auxiliary; // Haste
        case 4201004: return Kind::Attack; // Steal
        case 4201005: return Kind::Attack; // Savage Blow
        case 4210000: return Kind::Passive; // Shield Mastery
        case 4211001: return Kind::Auxiliary; // Chakra
        case 4211002: return Kind::Attack; // Assaulter
        case 4211003: return Kind::Auxiliary; // Pickpocket
        case 4211004: return Kind::Attack; // Band of Thieves
        case 4211005: return Kind::Auxiliary; // Meso Guard
        case 4211006: return Kind::Attack; // Meso Explosion
        case 4220002: return Kind::Passive; // Shadow Shifter
        case 4220005: return Kind::Passive; // Venomous Stab
        case 4221000: return Kind::Auxiliary; // Maple Warrior
        case 4221001: return Kind::Attack; // Assassinate
        case 4221003: return Kind::Attack; // Taunt
        case 4221004: return Kind::Auxiliary; // Ninja Ambush
        case 4221006: return Kind::Auxiliary; // Smokescreen
        case 4221007: return Kind::Attack; // Boomerang Step
        case 4221008: return Kind::Auxiliary; // Hero's Will
        case 5000000: return Kind::Passive; // Bullet Time
        case 5001001: return Kind::Attack; // Flash Fist
        case 5001002: return Kind::Attack; // Sommersault Kick
        case 5001003: return Kind::Attack; // Double Shot
        case 5001005: return Kind::Auxiliary; // Dash
        case 5100000: return Kind::Passive; // Improve MaxHP
        case 5100001: return Kind::Passive; // Knuckler Mastery
        case 5101002: return Kind::Attack; // Backspin Blow
        case 5101003: return Kind::Attack; // Double Uppercut
        case 5101004: return Kind::Charge; // Corkscrew Blow
        case 5101005: return Kind::Auxiliary; // MP Recovery
        case 5101006: return Kind::Auxiliary; // Knuckler Booster
        case 5101007: return Kind::Auxiliary; // Oak Barrel
        case 5110000: return Kind::Passive; // Stun Mastery
        case 5110001: return Kind::Passive; // Energy Charge
        case 5111002: return Kind::Attack; // Energy Blast
        case 5111004: return Kind::Attack; // Energy Drain
        case 5111005: return Kind::Auxiliary; // Transformation
        case 5111006: return Kind::Attack; // Shockwave
        case 5121000: return Kind::Auxiliary; // Maple Warrior
        case 5121001: return Kind::Attack; // Dragon Strike
        case 5121002: return Kind::Attack; // Energy Orb
        case 5121003: return Kind::Auxiliary; // Super Transformation
        case 5121004: return Kind::Attack; // Demolition
        case 5121005: return Kind::Attack; // Snatch
        case 5121007: return Kind::Attack; // Barrage
        case 5121008: return Kind::Auxiliary; // Pirate's Rage
        case 5121009: return Kind::Auxiliary; // Speed Infusion
        case 5121010: return Kind::Auxiliary; // Time Leap
        case 5200000: return Kind::Passive; // Gun Mastery
        case 5201001: return Kind::Attack; // Invisible Shot
        case 5201002: return Kind::Charge; // Grenade
        case 5201003: return Kind::Auxiliary; // Gun Booster
        case 5201004: return Kind::Attack; // Blank Shot
        case 5201005: return Kind::Auxiliary; // Wings
        case 5201006: return Kind::Attack; // Recoil Shot
        case 5210000: return Kind::Passive; // Burst Fire
        case 5211001: return Kind::Summon; // Octopus
        case 5211002: return Kind::Summon; // Gaviota
        case 5211004: return Kind::Attack; // Flamethrower
        case 5211005: return Kind::Attack; // Ice Splitter
        case 5211006: return Kind::Attack; // Homing Beacon
        case 5220001: return Kind::Passive; // Elemental Boost
        case 5220002: return Kind::Passive; // Wrath of the Octopi
        case 5220011: return Kind::Passive; // Bullseye
        case 5221000: return Kind::Auxiliary; // Maple Warrior
        case 5221003: return Kind::Attack; // Aerial Strike
        case 5221004: return Kind::Hold; // Rapid Fire
        case 5221006: return Kind::Auxiliary; // Battleship
        case 5221007: return Kind::Attack; // Battleship Cannon
        case 5221008: return Kind::Attack; // Battleship Torpedo
        case 5221009: return Kind::Attack; // Hypnotize
        case 5221010: return Kind::Auxiliary; // Speed Infusion
        default: return Kind::Unknown;
    }
}
constexpr bool attack(int32_t id) { auto k=kind(id); return k==Kind::Attack || k==Kind::Heal || k==Kind::Hold || k==Kind::Charge; }
constexpr bool hold(int32_t id) { return id==3121004 || id==5221004; }
constexpr bool charge(int32_t id) {return id==2121001 || id==2221001 || id==2321001 || id==3221001 || id==5101004 || id==5201002;}
constexpr bool teleport(int32_t id) {return id==2101002 || id==2201002 || id==2301001;}
constexpr bool magnet(int32_t id) {return id==1121001 || id==1221001 || id==1321001;}
constexpr int attack_mode(int32_t id) {
    const int job=id/10000;
    if(id==3101003 || id==3201003) return 0;
    if(job/100==2)return 2;
    return job/10==31 || job/10==32 || job/10==41 || job/10==52 || id==4001344 || id==5001003 ? 1:0;
}
constexpr bool uses_ammo(int32_t id) {
    return id==4121006 || ((attack(id) || id==5210000 || id==5220011 || mortal_blow(id)) && attack_mode(id)==1 && id!=4111004);
}
constexpr bool final_attack(int32_t id) {return id==1100002 || id==1100003 || id==1200002 || id==1200003 || id==1300002 || id==1300003 || id==3100001 || id==3200001;}
}
