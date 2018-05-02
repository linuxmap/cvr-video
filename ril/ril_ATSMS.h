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

#ifndef  __ATSMS_H__
#define __ATSMS_H__

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define PRINTF_FUN_LINE  //printf("vicent--%s--%d\n",__FUNCTION__, __LINE__);

#define MAX_PHONE_NUMBER 20
#define MAX_SMS_LENGTH 400

#define TABLE_NUM 7445
#define TABLE_EN_ARRAY_POS (0xA6 - 0x21)

#define SMS_HANDFRRE 1
#define SMS_NOR 0
#define SMS_NOCHANG 2

#define CHARSET_ASCII   0
#define CHARSET_UCS2    1

#define  SMS_HZ_SIZE   70
#define  SMS_ASC_SIZE  160
#define CHAR2HEX(x)   ((((x)>'9')?((x)-'A'+10):((x)-'0'))&0xF)

#define PDU_SMS_LEN     640     /*   4*160  */

typedef struct gsm_sms {
    int     index;
    char    coded;
    char    head;
    short   status;
    int     length;
    int     date;
    int     time;
    char    address[MAX_PHONE_NUMBER + 1];
    char    center[MAX_PHONE_NUMBER + 1];
    char    msg[MAX_SMS_LENGTH + 4];
} gsm_sms_t;

typedef struct {
    gsm_sms_t       *sms;
    unsigned char   tp_head;
    unsigned char   tp_serial;
    unsigned char   tp_pid;
    unsigned char   tp_dcs;
    unsigned char   tp_vp;
    unsigned char   tp_udl;
    unsigned char   handfree;
    unsigned char   blank2;
    char            pdu[PDU_SMS_LEN + 8];
} raw_sms_t;

int at_sms_read(char *raw_pdu, char* msg_buf);

#endif
