/**
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd
 * Author: Chad.ma <chad.ma@rock-chips.com>
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

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#include <linux/watchdog.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "audio/playback/audioplay.h"
#include "msg_list_manager.h"
#include "settime.h"
#include "video.h"
#include "videoplay.h"
#include "wifi_management.h"

#include "collision/collision.h"
#include "example/user.h"
#include "fs_manage/fs_msg.h"
#include "fs_manage/fs_sdcard.h"
#include "fs_manage/fs_sdv.h"
#include "fs_manage/fs_storage.h"
#include "fwk_protocol/rk_fwk.h"
#include "fwk_protocol/rk_protocol.h"
#include "parking_monitor/parking_monitor.h"
#include "power/thermal.h"
#include "ueventmonitor/ueventmonitor.h"
#include "ueventmonitor/usb_sd_ctrl.h"
#include "storage/storage.h"
#include "gps/nmea_parse.h"
#include "licenseplate.h"
#include "public_interface.h"
#include "uvc/uvc_user.h"

#ifdef USE_ADAS
#include "adas/adas_process.h"
#endif
#include "gyrosensor.h"

#ifdef _PCBA_SERVICE_
#include "pcba/pcba_service.h"
#endif

#ifdef USE_WATCHDOG
static int fd_wtd;
#endif
static int pre_mode = MODE_NONE;
static int cur_mode = MODE_NONE;
static void (*ui_preview_enter_call)(void) = NULL;
static void (*ui_preview_exit_call)(void) = NULL;

static struct gps_info g_gps_info;

void api_send_msg(int id, int type, void *msg0, void *msg1)
{
    struct public_message msgdata;

    msgdata.id = id;
    msgdata.type = type;
    dispatch_msg(&msgdata, msg0, msg1);
}

int api_get_sd_mount_state(void)
{
    return get_sd_mount_state();
}

void api_set_sd_mount_state(int state)
{
    set_sd_mount_state(state);
}

int api_video_get_record_state(void)
{
    return video_record_get_state();
}

void (*storage_filesize_event_call)(int cmd, void *msg0, void *msg1);

int storage_setting_reg_event_callback(void (*call)(
                                           int cmd,
                                           void *msg0,
                                           void *msg1))
{
    storage_filesize_event_call = call;
    return 0;
}

void storage_setting_event_callback(int cmd, void *msg0, void *msg1)
{
    if (storage_filesize_event_call)
        return (*storage_filesize_event_call)(cmd, msg0, msg1);
}

void api_set_video_idc(unsigned int i)
{
    parameter_save_video_idc(i);
    api_send_msg(MSG_VIDEO_IDC, TYPE_BROADCAST, (void *)i, NULL);
}

void api_set_video_cvbsout(unsigned int i)
{
    int ret;

    switch (i) {
        /* no external */
    case OUT_DEVICE_LCD:
        if (rk_fb_get_cvbsout_connect() == 1) {
            if (!system("echo 0 > /sys/class/display/TV/enable"))
                printf("shutdown external display\n");
            else
                printf("failed to shutdown external display\n");
        }
        break;
        /* cvbsout */
    case OUT_DEVICE_CVBS_PAL:
        if (rk_fb_get_cvbsout_connect() == 1) {
            if (!system("echo 1 > /sys/class/display/TV/enable"))
                printf("switch to cvbsout display\n");
            else
                printf("failed to switch to cvbsout display\n");

            ret = system("echo 720x576i-50 > /sys/class/display/TV/mode");
            if (!ret)
                printf("switch to cvbsout PAL mode\n");
            else
                printf("failed to switch cvbsout display mode\n");
            rk_fb_set_out_device(OUT_DEVICE_CVBS_PAL);
        }
        break;
    case OUT_DEVICE_CVBS_NTSC:
        if (rk_fb_get_cvbsout_connect() == 1) {
            if (!system("echo 1 > /sys/class/display/TV/enable"))
                printf("switch to cvbsout display\n");
            else
                printf("failed to switch to cvbsout display\n");

            ret = system("echo 720x480i-60 > /sys/class/display/TV/mode");
            if (!ret)
                printf("switch to cvbsout NTSC mode\n");
            else
                printf("failed to switch cvbsout display mode\n");
            rk_fb_set_out_device(OUT_DEVICE_CVBS_NTSC);
        }
        break;
    }
    api_send_msg(MSG_VIDEO_CVBSOUT, TYPE_BROADCAST, (void *)i, NULL);
}

void api_set_video_flip(unsigned int i)
{
    parameter_save_video_flip(i);
    rk_fb_set_flip(i);
    api_send_msg(MSG_VIDEO_FLIP, TYPE_BROADCAST, (void *)i, NULL);
}

void api_set_video_time_lapse(unsigned int i)
{
    int video_record_state = api_video_get_record_state();

    if (i == 0) {
        if (parameter_get_video_lapse_state())
            parameter_save_video_lapse_state(false);

        /* if recording, force stop and set new timelapse,then restart. */
        if (video_record_state == VIDEO_STATE_RECORD)
            api_forced_stop_rec();

        if (parameter_get_time_lapse_interval() != 0) {
            parameter_save_time_lapse_interval(0);
            video_record_set_timelapseint(0);
        }

        api_send_msg(MSG_MODE_CHANGE_VIDEO_NORMAL_NOTIFY, TYPE_BROADCAST, NULL, NULL);

        if (video_record_state == VIDEO_STATE_RECORD)
            api_forced_start_rec();
    } else {
        if (!parameter_get_video_lapse_state())
            parameter_save_video_lapse_state(true);

        if (parameter_get_time_lapse_interval() != i) {
            /* if recording, force stop and set new timelapse,then restart. */
            if (video_record_state == VIDEO_STATE_RECORD)
                api_forced_stop_rec();

            parameter_save_time_lapse_interval(i);
            video_record_set_timelapseint(i);
            api_send_msg(MSG_MODE_CHANGE_TIME_LAPSE_NOTIFY, TYPE_BROADCAST, (void *)i, NULL);

            if (video_record_state == VIDEO_STATE_RECORD)
                api_forced_start_rec();
        }
    }
}

void api_set_video_autooff_screen(int i)
{
    parameter_save_screenoff_time(i);
    api_send_msg(MSG_VIDEO_AUTOOFF_SCREEN, TYPE_BROADCAST, (void *)i, NULL);
}

void api_set_video_license_plate(char *buf)
{
    parameter_save_licence_plate(buf, MAX_LICN_NUM);
    parameter_save_licence_plate_flag(1);

    api_send_msg(MSG_VIDEO_LICENSE_PLATE, TYPE_BROADCAST, (void *)buf, NULL);
}

void api_set_video_quaity(unsigned int qualiy_level)
{
    int video_record_state = api_video_get_record_state();

    if (qualiy_level == parameter_get_bit_rate_per_pixel())
        return;

    if (video_record_state == VIDEO_STATE_RECORD)
        api_forced_stop_rec();

    parameter_save_bit_rate_per_pixel(qualiy_level);
    video_record_reset_bitrate();
    storage_setting_event_callback(0, NULL, NULL);

    if (video_record_state == VIDEO_STATE_RECORD)
        api_forced_start_rec();

    api_send_msg(MSG_VIDEO_QUAITY, TYPE_BROADCAST, (void *)qualiy_level, NULL);
}

int api_video_set_time_lenght(int time)
{
    int video_record_state = api_video_get_record_state();

    if (video_record_state == VIDEO_STATE_RECORD)
        api_forced_stop_rec();

    parameter_save_recodetime(time);
    storage_setting_event_callback(0, NULL, NULL);

    if (video_record_state == VIDEO_STATE_RECORD)
        api_forced_start_rec();

    api_send_msg(MSG_VIDEO_TIME_LENGTH, TYPE_BROADCAST, (void *)time, NULL);
    return 0;
}

void api_mic_onoff(int state)
{
    parameter_save_video_audio(state);
    video_record_setaudio(state);
    api_send_msg(MSG_VIDEO_MIC_ONOFF, TYPE_BROADCAST, (void *)state, NULL);
}

void api_video_init(int mode)
{
    struct video_param *front = parameter_get_video_frontcamera_reso();
    struct video_param *back = parameter_get_video_backcamera_reso();
    struct video_param *cif = parameter_get_video_cifcamera_reso();
    struct photo_param *param = parameter_get_photo_param();
    struct video_param photo = {param->width, param->height, 30};

    video_record_setaudio(parameter_get_video_audio());

    if (mode == VIDEO_MODE_PHOTO) {
        cur_mode = MODE_PHOTO;
        video_record_set_record_mode(false);
        video_record_init(&photo, &photo, &photo);
    } else {
        cur_mode = MODE_RECORDING;
        video_record_set_record_mode(true);
        video_record_init(front, back, cif);
    }

    if (parameter_get_collision_level() != 0)
        video_record_start_cache(COLLISION_CACHE_DURATION);
}

void api_video_deinit(bool black)
{
    /*
     * FIXME: I do not think it is a good idea that
     * manage wifi related business in UI thread,
     * too trivial.
     */
    video_record_stop_ts_transfer(1);

    video_record_stop_cache();
    stop_motion_detection();
    api_forced_stop_rec();
    video_record_deinit(black);
}

int api_start_rec(void)
{
    int ret = -1;

    ret = set_motion_detection_trigger_value(true);
    if (!ret)
        return ret;

    if (video_record_get_state() == VIDEO_STATE_PREVIEW) {
        audio_sync_play("/usr/local/share/sounds/VideoRecord.wav");
        ret = video_record_startrec();
        api_send_msg(MSG_VIDEO_REC_START, TYPE_BROADCAST, NULL, NULL);
    }

    return ret;
}

void api_stop_rec(void)
{
    if (!set_motion_detection_trigger_value(true))
        return;

    if (video_record_get_state() == VIDEO_STATE_RECORD) {
        audio_sync_play("/usr/local/share/sounds/KeypressStandard.wav");
        video_record_stoprec();
        api_send_msg(MSG_VIDEO_REC_STOP, TYPE_BROADCAST, NULL, NULL);
    }
}

int api_forced_start_rec(void)
{
    int ret = -1;

    if (video_record_get_state() == VIDEO_STATE_PREVIEW) {
        audio_sync_play("/usr/local/share/sounds/VideoRecord.wav");
        ret = video_record_startrec();
        api_send_msg(MSG_VIDEO_REC_START, TYPE_BROADCAST, NULL, NULL);
    }

    return ret;
}

void api_forced_stop_rec(void)
{
    if (video_record_get_state() == VIDEO_STATE_RECORD) {
        audio_sync_play("/usr/local/share/sounds/KeypressStandard.wav");
        video_record_stoprec();
        api_send_msg(MSG_VIDEO_REC_STOP, TYPE_BROADCAST, NULL, NULL);
    }
}

int api_get_front_resolution(struct video_param* frame,
                             int count)
{
    return video_record_get_front_resolution(frame, count);
}

int api_get_back_resolution(struct video_param* frame,
                            int count)
{
    return video_record_get_back_resolution(frame, count);
}

int api_get_cif_resolution(struct video_param* frame,
                           int count)
{
    return video_record_get_cif_resolution(frame, count);
}

void api_set_front_rec_resolution(int id)
{
    int front_resos_max = parameter_get_front_cam_resolutions_max();
    const struct video_param *front_cam_reso = parameter_get_front_cam_resolutions();

    if (front_cam_reso == NULL)
        return;

    if (id >= 0 && id < front_resos_max) {
        printf("parameter_save_video_frontcamera_all:%d %d %d %d\n", id,
               front_cam_reso[id].width, front_cam_reso[id].height, front_cam_reso[id].fps);
        parameter_save_video_frontcamera_all(id, front_cam_reso[id]);

#ifdef CVR
        if ((front_cam_reso[id].width == 1280)
            && (front_cam_reso[id].height == 720)) {
            printf("parameter_save_vcamresolution(0)\n");
            parameter_save_vcamresolution(0);
        } else if ((front_cam_reso[id].width == 1920) &&
                   (front_cam_reso[id].height == 1080)) {
            printf("parameter_save_vcamresolution(1)\n");
            parameter_save_vcamresolution(1);
        }
#endif
    }

    if ((api_video_get_record_state() != VIDEO_STATE_STOP)
        && (video_record_get_record_mode() == true))
        api_video_reinit();

    api_send_msg(MSG_VIDEO_FRONT_REC_RESOLUTION, TYPE_BROADCAST, (void *)id, NULL);
}

void api_set_back_rec_resolution(int id)
{
    int back_resos_max = parameter_get_front_cam_resolutions_max();
    const struct video_param *back_cam_reso = parameter_get_back_cam_resolutions();

    if (back_cam_reso == NULL)
        return;

    if (id >= 0 && id < back_resos_max)
        parameter_save_video_backcamera_all(id, back_cam_reso[id]);

    if ((api_video_get_record_state() != VIDEO_STATE_STOP)
        && (video_record_get_record_mode() == true))
        api_video_reinit();

    api_send_msg(MSG_VIDEO_BACK_REC_RESOLUTION, TYPE_BROADCAST, (void *)id, NULL);
}

void api_set_cif_rec_resolution(int id)
{
    static struct video_param cifcamera[4];

    api_get_cif_resolution(cifcamera, 4);
    if (id >= 0 && id < 4)
        parameter_save_video_cifcamera_all(id, cifcamera[id]);

    if ((api_video_get_record_state() != VIDEO_STATE_STOP)
        && (video_record_get_record_mode() == true))
        api_video_reinit();

    api_send_msg(MSG_VIDEO_CIF_REC_RESOLUTION, TYPE_BROADCAST, (void *)id, NULL);
}

void api_set_white_balance(int level)
{
    if (api_video_get_record_state() != VIDEO_STATE_STOP)
        video_record_set_white_balance(level);

    parameter_save_wb(level);
    api_send_msg(MSG_SET_WHITE_BALANCE, TYPE_BROADCAST, (void *)level, NULL);
}

void api_set_exposure_compensation(int level)
{
    if (api_video_get_record_state() != VIDEO_STATE_STOP)
        video_record_set_exposure_compensation(level);

    parameter_save_ex(level);
    api_send_msg(MSG_SET_EXPOSURE, TYPE_BROADCAST, (void *)level, NULL);
}

void api_video_mark_onoff(int onoff)
{
    int video_record_state = api_video_get_record_state();

    if (video_record_state == VIDEO_STATE_RECORD)
        api_forced_stop_rec();

    parameter_save_video_mark(onoff);

#ifdef USE_WATERMARK
    video_record_update_osd_num_region(onoff);
#endif

    api_send_msg(MSG_VIDEO_MARK_ONOFF, TYPE_BROADCAST, (void *)onoff, NULL);

    if (video_record_state == VIDEO_STATE_RECORD)
        api_forced_start_rec();
}

int api_recovery(void)
{
    api_forced_stop_rec();
    parameter_recover();
    api_power_reboot();

    return 1;
}

void api_uvc_init(void)
{
#if USE_USB_WEBCAM
    if (parameter_get_video_usb() == USB_MODE_UVC)
        android_usb_config_uvc();
#endif
}

void api_deinit_uvc(void)
{
#if USE_USB_WEBCAM
    if (parameter_get_video_usb() == USB_MODE_UVC)
        uvc_gadget_pthread_exit();
#endif
}

void api_change_usb_mode(int mode)
{
    if (parameter_get_video_usb() == mode) {
        api_send_msg(MSG_SET_USB_MODE, TYPE_BROADCAST, (void *)mode, NULL);
        return;
    }

    switch (mode) {
    case USB_MODE_MSC:
        api_deinit_uvc();
        parameter_save_video_usb(mode);
        android_usb_config_ums();
        break;
    case USB_MODE_ADB:
        api_deinit_uvc();
        parameter_save_video_usb(mode);
        android_usb_config_adb();
        break;
    case USB_MODE_UVC:
        parameter_save_video_usb(mode);
        api_uvc_init();
        break;
    default:
        break;
    }

    api_send_msg(MSG_SET_USB_MODE, TYPE_BROADCAST, (void *)mode, NULL);
}

void api_set_lcd_backlight(int level)
{
    parameter_save_video_backlt(level);
    rk_fb_set_lcd_backlight(level);
    api_send_msg(MSG_SET_BACKLIGHT, TYPE_BROADCAST, (void *)level, NULL);
}

void api_set_lcd_pip(int status)
{
    parameter_save_video_pip(status);
    api_send_msg(MSG_SET_PIP, TYPE_BROADCAST, (void *)status, NULL);
}

void api_set_language(int language)
{
    parameter_save_video_lan(language);
    api_send_msg(MSG_SET_LANGUAGE, TYPE_BROADCAST, (void *)language, NULL);
}

void api_3dnr_onoff(int onoff)
{
    parameter_save_video_3dnr(onoff);
    api_send_msg(MSG_SET_3DNR, TYPE_BROADCAST, (void *)onoff, NULL);
}

void api_set_photo_resolution(const struct photo_param* param)
{
    parameter_save_photo_param(param);
    if ((video_record_get_record_mode() == false) &&
        (video_record_get_state() == VIDEO_STATE_PREVIEW)) {
        api_video_deinit(false);
        api_video_init(VIDEO_MODE_PHOTO);
    }
    api_send_msg(MSG_SET_PHOTO_RESOLUTION, TYPE_BROADCAST, (void *)param, NULL);
}

void api_video_reinit(void)
{
    int video_record_state = api_video_get_record_state();

    if (video_record_state != VIDEO_STATE_STOP) {
        if (video_record_get_record_mode() == false) {
            api_video_deinit(false);
            api_video_init(VIDEO_MODE_PHOTO);
        } else {
            api_video_deinit(false);
            api_video_init(VIDEO_MODE_REC);
            if (video_record_state == VIDEO_STATE_RECORD)
                api_forced_start_rec();
        }
    }
}

void api_video_reinit_flip(unsigned int i)
{
    int video_record_state = api_video_get_record_state();

    if (video_record_state != VIDEO_STATE_STOP) {
        if (video_record_get_record_mode() == false) {
            api_video_deinit(false);
            api_set_video_flip(i);
            api_video_init(VIDEO_MODE_PHOTO);
        } else {
            api_video_deinit(false);
            api_set_video_flip(i);
            api_video_init(VIDEO_MODE_REC);
            if (video_record_state == VIDEO_STATE_RECORD)
                api_forced_start_rec();
        }
    }
}

void api_set_abmode(int mode)
{
    parameter_save_abmode(mode);
    api_video_reinit();
    api_send_msg(MSG_VIDEO_SET_ABMODE, TYPE_BROADCAST, (void *)mode, NULL);
}

void api_motion_detection_onoff(int onoff)
{
    if (onoff == parameter_get_video_de())
        return;

    parameter_save_video_de(onoff);

    if (parameter_get_video_de() == 0)
        stop_motion_detection();
    else
        start_motion_detection();

    api_send_msg(MSG_VIDEO_SET_MOTION_DETE, TYPE_BROADCAST, (void *)onoff, NULL);
}

void api_motion_detection_start(void)
{
    start_motion_detection();
}

void api_motion_detection_stop(void)
{
    stop_motion_detection();
}

void api_set_collision_level(int level)
{
    parameter_save_collision_level(level);
    if (level == COLLI_CLOSE) {
        video_record_stop_cache();
        collision_unregister();
    } else {
        collision_register();
        video_record_start_cache(COLLISION_CACHE_DURATION);
    }
    api_send_msg(MSG_COLLISION_LEVEL, TYPE_BROADCAST, (void *)level, NULL);
}

void api_video_autorecode_onoff(int onoff)
{
    parameter_save_video_autorec(onoff);
    api_send_msg(MSG_VIDEO_AUTO_RECODE, TYPE_BROADCAST, (void *)onoff, NULL);
}

void api_leavecarrec_onoff(int onoff)
{
    parameter_save_parkingmonitor(onoff);
    if (onoff)
        parking_register();
    else
        parking_unregister();
    api_send_msg(MSG_LEAVECARREC_ONOFF, TYPE_BROADCAST, (void *)onoff, NULL);
}

int api_adas_init(int width, int height)
{
#ifdef USE_ADAS
    return AdasInit(width, height);
#else
    return 0;
#endif
}

void api_adas_deinit(void)
{
#ifdef USE_ADAS
    AdasDeinit();
#endif
}

void api_adas_set_wh(int width, int height)
{
#ifdef USE_DISP_TS
    AdasSetWH(width, height);
#endif
}

int api_adas_onoff(int onoff)
{
#ifdef USE_ADAS
    char setting[2];

    if (onoff) {
        if (AdasOn())
            return -1;
        api_video_reinit();
        parameter_get_video_adas_setting(setting);
        dpp_adas_set_calibrate_line(
            (int)(ADAS_FCW_HEIGHT * ((float)setting[0] / 100)),
            (int)(ADAS_FCW_HEIGHT * ((float)setting[1] / 100)));
    } else {
        AdasOff();
        api_video_reinit();
    }
    api_send_msg(MSG_VIDEO_ADAS_ONOFF, TYPE_BROADCAST, (void *)onoff, NULL);
#endif
    return 0;
}

void api_adas_get_parameter(void *parameter)
{
#ifdef USE_ADAS
    AdasGetParameter((struct adas_parameter *)parameter);
#endif
}

void api_adas_save_parameter(void *parameter)
{
#ifdef USE_ADAS
    AdasSaveParameter((struct adas_parameter *)parameter);
#endif
}

void api_set_video_freq(int freq)
{
    parameter_save_video_fre(freq);
    video_record_set_power_line_frequency(freq);
    api_send_msg(MSG_SET_FREQ, TYPE_BROADCAST, (void *)freq, NULL);
}

void api_set_system_time(int year, int mon, int mday, int hour,
                         int min, int sec)
{
    struct tm tm;

    tm.tm_year = year - 1900;
    tm.tm_mon = mon - 1;
    tm.tm_mday = mday;
    tm.tm_hour = hour;
    tm.tm_min = min;
    tm.tm_sec = sec;
    setDateTime(&tm);

    api_send_msg(MSG_SET_SYS_TIME, TYPE_BROADCAST, NULL, NULL);
}

char * api_get_firmware_version(void)
{
    return parameter_get_firmware_version();
}

extern int set_playback_volume(long volume);
void api_set_key_tone(int key_tone_volume)
{
    set_playback_volume(key_tone_volume);
    parameter_save_key_tone_volume(key_tone_volume);
    api_send_msg(MSG_SET_KEY_TONE, TYPE_BROADCAST, (void *)key_tone_volume, NULL);
}

void api_set_car_mode_onoff(int onoff)
{
    parameter_save_video_carmode(onoff);
    api_send_msg(MSG_SET_CAR_MODE_ONOFF, TYPE_BROADCAST, (void *)onoff, NULL);
}

void api_set_video_dvs_onoff(int onoff)
{
    parameter_save_dvs_enabled(onoff);
    video_dvs_enable(onoff);
    api_send_msg(MSG_SET_VIDEO_DVS_ONOFF, TYPE_BROADCAST, (void *)onoff, NULL);
}

int api_set_video_dvs_calibration(void)
{
    int ret = -1;
    api_send_msg(MSG_SET_VIDEO_DVS_CALIB, TYPE_BROADCAST, NULL, NULL);
    sleep(2);
    ret = gyrosensor_calibration(GYRO_DIRECT_CALIBRATION);
    api_send_msg(MSG_ACK_VIDEO_DVS_CALIB, TYPE_BROADCAST, (void *)ret, NULL);
    return ret;
}

void api_take_photo(int photo_num)
{
    if (api_get_sd_mount_state() == SD_STATE_IN)
        api_send_msg(MSG_TAKE_PHOTO_START, TYPE_BROADCAST, (void *)photo_num, NULL);
    else
        /*No sd card */
        api_send_msg(MSG_TAKE_PHOTO_WARNING, TYPE_BROADCAST, NULL, NULL);
}

void fsfile_event_callbak(int cmd, void *msg0, void *msg1)
{
    printf("%s, cmd = %d, file name = %s, msg1 = %d\n",
           __func__, cmd, (char*)msg0, (int)msg1);
    FILE_NOTIFY notify_type = cmd;
    char *filename = (char *)msg0;

    switch (notify_type) {
    case FILE_OVERLONG:
        video_record_switch_next_recfile(filename);
        break;
    case FILE_END:
        /* If file close and stop now, need clear cache */
        if (video_record_get_state() != VIDEO_STATE_RECORD)
            sync();
    case FILE_NEW:
    case FILE_DEL:
    case FILE_FULL:
        api_send_msg(MSG_FS_NOTIFY, TYPE_BROADCAST, msg0, (void *)cmd);
        break;
    default:
        break;
    }
}

void record_event_callback(int cmd, void *msg0, void *msg1)
{
    switch (cmd) {
    case CMD_UPDATETIME:
        api_send_msg(MSG_VIDEO_UPDATETIME, TYPE_BROADCAST, msg0, NULL);
        break;
        break;
    case CMD_PHOTOEND:
        api_send_msg(MSG_VIDEO_PHOTO_END, TYPE_BROADCAST, msg0, msg1);
        break;
    }
}

#ifdef USE_ADAS
static void adas_event_callback(int cmd, void *msg0, void *msg1)
{
    api_send_msg(MSG_ADAS_UPDATE, TYPE_BROADCAST, msg0, msg1);
}
#endif

void sd_event_callback(int cmd, void *msg0, void *msg1)
{
    printf("%s, %d, %d, %d\n", __func__, (int)cmd, (int)msg0, (int)msg1);

    if (cmd == SD_MOUNT_FAIL) {
        api_send_msg(MSG_SDCORD_MOUNT_FAIL, TYPE_BROADCAST, msg0, msg1);
        return;
    }

    if (cmd == SD_CHANGE) {
        if (api_get_sd_mount_state() == SD_STATE_IN) {
            int ret = fs_manage_init(NULL, (void*)NULL);

            if (ret < 0) {
                api_set_sd_mount_state(SD_STATE_ERR);
                video_record_stop_savecache();
                api_send_msg(MSG_FS_INITFAIL, TYPE_BROADCAST,
                             (void *)NULL, (void *)ret);
                return;
            }

            if (parameter_get_video_de() == 1)
                api_motion_detection_start();
            /* Set the filename last time */
            storage_check_timestamp();
            video_record_get_user_noise();
        } else {
            api_motion_detection_stop();
            video_record_stop_savecache();
            api_stop_rec();
            videoplay_exit();
            fs_manage_deinit();
        }

        api_send_msg(MSG_SDCORD_CHANGE, TYPE_BROADCAST, msg0, msg1);
    }
}

void hdmi_event_callback(int cmd, void *msg0, void *msg1)
{
    //printf("%s cmd = %d, msg0 = %d, msg1 = %d\n", __func__, cmd, msg0, msg1);
    api_send_msg(MSG_HDMI_EVENT, TYPE_BROADCAST, msg0, msg1);
}

void cvbsout_event_callback(int cmd, void *msg0, void *msg1)
{
    api_send_msg(MSG_CVBSOUT_EVENT, TYPE_BROADCAST, msg0, msg1);
}

void camere_event_callback(int cmd, void *msg0, void *msg1)
{
    api_send_msg(MSG_CAMERE_EVENT, TYPE_BROADCAST, msg0, msg1);
}

void videoplay_event_callback(int cmd, void *msg0, void *msg1)
{
    printf("%s cmd = %d, msg0 = %d, msg1 = %d\n",
           __func__, (int)cmd, (int)msg0, (int)msg1);
    switch (cmd) {
    case CMD_VIDEOPLAY_EXIT:
        api_send_msg(MSG_VIDEOPLAY_EXIT, TYPE_BROADCAST, msg0, msg1);
        break;
    case CMD_VIDEOPLAY_UPDATETIME:
        api_send_msg(MSG_VIDEOPLAY_UPDATETIME, TYPE_BROADCAST, msg0, msg1);
        break;
    }
}

void user_event_callback(int cmd, void *msg0, void *msg1)
{
    printf("%s cmd = %d, msg0 = %d, msg1 = %d\n",
           __func__, (int)cmd, (int)msg0, (int)msg1);
    switch (cmd) {
    case CMD_USER_RECORD_RATE_CHANGE:
        if (!(bool)msg1 && (api_video_get_record_state() == VIDEO_STATE_PREVIEW) && api_get_sd_mount_state() == SD_STATE_IN) {
            api_forced_start_rec();
            video_record_rate_change((void *)msg0, (int)msg1);
        } else if (msg1 && (api_video_get_record_state() == VIDEO_STATE_RECORD)) {
            video_record_rate_change((void *)msg0, (int)msg1);
            api_forced_stop_rec();
        } else {
            video_record_rate_change((void *)msg0, (int)msg1);
        }
        //api_send_msg(MSG_USER_RECORD_RATE_CHANGE, TYPE_BROADCAST, msg0, msg1);
        break;
    case CMD_USER_MDPROCESSOR:
        video_record_attach_user_mdprocessor(msg0, msg1);
        //api_send_msg(MSG_USER_MDPROCESSOR, TYPE_BROADCAST, msg0, msg1);
        break;
    case CMD_USER_MUXER:
        video_record_attach_user_muxer(msg0, msg1, 1);
        //api_send_msg(MSG_USER_MUXER, TYPE_BROADCAST, msg0, msg1);
        break;
    }
}

void gpios_event_callback(int cmd, void *msg0, void *msg1)
{
    api_send_msg(MSG_GPIOS_EVENT, TYPE_BROADCAST, msg0, msg1);
}

void usb_event_callback(int cmd, void *msg0, void *msg1)
{
    api_send_msg(MSG_USB_EVENT, TYPE_BROADCAST, msg0, msg1);
}

void batt_event_callback(int cmd, void *msg0, void *msg1)
{
    switch (cmd) {
    case CMD_UPDATE_CAP:
        api_send_msg(MSG_BATT_UPDATE_CAP, TYPE_BROADCAST, msg0, msg1);
        break;
    case CMD_LOW_BATTERY:
        api_send_msg(MSG_BATT_LOW, TYPE_BROADCAST, msg0, msg1);
        break;
    case CMD_DISCHARGE:
        api_send_msg(MSG_BATT_DISCHARGE, TYPE_BROADCAST, msg0, msg1);
        break;
    default:
        break;
    }
}

static int sdcard_format_notifyback(void* arg, int param)
{
    if (param >= 0) {
        api_set_sd_mount_state(SD_STATE_IN);
        sd_event_callback(SD_CHANGE, (void *)NULL, (void *)SD_STATE_FORMATING);
    } else {
        api_set_sd_mount_state(SD_STATE_ERR);
    }

    api_send_msg(MSG_SDCARDFORMAT_NOTIFY, TYPE_BROADCAST, (void *)param, NULL);

    return 0;
}

/*
 * type:
 * 0, normal: it only mkfs.vfat /dev/mmcblk0p1;
 * 1, special: mkfs.vfat /dev/mmcblk0 &&
 *             fdisk -t /dev/mmcblk0 &&
 *             mkfs.vfat /dev/mmcblk0p1
 */
void api_sdcard_format(int type)
{
    if (api_get_sd_mount_state() == SD_STATE_OUT)
        return;

    api_forced_stop_rec();
    api_set_sd_mount_state(SD_STATE_FORMATING);
    api_send_msg(MSG_SDCARDFORMAT_START, TYPE_BROADCAST, NULL, NULL);
    fs_manage_format_sdcard(sdcard_format_notifyback, 0, type);
}

void api_watchdog_keep_alive(void)
{
#ifdef USE_WATCHDOG
    if (fd_wtd != -1)
        ioctl(fd_wtd, WDIOC_KEEPALIVE, 0);
#endif
}

/* callback when all videos is ready */
static void notify_videos_inited(int is_record_mode)
{
    int mode = is_record_mode ? MODE_RECORDING : MODE_PHOTO;
    api_send_msg(MSG_MODE_CHANGE_NOTIFY, TYPE_WIFI, (void *)pre_mode, (void *)mode);
}

int gdb_usb_debug = 0;

void api_poweron_init(fun_cb cb)
{
#ifdef USE_WATCHDOG
    char pathname[] = "/dev/watchdog";

    printf ("use watchdog\n");
    fd_wtd = open(pathname, O_WRONLY);
    if (fd_wtd == -1)
        printf ("/dev/watchdog open error\n");
    else {
        ioctl(fd_wtd, WDIOC_KEEPALIVE, 0);
    }
#endif
#ifdef DEBUG
    /* if /tmp/RV_USB_STATE_MASK_DEBUG is exist, do not care USB event */
    gdb_usb_debug = !access("/tmp/RV_USB_STATE_MASK_DEBUG", F_OK);
#endif
    if (!gdb_usb_debug)
        android_usb_init();
    parameter_init();
    video_record_init_lock(); //must before uevent init

    /* create a link list for ui msg dispatch */
    if (init_list() && cb)
        reg_entry(cb);

    thermal_init();

    gpios_reg_event_callback(gpios_event_callback);
    usb_reg_event_callback(usb_event_callback);
    batt_reg_event_callback(batt_event_callback);
    uevent_monitor_run();

    REC_RegEventCallback(record_event_callback);
#ifdef USE_ADAS
    AdasEventRegCallback(adas_event_callback);
#endif
    set_video_init_complete_cb(notify_videos_inited);
    sd_reg_event_callback(sd_event_callback);
    hdmi_reg_event_callback(hdmi_event_callback);
    cvbsout_reg_event_callback(cvbsout_event_callback);
    camera_reg_event_callback(camere_event_callback);
    USER_RegEventCallback(user_event_callback);
    VIDEOPLAY_RegEventCallback(videoplay_event_callback);
    fs_msg_file_reg_callback(fsfile_event_callbak);

#ifdef _PCBA_SERVICE_
    pcba_service_start();
#else
    if (parameter_get_wifi_en() == 1)
        wifi_management_start();
#endif
    if (!gdb_usb_debug && parameter_get_video_usb() == USB_MODE_RNDIS)
        rndis_management_start();

    api_uvc_init();

    /*
     * If file size have been changed, need call the callball to nofity fs_manage
     * to change storage policy. Such as recordtime, resolution,
     * bit_rate_per_pixel, all they will effect file size.
     */
    storage_setting_reg_event_callback(storage_setting_callback);
    storage_setting_event_callback(0, NULL, NULL);

#ifdef MSG_FWK
    rk_fwk_glue_init();
    rk_fwk_controller_init();
#endif
#ifdef PROTOCOL_GB28181
    protocol_rk_gb28181_init();
#endif
#ifdef PROTOCOL_IOTC
    protocol_rk_iotc_init();
#endif
}

void api_poweroff_deinit(void)
{
#ifdef USE_WATCHDOG
    //printf ("close watchdog\n");
    //sleep(2);
    //if (fd_wtd != -1) {
    //write(fd_wtd, "V", 1);
    //close(fd_wtd);
    //}
#endif

#ifdef PROTOCOL_IOTC
    protocol_rk_iotc_destroy();
#endif
#ifdef PROTOCOL_GB28181
    protocol_rk_gb28181_destroy();
#endif
#ifdef MSG_FWK
    rk_fwk_controller_destroy();
    rk_fwk_glue_destroy();
#endif

    api_deinit_uvc();

#ifdef _PCBA_SERVICE_
    pcba_service_stop();
#else
    wifi_management_stop();
#endif
    api_video_deinit(true);
    videoplay_deinit();

    collision_unregister();
    parking_unregister();
    if (parameter_get_parkingmonitor()) {
        gsensor_enable(1);
        gsensor_use_interrupt(GSENSOR_INT2, GSENSOR_INT_START);
    }
    gsensor_release();

    video_record_destroy_lock(); //must after uevent deinit
    fs_manage_deinit();
}

void api_reg_ui_preview_callback(void (*enter_call)(void), void (*exit_call)(void))
{
    ui_preview_enter_call = enter_call;
    ui_preview_exit_call = exit_call;
}

void api_ui_preview_enter(void)
{
    if (ui_preview_enter_call)
        ui_preview_enter_call();
}

void api_ui_preview_exit(void)
{
    if (ui_preview_exit_call)
        ui_preview_exit_call();
}

int api_get_mode(void)
{
    return cur_mode;
}

void api_change_mode(int mode)
{
    int old_mode = cur_mode;
    int do_init_video = 0;

    if (old_mode == mode)
        return;

    pre_mode = old_mode;
    switch (old_mode) {
    case MODE_PHOTO:
    case MODE_RECORDING:
        if (mode == MODE_USBDIALOG || mode == MODE_CHARGING) {
            api_video_deinit(true);
        } else if (mode == MODE_PREVIEW) {
            api_video_deinit(true);
            api_ui_preview_enter();
        } else if (mode == MODE_RECORDING) {
            api_video_deinit(false);
            api_video_init(VIDEO_MODE_REC);
            do_init_video = 1;
        } else if (mode == MODE_PHOTO) {
            api_video_deinit(false);
            api_video_init(VIDEO_MODE_PHOTO);
            do_init_video = 1;
        } else if (mode == MODE_SUSPEND) {
            api_video_deinit(true);
            video_record_deinit(true);
            rk_fb_screen_off();
            fs_manage_sdcard_unmount();
            runapp("echo mem > /sys/power/state");
            rk_fb_screen_on();
            if (old_mode != MODE_PHOTO) {
                api_video_init(VIDEO_MODE_REC);
            } else {
                api_video_init(VIDEO_MODE_PHOTO);
            }
            do_init_video = 1;
        }
        break;
    case MODE_PLAY:
        if (mode == MODE_USBDIALOG || mode == MODE_CHARGING) {
            videoplay_exit();
            api_ui_preview_exit();
        } else if (mode == MODE_PREVIEW) {
        } else if (mode == MODE_SUSPEND) {
            videoplay_exit();
            api_ui_preview_exit();
            rk_fb_screen_off();
            fs_manage_sdcard_unmount();
            runapp("echo mem > /sys/power/state");
            rk_fb_screen_on();
        }
        break;
    case MODE_NONE:
    case MODE_USBDIALOG:
        if (mode == MODE_RECORDING) {
            api_video_init(VIDEO_MODE_REC);
            do_init_video = 1;
        } else if (mode == MODE_PHOTO) {
            api_video_init(VIDEO_MODE_PHOTO);
            do_init_video = 1;
        }
        break;
    case MODE_USBCONNECTION:
        if (mode == MODE_RECORDING) {
            api_video_init(VIDEO_MODE_REC);
            do_init_video = 1;
        } else if (mode == MODE_PHOTO) {
            api_video_init(VIDEO_MODE_PHOTO);
            do_init_video = 1;
        }
        break;
    case MODE_PREVIEW:
        if (mode == MODE_CHARGING) {
            api_ui_preview_exit();
        } else if (mode == MODE_PLAY) {
        } else if (mode == MODE_RECORDING) {
            api_ui_preview_exit();
            api_video_init(VIDEO_MODE_REC);
            do_init_video = 1;
        } else if (mode == MODE_PHOTO) {
            api_ui_preview_exit();
            api_video_init(VIDEO_MODE_PHOTO);
            do_init_video = 1;
        } else if (mode == MODE_SUSPEND) {
            fs_manage_deinit();
            fs_manage_sdcard_unmount();
            api_ui_preview_exit();
            rk_fb_screen_off();
            runapp("echo mem > /sys/power/state");
            rk_fb_screen_on();
            api_ui_preview_enter();
        }
        break;
    case MODE_CHARGING:
        if (mode == MODE_RECORDING) {
            api_video_init(VIDEO_MODE_REC);
            do_init_video = 1;
        } else if (mode == MODE_PHOTO) {
            api_video_init(VIDEO_MODE_PHOTO);
            do_init_video = 1;
        }
        break;
    }

    if (do_init_video)
        api_send_msg(MSG_MODE_CHANGE_NOTIFY, TYPE_LOCAL, (void *)old_mode,
                     (void *)mode);
    else
        api_send_msg(MSG_MODE_CHANGE_NOTIFY, TYPE_BROADCAST, (void *)old_mode,
                     (void *)mode);

    if (mode != MODE_SUSPEND)
        cur_mode = mode;
    else if (cur_mode == MODE_PLAY)
        cur_mode = MODE_PREVIEW;
}

int api_get_photo_burst_num(void)
{
    return parameter_get_photo_burst_num();
}

int api_set_photo_burst_num(int num)
{
    if (num == 0) {
        if (parameter_get_photo_burst_state())
            parameter_save_photo_burst_state(false);

        api_send_msg(MSG_MODE_CHANGE_PHOTO_NORMAL_NOTIFY, TYPE_BROADCAST, NULL, NULL);
    } else {
        if (!parameter_get_photo_burst_state())
            parameter_save_photo_burst_state(true);

        if (parameter_get_photo_burst_num() != num)
            parameter_save_photo_burst_num(num);

        api_send_msg(MSG_MODE_CHANGE_PHOTO_BURST_NOTIFY, TYPE_BROADCAST, (void *)num, NULL);
    }
    return 0;
}

int api_get_dvs(void)
{
    return parameter_get_dvs_enabled();
}

int api_set_dvs(int op)
{
    return parameter_save_dvs_enabled(op);
}

static void gps_send_callback(const char* gpsData, int len, int searchType)
{
    static bool update_system_time = false;

    if (gpsData == NULL)
        return;

    gps_nmea_data_parse(&g_gps_info, gpsData, searchType, len);

    if (!update_system_time && (g_gps_info.status == 'A')) {
        update_system_time = true;

        api_set_system_time(g_gps_info.date.years, g_gps_info.date.months,
                            g_gps_info.date.day, g_gps_info.date.hour,
                            g_gps_info.date.min, g_gps_info.date.sec);
    }

#ifdef USE_ADAS
    AdasGpsUpdate((void *)&g_gps_info);
#endif
    api_send_msg(MSG_GPS_INFO, TYPE_BROADCAST, (void *)&g_gps_info, NULL);
    api_send_msg(MSG_GPS_RAW_INFO, TYPE_BROADCAST, (void *)gpsData, NULL);
}

int api_gps_init(void)
{
    return gps_init(gps_send_callback);
}

void api_gps_deinit(void)
{
    gps_deinit();
}

int api_power_reboot(void)
{
    printf("reboot now\n");
    api_poweroff_deinit();
    runapp("busybox reboot");
    exit(0);
}

#ifdef _PCBA_SERVICE_
int api_key_event_notice(int cmd, int msg0)
{
    api_send_msg(MSG_KEY_EVENT_INFO, TYPE_BROADCAST, (void *)cmd, (void *)msg0);
}
#endif

int api_power_shutdown(void)
{
    printf("shutdown now\n");

    api_poweroff_deinit();
    api_send_msg(MSG_POWEROFF, TYPE_BROADCAST, NULL, NULL);
    runapp("busybox poweroff");
    exit(0);
}

int api_check_firmware(void)
{
    int ret;

    ret = access(FW_DEFAULT_MOUNT_FULLNAME, 0);
    if (ret) {
        printf("Not found firmware: " FW_DEFAULT_MOUNT_FULLNAME "\n");
        ret = FW_NOTFOUND;
        goto err;
    }

    /* TODO: verify firmware */
    return FW_OK;

err:
    return ret;
}

void api_dvs_onoff(bool onoff)
{
    api_send_msg(MSG_VIDEO_DVS_ONOFF, TYPE_BROADCAST, (void *)onoff, NULL);
}

struct video_param *api_get_video_frontcamera_resolution(void)
{
    return parameter_get_video_frontcamera_reso();
}

void api_set_video_frontcamera_resolution(struct video_param *video_reso)
{
    if (video_reso != NULL) {
        parameter_save_video_frontcamera_reso(video_reso);
    }
}

int api_get_photo_resolution_max(void)
{
    return parameter_get_photo_resolutions_max();
}

const struct photo_param *api_get_photo_resolutions(void)
{
    return parameter_get_photo_resolutions();
}

int api_set_photo_resolutions(struct photo_param *photo_resos)
{
    if (photo_resos != NULL) {
        return parameter_save_photo_resolutions(photo_resos);
    }

    return -1;
}

int api_get_key_level(void)
{
    int key_tone_vol = parameter_get_key_tone_volume();

    switch (key_tone_vol) {
    case KEY_TONE_MUT:
        return 0;

    case KEY_TONE_LOW:
        return 1;

    case KEY_TONE_MID:
        return 2;

    case KEY_TONE_HIG:
        return 3;
    }

    return 3;
}

void api_set_key_volume(unsigned int key_tone_level)
{
    switch (key_tone_level) {
    case 0:
        api_set_key_tone(KEY_TONE_MUT);
        break;

    case 1:
        api_set_key_tone(KEY_TONE_LOW);
        break;

    case 2:
        api_set_key_tone(KEY_TONE_MID);
        break;

    case 3:
        api_set_key_tone(KEY_TONE_HIG);
        break;

    default:
        break;
    }
}

