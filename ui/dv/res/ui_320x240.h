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

#ifndef __SDV_UI_UI_320X240_H__
#define __SDV_UI_UI_320X240_H__

#define VIDEO_DISPLAY_ROTATE_ANGLE 90

#define WIN_W   320
#define WIN_H   240

#define A_IMG_X 120
#define A_IMG_Y 10
#define A_IMG_W 18
#define A_IMG_H 9

#define adas_X  0
#define adas_Y  30
#define adas_W  WATERMARK_TIME_X
#define adas_H  WIN_H

#define MOVE_IMG_X  353
#define MOVE_IMG_Y  8
#define MOVE_IMG_W  43
#define MOVE_IMG_H  44

#define USB_IMG_W 138
#define USB_IMG_H 121
#define USB_IMG_X ((WIN_W - USB_IMG_W) / 2)
#define USB_IMG_Y ((WIN_H - USB_IMG_H) / 2)

#define CHARGING_IMG_W 120
#define CHARGING_IMG_H 186
#define CHARGING_IMG_X ((WIN_W - CHARGING_IMG_W) / 2)
#define CHARGING_IMG_Y ((WIN_H - CHARGING_IMG_H) / 2)

#define BATT_IMG_X (WIN_W - 30)
#define BATT_IMG_Y 10
#define BATT_IMG_W 22
#define BATT_IMG_H 22

#define MODE_IMG_X 0
#define MODE_IMG_Y 0
#define MODE_IMG_W 65
#define MODE_IMG_H 30

#define RESOLUTION_IMG_X 75
#define RESOLUTION_IMG_Y 10
#define RESOLUTION_IMG_W 40
#define RESOLUTION_IMG_H 11

#define TOPBK_IMG_X 0
#define TOPBK_IMG_Y 0
#define TOPBK_IMG_W 1
#define TOPBK_IMG_H 30

#define BOTTOMBK_IMG_X 0
#define BOTTOMBK_IMG_Y (WIN_H - BOTTOMBK_IMG_H)
#define BOTTOMBK_IMG_W 1
#define BOTTOMBK_IMG_H 30

#define REC_TIME_X 40
#define REC_TIME_Y (WIN_H - 50)
#define REC_TIME_W 200
#define REC_TIME_H 50

#define REC_IMG_X 20
#define REC_IMG_Y (WIN_H - 50)
#define REC_IMG_W 14
#define REC_IMG_H 14

#define DV_REC_IMG_X 120
#define DV_REC_IMG_Y WIN_H - 20
#define DV_REC_IMG_W 9
#define DV_REC_IMG_H 9

#define DV_REC_TIME_X 140
#define DV_REC_TIME_Y (WIN_H-24)
#define DV_REC_TIME_W 150
#define DV_REC_TIME_H 30

#define DV_MOVE_IMG_X 120
#define DV_MOVE_IMG_Y 0
#define DV_MOVE_IMG_W 65
#define DV_MOVE_IMG_H 30

#define DV_MIN_IMG_X 195
#define DV_MIN_IMG_Y 0
#define DV_MIN_IMG_W 65
#define DV_MIN_IMG_H 30

#define DVP_MIN_IMG_X 10
#define DVP_MIN_IMG_Y 10
#define DVP_MIN_IMG_W 30
#define DVP_MIN_IMG_H 30

#define DVP_M_X 45
#define DVP_M_Y 15

#define DVP_NUM_X 10
#define DVP_NUM_Y (WIN_H -27)

#define DV_SET_X 35
#define DV_SET_Y 7

#define DV_WIFI_SSID_X 145
#define DV_WIFI_SSID_Y 90

#define DV_WIFI_PASS_X 145
#define DV_WIFI_PASS_Y 120


#define DV_MIC_IMG_X 265
#define DV_MIC_IMG_Y 5
#define DV_MIC_IMG_W 17
#define DV_MIC_IMG_H 19

#define DV_BATT_IMG_X (WIN_W - DV_BATT_IMG_W - 6)
#define DV_BATT_IMG_Y ((TOPBK_IMG_H - DV_INDEX_SOUND_H) / 2)
#define DV_BATT_IMG_W 24
#define DV_BATT_IMG_H 24

#define DV_TIME_W WIN_W
#define DV_TIME_H 30
#define DV_TIME_X 0
#define DV_TIME_Y (WIN_H - DV_TIME_H)

#define DV_RESOLUTION_X 65
#define DV_RESOLUTION_Y ((TOPBK_IMG_H - DV_RESOLUTION_H) / 2)
#define DV_RESOLUTION_W 117
#define DV_RESOLUTION_H 16

#define DV_INDEX_SOUND_X (DV_BATT_IMG_X - 27)
#define DV_INDEX_SOUND_Y ((TOPBK_IMG_H - DV_INDEX_SOUND_H) / 2)
#define DV_INDEX_SOUND_W 24
#define DV_INDEX_SOUND_H 24

#define DV_TF_IMG_X (DV_INDEX_SOUND_X - 27)
#define DV_TF_IMG_Y ((TOPBK_IMG_H - DV_TF_IMG_H) / 2)
#define DV_TF_IMG_W 24
#define DV_TF_IMG_H 24

#define DV_VIEW_W 100
#define DV_VIEW_H 30
#define DV_VIEW_X (WIN_W/2 - 30)
#define DV_VIEW_Y 5

#define DV_VIEW_PAG_X (WIN_W - 40)
#define DV_VIEW_PAG_Y 5

#define DV_CAM_IMG_X 5
#define DV_CAM_IMG_Y ((TOPBK_IMG_H - DV_CAM_IMG_H) / 2)
#define DV_CAM_IMG_W 24
#define DV_CAM_IMG_H 24

#define DV_CAM_CONT_IMG_X 5
#define DV_CAM_CONT_IMG_Y ((TOPBK_IMG_H - DV_CAM_CONT_IMG_H) / 2)
#define DV_CAM_CONT_IMG_W 30
#define DV_CAM_CONT_IMG_H 24

#define DV_INDEX_S_VIDEO_W 24
#define DV_INDEX_S_VIDEO_H 24
#define DV_INDEX_S_VIDEO_X 5
#define DV_INDEX_S_VIDEO_Y ((TOPBK_IMG_H - DV_INDEX_S_VIDEO_H) / 2)

#define DV_ICON_INDEX_TIME_W 24
#define DV_ICON_INDEX_TIME_H 24
#define DV_ICON_INDEX_TIME_X 35
#define DV_ICON_INDEX_TIME_Y ((TOPBK_IMG_H - DV_ICON_INDEX_TIME_H) / 2)

#define DV_ICON_ANTI_SHAKE_W 24
#define DV_ICON_ANTI_SHAKE_H 24
#define DV_ICON_ANTI_SHAKE_X 35
#define DV_ICON_ANTI_SHAKE_Y ((TOPBK_IMG_H - DV_ICON_ANTI_SHAKE_H) / 2)

#define DV_ENTER_IMG_X 10
#define DV_ENTER_IMG_Y 5
#define DV_ENTER_IMG_W 20
#define DV_ENTER_IMG_H 20

#define DV_LIST_SET_IMG_X 2
#define DV_LIST_SET_IMG_Y 59
#define DV_LIST_SET_IMG_W 317
#define DV_LIST_SET_IMG_H 1

#define DV_LIST_BMP_IMG_X 12
#define DV_LIST_BMP_IMG_Y 35
#define DV_LIST_BMP_IMG_W 20
#define DV_LIST_BMP_IMG_H 20

#define DV_LIST_BMP_TX_X 45
#define DV_LIST_BMP_TX_Y 37

#define DV_LIST_BMP_ARR_X (WIN_W - 35)
#define DV_LIST_BMP_ARR_Y 35
#define DV_LIST_BMP_ARR_W 20
#define DV_LIST_BMP_ARR_H 20

#define DV_REC_X 0
#define DV_REC_Y 30
#define DV_REC_W WIN_W
#define DV_REC_H 30

#define DV_PLAY_SPEED_X 10
#define DV_PLAY_SPEED_Y 20
#define DV_PLAY_SPEED_W 30
#define DV_PLAY_SPEED_H 16

#define DV_DLG_X        45
#define DV_DLG_Y        60
#define DV_DLG_W        235
#define DV_DLG_H        120

#define DV_DLG_BN_X     30
#define DV_DLG_BN_Y     60
#define DV_DLG_BN_W     80
#define DV_DLG_BN_H     30

#define KEY_ENTER_BOX_W 220
#define KEY_ENTER_BOX_H 29

#define MIC_IMG_X (WIN_W - 50)
#define MIC_IMG_Y (WIN_H - 50)
#define MIC_IMG_W 17
#define MIC_IMG_H 19

#define TFCAP_STR_X (WIN_W - 130)
#define TFCAP_STR_Y 7
#define TFCAP_STR_W 100
#define TFCAP_STR_H 30

#define SYSTIME_STR_X (WIN_W - 165)
#define SYSTIME_STR_Y 35
#define SYSTIME_STR_W WATERMARK_TIME_W
#define SYSTIME_STR_H WATERMARK_TIME_H

#define FILENAME_STR_X 20
#define FILENAME_STR_Y (WIN_H - 50)
#define FILENAME_STR_W 400
#define FILENAME_STR_H 42

#define WIFI_IMG_X (WIN_W - 85)
#define WIFI_IMG_Y (WIN_H - 47)
#define WIFI_IMG_W 24
#define WIFI_IMG_H 17

#define LICN_WIN_X 0
#define LICN_WIN_Y TOPBK_IMG_H
#define LICN_WIN_W WIN_W
#define LICN_WIN_H WIN_H - TOPBK_IMG_H

#define LICN_STR_X 15
#define LICN_STR_Y 20
#define LICN_STR_W 295
#define LICN_STR_H 30

#define LICN_RECT_X 22
#define LICN_RECT_Y 80
#define LICN_RECT_W 20
#define LICN_RECT_H 20

#define LICN_BUTTON_OK_X 240
#define LICN_BUTTON_OK_Y 160
#define LICN_BUTTON_OK_W 50
#define LICN_BUTTON_OK_H 20

#define LICN_RECT_GAP 36

/* Start 16-byte alignment*/
#define WATERMARK_TIME_W 160
#define WATERMARK_TIME_H 16
#define WATERMARK_TIME_X ((WIN_W - WATERMARK_TIME_W) & 0xfffffff0) //160
#define WATERMARK_TIME_Y ((TOPBK_IMG_H + 15) & 0xfffffff0) //32

#define WATERMARK_IMG_W 48
#define WATERMARK_IMG_H 16
#define WATERMARK_IMG_X ((WIN_W - WATERMARK_IMG_W - 16) & 0xfffffff0) //224
#define WATERMARK_IMG_Y (WATERMARK_TIME_Y + WATERMARK_TIME_H) //48

#define WATERMARK_LICN_W  80
#define WATERMARK_LICN_H  16
#define WATERMARK_LICN_X  ((WIN_W - WATERMARK_LICN_W) & 0xfffffff0) //688
#define WATERMARK_LICN_Y  (WATERMARK_IMG_Y + WATERMARK_IMG_H) //80
/* End 16-byte alignment */

#endif /* __SDV_UI_UI_320X240_H__ */
