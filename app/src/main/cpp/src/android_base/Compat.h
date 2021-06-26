//
// Created by j00401612 on 2021/6/26.
//

#pragma once
#include <unistd.h>
#define ZD "%zd"
#define ZD_TYPE ssize_t

/*
 * Needed for cases where something should be constexpr if possible, but not
 * being constexpr is fine if in pre-C++11 code (such as a const static float
 * member variable).
 */
#define CONSTEXPR constexpr


/*
 * TEMP_FAILURE_RETRY is defined by some, but not all, versions of
 * <unistd.h>. (Alas, it is not as standard as we'd hoped!) So, if it's
 * not already defined, then define it here.
 */
#ifndef TEMP_FAILURE_RETRY
/* Used to retry syscalls that can return EINTR. */
#define TEMP_FAILURE_RETRY(exp) ({         \
    typeof (exp) _rc;                      \
    do {                                   \
        _rc = (exp);                       \
    } while (_rc == -1 && errno == EINTR); \
    _rc; })
#endif

#define OS_PATH_SEPARATOR '/'