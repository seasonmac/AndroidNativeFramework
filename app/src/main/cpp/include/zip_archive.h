/*
 * Copyright (C) 2013 The Android Open Source Project
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

/*
 * Read-only access to Zip archives, with minimal heap allocation.
 */
#ifndef LIBZIPARCHIVE_ZIPARCHIVE_H_
#define LIBZIPARCHIVE_ZIPARCHIVE_H_

#include <stdint.h>
#include <string.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <ZipEntry.h>
#include <ZipString.h>
/* Zip compression methods we support */
enum {
  kCompressStored = 0,    // no compression
  kCompressDeflated = 8,  // standard deflate
};

typedef void* ZipArchiveHandle;

/*
 * Open a Zip archive, and sets handle to the value of the opaque
 * handle for the file. This handle must be released by calling
 * CloseArchive with this handle.
 *
 * Returns 0 on success, and negative values on failure.
 */
int32_t OpenArchive(const char* fileName, ZipArchiveHandle* handle);

/*
 * Close archive, releasing resources associated with it. This will
 * unmap the central directory of the zipfile and free all internal
 * data structures associated with the file. It is an error to use
 * this handle for any further operations without an intervening
 * call to one of the OpenArchive variants.
 */
void CloseArchive(ZipArchiveHandle handle);

/*
 * Find an entry in the Zip archive, by name. |entryName| must be a null
 * terminated string, and |data| must point to a writeable memory location.
 *
 * Returns 0 if an entry is found, and populates |data| with information
 * about this entry. Returns negative values otherwise.
 *
 * It's important to note that |data->crc32|, |data->compLen| and
 * |data->uncompLen| might be set to values from the central directory
 * if this file entry contains a data descriptor footer. To verify crc32s
 * and length, a call to VerifyCrcAndLengths must be made after entry data
 * has been processed.
 *
 * On non-Windows platforms this method does not modify internal state and
 * can be called concurrently.
 */
int32_t FindEntry(const ZipArchiveHandle handle, const ZipString& entryName, ZipEntry* data);

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
int32_t StartIteration(ZipArchiveHandle handle, void** cookie_ptr, const ZipString* optional_prefix,
                       const ZipString* optional_suffix);

/*
 * Advance to the next element in the zipfile in iteration order.
 *
 * Returns 0 on success, -1 if there are no more elements in this
 * archive and lower negative values on failure.
 */
int32_t Next(void* cookie, ZipEntry* data, ZipString* name);

/*
 * End iteration over all entries of a zip file and frees the memory allocated
 * in StartIteration.
 */
void EndIteration(void* cookie);

/*
 * Uncompress and write an entry to an open file identified by |fd|.
 * |entry->uncompressed_length| bytes will be written to the file at
 * its current offset, and the file will be truncated at the end of
 * the uncompressed data (no truncation if |fd| references a block
 * device).
 *
 * Returns 0 on success and negative values on failure.
 */
int32_t ExtractEntryToFile(ZipArchiveHandle handle, ZipEntry* entry, int fd);

const char* ErrorCodeString(int32_t error_code);



#endif  // LIBZIPARCHIVE_ZIPARCHIVE_H_
