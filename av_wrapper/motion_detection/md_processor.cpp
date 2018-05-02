#include "md_processor.h"

VPUMDProcessor::VPUMDProcessor()
{
    video_encoder = NULL;
    blk_w = -1;
    blk_h = -1;
    blk_hor = -1;
    blk_ver = -1;
    mv_len_per_frame = 0;
    frame_rate = 0;
    frame_inverval = 0;
    times = 0;
    memset(&attr, 0, sizeof(attr));
    buffer = NULL;
    result_collecs = NULL;
    memset(&result, 0, sizeof(result));
    result_cb = NULL;
    cb_arg = NULL;
    low_frame_rate = -1;
}

VPUMDProcessor::~VPUMDProcessor()
{
    if (result_collecs)
        free(result_collecs);
    if (buffer)
        free(buffer);
}

void VPUMDProcessor::init(int image_width, int image_height, int rate)
{
    blk_w = (image_width + 15) / 16;
    blk_h = (image_height + 15) / 16;
    /* NOTE: mv info block in 16 block align need extend buffer */
    blk_hor = ((image_width + 255) & ~255) / 16;
    blk_ver = (image_height + 15) / 16;
    assert(sizeof(MppEncMDBlkInfo) == sizeof(RK_U32));
    mv_len_per_frame = blk_hor * blk_ver * sizeof(MppEncMDBlkInfo);
    frame_rate = rate;
    frame_inverval = rate / 2; //default 500ms
}

int VPUMDProcessor::set_attributes(MDAttributes* in_attr)
{
    uint8_t* buf = NULL;
    attr = *in_attr;
    frame_inverval = frame_rate * in_attr->time_interval / 1000;
    if (frame_inverval < 1)
        frame_inverval = 1;
    result_collecs = (uint32_t*)malloc(frame_inverval * sizeof(uint32_t));
    if (!result_collecs)
        return -1;
    buf = (uint8_t*)malloc(mv_len_per_frame * in_attr->buffered_frames);
    if (buf) {
        for (int i = 0; i < in_attr->buffered_frames; i++) {
            buf_queue.push(buf + (i * mv_len_per_frame));
        }
        buffer = buf;
        return 0;
    } else {
        free(result_collecs);
        result_collecs = NULL;
    }
    return -1;
}

void VPUMDProcessor::release_attributes() {
  if (result_collecs) {
      free(result_collecs);
      result_collecs = NULL;
  }
  while(!buf_queue.empty())
    buf_queue.pop();
  if (buffer) {
      free(buffer);
      buffer = NULL;
  }
}

#define EXTRACOEFF 3
#if DEBUG_MD
#define SHOW_MD_RESULT 1
#else
#define SHOW_MD_RESULT 0
#endif

uint32_t VPUMDProcessor::simple_process(void* mv_data)
{
    //very simple example
    MppEncMDBlkInfo* data = (MppEncMDBlkInfo*)mv_data;
    unsigned int *val = (unsigned int *)mv_data;
    // int num = mv_len_per_frame / sizeof(MppEncMDBlkInfo);
    uint32_t change_pixels = 0;
#if SHOW_MD_RESULT
    size_t size = blk_h * (blk_w + 1) + 1;
    char *disp = (char *)malloc(size);
    char *tmp = disp;
#endif

    //printf("process mv info: [sad mvx mvy] %dx%d\n", blk_w, blk_h);
    for (int y = 0; y < blk_h; y++) {
        for (int x = 0; x < blk_w; x++) {
            RK_U32 sad = data[x].sad;
            // RK_S32 mvx = data[x].mvx;
            // RK_S32 mvy = data[x].mvy;

            RK_S32 change_found = (sad > 0xff*EXTRACOEFF);

            //printf("0x%08x - [%x %d %d]\n", val[x], sad, mvx, mvy);

            if (change_found) {
                //means change?
                change_pixels++;
            }
#if SHOW_MD_RESULT
            snprintf(tmp++, size--, "%c", change_found ? '+' : '-');
#endif
        }
#if SHOW_MD_RESULT
        snprintf(tmp++, size--, "\n");
#endif

        data += blk_hor;
        val += blk_hor;
    }

#if SHOW_MD_RESULT
    if (change_pixels)
        printf("%s\n", disp);

    free(disp);
#endif
    //printf("change_pixels: %d, num : %d\n", change_pixels, num);
    return change_pixels;
}

int VPUMDProcessor::push_mv_data(void* mv_data, void* raw_yuv, size_t yuv_size)
{
    //very simple example
    UNUSED(raw_yuv); //unused right now, TODO
    UNUSED(yuv_size);
    uint32_t ret = 0;
    //if (times == 0) {
#if 0
    void* buf = buf_queue.front();
    buf_queue.pop();
    memcpy(buf, mv_data, mv_len_per_frame);
    buf_queue.push(buf);
#else
    void* buf = mv_data;
#endif
    //do in other thread??
    ret = simple_process(buf);
    //}
    result_collecs[times] = ret;
    if (++times >= frame_inverval) {
        times = 0;
        //int num = mv_len_per_frame / sizeof(MppEncMDBlkInfo);
        int num = blk_w * blk_h;
        uint64_t change_pixels = result_collecs[0];
        for (int i = 1; i < frame_inverval; i++) {
            change_pixels += result_collecs[i];
        }
        change_pixels /= frame_inverval;
#if DEBUG_MD
          printf("change_pixels: %llu, num : %d\n", change_pixels, num);
#endif
        if (change_pixels * EXTRACOEFF * 100 > (uint64_t)attr.weight * num) {
            result.change = true;
        }
        if (result_cb) {
            result_cb(result, cb_arg);
        }
        result.change = false;
    }
    return 0;
}
