//
// Created by season on 2021/6/25.
//
#include "Timers.h"
nsecs_t systemTime(int clock) {
    static const clockid_t clocks[] = {
            CLOCK_REALTIME,
            CLOCK_MONOTONIC,
            CLOCK_PROCESS_CPUTIME_ID,
            CLOCK_THREAD_CPUTIME_ID,
            CLOCK_BOOTTIME
    };
    struct timespec t;
    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(clocks[clock], &t);
    return nsecs_t(t.tv_sec)*1000000000LL + t.tv_nsec;
}

nsecs_t uptimeMillis() {
    nsecs_t when = systemTime(SYSTEM_TIME_MONOTONIC);
    return nanoseconds_to_milliseconds(when);
}