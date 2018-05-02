// by wangh
#ifndef DECODER_DEMUXING_H
#define DECODER_DEMUXING_H

#include "../video_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <cvr_ffmpeg_shared.h>
#include <libavformat/avformat.h>

int decode_init_context(void** ctx, char* filepath);
void decode_deinit_context(void** ctx);
int decode_one_video_frame(void* ctx,
                           image_handle image_buf,
                           int* width,
                           int* height,
                           int* coded_width,
                           int* coded_height,
                           enum AVPixelFormat* pix_fmt);
int64_t decode_get_duration(void* ctx);
#ifdef __cplusplus
}
#endif

class DecoderDemuxer {
 private:
  AVFormatContext* fmt_ctx;
  media_info info;

  int video_stream_idx;
  AVStream* video_stream;
  AVCodecContext* video_dec_ctx;
  int video_frame_count;

  int audio_stream_idx;
  AVStream* audio_stream;
  AVCodecContext* audio_dec_ctx;
  int audio_frame_count;

#define AUDIO_DISABLE_FLAG (1 << 0)
#define VIDEO_DISABLE_FLAG (1 << 1)
  uint32_t disable_flag;
  bool finished;
  int decode_packet(AVPacket* pkt, AVFrame* frame, int* got_frame);

 public:
  DecoderDemuxer();
  ~DecoderDemuxer();
  int init(char* src_filepath, uint32_t disable_flag);
  int read_and_decode_one_frame(image_handle image_buf,  // in&out
                                AVFrame* out_frame,
                                int* got_frame,  // out
                                int* isVideo);
  media_info& get_media_info() { return info; }
  int rewind();  // rewind read
  bool is_finished() { return finished; }
  int get_video_coded_width();
  int get_video_coded_height();
  void set_audio_disable() { disable_flag |= AUDIO_DISABLE_FLAG; }
  int64_t get_duration();
};

#endif
