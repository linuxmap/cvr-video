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
#include "nv12_dvs.hpp"
#include "dpp_buffer.h"

#ifdef SDV

static void video_dvs_packet_release(void* data)
{
    // Release camera buffer reference.
    CameraBuffer* cam_buffer = (CameraBuffer*)data;
    if (cam_buffer) {
        rotation_data *output = (rotation_data*)cam_buffer->getPrivateData();
        if (output && output->dvs_context)
            dvs_set_rotation_buffer_available(output->dvs_context, output->buff_idx);
        cam_buffer->decUsedCnt();
    }
}

static int dvs_on_rotation(struct dvs_context *context, rotation_data *output)
{
    DppPacket* packet = nullptr;
    shared_ptr<BufferBase> inBuf;
    struct v4l2_buffer_metadata_s* meta;

    if (!output) {
        printf("Fatal error, DVS invalid output\n");
        return 0;
    }

    output->dvs_context = context;

    struct dvs_data* dvs = (struct dvs_data*)context->user_data;
    if (!dvs || !dvs->context) {
        printf("dvs_on_rotation when DVS context has been destroyed!\n");
        return 0;
    }

    // Find the relative video frame.
    struct dvs_video_frame* dvs_frame = nullptr;
    pthread_mutex_lock(&dvs->frame_list_mutex);
    std::list<struct dvs_video_frame*>::iterator it = dvs->frames.begin();
    while (it != dvs->frames.end()) {
        if ((*it)->video_info.frame_index == output->frame_index) {
            dvs_frame = *it;
            dvs->frames.erase(it);
            break;
        }
        ++it;
    }
    pthread_mutex_unlock(&dvs->frame_list_mutex);

    if (!dvs_frame) {
        printf("Cannot find a relative frame in dvs frame list, id=%d\n",
               output->frame_index);
        dvs_set_rotation_buffer_available(dvs->context, output->buff_idx);
        return 0;
    }

    // Create packet then send to dpp.
    inBuf = dvs_frame->frame_buffer;
    meta = (struct v4l2_buffer_metadata_s*)inBuf->getMetaData()->metedata_drv;

    uint32_t size = ((inBuf->getWidth() + 15) & ~15) *
                     ((inBuf->getHeight() + 15) & ~15) * 3 / 2;

    dpp::WrapBuffer* input_buffer = new dpp::WrapBuffer(inBuf->getFd(),
            inBuf->getVirtAddr(), (uint32_t)inBuf->getPhyAddr(),
            size, inBuf.get());
    if (!input_buffer) {
        printf("Create dpp::WrapBuffer failed, func=%s\n", __func__);
        return 0;
    }

    input_buffer->RegisterReleaseCallback(video_dvs_packet_release);
    inBuf->setPrivateData(output);

    dpp_packet_init(&packet, inBuf->getWidth(), inBuf->getHeight());
    if (!packet) {
        if (dvs_frame)
            delete dvs_frame;
        dvs_set_rotation_buffer_available(dvs->context, output->buff_idx);
        return 0;
    }

    if (meta)
        dpp_packet_set_pts(packet, meta->frame_t.vs_t);
    dpp_packet_set_input_buffer(packet, dpp::Buffer::SharedPtr(input_buffer));
    dpp_packet_set_output_width(packet, dvs->output_width);
    dpp_packet_set_output_height(packet, dvs->output_height);
    dpp_packet_set_dvs_remap(packet, dvs->rotation_ion[output->buff_idx].phys);
    dpp_packet_set_downscale_enabled(packet, 1);
    dpp_packet_set_downscale_width(packet, DOWN_SCALE_WIDTH);
    dpp_packet_set_downscale_height(packet, DOWN_SCALE_HEIGHT);

    dvs->dpp->put_packet(packet, false);
    delete dvs_frame;

    return 0;
}

static int dvs_on_stop(struct dvs_context *context)
{
    printf("dvs on stop\n");

    return 0;
}

int NV12_DVS::video_dvs_init(struct Video* video)
{
    // Create DVS context.
    struct dvs_config_param dvs_config;
    struct dvs_context* context = NULL;
    dvs_video_resolution video_resolution;

    video->dvs_data = new dvs_data();
    if (!video->dvs_data)
        return -1;
    video->dvs_data->context = NULL;
    video->dvs_data->gyros = NULL;
    for (int i = 0; i < ROTATION_BUFFER_NUM; i++) {
        struct video_ion* rotation_ion = &video->dvs_data->rotation_ion[i];
        memset(rotation_ion, 0, sizeof(*rotation_ion));
        rotation_ion->client = -1;
        rotation_ion->fd = -1;
    }
    memset(&video->dvs_data->sensor_data, 0, sizeof(video->dvs_data->sensor_data));

    memset(&dvs_config, 0, sizeof(dvs_config));

    dvs_config.filter_mode = 0;
    dvs_config.rs_mode = 0;
    dvs_config.dmp_mode = 0;
    dvs_config.black_border_mode = 80;
    (dvs_config.wd)[0] = 0;
    (dvs_config.wd)[1] = 0;
    (dvs_config.wd)[2] = 0;
    dvs_config.frame_mem_num = UNDEFINED_NUM;
    video_resolution.width = video->width;
    video_resolution.height = video->height;
    video_resolution.dst_width = video->output_width;
    video_resolution.dst_height = video->output_height;
    video_resolution.dvs_fps = video->fps_d / video->fps_n;
    video->dvs_data->output_width = video->output_width;
    video->dvs_data->output_height = video->output_height;

    dvs_init(&context, &dvs_config, &video_resolution);
    if (!context) {
        printf("Create DVS context failed.\n");
        return -1;
    }

    pthread_mutex_init(&video->dvs_data->frame_list_mutex, NULL);

    dvs_register_onrotation(context, dvs_on_rotation);
    dvs_register_onstop(context, dvs_on_stop);

    for (int i = 0; i < ROTATION_BUFFER_NUM; i++) {
        struct video_ion* rotation_ion = &video->dvs_data->rotation_ion[i];
        // DVS remap buffer size should be 200 * 200 * 16.
        if (video_ion_alloc(rotation_ion, 200, 200 * 16)) {
            printf("DVS rotation buffer ion alloc failed.\n");
            return -1;
        }
        dvs_prepare_dpp_data((char*)(rotation_ion->buffer), context, i);
    }

    video->dvs_data->gyros =
        (struct gyro_data*)malloc(sizeof(struct gyro_data) * MAX_NUM);
    if (!video->dvs_data->gyros) {
        printf("Allocate gyro buffer failed.\n");
        goto err;
    }

    gyrosensor_start();

    // Start DVS algorithm work thread.
    context->user_data = video->dvs_data;
    video->dvs_data->frame_count = 1;
    dvs_start(context);
    video->dvs_data->context = context;

    return 0;

err:
    if (video->dvs_data->gyros) {
        free(video->dvs_data->gyros);
        video->dvs_data->gyros = NULL;
    }

    for (int i = 0; i < ROTATION_BUFFER_NUM; i++)
        video_ion_free(&video->dvs_data->rotation_ion[i]);

    dvs_deinit(context);
    return -1;
}

int NV12_DVS::video_dvs_deinit(struct Video* video)
{
    if (!video || !video->dvs_data)
        return -1;

    struct dvs_data* dvs = video->dvs_data;

    dvs_deinit(dvs->context);

    free(dvs->gyros);

    for (int i = 0; i < ROTATION_BUFFER_NUM; i++)
        video_ion_free(&dvs->rotation_ion[i]);

    pthread_mutex_lock(&dvs->frame_list_mutex);
    while (dvs->frames.size()) {
        struct dvs_video_frame* frame = dvs->frames.front();
        dvs->frames.pop_front();
        frame->frame_buffer->decUsedCnt();
        delete frame;
    }
    pthread_mutex_unlock(&dvs->frame_list_mutex);

    pthread_mutex_destroy(&dvs->frame_list_mutex);

    free(video->dvs_data);
    video->dvs_data = NULL;

    return 0;
}

NV12_DVS::NV12_DVS(struct Video *video)
    : DppProcess(video, "NV12_DVS", kDigitalVideoStabilization, false)
{
    ALOGD("%s:%d enter", __func__, __LINE__);
    if (video_dvs_init(mVideo))
        printf("%s:%d dvs init failed!\n", __func__, __LINE__);
    ALOGD("%s:%d exit", __func__, __LINE__);
}

NV12_DVS::~NV12_DVS()
{
    ALOGD("%s:%d enter", __func__, __LINE__);
    video_dvs_deinit(mVideo);
    if (mDppCore) {
        delete mDppCore;
        mDppCore = nullptr;
    }
    ALOGD("%s:%d exit", __func__, __LINE__);
}

void NV12_DVS::stop()
{
    ALOGD("NV12_DVS:%s:%d enter", __func__, __LINE__);

    mDppCore->stop();

    StreamPUBase::stop();

    gyrosensor_stop();

    struct dvs_data* dvs = mVideo->dvs_data;
    if (dvs && dvs->context)
        dvs_request_stop(dvs->context);

    ALOGD("NV12_DVS:%s:%d exit", __func__, __LINE__);
}

int NV12_DVS::video_receive_gyrosensor_data(void)
{
    struct dvs_data* dvs = mVideo->dvs_data;

    if (!dvs->context)
        return 0;

    // Get sensor data from gyrosensor.
    struct sensor_data_set* data_set = &dvs->sensor_data;
    gyrosensor_direct_getdata(data_set);

    // Put sensor data to dvs algorithm.
    for (int i = 0; i < data_set->count; i++) {
        struct sensor_data* data = &data_set->data[i];

        if (!data->quat_flag) {
            dvs->gyros[i].timestamp_us = data->timestamp_us;

            dvs->gyros[i].gyro_axis.x = data->gyro_axis.x;
            dvs->gyros[i].gyro_axis.y = data->gyro_axis.y;
            dvs->gyros[i].gyro_axis.z = data->gyro_axis.z;
            // dvs->gyros[i].accel_axis.x = data->accel_axis.x;
            // dvs->gyros[i].accel_axis.y = data->accel_axis.y;
            // dvs->gyros[i].accel_axis.z = data->accel_axis.z;
        } else {
            printf("Error: Quat data is not supported for now.\n");
        }
    }

    dvs_put_gyro_frame(dvs->context, dvs->gyros, data_set->count);

    return 0;
}

bool NV12_DVS::doSomeThing(shared_ptr<BufferBase> outBuf, DppFrame* frame)
{
    Dpp_Buffer* dppBuf = reinterpret_cast<Dpp_Buffer*>(outBuf.get());
    dppBuf->createFromFrame(frame);
    dppBuf->incUsedCnt();

    return true;
}

int NV12_DVS::dppPacketProcess(shared_ptr<BufferBase>& inBuf)
{
    struct HAL_Buffer_MetaData* meta = inBuf->getMetaData();

    if (!meta || !mVideo->dvs_data)
        return -1;

    // Calculate timestamp
    uint64_t exp_end = (uint64_t)meta->timStamp.tv_sec * 1000000 +
                       (uint64_t)meta->timStamp.tv_usec;
    uint64_t exp_start = exp_end - (uint64_t)(meta->exp_time * 1000000);

    struct dvs_video_frame* dvs_frame = new dvs_video_frame();
    if (!dvs_frame)
        return -1;

    dvs_frame->video_info.frame_index = mVideo->dvs_data->frame_count;
    dvs_frame->video_info.time_stamp_st = exp_start;
    dvs_frame->video_info.time_stamp_end = exp_end;
    dvs_frame->video_info.bypass = (video_dvs_is_enable() ? 0 : 1);
    dvs_frame->frame_buffer = inBuf;

    pthread_mutex_lock(&mVideo->dvs_data->frame_list_mutex);
    mVideo->dvs_data->frames.push_back(dvs_frame);
    pthread_mutex_unlock(&mVideo->dvs_data->frame_list_mutex);
    if (mVideo->dvs_data->context)
        dvs_put_video_frame(mVideo->dvs_data->context,
                            &dvs_frame->video_info, 1);
    mVideo->dvs_data->frame_count++;
    mVideo->dvs_data->dpp = mDppCore;

    video_receive_gyrosensor_data();

    return 0;
}

#endif // #ifdef SDV
