// by wangh
#ifndef H264_ENCODER_H
#define H264_ENCODER_H

#ifndef DISABLE_FF_MPP
#include "ffmpeg_wrapper/ff_mpp_h264_encoder.h"
typedef FFMPPH264Encoder H264Encoder;
#else
#include "mpp_h264_encoder.h"
#include "ffmpeg_wrapper/ff_context.h"
typedef MPPH264Encoder H264Encoder;
#endif

#endif  // H264_ENCODER_H
