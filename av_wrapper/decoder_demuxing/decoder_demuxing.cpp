#include "decoder_demuxing.h"
#include "../video_common.h"

DecoderDemuxer::DecoderDemuxer() {
  fmt_ctx = nullptr;
  memset(&info, 0, sizeof(info));
  video_stream_idx = -1;
  video_stream = nullptr;
  video_dec_ctx = nullptr;
  video_frame_count = 0;

  audio_stream_idx = -1;
  audio_stream = nullptr;
  audio_dec_ctx = nullptr;
  audio_frame_count = 0;
  disable_flag = 0;
  finished = false;
}

DecoderDemuxer::~DecoderDemuxer() {
  avcodec_close(video_dec_ctx);
  avcodec_close(audio_dec_ctx);
  avformat_close_input(&fmt_ctx);
}

static int open_codec_context(int* stream_idx,
                              AVFormatContext* fmt_ctx,
                              enum AVMediaType type) {
  int ret, stream_index;
  AVStream* st;
  AVCodecContext* dec_ctx = NULL;
  AVCodec* dec = NULL;
  AVDictionary* opts = NULL;

  ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
  if (ret < 0) {
    // PRINTF("Could not find %s stream\n",
    //        av_get_media_type_string(type));
    return ret;
  } else {
    stream_index = ret;
    st = fmt_ctx->streams[stream_index];

    /* find decoder for the stream */
    dec_ctx = st->codec;
    dec = avcodec_find_decoder(dec_ctx->codec_id);
    if (!dec) {
      fprintf(stderr, "Failed to find %s codec, codec_id: %d\n",
              av_get_media_type_string(type), dec_ctx->codec_id);
      return AVERROR(EINVAL);
    }

    /* Init the decoders, with or without reference counting */
    // av_dict_set(&opts, "refcounted_frames", refcount ? "1" : "0", 0);
    if ((ret = avcodec_open2(dec_ctx, dec, &opts)) < 0) {
      fprintf(stderr, "Failed to open %s codec\n",
              av_get_media_type_string(type));
      return ret;
    }
    *stream_idx = stream_index;
  }

  return 0;
}

int DecoderDemuxer::init(char* src_filepath, uint32_t flag) {
  int ret = 0;
  int width = -1, height = -1;
  enum AVPixelFormat pix_fmt = AV_PIX_FMT_NV12;

  av_register_all();
  // av_log_set_level(AV_LOG_TRACE);
  /* open input file, and allocate format context */
  ret = avformat_open_input(&fmt_ctx, src_filepath, NULL, NULL);
  if (ret < 0) {
    fprintf(stderr, "Could not open source file %s\n", src_filepath);
    return ret;
  }
  /* retrieve stream information */
  ret = avformat_find_stream_info(fmt_ctx, NULL);
  if (ret < 0) {
    fprintf(stderr, "Could not find stream information\n");
    return ret;
  }
  if (open_codec_context(&video_stream_idx, fmt_ctx, AVMEDIA_TYPE_VIDEO) >= 0) {
    video_stream = fmt_ctx->streams[video_stream_idx];
    video_dec_ctx = video_stream->codec;
    width = video_dec_ctx->width;
    height = video_dec_ctx->height;
    pix_fmt = video_dec_ctx->pix_fmt;
  }
  if (open_codec_context(&audio_stream_idx, fmt_ctx, AVMEDIA_TYPE_AUDIO) >= 0) {
    audio_stream = fmt_ctx->streams[audio_stream_idx];
    audio_dec_ctx = audio_stream->codec;
  }
  /* dump input information to stderr */
  // av_dump_format(fmt_ctx, 0, src_filename, 0);
  if (!audio_stream && !video_stream) {
    fprintf(stderr,
            "Could not find audio or video stream in the input, aborting\n");
    return AVERROR(EINVAL);
  }
  if (video_stream) {
    info.pixel_fmt = pix_fmt;
    info.width = width;
    info.height = height;
    info.video_bit_rate = video_dec_ctx->bit_rate;
    info.video_encoder_level = video_dec_ctx->level;
    info.video_gop_size = video_dec_ctx->gop_size;
    info.stream_frame_rate =
        video_dec_ctx->time_base.den / video_dec_ctx->time_base.num;
  }
  if (audio_stream) {
    info.audio_bit_rate = audio_dec_ctx->bit_rate;
    info.audio_sample_rate = audio_dec_ctx->sample_rate;
    info.channel_layout = audio_dec_ctx->channel_layout;
    info.nb_channels = audio_dec_ctx->channels;
    info.out_nb_samples = audio_dec_ctx->frame_size;
    info.out_sample_fmt = audio_dec_ctx->sample_fmt;
    if (!info.channel_layout) {
      info.channel_layout = av_get_default_channel_layout(info.nb_channels);
      // audio_dec_ctx->channel_layout = info.channel_layout;
    }
  }
  disable_flag = flag;
  // dump_decode_media_info(decode_handle);
  return 0;
}

int DecoderDemuxer::decode_packet(AVPacket* pkt,
                                  AVFrame* frame,
                                  int* got_frame) {
  int ret = 0;
  int decoded = pkt->size;
  *got_frame = 0;
  if (pkt->stream_index == video_stream_idx) {
    /* decode video frame */
    ret = avcodec_decode_video2(video_dec_ctx, frame, got_frame, pkt);
    if (ret < 0) {
      char str[AV_ERROR_MAX_STRING_SIZE] = {0};
      av_strerror(ret, str, AV_ERROR_MAX_STRING_SIZE);
      fprintf(stderr, "Error decoding video frame (%s)\n", str);
      return ret;
    }
#if 0  // def DEBUG
        if (*got_frame) {
            PRINTF("video_frame n:%d coded_n:%d pts:%lld\n", video_frame_count++,
                   frame->coded_picture_number, frame->pts);
            //av_ts2timestr(frame->pts, &video_dec_ctx->time_base));
        }
#endif
  } else if (pkt->stream_index == audio_stream_idx) {
    /* decode audio frame */
    ret = avcodec_decode_audio4(audio_dec_ctx, frame, got_frame, pkt);
    if (ret < 0) {
      char str[AV_ERROR_MAX_STRING_SIZE] = {0};
      av_strerror(ret, str, AV_ERROR_MAX_STRING_SIZE);
      fprintf(stderr, "Error decoding audio frame (%s)\n", str);
      return ret;
    }
    if (!frame->channel_layout)
      frame->channel_layout = info.channel_layout;
    /* Some audio decoders decode only part of the packet, and have to be
     * called again with the remainder of the packet data.
     * Sample: fate-suite/lossless-audio/luckynight-partial.shn
     * Also, some decoders might over-read the packet. */
    decoded = FFMIN(ret, pkt->size);
    if (*got_frame) {
#if 0
            size_t unpadded_linesize =
                frame->nb_samples *
                av_get_bytes_per_sample((enum AVSampleFormat)frame->format);
            PRINTF("audio_frame n:%d nb_samples:%d pts:%lld\n", audio_frame_count++,
                   frame->nb_samples,
                   frame->pts);
#endif
      // av_ts2timestr(frame->pts, &audio_dec_ctx->time_base));

      /* Write the raw audio data samples of the first plane. This works
       * fine for packed formats (e.g. AV_SAMPLE_FMT_S16). However,
       * most audio decoders output planar audio, which uses a separate
       * plane of audio samples for each channel (e.g. AV_SAMPLE_FMT_S16P).
       * In other words, this code will write only the first audio channel
       * in these cases.
       * You should use libswresample or libavfilter to convert the frame
       * to packed data. */
      // av_assert0(audio_out_buf);
      //*audio_out_buf = frame->extended_data[0];
      //*audio_out_size = unpadded_linesize;
      // TODO use resample to external required SAMPLE FMT
    }
  }
  return decoded;
}

int DecoderDemuxer::read_and_decode_one_frame(image_handle image_buf /*in&out*/,
                                              AVFrame* out_frame,
                                              int* got_frame,
                                              int* isVideo) {
  AVPacket pkt;
  int ret = 0;

  av_init_packet(&pkt);
  pkt.data = NULL;
  pkt.size = 0;
  ret = av_read_frame(fmt_ctx, &pkt);
  if (pkt.stream_index == video_stream_idx) {
    *isVideo = 1;
    if (image_buf) {
      out_frame->format = video_dec_ctx->pix_fmt;
      out_frame->width = info.width;
      out_frame->height = info.height;
      out_frame->linesize[0] = out_frame->linesize[1] = info.width;
      out_frame->linesize[2] = 0;
      out_frame->user_flags |= VPU_MEM_ALLOC_EXTERNAL;
      out_frame->data[0] = out_frame->data[1] = out_frame->data[2] = 0;
      out_frame->data[3] = (uint8_t*)image_buf;
    } else {
      out_frame->user_flags &= (~VPU_MEM_ALLOC_EXTERNAL);
    }
  } else {
    *isVideo = 0;
  }

  if (((*isVideo) && (disable_flag & VIDEO_DISABLE_FLAG)) ||
      (!(*isVideo) && (disable_flag & AUDIO_DISABLE_FLAG))) {
    av_packet_unref(&pkt);
    if (ret < 0) {
      finished = true;
    }
    return 0;
  }

  if (ret >= 0) {
    AVPacket orig_pkt = pkt;
    do {
      ret = decode_packet(&pkt, out_frame, got_frame);
      if (ret < 0)
        break;
      pkt.data += ret;
      pkt.size -= ret;
    } while (pkt.size > 0);
    av_packet_unref(&orig_pkt);
    finished = false;
  } else {
    pkt.data = NULL;
    pkt.size = 0;
    do {
      ret = decode_packet(&pkt, out_frame, got_frame);
    } while (*got_frame);
    finished = true;
  }
  return ret;
}

int DecoderDemuxer::rewind() {
  int ret = avformat_seek_file(fmt_ctx, -1, INT64_MIN, 0, INT64_MAX, 0);
  if (0 == ret)
    finished = false;

  return ret;
}

int DecoderDemuxer::get_video_coded_width() {
  if (video_dec_ctx) {
    return video_dec_ctx->coded_width;
  }
  return -1;
}
int DecoderDemuxer::get_video_coded_height() {
  if (video_dec_ctx) {
    return video_dec_ctx->coded_height;
  }
  return -1;
}

int64_t DecoderDemuxer::get_duration() {
  if (fmt_ctx)
    return fmt_ctx->duration + 5000;
  return 0;
}

extern "C" int decode_init_context(void** ctx, char* filepath) {
  DecoderDemuxer* dd = new DecoderDemuxer();
  if (!dd) {
    PRINTF_NO_MEMORY;
    return -1;
  }
  *ctx = dd;
  return dd->init(filepath, 0);
}

extern "C" void decode_deinit_context(void** ctx) {
  DecoderDemuxer* dd = (DecoderDemuxer*)*ctx;
  if (dd)
    delete dd;
  *ctx = NULL;
}

extern "C" int decode_one_video_frame(void* ctx,
                                      image_handle image_buf,
                                      int* width,
                                      int* height,
                                      int* coded_width,
                                      int* coded_height,
                                      enum AVPixelFormat* pix_fmt) {
  DecoderDemuxer* dd = (DecoderDemuxer*)ctx;
  AVFrame* frame = NULL;
  int ret = 0;
  if (!dd)
    return -1;
  frame = av_frame_alloc();
  if (frame) {
    int got_frame = 0;
    int is_video = 0;
    dd->set_audio_disable();
    while (!got_frame) {
      ret = dd->read_and_decode_one_frame(image_buf, frame, &got_frame,
                                          &is_video);
      if (dd->is_finished()) {
        ret = -1;
        break;
      }
    }
    if (got_frame && is_video) {
      *width = frame->width;
      *height = frame->height;
      *coded_width = dd->get_video_coded_width();
      *coded_height = dd->get_video_coded_height();
      *pix_fmt = dd->get_media_info().pixel_fmt;
      ret = 0;
    }
    av_frame_free(&frame);
  } else {
    PRINTF_NO_MEMORY;
    return -1;
  }
  return ret;
}

extern "C" int64_t decode_get_duration(void* ctx) {
  DecoderDemuxer* dd = (DecoderDemuxer*)ctx;
  if (dd)
    return dd->get_duration();
  return 0;
}
