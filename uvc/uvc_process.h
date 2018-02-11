/**
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd
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

#ifndef __UVC_PROCESS_H__
#define __UVC_PROCESS_H__

#ifdef __cplusplus
extern "C" {
#endif

#define UVC_WINDOW_WIDTH 1920
#define UVC_WINDOW_HEIGHT 1080

struct uvc_window {
    unsigned int x;
    unsigned int y;
    unsigned int w;
    unsigned int h;
};

void uvc_process_the_window(int position, void *src_virt, int src_fd, int src_w, int src_h,
                            int src_fmt, int vir_w, int vir_h);
void uvc_process_shield_the_window(int position);
int uvc_process_set_position();
void uvc_process_reset_position(int pos);
void set_uvc_window_one(int type);
void set_uvc_window_two(int type);
int video_uvc_encode_init();
void video_uvc_encode_exit();

#ifdef __cplusplus
}
#endif

#endif
