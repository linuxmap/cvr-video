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

#include <stdio.h>
#include "rk_fwk.h"

#ifdef MSG_FWK
#include "libfwk_glue/fwk_glue.h"
#include "libfwk_controller/fwk_controller.h"
#endif

int rk_fwk_glue_init(void)
{
#ifdef MSG_FWK
	printf("rk_fwk_glue_init \n");
	fwk_glue_init();
#endif
	return 0;
}

int rk_fwk_glue_destroy(void)
{
#ifdef MSG_FWK
	fwk_glue_destroy();
#endif
	return 0;
}


int rk_fwk_controller_init(void)
{
#ifdef MSG_FWK
	printf("rk_fwk_controller_init \n");
	fwk_controller_init();
#endif

	return 0;
}

int rk_fwk_controller_destroy(void)
{
#ifdef MSG_FWK
	fwk_controller_destroy();
#endif
	return 0;
}
