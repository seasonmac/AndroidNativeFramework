/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <set>
#include <string>

#include <android_base/file.h>
#include <android_base/strings.h>
#include <zip_archive.h>
#include <android_base/file.h>
#include "unzip.h"
#include <HLog.h>

#define LOG_TAG "unzip"

enum OverwriteMode {
    kAlways,
    kNever,
    kPrompt,
};

static OverwriteMode overwrite_mode = kPrompt;
static const char *archive_name = nullptr;

static bool MakeDirectoryHierarchy(const std::string &path) {
    // stat rather than lstat because a symbolic link to a directory is fine too.
    struct stat sb;
    if (stat(path.c_str(), &sb) != -1 && S_ISDIR(sb.st_mode)) return true;

    // Ensure the parent directories exist first.
    if (!MakeDirectoryHierarchy(android::base::Dirname(path))) return false;

    // Then try to create this directory.
    return (mkdir(path.c_str(), 0777) != -1);
}


static std::string GetFileNameBase(const std::string &name) {
    int lastslash = name.find_last_of("/");
    return name.substr(lastslash + 1);;
}

static void
ExtractOne(ZipArchiveHandle zah, ZipEntry &entry, const std::string &name, const char *targetDir) {
    HLOGENTRY();
    if (android::base::StartsWith(name, "/") || android::base::StartsWith(name, "../") ||
        name.find("/../") != std::string::npos) {
        HLOGE("bad filename %s", name.c_str());
    }

    // Where are we actually extracting to (for human-readable output)?
    std::string dst = targetDir;
    if (!android::base::EndsWith(dst, "/")) dst += '/';
    dst += GetFileNameBase(name);

    // Ensure the directory hierarchy exists.
    if (!MakeDirectoryHierarchy(android::base::Dirname(name))) {
        HLOGE("couldn't create directory hierarchy for %s", dst.c_str());
    }

    // An entry in a zip file can just be a directory itself.
    if (android::base::EndsWith(name, "/")) {
        if (mkdir(name.c_str(), entry.unix_mode) == -1) {
            // If the directory already exists, that's fine.
            if (errno == EEXIST) {
                struct stat sb;
                if (stat(name.c_str(), &sb) != -1 && S_ISDIR(sb.st_mode)) return;
            }
            HLOGE("couldn't extract directory %s", dst.c_str());
        }
        return;
    }

    // Create the file.
    int fd = open(name.c_str(), O_CREAT | O_WRONLY | O_CLOEXEC | O_EXCL, entry.unix_mode);
    if (fd == -1 && errno == EEXIST) {
        if (overwrite_mode == kNever) return;
        // Either overwrite_mode is kAlways or the user consented to this specific case.
        fd = open(name.c_str(), O_WRONLY | O_CREAT | O_CLOEXEC | O_TRUNC, entry.unix_mode);
    }
    if (fd == -1) {
        HLOGE("couldn't create file %s", dst.c_str());
        return;
    }

    // Actually extract into the file.
    HLOGV("  inflating: %s\n", dst.c_str());
    int err = ExtractEntryToFile(zah, &entry, fd);
    if (err < 0) {
        HLOGE("failed to extract %s: %s", dst.c_str(), ErrorCodeString(err));
    }
    close(fd);
}

static void Process(ZipArchiveHandle zah, const std::string &fileName, const char *targetDir) {
    HLOGENTRY();
    void *cookie;
    int err = StartIteration(zah, &cookie, nullptr, nullptr);
    if (err != 0) {
        HLOGE("couldn't iterate %s", ErrorCodeString(err));
        return;
    }

    ZipEntry entry;
    ZipString string;
    {
        HLOGTENTRY("find entry");
        while ((err = Next(cookie, &entry, &string)) >= 0) {
            std::string name(string.name, string.name + string.name_length);
            if (fileName == name) {
                ExtractOne(zah, entry, name, targetDir);
                break;
            }
        }

    }

    if (err < -1) {
        HLOGE("failed iterating: %s", ErrorCodeString(err));
    }
    EndIteration(cookie);
}

int unzip(const char *archive_name, const char *targetDir, const std::string &fileName) {
    HLOGENTRY();
    if (!archive_name) {
        HLOGE("missing archive filename");
        return -1;
    }

    ZipArchiveHandle zah;
    int32_t err;
    if ((err = OpenArchive(archive_name, &zah)) != 0) {
        HLOGE("couldn't open %s: %s", archive_name, ErrorCodeString(err));
        return err;
    }
    if (targetDir != nullptr && chdir(targetDir) == -1) {
        HLOGE("couldn't chdir to %s,error is %s", targetDir, strerror(errno));
        return errno;
    }
    Process(zah, fileName, targetDir);

    CloseArchive(zah);
    return 0;
}

