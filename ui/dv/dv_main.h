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

#ifndef __DV_MAIN_H__
#define __DV_MAIN_H__

#include <stdbool.h>
#include <minigui/common.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include "common.h"

#define BLOCK_PREV_NUM      1   /* min */
#define BLOCK_LATER_NUM     1   /* min */
#define USB_MODE            1
#define BATTERY_CUSHION     3
#define USE_KEY_STOP_USB_DIS_SHUTDOWN

#define CHARGING_DISPLAY_TIME 5

#define PARKING_ENABLE          1
#define PARKING_RECORD_COUNT    90
#define PARKING_SUSPEND         1
#define PARKING_SHUTDOWN        2

#define VIDEO_SOUND_FILE        "/usr/local/share/sounds/VideoRecord.wav"
#define CAPTURE_SOUND_FILE      "/usr/local/share/sounds/camera_click.wav"
#define KEYPRESS_SOUND_FILE     "/usr/local/share/sounds/KeypressStandard.wav"

#define DV_MAIN_RES_PATH        "/usr/local/share/minigui/res/images/"
#define WAITING_IMAGE_FILE      "/usr/local/share/minigui/res/images/waitfor2.gif"

/*sdcard show infomation */
#define SDCARD_NO_CARD          0
#define SDCARD_LOAD_FAIL        1
#define SDCARD_NO_SPACE         2
#define SDCARD_INIT_FAIL_ASK    3
#define SDCARD_VIDEO_DAMAGE     4
#define SDCARD_FORMAT_FAIL      5
#define SDCARD_FORMAT           6
#define SDCARD_INIT_FAIL        7
#define SDCARD_NOTIFY_MAX       8

#define TITLE_WARNING           0
#define TITLE_PROMPT            1
#define TITLE_FWUPGRADE         2
#define TITLE_VERSION           3
#define TITLE_MAX               4

#define TEXT_VIDEO_DEL          0
#define TEXT_VIDEO_ERR          1
#define TEXT_VIDEO_MAX          2

#define TEXT_RECOVERY           0
#define TEXT_SETTING            1
#define TEXT_VIDEO_LAPSE        2
#define TEXT_PHOTO_LAPSE        3
#define TEXT_PHOTO_SETTING      4
#define TEXT_PHO_BUR_SETTING    5
#define TEXT_CALIBRATION        6
#define TEXT_CALIBRATION_NOTE   7
#define TEXT_CALIBRATION_DONE   8
#define TEXT_CALIBRATION_ERR    9
#define TEXT_LICENSE_PLATE      10
#define TEXT_DELETE_FILE        11
#define TEXT_NOT_FOUND_FW       12
#define TEXT_ISINVALID_FW       13
#define TEXT_ASK_UPGRADE        14
#define TEXT_DV_DEBUG           15
#define TEXT_INFO_MAX           16

#define USB_MSC_TEXT            0
#define USB_CHANGRE_TEXT        1
#define USB_TEXT_MAX            2

#define TEXT_BUTTON_YES         0
#define TEXT_BUTTON_NO          1
#define TEXT_BUTTON_CFM         2

#define FW_TEXT_MAX             1

#define VIDEO_LAPSE_TIME_NUM    5
#define PHOTO_LAPSE_TIME_NUM    3
#define PREVIEW_DEL_MENU_NUM    2

#define PHOTO_NORMAL_SETTING_NUM    1
#define PHOTO_BURST_SETTING_NUM     2
#define PHOTO_SUBMENU_NUM       2
#define PHOTO_BURST_DEFAULT_NUM 3
#define DV_MAIN_FUNC_NUM        6   //8

#define IDC_ICONVIEW            100
#define IDC_TITLE               101
#define IDC_CONTENT             102

#define DBG(format,...)       printf("FUNC: %s, LINE: %d: "format, __func__, __LINE__, ##__VA_ARGS__)

enum {
    UI_FORMAT_BGRA_8888 = 0x1000,
    UI_FORMAT_RGB_565,
};

enum dv_icon_res {
    ICON_VIDEO_IMG_ID,
    ICON_I_VIDEO_IMG_ID,
    //ICON_S_VIDEO_IMG_ID,
    ICON_PLAY_IMG_ID,
    ICON_CAMERA_IMG_ID,
    //ICON_I_CAMERA_IMG_ID,
    ICON_CONTINUOUS_IMG_ID,
    ICON_SET_IMG_ID,
    ICON_INDEX_1S_IMG_ID,
    ICON_INDEX_5S_IMG_ID,
    ICON_INDEX_10S_IMG_ID,
    ICON_INDEX_30S_IMG_ID,
    ICON_INDEX_60S_IMG_ID,
    ICON_INDEX_BATTERY_01_IMG_ID,
    ICON_INDEX_BATTERY_02_IMG_ID,
    ICON_INDEX_BATTERY_03_IMG_ID,
    ICON_INDEX_BATTERY_04_IMG_ID,
    ICON_INDEX_BATTERY_05_IMG_ID,
    ICON_INDEX_BATTERY_06_IMG_ID,
    ICON_INDEX_BATTERY_07_IMG_ID,
    BOX_HIGHLIGHT_SEL_IMG_ID,
    BOX_HIGHLIGHT_SUB_SEL_IMG_ID,
    BOX_HIGHLIGHT_FUNC_IMG_ID,
    THUMB_LINE_IMG_ID,
    DOT_IMG_ID,
    DOT02_IMG_ID,
    BK_IMG_ID,
    ARROW_UFS_IMG_ID,
    SET_ICON_MORE_IMG_ID,
    ICON_INDEX_CAMERA_IMG_ID,
    ICON_INDEX_CONTINUOUS_IMG_ID,
    ICON_INDEX_I_CAMERA_IMG_ID,
    ICON_INDEX_I_VIDEO_IMG_ID,
    ICON_INDEX_S_VIDEO_IMG_ID,
    ICON_INDEX_SOUND_N_IMG_ID,
    ICON_INDEX_SOUND_IMG_ID,
    ICON_INDEX_VIDEO_IMG_ID,
    ICON_SET_02_IMG_ID,
    INDEX_BOX_BG0_IMG_ID,
    INDEX_BOX_BG1_IMG_ID,
    ICON_INDEX_PLAY_IMG_ID,
    ICON_INDEX_ANTI_SHAKE_IMG_ID,
    SET_ICON_PPI_IMG_ID,
    SET_ICON_DATE_IMG_ID,
    SET_ICON_GYROSCOPE_IMG_ID,
    SET_ICON_LANGUAGE_IMG_ID,
    SET_ICON_LIGHT_IMG_ID,
    SET_ICON_MIC_IMG_ID,
    SET_ICON_TONE_ON_IMG_ID,
    SET_ICON_WATERMARK_IMG_ID,
//    SET_ICON_PLATE_IMG_ID,
    SET_ICON_AWB_IMG_ID,
    SET_ICON_RECOVERY_IMG_ID,
    SET_ICON_FORMAT_IMG_ID,
    SET_ICON_WIFI_SSID_IMG_ID,
    SET_ICON_HZ_IMG_ID,
    SET_ICON_SCREENSAVER_IMG_ID,
    SET_ICON_VIDEO_AUTO_IMG_ID,
    SET_ICON_VIDEO_REC_TIME_IMG_ID,
    SET_ICON_STREAM_IMG_ID,
//    SET_ICON_CAR_IMG_ID,
    SET_ICON_FW_UPGRADE_IMG_ID,
    SET_ICON_VESION_IMG_ID,
    SET_ICON_DEBUG_IMG_ID,
    SET_ICON_PHOTO_IMG_ID,

    ICON_CONTINUOUS_3,
    ICON_CONTINUOUS_4,
    ICON_CONTINUOUS_5,
    ICON_CONTINUOUS_6,
    ICON_CONTINUOUS_7,
    ICON_CONTINUOUS_8,
    ICON_CONTINUOUS_9,
    ICON_CONTINUOUS_10,
    ICON_INDEX_SD_01,
    ICON_INDEX_SD_02,
    ICON_720P_30FPS,
    ICON_720P_60FPS,
    ICON_1080P_30FPS,
    ICON_1080P_60FPS,
    ICON_4K_25FPS,
    ICON_STOP_IMG_ID,

    WATERMARK_IMG_ID,
    SET_ICON_VIDEO_LAPSE_IMG_ID,
    TOP_BK_IMG_ID,
    USB_IMG_ID,
    CHARGING_ID,

    MAX_ICON_NUM,
};

enum dv_watermark_res {
    WATERMARK_NORMAL,
    WATERMARK_240P,
    WATERMARK_360P,
    WATERMARK_480P,
    WATERMARK_720P,
    WATERMARK_1080P,
    WATERMARK_1440P,
    WATERMARK_4K,

    MAX_WATERMARK_NUM,
};

enum dv_main_func {
    VIDEO_FUNC,
    VIDEO_LAPSE_FUNC,
    //VIDEO_SLOW_FUNC,
    PLAYBACK_FUNC,
    PHOTO_FUNC,
    //PHOTO_LAPSE_FUNC,
    PHOTO_BURST_FUNC,
    SYS_SETTING_FUNC,

    DV_FUNC_MAX,
};

enum time_lapse {
    LAPSE_1S = 1,
    LAPSE_5S = 5,
    LAPSE_10S = 10,
    LAPSE_30S = 30,
    LAPSE_60S = 60,

    MAX_LAPSE_NUM = 5,
};

enum dv_module_menu {
    VIDEO_LAPSE,
    PHOTO_LAPSE,
    PHOTO_NORMAL,
    PHOTO_BURST,

    PREVIEW_DELETE,
    MAX_MODULE_MENU,
};

enum dv_photo_submenu {
    PHOTO_RESOLUTION,
    MAX_PHO_SUBMENU,
};

enum dv_photo_burst_submenu {
    PHO_BUR_RESOLUTION,
    PHO_BUR_NUM,

    MAX_PHO_BUR_SUBMENU,
};

enum dv_photo_resolution {
    RES_720,
    RES_1080,
    RES_2K,
    RES_4K,

    MAX_PHO_RES,
};

enum dv_photo_burst_num {
    BURST_3,
    BURST_4,
    BURST_5,

    MAX_BURST_NUM,
};

enum dv_photo_switch {
    ON = 0,
    OFF,

    MAX_SWITCH,
};

enum dv_file_delete  {
    YES = 0,
    NO,

    MAX_SELETE,
};

enum video_resouliton {
    R_1080P_30FPS,
    R_1080P_60FPS,
    R_720P_30FPS,
    R_720P_60FPS,
    R_4K_25FPS,

    MAX_VIDEO_RES,
};

enum dv_video_play_state {
    VIDEO_STOP,
    VIDEO_PLAYING,
    VIDEO_PAUSE,
    VIDEO_FFW,
    VIDEO_FFD,
};

enum dv_video_preview_state {
    PREVIEW_NONE,
    PREVIEW_IN,
    PREVIEW_OUT,
};

struct module_menu_st {
    int menu_selected;
    int submenu_sel_num;
    int submenu_max;
    const char *title_str;

    const char *menu_text[LANGUAGE_NUM][10];
    struct module_submenu_st *submenu;

    void (*show_module_menu_fuc)(HDC hdc, HWND hWnd);
};

struct module_submenu_st {
    int sub_menu_show;
    int sub_sel_num;
    int sub_menu_page;
    int sub_menu_max;
    const char *title_str;

    const char *sub_menu_text[LANGUAGE_NUM][10];
    void (*show_module_submenu_fuc)(HDC hdc, HWND hWnd, int module_id);
};

extern int key_enter_list;
extern int preview_isvideo;
extern HWND hMainWnd;
extern int photo_burst_num_idx;
extern const char *title_text[2][TITLE_MAX];
extern const char *sdcard_text[2][SDCARD_NOTIFY_MAX];
extern const char *info_text[2][TEXT_INFO_MAX];
extern BITMAP dv_video_all[MAX_ICON_NUM];
extern int video_lapse_time [VIDEO_LAPSE_TIME_NUM];
extern struct module_menu_st g_module_menu[MAX_MODULE_MENU];
extern int photo_burst_num[MAX_BURST_NUM];
extern int dv_show_warning;
extern int dv_play_state;
extern int dv_preview_state;

/* Used for debug */
extern int debug_enter_list;
extern int debug_flag;

int get_key_state(void);
int get_videopreview_wnd(void);
int get_window_mode(void);
void set_screen_off_time(int time);
void loadingWaitBmp(HWND hWnd);
void unloadingWaitBmp(HWND hWnd);
BOOL proc_warning_dlg(HWND hWnd, int id, DWORD time);

#ifdef __cplusplus
extern "C" {
#endif
void start_motion_detection(HWND ui_hnd);
void stop_motion_detection();
#ifdef __cplusplus
}
#endif

#endif /* __DV_MAIN_H__  */
