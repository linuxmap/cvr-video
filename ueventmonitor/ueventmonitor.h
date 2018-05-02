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

#ifndef __UEVENTMONITOR_H__
#define __UEVENTMONITOR_H__

#include <stdio.h>
#include <stdbool.h>

struct _uevent {
    char *strs[30];
    int size;
};

enum battery_event_cmd {
    CMD_UPDATE_CAP,
    CMD_LOW_BATTERY,
    CMD_DISCHARGE
};

bool charging_status_check(void);
int batt_reg_event_callback(void (*call)(int cmd, void *msg0, void *msg1));
int hdmi_reg_event_callback(void (*call)(int cmd, void *msg0, void *msg1));
int cvbsout_reg_event_callback(void (*call)(int cmd, void *msg0, void *msg1));
int camera_reg_event_callback(void (*call)(int cmd, void *msg0, void *msg1));
int uevent_monitor_run(void);
int gpios_reg_event_callback(void (*call)(int cmd, void *msg0, void *msg1));
void cvr_check_reverse_system(void);

#endif
