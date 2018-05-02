/**
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd
 * Author: Vicent Chi <vicent.chi@rock-chips.com>
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

#ifndef __RIL_EVENT_H_
#define __RIL_EVENT_H_

int send_mess_to_ril(int cmd_type, char* amendata);
int handup_ril_voip();
void ril_open_hot_share();
void ril_close_hot_share();
int ril_register(void);
int ril_unregister(void);
int ril_reg_readevent_callback(int (*call)(int cmd, void *msg0, void *msg1));

#endif

