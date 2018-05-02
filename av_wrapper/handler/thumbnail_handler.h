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

#ifndef THUMBNAIL_HANDLER_H
#define THUMBNAIL_HANDLER_H

#include "scale_encode_handler.h"
#include "encoder_muxing/encoder/mpp_jpeg_encoder.h"

class ThumbnailHandler : public ScaleEncodeHandler<MPPJpegEncoder> {
 public:
  ThumbnailHandler();
  int Init(MediaConfig config);
  bool Process(int src_fd,
               const VideoConfig& src_config,
               const char* video_file_path);

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
  static const int kThumbnailWidth = 320;
  char file_path_[128];
};

#endif  // THUMBNAIL_HANDLER_H
