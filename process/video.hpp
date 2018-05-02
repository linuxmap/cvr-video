/**
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd
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

#ifndef __VIDEO_HPP__
#define __VIDEO_HPP__

#include <CameraHal/CamCifDevHwItf.h>
#include <CameraHal/CamHalVersion.h>
#include <CameraHal/CamHwItf.h>
#include <CameraHal/CamIsp11DevHwItf.h>
#include <CameraHal/CamUSBDevHwItfImc.h>
#include <CameraHal/BufferBase.h>
#include <CameraHal/CameraIspTunning.h>
#include <CameraHal/linux/v4l2-controls.h>
#include <CameraHal/linux/media/rk-isp11-config.h>
#include <CameraHal/linux/media/v4l2-config_rockchip.h>
#include <CameraHal/IonCameraBuffer.h>
#include <CameraHal/StrmPUBase.h>
#include <iostream>

#include <linux/videodev2.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <unistd.h>

extern "C" {
#include "common.h"
#include <iep/iep.h>
#include <iep/iep_api.h>
#include "fs_manage/fs_storage.h"
#include "fs_manage/fs_manage.h"
#include <fs_manage/fs_picture.h>
#include "huffman.h"
#include "parameter.h"
#include "rk_rga/rk_rga.h"
#include "video_ion_alloc.h"
#include "vpu.h"

#include <jpeglib.h>

#include "watermark.h"
#include "power/thermal.h"
#include "ui_resolution.h"

#include <rk_fb/rk_fb.h>
#include "public_interface.h"
#include "show.h"
#include "parameter.h"
}

#include <dpp/common/error.h>
#include <dpp/dpp_frame.h>
#include <dpp/dpp_packet.h>
#include <dpp/core/core.h>
#include <dvs/dvs.h>

#include "face_detect_process.h"
#include "encode_handler.h"
#include "gyrosensor.h"
#include "jpeg_header.h"
#include "video_interface.h"
#include "uvc/uvc_user.h"

#include "av_wrapper/handler/audio_encode_handler.h"
#include "handler/scale_encode_ts_handler.h"
#include "video.h"
#include "display.h"
#include "gps/gps_ctrl.h"
#include "uvc/uvc_encode.h"

#include <autoconfig/main_app_autoconf.h>

using namespace rockchip;

// TODO: no need limit the number of video devices
#define MAX_VIDEO_DEVICE 30

//#define NV12_RAW_DATA
#define NV12_RAW_CNT 3

#define STACKSIZE (256 * 1024)

#define FRONT "isp"
#define CIF "cif"
#define USB_FMT_TYPE HAL_FRMAE_FMT_MJPEG

class YUYV_NV12_Stream;
class NV12_Display;
class MJPG_Photo;
class MJPG_NV12;
class NV12_Encode;
class H264_Encode;
class MP_DSP;
class NV12_MJPG;
class NV12_RAW;
class NV12_ADAS;
class NV12_IEP;
class NV12_TS;
class NV12_UVC;
class NV12_MIRROR;
class NV12_FLIP;
class MP_RGA;
class NV12_3DNR;
class NV12_DVS;
class TRANSPARENT;
class NV12_Encode_S;
class DPP_NV12_Encode_S;

struct hal {
    shared_ptr<CamHwItf> dev;
    shared_ptr<CamHwItf::PathBase> mpath;
    shared_ptr<CamHwItf::PathBase> spath;
    shared_ptr<YUYV_NV12_Stream> yuyv_nv12;
    shared_ptr<NV12_Display> nv12_disp;
    shared_ptr<MJPG_Photo> mjpg_photo;
    shared_ptr<MJPG_NV12> mjpg_nv12;
    shared_ptr<NV12_Encode> nv12_enc;
    shared_ptr<H264_Encode> h264_enc;
    shared_ptr<IonCameraBufferAllocator> bufAlloc;
    shared_ptr<MP_DSP> mp_dsp;
    shared_ptr<NV12_MJPG> nv12_mjpg;
    shared_ptr<NV12_RAW> nv12_raw;
    shared_ptr<NV12_IEP> nv12_iep;
    shared_ptr<NV12_TS> nv12_ts;
    shared_ptr<NV12_UVC> nv12_uvc;
    shared_ptr<NV12_MIRROR> nv12_mirr;
    shared_ptr<NV12_FLIP> nv12_flip;

    shared_ptr<MP_RGA> mp_rga;
    shared_ptr<NV12_MJPG> rga_mjpg;
    shared_ptr<NV12_Encode> rga_enc;

    shared_ptr<NV12_3DNR> mp_3dnr;
    shared_ptr<NV12_DVS> mp_dvs;
    shared_ptr<TRANSPARENT> dsp_trans;
    shared_ptr<NV12_Display> dnr_disp;
    shared_ptr<NV12_Encode> dnr_enc;
    shared_ptr<NV12_MJPG> dnr_mjpg;
    shared_ptr<NV12_UVC> dnr_uvc;
#if MAIN_APP_NEED_DOWNSCALE_STREAM
    shared_ptr<DPP_NV12_Encode_S> dnr_enc_s;
    shared_ptr<MP_RGA> sp_rga;
    shared_ptr<NV12_Encode_S> rga_enc_s;
#endif
#ifdef ENABLE_RS_FACE
    shared_ptr<FaceDetectProcess> nv12_face_detect;
#endif
    shared_ptr<NV12_Display> sp_disp;
    shared_ptr<NV12_Display> mp_disp;
    shared_ptr<NV12_ADAS> sp_adas;
};

typedef struct ispinfo {
    float exp_gain;
    float exp_time;
    int doortype;

    unsigned char exp_mean[25];

    float wb_gain_red;
    float wb_gain_green_r;
    float wb_gain_blue;
    float wb_gain_green_b;

    unsigned char luma_nr_en;
    unsigned char chroma_nr_en;
    unsigned char shp_en;
    unsigned char luma_nr_level;
    unsigned char chroma_nr_level;
    unsigned char shp_level;

    int reserves[16];
} ispinfo_t;

enum photo_state { PHOTO_ENABLE, PHOTO_BEGIN, PHOTO_END, PHOTO_DISABLE };

struct video_photo {
    enum photo_state state;
    struct video_ion rga_photo;
    struct video_ion rga_photo_ext;
    struct vpu_encode *encode;
    struct vpu_encode *encode_ext;
    pthread_t pid;
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    int rga_fd;
    unsigned int num;
    unsigned int user_num;
    unsigned int max_num;
    struct watermark_info watermark;
    int width;
    int height;
};

struct video_jpeg_dec {
    struct vpu_decode* decode;
    bool decoding;
};

struct dvs_video_frame {
    struct video_in_data video_info;
    shared_ptr<BufferBase> frame_buffer;
};

struct dvs_data {
    struct dvs_context *context;
    dpp::DppCore *dpp;
    uint32_t frame_count;
    std::list<struct dvs_video_frame*> frames;
    pthread_mutex_t frame_list_mutex;
    struct sensor_data_set sensor_data;
    struct video_ion rotation_ion[ROTATION_BUFFER_NUM];
    float *rotation[ROTATION_BUFFER_NUM];
    struct gyro_data* gyros;
    struct quat_data* quats;
    int output_width;
    int output_height;
    int enabled;
};

extern "C" void storage_setting_event_callback(int cmd, void *msg0, void *msg1);

struct Video {
    unsigned char businfo[32];
    int width;
    int height;

    struct hal* hal;

    struct Video* pre;
    struct Video* next;

    int type;
    int usb_type;
    int cif_type;
    pthread_t record_id;
    int deviceid;
    volatile int pthread_run;
    char save_en;

    bool mp4_encoding;

    pthread_mutex_t record_mutex;
    pthread_cond_t record_cond;

    frm_info_t frmFmt;
    frm_info_t spfrmFmt;

    // Means the video starts successfully with all below streams.
    // We need know whether the video is valid in some critical cases.
    volatile bool valid;
    // Maintain the encode status internally, user call interfaces too wayward.
    // Service the start_xx/stop_xx related to encode_hanlder.
    uint32_t encode_status;
#define RECORDING_FLAG          0x00000001
#define WIFI_TRANSFER_FLAG      0x00000002
#define CACHE_ENCODE_DATA_FLAG  0x00000004
#define MD_PROCESSING_FLAG      0x00000008
    EncodeHandler* encode_handler;
    struct watermark_info watermark;

    // Transfer stream handler.
    ScaleEncodeTSHandler* ts_handler;

    bool cachemode;

    int fps_total;
    int fps_last;
    int fps;

    bool video_save;
    bool front;

    bool high_temp;

    int fps_n;
    int fps_d;

#ifdef NV12_RAW_DATA
    struct video_ion raw[NV12_RAW_CNT];
    int raw_fd;
#endif
#ifdef _PCBA_SERVICE_
    struct video_ion pcba_ion;
    int pcba_cif_type;
#endif

    struct video_photo photo;

    struct video_jpeg_dec jpeg_dec;

    struct video_param video_param[2];
    int disp_position;

    int output_width;
    int output_height;

    struct uvc_encode* uvc_enc;

    bool path[5];

    struct dvs_data *dvs_data;

#if MAIN_APP_NEED_DOWNSCALE_STREAM
    bool enc_scale;
    EncodeHandler* encode_handler_s;
#endif

    bool out_exist;
    bool out_state;
};

void video_record_signal(struct Video* video);
int h264_encode_process(struct Video* video,
                        void* virt,
                        int fd,
                        void* hnd,
                        size_t size,
                        struct timeval& time_val,
                        PixelFormat fmt = PIX_FMT_NV12);
void video_record_takephoto_end(struct Video* video);
int video_try_format(struct Video* video, frm_info_t& in_frmFmt);
void video_h264_save(struct Video* video);
int video_init_setting(struct Video* video);
bool video_record_h264_have_added(void);
int video_set_fps(struct Video* video, int numerator, int denominator);
EncodeHandler* create_encode_handler(struct Video* video, int w, int h);
void destroy_encode_handler(EncodeHandler *&encode_handler);

#endif
