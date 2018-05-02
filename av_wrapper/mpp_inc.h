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

#ifndef MPP_INC_H
#define MPP_INC_H

#include <mpp/mpp_frame.h>
#include "encoder_muxing/encoder/base_encoder.h"

MppFrameFormat ConvertToMppPixFmt(const PixelFormat& fmt);
PixelFormat ConvertToPixFmt(const MppFrameFormat& mfmt);

#endif // MPP_INC_H
