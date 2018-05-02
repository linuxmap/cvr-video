#ifndef MV_DISPATCHER_H
#define MV_DISPATCHER_H

#include "../dispatcher.h"

class VPUMDProcessor;

class MVDispatcher : public Dispatcher<VPUMDProcessor, void>
{
public:
    void Dispatch(void* mv_data, void* raw_yuv, size_t yuv_size);
};

#endif // MV_DISPATCHER_H
