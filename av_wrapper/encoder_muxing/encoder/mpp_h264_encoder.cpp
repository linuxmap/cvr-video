/**
 * Copyright (C) 2016 Fuzhou Rockchip Electronics Co., Ltd
 * author: zhaojun jung.zhao@rock-chips.com
 *         herman.chen herman.chen@rock-chips.com
 *         hertz.wang hertz.wong@rock-chips.com
 *         linkesheng lks@rock-chips.com
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

#include "mpp_h264_encoder.h"

#include <unistd.h>

extern "C" {
#include "common.h"
#include "watermark.h"

#include <cvr_ffmpeg_shared.h>
}

#include <mpp/mpp_log.h>
#include <mpp/rk_mpi.h>

#include "buffer.h"

MPPH264Encoder::MPPH264Encoder() : extra_data_(nullptr), extra_data_size_(0) {
  memset(&mpp_enc_ctx_, 0, sizeof(mpp_enc_ctx_));
  mpp_enc_ctx_.video_type = MPP_VIDEO_CodingAVC;
}

MPPH264Encoder::~MPPH264Encoder() {
  if (mpp_enc_ctx_.ctx)
    mpp_destroy(mpp_enc_ctx_.ctx);
  if (extra_data_)
    free(extra_data_);
}

#define MPP_MIN_BPS (2 * 1000)
#define MPP_MAX_BPS (98 * 1000 * 1000)

static int CalcMppBps(MppEncRcCfg* rc_cfg, int bps) {
  if (bps > MPP_MAX_BPS) {
    fprintf(stderr, "bps <%d> too large\n", bps);
    return -1;
  }

  switch (rc_cfg->rc_mode) {
    case MPP_ENC_RC_MODE_CBR:
      // constant bitrate has very small bps range of 1/16 bps
      rc_cfg->bps_target = bps;
      rc_cfg->bps_max = bps * 17 / 16;
      rc_cfg->bps_min = bps * 15 / 16;
      break;
    case MPP_ENC_RC_MODE_VBR:
      // variable bitrate has large bps range
      rc_cfg->bps_target = bps;
      rc_cfg->bps_max = bps * 3 / 2;
      rc_cfg->bps_min = bps * 1 / 2;
      break;
    default:
      // TODO
      printf("right now rc_mode=%d is untested\n", rc_cfg->rc_mode);
      return -1;
  }

  if (rc_cfg->bps_max > MPP_MAX_BPS)
    rc_cfg->bps_max = MPP_MAX_BPS;
  if (rc_cfg->bps_min < MPP_MIN_BPS)
    rc_cfg->bps_min = MPP_MIN_BPS;

  return 0;
}

int MPPH264Encoder::InitConfig(MediaConfig& config) {
  VideoConfig& vconfig = config.video_config;
  MppFrameFormat pic_type;
  MpiCmd mpi_cmd = MPP_CMD_BASE;
  MppParam param = NULL;
  MppEncCfgSet cfg;
  MppEncRcCfg* rc_cfg = &cfg.rc;
  MppEncPrepCfg* prep_cfg = &cfg.prep;
  MppEncCodecCfg* codec_cfg = &cfg.codec;
  unsigned int need_block = 1;
  int dummy = 1;
  int profile = 100;
  int fps = vconfig.frame_rate;
  int bps = vconfig.bit_rate;
  int gop = vconfig.gop_size;

  if (fps < 1)
    fps = 1;
  else if (fps > ((1 << 16) - 1))
    fps = (1 << 16) - 1;

  int ret = mpp_create(&mpp_enc_ctx_.ctx, &mpp_enc_ctx_.mpi);
  if (ret) {
    fprintf(stderr, "mpp_create failed");
    return -1;
  }
  mpi_cmd = MPP_SET_INPUT_BLOCK;
  param = &need_block;
  ret = mpp_enc_ctx_.mpi->control(mpp_enc_ctx_.ctx, mpi_cmd, param);
  if (ret != 0) {
    fprintf(stderr, "mpp control %d failed", mpi_cmd);
    return -1;
  }

  ret = mpp_init(mpp_enc_ctx_.ctx, MPP_CTX_ENC, mpp_enc_ctx_.video_type);
  if (ret) {
    fprintf(stderr, "mpp_init failed");
    return -1;
  }

  pic_type = ConvertToMppPixFmt(vconfig.fmt);
  if (pic_type == -1) {
    fprintf(stderr, "error input pixel format!");
    return -1;
  }
  // NOTE: in mpp, chroma_addr = luma_addr + hor_stride*ver_stride,
  // so if there is no gap between luma and chroma in yuv,
  // hor_stride MUST be set to be width, ver_stride MUST be set to height.
  prep_cfg->change =
      MPP_ENC_PREP_CFG_CHANGE_INPUT | MPP_ENC_PREP_CFG_CHANGE_FORMAT;
  prep_cfg->width = vconfig.width;
  prep_cfg->height = vconfig.height;
  prep_cfg->hor_stride = vconfig.width;
  prep_cfg->ver_stride = vconfig.height;
  prep_cfg->format = pic_type;

  mpp_log_f("encode width %d height %d\n", prep_cfg->width, prep_cfg->height);
  ret = mpp_enc_ctx_.mpi->control(mpp_enc_ctx_.ctx, MPP_ENC_SET_PREP_CFG,
                                  prep_cfg);
  if (ret) {
    fprintf(stderr, "mpi control enc set prep cfg failed");
    return -1;
  }

  param = &dummy;
  ret = mpp_enc_ctx_.mpi->control(mpp_enc_ctx_.ctx, MPP_ENC_PRE_ALLOC_BUFF,
                                  param);
  if (ret) {
    fprintf(stderr, "mpi control pre alloc buff failed");
    return -1;
  }

  rc_cfg->change = MPP_ENC_RC_CFG_CHANGE_ALL;

  rc_cfg->rc_mode = static_cast<MppEncRcMode>(vconfig.rc_mode);
  // vconfig.rc_mode == 0 ? MPP_ENC_RC_MODE_VBR : MPP_ENC_RC_MODE_CBR;
  rc_cfg->quality = static_cast<MppEncRcQuality>(vconfig.quality);

  CalcMppBps(rc_cfg, bps);

  // fix input / output frame rate
  rc_cfg->fps_in_flex = 0;
  rc_cfg->fps_in_num = fps;
  rc_cfg->fps_in_denorm = 1;
  rc_cfg->fps_out_flex = 0;
  rc_cfg->fps_out_num = fps;
  rc_cfg->fps_out_denorm = 1;

  rc_cfg->gop = gop;
  rc_cfg->skip_cnt = 0;

  mpp_log_f("encode bps %d fps %d gop %d\n", bps, fps, gop);
  ret = mpp_enc_ctx_.mpi->control(mpp_enc_ctx_.ctx, MPP_ENC_SET_RC_CFG, rc_cfg);
  if (ret) {
    fprintf(stderr, "mpi control enc set rc cfg failed");
    return -1;
  }

  profile = vconfig.profile;
  if (profile != 66 && profile != 77)
    profile = 100;  // default PROFILE_HIGH 100

  codec_cfg->coding = MPP_VIDEO_CodingAVC;
  codec_cfg->h264.change =
      MPP_ENC_H264_CFG_CHANGE_PROFILE | MPP_ENC_H264_CFG_CHANGE_ENTROPY;
  codec_cfg->h264.profile = profile;
  codec_cfg->h264.level = vconfig.level;
  codec_cfg->h264.entropy_coding_mode =
      (profile == 66 || profile == 77) ? (0) : (1);
  codec_cfg->h264.cabac_init_idc = 0;

  // setup QP on CQP mode
  codec_cfg->h264.change |= MPP_ENC_H264_CFG_CHANGE_QP_LIMIT;
  if (rc_cfg->rc_mode == MPP_ENC_RC_MODE_CBR) {
    codec_cfg->h264.qp_init = vconfig.qp_init;
    codec_cfg->h264.qp_min = vconfig.qp_min;
    codec_cfg->h264.qp_max = vconfig.qp_max;
    codec_cfg->h264.qp_max_step = vconfig.qp_step;
  }

  mpp_log_f("encode profile %d level %d init_qp %d\n", profile,
            codec_cfg->h264.level, vconfig.qp_init);
  ret = mpp_enc_ctx_.mpi->control(mpp_enc_ctx_.ctx, MPP_ENC_SET_CODEC_CFG,
                                  codec_cfg);
  if (ret) {
    fprintf(stderr, "mpi control enc set codec cfg failed");
    return -1;
  }

  if (bps >= 50000000) {
    RK_U32 qp_scale = 2;  // 1 or 2
    ret = mpp_enc_ctx_.mpi->control(mpp_enc_ctx_.ctx, MPP_ENC_SET_QP_RANGE,
                                    &qp_scale);
    if (ret) {
      mpp_err("mpi control enc set qp scale failed ret %d\n", ret);
      return -1;
    }
    mpp_log_f("qp_scale:%d\n", qp_scale);
  }

  ret = mpp_enc_ctx_.mpi->control(mpp_enc_ctx_.ctx, MPP_ENC_GET_EXTRA_INFO,
                                  &mpp_enc_ctx_.packet);
  if (ret) {
    fprintf(stderr, "mpi control enc get extra info failed\n");
    return -1;
  }

  // Get and write sps/pps for H.264
  if (mpp_enc_ctx_.packet) {
    void* ptr = mpp_packet_get_pos(mpp_enc_ctx_.packet);
    size_t len = mpp_packet_get_length(mpp_enc_ctx_.packet);
    extra_data_ = malloc(len);
    if (!extra_data_) {
      fprintf(stderr, "extradata malloc failed");
      return -1;
    }
    extra_data_size_ = len;
    memcpy(extra_data_, ptr, len);
    mpp_enc_ctx_.packet = NULL;
  }

#ifdef USE_WATERMARK
  MppEncOSDPlt palette;
  memcpy((uint8_t*)palette.buf, (uint8_t*)yuv444_palette_table,
         PALETTE_TABLE_LEN * 4);

  ret = EncodeControl(MPP_ENC_SET_OSD_PLT_CFG, &palette);
  if (ret != 0) {
    printf("encode_control error, cmd 0x%08x", MPP_ENC_SET_OSD_PLT_CFG);
    return -1;
  }
#endif

  return BaseVideoEncoder::InitConfig(config);
}

int MPPH264Encoder::CheckConfigChange() {
  VideoConfig& vconfig = GetVideoConfig();
  MppEncRcCfg rc_cfg;

  if (DyncConfigTestAndSet(kFrameRateChange)) {
    assert(new_frame_rate_ > 0 && new_frame_rate_ < 120);
    rc_cfg.change = MPP_ENC_RC_CFG_CHANGE_FPS_OUT;
    rc_cfg.fps_out_flex = 0;
    rc_cfg.fps_out_num = new_frame_rate_;
    rc_cfg.fps_out_denorm = 1;
    if (EncodeControl(MPP_ENC_SET_RC_CFG, &rc_cfg) != 0) {
      printf(
          "encode_control MPP_ENC_SET_RC_CFG(MPP_ENC_RC_CFG_CHANGE_FPS_OUT) "
          "error!\n");
      return -1;
    }
    vconfig.frame_rate = new_frame_rate_;
  }

  if (DyncConfigTestAndSet(kBitRateChange)) {
    assert(new_bit_rate_ > 0 && new_bit_rate_ < 60 * 1000 * 1000);
    rc_cfg.change = MPP_ENC_RC_CFG_CHANGE_BPS;
    rc_cfg.rc_mode = static_cast<MppEncRcMode>(vconfig.rc_mode);
    CalcMppBps(&rc_cfg, new_bit_rate_);
    if (EncodeControl(MPP_ENC_SET_RC_CFG, &rc_cfg) != 0) {
      printf(
          "encode_control MPP_ENC_SET_RC_CFG(MPP_ENC_RC_CFG_CHANGE_BPS) "
          "error!\n");
      return -1;
    }
    vconfig.bit_rate = new_bit_rate_;
  }

  if (DyncConfigTestAndSet(kForceIdrFrame) &&
      EncodeControl(MPP_ENC_SET_IDR_FRAME, nullptr) != 0) {
    printf("encode_control MPP_ENC_SET_IDR_FRAME error!\n");
    return -1;
  }

  return 0;
}

static void get_expect_buf_size(VideoConfig& vconfig, size_t* size) {
  int w_align = vconfig.width;
  int h_align = vconfig.height;

  switch (vconfig.fmt) {
    case PIX_FMT_YUV420P:
    case PIX_FMT_NV12:
    case PIX_FMT_NV21:
      *size = w_align * h_align * 3 / 2;
      break;
    case PIX_FMT_YUV422P:
    case PIX_FMT_NV16:
    case PIX_FMT_NV61:
    case PIX_FMT_YVYU422:
    case PIX_FMT_UYVY422:
      *size = w_align * h_align * 2;
      break;
    case PIX_FMT_RGB565LE:
    case PIX_FMT_BGR565LE:
      *size = w_align * h_align * 2;
      break;
    case PIX_FMT_RGB24:
    case PIX_FMT_BGR24:
      *size = w_align * h_align * 3;
      break;
    case PIX_FMT_RGB32:
    case PIX_FMT_BGR32:
      *size = w_align * h_align * 4;
      break;
    default:
      fprintf(stderr, "unsupport compress format %d\n", vconfig.fmt);
      *size = 0;
  }
}

int MPPH264Encoder::EncodeOneFrame(Buffer* src, Buffer* dst, Buffer* mv) {
  int ret = 0;
  VideoConfig& vconfig = GetVideoConfig();
  RkMppEncContext* rkenc_ctx = &mpp_enc_ctx_;
  MppBuffer pic_buf = NULL;
  MppBuffer str_buf = NULL;
  MppBuffer mv_buf = NULL;
  MppTask task = NULL;
  MppBufferInfo info;

  if (!src)
    return 0;

  if (CheckConfigChange())
    return -1;

  mpp_frame_init(&rkenc_ctx->frame);
  mpp_frame_set_width(rkenc_ctx->frame, vconfig.width);
  mpp_frame_set_height(rkenc_ctx->frame, vconfig.height);
  mpp_frame_set_hor_stride(rkenc_ctx->frame, UPALIGNTO16(vconfig.width));
  mpp_frame_set_ver_stride(rkenc_ctx->frame, UPALIGNTO16(vconfig.height));

  // Import frame_data
  memset(&info, 0, sizeof(info));
  info.type = MPP_BUFFER_TYPE_ION;
  get_expect_buf_size(vconfig, &info.size);
  info.fd = src->GetFd();
  info.ptr = src->GetVirAddr();
  if (!info.size || info.size > src->Size()) {
    fprintf(stderr, "input src data size wrong, info.size: %u, buf_size: %u\n",
            info.size, src->Size());
    ret = -1;
    goto ENCODE_FREE_FRAME;
  }
  ret = mpp_buffer_import(&pic_buf, &info);
  if (ret) {
    fprintf(stderr, "pic_buf import buffer failed\n");
    goto ENCODE_FREE_FRAME;
  }
  mpp_frame_set_buffer(rkenc_ctx->frame, pic_buf);

  // Import pkt_data
  memset(&info, 0, sizeof(info));
  info.type = MPP_BUFFER_TYPE_ION;
  if (dst) {
    info.size = dst->Size();
    info.fd = dst->GetFd();
    info.ptr = dst->GetVirAddr();
  }
  if (info.size > 0) {
    ret = mpp_buffer_import(&str_buf, &info);
    if (ret) {
      fprintf(stderr, "str_buf import buffer failed\n");
      goto ENCODE_FREE_PACKET;
    }
    mpp_packet_init_with_buffer(&rkenc_ctx->packet, str_buf);
  }

  // Import mv_data
  memset(&info, 0, sizeof(info));
  info.type = MPP_BUFFER_TYPE_ION;
  if (mv) {
    info.size = mv->Size();
    info.fd = mv->GetFd();
    info.ptr = mv->GetVirAddr();
  }
  if (info.size > 0) {
    ret = mpp_buffer_import(&mv_buf, &info);
    if (ret) {
      fprintf(stderr, "mv_buf import buffer failed\n");
      goto ENCODE_FREE_MV;
    }
  }

  // Encode process
  // poll and dequeue empty task from mpp input port
  ret = rkenc_ctx->mpi->poll(rkenc_ctx->ctx, MPP_PORT_INPUT, MPP_POLL_BLOCK);
  if (ret) {
    mpp_err_f("input poll ret %d\n", ret);
    goto ENCODE_FREE_PACKET;
  }

  ret = rkenc_ctx->mpi->dequeue(rkenc_ctx->ctx, MPP_PORT_INPUT, &task);
  if (ret || NULL == task) {
    fprintf(stderr, "mpp task input dequeue failed\n");
    goto ENCODE_FREE_PACKET;
  }

  mpp_task_meta_set_frame(task, KEY_INPUT_FRAME, rkenc_ctx->frame);
  mpp_task_meta_set_packet(task, KEY_OUTPUT_PACKET, rkenc_ctx->packet);
  mpp_task_meta_set_buffer(task, KEY_MOTION_INFO, mv_buf);
  ret = rkenc_ctx->mpi->enqueue(rkenc_ctx->ctx, MPP_PORT_INPUT, task);
  if (ret) {
    fprintf(stderr, "mpp task input enqueue failed\n");
    goto ENCODE_FREE_PACKET;
  }
  task = NULL;

  // poll and task to mpp output port
  ret = rkenc_ctx->mpi->poll(rkenc_ctx->ctx, MPP_PORT_OUTPUT, MPP_POLL_BLOCK);
  if (ret) {
    mpp_err_f("output poll ret %d\n", ret);
    goto ENCODE_FREE_PACKET;
  }

  ret = rkenc_ctx->mpi->dequeue(rkenc_ctx->ctx, MPP_PORT_OUTPUT, &task);
  if (ret || !task) {
    fprintf(stderr, "ret %d mpp task output dequeue failed\n", ret);
    goto ENCODE_FREE_PACKET;
  }

  if (task) {
    MppPacket packet_out = NULL;

    mpp_task_meta_get_packet(task, KEY_OUTPUT_PACKET, &packet_out);
    ret = rkenc_ctx->mpi->enqueue(rkenc_ctx->ctx, MPP_PORT_OUTPUT, task);
    if (ret) {
      fprintf(stderr, "mpp task output enqueue failed\n");
      goto ENCODE_FREE_PACKET;
    }
    task = NULL;

    if (rkenc_ctx->packet != NULL) {
      assert(rkenc_ctx->packet == packet_out);
      dst->SetValidDataSize(mpp_packet_get_length(rkenc_ctx->packet));
      dst->SetUserFlag(mpp_packet_get_flag(rkenc_ctx->packet));
      // printf("~~~ got h264 data len: %d kB\n",
      //   mpp_packet_get_length(rkenc_ctx->packet) / 1024);
    }
    if (mv_buf) {
      assert(packet_out);
      mv->SetUserFlag(mpp_packet_get_flag(packet_out));
      mv->SetValidDataSize(mv->Size());
    }
    if (packet_out && packet_out != rkenc_ctx->packet)
      mpp_packet_deinit(&packet_out);
  }

ENCODE_FREE_PACKET:
  if (pic_buf)
    mpp_buffer_put(pic_buf);

  if (str_buf)
    mpp_buffer_put(str_buf);

  mpp_packet_deinit(&rkenc_ctx->packet);
  rkenc_ctx->packet = NULL;

ENCODE_FREE_FRAME:
  mpp_frame_deinit(&rkenc_ctx->frame);
  rkenc_ctx->frame = NULL;

ENCODE_FREE_MV:
  if (mv_buf)
    mpp_buffer_put(mv_buf);

  if (rkenc_ctx->osd_data) {
    mpp_buffer_put(rkenc_ctx->osd_data);
    rkenc_ctx->osd_data = NULL;
  }

  return ret;
}

int MPPH264Encoder::EncodeControl(int cmd, void* param) {
  int ret = 0;
  RkMppEncContext* rkenc_ctx = &mpp_enc_ctx_;
  MppApi* mpi = rkenc_ctx->mpi;
  MpiCmd mpi_cmd = (MpiCmd)cmd;

  if (mpi_cmd == MPP_ENC_SET_OSD_DATA_CFG) {
    MppEncOSDData* src = static_cast<MppEncOSDData*>(param);
    MppEncOSDData dst;
    MppBufferInfo info;
    DataBuffer_t* src_buf = static_cast<DataBuffer_t*>(src->buf);
    MppBuffer* dst_buf = &rkenc_ctx->osd_data;

    memset(&info, 0, sizeof(info));
    info.type = MPP_BUFFER_TYPE_ION;
    info.size = src_buf->buf_size;
    info.fd = src_buf->phy_fd;
    info.ptr = src_buf->vir_addr;

    if (info.size == 0) {
      fprintf(stderr, "rkenc_control osd data buf size is zero\n");
      return 1;
    }
    if (*dst_buf) {
      fprintf(stderr,
              "rkenc_control osd data buf should be free after last frame "
              "encode.\n");
      return 1;
    }

    ret = mpp_buffer_import(dst_buf, &info);
    if (ret) {
      fprintf(stderr, "rkenc_control osd_data buf import buffer failed\n");
      if (*dst_buf) {
        mpp_buffer_put(*dst_buf);
        *dst_buf = NULL;
      }
      return ret;
    }

    memcpy(&dst, src, sizeof(dst));
    dst.buf = *dst_buf;

    ret = mpi->control(rkenc_ctx->ctx, mpi_cmd, (MppParam)(&dst));
  } else {
    ret = mpi->control(rkenc_ctx->ctx, mpi_cmd, (MppParam)param);
  }

  if (ret != 0) {
    fprintf(stderr, "mpp control ctx %p cmd 0x%08x param %p failed\n",
            rkenc_ctx->ctx, cmd, param);
    return ret;
  }

  return 0;
}
