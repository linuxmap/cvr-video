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

#ifdef ENABLE_RS_FACE

#include "video.hpp"
#include "face_detect_process.h"
#include "dpp_buffer.h"
#include "public_interface.h"
#include "face_interface.h"

void (*face_event_call)(int cmd, void *msg0, void *msg1);

extern "C" int face_regevent_callback(void (*call)(int cmd, void *msg0, void *msg1))
{
    face_event_call = call;
    return 0;
}

FaceDetectProcess::FaceDetectProcess(struct Video* video)
    : DppProcess(video, "FaceDetectProcess", kRsFaceDetection, true)
{
    FACE_DBG( "%s:%d enter\n", __func__, __LINE__);
}

FaceDetectProcess::~FaceDetectProcess()
{
    FACE_DBG("%s:%d enter\n", __func__, __LINE__);
}

static void face_detect_packet_release(void* data)
{
    BufferBase* cam_buffer = (BufferBase*)data;
    if (cam_buffer)
        cam_buffer->decUsedCnt();
}

bool FaceDetectProcess::doSomeThing(shared_ptr<BufferBase> outBuf, DppFrame* frame)
{
    if (dpp_frame_get_type(frame) == kRsFaceDetection) {
        dpp::Buffer::SharedPtr buffer = dpp_frame_get_output_buffer(frame);
        int* face_buf = (int*)buffer->address();

        struct face_application_data *app_data = (struct face_application_data*)outBuf->getPrivateData();
        if (app_data == NULL) {
            /* app_data will be freed in freeAndSetPrivateData. */
            app_data = (struct face_application_data*)malloc(sizeof(struct face_application_data));
            memset(app_data, 0, sizeof(struct face_application_data));
            outBuf->freeAndSetPrivateData(app_data);
        }

        struct face_info* info = (struct face_info*)calloc(1, sizeof(*info));

        if (info == NULL) {
            FACE_DBG("face_info calloc error.\n");
            return false;
        }

        if (face_buf[0] > 0) {
            struct face_output* output = (struct face_output*)(face_buf + 1);

            FACE_DBG("face_buf[0] = %d\n", face_buf[0]);
            info->count = (face_buf[0] > MAX_FACE_COUNT ? MAX_FACE_COUNT : face_buf[0]);

            for (int i = 0; i < info->count; i++) {
                FACE_DBG("Face %d position is: [%d, %d, %d, %d]\n", i,
                       output[i].face_pos.left, output[i].face_pos.top,
                       output[i].face_pos.right, output[i].face_pos.bottom);

                info->objects[i].id = output[i].index;
                info->objects[i].x = output[i].face_pos.left;
                info->objects[i].y = output[i].face_pos.top;
                info->objects[i].width = output[i].face_pos.right - output[i].face_pos.left;
                info->objects[i].height = output[i].face_pos.bottom - output[i].face_pos.top;
                info->blur_prob = output[i].blur_prob;
                info->front_prob = output[i].front_prob;
            }
        } else {
            info->count = 0;
        }

        app_data->face_result = *info;

        if (face_event_call)
            (*face_event_call)(0, (void *)info, (void *)0);
    } else {
        FACE_DBG("debug: not face frame????\n");
    }

    return true;
}

int FaceDetectProcess::dppPacketProcess(shared_ptr<BufferBase>& inBuf)
{
    int ret = kSuccess;
    DppPacket* packet = nullptr;
    int rt = 0;

    dpp::WrapBuffer* input_buffer = new dpp::WrapBuffer(inBuf->getFd(),
            inBuf->getVirtAddr(), (uint32_t)inBuf->getPhyAddr(),
            inBuf->getWidth() * inBuf->getHeight() * 3 / 2, inBuf.get());
    if (!input_buffer) {
        FACE_DBG("Create dpp::WrapBuffer faild, func = %s.\n", __func__);
        rt = -1;
        goto exit;
    }

    input_buffer->RegisterReleaseCallback(face_detect_packet_release);

    ret = dpp_packet_init(&packet, inBuf->getWidth(), inBuf->getHeight());
    if (kSuccess != ret) {
        FACE_DBG("dpp_packet_init failed.\n");
        rt = -1;
        goto exit;
    }

    dpp_packet_set_input_buffer(packet, dpp::Buffer::SharedPtr(input_buffer));

    ret = mDppCore->put_packet(packet, false);
    if (kSuccess != ret) {
        FACE_DBG("put_packet failed.\n");
        rt = -1;
        goto exit;
    }
exit:
    return rt;
}

#endif

