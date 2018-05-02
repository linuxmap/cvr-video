/**
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "yuyv_process.hpp"

int convert_yuyv(int width,
                 int height,
                 void* srcbuf,
                 void* dstbuf,
                 int neadshownum)
{
    int *dstint_y, *dstint_uv, *srcint;
    int y_size = width * height;
    int i, j;

    dstint_y = (int*)dstbuf;
    srcint = (int*)srcbuf;
    dstint_uv = (int*)((char*)dstbuf + y_size);

    for (i = 0; i < height; i++) {
        for (j = 0; j<width >> 2; j++) {
            if (i % 2 == 0) {
                *dstint_uv++ =
                    (*(srcint + 1) & 0xff000000) | ((*(srcint + 1) & 0x0000ff00) << 8) |
                    ((*srcint & 0xff000000) >> 16) | ((*srcint & 0x0000ff00) >> 8);
            }
            *dstint_y++ = ((*(srcint + 1) & 0x00ff0000) << 8) |
                          ((*(srcint + 1) & 0x000000ff) << 16) |
                          ((*srcint & 0x00ff0000) >> 8) | (*srcint & 0x000000ff);
            srcint += 2;
        }
    }

    return 0;
}

