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

#ifndef __UVC_ENCODE_H__
#define __UVC_ENCODE_H__

extern "C" {
#include "vpu.h"
#include "video_ion_alloc.h"
}
#include "encode_handler.h"
#include "uvc_user.h"

struct uvc_encode {
    struct vpu_encode encode;
    MPPH264Encoder *h264_encoder;
    struct video_ion encode_dst_buff;
    BufferData dst_data;
    int rga_fd;
    struct video_ion uvc_out;
    struct video_ion uvc_mid;
    void* out_virt;
    int out_fd;
    struct video_ion uvc_in;
};

int uvc_encode_init(struct uvc_encode *e);
void uvc_encode_exit(struct uvc_encode *e);
int uvc_rga_process(struct uvc_encode* e, int in_width, int in_height,
                    void* in_virt, int in_fd);
bool uvc_encode_process(struct uvc_encode *e);

#endif
