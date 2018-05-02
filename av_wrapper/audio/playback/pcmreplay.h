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

#ifndef PCMREPLAY_H
#define PCMREPLAY_H

#ifdef __cplusplus
#include "audio/pcm_receiver.h"
#include "audio_output.h"

class PCMReplay : public PCMReceiver {
 public:
  PCMReplay(AudioPlayHandler* target_player);
  ~PCMReplay();
  virtual void AttachToSender() override;
  // Unlikely speechrec,
  // capture thread push the data to player directly.
  virtual void Receive(void* ptr, size_t size) override;

 private:
  AudioPlayHandler* player_;
  WantedAudioParams w_params_;
};

extern "C" {
#endif

// dev: pattern like hw:0,0
void* start_replay_from_mic(const char* dev);
void stop_replay_from_mic(void* replay);

#ifdef __cplusplus
}
#endif

#endif  // PCMREPLAY_H
