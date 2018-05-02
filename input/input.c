/**
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd
 * Author: hongjinkun <jinkun.hong@rock-chips.com>
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

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/prctl.h>

#include <linux/netlink.h>
#include <linux/kd.h>
#include <linux/input.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "input.h"

#define MAX_INPUTNUM    5
#define LONGPRESSCOUNT  3
#define PATHNUM         64

struct INPUTINFO {
    char events[MAX_INPUTNUM];
    int fds[MAX_INPUTNUM];
};

typedef void (*PTRCALLBACK)(int cmd, void *msg0, void *msg1);

static int gRunflag = 0;
static pthread_t gTid;
static PTRCALLBACK key_event_call;

static int check_keyboard(int num)
{
    FILE *fp;
    char tmp[PATHNUM];
    snprintf(tmp, PATHNUM, "/sys/class/input/input%d/uevent", num);

    fp = fopen(tmp, "r");

    if (fp > 0) {
        while (fgets(tmp, sizeof(tmp), fp)) {
            const char *p;

            if (! (p = strstr(tmp, "KEY")))
                continue;

            fclose(fp);

            return 0;
        }
        fclose(fp);
    }

    return -1;
}

static void input_open(struct INPUTINFO *info)
{
    int i = 0;
    char device[PATHNUM];

    for (i = 0; i < MAX_INPUTNUM; i++) {
        info->fds[i] = -1;
        if (check_keyboard(i) == 0) {
            snprintf(device, PATHNUM, "/dev/input/event%d", i);
            info->fds[i] = open(device, O_RDONLY | O_NOCTTY);
            printf("%s, fd = %d\n", device, info->fds[i]);
        }
    }
}

static void input_close(struct INPUTINFO *info)
{
    int i = 0;

    for (i = 0; i < MAX_INPUTNUM; i++) {
        if (info->fds[i] >= 0)
            close(info->fds[i]);
    }
}

static void send_event(int flag, int keycode)
{
    printf("%s %d, %d\n", __func__, flag, keycode);
    if (key_event_call)
        key_event_call(flag, (void *)keycode, (void *)NULL);
}

static int keyboard_update(unsigned int *key_status, struct INPUTINFO *info)
{
    int i;

    for (i = 0; i < MAX_INPUTNUM; i++) {
        if (info->events[i] == 1) {
            int    cc;
            struct input_event buftmp;
            unsigned int buf = 0;
            int ch = 0;
            int is_pressed = 0;

            info->events[i] = 0;
            cc = read(info->fds[i], &buftmp, sizeof(buftmp));
            if (cc >= 0) {
                if (buftmp.type) {
                    if (buftmp.value == 0) {
                        buf = 0x8000;
                    }
                    buf |= (unsigned char)(buftmp.code & 0x00ff);

                    is_pressed = !(buf & 0x8000);
                    ch         = buf & 0xff;

                    if (is_pressed) {
                        key_status[ch] = 1;
                        send_event(MSG_KEY_DOWN, ch);
                    } else {
                        if ((key_status[ch] >> 4) >= LONGPRESSCOUNT)
                            send_event(MSG_KEY_LONGUP, ch);
                        else
                            send_event(MSG_KEY_UP, ch);

                        key_status[ch] = 0;
                    }
                }
            }
        }
    }

    for (i = 0; i < NR_KEYS; i++) {
        if (key_status[i] & 1) {
            if ((key_status[i] >> 4) >= LONGPRESSCOUNT)
                send_event(MSG_KEY_LONGPRESS, i);
            else
                key_status[i] += 0x10;
        }
    }

    return 0;
}

static int wait_event(struct INPUTINFO *info)
{
    int i;
    int retvalue = 0;
    fd_set rfds;
    int    fd, e;
    int maxfd = 0;
    fd_set *in = NULL;
    fd_set *out = NULL;
    fd_set *except = NULL;
    struct timeval timeout;

    timeout.tv_sec = 0;
    timeout.tv_usec = 200000;

    if (!in) {
        in = &rfds;
        FD_ZERO(in);
    }

    for (i = 0; i < MAX_INPUTNUM; i++) {
        if (info->fds[i] >= 0) {
            fd = info->fds[i];
            FD_SET(fd, in);
            if (fd > maxfd)
                maxfd = fd;
        }
    }

    e = select (maxfd + 1, in, out, except, &timeout);
    if (e > 0) {
        for (i = 0; i < MAX_INPUTNUM; i++) {
            info->events[i] = 0;
            fd = info->fds[i];

            if (fd >= 0 && FD_ISSET (fd, in)) {
                FD_CLR (fd, in);
                info->events[i] = 1;
                retvalue = 1;
            }
        }
    } else if (e < 0) {
        return -1;
    }

    return retvalue;
}

static void *input_thread(void *arg)
{
    unsigned int key_status[NR_KEYS];
    struct INPUTINFO input_info;

    prctl(PR_SET_NAME, "input_thread", 0, 0, 0);
    memset(key_status, 0, sizeof(key_status));
    input_open(&input_info);
    while (gRunflag) {
        wait_event(&input_info);
        keyboard_update(key_status, &input_info);
    }
    input_close(&input_info);
    pthread_detach(pthread_self());
    pthread_exit(NULL);
}

void input_deint(void)
{
    gRunflag = 0;
    pthread_join(gTid, NULL);
}

int input_init(PTRCALLBACK call)
{
    gRunflag = 1;
    key_event_call = call;

    return pthread_create(&gTid, NULL, input_thread, NULL);
}
