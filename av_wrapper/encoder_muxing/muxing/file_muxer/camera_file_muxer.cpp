/**
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd
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

#include "camera_file_muxer.h"

CameraFileMuxer::CameraFileMuxer() {
#ifdef DEBUG
  snprintf(class_name, sizeof(class_name), "CameraFileMuxer");
#endif
  // default mp4
  snprintf(format, sizeof(format), "%s", "mp4");
  direct_piece = true;
  exit_id = MUXER_NORMAL_EXIT;
  no_async = true;  // no cache packet list and no thread handle
  memset(&lbr_pre_timeval, 0, sizeof(struct timeval));
  memset(&real_pre_timeval, 0, sizeof(struct timeval));
  low_frame_rate_mode = MODE_NORMAL;
}

CameraFileMuxer::~CameraFileMuxer() {}

bool CameraFileMuxer::is_low_frame_rate() {
  return low_frame_rate_mode != MODE_NORMAL;
}

bool CameraFileMuxer::set_low_frame_rate(uint32_t val,
                                         const timeval& start_timeval) {
  uint32_t old_mode = low_frame_rate_mode;
  if (val != MODE_NORMAL && low_frame_rate_mode == MODE_NORMAL) {
    lbr_pre_timeval = start_timeval;
  } else if (val == MODE_NORMAL) {
    if (is_low_frame_rate()) {
      low_frame_rate_mode = MODE_NORMAL_RELATIVE;
      return old_mode != low_frame_rate_mode;
    }
  }
  low_frame_rate_mode = val;
  return old_mode != low_frame_rate_mode;
}

void CameraFileMuxer::set_real_time(const struct timeval& start_timeval) {
  real_pre_timeval = start_timeval;
}

void CameraFileMuxer::reset_lbr_time() {
  if (is_low_frame_rate()) {
    memset(&lbr_pre_timeval, 0, sizeof(struct timeval));
    memset(&real_pre_timeval, 0, sizeof(struct timeval));
    low_frame_rate_mode = MODE_NORMAL;
  }
}

static void add_one_frame_time(struct timeval& val, int video_sample_rate) {
  int64_t usec = (int64_t)val.tv_usec + 1000000LL / video_sample_rate;
  val.tv_sec += (usec / 1000000LL);
  val.tv_usec = (usec % 1000000LL);
}

static void add_timeval(struct timeval& val, int64_t increment) {
  int64_t usec = (int64_t)val.tv_usec + increment;
  val.tv_sec += (usec / 1000000LL);
  val.tv_usec = (usec % 1000000LL);
}

void CameraFileMuxer::push_packet(EncodedPacket* pkt) {
  if (is_low_frame_rate()) {
    if (pkt->type == AUDIO_PACKET)
      return;
    // only save the key frame and readjust timestamp
    AVPacket& avpkt = pkt->av_pkt;
    if ((low_frame_rate_mode & MODE_ONLY_KEY_FRAME) &&
        !(avpkt.flags & AV_PKT_FLAG_KEY)) {
      if (lbr_pre_timeval.tv_sec == 0 && lbr_pre_timeval.tv_usec == 0) {
        pkt->lbr_time_val = pkt->time_val;
      } else {
        pkt->lbr_time_val = lbr_pre_timeval;
      }
      return;
    }

    struct timeval tval = pkt->time_val;
    if (low_frame_rate_mode & MODE_NORMAL_RELATIVE) {
      add_timeval(lbr_pre_timeval, difftime(real_pre_timeval, tval));
    } else {
      if ((lbr_pre_timeval.tv_sec == 0 && lbr_pre_timeval.tv_usec == 0) ||
          (((tval.tv_sec - lbr_pre_timeval.tv_sec) * 1000000LL +
            (tval.tv_usec - lbr_pre_timeval.tv_usec)) /
               1000 * video_sample_rate <
           1000))
        lbr_pre_timeval = tval;
      else
        add_one_frame_time(lbr_pre_timeval, video_sample_rate);
    }
    pkt->lbr_time_val = lbr_pre_timeval;
  }

  if (pkt->type == VIDEO_PACKET)
    real_pre_timeval = pkt->time_val;
  CameraMuxer::push_packet(pkt);
}

bool CameraFileMuxer::init_subtitle_stream(AVStream *stream) {
  int ret = 0;
  AVCodec *codec;
  AVCodecContext *c = nullptr;

  codec = avcodec_find_encoder(AV_CODEC_ID_MOV_TEXT);
  if (codec == nullptr)
    return false;

  c = stream->codec;
  if (c == nullptr)
    return false;

  c->codec_type = AVMEDIA_TYPE_SUBTITLE;
  c->codec_id = AV_CODEC_ID_MOV_TEXT;
  c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  c->subtitle_header = (uint8_t *)malloc(265);
  ret = avcodec_open2(c, codec, nullptr);
  if (ret < 0)
    return false;

  stream->codec = c;
  return true;
}
