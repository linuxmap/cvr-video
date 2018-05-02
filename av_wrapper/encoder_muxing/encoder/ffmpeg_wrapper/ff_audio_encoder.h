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

#ifndef FF_AUDIO_ENCODER_H
#define FF_AUDIO_ENCODER_H

#include "ff_base_encoder.h"

class FFAudioEncoder : public BaseEncoder, public FFBaseEncoder {
 public:
  FFAudioEncoder();
  virtual ~FFAudioEncoder();
  virtual int InitConfig(MediaConfig& config) override;
  void GetAudioBuffer(void** buf,
                      int* nb_samples,
                      int* channels,
                      enum AVSampleFormat* format);
  int EncodeAudioFrame(AVPacket* out_pkt);
  // return if there is no remaining audio data
  void EncodeFlushAudioData(AVPacket* out_pkt, bool& finish);
  virtual void* GetHelpContext() override;

 private:
  AVFrame* audio_frame_;
  AVFrame* audio_in_frame_;
  int samples_count_;
  struct SwrContext* swr_ctx_;
};

#endif  // FF_AUDIO_ENCODER_H
