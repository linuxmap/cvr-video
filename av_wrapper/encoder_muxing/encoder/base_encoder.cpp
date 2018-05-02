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

#include "base_encoder.h"

int BaseEncoder::CalPixelSize(const int w, const int h, const PixelFormat fmt) {
  switch (fmt) {
    case PIX_FMT_YUV420P:
    case PIX_FMT_NV12:
    case PIX_FMT_NV21:
      return w * h * 3 / 2;
    case PIX_FMT_YUV422P:
    case PIX_FMT_NV16:
    case PIX_FMT_NV61:
    case PIX_FMT_YVYU422:
    case PIX_FMT_UYVY422:
      return w * h * 2;
    case PIX_FMT_RGB565LE:
    case PIX_FMT_BGR565LE:
      return w * h * 2;
    case PIX_FMT_RGB24:
    case PIX_FMT_BGR24:
      return w * h * 3;
    case PIX_FMT_RGB32:
    case PIX_FMT_BGR32:
      return w * h * 4;
    default:
      fprintf(stderr, "unsupport compress format %d\n", fmt);
      return 0;
  }
}

int BaseEncoder::CalAlignPixelSize(const int w,
                                   const int h,
                                   const PixelFormat fmt) {
  const int w_align = UPALIGNTO16(w);
  const int h_align = UPALIGNTO16(h);
  return CalPixelSize(w_align, h_align, fmt);
}

int BaseEncoder::InitConfig(MediaConfig& config) {
  config_ = config;
  return 0;
}

BaseVideoEncoder::BaseVideoEncoder()
    : new_frame_rate_(0), new_bit_rate_(0), dync_config_flag_(0) {}

int BaseVideoEncoder::EncodeOneFrame(Buffer* src, Buffer* dst, Buffer* mv) {
  return 0;
}

// Copy from ffmpeg.
static const uint8_t* find_startcode_internal(const uint8_t* p,
                                              const uint8_t* end) {
  const uint8_t* a = p + 4 - ((intptr_t)p & 3);

  for (end -= 3; p < a && p < end; p++) {
    if (p[0] == 0 && p[1] == 0 && p[2] == 1)
      return p;
  }

  for (end -= 3; p < end; p += 4) {
    uint32_t x = *(const uint32_t*)p;
    //      if ((x - 0x01000100) & (~x) & 0x80008000) // little endian
    //      if ((x - 0x00010001) & (~x) & 0x00800080) // big endian
    if ((x - 0x01010101) & (~x) & 0x80808080) {  // generic
      if (p[1] == 0) {
        if (p[0] == 0 && p[2] == 1)
          return p;
        if (p[2] == 0 && p[3] == 1)
          return p + 1;
      }
      if (p[3] == 0) {
        if (p[2] == 0 && p[4] == 1)
          return p + 2;
        if (p[4] == 0 && p[5] == 1)
          return p + 3;
      }
    }
  }

  for (end += 3; p < end; p++) {
    if (p[0] == 0 && p[1] == 0 && p[2] == 1)
      return p;
  }

  return end + 3;
}

const uint8_t* find_h264_startcode(const uint8_t* p, const uint8_t* end) {
  const uint8_t* out = find_startcode_internal(p, end);
  if (p < out && out < end && !out[-1])
    out--;
  return out;
}
