#include <stdio.h>
#include <sys/prctl.h>
extern "C" {
#include "fs_manage/fs_storage.h"
#include "parameter.h"
#include "rk_rga/rk_rga.h"
}

#include "encode_handler.h"
#include "handler/audio_encode_handler.h"
#include "video.h"

#include <autoconfig/main_app_autoconf.h>

unsigned short EncodeHandler::TimeLapseInterval = 0;

EncodeHandler::EncodeHandler() {
  msg_queue = NULL;
  audio_capture = NULL;
  audio_enable = false;
  mute = false;
  audio_tid = 0;
#ifdef DEBUG
  process_pid = 0;
#endif
  edb_num = 1;  // default 1
  memset(&h264_config, 0, sizeof(h264_config));
  memset(&audio_config, 0, sizeof(audio_config));
  memset(&start_timestamp, 0, sizeof(start_timestamp));
  memset(&pre_frame_time, 0, sizeof(pre_frame_time));
  watermark = NULL;
#ifdef DISABLE_FF_MPP
  ff_ctx = NULL;
#endif
  h264_encoder = NULL;
  audio_encoder = NULL;
  camerafile = NULL;
  thumbnail_gen = false;
  cameracache = NULL;
  reset_mpp_enc_prepcfg();

  small_encodeing = false;
  filename[0] = 0;
  file_format[0] = 0;
  global_attr = NULL;
  record_time_notify = NULL;
}

EncodeHandler::~EncodeHandler() {
#ifdef SDV
  h264_audio_encoder_unref();
#endif
  // should closed all these pointers before deleting encode hanlder
  assert(!audio_enable);
  assert(!cameracache);
  assert(!camerafile);
  assert(!h264_encoder);
  assert(!audio_encoder);

  for (size_t i = 0; i < edb_ions.size(); i++) {
    struct video_ion& ion_buffer = edb_ions.at(i);
    video_ion_free(&ion_buffer);
  }

  if (audio_capture) {
    if (audio_capture->handle) {
      printf("warning: audio capture is alive when exit encode\n");
      alsa_capture_done(audio_capture);
    }
    free(audio_capture);
    audio_capture = NULL;
  }

  if (msg_queue) {
    if (!msg_queue->Empty()) {
      printf(
          "warning: encoder handler msg_queue is not empty,"
          " camera error before??\n");
    }
    msg_queue->clear();
    delete msg_queue;
    msg_queue = NULL;
  }
}

EncodeHandler* EncodeHandler::create(int dev_id,
                                     int video_type,
                                     int usb_type,
                                     int w,
                                     int h,
                                     int fps,
                                     int ac_dev_id,
                                     PixelFormat pixel_fmt) {
  EncodeHandler* pHandler = new EncodeHandler();
  if (pHandler && !pHandler->init(dev_id, video_type, usb_type, w, h, fps,
                                  ac_dev_id, pixel_fmt)) {
    return pHandler;
  } else {
    if (pHandler) {
      delete pHandler;
    }
    return NULL;
  }
}

int EncodeHandler::init(int dev_id,
                        int video_type,
                        int usb_type,
                        int width,
                        int height,
                        int fps,
                        int ac_dev_id,
                        PixelFormat pixel_fmt) {
#ifdef LARGE_ION_BUFFER
  // for 4k encode, separate encoding from memcpying to libfs
  if (width * height > 1920 * 1088)
    edb_num = 3;
#endif
  if (usb_type == USB_TYPE_H264)
    edb_num = 0;
  for (int i = 0; i < edb_num; i++) {
    struct video_ion ion_buffer;
    memset(&ion_buffer, 0, sizeof(ion_buffer));
    ion_buffer.client = -1;
    ion_buffer.fd = -1;
    if (video_ion_alloc_rational(&ion_buffer, width, height, 3, 2) == -1) {
      PRINTF_NO_MEMORY;
      return -1;
    }
    edb_ions.push_back(ion_buffer);
#ifdef LARGE_ION_BUFFER
    edb_rd.push_back(false);
#endif
  }

  if (ac_dev_id >= 0) {
    audio_capture =
        (struct alsa_capture*)calloc(1, sizeof(struct alsa_capture));
    if (!audio_capture) {
      PRINTF_NO_MEMORY;
      return -1;
    }
    audio_capture->sample_rate = MAIN_APP_AUDIO_SAMPLE_RATE;
    audio_capture->format = SND_PCM_FORMAT_S16_LE;
    audio_capture->dev_id = ac_dev_id;
  }

  VideoConfig& vconfig = h264_config.video_config;
  vconfig.width = width;
  vconfig.height = height;
  vconfig.fmt = pixel_fmt;
  vconfig.bit_rate =
      parameter_get_bit_rate_per_pixel() * width * height * fps / 30;
  if (vconfig.bit_rate > 1000000) {
    vconfig.bit_rate /= 1000000;
    vconfig.bit_rate *= 1000000;
  }
  vconfig.frame_rate = fps;
  vconfig.level = 51;
#ifdef MAIN_APP_CACHE_ENCODEDATA_IN_MEM
  vconfig.gop_size = fps;
#else
  vconfig.gop_size = 4 * fps;
#endif
  vconfig.profile = 100;
  vconfig.quality = MPP_ENC_RC_QUALITY_BEST;
  vconfig.qp_init = 26;
  vconfig.qp_step = 8;
  vconfig.qp_min = 4;
  vconfig.qp_max = 48;
  vconfig.rc_mode = MPP_ENC_RC_MODE_CBR;

  if (audio_capture) {
    AudioConfig& aconfig = audio_config.audio_config;
    aconfig.bit_rate = 24000;
    aconfig.nb_samples = MAIN_APP_AUDIO_SAMPLE_NUM;
    switch (audio_capture->format) {
      case SND_PCM_FORMAT_S16_LE:
        aconfig.fmt = SAMPLE_FMT_S16;
        break;
      default:
        // TODO add other fmts
        assert(0);
    }

    // TODO
    if (audio_capture->channel == 1)
      aconfig.channel_layout = AV_CH_LAYOUT_MONO;
    else
      aconfig.channel_layout = AV_CH_LAYOUT_STEREO;

    aconfig.sample_rate = audio_capture->sample_rate;
  }

  if (md_handler.Init(h264_config) < 0)
    md_handler.DeInit();

  MediaConfig jpg_config;
  jpg_config.jpeg_config.width = width;
  jpg_config.jpeg_config.height = height;
  jpg_config.jpeg_config.fmt = pixel_fmt;
  jpg_config.jpeg_config.qp = 10;

  if (thumbnail_handler.Init(jpg_config) < 0)
    thumbnail_handler.DeInit();

  msg_queue = new MessageQueue<EncoderMessageID>();
  if (!msg_queue) {
    PRINTF_NO_MEMORY;
    return -1;
  }
  this->device_id = dev_id;
  this->video_type = video_type;
  this->usb_type = usb_type;

#ifdef SDV
  h264_audio_encoder_ref();
#endif
  return 0;
}

// should have unref h264_encoder and change the bit_rate_per_pixel in parameter
void EncodeHandler::reset_video_bit_rate() {
#ifdef SDV
  h264_audio_encoder_unref();
#endif
  assert(!h264_encoder);
  VideoConfig& vconfig = h264_config.video_config;
  vconfig.bit_rate = parameter_get_bit_rate_per_pixel() * vconfig.width *
                     vconfig.height * vconfig.frame_rate / 30;
  if (vconfig.bit_rate > 1000000) {
    vconfig.bit_rate /= 1000000;
    vconfig.bit_rate *= 1000000;
  }
#ifdef SDV
  h264_audio_encoder_ref();
#endif
}

// should have unref h264_encoder
void EncodeHandler::set_pixel_format(PixelFormat pixel_fmt) {
  h264_config.video_config.fmt = pixel_fmt;
}

int EncodeHandler::send_message(EncoderMessageID id,
                                void* arg1,
                                void* arg2,
                                void* arg3,
                                bool sync,
                                FuncBefWaitMsg* call_func) {
  Message<EncoderMessageID>* msg = Message<EncoderMessageID>::create(id);
  if (msg) {
    msg->arg1 = arg1;
    msg->arg2 = arg2;
    msg->arg3 = arg3;
    if (sync) {
      std::mutex* mtx = (std::mutex*)msg->arg2;
      std::condition_variable* cond = (std::condition_variable*)msg->arg3;
      {
        std::unique_lock<std::mutex> _lk(*mtx);
        msg_queue->Push(msg);
        if (call_func)
          (*call_func)();
        cond->wait(_lk);
      }
    } else {
      msg_queue->Push(msg);
    }
    return 0;
  } else {
    PRINTF_NO_MEMORY;
    return -1;
  }
}

bool EncodeHandler::remove_message_of_id(
    EncoderMessageID id,
    std::function<void(Message<EncoderMessageID>*)>* release_func) {
  std::list<Message<EncoderMessageID>*> list;
  msg_queue->PopMessageOfId(id, list);
  if (!list.empty()) {
    for (Message<EncoderMessageID>* m : list) {
      if (release_func)
        (*release_func)(m);
      delete m;
    }
    return true;
  }
  return false;
}

#define DECLARE_MUTEX_COND \
  std::mutex mtx;          \
  std::condition_variable cond

void EncodeHandler::send_encoder_start() {
  send_message(START_ENCODE);
}

void EncodeHandler::send_encoder_stop() {
  if (!remove_message_of_id(START_ENCODE)) {
    DECLARE_MUTEX_COND;
    send_message(STOP_ENCODE, (void*)true, &mtx, &cond, true);
  }
}

void EncodeHandler::send_record_file_start(FFAudioEncoder* audio_enc) {
  send_message(START_RECODEFILE, audio_enc);
}

void EncodeHandler::send_record_file_stop(FuncBefWaitMsg* call_func) {
  if (!remove_message_of_id(START_RECODEFILE)) {
    DECLARE_MUTEX_COND;
    send_message(STOP_RECODEFILE, (void*)true, &mtx, &cond, true, call_func);
  }
}

void EncodeHandler::send_record_file_switch() {
  send_message(STOP_RECODEFILE, (void*)false, (void*)true);
}

void EncodeHandler::send_save_cache_start() {
  send_message(START_SAVE_CACHE);
}

void EncodeHandler::send_save_cache_stop() {
  if (!remove_message_of_id(START_SAVE_CACHE)) {
    DECLARE_MUTEX_COND;
    send_message(STOP_SAVE_CACHE, NULL, &mtx, &cond, true);
  }
}

void EncodeHandler::send_cache_data_start(int cache_duration,
                                          FFAudioEncoder* audio_enc) {
  send_message(START_CACHE, (void*)cache_duration, audio_enc);
}

void EncodeHandler::send_cache_data_stop() {
  if (!remove_message_of_id(START_CACHE)) {
    DECLARE_MUTEX_COND;
    send_message(STOP_CACHE, NULL, &mtx, &cond, true);
  }
}

void EncodeHandler::send_attach_user_muxer(CameraMuxer* muxer,
                                           char* uri,
                                           int need_av) {
  send_message(ATTACH_USER_MUXER, muxer, uri, (void*)need_av);
}

void EncodeHandler::sync_detach_user_muxer(CameraMuxer* muxer) {
  if (!remove_message_of_id(ATTACH_USER_MUXER)) {
    DECLARE_MUTEX_COND;
    send_message(DETACH_USER_MUXER, muxer, &mtx, &cond, true);
  }
}

void EncodeHandler::send_attach_user_mdprocessor(VPUMDProcessor* p,
                                                 MDAttributes* attr) {
  send_message(ATTACH_USER_MDP, p, attr);
}

void EncodeHandler::sync_detach_user_mdprocessor(VPUMDProcessor* p) {
  if (!remove_message_of_id(ATTACH_USER_MDP)) {
    DECLARE_MUTEX_COND;
    send_message(DETACH_USER_MDP, p, &mtx, &cond, true);
  }
}

void EncodeHandler::send_rate_change(VPUMDProcessor* p, int low_frame_rate) {
  send_message(RECORD_RATE_CHANGE, p, (void*)low_frame_rate);
}

void EncodeHandler::send_gen_idr_frame() {
  send_message(GEN_IDR_FRAME);
}

void* video_record_audio(void* arg) {
  int ret = 0;
  EncodeHandler* handler = (EncodeHandler*)arg;
  FFAudioEncoder* audio_encoder = handler->get_audio_encoder();
  PacketDispatcher* pkt_dispatcher = handler->get_pkts_dispatcher();
  struct alsa_capture* audio_capture = handler->get_audio_capture();
  uint64_t audio_idx = 0;
  // Produce a mute packet.
  AVPacket mute_packet;

  prctl(PR_SET_NAME, "record_audio", 0, 0, 0);

  if ((ret = gen_mute_packet(audio_encoder, mute_packet)) < 0) {
    fprintf(stderr, "produce a mute packet failed\n");
    goto out;
  }
  while (1) {
    if (handler->get_audio_tid() && !handler->is_audio_enable()) {
      bool finish = false;
      while (!finish) {
        AVPacket av_pkt;
        av_init_packet(&av_pkt);
        audio_encoder->EncodeFlushAudioData(&av_pkt, finish);
        PRINTF("~~finish : %d\n", finish);
        if (!finish) {
          EncodedPacket* pkt = new_audio_packet(++audio_idx);
          if (!pkt) {
            av_packet_unref(&av_pkt);
            ret = -1;
            goto out;
          }
          pkt->av_pkt = av_pkt;
          pkt_dispatcher->Dispatch(pkt);
          delete_audio_packet(pkt);
        }
      }
      break;
    }

    int nb_samples = 0;
    int channels = 0;
    void* audio_buf = nullptr;
    enum AVSampleFormat fmt;
    audio_encoder->GetAudioBuffer(&audio_buf, &nb_samples, &channels, &fmt);
    assert(audio_buf && nb_samples * channels > 0);
    audio_capture->buffer_frames = nb_samples;
    int capture_samples = alsa_capture_doing(audio_capture, audio_buf);
    if (capture_samples <= 0) {
      printf("capture_samples : %d, expect samples: %d\n", capture_samples,
             nb_samples);
      alsa_capture_done(audio_capture);
      if (alsa_capture_open(audio_capture, audio_capture->dev_id))
        goto out;
      capture_samples = alsa_capture_doing(audio_capture, audio_buf);
      if (capture_samples <= 0) {
        printf("there seems something wrong with audio capture, disable it\n");
        printf("capture_samples : %d, expect samples: %d\n", capture_samples,
               nb_samples);
        goto out;
      }
    }
    audio_idx++;

    EncodedPacket* pkt = nullptr;
    bool mute = handler->get_audio_mute();
    if (mute) {
#ifdef MAIN_APP_CACHE_ENCODEDATA_IN_MEM
      if (handler->cameracache) {
        // Encode a video packet with real input.
        EncodedPacket* pkt = new_audio_packet(audio_idx, audio_encoder);
        if (!pkt) {
          ret = -1;
          goto out;
        }
        pkt_dispatcher->DispatchToSpecial(pkt);
        delete_audio_packet(pkt);
      }
#endif
      pkt = new EncodedPacket();
      if (pkt) {
        pkt->type = AUDIO_PACKET;
        if (av_packet_ref(&pkt->av_pkt, &mute_packet)) {
          delete_audio_packet(pkt);
          ret = -1;
          goto out;
        }
        pkt->is_phy_buf = false;
        pkt->audio_index = audio_idx;
      }
    }
    if (!pkt)
      pkt = new_audio_packet(audio_idx, audio_encoder);
    if (!pkt) {
      ret = -1;
      goto out;
    }
    pkt_dispatcher->Dispatch(pkt);
    if (!mute)
      pkt_dispatcher->DispatchToSpecial(pkt);
    delete_audio_packet(pkt);
  }
out:
  printf("%s out\n", __func__);
  handler->set_enable_audio(false);
  av_packet_unref(&mute_packet);
  pthread_exit((void*)ret);
}

int EncodeHandler::encode_start_audio_thread() {
  if (audio_capture && audio_capture->dev_id >= 0 && !audio_enable) {
    if (alsa_capture_open(audio_capture, audio_capture->dev_id) == -1) {
      printf("alsa_capture_open err\n");
      return -1;
    }
    pthread_t tid = 0;
    if (pthread_create(&tid, global_attr, video_record_audio, (void*)this)) {
      printf("create audio pthread err\n");
      return -1;
    }
    audio_enable = true;
    audio_tid = tid;
  }
  return 0;
}

void EncodeHandler::encode_stop_audio_thread() {
  if (audio_tid) {
    audio_enable = false;
    pthread_join(audio_tid, NULL);
    audio_tid = 0;
  }
  if (audio_capture) {
    alsa_capture_done(audio_capture);
  }
}

// TODO: remove macro
#define ENCODER_REF(CLASSTYPE, VAR, PCONFIG) \
  if (!VAR) {                                \
    CLASSTYPE* encoder = new CLASSTYPE();    \
    if (!encoder) {                          \
      printf("create " #VAR " failed\n");    \
      return -1;                             \
    }                                        \
    if (0 != encoder->InitConfig(PCONFIG)) { \
      encoder->unref();                      \
      return -1;                             \
    }                                        \
    VAR = encoder;                           \
  } else {                                   \
    VAR->ref();                              \
  }                                          \
  PRINTF("ref " #VAR " : %p, refcount : %d\n", VAR, VAR->refcount())

#define ENCODER_UNREF(VAR)                                       \
  if (VAR) {                                                     \
    int refcnt = VAR->refcount();                                \
    if (0 == VAR->unref()) {                                     \
      VAR = NULL;                                                \
    }                                                            \
    PRINTF("unref " #VAR " : %p, refcount : %d\n", VAR, refcnt); \
    UNUSED(refcnt);                                              \
  }

// the following functions should only be called in h264_encode_process

int EncodeHandler::h264_encoder_ref() {
  ENCODER_REF(H264Encoder, h264_encoder, h264_config);
#ifdef DISABLE_FF_MPP
  if (ff_ctx)
    return 0;
  void* h264_extra_data = extra_data;
  size_t h264_extra_data_size = extra_data_size;
  ff_ctx = new FFContext();
  if (!ff_ctx)
    goto err;
  ff_ctx->SetCodecId(AV_CODEC_ID_H264);
  if (ff_ctx->InitConfig(h264_config.video_config, false))
    goto err;
  if (usb_type != USB_TYPE_H264)
    h264_encoder->GetExtraData(h264_extra_data, h264_extra_data_size);
  if (copy_extradata(*ff_ctx, h264_extra_data, h264_extra_data_size))
    goto err;
  h264_encoder->SetHelpContext(ff_ctx);
#endif
  return 0;
#ifdef DISABLE_FF_MPP
err:
  h264_encoder_unref();
  if (ff_ctx) {
    delete ff_ctx;
    ff_ctx = NULL;
  }
  return -1;
#endif
}
void EncodeHandler::h264_encoder_unref() {
  ENCODER_UNREF(h264_encoder);
#ifdef DISABLE_FF_MPP
  if (!h264_encoder) {
    assert(ff_ctx);
    delete ff_ctx;
    ff_ctx = NULL;
  }
#endif
}
int EncodeHandler::audio_encoder_ref() {
  if (!audio_capture)
    return 0;  // audio device is not ready
  ENCODER_REF(FFAudioEncoder, audio_encoder, audio_config);
  if (audio_encoder->refcount() == 1) {
    if (0 != encode_start_audio_thread()) {
      printf("start audio thread failed\n");
      if (0 == audio_encoder->unref()) {
        audio_encoder = NULL;
      }
      return -1;
    }
  }
  return 0;
}
void EncodeHandler::audio_encoder_unref() {
  if (!audio_encoder)
    return;
  int refcnt = audio_encoder->refcount();
  if (refcnt == 1) {
    encode_stop_audio_thread();
  }
  if (0 == audio_encoder->unref()) {
    audio_encoder = NULL;
  }
  PRINTF("unref audio_encoder : %p, refcount : %d\n", audio_encoder, refcnt);
}

int EncodeHandler::h264_audio_encoder_ref() {
  int ret = h264_encoder_ref();
  if (0 == ret) {
    h264_encoder->SetForceIdrFrame();
    ret = audio_encoder_ref();
    if (ret)
      h264_encoder_unref();
  }
  return ret;
}

void EncodeHandler::h264_audio_encoder_unref() {
  audio_encoder_unref();
  h264_encoder_unref();
}

int EncodeHandler::attach_user_muxer(CameraMuxer* muxer,
                                     char* uri,
                                     bool need_video,
                                     bool need_audio) {
  assert(muxer);
  H264Encoder* vencoder = NULL;
  FFAudioEncoder* aencoder = NULL;
  int ret = 0;
  int rate = 0;

  if (need_video) {
    ret = h264_encoder_ref();
    vencoder = ret ? NULL : h264_encoder;
    if (vencoder)
      rate = vencoder->GetVideoConfig().frame_rate;
  }

  if (need_audio) {
    ret = audio_encoder_ref();
    aencoder = ret ? NULL : audio_encoder;
  }

  if (0 > muxer->init_uri(uri, rate))
    goto err;

  muxer->set_encoder(vencoder, aencoder);
  if (0 > muxer->start_new_job(global_attr))
    goto err;
  else
    pkts_dispatcher.AddHandler(muxer);

  return 0;

err:
  if (vencoder && vencoder == h264_encoder)
    h264_encoder_unref();
  if (aencoder)
    audio_encoder_unref();

  return -1;
}

void EncodeHandler::detach_user_muxer(CameraMuxer* muxer) {
  BaseEncoder* vencoder = muxer->get_video_encoder();
  BaseEncoder* aencoder = muxer->get_audio_encoder();

  if (vencoder) {
    if (vencoder == h264_encoder)
      h264_encoder_unref();
  }
  if (aencoder && aencoder == audio_encoder)
    audio_encoder_unref();

  pkts_dispatcher.RemoveHandler(muxer);
  muxer->stop_current_job();
  muxer->sync_jobs_complete();
}

int EncodeHandler::attach_user_mdprocessor(VPUMDProcessor* p,
                                           MDAttributes* attr) {
  assert(p);
  return md_handler.AddMdProcessor(p, attr);
}

void EncodeHandler::detach_user_mdprocessor(VPUMDProcessor* p) {
  md_handler.RemoveMdProcessor(p);
  // low_frame_rate_mode = false;
  if (camerafile) {
    struct timeval tval = {0};
    if (camerafile->set_low_frame_rate(MODE_NORMAL, tval))
      h264_encoder->SetForceIdrFrame();
  }
}

#define MUXER_ALLOC(CLASSTYPE, VAR, FMT, ENCODER_REF_FUNC, ENCODER_UNREF_FUNC, \
                    URI, VIDEOENCODER, AUDIOENCODER)                           \
  do {                                                                         \
    assert(!VAR);                                                              \
    if (0 > ENCODER_REF_FUNC()) {                                              \
      msg_ret = -1;                                                            \
      break;                                                                   \
    }                                                                          \
    CLASSTYPE* muxer = CLASSTYPE::create();                                    \
    if (!muxer) {                                                              \
      ENCODER_UNREF_FUNC();                                                    \
      printf("create " #VAR " fail\n");                                        \
      msg_ret = -1;                                                            \
      break;                                                                   \
    }                                                                          \
    if (FMT)                                                                   \
      muxer->set_out_format(FMT);                                              \
    if (edb_num > 1)                                                           \
      muxer->set_async(true);                                                  \
    if (0 > muxer->init_uri(URI, VIDEOENCODER->GetVideoConfig().frame_rate)) { \
      ENCODER_UNREF_FUNC();                                                    \
      delete muxer;                                                            \
      msg_ret = -1;                                                            \
      break;                                                                   \
    }                                                                          \
    muxer->set_encoder(VIDEOENCODER, AUDIOENCODER);                            \
    if (0 > muxer->start_new_job(global_attr)) {                               \
      ENCODER_UNREF_FUNC();                                                    \
      delete muxer;                                                            \
      msg_ret = -1;                                                            \
    } else {                                                                   \
      VAR = muxer;                                                             \
    }                                                                          \
  } while (0);                                                                 \
  if (0 > msg_ret)                                                             \
  break

#define MUXER_FREE(VAR, ENCODER_UNREF_FUNC) \
  if (VAR) {                                \
    VAR->stop_current_job();                \
    delete VAR;                             \
    VAR = NULL;                             \
    ENCODER_UNREF_FUNC();                   \
  }

int EncodeHandler::file_encode_start_job() {
  video_record_getfilename(
      filename, sizeof(filename),
      fs_storage_folder_get_bytype(video_type, VIDEOFILE_TYPE), device_id,
      file_format, small_encodeing ? "_S" : "");
  if (0 > camerafile->init_uri(filename,
                               h264_encoder->GetVideoConfig().frame_rate)) {
    delete camerafile;
    camerafile = NULL;
    return -1;
  }
  memset(&start_timestamp, 0, sizeof(struct timeval));
  memset(&pre_frame_time, 0, sizeof(pre_frame_time));
  if (0 == camerafile->start_new_job(global_attr)) {
    // as file no cache in list, should manage muxers strictly when start or
    // stop
    pkts_dispatcher.AddHandler(camerafile);
    return 0;
  }
  return -1;
}

void EncodeHandler::file_encode_stop_current_job() {
  // as file no cache in list, should manage muxers strictly when start or stop
  pkts_dispatcher.RemoveHandler(camerafile);
  camerafile->stop_current_job();
  camerafile->reset_lbr_time();
  pkt_accpet = false;
}

int EncodeHandler::file_encode_restart_job() {
  // h264audio_encoder_unref();
  file_encode_stop_current_job();
  // if (0 > h264audio_encoder_ref()) {
  //  return -1;
  // }
  // camerafile->set_encoder(h264_encoder, audio_encoder);
  // restart immediately
  if (0 > file_encode_start_job()) {
    return -1;
  }
  thumbnail_gen = !small_encodeing;
  h264_encoder->SetForceIdrFrame();
  return 0;
}

void EncodeHandler::get_writeable_edb(BufferData& bd, int& w_index) {
#ifdef LARGE_ION_BUFFER
  if (edb_num > 1) {
    do {
      std::chrono::milliseconds msec(1000);
      std::unique_lock<std::mutex> _lk(edb_mtx);
      int i = 0;
      // As edb_num is a small value, looply access the vector is ok
      for (; i < edb_num && edb_rd[i]; i++)
        ;
      if (likely(i < edb_num)) {
        w_index = i;
        break;
      } else {
        if (edb_cond.wait_for(_lk, msec) == std::cv_status::timeout) {
          printf("wait reading release the encode out buffer time out!\n");
          w_index = -1;
          return;
        }
      }
    } while (true);
  } else {
    w_index = 0;
  }
#else
  w_index = 0;
#endif
  struct video_ion& ion_buffer = edb_ions.at(w_index);
  bd.vir_addr_ = ion_buffer.buffer;
  bd.ion_data_.fd_ = ion_buffer.fd;
  bd.ion_data_.handle_ = ion_buffer.handle;
  bd.mem_size_ = ion_buffer.size;
}

void EncodeHandler::set_ebd_writeable(int index) {
#ifdef LARGE_ION_BUFFER
  std::unique_lock<std::mutex> _lk(edb_mtx);
  edb_rd[index] = false;
  edb_cond.notify_one();
#endif
}

static void do_notify(void* arg1, void* arg2) {
  std::mutex* mtx = (std::mutex*)arg1;
  std::condition_variable* cond = (std::condition_variable*)arg2;
  if (mtx) {
    assert(cond);
    std::unique_lock<std::mutex> lk(*mtx);
    cond->notify_one();
  }
}

#ifdef USE_WATERMARK
inline static void draw_watermark(void* buffer,
                                  const VideoConfig& vconfig,
                                  struct watermark_info* watermark) {
  // TODO: if not nv12
  if (watermark && watermark->type > 0 && vconfig.fmt == PIX_FMT_NV12) {
    watermark_draw_on_nv12(watermark, (uint8_t*)buffer, vconfig.width,
                           vconfig.height, parameter_get_licence_plate_flag());
  }
}
#endif

// TODO, move msg_queue handler to thread which create video
int EncodeHandler::h264_encode_process(const Buffer& input_buf,
                                       const PixelFormat& fmt,
                                       void* extradata,
                                       size_t extradata_size) {
  int recorded_sec = 0;
  bool del_recoder_file = false;
  bool video_save = false;
  Message<EncoderMessageID>* msg = NULL;
  int fd = input_buf.GetFd();
  struct timeval& time_val = input_buf.GetTimeval();
  unsigned short tl_interval = TimeLapseInterval;

  if (h264_config.video_config.fmt != fmt) {
    // usb camera: Only know the fmt after the first input mjpeg pixels
    //             decoded successfully.
    if (!camerafile)
      h264_config.video_config.fmt = fmt;
    else
      printf("<%s>warning: fmt change from [%d] to [%d]\n", __func__,
             h264_config.video_config.fmt, fmt);
  }
  extra_data = extradata;
  extra_data_size = extradata_size;
  assert(msg_queue);
  if (!msg_queue->Empty()) {
    int msg_ret = 0;
#ifdef DEBUG
    if (!process_pid)
      process_pid = pthread_self();
    if (pthread_self() != process_pid)
      printf("warning: h264_encode_process run in different thread!\n");
#endif
    msg = msg_queue->PopFront();
    EncoderMessageID msg_id = msg->GetID();
    switch (msg_id) {
      case START_ENCODE:
        if (0 > h264_audio_encoder_ref())
          msg_ret = -1;
        break;
      case STOP_ENCODE:
        h264_audio_encoder_unref();
        do_notify(msg->arg2, msg->arg3);
        break;
      case START_CACHE:
        if (msg->arg2) {
          MUXER_ALLOC(CameraCacheMuxer<CameraFileMuxer>, cameracache,
                      file_format, h264_audio_encoder_ref,
                      h264_audio_encoder_unref, NULL, h264_encoder,
                      (FFAudioEncoder*)msg->arg2);
        } else {
          MUXER_ALLOC(CameraCacheMuxer<CameraFileMuxer>, cameracache,
                      file_format, h264_audio_encoder_ref,
                      h264_audio_encoder_unref, NULL, h264_encoder,
                      audio_encoder);
        }
        cameracache->SetCacheDuration((int)msg->arg1);
        // pkts_dispatcher.AddHandler(cameracache);
        pkts_dispatcher.AddSpecialMuxer(cameracache);
        AudioEncodeHandler::recheck_enc_pcm_always_ = true;
        break;
      case STOP_CACHE:
        // pkts_dispatcher.RemoveHandler(cameracache);
        pkts_dispatcher.RemoveSpecialMuxer(cameracache);
        AudioEncodeHandler::recheck_enc_pcm_always_ = true;
        MUXER_FREE(cameracache, h264_audio_encoder_unref);
        do_notify(msg->arg2, msg->arg3);
        break;
      case START_RECODEFILE:
        assert(!camerafile);
        video_record_getfilename(
            filename, sizeof(filename),
            fs_storage_folder_get_bytype(video_type, VIDEOFILE_TYPE), device_id,
            file_format, small_encodeing ? "_S" : "");
        memset(&start_timestamp, 0, sizeof(struct timeval));
        memset(&pre_frame_time, 0, sizeof(pre_frame_time));
        if (msg->arg1) {
          // external audio encoder
          assert(!audio_encoder);
          MUXER_ALLOC(CameraFileMuxer, camerafile, file_format,
                      h264_audio_encoder_ref, h264_audio_encoder_unref,
                      filename, h264_encoder, (FFAudioEncoder*)msg->arg1);
        } else {
          MUXER_ALLOC(CameraFileMuxer, camerafile, file_format,
                      h264_audio_encoder_ref, h264_audio_encoder_unref,
                      filename, h264_encoder, audio_encoder);
        }
        thumbnail_gen = !small_encodeing;
        pkts_dispatcher.AddHandler(camerafile);
        pkt_accpet = false;
        break;
      case STOP_RECODEFILE:
        del_recoder_file = (bool)msg->arg1;
        if (!camerafile) {
          if (del_recoder_file)
            do_notify(msg->arg2, msg->arg3);
          break;
        }
        PRINTF("Process STOP_RECODEFILE.\n");
        file_encode_stop_current_job();
        if (del_recoder_file) {
          // delete the file muxer
          camerafile->sync_jobs_complete();
          PRINTF("camerafile job completed.\n");
          delete camerafile;
          camerafile = NULL;
          h264_audio_encoder_unref();
          do_notify(msg->arg2, msg->arg3);
          PRINTF("del camerafile done.\n");
        } else if (!del_recoder_file && (bool)msg->arg2) {
          // restart immediately
          if (0 > file_encode_start_job()) {
            msg_ret = -1;
            break;
          }
          thumbnail_gen = !small_encodeing;
          h264_encoder->SetForceIdrFrame();
        }
        break;
      case START_SAVE_CACHE:
#ifdef MAIN_APP_CACHE_ENCODEDATA_IN_MEM
      {
        char file_path[128];
        video_record_getfilename(
            file_path, sizeof(file_path),
            fs_storage_folder_get_bytype(video_type, LOCKFILE_TYPE), device_id,
            file_format);
        assert(cameracache);
        cameracache->StoreCacheData(file_path, global_attr);
      }
#else
        video_save = true;
#endif
      break;
      case STOP_SAVE_CACHE:
        if (cameracache)
          cameracache->StopCurrentJobImmediately();
        do_notify(msg->arg2, msg->arg3);
        break;
      case ATTACH_USER_MUXER: {
        CameraMuxer* muxer = (CameraMuxer*)msg->arg1;
        char* uri = (char*)msg->arg2;
        uint32_t val = (uint32_t)msg->arg3;
        bool need_video = (val & 1) ? true : false;
        bool need_audio = (val & 2) ? true : false;
        attach_user_muxer(muxer, uri, need_video, need_audio);
        break;
      }
      case DETACH_USER_MUXER: {
        CameraMuxer* muxer = (CameraMuxer*)msg->arg1;
        detach_user_muxer(muxer);
        do_notify(msg->arg2, msg->arg3);
        break;
      }
      case ATTACH_USER_MDP: {
        VPUMDProcessor* p = (VPUMDProcessor*)msg->arg1;
        MDAttributes* attr = (MDAttributes*)msg->arg2;
        attach_user_mdprocessor(p, attr);
        break;
      }
      case DETACH_USER_MDP: {
        VPUMDProcessor* p = (VPUMDProcessor*)msg->arg1;
        detach_user_mdprocessor(p);
        do_notify(msg->arg2, msg->arg3);
        break;
      }
      case RECORD_RATE_CHANGE: {
        VPUMDProcessor* p = (VPUMDProcessor*)msg->arg1;
        bool low_frame_rate_mode = (bool)msg->arg2;
        if (!low_frame_rate_mode) {
          if (camerafile && camerafile->is_low_frame_rate())
            video_save = true;  // low->normal, save file immediately
        } else {
          if (camerafile && fd >= 0) {
            camerafile->set_low_frame_rate(MODE_ONLY_KEY_FRAME, time_val);
            h264_encoder->SetForceIdrFrame();
          }
        }
        if (md_handler.IsRunning())
          p->set_low_frame_rate(low_frame_rate_mode ? 1 : 0);
        break;
      }
      case GEN_IDR_FRAME:
        if (h264_encoder)
          h264_encoder->SetForceIdrFrame();
        break;
      default:
          // assert(0);
          ;
    }
    if (msg_ret < 0) {
      printf("error: handle msg < %d > failed\n", msg->GetID());
      delete msg;
      return msg_ret;
    }
  }
  if (msg)
    delete msg;

#ifdef USE_WATERMARK
  bool src_contain_wm = false;
#endif

  if (usb_type != USB_TYPE_H264) {
    // fd < 0 is normal for usb when pulling out usb connection
    if (fd < 0)
      return 0;
    VideoConfig& vconfig = h264_config.video_config;
    if (camerafile && thumbnail_gen) {
#ifdef USE_WATERMARK
      draw_watermark(input_buf.GetVirAddr(), vconfig, watermark);
      src_contain_wm = true;
#endif
      thumbnail_gen = !thumbnail_handler.Process(fd, vconfig, filename);
    }
    if (cameracache) {
#ifdef USE_WATERMARK
      if (!src_contain_wm) {
        draw_watermark(input_buf.GetVirAddr(), vconfig, watermark);
        src_contain_wm = true;
      }
#endif
      cameracache->TryGenThumbnail(thumbnail_handler, fd, vconfig);
    }
    md_handler.Process(fd, vconfig);
  } else if (!input_buf.GetVirAddr()) {
    return 0;
  }

  if (h264_encoder
#ifdef SDV
      && (camerafile || cameracache)
#endif
  ) {
    int64_t usec = 0;
    if (start_timestamp.tv_sec != 0 || start_timestamp.tv_usec != 0) {
      usec = difftime(start_timestamp, time_val);
    } else {
      start_timestamp = time_val;
      pre_frame_time = time_val;
    }

    recorded_sec = usec / 1000000LL;

    if (camerafile) {
      if (recorded_sec < 0)
        printf("**recorded_sec: %d\n", recorded_sec);
      assert(recorded_sec >= 0);

      if (tl_interval == 0) {
        if (record_time_notify)
          record_time_notify(this, recorded_sec);
        camerafile->set_low_frame_rate(MODE_NORMAL, time_val);
      } else if (tl_interval > 0) {
        if (usb_type != USB_TYPE_H264) {
          if (!camerafile->set_low_frame_rate(MODE_NORMAL_FIX_TIME, time_val)) {
            usec = difftime(pre_frame_time, time_val);
            if (usec > 0 && usec < tl_interval * 1000000LL) {
              camerafile->set_real_time(time_val);
              return 0;
            }
          }
          pre_frame_time = time_val;
        } else {
          camerafile->set_low_frame_rate(MODE_ONLY_KEY_FRAME, time_val);
        }
      }
    }

    int w_index = -1;
    EncodedPacket* pkt = new EncodedPacket();
    if (unlikely(!pkt)) {
      printf("alloc EncodedPacket failed\n");
      assert(0);
      return -1;
    }
    pkt->is_phy_buf = true;
    int encode_ret = -1;
    if (usb_type == USB_TYPE_H264) {
      encode_ret = FFContext::PackEncodedDataToAVPacket(input_buf, pkt->av_pkt);
      if (!encode_ret) {
        uint8_t* data = static_cast<uint8_t*>(input_buf.GetVirAddr());
        uint8_t nal_type = 0;
        // TODO: call find_h264_startcode() which consume more cpu
        if (data[2] == 0)
          nal_type = data[4] & 0x1f;
        else
          nal_type = data[3] & 0x1f;
        if (nal_type == 7 || nal_type == 8) {
          data += extradata_size;
          if (data[2] == 0)
            nal_type = data[4] & 0x1f;
          else
            nal_type = data[3] & 0x1f;
        }
        if (nal_type == 5) {
          // I frame
          /* TODO: Get the gop and framerate of this h264 usb camera */
          if (parameter_get_recodetime() > 0 &&
              recorded_sec + 1 >= parameter_get_recodetime()) {
            if (file_encode_restart_job())
              return -1;
            recorded_sec = 0;
            if (record_time_notify)
              record_time_notify(this, recorded_sec);
          }
          pkt_accpet = true;
          pkt->av_pkt.flags |= AV_PKT_FLAG_KEY;
        }
      }
      if (!pkt_accpet)
        encode_ret = -1;  // Do not push into file.
    } else {
      BufferData dst_data;
      get_writeable_edb(dst_data, w_index);
      do {
        if (unlikely(w_index < 0))
          break;
        Buffer dst_buf(dst_data);
#ifdef USE_WATERMARK
        if (watermark && watermark->type > 0 && !src_contain_wm)
          h264_encode_control(
              MPP_ENC_SET_OSD_DATA_CFG,
              &watermark->mpp_osd_data[watermark->buffer_index]);
#endif
        if (precfg.change)
          h264_encode_control(MPP_ENC_SET_PREP_CFG, (void*)&precfg);
        encode_ret = h264_encoder->EncodeOneFrame(
            const_cast<Buffer*>(&input_buf), &dst_buf, nullptr);
        if (unlikely(encode_ret))
          break;
        assert(dst_buf.GetValidDataSize() > 0);
        encode_ret = FFContext::PackEncodedDataToAVPacket(dst_buf, pkt->av_pkt);
        if (unlikely(encode_ret))
          break;
        if (dst_buf.GetUserFlag() & MPP_PACKET_FLAG_INTRA)
          pkt->av_pkt.flags |= AV_PKT_FLAG_KEY;
        // rk guarantee that return only one slice, and start with 00 00 01
        pkt->av_pkt.flags |= AV_PKT_FLAG_ONE_NAL;
      } while (0);
    }
    if (likely(!encode_ret)) {
      pkt->type = VIDEO_PACKET;
      pkt->time_val = time_val;
#ifdef LARGE_ION_BUFFER
      if (edb_num > 1) {
        edb_rd[w_index] = true;
        pkt->release_cb =
            std::bind(&EncodeHandler::set_ebd_writeable, this, w_index);
      }
#endif
      pkts_dispatcher.Dispatch(pkt);
      if (cameracache)
        pkts_dispatcher.DispatchToSpecial(pkt);
      if (camerafile && camerafile->is_low_frame_rate()) {
        usec = (pkt->lbr_time_val.tv_sec - start_timestamp.tv_sec) * 1000000LL +
               (pkt->lbr_time_val.tv_usec - start_timestamp.tv_usec);
        recorded_sec = usec / 1000000LL;

        // chad.ma: add for lapse video record.
        if (record_time_notify) {
          // printf("recorded_sec = %d \n",recorded_sec);
          record_time_notify(this, recorded_sec);
        }
      }
    }
    pkt->unref();
  }

  if (camerafile && !del_recoder_file) {
    if ((parameter_get_recodetime() > 0 &&
         recorded_sec >= parameter_get_recodetime()) ||
        video_save) {
      if (file_encode_restart_job())
        return -1;
    }
  }

  return 0;
}

int EncodeHandler::h264_encode_control(int cmd, void* param) {
  if (!h264_encoder)
    return 0;  // h264_encoder is not ready
  int ret = h264_encoder->EncodeControl(cmd, param);
  if (ret)
    printf("EncodeHandler::h264_encode_control cmd 0x%08x param %p", cmd,
           param);
  return ret;
}
