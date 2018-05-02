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

#ifndef WATER_MARK_BITMAP_H
#define WATER_MARK_BITMAP_H

#include <stdint.h>
#include "wmk_endianrw.h"
#include "wmk_readbmp.h"

/* rgb565 start */
#define RGB_FORMAT_RLOSS    3
#define RGB_FORMAT_GLOSS    2
#define RGB_FORMAT_BLOSS    3
#define RGB_FORMAT_ALOSS    8

#define RGB_FORMAT_RSHIFT   11
#define RGB_FORMAT_GSHIFT   5
#define RGB_FORMAT_BSHIFT   0
#define RGB_FORMAT_ASHIFT   0

#define RGB_FORMAT_RMASK    0xf800
#define RGB_FORMAT_GMASK    0x07e0
#define RGB_FORMAT_BMASK    0x001f
#define RGB_FORMAT_AMASK    0
/* rgb565 end */

/* Transparency definitions: These define alpha as the opacity of a surface */
#define WMK_ALPHA_OPAQUE        255
#define WMK_ALPHA_TRANSPARENT   0

#define wmk_fp_getc(fp)     rw_getc(fp)
#define wmk_fp_igetw(fp)    read_le16(fp)
#define wmk_fp_igetl(fp)    read_le32(fp)
#define wmk_fp_mgetw(fp)    read_be16(fp)
#define wmk_fp_mgetl(fp)    read_be32(fp)

void *init_bmp(struct mg_rw_ops *fp, struct mybitmap *bmp, struct rgb_t *pal);
void cleanup_bmp(void *init_info);
int load_bmp(struct mg_rw_ops *fp, void *init_info, struct mybitmap *bmp,
	     _cb_one_scanline cb, void *context);
BOOL check_bmp(struct mg_rw_ops *fp);
#endif
