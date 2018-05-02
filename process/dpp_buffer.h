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

#ifndef __VIDEO_DPP_BUFFER_H__
#define __VIDEO_DPP_BUFFER_H__

#include <oslayer/oslayer.h>
#include <CameraHal/BufferBase.h>
#include <CameraHal/camHalTrace.h>
#include <dpp/common/error.h>
#include <dpp/dpp_frame.h>

using namespace rockchip;

class Dpp_Buffer : public BufferBase
{
public:
    Dpp_Buffer(weak_ptr<ICameraBufferOwener> bufOwener) : BufferBase(bufOwener) {}

    virtual ~Dpp_Buffer() {
        printf("~Dpp_Buffer: mUsedCnt = %d\n", mUsedCnt);
    }

    virtual void* getHandle(void) const {return NULL;}

    virtual void* getPhyAddr(void) const {
        return (void*)mDppBuf->phys_address();
    }

    virtual void* getVirtAddr(void) const {
        return mDppBuf->address();
    }

    virtual int getFd() {
        return mDppBuf->fd();
    }

    virtual size_t getCapacity(void) const {
        return mDppBuf->size();
    }
    virtual unsigned int getStride(void) const {
        return mWidth;
    }

    virtual const char* getFormat(void) {return NULL;}

    virtual unsigned int getWidth(void) {
        return mWidth;
    }

    virtual unsigned int getHeight(void) {
        return mHeight;
    }

    virtual struct timeval getTimestamp() const { return mTimestamp; }

#if MAIN_APP_NEED_DOWNSCALE_STREAM
    virtual uint32_t getDownscaleWidth(void) const {
        return mDownscaleWidth;
    }

    virtual uint32_t getDownscaleHeight(void) const {
        return mDownscaleHeight;
    }

    virtual int getDownscaleFd(void) const {
        if (mDownscale)
            return mDownscale->fd();
        else
            return -1;
    }

    virtual uint32_t getDownscalePhys(void) const {
        if (mDownscale)
            return mDownscale->phys_address();
        else
            return 0;
    }

    virtual void* getDownscaleVirt(void) const {
        if (mDownscale)
            return mDownscale->address();
        else
            return 0;
    }
#endif
    virtual void setDataSize(size_t size) {}

    virtual size_t getDataSize(void) const {
        return mDppBuf->size();
    }

    virtual bool setMetaData(struct HAL_Buffer_MetaData* metaData) {
        mPbufMetadata = metaData;
        return true;
    }
    //nouse
    virtual bool lock(unsigned int usage = READ) {return true;}
    //nouse
    virtual bool unlock(unsigned int usage = READ) {return true;}

    virtual void getSharpness(struct dpp_sharpness &sharp) {
        sharp = mSharpness;
    }

    virtual void getNoise(uint32_t *noise) {
        noise[0] = mNoise[0];
        noise[1] = mNoise[1];
        noise[2] = mNoise[2];
        noise[3] = mNoise[3];
    }

    virtual bool get_nr_valid() {return nr_valid;}

    virtual bool incUsedCnt() {
        osMutexLock(&mBufferUsedCntMutex);
        mUsedCnt++;
        osMutexUnlock(&mBufferUsedCntMutex);
        return true;
    }

    virtual bool decUsedCnt() {
        bool ret = true;

        osMutexLock(&mBufferUsedCntMutex);
        mUsedCnt--;
        if (mUsedCnt < 0) {
            std::cout << __func__ << ":error used count " << mUsedCnt << "\n";
            mUsedCnt = 0;
        } else if (mUsedCnt == 0) {
            ret = releaseToOwener();
            mDppBuf.reset();
            mDownscale.reset();
        }
        osMutexUnlock(&mBufferUsedCntMutex);

        return ret;
    }

    bool createFromFrame(DppFrame* frame) {
        mDppBuf = dpp_frame_get_output_buffer(frame);
        mTimestamp = dpp_frame_get_pts(frame);
        if (dpp_frame_get_type(frame) == k3DNoiseReduction) {
            mPbufMetadata = (struct HAL_Buffer_MetaData*)dpp_frame_get_private_data(frame);
            dpp_frame_get_noise(frame, mNoise);
            dpp_frame_get_sharpness(frame, &mSharpness);
            nr_valid = true;
        } else {
            nr_valid = false;
        }
        mWidth = dpp_frame_get_output_width(frame);
        mHeight = dpp_frame_get_output_height(frame);

        mDownscale = dpp_frame_get_downscale(frame);
        if (mDownscale) {
            mDownscaleWidth = dpp_frame_get_downscale_width(frame);
            mDownscaleHeight = dpp_frame_get_downscale_height(frame);
        } else {
            mDownscaleWidth = 0;
            mDownscaleHeight = 0;
        }
        return true;
    }

protected:
    unsigned int mWidth;
    unsigned int mHeight;
    dpp::Buffer::SharedPtr mDppBuf;
    dpp::Buffer::SharedPtr mDownscale;
    uint32_t mDownscaleWidth;
    uint32_t mDownscaleHeight;
    uint32_t mNoise[4];
    struct dpp_sharpness mSharpness;
    bool nr_valid;
};

#endif
