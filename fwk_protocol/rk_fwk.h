/**
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd
 * author: Dayao Ji <jdy@rock-chips.com>
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



#ifndef __RK_FWK__
#define __RK_FWK__

#ifdef __cplusplus
extern "C" {
#endif

int rk_fwk_glue_init(void);

int rk_fwk_glue_destroy(void);

int rk_fwk_controller_init(void);

int rk_fwk_controller_destroy(void);


#ifdef __cplusplus
}
#endif

#endif  /*__RK_FWK__*/
