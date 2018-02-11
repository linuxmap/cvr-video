#ifndef __COMMON_H__
#define __COMMON_H__

#include <linux/fb.h>
#include <linux/videodev2.h>
#include <stddef.h>
#include <ion/ion.h>
#include "rk_fb/rk_fb.h"

#define ALIGN(value, bits) (((value) + ((bits) - 1)) & (~((bits) - 1)))
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define UI_FRAME_1440P \
  { 2560, 1440, 30 }
#define UI_FRAME_1080P \
  { 1920, 1080, 30 }
#define UI_FRAME_720P \
  { 1280, 720, 30 }
#define UI_FRAME_576I \
  { 720, 576, 25 }
#define UI_FRAME_480I \
  { 720, 480, 30 }
#define UI_FRAME_480P \
  { 640, 480, 30 }
#define UI_FRAME_360P \
  { 480, 360, 30 }
#define UI_FRAME_240P \
  { 320, 240, 30 }

#define USE_CIF_CAMERA
#define USE_USB_WEBCAM 1

typedef signed char s8;
typedef unsigned char u8;

typedef signed short s16;
typedef unsigned short u16;

typedef signed int s32;
typedef unsigned int u32;

typedef signed long long s64;
typedef unsigned long long u64;



#define BUFFER_COUNT 4

#define FB0                         "/dev/fb0"
#define ION_DEVICE                  "/dev/ion"
#define FW_DEFAULT_MOUNT_FULLNAME   "/mnt/sdcard/Firmware.img"

#define CIF_TYPE_SENSOR 0
#define CIF_TYPE_CVBS   1
#define CIF_TYPE_MIX    2
#define USB_TYPE_YUYV 0
#define USB_TYPE_MJPEG 1
#define USB_TYPE_H264 2
#define VIDEO_TYPE_ISP 3
#define VIDEO_TYPE_CIF 4
#define VIDEO_TYPE_USB 5

#define CAMARE_FREQ_50HZ 1
#define CAMARE_FREQ_60HZ 2
#define LCD_BACKLT_L 150
#define LCD_BACKLT_M 200
#define LCD_BACKLT_H 250

#define COLLI_CLOSE   0
#define COLLI_LEVEL_L 1
#define COLLI_LEVEL_M 2
#define COLLI_LEVEL_H 3
//#define COLLI_LEVEL_L 6
//#define COLLI_LEVEL_M 10
//#define COLLI_LEVEL_H 13

#define KEY_VOLUME_LEVEL_0 0
#define KEY_VOLUME_LEVEL_1 1
#define KEY_VOLUME_LEVEL_2 2
#define KEY_VOLUME_LEVEL_3 3

#define VIDEO_QUALITY_HIGH   9
#define VIDEO_QUALITY_MID    7
#define VIDEO_QUALITY_LOW    5

#ifdef CVR
#define _MAX_FRONT_VIDEO_RES        2
#else
#define _MAX_FRONT_VIDEO_RES        5
#endif
#define _MAX_BACK_VIDEO_RES         2
#define _MAX_CIF_VIDEO_RES          1
#define _MAX_PHO_RES                4

#define VIDEO_AUTO_OFF_SCREEN_ON        (15)    // 15 Seconds
#define VIDEO_AUTO_OFF_SCREEN_OFF       (-1)

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

struct v4l2_info {
    struct v4l2_buffer buf;
    struct v4l2_capability cap;
    struct v4l2_fmtdesc fmtdesc;
    struct v4l2_format fmt;
    struct v4l2_requestbuffers reqbuf;
    enum v4l2_buf_type type;
};

struct video_buf {
    void* start;
    size_t length;
};

struct video_param {
    unsigned short width;
    unsigned short height;
    unsigned short fps;
};

struct photo_param {
    int width;
    int height;
    unsigned int max_num;
};

/* Firmware Status */
enum {
    FW_OK = 0,
    FW_NOTFOUND,
    FW_INVALID,
};

enum target_type {
    TYPE_WIFI,
    TYPE_LOCAL,
    TYPE_BROADCAST,
};

struct public_message {
    unsigned int id;
    unsigned int type;
};

#endif
