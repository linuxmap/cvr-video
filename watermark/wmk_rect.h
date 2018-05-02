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

#ifndef __WMK_RECT_H__
#define __WMK_RECT_H__

/* watermark rect, must 16-byte alignment */
/* 3840x2160 */
#define WATERMARK4K_IMG_W       288
#define WATERMARK4K_IMG_H       96
#define WATERMARK4K_IMG_X      ((3840 - WATERMARK4K_IMG_W - 64) & 0xfffffff0)
#define WATERMARK4K_IMG_X_180   64

/* 2560x1440 */
#define WATERMARK1440p_IMG_W       192
#define WATERMARK1440p_IMG_H       64
#define WATERMARK1440p_IMG_X      ((2560 - WATERMARK1440p_IMG_W - 32) & 0xfffffff0)
#define WATERMARK1440p_IMG_X_180   32

/* 1920x1080 */
#define WATERMARK1080p_IMG_W       144
#define WATERMARK1080p_IMG_H       48
#define WATERMARK1080p_IMG_X      ((1920 - WATERMARK1080p_IMG_W - 32) & 0xfffffff0)
#define WATERMARK1080p_IMG_X_180   32

/* 1280x720 */
#define WATERMARK720p_IMG_W        96
#define WATERMARK720p_IMG_H        32
#define WATERMARK720p_IMG_X       ((1280 - WATERMARK720p_IMG_W - 32) & 0xfffffff0)
#define WATERMARK720p_IMG_X_180    32

/* 640x480 */
#define WATERMARK480p_IMG_W        72
#define WATERMARK480p_IMG_H        24
#define WATERMARK480p_IMG_X       ((640 - WATERMARK480p_IMG_W - 16) & 0xfffffff0)
#define WATERMARK480p_IMG_X_180    16

/* 480x320 */
#define WATERMARK360p_IMG_W        48
#define WATERMARK360p_IMG_H        16
#define WATERMARK360p_IMG_X       ((480 - WATERMARK360p_IMG_W - 16) & 0xfffffff0)
#define WATERMARK360p_IMG_X_180    16

/* 320x240 */
#define WATERMARK240p_IMG_W        48
#define WATERMARK240p_IMG_H        16
#define WATERMARK240p_IMG_X       ((320 - WATERMARK240p_IMG_W - 16) & 0xfffffff0)
#define WATERMARK240p_IMG_X_180    16

#endif