//
// Created by season on 2021/6/27.
//

#pragma once
#include "Writer.h"
#include <cstdlib>
#include <HLog.h>
#define LOG_TAG "FileWriter"

class FileWriter : public Writer {
public:
    // Creates a FileWriter for |fd| and prepare to write |entry| to it,
    // guaranteeing that the file descriptor is valid and that there's enough
    // space on the volume to write out the entry completely and that the file
    // is truncated to the correct length (no truncation if |fd| references a
    // block device).
    //
    // Returns a valid FileWriter on success, |nullptr| if an error occurred.
    static std::unique_ptr<FileWriter> Create(int fd, const ZipEntry *entry) {
        const uint32_t declared_length = entry->uncompressed_length;
        const off64_t current_offset = lseek64(fd, 0, SEEK_CUR);
        if (current_offset == -1) {
            HLOGW("Zip: unable to seek to current location on fd %d: %s", fd, strerror(errno));
            return nullptr;
        }

        int result = 0;
#if defined(__linux__)
        if (declared_length > 0) {
            // Make sure we have enough space on the volume to extract the compressed
            // entry. Note that the call to ftruncate below will change the file size but
            // will not allocate space on disk and this call to fallocate will not
            // change the file size.
            // Note: fallocate is only supported by the following filesystems -
            // btrfs, ext4, ocfs2, and xfs. Therefore fallocate might fail with
            // EOPNOTSUPP error when issued in other filesystems.
            // Hence, check for the return error code before concluding that the
            // disk does not have enough space.
            char sdkVersion[PROP_VALUE_MAX];
            __system_property_get("ro.build.version.sdk", sdkVersion);
            const int sdkVersionInt = atoi(sdkVersion);
            if (sdkVersionInt > __ANDROID_API_L__) {
                result = TEMP_FAILURE_RETRY(fallocate(fd, 0, current_offset, declared_length));
            } else {
                result = TEMP_FAILURE_RETRY(ftruncate(fd, declared_length + current_offset));
            }

            if (result == -1 && errno == ENOSPC) {
                HLOGW("Zip: unable to allocate  %"
                              PRId64
                              " bytes at offset %"
                              PRId64
                              " : %s",
                      static_cast<int64_t>(declared_length), static_cast<int64_t>(current_offset),
                      strerror(errno));
                return std::unique_ptr<FileWriter>(nullptr);
            }
        }
#endif  // __linux__

        struct stat sb;
        if (fstat(fd, &sb) == -1) {
            HLOGW("Zip: unable to fstat file: %s", strerror(errno));
            return std::unique_ptr<FileWriter>(nullptr);
        }

        // Block device doesn't support ftruncate(2).
        if (!S_ISBLK(sb.st_mode)) {
            result = TEMP_FAILURE_RETRY(ftruncate(fd, declared_length + current_offset));
            if (result == -1) {
                HLOGW("Zip: unable to truncate file to %"
                PRId64
                ": %s",
                        static_cast<int64_t>(declared_length + current_offset), strerror(errno));
                return std::unique_ptr<FileWriter>(nullptr);
            }
        }

        return std::unique_ptr<FileWriter>(new FileWriter(fd, declared_length));
    }

    virtual bool Append(uint8_t *buf, size_t buf_size) override {
        if (total_bytes_written_ + buf_size > declared_length_) {
            HLOGW("Zip: Unexpected size "
            ZD
            " (declared) vs "
            ZD
            " (actual)", declared_length_,
                    total_bytes_written_ + buf_size);
            return false;
        }

        const bool result = hms::File::WriteFully(fd_, buf, buf_size);
        if (result) {
            total_bytes_written_ += buf_size;
        } else {
            HLOGW("Zip: unable to write "
            ZD
            " bytes to file; %s", buf_size, strerror(errno));
        }

        return result;
    }

private:
    FileWriter(const int fd, const size_t declared_length)
            : Writer(), fd_(fd), declared_length_(declared_length), total_bytes_written_(0) {}

    const int fd_;
    const size_t declared_length_;
    size_t total_bytes_written_;
};

