/**
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd
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

#include "video.hpp"
#include "video_isp.hpp"
#include "nv12_3dnr.hpp"
#include "nv12_process.hpp"
#include "mjpg_process.hpp"
#include "yuyv_process.hpp"
#include "h264_process.hpp"
#include "nv12_adas.hpp"
#include "video_usb.hpp"
#include "video_cif.hpp"

static pthread_attr_t global_attr;

bool is_record_mode = true;
bool is_dvs_on;

static int nv12_raw_cnt = 0;

unsigned int user_noise = 0;

HAL_COLORSPACE color_space = HAL_COLORSPACE_SMPTE170M;

static bool disp_black = true;

bool cif_mirror = false;
static bool set_cif_mirror = false;

struct Video* video_list = NULL;
#ifdef USE_DISP_TS
struct ScaleEncodeTSHandler* g_disp_ts_handler = NULL;
#endif
static pthread_rwlock_t notelock;
static bool record_init_flag = false;
static int enablerec = 0; // TODO: enablerec seems not necessary
static int enableaudio = 1;

static list<pthread_t> record_id_list;

#ifdef _PCBA_SERVICE_
static unsigned char cif_type_alloc_bit = 0;
#endif

bool with_mp = false;
bool with_adas = false;
bool with_sp = false;
static int video_rec_state = VIDEO_STATE_STOP;

AudioEncodeHandler global_audio_ehandler(0, !enableaudio, &global_attr);

struct rk_cams_dev_info g_test_cam_infos;
static void (*rec_event_call)(int cmd, void *msg0, void *msg1);

static void (*videos_inited_cb)(int is_record_mode);

static int video_uvc_encode_init(struct Video* video)
{
#if USE_USB_WEBCAM
    video->uvc_enc = new uvc_encode();
    if (!video->uvc_enc) {
        printf("no memory!\n");
        return -1;
    }
    return uvc_encode_init(video->uvc_enc);
#endif
    return 0;
}

static void video_uvc_encode_exit(struct Video* video)
{
#if USE_USB_WEBCAM
    if (video->uvc_enc) {
        uvc_encode_exit(video->uvc_enc);
        delete video->uvc_enc;
        video->uvc_enc = NULL;
    }
#endif
}

static void video_record_wait(struct Video* video)
{
    pthread_mutex_lock(&video->record_mutex);
    if (video->pthread_run)
        pthread_cond_wait(&video->record_cond, &video->record_mutex);
    pthread_mutex_unlock(&video->record_mutex);
}

static void video_record_timedwait(struct Video* video)
{
    struct timespec timeout;
    struct timeval now;
    gettimeofday(&now, NULL);
    timeout.tv_sec = now.tv_sec + 1;
    timeout.tv_nsec = now.tv_usec * 1000;

    pthread_mutex_lock(&video->record_mutex);
    if (video->pthread_run)
        pthread_cond_timedwait(&video->record_cond, &video->record_mutex, &timeout);
    pthread_mutex_unlock(&video->record_mutex);
}

void video_record_signal(struct Video* video)
{
    pthread_mutex_lock(&video->record_mutex);
    video->pthread_run = 0;
    pthread_cond_signal(&video->record_cond);
    pthread_mutex_unlock(&video->record_mutex);
}

static struct Video* getfastvideo(void)
{
    return video_list;
}

void video_record_getfilename(char* str,
                              unsigned short size,
                              const char* path,
                              int ch,
                              const char* filetype,
                              const char* suffix)
{
    time_t now;
    struct tm* timenow;
    int year, mon, day, hour, min, sec;

    time(&now);
    timenow = localtime(&now);

    year = timenow->tm_year + 1900;
    mon = timenow->tm_mon + 1;
    day = timenow->tm_mday;
    hour = timenow->tm_hour;
    min = timenow->tm_min;
    sec = timenow->tm_sec;
    snprintf(str, size, "%s/%04d%02d%02d_%02d%02d%02d_%c%s.%s", path, year, mon,
             day, hour, min, sec, 'A' + ch, suffix, filetype);
}

static int video_record_getaudiocard(struct Video* video)
{
    int card = -1;
    DIR* dp;
    struct dirent* dirp;
    char path[128];
    char* temp1;
    char* temp2;
    int sid;
    int id;
    char devicename[32] = {0};
    int len;

    temp1 = strchr((char*)(video->businfo), '-') + 1;
    if (!temp1)
        return -1;
    else
        temp2 = strchr(temp1, '-');
    if (!temp2)
        return -1;
    len = (int)temp2 - (int)temp1;
    sscanf(temp2 + 1, "%d", &sid);
    strncpy(devicename, temp1, len);
    sscanf(&devicename[len - 1], "%d", &id);
    id += 1;

    snprintf(path, sizeof(path), "/sys/devices/%s/usb%d/%d-%d/%d-%d:%d.2/sound/",
             devicename, id, id, sid, id, sid, sid);
    printf("dir = %s\n", path);
    if ((dp = opendir(path)) != NULL) {
        while ((dirp = readdir(dp)) != NULL) {
            char* pcard;
            if (strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0)
                continue;

            pcard = strstr(dirp->d_name, "card");
            if (pcard) {
                sscanf(&dirp->d_name[4], "%d", &card);
            }
        }
        closedir(dp);
    }

    return card;
}

void video_dvs_enable(int enable)
{
    is_dvs_on = enable ? 1 : 0;
}

bool video_dvs_is_enable(void)
{
    return is_dvs_on;
}

#ifndef USE_DISP_TS
static int video_ts_init(struct Video* video)
{
    MediaConfig config;
    VideoConfig& vconfig = config.video_config;
    vconfig.fmt = PIX_FMT_NV12;
    vconfig.width = SCALED_WIDTH;
    vconfig.height = SCALED_HEIGHT;
    vconfig.bit_rate = SCALED_BIT_RATE;
    vconfig.frame_rate = video->fps_d;
    vconfig.level = 51;
    vconfig.gop_size = 30;
    vconfig.profile = 100;
    vconfig.quality = MPP_ENC_RC_QUALITY_MEDIUM;
    vconfig.qp_step = 6;
    vconfig.qp_min = 18;
    vconfig.qp_max = 48;
    vconfig.rc_mode = MPP_ENC_RC_MODE_CBR;
    ScaleEncodeTSHandler* ts_handler = new ScaleEncodeTSHandler(config);
    vconfig.width = video->output_width;
    vconfig.height = video->output_height;
    if (!ts_handler || ts_handler->Init(config)) {
        fprintf(stderr, "create ts handler failed\n");
        return -1;
    }
    video->ts_handler = ts_handler;

    return 0;
}

static void video_ts_deinit(struct Video* video)
{
    if (video->ts_handler) {
        delete video->ts_handler;
        video->ts_handler = NULL;
    }
}
#else
static ScaleEncodeTSHandler* get_ts_handler(void)
{
    return g_disp_ts_handler;
}

static void video_disp_callback(int fd, int width, int height)
{
    ScaleEncodeTSHandler* ts_handler = get_ts_handler();

    assert(fd > 0);
    if (ts_handler) {
        struct timeval t;
        VideoConfig config;

        config.fmt = PIX_FMT_NV12;
        config.width = width;
        config.height = height;
        gettimeofday(&t, NULL);
        ts_handler->Process(fd, config, t);
    }
}

static ScaleEncodeTSHandler* display_ts_init(void)
{
    MediaConfig config;
    VideoConfig& vconfig = config.video_config;
    vconfig.fmt = PIX_FMT_NV12;
    vconfig.width = SCALED_WIDTH;
    vconfig.height = SCALED_HEIGHT;
    vconfig.bit_rate = SCALED_BIT_RATE;
    vconfig.frame_rate = 30;
    vconfig.level = 51;
    vconfig.gop_size = 30;
    vconfig.profile = 100;
    vconfig.quality = MPP_ENC_RC_QUALITY_MEDIUM;
    vconfig.qp_step = 6;
    vconfig.qp_min = 18;
    vconfig.qp_max = 48;
    vconfig.rc_mode = MPP_ENC_RC_MODE_CBR;
    ScaleEncodeTSHandler* ts_handler = new ScaleEncodeTSHandler(config);

    if (!ts_handler || ts_handler->Init(config)) {
        fprintf(stderr, "create ts handler failed\n");
        return NULL;
    }
    g_disp_ts_handler = ts_handler;

    return g_disp_ts_handler;
}

static void display_ts_deinit(ScaleEncodeTSHandler* ts_handler)
{
    if (ts_handler) {
        delete ts_handler;
        ts_handler = NULL;
    }
}

#endif

static void send_record_time(EncodeHandler* handler, int sec)
{
    UNUSED(handler);
    static int last = -1;
    if (last == sec)
        return;
    else
        last = sec;

    if (rec_event_call)
        (*rec_event_call)(CMD_UPDATETIME, (void *)sec, (void *)0);
}

// must be called among locking notelock
static void set_record_time_cb()
{
    struct Video* video_cur = getfastvideo();
    bool check_h264 = false;
    while (video_cur) {
        if (video_record_h264_have_added() && 0 == video_cur->disp_position
            && VIDEO_TYPE_USB == video_cur->type)
            check_h264 = true;
        video_cur = video_cur->next;
    }
    video_cur = getfastvideo();
    if (check_h264) {
        while (video_cur) {
            if (video_cur->encode_handler) {
                if (VIDEO_TYPE_USB == video_cur->type)
                    video_cur->encode_handler->record_time_notify = send_record_time;
                else
                    video_cur->encode_handler->record_time_notify = NULL;
            }
            video_cur = video_cur->next;
        }
    } else {
        while (video_cur) {
            if (video_cur->encode_handler) {
                if (0 == video_cur->disp_position)
                    video_cur->encode_handler->record_time_notify = send_record_time;
                else
                    video_cur->encode_handler->record_time_notify = NULL;
            }
            video_cur = video_cur->next;
        }
    }
}

EncodeHandler* create_encode_handler(struct Video* video, int w, int h)
{
    int audio_id = -1;
    int fps = 30;
    if (video->type != VIDEO_TYPE_ISP) {
        audio_id = video_record_getaudiocard(video);
        fps = video->frmFmt.fps;
    } else {
        fps = video->fps_d;
    }

    EncodeHandler* handler = EncodeHandler::create(
                                 video->deviceid, video->type, video->usb_type,
                                 w, h, fps, audio_id);
    if (handler) {
        handler->set_global_attr(&global_attr);
        handler->set_audio_mute(enableaudio ? false : true);
        snprintf(handler->file_format, sizeof(handler->file_format),
                 "%s", parameter_get_storage_format());
    }
    return handler;
}

void destroy_encode_handler(EncodeHandler *&encode_handler)
{
    if (encode_handler) {
#ifdef USE_WATERMARK
        encode_handler->watermark = NULL;
#endif
        global_audio_ehandler.RmPacketDispatcher(
            encode_handler->get_pkts_dispatcher());
        delete encode_handler;
        encode_handler = NULL;
    }
}

static int video_encode_init(struct Video* video)
{
    EncodeHandler* handler = create_encode_handler(video,
                                                   video->output_width,
                                                   video->output_height);
    if (!handler)
        return -1;
    video->encode_handler = handler;

#if USE_GPS_MOVTEXT
    if (video->type == VIDEO_TYPE_ISP)
        gps_set_encoderhandler(video->encode_handler);
#endif

#if MAIN_APP_NEED_DOWNSCALE_STREAM
    if (video_isp_get_enc_scale(video)) {
        handler = create_encode_handler(video, DOWN_SCALE_WIDTH, DOWN_SCALE_HEIGHT);
        if (!handler)
            return -1;
        handler->small_encodeing = true;
        video->encode_handler_s = handler;
    }
#endif
    return 0;
}

static void video_encode_exit(struct Video* video)
{
    PRINTF_FUNC;
    video->save_en = 0;
    destroy_encode_handler(video->encode_handler);
#if MAIN_APP_NEED_DOWNSCALE_STREAM
    destroy_encode_handler(video->encode_handler_s);
#endif
    PRINTF_FUNC_OUT;
}

void video_record_takephoto_end(struct Video* video)
{
    struct Video* video_cur;
    bool end = false;

    video->photo.state = PHOTO_END;
    pthread_rwlock_wrlock(&notelock);

    video_cur = getfastvideo();
    while (video_cur) {
        if (video_cur->photo.state != PHOTO_END)
            break;
        video_cur = video_cur->next;
    }
    if (!video_cur)
        end = true;
    pthread_rwlock_unlock(&notelock);

    // send message for photoend
    if (end && rec_event_call)
        (*rec_event_call)(CMD_PHOTOEND, (void *)0, (void *)1);
}

static int mjpg_decode_init(struct Video* video)
{
    video->jpeg_dec.decode = (struct vpu_decode*)calloc(1, sizeof(struct vpu_decode));
    if (!video->jpeg_dec.decode) {
        printf("no memory!\n");
        return -1;
    }

    if (vpu_decode_jpeg_init(video->jpeg_dec.decode, video->width, video->height)) {
        printf("%s vpu_decode_jpeg_init fail\n", __func__);
        return -1;
    }

    return 0;
}

static void mjpg_decode_exit(struct Video* video)
{
    if (video->jpeg_dec.decode) {
        vpu_decode_jpeg_done(video->jpeg_dec.decode);
        free(video->jpeg_dec.decode);
        video->jpeg_dec.decode = NULL;
    }
}

static void video_record_thermal_fun(struct Video* video)
{
    struct video_param *param = parameter_get_video_frontcamera_reso();
    int fps = (param->fps > 30 && !is_record_mode) ? 30 : param->fps;
    int status = thermal_get_status();

    switch (status) {
    case THERMAL_LEVEL2:
        video->high_temp = false;
        break;
    case THERMAL_LEVEL3:
        video->high_temp = true;
        break;
    case THERMAL_LEVEL5:
#if !USE_USB_WEBCAM
        if (1 != video->fps_n || (fps / 2) != video->fps_d)
            video_set_fps(video, 1, (fps / 2));
#endif
        break;
    default:
        break;
    }

#if !USE_USB_WEBCAM
    if (status != THERMAL_LEVEL5)
        if (1 != video->fps_n || fps != video->fps_d)
            video_set_fps(video, 1, fps);
#endif
}

int video_set_fps(struct Video* video, int numerator, int denominator)
{
    int ret = 0;
    HAL_FPS_INFO_t fps;
    fps.numerator = numerator;
    fps.denominator = denominator;
    printf("fps : %d/%d\n", fps.numerator, fps.denominator);
    ret = video->hal->dev->setFps(fps);
    if (!ret) {
        video->fps_n = numerator;
        video->fps_d = denominator;
        printf("video set fps is %.2f\n", 1.0 * video->fps_d / video->fps_n);
    }
    return ret;
}

static int video_set_white_balance(struct Video* video, int i)
{
    enum HAL_WB_MODE mode;

    switch (i) {
    case 0:
        mode = HAL_WB_AUTO;
        break;
    case 1:
        mode = HAL_WB_DAYLIGHT;
        break;
    case 2:
        mode = HAL_WB_FLUORESCENT;
        break;
    case 3:
        mode = HAL_WB_CLOUDY_DAYLIGHT;
        break;
    case 4:
        mode = HAL_WB_INCANDESCENT;
        break;
    default:
        printf("video%d set white balance input error!\n", video->deviceid);
        return -1;
    }

    if (video->hal->dev->setWhiteBalance(mode)) {
        printf("video%d set white balance failed!\n", video->deviceid);
        return -1;
    }

    printf("video%d set white balance sucess!\n", video->deviceid);

    return 0;
}

static int video_set_exposure_compensation(struct Video* video, int i)
{
    int aeBias;

    switch (i) {
    case 0:
        aeBias = -300;
        break;
    case 1:
        aeBias = -200;
        break;
    case 2:
        aeBias = -100;
        break;
    case 3:
        aeBias = 0;
        break;
    case 4:
        aeBias = 100;
        break;
    default:
        printf("video%d set AeBias input error!\n", video->deviceid);
        return -1;
    }

    if (video->hal->dev->setAeBias(aeBias)) {
        printf("video%d set AeBias failed!\n", video->deviceid);
        return -1;
    }

    printf("video%d set AeBias success!\n", video->deviceid);

    return 0;
}

static int video_set_brightness(struct Video* video, int i)
{
    int val;

    /* Map 0~255 to -100~100 */
    val = (i - 127.0 ) / 127.0 * 100.0;
    if (video->hal->dev->setBrightness(val)) {
        printf("video%d setBrightness failed!\n", video->deviceid);
        return -1;
    }

    printf("video%d setBrightness:%d success!\n", video->deviceid, val);

    return 0;
}

static int video_set_contrast(struct Video* video, int i)
{
    int val;

    /* Map 0~255 to -100~100 */
    val = (i - 127.0 ) / 127.0 * 100.0;
    if (video->hal->dev->setContrast(val)) {
        printf("video%d setContrast failed!\n", video->deviceid);
        return -1;
    }

    printf("video%d set setContrast:%d success!\n", video->deviceid, val);

    return 0;
}

static int video_set_hue(struct Video* video, int i)
{
    int val;

    /* Map 0~255 to -90~90 */
    val = (i - 127.0 ) / 127.0 * 90.0;
    if (video->hal->dev->setHue(val)) {
        printf("video%d setHue failed!\n", video->deviceid);
        return -1;
    }

    printf("video%d set setHue:%d success!\n", video->deviceid, val);

    return 0;
}

static int video_set_saturation(struct Video* video, int i)
{
    int val;

    /* Map 0~256 to -100~100 */
    val = (i - 127.0 ) / 127.0 * 100.0;
    if (video->hal->dev->setSaturation(val)) {
        printf("video%d setSaturation failed!\n", video->deviceid);
        return -1;
    }

    printf("video%d set setSaturation:%d success!\n", video->deviceid, val);

    return 0;
}

static int video_set_power_line_frequency(struct Video* video, int i)
{
    if (i != 1 && i != 2) {
        printf("%s error paramerter:%d!\n", __func__, i);
        return 0;
    }

    if (video->hal->dev->setAntiBandMode((enum HAL_AE_FLK_MODE)i)) {
        printf("video%d set power line frequency failed!\n", video->deviceid);
        return -1;
    }

    return 0;
}

int video_try_format(struct Video* video, frm_info_t& in_frmFmt)
{
    if (!video->hal->dev->tryFormat(in_frmFmt, video->frmFmt)) {
        switch (video->frmFmt.frmFmt) {
        case HAL_FRMAE_FMT_MJPEG:
            video->usb_type = USB_TYPE_MJPEG;
            break;
        case HAL_FRMAE_FMT_YUYV:
            video->usb_type = USB_TYPE_YUYV;
            break;
        case HAL_FRMAE_FMT_H264:
            video->usb_type = USB_TYPE_H264;
            break;
        default:
            break;
        }
    } else
        return -1;

    video->width = video->frmFmt.frmSize.width;
    video->height = video->frmFmt.frmSize.height;
    video->fps_d = video->frmFmt.fps;

    return 0;
}

// must be called among locking notelock
void video_h264_save(struct Video* video)
{
    struct Video* video_cur;

    video_cur = getfastvideo();
    while (video_cur) {
        if (!strncmp((char*)(video_cur->businfo), (char*)(video->businfo),
                     sizeof(video->businfo))) {
            if (video_cur->usb_type != USB_TYPE_H264)
                video_cur->save_en = 0;
            else
                video->save_en = 0;
        }

        video_cur = video_cur->next;
    }
}

// must be called among locking notelock
bool video_record_h264_have_added(void)
{
    struct Video* video = NULL;
    bool ret = false;

    video = getfastvideo();

    while (video) {
        if (video->usb_type == USB_TYPE_H264) {
            ret = true;
            break;
        }
        video = video->next;
    }

    return ret;
}

int video_init_setting(struct Video* video)
{
    if (video->type != VIDEO_TYPE_CIF) {
        if (video_set_power_line_frequency(video, parameter_get_video_fre()))
            return -1;
    }

    if (video_set_white_balance(video, parameter_get_wb()))
        return -1;

    if (video->type == VIDEO_TYPE_ISP) {
        if (video_set_exposure_compensation(video, parameter_get_ex()))
            return -1;
    }

    return 0;
}

// must be called among locking notelock
static inline void video_record_addnode(struct Video* video)
{
    struct Video* video_cur;

    video_cur = getfastvideo();
    if (video_cur == NULL) {
        video_list = video;
    } else {
        while (video_cur->next) {
            video_cur = video_cur->next;
        }
        video->pre = video_cur;
        video_cur->next = video;
    }
}

static void video_record_deletenode(struct Video* video)
{
    pthread_rwlock_wrlock(&notelock);

    if (video->pre == 0) {
        video_list = video->next;
        if (video_list)
            video_list->pre = 0;
    } else {
        video->pre->next = video->next;
        if (video->next)
            video->next->pre = video->pre;
    }

    video->pre = NULL;
    video->next = NULL;

#ifdef _PCBA_SERVICE_
    if ( video->type == VIDEO_TYPE_CIF)
        cif_type_alloc_bit = cif_type_alloc_bit&~(1 << video->pcba_cif_type);

    if (video_ion_free(&video->pcba_ion))
        printf("benjo.lei : delete ion fault!\n");
#endif

    if (video->usb_type == USB_TYPE_MJPEG)
        mjpg_decode_exit(video);

#ifdef USE_WATERMARK
    watermark_deinit(&video->watermark);
    watermark_deinit(&video->photo.watermark);
#endif

    if (video->hal) {
        delete video->hal;
        video->hal = NULL;
    }

    video_uvc_encode_exit(video);

    pthread_mutex_destroy(&video->record_mutex);
    pthread_cond_destroy(&video->record_cond);

    shield_the_window(video->disp_position, disp_black);
    display_reset_position(video->disp_position);

    if (video)
        free(video);

    set_record_time_cb();
    pthread_rwlock_unlock(&notelock);
}

static void video_record_wait_flag(const bool* flag, const char* name)
{
    struct timeval t0, t1;
    gettimeofday(&t0, NULL);

    while (*flag) {
        gettimeofday(&t1, NULL);
        if (t1.tv_sec - t0.tv_sec > 0) {
            printf("%s %s\n", __func__, name);
            t0 = t1;
        }
        pthread_yield();
    }
}

// phone app
static void check_videos_ready()
{
    struct Video* video_node = NULL;
    bool ready = true;
    pthread_rwlock_wrlock(&notelock);
    if (!record_init_flag) {
        printf("video list have not been inited!\n");
        pthread_rwlock_unlock(&notelock);
        return;
    }
    video_node = getfastvideo();
    while (video_node) {
        ready &= video_node->valid;
        video_node = video_node->next;
    }
    if (ready && videos_inited_cb)
        videos_inited_cb(is_record_mode ? 1 : 0);
    pthread_rwlock_unlock(&notelock);
}

static void* video_record(void* arg)
{
    struct Video* video = (struct Video*)arg;

    prctl(PR_SET_NAME, "video_record", 0, 0, 0);
    printf("%s:%d\n", __func__, __LINE__);

    if (video->usb_type == USB_TYPE_MJPEG) {
        if (mjpg_decode_init(video)) {
            goto record_exit;
        }
    }

    printf("%s:%d\n", __func__, __LINE__);

    if (video_photo_init(video))
        goto record_exit;

    printf("%s:%d\n", __func__, __LINE__);

    if (video->type == VIDEO_TYPE_ISP) {
        if (isp_video_path(video))
            goto record_exit;

        if (isp_video_start(video)) {
            printf("isp video start err!\n");
            goto record_exit;
        }
    } else if (video->type == VIDEO_TYPE_USB) {
        if (usb_video_path(video))
            goto record_exit;

        if (usb_video_start(video)) {
            printf("usb video start err!\n");
            goto record_exit;
        }
    } else if (video->type == VIDEO_TYPE_CIF) {
        if (cif_video_path(video))
            goto record_exit;

        if (cif_video_start(video)) {
            printf("cif video start err!\n");
            goto record_exit;
        }
    }

#ifdef USE_WATERMARK
    /* video watermark */
    if (is_record_mode) {
        if (!watermark_config(video->output_width, video->output_height, &video->watermark))
            watermark_init(&video->watermark);
    }

    /* photo watemark */
    if (!watermark_config(video->photo.width, video->photo.height, &video->photo.watermark))
        watermark_init(&video->photo.watermark);

    if (video->encode_handler)
        video->encode_handler->watermark = &video->watermark;
#endif

    printf("%s start\n", __func__);
    video->valid = true;
    check_videos_ready();

    while (video->pthread_run) {
        if (video->type == VIDEO_TYPE_ISP) {
            if (video_isp_add_policy(video)) {
                video->pthread_run = 0;
                break;
            }
            video_record_thermal_fun(video);
            video_record_timedwait(video);
        } else {
            video_record_wait(video);
        }
    }

record_exit:
    video_record_wait_flag(&video->mp4_encoding, "mp4_encoding");
    video_record_wait_flag(&video->jpeg_dec.decoding, "jpeg_dec.decoding");
#if MAIN_APP_NEED_DOWNSCALE_STREAM
    shared_ptr<DPP_NV12_Encode_S> dnr_enc_s = video->hal->dnr_enc_s;
    if (dnr_enc_s)
        video_record_wait_flag(&dnr_enc_s->encoding, "dsp small encoding");
    shared_ptr<NV12_Encode_S> rga_enc_s = video->hal->rga_enc_s;
    if (rga_enc_s)
        video_record_wait_flag(&rga_enc_s->encoding, "rga small encoding");
#endif

    if (is_record_mode)
        video_encode_exit(video);

    if (video->type == VIDEO_TYPE_ISP)
        isp_video_deinit(video);
    else if (video->type == VIDEO_TYPE_USB)
        usb_video_deinit(video);
    else if (video->type == VIDEO_TYPE_CIF)
        cif_video_deinit(video);

#ifndef USE_DISP_TS
    video_ts_deinit(video);
#endif

    video_photo_exit(video);

    // if video never set to valid, there must be an error
    bool error_occur = !video->valid;

    video_record_deletenode(video);
    video = NULL;
    if (error_occur)
        check_videos_ready();

    // uevent call video_record_deletevideo() or other error occur, need to detach self
    pthread_rwlock_wrlock(&notelock);
    if (record_init_flag) {
        printf("pthread_detach self: %lu\n", pthread_self());
        pthread_detach(pthread_self());
        for (list<pthread_t>::iterator it = record_id_list.begin();
             it != record_id_list.end(); ++it) {
            if (*it == pthread_self()) {
                record_id_list.erase(it);
                break;
            }
        }
    }
    pthread_rwlock_unlock(&notelock);

    pthread_exit(NULL);
}

static int video_record_query_businfo(struct Video* video, int id)
{
    int fd = 0;
    char dev[20] = {0};
    struct v4l2_capability cap;

#if USE_USB_WEBCAM
    if (id == uvc_get_user_video_id())
        return -1;
#endif

    snprintf(dev, sizeof(dev), "/dev/video%d", id);
    fd = open(dev, O_RDWR);
    if (fd <= 0) {
        // printf("open %s failed\n",dev);
        return -1;
    }

    if (ioctl(fd, VIDIOC_QUERYCAP, &cap)) {
        // printf("%s VIDIOC_QUERYCAP failed!\n",dev);
        close(fd);
        return -1;
    }

    close(fd);

    memcpy(video->businfo, cap.bus_info, sizeof(video->businfo));
    //printf("%s businfo:%s\n",dev,video->businfo);

    return 0;
}

// must be called among locking notelock
static inline bool video_record_isp_have_added(void)
{
    struct Video* video = NULL;
    bool ret = false;

    video = getfastvideo();

    while (video) {
        if (video->type == VIDEO_TYPE_ISP) {
            ret = true;
            break;
        }
        video = video->next;
    }

    return ret;
}

static bool video_check_abmode(struct Video *video)
{
    if ((parameter_get_abmode() == 0 && video->front) ||
        (parameter_get_abmode() == 1 && !video->front) ||
        (parameter_get_abmode() == 2))
        return true;
    else
        return false;
}

// In order to ensure the camhal will call h264_encode_process
// at least once after it gets an error by hotplug usb camera
static void invoke_fake_buffer_ready(void* arg)
{
    struct Video* vnode = (struct Video*)arg;
    if (!vnode || !vnode->hal)
        return;
    if (vnode->type == VIDEO_TYPE_USB) {
        if (vnode->usb_type == USB_TYPE_H264) {
            if (vnode->hal->h264_enc.get())
                vnode->hal->h264_enc->invokeFakeNotify();
        } else {
            if (vnode->hal->nv12_enc.get())
                vnode->hal->nv12_enc->invokeFakeNotify();
        }
    } else {
        // cvbs TODO
        printf("!!TODO, not usb video is dynamically deleted, unsupported now\n");
    }
}

static void start_record(struct Video* vnode)
{
    EncodeHandler* handler = vnode->encode_handler;
    if (handler) {
        assert(!(vnode->encode_status & RECORDING_FLAG));
        if (!handler->get_audio_capture())
            global_audio_ehandler.AddPacketDispatcher(
                handler->get_pkts_dispatcher());
        handler->send_record_file_start(global_audio_ehandler.GetEncoder());
        vnode->encode_status |= RECORDING_FLAG;
    }
}

static void stop_record(struct Video* vnode)
{
    if (!vnode->valid)
        return;
    EncodeHandler* handler = vnode->encode_handler;
    if (handler && (vnode->encode_status & RECORDING_FLAG)) {
        FuncBefWaitMsg func(invoke_fake_buffer_ready, vnode);
        handler->send_record_file_stop(&func);
        if (!handler->get_audio_capture())
            global_audio_ehandler.RmPacketDispatcher(
                handler->get_pkts_dispatcher());
        vnode->encode_status &= ~RECORDING_FLAG;
    }
}

static int video_get_resolution(struct Video* video)
{
    size_t i = 0, j = 0, k = 0;
#if USE_1440P_CAMERA
    struct video_param isp[2] = { UI_FRAME_1440P, UI_FRAME_1080P };
#else
    struct video_param isp[2] = { UI_FRAME_1080P, UI_FRAME_720P };
#endif
    struct video_param usb[2] = { UI_FRAME_720P, UI_FRAME_480P };
    // struct video_param cvbs[2] = { UI_FRAME_576I, UI_FRAME_480I };

    switch (video->type) {
    case VIDEO_TYPE_ISP:
        for (i = 0; i < ARRAY_SIZE(video->video_param) && i < ARRAY_SIZE(isp); i++) {
            video->video_param[i].width = isp[i].width;
            video->video_param[i].height = isp[i].height;
            video->video_param[i].fps = isp[i].fps;
        }
        break;
    case VIDEO_TYPE_USB:
        for (i = 0; i < ARRAY_SIZE(video->video_param) && i < ARRAY_SIZE(usb); i++) {
            frm_info_t in_frmFmt = {
                .frmSize = {(unsigned int)usb[i].width, (unsigned int)usb[i].height},
                .frmFmt = USB_FMT_TYPE,
                .colorSpace = color_space,
                .fps = 30,
            };
            frm_info_t frmFmt;
            if (!video->hal->dev->tryFormat(in_frmFmt, frmFmt)) {
                video->video_param[i].width = frmFmt.frmSize.width;
                video->video_param[i].height = frmFmt.frmSize.height;
                video->video_param[i].fps = frmFmt.fps;
            } else {
                printf("%s fail!\n", __func__);
                return -1;
            }
        }
        for (j = 0; j < i; j++) {
            for (k = j + 1; k < i; k++) {
                if (video->video_param[j].width == video->video_param[k].width &&
                    video->video_param[j].height == video->video_param[k].height) {
                    video->video_param[k].width = 0;
                    video->video_param[k].height = 0;
                }
            }
        }
        break;
    case VIDEO_TYPE_CIF:
        video->video_param[0].width = video->frmFmt.frmSize.width;
        video->video_param[0].height = video->frmFmt.frmSize.height;
        video->video_param[0].fps = video->frmFmt.fps;
        break;
    default:
        printf("%s : unsupport type\n", __func__);
        return -1;
    }

    return 0;
}

extern "C" int video_record_addvideo(int id,
                                     struct video_param* front,
                                     struct video_param* back,
                                     struct video_param* cif,
                                     char rec_immediately,
                                     char check_record_init)
{
    struct Video* video;
    int width = 0, height = 0, fps = 0;
    pthread_attr_t attr;
    int ret = 0;

    pthread_rwlock_wrlock(&notelock);

    if (check_record_init && !record_init_flag) {
        ret = -1;
        goto addvideo_ret;
    }

    if (id < 0 || id >= MAX_VIDEO_DEVICE) {
        printf("video%d exit!\n", id);
        ret = 0;
        goto addvideo_ret;
    }

    video = (struct Video*)calloc(1, sizeof(struct Video));
    if (!video) {
        printf("no memory!\n");
        goto addvideo_exit;
    }

    pthread_mutex_init(&video->record_mutex, NULL);
    pthread_cond_init(&video->record_cond, NULL);

    video->pthread_run = 1;
    video->photo.state = PHOTO_END;

    if (parameter_get_cam_num() > 1) {
        if (video_record_query_businfo(video, id))
            goto addvideo_exit;
    } else {
        memcpy(video->businfo, "isp", sizeof("isp"));
    }

    if (strstr((char*)video->businfo, "isp")) {
        if (video_record_isp_have_added())
            goto addvideo_exit;
        else
            video->type = VIDEO_TYPE_ISP;
    } else if (strstr((char*)video->businfo, "usb")) {
        video->type = VIDEO_TYPE_USB;
    } else if (strstr((char*)video->businfo, "cif")) {
#ifdef USE_CIF_CAMERA
        video->type = VIDEO_TYPE_CIF;

#ifdef _PCBA_SERVICE_
        printf("benjo.lei : cif_type_alloc_bit = %x\n", (int)cif_type_alloc_bit);
        for (i = 0; i < 4; i++) {
            if (((cif_type_alloc_bit >> i) & 0x01) == 0) {
                video->pcba_cif_type = i;
                cif_type_alloc_bit |= 1 << i;
                break;
            }
        }
        printf("benjo.lei : cif_type = %d\n", video->pcba_cif_type);
#endif
#else
        goto addvideo_exit;
#endif
    } else if (strstr((char*)video->businfo, "gadget")) {
        uvc_gadget_pthread_init(id);
        goto addvideo_exit;
    } else {
        printf("businfo error!\n");
        goto addvideo_exit;
    }

    if (strstr((char*)video->businfo, FRONT)) {
        if (!front)
            goto addvideo_exit;
        width = front->width;
        height = front->height;
        fps = (front->fps > 30 && !is_record_mode) ? 30 : front->fps;
        video->front = true;
        video->fps_n = 1;
        video->fps_d = fps;
    } else if (strstr((char*)video->businfo, CIF)) {
        if (!cif)
            goto addvideo_exit;
        width = cif->width;
        height = cif->height;
        fps = cif->fps;
        video->front = false;
    } else {
        if (!back)
            goto addvideo_exit;
        width = back->width;
        height = back->height;
        fps = back->fps;
        video->front = false;
    }

    if (!video_check_abmode(video))
        goto addvideo_exit;

    video->hal = new hal();
    if (!video->hal) {
        printf("no memory!\n");
        goto addvideo_exit;
    }

    if (video_uvc_encode_init(video))
        goto addvideo_exit;

    video->deviceid = id;
    video->save_en = 1;

    if (video->type == VIDEO_TYPE_ISP) {
        printf("video%d is isp\n", video->deviceid);
        api_adas_set_wh(width, height);
        if (isp_video_init(video, id, width, height, fps))
            goto addvideo_exit;
    } else if (video->type == VIDEO_TYPE_USB) {
        printf("video%d is usb\n", video->deviceid);
        if (usb_video_init(video, id, width, height, fps))
            goto addvideo_exit;
    } else if (video->type == VIDEO_TYPE_CIF) {
        printf("video%d is cif\n", video->deviceid);
        if (cif_video_init(video, id, width, height, fps))
            goto addvideo_exit;
    }

    if (video_get_resolution(video))
        goto addvideo_exit;

    video->pre = 0;
    video->next = 0;

    if (video->type == VIDEO_TYPE_USB && video->usb_type == USB_TYPE_H264) {
        video->disp_position = -1;
    } else {
        if ((video->disp_position = display_set_position()) < 0)
            printf("video%d will not display.\n", video->deviceid);
    }

    if (pthread_attr_init(&attr)) {
        printf("pthread_attr_init failed!\n");
        goto addvideo_exit;
    }
    if (pthread_attr_setstacksize(&attr, 256 * 1024)) {
        printf("pthread_attr_setstacksize failed!\n");
        goto addvideo_exit;
    }

#ifdef SDV
    if (is_record_mode && video->type == VIDEO_TYPE_ISP && with_mp) {
        video->output_width = width;
        video->output_height = height;
    } else {
        video->output_width = video->width;
        video->output_height = video->height;
    }
#else
    video->output_width = video->width;
    video->output_height = video->height;
#endif

#if MAIN_APP_NEED_DOWNSCALE_STREAM
    if (video->output_height > 1440)
        video_isp_set_enc_scale(video, true);
#endif

#ifndef USE_DISP_TS
    if (video_ts_init(video))
        goto addvideo_exit;
#endif

    if (is_record_mode && video_encode_init(video)) {
        goto addvideo_exit;
    }

    video_record_addnode(video);
    set_record_time_cb();

#ifdef _PCBA_SERVICE_
    memset(&video->pcba_ion, 0, sizeof(video->pcba_ion));
    video->pcba_ion.client = -1;
    video->pcba_ion.fd = -1;
    if (video_ion_alloc_rational(&video->pcba_ion, video->width, video->height, 4, 1) < 0)
        printf("benjo.lei : ion alloc fail!\n");
#endif

    if (pthread_create(&video->record_id, &attr, video_record, video)) {
        printf("%s pthread create err!\n", __func__);
        if (video->pre == 0) {
            video_list = video->next;
            if (video_list)
                video_list->pre = 0;
        } else {
            video->pre->next = video->next;
            if (video->next)
                video->next->pre = video->pre;
        }
        video->pre = NULL;
        video->next = NULL;
        goto addvideo_exit;
    }

    if (rec_immediately)
        start_record(video);

    record_id_list.push_back(video->record_id);

    if (pthread_attr_destroy(&attr))
        printf("pthread_attr_destroy failed!\n");

    ret = 0;
    goto addvideo_ret;

addvideo_exit:

    if (video) {
        if (is_record_mode)
            video_encode_exit(video);
#ifndef USE_DISP_TS
        video_ts_deinit(video);
#endif
        if (video->hal) {
            if (video->hal->dev.get())
                video->hal->dev->deInitHw();

            delete video->hal;
            video->hal = NULL;
        }

        pthread_mutex_destroy(&video->record_mutex);
        pthread_cond_destroy(&video->record_cond);

        video_uvc_encode_exit(video);

        free(video);
        video = NULL;
    }

    printf("video%d exit!\n", id);
    ret = -1;

addvideo_ret:

    pthread_rwlock_unlock(&notelock);
    api_send_msg(MSG_VIDEO_ADD_EVENT, TYPE_BROADCAST, NULL, NULL);
    return ret;
}

extern "C" int video_record_deletevideo(int deviceid)
{
    struct Video* video_cur;

    pthread_rwlock_rdlock(&notelock);
    video_cur = getfastvideo();
    while (video_cur) {
        if (video_cur->deviceid == deviceid) {
            stop_record(video_cur);
            video_record_signal(video_cur);
            break;
        }
        video_cur = video_cur->next;
    }
    pthread_rwlock_unlock(&notelock);
    api_send_msg(MSG_VIDEO_DEL_EVENT, TYPE_BROADCAST, NULL, NULL);
    return 0;
}

static void video_record_get_parameter()
{
    with_mp = true;
    with_sp = true;
    with_adas = ((parameter_get_video_adas() == 1) ? true : false);
    printf("with_mp: %d, with_sp: %d, with_adas: %d\n", with_mp, with_sp, with_adas);
}

extern "C" void video_record_init_lock()
{
    pthread_rwlock_init(&notelock, NULL);
    memset(&g_test_cam_infos, 0, sizeof(g_test_cam_infos));
    CamHwItf::getCameraInfos(&g_test_cam_infos);
}

extern "C" void video_record_destroy_lock()
{
    pthread_rwlock_destroy(&notelock);
}

extern "C" void video_record_init(struct video_param* front,
                                  struct video_param* back,
                                  struct video_param* cif)
{
    if (pthread_attr_init(&global_attr)) {
        printf("pthread_attr_init failed!\n");
        return;
    }
    if (pthread_attr_setstacksize(&global_attr, STACKSIZE)) {
        printf("pthread_attr_setstacksize failed!\n");
        return;
    }

    video_record_get_parameter();

    disp_black = true;

    cif_mirror = set_cif_mirror;
    printf("cif_mirror: %d\n", cif_mirror);

    for (int i = 0, j = 0; i < MAX_VIDEO_DEVICE && j < parameter_get_cam_num(); i++)
        if (!video_record_addvideo(i, front, back, cif, 0, 0))
            j++;

    pthread_rwlock_wrlock(&notelock);
    record_init_flag = true;
    pthread_rwlock_unlock(&notelock);
    video_rec_state = VIDEO_STATE_PREVIEW;
    check_videos_ready();
}

extern "C" void video_record_deinit(bool black)
{
    struct Video* video_cur;
    list<pthread_t> save_list;

    disp_black = black;
    pthread_rwlock_wrlock(&notelock);
    if (!record_init_flag) {
        printf("video record have been deinit!\n");
        pthread_rwlock_unlock(&notelock);
        return ;
    }
    record_init_flag = false;
    video_cur = getfastvideo();
    while (video_cur) {
        while (video_cur->photo.state != PHOTO_END)
            pthread_yield();
        video_record_signal(video_cur);
        video_cur = video_cur->next;
    }
    save_list.clear();
    for (list<pthread_t>::iterator it = record_id_list.begin();
         it != record_id_list.end(); ++it)
        save_list.push_back(*it);
    record_id_list.clear();
    pthread_rwlock_unlock(&notelock);

    for (list<pthread_t>::iterator it = save_list.begin();
         it != save_list.end(); ++it) {
        printf("pthread_join record id: %lu\n", *it);
        pthread_join(*it, NULL);
    }
    save_list.clear();

    if (pthread_attr_destroy(&global_attr))
        printf("pthread_attr_destroy failed!\n");

    video_rec_state = VIDEO_STATE_STOP;
}

int h264_encode_process(struct Video* video,
                        void* virt,
                        int fd,
                        void* hnd,
                        size_t size,
                        struct timeval& time_val,
                        PixelFormat fmt)
{
    int ret = 0;

    video->mp4_encoding = true;
    if (!video->pthread_run)
        goto exit_h264_encode_process;

    if (video->encode_handler) {
        BufferData input_data;
        input_data.vir_addr_ = virt;
        input_data.ion_data_.fd_ = fd;
        input_data.ion_data_.handle_ = (ion_user_handle_t)hnd;
        input_data.mem_size_ = size;
        input_data.update_timeval_ = time_val;
        Buffer input_buffer(input_data);
        uint8_t* spspps = nullptr;
        size_t spspps_size = 0;
        if (video->usb_type == USB_TYPE_H264) {
            input_buffer.SetValidDataSize(size);
            shared_ptr<H264_Encode> h264_enc = video->hal->h264_enc;
            spspps = h264_enc->spspps;
            spspps_size = h264_enc->spspps_size;
        }
        ret = video->encode_handler->h264_encode_process(
                  input_buffer, fmt, spspps, spspps_size);
    }

exit_h264_encode_process:
    video->mp4_encoding = false;
    return ret;
}

static inline bool encode_handler_is_ready(struct Video* node)
{
    return node->valid && node->encode_handler;
}

// what means of this ?
static inline bool video_is_active(struct Video* node)
{
    return ((parameter_get_abmode() == 0 && node->front) ||
            (parameter_get_abmode() == 1 && !node->front) ||
            (parameter_get_abmode() == 2));
}

static void start_record_file(EncodeHandler* ehandler)
{
    if (!ehandler)
        return;
    if (!ehandler->get_audio_capture())
        global_audio_ehandler.AddPacketDispatcher(
            ehandler->get_pkts_dispatcher());
    ehandler->send_record_file_start(global_audio_ehandler.GetEncoder());
}

static void stop_record_file(EncodeHandler* ehandler)
{
    if (!ehandler)
        return;
    ehandler->send_record_file_stop();
    if (!ehandler->get_audio_capture())
        global_audio_ehandler.RmPacketDispatcher(
            ehandler->get_pkts_dispatcher());
}

extern "C" int video_record_startrec(void)
{
    int ret = -1;

    if (parameter_get_gps_mark())
        gps_write_init();

    pthread_rwlock_rdlock(&notelock);
    Video* video = getfastvideo();
    while (video) {
        EncodeHandler* ehandler = video->encode_handler;
        if (ehandler &&
            !(video->encode_status & RECORDING_FLAG) &&
            video_is_active(video)) {
            enablerec += 1;
            start_record_file(ehandler);
#if MAIN_APP_NEED_DOWNSCALE_STREAM
            EncodeHandler* shandler = video->encode_handler_s;
            if (shandler)
                start_record_file(shandler);
#endif
            video->encode_status |= RECORDING_FLAG;
            ret = 0;
        }
        video = video->next;
    }
    pthread_rwlock_unlock(&notelock);
    if (ret == 0)
        video_rec_state = VIDEO_STATE_RECORD;
    PRINTF("%s, enablerec: %d\n", __func__, enablerec);
    PRINTF_FUNC_OUT;
    return ret;
}
extern "C" int runapp(char* cmd);
extern "C" void video_record_stoprec(void)
{
    pthread_rwlock_rdlock(&notelock);
    Video* video = getfastvideo();
    fprintf(stderr, "In %s, fastvideo: %p\n", __func__, video);
    while (video) {
        if (video->encode_status & RECORDING_FLAG) {
            EncodeHandler* ehandler = video->encode_handler;
            //if (encode_handler_is_ready(video)) {
            stop_record_file(ehandler);
            //}
#if MAIN_APP_NEED_DOWNSCALE_STREAM
            EncodeHandler* shandler = video->encode_handler_s;
            if (shandler)
                stop_record_file(shandler);
#endif
            video->encode_status &= ~RECORDING_FLAG;
            enablerec -= 1;
        }
        video = video->next;
    }
    pthread_rwlock_unlock(&notelock);

    if (parameter_get_gps_mark())
        gps_write_deinit();

    char cmd[] = "sync\0";
    runapp(cmd);
    PRINTF("%s, enablerec: %d\n", __func__, enablerec);
    video_rec_state = VIDEO_STATE_PREVIEW;
    PRINTF_FUNC_OUT;
}

extern "C" void video_record_switch_next_recfile(const char* filename)
{
    pthread_rwlock_rdlock(&notelock);
    Video* video = getfastvideo();
    fprintf(stderr, "In %s, fastvideo: %p\n", __func__, video);
    while (video) {
        EncodeHandler* ehandler = video->encode_handler;
        if ((video->encode_status & RECORDING_FLAG)
            && strcmp(ehandler->filename, filename) == 0) {
            ehandler->send_record_file_switch();
        }
        video = video->next;
    }
    pthread_rwlock_unlock(&notelock);
    char cmd[] = "sync\0";
    runapp(cmd);
    PRINTF_FUNC_OUT;
}

extern "C" void video_record_blocknotify(int prev_num, int later_num)
{
    Video* video_cur;
    char* filename;

    pthread_rwlock_rdlock(&notelock);
    video_cur = getfastvideo();
    if (video_cur == NULL) {
        pthread_rwlock_unlock(&notelock);
        return;
    } else {
        filename = (char*)video_cur->encode_handler->filename;
        printf("video_record_blocknotify filename = %s\n", filename);
        fs_manage_blocknotify(prev_num, later_num, filename);
        while (video_cur->next) {
            video_cur = video_cur->next;
            filename = (char*)video_cur->encode_handler->filename;
            printf("video_record_blocknotify filename = %s\n", filename);
            fs_manage_blocknotify(prev_num, later_num, filename);
        }
    }
    pthread_rwlock_unlock(&notelock);
}

extern "C" void video_record_savefile(void)
{
    pthread_rwlock_rdlock(&notelock);
    struct Video* video = getfastvideo();
    while (video) {
#ifdef MAIN_APP_CACHE_ENCODEDATA_IN_MEM
        if ((video->encode_status & CACHE_ENCODE_DATA_FLAG) &&
            encode_handler_is_ready(video) &&
            !video->encode_handler->is_cache_writing())
#else
        if (enablerec && encode_handler_is_ready(video))
#endif
            video->encode_handler->send_save_cache_start();
        video = video->next;
    }
    pthread_rwlock_unlock(&notelock);
}

void video_record_stop_savecache()
{
#ifdef MAIN_APP_CACHE_ENCODEDATA_IN_MEM
    pthread_rwlock_rdlock(&notelock);
    struct Video* video = getfastvideo();
    while (video) {
        if (video->encode_status & CACHE_ENCODE_DATA_FLAG)
            video->encode_handler->send_save_cache_stop();
        video = video->next;
    }
    pthread_rwlock_unlock(&notelock);
#endif
}

static void stop_ts_transfer()
{
    Video* video = getfastvideo();
    while (video) {
        ScaleEncodeTSHandler* ts_handler = video->ts_handler;
        // Alas! The fastvideo may be change between start/stop ts transfer...
        if (video->encode_status & WIFI_TRANSFER_FLAG) {
            if (ts_handler) {
                PacketDispatcher* pkt_dispatcher =
                    ts_handler->GetPacketDispatcher();
                pkt_dispatcher->UnRegisterCbWhenEmpty();
                global_audio_ehandler.RmPacketDispatcher(pkt_dispatcher);
                ts_handler->StopTransferStream();
                video->hal->nv12_ts->SetActive(false);
            }
            enablerec -= 1;
            video->encode_status &= ~WIFI_TRANSFER_FLAG;
        }
        video = video->next;
    }
}

void video_record_start_ts_transfer(char* url, int device_id)
{
    PRINTF_FUNC;
    pthread_rwlock_rdlock(&notelock);
    ScaleEncodeTSHandler* ts_handler;
    Video* video = getfastvideo();
    assert(url);

#ifdef USE_DISP_TS
    if (!get_ts_handler())
        ts_handler = display_ts_init();
    display_set_display_callback(video_disp_callback);
#else
    while (video) {
        if (video->deviceid == device_id)
            break;
        video = video->next;
    }
    if (video)
        ts_handler = video->ts_handler;
#endif

    if (video && url && ts_handler &&
        !(video->encode_status & WIFI_TRANSFER_FLAG)) {
        stop_ts_transfer(); // stop the previous ts transfer
        PacketDispatcher* pkt_dispatcher =
            ts_handler->GetPacketDispatcher();
        global_audio_ehandler.AddPacketDispatcher(pkt_dispatcher);
        if (!ts_handler->StartTransferStream(
                url, &global_attr, global_audio_ehandler.GetEncoder())) {
            std::function<void(void)> cb = [&, pkt_dispatcher]() {
                global_audio_ehandler.RmPacketDispatcher(pkt_dispatcher);
            };
            pkt_dispatcher->RegisterCbWhenEmpty(cb);
            enablerec += 1;
            video->encode_status |= WIFI_TRANSFER_FLAG;
            video->hal->nv12_ts->SetActive(true);
        } else {
            global_audio_ehandler.RmPacketDispatcher(pkt_dispatcher);
        }
    }
    pthread_rwlock_unlock(&notelock);
    PRINTF("%s, enablerec: %d\n", __func__, enablerec);
}

void video_record_stop_ts_transfer(char sync)
{
    PRINTF_FUNC;
    pthread_rwlock_rdlock(&notelock);
#ifdef USE_DISP_TS
    ScaleEncodeTSHandler* ts_handler = get_ts_handler();
#else
    stop_ts_transfer();
#endif

#ifdef USE_DISP_TS
    if (sync)
        display_ts_deinit(ts_handler);
    else
        display_set_display_callback(NULL);
#endif
    pthread_rwlock_unlock(&notelock);
    PRINTF("%s, enablerec: %d\n", __func__, enablerec);
}

void video_record_attach_user_muxer(void* muxer, char* uri, int need_av)
{
    pthread_rwlock_rdlock(&notelock);
    Video* video = getfastvideo();
    if (video && encode_handler_is_ready(video)) {
        enablerec += 1;
        video->encode_handler->send_attach_user_muxer((CameraMuxer*)muxer, uri,
                                                      need_av);
    }
    pthread_rwlock_unlock(&notelock);
    PRINTF("%s, enablerec: %d\n", __func__, enablerec);
}

void video_record_detach_user_muxer(void* muxer)
{
    pthread_rwlock_rdlock(&notelock);
    Video* video = getfastvideo();
    if (video && encode_handler_is_ready(video)) {
        video->encode_handler->sync_detach_user_muxer((CameraMuxer*)muxer);
        enablerec -= 1;
    }
    pthread_rwlock_unlock(&notelock);
    PRINTF("%s, enablerec: %d\n", __func__, enablerec);
}

void video_record_attach_user_mdprocessor(void* processor, void* md_attr)
{
    pthread_rwlock_rdlock(&notelock);
    Video* video = getfastvideo();
    while (video) {
        // The top video is not always isp video, search it
        if (video->type == VIDEO_TYPE_ISP)
            break;
        video = video->next;
    }
    if (video && video->encode_handler) {
        enablerec += 1;
        video->encode_status |= MD_PROCESSING_FLAG;
        video->encode_handler->send_attach_user_mdprocessor(
            (VPUMDProcessor*)processor, (MDAttributes*)md_attr);
    }
    pthread_rwlock_unlock(&notelock);
    PRINTF("%s, enablerec: %d\n", __func__, enablerec);
}
void video_record_detach_user_mdprocessor(void* processor)
{
    pthread_rwlock_rdlock(&notelock);
    Video* video = getfastvideo();
    while (video) {
        if ((video->encode_status & MD_PROCESSING_FLAG) &&
            encode_handler_is_ready(video)) {
            video->encode_handler->sync_detach_user_mdprocessor(
                (VPUMDProcessor*)processor);
            video->encode_status &= ~MD_PROCESSING_FLAG;
            enablerec -= 1;
            break;
        }
        video = video->next;
    }
    pthread_rwlock_unlock(&notelock);
    PRINTF("%s, enablerec: %d\n", __func__, enablerec);
}
void video_record_rate_change(void* processor, int low_frame_rate)
{
    pthread_rwlock_rdlock(&notelock);
    Video* video = getfastvideo();
    while (video) {
        if (video->encode_status & MD_PROCESSING_FLAG)
            break;
        video = video->next;
    }
    if (video && encode_handler_is_ready(video)) {
        video->encode_handler->send_rate_change((VPUMDProcessor*)processor,
                                                low_frame_rate);
    }
    pthread_rwlock_unlock(&notelock);
}

// Note: The value of sec should be satisfied sec % (gop/fps) == 0.
void video_record_start_cache(int sec)
{
#ifdef MAIN_APP_CACHE_ENCODEDATA_IN_MEM
    pthread_rwlock_rdlock(&notelock);
    Video* video = getfastvideo();
    while (video) {
        EncodeHandler* ehandler = video->encode_handler;
        if (ehandler &&
            !(video->encode_status & CACHE_ENCODE_DATA_FLAG)) {
            if (!ehandler->get_audio_capture())
                global_audio_ehandler.AddPacketDispatcher(
                    ehandler->get_pkts_dispatcher());
            ehandler->send_cache_data_start(sec, global_audio_ehandler.GetEncoder());
            video->encode_status |= CACHE_ENCODE_DATA_FLAG;
        }
        video = video->next;
    }
    pthread_rwlock_unlock(&notelock);
#endif
}

void video_record_stop_cache()
{
#ifdef MAIN_APP_CACHE_ENCODEDATA_IN_MEM
    pthread_rwlock_rdlock(&notelock);
    Video* video = getfastvideo();
    while (video) {
        if (video->encode_status & CACHE_ENCODE_DATA_FLAG) {
            EncodeHandler* ehandler = video->encode_handler;
            //if (encode_handler_is_ready(video))
            video->encode_handler->send_cache_data_stop();
            if (!ehandler->get_audio_capture())
                global_audio_ehandler.RmPacketDispatcher(
                    ehandler->get_pkts_dispatcher());
            video->encode_status &= ~CACHE_ENCODE_DATA_FLAG;
        }
        video = video->next;
    }
    pthread_rwlock_unlock(&notelock);
#endif
}

void video_record_reset_bitrate()
{
    pthread_rwlock_rdlock(&notelock);
    Video* video = getfastvideo();
    while (video && encode_handler_is_ready(video)) {
        video->encode_handler->reset_video_bit_rate();
        video = video->next;
    }
    pthread_rwlock_unlock(&notelock);
}

extern "C" void video_record_setaudio(int flag)
{
    pthread_rwlock_rdlock(&notelock);
    enableaudio = flag;
    global_audio_ehandler.SetMute(!flag);
    Video* video = getfastvideo();
    while (video) {
        if (video->encode_handler)
            video->encode_handler->set_audio_mute(enableaudio ? false : true);
        video = video->next;
    }
    pthread_rwlock_unlock(&notelock);
}

extern "C" int video_record_set_power_line_frequency(int i)
{
    struct Video* video = NULL;

    // 1:50Hz,2:60Hz
    if (i != 1 && i != 2) {
        printf("%s parameter wrong\n", __func__);
        return -1;
    }

    pthread_rwlock_rdlock(&notelock);
    video = getfastvideo();

    while (video) {
        video_set_power_line_frequency(video, i);
        video = video->next;
    }

    pthread_rwlock_unlock(&notelock);

    return 0;
}

extern "C" int video_record_set_white_balance(int i)
{
    struct Video* video = NULL;

    pthread_rwlock_rdlock(&notelock);
    video = getfastvideo();

    while (video) {
        video_set_white_balance(video, i);

        video = video->next;
    }

    pthread_rwlock_unlock(&notelock);

    return 0;
}

extern "C" int video_record_set_exposure_compensation(int i)
{
    struct Video* video = NULL;

    pthread_rwlock_rdlock(&notelock);
    video = getfastvideo();

    while (video) {
        video_set_exposure_compensation(video, i);

        video = video->next;
    }

    pthread_rwlock_unlock(&notelock);

    return 0;
}

extern "C" int video_record_set_brightness(int i)
{
    struct Video* video = NULL;

    pthread_rwlock_rdlock(&notelock);
    video = getfastvideo();

    while (video) {
        video_set_brightness(video, i);
        video = video->next;
    }

    pthread_rwlock_unlock(&notelock);

    return 0;
}

extern "C" int video_record_set_contrast(int i)
{
    struct Video* video = NULL;

    pthread_rwlock_rdlock(&notelock);
    video = getfastvideo();

    while (video) {
        video_set_contrast(video, i);
        video = video->next;
    }

    pthread_rwlock_unlock(&notelock);

    return 0;
}

extern "C" int video_record_set_hue(int i)
{
    struct Video* video = NULL;

    pthread_rwlock_rdlock(&notelock);
    video = getfastvideo();

    while (video) {
        video_set_hue(video, i);
        video = video->next;
    }

    pthread_rwlock_unlock(&notelock);

    return 0;
}

extern "C" int video_record_set_saturation(int i)
{
    struct Video* video = NULL;

    pthread_rwlock_rdlock(&notelock);
    video = getfastvideo();

    while (video) {
        video_set_saturation(video, i);

        video = video->next;
    }

    pthread_rwlock_unlock(&notelock);

    return 0;
}

extern "C" int video_record_takephoto(unsigned int num)
{
    struct Video* video_cur;
    int ret = -1;
    bool enable = false;

    pthread_rwlock_rdlock(&notelock);
    video_cur = getfastvideo();
    while (video_cur) {
        ret = 0;
        if (video_cur->usb_type != USB_TYPE_H264 && video_cur->photo.state == PHOTO_END
            && video_cur->pthread_run) {
            enable = true;
            video_set_photo_user_num(video_cur, num);
            video_cur->photo.state = PHOTO_ENABLE;
        }
        video_cur = video_cur->next;
    }
    pthread_rwlock_unlock(&notelock);
    if (!enable && rec_event_call)
        (*rec_event_call)(CMD_PHOTOEND, (void *)0, (void *)1);

    return ret;
}

extern "C" void video_record_display_switch()
{
    struct Video* video = NULL;
    int *pos1 = NULL, *pos2 = NULL;

    pthread_rwlock_wrlock(&notelock);
    video = getfastvideo();
    while (video) {
        if (video->disp_position >= 0) {
            if (!pos1)
                pos1 = &video->disp_position;
            else if (!pos2)
                pos2 = &video->disp_position;
        }
        if (pos1 && pos2) {
            display_switch_pos(pos1, pos2);
            pos1 = pos2;
            pos2 = NULL;
        }
        video =  video->next;
    }
    set_record_time_cb();
    pthread_rwlock_unlock(&notelock);
}

extern "C" void video_record_display_switch_to_front(int type)
{
    struct Video* video = NULL;
    int *pos1 = NULL, *pos2 = NULL;

    pthread_rwlock_wrlock(&notelock);
    video = getfastvideo();
    while (video) {
        if (video->type == type && video->disp_position >= 0) {
            pos1 = &video->disp_position;
            break;
        }
        video = video->next;
    }

    if (pos1 && *pos1) {
        video = getfastvideo();
        while (video) {
            if (video->disp_position == 0) {
                pos2 = &video->disp_position;
                break;
            }
            video = video->next;
        }
    }

    if (pos1 && pos2)
        display_switch_pos(pos1, pos2);
    set_record_time_cb();
    pthread_rwlock_unlock(&notelock);
}

extern "C" void video_record_set_record_mode(bool mode)
{
    is_record_mode = mode;
    color_space = is_record_mode ? HAL_COLORSPACE_SMPTE170M : HAL_COLORSPACE_JPEG;
    if (is_record_mode)
        rk_fb_set_yuv_range(CSC_BT601);
    else
        rk_fb_set_yuv_range(CSC_BT601F);
    printf("is_record_mode = %d, color_space = %d\n", is_record_mode, color_space);
}

extern "C" void video_record_inc_nv12_raw_cnt(void)
{
    nv12_raw_cnt++;
    printf("nv12_raw_cnt = %d\n", nv12_raw_cnt);
}

extern "C" void video_record_fps_count(void)
{
    struct Video* video = NULL;

    pthread_rwlock_rdlock(&notelock);

    video = getfastvideo();
    while (video) {
        video->fps = video->fps_total - video->fps_last;
        video->fps_last = video->fps_total;

        video = video->next;
    }

    pthread_rwlock_unlock(&notelock);
}

extern "C" int video_record_get_list_num(void)
{
    struct Video* video = NULL;
    int num = 0;

    pthread_rwlock_rdlock(&notelock);

    video = getfastvideo();
    while (video) {
        num++;

        video = video->next;
    }

    pthread_rwlock_unlock(&notelock);

    return num;
}

static void video_get_saved_resolution(struct Video* video,
                                       struct video_param* frame,
                                       int count)
{
    int num = ARRAY_SIZE(video->video_param);
    int min = ((num < count) ? num : count);
    memcpy(frame, &video->video_param, min * sizeof(struct video_param));
}

extern "C" int video_record_get_front_resolution(struct video_param* frame,
                                                 int count)
{
    struct Video* video = NULL;

    memset(frame, 0, count * sizeof(struct video_param));

    pthread_rwlock_rdlock(&notelock);

    video = getfastvideo();
    while (video) {
        if ((!strcmp("isp", FRONT) && video->type == VIDEO_TYPE_ISP) ||
            (!strcmp("usb", FRONT) && video->type != VIDEO_TYPE_ISP)) {
            video_get_saved_resolution(video, frame, count);
        }

        video = video->next;
    }

    pthread_rwlock_unlock(&notelock);

    return 0;
}

extern "C" int video_record_get_back_resolution(struct video_param* frame,
                                                int count)
{
    struct Video* video = NULL;

    memset(frame, 0, count * sizeof(struct video_param));

    pthread_rwlock_rdlock(&notelock);

    video = getfastvideo();
    while (video) {
        if ((strcmp("isp", FRONT) && video->type == VIDEO_TYPE_ISP) ||
            (strcmp("usb", FRONT) && video->type == VIDEO_TYPE_USB)) {
            video_get_saved_resolution(video, frame, count);
        }

        video = video->next;
    }

    pthread_rwlock_unlock(&notelock);

    return 0;
}

extern "C" int video_record_get_cif_resolution(struct video_param* frame,
                                               int count)
{
    struct Video* video = NULL;

    memset(frame, 0, count * sizeof(struct video_param));

    pthread_rwlock_rdlock(&notelock);

    video = getfastvideo();
    while (video) {
        if (video->type == VIDEO_TYPE_CIF)
            video_get_saved_resolution(video, frame, count);

        video = video->next;
    }

    pthread_rwlock_unlock(&notelock);

    return 0;
}

extern "C" void video_record_update_osd_num_region(int onoff)
{
    struct Video* video = NULL;

    pthread_rwlock_rdlock(&notelock);

    video = getfastvideo();
    while (video) {
        watermark_update_osd_num_region(onoff, &video->watermark);
        watermark_update_osd_num_region(onoff, &video->photo.watermark);
        video = video->next;
    }

    pthread_rwlock_unlock(&notelock);
}

extern "C" void video_record_watermark_refresh()
{
    struct Video* video = NULL;

    pthread_rwlock_rdlock(&notelock);

    video = getfastvideo();
    while (video) {
        if (is_record_mode)
            watermark_refresh(video->output_width, video->output_height, &video->watermark);
        watermark_refresh(video->photo.width, video->photo.height, &video->photo.watermark);

        video = video->next;
    }

    pthread_rwlock_unlock(&notelock);
}

static void watermark_update_time(struct watermark_info &watermark, uint32_t* src, uint32_t srclen)
{
    if (watermark.type & WATERMARK_TIME) {
        uint32_t index = watermark.buffer_index;
        uint32_t offset = watermark.osd_data_offset.time_data_offset;
        uint8_t *dst = (uint8_t*)watermark.watermark_data[index ^ 1].buffer + offset;

        watermark_update_rect_bmp(src, srclen, watermark.coord_info.time_bmp,
                                  dst, watermark.color_info);

        // The data update is complete, switch buffer.
        watermark.buffer_index = index ^ 1;
    }
}

extern "C" void video_record_update_time(uint32_t* src, uint32_t srclen)
{
    struct Video* video = NULL;

    pthread_rwlock_rdlock(&notelock);

    video = getfastvideo();
    while (video) {
        watermark_update_time(video->watermark, src, srclen);
        watermark_update_time(video->photo.watermark, src, srclen);

        video = video->next;
    }

    pthread_rwlock_unlock(&notelock);
}

extern "C" void video_record_update_license(uint32_t* src, uint32_t srclen)
{
    struct Video* video = NULL;

    pthread_rwlock_rdlock(&notelock);

    video = getfastvideo();
    while (video) {
        if (video->watermark.type & WATERMARK_LICENSE)
            watermart_get_license_data(&video->watermark, src, srclen);

        if (video->photo.watermark.type & WATERMARK_LICENSE)
            watermart_get_license_data(&video->photo.watermark, src, srclen);

        video = video->next;
    }

    pthread_rwlock_unlock(&notelock);
}

extern "C" void video_record_get_user_noise(void)
{
    FILE* fp = NULL;
    char noise[10] = {0};

    fp = fopen("/mnt/sdcard/dsp_cfg_noise", "rb");
    if (fp) {
        fgets(noise, sizeof(noise), fp);
        user_noise = atoi(noise);
        printf("user_noise = %u\n", user_noise);
        fclose(fp);
    } else {
        printf("/mnt/sdcard/dsp_cfg_noise not exist!\n");
    }
}

extern "C" void video_record_set_timelapseint(unsigned short val)
{
    EncodeHandler::TimeLapseInterval = val;
}

extern "C" int REC_RegEventCallback(void (*call)(int cmd, void *msg0, void *msg1))
{
    rec_event_call = call;
    return 0;
}

extern "C" void set_video_init_complete_cb(void (*cb)(int arg0))
{
    videos_inited_cb = cb;
}

extern "C" int video_record_get_state()
{
    return video_rec_state;
}

extern "C" bool video_record_get_record_mode(void)
{
    return is_record_mode;
}

extern "C" void video_record_set_cif_mirror(bool mirror)
{
    set_cif_mirror = mirror;
}

extern "C" bool video_record_get_cif_mirror(void)
{
    return set_cif_mirror;
}

extern "C" void video_record_set_fps(int fps)
{
    struct Video* video = NULL;

    pthread_rwlock_rdlock(&notelock);
    video = getfastvideo();
    while (video) {
        video_set_fps(video, 1, fps);
        video = video->next;
    }
    pthread_rwlock_unlock(&notelock);
}

extern "C" void video_record_get_deviceid_by_type(int *devicdid, int type, int cnt)
{
    struct Video* video = NULL;
    int i = 0;

    pthread_rwlock_rdlock(&notelock);

    video = getfastvideo();
    while (video) {
        if (video->type == type) {
            if (type != VIDEO_TYPE_CIF) {
                *devicdid = video->deviceid;
                pthread_rwlock_unlock(&notelock);
                return;
            } else {
                if (i == cnt) {
                    *devicdid = video->deviceid;
                    pthread_rwlock_unlock(&notelock);
                    return;
                }
                i++;
            }
        }
        video = video->next;
    }

    pthread_rwlock_unlock(&notelock);
    *devicdid = -1;
}
