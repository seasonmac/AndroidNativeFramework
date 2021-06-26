#pragma once

#include <sstream>
#include <string>
#include <vector>

namespace hms {
    class StringUtils {
    public:
        static bool StartsWith(const std::string &s, const char *prefix);
        static bool EndsWith(const std::string& s, const char* suffix, bool case_sensitive);
        static bool EndsWith(const std::string &s, const char *suffix);
    };
}
