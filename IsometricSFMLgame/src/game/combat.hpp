// combat.hpp
#pragma once

#include <string>

#include "core/rng.hpp"

// Damage and hit resolution.
//
// Everything here is a pure function of its arguments (plus an explicit Rng), so
// a fight can be replayed exactly and tested without a game running. The numbers
// are Diablo-like on purpose:
//   * hit chance is 2*AR/(AR+DR) clamped to 5%..95%  ("you always have a chance")
//   * physical damage is softened by armor with diminishing returns
//   * elemental damage is reduced by resistance, capped at 75%
namespace game
{
enum class DamageType
{
    Physical,
    Fire,
    Cold,
    Lightning
};

inline constexpr int minHitChancePercent = 5;
inline constexpr int maxHitChancePercent = 95;
inline constexpr int maxResistancePercent = 75;

// Range accepted for *stored* resistance values. Items may overcap (the damage
// calculation clamps to maxResistancePercent) and a creature may be vulnerable,
// hence the negative minimum. Values outside this range are data errors and are
// rejected by validate(). Actor::setEquipment() clamps gear to this range.
inline constexpr int minResistancePercent = -100;
inline constexpr int maxStoredResistancePercent = 500;

// Armor value that halves incoming physical damage.
inline constexpr int armorSoftening = 50;

struct DamageRange
{
    int min = 1;
    int max = 2;
};

struct Resistances
{
    int physical = 0;
    int fire = 0;
    int cold = 0;
    int lightning = 0;
};

struct AttackProfile
{
    int attackRating = 20;
    DamageRange damage{};
    DamageType type = DamageType::Physical;
    int criticalChancePercent = 5;
    int criticalMultiplierPercent = 200;
};

struct DefenseProfile
{
    int defense = 10;
    int armor = 0;
    Resistances resistances{};
};

struct AttackResult
{
    bool hit = false;
    bool critical = false;
    DamageType type = DamageType::Physical;
    int damage = 0;
};

[[nodiscard]] const char* name(DamageType value);

[[nodiscard]] bool validate(const DamageRange& damage, std::string* reason = nullptr);
[[nodiscard]] bool validate(const Resistances& resistances, std::string* reason = nullptr);
[[nodiscard]] bool validate(const AttackProfile& attack, std::string* reason = nullptr);
[[nodiscard]] bool validate(const DefenseProfile& defense, std::string* reason = nullptr);

// Chance (in percent) that `attackRating` connects against `defense`.
[[nodiscard]] int hitChancePercent(int attackRating, int defense);

// One roll inside `damage` (inclusive).
[[nodiscard]] int rollDamage(iso::Rng& rng, const DamageRange& damage);

// Physical mitigation. Never reduces a positive hit below 1.
[[nodiscard]] int applyArmor(int damage, int armor);

// Elemental mitigation for a resistance in percent (clamped to the cap).
[[nodiscard]] int applyResistance(int damage, int resistancePercent);

[[nodiscard]] int resistanceAgainst(const Resistances& resistances, DamageType type);

// Full attack: hit roll, damage roll, critical roll, mitigation.
//
// The Rng is advanced by exactly three rolls in a fixed order, hit or miss, so
// that two attacks with the same seed and profiles are always identical.
[[nodiscard]] AttackResult resolveAttack(iso::Rng& rng, const AttackProfile& attack,
                                         const DefenseProfile& defense);
} // namespace game
