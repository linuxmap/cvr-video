/**
 * Copyright (C) 2016 Fuzhou Rockchip Electronics Co., Ltd
 * author: ZhiChao Yu zhichao.yu@rock-chips.com
 *         hertz.wang hertz.wong@rock-chips.com
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

#include "buffer.h"

#include <string.h>

BufferData::BufferData()
    : width_(0), height_(0), vir_addr_(nullptr), mem_size_(0) {
  memset(&update_timeval_, 0, sizeof(update_timeval_));
}

Buffer::Buffer(BufferData& buf_data) : valid_data_size_(0), user_flag_(0) {
  data_ = buf_data;
}

Buffer::~Buffer() {}
