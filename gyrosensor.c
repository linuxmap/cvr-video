/**
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd
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

#include "gyrosensor.h"

#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>

#include "parameter.h"

#define GYROSENSOR_DISABLE          0
#define GYROSENSOR_ENABLE           1

#define GYROSDEV "/dev/gyrosensor"

struct raw_data {
    struct sensor_axis_data gyro_axis;
    struct sensor_axis_data accel_axis;
    long long timestamp_usec;
    short temperature;
    long quaternion[4];
    int quat_flag;
    int use_irq;
};

struct raw_data_set {
    struct raw_data data[MAX_NUM];
    int avail_num;
};

struct gyrosensor {
    int fd;
    int thread_state;
    int state;
    int release_state;
    int auto_tc;
    int auto_tc_failure;
    int t_p;
    int t_n;

    pthread_mutex_t gs_mutex;
    pthread_t tid;
};

static struct raw_data_set *fifodata;

#define GYROSENSOR_IOCTL_MAGIC      'f'
#define GYROSENSOR_IOCTL_CLOSE      _IO(GYROSENSOR_IOCTL_MAGIC, 0x00)
#define GYROSENSOR_IOCTL_START      _IO(GYROSENSOR_IOCTL_MAGIC, 0x01)
#define GYROSENSOR_IOCTL_GETDATA        _IOWR(GYROSENSOR_IOCTL_MAGIC, 0x03, struct raw_data_set)
#define GYROSENSOR_IOCTL_CALIBRATION    _IOWR(GYROSENSOR_IOCTL_MAGIC, 0x04, char *)
#define GYROSENSOR_IOCTL_GETDATA_FIFO   _IOWR(GYROSENSOR_IOCTL_MAGIC, 0x05, struct raw_data_set)

static struct gyrosensor sensor;

static int gyrosensor_direct_read_fifo_data(struct sensor_data_set *data)
{
    int i;
    int ret;

    ret = ioctl(sensor.fd, GYROSENSOR_IOCTL_GETDATA, fifodata);
    if (ret < 0) {
        printf("GYROSENSOR_IOCTL_GETDATA error----ret =%d num=%d\n",
               ret, fifodata->avail_num);
        return -1;
    }

    for (i = 0; i < fifodata->avail_num; i++ ) {
        data->data[i].quat_flag = fifodata->data[i].quat_flag;
        if (fifodata->data[i].quat_flag == 0) {
            if (fifodata->data[i].use_irq)
                data->data[i].timestamp_us = fifodata->data[i].timestamp_usec;
            else
                data->data[i].timestamp_us = fifodata->data[i].timestamp_usec / 1000;

            data->data[i].gyro_axis.x = fifodata->data[i].gyro_axis.x;
            data->data[i].gyro_axis.y = fifodata->data[i].gyro_axis.y;
            data->data[i].gyro_axis.z = fifodata->data[i].gyro_axis.z;
            data->data[i].accel_axis.x = fifodata->data[i].accel_axis.x;
            data->data[i].accel_axis.y = fifodata->data[i].accel_axis.y;
            data->data[i].accel_axis.z = fifodata->data[i].accel_axis.z;
            data->data[i].temperature = fifodata->data[i].temperature;
        } else {
            printf("error: output quat data avail_num=%d\n", fifodata->avail_num);
        }
    }

    data->count = fifodata->avail_num;
    sensor.t_n = fifodata->data[i - 1].temperature;

    return 0;
}

int gyrosensor_direct_getdata(struct sensor_data_set *data)
{
    int ret = 0;

    if (sensor.state != GYROSENSOR_ENABLE) {
        printf("gyrosensor in disable mode\n");
        return -1;
    }

    ret = gyrosensor_direct_read_fifo_data(data);
    if (ret < 0) {
        /* Make sure gyroscope has data */
        usleep(2000);
        ret = gyrosensor_direct_read_fifo_data(data);
        if (ret < 0)
            return ret;
    }

    return 0;
}

static int gyrosensor_enable(int enable)
{
    int ret = -1;

    if (enable) {
        gyrosensor_calibration(GYRO_SET_CALIBRATION);
        ret = ioctl(sensor.fd, GYROSENSOR_IOCTL_START, &enable);
        if (ret < 0) {
            printf("%s:gyrosensor enable failure\n", __func__);
            return -1;
        }
        sensor.state = GYROSENSOR_ENABLE;
    } else {
        ret = ioctl(sensor.fd, GYROSENSOR_IOCTL_CLOSE, &enable);
        if (ret < 0) {
            printf("%s:gyrosensor disable failure\n", __func__);
            return -1;
        }
        sensor.state = GYROSENSOR_DISABLE;
    }

    return ret;
}

static int gyrosensor_delete_thread(void)
{
    pthread_detach(pthread_self());
    pthread_exit(0);

    return 0;
}

static void *gyrosensor_thread(void *arg)
{
    int ret = 0;
    struct timeval time_value;

    prctl(PR_SET_NAME, "gyro_thread", 0, 0, 0);

    while (!sensor.release_state) {
        /* sleep 10000 us */
        time_value.tv_sec = 0;
        time_value.tv_usec = 10000;
        select(0, NULL, NULL, NULL, &time_value);
        if ((sensor.auto_tc == 1) &&
            (sensor.t_p != 0) &&
            (sensor.t_n != 0)) {
            if (sensor.auto_tc_failure != 0) {
                sensor.auto_tc_failure++;
                if (sensor.auto_tc_failure == 500)
                    sensor.auto_tc_failure = 0;
            } else {
                if ((sensor.t_n - sensor.t_p > 3210) ||
                    (sensor.t_p - sensor.t_n > 3210)) {
                    ret = gyrosensor_calibration(GYRO_DIRECT_CALIBRATION);
                    if (ret >= 0)
                        printf("gyro calibration success\n");
                    else
                        sensor.auto_tc_failure = 1;
                }
            }
        }
    }

    free(fifodata);
    printf("gyrosensor_thread PTHREAD_EXIT:\n");
    sensor.release_state = 0;
    sensor.thread_state = 0;
    /* disable gyrosensor */
    ret = gyrosensor_enable(GYROSENSOR_DISABLE);
    if (ret < 0)
        printf ("gyrosensor disable failure\n");
    gyrosensor_delete_thread();

    return 0;
}

static int gyrosensor_create_thread(void)
{
    int ret = 0;

    if (pthread_create(&sensor.tid, NULL, gyrosensor_thread, NULL)) {
        printf("create gyrosensor_thread pthread failed\n");
        sensor.thread_state = 0;
        ret = -1;
    }

    return ret;
}

int gyrosensor_start(void)
{
    int ret = -1;

    pthread_mutex_lock(&sensor.gs_mutex);

    if (sensor.thread_state != 1) {
        sensor.thread_state = 1;

        fifodata = malloc(sizeof(struct raw_data_set));
        if (fifodata == NULL) {
            printf("fifodata malloc failure\n");
            return -1;
        } else {
            printf("fifodata malloc success\n");
            fifodata->avail_num  = 0;
        }

        /* enable gyrosensor */
        ret = gyrosensor_enable(GYROSENSOR_ENABLE);
        if (ret < 0) {
            printf ("gyrosensor enable failure\n");
            free(fifodata);
            return -1;
        }

        gyrosensor_create_thread();
    }

    pthread_mutex_unlock(&sensor.gs_mutex);

    return 0;
}

int gyrosensor_stop(void)
{
    pthread_mutex_lock(&sensor.gs_mutex);

    if (sensor.thread_state == 1) {
        printf("GyroSensor unregister delete thread\n");
        sensor.thread_state = 0;
        sensor.release_state = 1;
        printf("0: sensor.release_state =%d\n", sensor.release_state);
        pthread_join(sensor.tid, NULL);
        printf("1: sensor.release_state =%d\n", sensor.release_state);
    }

    pthread_mutex_unlock(&sensor.gs_mutex);

    return 0;
}

int gyrosensor_calibration(int status)
{
    int ret = 0, i = 0, start_gyro = 0, enable = 0;
    char gyro_offset[9] = {0};
    unsigned char data[8] = {0};

    if ((sensor.state != GYROSENSOR_ENABLE) &&
        (status == GYRO_DIRECT_CALIBRATION)) {
        start_gyro = 1;
        enable = 1;
        ret = ioctl(sensor.fd, GYROSENSOR_IOCTL_START, &enable);
        if (ret < 0) {
            printf("%s:gyrosensor enable failure\n", __func__);
            goto error;
        }
    }

    if (status == GYRO_DIRECT_CALIBRATION) {
        /* direction calibration */
        system("echo 1 >> /sys/class/misc/gyrosensor/device/gyro_calibration");
        gyro_offset[0] = status;
        ret = ioctl(sensor.fd, GYROSENSOR_IOCTL_CALIBRATION, &gyro_offset);
        if (ret < 0) {
            printf("%s:gyrosensor get calibration data failure\n", __func__);
            goto error;
        }
        sensor.t_p = gyro_offset[7];
        sensor.t_p = (sensor.t_p << 8) | gyro_offset[8];
        for (i = 0; i < 8; i++) {
            data[i] = gyro_offset[i + 1];
        }
        parameter_save_gyro_calibration_data(data);
    } else if (status == GYRO_SET_CALIBRATION) {
        /* set calibration offset value */
        gyro_offset[0] = status;
        parameter_get_gyro_calibration_data(data);
        if ((data[0] == 0) && (data[1] == 0) &&
            (data[2] == 0) && (data[3] == 0) &&
            (data[4] == 0) && (data[5] == 0)) {
            printf("%s:gyro offset data is 0, not set calibration\n",
                   __func__);
            ret = -1;
            goto error;
        }
        for (i = 0; i < 8; i++) {
            gyro_offset[i + 1] = data[i];
        }
        sensor.t_p = gyro_offset[7];
        sensor.t_p = (sensor.t_p << 8) | gyro_offset[8];
        ret = ioctl(sensor.fd, GYROSENSOR_IOCTL_CALIBRATION, &gyro_offset);
        if (ret < 0) {
            printf("%s:gyrosensor set calibration data failure\n", __func__);
            goto error;
        }
    }

error:
    if (start_gyro == 1) {
        enable = 0;
        ret = ioctl(sensor.fd, GYROSENSOR_IOCTL_CLOSE, &enable);
        if (ret < 0) {
            printf("%s:gyrosensor disable failure\n", __func__);
            return -1;
        }
    }

    return ret;
}

void gyrosensor_deinit(void)
{
    gyrosensor_stop();
    if (sensor.fd >= 0) {
        close(sensor.fd);
        sensor.fd = -1;
    }

    pthread_mutex_destroy(&sensor.gs_mutex);
}

int gyrosensor_init(void)
{
    printf("%s\n", __func__);
    memset(&sensor, 0, sizeof(sensor));
    sensor.auto_tc = 0;
    sensor.auto_tc_failure = 0;
    sensor.fd = -1;
    pthread_mutex_init(&sensor.gs_mutex, NULL);
    pthread_mutex_lock(&sensor.gs_mutex);

    sensor.fd = open(GYROSDEV, O_RDWR);
    if (sensor.fd < 0) {
        printf("failed to open gyrosensor dev\n");
        pthread_mutex_unlock(&sensor.gs_mutex);
        return -1;
    }

    pthread_mutex_unlock(&sensor.gs_mutex);

    printf("gyro init success\n");

    return 0;
}
