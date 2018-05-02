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

#ifndef __H264_PROCESS_HPP__
#define __H264_PROCESS_HPP__

#include "video.hpp"

class H264_Encode : public StreamPUBase
{
    struct Video* video;

public:
    uint8_t* spspps;
    size_t spspps_size;
    uint8_t got_frame_type;
    enum {
        GOT_SPS = 1,
        GOT_PPS = 2,
        GOT_IFRAME = 4
    };
    H264_Encode(struct Video* p) : StreamPUBase("H264_Encode", true, true) {
        video = p;
        spspps = nullptr;
        spspps_size = 0;
        got_frame_type = 0;
    }
    ~H264_Encode() {
        if (spspps)
            free(spspps);
    }
    bool processFrame(shared_ptr<BufferBase> inBuf,
                      shared_ptr<BufferBase> outBuf) {
        if (video->save_en && video->pthread_run) {
            void* virt = NULL;
            int fd = -1;
            void* hnd = NULL;
            size_t size = 0;
            struct timeval time_val = {0};
            gettimeofday(&time_val, NULL);
            if (inBuf.get()) {
                virt = inBuf->getVirtAddr();
                fd = (int)inBuf->getFd();
                hnd = inBuf->getHandle();
                size = inBuf->getDataSize();
                if (!(got_frame_type & GOT_IFRAME) && size > 4) {
                    const uint8_t* p = (const uint8_t*)virt;
                    const uint8_t* end = p + size;
                    const uint8_t* nal_start = nullptr, *nal_end = nullptr;
                    int start_len = p[2] ? 3 : 4;
                    nal_start = find_h264_startcode(p, end);
                    for (;;) {
                        if (nal_start == end)
                            break;
                        nal_start += start_len;
                        nal_end = find_h264_startcode(nal_start, end);
                        unsigned size = nal_end - nal_start + start_len;
                        uint8_t nal_type = (*nal_start) & 0x1F;
                        if (nal_type == 5) {
                            got_frame_type |= GOT_IFRAME;
                            break;
                        } else if (nal_type == 7 || nal_type == 8) {
                            static uint8_t map[2][2] = {{7, GOT_SPS}, {8, GOT_PPS}};
                            uint8_t idx = (nal_type == 7) ? 0 : 1;

                            if (!(got_frame_type & map[idx][1]) &&
                                nal_type == map[idx][0]) {
                                uint8_t* tmp = (uint8_t*)realloc(spspps, size + spspps_size);
                                if (!tmp) {
                                    video_record_signal(video);
                                    return false;
                                }
                                spspps = tmp;
                                uint8_t* start = spspps + spspps_size;
                                memcpy(start, nal_start - start_len, size);
                                spspps_size += size;
                                got_frame_type |= map[idx][1];
                            }
                        }
                        nal_start = nal_end;
                    }
                }
            }

            if (video->save_en && video->pthread_run &&
                (got_frame_type & (GOT_IFRAME | GOT_SPS | GOT_PPS)) &&
                h264_encode_process(video, virt, fd, hnd, size, time_val)) {
                video_record_signal(video);
                return false;
            }
        }
        return true;
    }
};

#endif
