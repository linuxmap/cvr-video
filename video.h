#ifndef __VIDEO_H__
#define __VIDEO_H__

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

#define CMD_UPDATETIME          0
#define CMD_PHOTOEND                1

#define USE_1440P_CAMERA 0

#define ISP_SP_WIDTH 640
#define ISP_SP_HEIGHT 360

#define MJPG_SAVE_THUMBNAIL 0

#define VIDEO_STATE_STOP    0
#define VIDEO_STATE_PREVIEW 1
#define VIDEO_STATE_RECORD  2

#define UVC_FROM_ISP 1
#define UVC_FROM_DSP 0
#define UVC_FROM_CIF 1
#define UVC_FROM_USB 0

void video_record_getfilename(char *str, unsigned short str_len,
                              const char *path, int ch, const char *filetype,
                              const char *suffix);

int video_record_addvideo(int id, struct video_param *front, struct video_param *back,
                          struct video_param *cif, char rec_immediately, char check_record_init);
int video_record_deletevideo(int deviceid);
void video_record_init(struct video_param *front,
                       struct video_param *back,
                       struct video_param *cif);
void video_record_deinit(bool black);
int video_record_startrec(void);
void video_record_stoprec(void);
void video_record_savefile(void);
void video_record_stop_savecache();
void video_record_switch_next_recfile(const char* filename);
void video_record_blocknotify(int prev_num, int later_num);

void video_record_setaudio(int flag);
int video_record_set_power_line_frequency(int i);
int video_record_takephoto(unsigned int num);
void video_record_start_cache(int sec);
void video_record_stop_cache();

void video_record_reset_bitrate();

void video_record_start_ts_transfer(char *url, int device_id);
void video_record_stop_ts_transfer(char sync);
void video_record_attach_user_muxer(void *muxer, char *uri, int need_av);
void video_record_detach_user_muxer(void *muxer);
void video_record_attach_user_mdprocessor(void *processor, void *md_attr);
void video_record_detach_user_mdprocessor(void *processor);
void video_record_rate_change(void *processor, int low_frame_rate);
int REC_RegEventCallback(void (*call)(int cmd, void *msg0, void *msg1));
int ADAS_RegEventCallback(void (*call)(int cmd, void *msg0, void *msg1));
void set_video_init_complete_cb(void (*cb)(int arg0));
int video_record_get_list_num(void);
int video_record_get_front_resolution(struct video_param* frame, int count);
int video_record_get_back_resolution(struct video_param* frame, int count);
void video_record_get_user_noise(void);
void video_record_init_lock();
void video_record_destroy_lock();
int video_record_get_state();
void video_record_set_record_mode(bool mode);
int video_record_get_cif_resolution(struct video_param *frame, int count);
void video_record_update_osd_num_region(int onoff);
void video_record_watermark_refresh(void);
void video_record_update_time(uint32_t* src, uint32_t srclen);
void video_record_fps_count(void);
void video_record_display_switch();
void video_record_display_switch_to_front(int type);
void video_record_inc_nv12_raw_cnt(void);
int video_record_set_white_balance(int i);
int video_record_set_exposure_compensation(int i);
int video_record_set_brightness(int i);
int video_record_set_contrast(int i);
int video_record_set_hue(int i);
int video_record_set_saturation(int i);
bool video_record_get_record_mode(void);
void video_record_update_license(uint32_t* src, uint32_t srclen);
void video_record_set_timelapseint(unsigned short val);
void video_record_set_cif_mirror(bool mirror);
bool video_record_get_cif_mirror(void);
void yuv420_display_adas(int width, int height, void* dstbuf);
void video_dvs_enable(int enable);
bool video_dvs_is_enable(void);
void video_record_set_fps(int fps);
void video_record_get_deviceid_by_type(int *devicdid, int type, int cnt);
void video_record_reset_uvc_position();
void video_record_set_uvc_position(int type);

#ifdef __cplusplus
}
#endif

#endif
