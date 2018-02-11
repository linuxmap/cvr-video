#include "parameter.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <pthread.h>
#include <netinet/in.h>
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>

#include <autoconfig/main_app_autoconf.h>
#include <init_hook/init_hook.h>
#include <public_interface.h>

#define ENABLE_FLASH_PARAM

#define PARAM_VERSION "1.0.8"
#define FIRMWARE_VERSION "SDK-V0.1.0 2016-9-7"

struct sys_param {
    char pararater_version[12];
    char firmware_version[30];
    char WIFI_SSID[33];
    char WIFI_PASS[65];
    char STA_WIFI_SSID[33];
    char STA_WIFI_PASS[65];
    char wifi_mode;
    char wifi_en;
    unsigned char vcam_resolution;
    char ccam_resolution;
    int recodetime;
    unsigned char abmode;
    char wb;
    char ex;
    char movedete;
    char timemark;
    char voicerec;
    // new add by lqw
    unsigned char video_de;
    char video_mark;
    unsigned char video_audioenable;
    char video_autorec;
    char video_lan;
    char video_fre;
    char video_3dnr;
    char video_adas;
    /* [0]: horizon  [1]: head_baseline */
    char video_adas_setting[ADAS_BASELINE_COUNT];
    char video_adas_direction;
    char video_adas_alert_distance;
    char video_backlt;
    char video_pip;
    char video_usb;
    char video_frontcamera;
    char video_backcamera;
    char video_cifcamera;
    char vcam_resolution_photo;
    char Debug_reboot;
    char Debug_recovery;
    struct video_param cif_reso;
    struct video_param back_reso;
    struct video_param front_reso;
    const struct video_param *back_resos_ptr;
    const struct video_param *front_resos_ptr;
    int cif_resos_max;
    int back_resos_max;
    int front_resos_max;
    short cif_inputid;
    char colli_level;
    char video_parkingmonitor;
    short screenoff_time;
    unsigned int bit_rate_per_pixel;
    char video_idc;
    char hot_share;
    /*
     * unit: second
     * no time lapse: 0;
     * 1s: 1; 5s: 5; 10s: 10; 30s: 30; 60s: 60.
     */
    unsigned short time_lapse_interval;
    unsigned char gyro_xl;
    unsigned char gyro_xh;
    unsigned char gyro_yl;
    unsigned char gyro_yh;
    unsigned char gyro_zl;
    unsigned char gyro_zh;
    unsigned char temp_l;
    unsigned char temp_h;
    char license_plate[8][3];
    char license_plate_flag;
    char video_flip;
    int reboot_cnt;
    long tv_sec;
    /* setting for the file format of recording video */
    char rec_file_format[8];
    /* setting for audio encode format, default aac */
    char rec_audio_enc_format[8];
    /* for sport dv*/
    char carmode;
    char dvs_enabled;
    struct photo_param photo_param;
    const struct photo_param *photo_resos_ptr;
    int photo_resos_max;
    int photo_burst_num;
    unsigned int isp_max_width;
    unsigned int isp_max_height;
    char gps_mark;
    int gps_watermark;
    int cam_num;
    /* key_vol = 0,close key sound play; key_vol > 0,set key sound volume */
    unsigned char key_tone_vol;
    bool photo_burst;
    bool video_lapse;
};

/*
 * we will storage flag in VENDOR_FLASH_ID when flush.
 * If flush, the flag will be set to 1.
 */
struct flash_param {
    int flashed_flag;
};

const struct video_param back_video_resolutions[_MAX_BACK_VIDEO_RES] = {
    {0, 0, 0},
    {0, 0, 0},
};

#ifdef CVR
const struct video_param front_video_resolutions[_MAX_FRONT_VIDEO_RES] = {
    {0, 0, 0},
    {0, 0, 0},
};
#else
const struct video_param front_video_resolutions[_MAX_FRONT_VIDEO_RES] = {
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
};
#endif

const struct photo_param photo_resolutions[_MAX_PHO_RES] = {
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
};

static struct flash_param g_flash_param;
static struct sys_param parameter;
static char debug_standby;
static char debug_mode_change;
static char debug_photo;
static char debug_video;
static char debug_beg_end_video;
static char debug_temp;
static char debug_effect_watermark;

// vendor parameter
#define VENDOR_REQ_TAG 0x56524551
#define VENDOR_READ_IO _IOW('v', 0x01, unsigned int)
#define VENDOR_WRITE_IO _IOW('v', 0x02, unsigned int)

#define VENDOR_SN_ID 1
#define VENDOR_WIFI_MAC_ID 2
#define VENDOR_LAN_MAC_ID 3
#define VENDOR_BLUETOOTH_ID 4

/* Storage parameter */
#define VENDOR_PARAMETER_ID 5
/* Change the id when flash firmware */
#define VENDOR_FW_UPDATE_ID 6
#define VENDOR_FW_TEST_COUNT_ID 7

#define VENDOR_DATA_SIZE (3 * 1024)  // 3k
#define VERDOR_DEVICE "/dev/vendor_storage"

typedef struct _RK_VERDOR_REQ {
    uint32_t tag;
    uint16_t id;
    uint16_t len;
    uint8_t data[VENDOR_DATA_SIZE];
} RK_VERDOR_REQ;

const struct video_param *get_back_video_resolutions(void) __attribute__((weak));
const struct video_param *get_front_video_resolutions(void) __attribute__((weak));
const struct photo_param *get_photo_resolutions(void) __attribute__((weak));

const struct video_param *get_back_video_resolutions(void)
{
    return back_video_resolutions;
}

const struct video_param *get_front_video_resolutions(void)
{
    return front_video_resolutions;
}

const struct photo_param *get_photo_resolutions(void)
{
    return photo_resolutions;
}

void rknand_print_hex_data(uint8_t* s, uint32_t* buf, uint32_t len)
{
    uint32_t i;
    RK_VERDOR_REQ* req = (RK_VERDOR_REQ*)buf;

    printf("%s\n", s);
    printf("tag:%x\n", req->tag);
    printf("id:%d\n", req->id);
    printf("len:%d\n", req->len);
    printf("data:");
    for (i = 0; i < req->len; i++)
        printf("%2x ", req->data[i]);

    printf("\n");
}

int parameter_vendor_read(int buffer_size, uint8_t* buffer, uint16_t vendor_id)
{
    int ret;
    RK_VERDOR_REQ req;

    int sys_fd = open(VERDOR_DEVICE, O_RDWR, 0);
    if (sys_fd < 0) {
        printf("vendor_storage open fail\n");
        return -2;
    }

    req.tag = VENDOR_REQ_TAG;
    req.id = vendor_id;
    /* max read length to read */
    req.len = buffer_size > VENDOR_DATA_SIZE ? VENDOR_DATA_SIZE : buffer_size;

    ret = ioctl(sys_fd, VENDOR_READ_IO, &req);
    /* return req->len is the real data length stored in the NV-storage */
    if (ret) {
        //  printf("vendor read error ret = %d\n", ret);
        close(sys_fd);
        return -1;
    }
    memcpy(buffer, req.data, req.len);
    // rknand_print_hex_data("vendor read:", (uint32_t *)(& req), req.len + 8);
    close(sys_fd);

    return 0;
}

int parameter_vendor_write(int buffer_size, uint8_t* buffer, uint16_t vendor_id)
{
    int ret;
    RK_VERDOR_REQ req;

    int sys_fd = open(VERDOR_DEVICE, O_RDWR, 0);
    if (sys_fd < 0) {
        printf("vendor_storage open fail\n");
        return -1;
    }

    req.tag = VENDOR_REQ_TAG;
    req.id = vendor_id;
    req.len = buffer_size > VENDOR_DATA_SIZE ? VENDOR_DATA_SIZE : buffer_size;
    memcpy(req.data, buffer, req.len);
    ret = ioctl(sys_fd, VENDOR_WRITE_IO, &req);
    if (ret) {
        printf("vendor write error\n");
        close(sys_fd);
        return -1;
    }
    // rknand_print_hex_data("vendor write:", (uint32_t *)(& req), req.len + 8);
    close(sys_fd);

    return 0;
}


int parameter_flash_write()
{
    return parameter_vendor_write(sizeof(struct flash_param),
                                  (uint8_t*)&g_flash_param, VENDOR_FW_UPDATE_ID);
}

int parameter_flash_read()
{
    return parameter_vendor_read(sizeof(struct flash_param),
                                 (uint8_t*)&g_flash_param, VENDOR_FW_UPDATE_ID);
}

int parameter_flash_set_flashed(int flag)
{
    g_flash_param.flashed_flag = flag;
    return parameter_flash_write();
}

/* if have flash flag, will return 1 */
int parameter_flash_get_flashed()
{
    int ret = -1;

    ret = parameter_flash_read();
    if (ret < 0)
        goto RET;
    ret = g_flash_param.flashed_flag;
RET:
    return ret;
}

int parameter_flash_clear_flashed()
{
    g_flash_param.flashed_flag = 0;
    return parameter_flash_write();
}

unsigned int parameter_flash_get_fwupdate_count(void)
{
    unsigned int fwupdate_count;
    int ret;

    ret = parameter_vendor_read(sizeof(fwupdate_count),
                                (uint8_t*)&fwupdate_count,
                                VENDOR_FW_TEST_COUNT_ID);
    if (ret < 0)
        return ret;

    return fwupdate_count;
}

int parameter_param_write()
{
    return parameter_vendor_write(sizeof(struct sys_param),
                                  (uint8_t*)&parameter, VENDOR_PARAMETER_ID);
}

int parameter_param_read()
{
    return parameter_vendor_read(sizeof(struct sys_param),
                                 (uint8_t*)&parameter, VENDOR_PARAMETER_ID);
}

int parameter_read_test(void)
{
    int ret;

    ret = parameter_param_read();

    printf("parameter_read_test = %d\n", ret);
    printf("pararater_version = %s\n", parameter.pararater_version);
    printf("WIFI_SSID = %s\n", parameter.WIFI_SSID);
    printf("WIFI_PASS = %s\n", parameter.WIFI_PASS);
    printf("parameter.video_fre = %d\n", parameter.video_fre);
    printf("parameter.recodetime = %d\n", parameter.recodetime);
    printf("parameter.video_audioenable = %d\n", parameter.video_audioenable);

    return 0;
}

int parameter_save()
{
    int ret;

    ret = parameter_param_write();
    if (ret < 0) {
        printf("vendor_storage_read_test fail\n");
        return -1;
    }

    return 0;
}

int parameter_sav_wifi_mode(char mod)
{
    parameter.wifi_mode = mod;
    return parameter_save();
}
char parameter_get_wifi_mode(void)
{
    return parameter.wifi_mode;
}

int parameter_savewifipass(char* password)
{
    memset(parameter.WIFI_PASS, 0, sizeof(parameter.WIFI_PASS));
    memcpy(parameter.WIFI_PASS, password, strlen(password));

    return parameter_save();
}

int parameter_sav_staewifiinfo(char* ssid, char* password)
{
    memset(parameter.STA_WIFI_SSID, 0, sizeof(parameter.STA_WIFI_SSID));
    memcpy(parameter.STA_WIFI_SSID, ssid, strlen(ssid));

    memset(parameter.STA_WIFI_PASS, 0, sizeof(parameter.STA_WIFI_PASS));
    memcpy(parameter.STA_WIFI_PASS, password, strlen(password));

    return parameter_save();
}

int parameter_getwifiinfo(char* ssid, char* password)
{
    memcpy(ssid, parameter.WIFI_SSID, sizeof(parameter.WIFI_SSID));
    memcpy(password, parameter.WIFI_PASS, sizeof(parameter.WIFI_PASS));

    return 0;
}

int parameter_getwifistainfo(char* ssid, char* password)
{
    memcpy(ssid, parameter.STA_WIFI_SSID, sizeof(parameter.STA_WIFI_SSID));
    memcpy(password, parameter.STA_WIFI_PASS, sizeof(parameter.STA_WIFI_PASS));

    return 0;
}

int parameter_save_wb(char wb)
{
    parameter.wb = wb;
    return parameter_save();
}

char parameter_get_wb(void)
{
    return parameter.wb;
}

int parameter_save_ex(char ex)
{
    parameter.ex = ex;
    return parameter_save();
}

char parameter_get_ex(void)
{
    return parameter.ex;
}

int parameter_save_vcamresolution(char resolution)
{
    parameter.vcam_resolution = resolution;
    return parameter_save();
}

unsigned char parameter_get_vcamresolution(void)
{
    return parameter.vcam_resolution;
}

int parameter_save_video_idc(char idc)
{
    parameter.video_idc = idc;
    return parameter_save();
}

char parameter_get_video_idc(void)
{
    return parameter.video_idc;
}

int parameter_save_hot_share(char onoff)
{
    parameter.hot_share = onoff;
    return parameter_save();
}

char parameter_get_hot_share(void)
{
    return parameter.hot_share;
}

int parameter_save_video_flip(char flip)
{
    parameter.video_flip = flip;
    return parameter_save();
}

char parameter_get_video_flip(void)
{
    return parameter.video_flip;
}

//--------------------------------------------------
int parameter_save_video_3dnr(char resolution)
{
    parameter.video_3dnr = resolution;
    return parameter_save();
}

char parameter_get_video_3dnr(void)
{
    return parameter.video_3dnr;
}

int parameter_save_video_frontcamera_all(char resolution, struct video_param reso)
{
    parameter.video_frontcamera = resolution;
    parameter.front_reso.width  = reso.width;
    parameter.front_reso.height = reso.height;
    parameter.front_reso.fps    = reso.fps;
    return parameter_save();
}

int parameter_save_video_fontcamera(char resolution)
{
    parameter.video_frontcamera = resolution;
    return parameter_save();
}

char parameter_get_video_fontcamera(void)
{
    return parameter.video_frontcamera;
}

int parameter_save_video_frontcamera_reso(struct video_param *front_reso)
{
    parameter.front_reso.width  = front_reso->width;
    parameter.front_reso.height = front_reso->height;
    parameter.front_reso.fps    = front_reso->fps;
    return parameter_save();
}

struct video_param *parameter_get_video_frontcamera_reso(void)
{
    return &parameter.front_reso;
}

int parameter_save_video_cifcamera_all(char resolution, struct video_param reso)
{
    parameter.video_cifcamera   = resolution;
    parameter.cif_reso.width    = reso.width;
    parameter.cif_reso.height   = reso.height;
    parameter.cif_reso.fps      = reso.fps;
    return parameter_save();
}

int parameter_save_video_cifcamera(char resolution)
{
    parameter.video_cifcamera = resolution;
    return parameter_save();
}

char parameter_get_video_cifcamera(void)
{
    return parameter.video_cifcamera;
}

int parameter_save_video_cifcamera_reso(struct video_param *cif_reso)
{
    parameter.cif_reso.width    = cif_reso->width;
    parameter.cif_reso.height   = cif_reso->height;
    parameter.cif_reso.fps      = cif_reso->fps;
    return parameter_save();
}
struct video_param *parameter_get_video_cifcamera_reso(void)
{
    return &parameter.cif_reso;
}

int parameter_save_video_backcamera_all(char resolution, struct video_param reso)
{
    parameter.video_backcamera  = resolution;
    parameter.back_reso.width   = reso.width;
    parameter.back_reso.height  = reso.height;
    parameter.back_reso.fps     = reso.fps;
    return parameter_save();
}

int parameter_save_video_backcamera(char resolution)
{
    parameter.video_backcamera = resolution;
    return parameter_save();
}

char parameter_get_video_backcamera(void)
{
    return parameter.video_backcamera;
}

int parameter_save_video_backcamera_reso(struct video_param *back_reso)
{
    parameter.back_reso.width   = back_reso->width;
    parameter.back_reso.height  = back_reso->height;
    parameter.back_reso.fps     = back_reso->fps;
    return parameter_save();
}

struct video_param *parameter_get_video_backcamera_reso(void)
{
    return &parameter.back_reso;
}

int parameter_save_video_adas(char adas)
{
    parameter.video_adas = adas;
    return parameter_save();
}

char parameter_get_video_adas(void)
{
    return parameter.video_adas;
}

int parameter_save_video_adas_setting(char *setting)
{
    memcpy(parameter.video_adas_setting, setting, ADAS_BASELINE_COUNT);
    return parameter_save();
}

char parameter_get_video_adas_setting(char *setting)
{
    memcpy(setting, parameter.video_adas_setting, ADAS_BASELINE_COUNT);
    return 0;
}

int parameter_save_video_adas_alert_distance(char setting)
{
    parameter.video_adas_alert_distance = setting;
    return parameter_save();
}

char parameter_get_video_adas_alert_distance(void)
{
    return parameter.video_adas_alert_distance;
}

int parameter_save_video_adas_direction(char direction)
{
    parameter.video_adas_direction = direction;
    return 0;
}

char parameter_get_video_adas_direction(void)
{
    return parameter.video_adas_direction;
}

int parameter_save_video_de(char resolution)
{
    parameter.video_de = resolution;
    return parameter_save();
}

unsigned char parameter_get_video_de(void)
{
    return parameter.video_de;
}
int parameter_save_video_mark(char resolution)
{
    parameter.video_mark = resolution;
    return parameter_save();
}

char parameter_get_video_mark(void)
{
    return parameter.video_mark;
}

int parameter_save_gps_mark(char resolution)
{
    parameter.gps_mark = resolution;
    return parameter_save();
}

char parameter_get_gps_mark(void)
{
    return parameter.gps_mark;
}

int parameter_save_gps_watermark(int gps_watermark)
{
    parameter.gps_watermark = gps_watermark;
    return parameter_save();
}

int parameter_get_gps_watermark(void)
{
    return parameter.gps_watermark;
}

int parameter_save_video_audio(char resolution)
{
    parameter.video_audioenable = resolution;
    return parameter_save();
}

unsigned char parameter_get_video_audio(void)
{
    return parameter.video_audioenable;
}
int parameter_save_video_autorec(char resolution)
{
    parameter.video_autorec = resolution;
    return parameter_save();
}

char parameter_get_video_autorec(void)
{
    return parameter.video_autorec;
}
int parameter_save_video_lan(char resolution)
{
    parameter.video_lan = resolution;
    return parameter_save();
}

char parameter_get_video_lan(void)
{
    return parameter.video_lan;
}
int parameter_save_video_usb(char resolution)
{
    parameter.video_usb = resolution;
    return parameter_save();
}

char parameter_get_video_usb(void)
{
    return parameter.video_usb;
}

int parameter_save_video_fre(char resolution)
{
    parameter.video_fre = resolution;
    return parameter_save();
}

char parameter_get_video_fre(void)
{
    return parameter.video_fre;
}

int parameter_save_video_backlt(char resolution)
{
    parameter.video_backlt = resolution;
    return parameter_save();
}

char parameter_get_video_backlt(void)
{
    return parameter.video_backlt;
}

int parameter_save_gyro_calibration_data(unsigned char *data)
{
    parameter.gyro_xl = data[0];
    parameter.gyro_xh = data[1];
    parameter.gyro_yl = data[2];
    parameter.gyro_yh = data[3];
    parameter.gyro_zl = data[4];
    parameter.gyro_zh = data[5];
    parameter.temp_l = data[6];
    parameter.temp_h = data[7];

    return parameter_save();
}

char parameter_get_gyro_calibration_data(unsigned char *data)
{
    data[0] = parameter.gyro_xl;
    data[1] = parameter.gyro_xh;
    data[2] = parameter.gyro_yl;
    data[3] = parameter.gyro_yh;
    data[4] = parameter.gyro_zl;
    data[5] = parameter.gyro_zh;
    data[6] = parameter.temp_l;
    data[7] = parameter.temp_h;

    return 0;
}

int parameter_save_video_pip(char pip_status)
{
    parameter.video_pip = pip_status;

    return parameter_save();
}

char parameter_get_video_pip(void)
{
    return parameter.video_pip;
}

int parameter_save_collision_level(char resolution)
{
    parameter.colli_level = resolution;
    return parameter_save();
}

char parameter_get_collision_level(void)
{
    return parameter.colli_level;
}

int parameter_save_parkingmonitor(char resolution)
{
    parameter.video_parkingmonitor = resolution;
    return parameter_save();
}

char parameter_get_parkingmonitor(void)
{
    return parameter.video_parkingmonitor;
}

//------------------------------------------------------
int parameter_save_ccamresolution(char resolution)
{
    parameter.ccam_resolution = resolution;
    return parameter_save();
}

char parameter_get_ccamresolution(void)
{
    return parameter.ccam_resolution;
}

int parameter_save_recodetime(int time)
{
    parameter.recodetime = time;
    return parameter_save();
}

int parameter_get_recodetime(void)
{
    return parameter.recodetime;
}

int parameter_save_cif_inputid(short cif_inputid)
{
    parameter.cif_inputid = cif_inputid;
    return parameter_save();
}

short parameter_get_cif_inputid(void)
{
    return parameter.cif_inputid;
}

int parameter_save_abmode(char mode)
{
    parameter.abmode = mode;
    return parameter_save();
}

unsigned char parameter_get_abmode(void)
{
    return parameter.abmode;
}

int parameter_save_movedete(char en)
{
    parameter.movedete = en;
    return parameter_save();
}

char parameter_get_movedete(void)
{
    return parameter.movedete;
}

int parameter_save_timemark(char en)
{
    parameter.timemark = en;
    return parameter_save();
}

char parameter_get_timemark(void)
{
    return parameter.timemark;
}

int parameter_save_voicerec(char en)
{
    parameter.voicerec = en;
    return parameter_save();
}

char parameter_get_voicerec()
{
    return parameter.voicerec;
}

char* parameter_get_firmware_version()
{
    return parameter.firmware_version;
}

char* parameter_get_param_version()
{
    return parameter.pararater_version;
}

char** parameter_get_licence_plate()
{
    return (char **)parameter.license_plate;
}

int parameter_save_licence_plate(char *licenseplate, int n)
{
    memcpy((char *)parameter.license_plate, (char *)licenseplate, n * 3);
    return parameter_save();
}

char parameter_get_licence_plate_flag()
{
    return parameter.license_plate_flag;
}

int parameter_save_licence_plate_flag(char flag)
{
    parameter.license_plate_flag = flag;
    return parameter_save();
}

// debug---------------------------
int parameter_save_debug_reboot(char resolution)
{
    parameter.Debug_reboot = resolution;
    return parameter_save();
}

char parameter_get_debug_reboot(void)
{
    return parameter.Debug_reboot;
}
int parameter_save_debug_recovery(char resolution)
{
    parameter.Debug_recovery = resolution;
    return parameter_save();
}

char parameter_get_debug_recovery(void)
{
    return parameter.Debug_recovery;
}

int parameter_save_debug_standby(char resolution)
{
    debug_standby = resolution;
    return 0;
}

char parameter_get_debug_standby(void)
{
    return debug_standby;
}

int parameter_save_debug_modechange(char resolution)
{
    debug_mode_change = resolution;
    return 0;
}

char parameter_get_debug_modechange(void)
{
    return debug_mode_change;
}

int parameter_save_debug_video(char resolution)
{
    debug_video = resolution;
    return 0;
}

char parameter_get_debug_video(void)
{
    return debug_video;
}
int parameter_save_debug_beg_end_video(char resolution)
{
    debug_beg_end_video = resolution;
    return 0;
}

char parameter_get_debug_beg_end_video(void)
{
    return debug_beg_end_video;
}
int parameter_save_debug_photo(char resolution)
{
    debug_photo = resolution;
    return 0;
}

char parameter_get_debug_photo(void)
{
    return debug_photo;
}

int parameter_save_debug_temp(char resolution)
{
    debug_temp = resolution;
    return 0;
}

char parameter_get_debug_temp(void)
{
    return debug_temp;
}

int parameter_save_debug_effect_watermark(char value)
{
    debug_effect_watermark = value;
    return 0;
}

char parameter_get_debug_effect_watermark(void)
{
    return debug_effect_watermark;
}

/* this function is only for dv debug */
char parameter_get_debug_auto_delete(void)
{
    return 1;
}

int parameter_save_screenoff_time(short screenoff_time)
{
    parameter.screenoff_time = screenoff_time;
    return parameter_save();
}

short parameter_get_screenoff_time(void)
{
    return parameter.screenoff_time;
}

int parameter_save_bit_rate_per_pixel(unsigned int val)
{
    parameter.bit_rate_per_pixel = val;
    return parameter_save();
}

unsigned int parameter_get_bit_rate_per_pixel(void)
{
    return parameter.bit_rate_per_pixel;
}

int parameter_save_wifi_en(char val)
{
    parameter.wifi_en = val;
    return parameter_save();
}

char parameter_get_wifi_en(void)
{
    return parameter.wifi_en;
}

unsigned short parameter_get_time_lapse_interval()
{
    return parameter.time_lapse_interval;
}

int parameter_save_time_lapse_interval(unsigned short val)
{
    parameter.time_lapse_interval = val;
    return parameter_save();
}

const char* parameter_get_storage_format()
{
    return parameter.rec_file_format;
}

const char* parameter_get_audio_enc_format()
{
    return parameter.rec_audio_enc_format;
}

int parameter_save_video_carmode(char resolution)
{
    parameter.carmode = resolution;
    return parameter_save();
}

char parameter_get_video_carmode(void)
{
    return parameter.carmode;
}

int parameter_save_dvs_enabled(char resolution)
{
    parameter.dvs_enabled = resolution;
    return parameter_save();
}

char parameter_get_dvs_enabled(void)
{
    return parameter.dvs_enabled;
}

int parameter_save_photo_burst_num(int burst_num)
{
    parameter.photo_burst_num = burst_num;
    return parameter_save();
}

int parameter_get_photo_burst_num(void)
{
    return parameter.photo_burst_num;
}

int parameter_save_photo_param(const struct photo_param* param)
{
    parameter.photo_param.width = param->width;
    parameter.photo_param.height = param->height;
    parameter.photo_param.max_num = param->max_num;
    return parameter_save();
}

int parameter_get_photo_num(void)
{
    int i;

    for (i = 0; i < parameter.photo_resos_max; i++) {
        if ( parameter.photo_param.width == parameter.photo_resos_ptr[i].width
             && parameter.photo_param.height == parameter.photo_resos_ptr[i].height)
            return i;
    }

    return -1;
}

struct photo_param* parameter_get_photo_param(void)
{
    return &parameter.photo_param;
}

int parameter_save_isp_max_resolution(unsigned int width, unsigned int height)
{
    parameter.isp_max_width = width;
    parameter.isp_max_height = height;
    return parameter_save();
}

void parameter_get_isp_max_resolution(unsigned int *width, unsigned int *height)
{
    *width = parameter.isp_max_width;
    *height = parameter.isp_max_height;
}

int parameter_save_cam_num(int num)
{
    parameter.cam_num = num;
    return parameter_save();
}

int parameter_get_cam_num(void)
{
    return parameter.cam_num;
}

int parameter_save_key_tone_volume(unsigned char vol)
{
    parameter.key_tone_vol = vol;
    return parameter_save();
}

unsigned char parameter_get_key_tone_volume(void)
{
    return parameter.key_tone_vol;
}

int parameter_save_photo_burst_state(bool is_photo_burst)
{
    parameter.photo_burst = is_photo_burst;
    return parameter_save();
}

bool parameter_get_photo_burst_state(void)
{
    return parameter.photo_burst;
}

int parameter_save_video_lapse_state(bool is_video_lapse)
{
    parameter.video_lapse = is_video_lapse;
    return parameter_save();
}

bool parameter_get_video_lapse_state(void)
{
    return parameter.video_lapse;
}

// TODO: set more others from main_app_autoconf.h
#ifndef MAIN_APP_RECORD_FORMAT
#define MAIN_APP_RECORD_FORMAT "mp4"
#endif

#ifndef MAIN_APP_AUDIO_ENC_FORMAT
#define MAIN_APP_AUDIO_ENC_FORMAT "aac"
#endif

// set default values
int parameter_recover(void)
{
    printf("%s\n", __func__);

    int randnum;
    struct timeval tpstart;
    gettimeofday(&tpstart, NULL);
    srand(tpstart.tv_usec);
    randnum = (rand() + 0x1000) % 0x10000;
    memset((void*)&parameter, 0, sizeof(struct sys_param));

    snprintf(parameter.pararater_version, sizeof(parameter.pararater_version), "%s", PARAM_VERSION);
    snprintf(parameter.WIFI_SSID, sizeof(parameter.WIFI_SSID), "RK_CVR_%X", randnum);
    snprintf(parameter.WIFI_PASS, sizeof(parameter.WIFI_PASS), "%s", "123456789");
    parameter.video_fre = CAMARE_FREQ_50HZ;
    //parameter.recodetime = -1;  // 60sec
    parameter.recodetime = 60;  // 60sec

#ifdef CVR
    parameter.video_audioenable = 0;
#ifdef _PCBA_SERVICE_
    parameter.video_autorec = 0;
#else
    parameter.video_autorec = 1;
#endif
    parameter.video_idc = 1;
    parameter.video_3dnr = 1;
    parameter.dvs_enabled = 0;
    parameter.wifi_mode = 0;
#elif defined SAMPLE
    parameter.video_audioenable = 0;
#ifdef _PCBA_SERVICE_
    parameter.video_autorec = 0;
#else
    parameter.video_autorec = 1;
#endif
    parameter.video_idc = 1;
    parameter.video_3dnr = 1;
    parameter.dvs_enabled = 0;
    parameter.wifi_mode = 1;
#else
    parameter.video_audioenable = 1;
    parameter.video_autorec = 0;
    parameter.video_idc = 0;
    parameter.video_3dnr = 0;
    parameter.dvs_enabled = 1;
    parameter.wifi_mode = 0;
#endif
    parameter.hot_share = 0;
    parameter.video_adas = 0;
    parameter.video_backlt = LCD_BACKLT_M;
    parameter.video_pip = 1;
    parameter.vcam_resolution = 1;

    parameter.cif_reso.width = 720;
    parameter.cif_reso.height = 576;
    parameter.cif_reso.fps = 25;
    parameter.video_cifcamera = 0;

    parameter.back_reso.width = 640;
    parameter.back_reso.height = 480;
    parameter.back_reso.fps = 30;
    parameter.video_backcamera = 1;

    parameter.front_reso.width = 1920;
    parameter.front_reso.height = 1080;
    parameter.front_reso.fps = 30;
    parameter.video_frontcamera = 0;

    parameter.abmode = 2;
    parameter.vcam_resolution_photo = 1;
    parameter.screenoff_time = -1;
    parameter.wifi_en = 0;
    parameter.ex = 3;
    parameter.video_mark = 1;
    parameter.gps_mark = 0;
    parameter.gps_watermark = 0;
    snprintf(parameter.firmware_version, sizeof(parameter.firmware_version), "%s", FIRMWARE_VERSION);
    parameter.bit_rate_per_pixel = VIDEO_QUALITY_MID;
    parameter.time_lapse_interval = 0;
    memset(parameter.license_plate, 0, sizeof(parameter.license_plate));
    parameter.license_plate_flag = 0;
    parameter.video_adas_setting[0] = 50;
    parameter.video_adas_setting[1] = 95;
    parameter.video_adas_direction = 50;
    parameter.video_adas_alert_distance = 6;
    snprintf(parameter.rec_file_format, sizeof(parameter.rec_file_format), MAIN_APP_RECORD_FORMAT);
    snprintf(parameter.rec_audio_enc_format, sizeof(parameter.rec_audio_enc_format),
             MAIN_APP_AUDIO_ENC_FORMAT);

    parameter.video_flip = 0;
    parameter.reboot_cnt = 0;
    parameter.tv_sec = 0;

    /*
    * add for dv
    */
    parameter.carmode = 0;
    parameter.back_resos_ptr = get_back_video_resolutions();
    parameter.front_resos_ptr = get_front_video_resolutions();
    parameter.cif_resos_max = _MAX_CIF_VIDEO_RES;
    parameter.back_resos_max = _MAX_BACK_VIDEO_RES;
    parameter.front_resos_max = _MAX_FRONT_VIDEO_RES;

    parameter.photo_burst_num = 3;
    parameter.photo_param.width = 1920;
    parameter.photo_param.height = 1080;
    parameter.photo_param.max_num = 5;

    parameter.photo_resos_ptr = get_photo_resolutions();
    parameter.photo_resos_max = _MAX_PHO_RES;

    parameter.isp_max_width = 1280;
    parameter.isp_max_height = 720;
    parameter.cam_num = 5;
    parameter.key_tone_vol = 100;
    parameter.photo_burst = false;
    parameter.video_lapse = false;
    /* USB_MODE_MSC,USB_MODE_ADB, USB_MODE_UVC, USB_MODE_RNDIS */
#ifdef _PCBA_SERVICE_
    parameter.video_usb = USB_MODE_RNDIS;
#else
    parameter.video_usb = USB_MODE_MSC;
#endif

    return parameter_save();
}

int parameter_init(void)
{
    int ret = -1;
    char *version = NULL;

    /* If set flag in loader, will recovery reset it */
#ifdef ENABLE_FLASH_PARAM
    ret = parameter_flash_get_flashed();
    /* If we flash the firmware via upgrade tool, we need recover parameters */
    if (ret == BOOT_FLASHED) {
        parameter_recover();
        parameter_flash_clear_flashed();
        return 0;
    }
#endif

    ret = parameter_param_read();
#if MAIN_APP_ALWAYS_REWRITE_PARAMETER
    ret = -1;
#endif
    if (ret == -1)
        return parameter_recover();
    if (ret == -2)
        return -1;
    /* If version have been change, will recovery */
    version = parameter_get_param_version();
    if (strcmp(version, PARAM_VERSION) != 0)
        return parameter_recover();
    return 0;
}

int parameter_save_reboot_cnt()
{
    struct timeval t;
    gettimeofday(&t, NULL);
    parameter.reboot_cnt++;
    parameter.tv_sec = t.tv_sec;
    return parameter_save();
}

void parameter_get_reboot_cnt()
{
    printf("parameter.reboot_cnt: %d\n", parameter.reboot_cnt);
    printf("parameter.tv_sec: %ld\n", parameter.tv_sec);
}

const struct video_param *parameter_get_back_cam_resolutions(void)
{
    return parameter.back_resos_ptr;
}

int parameter_save_back_cam_resolutions(struct video_param *back_resos_ptr)
{
    if (back_resos_ptr != NULL) {
        parameter.back_resos_ptr = back_resos_ptr;
        return parameter_save();
    } else {
        return 0;
    }
}

const struct video_param *parameter_get_front_cam_resolutions(void)
{
    return parameter.front_resos_ptr;
}

int parameter_save_front_cam_resolutions(struct video_param *front_resos_ptr)
{
    if (front_resos_ptr != NULL) {
        parameter.front_resos_ptr = front_resos_ptr;
        return parameter_save();
    } else {
        return 0;
    }
}

int parameter_get_cif_cam_resolutions_max(void)
{
    return parameter.cif_resos_max;
}

int parameter_save_cif_cam_resolutions_max(int max_num)
{
    parameter.cif_resos_max = max_num;
    return parameter_save();
}

int parameter_get_back_cam_resolutions_max(void)
{
    return parameter.back_resos_max;
}

int parameter_save_back_cam_resolutions_max(int max_num)
{
    parameter.back_resos_max = max_num;
    return parameter_save();
}

int parameter_get_front_cam_resolutions_max(void)
{
    return parameter.front_resos_max;
}

int parameter_save_front_cam_resolutions_max(int max_num)
{
    parameter.front_resos_max = max_num;
    return parameter_save();
}

const struct photo_param *parameter_get_photo_resolutions(void)
{
    return parameter.photo_resos_ptr;
}

int parameter_save_photo_resolutions(struct photo_param *photo_resos_ptr)
{
    if (photo_resos_ptr != NULL) {
        parameter.photo_resos_ptr = photo_resos_ptr;
        return parameter_save();
    } else {
        return 0;
    }
}

int parameter_get_photo_resolutions_max(void)
{
    return parameter.photo_resos_max;
}

int parameter_save_photo_resolutions_max(int max_num)
{
    parameter.photo_resos_max = max_num;
    return parameter_save();
}

