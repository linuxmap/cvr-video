#ifndef __ALSA_CAPTURE_H__
#define __ALSA_CAPTURE_H__

#ifdef __cplusplus
extern "C" {
#endif

struct alsa_capture {
    int dev_id;
    int buffer_frames;
    unsigned int sample_rate;
    unsigned int channel;
    snd_pcm_t* handle;
    snd_pcm_hw_params_t* hw_params;
    snd_pcm_format_t format;
    snd_pcm_uframes_t period_size;
    snd_pcm_uframes_t buffer_size;
};

int alsa_capture_open(struct alsa_capture* capture, int devid);
int alsa_capture_doing(struct alsa_capture* capture, void* buff);
void alsa_capture_done(struct alsa_capture* capture);

#ifdef __cplusplus
}
#endif

#endif
