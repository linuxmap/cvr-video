/**
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd
 * author: hertz.wang hertz.wong@rock-chips.com
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

#ifndef SCALEENCODETSHANDLER_H
#define SCALEENCODETSHANDLER_H

#include "scale_encode_handler.h"
#include "encoder_muxing/packet_dispatcher.h"
#include "encoder_muxing/muxing/ts/camera_ts.h"
#include "encoder_muxing/encoder/mpp_h264_encoder.h"
#include "encoder_muxing/encoder/ffmpeg_wrapper/ff_context.h"

#define SCALED_WIDTH 640
#define SCALED_HEIGHT 384
#define SCALED_BIT_RATE 800000

class FFAudioEncoder;

class ScaleEncodeTSHandler : public ScaleEncodeHandler<MPPH264Encoder>
{
public:
    ScaleEncodeTSHandler(MediaConfig& config);
    virtual ~ScaleEncodeTSHandler();
    int Init(MediaConfig src_config);
    virtual void DeInit() override;
    void Process(int src_fd, const VideoConfig& src_config, const timeval& time);
    int StartTransferStream(char* uri,
                            pthread_attr_t* global_attr,
                            FFAudioEncoder* audio_enc = nullptr);
    int StopTransferStream();
    bool Valid() { return valid_; }
    int GetCommuFd() { return commu_fd_; }
    CameraTs* GetTsMuxer() { return ts_; }
    MPPH264Encoder* GetVideoEncoder() { return encoder_; }
    PacketDispatcher* GetPacketDispatcher() { return &pkt_dispatcher_; }

protected:
    virtual void GetSrcConfig(const MediaConfig& src_config,
                              int& src_w,
                              int& src_h,
                              PixelFormat& src_fmt) final;
    virtual int PrepareBuffers(MediaConfig& src_config,
                               const int dst_numerator,
                               const int dst_denominator,
                               int& dst_w,
                               int& dst_h) override;
    virtual void Work() final;

private:
    PacketDispatcher pkt_dispatcher_;
    CameraTs* ts_;
    MediaConfig scale_config_;  // bit_rate, gop, width, height
    struct timeval time_val_;
    FFContext ff_ctx_;  // ffmpeg muxer help context

    // For pipe rtsp.
    static const char* kCommuPath;
    int commu_fd_;
    pthread_t commu_tid_;
    bool valid_;
};

#endif  // SCALEENCODETSHANDLER_H
