/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_BASE_MEMORY_H
#define ANDROID_BASE_MEMORY_H

#include <cstring>

namespace android {
namespace base {

// Use memcpy for access to unaligned data on targets with alignment
// restrictions.  The compiler will generate appropriate code to access these
// structures without generating alignment exceptions.
template <typename T>
static inline T get_unaligned(const void* address) {
  T result;
  //todo:memcpy_s
  memcpy(&result, address, sizeof(T));
  return result;
}

} // namespace base
} // namespace android

#endif  // ANDROID_BASE_MEMORY_H
