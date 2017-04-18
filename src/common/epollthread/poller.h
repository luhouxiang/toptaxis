/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


#ifndef __POLLER_H__
#define __POLLER_H__

#include <arpa/inet.h>
#include <sys/poll.h>
#include "myepoll.h"
#include "list.h"
#include "timestamp.h"

#define EPOLL_DATA_SLOT(x)	((x)->data.u64 & 0xFFFFFFFF)
#define EPOLL_DATA_SEQ(x)	((x)->data.u64 >> 32)

class CPollerUnit;
class CPollerObject;

#if 0
typedef struct _CEpollSlot 
{
	uint32_t seq;
	CPollerObject *poller;
	CEpollSlot *freeList;
}CEpollSlot;
#endif

struct CEpollSlot {
	uint32_t seq;
	CPollerObject *poller;
	struct CEpollSlot *freeList;
};

class CPollerObject 
{
public:
	CPollerObject (CPollerUnit *pCPollerUnit=NULL, int iNetFd = -1);
	virtual ~CPollerObject ();

	virtual void InputNotify (void);
	virtual void OutputNotify (void);
	virtual void HangupNotify (void);

	int AttachPoller (CPollerUnit *thread=NULL);
	int DetachPoller (void);
	int ApplyEvents ();
	int CheckLinkStatus(void);

	
	void EnableInput(void) 
	{
		newEvents |= EPOLLIN;
	}
	void EnableOutput(void) 
	{
		newEvents |= EPOLLOUT;
	}
	void DisableInput(void) 
	{
		newEvents &= ~EPOLLIN;
	}
	void DisableOutput(void) 
	{
		newEvents &= ~EPOLLOUT;
	}

	void EnableInput(bool i) 
	{
		if(i)
			newEvents |= EPOLLIN;
		else
			newEvents &= ~EPOLLIN;
	}
	void EnableOutput(bool o) 
	{
		if(o)
			newEvents |= EPOLLOUT;
		else
			newEvents &= ~EPOLLOUT;
	}


	friend class CPollerUnit;
	
	uint64_t m_ulLastAliveTime;
	

protected:
	int m_iNetFd;
	CPollerUnit *m_pCPollThread;
	int newEvents;
	int oldEvents;
	struct CEpollSlot *m_pObjEpollSlot;
};

class CPollerUnit {
public:
	friend class CPollerObject;
	CPollerUnit(unsigned int uiMaxPollers);
	~CPollerUnit();

	int InitializePollerUnit();
	void WaitPollerEvents(unsigned int uiTimeOut);
	void ProcessPollerEvents();
	int GetFD() { return m_iEpollFd; }

public:
	int VerifyEvents(struct epoll_event *);
	int Epctl (int op, int fd, struct epoll_event *events);
	int GetSlotId (struct CEpollSlot *p) {return ((char*)p - (char*)m_pEpollerTable) / sizeof (struct CEpollSlot);}

	void FreeEpollSlot (struct CEpollSlot *p);
	struct CEpollSlot *AllocEpollSlot ();

	epoll_event *m_pEpollEvent;
	int m_iEpollFd;
	unsigned int m_uiMaxPollers;
	unsigned int m_uiUsedEpoll;
	struct CEpollSlot *m_pFreeTableList;	
	struct CEpollSlot *m_pEpollerTable;

	 int m_uiEventNum;
};

#endif
