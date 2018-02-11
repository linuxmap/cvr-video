/**
 * Copyright (C) 2016 Fuzhou Rockchip Electronics Co., Ltd
 * author: Huaping Liao <huaping.liao@rock-chips.com>
 *         Jinkun Hong <hjk@rock-chips.com>
 *         Chris Zhong <zyw@rock-chips.com>
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

#include "ueventmonitor.h"
#include "usb_mode.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/prctl.h>

#include <linux/netlink.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "parameter.h"
#include "thermal.h"
#include "usb_sd_ctrl.h"
#include "fs_manage/fs_sdcard.h"

#define DEVICENAME_LEN      (64)
#define SDCARD_DELAY_TIME   (500000)
//#define LOW_POWER_SHUTDOWN
#ifndef _PCBA_SERVICE_
//#define USB_DISCONNECT_SHUTDOWN
#endif
struct sdcard_event_t {
    int event_flag;
    char device_name[DEVICENAME_LEN];
};

static struct sdcard_event_t g_cvr_sdcard_event;
static struct itimerval timer_val;
static pthread_mutex_t collision_lock = PTHREAD_MUTEX_INITIALIZER;
static int collision_state;
static int vbus_last_status = 0;

void (*batt_event_call)(int cmd, void *msg0, void *msg1);
void (*hdmi_event_call)(int cmd, void *msg0, void *msg1);
void (*cvbsout_event_call)(int cmd, void *msg0, void *msg1);
void (*camera_event_call)(int cmd, void *msg0, void *msg1);
void (*gpios_event_call)(int cmd, void *msg0, void *msg1);

extern int gdb_usb_debug;

int batt_reg_event_callback(void (*call)(int cmd, void *msg0, void *msg1))
{
    batt_event_call = call;
    return 0;
}

int hdmi_reg_event_callback(void (*call)(int cmd, void *msg0, void *msg1))
{
    hdmi_event_call = call;
    return 0;
}

int cvbsout_reg_event_callback(void (*call)(int cmd, void *msg0, void *msg1))
{
    cvbsout_event_call = call;
    return 0;
}

int camera_reg_event_callback(void (*call)(int cmd, void *msg0, void *msg1))
{
    camera_event_call = call;
    return 0;
}

int gpios_reg_event_callback(void (*call)(int cmd, void *msg0, void *msg1))
{
    gpios_event_call = call;
    return 0;
}

static void cvr_set_collision_state(void)
{
    pthread_mutex_lock(&collision_lock);
    collision_state++;
    pthread_mutex_unlock(&collision_lock);
}

bool charging_status_check(void)
{
    const char ac_state[] = "/sys/class/power_supply/ac/online";
    const char vbus_state[] = "/sys/class/power_supply/usb/online";
    char online[3] = { 0 };
    int ac_online = 0, usb_online = 0;
    int size, fd = -1;

    fd = open(ac_state, O_RDONLY);
    if (fd >= 0) {
        size = read(fd, &online, sizeof(online));
        if (size > 0)
            ac_online = (int)atoi(online);
        close(fd);
    }

    fd = open(vbus_state, O_RDONLY);
    if (fd >= 0) {
        size = read(fd, &online, sizeof(online));
        if (size > 0)
            usb_online = (int)atoi(online);
        close(fd);
    }

    printf("%s: ac = %d, usb = %d\n", __func__, ac_online, usb_online);
    return ac_online | usb_online;
}

/*
 * Return the capability value: from 0 to 100, if there is no battery, return
 * 100 as default capability.
 */
static int battery_capability_get(void)
{
    const char batt_cap[] = "/sys/class/power_supply/battery/capacity";
    char batt[3] = { 0 };
    int size, fd = -1;
    int capability = 100;

    fd = open(batt_cap, O_RDONLY);
    if (fd < 0) {
        printf("open %s failed\n", batt_cap);
        return capability;
    }

    size = read(fd, &batt, sizeof(batt));
    if (size < 0)
        printf("read capability failed: %d\n", size);
    else
        capability = (int)atoi(batt);

    close(fd);

    return capability;
}

static void cvr_power_supply_ctl()
{
    int cmd = CMD_UPDATE_CAP;
    int capability = battery_capability_get();

    if (charging_status_check()) {
        capability = 101;
        vbus_last_status = 1;
#ifdef LOW_POWER_SHUTDOWN
    } else if (capability <= 1) {
        vbus_last_status = 0;
        printf("batt low power: shutdown\n");
        cmd = CMD_LOW_BATTERY;
#endif
#ifdef USB_DISCONNECT_SHUTDOWN
    } else if (vbus_last_status == 1) {
        vbus_last_status = 0;
        printf("usb disconnect: shutdown\n");
        cmd = CMD_DISCHARGE;
#endif
    }

    if (batt_event_call)
        (*batt_event_call)(cmd, 0, (void *)capability);
}

static void battery_init(void)
{
    cvr_power_supply_ctl();
}

static void timeout_handle(int sigo)
{
    fs_manage_set_device(g_cvr_sdcard_event.device_name);
    cvr_usb_sd_ctl(SD_CTRL, 1);
}

static void cvr_init_sigaction(void)
{
    struct sigaction act;

    act.sa_handler = timeout_handle;
    act.sa_flags   = 0;
    sigemptyset(&act.sa_mask);
    sigaction(SIGALRM, &act, NULL);
}

static void cvr_init_timer(void)
{
    timer_val.it_value.tv_sec = 0;
    timer_val.it_value.tv_usec = SDCARD_DELAY_TIME;
    setitimer(ITIMER_REAL, &timer_val, NULL);
}

static void cvr_block_ctl(const struct _uevent *event)
{
    char *tmp = NULL;
    char *act = event->strs[0] + 7;

    tmp = strrchr(event->strs[1], '/');
    if (tmp) {
        tmp = tmp + 1;
        if (strstr(tmp, "mmcblk0") != NULL) {
            if (!strcmp(act, "add")) {
                if (g_cvr_sdcard_event.event_flag == 0) {
                    cvr_init_sigaction();
                    cvr_init_timer();
                    g_cvr_sdcard_event.event_flag++;
                } else {
                    cvr_init_timer();
                    g_cvr_sdcard_event.event_flag++;
                }
                memset(g_cvr_sdcard_event.device_name, 0,
                       DEVICENAME_LEN);
                strcpy(g_cvr_sdcard_event.device_name, tmp);
            } else {
                cvr_usb_sd_ctl(SD_CTRL, 0);
            }
        } else if (strstr(tmp, "sda1") != NULL) {
            if (!strcmp(act, "add")) {
                cvr_usb_sd_ctl(USB_STORAGE_CTRL, 1);
            } else {
                cvr_usb_sd_ctl(USB_STORAGE_CTRL, 0);
            }
        }
    }
}

static void cvr_video_ctl(const struct _uevent *event)
{
    const char dev_name[] = "DEVNAME=";
    char *tmp = NULL;
    char *act = event->strs[0] + 7;
    int i, id;

    for (i = 3; i < event->size; i++) {
        tmp = event->strs[i];
        /* search "DEVNAME=" */
        if (!strncmp(dev_name, tmp, strlen(dev_name)))
            break;
    }

    if (i < event->size) {
        tmp = strchr(tmp, '=') + 1;

        if (sscanf((char *)&tmp[strlen("video")], "%d", &id) < 1) {
            printf("failed to parse video id\n");
            return;
        }

        if (!strcmp(act, "add")) {
            printf("cvr_video_ctl: add camera %c\n", id);
            if (camera_event_call)
                (*camera_event_call)(0, (void *)id, (void *)1);
        } else {
            printf("cvr_video_ctl: remove camera %c\n", id);
            if (camera_event_call)
                (*camera_event_call)(0, (void *)id, (void *)0);
        }
    }
}

static void cvr_display_ctl(const struct _uevent *event)
{
    const char itf_type[] = "INTERFACE=";
    char *tmp = NULL;
    char *act = event->strs[0] + 7;
    int i;

    for (i = 3; i < event->size; i++) {
        tmp = event->strs[i];
        /* search "INTERFACE=" */
        if (!strncmp(itf_type, tmp, strlen(itf_type)))
            break;
    }

    if (i < event->size) {
        tmp = strchr(tmp, '=') + 1;
        if (!strncmp(tmp, "HDMI", strlen(tmp))) {
            if (!strcmp(act, "add")) {
                printf("cvr_display_ctl: add hdmi\n");
                if (hdmi_event_call)
                    (*hdmi_event_call)(0, 0, (void *)1);
            } else if (!strcmp(act, "change")) {
                printf("cvr_display_ctl: change hdmi\n");
                if (hdmi_event_call)
                    (*hdmi_event_call)(0, 0, (void *)1);
            } else {
                printf("cvr_display_ctl: remove hdmi\n");
                if (hdmi_event_call)
                    (*hdmi_event_call)(0, 0, (void *)0);
            }
        } else if (!strncmp(tmp, "CVBSOUT", strlen(tmp))) {
            if (!strcmp(act, "add")) {
                printf("cvr_display_ctl: add cvbsout\n");
                if (cvbsout_event_call)
                    (*cvbsout_event_call)(0, 0, (void *)1);
            } else {
                printf("cvr_display_ctl: remove cvbsout\n");
                if (cvbsout_event_call)
                    (*cvbsout_event_call)(0, 0, (void *)0);
            }
        }
    }
}

static void cvr_thermal_ctl(const struct _uevent *event)
{
    char *act = event->strs[0] + 7;

    if (!strcmp(act, "change"))
        thermal_update_state();
}

void cvr_check_reverse_system(void)
{
    const char car_reverse[] = "/dev/vehicle";

    if (open(car_reverse, O_RDWR) < 0)
        gpios_event_call(0, (void *)"vehicle_exit", (void *)0);
}

static void cvr_gpios_ctl(const struct _uevent *event)
{
    char *name = NULL;

    if (event->size < 4) {
        printf("%s:  event->size < 4\n", __func__);
        return;
    }
    name = strstr(event->strs[3], "GPIO_NAME=");

    if (name) {
        name = name + strlen("GPIO_NAME=");
        char *stat = strstr(name, "GPIO_STATE=");
        if (stat) {
            *(stat -1) = '\0';
            stat = stat + strlen("GPIO_STATE=");
            if (!strcmp(stat, "on"))
                gpios_event_call(0, (void *)name, (void *)1);
            else if (!strcmp(stat, "over"))
                gpios_event_call(0, (void *)name, (void *)0);
            else
                printf("gpios_ctrl: %s stat %s must be on|over\n", name, stat);
        } else {
            printf("gpios_ctrl: %s is error\n", name);
        }
    }
}

/*
 * e.g uevent info
 * ACTION=change
 * DEVPATH=/devices/11050000.i2c/i2c-0/0-0012/cvr_uevent/gsensor
 * SUBSYSTEM=cvr_uevent
 * CVR_DEV_NAME=gsensor
 * CVR_DEV_TYPE=2
 */
static void parse_event(const struct _uevent *event)
{
    char *tmp = NULL;
    char *sysfs = NULL;

    if (event->size <= 0)
        return;

    sysfs = event->strs[2] + 10;
    if (!strcmp(sysfs, "power_supply")) {
        cvr_power_supply_ctl();
    } else if (!strcmp(sysfs, "block")) {
        cvr_block_ctl(event);
    } else if (!strcmp(sysfs, "android_usb")) {
        if (!gdb_usb_debug && !cvr_usb_mode_switch(event)) {
            if (parameter_get_video_usb() == 0)
                cvr_android_usb_ctl(event);
        }
    } else if (!strcmp(sysfs, "cvr_uevent")) {
        tmp = event->strs[3];
        tmp = strchr(tmp, '=') + 1;
        if (!strcmp(tmp, "gsensor"))
            cvr_set_collision_state();
    } else if (!strcmp(sysfs, "video4linux")) {
        cvr_video_ctl(event);
    } else if (!strcmp(sysfs, "display")) {
        cvr_display_ctl(event);
    } else if (!strcmp(sysfs, "platform")) {
        if (!strcmp((event->strs[3] + 7), "tsadc"))
            cvr_thermal_ctl(event);
        else if (event->size >= 5 &&
                 !strcmp(event->strs[3],
                         "vehicle_exit=done")) {
            if (!strcmp(event->strs[4], "reverse_on"))
                gpios_event_call(0, (void *)"vehicle_exit", (void *)1);
            else
                gpios_event_call(0, (void *)"vehicle_exit", (void *)0);
        }
    } else if (!strcmp(sysfs, "gpio-detection")) {
            cvr_gpios_ctl(event);
    }
}

static void *event_monitor_thread(void *arg)
{
    int sockfd;
    int i, j, len;
    char buf[512];
    struct iovec iov;
    struct msghdr msg;
    struct sockaddr_nl sa;
    struct _uevent event;

    prctl(PR_SET_NAME, "event_monitor", 0, 0, 0);

    pthread_mutex_init(&collision_lock, NULL);

    memset(&sa, 0, sizeof(sa));
    sa.nl_family = AF_NETLINK;
    sa.nl_groups = NETLINK_KOBJECT_UEVENT;
    sa.nl_pid = 0;
    memset(&msg, 0, sizeof(msg));
    iov.iov_base = (void *)buf;
    iov.iov_len = sizeof(buf);
    msg.msg_name = (void *)&sa;
    msg.msg_namelen = sizeof(sa);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    sockfd = socket(AF_NETLINK, SOCK_RAW, NETLINK_KOBJECT_UEVENT);
    if (sockfd == -1) {
        printf("socket creating failed:%s\n", strerror(errno));
        goto err_event_monitor;
    }

    if (bind(sockfd, (struct sockaddr *)&sa, sizeof(sa)) == -1) {
        printf("bind error:%s\n", strerror(errno));
        goto err_event_monitor;
    }

    if (!gdb_usb_debug) {
        if (parameter_get_video_usb() == 0)
            android_usb_config_ums();
        else if (parameter_get_video_usb() == 1)
            android_usb_config_adb();
        else if (parameter_get_video_usb() == 3)
            android_usb_config_rndis();
    }

    battery_init();
    cvr_sd_ctl(1);
    cvr_check_reverse_system();

    while (1) {
        event.size = 0;
        len = recvmsg(sockfd, &msg, 0);
        if (len < 0) {
            printf("receive error\n");
        } else if (len < 32 || len > sizeof(buf)) {
            printf("invalid message");
        } else {
            for (i = 0, j = 0; i < len; i++) {
                if (*(buf + i) == '\0' && (i + 1) != len) {
                    event.strs[j++] = buf + i + 1;
                    event.size = j;
                }
            }
        }
        parse_event(&event);
    }

err_event_monitor:
    pthread_detach(pthread_self());
    pthread_exit(NULL);
}

int uevent_monitor_run(void)
{
    pthread_t tid;

    return pthread_create(&tid, NULL, event_monitor_thread, NULL);
}
