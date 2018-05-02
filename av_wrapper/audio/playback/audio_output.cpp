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

#define AUDIO_OUTPUT_CPP_
#include "audio_output.h"

#include <sys/prctl.h>

#include "video_common.h"

#include <autoconfig/main_app_autoconf.h>

static void notify_if_need(AudioFrame* in_frame) {
  if (in_frame && in_frame->user_reserve_buf[0]) {
    std::mutex* mtx = (std::mutex*)in_frame->user_reserve_buf[0];
    std::condition_variable* cond =
        (std::condition_variable*)in_frame->user_reserve_buf[1];
    std::unique_lock<std::mutex> _lk(*mtx);
    cond->notify_one();
  }
}

WantedAudioParams AudioPlayHandler::kWantedAudioParams = {
    AV_SAMPLE_FMT_S16, AV_CH_LAYOUT_STEREO, MAIN_APP_AUDIO_SAMPLE_RATE};

bool AudioPlayHandler::compare(WantedAudioParams& params) {
  return params.fmt != kWantedAudioParams.fmt ||
         params.channel_layout != kWantedAudioParams.channel_layout ||
         params.sample_rate != kWantedAudioParams.sample_rate;
}

AudioPlayHandler::AudioPlayHandler(const char* device)
    : tid_(0), run_(false), detached_(false) {
  memcpy(dev_, device, strlen(device) + 1);
  run_ = !StartThread(tid_, audio_play_routine, this);
  assert(run_);
}
AudioPlayHandler::~AudioPlayHandler() {
  {
    std::lock_guard<std::mutex> _lk(mutex_);
    run_ = false;
    cond_.notify_one();
  }
  StopThread(tid_);
  while (!frames_.empty()) {
    AudioFrame* frame = PopDataFrame();
    if (frame != &flush_flag_) {
      notify_if_need(frame);
      av_frame_free(&frame);
    }
  }
}

AudioFrame* AudioPlayHandler::PopDataFrame() {
  AudioFrame* frame = frames_.front();
  frames_.pop_front();
  return frame;
}

void AudioPlayHandler::PushDataFrame(AudioFrame* frame) {
  assert(frame == &flush_flag_ ||
         (frame->format == kWantedAudioParams.fmt &&
          frame->channel_layout == kWantedAudioParams.channel_layout &&
          frame->sample_rate == kWantedAudioParams.sample_rate));
  frames_.push_back(frame);
  if (frames_.size() > 30) {
    fprintf(stderr, "warning: too much audio frames waiting for playing!\n");
    for (auto it = frames_.begin(); it != frames_.end(); it++) {
      AudioFrame* f = *it;
      if (f != &flush_flag_) {
        frames_.erase(it);
        av_frame_free(&f);
        break;
      }
    }
  }
}

void AudioPlayHandler::LPushDataFrame(AudioFrame* frame) {
  std::lock_guard<std::mutex> _lk(mutex_);
  PushDataFrame(frame);
  cond_.notify_one();
}

void AudioPlayHandler::LPushFlushReq() {
  LPushDataFrame(&flush_flag_);
}

static AVFrame* convert_frame(struct SwrContext* swr,
                              AudioParams* audio_hw_params,
                              AVFrame& in_frame,
                              AVFrame& out_frame) {
  if (!swr)
    return &in_frame;
  // AVFrame should set the right member value before call this convert function
  // memset(&out_frame, 0, sizeof(out_frame));
  out_frame.format = audio_hw_params->fmt;
  out_frame.channel_layout = audio_hw_params->channel_layout;
  out_frame.channels = audio_hw_params->channels;
  out_frame.sample_rate = audio_hw_params->sample_rate;
  if (swr_convert_frame(swr, &out_frame, &in_frame)) {
    fprintf(stderr, "audio_frame_convert failed!\n");
    av_frame_unref(&out_frame);
    return nullptr;
  }
  return &out_frame;
}

static int out_buffer_to_dev(AudioParams* audio_hw_params,
                             uint8_t* buffer,
                             snd_pcm_sframes_t samples) {
  assert(buffer);
  assert(samples > 0);
  // printf("~out_buffer_to_dev samples : %ld\n", samples);
  while (samples > 0) {
    int status = audio_dev_write(audio_hw_params, buffer, &samples);
    if (status < 0)
      return -1;
    else if (status == 0)
      continue;
    buffer += (status * audio_hw_params->frame_size);
  }
  return 0;
}

static int flush_cache_to_dev(AudioParams* audio_hw_params,
                              uint8_t* const cache_buffer,
                              int& cache_offset) {
  int ret = 0;
  if (cache_offset > 0) {
    memset(cache_buffer + cache_offset, 0,
           audio_hw_params->audio_hw_buf_size - cache_offset);
    ret = out_buffer_to_dev(audio_hw_params, cache_buffer,
                            audio_hw_params->pcm_samples);
    cache_offset = 0;
  }
  return ret;
}

// fill cache, and output if full
// return size of processed data
static int fill_cache(AudioParams* audio_hw_params,
                      uint8_t* input_buffer,
                      int nb_samples,
                      uint8_t* const cache_buffer,
                      int& cache_offset) {
  int cache_free_samples = audio_hw_params->pcm_samples -
                           (cache_offset / audio_hw_params->frame_size);
  int copy_samples = 0;
  uint8_t* stream = nullptr;
  if (nb_samples < cache_free_samples) {
    copy_samples = nb_samples;
  } else {
    if (cache_offset == 0) {
      stream = input_buffer;
    } else {
      stream = cache_buffer;
      copy_samples = cache_free_samples;
    }
    nb_samples = cache_free_samples;
  }

  if (copy_samples) {
    // printf("copy_samples : %d\n", copy_samples);
    int copy_length = copy_samples * audio_hw_params->frame_size;
    memcpy(cache_buffer + cache_offset, input_buffer, copy_length);
    cache_offset += copy_length;
    if (cache_offset == audio_hw_params->audio_hw_buf_size)
      cache_offset = 0;
  }

  if (stream &&
      out_buffer_to_dev(audio_hw_params, stream, audio_hw_params->pcm_samples))
    return -1;

  return nb_samples;
}

static int out_to_dev(AVFrame* frame,
                      AudioParams* audio_hw_params,
                      uint8_t* const cache_buffer,
                      int& cache_offset) {
  uint8_t* input_buffer = frame->extended_data[0];
  int nb_samples = frame->nb_samples;
  while (nb_samples > 0) {
    // printf("process nb_samples: %d\n", nb_samples);
    int processed_samples = fill_cache(audio_hw_params, input_buffer,
                                       nb_samples, cache_buffer, cache_offset);
    // printf("fill_cache processed_samples: %d\n\n", processed_samples);
    if (processed_samples < 0) {
      // reset
      cache_offset = 0;
      return processed_samples;
    }
    nb_samples -= processed_samples;
    input_buffer += (processed_samples * audio_hw_params->frame_size);
  }
  return 0;
}

static void* audio_play_routine(void* arg) {
  AudioPlayHandler* handler = static_cast<AudioPlayHandler*>(arg);
  prctl(PR_SET_NAME, __FUNCTION__, 0, 0, 0);
  if (!handler)
    return nullptr;
  AudioParams audio_hw_params = {AV_SAMPLE_FMT_S16};
  uint8_t* cache_buffer = nullptr;
  int cache_offset = 0;

  struct SwrContext* swr = nullptr;
  AudioFrame* in_frame = nullptr;
  while (handler->run_) {
    {
      std::chrono::milliseconds msec(3000);
      std::unique_lock<std::mutex> _lk(handler->mutex_);
      if (handler->frames_.empty()) {
        if (handler->cond_.wait_for(_lk, msec) == std::cv_status::timeout) {
          // printf("no audio play request in %lld ms!\n", msec.count());
          audio_dev_close(&audio_hw_params);
          audio_deinit_swr_context(swr);
          if (cache_buffer) {
            free(cache_buffer);
            cache_buffer = nullptr;
            cache_offset = 0;
          }
        }
        continue;
      }
      in_frame = handler->PopDataFrame();
    }
    if (audio_dev_open(AudioPlayHandler::kWantedAudioParams.fmt,
                       AudioPlayHandler::kWantedAudioParams.channel_layout,
                       AudioPlayHandler::kWantedAudioParams.sample_rate,
                       &audio_hw_params, handler->dev_)) {
      fprintf(stderr, "open %s failed!!\n", handler->dev_);
      break;
    }
    if (!cache_buffer) {
      uint8_t* buffer =
          (uint8_t*)realloc(cache_buffer, audio_hw_params.audio_hw_buf_size);
      if (!buffer) {
        fprintf(stderr, "no memory for cache_buffer!!\n");
        break;
      } else {
        cache_buffer = buffer;
      }
      cache_offset = 0;
    }
    if (in_frame == &handler->flush_flag_) {
      // do flush
      if (flush_cache_to_dev(&audio_hw_params, cache_buffer, cache_offset))
        fprintf(stderr, "flush audio error!\n");
      continue;
    }
    if (!swr) {
      WantedAudioParams params = {audio_hw_params.fmt,
                                  audio_hw_params.channel_layout,
                                  audio_hw_params.sample_rate};
      if (AudioPlayHandler::compare(params)) {
        swr = audio_init_swr_context(AudioPlayHandler::kWantedAudioParams,
                                     params);
        if (!swr)
          break;
      }
    }
    AVFrame out_frame = {0};
    AVFrame* frame = convert_frame(swr, &audio_hw_params, *in_frame, out_frame);
    if (frame &&
        out_to_dev(frame, &audio_hw_params, cache_buffer, cache_offset)) {
      fprintf(stderr, "write frame error!\n");
      // close device?
      // assert(0);
    }
    notify_if_need(in_frame);
    av_frame_free(&in_frame);
    av_frame_unref(&out_frame);
  }

  if (in_frame != &handler->flush_flag_) {
    notify_if_need(in_frame);
    av_frame_free(&in_frame);
  }
  audio_dev_close(&audio_hw_params);
  if (cache_buffer)
    free(cache_buffer);
  audio_deinit_swr_context(swr);
  handler->run_ = false;
  return nullptr;
}

AudioPlayHandler g_audio_play_handler("hw:0,0");

extern "C" {
#include <libavutil/time.h>
}

static void push_play_frame(AudioPlayHandler* player,
                            AudioFrame* frame,
                            int block) {
  if (unlikely(!player->IsRunning())) {
    if (block)
      av_usleep(frame->nb_samples * 1000000LL / frame->sample_rate);
    return;
  }
  std::mutex mtx;
  std::condition_variable cond;
  AudioFrame* new_frame = av_frame_clone(frame);
  if (unlikely(!new_frame))
    return;
  assert(!new_frame->user_reserve_buf[0]);
  assert(!new_frame->user_reserve_buf[1]);
  if (block) {
    assert(sizeof(&mtx) <= sizeof(uint32_t));
    new_frame->user_reserve_buf[0] = (uint32_t)&mtx;
    assert(sizeof(&cond) <= sizeof(uint32_t));
    new_frame->user_reserve_buf[1] = (uint32_t)&cond;
    std::unique_lock<std::mutex> _lk(mtx);
    player->LPushDataFrame(new_frame);
    cond.wait(_lk);
  } else {
    player->LPushDataFrame(new_frame);
  }
}

void push_play_buffer(AudioPlayHandler* player,
                      uint8_t* buffer,
                      size_t size,
                      WantedAudioParams* w_params,
                      int nb_samples,
                      int block) {
  AudioFrame frame = {0};
  frame.format = w_params->fmt;
  frame.sample_rate = w_params->sample_rate;
  frame.channel_layout = w_params->channel_layout;
  frame.channels = av_get_channel_layout_nb_channels(frame.channel_layout);
  frame.nb_samples = nb_samples;
  frame.extended_data = frame.data;
  frame.extended_data[0] = buffer;
  push_play_frame(player, &frame, block);
}

extern "C" void default_push_play_frame(AudioFrame* frame, int block) {
  push_play_frame(&g_audio_play_handler, frame, block);
}

extern "C" void default_push_play_buffer(uint8_t* buffer,
                                         size_t size,
                                         WantedAudioParams* w_params,
                                         int nb_samples,
                                         int block) {
  push_play_buffer(&g_audio_play_handler, buffer, size, w_params, nb_samples,
                   block);
}

extern "C" void default_flush_play() {
  g_audio_play_handler.LPushFlushReq();
}

extern "C" WantedAudioParams* default_wanted_audio_params() {
  return &AudioPlayHandler::kWantedAudioParams;
}

extern "C" void default_audio_params(AudioParams* params) {
  WantedAudioParams* wap = default_wanted_audio_params();
  params->fmt = wap->fmt;
  params->channel_layout = wap->channel_layout;
  params->sample_rate = wap->sample_rate;
  params->channels = av_get_channel_layout_nb_channels(wap->channel_layout);
  params->frame_size =
      av_samples_get_buffer_size(NULL, params->channels, 1, params->fmt, 1);
  params->bytes_per_sec = av_samples_get_buffer_size(
      NULL, params->channels, params->sample_rate, params->fmt, 1);
  params->wanted_params = *wap;
  params->pcm_handle = nullptr;
  params->pcm_samples = MAIN_APP_AUDIO_SAMPLE_NUM;
  params->pcm_format = audio_convert_fmt(wap->fmt);
  params->audio_hw_buf_size =
      (params->channels * params->pcm_samples *
       snd_pcm_format_physical_width(params->pcm_format) / 8);
  params->period_size = 0;
}
