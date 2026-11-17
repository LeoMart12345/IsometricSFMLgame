// assert.hpp
#pragma once

#include <cstdio>
#include <cstdlib>

// Checking helpers used across the game.
//
// ISO_ASSERT     programmer invariant, compiled out with NDEBUG.
// ISO_ASSERT_MSG same, but with a custom message.
// ISO_ENSURE     input validation that must always run; reports and makes the
//                enclosing (bool returning) function return false.
// ISO_UNREACHABLE() for switch statements that are supposed to cover every
//                enumerator.
//
// The point of keeping all three in one place is that the game's simulation is
// validated instead of trusted: a bug shows up as a readable message with a
// file/line instead of corrupt state that is only noticed much later.
namespace iso
{
namespace detail
{
inline void reportFailure(const char* message, const char* file, int line)
{
    std::fprintf(stderr, "[check] %s (%s:%d)\n", message, file, line);
    std::fflush(stderr);
}
} // namespace detail
} // namespace iso

#if defined(NDEBUG)
#define ISO_ASSERT(condition) ((void)0)
#define ISO_ASSERT_MSG(condition, message) ((void)0)
#else
#define ISO_ASSERT(condition)                                                                    \
    do                                                                                           \
    {                                                                                            \
        if (!(condition))                                                                        \
        {                                                                                        \
            ::iso::detail::reportFailure("assertion failed: " #condition, __FILE__, __LINE__);   \
            std::abort();                                                                        \
        }                                                                                        \
    } while (false)

#define ISO_ASSERT_MSG(condition, message)                                                       \
    do                                                                                           \
    {                                                                                            \
        if (!(condition))                                                                        \
        {                                                                                        \
            ::iso::detail::reportFailure((message), __FILE__, __LINE__);                         \
            std::abort();                                                                        \
        }                                                                                        \
    } while (false)
#endif

// Always evaluated, even in release builds: use it for data that comes from
// outside the code (assets, generated levels, save games) where silently
// continuing would corrupt the game state.
#define ISO_ENSURE(condition)                                                                    \
    do                                                                                           \
    {                                                                                            \
        if (!(condition))                                                                        \
        {                                                                                        \
            ::iso::detail::reportFailure("check failed: " #condition, __FILE__, __LINE__);       \
            return false;                                                                        \
        }                                                                                        \
    } while (false)

#define ISO_UNREACHABLE()                                                                        \
    do                                                                                           \
    {                                                                                            \
        ::iso::detail::reportFailure("unreachable code reached", __FILE__, __LINE__);            \
        std::abort();                                                                            \
    } while (false)
