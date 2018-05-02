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

#include "mpp_inc.h"

MppFrameFormat ConvertToMppPixFmt(const PixelFormat& fmt) {
  // static const MppFrameFormat invalid_mpp_fmt = static_cast<MppFrameFormat>(-1);
  static_assert(PIX_FMT_YUV420P == 0, "The index should greater than 0\n");
  static MppFrameFormat mpp_fmts[PIX_FMT_NB] =
      {[PIX_FMT_YUV420P] = MPP_FMT_YUV420P,
       [PIX_FMT_NV12] = MPP_FMT_YUV420SP,
       [PIX_FMT_NV21] = MPP_FMT_YUV420SP_VU,
       [PIX_FMT_YUV422P] = MPP_FMT_YUV422P,
       [PIX_FMT_NV16] = MPP_FMT_YUV422SP,
       [PIX_FMT_NV61] = MPP_FMT_YUV422SP_VU,
       [PIX_FMT_YVYU422] = MPP_FMT_YUV422_YUYV,
       [PIX_FMT_UYVY422] = MPP_FMT_YUV422_UYVY,
       [PIX_FMT_RGB565LE] = MPP_FMT_RGB565,
       [PIX_FMT_BGR565LE] = MPP_FMT_BGR565,
       [PIX_FMT_RGB24] = MPP_FMT_RGB888,
       [PIX_FMT_BGR24] = MPP_FMT_BGR888,
       [PIX_FMT_RGB32] = MPP_FMT_ARGB8888,
       [PIX_FMT_BGR32] = MPP_FMT_ABGR8888};
  assert(fmt >= 0 && fmt < PIX_FMT_NB);
  return mpp_fmts[fmt];
}

PixelFormat ConvertToPixFmt(const MppFrameFormat& mfmt) {
  switch (mfmt) {
    case MPP_FMT_YUV420P:
      return PIX_FMT_YUV420P;
    case MPP_FMT_YUV420SP:
      return PIX_FMT_NV12;
    case MPP_FMT_YUV420SP_VU:
      return PIX_FMT_NV21;
    case MPP_FMT_YUV422P:
      return PIX_FMT_YUV422P;
    case MPP_FMT_YUV422SP:
      return PIX_FMT_NV16;
    case MPP_FMT_YUV422SP_VU:
      return PIX_FMT_NV61;
    case MPP_FMT_YUV422_YUYV:
      return PIX_FMT_YVYU422;
    case MPP_FMT_YUV422_UYVY:
      return PIX_FMT_UYVY422;
    case MPP_FMT_RGB565:
      return PIX_FMT_RGB565LE;
    case MPP_FMT_BGR565:
      return PIX_FMT_BGR565LE;
    case MPP_FMT_RGB888:
      return PIX_FMT_RGB24;
    case MPP_FMT_BGR888:
      return PIX_FMT_BGR24;
    case MPP_FMT_ARGB8888:
      return PIX_FMT_RGB32;
    case MPP_FMT_ABGR8888:
      return PIX_FMT_BGR32;
    default:
      printf("unsupport for pixel fmt: %d\n", mfmt);
      return PIX_FMT_NONE;
  }
}
