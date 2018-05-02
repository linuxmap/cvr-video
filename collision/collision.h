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

#ifndef __COLLISION_H_
#define __COLLISION_H_

#include "../gsensor.h"
#include "../parameter.h"

#define CMD_COLLISION            0
#define COLLISION_CACHE_DURATION 5

int collision_init(void);
int collision_register(void);
int collision_unregister(void);

int collision_regeventcallback(int (*call)(int cmd, void *msg0, void *msg1));

#endif
