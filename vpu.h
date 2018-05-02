#ifndef __VPU_ENCODE_H__
#define __VPU_ENCODE_H__

#include <memory.h>
#include <stdint.h>
#include <stdio.h>

#include <mpp/mpp_log.h>
#include <mpp/rk_mpi.h>
#include <libavutil/pixfmt.h>

#include "common.h"

#define MPP_ALIGN(x, a) (((x) + (a)-1) & ~((a)-1))

struct vpu_encode {
  MppCtx mpp_ctx;
  MppApi* mpi;
  int width;
  int height;
  struct video_ion jpeg_enc_out;
  RK_U8* enc_out_data;
  RK_U32 enc_out_length;
  MppPacket packet;
};

struct vpu_decode {
  int in_width;
  int in_height;
  RK_S32 pkt_size;
  MppCtx mpp_ctx;
  MppApi* mpi;
  MppBufferGroup memGroup;
  MppFrameFormat fmt;
};

int vpu_nv12_encode_jpeg_init_ext(struct vpu_encode* encode,
                              int width,
                              int height,
                              int quant);

int vpu_nv12_encode_jpeg_init(struct vpu_encode* encode, int width, int height);

int vpu_nv12_encode_jpeg_doing(struct vpu_encode* encode,
                               void* srcbuf,
                               int src_fd,
                               size_t src_size);

void vpu_nv12_encode_jpeg_done(struct vpu_encode* encode);

int vpu_decode_jpeg_init(struct vpu_decode* decode, int width, int height);

int vpu_decode_jpeg_doing(struct vpu_decode* decode,
                          char* in_data,
                          RK_S32 in_size,
                          int out_fd,
                          char* out_data);

int vpu_decode_jpeg_done(struct vpu_decode* decode);

inline enum AVPixelFormat vpu_color_space2ffmpeg(MppFrameFormat mpp_fmt);

#endif
