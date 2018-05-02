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

#include "ff_mpp_h264_encoder.h"

extern "C" {
#include "watermark.h"
#include "common.h"
}

#include "video_common.h"
#include "encoder_muxing/encoder/buffer.h"

FFMPPH264Encoder::FFMPPH264Encoder()
    : FFBaseEncoder(codec_name_), video_frame_(nullptr) {
  snprintf(codec_name_, sizeof(codec_name_), "h264_mpp");
}

FFMPPH264Encoder::~FFMPPH264Encoder() {
  av_frame_free(&video_frame_);
}

int FFMPPH264Encoder::InitConfig(MediaConfig& config) {
  assert(!video_frame_);
  AVFrame* frame = av_frame_alloc();
  if (!frame) {
    printf("Could not alloc video frame\n");
    return -1;
  }
  // We don't alloc the data buffer in av framework,
  // as we will use the buffer passed from outside.
  video_frame_ = frame;
  if (FFBaseEncoder::InitConfig(config.video_config))
    return -1;

  AVCodecContext* c = GetAVCodecContext();
  video_frame_->format = c->pix_fmt;
  video_frame_->width = c->width;
  video_frame_->height = c->height;

#ifdef USE_WATERMARK
  MppEncOSDPlt palette;
  memcpy((uint8_t*)palette.buf, (uint8_t*)yuv444_palette_table,
         PALETTE_TABLE_LEN * 4);

  int ret = EncodeControl(MPP_ENC_SET_OSD_PLT_CFG, &palette);
  if (ret != 0) {
    printf("encode_control error, cmd 0x%08x", MPP_ENC_SET_OSD_PLT_CFG);
    return -1;
  }
#endif
  if (BaseVideoEncoder::InitConfig(config))
    return -1;
  return 0;
}

int FFMPPH264Encoder::EncodeOneFrame(Buffer* src, Buffer* dst, Buffer* mv) {
  assert(src);
  assert(dst);
  FFMppShareBuffer src_buf = {(uint8_t*)src->GetVirAddr(), src->GetFd(),
                              (void*)src->GetHandle(), src->Size()};
  FFMppShareBuffer dst_buf = {(uint8_t*)dst->GetVirAddr(), dst->GetFd(),
                              (void*)dst->GetHandle(), dst->Size()};
  uint32_t dst_flag = 0;
  FFMppShareBuffer mv_buf;
  PFFMppShareBuffer p_mv_buf = nullptr;
  if (mv) {
    mv_buf.vir_addr = (uint8_t*)mv->GetVirAddr();
    mv_buf.phy_fd = mv->GetFd();
    mv_buf.handle = (void*)mv->GetHandle();
    mv_buf.buf_size = mv->Size();
    p_mv_buf = &mv_buf;
  }
  int out_pkt_size = EncodeOneFrame(&src_buf, &dst_buf, dst_flag, p_mv_buf);
  dst->SetValidDataSize(out_pkt_size);
  dst->SetUserFlag(dst_flag);
  if (out_pkt_size > 0) {
    if (mv)
      mv->SetValidDataSize(mv->Size());
    return 0;
  }
  return -1;
}

int FFMPPH264Encoder::EncodeOneFrame(PFFMppShareBuffer src_buf,
                                     PFFMppShareBuffer dst_buf,
                                     uint32_t& dst_flag,
                                     PFFMppShareBuffer mv_buf) {
  AVCodecContext* c = GetAVCodecContext();
  AVFrame* frame = video_frame_;
  int got_packet = 0;
  int out_pkt_size = -1;
  PFFMppShareBuffer pdata = nullptr;

  // RK hardware encoder need the buffer w/h align to 16.
  int w_align = UPALIGNTO16(frame->width);
  int h_align = UPALIGNTO16(frame->height);
  frame->data[0] = src_buf->vir_addr;
  frame->linesize[0] = w_align;
  frame->data[1] = frame->data[0] + h_align * frame->linesize[0];
  frame->linesize[1] = (w_align >> 1);
  frame->data[2] = frame->data[1] + (h_align >> 1) * frame->linesize[1];
  frame->linesize[2] = (w_align >> 1);
  assert(sizeof(*src_buf) <= sizeof(frame->user_reserve_buf));
  pdata = reinterpret_cast<PFFMppShareBuffer>(frame->user_reserve_buf);
  *pdata = *src_buf;

  AVPacket pkt;
  av_init_packet(&pkt);
  pkt.data = dst_buf->vir_addr;  // must set
  pkt.size = dst_buf->buf_size;
  memset(pkt.user_reserve_buf, 0, sizeof(pkt.user_reserve_buf));
  assert(2 * sizeof(*dst_buf) <= sizeof(pkt.user_reserve_buf));
  pdata = reinterpret_cast<PFFMppShareBuffer>(pkt.user_reserve_buf);
  *pdata = *dst_buf;
  if (mv_buf)
    *(pdata + 1) = *mv_buf;

  if (DyncConfigTestAndSet(kForceIdrFrame) &&
      EncodeControl(MPP_ENC_SET_IDR_FRAME, nullptr) != 0) {
    printf("encode_control MPP_ENC_SET_IDR_FRAME error!\n");
    return -1;
  }

  out_pkt_size = 0;
  // Encode the image.
  int ret = avcodec_encode_video2(c, &pkt, frame, &got_packet);
  if (ret < 0) {
    char str[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_strerror(ret, str, AV_ERROR_MAX_STRING_SIZE);
    printf("Error encoding video frame: %s\n", str);
    out_pkt_size = -1;
    goto out;
  }
  if (got_packet && pkt.data) {
    out_pkt_size = pkt.size;
    assert(pkt.data == dst_buf->vir_addr);
#ifndef MPP_PACKET_FLAG_INTRA
#define MPP_PACKET_FLAG_INTRA (0x00000008)
#endif
    if (pkt.flags & AV_PKT_FLAG_KEY)
      dst_flag = MPP_PACKET_FLAG_INTRA;
  }

out:
  frame->data[0] = nullptr;  // free by outsides
  av_packet_unref(&pkt);
  return out_pkt_size;
}

int FFMPPH264Encoder::EncodeControl(int cmd, void* param) {
  AVCodecContext* avctx = GetAVCodecContext();
  int ret = avctx->codec->control(avctx, cmd, param);
  if (ret != 0)
    printf("encode_control, avctx %p cmd 0x%08x param %p failed\n", avctx, cmd,
           param);
  return ret;
}

void* FFMPPH264Encoder::GetHelpContext() {
  return static_cast<void*>(FFBaseEncoder::GetHelpContext());
}
