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

#ifndef CAMERACACHEMUXER_H
#define CAMERACACHEMUXER_H

#include "handler/thumbnail_handler.h"
#include "encoder_muxing/muxing/camera_muxer.h"

template <class Muxer>
class CameraCacheMuxer : public CameraMuxer {
 public:
  static const int kCacheDuration = 5;  // seconds

  virtual ~CameraCacheMuxer() {
    if (muxer_)
      delete muxer_;
  }

  void SetCacheDuration(int duration) {
    setMaxVideoNum(duration * getVSampleRate());
    cache_duration_ = duration;
    write_enable = true;
  }

  int start_new_job(pthread_attr_t* attr) { return 0; }

  void push_packet(EncodedPacket* pkt) override {
    std::lock_guard<std::mutex> _lk(mutex_);
    if (muxer_ && muxer_is_ready_) {
      if ((pkt->type == VIDEO_PACKET) &&
          difftime(store_moment_, pkt->time_val) >
              cache_duration_ * 1000000LL) {
#ifdef DEBUG
        printf("cache_muxer stop time : %ld s\n", pkt->time_val.tv_sec);
#endif
        muxer_->stop_current_job();
        delete muxer_;
        muxer_ = nullptr;
        thumbnail_gen_ = false;
        muxer_is_ready_ = false;
        BaseVideoEncoder* venc =
            static_cast<BaseVideoEncoder*>(get_video_encoder());
        if (venc)
          venc->SetForceIdrFrame();  // Make the next frame as I Frame.
        return;
      }
      muxer_->push_packet(pkt);
    } else {
      CameraMuxer::push_packet(pkt);
    }
  }

  void Stop() {
    CameraMuxer::stop_current_job();
    if (muxer_) {
      muxer_->stop_current_job();
      delete muxer_;
      muxer_ = nullptr;
      thumbnail_gen_ = false;
      muxer_is_ready_ = false;
    }
  }

  void stop_current_job() override {
    std::lock_guard<std::mutex> _lk(mutex_);
    Stop();
  }

  void StopCurrentJobImmediately() {
    std::lock_guard<std::mutex> _lk(mutex_);
    if (muxer_)
      muxer_->set_exit_id(MUXER_IMMEDIATE_EXIT);
    Stop();
  }

  void StoreCacheData(char* filename, pthread_attr_t* thread_attr) {
    if (muxer_)
      return;
    snprintf(file_path_, sizeof(file_path_), "%s", filename);
    muxer_ = Muxer::create();
    if (!muxer_) {
      printf("create muxer for cachedata fail\n");
      return;
    }
    assert(format[0]);
    muxer_->set_out_format(format);
    muxer_->set_async(true);  // set async
    muxer_->set_encoder(get_video_encoder(), get_audio_encoder());
    if (muxer_->init_uri(filename, getVSampleRate()) ||
        muxer_->start_new_job(thread_attr)) {
      printf("start muxer for cachedata fail\n");
      delete muxer_;
      muxer_ = nullptr;
      return;
    }
    muxer_->setMaxVideoNum(2 * cache_duration_ * getVSampleRate());
#ifdef DEBUG
    // Note: The value of sec should be satisfied sec % (gop/fps) == 0.
    VideoConfig& vconfig = get_video_encoder()->GetConfig().video_config;
    if (cache_duration_ % (vconfig.gop_size / vconfig.frame_rate) != 0)
      printf(
          "Warning: cache_duration%%(gop/fps) [%d%%(%d/%d)]not equals 0,"
          "the duration of final event video may less than expectation\n",
          cache_duration_, vconfig.gop_size, vconfig.frame_rate);
#endif
    gettimeofday(&store_moment_, NULL);
    thumbnail_gen_ = true;
    EncodedPacket* pkt = nullptr;
    mutex_.lock();
#ifdef DEBUG
    bool got_first_vpacket = false;
#endif
    while ((pkt = pop_packet()) != nullptr) {
      muxer_->push_packet(pkt);
      if (pkt->type == VIDEO_PACKET) {
#ifdef DEBUG
        if (!got_first_vpacket) {
          printf("cache_muxer first pkt time : %ld s\n", pkt->time_val.tv_sec);
          got_first_vpacket = true;
        }
#endif
        store_moment_ = pkt->time_val;
      }
      pkt->unref();
    }
#ifdef DEBUG
    printf("cache_muxer trigger time : %ld s\n", store_moment_.tv_sec);
#endif
    muxer_is_ready_ = true;
    mutex_.unlock();
  }

  void TryGenThumbnail(ThumbnailHandler& tn_handler,
                       int src_fd,
                       const VideoConfig& src_config) {
    if (muxer_is_ready_ && thumbnail_gen_)
      thumbnail_gen_ = !tn_handler.Process(src_fd, src_config, file_path_);
  }

  inline Muxer* GetWorkingMuxer() { return muxer_; }

  int init() override { return 0; }
  CREATE_FUNC(CameraCacheMuxer)

 protected:
  CameraCacheMuxer()
      : muxer_(nullptr),
        muxer_is_ready_(false),
        cache_duration_(0),
        thumbnail_gen_(false) {
#ifdef DEBUG
    snprintf(class_name, sizeof(class_name), "CameraCacheMuxer");
#endif
    format[0] = 0;
    run_func = NULL;

    file_path_[0] = 0;
  }

 private:
  char file_path_[128];
  Muxer* volatile muxer_;
  bool muxer_is_ready_;
  struct timeval store_moment_;
  int cache_duration_;
  std::mutex mutex_;
  bool thumbnail_gen_;
};

#endif  // CAMERACACHEMUXER_H
