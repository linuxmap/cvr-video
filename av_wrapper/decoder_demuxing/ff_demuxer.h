/**
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd
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

#ifndef FF_DEMUXER_H
#define FF_DEMUXER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

int get_file_duration(char* file_path, int64_t* duration);

#ifdef __cplusplus
}
#endif

#endif  // FF_DEMUXER_H
