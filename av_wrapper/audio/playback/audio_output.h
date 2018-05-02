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

#ifndef AUDIO_OUTPUT_H_
#define AUDIO_OUTPUT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "audio_dev.h"

// Ffmpeg is enable
#include <libavutil/frame.h>
typedef AVFrame AudioFrame;

void default_push_play_frame(AudioFrame* frame, int block);
void default_push_play_buffer(uint8_t* buffer,
                              size_t size,
                              WantedAudioParams* w_params,
                              int nb_samples,
                              int block);
void default_flush_play();
WantedAudioParams* default_wanted_audio_params();
void default_audio_params(AudioParams* params);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>

#ifdef AUDIO_OUTPUT_CPP_
static void* audio_play_routine(void* arg);
#endif

class AudioPlayHandler {
 public:
  static WantedAudioParams kWantedAudioParams;

  static bool compare(WantedAudioParams& params);
  AudioPlayHandler(const char* device);
  ~AudioPlayHandler();
  inline bool IsRunning() { return run_; }
  bool Detached() { return detached_; }
  void SetDetach(bool val) { detached_ = val; }
  AudioFrame* PopDataFrame();
  void PushDataFrame(AudioFrame*);
  void LPushDataFrame(AudioFrame*);  // with locking
  void LPushFlushReq();

  friend void* audio_play_routine(void* arg);

 private:
  char dev_[32];
  pthread_t tid_;
  std::mutex mutex_;
  std::condition_variable cond_;
  // AVFrame in stack could not be pass into container,
  // as extended_data may equal data whose lifecyle is same to AVFrame.
  // Using AVFrame in heap.
  std::list<AudioFrame*> frames_;
  AudioFrame flush_flag_;
  volatile bool run_;
  bool detached_;
};

void push_play_buffer(AudioPlayHandler* player,
                      uint8_t* buffer,
                      size_t size,
                      WantedAudioParams* w_params,
                      int nb_samples,
                      int block);
#endif

#endif  // #ifndef AUDIO_OUTPUT_H_
