/*
 * Copyright 2016 Rockchip Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "../video_interface.h"

static void dump_h264_stream(VEncStreamInfo* enc_stream_info)
{
    printf("frm_type: %d\n", enc_stream_info->frm_type);
    printf("buf addr: %p, size: %d\n",
           enc_stream_info->buf_addr,enc_stream_info->buf_size);
    printf("time_val: %ld[s],%ld[us]\n",
           enc_stream_info->time_val.tv_sec, enc_stream_info->time_val.tv_usec);
    printf("spspps buf addr: %p, size: %d\n",
           enc_stream_info->ExtraInfo.sps_pps_info.data,
           enc_stream_info->ExtraInfo.sps_pps_info.data_size);
}

static void dump_jpeg_stream(VEncStreamInfo* enc_stream_info)
{
    printf("frm_type: %d\n", enc_stream_info->frm_type);
    printf("buf addr: %p, size: %d\n",
           enc_stream_info->buf_addr,enc_stream_info->buf_size);
    printf("time_val: %ld[s],%ld[us]\n",
           enc_stream_info->time_val.tv_sec, enc_stream_info->time_val.tv_usec);
    printf("jpeg width : %d, height: %d\n",
           enc_stream_info->ExtraInfo.jpeg_info.width,
           enc_stream_info->ExtraInfo.jpeg_info.height);
}

static void channel0_stream_cb(VEncStreamInfo* enc_stream_info)
{
    printf("channel 0: \n");
    dump_h264_stream(enc_stream_info);
    printf("===========\n\n");
}

static void channel1_stream_cb(VEncStreamInfo* enc_stream_info)
{
    printf("channel 1: \n");
    dump_h264_stream(enc_stream_info);
    printf("===========\n\n");
}

static void test_write_jpg(int w, int h, void *buf, int buf_size)
{
    char file_path[128];
    int fd = -1;
    if (!buf)
        return;
    snprintf(file_path, sizeof(file_path), "/mnt/sdcard/test_%dx%d.jpg", w, h);
    fd = open(file_path, O_WRONLY|O_CREAT, 0666);
    if (fd > 0) {
        write(fd, buf, (size_t)buf_size);
        close(fd);
    }
}

static void channel2_stream_cb(VEncStreamInfo* enc_stream_info)
{
    printf("channel 2: \n");
    dump_jpeg_stream(enc_stream_info);
    printf("===========\n\n");
    test_write_jpg(enc_stream_info->ExtraInfo.jpeg_info.width,
                   enc_stream_info->ExtraInfo.jpeg_info.height,
                   enc_stream_info->buf_addr, enc_stream_info->buf_size);
}

static void channel3_stream_cb(VEncStreamInfo* enc_stream_info)
{
    printf("channel 3: \n");
    dump_jpeg_stream(enc_stream_info);
    printf("===========\n\n");
    test_write_jpg(enc_stream_info->ExtraInfo.jpeg_info.width,
                   enc_stream_info->ExtraInfo.jpeg_info.height,
                   enc_stream_info->buf_addr, enc_stream_info->buf_size);
}


int main(void)
{
    MediaHandle handle;
    handle.video_node = NULL;
    handle.channel_num = 4;
    SensorAttr sensor_attr;
    sensor_attr.width = 1920;
    sensor_attr.height = 1080;
    sensor_attr.fps = 30;
    sensor_attr.freq = 50;
    int ret = 0;
    int i = 0;
    void *receiver[4] = {NULL};
    NotifyNewEncStream st_callback[4] = {
        channel0_stream_cb,
        channel1_stream_cb,
        channel2_stream_cb,
        channel3_stream_cb
    };
    system("mount /dev/mmcblk0p1 /mnt/sdcard");
    if (media_sys_init(&handle, &sensor_attr) < 0)
        goto exit;

    venc_set_frame_rate(&handle, 0, sensor_attr.fps);
    venc_set_gop_size(&handle, 0, sensor_attr.fps/2);
    venc_set_bit_rate(&handle, 0, 14*1000*1000);
    venc_set_profile(&handle, 0, 66);

    venc_set_resolution(&handle, 1, 800, 480);
    venc_set_frame_rate(&handle, 1, sensor_attr.fps);
    venc_set_bit_rate(&handle, 1, 1*1000*1000);
    venc_set_profile(&handle, 1, 66);

    venc_set_resolution(&handle, 3, 800, 480);

    for (i = 0; i < sizeof(receiver)/sizeof (receiver[0]); ++i) {
        ret = venc_start_stream(&handle, i);
        if (ret < 0) {
            printf("start stream for channle <%d> fail\n", i);
            continue;
        }
        ret = create_enc_stream_receiver(&handle, i, &receiver[i], st_callback[i]);
        if (ret < 0) {
            printf("create receiver for channle <%d> fail\n", i);
        }
    }
    if (receiver[2])
        jpegenc_request_encode_one_frame(&handle, 2);
    if (receiver[3])
        jpegenc_request_encode_one_frame(&handle, 3);

    sleep(10);
    if (receiver[0])
        venc_require_an_idr(&handle, 0);
    if (receiver[1])
        venc_require_an_idr(&handle, 1);
    sleep(5);
    for (i = 0; i < sizeof(receiver)/sizeof (receiver[0]); ++i) {
        if (receiver[i]) {
            destroy_enc_stream_receiver(&handle, i, receiver[i]);
            receiver[i] = NULL;
            venc_stop_stream(&handle, i);
        }
    }
exit:
    media_sys_uninit(&handle);
    system("umount /mnt/sdcard");
    return ret;
}
