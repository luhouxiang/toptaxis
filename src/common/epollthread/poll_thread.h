/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


#ifndef _POLL_THREAD_H__
#define _POLL_THREAD_H__


#include <sys/types.h>
#include <unistd.h>


#include "poller.h"
#include "timerlist.h"

class CPollThread : public CPollerUnit, public CTimerUnit
{
public:
	CPollThread (const char *name, unsigned int uiMaxPollers);
	virtual ~CPollThread ();

	int InitializeThread();
	int RunningThread ();
        void DeleteUnAliveLink(uint64_t now);
	const char *Name(void) { return m_szThreadName; }

protected:
	char m_szThreadName[32];
	unsigned int m_uiTimeOut;

	uint64_t m_ulNextCheckTime;  /*链路激活保持为24小时*/
	
private:
	virtual void Cleanup(void);
};

#endif
