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

#ifndef __MSG_LIST_MANAGER_H__
#define __MSG_LIST_MANAGER_H__

enum msg_id {
    MSG_DEBUG_REBOOT = 0,
    MSG_DEBUG_RECOVER,
    MSG_DEBUG_AWAKE_1,
    MSG_DEBUG_STANDBY,
    MSG_DEBUG_MODE_CHANGE,
    MSG_DEBUG_VIDEO,
    MSG_DEBUG_MODE_BEGIN_END_VIDEO,
    MSG_DEBUG_PHOTO,
    MSG_DEBUG_TEMP,
    MSG_DEBUG_VIDEO_BIT_RATE,

    MSG_VIDEO_TIME_LENGTH,
    MSG_SET_3DNR,
    MSG_SET_WHITE_BALANCE,
    MSG_SET_BACKLIGHT,
    MSG_SET_PIP,
    MSG_SET_EXPOSURE,
    MSG_SET_SYS_TIME,
    MSG_VIDEO_FRONT_REC_RESOLUTION,
    MSG_VIDEO_BACK_REC_RESOLUTION,
    MSG_VIDEO_CIF_REC_RESOLUTION,
    MSG_SET_USB_MODE,
    MSG_SET_PHOTO_RESOLUTION,
    MSG_SET_FREQ,
    MSG_VIDEO_AUTO_RECODE,
    MSG_COLLISION_LEVEL,
    MSG_LEAVECARREC_ONOFF,
    MSG_VIDEO_QUAITY,
    MSG_VIDEO_IDC,
    MSG_VIDEO_FLIP,
    MSG_VIDEO_CVBSOUT,
    MSG_VIDEO_AUTOOFF_SCREEN,
    MSG_VIDEO_LICENSE_PLATE,
    MSG_SET_LANGUAGE,
    MSG_SDCARDFORMAT_START,
    MSG_SDCARDFORMAT_NOTIFY,
    MSG_VIDEO_REC_START,
    MSG_VIDEO_REC_STOP,
    MSG_VIDEO_MIC_ONOFF,
    MSG_VIDEO_SET_ABMODE,
    MSG_VIDEO_SET_MOTION_DETE,
    MSG_VIDEO_MARK_ONOFF,
    MSG_VIDEO_ADAS_ONOFF,
    MSG_SET_KEY_TONE,
    MSG_SET_CAR_MODE_ONOFF,
    MSG_VIDEO_DVS_ONOFF,
    MSG_SET_VIDEO_DVS_ONOFF,
    MSG_SET_VIDEO_DVS_CALIB,
    MSG_ACK_VIDEO_DVS_CALIB,
    MSG_MODE_CHANGE_VIDEO_NORMAL_NOTIFY,
    MSG_MODE_CHANGE_TIME_LAPSE_NOTIFY,
    MSG_MODE_CHANGE_PHOTO_NORMAL_NOTIFY,
    MSG_MODE_CHANGE_PHOTO_BURST_NOTIFY,
    MSG_TAKE_PHOTO_START,
    MSG_TAKE_PHOTO_WARNING,
    MSG_VIDEO_UPDATETIME,
    MSG_ADAS_UPDATE,
    MSG_VIDEO_PHOTO_END,
    MSG_SDCORD_MOUNT_FAIL,
    MSG_SDCORD_CHANGE,
    MSG_HDMI_EVENT,
    MSG_CVBSOUT_EVENT,
    MSG_POWEROFF,
    MSG_BATT_UPDATE_CAP,
    MSG_BATT_LOW,
    MSG_BATT_DISCHARGE,
    MSG_CAMERE_EVENT,
    MSG_USB_EVENT,
    MSG_USER_RECORD_RATE_CHANGE,
    MSG_USER_MDPROCESSOR,
    MSG_USER_MUXER,
    MSG_VIDEOPLAY_EXIT,
    MSG_VIDEOPLAY_UPDATETIME,
    MSG_FS_INITFAIL,
    MSG_FS_NOTIFY,
    MSG_MODE_CHANGE_NOTIFY,
    MSG_SET_DVS_NOTIFY,
    MSG_GPS_INFO,
    MSG_GPS_RAW_INFO,
    MSG_KEY_EVENT_INFO,
    MSG_GPIOS_EVENT,
    MSG_VIDEO_ADD_EVENT,
    MSG_VIDEO_DEL_EVENT,

};

typedef void (* fun_cb)(void *, void *, void *);

struct type_node {
    fun_cb callback;
    struct type_node *next;
};
struct type_node *init_list();
struct type_node *reg_entry(fun_cb cb);
int unreg_entry(fun_cb cb);
void deinit_list();
void dispatch_msg(void *msg, void *prama0, void *param1);

#endif /* __MSG_LIST_MANAGER_H__ */
