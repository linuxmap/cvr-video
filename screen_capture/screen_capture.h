/**
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd
 * author: hertz.wang hertz.wong@rock-chips.com
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

#ifndef SCREEN_CAPTURE
#define SCREEN_CAPTURE

#ifdef __cplusplus
extern "C" {
#endif

/* Should called in ui thread */
int take_screen_capture();

#ifdef __cplusplus
}
#endif

#endif // #ifndef SCREEN_CAPTURE
