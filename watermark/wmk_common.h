/**
 * Copyright (C) 2016 Fuzhou Rockchip Electronics Co., Ltd
 * author: Tianfeng Chen <ctf@rock-chips.com>
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

#ifndef _WATER_MARK_COMMON_H
#define _WATER_MARK_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifndef BOOL
typedef int BOOL;
#define TRUE  1
#define FALSE 0
#endif

/* MAX/MIN/ABS macors */
#ifndef MAX
#define MAX(x, y)           (((x) > (y))?(x):(y))
#endif

#ifndef MIN
#define MIN(x, y)           (((x) < (y))?(x):(y))
#endif

#ifndef ABS
#define ABS(x)              (((x)<0) ? -(x) : (x))
#endif

#define WMK_HAS_64BIT_TYPE    long long
typedef uint32_t          pixel_t;

#define get_r_value(rgba)      ((unsigned char)(rgba))
#define get_g_value(rgba)      ((unsigned char)(((unsigned long)(rgba)) >> 8))
#define get_b_value(rgba)      ((unsigned char)((unsigned long)(rgba) >> 16))
#define get_a_value(rgba)      ((unsigned char)((unsigned long)(rgba) >> 24))

#define WMK_LIL_ENDIAN  1234
#define WMK_BIG_ENDIAN  4321
#define WMK_BYTEORDER   WMK_LIL_ENDIAN

#define BYTES_PER_PIXEL 2
#define BITS_PER_PIXEL 16
#endif
