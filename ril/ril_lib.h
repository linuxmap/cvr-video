/**
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd
 * Author: Vicent Chi <vicent.chi@rock-chips.com>
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

#ifndef __RIL_LIB_H_
#define __RIL_LIB_H_

#define L710
//#define L810

enum RIL_CMD {
    RIL_INIT = 0,
    RIL_GET_IP,
    RIL_GET_DNS,
    RIL_CALL_TEL,
    RIL_HAND_UP,
    RIL_ANSWER_TEL,
    RIL_SEND_MESSAGE_EN,
    RIL_SEND_MESSAGE_CH,
    RIL_READ_MESSAGE,
    RIL_CLOSE_NET,
    RIL_CTRL_Z,
    RIL_OTHER_CMD,
    RIL_END,
};

enum {
    EVENT_READ_RIL_NOTHING = 0,
    EVENT_READ_RIL_RING,
    EVENT_READ_RIL_NO_CARRIER,
    EVENT_READ_RIL_CALL,
    EVENT_READ_RIL_HANDUP,
    EVENT_READ_RIL_RECV_MESSAGE,
    EVENT_READ_RIL_READ_MESSAGE,

};

typedef struct CmdData {
    int cmd_type;
    int cmd_num;
    char at_cmd_ch[40][80];
} cmd_data;

typedef struct ListenEvent {
    int event_type;
    void* event_msg0;
    void* event_msg1;
} listen_event_data;

void open_hot_share();
void close_hot_share();
int send_listen_riL_cmd(int listen_fd, int cmd_type, char* amendata);
int port_exist();

#endif
