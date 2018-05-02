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

#ifndef __DV_SETTING_H__
#define __DV_SETTING_H__

#include <stdbool.h>
#include <minigui/common.h>
#include <minigui/gdi.h>

#define MAX_PAGE_LINE_NUM   7
#define MAX_MP_NUM          5
#define MAX_WB_NUM          5
#define MAX_LAN_NUM         2
#define MAX_LEVEL_NUM       3
#define MAX_REC_TIME_NUM    4
#define MAX_GS_NUM          3
#define MAX_HZ_NUM          2
#define MAX_SWITCH_NUM      2
#define MAX_KEY_TONE_NUM    4
#define MAX_DLG_BN_NUM      3

#define ONE_MINUTE          (1*60)

/* Dialog Button Ctrl ID */
#define IDC_YES             1000
#define IDC_NO              1001
#define IDC_CFM             1002

enum setting_item {
    PPI = 0,
    DATE,
    GYROSCOPE,
    LANGUAGE,
    LIGHT,
    MIC,
    KEY_TONE,
    /*PHOTO,*/
    WATERMARK,
//    PLATE,
    AWB,
    RECOVERY,
    FORMAT,
    WIFI,
    HZ,
    SCREEN_OFF,
    AUTO_REC,
    REC_TIME,
    VIDEO_QUALITY,
//    CAR_MODE,
    FW_UPGRADE,
    FW_VERSION,

    DV_DBG,

    MAX_SETTING_ITEM,
};

enum setting_date {
    DATE_SETTING = 0,
    DATE_Y,
    DATE_M,
    DATE_D,
    DATE_H,
    DATE_MIN,
    DATE_SEC,

    MAX_DATE_SETTING,
};

enum rec_time {
    REC_TIME_1MIN = 1 * ONE_MINUTE,
    REC_TIME_3MIN = 3 * ONE_MINUTE,
    REC_TIME_5MIN = 5 * ONE_MINUTE,
};

enum key_tone_vol {
    KEY_TONE_MUT = 0,
    KEY_TONE_LOW = 20,
    KEY_TONE_MID = 60,
    KEY_TONE_HIG = 100,
};

enum dlg_type {
    DLG_SD_FORMAT,
    DLG_SD_FORMAT_FAIL,
    DLG_SD_LOAD_FAIL,
    DLG_SD_NO_SPACE,
    DLG_SD_INIT_FAIL,
    DLG_SD_FWUPGRADE,
    DLG_RECOVERY,
};

struct settig_item_st {
    int item_selected;
    int item_page_num;

    int sub_sel_num;
    int sub_menu_page;
    int sub_menu_max;
    const char* item_str;

    void (*show_submenu_fuc)(HDC hdc, HWND hWnd);
};

extern HWND hMainWnd;
extern int debug_flag;
extern int dv_show_wifi;

void reformat_dv_sdcard(void);
void draw_list_of_setting(HWND hWnd, HDC hdc);
void dv_setting_item_paint(HDC hdc, HWND hWnd);
void dv_setting_key_up(HWND hWnd);
void dv_setting_key_down(HWND hWnd);
void dv_setting_key_enter(HWND hWnd);
void dv_setting_key_stdown(void);
bool dv_setting_is_show_submenu (void);
void dv_setting_init(void);
void dv_create_select_dialog(const char* caption_txt);
void dv_create_message_dialog(const char* caption_txt);
#endif /*__DV_SETTING_H__*/
