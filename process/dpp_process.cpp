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

#include "video.h"
#include "video.hpp"
#include "dpp_buffer.h"
#include "dpp_process.h"

DppProcess::DppProcess(struct Video *video, AlgorithmType type, bool shareInBuf)
    : StreamPUBase("DppProcess", true, shareInBuf)
{
    mVideo = video;
    mAlgorithmType = type;
}

DppProcess::DppProcess(struct Video *video, const char* name, AlgorithmType type, bool shareInBuf)
    : StreamPUBase(name, true, shareInBuf)
{
    mVideo = video;
    mAlgorithmType = type;
}

DppProcess::~DppProcess()
{
    ALOGD("%s:%d enter", __func__, __LINE__);
    if (mDppCore) {
        delete mDppCore;
        mDppCore = NULL;
    }
    ALOGD("%s:%d exit", __func__, __LINE__);
}

int DppProcess::dppPacketProcess(shared_ptr<BufferBase>& inBuf)
{
    return 0;
}

bool DppProcess::bufferReady(weak_ptr<BufferBase> buffer, int status)
{
    shared_ptr<BufferBase> spBuf = buffer.lock();
    //ALOGD("%s:%d index = %d", __func__, __LINE__, spBuf->getIndex());
    //TODO:if status is error, should subsequent PU be notified?
    int dpp_flag = 0;

    if (status != 0) {
        ALOGW("%s:%d status error, status=%d", __func__, __LINE__, status);
        goto exit_bufferReady;
    }

    osMutexLock(&mStateLock);
    if (mState != STREAMING) {
        osMutexUnlock(&mStateLock);
        goto exit_bufferReady;
    }
    osMutexUnlock(&mStateLock);

    spBuf->incUsedCnt();
    dpp_flag = dppPacketProcess(spBuf);

    if (dpp_flag == -1) {
        spBuf->decUsedCnt();
        printf("PU push CameraBuffer to DPP failed.\n");
        return false;
    } else if (dpp_flag == 1) {
        //drop frame for adas.
        spBuf->decUsedCnt();
        goto exit_bufferReady;
    }

    /*Some processer needn't change the content of inBuffer, just is an algorithm,
      we need hold on the inbuffer, wait the algorithm process end, and send to next process.
      eg. Adas only an algorithm for the memory, we want to take a result in the buffer, and pass to next processer.*/
    if (mShareInBuf) {
        spBuf->incUsedCnt();
        osMutexLock(&mBuffersLock);
        mInBuffers.push_back(spBuf);
        osMutexUnlock(&mBuffersLock);
    }
    //ALOGD("%s:%d ", __func__, __LINE__);
exit_bufferReady:

    return true;
}

bool DppProcess::prepare(
    const frm_info_t& frmFmt,
    unsigned int numBuffers,
    shared_ptr<CameraBufferAllocator> allocator)
{
    osMutexLock(&mStateLock);
    setReqFmt(frmFmt);

    if (mState != UNINITIALIZED) {
        ALOGE("%s: %d not in UNINITIALIZED state, cannot prepare", __func__, __LINE__);
        osMutexUnlock(&mStateLock);
        return false;
    }

    releaseBuffers();

    if (!mShareInBuf) {
        for (unsigned int i = 0; i < numBuffers; i++) {
            shared_ptr<BufferBase> dpp_buf(new Dpp_Buffer(shared_from_this()));
            dpp_buf->setIndex(i);
            mBufferPool.push_back(dpp_buf);
        }
    }

    mDppCore = new dpp::DppCore();
    int ret = mDppCore->init(mAlgorithmType);
    if (ret != kSuccess) {
        ALOGE("dpp_create failed.\n");
        osMutexUnlock(&mStateLock);
        return false;
    }

    mState = PREPARED;
    osMutexUnlock(&mStateLock);
    return true;
}

bool DppProcess::start()
{
    ALOGD("DppProcess:%s:%d ", __func__, __LINE__);
    osMutexLock(&mStateLock);
    bool ret = false;
    if (mState != PREPARED) {
        ALOGE("%s: %d cannot start, path is not in PREPARED state", __func__, __LINE__);
        osMutexUnlock(&mStateLock);
        return false;
    }

    //start dpp thread.
    mDppCore->start();

    mState = STREAMING;
    if (mNeedStrmTh) {
        mStrmPUThread = shared_ptr<CamThread>(new ProcessThread(this));
        if (mStrmPUThread->run(mName) == 0)
            ret = true;
    } else {
        ret = true;
    }
    if (ret != true) {
        mState = PREPARED;
        ALOGE("%s: PU thread start failed (error %d)", __func__, ret);
        osMutexUnlock(&mStateLock);
        return false;
    }
    ALOGD("DppProcess:%s:%d start success!", __func__, __LINE__);
    osMutexUnlock(&mStateLock);

    return ret;
}

void DppProcess::stop()
{
    ALOGD("DppProcess:%s:%d enter", __func__, __LINE__);
    mDppCore->stop();
    StreamPUBase::stop();
    ALOGD("DppProcess:%s:%d exit", __func__, __LINE__);
}

bool DppProcess::processFrame()
{
    DppFrame* frame = nullptr;
    DppRet ret = mDppCore->get_frame(&frame);
    if (!frame) {
        if (ret == kErrorTimeout) {
            printf("Get frame from dpp failed cause by timeout.\n");
            return true;
        } else {
            printf("Get frame failed, DPP request exit.\n");
            return false;
        }
    }

#if DBG_TIME
    struct timeval ts;
    ts = dpp_frame_get_pts(frame);
    printf ("dpp_get_frame after capture : %lld-%d\n",
            time2msec(ts), timeDelayAfter(ts));
#endif

    //get dpp out buffer from mBufferPool
    shared_ptr<BufferBase> outBuf = NULL;

    if (!mShareInBuf) {
        osMutexLock(&mBuffersLock);
        if (!mBufferPool.empty()) {
            outBuf = *mBufferPool.begin();
            mBufferPool.erase(mBufferPool.begin());
        } else
            ALOGW("%s:%d,no available buffer now!", __func__, __LINE__);
        osMutexUnlock(&mBuffersLock);
    } else {
        //get inbuf from mInBuffers
        osMutexLock(&mBuffersLock);
        if (!mInBuffers.empty()) {
            outBuf = *mInBuffers.begin();
            mInBuffers.erase(mInBuffers.begin());
        } else
            ALOGW("%s:%d,no found inbuf !!!", __func__, __LINE__);
        osMutexUnlock(&mBuffersLock);
    }

    if (outBuf.get() != NULL) {
        //for 3dnr and adas process.
        doSomeThing(outBuf, frame);

        //send to next process
        osMutexLock(&mNotifierLock);
        for (list<NewCameraBufferReadyNotifier*>::iterator i = mBufferReadyNotifierList.begin();
             i != mBufferReadyNotifierList.end(); i++)
            (*i)->bufferReady(outBuf, 0);
        osMutexUnlock(&mNotifierLock);

        if (!mShareInBuf) {
            osMutexLock(&mBuffersLock);
            mOutBuffers.push_back(outBuf);
            osMutexUnlock(&mBuffersLock);
        }

        outBuf->decUsedCnt();

    }

    dpp_frame_deinit(frame);

    return true;
}

bool DppProcess::doSomeThing(shared_ptr<BufferBase> outBuf, DppFrame* frame)
{
    return true;
}

bool DppProcess::ProcessThread::threadLoop(void)
{
    return mDppProcess->processFrame();
}

