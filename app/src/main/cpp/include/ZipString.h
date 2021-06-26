//
// Created by season on 2021/6/27.
//

#pragma once
#include <stdint.h>
struct ZipString {
    const uint8_t* name;
    uint16_t name_length;

    ZipString() {}

    /*
     * entry_name has to be an c-style string with only ASCII characters.
     */
    explicit ZipString(const char* entry_name){
        size_t len = strlen(entry_name);
        //CHECK_LE(len, static_cast<size_t>(UINT16_MAX));
        name_length = static_cast<uint16_t>(len);
    };

    bool operator==(const ZipString& rhs) const {
        return name && (name_length == rhs.name_length) && (memcmp(name, rhs.name, name_length) == 0);
    }

    bool StartsWith(const ZipString& prefix) const {
        return name && (name_length >= prefix.name_length) &&
               (memcmp(name, prefix.name, prefix.name_length) == 0);
    }

    bool EndsWith(const ZipString& suffix) const {
        return name && (name_length >= suffix.name_length) &&
               (memcmp(name + name_length - suffix.name_length, suffix.name, suffix.name_length) == 0);
    }
};