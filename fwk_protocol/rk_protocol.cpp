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
#include "rk_protocol.h"

#ifdef PROTOCOL_IOTC
#include "iotc/rk_iotc.h"
#endif

#ifdef PROTOCOL_GB28181
#include "gb28181/rk_gb28181.h"
#endif

int protocol_rk_iotc_init(void)
{
#ifdef PROTOCOL_IOTC
	printf("protocol_rk_iotc_init \n");
	rk_iotc_init();
#endif
	return 0;
}

int protocol_rk_iotc_destroy(void)
{
#ifdef PROTOCOL_IOTC
	rk_iotc_destroy();
#endif
	return 0;
}

int protocol_rk_gb28181_init(void)
{
#ifdef PROTOCOL_GB28181
	printf("protocol_rk_gb28181_init \n");
	rk_gb28181_init();
#endif
	return 0;
}

int protocol_rk_gb28181_destroy(void)
{
#ifdef PROTOCOL_GB28181
	rk_gb28181_destroy();
#endif
	return 0;
}
