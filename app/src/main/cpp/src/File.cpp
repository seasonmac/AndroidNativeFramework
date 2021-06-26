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

#include "File.h"

#include <cerrno>
#include <fcntl.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <memory>
#include <string>
#include <vector>
#include <HLog.h>

#define LOG_TAG "File"

#include "Macros.h"  // For TEMP_FAILURE_RETRY on Darwin.

namespace hms {

    bool File::WriteFully(int fd, const void *data, size_t byte_count) {
        const uint8_t *p = reinterpret_cast<const uint8_t *>(data);
        size_t remaining = byte_count;
        while (remaining > 0) {
            ssize_t n = TEMP_FAILURE_RETRY(write(fd, p, remaining));
            if (n == -1) return false;
            p += n;
            remaining -= n;
        }
        return true;
    }

    std::string File::Dirname(const std::string &path) {
        // Copy path because dirname may modify the string passed in.
        std::string result(path);

        // Note that if std::string uses copy-on-write strings, &str[0] will cause
        // the copy to be made, so there is no chance of us accidentally writing to
        // the storage for 'path'.
        char *parent = dirname(&result[0]);

        // In case dirname returned a pointer to a process global, copy that string
        // before leaving the lock.
        result.assign(parent);

        return result;
    }
}
