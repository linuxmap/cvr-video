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

#ifndef __VIDEO_ISP_HPP__
#define __VIDEO_ISP_HPP__

#include "video.hpp"
#include "nv12_process.hpp"
#include "nv12_3dnr.hpp"
#include "nv12_adas.hpp"
#include "nv12_dvs.hpp"

extern HAL_COLORSPACE color_space;
extern bool is_record_mode;

int video_isp_add_policy(struct Video* video);
void video_isp_remove_policy(struct Video* video);
int video_isp_add_mp_policy_fix(struct Video* video);
void video_isp_remove_mp_policy_fix(struct Video* video);
int video_isp_add_sp_policy_fix(struct Video* video);
void video_isp_remove_sp_policy_fix(struct Video* video);
int isp_video_init(struct Video* video,
                   int num,
                   unsigned int width,
                   unsigned int height,
                   unsigned int fps);
int isp_video_path(struct Video* video);
int isp_video_start(struct Video* video);
void isp_video_deinit(struct Video* video);

#if MAIN_APP_NEED_DOWNSCALE_STREAM
bool video_isp_get_enc_scale(struct Video* video);
void video_isp_set_enc_scale(struct Video* video, bool scale);
#endif

#endif
