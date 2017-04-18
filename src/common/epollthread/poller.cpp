/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


/*************************************************************************** 
 *       �ļ�����:  poller.cpp
 *       �ļ�����:  ����epoll �Ļ�����CPollerUnit����epoll ��������Ļ�����CPollerObject
 				      1: CPollerObject Ϊ������CListener �ĸ���
 				      2: CPollerObject Ϊ�ͻ���������CClientSync �ĸ���
 				      3: CPollerUnit Ϊ������������CPollThread �ĸ���
 *       �ļ�����:   
 *       �޸�ʱ��:  2011.03.22 
 *       �޸ļ�¼:  
 ***************************************************************************/
 
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>


#include "poller.h"
#include "log.h"


/*************************************************************************** 
 *       ��      ����: CPollerObject
 *       ��      ����: epoll ��������Ļ�����CPollerObject
 *       �ļ�����:  
 *       �޸�ʱ��: 2011.03.21 
 *       �޸ļ�¼:  
 ***************************************************************************/

CPollerObject::CPollerObject (CPollerUnit *pCPollerUnit, int iNetFd)
{
	m_iNetFd = iNetFd;
	m_pCPollThread = pCPollerUnit;
	newEvents = 0;
	oldEvents = 0;
	m_pObjEpollSlot = NULL;
}

CPollerObject::~CPollerObject ()
{
	/*�ͷ�epoll �������еļ���������Ԫ*/
	if (m_pCPollThread && m_pObjEpollSlot)
	{
		m_pCPollThread->FreeEpollSlot (m_pObjEpollSlot);
	}
		
	/*���fd ���ھ͹ر�*/
	if (m_iNetFd > 0) 
	{
		close (m_iNetFd);
		m_iNetFd = -1;
	}
}

/*************************************************************
*��������: AttachPoller()
*�������: ��
*���ز���: ��
*��������: ��fd ���ص����̵߳�epoll ��ȥ
************************************************************/
int CPollerObject::AttachPoller (CPollerUnit *pPollerUnit)
{
	if(pPollerUnit) 
	{
		m_pCPollThread = pPollerUnit;
	}

	if(m_iNetFd < 0)
	{
		return -1;
	}

	m_ulLastAliveTime = GET_TIMESTAMP();

	/*����epoll ��صĻ�����Ԫ*/
	if(NULL== m_pObjEpollSlot) 
	{
		if (!(m_pObjEpollSlot = m_pCPollThread->AllocEpollSlot ()))
		{
			return -1;
		}
			
		m_pObjEpollSlot->poller = this;

		/*����Ϊ��������lt ģʽ*/
		int flag = fcntl (m_iNetFd, F_GETFL);
		fcntl (m_iNetFd, F_SETFL, O_NONBLOCK | flag);
		
		struct epoll_event ev;
		ev.events = newEvents;
		ev.data.u64 = (++m_pObjEpollSlot->seq);
		ev.data.u64 = (ev.data.u64 << 32) + m_pCPollThread->GetSlotId (m_pObjEpollSlot);
		if (m_pCPollThread->Epctl (EPOLL_CTL_ADD, m_iNetFd, &ev) == 0)
		{
			oldEvents = newEvents;
		}
		else 
		{
			CRIT_LOG("reason[epctl_failed]fd[%d]" ,m_iNetFd);
			return -1;
		}
		return 0;

	}


	return ApplyEvents ();
}



/*************************************************************
*��������: DetachPoller()
*�������: ��
*���ز���: ��
*��������: ��epoll ��ɾ��������fd
************************************************************/
int CPollerObject::DetachPoller() 
{
	if(m_pObjEpollSlot) 
	{
		struct epoll_event ev;
		if (m_pCPollThread->Epctl (EPOLL_CTL_DEL, m_iNetFd, &ev) == 0)
		{
			oldEvents = newEvents;
		}
		else 
		{
			CRIT_LOG("reason[epctl_failed]fd[%d]" ,m_iNetFd);
			return -1;
		}
		m_pCPollThread->FreeEpollSlot(m_pObjEpollSlot);
		m_pObjEpollSlot = NULL;
	}
	return 0;
}


/*************************************************************
*��������: ApplyEvents()
*�������: ��
*���ز���: ��
*��������: ���¸�fd ��Ӧ��epoll �ļ����¼�
************************************************************/
int CPollerObject::ApplyEvents ()
{
	if (( NULL== m_pObjEpollSlot )|| (oldEvents == newEvents))
	{
		return 0;
	}
		
	struct epoll_event ev;

	ev.events = newEvents;
	ev.data.u64 = (++m_pObjEpollSlot->seq);
	ev.data.u64 = (ev.data.u64 << 32) + m_pCPollThread->GetSlotId (m_pObjEpollSlot);
	if (m_pCPollThread->Epctl (EPOLL_CTL_MOD, m_iNetFd, &ev) == 0)
	{
		oldEvents = newEvents;
	}
	else 
	{
		CRIT_LOG("reason[epctl_failed]fd[%d]" ,m_iNetFd);
		return -1;
	}

	return 0;
}


/*************************************************************
*��������: CheckLinkStatus()
*�������: ��
*���ز���: ��
*��������: �����·״̬
************************************************************/
int CPollerObject::CheckLinkStatus(void)
{
	char msg[1] = {0};
	int err = 0;

	err = recv(m_iNetFd, msg, sizeof(msg), MSG_DONTWAIT|MSG_PEEK);

	/* client already close connection. */
	if(err == 0 )
	{
		return -1;
	}

	if((err < 0) &&(errno != EAGAIN)&&(EINTR != errno) )
	{
		return -1;
	}
		
	return 0;
}


void CPollerObject::InputNotify(void) 
{
	EnableInput(false);
}

void CPollerObject::OutputNotify(void) 
{
	EnableOutput(false);
}

void CPollerObject::HangupNotify(void) 
{
	delete this;
}


/*************************************************************************** 
 *       ��      ����: CPollerUnit
 *       ��      ����: epoll �Ļ�����CPollerUnit
 *       �ļ�����:  
 *       �޸�ʱ��: 2011.03.21 
 *       �޸ļ�¼:  
 ***************************************************************************/
CPollerUnit::CPollerUnit(unsigned int uiMaxPollers)
{
	m_uiMaxPollers = uiMaxPollers;
	m_iEpollFd = -1;
	m_pEpollEvent = NULL;
	m_pEpollerTable = NULL;
	m_pFreeTableList = NULL;
	m_uiUsedEpoll = 0;
	m_uiEventNum = 0;
}

CPollerUnit::~CPollerUnit() 
{
	if (m_iEpollFd != -1)
	{
		close (m_iEpollFd);
		m_iEpollFd = -1;
	}

	
	if(NULL != m_pEpollerTable)
	{
		free(m_pEpollerTable);
	}
	


	if(NULL != m_pEpollEvent)
	{
		free(m_pEpollEvent);
	}
	

}


/*************************************************************
*��������: InitializePollerUnit()
*�������: ��
*���ز���: ��
*��������: ��ʼ��epoll ��Ԫ
************************************************************/
int CPollerUnit::InitializePollerUnit()
{

	/*����epoll�����Ļ�����λ*/
	m_pEpollerTable = (struct CEpollSlot*)calloc(m_uiMaxPollers, sizeof (struct CEpollSlot));
	if (NULL == m_pEpollerTable)
	{
		EMERG_LOG("CALLOC  m_pEpollerTable failed, num=%d", m_uiMaxPollers);
		return -1;
	}

	/*���ɻ�����λ�Ŀ�������*/
	for (unsigned int i = 0; i < m_uiMaxPollers - 1; i++)
	{
		m_pEpollerTable[i].freeList = &m_pEpollerTable[i+1];
	}
	m_pEpollerTable[m_uiMaxPollers - 1].freeList = NULL;
	m_pFreeTableList = &m_pEpollerTable[0];


	/*����epoll�������¼�*/
	m_pEpollEvent = (struct epoll_event *)calloc(m_uiMaxPollers, sizeof (struct epoll_event));
	if (NULL == m_pEpollEvent)
	{
		EMERG_LOG("CALLOC  m_pEpollEvent failed, num=%d", m_uiMaxPollers);
		return -1;
	}

	/*����epoll*/
	if ((m_iEpollFd = epoll_create (m_uiMaxPollers)) == -1)
	{
		EMERG_LOG("epoll_create failed");
		return -1;
	}

	/*�ӽ��̲���Ҫ�̳и�fd*/
	fcntl(m_iEpollFd, F_SETFD, FD_CLOEXEC);
	
	return 0;
}

/*************************************************************
*��������: VerifyEvents()
*�������: ��
*���ز���: ��
*��������: ����¼��ĺϷ���
			    1: ����Ӧ�������Ƿ����
			    2: �����Ϣ����Ƿ�Ϊ���������
************************************************************/
inline int CPollerUnit::VerifyEvents (struct epoll_event *ev)
{
	unsigned int idx = EPOLL_DATA_SLOT (ev);

	if ((idx >= m_uiMaxPollers) || (EPOLL_DATA_SEQ (ev) != m_pEpollerTable[idx].seq))
	{
		return -1;
	}

	if(m_pEpollerTable[idx].poller == NULL || m_pEpollerTable[idx].freeList != NULL)
	{
		CRIT_LOG("reason[invoid_epoll] idx[%u]seq[%u]poller[%p]freelist[%p]event[%x]",
				idx, (unsigned int)EPOLL_DATA_SEQ(ev), m_pEpollerTable[idx].poller, 
				m_pEpollerTable[idx].freeList, ev->events);

		return -1;
	}
	return 0;
}

/*************************************************************
*��������: FreeEpollSlot()
*�������: ��
*���ز���: ��
*��������: ���Ӷϵ�ʱ���ͷ�epoll�Ļ�����λ
************************************************************/
void CPollerUnit::FreeEpollSlot (struct CEpollSlot *p)
{
	p->freeList = m_pFreeTableList;
	m_pFreeTableList = p;
	m_uiUsedEpoll--;
	p->seq++;
	p->poller = NULL;
}


/*************************************************************
*��������: AllocEpollSlot()
*�������: ��
*���ز���: ��
*��������: �������ӵ���ʱ������epoll�Ļ�����λ
************************************************************/
struct CEpollSlot *CPollerUnit::AllocEpollSlot ()
{
	struct CEpollSlot *p = m_pFreeTableList;

	if (NULL== p) 
	{
		CRIT_LOG("reason[lack_epoll_slot]used[%u]total[%u]", m_uiUsedEpoll,m_uiMaxPollers);
		return NULL;
	}
	
	m_uiUsedEpoll++;
	m_pFreeTableList = m_pFreeTableList->freeList;
	p->freeList = NULL;

	return p;
}


/*************************************************************
*��������: Epctl()
*�������: ��
*���ز���: ��
*��������: �����ӵ��¼����뵽epoll��
************************************************************/
int CPollerUnit::Epctl (int op, int fd, struct epoll_event *events)
{
	if (epoll_ctl (m_iEpollFd,  op, fd, events) == -1)
 	{
		CRIT_LOG("reason[epoll_ctl_failed]epfd[%d]fd[%d]", m_iEpollFd, fd);
		return -1;
	}

	return 0;
}

/*************************************************************
*��������: WaitPollerEvents()
*�������: ��
*���ز���: ��
*��������: ����ʽ�ȴ�epoll
************************************************************/
void CPollerUnit::WaitPollerEvents(unsigned int uiTimeOut) 
{
	m_uiEventNum = epoll_wait (m_iEpollFd, m_pEpollEvent, m_uiMaxPollers, uiTimeOut);
}


/*************************************************************
*��������: ProcessPollerEvents()
*�������: ��
*���ز���: ��
*��������: ����epoll�¼�
************************************************************/
void CPollerUnit::ProcessPollerEvents() 
{
	for ( int i = 0; i < m_uiEventNum; i++)
	{
		if(VerifyEvents (m_pEpollEvent+i) == -1)
		{
			CRIT_LOG("reason[VerifyEvents_failed]u64[%llu]", (unsigned long long)m_pEpollEvent[i].data.u64);
			continue;
		}

		struct CEpollSlot *s = &m_pEpollerTable[EPOLL_DATA_SLOT(m_pEpollEvent+i)];
		CPollerObject *p = s->poller;

		p->newEvents = p->oldEvents;

		/*�����·������Ϣ����ɾ���ͻ���*/
		if(m_pEpollEvent[i].events & (EPOLLHUP | EPOLLERR))
		{
			/*����Զ˷�����RST����ô���˵�epoll�¼���ͬʱ�յ� EPOLLIN|EPOLLERR|EPOLLHUP;
			���Դ˴�Ҫ�ȴ���EPOLLHUP | EPOLLERR*/
			p->HangupNotify();
			continue;
		}

		/*���������Ϣ�������InputNotify����*/
		if(m_pEpollEvent[i].events & EPOLLIN)
		{
			p->InputNotify();
		}
			
		/*��������Ϣ�������OutputNotify����*/
		if(s->poller==p && m_pEpollEvent[i].events & EPOLLOUT)
		{
			p->OutputNotify();
		}
			
		/*���������¼�*/
		if(s->poller==p)
		{
			p->ApplyEvents();
		}
	}
}

