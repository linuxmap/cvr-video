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

#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#define MAX_WIN_NUM 6

struct display_window {
    unsigned int x;
    unsigned int y;
    unsigned int w;
    unsigned int h;
};

void set_display_window(struct display_window* win, int num, bool* pos);
void display_the_window(int position, int src_fd, int src_w, int src_h,
                        int src_fmt, int vir_w, int vir_h);
void shield_the_window(int position, bool black);
int display_set_position();
void display_reset_position(int pos);
void display_switch_pos(int *pos1, int *pos2);
void display_set_display_callback(void (*callback)(int fd, int width, int height));


#ifdef __cplusplus
}
#endif

#endif
