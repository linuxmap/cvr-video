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

#ifndef __WIFI_SETTING_INTERFACE_H__
#define __WIFI_SETTING_INTERFACE_H__

int setting_get_cmd_resolve(int nfp, char *buffer);
int tcp_func_get_on_off_recording(int nfp, char *buffer);
int tcp_func_get_ssid_pw(int nfp, char *buffer);
int tcp_func_get_back_camera_apesplution(int nfp, char *buffer);
int tcp_func_get_front_camera_apesplution(int nfp, char *buffer);
int tcp_func_get_front_setting_apesplution(int nfp, char * buffer);
int tcp_func_get_back_setting_apesplution(int nfp, char * buffer);
int tcp_func_get_format_status(int nfp, char *buffer);
int tcp_func_get_photo_quality(int nfp, char *buffer);
int tcp_func_get_setting_photo_quality(int nfp, char *buffer);
int setting_cmd_resolve(int nfd, char *cmdstr);
int debug_setting_get_cmd_resolve(int nfp, char *buffer);
void reg_msg_manager_cb(void);
void unreg_msg_manager_cb(void);
#endif
