/**
 * Copyright (C) 2016 Fuzhou Rockchip Electronics Co., Ltd
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

#ifndef CVR_VIDEO_INTERFACE_H__
#define CVR_VIDEO_INTERFACE_H__

#include <stddef.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* custom interface begin */

typedef struct __sensor_attr_s {
    int width;  // resolution
    int height;
    int fps;   // frame rate
    int freq;  // 50, 60
    // TODO, other settings
} SensorAttr;

typedef struct __media_handle {
    void* video_node;
    int channel_num;  // 0, initial stream; 1, scaled small stream;
    // 2, initial resolution jpeg; 3, thumbtail jpeg
} MediaHandle;

typedef enum {
    INVALID_FRAME = 0,
    JPEG_FRAME,
    I_FRAME,
    P_FRAME,
    B_FRAME
} FrameType;

struct SPSPPSInfo {
    const void* const data;  // sps and pps info
    int data_size;
};

struct JPEGInfo {
    int width;
    int height;
};

typedef struct __vid_stream {
    FrameType frm_type;
    void* const buf_addr;
    int buf_size;
    struct timeval time_val;

    union {
        struct SPSPPSInfo sps_pps_info;
        struct JPEGInfo jpeg_info;
    } ExtraInfo;
} VEncStreamInfo;

// A callback for notify coming new encoded stream.
// Note: The buffer in VEncStreamInfo must not be holding for long,
//       copy it or read it as quickly as possiable.
typedef void (*NotifyNewEncStream)(VEncStreamInfo* enc_stream_info);

// Init MediaHandle, and start isp camera with input sensor attribute.
// Function sensor_init(sensor_attr_s * pointer) is not needed.
int media_sys_init(MediaHandle* handle, SensorAttr* sensor_attr);

// Stop isp camera and deinit all resource.
int media_sys_uninit(MediaHandle* handle);

// Create and Bind stream receiver to channel, if there is coming new encoded
// stream, the NotifyNewEncStream callback in reveiver will be called.
int create_enc_stream_receiver(MediaHandle* handle,
                               int channel,
                               void** out_receiver,
                               NotifyNewEncStream new_stream_cb);

void destroy_enc_stream_receiver(MediaHandle* handle,
                                 int channel,
                                 void* receiver);

// The initial resolution channel 0,2 cannot be setting in this interface.
// width and height should be aligned to 16, means that Value%16==0.
int venc_set_resolution(MediaHandle* handle,
                        int channel,
                        int width,
                        int height);

// frame_rate between 1 and 60
int venc_set_frame_rate(MediaHandle* handle, int channel, int frame_rate);

// bit rate should be less than 60Mbps
int venc_set_bit_rate(MediaHandle* handle, int channel, int bit_rate);

// set the I frame interval
int venc_set_gop_size(MediaHandle* handle, int channel, int gop_size);

// set profile, rk only support:
//              H264_PROFILE_BASELINE(66),
//              H264_PROFILE_MAIN(77),
//              H264_PROFILE_HIGH(100)
int venc_set_profile(MediaHandle* handle, int channel, int profile);

// start encoding frame on channel xx, and passing to receiver
int venc_start_stream(MediaHandle* handle, int channel);

// stop encoding frame on channel xx, no need venc_destroy interface
int venc_stop_stream(MediaHandle* handle, int channel);

// force generating an idr frame, can take effect without stopping stream
int venc_require_an_idr(MediaHandle* handle, int channel);

// As jpeg is not encoding all the time with frame rate,
// explicitly request an encoding.
int jpegenc_request_encode_one_frame(MediaHandle* handle, int channel);

/* custom interface end */

#ifdef __cplusplus
}
#endif

#endif  // CVR_VIDEO_INTERFACE_H__
