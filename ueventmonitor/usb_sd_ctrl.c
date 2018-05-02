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

#include "usb_sd_ctrl.h"

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/mount.h>
#include <sys/stat.h>

#include "parameter.h"
#include "usb_mode.h"
#include "system.h"
#include "public_interface.h"
#include "fs_manage/fs_sdcard.h"

#define SYSFS_USB_ENABLE "/sys/class/android_usb/android0/enable"
#define SYSFS_USB_ISERIAL "/sys/class/android_usb/android0/iSerial"
#define SYSFS_USB_IPRODUCT "/sys/class/android_usb/android0/iProduct"
#define SYSFS_USB_IDVENDOR "/sys/class/android_usb/android0/idVendor"
#define SYSFS_USB_IDPRODUCT "/sys/class/android_usb/android0/idProduct"
#define SYSFS_USB_IMANUFACTURER "/sys/class/android_usb/android0/iManufacturer"
#define SYSFS_USB_FUNCTIONS "/sys/class/android_usb/android0/functions"

#define SYSFS_USB_RNDIS_MANUFACTURER "/sys/class/android_usb/android0/f_rndis/manufacturer"
#define SYSFS_USB_RNDIS_VENDORID "/sys/class/android_usb/android0/f_rndis/vendorID"
#define SYSFS_USB_RNDIS_WCEIS "/sys/class/android_usb/android0/f_rndis/wceis"

#define SYSFS_USB_MASS_INQUIRY_STRING "/sys/class/android_usb/android0/f_mass_storage/inquiry_string"

#define SYSFS_UMS_LUN0 "/sys/devices/30180000.usb/gadget/lun0/file"
#define UMS_SHARE_VOLUME "/dev/mmcblk0p1"

#define USB_ISERIAL "4U8RUG5HPF"
#define USB_IPRODUCT "RV1108"
#define USB_UMS_PRODUCTID "0000"
#define USB_ADB_PRODUCTID "0006"
#define USB_RNDIS_PRODUCTID "0007"
#define USB_UVC_PRODUCTID "0016"
#define USB_IMANUFACTURER "RockChip"

#define USB_RNDIS_MANUFACTURER "RockChip"
#define USB_VENDORID "2207"

#define USB_MASS_INQUIRY_STRING "rockchip_usb"
#define USB_MASS_CONFIGURE_STRING "mass_storage"
#define USB_ADB_CONFIGURE_STRING "adb"
#define USB_RNDIS_CONFIGURE_STRING "rndis"
#define USB_UVC_CONFIGURE_STRING "web_camera"

static short sd_mount = SD_STATE_OUT;
static int usb_mount_state;
static int conn_deep;
static int disc_deep;
void (*sdcard_event_call)(int cmd, void *msg0, void *msg1);
void (*usb_event_call)(int cmd, void *msg0, void *msg1);

void set_sd_mount_state(int state)
{
    sd_mount = state;
}

int get_sd_mount_state(void)
{
    return sd_mount;
}

int sd_reg_event_callback(void (*call)(int cmd, void *msg0, void *msg1))
{
    sdcard_event_call = call;
    return 0;
}

int usb_reg_event_callback(void (*call)(int cmd, void *msg0, void *msg1))
{
    usb_event_call = call;
    return 0;
}

static int sysfs_read(const char *path, char *str, int num_bytes)
{
    char buf[80];
    int count;
    int ret = 0;
    int fd = open(path, O_RDONLY);

    if (fd < 0) {
        strerror_r(errno, buf, sizeof(buf));
        printf("Failed to open %s: %d\n", path, fd);
        return -1;
    }

    count = read(fd, str, (num_bytes - 1));
    if (count < 0) {
        strerror_r(errno, buf, sizeof(buf));
        printf("Error reading from  %s: %s\n", path, buf);
        ret = -1;
    } else {
        str[count] = '\0';
    }
    close(fd);

    return ret;
}

static void sysfs_write(const char *path, const char *str)
{
    char buf[80];
    int len;
    int fd = open(path, O_WRONLY);

    if (fd < 0) {
        strerror_r(errno, buf, sizeof(buf));
        printf("Error opening %s: %s\n", path, buf);
        return;
    }

    if (strlen(str) == 0)
        len = write(fd, str, 1);
    else
        len = write(fd, str, strlen(str));

    if (len < 0) {
        strerror_r(errno, buf, sizeof(buf));
        printf("Error writing to %s: %s\n", path, buf);
    }

    close(fd);
}

void android_usb_config_ums(void)
{
    sysfs_write(SYSFS_USB_ENABLE, "0");
    sysfs_write(SYSFS_USB_IDVENDOR, USB_VENDORID);
    sysfs_write(SYSFS_USB_IDPRODUCT, USB_UMS_PRODUCTID);
    sysfs_write(SYSFS_USB_FUNCTIONS, USB_MASS_CONFIGURE_STRING);
    sysfs_write(SYSFS_USB_ENABLE, "1");
}

void android_usb_config_adb(void)
{
    system_fd_closexec("/usr/local/sbin/adbd &");
    sysfs_write(SYSFS_USB_ENABLE, "0");
    sysfs_write(SYSFS_USB_IDVENDOR, USB_VENDORID);
    sysfs_write(SYSFS_USB_IDPRODUCT, USB_ADB_PRODUCTID);
    sysfs_write(SYSFS_USB_FUNCTIONS, USB_ADB_CONFIGURE_STRING);
    sysfs_write(SYSFS_USB_ENABLE, "1");
}

void android_usb_config_rndis(void)
{
    sysfs_write(SYSFS_USB_ENABLE, "0");
    sysfs_write(SYSFS_USB_IDVENDOR, USB_VENDORID);
    sysfs_write(SYSFS_USB_IDPRODUCT, USB_RNDIS_PRODUCTID);
    sysfs_write(SYSFS_USB_FUNCTIONS, USB_RNDIS_CONFIGURE_STRING);
    sysfs_write(SYSFS_USB_ENABLE, "1");

    system("ifconfig lo 127.0.0.1 netmask 255.255.255.0");
    system("ifconfig rndis0 192.168.42.129 netmask 255.255.255.0 up");
}

void android_usb_config_uvc(void)
{
    sysfs_write(SYSFS_USB_ENABLE, "0");
    sysfs_write(SYSFS_USB_IDVENDOR, USB_VENDORID);
    sysfs_write(SYSFS_USB_IDPRODUCT, USB_UVC_PRODUCTID);
    sysfs_write(SYSFS_USB_FUNCTIONS, USB_UVC_CONFIGURE_STRING);
    sysfs_write(SYSFS_USB_ENABLE, "1");
}

void android_adb_keeplive(void)
{
    int ret = 0;

    ret = parameter_get_video_usb();
    if (ret == USB_MODE_ADB || ret == USB_MODE_UVC) {
        ret = runapp_result("pidof adbd");
        if (ret < 0) {
            system_fd_closexec("/usr/local/sbin/adbd &");
            printf("run  /usr/local/sbin/adbd again\n");
        }
    }
}

void android_usb_init(void)
{
    int ret = 0;

    sysfs_write(SYSFS_USB_ISERIAL, USB_ISERIAL);
    sysfs_write(SYSFS_USB_RNDIS_MANUFACTURER, USB_RNDIS_MANUFACTURER);
    sysfs_write(SYSFS_USB_RNDIS_VENDORID, USB_VENDORID);
    sysfs_write(SYSFS_USB_RNDIS_WCEIS, "1");
    sysfs_write(SYSFS_USB_IMANUFACTURER, USB_IMANUFACTURER);
    sysfs_write(SYSFS_USB_IPRODUCT, USB_IPRODUCT);
    sysfs_write(SYSFS_USB_MASS_INQUIRY_STRING, USB_MASS_INQUIRY_STRING);

    mkdir("/dev/usb-ffs", 755);
    mkdir("/dev/usb-ffs/adb", 755);
    errno = 0;
    ret = mount("adb", "/dev/usb-ffs/adb", "functionfs", 0, NULL);
    printf(" mount adb ffs ret %d, error : %s\n", ret, strerror(errno));
    sysfs_write("/sys/class/android_usb/android0/f_ffs/aliases", "adb");

    ret = parameter_get_video_usb();
    if (ret == USB_MODE_ADB || ret == USB_MODE_UVC)
        system_fd_closexec("/usr/local/sbin/adbd &");
}

void cvr_sd_ctl(char mount)
{
    int res = 0;
    int sd_exist = fs_manage_sd_exist(NULL);

    if (!sd_exist) {
        printf("%s No SD Card\n", __func__);
        sd_mount = SD_STATE_OUT;
        if (sdcard_event_call)
            (*sdcard_event_call)(SD_CHANGE, 0, (void *)0);
        return;
    }

    if (mount) {
        if (sd_mount == SD_STATE_OUT) {
            res = fs_manage_sdcard_mount();
            if (res == -1) {
                sd_mount = SD_STATE_ERR;
                if (sdcard_event_call)
                    (*sdcard_event_call)(SD_MOUNT_FAIL, 0,
                                         (void *)1);
            } else if (res == 0) {
                sd_mount = SD_STATE_IN;
                printf("mount sd success\n");
                if (sdcard_event_call)
                    (*sdcard_event_call)(SD_CHANGE, 0,
                                         (void *)1);
            } else if (res == -3) {
                sd_mount = SD_STATE_IN;
                printf("formating now\n");
            } else {
                printf("mount sd fail\n");
                sd_mount = SD_STATE_ERR;
            }
        } else {
            printf("SD Card already mount\n");
        }
    } else {
        if (sd_mount) {
            sd_mount = SD_STATE_OUT;
            if (sdcard_event_call)
                (*sdcard_event_call)(SD_CHANGE, 0, (void *)0);
            res =  fs_manage_sdcard_unmount();
            if (res)
                printf("umount /mnt failed.errno %d\n", res);
            else
                printf("umount sd success\n");
        } else {
            printf("SD Card already umount\n");
        }
    }
}

void cvr_usb_storage_ctl(char status)
{
    if (status) {
        mount("/dev/sda1", "/mnt/udisk", "vfat", 0, NULL);
    } else {
        umount2("/mnt/udisk", MNT_DETACH);
    }
}

static void share_volume(const char *path, bool share)
{
    if (share) {
        printf("share_sdcard_volume\n");
        sysfs_write(SYSFS_UMS_LUN0, path);
        disc_deep++;
        conn_deep++;
        usb_mount_state = 1;
    } else {
        printf("unshare_sdcard_volume\n");
        sysfs_write(SYSFS_UMS_LUN0, "\0");
        usb_mount_state = 0;
    }
}

static int check_usb_connection(void)
{
    int res;
    char str[20];
    const char status[] = "DISCONNECTED";
    const char path[] = "/sys/class/android_usb/android0/state";

    res = sysfs_read(path, str, 20);
    if (res < 0)
        return 1;

    if (strncmp(status, str, strlen(status)))
        return 1;

    return 0;
}

static void cvr_usb_ctl(int state)
{
    bool change = state ^ usb_mount_state;

    if (change)
        share_volume(UMS_SHARE_VOLUME, state);
}

void cvr_usb_sd_ctl(enum usb_sd_type type, char state)
{
    printf("%s: type %d state %d\n", __func__, type, state);
    printf("%s: conn_deep %d disc_deep %d\n", __func__, conn_deep,
           disc_deep);
    printf("%s: usb_mount_state %d sd_mount %d\n", __func__,
           usb_mount_state, sd_mount);

    if (type == USB_CTRL) {
        if (state) {
            if (sd_mount)
                cvr_sd_ctl(0);
            cvr_usb_ctl(1);
        } else {
            cvr_usb_ctl(0);
            if (!sd_mount)
                cvr_sd_ctl(1);
        }
    } else if (type == SD_CTRL) {
        if (state) {
            if (sd_mount == SD_STATE_OUT || sd_mount == SD_STATE_ERR)
                cvr_sd_ctl(1);
        } else {
            if (check_usb_connection()) {
                printf("The Device connect to PC\n");
                cvr_usb_ctl(0);
            }
            if (sd_mount)
                cvr_sd_ctl(0);
        }
    } else if (type == USB_STORAGE_CTRL) {
        if (state) {
            cvr_usb_storage_ctl(1);
        } else {
            cvr_usb_storage_ctl(0);
        }
    }
}

void cvr_android_usb_ctl(const struct _uevent *event)
{
    const char usb_state[] = "USB_STATE=";
    char buf_init_fuc[512] = {0};
    char *tmp = NULL;
    int i;

    sysfs_read(SYSFS_USB_FUNCTIONS, buf_init_fuc, sizeof(buf_init_fuc));

    if (!strstr(buf_init_fuc, USB_MASS_CONFIGURE_STRING))
        android_usb_config_ums();

    printf("\ncvr_android_usb_ctl:event->size = %d\n", event->size);
    for (i = 3; i < event->size; i++) {
        tmp = event->strs[i];
        /* search "USB_STATE=" */
        if (!strncmp(usb_state, tmp, strlen(usb_state)))
            break;
    }

    /* parse usb status */
    if (i < event->size) {
        tmp = strchr(tmp, '=') + 1;
        if (!strcmp(tmp, "DISCONNECTED")) {
            printf("%s: DISCONNECTED\n", __func__);
            if ((disc_deep == 1) && (conn_deep == 1)) {
                disc_deep--;
                conn_deep--;
            }
            if (disc_deep) {
                printf("%s: disc_deep\n", __func__);
                disc_deep--;
            } else if (disc_deep == 0) {
                if (usb_event_call)
                    (*usb_event_call)(0, 0, (void *)0);
            }
        } else if (!strcmp(tmp, "CONNECTED")) {
            printf("%s: CONNECTED\n", __func__);
            if (conn_deep) {
                printf("%s: conn_deep\n", __func__);
                conn_deep--;
            } else if (conn_deep == 0) {
                if (usb_event_call)
                    (*usb_event_call)(0, 0, (void *)1);
            }
        }
    }
}

bool cvr_usb_mode_switch(const struct _uevent *event)
{
    bool ret = false;
    char *tmp = NULL;
    int i;

    for (i = 3; i < event->size; i++) {
        tmp = event->strs[i];
        if (!strncmp("SET_UVC", tmp, strlen("SET_UVC"))) {
            ret = true;
            if (parameter_get_video_usb() == 0) {
                if (usb_event_call)
                    (*usb_event_call)(0, 0, (void *)0);
            }
            cmd_IDM_UVC();
            break;
        }
        if (!strncmp("SET_MSC", tmp, strlen("SET_MSC"))) {
            ret = true;
            cmd_IDM_USB();
            break;
        }
        if (!strncmp("SET_ADB", tmp, strlen("SET_ADB"))) {
            ret = true;
            if (parameter_get_video_usb() == 0) {
                if (usb_event_call)
                    (*usb_event_call)(0, 0, (void *)0);
            }
            cmd_IDM_ADB();
            break;
        }
    }

    return ret;
}
