// by wangh
#ifndef CAMERA_TS_H
#define CAMERA_TS_H

#include "../camera_muxer.h"

class CameraTs : public CameraMuxer {
 private:
  enum { ENTER, RUNNING, PAUSING, EXIT };

  volatile int state_;
  int ext_video_fd_;
  int ext_audio_fd_;
  int fatal_error_;
  CameraTs();

  int OpenLivePipe(const char* pipe_path);
  int PipeWriteSpsPps(const uint8_t* spspps,
                      size_t spspps_size,
                      const timeval& time,
                      const int start_len);

 public:
  ~CameraTs();
  int init_uri(char* uri, int rate) override;
  void stop_current_job() override;
  int muxer_write_header(AVFormatContext* oc, char* url) override;
  int muxer_write_tailer(AVFormatContext* oc) override;
  int muxer_write_free_packet(MuxProcessor* process,
                              EncodedPacket* pkt) override;
  static int OpenPipe(const char* pipe_path);
  static void ClosePipe(int& fd);
  bool Running() { return state_ == RUNNING; }
  void SetRunning() { state_ = RUNNING; }
  void SetPausing() { state_ = PAUSING; }
  bool StartByOutside() { return ext_video_fd_ >= 0; }
  int Error() { return fatal_error_; }
  CREATE_FUNC(CameraTs)
};

#endif  // CAMERA_TS_H
