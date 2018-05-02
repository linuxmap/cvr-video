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

#ifndef __VIDEO_HAL_HPP__
#define __VIDEO_HAL_HPP__

#include "video.hpp"

int hal_add_pu(shared_ptr<CamHwItf::PathBase> pre, shared_ptr<StreamPUBase> next,
               frm_info_t &frmFmt, unsigned int num, shared_ptr<CameraBufferAllocator> allocator);
int hal_add_pu(shared_ptr<StreamPUBase> pre, shared_ptr<StreamPUBase> next,
               frm_info_t &frmFmt, unsigned int num, shared_ptr<CameraBufferAllocator> allocator);
void hal_remove_pu(shared_ptr<CamHwItf::PathBase> pre, shared_ptr<StreamPUBase> next);
void hal_remove_pu(shared_ptr<StreamPUBase> pre, shared_ptr<StreamPUBase> next);

#endif
