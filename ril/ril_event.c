/**
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd
 * Author: Vicent Chi <vicent.chi@rock-chips.com>
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

#ifdef USE_RIL_MOUDLE

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ril_lib.h"

struct ril_data {
    int pthread_listen_state;
    int pthread_listen_exit;
    pthread_t listen_tid;
};

static struct ril_data rildata;
static int listen_fd = -1;

int (*ril_read_event_call)(int cmd, void *msg0, void *msg1);

int send_mess_to_ril(int cmd_type, char* amendata)
{
    if (listen_fd > 0)
        send_listen_riL_cmd(listen_fd, cmd_type, amendata);

    return 0;
}

int handup_ril_voip()
{
    stop_voip();
    send_mess_to_ril(RIL_HAND_UP, NULL);

    return 0;
}

void ril_open_hot_share()
{
    open_hot_share();
}

void ril_close_hot_share()
{
    close_hot_share();
}

static int ril_read_process(listen_event_data* event_data)
{

    if (event_data->event_type == EVENT_READ_RIL_RING || \
        event_data->event_type == EVENT_READ_RIL_CALL) {
        start_voip();
    } else if (event_data->event_type == EVENT_READ_RIL_NO_CARRIER) {
        stop_voip();
        send_mess_to_ril(RIL_HAND_UP, NULL);
    }

    if (ril_read_event_call)
        return (*ril_read_event_call)(event_data->event_type, \
                (void *)event_data->event_msg0, (void *)event_data->event_msg1);

    return 0;
}

int ril_reg_readevent_callback(int (*call)(int cmd, void *msg0, void *msg1))
{
    ril_read_event_call = call;
    return 0;
}


static int ril_delete_pthread(void)
{
    pthread_detach(pthread_self());
    pthread_exit(0);
    return 0;
}

static void *ril_listen_pthread(void *arg)
{
    int readType = 0;
    listen_event_data event_data;

    rildata.pthread_listen_exit = 0;

    while (!rildata.pthread_listen_exit) {
        if (!port_exist()) {
            if (listen_fd > 0) {
                close(listen_fd);
                listen_fd = -1;
            }
            sleep(3);
            printf("ACMexists is no, Just wait for 4G init!\n");
            continue;
        }

        if (listen_fd <= 0) {
            listen_fd = init_listen_ril();
        } else {
            listen_ril_event_process(listen_fd, &event_data);
            ril_read_process(&event_data);
        }
    }

    deinit_listen_ril(listen_fd);

    rildata.pthread_listen_state = 0;
    ril_delete_pthread();

    return NULL;
}

static int ril_create_pthread(void)
{
    if (pthread_create(&rildata.listen_tid, NULL, ril_listen_pthread, NULL)) {
        printf("create ril_thread pthread failed\n");
        rildata.pthread_listen_state = 0;
        return -1;
    }

    return 0;
}

int ril_register(void)
{
    memset(&rildata, 0, sizeof(rildata));
    rildata.pthread_listen_state = 1;
    ril_create_pthread();
    return 0;
}

int ril_unregister(void)
{
    if (rildata.pthread_listen_state == 1) {
        rildata.pthread_listen_exit = 1;
        pthread_join(rildata.listen_tid, NULL);
        rildata.pthread_listen_state = 0;
    }
    memset(&rildata, 0, sizeof(rildata));
    return 0;
}

#endif
