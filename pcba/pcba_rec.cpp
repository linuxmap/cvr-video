/**
 * Copyright (C) 2016 Fuzhou Rockchip Electronics Co., Ltd
 * author: Benjo Lei <benjo.lei@rock-chips.com>
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

#ifdef _PCBA_SERVICE_

#include <dirent.h>
#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "pcba_rec.h"
#include "audio/pcm_receiver.h"

struct PcbarecData {
    int pthread_reco_state;
    int pthread_reco_exit;
    pthread_t reco_tid;
    pthread_t reco_tid_write;
};

#define BUFF_SIZE  16000

static struct PcbarecData pcba_data;
static PCMReceiver* receiver = NULL;

int DeInitPcbaRec()
{
    if (receiver != NULL) {
        receiver->DetachFromSender();
        delete receiver;
        receiver = NULL;
    }
    return 0;
}

static void *pcba_reco_pthread(void *arg)
{
    void* buffer = nullptr;
    int fd = (int)arg;
    size_t size = 0, offset = 0;
    char pcm_data[BUFF_SIZE];
    int b_on = 1;
    int len = 0;

    printf("pcba_reco_pthread = %d\n", fd);
    if (fd < 0)
        return NULL;

    while (!pcba_data.pthread_reco_exit) {
        receiver->ObtainBuffer(buffer, size, BUFF_SIZE);

        if ( offset + size >= BUFF_SIZE) {
            if ((len = write(fd, pcm_data, offset)) < 0) {
                printf("write fail!\r\n");
                break;
            }
            offset -= len;
        }

        memcpy(&pcm_data[offset], buffer, size);
        offset += size;
        receiver->ReleaseBuffer();
    }

    DeInitPcbaRec();
    close(fd);
    return NULL;
}

static int pcba_create_pthread(int nfd)
{
    receiver = new PCMReceiver();
    receiver->AttachToSender();

    if (pthread_create(&pcba_data.reco_tid, NULL, pcba_reco_pthread, (void *)nfd)) {
        printf("create pcba_reco_pthread failed\n");
        pcba_data.pthread_reco_state = 0;
        return -1;
    }
    return 0;
}

int pcba_register(int nfd)
{
    if(pcba_data.pthread_reco_state != 0)
        return -1;

    memset(&pcba_data, 0, sizeof(pcba_data));
    pcba_data.pthread_reco_state = 1;
    return pcba_create_pthread(nfd);
}

int pcba_unregister(void)
{
    if (pcba_data.pthread_reco_state == 1) {
        pcba_data.pthread_reco_exit = 1;
        pthread_join(pcba_data.reco_tid, NULL);
        pcba_data.pthread_reco_state = 0;
    }
    memset(&pcba_data, 0, sizeof(pcba_data));

    return 0;
}
#endif

