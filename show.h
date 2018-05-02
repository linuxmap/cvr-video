/**
 * Copyright (C) 2016 Fuzhou Rockchip Electronics Co., Ltd
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

#ifndef __SHOW_H__
#define __SHOW_H__

#include <time.h>
#include <stdio.h>

#ifdef USE_DISP_TS
typedef enum {
    COLOR_Y,
    COLOR_R,
    COLOR_G,
    COLOR_B,
    COLOR_W,
} COLOR_Type;

typedef struct {
    int x;
    int y;
    int width;
    int height;
} YUV_Rect;

typedef struct {
    int x;
    int y;
} YUV_Point;

typedef struct {
    int Y;
    int U;
    int V;
} YUV_Color;

YUV_Color set_yuv_color(COLOR_Type color_type);

void yuv420_draw_line(void* imgdata,
    int width,
    int height,
    YUV_Point startPoint,
    YUV_Point endPoint,
    YUV_Color color);

void yuv420_draw_rectangle(void* imgdata,
    int width,
    int height,
    YUV_Rect rect_rio,
    YUV_Color color);

#endif

void show_string(const char* str,
                 int size,
                 int x_pos,
                 int y_pos,
                 int width,
                 int height,
                 void* dstbuf);

#endif
