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

#ifndef __VIDEO_USB_HPP__
#define __VIDEO_USB_HPP__

#include "video.hpp"

int usb_video_init(struct Video* video,
                   int num,
                   unsigned int width,
                   unsigned int height,
                   unsigned int fps);
int usb_video_path(struct Video* video);
int usb_video_start(struct Video* video);
void usb_video_deinit(struct Video* video);

#endif
