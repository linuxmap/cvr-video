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

#include "wifi_setting_interface.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <errno.h>

#include "common.h"
#include "parameter.h"
#include "public_interface.h"
#include "menu_commond.h"
#include "wifi_management.h"
#include "wlan_service_client.h"
#include "fs_manage/fs_storage.h"
#include "fs_manage/fs_msg.h"
#include "public_interface.h"
#include "ueventmonitor/usb_sd_ctrl.h"

#define STR_ARGSETTING          "CMD_ARGSETTING"

int setting_get_cmd_resolve(int nfp, char *buffer)
{
    char buff[1024] = {0};
    int arg;
    time_t t;
    struct tm *tm;
    char tbuf[128];
    char **plicence_plate;
    char licence_plate[8][3];
    int i;

    strcat(buff, "CMD_ACK_GET_ARGSETTING");
    strcat(buff, "Videolength:");
    arg = parameter_get_recodetime();
    if (arg == 60)
        strcat(buff, "1min");
    else if (arg == 180)
        strcat(buff, "3min");
    else if (arg == 300)
        strcat(buff, "5min");
    else if (arg == -1)
        strcat(buff, "infinity");

    strcat(buff, "RecordMode:");
    arg = parameter_get_abmode();
    if (arg == 0)
        strcat(buff, "Front");
    else if (arg == 1)
        strcat(buff, "Rear");
    else if (arg == 2)
        strcat(buff, "Double");

    strcat(buff, "3DNR:");
    if (parameter_get_video_3dnr())
        strcat(buff, "ON");
    else
        strcat(buff, "OFF");

    strcat(buff, "ADAS:");
    arg = parameter_get_video_adas();
    if (arg == 1)
        strcat(buff, "ON");
    else
        strcat(buff, "OFF");

    strcat(buff, "WhiteBalance:");
    arg = parameter_get_wb();
    if (arg == 0)
        strcat(buff, "auto");
    else if (arg == 1)
        strcat(buff, "Daylight");
    else if (arg == 2)
        strcat(buff, "fluocrescence");
    else if (arg == 3)
        strcat(buff, "cloudysky");
    else if (arg == 4)
        strcat(buff, "tungsten");

    strcat(buff, "Exposure:");
    arg = parameter_get_ex();
    if (arg == 0)
        strcat(buff, "-3");
    else if (arg == 1)
        strcat(buff, "-2");
    else if (arg == 2)
        strcat(buff, "-1");
    else if (arg == 3)
        strcat(buff, "0");
    else if (arg == 4)
        strcat(buff, "1");

    strcat(buff, "MotionDetection:");
    arg = parameter_get_video_de();
    if (arg == 1)
        strcat(buff, "ON");
    else if (arg == 0)
        strcat(buff, "OFF");

    strcat(buff, "DataStamp:");
    arg = parameter_get_video_mark();
    if (arg == 1)
        strcat(buff, "ON");
    else if (arg == 0)
        strcat(buff, "OFF");

    strcat(buff, "RecordAudio:");
    arg = parameter_get_video_audio();
    if (arg == 1)
        strcat(buff, "ON");
    else if (arg == 0)
        strcat(buff, "OFF");

    strcat(buff, "BootRecord:");
    arg = parameter_get_video_autorec();
    if (arg == 1)
        strcat(buff, "ON");
    else if (arg == 0)
        strcat(buff, "OFF");

    strcat(buff, "Language:");
    arg = parameter_get_video_lan();
    if (arg == 0)
        strcat(buff, "English");
    else if (arg == 1)
        strcat(buff, "Chinese");

    strcat(buff, "Frequency:");
    arg = parameter_get_video_fre();
    if (arg == CAMARE_FREQ_50HZ)
        strcat(buff, "50HZ");
    else if (arg == CAMARE_FREQ_60HZ)
        strcat(buff, "60HZ");

    strcat(buff, "Bright:");
    arg = parameter_get_video_backlt();
    if (arg == LCD_BACKLT_L)
        strcat(buff, "low");
    else if (arg == LCD_BACKLT_M)
        strcat(buff, "middle");
    else if (arg == LCD_BACKLT_H)
        strcat(buff, "hight");

    strcat(buff, "IDC:");
    arg = parameter_get_video_idc();
    if (arg == 1)
        strcat(buff, "ON");
    else
        strcat(buff, "OFF");

    strcat(buff, "FLIP:");
    arg = parameter_get_video_flip();
    if (arg == 1)
        strcat(buff, "ON");
    else
        strcat(buff, "OFF");

    strcat(buff, "TIME_LAPSE:");
    arg = parameter_get_time_lapse_interval();
    if (arg == 0)
        strcat(buff, "OFF");
    else if (arg == 1)
        strcat(buff, "1");
    else if (arg == 5)
        strcat(buff, "5");
    else if (arg == 10)
        strcat(buff, "10");
    else if (arg == 30)
        strcat(buff, "30");
    else if (arg == 60)
        strcat(buff, "60");

    strcat(buff, "AUTOOFF_SCREEN:");
    arg = parameter_get_screenoff_time();
    if (arg != -1)
        strcat(buff, "ON");
    else
        strcat(buff, "OFF");

    strcat(buff, "QUAITY:");
    arg = parameter_get_bit_rate_per_pixel();
    if (arg == VIDEO_QUALITY_HIGH)
        strcat(buff, "HIGH");
    else if (arg == VIDEO_QUALITY_MID)
        strcat(buff, "MID");
    else if (arg == VIDEO_QUALITY_LOW)
        strcat(buff, "LOW");

    strcat(buff, "Collision:");
    arg = parameter_get_collision_level();
    if (arg == COLLI_CLOSE)
        strcat(buff, "CLOSE");
    else if (arg == COLLI_LEVEL_L)
        strcat(buff, "L");
    else if (arg == COLLI_LEVEL_M)
        strcat(buff, "M");
    else if (arg == COLLI_LEVEL_H)
        strcat(buff, "H");

    strcat(buff, "LICENCE_PLATE:");
    if (parameter_get_licence_plate_flag()) {
        plicence_plate = parameter_get_licence_plate();
        memcpy(licence_plate, plicence_plate, sizeof(licence_plate));
        for (i = 0; i < 8; i++)
            strcat(buff, licence_plate[i]);
    } else
        strcat(buff, "NULL");

    strcat(buff, "PIP:");
    arg = parameter_get_video_pip();
    if (arg == 1)
        strcat(buff, "ON");
    else if (arg == 0)
        strcat(buff, "OFF");

    strcat(buff, "DVS:");
    arg = parameter_get_dvs_enabled();
    if (arg == 1)
        strcat(buff, "ON");
    else if (arg == 0)
        strcat(buff, "OFF");

    arg = parameter_get_key_tone_volume();
    snprintf(tbuf, sizeof(tbuf), "KeyTone:%d", arg);
    strcat(buff, tbuf);

    time(&t);
    tm = localtime(&t);
    snprintf(tbuf, sizeof(tbuf), "Time:%04d-%02d-%02d %02d:%02d:%02d",
             tm->tm_year + 1900,
             tm->tm_mon + 1,
             tm->tm_mday,
             tm->tm_hour,
             tm->tm_min,
             tm->tm_sec);
    strcat(buff, tbuf);

    strcat(buff, "Version:");
    strcat(buff, parameter_get_firmware_version());

    printf("cmd = %s\n", buff);
    if (0 > write(nfp, buff, strlen(buff)))
        printf("write fail <%d>!\r\n",errno);

    return 0;
}

int tcp_func_get_on_off_recording(int nfp, char *buffer)
{
    printf("CMD_GET_Control_Recording\n");

    if (get_src_state() != 2) {
        if (0 > write(nfp, "CMD_ACK_GET_Control_Recording_IDLE",
                      sizeof("CMD_ACK_GET_Control_Recording_IDLE")))
            printf("write fail!\r\n");
    } else {
        if (0 > write(nfp, "CMD_ACK_GET_Control_Recording_BUSY",
                      sizeof("CMD_ACK_GET_Control_Recording_BUSY")))
            printf("write fail!\r\n");
    }
    return 0;
}

int tcp_func_get_ssid_pw(int nfp, char *buffer)
{
    char wifi_info[100] = "CMD_WIFI_INFO";
    char ssid[33], passwd[65];

    parameter_getwifiinfo(ssid, passwd);

    strcat(wifi_info, "WIFINAME:");
    strcat(wifi_info, ssid);
    strcat(wifi_info, "WIFIPASSWORD:");
    strcat(wifi_info, passwd);
    strcat(wifi_info, "MODE:");
    if (parameter_get_wifi_mode() == 0)
        strcat(wifi_info, "AP");
    else
        strcat(wifi_info, "STA");
    strcat(wifi_info, "END");
    if (0 > write(nfp, wifi_info, strlen(wifi_info)))
        printf("write fail!\r\n");

    return 0;
}

int tcp_func_get_back_camera_apesplution(int nfp, char *buffer)
{
    char msg[128] = {0};
    char str[128] = {0};
    int i, max;
    const struct video_param * param = NULL;

    max = parameter_get_back_cam_resolutions_max();
    param = parameter_get_back_cam_resolutions();
    strcat(msg, "CMD_ACK_BACK_CAMERARESPLUTION:");

    for (i = 0; i < max; i++) {
        memset(str, 0, sizeof(str));
        if (param[i].width != 0 && param[i].height != 0) {
            snprintf(str, sizeof(str), "%d*%d %dFPS:", param[i].width, param[i].height, param[i].fps);
            strcat(msg, str);
        }
    }
    if (0 > write(nfp, msg, strlen(msg)))
        printf("write fail!\r\n");
    return 0;
}

int tcp_func_get_setting_photo_quality(int nfp, char *buffer)
{
    char buff[128] = {0};

    int index = parameter_get_photo_num();
    snprintf(buff, sizeof(buff), "CMD_CB_Photo_Quality:%d", index + 1);
    if (write(nfp, buff, strlen(buff)) < 0)
        printf("write fail! <%d> \r\n",errno);

    return 0;
}

int tcp_func_get_photo_quality(int nfp, char *buffer)
{
    char msg[128] = {0};
    char str[128] = {0};
    const struct photo_param *p;
    int max;
    int i;

    max = parameter_get_photo_resolutions_max();
    p = parameter_get_photo_resolutions();
    if (p == NULL)
        return -1;

    strcat(msg, "CMD_ACK_GET_PHOTO_QUALITY:");
    for (i = 0; i < max; i++) {
        memset(str, 0, sizeof(str));
        if (p[i].width != 0 && p[i].height != 0) {
            snprintf(str, sizeof(str), "%d*%d:", p[i].width,
                     p[i].height);
            strcat(msg, str);
        }
    }

    printf("quality= %s\n", msg);
    if (0 > write(nfp, msg, strlen(msg)))
        printf("write fail!\r\n");
    return 0;
}

int tcp_func_get_front_camera_apesplution(int nfp, char *buffer)
{
    char msg[128] = {0};
    char str[128] = {0};
    int i, max;
    const struct video_param * param = NULL;

    max = parameter_get_front_cam_resolutions_max();
    param = parameter_get_front_cam_resolutions();
    strcat(msg, "CMD_ACK_FRONT_CAMERARESPLUTION:");

    for (i = 0; i < max; i++) {
        memset(str, 0, sizeof(str));
        if (param[i].width != 0 && param[i].height != 0) {
            snprintf(str, sizeof(str), "%d*%d %dFPS:", param[i].width, param[i].height, param[i].fps);
            strcat(msg, str);
        }
    }
    if (0 > write(nfp, msg, strlen(msg)))
        printf("write fail!\r\n");

    return 0;
}

int tcp_func_get_front_setting_apesplution(int nfp, char * buffer)
{
    char buff[128] = {0};

    int video_frontcamera = parameter_get_video_fontcamera();
    sprintf(buff, "CMD_CB_FrontCamera:front%d", (int)video_frontcamera + 1);
    if (write(nfp, buff, strlen(buff)) < 0)
        printf("write fail!\r\n");

    return 0;
}

int tcp_func_get_back_setting_apesplution(int nfp, char * buffer)
{
    char buff[128] = {0};

    int video_backcamera = parameter_get_video_backcamera();
    sprintf(buff, "CMD_CB_BackCamera:back%d", (int)video_backcamera + 1);
    if (write(nfp, buff, strlen(buff)) < 0)
        printf("write fail!\r\n");

    return 0;
}

int tcp_func_get_format_status(int nfp, char *buffer)
{
    if (get_format_status() == SD_STATE_FORMATING) {
        if (0 > write(nfp, "CMD_GET_ACK_FORMAT_STATUS_BUSY",
                      sizeof("CMD_GET_ACK_FORMAT_STATUS_BUSY")))
            printf("write fail!\r\n");
    } else if (get_format_status() == SD_STATE_IN) {
        if (0 > write(nfp, "CMD_GET_ACK_FORMAT_STATUS_IDLE",
                      sizeof("CMD_GET_ACK_FORMAT_STATUS_IDLE")))
            printf("write fail!\r\n");
    } else if (get_format_status() == SD_STATE_OUT || get_format_status() == SD_STATE_ERR) {
        if (0 > write(nfp, "CMD_GET_ACK_FORMAT_STATUS_ERR",
                      sizeof("CMD_GET_ACK_FORMAT_STATUS_ERR")))
            printf("write fail!\r\n");
    }

    return 0;
}

static int setting_func_front_camera(int nfp, char *str)
{
    return setting_func_cammp_ui(0, str);
}

static int setting_func_back_camera(int nfp, char *str)
{
    return setting_func_cammp_ui(1, str);
}

static int setting_func_video_length(int nfp, char *str)
{
    return setting_func_video_length_ui(str);
}

static int setting_func_record_mode(int nfp, char *str)
{
    return setting_func_car_ui(str);
}

static int setting_func_3dnr(int nfp, char *str)
{
    return setting_func_3dnr_ui(str);
}

static int setting_func_adas(int nfp, char *str)
{
    return setting_func_adas_ui(str);
}

static int setting_func_white_balance(int nfd, char *str)
{
    return setting_func_white_balance_ui(str);
}

static int setting_func_exposure(int nfd, char *str)
{
    return setting_func_exposure_ui(str);
}

static int setting_func_motion_detection(int nfd, char *str)
{
    return setting_func_motion_detection_ui(str);
}

static int setting_func_data_stamp(int nfd, char *str)
{
    return setting_func_data_stamp_ui(str);
}

static int setting_func_record_audio(int nfd, char *str)
{
    return setting_func_record_audio_ui(str);
}

static int setting_func_boot_record(int nfd, char *str)
{
    return setting_func_autorecord_ui(str);
}

static int setting_func_language(int nfd, char *str)
{
    return setting_func_language_ui(str);
}

static int setting_func_frequency(int nfd, char *str)
{
    return setting_func_frequency_ui(str);
}

static int setting_func_bright(int nfd, char *str)
{
    return setting_func_backlight_lcd_ui(str);
}

static int setting_func_pip(int nfd, char *str)
{
    return setting_func_pip_lcd_ui(str);
}

static int setting_func_dvs(int nfd, char *str)
{
    return setting_func_dvs_ui(str);
}

static int setting_func_key_tone(int nfd, char *str)
{
    return setting_func_key_tone_ui(str);
}

static int setting_func_calibration(int nfd, char *str)
{
    return setting_func_calibration_ui(str);
}

static int setting_func_USBMODE(int nfd, char *str)
{
    return setting_func_USBMODE_ui(str);
}

static int setting_func_idc(int nfp, char *str)
{
    return setting_func_idc_ui(str);
}

static int setting_func_flip(int nfp, char *str)
{
    return setting_func_flip_ui(str);
}

static int setting_func_cvbs(int nfp, char *str)
{
    return 0;
}

static int setting_func_auto_off_screen(int nfp, char *str)
{
    return setting_func_autooff_screen_ui(str);
}

static int setting_func_collision_detect(int nfp, char *str)
{
    return setting_func_collision_ui(str);
}

static int setting_func_photo_quality(int nfp, char *str)
{
    return setting_func_photo_quaity_ui(str);
}

static int setting_func_video_quality(int nfp, char *str)
{
    return setting_func_quaity_ui(str);
}

static int setting_func_licence_plate(int nfp, char *str)
{
    return setting_func_licence_plate_ui(str);
}

static int setting_func_time_lapse(int nfp, char *str)
{
    return setting_func_time_lapse_ui(str);
}

static int setting_func_photo_burst(int nfp, char *str)
{
    return setting_func_photo_burst_ui(str);
}

static void *format_thread(void *arg)
{
    prctl(PR_SET_NAME, "format_thread", 0, 0, 0);
    setting_func_format_ui();
    return NULL;
}

static int setting_func_format(int nfd, char *str)
{
    pthread_t tid;

    if (get_format_status() == SD_STATE_FORMATING) {
        if (0 > write(nfd, "CMD_GET_ACK_FORMAT_STATUS_BUSY",
                      sizeof("CMD_GET_ACK_FORMAT_STATUS_BUSY")))
            printf("write fail!\r\n");
    } else if (get_format_status() == SD_STATE_OUT || get_format_status() == SD_STATE_ERR) {
        if (0 > write(nfd, "CMD_GET_ACK_FORMAT_STATUS_ERR",
                      sizeof("CMD_GET_ACK_FORMAT_STATUS_ERR")))
            printf("write fail!\r\n");
    } else {
        if (pthread_create(&tid, NULL, format_thread, NULL) != 0)
            printf("Create thread error!\n");
    }

    return 0;
}

static int setting_func_dateset(int nfd, char *str)
{
    return setting_func_setdata_ui(str);
}

static int setting_func_recovery(int nfd, char *str)
{
    return setting_func_recovery_ui();
}

static int fs_nofity_msg(char *filename, FILE_NOTIFY notify)
{
    char buff[512] = {0};
    int videotype = -1, i;
    FILETYPE filetype = UNKNOW;
    char * strFileType;
    char * path;

    fs_storage_fileinfo_get_bypath(filename, &filetype, &videotype);
    printf("filename=%s,filetype=%d,videotype=%d,notify=%d\n", filename,
           filetype, videotype, notify);


    if ( filetype == PICFILE_TYPE )
        strFileType = "picture";
    else if (filetype == LOCKFILE_TYPE)
        strFileType = "lock";
    else
        strFileType = "normal";

    switch (notify) {
    case FILE_NEW:
        return 0;

    case FILE_END:
        tcp_pack_file_for_path(strFileType, filename, videotype, filetype, buff);
        break;

    case FILE_DEL:
        path = strrchr(filename, '/');
        snprintf(buff, sizeof(buff), "CMD_CB_Delete:%sTYPE:%sFORM:%dEND", &path[1], strFileType, videotype);
        break;
    default:
        break;
    }

    printf("buff = %s\n", buff);
    for (i = 0; i < get_link_client_num(); i++) {
        if (write(get_tcp_server_socket(i), buff, strlen(buff)) < 0)
            printf("%s: write fail! <%d>\r\n", __func__, errno);
    }

    return 0;
}

static void wifi_msg_manager_cb(void *msgData, void *msg0, void *msg1)
{
    struct public_message msg;
    int i, arg;
    char buff[256] = {0};
    char **plicence_plate;
    char licence_plate[8][3];

    memset(&msg, 0, sizeof(msg));
    if (NULL != msgData) {
        msg.id = ((struct public_message *)msgData)->id;
        msg.type = ((struct public_message *)msgData)->type;
    }

    if (msg.type != TYPE_WIFI && msg.type != TYPE_BROADCAST)
        return;

    strcat(buff, "CMD_CB_");
    switch (msg.id) {
    case MSG_VIDEO_TIME_LENGTH:
        arg = (int)msg0;
        strcat(buff, "Videolength:");
        if (arg == 60)
            strcat(buff, "1min");
        else if (arg == 180)
            strcat(buff, "3min");
        else if (arg == 300)
            strcat(buff, "5min");
        else if (arg == -1)
            strcat(buff, "infinite");
        break;

    case MSG_SET_3DNR:
        arg = (int)msg0;
        strcat(buff, "3DNR:");
        if (arg)
            strcat(buff, "ON");
        else
            strcat(buff, "OFF");
        break;

    case MSG_SET_WHITE_BALANCE:
        arg = (int)msg0;
        strcat(buff, "WhiteBalance:");
        if (arg == 0)
            strcat(buff, "auto");
        else if (arg == 1)
            strcat(buff, "Daylight");
        else if (arg == 2)
            strcat(buff, "fluocrescence");
        else if (arg == 3)
            strcat(buff, "cloudysky");
        else if (arg == 4)
            strcat(buff, "tungsten");
        break;

    case MSG_SET_BACKLIGHT:
        arg = (int)msg0;
        strcat(buff, "Bright:");
        if (arg == LCD_BACKLT_L)
            strcat(buff, "low");
        else if (arg == LCD_BACKLT_M)
            strcat(buff, "middle");
        else if (arg == LCD_BACKLT_H)
            strcat(buff, "hight");
        break;

    case MSG_SET_PIP:
        arg = (int)msg0;
        strcat(buff, "PIP:");
        if (arg == 0)
            strcat(buff, "OFF");
        else if (arg == 1)
            strcat(buff, "ON");
        break;

    case MSG_SET_VIDEO_DVS_ONOFF:
        printf("get MSG_SET_VIDEO_DVS_ONOFF\n");
        arg = (int)msg0;
        strcat(buff, "DVS:");
        if (arg == 0)
            strcat(buff, "OFF");
        else if (arg == 1)
            strcat(buff, "ON");
        break;

    case MSG_SET_EXPOSURE:
        arg = (int)msg0;
        strcat(buff, "Exposure:");
        if (arg == 0)
            strcat(buff, "-3");
        else if (arg == 1)
            strcat(buff, "-2");
        else if (arg == 2)
            strcat(buff, "-1");
        else if (arg == 3)
            strcat(buff, "0");
        else if (arg == 4)
            strcat(buff, "1");
        break;

    case MSG_SET_SYS_TIME: {
        struct tm *tm;
        char tbuf[128];
        time_t t;

        time(&t);
        tm = localtime(&t);
        sprintf(tbuf, "Time:%04d-%02d-%02d %02d:%02d:%02d",
                tm->tm_year + 1900,
                tm->tm_mon + 1,
                tm->tm_mday,
                tm->tm_hour,
                tm->tm_min,
                tm->tm_sec);
        strcat(buff, tbuf);
    }
    break;

    case MSG_VIDEO_FRONT_REC_RESOLUTION:
        sprintf(buff, "CMD_CB_FrontCamera:front%d", (int)msg0 + 1);
        break;

    case MSG_VIDEO_BACK_REC_RESOLUTION:
        sprintf(buff, "CMD_CB_BackCamera:back%d", (int)msg0 + 1);
        break;

    case MSG_LEAVECARREC_ONOFF:
        arg = (int)msg0;
        strcat(buff, "LeaveCarRec:");
        if (arg)
            strcat(buff, "ON");
        else
            strcat(buff, "OFF");
        break;

    case MSG_SDCARDFORMAT_START:
        strcat(buff, "Format:Start");
        break;

    case MSG_SDCARDFORMAT_NOTIFY:
        arg = (int)msg0;
        if (arg)
            strcat(buff, "Format:Fault");
        else
            strcat(buff, "Format:Finish");
        break;

    case MSG_VIDEO_REC_START:
        strcat(buff, "StartRec");
        break;

    case MSG_VIDEO_REC_STOP:
        strcat(buff, "StopRec");
        break;

    case MSG_VIDEO_SET_ABMODE:
        arg = (int)msg0;
        if (arg == 0)
            strcat(buff, "RecordMode:Front");
        else if (arg == 1)
            strcat(buff, "RecordMode:Rear");
        else if (arg == 2)
            strcat(buff, "RecordMode:Double");
        break;

    case MSG_VIDEO_MARK_ONOFF:
        arg = (int)msg0;
        strcat(buff, "DataStamp:");
        if (arg)
            strcat(buff, "ON");
        else
            strcat(buff, "OFF");
        break;

    case MSG_SET_FREQ:
        arg = (int)msg0;
        strcat(buff, "Frequency:");
        if (arg == CAMARE_FREQ_50HZ)
            strcat(buff, "50HZ");
        else if (arg == CAMARE_FREQ_60HZ)
            strcat(buff, "60HZ");
        break;

    case MSG_VIDEO_AUTO_RECODE:
        arg = (int)msg0;
        strcat(buff, "BootRecord:");
        if (arg == 1)
            strcat(buff, "ON");
        else if (arg == 0)
            strcat(buff, "OFF");
        break;

    case MSG_SET_LANGUAGE:
        arg = (int)msg0;
        strcat(buff, "Language:");
        if (arg == 0)
            strcat(buff, "English");
        else if (arg == 1)
            strcat(buff, "Chinese");
        break;

    case MSG_VIDEO_MIC_ONOFF:
        arg = (int)msg0;
        strcat(buff, "RecordAudio:");
        if (arg == 1)
            strcat(buff, "ON");
        else if (arg == 0)
            strcat(buff, "OFF");
        break;

    case MSG_VIDEO_SET_MOTION_DETE:
        arg = (int)msg0;
        strcat(buff, "MotionDetection:");
        if (arg == 1)
            strcat(buff, "ON");
        else if (arg == 0)
            strcat(buff, "OFF");
        break;

    case MSG_VIDEO_ADAS_ONOFF:
        arg = (int)msg0;
        strcat(buff, "ADAS:");
        if (arg == 1)
            strcat(buff, "ON");
        else
            strcat(buff, "OFF");
        break;

    case MSG_VIDEO_IDC:
        arg = (int)msg0;
        strcat(buff, "IDC:");
        if (arg == 1)
            strcat(buff, "ON");
        else
            strcat(buff, "OFF");
        break;

    case MSG_VIDEO_FLIP:
        arg = (int)msg0;
        strcat(buff, "FLIP:");
        if (arg == 1)
            strcat(buff, "ON");
        else
            strcat(buff, "OFF");
        break;

    case MSG_VIDEO_CVBSOUT:
        arg = (int)msg0;
        strcat(buff, "CVBSOUT:");
        if (arg == OUT_DEVICE_CVBS_PAL)
            strcat(buff, "PAL");
        else if (arg == OUT_DEVICE_CVBS_NTSC)
            strcat(buff, "NTSC");
        else if (arg == OUT_DEVICE_LCD)
            strcat(buff, "LCD");
        break;

    case MSG_VIDEO_AUTOOFF_SCREEN:
        arg = (int)msg0;
        strcat(buff, "AUTOOFF_SCREEN:");
        if (arg != -1)
            strcat(buff, "ON");
        else
            strcat(buff, "OFF");
        break;

    case MSG_VIDEO_QUAITY:
        arg = (int)msg0;
        strcat(buff, "QUAITY:");
        if (arg == VIDEO_QUALITY_HIGH)
            strcat(buff, "HIGH");
        else if (arg == VIDEO_QUALITY_MID)
            strcat(buff, "MID");
        else if (arg == VIDEO_QUALITY_LOW)
            strcat(buff, "LOW");
        break;

    case MSG_COLLISION_LEVEL:
        arg = (int)msg0;
        strcat(buff, "Collision:");
        if (arg == COLLI_CLOSE)
            strcat(buff, "CLOSE");
        else if (arg == COLLI_LEVEL_L)
            strcat(buff, "L");
        else if (arg == COLLI_LEVEL_M)
            strcat(buff, "M");
        else if (arg == COLLI_LEVEL_H)
            strcat(buff, "H");
        break;

    case MSG_SET_VIDEO_DVS_CALIB:
        strcat(buff, "DVS_CALIB:START");
        break;

    case MSG_SET_KEY_TONE:
        sprintf(buff, "CMD_CB_KeyTone:%d", (int)msg0);
        break;

    case MSG_ACK_VIDEO_DVS_CALIB:
        arg = (int)msg0;
        strcat(buff, "DVS_CALIB:");
        if (arg == 0)
            strcat(buff, "SUCCESS");
        else
            strcat(buff, "FAULT");
        break;

    case MSG_MODE_CHANGE_PHOTO_NORMAL_NOTIFY:
        strcat(buff, "GET_MODE:PHOTO&OFF");
        break;

    case MSG_MODE_CHANGE_VIDEO_NORMAL_NOTIFY:
        strcat(buff, "GET_MODE:RECORDING&OFF");
        break;

    case MSG_MODE_CHANGE_TIME_LAPSE_NOTIFY:
        snprintf(buff, sizeof(buff), "CMD_CB_GET_MODE:LAPSE&%d",
                 parameter_get_time_lapse_interval());
        break;

    case MSG_MODE_CHANGE_PHOTO_BURST_NOTIFY:
        snprintf(buff, sizeof(buff), "CMD_CB_GET_MODE:BURST&%d",
                 parameter_get_photo_burst_num());
        break;

    case MSG_VIDEO_LICENSE_PLATE:
        strcat(buff, "LICENCE_PLATE:");
        if (parameter_get_licence_plate_flag()) {
            plicence_plate = parameter_get_licence_plate();
            memcpy(licence_plate, plicence_plate, sizeof(licence_plate));
            for (i = 0; i < 8; i++)
                strcat(buff, licence_plate[i]);
        }
        break;

    case MSG_FS_NOTIFY:
        printf("%s, create pic file name = %s, notify = %d\n", __func__, (char*)msg0, (FILE_NOTIFY)msg1);
        fs_nofity_msg((char *)msg0, (FILE_NOTIFY)msg1);
        return;

    case MSG_GPS_RAW_INFO:
        if ( get_live_client_num() > 0 ) {
            strcat(buff, "GPS_UPDAYA:");
            strcat(buff, (char *)msg0);
        }
        break;

    case MSG_SET_PHOTO_RESOLUTION: {
        const struct photo_param * p = (const struct photo_param *)msg0;
        const struct photo_param * start = api_get_photo_resolutions();
        int i, max = api_get_photo_resolution_max();

        if ( p == NULL || start == NULL )
            return;

        for (i = 0; i < max; i++) {
            if ( p->width == start[i].width && p->height == start[i].height) {
                sprintf(buff, "CMD_CB_Photo_Quality:%d", i + 1);
                break;
            }
        }
    }
    break;

    case MSG_MODE_CHANGE_NOTIFY:
        arg = (int)msg1;
        strcat(buff, "GET_MODE:");
        switch (arg) {
        case MODE_RECORDING:
            strcat(buff, "RECORDING&OFF");
            break;

        case MODE_PHOTO:
            strcat(buff, "PHOTO&OFF");
            break;

        case MODE_EXPLORER:
            strcat(buff, "EXPLORER");
            break;

        case MODE_PREVIEW:
            strcat(buff, "PREVIEW");
            break;

        case MODE_PLAY:
            strcat(buff, "PLAY");
            break;

        case MODE_USBDIALOG:
            strcat(buff, "USBDIALOG");
            break;

        case MODE_USBCONNECTION:
            strcat(buff, "USBCONNECTION");
            break;

        case MODE_SUSPEND:
            strcat(buff, "SUSPEND");
            break;

        case MODE_NONE:
        default:
            return;
        }

        break;

    default:
        return;
    }

    printf("%s: write msg = %s\n", __func__, buff);
    for (i = 0; i < get_link_client_num(); i++) {
        if (write(get_tcp_server_socket(i), buff, strlen(buff)) < 0)
            printf("write fail! <%d> socketid = %d buff-length = %d\r\n",errno,i,strlen(buff));
    }
}

void reg_msg_manager_cb(void)
{
    reg_entry(wifi_msg_manager_cb);
}

void unreg_msg_manager_cb(void)
{
    unreg_entry(wifi_msg_manager_cb);
}

const struct type_cmd_func cmd_setting_tab[] = {
    {setting_func_front_camera,     "Front_camera",},
    {setting_func_back_camera,      "Back_camera",},
    {setting_func_video_length,     "Videolength",},
    {setting_func_record_mode,      "RecordMode",},
    {setting_func_idc,              "IDC",},
    {setting_func_flip,             "FLIP",},
    {setting_func_cvbs,             "CVBS",},
    {setting_func_auto_off_screen,  "AutoOffScreen",},
    {setting_func_collision_detect, "Collision",},
    {setting_func_photo_quality,    "PhotoQuality",},
    {setting_func_video_quality,    "Quality",},
    {setting_func_licence_plate,    "LicencePlate",},
    {setting_func_time_lapse,       "TimeLapse",},
    {setting_func_photo_burst,      "Burst",},
    {setting_func_3dnr,             "3DNR",},
    {setting_func_adas,             "ADAS",},
    {setting_func_white_balance,    "WhiteBalance",},
    {setting_func_exposure,         "Exposure",},
    {setting_func_motion_detection, "MotionDetection",},
    {setting_func_data_stamp,       "DataStamp",},
    {setting_func_record_audio,     "RecordAudio",},
    {setting_func_boot_record,      "BootRecord",},
    {setting_func_language,         "Language",},
    {setting_func_frequency,        "Frequency",},
    {setting_func_bright,           "Bright",},
    {setting_func_pip,              "PIP",},
    {setting_func_dvs,              "DVS",},
    {setting_func_key_tone,         "KeyTone",},
    {setting_func_calibration,      "Calibration",},
    {setting_func_USBMODE,          "USBMODE",},
    {setting_func_format,           "Format",},
    {setting_func_dateset,          "DateSet",},
    {setting_func_recovery,         "Recovery",},
};

int setting_cmd_resolve(int nfd, char *cmdstr)
{
    int size = sizeof(cmd_setting_tab) / sizeof(cmd_setting_tab[0]);
    int i = 0;
    char *arg;

    for (i = 0; i < size; i++) {
        if (strstr(cmdstr, cmd_setting_tab[i].cmd) != 0 &&
            cmd_setting_tab[i].func != NULL) {
            arg = &cmdstr[sizeof(STR_ARGSETTING) +
                          strlen(cmd_setting_tab[i].cmd)];
            printf("setting buffer = %s\n arg = %s\n",
                   cmdstr, arg);
            return cmd_setting_tab[i].func(nfd, arg);
        }
    }

    return -1;
}
