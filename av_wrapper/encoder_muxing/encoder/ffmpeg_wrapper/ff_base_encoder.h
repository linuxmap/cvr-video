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

#ifndef FF_BASE_ENCODER_H
#define FF_BASE_ENCODER_H

#include "ff_context.h"

class FFBaseEncoder {
 public:
  FFBaseEncoder(char* codec_name);
  FFBaseEncoder(AVCodecID codec_id);
  // Return the context which help for muxing.
  inline FFContext* GetHelpContext() { return &ff_ctx_; }

 protected:
  int InitConfig(VideoConfig& vconfig);
  int InitConfig(AudioConfig& aconfig);
  inline AVCodecContext* GetAVCodecContext() {
    return ff_ctx_.GetAVCodecContext();
  }

 private:
  FFContext ff_ctx_;
};

#endif  // FF_BASE_ENCODER_H
