/**
 * Copyright (C) 2016 Fuzhou Rockchip Electronics Co., Ltd
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

#ifndef __UVC_USER_H__
#define __UVC_USER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <unistd.h>
#include "uvc-gadget.h"

struct uvc_user {
    unsigned int width;
    unsigned int height;
    bool run;
    char* buffer;
    unsigned int size;
    unsigned int fcc;
};

extern struct uvc_user uvc_user;

#define UVC_BUFFER_NUM 3

int uvc_buffer_init();
void uvc_buffer_deinit();
bool uvc_buffer_process_state();
bool uvc_buffer_write_enable();
void uvc_buffer_write(void* extra_data,
                      size_t extra_size,
                      void* data,
                      size_t size,
                      unsigned int fcc);
int uvc_gadget_pthread_init(int id);
void uvc_gadget_pthread_exit();
void uvc_set_user_resolution(int width, int height);
void uvc_get_user_resolution(int* width, int* height);
bool uvc_check_user_resolution_limit(int width, int height);
bool uvc_get_user_run_state();
void uvc_set_user_run_state(bool state);
void uvc_set_user_fcc(unsigned int fcc);
unsigned int uvc_get_user_fcc();
int uvc_get_user_video_id();
void uvc_set_user_video_id(int video_id);
void uvc_user_fill_buffer(struct uvc_device *dev, struct v4l2_buffer *buf);

#ifdef __cplusplus
}

#endif
#endif
