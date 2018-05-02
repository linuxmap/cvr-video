#ifndef __VIDEO_ION_ALLOC_H__
#define __VIDEO_ION_ALLOC_H__

#include "common.h"

int video_ion_alloc(struct video_ion* video_ion, int width, int height);
int video_ion_alloc_rational(struct video_ion* video_ion,
                             int width,
                             int height,
                             int num,
                             int den);
int video_ion_free(struct video_ion* video_ion);
void video_ion_buffer_black(struct video_ion* video_ion, int w, int h);

#endif
