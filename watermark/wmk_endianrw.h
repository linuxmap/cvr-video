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

#ifndef _WATER_MARK_ENDIAN_RW_H
#define _WATER_MARK_ENDIAN_RW_H

#include <stdint.h>
#include "wmk_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WMK_RWAREA_TYPE_UNKNOWN 0
#define WMK_RWAREA_TYPE_STDIO   1
#define WMK_RWAREA_TYPE_MEM     2

struct mg_rw_ops {
	int (*seek)(struct mg_rw_ops *context, int offset, int whence);
	int (*read)(struct mg_rw_ops *context, void *ptr, int objsize, int num);
	int (*write)(struct mg_rw_ops *context, const void *ptr, int objsize,
		     int num);

#ifdef _MGUSE_OWN_STDIO
	int (*ungetc)(struct mg_rw_ops *context, unsigned char c);
#endif

	int (*close)(struct mg_rw_ops *context);
	int (*eof)(struct mg_rw_ops *context);

	/**
	 * Indicates the type of data source.
	 *  can be one of the following values:
	 *  - RWAREA_TYPE_UNKNOWN\n
	 *      A unknown (uninitialized) data source type.
	 *  - RWAREA_TYPE_STDIO\n
	 *      Stdio stream data source.
	 *  - RWAREA_TYPE_MEM\n
	 *      Memory data source.
	 */
	uint32_t type;

	union {
		struct {
			int autoclose;
			FILE *fp;
		} stdio;
		struct {
			uint8_t *base;
			uint8_t *here;
			uint8_t *stop;
		} mem;
		struct {
			void *data1;
		} unknown;
	} hidden;
};

struct mg_rw_ops *rw_from_file(const char *file, const char *mode);
struct mg_rw_ops *rw_from_fp(FILE *fp, int autoclose);
struct mg_rw_ops *rw_from_mem(void *mem, int size);
void init_mem_rw(struct mg_rw_ops *area, void *mem, int size);
struct mg_rw_ops *alloc_rw(void);
void free_rw(struct mg_rw_ops *area);
int rw_getc(struct mg_rw_ops *area);

#define rw_seek(ctx, offset, whence)    (ctx)->seek(ctx, offset, whence)
#define rw_tell(ctx)                    (ctx)->seek(ctx, 0, SEEK_CUR)
#define rw_read(ctx, ptr, size, n)      (ctx)->read(ctx, ptr, size, n)
#define rw_write(ctx, ptr, size, n)     (ctx)->write(ctx, ptr, size, n)
#define rw_close(ctx)                   (ctx)->close(ctx)
#define rw_eof(ctx)                     (ctx)->eof(ctx)

/****************** Endian specific read/write interfaces *********************/
#ifdef linux
#include <endian.h>
#ifdef __arch__swab16
#define arch_swap16  __arch__swab16
#endif
#ifdef __arch__swab32
#define arch_swap32  __arch__swab32
#endif
#endif

#ifndef arch_swap16
static inline uint16_t arch_swap16(uint16_t D)
{
	return ((D << 8) | (D >> 8));
}
#endif

#ifndef arch_swap32
static inline uint32_t arch_swap32(uint32_t D)
{
	return ((D << 24) | ((D << 8) & 0x00FF0000)
		| ((D >> 8) & 0x0000FF00) | (D >> 24));
}
#endif

#ifdef WMK_HAS_64BIT_TYPE
#ifndef arch_swap64
static inline unsigned long long arch_swap64(unsigned long long val)
{
	uint32_t hi, lo;

	/* Separate into high and low 32-bit values and swap them */
	lo = (uint32_t)(val & 0xFFFFFFFF);
	val >>= 32;
	hi = (uint32_t)(val & 0xFFFFFFFF);
	val = arch_swap32(lo);
	val <<= 32;
	val |= arch_swap32(hi);
	return (val);
}
#endif
#else
#ifndef arch_swap64
#define arch_swap64(X)        (X)
#endif
#endif /* WMK_HAS_64BIT_TYPE */

/* Byteswap item from the specified endianness to the native endianness */
#if WMK_BYTEORDER == WMK_LIL_ENDIAN
/** Swaps a 16-bit little endian integer to the native endianness. */
#define ARCH_SWAP_LE16(X)        (X)
/** Swaps a 32-bit little endian integer to the native endianness. */
#define ARCH_SWAP_LE32(X)        (X)
/** Swaps a 64-bit little endian integer to the native endianness. */
#define ARCH_SWAP_LE64(X)        (X)
/** Swaps a 16-bit big endian integer to the native endianness. */
#define ARCH_SWAP_BE16(X)        arch_swap16(X)
/** Swaps a 32-bit big endian integer to the native endianness. */
#define ARCH_SWAP_BE32(X)        arch_swap32(X)
/** Swaps a 64-bit big endian integer to the native endianness. */
#define ARCH_SWAP_BE64(X)        arch_swap64(X)
#else
#define ARCH_SWAP_LE16(X)        arch_swap16(X)
#define ARCH_SWAP_LE32(X)        arch_swap32(X)
#define ARCH_SWAP_LE64(X)        arch_swap64(X)
#define ARCH_SWAP_BE16(X)        (X)
#define ARCH_SWAP_BE32(X)        (X)
#define ARCH_SWAP_BE64(X)        (X)
#endif

extern uint16_t read_le16(struct mg_rw_ops *src);
extern uint16_t read_be16(struct mg_rw_ops *src);
extern uint32_t read_le32(struct mg_rw_ops *src);
extern uint32_t read_be32(struct mg_rw_ops *src);
extern unsigned long long read_le64(struct mg_rw_ops *src);
extern unsigned long long read_be64(struct mg_rw_ops *src);
extern int write_le16(struct mg_rw_ops *dst, uint16_t value);
extern int write_be16(struct mg_rw_ops *dst, uint16_t value);
extern int write_le32(struct mg_rw_ops *dst, uint32_t value);
extern int write_be32(struct mg_rw_ops *dst, uint32_t value);
extern int write_le64(struct mg_rw_ops *dst, unsigned long long value);
extern int write_be64(struct mg_rw_ops *dst, unsigned long long value);
extern uint16_t read_le16_fp(FILE *src);
extern uint32_t read_le32_fp(FILE *src);
extern int write_le16_fp(FILE *dst, uint16_t value);
extern int write_le32_fp(FILE *dst, uint32_t value);

static inline uint16_t read_le16_mem(const uint8_t **data)
{
	uint16_t h1, h2;

	h1 = *(*data);
	(*data)++;
	h2 = *(*data);
	(*data)++;
	return ((h2 << 8) | h1);
}

static inline uint32_t read_le32_mem(const uint8_t **data)
{
	uint32_t q1, q2, q3, q4;

	q1 = *(*data);
	(*data)++;
	q2 = *(*data);
	(*data)++;
	q3 = *(*data);
	(*data)++;
	q4 = *(*data);
	(*data)++;
	return ((q4 << 24) | (q3 << 16) | (q2 << 8) | (q1));
}

static inline uint16_t read_be16_mem(const uint8_t **data)
{
	uint16_t h1, h2;

	h1 = *(*data);
	(*data)++;
	h2 = *(*data);
	(*data)++;
	return ((h1 << 8) | h2);
}

static inline uint32_t read_be32_mem(const uint8_t **data)
{
	uint32_t q1, q2, q3, q4;

	q1 = *(*data);
	(*data)++;
	q2 = *(*data);
	(*data)++;
	q3 = *(*data);
	(*data)++;
	q4 = *(*data);
	(*data)++;
	return ((q1 << 24) | (q2 << 16) | (q3 << 8) | (q4));
}

#ifdef __cplusplus
}
#endif

#endif

