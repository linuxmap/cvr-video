/* Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd
 * Author: cherry chen <cherry.chen@rock-chips.com>
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

#include "../gsensor.h"

#define COL_G 9.8
#define COL_PI 3.1415926
#define COL_M 30
#define COL_N 20
#define COL_FRAME 10

struct gsensor_axis_data {
    struct sensor_axis sensor_axis[COL_FRAME];
};

struct feature {
    float smv_avg;
    float smv_sigma;
    float smv_max;
    float smv_min;
    float angle_x;
    float angle_y;
    float angle_z;
};

enum collision_state {
    COLLISION_NOTHING = 0,
    COLLISION_HAPPEN,
    COLLISION_CONTINUE,
    COLLISION_TURNOVER,
    COLLISION_ERROR
};

struct col_state {
    int frame_count_in;
    int state_count;

    /* Not be used now, maybe add rollover detection then */
    int rollover_flag;
    int rollver_count;
};

enum collision_state collision_check(struct gsensor_axis_data *data_input, int level);
