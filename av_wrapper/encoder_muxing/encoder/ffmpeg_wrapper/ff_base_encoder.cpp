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

#include "ff_base_encoder.h"

FFBaseEncoder::FFBaseEncoder(char* codec_name) {
  ff_ctx_.SetCodecName(codec_name);
}

FFBaseEncoder::FFBaseEncoder(AVCodecID codec_id) {
  ff_ctx_.SetCodecId(codec_id);
}

int FFBaseEncoder::InitConfig(VideoConfig& vconfig) {
  return ff_ctx_.InitConfig(vconfig);
}

int FFBaseEncoder::InitConfig(AudioConfig& aconfig) {
  return ff_ctx_.InitConfig(aconfig);
}
