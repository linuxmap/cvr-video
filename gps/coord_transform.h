/**
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd
 * author: Tianfeng Chen <ctf@rock-chips.com>
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

#ifndef __COORD_TRANSFORM_H__
#define __COORD_TRANSFORM_H__

#ifdef __cplusplus
extern "C" {
#endif

void wgs2gcj(double wgsLat, double wgsLng, double *gcjLat, double *gcjLng);
void gcj2wgs(double gcjLat, double gcjLng, double *wgsLat, double *wgsLng);
void gcj2wgs_exact(double gcjLat, double gcjLng, double *wgsLat, double *wgsLng);
double coord_distance(double latA, double lngA, double latB, double lngB);

#ifdef __cplusplus
}
#endif

#endif