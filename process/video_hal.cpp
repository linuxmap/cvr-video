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

#include "video_hal.hpp"

int hal_add_pu(shared_ptr<CamHwItf::PathBase> pre,      shared_ptr<StreamPUBase> next,
               frm_info_t &frmFmt, unsigned int num, shared_ptr<CameraBufferAllocator> allocator)
{
    if (!pre.get() || !next.get()) {
        printf("%s: PU is NULL\n", __func__);
        return -1;
    }
    pre->addBufferNotifier(next.get());
    next->prepare(frmFmt, num, allocator);
    if (!next->start()) {
        printf("PU start failed!\n");
        return -1;
    }

    return 0;
}

int hal_add_pu(shared_ptr<StreamPUBase> pre, shared_ptr<StreamPUBase> next,
               frm_info_t &frmFmt, unsigned int num, shared_ptr<CameraBufferAllocator> allocator)
{
    if (!pre.get() || !next.get()) {
        printf("%s: PU is NULL\n", __func__);
        return -1;
    }
    pre->addBufferNotifier(next.get());
    next->prepare(frmFmt, num, allocator);
    if (!next->start()) {
        printf("PU start failed!\n");
        return -1;
    }

    return 0;
}

void hal_remove_pu(shared_ptr<CamHwItf::PathBase> pre, shared_ptr<StreamPUBase> next)
{
    if (!pre.get() || !next.get())
        return;

    pre->removeBufferNotifer(next.get());
    next->stop();
    next->releaseBuffers();
}

void hal_remove_pu(shared_ptr<StreamPUBase> pre, shared_ptr<StreamPUBase> next)
{
    if (!pre.get() || !next.get())
        return;

    pre->removeBufferNotifer(next.get());
    next->stop();
    next->releaseBuffers();
}

