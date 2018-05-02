/**
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd
 * author: Tianfeng Chen <ctf@rock-chips.com>
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

#ifndef __GPS_CTRL_H__
#define __GPS_CTRL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#define GPS_DEBUG
#ifdef GPS_DEBUG
#define GPS_DBG(...) printf(__VA_ARGS__)
#else
#define GPS_DBG(...)
#endif

#define FILE_NAME_LEN 128
#define GPS_BUF_LEN 200
#define MAX_GPS_INFO_LEN 50

typedef void (* send_cb)(const char *, int, int);

struct gps_data {
    int len;
    char buffer[GPS_BUF_LEN];
};

struct gps_send_data {
    send_cb send_data;
    struct gps_send_data *pre;
    struct gps_send_data *next;
};

int gps_init(send_cb gps_send_callback);
int gps_deinit(void);

int gps_write_init(void);
void gps_write_deinit(void);

void gps_file_open(const char *filename);
void gps_file_close(void);

#if USE_GPS_MOVTEXT
void gps_set_encoderhandler(void * handler);
#endif

#ifdef __cplusplus
}
#endif
#endif
