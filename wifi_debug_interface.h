/**
 * Copyright (C) 2016 Fuzhou Rockchip Electronics Co., Ltd
 * author: Benjo Lei <benjo.lei@rock-chips.com>
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

#ifndef __WIFI_DEBUG_INTERFACE_H__
#define __WIFI_DEBUG_INTERFACE_H__

int debug_setting_cmd_resolve(int nfd, char *cmdstr);
int debug_setting_get_cmd_resolve(int nfp, char *buffer);

#endif
