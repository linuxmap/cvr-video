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

#include "display.h"

#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/prctl.h>

#include "parameter.h"
#include "video_ion_alloc.h"
#include "rk_rga/rk_rga.h"
#include "ui_resolution.h"

struct display {
    struct video_ion ion;
    int pos;
    struct display_window map;
    struct display* next;
    struct display* pre;
};

static struct display_window* window = NULL;
static int window_num = -1;
static bool *window_pos = NULL;
static struct display* disp = NULL;
static pthread_mutex_t disp_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_t display_id = 0;
static bool display_run = false;
static int disp_rga_fd = -1;
static int disp_width, disp_height;
static int rotate_angle;

static struct display_window out_window[MAX_WIN_NUM];
static bool out_position[MAX_WIN_NUM];
static bool out_exist = false;
static pthread_mutex_t out_lock = PTHREAD_MUTEX_INITIALIZER;
static void (*disp_callback)(int fd, int width, int height);

void display_set_display_callback(void (*callback)(int fd, int width, int height))
{
    disp_callback = callback;
}

static void add_display_node(struct display* node)
{
    struct display* cur = NULL;

    if (!disp) {
        disp = node;
    } else {
        cur = disp;
        while (cur) {
            if (cur->next) {
                cur = cur->next;
            } else {
                cur->next = node;
                node->pre = cur;
                break;
            }
        }
    }
}

static void delete_display_node(struct display* node)
{
    struct display* cur = NULL;
    struct display* pre = NULL;
    struct display* next = NULL;

    cur = disp;
    while (cur) {
        if (cur == node) {
            if (cur->pre == NULL && cur->next == NULL) {
                disp = NULL;
            } else if (cur->next == NULL) {
                pre = cur->pre;
                pre->next = NULL;
            } else if (cur->pre == NULL) {
                disp = cur->next;
                disp->pre = NULL;
            } else {
                pre = cur->pre;
                next = cur->next;
                pre->next = next;
                next->pre = pre;
            }
            break;
        }
        cur = cur->next;
    }
}

static int map_the_window(int position, struct display_window* map)
{
    switch (rotate_angle) {
    case 0:
        map->w = window[position].w;
        map->h = window[position].h;
        map->x = window[position].x;
        map->y = window[position].y;
        break;
    case 90:
        map->w = window[position].h;
        map->h = window[position].w;
        map->x = disp_width - window[position].y - window[position].h;
        map->y = window[position].x;
        break;
    case 270:
        map->w = window[position].h;
        map->h = window[position].w;
        map->x = window[position].y;
        map->y = disp_height - window[position].x - window[position].w;
        break;
    default:
        return -1;
    }
    return 0;
}

static void display_compose_scale(struct display* cur, struct win* video_win)
{
    int src_w, src_h, src_fd, dst_w, dst_h, dst_fd;
    int src_fmt, dst_fmt;
    int x, y;

    src_w = cur->ion.width;
    src_h = cur->ion.height;
    src_fd = cur->ion.fd;
    src_fmt = RGA_FORMAT_YCBCR_420_SP;
    dst_w = disp_width;
    dst_h = disp_height;
    dst_fd = video_win->video_ion.fd;
    dst_fmt = RGA_FORMAT_YCBCR_420_SP;
    x = cur->map.x;
    y = cur->map.y;

    if (rk_rga_ionfd_to_ionfd_scal(disp_rga_fd,
                                   src_fd, src_w, src_h, src_fmt,
                                   dst_fd, dst_w, dst_h, dst_fmt,
                                   x, y, src_w, src_h, src_w, src_h))
        printf("%s: %d failed!\n", __func__, __LINE__);
}

static void display_compose()
{
    struct win* video_win;
    struct display* cur = NULL;

    video_win = rk_fb_getvideowin();
    pthread_mutex_lock(&disp_lock);

    /* display window 0 first, or other windows may be covered by window 0 */
    cur = disp;
    while (cur) {
        if (cur->pos == 0) {
            display_compose_scale(cur, video_win);
            break;
        }
        cur = cur->next;
    }

    /* display other windows */
    cur = disp;
    while (cur) {
        if (cur->pos != 0)
            display_compose_scale(cur, video_win);
        cur = cur->next;
    }

    pthread_mutex_unlock(&disp_lock);
    if (rk_fb_video_disp(video_win) == -1)
        printf("%s failed!\n", __func__);
    if (disp_callback)
        disp_callback(video_win->video_ion.fd, disp_width, disp_height);
}

static void display_check()
{
    int screen_w, screen_h;
    int out_device;
    static struct display_window* win = NULL;
    static int num = -1;
    static bool *pos = NULL;
    static int screen_w_pre = 0;
    static int screen_h_pre = 0;
    int i = 0;

    pthread_mutex_lock(&out_lock);
    out_device = rk_fb_get_out_device(&screen_w, &screen_h);

    if (out_device == OUT_DEVICE_HDMI
        || out_device == OUT_DEVICE_CVBS_PAL
        || out_device == OUT_DEVICE_CVBS_NTSC) {
        if ((screen_w_pre != screen_w) || (screen_h_pre != screen_h)) {
            screen_w_pre = screen_w;
            screen_h_pre = screen_h;
            out_exist = false;
            if (win && pos && num > 0) {
                printf("%s out device change again.\n", __func__);
                set_display_window(win, num, pos);
                win = NULL;
                num = -1;
                pos = NULL;
            }
        }

        if (!out_exist) {
            out_exist = true;
            if (!win) {
               win = window;
               num = window_num;
               pos = window_pos;
            }

            memset(out_window, 0, sizeof(out_window));
            memset(out_position, 0, sizeof(out_position));
            for (i = 0; i < window_num && i < MAX_WIN_NUM; i++) {
                if (i == 0) {
                    out_window[i].x = 0;
                    out_window[i].y = 0;
                    out_window[i].w = screen_w;
                    out_window[i].h = screen_h;
                } else {
                    out_window[i].x = screen_w * (window_num - i) / window_num;
                    out_window[i].y = screen_h * (window_num - 1) / window_num;
                    out_window[i].w = screen_w / window_num;
                    out_window[i].h = screen_h / window_num;
                }
            }
            set_display_window(out_window, window_num < MAX_WIN_NUM ? window_num : MAX_WIN_NUM,
                               out_position);
        }
    } else {
        if (out_exist) {
            out_exist = false;
            set_display_window(win, num, pos);
            win = NULL;
            num = -1;
            pos = NULL;
        }
    }

    pthread_mutex_unlock(&out_lock);
}

static void *display_thread(void *arg)
{
    struct timeval tv;

    prctl(PR_SET_NAME, "display_thread", 0, 0, 0);

    display_run = true;
    while (display_run) {
        tv.tv_sec = 0;
        tv.tv_usec = 30000;
        select(0, NULL, NULL, NULL, &tv); // 30ms timer
        if (!rk_fb_get_screen_state())
            continue;
        display_compose();
    }
    pthread_exit(NULL);
}

static int display_init()
{
    disp_rga_fd = rk_rga_open();
    if (disp_rga_fd < 0) {
        printf("%s rga fd open fail!\n", __func__);
        return -1;
    }
    if (pthread_create(&display_id, NULL, display_thread, NULL)) {
        printf("%s pthread create fail!\n", __func__);
        rk_rga_close(disp_rga_fd);
        disp_rga_fd = -1;
        return -1;
    }

    return 0;
}

static void display_exit()
{
    if (display_id) {
        pthread_join(display_id, NULL);
        display_id = 0;
    }
    rk_rga_close(disp_rga_fd);
    disp_rga_fd = -1;
}

static void rga_the_window(struct video_ion* ion, int src_fd, int src_w,
                           int src_h, int src_fmt, int vir_w, int vir_h)
{
    int dst_fd, dst_w, dst_h, dst_fmt;

    dst_fd = ion->fd;
    dst_w = ion->width;
    dst_h = ion->height;
    dst_fmt = RGA_FORMAT_YCBCR_420_SP;
    rk_rga_ionfd_to_ionfd_rotate(disp_rga_fd,
                                 src_fd, src_w, src_h, src_fmt, vir_w, vir_h,
                                 dst_fd, dst_w, dst_h, dst_fmt, rotate_angle);
}

static void display_black(void)
{
    struct win* video_win;
    int screen_w, screen_h;

    video_win = rk_fb_getvideowin();
    rk_fb_get_out_device(&screen_w, &screen_h);
    video_ion_buffer_black(&video_win->video_ion, screen_w, screen_h);
    if (rk_fb_video_disp(video_win) == -1)
        printf("%s failed!\n", __func__);
}

void set_display_window(struct display_window* win, int num, bool* pos)
{
    struct display* cur = NULL;
    int out_device;

    pthread_mutex_lock(&disp_lock);
    while (disp) {
        cur = disp;
        if (cur) {
            delete_display_node(cur);
            video_ion_free(&cur->ion);
            free(cur);
            display_black();
            display_black();
        }
    }
    if (!disp) {
        display_run = false;
        window = NULL;
        window_num = 0;
        window_pos = NULL;
    }
    pthread_mutex_unlock(&disp_lock);

    if (!display_run)
        display_exit();

    pthread_mutex_lock(&disp_lock);
    window = win;
    window_num = num;
    window_pos = pos;
    out_device = rk_fb_get_out_device(&disp_width, &disp_height);
    rotate_angle = ((out_device == OUT_DEVICE_HDMI ||
                     out_device == OUT_DEVICE_CVBS_PAL ||
                     out_device == OUT_DEVICE_CVBS_NTSC) ?
                    0 : VIDEO_DISPLAY_ROTATE_ANGLE);
    if (parameter_get_video_flip()) {
        rotate_angle += 180;
        rotate_angle %= 360;
    }
    pthread_mutex_unlock(&disp_lock);
}

void display_the_window(int position, int src_fd, int src_w, int src_h,
                        int src_fmt, int vir_w, int vir_h)
{
    struct display* cur = NULL;

    display_check();

    pthread_mutex_lock(&disp_lock);
    if (!window || position >= window_num || position < 0) {
        pthread_mutex_unlock(&disp_lock);
        return;
    }

    if (!disp) {
        if (display_init()) {
            pthread_mutex_unlock(&disp_lock);
            return;
        }
    }
    cur = disp;
    while (cur) {
        if (cur->pos == position)
            break;
        cur = cur->next;
    }
    if (!cur) {
        cur = (struct display*)calloc(1, sizeof(struct display));
        if (cur) {
            cur->pos = position;
            cur->next = NULL;
            cur->pre = NULL;
            if (!map_the_window(position, &cur->map)) {
                cur->ion.client = -1;
                cur->ion.fd = -1;
                if (!video_ion_alloc(&cur->ion, cur->map.w, cur->map.h)) {
                    add_display_node(cur);
                    rga_the_window(&cur->ion,
                                   src_fd, src_w, src_h, src_fmt, vir_w, vir_h);
                } else {
                    printf("%s: %d ion alloc failed!\n", __func__, __LINE__);
                    free(cur);
                }
            } else {
                printf("%s: %d map window failed!\n", __func__, __LINE__);
                free(cur);
            }
        } else {
            printf("%s: %d alloc failed!\n", __func__, __LINE__);
        }
    } else {
        rga_the_window(&cur->ion,
                       src_fd, src_w, src_h, src_fmt, vir_w, vir_h);
    }
    pthread_mutex_unlock(&disp_lock);
}

void shield_the_window(int position, bool black)
{
    struct display* cur = NULL;

    pthread_mutex_lock(&disp_lock);
    cur = disp;
    while (cur) {
        if (cur->pos == position) {
            delete_display_node(cur);
            video_ion_free(&cur->ion);
            free(cur);
            if (black) {
                display_black();
                display_black();
            }
            break;
        }
        cur = cur->next;
    }
    if (!disp)
        display_run = false;
    pthread_mutex_unlock(&disp_lock);
    if (!display_run)
        display_exit();
}

int display_set_position()
{
    int pos = -1;
    int i;

    pthread_mutex_lock(&disp_lock);
    for (i = 0; i < window_num; i++) {
        if (!window_pos[i]) {
            window_pos[i] = true;
            pos = i;
            break;
        }
    }
    pthread_mutex_unlock(&disp_lock);

    return pos;
}

void display_reset_position(int pos)
{
    int i;

    pthread_mutex_lock(&disp_lock);
    for (i = 0; i < window_num; i++) {
        if (pos == i) {
            window_pos[i] = false;
            break;
        }
    }
    pthread_mutex_unlock(&disp_lock);
}

void display_switch_pos(int *pos1, int *pos2)
{
    int pos;
    pthread_mutex_lock(&disp_lock);
    pos = *pos1;
    *pos1 = *pos2;
    *pos2 = pos;
    pthread_mutex_unlock(&disp_lock);
}
