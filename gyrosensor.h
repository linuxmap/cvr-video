/**
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd
 * Author: Wang RuoMing <wrm@rock-chips.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Freeoftware Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.ee the
 * GNU General Public License for more details.
 */

#ifndef __GYROSENSOR_H__
#define __GYROSENSOR_H__

#define MAX_NUM         200
#define GYRO_DIRECT_CALIBRATION 1
#define GYRO_SET_CALIBRATION    2

struct sensor_axis_data {
    int x;
    int y;
    int z;
};

struct sensor_data {
    long long timestamp_us;
    short temperature;
    long quaternion[4];
    struct sensor_axis_data gyro_axis;
    struct sensor_axis_data accel_axis;
    int quat_flag;
};

struct sensor_data_set {
    struct sensor_data data[MAX_NUM];
    int count;
};

#ifdef __cplusplus
extern "C" {
#endif

int gyrosensor_init(void);
void gyrosensor_deinit(void);
int gyrosensor_calibration(int status);
int gyrosensor_direct_getdata(struct sensor_data_set *data);
int gyrosensor_start(void);
int gyrosensor_stop(void);

#ifdef __cplusplus
}
#endif

#endif
