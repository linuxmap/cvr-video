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

#ifndef PCM_RECEIVER_H
#define PCM_RECEIVER_H

#include <condition_variable>
#include <mutex>

extern "C" {
#include <libavutil/samplefmt.h>
}

#include "av_wrapper/raw_data_receiver.h"

class PCMReceiver : public RawDataReceiver {
 public:
  PCMReceiver();
  virtual ~PCMReceiver() {}
  virtual void AttachToSender() override;
  virtual void DetachFromSender() override;
  virtual void Receive(void* ptr, size_t size) override;

  // for reading. timeout: millisecond
  void ObtainBuffer(void*& ptr, size_t& size, int timeout);
  void ReleaseBuffer();

  void GetSourceParams(enum AVSampleFormat& fmt,
                       uint64_t& channel_layout,
                       int& sample_rate);

 private:
  enum State { WRITEABLE, READABLE, STATESUMS };
  class InternalBuffer {
   public:
    InternalBuffer();
    ~InternalBuffer();
    bool Alloc(size_t size);
    void* ptr_;
    size_t size_;
    State state_;
  };
  static const int loop_num = 2;
  InternalBuffer cycle_buffers_[loop_num];
  // [0]: available write index, [1]: available read index
  int idx_[STATESUMS];
  InternalBuffer* reading_buffer;
  std::mutex mtx_;
  std::condition_variable cond_;
};

#endif
