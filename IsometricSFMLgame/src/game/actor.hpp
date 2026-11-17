// actor.hpp
#pragma once

#include <string>

#include "game/combat.hpp"
#include "game/stats.hpp"
#include "map/grid_geometry.hpp"

// A living thing in the dungeon: the player or a monster.
//
// Actor is deliberately SFML-free and owns its own state (health, experience,
// cooldowns, position), so combat and levelling can be tested headlessly and the
// game state only has to render what it finds here.
namespace game
{
enum class Faction
{
    Player,
    Monster
};

[[nodiscard]] const char* name(Faction value);

// What equipment adds on top of the attributes. Kept separate so gear can be
// swapped later without touching the attribute formulas.
struct Equipment
{
    int damageMin = 0;
    int damageMax = 0;
    int armor = 0;
    Resistances resistances{};
};

// Seconds between two attacks of the same actor for a faction.
[[nodiscard]] float defaultAttackDelay(Faction faction);

class Actor
{
public:
    Actor(int id, std::string name, Faction faction, CharacterClass characterClass, iso::GridPoint cell,
          int level = 1);

    // --- identity ---------------------------------------------------------
    [[nodiscard]] int id() const;
    [[nodiscard]] const std::string& name() const;
    [[nodiscard]] Faction faction() const;
    [[nodiscard]] CharacterClass characterClass() const;
    [[nodiscard]] bool isPlayer() const;

    // --- health and mana --------------------------------------------------
    [[nodiscard]] bool isAlive() const;
    [[nodiscard]] int health() const;
    [[nodiscard]] int maxHealth() const;
    [[nodiscard]] int mana() const;
    [[nodiscard]] int maxMana() const;
    [[nodiscard]] float healthFraction() const;

    // Returns the damage that was actually applied (0 when dead or `amount` <= 0).
    int takeDamage(int amount);

    // Returns the health that was actually restored.
    int heal(int amount);

    // Restores everything (level start, resurrection).
    void restore();

    // False when there is not enough mana; nothing is spent in that case.
    [[nodiscard]] bool spendMana(int amount);

    // Returns the mana that was actually restored.
    int restoreMana(int amount);

    // --- progression ------------------------------------------------------
    [[nodiscard]] int level() const;
    [[nodiscard]] int experience() const;

    // Experience still needed for the next level (0 at maxLevel).
    [[nodiscard]] int experienceToNextLevel() const;

    // Adds experience and levels up as often as it covers. Returns levels gained.
    int addExperience(int amount);

    [[nodiscard]] const Attributes& attributes() const;

    // Direct attribute changes (level up, debug tools, save loading).
    void addAttributes(const Attributes& delta);

    // --- derived combat values -------------------------------------------
    [[nodiscard]] const Equipment& equipment() const;
    void setEquipment(const Equipment& equipment);

    [[nodiscard]] DerivedStats derived() const;
    [[nodiscard]] AttackProfile attackProfile() const;
    [[nodiscard]] DefenseProfile defenseProfile() const;

    // --- position ---------------------------------------------------------
    // Logical cell the actor stands on or is walking toward.
    [[nodiscard]] iso::GridPoint cell() const;

    // Smoothly interpolated position in cell units, for rendering.
    [[nodiscard]] iso::PointF position() const;

    // Direction of the last real move, used to pick an animation (never {0, 0}).
    [[nodiscard]] iso::GridPoint facing() const;

    // Commits to a new cell; position() glides toward it.
    void moveTo(iso::GridPoint cell);

    // Instantly places the actor (spawn, teleport, level change).
    void placeAt(iso::GridPoint cell);

    // Advances the glide toward the current cell. Meant to be called once per
    // frame after the walker decided where the actor is heading.
    void advanceMovement(float cellsPerSecond, float deltaSeconds);

    [[nodiscard]] bool isMoving() const;

    // --- attack pacing ----------------------------------------------------
    [[nodiscard]] bool canAttack() const;
    void startAttackCooldown(float seconds);
    void tickCooldowns(float deltaSeconds);
    [[nodiscard]] float attackCooldownRemaining() const;

    // --- integrity --------------------------------------------------------
    // True when every stored value is self-consistent. `reason` explains a
    // failure, which the F2 self-test prints instead of guessing.
    [[nodiscard]] bool validate(std::string* reason = nullptr) const;

private:
    void clampVitals();
    void onLevelUp();

    int id_;
    std::string name_;
    Faction faction_;
    CharacterClass characterClass_;

    int level_ = 1;
    int experience_ = 0;
    Attributes attributes_{};
    Equipment equipment_{};

    int health_ = 1;
    int mana_ = 0;
    float attackCooldown_ = 0.f;

    iso::GridPoint cell_{0, 0};
    iso::PointF position_{0.f, 0.f};
    iso::GridPoint facing_{0, 1};
};
} // namespace game
