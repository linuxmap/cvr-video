/**
 * Copyright (C) 2016 Fuzhou Rockchip Electronics Co., Ltd
 * author: hogan.wang@rock-chips.com
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

#include "yuv.h"

void NV12_to_YUYV(int width, int height, void* src, void* dst)
{
    int i = 0, j = 0;
    int* src_y = (int*)src;
    int* src_uv = (int*)((char*)src + width * height);
    int* line = (int*)dst;

    for (j = 0; j < height; j++) {
        if (j % 2 != 0)
            src_uv -= width >> 2;
        for (i = 0; i<width >> 2; i++) {
            *line++ = ((*src_y & 0x000000ff)) | ((*src_y & 0x0000ff00) << 8) |
                      ((*src_uv & 0x000000ff) << 8) | ((*src_uv & 0x0000ff00) << 16);
            *line++ = ((*src_y & 0x00ff0000) >> 16) | ((*src_y & 0xff000000) >> 8) |
                      ((*src_uv & 0x00ff0000) >> 8) | ((*src_uv & 0xff000000));
            src_y++;
            src_uv++;
        }
    }
}
