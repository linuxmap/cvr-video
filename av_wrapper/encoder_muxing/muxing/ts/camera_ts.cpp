// by wangh
#include "camera_ts.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>

#include <encoder_muxing/encoder/base_encoder.h>

CameraTs::CameraTs()
    : state_(RUNNING), ext_video_fd_(-1), ext_audio_fd_(-1), fatal_error_(0) {
#ifdef DEBUG
  snprintf(class_name, sizeof(class_name), "CameraTs");
#endif
  snprintf(format, sizeof(format), "mpegts");
  exit_id = MUXER_IMMEDIATE_EXIT;
}

CameraTs::~CameraTs() {
  state_ = EXIT;
  ClosePipe(ext_video_fd_);
  ClosePipe(ext_audio_fd_);
}

int CameraTs::OpenPipe(const char* pipe_path) {
  int fd = -1;
  if (pipe_path) {
    // If do not exist, create it.
    if (access(pipe_path, F_OK) == -1 && mkfifo(pipe_path, 0777) != 0) {
      printf("mkfifo %s failed, errno: %d\n", pipe_path, errno);
      return -1;
    }
    fd = open(pipe_path, O_RDWR);
    if (fd < 0)
      printf("open %s fail, errno: %d\n", pipe_path, errno);
  }
  return fd;
}

void CameraTs::ClosePipe(int& fd) {
  if (fd >= 0 && close(fd))
    fprintf(stderr, "close fd <%d> failed, errno: %d\n", fd, errno);
  fd = -1;
}

// TODO: move to wifi_management.c?
#define AUDIO_FIFO_NAME "/tmp/rv110x_audio_fifo"

int CameraTs::OpenLivePipe(const char* pipe_path) {
  ext_video_fd_ = OpenPipe(pipe_path);
  ext_audio_fd_ = OpenPipe(AUDIO_FIFO_NAME);
  if (ext_video_fd_ >= 0 && ext_audio_fd_ >= 0) {
    return 0;
  } else {
    state_ = EXIT;
    ClosePipe(ext_video_fd_);
    ClosePipe(ext_audio_fd_);
    return -1;
  }
}

int CameraTs::init_uri(char* uri, int rate) {
  // av_log_set_level(AV_LOG_TRACE);
  if (!strncmp(uri, "rtsp", 4)) {
    snprintf(format, sizeof(format), "rtsp");
  } else if (!strncmp(uri, "rtp", 3)) {
    snprintf(format, sizeof(format), "rtp_mpegts");
  } else if (!strncmp(uri, "pipe", 4)) {
    char* pipe_path = uri + 7;  // pipe://
    if (OpenLivePipe(pipe_path) < 0)
      return -1;
    sprintf(uri, "pipe:%d&%d", ext_video_fd_, ext_audio_fd_);
    snprintf(format, sizeof(format), "h264");
    state_ = ENTER;
  } else if (!strncmp(uri, "rtmp", 4)) {
    snprintf(format, sizeof(format), "flv");
  }

  int ret = CameraMuxer::init_uri(uri, rate);
  max_video_num = rate;  // 1 seconds
  return ret;
}

void CameraTs::stop_current_job() {
  CameraMuxer::stop_current_job();
  free_packet_list();
}

static inline ssize_t pipe_write(int fd, void* buf, size_t buf_size) {
  size_t remain_size = buf_size;
  do {
    ssize_t ret = write(fd, buf, remain_size);
    if (ret != static_cast<ssize_t>(remain_size) && errno != EAGAIN) {
      printf(
          "write pipe failed, expectly write %u bytes, really write %u "
          "bytes\n",
          buf_size, buf_size - remain_size);
      return -1;
    }
    assert(ret >= 0);
    remain_size -= ret;
  } while (remain_size > 0);
  return buf_size;
}

int CameraTs::PipeWriteSpsPps(const uint8_t* spspps,
                              size_t spspps_size,
                              const struct timeval& time,
                              const int start_len /* 00 00 01 */) {
  assert(spspps_size > 0);
  if (!spspps || spspps_size <= 0)
    return -1;
  const uint8_t* p = spspps;
  const uint8_t* end = p + spspps_size;
  const uint8_t *nal_start = nullptr, *nal_end = nullptr;
  nal_start = find_h264_startcode(p, end);
  for (;;) {
    // while (nal_start < end && !*(nal_start++));
    if (nal_start == end)
      break;
    nal_start += start_len;
    nal_end = find_h264_startcode(nal_start, end);
    unsigned size = nal_end - nal_start + start_len;
    uint8_t nal_type = (*nal_start) & 0x1F;
    if (nal_type == 7 || nal_type == 8) {
      if (pipe_write(ext_video_fd_, (void*)&time, sizeof(time)) < 0)
        return -1;
      if (pipe_write(ext_video_fd_, &size, sizeof(size)) < 0)
        return -1;
      if (pipe_write(ext_video_fd_, (void*)(nal_start - start_len), size) < 0)
        return -1;
    }
    nal_start = nal_end;
  }
  return 0;
}

int CameraTs::muxer_write_header(AVFormatContext* oc, char* url) {
  if (ext_video_fd_ < 0)
    return CameraMuxer::muxer_write_header(oc, url);
  // Do the writting by ourself.
  for (uint i = 0; i < oc->nb_streams; i++) {
    AVStream* st = oc->streams[i];
    AVCodecContext* codec = st ? st->codec : nullptr;
    if (codec && codec->codec_id == AV_CODEC_ID_H264) {
      struct timeval time = {0, 0};
      gettimeofday(&time, nullptr);
      if (!PipeWriteSpsPps(codec->extradata, codec->extradata_size, time, 3))
        return 0;
    }
  }
  fprintf(stderr, "Error: could not get the sps pps info\n");
  return -1;
}

int CameraTs::muxer_write_tailer(AVFormatContext* oc) {
  if (ext_video_fd_ < 0)
    return CameraMuxer::muxer_write_tailer(oc);
  return 0;  // Do nothing.
}

int CameraTs::muxer_write_free_packet(MuxProcessor* process,
                                      EncodedPacket* pkt) {
  if (ext_video_fd_ < 0) {
    int ret = CameraMuxer::muxer_write_free_packet(process, pkt);
    if (ret != 0)
      fatal_error_ = ret;
    return ret;
  }
  int fd = -1;
  struct timeval time_val = pkt->time_val;
  if (pkt->type == VIDEO_PACKET) {
    fd = ext_video_fd_;
  } else if (pkt->type == AUDIO_PACKET) {
    fd = ext_audio_fd_;
    gettimeofday(&time_val, nullptr);
  }

  if (fd >= 0) {
    if (pipe_write(fd, &time_val, sizeof(time_val)) < 0)
      return -1;
    unsigned size = pkt->av_pkt.size;
    if (pipe_write(fd, &size, sizeof(size)) < 0)
      return -1;
    if (pipe_write(fd, pkt->av_pkt.data, size) < 0)
      return -1;
  }

  pkt->unref();

  return 0;
}
