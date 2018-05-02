/**
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd
 * author: hertz.wang hertz.wong@rock-chips.com
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

#include "screen_capture.h"

#include <autoconfig/main_app_autoconf.h>

#if MAIN_APP_ENABLE_SCREEN_CAPTURE

#include "video_ion_alloc.h"

#include <png.h>
#include <rk_fb/rk_fb.h>
#include <rk_rga/rga_copybit.h>
#include <rk_rga/rk_rga.h>

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static const char *sc_dir_path = "/mnt/sdcard/screen_capture";

static void write_to_png(uint8_t *buffer, int w, int h)
{
    if (access(sc_dir_path, F_OK)) {
        if (mkdir(sc_dir_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
            printf("create dir %s failed, errno: %d\n", sc_dir_path, errno);
            return;
        }
    }
    png_structp png_ptr;
    png_infop info_ptr;
    png_colorp palette;

    FILE *png_file = NULL;
    char png_path[128];
    int l = 0;
    time_t t = time(NULL);
    png_bytep row_pointers[h];
    l = snprintf(png_path, sizeof(png_path), "%s", sc_dir_path);
    strftime(png_path + l, sizeof(png_path) - l, "/%Y_%m_%d-%H_%M_%S.png",
             localtime(&t));
    png_file = fopen(png_path, "wb");
    if (!png_file) {
        printf("%s : create png file failed.\n", __FUNCTION__);
        goto out;
    }

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr)
        goto out;
    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
        goto out;
    if (setjmp(png_jmpbuf(png_ptr))) {
        /* If we get here, we had a problem writing the file */
        goto out;
    }
    png_init_io(png_ptr, png_file);
    png_set_IHDR(png_ptr, info_ptr, w, h, 8, PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
                 PNG_FILTER_TYPE_BASE);
    palette = (png_colorp)png_malloc(png_ptr,
                                     PNG_MAX_PALETTE_LENGTH * sizeof(png_color));
    png_set_PLTE(png_ptr, info_ptr, palette, PNG_MAX_PALETTE_LENGTH);
    png_write_info(png_ptr, info_ptr);
    for (l = 0; l < h; l++)
        row_pointers[l] = (png_bytep)buffer + l * w * 3;
    png_write_image(png_ptr, row_pointers);
    png_write_end(png_ptr, info_ptr);
    printf("writed screen capture to %s\n", png_path);

out:
    if (palette)
        png_free(png_ptr, palette);
    if (png_ptr)
        png_destroy_write_struct(&png_ptr, &info_ptr);
    if (png_file)
        fclose(png_file);
}

static void composite_rgb888(uint8_t *src, uint8_t *dst, int w, int h,
                             struct color_key *color_ignore)
{
    uint32_t rgb_ignore = (color_ignore->red << 16) | (color_ignore->green << 8) |
                          (color_ignore->blue);
    int i = 0;
    for (; i < w * h * 3; i += 3) {
        uint32_t pixel = (src[i] << 16) | (src[i + 1] << 8) | src[i + 2];
        if (pixel ^ rgb_ignore) {
            dst[i] = src[i];
            dst[i + 1] = src[i + 1];
            dst[i + 2] = src[i + 2];
        }
    }
}

static int rga_composite(int rga_fd, int src_fd, int src_fmt, int dst_fd,
                         int dst_fmt, int w, int h)
{
    struct rga_req req;
    memset(&req, 0x0, sizeof(struct rga_req));

    req.src.act_w = w;
    req.src.act_h = h;
    req.src.vir_w = w;
    req.src.vir_h = h;
    req.src.yrgb_addr = src_fd;
    req.src.format = src_fmt;

    req.dst.act_w = w;
    req.dst.act_h = h;
    req.dst.vir_w = w;
    req.dst.vir_h = h;
    req.dst.yrgb_addr = dst_fd;
    req.dst.format = dst_fmt;

    req.scale_mode = 2;
    req.render_mode = bitblt_mode;
    req.alpha_rop_mode = 1;
    req.alpha_rop_flag = 1 | (1 << 3) | (1 << 4);
    req.PD_mode = 3;
    req.mmu_info.mmu_en = 0;
    req.mmu_info.mmu_flag = ((2 & 0x3) << 4) | 1 | 1 << 31 | 1 << 8 | 1 << 10;

    if (ioctl(rga_fd, RGA_BLIT_SYNC, &req) != 0) {
        printf("%s:  rga operation error\n", __func__);
        return -1;
    }

    return 0;
}

int take_screen_capture()
{
    struct win *yuv_win = NULL;
    rk_fb_set_capture_disp(&yuv_win, 1);

    int rga_yuv_fmt = -1;
    if (!yuv_win) {
        printf("%s : g_fb have not inited?\n", __FUNCTION__);
        return -1;
    }

    if (yuv_win->format == HAL_PIXEL_FORMAT_YCrCb_NV12) {
        rga_yuv_fmt = RGA_FORMAT_YCBCR_420_SP;
    } else {
        rk_fb_set_capture_disp(NULL, 0);
        printf("%s : yuv_win->format <%d> not nv12.\n", __FUNCTION__,
               yuv_win->format);
        return -1;
    }

    struct win *ui_win = rk_fb_getuiwin();
    int rga_ui_fmt = -1;
    if (ui_win->format == HAL_PIXEL_FORMAT_RGB_565) {
        rga_ui_fmt = RGA_FORMAT_RGB_565;
    } else if (ui_win->format == HAL_PIXEL_FORMAT_BGRA_8888) {
        rga_ui_fmt = RGA_FORMAT_BGRA_8888;
    } else {
        rk_fb_set_capture_disp(NULL, 0);
        printf("%s : ui_win->format <%d> not rgb.\n", __FUNCTION__,
               ui_win->format);
        return -1;
    }

    int w = ui_win->width;
    int h = ui_win->height;
    struct video_ion ion_buf = {-1, 0, 0, NULL, -1, 0, 0, 0};
    uint8_t *dst_buf = NULL;
    int rga_fd = rk_rga_open();
    if (rga_fd < 0) {
        printf("%s : open rga failed.\n", __FUNCTION__);
        goto out;
    }

    // RGA_FORMAT_RGB_888
    if (video_ion_alloc_rational(&ion_buf, w, h, 3, 1) < 0) {
        printf("%s : alloc ion buffer failed.\n", __FUNCTION__);
        goto out;
    }

    if (rk_rga_ionfd_to_ionfd_scal(rga_fd, yuv_win->video_ion.fd, yuv_win->width,
                                   yuv_win->height, rga_yuv_fmt, ion_buf.fd, w, h,
                                   RGA_FORMAT_RGB_888, 0, 0, w, h, yuv_win->width,
                                   yuv_win->height) < 0)
        goto out;

    if (rga_ui_fmt == RGA_FORMAT_RGB_565) {
        dst_buf = (uint8_t *)malloc(w * h * 3);
        if (!dst_buf) {
            printf("%s : alloc virtual buffer failed.\n", __FUNCTION__);
            goto out;
        }
        memcpy(dst_buf, ion_buf.buffer, w * h * 3);
        if (rk_rga_ionfd_to_ionfd_scal(rga_fd, ui_win->video_ion.fd, w, h,
                                       rga_ui_fmt, ion_buf.fd, w, h,
                                       RGA_FORMAT_RGB_888, 0, 0, w, h, w, h) < 0)
            goto out;
        composite_rgb888((uint8_t *)ion_buf.buffer, dst_buf, w, h,
                         rk_fb_get_color_key());
        write_to_png(dst_buf, w, h);
    } else {
        if (rga_composite(rga_fd, ui_win->video_ion.fd, rga_ui_fmt, ion_buf.fd,
                          RGA_FORMAT_RGB_888, w, h) < 0)
            goto out;
        write_to_png((uint8_t *)ion_buf.buffer, w, h);
    }

out:
    rk_fb_set_capture_disp(NULL, 0);
    if (rga_fd >= 0)
        rk_rga_close(rga_fd);
    video_ion_free(&ion_buf);
    if (dst_buf)
        free(dst_buf);
    return 0;
}

#else

int take_screen_capture() { return 0; }

#endif
