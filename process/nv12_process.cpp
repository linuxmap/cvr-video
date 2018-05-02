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

#include "nv12_process.hpp"
extern unsigned int user_noise;

#ifdef _PCBA_SERVICE_
VIDEO_PCBA_CB_TYPE m_pfunc;
int camera_test_type = -1;

extern "C" void video_register_pcba_callback(int type, VIDEO_PCBA_CB_TYPE pfunc)
{
    camera_test_type = type;
    m_pfunc = pfunc;
}

extern "C" void video_cancellation_pcba_callback(void)
{
    camera_test_type = -1;
    m_pfunc = NULL;
}

extern "C" void video_pcba_yuv_to_rgba(int rga_fd,  int src_fd, int src_w, int src_h,
                                       int src_fmt, int src_vir_w, int src_vir_h,
                                       int dst_fd,  int dst_w, int dst_h,
                                       int dst_fmt, int rotate, int rotate_mode)
{
    rk_rga_ionfd_to_ionfd(rga_fd, src_fd, src_w, src_h,
                          src_fmt, src_vir_w, src_vir_h,
                          dst_fd,  dst_w, dst_h,
                          dst_fmt, rotate, rotate_mode);
}

extern "C" void video_do_pcba_call_back(void * buff, int width, int height, size_t size)
{
    m_pfunc(buff, width, height, size);
}

#endif

static void video_set_photo_max_num(struct Video* video, unsigned int num)
{
    video->photo.max_num = num;
}

static unsigned int video_get_photo_max_num(struct Video* video)
{
    return video->photo.max_num;
}

void video_set_photo_user_num(struct Video* video, unsigned int num)
{
    unsigned int min = 1;
    video->photo.user_num = std::min(std::max(num, min),
                                     video_get_photo_max_num(video));
}

unsigned int video_get_photo_user_num(struct Video* video)
{
    return video->photo.user_num;
}

static bool video_photo_is_continuous(struct Video* video)
{
    return (video->type == VIDEO_TYPE_ISP && !is_record_mode
            && video_get_photo_user_num(video) > 1);
}

static bool video_photo_is_continuous_inited(struct Video* video)
{
    return (video->type == VIDEO_TYPE_ISP && !is_record_mode);
}

static int nv12_encode_mjpg_save(struct Video* video, int num)
{
    int ret = 0;
    int fd = -1;
    char filename[128];
    char filetype[8] = {0};
    struct vpu_encode encode = video->photo.encode[num];

    if (!encode.enc_out_data)
        return -1;

    if (video_photo_is_continuous(video))
        snprintf(filetype, sizeof(filetype), "%d.jpg", num);
    else
        snprintf(filetype, sizeof(filetype), "jpg");
    video_record_getfilename(filename, sizeof(filename),
                             fs_storage_folder_get_bytype(video->type, PICFILE_TYPE),
                             video->deviceid, filetype, "");
    fd = fs_picture_open((char*)filename, O_WRONLY | O_CREAT, 0666);
    if (fd < 0) {
        printf("Cannot open jpg file\n");
        return -1;
    }

    ret = fs_picture_write(fd, encode.enc_out_data, encode.enc_out_length);
    if (ret <= 0)
        printf("fs_picture_write fail, ret=%d, errno=%d\n", ret, errno);
    else
        printf("%s:%s\n", __func__, filename);

    fs_picture_close(fd);

#if MJPG_SAVE_THUMBNAIL
    int fd_ext = -1;
    char filename_ext[128];
    struct vpu_encode encode_ext = video->photo.encode_ext[num];

    if (!encode_ext.enc_out_data)
        return -1;

    fs_storage_thumbname_get(filename, filename_ext);
    fd_ext = fs_picture_open((char*)filename_ext, O_WRONLY | O_CREAT, 0666);
    if (fd_ext < 0) {
        printf("Cannot open jpg file\n");
        return -1;
    }

    ret = fs_picture_write(fd_ext, encode_ext.enc_out_data, encode_ext.enc_out_length);
    if (ret <= 0)
        printf("fs_picture_write fail, ret=%d, errno=%d\n", ret, errno);
    else
        printf("%s:%s\n", __func__, filename_ext);

    fs_picture_close(fd_ext);
#endif

    return ret;
}

#ifdef USE_WATERMARK
static void video_photo_watermark(struct Video* video, uint8_t* buffer,
                                  int width, int height)
{
    if (!parameter_get_video_mark())
        return;
    watermark_draw_on_nv12(&video->photo.watermark, buffer, width, height,
                           parameter_get_licence_plate_flag());
}
#endif

int video_rga_photo_process(struct Video* video,
                            int s_fd, int s_width, int s_height, void* s_buf)
{
    int ret = 0;
    void* buffer = NULL;
    int size = 0;
    int fd = -1;
    int width = video->photo.encode[0].width;
    int height = video->photo.encode[0].height;

    int src_w, src_h, src_fd, dst_w, dst_h, dst_fd;

#if MJPG_SAVE_THUMBNAIL
    src_w = s_width;
    src_h = s_height;
    src_fd = s_fd;
    dst_w = video->photo.rga_photo_ext.width;
    dst_h = video->photo.rga_photo_ext.height;
    dst_fd = video->photo.rga_photo_ext.fd;
    ret = rk_rga_ionfd_to_ionfd_scal(video->photo.rga_fd,
                                     src_fd, src_w, src_h, RGA_FORMAT_YCBCR_420_SP,
                                     dst_fd, dst_w, dst_h, RGA_FORMAT_YCBCR_420_SP,
                                     0, 0, dst_w, dst_h, src_w, src_h);

    if (ret) {
        printf("%s rga fail!\n", __func__);
        goto exit;
    }
#endif

    if (width != s_width || height != s_height) {
        /* use for 4K encode */

        if (width != video->photo.rga_photo.width
            || height != video->photo.rga_photo.height) {
            video_ion_free(&video->photo.rga_photo);
            if (video_ion_alloc(&video->photo.rga_photo, width, height))
                goto exit;
        }
        src_w = s_width;
        src_h = s_height;
        src_fd = s_fd;
        dst_w = width;
        dst_h = height;
        dst_fd = video->photo.rga_photo.fd;
        ret = rk_rga_ionfd_to_ionfd_scal(video->photo.rga_fd,
                                         src_fd, src_w, src_h,
                                         RGA_FORMAT_YCBCR_420_SP,
                                         dst_fd, dst_w, dst_h,
                                         RGA_FORMAT_YCBCR_420_SP,
                                         0, 0, dst_w, dst_h, src_w, src_h);
        if (ret) {
            printf("%s rga failed.\n", __func__);
            goto exit;
        }
        buffer = video->photo.rga_photo.buffer;
        size = width * height * 3 / 2;
        fd = video->photo.rga_photo.fd;
    } else {
        /* use for source encode */
        buffer = s_buf;
        size = s_width * s_height * 3 / 2;
        fd = s_fd;
    }

#ifdef USE_WATERMARK
    video_photo_watermark(video, (uint8_t*)buffer, width, height);
#endif

    pthread_mutex_lock(&video->photo.mutex);
    if (!video_photo_is_continuous(video))
        video->photo.num = 0;

#if MJPG_SAVE_THUMBNAIL
    void* buffer_ext;
    int size_ext;
    int fd_ext;

    buffer_ext = video->photo.rga_photo_ext.buffer;
    size_ext = video->photo.rga_photo_ext.width * video->photo.rga_photo_ext.height * 3 / 2;
    fd_ext = video->photo.rga_photo_ext.fd;
    vpu_nv12_encode_jpeg_doing(&video->photo.encode_ext[video->photo.num],
                               buffer_ext, fd_ext, size_ext);
#endif
    ret = vpu_nv12_encode_jpeg_doing(&video->photo.encode[video->photo.num],
                                     buffer, fd, size);
    if (ret) {
        pthread_mutex_unlock(&video->photo.mutex);
        goto exit;
    }

    if (video_photo_is_continuous(video)) {
        video->photo.num++;
        if (video->photo.num == video_get_photo_user_num(video)) {
            pthread_cond_signal(&video->photo.condition);
            video->photo.num = 0;
        }
    } else {
        pthread_cond_signal(&video->photo.condition);
    }
    pthread_mutex_unlock(&video->photo.mutex);

    return 0;

exit:
    video_record_takephoto_end(video);

    return ret;
}

int show_video_mark(int width,
                    int height,
                    void* dstbuf,
                    int fps,
                    void* meta1,
                    uint32_t* noise,
                    struct dpp_sharpness* sharpness)
{
    int x_pos = 50, y_pos = 100, offset = 20;
    char name[256] = {0};
    struct HAL_Buffer_MetaData* meta = (struct HAL_Buffer_MetaData*)meta1;

    snprintf(name, sizeof(name), "HAL %s FPS %d", CAMHALVERSION, fps);
    show_string(name, strlen(name), x_pos, y_pos, width, height, dstbuf);

    if (meta) {
        struct v4l2_buffer_metadata_s* drv_metadata = (struct v4l2_buffer_metadata_s*)meta->metedata_drv;
        struct cifisp_isp_metadata* ispdrv_metadata = (struct cifisp_isp_metadata*)drv_metadata->isp;

        y_pos += offset;
        memset(name, 0, sizeof(name));
        snprintf(name, sizeof(name), "EXP T:%2.3f G:%2.2f AE Meas:[%u %u %u][%u %u %u][%u %u %u]",
            meta->exp_time,
            meta->exp_gain,
            ispdrv_metadata->meas_stat.params.ae.exp_mean[11],
            ispdrv_metadata->meas_stat.params.ae.exp_mean[12],
            ispdrv_metadata->meas_stat.params.ae.exp_mean[13],
            ispdrv_metadata->meas_stat.params.ae.exp_mean[16],
            ispdrv_metadata->meas_stat.params.ae.exp_mean[17],
            ispdrv_metadata->meas_stat.params.ae.exp_mean[18],
            ispdrv_metadata->meas_stat.params.ae.exp_mean[21],
            ispdrv_metadata->meas_stat.params.ae.exp_mean[22],
            ispdrv_metadata->meas_stat.params.ae.exp_mean[23]);
        show_string(name, strlen(name), x_pos, y_pos, width, height, dstbuf);

        y_pos += offset;
        memset(name, 0, sizeof(name));
        snprintf(name, sizeof(name), "LSC En:%d R:%d B:%d Gr:%d Gb:%d",
            ((ispdrv_metadata->other_cfg.module_ens & CIFISP_MODULE_LSC) == CIFISP_MODULE_LSC),
            ispdrv_metadata->other_cfg.lsc_config.r_data_tbl[0],
            ispdrv_metadata->other_cfg.lsc_config.b_data_tbl[0],
            ispdrv_metadata->other_cfg.lsc_config.gr_data_tbl[0],
            ispdrv_metadata->other_cfg.lsc_config.gb_data_tbl[0]);
        show_string(name, strlen(name), x_pos, y_pos, width, height, dstbuf);

        y_pos += offset;
        memset(name, 0, sizeof(name));
        snprintf(name, sizeof(name), "AWB En:%d R:%2.2f B:%2.2f Gr:%2.2f Gb:%2.2f Err:%d",
            ((ispdrv_metadata->other_cfg.module_ens & CIFISP_MODULE_AWB_GAIN) == CIFISP_MODULE_AWB_GAIN),
            meta->awb.wb_gain.gain_red,
            meta->awb.wb_gain.gain_blue,
            meta->awb.wb_gain.gain_green_r,
            meta->awb.wb_gain.gain_green_b,
            meta->awb.DoorType);
        show_string(name, strlen(name), x_pos, y_pos, width, height, dstbuf);

        y_pos += offset;
        memset(name, 0, sizeof(name));
        snprintf(name, sizeof(name), "    Wp:%d Y:[%d %d] CSUM:[%d %d] Rrf_Cr:%d Rrf_Cb:%d",
            ispdrv_metadata->meas_stat.params.awb.awb_mean[0].cnt,
            ispdrv_metadata->meas_cfg.awb_meas_config.min_y,
            ispdrv_metadata->meas_cfg.awb_meas_config.max_y,
            ispdrv_metadata->meas_cfg.awb_meas_config.min_c,
            ispdrv_metadata->meas_cfg.awb_meas_config.max_csum,
            ispdrv_metadata->meas_cfg.awb_meas_config.awb_ref_cr,
            ispdrv_metadata->meas_cfg.awb_meas_config.awb_ref_cb);
        show_string(name, strlen(name), x_pos, y_pos, width, height, dstbuf);

        y_pos += offset;
        memset(name, 0, sizeof(name));
        snprintf(name, sizeof(name), "CCM En:%d [%2.2f %2.2f %2.2f][%2.2f %2.2f %2.2f][%2.2f %2.2f %2.2f]",
            ((ispdrv_metadata->other_cfg.module_ens & CIFISP_MODULE_CTK) == CIFISP_MODULE_CTK),
            meta->ctk.coeff0,
            meta->ctk.coeff1,
            meta->ctk.coeff2,
            meta->ctk.coeff3,
            meta->ctk.coeff4,
            meta->ctk.coeff5,
            meta->ctk.coeff6,
            meta->ctk.coeff7,
            meta->ctk.coeff8);
        show_string(name, strlen(name), x_pos, y_pos, width, height, dstbuf);

        y_pos += offset;
        memset(name, 0, sizeof(name));
        snprintf(name, sizeof(name), "IE En:%d Effect:%d",
            ((ispdrv_metadata->other_cfg.module_ens & CIFISP_MODULE_IE) == CIFISP_MODULE_IE),
            ispdrv_metadata->other_cfg.ie_config.effect);
        show_string(name, strlen(name), x_pos, y_pos, width, height, dstbuf);

        y_pos += offset;
        memset(name, 0, sizeof(name));
        snprintf(name, sizeof(name), "WDR En:%d Max_Gain:%d",
            ((ispdrv_metadata->other_cfg.module_ens & CIFISP_MODULE_WDR) == CIFISP_MODULE_WDR),
            meta->wdr.wdr_gain_max_value);
        show_string(name, strlen(name), x_pos, y_pos, width, height, dstbuf);

        y_pos += offset;
        memset(name, 0, sizeof(name));
        snprintf(name, sizeof(name), "FLT En:%d FLT:%d Sharp:%d Grn_Stage:%d",
            ((ispdrv_metadata->other_cfg.module_ens & CIFISP_MODULE_FLT) == CIFISP_MODULE_FLT),
            meta->flt.denoise_level,
            meta->flt.sharp_level,
            ispdrv_metadata->other_cfg.flt_config.grn_stage1);
        show_string(name, strlen(name), x_pos, y_pos, width, height, dstbuf);

        y_pos += offset;
        memset(name, 0, sizeof(name));
        snprintf(name, sizeof(name), "DPF En:%d",
            ((ispdrv_metadata->other_cfg.module_ens & CIFISP_MODULE_DPF) == CIFISP_MODULE_DPF));
        show_string(name, strlen(name), x_pos, y_pos, width, height, dstbuf);

        y_pos += offset;
        memset(name, 0, sizeof(name));
        snprintf(name, sizeof(name), "MeanLuma:%2.2f DON_Fac:%2.2f ill:%s",
            meta->MeanLuma,
            meta->DON_Fac,
            meta->awb.IllName);
        show_string(name, strlen(name), x_pos, y_pos, width, height, dstbuf);

        y_pos += offset;
        memset(name, 0, sizeof(name));
        snprintf(name, sizeof(name), "3DNR %u %u %u %u %u %u %u %u %u %u %u",
            meta->dsp_3DNR.luma_sp_nr_en, meta->dsp_3DNR.luma_sp_nr_level,
            meta->dsp_3DNR.luma_te_nr_en, meta->dsp_3DNR.luma_te_nr_level,
            meta->dsp_3DNR.chrm_sp_nr_en, meta->dsp_3DNR.chrm_sp_nr_level,
            meta->dsp_3DNR.chrm_te_nr_en, meta->dsp_3DNR.chrm_te_nr_level,
            meta->dsp_3DNR.shp_en, meta->dsp_3DNR.shp_level,
            meta->dsp_3DNR.reserves.ex.night_flag);
        show_string(name, strlen(name), x_pos, y_pos, width, height, dstbuf);
    }

    if (noise) {
        memset(name, 0, sizeof(name));
        snprintf(name, sizeof(name), "%u %u %u   %d  %u", noise[0], noise[1],
                 noise[2], parameter_get_ex(), user_noise);
        y_pos += offset;
        show_string(name, strlen(name), x_pos, y_pos, width, height, dstbuf);
    }

    if (sharpness) {
        memset(name, 0, sizeof(name));
        snprintf(name, sizeof(name), "%u %u", sharpness->src_shp_l,
                 sharpness->src_shp_c);
        y_pos += offset;
        show_string(name, strlen(name), x_pos, y_pos, width, height, dstbuf);
    }

    return 0;
}

void iep_process_deinterlace(uint16_t src_w,
                             uint16_t src_h,
                             uint32_t src_fd,
                             uint16_t dst_w,
                             uint16_t dst_h,
                             uint32_t dst_fd)
{
    iep_interface* api = iep_interface::create_new();
    iep_img src1;
    iep_img dst1;

    src1.act_w = src_w;
    src1.act_h = src_h;
    src1.x_off = 0;
    src1.y_off = 0;
    src1.vir_w = src_w;
    src1.vir_h = src_h;
    src1.format = IEP_FORMAT_YCbCr_420_SP;
    src1.mem_addr = src_fd;
    src1.uv_addr = src_fd | (src_w * src_h) << 10;
    src1.v_addr = 0;

    dst1.act_w = dst_w;
    dst1.act_h = dst_h;
    dst1.x_off = 0;
    dst1.y_off = 0;
    dst1.vir_w = dst_w;
    dst1.vir_h = dst_h;
    dst1.format = IEP_FORMAT_YCbCr_420_SP;
    dst1.mem_addr = dst_fd;
    dst1.uv_addr = dst_fd | (dst_w * dst_h) << 10;
    dst1.v_addr = 0;

    api->init(&src1, &dst1);

    api->config_yuv_deinterlace();

    if (api->run_sync())
        printf("%d failure\n", getpid());

    iep_interface::reclaim(api);
}

static void* video_photo_save_pthread(void* arg)
{
    struct Video* video = (struct Video*)arg;
    unsigned int i = 0;

    prctl(PR_SET_NAME, "video_photo_save", 0, 0, 0);

    while (video->pthread_run) {
        pthread_mutex_lock(&video->photo.mutex);
        if (video->photo.state != PHOTO_DISABLE)
            pthread_cond_wait(&video->photo.condition, &video->photo.mutex);
        pthread_mutex_unlock(&video->photo.mutex);

        if (video->photo.state == PHOTO_DISABLE) {
            printf("receive the signal from video_photo_exit()\n");
            break;
        }
        for (i = 0; i < video_get_photo_user_num(video); i++) {
            nv12_encode_mjpg_save(video, i);
            if (!video_photo_is_continuous(video))
                break;
        }
        video_record_takephoto_end(video);
    }

    pthread_exit(NULL);
}

int video_photo_init(struct Video *video)
{
    int width = video->width;
    int height = video->height;
    unsigned char num = 0;
    unsigned int max_num = 1;
    struct photo_param *param = parameter_get_photo_param();

    if (video->type == VIDEO_TYPE_ISP) {
        width = param->width;
        height = param->height;
        max_num = param->max_num;
    }

    video->photo.width =  width;
    video->photo.height = height;

    video_set_photo_max_num(video, max_num);

    if (video->type == VIDEO_TYPE_USB && video->usb_type == USB_TYPE_MJPEG)
        return 0;

    pthread_mutex_init(&video->photo.mutex, NULL);
    pthread_cond_init(&video->photo.condition, NULL);

    video->photo.rga_photo.client = -1;
    video->photo.rga_photo.fd = -1;
    video->photo.rga_photo_ext.client = -1;
    video->photo.rga_photo_ext.fd = -1;

    num = video_photo_is_continuous_inited(video) ? video_get_photo_max_num(video) : 1;

    video_set_photo_user_num(video, num);

#if MJPG_SAVE_THUMBNAIL
    if (video_ion_alloc(&video->photo.rga_photo_ext, 320, 240))
        return -1;
#endif

    video->photo.encode = (struct vpu_encode *)calloc(num,
                                                      sizeof(struct vpu_encode));
    if (!video->photo.encode)
        return -1;

#if MJPG_SAVE_THUMBNAIL
    video->photo.encode_ext = (struct vpu_encode *)calloc(num,
                                                          sizeof(struct vpu_encode));
    if (!video->photo.encode_ext)
        return -1;
#endif

    for (unsigned int i = 0; i < video_get_photo_max_num(video); i++) {
        video->photo.encode[i].jpeg_enc_out.client = -1;
        video->photo.encode[i].jpeg_enc_out.fd = -1;
        if (vpu_nv12_encode_jpeg_init(&video->photo.encode[i], width, height))
            return -1;
#if MJPG_SAVE_THUMBNAIL
        if (vpu_nv12_encode_jpeg_init(&video->photo.encode_ext[i], 320, 240))
            return -1;
#endif
        if (!video_photo_is_continuous_inited(video))
            break;
    }

    if ((video->photo.rga_fd = rk_rga_open()) <= 0)
        return -1;

    if (pthread_create(&video->photo.pid, NULL, video_photo_save_pthread, video)) {
        printf("%s pthread create fail!\n", __func__);
        return -1;
    }

    return 0;
}

void video_photo_exit(struct Video *video)
{
    if (video->type == VIDEO_TYPE_USB && video->usb_type == USB_TYPE_MJPEG)
        return;

    for (unsigned int i = 0; i < video_get_photo_max_num(video); i++) {
        vpu_nv12_encode_jpeg_done(&video->photo.encode[i]);
#if MJPG_SAVE_THUMBNAIL
        vpu_nv12_encode_jpeg_done(&video->photo.encode_ext[i]);
#endif
        if (!video_photo_is_continuous_inited(video))
            break;
    }
    if (video->photo.encode) {
        free(video->photo.encode);
        video->photo.encode = NULL;
    }
    video_ion_free(&video->photo.rga_photo);
#if MJPG_SAVE_THUMBNAIL
    if (video->photo.encode_ext) {
        free(video->photo.encode_ext);
        video->photo.encode_ext = NULL;
    }
    video_ion_free(&video->photo.rga_photo_ext);
#endif

    if (video->photo.pid) {
        pthread_mutex_lock(&video->photo.mutex);
        video->photo.state = PHOTO_DISABLE;
        pthread_cond_signal(&video->photo.condition);
        pthread_mutex_unlock(&video->photo.mutex);
        pthread_join(video->photo.pid, NULL);
    }

    rk_rga_close(video->photo.rga_fd);

    pthread_mutex_destroy(&video->photo.mutex);
    pthread_cond_destroy(&video->photo.condition);
}
