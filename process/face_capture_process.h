/**
 * Copyright (C) 2018 Fuzhou Rockchip Electronics Co., Ltd
 * Author: frank <frank.liu@rock-chips.com>
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

#ifndef __FACE_CAPTURE_PROCESS_H__
#define __FACE_CAPTURE_PROCESS_H__

#include <CameraHal/StrmPUBase.h>
#include "face_interface.h"
#include "encode_handler.h"
#include "ui/face/display_face.h"
#include "readsense_face_recognition_sdk.h"
#include "serial_com.h"
#include "RSFaceSDK.h"
#define FACE_WIDTH 128
#define FACE_HEIGHT 128
#define FACE_WIN_TIMEOUT 1800
#define FACE_REPLACE_THRESHOLD 0.05
#define FACE_WINDOW_NUM 1

struct act_object {
    int act_x;
    int act_y;
    int act_w;
    int act_h;
};

struct face_win {
    int id;
    int pos;
    int state;
    int timeout;
    struct video_ion ion;
};

/*
 * clear(0 - 1): The larger the number, the clearer the face
 * front(0 - 1): The larger the number, the better the face angle
 */
struct face_jpeg_cache {
    int id;
    int width;
    int height;
    float clear;
    float front;
    shared_ptr<BufferBase> jpeg;
};

class FaceCaptureProcess final : public StreamPUBase
{
public:
    FaceCaptureProcess(struct Video* video);
    virtual ~FaceCaptureProcess();
    virtual bool processFrame(shared_ptr<BufferBase> inBuf, shared_ptr<BufferBase> outBuf);
    bool get_replace_state(float blur, float clear, int id);
	static void* SerialProcess(void* __this);
	int SerialCMDProcess(int fd, void* frame);

private:
    int rga_fd;
    int dsp_fd;
	int serial_fd;
	pthread_t pid;
	char rcv_buf[BUFFER_LEN];
	char ring_buf[1024];
	int read_pos;
	int write_pos;
    struct video_ion rga_dst_buf;
    struct video_ion model_ion;
    struct video_ion dst_ion;
    std::list<face_jpeg_cache*> face_jpeg_cache_list;
    RSHandle mFaceRecognition;	
    RSHandle mLicense;
	int registerflag;
};

#endif
