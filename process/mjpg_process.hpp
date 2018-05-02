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

#ifndef __MJPG_PROCESS_HPP__
#define __MJPG_PROCESS_HPP__

#include "video.hpp"

int mjpg_decode(struct Video* video,      void* srcbuf, unsigned int size, void* dstbuf, int dstfd);
int video_record_save_jpeg(struct Video* video,            void* srcbuf, unsigned int size);

class MJPG_NV12 : public StreamPUBase
{
    struct Video* video;
    bool got_valid_input;
public:
    MJPG_NV12(struct Video* p) : StreamPUBase("MJPG_NV12", true, false),
        video(p), got_valid_input(false) {}
    ~MJPG_NV12() {}

    bool processFrame(shared_ptr<BufferBase> inBuf,
                      shared_ptr<BufferBase> outBuf) {
        if (video->pthread_run && inBuf.get() && outBuf.get()) {
            if (mjpg_decode(video, inBuf->getVirtAddr(), inBuf->getDataSize(),
                            outBuf->getVirtAddr(), (int)(outBuf->getFd()))) {
                if (!got_valid_input) {
                    // If have never got a valid input stream,
                    // do not call the children PU.
                    return false;
                }
                outBuf->setDataSize(0);
            } else {
                got_valid_input = true;
                outBuf->setDataSize(video->width * video->height * 3 / 2);
            }
        }
        return true;
    }
};

class MJPG_Photo : public StreamPUBase
{
    struct Video* video;

public:
    MJPG_Photo(struct Video* p) : StreamPUBase("MJPG_Photo", true, true) {
        video = p;
    }
    ~MJPG_Photo() {}

    bool processFrame(shared_ptr<BufferBase> inBuf,
                      shared_ptr<BufferBase> outBuf) {
        if (video->pthread_run && video->photo.state == PHOTO_ENABLE && inBuf.get()) {
            video->photo.state = PHOTO_BEGIN;
            video_record_save_jpeg(video, inBuf->getVirtAddr(), inBuf->getDataSize());
        }
        return true;
    }
};

#endif
