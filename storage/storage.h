/**
 * Copyright (C) 2016 Fuzhou Rockchip Electronics Co., Ltd
 * author: huaping.liao huaping.liao@rock-chips.com
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

#ifndef __STORAGE_H__
#define __STORAGE_H__

enum storage_setting_type {
    SETTING_RECORDTIME = 0,
    SETTING_RESOLUTION,
    SETTING_BITSTREAM
};

void storage_setting_callback(int cmd, void *msg0, void *msg1);
int storage_check_timestamp();

#endif /* __STORAGE_H__ */
