// test_framework.hpp
#pragma once

#include <cmath>
#include <cstdio>
#include <exception>
#include <functional>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// A deliberately tiny test harness so the tests have no external dependency
// (the project does not vendor a test library, and this is all the tests need).
//
// Usage:
//   ISO_TEST(my_thing_works) { CHECK(1 + 1 == 2); }
//   int main(int argc, char** argv) { return testing::run(argc, argv); }
//
// Tests are discovered automatically through a static registrar, `--filter
// <substring>` runs a subset, and the process exit code is non-zero when any
// check failed, which is what CTest looks at.
namespace testing
{
struct TestCase
{
    std::string name;
    std::function<void()> body;
};

inline std::vector<TestCase>& registry()
{
    static std::vector<TestCase> cases;
    return cases;
}

inline int& failureCount()
{
    static int count = 0;
    return count;
}

inline int& checkCount()
{
    static int count = 0;
    return count;
}

struct Registrar
{
    Registrar(const char* name, std::function<void()> body)
    {
        registry().push_back({name, std::move(body)});
    }
};

inline void reportFailure(const char* file, int line, const std::string& message)
{
    ++failureCount();
    std::fprintf(stderr, "    FAILED %s:%d\n           %s\n", file, line, message.c_str());
    std::fflush(stderr);
}

// Printable description of a value for failure messages. Works for anything that
// can be streamed, prints enums as their underlying value, and degrades to a
// placeholder for types with no operator<< (rather than failing to compile).
template <typename T>
std::string describe(const T& value)
{
    if constexpr (requires(std::ostream& stream, const T& item) { stream << item; })
    {
        std::ostringstream stream;
        stream << value;
        return stream.str();
    }
    else if constexpr (std::is_enum_v<T>)
    {
        return std::to_string(static_cast<long long>(static_cast<std::underlying_type_t<T>>(value)));
    }
    else
    {
        return "<unprintable value>";
    }
}

inline void check(bool condition, const char* expression, const char* file, int line,
                  const std::string& note = {})
{
    ++checkCount();
    if (condition)
        return;

    std::string message = std::string("CHECK(") + expression + ")";
    if (!note.empty())
        message += " -- " + note;

    reportFailure(file, line, message);
}

template <typename Actual, typename Expected>
void checkEqual(const Actual& actual, const Expected& expected, const char* expression, const char* file,
                int line, const std::string& note = {})
{
    ++checkCount();
    if (actual == expected)
        return;

    std::ostringstream stream;
    stream << expression << " == " << describe(expected) << " (actual: " << describe(actual) << ")";
    if (!note.empty())
        stream << " -- " << note;

    reportFailure(file, line, stream.str());
}

template <typename Actual, typename Expected, typename Tolerance>
void checkNear(const Actual& actual, const Expected& expected, const Tolerance& tolerance,
               const char* expression, const char* file, int line, const std::string& note = {})
{
    ++checkCount();
    if (std::fabs(static_cast<double>(actual) - static_cast<double>(expected)) <=
        static_cast<double>(tolerance))
        return;

    std::ostringstream stream;
    stream << expression << " ~= " << describe(expected) << " +- " << describe(tolerance)
           << " (actual: " << describe(actual) << ")";
    if (!note.empty())
        stream << " -- " << note;

    reportFailure(file, line, stream.str());
}

template <typename Callable>
void checkThrows(Callable&& callable, const char* expression, const char* file, int line)
{
    ++checkCount();
    try
    {
        callable();
    }
    catch (const std::exception&)
    {
        return;
    }
    catch (...)
    {
        return;
    }

    reportFailure(file, line, std::string("expected an exception from: ") + expression);
}

inline int run(int argc, char** argv)
{
    std::string filter;
    for (int i = 1; i < argc; ++i)
    {
        const std::string argument = argv[i];
        if (argument == "--filter" && i + 1 < argc)
            filter = argv[++i];
    }

    int executed = 0;
    for (const TestCase& test : registry())
    {
        if (!filter.empty() && test.name.find(filter) == std::string::npos)
            continue;

        const int failuresBefore = failureCount();
        std::printf("[ RUN      ] %s\n", test.name.c_str());
        std::fflush(stdout);

        try
        {
            test.body();
        }
        catch (const std::exception& exception)
        {
            reportFailure(__FILE__, __LINE__, std::string("uncaught exception: ") + exception.what());
        }
        catch (...)
        {
            reportFailure(__FILE__, __LINE__, "uncaught non-standard exception");
        }

        ++executed;
        std::printf("[ %s ] %s\n", failureCount() == failuresBefore ? "       OK" : "   FAILED",
                    test.name.c_str());
    }

    std::printf("\n%d test(s), %d check(s), %d failure(s)\n", executed, checkCount(), failureCount());

    if (executed == 0)
    {
        std::printf("no test matched the filter '%s'\n", filter.c_str());
        return 1;
    }

    return failureCount() == 0 ? 0 : 1;
}
} // namespace testing

#define ISO_TEST(name)                                                                           \
    static void name();                                                                          \
    static const ::testing::Registrar name##_registrar(#name, name);                             \
    static void name()

#define CHECK(condition) ::testing::check((condition), #condition, __FILE__, __LINE__)

#define CHECK_NOTE(condition, note) ::testing::check((condition), #condition, __FILE__, __LINE__, (note))

#define CHECK_EQ(actual, expected) ::testing::checkEqual((actual), (expected), #actual, __FILE__, __LINE__)

#define CHECK_NEAR(actual, expected, tolerance)                                                  \
    ::testing::checkNear((actual), (expected), (tolerance), #actual, __FILE__, __LINE__)

#define CHECK_THROWS(expression)                                                                 \
    ::testing::checkThrows([&] { expression; }, #expression, __FILE__, __LINE__)
