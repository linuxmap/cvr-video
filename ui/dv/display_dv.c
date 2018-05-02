/**
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd
 * author: hogan.wang@rock-chips.com
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

#include "display_dv.h"
#include "display.h"
#include "ui_resolution.h"

#define DV_WINDOW_NUM 1

static struct display_window dv_window[DV_WINDOW_NUM] = {
    { 0, (WIN_H - WIN_W * 9 / 16) / 2, WIN_W, WIN_W * 9 / 16 },
};

static bool dv_position[DV_WINDOW_NUM];

void set_display_dv_window(void)
{
    set_display_window(dv_window, DV_WINDOW_NUM, dv_position);
}
