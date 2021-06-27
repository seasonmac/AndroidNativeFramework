//
// Created by season on 2021/6/27.
//

#pragma once

#include <ZipEntry.h>
#include <ZipString.h>
#include <sys/types.h>
#include <cstdlib>
#include <fstream>
#include <zconf.h>
#include "Writer.h"
#include "CentralDirectory.h"
#include "FileMap.h"
#include "MappedZipFile.h"
#include "IterationHandle.h"
#include <ZipFileCommon.h>

namespace hms {
//    typedef void *ZipFileHandle;
    enum {
        kCompressStored = 0,    // no compression
        kCompressDeflated = 8,  // standard deflate
    };
    static const char *kErrorMessages[] = {
            "Success",
            "Iteration ended",
            "Zlib error",
            "Invalid file",
            "Invalid handle",
            "Duplicate entries in archive",
            "Empty archive",
            "Entry not found",
            "Invalid offset",
            "Inconsistent information",
            "Invalid entry name",
            "I/O error",
            "File mapping failed",
    };
    enum ErrorCodes : int32_t {
        kIterationEnd = -1,

        // We encountered a Zlib error when inflating a stream from this file.
        // Usually indicates file corruption.
        kZlibError = -2,

        // The input file cannot be processed as a zip archive. Usually because
        // it's too small, too large or does not have a valid signature.
        kInvalidFile = -3,

        // An invalid iteration / ziparchive handle was passed in as an input
        // argument.
        kInvalidHandle = -4,

        // The zip archive contained two (or possibly more) entries with the same
        // name.
        kDuplicateEntry = -5,

        // The zip archive contains no entries.
        kEmptyArchive = -6,

        // The specified entry was not found in the archive.
        kEntryNotFound = -7,

        // The zip archive contained an invalid local file header pointer.
        kInvalidOffset = -8,

        // The zip archive contained inconsistent entry information. This could
        // be because the central directory & local file header did not agree, or
        // if the actual uncompressed length or crc32 do not match their declared
        // values.
        kInconsistentInformation = -9,

        // An invalid entry name was encountered.
        kInvalidEntryName = -10,

        // An I/O related system call (read, lseek, ftruncate, map) failed.
        kIoError = -11,

        // We were not able to mmap the central directory or entry contents.
        kMmapFailed = -12,

        kLastErrorCode = kMmapFailed,
    };

    class ZipFile {
    public:
        /*
         * Open a Zip archive, and sets handle to the value of the opaque
         * handle for the file. This handle must be released by calling
         * CloseArchive with this handle.
         *
         * Returns 0 on success, and negative values on failure.
         */
        int32_t OpenArchive();

        /*
         * Start iterating over all entries of a zip file. The order of iteration
         * is not guaranteed to be the same as the order of elements
         * in the central directory but is stable for a given zip file. |cookie| will
         * contain the value of an opaque cookie which can be used to make one or more
         * calls to Next. All calls to StartIteration must be matched by a call to
         * EndIteration to free any allocated memory.
         *
         * This method also accepts optional prefix and suffix to restrict iteration to
         * entry names that start with |optional_prefix| or end with |optional_suffix|.
         *
         * Returns 0 on success and negative values on failure.
         */
        int32_t StartIteration(const ZipString *optional_prefix, const ZipString *optional_suffix);

        /*
         * Advance to the next element in the zipfile in iteration order.
         *
         * Returns 0 on success, -1 if there are no more elements in this
         * archive and lower negative values on failure.
         */
        int32_t Next(ZipEntry *data, ZipString *name);


        /*
         * Uncompress and write an entry to an open file identified by |fd|.
         * |entry->uncompressed_length| bytes will be written to the file at
         * its current offset, and the file will be truncated at the end of
         * the uncompressed data (no truncation if |fd| references a block
         * device).
         *
         * Returns 0 on success and negative values on failure.
         */
        int32_t ExtractEntryToFile(ZipEntry *entry, int fd);

        const char *ErrorCodeString(int32_t error_code);

    private:
        bool IsValidEntryName(const uint8_t *entry_name, const size_t length) {
            for (size_t i = 0; i < length; ++i) {
                const uint8_t byte = entry_name[i];
                if (byte == 0) {
                    return false;
                } else if ((byte & 0x80) == 0) {
                    // Single byte sequence.
                    continue;
                } else if ((byte & 0xc0) == 0x80 || (byte & 0xfe) == 0xfe) {
                    // Invalid sequence.
                    return false;
                } else {
                    // 2-5 byte sequences.
                    for (uint8_t first = byte << 1; first & 0x80; first <<= 1) {
                        ++i;

                        // Missing continuation byte..
                        if (i == length) {
                            return false;
                        }

                        // Invalid continuation byte.
                        const uint8_t continuation_byte = entry_name[i];
                        if ((continuation_byte & 0xc0) != 0x80) {
                            return false;
                        }
                    }
                }
            }

            return true;
        }

        uint32_t RoundUpPower2(uint32_t val);

        uint32_t ComputeHash(const ZipString &name);

        int32_t AddToHash(const ZipString &name);

        int32_t MapCentralDirectory0(off64_t file_length, off64_t read_amount,
                                     uint8_t *scan_buffer);

        int32_t MapCentralDirectory();

        int32_t ParseZipArchive();

        int32_t OpenArchiveInternal();

        int32_t ValidateDataDescriptor(ZipEntry *entry);

        int32_t FindEntry(const int ent, ZipEntry *data);

        int32_t ExtractToWriter(ZipEntry *entry, Writer *writer);

        int32_t
        CopyEntryToWriter(const ZipEntry *entry, Writer *writer,
                          uint64_t *crc_out);

        int32_t InflateEntryToWriter(const ZipEntry *entry,
                                     Writer *writer, uint64_t *crc_out);

    public:
        mutable std::unique_ptr<hms::MappedZipFile> mapped_zip;
        std::unique_ptr<IterationHandle> mCookie;
        const bool close_file;

        // mapped central directory area
        off64_t directory_offset;
        CentralDirectory central_directory;
        std::unique_ptr<hms::FileMap> directory_map;

        // number of entries in the Zip archive
        uint16_t num_entries;

        // We know how many entries are in the Zip archive, so we can have a
        // fixed-size hash table. We define a load factor of 0.75 and over
        // allocate so the maximum number entries can never be higher than
        // ((4 * UINT16_MAX) / 3 + 1) which can safely fit into a uint32_t.
        uint32_t hash_table_size;
        ZipString *hash_table;

        ZipFile(const char *archive_name)
                : mArchiveName(archive_name),
                  close_file(true),
                  directory_offset(0),
                  central_directory(),
                  directory_map(new hms::FileMap()),
                  num_entries(0),
                  hash_table_size(0),
                  hash_table(nullptr) {
        }

        virtual ~ZipFile() {
            if (close_file && mapped_zip->GetFileDescriptor() >= 0) {
                close(mapped_zip->GetFileDescriptor());
            }

            free(hash_table);
        }

        bool InitializeCentralDirectory(off64_t cd_start_offset,
                                        size_t cd_size);

    private:
        static const uint32_t kMaxEOCDSearch = kMaxCommentLen + sizeof(EocdRecord);
        static const bool kCrcChecksEnabled = false;
        const char *mArchiveName;
    };
}

