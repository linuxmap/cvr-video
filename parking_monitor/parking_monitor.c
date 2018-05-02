/**
 * Copyright (C) 2016 Fuzhou Rockchip Electronics Co., Ltd
 * Author: Wang RuoMing <wrm@rock-chips.com>
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

#include "parking_monitor.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/prctl.h>

#define PARKING_LEVEL           1

struct parking_monitor {
    int num;
    int datainit;
    int pthread_state;
    int pthread_exit;
    int parking_flag;

    pthread_t tid;

    struct sensor_axis pm_data;
};

struct parking_monitor pm;

void (*parking_event_call)(int cmd, void *msg0, void *msg1);

static int parking_deinit(void);
static int parking_check(struct sensor_axis data, int threshold);
static void parking_process(void);
static int parking_create_pthread(void);
static int parking_delete_pthread(void);

int parking_get_gsdata(struct sensor_axis data)
{
    int ret = 0;
    char pm_level = PARKING_LEVEL;

    if (pm.datainit == 0) {
        pm.pm_data.x = data.x;
        pm.pm_data.y = data.y;
        pm.pm_data.z = data.z;
        pm.datainit = 1;
    } else {
        ret = parking_check(data, pm_level);
        if (ret == 1)
            pm.parking_flag = 1;
    }

    return 0;
}

static void *parking_pthread(void *arg)
{
    pm.pthread_exit = 0;

    prctl(PR_SET_NAME, "parking_pthread", 0, 0, 0);

    while (!pm.pthread_exit) {
        if (pm.parking_flag == 1) {
            parking_process();
            pm.parking_flag = 0;
        }
        usleep(5000);
    }
    pm.pthread_state = 0;
    parking_delete_pthread();

    return 0;
}

static int parking_delete_pthread(void)
{
    pthread_detach(pthread_self());
    pthread_exit(0);
}

static int parking_create_pthread(void)
{
    if (pthread_create(&pm.tid, NULL, parking_pthread, NULL)) {
        printf("create parking_thread pthread failed\n");
        pm.pthread_state = 0;
        return -1;
    }

    return 0;
}

int parking_register(void)
{
    if (pm.num == 0) {
        pm.num = gsensor_regsenddatafunc(parking_get_gsdata);
        if (pm.num > 0) {
            pm.pthread_state = 1;
            parking_create_pthread();
        }
    }

    return 0;
}

int parking_unregister(void)
{
    int ret = -1;

    if (pm.num != 0) {
        ret = gsensor_unregsenddatafunc(pm.num);
        if ((ret == 0) && (pm.pthread_state == 1)) {
            pm.pthread_exit = 1;
            pthread_join(pm.tid, NULL);
        }
        parking_deinit();
    }

    return 0;
}

int parking_regeventcallback(void (*call)(int cmd, void *msg0, void *msg1))
{
    parking_event_call = call;

    return 0;
}

static int parking_deinit(void)
{
    memset(&pm, 0, sizeof(pm));

    return 0;
}

int parking_init(void)
{
    memset(&pm, 0, sizeof(pm));

    return 0;
}

static void parking_process(void)
{
    if (parking_event_call)
        (*parking_event_call)(CMD_PARKING, 0, 0);
}

static int parking_check(struct sensor_axis data, int threshold)
{
    int ret = 0;

    if ((data.x > (pm.pm_data.x + threshold)) ||
        (data.x < (pm.pm_data.x - threshold))) {
        ret = 1;
    }
    if ((data.y > (pm.pm_data.y + threshold)) ||
        (data.y < (pm.pm_data.y - threshold))) {
        ret = 1;
    }
    if ((data.z > (pm.pm_data.z + threshold)) ||
        (data.z < (pm.pm_data.z - threshold))) {
        ret = 1;
    }

    /* update parking gsensor data */
    pm.pm_data.x = data.x;
    pm.pm_data.y = data.y;
    pm.pm_data.z = data.z;

    return ret;
}
