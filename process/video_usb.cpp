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

#include "video_usb.hpp"
#include "nv12_process.hpp"
#include "h264_process.hpp"
#include "mjpg_process.hpp"
#include "yuyv_process.hpp"
#include "video_hal.hpp"

extern HAL_COLORSPACE color_space;

int usb_video_init(struct Video* video,
                   int num,
                   unsigned int width,
                   unsigned int height,
                   unsigned int fps)
{
    frm_info_t in_frmFmt = {
        .frmSize = {width, height}, .frmFmt = USB_FMT_TYPE, .colorSpace = color_space, .fps = fps,
    };

    video->hal->dev =
        (shared_ptr<CamHwItf>)(new CamUSBDevHwItf());  // use usb test
    if (!video->hal->dev.get()) {
        printf("no memory!\n");
        return -1;
    }

    if (video->hal->dev->initHw(num) == false) {
        printf("video%d init fail!\n", num);
        return -1;
    }

    if (video_try_format(video, in_frmFmt)) {
        printf("video try format failed!\n");
        return -1;
    }

    if (video->usb_type == USB_TYPE_H264 && !is_record_mode)
        return -1;

    if (video->usb_type == USB_TYPE_H264 || video_record_h264_have_added())
        video_h264_save(video);

    if (video_init_setting(video))
        return -1;

    return 0;
}

static int usb_video_path_yuyv(struct Video* video)
{
    frm_info_t frmFmt = {
        .frmSize = {video->frmFmt.frmSize.width, video->frmFmt.frmSize.height},
        .frmFmt = HAL_FRMAE_FMT_NV12,
        .colorSpace = color_space,
        .fps = video->frmFmt.fps,
    };

    if (video->hal->mpath->prepare(video->frmFmt, 4,
                                   *(CameraBufferAllocator*)(NULL), false,
                                   0) == false) {
        printf("mp prepare faild!\n");
        return -1;
    }

    video->hal->yuyv_nv12 = shared_ptr<YUYV_NV12_Stream>(new YUYV_NV12_Stream(video));
    if (hal_add_pu(video->hal->mpath, video->hal->yuyv_nv12, frmFmt, 4, video->hal->bufAlloc))
        return -1;

    video->hal->nv12_disp = shared_ptr<NV12_Display>(new NV12_Display(video));
    if (hal_add_pu(video->hal->yuyv_nv12, video->hal->nv12_disp, frmFmt, 0, NULL))
        return -1;

    if (is_record_mode) {
        video->hal->nv12_enc = shared_ptr<NV12_Encode>(new NV12_Encode(video));
        if (hal_add_pu(video->hal->yuyv_nv12, video->hal->nv12_enc, frmFmt, 0, NULL))
            return -1;
    }

    video->hal->nv12_mjpg = shared_ptr<NV12_MJPG>(new NV12_MJPG(video));
    if (hal_add_pu(video->hal->yuyv_nv12, video->hal->nv12_mjpg, frmFmt, 0, NULL))
        return -1;

    video->hal->nv12_ts = std::make_shared<NV12_TS>(video);
    if (hal_add_pu(video->hal->yuyv_nv12, video->hal->nv12_ts, frmFmt, 0, NULL))
        return -1;

    return 0;
}

static int usb_video_path_mjpeg(struct Video* video)
{
    frm_info_t frmFmt = {
        .frmSize = {video->frmFmt.frmSize.width, video->frmFmt.frmSize.height},
        .frmFmt = HAL_FRMAE_FMT_YUV422P,
        .colorSpace = color_space,
        .fps = video->frmFmt.fps,
    };

    if (video->hal->mpath->prepare(video->frmFmt, 4,
                                   *(CameraBufferAllocator*)(NULL), false,
                                   0) == false) {
        printf("mp prepare faild!\n");
        return -1;
    }

    video->hal->mjpg_photo = shared_ptr<MJPG_Photo>(new MJPG_Photo(video));
    if (hal_add_pu(video->hal->mpath, video->hal->mjpg_photo, frmFmt, 0, NULL))
        return -1;

    video->hal->mjpg_nv12 = shared_ptr<MJPG_NV12>(new MJPG_NV12(video));
    if (hal_add_pu(video->hal->mpath, video->hal->mjpg_nv12, frmFmt, 3, video->hal->bufAlloc))
        return -1;

    video->hal->nv12_disp = shared_ptr<NV12_Display>(new NV12_Display(video));
    if (hal_add_pu(video->hal->mjpg_nv12, video->hal->nv12_disp, frmFmt, 0, NULL))
        return -1;

    if (is_record_mode) {
        video->hal->nv12_enc = shared_ptr<NV12_Encode>(new NV12_Encode(video));
        if (hal_add_pu(video->hal->mjpg_nv12, video->hal->nv12_enc, frmFmt, 0, NULL))
            return -1;
    }

    video->hal->nv12_ts = std::make_shared<NV12_TS>(video);
    if (hal_add_pu(video->hal->mjpg_nv12, video->hal->nv12_ts, frmFmt, 0, NULL))
        return -1;

    return 0;
}

static int usb_video_path_h264(struct Video* video)
{
    if (video->hal->mpath->prepare(video->frmFmt, 4,
                                   *(CameraBufferAllocator*)(NULL), false,
                                   0) == false) {
        printf("mp prepare faild!\n");
        return -1;
    }

    video->hal->h264_enc = shared_ptr<H264_Encode>(new H264_Encode(video));
    if (hal_add_pu(video->hal->mpath, video->hal->h264_enc, video->frmFmt, 0, NULL))
        return -1;

    // TODO: decode the h264 to nv12

    return 0;
}

int usb_video_path(struct Video* video)
{
    video->hal->mpath = video->hal->dev->getPath(CamHwItf::MP);
    if (video->hal->mpath.get() == NULL) {
        printf("mpath doesn't exist!\n");
        return -1;
    }

    if (video->usb_type != USB_TYPE_H264)
        video->hal->bufAlloc =
            shared_ptr<IonCameraBufferAllocator>(new IonCameraBufferAllocator());

    switch (video->usb_type) {
    case USB_TYPE_YUYV:
        printf("video%d path YUYV\n", video->deviceid);
        if (usb_video_path_yuyv(video))
            return -1;
        break;
    case USB_TYPE_MJPEG:
        printf("video%d path MJPEG\n", video->deviceid);
        if (usb_video_path_mjpeg(video))
            return -1;
        break;
    case USB_TYPE_H264:
        printf("video%d path H264\n", video->deviceid);
        if (usb_video_path_h264(video))
            return -1;
        break;
    default:
        printf("video%d path NULL\n", video->deviceid);
        return -1;
    }

    return 0;
}

int usb_video_start(struct Video* video)
{
    if (!video->hal->mpath->start()) {
        printf("mpath start failed!\n");
        return -1;
    }

    return 0;
}

static void usb_video_deinit_yuyv(struct Video* video)
{
    hal_remove_pu(video->hal->yuyv_nv12, video->hal->nv12_ts);
    video->hal->nv12_ts.reset();

    hal_remove_pu(video->hal->yuyv_nv12, video->hal->nv12_mjpg);

    hal_remove_pu(video->hal->yuyv_nv12, video->hal->nv12_enc);

    hal_remove_pu(video->hal->yuyv_nv12, video->hal->nv12_disp);

    hal_remove_pu(video->hal->mpath, video->hal->yuyv_nv12);
}

static void usb_video_deinit_mjpeg(struct Video* video)
{
    hal_remove_pu(video->hal->mjpg_nv12, video->hal->nv12_ts);
    video->hal->nv12_ts.reset();

    hal_remove_pu(video->hal->mjpg_nv12, video->hal->nv12_enc);

    hal_remove_pu(video->hal->mjpg_nv12, video->hal->nv12_disp);

    hal_remove_pu(video->hal->mpath, video->hal->mjpg_nv12);

    hal_remove_pu(video->hal->mpath, video->hal->mjpg_photo);
}

static void usb_video_deinit_h264(struct Video* video)
{
    hal_remove_pu(video->hal->mpath, video->hal->h264_enc);
}

void usb_video_deinit(struct Video* video)
{
    switch (video->usb_type) {
    case USB_TYPE_YUYV:
        usb_video_deinit_yuyv(video);
        break;
    case USB_TYPE_MJPEG:
        usb_video_deinit_mjpeg(video);
        break;
    case USB_TYPE_H264:
        usb_video_deinit_h264(video);
        break;
    }

    if (video->hal->mpath.get()) {
        video->hal->mpath->stop();
        video->hal->mpath->releaseBuffers();
    }

    if (video->hal->dev.get()) {
        video->hal->dev->deInitHw();
    }
}
