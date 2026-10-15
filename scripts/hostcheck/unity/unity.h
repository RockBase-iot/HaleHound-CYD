// Minimal host-side shim for the Unity test framework.
//
// Purpose: let the pure-logic native tests under test/native/ be compiled and
// executed on the developer's machine with a plain host C++ compiler, without
// pulling in the full Unity dependency or a PlatformIO `native` environment.
//
// Only the subset of the Unity API actually used by this repository is
// implemented. It is intentionally a *shim*: if a test needs a richer Unity
// feature, add it here rather than reaching for the upstream library.
#pragma once

#include <cstdio>
#include <cstring>

#ifdef __cplusplus
extern "C" {
#endif

extern int unity_tests_run;
extern int unity_tests_failed;

void unity_begin(void);
int unity_end(void);
void unity_fail(const char* message, const char* file, int line);
void unity_fail_cmp(const char* expected_expr,
                    const char* actual_expr,
                    long long expected,
                    long long actual,
                    const char* file,
                    int line);

#ifdef __cplusplus
}

// Best-effort conversion to a printable integer. Falls back to 0 for types
// without an integral conversion (std::string, containers, ...).
template <typename T>
static inline auto unity_to_ll(const T& value, int) -> decltype((long long)value) {
    return (long long)value;
}

template <typename T>
static inline long long unity_to_ll(const T&, long) {
    return 0LL;
}
#endif

#define UNITY_BEGIN() unity_begin()
#define UNITY_END() unity_end()

#define RUN_TEST(fn)                                  \
    do {                                              \
        ++unity_tests_run;                            \
        std::printf("[ RUN  ] %s\n", #fn);            \
        fn();                                         \
    } while (0)

#define UNITY_FAIL_(message)                          \
    do {                                              \
        unity_fail((message), __FILE__, __LINE__);    \
        return;                                       \
    } while (0)

#define TEST_ASSERT_TRUE(expr)                        \
    do {                                              \
        if (!(expr)) UNITY_FAIL_("expected true: " #expr); \
    } while (0)

// Message-carrying variants. Used where the failing value is a `const char*`
// detail string rather than something printable as an integer.
#define TEST_ASSERT_TRUE_MESSAGE(expr, message)       \
    do {                                              \
        if (!(expr)) UNITY_FAIL_(message);            \
    } while (0)

#define TEST_ASSERT_FALSE_MESSAGE(expr, message)      \
    do {                                              \
        if ((expr)) UNITY_FAIL_(message);             \
    } while (0)

#define TEST_ASSERT_FALSE(expr)                       \
    do {                                              \
        if ((expr)) UNITY_FAIL_("expected false: " #expr); \
    } while (0)

#define TEST_ASSERT_NULL(ptr)                         \
    do {                                              \
        if ((ptr) != NULL) UNITY_FAIL_("expected NULL: " #ptr); \
    } while (0)

#define TEST_ASSERT_NOT_NULL(ptr)                     \
    do {                                              \
        if ((ptr) == NULL) UNITY_FAIL_("expected non-NULL: " #ptr); \
    } while (0)

#define UNITY_ASSERT_CMP_(expected, actual, op, text) \
    do {                                              \
        if (!((expected) op (actual))) {              \
            unity_fail_cmp(#expected,                 \
                           #actual,                   \
                           unity_to_ll((expected), 0),\
                           unity_to_ll((actual), 0),  \
                           __FILE__,                  \
                           __LINE__);                 \
            return;                                   \
        }                                             \
        (void)text;                                   \
    } while (0)

#define TEST_ASSERT_EQUAL(expected, actual) \
    UNITY_ASSERT_CMP_(expected, actual, ==, "equal")
#define TEST_ASSERT_EQUAL_INT(expected, actual) \
    UNITY_ASSERT_CMP_(expected, actual, ==, "equal int")

// Typed equality variants. Upstream Unity declares a distinct overload per
// width; the shim reduces them all to the same comparison because it prints
// both values as 64-bit integers anyway. They exist so a suite written against
// the real Unity API compiles here unchanged.
#define TEST_ASSERT_EQUAL_UINT(expected, actual) \
    UNITY_ASSERT_CMP_(expected, actual, ==, "equal uint")
#define TEST_ASSERT_EQUAL_INT8(expected, actual) \
    UNITY_ASSERT_CMP_(expected, actual, ==, "equal int8")
#define TEST_ASSERT_EQUAL_INT16(expected, actual) \
    UNITY_ASSERT_CMP_(expected, actual, ==, "equal int16")
#define TEST_ASSERT_EQUAL_INT32(expected, actual) \
    UNITY_ASSERT_CMP_(expected, actual, ==, "equal int32")
#define TEST_ASSERT_EQUAL_INT64(expected, actual) \
    UNITY_ASSERT_CMP_(expected, actual, ==, "equal int64")
#define TEST_ASSERT_EQUAL_UINT8(expected, actual) \
    UNITY_ASSERT_CMP_(expected, actual, ==, "equal uint8")
#define TEST_ASSERT_EQUAL_UINT16(expected, actual) \
    UNITY_ASSERT_CMP_(expected, actual, ==, "equal uint16")
#define TEST_ASSERT_EQUAL_UINT32(expected, actual) \
    UNITY_ASSERT_CMP_(expected, actual, ==, "equal uint32")
#define TEST_ASSERT_EQUAL_UINT64(expected, actual) \
    UNITY_ASSERT_CMP_(expected, actual, ==, "equal uint64")
#define TEST_ASSERT_EQUAL_CHAR(expected, actual) \
    UNITY_ASSERT_CMP_(expected, actual, ==, "equal char")
#define TEST_ASSERT_EQUAL_HEX8(expected, actual) \
    UNITY_ASSERT_CMP_(expected, actual, ==, "equal hex8")
#define TEST_ASSERT_EQUAL_HEX16(expected, actual) \
    UNITY_ASSERT_CMP_(expected, actual, ==, "equal hex16")
#define TEST_ASSERT_EQUAL_HEX32(expected, actual) \
    UNITY_ASSERT_CMP_(expected, actual, ==, "equal hex32")
#define TEST_ASSERT_EQUAL_HEX64(expected, actual) \
    UNITY_ASSERT_CMP_(expected, actual, ==, "equal hex64")

#define TEST_ASSERT_NOT_EQUAL(expected, actual) \
    UNITY_ASSERT_CMP_(expected, actual, !=, "not equal")
#define TEST_ASSERT_GREATER_THAN(threshold, actual) \
    UNITY_ASSERT_CMP_(actual, threshold, >, "greater than")
#define TEST_ASSERT_GREATER_OR_EQUAL(threshold, actual) \
    UNITY_ASSERT_CMP_(actual, threshold, >=, "greater or equal")
#define TEST_ASSERT_LESS_THAN(threshold, actual) \
    UNITY_ASSERT_CMP_(actual, threshold, <, "less than")
#define TEST_ASSERT_LESS_OR_EQUAL(threshold, actual) \
    UNITY_ASSERT_CMP_(actual, threshold, <=, "less or equal")

#define TEST_ASSERT_EQUAL_STRING(expected, actual)                  \
    do {                                                            \
        const char* unity_e_ = (const char*)(expected);              \
        const char* unity_a_ = (const char*)(actual);                \
        if (unity_e_ == NULL || unity_a_ == NULL) {                  \
            if (unity_e_ != unity_a_)                                \
                UNITY_FAIL_("string compare against NULL");          \
        } else if (std::strcmp(unity_e_, unity_a_) != 0) {           \
            std::printf("  expected \"%s\"\n  actual   \"%s\"\n", unity_e_, unity_a_); \
            UNITY_FAIL_("strings differ: " #expected " vs " #actual);\
        }                                                            \
    } while (0)
