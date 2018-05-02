/*
 * Copyright (C) 2017 ROCKCHIP, Inc.
 * author: linyb@rock-chips.com<linyb@rock-chips.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _WIFI_API_H
#define _WIFI_API_H

#include <stdint.h>

typedef void (*tp_wifi_event_notify)(uint32_t event, void *msg);

#define WIFI_OP_BLOCK 1
#define WIFI_OP_NO_BLOCK 0

struct wifi_detail_info {
    char country_code[5];
    char country_code_enable;
    char hw_mode;
    char channel;
    char ieee80211n;
    int  beacon_int;
    char beacon_int_enable;
};

struct wifi_result {
    char mac_addr[6];
    char ssid[33];
    int channel;
    int frequency;
    int signal;
    int  flag;
};

#define FLAG_WPA_PSK_TKIP        (1<<0)
#define FLAG_WPA_PSK_TKIP_CCMP   (1<<1)
#define FLAG_WPA_PSK_CCMP        (1<<2)
#define FLAG_WPA2_PSK_TKIP       (1<<3)
#define FLAG_WPA2_PSK_TKIP_CCMP  (1<<4)
#define FLAG_WPA2_PSK_CCMP       (1<<5)

enum {
    WIFI_MODE_IDLE = 0xFF,
    WIFI_MODE_AP  = 0,
    WIFI_MODE_STA = 1,
    WIFI_MODE_STA_AP = 2,
};

enum {
    WIFI_EVENT_AP_STA_CONNECTED = 0,
    WIFI_EVENT_AP_STA_DISCONNECTED,
    WIFI_EVENT_SCAN_RESULTS,
    WIFI_EVENT_CONNECTED,
    WIFI_EVENT_DISCONNECTED,
    WIFI_EVENT_BSS_ADDED,
    WIFI_EVENT_BSS_REMOVED,
    WIFI_EVENT_STATE_CHANGE,
    WIFI_EVENT_REENABLED,
    WIFI_EVENT_TEMP_DISABLED,
    WIFI_EVENT_OPENED,
    WIFI_EVENT_OPEN_FAIL,
    WIFI_EVENT_CLOSED,
    WIFI_EVENT_CLOSE_FAIL,
};

enum {
    WIFI_POWER_STATUS_IDEL = 0,
    WIFI_POWER_STATUS_OPENING,
    WIFI_POWER_STATUS_OPENED,
    WIFI_POWER_STATUS_CLOSING,
    WIFI_POWER_STATUS_CLOSED,
};

//int WiFi_SetConfigInfo(void *pstConfigInfo);
int wifi_getconfiginfo(void *pstConfigInfo);
int wifi_switchmode(int eMode);
int wifi_getmode(int *eMode);
int wifi_regevent(tp_wifi_event_notify EventCallback);
int wifi_setssid(const char *const szSSID);
int wifi_getssid(char *szSSID);
int wifi_setpassword(const char *const szPassword);
int wifi_getpassword(char *szPassword);
int wifi_scanaroundssid(struct wifi_result *pList, int *ap_num);
int wifi_querydeviceinfo(void *pointer);
int wifi_getconnectedCount(int *ulCount);
int wifi_disconnectdevice(void *pointer);
int wifi_connectdevice(char *ssid, char *password);
int wifi_poweron(int eMode, int block);
int wifi_poweroff(int block);
int wifi_api_init(int mode);
void wifi_api_deinit(void);
int wifi_deamon_thread_start(int mode);
int wifi_unregevent(tp_wifi_event_notify eventcallback);
void wifi_deamon_thread_stop(void);

#endif
