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

#include "dv_debug.h"

#include <stdio.h>
#include <unistd.h>
#include <minigui/gdi.h>
#include <minigui/window.h>

#include "dv_main.h"
#include "dv_play.h"
#include "dv_photo.h"
#include "dv_video.h"
#include "dv_setting.h"
#include "parameter.h"
#include "public_interface.h"
#include "sport_dv_ui.h"
#include "ui_resolution.h"
#include "video.h"
#include "wifi_management.h"
#include "init_hook/init_hook.h"
#include "ueventmonitor/usb_sd_ctrl.h"

#ifdef DV_DEBUG
static int dbg_item_index;  /*item index in current page*/
static int dbg_page_index = 1;
int debug_enter_list;
int debug_flag;
int debug_show_submenu;

/* Just for debug */
int reboot_time;
int recovery_time;
int standby_time;
int modechange_time;
int video_time;
int beg_end_video_time;
int photo_time;
int fwupdate_test_time;

static const char *dv_debug_text[LANGUAGE_NUM][MAX_DV_DEBUG] = {
    {
        "Reboot",
        "recovery",
        "standby",
        "mode change",
        "Video",
        "beg_end_video",
        "photo",
        "temp control",
        "fireware upgrade",
        "effect watermark",
    },
    {
        "Reboot",
        "recovery拷机",
        "二级待机",
        "模式切换",
        "video(回放视频)",
        "开始/结束录像",
        "拍照",
        "温度控制",
        "固件升级",
        "效果参数水印",
    }
};

static const char *debug_switch_text [LANGUAGE_NUM][MAX_DBG_SWITCH_NUM] = {
    {
        "close",
        "open"
    },
    {
        "关闭",
        "开启"
    }
};

static struct debug_item_st debug_list[MAX_DV_DEBUG] = {
    {
        0, 1, 0, 1, MAX_DBG_SWITCH_NUM, "reboot", dv_debug_enter_reboot,
    },
    {
        0, 1, 0, 1, MAX_DBG_SWITCH_NUM, "recovery", dv_debug_enter_recovery,
    },
    {
        0, 1, 0, 1, MAX_DBG_SWITCH_NUM, "standby", dv_debug_enter_standby,
    },
    {
        0, 1, 0, 1, MAX_DBG_SWITCH_NUM, "modechange", dv_debug_enter_mode_change,
    },
    {
        0, 1, 0, 1, MAX_DBG_SWITCH_NUM, "videoplay", dv_debug_enter_videoplayback,
    },
    {
        0, 1, 0, 1, MAX_DBG_SWITCH_NUM, "beg_end_video", dv_debug_enter_vediotest,
    },
    {
        0, 1, 0, 1, MAX_DBG_SWITCH_NUM, "photo", dv_debug_enter_phototest,
    },
    {
        0, 2, 0, 1, MAX_DBG_SWITCH_NUM, "temp_control", dv_debug_enter_temprature,
    },
    {
        0, 2, 0, 0, 0, "fireware upgrade", NULL,
    },
    {
        0, 2, 0, 1, MAX_DBG_SWITCH_NUM, "effect_watermark", dv_debug_enter_effect_watermark,
    },
};

static void draw_debug_select_item(HDC hdc, HWND hWnd, int itemFlag, POINT *point)
{
    int line_pos = 0;

    SetBkColor(hdc, PIXEL_black);
    SetBkColor(hdc, GetWindowElementPixel(hMainWnd, WE_BGC_TOOLTIP));
    SelectFont(hdc, (PLOGFONT)GetWindowElementAttr(hWnd, WE_FONT_MENU));

    line_pos = itemFlag * KEY_ENTER_BOX_H;
    SetTextColor (hdc, PIXEL_lightwhite);
    SetBkMode(hdc, BM_TRANSPARENT);

    FillBoxWithBitmap(hdc, point->x, point->y + line_pos,
                      KEY_ENTER_BOX_W, KEY_ENTER_BOX_H, &dv_video_all[BOX_HIGHLIGHT_SUB_SEL_IMG_ID]);
}

static void draw_debug_menu_item(HDC hdc, int itemPagFlag, int itemCnt,
                                 POINT *point, const char *textArray[LANGUAGE_NUM][itemCnt])
{
    int i;
    int pag = 0;
    RECT rcDraw;
    int lang = (int)parameter_get_video_lan();

    rcDraw.left = point->x;
    rcDraw.right = rcDraw.left + KEY_ENTER_BOX_W;

    if (itemPagFlag == 1)
        pag = 0;

    SetBkMode (hdc, BM_TRANSPARENT);
    SetTextColor (hdc, PIXEL_lightwhite);

    if (itemPagFlag <= 1) {
        for (i = pag; i < (pag + itemCnt); i++) {
            rcDraw.top  = point->y + i * KEY_ENTER_BOX_H;
            rcDraw.bottom = rcDraw.top + KEY_ENTER_BOX_H;

            //printf("---- %s---- \n",textArray[parameter_get_video_lan()][i]);
            DrawText(hdc, textArray[lang][i],
                     -1, &rcDraw, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
        }
    }
}

static void draw_debug_item_common(HDC hdc, HWND hWnd, POINT *point, int dgb_item_id,
                                   int itemCnt, const char *textArray[LANGUAGE_NUM][itemCnt])
{
    struct debug_item_st debug_item = debug_list[dgb_item_id];

    draw_debug_select_item(hdc, hWnd, debug_item.sub_sel_num, point);
    draw_debug_menu_item(hdc, debug_item.sub_menu_page, itemCnt, point, textArray);
}

void dv_debug_enter_reboot(HDC hdc, HWND hWnd)
{
    POINT startPoint;

    startPoint.x = KEY_SUBMENU_TEXT_X;
    startPoint.y = KEY_TEXT_1_Y;
    draw_debug_item_common(hdc, hWnd, &startPoint,
                           DBG_REBOOT, MAX_DBG_SWITCH_NUM, debug_switch_text);
}

void dv_debug_enter_recovery(HDC hdc, HWND hWnd)
{
    POINT startPoint;

    startPoint.x = KEY_SUBMENU_TEXT_X;
    startPoint.y = KEY_TEXT_1_Y;
    draw_debug_item_common(hdc, hWnd, &startPoint,
                           DBG_RECOVERY, MAX_DBG_SWITCH_NUM, debug_switch_text);
}

void dv_debug_enter_standby(HDC hdc, HWND hWnd)
{
    POINT startPoint;

    startPoint.x = KEY_SUBMENU_TEXT_X;
    startPoint.y = KEY_TEXT_1_Y;
    draw_debug_item_common(hdc, hWnd, &startPoint,
                           DBG_STANDBY, MAX_DBG_SWITCH_NUM, debug_switch_text);
}

void dv_debug_enter_mode_change(HDC hdc, HWND hWnd)
{
    POINT startPoint;

    startPoint.x = KEY_SUBMENU_TEXT_X;
    startPoint.y = KEY_TEXT_1_Y;
    draw_debug_item_common(hdc, hWnd, &startPoint,
                           DBG_MODE_CHANGE, MAX_DBG_SWITCH_NUM, debug_switch_text);
}

void dv_debug_enter_videoplayback(HDC hdc, HWND hWnd)
{
    POINT startPoint;

    startPoint.x = KEY_SUBMENU_TEXT_X;
    startPoint.y = KEY_TEXT_1_Y;
    draw_debug_item_common(hdc, hWnd, &startPoint,
                           DBG_VIDEOPLAY, MAX_DBG_SWITCH_NUM, debug_switch_text);
}

void dv_debug_enter_vediotest(HDC hdc, HWND hWnd)
{
    POINT startPoint;

    startPoint.x = KEY_SUBMENU_TEXT_X;
    startPoint.y = KEY_TEXT_1_Y;
    draw_debug_item_common(hdc, hWnd, &startPoint,
                           DBG_BEGIN_END_VIDEO, MAX_DBG_SWITCH_NUM, debug_switch_text);
}

void dv_debug_enter_phototest(HDC hdc, HWND hWnd)
{
    POINT startPoint;

    startPoint.x = KEY_SUBMENU_TEXT_X;
    startPoint.y = KEY_TEXT_1_Y;
    draw_debug_item_common(hdc, hWnd, &startPoint,
                           DBG_PHOTO, MAX_DBG_SWITCH_NUM, debug_switch_text);
}

void dv_debug_enter_temprature(HDC hdc, HWND hWnd)
{
    POINT startPoint;

    startPoint.x = KEY_SUBMENU_TEXT_X;
    startPoint.y = KEY_TEXT_1_Y;
    draw_debug_item_common(hdc, hWnd, &startPoint,
                           DBG_TEMP_CONTROL, MAX_DBG_SWITCH_NUM, debug_switch_text);
}

void dv_debug_enter_effect_watermark(HDC hdc, HWND hWnd)
{
    POINT startPoint;

    startPoint.x = KEY_SUBMENU_TEXT_X;
    startPoint.y = KEY_TEXT_1_Y;
    draw_debug_item_common(hdc, hWnd, &startPoint,
                           DBG_EFFECT_WATERMARK, MAX_DBG_SWITCH_NUM, debug_switch_text);
}

void dv_debug_auto_delete(long long tf_free)
{
    /* free space left smaller than 1024, delete all files in SDCARD */
    if (tf_free >= 0 && tf_free <= 1024) {
        dv_video_proc();
        printf("\t\t NOTICE: Will auto delete all video files in TF card !\n");
        runapp("rm -rf /mnt/sdcard/VIDEO/*");
        runapp("rm -rf /mnt/sdcard/PHOTO/*");
        sync();
        sleep(3);
        dv_video_proc();
    }
}

void draw_list_debug(HWND hWnd, HDC hdc)
{
    char buf[10];
    int i, line_pos = 0, max_debug_page = 0;
    struct debug_item_st *debug_item;
    int lang = (int)parameter_get_video_lan();

    SetBkColor(hdc, GetWindowElementPixel(hMainWnd, WE_BGC_TOOLTIP));
    SetTextColor(hdc, PIXEL_lightwhite);
    SelectFont(hdc, (PLOGFONT)GetWindowElementAttr(hWnd, WE_FONT_MENU));
    SetPenColor(hdc, PIXEL_darkgray);

    max_debug_page = (MAX_DV_DEBUG + MAX_PAGE_LINE_NUM - 1) / MAX_PAGE_LINE_NUM;
    sprintf(buf,"%d/%d", dbg_page_index, max_debug_page);
    TextOut(hdc, DV_VIEW_PAG_X, DV_VIEW_PAG_Y, buf);

    for (i = 0; i < MAX_DV_DEBUG; ) {
        debug_item = &debug_list[i];

        if (debug_item->item_page_num == dbg_page_index) {
            MoveTo(hdc, 0, DV_LIST_SET_IMG_Y + line_pos);
            LineTo(hdc, WIN_W, DV_LIST_SET_IMG_Y + line_pos);

            TextOut(hdc, DV_LIST_BMP_TX_X, DV_LIST_BMP_TX_Y + line_pos, dv_debug_text[lang][i]);

            /* Draw more arrow icon */
            FillBoxWithBitmap(hdc, DV_LIST_BMP_ARR_X, DV_LIST_BMP_ARR_Y + line_pos,
                              DV_LIST_BMP_ARR_W, DV_LIST_BMP_ARR_H,
                              &dv_video_all[SET_ICON_MORE_IMG_ID]);
            line_pos = line_pos + KEY_ENTER_BOX_H;
            i++;
        } else {
            i += MAX_PAGE_LINE_NUM;
            line_pos = 0;
        }
    }

    line_pos = dbg_item_index * KEY_ENTER_BOX_H;

    /* Draw select highlight icon */
    FillBoxWithBitmap(hdc, DV_REC_X, DV_REC_Y + line_pos, DV_REC_W, DV_REC_H,
                      &dv_video_all[BOX_HIGHLIGHT_SEL_IMG_ID]);
    FillBoxWithBitmap(hdc, DV_LIST_BMP_ARR_X, DV_LIST_BMP_ARR_Y + line_pos,
                      DV_LIST_BMP_ARR_W, DV_LIST_BMP_ARR_H,
                      &dv_video_all[SET_ICON_MORE_IMG_ID]);
}

void dv_debug_paint(HDC hdc, HWND hWnd)
{
    int i;
    struct debug_item_st *debug_item;

    for (i = 0; i < MAX_DV_DEBUG; i++) {
        debug_item = &debug_list[i];

        if ((debug_item->item_selected == 1)
            && (debug_item->show_submenu_fuc) != NULL) {
            (*(debug_item->show_submenu_fuc))(hdc, hWnd);
            break;
        }
    }
}

void dv_debug_key_up(HWND hWnd)
{
    int i = 0, max_debug_page = 0;
    struct debug_item_st *debug_item;

    max_debug_page = (MAX_DV_DEBUG + MAX_PAGE_LINE_NUM - 1) / MAX_PAGE_LINE_NUM;

    if (debug_enter_list == 1) {
        for (i = 0; i < MAX_DV_DEBUG; i++) {
            debug_item = &debug_list[i];

            if (debug_item->item_selected == 1) {
                if (debug_item->sub_menu_page <= 1) {
                    if (debug_item->sub_sel_num < debug_item->sub_menu_max - 1)
                        debug_item->sub_sel_num++;
                    else if (debug_item->sub_sel_num == debug_item->sub_menu_max - 1)
                        debug_item->sub_sel_num = 0;
                }
            }
        }
    } else if (debug_enter_list == 0) {
        if (dbg_page_index <= max_debug_page - 1) {
            if (dbg_item_index < MAX_PAGE_LINE_NUM - 1)
                dbg_item_index ++;
            else if (dbg_item_index == MAX_PAGE_LINE_NUM - 1) {
                dbg_item_index = 0;
                dbg_page_index ++;
            }
        } else if (dbg_page_index == max_debug_page) {
            if (dbg_item_index < (MAX_PAGE_LINE_NUM - 1)) {
                int idx = MAX_DV_DEBUG % MAX_PAGE_LINE_NUM;
                dbg_item_index ++;

                if (dbg_item_index == ((idx > 0) ? idx : MAX_PAGE_LINE_NUM - 1)) {
                    dbg_item_index = 0;
                    dbg_page_index = 1;
                }
            } else if (dbg_item_index == (MAX_PAGE_LINE_NUM - 1)) {
                dbg_item_index = 0;
                dbg_page_index = 1;
            }
        }
    }

    InvalidateRect(hWnd, NULL, TRUE);
}

void dv_debug_key_down(HWND hWnd)
{
    int i = 0, max_debug_page = 0;
    struct debug_item_st *debug_item;

    max_debug_page = (MAX_DV_DEBUG + MAX_PAGE_LINE_NUM - 1) / MAX_PAGE_LINE_NUM;

    if (debug_enter_list == 1) {
        for (i = 0; i < MAX_DV_DEBUG; i++) {
            debug_item = &debug_list[i];

            if (debug_item->item_selected == 1) {
                if (debug_item->sub_menu_page == 1) {
                    if (debug_item->sub_sel_num >= 1)
                        debug_item->sub_sel_num--;
                    else if (debug_item->sub_sel_num == 0)
                        debug_item->sub_sel_num = debug_item->sub_menu_max - 1;
                }
            }
        }
    } else if (debug_enter_list == 0) {
        if (dbg_page_index > 1 && dbg_page_index <= max_debug_page) {
            if (dbg_item_index >= 1)
                dbg_item_index --;
            else if (dbg_item_index == 0) {
                dbg_item_index = MAX_PAGE_LINE_NUM - 1;
                dbg_page_index --;
            }
        } else if (dbg_page_index == 1) {
            if (dbg_item_index >= 1) {
                dbg_item_index --;
            } else if (dbg_item_index == 0) {
                int idx = MAX_DV_DEBUG % MAX_PAGE_LINE_NUM;
                dbg_item_index =  (idx > 0) ? (idx - 1) : (MAX_PAGE_LINE_NUM - 1);
                dbg_page_index = max_debug_page;
            }
        }
    }

    InvalidateRect(hWnd, NULL, TRUE);
}

bool dv_debug_is_show_submenu(void)
{
    int sel_func_index = (dbg_page_index - 1) * MAX_PAGE_LINE_NUM + dbg_item_index;
    struct debug_item_st *debug_item = &debug_list[sel_func_index];

    if (debug_item->item_selected)
        return true;
    else
        return false;
}

void dv_debug_key_enter(HWND hWnd)
{
    int sel_func_index = (dbg_page_index - 1) * MAX_PAGE_LINE_NUM + dbg_item_index;
    struct debug_item_st *debug_item = &debug_list[sel_func_index];

    if (debug_enter_list == 0) {
        debug_enter_list = 1;

        if (sel_func_index == DBG_FW_UPGRADE) {
            debug_enter_list = 0;
            printf( "DBG_FW_UPGRADE ============== >>>>>>>  \n");
            dv_fwupgrade_test(hWnd);
            return;
        }

        debug_item->item_selected = 1;
    } else if (debug_enter_list) {
        debug_enter_list = 0;

        /*
        * Before setting the parameters,add judgement the parameter to avoiding
        * to write flash sector much times.
        */
        if (debug_item->item_selected) {
            int submenu_index;
            submenu_index =  debug_item->sub_sel_num;

            switch (sel_func_index) {
            case DBG_REBOOT:
                printf( "DBG_REBOOT ============== >>>>>>>  \n");

                if (parameter_get_debug_reboot() != (char)submenu_index)
                    parameter_save_debug_reboot((char)submenu_index);

                break;

            case DBG_RECOVERY:
                printf( "DBG_RECOVERY ============== >>>>>>>  \n");

                if (parameter_get_debug_recovery() != (char)submenu_index)
                    parameter_save_debug_recovery((char)submenu_index);

                break;

            case DBG_STANDBY:
                printf( "DBG_STANDBY ============== >>>>>>>  \n");

                if (parameter_get_debug_standby() != (char)submenu_index)
                    parameter_save_debug_standby((char)submenu_index);

                break;

            case DBG_MODE_CHANGE:
                printf( "DBG_MODE_CHANGE ============== >>>>>>>  \n");

                if (parameter_get_debug_modechange() != (char)submenu_index)
                    parameter_save_debug_modechange((char)submenu_index);

                break;

            case DBG_VIDEOPLAY:
                printf( "DBG_VIDEOPLAY ============== >>>>>>>  \n");

                if (parameter_get_debug_video() != (char)submenu_index)
                    parameter_save_debug_video((char)submenu_index);

                break;

            case DBG_BEGIN_END_VIDEO:
                printf( "DBG_BEGIN_END_VIDEO ============== >>>>>>>  \n");

                if (parameter_get_debug_beg_end_video() != (char)submenu_index)
                    parameter_save_debug_beg_end_video((char)submenu_index);

                break;

            case DBG_PHOTO:
                printf( "DBG_PHOTO ============== >>>>>>>  \n");

                if (parameter_get_debug_photo() != (char)submenu_index)
                    parameter_save_debug_photo((char)submenu_index);

                break;

            case DBG_TEMP_CONTROL:
                printf( "DBG_TEMP_CONTROL ============== >>>>>>>  \n");

                if (parameter_get_debug_temp() != (char)submenu_index)
                    parameter_save_debug_temp((char)submenu_index);

                break;

            case DBG_EFFECT_WATERMARK:
                printf( "DBG_EFFECT_WATERMARK ============== >>>>>>>  \n");

                if (parameter_get_debug_effect_watermark() != (char)submenu_index)
                    parameter_save_debug_effect_watermark((char)submenu_index);

                break;
            }

            debug_item->item_selected = 0;
        }
    }

    InvalidateRect(hWnd, NULL, TRUE);
}

void dv_debug_key_stdown(void)
{
    int i;
    struct debug_item_st *debug_item;

    if (debug_enter_list == 1)
        debug_enter_list = 0;

    for (i = 0; i < MAX_DV_DEBUG; i++) {
        debug_item = &debug_list[i];

        if (debug_item->item_selected == 1)
            debug_item->item_selected = 0;
    }
}

static int check_fwudpate_test(void)
{
    static int need_check = 1;

    if (need_check) {
        if (parameter_flash_get_flashed() == BOOT_FWUPDATE_TEST_DOING)
            return 1;

        /*
         * If boot mode is not BOOT_FWUPDATE_TEST_DOING when startup,
         * we don't need to check flash flag via kernel ioctl always.
         */
        need_check = 0;
    }

    return 0;
}

void dv_fwupgrade_test(HWND hWnd)
{
    int ret = -1;
    int lang = (int)parameter_get_video_lan();

    if (lang == 0)
        ret = MessageBox(hWnd, "Will start fw_update test?",
                         "FW_UPDATE TEST", MB_YESNO | MB_DEFBUTTON2);
    else if (lang == 1)
        ret = MessageBox(hWnd, "开始固件升级测试？",
                         "固件升级测试", MB_YESNO | MB_DEFBUTTON2);

    if (ret == IDYES) {
        unsigned int fwupdate_cnt = parameter_flash_get_fwupdate_count();
        printf("FW_UPDATE_TEST (%d) cycles start...\n", fwupdate_cnt);

        /* The BOOT_FWUPDATE_TEST_START which is defined in init_hook.h */
        parameter_flash_set_flashed(BOOT_FWUPDATE_TEST_START);
        api_power_reboot();
    }
}

void dv_timer_debug_process(HWND hWnd)
{
    int dv_mode = get_window_mode();

    if (parameter_get_debug_reboot() == 1) {
        reboot_time++;
        printf("--------REBOOT_TIME1-------\n");

        if (reboot_time == REBOOT_TIME) {
            parameter_get_reboot_cnt();
            reboot_time = 0;
            printf("--------REBOOT_TIME-------\n");
            parameter_save_reboot_cnt();
            api_power_reboot();
        }
    }

    if (parameter_get_debug_recovery() == 1) {
        recovery_time++;

        if (recovery_time == RECOVERY_TIME) {
            recovery_time = 0;
            parameter_recover();
            InvalidateRect(hWnd, NULL, TRUE);
            parameter_save_debug_recovery(1);
        }
    }

    if (parameter_get_debug_modechange() == 1) {
        modechange_time++;

        if (dv_setting) {
            dv_setting = 0;
            api_change_mode(MODE_RECORDING);
            InvalidateRect(hWnd, NULL, TRUE);
        } else {
            if (dv_mode == MODE_RECORDING) {
                if (api_video_get_record_state() == VIDEO_STATE_PREVIEW) {
                    if (api_get_sd_mount_state() == SD_STATE_IN)
                        api_start_rec();
                } else if ((api_video_get_record_state() == VIDEO_STATE_RECORD)
                           && modechange_time == MODECHANGE_TIME)
                    api_stop_rec();

                if (modechange_time == MODECHANGE_TIME) {
                    modechange_time = 0;
                    api_change_mode(MODE_PHOTO);
                }
            }

            if (dv_mode == MODE_PHOTO) {
                if (modechange_time == MODECHANGE_TIME) {
                    dv_take_photo(hWnd);
                    modechange_time = 0;
                    //dv_setting = 1;
                    api_change_mode(MODE_RECORDING);
                }
            }
        }
    }

    if (parameter_get_debug_standby()) {
        standby_time++;

        if (standby_time == STANDBY_TIME) {
            runapp("echo +10 > /sys/class/rtc/rtc0/wakealarm");
            api_change_mode(MODE_SUSPEND);
            standby_time = 0;
        }
    }

    if (parameter_get_debug_video() == 1) {
        if (dv_setting)
            api_change_mode(MODE_PREVIEW);
        else {
            if (video_time == 0) {
                if (videopreview_get_state() != 0) {
                    if (parameter_get_video_lan() == 1) {
                        video_time = 0;
                        videopreview_next(hWnd);
                    } else if (parameter_get_video_lan() == 0) {
                        video_time = 0;
                        videopreview_next(hWnd);
                    }
                } else {
                    video_time = 2;
                    videopreview_play(hWnd);
                }
            }

            if (video_time == 1) {
                video_time = 0;
                videopreview_next(hWnd);
            }
        }
    }

    if (parameter_get_debug_beg_end_video() == 1) {
        if (dv_setting) {
            api_change_mode(MODE_RECORDING);
            InvalidateRect(hWnd, NULL, TRUE);
        } else {
            beg_end_video_time++;

            if (beg_end_video_time == 1) {
                if (api_video_get_record_state() == VIDEO_STATE_PREVIEW) {
                    if (api_get_sd_mount_state() == SD_STATE_IN)
                        api_start_rec();
                }
            }

            if (beg_end_video_time == BEG_END_VIDEO_TIME) {
                if (api_video_get_record_state() == VIDEO_STATE_RECORD)
                    api_stop_rec();

                beg_end_video_time = 0;
            }
        }
    }

    if (parameter_get_debug_photo() == 1) {
        if (dv_setting) {
            api_change_mode(MODE_PHOTO);
            InvalidateRect(hWnd, NULL, TRUE);
        } else {
            photo_time++;

            if (photo_time == PHOTO_TIME) {
                dv_take_photo(hWnd);
                photo_time = 0;
            }
        }
    }

    /*
     * FW_UPDATE_TEST automatically
     *
     * Check boot is BOOT_FWUPDATE_TEST_DOING
     */
    if (check_fwudpate_test()) {
        fwupdate_test_time++;

        if (fwupdate_test_time >= FWUPDATE_TEST_TIME) {
            unsigned int fwupdate_cnt = parameter_flash_get_fwupdate_count();

            printf("FW_UPDATE_TEST has passed (%d) cycles, and reboot now\n",
                   fwupdate_cnt);

            /* The BOOT_FWUPDATE_TEST_CONTINUE which is defined in init_hook.h */
            parameter_flash_set_flashed(BOOT_FWUPDATE_TEST_CONTINUE);
            fwupdate_test_time = 0;
            api_power_reboot();
        }
    }
}
#endif
