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

#ifndef __NMEA_PARSE_H__
#define __NMEA_PARSE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "gps_ctrl.h"
#include "coord_transform.h"
#include "stdint.h"

#define NMEA_MESSAGE_ID_LEN 6
#define NMEA_MESSAGE_LEN 100

/* 1nmi = 1.852km = 1852m */
#define NAUTICAL_MILE 1852

/* NMEA-0183 messages */
#define GPS_XXRMC "RMC"
#define GPS_XXGSV "GSV"
#define GPS_XXGGA "GGA"
#define GPS_XXGLL "GLL"
#define GPS_XXVTG "VTG"
#define GPS_XXGSA "GSA"
#define RMC_INITIAL_VALUE "$XXRMC,,V,,,,,,,,,,N*53\n"
#define GGA_INITIAL_VALUE "$XXGGA,,,,,,0,00,99.99,,,,,,*48\n"

enum {
    /* recommended minimum specific GNSS data */
    GPS_RMC = 1,
    /* GNSS satellites in view */
    GPS_GSV,
    /* global positoning system fixed data */
    GPS_GGA,
    /* geogrphic position ¨Clatitude / longitude */
    GPS_GLL,
    /* course over ground and ground speed */
    GPS_VTG,
    /* GNSS DOP and active satellites */
    GPS_GSA,
};

/*
 * example:
 * $GPGGA,<1>,<2>,<3>,<4>,<5>,<6>,<7>,<8>,<9>,<10>,<11>,<12>,<13>,<14>*hh<CR><LF>
 */
enum {
    GPS_DATA1 = 1,
    GPS_DATA2,
    GPS_DATA3,
    GPS_DATA4,
    GPS_DATA5,
    GPS_DATA6,
    GPS_DATA7,
    GPS_DATA8,
    GPS_DATA9,
    GPS_DATA10,
    GPS_DATA11,
    GPS_DATA12,
    GPS_DATA13,
    GPS_DATA14,
};

struct date_time {
    int years;
    int months;
    int day;
    int hour;
    int min;
    int sec;
};

struct gps_info {
    /* A: data valid, V: data invalid */
    char status;

    /* ddmm.mmmmm, 0000.00000~8959.9999, d is degree, m is minute */
    double latitude;
    /* N: north, S: south */
    char latitude_indicator;

    /* dddmm.mmmmm, 00000.00000~17959.9999, d is degree, m is minute */
    double longitude;
    /* E: east, W: west */
    char longitude_indicator;

    /* speed over ground, 000.0~999.9(nmi/h) */
    float speed;

    /* course over ground, 000.0~359.9 */
    float direction;

    /* MSL altitude, -9999.9~99999.9 */
    float altitude;

    /* number of satellites: 00~12 */
    int satellites;

    /* 0 fix not available or invalid
     * 1 GPS SPS mode,fix valid
     * 2 differential GPS,SPS mode,fix valid
     * 3 GPS PPS mode,fix valid
     */
    int position_fix_indicator;

    struct date_time date;
};

int gps_nmea_data_parse(struct gps_info *info,
                        const char *data,
                        int search_type,
                        int len);

struct text_track_amba_info {
    uint8_t fixation_datas1[10];
    uint8_t times[14];
    uint8_t fixation_data2;
    uint8_t gps_times[14];
    uint8_t fixation_data3;
    uint8_t gps_latitude[9];
    uint8_t gps_longitude[10];
    uint8_t fixation_datas4[5];
    uint8_t gps_speeds[3];
    uint8_t reserved_datas1[108];
    uint8_t gsensor_x[4];
    uint8_t gsensor_y[4];
    uint8_t gsensor_z[4];
    uint8_t fixation_datas5[72];
    uint8_t fixation_datas6[2];
};

void get_gsensor_data(int *x, int *y, int *z);
int text_data_copyfrom_amba_gps(struct text_track_amba_info *track_info,
                                struct gps_info *info);
#ifdef __cplusplus
}
#endif
#endif
