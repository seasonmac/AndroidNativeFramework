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

#include "android_base/strings.h"

#include <stdlib.h>
#include <string.h>

#include <string>
#include <vector>

namespace android {
namespace base {

#define CHECK_NE(a, b) \
  if ((a) == (b)) abort();

std::vector<std::string> Split(const std::string& s,
                               const std::string& delimiters) {
  CHECK_NE(delimiters.size(), 0U);

  std::vector<std::string> result;

  size_t base = 0;
  size_t found;
  while (true) {
    found = s.find_first_of(delimiters, base);
    result.push_back(s.substr(base, found - base));
    if (found == s.npos) break;
    base = found + 1;
  }

  return result;
}

bool StartsWith(const std::string& s, const char* prefix) {
  return strncmp(s.c_str(), prefix, strlen(prefix)) == 0;
}


static bool EndsWith(const std::string& s, const char* suffix, bool case_sensitive) {
  size_t suffix_length = strlen(suffix);
  size_t string_length = s.size();
  if (suffix_length > string_length) {
    return false;
  }
  size_t offset = string_length - suffix_length;
  return (case_sensitive ? strncmp : strncasecmp)(s.c_str() + offset, suffix, suffix_length) == 0;
}

bool EndsWith(const std::string& s, const char* suffix) {
  return EndsWith(s, suffix, true);
}

}  // namespace base
}  // namespace android
