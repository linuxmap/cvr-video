/**
 * Copyright (C) 2016 Fuzhou Rockchip Electronics Co., Ltd
 * author: huaping.liao huaping.liao@rock-chips.com
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
#include <time.h>
#include <unistd.h>

#include "common.h"
#include "parameter.h"
#include "settime.h"
#include "fs_manage/fs_storage.h"
#include "fs_manage/fs_file.h"

#include <autoconfig/main_app_autoconf.h>

#ifdef MAIN_APP_CACHE_ENCODEDATA_IN_MEM
#define COLLI_CACHE_TIME 10
#endif

#define RTC_MAX_TIME "20170101_000000"
#define RTC_MIN_TIME "20160101_000000"

int storage_get_filesize(int videotype, int filetype)
{
    size_t filesize = 0;
    int recordtime = 0;
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int bitstream = 0;
    unsigned int filesize_align = 0;
    struct video_param* front_param = parameter_get_video_frontcamera_reso();
    struct video_param* back_param = parameter_get_video_backcamera_reso();
    struct video_param* cif_param = parameter_get_video_cifcamera_reso();

    switch (videotype) {
    case VIDEO_TYPE_ISP:
        height = front_param->height;
        width = front_param->width;
        bitstream = parameter_get_bit_rate_per_pixel();
        break;
    case VIDEO_TYPE_USB:
        height = back_param->height;
        width = back_param->width;
        bitstream = parameter_get_bit_rate_per_pixel();
        break;
    case VIDEO_TYPE_CIF:
        height = cif_param->height;
        width = cif_param->width;
        bitstream = parameter_get_bit_rate_per_pixel();
        break;
    default:
        return 0;
    }

    recordtime = parameter_get_recodetime();

    /* if the record time is unlimited, do set the filesize to fs */
    if (recordtime == -1)
        return 0;

    filesize_align = fs_storage_sizealign_get_bytype(videotype, filetype);
    switch (filetype) {
    case VIDEOFILE_TYPE:
        filesize = height * width / 8 * bitstream * recordtime;
        filesize = ALIGN(filesize, filesize_align);
        break;
    case PICFILE_TYPE:
        filesize = height * width * 1.5;
        filesize = ALIGN(filesize, filesize_align);
        break;
    case LOCKFILE_TYPE:
#ifdef MAIN_APP_CACHE_ENCODEDATA_IN_MEM
        filesize = height * width / 8 * bitstream  * COLLI_CACHE_TIME;
#else
        filesize = height * width / 8 * bitstream * recordtime;
#endif
        filesize = ALIGN(filesize, filesize_align);
        break;
    case THUMBFILE_TYPE:
        filesize = THUMB_FILE_SIZE;
        break;
    default:
        return 0;
    }

    return filesize;
}

static void storage_set_filesize()
{
    int number;
    int filesize;
    FS_STORAGE_PATH *fs_storage_path;

    fs_storage_foreach_path(number, fs_storage_path) {
        /* only set video and collision size now */
        if (fs_storage_path->filetype != VIDEOFILE_TYPE &&
            fs_storage_path->filetype != LOCKFILE_TYPE)
            continue;

        filesize = storage_get_filesize(fs_storage_path->video_type,
                                        fs_storage_path->filetype);
        if (!filesize)
            continue;

        fs_storage_path->filesize = filesize;
        if (fs_storage_path->filelist)
            fs_storage_path->filelist->filesize = filesize;
        printf("Set, path=%s, size=%d MB\n", fs_storage_path->storage_path,
               filesize / 1024 / 1024);
    }
}

void storage_setting_callback(int cmd, void *msg0, void *msg1)
{
    storage_set_filesize();
}

time_t storage_get_last_time()
{
    int number;
    time_t last_time = 0;
    struct file_node *filenode = NULL;
    struct file_list *filelist = NULL;
    FS_STORAGE_PATH *fs_storage_path = NULL;

    fs_storage_foreach_path(number, fs_storage_path) {
        /* Only set video and collision size now */
        filelist = fs_storage_path->filelist;
        if (filelist == NULL)
            continue;

        filenode = fs_get_tail_filenode(filelist);

        if (filenode == NULL)
            continue;

        if (last_time < filenode->time)
            last_time = filenode->time;
    }
    printf("Get last time = %d\n", (int)last_time);
    return last_time;
}

int storage_check_timestamp()
{
    struct timeval tv;
    time_t file_time = 0;
    time_t rtc_max_time = 0;
    time_t rtc_min_time = 0;
    struct tm* local_time = NULL;

    file_time = storage_get_last_time();
    file_time++;

    string_to_time(RTC_MAX_TIME, &rtc_max_time);
    string_to_time(RTC_MIN_TIME, &rtc_min_time);
    gettimeofday(&tv, NULL);
    /*
     * If file_time > rtc_def_time, so the device haven't abnormal poweroff,
     * so don't need setDateTime. The judge just avoid customer set wrong time,
     * cause to can't restore normal time.
     */
    if (tv.tv_sec > rtc_max_time || file_time < rtc_min_time)
        return -1;

    if (file_time > tv.tv_sec) {
        local_time = localtime(&file_time);
        setDateTime(local_time);
        /* Sleep 200ms, avoid record time error */
        usleep(200000);
    }
    return 0;
}
