#include "mv_dispatcher.h"
#include "md_processor.h"

void MVDispatcher::Dispatch(void* mv_data, void* raw_yuv, size_t yuv_size)
{
    std::lock_guard<std::mutex> lk(list_mutex);
    for (std::list<VPUMDProcessor*>::iterator i = handlers.begin(); i != handlers.end(); i++) {
        VPUMDProcessor* processor = *i;
        processor->push_mv_data(mv_data, raw_yuv, yuv_size);
    }
}
