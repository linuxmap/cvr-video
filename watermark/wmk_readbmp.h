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

#ifndef __WATER_MARK_READ_BMP_H__
#define __WATER_MARK_READ_BMP_H__

#include <stdint.h>
#include "wmk_common.h"

#define WMK_BMP_TYPE_NORMAL         0x00
#define WMK_BMP_TYPE_RLE            0x01
#define WMK_BMP_TYPE_ALPHA          0x02
#define WMK_BMP_TYPE_ALPHACHANNEL   0x04
#define WMK_BMP_TYPE_COLORKEY       0x10

#define WMK_BMP_TYPE_ALPHA_MASK     0x20
#define WMK_BMP_TYPE_PRIV_PIXEL     0x00

#define WMK_MYBMP_TYPE_NORMAL       0x00000000
#define WMK_MYBMP_TYPE_RLE4         0x00000001
#define WMK_MYBMP_TYPE_RLE8         0x00000002
#define WMK_MYBMP_TYPE_RGB          0x00000003
#define WMK_MYBMP_TYPE_BGR          0x00000004
#define WMK_MYBMP_TYPE_RGBA         0x00000005
#define WMK_MYBMP_TYPE_MASK         0x0000000F

#define WMK_MYBMP_FLOW_DOWN         0x00000010
#define WMK_MYBMP_FLOW_UP           0x00000020
#define WMK_MYBMP_FLOW_MASK         0x000000F0

#define WMK_MYBMP_TRANSPARENT       0x00000100
#define WMK_MYBMP_ALPHACHANNEL      0x00000200
#define WMK_MYBMP_ALPHA             0x00000400

#define WMK_MYBMP_RGBSIZE_3         0x00001000
#define WMK_MYBMP_RGBSIZE_4         0x00002000

#define WMK_MYBMP_LOAD_GRAYSCALE    0x00010000
#define WMK_MYBMP_LOAD_ALLOCATE_ONE 0x00020000
#define WMK_MYBMP_LOAD_NONE         0x00000000

struct load_mybitmap_info_t {
	void *init_info;
};

struct rgb_t {
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
};

struct mybitmap {
	/**
	 * Flags of the bitmap, can be OR'ed by the following values:
	 *  - WMK_MYBMP_TYPE_NORMAL\n
	 *    A normal palette bitmap.
	 *  - WMK_MYBMP_TYPE_RGB\n
	 *    A RGB bitmap.
	 *  - WMK_MYBMP_TYPE_BGR\n
	 *    A BGR bitmap.
	 *  - WMK_MYBMP_TYPE_RGBA\n
	 *    A RGBA bitmap.
	 *  - WMK_MYBMP_FLOW_DOWN\n
	 *    The scanline flows from top to bottom.
	 *  - WMK_MYBMP_FLOW_UP\n
	 *    The scanline flows from bottom to top.
	 *  - WMK_MYBMP_TRANSPARENT\n
	 *    Have a trasparent value.
	 *  - WMK_MYBMP_ALPHACHANNEL\n
	 *    Have a alpha channel.
	 *  - WMK_MYBMP_ALPHA\n
	 *    Have a per-pixel alpha value.
	 *  - WMK_MYBMP_RGBSIZE_3\n
	 *    Size of each RGB triple is 3 bytes.
	 *  - WMK_MYBMP_RGBSIZE_4\n
	 *    Size of each RGB triple is 4 bytes.
	 *  - WMK_MYBMP_LOAD_GRAYSCALE\n
	 *    Tell bitmap loader to load a grayscale bitmap.
	 *  - WMK_MYBMP_LOAD_ALLOCATE_ONE\n
	 *    Tell bitmap loader to allocate space for only one scanline.
	 */
	unsigned long flags;
	/* The number of the frames. */
	int   frames;
	/* The pixel depth. */
	uint8_t depth;
	/* The alpha channel value. */
	uint8_t alpha;
	uint8_t reserved [2];
	/* The transparent pixel. */
	uint32_t transparent;

	/* The width of the bitmap. */
	uint32_t w;
	/* The height of the bitmap. */
	uint32_t h;
	/* The pitch of the bitmap. */
	uint32_t pitch;
	/** The size of the bits of the bitmap. */
	uint32_t size;

	/* The pointer to the bits of the bitmap. */
	unsigned char *bits;
};

struct bitmap {
	/**
	 * Bitmap types, can be OR'ed by the following values:
	 *  - WMK_BMP_TYPE_NORMAL\n
	 *    A nomal bitmap, without alpha and color key.
	 *  - WMK_BMP_TYPE_RLE\n
	 *    A RLE (run-length-encode) encoded bitmap.
	 *  - WMK_BMP_TYPE_ALPHA\n
	 *    Per-pixel alpha in the bitmap.
	 *  - WMK_BMP_TYPE_ALPHACHANNEL\n
	 *    The \a bmAlpha is a valid alpha channel value.
	 *  - WMK_BMP_TYPE_COLORKEY\n
	 *    The \a bmColorKey is a valid color key value.
	 *  - WMK_BMP_TYPE_ALPHA_MASK\n
	 *    The bitmap have a private Alpha Mask array.
	 */
	uint8_t   bmType;
	/* The bits per piexel. */
	uint8_t   bmBitsPerPixel;
	/* The bytes per piexel. */
	uint8_t   bmBytesPerPixel;
	/* The alpha channel value. */
	uint8_t   bmAlpha;
	/* The color key value. */
	uint32_t  bmColorKey;
#ifdef _FOR_MONOBITMAP
	uint32_t  bmColorRep;
#endif

	/* The width of the bitmap */
	uint32_t  bmWidth;
	/* The height of the bitmap */
	uint32_t  bmHeight;
	/* The pitch of the bitmap */
	uint32_t  bmPitch;
	/* The bits of the bitmap */
	uint8_t  *bmBits;

	/* The private pixel format */
	/*void*   bmAlphaPixelFormat;*/
	/** The Alpha Mask array of the bitmap */
	uint8_t  *bmAlphaMask;

	/** The Alpha Pitch of the bitmap */
	uint32_t  bmAlphaPitch;
};

static inline const char *
wmk_get_extension(const char *filename)
{
	const char *ext;

	ext = strrchr(filename, '.');
	if (ext)
		return ext + 1;

	return NULL;
}

/* The type of scanline loaded callback. */
typedef void (* _cb_one_scanline)(void *context, struct mybitmap *my_bmp,
				  int y);

/* Bitmap scaler's getting line buffer callback. */
typedef void *(* _cb_get_line_buff)(void *context, int y,
				    void **alpha_line_mask);

/* Bitmap scaler's getting line buffer callback. */
typedef void (* _cb_line_scaled)(void *context, const void *line, int y);

int load_bitmap_from_file(struct bitmap *bmp, const char *file_name);
#define load_bitmap  load_bitmap_from_file

void unload_bitmap(struct bitmap *pBitmap);

BOOL scale_bitmap(struct bitmap *dst, const struct bitmap *src);

int cleanup_mybitmap(struct mybitmap *my_bmp, void *load_info);

pixel_t get_pixel_in_bitmap(const struct bitmap *bmp, int x, int y,
			    uint8_t *alpha);

void pixel_to_rgbas(const pixel_t *pixels, struct rgb_t *rgbs, int count);

unsigned char *set_pixel(unsigned char *dst, int bpp, uint32_t pixel);

uint32_t get_box_size(uint8_t bytesPerPixel, uint8_t bitsPerPixel, uint32_t w,
                      uint32_t h, uint32_t *pitch_p);

void delete_bitmap_alpha_pixel(struct bitmap *bmp);
#endif
