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

#include "audio_encode_handler.h"

#include <alsa/asoundlib.h>
#include <sys/prctl.h>
#include <algorithm>

#include "alsa_capture.h"
#include "encoder_muxing/encoder/ffmpeg_wrapper/ff_audio_encoder.h"
#include "encoder_muxing/packet_dispatcher.h"

#include <autoconfig/main_app_autoconf.h>

EncodedPacket* new_audio_packet(uint64_t audio_idx,
                                FFAudioEncoder* audio_encoder) {
  EncodedPacket* pkt = new EncodedPacket();
  if (!pkt) {
    printf("alloc EncodedPacket failed\n");
    return nullptr;
  }
  if (audio_encoder)
    audio_encoder->EncodeAudioFrame(&pkt->av_pkt);
  pkt->type = AUDIO_PACKET;
  pkt->is_phy_buf = false;
  pkt->audio_index = audio_idx;
  return pkt;
}

void delete_audio_packet(EncodedPacket* pkt) {
  pkt->unref();
}

int gen_mute_packet(FFAudioEncoder* audio_encoder, AVPacket& mute_packet) {
  void* audio_buf = nullptr;
  int nb_samples = 0;
  int channels = 0;
  enum AVSampleFormat fmt;
  audio_encoder->GetAudioBuffer(&audio_buf, &nb_samples, &channels, &fmt);
  // Produce a mute packet.
  av_init_packet(&mute_packet);
  assert(audio_buf);
  int buffer_size =
      av_samples_get_buffer_size(NULL, channels, nb_samples, fmt, 1);
  assert(buffer_size > 0);
  memset(audio_buf, 0, buffer_size);
  if (audio_encoder->EncodeAudioFrame(&mute_packet)) {
    av_packet_unref(&mute_packet);
    return -1;
  }
  return 0;
}

void* audio_record(void* arg) {
  AudioEncodeHandler* handler = (AudioEncodeHandler*)arg;
  FFAudioEncoder* audio_encoder = handler->audio_encoder_;
  struct alsa_capture* cap = handler->audio_capture_;
  int ret = 0;
  uint64_t audio_idx = 0;

  prctl(PR_SET_NAME, "audio_record", 0, 0, 0);

  // Produce a mute packet.
  AVPacket mute_packet;
  if ((ret = gen_mute_packet(audio_encoder, mute_packet)) < 0) {
    fprintf(stderr, "produce a mute packet failed\n");
    goto out;
  }

  while (1) {
    if (handler->encode_tid_ && !handler->audio_enable_) {
      bool finish = false;
      while (!finish) {
        AVPacket av_pkt;
        av_init_packet(&av_pkt);
        audio_encoder->EncodeFlushAudioData(&av_pkt, finish);
        PRINTF("==finish : %d\n", finish);
        if (!finish) {
          EncodedPacket* pkt = new_audio_packet(++audio_idx);
          if (!pkt) {
            av_packet_unref(&av_pkt);
            ret = -1;
            goto out;
          }
          pkt->av_pkt = av_pkt;
          handler->Dispatch(pkt);
          delete_audio_packet(pkt);
        }
      }
      break;
    }
    void* audio_buf = nullptr;
    int nb_samples = 0;
    int channels = 0;
    enum AVSampleFormat fmt;
    audio_encoder->GetAudioBuffer(&audio_buf, &nb_samples, &channels, &fmt);
    assert(audio_buf && nb_samples * channels > 0);
    cap->buffer_frames = nb_samples;
    int capture_samples = alsa_capture_doing(cap, audio_buf);
    if (capture_samples <= 0) {
      printf("capture_samples : %d, expect samples: %d\n", capture_samples,
             nb_samples);
      alsa_capture_done(cap);
      if (alsa_capture_open(cap, cap->dev_id))
        goto out;
      capture_samples = alsa_capture_doing(cap, audio_buf);
      if (capture_samples <= 0) {
        printf("there seems something wrong with audio capture, disable it\n");
        printf("capture_samples : %d, expect samples: %d\n", capture_samples,
               nb_samples);
        goto out;
      }
    }
    handler->SendRawData(audio_buf, av_samples_get_buffer_size(
                                        NULL, channels, nb_samples, fmt, 1));
    audio_idx++;

    EncodedPacket* pkt = nullptr;
    bool mute = handler->mute_;
    if (mute) {
#ifdef MAIN_APP_CACHE_ENCODEDATA_IN_MEM
      if (handler->EncPcmAlways()) {
        // Encode a video packet with real input.
        EncodedPacket* pkt = new_audio_packet(audio_idx, audio_encoder);
        if (!pkt) {
          ret = -1;
          goto out;
        }
        handler->DispatchToSpecial(pkt);
        delete_audio_packet(pkt);
      }
#endif
      pkt = new EncodedPacket();
      if (pkt) {
        pkt->type = AUDIO_PACKET;
        if (av_packet_ref(&pkt->av_pkt, &mute_packet)) {
          delete_audio_packet(pkt);
          ret = -1;
          goto out;
        }
        pkt->is_phy_buf = false;
        pkt->audio_index = audio_idx;
      }
    }
    if (!pkt)
      pkt = new_audio_packet(audio_idx, audio_encoder);
    if (!pkt) {
      ret = -1;
      goto out;
    }
    handler->Dispatch(pkt);
    if (!mute)
      handler->DispatchToSpecial(pkt);
    delete_audio_packet(pkt);
  }

out:
  PRINTF_FUNC_OUT;
  handler->audio_enable_ = false;
  av_packet_unref(&mute_packet);
  pthread_exit((void*)ret);
}

std::atomic_bool AudioEncodeHandler::recheck_enc_pcm_always_(false);

AudioEncodeHandler::AudioEncodeHandler(int dev_id,
                                       bool mute,
                                       pthread_attr_t* pth_attr)
    : dev_id_(dev_id),
      pth_attr_(pth_attr),
      audio_capture_(nullptr),
      mute_(mute),
      encode_tid_(0),
      audio_enable_(false),
      audio_encoder_(nullptr),
      enc_pcm_always_(false) {}

AudioEncodeHandler::~AudioEncodeHandler() {
  assert(!audio_enable_);
}

int AudioEncodeHandler::Init() {
  struct alsa_capture* cap = nullptr;
  MediaConfig mconfig;
  AudioConfig& aconfig = mconfig.audio_config;
  FFAudioEncoder* encoder = nullptr;
  pthread_t tid = 0;

  if (encode_tid_)
    return 0;

  cap = (struct alsa_capture*)calloc(1, sizeof(struct alsa_capture));
  if (!cap) {
    PRINTF_NO_MEMORY;
    goto err;
  }
  cap->sample_rate = MAIN_APP_AUDIO_SAMPLE_RATE;
  cap->format = SND_PCM_FORMAT_S16_LE;
  cap->dev_id = dev_id_;
  audio_capture_ = cap;

  if (alsa_capture_open(cap, dev_id_) == -1) {
    printf("alsa_capture_open err\n");
    goto err;
  }

  switch (cap->format) {
    case SND_PCM_FORMAT_S16_LE:
      aconfig.fmt = SAMPLE_FMT_S16;
      break;
    case SND_PCM_FORMAT_S32_LE:
      aconfig.fmt = SAMPLE_FMT_S32;
      break;
    default:
      printf("TODO: unsupport snd format : %d\n", cap->format);
      assert(0);
      goto err;
  }
  aconfig.bit_rate = 24000;
  aconfig.nb_samples = MAIN_APP_AUDIO_SAMPLE_NUM;
  aconfig.sample_rate = cap->sample_rate;
  aconfig.nb_channels = cap->channel;
  assert(cap->channel == 1 || cap->channel == 2);
  aconfig.channel_layout = av_get_default_channel_layout(cap->channel);

  mconfig.audio_config = aconfig;
  encoder = new FFAudioEncoder();
  if (!encoder)
    goto err;
  audio_encoder_ = encoder;
  if (0 != encoder->InitConfig(mconfig))
    goto err;

  if (pthread_create(&tid, pth_attr_, audio_record, this)) {
    printf("create audio pthread err\n");
    goto err;
  }
  audio_enable_ = true;
  encode_tid_ = tid;
  return 0;

err:
  Deinit();
  return -1;
}

void AudioEncodeHandler::Deinit() {
  if (encode_tid_) {
    audio_enable_ = false;
    pthread_join(encode_tid_, nullptr);
    encode_tid_ = 0;
  }
  if (audio_encoder_) {
    audio_encoder_->unref();
    audio_encoder_ = nullptr;
  }
  if (audio_capture_) {
    alsa_capture_done(audio_capture_);
    free(audio_capture_);
    audio_capture_ = nullptr;
  }
}

using PDListIterator = std::list<PacketDispatcher*>::iterator;

void AudioEncodeHandler::AddPacketDispatcher(PacketDispatcher* dispatcher) {
  std::lock_guard<std::mutex> _lk(mtx_);
  if (dispatcher_list_.empty()) {
    if (Init())
      return;
  }
  dispatcher_list_.push_back(dispatcher);
  PDListIterator i = std::find(unique_dispatcher_list_.begin(),
                               unique_dispatcher_list_.end(), dispatcher);
  if (i == unique_dispatcher_list_.end())
    unique_dispatcher_list_.push_back(dispatcher);

  printf("< %s >dispatcher_list_ size: %u, unique_dispatcher_list_ size: %u\n",
         __func__, dispatcher_list_.size(), unique_dispatcher_list_.size());
}

void AudioEncodeHandler::RmPacketDispatcher(PacketDispatcher* dispatcher) {
  std::lock_guard<std::mutex> _lk(mtx_);
  // Not initilzation yet
  if (dispatcher_list_.empty())
    return;
  PDListIterator i =
      std::find(dispatcher_list_.begin(), dispatcher_list_.end(), dispatcher);
  if (i != dispatcher_list_.end()) {
    PDListIterator first_element = i;
    i = std::find(++i, dispatcher_list_.end(), dispatcher);
    if (i == dispatcher_list_.end())
      unique_dispatcher_list_.remove(dispatcher);
    dispatcher_list_.erase(first_element);
  } else {
    return;
  }
  printf("< %s >dispatcher_list_ size: %u, unique_dispatcher_list_ size: %u\n",
         __func__, dispatcher_list_.size(), unique_dispatcher_list_.size());

  if (dispatcher_list_.empty() && receiver_list_.empty())
    Deinit();
}

void AudioEncodeHandler::AddRawReceiver(RawDataReceiver* receiver) {
  std::lock_guard<std::mutex> _lk(mtx_);
  if (receiver_list_.empty()) {
    if (Init())
      return;
  }
  receiver_list_.push_back(receiver);
}

void AudioEncodeHandler::RmRawReceiver(RawDataReceiver* receiver) {
  std::lock_guard<std::mutex> _lk(mtx_);
  receiver_list_.remove(receiver);
  if (dispatcher_list_.empty() && receiver_list_.empty())
    Deinit();
}

void AudioEncodeHandler::Dispatch(EncodedPacket* pkt) {
  if (mtx_.try_lock()) {
    for (PacketDispatcher* dispatcher : unique_dispatcher_list_)
      dispatcher->Dispatch(pkt);
    mtx_.unlock();
  }
}

void AudioEncodeHandler::DispatchToSpecial(EncodedPacket* pkt) {
  if (mtx_.try_lock()) {
    for (PacketDispatcher* dispatcher : unique_dispatcher_list_)
      dispatcher->DispatchToSpecial(pkt);
    mtx_.unlock();
  }
}

void AudioEncodeHandler::SendRawData(void* ptr, int size) {
  if (mtx_.try_lock()) {
    for (RawDataReceiver* receiver : receiver_list_)
      receiver->Receive(ptr, size);
    mtx_.unlock();
  }
}

bool AudioEncodeHandler::EncPcmAlways() {
  bool expect = true;
  if (recheck_enc_pcm_always_.compare_exchange_strong(expect, !expect)) {
    std::lock_guard<std::mutex> _lk(mtx_);
    enc_pcm_always_ = false;
    for (PacketDispatcher* dispatcher : unique_dispatcher_list_) {
      enc_pcm_always_ |= dispatcher->HasSpecialMuxer();
      printf("enc_pcm_always_ : %d\n", enc_pcm_always_);
      if (enc_pcm_always_)
        return true;
    }
  }
  return enc_pcm_always_;
}

void AudioEncodeHandler::GetDataParams(enum AVSampleFormat& fmt,
                                       uint64_t& channel_layout,
                                       int& sample_rate) {
    if (!audio_encoder_)
      return;
    AudioConfig &config = audio_encoder_->GetConfig().audio_config;
    fmt = FFContext::ConvertToAVSampleFmt(config.fmt);
    channel_layout = config.channel_layout;
    sample_rate = config.sample_rate;
}
