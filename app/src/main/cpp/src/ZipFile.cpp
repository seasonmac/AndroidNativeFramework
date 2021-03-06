//
// Created by season on 2021/6/27.
//

#include <cassert>
#include <cerrno>
#include <fcntl.h>
#include <cinttypes>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <unistd.h>

#include <memory>
#include <vector>

#include <File.h>
#include <FileMap.h>
#include "zlib.h"
#include <ZipFileCommon.h>


#include <sys/system_properties.h>

#include <Macros.h>
#include <ZipFile.h>
#include <IterationHandle.h>
#include <FileWriter.h>
#include <HLog.h>
#define LOG_TAG "ZipFile"

namespace hms {
    /*
 * Round up to the next highest power of 2.
 *
 * Found on http://graphics.stanford.edu/~seander/bithacks.html.
 */
    uint32_t ZipFile::RoundUpPower2(uint32_t val) {
        val--;
        val |= val >> 1;
        val |= val >> 2;
        val |= val >> 4;
        val |= val >> 8;
        val |= val >> 16;
        val++;

        return val;
    }

    uint32_t ZipFile::ComputeHash(const ZipString &name) {
        uint32_t hash = 0;
        uint16_t len = name.name_length;
        const uint8_t *str = name.name;

        while (len--) {
            hash = hash * 31 + *str++;
        }

        return hash;
    }

    int32_t ZipFile::AddToHash(const ZipString &name) {
        const uint64_t hash = ComputeHash(name);
        uint32_t ent = hash & (hash_table_size - 1);

        /*
         * We over-allocated the table, so we're guaranteed to find an empty slot.
         * Further, we guarantee that the hashtable size is not 0.
         */
        while (hash_table[ent].name != NULL) {
            if (hash_table[ent] == name) {
                // We've found a duplicate entry. We don't accept it
                HLOGW("Zip: Found duplicate entry %.*s", name.name_length, name.name);
                return kDuplicateEntry;
            }
            ent = (ent + 1) & (hash_table_size - 1);
        }

        hash_table[ent].name = name.name;
        hash_table[ent].name_length = name.name_length;
        return 0;
    }

    int32_t ZipFile::MapCentralDirectory0(off64_t file_length, off64_t read_amount,
                                          uint8_t *scan_buffer) {
        const off64_t search_start = file_length - read_amount;

        if (!mapped_zip->ReadAtOffset(scan_buffer, read_amount, search_start)) {
            HLOGE("Zip: read %"
                          PRId64
                          " from offset %"
                          PRId64
                          " failed", static_cast<int64_t>(read_amount),
                  static_cast<int64_t>(search_start));
            return kIoError;
        }

        /*
         * Scan backward for the EOCD magic.  In an archive without a trailing
         * comment, we'll find it on the first try.  (We may want to consider
         * doing an initial minimal read; if we don't find it, retry with a
         * second read as above.)
         */
        int i = read_amount - sizeof(EocdRecord);
        for (; i >= 0; i--) {
            if (scan_buffer[i] == 0x50) {
                uint32_t *sig_addr = reinterpret_cast<uint32_t *>(&scan_buffer[i]);
                if (hms::get_unaligned<uint32_t>(sig_addr) == EocdRecord::kSignature) {
                    HLOGV("+++ Found EOCD at buf+%d", i);
                    break;
                }
            }
        }
        if (i < 0) {
            HLOGD("Zip: EOCD not found, %s is not zip", mArchiveName);
            return kInvalidFile;
        }

        const off64_t eocd_offset = search_start + i;
        const EocdRecord *eocd = reinterpret_cast<const EocdRecord *>(scan_buffer + i);
        /*
         * Verify that there's no trailing space at the end of the central directory
         * and its comment.
         */
        const off64_t calculated_length = eocd_offset + sizeof(EocdRecord) + eocd->comment_length;
        if (calculated_length != file_length) {
            HLOGW("Zip: %"
                          PRId64
                          " extraneous bytes at the end of the central directory",
                  static_cast<int64_t>(file_length - calculated_length));
            return kInvalidFile;
        }

        /*
         * Grab the CD offset and size, and the number of entries in the
         * archive and verify that they look reasonable.
         */
        if (static_cast<off64_t>(eocd->cd_start_offset) + eocd->cd_size > eocd_offset) {
            HLOGW("Zip: bad offsets (dir %"
                          PRIu32
                          ", size %"
                          PRIu32
                          ", eocd %"
                          PRId64
                          ")",
                  eocd->cd_start_offset, eocd->cd_size, static_cast<int64_t>(eocd_offset));
            if (eocd->cd_start_offset + eocd->cd_size <= eocd_offset) {
                HLOGE("31251826");
            }
            return kInvalidOffset;
        }
        if (eocd->num_records == 0) {
            HLOGW("Zip: empty archive?");
            return kEmptyArchive;
        }

        HLOGV("+++ num_entries=%"
                      PRIu32
                      " dir_size=%"
                      PRIu32
                      " dir_offset=%"
                      PRIu32, eocd->num_records,
              eocd->cd_size, eocd->cd_start_offset);

        /*
         * It all looks good.  Create a mapping for the CD, and set the fields
         * in archive.
         */

        if (!InitializeCentralDirectory(static_cast<off64_t>(eocd->cd_start_offset),
                                        static_cast<size_t>(eocd->cd_size))) {
            HLOGE("Zip: failed to intialize central directory.\n");
            return kMmapFailed;
        }

        num_entries = eocd->num_records;
        directory_offset = eocd->cd_start_offset;

        return 0;
    }

    int32_t ZipFile::MapCentralDirectory() {
        // Test file length. We use lseek64 to make sure the file
        // is small enough to be a zip file (Its size must be less than
        // 0xffffffff bytes).
        off64_t file_length = mapped_zip->GetFileLength();
        if (file_length == -1) {
            return kInvalidFile;
        }

        if (file_length > static_cast<off64_t>(0xffffffff)) {
            HLOGV("Zip: zip file too long %"
                          PRId64, static_cast<int64_t>(file_length));
            return kInvalidFile;
        }

        if (file_length < static_cast<off64_t>(sizeof(EocdRecord))) {
            HLOGV("Zip: length %"
                          PRId64
                          " is too small to be zip", static_cast<int64_t>(file_length));
            return kInvalidFile;
        }

        /*
         * Perform the traditional EOCD snipe hunt.
         *
         * We're searching for the End of Central Directory magic number,
         * which appears at the start of the EOCD block.  It's followed by
         * 18 bytes of EOCD stuff and up to 64KB of archive comment.  We
         * need to read the last part of the file into a buffer, dig through
         * it to find the magic number, parse some values out, and use those
         * to determine the extent of the CD.
         *
         * We start by pulling in the last part of the file.
         */
        off64_t read_amount = kMaxEOCDSearch;
        if (file_length < read_amount) {
            read_amount = file_length;
        }

        std::vector<uint8_t> scan_buffer(read_amount);
        int32_t result =
                MapCentralDirectory0(file_length, read_amount,
                                     scan_buffer.data());
        return result;
    }

    int32_t ZipFile::ParseZipArchive() {
        const uint8_t *const cd_ptr = central_directory.GetBasePtr();
        const size_t cd_length = central_directory.GetMapLength();
//        const uint16_t num_entries = num_entries;

        /*
         * Create hash table.  We have a minimum 75% load factor, possibly as
         * low as 50% after we round off to a power of 2.  There must be at
         * least one unused entry to avoid an infinite loop during creation.
         */
        hash_table_size = RoundUpPower2(1 + (num_entries * 4) / 3);
        hash_table =
                reinterpret_cast<ZipString *>(calloc(hash_table_size, sizeof(ZipString)));
        if (hash_table == nullptr) {
            HLOGW("Zip: unable to allocate the %u-entry hash_table, entry size: %zu",
                  hash_table_size, sizeof(ZipString));
            return -1;
        }

        /*
         * Walk through the central directory, adding entries to the hash
         * table and verifying values.
         */
        const uint8_t *const cd_end = cd_ptr + cd_length;
        const uint8_t *ptr = cd_ptr;
        for (uint16_t i = 0; i < num_entries; i++) {
            if (ptr > cd_end - sizeof(CentralDirectoryRecord)) {
                HLOGW("Zip: ran off the end (at %"
                              PRIu16
                              ")", i);
                return -1;
            }

            const CentralDirectoryRecord *cdr = reinterpret_cast<const CentralDirectoryRecord *>(ptr);
            if (cdr->record_signature != CentralDirectoryRecord::kSignature) {
                HLOGW("Zip: missed a central dir sig (at %"
                              PRIu16
                              ")", i);
                return -1;
            }

            const off64_t local_header_offset = cdr->local_file_header_offset;
            if (local_header_offset >= directory_offset) {
                HLOGW("Zip: bad LFH offset %"
                              PRId64
                              " at entry %"
                              PRIu16,
                      static_cast<int64_t>(local_header_offset), i);
                return -1;
            }

            const uint16_t file_name_length = cdr->file_name_length;
            const uint16_t extra_length = cdr->extra_field_length;
            const uint16_t comment_length = cdr->comment_length;
            const uint8_t *file_name = ptr + sizeof(CentralDirectoryRecord);

            if (file_name + file_name_length > cd_end) {
                HLOGW(
                        "Zip: file name boundary exceeds the central directory range, file_name_length: "
                        "%"
                        PRIx16
                        ", cd_length: %zu",
                        file_name_length, cd_length);
                return -1;
            }
            /* check that file name is valid UTF-8 and doesn't contain NUL (U+0000) characters */
            if (!IsValidEntryName(file_name, file_name_length)) {
                return -1;
            }

            /* add the CDE filename to the hash table */
            ZipString entry_name;
            entry_name.name = file_name;
            entry_name.name_length = file_name_length;
            const int add_result = AddToHash(entry_name);
            if (add_result != 0) {
                HLOGW("Zip: Error adding entry to hash table %d", add_result);
                return add_result;
            }

            ptr += sizeof(CentralDirectoryRecord) + file_name_length + extra_length +
                   comment_length;
            if ((ptr - cd_ptr) > static_cast<int64_t>(cd_length)) {
                HLOGW("Zip: bad CD advance (%tu vs %zu) at entry %"
                              PRIu16, ptr - cd_ptr, cd_length, i);
                return -1;
            }
        }

        uint32_t lfh_start_bytes;
        if (!mapped_zip->ReadAtOffset(reinterpret_cast<uint8_t *>(&lfh_start_bytes),
                                              sizeof(uint32_t), 0)) {
            HLOGW("Zip: Unable to read header for entry at offset == 0.");
            return -1;
        }

        if (lfh_start_bytes != LocalFileHeader::kSignature) {
            HLOGW("Zip: Entry at offset zero has invalid LFH signature %"
                          PRIx32, lfh_start_bytes);
            return -1;
        }

        HLOGV("+++ zip good scan %"
                      PRIu16
                      " entries", num_entries);

        return 0;
    }

    int32_t ZipFile::OpenArchiveInternal() {
        int32_t result = -1;
        if ((result = MapCentralDirectory()) != 0) {
            return result;
        }

        if ((result = ParseZipArchive())) {
            return result;
        }

        return 0;
    }

    int32_t ZipFile::OpenArchive() {
        HLOGENTRY();
        const int fd = open(mArchiveName, O_RDONLY | O_BINARY, 0);
        if (fd < 0) {
            HLOGW("Unable to open '%s': %s", mArchiveName, strerror(errno));
            return kIoError;
        }
        mapped_zip = std::unique_ptr<hms::MappedZipFile>(new MappedZipFile(fd));
        return OpenArchiveInternal();
    }

    int32_t ZipFile::ValidateDataDescriptor(ZipEntry *entry) {
        uint8_t ddBuf[sizeof(DataDescriptor) + sizeof(DataDescriptor::kOptSignature)];
        if (!mapped_zip->ReadData(ddBuf, sizeof(ddBuf))) {
            return kIoError;
        }

        const uint32_t ddSignature = *(reinterpret_cast<const uint32_t *>(ddBuf));
        const uint16_t offset = (ddSignature == DataDescriptor::kOptSignature) ? 4 : 0;
        const DataDescriptor *descriptor = reinterpret_cast<const DataDescriptor *>(ddBuf + offset);

        // Validate that the values in the data descriptor match those in the central
        // directory.
        if (entry->compressed_length != descriptor->compressed_size ||
            entry->uncompressed_length != descriptor->uncompressed_size ||
            entry->crc32 != descriptor->crc32) {
            HLOGW("Zip: size/crc32 mismatch. expected {%"
                          PRIu32
                          ", %"
                          PRIu32
                          ", %"
                          PRIx32
                          "}, was {%"
                          PRIu32
                          ", %"
                          PRIu32
                          ", %"
                          PRIx32
                          "}",
                  entry->compressed_length, entry->uncompressed_length, entry->crc32,
                  descriptor->compressed_size, descriptor->uncompressed_size, descriptor->crc32);
            return kInconsistentInformation;
        }

        return 0;
    }

    int32_t
    ZipFile::FindEntry(const int ent, ZipEntry *data) {
        const uint16_t nameLen = hash_table[ent].name_length;

        // Recover the start of the central directory entry from the filename
        // pointer.  The filename is the first entry past the fixed-size data,
        // so we can just subtract back from that.
        const uint8_t *ptr = hash_table[ent].name;
        ptr -= sizeof(CentralDirectoryRecord);

        // This is the base of our mmapped region, we have to sanity check that
        // the name that's in the hash table is a pointer to a location within
        // this mapped region.
        const uint8_t *base_ptr = central_directory.GetBasePtr();
        if (ptr < base_ptr || ptr > base_ptr + central_directory.GetMapLength()) {
            HLOGW("Zip: Invalid entry pointer");
            return kInvalidOffset;
        }

        const CentralDirectoryRecord *cdr = reinterpret_cast<const CentralDirectoryRecord *>(ptr);

        // The offset of the start of the central directory in the zipfile.
        // We keep this lying around so that we can sanity check all our lengths
        // and our per-file structures.
        const off64_t cd_offset = directory_offset;

        // Fill out the compression method, modification time, crc32
        // and other interesting attributes from the central directory. These
        // will later be compared against values from the local file header.
        data->method = cdr->compression_method;
        data->mod_time = cdr->last_mod_date << 16 | cdr->last_mod_time;
        data->crc32 = cdr->crc32;
        data->compressed_length = cdr->compressed_size;
        data->uncompressed_length = cdr->uncompressed_size;

        // Figure out the local header offset from the central directory. The
        // actual file data will begin after the local header and the name /
        // extra comments.
        const off64_t local_header_offset = cdr->local_file_header_offset;
        if (local_header_offset + static_cast<off64_t>(sizeof(LocalFileHeader)) >= directory_offset) {
            HLOGW("Zip: bad local hdr offset in zip");
            return kInvalidOffset;
        }

        uint8_t lfh_buf[sizeof(LocalFileHeader)];
        if (!mapped_zip->ReadAtOffset(lfh_buf, sizeof(lfh_buf), local_header_offset)) {
            HLOGW("Zip: failed reading lfh name from offset %"
                          PRId64,
                  static_cast<int64_t>(local_header_offset));
            return kIoError;
        }

        const LocalFileHeader *lfh = reinterpret_cast<const LocalFileHeader *>(lfh_buf);

        if (lfh->lfh_signature != LocalFileHeader::kSignature) {
            HLOGW("Zip: didn't find signature at start of lfh, offset=%"
                          PRId64,
                  static_cast<int64_t>(local_header_offset));
            return kInvalidOffset;
        }

        // Paranoia: Match the values specified in the local file header
        // to those specified in the central directory.

        // Warn if central directory and local file header don't agree on the use
        // of a trailing Data Descriptor. The reference implementation is inconsistent
        // and appears to use the LFH value during extraction (unzip) but the CD value
        // while displayng information about archives (zipinfo). The spec remains
        // silent on this inconsistency as well.
        //
        // For now, always use the version from the LFH but make sure that the values
        // specified in the central directory match those in the data descriptor.
        //
        // NOTE: It's also worth noting that unzip *does* warn about inconsistencies in
        // bit 11 (EFS: The language encoding flag, marking that filename and comment are
        // encoded using UTF-8). This implementation does not check for the presence of
        // that flag and always enforces that entry names are valid UTF-8.
        if ((lfh->gpb_flags & kGPBDDFlagMask) != (cdr->gpb_flags & kGPBDDFlagMask)) {
            HLOGW("Zip: gpb flag mismatch at bit 3. expected {%04"
                          PRIx16
                          "}, was {%04"
                          PRIx16
                          "}",
                  cdr->gpb_flags, lfh->gpb_flags);
        }

        // If there is no trailing data descriptor, verify that the central directory and local file
        // header agree on the crc, compressed, and uncompressed sizes of the entry.
        if ((lfh->gpb_flags & kGPBDDFlagMask) == 0) {
            data->has_data_descriptor = 0;
            if (data->compressed_length != lfh->compressed_size ||
                data->uncompressed_length != lfh->uncompressed_size || data->crc32 != lfh->crc32) {
                HLOGW("Zip: size/crc32 mismatch. expected {%"
                              PRIu32
                              ", %"
                              PRIu32
                              ", %"
                              PRIx32
                              "}, was {%"
                              PRIu32
                              ", %"
                              PRIu32
                              ", %"
                              PRIx32
                              "}",
                      data->compressed_length, data->uncompressed_length, data->crc32,
                      lfh->compressed_size,
                      lfh->uncompressed_size, lfh->crc32);
                return kInconsistentInformation;
            }
        } else {
            data->has_data_descriptor = 1;
        }

        // 4.4.2.1: the upper byte of `version_made_by` gives the source OS. Unix is 3.
        if ((cdr->version_made_by >> 8) == 3) {
            data->unix_mode = (cdr->external_file_attributes >> 16) & 0xffff;
        } else {
            data->unix_mode = 0777;
        }

        // Check that the local file header name matches the declared
        // name in the central directory.
        if (lfh->file_name_length == nameLen) {
            const off64_t name_offset = local_header_offset + sizeof(LocalFileHeader);
            if (name_offset + lfh->file_name_length > directory_offset) {
                HLOGW("Zip: Invalid declared length");
                return kInvalidOffset;
            }

            std::vector<uint8_t> name_buf(nameLen);
            if (!mapped_zip->ReadAtOffset(name_buf.data(), nameLen, name_offset)) {
                HLOGW("Zip: failed reading lfh name from offset %"
                              PRId64, static_cast<int64_t>(name_offset));
                return kIoError;
            }

            if (memcmp(hash_table[ent].name, name_buf.data(), nameLen)) {
                return kInconsistentInformation;
            }

        } else {
            HLOGW("Zip: lfh name did not match central directory.");
            return kInconsistentInformation;
        }

        const off64_t data_offset = local_header_offset + sizeof(LocalFileHeader) +
                                    lfh->file_name_length + lfh->extra_field_length;
        if (data_offset > directory_offset) {
            HLOGW("Zip: bad data offset %"
                          PRId64
                          " in zip", static_cast<int64_t>(data_offset));
            return kInvalidOffset;
        }

        if (static_cast<off64_t>(data_offset + data->compressed_length) > directory_offset) {
            HLOGW("Zip: bad compressed length in zip (%"
                          PRId64
                          " + %"
                          PRIu32
                          " > %"
                          PRId64
                          ")",
                  static_cast<int64_t>(data_offset), data->compressed_length,
                  static_cast<int64_t>(directory_offset));
            return kInvalidOffset;
        }

        if (data->method == kCompressStored &&
            static_cast<off64_t>(data_offset + data->uncompressed_length) > directory_offset) {
            HLOGW("Zip: bad uncompressed length in zip (%"
                          PRId64
                          " + %"
                          PRIu32
                          " > %"
                          PRId64
                          ")",
                  static_cast<int64_t>(data_offset), data->uncompressed_length,
                  static_cast<int64_t>(directory_offset));
            return kInvalidOffset;
        }

        data->offset = data_offset;
        return 0;
    }


    int32_t ZipFile::StartIteration(const ZipString *optional_prefix,
                                    const ZipString *optional_suffix) {
        HLOGENTRY();
        if (hash_table == nullptr) {
            HLOGW("Zip: Invalid ZipFileHandle");
            return kInvalidHandle;
        }

        mCookie = std::unique_ptr<IterationHandle>(new IterationHandle(optional_prefix, optional_suffix));
        mCookie->position = 0;
//        mCookie->archive = this;
        return 0;
    }


    int32_t ZipFile::Next(ZipEntry *data, ZipString *name) {
        if (mCookie == NULL) {
            return kInvalidHandle;
        }

        if (hash_table == NULL) {
            HLOGW("Zip: Invalid ZipFileHandle");
            return kInvalidHandle;
        }

        const uint32_t currentOffset = mCookie->position;

        for (uint32_t i = currentOffset; i < hash_table_size; ++i) {
            if (hash_table[i].name != NULL &&
                (mCookie->prefix.name_length == 0 || hash_table[i].StartsWith(mCookie->prefix)) &&
                (mCookie->suffix.name_length == 0 || hash_table[i].EndsWith(mCookie->suffix))) {
                mCookie->position = (i + 1);
                const int error = FindEntry(i, data);
                if (!error) {
                    name->name = hash_table[i].name;
                    name->name_length = hash_table[i].name_length;
                }

                return error;
            }
        }

        mCookie->position = 0;
        return kIterationEnd;
    }

    // This method is using libz macros with old-style-casts
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"

    static inline int zlib_inflateInit2(z_stream *stream, int window_bits) {
        return inflateInit2(stream, window_bits);
    }

#pragma GCC diagnostic pop

    int32_t ZipFile::InflateEntryToWriter(const ZipEntry *entry,
                                          Writer *writer, uint64_t *crc_out) {
        HLOGENTRY();
        const size_t kBufSize = 32768;
        std::vector<uint8_t> read_buf(kBufSize);
        std::vector<uint8_t> write_buf(kBufSize);
        z_stream zstream;
        int zerr;

        /*
         * Initialize the zlib stream struct.
         */
        memset(&zstream, 0, sizeof(zstream));
        zstream.zalloc = Z_NULL;
        zstream.zfree = Z_NULL;
        zstream.opaque = Z_NULL;
        zstream.next_in = NULL;
        zstream.avail_in = 0;
        zstream.next_out = &write_buf[0];
        zstream.avail_out = kBufSize;
        zstream.data_type = Z_UNKNOWN;

        /*
         * Use the undocumented "negative window bits" feature to tell zlib
         * that there's no zlib header waiting for it.
         */
        zerr = zlib_inflateInit2(&zstream, -MAX_WBITS);
        if (zerr != Z_OK) {
            if (zerr == Z_VERSION_ERROR) {
                HLOGE("Installed zlib is not compatible with linked version (%s)", ZLIB_VERSION);
            } else {
                HLOGW("Call to inflateInit2 failed (zerr=%d)", zerr);
            }

            return kZlibError;
        }

        auto zstream_deleter = [](z_stream *stream) {
            inflateEnd(stream); /* free up any allocated structures */
        };

        std::unique_ptr<z_stream, decltype(zstream_deleter)> zstream_guard(&zstream,
                                                                           zstream_deleter);

        const uint32_t uncompressed_length = entry->uncompressed_length;

        uint64_t crc = 0;
        uint32_t compressed_length = entry->compressed_length;
        do {
            /* read as much as we can */
            if (zstream.avail_in == 0) {
                const size_t getSize = (compressed_length > kBufSize) ? kBufSize
                                                                      : compressed_length;
                if (!mapped_zip->ReadData(read_buf.data(), getSize)) {
                    HLOGW("Zip: inflate read failed, getSize = %zu: %s", getSize, strerror(errno));
                    return kIoError;
                }

                compressed_length -= getSize;

                zstream.next_in = &read_buf[0];
                zstream.avail_in = getSize;
            }

            /* uncompress the data */
            zerr = inflate(&zstream, Z_NO_FLUSH);
            if (zerr != Z_OK && zerr != Z_STREAM_END) {
                HLOGW("Zip: inflate zerr=%d (nIn=%p aIn=%u nOut=%p aOut=%u)", zerr, zstream.next_in,
                      zstream.avail_in, zstream.next_out, zstream.avail_out);
                return kZlibError;
            }

            /* write when we're full or when we're done */
            if (zstream.avail_out == 0 || (zerr == Z_STREAM_END && zstream.avail_out != kBufSize)) {
                const size_t write_size = zstream.next_out - &write_buf[0];
                if (!writer->Append(&write_buf[0], write_size)) {
                    // The file might have declared a bogus length.
                    return kInconsistentInformation;
                } else {
                    crc = crc32(crc, &write_buf[0], write_size);
                }

                zstream.next_out = &write_buf[0];
                zstream.avail_out = kBufSize;
            }
        } while (zerr == Z_OK);

        assert(zerr == Z_STREAM_END); /* other errors should've been caught */

        // NOTE: zstream.adler is always set to 0, because we're using the -MAX_WBITS
        // "feature" of zlib to tell it there won't be a zlib file header. zlib
        // doesn't bother calculating the checksum in that scenario. We just do
        // it ourselves above because there are no additional gains to be made by
        // having zlib calculate it for us, since they do it by calling crc32 in
        // the same manner that we have above.
        *crc_out = crc;

        if (zstream.total_out != uncompressed_length || compressed_length != 0) {
            HLOGW("Zip: size mismatch on inflated file (%lu vs %"
                          PRIu32
                          ")", zstream.total_out,
                  uncompressed_length);
            return kInconsistentInformation;
        }

        return 0;
    }

    int32_t ZipFile::CopyEntryToWriter(const ZipEntry *entry,
                                       Writer *writer,
                                       uint64_t *crc_out) {
        HLOGENTRY();
        static const uint32_t kBufSize = 32768;
        std::vector<uint8_t> buf(kBufSize);

        const uint32_t length = entry->uncompressed_length;
        uint32_t count = 0;
        uint64_t crc = 0;
        while (count < length) {
            uint32_t remaining = length - count;

            // Safe conversion because kBufSize is narrow enough for a 32 bit signed
            // value.
            const size_t block_size = (remaining > kBufSize) ? kBufSize : remaining;
            if (!mapped_zip->ReadData(buf.data(), block_size)) {
                HLOGW("CopyFileToFile: copy read failed, block_size = %zu: %s", block_size,
                      strerror(errno));
                return kIoError;
            }

            if (!writer->Append(&buf[0], block_size)) {
                return kIoError;
            }
            crc = crc32(crc, &buf[0], block_size);
            count += block_size;
        }

        *crc_out = crc;

        return 0;
    }

    int32_t ZipFile::ExtractToWriter(ZipEntry *entry, Writer *writer) {
        HLOGENTRY();
        const uint16_t method = entry->method;
        off64_t data_offset = entry->offset;

        if (!mapped_zip->SeekToOffset(data_offset)) {
            HLOGW("Zip: lseek to data at %"
                          PRId64
                          " failed", static_cast<int64_t>(data_offset));
            return kIoError;
        }

        // this should default to kUnknownCompressionMethod.
        int32_t return_value = -1;
        uint64_t crc = 0;
        if (method == kCompressStored) {
            return_value = CopyEntryToWriter(entry, writer, &crc);
        } else if (method == kCompressDeflated) {
            return_value = InflateEntryToWriter(entry, writer, &crc);
        }

        if (!return_value && entry->has_data_descriptor) {
            return_value = ValidateDataDescriptor(entry);
            if (return_value) {
                return return_value;
            }
        }

        // Validate that the CRC matches the calculated value.
        if (kCrcChecksEnabled && (entry->crc32 != static_cast<uint32_t>(crc))) {
            HLOGW("Zip: crc mismatch: expected %"
                          PRIu32
                          ", was %"
                          PRIu64, entry->crc32, crc);
            return kInconsistentInformation;
        }

        return return_value;
    }

    int32_t ZipFile::ExtractEntryToFile(ZipEntry *entry, int fd) {
        HLOGENTRY();
        std::unique_ptr<Writer> writer(FileWriter::Create(fd, entry));
        if (writer.get() == nullptr) {
            return kIoError;
        }

        return ExtractToWriter(entry, writer.get());
    }

    const char *ZipFile::ErrorCodeString(int32_t error_code) {
        // Make sure that the number of entries in kErrorMessages and ErrorCodes
        // match.
        static_assert((-kLastErrorCode + 1) == arraysize(kErrorMessages),
                      "(-kLastErrorCode + 1) != arraysize(kErrorMessages)");

        const uint32_t idx = -error_code;
        if (idx < arraysize(kErrorMessages)) {
            return kErrorMessages[idx];
        }

        return "Unknown return code";
    }

    bool ZipFile::InitializeCentralDirectory(off64_t cd_start_offset,
                                             size_t cd_size) {
        if (mapped_zip->HasFd()) {
            if (!directory_map->create(mArchiveName, mapped_zip->GetFileDescriptor(),
                                       cd_start_offset,
                                       cd_size, true /* read only */)) {
                return false;
            }

            //        CHECK_EQ(directory_map->getDataLength(), cd_size);
            central_directory.Initialize(directory_map->getDataPtr(), 0 /*offset*/, cd_size);
        } else {
            if (mapped_zip->GetBasePtr() == nullptr) {
                HLOGE("Zip: Failed to map central directory, bad mapped_zip base pointer\n");
                return false;
            }
            if (static_cast<off64_t>(cd_start_offset) + static_cast<off64_t>(cd_size) >
                mapped_zip->GetFileLength()) {
                HLOGE(
                        "Zip: Failed to map central directory, offset exceeds mapped memory region ("
                        "start_offset %"
                        PRId64
                        ", cd_size %zu, mapped_region_size %"
                        PRId64
                        ")",
                        static_cast<int64_t>(cd_start_offset), cd_size,
                        mapped_zip->GetFileLength());
                return false;
            }

            central_directory.Initialize(mapped_zip->GetBasePtr(), cd_start_offset, cd_size);
        }
        return true;
    }
}
