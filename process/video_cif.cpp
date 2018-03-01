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

#include "video_cif.hpp"
#include "nv12_process.hpp"
#include "video_hal.hpp"

extern bool cif_mirror;
extern struct rk_cams_dev_info g_test_cam_infos;
extern HAL_COLORSPACE color_space;

int cif_video_init(struct Video* video,
                   int num,
                   unsigned int width,
                   unsigned int height,
                   unsigned int fps)
{
    int i;
    int cif_num = -1;
    short inputid = parameter_get_cif_inputid();
    bool exist = false;
    frm_info_t in_frmFmt = {
        .frmSize = {width, height}, .frmFmt = HAL_FRMAE_FMT_NV12, .colorSpace = color_space, .fps = fps,
    };
    struct video_param cif_reso;

    // translate to cif num
    for (i = 0; i < g_test_cam_infos.num_camers; i++) {
        if (g_test_cam_infos.cam[i]->type == RK_CAM_ATTACHED_TO_CIF &&
            (((struct rk_cif_dev_info*)(g_test_cam_infos.cam[i]->dev))
             ->video_node.video_index == num)) {
            cif_num =
                ((struct rk_cif_dev_info*)(g_test_cam_infos.cam[i]->dev))->cif_index;
        }
    }
    if (cif_num == -1) {
        printf("can't find cif which video num is %d\n", num);
        return -1;
    }

    video->hal->dev = shared_ptr<CamHwItf>(
                          new CamCifDevHwItf(&(g_test_cam_infos.cif_devs.cif_devs[cif_num])));
    if (!video->hal->dev.get()) {
        printf("no memory!\n");
        return -1;
    }

    for (i = 0; i < g_test_cam_infos.num_camers; i++) {
        if (g_test_cam_infos.cam[i]->type == RK_CAM_ATTACHED_TO_CIF &&
            (((struct rk_cif_dev_info*)(g_test_cam_infos.cam[i]->dev))
             ->video_node.video_index == num) &&
            g_test_cam_infos.cam[i]->index == inputid) {
            printf("connected cif camera name %s,input id %d\n",
                   g_test_cam_infos.cam[i]->name, g_test_cam_infos.cam[i]->index);
            if (strstr(g_test_cam_infos.cam[i]->name, "cvbs"))
                video->cif_type = CIF_TYPE_CVBS;
            else if (strstr(g_test_cam_infos.cam[i]->name, "nvp"))
                video->cif_type = CIF_TYPE_MIX;
            else
                video->cif_type = CIF_TYPE_SENSOR;
            if (video->hal->dev->initHw(g_test_cam_infos.cam[i]->index) == false) {
                printf("video%d init fail!\n", num);
                return -1;
            }

            exist = true;
            break;
        }
    }

    if (!exist) {
        printf("cif inputid %d no exist\n", inputid);
        return -1;
    }

    if (video_try_format(video, in_frmFmt)) {
        printf("video try format failed!\n");
        return -1;
    }

    if (video->cif_type == CIF_TYPE_MIX && video->width == 720 &&
        (video->height == 576 || video->height == 480))
        video->cif_type = CIF_TYPE_CVBS;

    cif_reso.width = video->width;
    cif_reso.height = video->height;
    cif_reso.fps = in_frmFmt.fps;
    parameter_save_video_cifcamera_reso(&cif_reso);
    storage_setting_event_callback(0, NULL, NULL);

    if (video_init_setting(video))
        return -1;

    return 0;
}

int cif_video_path(struct Video* video)
{
    video->hal->bufAlloc =
        shared_ptr<IonCameraBufferAllocator>(new IonCameraBufferAllocator());
    if (!video->hal->bufAlloc.get()) {
        printf("new IonCameraBufferAllocator failed!\n");
        return -1;
    }

    video->hal->mpath = video->hal->dev->getPath(CamHwItf::MP);
    if (video->hal->mpath.get() == NULL) {
        printf("%s:path doesn't exist!\n", __func__);
        return -1;
    }

    if (video->hal->mpath->prepare(
            video->frmFmt, 4, *(video->hal->bufAlloc.get()), false, 0) == false) {
        printf("mp prepare faild!\n");
        return -1;
    }
    printf("cif: width = %4d,height = %4d\n", video->frmFmt.frmSize.width,
           video->frmFmt.frmSize.height);

    if (cif_mirror) {
        video->hal->nv12_mirr = shared_ptr<NV12_MIRROR>(new NV12_MIRROR(video));
        if (hal_add_pu(video->hal->mpath, video->hal->nv12_mirr, video->frmFmt, 4, video->hal->bufAlloc))
            return -1;
    }

    if (video->cif_type == CIF_TYPE_CVBS) {
        video->hal->nv12_iep = shared_ptr<NV12_IEP>(new NV12_IEP(video));
        if (cif_mirror) {
            if (hal_add_pu(video->hal->nv12_mirr, video->hal->nv12_iep, video->frmFmt, 4, video->hal->bufAlloc))
                return -1;
        } else {
            if (hal_add_pu(video->hal->mpath, video->hal->nv12_iep, video->frmFmt, 4, video->hal->bufAlloc))
                return -1;
        }
    }

    video->hal->nv12_disp = shared_ptr<NV12_Display>(new NV12_Display(video));
    if (video->cif_type == CIF_TYPE_CVBS) {
 	   if (hal_add_pu(video->hal->nv12_iep, video->hal->nv12_disp, video->frmFmt, 0, NULL))
 		   return -1;
    } else {
 	   if (cif_mirror) {
 		   if (hal_add_pu(video->hal->nv12_mirr, video->hal->nv12_disp, video->frmFmt, 0, NULL))
 			   return -1;
 	   } else {
 		   if (hal_add_pu(video->hal->mpath, video->hal->nv12_disp, video->frmFmt, 0, NULL))
 			   return -1;
 	   }
    }

    if (is_record_mode) {
        video->hal->nv12_enc = shared_ptr<NV12_Encode>(new NV12_Encode(video));
        if (video->cif_type == CIF_TYPE_CVBS) {
            if (hal_add_pu(video->hal->nv12_iep, video->hal->nv12_enc, video->frmFmt, 0, NULL))
                return -1;
        } else {
            if (cif_mirror) {
                if (hal_add_pu(video->hal->nv12_mirr, video->hal->nv12_enc, video->frmFmt, 0, NULL))
                    return -1;
            } else {
                if (hal_add_pu(video->hal->mpath, video->hal->nv12_enc, video->frmFmt, 0, NULL))
                    return -1;
            }
        }
    }

    video->hal->nv12_ts = std::make_shared<NV12_TS>(video);
    shared_ptr<StreamPUBase> pre_stream = nullptr;
    shared_ptr<CamHwItf::PathBase> pre_path = nullptr;
    if (video->cif_type == CIF_TYPE_CVBS) {
        pre_stream = video->hal->nv12_iep;
    } else {
        if (cif_mirror)
            pre_stream = video->hal->nv12_mirr;
        else
            pre_path = video->hal->mpath;
    }
    if (pre_stream && hal_add_pu(pre_stream, video->hal->nv12_ts, video->frmFmt, 0, NULL))
        return -1;
    if (pre_path && hal_add_pu(pre_path, video->hal->nv12_ts, video->frmFmt, 0, NULL))
        return -1;

    video->hal->nv12_mjpg = shared_ptr<NV12_MJPG>(new NV12_MJPG(video));
    if (video->cif_type == CIF_TYPE_CVBS) {
        if (hal_add_pu(video->hal->nv12_iep, video->hal->nv12_mjpg, video->frmFmt, 0, NULL))
            return -1;
    } else {
        if (cif_mirror) {
            if (hal_add_pu(video->hal->nv12_mirr, video->hal->nv12_mjpg, video->frmFmt, 0, NULL))
                return -1;
        } else {
            if (hal_add_pu(video->hal->mpath, video->hal->nv12_mjpg, video->frmFmt, 0, NULL))
                return -1;
        }
    }

#if USE_USB_WEBCAM && UVC_FROM_CIF
    video->hal->nv12_uvc = shared_ptr<NV12_UVC>(new NV12_UVC(video));
    if (hal_add_pu(video->hal->mpath, video->hal->nv12_uvc, video->frmFmt, 0, NULL))
        return -1;
#endif

	video->hal->nv12_readface = shared_ptr<NV12_ReadFace>(new NV12_ReadFace(video));
	if (hal_add_pu(video->hal->mpath, video->hal->nv12_readface, video->frmFmt, 0, NULL))
		return -1;

	video->hal->nv12_face_capture = shared_ptr<FaceCaptureProcess>(new FaceCaptureProcess(video));
	if (hal_add_pu(video->hal->nv12_readface, video->hal->nv12_face_capture, video->frmFmt, 3, video->hal->bufAlloc))
		return -1;

    return 0;
}

int cif_video_start(struct Video* video)
{
    if (!video->hal->mpath->start()) {
        printf("mpath start failed!\n");
        return -1;
    }

    return 0;
}

void cif_video_deinit(struct Video* video)
{
	hal_remove_pu(video->hal->nv12_readface, video->hal->nv12_face_capture);

	hal_remove_pu(video->hal->mpath, video->hal->nv12_readface);

#if USE_USB_WEBCAM && UVC_FROM_CIF
    hal_remove_pu(video->hal->mpath, video->hal->nv12_uvc);
#endif

    if (video->cif_type == CIF_TYPE_CVBS)
        hal_remove_pu(video->hal->nv12_iep, video->hal->nv12_disp);
    else {
        if (cif_mirror)
            hal_remove_pu(video->hal->nv12_mirr, video->hal->nv12_disp);
        else
            hal_remove_pu(video->hal->mpath, video->hal->nv12_disp);
    }

    if (video->cif_type == CIF_TYPE_CVBS)
        hal_remove_pu(video->hal->nv12_iep, video->hal->nv12_enc);
    else {
        if (cif_mirror)
            hal_remove_pu(video->hal->nv12_mirr, video->hal->nv12_enc);
        else
            hal_remove_pu(video->hal->mpath, video->hal->nv12_enc);
    }

    if (video->cif_type == CIF_TYPE_CVBS)
        hal_remove_pu(video->hal->nv12_iep, video->hal->nv12_mjpg);
    else {
        if (cif_mirror)
            hal_remove_pu(video->hal->nv12_mirr, video->hal->nv12_mjpg);
        else
            hal_remove_pu(video->hal->mpath, video->hal->nv12_mjpg);
    }

    if (video->cif_type == CIF_TYPE_CVBS)
        hal_remove_pu(video->hal->nv12_iep, video->hal->nv12_ts);
    else {
        if (cif_mirror)
            hal_remove_pu(video->hal->nv12_mirr, video->hal->nv12_ts);
        else
            hal_remove_pu(video->hal->mpath, video->hal->nv12_ts);
    }
    video->hal->nv12_ts.reset();

    if (video->cif_type == CIF_TYPE_CVBS) {
        if (cif_mirror)
            hal_remove_pu(video->hal->nv12_mirr, video->hal->nv12_iep);
        else
            hal_remove_pu(video->hal->mpath, video->hal->nv12_iep);
    }

    if (cif_mirror)
        hal_remove_pu(video->hal->mpath, video->hal->nv12_mirr);

    if (video->hal->mpath.get()) {
        video->hal->mpath->stop();
        video->hal->mpath->releaseBuffers();
    }

    if (video->hal->dev.get()) {
        video->hal->dev->deInitHw();
    }
}
