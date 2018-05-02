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

#include "dv_main.h"

#include <unistd.h>

#include <minigui/control.h>
#include <minigui/gdi.h>
#include <minigui/minigui.h>
#include <minigui/window.h>
#include <minigui/rkfb.h>

#include "display_dv.h"
#include "dv_debug.h"
#include "dv_photo.h"
#include "dv_play.h"
#include "dv_setting.h"
#include "dv_video.h"
#include "gyrosensor.h"
#include "parameter.h"
#include "public_interface.h"
#include "sport_dv_ui.h"
#include "ui_resolution.h"
#include "video.h"
#include "videoplay.h"
#include "watermark.h"
#include "licenseplate.h"
#include "wifi_management.h"

#include "audio/playback/audioplay.h"

#include "fs_manage/fs_sdcard.h"
#include "fs_manage/fs_sdv.h"
#include "fs_manage/fs_storage.h"
#include "ueventmonitor/usb_sd_ctrl.h"

extern char licenseplate_str[20];
extern char licenseplate[MAX_LICN_NUM][3];
extern uint32_t licplate_pos[MAX_LICN_NUM];

extern int   g_dlg_type;
extern void *dlg_content;

static int time_second;
static int dv_func_idx;
static int gyro_calib_result = 1;

HWND hMainWnd;
static HWND hWndUSB;
static DWORD bkcolor;
static int key_lock;
static short screenoff_time;
static int g_shutdown_countdown;
static int battery;
static int SetMode = MODE_RECORDING;
static long long tf_free;
static long long tf_total;
static int play_speed;
static int dv_lowpower;

enum {
    DV_RECORDING_NORMAL,
    DV_RECORDING_LAPSE,
    DV_RECORDING_SLOW
};

enum {
    DV_PHOTO_NORMAL,
    DV_PHOTO_BURST,
    DV_PHOTO_LAPSE
};

static int dv_recording_mode;
static int dv_photo_mode;
static BOOL dv_screen_off;

int dv_show_warning;
int dv_desktop;
int dv_setting;
int dv_show_wifi;
int dv_play_state;
int dv_preview_state;

BITMAP dv_video_all[MAX_ICON_NUM];
struct bitmap watermark_bmap[MAX_WATERMARK_NUM];

int video_lapse_time[VIDEO_LAPSE_TIME_NUM] = {
    LAPSE_1S,
    LAPSE_5S,
    LAPSE_10S,
    LAPSE_30S,
    LAPSE_60S
};

int photo_lapse_time[PHOTO_LAPSE_TIME_NUM] = {
    10,
    20,
    30
};

int photo_resolution[MAX_PHO_RES] = {
    RES_720,
    RES_1080,
    RES_2K,
    RES_4K
};

int photo_burst_num[MAX_BURST_NUM] = {
    BURST_3,
    BURST_4,
    BURST_5
};

static const char *dv_video_img_all[MAX_ICON_NUM] = {
    "icon_video.png",
    "icon_i_video.png",
    //"icon_s_video.png",
    "icon_play.png",
    "icon_camera.png",
    //"icon_i_camera.png",
    "icon_continuous.png",
    "icon_set.png",
    "icon_index_1s.png",
    "icon_index_5s.png",
    "icon_index_10s.png",
    "icon_index_30s.png",
    "icon_index_60s.png",
    "icon_index_battery_01.png",
    "icon_index_battery_02.png",
    "icon_index_battery_03.png",
    "icon_index_battery_04.png",
    "icon_index_battery_05.png",
    "icon_index_battery_06.png",
    "icon_index_battery_07.png",
    "box_highlight_sel.png",
    "box_highlight_sub_sel.png",
    "box_highlight_func.png",
    "thumb_line.bmp",
    "dot.png",
    "dot02.png",
    "bk.png",
    "arrow_ufs.bmp",
    "set_icon_more.png",
    "icon_index_camera.png",
    "icon_index_continuous.png",
    "icon_index_i_camera.png",
    "icon_index_i_video.png",
    "icon_index_s_video.png",
    "icon_index_sound_n.png",
    "icon_index_sound.png",
    "icon_index_video.png",
    "icon_set_02.png",
    "index_box_bg0.png",
    "index_box_bg1.png",
    "icon_index_play.png",
    "icon_index_anti-shake.png",
    "set_icon_ppi.png",
    "set_icon_date.png",
    "set_icon_gyroscope.png",
    "set_icon_language.png",
    "set_icon_light.png",
    "set_icon_mic.png",
    "set_icon_tone_on.png",
    "set_icon_watermark.png",
//    "set_icon_plate.png",
    "set_icon_awb.png",
    "set_icon_recovery.png",
    "set_icon_format.png",
    "set_icon_wifi_ssid.png",
    "set_icon_hz.png",
    "set_icon_screensaver.png",
    "set_icon_video_02.png",
    "set_icon_video_01.png",
    "set_icon_stream.png",
//    "set_icon_car.png",
    "set_icon_upgrade.png",
    "set_icon_vesion.png",
    "set_icon_debug.png",
    "set_icon_photo.png",

    "icon_continuous_3.png",
    "icon_continuous_4.png",
    "icon_continuous_5.png",
    "icon_continuous_6.png",
    "icon_continuous_7.png",
    "icon_continuous_8.png",
    "icon_continuous_9.png",
    "icon_continuous_10.png",
    "icon_index_sd_01.png",
    "icon_index_sd_02.png",
    "icon_72030.png",
    "icon_72060.png",
    "icon_108030.png",
    "icon_108060.png",
    "icon_4K25.png",
    "icon_stop.png",

    "watermark.png",
    "set_icon_video_lapse.png",
    "top_bk.bmp",
    "usb.bmp",
    "charging.bmp",
};

static const char *watermark_img[MAX_WATERMARK_NUM] = {
    "watermark.bmp",
    "watermark_240p.bmp",
    "watermark_360p.bmp",
    "watermark_480p.bmp",
    "watermark_720p.bmp",
    "watermark_1080p.bmp",
    "watermark_1440p.bmp",
    "watermark_4K.bmp"
};

RECT msg_systemTime = {
    DV_TIME_X,
    DV_TIME_Y,
    DV_TIME_X + DV_TIME_W,
    DV_TIME_Y + DV_TIME_H
};

RECT msg_recordTime = {
    DV_REC_TIME_X,
    DV_REC_TIME_Y,
    DV_REC_TIME_X + DV_REC_TIME_W,
    DV_REC_TIME_Y + DV_REC_TIME_H
};

RECT msg_recordImg = {
    DV_REC_IMG_X,
    DV_REC_IMG_Y,
    DV_REC_IMG_X + DV_REC_IMG_W,
    DV_REC_IMG_Y + DV_REC_IMG_H
};

RECT msg_rcMove = {
    MOVE_IMG_X,
    MOVE_IMG_Y,
    MOVE_IMG_X + MOVE_IMG_W,
    MOVE_IMG_Y + MOVE_IMG_H
};

RECT USB_rc = {
    USB_IMG_X,
    USB_IMG_Y,
    USB_IMG_X + USB_IMG_W,
    USB_IMG_Y + USB_IMG_H
};

RECT msg_rcBtt = {
    BATT_IMG_X,
    BATT_IMG_Y,
    BATT_IMG_X + BATT_IMG_W,
    BATT_IMG_Y + BATT_IMG_H
};

RECT msg_rcMode = {
    MODE_IMG_X,
    MODE_IMG_Y,
    MODE_IMG_X + MODE_IMG_W,
    MODE_IMG_Y + MODE_IMG_H
};

RECT msg_rcTime = {
    REC_TIME_X,
    REC_TIME_Y,
    REC_TIME_X + REC_TIME_W,
    REC_TIME_Y + REC_TIME_H
};

RECT msg_rcMic = {
    MIC_IMG_X,
    MIC_IMG_Y,
    MIC_IMG_X + MIC_IMG_W,
    MIC_IMG_Y + MIC_IMG_H
};

RECT msg_rcSDCAP = {
    TFCAP_STR_X,
    TFCAP_STR_Y,
    TFCAP_STR_X + TFCAP_STR_W,
    TFCAP_STR_Y + TFCAP_STR_H
};

RECT msg_rcSYSTIME = {
    SYSTIME_STR_X,
    SYSTIME_STR_Y,
    SYSTIME_STR_X + SYSTIME_STR_W,
    SYSTIME_STR_Y + SYSTIME_STR_H
};

RECT msg_rcFILENAME = {
    FILENAME_STR_X,
    FILENAME_STR_Y,
    FILENAME_STR_X + FILENAME_STR_W,
    FILENAME_STR_Y + FILENAME_STR_H
};

RECT msg_rcWifi = {
    WIFI_IMG_X,
    WIFI_IMG_Y,
    WIFI_IMG_X + WIFI_IMG_W,
    WIFI_IMG_Y + WIFI_IMG_H
};

RECT msg_rcWatermarkTime = {
    WATERMARK_TIME_X,
    WATERMARK_TIME_Y,
    WATERMARK_TIME_X + WATERMARK_TIME_W,
    WATERMARK_TIME_Y + WATERMARK_TIME_H
};

RECT msg_rcWatermarkLicn = {
    WATERMARK_LICN_X,
    WATERMARK_LICN_Y,
    WATERMARK_LICN_X + WATERMARK_LICN_W,
    WATERMARK_LICN_Y + WATERMARK_LICN_H
};

RECT msg_rcWatermarkImg = {
    WATERMARK_IMG_X,
    WATERMARK_IMG_Y,
    WATERMARK_IMG_X + WATERMARK_IMG_W,
    WATERMARK_IMG_Y + WATERMARK_IMG_H
};

static const char *iconlabels[LANGUAGE_NUM][DV_MAIN_FUNC_NUM] = {
    {
        "Video",
        "VideoLapse",
        //"Slow rec",
        "Playback",
        "Photo",
        //"PhotoLapse",
        "BurstMode",
        "Setting",
    },
    {
        "录像",
        "间隔录像",
        //"慢速录像",
        "回放",
        "拍照",
        //"间隔拍照",
        "连拍",
        "设置",
    }
};

const char *sdcard_text[LANGUAGE_NUM][SDCARD_NOTIFY_MAX] = {
    {
        "No SD card insertted!!!",
        "SD card loading failed, \nformat the SD card ?",
        "SD card no enough free space ,\nformat the SD card ?",
        "SD card init fail ,\nformat the SD card ?",
        "SD card is damaged in video, \nwe have repair!",
        "SD card format failed ,\n re-format agagin?",
        "Disk formatting",
        "SD card init fail.",
    },

    {
        "SD卡不存在!!!",
        "SD卡加载失败，\n是否格式化SD卡?",
        "SD卡剩余空间不足\n是否格式化SD卡?",
        "SD卡初始化异常\n是否格式化SD卡?",
        "SD卡中有损坏视频，\n现已修复!",
        "SD卡格式化失败，是否重新格式化?",
        "磁盘格式化",
        "SD卡初始化异常",
    },
};

const char *title_text[LANGUAGE_NUM][TITLE_MAX] = {
    {
        "Warning!!!",
        "Prompt",
        "Firmware Update",
        "Version"
    },
    {
        "警告!!!",
        "提示",
        "固件升级",
        "版本信息"
    },
};

const char *info_text[LANGUAGE_NUM][TEXT_INFO_MAX] = {
    {
        "Are you sure to recovery ?",
        "Setting",
        "Video Lapse",
        "Photo Lapse",
        "Photo Setting",
        "Burst Setting",
        "Gyroscope calibration",
        "Please keep DV \nlevel and still!",
        "Calibration done!",
        "Calibration happen error!",
        "License Plate Setting",
        "Delete this file?",
        "Not found: \n/mnt/sdcard/Firmware.img",
        "Invalid firmware?",
        "Update firmware from \n/mnt/sdcard/Firmware.img?",
        "DEBUG",
    },
    {
        "确定要恢复出厂设置?",
        "设置",
        "间隔录像",
        "间隔拍照",
        "拍照设置",
        "连拍设置",
        "陀螺仪校准",
        "请保持DV \n水平静止放置",
        "校准完成",
        "校准出错",
        "车牌设置",
        "删除此文件?",
        "未找到: \n/mnt/sdcard/Firmware.img",
        "无效的固件?",
        "用卡中新固件\nFirmware.img升级?",
        "功能调试",
    },
};

const char *fw_version[LANGUAGE_NUM][FW_TEXT_MAX] = {
    {
        "FW Version",
    },
    {
        "固件版本",
    },
};

const char *usb_text[LANGUAGE_NUM][USB_TEXT_MAX] = {
    {
        " Disk ",
        " Charging ",
    },
    {
        " 磁  盘 ",
        " 充  电 "
    },
};

static CTRLDATA CtrlBook[] = {
    {
        CTRL_ICONVIEW,
        WS_CHILD | WS_VISIBLE ,
        0, 0, 350, 210,
        IDC_ICONVIEW,
        "iconview",
        0,
        0, NULL, NULL
    }
};

static DLGTEMPLATE DlgIcon = {
    WS_VISIBLE,
    WS_EX_NONE,
    0, 30, 320, 210,
    "DESKTOP",
    0, 0,
    TABLESIZE(CtrlBook), CtrlBook,
    0
};

struct module_submenu_st g_submenu[PHOTO_SUBMENU_NUM] = {
    {
        0,
        0,
        0,
        MAX_PHO_RES,
        "Resoluton Ratio",
        {
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
        },
        show_photo_submenu,
    },
    {
        0,
        0,
        0,
        MAX_BURST_NUM,
        "Burst Number",
        {
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
        },
        show_photo_submenu,
    },
};

struct module_menu_st g_module_menu[MAX_MODULE_MENU] = {
    {
        0,
        0,
        VIDEO_LAPSE_TIME_NUM,
        "Video Lapse",
        {
            {
                "1 s lapse",
                "5 s lapse",
                "10s lapse",
                "30s lapse",
                "60s lapse",
            },
            {
                "间隔 1  秒",
                "间隔 5  秒",
                "间隔 10 秒",
                "间隔 30 秒",
                "间隔 60 秒",
            }
        },
        NULL,
        show_video_lapse_menu,
    },
    {
        0,
        0,
        PHOTO_LAPSE_TIME_NUM,
        "Photo Lapse",
        {
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
        },
        NULL,
        show_photo_lapse_menu,
    },
    {
        0,
        0,
        PHOTO_NORMAL_SETTING_NUM,
        "Photo Normal",
        {
            {
                "Resolution ratio",
            },
            {
                "分辨率",
            },
        },
        g_submenu,
        show_photo_normal_menu,
    },
    {
        0,
        0,
        PHOTO_BURST_SETTING_NUM,
        "Photo Burst",
        {
            {
                "Resolution ratio",
                "Burst Number",
            },
            {
                "分辨率",
                "连拍张数",
            },
        },
        g_submenu,
        show_photo_burst_menu,
    },

    /*Preview file delete menu*/
    {
        0,
        0,
        PREVIEW_DEL_MENU_NUM,
        "Delete File",
        {
            {
                "Yes",
                "No",
            },
            {
                "是",
                "否",
            },
        },
        NULL,
        show_preview_delete_menu,
    },
};

static void dv_low_power_show(HWND hWnd, HDC hdc);

int get_window_mode(void)
{
    return SetMode;
}

int get_key_state(void)
{
    return key_lock;
}

void set_screen_off_time(int time)
{
    screenoff_time = time;
}

static int get_index_from_time_lapse(void)
{
    unsigned short tl_interval = parameter_get_time_lapse_interval();

    switch (tl_interval) {
    case 1:
        return 0;

    case 5:
        return 1;

    case 10:
        return 2;

    case 30:
        return 3;

    case 60:
        return 4;

    default:
        return -1;
    }

    return 0;
}

static void *shut_down_process(void *arg)
{
    while (!charging_status_check() && g_shutdown_countdown--) {
        sleep(1);
        InvalidateRect(hMainWnd, NULL, TRUE);
    }

    if (rkfb_get_screen_state() == 1)
        rkfb_screen_off();

    dv_lowpower = 0;
    api_power_shutdown();
    pthread_exit(NULL);
}

/* SHUTDOWN FUNCTION */
static int shutdown_deinit(HWND hWnd, int wait_time)
{
    pthread_t tid;

    if (!wait_time) {
        printf("\t\t ## Will SHUT DOWN ##\n\n");
        api_power_shutdown();
        return 0;
    }

    g_shutdown_countdown = wait_time;

    if (pthread_create(&tid, NULL, shut_down_process, (void *)hWnd)) {
        printf("Create run_usb_disconnect thread failure\n");
        return -1;
    }

    return 0;
}

static void dv_watermark_show(HDC hdc)
{
    char systime[128] = {0};
    time_t ltime;
    struct tm *today;

    if (parameter_get_video_mark() == 0)
        return;

    time(&ltime);
    today = localtime(&ltime);
    snprintf(systime, sizeof(systime), "%04d-%02d-%02d %02d:%02d:%02d\n",
             1900 + today->tm_year, today->tm_mon + 1, today->tm_mday,
             today->tm_hour, today->tm_min, today->tm_sec);

    SetTextColor(hdc, PIXEL_lightwhite);
    SetBkColor(hdc, bkcolor);
    TextOut(hdc, WATERMARK_TIME_X, WATERMARK_TIME_Y, systime);

#ifdef USE_WATERMARK

    if (SetMode == MODE_RECORDING || SetMode == MODE_PHOTO) {
        uint32_t show_time[MAX_TIME_LEN] = { 0 };

        watermark_get_show_time(show_time, today, MAX_TIME_LEN);
        video_record_update_time(show_time, MAX_TIME_LEN);

        FillBoxWithBitmap(hdc, WATERMARK_IMG_X, WATERMARK_IMG_Y, WATERMARK_IMG_W,
                          WATERMARK_IMG_H, &dv_video_all[WATERMARK_IMG_ID]);

        /* Show license plate */
        if (parameter_get_licence_plate_flag()) {
            if (licenseplate_str[0] == 0) {
                watermart_get_licenseplate_str((char *)parameter_get_licence_plate(),
                                               MAX_LICN_NUM, licenseplate_str);
                get_licenseplate_and_pos((char *)licenseplate, sizeof(licenseplate), licplate_pos);
            }

            if (is_licenseplate_valid((char *)parameter_get_licence_plate()))
                TextOut(hdc, WATERMARK_LICN_X, WATERMARK_LICN_Y, licenseplate_str);

            FillBoxWithBitmap(hdc, WATERMARK_IMG_X, WATERMARK_IMG_Y, WATERMARK_IMG_W,
                              WATERMARK_IMG_H, &dv_video_all[WATERMARK_IMG_ID]);
        }
    }

#endif
}

static void dv_info_dialog_show(HDC hdc, const char *title, const char *content)
{
    RECT rc;

    if (title == NULL || content == NULL)
        return;

    SetBkMode (hdc, BM_TRANSPARENT);
    FillBoxWithBitmap(hdc, INFO_DIALOG_X, INFO_DIALOG_Y,
                      INFO_DIALOG_W, INFO_DIALOG_H,
                      &dv_video_all[INDEX_BOX_BG1_IMG_ID]);
    rc.left = INFO_DIALOG_X;
    rc.top =  INFO_DIALOG_Y;
    rc.right = rc.left + INFO_DIALOG_W;
    rc.bottom = rc.top + INFO_DIALOG_TITLE_H;

    SetTextColor(hdc, COLOR_red);
    DrawText(hdc, title, -1, &rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

    rc.left = INFO_DIALOG_X;
    rc.top =  INFO_DIALOG_Y + INFO_DIALOG_TITLE_H;
    rc.right = rc.left + INFO_DIALOG_W;
    rc.bottom = INFO_DIALOG_Y + INFO_DIALOG_H;
    SetTextColor(hdc, COLOR_lightwhite);
    DrawText(hdc, content, -1, &rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
}

static void myDrawItem(HWND hWnd, GHANDLE hivi, HDC hdc, RECT *rcDraw)
{
    int x, y;
    RECT rcTxt;
    const PBITMAP pbmp = (PBITMAP)iconview_get_item_bitmap(hivi);
    const char *label = (const char *)iconview_get_item_label(hivi);
    const WINDOWINFO *win_info = GetWindowInfo(hWnd);

    SetTextColor(hdc, PIXEL_lightwhite);
    memset(&rcTxt, 0, sizeof(rcTxt));
    SetBkMode(hdc, BM_TRANSPARENT);

    if (iconview_is_item_hilight(hWnd, hivi)) {
        SetTextColor(hdc, GetWindowElementPixel(hWnd, WE_FGC_HIGHLIGHT_ITEM));
        win_info->we_rdr->draw_hilite_item(hWnd, hdc, rcDraw,
                                           GetWindowElementAttr(hWnd, WE_BGC_TOOLTIP));
        FillBoxWithBitmap(hdc, rcDraw->left - 1, rcDraw->top - 1,
                          ICONVIEW_ITEM_W + 2, ICONVIEW_ITEM_H + 2,
                          &dv_video_all[BOX_HIGHLIGHT_FUNC_IMG_ID]);
    } else {
        SetTextColor(hdc, PIXEL_lightwhite);
    }

    SetBkColor(hdc, PIXEL_lightwhite);

    if (label) {
        rcTxt = *rcDraw;
        rcTxt.top = rcTxt.bottom - GetWindowFont(hWnd)->size * 2 - 2;
        rcTxt.bottom = rcTxt.top + ICONVIEW_TEXT_GAP;

        DrawText(hdc, label, -1,
                 &rcTxt, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
    }

    if (pbmp) {
        int bmpw = 0, bmph = 0;

        x = rcDraw->left + (RECTWP(rcDraw) - (int)pbmp->bmWidth ) / 2;

        if (x < rcDraw->left) {
            x = rcDraw->left;
            bmpw = RECTWP(rcDraw);
        }

        y = rcDraw->top
            + ( RECTHP(rcDraw) - RECTH(rcTxt) - (int)pbmp->bmHeight) / 2;

        if (y < rcDraw->top) {
            y = rcDraw->top;
            bmph = RECTHP(rcDraw) - RECTH(rcTxt);
        }

        FillBoxWithBitmap(hdc, x, y, bmpw, bmph, pbmp);
    }

    SetPenColor(hdc, PIXEL_darkgray);
    Rectangle(hdc, rcDraw->left, rcDraw->top,
              rcDraw->left + ICONVIEW_ITEM_W, rcDraw->top + ICONVIEW_ITEM_H);

    dv_low_power_show(hWnd, hdc);
}

static void dv_desktop_keydown(HWND hDlg, int wParam)
{
    HWND hIconView = GetDlgItem(hDlg, IDC_ICONVIEW);
    audio_sync_play(KEYPRESS_SOUND_FILE);

    switch (wParam) {
    case SCANCODE_SHUTDOWN:
        EndDialog(hDlg, wParam);
        time_second = 0;
        dv_desktop = 0;

        if (dv_setting == 0 && dv_func_idx == SYS_SETTING_FUNC)
            api_change_mode(SetMode);

        InvalidateRect(hMainWnd, NULL, TRUE);
        break;

    case SCANCODE_CURSORBLOCKDOWN:
        if (dv_func_idx == SYS_SETTING_FUNC) {
            dv_func_idx = VIDEO_FUNC;
            SendMessage(hIconView, IVM_SETCURSEL, dv_func_idx, 1);
        } else
            dv_func_idx += 1;

        SendMessage(hIconView, IVM_SETCURSEL, dv_func_idx, 1);
        break;

    case SCANCODE_CURSORBLOCKUP:
        if (dv_func_idx == VIDEO_FUNC) {
            dv_func_idx = SYS_SETTING_FUNC;
            SendMessage(hIconView, IVM_SETCURSEL, dv_func_idx, 1);
        } else {
            dv_func_idx -= 1;
            SendMessage(hIconView, IVM_SETCURSEL, dv_func_idx, 1);
        }

        break;

    case SCANCODE_ENTER:
        switch (dv_func_idx) {
        case VIDEO_FUNC:
            EndDialog(hDlg, wParam);
            dv_desktop = 0;
            /* disable lapse recording */
            dv_recording_mode = DV_RECORDING_NORMAL;
            video_record_set_timelapseint(0);

            video_dvs_enable(parameter_get_dvs_enabled());
            parameter_save_video_lapse_state(false);
            api_change_mode(MODE_RECORDING);
            InvalidateRect(hMainWnd, NULL, TRUE);
            break;

        case VIDEO_LAPSE_FUNC:
            EndDialog(hDlg, 0);
            dv_desktop = 0;

            /* Check time interval setting*/
            if (parameter_get_time_lapse_interval() == 0)
                parameter_save_time_lapse_interval(1);

            video_record_set_timelapseint(parameter_get_time_lapse_interval());
            dv_recording_mode = DV_RECORDING_LAPSE;
            video_dvs_enable(0);
            parameter_save_video_lapse_state(true);
            api_change_mode(MODE_RECORDING);
            InvalidateRect(hMainWnd, NULL, TRUE);
            break;

        /*
        case VIDEO_SLOW_FUNC:
            EndDialog(hDlg, 0);
        dv_recording_mode = DV_RECORDING_SLOW;
            api_change_mode(MODE_RECORDING);
            InvalidateRect(hWnd, NULL, TRUE);
            // TODO:
            break;
        */

        case PLAYBACK_FUNC:
            EndDialog(hDlg, 0);
            dv_desktop = 0;
            DBG(" setmode = %d \n", SetMode);
            api_change_mode(MODE_PREVIEW);
            InvalidateRect(hMainWnd, NULL, TRUE);
            break;

        case PHOTO_FUNC:
            EndDialog(hDlg, 0);
            dv_desktop = 0;
            dv_photo_mode = DV_PHOTO_NORMAL;

            video_dvs_enable(0);
            parameter_save_photo_burst_state(false);
            api_change_mode(MODE_PHOTO);

            g_submenu[PHOTO_RESOLUTION].sub_sel_num = get_photo_resolution();
            InvalidateRect(hMainWnd, NULL, TRUE);
            break;

        /*
        case PHOTO_LAPSE_FUNC:
            EndDialog(hDlg, 0);
        dv_photo_mode = DV_PHOTO_LAPSE;
            break;
        */

        case PHOTO_BURST_FUNC:
            EndDialog(hDlg, 0);
            dv_desktop = 0;
            dv_photo_mode = DV_PHOTO_BURST;
            video_dvs_enable(0);
            parameter_save_photo_burst_state(true);
            api_change_mode(MODE_PHOTO);

            g_submenu[PHO_BUR_RESOLUTION].sub_sel_num = get_photo_resolution();
            g_submenu[PHO_BUR_NUM].sub_sel_num = get_dv_photo_burst_num_index();
            InvalidateRect(hMainWnd, NULL, TRUE);
            break;

        case SYS_SETTING_FUNC:
            EndDialog(hDlg, 0);
            dv_desktop = 0;
            dv_setting = 1;

            dv_setting_init();
            InvalidateRect(hMainWnd, NULL, TRUE);
            break;

        default:
            break;
        }
    }
}

static int dv_main_proc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
    HWND hIconView;
    int lang = parameter_get_video_lan();

    switch (message) {
    case MSG_INITDIALOG: {
        IVITEMINFO ivii;
        static int i = 0;
        RECT dlgMarginRc;

        hIconView = GetDlgItem(hDlg, IDC_ICONVIEW);
        dlgMarginRc.left = 1;
        dlgMarginRc.top = 0;
        dlgMarginRc.right = 1;
        dlgMarginRc.bottom = 0;

        SendMessage(hIconView, IVM_SETMARGINS, 0, (LPARAM)&dlgMarginRc);
        SendMessage(hIconView, IVM_SETITEMSIZE, ICONVIEW_ITEM_W, ICONVIEW_ITEM_H);
        SendMessage(hIconView, IVM_SETITEMDRAW, 0, (LPARAM)myDrawItem);

        for (i = 0; i < DV_MAIN_FUNC_NUM; i++) {
            memset (&ivii, 0, sizeof(IVITEMINFO));
            ivii.bmp = &dv_video_all[i + ICON_VIDEO_IMG_ID];
            ivii.nItem = i;
            ivii.label = iconlabels[lang][i];
            ivii.addData = (DWORD)iconlabels[lang][i];
            SendMessage(hIconView, IVM_ADDITEM, 0, (LPARAM)&ivii);
        }

        SendMessage(hIconView, IVM_SETCURSEL, dv_func_idx, 1);
        break;
    }

    case MSG_COMMAND:
        break;

    case MSG_KEYDOWN:
        dv_desktop_keydown(hDlg, wParam);
        break;

    case MSG_KEYLONGPRESS:
        if (key_lock)
            break;

        DBG("%s MSG_KEYLONGPRESS  key = %d\n", __func__, wParam);

        if (wParam == SCANCODE_SHUTDOWN) {
            EndDialog(hDlg, 0);
            shutdown_deinit(hDlg, 0);
            return 0;
        }

        break;

    case MSG_CLOSE:
        EndDialog(hDlg, 0);
        return 0;
    }

    return DefaultDialogProc(hDlg, message, wParam, lParam);
}

static void dv_main_view(void)
{
    DlgIcon.controls = CtrlBook;
    DialogBoxIndirectParam(&DlgIcon, HWND_DESKTOP, dv_main_proc, 0L);
}

static void unloadres(void)
{
    int i;

    for ( i = 0; i < MAX_ICON_NUM; i++)
        UnloadBitmap(&dv_video_all[i]);

    /* unload watermark bmp */
    for (i = 0; i < MAX_WATERMARK_NUM; i++)
        unload_bitmap(&watermark_bmap[i]);
}

static void change_to_recording(void)
{
    dv_desktop = 0;
    dv_setting = 0;

    if (dv_recording_mode != DV_RECORDING_NORMAL) {
        dv_recording_mode = DV_RECORDING_NORMAL;

        if (parameter_get_time_lapse_interval() != 0) {
            parameter_save_time_lapse_interval(0);
            video_record_set_timelapseint(0);
        }
    }

    video_dvs_enable(parameter_get_dvs_enabled());
    api_change_mode(MODE_RECORDING);
}

static void initrec(HWND hWnd)
{
    if (SetMode != MODE_PHOTO) {
        DBG("\t\t### REC mode ###\n\n");
        dv_recording_mode = DV_RECORDING_NORMAL;

        api_video_init(VIDEO_MODE_REC);
    } else {
        DBG("\t\t### PHOTHO mode ###\n\n");
        dv_photo_mode = DV_PHOTO_NORMAL;

        api_video_init(VIDEO_MODE_PHOTO);
    }

    dv_desktop = 0;
    dv_setting = 0;
    InvalidateRect(hWnd, NULL, FALSE);
}

static void ui_deinit(void)
{
    KillTimer(hMainWnd, _ID_TIMER);
    unloadres();
}

/* Just for debug used */
static char *mode_transform_to_str(int mode)
{
    char *mode_str = NULL;

    if (mode < MODE_RECORDING || mode >= MODE_MAX)
        return NULL;

    switch (mode) {
    case MODE_RECORDING:
        mode_str = "VIDEO";
        break;

    case MODE_PHOTO:
        mode_str = "PHOTO";
        break;

    case MODE_PREVIEW:
        mode_str = "PLAY_PREVIEW";
        break;

    case MODE_PLAY:
        mode_str = "PLAYBACK";
        break;

    case MODE_USBDIALOG:
        mode_str = "USBDIALOG";
        break;

    case MODE_USBCONNECTION:
        mode_str = "USBCONNECTION";
        break;

    case MODE_SUSPEND:
        mode_str = "SUSPEND";
        break;

    case MODE_CHARGING:
        mode_str = "CHARGING";
        break;
    }

    return mode_str;
}

static void print_mode_change(int old_mode, int new_mode)
{
    char *oldmode_str;
    char *newmode_str;

    oldmode_str = mode_transform_to_str(old_mode);
    newmode_str = mode_transform_to_str(new_mode);

    printf(" \t\t  %s --->>> %s \n\n", oldmode_str, newmode_str);
}

static void mode_change_notify(HWND hWnd, int premode, int mode)
{
    print_mode_change(premode, mode);

    if (mode == MODE_CHARGING) {
        dv_desktop = 0;
        dv_setting = 0;
    }

    if (premode == MODE_PLAY && mode == MODE_SUSPEND)
        SetMode = MODE_PREVIEW;
    else if (mode != MODE_SUSPEND)
        SetMode = mode;

    InvalidateRect(hWnd, NULL, TRUE);
}

#define TIMER_UPDATE    2000
#define TIMER_CLOSE     2001
static int dv_gyro_show(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    RECT rcDraw;
    int lang = parameter_get_video_lan();

    switch (message) {
    case MSG_INITDIALOG:
        SetWindowBkColor(hDlg, GetWindowElementPixel(hMainWnd, WE_BGC_DESKTOP));
        SetWindowElementAttr(hDlg, WE_BGCA_ACTIVE_CAPTION, COLOR_black);

        SetTimer(hDlg, TIMER_UPDATE, 100);
        return 1;

    case MSG_COMMAND:
        break;

    case MSG_TIMER:
        if (wParam == TIMER_UPDATE)
            InvalidateRect(hDlg, NULL, TRUE);

        if (wParam == TIMER_CLOSE) {
            gyro_calib_result = 1;
            EndDialog(hDlg, 0);
            return 1;
        }

        break;

    case MSG_PAINT:
        hdc = BeginPaint(hDlg);

        FillBoxWithBitmap(hdc, TOPBK_IMG_X, TOPBK_IMG_X, WIN_W - TOPBK_IMG_H * 2,
                          WIN_H - TOPBK_IMG_H * 2, &dv_video_all[TOP_BK_IMG_ID]);

        SetBkColor(hdc, GetWindowElementPixel(hMainWnd, WE_BGC_TOOLTIP));
        SetTextColor(hdc, PIXEL_lightwhite);
        SelectFont(hdc, (PLOGFONT)GetWindowElementAttr(hDlg, WE_FONT_MENU));

        rcDraw.left = 0;
        rcDraw.top = TOPBK_IMG_H;
        rcDraw.right = rcDraw.left + WIN_W - TOPBK_IMG_H * 2;
        rcDraw.bottom = rcDraw.top + TOPBK_IMG_H;
        DrawText(hdc, info_text[lang][TEXT_CALIBRATION_NOTE],
                 -1, &rcDraw, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

        rcDraw.left = 0;
        rcDraw.top = TOPBK_IMG_H + 70;
        rcDraw.right = rcDraw.left + WIN_W - TOPBK_IMG_H * 2;
        rcDraw.bottom = rcDraw.top + TOPBK_IMG_H;

        if (gyro_calib_result == 0)
            DrawText(hdc, info_text[lang][TEXT_CALIBRATION_DONE],
                     -1, &rcDraw, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

        else if (gyro_calib_result < 0)
            DrawText(hdc, info_text[lang][TEXT_CALIBRATION_ERR],
                     -1, &rcDraw, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

        if (gyro_calib_result != 1)
            SetTimer (hDlg, TIMER_CLOSE, 300);

        EndPaint(hDlg, hdc);
        break;

    case MSG_KEYDOWN:
        switch (wParam) {
        case SCANCODE_S:
            break;

        case SCANCODE_ENTER:
            gyro_calib_result = 1;
            EndDialog(hDlg, 0);
            break;
        }

        break;
    }

    return DefaultDialogProc (hDlg, message, wParam, lParam);
}

static void dv_gyro_calibration_win(void)
{
    int lang = parameter_get_video_lan();

    DLGTEMPLATE dlgcalibration = {
        WS_VISIBLE | WS_CAPTION,
        WS_EX_NOCLOSEBOX,
        TOPBK_IMG_H, TOPBK_IMG_H,
        WIN_W - TOPBK_IMG_H * 2, WIN_H - TOPBK_IMG_H * 2,
        info_text[lang][TEXT_CALIBRATION],
        0, 0,
        0, NULL,
        0
    };

    DialogBoxIndirectParam(&dlgcalibration, HWND_DESKTOP, dv_gyro_show, 0L);
}

static void *gyro_calibration_process(void *arg)
{
    dv_gyro_calibration_win();

    pthread_exit(NULL);
}

static int dv_gyro_calibration_proc(HWND hWnd)
{
    pthread_t tid;

    if (pthread_create(&tid, NULL, gyro_calibration_process, (void *)hWnd)) {
        printf("Create dv_gyro_calibration_proc thread failure\n");
        return -1;
    }

    return 0;
}

static void ui_msg_manager_cb(void *msgData, void *msg0, void *msg1)
{
    struct public_message msg;
    memset(&msg, 0, sizeof(msg));

    if (NULL != msgData) {
        msg.id = ((struct public_message *)msgData)->id;
        msg.type = ((struct public_message *)msgData)->type;
    }

    if (msg.type != TYPE_LOCAL && msg.type != TYPE_BROADCAST)
        return;

    switch (msg.id) {
    case MSG_SET_LANGUAGE:
        SetSystemLanguage(hMainWnd, (int)msg0);
        break;

    case MSG_SDCARDFORMAT_START:
        loadingWaitBmp(hMainWnd);
        break;

    case MSG_SDCARDFORMAT_NOTIFY:
        if ((int)msg0 < 0)
            PostMessage(hMainWnd, MSG_SDCARDFORMAT, 0, EVENT_SDCARDFORMAT_FAIL);
        else
            PostMessage(hMainWnd, MSG_SDCARDFORMAT, 0, EVENT_SDCARDFORMAT_FINISH);

        break;

    case MSG_VIDEO_REC_START:
        time_second = 0;

        fs_sdcard_check_capacity(&tf_free, &tf_total, NULL);

        if (SetMode == MODE_RECORDING) {
            InvalidateRect(hMainWnd, &msg_systemTime, TRUE);
            InvalidateRect(hMainWnd, &msg_recordTime, FALSE);
            InvalidateRect(hMainWnd, &msg_recordImg,  FALSE);
        }

        InvalidateRect (hMainWnd, &msg_rcWatermarkTime, TRUE);
        InvalidateRect (hMainWnd, &msg_rcWatermarkLicn, TRUE);
        InvalidateRect (hMainWnd, &msg_rcWatermarkImg,  TRUE);
        break;

    case MSG_VIDEO_REC_STOP:
        time_second = 0;

        if (SetMode == MODE_RECORDING) {
            InvalidateRect(hMainWnd, &msg_systemTime, FALSE);
            InvalidateRect(hMainWnd, &msg_recordTime, TRUE);
            InvalidateRect(hMainWnd, &msg_recordImg,  TRUE);
        }

        InvalidateRect(hMainWnd, &msg_rcWatermarkTime, TRUE);
        InvalidateRect(hMainWnd, &msg_rcWatermarkLicn, TRUE);
        InvalidateRect(hMainWnd, &msg_rcWatermarkImg,  TRUE);
        break;

    case MSG_VIDEO_MIC_ONOFF:
        InvalidateRect(hMainWnd, &msg_rcMic, FALSE);
        break;

    case MSG_SET_PHOTO_RESOLUTION:
        InvalidateRect(hMainWnd, NULL, TRUE);
        break;

    case MSG_VIDEO_SET_ABMODE:
        break;

    case MSG_VIDEO_SET_MOTION_DETE:
        InvalidateRect(hMainWnd, &msg_rcMove, FALSE);
        break;

    case MSG_VIDEO_MARK_ONOFF:
        InvalidateRect(hMainWnd, &msg_rcWatermarkTime, TRUE);
        InvalidateRect(hMainWnd, &msg_rcWatermarkLicn, TRUE);
        InvalidateRect(hMainWnd, &msg_rcWatermarkImg,  TRUE);
        break;

    case MSG_VIDEO_ADAS_ONOFF:
        break;

    case MSG_VIDEO_DVS_ONOFF:
        if (msg0 && api_get_mode() == MODE_RECORDING &&
            dv_recording_mode == DV_RECORDING_NORMAL)
            video_dvs_enable(parameter_get_dvs_enabled());
        else
            video_dvs_enable(0);

        break;

    case MSG_VIDEO_UPDATETIME:
        PostMessage(hMainWnd, MSG_VIDEOREC, (WPARAM)msg0, EVENT_VIDEOREC_UPDATETIME);
        break;

    case MSG_ADAS_UPDATE:
        PostMessage(hMainWnd, MSG_ADAS, (WPARAM)msg0, (LPARAM)msg1);
        break;

    case MSG_VIDEO_PHOTO_END:
        sync();
        SendNotifyMessage(hMainWnd, MSG_PHOTOEND, (WPARAM)msg0, (LPARAM)msg1);
        break;

    case MSG_SDCORD_MOUNT_FAIL:
        PostMessage(hMainWnd, MSG_SDMOUNTFAIL, (WPARAM)msg0, (LPARAM)msg1);
        break;

    case MSG_SDCORD_CHANGE:
        PostMessage(hMainWnd, MSG_SDCHANGE, (WPARAM)msg0, (LPARAM)msg1);
        break;

    case MSG_FS_INITFAIL:
        PostMessage(hMainWnd, MSG_FSINITFAIL, (WPARAM)msg0, (LPARAM)msg1);
        break;

    case MSG_HDMI_EVENT:
        SendMessage(hMainWnd, MSG_HDMI, (WPARAM)msg0, (LPARAM)msg1);
        break;

    case MSG_POWEROFF:
        ui_deinit();
        break;

    case MSG_BATT_UPDATE_CAP:
        PostMessage(hMainWnd, MSG_BATTERY, (WPARAM)msg0, (LPARAM)msg1);
        break;

    case MSG_BATT_LOW:
        dv_lowpower = 1;
        shutdown_deinit(hMainWnd, 10);
        InvalidateRect(hMainWnd, NULL, TRUE);
        break;

    case MSG_BATT_DISCHARGE:
        DBG("%s MSG_BATT_DISCHARGE %d\n", __func__, (unsigned int)msg0);

        if (SetMode == MODE_CHARGING)
            api_power_shutdown();

        /*When remove USB shutdown in Car mode  */
        if (parameter_get_video_carmode() && parameter_get_video_autorec()) {
            if (SetMode == MODE_RECORDING) {
                if (api_video_get_record_state() == VIDEO_STATE_RECORD) {
                    InvalidateRect(hMainWnd, NULL, TRUE);
                    api_power_shutdown();
                }
            }
        }

        PostMessage(hMainWnd, MSG_BATTERY, (WPARAM)msg0, (LPARAM)msg1);
        break;

    case MSG_CAMERE_EVENT:
        PostMessage(hMainWnd, MSG_CAMERA, (WPARAM)msg0, (LPARAM)msg1);
        break;

    case MSG_USB_EVENT:
        PostMessage(hMainWnd, MSG_USBCHAGE, (WPARAM)msg0, (LPARAM)msg1);
        break;

    case MSG_USER_RECORD_RATE_CHANGE:
        break;

    case MSG_USER_MDPROCESSOR:
        break;

    case MSG_USER_MUXER:
        break;

    case MSG_VIDEOPLAY_EXIT:
        PostMessage(hMainWnd, MSG_VIDEOPLAY, (WPARAM)NULL,
                    EVENT_VIDEOPLAY_EXIT);
        break;

    case MSG_VIDEOPLAY_UPDATETIME:
        SendMessage(hMainWnd, MSG_VIDEOPLAY, (WPARAM)msg0,
                    EVENT_VIDEOPLAY_UPDATETIME);
        break;

    case MSG_MODE_CHANGE_NOTIFY:
        mode_change_notify(hMainWnd, (int)msg0, (int)msg1);
        break;

    case MSG_VIDEO_AUTOOFF_SCREEN:
        screenoff_time = (int)msg0;

        if (screenoff_time < 0)
            rkfb_screen_on();

        break;

    case MSG_SET_VIDEO_DVS_ONOFF:
        InvalidateRect(hMainWnd, NULL, TRUE);
        break;

    case MSG_SET_VIDEO_DVS_CALIB:
        dv_gyro_calibration_proc(hMainWnd);
        break;

    case MSG_ACK_VIDEO_DVS_CALIB:
        gyro_calib_result = (WPARAM)msg0;
        break;

    case MSG_MODE_CHANGE_VIDEO_NORMAL_NOTIFY:
        dv_recording_mode = DV_RECORDING_NORMAL;
        video_dvs_enable(parameter_get_dvs_enabled());
        InvalidateRect(hMainWnd, NULL, TRUE);
        break;

    case MSG_MODE_CHANGE_TIME_LAPSE_NOTIFY:
        dv_recording_mode = DV_RECORDING_LAPSE;
        video_dvs_enable(0);
        InvalidateRect(hMainWnd, NULL, TRUE);
        break;

    case MSG_MODE_CHANGE_PHOTO_NORMAL_NOTIFY:
        dv_photo_mode = DV_PHOTO_NORMAL;
        g_submenu[PHOTO_RESOLUTION].sub_sel_num = get_photo_resolution();
        InvalidateRect(hMainWnd, NULL, TRUE);
        break;

    case MSG_MODE_CHANGE_PHOTO_BURST_NOTIFY:
        dv_photo_mode = DV_PHOTO_BURST;
        video_dvs_enable(0);
        g_submenu[PHO_BUR_RESOLUTION].sub_sel_num = get_photo_resolution();
        g_submenu[PHO_BUR_NUM].sub_sel_num = get_dv_photo_burst_num_index();
        InvalidateRect(hMainWnd, NULL, TRUE);
        break;

    case MSG_TAKE_PHOTO_START:
        if (!get_takephoto_state()) {
            set_takephoto_state(true);
            audio_sync_play(CAPTURE_SOUND_FILE);
            loadingWaitBmp(hMainWnd);

            if (video_record_takephoto((unsigned int)msg0) == -1) {
                printf("No input video devices!\n");
                unloadingWaitBmp(hMainWnd);
                set_takephoto_state(false);
            }
        }

        break;

    case MSG_TAKE_PHOTO_WARNING:
        dv_show_warning = 1;
        SetTimerEx(hMainWnd, ID_WARNING_DLG_TIMER, 300, proc_warning_dlg);
        break;

    case MSG_VIDEO_FRONT_REC_RESOLUTION:
        InvalidateRect(hMainWnd, NULL, TRUE);
        break;

    default:
        break;
    }
}

static int UsbSDProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
    HWND hFocus;
    HDC hdc;

    switch (message) {
    case MSG_INITDIALOG:
        hFocus = GetDlgDefPushButton(hDlg);
        hWndUSB = hDlg;

        if (hFocus)
            SetFocus(hFocus);

        SetWindowBkColor(hFocus, COLOR_lightwhite);
        SetWindowBkColor(GetDlgItem(hDlg, USBBAT), COLOR_lightwhite);
        return 0;

    case MSG_PAINT:
        hdc = BeginPaint(hDlg);
        FillBoxWithBitmap(hdc, TOPBK_IMG_X, TOPBK_IMG_X, WIN_W, WIN_H, &dv_video_all[TOP_BK_IMG_ID]);
        EndPaint(hDlg, hdc);
        break;

    case MSG_COMMAND:
        switch (wParam) {
        case IDUSB: {
            int lang = (int)parameter_get_video_lan();
            DBG("  UsbSDProc :case IDUSB:IDUSB  \n");

            if (api_get_sd_mount_state() == SD_STATE_IN) {
                cvr_usb_sd_ctl(USB_CTRL, 1);
                EndDialog(hDlg, wParam);
                api_change_mode(MODE_USBCONNECTION);
            } else if (api_get_sd_mount_state() == SD_STATE_OUT) {
                dlg_content = (void *)sdcard_text[lang][SDCARD_NO_CARD];
                dv_create_message_dialog(title_text[lang][TITLE_WARNING]);
            }

            break;
        }

        case USBBAT:
            DBG("  UsbSDProc :case USBBAT  \n");
            cvr_usb_sd_ctl(USB_CTRL, 0);
            EndDialog(hDlg, wParam);
            hWndUSB = 0;
            change_to_recording();
            break;
        }

        break;

    case MSG_KEYDOWN:
        DBG("  UsbSDProc :MSG_KEYDOWN  \n");
        audio_sync_play(KEYPRESS_SOUND_FILE);
        break;
    }

    return DefaultDialogProc(hDlg, message, wParam, lParam);
}

static int loadres(void)
{
    int i;
    char img[128];
    char respath[] = DV_MAIN_RES_PATH;

    for (i = 0 ; i < MAX_ICON_NUM; i++) {
        snprintf(img, sizeof(img), "%s%s", respath, dv_video_img_all[i]);

        if (LoadBitmap(HDC_SCREEN, &dv_video_all[i], img)) {
            printf("load icon file error, i = %d\n", i);
            return -1;
        }
    }

    /* load watermark bmp */
    for (i = 0; i < MAX_WATERMARK_NUM; i++) {
        snprintf(img, sizeof(img), "%s%s", respath, watermark_img[i]);

        if (load_bitmap(&watermark_bmap[i], img)) {
            printf("load watermark bmp error, i = %d\n", i);
            return -1;
        }
    }

    return 0;
}

void loadingWaitBmp(HWND hWnd)
{
    ANIMATION *anim = CreateAnimationFromGIF89aFile(HDC_SCREEN, WAITING_IMAGE_FILE);

    if (anim == NULL) {
        printf("anim=NULL\n");
        return;
    }

    key_lock = 1;
    SetWindowAdditionalData(hWnd, (DWORD)anim);
    CreateWindow(CTRL_ANIMATION, "",
                 WS_VISIBLE | ANS_AUTOLOOP,
                 IDC_ANIMATION,
                 DV_ANIMATION_X,  DV_ANIMATION_Y,
                 DV_ANIMATION_W, DV_ANIMATION_H,
                 hWnd, (DWORD)anim);
    SendMessage(GetDlgItem(hWnd, IDC_ANIMATION), ANM_STARTPLAY, 0, 0);
}

void unloadingWaitBmp(HWND hWnd)
{
    DWORD win_additionanl_data = GetWindowAdditionalData(hWnd);

    if (win_additionanl_data) {
        DestroyAnimation((ANIMATION *)win_additionanl_data, TRUE);
        DestroyAllControls(hWnd);
    }

    key_lock = 0;
}

BOOL proc_warning_dlg(HWND hWnd, int id, DWORD time)
{
    if (dv_show_warning) {
        dv_show_warning = 0;
        InvalidateRect(hWnd, NULL, TRUE);
        KillTimer(hWnd, id);
        return TRUE;
    }

    return FALSE;
}

static void proc_msg_usbchange(HWND hWnd,
                               WPARAM wParam,
                               LPARAM lParam)
{
    int lang = (int)parameter_get_video_lan();
    DBG(" ### proc_msg_usbchange in\n");
#if USB_MODE
    CTRLDATA UsbCtrl[] = {
        {
            CTRL_BUTTON, WS_VISIBLE | BS_DEFPUSHBUTTON | BS_PUSHBUTTON | WS_TABSTOP,
            90, 80, 100, 40, IDUSB, usb_text[lang][USB_MSC_TEXT], 0, 0, NULL, NULL
        },
        {
            CTRL_BUTTON, WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP, 90, 130, 100, 40,
            USBBAT, usb_text[lang][USB_CHANGRE_TEXT], 0, 0, NULL, NULL
        }
    };

    DLGTEMPLATE UsbDlg = {WS_VISIBLE, WS_EX_NONE, 0, 0, 300,  300,
                          SetDate,    0,          0, 2, NULL, 0
                         };
#endif
    int usb_sd_chage = (int)lParam;

    DBG("MSG_USBCHAGE : usb_sd_chage = ( %d )\n", usb_sd_chage);

    if (usb_sd_chage == 1) {
#if USB_MODE
        api_change_mode(MODE_USBDIALOG);
        UsbDlg.w = g_rcScr.right - g_rcScr.left;
        UsbDlg.h = g_rcScr.bottom - g_rcScr.top;

        UsbCtrl[0].x = (UsbDlg.w - UsbCtrl[0].w) >> 1;
        UsbCtrl[0].y = (UsbDlg.h - 50) >> 1;

        UsbCtrl[1].x = (UsbDlg.w - UsbCtrl[1].w) >> 1;
        UsbCtrl[1].y = (UsbDlg.h + 50) >> 1;

        UsbDlg.controls = UsbCtrl;
        DialogBoxIndirectParam(&UsbDlg, hMainWnd, UsbSDProc, 0L);
#else
        DBG("usb_sd_chage == 1:USB_MODE==0\n");

        if (api_get_sd_mount_state == SD_STATE_IN) {
            cvr_usb_sd_ctl(USB_CTRL, 1);
        } else if (api_get_sd_mount_state == SD_STATE_OUT) {
            dlg_content = (void *)sdcard_text[lang][SDCARD_NO_CARD];
            dv_create_message_dialog(title_text[lang][TITLE_WARNING]);
        }

#endif
    } else if (usb_sd_chage == 0) {
#if USB_MODE

        if (hWndUSB) {
            cvr_usb_sd_ctl(USB_CTRL, 0);
            EndDialog(hWndUSB, wParam);
            InvalidateRect(hWnd, &USB_rc, TRUE);
            hWndUSB = 0;
            change_to_recording();

            if (dv_preview_state == PREVIEW_IN)
                exitvideopreview();
        }

#else
        cvr_usb_sd_ctl(USB_CTRL, 0);
        DBG("usb_sd_chage == 0:USB_MODE==0\n");
#endif
    }
}

static void proc_msg_sdmountfail(HWND hWnd)
{
    int lang = (int)parameter_get_video_lan();

    g_dlg_type = DLG_SD_LOAD_FAIL;
    dlg_content = (void *)sdcard_text[lang][SDCARD_LOAD_FAIL];
    dv_create_select_dialog(title_text[lang][TITLE_WARNING]);
}

static void proc_msg_fs_initfail(HWND hWnd, int param)
{
    int lang = (int)parameter_get_video_lan();

    if (param == -1) {
        /* no space */
        g_dlg_type = DLG_SD_NO_SPACE;
        dlg_content = (void *)sdcard_text[lang][SDCARD_NO_SPACE];
        dv_create_select_dialog(title_text[lang][TITLE_WARNING]);

    } else if (param == -2) {
        /* init fail */
        g_dlg_type = DLG_SD_INIT_FAIL;
        dlg_content = (void *)sdcard_text[lang][SDCARD_INIT_FAIL_ASK];
        dv_create_select_dialog(title_text[lang][TITLE_WARNING]);
    } else {
        /* else error */
        dlg_content = (void *)sdcard_text[lang][SDCARD_INIT_FAIL];
        dv_create_message_dialog(title_text[lang][TITLE_WARNING]);
        return;
    }
}

static void proc_hdmi(HWND hWnd, int lParam)
{
    if (lParam == 1) {
        rk_fb_set_lcd_backlight(0);
        rk_fb_set_out_device(OUT_DEVICE_HDMI);

        if (parameter_get_screenoff_time() > 0 && !dv_screen_off ) {
            api_set_video_autooff_screen(VIDEO_AUTO_OFF_SCREEN_OFF);
            dv_screen_off = TRUE;
        }
    } else {
        rk_fb_set_out_device(OUT_DEVICE_LCD);
        /* Add 500ms delay to avoid display confusion */
        usleep(500000);

        if (dv_screen_off) {
            api_set_video_autooff_screen(VIDEO_AUTO_OFF_SCREEN_ON);
            dv_screen_off = FALSE;
        }

        if (SetMode == MODE_CHARGING)
            screenoff_time = CHARGING_DISPLAY_TIME;
    }

    if (SetMode == MODE_PREVIEW)
        videopreview_refresh(hWnd);
    else if (SetMode == MODE_USBDIALOG) {
        InvalidateRect(hWndUSB, NULL, TRUE);
        return;
    }

    InvalidateRect(hMainWnd, NULL, TRUE);

    if (lParam == 0)
        rk_fb_set_lcd_backlight(parameter_get_video_backlt());
}

static void proc_sdcard_change(HWND hWnd, int lParam)
{
    DBG("MSG_SDCHANGE sdcard state is: %s\n", (api_get_sd_mount_state() == SD_STATE_IN) ? "In" : "Out");
    DBG("SetMode = %d \n", SetMode);

    switch (SetMode) {
    case MODE_RECORDING:
        if (api_video_get_record_state() == VIDEO_STATE_PREVIEW &&
            api_get_sd_mount_state() == SD_STATE_IN &&
            !dv_desktop && !dv_setting) {
            if (parameter_get_video_autorec() && (lParam != SD_STATE_FORMATING))
                api_start_rec();
        }

        InvalidateRect(hWnd, &msg_rcSDCAP, FALSE);
        break;

    case MODE_PLAY:
        videoplay_exit();
        break;

    case MODE_PREVIEW:
        if (api_get_sd_mount_state() == SD_STATE_IN) {
            if (dv_preview_state == PREVIEW_IN)
                exitvideopreview();

            entervideopreview();
            InvalidateRect(hWnd, NULL, FALSE);
        } else {
            exitvideopreview();
            InvalidateRect(hWnd, NULL, FALSE);
        }

        break;

    case MODE_PHOTO:
        InvalidateRect(hWnd, &msg_rcSDCAP, FALSE);
        break;
    }
}

static void proc_battery_show(HWND hWnd, int lParam)
{
    int cap = lParam;

    if (cap > 101 || cap < 0) {
        printf("get batter error happen!!!");
        return;
    }

    switch (cap) {
    case 0 ... 16:
        battery = 0;
        break;

    case 17 ... 22:
        if (battery == 0 || battery == 6)
            battery = 0;

        break;

    case 23 ... 36:
        battery = 1;
        break;

    case 37 ... 42:
        if (battery == 0 || battery == 6)
            battery = 1;

        break;

    case 43 ... 56:
        battery = 2;
        break;

    case 57 ... 62:
        if (battery == 0 || battery == 6)
            battery = 2;

        break;

    case 63 ... 76:
        battery = 3;
        break;

    case 77 ... 82:
        if (battery == 0 || battery == 6)
            battery = 3;

        break;

    case 83 ... 86:
        battery = 4;
        break;

    case 87 ... 92:
        if (battery == 0 || battery == 6)
            battery = 4;

        break;

    case 93 ... 100:
        battery = 5;
        break;

    default:
        battery = 6;
        break;
    }

    DBG("battery level=%d\n", battery);
    InvalidateRect(hWnd, &msg_rcBtt, FALSE);
}

static void dv_low_power_show(HWND hwnd, HDC hdc)
{
    SIZE sz;
    int strwidth;
    int win_w = 0, win_h = 0;
    int x0, y0, w, h;
    char *content = "Low Power!!!";

    if (!dv_lowpower)
        return;

    if (IsControl(hwnd)) {
        RECT rect;
        GetWindowRect(hwnd, &rect);
        win_w = RECTW(rect);
        win_h = RECTH(rect);
    } else if (IsMainWindow(hwnd)) {
        win_w = WIN_W;
        win_h = WIN_H;
    } else {
        printf("--- Error Window hwnd ---\n");
        return;
    }

    strwidth = GetTextExtent(hdc, content, strlen(content), &sz);
    x0 = (win_w - strwidth) / 2 - 2;
    y0 = (win_h - sz.cy - 8) / 2;
    w  = strwidth + 2;
    h  = sz.cy + 8;

    SetBkMode(hdc, BM_TRANSPARENT);
    SetBrushColor(hdc, COLOR_yellow);
    FillBox(hdc, x0, y0, w, h);
    SetTextColor(hdc, COLOR_red);
    TextOut(hdc, x0 + 2, y0 + 4, content);
}

static void proc_video_display(HWND hWnd, HDC hdc)
{
    int video_reso_idx, id = 0;
    char strtime[20];
    char tf_cap[20];

    /* Show top bar */
    FillBoxWithBitmap(hdc, TOPBK_IMG_X, TOPBK_IMG_Y, g_rcScr.right,
                      TOPBK_IMG_H, &dv_video_all[TOP_BK_IMG_ID]);

    sprintf(strtime, "%02ld:%02ld:%02ld", (long int)(time_second / 3600),
            (long int)((time_second / 60) % 60), (long int)(time_second % 60));
    SetBkColor(hdc, bkcolor);

    switch (dv_recording_mode) {
    case DV_RECORDING_NORMAL:
        /* Show normal video icon */
        FillBoxWithBitmap(hdc, DV_INDEX_S_VIDEO_X, DV_INDEX_S_VIDEO_Y,
                          DV_INDEX_S_VIDEO_W, DV_INDEX_S_VIDEO_H,
                          &dv_video_all[ICON_INDEX_VIDEO_IMG_ID]);

        /* Show anti-shake icon */
        if (parameter_get_dvs_enabled() == 1)
            FillBoxWithBitmap(hdc, DV_ICON_ANTI_SHAKE_X, DV_ICON_ANTI_SHAKE_Y,
                              DV_ICON_ANTI_SHAKE_W, DV_ICON_ANTI_SHAKE_H,
                              &dv_video_all[ICON_INDEX_ANTI_SHAKE_IMG_ID]);

        break;

    case DV_RECORDING_LAPSE:
        id = get_index_from_time_lapse();

        if (id < 0)
            return;

        /* Show lapse interval video icon */
        FillBoxWithBitmap(hdc, DV_INDEX_S_VIDEO_X, DV_INDEX_S_VIDEO_Y,
                          DV_INDEX_S_VIDEO_W, DV_INDEX_S_VIDEO_H,
                          &dv_video_all[ICON_INDEX_I_VIDEO_IMG_ID]);

        FillBoxWithBitmap(hdc, DV_ICON_INDEX_TIME_X, DV_ICON_INDEX_TIME_Y,
                          DV_ICON_INDEX_TIME_W, DV_ICON_INDEX_TIME_H,
                          &dv_video_all[ICON_INDEX_1S_IMG_ID + id]);
        break;

    case DV_RECORDING_SLOW:
        /* Show slow video icon */
        FillBoxWithBitmap(hdc, DV_INDEX_S_VIDEO_X, DV_INDEX_S_VIDEO_Y,
                          DV_INDEX_S_VIDEO_W, DV_INDEX_S_VIDEO_H,
                          &dv_video_all[ICON_INDEX_S_VIDEO_IMG_ID]);
        break;
    }

    SetBkColor(hdc, GetWindowElementPixel(hMainWnd, WE_BGC_TOOLTIP));
    SetTextColor(hdc, PIXEL_lightwhite);
    SelectFont(hdc, (PLOGFONT)GetWindowElementAttr(hWnd, WE_FONT_MENU));

    if (api_get_sd_mount_state() == SD_STATE_IN) {
        if (tf_free > 0) {
            sprintf(tf_cap, "%4.1fG", (float)tf_free / 1024);
            TextOut(hdc, TFCAP_STR_X, TFCAP_STR_Y, tf_cap);
        }
    }

    /* Just for debug */
#ifdef DV_DEBUG

    if (tf_total != 0 && parameter_get_debug_auto_delete() == 1)
        dv_debug_auto_delete(tf_free);

#endif

    /* Show dv video resolutions icon */
    video_reso_idx = parameter_get_video_fontcamera();

    switch (video_reso_idx) {
    case R_1080P_30FPS:
        FillBoxWithBitmap(hdc, DV_RESOLUTION_X, DV_RESOLUTION_Y,
                          DV_RESOLUTION_W, DV_RESOLUTION_H,
                          &dv_video_all[ICON_1080P_30FPS]);
        break;

    case R_1080P_60FPS:
        FillBoxWithBitmap(hdc, DV_RESOLUTION_X, DV_RESOLUTION_Y,
                          DV_RESOLUTION_W, DV_RESOLUTION_H,
                          &dv_video_all[ICON_1080P_60FPS]);
        break;

    case R_720P_30FPS:
        FillBoxWithBitmap(hdc, DV_RESOLUTION_X, DV_RESOLUTION_Y,
                          DV_RESOLUTION_W, DV_RESOLUTION_H,
                          &dv_video_all[ICON_720P_30FPS]);
        break;

    case R_720P_60FPS:
        FillBoxWithBitmap(hdc, DV_RESOLUTION_X, DV_RESOLUTION_Y,
                          DV_RESOLUTION_W, DV_RESOLUTION_H,
                          &dv_video_all[ICON_720P_60FPS]);
        break;

    case R_4K_25FPS:
        FillBoxWithBitmap(hdc, DV_RESOLUTION_X, DV_RESOLUTION_Y,
                          DV_RESOLUTION_W, DV_RESOLUTION_H,
                          &dv_video_all[ICON_4K_25FPS]);
        break;
    }

    /* Show SD card icon*/
    if (api_get_sd_mount_state() == SD_STATE_IN)
        FillBoxWithBitmap(hdc, DV_TF_IMG_X, DV_TF_IMG_Y, DV_TF_IMG_W, DV_TF_IMG_H,
                          &dv_video_all[ICON_INDEX_SD_01]);
    else
        FillBoxWithBitmap(hdc, DV_TF_IMG_X, DV_TF_IMG_Y, DV_TF_IMG_W, DV_TF_IMG_H,
                          &dv_video_all[ICON_INDEX_SD_02]);

    /* Show mic icon */
    FillBoxWithBitmap(hdc, DV_INDEX_SOUND_X, DV_INDEX_SOUND_Y,
                      DV_INDEX_SOUND_W, DV_INDEX_SOUND_H,
                      &dv_video_all[ICON_INDEX_SOUND_N_IMG_ID + parameter_get_video_audio()]);

    /* Show battery icon */
    if (g_shutdown_countdown == 0
        || (g_shutdown_countdown !=0 && ((g_shutdown_countdown % 2) != 0)))
        FillBoxWithBitmap(hdc, DV_BATT_IMG_X, DV_BATT_IMG_Y, DV_BATT_IMG_W, DV_BATT_IMG_H,
                          &dv_video_all[battery + ICON_INDEX_BATTERY_01_IMG_ID]);

    SetBkColor(hdc, GetWindowElementPixel(hMainWnd, WE_BGC_TOOLTIP));
    /* Show bottom bar */
    FillBoxWithBitmap(hdc, BOTTOMBK_IMG_X, BOTTOMBK_IMG_Y, g_rcScr.right,
                      BOTTOMBK_IMG_H, &dv_video_all[TOP_BK_IMG_ID]);

    if (api_video_get_record_state() != VIDEO_STATE_RECORD) {
        /* Show date */
        char systime[22];
        time_t ltime;
        struct tm *today;
        RECT rcDraw;

        time(&ltime);
        today = localtime(&ltime);
        sprintf(systime, "%04d-%02d-%02d %02d:%02d:%02d\n",
                1900 + today->tm_year, today->tm_mon + 1, today->tm_mday,
                today->tm_hour, today->tm_min, today->tm_sec);

        rcDraw.left = 0;
        rcDraw.top = WIN_H - TOPBK_IMG_H;
        rcDraw.right = rcDraw.left + WIN_W;
        rcDraw.bottom = rcDraw.top + TOPBK_IMG_H;
        DrawText(hdc, systime, -1, &rcDraw, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
    } else {
        /* recording state */
        int sec_value = 0;

        if (dv_recording_mode == DV_RECORDING_NORMAL) {
            sec_value = time_second;
        } else if (dv_recording_mode == DV_RECORDING_LAPSE) {
            time_t ltime;
            struct tm *today;

            time(&ltime);
            today = localtime(&ltime);
            sec_value = today->tm_sec;
        }

        if (sec_value % 2 == 0)
            FillBoxWithBitmap(hdc, DV_REC_IMG_X, DV_REC_IMG_Y, DV_REC_IMG_W, DV_REC_IMG_H,
                              &dv_video_all[DOT_IMG_ID]);
        else
            FillBoxWithBitmap(hdc, DV_REC_IMG_X, DV_REC_IMG_Y, DV_REC_IMG_W, DV_REC_IMG_H,
                              &dv_video_all[DOT02_IMG_ID]);

        SetTextColor(hdc, PIXEL_red);
        TextOut(hdc, DV_REC_TIME_X, DV_REC_TIME_Y, strtime);

        /* Show waterMark */
        dv_watermark_show(hdc);
    }

    /* No sd card to show warning dialog.*/
    if (dv_show_warning) {
        int lang = (int)parameter_get_video_lan();
        dv_info_dialog_show(hdc, title_text[lang][TITLE_WARNING], sdcard_text[lang][SDCARD_NO_CARD]);
    }

    if (dv_recording_mode == DV_RECORDING_LAPSE) {
        struct module_menu_st menu_lapse = g_module_menu[VIDEO_LAPSE];

        if (menu_lapse.menu_selected == 1 && menu_lapse.show_module_menu_fuc != NULL)
            menu_lapse.show_module_menu_fuc(hdc, hWnd);
    }
}

static void proc_desktop_display(HWND hWnd, HDC hdc)
{
    /* Show top bar */
    FillBoxWithBitmap(hdc, TOPBK_IMG_X, TOPBK_IMG_Y, g_rcScr.right,
                      TOPBK_IMG_H, &dv_video_all[TOP_BK_IMG_ID]);

    SetBkColor(hdc, GetWindowElementPixel(hMainWnd, WE_BGC_TOOLTIP));
    SetTextColor(hdc, PIXEL_lightwhite);
    SelectFont(hdc, (PLOGFONT)GetWindowElementAttr(hWnd, WE_FONT_MENU));
    TextOut(hdc, DV_VIEW_X, DV_VIEW_Y, "SPORT DV");
    TextOut(hdc, DV_VIEW_PAG_X, DV_VIEW_PAG_Y, "1/1");
}

static void photo_submenu_display(HWND hWnd, HDC hdc, int photo_mode)
{
    RECT rcDraw;
    struct module_menu_st menu_photo;
    struct module_submenu_st *submenu_photo;
    int lang = (int)parameter_get_video_lan();
    int photo_reso_idx  = get_photo_resolution();

    SetBkColor(hdc, GetWindowElementPixel(hMainWnd, WE_BGC_TOOLTIP));
    SetTextColor(hdc, PIXEL_lightwhite);
    SelectFont(hdc, (PLOGFONT)GetWindowElementAttr(hWnd, WE_FONT_MENU));

    /* Show submenu if press key DOWN*/
    menu_photo = g_module_menu[photo_mode];
    submenu_photo = menu_photo.submenu + menu_photo.submenu_sel_num;

    /* Show father menu */
    if (submenu_photo->sub_menu_show == 0) {
        if (menu_photo.menu_selected == 1 && menu_photo.show_module_menu_fuc != NULL)
            menu_photo.show_module_menu_fuc(hdc, hWnd);

        /* Show photo resolution ratio */
        rcDraw.left     = TOPBK_IMG_X;
        rcDraw.top      = TOPBK_IMG_Y;
        rcDraw.right    = rcDraw.left + WIN_W;
        rcDraw.bottom   = rcDraw.top + TOPBK_IMG_H;
        DrawText(hdc, g_submenu[PHOTO_RESOLUTION].sub_menu_text[lang][photo_reso_idx],
                 -1, &rcDraw, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

        /* Update photo reso selected when app setting the resolution */
        if (g_submenu[PHOTO_RESOLUTION].sub_sel_num != photo_reso_idx)
            g_submenu[PHOTO_RESOLUTION].sub_sel_num = photo_reso_idx;
    } else {
        /* Show sub menu */
        if (submenu_photo != NULL && submenu_photo->show_module_submenu_fuc != NULL)
            submenu_photo->show_module_submenu_fuc(hdc, hWnd, photo_mode);
    }
}

static void proc_photo_display(HWND hWnd, HDC hdc)
{
    char tf_cap[20];

    /* Show top bar */
    FillBoxWithBitmap(hdc, TOPBK_IMG_X, TOPBK_IMG_Y, g_rcScr.right,
                      TOPBK_IMG_H, &dv_video_all[TOP_BK_IMG_ID]);

    /* Show SD card icon*/
    if (api_get_sd_mount_state() == SD_STATE_IN)
        FillBoxWithBitmap(hdc, DV_TF_IMG_X + 27, DV_TF_IMG_Y, DV_TF_IMG_W, DV_TF_IMG_H,
                          &dv_video_all[ICON_INDEX_SD_01]);
    else
        FillBoxWithBitmap(hdc, DV_TF_IMG_X + 27, DV_TF_IMG_Y, DV_TF_IMG_W, DV_TF_IMG_H,
                          &dv_video_all[ICON_INDEX_SD_02]);

    /* Show battery */
    if ((g_shutdown_countdown == 0)
        || (g_shutdown_countdown !=0 && ((g_shutdown_countdown % 2) != 0)))
        FillBoxWithBitmap(hdc, DV_BATT_IMG_X, DV_BATT_IMG_Y, DV_BATT_IMG_W, DV_BATT_IMG_H,
                          &dv_video_all[battery + ICON_INDEX_BATTERY_01_IMG_ID]);

    switch (dv_photo_mode) {
    case DV_PHOTO_NORMAL:
        /* Show cam icon */
        FillBoxWithBitmap(hdc, DV_CAM_IMG_X, DV_CAM_IMG_Y, DV_CAM_IMG_W, DV_CAM_IMG_H,
                          &dv_video_all[ICON_INDEX_CAMERA_IMG_ID]);
        photo_submenu_display(hWnd, hdc, PHOTO_NORMAL);
        break;

    case DV_PHOTO_BURST:
        /* Show continuous icon */
        FillBoxWithBitmap(hdc, DV_CAM_CONT_IMG_X, DV_CAM_CONT_IMG_Y,
                          DV_CAM_CONT_IMG_W, DV_CAM_CONT_IMG_H,
                          &dv_video_all[ICON_CONTINUOUS_3 + get_dv_photo_burst_num_index()]);
        photo_submenu_display(hWnd, hdc, PHOTO_BURST);
        break;

    case DV_PHOTO_LAPSE:
        /* Show i-cam icon */
        FillBoxWithBitmap(hdc, DV_CAM_IMG_X, DV_CAM_IMG_Y, DV_CAM_IMG_W, DV_CAM_IMG_H,
                          &dv_video_all[ICON_INDEX_I_CAMERA_IMG_ID]);
        photo_submenu_display(hWnd, hdc, PHOTO_LAPSE);
        break;
    }

    if (api_get_sd_mount_state() == SD_STATE_IN) {
        if (tf_free > 0) {
            sprintf(tf_cap, "%4.1fG", (float)tf_free / 1024);
            TextOut(hdc, TFCAP_STR_X + 27, TFCAP_STR_Y, tf_cap);
        }
    }

    /* Just for debug */
#ifdef DV_DEBUG

    if (tf_total != 0 && parameter_get_debug_auto_delete() == 1)
        dv_debug_auto_delete(tf_free);

#endif

    /* Show bottom bar */
    FillBoxWithBitmap(hdc, BOTTOMBK_IMG_X, BOTTOMBK_IMG_Y, g_rcScr.right,
                      BOTTOMBK_IMG_H, &dv_video_all[TOP_BK_IMG_ID]);
    /* Show waterMark */
    dv_watermark_show(hdc);

    if (dv_show_warning) {
        int lang = (int)parameter_get_video_lan();
        dv_info_dialog_show(hdc, title_text[lang][TITLE_WARNING], sdcard_text[lang][SDCARD_NO_CARD]);
    }
}

static void proc_wifi_display(HWND hWnd, HDC hdc)
{
    char ssid[33] = {0};
    char passwd[65] = {0};
    char ssid_str[80] = "SSID:";
    char pwd_str[80] = "PSWD:";
    RECT rc;

    SetBkMode(hdc, BM_TRANSPARENT);
    SetBrushColor (hdc, PIXEL_darkgray);
    FillBox (hdc, 0, 0, 320, 240);
    SetBkColor(hdc, PIXEL_black);
    SetTextColor(hdc, PIXEL_lightwhite);
    SelectFont(hdc, (PLOGFONT)GetWindowElementAttr(hWnd, WE_FONT_MENU));
    parameter_getwifiinfo(ssid, passwd);

    /* SSID */
    rc.left = 0;
    rc.top = DV_WIFI_SSID_Y;
    rc.right = WIN_W;
    rc.bottom = rc.top + 30;
    strcat(ssid_str, ssid);
    DrawText(hdc, ssid_str, -1, &rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

    /* Password */
    rc.top = DV_WIFI_PASS_Y;
    rc.bottom = rc.top + 30;
    strcat(pwd_str, passwd);
    DrawText(hdc, pwd_str, -1, &rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
}

static void proc_setting_display(HWND hWnd, HDC hdc)
{
    int lang = (int)parameter_get_video_lan();

    SetBkMode(hdc, BM_TRANSPARENT);
    SetBrushColor(hdc, RGB2Pixel(hdc, 51, 55, 64));
    FillBox(hdc, 0, TOPBK_IMG_H, g_rcScr.right, g_rcScr.bottom - TOPBK_IMG_H);
    dv_low_power_show(hWnd, hdc);

    if (dv_show_wifi) {
        proc_wifi_display(hWnd, hdc);
        return;
    }

    /* Show top bar */
    FillBoxWithBitmap(hdc, TOPBK_IMG_X, TOPBK_IMG_Y, g_rcScr.right,
                      TOPBK_IMG_H, &dv_video_all[TOP_BK_IMG_ID]);
    /* Show setting icon */
    FillBoxWithBitmap(hdc, DV_ENTER_IMG_X, DV_ENTER_IMG_Y,
                      DV_ENTER_IMG_W, DV_ENTER_IMG_H,
                      &dv_video_all[ICON_SET_02_IMG_ID]);

    SetBkColor(hdc, GetWindowElementPixel(hMainWnd, WE_BGC_TOOLTIP));
    SetTextColor(hdc, PIXEL_lightwhite);
    SelectFont(hdc, (PLOGFONT)GetWindowElementAttr(hWnd, WE_FONT_MENU));

    if (debug_flag) {
        TextOut(hdc, DV_SET_X, DV_SET_Y, info_text[lang][TEXT_DV_DEBUG]);
        /* Draw debug item*/
        draw_list_debug(hWnd, hdc);
    } else {
        TextOut(hdc, DV_SET_X, DV_SET_Y, info_text[lang][TEXT_SETTING]);
        /* Draw main setting item*/
        draw_list_of_setting(hWnd, hdc);
    }

    /* Draw item submenu*/
    if (key_enter_list == 1) {
        /* Draw sunmenu background*/
        SetBkMode(hdc, BM_TRANSPARENT);
        FillBoxWithBitmap(hdc, KEY_ENTER_LIST_X, KEY_ENTER_LIST_Y,
                          KEY_ENTER_LIST_W, KEY_ENTER_LIST_H,
                          &dv_video_all[INDEX_BOX_BG0_IMG_ID]);
        dv_setting_item_paint(hdc, hWnd);
    }

    if (debug_enter_list == 1) {
        /* Draw sunmenu background*/
        SetBkMode(hdc, BM_TRANSPARENT);
        FillBoxWithBitmap(hdc, KEY_ENTER_LIST_X, KEY_ENTER_LIST_Y,
                          KEY_ENTER_LIST_W, KEY_ENTER_LIST_H,
                          &dv_video_all[INDEX_BOX_BG0_IMG_ID]);
        dv_debug_paint(hdc, hWnd);
    }
}

static void proc_play_preview_display(HWND hWnd, HDC hdc)
{
    int x = g_rcScr.left;
    int y = g_rcScr.top;
    int w = g_rcScr.right - g_rcScr.left;
    int h = g_rcScr.bottom - y;

    SetBrushColor(hdc, bkcolor);
    SetBkColor(hdc, bkcolor);
    FillBox(hdc, x, y, w, h);
    SetTextColor(hdc, COLOR_red);
    TextOut(hdc, FILENAME_STR_X, FILENAME_STR_Y, (char *)dv_get_preview_name());

    /* Show play icon */
    if (preview_isvideo)
        FillBoxWithBitmap(hdc, (w - DV_PLAY_IMG_W) / 2, (h - DV_PLAY_IMG_H) / 2,
                          DV_PLAY_IMG_W, DV_PLAY_IMG_H,
                          &dv_video_all[ICON_PLAY_IMG_ID]);

    {
        struct module_menu_st menu_delete = g_module_menu[PREVIEW_DELETE];

        if (menu_delete.menu_selected == 1
            && menu_delete.show_module_menu_fuc != NULL)
            menu_delete.show_module_menu_fuc(hdc, hWnd);
    }
}

static void proc_playback_display(HWND hWnd, HDC hdc)
{
    int x, y, w, h;
    char strtime[20];
    char str_speed[8];

    x = g_rcScr.left;
    y = g_rcScr.top;
    w = g_rcScr.right - g_rcScr.left;
    h = g_rcScr.bottom - g_rcScr.top;

    if (dv_play_state == VIDEO_PAUSE) {
        FillBoxWithBitmap(hdc, (w - DV_STOP_IMG_W) / 2, (h - DV_STOP_IMG_H) / 2,
                          DV_STOP_IMG_W, DV_STOP_IMG_H,
                          &dv_video_all[ICON_STOP_IMG_ID]);
        return;
    }

    if (dv_play_state != VIDEO_PLAYING)
        return;

    SetBrushColor(hdc, bkcolor);
    FillBox(hdc, x, y, w, h);
    sprintf(strtime, "%02ld:%02ld:%02ld", (long int)(time_second / 3600),
            (long int)((time_second / 60) % 60), (long int)(time_second % 60));

    SetBkColor(hdc, bkcolor);
    SetTextColor(hdc, PIXEL_lightwhite);
    SelectFont(hdc, (PLOGFONT)GetWindowElementAttr(hWnd, WE_FONT_MENU));
    TextOut(hdc, REC_TIME_X, REC_TIME_Y, strtime);

    if (play_speed > 1) {
        snprintf(str_speed, sizeof(str_speed), "x %d", play_speed);
        TextOut(hdc, DV_PLAY_SPEED_X, DV_PLAY_SPEED_Y, str_speed);
    }
}

static void proc_usb_conn_display(HWND hWnd, HDC hdc)
{
    FillBoxWithBitmap(hdc, TOPBK_IMG_X, TOPBK_IMG_X, WIN_W, WIN_H,
                      &dv_video_all[BK_IMG_ID]);
    FillBoxWithBitmap(hdc, USB_IMG_X, USB_IMG_Y, USB_IMG_W, USB_IMG_H,
                      &dv_video_all[USB_IMG_ID]);
}

static void proc_charging_display(HWND hWnd, HDC hdc)
{
    FillBoxWithBitmap(hdc, CHARGING_IMG_X, CHARGING_IMG_Y,
                      CHARGING_IMG_W, CHARGING_IMG_H,
                      &dv_video_all[CHARGING_ID]);
}

static void proc_window_paint(HWND hWnd, HDC hdc)
{
    if (dv_desktop) {
        if (SetMode != MODE_USBCONNECTION)
            proc_desktop_display(hWnd, hdc);
        else
            proc_usb_conn_display(hWnd, hdc);

        return;
    }

    if (dv_setting) {
        if (SetMode != MODE_USBCONNECTION)
            proc_setting_display(hWnd, hdc);
        else
            proc_usb_conn_display(hWnd, hdc);

        return;
    }

    switch (SetMode) {
    case MODE_RECORDING:
        proc_video_display(hWnd, hdc);
        break;

    case MODE_PHOTO:
        proc_photo_display(hWnd, hdc);
        break;

    case MODE_PREVIEW:
        proc_play_preview_display(hWnd, hdc);
        break;

    case MODE_PLAY:
        proc_playback_display(hWnd, hdc);
        break;

    case MODE_USBCONNECTION:
        proc_usb_conn_display(hWnd, hdc);
        break;

    case MODE_CHARGING:
        proc_charging_display(hWnd, hdc);
        break;

    default:
        break;
    }

    dv_low_power_show(hWnd, hdc);
}

static void proc_video_keydown(HWND hWnd, int wParam)
{
    struct module_menu_st *menu_lapse;

    switch (wParam) {
    case SCANCODE_MENU:
    case SCANCODE_LEFTALT:
    case SCANCODE_RIGHTALT:
        break;

    case SCANCODE_CURSORBLOCKRIGHT:
        video_record_inc_nv12_raw_cnt();
        break;

    case SCANCODE_CURSORBLOCKUP:
        if (dv_recording_mode == DV_RECORDING_LAPSE) {
            menu_lapse = &g_module_menu[VIDEO_LAPSE];

            if ((menu_lapse->menu_selected) == 1) {
                if ((menu_lapse->submenu_sel_num) >= 1)
                    menu_lapse->submenu_sel_num --;
                else
                    menu_lapse->submenu_sel_num = menu_lapse->submenu_max - 1;
            }

            InvalidateRect(hWnd, NULL, TRUE);
        }

        break;

    case SCANCODE_CURSORBLOCKDOWN:
        if (dv_recording_mode == DV_RECORDING_LAPSE) {
            menu_lapse = &g_module_menu[VIDEO_LAPSE];

            if ((menu_lapse->menu_selected) == 0)
                menu_lapse->menu_selected = 1;
            else {
                menu_lapse->submenu_sel_num ++;

                if (menu_lapse->submenu_sel_num >= menu_lapse->submenu_max)
                    menu_lapse->submenu_sel_num = 0;
            }

            InvalidateRect(hWnd, NULL, TRUE);
        }

        break;

    case SCANCODE_SHUTDOWN:

        /* When recording state, don't respond SHUTDOWN key */
        if ( api_video_get_record_state() != VIDEO_STATE_RECORD ) {
            if (dv_recording_mode == DV_RECORDING_LAPSE) {
                menu_lapse = &g_module_menu[VIDEO_LAPSE];

                if (menu_lapse->menu_selected) {
                    menu_lapse->menu_selected = 0;
                    InvalidateRect(hWnd, NULL, TRUE);
                    return;
                }
            }

            dv_desktop = 1;
            InvalidateRect(hWnd, NULL, TRUE);
            dv_main_view();
        }

        break;

    case SCANCODE_ENTER:
        if (dv_show_warning) {
            audio_sync_play(KEYPRESS_SOUND_FILE);
            proc_warning_dlg(hWnd, ID_WARNING_DLG_TIMER, 0);
            break;
        }

        if (api_get_sd_mount_state() == SD_STATE_IN) {
            if (dv_recording_mode == DV_RECORDING_NORMAL)
                dv_video_proc();

            if (dv_recording_mode == DV_RECORDING_LAPSE)
                dv_video_lapse_proc(hWnd);
        } else {
            dv_show_warning = 1;
            audio_sync_play(KEYPRESS_SOUND_FILE);
            SetTimerEx(hWnd, ID_WARNING_DLG_TIMER, 300, proc_warning_dlg);
        }

        InvalidateRect(hWnd, NULL, TRUE);
        break;
    }
}

static void proc_photo_keydown(HWND hWnd, int wParam)
{
    struct module_menu_st *menu_photo = NULL;
    struct module_submenu_st *submenu_pho = NULL;

    if (dv_photo_mode == DV_PHOTO_NORMAL)
        menu_photo = &g_module_menu[PHOTO_NORMAL];
    else if (dv_photo_mode == DV_PHOTO_BURST)
        menu_photo = &g_module_menu[PHOTO_BURST];
    else if (dv_photo_mode == DV_PHOTO_LAPSE)
        menu_photo = &g_module_menu[PHOTO_LAPSE];

    submenu_pho = menu_photo->submenu + menu_photo->submenu_sel_num;

    switch (wParam) {
    case SCANCODE_MENU:
    case SCANCODE_LEFTALT:
    case SCANCODE_RIGHTALT:
        break;

    case SCANCODE_CURSORBLOCKUP:
        if (submenu_pho->sub_menu_show == 0) {
            if (menu_photo->menu_selected == 1) {
                if (menu_photo->submenu_sel_num >= 1)
                    menu_photo->submenu_sel_num --;
                else
                    menu_photo->submenu_sel_num = menu_photo->submenu_max - 1;
            }
        } else {
            if (submenu_pho->sub_sel_num >= 1)
                submenu_pho->sub_sel_num --;
            else
                submenu_pho->sub_sel_num = submenu_pho->sub_menu_max - 1;
        }

        InvalidateRect(hWnd, NULL, TRUE);
        break;

    case SCANCODE_CURSORBLOCKDOWN:
        if (submenu_pho->sub_menu_show == 0) {
            if ((menu_photo->menu_selected) == 0) {
                menu_photo->menu_selected = 1;
            } else {
                menu_photo->submenu_sel_num ++;

                if (menu_photo->submenu_sel_num >= menu_photo->submenu_max)
                    menu_photo->submenu_sel_num = 0;
            }
        } else {
            submenu_pho->sub_sel_num ++;

            if (submenu_pho->sub_sel_num >= submenu_pho->sub_menu_max)
                submenu_pho->sub_sel_num = 0;
        }

        InvalidateRect(hWnd, NULL, TRUE);
        break;

    case SCANCODE_SHUTDOWN:
        if (menu_photo->menu_selected) {
            /* if showed submenu ,destroy submenu */
            if (submenu_pho->sub_menu_show == 1)
                submenu_pho->sub_menu_show = 0;

            /* if showed menu ,destroy menu */
            else
                menu_photo->menu_selected = 0;

            InvalidateRect(hWnd, NULL, TRUE);
        } else {
            dv_desktop = 1;

            InvalidateRect(hWnd, NULL, TRUE);
            dv_main_view();
        }

        break;

    case SCANCODE_ENTER:
        if (dv_show_warning) {
            audio_sync_play(KEYPRESS_SOUND_FILE);
            proc_warning_dlg(hWnd, ID_WARNING_DLG_TIMER, 0);
            break;
        }

        if (dv_photo_mode == DV_PHOTO_NORMAL)
            dv_photo_proc(hWnd);
        else if (dv_photo_mode == DV_PHOTO_BURST)
            dv_photo_burst_proc(hWnd);
        else if (dv_photo_mode == DV_PHOTO_LAPSE)
            dv_photo_lapse_proc(hWnd);

        InvalidateRect(hWnd, NULL, TRUE);
        break;
    }
}

static void proc_wifi_keydown(HWND hWnd, int wParam)
{
    switch (wParam) {
    case SCANCODE_SHUTDOWN:
        dv_setting = 1;
        dv_show_wifi = 0;
        InvalidateRect(hWnd, NULL, TRUE);
        break;

    case SCANCODE_CURSORBLOCKUP:
        break;

    case SCANCODE_CURSORBLOCKDOWN:
        break;

    case SCANCODE_ENTER:
        dv_setting = 1;
        dv_show_wifi = 0;
        InvalidateRect(hWnd, NULL, TRUE);
        break;
    }
}

static void proc_setting_keydown(HWND hWnd, int wParam)
{
    if (dv_show_wifi) {
        proc_wifi_keydown(hWnd, wParam);
        return;
    }

    switch (wParam) {
    case SCANCODE_SHUTDOWN:
        if (dv_setting_is_show_submenu()) {
            dv_setting_key_stdown();
            InvalidateRect(hWnd, NULL, TRUE);
            return;
        } else {
            dv_desktop = 1;
            dv_setting = 0;
            InvalidateRect(hWnd, NULL, TRUE);
            dv_main_view();
        }

        break;

    case SCANCODE_CURSORBLOCKDOWN:
        dv_setting_key_up(hWnd);
        break;

    case SCANCODE_CURSORBLOCKUP:
        dv_setting_key_down(hWnd);
        break;

    case SCANCODE_ENTER:
        dv_setting_key_enter(hWnd);
        break;
    }
}

static void proc_playpreview_keyup(HWND hWnd, int wParam)
{
    struct module_menu_st *menu_delete = &g_module_menu[PREVIEW_DELETE];

    switch (wParam) {
    case SCANCODE_CURSORBLOCKUP:
    case SCANCODE_CURSORBLOCKRIGHT:
        if ((menu_delete->menu_selected) == 0)
            videopreview_next(hWnd);

        break;

    case SCANCODE_CURSORBLOCKDOWN:
    case SCANCODE_CURSORBLOCKLEFT:
        if ((menu_delete->menu_selected) == 0)
            videopreview_pre(hWnd);

        break;

    default:
        break;
    }
}

static void proc_playpreview_keydown(HWND hWnd, int wParam)
{
    struct module_menu_st *menu_delete = &g_module_menu[PREVIEW_DELETE];

    switch (wParam) {
    case SCANCODE_SHUTDOWN:
        dv_desktop = 1;
        InvalidateRect(hMainWnd, NULL, TRUE);
        dv_main_view();
        break;

    case SCANCODE_CURSORBLOCKUP:
    case SCANCODE_CURSORBLOCKRIGHT:
        if ((menu_delete->menu_selected) == 1) {
            if ((menu_delete->submenu_sel_num) >= 1)
                menu_delete->submenu_sel_num --;
            else
                menu_delete->submenu_sel_num = PREVIEW_DEL_MENU_NUM - 1;

            InvalidateRect(hWnd, NULL, TRUE);
        }

        break;

    case SCANCODE_CURSORBLOCKDOWN:
    case SCANCODE_CURSORBLOCKLEFT:
        if ((menu_delete->menu_selected) == 1) {
            menu_delete->submenu_sel_num ++;

            if (menu_delete->submenu_sel_num >= menu_delete->submenu_max)
                menu_delete->submenu_sel_num = 0;

            InvalidateRect(hWnd, NULL, TRUE);
        }

        break;

    case SCANCODE_ENTER:
        dv_preview_proc(hWnd);
        break;

    case SCANCODE_MENU:
        break;
    }
}

static void proc_playback_keydown(HWND hWnd, int wParam)
{
    switch (wParam) {
    case SCANCODE_SHUTDOWN:
        if (dv_play_state != VIDEO_PLAYING) {
            if (dv_play_state == VIDEO_PAUSE)
                exitplayvideo(hWnd);

            dv_desktop = 1;
            InvalidateRect(hWnd, NULL, TRUE);
            dv_main_view();
        }

        break;

    case SCANCODE_SPACE:
    case SCANCODE_ENTER:
        // do pause
        videoplay_pause();

        if (dv_play_state == VIDEO_PLAYING)
            dv_play_state = VIDEO_PAUSE;
        else if (dv_play_state == VIDEO_PAUSE)
            dv_play_state = VIDEO_PLAYING;

        break;

    case SCANCODE_S:
        //videoplay_step_play();
        break;

    case SCANCODE_CURSORBLOCKUP:
        if (dv_play_state == VIDEO_PAUSE)
            exitplayvideo(hWnd);
        else
            play_speed = videoplay_set_speed(1);

        break;

    case SCANCODE_CURSORBLOCKDOWN:
        if (dv_play_state == VIDEO_PAUSE)
            exitplayvideo(hWnd);
        else
            play_speed = videoplay_set_speed(-1);

        break;

    default:
        printf("scancode: %d\n", wParam);
    }

    InvalidateRect(hWnd, NULL, TRUE);
}

static void proc_charging_keydown(HWND hWnd, int wParam)
{
    if (wParam == SCANCODE_SHUTDOWN) {
        screenoff_time = parameter_get_screenoff_time();
        api_change_mode(MODE_RECORDING);
    } else {
        screenoff_time = CHARGING_DISPLAY_TIME;
    }
}

void proc_debug_keydown(HWND hWnd, int wParam)
{
    switch (wParam) {
    case SCANCODE_SHUTDOWN:
        if (dv_debug_is_show_submenu()) {
            dv_debug_key_stdown();
        } else {
            dv_desktop = 0;

            if (debug_flag)
                debug_flag = 0;
        }

        InvalidateRect(hWnd, NULL, TRUE);
        break;

    case SCANCODE_CURSORBLOCKDOWN:
        dv_debug_key_up(hWnd);
        break;

    case SCANCODE_CURSORBLOCKUP:
        dv_debug_key_down(hWnd);
        break;

    case SCANCODE_ENTER:
        dv_debug_key_enter(hWnd);
        break;
    }
}

static void proc_dv_keyup(HWND hWnd, int wParam)
{
    if (dv_desktop || dv_setting)
        return;

    if (SetMode == MODE_PREVIEW)
        proc_playpreview_keyup(hWnd, wParam);
}

static void proc_dv_keydown(HWND hWnd, int wParam)
{
    if (wParam != SCANCODE_ENTER) {
        /* FIXME: #134474, do not play key tone during audio playing */
        if (SetMode != MODE_PLAY)
            audio_sync_play(KEYPRESS_SOUND_FILE);
    } else {
        if (SetMode != MODE_PHOTO && SetMode != MODE_RECORDING)
            audio_sync_play(KEYPRESS_SOUND_FILE);
        else {
            /* Play key tone when in photo menu or setting menu */
            if (g_module_menu[PHOTO_NORMAL].menu_selected == 1
                || g_module_menu[PHOTO_BURST].menu_selected == 1
                || g_module_menu[PHOTO_LAPSE].menu_selected == 1
                || dv_setting == 1)
                audio_sync_play(KEYPRESS_SOUND_FILE);
        }
    }

    if (dv_desktop) {
        if (wParam == SCANCODE_SHUTDOWN && SetMode != MODE_USBCONNECTION)
            change_to_recording();

        return;
    }

    if (dv_setting) {
        if (SetMode == MODE_USBCONNECTION)
            return;

        if (!debug_flag)
            proc_setting_keydown(hWnd, wParam);

#ifdef DV_DEBUG
        else
            proc_debug_keydown(hWnd, wParam);

#endif
        return;
    }

    switch (SetMode) {
    case MODE_RECORDING:
        proc_video_keydown(hWnd, wParam);
        break;

    case MODE_PHOTO:
        proc_photo_keydown(hWnd, wParam);
        break;

    case MODE_PREVIEW:
        proc_playpreview_keydown(hWnd, wParam);
        break;

    case MODE_PLAY:
        proc_playback_keydown(hWnd, wParam);
        break;

    case MODE_CHARGING:
        proc_charging_keydown(hWnd, wParam);
        break;

    default:
        break;
    }
}

static int SportDVWinProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    int lang = (int)parameter_get_video_lan();

    switch (message) {
    case MSG_CREATE:
        SetTimer(hWnd, _ID_TIMER, 100);
        /* 3s to get tf card capacity */
        SetTimer(hWnd, ID_TF_CAPACITY_TIMER, 300);
        break;

    case MSG_HDMI:
        proc_hdmi(hWnd, lParam);
        break;

    case MSG_REPAIR:
        dlg_content = (void *)sdcard_text[lang][SDCARD_VIDEO_DAMAGE];
        dv_create_message_dialog(title_text[lang][TITLE_WARNING]);
        break;

    case MSG_FSINITFAIL:
        proc_msg_fs_initfail(hWnd, -1);
        break;

    case MSG_PHOTOEND:
        if (lParam == 1 && get_takephoto_state()) {
            unloadingWaitBmp(hWnd);
            set_takephoto_state(false);
        }

        break;

    case MSG_ADAS:
        break;

    case MSG_SDMOUNTFAIL:
        proc_msg_sdmountfail(hWnd);
        break;

    case MSG_SDCARDFORMAT:
        unloadingWaitBmp(hWnd);

        if (lParam == EVENT_SDCARDFORMAT_FINISH) {
            printf("sd card format finish\n");
        } else if (lParam == EVENT_SDCARDFORMAT_FAIL) {
            g_dlg_type = DLG_SD_FORMAT_FAIL;
            dlg_content = (void *)sdcard_text[lang][SDCARD_FORMAT_FAIL];
            dv_create_select_dialog(title_text[lang][TITLE_WARNING]);
        }

        break;

    case MSG_VIDEOPLAY:
        if (lParam == EVENT_VIDEOPLAY_EXIT) {
            exitplayvideo(hWnd);

            if (play_speed > 1)
                play_speed = 1;
        } else if (lParam == EVENT_VIDEOPLAY_UPDATETIME) {
            time_second = (int)wParam;
            InvalidateRect(hWnd, &msg_rcTime, FALSE);
        }

#ifdef DV_DEBUG

        //just for debug
        if (parameter_get_debug_video() == 1)
            video_time = 1;
        else if (parameter_get_debug_video() == 0)
            video_time = 0;

#endif
        break;

    case MSG_VIDEOREC:
        if (lParam == EVENT_VIDEOREC_UPDATETIME)
            time_second = (int)wParam;

        break;

    case MSG_CAMERA:
        break;

    case MSG_ATTACH_USER_MUXER:
        video_record_attach_user_muxer((void *)wParam, (void *)lParam, 1);
        break;

    case MSG_ATTACH_USER_MDPROCESSOR:
        video_record_attach_user_mdprocessor((void *)wParam, (void *)lParam);
        break;

    case MSG_COLLISION:
        if (SetMode == MODE_RECORDING
            && api_get_sd_mount_state() == SD_STATE_IN)
            video_record_savefile();

        break;

    case MSG_FILTER:
        break;

    case MSG_USBCHAGE:
        if (parameter_get_video_carmode() && parameter_get_video_autorec()) {
            /* Means USB is connected */
            if (lParam == 1) {
                cvr_usb_sd_ctl(USB_CTRL, 0);
                change_to_recording();

                if ((api_video_get_record_state() == VIDEO_STATE_PREVIEW)
                    && (api_get_sd_mount_state() == SD_STATE_IN)
                    && !dv_desktop
                    && !dv_setting)
                    api_start_rec();
            }

            break;
        }

        if (SetMode != MODE_CHARGING)
            proc_msg_usbchange(hWnd, wParam, lParam);

        break;

    case MSG_SDCHANGE:
        proc_sdcard_change(hWnd, lParam);
        break;

    case MSG_BATTERY:
        proc_battery_show(hWnd, lParam);
        break;

    case MSG_TIMER:
        if (wParam == _ID_TIMER) {
            if (screenoff_time > 0) {
                screenoff_time--;

                if (dv_play_state == VIDEO_PLAYING)
                    screenoff_time = parameter_get_screenoff_time();

                if (screenoff_time == 0) {
                    /* If HDMI connected,dot't shutoff screen, nothing todo */
                    if (rk_fb_get_hdmi_connect() != 1)
                        rkfb_screen_off();
                }
            }

            /* Gparking_gs_active (0: gsensor negative 1: gsneosr active) */
            if (dv_desktop == 0) {
                if (SetMode == MODE_RECORDING) {
                    InvalidateRect(hWnd, &msg_systemTime, FALSE);
                    InvalidateRect(hWnd, &msg_recordTime, FALSE);
                    InvalidateRect(hWnd, &msg_recordImg,  FALSE);
                }

                if (parameter_get_video_mark()) {
                    InvalidateRect (hWnd, &msg_rcWatermarkTime, TRUE); //time
                    InvalidateRect (hWnd, &msg_rcWatermarkLicn, FALSE); //license plate
                    InvalidateRect (hWnd, &msg_rcWatermarkImg, FALSE); //logo
                }

#ifdef DV_DEBUG
                /* Just for DEBUG USE*/
                dv_timer_debug_process(hWnd);
#endif
            }
        }

        if (wParam == ID_TF_CAPACITY_TIMER) {
            if (dv_desktop == 0) {
                if (SetMode >= MODE_RECORDING && SetMode < MODE_PREVIEW) {
                    if (api_get_sd_mount_state() == SD_STATE_IN) {
                        fs_sdcard_check_capacity(&tf_free, &tf_total, NULL);
                        InvalidateRect(hWnd, &msg_rcSDCAP,  FALSE);
                    } else {
                        tf_free = 0;
                        tf_total = 0;
                        InvalidateRect(hWnd, &msg_rcSDCAP, FALSE);
                    }
                }
            }
        }

        break;

    case MSG_PAINT:
        hdc = BeginPaint(hWnd);
        proc_window_paint(hWnd, hdc);
        EndPaint(hWnd, hdc);
        break;

    case MSG_COMMAND:
        break;

    case MSG_ACTIVEMENU:
        break;

    case MSG_CLOSE:
        api_video_deinit(true);
        KillTimer(hWnd, _ID_TIMER);
        DestroyAllControls(hWnd);
        DestroyMainWindow(hWnd);
        PostQuitMessage(hWnd);
        return 0;

    case MSG_KEYDOWN:
        if (key_lock)
            break;

        DBG("MSG_KEYDOWN SetMode = %d, key = %d\n", SetMode, wParam);
        proc_dv_keydown(hWnd, wParam);
        break;

    case MSG_KEYUP:
        if (key_lock)
            break;

        DBG("MSG_KEYUP SetMode = %d, key = %d\n", SetMode, wParam);
        proc_dv_keyup(hWnd, wParam);
        break;

    case MSG_KEYLONGPRESS:
        if (key_lock)
            break;

        DBG("MSG_KEYLONGPRESS  key = %d\n", wParam);

        if (wParam == SCANCODE_SHUTDOWN)
            shutdown_deinit(hWnd, 0);

        if (wParam == SCANCODE_CURSORBLOCKUP)
            if (MODE_PREVIEW == SetMode) {
                /* Because respond to KEYDOWN ,so previous the video to delete. */
                //videopreview_pre(hWnd);
                struct module_menu_st *menu_delete;
                menu_delete = &g_module_menu[PREVIEW_DELETE];

                if ((menu_delete->menu_selected) == 0) {
                    menu_delete->menu_selected = 1;
                    InvalidateRect(hWnd, NULL, TRUE);
                }
            }

        break;

    case MSG_KEYALWAYSPRESS:
        break;

    case MSG_USB_DISCONNECT:
        break;

    case MSG_MAINWIN_KEYDOWN:
        DBG("MSG_MAINWIN_KEYDOWN SetMode = %d, key = %d\n", SetMode, wParam);

        if (SetMode == MODE_CHARGING)
            screenoff_time = CHARGING_DISPLAY_TIME;
        else
            screenoff_time = parameter_get_screenoff_time();

        if (rkfb_get_screen_state() == 0)
            rkfb_screen_on();

        if (screenoff_time == 0 && SetMode == MODE_PREVIEW)
            videopreview_refresh(hWnd);

        break;

    case MSG_MAINWIN_KEYUP:
        break;

    case MSG_MAINWIN_KEYLONGPRESS:
        break;

    case MSG_MAINWIN_KEYALWAYSPRESS:
        break;
    }

    return DefaultMainWinProc(hWnd, message, wParam, lParam);
}

int MiniGUIMain(int argc, const char *argv[])
{
    MSG Msg;
    MAINWINCREATE CreateInfo;
    HDC sndHdc;
    struct color_key key;

    set_display_dv_window();

    if (loadres()) {
        printf("loadres fail\n");
        return -1;
    }

    memset(&CreateInfo, 0, sizeof(MAINWINCREATE));
    CreateInfo.dwStyle = WS_VISIBLE | WS_HIDEMENUBAR | WS_WITHOUTCLOSEMENU;
    CreateInfo.dwExStyle = WS_EX_NONE | WS_EX_AUTOSECONDARYDC;
    CreateInfo.spCaption = "sportDV";
    CreateInfo.hCursor = GetSystemCursor(0);
    CreateInfo.hIcon = 0;
    CreateInfo.MainWindowProc = SportDVWinProc;
    CreateInfo.lx = 0;
    CreateInfo.ty = 0;
    CreateInfo.rx = g_rcScr.right;
    CreateInfo.by = g_rcScr.bottom;
    CreateInfo.dwAddData = 0;
    CreateInfo.hHosting = HWND_DESKTOP;
    CreateInfo.language = parameter_get_video_lan();
    hMainWnd = CreateMainWindow(&CreateInfo);

    if (hMainWnd == HWND_INVALID)
        return -1;

    RegisterMainWindow(hMainWnd);

    api_poweron_init(ui_msg_manager_cb);
    api_reg_ui_preview_callback(entervideopreview, exitvideopreview);
    parameter_save_cam_num(1);

    screenoff_time = parameter_get_screenoff_time();
    rk_fb_set_lcd_backlight(parameter_get_video_backlt());
    rk_fb_set_flip(parameter_get_video_flip());

    bkcolor = GetWindowElementPixel(hMainWnd, WE_BGC_DESKTOP);
    SetWindowBkColor(hMainWnd, bkcolor);

    /*
     * Blit uses software surface, avoid to pop menu can not be hide
     * after we close them. The value 0 is dummy paramter in function.
     */
    sndHdc = GetSecondaryDC((HWND)hMainWnd);
    SetMemDCAlpha(sndHdc, MEMDC_FLAG_SWSURFACE, 0);

    key.blue = (bkcolor & 0x1f) << 3;
    key.green = ((bkcolor >> 5) & 0x3f) << 2;
    key.red = ((bkcolor >> 11) & 0x1f) << 3;

    switch (rkfb_get_pixel_format()) {
    case UI_FORMAT_RGB_565:
        key.enable = 1;
        break;

    case UI_FORMAT_BGRA_8888:
    default:
        key.enable = 0;
        break;
    }

    if (rkfb_set_color_key(&key) == -1)
        printf("rkfb_set_color_key err\n");

    ShowWindow(hMainWnd, SW_SHOWNORMAL);
    gyrosensor_init();
    video_dvs_enable(parameter_get_dvs_enabled());

    if (charging_status_check() && !parameter_get_debug_reboot()) {
        if (parameter_get_video_carmode()) {
            initrec(hMainWnd);
        } else {
            screenoff_time = CHARGING_DISPLAY_TIME;
            api_change_mode(MODE_CHARGING);
        }
    } else {
        initrec(hMainWnd);
    }

    while (GetMessage(&Msg, hMainWnd)) {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }

    api_poweroff_deinit();

    UnregisterMainWindow(hMainWnd);
    MainWindowThreadCleanup(hMainWnd);
    unloadres();

    /* Destroy link list */
    deinit_list();

    printf("camera exit\n");
    return 0;
}

#ifdef _MGRM_THREADS
#include <minigui/dti.c>
#endif
