/**
 * Copyright (C) 2016 Fuzhou Rockchip Electronics Co., Ltd
 * author: Huaping Liao <huaping.liao@rock-chips.com>
 *         Jinkun Hong <hjk@rock-chips.com>
 *         Chris Zhong <zyw@rock-chips.com>
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

#ifndef __USB_SD_CTRL_H__
#define __USB_SD_CTRL_H__

#include <stdio.h>
#include "ueventmonitor.h"

#define SD_CHANGE   0
#define SD_MOUNT_FAIL   1

enum usb_sd_type {
    SD_CTRL,
    USB_CTRL,
    USB_STORAGE_CTRL
};

enum sd_state_type {
    SD_STATE_OUT,
    SD_STATE_IN,
    SD_STATE_ERR,
    SD_STATE_FORMATING
};

int sd_reg_event_callback(void (*call)(int cmd, void *msg0, void *msg1));
int usb_reg_event_callback(void (*call)(int cmd, void *msg0, void *msg1));
void cvr_sd_ctl(char mount);
void android_usb_init(void);
void cvr_android_usb_ctl(const struct _uevent *event);
void cvr_usb_sd_ctl(enum usb_sd_type type, char state);
void selct_disk_charging(int disk);
void android_usb_config_ums(void);
void android_usb_config_adb(void);
void android_usb_config_rndis(void);
void android_usb_config_uvc(void);
void set_sd_mount_state(int state);
int get_sd_mount_state(void);
bool cvr_usb_mode_switch(const struct _uevent *event);
void android_adb_keeplive(void);

#endif
