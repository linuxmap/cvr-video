/**
 * Copyright (C) 2016 Fuzhou Rockchip Electronics Co., Ltd
 * author: Benjo Lei <benjo.lei@rock-chips.com>
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

#include "wifi_debug_interface.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "wifi_management.h"
#include "menu_commond.h"

#define STR_DEBUG_ARGSETTING    "CMD_DEBUG_ARGSETTING"

int debug_setting_get_cmd_resolve(int nfp, char *buffer)
{
    char buff[1024] = {0};
    int arg;

    strcat(buff, "CMD_ACK_GET_DEBUG_ARGSETTING");

    strcat(buff, "reboot:");
    arg = get_reboot_parameter();
    if (arg == 0)
        strcat(buff, "off");
    else
        strcat(buff, "on");

    strcat(buff, "recovery:");
    arg = get_recover_parameter();
    if (arg == 0)
        strcat(buff, "off");
    else
        strcat(buff, "on");

    strcat(buff, "standby:");
    arg = get_standby_parameter();
    if (arg == 0)
        strcat(buff, "off");
    else
        strcat(buff, "on");

    strcat(buff, "mode_change:");
    arg = get_mode_change_parameter();
    if (arg == 0)
        strcat(buff, "off");
    else
        strcat(buff, "on");

    strcat(buff, "debug_video:");
    arg = get_video_parameter();
    if (arg == 0)
        strcat(buff, "off");
    else
        strcat(buff, "on");

    strcat(buff, "begin_end_video:");
    arg = get_beg_end_video_parameter();
    if (arg == 0)
        strcat(buff, "off");
    else
        strcat(buff, "on");

    strcat(buff, "photo:");
    arg = get_photo_parameter();
    if (arg == 0)
        strcat(buff, "off");
    else
        strcat(buff, "on");

    strcat(buff, "temp_control:");
    arg = get_temp_parameter();
    if (arg == 0)
        strcat(buff, "off");
    else
        strcat(buff, "on");

    strcat(buff, "temp_video_bit_rate_per_pixel:");
    arg = get_video_bitrate_parameter();
    printf("get_video_bitrate_parameter = %d\n", arg);
    if (arg == 1)
        strcat(buff, "1");
    else if (arg == 2)
        strcat(buff, "2");
    else if (arg == 4)
        strcat(buff, "4");
    else if (arg == 6)
        strcat(buff, "6");
    else if (arg == 8)
        strcat(buff, "8");
    else if (arg == 10)
        strcat(buff, "10");
    else if (arg == 12)
        strcat(buff, "12");
    else
        strcat(buff, "error");

    printf("cmd = %s\n", buff);
    if (0 > write(nfp, buff, strlen(buff)))
        printf("write fail!\r\n");

    return 0;
}

const struct type_cmd_debug  cmd_debug_setting_tab[] = {
    {setting_func_reboot_ui,        "debug_reboot",},
    {setting_func_recover_ui,       "debug_recovery",},
    {setting_func_standby_ui,       "debug_standby",},
    {setting_func_modechange_ui,    "debug_mode_change",},
    {setting_func_video_ui,         "debug_video",},
    {setting_func_begin_end_video_ui, "debug_beg_end_video",},
    {setting_func_photo_ui,         "debug_photo",},
    {setting_func_temp_ui,          "debug_temp_control",},
};

int debug_setting_cmd_resolve(int nfd, char *cmdstr)
{
    int i = 0;
    char *cmd;
    int size;

    size = sizeof(cmd_debug_setting_tab) / sizeof(cmd_debug_setting_tab[0]);
    for (i = 0; i < size; i++) {
        if (strstr(cmdstr, cmd_debug_setting_tab[i].cmd) != 0 &&
            cmd_debug_setting_tab[i].func != NULL) {
            cmd = &cmdstr[sizeof(STR_DEBUG_ARGSETTING) +
                          strlen(cmd_debug_setting_tab[i].cmd)];
            printf("debug_setting buffer = %s\n %s = %s\n",
                   cmdstr, cmd_debug_setting_tab[i].cmd,
                   cmd);
            return cmd_debug_setting_tab[i].func(cmd);
        }
    }

    return -1;
}
