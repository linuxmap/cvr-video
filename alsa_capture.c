#include <alsa/asoundlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "alsa_capture.h"

#ifdef AEC_AGC_ANR_ALGORITHM
#include "rk_voice_prointerface.h"

#define BUFFER_MAX_SIZE 12288

void rk_voice_setpara(short int *pshw_params, short int  shw_len)
{
    short int shwcnt;

    for (shwcnt = 0; shwcnt < shw_len; shwcnt++)
        pshw_params[shwcnt] = 0;

    /* ----- 0.total ------ */
    pshw_params[0] = 16000;
    pshw_params[1] = 320;
    /* ------ 1.AEC ------ */
    pshw_params[10] = 1;
    pshw_params[11] = 320;
    pshw_params[12] = 1;
    pshw_params[13] = 0;
    pshw_params[14] = 160;
    pshw_params[15] = 16;
    pshw_params[16] = 0;
    pshw_params[17] = 320;
    pshw_params[18] = 640;
    pshw_params[19] = 16000;
    pshw_params[20] = 320;
    pshw_params[21] = 32;
    pshw_params[22] = 1;
    pshw_params[23] = 1;
    pshw_params[24] = 0;
    pshw_params[25] = 1;
    pshw_params[26] = 320;
    pshw_params[27] = 32;
    pshw_params[28] = 200;
    pshw_params[29] = 5;
    pshw_params[30] = 10;
    pshw_params[31] = 1;
    pshw_params[32] = 10;
    pshw_params[33] = 20;
    pshw_params[34] = (short int)(0.3f * (1 << 15));
    pshw_params[35] = 1;
    pshw_params[36] = 32;
    pshw_params[37] = 5;
    pshw_params[38] = 1;
    pshw_params[39] = 10;
    pshw_params[40] = 3;
    pshw_params[41] = 3;
    pshw_params[42] = (short int)(0.30f * (1 << 15));
    pshw_params[43] = (short int)(0.04f * (1 << 15));
    pshw_params[44] = (short int)(0.40f * (1 << 15));
    pshw_params[45] = (short int)(0.25f * (1 << 15));
    pshw_params[46] = (short int)(0.0313f * (1 << 15));
    pshw_params[47] = (short int)(0.0625 * (1 << 15));
    pshw_params[48] = (short int)(0.0938 * (1 << 15));
    pshw_params[49] = (short int)(0.1250 * (1 << 15));
    pshw_params[50] = (short int)(0.1563 * (1 << 15));
    pshw_params[51] = (short int)(0.1875 * (1 << 15));
    pshw_params[52] = (short int)(0.2188 * (1 << 15));
    pshw_params[53] = (short int)(0.2500 * (1 << 15));
    pshw_params[54] = (short int)(0.2813 * (1 << 15));
    pshw_params[55] = (short int)(0.3125 * (1 << 15));
    pshw_params[56] = (short int)(0.3438 * (1 << 15));
    pshw_params[57] = (short int)(0.3750 * (1 << 15));
    pshw_params[58] = (short int)(0.4063 * (1 << 15));
    pshw_params[59] = (short int)(0.4375 * (1 << 15));
    pshw_params[60] = (short int)(0.4688 * (1 << 15));
    pshw_params[61] = (short int)(0.5000 * (1 << 15));
    pshw_params[62] = (short int)(0.5313 * (1 << 15));
    pshw_params[63] = (short int)(0.5625 * (1 << 15));
    pshw_params[64] = (short int)(0.5938 * (1 << 15));
    pshw_params[65] = (short int)(0.6250 * (1 << 15));
    pshw_params[66] = (short int)(0.6563 * (1 << 15));
    pshw_params[67] = (short int)(0.6875 * (1 << 15));
    pshw_params[68] = (short int)(0.7188 * (1 << 15));
    pshw_params[69] = (short int)(0.7500 * (1 << 15));
    pshw_params[70] = (short int)(0.7813 * (1 << 15));
    pshw_params[71] = (short int)(0.8125 * (1 << 15));
    pshw_params[72] = (short int)(0.8438 * (1 << 15));
    pshw_params[73] = (short int)(0.8750 * (1 << 15));
    pshw_params[74] = (short int)(0.9063 * (1 << 15));
    pshw_params[75] = (short int)(0.9375 * (1 << 15));
    pshw_params[76] = (short int)(0.9688 * (1 << 15));
    pshw_params[77] = (short int)(1.0000 * (1 << 15));

    /* ------ 2.ANR ------ */
    pshw_params[90] = 1;
    pshw_params[91] = 16000;
    pshw_params[92] = 320;
    pshw_params[93] = 32;
    pshw_params[94] = 2;

    pshw_params[110] = 1;
    pshw_params[111] = 16000;
    pshw_params[112] = 320;
    pshw_params[113] = 32;
    pshw_params[114] = 2;

    /* ------ 3.AGC ------ */
    pshw_params[130] = 1;
    pshw_params[131] = 16000;
    pshw_params[132] = 320;
    pshw_params[133] = (short int)(6.0f * (1 << 5));
    pshw_params[134] = (short int)(-55.0f * (1 << 5));
    pshw_params[135] = (short int)(-46.0f * (1 << 5));
    pshw_params[136] = (short int)(-24.0f * (1 << 5));
    pshw_params[137] = (short int)(1.2f * (1 << 12));
    pshw_params[138] = (short int)(0.8f * (1 << 12));
    pshw_params[139] = (short int)(0.4f * (1 << 12));
    pshw_params[140] = 40;
    pshw_params[141] = 80;
    pshw_params[142] = 80;

    pshw_params[150] = 0;
    pshw_params[151] = 16000;
    pshw_params[152] = 320;
    pshw_params[153] = (short int)(6.0f * (1 << 5));
    pshw_params[154] = (short int)(-55.0f * (1 << 5));
    pshw_params[155] = (short int)(-46.0f * (1 << 5));
    pshw_params[156] = (short int)(-24.0f * (1 << 5));
    pshw_params[157] = (short int)(1.2f * (1 << 12));
    pshw_params[158] = (short int)(0.8f * (1 << 12));
    pshw_params[159] = (short int)(0.4f * (1 << 12));
    pshw_params[160] = 40;
    pshw_params[161] = 80;
    pshw_params[162] = 80;

    /* ------ 4.EQ ------ */
    pshw_params[170] = 0;
    pshw_params[171] = 320;
    pshw_params[172] = 1;
    pshw_params[173] = (short int)(1.0f * (1 << 15));

    pshw_params[330] = 0;
    pshw_params[331] = 320;
    pshw_params[332] = 1;
    pshw_params[333] = (short int)(1.0f * (1 << 15));

    /* ------ 5.CNG ------ */
    pshw_params[490] = 1;
    pshw_params[491] = 16000;
    pshw_params[492] = 320;
    pshw_params[493] = 2;
    pshw_params[494] = 10;
    pshw_params[495] = (short int)(0.92f * (1 << 15));
    pshw_params[496] = (short int)(0.3f * (1 << 15));
}

/* for before process */
static int g_capture_buffer_size = 0;
static char*  g_capture_buffer = NULL;
/* for after process */
static int g_captureout_buffer_size = 0;
static char*  g_captureout_buffer = NULL;

uint8_t trans_mark = 0;

int queue_capture_buffer(void *buffer, int bytes)
{
    if ((buffer == NULL) || (bytes <= 0)) {
        printf("queue_capture_buffer buffer error!");
        return -1;
    }
    if (g_capture_buffer_size + bytes > BUFFER_MAX_SIZE) {
        printf("unexpected cap buffer size too big!! return!");
        return -1;
    }

    memcpy((char *)g_capture_buffer + g_capture_buffer_size, (char *)buffer, bytes);
    g_capture_buffer_size += bytes;
    return 0;
}

int queue_captureout_buffer(void *buffer, int bytes)
{
    if ((buffer == NULL) || (bytes <= 0)) {
        printf("queue_captureout_buffer buffer error!");
        return -1;
    }
    if (g_captureout_buffer_size + bytes > BUFFER_MAX_SIZE) {
        printf("unexpected cap out buffer size too big!! return!");
        return -1;
    }

    memcpy((char *)g_captureout_buffer + g_captureout_buffer_size, (char *)buffer, bytes);
    g_captureout_buffer_size += bytes;
    return 0;
}
#endif

int alsa_capture_open(struct alsa_capture *capture, int devid)
{
    int err;
    char device_name[10];
#ifdef AEC_AGC_ANR_ALGORITHM
    short int  ashw_para[500] = {0};

    rk_voice_setpara(ashw_para, 500);
    trans_mark = 0;

    if (g_capture_buffer == NULL) {
        g_capture_buffer = (char *)malloc(BUFFER_MAX_SIZE);
        if (g_capture_buffer == NULL) {
            printf("malloc g_capture_buffer error");
            return -1;
        }
    }

    if (g_captureout_buffer == NULL) {
        g_captureout_buffer = (char *)malloc(BUFFER_MAX_SIZE);
        if (g_captureout_buffer == NULL) {
            printf("malloc g_capture_buffer error");
            return -1;
        }
    }
#endif
    snprintf(device_name, sizeof(device_name), "hw:%d,0", devid);

    if ((err = snd_pcm_open(&capture->handle, device_name, SND_PCM_STREAM_CAPTURE,
                            0)) < 0) {
        fprintf(stderr, "cannot open audio device %s (%s)\n", device_name,
                snd_strerror(err));
        return -1;
    }

    if ((err = snd_pcm_hw_params_malloc(&capture->hw_params)) < 0) {
        fprintf(stderr, "cannot allocate hardware parameter structure (%s)\n",
                snd_strerror(err));
        return -1;
    }

    memset(capture->hw_params, 0, snd_pcm_hw_params_sizeof());

    if ((err = snd_pcm_hw_params_any(capture->handle, capture->hw_params)) < 0) {
        fprintf(stderr, "Cannot initialize hardware parameter structure (%s)\n",
                snd_strerror(err));
        return -1;
    }
    snd_pcm_hw_params_get_channels_min(capture->hw_params, &capture->channel);

    if ((err = snd_pcm_hw_params_set_access(capture->handle, capture->hw_params,
                                            SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        fprintf(stderr, "cannot set access type (%s)\n", snd_strerror(err));
        return -1;
    }

    if ((err = snd_pcm_hw_params_set_format(capture->handle, capture->hw_params,
                                            capture->format)) < 0) {
        fprintf(stderr, "cannot set sample format (%s)\n", snd_strerror(err));
        return -1;
    }

    if ((err = snd_pcm_hw_params_set_rate_near(
                   capture->handle, capture->hw_params, &capture->sample_rate, 0)) <
        0) {
        fprintf(stderr, "cannot set sample rate (%s)\n", snd_strerror(err));
        return -1;
    }

    if ((err = snd_pcm_hw_params_set_channels(capture->handle, capture->hw_params,
                                              capture->channel)) < 0) {
        fprintf(stderr, "cannot set channel count (%s)\n", snd_strerror(err));
        return -1;
    }

    if ((err = snd_pcm_hw_params(capture->handle, capture->hw_params)) < 0) {
        fprintf(stderr, "cannot set parameters (%s)\n", snd_strerror(err));
        return -1;
    }

    snd_pcm_hw_params_get_period_size(capture->hw_params, &capture->period_size,
                                      0);
    snd_pcm_hw_params_get_buffer_size(capture->hw_params, &capture->buffer_size);
    if (capture->period_size == capture->buffer_size) {
        fprintf(stderr, "Can't use period equal to buffer size(%lu == %lu)\n",
                capture->period_size, capture->buffer_size);
        return -1;
    }
    printf("capture period_size: %lu, buffer_size: %lu\n", capture->period_size,
           capture->buffer_size);

    snd_pcm_hw_params_free(capture->hw_params);

    if ((err = snd_pcm_prepare(capture->handle)) < 0) {
        fprintf(stderr, "cannot prepare audio interface for use (%s)\n",
                snd_strerror(err));
        return -1;
    }
    capture->dev_id = devid;

#ifdef AEC_AGC_ANR_ALGORITHM
    VOICE_Init(ashw_para);
#endif

    printf("alsa capture open.\n");
    return 0;
}

int alsa_capture_doing(struct alsa_capture *capture, void *buff)
{
    int err = 0;
    int remain_len = capture->buffer_frames;
#ifdef AEC_AGC_ANR_ALGORITHM
    static int i = 0, j;
    int16_t *tmp1 = NULL;
    int16_t *tmp2 = NULL;
    int16_t *left = NULL;
    int16_t *right = NULL;
#endif

    do {
        err = snd_pcm_readi(capture->handle, buff, remain_len);
        if (err != remain_len) {
            if (err < 0) {
                if (err == -EAGAIN) {
                    /* Apparently snd_pcm_recover() doesn't handle this case - does it
                     * assume snd_pcm_wait() above? */
                    return 0;
                }
                err = snd_pcm_recover(capture->handle, err, 0);
                if (err < 0) {
                    /* Hmm, not much we can do - abort */
                    fprintf(stderr, "ALSA read failed (unrecoverable): %s\n",
                            snd_strerror(err));
                    return err;
                }
                err = snd_pcm_readi(capture->handle, buff, remain_len);
                if (err < 0) {
                    fprintf(stderr, "read from audio interface failed(%s)\n",
                            snd_strerror(err));
                    return err;
                }
            }
        }

#ifdef AEC_AGC_ANR_ALGORITHM
        /* hard code 1280bytes/320frames/16K stereo/20ms processed one at a time */
        queue_capture_buffer(buff, snd_pcm_frames_to_bytes(capture->handle,
                             remain_len));

        for (i = 0; i < g_capture_buffer_size / 1280; i++) {
            int16_t tmp_short = 320;
            short tmp_buffer[tmp_short * 2];
            memset(tmp_buffer, 0x00, tmp_short * 2);

            short in[tmp_short];
            memset(in, 0x00, tmp_short);

            short ref[tmp_short];
            memset(ref, 0x00, tmp_short);

            short out[tmp_short];
            memset(out, 0x00, tmp_short);

            tmp1 = (int16_t *)g_capture_buffer + i * 320 * 2;
            left = (int16_t *)ref;
            right = (int16_t *)in;
            for (j = 0; j < tmp_short; j++) {
                *left++ = *tmp1++;
                *right++ = *tmp1++;
            }

            if (trans_mark) {
                VOICE_ProcessTx((int16_t *)in, (int16_t *)ref, (int16_t *)out, 320);
            } else {
                /* Use ProcessRx to do ANR */
                VOICE_ProcessRx((int16_t *)in, (int16_t *)out, 320);
            }

            memset(tmp_buffer, 0x00, tmp_short * 2);

            tmp2 = (int16_t *) out;
            tmp1 = (int16_t *) tmp_buffer;
            for (j = 0; j < tmp_short; j++) {
                *tmp1++ = *tmp2;
                *tmp1++ = *tmp2++;
            }

            queue_captureout_buffer(tmp_buffer, 320 * 4);
        }
        /* Samples still keep in queuePlaybackBuffer */
        g_capture_buffer_size = g_capture_buffer_size - 1280 * i;
        /* Copy the rest of the sample to the beginning of the Buffer */
        memcpy((char *)g_capture_buffer, (char *)(g_capture_buffer + i * 1280),
               g_capture_buffer_size);

        if (g_captureout_buffer_size >= snd_pcm_frames_to_bytes(capture->handle,
            remain_len)) {
            memcpy((char *)buff, (char *)g_captureout_buffer,
                   snd_pcm_frames_to_bytes(capture->handle, remain_len));
            g_captureout_buffer_size = g_captureout_buffer_size - snd_pcm_frames_to_bytes(capture->handle,
                                       remain_len);
            memcpy((char *)g_captureout_buffer, (char *)(g_captureout_buffer +
                   snd_pcm_frames_to_bytes(capture->handle, remain_len)), g_captureout_buffer_size);
        } else {
            printf(">>>>>g_captureout_buffer_siz:e less than 1024 frames\n");
        }
#endif
        remain_len -= err;
    } while (remain_len > 0);

    return capture->buffer_frames - remain_len;
}

void alsa_capture_done(struct alsa_capture *capture)
{
    if (capture->handle) {
        snd_pcm_drain(capture->handle);
        snd_pcm_close(capture->handle);
        capture->handle = NULL;
    }

#ifdef AEC_AGC_ANR_ALGORITHM
    free(g_capture_buffer);
    g_capture_buffer = NULL;
    free(g_captureout_buffer);
    g_captureout_buffer = NULL;
    VOICE_Destory();
#endif
    printf("alsa capture close.\n");
}
