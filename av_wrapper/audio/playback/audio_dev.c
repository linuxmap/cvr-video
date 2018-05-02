#include "audio_dev.h"
#include <autoconfig/main_app_autoconf.h>
#include <libavutil/time.h>
#include <pthread.h>

#include "parameter.h"

#define AUDIO_MIN_BUFFER_SIZE MAIN_APP_AUDIO_SAMPLE_NUM
#define AUDIO_MAX_CALLBACKS_PER_SEC 30

static int ALSA_finalize_hardware(snd_pcm_t* pcm_handle,
                                  uint32_t* samples,
                                  int sample_size,
                                  snd_pcm_hw_params_t* hwparams,
                                  snd_pcm_uframes_t* period_size)
{
    int status;
    snd_pcm_uframes_t bufsize;
    int ret = 0;
    /* "set" the hardware with the desired parameters */
    status = snd_pcm_hw_params(pcm_handle, hwparams);
    if (status < 0) {
        av_log(NULL, AV_LOG_ERROR, "Couldn't set pcm_hw_params: %s\n",
               snd_strerror(status));
        ret = -1;
        goto err;
    }

    /* Get samples for the actual buffer size */
    status = snd_pcm_hw_params_get_buffer_size(hwparams, &bufsize);
    if (status < 0) {
        av_log(NULL, AV_LOG_ERROR, "Couldn't get buffer size: %s\n",
               snd_strerror(status));
        ret = -1;
        goto err;
    }
    if (bufsize != (*samples) * sample_size) {
        fprintf(stderr, "error, bufsize != (*samples) * %d; %lu != %u*%d\n",
                sample_size, bufsize, *samples, sample_size);
        ret = -1;
        goto err;
    }

    /* FIXME: Is this safe to do? */
//*samples = bufsize / 2;
err:
    snd_pcm_hw_params_get_period_size(hwparams, period_size, NULL);
#ifdef DEBUG
    /* This is useful for debugging */
    do {
        unsigned int periods = 0;
        snd_pcm_hw_params_get_periods(hwparams, &periods, NULL);
        fprintf(stderr,
                "ALSA: period size = %ld, periods = %u, buffer size = %lu\n",
                *period_size, periods, bufsize);
    } while (0);
#endif
    return ret;
}

static int ALSA_set_period_size(snd_pcm_t* pcm_handle,
                                uint32_t* samples,
                                int sample_size,
                                snd_pcm_hw_params_t* params,
                                snd_pcm_uframes_t* period_size /* out */)
{
    int status;
    snd_pcm_hw_params_t* hwparams;
    snd_pcm_uframes_t frames;
    unsigned int periods;

    /* Copy the hardware parameters for this setup */
    snd_pcm_hw_params_alloca(&hwparams);
    snd_pcm_hw_params_copy(hwparams, params);

    frames = MAIN_APP_AUDIO_SAMPLE_NUM;  //*samples;
    status = snd_pcm_hw_params_set_period_size_near(pcm_handle, hwparams, &frames,
                                                    NULL);
    if (status < 0) {
        av_log(NULL, AV_LOG_ERROR, "Couldn't set period size : %s\n",
               snd_strerror(status));
        return (-1);
    }

    periods = ((*samples) >> 10) * sample_size;
    status =
        snd_pcm_hw_params_set_periods_near(pcm_handle, hwparams, &periods, NULL);
    if (status < 0) {
        av_log(NULL, AV_LOG_ERROR, "Couldn't set periods : %s\n",
               snd_strerror(status));
        return (-1);
    }

    return ALSA_finalize_hardware(pcm_handle, samples, sample_size, hwparams,
                                  period_size);
}

static int ALSA_set_buffer_size(snd_pcm_t* pcm_handle,
                                uint32_t* samples,
                                int sample_size,
                                snd_pcm_hw_params_t* params,
                                snd_pcm_uframes_t* period_size /* out */)
{
    int status;
    snd_pcm_hw_params_t* hwparams;
    snd_pcm_uframes_t frames;
    /* Copy the hardware parameters for this setup */
    snd_pcm_hw_params_alloca(&hwparams);
    snd_pcm_hw_params_copy(hwparams, params);

    frames = (*samples) * sample_size;
    status =
        snd_pcm_hw_params_set_buffer_size_near(pcm_handle, hwparams, &frames);
    if (status < 0) {
        av_log(NULL, AV_LOG_ERROR, "Couldn't set buffer size : %s\n",
               snd_strerror(status));
        return (-1);
    }

    return ALSA_finalize_hardware(pcm_handle, samples, sample_size, hwparams,
                                  period_size);
}

static void show_available_sample_formats(snd_pcm_t* handle,
                                          snd_pcm_hw_params_t* params)
{
    snd_pcm_format_t format;

    fprintf(stderr, "Available formats:\n");
    for (format = 0; format <= SND_PCM_FORMAT_LAST; format++) {
        if (snd_pcm_hw_params_test_format(handle, params, format) == 0)
            fprintf(stderr, "- %s\n", snd_pcm_format_name(format));
    }
}

inline snd_pcm_format_t audio_convert_fmt(enum AVSampleFormat fmt)
{
    switch (fmt) {
    case AV_SAMPLE_FMT_S16:
        return SND_PCM_FORMAT_S16_LE;
    default:
        av_log(NULL, AV_LOG_ERROR, "TODO support other sample format, %d\n", fmt);
        return -1;
    }
}

int audio_dev_open(enum AVSampleFormat fmt,
                   uint64_t wanted_channel_layout,
                   int wanted_sample_rate,
                   AudioParams* audio_hw_params,
                   const char* dev_string)
{
    int status;
    snd_pcm_t* pcm_handle = NULL;
    snd_pcm_hw_params_t* hwparams = NULL;
    snd_pcm_sw_params_t* swparams = NULL;
    snd_pcm_format_t pcm_fmt;
    int wanted_nb_channels;
    uint32_t rate = 0;
    int sample_size = 0;
    pcm_handle = audio_hw_params->pcm_handle;
    if (pcm_handle)
        return 0;

    memset(audio_hw_params, 0, sizeof(*audio_hw_params));
#ifdef DEBUG
    printf(
        "wanted_fmt: %d, wanted_channel_layout: %lld, wanted_sample_rate: %d\n",
        fmt, wanted_channel_layout, wanted_sample_rate);
#endif
    audio_hw_params->wanted_params.fmt = fmt;
    audio_hw_params->wanted_params.channel_layout = wanted_channel_layout;
    audio_hw_params->wanted_params.sample_rate = wanted_sample_rate;
    pcm_fmt = audio_convert_fmt(fmt);
    status = snd_pcm_open(&pcm_handle, dev_string, SND_PCM_STREAM_PLAYBACK, 0);
    if (status < 0 || !pcm_handle) {
        av_log(NULL, AV_LOG_ERROR, ("audio open error: %s\n"),
               snd_strerror(status));
        goto err;
    }
    /* Figure out what the hardware is capable of */
    snd_pcm_hw_params_alloca(&hwparams);
    if (!hwparams) {
        fprintf(stderr, "hwparams alloca failed, no memory!\n");
        goto err;
    }
    status = snd_pcm_hw_params_any(pcm_handle, hwparams);
    if (status < 0) {
        av_log(NULL, AV_LOG_ERROR, "Couldn't get hardware config: %s\n",
               snd_strerror(status));
        goto err;
    }
#if 0  // def DEBUG
    {
        snd_output_t *log;
        snd_output_stdio_attach(&log, stderr, 0);
        // fprintf(stderr, "HW Params of device \"%s\":\n",
        //        snd_pcm_name(pcm_handle));
        fprintf(stderr, "--------------------\n");
        snd_pcm_hw_params_dump(hwparams, log);
        fprintf(stderr, "--------------------\n");
        snd_output_close(log);
    }
#endif
    status = snd_pcm_hw_params_set_access(pcm_handle, hwparams,
                                          SND_PCM_ACCESS_RW_INTERLEAVED);
    if (status < 0) {
        av_log(NULL, AV_LOG_ERROR, "Couldn't set interleaved access: %s\n",
               snd_strerror(status));
        goto err;
    }
    status = snd_pcm_hw_params_set_format(pcm_handle, hwparams, pcm_fmt);
    if (status < 0) {
        av_log(NULL, AV_LOG_ERROR, "Couldn't find any hardware audio formats\n");
        show_available_sample_formats(pcm_handle, hwparams);
        goto err;
    }
    audio_hw_params->pcm_format = pcm_fmt;
    wanted_nb_channels = av_get_channel_layout_nb_channels(wanted_channel_layout);
    status =
        snd_pcm_hw_params_set_channels(pcm_handle, hwparams, wanted_nb_channels);
    if (status < 0) {
        uint32_t channels;
        av_log(NULL, AV_LOG_WARNING, "Couldn't set audio channels1: %s\n",
               snd_strerror(status));
        status = snd_pcm_hw_params_get_channels(hwparams, &channels);
        if (status < 0) {
            av_log(NULL, AV_LOG_WARNING, "Couldn't set audio channels2: %s\n",
                   snd_strerror(status));
            status = snd_pcm_hw_params_get_channels_min(hwparams, &channels);
            if (status < 0) {
                av_log(NULL, AV_LOG_ERROR, "Couldn't set audio channels3: %s\n",
                       snd_strerror(status));
                goto err;
            }
        }
        wanted_nb_channels = channels;
        wanted_channel_layout = av_get_default_channel_layout(channels);
    }
    rate = wanted_sample_rate;
    status = snd_pcm_hw_params_set_rate_near(pcm_handle, hwparams, &rate, NULL);
    if (status < 0) {
        av_log(NULL, AV_LOG_ERROR, "Couldn't set audio frequency: %s\n",
               snd_strerror(status));
        goto err;
    }
    wanted_sample_rate = rate;
    audio_hw_params->pcm_samples =
        FFMIN(AUDIO_MIN_BUFFER_SIZE,
              2 << av_log2(wanted_sample_rate / AUDIO_MAX_CALLBACKS_PER_SEC));

    sample_size =
        wanted_nb_channels * (snd_pcm_format_physical_width(pcm_fmt) >> 3);
    snd_pcm_uframes_t period_size = 0;
    /* Set the buffer size, in samples */
    if (ALSA_set_period_size(pcm_handle, &audio_hw_params->pcm_samples,
                             sample_size, hwparams, &period_size) < 0) {
        if (ALSA_set_buffer_size(pcm_handle, &audio_hw_params->pcm_samples,
                                 sample_size, hwparams, &period_size) < 0) {
            goto err;
        }
    }
    audio_hw_params->period_size = period_size;
    snd_pcm_sw_params_alloca(&swparams);
    if (!swparams) {
        fprintf(stderr, "swparams alloca failed, no memory!\n");
        goto err;
    }
    status = snd_pcm_sw_params_current(pcm_handle, swparams);
    if (status < 0) {
        av_log(NULL, AV_LOG_ERROR, "Couldn't get software config: %s\n",
               snd_strerror(status));
        goto err;
    }

    status = snd_pcm_sw_params_set_avail_min(pcm_handle, swparams,
                                             /*period_size*/
                                             audio_hw_params->pcm_samples);
    if (status < 0) {
        av_log(NULL, AV_LOG_ERROR, "Couldn't set minimum available samples: %s\n",
               snd_strerror(status));
        goto err;
    }
    /* start the transfer when the buffer is almost full: */
    /* (buffer_size / avail_min) * avail_min */
    status = snd_pcm_sw_params_set_start_threshold(
                 pcm_handle, swparams, 1 /*(buffer_size / period_size) * period_size*/);
    if (status < 0) {
        av_log(NULL, AV_LOG_ERROR,
               "Unable to set start threshold mode for playback: %s\n",
               snd_strerror(status));
        goto err;
    }
    status = snd_pcm_sw_params(pcm_handle, swparams);
    if (status < 0) {
        av_log(NULL, AV_LOG_ERROR, "Couldn't set software audio parameters: %s\n",
               snd_strerror(status));
        goto err;
    }
    /* Switch to blocking mode for playback */
    // snd_pcm_nonblock(pcm_handle, 0);

    audio_hw_params->fmt = fmt;
    audio_hw_params->sample_rate = wanted_sample_rate;
    audio_hw_params->channel_layout = wanted_channel_layout;
    audio_hw_params->channels = wanted_nb_channels;
    audio_hw_params->frame_size = av_samples_get_buffer_size(
                                      NULL, audio_hw_params->channels, 1, audio_hw_params->fmt, 1);
    audio_hw_params->bytes_per_sec = av_samples_get_buffer_size(
                                         NULL, audio_hw_params->channels, audio_hw_params->sample_rate,
                                         audio_hw_params->fmt, 1);
    if (audio_hw_params->bytes_per_sec <= 0 || audio_hw_params->frame_size <= 0) {
        av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size failed\n");
        goto err;
    }
#ifdef DEBUG
    printf(
        "fmt: %d, freq: %d, channel_layout: %lld, channels: %d, "
        "bytes_per_sec: %d, frame_size: %d,"
        "pcm_samples: %u, snd_pcm_format_physical_width(pcm_fmt): %d\n",
        audio_hw_params->fmt, audio_hw_params->sample_rate,
        audio_hw_params->channel_layout, audio_hw_params->channels,
        audio_hw_params->bytes_per_sec, audio_hw_params->frame_size,
        audio_hw_params->pcm_samples, snd_pcm_format_physical_width(pcm_fmt));
#endif
    audio_hw_params->pcm_handle = pcm_handle;
    audio_hw_params->audio_hw_buf_size =
        (wanted_nb_channels * audio_hw_params->pcm_samples *
         snd_pcm_format_physical_width(pcm_fmt) / 8);
    set_playback_volume(parameter_get_key_tone_volume());
    return 0;

err:
    if (pcm_handle) {
        snd_pcm_drain(pcm_handle);
        snd_pcm_close(pcm_handle);
    }
    return -1;
}

void audio_dev_close(AudioParams* audio_hw_params)
{
    snd_pcm_t* pcm_handle = audio_hw_params->pcm_handle;
    if (pcm_handle) {
        snd_pcm_drain(pcm_handle);
        snd_pcm_close(pcm_handle);
        memset(audio_hw_params, 0, sizeof(*audio_hw_params));
        printf("audio playback close done\n");
    }
}

int audio_dev_write_mute(snd_pcm_sframes_t voiced_frames,
                         AudioParams* audio_hw_params,
                         uint8_t* mute_buffer)
{
    // mute_buffer length: audio_hw_params->period_size * audio_hw_params->frame_size
    int ret = 0;
    snd_pcm_sframes_t mute_frames = voiced_frames % audio_hw_params->period_size;
    if (mute_frames == 0)
        return 0;
    mute_frames = audio_hw_params->period_size - mute_frames;
    while (mute_frames > 0) {
        ret = audio_dev_write(audio_hw_params, mute_buffer, &mute_frames);
        if (ret < 0)
            break;
        else if (ret == 0)
            continue;
    }
    return ret;
}

int audio_dev_write(AudioParams* audio_hw_params,
                    const uint8_t* buf,
                    snd_pcm_sframes_t* frames_left)
{
    snd_pcm_t* pcm_handle = audio_hw_params->pcm_handle;
    int status = -1;
    if (pcm_handle) {
        snd_pcm_sframes_t frames = *frames_left;
        assert(frames > 0);
        status = snd_pcm_writei(pcm_handle, buf, frames);
        if (status < 0) {
            if (status == -EAGAIN) {
                /* Apparently snd_pcm_recover() doesn't handle this case - does it
                 * assume snd_pcm_wait() above? */
                av_usleep(100);
                return 0;
            }
            status = snd_pcm_recover(pcm_handle, status, 0);
            if (status < 0) {
                /* Hmm, not much we can do - abort */
                fprintf(stderr, "ALSA write failed (unrecoverable): %s\n",
                        snd_strerror(status));
                goto err;
            }
            goto err;
        }
        *frames_left -= status;
    }

err:
    return status;
}

int set_playback_volume(long volume)
{
    long min = 0, max = 0;
    snd_mixer_t* handle = NULL;
    snd_mixer_selem_id_t* sid = NULL;
    const char* card = "default";
    const char* selem_name = "Playback";
    snd_mixer_elem_t* elem = NULL;

    int ret = snd_mixer_open(&handle, 0);
    if (ret < 0)
        return ret;
    ret = snd_mixer_attach(handle, card);
    if (ret < 0)
        goto err;

    snd_mixer_selem_register(handle, NULL, NULL);
    snd_mixer_load(handle);

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, selem_name);

    elem = snd_mixer_find_selem(handle, sid);
    if (elem) {
        ret = snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
        if (ret < 0)
            goto err;
        ret = snd_mixer_selem_set_playback_volume_all(elem,
                                                      volume * (max - min) / 100);
        if (ret < 0) {
            fprintf(stderr, "set playback volume failed\n");
            goto err;
        }
    }
    snd_mixer_close(handle);
    return 0;

err:
    if (handle)
        snd_mixer_close(handle);
    return ret;
}

struct SwrContext* audio_init_swr_context(WantedAudioParams in_param,
                                          WantedAudioParams out_param)
{
    if (in_param.fmt == out_param.fmt &&
        in_param.channel_layout == out_param.channel_layout &&
        in_param.sample_rate == out_param.sample_rate)
        return NULL;  // convertion is not need
    struct SwrContext* swr_ctx = swr_alloc_set_opts(
                                     NULL, out_param.channel_layout, out_param.fmt, out_param.sample_rate,
                                     in_param.channel_layout, in_param.fmt, in_param.sample_rate, 0, NULL);
    if (!swr_ctx || swr_init(swr_ctx) < 0) {
        av_log(
            NULL, AV_LOG_ERROR,
            "Cannot create sample rate converter for "
            "conversion of %d Hz %s %d channels to %d Hz "
            "%s %d channels!\n",
            in_param.sample_rate, av_get_sample_fmt_name(in_param.fmt),
            av_get_channel_layout_nb_channels((uint64_t)in_param.channel_layout),
            out_param.sample_rate, av_get_sample_fmt_name(out_param.fmt),
            av_get_channel_layout_nb_channels((uint64_t)out_param.channel_layout));
        swr_free(&swr_ctx);
    }
    return swr_ctx;
}

void audio_deinit_swr_context(struct SwrContext* swr_ctx)
{
    swr_free(&swr_ctx);
}

inline uint64_t get_channel_layout(AVFrame* frame)
{
    return (frame->channel_layout &&
            av_frame_get_channels(frame) ==
            av_get_channel_layout_nb_channels(frame->channel_layout))
           ? frame->channel_layout
           : av_get_default_channel_layout(av_frame_get_channels(frame));
}

inline int get_buffer_size(AVFrame* frame)
{
    return av_samples_get_buffer_size(NULL, av_frame_get_channels(frame),
                                      frame->nb_samples,
                                      (enum AVSampleFormat)frame->format, 1);
}
