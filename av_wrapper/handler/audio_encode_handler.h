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

#ifndef AUDIO_ENCODE_HANDLER_H
#define AUDIO_ENCODE_HANDLER_H

#include <pthread.h>

#include <atomic>
#include <list>

#include "raw_data_receiver.h"
#include "video_common.h"

#include "encoder_muxing/encoder/base_encoder.h"

struct alsa_capture;
class FFAudioEncoder;
class PacketDispatcher;

void* audio_record(void* arg);

class AudioEncodeHandler {
 public:
  // Recheck the following enc_pcm_always_.
  static std::atomic_bool recheck_enc_pcm_always_;
  AudioEncodeHandler(int dev_id, bool mute, pthread_attr_t* pth_attr);
  ~AudioEncodeHandler();
  void AddPacketDispatcher(PacketDispatcher* dispatcher);
  void RmPacketDispatcher(PacketDispatcher* dispatcher);
  void AddRawReceiver(RawDataReceiver* receiver);
  void RmRawReceiver(RawDataReceiver* receiver);
  void SetMute(bool val) { mute_ = val; }
  FFAudioEncoder* GetEncoder() const { return audio_encoder_; }
  bool EncPcmAlways();

  void GetDataParams(enum AVSampleFormat& fmt,
                     uint64_t& channel_layout,
                     int& sample_rate);

 private:
  int dev_id_;
  pthread_attr_t* pth_attr_;
  struct alsa_capture* audio_capture_;
  bool mute_;
  pthread_t encode_tid_;
  bool audio_enable_;
  FFAudioEncoder* audio_encoder_;
  std::list<PacketDispatcher*> dispatcher_list_;
  std::list<PacketDispatcher*> unique_dispatcher_list_;
  std::list<RawDataReceiver*> receiver_list_;
  std::mutex mtx_;
  // For collision, encode pcm from microphone always.
  volatile bool enc_pcm_always_;

  int Init();
  void Deinit();
  void Dispatch(EncodedPacket* pkt);
  void DispatchToSpecial(EncodedPacket* pkt);
  void SendRawData(void* ptr, int size);

  friend void* audio_record(void* arg);
};

EncodedPacket* new_audio_packet(uint64_t audio_idx,
                                FFAudioEncoder* aac_encoder = nullptr);
void delete_audio_packet(EncodedPacket* pkt);
int gen_mute_packet(FFAudioEncoder* aac_encoder, AVPacket& mute_packet);

#endif  // AUDIO_ENCODE_HANDLER_H
