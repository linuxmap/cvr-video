/**
 * Copyright (C) 2018 Fuzhou Rockchip Electronics Co., Ltd
 * Author: frank <frank.liu@rock-chips.com>
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

#ifndef __FACE_DETECT_PROCESS_H__
#define __FACE_DETECT_PROCESS_H__

#ifdef ENABLE_RS_FACE

#include "dpp_process.h"

using namespace std;

//#define RS_DEBUG
#ifdef RS_DEBUG
#define RS_DBG(...) printf(__VA_ARGS__)
#else
#define RS_DBG(...)
#endif

class FaceDetectProcess : public DppProcess
{
public:
    FaceDetectProcess(struct Video *video);
    ~FaceDetectProcess();

    virtual bool doSomeThing(shared_ptr<BufferBase> outBuf, DppFrame* frame);
    virtual int dppPacketProcess(shared_ptr<BufferBase>& inBuf);
};

#endif
#endif
