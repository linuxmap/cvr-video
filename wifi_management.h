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

#ifndef __WIFI_MENAGEMENT_H__
#define __WIFI_MENAGEMENT_H__

struct type_cmd_func {
    int (*func)(int nfd, char *str);
    char *cmd;
};

struct type_cmd_debug {
    int (*func)(char *str);
    char *cmd;
};

int runapp(char *cmd);
void rndis_management_start(void);
void rndis_management_stop(void);
void wifi_management_start(void);
void wifi_management_stop(void);
int get_g_wifi_is_running(void);
int get_live_client_num(void);
int get_link_client_num(void);
int get_tcp_server_socket(int num);
int tcp_pack_file_for_path(char *type, char *path, int form,
                           int filetype, char *buff);
#endif
