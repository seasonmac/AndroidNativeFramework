//
// Created by j00401612 on 2021/6/25.
//

#pragma once

#include <sys/types.h>

namespace hms {
    class FileMap {
    public:
        FileMap(void);

        FileMap(FileMap &&f);

        FileMap &operator=(FileMap &&f);

        /*
         * Create a new mapping on an open file.
         *
         * Closing the file descriptor does not unmap the pages, so we don't
         * claim ownership of the fd.
         *
         * Returns "false" on failure.
         */
        bool create(const char *origFileName, int fd,
                    off64_t offset, size_t length, bool readOnly);

        ~FileMap(void);

        /*
         * Return the name of the file this map came from, if known.
         */
        const char *getFileName(void) const { return mFileName; }

        /*
         * Get a pointer to the piece of the file we requested.
         */
        void *getDataPtr(void) const { return mDataPtr; }

        /*
         * Get the length we requested.
         */
        size_t getDataLength(void) const { return mDataLength; }

        /*
         * Get the data offset used to create this map.
         */
        off64_t getDataOffset(void) const { return mDataOffset; }

        /*
         * This maps directly to madvise() values, but allows us to avoid
         * including <sys/mman.h> everywhere.
         */
        enum MapAdvice {
            NORMAL, RANDOM, SEQUENTIAL, WILLNEED, DONTNEED
        };

        /*
         * Apply an madvise() call to the entire file.
         *
         * Returns 0 on success, -1 on failure.
         */
        int advise(MapAdvice advice);

    protected:

    private:
        // these are not implemented
        FileMap(const FileMap &src);

        const FileMap &operator=(const FileMap &src);

        char *mFileName;      // original file name, if known
        void *mBasePtr;       // base of mmap area; page aligned
        size_t mBaseLength;    // length, measured from "mBasePtr"
        off64_t mDataOffset;    // offset used when map was created
        void *mDataPtr;       // start of requested data, offset from base
        size_t mDataLength;    // length, measured from "mDataPtr"

        static long mPageSize;
    };
}