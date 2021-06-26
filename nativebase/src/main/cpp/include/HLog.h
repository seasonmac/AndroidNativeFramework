//
// Created by season on 2021/6/24.
//

#pragma once

#include <android/Log.h>
#include <stdio.h>
#include <syscall.h>
#include <unistd.h>
#include <cstdint>
#include <chrono>

#include "base/Macros.h"

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 0   // HLOGI - 2; HLOGV -0
#endif  // DEBUG_LEVEL

#if DEBUG_LEVEL < 6
//  close all the logs
#ifndef __BASEFILE__
#    define __BASEFILE__ strrchr(__FILE__, '/')
#endif  // __BASEFILE__

#ifndef LOG_IF_ENABLED
#ifndef __E_LOG_VERBOSE
#define __E_LOG_VERBOSE 1
#endif
#ifndef __E_LOG_DEBUG
#define __E_LOG_DEBUG 1
#endif
#ifndef __E_LOG_INFO
#define __E_LOG_INFO 1
#endif
#ifndef __E_LOG_WARN
#define __E_LOG_WARN 1
#endif
#ifndef __E_LOG_ERROR
#define __E_LOG_ERROR 1
#endif
#ifndef __E_LOG_FATAL
#define __E_LOG_FATAL 1
#endif
#define LOG_IF_ENABLED(priority, tag, ...) \
    ( (CONDITION(__E_##priority)) \
    ? ((void)__android_log_print(ANDROID_##priority, tag, __VA_ARGS__)) \
    : (void)0 )

#endif

#define CONDITION(cond)     (__builtin_expect((cond)!=0, 0))

class __CallLog__ {
private:
    const char *logtag;
    const char *file;
    int32_t line;
    const char *func;
    std::chrono::time_point<std::chrono::high_resolution_clock> mEnterTime;
public:
    inline __CallLog__(const char *__logtag, const char *__file, int32_t __line, const char *__func)
            :
            logtag(__logtag), file(__file), line(__line), func(__func),
            mEnterTime(std::chrono::high_resolution_clock::now()) {
        LOG_IF_ENABLED(LOG_DEBUG, logtag, "TID:%d ...%s:%d:\tEnter %s\n",
                       static_cast<int32_t>(syscall(__NR_gettid)), strrchr(file, '/'), line, func);
    }

    inline void timeDiff(int32_t diffLine) {
        LOG_IF_ENABLED(LOG_DEBUG, logtag,
                       "TID:%d ...%s:%d:\tTime diff from line %d is %lld millis\n",
                       static_cast<int32_t>(syscall(__NR_gettid)), strrchr(file, '/'), line,
                       diffLine, std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - mEnterTime).count());
    }

    inline ~__CallLog__() {
        LOG_IF_ENABLED(LOG_DEBUG, logtag, "TID:%d ...%s:%d:\tLeave %s (takes %llu millis)\n",
                       static_cast<int32_t>(syscall(__NR_gettid)), strrchr(file, '/'), line, func,
                       std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - mEnterTime).count());
    }
};

#define HLOGENTRY(args...) __CallLog__ __call_log__(LOG_TAG, __FILE__, __LINE__, __FUNCTION__);
#define HLOGTENTRY(logtag, args...) __CallLog__ __call_log__(logtag, __FILE__, __LINE__, __FUNCTION__);

#define HLOGTIMEDIFF(args...) __call_log__.timeDiff(__LINE__);

#define HLOGTRACE(args...) \
 do { \
     android::CallStack callStack; \
     callStack.update(1); \
     int32_t _cs_size = callStack.size(); \
     HLOGI("Call stack:"); \
     for (int32_t _cs_i = 0; _cs_i < _cs_size; ++_cs_i) \
     { \
         LOG_IF_ENABLED(LOG_INFO, LOG_TAG, "\t%s", callStack.toString(_cs_i).string()); \
     } \
 }while(0)

#define HLOGTODO() HLOGV("TODO %s is not implemented yet.", __func__)
#define HLOGTTODO(logtag) HLOGTV(logtag, "TODO %s is not implemented yet.", __func__)
#define HLOGIMPL(logtype, logtag, format, args...) \
        do { \
            LOG_IF_ENABLED(logtype, logtag, "TID:%d ...%s:%d:\t" format "\n", static_cast<int32_t>(syscall(__NR_gettid)), __BASEFILE__, __LINE__, ##args); \
        }while(0)

#define HLOG(format, args...) \
    HLOGIMPL(LOG_DEBUG, LOG_TAG, format, ##args)

#define HLOGV(format, args...) \
    HLOGIMPL(LOG_VERBOSE, LOG_TAG, format, ##args)

#define HLOGD(format, args...) \
    HLOGIMPL(LOG_DEBUG, LOG_TAG, format, ##args)

#define HLOGI(format, args...) \
    HLOGIMPL(LOG_INFO, LOG_TAG, format, ##args)

#define HLOGW(format, args...) \
    HLOGIMPL(LOG_WARN, LOG_TAG, format, ##args)

#define HLOGE(format, args...) \
         do { \
            HLOGIMPL(LOG_ERROR, LOG_TAG, format, ##args); \
         }while(0)


#if DEBUG_LEVEL > 0
#    undef HLOGV
#    undef HLOGTV
#    define HLOGV(format, args...) (void)0
#    define HLOGTV(logtag, format, args...) (void)0
#endif  //
#if DEBUG_LEVEL > 1
#    undef HLOG
#    undef HLOGD
#    undef HLOGENTRY
#    undef HLOGTIMEDIFF
#    define HLOG(format, args...) (void)0
#    define HLOGD(format, args...) (void)0
#    define HLOGENTRY(args...) (void)0
#    define HLOGTIMEDIFF(args...) (void)0
#endif  //
#if DEBUG_LEVEL > 2
#    undef HLOGI
#    undef HLOGTI
#    undef HLOGTRACE
#    define HLOGI(format, args...) (void)0
#    define HLOGTRACE(args...) (void)0
#endif  //
#if DEBUG_LEVEL > 3
#    undef HLOGW
#    define HLOGW(format, args...) (void)0
#endif  //
#if DEBUG_LEVEL > 4
#    undef HLOGE
#    undef HLOGSE
#    define HLOGE(format, args...) (void)0
#endif  //
#else  // DEBUG_LEVEL >= 6
#define HLOGIMPL(logtype, logtag, format, args...) (void)0
#define HLOG(format, args...) (void)0
#define HLOGV(format, args...) (void)0
#define HLOGD(format, args...) (void)0
#define HLOGI(format, args...) (void)0
#define HLOGE(format, args...) (void)0
#define HLOGW(format, args...) (void)0
#define HLOGF(format, args...) (void)0
#define HLOGT(logtype, logtag, format, args...) (void)0
#define HLOGSE(format, args...) (void)0
#define HLOGENTRY(args...) (void)0
#define HLOGTIMEDIFF(args...) (void) 0
#define HLOGTODO() (void)0
#define HLOGTTODO(logtag) (void)0
#endif

