// stats.hpp
#pragma once

#include <string>

// Character attributes and the formulas derived from them.
//
// Everything here is pure arithmetic (no SFML, no globals), so balancing can be
// unit tested: the tests assert the *relationships* (more vitality means more
// health, a level up never lowers a stat, ...) rather than magic numbers, which
// keeps them valid when the numbers in stats.cpp are tuned.
namespace game
{
enum class CharacterClass
{
    Warrior,
    Rogue,
    Sorcerer
};

// Guards against nonsense values coming from data files or save games.
inline constexpr int minAttribute = 1;
inline constexpr int maxAttribute = 999;
inline constexpr int maxLevel = 99;

// Attribute points added to every attribute on level up.
inline constexpr int attributePointsPerLevel = 1;

struct Attributes
{
    int strength = 10;
    int dexterity = 10;
    int vitality = 10;
    int energy = 10;
};

// Everything the simulation needs to know about a character, recomputed from
// attributes + level + class so there is a single source of truth.
struct DerivedStats
{
    int maxHealth = 1;
    int maxMana = 0;
    int attackRating = 1;
    int defense = 0;
    int damageMin = 1;
    int damageMax = 2;
};

[[nodiscard]] const char* name(CharacterClass value);

// Attributes a new character of `value` starts with.
[[nodiscard]] Attributes startingAttributes(CharacterClass value);

[[nodiscard]] DerivedStats derive(const Attributes& attributes, int level, CharacterClass characterClass);

// True when every attribute is in range. Fills `reason` when it is not.
[[nodiscard]] bool validate(const Attributes& attributes, std::string* reason = nullptr);

// Total experience needed to *be* `level` (level 1 == 0).
[[nodiscard]] int experienceForLevel(int level);

// Highest level whose requirement is covered by `experience` (1..maxLevel).
[[nodiscard]] int levelForExperience(int experience);
} // namespace game
