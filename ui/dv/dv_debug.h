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

#ifndef __DV_DEBUG_H__
#define __DV_DEBUG_H__

#include <stdbool.h>
#include <minigui/common.h>

#define MAX_PAGE_LINE_NUM   7
#define MAX_DBG_SWITCH_NUM  2

extern int reboot_time;
extern int recovery_time;
extern int standby_time;
extern int modechange_time;
extern int video_time;
extern int beg_end_video_time;
extern int photo_time;
extern int fwupdate_test_time;

enum dv_debug {
    DBG_REBOOT,
    DBG_RECOVERY,
    DBG_STANDBY,
    DBG_MODE_CHANGE,
    DBG_VIDEOPLAY,
    DBG_BEGIN_END_VIDEO,
    DBG_PHOTO,
    DBG_TEMP_CONTROL,
    DBG_FW_UPGRADE,
    DBG_EFFECT_WATERMARK,

    MAX_DV_DEBUG,
};

struct debug_item_st {
    int item_selected;
    int item_page_num;

    int sub_sel_num;
    int sub_menu_page;
    int sub_menu_max;
    const char *item_str;

    void (*show_submenu_fuc)(HDC hdc, HWND hWnd);
};

extern int dv_setting;
void dv_fwupgrade_test(HWND hWnd);

void dv_debug_enter_reboot(HDC hdc, HWND hWnd);
void dv_debug_enter_recovery(HDC hdc, HWND hWnd);
void dv_debug_enter_awake(HDC hdc, HWND hWnd);
void dv_debug_enter_standby(HDC hdc, HWND hWnd);
void dv_debug_enter_mode_change(HDC hdc, HWND hWnd);
void dv_debug_enter_videoplayback(HDC hdc, HWND hWnd);
void dv_debug_enter_vediotest(HDC hdc, HWND hWnd);
void dv_debug_enter_phototest(HDC hdc, HWND hWnd);
void dv_debug_enter_temprature(HDC hdc, HWND hWnd);
void dv_debug_enter_effect_watermark(HDC hdc, HWND hWnd);
void dv_debug_auto_delete(long long tf_free);
void dv_debug_key_up(HWND hWnd);
void dv_debug_key_down(HWND hWnd);
void dv_debug_key_enter(HWND hWnd);
bool dv_debug_is_show_submenu(void);
void dv_debug_key_stdown(void);
void dv_debug_paint(HDC hdc, HWND hWnd);
void proc_debug_keydown(HWND hWnd, int wParam);
void draw_list_debug(HWND hWnd, HDC hdc);
void dv_timer_debug_process(HWND hWnd);

#endif //__DV_DEBUG_H__
