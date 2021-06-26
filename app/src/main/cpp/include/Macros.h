//
// Created by season on 2021/6/24.
//

#pragma once

#define CONDITION(cond)     (__builtin_expect((cond)!=0, 0))

#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(exp)            \
  ({                                       \
    decltype(exp) _rc;                     \
    do {                                   \
      _rc = (exp);                         \
    } while (_rc == -1 && errno == EINTR); \
    _rc;                                   \
  })
#endif


#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&) = delete;      \
  void operator=(const TypeName&) = delete



template <typename T, size_t N>
char(&ArraySizeHelper(T(&array)[N]))[N];

#define arraysize(array) (sizeof(ArraySizeHelper(array)))


#define ZD "%zd"
#define ZD_TYPE ssize_t
#define OS_PATH_SEPARATOR '/'

namespace hms {
    template<typename T>
    static inline T get_unaligned(const void *address) {
      T result;
      //todo:memcpy_s
      memcpy(&result, address, sizeof(T));
      return result;
    }
}