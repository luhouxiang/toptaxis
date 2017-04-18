/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


/*                               -*- Mode: C -*- 
 * Filename:  bitsop.h
 * 
 * Keywords:  位操作函数，功能类似于FD_SET FD_ISSET FD_CLEAR，但在任意平台都是按照字节来置位
 * 
 *       This is unpublished proprietary
 *       source code of Tencent Ltd.
 *       The copyright notice above does not
 *       evidence any actual or intended
 *       publication of such source code.
 *
 *       NOTICE: UNAUTHORIZED DISTRIBUTION,ADAPTATION OR 
 *       USE MAY BE SUBJECT TO CIVIL AND CRIMINAL PENALTIES.
 *
 *  $Id: bitsop.h 7001 2010-09-13 02:00:40Z  $
 */

/* Change log:
 *
 *   select.h中提供的FD_*宏在32位机器上是按照byte来置位的（汇编实现），但在64位
 *   机器上是按照8 bytes来置位的，所以当碰到mmap文件末尾时，有可能segment fault。
 * 
 */

/* Code: */

#ifndef __BITSOP_H__
#define __BITSOP_H__
#include <stdint.h>

#include <linux/types.h>
#include <sys/cdefs.h>
__BEGIN_DECLS
/*
   bits操作函数
   */

#define CHAR_BITS   8

extern const unsigned char __bitcount[];
extern int CountBits(const char *buf, int sz);

void store_bits_to(char *dest, int offset, uint64_t value, int bits);
uint64_t get_bits_from(const char *addr, int offset, int bits);

//bits op interface
#define SET_B(bit, addr) 	__set_b(bit, addr)
#define CLR_B(bit, addr) 	__clr_b(bit, addr)
#define ISSET_B(bit, addr) 	__isset_b(bit, addr)
#define COPY_B(dest_bit, dest_addr, src_bit, src_addr, count) __bit_copy(dest_bit, dest_addr, src_bit, src_addr, count)
#define COUNT_B(buf, size) CountBits(buf, size)


static inline void __set_b(__u32 bit, const void *addr)
{
	__u8 *begin = (__u8 *)addr + ( bit / CHAR_BITS);
	__u8 shift = bit % CHAR_BITS;

	*begin |= ((__u8)0x1 << shift);

	return;
}

static inline int __isset_b(__u32 bit, const void *addr)
{
	__u8 *begin = (__u8 *)addr + ( bit / CHAR_BITS);
	__u8 shift = bit % CHAR_BITS;

	return (*begin & ((__u8)0x1 << shift)) > 0 ? 1:0;
}

static inline void __clr_b(__u32 bit, const void *addr)
{
	__u8 *begin = (__u8 *)addr + ( bit / CHAR_BITS);
	__u8 shift = bit % CHAR_BITS;

	*begin &= ~((__u8)0x1 << shift);
}

static inline __u8 __readbyte(const volatile void  *addr)
{
	return *(volatile __u8 *)addr;
}
static inline void __writebyte(__u8 val, volatile void  *addr)
{
	*(volatile __u8 *)addr = val;
}

static inline void __bit_copy(__u32 dest_bit, void *dest,
		__u32 src_bit,  void *src,
		__u32 count)
{
	__u32 i;
	for(i=0; i<count; i++)
	{
		if(__isset_b(src_bit, src))
			__set_b(dest_bit, dest);
		else
			__clr_b(dest_bit, dest);

		dest_bit++;
		src_bit++;
	}

	return;
}

__END_DECLS
#endif

/* bitsop.h ends here */
