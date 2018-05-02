/**
 * example for how to get the h264 data
 */

#include <sys/time.h>
extern "C" {
#include "../parameter.h"
}
#include "../av_wrapper/encoder_muxing/muxing/camera_muxer.h"
#include "../av_wrapper/motion_detection/md_processor.h"
#include "../av_wrapper/video_common.h"
#include "../video.h"
#include "user.h"

static void (*user_event_call)(int cmd, void* msg0, void* msg1);

class UserMDProcessor : public VPUMDProcessor
{
private:
    struct timeval tval;
    volatile int force_result;

public:
    UserMDProcessor() : VPUMDProcessor() {
        memset(&tval, 0, sizeof(tval));
        force_result = -1;
    }
    int get_force_result() {
        return __atomic_exchange_n(&force_result, -1, __ATOMIC_SEQ_CST);
    }
    void set_force_result(int val) {
        __atomic_store_n(&force_result, val, __ATOMIC_SEQ_CST);
    }
    int calc_time_val(MDResult& result, bool& set_val, bool force_timeout) {
        int low = get_low_frame_rate();
#if DEBUG_MD
        printf(
            "md result: %d, low frame rate: %d, force_timeout: %d,"
            "tval.tv_sec: %ld, tval.tv_usec: %ld\n",
            result.change, low, force_timeout, tval.tv_sec, tval.tv_usec);
#endif
        if (result.change) {
            memset(&tval, 0, sizeof(tval));
            if (low) {
                set_val = false;
                return 1;
            }
        } else if (!result.change && low <= 0) {
            if (force_timeout) {
                memset(&tval, 0, sizeof(tval));
                set_val = true;
                return 1;
            }
            struct timeval now;
            gettimeofday(&now, NULL);
            if (tval.tv_sec == 0 && tval.tv_usec == 0) {
                tval = now;
                return 0;
            } else {
                if ((now.tv_sec - tval.tv_sec) * 1000000LL + now.tv_usec -
                    tval.tv_usec >=
                    10 * 1000000LL) {
                    memset(&tval, 0, sizeof(tval));
                    set_val = true;
                    return 1;
                }
            }
        }
        return 0;
    }
    int push_mv_data(void* mv_data, void* raw_yuv, size_t yuv_size) {
        // example code
        return VPUMDProcessor::push_mv_data(mv_data, raw_yuv, yuv_size);
    }
    uint32_t simple_process(void* mv_data) {
        // example code
        return VPUMDProcessor::simple_process(mv_data);
    }
};

static UserMDProcessor* md_process = NULL;
static MDAttributes md_attr = {0};

static void MD_Result_doing(MDResult result, void* arg)
{
    UserMDProcessor* p = (UserMDProcessor*)arg;
    bool low_frame_rate = false;
    int force_change = p->get_force_result();
    bool force_timeout = false;
    if (force_change >= 0) {
        result.change = force_change;
        force_timeout = true;
    }
#if DEBUG_MD
    printf("md result: %d\n", result.change);
#endif
    if (p->calc_time_val(result, low_frame_rate, force_timeout)) {
        assert(sizeof(void*) >= sizeof(UserMDProcessor*));
        assert(sizeof(void*) >= sizeof(bool));
        if (user_event_call)
            (*user_event_call)(CMD_USER_RECORD_RATE_CHANGE, (void*)p,
                               (void*)low_frame_rate);
    }
}

extern "C" void start_motion_detection()
{
    if (!md_process) {
        md_process = new UserMDProcessor();
        if (md_process) {
            md_process->register_result_cb(MD_Result_doing, (void*)md_process);
            md_attr.horizontal_area_num = 0;  // unused
            md_attr.vertical_area_num = 0;    // unused
            md_attr.time_interval = 500;      // milliseconds
            md_attr.weight = 5;
            md_attr.buffered_frames = 1;
            if (user_event_call)
                (*user_event_call)(CMD_USER_MDPROCESSOR, (void*)md_process,
                                   (void*)&md_attr);
        }
        printf("start_motion_detection md_process: %p\n", md_process);
    }
}

extern "C" void stop_motion_detection()
{
    // example code
    // assert(parameter_get_video_de() == 0);
    printf("stop_motion_detection md_process: %p\n", md_process);
    if (md_process) {
        video_record_detach_user_mdprocessor((void*)md_process);
        delete md_process;
        md_process = 0;
    }
}

extern "C" int set_motion_detection_trigger_value(bool result)
{
    if (md_process) {
        md_process->set_force_result(result ? 1 : 0);
        return 0;
    }
    return -1;
}

extern "C" int USER_RegEventCallback(void (*call)(int cmd,
                                                  void* msg0,
                                                  void* msg1))
{
    user_event_call = call;
    return 0;
}
