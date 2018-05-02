/**
 * Copyright (C) 2016 Fuzhou Rockchip Electronics Co., Ltd
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


#ifndef __GSENSOR_H__
#define __GSENSOR_H__

#include "common.h"

#define GSENSOR_INT_START       1
#define GSENSOR_INT_STOP        0
#define GSENSOR_INT1            1
#define GSENSOR_INT2            2

struct sensor_axis {
    float x;
    float y;
    float z;
};

typedef int fpI2C_Event(struct sensor_axis, int state);

int gsensor_init(void);
int gsensor_enable(int enable);
void gsensor_release(void);
int gsensor_regsenddatafunc(int (*send)(struct sensor_axis data));
int gsensor_unregsenddatafunc(int num);
int gsensor_use_interrupt(int intr_num, int intr_st);

int I2C_GSensor_Read(int addr, unsigned char *pdata);
int I2C_GSensor_Write(int addr, unsigned char *pdata);
int I2C_RegEventCallback(fpI2C_Event *EventCallBack);

#endif
