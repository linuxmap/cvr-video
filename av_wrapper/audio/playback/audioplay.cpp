#include "audioplay.h"
#include "audio_output.h"
#include "av_wrapper/decoder_demuxing/decoder_demuxing.h"
#include "av_wrapper/video_common.h"
extern "C" {
#include <libavutil/time.h>
#include <libswresample/swresample.h>
}
#include <sys/prctl.h>
#include <atomic>
#include <vector>

class AudioPlayer {
 private:
  DecoderDemuxer dd;
  bool cache_all;  // cache all frame in memory
  bool auto_delete;
  WantedAudioParams* tgt_params;
  WantedAudioParams src_params;
  struct SwrContext* swr_ctx;
  std::vector<AVFrame*>* frame_vector;
  int vector_index;  // current reading frame index in vector
  std::mutex vector_mutex;
  std::condition_variable vector_cond;
  pthread_t play_tid;
  bool stop_play;
  bool fatal_error_occur;
  int repeat_times;
  int repeated;

 public:
  AudioPlayer();
  ~AudioPlayer();
  int init(char* audio_file_path);
  int set_cache_flag(int cache);
  bool get_cache_flag();
  void set_auto_delete(bool auto_del) { auto_delete = auto_del; }
  bool get_auto_delete() { return auto_delete; }
  WantedAudioParams* get_tgt_params();
  AVFrame* get_next_frame();
  int convert_frame(AVFrame* in_frame, AVFrame** out_frame);
  void set_fatal_error();
  int play(bool sync = false);
  void stop_immediately();
  void set_repeat_times(int times) { repeat_times = times; }
  bool play_again() {
    return repeat_times < 0 || (repeat_times > 0 && repeated < repeat_times);
  }
  bool reset_reading() {
    if (0 > dd.rewind()) {
      fprintf(stderr, "audio reading rewind failed!\n");
      return false;
    }
    return true;
  }
};

static std::atomic_bool audio_outing(false);

static void playing(AudioPlayer* player) {
  while (1) {
    AVFrame* frame = player->get_next_frame();
    if (!frame)
      break;
    default_push_play_frame(frame, 1);
    if (!player->get_cache_flag())
      av_frame_free(&frame);
  }
  default_flush_play();
}

static void* audio_refresh_thread(void* arg) {
  AudioPlayer* player = (AudioPlayer*)arg;

  prctl(PR_SET_NAME, "audio_refresh", 0, 0, 0);
  playing(player);
  if (player->get_auto_delete()) {
    printf("audio_refresh_thread delete %p.\n", arg);
    pthread_detach(pthread_self());
    // if non-cache, delete when completing playing
    delete player;
  }
  audio_outing = false;
  // printf("out of audio_refresh_thread\n");
  return nullptr;
}

static void clear_frame_vector(std::vector<AVFrame*>* frame_vector) {
  if (!frame_vector)
    return;
  while (!frame_vector->empty()) {
    AVFrame* frame = frame_vector->back();
    av_frame_free(&frame);
    frame_vector->pop_back();
  }
}

static void free_frame_vector(std::vector<AVFrame*>** vector,
                              std::mutex* mutex) {
  std::unique_lock<std::mutex> lk(*mutex);
  if (vector && *vector) {
    std::vector<AVFrame*>* frame_vector = *vector;
    clear_frame_vector(frame_vector);
    delete frame_vector;
    *vector = nullptr;
  }
}

AudioPlayer::AudioPlayer()
    : cache_all(false),
      auto_delete(false),
      tgt_params(nullptr),
      swr_ctx(nullptr),
      frame_vector(nullptr),
      vector_index(-1),
      play_tid(0),
      stop_play(true),
      fatal_error_occur(false),
      repeat_times(0),
      repeated(0) {
  memset(&src_params, 0, sizeof(src_params));
}

AudioPlayer::~AudioPlayer() {
  stop_immediately();
  free_frame_vector(&frame_vector, &vector_mutex);
  swr_free(&swr_ctx);
}

int AudioPlayer::init(char* audio_file_path) {
  int ret = dd.init(audio_file_path, VIDEO_DISABLE_FLAG);
  if (0 == ret) {
    tgt_params = default_wanted_audio_params();
    src_params = *tgt_params;
  }
  return ret;
}

int AudioPlayer::set_cache_flag(int cache) {
  cache_all = cache > 0;
  free_frame_vector(&frame_vector, &vector_mutex);
  if (cache_all) {
    frame_vector = new std::vector<AVFrame*>();
    if (!frame_vector) {
      fprintf(stderr, "set_cache failed, no memory!\n");
      return AVERROR(ENOMEM);
    }
  }
  return 0;
}

bool AudioPlayer::get_cache_flag() {
  return cache_all;
}

WantedAudioParams* AudioPlayer::get_tgt_params() {
  return tgt_params;
}

int AudioPlayer::convert_frame(AVFrame* in_frame, AVFrame** out_frame) {
  int data_size, resampled_data_size;
  uint64_t dec_channel_layout;
  AVFrame* frame = nullptr;

  data_size = get_buffer_size(in_frame);
  dec_channel_layout = get_channel_layout(in_frame);

  if (in_frame->format != src_params.fmt ||
      dec_channel_layout != src_params.channel_layout ||
      in_frame->sample_rate != src_params.sample_rate) {
    swr_free(&swr_ctx);
    swr_ctx = swr_alloc_set_opts(
        NULL, tgt_params->channel_layout, (enum AVSampleFormat)tgt_params->fmt,
        tgt_params->sample_rate, dec_channel_layout,
        (enum AVSampleFormat)in_frame->format, in_frame->sample_rate, 0, NULL);
    if (!swr_ctx || swr_init(swr_ctx) < 0) {
      av_log(NULL, AV_LOG_ERROR,
             "Cannot create sample rate converter for "
             "conversion of %d Hz %s %d channels to %d Hz "
             "%s %d channels!\n",
             in_frame->sample_rate,
             av_get_sample_fmt_name((enum AVSampleFormat)in_frame->format),
             av_frame_get_channels(in_frame), tgt_params->sample_rate,
             av_get_sample_fmt_name((enum AVSampleFormat)tgt_params->fmt),
             av_get_channel_layout_nb_channels(tgt_params->channel_layout));
      swr_free(&swr_ctx);
      return -1;
    }

    src_params.channel_layout = dec_channel_layout;
    src_params.sample_rate = in_frame->sample_rate;
    src_params.fmt = (enum AVSampleFormat)in_frame->format;
  }
  if (swr_ctx) {
    // PRINTF("do audio swr convert\n");
    frame = av_frame_alloc();
    if (!frame) {
      av_log(NULL, AV_LOG_ERROR, "frame alloc failed, no memory!\n");
      return -1;
    }
    frame->format = tgt_params->fmt;
    frame->channel_layout = tgt_params->channel_layout;
    frame->sample_rate = tgt_params->sample_rate;
    if (swr_convert_frame(swr_ctx, frame, in_frame)) {
      fprintf(stderr, "convert_frame failed!\n");
      goto err;
    }
    av_frame_free(&in_frame);
    *out_frame = frame;
    resampled_data_size = get_buffer_size(frame);
  } else {
    *out_frame = in_frame;
    resampled_data_size = data_size;
  }

  return resampled_data_size;

err:
  av_frame_free(&frame);
  return -1;
}

AVFrame* AudioPlayer::get_next_frame() {
  std::unique_lock<std::mutex> lk(vector_mutex);
  if (stop_play)
    return nullptr;
  if (cache_all) {
    if (dd.is_finished()) {
      audio_outing = false;
      if (!frame_vector || frame_vector->empty()) {
        fprintf(stderr,
                "!!file is end, however frame array is empty. Is "
                "it audio file?\n");
        return nullptr;
      }
      int idx = vector_index;
      if (idx + 2 >= (int)frame_vector->size()) {
        if (!play_again()) {
          vector_cond.wait(lk);
          if (stop_play)
            return frame_vector->at(frame_vector->size() - 1);
        } else
          repeated++;
        idx = 0;
      } else {
        idx++;
      }
      vector_index = idx;
      return frame_vector->at(idx);
    }
  }

  AVFrame* frame = av_frame_alloc();
  if (!frame) {
    fprintf(stderr, "frame_alloc failed, no memory!\n");
    return nullptr;
  }
  int got_frame = 0;
  int is_video = 0;
  while (!got_frame) {
    int ret =
        dd.read_and_decode_one_frame(nullptr, frame, &got_frame, &is_video);
    UNUSED(ret);
    if (dd.is_finished()) {
      bool again = play_again();
      audio_outing = again;
      if (cache_all) {
        if (!again)
          vector_cond.wait(lk);
        else
          repeated++;
        frame_vector->push_back(frame);  // zero length
        return frame;
      } else {
        if (again) {
          if (!reset_reading())
            return nullptr;
          repeated++;
        } else {
          stop_play = true;
          break;
        }
      }
    }
  }
  if (got_frame) {
    AVFrame* out_frame = nullptr;
    assert(frame->data == frame->extended_data);
    if (0 > convert_frame(frame, &out_frame)) {
      return nullptr;
    }
    frame = out_frame;

    if (cache_all) {
      // FFMPEG will free the data buffer in frame, clone this frame.
      AVFrame* new_frame = av_frame_clone(frame);
      if (!new_frame) {
        fprintf(stderr, "av_frame_clone failed, no memory!\n");
        av_frame_free(&frame);
        return nullptr;
      }
      av_frame_free(&frame);
      frame = new_frame;
      frame_vector->push_back(frame);
    }
  } else {
    av_frame_free(&frame);
  }
  return frame;
}

void AudioPlayer::set_fatal_error() {
  std::unique_lock<std::mutex> _lk(vector_mutex);
  stop_play = true;
  fatal_error_occur = true;
  audio_outing = false;
  if (play_tid) {
    pthread_detach(play_tid);  // let it selfdestory
    play_tid = 0;
  }
  fprintf(stderr, "Warning: AudioPlayer::set_fatal_error\n");
}

int AudioPlayer::play(bool sync) {
  if (audio_outing) {
    PRINTF("another audio file is playing\n");
    return -1;
  }
  int ret = 0;
  std::unique_lock<std::mutex> _lk(vector_mutex);
  if (fatal_error_occur) {
    fprintf(stderr,
            "audio fatal error occured, should recreate AudioObject?\n");
    return -1;
  }
  stop_play = false;
  if (cache_all) {
    vector_index = -1;
    vector_cond.notify_one();
  } else {
    if (!reset_reading())
      return -1;
  }
  if (sync) {
    vector_mutex.unlock();
    playing(this);
    vector_mutex.lock();
    audio_outing = false;
  } else {
    if (play_tid == 0) {
      pthread_attr_t attr;
      if (pthread_attr_init(&attr)) {
        fprintf(stderr, "pthread_attr_init failed!\n");
        return -1;
      }
      if (pthread_attr_setstacksize(&attr, 48 * 1024)) {
        pthread_attr_destroy(&attr);
        return -1;
      }
      if (pthread_create(&play_tid, &attr, audio_refresh_thread, this)) {
        fprintf(stderr, "audio_refresh_thread create failed!\n");
        ret = -1;
      }
      if (auto_delete)
        play_tid = 0;
      pthread_attr_destroy(&attr);
    }
    audio_outing = !ret;
  }
  return ret;
}

void AudioPlayer::stop_immediately() {
  std::unique_lock<std::mutex> lk(vector_mutex);
  stop_play = true;
  if (cache_all)
    vector_cond.notify_one();
  pthread_t tid = play_tid;
  if (tid) {
    vector_mutex.unlock();
    pthread_join(tid, NULL);
    vector_mutex.lock();
    play_tid = 0;
  }
}

int audio_play_init(void** handle,
                    char* audio_file_path,
                    int cache_flag,
                    int auto_delete) {
  if (cache_flag && auto_delete) {
    perror("AudioPlay: Do not support auto-delete for cache mode.\n");
    return -1;
  }
  AudioPlayer* player = new AudioPlayer();
  if (!player) {
    fprintf(stderr, "no memory\n");
    return -1;
  }
  if (0 > player->init(audio_file_path) ||
      0 > player->set_cache_flag(cache_flag)) {
    delete player;
    return -1;
  }
  player->set_auto_delete(!!auto_delete);
  *handle = player;
  return 0;
}

void audio_play_deinit(void* handle) {
  if (handle) {
    AudioPlayer* player = (AudioPlayer*)handle;
    delete player;
  }
}

int audio_play(void* handle, int repeat) {
  AudioPlayer* player = (AudioPlayer*)handle;
  if (player)
    player->set_repeat_times(repeat);
  return player ? player->play() : -1;
}

void audio_stop_repeat_play(void* handle) {
  AudioPlayer* player = (AudioPlayer*)handle;
  if (player)
    player->set_repeat_times(0);
}

int audio_play0(char* audio_file_path) {
  void* handle = nullptr;
  int ret = audio_play_init(&handle, audio_file_path, 0, 1);
  if (handle) {
    ret = audio_play(handle, 0);
    // For non cache frame playing,
    // handle will auto deinit when completing playing.
    if (ret < 0) {
      delete (AudioPlayer*)handle;
    }
  }
  return ret;
}

int audio_sync_play(char* audio_file_path) {
  AudioPlayer* player = nullptr;
  int ret =
      audio_play_init(reinterpret_cast<void**>(&player), audio_file_path, 0, 0);
  if (player) {
    ret = player->play(true);
    delete player;
  }
  return ret;
}
