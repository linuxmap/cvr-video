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

#ifndef ENCODER_CODECS_FF_CONTEXT_H_
#define ENCODER_CODECS_FF_CONTEXT_H_

extern "C" {
#include <libavformat/avformat.h>
}

#include "encoder_muxing/encoder/base_encoder.h"

class FFContext {
 public:
  FFContext();
  virtual ~FFContext();
  int InitConfig(VideoConfig& vconfig, bool ff_open_codec = true);
  int InitConfig(AudioConfig& aconfig);
  inline AVFormatContext* GetAVFmtContext() { return av_fmt_ctx_; }
  inline AVCodec* GetAVCodec() { return av_codec_; }
  inline AVStream* GetAVStream() { return av_stream_; }
  inline AVCodecContext* GetAVCodecContext() {
    return av_stream_ ? av_stream_->codec : nullptr;
  }
  inline void SetCodecName(char* codec_name) { codec_name_ = codec_name; }
  inline void SetCodecId(AVCodecID codec_id) { codec_id_ = codec_id; }

  static enum AVPixelFormat ConvertToAVPixFmt(const PixelFormat& fmt);
  static enum AVSampleFormat ConvertToAVSampleFmt(const SampleFormat& fmt);

  // Pack the encoded data to avpacket,
  // default do not copy content to avpacket.
  static int PackEncodedDataToAVPacket(const Buffer& buf,
                                       AVPacket& out_pkt,
                                       const bool copy = false);

 private:
  AVFormatContext* av_fmt_ctx_;
  AVCodec* av_codec_;
  AVStream* av_stream_;
  AVCodecID codec_id_;
  char* codec_name_;

  char* GetCodecName();
  int Init(bool ff_open_codec);
};

#endif  // ENCODER_CODECS_FF_CONTEXT_H_
