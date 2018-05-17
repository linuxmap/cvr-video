/**
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd
 * author: hogan.wang@rock-chips.com
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

#include "uvc_encode.h"
extern "C" {
#include "parameter.h"
}
#include "uvc_process.h"

static void init_h264_config(MediaConfig &h264_config,
                             int width, int height, int fps, PixelFormat fmt)
{
    memset(&h264_config, 0, sizeof(h264_config));
    VideoConfig& vconfig = h264_config.video_config;
    vconfig.width = width;
    vconfig.height = height;
    vconfig.fmt = fmt;
    vconfig.frame_rate = fps;
    vconfig.level = 51;
    vconfig.gop_size = 1;
    vconfig.profile = 100;

    switch (height) {
    case 1080:
        vconfig.bit_rate = width * height;
        vconfig.quality = MPP_ENC_RC_QUALITY_MEDIUM;
        vconfig.qp_init = 0;
        vconfig.qp_step = 1;
        vconfig.qp_min = 0;
        vconfig.qp_max = 48;
        vconfig.rc_mode = MPP_ENC_RC_MODE_VBR;
        break;
    case 720:
        vconfig.bit_rate = 1000000;
        vconfig.quality = MPP_ENC_RC_QUALITY_BEST;
        vconfig.qp_init = 10;
        vconfig.qp_step = 0;
        vconfig.qp_min = 10;
        vconfig.qp_max = 10;
        vconfig.rc_mode = MPP_ENC_RC_MODE_CBR;
        break;
    default:
        vconfig.bit_rate = 4000000;
        vconfig.quality = MPP_ENC_RC_QUALITY_BEST;
        vconfig.qp_init = 26;
        vconfig.qp_step = 8;
        vconfig.qp_min = 4;
        vconfig.qp_max = 48;
        vconfig.rc_mode = MPP_ENC_RC_MODE_CBR;
        break;
    }
}

static int h264_init_alloc(struct uvc_encode *e, int width, int height)
{
    MediaConfig h264_config;

    init_h264_config(h264_config, width, height, 30, PIX_FMT_NV12);
    if (e->h264_encoder)
        e->h264_encoder->unref();
    e->h264_encoder = new MPPH264Encoder();
    if (!e->h264_encoder) {
        printf("%s: %d failed!\n", __func__, __LINE__);
        return -1;
    }
    e->h264_encoder->InitConfig(h264_config);
    video_ion_free(&e->encode_dst_buff);
    if (video_ion_alloc_rational(&e->encode_dst_buff, width, height, 3, 2) == -1) {
        printf("%s: %d failed!\n", __func__, __LINE__);
        return -1;
    } else {
        e->dst_data.vir_addr_ = e->encode_dst_buff.buffer;
        e->dst_data.ion_data_.fd_ = e->encode_dst_buff.fd;
        e->dst_data.ion_data_.handle_ = e->encode_dst_buff.handle;
        e->dst_data.mem_size_ = e->encode_dst_buff.size;
    }

    return 0;
}

int uvc_encode_init(struct uvc_encode *e)
{
	printf("czy: mpp init\n");
    if (vpu_nv12_encode_jpeg_init_ext(&e->encode, 640, 360, 7)) {
        printf("%s: %d failed!\n", __func__, __LINE__);
        return -1;
    }

    e->h264_encoder = NULL;
    memset(&e->encode_dst_buff, 0, sizeof(e->encode_dst_buff));
    e->encode_dst_buff.fd = -1;
    e->encode_dst_buff.client = -1;
    if (h264_init_alloc(e, 640, 360)) {
        printf("%s: %d failed!\n", __func__, __LINE__);
        return -1;
    }

    e->rga_fd = rk_rga_open();
    if (e->rga_fd < 0) {
        printf("%s: %d failed!\n", __func__, __LINE__);
        return -1;
    }

    memset(&e->uvc_out, 0, sizeof(e->uvc_out));
    e->uvc_out.fd = -1;
    e->uvc_out.client = -1;
    if (video_ion_alloc(&e->uvc_out, 1280, 720)) {
        printf("%s: %d failed!\n", __func__, __LINE__);
        return -1;
    }

    unsigned int max_w, max_h;
    parameter_get_isp_max_resolution(&max_w, &max_h);
    memset(&e->uvc_mid, 0, sizeof(e->uvc_mid));
    e->uvc_mid.fd = -1;
    e->uvc_mid.client = -1;
    if (video_ion_alloc(&e->uvc_mid, max_w, max_h)) {
        printf("%s: %d failed!\n", __func__, __LINE__);
        return -1;
    }

    memset(&e->uvc_in, 0, sizeof(e->uvc_in));
    e->uvc_in.fd = -1;
    e->uvc_in.client = -1;
    if (video_ion_alloc(&e->uvc_in, UVC_WINDOW_WIDTH, UVC_WINDOW_HEIGHT)) {
        printf("%s: %d failed!\n", __func__, __LINE__);
        return -1;
    }
    video_ion_buffer_black(&e->uvc_in, e->uvc_in.width, e->uvc_in.height);
	
    return 0;
}

void uvc_encode_exit(struct uvc_encode *e)
{
    vpu_nv12_encode_jpeg_done(&e->encode);

    if (e->h264_encoder)
        e->h264_encoder->unref();
    video_ion_free(&e->encode_dst_buff);

    rk_rga_close(e->rga_fd);
    video_ion_free(&e->uvc_out);
    video_ion_free(&e->uvc_mid);
    video_ion_free(&e->uvc_in);
}

int uvc_rga_process(struct uvc_encode* e, int in_width, int in_height,
                    void* in_virt, int in_fd)
{
    int width, height;
    int rotate_angle = 0;
    int mid_width, mid_height;
    void* mid_virt;
    int mid_fd;

    if (!uvc_get_user_run_state())
        return -1;
    uvc_get_user_resolution(&width, &height);
    int cmp_width = width > height ? width : height;
    int cmp_height = width > height ? height : width;
    if (in_width * cmp_height == in_height * cmp_width) {
        mid_width = in_width;
        mid_height = in_height;
        mid_virt = in_virt;
        mid_fd = in_fd;
    } else if (cmp_width != 0 && cmp_height != 0) {
        int src_fd = in_fd, src_w = in_width, src_h = in_height;
        int src_fmt = RGA_FORMAT_YCBCR_420_SP;
        int src_vir_w = src_w, src_vir_h = src_h;
        int src_act_w, src_act_h, src_x_offset, src_y_offset;
        if (in_width * cmp_height > in_height * cmp_width) {
            src_act_w = src_w * cmp_width / cmp_height * in_height / in_width;
            src_act_w = src_act_w - src_act_w % 16;
            src_act_h = src_h;
            src_x_offset = (src_w - src_act_w) / 2;
            src_y_offset = 0;
        } else {
            src_act_w = src_w;
            src_act_h = src_h * in_width / in_height * cmp_height / cmp_width;
            src_act_h = src_act_h - src_act_h % 16;
            src_x_offset = 0;
            src_y_offset = (src_h - src_act_h) / 2;
        }

        int dst_fd = e->uvc_mid.fd, dst_w = src_act_w, dst_h = src_act_h;
        int dst_fmt = RGA_FORMAT_YCBCR_420_SP;
        if (rk_rga_ionfd_to_ionfd_rotate_offset_ext(e->rga_fd,
                                                    src_fd, src_w, src_h, src_fmt,
                                                    src_vir_w, src_vir_h, src_act_w, src_act_h,
                                                    src_x_offset, src_y_offset,
                                                    dst_fd, dst_w, dst_h, dst_fmt, 0)) {
            printf("%s: %d fail!\n", __func__, __LINE__);
            return -1;
        }

        mid_width = dst_w;
        mid_height = dst_h;
        mid_virt = e->uvc_mid.buffer;
        mid_fd = e->uvc_mid.fd;
    } else {
        return -1;
    }
    if (mid_width == width && mid_height == height) {
        e->out_virt = mid_virt;
        e->out_fd = mid_fd;
    } else if (width != 0 && height != 0) {
        int src_w, src_h, src_fd, dst_w, dst_h, dst_fd;
        src_w = mid_width;
        src_h = mid_height;
        src_fd = mid_fd;
        dst_w = width;
        dst_h = height;
        dst_fd = e->uvc_out.fd;
        if (width < height)
            rotate_angle = 90;
        if (rk_rga_ionfd_to_ionfd_rotate(e->rga_fd, src_fd, src_w, src_h, RGA_FORMAT_YCBCR_420_SP,
                                         src_w, src_h, dst_fd, dst_w, dst_h,
                                         RGA_FORMAT_YCBCR_420_SP, rotate_angle)) {
            printf("%s: %d fail!\n", __func__, __LINE__);
            return -1;
        }
        e->out_virt = e->uvc_out.buffer;
        e->out_fd = e->uvc_out.fd;
    } else {
        return -1;
    }

    return 0;
}

bool uvc_encode_process(struct uvc_encode *e)
{
    int ret = 0;
    unsigned int fcc;
    int size;
    void* extra_data = nullptr;
    size_t extra_size = 0;
    int width, height;
    int jpeg_quant;
    void* virt = e->out_virt;
    int fd = e->out_fd;
    void* hnd = NULL;

    if (!uvc_get_user_run_state() || !uvc_buffer_write_enable())
        return false;

    uvc_get_user_resolution(&width, &height);
    fcc = uvc_get_user_fcc();
    switch (fcc) {
    case V4L2_PIX_FMT_YUYV:
        size = width * height * 2;
        uvc_buffer_write(extra_data, extra_size, virt, size, fcc);
        break;
    case V4L2_PIX_FMT_MJPEG:
        if (e->encode.width != width || e->encode.height != height) {
            vpu_nv12_encode_jpeg_done(&e->encode);
            jpeg_quant = (height >= 1080) ? 5 : 7;
            if (vpu_nv12_encode_jpeg_init_ext(&e->encode, width, height, jpeg_quant)) {
                printf("%s: %d failed!\n", __func__, __LINE__);
                return false;
            }
        }
        size = width * height * 3 / 2;
        ret = vpu_nv12_encode_jpeg_doing(&e->encode, virt, fd, size);
        if (!ret)
            uvc_buffer_write(extra_data, extra_size,
                             e->encode.enc_out_data, e->encode.enc_out_length, fcc);
        break;
    case V4L2_PIX_FMT_H264: {
        if (e->encode_dst_buff.width != width || e->encode_dst_buff.height != height)
            if (h264_init_alloc(e, width, height))
                return false;
        Buffer dst_buf(e->dst_data);
        struct timeval time;
        gettimeofday(&time, NULL);
        BufferData src_data;
        src_data.vir_addr_ = virt;
        src_data.ion_data_.fd_ = fd;
        src_data.ion_data_.handle_ = (ion_user_handle_t)hnd;
        src_data.mem_size_ = width * height * 3 / 2;
        src_data.update_timeval_ = time;
        const Buffer src_buf(src_data);
        int ret = -1;
        ret = e->h264_encoder->EncodeOneFrame(const_cast<Buffer*>(&src_buf), &dst_buf, nullptr);
        if (!ret) {
            if (dst_buf.GetValidDataSize() <= 0) {
                printf("error encode h264.\n");
                return false;
            }
            assert(dst_buf.GetValidDataSize() > 0);
            uint8_t* data = static_cast<uint8_t*>(dst_buf.GetVirAddr());
            int size = dst_buf.GetValidDataSize();
            if (dst_buf.GetUserFlag() & MPP_PACKET_FLAG_INTRA)
                e->h264_encoder->GetExtraData(extra_data, extra_size);
            uvc_buffer_write(extra_data, extra_size, data, size, fcc);
        }
    }
    break;
    default:
        printf("%s: not support fcc: %u\n", __func__, fcc);
        break;
    }

    return true;
}
