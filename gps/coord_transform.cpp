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

#include "coord_transform.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <autoconfig/main_app_autoconf.h>

#ifdef GPS
#undef INLINE
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199900L
#define INLINE inline
#else
#define INLINE
#endif /* STDC */

#define EARTH_R 6378137.0

#define fabs(x) __ev_fabs(x)
/* do not use >=, or compiler may not know to optimize this
 * with a simple (x & 0x80...0). */
INLINE static double __ev_fabs(double x){ return x > 0.0 ? x : -x; }

INLINE static int out_of_china(double lat, double lng)
{
    int ret = 0;

    if (lng < 72.004 || lng > 137.8347)
        ret = 1;

    if (lat < 0.8293 || lat > 55.8271)
        ret = 1;

    return ret;
}

static void transform(double x, double y, double *lat, double *lng)
{
    double xy = x * y;
    double absX = sqrt(fabs(x));
    double xPi = x * M_PI;
    double yPi = y * M_PI;
    double d = 20.0 * sin(6.0 * xPi) + 20.0 * sin(2.0 * xPi);

    *lat = d;
    *lng = d;

    *lat += 20.0 * sin(yPi) + 40.0 * sin(yPi / 3.0);
    *lng += 20.0 * sin(xPi) + 40.0 * sin(xPi / 3.0);

    *lat += 160.0 * sin(yPi / 12.0) + 320 * sin(yPi / 30.0);
    *lng += 150.0 * sin(xPi / 12.0) + 300.0 * sin(xPi / 30.0);

    *lat *= 2.0 / 3.0;
    *lng *= 2.0 / 3.0;

    *lat += -100.0 + (2.0 * x) + (3.0 * y) + (0.2 * y * y)
            + (0.1 * xy) + (0.2 * absX);
    *lng += 300.0 + x + (2.0 * y) + (0.1 * x * x) + (0.1 * xy) + (0.1 * absX);
}

static void delta(double lat, double lng, double *dLat, double *dLng)
{
    double radLat;
    double magic;
    double sqrtMagic;
    const double ee = 0.00669342162296594323;

    if ((dLat == NULL) || (dLng == NULL)) {
        printf("%s: dLat or dLng is NULL\n", __func__);
        return;
    }

    transform(lng - 105.0, lat - 35.0, dLat, dLng);

    radLat = lat / 180.0 * M_PI;
    magic = sin(radLat);
    magic = 1 - ee * magic * magic;
    sqrtMagic = sqrt(magic);

    *dLat = (*dLat * 180.0) / ((EARTH_R * (1 - ee)) / (magic * sqrtMagic) * M_PI);
    *dLng = (*dLng * 180.0) / (EARTH_R / sqrtMagic * cos(radLat) * M_PI);
}

extern "C" void wgs2gcj(double wgsLat, double wgsLng,
                        double *gcjLat, double *gcjLng)
{
    double dLat, dLng;

    if ((gcjLat == NULL) || (gcjLng == NULL)) {
        printf("%s: gcjLat or gcjLng is NULL\n", __func__);
        return;
    }

    if (out_of_china(wgsLat, wgsLng)) {
        *gcjLat = wgsLat;
        *gcjLng = wgsLng;
        return;
    }

    delta(wgsLat, wgsLng, &dLat, &dLng);
    *gcjLat = wgsLat + dLat;
    *gcjLng = wgsLng + dLng;
}

extern "C" void gcj2wgs(double gcjLat, double gcjLng,
                        double *wgsLat, double *wgsLng)
{
    double dLat, dLng;

    if ((wgsLat == NULL) || (wgsLng == NULL)) {
        printf("%s: wgsLat or wgsLng is NULL\n", __func__);
        return;
    }

    if (out_of_china(gcjLat, gcjLng)) {
        *wgsLat = gcjLat;
        *wgsLng = gcjLng;
        return;
    }

    delta(gcjLat, gcjLng, &dLat, &dLng);
    *wgsLat = gcjLat - dLat;
    *wgsLng = gcjLng - dLng;
}

extern "C" void gcj2wgs_exact(double gcjLat, double gcjLng,
                              double *wgsLat, double *wgsLng)
{
    int i;
    double dLat, dLng;
    double mLat, mLng;
    double pLat, pLng;
    double initDelta = 0.01;
    double threshold = 0.000001;

    dLat = dLng = initDelta;
    mLat = gcjLat - dLat;
    mLng = gcjLng - dLng;
    pLat = gcjLat + dLat;
    pLng = gcjLng + dLng;

    for (i = 0; i < 30; i++) {
        double tmpLat, tmpLng;

        *wgsLat = (mLat + pLat) / 2;
        *wgsLng = (mLng + pLng) / 2;
        wgs2gcj(*wgsLat, *wgsLng, &tmpLat, &tmpLng);

        dLat = tmpLat - gcjLat;
        dLng = tmpLng - gcjLng;
        if ((fabs(dLat) < threshold) && (fabs(dLng) < threshold))
            return;

        if (dLat > 0)
            pLat = *wgsLat;
        else
            mLat = *wgsLat;

        if (dLng > 0)
            pLng = *wgsLng;
        else
            mLng = *wgsLng;
    }
}

extern "C" double coord_distance(double latA, double lngA,
                                 double latB, double lngB)
{
    double alpha;
    double arcLatA = latA * M_PI / 180;
    double arcLatB = latB * M_PI / 180;
    double x = cos(arcLatA) * cos(arcLatB) * cos((lngA-lngB) * M_PI / 180);
    double y = sin(arcLatA) * sin(arcLatB);
    double s = x + y;

    if (s > 1)
        s = 1;

    if (s < -1)
        s = -1;

    alpha = acos(s);
    return alpha * EARTH_R;
}
#else
extern "C" void wgs2gcj(double wgsLat, double wgsLng,
                        double *gcjLat, double *gcjLng) {}

extern "C" void gcj2wgs(double gcjLat, double gcjLng,
                        double *wgsLat, double *wgsLng) {}

extern "C" void gcj2wgs_exact(double gcjLat, double gcjLng,
                              double *wgsLat, double *wgsLng) {}

extern "C" double coord_distance(double latA, double lngA,
                                 double latB, double lngB)
{
    return 0.0;
}
#endif
