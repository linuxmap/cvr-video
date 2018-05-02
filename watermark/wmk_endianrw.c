/**
 * Copyright (C) 2016 Fuzhou Rockchip Electronics Co., Ltd
 * author: Tianfeng Chen <ctf@rock-chips.com>
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

#include "wmk_common.h"
#include "wmk_endianrw.h"

uint16_t read_le16(struct mg_rw_ops *src)
{
	uint16_t value;

	rw_read(src, &value, (sizeof value), 1);
	return (ARCH_SWAP_LE16(value));
}

uint16_t read_be16(struct mg_rw_ops *src)
{
	uint16_t value;

	rw_read(src, &value, (sizeof value), 1);
	return (ARCH_SWAP_BE16(value));
}

uint32_t read_le32(struct mg_rw_ops *src)
{
	uint32_t value;

	rw_read(src, &value, (sizeof value), 1);
	return (ARCH_SWAP_LE32(value));
}

uint32_t read_be32(struct mg_rw_ops *src)
{
	uint32_t value;

	rw_read(src, &value, (sizeof value), 1);
	return (ARCH_SWAP_BE32(value));
}

unsigned long long read_le64(struct mg_rw_ops *src)
{
	unsigned long long value;

	rw_read(src, &value, (sizeof value), 1);
	return (ARCH_SWAP_LE64(value));
}

unsigned long long read_be64(struct mg_rw_ops *src)
{
	unsigned long long value;

	rw_read(src, &value, (sizeof value), 1);
	return (ARCH_SWAP_BE64(value));
}

int write_le16(struct mg_rw_ops *dst, uint16_t value)
{
	value = ARCH_SWAP_LE16(value);
	return (rw_write(dst, &value, (sizeof value), 1));
}

int write_be16(struct mg_rw_ops *dst, uint16_t value)
{
	value = ARCH_SWAP_BE16(value);
	return (rw_write(dst, &value, (sizeof value), 1));
}

int write_le32(struct mg_rw_ops *dst, uint32_t value)
{
	value = ARCH_SWAP_LE32(value);
	return (rw_write(dst, &value, (sizeof value), 1));
}

int write_be32(struct mg_rw_ops *dst, uint32_t value)
{
	value = ARCH_SWAP_BE32(value);
	return (rw_write(dst, &value, (sizeof value), 1));
}

int write_le64(struct mg_rw_ops *dst, unsigned long long value)
{
	value = ARCH_SWAP_LE64(value);
	return (rw_write(dst, &value, (sizeof value), 1));
}

int write_be64(struct mg_rw_ops *dst, unsigned long long value)
{
	value = ARCH_SWAP_BE64(value);
	return (rw_write(dst, &value, (sizeof value), 1));
}

uint16_t read_le16_fp(FILE *src)
{
	uint16_t value;

	fread(&value, (sizeof value), 1, src);
	return (ARCH_SWAP_LE16(value));
}

uint32_t read_le32_fp(FILE *src)
{
	uint32_t value;

	fread(&value, (sizeof value), 1, src);
	return (ARCH_SWAP_LE32(value));
}

int write_le16_fp(FILE *dst, uint16_t value)
{
	value = ARCH_SWAP_LE16(value);
	return (fwrite(&value, (sizeof value), 1, dst));
}

int write_le32_fp(FILE *dst, uint32_t value)
{
	value = ARCH_SWAP_LE32(value);
	return (fwrite(&value, (sizeof value), 1, dst));
}

static int stdio_seek(struct mg_rw_ops *context, int offset, int whence)
{
	if (fseek(context->hidden.stdio.fp, offset, whence) == 0) {
		return (ftell(context->hidden.stdio.fp));
	} else {
		printf("watermark stdio_seek error\n");
		return (-1);
	}
}

static int stdio_read(struct mg_rw_ops *context, void *ptr, int size,
		      int maxnum)
{
	size_t nread;

	nread = fread(ptr, size, maxnum, context->hidden.stdio.fp);
	if (nread == 0 && ferror(context->hidden.stdio.fp)) {
		printf("watermark stdio_read error\n");
	}
	return (nread);
}
static int stdio_write(struct mg_rw_ops *context, const void *ptr, int size,
		       int num)
{
	size_t nwrote;

	nwrote = fwrite(ptr, size, num, context->hidden.stdio.fp);
	if (nwrote == 0 && ferror(context->hidden.stdio.fp)) {
		printf("watermark stdio_write error\n");
	}
	return (nwrote);
}

#ifdef _MGUSE_OWN_STDIO
static int stdio_ungetc(struct mg_rw_ops *context, unsigned char c)
{
	return ungetc(c, context->hidden.stdio.fp);
}
#endif

static int stdio_close(struct mg_rw_ops *context)
{
	if (context) {
		if (context->hidden.stdio.autoclose) {
			/* WARNING:  Check the return value here! */
			fclose(context->hidden.stdio.fp);
		}
		free(context);
	}
	return (0);
}
static int stdio_eof(struct mg_rw_ops *context)
{
	return feof(context->hidden.stdio.fp);
}

#ifdef macintosh
static char *wmk_unix_to_mac(const char *file)
{
	int flen = strlen(file);
	char *path = calloc(1, flen + 2);
	const char *src = file;
	char *dst = path;
	if (*src == '/') {
		/* really depends on filesystem layout, hope for the best */
		src++;
	} else {
		/* Check if this is a MacOS path to begin with */
		if (*src != ':') {
			/* relative paths begin with ':' */
			*dst++ = ':';
		}
	}
	while (src < file + flen) {
		const char *end = strchr(src, '/');
		int len;
		if (!end) {
			/* last component */
			end = file + flen;
		}
		len = end - src;
		if (len == 0 || (len == 1 && src[0] == '.')) {
			/* remove repeated slashes and . */
		} else {
			if (len == 2 && src[0] == '.' && src[1] == '.') {
				/* replace .. with the empty string */
			} else {
				memcpy(dst, src, len);
				dst += len;
			}
			if (end < file + flen)
				*dst++ = ':';
		}
		src = end + 1;
	}
	*dst++ = '\0';
	return path;
}
#endif /* macintosh */

struct mg_rw_ops *rw_from_file(const char *file, const char *mode)
{
	FILE *fp;
	struct mg_rw_ops *rwops;

	rwops = NULL;

#ifdef macintosh
	{
		char *mpath = wmk_unix_to_mac(file);
		fp = fopen(mpath, mode);
		free(mpath);
	}
#else
	fp = fopen(file, (char *)mode);
#endif
	if (fp == NULL) {
		printf("MISC>RWOps: Couldn't open %s\n", file);
	} else {
		rwops = rw_from_fp(fp, 1);
	}

	return (rwops);
}

struct mg_rw_ops *rw_from_fp(FILE *fp, int autoclose)
{
	struct mg_rw_ops *rwops;

	rwops = alloc_rw();
	if (rwops != NULL) {
		rwops->seek = stdio_seek;
		rwops->read = stdio_read;
		rwops->write = stdio_write;
#ifdef _MGUSE_OWN_STDIO
		rwops->ungetc = stdio_ungetc;
#endif
		rwops->close = stdio_close;
		rwops->eof   = stdio_eof;
		rwops->hidden.stdio.fp = fp;
		rwops->hidden.stdio.autoclose = autoclose;

		rwops->type = WMK_RWAREA_TYPE_STDIO;
	}
	return (rwops);
}

struct mg_rw_ops *alloc_rw(void)
{
	struct mg_rw_ops *area;

	area = (struct mg_rw_ops *)calloc(1, sizeof * area);
	if (area == NULL) {
		printf("MISC>RWOps: Out of memory\n");
	} else
		area->type = WMK_RWAREA_TYPE_UNKNOWN;

	return (area);
}

void free_rw(struct mg_rw_ops *area)
{
	free(area);
}

int rw_getc(struct mg_rw_ops *area)
{
	unsigned char c;

	if (rw_read(area, &c, 1, 1) == 0)
		return EOF;

	return c;
}
