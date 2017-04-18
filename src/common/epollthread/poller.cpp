/**
 * Tencent is pleased to support the open source community by making CTAXIS available.  CTAXIS is licensed under the Apache License, Version 2.0, and a copy of the license is included in this file.
 * Copyright (C) 2014 THL A29 Limited, a Tencent company.  All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
 */


/*************************************************************************** 
 *       文件名称:  poller.cpp
 *       文件功能:  定义epoll 的基本类CPollerUnit，及epoll 监听对象的基本类CPollerObject
 				      1: CPollerObject 为监听类CListener 的父类
 				      2: CPollerObject 为客户端连接类CClientSync 的父类
 				      3: CPollerUnit 为主工作进程类CPollThread 的父类
 *       文件作者:   
 *       修改时间:  2011.03.22 
 *       修改记录:  
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
 *       类      名称: CPollerObject
 *       类      功能: epoll 监听对象的基本类CPollerObject
 *       文件作者:  
 *       修改时间: 2011.03.21 
 *       修改记录:  
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
	/*释放epoll 基础类中的监听基本单元*/
	if (m_pCPollThread && m_pObjEpollSlot)
	{
		m_pCPollThread->FreeEpollSlot (m_pObjEpollSlot);
	}
		
	/*如果fd 存在就关闭*/
	if (m_iNetFd > 0) 
	{
		close (m_iNetFd);
		m_iNetFd = -1;
	}
}

/*************************************************************
*函数名称: AttachPoller()
*输入参数: 略
*返回参数: 略
*函数功能: 将fd 挂载到主线程的epoll 上去
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

	/*申请epoll 监控的基本单元*/
	if(NULL== m_pObjEpollSlot) 
	{
		if (!(m_pObjEpollSlot = m_pCPollThread->AllocEpollSlot ()))
		{
			return -1;
		}
			
		m_pObjEpollSlot->poller = this;

		/*设置为非阻塞的lt 模式*/
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
*函数名称: DetachPoller()
*输入参数: 略
*返回参数: 略
*函数功能: 从epoll 中删除监听的fd
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
*函数名称: ApplyEvents()
*输入参数: 略
*返回参数: 略
*函数功能: 更新该fd 对应的epoll 的监听事件
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
*函数名称: CheckLinkStatus()
*输入参数: 略
*返回参数: 略
*函数功能: 检查链路状态
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
 *       类      名称: CPollerUnit
 *       类      功能: epoll 的基本类CPollerUnit
 *       文件作者:  
 *       修改时间: 2011.03.21 
 *       修改记录:  
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
*函数名称: InitializePollerUnit()
*输入参数: 略
*返回参数: 略
*函数功能: 初始化epoll 单元
************************************************************/
int CPollerUnit::InitializePollerUnit()
{

	/*申请epoll监听的基本单位*/
	m_pEpollerTable = (struct CEpollSlot*)calloc(m_uiMaxPollers, sizeof (struct CEpollSlot));
	if (NULL == m_pEpollerTable)
	{
		EMERG_LOG("CALLOC  m_pEpollerTable failed, num=%d", m_uiMaxPollers);
		return -1;
	}

	/*生成基本单位的空闲链表*/
	for (unsigned int i = 0; i < m_uiMaxPollers - 1; i++)
	{
		m_pEpollerTable[i].freeList = &m_pEpollerTable[i+1];
	}
	m_pEpollerTable[m_uiMaxPollers - 1].freeList = NULL;
	m_pFreeTableList = &m_pEpollerTable[0];


	/*申请epoll监听的事件*/
	m_pEpollEvent = (struct epoll_event *)calloc(m_uiMaxPollers, sizeof (struct epoll_event));
	if (NULL == m_pEpollEvent)
	{
		EMERG_LOG("CALLOC  m_pEpollEvent failed, num=%d", m_uiMaxPollers);
		return -1;
	}

	/*创建epoll*/
	if ((m_iEpollFd = epoll_create (m_uiMaxPollers)) == -1)
	{
		EMERG_LOG("epoll_create failed");
		return -1;
	}

	/*子进程不需要继承该fd*/
	fcntl(m_iEpollFd, F_SETFD, FD_CLOEXEC);
	
	return 0;
}

/*************************************************************
*函数名称: VerifyEvents()
*输入参数: 略
*返回参数: 略
*函数功能: 检查事件的合法性
			    1: 检查对应的连接是否存在
			    2: 检查消息序号是否为递增的序号
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
*函数名称: FreeEpollSlot()
*输入参数: 略
*返回参数: 略
*函数功能: 连接断掉时，释放epoll的基本单位
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
*函数名称: AllocEpollSlot()
*输入参数: 略
*返回参数: 略
*函数功能: 有新连接到来时，申请epoll的基本单位
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
*函数名称: Epctl()
*输入参数: 略
*返回参数: 略
*函数功能: 将连接的事件加入到epoll中
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
*函数名称: WaitPollerEvents()
*输入参数: 略
*返回参数: 略
*函数功能: 阻塞式等待epoll
************************************************************/
void CPollerUnit::WaitPollerEvents(unsigned int uiTimeOut) 
{
	m_uiEventNum = epoll_wait (m_iEpollFd, m_pEpollEvent, m_uiMaxPollers, uiTimeOut);
}


/*************************************************************
*函数名称: ProcessPollerEvents()
*输入参数: 略
*返回参数: 略
*函数功能: 处理epoll事件
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

		/*如果链路错误消息，则删除客户端*/
		if(m_pEpollEvent[i].events & (EPOLLHUP | EPOLLERR))
		{
			/*如果对端发送了RST，那么本端的epoll事件会同时收到 EPOLLIN|EPOLLERR|EPOLLHUP;
			所以此处要先处理EPOLLHUP | EPOLLERR*/
			p->HangupNotify();
			continue;
		}

		/*如果输入消息，则调用InputNotify处理*/
		if(m_pEpollEvent[i].events & EPOLLIN)
		{
			p->InputNotify();
		}
			
		/*如果输出消息，则调用OutputNotify处理*/
		if(s->poller==p && m_pEpollEvent[i].events & EPOLLOUT)
		{
			p->OutputNotify();
		}
			
		/*修正监听事件*/
		if(s->poller==p)
		{
			p->ApplyEvents();
		}
	}
}

