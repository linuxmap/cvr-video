// by wangh

#ifndef ENCODE_HANDLER_H
#define ENCODE_HANDLER_H

#include <functional>

extern "C" {
#include <alsa/asoundlib.h>
#include <libavutil/time.h>
#include "video_ion_alloc.h"

#include "watermark.h"
}

#include "alsa_capture.h"
#include "message_queue.h"
#include "mpp/rk_mpi_cmd.h"

#include "encoder_muxing/encoder/buffer.h"
#include "encoder_muxing/encoder/ffmpeg_wrapper/ff_audio_encoder.h"
#include "encoder_muxing/encoder/h264_encoder.h"
#include "encoder_muxing/muxing/cache/camera_cache_muxer.h"
#include "encoder_muxing/muxing/file_muxer/camera_file_muxer.h"
#include "encoder_muxing/packet_dispatcher.h"

#include "handler/motion_detect_handler.h"
#include "handler/thumbnail_handler.h"

#include "motion_detection/md_processor.h"
#include "motion_detection/mv_dispatcher.h"

typedef enum {
  START_ENCODE = 1,  // initial h264 encoding
  STOP_ENCODE,
  START_CACHE,  // cache encoded data
  STOP_CACHE,
  START_RECODEFILE,
  STOP_RECODEFILE,
  START_SAVE_CACHE,
  STOP_SAVE_CACHE,
  ATTACH_USER_MUXER,  // attach to initial h264
  DETACH_USER_MUXER,
  ATTACH_USER_MDP,
  DETACH_USER_MDP,
  RECORD_RATE_CHANGE,
  GEN_IDR_FRAME,  // generate an idr for next encode frame
  ID_NB
} EncoderMessageID;

class EncodeHandler;

// Feedback recording duration in sec.
typedef void (*record_time_notifier)(EncodeHandler* handler, int sec);

// Be called before waiting cond.
typedef void (*call_bef_wait)(void* arg);

class FuncBefWaitMsg {
 public:
  FuncBefWaitMsg(call_bef_wait func, void* arg) : func_(func), arg_(arg) {}
  void operator()(void) { func_(arg_); }

 private:
  call_bef_wait func_;
  void* arg_;
};

void* video_record_audio(void* arg);

class EncodeHandler {
 private:
  int device_id;
  int video_type;
  int usb_type;
  MessageQueue<EncoderMessageID>* msg_queue;

  struct alsa_capture* audio_capture;
  bool audio_enable;
  bool mute;
  pthread_t audio_tid;

#ifdef DEBUG
  pthread_t process_pid;
#endif

  int edb_num;                             // num of encode destination buffer
  std::vector<struct video_ion> edb_ions;  // encode destination buffer
#ifdef LARGE_ION_BUFFER
  std::vector<bool> edb_rd;  // whether encode dst buffer is reading
  std::mutex edb_mtx;
  std::condition_variable edb_cond;
#endif

  struct timeval start_timestamp;
#ifdef DISABLE_FF_MPP
  FFContext* ff_ctx;  // ffmpeg muxer help context
#endif
  H264Encoder* h264_encoder;
  FFAudioEncoder* audio_encoder;
  PacketDispatcher pkts_dispatcher;
  CameraFileMuxer* camerafile;
  ThumbnailHandler thumbnail_handler;
  bool thumbnail_gen;

  CameraCacheMuxer<CameraFileMuxer>* cameracache;
  MotionDetectHandler md_handler;

  struct timeval pre_frame_time;  // used for time lapse

  // For USB_TYPE_H264.
  void* extra_data;
  size_t extra_data_size;
  bool pkt_accpet;

  MppEncPrepCfg precfg;

  EncodeHandler();
  // dev_id: camera device id
  // ac_dev_id: audio capture device id
  int init(int dev_id,
           int video_type,
           int usb_type,
           int width,
           int height,
           int fps,
           int ac_dev_id,
           PixelFormat pixel_fmt);
  void get_writeable_edb(BufferData& bd, int& w_index);
  void set_ebd_writeable(int index);
  int encode_start_audio_thread();
  void encode_stop_audio_thread();
  int h264_audio_encoder_ref();
  void h264_audio_encoder_unref();
  int h264_encoder_ref();
  void h264_encoder_unref();
  int audio_encoder_ref();
  void audio_encoder_unref();

  int file_encode_start_job();
  void file_encode_stop_current_job();
  int file_encode_restart_job();

  int send_message(EncoderMessageID id,
                   void* arg1 = NULL,
                   void* arg2 = NULL,
                   void* arg3 = NULL,
                   bool sync = false,
                   FuncBefWaitMsg* call_func = NULL);
  bool remove_message_of_id(
      EncoderMessageID id,
      std::function<void(Message<EncoderMessageID>*)>* release_func = nullptr);
  int attach_user_muxer(CameraMuxer* muxer,
                        char* uri,
                        bool need_video,
                        bool need_audio);
  void detach_user_muxer(CameraMuxer* muxer);
  int attach_user_mdprocessor(VPUMDProcessor* p, MDAttributes* attr);
  void detach_user_mdprocessor(VPUMDProcessor* p);

 public:
  bool small_encodeing;
  char filename[128];
  char file_format[8];
  pthread_attr_t* global_attr;
  // register callback
  record_time_notifier record_time_notify;

  MediaConfig h264_config;
  MediaConfig audio_config;

  struct watermark_info* watermark;

  static unsigned short TimeLapseInterval;

  ~EncodeHandler();
  static EncodeHandler* create(int dev_id,
                               int video_type,
                               int usb_type,
                               int w,
                               int h,
                               int fps,
                               int ac_dev_id,
                               PixelFormat pixel_fmt = PIX_FMT_NV12);
  FFAudioEncoder* get_audio_encoder() { return audio_encoder; }
  PacketDispatcher* get_pkts_dispatcher() { return &pkts_dispatcher; }
  struct alsa_capture* get_audio_capture() {
    return audio_capture;
  }
  bool is_audio_enable() { return audio_enable; }
  void set_enable_audio(bool en_val) { audio_enable = en_val; }
  pthread_t get_audio_tid() { return audio_tid; }
  bool get_audio_mute() { return mute; }
  void set_audio_mute(bool mute_val) { mute = mute_val; }
  void set_global_attr(pthread_attr_t* attr) { global_attr = attr; }
  bool is_cache_writing() {
    return cameracache ? !!cameracache->GetWorkingMuxer() : false;
  }
  void reset_video_bit_rate();

  // for usb input
  void set_pixel_format(PixelFormat pixel_fmt);

  void send_encoder_start();
  void send_encoder_stop();
  void send_record_file_start(FFAudioEncoder* audio_enc = NULL);
  void send_record_file_stop(FuncBefWaitMsg* call_func = NULL);
  void send_record_file_switch();
  void send_save_cache_start();
  void send_save_cache_stop();
  void send_cache_data_start(int cache_duration,
                             FFAudioEncoder* audio_enc = NULL);
  void send_cache_data_stop();

  /** muxer  : user muxer
   *  uri    : dst path
   *  need_av: 1, need video; 2, need audio; 3, need both
   */
  void send_attach_user_muxer(CameraMuxer* muxer, char* uri, int need_av);
  void sync_detach_user_muxer(CameraMuxer* muxer);

  void send_attach_user_mdprocessor(VPUMDProcessor* p, MDAttributes* attr);
  void sync_detach_user_mdprocessor(VPUMDProcessor* p);
  void send_rate_change(VPUMDProcessor* p, int low_bit_rate);
  void send_gen_idr_frame();

  MppEncPrepCfg& get_mpp_enc_prepcfg() { return precfg; }
  void reset_mpp_enc_prepcfg() { memset(&precfg, 0, sizeof(precfg)); }

  int h264_encode_process(const Buffer& input_buf,
                          const PixelFormat& fmt,
                          void* extradata = nullptr,
                          size_t extradata_size = 0);
  // must called before h264_encode_process in the same thread
  int h264_encode_control(int cmd, void* param);

  friend void* video_record_audio(void* arg);
};

#endif  // ENCODE_HANDLER_H
