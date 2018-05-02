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

#include "collision.h"
#include "collision_algorithm.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/prctl.h>

#define FRAME 10

struct collision_data {
	int num;
	int datainit;
	int pthread_state;
	int pthread_exit;
	int collision_flag;
	int frame_count;

	pthread_t tid;

	struct sensor_axis cls_data;
};

struct collision_data cls;
struct gsensor_axis_data data_input;

int (*collision_event_call)(int cmd, void *msg0, void *msg1);

static int collision_deinit(void);
static int collision_process(void);
static int collision_create_pthread(void);
static int collision_delete_pthread(void);

int collision_get_gsdata(struct sensor_axis data)
{
	int ret = 0;
	char cls_level = 0;

	cls_level = parameter_get_collision_level();

	if (cls.datainit == 0) {
		cls.cls_data.x = data.x;
		cls.cls_data.y = data.y;
		cls.cls_data.z = data.z;
		cls.datainit = 1;
	} else {
		if (cls.frame_count < FRAME) {
			data_input.sensor_axis[cls.frame_count].x = data.x;
			data_input.sensor_axis[cls.frame_count].y = data.y;
			data_input.sensor_axis[cls.frame_count].z = data.z;
			cls.frame_count++;
		} else if (cls.frame_count == 10) {
			ret = collision_check(&data_input, cls_level);
			if ((ret > 0) && (ret <= 3))
				cls.collision_flag = 1;

			cls.frame_count = 0;
		}
	}

	return 0;
}

static void *collision_pthread(void *arg)
{
	int time = 0;

	cls.pthread_exit = 0;

	prctl(PR_SET_NAME, "collision", 0, 0, 0);

	while (!cls.pthread_exit) {
		if (cls.collision_flag == 1) {
			printf("collision flag=%d\n", cls.collision_flag);
			time = collision_process();
			if ((time > 0) && (time <= COLLISION_CACHE_DURATION))
				sleep(time);
			cls.collision_flag = 0;
		}
		usleep(500000);
	}
	cls.pthread_state = 0;
	collision_delete_pthread();

	return 0;
}

static int collision_delete_pthread(void)
{
	pthread_detach(pthread_self());
	pthread_exit(0);
}

static int collision_create_pthread(void)
{
	if (pthread_create(&cls.tid, NULL, collision_pthread, NULL)) {
		printf("create collision_thread pthread failed\n");
		cls.pthread_state = 0;
		return -1;
	}

	return 0;
}

int collision_register(void)
{
	if (cls.num == 0) {
		cls.num = gsensor_regsenddatafunc(collision_get_gsdata);
		if (cls.num > 0) {
			cls.pthread_state = 1;
			collision_create_pthread();
		}
	}

	return 0;
}

int collision_unregister(void)
{
	int ret = -1;

	if (cls.num != 0) {
		ret = gsensor_unregsenddatafunc(cls.num);
		if ((ret == 0) && (cls.pthread_state == 1)) {
			cls.pthread_exit = 1;
			pthread_join(cls.tid, NULL);
		}
		collision_deinit();
	}

	return 0;
}

int collision_regeventcallback(int (*call)(int cmd, void *msg0, void *msg1))
{
	collision_event_call = call;

	return 0;
}

static int collision_deinit(void)
{
	memset(&cls, 0, sizeof(cls));

	return 0;
}

int collision_init(void)
{
	memset(&cls, 0, sizeof(cls));

	return 0;
}

static int collision_process(void)
{
	if (collision_event_call)
		return (*collision_event_call)(CMD_COLLISION, 0, 0);

	return 0;
}
