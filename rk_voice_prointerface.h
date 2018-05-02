/* Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd
 * Author: cherry chen <cherry.chen@rock-chips.com>
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

#ifndef RK_VOICE_PROINTERFACE_H
#define RK_VOICE_PROINTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

extern void VOICE_Init(short int *pshwPara);
extern void VOICE_ProcessTx(short int *pshwIn,
                            short int *pshwRx,
                            short int *pshwOut,
                            int swFrmLen);
extern void VOICE_ProcessRx(short int *pshwIn,
                            short int *pshwOut,
                            int swFrmLen);
extern void VOICE_Destory();

#ifdef __cplusplus
}
#endif

#endif
