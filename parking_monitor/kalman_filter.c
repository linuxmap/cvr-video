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

#include<stdio.h>

#include "kalman.h"
#include"../gsensor.h"

void kalman_init(struct kalman *k)
{
    k->P = 10;
    k->Q = 0.01;
    k->R = 10;
    k->ky0 = 9.8;
}

static float kalman_filter(struct kalman *k, float y0, float u)
{
    float y;
    float P01 = k->P + k->Q;

    k->Kk = P01 / (P01 + k->R);
    y = y0 + k->Kk * (u - y0);
    k->P = (1 - k->Kk) * P01;

    return y;
}
/* k: kalman parameter
 * gd1: gsensor data
 * len : length of gsensor data during 1 second
 * thr : threshold ofjudging parking state, approach to zero in theory
 */
int sliding_filter(struct kalman *k, struct gsensor_data *gd1, int len, double thr)
{
    int i;
    float meana = 0;
    float vara = 0;
    int state = 0;
    float y[MAX_BUF_LEN];
    struct sensor_data sd;

    if (len > MAX_BUF_LEN) {
        printf("Samplaterate outsize!!!\n");
        return -1;
    }
    for (i = 0; i < len; i++) {
        sd.sensor_a[i] = gd1->sa[i].x + gd1->sa[i].y + gd1->sa[i].z;
        y[i] = kalman_filter(k, k->ky0, sd.sensor_a[i]);
        k->ky0 = y[i];
        meana += y[i];
    }
    meana = meana / len;
    for (i = 0; i < len; i++) {
        vara += (y[i] - meana) * (y[i] - meana);
    }
    vara = vara / len;

    /* Parking detection rule */
    if (vara > thr)
        state = 1;
    else
        state = 0;

    return state;
}
