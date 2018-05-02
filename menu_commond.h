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

#ifndef __MENUCOMMOND_H__
#define __MENUCOMMOND_H__

#include <minigui/common.h>
#include "common.h"


extern RECT msg_rcAB;
extern RECT adas_rc;
extern RECT msg_rcMove;
extern struct FlagForUI FlagForUI_ca;

void get_main_window_hwnd(HWND hwnd);
struct video_param *getfontcamera(int n);
struct video_param *getbackcamera(int n);
int setting_func_recovery_ui(void);
int setting_func_USBMODE_ui(char *str);
int setting_func_phtomp_ui(struct photo_param* param);
int setting_func_video_length_ui(char *str);
int setting_func_3dnr_ui(char *str);
int setting_func_format_ui(void);
int setting_func_adas_ui(char *str);
int setting_func_white_balance_ui(char *str);
int setting_func_exposure_ui(char *str);
int setting_func_motion_detection_ui(char *str);
int setting_func_data_stamp_ui(char *str);
int setting_func_record_audio_ui(char *str);
int setting_func_language_ui(char *str);
int setting_func_backlight_lcd_ui(char *str);
int setting_func_pip_lcd_ui(char *str);
int setting_func_key_tone_ui(char *str);
int setting_func_calibration_ui(char *str);
int setting_func_dvs_ui(char *str);
int setting_func_cammp_ui(int m, char *str);
int setting_func_setdata_ui(char *str);
int setting_func_frequency_ui(char *str);
int setting_func_autorecord_ui(char *str);
int setting_func_car_ui(char *str);
int setting_func_rec_ui(char *str);
int setting_func_parking_monitor_ui(char *str);

int setting_func_idc_ui(char *str);
int setting_func_flip_ui(char *str);
int setting_func_autooff_screen_ui(char *str);
int setting_func_photo_burst_ui(char *str);
int setting_func_time_lapse_ui(char *str);
int setting_func_quaity_ui(char *str);
int setting_func_collision_ui(char *str);
int setting_func_licence_plate_ui(char *str);

int setting_func_reboot_ui(char *str);
int get_reboot_parameter(void);
void save_reboot_parameter(int param);
int setting_func_recover_ui(char *str);
int get_recover_parameter(void);
void save_recover_parameter(int param);
int setting_func_awake_1_ui(char *str);
int get_awake_parameter(void);
void save_awake_parameter(int param);
int setting_func_standby_ui(char *str);
int get_standby_parameter(void);
void save_standby_parameter(int param);
int setting_func_modechange_ui(char *str);
int settingsscanf_func_modechange_ui(char *str);
int get_mode_change_parameter(void);
void save_mode_change_parameter(int param);
int setting_func_video_ui(char *str);
int get_video_parameter(void);
void save_video_parameter(int param);
int setting_func_begin_end_video_ui(char *str);
int get_beg_end_video_parameter(void);
void save_beg_end_video_parameter(int param);
int setting_func_photo_ui(char *str);
int get_photo_parameter(void);
void save_photo_parameter(int param);
int setting_func_temp_ui(char *str);
int get_temp_parameter(void);
void save_temp_parameter(int param);
int get_video_bitrate_parameter(void);
void save_video_bitrate_parameter(int param);
int getfontcameranum(void);
int get_format_status(void);
int getbackcameranum(void);
int get_src_state(void);
int setting_func_photo_quaity_ui(char *str);
#endif /* __MENUCOMMOND_H__ */
