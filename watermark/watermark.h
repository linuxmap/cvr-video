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

#ifndef __WATER_MARK_H__
#define __WATER_MARK_H__

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include <mpp/rk_mpi.h>
#include <mpp/rk_mpi_cmd.h>

#include "cvr_ffmpeg_shared.h"
#include "video_ion_alloc.h"

#include "wmk_readbmp.h"

#define WATERMARK_TIME 0x00000001
#define WATERMARK_LOGO 0x00000010
#define WATERMARK_MODEL 0x00000100
#define WATERMARK_SPEED 0x00001000
#define WATERMARK_LICENSE 0x00010000

#define PALETTE_TABLE_LEN 256
#define BUFFER_NUM 2
#define MAX_TIME_LEN 19

enum watermark_logo_index {
	LOGO240P = 1,
	LOGO360P,
	LOGO480P,
	LOGO720P,
	LOGO1080P,
	LOGO1440P,
	LOGO4K,
};

struct watermark_rect {
	uint32_t x;
	uint32_t y;
	uint32_t w;       /* not 16-byte alignment */
	uint32_t h;       /* not 16-byte alignment */
	uint32_t width;   /* 16-byte alignment */
	uint32_t height;  /* 16-byte alignment */
};

struct watermark_color_info {
	uint32_t set_image_transparent;
	uint16_t transparent_color;
	uint16_t text_color;
	uint16_t text_border_color;
};

struct watermark_coord_info {
	struct watermark_rect time_bmp;
	struct watermark_rect logo_bmp;
	struct watermark_rect model_bmp;
	struct watermark_rect speed_bmp;
	struct watermark_rect license_bmp;
};

struct osd_data_offset_info {
	uint32_t time_data_offset;
	uint32_t logo_data_offset;
	uint32_t model_data_offset;
	uint32_t speed_data_offset;
	uint32_t license_data_offset;
};

struct watermark_info {
	int type;
	uint32_t logo_index;
	/* osd buffer index */
	uint32_t buffer_index;
	uint32_t osd_num_region;

	struct osd_data_offset_info osd_data_offset;
	struct watermark_coord_info coord_info;
	struct watermark_color_info color_info;

	/* In order to ensure complete data updates, double buffers are used. */
	struct video_ion watermark_data[BUFFER_NUM];
	DataBuffer_t data_buffer[BUFFER_NUM];
	MppEncOSDData mpp_osd_data[BUFFER_NUM];
};

bool is_licenseplate_valid(char *licenStr);
extern const uint32_t yuv444_palette_table[PALETTE_TABLE_LEN];

void watermark_update_osd_num_region(int onoff,
				     struct watermark_info *watermark);

void watermark_init(struct watermark_info *watermark);

void watermark_deinit(struct watermark_info *watermark);

int watermark_config(int video_width, int video_height,
		     struct watermark_info *watermark);

int watermark_set_parameters(int video_width, int video_height,
			     enum watermark_logo_index logo_index, int type,
			     struct watermark_coord_info coord_info,
			     struct watermark_info *watermark);

void watermark_get_show_time(uint32_t *show_time, struct tm *time_now,
			     uint32_t len);

void watermark_update_rect_bmp(uint32_t *src, uint32_t src_len,
			       struct watermark_rect dst_rect, uint8_t *dst,
			       struct watermark_color_info color_info);

void watermark_get_show_license(uint32_t *show_license, char *licenseStr,
				uint32_t *len);

void watermart_get_license_data(struct watermark_info *watermark, uint32_t *src,
				uint32_t srclen);

void watermart_get_licenseplate_str(char *lic_plate, int len,
				    char *licplate_str);

void watermark_refresh(int video_width, int video_height,
		       struct watermark_info *watermark);

void watermark_draw_on_nv12(struct watermark_info *watermark, uint8_t *dst_buf,
			    int dst_w, int dst_h, char show_license);
#endif
