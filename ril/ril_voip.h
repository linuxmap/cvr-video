/**
 * Copyright (C) 2016 Fuzhou Rockchip Electronics Co., Ltd
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

#ifndef __RIL_VOIP_H_
#define __RIL_VOIP_H_

#include <tinyalsa/asoundlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <string.h>

struct ril_voip_data {
    int pthread_voip_state;
    int pthread_voip_exit;
    pthread_t voip_tid;

    unsigned int codec_in_size;
    unsigned int ril_in_size;
    char *codec_in_buffer;
    char *ril_in_buffer;

    struct pcm_config config;
    struct pcm *pcm_codec_in;
    struct pcm *pcm_ril_in;
    struct pcm_config out_config;
    struct pcm *pcm_codec_out;
    struct pcm *pcm_ril_out;
};

int start_voip();
int stop_voip();

#endif

