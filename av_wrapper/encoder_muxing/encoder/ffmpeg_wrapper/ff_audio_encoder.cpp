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

#include "ff_audio_encoder.h"

extern "C" {
#include <libavutil/opt.h>
#include <libswresample/swresample.h>

#include "parameter.h"
}

FFAudioEncoder::FFAudioEncoder()
    : BaseEncoder(),
      FFBaseEncoder(AV_CODEC_ID_NONE),
      audio_frame_(nullptr),
      audio_in_frame_(nullptr),
      samples_count_(0),
      swr_ctx_(nullptr) {}

FFAudioEncoder::~FFAudioEncoder() {
  av_frame_free(&audio_frame_);
  av_frame_free(&audio_in_frame_);
  swr_free(&swr_ctx_);
}

static AVFrame* alloc_audio_frame(enum AVSampleFormat sample_fmt,
                                  uint64_t channel_layout,
                                  int sample_rate,
                                  int nb_samples) {
  AVFrame* frame = av_frame_alloc();
  if (!frame) {
    fprintf(stderr, "Error allocating an audio frame\n");
    return NULL;
  }
  frame->format = sample_fmt;
  frame->channel_layout = channel_layout;
  frame->sample_rate = sample_rate;
  frame->nb_samples = nb_samples;
  if (nb_samples) {
    int ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
      fprintf(stderr, "Error allocating an audio buffer\n");
      av_frame_free(&frame);
      return NULL;
    }
  }
  return frame;
}

int FFAudioEncoder::InitConfig(MediaConfig& config) {
  int nb_samples = 0, out_nb_samples = 0;
  AudioConfig& aconfig = config.audio_config;
  enum AVSampleFormat in_sample_fmt =
      FFContext::ConvertToAVSampleFmt(aconfig.fmt);
  const char* enc_format = parameter_get_audio_enc_format();
  FFContext* help_ctx = FFBaseEncoder::GetHelpContext();
  if (!strcmp(enc_format, "aac")) {
    help_ctx->SetCodecId(AV_CODEC_ID_AAC);
  } else if (!strcmp(enc_format, "pcm") || !strcmp(enc_format, "raw")) {
    if (in_sample_fmt == AV_SAMPLE_FMT_S16) {
      help_ctx->SetCodecId(AV_CODEC_ID_PCM_S16LE);
    } else {
      fprintf(stderr, "TODO: in sample fmt: %d\n", in_sample_fmt);
      return -1;
    }
  }

  if (FFBaseEncoder::InitConfig(aconfig))
    return -1;

  AVCodecContext* c = GetAVCodecContext();
  nb_samples = aconfig.nb_samples;
  if (c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
    out_nb_samples = nb_samples;
  else {
    out_nb_samples = c->frame_size;
  }
  audio_in_frame_ = alloc_audio_frame(in_sample_fmt, c->channel_layout,
                                      c->sample_rate, nb_samples);
  if (!audio_in_frame_)
    return -1;
  if ((c->sample_fmt != in_sample_fmt) || (out_nb_samples != nb_samples)) {
    PRINTF("audio sample_fmt: %d, nb_samples: %d, channels: %d\n",
           c->sample_fmt, nb_samples, c->channels);
    audio_frame_ = alloc_audio_frame(c->sample_fmt, c->channel_layout,
                                     c->sample_rate, out_nb_samples);
    if (!audio_frame_)
      return -1;
    // Create resampler context
    swr_ctx_ = swr_alloc();
    if (!swr_ctx_) {
      fprintf(stderr, "Could not allocate resampler context\n");
      return -1;
    }
    // Set options
    av_opt_set_int(swr_ctx_, "in_channel_count", c->channels, 0);
    av_opt_set_int(swr_ctx_, "in_sample_rate", c->sample_rate, 0);
    av_opt_set_sample_fmt(swr_ctx_, "in_sample_fmt", in_sample_fmt, 0);
    av_opt_set_int(swr_ctx_, "out_channel_count", c->channels, 0);
    av_opt_set_int(swr_ctx_, "out_sample_rate", c->sample_rate, 0);
    av_opt_set_sample_fmt(swr_ctx_, "out_sample_fmt", c->sample_fmt, 0);
    // Initialize the resampling context
    if (swr_init(swr_ctx_) < 0) {
      fprintf(stderr, "Failed to initialize the resampling context\n");
      return -1;
    }
  }

  if (BaseEncoder::InitConfig(config))
    return -1;

  return 0;
}

// Just return the audio_tmp_frame
void FFAudioEncoder::GetAudioBuffer(void** buf,
                                    int* nb_samples,
                                    int* channels,
                                    enum AVSampleFormat* format) {
  AVFrame* frame = audio_in_frame_;
  *buf = frame->data[0];
  *nb_samples = frame->nb_samples;
  *channels = frame->channels;
  *format = (enum AVSampleFormat)frame->format;
}

int FFAudioEncoder::EncodeAudioFrame(AVPacket* out_pkt) {
  // Data in audio_tmp_frame
  AVCodecContext* c = GetAVCodecContext();
  AVPacket pkt = {0};
  AVFrame* frame = audio_in_frame_;
  int ret;
  int got_packet;
  int dst_nb_samples;
  av_init_packet(&pkt);

  if (swr_ctx_) {
    // Convert samples from native format to destination codec format, using the
    // resampler compute destination number of samples.
    dst_nb_samples = av_rescale_rnd(
        swr_get_delay(swr_ctx_, c->sample_rate) + frame->nb_samples,
        c->sample_rate, c->sample_rate, AV_ROUND_UP);
    assert(dst_nb_samples == audio_frame_->nb_samples);

    // When we pass a frame to the encoder, it may keep a reference to it
    // internally;
    // make sure we do not overwrite it here.
    ret = av_frame_make_writable(audio_frame_);
    if (ret < 0) {
      fprintf(stderr, "Error av_frame_make_writable\n");
      return -1;
    }
    // Convert to destination format
    ret = swr_convert(swr_ctx_, audio_frame_->data, dst_nb_samples,
                      (const uint8_t**)frame->data, frame->nb_samples);
    if (ret < 0) {
      fprintf(stderr, "Error while converting\n");
      return -1;
    }
    frame = audio_frame_;

    frame->pts = av_rescale_q(samples_count_, (AVRational){1, c->sample_rate},
                              c->time_base);
    samples_count_ += dst_nb_samples;
  }

  ret = avcodec_encode_audio2(c, &pkt, frame, &got_packet);
  if (ret < 0) {
    char str[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_strerror(ret, str, AV_ERROR_MAX_STRING_SIZE);
    fprintf(stderr, "Error encoding audio frame: %s\n", str);
    goto a_out;
  }

  if (got_packet) {
    if (out_pkt) {
      *out_pkt = pkt;
      return 0;
    } else {
      assert(0);
    }
  }
a_out:
  av_packet_unref(&pkt);
  return 0;
}

// Return if there is no remaining audio data.
void FFAudioEncoder::EncodeFlushAudioData(AVPacket* out_pkt, bool& finish) {
  int got_packet = 0;
  AVPacket pkt = {0};  // data and size must be 0
  av_init_packet(&pkt);
  int av_ret =
      avcodec_encode_audio2(GetAVCodecContext(), &pkt, NULL, &got_packet);
  if (av_ret < 0) {
    char str[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_strerror(av_ret, str, AV_ERROR_MAX_STRING_SIZE);
    fprintf(stderr, "Error encoding audio frame: %s\n", str);
    got_packet = 0;
  }

  if (got_packet) {
    assert(pkt.pts == pkt.dts);
    finish = false;
    *out_pkt = pkt;
    return;
  }
  av_packet_unref(&pkt);
  if (!got_packet) {
    finish = true;
  }
}

void* FFAudioEncoder::GetHelpContext() {
  return static_cast<void*>(FFBaseEncoder::GetHelpContext());
}
