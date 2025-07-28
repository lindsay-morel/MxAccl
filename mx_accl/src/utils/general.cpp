// Copyright (c) 2025 MemryX
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memx/accl/utils/general.h>

void MX::Utils::mx_checkandthrow(MX::Utils::mx_retval ret)
{
    if(!ret.error_flag) {
        throw std::runtime_error(ret.error_msg);
    }
}

void MX::Utils::mx_checkandprint(MX::Utils::mx_retval ret)
{
    if(!ret.error_flag) {
        std::cerr << ret.error_msg << "\n";
    }
}
