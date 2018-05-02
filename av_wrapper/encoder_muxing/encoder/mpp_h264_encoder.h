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

#ifndef MPP_H264_ENCODER_H
#define MPP_H264_ENCODER_H

#include "base_encoder.h"
#include "mpp_inc.h"
#include <mpp/rk_mpi.h>

// A encoder which call the mpp interface directly.
// Not thread-safe.
class MPPH264Encoder : public BaseVideoEncoder {
 public:
#ifndef MPP_PACKET_FLAG_INTRA
#define MPP_PACKET_FLAG_INTRA (0x00000008)
#endif
  typedef struct {
    MppCodingType video_type;
    MppCtx ctx;
    MppApi* mpi;
    MppPacket packet;
    MppFrame frame;
    MppBuffer osd_data;
  } RkMppEncContext;

  MPPH264Encoder();
  ~MPPH264Encoder();

  virtual int InitConfig(MediaConfig& config);
  // Change configs which are not contained in sps/pps.
  int CheckConfigChange();
  // Encode the raw srcbuf to dstbuf
  virtual int EncodeOneFrame(Buffer* src, Buffer* dst, Buffer* mv);
  // Control before encoding.
  int EncodeControl(int cmd, void* param);
  inline void GetExtraData(void*& extra_data, size_t& extra_data_size) {
    extra_data = extra_data_;
    extra_data_size = extra_data_size_;
  }

 private:
  RkMppEncContext mpp_enc_ctx_;
  void* extra_data_;
  size_t extra_data_size_;
};

#endif  // MPP_H264_ENCODER_H
