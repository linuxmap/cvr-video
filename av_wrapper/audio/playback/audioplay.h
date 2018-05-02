// by wangh
#ifndef AUDIOPLAY_H
#define AUDIOPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

// If audio file is very small, cache_flag can be 1, else should not set it 1.
int audio_play_init(void** handle,
                    char* audio_file_path,
                    int cache_flag,
                    int auto_delete);
void audio_play_deinit(void* handle);
// repeat: -1, repeat forever until deinit; >=0, repeat times.
int audio_play(void* handle, int repeat);
void audio_stop_repeat_play(void* handle);

// play some audio file which is not played frequently
int audio_play0(char* audio_file_path);

// Sync playing audio file.
int audio_sync_play(char* audio_file_path);

#ifdef __cplusplus
}
#endif

#endif  // AUDIOPLAY_H
