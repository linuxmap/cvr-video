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

#include "display_sample.h"
#include "display.h"
#include "ui_resolution.h"

#define SAMPLE_WINDOW_NUM 5

static struct display_window sample_window[SAMPLE_WINDOW_NUM] = {
    { 0, 0, WIN_W, WIN_H },
    { WIN_W * 4 / 5, WIN_H * 4 / 5, WIN_W / 5, WIN_H / 5 },
    { WIN_W * 3 / 5, WIN_H * 4 / 5, WIN_W / 5, WIN_H / 5 },
    { WIN_W * 2 / 5, WIN_H * 4 / 5, WIN_W / 5, WIN_H / 5 },
    { WIN_W * 1 / 5, WIN_H * 4 / 5, WIN_W / 5, WIN_H / 5 },
};

static bool sample_position[SAMPLE_WINDOW_NUM];

void set_display_sample_window(void)
{
    set_display_window(sample_window, SAMPLE_WINDOW_NUM, sample_position);
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
