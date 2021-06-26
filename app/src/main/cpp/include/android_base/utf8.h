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

#ifndef ANDROID_BASE_UTF8_H
#define ANDROID_BASE_UTF8_H

#include <fcntl.h>      // open
#include <stdio.h>      // fopen
#include <sys/stat.h>   // mkdir
#include <unistd.h>     // unlink

namespace android {
namespace base {

// The functions in the utf8 namespace take UTF-8 strings. For Windows, these
// are wrappers, for non-Windows these just expose existing APIs. To call these
// functions, use:
//
// // anonymous namespace to avoid conflict with existing open(), unlink(), etc.
// namespace {
//   // Import functions into anonymous namespace.
//   using namespace android::base::utf8;
//
//   void SomeFunction(const char* name) {
//     int fd = open(name, ...);  // Calls android::base::utf8::open().
//     ...
//     unlink(name);              // Calls android::base::utf8::unlink().
//   }
// }
namespace utf8 {


using ::fopen;
using ::mkdir;
using ::open;
using ::unlink;


}  // namespace utf8
}  // namespace base
}  // namespace android

#endif  // ANDROID_BASE_UTF8_H
