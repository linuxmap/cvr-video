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

#ifdef USE_SPEECHREC
#include <stdio.h>

#include "speechrec.h"
#include "audio/pcm_receiver.h"

#define SAVE_PCM_RAW 0

#define CAPRAW_CNT 40
#define DELAY_CAPRAW_CNT -(CAPRAW_CNT>>1)
#define IGNORE_CAPRAW_CNT 40

struct SpeechrecData {
    int pthread_reco_state;
    int pthread_reco_exit;
    pthread_t reco_tid;
};

struct SpeechrecData speechrec_data;
PCMReceiver* receiver = NULL;
static ps_decoder_t *ps;
static ps_decoder_t *ps2;
bool speechrec_wakeup = false;

int (*speechrec_read_event_call)(int cmd, void *msg0, void *msg1);
static int dealwith(char const * hyp);

int speechrec_reg_readevent_callback(int (*call)(int cmd, void *msg0, void *msg1))
{
    speechrec_read_event_call = call;
    return 0;
}

int InitSpeechRec()
{
    int ret = 0;
    cmd_ln_t *config;
    char etc_path[3][40];

    sprintf(etc_path[0],"%s/%s", SPEECHREC_ETC_FILE_PATH, "others");
    sprintf(etc_path[1],"%s/%s", SPEECHREC_ETC_FILE_PATH, "others/eyesee.lm");
    sprintf(etc_path[2],"%s/%s", SPEECHREC_ETC_FILE_PATH, "others/eyesee.dic");

    config = cmd_ln_init(NULL, ps_args(), TRUE,
                         "-hmm", etc_path[0],
                         "-lm",  etc_path[1],
                         "-dict", etc_path[2],
                         "-debug", "0",
                         NULL);
    if (config == NULL) {
        printf("cmd_ln_init is false!\n");
        goto init_reco_out1;
    }
    ps = ps_init(config);
    if (ps == NULL) {
        printf("ps_init is false!\n");
        goto init_reco_out1;
    }
    ps2 = ps_init(config);
    if (ps2 == NULL) {
        printf("ps_init 2 is false!\n");
        goto init_reco_out1;
    }

    ret = ps_start_utt(ps);
    if (ret < 0) {
        printf("ps_start_utt is false!\n");
        goto init_reco_out2;
    }
    ret = ps_start_utt(ps2);
    if (ret < 0) {
        printf("ps_start_utt 2 is false!\n");
        goto init_reco_out2;
    }

    receiver = new PCMReceiver();
    receiver->AttachToSender();
    return 0;

init_reco_out2:
    ps_free(ps2);
init_reco_out1:
    ps_free(ps);
    return -1;
}

int DeInitSpeechRec()
{
    if (receiver != NULL) {
        receiver->DetachFromSender();
        delete receiver;
        receiver = NULL;
    }

    ps_free(ps2);
    ps_free(ps);
    return 0;
}

static int speechrec_delete_pthread(void)
{
    pthread_detach(pthread_self());
    pthread_exit(0);
    return 0;
}

static int speechrec_read_process(int rectype)
{
    int cmd = CMD_SPEECH_REC;

    if (speechrec_read_event_call)
        return (*speechrec_read_event_call)(cmd, (void *)rectype, 0);
    return 0;
}

static void *speechrec_reco_pthread(void *arg)
{
    void* buffer = nullptr;
    size_t size = 0;
    int resamplesize = 0, cnt = 0;
    short* p_out_data = NULL;
    int score;
    int rectype = SPEECH_UNKNOW;
    int ignorerec = 0;
    int speechrec_cnt = 0;
    int speechrec_cnt2 = DELAY_CAPRAW_CNT;
    char const *hyp;
    char const *hyp2;

#if SAVE_PCM_RAW
    FILE* fd;
    int write_offset = 0;
    fd = fopen("/mnt/sdcard/record.pcm", "wb+");
    if (fd == NULL) {
        printf("DEBUG open /mnt/sdcard/record.pcm error =%d ,errno = %d", fd, errno);
        write_offset = 0;
    }
#endif

    InitSpeechRec();
    while (!speechrec_data.pthread_reco_exit) {
        receiver->ObtainBuffer(buffer, size, 1000);
        // do something. if time out, buffer and size will not set
        short* p_capture_buf = (short*)buffer;
        if (p_out_data == NULL) {
            p_out_data = (short*)malloc(size >> 1);
            printf("&&&& obtain buffer: %p, %u\n", buffer, size);
            printf("&&&& obtain p_out_data: %p, %u\n", p_out_data, size >> 1);
        }
        for (cnt = 0, resamplesize = 0; cnt < (size >> 1); cnt += 2) {
            *(p_out_data + resamplesize) =  (*(p_capture_buf + cnt) >> 1) + (*(p_capture_buf + cnt + 1) >> 1);
            resamplesize++;
        }
#if SAVE_PCM_RAW
        fwrite(buffer, resamplesize, sizeof(short), fd);
        write_offset += resamplesize * sizeof(short);
#endif
        if (ignorerec > 0) {
            ignorerec--;
            if (0 == ignorerec)
                printf("ps_process_raw do again\n");
        } else {
            if (speechrec_cnt < CAPRAW_CNT) {
                ps_process_raw(ps, (int16 const *)p_out_data, resamplesize, FALSE, FALSE);
                speechrec_cnt++;
            }
            if (speechrec_cnt == CAPRAW_CNT) {
                speechrec_cnt = 0;
                ps_end_utt(ps);
                hyp = ps_get_hyp(ps, &score);
                if (hyp != NULL) {
                    rectype = dealwith(hyp);
                    if (rectype != SPEECH_UNKNOW) {
                        printf("ps_get_hyp-Recognized 1: *%s*\n", hyp);
                        speechrec_read_process(rectype);
                        if(rectype != SPEECH_START_WAKEUP)
                            ignorerec = IGNORE_CAPRAW_CNT;
                        speechrec_cnt = 0;
                        speechrec_cnt2 = DELAY_CAPRAW_CNT;
                        ps_end_utt(ps2);
                        ps_start_utt(ps2);
                    }
                }
                ps_start_utt(ps);
            }

            if (speechrec_cnt2 < CAPRAW_CNT) {
                ps_process_raw(ps2, (int16 const *)p_out_data, resamplesize, FALSE, FALSE);
                speechrec_cnt2++;
            }
            if (speechrec_cnt2 == CAPRAW_CNT) {
                speechrec_cnt2 = 0;
                ps_end_utt(ps2);
                hyp2 = ps_get_hyp(ps2, &score);
                if (hyp2 != NULL) {
                    rectype = dealwith(hyp2);
                    if (rectype != SPEECH_UNKNOW) {
                        printf("ps_get_hyp-Recognized 2: *%s*\n", hyp2);
                        speechrec_read_process(rectype);
                        if(rectype != SPEECH_START_WAKEUP)
                            ignorerec = IGNORE_CAPRAW_CNT;
                        speechrec_cnt = 0;
                        speechrec_cnt2 = DELAY_CAPRAW_CNT;
                        ps_end_utt(ps);
                        ps_start_utt(ps);
                    }
                }
                ps_start_utt(ps2);
            }
        }

        receiver->ReleaseBuffer();

#if SAVE_PCM_RAW
        if (write_offset >= size * 256)
            speechrec_data.pthread_reco_exit = 1;
#endif
    }

#if SAVE_PCM_RAW
    fflush(fd);
    fclose(fd);
#endif

    free(p_out_data);
    p_out_data = NULL;
    DeInitSpeechRec();
    speechrec_delete_pthread();
}

static int speechrec_create_pthread(void)
{

    if (pthread_create(&speechrec_data.reco_tid, NULL, speechrec_reco_pthread, NULL)) {
        printf("create ril_thread pthread failed\n");
        speechrec_data.pthread_reco_state = 0;
        return -1;
    }
    return 0;
}

int speechrec_register(void)
{
    memset(&speechrec_data, 0, sizeof(speechrec_data));
    speechrec_data.pthread_reco_state = 1;
    speechrec_create_pthread();
    return 0;
}

int speechrec_unregister(void)
{
    int ret = -1;

    if (speechrec_data.pthread_reco_state == 1) {
        speechrec_data.pthread_reco_exit = 1;
        pthread_join(speechrec_data.reco_tid, NULL);
        speechrec_data.pthread_reco_state = 0;
    }
    memset(&speechrec_data, 0, sizeof(speechrec_data));
    return 0;
}

static int dealwith(char const * hyp)
{
    int rectype = SPEECH_UNKNOW;

    if(!speechrec_wakeup){
        if (strstr(hyp, "小易在吗") != NULL) {
            rectype = SPEECH_START_WAKEUP;
            speechrec_wakeup = true;
            printf("speechrec-------------------------wake up\n");
        }
    }else{
        if (strstr(hyp, "开启录像") != NULL) {
            rectype =  SPEECH_START_RECORD;
        } else if (strstr(hyp, "关闭录像") != NULL) {
            rectype =  SPEECH_STOP_RECORD;
        } else if (strstr(hyp, "开启倒车") != NULL) {
            rectype = SPEECH_BACK_CAR;
        } else if (strstr(hyp, "关闭倒车") != NULL) {
            rectype = SPEECH_CANCEL_BACK_CAR;
        } else if (strstr(hyp, "拍照") != NULL) {
            rectype = SPEECH_TAKE_PHOTOS;
        } else if (strstr(hyp, "视频上锁") != NULL || strstr(hyp, "上锁视频") != NULL) {
            rectype = SPEECH_LOCK_FILE;
        } else if (strstr(hyp, "开启屏保") != NULL) {
            rectype = SPEECH_OPENSCREEN;
        } else if (strstr(hyp, "关闭屏保") != NULL) {
            rectype = SPEECH_CLOSESCREEN;
        }
        speechrec_wakeup = false;
        printf("speechrec-------------------------need wake up\n");
    }

    return rectype;
}
#endif
