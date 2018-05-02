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

#include "video_common.h"

extern "C" {
#include <libavutil/time.h>
}

void video_object::ref() {
  __atomic_add_fetch(&ref_cnt, 1, __ATOMIC_SEQ_CST);
}
int video_object::unref() {
  if (!__atomic_sub_fetch(&ref_cnt, 1, __ATOMIC_SEQ_CST)) {
    if (release_cb)
      release_cb();
    delete this;
    return 0;
  }
  return 1;
}
int video_object::refcount() {
  return __atomic_load_n(&ref_cnt, __ATOMIC_SEQ_CST);
}

EncodedPacket::EncodedPacket() {
  av_init_packet(&av_pkt);
  type = PACKET_TYPE_COUNT;
  memset(&time_val, 0, sizeof(struct timeval));
  audio_index = 0;
  is_phy_buf = false;
  memset(&lbr_time_val, 0, sizeof(lbr_time_val));
}

EncodedPacket::~EncodedPacket() {
  // printf("pkt: %p, free av_pkt: %d\n", this, av_pkt.size);
  av_packet_unref(&av_pkt);
}

int EncodedPacket::copy_av_packet() {
  assert(is_phy_buf);
  AVPacket pkt = av_pkt;
  if (0 != av_copy_packet(&av_pkt, &pkt)) {
    printf("in %s, av_copy_packet failed!\n", __func__);
    return -1;
  }
  is_phy_buf = false;
  return 0;
}

int StartThread(pthread_t& tid, void* (*start_routine)(void*), void* arg) {
  if (tid)
    return -1;
  int ret = 0;
  pthread_attr_t attr;
  if (pthread_attr_init(&attr)) {
    printf("pthread_attr_init failed!\n");
    return -1;
  }

  static int stacksize = 64 * 1024;
  if (pthread_attr_setstacksize(&attr, stacksize)) {
    printf("pthread_attr_setstacksize <%d> failed!\n", stacksize);
    ret = -1;
    goto out;
  }

  if (pthread_create(&tid, NULL, start_routine, arg)) {
    printf("pthread create failed!\n");
    ret = -1;
  }
out:
  if (pthread_attr_destroy(&attr))
    printf("pthread_attr_destroy failed!\n");
  return ret;
}

void StopThread(pthread_t& tid) {
  if (tid) {
    pthread_join(tid, nullptr);
    tid = 0;
  }
}
