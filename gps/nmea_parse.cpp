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

#include "nmea_parse.h"
#include <autoconfig/main_app_autoconf.h>

#ifdef GPS
static double gps_degree_minute_to_degree(float dm)
{
    double degree = 0.0;
    int d = 0, integer = 0;
    double m = 0.0, decimals = 0.0;

    integer = (int)dm;
    decimals = dm - integer;

    /* ddmm.mmmmm or dddmm.mmmmm */
    d = integer / 100;
    m = integer % 100 + decimals;

    /* 1 degree = 60 minute */
    degree = d + (m / 60);

    return degree;
}

static void analysis_gsv(int nmea_count,
                         char *nmea_data,
                         struct gps_info *info)
{
    switch (nmea_count) {
    case GPS_DATA3:
        info->satellites = atoi(nmea_data);
        GPS_DBG("satellites: %d\n", info->satellites);
        break;

    default:
        break;
    }
}

static void analysis_gga(int nmea_count,
                         char *nmea_data,
                         struct gps_info *info)
{
    int date;
    float latitude;
    float longitude;

    switch (nmea_count) {
    case GPS_DATA1:
        date = atoi(nmea_data);
        info->date.sec = date % 100;
        info->date.min = (date / 100) % 100;

        /* the calibration time zone, Beijing time */
        info->date.hour = date / 10000 + 8;

        GPS_DBG("hour-min-sec: %d-%d-%d\n", info->date.hour,
                info->date.min, info->date.sec);
        break;

    case GPS_DATA2:
        latitude = atof(nmea_data);
        info->latitude = gps_degree_minute_to_degree(latitude);
        GPS_DBG("latitude: %f\n", info->latitude);
        break;

    case GPS_DATA3:
        info->latitude_indicator = nmea_data[0];
        GPS_DBG("latitude_indicator: %c\n", info->latitude_indicator);
        break;

    case GPS_DATA4:
        longitude = atof(nmea_data);
        info->longitude = gps_degree_minute_to_degree(longitude);
        GPS_DBG("longitude: %f\n", info->longitude);
        break;

    case GPS_DATA5:
        info->longitude_indicator = nmea_data[0];
        GPS_DBG("longitude_indicator: %c\n", info->longitude_indicator);
        break;

    case GPS_DATA6:
        info->position_fix_indicator = atoi(nmea_data);
        GPS_DBG("position_fix_indicator: %d\n", info->position_fix_indicator);
        break;

    case GPS_DATA7:
        info->satellites = atoi(nmea_data);
        GPS_DBG("satellites: %d\n", info->satellites);
        break;

    case GPS_DATA9:
        info->altitude = atof(nmea_data);
        GPS_DBG("altitude: %f\n", info->altitude);
        break;

    case GPS_DATA8:
    case GPS_DATA10:
    case GPS_DATA11:
    case GPS_DATA12:
    case GPS_DATA13:
    case GPS_DATA14:
        break;

    default:
        break;
    }
}

static void analysis_rmc(int nmea_count,
                         char *nmea_data,
                         struct gps_info *info)
{
    int date;
    float latitude;
    float longitude;

    switch (nmea_count) {
    case GPS_DATA1:
        date = atoi(nmea_data);
        info->date.sec = date % 100;
        info->date.min = (date / 100) % 100;

        /* the calibration time zone, Beijing time */
        info->date.hour = date / 10000 + 8;

        GPS_DBG("hour-min-sec: %d-%d-%d\n", info->date.hour,
                info->date.min, info->date.sec);
        break;

    case GPS_DATA2:
        info->status = nmea_data[0];
        GPS_DBG("status: %c\n", info->status);
        break;

    case GPS_DATA3:
        latitude = atof(nmea_data);
        info->latitude = gps_degree_minute_to_degree(latitude);
        GPS_DBG("latitude: %f\n", info->latitude);
        break;

    case GPS_DATA4:
        info->latitude_indicator = nmea_data[0];
        GPS_DBG("latitude_indicator: %c\n", info->latitude_indicator);
        break;

    case GPS_DATA5:
        longitude = atof(nmea_data);
        info->longitude = gps_degree_minute_to_degree(longitude);
        GPS_DBG("longitude: %f\n", info->longitude);
        break;

    case GPS_DATA6:
        info->longitude_indicator = nmea_data[0];
        GPS_DBG("longitude_indicator: %c\n", info->longitude_indicator);
        break;

    case GPS_DATA7:
        info->speed = atof(nmea_data);

        /* nmi/h converted to km/h */
        info->speed = (info->speed * NAUTICAL_MILE) / 1000;
        GPS_DBG("speed: %f\n", info->speed);
        break;

    case GPS_DATA8:
        info->direction = atof(nmea_data);
        GPS_DBG("direction: %f\n", info->direction);
        break;

    case GPS_DATA9:
        date = atoi(nmea_data);
        info->date.years = date % 100 + 100 + 1900;
        info->date.months = (date / 100) % 100;
        info->date.day = date / 10000;

        GPS_DBG("years-months-day: %d-%d-%d\n", info->date.years,
                info->date.months, info->date.day);
        break;

    case GPS_DATA10:
    case GPS_DATA11:
    case GPS_DATA12:
        break;

    default:
        break;
    }
}

static int analysis_msg_type(int search_type,
                             int nmea_count,
                             char *nmea_data,
                             struct gps_info *info)
{
    switch (search_type) {
    case GPS_RMC:
        if (nmea_count == GPS_DATA2 && nmea_data[0] == '0') {
            printf("gps no locate\n");
            return -1;
        }

        analysis_rmc(nmea_count, nmea_data, info);
        break;

    case GPS_GSV:
        analysis_gsv(nmea_count, nmea_data, info);
        break;

    case GPS_GGA:
        analysis_gga(nmea_count, nmea_data, info);
        break;
    }

    return 0;
}

extern "C" int gps_nmea_data_parse(struct gps_info *info,
                                   const char *data,
                                   int search_type,
                                   int len)
{
    int i, ret = -1;
    int nmea_count = 0;
    int data_pos = 0;
    char nmea_data[NMEA_MESSAGE_LEN] = "";

    GPS_DBG("%s, gps len %d\n", __func__, len);
    for (i = 0; i < len; i++)
        GPS_DBG("%c", data[i]);
    GPS_DBG("\n");

    if (len <= NMEA_MESSAGE_ID_LEN) {
        printf("%s, gps data is invalid, len = %d\n", __func__, len);
        return -1;
    }

    for (i = 0; i < len; i++) {
        if (data[i] == ',') {
            nmea_data[0] = '0';
            nmea_data[1] = '\0';
            data_pos = 0;

            ret = analysis_msg_type(search_type, nmea_count, nmea_data, info);
            if (ret < 0)
                break;

            nmea_count++;
            continue;
        }

        if (data[i + 1] == ',') {
            nmea_data[data_pos] = data[i];
            nmea_data[data_pos + 1] = '\0';
            data_pos = 0;

            ret = analysis_msg_type(search_type, nmea_count, nmea_data, info);
            if (ret < 0)
                break;

            nmea_count++;
            i++;
        } else {
            nmea_data[data_pos] = data[i];
            data_pos++;
        }
    }

    return ret;
}

static bool flip_flag = false;

extern "C" int text_data_copyfrom_amba_gps(struct text_track_amba_info *track_info,
                                           struct gps_info *info)
{
    uint32_t temp_integer, temp_decimal;
    double decimal_part;

    char fixation_datas1[10] = {0x01, 0x03, 0x00, 0x00, 0xF2, 0xE1, 0xF0, 0xEE, 0x54, 0x54};
    for(int i = 0; i < 10; i++)
        track_info->fixation_datas1[i] = fixation_datas1[i];

    char str_date[15] = {0};
    sprintf(str_date, "%4d%2d%2d%2d%2d%2d", info->date.years, info->date.months, info->date.day,
                                            info->date.hour, info->date.min, info->date.sec);
    for(int i = 0; i < 14; i++){
        track_info->times[i] = (uint8_t)(str_date[i] ^ 0xAA);
    }

    track_info->fixation_data2 = 0xA6;

    for(int i = 0; i < 14; i++)
        track_info->gps_times[i] = 0x9A;

    track_info->fixation_data3 = 0xA9;

    track_info->gps_latitude[0] = (uint8_t)info->latitude_indicator ^ 0xAA;
    temp_integer = (uint32_t)(info->latitude);
    decimal_part = (info->latitude - (double)temp_integer)*0.6;
    temp_decimal = (uint32_t)(decimal_part*1000000);
    temp_integer =  temp_integer*1000000 + temp_decimal;
    for(int i = 8; i >= 1; i--){
        track_info->gps_latitude[i] = (uint8_t)((temp_integer % 10) + 0x30) ^ 0xAA;
        temp_integer = temp_integer / 10;
    }

    track_info->gps_longitude[0] = (uint8_t)info->longitude_indicator ^ 0xAA;
    temp_integer = (uint32_t)(info->longitude);
    decimal_part = (info->longitude - (double)temp_integer)*0.6;
    temp_decimal = (uint32_t)(decimal_part*1000000);
    temp_integer =  temp_integer*1000000 + temp_decimal;
    for(int i = 9; i >= 1; i--){
        track_info->gps_longitude[i] = (uint8_t)((temp_integer % 10) + 0x30) ^ 0xAA;
        temp_integer = temp_integer / 10;
    }

    char fixation_datas4[5] = {0x81, 0x9A, 0x9A, 0x99, 0x9D};
    for(int i = 0; i < 5; i++)
        track_info->fixation_datas4[i] = fixation_datas4[i];

    temp_integer = (uint8_t)info->speed;
    for(int i = 2; i >= 0; i--){
        track_info->gps_speeds[i] = ((temp_integer % 10) + 0x30) ^ 0xAA;
        temp_integer = temp_integer / 10;
    }

    int sensor_data_x, sensor_data_y, sensor_data_z;
    get_gsensor_data(&sensor_data_x, &sensor_data_y, &sensor_data_z);

    if(sensor_data_x < 0)
        track_info->gsensor_x[0] = 0x87;
    else
        track_info->gsensor_x[0] = 0x81;
    track_info->gsensor_x[1] = ((sensor_data_x/1000000) + 0x30)^0xAA;
    track_info->gsensor_x[2] = (((sensor_data_x%1000000)/100000) + 0x30)^0xAA;
    track_info->gsensor_x[3] = ((((sensor_data_x%1000000)%100000)/10000) + 0x30)^0xAA;

    if(sensor_data_y < 0)
        track_info->gsensor_y[0] = 0x87;
    else
        track_info->gsensor_y[0] = 0x81;
    track_info->gsensor_y[1] = ((sensor_data_y/1000000) + 0x30)^0xAA;
    track_info->gsensor_y[2] = (((sensor_data_y%1000000)/100000) + 0x30)^0xAA;
    track_info->gsensor_y[3] = ((((sensor_data_y%1000000)%100000)/10000) + 0x30)^0xAA;

    if(sensor_data_z < 0)
        track_info->gsensor_z[0] = 0x87;
    else
        track_info->gsensor_z[0] = 0x81;
    track_info->gsensor_z[1] = ((sensor_data_z/1000000) + 0x30)^0xAA;
    track_info->gsensor_z[2] = (((sensor_data_z%1000000)/100000) + 0x30)^0xAA;
    track_info->gsensor_z[3] = ((((sensor_data_z%1000000)%100000)/10000) + 0x30)^0xAA;

    for(int i = 0; i < 72; i++){
        if(flip_flag)
            track_info->fixation_datas5[i] = 0x00;
        else
            track_info->fixation_datas5[i] = 0xAA;
    }
    flip_flag = !flip_flag;

    char fixation_datas6[2] = {0xFF, 0xFD};
    for(int i = 0; i < 2; i++)
        track_info->fixation_datas6[i] = fixation_datas6[i];

    return 0;
}

#else
extern "C" int gps_nmea_data_parse(struct gps_info *info,
                                   const char *data,
                                   int search_type,
                                   int len)
{
    return -1;
}

extern "C" int text_data_copyfrom_amba_gps(struct text_track_amba_info *track_info,
                                   struct gps_info *info)
{
    return -1;
}
#endif
