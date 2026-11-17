// rng.cpp
#include "core/rng.hpp"

namespace
{
constexpr std::uint64_t kTwoPow32 = 1ULL << 32;
constexpr float kInvTwoPow24 = 1.f / 16777216.f;
} // namespace

iso::Rng::Rng(std::uint64_t seed) : state_(seed)
{
    // Mix the seed once so that seeds 0, 1, 2, ... do not produce correlated
    // first outputs (which would make dungeons look alike).
    (void)next();
}

std::uint32_t iso::Rng::next()
{
    const std::uint64_t previous = state_;
    state_ = previous * 6364136223846793005ULL + increment_;

    const auto xorshifted = static_cast<std::uint32_t>(((previous >> 18U) ^ previous) >> 27U);
    const auto rotation = static_cast<unsigned int>(previous >> 59U);

    return (xorshifted >> rotation) | (xorshifted << ((0U - rotation) & 31U));
}

int iso::Rng::range(int minInclusive, int maxInclusive)
{
    ISO_ASSERT(minInclusive <= maxInclusive);

    // 64 bit arithmetic: int_max - int_min overflows a 32 bit int, and for a
    // full-range request the span is 2^32, which does not fit either.
    const std::uint64_t span = static_cast<std::uint64_t>(static_cast<std::int64_t>(maxInclusive) -
                                                          static_cast<std::int64_t>(minInclusive)) +
                               1U;

    // Reject the values that would make the modulo result biased. When the span
    // covers the whole 32 bit range the limit is 2^32 and nothing is rejected.
    const std::uint64_t limit = kTwoPow32 - (kTwoPow32 % span);

    std::uint32_t value = next();
    while (static_cast<std::uint64_t>(value) >= limit)
        value = next();

    // Add in 64 bit so that e.g. range(INT_MIN, INT_MAX) cannot wrap around.
    return static_cast<int>(static_cast<std::int64_t>(minInclusive) +
                            static_cast<std::int64_t>(static_cast<std::uint64_t>(value) % span));
}

float iso::Rng::unit()
{
    // 24 bits is the full precision of a float's mantissa.
    return static_cast<float>(next() >> 8U) * kInvTwoPow24;
}

bool iso::Rng::chance(float probability)
{
    if (probability <= 0.f)
        return false;
    if (probability >= 1.f)
        return true;

    return unit() < probability;
}
