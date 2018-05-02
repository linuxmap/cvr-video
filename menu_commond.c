/**
 * Copyright (C) 2016 Fuzhou Rockchip Electronics Co., Ltd
 * author: Neil Lin <neil.lin@rock-chips.com>
 *         Benjo Lei <benjo.lei@rock-chips.com>
 *         Jinkun Hong <hjk@rock-chips.com>
 *         Chris Zhong <zyw@rock-chips.com>
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

#include "menu_commond.h"

#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common.h"
#include "msg_list_manager.h"
#include "parameter.h"
#include "public_interface.h"
#include "licenseplate.h"
#include "settime.h"
#include "storage/storage.h"
#include "video.h"

#include "fs_manage/fs_sdcard.h"
#include "ueventmonitor/usb_sd_ctrl.h"

HWND hwnd_for_menu_command;

void get_main_window_hwnd(HWND hwnd)
{
    hwnd_for_menu_command = hwnd;
}

static struct video_param frontcamera_menu[2], backcamera_menu[2];

int getfontcameranum(void)
{
    return ARRAY_SIZE(frontcamera_menu);
}

int getbackcameranum(void)
{
    return ARRAY_SIZE(backcamera_menu);
}

struct video_param *getfontcamera(int n)
{
    video_record_get_front_resolution(frontcamera_menu, 2);

    return &frontcamera_menu[n];
}

struct video_param *getbackcamera(int n)
{
    video_record_get_back_resolution(backcamera_menu, 2);

    return &backcamera_menu[n];
}

int setting_func_recovery_ui(void)
{
    return api_recovery();
}

int setting_func_USBMODE_ui(char *str)
{
    if (!strcmp(str, "msc")) {
        printf("SET usb mode msc\n");
        api_change_usb_mode(USB_MODE_MSC);
        return 1;
    } else if (!strcmp(str, "adb")) {
        printf("SET usb mode adb\n");
        api_change_usb_mode(USB_MODE_ADB);
        return 1;
    }

    return 0;
}

int setting_func_phtomp_ui(struct photo_param* param)
{
    api_set_photo_resolution(param);

    return 0;
}

int setting_func_video_length_ui(char *str)
{
    int time = 0;

    if (!strcmp(str, "1min"))
        time = 60;
    else if (!strcmp(str, "3min"))
        time = 180;
    else if (!strcmp(str, "5min"))
        time = 300;
    else if (!strcmp(str, "infinite"))
        time = -1;
    else
        return -1;

    api_video_set_time_lenght(time);

    return 0;
}

int setting_func_3dnr_ui(char *str)
{
    char en_3dnr = 0;

    if (!strcmp(str, "ON"))
        en_3dnr = 1;
    else if (!strcmp(str, "OFF"))
        en_3dnr = 0;
    else
        return -1;

    api_3dnr_onoff(en_3dnr);

    return 0;
}

int get_format_status(void)
{
    return api_get_sd_mount_state();
}

int get_src_state(void)
{
    return api_video_get_record_state();
}

int setting_func_format_ui(void)
{
    api_sdcard_format(0);

    return 0;
}

int setting_func_adas_ui(char *str)
{
    if (!strcmp(str, "ON"))
        api_adas_onoff(1);
    else if (!strcmp(str, "OFF"))
        api_adas_onoff(0);

    return 0;
}

int setting_func_white_balance_ui(char *str)
{
    int wb = 0;

    if (!strcmp(str, "auto"))
        wb = 0;
    else if (!strcmp(str, "Daylight"))
        wb = 1;
    else if (!strcmp(str, "fluocrescence"))
        wb = 2;
    else if (!strcmp(str, "cloudysky"))
        wb = 3;
    else if (!strcmp(str, "tungsten"))
        wb = 4;
    else
        return -1;

    api_set_white_balance(wb);

    return 0;
}

int setting_func_exposure_ui(char *str)
{
    int ec = 0;

    if (!strcmp(str, "-3"))
        ec = 0;
    else if (!strcmp(str, "-2"))
        ec = 1;
    else if (!strcmp(str, "-1"))
        ec = 2;
    else if (!strcmp(str, "0"))
        ec = 3;
    else if (!strcmp(str, "1"))
        ec = 4;
    else
        return -1;

    api_set_exposure_compensation(ec);

    return 0;
}

int setting_func_motion_detection_ui(char *str)
{
    char md_en = 0;

    if (!strcmp(str, "ON"))
        md_en = 1;
    else if (!strcmp(str, "OFF"))
        md_en = 0;
    else
        return -1;

    api_motion_detection_onoff(md_en);

    return 0;
}

int setting_func_data_stamp_ui(char *str)
{
    char mark_en = 0;

    if (!strcmp(str, "ON"))
        mark_en = 1;
    else if (!strcmp(str, "OFF"))
        mark_en = 0;
    else
        return -1;

    api_video_mark_onoff(mark_en);

    return 0;
}

int setting_func_record_audio_ui(char *str)
{
    bool mic_en = false;

    if (!strcmp(str, "ON"))
        mic_en = true;
    else if (!strcmp(str, "OFF"))
        mic_en = false;
    else
        return -1;

    api_mic_onoff(mic_en);

    return 0;
}

int setting_func_language_ui(char *str)
{
    int language = LANGUAGE_ENG;

    if (!strcmp(str, "English"))
        language = LANGUAGE_ENG;
    else if (!strcmp(str, "Chinese"))
        language = LANGUAGE_CHA;
    else
        return -1;

    api_set_language(language);

    return 0;
}

int setting_func_backlight_lcd_ui(char *str)
{
    int level = LCD_BACKLT_H;

    if (!strcmp(str, "high"))
        level = LCD_BACKLT_H;
    else if (!strcmp(str, "mid"))
        level = LCD_BACKLT_M;
    else if (!strcmp(str, "low"))
        level = LCD_BACKLT_L;
    else
        return -1;

    api_set_lcd_backlight(level);

    return 0;
}

int setting_func_pip_lcd_ui(char *str)
{
    int status = 0;

    if (!strcmp(str, "ON"))
        status = 1;
    else if (!strcmp(str, "OFF"))
        status = 0;
    else
        return -1;
    api_set_lcd_pip(status);

    return 0;
}

int setting_func_dvs_ui(char *str)
{
    int status = 0;

    if (!strcmp(str, "ON"))
        status = 1;
    else if (!strcmp(str, "OFF"))
        status = 0;
    else
        return -1;

    api_set_video_dvs_onoff(status);

    return 0;
}

int setting_func_key_tone_ui(char *str)
{
    api_set_key_tone(atoi(str));
    return 0;
}

int setting_func_calibration_ui(char *str)
{
    return api_set_video_dvs_calibration();
}

int setting_func_cammp_ui(int m, char *str)
{
    int arg;

    if (m == 0) {
        sscanf(str, "front%d", &arg);
        printf("menu_commnd:%d\n", arg - 1);
        api_set_front_rec_resolution(arg - 1);
    } else if (m == 1) {
        sscanf(str, "back%d", &arg);
        printf("menu_commnd:%d\n", arg - 1);
        api_set_back_rec_resolution(arg - 1);
    }
    return 0;
}

int setting_func_setdata_ui(char *str)
{
    int ret;
    unsigned int year, mon, mday, hour, min, sec;
    bool dvs_on = video_dvs_is_enable();
    int video_record_state = api_video_get_record_state();

    /* If recording ,return directly */
    if (video_record_state == VIDEO_STATE_RECORD) {
        printf("Couldn't process date: %s. RECORDING......\n", str);
        return 0;
    }

    ret = sscanf(str, "%u-%u-%u %u:%u:%u",
                 &year, &mon, &mday, &hour, &min, &sec);
    if (ret != 6) {
        printf("Couldn't process date: %s\n", str);
        return -1;
    }

    if (dvs_on)
        video_dvs_enable(0);

    api_set_system_time(year, mon, mday, hour, min, sec);
    usleep(200000);

    if (dvs_on)
        video_dvs_enable(1);

    return 0;
}

int setting_func_frequency_ui(char *str)
{
    char freq = CAMARE_FREQ_50HZ;

    if (!strcmp(str, "50HZ"))
        freq = CAMARE_FREQ_50HZ;
    else if (!strcmp(str, "60HZ"))
        freq = CAMARE_FREQ_60HZ;
    else
        return -1;

    api_set_video_freq(freq);

    return 0;
}

int setting_func_autorecord_ui(char *str)
{
    char auto_rec_en = 0;
    printf("%s %s\n", __func__, str);
    if (!strcmp(str, "ON"))
        auto_rec_en = 1;
    else if (!strcmp(str, "OFF"))
        auto_rec_en = 0;
    else
        return -1;

    api_video_autorecode_onoff(auto_rec_en);

    return 0;
}

int setting_func_car_ui(char *str)
{
    char camera = 0;

    if (!strcmp(str, "Front"))
        camera = 0;
    else if (!strcmp(str, "Rear"))
        camera = 1;
    else if (!strcmp(str, "Double"))
        camera = 2;
    else
        return -1;

    api_set_abmode(camera);

    return 0;
}

int setting_func_rec_ui(char *str)
{
    if (!strcmp(str, "on")) {
        if (api_video_get_record_state() == VIDEO_STATE_PREVIEW) {
            if (api_get_sd_mount_state() == SD_STATE_IN)
                api_start_rec();
            else
                return 1;
        }
    } else if (!strcmp(str, "off")) {
        if (api_video_get_record_state() == VIDEO_STATE_RECORD)
            api_stop_rec();
        else
            return -1;
    }

    return 0;
}

int setting_func_collision_ui(char *str)
{
    char level = COLLI_CLOSE;

    if (!strcmp(str, "CLOSE"))
        level = COLLI_CLOSE;
    else if (!strcmp(str, "LOW"))
        level = COLLI_LEVEL_L;
    else if (!strcmp(str, "MID"))
        level = COLLI_LEVEL_M;
    else if (!strcmp(str, "HIGH"))
        level = COLLI_LEVEL_H;
    else
        return -1;

    api_set_collision_level(level);

    return 0;
}

int setting_func_parking_monitor_ui(char *str)
{
    char monitor_en = 0;

    if (!strcmp(str, "off"))
        monitor_en = 0;
    else if (!strcmp(str, "on"))
        monitor_en = 1;
    else
        return -1;

    api_leavecarrec_onoff(monitor_en);

    return 0;
}

int setting_func_reboot_ui(char *str)
{
    if (!strcmp(str, "off"))
        parameter_save_debug_reboot(0);
    else if (!strcmp(str, "on"))
        parameter_save_debug_reboot(1);

    return 0;
}

int setting_func_recover_ui(char *str)
{
    if (!strcmp(str, "off"))
        parameter_save_debug_recovery(0);
    else if (!strcmp(str, "on"))
        parameter_save_debug_recovery(1);

    return 0;
}

int setting_func_standby_ui(char *str)
{
    if (!strcmp(str, "off"))
        parameter_save_debug_standby(0);
    else if (!strcmp(str, "on"))
        parameter_save_debug_standby(1);

    return 0;
}

int setting_func_modechange_ui(char *str)
{
    if (!strcmp(str, "off"))
        parameter_save_debug_modechange(0);
    else if (!strcmp(str, "on"))
        parameter_save_debug_modechange(1);

    return 0;
}

int setting_func_video_ui(char *str)
{
    if (!strcmp(str, "off"))
        parameter_save_debug_video(0);
    else if (!strcmp(str, "on"))
        parameter_save_debug_video(1);

    return 0;
}

int setting_func_begin_end_video_ui(char *str)
{
    if (!strcmp(str, "off"))
        parameter_save_debug_beg_end_video(0);
    else if (!strcmp(str, "on"))
        parameter_save_debug_beg_end_video(1);

    return 0;
}

int setting_func_photo_ui(char *str)
{
    if (!strcmp(str, "off"))
        parameter_save_debug_photo(0);
    else if (!strcmp(str, "on"))
        parameter_save_debug_photo(1);

    return 0;
}

int setting_func_temp_ui(char *str)
{
    if (!strcmp(str, "off"))
        parameter_save_debug_temp(0);
    else if (!strcmp(str, "on"))
        parameter_save_debug_temp(1);

    return 0;
}

int setting_func_idc_ui(char *str)
{
    if (!strcmp(str, "OFF"))
        api_set_video_idc(0);
    else if (!strcmp(str, "ON"))
        api_set_video_idc(1);

    return 0;
}

int setting_func_flip_ui(char *str)
{
    if (!strcmp(str, "OFF"))
        api_video_reinit_flip(0);
    else if (!strcmp(str, "ON"))
        api_video_reinit_flip(1);

    return 0;
}

int setting_func_autooff_screen_ui(char *str)
{
    if (!strcmp(str, "OFF"))
        api_set_video_autooff_screen(VIDEO_AUTO_OFF_SCREEN_OFF);
    else if (!strcmp(str, "ON"))
        api_set_video_autooff_screen(VIDEO_AUTO_OFF_SCREEN_ON);

    return 0;
}

int setting_func_time_lapse_ui(char *str)
{
    if (!strcmp(str, "OFF"))
        api_set_video_time_lapse(0);
    else if (!strcmp(str, "1"))
        api_set_video_time_lapse(1);
    else if (!strcmp(str, "5"))
        api_set_video_time_lapse(5);
    else if (!strcmp(str, "10"))
        api_set_video_time_lapse(10);
    else if (!strcmp(str, "30")) {
        api_set_video_time_lapse(30);
    } else if (!strcmp(str, "60"))
        api_set_video_time_lapse(60);

    return 0;
}

int setting_func_photo_burst_ui(char *str)
{
    if (!strcmp(str, "OFF"))
        api_set_photo_burst_num(0);
    else if (!strcmp(str, "3"))
        api_set_photo_burst_num(3);
    else if (!strcmp(str, "4"))
        api_set_photo_burst_num(4);
    else if (!strcmp(str, "5"))
        api_set_photo_burst_num(5);

    return 0;
}

int setting_func_photo_quaity_ui(char *str)
{
    int arg = atoi(str);
    const struct photo_param * s = api_get_photo_resolutions();
    int max = parameter_get_photo_resolutions_max();

    if ( s == NULL || arg > max )
        return -1;

    printf("with= %d , high= %d\n", s[arg - 1].width, s[arg - 1].height);
    api_set_photo_resolution(&s[arg - 1]);
    return 0;
}

int setting_func_quaity_ui(char *str)
{
    unsigned int level = 0;

    if (!strcmp(str, "LOW"))
        level = VIDEO_QUALITY_LOW;
    else if (!strcmp(str, "MID"))
        level = VIDEO_QUALITY_MID;
    else if (!strcmp(str, "HIGH"))
        level = VIDEO_QUALITY_HIGH;
    else
        return -1;

    api_set_video_quaity(level);

    return 0;
}

int setting_func_licence_plate_ui(char *str)
{
    int i;
    char licence_plate[MAX_LICN_NUM][ONE_CHN_SIZE];

    if (str == NULL)
        return -1;

    licence_plate[0][0] = str[0];
    licence_plate[0][1] = str[1];
    licence_plate[0][2] = 0;

    for (i = 1; i < MAX_LICN_NUM; i++) {
        licence_plate[i][0] = str[i + 1];
        licence_plate[i][1] = 0;
    }
    api_set_video_license_plate((char *)licence_plate);

    return 0;
}

int get_reboot_parameter(void)
{
    return parameter_get_debug_reboot();
}

void save_reboot_parameter(int param)
{
    parameter_save_debug_reboot(param);
}

int get_recover_parameter(void)
{
    return parameter_get_debug_recovery();
}

void save_recover_parameter(int param)
{
    parameter_save_debug_recovery(param);
}

int get_standby_parameter(void)
{
    return parameter_get_debug_standby();
}

void save_standby_parameter(int param)
{
    parameter_save_debug_standby(param);
}

int get_mode_change_parameter(void)
{
    return parameter_get_debug_modechange();
}

void save_mode_change_parameter(int param)
{
    parameter_save_debug_modechange(param);
}

int get_video_parameter(void)
{
    return parameter_get_debug_video();
}

void save_video_parameter(int param)
{
    parameter_save_debug_video(param);
}

int get_beg_end_video_parameter(void)
{
    return parameter_get_debug_beg_end_video();
}

void save_beg_end_video_parameter(int param)
{
    parameter_save_debug_beg_end_video(param);
}

int get_photo_parameter(void)
{
    return parameter_get_debug_photo();
}

void save_photo_parameter(int param)
{
    parameter_save_debug_photo(param);
}

int get_temp_parameter(void)
{
    return parameter_get_debug_temp();
}

void save_temp_parameter(int param)
{
    parameter_save_debug_temp(param);
}

int get_video_bitrate_parameter(void)
{
    return parameter_get_bit_rate_per_pixel();
}

void save_video_bitrate_parameter(int param)
{
    parameter_save_bit_rate_per_pixel(param);
}

