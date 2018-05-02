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

#ifndef MOTION_DETECT_HANDLER_H
#define MOTION_DETECT_HANDLER_H

#include "scale_encode_handler.h"
#include "motion_detection/mv_dispatcher.h"
#include "motion_detection/md_processor.h"
#include "encoder_muxing/encoder/mpp_h264_encoder.h"

class MotionDetectHandler : public ScaleEncodeHandler<MPPH264Encoder> {
 public:
  // Make these user configurable ?
  static const int kMDWidth = 320;
  MotionDetectHandler();
  virtual ~MotionDetectHandler();
  int Init(MediaConfig config);
  virtual void DeInit() override;
  virtual void Process(int src_fd, const VideoConfig& src_config) override;
  int AddMdProcessor(VPUMDProcessor* processor, MDAttributes* attr);
  int RemoveMdProcessor(VPUMDProcessor* processor);
  inline bool IsRunning() { return pid_ != 0; }

 protected:
  virtual void GetSrcConfig(const MediaConfig& src_config,
                            int& src_w,
                            int& src_h,
                            PixelFormat& src_fmt) final;
  virtual int PrepareBuffers(MediaConfig& src_config,
                             const int dst_numerator,
                             const int dst_denominator,
                             int& dst_w,
                             int& dst_h) override;
  virtual void Work() final;

 private:
  MVDispatcher mv_dispatcher_;
  int md_processor_num_;
  struct video_ion mv_buf_;
  BufferData mv_data_;
};

#endif  // MOTION_DETECT_HANDLER_H
