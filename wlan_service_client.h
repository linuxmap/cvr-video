/**
 * Copyright (C) 2016 Fuzhou Rockchip Electronics Co., Ltd
 * author: ZhiChao Yu zhichao.yu@rock-chips.com
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
#ifndef WLAN_SERVICE_CLIENT_H_
#define WLAN_SERVICE_CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif

int WlanServiceSetPower(const char* power);
int WlanServiceSetMode(const char* mode, const char* ssid, const char* password);

#ifdef __cplusplus
}
#endif

#endif // WLAN_SERVICE_CLIENT_H_