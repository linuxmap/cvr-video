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

#include "wmk_bitmap.h"

#define WMK_SIZEOF_RGBQUAD      4

#define WMK_BI_RGB          0
#define WMK_BI_RLE8         1
#define WMK_BI_RLE4         2
#define WMK_BI_BITFIELDS    3

#define WMK_SIZEOF_BMFH     14
#define WMK_SIZEOF_BMIH     40

#define WMK_OS2INFOHEADERSIZE  12
#define WMK_WININFOHEADERSIZE  40

#define WMK_BMP_ERR     0
#define WMK_BMP_LINE    1
#define WMK_BMP_END     2

#define WMK_PIX2BYTES(n)    (((n)+7)/8)

struct bitmap_file_header {
	unsigned short bfType;
	unsigned long  bfSize;
	unsigned short bfReserved1;
	unsigned short bfReserved2;
	unsigned long  bfOffBits;
};

/*
 * Used for both OS/2 and Windows BMP.
 * Contains only the parameters needed to load the image
 */
struct bitmap_info_header {
	unsigned long  biWidth;
	unsigned long  biHeight;
	unsigned short biBitCount;
	unsigned long  biCompression;
};

/* size: 40 */
struct winbmp_info_header {
	unsigned long  biSize;
	unsigned long  biWidth;
	unsigned long  biHeight;
	unsigned short biPlanes;
	unsigned short biBitCount;
	unsigned long  biCompression;
	unsigned long  biSizeImage;
	unsigned long  biXPelsPerMeter;
	unsigned long  biYPelsPerMeter;
	unsigned long  biClrUsed;
	unsigned long  biClrImportant;
};

/* size: 12 */
struct os2bmp_info_header {
	unsigned long  biSize;
	unsigned short biWidth;
	unsigned short biHeight;
	unsigned short biPlanes;
	unsigned short biBitCount;
};

struct wmk_init_info {
	unsigned long  biWidth;
	unsigned long  biHeight;
	unsigned short biBitCount;
	unsigned long  biCompression;
	unsigned long rmask, gmask, bmask;
};

static int read_bmp_file_header(struct mg_rw_ops *f,
				struct bitmap_file_header *fileheader)
{
	fileheader->bfType = wmk_fp_igetw(f);
	fileheader->bfSize = wmk_fp_igetl(f);
	fileheader->bfReserved1 = wmk_fp_igetw(f);
	fileheader->bfReserved2 = wmk_fp_igetw(f);
	fileheader->bfOffBits = wmk_fp_igetl(f);

	if (fileheader->bfType != 19778)
		return -1;

	return 0;
}

static int read_win_bmp_info_header(struct mg_rw_ops *f,
				    struct bitmap_info_header *infoheader)
{
	struct winbmp_info_header win_infoheader;

	win_infoheader.biWidth = wmk_fp_igetl(f);
	win_infoheader.biHeight = wmk_fp_igetl(f);
	win_infoheader.biPlanes = wmk_fp_igetw(f);
	win_infoheader.biBitCount = wmk_fp_igetw(f);
	win_infoheader.biCompression = wmk_fp_igetl(f);
	win_infoheader.biSizeImage = wmk_fp_igetl(f);
	win_infoheader.biXPelsPerMeter = wmk_fp_igetl(f);
	win_infoheader.biYPelsPerMeter = wmk_fp_igetl(f);
	win_infoheader.biClrUsed = wmk_fp_igetl(f);
	win_infoheader.biClrImportant = wmk_fp_igetl(f);


	infoheader->biWidth = win_infoheader.biWidth;
	infoheader->biHeight = win_infoheader.biHeight;
	infoheader->biBitCount = win_infoheader.biBitCount;
	infoheader->biCompression = win_infoheader.biCompression;

	return 0;
}

static void read_bmp_icolors(int ncols, struct rgb_t *pal, struct mg_rw_ops *f,
			     int win_flag)
{
	int i;

	for (i = 0; i < ncols; i++) {
		pal[i].b = wmk_fp_getc(f);
		pal[i].g = wmk_fp_getc(f);
		pal[i].r = wmk_fp_getc(f);
		if (win_flag)
			wmk_fp_getc(f);
	}
}

static int read_os2_bmp_info_header(struct mg_rw_ops *f,
				    struct bitmap_info_header *infoheader)
{
	struct os2bmp_info_header os2_infoheader;

	os2_infoheader.biWidth = wmk_fp_igetw(f);
	os2_infoheader.biHeight = wmk_fp_igetw(f);
	os2_infoheader.biPlanes = wmk_fp_igetw(f);
	os2_infoheader.biBitCount = wmk_fp_igetw(f);

	infoheader->biWidth = os2_infoheader.biWidth;
	infoheader->biHeight = os2_infoheader.biHeight;
	infoheader->biBitCount = os2_infoheader.biBitCount;
	infoheader->biCompression = 0;

	return 0;
}

static int bmp_compute_pitch(int bpp, uint32_t width, uint32_t *pitch,
			     BOOL does_round)
{
	int linesize;
	int bytespp = 1;

	if (bpp == 1)
		linesize = WMK_PIX2BYTES(width);
	else if (bpp <= 4)
		linesize = WMK_PIX2BYTES(width << 2);
	else if (bpp <= 8)
		linesize = width;
	else if (bpp <= 16) {
		linesize = width * 2;
		bytespp = 2;
	} else if (bpp <= 24) {
		linesize = width * 3;
		bytespp = 3;
	} else {
		linesize = width * 4;
		bytespp = 4;
	}

	/* rows are DWORD right aligned */
	if (does_round)
		*pitch = (linesize + 3) & -4;
	else
		*pitch = linesize;
	return bytespp;
}

static void wmk_read_16bit_image(struct mg_rw_ops *f, unsigned char *bits,
				 int pitch, int width, unsigned long gmask)
{
	int i;
	unsigned short pixel;
	unsigned char *line;

	line = bits;
	for (i = 0; i < width; i++) {
		pixel = wmk_fp_igetw(f);
		if (gmask == 0x03e0) {
			/* 5-5-5 */
			line [2] = ((pixel >> 10) & 0x1f) << 3;
			line [1] = ((pixel >> 5) & 0x1f) << 3;
			line [0] = (pixel & 0x1f) << 3;
		} else {
			/* 5-6-5 */
			line [2] = ((pixel >> 11) & 0x1f) << 3;
			line [1] = ((pixel >> 5) & 0x3f) << 2;
			line [0] = (pixel & 0x1f) << 3;
		}

		line += 3;
	}

	/* read the gap */
	if (width & 0x01)
		pixel = wmk_fp_igetw(f);

}

static int wmk_read_rle4_compressed_image(struct mg_rw_ops *f,
		unsigned char *bits, int pitch, int width)
{
	unsigned char b[8];
	unsigned char count;
	unsigned short val0, val;
	int j, k, pos = 0, flag = WMK_BMP_ERR;

	while (pos <= width && flag == WMK_BMP_ERR) {
		count = wmk_fp_getc(f);
		val = wmk_fp_getc(f);

		/* repeat pixels count times */
		if (count > 0) {
			b[1] = val & 15;
			b[0] = (val >> 4) & 15;
			for (j = 0; j < count; j++) {
				if (pos % 2 == 0)
					bits[pos / 2] = b[0] << 4;
				else
					bits[pos / 2] = bits[pos / 2] | b[1];
				if (pos >= width) return flag;
				pos++;
			}
		} else {
			switch (val) {
			case 0:
				/* end of line */
				flag = WMK_BMP_LINE;
				break;

			case 1:
				/* end of picture */
				flag = WMK_BMP_END;
				break;

			case 2:
				/* displace image */
				count = wmk_fp_getc(f);
				val = wmk_fp_getc(f);
				pos += count;
				break;

			default:
				/* read in absolute mode */
				for (j = 0; j < val; j++) {
					if ((j % 4) == 0) {
						val0 = wmk_fp_igetw(f);
						for (k = 0; k < 2; k++) {
							b[2 * k + 1] = val0 & 15;
							val0 = val0 >> 4;
							b[2 * k] = val0 & 15;
							val0 = val0 >> 4;
						}
					}

					if (pos % 2 == 0)
						bits [pos / 2] = b[j % 4] << 4;
					else
						bits [pos / 2] = bits [pos / 2] | b[j % 4];
					pos++;
				}
				break;

			}
		}
	}

	return flag;
}

static int wmk_read_rle8_compressed_image(struct mg_rw_ops *f,
		unsigned char *bits, int pitch, int width)
{
	unsigned char count, val, val0;
	int j, pos = 0;
	int flag = WMK_BMP_ERR;

	while (pos <= width && flag == WMK_BMP_ERR) {
		count = wmk_fp_getc(f);
		val = wmk_fp_getc(f);

		if (count > 0) {
			for (j = 0; j < count; j++) {
				bits[pos] = val;
				pos++;
			}
		} else {
			switch (val) {

			case 0:
				/* end of line flag */
				flag = WMK_BMP_LINE;
				break;

			case 1:
				/* end of picture flag */
				flag = WMK_BMP_END;
				break;

			case 2:
				/* displace picture */
				count = wmk_fp_getc(f);
				val = wmk_fp_getc(f);
				pos += count;
				break;

			default:
				/* read in absolute mode */
				for (j = 0; j < val; j++) {
					val0 = wmk_fp_getc(f);
					bits[pos] = val0;
					pos++;
				}

				/* align on word boundary */
				if (j % 2 == 1)
					val0 = wmk_fp_getc(f);

				break;
			}
		}
	}

	return flag;
}

void *init_bmp(struct mg_rw_ops *fp, struct mybitmap *bmp, struct rgb_t *pal)
{
	int effect_depth, biSize;
	int ncol;
	struct bitmap_file_header fileheader;
	struct bitmap_info_header infoheader;
	struct wmk_init_info *bmp_info = NULL;

	bmp_info = (struct wmk_init_info *)calloc(1, sizeof(struct wmk_init_info));
	if (!bmp_info)
		return NULL;

	bmp_info->rmask = 0x001f;
	bmp_info->gmask = 0x03e0;
	bmp_info->bmask = 0x7c00;
	if (read_bmp_file_header(fp, &fileheader) != 0)
		goto err;

	biSize = wmk_fp_igetl(fp);
	if (biSize >= WMK_WININFOHEADERSIZE) {
		if (read_win_bmp_info_header(fp, &infoheader) != 0)
			goto err;

		rw_seek(fp, biSize - WMK_WININFOHEADERSIZE, SEEK_CUR);
		ncol = (fileheader.bfOffBits - biSize - 14) / 4;

		/* there only 1,4,8 bit read color panel data */
		if (infoheader.biBitCount <= 8)
			read_bmp_icolors(ncol, pal, fp, 1);
	} else if (biSize == WMK_OS2INFOHEADERSIZE) {
		if (read_os2_bmp_info_header(fp, &infoheader) != 0)
			goto err;
		ncol = (fileheader.bfOffBits - 26) / 3;

		if (infoheader.biBitCount <= 8)
			read_bmp_icolors(ncol, pal, fp, 0);
	} else
		goto err;

	if (infoheader.biBitCount == 16)
		effect_depth = 24;
	else
		effect_depth = infoheader.biBitCount;

	bmp_compute_pitch(effect_depth, infoheader.biWidth,
			  (uint32_t *)(&bmp->pitch), TRUE);

	bmp->flags |= WMK_MYBMP_TYPE_BGR | WMK_MYBMP_FLOW_DOWN;
	bmp->depth = effect_depth;
	bmp->w     = infoheader.biWidth;
	bmp->h     = infoheader.biHeight;
	bmp->frames = 1;
	bmp->size  = biSize;

	*(struct bitmap_info_header *)bmp_info = infoheader;

	return bmp_info;

err:
	free(bmp_info);
	return NULL;
}

void cleanup_bmp(void *init_info)
{
	if (init_info)
		free(init_info);
}

int load_bmp(struct mg_rw_ops *fp, void *init_info, struct mybitmap *bmp,
	     _cb_one_scanline cb, void *context)
{
	struct wmk_init_info *info = (struct wmk_init_info *)init_info;
	int i, flag;
	int pitch = 0;
	unsigned char *bits;

	if (!(bmp->flags & WMK_MYBMP_LOAD_ALLOCATE_ONE))
		pitch = bmp->pitch;

	switch (info->biCompression) {
	case WMK_BI_BITFIELDS:
		info->rmask = wmk_fp_igetl(fp);
		info->gmask = wmk_fp_igetl(fp);
		info->bmask = wmk_fp_igetl(fp);
		break;

	case WMK_BI_RGB:
		if (info->biBitCount == 16)
			bmp->flags |= WMK_MYBMP_RGBSIZE_3;
		else if (info->biBitCount == 32)
			bmp->flags |= WMK_MYBMP_RGBSIZE_4;
		else
			bmp->flags |= WMK_MYBMP_RGBSIZE_3;
		break;

	case WMK_BI_RLE8:
	case WMK_BI_RLE4:
		break;

	default:
		goto err;
	}

	flag = WMK_BMP_LINE;
	bits = bmp->bits + (bmp->h - 1) * pitch;
	for (i = bmp->h - 1; i >= 0; i--, bits -= pitch) {
		switch (info->biCompression) {
		case WMK_BI_BITFIELDS:
		case WMK_BI_RGB:
			if (info->biBitCount == 16)
				wmk_read_16bit_image(fp, bits, bmp->pitch, bmp->w, info->gmask);
			else if (info->biBitCount == 32)
				rw_read(fp, bits, 1, bmp->pitch);
			else
				rw_read(fp, bits, 1, bmp->pitch);
			break;

		case WMK_BI_RLE8:
			flag = wmk_read_rle8_compressed_image(fp, bits, bmp->pitch, bmp->w);
			if (flag == WMK_BMP_ERR)
				goto err;
			break;

		case WMK_BI_RLE4:
			flag = wmk_read_rle4_compressed_image(fp, bits, bmp->pitch, bmp->w);
			if (flag == WMK_BMP_ERR)
				goto err;
			break;

		}

		if (cb && !pitch) cb(context, bmp, i);

		if (flag == WMK_BMP_END)
			break;
	}

	return 0;

err:
	return -1;
}

BOOL check_bmp(struct mg_rw_ops *fp)
{
	unsigned short bfType = wmk_fp_igetw(fp);

	/* bmp file type, 'MB' = 19778 */
	if (bfType != 0x4d42)
		return FALSE;

	return TRUE;
}