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

#ifndef __YUYV_PROCESS_HPP__
#define __YUYV_PROCESS_HPP__

#include "video.hpp"

int convert_yuyv(int width,
                 int height,
                 void* srcbuf,
                 void* dstbuf,
                 int neadshownum);

class YUYV_NV12_Stream : public StreamPUBase
{
    struct Video* video;

public:
    YUYV_NV12_Stream(struct Video* p) : StreamPUBase("YUYV_NV12", true, false) {
        video = p;
    }
    ~YUYV_NV12_Stream() {}
    bool processFrame(shared_ptr<BufferBase> inBuf,
                      shared_ptr<BufferBase> outBuf) {
        if (video->pthread_run && inBuf.get() && outBuf.get()) {
            if (convert_yuyv(video->width, video->height, inBuf->getVirtAddr(),
                             outBuf->getVirtAddr(), parameter_get_video_mark())) {
                video_record_signal(video);
                return false;
            }
            outBuf->setDataSize(video->width * video->height * 3 / 2);
        }

        return true;
    }
};

#endif
