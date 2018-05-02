// by wangh
#ifndef AUDIO_DEV_H
#define AUDIO_DEV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <alloca.h>
#include <alsa/asoundlib.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>

typedef struct {
    enum AVSampleFormat fmt;
    uint64_t channel_layout;
    int sample_rate;
} WantedAudioParams;

typedef struct AudioParams {
    enum AVSampleFormat fmt;
    uint64_t channel_layout;
    int sample_rate;
    int channels;
    int frame_size;
    int bytes_per_sec;

    WantedAudioParams wanted_params;
    snd_pcm_t* pcm_handle;
    uint32_t pcm_samples;
    snd_pcm_format_t pcm_format;
    int audio_hw_buf_size;
    snd_pcm_uframes_t period_size;
} AudioParams;

snd_pcm_format_t audio_convert_fmt(enum AVSampleFormat fmt);
int audio_dev_open(enum AVSampleFormat fmt,
                   uint64_t wanted_channel_layout,
                   int wanted_sample_rate,
                   AudioParams* audio_hw_params,
                   const char* dev_string);
void audio_dev_close(AudioParams* audio_hw_params);
int audio_dev_write_mute(snd_pcm_sframes_t voiced_frames,
                         AudioParams* audio_hw_params,
                         uint8_t* mute_buffer);
int audio_dev_write(AudioParams* audio_hw_params,
                    const uint8_t* buf,
                    snd_pcm_sframes_t* frames_left);
int set_playback_volume(long volume);

struct SwrContext* audio_init_swr_context(WantedAudioParams in_param,
                                          WantedAudioParams out_param);
void audio_deinit_swr_context(struct SwrContext* swr_ctx);
uint64_t get_channel_layout(AVFrame* frame);
int get_buffer_size(AVFrame* frame);

#ifdef __cplusplus
}
#endif

#endif  // AUDIO_DEV_H
