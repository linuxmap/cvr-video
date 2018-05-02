/**
 * Copyright (C) 2016 Fuzhou Rockchip Electronics Co., Ltd
 * author: hertz.wang hertz.wong@rock-chips.com
 *         timkingh.huang timkingh.huang@rock-chips.com
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

#include "mpp_jpeg_encoder.h"

#include "buffer.h"

MPPJpegEncoder::MPPJpegEncoder() : venc_(NULL) {}

MPPJpegEncoder::~MPPJpegEncoder() {
  if (!venc_)
    return;

  if (venc_->mpp_ctx) {
    mpp_destroy(venc_->mpp_ctx);
    venc_->mpp_ctx = NULL;
  }
  free(venc_);
  venc_ = NULL;
}

int MPPJpegEncoder::InitConfig(MediaConfig& config) {
  venc_ = (struct vpu_encode*)calloc(1, sizeof(struct vpu_encode));
  if (!venc_) {
    PRINTF_NO_MEMORY;
    return -1;
  }

  JpegEncConfig& jconfig = config.jpeg_config;
  MPP_RET ret = MPP_OK;
  MppEncPrepCfg prep_cfg;
  MppEncCodecCfg codec_cfg;
  MppCodingType type = MPP_VIDEO_CodingMJPEG;
  MppFrameFormat fmt = MPP_FMT_YUV420SP;
  venc_->width = jconfig.width;
  venc_->height = jconfig.height;
  venc_->enc_out_data = NULL;
  venc_->enc_out_length = 0;

  ret = mpp_create(&venc_->mpp_ctx, &venc_->mpi);
  if (MPP_OK != ret) {
    mpp_err_f("mpp_create failed\n");
    return -1;
  }

  MppCtx mpp_ctx = venc_->mpp_ctx;
  ret = mpp_init(mpp_ctx, MPP_CTX_ENC, type);
  if (MPP_OK != ret) {
    mpp_err_f("mpp_init failed\n");
    return -1;
  }

  // NOTE: in mpp, chroma_addr = luma_addr + hor_stride*ver_stride,
  // so if there is no gap between luma and chroma in yuv,
  // hor_stride MUST be set to be width, ver_stride MUST be set to height.
  fmt = ConvertToMppPixFmt(jconfig.fmt);
  memset(&prep_cfg, 0, sizeof(prep_cfg));
  prep_cfg.change =
      MPP_ENC_PREP_CFG_CHANGE_INPUT | MPP_ENC_PREP_CFG_CHANGE_FORMAT;
  prep_cfg.width = jconfig.width;
  prep_cfg.height = jconfig.height;
  prep_cfg.hor_stride = jconfig.width;
  prep_cfg.ver_stride = jconfig.height;
  prep_cfg.format = fmt;

  MppApi* mpi = venc_->mpi;
  ret = mpi->control(mpp_ctx, MPP_ENC_SET_PREP_CFG, &prep_cfg);
  if (MPP_OK != ret) {
    mpp_err_f("mpi control enc set cfg failed\n");
    return -1;
  }

  memset(&codec_cfg, 0, sizeof(codec_cfg));
  codec_cfg.coding = type;
  codec_cfg.jpeg.change = MPP_ENC_JPEG_CFG_CHANGE_QP;
  codec_cfg.jpeg.quant = jconfig.qp; /* range 0 - 10, worst -> best */

  ret = mpi->control(mpp_ctx, MPP_ENC_SET_CODEC_CFG, &codec_cfg);
  if (ret) {
    mpp_err("mpi control enc set codec cfg failed ret %d\n", ret);
    return -1;
  }

  return BaseEncoder::InitConfig(config);
}

int MPPJpegEncoder::Encode(Buffer* src, Buffer* dst) {
  size_t dst_size = (int64_t)dst->Size();
  RK_U8* dstbuf = (RK_U8*)dst->GetVirAddr();
  int dst_fd = dst->GetFd();

  RK_U8* srcbuf = (RK_U8*)src->GetVirAddr();
  const JpegEncConfig& jconfig = GetConfig().jpeg_config;
  size_t src_size = CalPixelSize(jconfig.width, jconfig.height, jconfig.fmt);
  int src_fd = src->GetFd();

  MPP_RET ret = MPP_OK;
  RK_U32 width = venc_->width;
  RK_U32 height = venc_->height;
  MppCtx mpp_ctx = venc_->mpp_ctx;
  MppApi* mpi = venc_->mpi;
  MppTask task = NULL;
  MppFrame frame = NULL;
  MppPacket packet = NULL;
  MppBuffer pic_buf = NULL;
  MppBuffer str_buf = NULL;
  MppBufferGroup memGroup = NULL;

  ret = mpp_frame_init(&frame);
  if (MPP_OK != ret) {
    mpp_err_f("mpp_frame_init failed\n");
    goto ENCODE_OUT;
  }

  mpp_frame_set_width(frame, width);
  mpp_frame_set_height(frame, height);
  mpp_frame_set_hor_stride(frame, width);
  mpp_frame_set_ver_stride(frame, height);
  mpp_frame_set_eos(frame, 1);

  if (src_fd > 0) {
    MppBufferInfo inputCommit;

    memset(&inputCommit, 0, sizeof(inputCommit));
    inputCommit.type = MPP_BUFFER_TYPE_ION;
    inputCommit.size = src_size;
    inputCommit.fd = src_fd;

    ret = mpp_buffer_import(&pic_buf, &inputCommit);
    if (ret) {
      mpp_err_f("import input picture buffer failed\n");
      goto ENCODE_OUT;
    }
  } else {
    if (NULL == srcbuf) {
      ret = MPP_ERR_NULL_PTR;
      goto ENCODE_OUT;
    }

    ret = mpp_buffer_get(memGroup, &pic_buf, src_size);
    if (ret) {
      mpp_err_f("allocate input picture buffer failed\n");
      goto ENCODE_OUT;
    }
    memcpy((RK_U8*)mpp_buffer_get_ptr(pic_buf), srcbuf, src_size);
  }

  if (dst_fd > 0) {
    MppBufferInfo outputCommit;

    memset(&outputCommit, 0, sizeof(outputCommit));
    outputCommit.type = MPP_BUFFER_TYPE_ION;
    outputCommit.fd = dst_fd;
    outputCommit.size = dst_size;
    outputCommit.ptr = (void*)dstbuf;

    ret = mpp_buffer_import(&str_buf, &outputCommit);
    if (ret) {
      mpp_err_f("import output stream buffer failed\n");
      goto ENCODE_OUT;
    }
  } else {
    ret = mpp_buffer_get(memGroup, &str_buf, width * height);
    if (ret) {
      mpp_err_f("allocate output stream buffer failed\n");
      goto ENCODE_OUT;
    }
  }

  mpp_frame_set_buffer(frame, pic_buf);
  mpp_packet_init_with_buffer(&packet, str_buf);
  // mpp_log_f("mpp import input fd %d output fd %d",
  //	    mpp_buffer_get_fd(pic_buf), mpp_buffer_get_fd(str_buf));

  ret = mpi->poll(mpp_ctx, MPP_PORT_INPUT, MPP_POLL_BLOCK);
  if (ret) {
    mpp_err("mpp task input poll failed ret %d\n", ret);
    goto ENCODE_OUT;
  }

  ret = mpi->dequeue(mpp_ctx, MPP_PORT_INPUT, &task);
  if (ret || NULL == task) {
    mpp_err("mpp task input dequeue failed ret %d task %p\n", ret, task);
    goto ENCODE_OUT;
  }

  mpp_task_meta_set_frame(task, KEY_INPUT_FRAME, frame);
  mpp_task_meta_set_packet(task, KEY_OUTPUT_PACKET, packet);

  ret = mpi->enqueue(mpp_ctx, MPP_PORT_INPUT, task);
  if (ret) {
    mpp_err("mpp task input enqueue failed\n");
    goto ENCODE_OUT;
  }

  ret = mpi->poll(mpp_ctx, MPP_PORT_OUTPUT, MPP_POLL_BLOCK);
  if (ret) {
    mpp_err("mpp task output poll failed ret %d\n", ret);
    goto ENCODE_OUT;
  }

  ret = mpi->dequeue(mpp_ctx, MPP_PORT_OUTPUT, &task);
  if (ret || NULL == task) {
    mpp_err("mpp task output dequeue failed ret %d task %p\n", ret, task);
    goto ENCODE_OUT;
  }

  if (task) {
    MppFrame packet_out = NULL;

    mpp_task_meta_get_packet(task, KEY_OUTPUT_PACKET, &packet_out);

    mpp_assert(packet_out == packet);

    ret = mpi->enqueue(mpp_ctx, MPP_PORT_OUTPUT, task);
    if (ret) {
      mpp_err_f("mpp task output enqueue failed\n");
      goto ENCODE_OUT;
    }
    task = NULL;
  }

  if ((packet != NULL) && (dst_fd >= 0)) {
    size_t length = mpp_packet_get_length(packet);
    dst->SetValidDataSize(length);
  }

  ret = mpi->reset(mpp_ctx);
  if (MPP_OK != ret) {
    mpp_err_f("mpi->reset failed\n");
    goto ENCODE_OUT;
  }

ENCODE_OUT:
  if (pic_buf) {
    mpp_buffer_put(pic_buf);
    pic_buf = NULL;
  }

  if (str_buf) {
    mpp_buffer_put(str_buf);
    str_buf = NULL;
  }

  if (frame)
    mpp_frame_deinit(&frame);

  if (packet)
    mpp_packet_deinit(&packet);

  return ret;
}
