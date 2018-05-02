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

#include "dv_video.h"

#include <minigui/gdi.h>
#include <minigui/window.h>

#include "dv_main.h"
#include "parameter.h"
#include "public_interface.h"
#include "sport_dv_ui.h"
#include "ui_resolution.h"
#include "video.h"

const char *video_lapse_text [LANGUAGE_NUM][VIDEO_LAPSE_TIME_NUM] = {
    {
        "1 s lapse",
        "5 s lapse",
        "10s lapse",
        "30s lapse",
        "60s lapse",
    },
    {
        "¼ä¸ô 1  Ãë",
        "¼ä¸ô 5  Ãë",
        "¼ä¸ô 10 Ãë",
        "¼ä¸ô 30 Ãë",
        "¼ä¸ô 60 Ãë",
    }
};

void dv_video_proc(void)
{
    if (api_video_get_record_state() == VIDEO_STATE_PREVIEW)
        api_start_rec();
    else if (api_video_get_record_state() == VIDEO_STATE_RECORD)
        api_stop_rec();
}

void dv_video_lapse_proc(HWND hWnd)
{
    struct module_menu_st *menu_lapse;
    menu_lapse = &g_module_menu[VIDEO_LAPSE];

    if (menu_lapse->menu_selected == 0) {
        if (api_video_get_record_state() == VIDEO_STATE_PREVIEW) {
            api_start_rec();
        } else if (api_video_get_record_state() == VIDEO_STATE_RECORD) {
            api_stop_rec();
        }
    } else {
        bool is_recording = false;
        /* Destory lapse menu */
        if (parameter_get_time_lapse_interval() != video_lapse_time[menu_lapse->submenu_sel_num]) {
            parameter_save_time_lapse_interval(video_lapse_time[menu_lapse->submenu_sel_num]);

            /* If recording,force stop and set new timelapse,then restart. */
            if (api_video_get_record_state() == VIDEO_STATE_RECORD) {
               is_recording = true;
               api_forced_stop_rec();
            }

            video_record_set_timelapseint(video_lapse_time[menu_lapse->submenu_sel_num]);

            if (is_recording)
                api_forced_start_rec();
        }

        menu_lapse->menu_selected = 0;
        InvalidateRect(hWnd, NULL, TRUE);
    }
}

void dv_video_slow_proc(HWND hWnd)
{
    struct module_menu_st *menu_lapse;
    menu_lapse = &g_module_menu[VIDEO_LAPSE];

    if (menu_lapse->menu_selected == 0) {
        if (video_record_get_state() == VIDEO_STATE_PREVIEW) {
            api_start_rec();
        } else if (video_record_get_state() == VIDEO_STATE_RECORD) {
            api_stop_rec();
        }
    } else {
        /* Destory slow menu */
        menu_lapse->menu_selected = 0;
        InvalidateRect(hWnd, NULL, TRUE);
    }
}

/* press key_down pompt lapse menu */
void show_video_lapse_menu(HDC hdc, HWND hWnd)
{
    int i;
    int line_pos = 0, line_select_pos = 0;
    RECT rcDraw;
    struct module_menu_st menu_lapse;
    menu_lapse = g_module_menu[VIDEO_LAPSE];

    int lang = parameter_get_video_lan();

    if (menu_lapse.menu_selected == 1) {
        SetBkColor(hdc, GetWindowElementPixel(hMainWnd, WE_BGC_TOOLTIP));
        SelectFont(hdc, (PLOGFONT)GetWindowElementAttr(hWnd, WE_FONT_MENU));

        FillBoxWithBitmap(hdc, KEY_DOWN_LIST_X, KEY_DOWN_LIST_Y,
                          KEY_DOWN_LIST_W, KEY_DOWN_LIST_H, &dv_video_all[INDEX_BOX_BG1_IMG_ID]);

        SetBkMode (hdc, BM_TRANSPARENT);
        SetTextColor (hdc, PIXEL_lightwhite);

        for (i = 0; i <= menu_lapse.submenu_max; i++) {
            rcDraw.left = KEY_SUBMENU_TEXT_X;
            rcDraw.top  = KEY_DOWN_LIST_Y + i * KEY_ENTER_BOX_H;
            rcDraw.right = rcDraw.left + KEY_ENTER_BOX_W;
            rcDraw.bottom = rcDraw.top + KEY_ENTER_BOX_H;

            /* Draw submenu title */
            if (i == 0)
                DrawText(hdc, info_text[lang][TEXT_VIDEO_LAPSE],
                         -1, &rcDraw, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
            /* Draw submenu content */
            else
                DrawText(hdc, video_lapse_text[lang][i - 1],
                         -1, &rcDraw, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
        }

        line_select_pos = menu_lapse.submenu_sel_num;
        line_pos = (line_select_pos + 1) * KEY_ENTER_BOX_H;
        FillBoxWithBitmap(hdc, KEY_DOWN_LIST_X, KEY_DOWN_LIST_Y + line_pos,
                          KEY_DOWN_LIST_W, KEY_ENTER_BOX_H,
                          &dv_video_all[BOX_HIGHLIGHT_SUB_SEL_IMG_ID]);

        SetTextColor(hdc, COLOR_red);
        rcDraw.top  = KEY_DOWN_LIST_Y + line_pos;
        rcDraw.bottom = rcDraw.top + KEY_ENTER_BOX_H;
        DrawText(hdc, video_lapse_text[lang][line_select_pos],
                 -1, &rcDraw, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
    }
}
