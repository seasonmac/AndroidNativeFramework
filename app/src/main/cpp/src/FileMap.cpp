//
// Created by j00401612 on 2021/6/25.
//

#include "FileMap.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <memory.h>
#include <errno.h>
#include <assert.h>
#include "HLog.h"
#define LOG_TAG "FileMap"

namespace hms {
    long FileMap::mPageSize = -1;

// Constructor.  Create an empty object.
    FileMap::FileMap(void)
            : mFileName(NULL),
              mBasePtr(NULL),
              mBaseLength(0),
              mDataPtr(NULL),
              mDataLength(0)
    {
    }

// Move Constructor.
    FileMap::FileMap(FileMap&& other)
            : mFileName(other.mFileName), mBasePtr(other.mBasePtr), mBaseLength(other.mBaseLength),
              mDataOffset(other.mDataOffset), mDataPtr(other.mDataPtr), mDataLength(other.mDataLength)
    {
        other.mFileName = NULL;
        other.mBasePtr = NULL;
        other.mDataPtr = NULL;
    }

// Move assign operator.
    FileMap& FileMap::operator=(FileMap&& other) {
        mFileName = other.mFileName;
        mBasePtr = other.mBasePtr;
        mBaseLength = other.mBaseLength;
        mDataOffset = other.mDataOffset;
        mDataPtr = other.mDataPtr;
        mDataLength = other.mDataLength;
        other.mFileName = NULL;
        other.mBasePtr = NULL;
        other.mDataPtr = NULL;
        return *this;
    }

// Destructor.
    FileMap::~FileMap(void)
    {
        if (mFileName != NULL) {
            free(mFileName);
        }
        if (mBasePtr && munmap(mBasePtr, mBaseLength) != 0) {
            HLOGD("munmap(%p, %zu) failed\n", mBasePtr, mBaseLength);
        }
    }


// Create a new mapping on an open file.
//
// Closing the file descriptor does not unmap the pages, so we don't
// claim ownership of the fd.
//
// Returns "false" on failure.
    bool FileMap::create(const char* origFileName, int fd, off64_t offset, size_t length,
                         bool readOnly)
    {
        int     prot, flags, adjust;
        off64_t adjOffset;
        size_t  adjLength;

        void* ptr;

        assert(fd >= 0);
        assert(offset >= 0);
        assert(length > 0);

        // init on first use
        if (mPageSize == -1) {
            mPageSize = sysconf(_SC_PAGESIZE);
            if (mPageSize == -1) {
                HLOGE("could not get _SC_PAGESIZE\n");
                return false;
            }
        }

        adjust = offset % mPageSize;
        adjOffset = offset - adjust;
        adjLength = length + adjust;

        flags = MAP_SHARED;
        prot = PROT_READ;
        if (!readOnly)
            prot |= PROT_WRITE;

        ptr = mmap(NULL, adjLength, prot, flags, fd, adjOffset);
        if (ptr == MAP_FAILED) {
            HLOGE("mmap(%lld,%zu) failed: %s\n",
                  (long long)adjOffset, adjLength, strerror(errno));
            return false;
        }
        mBasePtr = ptr;

        mFileName = origFileName != NULL ? strdup(origFileName) : NULL;
        mBaseLength = adjLength;
        mDataOffset = offset;
        mDataPtr = (char*) mBasePtr + adjust;
        mDataLength = length;

        assert(mBasePtr != NULL);

        HLOGV("MAP: base %p/%zu data %p/%zu\n",
              mBasePtr, mBaseLength, mDataPtr, mDataLength);

        return true;
    }

}