/**
 * Copyright (C) 2016 Fuzhou Rockchip Electronics Co., Ltd
 * Author: Cain Cai <cain.cai@rock-chips.com>
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

#ifndef __THERMAL_H__
#define __THERMAL_H__

#include <stdbool.h>

#define THRESHOLD_MAX 5

enum {
    THERMAL_LEVEL0 = 0,
    THERMAL_LEVEL1,
    THERMAL_LEVEL2,
    THERMAL_LEVEL3,
    THERMAL_LEVEL4,
    THERMAL_LEVEL5,
};

struct thermal_dev_t {
    char *name;
    int temp;
    int ltemp;
    int htemp;
    int state;
    int old_state;
    int lthreshold[THRESHOLD_MAX];
    int hthreshold[THRESHOLD_MAX];
    bool stop_kernel_thermal;

    bool handle_brightness;
    int brightness;
    int cur_brightness;

    bool handle_wifi;
    bool off_wifi;

    bool handle_fb_fps;
    int fb_fps;
    int cur_fb_fps;

    bool handle_3dnr;
    bool off_3dnr;

    bool handle_ddr;
    bool ddr_low_power;
    char ddr_state;
};

int thermal_init(void);
void thermal_update_state(void);
int thermal_get_status(void);

#endif
