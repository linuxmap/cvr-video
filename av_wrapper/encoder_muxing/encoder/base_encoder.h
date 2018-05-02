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

#ifndef ENCODER_CODECS_BASE_ENCODER_H_
#define ENCODER_CODECS_BASE_ENCODER_H_

#include <stdint.h>

#include "video_common.h"

typedef enum {
  PIX_FMT_NONE = -1,
  PIX_FMT_YUV420P,
  PIX_FMT_NV12,
  PIX_FMT_NV21,
  PIX_FMT_YUV422P,
  PIX_FMT_NV16,
  PIX_FMT_NV61,
  PIX_FMT_YVYU422,
  PIX_FMT_UYVY422,
  PIX_FMT_RGB565LE,
  PIX_FMT_BGR565LE,
  PIX_FMT_RGB24,
  PIX_FMT_BGR24,
  PIX_FMT_RGB32,
  PIX_FMT_BGR32,
  PIX_FMT_NB
} PixelFormat;

typedef enum {
  SAMPLE_FMT_NONE = -1,
  SAMPLE_FMT_U8,
  SAMPLE_FMT_S16,
  SAMPLE_FMT_S32,
  SAMPLE_FMT_NB
} SampleFormat;

typedef struct {
  PixelFormat fmt;
  int width;
  int height;
  int bit_rate;
  int frame_rate;
  int level;
  int gop_size;
  int profile;
  // quality - quality parameter
  // mpp does not give the direct parameter in different protocol.
  // mpp provide total 5 quality level 1 ~ 5
  // 0 : MPP_ENC_RC_QUALITY_WORST
  // 1 : MPP_ENC_RC_QUALITY_WORSE
  // 2 : MPP_ENC_RC_QUALITY_MEDIUM
  // 3 : MPP_ENC_RC_QUALITY_BETTER
  // 4 : MPP_ENC_RC_QUALITY_BEST
  // 5 : MPP_ENC_RC_QUALITY_CQP
  //     (extra CQP level means special constant-qp (CQP) mode)
  int quality;
  int qp_init;
  int qp_step;
  int qp_min;
  int qp_max;
  // rc_mode - rate control mode
  // Mpp balances quality and bit rate by the mode index
  // Mpp provide 2 level of balance mode of quality and bit rate
  // 0 : MPP_ENC_RC_MODE_VBR
  // 1 : MPP_ENC_RC_MODE_CBR
  int rc_mode;
} VideoConfig;

typedef struct {
  SampleFormat fmt;
  int bit_rate;
  int nb_samples;
  int sample_rate;
  int nb_channels;
  uint64_t channel_layout;
} AudioConfig;

typedef struct {
  PixelFormat fmt;
  int width;
  int height;
  int qp;
} JpegEncConfig;

typedef union {
  VideoConfig video_config;
  AudioConfig audio_config;
  JpegEncConfig jpeg_config;
} MediaConfig;

class BaseEncoder : public video_object {
 public:
  BaseEncoder() : help_ctx_(nullptr) { codec_name_[0] = 0; }
  virtual ~BaseEncoder() = default;
  static int CalPixelSize(const int w, const int h, const PixelFormat fmt);
  static int CalAlignPixelSize(const int w, const int h, const PixelFormat fmt);
  virtual int InitConfig(MediaConfig& config);
  inline char* GetCodecName() { return codec_name_; }
  inline MediaConfig& GetConfig() { return config_; }
  // Return the context which help for muxing.
  // If ffmpeg do muxing, must return type FFContext.
  virtual void* GetHelpContext() { return help_ctx_; }
  virtual void SetHelpContext(void* ctx) { help_ctx_ = ctx; }

 protected:
  char codec_name_[16];
  MediaConfig config_;
  void* help_ctx_;
};

class Buffer;
class BaseVideoEncoder : public BaseEncoder {
 public:
  BaseVideoEncoder();
  virtual ~BaseVideoEncoder() {}
  inline VideoConfig& GetVideoConfig() { return config_.video_config; }
  // This is a blocking sync calling, return util encoding finish.
  virtual int EncodeOneFrame(Buffer* src, Buffer* dst, Buffer* mv) = 0;
  typedef enum {
    kFrameRateChange = 0x00000001,
    kBitRateChange = 0x00000002,
    kForceIdrFrame = 0x00000004,
  } DyncConfFlag;
  inline bool DyncConfigTestAndSet(DyncConfFlag flag) {
    bool ret = dync_config_flag_ & flag;
    if (ret)
      dync_config_flag_ &= ~flag;
    return ret;
  }
  // Should be called in the same thread to EncodeOneFrame.
  inline void SetNewFrameRate(int frame_rate) {
    new_frame_rate_ = frame_rate;
    dync_config_flag_ |= kFrameRateChange;
  }
  inline void SetNewBitRate(int bit_rate) {
    new_bit_rate_ = bit_rate;
    dync_config_flag_ |= kBitRateChange;
  }
  inline void SetForceIdrFrame() { dync_config_flag_ |= kForceIdrFrame; }

 protected:
  // the dynamic configs
  int new_frame_rate_;
  int new_bit_rate_;
  uint32_t dync_config_flag_;
};

const uint8_t* find_h264_startcode(const uint8_t* p, const uint8_t* end);

#endif  // ENCODER_CODECS_BASE_ENCODER_H_
