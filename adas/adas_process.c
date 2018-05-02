/**
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd
 * Author: Zain Wang<wzz@rock-chips.com>
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

#include "adas_process.h"
#include "parameter.h"
#include "ui_resolution.h"
#include <dpp/algo/adas/adas.h>
#include <stdio.h>
#include <string.h>
#include <video.h>
#include <sys/time.h>
#include <signal.h>
#include "gps/nmea_parse.h"
#include "public_interface.h"
#include "parking_monitor/kalman.h"

#define ADAS_KALMAN_FILTER_SENSITIVE_HEIGHT     0.0002
#define ADAS_KALMAN_FILTER_SENSITIVE_LOW        0.002

struct adas_speed_detection {
        struct gps_info         gps;
        struct kalman           kalman;
        struct gsensor_data     gs_data;
        int                     gs_cnt;
        int                     car_status;
        int                     car_status_store;
        float                   speed;
};

static struct adas_parameter g_adas_parameter;
static struct adas_info g_adas_info;
static struct adas_process_output g_adas_output;
static struct adas_speed_detection g_adas_speed;

static struct itimerval g_timer_val;

void AdasGetParameter(struct adas_parameter *parameter)
{
    memcpy(parameter, &g_adas_parameter, sizeof(g_adas_parameter));
}

static void AdasLimitValue(char value, char max, char min)
{
    if (value > max)
        value = max;
    if (value < min)
        value = min;
}

static void AdasCheckParameter(struct adas_parameter *parameter)
{
    AdasLimitValue(parameter->adas_setting[0], 0, 100);
    AdasLimitValue(parameter->adas_setting[1], 0, 100);
    AdasLimitValue(parameter->adas_direction, 0, 100);
}

void AdasSaveParameter(struct adas_parameter *parameter)
{
    AdasCheckParameter(parameter);
    memcpy(&g_adas_parameter, parameter, sizeof(g_adas_parameter));
    parameter_save_video_adas_setting(g_adas_parameter.adas_setting);
    parameter_save_video_adas_alert_distance(g_adas_parameter.adas_alert_distance);
    parameter_save_video_adas_direction(g_adas_parameter.adas_direction);
    parameter_save_video_adas(g_adas_parameter.adas_state);
    dpp_adas_set_calibrate_line(
            (int)(ADAS_FCW_HEIGHT * ((float)parameter->adas_setting[0] / 100)),
            (int)(ADAS_FCW_HEIGHT * ((float)parameter->adas_setting[1] / 100)));
}

static int AdasCarStatusDetection(float speed)
{
    int car_status;

    if (g_adas_info.is_gps_support || g_adas_info.is_gs_support) {
        if (speed < ADAS_SPEED_SLOW)
            car_status = ADAS_CAR_STATUS_STOP;
        else if (speed < ADAS_SPEED_FAST)
            car_status = ADAS_CAR_STATUS_PACE;
        else
            car_status = ADAS_CAR_STATUS_RUN;
    } else {
        car_status = ADAS_CAR_STATUS_UNKNOWN;
    }

    return car_status;
}

void AdasProcess(int cmd, void *msg0, void *msg1)
{
    int i;
    struct adas_output *in = (struct adas_output *)msg0;
    struct odt_object *obj;
    struct ldw_output *ldw;
    struct adas_fcw *fcw;
    int direction;

    g_adas_output.fcw_count = 0;
    g_adas_output.fcw_alert = 0;
    g_adas_output.ldw_flag = 0;
    direction = (int)(g_adas_info.width * ((float)g_adas_parameter.adas_direction / 100));

    /* FCW process */
    if (in->odt.valid) {
        pthread_mutex_lock(&g_adas_info.process_mutex);
        if (g_adas_speed.car_status == g_adas_speed.car_status_store)
            g_adas_output.car_status = g_adas_speed.car_status;
        g_adas_output.speed = g_adas_speed.speed;

        g_adas_output.fcw_count = in->odt.count;
        for (i = 0; i < in->odt.count; i++) {
            obj = &in->odt.objects[i];
            fcw = &g_adas_output.fcw_out[i];
            fcw->img.x = (obj->x * g_adas_info.width) / ADAS_FCW_WIDTH;
            fcw->img.y = (obj->y * g_adas_info.height) / ADAS_FCW_HEIGHT;
            fcw->img.width = (obj->width * g_adas_info.width) / ADAS_FCW_WIDTH;
            fcw->img.height = (obj->height* g_adas_info.height) / ADAS_FCW_HEIGHT;
            fcw->distance = obj->distance;
            fcw->alert = 0;

            if (g_adas_output.car_status == ADAS_CAR_STATUS_RUN) {
                int alert = (g_adas_output.speed - ADAS_SPEED_FAST) *
                            g_adas_parameter.coefficient + g_adas_parameter.base;

                if ((obj->distance < alert) &&
                    (fcw->img.x < direction &&
                     (fcw->img.x + fcw->img.width) > direction))
                    fcw->alert = 1;
            } else if (g_adas_output.car_status == ADAS_CAR_STATUS_STOP) {
                if ((obj->distance > g_adas_parameter.adas_alert_distance) &&
                    (fcw->img.x < direction &&
                     (fcw->img.x + fcw->img.width) > direction))
                    fcw->alert = 1;
            }
            g_adas_output.fcw_alert += fcw->alert;
        }
        pthread_mutex_unlock(&g_adas_info.process_mutex);

        if (g_adas_output.fcw_alert) {
            if (g_adas_output.car_status == ADAS_CAR_STATUS_RUN)
                g_adas_output.fcw_alert = ADAS_ALERT_RUN;
            else if (g_adas_output.car_status == ADAS_CAR_STATUS_STOP)
                g_adas_output.fcw_alert = ADAS_ALERT_STOP;
        }
    }

    /* LDW process */
    if (in->ldw.valid) {
        g_adas_output.ldw_flag = in->ldw.turn_flag + 1;
        ldw = &in->ldw;
        for (i = 0; i < 4; i++) {
            g_adas_output.ldw_pos[i].x =
                ldw->end_points[i][0] * g_adas_info.width / ADAS_LDW_WIDTH;
            g_adas_output.ldw_pos[i].y =
                ldw->end_points[i][1] * g_adas_info.height / ADAS_LDW_HEIGHT;
        }
    }

    if (g_adas_info.callback)
        g_adas_info.callback(0, (void *)&g_adas_output, NULL);
}

int AdasEventRegCallback(void (*call)(int cmd, void *msg0, void *msg1))
{
    g_adas_info.callback = call;
    return 0;
}

void AdasGpsUpdate(void *gps)
{
    if(gps == NULL)
        return;

    pthread_mutex_lock(&g_adas_info.data_mutex);
    memcpy(&g_adas_speed.gps, gps, sizeof(struct gps_info));
    pthread_mutex_unlock(&g_adas_info.data_mutex);
}

static int adas_get_gsdata(struct sensor_axis data)
{
    pthread_mutex_lock(&g_adas_info.data_mutex);
    if (g_adas_speed.gs_cnt < MAX_BUF_LEN) {
        memcpy(&g_adas_speed.gs_data.sa[g_adas_speed.gs_cnt], &data,
               sizeof(data));
        g_adas_speed.gs_cnt++;
    }
    pthread_mutex_unlock(&g_adas_info.data_mutex);

    return 0;
}

static void adas_timer_handle(int sigo)
{
    pthread_mutex_lock(&g_adas_info.process_mutex);
    pthread_mutex_lock(&g_adas_info.data_mutex);

    g_adas_speed.speed = 0;

    if (g_adas_info.is_gps_support) {
        if (g_adas_speed.gps.status == 'A') {
            g_adas_speed.speed = g_adas_speed.gps.speed;
            goto BACK;
        }
    }

    if (g_adas_info.is_gs_support) {
        if (g_adas_speed.gs_cnt) {
            if (sliding_filter(&g_adas_speed.kalman, &g_adas_speed.gs_data,
                g_adas_speed.gs_cnt,
                g_adas_output.car_status == ADAS_CAR_STATUS_STOP ?
                (double)ADAS_KALMAN_FILTER_SENSITIVE_LOW :
                (double)ADAS_KALMAN_FILTER_SENSITIVE_HEIGHT))
                /*
                 * If GPS unable to work, we can use GS instead GPS.
                 * But GS unsupport speed data, we can only know whether the
                 * the car in moved or stopped.
                 * For less mistake alrams, let's defined the default
                 * speed(30 Km/h) for GS moved. In this speed, It always in
                 * ADAS_CAR_STATUS_PACE which can't meet any alarms.
                 */
                g_adas_speed.speed = 30;
            else
                g_adas_speed.speed = 0;
        }
    }

    g_adas_speed.car_status_store = g_adas_speed.car_status;
    g_adas_speed.car_status = AdasCarStatusDetection(g_adas_speed.speed);
BACK:
    g_adas_speed.gs_cnt = 0;
    pthread_mutex_unlock(&g_adas_info.data_mutex);
    pthread_mutex_unlock(&g_adas_info.process_mutex);
}

static void adas_timer_init(void)
{
    struct sigaction act;

    act.sa_handler = adas_timer_handle;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaction(SIGALRM, &act, NULL);

    g_timer_val.it_value.tv_sec = 1;
    g_timer_val.it_value.tv_usec = 0;
    g_timer_val.it_interval.tv_sec = 1;
    g_timer_val.it_interval.tv_usec = 0;

    setitimer(ITIMER_REAL, &g_timer_val, NULL);
}

static void adas_timer_deinit(void)
{
    g_timer_val.it_value.tv_sec = 0;
    g_timer_val.it_interval.tv_sec = 0;

    setitimer(ITIMER_REAL, &g_timer_val, NULL);
}

int AdasInit(int width, int height)
{
    memset(&g_adas_parameter, 0, sizeof(g_adas_parameter));
    memset(&g_adas_speed, 0, sizeof(g_adas_speed));
    parameter_get_video_adas_setting(g_adas_parameter.adas_setting);
    g_adas_parameter.adas_alert_distance = parameter_get_video_adas_alert_distance();
    g_adas_parameter.adas_direction = parameter_get_video_adas_direction();
    g_adas_parameter.adas_state = parameter_get_video_adas();
    g_adas_info.width = width;
    g_adas_info.height = height;

    g_adas_parameter.coefficient = 0.75;
    g_adas_parameter.base = 10;
    g_adas_info.is_gps_support = true;
    g_adas_info.is_gs_support = true;

    if (g_adas_parameter.adas_state) {
        if (api_gps_init() < 0) {
            g_adas_info.is_gps_support = false;
            printf("[ADAS] Warning: %s: Can't open GPS\n", __func__);
        }

        g_adas_info.gsensor_reg = gsensor_regsenddatafunc(adas_get_gsdata);
        if (g_adas_info.gsensor_reg <= 0) {
            g_adas_info.is_gs_support = false;
            printf("[ADAS] Warning: %s: Can't open GS\n", __func__);
        } else {
            kalman_init(&g_adas_speed.kalman);
        }

        if (g_adas_info.is_gps_support || g_adas_info.is_gs_support)
            adas_timer_init();
    }

    dpp_adas_set_calibrate_line(
            (int)(ADAS_FCW_HEIGHT * ((float)g_adas_parameter.adas_setting[0] / 100)),
            (int)(ADAS_FCW_HEIGHT * ((float)g_adas_parameter.adas_setting[1] / 100)));
    ADAS_RegEventCallback(AdasProcess);
    g_adas_speed.gs_cnt = 0;
    pthread_mutex_init(&g_adas_info.data_mutex, NULL);
    pthread_mutex_init(&g_adas_info.process_mutex, NULL);

    return 0;
}

void AdasDeinit(void)
{
    if (g_adas_parameter.adas_state) {
        adas_timer_deinit();
        if (g_adas_info.is_gs_support)
            gsensor_unregsenddatafunc(g_adas_info.gsensor_reg);
        if (g_adas_info.is_gps_support)
            api_gps_deinit();
        AdasSaveParameter(&g_adas_parameter);
        pthread_mutex_destroy(&g_adas_info.data_mutex);
        pthread_mutex_destroy(&g_adas_info.process_mutex);
    }
}

int AdasOn(void)
{
    if (!g_adas_parameter.adas_state) {
        g_adas_info.is_gps_support = true;
        g_adas_info.is_gs_support = true;
        if (api_gps_init() < 0) {
            g_adas_info.is_gps_support = false;
            printf("[ADAS] Warning: %s: Can't open GPS\n", __func__);
        }
        g_adas_info.gsensor_reg = gsensor_regsenddatafunc(adas_get_gsdata);
        if (g_adas_info.gsensor_reg <= 0) {
            g_adas_info.is_gs_support = false;
            printf("[ADAS] Warning: %s: Can't open GS\n", __func__);
        }
        kalman_init(&g_adas_speed.kalman);
        adas_timer_init();
        g_adas_parameter.adas_state = 1;
        parameter_save_video_adas(g_adas_parameter.adas_state);
    }

    return 0;
}

int AdasOff(void)
{
    if (g_adas_parameter.adas_state) {
        adas_timer_deinit();
        if (g_adas_info.is_gs_support)
            gsensor_unregsenddatafunc(g_adas_info.gsensor_reg);
        if (g_adas_info.is_gps_support)
            api_gps_deinit();
        g_adas_parameter.adas_state = 0;
        parameter_save_video_adas(g_adas_parameter.adas_state);
    }
    return 0;
}

void AdasSetWH(int width, int height)
{
    g_adas_info.width = width;
    g_adas_info.height = height;
}
