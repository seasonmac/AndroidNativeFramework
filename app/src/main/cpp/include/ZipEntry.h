//
// Created by season on 2021/6/27.
//

#pragma once
#include <stdint.h>
struct ZipEntry {
    // Compression method: One of kCompressStored or
    // kCompressDeflated.
    uint16_t method;

    // Modification time. The zipfile format specifies
    // that the first two little endian bytes contain the time
    // and the last two little endian bytes contain the date.
    uint32_t mod_time;

    // Suggested Unix mode for this entry, from the zip archive if created on
    // Unix, or a default otherwise.
    mode_t unix_mode;

    // 1 if this entry contains a data descriptor segment, 0
    // otherwise.
    uint8_t has_data_descriptor;

    // Crc32 value of this ZipEntry. This information might
    // either be stored in the local file header or in a special
    // Data descriptor footer at the end of the file entry.
    uint32_t crc32;

    // Compressed length of this ZipEntry. Might be present
    // either in the local file header or in the data descriptor
    // footer.
    uint32_t compressed_length;

    // Uncompressed length of this ZipEntry. Might be present
    // either in the local file header or in the data descriptor
    // footer.
    uint32_t uncompressed_length;

    // The offset to the start of data for this ZipEntry.
    off64_t offset;
};