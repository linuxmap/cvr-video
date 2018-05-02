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

#include "ff_context.h"

#include "video_common.h"
#include "encoder_muxing/encoder/buffer.h"

FFContext::FFContext()
    : av_fmt_ctx_(nullptr),
      av_codec_(nullptr),
      av_stream_(nullptr),
      codec_id_(AV_CODEC_ID_NONE),
      codec_name_(nullptr) {}

FFContext::~FFContext() {
  if (av_stream_) {
    avcodec_close(av_stream_->codec);
    av_codec_ = nullptr;
    av_stream_ = nullptr;
  }
  if (av_fmt_ctx_) {
    avformat_free_context(av_fmt_ctx_);
    av_fmt_ctx_ = nullptr;
  }
}

char* FFContext::GetCodecName() {
  return codec_id_ != AV_CODEC_ID_NONE ? (char*)avcodec_get_name(codec_id_)
                                       : codec_name_;
}

int FFContext::Init(bool ff_open_codec) {
  assert(!av_codec_);
  av_codec_ = codec_id_ != AV_CODEC_ID_NONE
                  ? avcodec_find_encoder(codec_id_)
                  : avcodec_find_encoder_by_name(codec_name_);
  if (ff_open_codec && !av_codec_) {
    printf("Could not find encoder for '%s'\n", GetCodecName());
    return -1;
  }
  assert(!av_fmt_ctx_);
  av_fmt_ctx_ = avformat_alloc_context();
  if (!av_fmt_ctx_) {
    printf("alloc avfmtctx failed for '%s'\n", GetCodecName());
    return -1;
  }
  assert(!av_stream_);
  av_stream_ = avformat_new_stream(av_fmt_ctx_, av_codec_);
  if (!av_stream_) {
    printf("alloc stream failed for '%s'\n", GetCodecName());
    return -1;
  }
  return 0;
}

int FFContext::InitConfig(VideoConfig& vconfig, bool ff_open_codec) {
  if (Init(ff_open_codec)) {
    printf("ff context init failed!\n");
    return -1;
  }

  AVStream* stream = GetAVStream();
  AVCodec* codec = GetAVCodec();
  AVCodecContext* c = stream->codec;

  int frame_rate = vconfig.frame_rate;
  assert(frame_rate > 0);
  assert(c);
  if (codec)
    c->codec_id = codec->id;
  c->codec_type = AVMEDIA_TYPE_VIDEO;
  c->bit_rate = vconfig.bit_rate;
  assert(vconfig.bit_rate);
  // Resolution must be a multiple of two.
  c->width = vconfig.width;
  c->height = vconfig.height;
  stream->time_base = (AVRational){1, frame_rate};
  // timebase: This is the fundamental unit of time (in seconds) in terms
  // of which frame timestamps are represented. For fixed-fps content,
  // timebase should be 1/framerate and timestamp increments should be
  // identical to 1.
  c->time_base = stream->time_base;
  c->level = vconfig.level;
  c->gop_size =
      vconfig.gop_size;  // emit one intra frame every GOP_SIZE frames at most
  assert(c->gop_size >= 0);
  c->profile = vconfig.profile;
  c->pix_fmt = FFContext::ConvertToAVPixFmt(vconfig.fmt);
  assert(c->pix_fmt != AV_PIX_FMT_NONE);
  c->global_quality = vconfig.quality;
  c->qval = vconfig.qp_init;
  c->qmin = vconfig.qp_min;
  c->qmax = vconfig.qp_max;
  c->max_qdiff = vconfig.qp_step;
  c->compression_level = vconfig.rc_mode;
  // Some formats want stream headers to be separate.
  c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

  // If the context does not do encoding but muxing, do not open the codec.
  if (ff_open_codec) {
    // Open the video codec.
    int av_ret = avcodec_open2(c, codec, nullptr);
    if (av_ret < 0) {
      char str[AV_ERROR_MAX_STRING_SIZE] = {0};
      av_strerror(av_ret, str, AV_ERROR_MAX_STRING_SIZE);
      fprintf(stderr, "Could not open video codec: %s\n", str);
      return -1;
    }
  }
  return 0;
}

int FFContext::InitConfig(AudioConfig& aconfig) {
  // As audio encoding is done by ffmpeg, set ff_open_codec true.
  if (Init(true)) {
    printf("ff context init failed!\n");
    return -1;
  }
  AVStream* stream = GetAVStream();
  AVCodec* codec = GetAVCodec();
  AVCodecContext* c = stream->codec;

  c->sample_fmt =
      codec->sample_fmts ? codec->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
  c->bit_rate = aconfig.bit_rate;
  c->sample_rate = aconfig.sample_rate;
  if (codec->supported_samplerates) {
    uint8_t match = 0;
    for (int i = 0; codec->supported_samplerates[i]; i++) {
      if (codec->supported_samplerates[i] == c->sample_rate)
        match = 1;
    }
    if (!match) {
      // It's better to set the same sample_rate.
      fprintf(stderr, "uncorrect config for audio sample_rate: %d\n",
              c->sample_rate);
      return -1;
    }
  }
  c->channel_layout = aconfig.channel_layout;

  if (codec->channel_layouts) {
    uint8_t match = 0;
    for (int i = 0; codec->channel_layouts[i]; i++) {
      if (codec->channel_layouts[i] == c->channel_layout)
        match = 1;
    }
    if (!match) {
      // It's better to set the same channel_layout
      fprintf(stderr, "uncorrect config for audio channel layout\n");
      return -1;
    }
  }
  c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
  aconfig.nb_channels = c->channels;
  stream->time_base = (AVRational){1, c->sample_rate};
  c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  int av_ret = avcodec_open2(c, codec, NULL);
  if (av_ret < 0) {
    char str[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_strerror(av_ret, str, AV_ERROR_MAX_STRING_SIZE);
    fprintf(stderr, "Could not open audio codec: %s\n", str);
    return -1;
  }
  return 0;
}

enum AVPixelFormat FFContext::ConvertToAVPixFmt(const PixelFormat& fmt) {
  static enum AVPixelFormat av_fmts[PIX_FMT_NB] =
      {[PIX_FMT_YUV420P] = AV_PIX_FMT_YUV420P,
       [PIX_FMT_NV12] = AV_PIX_FMT_NV12,
       [PIX_FMT_NV21] = AV_PIX_FMT_NV21,
       [PIX_FMT_YUV422P] = AV_PIX_FMT_YUV422P,
       [PIX_FMT_NV16] = AV_PIX_FMT_NV16,
       [PIX_FMT_NV61] = AV_PIX_FMT_NV61,
       [PIX_FMT_YVYU422] = AV_PIX_FMT_YVYU422,
       [PIX_FMT_UYVY422] = AV_PIX_FMT_UYVY422,
       [PIX_FMT_RGB565LE] = AV_PIX_FMT_RGB565LE,
       [PIX_FMT_BGR565LE] = AV_PIX_FMT_BGR565LE,
       [PIX_FMT_RGB24] = AV_PIX_FMT_RGB24,
       [PIX_FMT_BGR24] = AV_PIX_FMT_BGR24,
       [PIX_FMT_RGB32] = AV_PIX_FMT_RGB32,
       [PIX_FMT_BGR32] = AV_PIX_FMT_BGR32};
  assert(fmt >= 0 && fmt < PIX_FMT_NB);
  return av_fmts[fmt];
}

enum AVSampleFormat FFContext::ConvertToAVSampleFmt(const SampleFormat& fmt) {
  switch (fmt) {
    case SAMPLE_FMT_U8:
      return AV_SAMPLE_FMT_U8;
    case SAMPLE_FMT_S16:
      return AV_SAMPLE_FMT_S16;
    case SAMPLE_FMT_S32:
      return AV_SAMPLE_FMT_S32;
    default:
      printf("unsupport for sample fmt: %d\n", fmt);
      return AV_SAMPLE_FMT_NONE;
  }
}

int FFContext::PackEncodedDataToAVPacket(const Buffer& buf,
                                         AVPacket& out_pkt,
                                         const bool copy) {
  AVPacket pkt;
  av_init_packet(&pkt);
  pkt.data = static_cast<uint8_t*>(buf.GetVirAddr());
  pkt.size = buf.GetValidDataSize();

  if (copy) {
    int ret = 0;
    if (0 != av_copy_packet(&out_pkt, &pkt)) {
      printf("in %s, av_copy_packet failed!\n", __func__);
      ret = -1;
    }
    av_packet_unref(&pkt);
    return ret;
  } else {
    out_pkt = pkt;
    return 0;
  }
}

class FFAVGlobalRegister {
 public:
  FFAVGlobalRegister() { av_register_all(); }
};

static FFAVGlobalRegister gAVReg;
