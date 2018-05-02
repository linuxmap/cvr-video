/**
 * Copyright (C) 2016 Fuzhou Rockchip Electronics Co., Ltd
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

#ifndef CVR_AV_WRAPPER_CUSTOM_MUXER_H__
#define CVR_AV_WRAPPER_CUSTOM_MUXER_H__

#include "../../../video_interface.h"
#include "camera_muxer.h"

class CustomMuxer : public CameraMuxer {
 public:
  inline void set_notify_new_enc_stream_callback(NotifyNewEncStream cb) {
    data_cb = cb;
  }
  int init_muxer_processor(MuxProcessor* process);
  void deinit_muxer_processor(AVFormatContext* oc);
  int muxer_write_header(AVFormatContext* oc, char* url);
  int muxer_write_tailer(AVFormatContext* oc);
  int muxer_write_free_packet(MuxProcessor* process, EncodedPacket* pkt);

  CREATE_FUNC(CustomMuxer)

 private:
  CustomMuxer();
  NotifyNewEncStream data_cb;
};

#endif  // CVR_AV_WRAPPER_CUSTOM_MUXER_H__
