/**
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd
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

#ifndef CAMERA_FILE_MUXER_H
#define CAMERA_FILE_MUXER_H

#include "../camera_muxer.h"

// For file storage.
class CameraFileMuxer : public CameraMuxer {
 private:
  CameraFileMuxer();

#define MODE_NORMAL (0 << 0)
#define MODE_ONLY_KEY_FRAME (1 << 0)
#define MODE_NORMAL_FIX_TIME (1 << 1)
#define MODE_NORMAL_RELATIVE (1 << 2)

  uint32_t low_frame_rate_mode;

  struct timeval lbr_pre_timeval;
  struct timeval real_pre_timeval;  // actual time

 public:
  ~CameraFileMuxer();
  bool is_low_frame_rate();
  bool set_low_frame_rate(uint32_t val, const struct timeval& start_timeval);
  void reset_lbr_time();
  void set_real_time(const struct timeval& start_timeval);
  void push_packet(EncodedPacket* pkt);
  CREATE_FUNC(CameraFileMuxer)
  bool init_subtitle_stream(AVStream *stream);
};

#endif  // CAMERA_FILE_MUXER_H
