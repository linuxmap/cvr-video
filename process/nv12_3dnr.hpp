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

#ifndef __NV12_3DNR_HPP__
#define __NV12_3DNR_HPP__

#include "dpp_process.h"

class NV12_3DNR : public DppProcess
{
public:
    NV12_3DNR(struct Video *video);
    ~NV12_3DNR();

    virtual bool doSomeThing(shared_ptr<BufferBase> outBuf, DppFrame* frame);
    virtual int dppPacketProcess(shared_ptr<BufferBase>& inBuf);
};

#endif

