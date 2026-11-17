// stats.cpp
#include "game/stats.hpp"

#include <algorithm>

namespace
{
// Health/mana granted by one point of vitality/energy, and per level.
constexpr int kHealthPerVitality = 2;
constexpr int kHealthPerLevel = 4;
constexpr int kManaPerEnergy = 2;
constexpr int kManaPerLevel = 1;

// Accuracy/defense baseline shared by every class.
constexpr int kAttackRatingBase = 15;
constexpr int kAttackRatingPerLevel = 2;
constexpr int kDefensePerDexterity = 2; // divide by this

// Experience needed to advance from level L to L+1 is kExperienceGrowth * L^2.
// Quadratic growth means deeper levels need noticeably longer, without the
// runaway of an exponential curve.
constexpr int kExperienceGrowth = 60;

struct ClassProfile
{
    int baseHealth;
    int baseMana;
    int attackRatingBonus;
    int defenseBonus;
    int damageMin;
    int damageMax;
    game::Attributes startingAttributes;
};

constexpr ClassProfile profileOf(game::CharacterClass characterClass)
{
    switch (characterClass)
    {
    // Tough, accurate enough, hits hardest in melee.
    case game::CharacterClass::Warrior:
        return {30, 5, 10, 5, 4, 9, {14, 10, 13, 8}};

    // Fast, accurate, balanced.
    case game::CharacterClass::Rogue:
        return {25, 10, 20, 3, 3, 7, {10, 14, 10, 10}};

    // Fragile in melee, big mana pool.
    case game::CharacterClass::Sorcerer:
        return {20, 25, 5, 1, 2, 5, {8, 10, 9, 15}};
    }

    return {20, 10, 0, 0, 1, 2, {}};
}
} // namespace

const char* game::name(CharacterClass value)
{
    switch (value)
    {
    case CharacterClass::Warrior:
        return "Warrior";
    case CharacterClass::Rogue:
        return "Rogue";
    case CharacterClass::Sorcerer:
        return "Sorcerer";
    }

    return "Unknown";
}

game::Attributes game::startingAttributes(CharacterClass value)
{
    return profileOf(value).startingAttributes;
}

game::DerivedStats game::derive(const Attributes& attributes, int level, CharacterClass characterClass)
{
    const ClassProfile profile = profileOf(characterClass);
    const int clampedLevel = std::clamp(level, 1, maxLevel);

    // Clamp both ends: a caller can pass unvalidated values (data files), and an
    // unbounded vitality would overflow the health calculation below.
    const int strength = std::clamp(attributes.strength, minAttribute, maxAttribute);
    const int dexterity = std::clamp(attributes.dexterity, minAttribute, maxAttribute);
    const int vitality = std::clamp(attributes.vitality, minAttribute, maxAttribute);
    const int energy = std::clamp(attributes.energy, minAttribute, maxAttribute);

    DerivedStats stats;
    stats.maxHealth = std::max(1, profile.baseHealth + vitality * kHealthPerVitality +
                                      (clampedLevel - 1) * kHealthPerLevel);
    stats.maxMana = std::max(0, profile.baseMana + energy * kManaPerEnergy + (clampedLevel - 1) * kManaPerLevel);
    stats.attackRating = std::max(1, kAttackRatingBase + dexterity + profile.attackRatingBonus +
                                         (clampedLevel - 1) * kAttackRatingPerLevel);
    stats.defense = std::max(0, dexterity / kDefensePerDexterity + profile.defenseBonus);

    const int strengthBonus = strength / 4;
    stats.damageMin = std::max(1, profile.damageMin + strengthBonus);
    stats.damageMax = std::max(stats.damageMin + 1, profile.damageMax + strengthBonus);
    return stats;
}

bool game::validate(const Attributes& attributes, std::string* reason)
{
    const auto fail = [reason](const char* message) {
        if (reason != nullptr)
            *reason = message;
        return false;
    };

    const auto inRange = [](int value) { return value >= minAttribute && value <= maxAttribute; };

    if (!inRange(attributes.strength))
        return fail("strength is out of range");
    if (!inRange(attributes.dexterity))
        return fail("dexterity is out of range");
    if (!inRange(attributes.vitality))
        return fail("vitality is out of range");
    if (!inRange(attributes.energy))
        return fail("energy is out of range");

    if (reason != nullptr)
        reason->clear();
    return true;
}

int game::experienceForLevel(int level)
{
    if (level <= 1)
        return 0;

    const int target = std::min(level, maxLevel);

    int total = 0;
    for (int current = 1; current < target; ++current)
        total += kExperienceGrowth * current * current;

    return total;
}

int game::levelForExperience(int experience)
{
    int level = 1;
    while (level < maxLevel && experience >= experienceForLevel(level + 1))
        ++level;

    return level;
}
