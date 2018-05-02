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

#include "dv_photo.h"

#include <minigui/gdi.h>
#include <minigui/window.h>

#include "dv_main.h"
#include "parameter.h"
#include "public_interface.h"
#include "sport_dv_ui.h"
#include "video.h"
#include "audio/playback/audioplay.h"
#include "ueventmonitor/usb_sd_ctrl.h"

bool takephoto;
int photo_burst_num_idx = 0;    /*default:unlimited*/

const char *photo_lapse_text[LANGUAGE_NUM][PHOTO_LAPSE_TIME_NUM] = {
    {
        "10s lapse",
        "20s lapse",
        "30s lapse",
    },
    {
        "间隔 10 秒",
        "间隔 20 秒",
        "间隔 30 秒",
    }
};

const char *photo_menu_text[LANGUAGE_NUM][MAX_PHO_SUBMENU] = {
    {
        "Resolution ratio",
    },
    {
        "分辨率",
    },
};

const char *photo_burst_menu_text[LANGUAGE_NUM][MAX_PHO_BUR_SUBMENU] = {
    {
        "Resolution ratio",
        "Burst Number",
    },
    {
        "分辨率",
        "连拍张数",
    },
};

const char *photo_resolution_text[LANGUAGE_NUM][MAX_PHO_RES] = {
    {
        "1280X720",
        "1920X1080",
        "2560X1440",
        "3840X2160",
    },
    {
        "1280X720",
        "1920X1080",
        "2560X1440",
        "3840X2160",
    },
};

const char *photo_burst_num_text[LANGUAGE_NUM][MAX_BURST_NUM] = {
    {
        "3",
        "4",
        "5",
    },
    {
        "3",
        "4",
        "5",
    },
};

const char *photo_switch_text[LANGUAGE_NUM][MAX_SWITCH] = {
    {
        "On",
        "Off",
    },
    {
        "打开",
        "关闭",
    },
};

const char *file_delete_text[LANGUAGE_NUM][MAX_SELETE] = {
    {
        "Yes",
        "No",
    },
    {
        "是",
        "否",
    },
};

const struct photo_param dv_photo_reso[MAX_PHO_RES] = {
    {1280, 720,  5},
    {1920, 1080, 5},
    {2560, 1440, 5},
    {3840, 2160, 5},
};

const struct photo_param *get_photo_resolutions(void)
{
    return dv_photo_reso;
}

int get_photo_resolution(void)
{
    struct photo_param *param = parameter_get_photo_param();

    switch (param->width) {
    case 1280:
        return RES_720;

    case 1920:
        return RES_1080;

    case 2560:
        return RES_2K;

    case 3840:
        return RES_4K;
    }

    return RES_1080;
}

int get_dv_photo_burst_num_index (void)
{
    int burst_num = parameter_get_photo_burst_num();
    burst_num -= PHOTO_BURST_DEFAULT_NUM;

    switch (burst_num) {
    case BURST_3:
        return 0;

    case BURST_4:
        return 1;

    case BURST_5:
        return 2;
    }

    return 0;
}

void set_dv_photo_burst_num(int sel)
{
    switch (sel) {
    case 0:
        parameter_save_photo_burst_num(BURST_3 + PHOTO_BURST_DEFAULT_NUM);
        break;

    case 1:
        parameter_save_photo_burst_num(BURST_4 + PHOTO_BURST_DEFAULT_NUM);
        break;

    case 2:
        parameter_save_photo_burst_num(BURST_5 + PHOTO_BURST_DEFAULT_NUM);
        break;

    default:
        parameter_save_photo_burst_num(BURST_3 + PHOTO_BURST_DEFAULT_NUM);
        break;
    }
}

bool get_takephoto_state(void)
{
    return takephoto;
}

void set_takephoto_state(bool takingphoto)
{
    takephoto = takingphoto;
}

void dv_photo_proc(HWND hWnd)
{
    int reso_idx;
    struct photo_param *param;
    struct module_menu_st *menu_photo = &g_module_menu[PHOTO_NORMAL];
    struct module_submenu_st *submenu_photo = menu_photo->submenu +
                menu_photo->submenu_sel_num;

    if (menu_photo->menu_selected == 0) {
        dv_take_photo(hWnd);
    } else {
        if ( menu_photo->submenu != NULL ) {
            if (submenu_photo->sub_menu_show == 0) {
                submenu_photo->sub_menu_show = 1;

                InvalidateRect(hWnd, NULL, TRUE);
            } else {
                switch (menu_photo->submenu_sel_num) {
                case PHOTO_RESOLUTION:
                    reso_idx = submenu_photo->sub_sel_num;
                    param = parameter_get_photo_param();

                    if (param->width == dv_photo_reso[reso_idx].width
                        && param->height == dv_photo_reso[reso_idx].height) {
                        break;
                    } else {
                        param->width = dv_photo_reso[reso_idx].width;
                        param->height = dv_photo_reso[reso_idx].height;
                    }

                    api_set_photo_resolution(param);
                    break;
                }

                submenu_photo->sub_menu_show = 0;
                InvalidateRect(hWnd, NULL, TRUE);
            }
        }
    }
}

void dv_photo_burst_proc(HWND hWnd)
{
    int reso_idx, burst_num_idx;
    struct photo_param *param;
    struct module_menu_st *menu_photo_bur = &g_module_menu[PHOTO_BURST];
    struct module_submenu_st *submenu_photo = menu_photo_bur->submenu +
                menu_photo_bur->submenu_sel_num;

    if (menu_photo_bur->menu_selected == 0) {
        dv_take_photo_burst(hWnd);
    } else {
        if ( menu_photo_bur->submenu != NULL ) {
            if (submenu_photo->sub_menu_show == 0) {
                submenu_photo->sub_menu_show = 1;

                InvalidateRect(hWnd, NULL, TRUE);
            } else {
                switch (menu_photo_bur->submenu_sel_num) {
                case PHO_BUR_RESOLUTION:
                    reso_idx = submenu_photo->sub_sel_num;
                    param = parameter_get_photo_param();

                    if (reso_idx == get_photo_resolution())
                        break;

                    param->width = dv_photo_reso[reso_idx].width;
                    param->height = dv_photo_reso[reso_idx].height;

                    api_set_photo_resolution(param);
                    break;

                case PHO_BUR_NUM:
                    burst_num_idx = get_dv_photo_burst_num_index();

                    if (burst_num_idx == submenu_photo->sub_sel_num)
                        break;

                    photo_burst_num_idx = submenu_photo->sub_sel_num;
                    set_dv_photo_burst_num(submenu_photo->sub_sel_num);

                    break;
                }

                submenu_photo->sub_menu_show = 0;
                InvalidateRect(hWnd, NULL, TRUE);
            }
        }
    }
}

void dv_photo_lapse_proc(HWND hWnd)
{
#if 0

    if (api_get_sd_mount_state() == SD_STATE_IN && !takephoto) {
        takephoto = true;
        audio_sync_play(CAMERA_CLICK_FILE);
        loadingWaitBmp(hWnd);

        if (video_record_takephoto() == -1) {
            DBG("No input video devices!\n");
            unloadingWaitBmp(hWnd);
            takephoto = false;
        }
    }

#else
    struct module_menu_st *menu_lapse;
    menu_lapse = &g_module_menu[PHOTO_LAPSE];

    if (menu_lapse->menu_selected == 0) {
        dv_take_photo_lapse(hWnd);
    } else {
        /* destory lapse menu*/
        //parameter_save_time_lapse_interval(video_lapse_time[menu_lapse->submenu_sel_num]);

        menu_lapse->menu_selected = 0;
        InvalidateRect(hWnd, NULL, TRUE);
    }

#endif
}

void dv_take_photo_lapse(HWND hWnd)
{
    /*
        if (api_get_sd_mount_state() == SD_STATE_IN && !takephoto) {
            takephoto = true;
            audio_sync_play(CAMERA_CLICK_FILE);
            loadingWaitBmp(hWnd);

            if (video_record_takephoto(take_num) == -1) {
                DBG("No input video devices!\n");
                unloadingWaitBmp(hWnd);
                takephoto = false;
            }
        }
    */

    if (api_get_sd_mount_state() == SD_STATE_OUT) {
        dv_show_warning = 1;
        audio_sync_play(KEYPRESS_SOUND_FILE);
        SetTimerEx(hWnd, ID_WARNING_DLG_TIMER, 300, proc_warning_dlg);
    }
}

void dv_take_photo(HWND hWnd)
{
    if (api_get_sd_mount_state() == SD_STATE_IN && !takephoto) {
        takephoto = true;
        DBG(" play camera sound \n");
        audio_sync_play(CAPTURE_SOUND_FILE);
        loadingWaitBmp(hWnd);

        if (video_record_takephoto(1) == -1) {
            printf("No input video devices!\n");
            unloadingWaitBmp(hWnd);
            takephoto = false;
        }
    }

    if (api_get_sd_mount_state() == SD_STATE_OUT) {
        audio_sync_play(KEYPRESS_SOUND_FILE);
        dv_show_warning = 1;
        SetTimerEx(hWnd, ID_WARNING_DLG_TIMER, 300, proc_warning_dlg);
    }
}

void dv_take_photo_burst(HWND hWnd)
{
    int take_num;
    photo_burst_num_idx = get_dv_photo_burst_num_index();
    take_num = photo_burst_num[photo_burst_num_idx] + PHOTO_BURST_DEFAULT_NUM;

    if (api_get_sd_mount_state() == SD_STATE_IN && !takephoto) {
        takephoto = true;
        audio_sync_play(CAPTURE_SOUND_FILE);
        loadingWaitBmp(hWnd);

        if (video_record_takephoto(take_num) == -1) {
            printf("No input video devices!\n");
            unloadingWaitBmp(hWnd);
            takephoto = false;
        }
    }

    if (api_get_sd_mount_state() == SD_STATE_OUT) {
        dv_show_warning = 1;
        audio_sync_play(KEYPRESS_SOUND_FILE);
        SetTimerEx(hWnd, ID_WARNING_DLG_TIMER, 300, proc_warning_dlg);
    }
}

static void dv_show_menu_common(HDC hdc, HWND hWnd, int moduleID)
{
    RECT rcDraw;
    const char *lable_str = NULL;
    int i, line_pos = 0, line_select_pos = 0;
    unsigned int oldcolor;
    int lang = (int)parameter_get_video_lan();
    struct module_menu_st menu_photo = g_module_menu[moduleID];

    if (menu_photo.menu_selected != 1)
        return;

    SetBkColor(hdc, GetWindowElementPixel(hMainWnd, WE_BGC_TOOLTIP));
    SelectFont(hdc, (PLOGFONT)GetWindowElementAttr(hWnd, WE_FONT_MENU));
    FillBoxWithBitmap(hdc, KEY_DOWN_LIST_X, KEY_DOWN_LIST_Y,
                      KEY_DOWN_LIST_W, KEY_DOWN_LIST_H, &dv_video_all[INDEX_BOX_BG1_IMG_ID]);

    SetBkMode (hdc, BM_TRANSPARENT);
    SetTextColor (hdc, PIXEL_lightwhite);

    switch (moduleID) {
    case PHOTO_NORMAL:
        lable_str = info_text[lang][TEXT_PHOTO_SETTING];
        break;

    case PHOTO_LAPSE:
        lable_str = info_text[lang][TEXT_PHOTO_LAPSE];
        break;

    case PHOTO_BURST:
        lable_str = info_text[lang][TEXT_PHO_BUR_SETTING];
        break;

    case PREVIEW_DELETE:
        lable_str = info_text[lang][TEXT_DELETE_FILE];
        break;

    default:
        lable_str = " ";
        break;
    }

    for (i = 0; i <= menu_photo.submenu_max; i++) {
        rcDraw.left     = KEY_SUBMENU_TEXT_X;
        rcDraw.top      = KEY_DOWN_LIST_Y + i * KEY_ENTER_BOX_H;
        rcDraw.right    = rcDraw.left + KEY_ENTER_BOX_W;
        rcDraw.bottom   = rcDraw.top + KEY_ENTER_BOX_H;

        /*Draw submenu title*/
        if (i == 0)
            DrawText(hdc, lable_str,
                     -1, &rcDraw, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
        /*Draw submenu content*/
        else
            DrawText(hdc, menu_photo.menu_text[lang][i - 1],
                     -1, &rcDraw, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
    }

    line_select_pos = menu_photo.submenu_sel_num;
    line_pos = (line_select_pos + 1) * KEY_ENTER_BOX_H;
    FillBoxWithBitmap(hdc, KEY_DOWN_LIST_X, KEY_DOWN_LIST_Y + line_pos,
                      KEY_DOWN_LIST_W, KEY_ENTER_BOX_H,
                      &dv_video_all[BOX_HIGHLIGHT_SUB_SEL_IMG_ID]);

    oldcolor = SetTextColor(hdc, COLOR_red);
    rcDraw.top  = KEY_DOWN_LIST_Y + line_pos;
    rcDraw.bottom = rcDraw.top + KEY_ENTER_BOX_H;

    switch (moduleID) {
    case PHOTO_NORMAL:
        DrawText(hdc, photo_menu_text[lang][line_select_pos],
                 -1, &rcDraw, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
        break;

    case PHOTO_LAPSE:
        DrawText(hdc, photo_lapse_text[lang][line_select_pos],
                 -1, &rcDraw, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
        break;

    case PHOTO_BURST:
        DrawText(hdc, photo_burst_menu_text[lang][line_select_pos],
                 -1, &rcDraw, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
        break;

    case PREVIEW_DELETE:
        DrawText(hdc, file_delete_text[lang][line_select_pos],
                 -1, &rcDraw, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
        break;
    }

    SetTextColor(hdc, oldcolor);
}

void show_photo_normal_menu(HDC hdc, HWND hWnd)
{
    dv_show_menu_common(hdc, hWnd, PHOTO_NORMAL);
}

void show_photo_lapse_menu(HDC hdc, HWND hWnd)
{
    dv_show_menu_common(hdc, hWnd, PHOTO_LAPSE);
}

void show_photo_burst_menu(HDC hdc, HWND hWnd)
{
    dv_show_menu_common(hdc, hWnd, PHOTO_BURST);
}

void show_preview_delete_menu(HDC hdc, HWND hWnd)
{
    dv_show_menu_common(hdc, hWnd, PREVIEW_DELETE);
}

/*
 * press key_down pompt photo setting menu item submenu according to father
 * menu id.
 */
void show_photo_submenu(HDC hdc, HWND hWnd, int module_id)
{
    int i;
    int line_pos = 0;
    int line_select_pos = 0;
    RECT rcDraw;
    unsigned int old_text_color;
    int lang = (int)parameter_get_video_lan();
    struct module_menu_st *modu_menu = &g_module_menu[module_id];
    struct module_submenu_st *sub_menu = modu_menu->submenu + modu_menu->submenu_sel_num;

    if (modu_menu->menu_selected != 1)
        return;

    SetBkColor(hdc, GetWindowElementPixel(hMainWnd, WE_BGC_TOOLTIP));
    SelectFont(hdc, (PLOGFONT)GetWindowElementAttr(hWnd, WE_FONT_MENU));

    FillBoxWithBitmap(hdc, KEY_DOWN_LIST_X, KEY_DOWN_LIST_Y,
                      KEY_DOWN_LIST_W, KEY_DOWN_LIST_H, &dv_video_all[INDEX_BOX_BG1_IMG_ID]);

    SetBkMode (hdc, BM_TRANSPARENT);
    SetTextColor (hdc, PIXEL_lightwhite);

    for (i = 0; i <= sub_menu->sub_menu_max; i++) {
        rcDraw.left     = KEY_SUBMENU_TEXT_X;
        rcDraw.top      = KEY_DOWN_LIST_Y + i * KEY_ENTER_BOX_H;
        rcDraw.right    = rcDraw.left + KEY_ENTER_BOX_W;
        rcDraw.bottom   = rcDraw.top + KEY_ENTER_BOX_H;

        /*Draw submenu title*/
        if (i == 0)
            DrawText(hdc, modu_menu->menu_text[lang][modu_menu->submenu_sel_num],
                     -1, &rcDraw, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
        /*Draw submenu content*/
        else
            DrawText(hdc, sub_menu->sub_menu_text[lang][i - 1],
                     -1, &rcDraw, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
    }

    line_select_pos = sub_menu->sub_sel_num;
    line_pos = (line_select_pos + 1) * KEY_ENTER_BOX_H;
    FillBoxWithBitmap(hdc, KEY_DOWN_LIST_X, KEY_DOWN_LIST_Y + line_pos,
                      KEY_DOWN_LIST_W, KEY_ENTER_BOX_H,
                      &dv_video_all[BOX_HIGHLIGHT_SUB_SEL_IMG_ID]);

    old_text_color = SetTextColor(hdc, COLOR_red);
    rcDraw.top  = KEY_DOWN_LIST_Y + line_pos;
    rcDraw.bottom = rcDraw.top + KEY_ENTER_BOX_H;
    DrawText(hdc, sub_menu->sub_menu_text[lang][line_select_pos],
             -1, &rcDraw, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
    SetTextColor(hdc, old_text_color);
}
