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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>

#include "ril_lib.h"
#include "ril_ATSMS.h"

#ifdef L710
#define ATPORT0 "/dev/ttyUSB0"
#define ATPORT1 "/dev/ttyUSB1"
#define ATPORT2 "/dev/ttyUSB2"
#else
#define ATPORT0 "/dev/ttyACM0"
#define ATPORT1 "/dev/ttyACM1"
#define ATPORT2 "/dev/ttyACM2"
#endif

#define BUFSIZE 1000
#define BAUDRATE B115200

char sms_msg_buf[MAX_SMS_LENGTH + 4];

#ifdef L710
static cmd_data at_cmd_table[RIL_END + 1] = {
    {
        RIL_INIT, 13,
        {
            "AT\r\n",
            "AT+GTRAT=6,3,2\r\n",
            "AT+CPIN?\r\n",
            "AT+CIMI\r\n",
            "AT+CSQ?\r\n",
            "AT+CFUN?\r\n",
            "AT+CFUN=1\r\n",
            "AT+COPS?\r\n",
            "AT+CGREG?\r\n",
            "AT+CEREG?\r\n",
            "AT+CGDCONT=1,\"IP\", \"3GNET\"\r\n",
            "AT+GTRNDIS=1,1\r\n",
            "AT+CNMI=2,1,0,0\r\n",
        }
    },
    {
        RIL_GET_IP, 1,
        {
            "AT+CGPADDR=1\r\n",
        }
    },
    {
        RIL_GET_DNS, 1,
        {
            "AT+XDNS?\r\n",
        }
    },
    {
        RIL_CALL_TEL, 1,
        {
            "ATD\r\n",
        }
    },
    {
        RIL_HAND_UP, 1,
        {
            "ATH\r\n",
        }
    },
    {
        RIL_ANSWER_TEL, 1,
        {
            "ATA\r\n",
        }
    },
    {
        RIL_OTHER_CMD, 1,
        {
            "AT\r\n",
        }
    },
    {
        RIL_SEND_MESSAGE_EN, 3,
        {
            "AT+CMGF=1\r\n",
            "AT+CMGS=\"+86telnum\"\r\n",
            "data+ctrl+z\r\n",
        }
    },
    {
        RIL_SEND_MESSAGE_CH, 3,
        {
            "AT+CMGF=0\r\n",
            "AT+CMGS=\"len\"\r\n",
            "pdudata+ctrl+z\r\n",
        }
    },
    {
        RIL_READ_MESSAGE, 1,
        {
            "AT+CMGR=posnum\r\n",
        }
    },
    {
        RIL_CLOSE_NET, 1,
        {
            "AT+CGACT=0,1\r\n",
        }
    },
    {
        RIL_CTRL_Z, 1,
        {
            "AT\r\n",
        }
    },
    {
        RIL_OTHER_CMD, 1,
        {
            "AT\r\n",
        }
    }
};

#else
static cmd_data at_cmd_table[RIL_END + 1] = {
    {
        RIL_INIT, 19,
        {
            "AT\r\n",
            "AT+GTRAT=6,3,2\r\n",
            "AT+CPIN?\r\n",
            "AT+CIMI\r\n",
            "AT+CSQ?\r\n",
            "AT+CFUN?\r\n",
            "AT+CFUN=1\r\n",
            "AT+COPS?\r\n",
            "AT+CGREG?\r\n",
            "AT+CEREG?\r\n",
            "AT+CGACT?\r\n",
            "AT+CGDCONT=1,\"IP\", \"3GNET\"\r\n",
            "AT+XDNS=1,1\r\n",
            "AT+CGACT=1,1\r\n",
            "AT+XDATACHANNEL=1,1,\"/USBCDC/0\",\"/USBHS/NCM/2\",0\r\n",
            "AT+CGDATA=\"M-RAW_IP\",1\r\n",
            "AT+CGEREP=1,0\r\n",
            "AT+CNMI=2,1,0,0\r\n",
            "AT+CGACT=1,1\r\n",
        }
    },
    {
        RIL_GET_IP, 1,
        {
            "AT+CGPADDR=1\r\n",
        }
    },
    {
        RIL_GET_DNS, 1,
        {
            "AT+XDNS?\r\n",
        }
    },
    {
        RIL_CALL_TEL, 1,
        {
            "ATD\r\n",
        }
    },
    {
        RIL_HAND_UP, 1,
        {
            "ATH\r\n",
        }
    },
    {
        RIL_ANSWER_TEL, 1,
        {
            "ATA\r\n",
        }
    },
    {
        RIL_OTHER_CMD, 1,
        {
            "AT\r\n",
        }
    },
    {
        RIL_SEND_MESSAGE_EN, 3,
        {
            "AT+CMGF=1\r\n",
            "AT+CMGS=\"+86telnum\"\r\n",
            "data+ctrl+z\r\n",
        }
    },
    {
        RIL_SEND_MESSAGE_CH, 3,
        {
            "AT+CMGF=0\r\n",
            "AT+CMGS=\"len\"\r\n",
            "pdudata+ctrl+z\r\n",
        }
    },
    {
        RIL_READ_MESSAGE, 1,
        {
            "AT+CMGR=posnum\r\n",
        }
    },
    {
        RIL_CLOSE_NET, 1,
        {
            "AT+CGACT=0,1\r\n",
        }
    },
    {
        RIL_CTRL_Z, 1,
        {
            "AT\r\n",
        }
    },
    {
        RIL_OTHER_CMD, 1,
        {
            "AT\r\n",
        }
    }
};
#endif

int port_exist()
{
    int ret = 0;
    ret += access(ATPORT0, 0);
    ret += access(ATPORT1, 0);
    ret += access(ATPORT2, 0);
    return (ret == 0);
}

int open_port(char *port, int flag, mode_t mode)
{
    int port_fd;
    struct termios options;

    port_fd = open(port, mode | O_NOCTTY | O_NDELAY); // O_RDONLY | O_WDONLY | O_RDWR
    if (port_fd == -1) {
        printf("%s: Unable to open the port - \r\n", __func__);
        return -1;
    } else {
        //fcntl(port_fd, F_SETFL, FNDELAY);
        //fcntl(port_fd, F_SETFL, 0);
        fcntl(port_fd, F_SETFL, flag);
        tcgetattr(port_fd, &options );
        cfsetispeed(&options, BAUDRATE );
        cfsetospeed(&options, BAUDRATE );
        options.c_cflag |= ( CLOCAL | CREAD);
        options.c_cflag &= ~(CSIZE | PARENB | CSTOPB | CSIZE);
        options.c_cflag |= CS8;
        options.c_cflag &= ~CRTSCTS;
        options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
        options.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL | INLCR | IGNCR);
        options.c_oflag &= ~OPOST;
        if (tcsetattr( port_fd, TCSANOW, &options ) == -1 ) {
            printf ("Error with tcsetattr = %s\r\n", strerror(errno));
            return -1;
        } else {
            printf ( "%s: Open port succeed\r\n", __func__);
        }
    }
    return port_fd;
}


int send_listen_riL_cmd(int listen_fd, int cmd_type, char* amendata)
{
    if (cmd_type == RIL_CALL_TEL) {
        sprintf(at_cmd_table[cmd_type].at_cmd_ch[0], "%s%s;\r\n", "ATD", amendata);
        printf("%s--send_riL_cmd: %s", __func__, at_cmd_table[cmd_type].at_cmd_ch[0]);
        write(listen_fd, at_cmd_table[cmd_type].at_cmd_ch[0] , \
              (strlen(at_cmd_table[cmd_type].at_cmd_ch[0]) + 1));
    } else {
        printf("%s--send_riL_cmd: %s", __func__, at_cmd_table[cmd_type].at_cmd_ch[0]);
        write(listen_fd, at_cmd_table[cmd_type].at_cmd_ch[0] , \
              (strlen(at_cmd_table[cmd_type].at_cmd_ch[0]) + 1));
    }

    return 0;
}

void open_hot_share()
{
    runapp("echo 1 > /proc/sys/net/ipv4/ip_forward");
//  runapp("xtables-multi iptables -A FORWARD -s 192.168.100.0/24 -j ACCEPT");
//  runapp("xtables-multi iptables -t nat -A POSTROUTING -s 192.168.100.0/24 -j MASQUERADE");
}

void close_hot_share()
{
//  runapp("xtables-multi iptables -F");
    runapp("echo 0 > /proc/sys/net/ipv4/ip_forward");
}

int amend_cmd_before_send(int cmd_type, char* amendata[])
{
    char strcmd_len[100];
    char strcmd_pdu[400];

    switch (cmd_type) {
    case RIL_CALL_TEL:
        sprintf(at_cmd_table[cmd_type].at_cmd_ch[0], "%s%s;\r\n", "ATD", amendata[0]);
        break;
    case RIL_SEND_MESSAGE_EN:
        sprintf(at_cmd_table[cmd_type].at_cmd_ch[1], "AT+CMGS=\"%s\"\r\n", amendata[0]);
        sprintf(at_cmd_table[cmd_type].at_cmd_ch[2], "%s%c\r\n", amendata[1], 0x1A);
        break;
    case RIL_SEND_MESSAGE_CH:
        AT_SmsSendTo(amendata[0], amendata[1], SMS_HANDFRRE, strcmd_len, strcmd_pdu);
        sprintf(at_cmd_table[cmd_type].at_cmd_ch[1], "%s\r\n", strcmd_len);
        sprintf(at_cmd_table[cmd_type].at_cmd_ch[2], "%s%c\r\n", strcmd_pdu, 0x1A);
        break;
    case RIL_READ_MESSAGE:
        sprintf(at_cmd_table[cmd_type].at_cmd_ch[0], "AT+CMGR=%s\r\n", amendata[0]);
        break;
    case RIL_OTHER_CMD:
        sprintf(at_cmd_table[cmd_type].at_cmd_ch[0], "%s\r\n", amendata[0]);
        break;
    case RIL_CTRL_Z:
        sprintf(at_cmd_table[cmd_type].at_cmd_ch[0], "%c\r\n", 0x1A);
        break;
    default:
        break;
    }
    return 0;
}

int amend_buf_after_send(int at_cmd_type, char* buf)
{
    char* ch_pos1;
    char* ch_pos2;
    char* tmpBuf;
    int len;
    char messbuff[MAX_SMS_LENGTH];

    switch (at_cmd_type) {
    case RIL_GET_IP:
#ifdef L710
        runapp("busybox udhcpc -i eth0");
#else
        ch_pos1 = strchr(buf, '\"');
        ch_pos2 = strrchr(buf, '\"');
        len = (int)ch_pos2 - (int)ch_pos1 - 1;
        memcpy(ipaddr, (ch_pos1 + 1), len);
        ipaddr[len] = '\0';
        if (len > 7) {
            sprintf(app_cmd, "ifconfig usb0 %s netmask 255.255.255.255 -arp", ipaddr);
            runapp(app_cmd);
            sprintf(app_cmd, "ip r add %s dev usb0", ipaddr);
            runapp(app_cmd);
            sprintf(app_cmd, "ip r add 0.0.0.0/0 via %s dev usb0", ipaddr);
            runapp(app_cmd);
        } else {
            printf("%s set ipaddr is error!\n", __func__);
            return -1;
        }
#endif
        break;
    case RIL_READ_MESSAGE:
        tmpBuf = strstr(buf, "+CMGR:");
        if (tmpBuf == NULL) {
            printf("%s read message tmpBuf is NULL 1!\n", __func__);
            return -1;
        }
        ch_pos1 = strchr(tmpBuf, '\n');
        if (ch_pos1 == NULL) {
            printf("%s read message chPos1 is NULL 2!\n", __func__);
            return -1;
        }
        ch_pos2 = strchr(ch_pos1 + 1, '\n');
        if (ch_pos2 == NULL) {
            printf("%s read message chPos2 is NULL 3!\n", __func__);
            return -1;
        }
        len = (int)ch_pos2 - (int)ch_pos1 - 1;
        if (len > 0 && len < MAX_SMS_LENGTH) {
            memcpy(messbuff, (ch_pos1 + 1), len);
            messbuff[len] = '\0';
            printf("%s messbuff = %s\n", __func__, messbuff);
        } else {
            printf("%s read message is error!\n", __func__);
            return -1;
        }
        at_sms_read(messbuff, buf);
        break;
    default:
        break;
    }
    return 0;
}

int send_riL_cmd_and_read(int cmd_type, char* amendata[])
{
    int cmd_fd;
    int i = 0, ret = 0;
    int retry_times = 0;
    char buf[BUFSIZE];

    cmd_fd = open_port(ATPORT2, FNDELAY, O_RDWR);

    amend_cmd_before_send(cmd_type, amendata);

    for (i = 0; i < at_cmd_table[cmd_type].cmd_num; i++) {
        if (!port_exist()) {
            if (cmd_fd > 0)
                close(cmd_fd);
            break;
        }
        memset(buf, 0, BUFSIZE);

        printf("%s send_riL_cmd: %s", __func__, at_cmd_table[cmd_type].at_cmd_ch[i]);

        write(cmd_fd, at_cmd_table[cmd_type].at_cmd_ch[i] , \
              (strlen(at_cmd_table[cmd_type].at_cmd_ch[i]) + 1));
        usleep(100 * 1000);
        read(cmd_fd, buf, BUFSIZE);

        printf("%s read_riL_buf:\n%s", __func__, buf);

        //need reopen port for ctrl cmd success
        if (cmd_type == RIL_INIT) {
            if ((NULL == strstr(buf, "OK") && NULL == strstr(buf, "CONNECT")) && retry_times < 3) {
                close(cmd_fd);
                sleep(1);
                cmd_fd = open_port(ATPORT2, FNDELAY, O_RDWR);
                sleep(1);
                printf("%s at_cmd_table[%d].at_cmd_ch[%d] need try again late!\n", \
                       __func__, cmd_type, i);
                i--;
                retry_times++;
            } else {
                retry_times = 0;
            }
        }
    }

    ret = amend_buf_after_send(cmd_type, amendata);
    if (ret < 0) {
        goto send_riL_cmd_and_read_exit;
    }
    close(cmd_fd);
    return 0;

send_riL_cmd_and_read_exit:
    close(cmd_fd);
    return -1;
}

int init_listen_ril()
{
    int listen_fd;
    char at_cmd[100] = "AT+CNMI=2,1,0,0\r\n";

    send_riL_cmd_and_read(RIL_INIT, NULL);
    send_riL_cmd_and_read(RIL_GET_IP, NULL);

    listen_fd = open_port(ATPORT0, 0, O_RDWR);
    write(listen_fd,  at_cmd, (strlen(at_cmd) + 1));

    return listen_fd;
}

int listen_ril_event_process(int listen_fd, listen_event_data* event_data)
{
    char* ch_pos1;
    char* ch_pos2;
    char mess_num[4];
    char cmd[20];
    char buf[BUFSIZE];
    int size, len;

    memset(buf, 0, BUFSIZE);
    size = read(listen_fd, buf, BUFSIZE);
    if (size > 0) {
        printf("%s: size:%d buf:\n%s", __func__, size, buf);

        if (NULL != strstr(buf, "RING")) {
            event_data->event_type = EVENT_READ_RIL_RING;
        } else if (NULL != strstr(buf, "NO CARRIER")) {
            event_data->event_type = EVENT_READ_RIL_NO_CARRIER;
        } else if (NULL != strstr(buf, "ATD")) {
            event_data->event_type = EVENT_READ_RIL_CALL;
        } else if (NULL != strstr(buf, "ATH")) {
            event_data->event_type = EVENT_READ_RIL_HANDUP;
        } else if (NULL != strstr(buf, "+CMTI:")) {
            ch_pos1 = strchr(buf, ',');
            ch_pos2 = strchr(ch_pos1, '\n');
            if (ch_pos1 != NULL && ch_pos2 != NULL) {
                len = (int)ch_pos2 - (int)ch_pos1 - 1;
                if (len > 0 && len < MAX_SMS_LENGTH) {
                    memcpy(mess_num, (ch_pos1 + 1), len);
                    mess_num[len] = '\0';
                    printf("%s messNum = %s\n", __func__, mess_num);
                    sprintf(cmd, "AT+CMGR=%s\r\n", mess_num);
                    printf("%s cmd %d = %s\n", __func__, (strlen(cmd) + 1), cmd);
                    write(listen_fd, cmd, (strlen(cmd) + 1));
                    event_data->event_type = EVENT_READ_RIL_RECV_MESSAGE;
                }
            } else {
                printf("%s read message is NULL!\n", __func__);
            }
        } else if (NULL != strstr(buf, "+CMGR:")) {
            amend_buf_after_send(RIL_READ_MESSAGE, buf);
            memcpy(sms_msg_buf , buf, (strlen(buf) + 1));
            event_data->event_msg0 = (void *)sms_msg_buf;
            event_data->event_type = EVENT_READ_RIL_READ_MESSAGE;
        }
    } else {
        event_data->event_type = EVENT_READ_RIL_NOTHING;
    }

    return 0;
}

int deinit_listen_ril(int listen_fd)
{
    send_riL_cmd_and_read(RIL_CLOSE_NET, NULL);
    close(listen_fd);
    return 0;
}

