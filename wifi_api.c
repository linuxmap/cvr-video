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

#include "wifi_api.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <asm/types.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include "wlan_service_client.h"
#include "wpa_ctrl.h"
#include "../parameter.h"
#include "../ueventmonitor/ueventmonitor.h"

#define WIFI_EVENT_CB_MAX 6
#define WIFI_OP_TIMEOUT_MS (10000)

struct wpa_ctrl {
    int s;
    struct sockaddr_un local;
    struct sockaddr_un dest;
};

static int hostapd_cli_cmd_all_sta(struct wpa_ctrl *ctrl, int argc,
                                   char *argv[]);
static int wpa_cli_cmd_scan(struct wpa_ctrl *ctrl, int argc, char *argv[],
                            char *rsp_buf);

static int wpa_cli_cmd_scan_results(struct wpa_ctrl *ctrl, int argc,
                                    char *argv[], char *rsp_buf);
static int wpa_cli_cmd_disconnect(struct wpa_ctrl *ctrl, int argc,
                                  char *argv[], char *rsp_buf);
static int hostapd_cli_cmd_disassociate(struct wpa_ctrl *ctrl, int argc,
                                        char *argv[], char *rsp_buf);
int wifi_deamon_thread_start(int mode);
void wifi_deamon_thread_stop(void);

static struct wpa_ctrl *hostapd_ctrl_conn;
static struct wpa_ctrl *hostapd_event_conn;
static struct wpa_ctrl *wpa_ctrl_conn;
static struct wpa_ctrl *wpa_event_conn;
static pthread_t wifi_hostapddeamon_pid;
static pthread_t wifi_wpadeamon_pid;
static int all_sta_cnt;
static int all_sta_cnt2;
static int wifi_api_init_flag = 0;
static int current_mode = WIFI_MODE_IDLE;
static int wifi_power_status = WIFI_POWER_STATUS_IDEL;
tp_wifi_event_notify wifi_event_cb[WIFI_EVENT_CB_MAX] = {0};
struct wifi_detail_info wifi_detailinfo;
static sem_t wifi_scan_sem;

static void sleep_ms(int ms)
{
    usleep(ms * 1000);
}

int wifi_getconfiginfo(void *pstconfiginfo)
{
    memcpy(pstconfiginfo, &wifi_detailinfo, sizeof(struct wifi_detail_info));
    return 0;
}

int wifi_switchmode(int emode)
{
    char ssid[33];
    char passwd[65];

    if (current_mode == emode) {
        printf("wifi is already in %d mode \n", emode);
        return -1;
    }

    if ((current_mode != WIFI_MODE_IDLE) && (current_mode != emode)) {
        wifi_poweroff(WIFI_OP_BLOCK);
        wifi_poweron(emode, WIFI_OP_BLOCK);
    }

    if (emode == WIFI_MODE_STA) {
        printf("start station\n");
        parameter_getwifistainfo(ssid, passwd);
        WlanServiceSetMode("station", ssid, passwd);
    } else if (emode == WIFI_MODE_AP) {
        printf("start access point\n");
        parameter_getwifiinfo(ssid, passwd);
        WlanServiceSetMode("ap", ssid, passwd);
        system("ifconfig wlan0 192.168.100.1 netmask 255.255.255.0");
    } else if (emode == WIFI_MODE_STA_AP) {
        return -1;
    } else {
        printf("wifi mode unsupport\n");
        return -1;
    }
    current_mode = emode;
    parameter_sav_wifi_mode(emode);

    return 0;
}
int wifi_getmode(int *emode)
{
    int mode;

    mode = parameter_get_wifi_mode();
    *emode = mode;
    return mode;
}
int wifi_regevent(tp_wifi_event_notify eventcallback)
{
    int i;

    for (i = 0; i < WIFI_EVENT_CB_MAX; i++) {
        if (wifi_event_cb[i] == 0) {
            wifi_event_cb[i] = eventcallback;
            return 0;
        }
    }

    return -1;
}

int wifi_unregevent(tp_wifi_event_notify eventcallback)
{
    int i;

    for (i = 0; i < WIFI_EVENT_CB_MAX; i++) {
        if (wifi_event_cb[i] == eventcallback) {
            wifi_event_cb[i] = 0;
            return 0;
        }
    }

    return -1;
}

void wifi_event_dispatch(uint32_t event, void *msg)
{
    int i;

    for (i = 0; i < WIFI_EVENT_CB_MAX; i++) {
        if (wifi_event_cb[i])
            wifi_event_cb[i](event, msg);
    }
}

int wifi_setssid(const char *const szssid)
{
    return 0;
}

int wifi_getssid(char *szssid)
{
    char passwd[65];

    parameter_getwifiinfo(szssid, passwd);

    return 0;
}

int wifi_setpassword(const char *const szpassword)
{
    return 0;
}

int wifi_getpassword(char *szpassword)
{
    char ssid[33];

    parameter_getwifiinfo(ssid, szpassword);

    return 0;
}

static unsigned int _atoh(const char *hexstr, size_t hexlen)
{
    unsigned int ua_value = 0U;
    unsigned int uh_value = 0U;

    size_t temp_size = hexlen;
    const char *tmpstr = hexstr;

    while (temp_size--) {
        if (*tmpstr > 0x2F && *tmpstr < 0x3A)
            ua_value = *tmpstr - 0x30;
        else if (*tmpstr > 0x60 && *tmpstr < 0x67)
            ua_value = *tmpstr - 0x57;
        else if (*tmpstr > 0x40 && *tmpstr < 0x47)
            ua_value = *tmpstr - 0x37;
        else
            return 0U;

        uh_value += ua_value << 4U * temp_size;
        ++tmpstr;
    }

    return uh_value;
}

static void parse_ssid(char *input, struct wifi_result *output)
{
    memcpy(output->ssid, input, strlen(input));

    return;
}

static void parse_signal(char *input, struct wifi_result *output)
{
    output->signal = atoi(input);
}

static void parse_frequency(char *input, struct wifi_result *output)
{
    output->frequency  = atoi(input);
}

static int parse_macaddr(char *input, struct wifi_result *output)
{
    char *p;
    int i;
    if ((p = strtok(input, ":")) == NULL)
        return -1;
    output->mac_addr[0] = _atoh(p, 2);
    i = 1;
    while ((p = strtok(NULL, ":"))) {
        output->mac_addr[i++] = _atoh(p, 2);
        if (i == 6)
            break;
    }

    return 0;
}

static int parse_one_scan_result(char *input, struct wifi_result *output)
{
    char *p;
    int index = 0;
    if ((p = strtok(input, "\t")) == NULL)
        return -1;
    index += strlen(p) + 1;
    if (parse_macaddr(p, output))
        return -1;

    if ((p = strtok(input + index, "\t")) == NULL)
        return -1;

    index += strlen(p) + 1;
    parse_frequency(p, output);

    if ((p = strtok(input + index, "\t")) == NULL)
        return -1;
    index += strlen(p) + 1;
    parse_signal(p, output);

    if ((p = strtok(input + index, "\t")) == NULL)
        return -1;

    index += strlen(p) + 1;
    if ((p = strtok(input + index, "\t")) == NULL)
        return -1;
    index += strlen(p) + 1;
    parse_ssid(p, output);

    return 0;
}

static void print_result(struct wifi_result *result)
{
    printf("get one result\n");
    printf("mac addr :0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x\n",
           result->mac_addr[0], result->mac_addr[1], result->mac_addr[2],
           result->mac_addr[3], result->mac_addr[4], result->mac_addr[5]);
    printf("frequency = %d\n", result->frequency);
    printf("signal = %d\n", result->signal);
    printf("ssid = %s\n", result->ssid);
}

static int parse_results(char *input, void *output, int *output_num)
{
    struct wifi_result *results = output;
    char *line;
    int i = 0;
    int index = 0;
    int ap_num = 0;

    if (output == NULL)
        return -1;

    line = strtok(input, "\n");
    printf("the frist line :%s", line);
    index += strlen(line) + 1;
    while ((line = strtok(input + index, "\n"))) {
        index += strlen(line) + 1;
        parse_one_scan_result(line, &results[i++]);
        ap_num++;
        print_result(&results[i - 1]);
    }
    *output_num = ap_num;

    return 0;
}

int wifi_scanaroundssid(struct wifi_result *pList, int *ap_num)
{
    struct wpa_ctrl *ctrl;
    int Mode;
    char *results_buf;

    if (pList == NULL)
        return -1;

    if ((results_buf = malloc(16 * 1024)) == NULL)
        return -1;

    if (wifi_getmode(&Mode) == WIFI_MODE_STA)
        ctrl = wpa_ctrl_conn;
    else
        ctrl = hostapd_ctrl_conn;

    wpa_cli_cmd_scan(ctrl, 0, NULL, NULL);
    sem_wait(&wifi_scan_sem);
    wpa_cli_cmd_scan_results(ctrl, 0, NULL, results_buf);
    *ap_num = 0;
    parse_results(results_buf, pList, ap_num);
    free(results_buf);

    return 0;
}

int wifi_querydeviceinfo(void *pointer)
{
    struct wifi_result *result;
    int ap_num;
    int i;

    if ((result = malloc(sizeof(struct wifi_result) * 200)) == NULL)
        return -1;
    wifi_scanaroundssid(result, &ap_num);
    for (i = 0; i < ap_num; i++) {
        if (memcmp(result[i].mac_addr, pointer, 6) == 0)
            return 0;
    }

    return -1;
}

int wifi_getconnectedcount(int *ulCount)
{
    hostapd_cli_cmd_all_sta(hostapd_ctrl_conn, 0, NULL);

    return all_sta_cnt;
}

int wifi_disconnectdevice(void *pointer)
{
    char *mac;
    int Mode;
    int ret = -1;

    mac = pointer;
    if (wifi_getmode(&Mode) == WIFI_MODE_STA) {
        ret = wpa_cli_cmd_disconnect(wpa_ctrl_conn, 0, NULL, NULL);
    } else if (wifi_getmode(&Mode) == WIFI_MODE_AP) {
        char addr_buf[32] = {0};
        snprintf(addr_buf, sizeof(addr_buf), "%02x:%02x:%02x:%02x:%02x:%02x", mac[0],
                 mac[1], mac[2], mac[3], mac[4], mac[5]);
        ret = hostapd_cli_cmd_disassociate(hostapd_ctrl_conn, 1,
                                           (char **)addr_buf, NULL);
    }
    return ret;
}

int wifi_setstaconnectdevice(char *ssid, char *password)
{
    parameter_sav_staewifiinfo(ssid, password);
    return 0;
}

int wifi_connectdevice(char *ssid, char *password)
{
    parameter_sav_staewifiinfo(ssid, password);
    wifi_switchmode(WIFI_MODE_STA);
    return 0;
}

int wifi_setconnectdevice(char *ssid, char *password)
{
    parameter_sav_staewifiinfo(ssid, password);
    return 0;
}

static void *poweron_check_thread(void *arg)
{
    int timeout_ms = WIFI_OP_TIMEOUT_MS;
    prctl(PR_SET_NAME, __func__, 0, 0, 0);

    while (access("/sys/class/net/wlan0", F_OK)) {
        sleep_ms(100);
        timeout_ms -= 100;
        if (timeout_ms == 0) {
            wifi_event_dispatch(WIFI_EVENT_OPEN_FAIL, NULL);
            wifi_power_status = WIFI_POWER_STATUS_IDEL;
            printf("wifi poweron timeout\n");
            return NULL;
        }
    }

    wifi_event_dispatch(WIFI_EVENT_OPENED, NULL);
    wifi_power_status = WIFI_POWER_STATUS_OPENED;

    return NULL;
}

static void start_poweron_check(void)
{
    pthread_t thread_id;

    pthread_create(&thread_id, NULL, poweron_check_thread, NULL);
}

int wifi_poweron(int mode, int block)
{
    int timeout_ms = WIFI_OP_TIMEOUT_MS;

    if ((wifi_power_status != WIFI_POWER_STATUS_IDEL) &&
        (wifi_power_status != WIFI_POWER_STATUS_CLOSED)) {
        printf("wifi operation is busy!\n");
        return -1;
    }

    wifi_power_status = WIFI_POWER_STATUS_OPENING;
    WlanServiceSetPower("on");
    while (block && access("/sys/class/net/wlan0", F_OK)) {
        sleep_ms(100);
        timeout_ms -= 100;
        if (timeout_ms == 0) {
            wifi_event_dispatch(WIFI_EVENT_OPEN_FAIL, NULL);
            wifi_power_status = WIFI_POWER_STATUS_IDEL;
            printf("wifi poweron timeout\n");
            return -1;
        }
    }

    if (block) {
        wifi_event_dispatch(WIFI_EVENT_OPENED, NULL);
        wifi_power_status = WIFI_POWER_STATUS_OPENED;
    } else
        start_poweron_check();

    if (wifi_deamon_thread_start(mode)) {
        WlanServiceSetPower("off");
        return -1;
    }
    return 0;
}

static void *poweroff_check_thread(void *arg)
{
    int timeout_ms = WIFI_OP_TIMEOUT_MS;
    prctl(PR_SET_NAME, __func__, 0, 0, 0);

    while (access("/sys/class/net/wlan0", F_OK) == 0) {
        sleep_ms(100);
        timeout_ms -= 100;
        if (timeout_ms == 0) {
            wifi_event_dispatch(WIFI_EVENT_CLOSE_FAIL, NULL);
            wifi_power_status = WIFI_POWER_STATUS_OPENED;
            printf("wifi poweroff timeout\n");
            return NULL;
        }
    }
    wifi_power_status = WIFI_POWER_STATUS_CLOSED;
    wifi_event_dispatch(WIFI_EVENT_CLOSED, NULL);

    return NULL;
}

static void start_poweroff_check(void)
{
    pthread_t thread_id;

    pthread_create(&thread_id, NULL, poweroff_check_thread, NULL);
}

int wifi_poweroff(int block)
{
    int timeout_ms = WIFI_OP_TIMEOUT_MS;

    if (wifi_power_status != WIFI_POWER_STATUS_OPENED) {
        printf("wifi has not be opened\n");
        return -1;
    }

    wifi_power_status = WIFI_POWER_STATUS_CLOSING;
    WlanServiceSetPower("off");
    while (block && access("/sys/class/net/wlan0", F_OK) == 0) {
        sleep_ms(100);
        timeout_ms -= 100;
        if (timeout_ms == 0) {
            wifi_event_dispatch(WIFI_EVENT_CLOSE_FAIL, NULL);
            wifi_power_status = WIFI_POWER_STATUS_OPENED;
            printf("wifi poweroff timeout\n");
            return -1;
        }

    }

    if (block) {
        wifi_power_status = WIFI_POWER_STATUS_CLOSED;
        wifi_event_dispatch(WIFI_EVENT_CLOSED, NULL);
    } else
        start_poweroff_check();

    wifi_deamon_thread_stop();

    return 0;
}

static struct wpa_ctrl *hostapd_cli_open_connection(const char *ifname)
{
    struct wpa_ctrl *ctrl;

    ctrl = wpa_ctrl_open("/tmp/hostapd/wlan0");

    return ctrl;
}

static struct wpa_ctrl *wpa_cli_open_connection(const char *ifname)
{
    struct wpa_ctrl *ctrl;

    ctrl = wpa_ctrl_open("/var/run/wpa_supplicant/wlan0");

    return ctrl;
}

static void hostapd_cli_close_connection(struct wpa_ctrl *ctrl)
{
    if (ctrl == NULL)
        return;

    wpa_ctrl_close(ctrl);
}

static void hostapd_cli_msg_cb(char *msg, size_t len)
{
    printf("%s\n", msg);
}

static int _wpa_ctrl_command(struct wpa_ctrl *ctrl, char *cmd, int print,
                             char *rsp_buf)
{
    char buf_temp[4096];
    char *buf;
    size_t len;
    int ret;

    if (ctrl == NULL) {
        printf("Not connected to hostapd - command dropped.\n");
        return -1;
    }
    buf = (rsp_buf) ? rsp_buf : buf_temp;
    len = sizeof(buf_temp) - 1;
    ret = wpa_ctrl_request(ctrl, cmd, strlen(cmd), buf, &len,
                           hostapd_cli_msg_cb);
    if (ret == -2) {
        printf("'%s' command timed out.\n", cmd);
        return -2;
    } else if (ret < 0) {
        printf("'%s' command failed.\n", cmd);
        return -1;
    }
    if (print) {
        buf[len] = '\0';
        printf("%s", buf);
    }

    return 0;
}

static inline int wpa_ctrl_command(struct wpa_ctrl *ctrl, char *cmd, char *buf)
{
    return _wpa_ctrl_command(ctrl, cmd, 1, buf);
}

static int wpa_ctrl_command_sta(struct wpa_ctrl *ctrl, char *cmd,
                                char *addr, size_t addr_len)
{
    char buf[4096], *pos;
    size_t len;
    int ret;

    if (ctrl == NULL) {
        printf("Not connected to hostapd - command dropped.\n");
        return -1;
    }
    len = sizeof(buf) - 1;
    ret = wpa_ctrl_request(ctrl, cmd, strlen(cmd), buf, &len,
                           hostapd_cli_msg_cb);
    if (ret == -2) {
        printf("'%s' command timed out.\n", cmd);
        return -2;
    } else if (ret < 0) {
        printf("'%s' command failed.\n", cmd);
        return -1;
    }

    buf[len] = '\0';
    if (memcmp(buf, "FAIL", 4) == 0)
        return -1;
    printf("%s", buf);

    pos = buf;
    while (*pos != '\0' && *pos != '\n')
        pos++;
    *pos = '\0';
    strlcpy(addr, buf, addr_len);

    return 0;
}

static int hostapd_cli_cmd_all_sta(struct wpa_ctrl *ctrl, int argc,
                                   char *argv[])
{
    char addr[32], cmd[64];

    all_sta_cnt = 0;
    if (wpa_ctrl_command_sta(ctrl, "STA-FIRST", addr, sizeof(addr)))
        return 0;
    do {
        snprintf(cmd, sizeof(cmd), "STA-NEXT %s", addr);
        all_sta_cnt++;
    } while (wpa_ctrl_command_sta(ctrl, cmd, addr, sizeof(addr)) == 0);

    return -1;
}

static int hostapd_cli_cmd_disassociate(struct wpa_ctrl *ctrl, int argc,
                                        char *argv[], char *rsp_buf)
{
    char buf[64];

    if (argc < 1) {
        printf("Invalid 'disassociate' command - exactly one "
               "argument, STA address, is required.\n");
        return -1;
    }
    if (argc > 1)
        snprintf(buf, sizeof(buf), "DISASSOCIATE %s %s",
                 argv[0], argv[1]);
    else
        snprintf(buf, sizeof(buf), "DISASSOCIATE %s", argv[0]);

    return wpa_ctrl_command(ctrl, buf, rsp_buf);
}

static int wpa_cli_cmd_scan(struct wpa_ctrl *ctrl, int argc, char *argv[],
                            char *rsp_buf)
{
    return wpa_ctrl_command(ctrl, "SCAN", rsp_buf);
}

static int wpa_cli_cmd_scan_results(struct wpa_ctrl *ctrl, int argc,
                                    char *argv[], char *rsp_buf)
{
    return wpa_ctrl_command(ctrl, "SCAN_RESULTS", rsp_buf);
}

static int wpa_cli_cmd_disconnect(struct wpa_ctrl *ctrl, int argc,
                                  char *argv[], char *rsp_buf)
{
    return wpa_ctrl_command(ctrl, "DISCONNECT", rsp_buf);
}

static int check_udhcpc_isup()
{
    char buf[128];
    int fd;

    memset(buf, 0 , sizeof(buf));
    system("ps | grep \"udhcpc\" > /tmp/tmp_udhcpc.tmp");
    fd = open("/tmp/tmp_udhcpc.tmp", O_RDONLY);
    if (fd < 0) {
        system("rm /tmp/tmp_udhcpc.tmp");

        return 0;
    }
    read(fd, buf, sizeof(buf) - 1);
    close(fd);
    system("rm /tmp/tmp_udhcpc.tmp");

    if (strstr(buf, "/sbin/udhcpc"))
        return 1;

    return 0;
}

static int udhcpc_up(void)
{
    if (check_udhcpc_isup() == 0)
        system("/sbin/udhcpc -i wlan0 -b");

    return 0;
}

static int route_up(void)
{
    static int is_up = 0;

    if (is_up == 0) {
        is_up = 1;
        system("route add -net 224.0.0.0 netmask 240.0.0.0 dev wlan0");
    }
    return 0;
}

void *wifi_hostapd_deamon_thread(void *arg)
{
    char buf[4096];
    size_t recv_len;
    int timeout_ms = 20000;
    prctl(PR_SET_NAME, __func__, 0, 0, 0);
    while (1) {
        hostapd_ctrl_conn = hostapd_cli_open_connection(NULL);
        hostapd_event_conn = hostapd_cli_open_connection(NULL);
        sleep_ms(50);
        if (hostapd_ctrl_conn) {
            printf("connect hostapd ctrl ok\n");
            break;
        }
        timeout_ms -= 50;
        if (timeout_ms == 0)
            return NULL;
    }

    if (hostapd_event_conn) {
        wpa_ctrl_attach(hostapd_event_conn);
        printf("connect hostapd event ok\n");
    } else {
        printf("warning host event channel can't attach\n");
        return NULL;
    }
    while (1) {
        memset(buf, 0, 4096);
        if (wpa_ctrl_recv(hostapd_event_conn, buf, &recv_len) == 0) {
            if (recv_len) {
                printf("get hostapd_ctrl len = %d\n", recv_len);
                printf("%s\n", buf);
            }
            if (strstr(buf, AP_STA_CONNECTED)) {
                all_sta_cnt2++;
                route_up();
                udhcpc_up();
                wifi_event_dispatch(WIFI_EVENT_AP_STA_CONNECTED, NULL);
                printf("WIFI_EVENT_AP_STA_CONNECTED\n");
            } else if (strstr(buf, AP_STA_DISCONNECTED)) {
                all_sta_cnt2--;
                wifi_event_dispatch(WIFI_EVENT_AP_STA_DISCONNECTED, NULL);
                printf("WIFI_EVENT_AP_STA_DISCONNECTED\n");
            } else if (strstr(buf, WPA_EVENT_SCAN_RESULTS)) {
                wifi_event_dispatch(WIFI_EVENT_SCAN_RESULTS, NULL);
                sem_post(&wifi_scan_sem);
                printf("hostapd WIFI_EVENT_SCAN_RESULTS\n");
            }
        }
    }

    return NULL;
}

void *wifi_wpa_deamon_thread(void *arg)
{
    char buf[512];
    size_t recv_len;
    int timeout_ms = 20000;
    prctl(PR_SET_NAME, __func__, 0, 0, 0);
    while (1) {
        wpa_ctrl_conn = wpa_cli_open_connection(NULL);
        wpa_event_conn = wpa_cli_open_connection(NULL);
        sleep_ms(50);
        if (wpa_ctrl_conn) {
            printf("connect wpa ctrl ok\n");
            break;
        }
        timeout_ms -= 50;
        if (timeout_ms == 0)
            return NULL;
    }

    if (wpa_event_conn) {
        if (wpa_ctrl_attach(wpa_event_conn))
            printf("wpa_ctrl_attach fail\n");
        printf("connect wpa event ok\n");
    }

    while (1) {
        memset(buf, 0, 512);
        wpa_ctrl_pending(wpa_event_conn);
        recv_len = 512;
        if (wpa_ctrl_recv(wpa_event_conn, buf, &recv_len) == 0) {
            if (recv_len) {
                printf("%s get wpa_ctrl len = %d\n",
                       __FUNCTION__, recv_len);
                printf("%s\n", buf);
            }

            if (strstr(buf, WPA_EVENT_SCAN_RESULTS)) {
                wifi_event_dispatch(WIFI_EVENT_SCAN_RESULTS, NULL);
                sem_post(&wifi_scan_sem);
                printf("WIFI_EVENT_SCAN_RESULTS\n");
            } else if (strstr(buf, WPA_EVENT_CONNECTED)) {
                route_up();
                udhcpc_up();
                wifi_event_dispatch(WIFI_EVENT_CONNECTED, NULL);
                printf("WIFI_EVENT_CONNECTED\n");
            } else if (strstr(buf, WPA_EVENT_DISCONNECTED)) {
                wifi_event_dispatch(WIFI_EVENT_DISCONNECTED, NULL);
                printf("WIFI_EVENT_DISCONNECTED\n");
            } else if (strstr(buf, WPA_EVENT_BSS_ADDED)) {
                wifi_event_dispatch(WIFI_EVENT_BSS_ADDED, NULL);
            } else if (strstr(buf, WPA_EVENT_BSS_REMOVED)) {
                wifi_event_dispatch(WIFI_EVENT_BSS_REMOVED, NULL);
            } else if (strstr(buf, WPA_EVENT_STATE_CHANGE)) {
                wifi_event_dispatch(WIFI_EVENT_STATE_CHANGE, NULL);
            } else if (strstr(buf, WPA_EVENT_REENABLED)) {
                wifi_event_dispatch(WIFI_EVENT_REENABLED, NULL);
            } else if (strstr(buf , WPA_EVENT_TEMP_DISABLED)) {
                wifi_event_dispatch(WIFI_EVENT_TEMP_DISABLED, NULL);
            }
        }
    }

    return NULL;
}

int wifi_deamon_thread_start(int mode)
{
    if (wifi_api_init_flag == 0) {
        wifi_hostapddeamon_pid = 0;
        wifi_wpadeamon_pid = 0;
        all_sta_cnt2 = 0;
        sem_init(&wifi_scan_sem, 0 , 0);
        if (mode == WIFI_MODE_AP || mode == WIFI_MODE_STA_AP) {
            if (pthread_create(&wifi_hostapddeamon_pid, NULL,
                               wifi_hostapd_deamon_thread, NULL))
                return -1;
        }

        if (mode == WIFI_MODE_STA || mode == WIFI_MODE_STA_AP) {
            if (pthread_create(&wifi_wpadeamon_pid, NULL,
                               wifi_wpa_deamon_thread, NULL)) {
                if (wifi_hostapddeamon_pid)
                    pthread_cancel(wifi_hostapddeamon_pid);
                wifi_hostapddeamon_pid = 0;
                return -1;
            }
        }

        wifi_api_init_flag = 1;
        return 0;
    }

    return 0;
}

void wifi_deamon_thread_stop(void)
{
    if (wifi_api_init_flag == 1) {
        if (wifi_hostapddeamon_pid)
            pthread_cancel(wifi_hostapddeamon_pid);

        if (wifi_wpadeamon_pid)
            pthread_cancel(wifi_wpadeamon_pid);

        hostapd_cli_close_connection(wpa_ctrl_conn);
        hostapd_cli_close_connection(wpa_event_conn);
        hostapd_cli_close_connection(hostapd_ctrl_conn);
        hostapd_cli_close_connection(hostapd_event_conn);
        wpa_ctrl_conn = 0;
        wpa_event_conn = 0;
        hostapd_ctrl_conn = 0;
        hostapd_event_conn = 0;
        wifi_hostapddeamon_pid = 0;
        wifi_wpadeamon_pid = 0;
        all_sta_cnt2 = 0;
        wifi_api_init_flag = 0;
        sem_destroy(&wifi_scan_sem);
    }
}
