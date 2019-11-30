#pragma once
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

extern char _ax_test_fail_reason[1024];
extern char _ax_test_fail_loc[256];
extern void _ax_test_fail();

extern const char* _ax_true_str;
extern const char* _ax_false_str;

/* int fmt_int(char* dst, int x) { return sprintf(dst, "%d", x); } */
/* int fmt_float(char* dst, float x) { return sprintf(dst, "%.3f", x); } */
/* int eq_int(int x, int y) { return x == y; } */
/* int eq_float(float x, float y) { return (x > y) ? (x - y < 0.0001) : (y - x < 0.0001); } */

#define TEST(_name) void test_ ## _name ()

#define CHECK(_b, ...) do {                             \
        if (!(_b)) {                                    \
            sprintf(_ax_test_fail_loc, "%s:%d",         \
                    __FILE__, __LINE__);                \
            sprintf(_ax_test_fail_reason, __VA_ARGS__); \
            _ax_test_fail();                            \
        } } while(0)

#define CHECK_IEQ(_lhs, _rhs)                   \
    CHECK((_lhs) == (_rhs),                     \
          "%d does not equal %d",               \
          (_lhs), (_rhs))

#define CHECK_SZEQ(_lhs, _rhs)                  \
    CHECK((_lhs) == (_rhs),                     \
          "%zu does not equal %zu",             \
          (_lhs), (_rhs))

#define CHECK_IEQ_HEX(_lhs, _rhs)               \
    CHECK((_lhs) == (_rhs),                     \
          "0x%x does not equal 0x%x",           \
          (_lhs), (_rhs))

#define CHECK_STREQ(_lhs, _rhs)                 \
    CHECK(strcmp(_lhs, _rhs) == 0,              \
          "\"%s\" does not equal \"%s\"",       \
          (_lhs), (_rhs))

#define CHECK_PEQ(_lhs, _rhs)                   \
    CHECK((void*) (_lhs) == (void*) (_rhs),     \
          "%p does not equal %p",               \
          (_lhs), (_rhs))

#define CHECK_BEQ(_lhs, _rhs)                       \
    CHECK((_lhs) == (_rhs),                         \
          "%s does not equal %s",                   \
          (_lhs) ? _ax_true_str : _ax_false_str,    \
          (_rhs) ? _ax_true_str : _ax_false_str)

#define CHECK_TRUE(_x) CHECK_BEQ(_x, true)
#define CHECK_FALSE(_x) CHECK_BEQ(_x, false)

static inline int _ax_float_eq_threshold(float t, float x, float y)
{
    if (x < y) {
        return (y - x) < t;
    } else {
        return (x - y) < t;
    }
}

#define CHECK_FLEQ(_threshold, _lhs, _rhs)                  \
    CHECK(_ax_float_eq_threshold(_threshold, _lhs, _rhs),   \
          "%.3f does not equal %.3f",                       \
          (_lhs), (_rhs))

#define CHECK_POSEQ(_lhs, _rhs)                                 \
    CHECK(_ax_float_eq_threshold(0.01, (_lhs).x, (_rhs).x) &&   \
          _ax_float_eq_threshold(0.01, (_lhs).y, (_rhs).y),     \
          "(%.2f, %.2f) does not equal (%.2f, %.2f)",           \
          (_lhs).x, (_lhs).y, (_rhs).x, (_rhs).y)

#define CHECK_DIMEQ(_lhs, _rhs)                                 \
    CHECK(_ax_float_eq_threshold(0.01, (_lhs).w, (_rhs).w) &&   \
          _ax_float_eq_threshold(0.01, (_lhs).h, (_rhs).h),     \
          "%.2fx%.2f does not equal %.2fx%.2f",                 \
          (_lhs).w, (_lhs).h, (_rhs).w, (_rhs).h)
