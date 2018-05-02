/**
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd
 * author: mick.mao mingkang.mao@rock-chips.com
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

#ifndef __DPP_PROCESS_H__
#define __DPP_PROCESS_H__

#include <CameraHal/StrmPUBase.h>
#include <dpp/dpp_frame.h>
#include <dpp/algo/algorithm_type.h>

using namespace rockchip;

#define DPP_OUT_BUFFER_NUM  3

class DppProcess : public StreamPUBase
{
public:
    DppProcess(struct Video *video, AlgorithmType type, bool shareInBuf);
    DppProcess(struct Video *video, const char* name, AlgorithmType type, bool shareInBuf);
    ~DppProcess();

    //from NewCameraBufferReadyNotifier
    virtual bool bufferReady(weak_ptr<BufferBase> buffer, int status);
    //need process result frame buffer
    virtual bool prepare(const frm_info_t& frmFmt, unsigned int numBuffers,
                         shared_ptr<CameraBufferAllocator> allocator);
    virtual bool start();
    virtual void stop();
protected:
    class ProcessThread : public CamThread
    {
    public:
        ProcessThread(DppProcess* dppProc): mDppProcess(dppProc) {};
        virtual bool threadLoop(void);
    private:
        DppProcess* mDppProcess;
    };
    virtual bool processFrame();
    virtual bool doSomeThing(shared_ptr<BufferBase> outBuf, DppFrame* frame);
    virtual int dppPacketProcess(shared_ptr<BufferBase>& inBuf);

    struct Video *mVideo;
    dpp::DppCore* mDppCore;
    AlgorithmType mAlgorithmType;

};

#endif
