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

#include "uvc_user.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/videodev2.h>

#include <list>

#include "uvc-gadget.h"
#include "../yuv.h"
#include <sys/prctl.h>

struct uvc_buffer {
    void* buffer;
    size_t size;
    size_t total_size;
    int width;
    int height;
};

struct uvc_buffer_list {
    std::list<struct uvc_buffer*> buffer_list;
    pthread_mutex_t mutex;
};

struct video_uvc {
    struct uvc_buffer_list write;
    struct uvc_buffer_list read;
};

static struct video_uvc* uvc = NULL;
static pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t user_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool uvc_process = false;
static pthread_t uvc_id = 0;
struct uvc_user uvc_user;
static int uvc_video_id = -1;

static struct uvc_buffer* uvc_buffer_create(int width, int height)
{
    struct uvc_buffer* buffer = NULL;

    buffer = (struct uvc_buffer*)calloc(1, sizeof(struct uvc_buffer));
    if (!buffer)
        return NULL;
    buffer->width = width;
    buffer->height = height;
    buffer->size = buffer->width * buffer->height * 2;
    buffer->buffer = calloc(1, buffer->size);
    if (!buffer->buffer) {
        free(buffer);
        return NULL;
    }
    buffer->total_size = buffer->size;
    return buffer;
}

static void uvc_buffer_push_back(struct uvc_buffer_list* uvc_buffer,
                                 struct uvc_buffer* buffer)
{
    pthread_mutex_lock(&uvc_buffer->mutex);
    uvc_buffer->buffer_list.push_back(buffer);
    pthread_mutex_unlock(&uvc_buffer->mutex);
}

static struct uvc_buffer* uvc_buffer_pop_front(
    struct uvc_buffer_list* uvc_buffer)
{
    struct uvc_buffer* buffer = NULL;

    pthread_mutex_lock(&uvc_buffer->mutex);
    if (!uvc_buffer->buffer_list.empty()) {
        buffer = uvc_buffer->buffer_list.front();
        uvc_buffer->buffer_list.pop_front();
    }
    pthread_mutex_unlock(&uvc_buffer->mutex);
    return buffer;
}

static struct uvc_buffer* uvc_buffer_front(struct uvc_buffer_list* uvc_buffer)
{
    struct uvc_buffer* buffer = NULL;

    pthread_mutex_lock(&uvc_buffer->mutex);
    if (!uvc_buffer->buffer_list.empty())
        buffer = uvc_buffer->buffer_list.front();
    else
        buffer = NULL;
    pthread_mutex_unlock(&uvc_buffer->mutex);
    return buffer;
}

static bool uvc_buffer_check(struct uvc_buffer* buffer)
{
    int width = 0, height = 0;

    uvc_get_user_resolution(&width, &height);
    if (buffer->width == width && buffer->height == height)
        return true;
    else
        return false;
}

static void uvc_buffer_destroy(struct uvc_buffer_list* uvc_buffer)
{
    struct uvc_buffer* buffer = NULL;

    pthread_mutex_lock(&uvc_buffer->mutex);
    while (!uvc_buffer->buffer_list.empty()) {
        buffer = uvc_buffer->buffer_list.front();
        free(buffer->buffer);
        free(buffer);
        uvc_buffer->buffer_list.pop_front();
    }
    pthread_mutex_unlock(&uvc_buffer->mutex);
    pthread_mutex_destroy(&uvc_buffer->mutex);
}

static void uvc_buffer_clear(struct uvc_buffer_list* uvc_buffer)
{
    pthread_mutex_lock(&uvc_buffer->mutex);
    uvc_buffer->buffer_list.clear();
    pthread_mutex_unlock(&uvc_buffer->mutex);
}

int uvc_buffer_init()
{
    int i = 0;
    int ret = 0;
    struct uvc_buffer* buffer = NULL;
    int width, height;

    uvc_get_user_resolution(&width, &height);

    pthread_mutex_lock(&buffer_mutex);

    uvc = new video_uvc();
    if (!uvc) {
        ret = -1;
        goto exit;
    }
    pthread_mutex_init(&uvc->write.mutex, NULL);
    pthread_mutex_init(&uvc->read.mutex, NULL);
    uvc_buffer_clear(&uvc->write);
    uvc_buffer_clear(&uvc->read);
    printf("UVC_BUFFER_NUM = %d\n", UVC_BUFFER_NUM);
    for (i = 0; i < UVC_BUFFER_NUM; i++) {
        buffer = uvc_buffer_create(width, height);
        if (!buffer) {
            ret = -1;
            goto exit;
        }
        uvc_buffer_push_back(&uvc->write, buffer);
    }
    uvc_process = true;

exit:
    pthread_mutex_unlock(&buffer_mutex);
    return ret;
}

void uvc_buffer_deinit()
{
    pthread_mutex_lock(&buffer_mutex);
    if (uvc) {
        uvc_process = false;
        uvc_buffer_destroy(&uvc->write);
        uvc_buffer_destroy(&uvc->read);
        delete uvc;
        uvc = NULL;
    }
    pthread_mutex_unlock(&buffer_mutex);
}

bool uvc_buffer_write_enable()
{
    bool ret = false;

    if (pthread_mutex_trylock(&buffer_mutex))
        return ret;
    if (uvc && uvc_buffer_front(&uvc->write))
        ret = true;
    pthread_mutex_unlock(&buffer_mutex);
    return ret;
}

void uvc_buffer_write(void* extra_data,
                      size_t extra_size,
                      void* data,
                      size_t size,
                      unsigned int fcc)
{
    pthread_mutex_lock(&buffer_mutex);
    if (uvc && data) {
        struct uvc_buffer* buffer = uvc_buffer_pop_front(&uvc->write);
        if (buffer && buffer->buffer) {
            if (buffer->total_size >= extra_size + size) {
                switch (fcc) {
                case V4L2_PIX_FMT_YUYV:
                    NV12_to_YUYV(buffer->width, buffer->height, data, buffer->buffer);
                    break;
                case V4L2_PIX_FMT_MJPEG:
                    memcpy(buffer->buffer, data, size);
                    break;
                case V4L2_PIX_FMT_H264:
                    if (extra_data && extra_size > 0)
                        memcpy(buffer->buffer, extra_data, extra_size);
                    if (extra_size >= 0)
                        memcpy((char*)buffer->buffer + extra_size, data, size);
                    break;
                }
                buffer->size = extra_size + size;
                uvc_buffer_push_back(&uvc->read, buffer);
            } else {
                uvc_buffer_push_back(&uvc->write, buffer);
            }
        }
    }
    pthread_mutex_unlock(&buffer_mutex);
}

bool uvc_buffer_process_state()
{
    return uvc_process;
}

static void* uvc_gadget_pthread(void* arg)
{
    prctl(PR_SET_NAME, "uvc_gadget_pthread", 0, 0, 0);

    uvc_gadget_main();
    uvc_set_user_run_state(true);
    pthread_exit(NULL);
}

int uvc_gadget_pthread_init(int id)
{
    uvc_set_user_video_id(id);
    if (pthread_create(&uvc_id, NULL, uvc_gadget_pthread, NULL)) {
        printf("create uvc_gadget_pthread fail!\n");
        return -1;
    }
    return 0;
}

void uvc_gadget_pthread_exit()
{
    if (uvc_get_user_video_id() < 0)
        return;
    while(!uvc_get_user_run_state())
        pthread_yield();
    uvc_set_user_run_state(false);
    if (uvc_id) {
        pthread_join(uvc_id, NULL);
        uvc_id = 0;
    }
    uvc_set_user_video_id(-1);
    memset(&uvc_user, 0, sizeof(struct uvc_user));
}

void uvc_set_user_resolution(int width, int height)
{
    pthread_mutex_lock(&user_mutex);
    uvc_user.width = width;
    uvc_user.height = height;
    printf("uvc_user.width = %u, uvc_user.height = %u\n", uvc_user.width,
           uvc_user.height);
    pthread_mutex_unlock(&user_mutex);
}

void uvc_get_user_resolution(int* width, int* height)
{
    pthread_mutex_lock(&user_mutex);
    *width = uvc_user.width;
    *height = uvc_user.height;
    pthread_mutex_unlock(&user_mutex);
}

bool uvc_check_user_resolution_limit(int width, int height)
{
    int w, h;

    uvc_get_user_resolution(&w, &h);
    if (width < w || height < h)
        return false;
    else
        return true;
}

bool uvc_get_user_run_state()
{
    int ret;

    pthread_mutex_lock(&user_mutex);
    ret = uvc_user.run;
    pthread_mutex_unlock(&user_mutex);
    return ret;
}

void uvc_set_user_run_state(bool state)
{
    pthread_mutex_lock(&user_mutex);
    uvc_user.run = state;
    pthread_mutex_unlock(&user_mutex);
}

void uvc_set_user_fcc(unsigned int fcc)
{
    uvc_user.fcc = fcc;
}

unsigned int uvc_get_user_fcc()
{
    return uvc_user.fcc;
}

int uvc_get_user_video_id()
{
    return uvc_video_id;
}

void uvc_set_user_video_id(int video_id)
{
    uvc_video_id = video_id;
}

void uvc_user_fill_buffer(struct uvc_device *dev, struct v4l2_buffer *buf)
{
    struct uvc_buffer* buffer = NULL;

    while (uvc_get_user_run_state() && !(buffer = uvc_buffer_front(&uvc->read)))
        usleep(1000);
    if (buffer) {
        if (!uvc_buffer_check(buffer))
            return;
        if (uvc_get_user_run_state() && uvc_buffer_process_state()) {
            if (buf->length >= buffer->size && buffer->buffer) {
                buf->bytesused = buffer->size;
                memcpy(dev->mem[buf->index].start, buffer->buffer, buffer->size);
            }
        } else {
            buf->bytesused = buf->length;
        }
        buffer = uvc_buffer_pop_front(&uvc->read);
        uvc_buffer_push_back(&uvc->write, buffer);
    } else {
        buf->bytesused = buf->length;
    }
}

