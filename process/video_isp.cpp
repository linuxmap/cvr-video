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

#include "video_isp.hpp"
#include "video_hal.hpp"

extern bool with_adas;
extern bool with_mp;
extern bool with_sp;
extern bool is_record_mode;
extern struct rk_cams_dev_info g_test_cam_infos;

#if MAIN_APP_NEED_DOWNSCALE_STREAM
bool video_isp_get_enc_scale(struct Video* video)
{
    return video->enc_scale;
}

void video_isp_set_enc_scale(struct Video* video, bool scale)
{
    video->enc_scale = scale;
}
#endif

static int video_isp_add_mp_enc(struct Video* video)
{
    video->hal->nv12_mjpg = shared_ptr<NV12_MJPG>(new NV12_MJPG(video));
    if (hal_add_pu(video->hal->mpath, video->hal->nv12_mjpg,
                   video->frmFmt, 0, NULL))
        return -1;

    video->hal->nv12_enc = shared_ptr<NV12_Encode>(new NV12_Encode(video));
    if (hal_add_pu(video->hal->mpath, video->hal->nv12_enc,
                   video->frmFmt, 0, NULL))
        return -1;

    return 0;
}

static void video_isp_remove_mp_enc(struct Video* video)
{
    hal_remove_pu(video->hal->mpath, video->hal->nv12_mjpg);
    video->hal->nv12_mjpg.reset();

    hal_remove_pu(video->hal->mpath, video->hal->nv12_enc);
    video->hal->nv12_enc.reset();
}


static int video_isp_add_mp_rga(struct Video* video)
{
    frm_info_t frmFmt = {
        .frmSize = {(unsigned int)video->output_width, (unsigned int)video->output_height},
        .frmFmt = HAL_FRMAE_FMT_NV12,
        .colorSpace = color_space,
        .fps = video->frmFmt.fps,
    };

    video->hal->mp_rga = shared_ptr<MP_RGA>(new MP_RGA(video));
    if (hal_add_pu(video->hal->mpath, video->hal->mp_rga,
                   frmFmt, 5, video->hal->bufAlloc))
        return -1;

    video->hal->rga_mjpg = shared_ptr<NV12_MJPG>(new NV12_MJPG(video));
    if (hal_add_pu(video->hal->mp_rga, video->hal->rga_mjpg,
                   frmFmt, 0, NULL))
        return -1;

    video->hal->rga_enc = shared_ptr<NV12_Encode>(new NV12_Encode(video));
    if (hal_add_pu(video->hal->mp_rga, video->hal->rga_enc,
                   frmFmt, 0, NULL))
        return -1;

    return 0;
}

static void video_isp_remove_mp_rga(struct Video* video)
{
    hal_remove_pu(video->hal->mp_rga, video->hal->rga_enc);
    video->hal->rga_enc.reset();

    hal_remove_pu(video->hal->mp_rga, video->hal->rga_mjpg);
    video->hal->rga_mjpg.reset();

    hal_remove_pu(video->hal->mpath, video->hal->mp_rga);
    video->hal->mp_rga.reset();
}

static int video_isp_add_mp_3dnr(struct Video* video)
{
    shared_ptr<StreamPUBase> pre;

#ifdef SDV
    video->hal->mp_dvs = shared_ptr<NV12_DVS>(new NV12_DVS(video));
    if (hal_add_pu(video->hal->mpath, video->hal->mp_dvs,
                   video->frmFmt, DPP_OUT_BUFFER_NUM, NULL))
        return -1;

    video->hal->dsp_trans = shared_ptr<TRANSPARENT>(new TRANSPARENT(video));
    if (hal_add_pu(video->hal->mp_dvs, video->hal->dsp_trans,
                   video->frmFmt, 0, NULL))
        return -1;
#else
    video->hal->mp_3dnr = shared_ptr<NV12_3DNR>(new NV12_3DNR(video));
    if (hal_add_pu(video->hal->mpath, video->hal->mp_3dnr,
                   video->frmFmt, DPP_OUT_BUFFER_NUM, NULL))
        return -1;

    video->hal->dsp_trans = shared_ptr<TRANSPARENT>(new TRANSPARENT(video));
    if (hal_add_pu(video->hal->mp_3dnr, video->hal->dsp_trans,
                   video->frmFmt, 0, NULL))
        return -1;

#endif

    if (parameter_get_video_flip() && !USE_ISP_FLIP) {
        video->hal->nv12_flip = shared_ptr<NV12_FLIP>(new NV12_FLIP(video));
        if (hal_add_pu(video->hal->dsp_trans, video->hal->nv12_flip,
                       video->frmFmt, 2, video->hal->bufAlloc))
            return -1;
        pre = video->hal->nv12_flip;
    } else
        pre = video->hal->dsp_trans;

//	video->hal->dnr_readface = shared_ptr<NV12_ReadFace>(new NV12_ReadFace(video));
//	if (hal_add_pu(video->hal->dsp_trans, video->hal->dnr_readface,
//				   video->frmFmt, 0, NULL))
//		return -1;

    video->hal->dnr_disp = shared_ptr<NV12_Display>(new NV12_Display(video));
    if (hal_add_pu(pre, video->hal->dnr_disp,
                   video->frmFmt, 0, NULL))
        return -1;

    if (is_record_mode) {
        video->hal->dnr_enc = shared_ptr<NV12_Encode>(new NV12_Encode(video));
        if (hal_add_pu(pre, video->hal->dnr_enc,
                       video->frmFmt, 0, NULL))
            return -1;
#if MAIN_APP_NEED_DOWNSCALE_STREAM
        if (video_isp_get_enc_scale(video)) {
            video->hal->dnr_enc_s = shared_ptr<DPP_NV12_Encode_S>(new DPP_NV12_Encode_S(video));
            if (hal_add_pu(pre, video->hal->dnr_enc_s,
                           video->frmFmt, 0, NULL))
                return -1;
        }
#endif
    }

    video->hal->dnr_mjpg = shared_ptr<NV12_MJPG>(new NV12_MJPG(video));
    if (hal_add_pu(pre, video->hal->dnr_mjpg,
                   video->frmFmt, 0, NULL))
        return -1;

    if (!video->hal->nv12_ts) {
        video->hal->nv12_ts = std::make_shared<NV12_TS>(video);
        if (hal_add_pu(video->hal->dsp_trans, video->hal->nv12_ts,
                       video->frmFmt, 0, NULL))
            return -1;
    }

#if USE_USB_WEBCAM && UVC_FROM_DSP
    video->hal->dnr_uvc = shared_ptr<NV12_UVC>(new NV12_UVC(video));
    if (hal_add_pu(pre, video->hal->dnr_uvc,
                   video->frmFmt, 0, NULL))
        return -1;
#endif

#ifdef ENABLE_RS_FACE
    video->hal->nv12_face_detect = shared_ptr<FaceDetectProcess>(new FaceDetectProcess(video));
    if (hal_add_pu(video->hal->mpath, video->hal->nv12_face_detect,
                        video->frmFmt, 0, NULL))
        return -1;
#endif
//	video->hal->nv12_readface = shared_ptr<NV12_ReadFace>(new NV12_ReadFace(video));
//	if (hal_add_pu(video->hal->mpath, video->hal->nv12_readface, video->frmFmt, 0, NULL))
//		return -1;
//	
//	video->hal->nv12_face_capture = shared_ptr<FaceCaptureProcess>(new FaceCaptureProcess(video));
//	if (hal_add_pu(video->hal->nv12_readface, video->hal->nv12_face_capture, video->frmFmt, 3, video->hal->bufAlloc))
//		return -1;

    return 0;
}

static void video_isp_remove_mp_3dnr(struct Video* video)
{
    shared_ptr<StreamPUBase> pre;

//	hal_remove_pu(video->hal->nv12_readface, video->hal->nv12_face_capture);
//
//	hal_remove_pu(video->hal->mpath, video->hal->nv12_readface);

    if (parameter_get_video_flip() && !USE_ISP_FLIP)
        pre = video->hal->nv12_flip;
    else
        pre = video->hal->dsp_trans;

#if USE_USB_WEBCAM && UVC_FROM_DSP
    hal_remove_pu(pre, video->hal->dnr_uvc);
    video->hal->dnr_uvc.reset();
#endif

    if (video->hal->nv12_ts) {
        hal_remove_pu(video->hal->dsp_trans, video->hal->nv12_ts);
        video->hal->nv12_ts.reset();
    }

    hal_remove_pu(pre, video->hal->dnr_mjpg);
    video->hal->dnr_mjpg.reset();

    if (is_record_mode) {
        hal_remove_pu(pre, video->hal->dnr_enc);
        video->hal->dnr_enc.reset();

#if MAIN_APP_NEED_DOWNSCALE_STREAM
        if (video_isp_get_enc_scale(video)) {
            hal_remove_pu(pre, video->hal->dnr_enc_s);
            video->hal->dnr_enc_s.reset();
        }
#endif
    }

    hal_remove_pu(pre, video->hal->dnr_disp);
    video->hal->dnr_disp.reset();

//    hal_remove_pu(video->hal->dsp_trans, video->hal->dnr_readface);
//    video->hal->dnr_readface.reset();

    if (parameter_get_video_flip() && !USE_ISP_FLIP) {
        hal_remove_pu(video->hal->dsp_trans, video->hal->nv12_flip);
        video->hal->nv12_flip.reset();
    }

#ifdef SDV
    hal_remove_pu(video->hal->mp_dvs, video->hal->dsp_trans);
    video->hal->dsp_trans.reset();

    hal_remove_pu(video->hal->mpath, video->hal->mp_dvs);
    video->hal->mp_dvs.reset();
#else
    hal_remove_pu(video->hal->mp_3dnr, video->hal->dsp_trans);
    video->hal->dsp_trans.reset();

    hal_remove_pu(video->hal->mpath, video->hal->mp_3dnr);
    video->hal->mp_3dnr.reset();
#endif

#ifdef ENABLE_RS_FACE
    hal_remove_pu(video->hal->mpath, video->hal->nv12_face_detect);
    video->hal->nv12_face_detect.reset();
#endif
}

static int video_isp_add_sp_display(struct Video* video)
{
    video->hal->sp_disp = shared_ptr<NV12_Display>(new NV12_Display(video));
    if (hal_add_pu(video->hal->spath, video->hal->sp_disp,
                   video->spfrmFmt, 0, NULL))
        return -1;
	
	video->hal->sp_readface = shared_ptr<NV12_ReadFace>(new NV12_ReadFace(video));
	if (hal_add_pu(video->hal->spath, video->hal->sp_readface,
				   video->frmFmt, 0, NULL))
		return -1;

    return 0;
}

static void video_isp_remove_sp_display(struct Video* video)
{
	hal_remove_pu(video->hal->spath, video->hal->sp_readface);
	video->hal->sp_readface.reset();

    hal_remove_pu(video->hal->spath, video->hal->sp_disp);
    video->hal->sp_disp.reset();
}

static int video_isp_add_sp_enc_small(struct Video* video)
{
#if MAIN_APP_NEED_DOWNSCALE_STREAM
    if (video_isp_get_enc_scale(video)) {
        frm_info_t frmFmt = {
            .frmSize = {DOWN_SCALE_WIDTH, DOWN_SCALE_HEIGHT},
            .frmFmt = HAL_FRMAE_FMT_NV12,
            .colorSpace = color_space,
            .fps = video->frmFmt.fps,
        };

        video->hal->sp_rga = shared_ptr<MP_RGA>(new MP_RGA(video));
        if (hal_add_pu(video->hal->spath, video->hal->sp_rga, frmFmt, 3,
                       video->hal->bufAlloc))
            return -1;
        video->hal->rga_enc_s = shared_ptr<NV12_Encode_S>(new NV12_Encode_S(video));
        if (hal_add_pu(video->hal->sp_rga, video->hal->rga_enc_s, video->frmFmt, 0,
                       NULL))
            return -1;
    }
#endif

    return 0;
}

static void video_isp_remove_sp_enc_small(struct Video* video)
{
#if MAIN_APP_NEED_DOWNSCALE_STREAM
    if (video_isp_get_enc_scale(video)) {
        hal_remove_pu(video->hal->sp_rga, video->hal->rga_enc_s);
        video->hal->rga_enc_s.reset();
        hal_remove_pu(video->hal->spath, video->hal->sp_rga);
        video->hal->sp_rga.reset();
    }
#endif
}

static int video_isp_add_mp_display(struct Video* video)
{
    video->hal->mp_disp = shared_ptr<NV12_Display>(new NV12_Display(video));
    if (hal_add_pu(video->hal->mpath, video->hal->mp_disp,
                   video->frmFmt, 0, NULL))
        return -1;

    return 0;
}

static void video_isp_remove_mp_display(struct Video* video)
{
    hal_remove_pu(video->hal->mpath, video->hal->mp_disp);
    video->hal->mp_disp.reset();
}

static int video_isp_add_sp_adas(struct Video* video)
{
    if (with_adas) {
        video->hal->sp_adas = shared_ptr<NV12_ADAS>(new NV12_ADAS(video));
        if (hal_add_pu(video->hal->spath, video->hal->sp_adas,
                       video->spfrmFmt, 0, NULL))
            return -1;
    }

    return 0;
}

static void video_isp_remove_sp_adas(struct Video* video)
{
    if (with_adas) {
        hal_remove_pu(video->hal->spath, video->hal->sp_adas);
        video->hal->sp_adas.reset();
    }
}

static int video_isp_policy(struct Video* video, int policy)
{
    int ret = 0;
    int screen_w, screen_h;
    int out_device = rk_fb_get_out_device(&screen_w, &screen_h);

    if (out_device == OUT_DEVICE_HDMI
        || out_device == OUT_DEVICE_CVBS_PAL
        || out_device == OUT_DEVICE_CVBS_NTSC) {
        video->out_exist = true;
    } else {
        video->out_exist = false;
    }

    if (video->out_exist != video->out_state) {
        video->out_state = video->out_exist;
        memset(video->path, 0, sizeof(video->path));
    }

    if (!video->path[policy]) {
        memset(video->path, 0, sizeof(video->path));
        video_isp_remove_policy(video);
        //video->hal->spath->removeBufferNotifer(video->hal->sp_dsp.get());

        video->path[policy] = true;
        switch (policy) {
        case 0:
            ret = video_isp_add_mp_rga(video);
            if (!ret) {
                if (video->out_exist)
                    ret = video_isp_add_mp_display(video);
                else
                    ret = video_isp_add_sp_display(video);
            }
            video_isp_add_sp_enc_small(video);
            break;
        case 1:
            ret = video_isp_add_mp_enc(video);
            if (!ret) {
                if (video->out_exist)
                    ret = video_isp_add_mp_display(video);
                else
                    ret = video_isp_add_sp_display(video);
            }
            video_isp_add_sp_enc_small(video);
            break;
        case 2:
            if (video->out_exist)
                ret = video_isp_add_mp_display(video);
            else
                ret = video_isp_add_sp_display(video);
            break;
        case 3:
            ret = video_isp_add_mp_3dnr(video);
            if (!ret)
                ret = video_isp_add_sp_adas(video);
            break;
        case 4:
            ret = video_isp_add_sp_adas(video);
            //video->hal->spath->addBufferNotifier(video->hal->sp_dsp.get());
            break;
        default:
            printf("%s: not support policy: %d\n", __func__, policy);
            break;
        }
    }

    return ret;
}

int video_isp_add_policy(struct Video* video)
{
    if (video->type != VIDEO_TYPE_ISP)
        return 0;

    if (video->high_temp) {
        if (1/*is_record_mode || video->photo.state != PHOTO_END*/) {
            if (video->width != video->output_width
                || video->height != video->output_height) {
                /*add MP--RGA--ENC SP--DISP path[0]*/
                return video_isp_policy(video, 0);
            } else {
                /*add MP--ENC SP--DISP path[1]*/
                return video_isp_policy(video, 1);
            }
        } else {
            /*add SP--DISP path[2]*/
            return video_isp_policy(video, 2);
        }
    } else {
        if (1/*is_record_mode || video->photo.state != PHOTO_END*/) {
            /*add MP--DSP--ENC,DISP path[3]*/
            return video_isp_policy(video, 3);
        } else {
            /*add SP--DSP--DISP path[4]*/
            return video_isp_policy(video, 4);
        }
    }
}

void video_isp_remove_policy(struct Video* video)
{
    if (video->hal->mpath.get()) {
        video_isp_remove_mp_rga(video);
        video_isp_remove_mp_enc(video);
        video_isp_remove_mp_3dnr(video);
        video_isp_remove_mp_display(video);
    }
    if (video->hal->spath.get()) {
        video_isp_remove_sp_display(video);
        video_isp_remove_sp_adas(video);
        video_isp_remove_sp_enc_small(video);
    }
}

int video_isp_add_mp_policy_fix(struct Video* video)
{
#ifdef NV12_RAW_DATA
    for (i = 0; i < NV12_RAW_CNT; i++) {
        memset (&video->raw[i], 0, sizeof(struct video_ion));
        video->raw[i].client = -1;
        video->raw[i].fd = -1;
        if (video_ion_alloc(&video->raw[i], video->width, video->height)) {
            printf("isp_video_path_mp ion alloc fail!\n");
            return -1;
        }
    }

    video->raw_fd = rk_rga_open();
    if (!video->raw_fd)
        return -1;

    video->hal->nv12_raw = shared_ptr<NV12_RAW>(new NV12_RAW(video));
    if (hal_add_pu(video->hal->mpath, video->hal->nv12_raw,
                   video->frmFmt, 0, NULL))
        return -1;
#endif

#if USE_USB_WEBCAM && UVC_FROM_ISP
    video->hal->nv12_uvc = shared_ptr<NV12_UVC>(new NV12_UVC(video));
    if (hal_add_pu(video->hal->mpath, video->hal->nv12_uvc,
                   video->frmFmt, 0, NULL))
        return -1;
#endif

    return 0;
}

void video_isp_remove_mp_policy_fix(struct Video* video)
{
#if USE_USB_WEBCAM && UVC_FROM_ISP
    hal_remove_pu(video->hal->mpath, video->hal->nv12_uvc);
    video->hal->nv12_uvc.reset();
#endif

#ifdef NV12_RAW_DATA
    hal_remove_pu(video->hal->mpath, video->hal->nv12_raw);
    video->hal->nv12_raw.reset();

    for (i = 0; i < NV12_RAW_CNT; i++)
        video_ion_free(&video->raw[i]);

    if (video->raw_fd)
        rk_rga_close(video->raw_fd);
#endif
}

int video_isp_add_sp_policy_fix(struct Video* video)
{
    if (!with_mp) {
        video->width = video->spfrmFmt.frmSize.width;
        video->height = video->spfrmFmt.frmSize.height;
        if (is_record_mode) {
            video->hal->nv12_enc = shared_ptr<NV12_Encode>(new NV12_Encode(video));
            if (hal_add_pu(video->hal->spath, video->hal->nv12_enc,
                           video->spfrmFmt, 0, NULL))
                return -1;
        }

        video->hal->nv12_ts = std::make_shared<NV12_TS>(video);
        if (hal_add_pu(video->hal->spath, video->hal->nv12_ts,
                       video->spfrmFmt, 0, NULL))
            return -1;

        video->hal->nv12_mjpg = shared_ptr<NV12_MJPG>(new NV12_MJPG(video));
        if (hal_add_pu(video->hal->spath, video->hal->nv12_mjpg,
                       video->spfrmFmt, 0, NULL))
            return -1;
    }

    return 0;
}

void video_isp_remove_sp_policy_fix(struct Video* video)
{
    if (!with_mp) {
        hal_remove_pu(video->hal->spath, video->hal->nv12_ts);
        video->hal->nv12_ts.reset();

        hal_remove_pu(video->hal->spath, video->hal->nv12_enc);
        video->hal->nv12_enc.reset();

        hal_remove_pu(video->hal->spath, video->hal->nv12_mjpg);
        video->hal->nv12_mjpg.reset();
    }
}

int isp_video_init(struct Video* video,
                   int num,
                   unsigned int width,
                   unsigned int height,
                   unsigned int fps)
{
    int i = 0;
    bool exist = false;
    unsigned int isp_max_width = 0, isp_max_height = 0;
    parameter_get_isp_max_resolution(&isp_max_width, &isp_max_height);
    frm_info_t in_frmFmt = {
        .frmSize = {
            (width > isp_max_width) ? isp_max_width : width,
            (height > isp_max_height) ? isp_max_height : height
        },
        .frmFmt = HAL_FRMAE_FMT_NV12, .colorSpace = color_space, .fps = fps,
    };

	printf("czy: isp video init \n");
    if (with_sp) {
        if (with_mp) {
            frm_info_t spfrmFmt = {
                .frmSize = {ISP_SP_WIDTH, ISP_SP_HEIGHT},
                .frmFmt = HAL_FRMAE_FMT_NV12,
                .colorSpace = color_space,
                .fps = fps,
            };
            memcpy(&video->spfrmFmt, &spfrmFmt, sizeof(frm_info_t));
        } else {
            memcpy(&video->spfrmFmt, &in_frmFmt, sizeof(frm_info_t));
        }
    }

    video->hal->dev = getCamHwItf(&(g_test_cam_infos.isp_dev));
    //(shared_ptr<CamHwItf>)(new CamIsp11DevHwItf());
    if (!video->hal->dev.get()) {
        printf("no memory!\n");
        return -1;
    }

    for (i = 0; i < g_test_cam_infos.num_camers; i++) {
        if (g_test_cam_infos.cam[i]->type == RK_CAM_ATTACHED_TO_ISP) {
            printf("connected isp camera name %s,input id %d\n",
                   g_test_cam_infos.cam[i]->name, g_test_cam_infos.cam[i]->index);

            if (video->hal->dev->initHw(g_test_cam_infos.cam[i]->index) == false) {
                printf("video%d init fail!\n", num);
                return -1;
            }

            exist = true;
            break;
        }
    }

    if (!exist)
        return -1;

#if 0
    if (video_try_format(video, in_frmFmt)) {
        printf("video try format failed!\n");
        return -1;
    }
#else
    memcpy(&video->frmFmt, &in_frmFmt, sizeof(frm_info_t));
    video->type = VIDEO_TYPE_ISP;
    video->width = video->frmFmt.frmSize.width;
    video->height = video->frmFmt.frmSize.height;
#endif

    if (video_init_setting(video))
        return -1;

    printf("USE_ISP_FLIP: %d\n", USE_ISP_FLIP);
    if (parameter_get_video_flip() && USE_ISP_FLIP)
        video->hal->dev->setFlip(HAL_FLIP_H | HAL_FLIP_V);
    else
        video->hal->dev->setFlip(0);

    return 0;
}

int isp_video_path_sp(struct Video* video)
{
    video->hal->spath = video->hal->dev->getPath(CamHwItf::SP);
    if (video->hal->spath.get() == NULL) {
        printf("%s:path doesn't exist!\n", __func__);
        return -1;
    }

    if (with_mp) {
        if (video->hal->spath->prepare(video->spfrmFmt, 4,
                                       *(video->hal->bufAlloc.get()), false,
                                       0) == false) {
            printf("sp prepare faild!\n");
            return -1;
        }
    } else {
        if (video->hal->spath->prepare(video->spfrmFmt, 4,
                                       *(video->hal->bufAlloc.get()), false,
                                       0) == false) {
            printf("sp prepare faild!\n");
            return -1;
        }
    }
    printf("sp: width = %4d,height = %4d\n", video->spfrmFmt.frmSize.width,
           video->spfrmFmt.frmSize.height);

    video_set_fps(video, 1, video->spfrmFmt.fps);

    if (video_isp_add_sp_policy_fix(video))
        return -1;

    return 0;
}

int isp_video_path_mp(struct Video* video)
{
	printf("czy: isp video path mp\n");
    video->hal->mpath = video->hal->dev->getPath(CamHwItf::MP);
    if (video->hal->mpath.get() == NULL) {
        printf("%s:path doesn't exist!\n", __func__);
        return -1;
    }

    if (video->hal->mpath->prepare(
            video->frmFmt, 5, *(video->hal->bufAlloc.get()), false, 0) == false) {
        printf("mp prepare faild!\n");
        return -1;
    }
    printf("mp: width = %4d,height = %4d\n", video->frmFmt.frmSize.width,
           video->frmFmt.frmSize.height);

    video_set_fps(video, 1, video->frmFmt.fps);

    if (video_isp_add_mp_policy_fix(video))
        return -1;

    return 0;
}

int isp_video_path(struct Video* video)
{
    video->hal->bufAlloc =
        shared_ptr<IonCameraBufferAllocator>(new IonCameraBufferAllocator());
    if (!video->hal->bufAlloc.get()) {
        printf("new IonCameraBufferAllocator failed!\n");
        return -1;
    }

    if (with_mp) {
        if (isp_video_path_mp(video))
            return -1;
    }

    if (with_sp) {
        if (isp_video_path_sp(video))
            return -1;
    }

    return 0;
}

int isp_video_start(struct Video* video)
{
    if (with_mp) {
        if (!video->hal->mpath->start()) {
            printf("mpath start failed!\n");
            return -1;
        }
    }

    if (with_sp) {
        if (!video->hal->spath->start()) {
            printf("spath start failed!\n");
            return -1;
        }
    }

    return 0;
}

void isp_video_deinit(struct Video* video)
{
    video_isp_remove_policy(video);

    if (video->hal->mpath.get()) {
        video_isp_remove_mp_policy_fix(video);
        video->hal->mpath->stop();
        video->hal->mpath->releaseBuffers();
    }

    if (video->hal->spath.get()) {
        video_isp_remove_sp_policy_fix(video);
        video->hal->spath->stop();
        video->hal->spath->releaseBuffers();
    }

    if (video->hal->dev.get()) {
        video->hal->dev->deInitHw();
    }
}
