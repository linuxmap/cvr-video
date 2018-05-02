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

#include "gps_ctrl.h"

#include <assert.h>
#include <fcntl.h>
#include <mutex>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <termios.h>
#include <autoconfig/main_app_autoconf.h>

extern "C" {
#include "common.h"
#include "../parameter.h"

#ifdef GPS_ZS
#include "zsgpslib.h"
#endif

#include "fs_manage/fs_manage.h"
#include "fs_manage/fs_storage.h"
}

#include "nmea_parse.h"

#ifdef GPS
#define GPS_DEV_IOC_MAGIC 'g'
#define GPS_OPEN _IO(GPS_DEV_IOC_MAGIC, 1)
#define GPS_CLOSE _IO(GPS_DEV_IOC_MAGIC, 2)

#define DEV_GPS "/dev/GPS"
#define DEV_TTY "/dev/ttyS1"
#define ZSGPS_CONF_PATH "/usr/local/bin/zsgps.conf"

#define FILE_NAME_LEN 128
#define GPS_START_FLAG "$GPSCAMTIME"
#define GPS_END_FLAG "$GPSENDTIME"

struct gps_handler {
    int ttys_fd;
    int gps_fd;
    int file_fd;
    char flag[FILE_NAME_LEN];
    char filename[FILE_NAME_LEN];
    char *storage_folder;

    struct gps_data rmc_data;
    struct gps_data gga_data;

    pthread_t read_tid;
    pthread_t write_tid;
    pthread_mutex_t write_mutex;
    send_cb send_data;
};

static struct gps_handler ggps_handler;
static int gread_done = 0;
static int gwrite_done = 0;
static int gmodule_count = 0; /* the number of modules using gps */
static pthread_mutex_t ggps_mutex = PTHREAD_MUTEX_INITIALIZER;

static int gps_open_file(char* str, unsigned short size)
{
    time_t now;
    struct tm* timenow;
    int year, mon, day, hour, min, sec;

    int flag_len = sizeof(ggps_handler.flag);
    int recode_time = parameter_get_recodetime() / 60; /* sec to min */

    time(&now);
    timenow = localtime(&now);

    year = timenow->tm_year + 1900;
    mon = timenow->tm_mon + 1;
    day = timenow->tm_mday;
    hour = timenow->tm_hour;
    min = timenow->tm_min;
    sec = timenow->tm_sec;

    snprintf(str, size, "%s/%04d%02d%02d_%02d%02d%02d_%d.%s",
             ggps_handler.storage_folder, year, mon, day, hour, min,
             sec, recode_time, "txt");

    snprintf(ggps_handler.flag, flag_len, "%s:%04d%02d%02d%02d%02d%02d\n",
             GPS_START_FLAG, year, mon, day, hour, min, sec);

    GPS_DBG("gps filename: %s\n", str);
    ggps_handler.file_fd = fs_manage_open(str, O_WRONLY | O_CREAT, 0666);
    if (ggps_handler.file_fd < 0) {
        printf("%s: %s open failed\n", __func__, ggps_handler.filename);
        return -1;
    }

    fs_manage_write(ggps_handler.file_fd, ggps_handler.flag,
                    strlen(ggps_handler.flag));

    return 0;
}

static void write_end_flag()
{
    time_t now;
    struct tm* timenow;
    int year, mon, day, hour, min, sec;

    int flag_len = sizeof(ggps_handler.flag);

    time(&now);
    timenow = localtime(&now);

    year = timenow->tm_year + 1900;
    mon = timenow->tm_mon + 1;
    day = timenow->tm_mday;
    hour = timenow->tm_hour;
    min = timenow->tm_min;
    sec = timenow->tm_sec;

    snprintf(ggps_handler.flag, flag_len, "%s:%04d%02d%02d%02d%02d%02d\n",
             GPS_END_FLAG, year, mon, day, hour, min, sec);

    fs_manage_write(ggps_handler.file_fd, ggps_handler.flag,
                    strlen(ggps_handler.flag));
}

#if USE_GPS_MOVTEXT
#include "encode_handler.h"
#include "../video.h"
#include "../av_wrapper/message_queue.h"
#include "../av_wrapper/video_common.h"
#include "../av_wrapper/encoder_muxing/packet_dispatcher.h"

static EncodedPacket* sub_pkt = NULL;
static EncodeHandler *phandler = nullptr;
static struct gps_info pkt_gps_info;
static struct text_track_amba_info pkt_track_amba_info;

extern "C" void gps_set_encoderhandler(void * handler)
{
    printf("create video EncodeHandler ok/n");
    phandler = (EncodeHandler*)handler;
}
#endif

static void send_data(const char *data, int len)
{
    int search_type = 0;
    gps_data *gps_data;

    if (strstr(data, GPS_XXRMC) != NULL) {
        search_type = GPS_RMC;
        gps_data = &ggps_handler.rmc_data;
    } else if (strstr(data, GPS_XXGGA) != NULL) {
        search_type = GPS_GGA;
        gps_data = &ggps_handler.gga_data;
    } else {
        return;
    }

    pthread_mutex_lock(&ggps_handler.write_mutex);
    if (gwrite_done) {
        memcpy(gps_data->buffer, data, len);
        gps_data->len = len;
    }
    pthread_mutex_unlock(&ggps_handler.write_mutex);

    if (ggps_handler.send_data != NULL)
        ggps_handler.send_data(data, len, search_type);
#if USE_GPS_MOVTEXT
        if(video_record_get_state() == VIDEO_STATE_RECORD){
            sub_pkt = new EncodedPacket();
            if (unlikely(!sub_pkt)) {
                printf("alloc EncodedPacket sub_pkt failed 1\n");
                assert(0);
                return;
            }
            sub_pkt->type = SUBTITLE_PACKET;
            sub_pkt->av_pkt.dts = 1;
            sub_pkt->av_pkt.pts = 1;
            sub_pkt->av_pkt.duration = 1;
            memset(&pkt_track_amba_info, 0, sizeof(pkt_track_amba_info));
            av_new_packet(&sub_pkt->av_pkt, sizeof(pkt_track_amba_info));
            gps_nmea_data_parse(&pkt_gps_info, data, search_type, len);
            text_data_copyfrom_amba_gps(&pkt_track_amba_info, &pkt_gps_info);
            memcpy(sub_pkt->av_pkt.data, &pkt_track_amba_info, sizeof(pkt_track_amba_info));
            if (phandler) {
                PacketDispatcher* pkts_dispatcher = NULL;
                pkts_dispatcher = phandler->get_pkts_dispatcher();
                pkts_dispatcher->Dispatch(sub_pkt);
            }
            sub_pkt->unref();
        }
#endif
}

static void gps_sleep(int sec, int usec)
{
    struct timeval tv;

    tv.tv_sec = sec;
    tv.tv_usec = usec;
    select(0, NULL, NULL, NULL, &tv);
}

#ifndef GPS_ZS
static int set_opt(int fd, int nSpeed, int nBits, char nEvent, int nStop)
{
    struct termios newtio, oldtio;
    if (tcgetattr(fd, &oldtio) != 0) {
        perror("SetupSerial 1");
        return -1;
    }
    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag |= CLOCAL | CREAD;
    newtio.c_cflag &= ~CSIZE;

    switch (nBits) {
    case 7:
        newtio.c_cflag |= CS7;
        break;

    case 8:
        newtio.c_cflag |= CS8;
        break;
    }

    switch (nEvent) {
    case 'O':  /* odd check */
        newtio.c_cflag |= PARENB;
        newtio.c_cflag |= PARODD;
        newtio.c_iflag |= (INPCK | ISTRIP);
        break;

    case 'E':  /* even check */
        newtio.c_iflag |= (INPCK | ISTRIP);
        newtio.c_cflag |= PARENB;
        newtio.c_cflag &= ~PARODD;
        break;

    case 'N':  /* no parity bit */
        newtio.c_cflag &= ~PARENB;
        break;
    }

    switch (nSpeed) {
    case 2400:
        cfsetispeed(&newtio, B2400);
        cfsetospeed(&newtio, B2400);
        break;

    case 4800:
        cfsetispeed(&newtio, B4800);
        cfsetospeed(&newtio, B4800);
        break;

    case 9600:
        cfsetispeed(&newtio, B9600);
        cfsetospeed(&newtio, B9600);
        break;

    case 115200:
        cfsetispeed(&newtio, B115200);
        cfsetospeed(&newtio, B115200);
        break;

    case 460800:
        cfsetispeed(&newtio, B460800);
        cfsetospeed(&newtio, B460800);
        break;

    default:
        cfsetispeed(&newtio, B9600);
        cfsetospeed(&newtio, B9600);
        break;
    }

    if (nStop == 1)
        newtio.c_cflag &= ~CSTOPB;
    else if (nStop == 2)
        newtio.c_cflag |= CSTOPB;

    newtio.c_cc[VTIME] = 0;
    newtio.c_cc[VMIN] = 100;  /* the minimum value returned */
    tcflush(fd, TCIFLUSH);    /* clear the original cache cache */

    if ((tcsetattr(fd, TCSANOW, &newtio)) != 0) {
        perror("com set error");
        return -1;
    }

    GPS_DBG("%s: set opt done!\n", __func__);
    return 0;
}

static int open_tty_dev(void)
{
    ggps_handler.ttys_fd =
        open(DEV_TTY, O_RDWR | O_NOCTTY | O_NONBLOCK);  /* O_NDELAY */
    if (ggps_handler.ttys_fd < 0) {
        printf("%s: failed to open gps tty dev\n", __func__);
        return -1;
    }

    if (set_opt(ggps_handler.ttys_fd, 9600, 8, 'N', 1) == -1) {
        printf("%s: failed to set opt\n", __func__);
        return -1;
    }

    return 0;
}

static void close_tty_dev(void)
{
    if (ggps_handler.ttys_fd >= 0) {
        close(ggps_handler.ttys_fd);
        ggps_handler.ttys_fd = -1;
    }
}

static void *gps_read_thread(void *arg)
{
    int nread;
    char buf[GPS_BUF_LEN];

    GPS_DBG("%s: \n", __func__);

    prctl(PR_SET_NAME, "gps_read_thread", 0, 0, 0);

    while (gread_done) {
        memset(buf, 0, GPS_BUF_LEN);
        for (nread = 0; nread < GPS_BUF_LEN;) {
            if (read(ggps_handler.ttys_fd, &buf[nread], 1) <= 0) {
                if (!gread_done)
                    goto exit;

                GPS_DBG("%s: wait for 200ms, then read the gps data\n", __func__);
                gps_sleep(0, 200000); /* 200ms timer */
                continue;
            }

            if (buf[nread] == '\n')
                break;

            nread++;
        }

        GPS_DBG("%s, buf: %s\n", __func__, buf);

        if ((nread == GPS_BUF_LEN - 1) &&
            (buf[nread] != '\n')) {  /* data len: 30 ~ 100 */
            printf("%s: gps read buf is too small\n", __func__);
            continue;
        }

        send_data(buf, nread + 1);
    }

exit:
    printf("%s: exit gps read thread\n", __func__);
    pthread_exit(0);
}
#endif

#ifdef GPS_ZS
static void gps_read_nmea_callBack(const char *nmea, int len)
{
    if (nmea == NULL)
        return;

    send_data(nmea, len);
}
#endif

static void* gps_write_thread(void* arg)
{
    int write_count = 0;
    struct timeval t0 = {0};
    struct timeval t1 = {0};

    GPS_DBG("%s: \n", __func__);

    prctl(PR_SET_NAME, "gps_write_thread", 0, 0, 0);

open_file:
    if (gps_open_file(ggps_handler.filename, sizeof(ggps_handler.filename)) < 0)
        gwrite_done = 0;

    while (gwrite_done) {
        gettimeofday(&t1, NULL);

        /* 1s to write 2 lines data */
        if ((t1.tv_sec * 1000000 + t1.tv_usec)
            - (t0.tv_sec * 1000000 + t0.tv_usec) >= 1000000) {
            t0 = t1;
            write_count++;

            GPS_DBG("gps write_count: %d\n", write_count);
            GPS_DBG("len: %d, rmc.buffer: %s\n", ggps_handler.rmc_data.len,
                    ggps_handler.rmc_data.buffer);
            GPS_DBG("len: %d, gga.buffer: %s\n", ggps_handler.gga_data.len,
                    ggps_handler.gga_data.buffer);

            pthread_mutex_lock(&ggps_handler.write_mutex);
            fs_manage_write(ggps_handler.file_fd, ggps_handler.rmc_data.buffer,
                            ggps_handler.rmc_data.len);
            fs_manage_write(ggps_handler.file_fd, ggps_handler.gga_data.buffer,
                            ggps_handler.gga_data.len);
            pthread_mutex_unlock(&ggps_handler.write_mutex);

            /*
             * when the number of gps info to write equal to the recording time,
             * close the current file, open a new file, rewrite
             */
            if (write_count == parameter_get_recodetime()) {
                write_count = 0;
                write_end_flag();
                fs_manage_close(ggps_handler.file_fd);
                ggps_handler.file_fd = -1;
                goto open_file;
            }
        }

        gps_sleep(0, 200000); /* 200ms timer */
    }

    printf("%s: exit gps write thread\n", __func__);
    pthread_exit(0);
}

static int gps_open(void)
{
    GPS_DBG("%s\n", __func__);

    ggps_handler.gps_fd = open(DEV_GPS, O_RDWR);
    if (ggps_handler.gps_fd < 0) {
        printf("%s: gps dev open failed, fd: %d\n", __func__, ggps_handler.gps_fd);
        return -1;
    }

    if (ioctl(ggps_handler.gps_fd, GPS_OPEN) < 0) {
        printf("%s: ioctl GPS_OPEN failed\n", __func__);
        return -1;
    }

#ifdef GPS_ZS
    if (ZS_GPSOpen(ZSGPS_CONF_PATH) != 0) {
        printf("%s: ZS_GPSOpen failed!\n", __func__);
        return -1;
    }
#else
    if (open_tty_dev() != 0) {
        printf("%s: open_tty_dev failed!\n", __func__);
        return -1;
    }
#endif

    return 0;
}

static int gps_close(void)
{
    GPS_DBG("%s\n", __func__);

#ifdef GPS_ZS
    ZS_GPSClose();
#else
    close_tty_dev();
#endif

    if (ggps_handler.gps_fd >= 0) {
        if (ioctl(ggps_handler.gps_fd, GPS_CLOSE) < 0) {
            printf("%s: ioctl GPS_CLOSE failed\n", __func__);
            return -1;
        }

        close(ggps_handler.gps_fd);
        ggps_handler.gps_fd = -1;
    }

    return 0;
}

static int gps_create_read_thread(void)
{
#ifdef GPS_ZS
    ZS_RegNmeaCB(gps_read_nmea_callBack);
#else
    if (pthread_create(&ggps_handler.read_tid, NULL, gps_read_thread, NULL)) {
        printf("%s: create gps_read pthread failed\n", __func__);
        return -1;
    }
#endif

    return 0;
}

static void gps_delete_read_thread(void)
{
    GPS_DBG("%s\n", __func__);

#ifdef GPS_ZS
    ZS_DeRegNmeaCB();
#else
    if (ggps_handler.read_tid) {
        pthread_join(ggps_handler.read_tid, NULL);
        ggps_handler.read_tid = 0;
    }
#endif
}

static void init_gps_data(gps_data *gps_data, const char *data)
{
    gps_data->len = strlen(data);
    if(gps_data->len > GPS_BUF_LEN)
        gps_data->len = GPS_BUF_LEN;

    memcpy(gps_data->buffer, data, gps_data->len);
}

static void init_parameter(send_cb gps_send_callback)
{
    memset(&ggps_handler, 0, sizeof(struct gps_handler));

    ggps_handler.gps_fd = -1;
    ggps_handler.ttys_fd = -1;
    ggps_handler.file_fd = -1;

    ggps_handler.storage_folder
        = fs_storage_folder_get_bytype(VIDEO_TYPE_ISP, GPS_TYPE);
    GPS_DBG("gps file storage folder: %s\n", ggps_handler.storage_folder);

    ggps_handler.send_data = gps_send_callback;

    init_gps_data(&ggps_handler.rmc_data, RMC_INITIAL_VALUE);
    init_gps_data(&ggps_handler.gga_data, GGA_INITIAL_VALUE);
}

static int init(send_cb gps_send_callback)
{
    GPS_DBG("%s\n", __func__);

    init_parameter(gps_send_callback);

    if (gps_open() < 0) {
        printf("%s: gps open failed!\n", __func__);
        return -1;
    }

    if (gps_create_read_thread() < 0) {
        printf("%s: gps_create_read_thread failed!\n", __func__);
        return -1;
    }

    return 0;
}

static void deinit(void)
{
    GPS_DBG("%s\n", __func__);

    ggps_handler.send_data = NULL;
    gps_delete_read_thread();

    if (gps_close() < 0) {
        printf("%s: GPS close faild!\n", __func__);
    }
}

extern "C" int gps_write_init()
{
    gwrite_done = 1;

    pthread_mutex_init(&ggps_handler.write_mutex, NULL);

    if (pthread_create(&ggps_handler.write_tid, NULL, gps_write_thread, NULL)) {
        printf("%s: create gps write pthread failed\n", __func__);
        gwrite_done = 0;
        return -1;
    }

    return 0;
}

extern "C" void gps_write_deinit()
{
    gwrite_done = 0;

    write_end_flag();

    if (ggps_handler.write_tid) {
        pthread_join(ggps_handler.write_tid, NULL);
        ggps_handler.write_tid = 0;
    }

    if (ggps_handler.file_fd >= 0) {
        fs_manage_close(ggps_handler.file_fd);
        ggps_handler.file_fd = -1;
    }

    pthread_mutex_destroy(&ggps_handler.write_mutex);
}

extern "C" int gps_init(send_cb gps_send_callback)
{
    GPS_DBG("%s\n", __func__);

    pthread_mutex_lock(&ggps_mutex);
    if (gmodule_count == 0) { /* only initialized once */
        gread_done = 1;
        if (init(gps_send_callback) < 0) {
            printf("%s: init failed\n", __func__);
            gread_done = 0;
            deinit();
            pthread_mutex_unlock(&ggps_mutex);
            return -1;
        }
    }
    gmodule_count++;
    pthread_mutex_unlock(&ggps_mutex);

    return gmodule_count;
}

extern "C" int gps_deinit(void)
{
    GPS_DBG("%s\n", __func__);

    pthread_mutex_lock(&ggps_mutex);
    gmodule_count--;
    if (gmodule_count == 0) {  /* only deinitialized once */
        gread_done = 0;
        gps_write_deinit();
        deinit();
    }
    pthread_mutex_unlock(&ggps_mutex);

    return gmodule_count;
}
#else
extern "C" int gps_init(send_cb gps_send_callback) { return -1; }

extern "C" int gps_deinit(void) { return -1; }

extern "C" int gps_write_init(void) { return -1; }

extern "C" void gps_write_deinit(void) {}

extern "C" void gps_file_open(const char *filename) {}

extern "C" void gps_file_close(void) {}
#endif
