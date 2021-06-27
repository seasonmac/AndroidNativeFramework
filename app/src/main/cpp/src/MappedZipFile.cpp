//
// Created by j00401612 on 2021/6/25.
//
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <memory>
#include <vector>


#include "MappedZipFile.h"

#include "HLog.h"

#define LOG_TAG "MappedZipFile"

namespace hms {

    int MappedZipFile::GetFileDescriptor() const {
        if (!has_fd_) {
            HLOGW("Zip: MappedZipFile doesn't have a file descriptor.");
            return -1;
        }
        return fd_;
    }

    void *MappedZipFile::GetBasePtr() const {
        if (has_fd_) {
            HLOGW("Zip: MappedZipFile doesn't have a base pointer.");
            return nullptr;
        }
        return base_ptr_;
    }

    off64_t MappedZipFile::GetFileLength() const {
        if (has_fd_) {
            off64_t result = lseek64(fd_, 0, SEEK_END);
            if (result == -1) {
                HLOGE("Zip: lseek on fd %d failed: %s", fd_, strerror(errno));
            }
            return result;
        } else {
            if (base_ptr_ == nullptr) {
                HLOGE("Zip: invalid file map\n");
                return -1;
            }
            return static_cast<off64_t>(data_length_);
        }
    }

    bool MappedZipFile::SeekToOffset(off64_t offset) {
        if (has_fd_) {
            if (lseek64(fd_, offset, SEEK_SET) != offset) {
                HLOGE("Zip: lseek to %" PRId64 " failed: %s\n", offset, strerror(errno));
                return false;
            }
            return true;
        } else {
            if (offset < 0 || offset > static_cast<off64_t>(data_length_)) {
                HLOGE("Zip: invalid offset: %"
                              PRId64
                              ", data length: %"
                              PRId64
                              "\n", offset, data_length_);
                return false;
            }

            read_pos_ = offset;
            return true;
        }
    }

    bool MappedZipFile::ReadFully(void *data, size_t byte_count) {
        if (!has_fd_) {
            HLOGE("Invalid ZipFile fd");
            return false;
        }
        uint8_t *p = reinterpret_cast<uint8_t *>(data);
        size_t remaining = byte_count;
        while (remaining > 0) {
            ssize_t n = TEMP_FAILURE_RETRY(read(fd_, p, remaining));
            if (n <= 0) return false;
            p += n;
            remaining -= n;
        }
        return true;
    }

    bool MappedZipFile::ReadData(uint8_t *buffer, size_t read_amount) {
        if (has_fd_) {
            if (!ReadFully(buffer, read_amount)) {
                HLOGE("Zip: read from %d failed\n", fd_);
                return false;
            }
        } else {
            memcpy(buffer, static_cast<uint8_t *>(base_ptr_) + read_pos_, read_amount);
            read_pos_ += read_amount;
        }
        return true;
    }

// Attempts to read |len| bytes into |buf| at offset |off|.
    bool MappedZipFile::ReadAtOffset(uint8_t *buf, size_t len, off64_t off) {
        if (has_fd_) {
            if (static_cast<size_t>(TEMP_FAILURE_RETRY(pread64(fd_, buf, len, off))) != len) {
                HLOGE("Zip: failed to read at offset %"
                              PRId64
                              "\n", off);
                return false;
            }
            return true;
        }
        if (!SeekToOffset(off)) {
            return false;
        }
        return ReadData(buf, len);
    }

}