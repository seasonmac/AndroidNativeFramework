//
// Created by season on 2021/6/27.
//

#pragma once

#include "ZipString.h"
#include "ZipFile.h"

namespace hms {
    class IterationHandle {
    public:
        uint32_t position;
        // We're not using vector here because this code is used in the Windows SDK
        // where the STL is not available.
        ZipString prefix;
        ZipString suffix;
        ZipFile *archive;

        IterationHandle(const ZipString *in_prefix, const ZipString *in_suffix) {
            if (in_prefix) {
                uint8_t *name_copy = new uint8_t[in_prefix->name_length];
                memcpy(name_copy, in_prefix->name, in_prefix->name_length);
                prefix.name = name_copy;
                prefix.name_length = in_prefix->name_length;
            } else {
                prefix.name = nullptr;
                prefix.name_length = 0;
            }
            if (in_suffix) {
                uint8_t *name_copy = new uint8_t[in_suffix->name_length];
                memcpy(name_copy, in_suffix->name, in_suffix->name_length);
                suffix.name = name_copy;
                suffix.name_length = in_suffix->name_length;
            } else {
                suffix.name = nullptr;
                suffix.name_length = 0;
            }
        }

        virtual ~IterationHandle() {
            delete[] prefix.name;
            delete[] suffix.name;
        }
    };
}
