#ifndef __USER_H__
#define __USER_H__

#ifdef __cplusplus
extern "C" {
#endif

#define CMD_USER_RECORD_RATE_CHANGE         0
#define CMD_USER_MDPROCESSOR                        1
#define CMD_USER_MUXER                                  2

int USER_RegEventCallback(void (*call)(int cmd, void *msg0, void *msg1));
void stop_motion_detection();
int set_motion_detection_trigger_value(bool result);
void start_motion_detection();

#ifdef __cplusplus
}
#endif
#endif
