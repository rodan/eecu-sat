/*
 * This file is part of the eecu-sat project.
 *
 * Copyright (C) 2024 Petre Rodan <2b4eda@subdimension.ro>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "saleae.h"

struct __attribute__((packed)) hdr_unk {
    uint8_t identifier[8];
    int32_t version;
    int32_t type;
};

uint8_t saleae_get_hdr_type(uint8_t *data)
{
    // 00000000  3c 53 41 4c 45 41 45 3e  00 00 00 00 01 00 00 00  |<SALEAE>........|
    const char saleae_magic[8] = {0x3c, 0x53, 0x41, 0x4c, 0x45, 0x41, 0x45, 0x3e};
    struct hdr_unk *hdr;

    if (memcmp(data, saleae_magic, 8) != 0)
        return SALEAE_UNKNOWN;

    hdr = (struct hdr_unk *) data;
    if (hdr->version != 0)
        return SALEAE_UNKNOWN_VER;

    if (hdr->type == SALEAE_TYPE_DIGITAL) {
        return SALEAE_DIGITAL;
    } else if (hdr->type == SALEAE_TYPE_ANALOG) {
        return SALEAE_ANALOG;
    }

    return SALEAE_UNKNOWN;
}

