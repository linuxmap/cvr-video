#include "vpu.h"
#include <dlfcn.h>
#include <errno.h>
#include <fs_manage/fs_picture.h>

#include "common.h"
#include "video_ion_alloc.h"

enum AVPixelFormat vpu_color_space2ffmpeg(MppFrameFormat mpp_fmt) {
  switch (mpp_fmt) {
    case MPP_FMT_YUV420P:
      return AV_PIX_FMT_YUV420P; /* YYYY... UUUU... VVVV */
    case MPP_FMT_YUV420SP:
      return AV_PIX_FMT_NV12; /* YYYY... UVUVUV...    */
    case MPP_FMT_YUV422_YUYV:
      return AV_PIX_FMT_YVYU422; /* YUYVYUYV...          */
    case MPP_FMT_YUV422_UYVY:
      return AV_PIX_FMT_UYVY422; /* UYVYUYVY...          */
    case MPP_FMT_RGB565:
      return AV_PIX_FMT_RGB565LE; /* 16-bit RGB           */
    case MPP_FMT_BGR565:
      return AV_PIX_FMT_BGR565LE; /* 16-bit RGB           */
    case MPP_FMT_RGB555:
      return AV_PIX_FMT_RGB555LE; /* 15-bit RGB           */
    case MPP_FMT_BGR555:
      return AV_PIX_FMT_BGR555LE; /* 15-bit RGB           */
    case MPP_FMT_RGB444:
      return AV_PIX_FMT_RGB444LE; /* 12-bit RGB           */
    case MPP_FMT_BGR444:
      return AV_PIX_FMT_BGR444LE; /* 12-bit RGB           */
    case MPP_FMT_RGB888:
      return AV_PIX_FMT_RGB24; /* 24-bit RGB           */
    case MPP_FMT_BGR888:
      return AV_PIX_FMT_BGR24; /* 24-bit RGB           */
    default:
      fprintf(stderr, "unsupport mpp format to ffmpeg: %d", mpp_fmt);
      break;
  }
  return (enum AVPixelFormat) - 1;
}

int vpu_nv12_encode_jpeg_init_ext(struct vpu_encode* encode,
                                  int width,
                                  int height,
                                  int quant) {
  MPP_RET ret = MPP_OK;
  MppEncPrepCfg prep_cfg;
  MppEncCodecCfg codec_cfg;
  MppCodingType type = MPP_VIDEO_CodingMJPEG;
  MppFrameFormat fmt = MPP_FMT_YUV420SP;
  memset(encode, 0, sizeof(*encode));
  encode->jpeg_enc_out.fd = -1;
  encode->jpeg_enc_out.client = -1;
  encode->width = width;
  encode->height = height;
  encode->enc_out_data = NULL;
  encode->enc_out_length = 0;

  ret = mpp_create(&encode->mpp_ctx, &encode->mpi);
  if (MPP_OK != ret) {
    mpp_err_f("mpp_create failed\n");
    return -1;
  }

  MppCtx mpp_ctx = encode->mpp_ctx;
  ret = mpp_init(mpp_ctx, MPP_CTX_ENC, type);
  if (MPP_OK != ret) {
    mpp_err_f("mpp_init failed\n");
    return -1;
  }

  memset(&prep_cfg, 0, sizeof(prep_cfg));
  prep_cfg.change =
      MPP_ENC_PREP_CFG_CHANGE_INPUT | MPP_ENC_PREP_CFG_CHANGE_FORMAT;
  prep_cfg.width = width;
  prep_cfg.height = height;
  prep_cfg.hor_stride = width;
  prep_cfg.ver_stride = height;
  prep_cfg.format = fmt;

  MppApi* mpi = encode->mpi;
  ret = mpi->control(mpp_ctx, MPP_ENC_SET_PREP_CFG, &prep_cfg);
  if (MPP_OK != ret) {
    mpp_err_f("mpi control enc set cfg failed\n");
    return -1;
  }

  memset(&codec_cfg, 0, sizeof(codec_cfg));
  codec_cfg.coding = type;
  codec_cfg.jpeg.change = MPP_ENC_JPEG_CFG_CHANGE_QP;
  codec_cfg.jpeg.quant = quant; /* range 0 - 10, worst -> best */

  ret = mpi->control(mpp_ctx, MPP_ENC_SET_CODEC_CFG, &codec_cfg);
  if (ret) {
    mpp_err("mpi control enc set codec cfg failed ret %d\n", ret);
    return -1;
  }

  if (video_ion_alloc_rational(&encode->jpeg_enc_out, width, height, 1, 1)) {
    printf("%s:video ion alloc fail!\n", __func__);
    return -1;
  }

  return 0;
}

int vpu_nv12_encode_jpeg_init(struct vpu_encode* encode,
                              int width,
                              int height) {
  return vpu_nv12_encode_jpeg_init_ext(encode, width, height, 10);
}

int vpu_nv12_encode_jpeg_doing(struct vpu_encode* encode,
                               void* srcbuf,
                               int src_fd,
                               size_t src_size) {
  MPP_RET ret = MPP_OK;
  RK_U32 width = encode->width;
  RK_U32 height = encode->height;
  MppCtx mpp_ctx = encode->mpp_ctx;
  MppApi* mpi = encode->mpi;
  MppTask task = NULL;
  MppFrame frame = NULL;
  MppBuffer pic_buf = NULL;
  MppBuffer str_buf = NULL;
  MppBufferGroup memGroup = NULL;

  encode->enc_out_data = NULL;
  encode->enc_out_length = 0;

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

  if (encode->jpeg_enc_out.fd > 0) {
    MppBufferInfo outputCommit;

    memset(&outputCommit, 0, sizeof(outputCommit));
    outputCommit.type = MPP_BUFFER_TYPE_ION;
    outputCommit.fd = encode->jpeg_enc_out.fd;
    outputCommit.size = encode->jpeg_enc_out.size;
    outputCommit.ptr = (void*)encode->jpeg_enc_out.buffer;

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
  if (encode->packet)
    mpp_packet_deinit(&encode->packet);
  mpp_packet_init_with_buffer(&encode->packet, str_buf);
  //mpp_log_f("mpp import input fd %d output fd %d", mpp_buffer_get_fd(pic_buf),
  //          mpp_buffer_get_fd(str_buf));

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
  mpp_task_meta_set_packet(task, KEY_OUTPUT_PACKET, encode->packet);

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

    mpp_assert(packet_out == encode->packet);

    ret = mpi->enqueue(mpp_ctx, MPP_PORT_OUTPUT, task);
    if (ret) {
      mpp_err_f("mpp task output enqueue failed\n");
      goto ENCODE_OUT;
    }
    task = NULL;
  }

  if ((encode->packet != NULL)) {
    RK_U8* data = (RK_U8*)mpp_packet_get_data(encode->packet);
    size_t length = mpp_packet_get_length(encode->packet);

    encode->enc_out_data = data;
    encode->enc_out_length = length;
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

  return ret;
}

void vpu_nv12_encode_jpeg_done(struct vpu_encode* encode) {
  if (encode) {
    if (encode->packet)
      mpp_packet_deinit(&encode->packet);

    if (encode->mpp_ctx) {
      mpp_destroy(encode->mpp_ctx);
      encode->mpp_ctx = NULL;
    }

    video_ion_free(&encode->jpeg_enc_out);
  }
}

int vpu_decode_jpeg_init(struct vpu_decode* decode, int width, int height) {
  int ret;
  decode->in_width = width;
  decode->in_height = height;

  ret = mpp_buffer_group_get_internal(&decode->memGroup, MPP_BUFFER_TYPE_ION);
  if (MPP_OK != ret) {
    mpp_err("memGroup mpp_buffer_group_get failed\n");
    return ret;
  }

  ret = mpp_create(&decode->mpp_ctx, &decode->mpi);
  if (MPP_OK != ret) {
    mpp_err("mpp_create failed\n");
    return -1;
  }

  ret = mpp_init(decode->mpp_ctx, MPP_CTX_DEC, MPP_VIDEO_CodingMJPEG);
  if (MPP_OK != ret) {
    mpp_err("mpp_init failed\n");
    return -1;
  }

  MppApi* mpi = decode->mpi;
  MppCtx mpp_ctx = decode->mpp_ctx;
  MppFrame frame;
  ret = mpp_frame_init(&frame);
  if (!frame || (MPP_OK != ret)) {
    mpp_err("failed to init mpp frame!");
    return MPP_ERR_NOMEM;
  }

  mpp_frame_set_fmt(frame, MPP_FMT_YUV420SP);
  mpp_frame_set_width(frame, decode->in_width);
  mpp_frame_set_height(frame, decode->in_height);
  mpp_frame_set_hor_stride(frame, MPP_ALIGN(decode->in_width, 16));
  mpp_frame_set_ver_stride(frame, MPP_ALIGN(decode->in_height, 16));

  ret = mpi->control(mpp_ctx, MPP_DEC_SET_FRAME_INFO, (MppParam)frame);
  mpp_frame_deinit(&frame);

  return 0;
}

int vpu_decode_jpeg_doing(struct vpu_decode* decode,
                          char* in_data,
                          RK_S32 in_size,
                          int out_fd,
                          char* out_data) {
  MPP_RET ret = MPP_OK;
  MppTask task = NULL;

  decode->pkt_size = in_size;
  if (decode->pkt_size <= 0) {
    mpp_err("invalid input size %d\n", decode->pkt_size);
    return MPP_ERR_UNKNOW;
  }

  if (NULL == in_data) {
    ret = MPP_ERR_NULL_PTR;
    goto DECODE_OUT;
  }

  /* try import input buffer and output buffer */
  RK_U32 width = decode->in_width;
  RK_U32 height = decode->in_height;
  RK_U32 hor_stride = MPP_ALIGN(width, 16);
  RK_U32 ver_stride = MPP_ALIGN(height, 16);
  MppFrame frame = NULL;
  MppPacket packet = NULL;
  MppBuffer str_buf = NULL; /* input */
  MppBuffer pic_buf = NULL; /* output */
  MppCtx mpp_ctx = decode->mpp_ctx;
  MppApi* mpi = decode->mpi;

  ret = mpp_frame_init(&frame);
  if (MPP_OK != ret) {
    mpp_err_f("mpp_frame_init failed\n");
    goto DECODE_OUT;
  }

  ret = mpp_buffer_get(decode->memGroup, &str_buf, decode->pkt_size);
  if (ret) {
    mpp_err_f("allocate input picture buffer failed\n");
    goto DECODE_OUT;
  }
  memcpy((RK_U8*)mpp_buffer_get_ptr(str_buf), in_data, decode->pkt_size);

  if (out_fd > 0) {
    MppBufferInfo outputCommit;

    memset(&outputCommit, 0, sizeof(outputCommit));
    /* in order to avoid interface change use space in output to transmit
     * information */
    outputCommit.type = MPP_BUFFER_TYPE_ION;
    outputCommit.fd = out_fd;
    outputCommit.size = hor_stride * ver_stride * 2;
    outputCommit.ptr = (void*)out_data;

    ret = mpp_buffer_import(&pic_buf, &outputCommit);
    if (ret) {
      mpp_err_f("import output stream buffer failed\n");
      goto DECODE_OUT;
    }
  } else {
    ret =
        mpp_buffer_get(decode->memGroup, &pic_buf, hor_stride * ver_stride * 2);
    if (ret) {
      mpp_err_f("allocate output stream buffer failed\n");
      goto DECODE_OUT;
    }
  }

  mpp_packet_init_with_buffer(&packet, str_buf); /* input */
  mpp_frame_set_buffer(frame, pic_buf);          /* output */

  // printf("mpp import input fd %d output fd %d\n",
  //        mpp_buffer_get_fd(str_buf), mpp_buffer_get_fd(pic_buf));

  ret = mpi->poll(mpp_ctx, MPP_PORT_INPUT, MPP_POLL_BLOCK);
  if (ret) {
    mpp_err("mpp input poll failed\n");
    goto DECODE_OUT;
  }

  ret = mpi->dequeue(mpp_ctx, MPP_PORT_INPUT, &task); /* input queue */
  if (ret) {
    mpp_err("mpp task input dequeue failed\n");
    goto DECODE_OUT;
  }

  mpp_assert(task);

  mpp_task_meta_set_packet(task, KEY_INPUT_PACKET, packet);
  mpp_task_meta_set_frame(task, KEY_OUTPUT_FRAME, frame);

  ret = mpi->enqueue(mpp_ctx, MPP_PORT_INPUT, task); /* input queue */
  if (ret) {
    mpp_err("mpp task input enqueue failed\n");
    goto DECODE_OUT;
  }

  /* poll and wait here */
  ret = mpi->poll(mpp_ctx, MPP_PORT_OUTPUT, MPP_POLL_BLOCK);
  if (ret) {
    mpp_err("mpp output poll failed\n");
    goto DECODE_OUT;
  }

  ret = mpi->dequeue(mpp_ctx, MPP_PORT_OUTPUT, &task); /* output queue */
  if (ret) {
    mpp_err("mpp task output dequeue failed\n");
    goto DECODE_OUT;
  }

  mpp_assert(task);

  if (task) {
    RK_U32 err_info = 0;
    MppFrame frame_out = NULL;
    decode->fmt = MPP_FMT_YUV420SP;

    mpp_task_meta_get_frame(task, KEY_OUTPUT_FRAME, &frame_out);
    mpp_assert(frame_out == frame);

    err_info = mpp_frame_get_errinfo(frame_out);
    if (!err_info) {
      decode->fmt = mpp_frame_get_fmt(frame_out);
      if (MPP_FMT_YUV422SP != decode->fmt && MPP_FMT_YUV420SP != decode->fmt)
        printf("No support USB JPEG decode format!\n");
    }

    ret = mpi->enqueue(mpp_ctx, MPP_PORT_OUTPUT, task);
    if (ret) {
      mpp_err("mpp task output enqueue failed\n");
      goto DECODE_OUT;
    }
    task = NULL;

    if (err_info)
      ret = MPP_NOK;
  }

DECODE_OUT:
  if (str_buf) {
    mpp_buffer_put(str_buf);
    str_buf = NULL;
  }

  if (pic_buf) {
    mpp_buffer_put(pic_buf);
    pic_buf = NULL;
  }

  if (frame)
    mpp_frame_deinit(&frame);

  if (packet)
    mpp_packet_deinit(&packet);

  return ret;
}

int vpu_decode_jpeg_done(struct vpu_decode* decode) {
  MPP_RET ret = MPP_OK;
  ret = mpp_destroy(decode->mpp_ctx);
  if (ret != MPP_OK) {
    mpp_err("something wrong with mpp_destroy! ret:%d\n", ret);
  }

  if (decode->memGroup) {
    mpp_buffer_group_put(decode->memGroup);
    decode->memGroup = NULL;
  }

  return ret;
}
