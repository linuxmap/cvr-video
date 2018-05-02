/**
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd
 * Author: Chad.ma <chad.ma@rock-chips.com>
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

#include "dv_play.h"

#include <string.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <unistd.h>

#include "dv_main.h"
#include "dv_setting.h"
#include "parameter.h"
#include "public_interface.h"
#include "videoplay.h"
#include "wifi_management.h"
#include "fs_manage/fs_storage.h"
#include "ueventmonitor/usb_sd_ctrl.h"

#define FILENAME_LEN 128

char previewname[FILENAME_LEN];
int preview_isvideo = 0;
static int videoplay = 0;
static struct file_list *filelist;
static struct file_node *currentfile;
static int totalfilenum = 0;
static int currentfilenum = 0;

extern void *dlg_content;

const char *video_text[LANGUAGE_NUM][TEXT_VIDEO_MAX] = {
    {
        "Delete this video ?",
        "Viedo error !!!",
    },
    {
        "É¾³ý´ËÊÓÆµ?",
        "ÊÓÆµ´íÎó !!!",
    },
};

int videopreview_get_state(void)
{
    return videoplay;
}

static void videopreview_show_decode(char *pname,
                                     char *videoname,
                                     char *filename)
{
    char suffix_fmt[8];
    videoplay = 0;

    if (pname == NULL)
        return;

    preview_isvideo = 0;
    sprintf(pname, "%s(%d/%d)", videoname, currentfilenum, totalfilenum);
    DBG("file = %s, pname = %s\n", filename, pname);
    snprintf(suffix_fmt, sizeof(suffix_fmt), ".%s",
             parameter_get_storage_format());

    if (strstr(pname, suffix_fmt)) {
        DBG("videoplay_decode_one_frame(%s)\n", filename);
        preview_isvideo = 1;
        videoplay = videoplay_decode_one_frame(filename);
        DBG("videoplay= %d\n", videoplay);

        if (videoplay != 0) {
            int lang = parameter_get_video_lan();
            printf("videoplay==-1\n");
            dlg_content =(void *)video_text[lang][TEXT_VIDEO_ERR];
            dv_create_message_dialog(title_text[lang][TITLE_WARNING]);
        }
    } else if (strstr(pname, ".jpg")) {
        DBG("videoplay_decode_jpeg(%s)\n", filename);
        videoplay_decode_jpeg(filename);
    }
}

static void videopreview_getfilename(char *filename, struct file_node *filenode)
{
    char *filepath = NULL;

    if (filenode == NULL)
        return;

    filepath = fs_storage_folder_get_bynode(filenode);
    snprintf(filename, FILENAME_LEN, "%s/%s", filepath, filenode->name);
}

void entervideopreview(void)
{
    char filename[FILENAME_LEN];

    totalfilenum = 0;
    currentfilenum = 0;
    preview_isvideo = 0;
    videoplay = 0;
    if (api_get_sd_mount_state() == SD_STATE_OUT) {
        sprintf(previewname, "%s", "no sd card");
        return;
    }

    runapp("sync");  // sync when showing
    filelist = fs_manage_getmediafilelist();

    if (filelist == NULL) {
        sprintf(previewname, "(%d/%d)", currentfilenum, totalfilenum);
        return;
    }

    currentfile = filelist->file_tail;

    if (currentfile == NULL) {
        sprintf(previewname, "(%d/%d)", currentfilenum, totalfilenum);
        return;
    }

    totalfilenum = filelist->filenum;

    if (totalfilenum)
        currentfilenum = 1;

    currentfile = filelist->file_tail;

    videopreview_getfilename(filename, currentfile);
    videopreview_show_decode(previewname, currentfile->name, filename);

    dv_preview_state = PREVIEW_IN;
}

void videopreview_refresh(HWND hWnd)
{
    char filename[FILENAME_LEN];

    if (currentfile == NULL)
        return;

    videopreview_getfilename(filename, currentfile);
    videopreview_show_decode(previewname, currentfile->name, filename);
    InvalidateRect(hWnd, NULL, FALSE);
}

void videopreview_next(HWND hWnd)
{
    char filename[FILENAME_LEN];

    if (currentfile == NULL)
        return;

    if (currentfile->pre == NULL) {
        currentfile = filelist->file_tail;

        if (currentfile == NULL)
            return;

        currentfilenum = 1;
    } else {
        currentfile = currentfile->pre;
        currentfilenum++;
    }

    videopreview_getfilename(filename, currentfile);
    videopreview_show_decode(previewname, currentfile->name, filename);
    InvalidateRect(hWnd, NULL, FALSE);
}

void videopreview_pre(HWND hWnd)
{
    char filename[FILENAME_LEN];

    if (currentfile == NULL)
        return;

    if (currentfile->next == NULL) {
        currentfile = filelist->file_head;

        if (currentfile == NULL)
            return;

        currentfilenum = totalfilenum;
    } else {
        currentfile = currentfile->next;
        currentfilenum--;
    }

    if (currentfilenum < 0)
        currentfilenum = 0;

    videopreview_getfilename(filename, currentfile);
    videopreview_show_decode(previewname, currentfile->name, filename);
    InvalidateRect(hWnd, NULL, FALSE);
}

void videopreview_play(HWND hWnd)
{
    char filename[FILENAME_LEN];
    char suffix_fmt[8];

    if (currentfile == NULL)
        return;

    videopreview_getfilename(filename, currentfile);
    snprintf(suffix_fmt, sizeof(suffix_fmt), ".%s",
             parameter_get_storage_format());

    if (strstr(currentfile->name, suffix_fmt)) {
        DBG("videoplay_decode_one_frame(%s)\n", filename);
        startplayvideo(hWnd, filename);
    }
}

int videopreview_delete(HWND hWnd)
{
    char filename[FILENAME_LEN];
    struct file_node *current_filenode;

    if (currentfile == NULL)
        return -1;

    current_filenode = currentfile;

    if (currentfile->pre == NULL) {
        currentfile = currentfile->next;
        currentfilenum--;
    } else
        currentfile = currentfile->pre;

    totalfilenum--;

    videopreview_getfilename(filename, current_filenode);
    fs_storage_remove(filename, 1); //delete select file.
    sync();
    videoplay = 0;
    preview_isvideo = 0;

    if (currentfile == NULL) {
        sprintf(previewname, "(%d/%d)", currentfilenum, totalfilenum);
        printf("%s previewname = %s\n", __func__, previewname);
        videoplay_set_fb_black();
        goto out;
    }

    videopreview_getfilename(filename, currentfile);
    videopreview_show_decode(previewname, currentfile->name, filename);
out:
    InvalidateRect(hWnd, NULL, FALSE);
    return 0;
}

void startplayvideo(HWND hWnd, char *filename)
{
    int ret;

    DBG("filename = %s\n", filename);
    api_change_mode(MODE_PLAY);
    ret = videoplay_init(filename);

    /* Videoplay init success. */
    if (ret != -1)
        dv_play_state = VIDEO_PLAYING;
}

void exitvideopreview(void)
{
    fs_manage_free_mediafilelist();
    filelist = 0;
    currentfile = 0;

    totalfilenum = 0;
    currentfilenum = 0;
    preview_isvideo = 0;
    videoplay = 0;

    if (api_get_sd_mount_state() == SD_STATE_OUT)
        sprintf(previewname, "%s", "no sd card");

    videoplay_set_fb_black();
    dv_preview_state = PREVIEW_OUT;
}

void exitplayvideo(HWND hWnd)
{
    videoplay_deinit();

    if (get_window_mode() == MODE_PLAY) {
        api_change_mode(MODE_PREVIEW);
        dv_play_state = VIDEO_STOP;
        videopreview_refresh(hWnd);
    }

    InvalidateRect(hWnd, NULL, FALSE);
}

char* dv_get_preview_name(void)
{
    return previewname;
}

void dv_preview_proc(HWND hWnd)
{
    struct module_menu_st *menu_delete = &g_module_menu[PREVIEW_DELETE];

    if (menu_delete->menu_selected == 0) {
        if (videopreview_get_state() != 0) {
            int lang = (int)parameter_get_video_lan();
            dlg_content =(void *)video_text[lang][TEXT_VIDEO_ERR];
            dv_create_message_dialog(title_text[lang][TITLE_WARNING]);
        } else {
            videopreview_play(hWnd);
        }
    } else {
        switch (menu_delete->submenu_sel_num) {
        case YES:
            videopreview_delete(hWnd);
            break;

        case NO:
            break;
        }

        menu_delete->menu_selected = 0;
        InvalidateRect(hWnd, NULL, TRUE);
    }
}
