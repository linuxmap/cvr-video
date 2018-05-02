/**
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "mjpg_process.hpp"

int mjpg_decode(struct Video* video,      void* srcbuf, unsigned int size, void* dstbuf, int dstfd)
{
    char* in_data = (char*)srcbuf;
    unsigned int in_size = size;
    int i = 0;
    int ret = 0;

    /* MJPEG buffer data should end of 0xFF 0xD9, the size should be 2 at least */
    if (in_size < 2) {
        printf("%s in_size error!\n", __func__);
        ret = -1;
        goto exit_mjpg_decode;
    }

    video->jpeg_dec.decoding = true;
    if (!video->pthread_run)
        goto exit_mjpg_decode;

    /* some USB camera input buffer data may not end of 0xFF 0xD9, it may align 16 Bytes */
    while (*(in_data + in_size - 2) != 0xFF || *(in_data + in_size - 1) != 0xD9) {
        in_size--;
        if (i++ > 16 || in_size < 2) {
            printf("err mjpg data.\n");
            ret = -1;
            goto exit_mjpg_decode;
        }
    }

    if (vpu_decode_jpeg_doing(video->jpeg_dec.decode, in_data, in_size,
                              dstfd, (char*)dstbuf)) {
        printf("vpu_decode_jpeg_doing err\n");
        ret = -1;
        goto exit_mjpg_decode;
    }

exit_mjpg_decode:
    video->jpeg_dec.decoding = false;
    return ret;
}

static int video_save_jpeg(struct Video* video,
                           char* filename,
                           void* srcbuf,
                           unsigned int size)
{
    return fs_picture_mjpg_write(filename, srcbuf, size);
}

int video_record_save_jpeg(struct Video* video,            void* srcbuf, unsigned int size)
{
    char filename[128];

    video_record_getfilename(filename, sizeof(filename),
                             fs_storage_folder_get_bytype(video->type, PICFILE_TYPE),
                             video->deviceid, "jpg", "");
    video_save_jpeg(video, filename, srcbuf, size);

    video_record_takephoto_end(video);

    return 0;
}

