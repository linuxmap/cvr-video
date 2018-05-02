/**
 * Copyright (C) 2016 Fuzhou Rockchip Electronics Co., Ltd
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

#include "jpeg_header.h"

#define UPPER_LEN 2048

int has_huffman(unsigned char *buf, unsigned int buf_size)
{
	unsigned int i = 0;
	unsigned char cur_byte;

	/* modify from video.cpp */
	while (i < buf_size) {
		cur_byte = buf[i++];
		if (cur_byte == 0xFF) {
			cur_byte = buf[i++];
			if (cur_byte == DHT)
				return 1;
			else if (cur_byte == SOS)
				break;
		}
		if (i > UPPER_LEN)
			break;
	}
	return 0;
}
