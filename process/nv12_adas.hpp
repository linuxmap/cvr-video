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

#ifndef __NV12_ADAS_HPP__
#define __NV12_ADAS_HPP__

#include "video.hpp"
#include "dpp_process.h"

class NV12_ADAS : public DppProcess
{
public:
    NV12_ADAS(struct Video *video);
    ~NV12_ADAS();

    virtual int dppPacketProcess(shared_ptr<BufferBase>& inBuf);
    virtual bool processFrame();
protected:
    struct adas_output output;
    struct video_ion ldw_input;
    struct video_ion* curr_ldw;
    int rga_fd;
};

#endif
