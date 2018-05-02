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

#include "ff_demuxer.h"

#include <libavformat/avformat.h>

int get_file_duration(char* file_path, int64_t* duration) {
  AVFormatContext* fmt_ctx = NULL;
  av_register_all();
  *duration = -1;
  if (avformat_open_input(&fmt_ctx, file_path, NULL, NULL) < 0) {
    fprintf(stderr, "Could not open source file %s\n", file_path);
    return -1;
  }
  if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
    fprintf(stderr, "Could not find stream information\n");
    avformat_close_input(&fmt_ctx);
    return -1;
  }
  if (fmt_ctx->duration != AV_NOPTS_VALUE) {
#ifdef DEBUG
    int hours, mins, secs, us;
    int64_t time_len = fmt_ctx->duration + 5000;
    secs = time_len / AV_TIME_BASE;
    us = time_len % AV_TIME_BASE;
    mins = secs / 60;
    secs %= 60;
    hours = mins / 60;
    mins %= 60;
    fprintf(stderr, "%s time length: %02d:%02d:%02d.%02d\n", file_path, hours,
            mins, secs, (100 * us) / AV_TIME_BASE);
#endif
    *duration = fmt_ctx->duration + 5000;
  }
  avformat_close_input(&fmt_ctx);
  return 0;
}
