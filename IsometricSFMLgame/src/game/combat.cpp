// combat.cpp
#include "game/combat.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>

namespace
{
// Damage is computed in 64 bit and clamped back, so extreme values from data
// files or save games cannot overflow an int.
constexpr std::int64_t kMaxDamage = std::numeric_limits<int>::max();
} // namespace

const char* game::name(DamageType value)
{
    switch (value)
    {
    case DamageType::Physical:
        return "Physical";
    case DamageType::Fire:
        return "Fire";
    case DamageType::Cold:
        return "Cold";
    case DamageType::Lightning:
        return "Lightning";
    }

    return "Unknown";
}

bool game::validate(const DamageRange& damage, std::string* reason)
{
    if (damage.min < 1)
    {
        if (reason != nullptr)
            *reason = "minimum damage must be at least 1";
        return false;
    }

    if (damage.max < damage.min)
    {
        if (reason != nullptr)
            *reason = "maximum damage is below the minimum";
        return false;
    }

    if (reason != nullptr)
        reason->clear();
    return true;
}

bool game::validate(const Resistances& resistances, std::string* reason)
{
    const auto inRange = [](int value) {
        return value >= minResistancePercent && value <= maxStoredResistancePercent;
    };

    if (!inRange(resistances.physical) || !inRange(resistances.fire) || !inRange(resistances.cold) ||
        !inRange(resistances.lightning))
    {
        if (reason != nullptr)
            *reason = "resistance out of range";
        return false;
    }

    if (reason != nullptr)
        reason->clear();
    return true;
}

bool game::validate(const AttackProfile& attack, std::string* reason)
{
    if (!validate(attack.damage, reason))
        return false;

    if (attack.attackRating < 1)
    {
        if (reason != nullptr)
            *reason = "attack rating must be at least 1";
        return false;
    }

    if (attack.criticalChancePercent < 0 || attack.criticalChancePercent > 100)
    {
        if (reason != nullptr)
            *reason = "critical chance must be a percentage";
        return false;
    }

    if (attack.criticalMultiplierPercent < 100)
    {
        if (reason != nullptr)
            *reason = "a critical hit cannot deal less damage than a normal one";
        return false;
    }

    if (reason != nullptr)
        reason->clear();
    return true;
}

bool game::validate(const DefenseProfile& defense, std::string* reason)
{
    if (!validate(defense.resistances, reason))
        return false;

    if (defense.defense < 0 || defense.armor < 0)
    {
        if (reason != nullptr)
            *reason = "defense and armor cannot be negative";
        return false;
    }

    if (reason != nullptr)
        reason->clear();
    return true;
}

int game::hitChancePercent(int attackRating, int defense)
{
    // Percentage form of 2*AR/(AR+DR): 100 * 2*AR/(AR+DR). 64 bit because
    // 200 * attackRating overflows for ratings a data file could contain.
    const std::int64_t attack = std::max(1, attackRating);
    const std::int64_t guard = std::max(0, defense);

    const std::int64_t chance = (200 * attack) / (attack + guard);
    return static_cast<int>(
        std::clamp<std::int64_t>(chance, minHitChancePercent, maxHitChancePercent));
}

int game::rollDamage(iso::Rng& rng, const DamageRange& damage)
{
    const int low = std::max(1, damage.min);
    const int high = std::max(low, damage.max);
    return rng.range(low, high);
}

int game::applyArmor(int damage, int armor)
{
    if (damage <= 0)
        return 0;

    const std::int64_t softening = std::max(0, armor);
    const std::int64_t mitigated = (static_cast<std::int64_t>(damage) * armorSoftening) /
                                   (static_cast<std::int64_t>(armorSoftening) + softening);

    // Armor reduces damage; it never makes a hit harmless.
    return static_cast<int>(std::max<std::int64_t>(1, std::min<std::int64_t>(mitigated, damage)));
}

int game::applyResistance(int damage, int resistancePercent)
{
    if (damage <= 0)
        return 0;

    const int capped = std::clamp(resistancePercent, 0, maxResistancePercent);
    const std::int64_t mitigated = (static_cast<std::int64_t>(damage) * (100 - capped)) / 100;

    return static_cast<int>(std::max<std::int64_t>(1, std::min<std::int64_t>(mitigated, damage)));
}

int game::resistanceAgainst(const Resistances& resistances, DamageType type)
{
    switch (type)
    {
    case DamageType::Physical:
        return resistances.physical;
    case DamageType::Fire:
        return resistances.fire;
    case DamageType::Cold:
        return resistances.cold;
    case DamageType::Lightning:
        return resistances.lightning;
    }

    return 0;
}

game::AttackResult game::resolveAttack(iso::Rng& rng, const AttackProfile& attack, const DefenseProfile& defense)
{
    AttackResult result;
    result.type = attack.type;

    // Always three rolls in this order, hit or miss, so the Rng state only
    // depends on how many attacks were made (deterministic replays).
    const int hitRoll = rng.range(1, 100);
    const int damageRoll = rollDamage(rng, attack.damage);
    const int criticalRoll = rng.range(1, 100);

    if (hitRoll > hitChancePercent(attack.attackRating, defense.defense))
        return result;

    result.hit = true;
    result.critical = criticalRoll <= attack.criticalChancePercent;

    int damage = damageRoll;
    if (result.critical)
    {
        // 64 bit: damage * multiplier overflows for extreme multipliers. The
        // result is clamped instead of wrapping, so a critical never hurts less.
        const std::int64_t boosted =
            (static_cast<std::int64_t>(damage) * attack.criticalMultiplierPercent) / 100;
        damage = static_cast<int>(std::min(kMaxDamage, boosted));
    }

    if (attack.type == DamageType::Physical)
    {
        damage = applyArmor(damage, defense.armor);
        damage = applyResistance(damage, defense.resistances.physical);
    }
    else
    {
        damage = applyResistance(damage, resistanceAgainst(defense.resistances, attack.type));
    }

    result.damage = std::max(1, damage);
    return result;
}
