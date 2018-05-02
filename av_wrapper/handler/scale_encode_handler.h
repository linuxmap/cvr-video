/**
 * Copyright (C) 2016 Fuzhou Rockchip Electronics Co., Ltd
 * author: hertz.wang hertz.wong@rock-chips.com
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

#ifndef SCALE_ENCODE_HANDLER_H
#define SCALE_ENCODE_HANDLER_H

#include <pthread.h>

#include <typeinfo>
#include <sys/prctl.h>

extern "C" {
#include "video_ion_alloc.h"
#include <rk_rga/rk_rga.h>
}

#include "video_common.h"
#include "encoder_muxing/encoder/buffer.h"
#include "encoder_muxing/encoder/base_encoder.h"

template <class Encoder>
class ScaleEncodeHandler;

template <class Encoder>
static void* ScaleEncodeLoop(void* user) {
  ScaleEncodeHandler<Encoder>* handler =
      static_cast<ScaleEncodeHandler<Encoder>*>(user);
  prctl(PR_SET_NAME, "ScaleEncodeLoop", 0, 0, 0);
  while (handler->run_) {
    std::unique_lock<std::mutex> _lk(handler->mtx_);
    if (!handler->run_)
      break;
    if (!handler->yuv_buf_valid_)
      handler->cond_.wait(_lk);
    if (handler->yuv_buf_valid_) {
      handler->yuv_buf_valid_ = false;
      handler->Work();
    }
  }
  return nullptr;
}

template <class Encoder>
class ScaleEncodeHandler {
 public:
  ScaleEncodeHandler()
      : encoder_(nullptr),
        run_(false),
        rga_fd_(-1),
        pid_(0),
        yuv_buf_valid_(false) {
    memset(&yuv_buf_, 0, sizeof(yuv_buf_));
    yuv_buf_.client = -1;
    yuv_buf_.fd = -1;
    memset(&dst_buf_, 0, sizeof(dst_buf_));
    dst_buf_.client = -1;
    dst_buf_.fd = -1;
  }
  virtual ~ScaleEncodeHandler() { DeInit(); }
  int Init(MediaConfig& src_config,
           const int dst_numerator,
           const int dst_denominator,
           int dst_w,
           int dst_h = 0) {
    int ret = PrepareBuffers(src_config, dst_numerator, dst_denominator, dst_w,
                             dst_h);
    if (ret)
      return -1;
    rga_fd_ = rk_rga_open();
    if (rga_fd_ < 0) {
      printf("open rga fd failed in %s\n", __func__);
      return -1;
    }
    encoder_ = new Encoder();
    if (!encoder_ || encoder_->InitConfig(src_config)) {
      printf("<%s> encoder create failed\n", typeid(Encoder).name());
      return -1;
    }
    if (StartThread())
      return -1;
    return 0;
  }
  virtual void DeInit() {
    StopThread();
    if (encoder_) {
      encoder_->unref();
      encoder_ = nullptr;
    }
    rk_rga_close(rga_fd_);
    rga_fd_ = -1;
    video_ion_free(&yuv_buf_);
    video_ion_free(&dst_buf_);
  }
  static int ConvertToRGAPixFmt(const PixelFormat& fmt) {
    static int rga_fmts[PIX_FMT_NB] =
        {[PIX_FMT_YUV420P] = RGA_FORMAT_YCBCR_420_P,
         [PIX_FMT_NV12] = RGA_FORMAT_YCBCR_420_SP,
         [PIX_FMT_NV21] = RGA_FORMAT_YCRCB_420_SP,
         [PIX_FMT_YUV422P] = RGA_FORMAT_YCBCR_422_P,
         [PIX_FMT_NV16] = RGA_FORMAT_YCBCR_422_SP,
         [PIX_FMT_NV61] = RGA_FORMAT_YCRCB_422_SP,
         [PIX_FMT_YVYU422] = -1,
         [PIX_FMT_UYVY422] = -1,
         [PIX_FMT_RGB565LE] = RGA_FORMAT_RGB_565,
         [PIX_FMT_BGR565LE] = -1,
         [PIX_FMT_RGB24] = RGA_FORMAT_RGB_888,
         [PIX_FMT_BGR24] = RGA_FORMAT_BGR_888,
         [PIX_FMT_RGB32] = RGA_FORMAT_RGBA_8888,
         [PIX_FMT_BGR32] = RGA_FORMAT_BGRA_8888};
    assert(fmt >= 0 && fmt < PIX_FMT_NB);
    return rga_fmts[fmt];
  }
  virtual void Process(int src_fd, const VideoConfig& src_config) {
    if (!encoder_)
      return;
    int src_rga_fmt = ConvertToRGAPixFmt(src_config.fmt);
    int src_w = src_config.width;
    int src_h = src_config.height;
    if (src_rga_fmt < 0) {
      printf("TODO: src fmt <%d> in %s\n", src_config.fmt, __func__);
      return;
    }
    assert(src_fd >= 0);
    int ret = rk_rga_ionfd_to_ionfd_scal(
        rga_fd_, src_fd, src_w, src_h, src_rga_fmt, yuv_buf_.fd, yuv_buf_.width,
        yuv_buf_.height, RGA_FORMAT_YCBCR_420_SP, 0, 0, yuv_buf_.width,
        yuv_buf_.height, src_w, src_h);
    if (!ret) {
      yuv_buf_valid_ = true;
      cond_.notify_one();
    }
  }

  friend void* ScaleEncodeLoop<Encoder>(void* user);

 protected:
  // yuv_buf_'format will be always PIX_FMT_NV12
  static const PixelFormat kCacheFmt = PIX_FMT_NV12;
  Encoder* encoder_;
  volatile bool run_;
  int rga_fd_;
  pthread_t pid_;
  struct video_ion yuv_buf_, dst_buf_;
  BufferData yuv_data_, dst_data_;
  bool yuv_buf_valid_;
  std::mutex mtx_;
  std::condition_variable cond_;
  virtual void GetSrcConfig(const MediaConfig& src_config,
                            int& src_w,
                            int& src_h,
                            PixelFormat& src_fmt) = 0;
  virtual int PrepareBuffers(MediaConfig& src_config,
                             const int dst_numerator,
                             const int dst_denominator,
                             int& dst_w,
                             int& dst_h) {
    int src_w = 0;
    int src_h = 0;
    PixelFormat src_fmt = kCacheFmt;
    GetSrcConfig(src_config, src_w, src_h, src_fmt);
    int width = dst_w;
    int height = dst_h;
    if (height == 0) {
      // Remain the pixel aspect ratio
      height = src_h * width / src_w;
      height = UPALIGNTO16(height);
      dst_h = height;
    }
    assert(height);
    printf("dst w,h : %d, %d\n", width, height);
    if (video_ion_alloc(&yuv_buf_, width, height) == -1) {
      printf("%s alloc yuv_buf_ err, no memory!\n", __func__);
      return -1;
    }
    yuv_data_.vir_addr_ = yuv_buf_.buffer;
    yuv_data_.ion_data_.fd_ = yuv_buf_.fd;
    yuv_data_.ion_data_.handle_ = yuv_buf_.handle;
    yuv_data_.mem_size_ = yuv_buf_.size;

    if (video_ion_alloc_rational(&dst_buf_, width, height, dst_numerator,
                                 dst_denominator) == -1) {
      printf("%s alloc dst_buf_ err, no memory!\n", __func__);
      return -1;
    }
    dst_data_.vir_addr_ = dst_buf_.buffer;
    dst_data_.ion_data_.fd_ = dst_buf_.fd;
    dst_data_.ion_data_.handle_ = dst_buf_.handle;
    dst_data_.mem_size_ = dst_buf_.size;
    return 0;
  }
  virtual void Work() = 0;

 private:
  int StartThread() {
    if (pid_)
      return -1;
    int ret = ::StartThread(pid_, ScaleEncodeLoop<Encoder>, this);
    run_ = !ret;
    return ret;
  }
  void StopThread() {
    if (pid_) {
      {
        std::unique_lock<std::mutex> _lk(mtx_);
        run_ = false;
        cond_.notify_one();
      }
      ::StopThread(pid_);
    }
  }
};

#endif  // SCALE_ENCODE_HANDLER_H
