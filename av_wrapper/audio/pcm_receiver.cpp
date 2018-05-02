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

#include "pcm_receiver.h"

#include "av_wrapper/handler/audio_encode_handler.h"

extern AudioEncodeHandler global_audio_ehandler;

PCMReceiver::PCMReceiver() : reading_buffer(nullptr) {
  idx_[WRITEABLE] = idx_[READABLE] = 0;
}

void PCMReceiver::AttachToSender() {
  global_audio_ehandler.AddRawReceiver(this);
}

void PCMReceiver::DetachFromSender() {
  global_audio_ehandler.RmRawReceiver(this);
}

void PCMReceiver::GetSourceParams(enum AVSampleFormat& fmt,
                                  uint64_t& channel_layout,
                                  int& sample_rate) {
  global_audio_ehandler.GetDataParams(fmt, channel_layout, sample_rate);
}

void PCMReceiver::Receive(void* ptr, size_t size) {
  std::lock_guard<std::mutex> _lk(mtx_);
  int writeable_idx = idx_[WRITEABLE];
  InternalBuffer* buf = &cycle_buffers_[writeable_idx];
  if (buf->state_ != WRITEABLE) {
    printf("warning: seems speech recognition process too slow\n");
    return;
  }
  buf->Alloc(size);
  memcpy(buf->ptr_, ptr, size);
  buf->state_ = READABLE;
  writeable_idx++;
  idx_[WRITEABLE] = writeable_idx % STATESUMS;
  cond_.notify_one();
}

void PCMReceiver::ObtainBuffer(void*& ptr, size_t& size, int timeout) {
  std::unique_lock<std::mutex> _lk(mtx_);
  std::chrono::milliseconds msec(timeout);
  reading_buffer = &cycle_buffers_[idx_[READABLE]];
  do {
    if (reading_buffer->state_ == READABLE) {
      ptr = reading_buffer->ptr_;
      size = reading_buffer->size_;
      break;
    }
    if (cond_.wait_for(_lk, msec) == std::cv_status::timeout) {
      reading_buffer = nullptr;
      size = 0;
      break;
    }
  } while (true);
}

void PCMReceiver::ReleaseBuffer() {
  std::unique_lock<std::mutex> _lk(mtx_);
  if (reading_buffer) {
    reading_buffer->state_ = WRITEABLE;
    int readable_idx = idx_[READABLE];
    readable_idx++;
    idx_[READABLE] = readable_idx % STATESUMS;
  }
}

PCMReceiver::InternalBuffer::InternalBuffer()
    : ptr_(nullptr), size_(0), state_(WRITEABLE) {}

PCMReceiver::InternalBuffer::~InternalBuffer() {
  if (ptr_)
    free(ptr_);
}

bool PCMReceiver::InternalBuffer::Alloc(size_t size) {
  if (ptr_) {
    assert(size == size_);
    return true;
  }
  ptr_ = malloc(size);
  if (ptr_) {
    size_ = size;
    return true;
  }
  return false;
}
