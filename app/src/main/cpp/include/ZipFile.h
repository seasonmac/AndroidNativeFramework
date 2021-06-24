//
// Created by season on 2021/6/24.
//

#pragma once

#include "Micros.h"
#include <vector>
#include <stdio.h>
#include <string>

namespace hms {
    class ZipFile {
    public:
        ZipFile(std::string& archive_name);
        virtual ~ZipFile(void) {}

        int unCompress(const char *extractFileName);
    private:
        DISALLOW_COPY_AND_ASSIGN(ZipFile);
        std::string mZip;
    };


}