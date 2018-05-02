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

#include <time.h>
#include "wmk_readbmp.h"
#include "wmk_bitmap.h"

struct load_bitmap_info {
	struct bitmap *bmp;
	struct rgb_t *pal;
};

struct scaler_info {
	struct bitmap *dst;
	int last_y;
};

static const struct rgb_t windows_stdcolor[] = {
	{0x00, 0x00, 0x00},     /* black         --0 */
	{0x80, 0x00, 0x00},     /* dark red      --1 */
	{0x00, 0x80, 0x00},     /* dark green    --2 */
	{0x80, 0x80, 0x00},     /* dark yellow   --3 */
	{0x00, 0x00, 0x80},     /* dark blue     --4 */
	{0x80, 0x00, 0x80},     /* dark magenta  --5 */
	{0x00, 0x80, 0x80},     /* dark cyan     --6 */
	{0xC0, 0xC0, 0xC0},     /* light gray    --7 */
	{0x80, 0x80, 0x80},     /* dark gray     --8 */
	{0xFF, 0x00, 0x00},     /* red           --9 */
	{0x00, 0xFF, 0x00},     /* green         --10 */
	{0xFF, 0xFF, 0x00},     /* yellow        --11 */
	{0x00, 0x00, 0xFF},     /* blue          --12 */
	{0xFF, 0x00, 0xFF},     /* magenta       --13 */
	{0x00, 0xFF, 0xFF},     /* cyan          --14 */
	{0xFF, 0xFF, 0xFF},     /* light white   --15 */
};

static BOOL bitmap_dda_scaler(void *context, const struct bitmap *src_bmp,
			      int dst_w, int dst_h,
			      _cb_get_line_buff cb_line_buff, _cb_line_scaled cb_line_scaled);

/* Find the pixel value corresponding to an RGBA quadruple */
static uint32_t map_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	uint32_t pixv = (r >> RGB_FORMAT_RLOSS) << RGB_FORMAT_RSHIFT
			| (g >> RGB_FORMAT_GLOSS) << RGB_FORMAT_GSHIFT
			| (b >> RGB_FORMAT_BLOSS) << RGB_FORMAT_BSHIFT
			| ((a >> RGB_FORMAT_ALOSS) << RGB_FORMAT_ASHIFT & RGB_FORMAT_AMASK);
#ifdef _NEWGAL_SWAP16
	pixv = (pixv & 0xFFFF0000) | arch_swap16((unsigned short)pixv);
#endif
	return pixv;
}

/* Find the opaque pixel value corresponding to an RGB triple */
static uint32_t map_rgb(uint8_t r, uint8_t g, uint8_t b)
{
	uint32_t pixv = (r >> RGB_FORMAT_RLOSS) << RGB_FORMAT_RSHIFT
			| (g >> RGB_FORMAT_GLOSS) << RGB_FORMAT_GSHIFT
			| (b >> RGB_FORMAT_BLOSS) << RGB_FORMAT_BSHIFT
			| RGB_FORMAT_AMASK;
#ifdef _NEWGAL_SWAP16
	pixv = (pixv & 0xFFFF0000) | arch_swap16((unsigned short)pixv);
#endif
#if defined(_EM86_IAL) || defined (_EM85_IAL)
	if (pixv == 0) pixv ++;
#endif
	return pixv;
}

static void get_rgba(uint32_t pixel, uint8_t *r, uint8_t *g, uint8_t *b,
		     uint8_t *a)
{
	unsigned rv, gv, bv, av;
#ifdef _NEWGAL_SWAP16
	pixel = (pixel & 0xFFFF0000) | arch_swap16((unsigned short)pixel);
#endif

	rv = (pixel & RGB_FORMAT_RMASK) >> RGB_FORMAT_RSHIFT;
	*r = (rv << RGB_FORMAT_RLOSS) + (rv >> (8 - RGB_FORMAT_RLOSS));
	gv = (pixel & RGB_FORMAT_GMASK) >> RGB_FORMAT_GSHIFT;
	*g = (gv << RGB_FORMAT_GLOSS) + (gv >> (8 - RGB_FORMAT_GLOSS));
	bv = (pixel & RGB_FORMAT_BMASK) >> RGB_FORMAT_BSHIFT;
	*b = (bv << RGB_FORMAT_BLOSS) + (bv >> (8 - RGB_FORMAT_BLOSS));
	if (RGB_FORMAT_AMASK) {
		av = (pixel & RGB_FORMAT_AMASK) >> RGB_FORMAT_ASHIFT;
		*a = (av << RGB_FORMAT_ALOSS) + (av >> (8 - RGB_FORMAT_ALOSS));
	} else
		*a = WMK_ALPHA_OPAQUE;

}

static inline BOOL wmk_device_has_alpha()
{
	if (RGB_FORMAT_AMASK) {
		int max = MAX(MAX(RGB_FORMAT_RLOSS, RGB_FORMAT_GLOSS), RGB_FORMAT_BLOSS);
		if (RGB_FORMAT_ALOSS > max)
			return FALSE;

		return TRUE;
	}
	return FALSE;
}

static inline int wmk_muldiv64(int m1, int m2, int d)
{
	signed long long mul = (signed long long) m1 * m2;

	return (int)(mul / d);
}

static BOOL wmk_find_color_key(const struct mybitmap *my_bmp,
			       const struct rgb_t *pal, struct rgb_t *trans)
{
	BOOL ret = FALSE;
	int i, j;
	int color_num = 0;
	uint32_t *pixels = NULL;
	uint32_t pixel_key;

	if (!(my_bmp->flags & WMK_MYBMP_TRANSPARENT))
		goto free_and_ret;

	if (my_bmp->depth <= 1 || my_bmp->depth > 8)
		goto free_and_ret;

	color_num = 1 << my_bmp->depth;
	pixels = calloc(color_num, sizeof(uint32_t));
	for (i = 0; i < color_num; i++)
		pixels[i] = map_rgb(pal[i].r, pal[i].g, pal[i].b);

	pixel_key = pixels [my_bmp->transparent];

	for (i = 0; i < color_num; i++)
		if (pixels[i] == pixel_key && i != my_bmp->transparent)
			break;

	if (i == color_num)
		goto free_and_ret;

	srand(time(NULL));
	for (j = 0; j < 256; j++) {
		trans->r = rand() % 256;
		trans->g = rand() % 256;
		trans->b = rand() % 256;
		pixel_key = map_rgb(trans->r, trans->g, trans->b);

		for (i = 0; i < color_num; i++)
			if (pixels[i] == pixel_key)
				break;

		if (i == color_num) {
			ret = TRUE;
			break;
		}
	}

free_and_ret:
	if (pixels)
		free(pixels);

	return ret;
}

unsigned char *set_pixel(unsigned char *dst, int bpp, uint32_t pixel)
{
	switch (bpp) {
	case 1:
		*dst = pixel;
		break;
	case 2:
		*(uint16_t *) dst = pixel;
		break;
	case 3:
		if (WMK_BYTEORDER == WMK_LIL_ENDIAN) {
			dst [0] = pixel;
			dst [1] = pixel >> 8;
			dst [2] = pixel >> 16;
		} else {
			dst [0] = pixel >> 16;
			dst [1] = pixel >> 8;
			dst [2] = pixel;
		}
		break;
	case 4:
		*(uint32_t *) dst = pixel;
		break;
	}

	return dst + bpp;
}

static inline pixel_t get_pixel(uint8_t *dst, int bpp)
{
	switch (bpp) {
	case 1:
		return *dst;
	case 2:
		return *(uint16_t *)dst;
	case 3: {
		pixel_t pixel;
		if (WMK_BYTEORDER == WMK_LIL_ENDIAN)
			pixel = dst[0] + (dst[1] << 8) + (dst[2] << 16);
		else
			pixel = (dst[0] << 16) + (dst[1] << 8) + dst[2];

		return pixel;
	}
	case 4:
		return *(uint32_t *)dst;
	}

	return 0;
}

static void *wmk_get_line_buff_scalebitmap(void *context, int y,
		void **alpha_line_mask)
{
	unsigned char *line;
	struct scaler_info *info = (struct scaler_info *) context;

	line = info->dst->bmBits;
	line += info->dst->bmPitch * y;
	info->last_y = y;

	if ((info->dst->bmType & WMK_BMP_TYPE_ALPHA_MASK)
	    && info->dst->bmAlphaMask) {
		int alpha_pitch = info->dst->bmAlphaPitch;
		*(unsigned char **)alpha_line_mask = info->dst->bmAlphaMask + alpha_pitch * y;
	}
	return line;
}

static void wmk_line_scaled_scalebitmap(void *context, const void *line, int y)
{
	unsigned char *dst_line;
	struct scaler_info *info = (struct scaler_info *) context;

	if (y != info->last_y) {
		dst_line = info->dst->bmBits + info->dst->bmPitch * y;
		memcpy(dst_line, line, info->dst->bmPitch);
	}
}

/* This function expand monochorate bitmap. */
static void expand_mono_bitmap(unsigned char *bits, uint32_t pitch,
			       const unsigned char *my_bits, uint32_t my_pitch,
			       uint32_t w, uint32_t h, unsigned long flags,
			       uint32_t bg, uint32_t fg)
{
	uint32_t x, y;
	unsigned char *dst, *dst_line;
	const unsigned char *src, *src_line;
	uint32_t pixel;
	int bpp = BYTES_PER_PIXEL;
	unsigned char byte = 0;

	dst_line = bits;
	if (flags & WMK_MYBMP_FLOW_UP)
		src_line = my_bits + my_pitch * (h - 1);
	else
		src_line = my_bits;

	for (y = 0; y < h; y++) {
		src = src_line;
		dst = dst_line;

		for (x = 0; x < w; x++) {
			if (x % 8 == 0)
				byte = *src++;

			if ((byte & (128 >> (x % 8))))
				pixel = fg;
			else
				pixel = bg;
			dst = set_pixel(dst, bpp, pixel);
		}

		if (flags & WMK_MYBMP_FLOW_UP)
			src_line -= my_pitch;
		else
			src_line += my_pitch;

		dst_line += pitch;
	}
}

/* This function expand 16-color bitmap. */
static void expand_16color_bitmapEx(unsigned char *bits, uint32_t pitch,
				    const unsigned char *my_bits, uint32_t my_pitch,
				    uint32_t w, uint32_t h, unsigned long flags,
				    const struct rgb_t *pal, unsigned char use_pal_alpha,
				    unsigned char alpha)
{
	uint32_t x, y;
	unsigned char *dst, *dst_line;
	const unsigned char *src, *src_line;
	int index, bpp;
	uint32_t pixel;
	unsigned char byte = 0;

	bpp = BYTES_PER_PIXEL;

	dst_line = bits;
	if (flags & WMK_MYBMP_FLOW_UP)
		src_line = my_bits + my_pitch * (h - 1);
	else
		src_line = my_bits;

	for (y = 0; y < h; y++) {
		src = src_line;
		dst = dst_line;
		for (x = 0; x < w; x++) {
			if (x % 2 == 0) {
				byte = *src++;
				index = (byte >> 4) & 0x0f;
			} else
				index = byte & 0x0f;

			if (pal)
				pixel = map_rgba(pal[index].r, pal[index].g, pal[index].b,
						 use_pal_alpha ? pal[index].a : 0xFF);
			else
				pixel = map_rgba(windows_stdcolor[index].r,
						 windows_stdcolor[index].g,
						 windows_stdcolor[index].b,
						 alpha);

			dst = set_pixel(dst, bpp, pixel);
		}

		if (flags & WMK_MYBMP_FLOW_UP)
			src_line -= my_pitch;
		else
			src_line += my_pitch;

		dst_line += pitch;
	}
}

static void expand_16color_bitmap(unsigned char *bits, uint32_t pitch,
				  const unsigned char *my_bits, uint32_t my_pitch,
				  uint32_t w, uint32_t h, unsigned long flags,
				  const struct rgb_t *pal)
{
	expand_16color_bitmapEx(bits, pitch, my_bits, my_pitch,
				w, h, flags, pal, FALSE, 0xFF);
}

/* This function expands 256-color bitmap. */
static void expand_256color_bitmapEx(unsigned char *bits, uint32_t pitch,
				     const unsigned char *my_bits, uint32_t my_pitch,
				     uint32_t w, uint32_t h, unsigned long flags,
				     const struct rgb_t *pal, unsigned char use_pal_alpha,
				     unsigned char alpha)
{
	uint32_t x, y;
	int bpp;
	unsigned char *dst, *dst_line;
	const unsigned char *src, *src_line;
	uint32_t pixel;
	unsigned char byte;

	bpp = BYTES_PER_PIXEL;

	dst_line = bits;
	if (flags & WMK_MYBMP_FLOW_UP)
		src_line = my_bits + my_pitch * (h - 1);
	else
		src_line = my_bits;

	for (y = 0; y < h; y++) {
		src = src_line;
		dst = dst_line;
		for (x = 0; x < w; x++) {
			byte = *src++;

			if (pal)
				pixel = map_rgba(pal[byte].r, pal[byte].g, pal[byte].b,
						 use_pal_alpha ? pal[byte].a : 0xFF);
			else if (bpp == 1)
				/*
				 * Assume that the palette of the bitmap is the same as
				 * the palette of the DC.
				 */
				pixel = byte;
			else
				/*
				 * Treat the bitmap uses the dithered colorful palette.
				 */
				pixel = map_rgba((byte >> 5) & 0x07,
						 (byte >> 2) & 0x07,
						 byte & 0x03,
						 alpha);

			dst = set_pixel(dst, bpp, pixel);
		}

		if (flags & WMK_MYBMP_FLOW_UP)
			src_line -= my_pitch;
		else
			src_line += my_pitch;

		dst_line += pitch;
	}
}

static void expand_256color_bitmap(unsigned char *bits, uint32_t pitch,
				   const unsigned char *my_bits, uint32_t my_pitch,
				   uint32_t w, uint32_t h, unsigned long flags,
				   const struct rgb_t *pal)
{
	expand_256color_bitmapEx(bits, pitch, my_bits, my_pitch,
				 w, h, flags, pal, FALSE, 0xFF);
}

/* This function compile a RGBA bitmap. */
static void compile_rgba_bitmap(unsigned char *bits, uint32_t pitch,
				const unsigned char *my_bits, uint32_t my_pitch, uint32_t w,
				uint32_t h, unsigned long flags)
{
	uint32_t x, y;
	int bpp;
	unsigned char *dst, *dst_line;
	const unsigned char *src, *src_line;
	uint32_t pixel;
	struct rgb_t rgb;

	bpp = BYTES_PER_PIXEL;

	dst_line = bits;
	if (flags & WMK_MYBMP_FLOW_UP)
		src_line = my_bits + my_pitch * (h - 1);
	else
		src_line = my_bits;

	/* expand bits here. */
	for (y = 0; y < h; y++) {
		src = src_line;
		dst = dst_line;
		for (x = 0; x < w; x++) {
			if ((flags & WMK_MYBMP_TYPE_MASK) == WMK_MYBMP_TYPE_BGR) {
				rgb.b = *src++;
				rgb.g = *src++;
				rgb.r = *src++;
			} else {
				rgb.r = *src++;
				rgb.g = *src++;
				rgb.b = *src++;
			}

			if (flags & WMK_MYBMP_RGBSIZE_4) {
				if (flags & WMK_MYBMP_ALPHA) {
					rgb.a = *src;
					pixel = map_rgba(rgb.r, rgb.g, rgb.b, rgb.a);
				} else {
					pixel = map_rgb(rgb.r, rgb.g, rgb.b);
				}
				src++;
			} else {
				pixel = map_rgb(rgb.r, rgb.g, rgb.b);
			}

			dst = set_pixel(dst, bpp, pixel);
		}

		if (flags & WMK_MYBMP_FLOW_UP)
			src_line -= my_pitch;
		else
			src_line += my_pitch;

		dst_line += pitch;
	}
}

static void wmk_compilergba_bitmap_sl(unsigned char *bits,
				      unsigned char *alpha_mask, struct mybitmap *my_bmp)
{
	struct rgb_t rgb;
	uint32_t pixel;
	unsigned long flags;
	unsigned char *src, *dst;
	int x = 0, w = 0;
	int bpp;

	bpp = BYTES_PER_PIXEL;
	dst = bits;
	src = my_bmp->bits;
	flags = my_bmp->flags;
	w = my_bmp->w;

	for (x = 0; x < w; x++) {
		if ((flags & WMK_MYBMP_TYPE_MASK) == WMK_MYBMP_TYPE_BGR) {
			rgb.b = *src++;
			rgb.g = *src++;
			rgb.r = *src++;
		} else {
			rgb.r = *src++;
			rgb.g = *src++;
			rgb.b = *src++;
		}

		if (flags & WMK_MYBMP_RGBSIZE_4) {
			if (flags & WMK_MYBMP_ALPHA) {
				rgb.a = *src;
				if (alpha_mask) {
					alpha_mask[x] = rgb.a;
					pixel = map_rgb(rgb.r, rgb.g, rgb.b);
				} else {
					pixel = map_rgba(rgb.r, rgb.g, rgb.b, rgb.a);
				}
			} else {
				pixel = map_rgb(rgb.r, rgb.g, rgb.b);
			}
			src++;
		} else {
			pixel = map_rgb(rgb.r, rgb.g, rgb.b);
		}

		dst = set_pixel(dst, bpp, pixel);
	}
}

/*
 * Calculate the pad-aligned box sizew in a surface
 */
uint32_t get_box_size(uint8_t bytesPerPixel, uint8_t bitsPerPixel, uint32_t w,
		      uint32_t h, uint32_t *pitch_p)
{
	uint32_t pitch;

	/* Box should be 4-byte aligned for speed */
	pitch = w * bytesPerPixel;
	switch (bitsPerPixel) {
	case 1:
		pitch = (pitch + 7) / 8;
		break;
	case 4:
		pitch = (pitch + 1) / 2;
		break;
	default:
		break;
	}

	/* 4-byte aligning */
	pitch = (pitch + 3) & ~3;

	if (pitch_p)
		*pitch_p = pitch;

	return pitch * h;
}

pixel_t get_pixel_in_bitmap(const struct bitmap *bmp, int x, int y,
			    uint8_t *alpha)
{
	unsigned char *dst;

	if (x < 0 || y < 0 || x >= bmp->bmWidth || y >= bmp->bmHeight)
		return 0;

	if (alpha && bmp->bmAlphaMask) {
		int alpha_pitch = bmp->bmAlphaPitch;
		*alpha = bmp->bmAlphaMask[y * alpha_pitch + x];
	}

	dst = bmp->bmBits + y * bmp->bmPitch + x * bmp->bmBytesPerPixel;
	return get_pixel(dst, bmp->bmBytesPerPixel);
}

void pixel_to_rgbas(const pixel_t *pixels, struct rgb_t *rgbs, int count)
{
	int i;

	for (i = 0; i < count; i++)
		get_rgba(pixels[i], &rgbs[i].r, &rgbs[i].g, &rgbs[i].b, &rgbs[i].a);
}

static BOOL bitmap_dda_scaler(void *context, const struct bitmap *src_bmp,
			      int dst_w, int dst_h,
			      _cb_get_line_buff cb_line_buff, _cb_line_scaled cb_line_scaled)
{
	unsigned char *dp1 = src_bmp->bmBits;
	unsigned char *dp2;
	int xfactor;
	int yfactor;

	if (dst_w <= 0 || dst_h <= 0)
		return FALSE;

	/* scaled by 65536 */
	xfactor = wmk_muldiv64(src_bmp->bmWidth, 65536, dst_w);
	yfactor = wmk_muldiv64(src_bmp->bmHeight, 65536, dst_h);

	switch (src_bmp->bmBytesPerPixel) {
	case 1: {
		int y, sy;
		unsigned char *dp3 = src_bmp->bmAlphaMask;
		unsigned char *dp4 = NULL;
		sy = 0;
		for (y = 0; y < dst_h;) {
			int sx = 0;
			int x;

			x = 0;
			dp2 = cb_line_buff(context, y, (void *)&dp4);
			while (x < dst_w) {
				*(dp2 + x) = *(dp1 + (sx >> 16));
				if (dp4 && src_bmp->bmType & WMK_BMP_TYPE_ALPHA_MASK)
					dp4[x] = dp3[(sx >> 16)];

				sx += xfactor;
				x++;
			}
			cb_line_scaled(context, dp2, y);
			y++;
			while (y < dst_h) {
				int syint = sy >> 16;
				sy += yfactor;
				if ((sy >> 16) != syint)
					break;
				/* Copy identical lines. */
				cb_line_scaled(context, dp2, y);
				y++;
			}
			dp1 = src_bmp->bmBits + (sy >> 16) * src_bmp->bmPitch;
			if (src_bmp->bmType & WMK_BMP_TYPE_ALPHA_MASK)
				dp3 = src_bmp->bmAlphaMask + (sy >> 16) * src_bmp->bmAlphaPitch;
		}

		break;
	}
	case 2: {
		int y, sy;
		unsigned char *dp3 = src_bmp->bmAlphaMask;
		unsigned char *dp4 = NULL;
		sy = 0;
		for (y = 0; y < dst_h;) {
			int sx = 0;
			int x;

			x = 0;
			dp2 = cb_line_buff(context, y, (void *)&dp4);
			/*
			 * This can be greatly optimized with loop
			 * unrolling; omitted to save space.
			 */
			while (x < dst_w) {
				*(unsigned short *)(dp2 + x * 2) =
					*(unsigned short *)(dp1 + (sx >> 16) * 2);
				if (dp4 && src_bmp->bmType & WMK_BMP_TYPE_ALPHA_MASK)
					dp4[x] = dp3[(sx >> 16)];

				sx += xfactor;
				x++;
			}
			cb_line_scaled(context, dp2, y);
			y++;
			while (y < dst_h) {
				int syint = sy >> 16;
				sy += yfactor;
				if ((sy >> 16) != syint)
					break;
				/* Copy identical lines. */
				cb_line_scaled(context, dp2, y);
				y++;
			}
			dp1 = src_bmp->bmBits + (sy >> 16) * src_bmp->bmPitch;
			if (dp4 && src_bmp->bmType & WMK_BMP_TYPE_ALPHA_MASK)
				dp3 = src_bmp->bmAlphaMask + (sy >> 16) * src_bmp->bmAlphaPitch;
		}

		break;
	}
	case 3: {
		int y, sy;
		unsigned char *dp3 = src_bmp->bmAlphaMask;
		unsigned char *dp4 = NULL;
		sy = 0;
		for (y = 0; y < dst_h;) {
			int sx = 0;
			int x;

			x = 0;
			dp2 = cb_line_buff(context, y, (void *)&dp4);
			/*
			 * This can be greatly optimized with loop
			 * unrolling; omitted to save space.
			 */
			while (x < dst_w) {
				set_pixel(dp2 + x * 3, 3,
					  get_pixel(dp1 + (sx >> 16) * 3, 3));

				if (dp4 && src_bmp->bmType & WMK_BMP_TYPE_ALPHA_MASK)
					dp4[x] = dp3[(sx >> 16)];

				sx += xfactor;
				x++;
			}
			cb_line_scaled(context, dp2, y);
			y++;
			while (y < dst_h) {
				int syint = sy >> 16;
				sy += yfactor;
				if ((sy >> 16) != syint)
					break;
				/* Copy identical lines. */
				cb_line_scaled(context, dp2, y);
				y++;
			}
			dp1 = src_bmp->bmBits + (sy >> 16) * src_bmp->bmPitch;
			if (dp4 && src_bmp->bmType & WMK_BMP_TYPE_ALPHA_MASK)
				dp3 = src_bmp->bmAlphaMask + (sy >> 16) * src_bmp->bmAlphaPitch;
		}

		break;
	}
	case 4: {
		int y, sy;
		unsigned char *dp3 = src_bmp->bmAlphaMask;
		unsigned char *dp4 = NULL;
		sy = 0;
		for (y = 0; y < dst_h;) {
			int sx = 0;
			int x;

			x = 0;
			dp2 = cb_line_buff(context, y, (void *)&dp4);
			/*
			 * This can be greatly optimized with loop
			 * unrolling; omitted to save space.
			 */
			while (x < dst_w) {
				*(unsigned *)(dp2 + x * 4) =
					*(unsigned *)(dp1 + (sx >> 16) * 4);
				if (dp4 && src_bmp->bmType & WMK_BMP_TYPE_ALPHA_MASK)
					dp4[x] = dp3[(sx >> 16)];

				sx += xfactor;
				x++;
			}
			cb_line_scaled(context, dp2, y);
			y++;
			while (y < dst_h) {
				int syint = sy >> 16;
				sy += yfactor;
				if ((sy >> 16) != syint)
					break;
				/* Copy identical lines. */
				cb_line_scaled(context, dp2, y);
				y++;
			}
			dp1 = src_bmp->bmBits + (sy >> 16) * src_bmp->bmPitch;
			if (dp4 && src_bmp->bmType & WMK_BMP_TYPE_ALPHA_MASK)
				dp3 = src_bmp->bmAlphaMask + (sy >> 16) * src_bmp->bmAlphaPitch;
		}

		break;
	}
	}

	return TRUE;
}

BOOL scale_bitmap(struct bitmap *dst, const struct bitmap *src)
{
	struct scaler_info info;

	info.dst = dst;
	info.last_y = 0;

	if (dst->bmWidth == 0 || dst->bmHeight == 0)
		return TRUE;

	if (dst->bmBytesPerPixel != src->bmBytesPerPixel)
		return FALSE;

	bitmap_dda_scaler(&info, src, dst->bmWidth, dst->bmHeight,
			  wmk_get_line_buff_scalebitmap, wmk_line_scaled_scalebitmap);

	return TRUE;
}

/* this function delete the pixel format of a bitmap */
void delete_bitmap_alpha_pixel(struct bitmap *bmp)
{
	if (bmp->bmType & WMK_BMP_TYPE_ALPHA_MASK) {
		free(bmp->bmAlphaMask);
		bmp->bmType &= ~WMK_BMP_TYPE_ALPHA_MASK;
		bmp->bmAlphaMask = NULL;
	}
}

int cleanup_mybitmap(struct mybitmap *my_bmp, void *load_info)
{
	struct load_mybitmap_info_t *info = (struct load_mybitmap_info_t *) load_info;

	if (info == NULL)
		return -1;

	free(my_bmp->bits);
	my_bmp->bits = NULL;

	cleanup_bmp(info->init_info);
	info->init_info = NULL;
	free(info);

	return 0;
}

/* Initializes a bitmap object with a mybitmap object */
static int wmk_init_bitmap_from_mybmp(struct bitmap *bmp,
				      const struct mybitmap *my_bmp,
				      struct rgb_t *pal, BOOL alloc_all)
{
	int ret = 0;
	unsigned int size;
	uint32_t alpha_pitch;

	bmp->bmBitsPerPixel = BITS_PER_PIXEL;
	bmp->bmBytesPerPixel = BYTES_PER_PIXEL;
	bmp->bmAlphaMask = NULL;
	bmp->bmAlphaPitch = 0;

	size = get_box_size(BYTES_PER_PIXEL, BITS_PER_PIXEL, my_bmp->w, my_bmp->h,
			    &bmp->bmPitch);

	if (!(bmp->bmBits = calloc(1, alloc_all ? size : bmp->bmPitch))) {
		ret = -1;
		goto cleanup_and_ret;
	}

	bmp->bmWidth = my_bmp->w;
	bmp->bmHeight = my_bmp->h;

	bmp->bmType = WMK_BMP_TYPE_NORMAL;
	if (my_bmp->flags & WMK_MYBMP_TRANSPARENT) {
		struct rgb_t trans;

		bmp->bmType |= WMK_BMP_TYPE_COLORKEY;
		if (pal && my_bmp->depth <= 8) {
			if (wmk_find_color_key(my_bmp, pal, &trans))
				pal [my_bmp->transparent] = trans;
			else
				trans = pal [my_bmp->transparent];
		} else {
			if ((my_bmp->flags & WMK_MYBMP_TYPE_MASK) == WMK_MYBMP_TYPE_BGR) {
				trans.b = get_r_value(my_bmp->transparent);
				trans.g = get_g_value(my_bmp->transparent);
				trans.r = get_b_value(my_bmp->transparent);
			} else {
				trans.r = get_r_value(my_bmp->transparent);
				trans.g = get_g_value(my_bmp->transparent);
				trans.b = get_b_value(my_bmp->transparent);
			}
		}
		bmp->bmColorKey = map_rgb(trans.r, trans.g, trans.b);
	} else
		bmp->bmColorKey = 0;

	if (my_bmp->flags & WMK_MYBMP_ALPHACHANNEL) {
		bmp->bmType |= WMK_BMP_TYPE_ALPHACHANNEL;
		bmp->bmAlpha = my_bmp->alpha;
	} else
		bmp->bmAlpha = 0;

	if (my_bmp->flags & WMK_MYBMP_ALPHA && my_bmp->depth == 32) {
		bmp->bmType |= WMK_BMP_TYPE_ALPHA;
		if (!wmk_device_has_alpha()) {
			bmp->bmType |= WMK_BMP_TYPE_ALPHA_MASK;
			alpha_pitch = (bmp->bmWidth + 3) & (~3);

			if (alloc_all)
				size = bmp->bmHeight * alpha_pitch;
			else
				size = alpha_pitch;

			bmp->bmAlphaMask = calloc(1, size);
			bmp->bmAlphaPitch = alpha_pitch;

			if (bmp->bmAlphaMask == NULL) {
				ret = -1;
				goto cleanup_and_ret;
			}
		}
	}

	return 0;

cleanup_and_ret:
	if (ret != 0) {
		delete_bitmap_alpha_pixel(bmp);
		if (bmp->bmBits) {
			free(bmp->bmBits);
			bmp->bmBits = NULL;
		}
	}

	return ret;
}

static void *mybmp_init(struct mg_rw_ops *area, struct mybitmap *my_bmp,
			struct rgb_t *pal)
{
	struct load_mybitmap_info_t *load_info;

	load_info = calloc(1, sizeof(struct load_mybitmap_info_t));
	if (load_info == NULL)
		return NULL;

	my_bmp->flags = WMK_MYBMP_LOAD_ALLOCATE_ONE;
	my_bmp->bits = NULL;
	my_bmp->frames = 1;
	my_bmp->depth = BYTES_PER_PIXEL << 3;

	if (my_bmp->depth <= 8) {
		printf("%s: unsupported bmp depth: %d\n", __func__, my_bmp->depth);
		goto fail;
	}

	/*
	 * This is just for gray screen. If your screen is gray,
	 * please define this macro _GRAY_SCREEN.
	 */
#ifdef _GRAY_SCREEN
	my_bmp->flags |= WMK_MYBMP_LOAD_GRAYSCALE;
#endif

	load_info->init_info = init_bmp(area, my_bmp, pal);
	if (load_info->init_info == NULL)
		goto fail;

	my_bmp->bits = calloc(1, my_bmp->pitch);
	if (my_bmp->bits == NULL)
		goto fail;

	return load_info;
fail:
	if (my_bmp->bits != NULL)
		free(my_bmp->bits);

	free(load_info);
	return NULL;
}

static void wmk_cb_load_bitmap_sl(void *context, struct mybitmap *my_bmp,
				  int y)
{
	struct load_bitmap_info *info = (struct load_bitmap_info *)context;
	uint8_t *src_bits, *dst_bits, *dst_alpha_mask;

	src_bits = my_bmp->bits;
	dst_bits = info->bmp->bmBits + info->bmp->bmPitch * y;

	if (info->bmp->bmType & WMK_BMP_TYPE_ALPHA_MASK)
		dst_alpha_mask = info->bmp->bmAlphaMask + info->bmp->bmAlphaPitch * y;
	else
		dst_alpha_mask = NULL;

	switch (my_bmp->depth) {
	case 1:
		expand_mono_bitmap(dst_bits, info->bmp->bmPitch,
				   src_bits, my_bmp->pitch, my_bmp->w, 1, my_bmp->flags,
				   map_rgb(info->pal[0].r, info->pal[0].g, info->pal[0].b),
				   map_rgb(info->pal[1].r, info->pal[1].g, info->pal[1].b));
		break;
	case 4:
		expand_16color_bitmap(dst_bits, info->bmp->bmPitch,
				      src_bits, my_bmp->pitch, my_bmp->w, 1,
				      my_bmp->flags, info->pal);
		break;
	case 8:
		expand_256color_bitmap(dst_bits, info->bmp->bmPitch,
				       src_bits, my_bmp->pitch, my_bmp->w, 1,
				       my_bmp->flags, info->pal);
		break;
	case 24:
		compile_rgba_bitmap(dst_bits, info->bmp->bmPitch,
				    src_bits, my_bmp->pitch, my_bmp->w, 1,
				    my_bmp->flags & ~WMK_MYBMP_RGBSIZE_4);
		break;
	case 32:
		wmk_compilergba_bitmap_sl(dst_bits, dst_alpha_mask, my_bmp);
		break;
	default:
		break;
	}
}

static int load_bitmapEx(struct mg_rw_ops *area, struct bitmap *bmp)
{
	struct mybitmap my_bmp;
	struct rgb_t pal [256];
	int ret;
	void *load_info;
	struct load_bitmap_info info;

	info.bmp = bmp;
	info.pal = pal;

	load_info = mybmp_init(area, &my_bmp, pal);
	if (load_info == NULL)
		return -1;

	ret = wmk_init_bitmap_from_mybmp(bmp, &my_bmp, pal, TRUE);
	if (ret) {
		cleanup_mybitmap(&my_bmp, load_info);
		return ret;
	}

	ret = load_bmp(area, ((struct load_mybitmap_info_t *)load_info)->init_info,
		       &my_bmp, wmk_cb_load_bitmap_sl, &info);

	cleanup_mybitmap(&my_bmp, load_info);

	return ret;
}

int load_bitmap_from_file(struct bitmap *bmp, const char *file_name)
{
	int ret;
	struct mg_rw_ops *area;
	const char *ext;

	/* format, bmp. */
	ext = wmk_get_extension(file_name);
	if (strcasecmp(ext, "bmp") != 0) {
		printf("watermark image is not BMP format.\n");
		return -1;
	}
	if (!(area = rw_from_file(file_name, "rb"))) {
		printf("Wmk_RWFromFile error\n");
		return -1;
	}

	ret = load_bitmapEx(area, bmp);

	rw_close(area);

	return ret;
}

/* this function unloads bitmap */
void unload_bitmap(struct bitmap *bmp)
{
	delete_bitmap_alpha_pixel(bmp);

	free(bmp->bmBits);
	bmp->bmBits = NULL;
}
