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

#include "wifi_management.h"

#include <dirent.h>
#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <sys/stat.h>

#include "common.h"
#include "menu_commond.h"
#include "parameter.h"
#include "public_interface.h"
#include "system.h"
#include "video.h"
#include "wifi_api.h"
#include "wifi_ota_interface.h"
#include "wlan_service_client.h"
#include "wifi_setting_interface.h"
#include "wifi_debug_interface.h"
#include "fs_manage/fs_storage.h"
#include "wifi_api.h"

#define RTP_TS_TRANS_PORT 20000
#define MAX_CLIENT_LIMIT_NUM   15
#define RTSP_PIPE_NAME  "pipe:///tmp/rv110x_video_fifo"

#define BUFF_SIZE_S 64
#define BUFF_SIZE_M 128
#define BUFF_SIZE_L 256

#define UDPPORT 18889
#define TCPPORT 8888

static int broadcast_fd = -1;
static int tcp_server_fd = -1;
static int tcp_server_tab[MAX_CLIENT_LIMIT_NUM];
static pthread_t broadcast_tid;
static pthread_t tcp_server_tid;
static int g_wifi_is_running;
static int g_rndis_is_running;
static int live_client_num;
static int live_mode_flag;
static int link_client_num;
char SSID[33];
char PASSWD[65];
static int is_stopwifi;
static struct file_list *mmedia_file_list = NULL;
static int live_deviceid;

#define STR_DEBUG_ARGSETTING    "CMD_DEBUG_ARGSETTING"

static int cmd_resolve(int fd, char *cmdstr);

int runapp(char *cmd)
{
    return system_fd_closexec(cmd);
#if 0
    char buffer[BUFSIZ] = {0};
    FILE *read_fp;
    int chars_read;
    int ret;

    read_fp = popen(cmd, "r");
    if (read_fp != NULL) {
        chars_read = fread(buffer, sizeof(char), BUFSIZ - 1, read_fp);
        if (chars_read > 0)
            ret = 1;
        else
            ret = -1;
        pclose(read_fp);
    } else {
        ret = -1;
    }

    return ret;
#endif
}

int get_live_client_num(void)
{
    return live_client_num;
}

int get_link_client_num(void)
{
    return link_client_num;
}

int get_tcp_server_socket(int num)
{
    return tcp_server_tab[num];
}

int linux_get_mac(char *mac)
{
    int sockfd;
    struct ifreq tmp;
    char mac_addr[30] = {0};

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("create socket fail");
        return -1;
    }

    memset(&tmp, 0, sizeof(struct ifreq));
    strncpy(tmp.ifr_name, "wlan0", sizeof(tmp.ifr_name) - 1);
    if ((ioctl(sockfd, SIOCGIFHWADDR, &tmp)) < 0) {
        printf("mac ioctl error\n");
        return -1;
    }

    snprintf(mac_addr, sizeof(mac_addr), "%02x%02x%02x%02x%02x%02x",
             (unsigned char)tmp.ifr_hwaddr.sa_data[0],
             (unsigned char)tmp.ifr_hwaddr.sa_data[1],
             (unsigned char)tmp.ifr_hwaddr.sa_data[2],
             (unsigned char)tmp.ifr_hwaddr.sa_data[3],
             (unsigned char)tmp.ifr_hwaddr.sa_data[4],
             (unsigned char)tmp.ifr_hwaddr.sa_data[5]
            );
    close(sockfd);
    memcpy(mac, mac_addr, strlen(mac_addr));

    return 0;
}

int get_g_wifi_is_running(void)
{
    return g_wifi_is_running;
}

static int tcp_func_delete_file(int nfp, char *buffer)
{
    const char *fault = "CMD_DELFAULT";
    const char *success = "CMD_DELSUCCESS";
    char name[128] = {0}, type[64] = {0};
    int len;
    int form;
    int *camera_type;
    char path[512] = {0};
    char *ptr;

    printf("delete file cmd : %s\n", buffer);

    ptr = strstr(buffer, "FORM:");
    if (ptr == NULL) {
        printf("strstr FORM fault\n");
        return -1;
    }
    ptr[0] = 0;
    ptr = &ptr[5];
    form = atoi(ptr);

    ptr = strstr(buffer, "TYPE:");
    if (ptr == NULL) {
        printf("strstr TYPE fault\n");
        return -1;
    }
    ptr[0] = 0;
    ptr = &ptr[5];
    strcpy(type, ptr);

    ptr = strstr(buffer, "NAME:");
    if (ptr == NULL) {
        printf("strstr NAME fault\n");
        return -1;
    }
    ptr[0] = 0;
    ptr = &ptr[5];
    strcpy(name, ptr);

    printf("Name = %s , TYPE = %s , FORM = %d\n", name, type, form);

    camera_type = fs_storage_filetypetbl_get();

    strcat(path, fs_storage_folder_get_bytype(*(camera_type + form),
                                              THUMBFILE_TYPE));
    strcat(path, "/");
    strcat(path, name);
    len = strlen(path);
    path[len - 1] = 'g';
    path[len - 2] = 'p';
    path[len - 3] = 'j';
    printf("delete preview:%s\n", path);
    if (fs_storage_remove(path, 0) < 0) {
        perror("remove");
    } else {
        printf("delete %s success\n", path);
    }
    memset(path, 0, sizeof(path));

    if (strcmp(type, "normal") == 0)
        strcat(path, fs_storage_folder_get_bytype(*(camera_type + form),
                                                  VIDEOFILE_TYPE));
    else if (strcmp(type, "lock") == 0)
        strcat(path, fs_storage_folder_get_bytype(*(camera_type + form),
                                                  LOCKFILE_TYPE));
    else if (strcmp(type, "picture") == 0)
        strcat(path, fs_storage_folder_get_bytype(*(camera_type + form), PICFILE_TYPE));

    strcat(path, "/");
    strcat(path, name);
    printf("path:%s\n", path);

    if (fs_storage_remove(path, 0) < 0) {
        perror("remove");
        if (write(nfp, fault, strlen(fault)) < 0)
            printf("write fail!\r\n");
    } else {
        printf("delete %s success\n", path);
        if (write(nfp, success, strlen(success)) < 0)
            printf("write fail!\r\n");
    }
    return 0;
}

int tcp_pack_file_for_path(char *type, char *path, int form,
                           int filetype, char *buff)
{
    struct stat fileinfo;
    char * d_name;
    char *pstr = NULL;
    struct tm *time;

    d_name = strrchr(path, '/');
    d_name = &d_name[1];
    printf("d_name = %s\n", d_name);

    if (stat(path, &fileinfo) < 0) {
        printf("stat(%s);\n", path);
        perror("stat");
        return -1;
    }

    strcat(buff, "CMD_CB_GETCAMFILE");
    strcat(buff, "NAME:");
    strcat(buff, d_name);
    strcat(buff, "TYPE:");
    strcat(buff, type);

    strcat(buff, "PATH:");

    pstr = fs_storage_folder_get_bytype(form, filetype);
    if (pstr == NULL) {
        strcat(buff, "NULL");
    } else {
        strcat(buff, &pstr[sizeof("/mnt/sdcard/") - 1]);
    }

    strcat(buff, "&");
    if (filetype == PICFILE_TYPE) {
        pstr = fs_storage_folder_get_bytype(form, filetype);
        if (pstr == NULL)
            strcat(buff, "NULL");
        else
            strcat(buff, &pstr[sizeof("/mnt/sdcard/") - 1]);
    } else {
        pstr = fs_storage_folder_get_bytype(form, THUMBFILE_TYPE);
        if (pstr == NULL)
            strcat(buff, "NULL");
        else
            strcat(buff, &pstr[sizeof("/mnt/sdcard/") - 1]);
    }

    strcat(buff, "&");
    pstr = fs_storage_folder_get_bytype(form, GPS_TYPE);
    if (pstr == NULL)
        strcat(buff, "NULL");
    else
        strcat(buff, &pstr[sizeof("/mnt/sdcard/") - 1]);

    sprintf(&buff[strlen(buff)], "FORM:%dSIZE:%ld",
            form, (long int)fileinfo.st_size);
    time = localtime(&fileinfo.st_mtime);
    sprintf(&buff[strlen(buff)],
            "DAY:%04d-%02d-%02d", time->tm_year + 1900,
            time->tm_mon + 1, time->tm_mday);
    sprintf(&buff[strlen(buff)],
            "TIME:%02d:%02d:%02d", time->tm_hour,
            time->tm_min, time->tm_sec);

    strcat(buff, "COUNT:0");
    strcat(buff, "END");

    return 0;
}

static int tcp_pack_file_list(int nfp, char *path, char *type, int form,
                              FILETYPE filetype)
{
    int i = 0;
    char count[32];
    char wifi_info[1024];
    char ackbuf[256];
    struct tm *time;
    char filepath[128];
    struct dirent *ent = NULL;
    DIR *p_dir;
    struct stat fileinfo;
    char *pstr = NULL;
    int *camera_type;

    printf("read dirent = %s\n", path);

    p_dir = opendir(path);
    if (p_dir == NULL) {
        perror("opendir");
        return -1;
    }

    camera_type = fs_storage_filetypetbl_get();

    while (NULL != (ent = readdir(p_dir))) {
        if (ent->d_type == 8) {
            memset(count, 0, sizeof(count));
            memset(wifi_info, 0, sizeof(wifi_info));
            memset(ackbuf, 0, sizeof(ackbuf));
            snprintf(count, sizeof(count), "%d", i);

            snprintf(filepath, sizeof(filepath), "%s/%s", path, ent->d_name);
            if (stat(filepath, &fileinfo) < 0) {
                printf("stat(%s);\n", filepath);
                perror("stat");
                return -1;
            }

            if (ent->d_name[0] == '.')
                continue;

            strcat(wifi_info, "CMD_GETCAMFILE");
            strcat(wifi_info, "NAME:");
            strcat(wifi_info, ent->d_name);
            strcat(wifi_info, "TYPE:");
            strcat(wifi_info, type);

            strcat(wifi_info, "PATH:");

            pstr = fs_storage_folder_get_bytype(*(camera_type + form), filetype);
            if (pstr == NULL) {
                strcat(wifi_info, "NULL");
            } else {
                strcat(wifi_info, &pstr[sizeof("/mnt/sdcard/") - 1]);
            }
            strcat(wifi_info, "&");

            if (filetype == PICFILE_TYPE) {
                pstr = fs_storage_folder_get_bytype(*(camera_type + form), filetype);
                if (pstr == NULL) {
                    strcat(wifi_info, "NULL");
                } else {
                    strcat(wifi_info, &pstr[sizeof("/mnt/sdcard/") - 1]);
                }
            } else {
                pstr = fs_storage_folder_get_bytype(*(camera_type + form), THUMBFILE_TYPE);
                if (pstr == NULL) {
                    strcat(wifi_info, "NULL");
                } else {
                    strcat(wifi_info, &pstr[sizeof("/mnt/sdcard/") - 1]);
                }
            }

            strcat(wifi_info, "&");
            pstr = fs_storage_folder_get_bytype(*(camera_type + form), GPS_TYPE);
            if (pstr == NULL) {
                strcat(wifi_info, "NULL");
            } else {
                strcat(wifi_info, &pstr[sizeof("/mnt/sdcard/") - 1]);
            }

            sprintf(&wifi_info[strlen(wifi_info)], "FORM:%dSIZE:%ld",
                    form, (long int)fileinfo.st_size);
            time = localtime(&fileinfo.st_mtime);
            sprintf(&wifi_info[strlen(wifi_info)],
                    "DAY:%04d-%02d-%02d", time->tm_year + 1900,
                    time->tm_mon + 1, time->tm_mday);
            sprintf(&wifi_info[strlen(wifi_info)],
                    "TIME:%02d:%02d:%02d", time->tm_hour,
                    time->tm_min, time->tm_sec);

            strcat(wifi_info, "COUNT:");
            strcat(wifi_info, count);
            strcat(wifi_info, "END");
            printf("wifi_info = %s\n", wifi_info);

            if (0 > write(nfp, wifi_info, strlen(wifi_info)))
                printf("write fail!\r\n");

            if (0 > read(nfp, ackbuf, sizeof(ackbuf)))
                printf("write fail!\r\n");

            if (strcmp(ackbuf, "CMD_NEXTFILE") < 0) {
                printf("CMD_NEXTFILE fault : %s\n", ackbuf);
                break;
            }

            i++;
        }
    }

    closedir(p_dir);

    return 0;
}

static int tcp_func_set_ssid_pw(int nfp, char *buffer)
{
    char *p = NULL, *p1 = NULL;
    char ssid[33] = {0}, passwd[65] = {0};

    printf("CMD_SETWIFI Get Data:%s\n", buffer);

    p1 = buffer;
    p = strstr(buffer, "WIFINAME:");
    if (p != NULL) {
        p = &p[sizeof("WIFINAME:") - 1];

        while (strstr(p1, "PASSWD:") != NULL) {
            p1 = strstr(p1, "PASSWD:");
            p1 = &p1[sizeof("PASSWD:") - 1];
        }

        if (p1 != buffer) {
            memcpy(ssid, p,
                   (int)p1 - (int)p - sizeof("PASSWD:") + 1);
            if (strlen(p1) > sizeof(passwd)) {
                printf("passwd error:%s\n", p1);
                return -1;
            }
            memcpy(passwd, p1, strlen(p1));
        } else {
            printf("without PASSWD:\n");
            return -1;
        }
    } else {
        printf("without SSID:\n");
        return -1;
    }
    printf("CMD_SETWIFI SSID = %s PASSWD = %s\n",
           ssid, passwd);

    parameter_getwifiinfo(SSID, PASSWD);
    if (strcmp(ssid, SSID) != 0) {
        printf("%s != %s\n", ssid, SSID);
        return -1;
    }

    parameter_savewifipass(passwd);
    parameter_sav_wifi_mode(0);

    api_power_reboot();
    return 0;
}

static int tcp_func_appear_ssid(int nfp, char *buffer)
{
    char *p = NULL, *p1 = NULL;
    char ssid[33] = {0}, passwd[65] = {0};

    printf("CMD_APPEARSSID Get Data:%s\n", buffer);

    p1 = buffer;
    p = strstr(buffer, "SSID:");
    if (p != NULL) {
        p = &p[sizeof("SSID:") - 1];

        while (strstr(p1, "PASSWD:") != NULL) {
            p1 = strstr(p1, "PASSWD:");
            p1 = &p1[sizeof("PASSWD:") - 1];
        }

        if (p1 != buffer) {
            memcpy(ssid, p,
                   (int)p1 - (int)p - sizeof("PASSWD:") + 1);
            if (strlen(p1) > sizeof(passwd)) {
                printf("passwd error:%s\n", p1);
                return -1;
            }
            memcpy(passwd, p1, strlen(p1));
        } else {
            printf("without PASSWD:\n");
            return -1;
        }
    } else {
        printf("without SSID:\n");
        return -1;
    }

    printf("CMD_APPEARSSID:\nssid = %s\n passwd = %s\n",
           ssid, passwd);
    parameter_sav_wifi_mode(1);
    parameter_sav_staewifiinfo(ssid, passwd);

    printf("reboot....\n");
    api_power_reboot();
    return 0;
}

static int tcp_func_get_file_list(int nfp, char *buffer)
{
    int i, ret = -1;
    char *path;
    int type_num, *camera_type;

    if (strstr(buffer, "CMD_GETFCAMFILETYPE:") == 0) {
        printf("ResolveGetFileList cmd error %s\n", buffer);
        return -1;
    }
    buffer = &buffer[sizeof("CMD_GETFCAMFILETYPE:") - 1];

    printf("type = %s \n", buffer);

    type_num = fs_storage_filetypenum_get();
    camera_type = fs_storage_filetypetbl_get();
    if (strcmp(buffer, "normal") == 0) {
        for (i = 0; i < type_num ; i++) {
            path = fs_storage_folder_get_bytype(*(camera_type + i), VIDEOFILE_TYPE);
            ret = tcp_pack_file_list(nfp, path, "normal", i, VIDEOFILE_TYPE);
            if (ret < 0)
                return ret;
        }
    } else if (strcmp(buffer, "lock") == 0) {
        for (i = 0; i < type_num ; i++) {
            path = fs_storage_folder_get_bytype(*(camera_type + i), LOCKFILE_TYPE);
            ret = tcp_pack_file_list(nfp, path, "lock", i, LOCKFILE_TYPE);
            if (ret < 0)
                return ret;
        }
    } else if (strcmp(buffer, "picture") == 0) {
        for (i = 0; i < type_num ; i++) {
            path = fs_storage_folder_get_bytype(*(camera_type + i), PICFILE_TYPE);
            ret = tcp_pack_file_list(nfp, path, "picture", i, PICFILE_TYPE);
            if (ret < 0)
                return ret;
        }
    }

    if (0 > write(nfp, "CMD_ACK_GETCAMFILE_FINISH",
                  strlen("CMD_ACK_GETCAMFILE_FINISH")))
        printf("write fail!\r\n");
    return ret;
}

static int tcp_func_set_on_off_recording(int nfp, char *buffer)
{
    char *p = &buffer[sizeof("CMD_Control_Recording")];

    if ( setting_func_rec_ui(p) == 1 ) {
        if (0 > write(nfp, "CMD_CB_NoSDCard",
                      strlen("CMD_CB_NoSDCard")))
            printf("write fail!\r\n");
    }
    return 0;
}

static int tcp_func_start_rtsp_live(int nfp, char *buffer)
{
    if (live_client_num != 0)
        return 0;
    live_client_num++;

    printf("start rtsp : %s %d\n", RTSP_PIPE_NAME, live_deviceid);
    video_record_start_ts_transfer(RTSP_PIPE_NAME, live_deviceid);
    // video_record_start_ts_transfer("rtmp://192.168.31.160:1935/rtmplive/home live=1");
    if (0 > write(nfp, "CMD_ACK_START_RTSP_LIVE",
                  strlen("CMD_ACK_START_RTSP_LIVE")))
        printf("write fail!\r\n");

    live_mode_flag = 1;
    return 0;
}

static int tcp_func_start_ts_live(int nfp, char *buffer)
{
    char buf[64] = {0};
    struct sockaddr_in sa;
    unsigned int len = sizeof(sa);

    live_mode_flag = 0;
    if (live_client_num != 0) {
        video_record_stop_ts_transfer(1);
        if (0 > write(nfp, "CMD_ACK_STOP_TS_LIVE",
                      strlen("CMD_ACK_STOP_TS_LIVE")))
            printf("write fail!\r\n");
    }
    live_client_num = 1;

    if (!getpeername(nfp, (struct sockaddr *)&sa, &len)) {
        snprintf(buf, sizeof(buf), "rtp://%s:%d", inet_ntoa(sa.sin_addr), RTP_TS_TRANS_PORT);
        printf("video_record_start_ts_transfer(%s);\n", buf);
        video_record_start_ts_transfer(buf, live_deviceid);

        if (0 > write(nfp, "CMD_ACK_START_TS_LIVE",
                      strlen("CMD_ACK_START_TS_LIVE")))
            printf("write fail!\r\n");
    }
    return 0;
}

static int tcp_func_stop_live(int nfp, char *buffer)
{
    if (live_client_num == 0)
        return 0;
    live_client_num = 0;

    printf("tcp_func_stop_live\n");

    if (live_mode_flag == 0) {
        video_record_stop_ts_transfer(1);
        if (0 > write(nfp, "CMD_ACK_STOP_TS_LIVE",
                      strlen("CMD_ACK_STOP_TS_LIVE")))
            printf("write fail!\r\n");
    } else if (live_mode_flag == 1) {
        video_record_stop_ts_transfer(0);
    }
    return 0;
}

static int tcp_func_get_mode(int nfp, char *buffer)
{
    char tx_buff[BUFF_SIZE_S] = {0};

    switch (api_get_mode()) {
    case MODE_RECORDING:
        if (parameter_get_video_lapse_state() == 0)
            memcpy(tx_buff, "CMD_CB_GET_MODE:RECORDING&OFF", sizeof("CMD_CB_GET_MODE:RECORDING&OFF"));
        else
            snprintf(tx_buff, sizeof(tx_buff), "CMD_CB_GET_MODE:LAPSE&%d",
                     parameter_get_time_lapse_interval());
        break;

    case MODE_PHOTO:
        if (parameter_get_photo_burst_state() == 0)
            memcpy(tx_buff, "CMD_CB_GET_MODE:PHOTO&OFF", sizeof("CMD_CB_GET_MODE:PHOTO&OFF"));
        else
            snprintf(tx_buff, sizeof(tx_buff), "CMD_CB_GET_MODE:BURST&%d",
                     parameter_get_photo_burst_num());
        break;

    case MODE_EXPLORER:
        memcpy(tx_buff, "CMD_CB_GET_MODE:EXPLORER", sizeof("CMD_CB_GET_MODE:EXPLORER"));
        break;

    case MODE_PREVIEW:
        memcpy(tx_buff, "CMD_CB_GET_MODE:PREVIEW", sizeof("CMD_CB_GET_MODE:PREVIEW"));
        break;

    case MODE_PLAY:
        memcpy(tx_buff, "CMD_CB_GET_MODE:PLAY", sizeof("CMD_CB_GET_MODE:PLAY"));
        break;

    case MODE_USBDIALOG:
        memcpy(tx_buff, "CMD_CB_GET_MODE:USBDIALOG", sizeof("CMD_CB_GET_MODE:USBDIALOG"));
        break;

    case MODE_USBCONNECTION:
        memcpy(tx_buff, "CMD_CB_GET_MODE:USBCONNECTION", sizeof("CMD_CB_GET_MODE:USBCONNECTION"));
        break;

    case MODE_SUSPEND:
        memcpy(tx_buff, "CMD_CB_GET_MODE:SUSPEND", sizeof("CMD_CB_GET_MODE:SUSPEND"));
        break;

    case MODE_NONE:
    default:
        return -1;
    }

    if (0 > write(nfp, tx_buff, strlen(tx_buff)))
        printf("write fail!\r\n");

    return 0;
}

static int tcp_func_change_mode(int nfp, char *buffer)
{
    char * p;

    p = &buffer[sizeof("CMD_CHANGE_MODE")];

    printf("mode change = %s\n", p);
    if (strcmp(p, "RECORDING") == 0) {
        if (parameter_get_video_lapse_state())
            parameter_save_video_lapse_state(false);

        if (parameter_get_time_lapse_interval() != 0) {
            parameter_save_time_lapse_interval(0);
            video_record_set_timelapseint(0);
        }

        api_send_msg(MSG_MODE_CHANGE_VIDEO_NORMAL_NOTIFY, TYPE_LOCAL, NULL, NULL);
        api_change_mode(MODE_RECORDING);
    } else if (strcmp(p, "PHOTO") == 0) {
        if (parameter_get_photo_burst_state())
            parameter_save_photo_burst_state(false);

        api_send_msg(MSG_MODE_CHANGE_PHOTO_NORMAL_NOTIFY, TYPE_LOCAL, NULL, NULL);
        api_change_mode(MODE_PHOTO);
    }

    return 0;
}

static int tcp_func_camera_switch(int nfp, char *buffer)
{
    printf("tcp_func_camera_switch \n");
    video_record_display_switch();

    return 0;
}

static time_t Date2Unix(struct tm t)
{
    t.tm_year -= 1900;
    t.tm_mon -= 1;
    return mktime(&t);
}

static int tcp_send_file_name(int nfp, char * tx_buff)
{
    char ackbuf[256];

    printf("send = %s\n", tx_buff);
    if (0 > write(nfp, tx_buff, strlen(tx_buff))) {
        printf("write fail!\r\n");
        return -1;
    }

    if (0 > read(nfp, ackbuf, sizeof(ackbuf))) {
        printf("ack fail!\r\n");
        return -1;
    }

    if (strcmp(ackbuf, "CMD_NEXTFILE") < 0) {
        printf("CMD_NEXTFILE fault : %s\n", ackbuf);
        return -1;
    }
    return 0;
}

static int traversal_fit_handle(int nfp, struct file_node * node_head, int recode_time)
{
    time_t t1, t2;
    struct file_node* node_tmp = node_head;
    struct file_node* node_bck = NULL;
    char tx_buff[BUFF_SIZE_L] = {0};

    t1 = node_tmp->time;

    while (node_tmp) {
        node_bck = node_tmp;
        node_tmp = node_tmp->pre;
        if (node_tmp == NULL)
            break;

        t2 = node_tmp->time;

        if (t1 > t2) {
            if (t1 - t2 <= recode_time) {
                t1 = t2;
                continue;
            }
        } else {
            if (t2 - t1 <= recode_time) {
                t1 = t2;
                continue;
            }
        }
        break;
    }

    node_tmp = node_bck;
    t1 = node_tmp->time;

    while (node_tmp) {
        t2 = node_tmp->time;

        if (t1 > t2) {
            if (t1 - t2 <= recode_time) {
                t1 = t2;
                snprintf(tx_buff, sizeof(tx_buff), "CMD_ACK_GET_GPS_LIST:%s", node_tmp->name);
                if (tcp_send_file_name(nfp, tx_buff) < 0)
                    return -1;
            }
        } else {
            if (t2 - t1 <= recode_time) {
                t1 = t2;
                snprintf(tx_buff, sizeof(tx_buff), "CMD_ACK_GET_GPS_LIST:%s", node_tmp->name);
                if (tcp_send_file_name(nfp, tx_buff) < 0)
                    return -1;
            }
        }
        node_tmp = node_tmp->next;
        if (node_tmp == NULL)
            break;
    }

    return 0;
}

static int tcp_func_get_gps_list(int nfp, char *buffer)
{
    char tx_buff[BUFF_SIZE_L] = {0};
    struct tm tm;
    int recode_time;
    char type[BUFF_SIZE_S];
    time_t t1, t2;
    struct file_list **file_list = &mmedia_file_list;
    struct file_node* node_tmp;
    struct file_node * node_head = NULL;
    char * p;

    buffer = &buffer[sizeof("CMD_GET_GPS_LIST")];

    sscanf(buffer, "%04d%02d%02d_%02d%02d%02d_%s",
           &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
           &tm.tm_hour, &tm.tm_min, &tm.tm_sec,
           type);
    t1 = Date2Unix(tm);

    (*file_list) = calloc(1, sizeof(struct file_list));
    if ((*file_list) == NULL) {
        printf("creat video file list err\n");
        return -1;
    }

    fs_storage_get_medialist_bytype(VIDEO_TYPE_ISP,
                                    GPS_TYPE,
                                    *file_list);

    if (*file_list) {
        node_tmp = (*file_list)->file_head;

        while (node_tmp) {
            if (node_tmp->name != NULL) {
                if (strlen(node_tmp->name) < sizeof("yyyymmdd_hhmmss_x.txt") - 1) {
                    free(*file_list);
                    return -1;
                }
                p = &node_tmp->name[sizeof("yyyymmdd_hhmmss_") - 1];
                sscanf(p, "%d.txt", &recode_time);
                recode_time *= 60;
            } else {
                return -1;
            }

            t2 = node_tmp->time;

            if (t1 == t2) {
                traversal_fit_handle(nfp, node_tmp, recode_time * 2);
                free(*file_list);
                return 0;
            } else if (t1 > t2) {
                if (t1 - t2 < recode_time) {
                    node_head = node_tmp;
                    traversal_fit_handle(nfp, node_tmp, recode_time * 2);
                    goto end;
                }
            } else {
                if (t2 - t1 < recode_time) {
                    node_head = node_tmp;
                    traversal_fit_handle(nfp, node_tmp, recode_time * 2);
                    goto end;
                }
            }

            node_tmp = node_tmp->next;
        }
    }
end:
    if (node_head == NULL) {
        snprintf(tx_buff, sizeof(tx_buff), "CMD_ACK_GET_GPS_LIST_END:NULL");
    } else {
        snprintf(tx_buff, sizeof(tx_buff), "CMD_ACK_GET_GPS_LIST_END:%s", node_head->name);
    }

    printf("send = %s\n", tx_buff);
    if (0 > write(nfp, tx_buff, strlen(tx_buff)))
        printf("write fail!\r\n");

    free(*file_list);
    return -1;
}

static int tcp_func_take_photo(int nfp, char *buffer)
{
    if ( !parameter_get_photo_burst_state())
        api_take_photo(1);
    else
        api_take_photo(parameter_get_photo_burst_num());

    return 0;
}

static int tcp_func_test_alive(int nfp, char *buffer)
{
    if (0 > write(nfp, "ALIVE", strlen("ALIVE")))
        printf("write fail!\r\n");

    return 0;
}

static int tcp_func_get_live_video(int nfp, char *buffer)
{
    char buf[512] = {0};

    snprintf(buf, sizeof(buf), "CMD_ACK_GET_LIVE_VIDEO:%d", live_deviceid);
    if (0 > write(nfp, buf, strlen(buf)))
        printf("write fail!\r\n");

    return 0;
}

static int tcp_func_set_live_video(int nfp, char *buffer)
{
    char * p = &buffer[sizeof("CMD_SET_LIVE_VIDEO")];

    live_deviceid = atoi(p);
    if (0 > write(nfp, "CMD_ACK_SET_LIVE_VIDEO", sizeof("CMD_ACK_SET_LIVE_VIDEO")))
        printf("write fail!\r\n");

    return 0;
}

static int tcp_func_get_video_list(int nfp, char *buffer)
{
    char buf[512] = "CMD_ACK_GET_VIDEO_LIST:";
    char str[64];
    int id = -1;

    video_record_get_deviceid_by_type(&id, VIDEO_TYPE_ISP, 0);
    if (id >= 0) {
        strcat(buf, "ISP&");
        snprintf(str, sizeof(str), "%d&", id);
        strcat(buf, str);
    }

    video_record_get_deviceid_by_type(&id, VIDEO_TYPE_USB, 0);
    if (id >= 0) {
        strcat(buf, "USB&");
        snprintf(str, sizeof(str), "%d&", id);
        strcat(buf, str);
    }

    video_record_get_deviceid_by_type(&id, VIDEO_TYPE_CIF, 0);
    if (id >= 0) {
        strcat(buf, "CIF0&");
        snprintf(str, sizeof(str), "%d&", id);
        strcat(buf, str);
    }

    video_record_get_deviceid_by_type(&id, VIDEO_TYPE_CIF, 1);
    if (id >= 0) {
        strcat(buf, "CIF1&");
        snprintf(str, sizeof(str), "%d&", id);
        strcat(buf, str);
    }

    video_record_get_deviceid_by_type(&id, VIDEO_TYPE_CIF, 2);
    if (id >= 0) {
        strcat(buf, "CIF2&");
        snprintf(str, sizeof(str), "%d&", id);
        strcat(buf, str);
    }

    video_record_get_deviceid_by_type(&id, VIDEO_TYPE_CIF, 3);
    if (id >= 0) {
        strcat(buf, "CIF3&");
        snprintf(str, sizeof(str), "%d&", id);
        strcat(buf, str);
    }

    printf(buf);
    if (0 > write(nfp, buf, strlen(buf)))
        printf("write fail!\r\n");
    return 0;
}

static void *tcp_receive(void *arg)
{
    int nfp = (int)arg;
    int recbytes;
    char buffer[512] = {0};
    char data[512] = {0};

    prctl(PR_SET_NAME, __func__, 0, 0, 0);
    pthread_detach(pthread_self());
    printf("tcp_receive : tcp_receive nfp = 0x%x\n", nfp);
    while (g_wifi_is_running || g_rndis_is_running) {
        memset(buffer, 0, sizeof(buffer));
        memset(data, 0, sizeof(data));

        printf("tcp_receive : Wait TCP CMD.....\n");

        recbytes = read(nfp, buffer, sizeof(buffer));
        if (0 >= recbytes) {
            printf("tcp_receive : disconnect!\r\n");
            goto stop_tcp_receive;
        }

        buffer[recbytes] = '\0';

        cmd_resolve(nfp, buffer);
    }

stop_tcp_receive:
    printf("tcp_receive : stop tcp_receive\n");
    if (link_client_num == 0) {
        pthread_exit(NULL);
        return NULL;
    }

    if (!is_stopwifi) {
        if (--link_client_num == 0)
            tcp_func_stop_live(nfp, NULL);

        shutdown(nfp, 2);
        close(nfp);
        tcp_server_tab[link_client_num] = -1;
    }

    pthread_exit(NULL);
    return NULL;
}

static void *tcp_server_thread(void *arg)
{
    int nfp = 0;
    struct sockaddr_in s_add, c_add;
    socklen_t sin_size;
    pthread_t tcpreceive_tid;
    int i;
    int opt = 1;

    prctl(PR_SET_NAME, __func__, 0, 0, 0);
    pthread_detach(pthread_self());

    for (i = 0; i < MAX_CLIENT_LIMIT_NUM; i++)
        tcp_server_tab[i] = -1;

    tcp_server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_server_fd < 0) {
        printf("tcp_server_thread : socket fail ! \r\n");
        pthread_exit(NULL);
        return NULL;
    }
    if (setsockopt(tcp_server_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
                   sizeof(opt)) < 0) {
        perror("SO_REUSEADDR");
        shutdown(tcp_server_fd, 2);
        close(tcp_server_fd);
        pthread_exit(NULL);
        return NULL;
    }
    printf("socket ok !\r\n");

    bzero(&s_add, sizeof(struct sockaddr_in));
    s_add.sin_family = AF_INET;
    s_add.sin_addr.s_addr = INADDR_ANY;
    s_add.sin_port = htons(TCPPORT);

    while (bind(tcp_server_fd, (struct sockaddr *)(&s_add),
                sizeof(struct sockaddr)) != 0) {
        perror("bind fail");
        sleep(1);
    }
    printf("tcp_server_thread : bind ok !\r\n");

    if (listen(tcp_server_fd, MAX_CLIENT_LIMIT_NUM) < 0) {
        perror("listen fail");
        goto stop_tcp_server_thread;
    }
    printf("listen ok\r\n");
    while (g_wifi_is_running || g_rndis_is_running) {
        sin_size = sizeof(struct sockaddr_in);

        nfp = accept(tcp_server_fd, (struct sockaddr *)(&c_add),
                     &sin_size);
        if (nfp < 0) {
            perror("accept fail!");
            goto stop_tcp_server_thread;
        }
        printf("accept ok!\r\nServer get connect from %#x : %#x\r\n",
               ntohl(c_add.sin_addr.s_addr), ntohs(c_add.sin_port));

        if (link_client_num >= MAX_CLIENT_LIMIT_NUM)
            printf("!!! link [%d] >= listen [%d]!!!\n", link_client_num,
                   MAX_CLIENT_LIMIT_NUM);
        tcp_server_tab[link_client_num++] = nfp;

        printf("tcp_receive : link_client_num = %d\n", link_client_num);

        if (pthread_create(&tcpreceive_tid, NULL, tcp_receive,
                           (void *)nfp) != 0) {
            perror("Create thread error!");
            goto stop_tcp_server_thread;
        }

        printf("tcpreceive TID in pthread_create function: %x.\n",
               (int)tcpreceive_tid);
    }

stop_tcp_server_thread:
    printf("tcp_server_thread : stop tcp_server_thread\n");
    if (tcp_server_fd >= 0) {
        shutdown(tcp_server_fd, 2);
        close(tcp_server_fd);
        tcp_server_fd = -1;
    }

    while (link_client_num > 0) {
        if (--link_client_num == 0)
            tcp_func_stop_live(tcp_server_tab[link_client_num], NULL);

        shutdown(tcp_server_tab[link_client_num], 2);
        close(tcp_server_tab[link_client_num]);
        tcp_server_tab[link_client_num] = -1;
    }
    pthread_exit(NULL);
    return NULL;
}

static void *broadcast_thread(void *arg)
{
    socklen_t addr_len = 0;
    char buf[64];
    char name[64];
    size_t len = sizeof(name);
    struct sockaddr_in server_addr;
    char wifi_info[128];
    int opt = 1;

    prctl(PR_SET_NAME, __func__, 0, 0, 0);
    pthread_detach(pthread_self());
    broadcast_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (broadcast_fd < 0) {
        perror("socket");
        pthread_exit(NULL);
    }
    printf("broadcast_thread socketfd = %d\n", broadcast_fd);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(UDPPORT);
    addr_len = sizeof(server_addr);

    if (setsockopt(broadcast_fd, SOL_SOCKET, SO_BROADCAST, (char *)&opt,
                   sizeof(opt)) < 0) {
        perror("setsockopt SO_BROADCAST");
        goto stop_broadcast_thread;
    }
    if (setsockopt(broadcast_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
                   sizeof(opt)) < 0) {
        perror("setsockopt SO_REUSEADDR");
        goto stop_broadcast_thread;
    }

    while (bind(broadcast_fd, (struct sockaddr *)&server_addr, addr_len) != 0) {
        perror("broadcast_thread bind");
        sleep(1);
    }

    gethostname(name, len);
    while (g_wifi_is_running || g_rndis_is_running) {
        printf("Wait Discover....\n");
        addr_len = sizeof(server_addr);
        memset(&server_addr, 0, sizeof(server_addr));
        memset(buf, 0, sizeof(buf));
        if (recvfrom(broadcast_fd, buf, 64, 0,
                     (struct sockaddr *)&server_addr, &addr_len) < 0) {
            perror("broadcast_thread recvfrom");
            goto stop_broadcast_thread;
        }
        printf("broadcast_thread : from: %s port: %d > %s\n",
               inet_ntoa(server_addr.sin_addr),
               ntohs(server_addr.sin_port), buf);

        if (strcmp("CMD_DISCOVER", buf) == 0) {
            memset(wifi_info, 0, sizeof(wifi_info));

            strcat(wifi_info, "CMD_DISCOVER:WIFINAME:");
            strcat(wifi_info, SSID);

            strcat(wifi_info, "SID:");
            linux_get_mac(&wifi_info[strlen(wifi_info)]);

#ifdef CVR
            strcat(wifi_info, "PRODUCT:CVR");
#elif SDV
            strcat(wifi_info, "PRODUCT:SportDV");
#endif

            strcat(wifi_info, "END");

            addr_len = sizeof(server_addr);
            printf("sendto: %s port: %d > %s\n",
                   inet_ntoa(server_addr.sin_addr),
                   ntohs(server_addr.sin_port), wifi_info);
            if (sendto(broadcast_fd, wifi_info, strlen(wifi_info),
                       0, (struct sockaddr *)&server_addr,
                       addr_len) < 0) {
                perror("broadcast_thread recvfrom");
                goto stop_broadcast_thread;
            }
        }
    }

stop_broadcast_thread:
    printf("stop broadcast_thread\n");
    if (broadcast_fd >= 0) {
        shutdown(broadcast_fd, 2);
        close(broadcast_fd);
        broadcast_fd = -1;
    }
    pthread_exit(NULL);
    return NULL;
}

static void set_default_live_video(void)
{
    int id = -1;

    video_record_get_deviceid_by_type(&id, VIDEO_TYPE_ISP, 0);
    if (id >= 0) {
        live_deviceid = id;
        return;
    }

    video_record_get_deviceid_by_type(&id, VIDEO_TYPE_USB, 0);
    if (id >= 0) {
        live_deviceid = id;
        return;
    }

    video_record_get_deviceid_by_type(&id, VIDEO_TYPE_CIF, 0);
    if (id >= 0) {
        live_deviceid = id;
        return;
    }

    video_record_get_deviceid_by_type(&id, VIDEO_TYPE_CIF, 1);
    if (id >= 0) {
        live_deviceid = id;
        return;
    }

    video_record_get_deviceid_by_type(&id, VIDEO_TYPE_CIF, 2);
    if (id >= 0) {
        live_deviceid = id;
        return;
    }

    video_record_get_deviceid_by_type(&id, VIDEO_TYPE_CIF, 3);
    if (id >= 0) {
        live_deviceid = id;
        return;
    }
}

void wifi_event_notify(uint32_t event, void *msg)
{
    switch (event) {
    case WIFI_EVENT_AP_STA_CONNECTED:

        break;
    case WIFI_EVENT_AP_STA_DISCONNECTED:

        break;
    case WIFI_EVENT_SCAN_RESULTS:

        break;
    case WIFI_EVENT_CONNECTED:

        break;
    case WIFI_EVENT_DISCONNECTED:

        break;
    case WIFI_EVENT_BSS_ADDED:

        break;
    case WIFI_EVENT_BSS_REMOVED:

        break;
    case WIFI_EVENT_STATE_CHANGE:

        break;
    case WIFI_EVENT_REENABLED:

        break;
    case WIFI_EVENT_TEMP_DISABLED:

        break;
    case WIFI_EVENT_OPENED:

        break;
    case WIFI_EVENT_OPEN_FAIL:

        break;
    case WIFI_EVENT_CLOSED:

        break;
    case WIFI_EVENT_CLOSE_FAIL:

        break;
    }

    return;
}

void socket_thread_create(void)
{
    if (pthread_create(&broadcast_tid, NULL, broadcast_thread, NULL) != 0)
        printf("Create thread error!\n");

    if (pthread_create(&tcp_server_tid, NULL, tcp_server_thread, NULL) != 0)
        printf("Createthread error!\n");
}

void socket_thread_close(void)
{
    int temp;
    if (broadcast_fd >= 0) {
        temp = broadcast_fd;
        broadcast_fd = -1;
        shutdown(temp, 2);
        close(temp);

    }
    if (tcp_server_fd >= 0) {
        temp = tcp_server_fd;
        tcp_server_fd = -1;
        shutdown(temp, 2);
        close(temp);
    }
#if 0
    while (link_client_num > 0) {
        if (--link_client_num == 0)
            tcp_func_stop_live(tcp_server_tab[link_client_num], NULL);

        if (tcp_server_tab[link_client_num] != -1) {
            shutdown(tcp_server_tab[link_client_num], 2);
            close(tcp_server_tab[link_client_num]);
            tcp_server_tab[link_client_num] = -1;
        }
    }
#endif
}

void rndis_management_start(void)
{
    reg_msg_manager_cb();

    if (g_rndis_is_running) {
        printf("rndis is now runnning, don't start again!\n");
        return;
    }
    g_rndis_is_running = 1;

    if (!g_wifi_is_running)
        socket_thread_create();
}
void rndis_management_stop(void)
{
    if (!g_rndis_is_running) {
        printf("rndis is not runnning, cannot stop it!\n");
        return;
    }
    g_rndis_is_running = 0;

    if (!g_wifi_is_running)
        socket_thread_close();
}

void wifi_management_start(void)
{
    reg_msg_manager_cb();
    if (g_wifi_is_running) {
        printf("Wifi is now runnning, don't start again!\n");
        return;
    }

    is_stopwifi = 0;
    g_wifi_is_running = 1;

    printf("This is wifi_management_start\n");

    WlanServiceSetPower("on");

    if (parameter_get_wifi_mode()) {
        printf("start station\n");
        wifi_deamon_thread_start(WIFI_MODE_STA);
        parameter_getwifistainfo(SSID, PASSWD);
        WlanServiceSetMode("station", SSID, PASSWD);

    } else {
        printf("start access point\n");
        parameter_getwifiinfo(SSID, PASSWD);
        WlanServiceSetMode("ap", SSID, PASSWD);
    }
    wifi_regevent(wifi_event_notify);

    set_default_live_video();
    if (!g_rndis_is_running)
        socket_thread_create();

    runapp("/usr/local/sbin/lighttpd -f /usr/local/etc/lighttpd.conf");
}

void wifi_management_stop(void)
{
    unreg_msg_manager_cb();

    if (!g_wifi_is_running) {
        printf("Wifi is not runnning, cannot stop it!\n");
        return;
    }

    g_wifi_is_running = 0;
    if (!g_rndis_is_running) {
        is_stopwifi = 1;
        socket_thread_close();
    }

    printf("stop lighttpd...!\n");
    runapp("kill -15 $(pidof lighttpd)");
    wifi_unregevent(wifi_event_notify);
    wifi_deamon_thread_stop();

    WlanServiceSetPower("off");
}

// The command must start with "CMD_".
#define CMD_PREFIX "CMD_"
const struct type_cmd_func cmd_tab[] = {
    {tcp_func_get_ssid_pw,                  "CMD_GETWIFI",},
    {tcp_func_set_ssid_pw,                  "CMD_SETWIFI",},
    {tcp_func_get_file_list,                "CMD_GETFCAMFILE",},
    {tcp_func_delete_file,                  "CMD_DELFCAMFILE",},
    {tcp_func_appear_ssid,                  "CMD_APPEARSSID",},
    {setting_cmd_resolve,                   "CMD_ARGSETTING",},
    {setting_get_cmd_resolve,               "CMD_GET_ARGSETTING",},
    {tcp_func_get_photo_quality,            "CMD_GET_PHOTO_QUALITY",},
    {tcp_func_get_setting_photo_quality,    "CMD_GET_SETTING_PHOTO_QUALITY",},
    {tcp_func_get_front_camera_apesplution, "CMD_GET_FRONT_CAMERARESPLUTION",},
    {tcp_func_get_back_camera_apesplution,  "CMD_GET_BACK_CAMERARESPLUTION",},
    {tcp_func_get_front_setting_apesplution, "CMD_GET_FRONT_SETTING_RESPLUTION",},
    {tcp_func_get_back_setting_apesplution, "CMD_GET_BACK_SETTING_RESPLUTION",},
    {tcp_func_get_format_status,            "CMD_GET_FORMAT_STATUS",},
    {tcp_func_set_on_off_recording,         "CMD_Control_Recording",},
    {tcp_func_get_on_off_recording,         "CMD_GET_Control_Recording",},
    {tcp_func_start_rtsp_live,              "CMD_RTSP_TRANS_START",},
    {tcp_func_start_ts_live,                "CMD_RTP_TS_TRANS_START",},
    {tcp_func_stop_live,                    "CMD_LIVE_STOP",},
    {tcp_func_camera_switch,                "CMD_CAMERA_CHANGE",},
    {debug_setting_cmd_resolve,             "CMD_DEBUG_ARGSETTING",},
    {debug_setting_get_cmd_resolve,         "CMD_GET_DEBUG_ARGSETTING",},
    {tcp_func_take_photo,                   "CMD_Control_Photograph",},
    {tcp_func_ota_updata,                   "CMD_OTA_Update",},
    {tcp_func_upload_file,                  "CMD_OTA_Upload_File",},
    {tcp_func_get_mode,                     "CMD_GET_MODE",},
    {tcp_func_change_mode,                  "CMD_CHANGE_MODE"},
    {tcp_func_get_gps_list,                 "CMD_GET_GPS_LIST"},
    {tcp_func_test_alive,                   "CMD_HEART"},
    {tcp_func_get_video_list,               "CMD_GET_VIDEO_LIST"},
    {tcp_func_set_live_video,               "CMD_SET_LIVE_VIDEO"},
    {tcp_func_get_live_video,               "CMD_GET_LIVE_VIDEO"},
};

static int cmd_resolve(int fd, char *cmdstr)
{
    size_t i = 0;

    printf("cmd_resolve = %s\n", cmdstr);

    for (i = 0; i < sizeof(cmd_tab) / sizeof(cmd_tab[1]); i++) {
        char *result = strstr(cmdstr, CMD_PREFIX);
        if (result != NULL) {
            size_t cmdlen = strlen(cmd_tab[i].cmd);
            if (strncmp(result, cmd_tab[i].cmd, cmdlen) == 0) {
                if (cmd_tab[i].func != NULL)
                    cmd_tab[i].func(fd, result);
                cmdstr = result + cmdlen;
                if (*cmdstr == '\0' || strstr(cmdstr, CMD_PREFIX) == NULL)
                    return 0;
                printf("handle continuous cmd: %s\n", cmdstr);
                i = 0;
            }
        }
    }
    return -1;
}
