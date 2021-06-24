//
// Created by season on 2021/6/24.
//

#pragma once

#include <stdint.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int64_t nsecs_t;       // nano-seconds

static constexpr inline nsecs_t seconds_to_nanoseconds(nsecs_t secs) {
    return secs * 1000000000;
}

static constexpr inline nsecs_t milliseconds_to_nanoseconds(nsecs_t secs) {
    return secs * 1000000;
}

static constexpr inline nsecs_t microseconds_to_nanoseconds(nsecs_t secs) {
    return secs * 1000;
}

static constexpr inline nsecs_t nanoseconds_to_seconds(nsecs_t secs) {
    return secs / 1000000000;
}

static constexpr inline nsecs_t nanoseconds_to_milliseconds(nsecs_t secs) {
    return secs / 1000000;
}

static constexpr inline nsecs_t nanoseconds_to_microseconds(nsecs_t secs) {
    return secs / 1000;
}

static constexpr inline nsecs_t s2ns(nsecs_t v) { return seconds_to_nanoseconds(v); }
static constexpr inline nsecs_t ms2ns(nsecs_t v) { return milliseconds_to_nanoseconds(v); }
static constexpr inline nsecs_t us2ns(nsecs_t v) { return microseconds_to_nanoseconds(v); }
static constexpr inline nsecs_t ns2s(nsecs_t v) { return nanoseconds_to_seconds(v); }
static constexpr inline nsecs_t ns2ms(nsecs_t v) { return nanoseconds_to_milliseconds(v); }
static constexpr inline nsecs_t ns2us(nsecs_t v) { return nanoseconds_to_microseconds(v); }

static constexpr inline nsecs_t seconds(nsecs_t v) { return s2ns(v); }
static constexpr inline nsecs_t milliseconds(nsecs_t v) { return ms2ns(v); }
static constexpr inline nsecs_t microseconds(nsecs_t v) { return us2ns(v); }

#define SYSTEM_TIME_MONOTONIC 1
nsecs_t systemTime(int clock = SYSTEM_TIME_MONOTONIC);

nsecs_t uptimeMillis();

#ifdef __cplusplus
} // extern "C"
#endif
