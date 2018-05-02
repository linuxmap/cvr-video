#include "video_ion_alloc.h"

#include <assert.h>
#include <execinfo.h>
#include <fcntl.h>
#include <malloc.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <ion/ion.h>

static void fatal_dump(void)
{
    void*array[30];
    size_t size;
    char**strings;
    size_t i;

    printf ("<<<<<<--- ION FATAL DUMP --->>>>>>\n");

    size = backtrace(array, 30);
    strings = (char**)backtrace_symbols(array, size);
    for (i = 0; i < size; i++)
        printf ("%d : %s \n", i, strings[i]);
    free (strings);
}

static int video_ion_alloc_buf(struct video_ion* video_ion)
{
    int ret = ion_alloc(video_ion->client, video_ion->size, 0,
                        ION_HEAP_TYPE_DMA_MASK, 0, &video_ion->handle);
    if (ret < 0) {
        video_ion_free(video_ion);
        printf("ion_alloc failed!\n");
        return -1;
    }
    ret = ion_share(video_ion->client, video_ion->handle, &video_ion->fd);
    if (ret < 0) {
        video_ion_free(video_ion);
        printf("ion_share failed!\n");
        return -1;
    }
    ion_get_phys(video_ion->client, video_ion->handle, &video_ion->phys);
    video_ion->buffer = mmap(NULL, video_ion->size, PROT_READ | PROT_WRITE,
                             MAP_SHARED | MAP_LOCKED, video_ion->fd, 0);
    if (!video_ion->buffer) {
        video_ion_free(video_ion);
        printf("%s mmap failed!\n", __func__);
        return -1;
    }
    printf("--- ion alloc, get fd: %d\n", video_ion->fd);
    return 0;
}

int video_ion_alloc_rational(struct video_ion* video_ion,
                             int width,
                             int height,
                             int num,
                             int den)
{
    if (video_ion->client >= 0) {
        printf("warning: video_ion client has been already opened\n");
        fatal_dump();
        return -1;
    }
    video_ion->client = ion_open();
    if (video_ion->client < 0) {
        printf("%s:open /dev/ion failed!\n", __func__);
        return -1;
    }

    video_ion->width = width;
    video_ion->height = height;
    video_ion->size = ((width + 15) & ~15) * ((height + 15) & ~15) * num / den;
    return video_ion_alloc_buf(video_ion);
}

int video_ion_alloc(struct video_ion* video_ion, int width, int height)
{
    return video_ion_alloc_rational(video_ion, width, height, 3, 2);
}

int video_ion_free(struct video_ion* video_ion)
{
    int ret = 0;

    if (video_ion->buffer) {
        munmap(video_ion->buffer, video_ion->size);
        video_ion->buffer = NULL;
    }

    if (video_ion->fd >= 0) {
        printf("--- ion free, release fd: %d\n", video_ion->fd);
        close(video_ion->fd);
        video_ion->fd = -1;
    }

    if (video_ion->client >= 0) {
        if (video_ion->handle) {
            ret = ion_free(video_ion->client, video_ion->handle);
            if (ret)
                printf("ion_free failed!\n");
            video_ion->handle = 0;
        }

        ion_close(video_ion->client);
        video_ion->client = -1;
    }

    memset(video_ion, 0, sizeof(struct video_ion));
    video_ion->client = -1;
    video_ion->fd = -1;

    return ret;
}

void video_ion_buffer_black(struct video_ion* video_ion, int w, int h)
{
    memset(video_ion->buffer, 16, w * h);
    memset((char *)video_ion->buffer + w * h, 128, w * h / 2);
}
