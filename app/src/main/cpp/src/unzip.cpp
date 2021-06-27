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

#include <cerrno>
#include <error.h>
#include <fcntl.h>
#include <getopt.h>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctime>
#include <unistd.h>

#include <set>
#include <string>

#include <File.h>
#include <StringUtils.h>
#include <File.h>
#include "unzip.h"
#include <HLog.h>
#include <ZipFile.h>

#define LOG_TAG "unzip"

static bool MakeDirHierarchy(const std::string &path) {
    struct stat sb{};
    if (stat(path.c_str(), &sb) != -1 && S_ISDIR(sb.st_mode)) {
        return true;
    }

    // 递归创建目录，现保证父目录创建成功
    if (!MakeDirHierarchy(hms::File::Dirname(path))) {
        return false;
    }

    // 最后创建此目录
    return (mkdir(path.c_str(), 0777) != -1);
}

static std::string GetFileNameBase(const std::string &name) {
    int lastSlash = name.find_last_of(OS_PATH_SEPARATOR);
    return name.substr(lastSlash + 1);;
}

static void
ExtractOne(hms::ZipFile &zipFile, ZipEntry &entry, const std::string &name, const char *targetDir) {
    HLOGENTRY();
    if (hms::StringUtils::StartsWith(name, "/") || hms::StringUtils::StartsWith(name, "../") ||
        name.find("/../") != std::string::npos) {
        HLOGE("bad filename %s", name.c_str());
    }

    // Where are we actually extracting to (for human-readable output)?
    std::string dstPath = targetDir;
    if (!hms::StringUtils::EndsWith(dstPath, "/")) dstPath += '/';
    dstPath += GetFileNameBase(name);

    // 创建目录
    if (!MakeDirHierarchy(hms::File::Dirname(targetDir))) {
        HLOGE("couldn't create directory hierarchy for %s", dstPath.c_str());
    }

//    // An entry in a zip file can just be a directory itself.
//    if (hms::StringUtils::EndsWith(name, "/")) {
//        if (mkdir(name.c_str(), entry.unix_mode) == -1) {
//            // If the directory already exists, that's fine.
//            if (errno == EEXIST) {
//                struct stat sb;
//                if (stat(name.c_str(), &sb) != -1 && S_ISDIR(sb.st_mode)) return;
//            }
//            HLOGE("couldn't extract directory %s", dstPath.c_str());
//        }
//        return;
//    }

    // 创建解压文件
    int fd = open(dstPath.c_str(), O_CREAT | O_WRONLY | O_CLOEXEC | O_EXCL, entry.unix_mode);
    if (fd == -1 && errno == EEXIST) {
        HLOGI("%s exsits, will overwrite it!", dstPath.c_str());
        fd = open(dstPath.c_str(), O_WRONLY | O_CREAT | O_CLOEXEC | O_TRUNC, entry.unix_mode);
    }
    if (fd == -1) {
        HLOGE("couldn't create file %s", dstPath.c_str());
        return;
    }

    // Actually extract into the file.
    HLOGV("  inflating: %s\n", dstPath.c_str());
    int err = zipFile.ExtractEntryToFile(&entry, fd);
    if (err < 0) {
        HLOGE("failed to extract %s: %s", dstPath.c_str(), zipFile.ErrorCodeString(err));
    }
    close(fd);
}


static void
Process(hms::ZipFile &zipFile, const std::string &extractfileName, const char *targetDir) {
    HLOGENTRY();
    int err = zipFile.StartIteration(nullptr, nullptr);
    if (err != 0) {
        HLOGE("couldn't iterate %s", zipFile.ErrorCodeString(err));
        return;
    }

    ZipEntry entry{};
    ZipString zipString;
    {
        HLOGTENTRY("find entry");
        while ((err = zipFile.Next(&entry, &zipString)) >= 0) {
            std::string name(zipString.name, zipString.name + zipString.name_length);
            if (extractfileName == name) {
                ExtractOne(zipFile, entry, extractfileName, targetDir);
                break;
            }
        }

    }

    if (err < -1) {
        HLOGE("failed iterating: %s", zipFile.ErrorCodeString(err));
    }
}


int extractFileFromZip(const char *zipFileName, const std::string &extractFileName, const char *dstFilePath) {
    HLOGENTRY();
    if (!zipFileName || !dstFilePath) {
        HLOGE("missing archive filename");
        return -1;
    }

    int32_t err;
    hms::ZipFile zipFile(zipFileName);

    if ((err = zipFile.OpenArchive()) != 0) {
        HLOGE("couldn't open %s: %s", zipFileName, zipFile.ErrorCodeString(err));
        return err;
    }
    Process(zipFile, extractFileName, dstFilePath);
    return 0;
}

