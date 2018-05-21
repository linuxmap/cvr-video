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

#include "uvc_process.h"

#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/prctl.h>

#include "uvc_encode.h"

extern "C" {
#include "video_ion_alloc.h"
#include "rk_rga/rk_rga.h"
}
#include "video.h"

struct uvc_process {
    struct video_ion ion;
    int pos;
    struct uvc_window win;
    struct uvc_process* next;
    struct uvc_process* pre;
};

static struct uvc_window* uvc_window = NULL;
static int uvc_window_num = -1;
static bool *uvc_window_pos = NULL;
static struct uvc_process* uvc_process = NULL;
static pthread_mutex_t uvc_process_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_t uvc_process_id = 0;
static bool uvc_process_run = false;
static int uvc_rga_fd = -1;
struct uvc_encode* uvc_enc = NULL;
static struct uvc_window uvc_win_one[1] = {
    { 0, 0, UVC_WINDOW_WIDTH, UVC_WINDOW_HEIGHT }
};
static bool uvc_pos_one[1];
static struct uvc_window uvc_win_two[2] = {
    { 0, 0, UVC_WINDOW_WIDTH / 2, UVC_WINDOW_HEIGHT },
    { UVC_WINDOW_WIDTH / 2, 0, UVC_WINDOW_WIDTH / 2, UVC_WINDOW_HEIGHT }
};
static bool uvc_pos_two[2];

static void add_uvc_process_node(struct uvc_process* node)
{
    struct uvc_process* cur = NULL;

    if (!uvc_process) {
        uvc_process = node;
    } else {
        cur = uvc_process;
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

static void delete_uvc_process_node(struct uvc_process* node)
{
    struct uvc_process* cur = NULL;
    struct uvc_process* pre = NULL;
    struct uvc_process* next = NULL;

    cur = uvc_process;
    while (cur) {
        if (cur == node) {
            if (cur->pre == NULL && cur->next == NULL) {
                uvc_process = NULL;
            } else if (cur->next == NULL) {
                pre = cur->pre;
                pre->next = NULL;
            } else if (cur->pre == NULL) {
                uvc_process = cur->next;
                uvc_process->pre = NULL;
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

static void uvc_process_compose_scale(struct uvc_process* cur, struct uvc_encode* uvc_enc)
{
    int src_w, src_h, src_fd, dst_w, dst_h, dst_fd;
    int src_fmt, dst_fmt;
    int x, y;

    src_w = cur->ion.width;
    src_h = cur->ion.height;
    src_fd = cur->ion.fd;
    src_fmt = RGA_FORMAT_YCBCR_420_SP;
    dst_w = uvc_enc->uvc_in.width;
    dst_h = uvc_enc->uvc_in.height;
    dst_fd = uvc_enc->uvc_in.fd;
    dst_fmt = RGA_FORMAT_YCBCR_420_SP;
    x = cur->win.x;
    y = cur->win.y;

    if (rk_rga_ionfd_to_ionfd_scal(uvc_rga_fd,
                                   src_fd, src_w, src_h, src_fmt,
                                   dst_fd, dst_w, dst_h, dst_fmt,
                                   x, y, src_w, src_h, src_w, src_h))
        printf("%s: %d failed!\n", __func__, __LINE__);
}

static void uvc_process_compose(struct uvc_encode* uvc_enc)
{
    struct uvc_process* cur = NULL;

    pthread_mutex_lock(&uvc_process_lock);
    cur = uvc_process;
    while (cur) {
        uvc_process_compose_scale(cur, uvc_enc);
        cur = cur->next;
    }
    pthread_mutex_unlock(&uvc_process_lock);

    if (!uvc_rga_process(uvc_enc, uvc_enc->uvc_in.width, uvc_enc->uvc_in.height,
                         uvc_enc->uvc_in.buffer, uvc_enc->uvc_in.fd))
        uvc_encode_process(uvc_enc);
}

static void *uvc_process_thread(void *arg)
{
    struct timeval tv;

    prctl(PR_SET_NAME, "uvc_process_thread", 0, 0, 0);

    uvc_process_run = true;
    while (uvc_process_run) {
        tv.tv_sec = 0;
        tv.tv_usec = 30000;
        select(0, NULL, NULL, NULL, &tv); // 30ms timer
        uvc_process_compose(uvc_enc);
    }

    pthread_exit(NULL);
}

static int uvc_process_init()
{
    uvc_rga_fd = rk_rga_open();
    if (uvc_rga_fd < 0) {
        printf("%s rga fd open fail!\n", __func__);
        return -1;
    }
    if (pthread_create(&uvc_process_id, NULL, uvc_process_thread, NULL)) {
        printf("%s pthread create fail!\n", __func__);
        rk_rga_close(uvc_rga_fd);
        uvc_rga_fd = -1;
        return -1;
    }

    return 0;
}

static void uvc_process_exit()
{
    if (uvc_process_id) {
        pthread_join(uvc_process_id, NULL);
        uvc_process_id = 0;
    }
    rk_rga_close(uvc_rga_fd);
    uvc_rga_fd = -1;
}

static void rga_the_window(struct video_ion* ion, int src_fd, int src_w,
                           int src_h, int src_fmt, int vir_w, int vir_h)
{
    int dst_fd, dst_w, dst_h, dst_fmt;

    dst_fd = ion->fd;
    dst_w = ion->width;
    dst_h = ion->height;
    dst_fmt = RGA_FORMAT_YCBCR_420_SP;
    rk_rga_ionfd_to_ionfd_rotate(uvc_rga_fd,
                                 src_fd, src_w, src_h, src_fmt, vir_w, vir_h,
                                 dst_fd, dst_w, dst_h, dst_fmt, 0);
}

static void set_uvc_window(struct uvc_window* win, int num, bool* pos, int type)
{
    struct uvc_process* cur = NULL;

	printf("czy: set uvc window\n");
    video_record_reset_uvc_position();

    pthread_mutex_lock(&uvc_process_lock);
    while (uvc_process) {
	printf("czy: uvc_process not null\n");
        cur = uvc_process;
        if (cur) {
            delete_uvc_process_node(cur);
            video_ion_free(&cur->ion);
            free(cur);
        }
    }
    if (!uvc_process) {
	printf("czy: uvc process is null\n");
        uvc_process_run = false;
        uvc_window = NULL;
        uvc_window_num = 0;
        uvc_window_pos = NULL;
    }
    pthread_mutex_unlock(&uvc_process_lock);

    if (!uvc_process_run) {
	printf("czy: uvc_process not run\n");
        uvc_process_exit();

	}
    pthread_mutex_lock(&uvc_process_lock);
    uvc_window = win;
    uvc_window_num = num;
    uvc_window_pos = pos;
    pthread_mutex_unlock(&uvc_process_lock);

    video_record_set_uvc_position(type);
	printf("czy: set uvc window finish\n");
	printf("czy: **************************\n");
}

static int uvc_get_the_window(int position, struct uvc_window* win)
{
    win->w = uvc_window[position].w;
    win->h = uvc_window[position].h;
    win->x = uvc_window[position].x;
    win->y = uvc_window[position].y;
    return 0;
}

void uvc_process_the_window(int position, void *src_virt, int src_fd, int src_w, int src_h,
                            int src_fmt, int vir_w, int vir_h)
{
    struct uvc_process* cur = NULL;

    pthread_mutex_lock(&uvc_process_lock);
    if (!uvc_window || position >= uvc_window_num || position < 0) {
        pthread_mutex_unlock(&uvc_process_lock);
        return;
    }

    /* one camera direct process the buffer, MJPG camera need check src_fmt */
    if (src_fmt != RGA_FORMAT_YCBCR_422_SP || src_w % 16 || src_w % 16) {
        if (uvc_window_num == 1) {
            if (!uvc_rga_process(uvc_enc, src_w, src_h, src_virt, src_fd))
                uvc_encode_process(uvc_enc);
            pthread_mutex_unlock(&uvc_process_lock);
            return;
        }
    }

    /* two or more camera process the buffer with rga, and compose buffers in thread */
    if (!uvc_process) {
        if (uvc_process_init()) {
            pthread_mutex_unlock(&uvc_process_lock);
            return;
        }
    }
    cur = uvc_process;
    while (cur) {
        if (cur->pos == position)
            break;
        cur = cur->next;
    }
    if (!cur) {
        cur = (struct uvc_process*)calloc(1, sizeof(struct uvc_process));
        if (cur) {
            cur->pos = position;
            cur->next = NULL;
            cur->pre = NULL;
            if (!uvc_get_the_window(position, &cur->win)) {
                cur->ion.client = -1;
                cur->ion.fd = -1;
                if (!video_ion_alloc(&cur->ion, cur->win.w, cur->win.h)) {
                    add_uvc_process_node(cur);
                    rga_the_window(&cur->ion,
                                   src_fd, src_w, src_h, src_fmt, vir_w, vir_h);
                } else {
                    printf("%s: %d ion alloc failed!\n", __func__, __LINE__);
                    free(cur);
                }
            } else {
                printf("%s: %d uvc_window failed!\n", __func__, __LINE__);
                free(cur);
            }
        } else {
            printf("%s: %d alloc failed!\n", __func__, __LINE__);
        }
    } else {
        rga_the_window(&cur->ion,
                       src_fd, src_w, src_h, src_fmt, vir_w, vir_h);
    }
    pthread_mutex_unlock(&uvc_process_lock);
}

void uvc_process_shield_the_window(int position)
{
    struct uvc_process* cur = NULL;

    pthread_mutex_lock(&uvc_process_lock);
    cur = uvc_process;
    while (cur) {
        if (cur->pos == position) {
            delete_uvc_process_node(cur);
            video_ion_free(&cur->ion);
            free(cur);
            break;
        }
        cur = cur->next;
    }
    if (!uvc_process)
        uvc_process_run = false;
    pthread_mutex_unlock(&uvc_process_lock);
    if (!uvc_process_run)
        uvc_process_exit();
}

int uvc_process_set_position()
{
    int pos = -1;
    int i;

    pthread_mutex_lock(&uvc_process_lock);
    for (i = 0; i < uvc_window_num; i++) {
        if (!uvc_window_pos[i]) {
            uvc_window_pos[i] = true;
            pos = i;
            break;
        }
    }
    pthread_mutex_unlock(&uvc_process_lock);

    return pos;
}

void uvc_process_reset_position(int pos)
{
    int i;

    pthread_mutex_lock(&uvc_process_lock);
    for (i = 0; i < uvc_window_num && uvc_window_num >= 0; i++) {
        if (pos == i) {
            uvc_window_pos[i] = false;
            break;
        }
    }
    pthread_mutex_unlock(&uvc_process_lock);
}

void set_uvc_window_one(int type)
{
    set_uvc_window(uvc_win_one, 1, uvc_pos_one, type);
}

void set_uvc_window_two(int type)
{
    set_uvc_window(uvc_win_two, 2, uvc_pos_two, type);
}

int video_uvc_encode_init()
{
    uvc_enc = new uvc_encode();
    if (!uvc_enc) {
        printf("no memory!\n");
        return -1;
    }
    if (uvc_encode_init(uvc_enc))
        return -1;

    return 0;
}

void video_uvc_encode_exit()
{
    if (uvc_enc) {
        uvc_encode_exit(uvc_enc);
        delete uvc_enc;
        uvc_enc = NULL;
    }
}
