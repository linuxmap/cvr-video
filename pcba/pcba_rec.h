/**
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd
 * Author: benjo.lei <benjo.lei@rock-chips.com>
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

#ifndef _PCBA_REC_H
#define _PCBA_REC_H

#define CMD_SPEECH_REC            0

enum pcbaRec {
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

int pcba_register(int nfd);
int pcba_unregister(void);

#ifdef __cplusplus
}
#endif

#endif
