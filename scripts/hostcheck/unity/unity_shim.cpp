#include "unity.h"

int unity_tests_run = 0;
int unity_tests_failed = 0;

void unity_begin(void) {
    unity_tests_run = 0;
    unity_tests_failed = 0;
    std::printf("---- hostcheck: begin ----\n");
}

int unity_end(void) {
    std::printf("---- hostcheck: %d test(s), %d failure(s) ----\n",
                unity_tests_run,
                unity_tests_failed);
    return unity_tests_failed;
}

void unity_fail(const char* message, const char* file, int line) {
    ++unity_tests_failed;
    std::printf("  FAIL %s:%d: %s\n", file, line, message);
}

void unity_fail_cmp(const char* expected_expr,
                    const char* actual_expr,
                    long long expected,
                    long long actual,
                    const char* file,
                    int line) {
    ++unity_tests_failed;
    std::printf("  FAIL %s:%d: expected %s == %s but got %lld vs %lld\n",
                file,
                line,
                expected_expr,
                actual_expr,
                expected,
                actual);
}
