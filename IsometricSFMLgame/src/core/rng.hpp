// rng.hpp
#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "core/assert.hpp"

namespace iso
{
// Deterministic pseudo random generator (PCG32).
//
// Every random decision in the game (loot, damage rolls, dungeon layout, ...)
// goes through an instance of this class instead of std::rand()/std::mt19937
// globals, for two reasons:
//   * a seed reproduces a whole play session (bug reports, tests, replays),
//   * tests can assert on exact values instead of "roughly sometimes".
// The state is a single integer, so it can be saved/restored with the game.
class Rng
{
public:
    explicit Rng(std::uint64_t seed = 0x853c49e6748fea9bULL);

    // Raw 32 bit output. Advances the state.
    [[nodiscard]] std::uint32_t next();

    // Uniform value in [minInclusive, maxInclusive] (inclusive on both ends).
    // Unbiased: values outside the largest multiple of the range are rejected.
    [[nodiscard]] int range(int minInclusive, int maxInclusive);

    // Uniform float in [0, 1).
    [[nodiscard]] float unit();

    // True with the given probability (0 = never, 1 = always).
    [[nodiscard]] bool chance(float probability);

    // Picks a random element. `values` must not be empty.
    template <typename T>
    [[nodiscard]] const T& pick(const std::vector<T>& values)
    {
        ISO_ASSERT(!values.empty());
        return values[static_cast<std::size_t>(range(0, static_cast<int>(values.size()) - 1))];
    }

    // Fisher-Yates. Keeps every permutation equally likely, which matters when
    // shuffling e.g. spawn candidates so the same seed always yields the same
    // layout.
    template <typename T>
    void shuffle(std::vector<T>& values)
    {
        for (std::size_t remaining = values.size(); remaining > 1; --remaining)
        {
            const auto last = static_cast<std::size_t>(range(0, static_cast<int>(remaining) - 1));
            std::swap(values[remaining - 1], values[last]);
        }
    }

    [[nodiscard]] std::uint64_t state() const { return state_; }
    void setState(std::uint64_t state) { state_ = state; }

private:
    std::uint64_t state_ = 0;
    std::uint64_t increment_ = 1442695040888963407ULL; // PCG default stream
};
} // namespace iso
