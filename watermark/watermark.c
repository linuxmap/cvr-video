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

#include "watermark.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "parameter.h"
#include "licenseplate.h"
#include "ui_resolution.h"
#include "wmk_rect.h"
#include "wmk_glyph.h"

/* argb(lower address -> high address) */
static const uint32_t rgb888_palette_table[PALETTE_TABLE_LEN] = {
	0xffffffff, 0xd7ffffff, 0xafffffff, 0x87ffffff, 0x5fffffff, 0x00ffffff,
	0xffd7ffff, 0xd7d7ffff, 0xafd7ffff, 0x87d7ffff, 0x5fd7ffff, 0x00d7ffff,
	0xffafffff, 0xd7afffff, 0xafafffff, 0x87afffff, 0x5fafffff, 0x00afffff,
	0xff87ffff, 0xd787ffff, 0xaf87ffff, 0x8787ffff, 0x5f87ffff, 0x0087ffff,
	0xff5fffff, 0xd75fffff, 0xaf5fffff, 0x875fffff, 0x5f5fffff, 0x005fffff,
	0xff00ffff, 0xd700ffff, 0xaf00ffff, 0x8700ffff, 0x5f00ffff, 0x0000ffff,
	0xffffd7ff, 0xd7ffd7ff, 0xafffd7ff, 0x87ffd7ff, 0x5fffd7ff, 0x00ffd7ff,
	0xffd7d7ff, 0xd7d7d7ff, 0xafd7d7ff, 0x87d7d7ff, 0x5fd7d7ff, 0x00d7d7ff,
	0xffafd7ff, 0xd7afd7ff, 0xafafd7ff, 0x87afd7ff, 0x5fafd7ff, 0x00afd7ff,
	0xff87d7ff, 0xd787d7ff, 0xaf87d7ff, 0x8787d7ff, 0x5f87d7ff, 0x0087d7ff,
	0xff5fd7ff, 0xd75fd7ff, 0xaf5fd7ff, 0x875fd7ff, 0x5f5fd7ff, 0x005fd7ff,
	0xff00d7ff, 0xd700d7ff, 0xaf00d7ff, 0x8700d7ff, 0x5f00d7ff, 0x0000d7ff,
	0xffffafff, 0xd7ffafff, 0xafffafff, 0x87ffafff, 0x5fffafff, 0x00ffafff,
	0xffd7afff, 0xd7d7afff, 0xafd7afff, 0x87d7afff, 0x5fd7afff, 0x00d7afff,
	0xffafafff, 0xd7afafff, 0xafafafff, 0x87afafff, 0x5fafafff, 0x00afafff,
	0xff87afff, 0xd787afff, 0xaf87afff, 0x8787afff, 0x5f87afff, 0x0087afff,
	0xff5fafff, 0xd75fafff, 0xaf5fafff, 0x875fafff, 0x5f5fafff, 0x005fafff,
	0xff00afff, 0xd700afff, 0xaf00afff, 0x8700afff, 0x5f00afff, 0x0000afff,
	0xffff87ff, 0xd7ff87ff, 0xafff87ff, 0x87ff87ff, 0x5fff87ff, 0x00ff87ff,
	0xffd787ff, 0xd7d787ff, 0xafd787ff, 0x87d787ff, 0x5fd787ff, 0x00d787ff,
	0xffaf87ff, 0xd7af87ff, 0xafaf87ff, 0x87af87ff, 0x5faf87ff, 0x00af87ff,
	0xff8787ff, 0xd78787ff, 0xaf8787ff, 0x878787ff, 0x5f8787ff, 0x008787ff,
	0xff5f87ff, 0xd75f87ff, 0xaf5f87ff, 0x875f87ff, 0x5f5f87ff, 0x005f87ff,
	0xff0087ff, 0xd70087ff, 0xaf0087ff, 0x870087ff, 0x5f0087ff, 0x000087ff,
	0xffff5fff, 0xd7ff5fff, 0xafff5fff, 0x87ff5fff, 0x5fff5fff, 0x00ff5fff,
	0xffd75fff, 0xd7d75fff, 0xafd75fff, 0x87d75fff, 0x5fd75fff, 0x00d75fff,
	0xffaf5fff, 0xd7af5fff, 0xafaf5fff, 0x87af5fff, 0x5faf5fff, 0x00af5fff,
	0xff875fff, 0xd7875fff, 0xaf875fff, 0x87875fff, 0x5f875fff, 0x00875fff,
	0xff5f5fff, 0xd75f5fff, 0xaf5f5fff, 0x875f5fff, 0x5f5f5fff, 0x005f5fff,
	0xff005fff, 0xd7005fff, 0xaf005fff, 0x87005fff, 0x5f005fff, 0x00005fff,
	0xffff00ff, 0xd7ff00ff, 0xafff00ff, 0x87ff00ff, 0x5fff00ff, 0x00ff00ff,
	0xffd700ff, 0xd7d700ff, 0xafd700ff, 0x87d700ff, 0x5fd700ff, 0x00d700ff,
	0xffaf00ff, 0xd7af00ff, 0xafaf00ff, 0x87af00ff, 0x5faf00ff, 0x00af00ff,
	0xff8700ff, 0xd78700ff, 0xaf8700ff, 0x878700ff, 0x5f8700ff, 0x008700ff,
	0xff5f00ff, 0xd75f00ff, 0xaf5f00ff, 0x875f00ff, 0x5f5f00ff, 0x005f00ff,
	0xff0000ff, 0xd70000ff, 0xaf0000ff, 0x870000ff, 0x5f0000ff, 0x000000ff,
	0xeeeeeeff, 0xe4e4e4ff, 0xdadadaff, 0xd0d0d0ff, 0xc6c6c6ff, 0xbcbcbcff,
	0xb2b2b2ff, 0xa8a8a8ff, 0x9e9e9eff, 0x949494ff, 0x8a8a8aff, 0x767676ff,
	0x6c6c6cff, 0x626262ff, 0x585858ff, 0x4e4e4eff, 0x444444ff, 0x3a3a3aff,
	0x303030ff, 0x262626ff, 0x1c1c1cff, 0x121212ff, 0x080808ff, 0x000080ff,
	0x008000ff, 0x008080ff, 0x800000ff, 0x800080ff, 0x808000ff, 0xc0c0c0ff,
	0x808080ff, 0xffffccff, 0xccccccff, 0x99ccccff, 0x9999ccff, 0x3366ccff,
	0x0033ccff, 0x3300ccff, 0x00ffccff, 0xffffff00
};

/* yuva(lower address -> high address) */
const uint32_t yuv444_palette_table[PALETTE_TABLE_LEN] = {
	0xff8080eb, 0xff826ee9, 0xff835de6, 0xff854be4, 0xff863ae1, 0xff8a10db,
	0xff908ed2, 0xff927cd0, 0xff936acd, 0xff9559cb, 0xff9647c9, 0xff9a1ec3,
	0xffa09bba, 0xffa28ab7, 0xffa378b5, 0xffa566b2, 0xffa655b0, 0xffaa2baa,
	0xffb0a9a1, 0xffb1979f, 0xffb3859c, 0xffb5749a, 0xffb66297, 0xffba3991,
	0xffc0b689, 0xffc1a586, 0xffc39384, 0xffc58181, 0xffc6707f, 0xffca4679,
	0xffe6d64e, 0xffe7c54c, 0xffe9b349, 0xffeba247, 0xffec9044, 0xfff0663f,
	0xff6e84e4, 0xff7072e1, 0xff7261df, 0xff734fdc, 0xff753eda, 0xff7914d4,
	0xff7e92cb, 0xff8080c9, 0xff826ec6, 0xff835dc4, 0xff854bc1, 0xff8922bb,
	0xff8e9fb3, 0xff908eb0, 0xff927cae, 0xff936aab, 0xff9559a9, 0xff992fa3,
	0xff9ead9a, 0xffa09b98, 0xffa28a95, 0xffa37893, 0xffa56690, 0xffa93d8a,
	0xffaeba81, 0xffb0a97f, 0xffb1977c, 0xffb3857a, 0xffb57477, 0xffb94a72,
	0xffd4da47, 0xffd6c945, 0xffd7b742, 0xffd9a640, 0xffdb943d, 0xffde6a37,
	0xff5d88dc, 0xff5e76da, 0xff6065d7, 0xff6253d5, 0xff6342d2, 0xff6718cd,
	0xff6d96c4, 0xff6e84c1, 0xff7072bf, 0xff7261bc, 0xff734fba, 0xff7726b4,
	0xff7da3ab, 0xff7e92a9, 0xff8080a6, 0xff826ea4, 0xff835da1, 0xff87339b,
	0xff8db193, 0xff8e9f90, 0xff908e8e, 0xff927c8b, 0xff936a89, 0xff974183,
	0xff9dbe7a, 0xff9ead78, 0xffa09b75, 0xffa28a73, 0xffa37870, 0xffa74e6a,
	0xffc3de40, 0xffc4cd3d, 0xffc6bb3b, 0xffc7aa38, 0xffc99836, 0xffcd6e30,
	0xff4b8cd5, 0xff4d7bd3, 0xff4f69d0, 0xff5057ce, 0xff5246cb, 0xff561cc5,
	0xff5b9abd, 0xff5d88ba, 0xff5e76b8, 0xff6065b5, 0xff6253b3, 0xff662aad,
	0xff6ba7a4, 0xff6d96a1, 0xff6e849f, 0xff70729d, 0xff72619a, 0xff753794,
	0xff7bb58b, 0xff7da389, 0xff7e9286, 0xff808084, 0xff826e81, 0xff85457c,
	0xff8bc273, 0xff8db170, 0xff8e9f6e, 0xff908e6b, 0xff927c69, 0xff955263,
	0xffb1e238, 0xffb3d136, 0xffb4bf34, 0xffb6ae31, 0xffb79c2f, 0xffbb7229,
	0xff3a90ce, 0xff3b7fcb, 0xff3d6dc9, 0xff3f5bc6, 0xff404ac4, 0xff4420be,
	0xff4a9eb5, 0xff4b8cb3, 0xff4d7bb0, 0xff4f69ae, 0xff5057ab, 0xff542ea5,
	0xff5aab9d, 0xff5b9a9a, 0xff5d8898, 0xff5e7695, 0xff606593, 0xff643b8d,
	0xff6ab984, 0xff6ba782, 0xff6d967f, 0xff6e847d, 0xff70727a, 0xff744974,
	0xff7ac66c, 0xff7bb569, 0xff7da367, 0xff7e9264, 0xff808062, 0xff84565c,
	0xff9fe631, 0xffa1d52f, 0xffa3c32c, 0xffa4b22a, 0xffa6a027, 0xffaa7621,
	0xff109abc, 0xff1288ba, 0xff1377b7, 0xff1565b5, 0xff1653b3, 0xff1a2aad,
	0xff20a7a4, 0xff2296a1, 0xff23849f, 0xff25729c, 0xff26619a, 0xff2a3794,
	0xff30b58b, 0xff32a389, 0xff339286, 0xff358084, 0xff366e81, 0xff3a457b,
	0xff40c273, 0xff41b170, 0xff439f6e, 0xff458e6b, 0xff467c69, 0xff4a5263,
	0xff50d05a, 0xff51be58, 0xff53ad55, 0xff559b53, 0xff568a50, 0xff5a604a,
	0xff76f020, 0xff77de1d, 0xff79cd1b, 0xff7bbb18, 0xff7caa16, 0xff808010,
	0xff8080dc, 0xff8080d4, 0xff8080cb, 0xff8080c3, 0xff8080ba, 0xff8080b1,
	0xff8080a9, 0xff8080a0, 0xff808098, 0xff80808f, 0xff808087, 0xff808075,
	0xff80806d, 0xff808064, 0xff80805c, 0xff808053, 0xff80804a, 0xff808042,
	0xff808039, 0xff808031, 0xff808028, 0xff80801f, 0xff808017, 0xffb87327,
	0xff4d555f, 0xff854876, 0xff7bb818, 0xffb3ab2f, 0xff488d67, 0xff8080b5,
	0xff80807e, 0xff6a85e2, 0xff8080bf, 0xff826abc, 0xff967b9d, 0xffaf5f77,
	0xffc55a55, 0xffd88238, 0xff7415d2, 0x008080eb
};

#ifdef SDV
extern struct bitmap watermark_bmap[8];
#else
extern struct bitmap watermark_bmap[7];
#endif

/* Match an RGB value to a particular palette index */
static uint8_t find_color(const uint32_t *pal, uint32_t len, uint8_t r,
			  uint8_t g, uint8_t b)
{
	int i = 0;
	uint8_t pixel = 0;
	unsigned int smallest = 0;
	unsigned int distance = 0;
	int rd, gd, bd;
	uint8_t rp, gp, bp;

	smallest = ~0;

	for (i = 0; i < len; ++i) {
		bp = (pal[i] & 0xff000000) >> 24;
		gp = (pal[i] & 0x00ff0000) >> 16;
		rp = (pal[i] & 0x0000ff00) >> 8;

		rd = rp - r;
		gd = gp - g;
		bd = bp - b;

		distance = (rd * rd) + (gd * gd) + (bd * bd);
		if (distance < smallest) {
			pixel = i;

			/* Perfect match! */
			if (distance == 0)
				break;

			smallest = distance;
		}
	}

	return pixel;
}

static void set_xy_coord(int video_width, int video_height,
			 struct watermark_coord_info *coord_info)
{
	if (coord_info == NULL)
		return;

	if (video_width >= 3840) {
		/* 4K */
		coord_info->logo_bmp.x = WATERMARK4K_IMG_X;
		coord_info->time_bmp.y = WATERMARK_TIME_Y;
	} else if (video_width >= 2560 && video_width < 3840) {
		/* 2K */
		coord_info->logo_bmp.x = WATERMARK1440p_IMG_X;
		coord_info->time_bmp.y = WATERMARK_TIME_Y;
	} else if (video_width >= 1920 && video_width < 2560) {
		/* 1080P */
		coord_info->logo_bmp.x = WATERMARK1080p_IMG_X;
		coord_info->time_bmp.y = WATERMARK_TIME_Y;
	} else if (video_width >= 1280 && video_width < 1920) {
		/* 720P */
		coord_info->logo_bmp.x = WATERMARK720p_IMG_X;
		coord_info->time_bmp.y = WATERMARK_TIME_Y;
	} else if (video_width >= 640 && video_width < 1280) {
		coord_info->logo_bmp.x = WATERMARK480p_IMG_X;
		coord_info->time_bmp.y = WATERMARK_TIME_Y / 2;
	} else if (video_width >= 480 && video_width < 640) {
		coord_info->logo_bmp.x = WATERMARK360p_IMG_X;
		coord_info->time_bmp.y = WATERMARK_TIME_Y / 2;
	} else if (video_width >= 320 && video_width < 480) {
		coord_info->logo_bmp.x = WATERMARK240p_IMG_X;
		coord_info->time_bmp.y = WATERMARK_TIME_Y / 2;
	} else {
		return;
	}

	coord_info->time_bmp.x = coord_info->logo_bmp.x -
				 (coord_info->time_bmp.width - coord_info->logo_bmp.width);

	coord_info->logo_bmp.y = coord_info->time_bmp.y + coord_info->time_bmp.height;

	coord_info->license_bmp.x = coord_info->logo_bmp.x -
				    (coord_info->license_bmp.width - coord_info->logo_bmp.width);

	coord_info->license_bmp.y = coord_info->logo_bmp.y +
				    coord_info->logo_bmp.height;
}

static void set_image_transparent_color(struct watermark_color_info *color_info,
					uint32_t enable, uint16_t rgb565_pixel)
{
	color_info->set_image_transparent = enable;
	color_info->transparent_color = rgb565_pixel;
}

static void set_text_color(struct watermark_color_info *color_info,
			   uint16_t text_color, uint16_t text_border_color)
{
	color_info->text_color = text_color;
	color_info->text_border_color = text_border_color;
}

static void get_logo_data(uint8_t *dst, uint32_t logo_index,
			  struct watermark_rect rect, uint32_t flag, uint16_t rgb565_pixel)
{
	int i, j, index;
	pixel_t pixel = 0;
	struct rgb_t rgb;

	memset(&rgb, 0, sizeof(struct rgb_t));

	if (dst == NULL) {
		printf("%s error, dst buf is NULL\n", __func__);
		return;
	}

	if (watermark_bmap[logo_index].bmBits == NULL) {
		printf("%s: watermark_bmap is NULL, bmpIndex = %d\n", __func__, logo_index);

		return;
	}

	for (j = 0; j < watermark_bmap[logo_index].bmHeight; j++) {
		for (i = 0; i < watermark_bmap[logo_index].bmWidth; i++) {
			pixel = get_pixel_in_bitmap(&watermark_bmap[logo_index], i, j, NULL);
			index = i + j * rect.width;

			if (flag && ((uint16_t)pixel == rgb565_pixel)) {
				dst[index] = PALETTE_TABLE_LEN - 1;
				continue;
			}

			pixel_to_rgbas(&pixel, &rgb, 1);

			if (rgb.a == 0)
				dst[index] = PALETTE_TABLE_LEN - 1;
			else
				dst[index] = find_color(rgb888_palette_table,
							PALETTE_TABLE_LEN,
							rgb.r, rgb.g, rgb.b);
		}
	}
}

static uint8_t get_color_index(uint16_t color)
{
	struct rgb_t rgb;

	memset(&rgb, 0, sizeof(struct rgb_t));

	rgb.r = (color >> 8) & 0x00f8;
	rgb.g = (color >> 3) & 0x00fc;
	rgb.b = (color << 3) & 0x00f8;

	return find_color(rgb888_palette_table, PALETTE_TABLE_LEN, rgb.r,
			  rgb.g, rgb.b);
}

static void get_rect_bmp_data(uint8_t *dst, struct bitmap *bmp,
			      struct watermark_color_info color_info,
			      struct watermark_rect rect)
{
	int i, j, index;
	pixel_t pixel = 0;
	uint8_t text_index = get_color_index(color_info.text_color);;

#ifdef DOUBLE_COLOR_FONTS
	uint8 text_border_index = get_color_index(color_info.text_border_color);
#endif

	if (dst == NULL) {
		printf("%s error, dst buf is NULL\n", __func__);
		return;
	}

	for (j = 0; j < bmp->bmHeight; j++) {
		for (i = 0; i < bmp->bmWidth; i++) {
			pixel = get_pixel_in_bitmap(bmp, i, j, NULL);
			index = i + j * rect.width;

			if ((uint16_t)pixel == color_info.text_color)
				dst[index] = text_index;
#ifdef DOUBLE_COLOR_FONTS
			else if ((uint16_t)pixel == color_info.text_border_color)
				dst[index] = text_border_index;
#endif
			else
				dst[index] = PALETTE_TABLE_LEN - 1;
		}
	}
}

static int one_glyph_to_bitmap(uint32_t glyph, uint32_t index,
			       struct bitmap *bmp,
			       struct watermark_color_info color_info)
{
	int i, j, l;
	unsigned char *dst, *dst_line;
	uint16_t text_color = color_info.text_color;

#ifdef DOUBLE_COLOR_FONTS
	uint16_t border_color = color_info.text_border_color;
#endif

	/* Add 34 Chinese charactors */
	if (glyph >= SUPORT_CHAR_NUM + 35 || glyph < 0) {
		printf("%s: input number error\n", __func__);
		return -1;
	}

	if (glyph < SUPORT_CHAR_NUM) {
		dst_line = bmp->bmBits + (index * WMK_FONTX * bmp->bmBytesPerPixel);
	} else {
		dst_line = bmp->bmBits + (index * WMK_CHINESE_FONTX * bmp->bmBytesPerPixel);
	}

#ifdef DOUBLE_COLOR_FONTS
	for (j = 0; j < WMK_FONTY; j++) {
		dst = dst_line;
		for (i = 0; i < WMK_FONTXD8; i++) {
			for (l = 0; l < 8; l++) {
				if (wmk_glyph[glyph][j][i] >> l & 0x01)
					dst = set_pixel(dst, bmp->bmBytesPerPixel, border_color);
				else
					dst = set_pixel(dst, bmp->bmBytesPerPixel, text_color);
			}
		}

		dst_line += bmp->bmPitch;
	}
#else
	for (j = 0; j < WMK_FONTY; j++) {
		dst = dst_line;

		if (glyph < SUPORT_CHAR_NUM) {
			for (i = 0; i < WMK_FONTXD8; i++) {
				for (l = 0; l < 8; l++) {
					if (wmk_glyph[glyph][j][i] >> l & 0x01)
						dst = set_pixel(dst, bmp->bmBytesPerPixel, text_color);
					else
						dst = set_pixel(dst, bmp->bmBytesPerPixel, ~text_color);
				}
			}
		} else {
			for (i = 0; i < WMK_CHINESE_FONTXD8; i++) {
				for (l = 0; l < 8; l++) {
					if (wmk_CHINESE_glyph[glyph - SUPORT_CHAR_NUM][j][i] >> l & 0x01)
						dst = set_pixel(dst, bmp->bmBytesPerPixel, text_color);
					else
						dst = set_pixel(dst, bmp->bmBytesPerPixel, ~text_color);
				}
			}
		}

		dst_line += bmp->bmPitch;
	}
#endif

	return 0;
}

static int glyph_to_bitmap(uint32_t *src, uint32_t src_len,
			   struct bitmap *bmp,
			   struct watermark_color_info color_info)
{
	int i;
	uint32_t size;
	uint32_t w = WMK_FONTX * src_len;
	uint32_t h = WMK_FONTY;

	bmp->bmWidth = w;
	bmp->bmHeight = h;
	bmp->bmType = WMK_BMP_TYPE_NORMAL;
	bmp->bmBitsPerPixel = BITS_PER_PIXEL;
	bmp->bmBytesPerPixel = BYTES_PER_PIXEL;
	bmp->bmAlphaMask = NULL;
	bmp->bmAlphaPitch = 0;
	bmp->bmColorKey = 0;
	bmp->bmAlpha = 0;

	size = get_box_size(BYTES_PER_PIXEL, BITS_PER_PIXEL, w, h, &bmp->bmPitch);
	bmp->bmBits = calloc(1, size);
	if (!bmp->bmBits) {
		printf("%s: calloc error\n", __func__);
		goto cleanup_and_ret;
	}

	for (i = 0; i < src_len; i++) {
		if (one_glyph_to_bitmap(src[i], i, bmp, color_info) < 0)
			goto cleanup_and_ret;

		/* Chinese charactor has 2 Bytes and 16 * 2 Bytes glphy datas */
		if (src[i] >= SUPORT_CHAR_NUM)
			i += 1;
	}

	return 0;

cleanup_and_ret:
	delete_bitmap_alpha_pixel(bmp);
	if (bmp->bmBits) {
		free(bmp->bmBits);
		bmp->bmBits = NULL;
	}

	return -1;
}

static void set_osd_region(MppEncOSDRegion *region, uint32_t data_offset,
			   struct watermark_rect rect)
{
	region->enable = 1;
	region->inverse = 0;
	region->start_mb_x = rect.x / 16;
	region->start_mb_y = rect.y / 16;
	region->num_mb_x = rect.width / 16;
	region->num_mb_y = rect.height / 16;
	region->buf_offset = data_offset;
}

static void mpp_osd_data_init(struct watermark_info *watermark)
{
	int i;
	int num_region = 0;

	for (i = 0; i < BUFFER_NUM; i++) {
		watermark->data_buffer[i].vir_addr
			= (uint8_t *)watermark->watermark_data[i].buffer;
		watermark->data_buffer[i].phy_fd
			= watermark->watermark_data[i].fd;
		watermark->data_buffer[i].handle
			= (void *)(&watermark->watermark_data[i].handle);
		watermark->data_buffer[i].buf_size
			= watermark->watermark_data[i].size;
		watermark->mpp_osd_data[i].buf
			= (MppBuffer)(&watermark->data_buffer[i]);

		/* show time */
		if (watermark->type & WATERMARK_TIME) {
			num_region = watermark->mpp_osd_data[i].num_region;

			set_osd_region(&watermark->mpp_osd_data[i].region[num_region],
				       watermark->osd_data_offset.time_data_offset,
				       watermark->coord_info.time_bmp);

			watermark->mpp_osd_data[i].num_region++;
		}

		/* show image */
		if (watermark->type & WATERMARK_LOGO) {
			num_region = watermark->mpp_osd_data[i].num_region;

			set_osd_region(&watermark->mpp_osd_data[i].region[num_region],
				       watermark->osd_data_offset.logo_data_offset,
				       watermark->coord_info.logo_bmp);

			watermark->mpp_osd_data[i].num_region++;
		}

		/* show model */
		if (watermark->type & WATERMARK_MODEL) {
			num_region = watermark->mpp_osd_data[i].num_region;

			set_osd_region(&watermark->mpp_osd_data[i].region[num_region],
				       watermark->osd_data_offset.model_data_offset,
				       watermark->coord_info.model_bmp);

			watermark->mpp_osd_data[i].num_region++;
		}

		/* show speed */
		if (watermark->type & WATERMARK_SPEED) {
			num_region = watermark->mpp_osd_data[i].num_region;

			set_osd_region(&watermark->mpp_osd_data[i].region[num_region],
				       watermark->osd_data_offset.speed_data_offset,
				       watermark->coord_info.speed_bmp);

			watermark->mpp_osd_data[i].num_region++;
		}

		/* Show license plate */
		if (watermark->type & WATERMARK_LICENSE) {
			num_region = watermark->mpp_osd_data[i].num_region;

			set_osd_region(&watermark->mpp_osd_data[i].region[num_region],
				       watermark->osd_data_offset.license_data_offset,
				       watermark->coord_info.license_bmp);

			watermark->mpp_osd_data[i].num_region++;
		}
	}

	watermark->osd_num_region = watermark->mpp_osd_data[0].num_region;
}

static void coord_info_copy(struct watermark_rect *dst_bmp,
			    struct watermark_rect *src_bmp)
{
	dst_bmp->x = src_bmp->x;
	dst_bmp->y = src_bmp->y;
	dst_bmp->w = src_bmp->w;
	dst_bmp->h = src_bmp->h;
	dst_bmp->width = src_bmp->width;
	dst_bmp->height = src_bmp->height;
}

static int cross_border_judge(int video_width, int video_height,
			      struct watermark_rect rect)
{
	if ((rect.x < 0) || (rect.y < 0)
	    || (rect.x + rect.width > video_width)
	    || (rect.y + rect.height > video_height))
		return -1;

	return 0;
}

static void get_time_data(struct watermark_info *watermark)
{
	time_t ltime;
	struct tm *today;
	uint32_t show_time[MAX_TIME_LEN] = { 0 };
	uint32_t index = watermark->buffer_index;
	uint32_t offset = watermark->osd_data_offset.time_data_offset;
	uint8_t *dst = (uint8_t *) watermark->watermark_data[index].buffer + offset;

	time(&ltime);
	today = localtime(&ltime);

	watermark_get_show_time(show_time, today, MAX_TIME_LEN);
	watermark_update_rect_bmp(show_time, MAX_TIME_LEN,
				  watermark->coord_info.time_bmp,
				  dst, watermark->color_info);
}

static void watermark_data_init(struct watermark_info *watermark)
{
	int i;

	if (watermark->type & WATERMARK_TIME)
		get_time_data(watermark);

	if (watermark->type & WATERMARK_LOGO) {
		set_image_transparent_color(&watermark->color_info, 1, 0x0800);

		for (i = 0; i < BUFFER_NUM; i++) {
			get_logo_data((uint8_t *)watermark->watermark_data[i].buffer +
				      watermark->osd_data_offset.logo_data_offset,
				      watermark->logo_index,
				      watermark->coord_info.logo_bmp,
				      watermark->color_info.set_image_transparent,
				      watermark->color_info.transparent_color);
		}
	}

	if ((watermark->type & WATERMARK_LICENSE) &&
	    parameter_get_licence_plate_flag()) {
		uint32_t license_len;
		uint32_t show_license[MAX_TIME_LEN] = { 0 };
		char licenseplatestr[20] = { 0 };

		if (!is_licenseplate_valid((char *)parameter_get_licence_plate()))
			return;

		watermart_get_licenseplate_str((char *)parameter_get_licence_plate(),
					       MAX_LICN_NUM, (char *)licenseplatestr);
		watermark_get_show_license(show_license, (char *)licenseplatestr, &license_len);
		watermart_get_license_data(watermark, show_license, license_len);
	}
}

void watermart_get_license_data(struct watermark_info *watermark, uint32_t *src,
				uint32_t srclen)
{
	uint32_t i;
	uint32_t offset = watermark->osd_data_offset.license_data_offset;

	for (i = 0; i < BUFFER_NUM; i++) {
		uint8_t *dst = (uint8_t *)watermark->watermark_data[i].buffer
			       + offset;

		watermark_update_rect_bmp(src, srclen, watermark->coord_info.license_bmp,
					  dst, watermark->color_info);
	}
}

void watermark_update_osd_num_region(int onoff,
				     struct watermark_info *watermark)
{
	int i;

	if (!watermark) {
		printf("%s: error, watermark is NULL\n", __func__);
		return;
	}

	for (i = 0; i < BUFFER_NUM; i++) {
		if (onoff)
			watermark->mpp_osd_data[i].num_region = watermark->osd_num_region;
		else
			watermark->mpp_osd_data[i].num_region = 0;
	}
}

void watermark_init(struct watermark_info *watermark)
{
	uint32_t i, len;
	struct osd_data_offset_info *osd_data_offset = NULL;

	uint32_t time_bmp_width = watermark->coord_info.time_bmp.width;
	uint32_t time_bmp_height = watermark->coord_info.time_bmp.height;

	uint32_t logo_bmp_width = watermark->coord_info.logo_bmp.width;
	uint32_t logo_bmp_height = watermark->coord_info.logo_bmp.height;

	uint32_t model_bmp_width = watermark->coord_info.model_bmp.width;
	uint32_t model_bmp_height = watermark->coord_info.model_bmp.height;

	uint32_t speed_bmp_width = watermark->coord_info.speed_bmp.width;
	uint32_t speed_bmp_height = watermark->coord_info.speed_bmp.height;

	uint32_t license_bmp_width = watermark->coord_info.license_bmp.width;
	uint32_t license_bmp_height = watermark->coord_info.license_bmp.height;

	len = time_bmp_width * time_bmp_height + logo_bmp_width * logo_bmp_height
	      + model_bmp_width * model_bmp_height + speed_bmp_width * speed_bmp_height
	      + license_bmp_width * license_bmp_height;

	osd_data_offset = &watermark->osd_data_offset;
	osd_data_offset->time_data_offset = 0;
	osd_data_offset->logo_data_offset = time_bmp_width * time_bmp_height;
	osd_data_offset->model_data_offset = logo_bmp_width * logo_bmp_height
					     + osd_data_offset->logo_data_offset;
	osd_data_offset->speed_data_offset = model_bmp_width * model_bmp_height
					     + osd_data_offset->model_data_offset;
	osd_data_offset->license_data_offset = speed_bmp_width * speed_bmp_height
					       + osd_data_offset->speed_data_offset;

	for (i = 0; i < BUFFER_NUM; i++) {
		if (video_ion_alloc_rational(&watermark->watermark_data[i],
					     len / 16, 16, 1, 1) == -1) {
			printf("%s alloc watermark_data err, no memory!\n", __func__);
			memset(watermark, 0, sizeof(struct watermark_info));
			return;
		}

		memset(watermark->watermark_data[i].buffer, PALETTE_TABLE_LEN - 1,
		       watermark->watermark_data[i].size);
	}

	mpp_osd_data_init(watermark);

	set_text_color(&watermark->color_info, 0xffff, 0x0000);

	watermark_data_init(watermark);

	if (!parameter_get_video_mark())
		watermark_update_osd_num_region(0, watermark);
}

void watermark_deinit(struct watermark_info *watermark)
{
	int i;

	if (!watermark) {
		printf("%s: error, watermark is NULL\n", __func__);
		return;
	}

	for (i = 0; i < BUFFER_NUM; i++) {
		if (watermark->watermark_data[i].buffer) {
			video_ion_free(&watermark->watermark_data[i]);
			watermark->watermark_data[i].buffer = NULL;
		}
	}
}

int watermark_set_parameters(int video_width, int video_height,
			     enum watermark_logo_index logo_index, int type,
			     struct watermark_coord_info coord_info,
			     struct watermark_info *watermark)
{
	int i;

	memset(watermark, 0, sizeof(struct watermark_info));
	for (i = 0; i < BUFFER_NUM; i++) {
		watermark->watermark_data[i].fd = -1;
		watermark->watermark_data[i].client = -1;
	}

	if (type <= 0) {
		printf("%s: please set type, type = %d\n", __func__, type);
		return -1;
	}

	if (cross_border_judge(video_width, video_height, coord_info.time_bmp) < 0) {
		printf("%s: time coordinate cross-border\n", __func__);
		return -1;
	}

	if (cross_border_judge(video_width, video_height, coord_info.logo_bmp) < 0) {
		printf("%s: logo coordinate cross-border\n", __func__);
		return -1;
	}

	if (cross_border_judge(video_width, video_height, coord_info.model_bmp) < 0) {
		printf("%s: model coordinate cross-border\n", __func__);
		return -1;
	}

	if (cross_border_judge(video_width, video_height, coord_info.speed_bmp) < 0) {
		printf("%s: speed coordinate cross-border\n", __func__);
		return -1;
	}

	if (cross_border_judge(video_width, video_height, coord_info.license_bmp) < 0) {
		printf("%s: license coordinate cross-border\n", __func__);
		return -1;
	}

	watermark->type = type;
	watermark->logo_index = logo_index;

	if (type & WATERMARK_TIME)
		coord_info_copy(&watermark->coord_info.time_bmp, &coord_info.time_bmp);

	if (type & WATERMARK_LOGO)
		coord_info_copy(&watermark->coord_info.logo_bmp, &coord_info.logo_bmp);

	if (type & WATERMARK_MODEL)
		coord_info_copy(&watermark->coord_info.model_bmp, &coord_info.model_bmp);

	if (type & WATERMARK_SPEED)
		coord_info_copy(&watermark->coord_info.speed_bmp, &coord_info.speed_bmp);

	if (type & WATERMARK_LICENSE)
		coord_info_copy(&watermark->coord_info.license_bmp, &coord_info.license_bmp);

	return 0;
}

int watermark_config(int video_width, int video_height,
		     struct watermark_info *watermark)
{
	enum watermark_logo_index logo_index = -1;
	int type = WATERMARK_TIME | WATERMARK_LOGO | WATERMARK_LICENSE;

	struct watermark_coord_info coord_info;
	memset(&coord_info, 0, sizeof(struct watermark_coord_info));

	printf("###video_width = %d video_height = %d\n", video_width, video_height);

	if (video_width >= 3840) {
		/* 4K */
		logo_index = LOGO4K;
		coord_info.logo_bmp.w = WATERMARK4K_IMG_W;
		coord_info.logo_bmp.h = WATERMARK4K_IMG_H;

		coord_info.time_bmp.w = WATERMARK_TIME_W * 4;
		coord_info.time_bmp.h = WATERMARK_TIME_H * 4;

		coord_info.license_bmp.w = WATERMARK_LICN_W * 4;
		coord_info.license_bmp.h = WATERMARK_LICN_H * 4;
	} else if (video_width >= 2560 && video_width < 3840) {
		/* 2K */
		logo_index = LOGO1440P;
		coord_info.logo_bmp.w = WATERMARK1440p_IMG_W;
		coord_info.logo_bmp.h = WATERMARK1440p_IMG_H;

		coord_info.time_bmp.w = WATERMARK_TIME_W * 3;
		coord_info.time_bmp.h = WATERMARK_TIME_H * 3;

		coord_info.license_bmp.w = WATERMARK_LICN_W * 3;
		coord_info.license_bmp.h = WATERMARK_LICN_H * 3;
	} else if (video_width >= 1920 && video_width < 2560) {
		/* 1080P */
		logo_index = LOGO1080P;
		coord_info.logo_bmp.w = WATERMARK1080p_IMG_W;
		coord_info.logo_bmp.h = WATERMARK1080p_IMG_H;

		coord_info.time_bmp.w = WATERMARK_TIME_W * 2;
		coord_info.time_bmp.h = WATERMARK_TIME_H * 2;

		coord_info.license_bmp.w = WATERMARK_LICN_W * 2;
		coord_info.license_bmp.h = WATERMARK_LICN_H * 2;
	} else if (video_width >= 1280 && video_width < 1920) {
		/* 720P */
		logo_index = LOGO720P;
		coord_info.logo_bmp.w = WATERMARK720p_IMG_W;
		coord_info.logo_bmp.h = WATERMARK720p_IMG_H;

		coord_info.time_bmp.w = WATERMARK_TIME_W * 3 / 2;
		coord_info.time_bmp.h = WATERMARK_TIME_H * 3 / 2;

		coord_info.license_bmp.w = WATERMARK_LICN_W * 3 / 2;
		coord_info.license_bmp.h = WATERMARK_LICN_H * 3 / 2;
	} else if (video_width >= 640 && video_width < 1280) {
		logo_index = LOGO480P;
		coord_info.logo_bmp.w = WATERMARK480p_IMG_W;
		coord_info.logo_bmp.h = WATERMARK480p_IMG_H;

		coord_info.time_bmp.w = WATERMARK_TIME_W;
		coord_info.time_bmp.h = WATERMARK_TIME_H;

		coord_info.license_bmp.w = WATERMARK_LICN_W;
		coord_info.license_bmp.h = WATERMARK_LICN_H;
	} else if (video_width >= 480 && video_width < 640) {
		logo_index = LOGO360P;
		coord_info.logo_bmp.w = WATERMARK360p_IMG_W;
		coord_info.logo_bmp.h = WATERMARK360p_IMG_H;

		coord_info.time_bmp.w = WATERMARK_TIME_W;
		coord_info.time_bmp.h = WATERMARK_TIME_H;

		coord_info.license_bmp.w = WATERMARK_LICN_W;
		coord_info.license_bmp.h = WATERMARK_LICN_H;
	} else if (video_width >= 320 && video_width < 480) {
		logo_index = LOGO240P;
		coord_info.logo_bmp.w = WATERMARK240p_IMG_W;
		coord_info.logo_bmp.h = WATERMARK240p_IMG_H;

		coord_info.time_bmp.w = WATERMARK_TIME_W;
		coord_info.time_bmp.h = WATERMARK_TIME_H;

		coord_info.license_bmp.w = WATERMARK_LICN_W;
		coord_info.license_bmp.h = WATERMARK_LICN_H;
	} else {
		return -1;
	}

	coord_info.logo_bmp.width = ALIGN(coord_info.logo_bmp.w, 16);
	coord_info.logo_bmp.height = ALIGN(coord_info.logo_bmp.h, 16);

	coord_info.time_bmp.width = ALIGN(coord_info.time_bmp.w, 16);
	coord_info.time_bmp.height = ALIGN(coord_info.time_bmp.h, 16);

	coord_info.license_bmp.width = ALIGN(coord_info.license_bmp.w, 16);
	coord_info.license_bmp.height = ALIGN(coord_info.license_bmp.h, 16);

	set_xy_coord(video_width, video_height, &coord_info);

	return watermark_set_parameters(video_width, video_height, logo_index, type,
					coord_info, watermark);
}

void watermark_refresh(int video_width, int video_height,
		       struct watermark_info *watermark)
{
	int i;

	if (watermark == NULL)
		return;

	for (i = 0; i < BUFFER_NUM; i++) {
		memset(watermark->watermark_data[i].buffer, PALETTE_TABLE_LEN - 1,
		       watermark->watermark_data[i].size);

		watermark->mpp_osd_data[i].num_region = 0;
	}

	set_xy_coord(video_width, video_height, &watermark->coord_info);
	mpp_osd_data_init(watermark);
	watermark_data_init(watermark);
}

void watermark_get_show_time(uint32_t *show_time, struct tm *time_now,
			     uint32_t len)
{
	if (len < MAX_TIME_LEN) {
		printf("%s: the length of the time-buffer is too small\n", __func__);
		return;
	}

	/* 2106-12-23 18:14:00 */
	show_time[0] = (time_now->tm_year + 1900) / 1000;
	show_time[1] = (time_now->tm_year + 1900) % 1000 / 100;
	show_time[2] = (time_now->tm_year + 1900) % 100 / 10;
	show_time[3] = (time_now->tm_year + 1900) % 10;
	show_time[4] = 10;
	show_time[5] = (time_now->tm_mon + 1) / 10;
	show_time[6] = (time_now->tm_mon + 1) % 10;
	show_time[7] = 10;
	show_time[8] = time_now->tm_mday / 10;
	show_time[9] = time_now->tm_mday % 10;
	show_time[10] = 12;
	show_time[11] = time_now->tm_hour / 10;
	show_time[12] = time_now->tm_hour % 10;
	show_time[13] = 11;
	show_time[14] = time_now->tm_min / 10;
	show_time[15] = time_now->tm_min % 10;
	show_time[16] = 11;
	show_time[17] = time_now->tm_sec / 10;
	show_time[18] = time_now->tm_sec % 10;
}

extern uint32_t licplate_pos[8];
void watermark_get_show_license(uint32_t *show_license, char *licenseStr,
				uint32_t *len)
{
	uint32_t i, license_len;
	license_len = strlen(licenseStr);

	if (licenseStr == NULL || show_license == NULL)
		return;

	for (i = 0; i < license_len;) {
		/* Chinese charactor */
		if (licenseStr[i] > 0x80) {
			show_license[i]   = SUPORT_CHAR_NUM + licplate_pos[0] - 1;
			show_license[i + 1] = SUPORT_CHAR_NUM + licplate_pos[0] - 1;
			i += 2;
		} else { /* ASCII charactor */
			if ((licenseStr[i] >= '0') && (licenseStr[i] <= '9'))
				show_license[i] = licenseStr[i] - '0';
			else if ((licenseStr[i] >= 'a') && (licenseStr[i] <= 'z'))
				show_license[i] = a_POS + (licenseStr[i] - 'a');
			else if ((licenseStr[i] >= 'A') && (licenseStr[i] <= 'Z'))
				show_license[i] = A_POS + (licenseStr[i] - 'A');
			else {
				if (licenseStr[i] == ' ')
					show_license[i] = 12;
				else if (licenseStr[i] == ':')
					show_license[i] = 11;
				else if (licenseStr[i] == '-')
					show_license[i] = 10;
				else if (licenseStr[i] == '*')
					show_license[i] = 66;
				else
					printf("/////// not find %d [%c] glphy data.Pls check it.//////\n", i,
					       licenseStr[i]);
			}

			i++;
		}
	}

	if (i > license_len) {
		*len = 0;
		return;
	}

	if (i == license_len)
		*len = i;
}

void watermark_update_rect_bmp(uint32_t *src, uint32_t src_len,
			       struct watermark_rect dst_rect,
			       uint8_t *dst, struct watermark_color_info color_info)
{
	uint32_t image_size, pitch;
	struct bitmap bmp, scaled_bmp;

	if (src_len <= 0) {
		printf("%s: update src len = %d, error\n", __func__, src_len);
		return;
	}

	memset(&bmp, 0, sizeof(struct bitmap));
	memset(&scaled_bmp, 0, sizeof(struct bitmap));

	if (glyph_to_bitmap(src, src_len, &bmp, color_info) < 0) {
		printf("%s: String2Bmp error\n", __func__);
		goto free_ret;
	}

	if ((bmp.bmWidth != dst_rect.w) || (bmp.bmHeight != dst_rect.h)) {
		image_size = get_box_size(BYTES_PER_PIXEL, BITS_PER_PIXEL, dst_rect.w,
					  dst_rect.h, &pitch);

		if (image_size <= 0) {
			printf("%s: get_box_size error, imagesize: %d\n", __func__, image_size);
			goto free_ret;
		}

		scaled_bmp.bmType = bmp.bmType;
		scaled_bmp.bmBytesPerPixel = bmp.bmBytesPerPixel;
		scaled_bmp.bmWidth = dst_rect.w;
		scaled_bmp.bmHeight = dst_rect.h;
		scaled_bmp.bmPitch = pitch;
		scaled_bmp.bmBits = calloc(1, image_size);

		if (scaled_bmp.bmBits == NULL) {
			printf("%s: calloc error\n", __func__);
			goto free_ret;
		}

		scale_bitmap(&scaled_bmp, &bmp);
		get_rect_bmp_data(dst, &scaled_bmp, color_info, dst_rect);
	} else {
		get_rect_bmp_data(dst, &bmp, color_info, dst_rect);
	}

free_ret:
	if (bmp.bmBits != NULL) {
		free(bmp.bmBits);
		bmp.bmBits = NULL;
	}

	if (bmp.bmAlphaMask != NULL) {
		free(bmp.bmAlphaMask);
		bmp.bmAlphaMask = NULL;
	}

	if (scaled_bmp.bmBits != NULL) {
		free(scaled_bmp.bmBits);
		scaled_bmp.bmBits = NULL;
	}

	if (scaled_bmp.bmAlphaMask != NULL) {
		free(scaled_bmp.bmAlphaMask);
		scaled_bmp.bmAlphaMask = NULL;
	}
}

void watermart_get_licenseplate_str(char *lic_plate, int len,
				    char *licplate_str)
{
	int i;

	if (licplate_str == NULL)
		return;

	if (parameter_get_licence_plate_flag() == 1) {
		for (i = 0; i < len; i++)
			strcat(licplate_str, (const char *)(lic_plate + i * ONE_CHN_SIZE));
	}
}

static void draw_on_nv12(uint8_t* dst_buf,
			 uint32_t dst_width,
			 uint32_t dst_height,
			 uint8_t* src_buf,
			 uint32_t src_width,
			 uint32_t src_height,
			 uint32_t x_pos,
			 uint32_t y_pos)
{
	uint32_t i, j;
	uint8_t index;
	uint8_t *y_addr = NULL, *uv_addr = NULL;
	uint8_t *start_y = NULL, *start_uv = NULL;

	if ((x_pos + src_width) >= dst_width || y_pos < 0 ||
	    (y_pos + src_height) >= dst_height) {
		printf("%s error input number or position.\n", __func__);
		return;
	}

	y_addr = dst_buf + y_pos * dst_width + x_pos;
	uv_addr = dst_buf + dst_width * dst_height + y_pos * dst_width / 2 + x_pos;

	for (j = 0; j < src_height; j++) {
		start_y = y_addr + j * dst_width;
		start_uv = uv_addr + j * dst_width / 2;

		for (i = 0; i < src_width; i++) {
			index = src_buf[i + j * src_width];
			if (((yuv444_palette_table[index] & 0xff000000) >> 24) == 0xff) {
				*start_y = yuv444_palette_table[index] & 0x000000ff;

				if ((j % 2 == 0) && (i % 2 == 0)) {
					*start_uv = (yuv444_palette_table[index] & 0x0000ff00) >> 8;
					*(start_uv + 1) = (yuv444_palette_table[index] & 0x00ff0000) >> 16;
				}
			}

			start_y++;
			if ((j % 2 == 0) && (i % 2 == 0))
				start_uv += 2;
		}
	}
}

void watermark_draw_on_nv12(struct watermark_info *watermark, uint8_t *dst_buf,
			    int dst_w, int dst_h, char show_license)
{
	uint32_t buffer_index = watermark->buffer_index;
	uint8_t *watermark_data =
		(uint8_t*)watermark->watermark_data[buffer_index].buffer;

	if (!watermark_data)
		return;

	// show time
	if (watermark->type & WATERMARK_TIME) {
		draw_on_nv12(
		dst_buf, dst_w, dst_h,
		watermark_data + watermark->osd_data_offset.time_data_offset,
		watermark->coord_info.time_bmp.width,
		watermark->coord_info.time_bmp.height,
		watermark->coord_info.time_bmp.x,
		watermark->coord_info.time_bmp.y);
	}

	// show image
	if (watermark->type & WATERMARK_LOGO) {
		draw_on_nv12(
		dst_buf, dst_w, dst_h,
		watermark_data + watermark->osd_data_offset.logo_data_offset,
		watermark->coord_info.logo_bmp.width,
		watermark->coord_info.logo_bmp.height,
		watermark->coord_info.logo_bmp.x,
		watermark->coord_info.logo_bmp.y);
	}

	/* Show license plate */
	if (show_license && watermark->type & WATERMARK_LICENSE) {
		draw_on_nv12(
		dst_buf, dst_w, dst_h,
		watermark_data + watermark->osd_data_offset.license_data_offset,
		watermark->coord_info.license_bmp.width,
		watermark->coord_info.license_bmp.height,
		watermark->coord_info.license_bmp.x,
		watermark->coord_info.license_bmp.y);
	}
}
