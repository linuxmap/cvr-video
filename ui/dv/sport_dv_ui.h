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

#ifndef __SPORT_DV_UI_H__
#define __SPORT_DV_UI_H__

#define SetDate "Set Date"
#define SetLicense "Set License Plate"

#define KEY_DOWN_LIST_X 50
#define KEY_DOWN_LIST_Y 35
#define KEY_DOWN_LIST_W 220
#define KEY_DOWN_LIST_H 176

#define KEY_ENTER_LIST_X 50
#define KEY_ENTER_LIST_Y 35
#define KEY_ENTER_LIST_W 220
#define KEY_ENTER_LIST_H 170

#define KEY_SUBMENU_TEXT_X 50
#define KEY_SUBMENU_TEXT_Y 55

#define KEY_TEXT_1_X 100
#define KEY_TEXT_1_Y 90//OPEN CLOSE
#define KEY_TEXT_2_Y 67//light
#define KEY_TEXT_3_Y 37//lapse

#define INFO_DIALOG_X  60
#define INFO_DIALOG_Y  50
#define INFO_DIALOG_W  200
#define INFO_DIALOG_H  120
#define INFO_DIALOG_TITLE_H  24

#define ICONVIEW_ITEM_W  106
#define ICONVIEW_ITEM_H  104
#define ICONVIEW_TEXT_GAP  20

#define DV_ANIMATION_X  110
#define DV_ANIMATION_Y  70
#define DV_ANIMATION_W  98
#define DV_ANIMATION_H  98
#define IDC_ANIMATION   190

#define KEY_ENTER_BOX_W 220
#define KEY_ENTER_BOX_H 29

#define DV_PLAY_IMG_W 46
#define DV_PLAY_IMG_H 46
#define DV_STOP_IMG_W 46
#define DV_STOP_IMG_H 46

#define MSG_SDCHANGE (MSG_USER + 1)
#define MSG_BATTERY (MSG_USER + 2)
#define MSG_CAMERA (MSG_USER + 3)
#define MSG_VIDEOREC (MSG_USER + 4)
#define MSG_VIDEOPLAY (MSG_USER + 5)
#define MSG_SDCARDFORMAT (MSG_USER + 6)
#define MSG_USBCHAGE (MSG_USER + 7)
#define MSG_FILTER (MSG_USER + 8)
#define MSG_SDMOUNTFAIL (MSG_USER + 9)
#define MSG_SDNOTFIT (MSG_USER + 10)
#define MSG_ADAS (MSG_USER + 11)
#define MSG_PHOTOEND (MSG_USER + 13)
#define MSG_REPAIR (MSG_USER + 14)
#define MSG_FSINITFAIL (MSG_USER + 15)
#define MSG_HDMI (MSG_USER + 16)
#define MSG_USB_DISCONNECT (MSG_USER + 17)
#define MSG_RK_USER (MSG_USER + 30)
#define MSG_ATTACH_USER_MUXER (MSG_RK_USER + 0)
#define MSG_DETACH_USER_MUXER (MSG_RK_USER + 1)
#define MSG_ATTACH_USER_MDPROCESSOR (MSG_RK_USER + 2)
#define MSG_DETACH_USER_MDPROCESSOR (MSG_RK_USER + 3)
#define MSG_RECORD_RATE_CHANGE (MSG_RK_USER + 4)
#define MSG_COLLISION (MSG_RK_USER + 5)

#define REBOOT_TIME 10
#define RECOVERY_TIME 5
#define AWAKE_TIME 5
#define STANDBY_TIME 5
#define MODECHANGE_TIME 5
#define VIDEO_TIME 2
#define BEG_END_VIDEO_TIME 5
#define PHOTO_TIME 5
#define FWUPDATE_TEST_TIME 5

#define EVENT_VIDEOREC_UPDATETIME 1
#define EVENT_VIDEOPLAY_UPDATETIME 1
#define EVENT_VIDEOPLAY_EXIT 2

#define EVENT_SDCARDFORMAT_FAIL 0
#define EVENT_SDCARDFORMAT_FINISH 1

#define IDM_NEW 1
#define IDM_SAVE 3
#define IDM_SAVEAS 4
#define IDM_EXIT 5
#define _ID_TIMER 106
#define ID_TF_CAPACITY_TIMER 1000
#define ID_WARNING_DLG_TIMER 1001

#define IDL_DIR 106
#define IDL_FILE 110
#define IDC_PATH 120
#define IDC_LISTVIEW 121

#define IDM_ABOUT_MP 103
#define IDM_ABOUT_TIME 104
#define IDM_ABOUT_CAR 105
#define IDM_ABOUT_SETTING 106
#define IDM_QUIT 107
#define IDM_ABOUT_LINE1 108
#define IDM_ABOUT_LINE2 109
#define IDM_ABOUT_LINE3 110
#define IDM_ABOUT_LINE4 111
#define IDM_720P 112
#define IDM_1080P 113
#define IDM_1MIN 114
#define IDM_3MIN 115
#define IDM_5MIN 116
#define IDM_OFF 117
#define IDM_CAR1 118
#define IDM_CAR2 119
#define IDM_CAR3 120
#define IDM_bright 121
#define IDM_exposal 122
#define IDM_detect 123
#define IDM_mark 124
#define IDM_record 125

#define IDM_bright1 126
#define IDM_bright2 127
#define IDM_bright3 128
#define IDM_bright4 129
#define IDM_bright5 130

#define IDM_exposal1 131
#define IDM_exposal2 132
#define IDM_exposal3 133
#define IDM_exposal4 134
#define IDM_exposal5 135

#define IDM_detectOFF 136
#define IDM_detectON 137

#define IDM_markOFF 138
#define IDM_markON 139

#define IDM_recordOFF 140
#define IDM_recordON 141

#define IDM_ABOUT_AUTORECORD 142
#define IDM_ABOUT_LANGUAGE 143
#define IDM_ABOUT_FREQUENCY 144
#define IDM_ABOUT_AVIN 145
#define IDM_ABOUT_AUTOOFF 146
#define IDM_ABOUT_SAVERR 147
#define IDM_ABOUT_GSENSOR 148
#define IDM_ABOUT_PARKING 149
#define IDM_ABOUT_DATE_SET 150
#define IDM_ABOUT_FORMAT 151
#define IDM_ABOUT_DEF_SET 152
#define IDM_ABOUT_VERSION 153

#define IDM_autorecordOFF 154
#define IDM_autorecordON 155

#define IDM_langEN 156
#define IDM_langCN 157

#define IDM_50HZ 158
#define IDM_60HZ 159
#define IDM_MODE 160
#define IDM_VEDIO 163
#define IDM_PLAY 162
#define IDM_CAMERA 161

#define IDM_1M 164
#define IDM_2M 165
#define IDM_3M 166

#define IDM_DELETE 167
#define IDM_YES_DELETE 168
#define IDM_NO_DELETE 169

#define IDM_FILE 170

#define IDM_FORMAT 171
#define IDM_SETDATE 172
#define IDC_BOX4 173
#define IDC_HOUR 174
#define IDC_MINUTE 175
#define IDC_SECOND 176
#define IDL_DAXIA 177
#define IDL_YEAR 178
#define IDL_MONTH 179
#define IDL_DAY 180
#define IDL_SEC 181

#define IDC_PROMPT 182

#define IDUSB 183
#define USBBAT 184

#define IDM_3DNR 185
#define IDM_3DNROFF 186
#define IDM_3DNRON 187

#define IDM_LDW 188
#define IDM_LDWOFF 189
#define IDM_LDWON 190

#define IDM_BACKLIGHT 192
#define IDM_BACKLIGHT_L 193
#define IDM_BACKLIGHT_M 194
#define IDM_BACKLIGHT_H 195

#define IDC_SETMODE 196
#define IDM_USB 197
#define IDM_ADB 198
#define IDM_FWVER 199
#define IDM_RECOVERY 200

#define IDM_1M_ph 201
#define IDM_2M_ph 202
#define IDM_3M_ph 203
#define IDM_ABOUT_DEBUG 204

#define IDM_BOOT_OFF 205
#define IDM_RECOVERY_OFF 206
#define IDM_AWAKE_1_OFF 207
#define IDM_STANDBY_2_OFF 208
#define IDM_MODE_CHANGE_OFF 209
#define IDM_DEBUG_VIDEO_OFF 210
#define IDM_BEG_END_VIDEO_OFF 211
#define IDM_DEBUG_PHOTO_OFF 212

#define IDM_BOOT_ON 213
#define IDM_RECOVERY_ON 214
#define IDM_AWAKE_1_ON 215
#define IDM_STANDBY_2_ON 216
#define IDM_MODE_CHANGE_ON 217
#define IDM_DEBUG_VIDEO_ON 218
#define IDM_BEG_END_VIDEO_ON 219
#define IDM_DEBUG_PHOTO_ON 220

#define IDM_BOOT 221
#define IDM_RECOVERY_DEBUG 222
#define IDM_AWAKE_1 223
#define IDM_STANDBY_2 224
#define IDM_MODE_CHANGE 225
#define IDM_DEBUG_VIDEO 226
#define IDM_BEG_END_VIDEO 227
#define IDM_DEBUG_PHOTO 228

#define IDM_FONTCAMERA 229
#define IDM_BACKCAMERA 230
#define IDM_FONT_1 231
#define IDM_FONT_2 232
#define IDM_FONT_3 233
#define IDM_FONT_4 234
#define IDM_BACK_1 235
#define IDM_BACK_2 236
#define IDM_BACK_3 237
#define IDM_BACK_4 238

#define IDM_COLLISION 239
#define IDM_COLLISION_NO 240
#define IDM_COLLISION_L 241
#define IDM_COLLISION_M 242
#define IDM_COLLISION_H 243
#define IDM_LEAVEREC 244
#define IDM_LEAVEREC_ON 245
#define IDM_LEAVEREC_OFF 246
#define IDM_AUTOOFFSCREEN 247
#define IDM_AUTOOFFSCREENOFF 248
#define IDM_AUTOOFFSCREENON 249
#define IDM_VIDEO_QUALITY 250
#define IDM_VIDEO_QUALITY_H 251
#define IDM_VIDEO_QUALITY_M 252
#define IDM_VIDEO_QUALITY_L 253
#define IDM_LICENSEPLATE_WATERMARK 260
#define IDM_DEBUG_VIDEO_BIT_RATE 280

#define IDM_DEBUG_VIDEO_BIT_RATE1 (IDM_DEBUG_VIDEO_BIT_RATE + 1)
#define IDM_DEBUG_VIDEO_BIT_RATE2 (IDM_DEBUG_VIDEO_BIT_RATE1 + 1)
#define IDM_DEBUG_VIDEO_BIT_RATE4 (IDM_DEBUG_VIDEO_BIT_RATE2 + 2)
#define IDM_DEBUG_VIDEO_BIT_RATE5 (IDM_DEBUG_VIDEO_BIT_RATE4 + 1)
#define IDM_DEBUG_VIDEO_BIT_RATE6 (IDM_DEBUG_VIDEO_BIT_RATE5 + 1)
#define IDM_DEBUG_VIDEO_BIT_RATE8 (IDM_DEBUG_VIDEO_BIT_RATE6 + 2)
#define IDM_DEBUG_VIDEO_BIT_RATE_MAX IDM_DEBUG_VIDEO_BIT_RATE8
#define IDM_WIFI (IDM_DEBUG_VIDEO_BIT_RATE_MAX + 1)
#define IDM_WIFION (IDM_WIFI + 1)
#define IDM_WIFIOFF (IDM_WIFION + 1)

#define IDM_DEBUG_TEMP (IDM_WIFIOFF + 1)
#define IDM_DEBUG_TEMP_ON (IDM_DEBUG_TEMP + 1)
#define IDM_DEBUG_TEMP_OFF (IDM_DEBUG_TEMP_ON + 1)

#define IDM_IDC     (IDM_DEBUG_TEMP_OFF + 1)
#define IDM_IDCOFF  (IDM_IDC + 1)
#define IDM_IDCON   (IDM_IDCOFF + 1)

#define IDM_timelapse               (IDM_IDCON + 1)
#define IDM_TIME_LAPSE_OFF          (IDM_timelapse + 1)
#define IDM_TIME_LAPSE_INTERNAL_1s  (IDM_TIME_LAPSE_OFF + 1)
#define IDM_TIME_LAPSE_INTERNAL_5s  (IDM_TIME_LAPSE_INTERNAL_1s + 1)
#define IDM_TIME_LAPSE_INTERNAL_10s (IDM_TIME_LAPSE_INTERNAL_5s + 1)
#define IDM_TIME_LAPSE_INTERNAL_30s (IDM_TIME_LAPSE_INTERNAL_10s + 1)
#define IDM_TIME_LAPSE_INTERNAL_60s (IDM_TIME_LAPSE_INTERNAL_30s + 1)

#define FILE_TYPE_UNKNOW 0
#define FILE_TYPE_PIC 1
#define FILE_TYPE_VIDEO 2

#endif  /* __SPORT_DV_UI_H__ */
