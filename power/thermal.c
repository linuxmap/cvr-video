/**
 * Copyright (C) 2016 Fuzhou Rockchip Electronics Co., Ltd
 * Author: Cain Cai <cain.cai@rock-chips.com>
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

#include "rk_fb/rk_fb.h"
#include "thermal.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>

#include "parameter.h"
#include "public_interface.h"
#include "wifi_management.h"

#define THERMAL_VERSION "v0.0.3"
#define MAX_BUF 10

#define KERNEL_THERMAL  "/sys/module/rockchip_pm/parameters/policy"

#define TEMP_INPUT  "/sys/devices/10370000.tsadc/temp0_input"
#define TEMP_MIN    "/sys/devices/10370000.tsadc/temp0_min"
#define TEMP_MIN_ALARM  "/sys/devices/10370000.tsadc/temp0_min_alarm"
#define TEMP_MAX    "/sys/devices/10370000.tsadc/temp0_max"
#define TEMP_MAX_ALARM  "/sys/devices/10370000.tsadc/temp0_max_alarm"

#define CPU_TEMP_TARGET "/sys/dvfs/cpu_temp_target"
#define CPU_FREQ    "/sys/devices/system/cpu/cpu0/cpufreq"
#define CPU_FREQ_MAX    "/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq"

#define DDR_400M  'l'
#define DDR_600M  'n'
#define DDR_800M  'p'
#define DDR_EXIT_LOW_POWER  'L'
#define DDR_DEV     "/dev/video_state"

#define FB_FPS      "/sys/class/graphics/fb0/fps"
#define BRIGHTNESS  "/sys/class/backlight/rk28_bl/brightness"

#define MIN_THERMAL_LEVEL THERMAL_LEVEL0
#define MAX_THERMAL_LEVEL THERMAL_LEVEL5

struct thermal_dev_t thermal_dev;

static int temperature_set(const char *path, int val)
{
    int ret = -1, fd = -1;
    char buf[MAX_BUF] = { 0 };

    fd = open(path, O_WRONLY);
    if (fd < 0) {
        printf("%s can't open %s\n", __func__, path);
        goto out;
    }

    snprintf(buf, sizeof(buf), "%d", val);
    ret = write(fd, buf, strlen(buf));
    close(fd);
    if (ret <= 0)
        printf("%s write Error,ret=%d,fd=%d\n", __func__, ret, fd);

out:
    return ret;
}

static int temperature_read(void)
{
    int ret = -1, fd = -1;
    char buf[MAX_BUF] = { 0 };

    fd = open(TEMP_INPUT, O_RDONLY);
    if (fd < 0) {
        printf("%s can't open %s\n", __func__, TEMP_INPUT);
        goto out;
    }

    ret = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (ret <= 0)
        printf("%s read Error,ret=%d,fd=%d\n", __func__, ret, fd);

out:
    return atoi(buf);
}

static int temperature_get_state(void)
{
    int ret, state, fd = -1;
    char buf[MAX_BUF] = { 0 };

    state = thermal_dev.state;
    thermal_dev.temp = temperature_read();

    fd = open(TEMP_MIN_ALARM, O_RDONLY);
    if (fd < 0) {
        printf("%s can't open %s\n", __func__, TEMP_MIN_ALARM);
        state = -1;
        goto out;
    }

    ret = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (ret <= 0)
        printf("%s read Error,ret=%d,fd=%d\n", __func__, ret, fd);

    if (atoi(buf)) {
        if (thermal_dev.state != MIN_THERMAL_LEVEL)
            state = thermal_dev.state - 1;
        else
            state = MIN_THERMAL_LEVEL;

        thermal_dev.htemp = thermal_dev.hthreshold[state];
        if (state != MIN_THERMAL_LEVEL)
            thermal_dev.ltemp = thermal_dev.lthreshold[state - 1];
        else
            thermal_dev.ltemp = thermal_dev.lthreshold[state];
        goto out;
    }

    memset(buf, 0, sizeof(buf));
    fd = open(TEMP_MAX_ALARM, O_RDONLY);
    if (fd < 0) {
        printf("%s can't open %s\n", __func__, TEMP_MAX_ALARM);
        state = -1;
        goto out;
    }

    ret = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (ret <= 0)
        printf("%s read Error,ret=%d,fd=%d\n", __func__, ret, fd);

    if (atoi(buf)) {
        if (thermal_dev.state != MAX_THERMAL_LEVEL)
            state = thermal_dev.state + 1;
        else
            state = MAX_THERMAL_LEVEL;

        thermal_dev.ltemp = thermal_dev.lthreshold[state - 1];
        if (state != MAX_THERMAL_LEVEL)
            thermal_dev.htemp = thermal_dev.hthreshold[state];
        else
            thermal_dev.htemp = thermal_dev.hthreshold[state - 1];
    }

out:
    return state;
}

static int thermal_set_alarm(int min_alarm, int max_alarm)
{
    int ret = -1;

    if ((min_alarm > max_alarm) || (min_alarm == 0) || (max_alarm == 0)) {
        printf("the value of min_alarm:%d max_alarm:%d invalid\n",
               min_alarm, max_alarm);
        goto out;
    }

    printf("thermal set min alarm:%d\n", min_alarm);
    ret = temperature_set(TEMP_MIN, min_alarm);
    if (ret <= 0)
        goto out;

    printf("thermal set max alarm:%d\n", max_alarm);
    ret = temperature_set(TEMP_MAX, max_alarm);

out:
    return ret;
}

static int get_ddr_state(void)
{
    int ret = -1, fd = -1;
    char buf[MAX_BUF] = { 0 };
    long val = 0;

    fd = open(DDR_DEV, O_RDONLY);
    if (fd < 0) {
        printf("%s can't open %s\n", __func__, DDR_DEV);
        thermal_dev.handle_ddr = false;
        goto out;
    }

    ret = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (ret <= 0) {
        printf("%s read Error,ret=%d,fd=%d\n", __func__, ret, fd);
        thermal_dev.handle_ddr = false;
        goto out;
    }

    val = strtol(buf, NULL, 16);
    if (val >> 17) {
        return DDR_400M;
    } else if (val >> 13) {
        return DDR_800M;
    } else {
        return DDR_600M;
    }
out:
    return ret;
}

static int set_ddr_state(const char state)
{
    int ret = -1, fd = -1;

    if (get_ddr_state() == state)
        return 0;

    fd = open(DDR_DEV, O_WRONLY);
    if (fd < 0) {
        printf("%s can't open %s\n", __func__, DDR_DEV);
        thermal_dev.handle_ddr = false;
        goto out;
    }
    ret = write(fd, &state, 1);
    close(fd);
    if (ret <= 0) {
        printf("%s write Error,ret=%d,fd=%d\n", __func__, ret, fd);
        thermal_dev.handle_ddr = false;
    }

out:
    return ret;
}

static int get_fb_fps(void)
{
    int ret = -1, fd = -1;
    char buf[MAX_BUF] = { 0 };

    fd = open(FB_FPS, O_RDONLY);
    if (fd < 0) {
        printf("%s can't open %s\n", __func__, FB_FPS);
        thermal_dev.handle_fb_fps = false;
        goto out;
    }

    ret = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (ret <= 0) {
        printf("%s read Error,ret=%d,fd=%d\n", __func__, ret, fd);
        thermal_dev.handle_fb_fps = false;
    }
out:
    /* format: fps:xx */
    return atoi(buf + 4);
}

static int set_fb_fps(int fb_fps)
{
    int ret = -1, fd = -1;
    char buf[MAX_BUF] = { 0 };

    if (rk_fb_get_hdmi_connect())
        goto out;

    if (thermal_dev.handle_fb_fps &&
        (fb_fps != thermal_dev.cur_fb_fps) && rk_fb_get_screen_state()) {
        fd = open(FB_FPS, O_WRONLY);
        if (fd < 0) {
            printf("%s can't open %s\n", __func__, FB_FPS);
            thermal_dev.handle_fb_fps = false;
            goto out;
        }

        thermal_dev.cur_fb_fps = fb_fps;
        snprintf(buf, sizeof(buf), "%d", fb_fps);
        ret = write(fd, buf, strlen(buf));
        close(fd);
        if (ret <= 0) {
            printf("%s write Error,ret=%d,fd=%d\n", __func__, ret, fd);
            thermal_dev.handle_fb_fps = false;
        }
    }
out:
    return ret;
}

static int set_backlight(int brightness)
{
    int ret = -1, fd = -1;
    char buf[MAX_BUF] = { 0 };

    if (thermal_dev.handle_brightness &&
        (brightness != thermal_dev.cur_brightness) && rk_fb_get_screen_state()) {
        fd = open(BRIGHTNESS, O_WRONLY);
        if (fd < 0) {
            printf("%s can't open %s\n", __func__, BRIGHTNESS);
            thermal_dev.handle_brightness = false;
            goto out;
        }

        thermal_dev.cur_brightness = brightness;
        snprintf(buf, sizeof(buf), "%d", brightness);
        ret = write(fd, buf, strlen(buf));
        close(fd);
        if (ret <= 0) {
            printf("%s write Error,ret=%d,fd=%d\n", __func__, ret, fd);
            thermal_dev.handle_brightness = false;
        }
    }

out:
    return ret;
}

static void thermal_handle(int state)
{
    switch (state) {
    case THERMAL_LEVEL0:
        set_backlight(thermal_dev.brightness);
        if (thermal_dev.off_3dnr) {
            parameter_save_video_3dnr(1);
            thermal_dev.off_3dnr = false;
        }
        api_dvs_onoff(1);
        break;
    case THERMAL_LEVEL1:
        set_backlight(thermal_dev.brightness - 10);
        if (thermal_dev.handle_ddr && (thermal_dev.ddr_state == DDR_800M)) {
            set_ddr_state(DDR_800M);
        }
        if (thermal_dev.handle_3dnr && parameter_get_video_3dnr()) {
            thermal_dev.off_3dnr = true;
            parameter_save_video_3dnr(0);
        }
        api_dvs_onoff(0);
        break;
    case THERMAL_LEVEL2:
        set_backlight(thermal_dev.brightness - 20);
        if (thermal_dev.handle_ddr && (thermal_dev.ddr_state == DDR_800M)) {
            set_ddr_state(DDR_600M);
        }
        break;
    case THERMAL_LEVEL3:
        set_backlight(thermal_dev.brightness - 30);
        if (thermal_dev.off_wifi) {
            parameter_save_wifi_en(1);
            wifi_management_start();
        }
        set_fb_fps(thermal_dev.fb_fps);
        if (thermal_dev.ddr_low_power) {
            set_ddr_state(DDR_EXIT_LOW_POWER);
            thermal_dev.ddr_low_power = false;
        }
        break;
    case THERMAL_LEVEL4:
        set_backlight(thermal_dev.brightness - 40);
        if (thermal_dev.handle_wifi && parameter_get_wifi_en()) {
            thermal_dev.off_wifi = true;
            parameter_save_wifi_en(0);
            wifi_management_stop();
        }
        set_fb_fps(thermal_dev.fb_fps - 10);
        if (thermal_dev.handle_ddr && !thermal_dev.ddr_low_power) {
            set_ddr_state(DDR_400M);
            thermal_dev.ddr_low_power = true;
        }
        break;
    case THERMAL_LEVEL5:
        break;
    default:
        printf("%s invalid state:%d\n", __func__, state);
    }
}

void thermal_update_state(void)
{
    thermal_dev.state = temperature_get_state();
    if ((thermal_dev.state < MIN_THERMAL_LEVEL) ||
        (thermal_dev.state > MAX_THERMAL_LEVEL)) {
        printf("%s invalid state:%d\n", __func__, thermal_dev.state);
        return;
    }

    if (thermal_dev.old_state != thermal_dev.state) {
        thermal_dev.old_state = thermal_dev.state;
        if (!parameter_get_debug_temp())
            thermal_handle(thermal_dev.state);
        thermal_set_alarm(thermal_dev.ltemp, thermal_dev.htemp);
        printf("%s state:%d temp:%d\n",
               __func__, thermal_dev.state, thermal_dev.temp);
    }
}

static int thermal_parse_cfg(void)
{
    int ret = -1, fd = -1;
    int cpu_target_temp;
    char buf[MAX_BUF] = { 0 };

    memset(&thermal_dev, 0, sizeof(thermal_dev));
    fd = open(CPU_TEMP_TARGET, O_RDONLY);
    if (fd < 0) {
        printf("%s can't open %s\n", __func__, CPU_TEMP_TARGET);
        goto out;
    }

    ret = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (ret <= 0) {
        printf("%s read Error,ret=%d,fd=%d\n", __func__, ret, fd);
        goto out;
    }

    cpu_target_temp = atoi(buf + 4);
    if (cpu_target_temp > 100)
        cpu_target_temp = 100;

    thermal_dev.lthreshold[0] = cpu_target_temp - 15;
    thermal_dev.lthreshold[1] = cpu_target_temp - 5;
    thermal_dev.lthreshold[2] = cpu_target_temp;
    thermal_dev.lthreshold[3] = cpu_target_temp + 10;
    thermal_dev.lthreshold[4] = cpu_target_temp + 15;

    thermal_dev.hthreshold[0] = cpu_target_temp - 5;
    thermal_dev.hthreshold[1] = cpu_target_temp;
    thermal_dev.hthreshold[2] = cpu_target_temp + 10;
    thermal_dev.hthreshold[3] = cpu_target_temp + 15;
    thermal_dev.hthreshold[4] = cpu_target_temp + 18;

    thermal_dev.state = THERMAL_LEVEL0;
    thermal_dev.old_state = thermal_dev.state;
    thermal_dev.temp = temperature_read();
    thermal_dev.stop_kernel_thermal = false;
    thermal_dev.ltemp = thermal_dev.lthreshold[0];
    thermal_dev.htemp = thermal_dev.hthreshold[0];
    thermal_dev.handle_brightness = true;
    thermal_dev.handle_wifi = true;
    thermal_dev.off_wifi = false;
    thermal_dev.handle_fb_fps = false;
    thermal_dev.handle_3dnr = true;
    thermal_dev.off_3dnr = false;
    thermal_dev.handle_ddr = true;
    thermal_dev.ddr_low_power = false;

    thermal_dev.ddr_state = get_ddr_state();
    if (thermal_dev.ddr_state == DDR_400M)
        thermal_dev.handle_ddr = false;

    ret = thermal_set_alarm(thermal_dev.ltemp, thermal_dev.htemp);

    thermal_dev.fb_fps = get_fb_fps();

    fd = open(BRIGHTNESS, O_RDONLY);
    if (fd < 0) {
        printf("%s can't open %s\n", __func__, BRIGHTNESS);
        thermal_dev.handle_brightness = false;
        goto out;
    }

    memset(buf, 0, sizeof(buf));

    ret = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (ret <= 0) {
        printf("%s read Error,ret=%d,fd=%d\n", __func__, ret, fd);
        thermal_dev.handle_brightness = false;
        goto out;
    }
    thermal_dev.brightness = atoi(buf);
    thermal_dev.cur_brightness = thermal_dev.brightness;

out:
    return ret;
}

static int stop_kernel_thermal(void)
{
    int ret = -1, fd = -1;
    char buf[MAX_BUF] = { 0 };

    fd = open(KERNEL_THERMAL, O_RDWR);
    if (fd < 0) {
        printf("%s can't open %s\n", __func__, KERNEL_THERMAL);
        goto out;
    }

    snprintf(buf, sizeof(buf), "%d", 0);
    ret = write(fd, buf, strlen(buf));
    close(fd);
    if (ret <= 0) {
        printf("%s write Error,ret=%d,fd=%d\n", __func__, ret, fd);
        goto out;
    }

out:
    return ret;
}

static void thermal_test(void)
{
    static int num = 0;

    num++;
    switch (num) {
    case 1:
        thermal_dev.state = THERMAL_LEVEL1;
        thermal_handle(thermal_dev.state);
        break;
    case 2:
        thermal_dev.state = THERMAL_LEVEL2;
        thermal_handle(thermal_dev.state);
        break;
    case 3:
        thermal_dev.state = THERMAL_LEVEL3;
        thermal_handle(thermal_dev.state);
        break;
    case 4:
        thermal_dev.state = THERMAL_LEVEL4;
        thermal_handle(thermal_dev.state);
        break;
    case 5:
        thermal_dev.state = THERMAL_LEVEL5;
        thermal_handle(thermal_dev.state);
        break;
    case 6:
        thermal_dev.state = THERMAL_LEVEL4;
        thermal_handle(thermal_dev.state);
        break;
    case 7:
        thermal_dev.state = THERMAL_LEVEL3;
        thermal_handle(thermal_dev.state);
        break;
    case 8:
        thermal_dev.state = THERMAL_LEVEL2;
        thermal_handle(thermal_dev.state);
        break;
    case 9:
        thermal_dev.state = THERMAL_LEVEL1;
        thermal_handle(thermal_dev.state);
        break;
    case 10:
        thermal_dev.state = THERMAL_LEVEL0;
        thermal_handle(thermal_dev.state);
        num = 0;
        break;
    }
}

int thermal_get_status(void)
{
    if (parameter_get_debug_temp())
        thermal_test();
    return thermal_dev.state;
}

int thermal_init(void)
{
    if (thermal_parse_cfg() <= 0)
        goto err;

    if (thermal_dev.stop_kernel_thermal && stop_kernel_thermal() <= 0) {
        thermal_dev.stop_kernel_thermal = false;
        goto err;
    }

    printf("thermal init <version:%s>\n", THERMAL_VERSION);

    return 0;
err:
    printf("thermal init fail!!!\n");
    return -1;
}
