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

#ifndef __FACE_INTERFACE_H__
#define __FACE_INTERFACE_H__

#include <rk_fb/rk_fb.h>
#include "RSCommon.h"
#ifdef __cplusplus
extern "C" {
#endif

#define FACE_DEBUG
#ifdef FACE_DEBUG
#define FACE_DBG(...) printf(__VA_ARGS__)
#else
#define FACE_DBG(...)
#endif

#define MAX_FACE_COUNT 10
#define RS_FACE_RECOGNITION_FEATURE_DIMENSION 128

struct face_detect_object {
    int id;
    int x;
    int y;
    int width;
    int height;
	rs_point landmarks21[21];
};

struct face_detect_pos {
    int left;
    int top;
    int right;
    int bottom;
};

/*
 * blur_prob(0 - 1): Ambiguity, The smaller the number, the clearer.
 * front_prob(0 - 1): Face angle, The larger the number, the better the face angle
 */
struct face_info {
    int count;
    float blur_prob;
    float front_prob;

    struct face_detect_object objects[MAX_FACE_COUNT];
};

struct face_application_data {
    RSHandle mFaceRecognition;	
    RSHandle mLicense;
    RSHandle mDetect;
    RSHandle mFaceTrack;
    struct face_info face_result;
};

/* This structure should be the same as readsense face tracking sdk. */
struct face_output {
    int index;

    struct face_detect_pos face_pos;

    float blur_prob;
    float front_prob;

    int fr_available_flag;
    float fr_feature[RS_FACE_RECOGNITION_FEATURE_DIMENSION];
};

int face_regevent_callback(void (*call)(int cmd, void *msg0, void *msg1));

#ifdef __cplusplus
}
#endif

#endif
