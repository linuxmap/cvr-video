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

#ifndef __DV_PHOTO_H__
#define __DV_PHOTO_H__

#include <stdbool.h>
#include <minigui/common.h>

int get_photo_resolution(void);
int get_dv_photo_burst_num_index (void);
void set_dv_photo_burst_num(int sel);
bool get_takephoto_state(void);
void set_takephoto_state(bool takingphoto);
void dv_photo_proc(HWND hWnd);
void dv_photo_burst_proc(HWND hWnd);

void dv_photo_lapse_proc(HWND hWnd);

void dv_take_photo_lapse(HWND hWnd);
void dv_take_photo(HWND hWnd);
void dv_take_photo_burst(HWND hWnd);

void show_photo_normal_menu(HDC hdc, HWND hWnd);
void show_photo_lapse_menu(HDC hdc, HWND hWnd);
void show_photo_burst_menu(HDC hdc, HWND hWnd);
void show_preview_delete_menu(HDC hdc, HWND hWnd);
void show_photo_submenu(HDC hdc, HWND hWnd, int module_id);

#endif /* __DV_PHOTO_H__ */
