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

#include <CameraHal/camHalTrace.h>
#include "video.h"
#include "video.hpp"
#include "nv12_3dnr.hpp"
#include "dpp_buffer.h"

extern void (*rec_event_call)(int cmd, void *msg0, void *msg1);
extern unsigned int user_noise;

NV12_3DNR::NV12_3DNR(struct Video *video)
    : DppProcess(video, "NV12_3DNR", k3DNoiseReduction, false) {}

NV12_3DNR::~NV12_3DNR() {}

static void video_dpp_packet_release(void* data)
{
    BufferBase* cam_buffer = (BufferBase*)data;
    if (cam_buffer)
        cam_buffer->decUsedCnt();
}

bool NV12_3DNR::doSomeThing(shared_ptr<BufferBase> outBuf, DppFrame* frame)
{
    Dpp_Buffer* dppBuf = reinterpret_cast<Dpp_Buffer*>(outBuf.get());
    dppBuf->createFromFrame(frame);
    dppBuf->incUsedCnt();

    return true;
}

int NV12_3DNR::dppPacketProcess(shared_ptr<BufferBase>& inBuf)
{
    DppRet ret = kSuccess;
    DppPacket* packet = nullptr;
    assert(inBuf->getMetaData());
    struct v4l2_buffer_metadata_s* metadata_drv =
        (struct v4l2_buffer_metadata_s*)inBuf->getMetaData()->metedata_drv;
    assert(metadata_drv);
    struct timeval time_val = metadata_drv->frame_t.vs_t;
    int rt = 0;
    struct HAL_Buffer_MetaData* meta = inBuf->getMetaData();

    uint32_t size = ((inBuf->getWidth() + 15) & ~15) *
                     ((inBuf->getHeight() + 15) & ~15) * 3 / 2;

    dpp::WrapBuffer* input_buffer = new dpp::WrapBuffer(inBuf->getFd(),
            inBuf->getVirtAddr(), (uint32_t)inBuf->getPhyAddr(),
            size, inBuf.get());
    if (!input_buffer) {
        printf("Create dpp::WrapBuffer failed, func=%s\n", __func__);
        rt = -1;
        goto packet_process_exit;
    }

    input_buffer->RegisterReleaseCallback(video_dpp_packet_release);

    ret = dpp_packet_init(&packet, inBuf->getWidth(), inBuf->getHeight());
    if (kSuccess != ret) {
        printf("dpp_packet_init_with_buf_info failed.\n");
        rt = -1;
        goto packet_process_exit;
    }

    dpp_packet_set_input_buffer(packet, dpp::Buffer::SharedPtr(input_buffer));
    dpp_packet_set_pts(packet, time_val);
    dpp_packet_set_noise(packet, &user_noise);
    dpp_packet_set_idc_enabled(packet, parameter_get_video_idc());
    dpp_packet_set_nr_enabled(packet, parameter_get_video_3dnr());
    dpp_packet_set_output_width(packet, mVideo->output_width);
    dpp_packet_set_output_height(packet, mVideo->output_height);

    if (meta) {
        struct ispinfo info;
        memset(&info, 0, sizeof(info));
        info.exp_gain = meta->exp_gain;
        info.exp_time = meta->exp_time;
        info.doortype = meta->awb.DoorType;
        info.wb_gain_red = 0;  // TODO, add other isp information below
        info.wb_gain_green_r = 0;
        info.wb_gain_blue = 0;
        info.wb_gain_green_b = 0;
        //info.luma_nr_en = meta->dsp_3DNR.luma_nr_en;
        //info.chroma_nr_en = meta->dsp_3DNR.chroma_nr_en;
        info.shp_en = meta->dsp_3DNR.shp_en;
        //info.luma_nr_level = meta->dsp_3DNR.luma_nr_level;
        //info.chroma_nr_level = meta->dsp_3DNR.chroma_nr_level;
        info.shp_level = meta->dsp_3DNR.shp_level;
        dpp_packet_set_private_data(packet, meta);
        dpp_packet_set_params(packet, &info, sizeof(info));
    }

    ret = mDppCore->put_packet(packet, false);
    if (kSuccess != ret) {
        printf("put_packet failed.\n");
        rt = -1;
        goto packet_process_exit;
    }

packet_process_exit:

    return rt;
}
