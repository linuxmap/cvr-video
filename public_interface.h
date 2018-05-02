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

#ifndef __PUBLIC_VIDEO_INTERFACE_H__
#define __PUBLIC_VIDEO_INTERFACE_H__

#include "common.h"
#include "msg_list_manager.h"
#include <stdbool.h>

enum {
    USB_MODE_MSC = 0,
    USB_MODE_ADB,
    USB_MODE_UVC,
    USB_MODE_RNDIS,
};

enum {
    VIDEO_MODE_REC = 0,
    VIDEO_MODE_PHOTO,
};

enum {
    LANGUAGE_ENG = 0,
    LANGUAGE_CHA,
};

enum {
    MODE_NONE = -1,
    MODE_RECORDING = 0,
    MODE_PHOTO,
    MODE_EXPLORER,
    MODE_PREVIEW,
    MODE_PLAY,
    MODE_USBDIALOG,
    MODE_USBCONNECTION,
    MODE_SUSPEND,
    MODE_CHARGING,
    MODE_PHOTO_BURST,
    MODE_TIME_BURST,
    MODE_MAX
};

enum key_tone_vol {
    KEY_TONE_MUT = 0,
    KEY_TONE_LOW = 20,
    KEY_TONE_MID = 60,
    KEY_TONE_HIG = 100,
};
#ifdef __cplusplus
extern "C" {
#endif

void api_send_msg(int id, int type, void *msg0, void *msg1);
int storage_setting_reg_event_callback(void (*call)(int cmd, void *msg0, void *msg1));
void storage_setting_event_callback(int cmd, void *msg0, void *msg1);
void api_video_init(int mode);
void api_video_deinit(bool black);
int api_start_rec(void);
void api_stop_rec(void);
int api_forced_start_rec(void);
void api_forced_stop_rec(void);
int api_get_front_resolution(struct video_param* frame,
                             int count);
int api_get_back_resolution(struct video_param* frame,
                            int count);
int api_get_cif_resolution(struct video_param* frame,
                           int count);
int api_recovery(void);
void api_change_usb_mode(int mode);
void api_set_lcd_backlight(int level);
void api_set_lcd_pip(int status);
void api_set_language(int language);
int api_video_get_record_state(void);
void api_mic_onoff(int state);
int api_video_set_time_lenght(int time);
void api_set_photo_resolution(const struct photo_param* param);
void api_3dnr_onoff(int onoff);
void api_set_abmode(int mode);
void api_motion_detection_onoff(int onoff);
void api_motion_detection_start(void);
void api_motion_detection_stop(void);
void api_set_collision_level(int level);
void api_video_autorecode_onoff(int onoff);
void api_leavecarrec_onoff(int onoff);
void api_set_front_rec_resolution(int id);
void api_set_back_rec_resolution(int id);
void api_set_cif_rec_resolution(int id);

void api_video_reinit(void);
void api_video_reinit_flip(unsigned int i);
void api_set_white_balance(int level);
void api_set_exposure_compensation(int level);
void api_video_mark_onoff(int onoff);
int api_adas_init(int width, int height);
void api_adas_deinit(void);
void api_adas_set_wh(int width, int height);
int api_adas_onoff(int onoff);
void api_adas_get_parameter(void *parameter);
void api_adas_save_parameter(void *parameter);
void api_set_video_freq(int freq);
void api_set_system_time(int year, int mon, int mday, int hour,
                         int min, int sec);
char* api_get_firmware_version(void);
void api_set_key_tone(int key_tone_volume);
void api_set_car_mode_onoff(int onoff);
void api_set_video_dvs_onoff(int onoff);
int api_set_video_dvs_calibration(void);
void api_take_photo(int photo_num);
void api_set_video_quaity(unsigned int qualiy_level);
void api_poweron_init(fun_cb cb);
int api_get_sd_mount_state(void);
void api_set_sd_mount_state(int state);
void api_sdcard_format(int type);
void api_poweroff_deinit(void);
void api_watchdog_keep_alive(void);

void api_set_video_idc(unsigned int i);
void api_set_video_cvbsout(unsigned int i);
void api_set_video_flip(unsigned int i);
void api_set_video_time_lapse(unsigned int i);
void api_set_video_autooff_screen(int i);
void api_set_video_license_plate(char *buf);

void api_reg_ui_preview_callback(void (*enter_call)(void), void (*exit_call)(void));
void api_ui_preview_enter(void);
void api_ui_preview_exit(void);
void api_change_mode(int mode);
int api_get_mode(void);
int api_get_photo_burst_num(void);
int api_set_photo_burst_num(int num);
int api_get_dvs(void);
int api_set_dvs(int op);

int api_gps_init(void);
void api_gps_deinit(void);
int api_check_firmware(void);
int api_power_reboot(void);
int api_power_shutdown(void);
void api_dvs_onoff(bool onoff);
int api_get_photo_resolution_max(void);
const struct photo_param *api_get_photo_resolutions(void);
const struct video_param *api_get_front_cam_resolutions(void);
struct video_param *api_get_video_frontcamera_resolution();
void api_set_video_frontcamera_resolution(struct video_param *video_reso);

int api_get_key_level(void);
void api_set_key_volume(unsigned int key_tone_level);

#ifdef _PCBA_SERVICE_
int api_key_event_notice(int cmd, int msg0);
#endif

#ifdef __cplusplus
}
#endif
#endif /* __PUBLIC_VIDEO_INTERFACE_H__ */
