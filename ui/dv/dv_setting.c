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

#include "dv_setting.h"

#include <init_hook/init_hook.h>
#include <stdint.h>
#include <minigui/window.h>
#include <minigui/control.h>
#include <unistd.h>

#include "dv_main.h"
#include "gyrosensor.h"
#include "parameter.h"
#include "public_interface.h"
#include "ui_resolution.h"
#include "sport_dv_ui.h"
#include "video.h"
#include "watermark.h"
#include "licenseplate.h"
#include "wifi_management.h"
#include "audio/playback/audioplay.h"
#include "fs_manage/fs_sdcard.h"
#include "ueventmonitor/usb_sd_ctrl.h"

#define IDC_BUTTON_OK  1000
#define IDC_STATIC1    1001
#define IDC_STATIC2    1002
#define IDC_STATIC3    1003
#define IDC_STATIC4    1004
#define IDC_STATIC5    1005
#define IDC_STATIC6    1006
#define IDC_STATIC7    1007
#define IDC_STATIC8    1008

static int gyro_calibrate_succ = 1;
static int item_index ;  /*item index in current page*/
static int page_index = 1;
int key_enter_list;

int   g_dlg_type = -1;
void *dlg_content = NULL;

static const char *dv_set_text [LANGUAGE_NUM][MAX_SETTING_ITEM] = {
    {
        "ppi",
        "date",
        "gyroscope",
        "language",
        "light",
        "mic",
        "key tone",
        "watermark",
//        "plate",
        "awb",
        "recovery",
        "format",
        "wifi",
        "HZ",
        "Sreen Shutdown",
        "Auto Rec",
        "Rec Time",
        "Video Quality",
//        "Car Mode",
        "Firmware Upgrade",
        "Firmware Version",
        "Debug"
    },
    {
        "录像尺寸",
        "日期设置",
        "陀螺仪防抖",
        "语言",
        "亮度调节",
        "录音",
        "按键声",
        "水印设置",
//        "车牌水印",
        "白平衡",
        "恢复默认设置",
        "格式化",
        "WiFi",
        "频率",
        "屏幕自动关闭",
        "开机自动录像",
        "录像时长",
        "视频质量",
//        "车载模式",
        "固件升级",
        "固件版本",

        "调试"
    }
};

static const char *list_mp_text [LANGUAGE_NUM][MAX_MP_NUM] = {
    {
        "1080P 30FPS",
        "1080P 60FPS",
        " 720P 30FPS",
        " 720P 60FPS",
        "   4K 25FPS"
    },
    {
        "1080P 30FPS",
        "1080P 60FPS",
        " 720P 30FPS",
        " 720P 60FPS",
        "   4K 25FPS"
    }
};

static const char *list_wb_text [LANGUAGE_NUM][MAX_WB_NUM] = {
    {
        "Auto",
        "Sun",
        "Flu",
        "Cloud",
        "Tun"
    },
    {
        "自动",
        "日光",
        "荧光",
        "阴天",
        "灯光"
    }
};

static const char *list_level_text [LANGUAGE_NUM][MAX_LEVEL_NUM] = {
    {
        "Low",
        "Mid",
        "High"
    },
    {
        "低",
        "中",
        "高"
    }
};

static const char *list_videotime_text [LANGUAGE_NUM][MAX_REC_TIME_NUM] = {
    {
        "1 MIN",
        "3 MIN",
        "5 MIN",
        "Unlimited"
    },
    {
        "1分钟",
        "3分钟",
        "5分钟",
        "不限时长"
    }
};

static const char *list_gs_text [LANGUAGE_NUM][MAX_GS_NUM] = {
    {
        "close",
        "open",
        "calibration"
    },
    {
        "关闭防抖",
        "开启防抖",
        "陀螺仪校准",
    }
};

static const char *list_switch_text [LANGUAGE_NUM][MAX_SWITCH_NUM] = {
    {
        "close",
        "open"
    },
    {
        "关闭",
        "开启"
    }
};

static const char *list_lan_text [LANGUAGE_NUM][MAX_LAN_NUM] = {
    {
        "English",
        "中文"
    },
    {
        "English",
        "中文"
    }
};

static const char *list_hz_text [LANGUAGE_NUM][MAX_HZ_NUM] = {
    {
        "50HZ",
        "60HZ"
    },
    {
        "50HZ",
        "60HZ"
    }
};

static const char *list_keytone_text [LANGUAGE_NUM][MAX_KEY_TONE_NUM] = {
    {
        "Low",
        "Mid",
        "High",
        "Close",
    },
    {
        "低",
        "中",
        "高",
        "关闭",
    }
};

static const char *text_date_setting [LANGUAGE_NUM][MAX_DATE_SETTING] = {
    {
        "Date Setting",
        "Y",
        "M",
        "D",
        "H",
        "MIN",
        "SEC",
    },
    {
        "日期设置",
        "年",
        "月",
        "日",
        "时",
        "分",
        "秒",
    }
};

static const char *text_msg_button_text [LANGUAGE_NUM][MAX_DLG_BN_NUM] = {
    {
        "Yes",
        "No",
        "OK"
    },
    {
        "是",
        "否",
        "确定",
    }
};

const struct video_param dv_video_resolution[MAX_VIDEO_RES] = {
    {1920, 1080, 30},
    {1920, 1080, 60},
    {1280,  720, 30},
    {1280,  720, 60},
    {3840, 2160, 25},
};

/* Chinese index */
uint32_t lic_chinese_idx;
/* Charactor or number index */
uint32_t lic_num_idx;

/* Save the car license plate */
char licenseplate[MAX_LICN_NUM][3] = {"-", "-", "-", "-", "-", "-", "-", "-",};
uint32_t licplate_pos[MAX_LICN_NUM] = {0};

extern const char g_license_chinese[PROVINCE_ABBR_MAX][ONE_CHN_SIZE];
extern const char g_license_char[LICENSE_CHAR_MAX][ONE_CHAR_SIZE];

static void key_enter_listfuc_mp(HDC hdc, HWND hWnd);
static void key_enter_listfuc_gs(HDC hdc, HWND hWnd);
static void key_enter_listfuc_hz(HDC hdc, HWND hWnd);
static void key_enter_listfuc_mic(HDC hdc, HWND hWnd);
static void key_enter_listfuc_light(HDC hdc, HWND hWnd);
static void key_enter_listfuc_wifi(HDC hdc, HWND hWnd);
static void key_enter_listfuc_screen(HDC hdc, HWND hWnd);
static void key_enter_listfuc_wb(HDC hdc, HWND hWnd);
//static void key_enter_listfuc_carmode(HDC hdc, HWND hWnd);
static void key_enter_listfuc_lan(HDC hdc, HWND hWnd);
static void key_enter_listfuc_autovideo(HDC hdc, HWND hWnd);
static void key_enter_listfuc_videoquality(HDC hdc, HWND hWnd);
static void key_enter_listfuc_videotime(HDC hdc, HWND hWnd);
static void key_enter_listfuc_watermark(HDC hdc, HWND hWnd);
static void key_enter_listfuc_keytone(HDC hdc, HWND hWnd);

static struct settig_item_st setting_list[MAX_SETTING_ITEM] = {
    {
        0, 1, 0, 1, MAX_MP_NUM, "ppi", key_enter_listfuc_mp,
    },
    {
        0, 1, 0, 0, 0, "date", NULL,
    },
    {
        0, 1, 0, 1, MAX_GS_NUM, "gyroscope", key_enter_listfuc_gs,
    },
    {
        0, 1, 0, 1, MAX_LAN_NUM, "language", key_enter_listfuc_lan,
    },
    {
        0, 1, 0, 1, MAX_LEVEL_NUM, "light", key_enter_listfuc_light,
    },
    {
        0, 1, 0, 1, MAX_SWITCH_NUM, "mic", key_enter_listfuc_mic,
    },
    {
        0, 1, 0, 1, MAX_KEY_TONE_NUM, "key tone", key_enter_listfuc_keytone,
    },
    {
        0, 2, 0, 1, MAX_SWITCH_NUM, "watermark", key_enter_listfuc_watermark,
    },
//    {
//        0, 2, 0, 0, 0, "plate", NULL,
//    },
    {
        0, 2, 0, 1, MAX_WB_NUM, "awb", key_enter_listfuc_wb,
    },
    {
        0, 2, 0, 0, 0, "recovery", NULL,
    },
    {
        0, 2, 0, 0, 0, "format", NULL,
    },
    {
        0, 2, 0, 1, MAX_SWITCH_NUM, "wifi", key_enter_listfuc_wifi,
    },
    {
        0, 2, 0, 1, MAX_HZ_NUM, "hz", key_enter_listfuc_hz,
    },
    {
        0, 2, 0, 1, MAX_SWITCH_NUM, "screen shutdown", key_enter_listfuc_screen,
    },
    {
        0, 3, 0, 1, MAX_SWITCH_NUM, "auto rec", key_enter_listfuc_autovideo,
    },
    {
        0, 3, 0, 1, MAX_REC_TIME_NUM, "rec time", key_enter_listfuc_videotime,
    },
    {
        0, 3, 0, 1, MAX_LEVEL_NUM, "video quality", key_enter_listfuc_videoquality,
    },
//    {
//        0, 3, 0, 1, MAX_SWITCH_NUM, "car mode", key_enter_listfuc_carmode,
//    },
    {
        0, 3, 0, 0, 0, "firmware update", NULL,
    },
    {
        0, 3, 0, 0, 0, "firmware version", NULL,
    },

    {
        0, 3, 0, 0, 0, "debug", NULL,
    },
};

const struct video_param *get_front_video_resolutions(void)
{
    return dv_video_resolution;
}

static int get_dv_video_mp(void)
{
    int i, mp = 0;
    struct video_param *mp_st;

    mp_st = api_get_video_frontcamera_resolution();

    for (i = 0; i < MAX_VIDEO_RES; i++) {
        if (dv_video_resolution[i].fps == mp_st->fps
            && dv_video_resolution[i].width == mp_st->width
            && dv_video_resolution[i].height == mp_st->height) {
            mp = i;
            return mp;
        }
    }

    return mp;
}

static int get_dv_backlight(void)
{
    int level = parameter_get_video_backlt();

    switch (level) {
    case LCD_BACKLT_L:
        return 0;

    case LCD_BACKLT_M:
        return 1;

    case LCD_BACKLT_H:
        return 2;
    }

    return 2;
}

static void set_dv_backlight(int sel)
{
    switch (sel) {
    case 0:
        api_set_lcd_backlight(LCD_BACKLT_L);
        break;

    case 1:
        api_set_lcd_backlight(LCD_BACKLT_M);
        break;

    case 2:
        api_set_lcd_backlight(LCD_BACKLT_H);
        break;

    default:
        api_set_lcd_backlight(LCD_BACKLT_H);
        break;
    }
}

static int get_dv_video_quality(void)
{
    int quality = parameter_get_bit_rate_per_pixel();

    switch (quality) {
    case VIDEO_QUALITY_LOW:
        return 0;

    case VIDEO_QUALITY_MID:
        return 1;

    case VIDEO_QUALITY_HIGH:
        return 2;
    }

    return 2;
}

static void set_dv_video_quality(unsigned int qualiy_level)
{
    switch (qualiy_level) {
    case 0:
        api_set_video_quaity(VIDEO_QUALITY_LOW);
        break;

    case 1:
        api_set_video_quaity(VIDEO_QUALITY_MID);
        break;

    case 2:
        api_set_video_quaity(VIDEO_QUALITY_HIGH);
        break;

    default:
        break;
    }
}

static int get_dv_key_tone(void)
{
    int key_tone_vol = parameter_get_key_tone_volume();

    switch (key_tone_vol) {
    case KEY_TONE_LOW:
        return 0;

    case KEY_TONE_MID:
        return 1;

    case KEY_TONE_HIG:
        return 2;

    case KEY_TONE_MUT:
        return 3;
    }

    return 3;
}

static void set_dv_key_tone(unsigned int key_tone_level)
{
    switch (key_tone_level) {
    case 0:
        api_set_key_tone(KEY_TONE_LOW);
        break;

    case 1:
        api_set_key_tone(KEY_TONE_MID);
        break;

    case 2:
        api_set_key_tone(KEY_TONE_HIG);
        break;

    case 3:
        api_set_key_tone(KEY_TONE_MUT);
        break;

    default:
        break;
    }
}

static int get_dv_record_time(void)
{
    int video_length = parameter_get_recodetime();

    if (video_length < 0)
        return 3;

    switch (video_length) {
    case REC_TIME_1MIN:
        return 0;

    case REC_TIME_3MIN:
        return 1;

    case REC_TIME_5MIN:
        return 2;
    }

    return 0;
}

static void set_dv_record_time(unsigned int time_level)
{
    switch (time_level) {
    case 0:
        api_video_set_time_lenght(REC_TIME_1MIN);
        break;

    case 1:
        api_video_set_time_lenght(REC_TIME_3MIN);
        break;

    case 2:
        api_video_set_time_lenght(REC_TIME_5MIN);
        break;

    case 3:
        api_video_set_time_lenght(-1);
        break;

    default:
        break;
    }
}

static void set_dv_recovery(HWND hWnd)
{
    int lang = (int)parameter_get_video_lan();

    g_dlg_type = DLG_RECOVERY;
    dlg_content = (void *)info_text[lang][TEXT_RECOVERY];
    dv_create_select_dialog(title_text[lang][TITLE_WARNING]);
}

static void set_dv_fwupgrade(HWND hWnd)
{
    int ret = api_check_firmware();
    int lang = (int)parameter_get_video_lan();

    switch (ret) {
    case FW_NOTFOUND:
        printf("check FW_NOTFOUND: (%d)\n", ret);
        dlg_content = (void *)info_text[lang][TEXT_NOT_FOUND_FW];
        dv_create_message_dialog(title_text[lang][TITLE_FWUPGRADE]);
        break;

    case FW_INVALID:
        printf("check FW_INVALID: (%d)\n", ret);
        dlg_content = (void *)info_text[lang][TEXT_ISINVALID_FW];
        dv_create_message_dialog(title_text[lang][TITLE_FWUPGRADE]);
        break;

    case FW_OK:
    default:
        printf("check FW_OK\n");
        g_dlg_type = DLG_SD_FWUPGRADE;
        dlg_content = (void *)info_text[lang][TEXT_ASK_UPGRADE];
        dv_create_select_dialog(title_text[lang][TITLE_FWUPGRADE]);

        break;
    }
}

static void show_dv_fw_version(HWND hWnd)
{
    int lang = (int)parameter_get_video_lan();
    char *version_str = parameter_get_firmware_version();
    dlg_content = (void *)version_str;

    dv_create_message_dialog(title_text[lang][TITLE_VERSION]);
}

static void set_dv_format(HWND hWnd)
{
    int lang = (int)parameter_get_video_lan();
    int sd_state = api_get_sd_mount_state();

    if (sd_state == SD_STATE_IN) {
        g_dlg_type = DLG_SD_FORMAT;
        dlg_content = (void *)sdcard_text[lang][SDCARD_FORMAT];
        dv_create_select_dialog(title_text[lang][TITLE_WARNING]);
    } else if (sd_state == SD_STATE_OUT) {
        dlg_content = (void *)sdcard_text[lang][SDCARD_NO_CARD];
        dv_create_message_dialog(title_text[lang][TITLE_WARNING]);
    }
}

static int dv_select_proc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    RECT rcDraw;
    SIZE sz;
    int fontheight, dlgwidth;

    switch (message) {
    case MSG_INITDIALOG:
        SetWindowElementAttr(hDlg, WE_BGCA_ACTIVE_CAPTION, COLOR_black);
        return 1;

    case MSG_PAINT:
        hdc = BeginPaint(hDlg);
        GetWindowRect(hDlg, &rcDraw);
        FillBoxWithBitmap(hdc, 0, 0, RECTW(rcDraw), RECTH(rcDraw), &dv_video_all[TOP_BK_IMG_ID]);

        SetBkColor(hdc, GetWindowElementPixel(hMainWnd, WE_BGC_TOOLTIP));
        SetTextColor(hdc, PIXEL_lightwhite);
        SelectFont(hdc, (PLOGFONT)GetWindowElementAttr(hDlg, WE_FONT_MENU));

        dlgwidth = RECTW(rcDraw);
        rcDraw.left   = 0;
        rcDraw.top    = 20;
        rcDraw.right  = rcDraw.left + dlgwidth;
        rcDraw.bottom = rcDraw.top + 20;

        fontheight = GetFontHeight(hdc);
        GetTabbedTextExtent(hdc, (const char *)dlg_content, strlen(dlg_content), &sz);

        if ((sz.cy / fontheight) > 1) {
            /* muti-lines*/
            rcDraw.bottom += 20;
            DrawText(hdc, (const char *)dlg_content, -1, &rcDraw, DT_NOCLIP | DT_CENTER);
        } else {
            /* Single line*/
            DrawText(hdc, (const char *)dlg_content, -1, &rcDraw,
                     DT_NOCLIP | DT_SINGLELINE | DT_CENTER | DT_VCENTER);
        }

        EndPaint(hDlg, hdc);
        break;

    case MSG_COMMAND:
        switch (wParam) {
        case IDC_YES: {
            int ret = 0;

            switch (g_dlg_type) {
            case DLG_SD_FORMAT:
                api_sdcard_format(0);
                break;

            case DLG_SD_FORMAT_FAIL:
                reformat_dv_sdcard();
                break;

            case DLG_SD_LOAD_FAIL:
            case DLG_SD_NO_SPACE:
            case DLG_SD_INIT_FAIL:
                api_sdcard_format(0);
                break;

            case DLG_SD_FWUPGRADE:
                ret = api_check_firmware();

                if (ret == FW_OK) {
                    /* The BOOT_FWUPDATE which is defined in init_hook.h */
                    printf("will reboot and update...\n");
                    parameter_flash_set_flashed(BOOT_FWUPDATE);
                    api_power_reboot();
                }

                break;

            case DLG_RECOVERY:
                parameter_recover();
                api_power_reboot();
                break;

            default:
                break;
            }

            g_dlg_type = -1;
            dlg_content = NULL;
            EndDialog(hDlg, 0);
            break;
        }

        case IDC_NO:
            if (g_dlg_type == DLG_SD_FORMAT_FAIL
                && api_get_sd_mount_state() == SD_STATE_IN)
                cvr_sd_ctl(0);

            g_dlg_type = -1;
            dlg_content = NULL;
            EndDialog(hDlg, 0);
            break;
        }

        break;

    case MSG_KEYDOWN:
        audio_sync_play(KEYPRESS_SOUND_FILE);
        break;
    }

    return DefaultDialogProc(hDlg, message, wParam, lParam);
}

void dv_create_select_dialog(const char *caption_txt)
{
    int lang = (int)parameter_get_video_lan();

    if (g_dlg_type == -1 || dlg_content == NULL)
        return;

    CTRLDATA selectCtrl[] = {
        {
            CTRL_BUTTON, WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
            DV_DLG_BN_X, DV_DLG_BN_Y, DV_DLG_BN_W, DV_DLG_BN_H,
            IDC_YES, text_msg_button_text[lang][TEXT_BUTTON_YES], 0, 0, NULL, NULL
        },
        {
            CTRL_BUTTON, WS_VISIBLE | BS_DEFPUSHBUTTON | BS_PUSHBUTTON | WS_TABSTOP,
            DV_DLG_BN_X + 95, DV_DLG_BN_Y, DV_DLG_BN_W, DV_DLG_BN_H,
            IDC_NO, text_msg_button_text[lang][TEXT_BUTTON_NO], 0, 0, NULL, NULL
        }
    };

    DLGTEMPLATE select_dlg = {
        WS_VISIBLE | WS_CAPTION,
        WS_EX_NOCLOSEBOX,
        DV_DLG_X, DV_DLG_Y, DV_DLG_W, DV_DLG_H,
        caption_txt, 0, 0, 0, NULL, 0
    };

    select_dlg.controlnr = 2;
    select_dlg.controls = selectCtrl;
    DialogBoxIndirectParam(&select_dlg, hMainWnd, dv_select_proc, 0L);
}

static int dv_warning_proc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    RECT rcDraw;
    SIZE sz;
    int fontheight, dlgwidth;

    switch (message) {
    case MSG_INITDIALOG:
        SetWindowElementAttr(hDlg, WE_BGCA_ACTIVE_CAPTION, COLOR_black);
        return 1;

    case MSG_PAINT:
        hdc = BeginPaint(hDlg);
        GetWindowRect(hDlg, &rcDraw);
        FillBoxWithBitmap(hdc, 0, 0, RECTW(rcDraw), RECTH(rcDraw), &dv_video_all[TOP_BK_IMG_ID]);

        SetBkColor(hdc, GetWindowElementPixel(hMainWnd, WE_BGC_TOOLTIP));
        SetTextColor(hdc, PIXEL_lightwhite);
        SelectFont(hdc, (PLOGFONT)GetWindowElementAttr(hDlg, WE_FONT_MENU));

        dlgwidth = RECTW(rcDraw);
        rcDraw.left   = 0;
        rcDraw.top    = 20;
        rcDraw.right  = rcDraw.left + dlgwidth;
        rcDraw.bottom = rcDraw.top + 20;

        fontheight = GetFontHeight(hdc);
        GetTabbedTextExtent(hdc, (const char *)dlg_content, strlen(dlg_content), &sz);

        if ( (sz.cy / fontheight) > 1) {
            /* muti-lines*/
            rcDraw.bottom += 20;
            DrawText(hdc, (const char *)dlg_content, -1, &rcDraw, DT_NOCLIP | DT_CENTER);
        } else {
            /* Single line*/
            DrawText(hdc, (const char *)dlg_content, -1, &rcDraw,
                     DT_NOCLIP | DT_SINGLELINE | DT_CENTER | DT_VCENTER);
        }

        EndPaint(hDlg, hdc);
        break;

    case MSG_COMMAND:
        if (wParam == IDC_CFM) {
            dlg_content = NULL;
            EndDialog(hDlg, 0);
        }

        break;

    case MSG_KEYDOWN:
        audio_sync_play(KEYPRESS_SOUND_FILE);
        break;
    }

    return DefaultDialogProc(hDlg, message, wParam, lParam);
}

void dv_create_message_dialog(const char *caption_txt)
{
    int lang = (int)parameter_get_video_lan();

    CTRLDATA warningCtrl[] = {
        {
            CTRL_BUTTON, WS_VISIBLE | BS_DEFPUSHBUTTON | BS_PUSHBUTTON | WS_TABSTOP,
            DV_DLG_BN_X + 45, DV_DLG_BN_Y, DV_DLG_BN_W, DVP_MIN_IMG_H,
            IDC_CFM, text_msg_button_text[lang][TEXT_BUTTON_CFM], 0, 0, NULL, NULL
        },
    };

    DLGTEMPLATE warnning_dlg = {
        WS_VISIBLE | WS_CAPTION,
        WS_EX_NOCLOSEBOX,
        DV_DLG_X, DV_DLG_Y, DV_DLG_W, DV_DLG_H,
        caption_txt, 0, 0, 0, NULL, 0
    };

    warnning_dlg.controlnr = 1;
    warnning_dlg.controls = warningCtrl;
    DialogBoxIndirectParam(&warnning_dlg, hMainWnd, dv_warning_proc, 0L);
}


/* If format fail, retry format. */
void reformat_dv_sdcard(void)
{
    int lang = (int)parameter_get_video_lan();

    if (fs_manage_sd_exist(SDCARD_DEVICE) != 0) {
        api_sdcard_format(1);
    } else {
        dlg_content = (void *)sdcard_text[lang][SDCARD_NO_CARD];
        dv_create_message_dialog(title_text[lang][TITLE_WARNING]);
    }
}

static void draw_select_item(HDC hdc, HWND hWnd, int itemFlag, POINT *point)
{
    int line_pos = 0;

    SetBkColor(hdc, PIXEL_black);
    SetBkColor(hdc, GetWindowElementPixel(hMainWnd, WE_BGC_TOOLTIP));
    SelectFont(hdc, (PLOGFONT)GetWindowElementAttr(hWnd, WE_FONT_MENU));

    line_pos = itemFlag * KEY_ENTER_BOX_H;
    SetTextColor (hdc, PIXEL_lightwhite);
    SetBkMode(hdc, BM_TRANSPARENT);

    FillBoxWithBitmap(hdc, point->x, point->y + line_pos,
                      KEY_ENTER_BOX_W, KEY_ENTER_BOX_H,
                      &dv_video_all[BOX_HIGHLIGHT_SUB_SEL_IMG_ID]);
}

static void draw_menu_item(HDC hdc, int itemPagFlag, int itemCnt,
                           POINT *point, const char *textArray[2][itemCnt])
{
    int i, pag = 0;
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

            DrawText(hdc, textArray[lang][i],
                     -1, &rcDraw, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
        }
    }
}

static void draw_setting_item_common(HDC hdc, HWND hWnd, POINT *point,
                                     int item_id, int itemCnt,
                                     const char *textArray[LANGUAGE_NUM][itemCnt])
{
    struct settig_item_st setting_item = setting_list[item_id];

    draw_select_item(hdc, hWnd, setting_item.sub_sel_num, point);
    draw_menu_item(hdc, setting_item.sub_menu_page, itemCnt, point, textArray);
}

static void key_enter_listfuc_screen(HDC hdc, HWND hWnd)
{
    POINT startPoint;

    startPoint.x = KEY_SUBMENU_TEXT_X;
    startPoint.y = KEY_TEXT_1_Y;
    draw_setting_item_common(hdc, hWnd, &startPoint, SCREEN_OFF,
                             MAX_SWITCH_NUM, list_switch_text);
}

static void key_enter_listfuc_wifi(HDC hdc, HWND hWnd)
{
    POINT startPoint;

    startPoint.x = KEY_SUBMENU_TEXT_X;
    startPoint.y = KEY_TEXT_1_Y;
    draw_setting_item_common(hdc, hWnd, &startPoint, WIFI, MAX_SWITCH_NUM, list_switch_text);
}

static void key_enter_listfuc_hz(HDC hdc, HWND hWnd)
{
    POINT startPoint;

    startPoint.x = KEY_SUBMENU_TEXT_X;
    startPoint.y = KEY_TEXT_1_Y;
    draw_setting_item_common(hdc, hWnd, &startPoint, HZ, MAX_HZ_NUM, list_hz_text);
}

static void key_enter_listfuc_wb(HDC hdc, HWND hWnd)
{
    POINT startPoint;

    startPoint.x = KEY_SUBMENU_TEXT_X;
    startPoint.y = KEY_SUBMENU_TEXT_Y;
    draw_setting_item_common(hdc, hWnd, &startPoint, AWB, MAX_WB_NUM, list_wb_text);
}

static void key_enter_listfuc_gs(HDC hdc, HWND hWnd)
{
    POINT startPoint;

    startPoint.x = KEY_SUBMENU_TEXT_X;
    startPoint.y = KEY_TEXT_2_Y;
    draw_setting_item_common(hdc, hWnd, &startPoint, GYROSCOPE, MAX_GS_NUM, list_gs_text);
}

static void key_enter_listfuc_lan(HDC hdc, HWND hWnd)
{
    POINT startPoint;

    startPoint.x = KEY_SUBMENU_TEXT_X;
    startPoint.y = KEY_TEXT_1_Y;
    draw_setting_item_common(hdc, hWnd, &startPoint, LANGUAGE, MAX_LAN_NUM, list_lan_text);
}

static void key_enter_listfuc_light(HDC hdc, HWND hWnd)
{
    POINT startPoint;

    startPoint.x = KEY_SUBMENU_TEXT_X;
    startPoint.y = KEY_TEXT_2_Y;
    draw_setting_item_common(hdc, hWnd, &startPoint, LIGHT, MAX_LEVEL_NUM, list_level_text);
}

static void key_enter_listfuc_mp(HDC hdc, HWND hWnd)
{
    POINT startPoint;

    startPoint.x = KEY_SUBMENU_TEXT_X;
    startPoint.y = KEY_SUBMENU_TEXT_Y;
    draw_setting_item_common(hdc, hWnd, &startPoint, PPI, MAX_MP_NUM, list_mp_text);
}

static void key_enter_listfuc_mic(HDC hdc, HWND hWnd)
{
    POINT startPoint;

    startPoint.x = KEY_SUBMENU_TEXT_X;
    startPoint.y = KEY_TEXT_1_Y;
    draw_setting_item_common(hdc, hWnd, &startPoint, MIC, MAX_SWITCH_NUM, list_switch_text);
}

static void key_enter_listfuc_keytone(HDC hdc, HWND hWnd)
{
    POINT startPoint;

    startPoint.x = KEY_SUBMENU_TEXT_X;
    startPoint.y = KEY_TEXT_2_Y;
    draw_setting_item_common(hdc, hWnd, &startPoint, KEY_TONE, MAX_KEY_TONE_NUM, list_keytone_text);
}

static void key_enter_listfuc_watermark(HDC hdc, HWND hWnd)
{
    POINT startPoint;

    startPoint.x = KEY_SUBMENU_TEXT_X;
    startPoint.y = KEY_TEXT_1_Y;
    draw_setting_item_common(hdc, hWnd, &startPoint, WATERMARK, MAX_SWITCH_NUM, list_switch_text);
}

static void key_enter_listfuc_autovideo(HDC hdc, HWND hWnd)
{
    POINT startPoint;

    startPoint.x = KEY_SUBMENU_TEXT_X;
    startPoint.y = KEY_TEXT_1_Y;
    draw_setting_item_common(hdc, hWnd, &startPoint, AUTO_REC, MAX_SWITCH_NUM, list_switch_text);
}

static void key_enter_listfuc_videotime(HDC hdc, HWND hWnd)
{
    POINT startPoint;

    startPoint.x = KEY_SUBMENU_TEXT_X;
    startPoint.y = KEY_TEXT_2_Y;
    draw_setting_item_common(hdc, hWnd, &startPoint, REC_TIME, MAX_REC_TIME_NUM, list_videotime_text);
}

static void key_enter_listfuc_videoquality(HDC hdc, HWND hWnd)
{
    POINT startPoint;

    startPoint.x = KEY_SUBMENU_TEXT_X;
    startPoint.y = KEY_TEXT_2_Y;
    draw_setting_item_common(hdc, hWnd, &startPoint, VIDEO_QUALITY, MAX_LEVEL_NUM, list_level_text);
}

/*
static void key_enter_listfuc_carmode(HDC hdc, HWND hWnd)
{
    POINT startPoint;

    startPoint.x = KEY_SUBMENU_TEXT_X;
    startPoint.y = KEY_TEXT_1_Y;
    draw_setting_item_common(hdc, hWnd, &startPoint, CAR_MODE, MAX_SWITCH_NUM, list_switch_text);
}
*/

/* Data / time setting */
static void prompt(HWND hDlg)
{
    int year = SendDlgItemMessage(hDlg, IDL_YEAR, CB_GETSPINVALUE, 0, 0);
    int mon = SendDlgItemMessage(hDlg, IDL_MONTH, CB_GETSPINVALUE, 0, 0);
    int mday = SendDlgItemMessage(hDlg, IDL_DAY, CB_GETSPINVALUE, 0, 0);
    int hour = SendDlgItemMessage(hDlg, IDC_HOUR, CB_GETSPINVALUE, 0, 0);
    int min = SendDlgItemMessage(hDlg, IDC_MINUTE, CB_GETSPINVALUE, 0, 0);
    int sec = SendDlgItemMessage(hDlg, IDL_SEC, CB_GETSPINVALUE, 0, 0);

    printf("%d--%d--%d--%d--%d--%d\n", year, mon, mday, hour, min, sec);

    api_set_system_time(year, mon, mday, hour, min, sec);
}

static int date_box_proc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
    time_t t;
    HWND hCurFocus, hNewFocus;
    struct tm *tm;

    switch (message) {
    case MSG_INITDIALOG:
        time(&t);
        tm = localtime(&t);
        hCurFocus = GetFocusChild(hDlg);
        SendDlgItemMessage(hDlg, IDL_YEAR, CB_SETSPINFORMAT, 0, (LPARAM) "%04d");
        SendDlgItemMessage(hDlg, IDL_YEAR, CB_SETSPINRANGE, 1900, 2100);
        SendDlgItemMessage(hDlg, IDL_YEAR, CB_SETSPINVALUE, tm->tm_year + 1900, 0);
        SendDlgItemMessage(hDlg, IDL_YEAR, CB_SETSPINPACE, 1, 1);

        SendDlgItemMessage(hDlg, IDL_MONTH, CB_SETSPINFORMAT, 0, (LPARAM) "%02d");
        SendDlgItemMessage(hDlg, IDL_MONTH, CB_SETSPINRANGE, 1, 12);
        SendDlgItemMessage(hDlg, IDL_MONTH, CB_SETSPINVALUE, tm->tm_mon + 1, 0);
        SendDlgItemMessage(hDlg, IDL_MONTH, CB_SETSPINPACE, 1, 2);

        SendDlgItemMessage(hDlg, IDL_DAY, CB_SETSPINFORMAT, 0, (LPARAM) "%02d");
        SendDlgItemMessage(hDlg, IDL_DAY, CB_SETSPINRANGE, 0, 31);
        SendDlgItemMessage(hDlg, IDL_DAY, CB_SETSPINVALUE, tm->tm_mday, 0);
        SendDlgItemMessage(hDlg, IDL_DAY, CB_SETSPINPACE, 1, 3);

        SendDlgItemMessage(hDlg, IDC_HOUR, CB_SETSPINFORMAT, 0, (LPARAM) "%02d");
        SendDlgItemMessage(hDlg, IDC_HOUR, CB_SETSPINRANGE, 0, 23);
        SendDlgItemMessage(hDlg, IDC_HOUR, CB_SETSPINVALUE, tm->tm_hour, 0);
        SendDlgItemMessage(hDlg, IDC_HOUR, CB_SETSPINPACE, 1, 4);

        SendDlgItemMessage(hDlg, IDC_MINUTE, CB_SETSPINFORMAT, 0, (LPARAM) "%02d");
        SendDlgItemMessage(hDlg, IDC_MINUTE, CB_SETSPINRANGE, 0, 59);
        SendDlgItemMessage(hDlg, IDC_MINUTE, CB_SETSPINVALUE, tm->tm_min, 0);
        SendDlgItemMessage(hDlg, IDC_MINUTE, CB_SETSPINPACE, 1, 5);

        SendDlgItemMessage(hDlg, IDL_SEC, CB_SETSPINFORMAT, 0, (LPARAM) "%02d");
        SendDlgItemMessage(hDlg, IDL_SEC, CB_SETSPINRANGE, 0, 59);
        SendDlgItemMessage(hDlg, IDL_SEC, CB_SETSPINVALUE, tm->tm_sec, 0);
        SendDlgItemMessage(hDlg, IDL_SEC, CB_SETSPINPACE, 1, 6);
        hNewFocus = GetNextDlgTabItem(hDlg, hCurFocus, FALSE);
        SetNullFocus(hCurFocus);
        SetFocus(hNewFocus);
        return 0;

    case MSG_COMMAND:
        break;

    case MSG_KEYDOWN:
        switch (wParam) {
        case SCANCODE_ESCAPE:
            audio_sync_play(KEYPRESS_SOUND_FILE);
            EndDialog(hDlg, wParam);
            break;

        case SCANCODE_SHUTDOWN:
            audio_sync_play(KEYPRESS_SOUND_FILE);
            hCurFocus = GetFocusChild(hDlg);
            hCurFocus = GetNextDlgTabItem(hDlg, hCurFocus, FALSE);
            SetFocus(hCurFocus);
            break;

        case SCANCODE_ENTER: {
            bool dvs_on = video_dvs_is_enable();
            audio_sync_play(KEYPRESS_SOUND_FILE);

            key_enter_list = 0;

            if (dvs_on)
                video_dvs_enable(0);

            prompt(hDlg);
            EndDialog(hDlg, wParam);
            usleep(200000);

            if (dvs_on)
                video_dvs_enable(1);

            break;
        }
        }

        break;
    }

    return DefaultDialogProc(hDlg, message, wParam, lParam);
}

static void set_dv_date(void)
{
    DLGTEMPLATE DlgMyDate = {WS_VISIBLE,
                             WS_EX_NONE,
                             0,
                             60,
                             320,
                             110,
                             SetDate,
                             0,
                             0,
                             14,
                             NULL,
                             0
                            };

    int lang = parameter_get_video_lan();

    CTRLDATA CtrlMyDate[] = {
        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE, 100, 8, 150, 30, IDC_STATIC,
            text_date_setting[lang][DATE_SETTING], 0, 0, NULL, NULL
        },
        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE, 0, 45, 320, 2, IDC_STATIC,
            "-----", 0, 0, NULL, NULL
        },
        {
            CTRL_COMBOBOX,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_AUTOSPIN | CBS_AUTOLOOP, 60, 48,
            80, 20, IDL_YEAR, "", 0, 0, NULL, NULL
        },

        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE, 140, 50, 20, 20, IDC_STATIC,
            text_date_setting[lang][DATE_Y], 0, 0, NULL, NULL
        },

        {
            CTRL_COMBOBOX,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_AUTOSPIN | CBS_AUTOLOOP, 160,
            48, 40, 20, IDL_MONTH, "", 0, 0, NULL, NULL
        },
        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE, 200, 50, 20, 20, IDC_STATIC,
            text_date_setting[lang][DATE_M], 0, 0, NULL, NULL
        },

        {
            CTRL_COMBOBOX,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_AUTOSPIN | CBS_AUTOLOOP, 220,
            48, 40, 20, IDL_DAY, "", 0, 0, NULL, NULL
        },
        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE, 260, 50, 20, 20, IDC_STATIC,
            text_date_setting[lang][DATE_D], 0, 0, NULL, NULL
        },

        {
            CTRL_COMBOBOX,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_AUTOSPIN | CBS_AUTOLOOP, 75, 78,
            40, 20, IDC_HOUR, "", 0, 0, NULL, NULL
        },
        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE, 115, 80, 20, 20, IDC_STATIC,
            text_date_setting[lang][DATE_H], 0, 0, NULL, NULL
        },

        {
            CTRL_COMBOBOX,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_AUTOSPIN | CBS_AUTOLOOP, 135,
            78, 40, 20, IDC_MINUTE, "", 0, 0, NULL, NULL
        },
        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE, 175, 80, 25, 20, IDC_STATIC,
            text_date_setting[lang][DATE_MIN], 0, 0, NULL, NULL
        },

        {
            CTRL_COMBOBOX,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_AUTOSPIN | CBS_AUTOLOOP, 200,
            78, 40, 20, IDL_SEC, "", 0, 0, NULL, NULL
        },
        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE, 240, 80, 25, 20, IDC_STATIC,
            text_date_setting[lang][DATE_SEC], 0, 0, NULL, NULL
        },
    };

    DlgMyDate.controls = CtrlMyDate;
    DialogBoxIndirectParam(&DlgMyDate, hMainWnd, date_box_proc, 0L);
}

/*
static int license_plate_proc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    int dlgID;
    int i, k;
    HWND hCurFocus;
    RECT rt;
    static HWND hFocus;

    switch (message) {
    case MSG_INITDIALOG: {
        hCurFocus = GetFocus(hDlg);
        hFocus = GetNextDlgTabItem(hDlg, hCurFocus, FALSE);
        dlgID = GetDlgCtrlID(hFocus);

        get_licenseplate_and_pos((char *)licenseplate, sizeof(licenseplate), licplate_pos);

        // Set licence plate at init.
        for (k = 0; k < MAX_LICN_NUM; k++) {
            if (dlgID == ((k + 1) + IDC_BUTTON_OK))
                SetWindowText(hFocus, (char *)licenseplate[k]);

            hFocus = GetNextDlgTabItem(hDlg, hFocus, FALSE);
            dlgID = GetDlgCtrlID(hFocus);
        }

        hFocus = GetNextDlgTabItem(hDlg, hFocus, FALSE);
        dlgID = GetDlgCtrlID(hFocus);
        SetFocus(hFocus);
        return 0;
    }

    case MSG_COMMAND:
        break;

    case MSG_PAINT: {
        uint32_t old_bk_color;
        uint32_t old_pen_color;

        hdc = BeginPaint(hDlg);
        GetClientRect(hDlg, &rt);
        hFocus = GetFocus(hDlg);
        dlgID = GetDlgCtrlID(hFocus);

        if (dlgID == IDC_BUTTON_OK)
            SetFocus(hFocus);

        old_bk_color = GetBkColor(hdc);
        old_pen_color = GetPenColor(hdc);

        for (i = IDC_STATIC1; i <= IDC_STATIC8; i++) {
            if (i == dlgID) {
                // Selected
                SetPenColor(hdc, COLOR_red);
                SetPenWidth(hdc, 4);
            } else {
                // Unselected
                SetPenColor(hdc, old_bk_color);
            }
            Rectangle(hdc,
                      LICN_RECT_X - 4 + (i - IDC_STATIC1) * LICN_RECT_GAP,
                      LICN_RECT_Y - 4,
                      LICN_RECT_X + LICN_RECT_W + 4 + (i - IDC_STATIC1) * LICN_RECT_GAP,
                      LICN_RECT_Y + LICN_RECT_H + 4);
        }

        if (dlgID == IDC_BUTTON_OK) {
            SetPenColor(hdc, COLOR_red);
            SetPenWidth(hdc, 4);
        } else {
            uint8_t r, g, b;
            GetPixelRGB(hdc, 0, 0, &r, &g, &b);
            SetPenColor(hdc, RGB2Pixel(hdc, r, g, b));
        }
        Rectangle(hdc,
                  LICN_BUTTON_OK_X - 2,
                  LICN_BUTTON_OK_Y - 2,
                  LICN_BUTTON_OK_X + LICN_BUTTON_OK_W + 2,
                  LICN_BUTTON_OK_Y + LICN_BUTTON_OK_H + 2);

        SetPenColor(hdc, old_pen_color);
        Rectangle(hdc, 10, 10, rt.right - 10, rt.bottom - 10);
        EndPaint(hDlg, hdc);
    }
    break;

    case MSG_KEYDOWN: {
        if (get_key_state())
            break;

        int index = 0;
        hdc = GetClientDC(hDlg);
        audio_sync_play(KEYPRESS_SOUND_FILE);

        switch (wParam) {
        case SCANCODE_ESCAPE:
            EndDialog(hDlg, wParam);
            break;

        case SCANCODE_MENU:
            save_licenseplate((char *)licenseplate);
            EndDialog(hDlg, wParam);
            break;

        case SCANCODE_ENTER: {
            audio_sync_play(KEYPRESS_SOUND_FILE);
            save_licenseplate((char *)licenseplate);
            EndDialog(hDlg, wParam);
            break;
        }

        case SCANCODE_CURSORBLOCKUP: {
            hFocus = GetFocus(hDlg);
            dlgID = GetDlgCtrlID(hFocus);

            if (dlgID == IDC_BUTTON_OK)
                break;

            if (dlgID == IDC_STATIC1) {
                if (lic_chinese_idx >= PROVINCE_ABBR_MAX - 1)
                    lic_chinese_idx = 0;

                SetWindowText(hFocus, g_license_chinese[++lic_chinese_idx]);
                licplate_pos[0] = lic_chinese_idx;
            } else {
                index = dlgID - IDC_STATIC1;
                lic_num_idx = licplate_pos[index];

                if (lic_num_idx >= LICENSE_CHAR_MAX - 1)
                    lic_num_idx = 0;

                SetWindowText(hFocus, (char *)g_license_char[++lic_num_idx]);
                licplate_pos[index] = lic_num_idx;
            }

            break;
        }

        case SCANCODE_CURSORBLOCKDOWN: {
            hFocus = GetFocus(hDlg);
            dlgID = GetDlgCtrlID(hFocus);

            if (dlgID == IDC_BUTTON_OK)
                break;

            if (dlgID == IDC_STATIC1) {
                if (lic_chinese_idx == 0)
                    lic_chinese_idx = PROVINCE_ABBR_MAX - 1;

                if (lic_chinese_idx == 1)
                    lic_chinese_idx = PROVINCE_ABBR_MAX;

                SetWindowText(hFocus, (char *)g_license_chinese[--lic_chinese_idx]);
                licplate_pos[0] = lic_chinese_idx;
            } else {
                index = dlgID - IDC_STATIC1;
                lic_num_idx = licplate_pos[index];

                if (lic_num_idx == 0)
                    lic_num_idx = LICENSE_CHAR_MAX - 1;

                if (lic_num_idx == 1)
                    lic_num_idx = LICENSE_CHAR_MAX;

                SetWindowText(hFocus, g_license_char[--lic_num_idx]);
                licplate_pos[index] = lic_num_idx;
            }

            break;
        }

        case SCANCODE_CURSORBLOCKLEFT:
            hFocus = GetNextDlgTabItem(hDlg, hFocus, TRUE);
            SetFocus(hFocus);
            GetClientRect(hDlg, &rt);
            InvalidateRect(hDlg, &rt, FALSE);
            break;

        case SCANCODE_CURSORBLOCKRIGHT:
            hFocus = GetNextDlgTabItem(hDlg, hFocus, FALSE);
            SetFocus(hFocus);
            GetClientRect(hDlg, &rt);
            InvalidateRect(hDlg, &rt, FALSE);
            break;

        case SCANCODE_SHUTDOWN: {
            RECT rt;
            hCurFocus = GetFocus(hDlg);
            hFocus = GetNextDlgTabItem(hDlg, hCurFocus, FALSE);
            dlgID = GetDlgCtrlID(hFocus);
            SetFocus(hFocus);

            lic_chinese_idx = licplate_pos[0];

            if (dlgID != IDC_BUTTON_OK)
                lic_num_idx = licplate_pos[dlgID + 1];

            GetClientRect(hDlg, &rt);
            InvalidateRect(hDlg, &rt, FALSE);
        }

        break;
        }

        ReleaseDC(hdc);
        break;
    }
    }

    return DefaultDialogProc(hDlg, message, wParam, lParam);
}

static void set_dv_licenseplate(void)
{
    int lang = parameter_get_video_lan();

    DLGTEMPLATE DlgMyLiense = {WS_VISIBLE,
                               WS_EX_NONE,
                               LICN_WIN_X,
                               LICN_WIN_Y,
                               LICN_WIN_W,
                               LICN_WIN_H,
                               SetLicense,
                               0,
                               0,
                               10,
                               NULL,
                               0
                              };
    CTRLDATA CtrlMyLicense[] = {
        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE,
            LICN_STR_X, LICN_STR_Y, LICN_STR_W, LICN_STR_H, IDC_STATIC,
            info_text[lang][TEXT_LICENSE_PLATE], 0, 0, NULL, NULL
        },

        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE | WS_TABSTOP,
            LICN_RECT_X, LICN_RECT_Y, LICN_RECT_W, LICN_RECT_H, IDC_STATIC1,
            "*", 0, 0, NULL, NULL
        },

        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE | WS_TABSTOP,
            LICN_RECT_X + LICN_RECT_GAP, LICN_RECT_Y, LICN_RECT_W, LICN_RECT_H, IDC_STATIC2,
            "*", 0, 0, NULL, NULL
        },

        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE | WS_TABSTOP,
            LICN_RECT_X + LICN_RECT_GAP * 2, LICN_RECT_Y, LICN_RECT_W, LICN_RECT_H, IDC_STATIC3,
            "*", 0, 0, NULL, NULL
        },

        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE | WS_TABSTOP,
            LICN_RECT_X + LICN_RECT_GAP * 3, LICN_RECT_Y, LICN_RECT_W, LICN_RECT_H, IDC_STATIC4,
            "*", 0, 0, NULL, NULL
        },

        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE | WS_TABSTOP,
            LICN_RECT_X + LICN_RECT_GAP * 4, LICN_RECT_Y, LICN_RECT_W, LICN_RECT_H, IDC_STATIC5,
            "*", 0, 0, NULL, NULL
        },

        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE | WS_TABSTOP,
            LICN_RECT_X + LICN_RECT_GAP * 5, LICN_RECT_Y, LICN_RECT_W, LICN_RECT_H, IDC_STATIC6,
            "*", 0, 0, NULL, NULL
        },

        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE | WS_TABSTOP,
            LICN_RECT_X + LICN_RECT_GAP * 6, LICN_RECT_Y, LICN_RECT_W, LICN_RECT_H, IDC_STATIC7,
            "*", 0, 0, NULL, NULL
        },

        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE | WS_TABSTOP,
            LICN_RECT_X + LICN_RECT_GAP * 7, LICN_RECT_Y, LICN_RECT_W, LICN_RECT_H, IDC_STATIC8,
            "*", 0, 0, NULL, NULL
        },

        {
            CTRL_BUTTON,
            WS_VISIBLE | BS_DEFPUSHBUTTON | BS_PUSHBUTTON | WS_TABSTOP,
            LICN_BUTTON_OK_X, LICN_BUTTON_OK_Y, LICN_BUTTON_OK_W, LICN_BUTTON_OK_H, IDC_BUTTON_OK,
            "OK", 0, 0, NULL, NULL
        },
    };

    DlgMyLiense.controls = CtrlMyLicense;
    DialogBoxIndirectParam(&DlgMyLiense, hMainWnd, license_plate_proc, 0L);
}
*/

static BOOL proc_gyro_timer(HWND hWnd, int id, DWORD time)
{
    if (gyro_calibrate_succ < 1) {
        gyro_calibrate_succ = 1;
        InvalidateRect(hWnd, NULL, TRUE);
        KillTimer(hWnd, id);
        EndDialog(hWnd, id);
        return TRUE;
    }

    return FALSE;
}

#define TIMER_1 1000
#define TIMER_2 1001
static int dv_gyro_proc (HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    RECT rcDraw;
    int lang = parameter_get_video_lan();

    switch (message) {
    case MSG_INITDIALOG:
        SetTimer (hDlg, TIMER_1, 300);

        SetWindowBkColor(hDlg, GetWindowElementPixel(hMainWnd, WE_BGC_DESKTOP));
        SetWindowElementAttr(hDlg, WE_BGCA_ACTIVE_CAPTION, COLOR_black);
        return 1;

    case MSG_COMMAND:
        break;

    case MSG_TIMER:
        if (wParam == TIMER_1) {
            SendMessage(hDlg, MSG_KEYDOWN, SCANCODE_S, 0L);
            return 1;
        }

        if (wParam == TIMER_2)
            EndDialog(hDlg, wParam);

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

        if (gyro_calibrate_succ == 0)
            DrawText(hdc, info_text[lang][TEXT_CALIBRATION_DONE],
                     -1, &rcDraw, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

        else if (gyro_calibrate_succ < 0)
            DrawText(hdc, info_text[lang][TEXT_CALIBRATION_ERR],
                     -1, &rcDraw, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

        EndPaint(hDlg, hdc);
        break;

    case MSG_KEYDOWN:
        switch (wParam) {
        case SCANCODE_S:
            KillTimer(hDlg, TIMER_1);
            gyro_calibrate_succ = gyrosensor_calibration(GYRO_DIRECT_CALIBRATION);

            SetTimerEx(hDlg, TIMER_2, 300, proc_gyro_timer);
            InvalidateRect(hDlg, NULL, TRUE);
            break;

        case SCANCODE_ENTER:
            if (gyro_calibrate_succ < 1)
                gyro_calibrate_succ = 1;

            audio_sync_play(KEYPRESS_SOUND_FILE);
            EndDialog(hDlg, wParam);
            break;
        }

        break;
    }

    return DefaultDialogProc (hDlg, message, wParam, lParam);
}

static void set_dv_gyro_calibration(void)
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

    DialogBoxIndirectParam(&dlgcalibration, hMainWnd, dv_gyro_proc, 0L);
}

void draw_list_of_setting(HWND hWnd, HDC hdc)
{
    char buf[10];
    int i, pag_start_idx = 0, max_page_num = 0;
    int line_pos = 0, line_select_pos = 0;
    struct settig_item_st *setting_item;
    int lang = (int)parameter_get_video_lan();

    SetBkColor(hdc, GetWindowElementPixel(hMainWnd, WE_BGC_TOOLTIP));
    SetTextColor(hdc, PIXEL_lightwhite);
    SelectFont(hdc, (PLOGFONT)GetWindowElementAttr(hWnd, WE_FONT_MENU));
    SetPenColor(hdc, PIXEL_darkgray);

    max_page_num = (MAX_SETTING_ITEM + MAX_PAGE_LINE_NUM - 1) / MAX_PAGE_LINE_NUM;
    sprintf(buf, "%d/%d", page_index, max_page_num);
    TextOut(hdc, DV_VIEW_PAG_X, DV_VIEW_PAG_Y, buf);

    for (i = 0; i < MAX_SETTING_ITEM; ) {
        setting_item = &setting_list[i];

        if (setting_item->item_page_num == page_index) {
            MoveTo(hdc, 0, DV_LIST_SET_IMG_Y + line_pos);
            LineTo(hdc, WIN_W, DV_LIST_SET_IMG_Y + line_pos);

            FillBoxWithBitmap(hdc, DV_LIST_BMP_IMG_X, DV_LIST_BMP_IMG_Y + line_pos,
                              DV_LIST_BMP_IMG_W, DV_LIST_BMP_IMG_H, &dv_video_all[i + SET_ICON_PPI_IMG_ID]);

            TextOut(hdc, DV_LIST_BMP_TX_X, DV_LIST_BMP_TX_Y + line_pos,
                    dv_set_text[lang][i]);
            FillBoxWithBitmap(hdc, DV_LIST_BMP_ARR_X, DV_LIST_BMP_ARR_Y + line_pos,
                              DV_LIST_BMP_ARR_W, DV_LIST_BMP_ARR_H, &dv_video_all[SET_ICON_MORE_IMG_ID]);
            line_pos = line_pos + KEY_ENTER_BOX_H;
            i++;
        } else {
            i += MAX_PAGE_LINE_NUM;
            line_pos = 0;
        }
    }

    pag_start_idx = (page_index - 1) * MAX_PAGE_LINE_NUM;
    line_select_pos = pag_start_idx + item_index;
    line_pos = item_index * KEY_ENTER_BOX_H;

    FillBoxWithBitmap(hdc, DV_REC_X, DV_REC_Y + line_pos, DV_REC_W, DV_REC_H,
                      &dv_video_all[BOX_HIGHLIGHT_SEL_IMG_ID]);
    FillBoxWithBitmap(hdc, DV_LIST_BMP_IMG_X, DV_LIST_BMP_IMG_Y + line_pos,
                      DV_LIST_BMP_IMG_W, DV_LIST_BMP_IMG_H,
                      &dv_video_all[line_select_pos + SET_ICON_PPI_IMG_ID]);
    FillBoxWithBitmap(hdc, DV_LIST_BMP_ARR_X, DV_LIST_BMP_ARR_Y + line_pos,
                      DV_LIST_BMP_ARR_W, DV_LIST_BMP_ARR_H,
                      &dv_video_all[SET_ICON_MORE_IMG_ID]);
}

void dv_setting_item_paint(HDC hdc, HWND hWnd)
{
    int i;
    struct settig_item_st *setting_item;

    for (i = 0; i < MAX_SETTING_ITEM; i++) {
        setting_item = &setting_list[i];

        if ((setting_item->item_selected == 1)
            && (setting_item->show_submenu_fuc) != NULL) {
            (*(setting_item->show_submenu_fuc))(hdc, hWnd);
            break;
        }
    }
}

void dv_setting_key_up(HWND hWnd)
{
    int i = 0, max_page_num = 0;
    struct settig_item_st *setting_item;

    max_page_num = (MAX_SETTING_ITEM + MAX_PAGE_LINE_NUM - 1) / MAX_PAGE_LINE_NUM;

    if (key_enter_list == 1) {
        for (i = 0; i < MAX_SETTING_ITEM; i++) {
            setting_item = &setting_list[i];

            if (setting_item->item_selected == 1) {
                if (setting_item->sub_menu_page <= 1) {
                    if (setting_item->sub_sel_num < setting_item->sub_menu_max - 1)
                        setting_item->sub_sel_num++;
                    else if (setting_item->sub_sel_num == setting_item->sub_menu_max - 1)
                        setting_item->sub_sel_num = 0;
                }
            }
        }
    } else if (key_enter_list == 0) {
        if (page_index <= max_page_num - 1) {
            if (item_index < MAX_PAGE_LINE_NUM - 1) {
                item_index ++;
            } else if (item_index == MAX_PAGE_LINE_NUM - 1) {
                item_index = 0;
                page_index ++;
            }
        } else if (page_index == max_page_num) {
            if (item_index < (MAX_PAGE_LINE_NUM - 1)) {
                int idx = MAX_SETTING_ITEM % MAX_PAGE_LINE_NUM;
                item_index ++;

                if (item_index == ((idx > 0) ? idx : MAX_PAGE_LINE_NUM - 1)) {
                    item_index = 0;
                    page_index = 1;
                }
            } else if (item_index == (MAX_PAGE_LINE_NUM - 1)) {
                item_index = 0;
                page_index = 1;
            }
        }
    }

    InvalidateRect(hWnd, NULL, TRUE);
}

void dv_setting_key_down(HWND hWnd)
{
    int i = 0, max_page_num = 0;
    struct settig_item_st *setting_item;

    max_page_num = (MAX_SETTING_ITEM + MAX_PAGE_LINE_NUM - 1) / MAX_PAGE_LINE_NUM;

    if (key_enter_list == 1) {
        for (i = 0; i < MAX_SETTING_ITEM; i++) {
            setting_item = &setting_list[i];

            if (setting_item->item_selected == 1) {
                if (setting_item->sub_menu_page == 1) {
                    if (setting_item->sub_sel_num >= 1)
                        setting_item->sub_sel_num--;
                    else if (setting_item->sub_sel_num == 0)
                        setting_item->sub_sel_num = setting_item->sub_menu_max - 1;
                }
            }
        }
    } else if (key_enter_list == 0) {
        if (page_index > 1 && page_index <= max_page_num) {
            if (item_index >= 1) {
                item_index --;
            } else if (item_index == 0) {
                item_index = MAX_PAGE_LINE_NUM - 1;
                page_index --;
            }
        } else if (page_index == 1) {
            if (item_index >= 1) {
                item_index --;
            } else if (item_index == 0) {
                int idx = MAX_SETTING_ITEM % MAX_PAGE_LINE_NUM;
                item_index =  (idx > 0) ? (idx - 1) : (MAX_PAGE_LINE_NUM - 1);
                page_index = max_page_num;
            }
        }
    }

    InvalidateRect(hWnd, NULL, TRUE);
}

bool dv_setting_is_show_submenu (void)
{
    int sel_func_index;
    struct settig_item_st *setting_item;

    sel_func_index = (page_index - 1) * MAX_PAGE_LINE_NUM + item_index;
    setting_item = &setting_list[sel_func_index];

    if (setting_item->item_selected)
        return true;
    else
        return false;
}

void dv_setting_key_enter(HWND hWnd)
{
    int sel_func_index;
    struct settig_item_st *setting_item;

    sel_func_index = (page_index - 1) * MAX_PAGE_LINE_NUM + item_index;
    setting_item = &setting_list[sel_func_index];

    if (key_enter_list == 0) {
        key_enter_list = 1;

        switch ( sel_func_index ) {
        case DATE:
            key_enter_list = 0;
            set_dv_date();
            return;

//        case PLATE:
//            key_enter_list = 0;
//            set_dv_licenseplate();
//            return;

        case RECOVERY:
            key_enter_list = 0;
            set_dv_recovery(hWnd);
            return;

        case FORMAT:
            key_enter_list = 0;
            set_dv_format(hWnd);
            return;

        case FW_VERSION:
            key_enter_list = 0;
            show_dv_fw_version(hWnd);
            return;

        case FW_UPGRADE:
            key_enter_list = 0;
            set_dv_fwupgrade(hWnd);
            return;

        case DV_DBG:
            debug_flag = 1;
            key_enter_list = 0;
            InvalidateRect(hWnd, NULL, TRUE);
            return;
        }

        setting_item->item_selected = 1;
    } else if (key_enter_list) {
        key_enter_list = 0;

        /*
        * Before setting the parameters,add judgement the parameter to avoiding
        * to write flash sector much times.
        */
        if (setting_item->item_selected) {
            int submenu_index;
            submenu_index =  setting_item->sub_sel_num;

            switch (sel_func_index) {
            case PPI:
                if (parameter_get_video_fontcamera() != submenu_index)
                    api_set_front_rec_resolution(submenu_index);

                break;

            case GYROSCOPE:
                if (submenu_index < 2) {
                    /* open / close DV anti-shake */
                    if (parameter_get_dvs_enabled() != submenu_index)
                        api_set_video_dvs_onoff(submenu_index);
                } else if (submenu_index == 2) {
                    /* gyrosensor calibration */
                    set_dv_gyro_calibration();
                }

                break;

            case LANGUAGE:
                if (parameter_get_video_lan() != (char)submenu_index)
                    api_set_language(submenu_index);

                break;

            case LIGHT:
                if (get_dv_backlight() != submenu_index)
                    set_dv_backlight(submenu_index);

                break;

            case MIC:
                if (parameter_get_video_audio() != (char)submenu_index)
                    api_mic_onoff(submenu_index);

                break;

            case KEY_TONE:
                if (get_dv_key_tone() != submenu_index)
                    set_dv_key_tone(submenu_index);

                break;

            case WATERMARK:
                if (parameter_get_video_mark() != (char)submenu_index)
                    api_video_mark_onoff(submenu_index);

                break;

            case AWB:
                if (parameter_get_wb() != (char)submenu_index)
                    api_set_white_balance(submenu_index);

                break;

            case WIFI:
                if (submenu_index == 0) {
                    if (parameter_get_wifi_en() == 1) {
                        parameter_save_wifi_en(0);
                        wifi_management_stop();
                    }
                } else if (submenu_index == 1) {
                    if (parameter_get_wifi_en() == 0) {
                        parameter_save_wifi_en(1);
                        wifi_management_start();
                    }

                    dv_show_wifi = 1;
                }

                break;

            case HZ:
                if (parameter_get_video_fre() != (char)(CAMARE_FREQ_50HZ + submenu_index))
                    api_set_video_freq(CAMARE_FREQ_50HZ + submenu_index);

                break;

            case SCREEN_OFF:
                if (parameter_get_screenoff_time() < 0 && submenu_index == 0)
                    break;
                else if (parameter_get_screenoff_time() > 0 && submenu_index == 1)
                    break;

                if (submenu_index == 0)
                    api_set_video_autooff_screen(VIDEO_AUTO_OFF_SCREEN_OFF);
                else if (submenu_index == 1)
                    api_set_video_autooff_screen(VIDEO_AUTO_OFF_SCREEN_ON);

                break;

            case AUTO_REC:
                if (parameter_get_video_autorec() != (char)submenu_index)
                    api_video_autorecode_onoff(submenu_index);

                break;

            case REC_TIME:
                if (get_dv_record_time() != submenu_index)
                    set_dv_record_time(submenu_index);

                storage_setting_event_callback(0, NULL, NULL);
                break;

            case VIDEO_QUALITY:
                if (get_dv_video_quality() != submenu_index)
                    set_dv_video_quality(submenu_index);

                break;

//            case CAR_MODE:
//               if (parameter_get_video_carmode() != (char)submenu_index) {
//                    /* If open car mode and record time is unlimited,set record time to 60s. */
//                    if (submenu_index && get_dv_record_time() == 3)
//                        set_dv_record_time(0);
//
//                    api_set_car_mode_onoff(submenu_index);
//                    /* Open auto record */
//                    api_video_autorecode_onoff(submenu_index);
//                }
//                break;
            }

            setting_item->item_selected = 0;
        }
    }

    InvalidateRect(hWnd, NULL, TRUE);
}

void dv_setting_key_stdown(void)
{
    int i;
    struct settig_item_st *setting_item;

    if (key_enter_list == 1)
        key_enter_list = 0;

    for (i = 0; i < MAX_SETTING_ITEM; i++) {
        setting_item = &setting_list[i];

        if (setting_item->item_selected == 1)
            setting_item->item_selected = 0;
    }
}

void dv_setting_init(void)
{
    /* According to parameter to init setting. */
    setting_list[PPI].sub_sel_num = get_dv_video_mp();
    setting_list[GYROSCOPE].sub_sel_num = parameter_get_dvs_enabled();
    setting_list[LANGUAGE].sub_sel_num = parameter_get_video_lan();
    setting_list[LIGHT].sub_sel_num = get_dv_backlight();
    setting_list[MIC].sub_sel_num = parameter_get_video_audio();
    setting_list[AWB].sub_sel_num = parameter_get_wb();
    setting_list[WIFI].sub_sel_num = parameter_get_wifi_en();
    setting_list[WATERMARK].sub_sel_num = parameter_get_video_mark();
    setting_list[HZ].sub_sel_num = parameter_get_video_fre() - CAMARE_FREQ_50HZ;

    if (parameter_get_screenoff_time() < 0)
        setting_list[SCREEN_OFF].sub_sel_num = 0;
    else
        setting_list[SCREEN_OFF].sub_sel_num = 1;

    setting_list[AUTO_REC].sub_sel_num = parameter_get_video_autorec();
    setting_list[REC_TIME].sub_sel_num = get_dv_record_time();
    setting_list[VIDEO_QUALITY].sub_sel_num = get_dv_video_quality();
//    setting_list[CAR_MODE].sub_sel_num = parameter_get_video_carmode();
    setting_list[KEY_TONE].sub_sel_num = get_dv_key_tone();
}
