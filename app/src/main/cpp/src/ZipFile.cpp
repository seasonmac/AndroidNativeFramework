//
// Created by season on 2021/6/24.
//

#include "ZipFile.h"
#include "HLog.h"

#define LOG_TAG "ZipFile"

namespace hms {
    ZipFile::ZipFile(std::string& archive_name)
        :mZip(archive_name){
    }

    int ZipFile::unCompress(const char *extractFileName) {
        HLOGENTRY();
        if (extractFileName == nullptr) {
            HLOGE("missing archive filename");
            return -1;
        }

        // We can't support "-" to unzip from stdin because libziparchive relies on mmap.
//        ZipArchiveHandle zah;
//        int32_t err;
//        if ((err = OpenArchive(archive_name, &zah)) != 0) {
//            error(1, 0, "couldn't open %s: %s", archive_name, ErrorCodeString(err));
//        }
//
//        ProcessAll(zah);
//
//        CloseArchive(zah);
        return 0;
    }
}