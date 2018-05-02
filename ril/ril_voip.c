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
#include "ril_voip.h"

#include <sys/prctl.h>

static struct ril_voip_data rilviopdata;
static int ril_voip_status = 0;

static int check_param(struct pcm_params *params, unsigned int param, unsigned int value,
                       char *param_name, char *param_unit);

static int sample_is_playable(unsigned int card, unsigned int device, unsigned int channels,
                              unsigned int rate, unsigned int bits, unsigned int period_size,
                              unsigned int period_count);

static int init_audio_hw()
{
    unsigned int card0 = 0;
    unsigned int card1 = 1;
    unsigned int device = 0;
    unsigned int channels = 2;
    unsigned int rate = 8000;
    unsigned int bits = 16;
    unsigned int period_size = 1024;
    unsigned int period_count = 4;
    enum pcm_format format = PCM_FORMAT_S16_LE;

    rilviopdata.config.channels = channels;
    rilviopdata.config.rate = rate;
    rilviopdata.config.period_size = period_size;
    rilviopdata.config.period_count = period_count;
    rilviopdata.config.format = format;
    rilviopdata.config.start_threshold = 0;
    rilviopdata.config.stop_threshold = 0;
    rilviopdata.config.silence_threshold = 0;


    rilviopdata.out_config.channels = channels;
    rilviopdata.out_config.rate = rate;
    rilviopdata.out_config.period_size = period_size;
    rilviopdata.out_config.period_count = period_count;
    rilviopdata.out_config.format = format;
    rilviopdata.out_config.start_threshold = 0;
    rilviopdata.out_config.stop_threshold = 0;
    rilviopdata.out_config.silence_threshold = 0;

    rilviopdata.pcm_codec_in = pcm_open(card0, device, PCM_IN, &rilviopdata.config);
    if (!rilviopdata.pcm_codec_in || !pcm_is_ready(rilviopdata.pcm_codec_in)) {
        fprintf(stderr, "Unable to open PCM device (%s)\n",
                pcm_get_error(rilviopdata.pcm_codec_in));
        return -1;
    }

    rilviopdata.codec_in_size = pcm_frames_to_bytes(rilviopdata.pcm_codec_in,
                                    pcm_get_buffer_size(rilviopdata.pcm_codec_in));
    rilviopdata.codec_in_buffer = malloc(rilviopdata.codec_in_size);
    if (!rilviopdata.codec_in_buffer) {
        fprintf(stderr, "Unable to allocate %d bytes\n", rilviopdata.codec_in_size);
        goto init_audio_hw_exit0;
    }
    printf("pcm_codec_in Capturing sample: %u ch, %u hz, %u bit\n", channels, rate,
           pcm_format_to_bits(format));

    rilviopdata.pcm_ril_in = pcm_open(card1, device, PCM_IN, &rilviopdata.config);
    if (!rilviopdata.pcm_ril_in || !pcm_is_ready(rilviopdata.pcm_ril_in)) {
        fprintf(stderr, "Unable to open PCM device (%s)\n",
                pcm_get_error(rilviopdata.pcm_ril_in));
        goto init_audio_hw_exit0;
    }

    rilviopdata.ril_in_size = pcm_frames_to_bytes(rilviopdata.pcm_ril_in, \
                                  pcm_get_buffer_size(rilviopdata.pcm_ril_in));
    rilviopdata.ril_in_buffer = malloc(rilviopdata.ril_in_size);
    if (!rilviopdata.ril_in_buffer) {
        fprintf(stderr, "Unable to allocate %d bytes\n", rilviopdata.ril_in_size);
        goto init_audio_hw_exit1;
    }
    printf("pcm_ril_in Capturing sample: %u ch, %u hz, %u bit\n", channels, rate,
           pcm_format_to_bits(format));


    if (!sample_is_playable(card0, device, channels, rate, bits, period_size, period_count)) {
        goto init_audio_hw_exit1;
    }
    rilviopdata.pcm_codec_out = pcm_open(card0, device, PCM_OUT, &rilviopdata.out_config);
    if (!rilviopdata.pcm_codec_out || !pcm_is_ready(rilviopdata.pcm_codec_out)) {
        fprintf(stderr, "Unable to open PCM device %u (%s)\n",
                device, pcm_get_error(rilviopdata.pcm_codec_out));
        goto init_audio_hw_exit2;
    }

    if (!sample_is_playable(card1, device, channels, rate, bits, period_size, period_count)) {
        goto init_audio_hw_exit2;
    }
    rilviopdata.pcm_ril_out = pcm_open(card1, device, PCM_OUT, &rilviopdata.out_config);
    if (!rilviopdata.pcm_ril_out || !pcm_is_ready(rilviopdata.pcm_ril_out)) {
        fprintf(stderr, "Unable to open PCM device %u (%s)\n",
                device, pcm_get_error(rilviopdata.pcm_ril_out));
        goto init_audio_hw_exit3;
    }

    return 0;

init_audio_hw_exit3:
    pcm_close(rilviopdata.pcm_ril_out);

init_audio_hw_exit2:
    pcm_close(rilviopdata.pcm_codec_out);

init_audio_hw_exit1:
    free(rilviopdata.ril_in_buffer);
    pcm_close(rilviopdata.pcm_ril_in);

init_audio_hw_exit0:
    free(rilviopdata.codec_in_buffer);
    pcm_close(rilviopdata.pcm_codec_in);

    return -1;
}

static int capture_process()
{
    if (!pcm_read(rilviopdata.pcm_ril_in, rilviopdata.ril_in_buffer, rilviopdata.ril_in_size)) {
        if (pcm_write(rilviopdata.pcm_codec_out, rilviopdata.ril_in_buffer, rilviopdata.ril_in_size)) {
            fprintf(stderr, "Error playing sample ril\n");
            return -1;
        }
    }
    if (!pcm_read(rilviopdata.pcm_codec_in, rilviopdata.codec_in_buffer, rilviopdata.codec_in_size)) {
        if (pcm_write(rilviopdata.pcm_ril_out, rilviopdata.codec_in_buffer, rilviopdata.codec_in_size)) {
            fprintf(stderr, "Error playing sample codec\n");
            return -1;
        }
    }

    return 0;
}

static int deinit_audio_hw()
{
    pcm_close(rilviopdata.pcm_ril_out);
    pcm_close(rilviopdata.pcm_codec_out);
    pcm_close(rilviopdata.pcm_ril_in);
    pcm_close(rilviopdata.pcm_codec_in);
    free(rilviopdata.ril_in_buffer);
    free(rilviopdata.codec_in_buffer);
}

static int ril_delete_voip_pthread(void)
{
    pthread_detach(pthread_self());
    pthread_exit(0);
}

static void *ril_voip_pthread(void *arg)
{
    int ret;
    rilviopdata.pthread_voip_exit = 0;
    prctl(PR_SET_NAME, __func__, 0, 0, 0);

    if (!init_audio_hw()) {

        while (!rilviopdata.pthread_voip_exit) {
            if (capture_process())
                break;
        }
        deinit_audio_hw();

    }

    rilviopdata.pthread_voip_state = 0;
    ril_delete_voip_pthread();
}

static int ril_create_voip_pthread(void)
{
    if (pthread_create(&rilviopdata.voip_tid, NULL, ril_voip_pthread, NULL)) {
        printf("create ril_voip_thread pthread failed\n");
        rilviopdata.pthread_voip_state = 0;
        return -1;
    }

    return 0;
}

int start_voip()
{
    if (ril_voip_status == 0) {
        ril_voip_status = 1;
        printf("start_voip!\n");
        memset(&rilviopdata, 0, sizeof(rilviopdata));
        rilviopdata.pthread_voip_state = 1;
        ril_create_voip_pthread();
    }
    return 0;
}

int stop_voip()
{
    if (ril_voip_status == 1) {
        ril_voip_status = 0;
        printf("stop_voip!\n");
        if (rilviopdata.pthread_voip_state == 1) {
            rilviopdata.pthread_voip_exit = 1;
            pthread_join(rilviopdata.voip_tid, NULL);
            rilviopdata.pthread_voip_state = 0;
        }

        memset(&rilviopdata, 0, sizeof(rilviopdata));
    }
    return 0;
}

static int check_param(struct pcm_params *params, unsigned int param, unsigned int value,
                       char *param_name, char *param_unit)
{
    unsigned int min;
    unsigned int max;
    int is_within_bounds = 1;

    min = pcm_params_get_min(params, param);
    if (value < min) {
        fprintf(stderr, "%s is %u%s, device only supports >= %u%s\n", param_name, value,
                param_unit, min, param_unit);
        is_within_bounds = 0;
    }

    max = pcm_params_get_max(params, param);
    if (value > max) {
        fprintf(stderr, "%s is %u%s, device only supports <= %u%s\n", param_name, value,
                param_unit, max, param_unit);
        is_within_bounds = 0;
    }

    return is_within_bounds;
}

static int sample_is_playable(unsigned int card, unsigned int device, unsigned int channels,
                              unsigned int rate, unsigned int bits, unsigned int period_size,
                              unsigned int period_count)
{
    struct pcm_params *params;
    int can_play;

    params = pcm_params_get(card, device, PCM_OUT);
    if (params == NULL) {
        fprintf(stderr, "Unable to open PCM device %u.\n", device);
        return 0;
    }

    can_play = check_param(params, PCM_PARAM_RATE, rate, "Sample rate", "Hz");
    can_play &= check_param(params, PCM_PARAM_CHANNELS, channels, "Sample", " channels");
    can_play &= check_param(params, PCM_PARAM_SAMPLE_BITS, bits, "Bitrate", " bits");
    can_play &= check_param(params, PCM_PARAM_PERIOD_SIZE, period_size, "Period size", "Hz");
    can_play &= check_param(params, PCM_PARAM_PERIODS, period_count, "Period count", "Hz");

    pcm_params_free(params);

    return can_play;
}
