// Copyright (c) 2025 MemryX
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef COLOR_PRINT_H
#define COLOR_PRINT_H

#pragma once
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstdarg>
#include <cstring>
#include <string>


// adds:
//
// 1. typedef enum of colors
// 2. void color_printf(color, tag string, const char *fmt, ...);

typedef enum : uint8_t {
    COLOR_RESET = 0,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_YELLOW,
    COLOR_BLUE,
    COLOR_MAGENTA,
    COLOR_CYAN,
    COLOR_WHITE
} color_t;

void tag_printf(color_t c, const char* tag, int num, const char* fmt, ...);

#endif // COLOR_PRINT_H
