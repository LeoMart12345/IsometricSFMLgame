// actor.cpp
#include "game/actor.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <utility>

#include "core/constants.hpp"

namespace
{
// How close the interpolated position must be to count as "arrived".
constexpr float kArrivalEpsilon = 0.001f;

// Damage and resistance values are stored in ints; gear bonuses are clamped here.
constexpr int kMaxIntValue = std::numeric_limits<int>::max();
} // namespace

const char* game::name(Faction value)
{
    switch (value)
    {
    case Faction::Player:
        return "Player";
    case Faction::Monster:
        return "Monster";
    }

    return "Unknown";
}

float game::defaultAttackDelay(Faction faction)
{
    return faction == Faction::Player ? config::playerAttackDelay : config::monsterAttackDelay;
}

game::Actor::Actor(int id, std::string name, Faction faction, CharacterClass characterClass, iso::GridPoint cell,
                   int level)
    : id_(id), name_(std::move(name)), faction_(faction), characterClass_(characterClass),
      level_(std::clamp(level, 1, maxLevel)), attributes_(startingAttributes(characterClass)), cell_(cell),
      position_{static_cast<float>(cell.x), static_cast<float>(cell.y)}
{
    // A higher-level spawn starts with the attributes it would have earned, so
    // deep dungeon levels have monsters that match the player's growth. The
    // addition is done in 64 bit and clamped to the same range validate()
    // enforces; a future experience curve must not be able to create an actor
    // whose attributes are already invalid.
    const int levelsGained = level_ - 1;
    if (levelsGained > 0)
    {
        const auto grownAttribute = [levelsGained](int base) {
            const std::int64_t grown = static_cast<std::int64_t>(base) +
                                       static_cast<std::int64_t>(attributePointsPerLevel) * levelsGained;
            return static_cast<int>(std::clamp<std::int64_t>(grown, minAttribute, maxAttribute));
        };

        attributes_.strength = grownAttribute(attributes_.strength);
        attributes_.dexterity = grownAttribute(attributes_.dexterity);
        attributes_.vitality = grownAttribute(attributes_.vitality);
        attributes_.energy = grownAttribute(attributes_.energy);
    }

    experience_ = experienceForLevel(level_);
    restore();
}

// ---------------------------------------------------------------------------
// Identity
// ---------------------------------------------------------------------------
int game::Actor::id() const
{
    return id_;
}

const std::string& game::Actor::name() const
{
    return name_;
}

game::Faction game::Actor::faction() const
{
    return faction_;
}

game::CharacterClass game::Actor::characterClass() const
{
    return characterClass_;
}

bool game::Actor::isPlayer() const
{
    return faction_ == Faction::Player;
}

// ---------------------------------------------------------------------------
// Health and mana
// ---------------------------------------------------------------------------
bool game::Actor::isAlive() const
{
    return health_ > 0;
}

int game::Actor::health() const
{
    return health_;
}

int game::Actor::maxHealth() const
{
    return derived().maxHealth;
}

int game::Actor::mana() const
{
    return mana_;
}

int game::Actor::maxMana() const
{
    return derived().maxMana;
}

float game::Actor::healthFraction() const
{
    const int maximum = maxHealth();
    if (maximum <= 0)
        return 0.f;

    return std::clamp(static_cast<float>(health_) / static_cast<float>(maximum), 0.f, 1.f);
}

int game::Actor::takeDamage(int amount)
{
    if (amount <= 0 || !isAlive())
        return 0;

    const int applied = std::min(amount, health_);
    health_ -= applied;
    return applied;
}

int game::Actor::heal(int amount)
{
    if (amount <= 0 || !isAlive())
        return 0;

    const int restored = std::min(amount, maxHealth() - health_);
    health_ += std::max(0, restored);
    return std::max(0, restored);
}

void game::Actor::restore()
{
    health_ = maxHealth();
    mana_ = maxMana();
}

bool game::Actor::spendMana(int amount)
{
    if (amount <= 0)
        return true;

    if (mana_ < amount)
        return false;

    mana_ -= amount;
    return true;
}

int game::Actor::restoreMana(int amount)
{
    if (amount <= 0)
        return 0;

    const int restored = std::min(amount, maxMana() - mana_);
    mana_ += std::max(0, restored);
    return std::max(0, restored);
}

// ---------------------------------------------------------------------------
// Progression
// ---------------------------------------------------------------------------
int game::Actor::level() const
{
    return level_;
}

int game::Actor::experience() const
{
    return experience_;
}

int game::Actor::experienceToNextLevel() const
{
    if (level_ >= maxLevel)
        return 0;

    return experienceForLevel(level_ + 1) - experience_;
}

int game::Actor::addExperience(int amount)
{
    if (amount <= 0)
        return 0;

    // Saturating add: experience is an int and a long session must not wrap.
    constexpr int kIntMax = std::numeric_limits<int>::max();
    experience_ = (experience_ > kIntMax - amount) ? kIntMax : experience_ + amount;

    const int reachedLevel = levelForExperience(experience_);
    int gained = 0;
    while (level_ < reachedLevel)
    {
        ++level_;
        ++gained;
        onLevelUp();
    }

    return gained;
}

void game::Actor::onLevelUp()
{
    const bool wasAlive = isAlive();
    const int healthBefore = maxHealth();
    const int manaBefore = maxMana();

    attributes_.strength = std::min(maxAttribute, attributes_.strength + attributePointsPerLevel);
    attributes_.dexterity = std::min(maxAttribute, attributes_.dexterity + attributePointsPerLevel);
    attributes_.vitality = std::min(maxAttribute, attributes_.vitality + attributePointsPerLevel);
    attributes_.energy = std::min(maxAttribute, attributes_.energy + attributePointsPerLevel);

    // Level ups grant the extra capacity immediately, otherwise a fresh level
    // would run around "level 5 with the health bar of a level 2". A corpse must
    // stay a corpse: adding the capacity to zero health would silently resurrect
    // a dead actor, and `isAlive()` intentionally means health > 0.
    if (wasAlive)
    {
        health_ += maxHealth() - healthBefore;
        mana_ += maxMana() - manaBefore;
    }

    clampVitals();
}

const game::Attributes& game::Actor::attributes() const
{
    return attributes_;
}

void game::Actor::addAttributes(const Attributes& delta)
{
    // 64 bit: a large negative/positive delta overflows int otherwise, and the
    // wrapped result could even land back inside the valid range.
    const auto add = [](int value, int change) {
        const std::int64_t sum = static_cast<std::int64_t>(value) + static_cast<std::int64_t>(change);
        return static_cast<int>(std::clamp<std::int64_t>(sum, minAttribute, maxAttribute));
    };

    attributes_.strength = add(attributes_.strength, delta.strength);
    attributes_.dexterity = add(attributes_.dexterity, delta.dexterity);
    attributes_.vitality = add(attributes_.vitality, delta.vitality);
    attributes_.energy = add(attributes_.energy, delta.energy);

    // Losing vitality or energy lowers the maximum: the current health/mana may
    // now be above it, which validate() would reject.
    clampVitals();
}

// ---------------------------------------------------------------------------
// Derived combat values
// ---------------------------------------------------------------------------
const game::Equipment& game::Actor::equipment() const
{
    return equipment_;
}

void game::Actor::setEquipment(const Equipment& equipment)
{
    // Gear usually comes from a data file, so keep it inside the range that
    // validate() accepts instead of letting a typo invalidate the actor.
    equipment_ = equipment;
    equipment_.damageMin = std::max(0, equipment.damageMin);
    equipment_.damageMax = std::max(0, equipment.damageMax);
    equipment_.armor = std::max(0, equipment.armor);

    const auto clampResistance = [](int value) {
        return std::clamp(value, minResistancePercent, maxStoredResistancePercent);
    };
    equipment_.resistances.physical = clampResistance(equipment.resistances.physical);
    equipment_.resistances.fire = clampResistance(equipment.resistances.fire);
    equipment_.resistances.cold = clampResistance(equipment.resistances.cold);
    equipment_.resistances.lightning = clampResistance(equipment.resistances.lightning);

    clampVitals();
}

game::DerivedStats game::Actor::derived() const
{
    return derive(attributes_, level_, characterClass_);
}

game::AttackProfile game::Actor::attackProfile() const
{
    const DerivedStats stats = derived();

    // Gear bonuses are added in 64 bit and clamped: a data file can contain a
    // damage bonus big enough to overflow the sum otherwise.
    const auto addBonus = [](int base, int bonus) {
        const std::int64_t total = static_cast<std::int64_t>(base) + static_cast<std::int64_t>(std::max(0, bonus));
        return static_cast<int>(std::min<std::int64_t>(total, kMaxIntValue));
    };

    AttackProfile profile;
    profile.attackRating = stats.attackRating;
    profile.damage.min = std::max(1, addBonus(stats.damageMin, equipment_.damageMin));
    profile.damage.max = std::max(profile.damage.min, addBonus(stats.damageMax, equipment_.damageMax));
    return profile;
}

game::DefenseProfile game::Actor::defenseProfile() const
{
    const DerivedStats stats = derived();

    DefenseProfile profile;
    profile.defense = stats.defense;
    profile.armor = std::max(0, equipment_.armor);
    profile.resistances = equipment_.resistances;
    return profile;
}

// ---------------------------------------------------------------------------
// Position
// ---------------------------------------------------------------------------
iso::GridPoint game::Actor::cell() const
{
    return cell_;
}

iso::PointF game::Actor::position() const
{
    return position_;
}

iso::GridPoint game::Actor::facing() const
{
    return facing_;
}

void game::Actor::moveTo(iso::GridPoint cell)
{
    // 64 bit: subtracting two valid int cells (INT_MIN vs INT_MAX) would overflow
    // and could flip the sign used for facing.
    const std::int64_t dx = static_cast<std::int64_t>(cell.x) - static_cast<std::int64_t>(cell_.x);
    const std::int64_t dy = static_cast<std::int64_t>(cell.y) - static_cast<std::int64_t>(cell_.y);

    if (dx != 0 || dy != 0)
    {
        facing_ = {dx == 0 ? 0 : (dx > 0 ? 1 : -1), dy == 0 ? 0 : (dy > 0 ? 1 : -1)};
    }

    cell_ = cell;
}

void game::Actor::placeAt(iso::GridPoint cell)
{
    cell_ = cell;
    position_ = {static_cast<float>(cell.x), static_cast<float>(cell.y)};
}

void game::Actor::advanceMovement(float cellsPerSecond, float deltaSeconds)
{
    // Reject NaN/infinity as well as non-positive values. A NaN would make every
    // comparison false and poison position_ through the interpolation below.
    if (!(deltaSeconds > 0.f) || !(cellsPerSecond > 0.f) || !std::isfinite(deltaSeconds) ||
        !std::isfinite(cellsPerSecond))
        return;

    const float targetX = static_cast<float>(cell_.x);
    const float targetY = static_cast<float>(cell_.y);
    const float dx = targetX - position_.x;
    const float dy = targetY - position_.y;
    const float distance = std::sqrt(dx * dx + dy * dy);

    if (distance <= kArrivalEpsilon)
    {
        position_ = {targetX, targetY};
        return;
    }

    const float step = cellsPerSecond * deltaSeconds;
    if (step >= distance)
    {
        position_ = {targetX, targetY};
        return;
    }

    position_.x += (dx / distance) * step;
    position_.y += (dy / distance) * step;
}

bool game::Actor::isMoving() const
{
    const float dx = static_cast<float>(cell_.x) - position_.x;
    const float dy = static_cast<float>(cell_.y) - position_.y;
    return (dx * dx + dy * dy) > kArrivalEpsilon * kArrivalEpsilon;
}

// ---------------------------------------------------------------------------
// Attack pacing
// ---------------------------------------------------------------------------
bool game::Actor::canAttack() const
{
    return isAlive() && attackCooldown_ <= 0.f;
}

void game::Actor::startAttackCooldown(float seconds)
{
    // An infinite cooldown would make canAttack() false forever and validate()
    // rejects it, so invalid input simply clears the cooldown.
    attackCooldown_ = (std::isfinite(seconds) && seconds > 0.f) ? seconds : 0.f;
}

void game::Actor::tickCooldowns(float deltaSeconds)
{
    if (!(deltaSeconds > 0.f) || !std::isfinite(deltaSeconds))
        return;

    attackCooldown_ = std::max(0.f, attackCooldown_ - deltaSeconds);
}

float game::Actor::attackCooldownRemaining() const
{
    return attackCooldown_;
}

// ---------------------------------------------------------------------------
// Integrity
// ---------------------------------------------------------------------------
bool game::Actor::validate(std::string* reason) const
{
    const auto fail = [reason](const char* message) {
        if (reason != nullptr)
            *reason = message;
        return false;
    };

    if (name_.empty())
        return fail("actor has no name");

    if (level_ < 1 || level_ > maxLevel)
        return fail("actor level is out of range");

    if (experience_ < 0)
        return fail("actor experience is negative");

    if (!game::validate(attributes_, reason))
        return false;

    if (health_ < 0)
        return fail("actor health is negative");

    if (health_ > maxHealth())
        return fail("actor health exceeds its maximum");

    if (mana_ < 0)
        return fail("actor mana is negative");

    if (mana_ > maxMana())
        return fail("actor mana exceeds its maximum");

    if (!game::validate(attackProfile(), reason))
        return false;

    if (!game::validate(defenseProfile(), reason))
        return false;

    if (attackCooldown_ < 0.f || !std::isfinite(attackCooldown_))
        return fail("actor has an invalid attack cooldown");

    if (!std::isfinite(position_.x) || !std::isfinite(position_.y))
        return fail("actor position is not finite");

    if (facing_.x == 0 && facing_.y == 0)
        return fail("actor has no facing direction");

    if (reason != nullptr)
        reason->clear();
    return true;
}

void game::Actor::clampVitals()
{
    health_ = std::clamp(health_, 0, maxHealth());
    mana_ = std::clamp(mana_, 0, maxMana());
}
