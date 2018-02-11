/*
** $Id: helloworld.c 793 2010-07-28 03:36:29Z dongjunjie $
**
** Listing 1.1
**
** helloworld.c: Sample program for MiniGUI Programming Guide
**      The first MiniGUI application.
**
** Copyright (C) 2004 ~ 2009 Feynman Software.
**
** License: GPL
*/

#include <stdio.h>
#include <string.h>

#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <pwd.h>
#include <errno.h>
#include <signal.h>
#include <linux/watchdog.h>

#include <time.h>
#include <init_hook/init_hook.h>
#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>
#include <minigui/rkfb.h>
#ifdef _LANG_ZHCN
#include "camera_ui_res_cn.h"
#elif defined _LANG_ZHTW
#include "camera_ui_res_tw.h"
#else
#include "camera_ui_res_en.h"
#endif

#include "camera_ui.h"
#include "camera_ui_def.h"
#include "ueventmonitor/ueventmonitor.h"
#include "ueventmonitor/usb_sd_ctrl.h"
#include "wifi_management.h"
#include "parameter.h"
#include "fs_manage/fs_manage.h"
#include "fs_manage/fs_storage.h"
#include "fs_manage/fs_sdcard.h"
#include "fs_manage/fs_sdv.h"

#include "common.h"
#include "videoplay.h"
#include <dpp/algo/adas/adas.h>


#include "audio/playback/audioplay.h"
#include <assert.h>

#include <pthread.h>

#include "ui_resolution.h"
#include "licenseplate.h"
#include "watermark.h"
#include "video.h"
#include "example/user.h"
#include "videoplay.h"
#include "gsensor.h"
#include "power/thermal.h"
#include "collision/collision.h"
#include "parking_monitor/parking_monitor.h"
#include "adas/adas_process.h"

#include "fwk_protocol/rk_protocol.h"
#include "fwk_protocol/rk_fwk.h"
#include "storage/storage.h"
#include "public_interface.h"
#include "msg_list_manager.h"

#include "usb_mode.h"
#include "display_cvr.h"
#include "show.h"

#include "gps/nmea_parse.h"
#include "uvc/uvc_user.h"
#include "screen_capture/screen_capture.h"

#include "process/face_interface.h"

#ifdef USE_RIL_MOUDLE
#include "ril_event.h"
#endif

#ifdef USE_SPEECHREC
#include "speechrec/speechrec.h"
#endif
#include <autoconfig/main_app_autoconf.h>
#include "uvc_process.h"

//#define USE_WATCHDOG

#define BLOCK_PREV_NUM 1   // min
#define BLOCK_LATER_NUM 1  // min

//USB
#define USB_MODE 1
#define BATTERY_CUSHION 3
#define USE_KEY_STOP_USB_DIS_SHUTDOWN

#define PARKINGMONITOR_OFF      0
#define PARKINGMONITOR_SHUTDOWN_MODE    1
#define PARKINGMONITOR_SUSPEND_MODE 2
#define PARKINGMONITOR_MOTION_DETECT    3
#define PARKINGMONITOR_TIMELAPSE    4
#define PARKING_RECORD_COUNT        90
#define MOTION_DETECT_COUNT     30
#define TIMELAPSE_RECORD_COUNT      30
#define PARKING_SUSPEND         1
#define PARKING_SHUTDOWN        2

static int test_replay;
static char testpath[256];
static struct adas_process_output g_adas_process;
static short screenoff_time;

//battery stable
static int last_battery = 1;

static BITMAP A_bmap[3];

RECT msg_rcAB = {A_IMG_X, A_IMG_Y, A_IMG_X + A_IMG_W, A_IMG_Y + A_IMG_H};

RECT adas_rc = {adas_X, adas_Y, adas_W, adas_H};

RECT msg_rcMove = {MOVE_IMG_X, MOVE_IMG_Y, MOVE_IMG_X + MOVE_IMG_W, MOVE_IMG_Y + MOVE_IMG_H};


RECT USB_rc = {USB_IMG_X, USB_IMG_Y, USB_IMG_X + USB_IMG_W,
               USB_IMG_Y + USB_IMG_H
              };

RECT msg_rcBtt = {BATT_IMG_X, BATT_IMG_Y, BATT_IMG_X + BATT_IMG_W,
                  BATT_IMG_Y + BATT_IMG_H
                 };

static BITMAP batt_bmap[5];

RECT msg_rcSD = {TF_IMG_X, TF_IMG_Y, TF_IMG_X + TF_IMG_W,
                 TF_IMG_Y + TF_IMG_H
                };

static BITMAP tf_bmap[2];

RECT msg_rcMode = {MODE_IMG_X, MODE_IMG_Y, MODE_IMG_X + MODE_IMG_W,
                   MODE_IMG_Y + MODE_IMG_H
                  };

static BITMAP mode_bmap[4];

RECT msg_rcResolution = {RESOLUTION_IMG_X, RESOLUTION_IMG_Y,
                         RESOLUTION_IMG_X + RESOLUTION_IMG_W,
                         RESOLUTION_IMG_Y + RESOLUTION_IMG_H
                        };

static BITMAP resolution_bmap[2];

static BITMAP move_bmap[2];

static BITMAP topbk_bmap;

RECT msg_rcTime = {REC_TIME_X, REC_TIME_Y, REC_TIME_X + REC_TIME_W,
                   REC_TIME_Y + REC_TIME_H
                  };

RECT msg_rcRecimg = {REC_IMG_X, REC_IMG_Y, REC_IMG_X + REC_IMG_W,
                     REC_IMG_Y + REC_IMG_H
                    };
static BITMAP recimg_bmap;

RECT msg_motiondetect = {MOTIONDETECT_IMG_X,
                         MOTIONDETECT_IMG_Y,
                         MOTIONDETECT_IMG_X + MOTIONDETECT_IMG_W,
                         MOTIONDETECT_IMG_Y + MOTIONDETECT_IMG_H,
                        };
static BITMAP motiondetect_bmap;

RECT msg_motiondetect_on = {MOTIONDETECT_ON_IMG_X,
                            MOTIONDETECT_ON_IMG_Y,
                            MOTIONDETECT_ON_IMG_X + MOTIONDETECT_ON_IMG_W,
                            MOTIONDETECT_ON_IMG_Y + MOTIONDETECT_ON_IMG_H,
                           };
static BITMAP motiondetect_on_bmap;

RECT msg_timeLapseimg = {TIMELAPSE_IMG_X,
                         TIMELAPSE_IMG_Y,
                         TIMELAPSE_IMG_X + TIMELAPSE_IMG_W,
                         TIMELAPSE_IMG_Y + TIMELAPSE_IMG_H
                        };
static BITMAP timelapse_bmap;

RECT msg_timeLapse_on_img = {TIMELAPSE_ON_IMG_X,
                             TIMELAPSE_ON_IMG_Y,
                             TIMELAPSE_ON_IMG_X + TIMELAPSE_ON_IMG_W,
                             TIMELAPSE_ON_IMG_Y + TIMELAPSE_ON_IMG_H
                            };
static BITMAP timelapse_on_bmap;

RECT msg_rcMic = {MIC_IMG_X, MIC_IMG_Y, MIC_IMG_X + MIC_IMG_W,
                  MIC_IMG_Y + MIC_IMG_H
                 };

static BITMAP mic_bmap[2];

RECT msg_rcSDCAP = {TFCAP_STR_X, TFCAP_STR_Y, TFCAP_STR_X + TFCAP_STR_W,
                    TFCAP_STR_Y + TFCAP_STR_H
                   };

RECT msg_rcSYSTIME = {SYSTIME_STR_X, SYSTIME_STR_Y,
                      SYSTIME_STR_X + SYSTIME_STR_W,
                      SYSTIME_STR_Y + SYSTIME_STR_H
                     };

RECT msg_rcFILENAME = {FILENAME_STR_X, FILENAME_STR_Y,
                       FILENAME_STR_X + FILENAME_STR_W,
                       FILENAME_STR_Y + FILENAME_STR_H
                      };

RECT msg_rcWifi = {WIFI_IMG_X, WIFI_IMG_Y, WIFI_IMG_X + WIFI_IMG_W,
                   WIFI_IMG_Y + WIFI_IMG_H
                  };

static BITMAP wifi_bmap[5];

RECT msg_rcWatermarkTime = {WATERMARK_TIME_X, WATERMARK_TIME_Y,
                            WATERMARK_TIME_X + WATERMARK_TIME_W,
                            WATERMARK_TIME_Y + WATERMARK_TIME_H
                           };

RECT msg_rcWatermarkLicn = {WATERMARK_LICN_X, WATERMARK_LICN_Y,
                            WATERMARK_LICN_X + WATERMARK_LICN_W,
                            WATERMARK_LICN_Y + WATERMARK_LICN_H
                           };

RECT msg_rcWatermarkImg = {WATERMARK_IMG_X, WATERMARK_IMG_Y,
                           WATERMARK_IMG_X + WATERMARK_IMG_W,
                           WATERMARK_IMG_Y + WATERMARK_IMG_H
                          };

struct bitmap watermark_bmap[7];

/* gps info */
RECT msg_rcGpsState = {GPS_STATUS_X, GPS_STATUS_Y,
                       GPS_STATUS_X + GPS_STATUS_W,
                       GPS_STATUS_Y + GPS_STATUS_H
                      };

RECT msg_rcGpsSatellite = {GPS_SATELLITE_X, GPS_SATELLITE_Y,
                           GPS_SATELLITE_X + GPS_SATELLITE_W,
                           GPS_SATELLITE_Y + GPS_SATELLITE_H
                          };

RECT msg_rcGpsSpeed = {GPS_SPEED_X, GPS_SPEED_Y,
                       GPS_SPEED_X + GPS_SPEED_W,
                       GPS_SPEED_Y + GPS_SPEED_H
                      };

RECT msg_rcGpsLatitude = {GPS_LATITUDE_X, GPS_LATITUDE_Y,
                          GPS_LATITUDE_X + GPS_LATITUDE_W,
                          GPS_LATITUDE_Y + GPS_LATITUDE_H
                         };

RECT msg_rcGpsLongitude = {GPS_LONGITUDE_X, GPS_LONGITUDE_Y,
                           GPS_LONGITUDE_X + GPS_LONGITUDE_W,
                           GPS_LONGITUDE_Y + GPS_LONGITUDE_H
                          };

RECT msg_rcGpsTime = {GPS_TIME_X, GPS_TIME_Y,
                      GPS_TIME_X + GPS_TIME_W,
                      GPS_TIME_Y + GPS_TIME_H
                     };

const struct video_param cvr_video_front_resolution[_MAX_FRONT_VIDEO_RES] = {
#if USE_1440P_CAMERA
    UI_FRAME_1440P,
    UI_FRAME_1080P,
#else
    UI_FRAME_1080P,
    UI_FRAME_720P,
#endif
};

const struct video_param cvr_video_back_resolution[_MAX_BACK_VIDEO_RES] = {
#if USE_1440P_CAMERA
    UI_FRAME_1440P,
    UI_FRAME_1080P,
#else
    UI_FRAME_1080P,
    UI_FRAME_720P,
#endif
};

static BITMAP play_bmap;
static pthread_mutex_t set_mutex;

#define ADASFLAG_DRAW                   (1 << 0)
#define ADASFLAG_SETTING                (1 << 1)
#define ADASFLAG_OPTION_SELECTED        (7 << 2)
#define ADASFLAG_ALERT                  (1 << 5)

#define ADASFLAG_OPTION_HORIZONTAL      0
#define ADASFLAG_OPTION_HEAD            1
#define ADASFLAG_OPTION_DIRECTION       2
#define ADASFLAG_OPTION_DISTANCE        3

#define ADASFLAG_OPTION_COEFICIENT      4
#define ADASFLAG_OPTION_BASE            5

#define ADASFLAG_OPTION_MAX             ADASFLAG_OPTION_BASE

/*
 * bit 2: line[0] selected
 * bit 1: adas setting flag
 * bit 0: adas draw flag
 */
static int adasFlag = 0;
/*
 * We want just 1 alert sound for a continuous alarm.
 * If there is not alert event for 1 second, we thinks the alarm is
 * over, and we can sound the alarm voice if the new alert comes.
 * adas would output about 30 events per second.
 */
#define ADAS_ALERT_CNT                  30
static int AdasAlertCnt = 0;
static bool takephoto = false;

// menu
static BITMAP bmp_menu_mp[2];
static BITMAP bmp_menu_time[2];
static BITMAP bmp_menu_car[2];
static BITMAP bmp_menu_setting[2];
static BITMAP png_menu_debug[2];
static BITMAP folder_bmap;
static BITMAP back_bmap;
static BITMAP usb_bmap;
static BITMAP filetype_bmap[FILE_TYPE_MAX];

//------------------------------------
static const char* DSP_IDC[2] = {" IDC ",
                                 " 畸变校正 "
                                };
static const char* video_flip[2] = {" Flip ",
                                    " 视频翻转 "
                                   };
static const char* ui_cif_mirror[2] = {" CIF MIRROR ",
                                    " CIF 镜像 "
                                   };

static const char* DSP_3DNR[2] = {" 3DNR ",
                                  //    "night-vision enhance",
                                  " 夜视增强 "
                                 };
static const char* DISP_CVBSOUT[2] = { " CVBS ",
                                       " 切换CVBS显示 "
                                     };
static const char* DISP_CVBSOUT_PAL[2] = { " PAL ",
                                           " PAL制式 "
                                         };
static const char* DISP_CVBSOUT_NTSC[2] = { " NTSC ",
                                            " NTSC制式 "
                                          };
static const char* fontcamera_ui[2] = {" Front ", " 前置 "};
static const char* backcamera_ui[2] = {" Back ", " 后置 "};
static const char* cifcamera_ui[2] = {" CIF ", " cif "};

static const char* str_fw_update[2] = {" Firmware Update ", " 固件升级 "};
static const char* str_fw_update_test[2] = {" FW_UPDATE TEST ", " 固件升级测试 "};
static const char* reboot[2] = {" reboot ", " reboot "};
static const char* recovery_debug[2] = {" recovery ", " recovery拷机  "};
static const char* standby[2] = {" standby ", " 二级待机 "};
static const char* mode_change[2] = {" mode change ", " 模式切换 "};
static const char* video_debug[2] = {" Video ", " video(回放视频) "};
static const char* beg_end_video[2] = {" beg_end_video ", " 开始/结束录像 "};

static const char* photo_debug[2] = {" photo ", " 录像拍照 "};
static const char* temp_debug[2] = {" temp control ", " 温度控制 "};
static const char* effect_watermark[2] = {" effect watermark ", " 效果参数水印 "};
static const char* DSP_ADAS[2] = {" ADAS ",
                                  // "lane departure warning",
                                  " 车道偏离预警 "
                                 };

static const char* MP[2] = {
    //  "  MP ",
    "Resolution", "清晰度"
};
static const char* Record_Time[2] = {
    //  " Record_Time ",
    "Video length", " 录像时长 "
};
static const char* Time_OneMIN[2] = {" 1min ", " 1分钟 "};
static const char* Time_TMIN[2] = {" 3min ", " 3分钟 "};
static const char* Time_FMIN[2] = {" 5min ", " 5分钟 "};
static const char* OFF[2] = {" OFF ", " 关闭 "};
static const char* ON[2] = {" ON ", " 打开 "};
static const char* Record_Mode[2] = {
    //  " Record_Mode ",
    "Record Mode", " 录像模式 "
};
static const char* Front_Record[2] = {
    //  " Front_Record ",
    "Front video", " 前镜头录像 "
};
static const char* Back_Record[2] = {
    //  " Back_Record ",
    "Rear video", " 后镜头录像 "
};
static const char* Double_Record[2] = {
    //  " Double_Record ",
    "Double video", " 双镜头录像 "
};
static const char* setting[2] = {" Setting ", " 设置 "};
static const char* debug[2] = {" DEBUG ", " DEBUG "};

static const char* WB[2] = {" White Balance ", " 白平衡 "};
static const char* exposal[2] = {" Exposure ", " 曝光度 "};
#if ENABLE_MD_IN_MENU
static const char* auto_detect[2] = {" motion detection ", " 移动侦测 "};
#endif
static const char* water_mark[2] = {" Date Stamp ", " 日期标签 "};
#ifdef GPS
static const char* gps[2] = {" GPS ", " GPS "};
static const char* gps_watermark[2] = {" GPS Watermark ", " GPS 水印 "};
#endif
static const char* record[2] = {" Record Audio ", " 录影音频 "};
static const char* key_volume[2] = {" Key volume ", " 按键音量 "};
static const char* volume_0[2] = {" close ", " 关闭 "};
static const char* volume_1[2] = {" low ", " 低 "};
static const char* volume_2[2] = {" middle ", " 中 "};
static const char* volume_3[2] = {" hight ", " 高 "};
static const char* auto_record[2] = {" Boot record ", " 开机录像 "};
static const char* language[2] = {" language ", " 语言选择 "};
static const char* frequency[2] = {" frequency ", " 频率 "};
static const char* format[2] = {" Format ", " 格式化 "};
static const char* setdate[2] = {" Date Set ", " 设置日期 "};
static const char* usbmode[2] = {" USB MODE ", " USB MODE "};
static const char* fw_ver[2] = {" Version ", " 固件版本 "};
static const char* recovery[2] = {" Recovery ", " 恢复出厂设置 "};
static const char* Br_Auto[2] = {" auto ", " 自动 "};
static const char* Br_Sun[2] = {" Daylight ", " 日光 "};
static const char* Br_Flu[2] = {" fluorescence ", " 荧光 "};
static const char* Br_Cloud[2] = {" cloudysky ", " 阴天 "};
static const char* Br_tun[2] = {" tungsten ", " 灯光 "};
static const char* HzNum_50[2] = {" 50HZ ", " 50HZ "};

static const char* HzNum_60[2] = {" 60HZ ", " 60HZ "};

static const char* LAN_ENG[2] = {" English ", " English "};
static const char* LAN_CHN[2] = {" 简体中文 ", " 简体中文 "};

static const char* ExposalData_0[2] = {" -3 ", " -3 "};

static const char* ExposalData_1[2] = {" -2 ", " -2 "};

static const char* ExposalData_2[2] = {" -1 ", " -1 "};

static const char* ExposalData_3[2] = {"  0 ", "  0 "};

static const char* ExposalData_4[2] = {"  1 ", "  1 "};

static const char* backlight[2] = {" Bright ", " 亮度 "};

static const char* collision[2] = {" collision detect ", " 碰撞检测 "};
static const char* Collision_0[2] = {" close ", " 关闭 "};
static const char* Collision_1[2] = {" low ", " 低 "};
static const char* Collision_2[2] = {" middle ", " 中 "};
static const char* Collision_3[2] = {" hight ", " 高 "};
static const char* parking_monitor[2] = {" parking monitor ", " 停车录像 "};
static const char* parkingmonitor_Off[2] = {" off ", " 关闭停车录像 "};
static const char* Shutdown_On[2] = {" shutdown ", " 关机模式 "};
static const char* Motion_Detect[2] = {" motion ", " 运动感应 "};
static const char* Time_Lapse_Record[2] = {" timeLapse ", " 缩时录像 "};
static const char* screenoffset[2] = {" Auto Off Screen ", " 屏幕自动关闭 "};
static const char* vbit_rate_vals[6] = {" 1 ", " 2 ",  " 4 ",
                                        " 5 ", " 6 ", " 8 "
                                       };
static const char* vbit_rate_debug[2] = {" video bit rate per pixel ",
                                         " 视频单像素码率 "
                                        };
static const char* wifi[2] = {" WIFI ",
                              " WIFI "
                             };
static const char* time_lapse[LANGUAGE_NUM] =
{" Timelapse Record ", " 缩时录影 "};
static const char* time_off[2] = {" OFF ", " 关闭 "};
static const char* time_one_sec[LANGUAGE_NUM] = {" 1s space ", " 间隔1秒 "};
static const char* time_five_sec[LANGUAGE_NUM] = {" 5s space ", " 间隔5秒 "};
static const char* time_ten_sec[LANGUAGE_NUM] = {" 10s space ", " 间隔10秒 "};
static const char* time_thirty_sec[LANGUAGE_NUM] = {" 30s space ", " 间隔30秒 "};
static const char* time_sixty_sec[LANGUAGE_NUM] = {" 60s space ", " 间隔60秒 "};
static const char* video_quality[2] = {" video quality ", " 视频质量 "};
static const char* quality_hig[2]   = {" high ", " 高 "};
static const char* quality_mid[2]   = {" middle ", " 中 "};
static const char* quality_low[2]   = {" low ", " 低 "};
static const char* license_plate_watermark[2] = {" license plate watermark ", " 车牌水印 "};
static const char* hot_share[2] = {" Hot Shared ", " 热点共享 "};
static const char* phone_call[2] = {" call ", " 拨打电话 "};
static int battery;

static int cap;
static int SetMode;
static int SetCameraMP;
static HMENU hmnuFor3DNR;
static HMENU hmnuForADAS;

static HWND hWndUSB;
static HWND hMainWnd;
static HWND explorer_wnd;
static HMENU explorer_menu;
static HMENU hmnuForFRE;
static HMENU hmnuForLAN;
static HMENU hmnuForKeyVol;
static HMENU hmnuForAutoRE;
static HMENU hmnuForBr;
static HMENU hmnuForEx;
static HMENU hmnuForRe;
static HMENU hmnuForMark;
static HMENU hmnuMain;
static HWND hMainWndForPlay;
static HMENU hmnuForCOLLI;
static HMENU hmnuForLEAVEREC;
static HMENU hmnuForAutoOffScreen;
static HMENU hmnuForVideoQuality;
static HMENU hmnuForCVBSOUT;
static HMENU hmnuForHotShare;

//-------------------------
static int video_play_state;
static DWORD bkcolor;
static int sec;
static char* explorer_path;
static pthread_t explorer_update_tid;
static int explorer_update_run;
static int explorer_update_reupdate;

#define FILENAME_LEN 128
static char previewname[FILENAME_LEN];
static struct file_list* filelist;
static struct file_node* currentfile;
static int totalfilenum;
static int currentfilenum;
static int preview_isvideo;
static int videoplay;
static int adasflagtooff;

// TODO: file path should be read from config_file
// These files are copied from $ANDROID_PROJECT/frameworks/base/data/sounds.
static char* capture_sound_file = "/usr/local/share/sounds/camera_click.wav";
static char* keypress_sound_file = "/usr/local/share/sounds/KeypressStandard.wav";
static char* adasremind_sound_file = "/usr/local/share/sounds/AdasRemind.wav";

static struct video_param frontcamera[4], backcamera[4], cifcamera[4];
static int menucreated;
// debug
static int reboot_time;
static int recovery_time;
static int standby_time;
static int modechange_time;
static int video_time;
static int beg_end_video_time;
static int photo_time;
static int fwupdate_test_time;

// int modechange_pre_flage=0;
//sd
int param_sd;

/*SHUTDOWN PARAMS*/
static int gshutdown;

static int key_lock;
static pthread_mutex_t window_mutex = PTHREAD_MUTEX_INITIALIZER;

static char gparking;
static char gparking_mode_change;
static int gparking_gs_active;
static int gparking_count;
static int gmotion_detection;
static int gtimelapse_record;

static long int collision_rec_time;

static struct gps_info ggps_info;
static void gps_info_show(HDC hdc);
static void gps_ui_control(HWND hWnd, BOOL bEraseBkgnd);

static void initrec(HWND hWnd);
void changemode(HWND hWnd, int mode);
void exitvideopreview(void);
void loadingWaitBmp(HWND hWnd);
void startrec(HWND hWnd);
void stoprec(HWND hWnd);
void unloadingWaitBmp(HWND hWnd);

static int shutdown_deinit(HWND hWnd);
static int shutdown_usb_disconnect(HWND hWnd);
void ui_deinit(void);

enum {
    UI_FORMAT_BGRA_8888 = 0x1000,
    UI_FORMAT_RGB_565,
};

static BOOL InvalidateImg(HWND hWnd, const RECT* prc, BOOL bEraseBkgnd)
{
    BOOL ret;

    pthread_mutex_lock(&window_mutex);
    ret = InvalidateRect(hWnd, prc, bEraseBkgnd);
    pthread_mutex_unlock(&window_mutex);
    return ret;
}

const struct video_param *get_front_video_resolutions(void)
{
    return cvr_video_front_resolution;
}

const struct video_param *get_back_video_resolutions(void)
{
    return cvr_video_back_resolution;
}

/*SHUTDOWN FUNCTION*/
static int shutdown_deinit(HWND hWnd)
{
    if ((gshutdown == 1) || (gshutdown == 2)) {
        gshutdown = 2; //tell usb disconnect shutdown
        return -1;
    }
    gshutdown = 1;

    api_power_shutdown();

    return 0;
}

/* parking record video shutdown or suspend */
static int parking_record_process(int state)
{
    if (state == PARKING_SUSPEND) {
        printf("parking_record_process: suspend\n");
        changemode(hMainWnd, MODE_SUSPEND);
    } else if (state == PARKING_SHUTDOWN) {
        printf("parking_record_process: shutdown\n");
        shutdown_deinit(hMainWnd);
    }

    return 0;
}

static void gps_info_show(HDC hdc)
{
    char info[MAX_GPS_INFO_LEN];
    double gcjLat , gcjLng;

    SetBkColor (hdc, bkcolor);
    wgs2gcj(ggps_info.latitude, ggps_info.longitude, &gcjLat, &gcjLng);

    memset(info, 0, MAX_GPS_INFO_LEN);
    snprintf(info, sizeof(info), "status: %c\n", ggps_info.status);
    TextOut (hdc, GPS_STATUS_X, GPS_STATUS_Y, info);

    memset(info, 0, MAX_GPS_INFO_LEN);
    snprintf(info, sizeof(info), "Satellites: %02d\n", ggps_info.satellites);
    TextOut (hdc, GPS_SATELLITE_X, GPS_SATELLITE_Y, info);

    memset(info, 0, MAX_GPS_INFO_LEN);
    snprintf(info, sizeof(info), "Speed: %lf km/h\n", ggps_info.speed);
    TextOut (hdc, GPS_SPEED_X, GPS_SPEED_Y, info);

    memset(info, 0, MAX_GPS_INFO_LEN);
    snprintf(info, sizeof(info), "Latitude: %c-%lf\n",
            ggps_info.latitude_indicator, gcjLat);
    TextOut (hdc, GPS_LATITUDE_X, GPS_LATITUDE_Y, info);

    memset(info, 0, MAX_GPS_INFO_LEN);
    snprintf(info, sizeof(info), "Longitude: %c-%lf\n",
            ggps_info.longitude_indicator, gcjLng);
    TextOut (hdc, GPS_LONGITUDE_X, GPS_LONGITUDE_Y, info);

    memset(info, 0, MAX_GPS_INFO_LEN);
    snprintf(info, sizeof(info), "Time: %04d-%02d-%02d %02d:%02d:%02d\n",
            ggps_info.date.years, ggps_info.date.months, ggps_info.date.day,
            ggps_info.date.hour, ggps_info.date.min, ggps_info.date.sec);
    TextOut (hdc, GPS_TIME_X, GPS_TIME_Y, info);
}

static void gps_ui_control(HWND hWnd, BOOL bEraseBkgnd)
{
    InvalidateImg(hWnd, &msg_rcGpsState, bEraseBkgnd);
    InvalidateImg(hWnd, &msg_rcGpsSatellite, bEraseBkgnd);
    InvalidateImg(hWnd, &msg_rcGpsSpeed, bEraseBkgnd);
    InvalidateImg(hWnd, &msg_rcGpsLatitude, bEraseBkgnd);
    InvalidateImg(hWnd, &msg_rcGpsLongitude, bEraseBkgnd);
    InvalidateImg(hWnd, &msg_rcGpsTime, bEraseBkgnd);
}

int collision_event_callback(int cmd, void *msg0, void *msg1)
{
    switch (cmd) {
    case CMD_COLLISION:
#ifdef MAIN_APP_CACHE_ENCODEDATA_IN_MEM
        PostMessage(hMainWnd, MSG_COLLISION,
                    (WPARAM)msg0, (LPARAM)msg1);

        return COLLISION_CACHE_DURATION;
#else
        if ((SetMode == MODE_RECORDING) &&
            (api_video_get_record_state() == VIDEO_STATE_RECORD)) {
            video_record_blocknotify(BLOCK_PREV_NUM, BLOCK_LATER_NUM);
            if (collision_rec_time >= COLLISION_CACHE_DURATION) {
                video_record_savefile();
                collision_rec_time = 0;
                return COLLISION_CACHE_DURATION;
            }

            return (COLLISION_CACHE_DURATION - collision_rec_time);
        }
#endif
        break;
    default:
        break;
    }

    return 0;
}

void parking_event_callback(int cmd, void *msg0, void *msg1)
{
    switch (cmd) {
    case CMD_PARKING:
        gparking_gs_active = 1;
        break;
    default:
        break;
    }
}

#ifdef USE_SPEECHREC
void speechrec_event_callback(int cmd, void *msg0, void *msg1)
{
    int rectype;
    char wav_path[40];
    char* speechrec_sound_file[SPEECH_END] = {
        "wav/wakeup.wav",
        "wav/startrec.wav",
        "wav/stoprec.wav",
        "wav/takepic.wav",
        "wav/lockfile.wav",
        "wav/startbackcar.wav",
        "wav/stopbackcar.wav",
        "wav/startscreenoff.wav",
        "wav/stopscreenoff.wav"
    };

    sprintf(wav_path,"%s/%s", SPEECHREC_ETC_FILE_PATH, speechrec_sound_file[(int) msg0]);
    switch (cmd) {
    case CMD_SPEECH_REC:
        printf("###################play: *%s*\n", wav_path);
        if((int) msg0 < SPEECH_END )
            audio_sync_play(wav_path);
        break;
    default:
        break;
    }
}
#endif

#ifdef USE_RIL_MOUDLE

#include "ril_lib.h"

void ril_event_callback(int cmd, void *msg0, void *msg1)
{
    int rectype;

    switch (cmd) {
    case EVENT_READ_RIL_READ_MESSAGE:
        PostMessage(hMainWnd, MSG_RIL_RECV_MESSAGE, (WPARAM)msg0, (LPARAM)msg1);
        printf("ril_event_callback EVENT_READ_RIL_READ_MESSAGE %s\n", (char *)msg0);
        break;
    default:
        break;
    }
}
#endif

/*usb disconenct shutdown process
 */
#ifdef USE_KEY_STOP_USB_DIS_SHUTDOWN
int stop_usb_disconnect_poweroff()
{
    gshutdown = 3;
    return 0;
}
#endif

int usb_disconnect_poweroff(void *arg)
{
    int i;
    HWND hWnd = (HWND)arg;

    for (i = 0; i < 10; i++) {
        if (charging_status_check()) {
            gshutdown = 0;
            printf("%s[%d]:vbus conenct,cancel shutdown\n", __func__, __LINE__);
            return 0;
        }
        if (gshutdown == 2)//powerkey or low battery shutdown
            break;
#ifdef USE_KEY_STOP_USB_DIS_SHUTDOWN
        if (gshutdown == 3) {
            //key stop usb disconnect shutdown
            gshutdown = 0;
            printf("%s[%d]:key stop usb disconnect shutdown\n", __func__, __LINE__);
            return 0;
        }
#endif
        printf("%s[%d]:shutdown wait---%d\n", __func__, __LINE__, i);
        sleep(1);
    }
    printf("%s[%d]:gshutdown=%d,shutdown...\n", __func__, __LINE__, gshutdown);
    api_video_deinit(true);

    if (charging_status_check()) {
        gshutdown = 0;
        printf("%s[%d]:vbus conenct,cancel shutdown\n", __func__, __LINE__);
        video_record_deinit(false);
        printf("init rec---\n");
        initrec(hWnd);
        if ((api_video_get_record_state() == VIDEO_STATE_PREVIEW) && api_get_sd_mount_state() == SD_STATE_IN)
            api_start_rec();
        return 0;
    }

    api_power_shutdown();
    return 0;
}

void *usb_disconnect_process(void *arg)
{
    printf ("usb_disconnect_process\n");
    prctl(PR_SET_NAME, "usb_disconnect", 0, 0, 0);
    usb_disconnect_poweroff(arg);
    pthread_exit(NULL);
}

pthread_t run_usb_disconnect(HWND hWnd)
{
    pthread_t tid;
    printf ("run_usb_disconnect\n");
    if (pthread_create(&tid, NULL, usb_disconnect_process, (void *)hWnd)) {
        printf("Create run_usb_disconnect thread failure\n");
        return -1;
    }

    return tid;
}

static int shutdown_usb_disconnect(HWND hWnd)
{
    if (gshutdown == 1)
        return -1;
    gshutdown = 1;

    run_usb_disconnect(hWnd);
    return 0;
}

static int display_timelapse_record_mode_ui(HWND hWnd, int enable)
{
    InvalidateImg(hWnd, &msg_timeLapseimg, TRUE);

    return 0;
}

static int display_timelapse_on_ui(HWND hWnd, int enable)
{
    InvalidateImg(hWnd, &msg_timeLapse_on_img, TRUE);
    if (enable == 1) {
        gtimelapse_record = 1;
    } else if (enable == 0) {
        printf("display_timelapse_on_ui close\n");
        gtimelapse_record = 0;
    }

    return 0;
}

static int display_motion_detect_mode_ui(HWND hWnd, int enable)
{
    InvalidateImg(hWnd, &msg_motiondetect, TRUE);

    return 0;
}

static int display_motion_detect_on_ui(HWND hWnd, int enable)
{
    InvalidateImg(hWnd, &msg_motiondetect_on, TRUE);
    if (enable == 1) {
        gmotion_detection = 1;
    } else if (enable == 0) {
        printf("display_motion_detect_on_ui close\n");
        gmotion_detection = 0;
    }

    return 0;
}

static int parking_ui_display(HDC hdc)
{
    if (gparking == PARKINGMONITOR_MOTION_DETECT) {
        if (gmotion_detection == 1) {
            FillBoxWithBitmap(hdc,
                              MOTIONDETECT_ON_IMG_X,
                              MOTIONDETECT_ON_IMG_Y,
                              MOTIONDETECT_ON_IMG_W,
                              MOTIONDETECT_ON_IMG_H,
                              &motiondetect_on_bmap);
        } else {
            FillBoxWithBitmap(hdc,
                              MOTIONDETECT_IMG_X,
                              MOTIONDETECT_IMG_Y,
                              MOTIONDETECT_IMG_W,
                              MOTIONDETECT_IMG_H,
                              &motiondetect_bmap);
        }
    } else if (gparking == PARKINGMONITOR_TIMELAPSE) {
        if (gtimelapse_record == 1) {
            FillBoxWithBitmap(hdc,
                              MOTIONDETECT_ON_IMG_X,
                              MOTIONDETECT_ON_IMG_Y,
                              MOTIONDETECT_ON_IMG_W,
                              MOTIONDETECT_ON_IMG_H,
                              &timelapse_on_bmap);

        } else {
            FillBoxWithBitmap(hdc,
                              TIMELAPSE_IMG_X,
                              TIMELAPSE_IMG_Y,
                              TIMELAPSE_IMG_W,
                              TIMELAPSE_IMG_H,
                              &timelapse_bmap);
        }
    }

    return 0;
}

static int parking_prompt_window(HWND hWnd, char parking_mode)
{
    if (parking_mode == PARKINGMONITOR_MOTION_DETECT) {
        if (gmotion_detection == 1)
            display_motion_detect_on_ui(hWnd, 1);
        else
            display_motion_detect_mode_ui(hWnd, 1);
    } else if (parking_mode == PARKINGMONITOR_TIMELAPSE) {
        if (gtimelapse_record == 1)
            display_timelapse_on_ui(hWnd, 1);
        else
            display_timelapse_record_mode_ui(hWnd, 1);
    } else {
        display_motion_detect_mode_ui(hWnd, 0);
        display_timelapse_record_mode_ui(hWnd, 0);
    }

    return 0;
}

static int parking_monitor_mode_switch(HWND hWnd)
{
    gparking = parameter_get_parkingmonitor();
    if (gparking_mode_change == 1) {
        if (gtimelapse_record == 1) {
            display_timelapse_on_ui(hWnd, 0);
        }
        if (gmotion_detection == 1) {
            display_motion_detect_on_ui(hWnd, 0);
        }

        gparking_mode_change = 0;
    }

    parking_prompt_window(hWnd, gparking);

    if (gparking == PARKINGMONITOR_MOTION_DETECT) {
        if (gparking_gs_active == 1) {
            gparking_count = 0;
            gparking_gs_active = 0;
        } else if (SetMode == MODE_RECORDING) {
            gparking_count++;
        } else {
            gparking_count = 0;
        }

        if (gparking_count % 10 == 0)
            printf("MOTION_Detect: gparking_count = %d\n",
                   gparking_count);
        if ((gparking_count >= MOTION_DETECT_COUNT) &&
            (gmotion_detection == 0)) {
            printf("start_motion_detection\n");
            start_motion_detection();
            gmotion_detection = 1;

            /* UI: prompt start motion detection */
            display_motion_detect_on_ui(hWnd, 1);
        } else if ((gmotion_detection == 1) &&
                   (gparking_count == 0)) {
            printf("close_motion_detection\n");
            stop_motion_detection();
            gmotion_detection = 0;

            /* UI: prompt close motion detection */
            display_motion_detect_on_ui(hWnd, 0);

            printf("init rec---\n");
            if (api_get_sd_mount_state() == SD_STATE_IN)
                api_start_rec();
        }
    } else if ((gparking == PARKINGMONITOR_SHUTDOWN_MODE) ||
               (gparking == PARKINGMONITOR_SUSPEND_MODE)) {
        if ((gparking_gs_active == 0) &&
            (PARKINGMONITOR_OFF != gparking)) {
            if ((SetMode != MODE_USBDIALOG) &&
                (SetMode != MODE_USBCONNECTION))
                gparking_count++;
            else
                gparking_count = 0;
            if (gparking_count % 10 == 0)
                printf("PARKING_Monitor : gparking_count = %d\n",
                       gparking_count);
            if (gparking_count >= PARKING_RECORD_COUNT) {
                if (gparking == PARKINGMONITOR_SHUTDOWN_MODE) {
                    printf("shutdown\n");
                    parking_record_process(PARKING_SHUTDOWN);
                } else if (gparking == PARKINGMONITOR_SUSPEND_MODE) {
                    printf("suspend:\n");
                    parking_record_process(PARKING_SUSPEND);
                }
            }
        }

        if (((gparking_gs_active == 1) &&
             (PARKINGMONITOR_OFF != gparking)) ||
            (PARKINGMONITOR_OFF == gparking)) {
            gparking_count = 0;
            gparking_gs_active = 0;
        }
    } else if (gparking == PARKINGMONITOR_TIMELAPSE) {
        if (gparking_gs_active == 1) {
            gparking_count = 0;
            gparking_gs_active = 0;
        } else if (SetMode == MODE_RECORDING) {
            gparking_count++;
        } else {
            gparking_count = 0;
        }

        if (gparking_count % 10 == 0)
            printf("Timelapse_record : gparking_count = %d\n",
                   gparking_count);
        if ((gparking_count >= TIMELAPSE_RECORD_COUNT) &&
            (gtimelapse_record == 0)) {
            printf("start_timelapse_record\n");
            gtimelapse_record = 1;
            parameter_save_time_lapse_interval(1);
            video_record_set_timelapseint(1);

            /* UI: prompt start timelapse record */
            display_timelapse_on_ui(hWnd, 1);
        } else if ((gtimelapse_record == 1) &&
                   (gparking_count == 0)) {
            printf("close_timelapse_record\n");
            parameter_save_time_lapse_interval(0);
            video_record_set_timelapseint(0);
            gtimelapse_record = 0;
            /* UI: prompt close timelapse record */
            display_timelapse_on_ui(hWnd, 0);
        }
    } else {
        gparking_gs_active = 0;
        gparking_count = 0;
        gmotion_detection = 0;
    }

    return 0;
}

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

static int MyDateBoxProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
    time_t t;
    //    char bufff[512] = "";
    struct tm* tm;
    switch (message) {
    case MSG_INITDIALOG: {
        HWND hCurFocus;
        HWND hNewFocus;

        time(&t);
        tm = localtime(&t);
        hCurFocus = GetFocusChild(hDlg);
        SendDlgItemMessage(hDlg, IDL_YEAR, CB_SETSPINFORMAT, 0, (LPARAM) "%04d");
        SendDlgItemMessage(hDlg, IDL_YEAR, CB_SETSPINRANGE, 1900, 2100);
        SendDlgItemMessage(hDlg, IDL_YEAR, CB_SETSPINVALUE, tm->tm_year + 1900,
                           0);
        SendDlgItemMessage(hDlg, IDL_YEAR, CB_SETSPINPACE, 1, 1);

        SendDlgItemMessage(hDlg, IDL_MONTH, CB_SETSPINFORMAT, 0, (LPARAM) "%02d");
        SendDlgItemMessage(hDlg, IDL_MONTH, CB_SETSPINRANGE, 1, 12);
        SendDlgItemMessage(hDlg, IDL_MONTH, CB_SETSPINVALUE, tm->tm_mon + 1, 0);
        SendDlgItemMessage(hDlg, IDL_MONTH, CB_SETSPINPACE, 1, 2);

        SendDlgItemMessage(hDlg, IDL_DAY, CB_SETSPINFORMAT, 0, (LPARAM) "%02d");
        SendDlgItemMessage(hDlg, IDL_DAY, CB_SETSPINRANGE, 0, 30);
        SendDlgItemMessage(hDlg, IDL_DAY, CB_SETSPINVALUE, tm->tm_mday, 0);
        SendDlgItemMessage(hDlg, IDL_DAY, CB_SETSPINPACE, 1, 3);

        SendDlgItemMessage(hDlg, IDC_HOUR, CB_SETSPINFORMAT, 0, (LPARAM) "%02d");
        SendDlgItemMessage(hDlg, IDC_HOUR, CB_SETSPINRANGE, 0, 23);
        SendDlgItemMessage(hDlg, IDC_HOUR, CB_SETSPINVALUE, tm->tm_hour, 0);
        SendDlgItemMessage(hDlg, IDC_HOUR, CB_SETSPINPACE, 1, 4);

        SendDlgItemMessage(hDlg, IDC_MINUTE, CB_SETSPINFORMAT, 0,
                           (LPARAM) "%02d");
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
    }
    case MSG_COMMAND: {
        break;
    }
    case MSG_KEYDOWN: {
        switch (wParam) {
        case SCANCODE_ESCAPE:
            EndDialog(hDlg, wParam);
            break;
        case SCANCODE_MENU:
            EndDialog(hDlg, wParam);
            break;
        case SCANCODE_ENTER: {
            prompt(hDlg);
            EndDialog(hDlg, wParam);
            break;
        }
        }
        break;
    }
    }

    return DefaultDialogProc(hDlg, message, wParam, lParam);
}

static void proc_MSG_FS_INITFAIL(HWND hWnd, int param);
int sdcardmount_back(void* arg, int param)
{
    HWND hWnd = (HWND)arg;
    printf("sdcardmount_back param = %d\n", param);

    if (param == 0) {
        PostMessage(hWnd, MSG_REPAIR, 0, 1);
    } else if (param == 1) {
        PostMessage(hWnd, MSG_FSINITFAIL, 0, 1);
    }

    return 0;
}

//-----------------
static int UsbSDProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case MSG_INITDIALOG: {
        HWND hFocus = GetDlgDefPushButton(hDlg);
        hWndUSB = hDlg;

        if (hFocus)
            SetFocus(hFocus);
        return 0;
    }
    case MSG_COMMAND: {
        switch (wParam) {
        case IDUSB: {
            printf("  UsbSDProc :case IDUSB:IDUSB  \n");
            if (api_get_sd_mount_state() == SD_STATE_IN) {
                //  printf("UsbSDProc:");
                cvr_usb_sd_ctl(USB_CTRL, 1);
                EndDialog(hDlg, wParam);
                // hWndUSB = 0;
                changemode(hMainWnd, MODE_USBCONNECTION);
            } else if (api_get_sd_mount_state() == SD_STATE_OUT) {
                //  EndDialog(hDlg, wParam);
                if (parameter_get_video_lan() == 1)
                    MessageBox(hDlg, "SD卡不存在!!!", "警告!!!", MB_OK);
                else if (parameter_get_video_lan() == 0)
                    MessageBox(hDlg, "no SD card insertted!!!", "Warning!!!", MB_OK);
            }

            break;
        }
        case USBBAT: {
            printf("  UsbSDProc :case USBBAT  \n");
            cvr_usb_sd_ctl(USB_CTRL, 0);
            EndDialog(hDlg, wParam);
            hWndUSB = 0;
            changemode(hMainWnd, MODE_RECORDING);
            break;
        }
        }
        break;
    }
    }

    return DefaultDialogProc(hDlg, message, wParam, lParam);
}

void cmd_IDM_SETDATE(void)
{
    DLGTEMPLATE DlgMyDate = {WS_VISIBLE,
                             WS_EX_NONE,
                             (WIN_W - 350) / 2,
                             (WIN_H - 110) / 2,
                             350,
                             110,
                             SetDate,
                             0,
                             0,
                             14,
                             NULL,
                             0
                            };

    CTRLDATA CtrlMyDate[] = {
        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE, 100, 8, 150, 30, IDC_STATIC,
            "日期设置", 0
        },
        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE, 0, 45, 350, 2, IDC_STATIC,
            "-----", 0
        },
        {
            CTRL_COMBOBOX,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_AUTOSPIN | CBS_AUTOLOOP, 60, 48,
            80, 20, IDL_YEAR, "", 0
        },

        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE, 140, 50, 20, 20, IDC_STATIC,
            "年", 0
        },

        {
            CTRL_COMBOBOX,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_AUTOSPIN | CBS_AUTOLOOP, 160,
            48, 40, 20, IDL_MONTH, "", 0
        },
        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE, 200, 50, 20, 20, IDC_STATIC,
            "月", 0
        },

        {
            CTRL_COMBOBOX,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_AUTOSPIN | CBS_AUTOLOOP, 220,
            48, 40, 20, IDL_DAY, "", 0
        },
        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE, 260, 50, 20, 20, IDC_STATIC,
            "日", 0
        },

        {
            CTRL_COMBOBOX,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_AUTOSPIN | CBS_AUTOLOOP, 75, 78,
            40, 20, IDC_HOUR, "", 0
        },
        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE, 115, 80, 20, 20, IDC_STATIC,
            "时", 0
        },

        {
            CTRL_COMBOBOX,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_AUTOSPIN | CBS_AUTOLOOP, 135,
            78, 40, 20, IDC_MINUTE, "", 0
        },
        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE, 175, 80, 20, 20, IDC_STATIC,
            "分", 0
        },

        {
            CTRL_COMBOBOX,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_AUTOSPIN | CBS_AUTOLOOP, 195,
            78, 40, 20, IDL_SEC, "", 0
        },
        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE, 235, 80, 20, 20, IDC_STATIC,
            "秒", 0
        },
    };

    CTRLDATA CtrlMyDate_en[] = {
        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE, 100, 8, 150, 30, IDC_STATIC,
            "Date Set", 0
        },
        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE, 0, 45, 350, 2, IDC_STATIC,
            "-----", 0
        },
        {
            CTRL_COMBOBOX,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_AUTOSPIN | CBS_AUTOLOOP, 60, 48,
            80, 20, IDL_YEAR, "", 0
        },

        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE, 140, 50, 20, 20, IDC_STATIC,
            "Y", 0
        },

        {
            CTRL_COMBOBOX,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_AUTOSPIN | CBS_AUTOLOOP, 160,
            48, 40, 20, IDL_MONTH, "", 0
        },
        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE, 200, 50, 20, 20, IDC_STATIC,
            "M", 0
        },

        {
            CTRL_COMBOBOX,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_AUTOSPIN | CBS_AUTOLOOP, 220,
            48, 40, 20, IDL_DAY, "", 0
        },
        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE, 260, 50, 20, 20, IDC_STATIC,
            "D", 0
        },

        {
            CTRL_COMBOBOX,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_AUTOSPIN | CBS_AUTOLOOP, 75, 78,
            40, 20, IDC_HOUR, "", 0
        },
        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE, 115, 80, 20, 20, IDC_STATIC,
            "H", 0
        },

        {
            CTRL_COMBOBOX,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_AUTOSPIN | CBS_AUTOLOOP, 135,
            78, 40, 20, IDC_MINUTE, "", 0
        },
        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE, 175, 80, 20, 20, IDC_STATIC,
            "MIN", 0
        },

        {
            CTRL_COMBOBOX,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_AUTOSPIN | CBS_AUTOLOOP, 195,
            78, 40, 20, IDL_SEC, "", 0
        },
        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE, 235, 80, 20, 20, IDC_STATIC,
            "SEC", 0
        },
    };

    if (parameter_get_video_lan() == 1)
        DlgMyDate.controls = CtrlMyDate;
    else if (parameter_get_video_lan() == 0)
        DlgMyDate.controls = CtrlMyDate_en;
    DialogBoxIndirectParam(&DlgMyDate, HWND_DESKTOP, MyDateBoxProc, 0L);
}

static void cmd_IDM_RECOVERY(HWND hWnd)
{
    int ret = -1;

    if (parameter_get_video_lan() == 0)
        ret = MessageBox(hWnd, "You sure you want to restore the factory settings?", "Warning", MB_YESNO | MB_DEFBUTTON2);
    else if (parameter_get_video_lan() == 1)
        ret = MessageBox(hWnd, "确定要恢复出厂设置?", "警告", MB_YESNO | MB_DEFBUTTON2);

    if (ret == IDYES)
        api_recovery();
}

static void cmd_IDM_FWVER(HWND hWnd)
{
    if (parameter_get_video_lan() == 0)
        MessageBox(hWnd, parameter_get_firmware_version(), "FW Version", MB_OK);
    else if (parameter_get_video_lan() == 1)
        MessageBox(hWnd, parameter_get_firmware_version(), "固件版本", MB_OK);
}

static void cmd_IDM_SETTING_FWUPDATE(HWND hWnd)
{
    int ret;

    ret = api_check_firmware();

check_info:
    switch (ret) {
    case FW_NOTFOUND:
        printf("check FW_NOTFOUND: (%d)\n", ret);
        if (parameter_get_video_lan() == 0)
            MessageBox(hWnd, "Not found: /mnt/sdcard/Firmware.img", "Firmware Update", MB_OK);
        else if (parameter_get_video_lan() == 1)
            MessageBox(hWnd, "未找到: /mnt/sdcard/Firmware.img", "固件升级", MB_OK);
        break;
    case FW_INVALID:
        printf("check FW_INVALID: (%d)\n", ret);
        if (parameter_get_video_lan() == 0)
            MessageBox(hWnd, "Invalid firmware?", "Firmware Update", MB_OK);
        else if (parameter_get_video_lan() == 1)
            MessageBox(hWnd, "无效的固件？", "固件升级", MB_OK);
        break;
    case FW_OK:
    default:
        printf("check FW_OK\n");
        if (parameter_get_video_lan() == 0)
            ret = MessageBox(hWnd, "Will update firmware from /mnt/sdcard/Firmware.img?",
                             "Firmware Update", MB_YESNO | MB_DEFBUTTON2);
        else if (parameter_get_video_lan() == 1)
            ret = MessageBox(hWnd, "用新固件 /mnt/sdcard/Firmware.img 升级？",
                             "固件升级", MB_YESNO | MB_DEFBUTTON2);
        if (ret == IDYES) {
            /* check again */
            ret = api_check_firmware();
            if (ret == FW_OK) {
                /* The BOOT_FWUPDATE which is defined in init_hook.h */
                printf("will reboot and update...\n");
                parameter_flash_set_flashed(BOOT_FWUPDATE);
                api_power_reboot();
            } else {
                goto check_info;
            }
        }

        break;
    }
}

static void cmd_IDM_DEBUG_FWUPDATE_TEST(HWND hWnd)
{
    int ret = -1;

    if (parameter_get_video_lan() == 0)
        ret = MessageBox(hWnd, "Will start fw_update test?",
                         "FW_UPDATE TEST", MB_YESNO | MB_DEFBUTTON2);
    else if (parameter_get_video_lan() == 1)
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

void cmd_IDM_CAR(HWND hWnd, char i)
{
    api_set_abmode(i);
}

static void cmd_IDM_3DNR(HWND hWnd, char i)
{
    api_3dnr_onoff(i);
}

#if 0
static void cmd_IDM_IDC(HWND hWnd, char i)
{
    parameter_save_video_idc(i);
}

static void cmd_IDM_FLIP(HWND hWnd, char i)
{
    parameter_save_video_flip(i);
    rk_fb_set_flip(i);
}

static void cmd_IDM_CVBSOUT(HWND hWnd, int i)
{
    int ret;

    switch (i) {
        /* no external */
    case OUT_DEVICE_LCD:
        if (rk_fb_get_cvbsout_connect()) {
            if (!system("echo 0 > /sys/class/display/TV/enable"))
                printf("shutdown external display\n");
            else
                printf("failed to shutdown external display\n");
        }
        break;
        /* cvbsout */
    case OUT_DEVICE_CVBS_PAL:
        if (rk_fb_get_cvbsout_connect()) {
            if (!system("echo 1 > /sys/class/display/TV/enable"))
                printf("switch to cvbsout display\n");
            else
                printf("failed to switch to cvbsout display\n");

            ret = system("echo 720x576i-50 > /sys/class/display/TV/mode");
            if (!ret)
                printf("switch to cvbsout PAL mode\n");
            else
                printf("failed to switch cvbsout display mode\n");
            rk_fb_set_out_device(OUT_DEVICE_CVBS_PAL);
        }
        break;
    case OUT_DEVICE_CVBS_NTSC:
        if (rk_fb_get_cvbsout_connect()) {
            if (!system("echo 1 > /sys/class/display/TV/enable"))
                printf("switch to cvbsout display\n");
            else
                printf("failed to switch to cvbsout display\n");

            ret = system("echo 720x480i-60 > /sys/class/display/TV/mode");
            if (!ret)
                printf("switch to cvbsout NTSC mode\n");
            else
                printf("failed to switch cvbsout display mode\n");
            rk_fb_set_out_device(OUT_DEVICE_CVBS_NTSC);
        }
        break;
    }
}
#endif

static void cmd_IDM_ADAS(HWND hWnd, WPARAM wParam)
{
    switch (wParam) {
    case IDM_ADAS_OFF:
        api_adas_onoff(0);
        break;
    case IDM_ADAS_ON:
        if (api_adas_onoff(1))
            break;
        adasFlag &= ~ADASFLAG_SETTING;
        break;
    default:
        break;
    }
}

static void cmd_IDM_detect(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    //printf("cmd_IDM_detect, wParam: %ld, lParam: %ld\n", wParam, lParam);
    switch (wParam) {
    case IDM_detectOFF:
        if (!lParam && parameter_get_video_de() == 0)
            return;
        stop_motion_detection();
        if (!lParam)
            parameter_save_video_de(0);
        InvalidateImg(hWnd, &msg_rcMove, FALSE);
        break;
    case IDM_detectON:
        if (!lParam && parameter_get_video_de() == 1)
            return;
        if (!lParam)
            parameter_save_video_de(1);
        start_motion_detection(hWnd);
        InvalidateImg(hWnd, &msg_rcMove, FALSE);
        break;
    default:
        break;
    }
}

static void cmd_IDM_YES_DELETE(void)
{
    if (parameter_get_video_lan() == 0)
        MessageBox(hMainWndForPlay, "You choose YES!!", "DELETE", MB_OK);
    else if (parameter_get_video_lan() == 1)
        MessageBox(hMainWndForPlay, "你选择是!!", "删除", MB_OK);
}

static void cmd_IDM_NO_DELETE(void)
{
    if (parameter_get_video_lan() == 0)
        MessageBox(hMainWndForPlay, "You choose NO!!", "DELETE", MB_OK);
    else if (parameter_get_video_lan() == 1)
        MessageBox(hMainWndForPlay, "你选择否!!", "删除", MB_OK);
}
//if format fail, retry format.
static void cmd_IDM_RE_FORMAT(HWND hWnd)
{
    // just need judge mmcblk0
    if (fs_manage_sd_exist(SDCARD_DEVICE) != 0) {
        api_sdcard_format(1);
    } else {
        if (parameter_get_video_lan() == 1)
            MessageBox(hWnd, "SD卡不存在!!", "警告!!!", MB_OK);
        else if (parameter_get_video_lan() == 0)
            MessageBox(hWnd, "no SD card insertted!!!", "Warning!!!", MB_OK);
    }

    return;
}


static void cmd_IDM_FORMAT(HWND hWnd)
{
    int mesg = 0;
    if (api_get_sd_mount_state() != SD_STATE_OUT) {
        if (parameter_get_video_lan() == 0) {
            mesg = MessageBox(hWnd, "disk formatting", "Warning!!!", MB_YESNO | MB_DEFBUTTON2);
        } else if (parameter_get_video_lan() == 1) {
            mesg = MessageBox(hWnd, "磁盘格式化", "警告!!!", MB_YESNO | MB_DEFBUTTON2);
        }
        if (mesg == IDYES) {
            if (api_get_sd_mount_state() != SD_STATE_OUT) {
                api_sdcard_format(0);
            } else {
                if (parameter_get_video_lan() == 1)
                    MessageBox(hWnd, "SD卡不存在!!", "警告!!!", MB_OK);
                else if (parameter_get_video_lan() == 0)
                    MessageBox(hWnd, "no SD card insertted!!!", "Warning!!!", MB_OK);
            }
        }
    } else if (api_get_sd_mount_state() == SD_STATE_OUT) {
        if (parameter_get_video_lan() == 1)
            MessageBox(hWnd, "SD卡不存在!!", "警告!!!", MB_OK);
        else if (parameter_get_video_lan() == 0)
            MessageBox(hWnd, "no SD card insertted!!!", "Warning!!!", MB_OK);
    }
}

static void cmd_IDM_BACKLIGHT(WPARAM wParam)
{
    switch (wParam) {
    case IDM_BACKLIGHT_L:
        api_set_lcd_backlight(LCD_BACKLT_L);
        break;
    case IDM_BACKLIGHT_M:
        api_set_lcd_backlight(LCD_BACKLT_M);
        break;
    case IDM_BACKLIGHT_H:
        api_set_lcd_backlight(LCD_BACKLT_H);
        break;
    default:
        break;
    }
}

/* Chinese index */
uint32_t lic_chinese_idx = 0;
/* Charactor or number index */
uint32_t lic_num_idx = 0;

/* Save the car license plate */
char licenseplate[MAX_LICN_NUM][ONE_CHN_SIZE] = {"-", "-", "-", "-", "-", "-", "-", "-",};
uint32_t licplate_pos[MAX_LICN_NUM] = {0};
extern char licenseplate_str[20];
extern const char g_license_chinese[PROVINCE_ABBR_MAX][ONE_CHN_SIZE];
extern const char g_license_char[LICENSE_CHAR_MAX][ONE_CHAR_SIZE];

static int MyLicensePlateProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
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

        /* Set licence plate at init. */
        for (k = 0; k < MAX_LICN_NUM; k++) {
            if (dlgID == ((k + 1) + IDC_BUTTON_OK))
                SetWindowText(hFocus, (char*)licenseplate[k]);

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
                /* Selected */
                SetPenColor(hdc, COLOR_red);
                SetPenWidth(hdc, 4);
            } else {
                /* Unselected */
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
        if (key_lock)
            break;

        int index = 0;

        hdc = GetClientDC(hDlg);

        switch (wParam) {
        case SCANCODE_ESCAPE:
            EndDialog(hDlg, wParam);
            break;
        case SCANCODE_MENU:
            save_licenseplate((char *)licenseplate);
            EndDialog(hDlg, wParam);
            break;
        case SCANCODE_ENTER: {
            save_licenseplate((char *)licenseplate);
            EndDialog(hDlg, wParam);
            break;
        }
        case SCANCODE_CURSORBLOCKDOWN: {
            hFocus = GetFocus(hDlg);
            dlgID = GetDlgCtrlID(hFocus);
            if (dlgID == IDC_BUTTON_OK)
                break;

            /* Select Chinese Character */
            if (dlgID == IDC_STATIC1) {
                if (lic_chinese_idx >= PROVINCE_ABBR_MAX - 1)
                    lic_chinese_idx = 0;
                /* Ignore space char */
                SetWindowText(hFocus, g_license_chinese[++lic_chinese_idx]);
                licplate_pos[0] = lic_chinese_idx;
            } else {
                index = dlgID - IDC_STATIC1;
                lic_num_idx = licplate_pos[index];

                if (lic_num_idx >= LICENSE_CHAR_MAX - 1)
                    lic_num_idx = 0;

                SetWindowText(hFocus, (char*)g_license_char[++lic_num_idx]);
                licplate_pos[index] = lic_num_idx;
            }
            break;
        }
        case SCANCODE_CURSORBLOCKUP: {
            hFocus = GetFocus(hDlg);
            dlgID = GetDlgCtrlID(hFocus);
            if (dlgID == IDC_BUTTON_OK)
                break;

            /* Select Chinese Character */
            if (dlgID == IDC_STATIC1) {
                if (lic_chinese_idx == 0)
                    lic_chinese_idx = PROVINCE_ABBR_MAX - 1;

                if (lic_chinese_idx == 1)
                    lic_chinese_idx = PROVINCE_ABBR_MAX;
                SetWindowText(hFocus, (char*)g_license_chinese[--lic_chinese_idx]);
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
            InvalidateImg(hDlg, &rt, FALSE);
            break;
        case SCANCODE_CURSORBLOCKRIGHT:
            hFocus = GetNextDlgTabItem(hDlg, hFocus, FALSE);
            SetFocus(hFocus);
            GetClientRect(hDlg, &rt);
            InvalidateImg(hDlg, &rt, FALSE);
            break;
        case SCANCODE_TAB: {
            RECT rt;
            hFocus = GetFocus(hDlg);
            dlgID = GetDlgCtrlID(hFocus);

            lic_chinese_idx = licplate_pos[0];
            if (dlgID != IDC_BUTTON_OK)
                lic_num_idx = licplate_pos[dlgID + 1];  /* Get the next ctrlID's char index */

            GetClientRect(hDlg, &rt);
            InvalidateImg(hDlg, &rt, FALSE);
        }
        break;
        }
        ReleaseDC(hdc);
        break;
    }
    }

    return DefaultDialogProc(hDlg, message, wParam, lParam);
}

void cmd_IDM_SetLicensePlate(HWND hWnd)
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

    const char *info_text[LANGUAGE_NUM][1] = {
        {
            "License Plate Setting",
        },
        {
            "车牌设置",
        },
    };

    CTRLDATA CtrlMyLicense[] = {
        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE,
            LICN_STR_X, LICN_STR_Y, LICN_STR_W, LICN_STR_H, IDC_STATIC,
            info_text[lang][0], 0
        },

        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE | WS_TABSTOP,
            LICN_RECT_X, LICN_RECT_Y, 20, 20, IDC_STATIC1,
            "*", 0
        },

        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE | WS_TABSTOP,
            LICN_RECT_X + LICN_RECT_GAP, LICN_RECT_Y, LICN_RECT_W, LICN_RECT_H, IDC_STATIC2,
            "*", 0
        },

        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE | WS_TABSTOP,
            LICN_RECT_X + LICN_RECT_GAP * 2, LICN_RECT_Y, LICN_RECT_W, LICN_RECT_H, IDC_STATIC3,
            "*", 0
        },

        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE | WS_TABSTOP,
            LICN_RECT_X + LICN_RECT_GAP * 3, LICN_RECT_Y, LICN_RECT_W, LICN_RECT_H, IDC_STATIC4,
            "*", 0
        },

        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE | WS_TABSTOP,
            LICN_RECT_X + LICN_RECT_GAP * 4, LICN_RECT_Y, LICN_RECT_W, LICN_RECT_H, IDC_STATIC5,
            "*", 0
        },

        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE | WS_TABSTOP,
            LICN_RECT_X + LICN_RECT_GAP * 5, LICN_RECT_Y, LICN_RECT_W, LICN_RECT_H, IDC_STATIC6,
            "*", 0
        },

        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE | WS_TABSTOP,
            LICN_RECT_X + LICN_RECT_GAP * 6, LICN_RECT_Y, LICN_RECT_W, LICN_RECT_H, IDC_STATIC7,
            "*", 0
        },

        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE | WS_TABSTOP,
            LICN_RECT_X + LICN_RECT_GAP * 7, LICN_RECT_Y, LICN_RECT_W, LICN_RECT_H, IDC_STATIC8,
            "*", 0
        },

        {
            CTRL_BUTTON,
            WS_VISIBLE | BS_DEFPUSHBUTTON | BS_PUSHBUTTON | WS_TABSTOP,
            LICN_BUTTON_OK_X, LICN_BUTTON_OK_Y, LICN_BUTTON_OK_W, LICN_BUTTON_OK_H, IDC_BUTTON_OK,
            "OK", 0
        },
    };

    DlgMyLiense.controls = CtrlMyLicense;
    DialogBoxIndirectParam(&DlgMyLiense, HWND_DESKTOP, MyLicensePlateProc, 0L);
}

static char phone_num[MAX_PHONE_NUM+1] = {0};

static int PhonecallProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    int dlgID;
    int i, k;
    RECT rt;
    static HWND hFocus;
    char win_caption[2];

    switch (message) {
    case MSG_INITDIALOG:
        break;

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

        for (i = IDC_STATIC1; i <= IDC_STATIC12; i++) {
            if (i == dlgID) {
                /* Selected */
                SetPenColor(hdc, COLOR_red);
                SetPenWidth(hdc, 4);
                Rectangle(hdc, 96 + ((i - IDC_STATIC1)%4)*50, 76 + ((i - IDC_STATIC1)/4)*50,
                    144 + ((i - IDC_STATIC1)%4)*50, 124 + ((i - IDC_STATIC1)/4)*50);
            } else {
                /* Unselected */
                SetPenColor(hdc, old_bk_color);
                Rectangle(hdc, 96 + ((i - IDC_STATIC1)%4)*50, 76 + ((i - IDC_STATIC1)/4)*50,
                    144 + ((i - IDC_STATIC1)%4)*50, 124 + ((i - IDC_STATIC1)/4)*50);
            }
        }

        SetPenColor(hdc, old_pen_color);
        Rectangle(hdc, 10, 10, rt.right - 10, rt.bottom - 10);
        EndPaint(hDlg, hdc);
    }
    break;
    case MSG_KEYDOWN: {
        if (key_lock)
            break;

        int index = 0;
        hdc = GetClientDC(hDlg);

        switch (wParam) {
        case SCANCODE_ESCAPE:
            break;
        case SCANCODE_MENU:
#ifdef USE_RIL_MOUDLE
            handup_ril_voip();
#endif
            EndDialog(hDlg, wParam);
            break;
        case SCANCODE_ENTER: {
            hFocus = GetFocus(hDlg);
            dlgID = GetDlgCtrlID(hFocus);
            if (dlgID == IDC_STATIC4){
                index = strlen(phone_num);
                if(index <= 0)
                    break;
                phone_num[index-1] = '\0';
            }else if (dlgID == IDC_STATIC12){
#ifdef USE_RIL_MOUDLE
                printf("send_call_num_to_ril phone_num %s\n",phone_num);
                send_mess_to_ril(RIL_CALL_TEL, phone_num);
#endif
            }else{
                index = strlen(phone_num);
                if(index >= MAX_PHONE_NUM)
                    break;
                GetWindowText(hFocus, win_caption, 2);
                strcat(phone_num, win_caption);
            }
            SetWindowText(GetDlgItem (hDlg, IDC_STATIC), phone_num);
            break;
        }
        case SCANCODE_CURSORBLOCKDOWN: {
            for (k = 0; k < 4; k++){
                hFocus = GetNextDlgTabItem(hDlg, hFocus, FALSE);
            }
            SetFocus(hFocus);
            dlgID = GetDlgCtrlID(hFocus);
            index = dlgID - IDC_STATIC1;
            GetClientRect(hDlg, &rt);
            InvalidateRect(hDlg, &rt, FALSE);
            break;
        }
        case SCANCODE_CURSORBLOCKUP: {
            for (k = 0; k < 4; k++){
                hFocus = GetNextDlgTabItem(hDlg, hFocus, TRUE);
            }
            SetFocus(hFocus);
            dlgID = GetDlgCtrlID(hFocus);
            index = dlgID - IDC_STATIC1;
            GetClientRect(hDlg, &rt);
            InvalidateRect(hDlg, &rt, FALSE);
            break;
        }
        case SCANCODE_CURSORBLOCKLEFT:
            hFocus = GetNextDlgTabItem(hDlg, hFocus, TRUE);
            SetFocus(hFocus);
            dlgID = GetDlgCtrlID(hFocus);
            index = dlgID - IDC_STATIC1;
            GetClientRect(hDlg, &rt);
            InvalidateRect(hDlg, &rt, FALSE);
            break;
        case SCANCODE_CURSORBLOCKRIGHT:
            hFocus = GetNextDlgTabItem(hDlg, hFocus, FALSE);
            SetFocus(hFocus);
            dlgID = GetDlgCtrlID(hFocus);
            if (dlgID == IDC_BUTTON_OK)
                break;

            index = dlgID - IDC_STATIC1;
            GetClientRect(hDlg, &rt);
            InvalidateRect(hDlg, &rt, FALSE);
            break;
        case SCANCODE_TAB: {
            break;
        }
        break;
        }
        ReleaseDC(hdc);
        break;
    }
    }

    return DefaultDialogProc(hDlg, message, wParam, lParam);
}

void cmd_IDM_SET_PHONE_CALL(HWND hWnd)
{
    DLGTEMPLATE DlgPhonecall = {WS_VISIBLE,
                               WS_EX_NONE,
                               (WIN_W - 400) / 2,
                               (WIN_H - 320) / 2,
                               400,
                               320,
                               SetCallNum,
                               0,
                               0,
                               13,
                               NULL,
                               0
                              };

    CTRLDATA CtrlPhonecall[] = {
        {
            CTRL_STATIC, WS_CHILD | SS_CENTER | WS_VISIBLE, 20, 20, 370, 30, IDC_STATIC,
            " Please enter the number dialed ", 0
        },

        {
            CTRL_STATIC, WS_CHILD | SS_CENTER | WS_VISIBLE | WS_TABSTOP, 100, 80, 40, 40, IDC_STATIC1,
            "1", 0
        },

        {
            CTRL_STATIC, WS_CHILD | SS_CENTER | WS_VISIBLE | WS_TABSTOP, 150, 80, 40, 40, IDC_STATIC2,
            "2", 0
        },

        {
            CTRL_STATIC, WS_CHILD | SS_CENTER | WS_VISIBLE | WS_TABSTOP, 200, 80, 40, 40, IDC_STATIC3,
            "3", 0
        },

        {
            CTRL_STATIC, WS_CHILD | SS_CENTER | WS_VISIBLE | WS_TABSTOP, 250, 80, 40, 40, IDC_STATIC4,
            "del", 0
        },

        {
            CTRL_STATIC, WS_CHILD | SS_CENTER | WS_VISIBLE | WS_TABSTOP, 100, 130, 40, 40, IDC_STATIC5,
            "4", 0
        },

        {
            CTRL_STATIC, WS_CHILD | SS_CENTER | WS_VISIBLE | WS_TABSTOP, 150, 130, 40, 40, IDC_STATIC6,
            "5", 0
        },

        {
            CTRL_STATIC, WS_CHILD | SS_CENTER | WS_VISIBLE | WS_TABSTOP, 200, 130, 40, 40, IDC_STATIC7,
            "6", 0
        },

        {
            CTRL_STATIC, WS_CHILD | SS_CENTER | WS_VISIBLE | WS_TABSTOP, 250, 130, 40, 40, IDC_STATIC8,
            "0", 0
        },

        {
            CTRL_STATIC, WS_CHILD | SS_CENTER | WS_VISIBLE | WS_TABSTOP, 100, 180, 40, 40, IDC_STATIC9,
            "7", 0
        },

        {
            CTRL_STATIC, WS_CHILD | SS_CENTER | WS_VISIBLE | WS_TABSTOP, 150, 180, 40, 40, IDC_STATIC10,
            "8", 0
        },

        {
            CTRL_STATIC, WS_CHILD | SS_CENTER | WS_VISIBLE | WS_TABSTOP, 200, 180, 40, 40, IDC_STATIC11,
            "9", 0
        },

        {
            CTRL_STATIC, WS_CHILD | SS_CENTER | WS_VISIBLE | WS_TABSTOP, 250, 180, 40, 40, IDC_STATIC12,
            "OK", 0
        },

    };

    CTRLDATA CtrlPhonecall_en[] = {
        {
            "static", WS_CHILD | SS_CENTER | WS_VISIBLE, 20, 20, 370, 30, IDC_STATIC,
            " 请输入拨打的号码 ", 0
        },

        {
            CTRL_STATIC, WS_CHILD | SS_CENTER | WS_VISIBLE | WS_TABSTOP, 100, 80, 40, 40, IDC_STATIC1,
            "1", 0
        },

        {
            CTRL_STATIC, WS_CHILD | SS_CENTER | WS_VISIBLE | WS_TABSTOP, 150, 80, 40, 40, IDC_STATIC2,
            "2", 0
        },

        {
            CTRL_STATIC, WS_CHILD | SS_CENTER | WS_VISIBLE | WS_TABSTOP, 200, 80, 40, 40, IDC_STATIC3,
            "3", 0
        },

        {
            CTRL_STATIC, WS_CHILD | SS_CENTER | WS_VISIBLE | WS_TABSTOP, 250, 80, 40, 40, IDC_STATIC4,
            "回退", 0
        },

        {
            CTRL_STATIC, WS_CHILD | SS_CENTER | WS_VISIBLE | WS_TABSTOP, 100, 130, 40, 40, IDC_STATIC5,
            "4", 0
        },

        {
            CTRL_STATIC, WS_CHILD | SS_CENTER | WS_VISIBLE | WS_TABSTOP, 150, 130, 40, 40, IDC_STATIC6,
            "5", 0
        },

        {
            CTRL_STATIC, WS_CHILD | SS_CENTER | WS_VISIBLE | WS_TABSTOP, 200, 130, 40, 40, IDC_STATIC7,
            "6", 0
        },

        {
            CTRL_STATIC, WS_CHILD | SS_CENTER | WS_VISIBLE | WS_TABSTOP, 250, 130, 40, 40, IDC_STATIC8,
            "0", 0
        },

        {
            CTRL_STATIC, WS_CHILD | SS_CENTER | WS_VISIBLE | WS_TABSTOP, 100, 180, 40, 40, IDC_STATIC9,
            "7", 0
        },

        {
            CTRL_STATIC, WS_CHILD | SS_CENTER | WS_VISIBLE | WS_TABSTOP, 150, 180, 40, 40, IDC_STATIC10,
            "8", 0
        },

        {
            CTRL_STATIC, WS_CHILD | SS_CENTER | WS_VISIBLE | WS_TABSTOP, 200, 180, 40, 40, IDC_STATIC11,
            "9", 0
        },

        {
            CTRL_STATIC, WS_CHILD | SS_CENTER | WS_VISIBLE | WS_TABSTOP, 250, 180, 40, 40, IDC_STATIC12,
            " 确定 ", 0
        },
    };

    if (parameter_get_video_lan() == 1)
        DlgPhonecall.controls = CtrlPhonecall;
    else if (parameter_get_video_lan() == 0)
        DlgPhonecall.controls = CtrlPhonecall_en;
    DialogBoxIndirectParam(&DlgPhonecall, HWND_DESKTOP, PhonecallProc, 0L);
}

static void cmd_IDM_DEBUG_PHOTO_ON(HWND hWnd)
{
    if ((api_video_get_record_state() == VIDEO_STATE_PREVIEW)
        && api_get_sd_mount_state() == SD_STATE_IN)
        api_start_rec();
    parameter_save_debug_photo(1);
}

static void cmd_IDM_DEBUG_PHOTO_OFF(HWND hWnd)
{
    if (api_video_get_record_state() == VIDEO_STATE_RECORD)
        api_stop_rec();
    parameter_save_debug_photo(0);
}

static int WifiInfoProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case MSG_KEYDOWN: {
        if (key_lock)
            break;
        switch (wParam) {
        case SCANCODE_ESCAPE:
        case SCANCODE_MENU:
        case SCANCODE_ENTER:
        case SCANCODE_CURSORBLOCKDOWN:
        case SCANCODE_CURSORBLOCKUP:
        case SCANCODE_CURSORBLOCKLEFT:
        case SCANCODE_CURSORBLOCKRIGHT:
        case SCANCODE_TAB:
            EndDialog(hDlg, wParam);
            break;
        break;
        }
        break;
    }
    }

    return DefaultDialogProc(hDlg, message, wParam, lParam);
}

void cmd_IDM_SHOW_WIFI_INFO(HWND hWnd)
{
    char ssid[48];
    char password[48];
    char sta_ssid[48];
    char sta_password[48];

    char ap_ssid_caption_en[48];
    char ap_pass_caption_en[48];
    char sta_ssid_caption_en[48];
    char sta_pass_caption_en[48];

    char ap_ssid_caption[48];
    char ap_pass_caption[48];
    char sta_ssid_caption[48];
    char sta_pass_caption[48];

    parameter_getwifiinfo(ssid, password);
    parameter_getwifistainfo(sta_ssid, sta_password);

    snprintf(ap_ssid_caption_en, sizeof(ap_ssid_caption_en), "ap ssid:%s", ssid);
    snprintf(ap_pass_caption_en, sizeof(ap_pass_caption_en), "ap pass:%s", password);
    snprintf(sta_ssid_caption_en, sizeof(sta_ssid_caption_en), "sta ssid:%s", sta_ssid);
    snprintf(sta_pass_caption_en, sizeof(sta_pass_caption_en), "sta pass:%s", sta_password);

    snprintf(ap_ssid_caption, sizeof(ap_ssid_caption), "ap 路由器:%s", ssid);
    snprintf(ap_pass_caption, sizeof(ap_pass_caption), "ap 密  码:%s", password);
    snprintf(sta_ssid_caption, sizeof(sta_ssid_caption), "sta 路由器:%s", sta_ssid);
    snprintf(sta_pass_caption, sizeof(sta_pass_caption), "sta 密  码:%s", sta_password);

    DLGTEMPLATE DlgWifiInfo = {WS_VISIBLE,
                               WS_EX_NONE,
                               (WIN_W - 400) / 2,
                               (WIN_H - 320) / 2,
                               400,
                               320,
                               ShowWifiInfo,
                               0,
                               0,
                               5,
                               NULL,
                               0
                              };

    CTRLDATA CtrlWifiInfo_en[] = {
        {
            CTRL_STATIC, WS_CHILD | SS_CENTER | WS_VISIBLE, 20, 20, 370, 30, IDC_STATIC,
            " wifi info: ", 0
        },

        {
            CTRL_STATIC, WS_CHILD | SS_CENTER | WS_VISIBLE, 20, 100, 370, 30, IDC_STATIC1,
            ap_ssid_caption_en, 0
        },

        {
            CTRL_STATIC, WS_CHILD | SS_CENTER | WS_VISIBLE, 20, 140, 370, 30, IDC_STATIC2,
            ap_pass_caption_en, 0
        },

        {
            CTRL_STATIC, WS_CHILD | SS_CENTER | WS_VISIBLE, 20, 180, 370, 30, IDC_STATIC3,
            sta_ssid_caption_en, 0
        },

        {
            CTRL_STATIC, WS_CHILD | SS_CENTER | WS_VISIBLE, 20, 220, 370, 30, IDC_STATIC4,
            sta_pass_caption_en, 0
        }
    };

    CTRLDATA CtrlWifiInfo[] = {
        {
            CTRL_STATIC, WS_CHILD | SS_CENTER | WS_VISIBLE, 20,  20, 370, 30, IDC_STATIC,
            " wifi 信息 ", 0
        },

        {
            CTRL_STATIC, WS_CHILD | SS_CENTER | WS_VISIBLE,  20, 100, 370, 30, IDC_STATIC1,
            ap_ssid_caption, 0
        },

        {
            CTRL_STATIC, WS_CHILD | SS_CENTER | WS_VISIBLE,  20, 140, 370, 30, IDC_STATIC2,
            ap_pass_caption, 0
        },

        {
            CTRL_STATIC, WS_CHILD | SS_CENTER | WS_VISIBLE,  20, 180, 370, 30, IDC_STATIC3,
            sta_ssid_caption, 0
        },

        {
            CTRL_STATIC, WS_CHILD | SS_CENTER | WS_VISIBLE,  20, 220, 370, 30, IDC_STATIC4,
            sta_pass_caption, 0
        },
    };

    if (parameter_get_video_lan() == 1)
        DlgWifiInfo.controls = CtrlWifiInfo;
    else if (parameter_get_video_lan() == 0)
        DlgWifiInfo.controls = CtrlWifiInfo_en;
    DialogBoxIndirectParam(&DlgWifiInfo, HWND_DESKTOP, WifiInfoProc, 0L);
}

static void cmd_IDM_DEBUG_TEMP_ON(HWND hWnd)
{
    parameter_save_debug_temp(1);
}

static void cmd_IDM_DEBUG_TEMP_OFF(HWND hWnd)
{
    parameter_save_debug_temp(0);
}

static void cmd_IDM_EFFECT_WATERMARK_ON(HWND hWnd)
{
    parameter_save_debug_effect_watermark(1);
}

static void cmd_IDM_EFFECT_WATERMARK_OFF(HWND hWnd)
{
    parameter_save_debug_effect_watermark(0);
}

extern void GUIAPI HideMenuBar (HWND hwnd);
void MainMenuClean(HWND hWnd)
{
    SendMessage (HWND_DESKTOP, MSG_CLOSEMENU, 0, 0);
    HideMenuBar(hWnd);
}

//---------------MSG_COMMAND消息处理函数---------------------
static int commandEvent(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    struct photo_param photo_param1 = {1280, 720, 7};
    struct photo_param photo_param2 = {1920, 1080, 5};
    struct photo_param photo_param3 = {2560, 1440, 3};

    switch (wParam) {
    case IDM_FONT_1:
        api_set_front_rec_resolution(0);
        break;
    case IDM_FONT_2:
        api_set_front_rec_resolution(1);
        break;
    case IDM_FONT_3:
        api_set_front_rec_resolution(2);
        break;
    case IDM_FONT_4:
        api_set_front_rec_resolution(3);
        break;
    case IDM_BACK_1:
        api_set_back_rec_resolution(0);
        break;
    case IDM_BACK_2:
        api_set_back_rec_resolution(1);
        break;
    case IDM_BACK_3:
        api_set_back_rec_resolution(2);
        break;
    case IDM_BACK_4:
        api_set_back_rec_resolution(3);
        break;
    case IDM_CIF_1:
        api_set_cif_rec_resolution(0);
        break;
    case IDM_CIF_2:
        api_set_cif_rec_resolution(1);
        break;
    case IDM_CIF_3:
        api_set_cif_rec_resolution(2);
        break;
    case IDM_CIF_4:
        api_set_cif_rec_resolution(3);
        break;
    case IDM_BOOT_OFF:
        parameter_save_debug_reboot(0);
        break;
    case IDM_RECOVERY_OFF:
        parameter_save_debug_recovery(0);
        break;
    case IDM_STANDBY_2_OFF:
        parameter_save_debug_standby(0);
        break;
    case IDM_MODE_CHANGE_OFF:
        parameter_save_debug_modechange(0);
        break;
    case IDM_DEBUG_VIDEO_OFF:
        parameter_save_debug_video(0);
        break;
    case IDM_BEG_END_VIDEO_OFF:
        parameter_save_debug_beg_end_video(0);
        break;
    case IDM_DEBUG_PHOTO_OFF:
        cmd_IDM_DEBUG_PHOTO_OFF(hWnd);
        break;
    case IDM_DEBUG_TEMP_ON:
        cmd_IDM_DEBUG_TEMP_ON(hWnd);
        break;
    case IDM_DEBUG_TEMP_OFF:
        cmd_IDM_DEBUG_TEMP_OFF(hWnd);
        break;
    case IDM_EFFECT_WATERMARK_ON:
        cmd_IDM_EFFECT_WATERMARK_ON(hWnd);
        break;
    case IDM_EFFECT_WATERMARK_OFF:
        cmd_IDM_EFFECT_WATERMARK_OFF(hWnd);
        break;
    case IDM_BOOT_ON:
        parameter_save_debug_reboot(1);
        break;
    case IDM_RECOVERY_ON:
        parameter_save_debug_recovery(1);
        break;
    case IDM_STANDBY_2_ON:
        parameter_save_debug_standby(1);
        break;
    case IDM_MODE_CHANGE_ON:
        parameter_save_debug_modechange(1);
        break;
    case IDM_DEBUG_VIDEO_ON:
        parameter_save_debug_video(1);
        break;
    case IDM_BEG_END_VIDEO_ON:
        parameter_save_debug_beg_end_video(1);
        break;
    case IDM_DEBUG_PHOTO_ON:
        cmd_IDM_DEBUG_PHOTO_ON(hWnd);
        break;
    case IDM_VIDEO_QUALITY_H:
        api_set_video_quaity(VIDEO_QUALITY_HIGH);
        break;
    case IDM_VIDEO_QUALITY_M:
        api_set_video_quaity(VIDEO_QUALITY_MID);
        break;
    case IDM_VIDEO_QUALITY_L:
        api_set_video_quaity(VIDEO_QUALITY_LOW);
        break;
    case IDM_LICENSEPLATE_WATERMARK:
        cmd_IDM_SetLicensePlate(hWnd);
        break;
    case IDM_PHONE_CALL:
        cmd_IDM_SET_PHONE_CALL(hWnd);
        break;
    case IDM_1M_ph:
        api_set_photo_resolution(&photo_param1);
        break;
    case IDM_2M_ph:
        api_set_photo_resolution(&photo_param2);
        break;
    case IDM_3M_ph:
        api_set_photo_resolution(&photo_param3);
        break;
    case IDM_RECOVERY:
        cmd_IDM_RECOVERY(hWnd);
        break;
    case IDM_FWVER:
        cmd_IDM_FWVER(hWnd);
        break;
    case IDM_SETTING_FWUPDATE:
        cmd_IDM_SETTING_FWUPDATE(hWnd);
        break;
    case IDM_DEBUG_FWUPDATE_TEST:
        cmd_IDM_DEBUG_FWUPDATE_TEST(hWnd);
        break;
    case IDM_USB:
        cmd_IDM_USB();
        break;
    case IDM_ADB:
        cmd_IDM_ADB();
        break;
    case IDM_UVC:
        cmd_IDM_UVC();
        break;
    case IDM_RNDIS:
        cmd_IDM_RNDIS();
        break;
    case IDM_ABOUT_CAR:
        break;
    case IDM_ABOUT_TIME:
        break;
    case IDM_1MIN:
        api_video_set_time_lenght(60);
        break;
    case IDM_3MIN:
        api_video_set_time_lenght(180);
        break;
    case IDM_5MIN:
        api_video_set_time_lenght(300);
        break;
    case IDM_CAR1:
        cmd_IDM_CAR(hWnd, 0);
        break;
    case IDM_CAR2:
        cmd_IDM_CAR(hWnd, 1);
        break;
    case IDM_CAR3:
        cmd_IDM_CAR(hWnd, 2);
        break;
    case IDM_IDCOFF:
        api_set_video_idc(0);
        break;
    case IDM_IDCON:
        api_set_video_idc(1);
        break;
    case IDM_FLIP_OFF:
        api_video_reinit_flip(0);
        video_record_watermark_refresh();
        break;
    case IDM_FLIP_ON:
        api_video_reinit_flip(1);
        video_record_watermark_refresh();
        break;
    case IDM_CIF_MIRROR_OFF:
        video_record_set_cif_mirror(false);
        api_video_reinit();
        break;
    case IDM_CIF_MIRROR_ON:
        video_record_set_cif_mirror(true);
        api_video_reinit();
        break;
    case IDM_3DNROFF:
        cmd_IDM_3DNR(hWnd, 0);
        break;
    case IDM_3DNRON:
        cmd_IDM_3DNR(hWnd, 1);
        break;
    case IDM_CVBSOUT_ON_PAL:
        api_set_video_cvbsout(OUT_DEVICE_CVBS_PAL);
        break;
    case IDM_CVBSOUT_ON_NTSC:
        api_set_video_cvbsout(OUT_DEVICE_CVBS_NTSC);
        break;
    case IDM_CVBSOUT_OFF:
        api_set_video_cvbsout(OUT_DEVICE_LCD);
        break;
    case IDM_ADAS_OFF:
        cmd_IDM_ADAS(hWnd, wParam);
        break;
    case IDM_ADAS_ON:
        cmd_IDM_ADAS(hWnd, wParam);
        break;
    case IDM_ADAS_SETTING:
        adasFlag |= ADASFLAG_SETTING;
        MainMenuClean(hWnd);
        if (SetMode == MODE_RECORDING) {
            DestroyMainWindowMenu(hWnd);
            menucreated = 0;
        }
        break;
    case IDM_bright1:
        api_set_white_balance(0);
        break;
    case IDM_bright2:
        api_set_white_balance(1);
        break;
    case IDM_bright3:
        api_set_white_balance(2);
        break;
    case IDM_bright4:
        api_set_white_balance(3);
        break;
    case IDM_bright5:
        api_set_white_balance(4);
        break;
    case IDM_exposal1:
        api_set_exposure_compensation(0);
        break;
    case IDM_exposal2:
        api_set_exposure_compensation(1);
        break;
    case IDM_exposal3:
        api_set_exposure_compensation(2);
        break;
    case IDM_exposal4:
        api_set_exposure_compensation(3);
        break;
    case IDM_exposal5:
        api_set_exposure_compensation(4);
        break;
    case IDM_detectON:
    case IDM_detectOFF:
        cmd_IDM_detect(hWnd, wParam, lParam);
        break;
    case IDM_markOFF:
        api_video_mark_onoff(0);
        break;
    case IDM_markON:
        api_video_mark_onoff(1);
        break;
    case IDM_GPSOFF:
        if (parameter_get_gps_mark()) {
            gps_ui_control(hWnd, TRUE);
            api_gps_deinit();
            parameter_save_gps_mark(0);
        }
        break;
    case IDM_GPSON:
        if (!parameter_get_gps_mark()) {
            if (api_gps_init() >= 0) {
                parameter_save_gps_mark(1);
                if(parameter_get_gps_watermark())
                    gps_ui_control(hWnd, FALSE);
            }
        }
        break;
    case IDM_GPS_WATERMARK_OFF:
        parameter_save_gps_watermark(0);
        break;
    case IDM_GPS_WATERMARK_ON:
        parameter_save_gps_watermark(1);
        break;
    case IDM_recordOFF:
        api_mic_onoff(0);
        break;
    case IDM_recordON:
        api_mic_onoff(1);
        break;
    case IDM_AUTOOFFSCREENON:
        screenoff_time = VIDEO_AUTO_OFF_SCREEN_ON;
        api_set_video_autooff_screen(VIDEO_AUTO_OFF_SCREEN_ON);
        break;
    case IDM_AUTOOFFSCREENOFF:
        screenoff_time = VIDEO_AUTO_OFF_SCREEN_OFF;
        api_set_video_autooff_screen(VIDEO_AUTO_OFF_SCREEN_OFF);
        break;
    case IDM_KEY_VOLUME_LEVEL0:
        if (api_get_key_level() != KEY_VOLUME_LEVEL_0)
            api_set_key_volume(KEY_VOLUME_LEVEL_0);
        break;
    case IDM_KEY_VOLUME_LEVEL1:
        if (api_get_key_level() != KEY_VOLUME_LEVEL_1)
            api_set_key_volume(KEY_VOLUME_LEVEL_1);
        break;
    case IDM_KEY_VOLUME_LEVEL2:
        if (api_get_key_level() != KEY_VOLUME_LEVEL_2)
            api_set_key_volume(KEY_VOLUME_LEVEL_2);
        break;
    case IDM_KEY_VOLUME_LEVEL3:
        if (api_get_key_level() != KEY_VOLUME_LEVEL_3)
            api_set_key_volume(KEY_VOLUME_LEVEL_3);
        break;
    case IDM_autorecordOFF:
        api_video_autorecode_onoff(0);
        break;
    case IDM_autorecordON:
        api_video_autorecode_onoff(1);
        break;
    case IDM_WIFIOFF:
        parameter_save_wifi_en(0);
        wifi_management_stop();
        break;
    case IDM_WIFION:
        parameter_save_wifi_en(1);
        wifi_management_start();
        cmd_IDM_SHOW_WIFI_INFO(hWnd);
        break;
    case IDM_langEN:
        api_set_language(0);
        break;
    case IDM_langCN:
        api_set_language(1);
        break;
    case IDM_50HZ:
        api_set_video_freq(CAMARE_FREQ_50HZ);
        break;
    case IDM_60HZ:
        api_set_video_freq(CAMARE_FREQ_60HZ);
        break;
    case IDM_YES_DELETE:
        cmd_IDM_YES_DELETE();
        break;
    case IDM_NO_DELETE:
        cmd_IDM_NO_DELETE();
        break;
    case IDM_1M:
        SetCameraMP = 0;
        break;
    case IDM_2M:
        SetCameraMP = 1;
        break;
    case IDM_3M:
        SetCameraMP = 2;
        break;
    case IDM_FORMAT:
        cmd_IDM_FORMAT(hWnd);
        break;
    case IDM_SETDATE:
        cmd_IDM_SETDATE();
        break;
    case IDM_BACKLIGHT_L:
        cmd_IDM_BACKLIGHT(wParam);
        break;
    case IDM_BACKLIGHT_M:
        cmd_IDM_BACKLIGHT(wParam);
        break;
    case IDM_BACKLIGHT_H:
        cmd_IDM_BACKLIGHT(wParam);
        break;
    case IDM_ABOUT_SETTING:
        break;
    case IDM_COLLISION_NO:
        api_set_collision_level(COLLI_CLOSE);
        break;
    case IDM_COLLISION_L:
        api_set_collision_level(COLLI_LEVEL_L);
        break;
    case IDM_COLLISION_M:
        api_set_collision_level(COLLI_LEVEL_M);
        break;
    case IDM_COLLISION_H:
        api_set_collision_level(COLLI_LEVEL_H);
        break;
    case IDM_PARKINGMONITOR_OFF:
        parameter_save_parkingmonitor(PARKINGMONITOR_OFF);
        parameter_save_video_de(0);
        parking_unregister();
        if (gmotion_detection == 1) {
            stop_motion_detection();
            gmotion_detection = 0;
        }
        if (gtimelapse_record == 1) {
            parameter_save_time_lapse_interval(0);
            video_record_set_timelapseint(0);
        }
        if (gparking != IDM_PARKINGMONITOR_OFF) {
            gparking_count = 0;
        }

        break;
    case IDM_PARKINGMONITOR_SHUTDOWN_MODE:
        if (gmotion_detection == 1) {
            stop_motion_detection();
        }
        if (gtimelapse_record == 1) {
            parameter_save_time_lapse_interval(0);
            video_record_set_timelapseint(0);
        }
        if (gparking != PARKINGMONITOR_SHUTDOWN_MODE) {
            gparking_count = 0;
            gparking_mode_change = 1;
        }
        parameter_save_parkingmonitor(PARKINGMONITOR_SHUTDOWN_MODE);
        gparking = PARKINGMONITOR_SHUTDOWN_MODE;
        parking_register();

        break;
    case IDM_MOTION_DETECT:
        if (gtimelapse_record == 1) {
            parameter_save_time_lapse_interval(0);
            video_record_set_timelapseint(0);
        }
        if (gparking != PARKINGMONITOR_MOTION_DETECT) {
            gparking_count = 0;
            gparking_mode_change = 1;
        }
        parking_register();
        parameter_save_parkingmonitor(PARKINGMONITOR_MOTION_DETECT);
        gparking = PARKINGMONITOR_MOTION_DETECT;

        break;
    case IDM_TIME_LAPSE_RECORD:
        if (gmotion_detection == 1) {
            stop_motion_detection();
        }
        if (gparking != PARKINGMONITOR_TIMELAPSE) {
            gparking_count = 0;
            gparking_mode_change = 1;
        }
        parking_register();
        parameter_save_parkingmonitor(PARKINGMONITOR_TIMELAPSE);
        gparking = PARKINGMONITOR_TIMELAPSE;

        break;
        /* TODO: show status of time-lapse by icon in screen */
    case IDM_TIME_LAPSE_OFF:
        api_set_video_time_lapse(0);
        break;
    case IDM_TIME_LAPSE_INTERNAL_1s:
        api_set_video_time_lapse(1);
        break;
    case IDM_TIME_LAPSE_INTERNAL_5s:
        api_set_video_time_lapse(5);
        break;
    case IDM_TIME_LAPSE_INTERNAL_10s:
        api_set_video_time_lapse(10);
        break;
    case IDM_TIME_LAPSE_INTERNAL_30s:
        api_set_video_time_lapse(30);
        break;
    case IDM_TIME_LAPSE_INTERNAL_60s:
        api_set_video_time_lapse(60);
        break;
    case IDM_HOT_SHAREON:
#ifdef USE_RIL_MOUDLE
        ril_open_hot_share();
#endif
        break;
    case IDM_HOT_SHAREOFF:
#ifdef USE_RIL_MOUDLE
        ril_close_hot_share();
#endif
        break;
    case IDM_QUIT:
        DestroyMainWindow(hWnd);
        PostQuitMessage(hWnd);
        return 0;
    }

    return 1;
}

//----碰撞检测菜单实现-----------
static HMENU createpmenufileCOLLISION()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;
    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)collision[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)collision[i];

    hmnu = CreatePopupMenu(&mii);
    hmnuForCOLLI = hmnu;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_collision_level() == COLLI_CLOSE) ? MFS_CHECKED
                : MFS_UNCHECKED;
    mii.id = IDM_COLLISION_NO;
    mii.typedata = (DWORD)Collision_0[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)Collision_0[i];

    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 1, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_collision_level() == COLLI_LEVEL_L)
                ? MFS_CHECKED
                : MFS_UNCHECKED;
    mii.id = IDM_COLLISION_L;
    mii.typedata = (DWORD)Collision_1[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)Collision_1[i];

    InsertMenuItem(hmnu, 2, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 3, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_collision_level() == COLLI_LEVEL_M)
                ? MFS_CHECKED
                : MFS_UNCHECKED;
    mii.id = IDM_COLLISION_M;
    mii.typedata = (DWORD)Collision_2[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)Collision_2[i];

    InsertMenuItem(hmnu, 4, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 5, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_collision_level() == COLLI_LEVEL_H)
                ? MFS_CHECKED
                : MFS_UNCHECKED;
    mii.id = IDM_COLLISION_H;
    mii.typedata = (DWORD)Collision_3[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)Collision_3[i];

    InsertMenuItem(hmnu, 6, TRUE, &mii);

    return hmnu;
}

//----停车录像菜单实现-----------
static HMENU createpmenufileLEAVECARREC()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)parking_monitor[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)parking_monitor[i];

    hmnu = CreatePopupMenu(&mii);
    hmnuForLEAVEREC = hmnu;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_parkingmonitor() == PARKINGMONITOR_OFF) ?
                MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_PARKINGMONITOR_OFF;
    mii.typedata = (DWORD)parkingmonitor_Off[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)parkingmonitor_Off[i];
    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type = MFT_SEPARATOR;
    mii.state = 0;
    mii.id = 0;
    mii.typedata = 0;
    InsertMenuItem(hmnu, 1, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_parkingmonitor() == PARKINGMONITOR_SHUTDOWN_MODE) ?
                MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_PARKINGMONITOR_SHUTDOWN_MODE;
    mii.typedata = (DWORD)Shutdown_On[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)Shutdown_On[i];
    InsertMenuItem(hmnu, 3, TRUE, &mii);

    mii.type = MFT_SEPARATOR;
    mii.state = 0;
    mii.id = 0;
    mii.typedata = 0;
    InsertMenuItem(hmnu, 4, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_parkingmonitor() == PARKINGMONITOR_MOTION_DETECT) ?
                MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_MOTION_DETECT;
    mii.typedata = (DWORD)Motion_Detect[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)Motion_Detect[i];
    InsertMenuItem(hmnu, 5, TRUE, &mii);

    mii.type = MFT_SEPARATOR;
    mii.state = 0;
    mii.id = 0;
    mii.typedata = 0;
    InsertMenuItem(hmnu, 6, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_parkingmonitor() == PARKINGMONITOR_TIMELAPSE) ?
                MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_TIME_LAPSE_RECORD;
    mii.typedata = (DWORD)Time_Lapse_Record[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)Time_Lapse_Record[i];
    InsertMenuItem(hmnu, 7, TRUE, &mii);

    return hmnu;
}
//----频率菜单实现-----------
static HMENU createpmenufileFREQUENCY()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)frequency[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)frequency[i];

    hmnu = CreatePopupMenu(&mii);
    hmnuForFRE = hmnu;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_video_fre() == CAMARE_FREQ_50HZ) ? MFS_CHECKED
                : MFS_UNCHECKED;
    mii.id = IDM_50HZ;
    mii.typedata = (DWORD)HzNum_50[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)HzNum_50[i];

    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 1, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_video_fre() == CAMARE_FREQ_60HZ) ? MFS_CHECKED
                : MFS_UNCHECKED;
    mii.id = IDM_60HZ;
    mii.typedata = (DWORD)HzNum_60[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)HzNum_60[i];

    InsertMenuItem(hmnu, 2, TRUE, &mii);

    return hmnu;
}
//----背光亮度菜单实现-----------
static HMENU createpmenufileBACKLIGHT()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;

    char BackLight_1[2][20] = {" low ", " 低 "};
    char BackLight_2[2][20] = {" middle ", " 中 "};
    char BackLight_3[2][20] = {" hight ", " 高 "};

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)backlight[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)backlight[i];

    hmnu = CreatePopupMenu(&mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_video_backlt() == LCD_BACKLT_L) ? MFS_CHECKED
                : MFS_UNCHECKED;
    mii.id = IDM_BACKLIGHT_L;
    mii.typedata = (DWORD)BackLight_1[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)BackLight_1[i];

    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 1, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_video_backlt() == LCD_BACKLT_M) ? MFS_CHECKED
                : MFS_UNCHECKED;
    mii.id = IDM_BACKLIGHT_M;
    mii.typedata = (DWORD)BackLight_2[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)BackLight_2[i];

    InsertMenuItem(hmnu, 2, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 3, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_video_backlt() == LCD_BACKLT_H) ? MFS_CHECKED
                : MFS_UNCHECKED;
    mii.id = IDM_BACKLIGHT_H;
    mii.typedata = (DWORD)BackLight_3[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)BackLight_3[i];

    InsertMenuItem(hmnu, 4, TRUE, &mii);

    return hmnu;
}
static HMENU createpmenufileUSB()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;
    char* UsbMsc[2] = {" msc ", " msc "};
    char* UsbAdb[2] = {" adb ", " adb "};
    char* UsbRndis[2] = {" rndis ", " rndis "};
#if USE_USB_WEBCAM
    char* UsbUvc[2] = {" uvc ", " uvc "};
#endif

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)usbmode[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)usbmode[i];

    hmnu = CreatePopupMenu(&mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_video_usb() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_USB;
    mii.typedata = (DWORD)UsbMsc[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)UsbMsc[i];

    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 1, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_video_usb() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_ADB;
    mii.typedata = (DWORD)UsbAdb[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)UsbAdb[i];

    InsertMenuItem(hmnu, 2, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_SEPARATOR;
    mii.state = 0;
    mii.id = 0;
    mii.typedata = 0;
    InsertMenuItem(hmnu, 3, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_video_usb() == 3) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_RNDIS;
    mii.typedata = (DWORD)UsbRndis[0];
    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)UsbRndis[i];
    InsertMenuItem(hmnu, 4, TRUE, &mii);

#if USE_USB_WEBCAM
    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_SEPARATOR;
    mii.state = 0;
    mii.id = 0;
    mii.typedata = 0;
    InsertMenuItem(hmnu, 5, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_video_usb() == 2) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_UVC;
    mii.typedata = (DWORD)UsbUvc[0];
    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)UsbUvc[i];
    InsertMenuItem(hmnu, 6, TRUE, &mii);
#endif




    return hmnu;
}

//------------语言选择菜单实现-----------------
static HMENU createpmenufileLANGUAGE()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)language[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)language[i];

    hmnu = CreatePopupMenu(&mii);
    hmnuForLAN = hmnu;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_video_lan() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_langEN;
    mii.typedata = (DWORD)LAN_ENG[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)LAN_ENG[i];

    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 1, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_video_lan() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_langCN;
    mii.typedata = (DWORD)LAN_CHN[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)LAN_CHN[i];

    InsertMenuItem(hmnu, 2, TRUE, &mii);

    return hmnu;
}

//---------------屏幕自动关闭菜单实现-------------------
static HMENU createpmenufileWIFIONOFF()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)wifi[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)wifi[i];

    hmnu = CreatePopupMenu(&mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state =
        (parameter_get_wifi_en() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_WIFIOFF;
    mii.typedata = (DWORD)OFF[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)OFF[i];

    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 1, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state =
        (parameter_get_wifi_en() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_WIFION;
    mii.typedata = (DWORD)ON[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)ON[i];

    InsertMenuItem(hmnu, 2, TRUE, &mii);

    return hmnu;
}

//---------------屏幕自动关闭菜单实现-------------------
static HMENU createpmenufileAUTOOFFSCREEN()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)screenoffset[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)screenoffset[i];

    hmnu = CreatePopupMenu(&mii);
    hmnuForAutoOffScreen = hmnu;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state =
        (parameter_get_screenoff_time() == -1) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_AUTOOFFSCREENOFF;
    mii.typedata = (DWORD)OFF[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)OFF[i];

    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 1, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state =
        (parameter_get_screenoff_time() > 0) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_AUTOOFFSCREENON;
    mii.typedata = (DWORD)ON[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)ON[i];

    InsertMenuItem(hmnu, 2, TRUE, &mii);

    return hmnu;
}

//---------------按键音音量控制实现-------------------
static HMENU createpmenufileKEYTONE()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)key_volume[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)key_volume[i];

    hmnu = CreatePopupMenu(&mii);
    hmnuForKeyVol = hmnu;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state =
        (api_get_key_level() == KEY_VOLUME_LEVEL_0) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_KEY_VOLUME_LEVEL0;
    mii.typedata = (DWORD)volume_0[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)volume_0[i];

    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 1, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state =
        (api_get_key_level() == KEY_VOLUME_LEVEL_1) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_KEY_VOLUME_LEVEL1;
    mii.typedata = (DWORD)volume_1[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)volume_1[i];

    InsertMenuItem(hmnu, 2, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 3, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state =
        (api_get_key_level() == KEY_VOLUME_LEVEL_2) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_KEY_VOLUME_LEVEL2;
    mii.typedata = (DWORD)volume_2[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)volume_2[i];

    InsertMenuItem(hmnu, 4, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 5, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state =
        (api_get_key_level() == KEY_VOLUME_LEVEL_3) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_KEY_VOLUME_LEVEL3;
    mii.typedata = (DWORD)volume_3[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)volume_3[i];

    InsertMenuItem(hmnu, 6, TRUE, &mii);

    return hmnu;
}


//---------------开机录像菜单实现-------------------
static HMENU createpmenufileAUTORECORD()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)auto_record[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)auto_record[i];

    hmnu = CreatePopupMenu(&mii);
    hmnuForAutoRE = hmnu;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state =
        (parameter_get_video_autorec() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_autorecordOFF;
    mii.typedata = (DWORD)OFF[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)OFF[i];

    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 1, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state =
        (parameter_get_video_autorec() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_autorecordON;
    mii.typedata = (DWORD)ON[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)ON[i];

    InsertMenuItem(hmnu, 2, TRUE, &mii);

    return hmnu;
}

static HMENU createpmenufileIDC()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)DSP_IDC[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)DSP_IDC[i];

    hmnu = CreatePopupMenu(&mii);
    hmnuFor3DNR = hmnu;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_video_idc() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_IDCOFF;
    mii.typedata = (DWORD)OFF[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)OFF[i];

    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type = MFT_SEPARATOR;
    mii.state = 0;
    mii.id = 0;
    mii.typedata = 0;
    InsertMenuItem(hmnu, 1, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_video_idc() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_IDCON;
    mii.typedata = (DWORD)ON[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)ON[i];

    InsertMenuItem(hmnu, 2, TRUE, &mii);

    return hmnu;
}

static HMENU createpmenufileVIDEOFLIP()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)video_flip[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)video_flip[i];

    hmnu = CreatePopupMenu(&mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_video_flip() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_FLIP_OFF;
    mii.typedata = (DWORD)OFF[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)OFF[i];

    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type = MFT_SEPARATOR;
    mii.state = 0;
    mii.id = 0;
    mii.typedata = 0;
    InsertMenuItem(hmnu, 1, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_video_flip() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_FLIP_ON;
    mii.typedata = (DWORD)ON[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)ON[i];

    InsertMenuItem(hmnu, 2, TRUE, &mii);

    return hmnu;
}

static HMENU createpmenufileCIFMIRROR()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)ui_cif_mirror[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)ui_cif_mirror[i];

    hmnu = CreatePopupMenu(&mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (!video_record_get_cif_mirror()) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_CIF_MIRROR_OFF;
    mii.typedata = (DWORD)OFF[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)OFF[i];

    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type = MFT_SEPARATOR;
    mii.state = 0;
    mii.id = 0;
    mii.typedata = 0;
    InsertMenuItem(hmnu, 1, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (video_record_get_cif_mirror()) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_CIF_MIRROR_ON;
    mii.typedata = (DWORD)ON[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)ON[i];

    InsertMenuItem(hmnu, 2, TRUE, &mii);

    return hmnu;
}


//---------3DNR-------------------------
static HMENU createpmenufile3DNR()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)DSP_3DNR[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)DSP_3DNR[i];

    hmnu = CreatePopupMenu(&mii);
    hmnuFor3DNR = hmnu;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_video_3dnr() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_3DNROFF;
    mii.typedata = (DWORD)OFF[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)OFF[i];

    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 1, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_video_3dnr() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_3DNRON;
    mii.typedata = (DWORD)ON[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)ON[i];

    InsertMenuItem(hmnu, 2, TRUE, &mii);

    return hmnu;
}

/*----------------External Display---------------------*/
static HMENU createpmenufileCVBSOUT()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;
    int tmp;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)DISP_CVBSOUT[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)DISP_CVBSOUT[i];

    hmnu = CreatePopupMenu(&mii);
    hmnuForCVBSOUT = hmnu;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (rk_fb_get_out_device(&tmp, &tmp) == OUT_DEVICE_LCD) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_CVBSOUT_OFF;
    mii.typedata = (DWORD)OFF[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)OFF[i];

    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type = MFT_SEPARATOR;
    mii.state = 0;
    mii.id = 0;
    mii.typedata = 0;
    InsertMenuItem(hmnu, 1, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (rk_fb_get_out_device(&tmp, &tmp) == OUT_DEVICE_CVBS_PAL) ?
                MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_CVBSOUT_ON_PAL;
    mii.typedata = (DWORD)DISP_CVBSOUT_PAL[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)DISP_CVBSOUT_PAL[i];

    InsertMenuItem(hmnu, 2, TRUE, &mii);

    mii.type = MFT_SEPARATOR;
    mii.state = 0;
    mii.id = 0;
    mii.typedata = 0;
    InsertMenuItem(hmnu, 3, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (rk_fb_get_out_device(&tmp, &tmp) == OUT_DEVICE_CVBS_NTSC) ?
                MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_CVBSOUT_ON_NTSC;
    mii.typedata = (DWORD)DISP_CVBSOUT_NTSC[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)DISP_CVBSOUT_NTSC[i];

    InsertMenuItem(hmnu, 4, TRUE, &mii);

    return hmnu;
}

// font camera
static HMENU createpmenufileFONTCAMERA()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;
    int cnt = 0;
    char mpstr[128] = {0};
    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)fontcamera_ui[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)fontcamera_ui[i];

    hmnu = CreatePopupMenu(&mii);

    if (!video_record_get_front_resolution(frontcamera, 4)) {
        if ((frontcamera[0].width > 0) && (frontcamera[0].height > 0)) {
            memset(&mpstr, 0, 10);
            snprintf(mpstr, sizeof(mpstr), "%d * %d", frontcamera[0].width, frontcamera[0].height);
            memset(&mii, 0, sizeof(MENUITEMINFO));
            mii.type = MFT_RADIOCHECK;
            mii.state =
                (parameter_get_video_fontcamera() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
            mii.id = IDM_FONT_1;
            mii.typedata = (DWORD)mpstr;

            for (i = 0; i < LANGUAGE_NUM; i++)
                mii.str[i] = mpstr;

            InsertMenuItem(hmnu, cnt++, TRUE, &mii);
        }
        if ((frontcamera[1].width > 0) && (frontcamera[1].height > 0)) {
            if (cnt == 1) {
                mii.type = MFT_SEPARATOR;  //类型
                mii.state = 0;
                mii.id = 0;  // ID
                mii.typedata = 0;
                InsertMenuItem(hmnu, cnt++, TRUE, &mii);
            }

            memset(&mpstr, 0, 10);
            snprintf(mpstr, sizeof(mpstr), "%d * %d", frontcamera[1].width, frontcamera[1].height);
            memset(&mii, 0, sizeof(MENUITEMINFO));
            mii.type = MFT_RADIOCHECK;
            mii.state =
                (parameter_get_video_fontcamera() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
            mii.id = IDM_FONT_2;
            mii.typedata = (DWORD)mpstr;

            for (i = 0; i < LANGUAGE_NUM; i++)
                mii.str[i] = mpstr;

            InsertMenuItem(hmnu, cnt++, TRUE, &mii);
        }
        if ((frontcamera[2].width > 0) && (frontcamera[2].height > 0)) {
            if (cnt >= 1) {
                mii.type = MFT_SEPARATOR;  //类型
                mii.state = 0;
                mii.id = 0;  // ID
                mii.typedata = 0;
                InsertMenuItem(hmnu, cnt++, TRUE, &mii);
            }

            memset(&mpstr, 0, 10);
            snprintf(mpstr, sizeof(mpstr), "%d * %d", frontcamera[2].width, frontcamera[2].height);
            memset(&mii, 0, sizeof(MENUITEMINFO));
            mii.type = MFT_RADIOCHECK;
            mii.state =
                (parameter_get_video_fontcamera() == 2) ? MFS_CHECKED : MFS_UNCHECKED;
            mii.id = IDM_FONT_3;
            mii.typedata = (DWORD)mpstr;

            for (i = 0; i < LANGUAGE_NUM; i++)
                mii.str[i] = mpstr;

            InsertMenuItem(hmnu, cnt++, TRUE, &mii);
        }
        if ((frontcamera[3].width > 0) && (frontcamera[3].height > 0)) {
            if (cnt >= 1) {
                mii.type = MFT_SEPARATOR;  //类型
                mii.state = 0;
                mii.id = 0;  // ID
                mii.typedata = 0;
                InsertMenuItem(hmnu, cnt++, TRUE, &mii);
            }

            memset(&mpstr, 0, 10);
            snprintf(mpstr, sizeof(mpstr), "%d * %d", frontcamera[3].width, frontcamera[3].height);

            memset(&mii, 0, sizeof(MENUITEMINFO));
            mii.type = MFT_RADIOCHECK;
            mii.state =
                (parameter_get_video_fontcamera() == 3) ? MFS_CHECKED : MFS_UNCHECKED;
            mii.id = IDM_FONT_4;
            mii.typedata = (DWORD)mpstr;

            for (i = 0; i < LANGUAGE_NUM; i++)
                mii.str[i] = mpstr;

            InsertMenuItem(hmnu, cnt++, TRUE, &mii);
        }
    }

    return hmnu;
}

static HMENU createpmenufileCIFCamera()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;
    int cnt = 0;
    char mpstr[32] = {0};

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)cifcamera_ui[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)cifcamera_ui[i];

    hmnu = CreatePopupMenu(&mii);

    if (!video_record_get_cif_resolution(cifcamera, 4)) {
        if ((cifcamera[0].width > 0) && (cifcamera[0].height > 0)) {
            memset(&mpstr, 0, 10);
            snprintf(mpstr, sizeof(mpstr), "%d * %d", cifcamera[0].width, cifcamera[0].height);
            memset(&mii, 0, sizeof(MENUITEMINFO));
            mii.type = MFT_RADIOCHECK;
            mii.state =
                (parameter_get_video_cifcamera() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
            mii.id = IDM_CIF_1;
            mii.typedata = (DWORD)mpstr;

            for (i = 0; i < LANGUAGE_NUM; i++)
                mii.str[i] = mpstr;

            InsertMenuItem(hmnu, cnt++, TRUE, &mii);
        }

        if ((cifcamera[1].width > 0) && (cifcamera[1].height > 0)) {
            if (cnt == 1) {
                mii.type = MFT_SEPARATOR;
                mii.state = 0;
                mii.id = 0;
                mii.typedata = 0;
                InsertMenuItem(hmnu, cnt++, TRUE, &mii);
            }

            memset(&mpstr, 0, 10);
            snprintf(mpstr, sizeof(mpstr), "%d * %d", cifcamera[1].width, cifcamera[1].height);
            memset(&mii, 0, sizeof(MENUITEMINFO));
            mii.type = MFT_RADIOCHECK;
            mii.state =
                (parameter_get_video_cifcamera() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
            mii.id = IDM_CIF_2;
            mii.typedata = (DWORD)mpstr;

            for (i = 0; i < LANGUAGE_NUM; i++)
                mii.str[i] = mpstr;

            InsertMenuItem(hmnu, cnt++, TRUE, &mii);
        }

        if ((cifcamera[2].width > 0) && (cifcamera[2].height > 0)) {
            if (cnt >= 1) {
                mii.type = MFT_SEPARATOR;
                mii.state = 0;
                mii.id = 0;
                mii.typedata = 0;
                InsertMenuItem(hmnu, cnt++, TRUE, &mii);
            }

            memset(&mpstr, 0, 10);
            snprintf(mpstr, sizeof(mpstr), "%d * %d", cifcamera[2].width, cifcamera[2].height);
            memset(&mii, 0, sizeof(MENUITEMINFO));
            mii.type = MFT_RADIOCHECK;
            mii.state =
                (parameter_get_video_cifcamera() == 2) ? MFS_CHECKED : MFS_UNCHECKED;
            mii.id = IDM_CIF_3;
            mii.typedata = (DWORD)mpstr;

            for (i = 0; i < LANGUAGE_NUM; i++)
                mii.str[i] = mpstr;

            InsertMenuItem(hmnu, cnt++, TRUE, &mii);
        }

        if ((cifcamera[3].width > 0) && (cifcamera[3].height > 0)) {
            if (cnt >= 1) {
                mii.type = MFT_SEPARATOR;
                mii.state = 0;
                mii.id = 0;
                mii.typedata = 0;
                InsertMenuItem(hmnu, cnt++, TRUE, &mii);
            }

            memset(&mpstr, 0, 10);
            snprintf(mpstr, sizeof(mpstr), "%d * %d", cifcamera[3].width, cifcamera[3].height);

            memset(&mii, 0, sizeof(MENUITEMINFO));
            mii.type = MFT_RADIOCHECK;
            mii.state =
                (parameter_get_video_cifcamera() == 3) ? MFS_CHECKED : MFS_UNCHECKED;
            mii.id = IDM_CIF_4;
            mii.typedata = (DWORD)mpstr;

            for (i = 0; i < LANGUAGE_NUM; i++)
                mii.str[i] = mpstr;

            InsertMenuItem(hmnu, cnt++, TRUE, &mii);
        }
    }
    return hmnu;
}

static HMENU createpmenufileBACKCAMERA()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;
    int cnt = 0;
    char mpstr[32] = {0};
    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)backcamera_ui[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)backcamera_ui[i];

    hmnu = CreatePopupMenu(&mii);

    if (!video_record_get_back_resolution(backcamera, 4)) {
        if ((backcamera[0].width > 0) && (backcamera[0].height > 0)) {
            memset(&mpstr, 0, 10);
            snprintf(mpstr, sizeof(mpstr), "%d * %d", backcamera[0].width, backcamera[0].height);
            memset(&mii, 0, sizeof(MENUITEMINFO));
            mii.type = MFT_RADIOCHECK;
            mii.state =
                (parameter_get_video_backcamera() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
            mii.id = IDM_BACK_1;
            mii.typedata = (DWORD)mpstr;

            for (i = 0; i < LANGUAGE_NUM; i++)
                mii.str[i] = mpstr;

            InsertMenuItem(hmnu, cnt++, TRUE, &mii);
        }
        if ((backcamera[1].width > 0) && (backcamera[1].height > 0)) {
            if (cnt == 1) {
                mii.type = MFT_SEPARATOR;  //类型
                mii.state = 0;
                mii.id = 0;  // ID
                mii.typedata = 0;
                InsertMenuItem(hmnu, cnt++, TRUE, &mii);
            }

            memset(&mpstr, 0, 10);
            snprintf(mpstr, sizeof(mpstr), "%d * %d", backcamera[1].width, backcamera[1].height);
            memset(&mii, 0, sizeof(MENUITEMINFO));
            mii.type = MFT_RADIOCHECK;
            mii.state =
                (parameter_get_video_backcamera() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
            mii.id = IDM_BACK_2;
            mii.typedata = (DWORD)mpstr;

            for (i = 0; i < LANGUAGE_NUM; i++)
                mii.str[i] = mpstr;

            InsertMenuItem(hmnu, cnt++, TRUE, &mii);
        }
        if ((backcamera[2].width > 0) && (backcamera[2].height > 0)) {
            if (cnt >= 1) {
                mii.type = MFT_SEPARATOR;  //类型
                mii.state = 0;
                mii.id = 0;  // ID
                mii.typedata = 0;
                InsertMenuItem(hmnu, cnt++, TRUE, &mii);
            }

            memset(&mpstr, 0, 10);
            snprintf(mpstr, sizeof(mpstr), "%d * %d", backcamera[2].width, backcamera[2].height);
            memset(&mii, 0, sizeof(MENUITEMINFO));
            mii.type = MFT_RADIOCHECK;
            mii.state =
                (parameter_get_video_backcamera() == 2) ? MFS_CHECKED : MFS_UNCHECKED;
            mii.id = IDM_BACK_3;
            mii.typedata = (DWORD)mpstr;

            for (i = 0; i < LANGUAGE_NUM; i++)
                mii.str[i] = mpstr;

            InsertMenuItem(hmnu, cnt++, TRUE, &mii);
        }
        if ((backcamera[3].width > 0) && (backcamera[3].height > 0)) {
            if (cnt >= 1) {
                mii.type = MFT_SEPARATOR;  //类型
                mii.state = 0;
                mii.id = 0;  // ID
                mii.typedata = 0;
                InsertMenuItem(hmnu, cnt++, TRUE, &mii);
            }

            memset(&mpstr, 0, 10);
            snprintf(mpstr, sizeof(mpstr), "%d * %d", backcamera[3].width, backcamera[3].height);

            memset(&mii, 0, sizeof(MENUITEMINFO));
            mii.type = MFT_RADIOCHECK;
            mii.state =
                (parameter_get_video_backcamera() == 3) ? MFS_CHECKED : MFS_UNCHECKED;
            mii.id = IDM_BACK_4;
            mii.typedata = (DWORD)mpstr;

            for (i = 0; i < LANGUAGE_NUM; i++)
                mii.str[i] = mpstr;

            InsertMenuItem(hmnu, cnt++, TRUE, &mii);
        }
    } else {
        //          hmnu = NULL;
    }

    return hmnu;
}

// REBOOT
static HMENU createpmenufileREBOOT()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)reboot[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)reboot[i];

    hmnu = CreatePopupMenu(&mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_debug_reboot() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_BOOT_OFF;
    mii.typedata = (DWORD)OFF[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)OFF[i];

    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 1, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_debug_reboot() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_BOOT_ON;
    mii.typedata = (DWORD)ON[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)ON[i];

    InsertMenuItem(hmnu, 2, TRUE, &mii);

    return hmnu;
}
static HMENU createpmenufileRECOVERY()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)recovery_debug[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)recovery_debug[i];

    hmnu = CreatePopupMenu(&mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state =
        (parameter_get_debug_recovery() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_RECOVERY_OFF;
    mii.typedata = (DWORD)OFF[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)OFF[i];

    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 1, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state =
        (parameter_get_debug_recovery() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_RECOVERY_ON;
    mii.typedata = (DWORD)ON[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)ON[i];

    InsertMenuItem(hmnu, 2, TRUE, &mii);

    return hmnu;
}

static HMENU createpmenufileSTANDBY()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)standby[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)standby[i];

    hmnu = CreatePopupMenu(&mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state =
        (parameter_get_debug_standby() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_STANDBY_2_OFF;
    mii.typedata = (DWORD)OFF[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)OFF[i];

    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 1, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state =
        (parameter_get_debug_standby() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_STANDBY_2_ON;
    mii.typedata = (DWORD)ON[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)ON[i];

    InsertMenuItem(hmnu, 2, TRUE, &mii);

    return hmnu;
}

static HMENU createpmenufileMODECHANGE()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)mode_change[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)mode_change[i];

    hmnu = CreatePopupMenu(&mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state =
        (parameter_get_debug_modechange() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_MODE_CHANGE_OFF;
    mii.typedata = (DWORD)OFF[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)OFF[i];

    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 1, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state =
        (parameter_get_debug_modechange() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_MODE_CHANGE_ON;
    mii.typedata = (DWORD)ON[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)ON[i];

    InsertMenuItem(hmnu, 2, TRUE, &mii);

    return hmnu;
}
static HMENU createpmenufileDEBUGVIDEO()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)video_debug[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)video_debug[i];

    hmnu = CreatePopupMenu(&mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_debug_video() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_DEBUG_VIDEO_OFF;
    mii.typedata = (DWORD)OFF[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)OFF[i];

    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 1, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_debug_video() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_DEBUG_VIDEO_ON;
    mii.typedata = (DWORD)ON[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)ON[i];

    InsertMenuItem(hmnu, 2, TRUE, &mii);

    return hmnu;
}
static HMENU createpmenufileBEGENDVIDEO()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)beg_end_video[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)beg_end_video[i];

    hmnu = CreatePopupMenu(&mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state =
        (parameter_get_debug_beg_end_video() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_BEG_END_VIDEO_OFF;
    mii.typedata = (DWORD)OFF[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)OFF[i];

    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 1, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state =
        (parameter_get_debug_beg_end_video() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_BEG_END_VIDEO_ON;
    mii.typedata = (DWORD)ON[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)ON[i];

    InsertMenuItem(hmnu, 2, TRUE, &mii);

    return hmnu;
}
static HMENU createpmenufileDEBUGPHOTO()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)photo_debug[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)photo_debug[i];

    hmnu = CreatePopupMenu(&mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_debug_photo() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_DEBUG_PHOTO_OFF;
    mii.typedata = (DWORD)OFF[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)OFF[i];

    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 1, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_debug_photo() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_DEBUG_PHOTO_ON;
    mii.typedata = (DWORD)ON[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)ON[i];

    InsertMenuItem(hmnu, 2, TRUE, &mii);

    return hmnu;
}

static HMENU createpmenufileDEBUGTEMP()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)temp_debug[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)temp_debug[i];

    hmnu = CreatePopupMenu(&mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_debug_temp() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_DEBUG_TEMP_OFF;
    mii.typedata = (DWORD)OFF[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)OFF[i];

    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 1, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_debug_temp() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_DEBUG_TEMP_ON;
    mii.typedata = (DWORD)ON[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)ON[i];

    InsertMenuItem(hmnu, 2, TRUE, &mii);

    return hmnu;
}

static HMENU createpmenufileEFFECTWATERMARK()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)effect_watermark[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)effect_watermark[i];

    hmnu = CreatePopupMenu(&mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_debug_effect_watermark() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_EFFECT_WATERMARK_OFF;
    mii.typedata = (DWORD)OFF[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)OFF[i];

    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type = MFT_SEPARATOR;
    mii.state = 0;
    mii.id = 0;
    mii.typedata = 0;
    InsertMenuItem(hmnu, 1, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_debug_effect_watermark() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_EFFECT_WATERMARK_ON;
    mii.typedata = (DWORD)ON[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)ON[i];

    InsertMenuItem(hmnu, 2, TRUE, &mii);

    return hmnu;
}

inline static void add_seperator_line(HMENU hmnu, MENUITEMINFO* mii, int* cnt)
{
    mii->type = MFT_SEPARATOR;  //类型
    mii->state = 0;
    mii->id = 0;  // ID
    mii->typedata = 0;
    InsertMenuItem(hmnu, *cnt, TRUE, mii);
    *cnt += 1;
}

#define ADD_SUB_VBR_MENU(INDEX, VAL)                                       \
  memset(&mii, 0, sizeof(MENUITEMINFO));                                   \
  mii.type = MFT_RADIOCHECK;                                               \
  mii.state = (parameter_get_bit_rate_per_pixel() == VAL) ? MFS_CHECKED    \
                                                          : MFS_UNCHECKED; \
  mii.id = IDM_DEBUG_VIDEO_BIT_RATE + VAL;                           \
  mii.typedata = (DWORD)vbit_rate_vals[INDEX];                             \
  mii.str[0] = (char*)vbit_rate_vals[INDEX];                               \
  InsertMenuItem(hmnu, cnt++, TRUE, &mii)

static HMENU createpmenufileDEBUGVideoBitRate()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;
    int cnt = 0;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)vbit_rate_debug[0];

    for (i = 0; i < 2; i++)
        mii.str[i] = (char*)vbit_rate_debug[i];

    hmnu = CreatePopupMenu(&mii);

    for (i = 0; i < sizeof(vbit_rate_vals) / sizeof(vbit_rate_vals[0]); i++) {
        int val = atoi(vbit_rate_vals[i]);
        add_seperator_line(hmnu, &mii, &cnt);
        ADD_SUB_VBR_MENU(i, val);
    }

    return hmnu;
}

static HMENU createpmenufileADAS()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)DSP_ADAS[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)DSP_ADAS[i];

    hmnu = CreatePopupMenu(&mii);
    hmnuForADAS = hmnu;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_video_adas() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_ADAS_OFF;
    mii.typedata = (DWORD)OFF[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)OFF[i];

    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type = MFT_SEPARATOR;
    mii.state = 0;
    mii.id = 0;
    mii.typedata = 0;
    InsertMenuItem(hmnu, 1, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_video_adas() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_ADAS_ON;
    mii.typedata = (DWORD)ON[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)ON[i];

    InsertMenuItem(hmnu, 2, TRUE, &mii);

    mii.type = MFT_SEPARATOR;
    mii.state = 0;
    mii.id = 0;
    mii.typedata = 0;
    InsertMenuItem(hmnu, 3, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = MFS_UNCHECKED;
    mii.id = IDM_ADAS_SETTING;
    mii.typedata = (DWORD)setting[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)setting[i];

    InsertMenuItem(hmnu, 4, TRUE, &mii);

    return hmnu;
}

//?-------------------白平衡菜单实现-------------
static HMENU createpmenufileWB()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)WB[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)WB[i];

    hmnu = CreatePopupMenu(&mii);
    hmnuForBr = hmnu;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_wb() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_bright1;
    mii.typedata = (DWORD)Br_Auto[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)Br_Auto[i];

    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 1, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_wb() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_bright2;
    mii.typedata = (DWORD)Br_Sun[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)Br_Sun[i];

    InsertMenuItem(hmnu, 2, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 3, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_wb() == 2) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_bright3;
    mii.typedata = (DWORD)Br_Flu[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)Br_Flu[i];

    InsertMenuItem(hmnu, 4, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 5, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_wb() == 3) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_bright4;
    mii.typedata = (DWORD)Br_Cloud[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)Br_Cloud[i];

    InsertMenuItem(hmnu, 6, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 7, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_wb() == 4) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_bright5;
    mii.typedata = (DWORD)Br_tun[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)Br_tun[i];

    InsertMenuItem(hmnu, 8, TRUE, &mii);

    return hmnu;
}
//-----曝光度-------------------------
static HMENU createpmenufileEXPOSAL()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)exposal[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)exposal[i];

    hmnu = CreatePopupMenu(&mii);
    hmnuForEx = hmnu;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_ex() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_exposal1;
    mii.typedata = (DWORD)ExposalData_0[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)ExposalData_0[i];

    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 1, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_ex() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_exposal2;
    mii.typedata = (DWORD)ExposalData_1[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)ExposalData_1[i];

    InsertMenuItem(hmnu, 2, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 3, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_ex() == 2) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_exposal3;
    mii.typedata = (DWORD)ExposalData_2[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)ExposalData_2[i];

    InsertMenuItem(hmnu, 4, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 5, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_ex() == 3) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_exposal4;
    mii.typedata = (DWORD)ExposalData_3[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)ExposalData_3[i];

    InsertMenuItem(hmnu, 6, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 7, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_ex() == 4) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_exposal5;
    mii.typedata = (DWORD)ExposalData_4[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)ExposalData_4[i];

    InsertMenuItem(hmnu, 8, TRUE, &mii);

    return hmnu;
}
//------------录音----------------------------
static HMENU createpmenufileRECORD()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)record[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)record[i];

    hmnu = CreatePopupMenu(&mii);
    hmnuForRe = hmnu;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_video_audio() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_recordOFF;
    mii.typedata = (DWORD)OFF[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)OFF[i];

    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 1, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_video_audio() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_recordON;
    mii.typedata = (DWORD)ON[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)ON[i];

    InsertMenuItem(hmnu, 2, TRUE, &mii);

    return hmnu;
}
//---------时间水印-------------------------
static HMENU createpmenufileMARK()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)water_mark[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)water_mark[i];

    hmnu = CreatePopupMenu(&mii);
    hmnuForMark = hmnu;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_video_mark() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_markOFF;
    mii.typedata = (DWORD)OFF[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)OFF[i];

    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 1, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_video_mark() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_markON;
    mii.typedata = (DWORD)ON[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)ON[i];

    InsertMenuItem(hmnu, 2, TRUE, &mii);

    return hmnu;
}

#ifdef GPS
/*---------------GPS-------------------*/
static HMENU createpmenufileGPS()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)gps[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)gps[i];

    hmnu = CreatePopupMenu(&mii);
    hmnuForMark = hmnu;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_gps_mark() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_GPSOFF;
    mii.typedata = (DWORD)OFF[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)OFF[i];

    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type = MFT_SEPARATOR;
    mii.state = 0;
    mii.id = 0;
    mii.typedata = 0;
    InsertMenuItem(hmnu, 1, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_gps_mark() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_GPSON;
    mii.typedata = (DWORD)ON[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)ON[i];

    InsertMenuItem(hmnu, 2, TRUE, &mii);

    return hmnu;
}

static HMENU createpmenufileGPS_watermark()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)gps_watermark[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)gps_watermark[i];

    hmnu = CreatePopupMenu(&mii);
    hmnuForMark = hmnu;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_gps_watermark() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_GPS_WATERMARK_OFF;
    mii.typedata = (DWORD)OFF[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)OFF[i];

    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type = MFT_SEPARATOR;
    mii.state = 0;
    mii.id = 0;
    mii.typedata = 0;
    InsertMenuItem(hmnu, 1, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_gps_watermark() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_GPS_WATERMARK_ON;
    mii.typedata = (DWORD)ON[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)ON[i];

    InsertMenuItem(hmnu, 2, TRUE, &mii);

    return hmnu;
}
#endif

static HMENU createpmenufileHot_Share()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)hot_share[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)hot_share[i];

    hmnu = CreatePopupMenu(&mii);
    hmnuForHotShare = hmnu;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_hot_share() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_HOT_SHAREOFF;
    mii.typedata = (DWORD)OFF[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)OFF[i];

    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type = MFT_SEPARATOR;
    mii.state = 0;
    mii.id = 0;
    mii.typedata = 0;
    InsertMenuItem(hmnu, 1, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_hot_share() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_HOT_SHAREON;
    mii.typedata = (DWORD)ON[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)ON[i];

    InsertMenuItem(hmnu, 2, TRUE, &mii);

    return hmnu;
}

#if ENABLE_MD_IN_MENU
//--------------------移动侦测---------------------
static HMENU createpmenufileDETECT()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)auto_detect[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)auto_detect[i];

    hmnu = CreatePopupMenu(&mii);
    hmnuForDe = hmnu;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_video_de() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_detectOFF;
    mii.typedata = (DWORD)OFF[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)OFF[i];

    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 1, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_video_de() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_detectON;
    mii.typedata = (DWORD)ON[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)ON[i];

    InsertMenuItem(hmnu, 2, TRUE, &mii);

    return hmnu;
}
#endif

static HMENU createpmenufileMP_photo()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;
    struct photo_param *param = parameter_get_photo_param();
    char* MPVed_1M[2] = {" 1280 * 720 ", " 1280 * 720 "};
    char* MPVed_2M[2] = {" 1920 * 1080 ", " 1920 * 1080 "};
    char* MPVed_3M[2] = {" 2560 * 1440 ", " 2560 * 1440 "};

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)MP[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)MP[i];

    hmnu = CreatePopupMenu(&mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (param->height == 720) ? MF_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_1M_ph;
    mii.typedata = (DWORD)MPVed_1M[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)MPVed_1M[i];

    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 1, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (param->height == 1080) ? MF_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_2M_ph;
    mii.typedata = (DWORD)MPVed_2M[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)MPVed_2M[i];

    InsertMenuItem(hmnu, 2, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 3, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (param->height == 1440) ? MF_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_3M_ph;
    mii.typedata = (DWORD)MPVed_3M[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)MPVed_3M[i];

    InsertMenuItem(hmnu, 4, TRUE, &mii);

    return hmnu;
}
//------------清晰度菜单--------
static HMENU createpmenufileMP()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;
    int cnt = 0;
    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)MP[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)MP[i];

    hmnu = CreatePopupMenu(&mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_FONTCAMERA;
    mii.typedata = (DWORD)fontcamera_ui[0];
    mii.hsubmenu = createpmenufileFONTCAMERA();
    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)fontcamera_ui[i];
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_BACKCAMERA;
    mii.typedata = (DWORD)backcamera_ui[0];
    mii.hsubmenu = createpmenufileBACKCAMERA();

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)backcamera_ui[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_SEPARATOR;
    mii.state = 0;
    mii.id = 0;
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_CIFCAMERA;
    mii.typedata = (DWORD)cifcamera_ui[0];
    mii.hsubmenu = createpmenufileCIFCamera();

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)cifcamera_ui[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    return hmnu;
}

//-------------录像时长菜单-----------
static HMENU createpmenufileTIME()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)Record_Time[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)Record_Time[i];

    hmnu = CreatePopupMenu(&mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_recodetime() == 60) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_1MIN;
    mii.typedata = (DWORD)Time_OneMIN[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)Time_OneMIN[i];

    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 1, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_recodetime() == 180) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_3MIN;
    mii.typedata = (DWORD)Time_TMIN[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)Time_TMIN[i];

    InsertMenuItem(hmnu, 2, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 3, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_recodetime() == 300) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_5MIN;
    mii.typedata = (DWORD)Time_FMIN[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)Time_FMIN[i];

    InsertMenuItem(hmnu, 4, TRUE, &mii);
#if 0
    mii.type = MFT_SEPARATOR; //类型
    mii.state = 0;
    mii.id = 0; //ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 5, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_recodetime() == 3) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_OFF;
    mii.typedata = (DWORD)OFF[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char *)OFF[i];

    InsertMenuItem(hmnu, 6, TRUE, &mii);
#endif

    return hmnu;
}
//-----------------------录像模式菜单-------------------------
static HMENU createpmenufileCAR()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)Record_Mode[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)Record_Mode[i];

    hmnu = CreatePopupMenu(&mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_abmode() == 0) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_CAR1;
    mii.typedata = (DWORD)Front_Record[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)Front_Record[i];

    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 1, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_abmode() == 1) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_CAR2;
    mii.typedata = (DWORD)Back_Record[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)Back_Record[i];

    InsertMenuItem(hmnu, 2, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 3, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_abmode() == 2) ? MFS_CHECKED : MFS_UNCHECKED;
    mii.id = IDM_CAR3;
    mii.typedata = (DWORD)Double_Record[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)Double_Record[i];

    InsertMenuItem(hmnu, 4, TRUE, &mii);

    return hmnu;
}

// DEBUG
static HMENU createpmenufileDEBUG()
{
    int i;
    int cnt = 0;
    HMENU hmnu;
    MENUITEMINFO mii;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)debug[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)debug[i];

    hmnu = CreatePopupMenu(&mii);

    /* FW_UPDATE TEST */
    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_DEBUG_FWUPDATE_TEST;
    mii.typedata = (DWORD)str_fw_update_test[0];
    mii.hsubmenu = 0;

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)str_fw_update_test[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

#ifdef GPS
    /* gps watermark */
    mii.type = MFT_SEPARATOR;
    mii.state = 0;
    mii.id = 0;
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_GPS_WATERMARK;
    mii.typedata = (DWORD)gps_watermark[0];
    mii.hsubmenu = createpmenufileGPS_watermark();

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)gps_watermark[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);
#endif

    mii.type = MFT_SEPARATOR;
    mii.state = 0;
    mii.id = 0;
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_BOOT;
    mii.typedata = (DWORD)reboot[0];
    mii.hsubmenu = createpmenufileREBOOT();

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)reboot[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_RECOVERY_DEBUG;
    mii.typedata = (DWORD)recovery_debug[0];
    mii.hsubmenu = createpmenufileRECOVERY();

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)recovery_debug[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_STANDBY_2;
    mii.typedata = (DWORD)standby[0];
    mii.hsubmenu = createpmenufileSTANDBY();

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)standby[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_MODE_CHANGE;
    mii.typedata = (DWORD)mode_change[0];
    mii.hsubmenu = createpmenufileMODECHANGE();

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)mode_change[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_DEBUG_VIDEO;
    mii.typedata = (DWORD)video_debug[0];
    mii.hsubmenu = createpmenufileDEBUGVIDEO();

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)video_debug[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_BEG_END_VIDEO;
    mii.typedata = (DWORD)beg_end_video[0];
    mii.hsubmenu = createpmenufileBEGENDVIDEO();

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)beg_end_video[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_DEBUG_PHOTO;
    mii.typedata = (DWORD)photo_debug[0];
    mii.hsubmenu = createpmenufileDEBUGPHOTO();

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)photo_debug[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_DEBUG_TEMP;
    mii.typedata = (DWORD)temp_debug[0];
    mii.hsubmenu = createpmenufileDEBUGTEMP();

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)temp_debug[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    add_seperator_line(hmnu, &mii, &cnt);
    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_DEBUG_VIDEO_BIT_RATE;
    mii.typedata = (DWORD)vbit_rate_debug[0];
    mii.hsubmenu = createpmenufileDEBUGVideoBitRate();

    for (i = 0; i < 2; i++)
        mii.str[i] = (char*)vbit_rate_debug[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_EFFECT_WATERMARK;
    mii.typedata = (DWORD)effect_watermark[0];
    mii.hsubmenu = createpmenufileEFFECTWATERMARK();

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)effect_watermark[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    return hmnu;
}

//-------------缩时录影间隔时长菜单-----------

#define ADD_CHILD_ITEM(get_func, comp_val, idm_id, item_str) \
  do { \
    memset(&mii, 0, sizeof(MENUITEMINFO)); \
    mii.type = MFT_RADIOCHECK; \
    mii.state = (get_func() == comp_val) ? MFS_CHECKED : MFS_UNCHECKED; \
    mii.id = idm_id; \
    mii.typedata = (DWORD)item_str[0]; \
    for (i = 0; i < LANGUAGE_NUM; i++) \
      mii.str[i] = (char*)item_str[i]; \
    InsertMenuItem(hmnu, cnt++, TRUE, &mii); \
  } while(0)

static HMENU create_menu_time_lapse()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;
    int cnt = 0;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)time_lapse[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)time_lapse[i];

    hmnu = CreatePopupMenu(&mii);

    ADD_CHILD_ITEM(parameter_get_time_lapse_interval, 0,
                   IDM_TIME_LAPSE_OFF, time_off);
    add_seperator_line(hmnu, &mii, &cnt);
    ADD_CHILD_ITEM(parameter_get_time_lapse_interval, 1,
                   IDM_TIME_LAPSE_INTERNAL_1s, time_one_sec);
    add_seperator_line(hmnu, &mii, &cnt);
    ADD_CHILD_ITEM(parameter_get_time_lapse_interval, 5,
                   IDM_TIME_LAPSE_INTERNAL_5s, time_five_sec);
    add_seperator_line(hmnu, &mii, &cnt);
    ADD_CHILD_ITEM(parameter_get_time_lapse_interval, 10,
                   IDM_TIME_LAPSE_INTERNAL_10s, time_ten_sec);
    add_seperator_line(hmnu, &mii, &cnt);
    ADD_CHILD_ITEM(parameter_get_time_lapse_interval, 30,
                   IDM_TIME_LAPSE_INTERNAL_30s, time_thirty_sec);
    add_seperator_line(hmnu, &mii, &cnt);
    ADD_CHILD_ITEM(parameter_get_time_lapse_interval, 60,
                   IDM_TIME_LAPSE_INTERNAL_60s, time_sixty_sec);
    return hmnu;
}

static HMENU createpmenufileVIDEOQUALITY()
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;
    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)video_quality[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)video_quality[i];

    hmnu = CreatePopupMenu(&mii);
    hmnuForVideoQuality = hmnu;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_bit_rate_per_pixel() == VIDEO_QUALITY_HIGH)
                ? MFS_CHECKED
                : MFS_UNCHECKED;
    mii.id = IDM_VIDEO_QUALITY_H;
    mii.typedata = (DWORD)quality_hig[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)quality_hig[i];

    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type = MFT_SEPARATOR;
    mii.state = 0;
    mii.id = 0;
    mii.typedata = 0;
    InsertMenuItem(hmnu, 1, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_bit_rate_per_pixel() == VIDEO_QUALITY_MID)
                ? MFS_CHECKED
                : MFS_UNCHECKED;
    mii.id = IDM_VIDEO_QUALITY_M;
    mii.typedata = (DWORD)quality_mid[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)quality_mid[i];

    InsertMenuItem(hmnu, 2, TRUE, &mii);

    mii.type = MFT_SEPARATOR;
    mii.state = 0;
    mii.id = 0;
    mii.typedata = 0;
    InsertMenuItem(hmnu, 3, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_RADIOCHECK;
    mii.state = (parameter_get_bit_rate_per_pixel() == VIDEO_QUALITY_LOW)
                ? MFS_CHECKED
                : MFS_UNCHECKED;
    mii.id = IDM_VIDEO_QUALITY_L;
    mii.typedata = (DWORD)quality_low[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)quality_low[i];

    InsertMenuItem(hmnu, 4, TRUE, &mii);

    return hmnu;
}

//----------------设置菜单----------------------
static HMENU createpmenufileSETTING()
{
    int i;
    int cnt = 0;
    HMENU hmnu;
    MENUITEMINFO mii;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD)setting[0];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)setting[i];

    hmnu = CreatePopupMenu(&mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_IDC;
    mii.typedata = (DWORD)DSP_IDC[0];
    mii.hsubmenu = createpmenufileIDC();

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)DSP_IDC[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    mii.type = MFT_SEPARATOR;
    mii.state = 0;
    mii.id = 0;
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_FLIP;
    mii.typedata = (DWORD)video_flip[0];
    mii.hsubmenu = createpmenufileVIDEOFLIP();

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)video_flip[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    mii.type = MFT_SEPARATOR;
    mii.state = 0;
    mii.id = 0;
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    // debug
    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_3DNR;
    mii.typedata = (DWORD)DSP_3DNR[0];
    mii.hsubmenu = createpmenufile3DNR();

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)DSP_3DNR[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    mii.type = MFT_SEPARATOR;
    mii.state = 0;
    mii.id = 0;
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_CVBSOUT;
    mii.typedata = (DWORD)DISP_CVBSOUT[0];
    mii.hsubmenu = createpmenufileCVBSOUT();

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)DISP_CVBSOUT[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    mii.type = MFT_SEPARATOR;
    mii.state = 0;
    mii.id = 0;
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_MIRROR;
    mii.typedata = (DWORD)ui_cif_mirror[0];
    mii.hsubmenu = createpmenufileCIFMIRROR();

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)ui_cif_mirror[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_ADAS;
    mii.typedata = (DWORD)DSP_ADAS[0];
    mii.hsubmenu = createpmenufileADAS();

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)DSP_ADAS[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    mii.type = MFT_SEPARATOR;
    mii.state = 0;
    mii.id = 0;
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_bright;
    mii.typedata = (DWORD)WB[0];
    mii.hsubmenu = createpmenufileWB();

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)WB[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_exposal;
    mii.typedata = (DWORD)exposal[0];
    mii.hsubmenu = createpmenufileEXPOSAL();

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)exposal[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

#if ENABLE_MD_IN_MENU
    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_detect;
    mii.typedata = (DWORD)auto_detect[0];
    mii.hsubmenu = createpmenufileDETECT();

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)auto_detect[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);
#endif

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_mark;
    mii.typedata = (DWORD)water_mark[0];
    mii.hsubmenu = createpmenufileMARK();

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)water_mark[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

#ifdef GPS
    //gps
    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_GPS;
    mii.typedata = (DWORD)gps[0];
    mii.hsubmenu = createpmenufileGPS();

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)gps[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    mii.type = MFT_SEPARATOR;
    mii.state = 0;
    mii.id = 0;
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);
#endif

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_record;
    mii.typedata = (DWORD)record[0];
    mii.hsubmenu = createpmenufileRECORD();

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)record[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    // createpmenufileKEYTONE
    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_KEY_VOLUME;
    mii.typedata = (DWORD)key_volume[0];
    mii.hsubmenu = createpmenufileKEYTONE();

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)key_volume[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    // createpmenufileAUTORECORD
    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_ABOUT_AUTORECORD;
    mii.typedata = (DWORD)auto_record[0];
    mii.hsubmenu = createpmenufileAUTORECORD();

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)auto_record[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_ABOUT_LANGUAGE;
    mii.typedata = (DWORD)language[0];
    mii.hsubmenu = createpmenufileLANGUAGE();

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)language[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_ABOUT_FREQUENCY;
    mii.typedata = (DWORD)frequency[0];
    mii.hsubmenu = createpmenufileFREQUENCY();

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)frequency[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    //---------------------屏幕自动关闭--
    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_AUTOOFFSCREEN;
    mii.typedata = (DWORD)screenoffset[0];
    mii.hsubmenu = createpmenufileAUTOOFFSCREEN();

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)screenoffset[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    //---------------------WIFI开关--
    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_WIFI;
    mii.typedata = (DWORD)wifi[0];
    mii.hsubmenu = createpmenufileWIFIONOFF();

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)wifi[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    //---------------------背光亮度--
    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_BACKLIGHT;
    mii.typedata = (DWORD)backlight[0];
    mii.hsubmenu = createpmenufileBACKLIGHT();

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)backlight[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);
    //---------usb mode--------------
    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDC_SETMODE;
    mii.typedata = (DWORD)usbmode[0];
    mii.hsubmenu = createpmenufileUSB();

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)usbmode[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    //------------Format-------------------

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_FORMAT;
    mii.typedata = (DWORD)format[0];
    mii.hsubmenu = 0;

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)format[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);
    //--------------set date -------------
    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_SETDATE;
    mii.typedata = (DWORD)setdate[0];
    mii.hsubmenu = 0;

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)setdate[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    //-----碰撞检测-----
    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_COLLISION;
    mii.typedata = (DWORD)collision[0];
    mii.hsubmenu = createpmenufileCOLLISION();

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)collision[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    //-----------------------
    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_PARKINGMONITOR;
    mii.typedata = (DWORD)parking_monitor[0];
    mii.hsubmenu = createpmenufileLEAVECARREC();

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)parking_monitor[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    //---------recovery--------------
    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_RECOVERY;
    mii.typedata = (DWORD)recovery[0];
    mii.hsubmenu = 0;

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)recovery[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    mii.type = MFT_SEPARATOR;
    mii.state = 0;
    mii.id = 0;
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_VIDEO_QUALITY;
    mii.typedata = (DWORD)video_quality[0];
    mii.hsubmenu = createpmenufileVIDEOQUALITY();

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)video_quality[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    /* -------- License plate watermark------------ */
    mii.type = MFT_SEPARATOR;
    mii.state = 0;
    mii.id = 0;
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_LICENSEPLATE_WATERMARK;
    mii.typedata = (DWORD)license_plate_watermark[0];
    mii.hsubmenu = 0;

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)license_plate_watermark[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    /* -------- phone call ------------ */
    mii.type = MFT_SEPARATOR;
    mii.state = 0;
    mii.id = 0;
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_PHONE_CALL;
    mii.typedata = (DWORD)phone_call[0];
    mii.hsubmenu = 0;

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)phone_call[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    /* -------- Hot share ------------ */
    mii.type = MFT_SEPARATOR;
    mii.state = 0;
    mii.id = 0;
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_HOT_SHARE;
    mii.typedata = (DWORD)hot_share[0];
    mii.hsubmenu = createpmenufileHot_Share();

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)hot_share[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    //---------fw ver--------------
    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_FWVER;
    mii.typedata = (DWORD)fw_ver[0];
    mii.hsubmenu = 0;

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)fw_ver[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    /* time lapse setting */
    mii.type = MFT_SEPARATOR;
    mii.state = 0;
    mii.id = 0;
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_timelapse;
    mii.typedata = (DWORD)time_lapse[0];
    mii.hsubmenu = create_menu_time_lapse();
    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)time_lapse[i];
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    /* Firmware Update */
    mii.type = MFT_SEPARATOR;
    mii.state = 0;
    mii.id = 0;
    mii.typedata = 0;
    InsertMenuItem(hmnu, cnt++, TRUE, &mii);
    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.state = 0;
    mii.id = IDM_SETTING_FWUPDATE;
    mii.typedata = (DWORD)str_fw_update[0];
    mii.hsubmenu = 0;

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)str_fw_update[i];

    InsertMenuItem(hmnu, cnt++, TRUE, &mii);

    return hmnu;
}

static HMENU creatmenu(void)  //创建主
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;
    //    HDC hdcForFont;
    //    hdcForFont = GetDC(hMainWndForPlay);
    hmnu = CreateMenu();
    hmnuMain = hmnu;
    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_BITMAP;  //类型
    mii.state = 0;
    mii.id = IDM_ABOUT_MP;  // ID
    mii.typedata = (DWORD)MP[0];
    mii.uncheckedbmp = &bmp_menu_mp[0];
    mii.checkedbmp = &bmp_menu_mp[1];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)MP[i];

    mii.hsubmenu = createpmenufileMP();
    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 1, TRUE, &mii);

    mii.type = MFT_BITMAP;
    mii.state = 0;
    mii.id = IDM_ABOUT_TIME;
    mii.typedata = (DWORD)Record_Time[0];
    mii.uncheckedbmp = &bmp_menu_time[0];
    mii.checkedbmp = &bmp_menu_time[1];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)Record_Time[i];

    mii.hsubmenu = createpmenufileTIME();
    InsertMenuItem(hmnu, 2, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 3, TRUE, &mii);

    mii.type = MFT_BITMAP;
    mii.state = 0;
    mii.id = IDM_ABOUT_CAR;
    mii.typedata = (DWORD)Record_Mode[0];
    mii.uncheckedbmp = &bmp_menu_car[0];
    mii.checkedbmp = &bmp_menu_car[1];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)Record_Mode[i];

    mii.hsubmenu = createpmenufileCAR();
    InsertMenuItem(hmnu, 4, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 5, TRUE, &mii);

    mii.type = MFT_BITMAP;
    mii.state = 0;
    mii.id = IDM_ABOUT_SETTING;
    mii.typedata = (DWORD)setting[0];
    mii.uncheckedbmp = &bmp_menu_setting[0];
    mii.checkedbmp = &bmp_menu_setting[1];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)setting[i];

    mii.hsubmenu = createpmenufileSETTING();
    InsertMenuItem(hmnu, 6, TRUE, &mii);

    // debug
    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 7, TRUE, &mii);

    mii.type = MFT_BITMAP;
    mii.state = 0;
    mii.id = IDM_ABOUT_DEBUG;
    mii.typedata = (DWORD)debug[0];
    mii.uncheckedbmp = &png_menu_debug[0];
    mii.checkedbmp = &png_menu_debug[1];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)debug[i];

    mii.hsubmenu = createpmenufileDEBUG();
    InsertMenuItem(hmnu, 8, TRUE, &mii);
    //
    return hmnu;
}

static HMENU creatmenu_photo(void)  //创建主
{
    int i;
    HMENU hmnu;
    MENUITEMINFO mii;
    printf("creatmenu_photo\n");
    hmnu = CreateMenu();
    hmnuMain = hmnu;
    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_BITMAP;  //类型
    mii.state = 0;
    mii.id = IDM_ABOUT_MP;  // ID
    mii.typedata = (DWORD)MP[0];
    mii.uncheckedbmp = &bmp_menu_mp[0];
    mii.checkedbmp = &bmp_menu_mp[1];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)MP[i];

    mii.hsubmenu = createpmenufileMP_photo();
    InsertMenuItem(hmnu, 0, TRUE, &mii);

    mii.type = MFT_SEPARATOR;  //类型
    mii.state = 0;
    mii.id = 0;  // ID
    mii.typedata = 0;
    InsertMenuItem(hmnu, 1, TRUE, &mii);

    mii.type = MFT_BITMAP;
    mii.state = 0;
    mii.id = IDM_ABOUT_SETTING;
    mii.typedata = (DWORD)setting[0];
    mii.uncheckedbmp = &bmp_menu_setting[0];
    mii.checkedbmp = &bmp_menu_setting[1];

    for (i = 0; i < LANGUAGE_NUM; i++)
        mii.str[i] = (char*)setting[i];

    mii.hsubmenu = createpmenufileSETTING();
    InsertMenuItem(hmnu, 2, TRUE, &mii);

    return hmnu;
}

static HMENU create_rightbutton_menu(void)
{
    int i;
    HMENU hMenu;
    MENUITEMINFO mii;
    char* msg[] = {open_, copy, delete_, rename, properties_};

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.type = MFT_STRING;
    mii.id = 0;
    mii.typedata = (DWORD) "File";

    hMenu = CreatePopupMenu(&mii);

    for (i = 0; i < 5; i++) {
        memset(&mii, 0, sizeof(MENUITEMINFO));
        mii.type = MFT_STRING;
        mii.id = IDM_FILE + i;
        mii.state = 0;
        mii.typedata = (DWORD)msg[i];
        InsertMenuItem(hMenu, i, TRUE, &mii);
    }

    return hMenu;
    // return StripPopupHead(hMenu);
}

void lv_notify_process(HWND hwnd, int id, int code, DWORD addData)
{
    if (code == LVN_KEYDOWN) {
        PLVNM_KEYDOWN down;
        int key;

        down = (PLVNM_KEYDOWN)addData;
        key = LOWORD(down->wParam);

        if (key == SCANCODE_REMOVE) {
            HLVITEM hlvi;
            hlvi = SendMessage(hwnd, LVM_GETSELECTEDITEM, 0, 0);
            if (hlvi) {
                if (MessageBox(hMainWnd, are_you_really_want_to_delete_this_file,
                               warning, MB_YESNO | MB_DEFBUTTON2) == IDYES) {
                    // not really delete yet.
                    SendMessage(hwnd, LVM_DELITEM, 0, hlvi);
                }
            }
        }
        if (key == SCANCODE_ENTER) {
        }
    }
    if (code == LVN_ITEMRUP) {
        PLVNM_ITEMRUP up;
        int x, y;

        up = (PLVNM_ITEMRUP)addData;
        x = LOSWORD(up->lParam);
        y = HISWORD(up->lParam);

        ClientToScreen(explorer_wnd, &x, &y);

        TrackPopupMenu(GetPopupSubMenu(explorer_menu),
                       TPM_LEFTALIGN | TPM_LEFTBUTTON, x, y, hMainWnd);
    }
    if (code == LVN_ITEMDBCLK) {
        HLVITEM hlvi = SendMessage(hwnd, LVM_GETSELECTEDITEM, 0, 0);
        if (hlvi > 0) {
            if (MessageBox(hMainWnd, Are_you_really_want_to_open_this_file, Question,
                           MB_YESNO) == IDYES) {
                MessageBox(hMainWnd, "Me too.", "Sorry", MB_OK);
            }
        }
    }
}

static void initrec(HWND hWnd)
{
    memset(&g_adas_process, 0, sizeof(g_adas_process));

    if (SetMode != MODE_PHOTO) {
        api_video_init(VIDEO_MODE_REC);
    } else {
        api_video_init(VIDEO_MODE_PHOTO);
    }

    InvalidateImg(hWnd, NULL, FALSE);
}

void startplayvideo(HWND hWnd, char* filename)
{
    printf("%s filename = %s\n", __func__, filename);
    changemode(hWnd, MODE_PLAY);
    snprintf(testpath, sizeof(testpath), "%s", filename);
    videoplay_init(filename);
}

void exitplayvideo(HWND hWnd)
{
    videoplay_deinit();
    changemode(hWnd, MODE_PREVIEW);
    video_play_state = 0;
    if (api_get_sd_mount_state() != SD_STATE_IN) {
        exitvideopreview();
        /*
        if (explorer_path == 0)
                explorer_path = calloc(1, 256);
        sprintf(explorer_path, "%s", SDCARD_PATH);
        explorer_update(explorer_wnd);
        */
    }
    InvalidateImg(hWnd, NULL, FALSE);
}

int explorer_getfiletype(char* filename)
{
    int type = FILE_TYPE_UNKNOW;
    char* ext_name;
    if (!filename || filename[0] == '.') {
        return type;
    }
    ext_name = strrchr(filename, '.');
    if (ext_name) {
        ext_name++;

        if (strcmp(ext_name, parameter_get_storage_format()) == 0) {
            type = FILE_TYPE_VIDEO;
        } else if (strcmp(ext_name, "jpg") == 0) {
            type = FILE_TYPE_PIC;
        }
    }

    return type;
}

static int explorer_update(HWND hWnd)
{
    printf("%s 1\n", __func__);
    explorer_update_reupdate = 1;
    printf("%s 2\n", __func__);

    return 0;
}

int explorer_enter(HWND hWnd)
{
    HLVITEM hItemSelected;
    LVITEM lvItem;

    hItemSelected = SendMessage(explorer_wnd, LVM_GETSELECTEDITEM, 0, 0);
    SendMessage(explorer_wnd, LVM_GETITEM, hItemSelected, (LPARAM)&lvItem);
    if (lvItem.itemData) {
        if (lvItem.dwFlags) {
            if (strcmp((char*)lvItem.itemData, ".") == 0) {
                sprintf(explorer_path, "%s", SDCARD_PATH);
            } else if (strcmp((char*)lvItem.itemData, "..") == 0) {
                char* seek = strrchr(explorer_path, '/');
                if (strcmp(explorer_path, SDCARD_PATH) > 0) {
                    explorer_path[seek - explorer_path] = 0;
                }
            } else {
                strcat(explorer_path, "/");
                strcat(explorer_path, (const char *)lvItem.itemData);
            }
            explorer_update(explorer_wnd);
        } else {
            int filetype = explorer_getfiletype((char*)lvItem.itemData);
            if (filetype == FILE_TYPE_VIDEO || filetype == FILE_TYPE_PIC) {
                // char file[128];
                snprintf(testpath, sizeof(testpath), "%s/%s", explorer_path,
                         (char*)lvItem.itemData);
                test_replay = 1;
                startplayvideo(hWnd, testpath);
            }
        }
    }

    return 0;
}

int explorer_back(HWND hWnd)
{
    char* seek = strrchr(explorer_path, '/');

    if (strcmp(explorer_path, SDCARD_PATH) > 0) {
        explorer_path[seek - explorer_path] = 0;
    }

    explorer_update(explorer_wnd);

    return 0;
}

int explorer_filedelete(HWND hWnd)
{
    HLVITEM hItemSelected;
    LVITEM lvItem;
    int mesg = 0;
    hItemSelected = SendMessage(explorer_wnd, LVM_GETSELECTEDITEM, 0, 0);
    SendMessage(explorer_wnd, LVM_GETITEM, hItemSelected, (LPARAM)&lvItem);

    if (lvItem.dwFlags == 0) {
        if (lvItem.itemData) {
            if (parameter_get_video_lan() == 1)
                mesg = MessageBox(explorer_wnd, "删除此视频?", "提示", MB_YESNO | MB_DEFBUTTON2);
            else if (parameter_get_video_lan() == 0)
                mesg =
                    MessageBox(explorer_wnd, "Delete this video ?", "Prompt", MB_YESNO | MB_DEFBUTTON2);
            if (mesg == IDYES) {
                // char file[256];
                // sprintf(file, "%s/%s", explorer_path, lvItem.itemData);
                // printf("file = %s\n", file);
                // fs_manage_remove(explorer_path, (char *)lvItem.itemData);
                SendMessage(explorer_wnd, LVM_DELITEM, 0, (LPARAM)hItemSelected);
                // remove(file);
            }
        }
    }

    return 0;
}

void* explorer_update_thread(void* arg)
{
    LVSUBITEM subdata;
    LVITEM item;
    HWND hWnd = (HWND)arg;

    prctl(PR_SET_NAME, "explorer_update", 0, 0, 0);

    struct file_list* list;
    while (explorer_update_run) {
        if (explorer_update_reupdate == 0) {
            usleep(10000);
        } else {
            int i = 0;
            explorer_update_reupdate = 0;
            SendMessage(hWnd, LVM_DELALLITEM, 0, 0);
            // printf("%s 1, explorer_path = %s\n", __func__, explorer_path);
            list = fs_manage_getmediafilelist();
            if (list) {
                struct file_node* node_tmp;

                node_tmp = list->folder_tail;
                while (node_tmp && explorer_update_run && !explorer_update_reupdate) {
                    // printf("%s i =%d\n", __func__, i);
                    item.itemData = (DWORD)strdup(node_tmp->name);
                    item.nItem = i;
                    item.nItemHeight = 28;
                    item.dwFlags = LVIF_FOLD;
                    SendMessage(hWnd, LVM_ADDITEM, 0, (LPARAM)&item);

                    subdata.nItem = i;

                    subdata.subItem = 0;
                    subdata.flags = 0;
                    subdata.pszText = node_tmp->name;
                    subdata.flags = LVFLAG_BITMAP;

                    if ((strcmp(node_tmp->name, ".") == 0) ||
                        (strcmp(node_tmp->name, "..") == 0)) {
                        subdata.image = (DWORD)&back_bmap;
                    } else {
                        subdata.image = (DWORD)&folder_bmap;
                    }

                    subdata.nTextColor = PIXEL_blue;
                    SendMessage(hWnd, LVM_FILLSUBITEM, 0, (LPARAM)&subdata);

                    i++;
                    node_tmp = node_tmp->pre;
                    if ((i % 500) == 0)
                        usleep(10000);
                }

                node_tmp = list->file_tail;
                while (node_tmp && explorer_update_run && !explorer_update_reupdate) {
                    int filetype;
                    // printf("%s i =%d\n", __func__, i);
                    item.itemData = (DWORD)strdup(node_tmp->name);
                    item.nItem = i;
                    item.nItemHeight = 28;
                    item.dwFlags = 0;
                    SendMessage(hWnd, LVM_ADDITEM, 0, (LPARAM)&item);

                    subdata.nItem = i;

                    subdata.subItem = 0;
                    subdata.flags = 0;
                    subdata.pszText = node_tmp->name;
                    subdata.flags = LVFLAG_BITMAP;
                    filetype = explorer_getfiletype(node_tmp->name);

                    subdata.nTextColor = PIXEL_black;
                    subdata.image = (DWORD)&filetype_bmap[filetype];

                    SendMessage(hWnd, LVM_FILLSUBITEM, 0, (LPARAM)&subdata);
                    i++;
                    node_tmp = node_tmp->pre;
                    if ((i % 500) == 0)
                        usleep(10000);
                }
                fs_manage_free_filelist(&list);
                printf("add file finish\n");
            }
        }
    }
    printf("%s out\n", __func__);
    pthread_exit(NULL);
}

void create_explorer(HWND hWnd)
{
    LVCOLUMN s1;
    int width;

    explorer_menu = create_rightbutton_menu();

    width = g_rcScr.right - g_rcScr.left;

    pthread_mutex_lock(&window_mutex);
    explorer_wnd = CreateWindowEx(
                       CTRL_LISTVIEW, "List View",
                       WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | WS_BORDER, WS_EX_NONE,
                       IDC_LISTVIEW, g_rcScr.left, g_rcScr.top + TOPBK_IMG_H, width,
                       g_rcScr.bottom - TOPBK_IMG_H, hWnd, 0);
    pthread_mutex_unlock(&window_mutex);

    SetNotificationCallback(explorer_wnd, lv_notify_process);

    s1.nCols = 0;
    s1.pszHeadText = File_name;
    s1.width = width - 10;
    s1.pfnCompare = NULL;
    s1.colFlags = 0;
    SendMessage(explorer_wnd, LVM_ADDCOLUMN, 0, (LPARAM)&s1);

    if (explorer_path == 0)
        explorer_path = calloc(1, 256);
    sprintf(explorer_path, "%s", SDCARD_PATH);
    explorer_update_reupdate = 0;
    explorer_update_run = 1;
    if (pthread_create(&explorer_update_tid, NULL, explorer_update_thread,
                       (void*)explorer_wnd)) {
        printf("%s pthread_create err\n", __func__);
    }
    explorer_update(explorer_wnd);

    SetFocusChild(explorer_wnd);
}

void destroy_explorer(void)
{
    explorer_update_run = 0;
    if (explorer_update_tid)
        pthread_join(explorer_update_tid, NULL);
    explorer_update_tid = 0;
    DestroyMenu(explorer_menu);
    pthread_mutex_lock(&window_mutex);
    DestroyWindow(explorer_wnd);
    pthread_mutex_unlock(&window_mutex);
    if (explorer_path) {
        free(explorer_path);
        explorer_path = 0;
    }
    explorer_menu = 0;
    explorer_wnd = 0;
}

void popmenu(HWND hWnd)
{
    SendAsyncMessage(hWnd, MSG_MENUBARPAINT, 0, 0);
    TrackMenuBar(hWnd, 0);
}

void videopreview_getfilename(char* filename, struct file_node* filenode)
{
    char *filepath = NULL;

    if (filenode == NULL)
        return;

    filepath = fs_storage_folder_get_bynode(filenode);
    snprintf(filename, FILENAME_LEN, "%s/%s", filepath, filenode->name);

    return;
}

void videopreview_show_decode(char* previewname,
                              char* videoname,
                              char* filename)
{
    char suffix_fmt[8];
    videoplay = 0;
    if (previewname == NULL)
        return;

    preview_isvideo = 0;
    sprintf(previewname, "%s(%d/%d)", videoname, currentfilenum, totalfilenum);
    printf("%s file = %s, previewname = %s\n", __func__, filename, previewname);
    snprintf(suffix_fmt, sizeof(suffix_fmt), ".%s",
                 parameter_get_storage_format());
    if (strstr(previewname, suffix_fmt)) {
        printf("videoplay_decode_one_frame(%s)\n", filename);
        preview_isvideo = 1;
        videoplay = videoplay_decode_one_frame(filename);
        printf("videoplay= %d\n", videoplay);
        if ((videoplay != 0) && (parameter_get_debug_modechange() == 0)) {
            printf("videoplay==-1\n");
            if (parameter_get_video_lan() == 1)
                MessageBox(hMainWnd, "视频错误!!!", "警告!!!", MB_OK);
            else if (parameter_get_video_lan() == 0)
                MessageBox(hMainWnd, "video error !!!", "Warning!!!", MB_OK);
        }
    } else if (strstr(previewname, ".jpg")) {
        printf("videoplay_decode_jpeg(%s)\n", filename);
        videoplay_decode_jpeg(filename);
    }

    return;
}

void entervideopreview(void)
{
    char filename[FILENAME_LEN];

    totalfilenum = 0;
    currentfilenum = 0;
    preview_isvideo = 0;
    videoplay = 0;
    printf("%s\n", __func__);

    if (api_get_sd_mount_state() != SD_STATE_IN) {
        snprintf(previewname, sizeof(previewname), "%s", "no sd card");
        sprintf(previewname, "%s", "no sd card");
        return;
    }
    char cmd[] = "sync\0";
    runapp(cmd);  // sync when showing
    filelist = fs_manage_getmediafilelist();
    if (filelist == NULL) {
        snprintf(previewname, sizeof(previewname), "(%d/%d)", currentfilenum, totalfilenum);
        return;
    }
    currentfile = filelist->file_tail;

    if (currentfile == NULL) {
        snprintf(previewname, sizeof(previewname), "(%d/%d)", currentfilenum, totalfilenum);
        return;
    }
    totalfilenum = filelist->filenum;
    if (totalfilenum)
        currentfilenum = 1;

    currentfile = filelist->file_tail;

    videopreview_getfilename(filename, currentfile);
    videopreview_show_decode(previewname, currentfile->name, filename);

    return;
}

void videopreview_refresh(HWND hWnd)
{
    char filename[FILENAME_LEN];

    printf("%s\n", __func__);
    if (currentfile == NULL)
        return;

    videopreview_getfilename(filename, currentfile);
    videopreview_show_decode(previewname, currentfile->name, filename);
    InvalidateImg(hWnd, NULL, FALSE);

    return;
}

void videopreview_next(HWND hWnd)
{
    char filename[FILENAME_LEN];

    printf("%s\n", __func__);
    if (currentfile == NULL)
        return;

    if (currentfile->pre == NULL) {
        currentfile = filelist->file_tail;
        if (currentfile == NULL)
            return;
        currentfilenum = 1;
    } else {
        currentfile = currentfile->pre;
        currentfilenum++;
    }
    videopreview_getfilename(filename, currentfile);
    videopreview_show_decode(previewname, currentfile->name, filename);
    InvalidateImg(hWnd, NULL, FALSE);

    return;
}

void videopreview_pre(HWND hWnd)
{
    char filename[FILENAME_LEN];
    printf("%s\n", __func__);
    if (currentfile == NULL)
        return;

    if (currentfile->next == NULL) {
        currentfile = filelist->file_head;
        if (currentfile == NULL)
            return;
        currentfilenum = totalfilenum;
    } else {
        currentfile = currentfile->next;
        currentfilenum--;
    }
    if (currentfilenum < 0)
        currentfilenum = 0;

    videopreview_getfilename(filename, currentfile);
    videopreview_show_decode(previewname, currentfile->name, filename);
    InvalidateImg(hWnd, NULL, FALSE);

    return;
}

void videopreview_play(HWND hWnd)
{
    char filename[FILENAME_LEN];
    char suffix_fmt[8];
    printf("%s\n", __func__);
    if (currentfile == NULL)
        return;

    videopreview_getfilename(filename, currentfile);
    snprintf(suffix_fmt, sizeof(suffix_fmt), ".%s",
                 parameter_get_storage_format());

    if (strstr(currentfile->name, suffix_fmt)) {
        printf("videoplay_decode_one_frame(%s)\n", filename);
        startplayvideo(hWnd, filename);
    } else {
        if (parameter_get_debug_video() == 1)
            video_time = VIDEO_TIME - 1;
        if (parameter_get_debug_modechange() == 1) {
            // modechange_pre_flage==1;
            modechange_time = MODECHANGE_TIME - 1;
        }
    }
}

int videopreview_delete(HWND hWnd)
{
    char filename[FILENAME_LEN];
    struct file_node* current_filenode;
    int mesg = 0;

    if (currentfile == NULL)
        return 0;

    if (parameter_get_video_lan() == 1)
        mesg = MessageBox(hWnd, "删除此视频?", "提示", MB_YESNO | MB_DEFBUTTON2);
    else if (parameter_get_video_lan() == 0)
        mesg = MessageBox(hWnd, "Delete this video ?", "Prompt", MB_YESNO | MB_DEFBUTTON2);
    if (mesg == IDYES) {
        // char file[256];
        // sprintf(file, "%s/%s", explorer_path, lvItem.itemData);
        // printf("file = %s\n", file);
        current_filenode = currentfile;
        if (currentfile->pre == NULL) {
            currentfile = currentfile->next;
            currentfilenum--;
        } else {
            currentfile = currentfile->pre;
        }

        totalfilenum--;

        videopreview_getfilename(filename, current_filenode);
        fs_storage_remove(filename, 0);
        videoplay = 0;
        preview_isvideo = 0;

        if (currentfile == NULL) {
            snprintf(previewname, sizeof(previewname), "(%d/%d)", currentfilenum, totalfilenum);
            printf("%spreviewname = %s\n", __func__, previewname);
            videoplay_set_fb_black();
            goto out;
        }
        videopreview_getfilename(filename, currentfile);
        videopreview_show_decode(previewname, currentfile->name, filename);
    out:
        InvalidateImg(hWnd, NULL, FALSE);
        return 0;
    }

    return 0;
}

void exitvideopreview(void)
{
    printf("%s\n", __func__);
    fs_manage_free_mediafilelist();
    filelist = 0;
    currentfile = 0;
    totalfilenum = 0;
    currentfilenum = 0;
    preview_isvideo = 0;
    videoplay = 0;

    if (api_get_sd_mount_state() != SD_STATE_IN) {
        snprintf(previewname, sizeof(previewname), "%s", "no sd card");
    }
    videoplay_set_fb_black();
}

void changemode(HWND hWnd, int mode)
{
    api_change_mode(mode);
}

static void changemode_notify(HWND hWnd, int premode, int mode)
{
    switch (premode) {
    case MODE_PHOTO:
    case MODE_RECORDING:
        if (mode == MODE_USBDIALOG) {
            DestroyAllMenu();
        } else if (mode == MODE_PREVIEW) {
            DestroyAllMenu();
        } else if (mode == MODE_RECORDING || mode == MODE_PHOTO) {
            DestroyAllMenu();
            CreateMainWindowMenu(hWnd, creatmenu_photo());
        }
        break;
    case MODE_PLAY:
        if (mode == MODE_USBDIALOG) {
            test_replay = 0;
        } else if (mode == MODE_SUSPEND) {
            SetMode = MODE_PREVIEW;
        }
        break;
    case MODE_USBDIALOG:
        CreateMainWindowMenu(hWnd, creatmenu());
        break;
    case MODE_USBCONNECTION:
        break;
    case MODE_PREVIEW:
        if (mode == MODE_RECORDING || mode == MODE_PHOTO) {
            CreateMainWindowMenu(hWnd, creatmenu());
        }
        break;
    }
    if (mode != MODE_SUSPEND)
        SetMode = mode;
    InvalidateImg(hWnd, NULL, TRUE);
}

void loadingWaitBmp(HWND hWnd)
{
    ANIMATION* anim = CreateAnimationFromGIF89aFile(
                          HDC_SCREEN, "/usr/local/share/minigui/res/images/waitfor2.gif");
    if (anim == NULL) {
        printf("anim=NULL\n");
        return;
    }
    key_lock = 1;
    pthread_mutex_lock(&window_mutex);
    SetWindowAdditionalData(hWnd, (DWORD)anim);
    CreateWindow(CTRL_ANIMATION, "", WS_VISIBLE | ANS_AUTOLOOP, 190,
                 (WIN_W - 98) / 2, (WIN_H - 98) / 2, 98, 98, hWnd, (DWORD)anim);
    SendMessage(GetDlgItem(hWnd, 190), ANM_STARTPLAY, 0, 0);
    pthread_mutex_unlock(&window_mutex);

    return;
}

void unloadingWaitBmp(HWND hWnd)
{
    DWORD win_additionanl_data = GetWindowAdditionalData(hWnd);
    if (win_additionanl_data) {
        pthread_mutex_lock(&window_mutex);
        DestroyAnimation((ANIMATION*)win_additionanl_data, TRUE);
        DestroyAllControls(hWnd);
        pthread_mutex_unlock(&window_mutex);
    }
    key_lock = 0;
}

static void proc_MSG_USBCHAGE(HWND hWnd,
                              int message,
                              WPARAM wParam,
                              LPARAM lParam)
{
#if USB_MODE
    CTRLDATA UsbCtrl[] = {
        {
            CTRL_BUTTON, WS_VISIBLE | BS_DEFPUSHBUTTON | BS_PUSHBUTTON | WS_TABSTOP,
            90, 80, 100, 40, IDUSB, " 磁  盘 ", 0
        },
        {
            CTRL_BUTTON, WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP, 90, 130, 100, 40,
            USBBAT, " 充  电 ", 0
        }
    };
    CTRLDATA UsbCtrl_en[] = {
        {
            CTRL_BUTTON, WS_VISIBLE | BS_DEFPUSHBUTTON | BS_PUSHBUTTON | WS_TABSTOP,
            90, 80, 100, 40, IDUSB, " Disk ", 0
        },
        {
            CTRL_BUTTON, WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP, 90, 130, 100, 40,
            USBBAT, " Charging ", 0
        }
    };
    DLGTEMPLATE UsbDlg = {WS_VISIBLE, WS_EX_NONE, 0, 0, 300,  300,
                          SetDate,    0,          0, 2, NULL, 0
                         };
#endif
    int usb_sd_chage = (int)lParam;

    printf("MSG_USBCHAGE : usb_sd_chage = ( %d )\n", usb_sd_chage);

    if (usb_sd_chage == 1) {
#if USB_MODE
        changemode(hWnd, MODE_USBDIALOG);
        UsbDlg.w = g_rcScr.right - g_rcScr.left;
        UsbDlg.h = g_rcScr.bottom - g_rcScr.top;
        if (parameter_get_video_lan() == 1) {
            UsbCtrl[0].x = (UsbDlg.w - UsbCtrl[0].w) >> 1;
            UsbCtrl[0].y = (UsbDlg.h - 50) >> 1;

            UsbCtrl[1].x = (UsbDlg.w - UsbCtrl[1].w) >> 1;
            UsbCtrl[1].y = (UsbDlg.h + 50) >> 1;

            UsbDlg.controls = UsbCtrl;
        } else if (parameter_get_video_lan() == 0) {
            UsbCtrl_en[0].x = (UsbDlg.w - UsbCtrl_en[0].w) >> 1;
            UsbCtrl_en[0].y = (UsbDlg.h - 50) >> 1;

            UsbCtrl_en[1].x = (UsbDlg.w - UsbCtrl_en[1].w) >> 1;
            UsbCtrl_en[1].y = (UsbDlg.h + 50) >> 1;

            UsbDlg.controls = UsbCtrl_en;
        }

        DialogBoxIndirectParam(&UsbDlg, HWND_DESKTOP, UsbSDProc, 0L);
#else
        printf("usb_sd_chage == 1:USB_MODE==0\n");
        if (api_get_sd_mount_state() == SD_STATE_IN) {
            cvr_usb_sd_ctl(USB_CTRL, 1);
        } else if (api_get_sd_mount_state() == SD_STATE_OUT) {
            //  EndDialog(hDlg, wParam);
            if (parameter_get_video_lan() == 1)
                MessageBox(hWnd, "SD卡不存在!!!", "警告!!!", MB_OK);
            else if (parameter_get_video_lan() == 0)
                MessageBox(hWnd, "no SD card insertted!!!", "Warning!!!", MB_OK);
        }
#endif
    } else if (usb_sd_chage == 0) {
#if USB_MODE
        if (hWndUSB) {
            cvr_usb_sd_ctl(USB_CTRL, 0);
            EndDialog(hWndUSB, wParam);
            InvalidateImg(hWnd, &USB_rc, TRUE);
            hWndUSB = 0;
            changemode(hWnd, MODE_RECORDING);
        }
#else
        cvr_usb_sd_ctl(USB_CTRL, 0);
        printf("usb_sd_chage == 0:USB_MODE==0\n");
#endif
    }
}

static void proc_record_SCANCODE_CURSORBLOCKRIGHT(HWND hWnd)
{
#ifdef USE_CIF_CAMERA
    short inputid = parameter_get_cif_inputid();
    inputid = (inputid == 0) ? 1 : 0;
    parameter_save_cif_inputid(inputid);
    api_video_reinit();
#endif

    video_record_inc_nv12_raw_cnt();
}

static void ui_takephoto(HWND hWnd)
{
    static bool test = false;

    if (test) {
        test = false;
        set_uvc_window_two(VIDEO_TYPE_CIF);
        //set_uvc_window_one(VIDEO_TYPE_ISP);
		printf("cif\n");
    } else {
        test = true;
        set_uvc_window_two(VIDEO_TYPE_ISP);
        //set_uvc_window_one(VIDEO_TYPE_USB);
		printf("isp\n");
    }

    if (api_get_sd_mount_state() == SD_STATE_IN && !takephoto) {
        takephoto = true;
        audio_sync_play(capture_sound_file);
        loadingWaitBmp(hWnd);
        if (video_record_takephoto(1) == -1) {
            printf("No input video devices!\n");
            unloadingWaitBmp(hWnd);
            takephoto = false;
        }
    }
}

static void proc_MSG_SDMOUNTFAIL(HWND hWnd)
{
    int mesg = 0;

    printf("MSG_SDMOUNTFAIL\n");
    if (parameter_get_video_lan() == 1)
        mesg =
            MessageBox(hWnd, "加载sd卡失败，\n是否格式化sd卡?", "警告!!!", MB_YESNO | MB_DEFBUTTON2);
    else if (parameter_get_video_lan() == 0)
        mesg = MessageBox(hWnd, "sd card loading failed, \nformat the sd card ?",
                          "Warning!!!", MB_YESNO);
    if (mesg == IDYES) {
        api_sdcard_format(0);
    }
}

static void proc_MSG_FS_INITFAIL(HWND hWnd, int param)
{
    int mesg = 0;
    printf("FS_INITFAIL\n");
    // no space
    if (param == -1) {
        if (parameter_get_video_lan() == 1)
            mesg = MessageBox(hWnd, "sd卡剩余空间不足\n是否格式化sd卡?", "警告!!!", MB_YESNO | MB_DEFBUTTON2);
        else if (parameter_get_video_lan() == 0)
            mesg = MessageBox(hWnd, "sd card no enough free space ,\nformat the sd card ?",
                              "Warning!!!", MB_YESNO);
        // init fail
    } else if (param == -2) {
        if (parameter_get_video_lan() == 1)
            mesg = MessageBox(hWnd, "sd卡初始化异常\n是否格式化sd卡?", "警告!!!", MB_YESNO | MB_DEFBUTTON2);
        else if (parameter_get_video_lan() == 0)
            mesg = MessageBox(hWnd, "sd card init fail ,\nformat the sd card ?",
                              "Warning!!!", MB_YESNO);
        // else error
    } else {
        if (parameter_get_video_lan() == 1)
            MessageBox(hWnd, "sd卡初始化异常", "警告!!!", MB_OK);
        else if (parameter_get_video_lan() == 0)
            MessageBox(hWnd, "sd card init fail", "Warning!!!", MB_OK);
        return;
    }

    if (mesg == IDYES) {
        api_sdcard_format(0);
    }

    return;
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

static void debug_cleancount(void)
{
    reboot_time = 0;
    recovery_time = 0;
    modechange_time = 0;
    standby_time = 0;
    beg_end_video_time = 0;
    video_time = 0;
    photo_time = 0;
    fwupdate_test_time = 0;
}

static void ui_timer_debug_process(HWND hWnd)
{
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
            parameter_save_debug_recovery(1);
        }
    }
    if (parameter_get_debug_modechange() == 1) {
        modechange_time++;
        if (SetMode == MODE_RECORDING) {
            if (api_video_get_record_state() == VIDEO_STATE_PREVIEW) {
                if (api_get_sd_mount_state() == SD_STATE_IN) {
                    api_start_rec();
                }
            } else if ((api_video_get_record_state() == VIDEO_STATE_RECORD) && modechange_time == MODECHANGE_TIME) {
                api_stop_rec();
            }
            if (modechange_time == MODECHANGE_TIME) {
                modechange_time = 0;
                changemode(hWnd, MODE_PREVIEW);
            }
        } else if (SetMode == MODE_PREVIEW) {
            /*
            if(modechange_time ==1) {
              if(videoplay!=0) {
                //  printf("videoplay==-1\n");
                if(parameter_get_video_lan()==1) {
                  modechange_time=MODECHANGE_TIME-1;
                } else if(parameter_get_video_lan()==0) {
                  modechange_time=MODECHANGE_TIME-1;
                }
              } else {
                videopreview_play(hWnd);
              }
            }*/
            if (modechange_time == MODECHANGE_TIME /*&&modechange_pre_flage==1*/) {
                //modechange_pre_flage =0 ;
                modechange_time = 0;
                changemode(hWnd, MODE_RECORDING);
            }
        }
    }

    if (parameter_get_debug_standby()) {
        standby_time++;
        if (standby_time == STANDBY_TIME) {
            runapp("echo +10 > /sys/class/rtc/rtc0/wakealarm");
            changemode(hWnd, MODE_SUSPEND);
            standby_time = 0;
        }
    }

    if (parameter_get_debug_video() == 1) {
        if (SetMode == MODE_PREVIEW) {
            if (video_time == 0) {
                if (videoplay != 0) {
                    //  printf("videoplay==-1\n");
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
        if (SetMode == MODE_RECORDING) {
            beg_end_video_time++;
            if (beg_end_video_time == 1) {
                if (api_video_get_record_state() == VIDEO_STATE_PREVIEW) {
                    if (api_get_sd_mount_state() == SD_STATE_IN) {
                        api_start_rec();
                    }
                }
            }
            if (beg_end_video_time == BEG_END_VIDEO_TIME) {
                if (api_video_get_record_state() == VIDEO_STATE_RECORD) {
                    api_stop_rec();
                }
                beg_end_video_time = 0;
            }
        }
    }
    if (parameter_get_debug_photo() == 1) {
        if (SetMode == MODE_RECORDING) {
            photo_time++;
            if (photo_time == PHOTO_TIME) {
                ui_takephoto(hWnd);
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

#ifdef USE_DISP_TS
static void PaintHorizontalBaseline(int width, int height, void* dstbuf,
    COLOR_Type color_type, int bold, int value, RECT *img)
{
    YUV_Color color;
    YUV_Point pos[2];

    color = set_yuv_color(color_type);
    pos[0].x = img->left;
    pos[0].y = value;
    pos[1].x = img->right;
    pos[1].y = value;

    if (bold) {
        pos[0].x = img->left;
        pos[0].y = value + 1;
        pos[1].x = img->right;
        pos[1].y = value + 1;
        yuv420_draw_line(dstbuf, width, height, pos[0], pos[1], color);
        pos[0].x = img->left;
        pos[0].y = value - 1;
        pos[1].x = img->right;
        pos[1].y = value - 1;
        yuv420_draw_line(dstbuf, width, height, pos[0], pos[1], color);
    }
}

void yuv420_display_adas(int width, int height, void* dstbuf)
{
    YUV_Color color;
    int i = 0;

    if (SetMode == MODE_RECORDING && parameter_get_video_adas()
        && (adasFlag & (ADASFLAG_SETTING | ADASFLAG_DRAW))) {
        /* Draw LDW lines. */
        if (g_adas_process.ldw_flag) {
            YUV_Point pos[4];
            color = set_yuv_color(COLOR_R);
            for (i = 0; i < 4; i++)
                pos[i] = *((YUV_Point*)&g_adas_process.ldw_pos[i]);
            yuv420_draw_line(dstbuf, width, height, pos[0], pos[1], color);
            yuv420_draw_line(dstbuf, width, height, pos[2], pos[3], color);
        }

        /* Draw FCW rectangles. */
        if (g_adas_process.fcw_count) {
            YUV_Rect rect_rio;
            color = set_yuv_color(COLOR_R);
            for (i = 0; i < g_adas_process.fcw_count; i++) {
                rect_rio = *((YUV_Rect*)&g_adas_process.fcw_out[i]);
                yuv420_draw_rectangle(dstbuf, width, height, rect_rio, color);
            }
        }
        adasFlag &= ~ADASFLAG_DRAW;

        /* Draw FCW setting Lines */
        if (adasFlag & ADASFLAG_SETTING) {
            struct adas_parameter parameter;
            char *store;
            int value;
            int bold;

            bold = (adasFlag & ADASFLAG_OPTION_SELECTED) >> 2;

            api_adas_get_parameter(&parameter);
            store = parameter.adas_setting;
            /* Draw Horizon */
            value = (int)(height * ((float)store[0] / 100));
            PaintHorizontalBaseline(width, height, dstbuf, COLOR_R,
                                    bold == ADASFLAG_OPTION_HORIZONTAL, value,
                                    &adas_rc);
            /* Draw head baseline */
            value = (int)(height * ((float)store[1] / 100));
            PaintHorizontalBaseline(width, height, dstbuf, COLOR_G,
                                    bold == ADASFLAG_OPTION_HEAD, value,
                                    &adas_rc);
        }
    }
}
#else
static void PaintHorizontalBaseline(HDC hdc, DWORD color, int bold, int height,
                                    RECT *img)
{
    int dist, bias, dif[2];

    if ((height < img->top) || (height > img->bottom))
        return;

    SetPenColor(hdc, color);

    MoveTo(hdc, img->left, height);
    LineTo(hdc, img->right, height);
    if (bold) {
        dif[0] = height - img->top;
        dif[1] = img->bottom - height;
        if (dif[0] > dif[1]) {
            dist = dif[1];
            bias = 1;
        } else {
            dist = dif[0];
            bias = 0;
        }

        if (!(bias && !dist)) {
            MoveTo(hdc, img->left, height + 1);
            LineTo(hdc, img->right, height + 1);
        }

        if (!(!bias && !dist)) {
            MoveTo(hdc, img->left, height - 1);
            LineTo(hdc, img->right, height - 1);
        }
    }
}

static void PaintVerticalBaseline(HDC hdc, DWORD color, int bold, int width,
                                  RECT *img)
{
    int dist, bias, dif[2];

    if ((width < img->left) || (width > img->right))
        return;

    SetPenColor(hdc, color);

    MoveTo(hdc, width, img->top);
    LineTo(hdc, width, img->bottom);

    if (bold) {
        dif[0] = width - img->left;
        dif[1] = img->right - width;
        if (dif[0] > dif[1]) {
            dist = dif[1];
            bias = 1;
        } else {
            dist = dif[0];
            bias = 0;
        }

        if (!(bias && !dist)) {
            MoveTo(hdc, width + 1, img->top);
            LineTo(hdc, width + 1, img->bottom);
        }

        if (!(!bias && !dist)) {
            MoveTo(hdc, width - 1, img->top);
            LineTo(hdc, width - 1, img->bottom);
        }
    }
}

static void AlertAdas(char *sound_path)
{
    if (adasFlag & ADASFLAG_ALERT)
        return;

    audio_play0(sound_path);
    adasFlag |= ADASFLAG_ALERT;
    AdasAlertCnt = ADAS_ALERT_CNT;
}

static void PaintADAS(HDC hdc)
{
    char setting_info[20];

    if (adasflagtooff == 1) {
        printf("adasflagtooff\n");
        SetBrushColor(hdc, bkcolor);
        SetBkColor(hdc, bkcolor);
        FillBox(hdc, adas_X, adas_Y, adas_W, adas_H);
        adasflagtooff = 0;
    }

    if (SetMode == MODE_RECORDING && parameter_get_video_adas()
        && (adasFlag & (ADASFLAG_SETTING | ADASFLAG_DRAW))) {
        SetBrushColor(hdc, bkcolor);
        SetBkColor(hdc, bkcolor);
        FillBox(hdc, adas_X, adas_Y, adas_W, adas_H);

        if (g_adas_process.ldw_flag) {
            SetPenColor(hdc, PIXEL_red);
            POINT *pos[4];
            int i;

            for (i = 0; i < 4; i++)
                pos[i] = (POINT *)&g_adas_process.ldw_pos[i];

            PaintLine(hdc, pos[0], pos[1]);
            PaintLine(hdc, pos[2], pos[3]);
        }

        if (g_adas_process.fcw_count) {
            int i = 0;
            for (i = 0; i < g_adas_process.fcw_count; i++) {
                RECT *img = (RECT *)(&g_adas_process.fcw_out[i].img);
                if (!g_adas_process.fcw_out[i].alert)
                    SetPenColor(hdc, PIXEL_green);
                else
                    SetPenColor(hdc, PIXEL_red);

                /* change adas_img to RECT */
                img->right += img->left;
                img->bottom += img->top;
                PaintRectBold(Rectangle, hdc, img);
                snprintf(setting_info, sizeof(setting_info), "%3.2f",
                         g_adas_process.fcw_out[i].distance);
                TextOut(hdc, img->left, img->bottom + 2, setting_info);
            }

            if (!g_adas_process.fcw_alert) {
                if (AdasAlertCnt)
                    AdasAlertCnt--;
                else
                    adasFlag &= ~ADASFLAG_ALERT;
            } else {
                AlertAdas(adasremind_sound_file);
            }
        }

        if (g_adas_process.car_status == ADAS_CAR_STATUS_STOP)
            SetTextColor(hdc, PIXEL_red);
        else if (g_adas_process.car_status == ADAS_CAR_STATUS_UNKNOWN)
            SetTextColor(hdc, PIXEL_lightwhite);
        else
            SetTextColor(hdc, PIXEL_green);

        snprintf(setting_info, sizeof(setting_info), "speed: %3.2f",
                 g_adas_process.speed);
        TextOut(hdc, 5, adas_Y + 160, setting_info);
        SetTextColor(hdc, PIXEL_lightwhite);

        adasFlag &= ~ADASFLAG_DRAW;

        /* Draw FCW setting Lines */
        if (adasFlag & ADASFLAG_SETTING) {
            struct adas_parameter parameter;
            char *store;
            int val;
            int bold;
            RECT select;

            bold = (adasFlag & ADASFLAG_OPTION_SELECTED) >> 2;

            api_adas_get_parameter(&parameter);
            store = parameter.adas_setting;
            /* Draw Horizon */
            val = (int)(WIN_H * ((float)store[0] / 100));
            PaintHorizontalBaseline(hdc, PIXEL_red,
                                    bold == ADASFLAG_OPTION_HORIZONTAL, val,
                                    &adas_rc);
            snprintf(setting_info, sizeof(setting_info), "horizon line: %3d%%",
                     store[0]);
            TextOut(hdc, 5, adas_Y, setting_info);

            /* Draw head baseline */
            val = (int)(WIN_H * ((float)store[1] / 100));
            PaintHorizontalBaseline(hdc, PIXEL_green,
                                    bold == ADASFLAG_OPTION_HEAD, val,
                                    &adas_rc);
            snprintf(setting_info, sizeof(setting_info), "head line:    %3d%%",
                     store[1]);
            TextOut(hdc, 5, adas_Y + 20, setting_info);

            /* Draw head direction */
            val = (int)(WIN_W * ((float)parameter.adas_direction / 100));
            PaintVerticalBaseline(hdc, PIXEL_blue,
                                  bold == ADASFLAG_OPTION_DIRECTION, val,
                                  &adas_rc);
            snprintf(setting_info, sizeof(setting_info), "direction:    %3d%%",
                     parameter.adas_direction);
            TextOut(hdc, 5, adas_Y + 40, setting_info);

            snprintf(setting_info, sizeof(setting_info), "distance:      %3d",
                     parameter.adas_alert_distance);
            TextOut(hdc, 5, adas_Y + 60, setting_info);

            snprintf(setting_info, sizeof(setting_info), "coefficient:   %3.2f",
                     parameter.coefficient);
            TextOut(hdc, 5, adas_Y + 80, setting_info);

            snprintf(setting_info, sizeof(setting_info), "base:          %3d",
                     parameter.base);
            TextOut(hdc, 5, adas_Y + 100, setting_info);

            select.left = 1;
            select.top = adas_Y + 1 + bold * 20;
            select.right = 155;
            select.bottom = adas_Y + 17 + bold * 20;
            SetPenColor(hdc, PIXEL_red);
            PaintRectBold(Rectangle, hdc, &select);
        }
    }
}
#endif

void keyprocess_adassetting(WPARAM key)
{
    char *p_setting = NULL;
    char limit_high, limit_low;
    struct adas_parameter parameter;
    int option;
    char tmp;

    option = (adasFlag & ADASFLAG_OPTION_SELECTED) >> 2;
    limit_low = 0;
    limit_high = 100;

    api_adas_get_parameter(&parameter);
    if (option == ADASFLAG_OPTION_HORIZONTAL) {
        p_setting = &parameter.adas_setting[0];
        limit_high = parameter.adas_setting[1];
    } else if (option == ADASFLAG_OPTION_HEAD) {
        p_setting = &parameter.adas_setting[1];
        limit_low = parameter.adas_setting[0];
    } else if (option == ADASFLAG_OPTION_DIRECTION) {
        p_setting = &parameter.adas_direction;
    } else if (option == ADASFLAG_OPTION_DISTANCE) {
        p_setting = &parameter.adas_alert_distance;
    } else if (option == ADASFLAG_OPTION_COEFICIENT) {
        tmp = parameter.coefficient * 20;
        p_setting = &tmp;
    } else if (option == ADASFLAG_OPTION_BASE) {
        p_setting = &parameter.base;
    }

    switch (key) {
    case SCANCODE_TAB:
        if (++option > ADASFLAG_OPTION_MAX)
            option = ADASFLAG_OPTION_HORIZONTAL;

        adasFlag &= ~ADASFLAG_OPTION_SELECTED;
        adasFlag |= option << 2;
        if (option == ADASFLAG_OPTION_COEFICIENT)
            tmp = parameter.coefficient * 20;
        break;
    case SCANCODE_CURSORBLOCKDOWN:
        if (*p_setting < limit_high)
            (*p_setting) ++;
        break;
    case SCANCODE_CURSORBLOCKRIGHT:
        if (*p_setting < (limit_high - 20) )
            *p_setting += 20;
        else
            *p_setting = limit_high;
        break;
    case SCANCODE_CURSORBLOCKUP:
        if (*p_setting > limit_low )
            (*p_setting) --;
        break;
    case SCANCODE_CURSORBLOCKLEFT:
        if (*p_setting > (limit_low + 20))
            *p_setting -= 20;
        else
            *p_setting = limit_low;
        break;
    case SCANCODE_ENTER:
        adasFlag &= ~ADASFLAG_SETTING;
        break;
    }

    if (option == ADASFLAG_OPTION_COEFICIENT)
        parameter.coefficient = (float)tmp / 20;
    api_adas_save_parameter(&parameter);
}

static int CameraWinProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    int tmp;

    // fprintf( stderr, "wParam =%d ,lParam =%d \n",(int)wParam,(int)lParam);
    switch (message) {
    case MSG_CREATE:
        SetTimer(hWnd, _ID_TIMER, 100);
        break;
    case MSG_HDMI: {
        //printf("%s wParam = %s, lParam = %d, hMainWnd = %d, hWnd = %d\n", __func__, wParam, lParam, hMainWnd, hWnd);
        if (lParam == 1) {
            rk_fb_set_lcd_backlight(0);
            rk_fb_set_out_device(OUT_DEVICE_HDMI);
            usleep(500000);
        } else if (rk_fb_get_out_device(&tmp, &tmp) == OUT_DEVICE_HDMI) {
            rk_fb_set_out_device(OUT_DEVICE_LCD);
        }

        if (SetMode == MODE_PREVIEW)
            videopreview_refresh(hWnd);
        else if (SetMode == MODE_USBDIALOG) {
            InvalidateImg(hWndUSB, NULL, TRUE);
            break;
        }
        InvalidateImg(hMainWnd, NULL, TRUE);

        if (lParam == 0)
            rk_fb_set_lcd_backlight(parameter_get_video_backlt());

        break;
    }

    case MSG_CVBSOUT:
        if (lParam == 1) {
            rk_fb_set_out_device(rk_fb_get_cvbsout_mode());
        } else {
            rk_fb_set_out_device(OUT_DEVICE_LCD);
        }
        /* wait for kernel changing display to CVBSOUT completely */
        usleep(500000);
        if (SetMode == MODE_PREVIEW) {
            videopreview_refresh(hWnd);
        } else if (SetMode == MODE_USBDIALOG) {
            InvalidateImg(hWndUSB, NULL, TRUE);
            break;
        }
        InvalidateImg(hMainWnd, NULL, TRUE);
        break;

    case MSG_REPAIR:
        if (parameter_get_video_lan() == 1)
            MessageBox(hWnd, "sd卡中有损坏视频，\n现已修复!", "警告!!!", MB_OK);
        else if (parameter_get_video_lan() == 0)
            MessageBox(hWnd, "Sd card is damaged in video, \nwe have repair!",
                       "Warning!!!", MB_OK);
        break;
    case MSG_FSINITFAIL:
        proc_MSG_FS_INITFAIL(hWnd, -1);
        break;
    case MSG_PHOTOEND:
        if (lParam == 1 && takephoto) {
            unloadingWaitBmp(hWnd);
            takephoto = false;
        }
        break;
    case MSG_SDMOUNTFAIL:
        proc_MSG_SDMOUNTFAIL(hWnd);
        break;
    case MSG_SDCARDFORMAT:
        unloadingWaitBmp(hWnd);
        if (lParam == EVENT_SDCARDFORMAT_FINISH) {
            printf("sd card format finish\n");
        } else if (lParam == EVENT_SDCARDFORMAT_FAIL) {
            int formatfail = MessageBox(hWnd, "sd卡格式话失败，是否重新格式化！", "警告!!!", MB_YESNO);
            if (formatfail == IDYES)
                cmd_IDM_RE_FORMAT(hWnd);
            else if (api_get_sd_mount_state() == SD_STATE_IN)
                cvr_sd_ctl(0);
        }
        break;
    case MSG_VIDEOPLAY:
        if (lParam == EVENT_VIDEOPLAY_EXIT) {
            exitplayvideo(hWnd);
            // InvalidateImg (hWnd, &msg_rcTime, TRUE);
            if (test_replay)
                startplayvideo(hWnd, testpath);
            if (parameter_get_debug_modechange() == 1) {
                //    modechange_pre_flage =1;
                modechange_time = MODECHANGE_TIME - 1;
            } else if (parameter_get_debug_modechange() == 0) {
                //        modechange_pre_flage=0;
                modechange_time = 0;
            }
            if (parameter_get_debug_video() == 1) {
                video_time = 1;
            } else if (parameter_get_debug_video() == 0) {
                video_time = 0;
            }
        } else if (lParam == EVENT_VIDEOPLAY_UPDATETIME) {
            video_play_state = 1;
            sec = (int)wParam;
            if (parameter_get_debug_modechange() == 1) {
                modechange_time = MODECHANGE_TIME - 1;
            } else if (parameter_get_debug_modechange() == 0) {
                modechange_time = 0;
            }
            if (parameter_get_debug_video() == 1) {
                video_time = 1;
            } else if (parameter_get_debug_video() == 0) {
                video_time = 0;
            }
            InvalidateImg(hWnd, &msg_rcTime, FALSE);
        }
        break;
    case MSG_VIDEOREC:
        if (lParam == EVENT_VIDEOREC_UPDATETIME) {
            sec = (int)wParam;
            InvalidateImg(hWnd, &msg_rcTime, FALSE);
            InvalidateImg(hWnd, &msg_rcRecimg, FALSE);
        }
        break;
    case MSG_RIL_RECV_MESSAGE:
        if (parameter_get_video_lan() == 1)
            MessageBox(hMainWnd, (char *)wParam, "收到短信", MB_OK);
        else
            MessageBox(hMainWnd, (char *)wParam, "Recv Message", MB_OK);
        break;
    case MSG_CAMERA: {
        // printf("%s wParam = %s, lParam = %d\n", __func__, wParam, lParam);
        if (SetMode != MODE_RECORDING && SetMode != MODE_PHOTO)
            break;
        if (lParam == 1) {
            struct video_param *front = parameter_get_video_frontcamera_reso();
            struct video_param *back = parameter_get_video_backcamera_reso();
            struct video_param *cif = parameter_get_video_cifcamera_reso();

            usleep(200000);
            video_record_addvideo(wParam, front, back, cif, api_video_get_record_state() == VIDEO_STATE_RECORD ? 1 : 0, 1);
        } else {
            if (wParam == uvc_get_user_video_id()) {
                uvc_gadget_pthread_exit();
                android_usb_config_uvc();
            } else
                video_record_deletevideo(wParam);
        }
        break;
    }
    case MSG_COLLISION: {
        if (SetMode == MODE_RECORDING && api_get_sd_mount_state() == SD_STATE_IN)
            video_record_savefile();
        break;
    }
    case MSG_FILTER: {
        // filterForUI= (int)lParam;
        break;
    }

    case MSG_USBCHAGE:
        proc_MSG_USBCHAGE(hWnd, message, wParam, lParam);
        break;
    case MSG_SDCHANGE:
        printf("MSG_SDCHANGE sdcard = %d\n", api_get_sd_mount_state());
        if (SetMode == MODE_RECORDING) {
            if (api_get_sd_mount_state() == SD_STATE_IN) {
                if (parameter_get_video_autorec())
                    api_start_rec();
            }// else {
            //api_stop_rec();
            //}
            InvalidateImg(hWnd, &msg_rcSD, FALSE);
            InvalidateImg(hWnd, &msg_rcSDCAP, FALSE);
        } else if (SetMode == MODE_EXPLORER) {
            if (explorer_path == 0)
                explorer_path = calloc(1, 256);
            sprintf(explorer_path, "%s", SDCARD_PATH);
            explorer_update(explorer_wnd);
            InvalidateImg(hWnd, &msg_rcSD, FALSE);
            InvalidateImg(hWnd, &msg_rcSDCAP, FALSE);
        } else if (SetMode == MODE_PLAY) {
            //videoplay_exit();
        } else if (SetMode == MODE_PREVIEW) {
            if (api_get_sd_mount_state() == SD_STATE_IN) {
                entervideopreview();
                InvalidateImg(hWnd, NULL, FALSE);
            } else {
                exitvideopreview();
                InvalidateImg(hWnd, NULL, FALSE);
            }
        } else if (SetMode == MODE_PHOTO) {
            InvalidateImg(hWnd, &msg_rcSD, FALSE);
            InvalidateImg(hWnd, &msg_rcSDCAP, FALSE);
        }
        break;

    case MSG_BATTERY:
        cap = lParam;
        if (cap > 100)
            battery = 0;
        else if ((cap >= 0) && (cap < (25 - BATTERY_CUSHION)))
            battery = 1;
        else if ((cap >= (25 + BATTERY_CUSHION)) && (cap < (50 - BATTERY_CUSHION)))
            battery = 2;
        else if ((cap >= (50 + BATTERY_CUSHION)) && (cap < (75 - BATTERY_CUSHION)))
            battery = 3;
        else if (cap >= (75 + BATTERY_CUSHION))
            battery = 4;
        else {
            if ((battery != 0) && (last_battery != 0))
                battery = last_battery;
            if (battery == 0) {
                if ((cap >= (25 - BATTERY_CUSHION)) && (cap < (25 + BATTERY_CUSHION)))
                    battery  = 1;
                if ((cap >= (50 - BATTERY_CUSHION)) && (cap < (50 + BATTERY_CUSHION)))
                    battery  = 2;
                if ((cap >= (75 - BATTERY_CUSHION)) && (cap < (75 + BATTERY_CUSHION)))
                    battery  = 3;
            }
        }
        last_battery = battery;

        //printf("battery level=%d\n", battery);
        InvalidateImg(hWnd, &msg_rcBtt, FALSE);
        break;

    case MSG_TIMER:
        /* gparking_gs_active (0: gsensor negative 1: gsneosr active) */
        if (SetMode == MODE_RECORDING) {
            parking_monitor_mode_switch(hWnd);
        }

        api_watchdog_keep_alive();

        if (SetMode != MODE_PLAY) {
            if (screenoff_time > 0) {
                screenoff_time--;
                if (screenoff_time == 0) {
                    rkfb_screen_off();
                }
            }
        }
        if (SetMode < MODE_PLAY) {
            InvalidateImg(hWnd, &msg_rcWifi, FALSE);
            InvalidateImg(hWnd, &msg_rcSDCAP, FALSE);
            InvalidateImg (hWnd, &msg_rcWatermarkTime, FALSE);
            InvalidateImg (hWnd, &msg_rcWatermarkLicn, FALSE);
            InvalidateImg (hWnd, &msg_rcWatermarkImg, FALSE);

            if(parameter_get_gps_mark() && parameter_get_gps_watermark())
                gps_ui_control(hWnd, FALSE);
        }
        if (SetMode == MODE_RECORDING) {
            video_record_fps_count();
        }

        ui_timer_debug_process(hWnd);
        android_adb_keeplive();
        break;
    case MSG_CAR_REVERSE:
        rk_fb_set_disphold(0);
        break;
    case MSG_VEHICLE_EXIT:
        if ((int)lParam == 1)
            rk_fb_set_disphold(1);
        else
            rk_fb_set_disphold(0);
        InvalidateRect(hMainWnd, NULL, TRUE);
        break;
    case MSG_PAINT:
        hdc = BeginPaint(hWnd);

        if (SetMode == MODE_USBCONNECTION) {
            FillBoxWithBitmap(hdc, USB_IMG_X, USB_IMG_Y, USB_IMG_W, USB_IMG_H,
                              &usb_bmap);
        } else {
            if (SetMode < MODE_PLAY) {
                char tf_cap[20];
                long long tf_free;
                long long tf_total;
                FillBoxWithBitmap(hdc, TOPBK_IMG_X, TOPBK_IMG_Y, g_rcScr.right,
                                  TOPBK_IMG_H, &topbk_bmap);
                FillBoxWithBitmap(hdc, BATT_IMG_X, BATT_IMG_Y, BATT_IMG_W, BATT_IMG_H,
                                  &batt_bmap[battery]);
                FillBoxWithBitmap(hdc, TF_IMG_X, TF_IMG_Y, TF_IMG_W, TF_IMG_H,
                                  &tf_bmap[api_get_sd_mount_state() == SD_STATE_IN ? 1 : 0]);
                FillBoxWithBitmap(hdc, MODE_IMG_X, MODE_IMG_Y, MODE_IMG_W, MODE_IMG_H,
                                  &mode_bmap[SetMode]);
                SetBkColor(hdc, GetWindowElementPixel(hMainWnd, WE_BGC_TOOLTIP));
                SetTextColor(hdc, PIXEL_lightwhite);
                SelectFont(hdc, (PLOGFONT)GetWindowElementAttr(hWnd, WE_FONT_MENU));
                if (api_get_sd_mount_state() == SD_STATE_IN) {
                    fs_manage_getsdcardcapacity(&tf_free, &tf_total);
                    snprintf(tf_cap, sizeof(tf_cap), "%4.1fG/%4.1fG", (float)tf_free / 1024,
                             (float)tf_total / 1024);
                    TextOut(hdc, TFCAP_STR_X, TFCAP_STR_Y, tf_cap);
                }
#ifndef USE_DISP_TS
                PaintADAS(hdc);
#endif
                {
                    char systime[128] = {0};
                    time_t ltime;
                    struct tm* today;
                    time(&ltime);
                    today = localtime(&ltime);
                    snprintf(systime, sizeof(systime), "%04d-%02d-%02d %02d:%02d:%02d\n",
                             1900 + today->tm_year, today->tm_mon + 1, today->tm_mday,
                             today->tm_hour, today->tm_min, today->tm_sec);

                    SetTextColor(hdc, PIXEL_lightwhite);
                    SetBkColor (hdc, bkcolor);
                    TextOut (hdc, WATERMARK_TIME_X, WATERMARK_TIME_Y, systime);

#ifdef USE_WATERMARK
                    if (((SetMode == MODE_RECORDING) || (SetMode == MODE_PHOTO)) &&
                        parameter_get_video_mark()) {
                        uint32_t show_time[MAX_TIME_LEN] = { 0 };
                        watermark_get_show_time(show_time, today, MAX_TIME_LEN);
                        video_record_update_time(show_time, MAX_TIME_LEN);
                        FillBoxWithBitmap(hdc, WATERMARK_IMG_X, WATERMARK_TIME_Y + WATERMARK_TIME_H, WATERMARK_IMG_W,
                                          WATERMARK_IMG_H, (BITMAP*)&watermark_bmap[0]);

                        /* Show license plate */
                        if (parameter_get_licence_plate_flag()) {
                            if (licenseplate_str[0] == 0) {
                                watermart_get_licenseplate_str((char *)parameter_get_licence_plate(), MAX_LICN_NUM, licenseplate_str);
                                get_licenseplate_and_pos((char *)licenseplate, sizeof(licenseplate), licplate_pos);
                            }

                            if (is_licenseplate_valid((char *)parameter_get_licence_plate()))
                                TextOut(hdc, WATERMARK_LICN_X, WATERMARK_LICN_Y, licenseplate_str);
                            FillBoxWithBitmap(hdc, WATERMARK_IMG_X, WATERMARK_IMG_Y, WATERMARK_IMG_W,
                                              WATERMARK_IMG_H, (BITMAP*)&watermark_bmap[0]);
                        }
                    }
#endif
                }

                if(parameter_get_gps_mark() && parameter_get_gps_watermark())
                    gps_info_show(hdc);
            }

            if (SetMode == MODE_RECORDING) {
                FillBoxWithBitmap(hdc, MOVE_IMG_X, MOVE_IMG_Y, MOVE_IMG_W, MOVE_IMG_H,
                                  &move_bmap[parameter_get_video_de()]);
                FillBoxWithBitmap(hdc, MIC_IMG_X, MIC_IMG_Y, MIC_IMG_W, MIC_IMG_H,
                                  &mic_bmap[parameter_get_video_audio()]);
                if (parameter_get_wifi_en() == 1) {
                    FillBoxWithBitmap(hdc, WIFI_IMG_X, WIFI_IMG_Y, WIFI_IMG_W, WIFI_IMG_H,
                                      &wifi_bmap[4]);
                } else {
                    SetBrushColor(hdc, bkcolor);
                    SetBkColor(hdc, bkcolor);
                    FillBox(hdc, WIFI_IMG_X, WIFI_IMG_Y, WIFI_IMG_W, WIFI_IMG_H);
                }
#if 0
                {
                    char ab[3][3] = {"A", "B", "AB"};
                    char resolution[3][6] = {"720P", "1080P", "1440P"};
                    SetBkColor(hdc, GetWindowElementPixel(hMainWnd, WE_BGC_TOOLTIP));
                    SetTextColor(hdc, PIXEL_lightwhite);
                    SelectFont(hdc, (PLOGFONT)GetWindowElementAttr(hWnd, WE_FONT_MENU));
                    TextOut(hdc, A_IMG_X, A_IMG_Y, ab[parameter_get_abmode()]);
                    TextOut(hdc, RESOLUTION_IMG_X, RESOLUTION_IMG_Y, resolution[parameter_get_vcamresolution()]);
                }
#else
                FillBoxWithBitmap(hdc, A_IMG_X, A_IMG_Y, A_IMG_W, A_IMG_H,
                                  &A_bmap[parameter_get_abmode()]);
                FillBoxWithBitmap(hdc, RESOLUTION_IMG_X, RESOLUTION_IMG_Y,
                                  RESOLUTION_IMG_W, RESOLUTION_IMG_H,
                                  &resolution_bmap[parameter_get_vcamresolution()]);
#endif
                parking_ui_display(hdc);

            } else if (SetMode == MODE_PLAY) {
                SetBrushColor(hdc, bkcolor);
                FillBox(hdc, g_rcScr.left, g_rcScr.top, g_rcScr.right - g_rcScr.left,
                        g_rcScr.bottom - g_rcScr.top);
            } else if (SetMode == MODE_PREVIEW) {
                int x = g_rcScr.left;
                int y = g_rcScr.top + TOPBK_IMG_H;
                int w = g_rcScr.right - g_rcScr.left;
                int h = g_rcScr.bottom - y;
                SetBrushColor(hdc, bkcolor);
                SetBkColor(hdc, bkcolor);
                FillBox(hdc, x, y, w, h);
                TextOut(hdc, FILENAME_STR_X, FILENAME_STR_Y, previewname);
                if (preview_isvideo)
                    FillBoxWithBitmap(hdc, (w - PLAY_IMG_W) / 2,
                                      (h - PLAY_IMG_H) / 2 + TOPBK_IMG_H, PLAY_IMG_W,
                                      PLAY_IMG_H, &play_bmap);
            }

            if ((api_video_get_record_state() == VIDEO_STATE_RECORD) || video_play_state) {
                char strtime[20];

                FillBoxWithBitmap(hdc, REC_IMG_X, REC_IMG_Y, REC_IMG_W, REC_IMG_H,
                                  &recimg_bmap);
                snprintf(strtime, sizeof(strtime), "%02ld:%02ld:%02ld", (long int)(sec / 3600),
                         (long int)((sec / 60) % 60), (long int)(sec % 60));
                collision_rec_time = (long int)(sec % 60);
                SetBkColor(hdc, bkcolor);
                SetTextColor(hdc, PIXEL_lightwhite);
                SelectFont(hdc, (PLOGFONT)GetWindowElementAttr(hWnd, WE_FONT_MENU));
                TextOut(hdc, REC_TIME_X, REC_TIME_Y, strtime);
            }
        }
        EndPaint(hWnd, hdc);
        break;

        //处理MSG_COMMAND消息,处理各个菜单明令
    case MSG_COMMAND:
        commandEvent(hWnd, wParam, lParam);
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
    case MSG_KEYDOWN: {
        if (key_lock)
            break;
        printf("%s MSG_KEYDOWN SetMode = %d, key = %d\n", __func__, SetMode,
               wParam);

        if (SetMode == MODE_RECORDING && parameter_get_video_adas()
            && (adasFlag & ADASFLAG_SETTING)) {
            keyprocess_adassetting(wParam);
            /* update paint */
            InvalidateImg(hWnd, &adas_rc, FALSE);
            break;
        }

#ifdef USE_KEY_STOP_USB_DIS_SHUTDOWN
        //key stop usb disconnect shutdown
        if ((wParam != 116) && (gshutdown != 0)) {
            stop_usb_disconnect_poweroff();
        }
#endif
        if (SetMode == MODE_PREVIEW) {
            switch (wParam) {
            case SCANCODE_TAB: {
                changemode(hWnd, MODE_RECORDING);
                break;
            }
            case SCANCODE_CURSORBLOCKUP:
            case SCANCODE_CURSORBLOCKRIGHT: {
                videopreview_next(hWnd);
                break;
            }
            case SCANCODE_CURSORBLOCKDOWN:
            case SCANCODE_CURSORBLOCKLEFT: {
                videopreview_pre(hWnd);
                break;
            }
            case SCANCODE_ENTER: {
                if (videoplay != 0) {
                    //    printf("videoplay==-1\n");
                    if (parameter_get_video_lan() == 1)
                        MessageBox(hWnd, "视频错误!!!", "警告!!!", MB_OK);
                    else if (parameter_get_video_lan() == 0)
                        MessageBox(hWnd, "video error !!!", "Warning!!!", MB_OK);
                } else {
                    videopreview_play(hWnd);
                }
                break;
            }
            case SCANCODE_MENU: {
                videopreview_delete(hWnd);
                break;
            }
            }
        } else if (SetMode == MODE_USBDIALOG) {
        } else if (SetMode == MODE_PLAY) {
            switch (wParam) {
            case SCANCODE_TAB:
            case SCANCODE_Q:
            case SCANCODE_ESCAPE:
                test_replay = 0;
                videoplay_exit();
                break;
            case SCANCODE_SPACE:
            case SCANCODE_ENTER:
                // do pause
                videoplay_pause();
                break;
            case SCANCODE_S:
                videoplay_step_play();
                break;
            case SCANCODE_CURSORBLOCKLEFT:
                videoplay_seek_time(-5.0);
                break;
            case SCANCODE_CURSORBLOCKRIGHT:
                videoplay_seek_time(5.0);
                break;
            case SCANCODE_CURSORBLOCKUP:
                videoplay_set_speed(1);
                break;
            case SCANCODE_CURSORBLOCKDOWN:
                videoplay_set_speed(-1);
                break;
            default:
                printf("scancode: %d\n", wParam);
            }
        } else if (SetMode == MODE_EXPLORER) {
            switch (wParam) {
            case SCANCODE_A:
            case SCANCODE_ENTER:
                explorer_enter(hWnd);
                break;
            case SCANCODE_TAB: {
                changemode(hWnd, MODE_RECORDING);
                break;
            }
            case SCANCODE_Q:
                explorer_back(hWnd);
                break;
            case SCANCODE_D:
                explorer_filedelete(hWnd);
                break;
            }
        } else if (SetMode == MODE_RECORDING) {
            switch (wParam) {
            case SCANCODE_MENU:
            case SCANCODE_LEFTALT:
            case SCANCODE_RIGHTALT:
                printf("add by lqw:SCANCODE_RIGHTALT\n");
                //   CreateMainWindowMenu(hWnd, creatmenu());
                if (menucreated == 0) {
                    CreateMainWindowMenu(hWnd, creatmenu());
                    menucreated = 1;
                } else if ((video_record_get_front_resolution(frontcamera, 4) ==
                            0) ||
                           (video_record_get_back_resolution(backcamera, 4) == 0)) {
                    DestroyMainWindowMenu(hWnd);
                    CreateMainWindowMenu(hWnd, creatmenu());
                }
                if (api_video_get_record_state() == VIDEO_STATE_PREVIEW)
                    popmenu(hWnd);
                break;
            case SCANCODE_CURSORBLOCKDOWN:
                api_mic_onoff(parameter_get_video_audio() == 0 ? 1 : 0);
                break;
            case SCANCODE_CURSORBLOCKUP:
                if (api_video_get_record_state() == VIDEO_STATE_PREVIEW) {
                    if (api_get_sd_mount_state() == SD_STATE_IN) {
                        api_start_rec();
                    }
                } else {
                    api_stop_rec();
                }
                break;
            case SCANCODE_CURSORBLOCKRIGHT:
                proc_record_SCANCODE_CURSORBLOCKRIGHT(hWnd);
                break;
            case SCANCODE_TAB: {
                // changemode(hWnd, MODE_EXPLORER);
                changemode(hWnd, MODE_PHOTO);
                // changemode(hWnd, MODE_PREVIEW);
                break;
            }
            case SCANCODE_ENTER:
                ui_takephoto(hWnd);
                break;
            case SCANCODE_ESCAPE:
                video_record_display_switch();
                break;

            case SCANCODE_B:  // block video save
                video_record_blocknotify(BLOCK_PREV_NUM, BLOCK_LATER_NUM);
                video_record_savefile();
                printf("The envent SCANCODE_B\n");
                break;
            }
        } else if (SetMode == MODE_PHOTO) {
            switch (wParam) {
            case SCANCODE_MENU:
            case SCANCODE_RIGHTALT:
                if (api_video_get_record_state() == VIDEO_STATE_PREVIEW)
                    popmenu(hWnd);
                break;
            case SCANCODE_TAB:
                changemode(hWnd, MODE_PREVIEW);
                break;
            case SCANCODE_ENTER:
                ui_takephoto(hWnd);
                break;
            }
        }
    } break;

    case MSG_KEYLONGPRESS:
        if (wParam == SCANCODE_SHUTDOWN) {
            shutdown_deinit(hWnd);
        }
        break;
    case MSG_USB_DISCONNECT:
        if (wParam == SCANCODE_SHUTDOWN) {
            shutdown_usb_disconnect(hWnd);
        }
        break;
    case MSG_KEYALWAYSPRESS:
        break;

    case MSG_KEYUP_LONG:
        break;
    case MSG_KEYUP:
        switch (wParam) {
        case SCANCODE_SHUTDOWN:
#ifndef MAIN_APP_ENABLE_SCREEN_CAPTURE
            changemode(hWnd, MODE_SUSPEND);
#endif
            break;
        default:
            break;
        }
        break;
    case MSG_MAINWIN_KEYDOWN:
#ifdef MAIN_APP_ENABLE_SCREEN_CAPTURE
        if (wParam == SCANCODE_SHUTDOWN &&
            api_get_sd_mount_state() == SD_STATE_IN)
            take_screen_capture();
#endif
        if (SetMode != MODE_PLAY && SetMode != MODE_PREVIEW)
            audio_sync_play(keypress_sound_file);

        if (screenoff_time == 0) {
            screenoff_time = parameter_get_screenoff_time();
            rkfb_screen_on();
        } else if (screenoff_time > 0) {
            screenoff_time = parameter_get_screenoff_time();
        }
        debug_cleancount();
        break;
    case MSG_MAINWIN_KEYUP:
        break;
    case MSG_MAINWIN_KEYUP_LONG:
        break;
    case MSG_MAINWIN_KEYLONGPRESS:
        break;
    case MSG_MAINWIN_KEYALWAYSPRESS:
        break;
    }

    return DefaultMainWinProc(hWnd, message, wParam, lParam);
}

int loadres(void)
{
    int i;
    char img[128];
    char respath[] = "/usr/local/share/minigui/res/images/";

    char debug_img[2][20] = {"debug.bmp", "debug_sel.bmp"};
    char filetype_img[FILE_TYPE_MAX][20] = {"file_unknown.bmp", "file_jpeg.bmp",
                                            "file_video.bmp"
                                           };
    char play_img[] = "play.bmp";
    char back_img[] = "file_back.bmp";
    char usb_img[] = "usb.bmp";
    char folder_img[] = "folder.bmp";
    char mic_img[2][20] = {"mic_off.bmp", "mic_on.bmp"};
    char A_img[3][10] = {"a.bmp", "b.bmp", "ab.bmp"};
    char topbk_img[] = "top_bk.bmp";
    char rec_img[] = "rec.bmp";
    char timelapse_img[] = "timelapse.bmp";
    char timelapse_on_img[] = "timelapse_on.bmp";
    char motiondetect_img[] = "motiondetect.bmp";
    char motiondetect_on_img[] = "motiondetect_on.bmp";
    char watermark_img[7][30] = {"watermark.bmp", "watermark_240p.bmp", "watermark_360p.bmp", "watermark_480p.bmp",
                                 "watermark_720p.bmp", "watermark_1080p.bmp", "watermark_1440p.bmp"
                                };
    char batt_img[5][20] = {"battery_0.bmp", "battery_1.bmp", "battery_2.bmp",
                            "battery_3.bmp", "battery_4.bmp"
                           };
    char tf_img[2][20] = {"tf_out.bmp", "tf_in.bmp"};
    char mode_img[4][20] = {"icon_video.bmp", "icon_camera.bmp", "icon_play.bmp",
                            "icon_play.bmp"
                           };
    char resolution_img[2][30] = {"resolution_720p.bmp", "resolution_1080p.bmp"};
    char move_img[2][20] = {"move_0.bmp", "move_1.bmp"};
    char mp_img[2][20] = {"mp.bmp", "mp_sel.bmp"};
    char car_img[2][20] = {"car.bmp", "car_sel.bmp"};
    char time_img[2][20] = {"time.bmp", "time_sel.bmp"};
    char setting_img[2][20] = {"setting.bmp", "setting_sel.bmp"};
    char wifi_img[5][20] = {"wifi_0.bmp", "wifi_1.bmp", "wifi_2.bmp",
                            "wifi_3.bmp", "wifi_4.bmp"
                           };

    /*load wifi bmp*/
    for (i = 0; i < (sizeof(wifi_bmap) / sizeof(BITMAP)); i++) {
        snprintf(img, sizeof(img), "%s%s", respath, wifi_img[i]);
        if (LoadBitmap(HDC_SCREEN, &wifi_bmap[i], img))
            return -1;
    }

    // load debug
    for (i = 0; i < (sizeof(png_menu_debug) / sizeof(BITMAP)); i++) {
        snprintf(img, sizeof(img), "%s%s", respath, debug_img[i]);
        if (LoadBitmap(HDC_SCREEN, &png_menu_debug[i], img))
            return -1;
    }
    /*load filetype bmp*/
    for (i = 0; i < (sizeof(filetype_bmap) / sizeof(BITMAP)); i++) {
        snprintf(img, sizeof(img), "%s%s", respath, filetype_img[i]);
        if (LoadBitmap(HDC_SCREEN, &filetype_bmap[i], img))
            return -1;
    }

    /* load play bmp */
    snprintf(img, sizeof(img), "%s%s", respath, play_img);
    if (LoadBitmap(HDC_SCREEN, &play_bmap, img))
        return -1;

    /* load back bmp */
    snprintf(img, sizeof(img), "%s%s", respath, back_img);
    if (LoadBitmap(HDC_SCREEN, &back_bmap, img))
        return -1;

    // load usb bmp
    snprintf(img, sizeof(img), "%s%s", respath, usb_img);
    if (LoadBitmap(HDC_SCREEN, &usb_bmap, img))
        return -1;

    /* load folder bmp */
    snprintf(img, sizeof(img), "%s%s", respath, folder_img);
    if (LoadBitmap(HDC_SCREEN, &folder_bmap, img))
        return -1;

    /*load mic bmp*/
    for (i = 0; i < (sizeof(mic_bmap) / sizeof(BITMAP)); i++) {
        snprintf(img, sizeof(img), "%s%s", respath, mic_img[i]);
        if (LoadBitmap(HDC_SCREEN, &mic_bmap[i], img))
            return -1;
    }

    /*load a bmp*/
    for (i = 0; i < (sizeof(A_bmap) / sizeof(BITMAP)); i++) {
        snprintf(img, sizeof(img), "%s%s", respath, A_img[i]);
        if (LoadBitmap(HDC_SCREEN, &A_bmap[i], img))
            return -1;
    }

    /* load topbk bmp */
    snprintf(img, sizeof(img), "%s%s", respath, topbk_img);
    if (LoadBitmap(HDC_SCREEN, &topbk_bmap, img))
        return -1;

    /* load recimg bmp */
    snprintf(img, sizeof(img), "%s%s", respath, rec_img);
    if (LoadBitmap(HDC_SCREEN, &recimg_bmap, img))
        return -1;

    /* load timelapse png */
    snprintf(img, sizeof(img), "%s%s", respath, timelapse_img);
    if (LoadBitmap(HDC_SCREEN, &timelapse_bmap, img))
        return -1;

    /* load timelapse_on png */
    snprintf(img, sizeof(img), "%s%s", respath, timelapse_on_img);
    if (LoadBitmap(HDC_SCREEN, &timelapse_on_bmap, img))
        return -1;

    /* load motiondetect png */
    snprintf(img, sizeof(img), "%s%s", respath, motiondetect_img);
    if (LoadBitmap(HDC_SCREEN, &motiondetect_bmap, img))
        return -1;

    /* load motiondetect_on png */
    snprintf(img, sizeof(img), "%s%s", respath, motiondetect_on_img);
    if (LoadBitmap(HDC_SCREEN, &motiondetect_on_bmap, img))
        return -1;

    /* load watermark bmp */
    for (i = 0; i < ARRAY_SIZE(watermark_bmap); i++) {
        snprintf(img, sizeof(img), "%s%s", respath, watermark_img[i]);
        if (load_bitmap(&watermark_bmap[i], img)) {
            printf("load watermark bmp error, i = %d\n", i);
            return -1;
        }
    }

    /* load batt bmp */
    for (i = 0; i < (sizeof(batt_bmap) / sizeof(BITMAP)); i++) {
        snprintf(img, sizeof(img), "%s%s", respath, batt_img[i]);
        if (LoadBitmap(HDC_SCREEN, &batt_bmap[i], img))
            return -1;
    }

    /* load tf card bmp */
    for (i = 0; i < (sizeof(tf_bmap) / sizeof(BITMAP)); i++) {
        snprintf(img, sizeof(img), "%s%s", respath, tf_img[i]);
        if (LoadBitmap(HDC_SCREEN, &tf_bmap[i], img))
            return -1;
    }

    /* load mode bmp */
    for (i = 0; i < (sizeof(mode_bmap) / sizeof(BITMAP)); i++) {
        snprintf(img, sizeof(img), "%s%s", respath, mode_img[i]);
        if (LoadBitmap(HDC_SCREEN, &mode_bmap[i], img))
            return -1;
    }

    /* load resolution bmp */
    for (i = 0; i < (sizeof(resolution_bmap) / sizeof(BITMAP)); i++) {
        snprintf(img, sizeof(img), "%s%s", respath, resolution_img[i]);
        if (LoadBitmap(HDC_SCREEN, &resolution_bmap[i], img))
            return -1;
    }

    /* load move bmp */
    for (i = 0; i < (sizeof(move_bmap) / sizeof(BITMAP)); i++) {
        snprintf(img, sizeof(img), "%s%s", respath, move_img[i]);
        if (LoadBitmap(HDC_SCREEN, &move_bmap[i], img))
            return -1;
    }

    /* load mp bmp */
    for (i = 0; i < (sizeof(bmp_menu_mp) / sizeof(BITMAP)); i++) {
        snprintf(img, sizeof(img), "%s%s", respath, mp_img[i]);
        if (LoadBitmap(HDC_SCREEN, &bmp_menu_mp[i], img))
            return -1;
    }

    /* load car bmp */
    for (i = 0; i < (sizeof(bmp_menu_car) / sizeof(BITMAP)); i++) {
        snprintf(img, sizeof(img), "%s%s", respath, car_img[i]);
        if (LoadBitmap(HDC_SCREEN, &bmp_menu_car[i], img))
            return -1;
    }

    /* load time bmp */
    for (i = 0; i < (sizeof(bmp_menu_time) / sizeof(BITMAP)); i++) {
        snprintf(img, sizeof(img), "%s%s", respath, time_img[i]);
        if (LoadBitmap(HDC_SCREEN, &bmp_menu_time[i], img))
            return -1;
    }
    /* load setting bmp */
    for (i = 0; i < (sizeof(bmp_menu_setting) / sizeof(BITMAP)); i++) {
        snprintf(img, sizeof(img), "%s%s", respath, setting_img[i]);
        if (LoadBitmap(HDC_SCREEN, &bmp_menu_setting[i], img))
            return -1;
    }

    return 0;
}

void unloadres(void)
{
    int i;

    /* unload wifi bmp */
    for (i = 0; i < (sizeof(wifi_bmap) / sizeof(BITMAP)); i++) {
        UnloadBitmap(&wifi_bmap[i]);
    }

    /* unload filetype bmp */
    for (i = 0; i < (sizeof(filetype_bmap) / sizeof(BITMAP)); i++) {
        UnloadBitmap(&filetype_bmap[i]);
    }

    /* unload back bmp */
    UnloadBitmap(&play_bmap);

    /* unload back bmp */
    UnloadBitmap(&back_bmap);

    /* unload folder bmp */
    UnloadBitmap(&folder_bmap);

    /* unload topbk bmp */
    UnloadBitmap(&topbk_bmap);

    /* unload recimg bmp */
    UnloadBitmap(&recimg_bmap);

    /* unload timelapse bmp */
    UnloadBitmap(&timelapse_bmap);

    /* unload motiondetect bmp */
    UnloadBitmap(&motiondetect_bmap);

    /* unload timelapse_on bmp */
    UnloadBitmap(&timelapse_on_bmap);

    /* unload motiondetect_on bmp */
    UnloadBitmap(&motiondetect_on_bmap);

    /* unload watermark bmp */
    for (i = 0; i < ARRAY_SIZE(watermark_bmap); i++)
        unload_bitmap(&watermark_bmap[i]);

    // unload usb bmp
    UnloadBitmap(&usb_bmap);
    /* unload mic bmp */
    for (i = 0; i < (sizeof(mic_bmap) / sizeof(BITMAP)); i++) {
        UnloadBitmap(&mic_bmap[i]);
    }

    /* unload a bmp */
    for (i = 0; i < (sizeof(A_bmap) / sizeof(BITMAP)); i++) {
        UnloadBitmap(&A_bmap[i]);
    }

    /* unload batt bmp */
    for (i = 0; i < (sizeof(batt_bmap) / sizeof(BITMAP)); i++) {
        UnloadBitmap(&batt_bmap[i]);
    }

    /* unload tf card bmp */
    for (i = 0; i < (sizeof(tf_bmap) / sizeof(BITMAP)); i++) {
        UnloadBitmap(&tf_bmap[i]);
    }

    /* unload mode bmp */
    for (i = 0; i < (sizeof(mode_bmap) / sizeof(BITMAP)); i++) {
        UnloadBitmap(&mode_bmap[i]);
    }

    /* unload resolution bmp */
    for (i = 0; i < (sizeof(resolution_bmap) / sizeof(BITMAP)); i++) {
        UnloadBitmap(&resolution_bmap[i]);
    }

    /* unload move bmp */
    for (i = 0; i < (sizeof(move_bmap) / sizeof(BITMAP)); i++) {
        UnloadBitmap(&move_bmap[i]);
    }

    /* unload mp bmp */
    for (i = 0; i < (sizeof(bmp_menu_mp) / sizeof(BITMAP)); i++) {
        UnloadBitmap(&bmp_menu_mp[i]);
    }

    /*un load car bmp */
    for (i = 0; i < (sizeof(bmp_menu_car) / sizeof(BITMAP)); i++) {
        UnloadBitmap(&bmp_menu_car[i]);
    }

    /* unload time bmp */
    for (i = 0; i < (sizeof(bmp_menu_time) / sizeof(BITMAP)); i++) {
        UnloadBitmap(&bmp_menu_time[i]);
    }

    /* unload setting bmp */
    for (i = 0; i < (sizeof(bmp_menu_setting) / sizeof(BITMAP)); i++) {
        UnloadBitmap(&bmp_menu_setting[i]);
    }
}

void ui_deinit(void)
{
    KillTimer(hMainWnd, _ID_TIMER);
    DestroyAllControls(hMainWnd);
    DestroyMainWindow(hMainWnd);
    UnregisterMainWindow(hMainWnd);
    MainWindowThreadCleanup(hMainWnd);
    unloadres();
}

void ui_msg_manager_cb(void *msgData, void *msg0, void *msg1)
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
        printf("%s MSG_SDCARDFORMAT_START %d\n", __func__, (int)msg0);
        loadingWaitBmp(hMainWnd);
        break;
    case MSG_SDCARDFORMAT_NOTIFY:
        printf("%s MSG_SDCARDFORMAT_NOTIFY %d\n", __func__, (int)msg0);
        if (msg0 < 0)
            PostMessage(hMainWnd, MSG_SDCARDFORMAT, 0, EVENT_SDCARDFORMAT_FAIL);
        else
            PostMessage(hMainWnd, MSG_SDCARDFORMAT, 0, EVENT_SDCARDFORMAT_FINISH);
        break;
    case MSG_VIDEO_REC_START:
        sec = 0;
        InvalidateImg(hMainWnd, &msg_rcTime, TRUE);
        InvalidateImg(hMainWnd, &msg_rcRecimg, TRUE);

        InvalidateImg (hMainWnd, &msg_rcWatermarkTime, TRUE);
        InvalidateImg (hMainWnd, &msg_rcWatermarkLicn, TRUE);
        InvalidateImg (hMainWnd, &msg_rcWatermarkImg, TRUE);
        break;
    case MSG_VIDEO_REC_STOP:
        InvalidateImg(hMainWnd, &msg_rcTime, TRUE);
        InvalidateImg(hMainWnd, &msg_rcRecimg, TRUE);

        InvalidateImg(hMainWnd, &msg_rcWatermarkTime, TRUE);
        InvalidateImg(hMainWnd, &msg_rcWatermarkLicn, TRUE);
        InvalidateImg(hMainWnd, &msg_rcWatermarkImg, TRUE);
        break;
    case MSG_VIDEO_MIC_ONOFF:
        InvalidateImg(hMainWnd, &msg_rcMic, FALSE);
        break;
    case MSG_VIDEO_SET_ABMODE:
        InvalidateImg(hMainWnd, &msg_rcAB, FALSE);
        break;
    case MSG_VIDEO_SET_MOTION_DETE:
        InvalidateImg(hMainWnd, &msg_rcMove, FALSE);
        break;
    case MSG_VIDEO_MARK_ONOFF:
        InvalidateImg(hMainWnd, &msg_rcWatermarkTime, TRUE);
        InvalidateImg(hMainWnd, &msg_rcWatermarkLicn, TRUE);
        InvalidateImg(hMainWnd, &msg_rcWatermarkImg, TRUE);
        break;
    case MSG_VIDEO_ADAS_ONOFF:
        adasflagtooff = 1;
        InvalidateImg(hMainWnd, &adas_rc, TRUE);
        break;
    case MSG_VIDEO_UPDATETIME:
        PostMessage(hMainWnd, MSG_VIDEOREC, (WPARAM)msg0, EVENT_VIDEOREC_UPDATETIME);
        break;
    case MSG_ADAS_UPDATE:
        if (SetMode == MODE_RECORDING) {
            memcpy(&g_adas_process, (struct adas_process_output *)msg0,
                   sizeof(g_adas_process));
            adasFlag |= ADASFLAG_DRAW;
            InvalidateImg(hMainWnd, &adas_rc, FALSE);
        } else {
            memset(&g_adas_process, 0, sizeof(g_adas_process));
            adasFlag &= ~ADASFLAG_DRAW;
        }
        break;
    case MSG_VIDEO_PHOTO_END:
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
    case MSG_CVBSOUT_EVENT:
        SendMessage(hMainWnd, MSG_CVBSOUT, (WPARAM)msg0, (LPARAM)msg1);
        break;
    case MSG_POWEROFF:
        ui_deinit();
        break;
    case MSG_BATT_UPDATE_CAP:
        PostMessage(hMainWnd, MSG_BATTERY, (WPARAM)msg0, (LPARAM)msg1);
        break;
    case MSG_BATT_LOW:
        shutdown_deinit(hMainWnd);
        break;
    case MSG_BATT_DISCHARGE:
        PostMessage(hMainWnd, MSG_BATTERY, (WPARAM)msg0, (LPARAM)msg1);
        shutdown_usb_disconnect(hMainWnd);
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
        changemode_notify(hMainWnd, (int)msg0, (int)msg1);
        break;
    case MSG_VIDEO_FRONT_REC_RESOLUTION:
        InvalidateImg(hMainWnd, &msg_rcResolution, FALSE);
        break;
    case MSG_GPS_INFO:
        memset(&ggps_info, 0, sizeof(struct gps_info));
        memcpy(&ggps_info, msg0, sizeof(struct gps_info));
        break;

    case MSG_SET_PIP:
        if ((int)msg0)
            set_display_cvr_window();
        else
            set_display_main_window();
        break;

    case MSG_GPIOS_EVENT:
        if (!strcmp((char *)msg0, "car-reverse")) {
            PostMessage(hMainWnd, MSG_CAR_REVERSE, (WPARAM)msg0, (LPARAM)msg1);
        } else if (!strcmp((char *)msg0, "vehicle_exit")) {
            PostMessage(hMainWnd, MSG_VEHICLE_EXIT, (WPARAM)msg0, (LPARAM)msg1);
            printf("%s: vehicle_exit reverse %d\n", __func__, (int)msg1);
        }
        break;
    default:
        break;
    }
}

__attribute((constructor)) void before_main()
{
#ifdef MAIN_APP_ENABLE_DISP_HOLD
    rk_fb_set_disphold(1);
#endif
}

int MiniGUIMain(int argc, const char* argv[])
{
    MSG Msg;
    MAINWINCREATE CreateInfo;
    HDC sndHdc;
    struct color_key key;
    char collision_level = 0;
    char parkingrec = 0;

    printf("camera run\n");

    set_display_cvr_window();

    if (loadres())
        printf("loadres fail\n");

    pthread_mutex_init(&set_mutex, NULL);
    api_poweron_init(ui_msg_manager_cb);
    memset(&CreateInfo, 0, sizeof(MAINWINCREATE));
    CreateInfo.dwStyle = WS_VISIBLE | WS_HIDEMENUBAR | WS_WITHOUTCLOSEMENU;
    CreateInfo.dwExStyle = WS_EX_NONE | WS_EX_AUTOSECONDARYDC;
    CreateInfo.spCaption = "camera";
    CreateInfo.hCursor = GetSystemCursor(0);
    CreateInfo.hIcon = 0;
    CreateInfo.MainWindowProc = CameraWinProc;
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

    api_reg_ui_preview_callback(entervideopreview, exitvideopreview);

    gsensor_init();
    gsensor_use_interrupt(GSENSOR_INT2, GSENSOR_INT_STOP);

    if (parameter_get_gps_mark()) {
        if (api_gps_init() >= 0) {
            if(parameter_get_gps_watermark())
                gps_ui_control(hMainWnd, FALSE);
        } else {
            parameter_save_gps_mark(0);
        }
    }

    /* register collision get data function */
    collision_level = parameter_get_collision_level();
    collision_init();
    if (collision_level != 0)
        collision_register();
    collision_regeventcallback(collision_event_callback);

    /* register parkingrec get data function */
    /* register parkingrec get data function */
    gparking = parameter_get_parkingmonitor();
    parking_init();
    if (gparking != PARKINGMONITOR_OFF)
        parking_register();
    parking_regeventcallback(parking_event_callback);

    api_adas_init(WIN_W, WIN_H);

    rk_fb_set_lcd_backlight(parameter_get_video_backlt());
    rk_fb_set_flip(parameter_get_video_flip());

    screenoff_time = parameter_get_screenoff_time();

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

    if (rkfb_set_color_key(&key) == -1) {
        printf("rkfb_set_color_key err\n");
    }

    ShowWindow(hMainWnd, SW_SHOWNORMAL);

    initrec(hMainWnd);
#ifdef USE_SPEECHREC
    printf("vicent----------------------SPEECHREC_ETC_FILE_PATH----%s\n", SPEECHREC_ETC_FILE_PATH);
    speechrec_register();
    speechrec_reg_readevent_callback(speechrec_event_callback);
#endif

#ifdef USE_RIL_MOUDLE
    ril_register();
    ril_reg_readevent_callback(ril_event_callback);
#endif

#ifdef MAIN_APP_ENABLE_DISP_HOLD
    rk_fb_set_disphold(0);
#endif

    while (GetMessage(&Msg, hMainWnd)) {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }

#ifdef USE_RIL_MOUDLE
    ril_unregister();
#endif

#ifdef USE_SPEECHREC
    speechrec_unregister();
#endif
    api_poweroff_deinit();

    collision_unregister();
    parking_unregister();
    parkingrec = parameter_get_parkingmonitor();
    if (parkingrec != 0) {
        gsensor_enable(1);
        gsensor_use_interrupt(GSENSOR_INT2, GSENSOR_INT_START);
    }
    gsensor_release();

    if(parameter_get_gps_mark())
        api_gps_deinit();

    if(parameter_get_video_adas())
        api_adas_deinit();

    UnregisterMainWindow(hMainWnd);
    MainWindowThreadCleanup(hMainWnd);
    unloadres();

    printf("camera exit\n");
    return 0;
}

#ifdef _MGRM_THREADS
#include <minigui/dti.c>
#endif
