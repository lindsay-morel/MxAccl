// Copyright (c) 2025 MemryX
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "color_print.h"

void tag_printf(color_t c, const char* tag, int num, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    std::string tag_with_num = std::string(tag);
    if(num >= 0) {
        tag_with_num += " ";
        tag_with_num += std::to_string(num);
    }

    switch(c) {
        case COLOR_RED:
            printf("[\033[91m%s\033[0m] ", tag_with_num.c_str());
            break;
        case COLOR_GREEN:
            printf("[\033[92m%s\033[0m] ", tag_with_num.c_str());
            break;
        case COLOR_YELLOW:
            printf("[\033[93m%s\033[0m] ", tag_with_num.c_str());
            break;
        case COLOR_BLUE:
            printf("[\033[94m%s\033[0m] ", tag_with_num.c_str());
            break;
        case COLOR_MAGENTA:
            printf("[\033[95m%s\033[0m] ", tag_with_num.c_str());
            break;
        case COLOR_CYAN:
            printf("[\033[96m%s\033[0m] ", tag_with_num.c_str());
            break;
        case COLOR_WHITE:
            printf("[\033[97m%s\033[0m] ", tag_with_num.c_str());
            break;
        default:
            printf("[\033[0m%s\033[0m] ", tag_with_num.c_str());
            break;
    }
    vprintf(fmt, args);
    va_end(args);
}
