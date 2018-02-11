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

#include "display_cvr.h"
#include "display.h"
#include "ui_resolution.h"

#define CVR_WINDOW_NUM 2

static struct display_window cvr_window[CVR_WINDOW_NUM] = {
    { 0, 0, WIN_W, WIN_H },
    { WIN_W * 1 / 2, WIN_H * 1 / 2, WIN_W / 2, WIN_H / 2 },
};

static bool cvr_position[CVR_WINDOW_NUM];

void set_display_cvr_window(void)
{
    set_display_window(cvr_window, CVR_WINDOW_NUM, cvr_position);
}

#define MAIN_WINDOW_NUM 1

static struct display_window main_window[MAIN_WINDOW_NUM] = {
    { 0, 0, WIN_W, WIN_H },
};

static bool main_position[MAIN_WINDOW_NUM];

void set_display_main_window(void)
{
    set_display_window(main_window, MAIN_WINDOW_NUM, main_position);
}
