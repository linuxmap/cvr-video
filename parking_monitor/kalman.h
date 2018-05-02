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

#define M 1
#define MAX_BUF_LEN 100

struct kalman {
    float Kk;
    float P;
    float Q;
    float R;
    float ky0;
};
struct sensor_data {
    float sensor_a[MAX_BUF_LEN];
};

struct gsensor_data {
    struct sensor_axis sa[MAX_BUF_LEN];
};

void kalman_init(struct kalman *k);
int sliding_filter(struct kalman* k, struct gsensor_data* gd1, int len, double thr);
