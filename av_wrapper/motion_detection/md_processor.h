#ifndef __MD_PROCESSOR_H__
#define __MD_PROCESSOR_H__

#include <queue>

#include <mpp/mpp_frame.h>
#include <mpp/rk_mpi_cmd.h>

#include "../encoder_muxing/encoder/h264_encoder.h"
#include "../video_common.h"

/**
 * VPU hardware md processor
 * 1. only support 16*16 macro block, hardware limit
 * 2. donot support selfdefined image as reference image
 * 3. now the class is a very simple algorithm implement, TODO
 */

typedef enum {
    STATIC_REF_MODE,
    DYNAMIC_REF_MODE, //default
    USER_REF_MODE
} MD_MODE;

typedef enum {
    FRAME_DIFF_ALGORITHM, // frame difference ALGORITHM, default
    BG_REF_ALGORITHM // background image reference mode
} MD_ALGORITHM;

typedef struct __MDATTRIBUTES {
    int horizontal_area_num; //4
    int vertical_area_num; //4
    int time_interval; // milliseconds
    int buffered_frames;
    int weight; //10~100%
} MDAttributes;

typedef struct {
    bool change;
} MDResult;

typedef void (*MD_Result_CallBack)(MDResult result, void* arg);

class VPUMDProcessor
{
private:
    BaseVideoEncoder* video_encoder;
    int blk_w;
    int blk_h;
    int blk_hor;
    int blk_ver;
    size_t mv_len_per_frame;
    int frame_rate;
    int frame_inverval;
    int times;
    MDAttributes attr;
    std::queue<void*> buf_queue;
    void* buffer;
    uint32_t* result_collecs;
    MDResult result;
    MD_Result_CallBack result_cb;
    void* cb_arg;
    volatile int low_frame_rate; //-1, init state; 0, normal rate; 1, low rate

public:
    VPUMDProcessor();
    virtual ~VPUMDProcessor();
    void init(int image_width, int image_height, int rate);
    int set_attributes(MDAttributes* in_attr);
    void release_attributes();
    void register_result_cb(MD_Result_CallBack cb, void* arg)
    {
        result_cb = cb;
        cb_arg = arg;
    }
    void set_video_encoder(BaseVideoEncoder* encoder)
    {
        video_encoder = encoder;
    }
    BaseVideoEncoder* get_video_encoder()
    {
        return video_encoder;
    }
    inline int get_low_frame_rate()
    {
        return __atomic_load_n(&low_frame_rate, __ATOMIC_SEQ_CST);
    }
    void set_low_frame_rate(int val)
    {
        __atomic_store_n(&low_frame_rate, val, __ATOMIC_SEQ_CST);
    }
    virtual int push_mv_data(void* mv_data, void* raw_yuv, size_t yuv_size);
    virtual uint32_t simple_process(void* mv_data);
};

#endif
