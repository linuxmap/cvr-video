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

#include "collision_algorithm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static float fir_coef[COL_N] = {
    -0.010723, -0.031768, -0.044994, -0.042405, -0.019299,
    0.023748,  0.080370,  0.139643,  0.188796,  0.216632,
    0.216632,  0.188796,  0.139643,  0.080370,  0.023748,
    -0.019299, -0.042405, -0.044994, -0.031768, -0.010723,
};
struct feature f_data;
struct gsensor_axis_data data_out;
struct col_state cur_state;
static float a_x[COL_M] = { 0 }, a_y[COL_M] = { 0 }, a_z[COL_M] = { 0 };
enum collision_state state_flag = 0;

static void fre_extract(struct gsensor_axis_data data)
{
    int i = 0, j = 0, k = 0;
    int n = 0, m = 0;
    float smv[COL_M], smv_avg = 0, smv_sigma = 0;
    float angle_avgx = 0, angle_avgy = 0, angle_avgz = 0;
    float x = 0, y = 0, z = 0;
    float tmp_x = 0.0, tmp_y = 0.0, tmp_z = 0.0;
    float smv_f[COL_FRAME];
    float angle_x[COL_FRAME], angle_y[COL_FRAME], angle_z[COL_FRAME];

    for (i = 0; i < (2 * COL_M) / 3; i++) {
        a_x[i] = a_x[i + COL_M / 3];
        a_y[i] = a_y[i + COL_M / 3];
        a_z[i] = a_z[i + COL_M / 3];
    }
    for (i = (2 * COL_M) / 3; i < COL_M; i++) {
        a_x[i] = data.sensor_axis[i - (2 * COL_M) / 3].x;
        a_y[i] = data.sensor_axis[i - (2 * COL_M) / 3].y;
        a_z[i] = data.sensor_axis[i - (2 * COL_M) / 3].z;
    }
    for (i = 0; i < COL_M; i++) {
        x += a_x[i];
        y += a_y[i];
        z += a_z[i];
    }
    x = x / COL_M;
    y = y / COL_M;
    z = z / COL_M;

    /* preprocessing: removing direct current */
    for (i = 0; i < COL_M; i++) {
        a_x[i] = a_x[i] - x;
        a_y[i] = a_y[i] - y;
        a_z[i] = a_z[i] - z;
        smv[i] = a_x[i] * a_x[i] + a_y[i] * a_y[i] + a_z[i] * a_z[i];
        smv[i] = sqrt(smv[i]) / COL_G;
    }

    /* FIR filter */
    for (n = COL_M / 3; n < (2 * COL_M) / 3; n++) {
        smv_f[n - COL_M / 3] = 0;
        data_out.sensor_axis[n - COL_M / 3].x = 0;
        data_out.sensor_axis[n - COL_M / 3].y = 0;
        data_out.sensor_axis[n - COL_M / 3].z = 0;
        for (m = -COL_N / 2; m < COL_N / 2; m++) {
            if ((n - m) < 0) {
                smv_f[n - COL_M / 3] += 0;
                data_out.sensor_axis[n - COL_M / 3].x += 0;
                data_out.sensor_axis[n - COL_M / 3].y = 0;
                data_out.sensor_axis[n - COL_M / 3].z = 0;
            } else {
                i = n - COL_M / 3;
                j = n - m;
                k = m + COL_N / 2;
                smv_f[i] += smv[j] * fir_coef[k];
                tmp_x = a_x[j] * fir_coef[k];
                tmp_y = a_y[j] * fir_coef[k];
                tmp_z = a_z[j] * fir_coef[k];
                data_out.sensor_axis[i].x += tmp_x;
                data_out.sensor_axis[i].y += tmp_y;
                data_out.sensor_axis[i].z += tmp_z;
            }
        }
    }
    for (i = 0; i < COL_FRAME; i++) {
        tmp_x = data.sensor_axis[i].x;
        tmp_y = data.sensor_axis[i].y;
        tmp_z = data.sensor_axis[i].z;
        smv_avg += smv_f[i];
        angle_x[i] = tmp_y * tmp_y + tmp_z * tmp_z;
        angle_x[i] = atan(tmp_x / sqrt(angle_x[i])) * 180 / COL_PI;
        angle_y[i] = tmp_x * tmp_x + tmp_z * tmp_z;
        angle_y[i] = atan(tmp_y / sqrt(angle_y[i])) * 180 / COL_PI;
        angle_z[i] = tmp_x * tmp_x + tmp_y * tmp_y;
        angle_z[i] = atan(sqrt(angle_z[i]) / tmp_z) * 180 / COL_PI;
        angle_avgx += angle_x[i];
        angle_avgy += angle_y[i];
        angle_avgz += angle_z[i];
    }

    /* average */
    smv_avg = smv_avg / COL_FRAME;
    angle_avgx = angle_avgx / COL_FRAME;
    angle_avgy = angle_avgy / COL_FRAME;
    angle_avgz = angle_avgz / COL_FRAME;

    /* var */
    for (i = 0; i < COL_FRAME; i++)
        smv_sigma += (smv_f[i] - smv_avg) * (smv_f[i] - smv_avg);
    smv_sigma = smv_sigma / (COL_FRAME - 1);
    f_data.smv_avg = smv_avg;
    f_data.smv_sigma = smv_sigma;
    f_data.angle_x = angle_avgx;
    f_data.angle_y = angle_avgy;
    f_data.angle_z = fabs(angle_avgz);
}

enum collision_state collision_check(struct gsensor_axis_data *data_input, int level)
{
    float avg_th_l, avg_th_h;
    float sigma_th_l, sigma_th_m, sigma_th_h;
    float sigma = 0;
    float avg = 0;
    struct gsensor_axis_data data = *data_input;

    fre_extract(data);
    sigma = f_data.smv_sigma;
    avg = f_data.smv_avg;

    cur_state.frame_count_in++;

    /* level = 1 indicate low sensitivity;
     * level = 2 indicate middle sensitivity;
     * level = 3 indicate high sensitivity;
     * the thresholds is obtained by experiment,
     * can modify the threshold refer to the document
     */
    switch (level) {
    case 1:
        sigma_th_h = 0.3;
        sigma_th_m = 0.15;
        sigma_th_l = 0.1;
        avg_th_h = 1.2;
        avg_th_l = 0.6;
        break;
    case 2:
        sigma_th_h = 0.2;
        sigma_th_m = 0.12;
        sigma_th_l = 0.08;
        avg_th_h = 1;
        avg_th_l = 0.5;
        break;
    case 3:
        sigma_th_h = 0.06;
        sigma_th_m = 0.03;
        sigma_th_l = 0.015;
        avg_th_h = 0.4;
        avg_th_l = 0.2;
        break;
    default:
        return COLLISION_ERROR;
    }
    if (cur_state.frame_count_in < 10) {
        state_flag = COLLISION_NOTHING;
        cur_state.state_count = 0;
    } else if (sigma < sigma_th_l) {
        cur_state.state_count = 0;
        state_flag = COLLISION_NOTHING;
    } else if ((sigma_th_l <= sigma) && (sigma < sigma_th_m)) {
        if (((avg_th_l <= avg) && (avg < avg_th_h)) || (state_flag == 2)) {
            if (cur_state.state_count >= 3) {
                cur_state.state_count = 0;
                state_flag = COLLISION_HAPPEN;
                printf("1:collision happening\n");
            } else {
                cur_state.state_count++;
                state_flag = COLLISION_CONTINUE;
            }
        } else if (avg >= avg_th_h) {
            cur_state.state_count = 0;
            state_flag = COLLISION_HAPPEN;
            printf("2:collision happening\n");
        }
    } else if ((sigma_th_m <= sigma) && (sigma < sigma_th_h)) {
        if ((avg >= avg_th_l) || (cur_state.state_count >= 3)) {
            cur_state.state_count = 0;
            state_flag = COLLISION_HAPPEN;
            printf("3:collision happening\n");
        } else {
            cur_state.state_count++;
            state_flag = COLLISION_CONTINUE;
        }
    } else {
        cur_state.state_count = 0;
        state_flag = COLLISION_HAPPEN;
        printf("4:collision happening\n");
    }
    if (state_flag == COLLISION_CONTINUE)
        return COLLISION_NOTHING;

    return state_flag;
}
