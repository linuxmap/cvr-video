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

#include "custom_muxer.h"
#include "encoder_muxing/encoder/ffmpeg_wrapper/ff_context.h"

CustomMuxer::CustomMuxer() {
#ifdef DEBUG
  snprintf(class_name, sizeof(class_name), "custom_muxer");
#endif
  exit_id = MUXER_NORMAL_EXIT;
  no_async = true;  // no cache packet list and no thread handle
  data_cb = NULL;
}

int CustomMuxer::init_muxer_processor(MuxProcessor* process) {
  UNUSED(process);
  return 0;
}

void CustomMuxer::deinit_muxer_processor(AVFormatContext* oc) {
  UNUSED(oc);
}

int CustomMuxer::muxer_write_header(AVFormatContext* oc, char* url) {
  UNUSED(oc);
  UNUSED(url);
  return 0;
}

int CustomMuxer::muxer_write_tailer(AVFormatContext* oc) {
  UNUSED(oc);
  return 0;
}

int CustomMuxer::muxer_write_free_packet(MuxProcessor* process,
                                         EncodedPacket* pkt) {
  if (data_cb && pkt->type == VIDEO_PACKET) {
    BaseEncoder * venc = get_video_encoder();
    assert(venc);
    FFContext* ff_ctx = static_cast<FFContext*>(venc->GetHelpContext());
    AVCodecContext* codec_ctx = ff_ctx->GetAVStream()->codec;
    assert(codec_ctx);
    AVPacket* avpkt = &pkt->av_pkt;
    VEncStreamInfo info = {
        .frm_type = (avpkt->flags & AV_PKT_FLAG_KEY) ? I_FRAME : P_FRAME,
        .buf_addr = avpkt->data,
        .buf_size = avpkt->size,
        .time_val = pkt->time_val,
        .ExtraInfo = {
            .sps_pps_info = {.data = codec_ctx->extradata,
                             .data_size = codec_ctx->extradata_size}}};
    data_cb(&info);
  }
  pkt->unref();
  return 0;
}
