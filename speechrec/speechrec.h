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

#ifndef _SPEECHREC_H
#define _SPEECHREC_H

#include "pocketsphinx/pocketsphinx.h"
#include "sphinxbase/err.h"
#include "sphinxbase/ad.h"

#define CMD_SPEECH_REC            0

enum speechRec {
    SPEECH_START_WAKEUP         = 0,
    SPEECH_START_RECORD,
    SPEECH_STOP_RECORD,
    SPEECH_TAKE_PHOTOS,
    SPEECH_LOCK_FILE,
    SPEECH_BACK_CAR,
    SPEECH_CANCEL_BACK_CAR,
    SPEECH_OPENSCREEN,
    SPEECH_CLOSESCREEN,
    SPEECH_END,
    SPEECH_UNKNOW
};

#ifdef __cplusplus
extern "C" {
#endif

int speechrec_register(void);
int speechrec_unregister(void);
int speechrec_reg_readevent_callback(int (*call)(int cmd, void *msg0, void *msg1));

#ifdef __cplusplus
}
#endif

#endif