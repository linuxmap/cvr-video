/**
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd
 * author: hogan.wang@rock-chips.com
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

#ifndef __USB_MODE_H__
#define __USB_MODE_H__

#ifdef __cplusplus
extern "C" {
#endif

void cmd_IDM_USB(void);
void cmd_IDM_ADB(void);
void cmd_IDM_UVC(void);
void cmd_IDM_RNDIS(void);

void ui_deinit_uvc();
int ui_init_uvc();

#ifdef __cplusplus
}
#endif

#endif
