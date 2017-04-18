/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


#ifndef LOG_H
#define LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

#if __GNUC__ == 2 && __GNUC_MINOR__ < 96
#define __builtin_expect(x, expected_value) (x)
#endif

#ifndef likely
#define likely(x)  __builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
#define unlikely(x)  __builtin_expect(!!(x), 0)
#endif

#define APP_EMERG 		0  /* system is unusable               */
#define APP_ALERT		1  /* action must be taken immediately */
#define APP_CRIT		2  /* critical conditions              */
#define APP_ERROR		3  /* error conditions                 */
#define APP_WARNING		4  /* warning conditions               */
#define APP_NOTICE		5  /* normal but significant condition */
#define APP_INFO		6  /* informational                    */
#define APP_DEBUG		7  /* debug-level messages             */
#define APP_TRACE		8  /* trace-level messages             */

#define DETAIL(level, fmt, args...) \
	write_log (level, "[%s][%d]%s: " fmt "\n", __FILE__, __LINE__, __FUNCTION__, ##args) 
	
#define SIMPLY(level, fmt, args...) write_log(level, fmt "\n", ##args)

#define ERROR_LOG(fmt, args...)	DETAIL(APP_ERROR, fmt, ##args)
#define CRIT_LOG(fmt, args...)	DETAIL(APP_CRIT, fmt, ##args)
#define ALERT_LOG(fmt, args...)	DETAIL(APP_ALERT, fmt, ##args)
#define EMERG_LOG(fmt, args...)	DETAIL(APP_EMERG, fmt, ##args)

#define WARN_LOG(fmt, args...)	DETAIL(APP_WARNING, fmt, ##args)
#define NOTI_LOG(fmt, args...)	DETAIL(APP_NOTICE, fmt, ##args)
#define INFO_LOG(fmt, args...)	SIMPLY(APP_INFO, fmt, ##args)
#define DEBUG_LOG(fmt, args...)	SIMPLY(APP_DEBUG, fmt, ##args)
#define BOOT_LOG(OK, fmt, args...) do{ \
	boot_log(OK, 0, fmt, ##args); \
	return OK; \
}while (0)

#define BOOT_LOG_EN(OK, fmt, args...) do{ \
	boot_log(OK, 0, fmt, ##args); \
}while (0)

#define BOOT_LOG2(OK, n, fmt, args...) do{ \
	boot_log(OK, n, fmt , ##args); \
	return OK; \
}while (0)

#define ERROR_RETURN(X, Y) do{ \
	ERROR_LOG X; \
	return Y; \
}while (0)

#ifdef DEBUG
#define TRACE_LOG(fmt,args...)	SIMPLY(APP_TRACE, fmt, ##args)
#else
#define TRACE_LOG(fmt,args...)	
#endif

void boot_log (int OK, int dummy, const char* fmt, ...);
void write_log (int lvl, const char* fmt, ...);
int log_init (const char* dir, int lvl, u_int size, const char* pre_name);
void log_close (void);

#ifdef __cplusplus
}
#endif

#endif //#ifndef LOG_H

