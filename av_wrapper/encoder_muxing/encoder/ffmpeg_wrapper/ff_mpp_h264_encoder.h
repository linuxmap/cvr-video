/**
 * Copyright (C) 2016 Fuzhou Rockchip Electronics Co., Ltd
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

#ifndef FF_MPP_H264_ENCODER_H
#define FF_MPP_H264_ENCODER_H

#include "ff_base_encoder.h"

extern "C" {
#include <cvr_ffmpeg_shared.h>
}

#include <mpp/rk_mpi.h>

// The share data struct between ffmpeg and mpp.
typedef DataBuffer_t FFMppShareBuffer;
typedef FFMppShareBuffer* PFFMppShareBuffer;

// A encoder which call the ffmpeg interface,
// finally towards to mpp interface.
class FFMPPH264Encoder : public BaseVideoEncoder, public FFBaseEncoder {
 public:
  FFMPPH264Encoder();
  virtual ~FFMPPH264Encoder();
  // Must be called before encoding.
  virtual int InitConfig(MediaConfig& config) override;
  // Encode the raw srcbuf to dstbuf.
  virtual int EncodeOneFrame(Buffer* src, Buffer* dst, Buffer* mv) override;
  // Control before encoding.
  int EncodeControl(int cmd, void* param);
  virtual void* GetHelpContext() override;

 private:
  FFContext ff_ctx_;
  AVFrame* video_frame_;
  // Return the encoded pkt size.
  int EncodeOneFrame(PFFMppShareBuffer src_buf,
                     PFFMppShareBuffer dst_buf,
                     uint32_t& dst_flag,
                     PFFMppShareBuffer mv_buf);
};

#endif  // FF_MPP_H264_ENCODER_H
