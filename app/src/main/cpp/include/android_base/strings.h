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

#ifndef ANDROID_BASE_STRINGS_H
#define ANDROID_BASE_STRINGS_H

#include <sstream>
#include <string>
#include <vector>

namespace android {
namespace base {

// Splits a string into a vector of strings.
std::vector<std::string> Split(const std::string& s,
                               const std::string& delimiters);

// Tests whether 's' starts with 'prefix'.
bool StartsWith(const std::string& s, const char* prefix);

// Tests whether 's' ends with 'suffix'.
bool EndsWith(const std::string& s, const char* suffix);

}  // namespace base
}  // namespace android

#endif  // ANDROID_BASE_STRINGS_H
