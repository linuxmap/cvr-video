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

#include "pcmreplay.h"

PCMReplay::PCMReplay(AudioPlayHandler* target_player)
    : PCMReceiver(), player_(target_player) {}

PCMReplay::~PCMReplay() {
  if (player_ && player_->Detached())
    delete player_;
}

void PCMReplay::AttachToSender() {
  PCMReceiver::AttachToSender();
  GetSourceParams(w_params_.fmt, w_params_.channel_layout,
                  w_params_.sample_rate);
}

void PCMReplay::Receive(void* ptr, size_t size) {
  int nb_samples =
      size / av_samples_get_buffer_size(
                 NULL,
                 av_get_channel_layout_nb_channels(w_params_.channel_layout), 1,
                 w_params_.fmt, 1);
  push_play_buffer(player_, (uint8_t*)ptr, size, &w_params_, nb_samples, 0);
}

extern AudioPlayHandler g_audio_play_handler;

extern "C" void* start_replay_from_mic(const char* dev) {
  AudioPlayHandler* target_player = nullptr;
  if (!strcmp(dev, "hw:0,0")) {
    target_player = &g_audio_play_handler;
  } else {
    target_player = new AudioPlayHandler(dev);
    if (!target_player)
      return nullptr;
    else
      target_player->SetDetach(true);
  }

  PCMReplay* replay = new PCMReplay(target_player);
  if (!replay && target_player->Detached()) {
    delete target_player;
    return nullptr;
  }
  replay->AttachToSender();
  return replay;
}

extern "C" void stop_replay_from_mic(void* replay) {
  if (!replay)
    return;
  PCMReplay* pcm_replay = (PCMReplay*)replay;
  pcm_replay->DetachFromSender();
  delete pcm_replay;
}
