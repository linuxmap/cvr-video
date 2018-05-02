#ifndef __PARAMETER_H__
#define __PARAMETER_H__

#include <stdbool.h>
#include "common.h"

#define ADAS_BASELINE_COUNT             2

int parameter_init(void);
int parameter_flash_set_flashed(int flag);
int parameter_flash_get_flashed();
int parameter_sav_staewifiinfo(char* ssid, char* password);
int parameter_getwifistainfo(char* ssid, char* password);
int parameter_savewifipass(char* password);
int parameter_getwifiinfo(char* ssid, char* password);
int parameter_sav_wifi_mode(char mod);
char parameter_get_wifi_mode(void);
int parameter_recover(void);
int parameter_save_wb(char wb);
char parameter_get_wb(void);
int parameter_save_ex(char ex);
char parameter_get_ex(void);
int parameter_save_vcamresolution(char resolution);
int parameter_save_fcamresolution(char resolution);
char parameter_get_fcamresolution(void);
int parameter_save_bcamresolution(char resolution);
char parameter_get_bcamresolution(void);
int parameter_save_recodetime(int time);
int parameter_get_recodetime(void);
int parameter_save_abmode(char mode);
unsigned char parameter_get_abmode(void);
int parameter_save_movedete(char en);
char parameter_get_movedete(void);
int parameter_save_timemark(char en);
char parameter_get_timemark(void);
int parameter_save_voicerec(char en);
char parameter_get_voicerec();
int parameter_save();
char parameter_get_video_mark(void);
int parameter_save_video_mark(char resolution);
int parameter_save_video_audio(char resolution);
unsigned char parameter_get_video_audio(void);
int parameter_save_video_3dnr(char resolution);
char parameter_get_video_3dnr(void);
int parameter_save_video_adas(char adas);
char parameter_get_video_adas(void);
int parameter_save_video_adas_setting(char *setting);
char parameter_get_video_adas_setting(char *setting);
int parameter_save_video_adas_alert_distance(char setting);
char parameter_get_video_adas_alert_distance(void);
int parameter_save_video_adas_direction(char direction);
char parameter_get_video_adas_direction(void);
int parameter_save_video_de(char resolution);
unsigned char parameter_get_video_de(void);
int parameter_save_video_backlt(char resolution);
char parameter_get_video_pip(void);
int parameter_save_video_pip(char resolution);
char parameter_get_video_backlt(void);

int parameter_save_parkingmonitor(char resolution);
char parameter_get_parkingmonitor(void);
int parameter_save_video_fre(char resolution);
char parameter_get_video_fre(void);
int parameter_save_video_lan(char resolution);
int parameter_save_collision_level(char resolution);
char parameter_get_collision_level(void);
int parameter_save_leavecarrec(char resolution);
char parameter_get_leavecarrec(void);
int parameter_save_video_usb(char resolution);
char parameter_get_video_usb(void);
char* parameter_get_firmware_version();
char** parameter_get_licence_plate();
int parameter_save_licence_plate(char *licenseplate, int n);
char parameter_get_licence_plate_flag();
int parameter_save_licence_plate_flag(char flag);
char parameter_get_video_fre(void);

int parameter_save_video_fontcamera(char resolution);
char parameter_get_video_fontcamera(void);
int parameter_save_video_backcamera(char resolution);
char parameter_get_video_backcamera(void);

unsigned char parameter_get_vcamresolution(void);
int parameter_save_debug_reboot(char resolution);
char parameter_get_debug_reboot(void);
int parameter_save_debug_recovery(char resolution);
char parameter_get_debug_recovery(void);
int parameter_save_debug_standby(char resolution);
char parameter_get_debug_standby(void);
int parameter_save_debug_modechange(char resolution);
char parameter_get_debug_modechange(void);
int parameter_save_debug_video(char resolution);
char parameter_get_debug_video(void);
int parameter_save_debug_beg_end_video(char resolution);
char parameter_get_debug_beg_end_video(void);
int parameter_save_debug_photo(char resolution);
char parameter_get_debug_photo(void);
char parameter_get_debug_temp(void);
int parameter_save_debug_temp(char resolution);
char parameter_get_debug_effect_watermark(void);
int parameter_save_debug_effect_watermark(char value);
char parameter_get_debug_auto_delete(void);

char parameter_get_video_cifcamera(void);
int parameter_save_video_cifcamera_reso(struct video_param *cif_reso);
struct video_param *parameter_get_video_cifcamera_reso(void);
int parameter_save_video_cifcamera_all(char resolution, struct video_param reso);
int parameter_save_video_backcamera_reso(struct video_param *back_reso);
struct video_param *parameter_get_video_backcamera_reso(void);
int parameter_save_video_frontcamera_reso(struct video_param *front_reso);
struct video_param *parameter_get_video_frontcamera_reso(void);

int parameter_save_video_frontcamera_all(char resolution, struct video_param reso);
int parameter_save_video_backcamera_all(char resolution, struct video_param reso);
int parameter_save_cif_inputid(short cif_inputid);
short parameter_get_cif_inputid(void);
int parameter_save_video_autorec(char resolution);
char parameter_get_video_autorec(void);
int parameter_save_screenoff_time(short screenoff_time);
short parameter_get_screenoff_time(void);
int parameter_save_bit_rate_per_pixel(unsigned int val);
unsigned int parameter_get_bit_rate_per_pixel(void);
int parameter_save_wifi_en(char val);
char parameter_get_wifi_en(void);
int parameter_save_hot_share(char onoff);
char parameter_get_hot_share(void);
int parameter_save_video_idc(char idc);
char parameter_get_video_idc(void);
unsigned short parameter_get_time_lapse_interval();
int parameter_save_time_lapse_interval(unsigned short val);
int parameter_save_reboot_cnt();
void parameter_get_reboot_cnt();
int parameter_save_video_flip(char flip);
char parameter_get_video_flip(void);
char parameter_get_video_lan(void);
unsigned int parameter_flash_get_fwupdate_count(void);
const char* parameter_get_storage_format();
const char* parameter_get_audio_enc_format();

int parameter_save_cif_cam_resolutions(struct video_param *cif_reso_ptr);
const struct video_param *parameter_get_cif_cam_resolutions(void);
int parameter_save_back_cam_resolutions(struct video_param *back_reso_ptr);
const struct video_param *parameter_get_back_cam_resolutions(void);
int parameter_save_front_cam_resolutions(struct video_param *front_reso_ptr);
const struct video_param *parameter_get_front_cam_resolutions(void);


int parameter_save_cif_cam_resolutions_max(int max_num);
int parameter_get_cif_cam_resolutions_max(void);
int parameter_save_back_cam_resolutions_max(int max_num);
int parameter_get_back_cam_resolutions_max(void);
int parameter_save_front_cam_resolutions_max(int max_num);
int parameter_get_front_cam_resolutions_max(void);

int parameter_save_photo_resolutions(struct photo_param *photo_resos_ptr);
const struct photo_param *parameter_get_photo_resolutions(void);
int parameter_get_photo_resolutions_max(void);
int parameter_save_photo_resolutions_max(int max_num);
int parameter_save_video_fre(char resolution);
char parameter_get_video_fre(void);
int parameter_save_leavecarrec(char resolution);
char parameter_get_leavecarrec(void);
int parameter_save_video_carmode(char resolution);
char parameter_get_video_carmode(void);
int parameter_save_gyro_calibration_data(unsigned char *data);
char parameter_get_gyro_calibration_data(unsigned char *data);
int parameter_save_dvs_enabled(char resolution);
char parameter_get_dvs_enabled(void);
int parameter_save_photo_burst_num(int resolution);
int parameter_get_photo_burst_num(void);
int parameter_save_photo_param(const struct photo_param* param);
struct photo_param* parameter_get_photo_param(void);
int parameter_save_isp_max_resolution(unsigned int width, unsigned int height);
void parameter_get_isp_max_resolution(unsigned int *width, unsigned int *height);
char parameter_get_gps_mark(void);
int parameter_save_gps_mark(char resolution);
int parameter_get_gps_watermark(void);
int parameter_save_gps_watermark(int gps_watermark);
int parameter_save_cam_num(int num);
int parameter_get_cam_num(void);
unsigned char parameter_get_key_tone_volume(void);
int parameter_save_key_tone_volume(unsigned char vol);
void api_set_video_dvs_onoff(int onoff);
int api_set_video_dvs_calibration(void);
int parameter_save_photo_burst_state(bool is_photo_burst);
bool parameter_get_photo_burst_state(void);
int parameter_save_video_lapse_state(bool is_video_lapse);
bool parameter_get_video_lapse_state(void);
int parameter_get_photo_num(void);

#endif
