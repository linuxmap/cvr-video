#ifndef __CAMERA_UI_H__
#define __CAMERA_UI_H__
#include <stdbool.h>
#include "common.h"

#define PaintRect(func, handle, rect) \
            func(handle, ((RECT *)rect)->left, ((RECT *)rect)->top, \
                         ((RECT *)rect)->right, ((RECT *)rect)->bottom)

#define PaintRectBold(func, handle, rect) \
            func(handle, ((RECT *)rect)->left, ((RECT *)rect)->top, \
                         ((RECT *)rect)->right, ((RECT *)rect)->bottom); \
            func(handle, ((RECT *)rect)->left - 1, ((RECT *)rect)->top - 1, \
                         ((RECT *)rect)->right + 1, ((RECT *)rect)->bottom + 1); \
            func(handle, ((RECT *)rect)->left + 1, ((RECT *)rect)->top + 1, \
                         ((RECT *)rect)->right - 1, ((RECT *)rect)->bottom - 1)

#define PaintPoint(func, handle, point) \
            func(handle, ((POINT *)point)->x, ((POINT *)point)->y)

#define PaintLine(handle, start, end) \
            PaintPoint(MoveTo, handle, start); \
            PaintPoint(LineTo, handle, end)

void ui_deinit_init_camera(HWND hWnd, char i, char j, char k);
int sdcardformat_back(void *arg, int param);
void ui_set_white_balance(int i);
void ui_set_exposure_compensation(int i);
void cmd_IDM_frequency(char i);
void changemode(HWND hWnd, int mode);
void cmd_IDM_CAR(HWND hWnd, char i);
void startrec(HWND hWnd);
void stoprec(HWND hWnd);

void cmd_IDM_VIDEO_QUALITY(HWND hWnd, unsigned int qualiy_level);
void cmd_IDM_DEBUG_VIDEO_BIT_RATE(HWND hWnd, unsigned int val);

int getSD(void);

#ifdef __cplusplus
extern "C" {
#endif
void start_motion_detection();
void stop_motion_detection();
#ifdef __cplusplus
}
#endif

#endif
