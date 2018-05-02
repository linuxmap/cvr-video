/**
 * Copyright (C) 2018 Fuzhou Rockchip Electronics Co., Ltd
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

#ifndef __UI_320X240_H__
#define __UI_320X240_H__

#define VIDEO_DISPLAY_ROTATE_ANGLE 0

#define WIN_W 320
#define WIN_H 240

#define A_IMG_X 120
#define A_IMG_Y 10
#define A_IMG_W 18
#define A_IMG_H 9

#define MOVE_IMG_X 353
#define MOVE_IMG_Y 8
#define MOVE_IMG_W 43
#define MOVE_IMG_H 44

#define USB_IMG_W 138
#define USB_IMG_H 121
#define USB_IMG_X ((WIN_W - USB_IMG_W) / 2)
#define USB_IMG_Y ((WIN_H - USB_IMG_H) / 2)

#define BATT_IMG_X (WIN_W - 30)
#define BATT_IMG_Y 10
#define BATT_IMG_W 24
#define BATT_IMG_H 11

#define TF_IMG_X (WIN_W - 60)
#define TF_IMG_Y 5
#define TF_IMG_W 21
#define TF_IMG_H 20

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

#define REC_TIME_X 40
#define REC_TIME_Y (WIN_H - 52)
#define REC_TIME_W 150
#define REC_TIME_H 50

#define REC_IMG_X 20
#define REC_IMG_Y (WIN_H - 50)
#define REC_IMG_W 14
#define REC_IMG_H 14

#define TIMELAPSE_IMG_X (WIN_W / 2 / 2 / 2 / 2 / 2 )
#define TIMELAPSE_IMG_Y (WIN_H / 2 - TIMELAPSE_IMG_H / 2 - WIN_H / 4)
#define TIMELAPSE_IMG_W 35
#define TIMELAPSE_IMG_H 30

#define MOTIONDETECT_IMG_X (WIN_W / 2 / 2 / 2 / 2 / 2 )
#define MOTIONDETECT_IMG_Y (WIN_H / 2 - MOTIONDETECT_IMG_H / 2 - WIN_H / 4)
#define MOTIONDETECT_IMG_W 35
#define MOTIONDETECT_IMG_H 30

#define MOTIONDETECT_ON_IMG_X (WIN_W / 2 / 2 / 2 / 2 / 2 )
#define MOTIONDETECT_ON_IMG_Y (WIN_H / 2 - MOTIONDETECT_ON_IMG_H / 2 - WIN_H / 4)
#define MOTIONDETECT_ON_IMG_W 35
#define MOTIONDETECT_ON_IMG_H 30

#define TIMELAPSE_ON_IMG_X (WIN_W / 2 / 2 / 2 / 2 / 2 )
#define TIMELAPSE_ON_IMG_Y (WIN_H / 2 - TIMELAPSE_ON_IMG_H / 2 - WIN_H / 4)
#define TIMELAPSE_ON_IMG_W 35
#define TIMELAPSE_ON_IMG_H 30

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

#define MIC_IMG_X (WIN_W - 50)
#define MIC_IMG_Y (WIN_H - 50)
#define MIC_IMG_W 17
#define MIC_IMG_H 19

#define TFCAP_STR_X (WIN_W - 155)
#define TFCAP_STR_Y 7
#define TFCAP_STR_W 100
#define TFCAP_STR_H 42

#define SYSTIME_STR_X (WIN_W - 165)
#define SYSTIME_STR_Y 35
#define SYSTIME_STR_W WATERMARK_TIME_W
#define SYSTIME_STR_H WATERMARK_TIME_H

#define FILENAME_STR_X 20
#define FILENAME_STR_Y (WIN_H - 50)
#define FILENAME_STR_W 400
#define FILENAME_STR_H 42

#define PLAY_IMG_W 32
#define PLAY_IMG_H 32

#define WIFI_IMG_X (WIN_W - 85)
#define WIFI_IMG_Y (WIN_H - 47)
#define WIFI_IMG_W 24
#define WIFI_IMG_H 17

#endif/* __UI_320X240_H__ */
