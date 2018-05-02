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

#include "nv12_adas.hpp"

static void (*adas_event_call)(int cmd, void *msg0, void *msg1);

static void video_fcw_packet_release(void* data)
{
    BufferBase* cam_buffer = (BufferBase*)data;
    if (cam_buffer)
        cam_buffer->decUsedCnt();
}

static void video_ldw_packet_release(void* data)
{
    struct video_ion** curr_ldw = (struct video_ion**)data;
    *curr_ldw = nullptr;
}

static int video_fcw_packet_process(struct Video* video,
                                    shared_ptr<BufferBase>& inBuf,
                                    dpp::DppCore* dpp, struct video_ion** ldw)
{
    DppRet ret = kSuccess;
    DppPacket* packet = nullptr;
    int rt = 0;

    dpp::WrapBuffer* input_buffer = new dpp::WrapBuffer(inBuf->getFd(),
            inBuf->getVirtAddr(), (uint32_t)inBuf->getPhyAddr(),
            inBuf->getWidth() * inBuf->getHeight() * 3 / 2, inBuf.get());
    if (!input_buffer) {
        printf("Create dpp::WrapBuffer failed, func=%s\n", __func__);
        rt = -1;
        goto adas_packet_exit;
    }
    input_buffer->RegisterReleaseCallback(video_fcw_packet_release);

    ret = dpp_packet_init(&packet, inBuf->getWidth(), inBuf->getHeight());
    if (kSuccess != ret) {
        printf("dpp_packet_init failed.\n");
        rt = -1;
        goto adas_packet_exit;
    }

    dpp_packet_set_input_buffer(packet, dpp::Buffer::SharedPtr(input_buffer));
    dpp_packet_set_adas_mode(packet, ADAS_MODE_DAY);
    dpp_packet_set_ldw_enabled(packet, false);
    dpp_packet_set_odt_enabled(packet, true);
    ret = dpp->put_packet(packet, true);
    if (kSuccess != ret) {
        printf("put_packet failed.\n");
        rt = -1;
        goto adas_packet_exit;
    }

adas_packet_exit:

    return rt;
}

static int video_ldw_packet_process(struct Video* video,
                                    struct video_ion* in, dpp::DppCore* dpp, struct video_ion** ldw)
{
    DppRet ret = kSuccess;
    DppPacket* packet = nullptr;
    int rt = 0;

    dpp::WrapBuffer* input_buffer = new dpp::WrapBuffer(in->fd,
            in->buffer, (uint32_t)in->phys,
            in->width * in->height * 3 / 2, ldw);
    if (!input_buffer) {
        printf("Create dpp::WrapBuffer failed, func=%s\n", __func__);
        rt = -1;
        goto adas_packet_exit;
    }
    input_buffer->RegisterReleaseCallback(video_ldw_packet_release);

    ret = dpp_packet_init(&packet, in->width, in->height);
    if (kSuccess != ret) {
        printf("dpp_packet_init_with_buf_info failed.\n");
        rt = -1;
        goto adas_packet_exit;
    }

    dpp_packet_set_input_buffer(packet, dpp::Buffer::SharedPtr(input_buffer));
    dpp_packet_set_adas_mode(packet, ADAS_MODE_DAY);
    dpp_packet_set_ldw_enabled(packet, true);
    dpp_packet_set_odt_enabled(packet, false);
    ret = dpp->put_packet(packet, false);
    if (kSuccess != ret) {
        printf("put_packet failed.\n");
        rt = -1;
        goto adas_packet_exit;
    }

adas_packet_exit:

    return rt;
}

extern "C" int ADAS_RegEventCallback(void (*call)(int cmd, void *msg0, void *msg1))
{
    adas_event_call = call;
    return 0;
}

NV12_ADAS::NV12_ADAS(struct Video *video)
    : DppProcess(video, "NV12_ADAS", kFrontCollisionWarning, false)
{
    printf("%s:%d enter\n", __func__, __LINE__);

    if ((rga_fd = rk_rga_open()) < 0)
        printf("%s open rga device failed!\n", __func__);

    // Create buffers for ADAS.
    memset (&ldw_input, 0, sizeof(struct video_ion));
    ldw_input.client = -1;
    ldw_input.fd = -1;
    if (video_ion_alloc(&ldw_input, ADAS_LDW_WIDTH, ADAS_LDW_HEIGHT))
        printf("%s ADAS LDW ion alloc fail!\n", __func__);
    curr_ldw = NULL;

    printf("%s:%d exit\n", __func__, __LINE__);
}

NV12_ADAS::~NV12_ADAS()
{
    printf("%s:%d enter\n", __func__, __LINE__);

    video_ion_free(&ldw_input);

    rk_rga_close(rga_fd);
    rga_fd = -1;

    printf("%s:%d exit\n", __func__, __LINE__);
}

int NV12_ADAS::dppPacketProcess(shared_ptr<BufferBase>& inBuf)
{
    if (video_fcw_packet_process(mVideo, inBuf, mDppCore, &curr_ldw))
        return -1;

    // LDW request.
    if (!curr_ldw && rga_fd >= 0 && ldw_input.buffer) {
        int src_fd, src_w, src_h, dst_fd, dst_w, dst_h;
        curr_ldw = &ldw_input;
        src_fd = (int)(inBuf->getFd());
        src_w = inBuf->getWidth();
        src_h = inBuf->getHeight();
        dst_fd = curr_ldw->fd;
        dst_w = curr_ldw->width;
        dst_h = curr_ldw->height;
        rk_rga_ionfd_to_ionfd_scal(rga_fd,
                                   src_fd, src_w, src_h,
                                   RGA_FORMAT_YCBCR_420_SP,
                                   dst_fd, dst_w, dst_h,
                                   RGA_FORMAT_YCBCR_420_SP,
                                   0, 0, dst_w, dst_h, src_w, src_h);
        video_ldw_packet_process(mVideo, curr_ldw, mDppCore, &curr_ldw);
    }

    return 0;
}

bool NV12_ADAS::processFrame()
{
    DppFrame* frame = nullptr;
    DppRet ret = mDppCore->get_frame(&frame);
    struct adas_output* adas_output = nullptr;

    if (!frame) {
        if (ret == kErrorTimeout) {
            printf("Get frame from dpp failed cause by timeout.\n");
            return true;
        } else {
            printf("Get frame failed, DPP request exit.\n");
            return false;
        }
    }

    if (dpp_frame_get_type(frame) == kFrontCollisionWarning) {
        adas_output = dpp_frame_get_adas_output(frame);
        memcpy(&output, adas_output, sizeof(*adas_output));
        if (adas_event_call)
            (*adas_event_call)(0, (void *)&output, (void *)0);
    }

    dpp_frame_deinit(frame);

    return true;
}
