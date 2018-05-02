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

#include "motion_detect_handler.h"

MotionDetectHandler::MotionDetectHandler() : md_processor_num_(0) {
  memset(&mv_buf_, 0, sizeof(mv_buf_));
  mv_buf_.client = -1;
  mv_buf_.fd = -1;
}

MotionDetectHandler::~MotionDetectHandler() {
  assert(md_processor_num_ == 0);
  DeInit();
}

int MotionDetectHandler::Init(MediaConfig config) {
  return ScaleEncodeHandler<MPPH264Encoder>::Init(config, 1, 2, kMDWidth);
}

void MotionDetectHandler::DeInit() {
  ScaleEncodeHandler<MPPH264Encoder>::DeInit();
  video_ion_free(&mv_buf_);
}

void MotionDetectHandler::GetSrcConfig(const MediaConfig& src_config,
                                       int& src_w,
                                       int& src_h,
                                       PixelFormat& src_fmt) {
  src_w = src_config.video_config.width;
  src_h = src_config.video_config.height;
  src_fmt = src_config.video_config.fmt;
}

int MotionDetectHandler::PrepareBuffers(MediaConfig& src_config,
                                        const int dst_numerator,
                                        const int dst_denominator,
                                        int& dst_w,
                                        int& dst_h) {
  int ret = ScaleEncodeHandler<MPPH264Encoder>::PrepareBuffers(
      src_config, dst_numerator, dst_denominator, dst_w, dst_h);
  if (ret)
    return ret;
  VideoConfig& vconf = src_config.video_config;
  vconf.fmt = kCacheFmt;
  vconf.width = dst_w;
  vconf.height = dst_h;
  vconf.bit_rate = 512000;
  vconf.quality = 0;
  vconf.qp_init = 18;
  vconf.qp_step = 0;
  vconf.qp_min = vconf.qp_init;
  vconf.qp_max = vconf.qp_init;
  vconf.rc_mode = MPP_ENC_RC_MODE_CBR;
  // 32 bits per MB. picture width should be aligned up to 256 pixels(16MBs).
  if (video_ion_alloc_rational(&mv_buf_, (dst_w + 255) & (~255), dst_h, 4,
                               256) == -1) {
    PRINTF_NO_MEMORY;
    return -1;
  }
  mv_data_.vir_addr_ = mv_buf_.buffer;
  mv_data_.ion_data_.fd_ = mv_buf_.fd;
  mv_data_.ion_data_.handle_ = mv_buf_.handle;
  mv_data_.mem_size_ = mv_buf_.size;
  return 0;
}
void MotionDetectHandler::MotionDetectHandler::Work() {
  Buffer src(yuv_data_);
  Buffer dst(dst_data_);
  Buffer mv(mv_data_);
  int ret = encoder_->EncodeOneFrame(&src, &dst, &mv);
  if (!ret && !(mv.GetUserFlag() & MPP_PACKET_FLAG_INTRA))
    mv_dispatcher_.Dispatch(mv.GetVirAddr(), src.GetVirAddr(), src.Size());
}

void MotionDetectHandler::Process(int src_fd, const VideoConfig &src_config) {
  if (md_processor_num_ <= 0)
    return;
  if (!mtx_.try_lock()) {
    printf("MD: try lock yuv_buf_ get false.\n");
    return;
  } else {
    ScaleEncodeHandler<MPPH264Encoder>::Process(src_fd, src_config);
    mtx_.unlock();
  }
}

int MotionDetectHandler::AddMdProcessor(VPUMDProcessor* processor,
                                        MDAttributes* attr) {
  if (!processor || !encoder_)
    return -1;

  VideoConfig& config = encoder_->GetVideoConfig();
  processor->init(config.width, config.height, config.frame_rate);
  processor->set_video_encoder(encoder_);
  if (!processor->set_attributes(attr)) {
    mv_dispatcher_.AddHandler(processor);
    md_processor_num_++;
    assert(md_processor_num_ >= 0);
    return 0;
  }
  assert(md_processor_num_ >= 0);
  return -1;
}

int MotionDetectHandler::RemoveMdProcessor(VPUMDProcessor* processor) {
  if (!processor || !encoder_)
    return -1;

  bool real_remove = false;
  mv_dispatcher_.RemoveHandler(processor, real_remove);
  if (real_remove)
    md_processor_num_--;
  processor->release_attributes();
  assert(md_processor_num_ >= 0);
  return 0;
}
