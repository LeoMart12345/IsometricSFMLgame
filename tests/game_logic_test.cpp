// game_logic_test.cpp
//
// Tests for the headless simulation layer: character stats, combat resolution
// and actors. They assert *relationships* (levels never lower a stat, armor never
// increases damage, the same seed replays the same fight) instead of exact
// balance numbers, so tuning stats.cpp does not break the suite.
#include <limits>
#include <string>

#include "core/rng.hpp"
#include "game/actor.hpp"
#include "game/combat.hpp"
#include "game/stats.hpp"
#include "test_framework.hpp"

namespace
{
constexpr game::CharacterClass kAllClasses[] = {game::CharacterClass::Warrior, game::CharacterClass::Rogue,
                                                game::CharacterClass::Sorcerer};

game::Actor makeActor(game::Faction faction = game::Faction::Player,
                      game::CharacterClass characterClass = game::CharacterClass::Warrior)
{
    return game::Actor(1, "Test", faction, characterClass, {3, 4});
}
} // namespace

// ---------------------------------------------------------------------------
// Stats
// ---------------------------------------------------------------------------
ISO_TEST(every_class_starts_with_valid_attributes)
{
    for (const game::CharacterClass characterClass : kAllClasses)
    {
        const game::Attributes attributes = game::startingAttributes(characterClass);
        std::string reason;

        CHECK_NOTE(game::validate(attributes, &reason), reason.c_str());
        CHECK_NOTE(game::derive(attributes, 1, characterClass).maxHealth >= 1,
                   game::name(characterClass));
    }
}

ISO_TEST(derived_stats_stay_sane_at_every_level)
{
    for (const game::CharacterClass characterClass : kAllClasses)
    {
        const game::Attributes base = game::startingAttributes(characterClass);

        for (int level = 1; level <= game::maxLevel; ++level)
        {
            const game::DerivedStats stats = game::derive(base, level, characterClass);

            CHECK_NOTE(stats.maxHealth >= 1, "health");
            CHECK_NOTE(stats.maxMana >= 0, "mana");
            CHECK_NOTE(stats.attackRating >= 1, "attack rating");
            CHECK_NOTE(stats.defense >= 0, "defense");
            CHECK_NOTE(stats.damageMin >= 1, "minimum damage");
            CHECK_NOTE(stats.damageMax > stats.damageMin, "damage range is empty");
        }
    }
}

ISO_TEST(more_vitality_and_more_levels_mean_more_health)
{
    game::Attributes attributes = game::startingAttributes(game::CharacterClass::Warrior);
    const int baseHealth = game::derive(attributes, 1, game::CharacterClass::Warrior).maxHealth;

    attributes.vitality += 5;
    const int tougherHealth = game::derive(attributes, 1, game::CharacterClass::Warrior).maxHealth;
    CHECK_NOTE(tougherHealth > baseHealth, "vitality did not add health");

    const int higherLevelHealth = game::derive(attributes, 10, game::CharacterClass::Warrior).maxHealth;
    CHECK_NOTE(higherLevelHealth > tougherHealth, "levels did not add health");
}

ISO_TEST(more_dexterity_means_more_accuracy)
{
    game::Attributes attributes = game::startingAttributes(game::CharacterClass::Rogue);
    const int baseRating = game::derive(attributes, 1, game::CharacterClass::Rogue).attackRating;

    attributes.dexterity += 10;
    const int betterRating = game::derive(attributes, 1, game::CharacterClass::Rogue).attackRating;
    CHECK(betterRating > baseRating);
}

ISO_TEST(attribute_validation_rejects_nonsense)
{
    game::Attributes attributes = game::startingAttributes(game::CharacterClass::Warrior);
    CHECK(game::validate(attributes));

    attributes.strength = game::minAttribute - 1;
    CHECK(!game::validate(attributes));

    attributes = game::startingAttributes(game::CharacterClass::Warrior);
    attributes.dexterity = game::maxAttribute + 1;
    CHECK(!game::validate(attributes));

    attributes = game::startingAttributes(game::CharacterClass::Warrior);
    attributes.energy = 0;
    CHECK(!game::validate(attributes));

    // A reason string is filled in for the F2 self-test.
    std::string reason;
    CHECK(!game::validate(attributes, &reason));
    CHECK(!reason.empty());
}

ISO_TEST(experience_curve_grows_monotonically)
{
    CHECK_EQ(game::experienceForLevel(0), 0);
    CHECK_EQ(game::experienceForLevel(1), 0);

    for (int level = 2; level <= 40; ++level)
    {
        const int previous = game::experienceForLevel(level - 1);
        const int current = game::experienceForLevel(level);

        CHECK_NOTE(current > previous, "experience requirement did not grow");
        // Later levels must be slower than earlier ones.
        CHECK(current - previous > game::experienceForLevel(level - 1) - game::experienceForLevel(level - 2));
    }
}

ISO_TEST(level_lookup_matches_the_experience_table)
{
    CHECK_EQ(game::levelForExperience(0), 1);
    CHECK_EQ(game::levelForExperience(-100), 1);

    for (int level = 1; level <= 40; ++level)
    {
        const int threshold = game::experienceForLevel(level);

        CHECK_EQ(game::levelForExperience(threshold), level);

        if (level > 1)
            CHECK_EQ(game::levelForExperience(threshold - 1), level - 1);
    }

    CHECK_EQ(game::levelForExperience(game::experienceForLevel(game::maxLevel)), game::maxLevel);
    CHECK_EQ(game::levelForExperience(2000000000), game::maxLevel);
}

// ---------------------------------------------------------------------------
// Combat
// ---------------------------------------------------------------------------
ISO_TEST(hit_chance_is_bounded_and_monotonic)
{
    // Even a hopeless attacker keeps the minimum chance: a fight must always be
    // winnable, and the UI must never show a 0% chance next to a hit.
    CHECK_EQ(game::hitChancePercent(1, 100000), game::minHitChancePercent);
    CHECK_EQ(game::hitChancePercent(100000, 1), game::maxHitChancePercent);

    for (int defense = 0; defense <= 500; defense += 25)
    {
        int previous = 0;
        for (int attackRating = 1; attackRating <= 500; attackRating += 25)
        {
            const int chance = game::hitChancePercent(attackRating, defense);
            CHECK(chance >= game::minHitChancePercent);
            CHECK(chance <= game::maxHitChancePercent);
            CHECK_NOTE(chance >= previous, "more accuracy made hitting harder");
            previous = chance;
        }
    }
}

ISO_TEST(armor_softens_but_never_removes_damage)
{
    CHECK_EQ(game::applyArmor(0, 100), 0);
    CHECK_EQ(game::applyArmor(-5, 100), 0);

    for (int damage = 1; damage <= 200; damage += 7)
    {
        int previous = damage;
        for (int armor = 0; armor <= 2000; armor += 100)
        {
            const int mitigated = game::applyArmor(damage, armor);
            CHECK(mitigated >= 1);
            CHECK_NOTE(mitigated <= previous, "armor increased damage");
            previous = mitigated;
        }
    }
}

ISO_TEST(resistance_is_capped_at_three_quarters)
{
    CHECK_EQ(game::applyResistance(100, 0), 100);
    CHECK_EQ(game::applyResistance(100, 50), 50);
    CHECK_EQ(game::applyResistance(100, game::maxResistancePercent), 25);

    // Values above the cap behave like the cap; negative resistance cannot heal.
    CHECK_EQ(game::applyResistance(100, 500), game::applyResistance(100, game::maxResistancePercent));
    CHECK_EQ(game::applyResistance(100, -200), 100);
    CHECK_EQ(game::applyResistance(0, 50), 0);
}

ISO_TEST(attack_profiles_are_validated)
{
    game::AttackProfile attack;
    CHECK(game::validate(attack));

    attack.damage = {5, 2};
    CHECK(!game::validate(attack));

    attack = game::AttackProfile{};
    attack.criticalChancePercent = 150;
    CHECK(!game::validate(attack));

    attack = game::AttackProfile{};
    attack.criticalMultiplierPercent = 90;
    CHECK(!game::validate(attack));

    game::DefenseProfile defense;
    CHECK(game::validate(defense));

    defense.armor = -1;
    CHECK(!game::validate(defense));

    defense = game::DefenseProfile{};
    defense.resistances.fire = 100000;
    CHECK(!game::validate(defense));
}

ISO_TEST(the_same_seed_replays_the_same_fight)
{
    game::AttackProfile attack;
    attack.attackRating = 60;
    attack.damage = {3, 12};
    attack.criticalChancePercent = 20;

    game::DefenseProfile defense;
    defense.defense = 40;
    defense.armor = 30;
    defense.resistances.physical = 10;

    iso::Rng first(1234);
    iso::Rng second(1234);

    for (int attackIndex = 0; attackIndex < 200; ++attackIndex)
    {
        const game::AttackResult left = game::resolveAttack(first, attack, defense);
        const game::AttackResult right = game::resolveAttack(second, attack, defense);

        CHECK(left.hit == right.hit);
        CHECK(left.critical == right.critical);
        CHECK_EQ(left.damage, right.damage);
    }
}

ISO_TEST(a_hit_always_deals_at_least_one_damage)
{
    game::AttackProfile attack;
    attack.attackRating = 1000;
    attack.damage = {1, 2};
    attack.criticalChancePercent = 0;

    game::DefenseProfile defense;
    defense.defense = 1;
    defense.armor = 100000;
    defense.resistances.physical = game::maxResistancePercent;

    iso::Rng rng(7);
    int hits = 0;
    for (int i = 0; i < 500; ++i)
    {
        const game::AttackResult result = game::resolveAttack(rng, attack, defense);
        if (!result.hit)
        {
            CHECK_EQ(result.damage, 0);
            continue;
        }

        ++hits;
        CHECK_NOTE(result.damage >= 1, "armor and resistance made a hit harmless");
    }

    CHECK_NOTE(hits > 400, "even a very accurate attacker should mostly connect");
}

ISO_TEST(a_miss_deals_no_damage)
{
    game::AttackProfile attack;
    attack.attackRating = 1;
    attack.damage = {10, 20};

    game::DefenseProfile defense;
    defense.defense = 1000000;

    iso::Rng rng(99);
    int misses = 0;
    for (int i = 0; i < 200; ++i)
    {
        const game::AttackResult result = game::resolveAttack(rng, attack, defense);
        if (result.hit)
            continue;

        ++misses;
        CHECK_EQ(result.damage, 0);
        CHECK(!result.critical);
    }

    CHECK_NOTE(misses > 0, "the worst possible attacker never missed");
}

ISO_TEST(critical_hits_hurt_more_and_resistance_helps)
{
    game::AttackProfile attack;
    attack.attackRating = 100000;
    attack.damage = {10, 10};
    attack.criticalChancePercent = 50;
    attack.criticalMultiplierPercent = 200;

    game::DefenseProfile noResistance;
    noResistance.defense = 1;

    iso::Rng rng(4242);

    long normalTotal = 0;
    long criticalTotal = 0;
    int normalCount = 0;
    int criticalCount = 0;

    for (int i = 0; i < 2000; ++i)
    {
        const game::AttackResult result = game::resolveAttack(rng, attack, noResistance);
        if (!result.hit)
        {
            CHECK_EQ(result.damage, 0);
            continue;
        }

        if (result.critical)
        {
            criticalTotal += result.damage;
            ++criticalCount;
        }
        else
        {
            normalTotal += result.damage;
            ++normalCount;
        }
    }

    CHECK(normalCount > 0);
    CHECK(criticalCount > 0);
    CHECK_NOTE(criticalTotal * normalCount > normalTotal * criticalCount, "criticals were not stronger");

    // 50% resistance on the same setup must roughly halve the damage.
    game::DefenseProfile resistant;
    resistant.defense = 1;
    resistant.resistances.physical = 50;

    iso::Rng plain(11);
    iso::Rng guarded(11);

    long plainTotal = 0;
    long guardedTotal = 0;
    for (int i = 0; i < 1000; ++i)
    {
        plainTotal += game::resolveAttack(plain, attack, noResistance).damage;
        guardedTotal += game::resolveAttack(guarded, attack, resistant).damage;
    }

    CHECK_NOTE(guardedTotal < plainTotal, "resistance did not reduce damage");
    CHECK_NOTE(guardedTotal * 3 > plainTotal, "resistance reduced damage far too much");
}

ISO_TEST(damage_types_use_their_own_resistance)
{
    game::Resistances resistances;
    resistances.fire = 50;

    CHECK_EQ(game::resistanceAgainst(resistances, game::DamageType::Fire), 50);
    CHECK_EQ(game::resistanceAgainst(resistances, game::DamageType::Cold), 0);

    game::AttackProfile fire;
    fire.attackRating = 100000;
    fire.damage = {40, 40};
    fire.criticalChancePercent = 0;
    fire.type = game::DamageType::Fire;

    game::DefenseProfile defense;
    defense.defense = 1;
    defense.armor = 500; // armor must not matter for elemental damage
    defense.resistances = resistances;

    iso::Rng rng(3);
    const game::AttackResult result = game::resolveAttack(rng, fire, defense);

    CHECK(result.hit);
    CHECK_EQ(result.type, game::DamageType::Fire);
    CHECK_EQ(result.damage, 20);
}

// ---------------------------------------------------------------------------
// Actors
// ---------------------------------------------------------------------------
ISO_TEST(a_new_actor_is_alive_and_consistent)
{
    for (const game::CharacterClass characterClass : kAllClasses)
    {
        game::Actor actor(1, "Hero", game::Faction::Player, characterClass, {2, 3});
        std::string reason;

        CHECK_NOTE(actor.validate(&reason), reason.c_str());
        CHECK(actor.isAlive());
        CHECK_EQ(actor.health(), actor.maxHealth());
        CHECK_EQ(actor.mana(), actor.maxMana());
        CHECK_EQ(actor.level(), 1);
        CHECK_EQ(actor.experience(), 0);
        CHECK_EQ(actor.cell().x, 2);
        CHECK_EQ(actor.cell().y, 3);
        CHECK(actor.isPlayer());
    }
}

ISO_TEST(damage_and_healing_respect_the_health_bounds)
{
    game::Actor actor = makeActor();
    const int maximum = actor.maxHealth();

    CHECK_EQ(actor.takeDamage(0), 0);
    CHECK_EQ(actor.takeDamage(-10), 0);

    CHECK_EQ(actor.takeDamage(10), 10);
    CHECK_EQ(actor.health(), maximum - 10);

    // Overhealing cannot exceed the maximum.
    CHECK_EQ(actor.heal(maximum * 10), 10);
    CHECK_EQ(actor.health(), maximum);

    // Overkill is clamped and death is final.
    const int applied = actor.takeDamage(maximum * 10);
    CHECK_EQ(applied, maximum);
    CHECK_EQ(actor.health(), 0);
    CHECK(!actor.isAlive());

    CHECK_EQ(actor.takeDamage(5), 0);
    CHECK_EQ(actor.heal(5), 0);
    CHECK_EQ(actor.health(), 0);

    CHECK(actor.validate());
}

ISO_TEST(mana_cannot_be_spent_twice)
{
    game::Actor actor = makeActor(game::Faction::Player, game::CharacterClass::Sorcerer);
    const int maximum = actor.maxMana();

    CHECK(maximum > 0);
    CHECK(actor.spendMana(maximum));
    CHECK_EQ(actor.mana(), 0);
    CHECK(!actor.spendMana(1));
    CHECK_EQ(actor.mana(), 0);

    CHECK_EQ(actor.restoreMana(5), 5);
    CHECK_EQ(actor.restoreMana(maximum * 10), maximum - 5);
    CHECK_EQ(actor.mana(), maximum);

    // Spending nothing always succeeds, like a free spell.
    CHECK(actor.spendMana(0));
}

ISO_TEST(levelling_up_improves_the_actor)
{
    game::Actor actor = makeActor();
    const int healthBefore = actor.maxHealth();
    const int damageBefore = actor.attackProfile().damage.max;
    const game::Attributes attributesBefore = actor.attributes();

    const int gained = actor.addExperience(game::experienceForLevel(5));

    CHECK_EQ(gained, 4);
    CHECK_EQ(actor.level(), 5);
    CHECK(actor.maxHealth() > healthBefore);
    CHECK_NOTE(actor.attackProfile().damage.max >= damageBefore, "levels lowered damage");
    CHECK(actor.attributes().vitality > attributesBefore.vitality);
    CHECK(actor.health() > healthBefore); // the new capacity is granted at once
    CHECK(actor.validate());

    // Experience that is not enough for a level changes nothing else.
    CHECK_EQ(actor.addExperience(1), 0);
    CHECK_EQ(actor.level(), 5);
    CHECK_EQ(actor.addExperience(0), 0);
    CHECK_EQ(actor.addExperience(-500), 0);
}

ISO_TEST(experience_cannot_overflow)
{
    game::Actor actor = makeActor();

    actor.addExperience(2000000000);
    actor.addExperience(2000000000);

    CHECK_EQ(actor.level(), game::maxLevel);
    CHECK(actor.experience() > 0);
    CHECK(actor.validate());
}

ISO_TEST(attack_cooldown_gates_attacks)
{
    game::Actor actor = makeActor();

    CHECK(actor.canAttack());
    actor.startAttackCooldown(0.5f);
    CHECK(!actor.canAttack());

    actor.tickCooldowns(0.25f);
    CHECK(!actor.canAttack());
    CHECK_NEAR(actor.attackCooldownRemaining(), 0.25f, 0.0001f);

    actor.tickCooldowns(0.5f);
    CHECK(actor.canAttack());
    CHECK_EQ(actor.attackCooldownRemaining(), 0.f);

    // A dead actor never attacks.
    actor.takeDamage(actor.maxHealth() * 2);
    CHECK(!actor.canAttack());
}

ISO_TEST(actors_glide_between_cells)
{
    game::Actor actor = makeActor();
    actor.placeAt({0, 0});

    CHECK(!actor.isMoving());

    actor.moveTo({2, 0});
    CHECK(actor.isMoving());
    CHECK_EQ(actor.facing().x, 1);
    CHECK_EQ(actor.facing().y, 0);

    // One second at 4 cells/second is enough for a 2 cell move.
    actor.advanceMovement(4.f, 1.f);
    CHECK(!actor.isMoving());
    CHECK_NEAR(actor.position().x, 2.f, 0.0001f);
    CHECK_NEAR(actor.position().y, 0.f, 0.0001f);

    // A diagonal move keeps walking in a straight line (no axis snapping).
    actor.moveTo({1, 1});
    actor.advanceMovement(1.f, 0.5f);
    CHECK(actor.isMoving());
    CHECK_NEAR(actor.position().x, 2.f - 0.35355339f, 0.001f);
    CHECK_NEAR(actor.position().y, 0.35355339f, 0.001f);
    CHECK_EQ(actor.facing().x, -1);
    CHECK_EQ(actor.facing().y, 1);

    // Standstill is harmless.
    actor.advanceMovement(4.f, 0.f);
    actor.advanceMovement(0.f, 1.f);
}

ISO_TEST(equipment_feeds_the_combat_profiles)
{
    game::Actor actor = makeActor();
    const game::AttackProfile bare = actor.attackProfile();
    const game::DefenseProfile unarmored = actor.defenseProfile();

    game::Equipment equipment;
    equipment.damageMin = 3;
    equipment.damageMax = 6;
    equipment.armor = 12;
    equipment.resistances.fire = 25;
    actor.setEquipment(equipment);

    const game::AttackProfile armed = actor.attackProfile();
    const game::DefenseProfile armored = actor.defenseProfile();

    CHECK_EQ(armed.damage.min, bare.damage.min + 3);
    CHECK_EQ(armed.damage.max, bare.damage.max + 6);
    CHECK_EQ(armored.armor, unarmored.armor + 12);
    CHECK_EQ(armored.resistances.fire, 25);

    // Nonsense gear must not produce a broken profile.
    game::Equipment broken;
    broken.damageMin = -5;
    broken.damageMax = -5;
    actor.setEquipment(broken);
    CHECK(actor.validate());
    CHECK(actor.attackProfile().damage.max > actor.attackProfile().damage.min);

    // Extreme gear (a typo in a data file) is clamped instead of leaving the
    // actor in a state validate() rejects.
    game::Equipment insane;
    insane.damageMax = std::numeric_limits<int>::max();
    insane.armor = -1000;
    insane.resistances.fire = std::numeric_limits<int>::max();
    insane.resistances.cold = std::numeric_limits<int>::min();
    actor.setEquipment(insane);

    CHECK(actor.validate());
    CHECK_EQ(actor.equipment().armor, 0);
    CHECK(actor.equipment().resistances.fire <= game::maxStoredResistancePercent);
    CHECK(actor.equipment().resistances.cold >= game::minResistancePercent);
    CHECK(actor.defenseProfile().armor >= 0);
}

ISO_TEST(attributes_stay_valid_when_they_shrink)
{
    game::Actor actor = makeActor();
    CHECK(actor.validate());

    // Draining vitality and energy lowers the maximum health and mana: the
    // current values must follow, or validate() reports "exceeds its maximum".
    game::Attributes drain;
    drain.vitality = -1000;
    drain.energy = -1000;
    actor.addAttributes(drain);

    CHECK(actor.validate());
    CHECK(actor.health() <= actor.maxHealth());
    CHECK(actor.mana() <= actor.maxMana());

    // Absurd deltas are clamped, not wrapped around into the valid range.
    game::Attributes surge;
    surge.strength = std::numeric_limits<int>::max();
    surge.vitality = std::numeric_limits<int>::max();
    actor.addAttributes(surge);

    CHECK(actor.validate());
    CHECK_EQ(actor.attributes().strength, game::maxAttribute);
    CHECK_EQ(actor.attributes().vitality, game::maxAttribute);
}

ISO_TEST(extreme_roll_ranges_are_safe)
{
    // The span of a full-range roll is 2^32 and does not fit in an int; the
    // computation used to overflow, divide by a zero span and raise SIGFPE.
    iso::Rng rng(424242);

    bool sawNegative = false;
    bool sawPositive = false;
    for (int i = 0; i < 500; ++i)
    {
        const int value = rng.range(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
        sawNegative = sawNegative || value < 0;
        sawPositive = sawPositive || value > 0;
    }
    CHECK(sawNegative);
    CHECK(sawPositive);

    // Both inclusive bounds are reachable at the edges of the int range.
    for (int i = 0; i < 100; ++i)
    {
        const int low = rng.range(std::numeric_limits<int>::min(), std::numeric_limits<int>::min() + 1);
        CHECK(low == std::numeric_limits<int>::min() || low == std::numeric_limits<int>::min() + 1);

        const int high = rng.range(std::numeric_limits<int>::max() - 1, std::numeric_limits<int>::max());
        CHECK(high == std::numeric_limits<int>::max() - 1 || high == std::numeric_limits<int>::max());

        CHECK_EQ(rng.range(7, 7), 7);
        CHECK(rng.range(-3, 0) <= 0);
    }
}

ISO_TEST(a_fight_can_be_played_out_headlessly)
{
    // The player and a monster trade blows using the real combat rules until one
    // dies: this is the closest thing to "the game runs" without a window.
    iso::Rng rng(20240607);

    game::Actor player(1, "Hero", game::Faction::Player, game::CharacterClass::Warrior, {0, 0});
    game::Actor monster(2, "Skeleton", game::Faction::Monster, game::CharacterClass::Rogue, {1, 0},
                        std::max(1, player.level()));

    int rounds = 0;
    while (player.isAlive() && monster.isAlive() && rounds < 10000)
    {
        ++rounds;

        const game::AttackResult swung =
            game::resolveAttack(rng, player.attackProfile(), monster.defenseProfile());
        monster.takeDamage(swung.damage);

        if (!monster.isAlive())
            break;

        const game::AttackResult bitten =
            game::resolveAttack(rng, monster.attackProfile(), player.defenseProfile());
        player.takeDamage(bitten.damage);

        CHECK(player.validate());
        CHECK(monster.validate());
        CHECK(player.health() >= 0);
        CHECK(monster.health() >= 0);
    }

    CHECK_NOTE(!player.isAlive() || !monster.isAlive(), "the fight never ended");
    CHECK(rounds < 10000);
}

int main(int argc, char** argv)
{
    return testing::run(argc, argv);
}
