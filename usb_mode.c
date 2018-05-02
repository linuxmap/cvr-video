/**
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd
 * author: hogan.wang@rock-chips.com
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

#include "usb_mode.h"

#include <stdio.h>
#include "parameter.h"
#include "uvc_user.h"
#include "common.h"
#include "wifi_management.h"

#include "ueventmonitor/usb_sd_ctrl.h"

void cmd_IDM_USB(void)
{
    ui_deinit_uvc();

    parameter_save_video_usb(0);
    android_usb_config_ums();
    rndis_management_stop();
    printf("msc mode\n");
}

void cmd_IDM_ADB(void)
{
    ui_deinit_uvc();

    parameter_save_video_usb(1);
    android_usb_config_adb();
    rndis_management_stop();
    printf("adb mode\n");
}

void cmd_IDM_UVC(void)
{
    if (parameter_get_video_usb() == 2)
        return ;
    parameter_save_video_usb(2);
    ui_init_uvc();
    rndis_management_stop();
    printf("uvc mode\n");
}

void cmd_IDM_RNDIS(void)
{
    ui_deinit_uvc();
    parameter_save_video_usb(3);
    android_usb_config_rndis();
    rndis_management_start();
    printf("rndis mode\n");
}

void ui_deinit_uvc()
{
#if USE_USB_WEBCAM
    if (parameter_get_video_usb() == 2)
        uvc_gadget_pthread_exit();
#endif
}

int ui_init_uvc()
{
    int ret = 0;

#if USE_USB_WEBCAM
    if (parameter_get_video_usb() == 2)
        android_usb_config_uvc();
#endif
    return ret;
}

