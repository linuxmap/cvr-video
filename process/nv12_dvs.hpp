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

#ifndef __NV12_DVS_HPP__
#define __NV12_DVS_HPP__

#include "dpp_process.h"

#define DOWN_SCALE_WIDTH 320
#define DOWN_SCALE_HEIGHT 180

class NV12_DVS : public DppProcess
{
public:
    NV12_DVS(struct Video *video);
    ~NV12_DVS();

    void stop();
    virtual bool doSomeThing(shared_ptr<BufferBase> outBuf, DppFrame* frame);
    virtual int dppPacketProcess(shared_ptr<BufferBase>& inBuf);
    int video_receive_gyrosensor_data(void);
    int video_dvs_init(struct Video* video);
    int video_dvs_deinit(struct Video* video);
};

#endif

