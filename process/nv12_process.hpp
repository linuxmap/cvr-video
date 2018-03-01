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

#ifndef __NV12_PROCESS_HPP__
#define __NV12_PROCESS_HPP__

#include "video.hpp"
#include "nv12_3dnr.hpp"
#include "dpp_buffer.h"
extern "C" {
#include "RSFaceSDK.h"
}
#include "face_interface.h"

#define APPID          "f4cbb59ac975f839cf4e5c5c4662eda7"
#define APPSECRET      "09aae635d26e4e52febc9ea1c3fd58222b6f3498"
#define DBPATH         "/param/face.db"

#ifdef _PCBA_SERVICE_
typedef int (*VIDEO_PCBA_CB_TYPE)(void * buff, int width, int height, size_t size);
extern VIDEO_PCBA_CB_TYPE m_pfunc;
extern int camera_test_type;

extern "C" {
    extern int rk_rga_ionfd_to_ionfd( int rga_fd,  int src_fd, int src_w, int src_h,
                                      int src_fmt, int src_vir_w, int src_vir_h,
                                      int dst_fd,  int dst_w, int dst_h,
                                      int dst_fmt, int rotate, int rotate_mode) ;

    extern void video_pcba_yuv_to_rgba(int rga_fd,  int src_fd, int src_w, int src_h,
                                       int src_fmt, int src_vir_w, int src_vir_h,
                                       int dst_fd,  int dst_w, int dst_h,
                                       int dst_fmt, int rotate, int rotate_mode);

    extern void video_do_pcba_call_back(void * buff, int width, int height, size_t size);
}
#endif

extern bool is_record_mode;

void iep_process_deinterlace(uint16_t src_w,
                             uint16_t src_h,
                             uint32_t src_fd,
                             uint16_t dst_w,
                             uint16_t dst_h,
                             uint32_t dst_fd);
int show_video_mark(int width,
                    int height,
                    void* dstbuf,
                    int fps,
                    void* meta1,
                    uint32_t* noise,
                    struct dpp_sharpness* sharpness);
int video_photo_init(struct Video *video);
void video_photo_exit(struct Video *video);
int video_rga_photo_process(struct Video* video,
                            int s_fd, int s_width, int s_height, void* s_buf);
void video_set_photo_user_num(struct Video* video, unsigned int num);
unsigned int video_get_photo_user_num(struct Video* video);

class NV12_Display : public StreamPUBase
{
    struct Video* video;
    int rga_fd = -1;

public:
    NV12_Display(struct Video* p) : StreamPUBase("NV12_Display", true, true) {
        video = p;
    }
    ~NV12_Display() {
#ifdef _PCBA_SERVICE_
        if (rga_fd >= 0) {
            rk_rga_close(rga_fd);
            rga_fd = -1;
        }
#endif
    }
    bool processFrame(shared_ptr<BufferBase> inBuf,
                      shared_ptr<BufferBase> outBuf) {
        int src_w, src_h, src_fd, vir_w, vir_h, src_fmt;
        if (video->pthread_run && inBuf.get()) {
            if (video->type == VIDEO_TYPE_ISP) {
#ifdef USE_DISP_TS
                void* address2 = inBuf->getVirtAddr();
                yuv420_display_adas(video->width, video->height, address2);
#endif
            }

            if (video->out_exist) {
                src_w = inBuf->getWidth();
                src_h = inBuf->getHeight();
                src_fd = (int)(inBuf->getFd());
            } else {
#if MAIN_APP_NEED_DOWNSCALE_STREAM
                Dpp_Buffer* dppBuf = dynamic_cast<Dpp_Buffer*>(inBuf.get());

                if (dppBuf && dppBuf->getDownscaleFd() >= 0) {
                    src_w = dppBuf->getDownscaleWidth();
                    src_h = dppBuf->getDownscaleHeight();
                    src_fd = (int)(dppBuf->getDownscaleFd());
                } else
#endif
                {
                    src_w = inBuf->getWidth();
                    src_h = inBuf->getHeight();
                    src_fd = (int)(inBuf->getFd());
                }
            }

            if (video->jpeg_dec.decode) {
                if (video->jpeg_dec.decode->pkt_size <= 0)
                    return false;
                vir_w = ALIGN(src_w, 16);
                vir_h = ALIGN(src_h, 16);
                if (MPP_FMT_YUV422SP == video->jpeg_dec.decode->fmt)
                    src_fmt = RGA_FORMAT_YCBCR_422_SP;
                else
                    src_fmt = RGA_FORMAT_YCBCR_420_SP;
            } else {
                vir_w = src_w;
                vir_h = src_h;
                src_fmt = RGA_FORMAT_YCBCR_420_SP;
            }
//			void* address = inBuf->getVirtAddr();
//			yuv420_draw_rectangle(address, src_w, src_h, {100,100,50,50}, set_yuv_color(COLOR_R));

#ifdef _PCBA_SERVICE_
            if (rga_fd < 0) {
                rga_fd = rk_rga_open();
            }

            if (m_pfunc != NULL) {

                switch ( camera_test_type ) {
                case 1:
                    if (video->type == VIDEO_TYPE_ISP) {
                        video_pcba_yuv_to_rgba(rga_fd, src_fd, src_w, src_h,
                                               RGA_FORMAT_YCRCB_420_SP, vir_w, vir_h,
                                               video->pcba_ion.fd, src_w, src_h,
                                               RGA_FORMAT_RGBA_8888, 0, 0);

                        video_do_pcba_call_back(video->pcba_ion.buffer, video->pcba_ion.width, video->pcba_ion.height,
                                                video->pcba_ion.size);
                    }
                    break;

                case 2:
                    if (video->type == VIDEO_TYPE_USB) {
                        video_pcba_yuv_to_rgba(rga_fd, src_fd, src_w, src_h,
                                               RGA_FORMAT_YCRCB_420_SP, vir_w, vir_h,
                                               video->pcba_ion.fd, src_w, src_h,
                                               RGA_FORMAT_RGBA_8888, 0, 0);

                        video_do_pcba_call_back(video->pcba_ion.buffer, video->pcba_ion.width, video->pcba_ion.height,
                                                video->pcba_ion.size);
                    }
                    break;

                case 3:
                    if (video->type == VIDEO_TYPE_CIF)
                        if (video->pcba_cif_type == 0) {
                            video_pcba_yuv_to_rgba(rga_fd, src_fd, src_w, src_h,
                                                   RGA_FORMAT_YCRCB_420_SP, vir_w, vir_h,
                                                   video->pcba_ion.fd, src_w, src_h,
                                                   RGA_FORMAT_RGBA_8888, 0, 0);

                            video_do_pcba_call_back(video->pcba_ion.buffer, video->pcba_ion.width, video->pcba_ion.height,
                                                    video->pcba_ion.size);
                        }
                    break;

                case 4:
                    if (video->type == VIDEO_TYPE_CIF)
                        if (video->pcba_cif_type == 1) {
                            video_pcba_yuv_to_rgba(rga_fd, src_fd, src_w, src_h,
                                                   RGA_FORMAT_YCRCB_420_SP, vir_w, vir_h,
                                                   video->pcba_ion.fd, src_w, src_h,
                                                   RGA_FORMAT_RGBA_8888, 0, 0);

                            video_do_pcba_call_back(video->pcba_ion.buffer, video->pcba_ion.width, video->pcba_ion.height,
                                                    video->pcba_ion.size);
                        }
                    break;

                case 5:
                    if (video->type == VIDEO_TYPE_CIF)
                        if (video->pcba_cif_type == 2) {
                            video_pcba_yuv_to_rgba(rga_fd, src_fd, src_w, src_h,
                                                   RGA_FORMAT_YCRCB_420_SP, vir_w, vir_h,
                                                   video->pcba_ion.fd, src_w, src_h,
                                                   RGA_FORMAT_RGBA_8888, 0, 0);

                            video_do_pcba_call_back(video->pcba_ion.buffer, video->pcba_ion.width, video->pcba_ion.height,
                                                    video->pcba_ion.size);
                        }
                    break;

                case 6:
                    if (video->type == VIDEO_TYPE_CIF)
                        if (video->pcba_cif_type == 3) {
                            video_pcba_yuv_to_rgba(rga_fd, src_fd, src_w, src_h,
                                                   RGA_FORMAT_YCRCB_420_SP, vir_w, vir_h,
                                                   video->pcba_ion.fd, src_w, src_h,
                                                   RGA_FORMAT_RGBA_8888, 0, 0);

                            video_do_pcba_call_back(video->pcba_ion.buffer, video->pcba_ion.width, video->pcba_ion.height,
                                                    video->pcba_ion.size);
                        }
                    break;

                default:
                    goto display_flag;
                }

            }
display_flag:
#endif
            display_the_window(video->disp_position, src_fd, src_w, src_h,
                               src_fmt, vir_w, vir_h);
        }
        return true;
    }
};

class NV12_MJPG : public StreamPUBase
{
    struct Video* video;

public:
    NV12_MJPG(struct Video* p) : StreamPUBase("NV12_MJPG", true, true) {
        video = p;
    }
    ~NV12_MJPG() {}

    bool processFrame(shared_ptr<BufferBase> inBuf,
                      shared_ptr<BufferBase> outBuf) {
        static unsigned int cnt = 0;
        if (video->pthread_run && video->photo.state == PHOTO_ENABLE && inBuf.get()) {
            cnt++;
            if (cnt == video_get_photo_user_num(video))
                video->photo.state = PHOTO_BEGIN;
            video_rga_photo_process(video, inBuf->getFd(), inBuf->getWidth(),
                                    inBuf->getHeight(), inBuf->getVirtAddr());
        }
        if (video->photo.state != PHOTO_ENABLE)
            cnt = 0;
        return true;
    }
};

class NV12_TS : public StreamPUBase
{
public:
    NV12_TS(struct Video* p) :
      StreamPUBase("NV12_TS", true, true),
      video(p),
      active(false) {}
    void SetActive(bool val) { active = val; }
    bool processFrame(shared_ptr<BufferBase> inBuf,
                      shared_ptr<BufferBase> outBuf) override {
        assert(video->ts_handler);
        if (active && video->pthread_run && inBuf.get()) {
            struct timeval time_val = {0, 0};
            if (inBuf->getMetaData() && inBuf->getMetaData()->metedata_drv) {
                struct v4l2_buffer_metadata_s* metadata_drv =
                    (struct v4l2_buffer_metadata_s*)inBuf->getMetaData()->metedata_drv;
                assert(metadata_drv);
                time_val = metadata_drv->frame_t.vs_t;
            } else {
                gettimeofday(&time_val, NULL);
            }
            VideoConfig config;
            config.fmt = PIX_FMT_NV12;
            config.width = inBuf->getWidth();
            config.height = inBuf->getHeight();
            video->ts_handler->Process(inBuf->getFd(), config, time_val);
        }
        return true;
    }

private:
    struct Video* video;
    volatile bool active;
};

#define LIGHT_DAY_NIGHT_BOUNDARY 20
class NV12_Encode : public StreamPUBase
{
    struct Video* video;
    bool got_valid_input;
    PixelFormat input_fmt;
    int enc_denoise;
    int enc_rotation;
public:
    NV12_Encode(struct Video* p) : StreamPUBase("NV12_Encode", true, true),
        video(p), got_valid_input(false), input_fmt(PIX_FMT_NV12),
        enc_denoise(0), enc_rotation(MPP_ENC_ROT_0) {}
    ~NV12_Encode() {}

    bool processFrame(shared_ptr<BufferBase> inBuf,
                      shared_ptr<BufferBase> outBuf) {
        if (video->save_en && video->pthread_run) {
            void* virt = NULL;
            int fd = -1;
            void* hnd = NULL;
            size_t size = 0;
            struct timeval time_val = {0};
            if (inBuf.get()) {
                virt = inBuf->getVirtAddr();
                fd = (int)inBuf->getFd();
                hnd = inBuf->getHandle();
                size = inBuf->getWidth() * inBuf->getHeight() * 3 / 2;
                struct vpu_decode* vpu_dec = video->jpeg_dec.decode;
                if (vpu_dec) {
                    if (inBuf->getDataSize() > 0) {
                        input_fmt = ConvertToPixFmt(video->jpeg_dec.decode->fmt);
                        size = BaseEncoder::CalPixelSize(inBuf->getWidth(),
                                                         inBuf->getHeight(),
                                                         input_fmt);
                        got_valid_input = true;
                    } else {
                        printf("Debug: NV12_Encode inBuf datasize get 0\n");
                        virt = NULL;
                        fd = -1;
                        hnd = NULL;
                        size = 0;
                    }
                } else {
                    got_valid_input = true;
                }

                Dpp_Buffer* buf = dynamic_cast<Dpp_Buffer*>(inBuf.get());
                if (buf)
                    time_val = buf->getTimestamp();
                else
                    gettimeofday(&time_val, NULL);
                if (video->pthread_run && video->encode_handler) {
                    struct HAL_Buffer_MetaData* meta = inBuf->getMetaData();
                    MppEncPrepCfg &precfg =
                        video->encode_handler->get_mpp_enc_prepcfg();
                    precfg.denoise = meta ?
                        (meta->DON_Fac > LIGHT_DAY_NIGHT_BOUNDARY ? 0 : 1) : 0;
                    if (precfg.denoise != enc_denoise) {
                        precfg.change |= MPP_ENC_PREP_CFG_CHANGE_DENOISE;
                        enc_denoise = precfg.denoise;
                    }
                    precfg.rotation = MPP_ENC_ROT_0;
                    if (precfg.rotation != enc_rotation) {
                        precfg.change |= MPP_ENC_PREP_CFG_CHANGE_ROTATION;
                        enc_rotation = precfg.rotation;
                    }
                    if (buf && buf->get_nr_valid()) {
                        struct dpp_sharpness sharpness;
                        buf->getSharpness(sharpness);
                        precfg.change |= MPP_ENC_PREP_CFG_CHANGE_SHARPEN;
                        precfg.sharpen.enable_y = sharpness.src_shp_l;
                        precfg.sharpen.enable_uv = sharpness.src_shp_c;
                        precfg.sharpen.threshold = sharpness.src_shp_thr;
                        precfg.sharpen.div = sharpness.src_shp_div;
                        precfg.sharpen.coef[0] = sharpness.src_shp_w0;
                        precfg.sharpen.coef[1] = sharpness.src_shp_w1;
                        precfg.sharpen.coef[2] = sharpness.src_shp_w2;
                        precfg.sharpen.coef[3] = sharpness.src_shp_w3;
                        precfg.sharpen.coef[4] = sharpness.src_shp_w4;
                    }
                }
            }
            // If never got a valid input data, return false.
            if (!got_valid_input)
                return false;
            if (video->save_en && video->pthread_run &&
                h264_encode_process(video, virt, fd, hnd,
                                    size, time_val, input_fmt)) {
                video_record_signal(video);
                return false;
            }
        }
        return true;
    }
};

#if MAIN_APP_NEED_DOWNSCALE_STREAM
class NV12_Encode_S : public StreamPUBase
{
    struct Video* video;

public:
    bool encoding;
    NV12_Encode_S(struct Video* p) : StreamPUBase("NV12_Encode_S", true, true),
        video(p), encoding(false) {}

    virtual ~NV12_Encode_S() {
        assert(!encoding);
    }

    virtual void GetBufferInfo(shared_ptr<BufferBase> inBuf,
                               BufferData &input_data) {
        if (inBuf) {
            int w = inBuf->getWidth();
            int h = inBuf->getHeight();
            input_data.vir_addr_ = inBuf->getVirtAddr();
            input_data.ion_data_.fd_ = (int)inBuf->getFd();
            input_data.ion_data_.handle_ = (ion_user_handle_t)inBuf->getHandle();
            input_data.mem_size_ = w * h * 3 / 2;
            gettimeofday(&input_data.update_timeval_, NULL);
        }
    }

    bool processFrame(shared_ptr<BufferBase> inBuf,
                      shared_ptr<BufferBase> outBuf) {
        if (video->save_en && video->pthread_run && video->encode_handler_s) {
            BufferData input_data;
            GetBufferInfo(inBuf, input_data);
            if (input_data.ion_data_.fd_ >= 0) {
                encoding = true;
                Buffer input_buffer(input_data);
                video->encode_handler_s->h264_encode_process(
                    input_buffer, PIX_FMT_NV12);
                encoding = false;
            }
        }
        return true;
    }
};

class DPP_NV12_Encode_S : public NV12_Encode_S
{
public:
    DPP_NV12_Encode_S(struct Video* p): NV12_Encode_S(p) {}
    virtual void GetBufferInfo(shared_ptr<BufferBase> inBuf,
                               BufferData &input_data) {
        Dpp_Buffer* dppBuf = dynamic_cast<Dpp_Buffer*>(inBuf.get());
        if (dppBuf && dppBuf->getDownscaleFd() >= 0) {
            int w = dppBuf->getDownscaleWidth();
            int h = dppBuf->getDownscaleHeight();
            input_data.vir_addr_ = dppBuf->getDownscaleVirt();
            input_data.ion_data_.fd_ = dppBuf->getDownscaleFd();
            input_data.ion_data_.handle_ = 0;
            input_data.mem_size_ = w * h * 3 / 2;
            input_data.update_timeval_ = dppBuf->getTimestamp();
        }
    }
};
#endif

class NV12_UVC : public StreamPUBase
{
    struct Video* video;

public:
    NV12_UVC(struct Video* p) : StreamPUBase("NV12_UVC", true, true) {
        video = p;

    }
    ~NV12_UVC() {}
    bool processFrame(shared_ptr<BufferBase> inBuf,
                      shared_ptr<BufferBase> outBuf) {
        int src_w, src_h, src_fd, vir_w, vir_h, src_fmt;

        src_w = inBuf->getWidth();
        src_h = inBuf->getHeight();
        src_fd = (int)(inBuf->getFd());
        if (video->jpeg_dec.decode) {
            if (video->jpeg_dec.decode->pkt_size <= 0)
                return false;
            vir_w = ALIGN(src_w, 16);
            vir_h = ALIGN(src_h, 16);
            if (MPP_FMT_YUV422SP == video->jpeg_dec.decode->fmt)
                src_fmt = RGA_FORMAT_YCBCR_422_SP;
            else
                src_fmt = RGA_FORMAT_YCBCR_420_SP;
        } else {
            vir_w = src_w;
            vir_h = src_h;
            src_fmt = RGA_FORMAT_YCBCR_420_SP;
        }
        uvc_process_the_window(video->uvc_position, inBuf->getVirtAddr(), src_fd,
                               src_w, src_h, src_fmt, vir_w, vir_h);
        return true;
    }
};

class NV12_IEP : public StreamPUBase
{
    struct Video* video;

public:
    NV12_IEP(struct Video* p) : StreamPUBase("NV12_IEP", true, false) {
        video = p;
    }
    ~NV12_IEP() {}
    bool processFrame(shared_ptr<BufferBase> inBuf,
                      shared_ptr<BufferBase> outBuf) {
        if (video->pthread_run && inBuf.get() && outBuf.get()) {
            iep_process_deinterlace(inBuf->getWidth(), inBuf->getHeight(),
                                    inBuf->getFd(), outBuf->getWidth(),
                                    outBuf->getHeight(), outBuf->getFd());
            outBuf->setDataSize(video->width * video->height * 3 / 2);
        }

        return true;
    }
};

class NV12_MIRROR : public StreamPUBase
{
    struct Video* video;
    int rga_fd;

public:
    NV12_MIRROR(struct Video* p) : StreamPUBase("NV12_MIRROR", true, false) {
        video = p;
        rga_fd = rk_rga_open();
    }
    ~NV12_MIRROR() {
        rk_rga_close(rga_fd);
    }
    bool processFrame(shared_ptr<BufferBase> inBuf,
                      shared_ptr<BufferBase> outBuf) {
        if (rga_fd < 0)
            return false;
        if (video->pthread_run && inBuf.get() && outBuf.get()) {
            int dst_fd, dst_w, dst_h, dst_fmt;
            int src_fd, src_w, src_h, src_fmt, vir_w, vir_h;

            src_fd = inBuf->getFd();
            src_w = inBuf->getWidth();
            src_h = inBuf->getHeight();
            src_fmt = RGA_FORMAT_YCBCR_420_SP;
            vir_w = src_w;
            vir_h = src_h;
            dst_fd = outBuf->getFd();
            dst_w = outBuf->getWidth();
            dst_h = outBuf->getHeight();
            dst_fmt = RGA_FORMAT_YCBCR_420_SP;
            rk_rga_ionfd_to_ionfd_mirror(rga_fd,
                                         src_fd, src_w, src_h, src_fmt, vir_w, vir_h,
                                         dst_fd, dst_w, dst_h, dst_fmt);
            outBuf->setDataSize(video->width * video->height * 3 / 2);
        }

        return true;
    }
};

class NV12_FLIP : public StreamPUBase
{
    struct Video* video;
    int rga_fd;

public:
    NV12_FLIP(struct Video* p) : StreamPUBase("NV12_FLIP", true, false) {
        video = p;
        rga_fd = rk_rga_open();
    }
    ~NV12_FLIP() {
        rk_rga_close(rga_fd);
    }
    bool processFrame(shared_ptr<BufferBase> inBuf,
                      shared_ptr<BufferBase> outBuf) {
        if (rga_fd < 0)
            return false;
        if (video->pthread_run && inBuf.get() && outBuf.get()) {
            int dst_fd, dst_w, dst_h, dst_fmt;
            int src_fd, src_w, src_h, src_fmt, vir_w, vir_h;

            src_fd = inBuf->getFd();
            src_w = inBuf->getWidth();
            src_h = inBuf->getHeight();
            src_fmt = RGA_FORMAT_YCBCR_420_SP;
            vir_w = src_w;
            vir_h = src_h;
            dst_fd = outBuf->getFd();
            dst_w = outBuf->getWidth();
            dst_h = outBuf->getHeight();
            dst_fmt = RGA_FORMAT_YCBCR_420_SP;
            rk_rga_ionfd_to_ionfd_rotate(rga_fd,
                                         src_fd, src_w, src_h, src_fmt, vir_w, vir_h,
                                         dst_fd, dst_w, dst_h, dst_fmt, 180);
            outBuf->setDataSize(video->width * video->height * 3 / 2);
        }

        return true;
    }
};

class MP_RGA : public StreamPUBase
{
    struct Video* video;
    int rga_fd;

public:
    MP_RGA(struct Video* p) : StreamPUBase("MP_RGA", true, false) {
        video = p;
        rga_fd = rk_rga_open();
        if (rga_fd < 0)
            printf("%s open rga fd failed!\n", __func__);
    }
    ~MP_RGA() {
        rk_rga_close(rga_fd);
    }
    bool processFrame(shared_ptr<BufferBase> inBuf,
                      shared_ptr<BufferBase> outBuf) {
        if (rga_fd < 0) {
            video_record_signal(video);
            return false;
        }
        if (video->pthread_run && inBuf.get() && outBuf.get()) {
            int src_w, src_h, src_fd, dst_w, dst_h, dst_fd;
            src_w = inBuf->getWidth();
            src_h = inBuf->getHeight();
            src_fd = inBuf->getFd();
            dst_w = outBuf->getWidth();
            dst_h = outBuf->getHeight();
            dst_fd = outBuf->getFd();
            if (rk_rga_ionfd_to_ionfd_scal(rga_fd,
                                           src_fd, src_w, src_h, RGA_FORMAT_YCBCR_420_SP,
                                           dst_fd, dst_w, dst_h, RGA_FORMAT_YCBCR_420_SP,
                                           0, 0, dst_w, dst_h, src_w, src_h)) {
                printf("%s: %d fail!\n", __func__, __LINE__);
                return false;
            }
            outBuf->setDataSize(dst_w * dst_h * 3 / 2);
        }

        return true;
    }
};

class NV12_RAW : public StreamPUBase
{
    struct Video* video;

public:
    NV12_RAW(struct Video* p) : StreamPUBase("MP_DSP", true, true) { video = p; }
    ~NV12_RAW() {}
    bool processFrame(shared_ptr<BufferBase> inBuf,
                      shared_ptr<BufferBase> outBuf) {
#ifdef NV12_RAW_DATA
        static FILE* f_tmp = NULL;
        static char filename[30] = {0};
        static int cnt = 0;
        char c = '\0';
        char command[50] = {0};
        static int cmd_cnt = 0;
        int i = 0;
        int src_w, src_h, src_fd, dst_w, dst_h, dst_fd;
        static int write_cnt = 0;

        struct timeval t0, t1;

        if (video->pthread_run && inBuf.get() && nv12_raw_cnt % 2) {
            write_cnt++;
            if (write_cnt < 5)
                return true;
            if (!f_tmp) {
                cmd_cnt++;
                snprintf(filename, sizeof(filename), "/mnt/sdcard/NV12_RAW_%d",
                         cmd_cnt);
                f_tmp = fopen(filename, "wb");
            }
            if (f_tmp && cnt < NV12_RAW_CNT) {
                src_w = inBuf->getWidth();
                src_h = inBuf->getHeight();
                src_fd = inBuf->getFd();
                dst_w = video->raw[cnt].width;
                dst_h = video->raw[cnt].height;
                dst_fd = video->raw[cnt].fd;

                gettimeofday(&t0, NULL);
                if (rk_rga_ionfd_to_ionfd_scal(video->raw_fd,
                                               src_fd, src_w, src_h, RGA_FORMAT_YCBCR_420_SP,
                                               dst_fd, dst_w, dst_h, RGA_FORMAT_YCBCR_420_SP,
                                               0, 0, dst_w, dst_h, src_w, src_h)) {
                    printf("rk_rga_ionfdnv12_to_ionfdnv12_scal fail!\n");
                }
                gettimeofday(&t1, NULL);
                printf("rga:%ldus\n",
                       (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec);
                cnt++;
            } else if (f_tmp && cnt >= NV12_RAW_CNT) {
                gettimeofday(&t0, NULL);
                for (i = 0; i < NV12_RAW_CNT; i++) {
                    fwrite(video->raw[i].buffer, 1, inBuf->getDataSize(), f_tmp);
                }
                gettimeofday(&t1, NULL);
                printf("write:%ldus\n",
                       (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec);
                cnt = 0;
                fflush(f_tmp);
                fclose(f_tmp);
                f_tmp = NULL;
                write_cnt = 0;
            }
        } else {
            if (f_tmp) {
                gettimeofday(&t0, NULL);
                for (i = 0; i < cnt; i++) {
                    fwrite(video->raw[i].buffer, 1, inBuf->getDataSize(), f_tmp);
                }
                gettimeofday(&t1, NULL);
                printf("write:%ldus\n",
                       (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec);
                cnt = 0;
                fflush(f_tmp);
                fclose(f_tmp);
                f_tmp = NULL;
                write_cnt = 0;
            }
        }
#endif

        return true;
    }
};

class NV12_ReadFace : public StreamPUBase
{
    struct Video* video;
    RSHandle mFaceRecognition = NULL;	
    RSHandle mLicense = NULL;
    RSHandle mDetect = NULL;
    RSHandle mFaceTrack = NULL;
	rs_face* pFaceArray = NULL;
	rs_face_feature pFeature;
	int iFaceCount = 0;
	int trackid = 0;
	int InitReadFace() {
		rsInitLicenseManager(&mLicense, APPID, APPSECRET);
		if (mLicense == NULL) {
			printf("init RSface license error!\n");
			return -1;
		}
		rsInitFaceDetect(&mDetect, mLicense);
		if (mDetect == NULL) {
			printf("init RSface detect error!\n");
			return -1;
		}
		rsInitFaceTrack(&mFaceTrack, mLicense);
		if (mFaceTrack == NULL) {
			printf("init RSface track error!\n");
			return -1;
		}
		rsInitFaceRecognition(&mFaceRecognition, mLicense, DBPATH);
		if (mFaceRecognition == NULL) {
			printf("init RSface recognition error!\n");
			return -1;
		}
		rsRecognitionSetConfidence(mFaceRecognition, 55);
		return 0;
	}

	void UnInitReadFace() {
		rsUnInitFaceTrack(&mFaceTrack);
		rsUnInitFaceDetect(&mDetect);
		rsUnInitLicenseManager(&mLicense);
		rsUnInitFaceRecognition(&mFaceRecognition);
	}
public:
    NV12_ReadFace(struct Video* p) : StreamPUBase("NV12_ReadFace", true, true) {
        video = p;
		printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>readface init!!!\n");
		InitReadFace();
    }
	
    ~NV12_ReadFace() {
		UnInitReadFace();
	}
    bool processFrame(shared_ptr<BufferBase> inBuf,
                      shared_ptr<BufferBase> outBuf) {
//        int src_w, src_h, src_str, src_fd, vir_w, vir_h, src_fmt, ret;
        int src_w, src_h, src_str, ret;
		unsigned char * address = NULL;

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

        if (video->pthread_run && inBuf.get()) {
            src_w = inBuf->getWidth();
            src_h = inBuf->getHeight();
			src_str = inBuf->getStride();
//			src_fd = (int)(inBuf->getFd());
			address = (unsigned char*)inBuf->getVirtAddr();

//			if (video->jpeg_dec.decode) {
//				if (video->jpeg_dec.decode->pkt_size <= 0)
//					return false;
//				vir_w = ALIGN(src_w, 16);
//				vir_h = ALIGN(src_h, 16);
//				if (MPP_FMT_YUV422SP == video->jpeg_dec.decode->fmt)
//					src_fmt = RGA_FORMAT_YCBCR_422_SP;
//				else
//					src_fmt = RGA_FORMAT_YCBCR_420_SP;
//			} else {
//				vir_w = src_w;
//				vir_h = src_h;
//				src_fmt = RGA_FORMAT_YCBCR_420_SP;
//			}

			if (mFaceTrack != NULL) {
				ret = rsRunFaceTrack(mFaceTrack, address, PIX_FORMAT_NV12, src_w, src_h, src_str, RS_IMG_CLOCKWISE_ROTATE_0, &pFaceArray, &iFaceCount);
				if (0 != ret) {
					iFaceCount = 0;
				}
//				printf(">>>>>>>>>>>>>>>%d faces have been detected\n", iFaceCount);
				if (pFaceArray) {
//					for (int i = 0; i < iFaceCount; i++) {
//						printf(">>>>>>>>>>>>>>>faces%d have been detected, trackId is %d\n", i, pFaceArray[i].trackId);
//						YUV_Rect rect_rio = {pFaceArray[i].rect.left, pFaceArray[i].rect.top, pFaceArray[i].rect.height, pFaceArray[i].rect.height};
//
//			            src_w = inBuf->getWidth();
//			            src_h = inBuf->getHeight();
//						yuv420_draw_rectangle(address, src_w, src_h, rect_rio, set_yuv_color(COLOR_Y));
//
//					//TODO
//
//					}

		            FACE_DBG("iFaceCount = %d\n", iFaceCount);
		            info->count = (iFaceCount > MAX_FACE_COUNT ? MAX_FACE_COUNT : iFaceCount);
		            for (int i = 0; i < info->count; i++) {
		                FACE_DBG("Face %d position is: [%d, %d, %d, %d]\n", i,
		                       pFaceArray[i].rect.left, pFaceArray[i].rect.top,
		                       pFaceArray[i].rect.width, pFaceArray[i].rect.height);

		                info->objects[i].id = pFaceArray[i].trackId;
		                info->objects[i].x = pFaceArray[i].rect.left;
		                info->objects[i].y = pFaceArray[i].rect.top;
		                info->objects[i].width = pFaceArray[i].rect.width;
		                info->objects[i].height = pFaceArray[i].rect.height;
						memcpy(info->objects[i].landmarks21, pFaceArray[i].landmarks21, sizeof(pFaceArray[i].landmarks21));
//		                info->blur_prob = output[i].blur_prob;
//		                info->front_prob = output[i].front_prob;
		            }
				} else {
			            info->count = 0;
		        }
				app_data->mLicense = mLicense;
				app_data->mFaceRecognition = mFaceRecognition;
				app_data->face_result = *info;
			}

//			display_the_window(video->disp_position, src_fd, src_w, src_h, src_fmt, vir_w, vir_h);

				
			if (pFaceArray) {
				releaseFaceTrackResult(pFaceArray, iFaceCount);
			}
        }
        return true;
    }
};

class TRANSPARENT : public StreamPUBase
{
    struct Video* video;

public:
    TRANSPARENT(struct Video* p) : StreamPUBase("TRANSPARENT", false, true) { video = p; }
    ~TRANSPARENT() {}
    bool processFrame(shared_ptr<BufferBase> inBuf,      shared_ptr<BufferBase> outBuf) {
        if (video->type == VIDEO_TYPE_ISP && parameter_get_debug_effect_watermark())
            show_video_mark(inBuf->getWidth(), inBuf->getHeight(),
                inBuf->getVirtAddr(), video->fps, inBuf->getMetaData(), NULL, NULL);
        return true;
    }
};

#endif
