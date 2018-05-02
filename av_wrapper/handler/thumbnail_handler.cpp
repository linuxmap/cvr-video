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

#include "thumbnail_handler.h"

#include <fcntl.h>

extern "C" {
#include "fs_manage/fs_picture.h"
#include "fs_manage/fs_storage.h"
}

ThumbnailHandler::ThumbnailHandler() {
  file_path_[0] = 0;
}

int ThumbnailHandler::Init(MediaConfig config) {
  return ScaleEncodeHandler<MPPJpegEncoder>::Init(config, 2, 1,
                                                  kThumbnailWidth);
}

bool ThumbnailHandler::Process(int src_fd,
                               const VideoConfig& src_config,
                               const char* video_file_path) {
  if (!mtx_.try_lock()) {
    printf("THUMB: try lock yuv_buf_ get false.\n");
    return false;
  }
  fs_storage_thumbname_get(video_file_path, file_path_);
  printf("~~~ fs storage return thumbnail path: %s\n", file_path_);
  ScaleEncodeHandler<MPPJpegEncoder>::Process(src_fd, src_config);
  mtx_.unlock();
  return true;
}

void ThumbnailHandler::GetSrcConfig(const MediaConfig& src_config,
                                    int& src_w,
                                    int& src_h,
                                    PixelFormat& src_fmt) {
  src_w = src_config.jpeg_config.width;
  src_h = src_config.jpeg_config.height;
  src_fmt = src_config.jpeg_config.fmt;
}

int ThumbnailHandler::PrepareBuffers(MediaConfig& src_config,
                                     const int dst_numerator,
                                     const int dst_denominator,
                                     int& dst_w,
                                     int& dst_h) {
  int ret = ScaleEncodeHandler<MPPJpegEncoder>::PrepareBuffers(
      src_config, dst_numerator, dst_denominator, dst_w, dst_h);
  if (ret)
    return ret;
  src_config.jpeg_config.fmt = kCacheFmt;
  src_config.jpeg_config.width = yuv_buf_.width;
  src_config.jpeg_config.height = yuv_buf_.height;
  return 0;
}

void ThumbnailHandler::Work() {
  Buffer src(yuv_data_);
  Buffer dst(dst_data_);
  int ret = encoder_->Encode(&src, &dst);
  if (!ret) {
    int fd = fs_picture_open(file_path_, O_WRONLY | O_CREAT, 0666);
    if (fd < 0) {
      printf("fs open < %s > failed\n", file_path_);
      return;
    }
    ret = fs_picture_write(fd, dst.GetVirAddr(), dst.GetValidDataSize());
    if (ret <= 0)
      printf("fs_picture_write fail, ret=%d, errno=%d\n", ret, errno);
    fs_picture_close(fd);
  }
}
