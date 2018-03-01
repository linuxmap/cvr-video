#ifndef RS_COMMON_H_
#define RS_COMMON_H_

typedef void * RSHandle;


typedef enum rs_pixel_format_t{
	PIX_FORMAT_GRAY,
	PIX_FORMAT_BGR888,
	PIX_FORMAT_NV21,
	PIX_FORMAT_BGRA8888,
	PIX_FORMAT_NV12,
	PIX_FORMAT_YUV_I420
}rs_pixel_format;

typedef struct rs_rect {
		int left;
		int top;
		int width;
		int height;
}rs_rect;

typedef struct rs_point {
	float x;
	float y;
}rs_point;

typedef enum {
	RS_IMG_CLOCKWISE_ROTATE_0,
	RS_IMG_CLOCKWISE_ROTATE_90,
	RS_IMG_CLOCKWISE_ROTATE_180,
	RS_IMG_CLOCKWISE_ROTATE_270
}rs_image_rotation;

typedef struct rs_image_tag{
int nWidth;
int nHeight;
int nPitch;
rs_pixel_format nFormat;
unsigned char *pData;
}rs_image, *LP_rs_image;

#define ERR_SDK_TRIAL_EXPIRED (-101)
#define ERR_SDK_INVALID_DEVICE_KEY (-102)
#define ERR_SDK_INVALID_APPID (-103)

#define ERR_SDK_LM_INVALID_FILE   (-1001)
#define ERR_SDK_LM_VERIFICATION   (-1002)
#define ERR_SDK_LM_NET_RESOLVE_HOST  (-1106)
#define ERR_SDK_LM_NET_TIMEDOUT   (-1128)

#endif