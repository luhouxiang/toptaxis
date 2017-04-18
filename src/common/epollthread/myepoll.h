/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


#ifndef _SYS_EPOLL_H_
#define _SYS_EPOLL_H_

#if __x86_64__
#define _SYS_EPOLL_
#endif

#ifdef _SYS_EPOLL_
#include <sys/epoll.h>
#else

#include <stdint.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/poll.h>
#include <asm/unistd.h>
#include <unistd.h>

/* Valid opcodes to issue to sys_epoll_ctl() */
#ifndef EPOLL_CTL_ADD
#define EPOLL_CTL_ADD 1
#endif
#ifndef EPOLL_CTL_DEL
#define EPOLL_CTL_DEL 2
#endif
#ifndef EPOLL_CTL_MOD
#define EPOLL_CTL_MOD 3
#endif

/* These are specified by iBCS2 */
#if defined(POLLIN) && !defined(EPOLLIN)
#define EPOLLIN          POLLIN
#endif
#if defined(POLLPRI) && !defined(EPOLLPRI)
#define EPOLLPRI          POLLPRI
#endif
#if defined(POLLOUT) && !defined(EPOLLOUT)
#define EPOLLOUT          POLLOUT
#endif
#if defined(POLLERR) && !defined(EPOLLERR)
#define EPOLLERR          POLLERR
#endif
#if defined(POLLHUP) && !defined(EPOLLHUP)
#define EPOLLHUP          POLLHUP
#endif
#if defined(POLLNVAL) && !defined(EPOLLNVAL)
#define EPOLLNVAL          POLLNVAL
#endif
#if defined(POLLRDNORM) && !defined(EPOLLRDNORM)
#define EPOLLRDNORM          POLLRDNORM
#endif
#if defined(POLLRDBAND) && !defined(EPOLLRDBAND)
#define EPOLLRDBAND          POLLRDBAND
#endif
#if defined(POLLWRNORM) && !defined(EPOLLWRNORM)
#define EPOLLWRNORM          POLLWRNORM
#endif
#if defined(POLLWRBAND) && !defined(EPOLLWRBAND)
#define EPOLLWRBAND          POLLWRBAND
#endif
#if defined(POLLMSG) && !defined(EPOLLMSG)
#define EPOLLMSG          POLLMSG
#endif

/* Set the One Shot behaviour for the target file descriptor */
#ifndef EPOLLONESHOT
#define EPOLLONESHOT (1 << 30)
#endif

/* Set the Edge Triggered behaviour for the target file descriptor */
#ifndef EPOLLET
#define EPOLLET (1 << 31)
#endif

/* 
 * On x86-64 make the 64bit structure have the same alignment as the
 * 32bit structure. This makes 32bit emulation easier.
 */
#ifdef __x86_64__
#define EPOLL_PACKED __attribute__((packed))
#else
#define EPOLL_PACKED
#endif

struct epoll_event {
	uint32_t events;
	union {
		void *ptr;
		int fd;
		uint32_t u32;
		uint64_t u64;
	} data;
} EPOLL_PACKED;

#ifndef __NR_epoll_create
#ifdef __x86_64__
#define __NR_epoll_create       213
#else
#define __NR_epoll_create       254
#endif
#endif
#ifndef __NR_epoll_ctl
#define __NR_epoll_ctl		(__NR_epoll_create+1)
#endif
#ifndef __NR_epoll_wait
#define __NR_epoll_wait		(__NR_epoll_create+2)
#endif

#ifdef __EPOLL_SYSCALLS__
static inline _syscall1(int, epoll_create, int, maxfds);
static inline _syscall4(int, epoll_ctl, int, epfd, int, op, int, fd, struct epoll_event *, event);
static inline _syscall4(int, epoll_wait, int, epfd, struct epoll_event *, events, int, maxevents, int, timeout);
#else
static inline int epoll_create( int maxfds) {
return syscall(__NR_epoll_create, maxfds);
}

static inline int epoll_ctl( int epfd, int op, int fd, struct epoll_event * event){
return syscall(__NR_epoll_ctl, epfd, op, fd, event);
}
static inline int epoll_wait( int epfd, struct epoll_event * events, int maxevents, int timeout){
return syscall(__NR_epoll_wait, epfd, events, maxevents, timeout);
}
#endif

#endif
#endif
