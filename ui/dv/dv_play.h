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

#ifndef __DV_PLAY_H__
#define __DV_PLAY_H__

#include <minigui/common.h>

int videopreview_get_state(void);
void entervideopreview(void);
void videopreview_refresh(HWND hWnd);
void videopreview_next(HWND hWnd);
void videopreview_pre(HWND hWnd);
void videopreview_play(HWND hWnd);
int videopreview_delete(HWND hWnd);
void startplayvideo(HWND hWnd, char *filename);
void exitvideopreview(void);
void exitplayvideo(HWND hWnd);
char *dv_get_preview_name(void);
void dv_preview_proc(HWND hWnd);

#endif /* __DV_PLAY_H__ */
