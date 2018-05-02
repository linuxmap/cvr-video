/**
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd
 * author: Chad.ma <chad.ma@rock-chips.com>
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

#ifndef __LICENSE_PLATE_H__
#define __LICENSE_PLATE_H__

#include <stdbool.h>
#include <stdint.h>

#define PROVINCE_ABBR_MAX  37
#define LICENSE_CHAR_MAX   39
#define MAX_LICN_NUM        8
#define ONE_CHN_SIZE        3
#define ONE_CHAR_SIZE       2

bool is_licenseplate_valid(char *licenStr);
void save_licenseplate(char *licnplate);
void get_licenseplate_and_pos(char *licnplate,int plate_size,uint32_t *licn_pos);

#endif  /* __LICENSE_PLATE_H__ */
