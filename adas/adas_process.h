/**
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd
 * Author: Zain Wang<wzz@rock-chips.com>
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

#ifndef __ADAS_PROCESS_H_
#define __ADAS_PROCESS_H_

#include <stdbool.h>
#include <dpp/algo/adas/adas.h>
#include <pthread.h>

#define ADAS_ALERT_STOP                 1
#define ADAS_ALERT_RUN                  2

#define ADAS_CAR_STATUS_UNKNOWN         0
#define ADAS_CAR_STATUS_STOP            1
#define ADAS_CAR_STATUS_PACE            2
#define ADAS_CAR_STATUS_RUN             3

#define ADAS_SPEED_SLOW                 2
#define ADAS_SPEED_FAST                 50

struct adas_parameter {
    char adas_state;
    char adas_setting[2];
    char adas_alert_distance;
    char adas_direction;

    float coefficient;
    char base;
};

struct adas_info {
    int width;
    int height;
    int gsensor_reg;
    pthread_mutex_t data_mutex;
    pthread_mutex_t process_mutex;
    void (*callback)(int cmd, void *msg0, void *msg1);
    bool is_gps_support;
    bool is_gs_support;
};

struct adas_img {
    int x;
    int y;
    int width;
    int height;
};

struct adas_fcw {
    int alert;
    float distance;
    struct adas_img img;
};

struct adas_pos {
    int x;
    int y;
};

struct adas_process_output {
    int fcw_count;
    int fcw_alert;
    struct adas_fcw fcw_out[ODT_MAX_OBJECT];
    /* 0: LDW invalid 1: turn left 2: normal 3 turn right */
    int ldw_flag;
    struct adas_pos ldw_pos[4];
    float speed;
    int car_status;
};

int AdasInit(int width, int height);
void AdasDeinit(void);
int AdasOn(void);
int AdasOff(void);
void AdasSetWH(int width, int height);
void AdasGetParameter(struct adas_parameter *parameter);
void AdasSaveParameter(struct adas_parameter *parameter);
int AdasEventRegCallback(void (*call)(int cmd, void *msg0, void *msg1));
void AdasGpsUpdate(void *gps);

#endif
